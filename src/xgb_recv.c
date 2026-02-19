/* file: xgb_recv.c
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-12-22
 */

#include "xgb_recv.h"

#define LISTEN_PORT 8888
#define NUM_PACKET_SLOTS 10
#define MAX_PAYLOAD_SIZE 8192
#define CAPTURE_FILE "raw_capture.dat"
#define SELECT_TIMEOUT_SEC 1
#define VIS_SLEEP_US 100000
#define RECV_BUF_SIZE (2 * 1024 * 1024)

volatile sig_atomic_t run_net_thread = 1;

/*
 * Wrapper for stand-alone testing.
 */
int main(int argc, char **argv)
{
	return receive_packets();
}

/*
 * Set up and spawn three threads.
 * net_thread writes data from the nic into a ring buffer.
 * hdd_thread reads data from a ring buffer to hard disk.
 * vis_thread draws live ring buffer stats on stderr.
 * Wait for all threads to re-join.
 */
int receive_packets()
{
	RING_BUFFER *pkt_buffer = ring_buffer_create(NUM_PACKET_SLOTS, MAX_PAYLOAD_SIZE);
	if (pkt_buffer == NULL)
	{
		fprintf(stderr, "Failed to create packet ring buffer.\n");
		return 1;
	}

	THREAD_ARGS thread_args;
	thread_args.pkt_buffer = pkt_buffer;

	// start listening for Ctrl-C
	signal(SIGINT, cleanup);

	// make stdout unbuffered
	setbuf(stdout, NULL);

	pthread_t net_thread, hdd_thread, vis_thread;
	pthread_create(&net_thread, NULL, net_thread_function, &thread_args);
	pthread_create(&hdd_thread, NULL, hdd_thread_function, &thread_args);
	pthread_create(&vis_thread, NULL, vis_thread_function, &thread_args);

	pthread_join(net_thread, NULL);
	pthread_join(hdd_thread, NULL);
	pthread_join(vis_thread, NULL);

	ring_buffer_delete(pkt_buffer);

	return 0;
}

/*
 * Read data from the network.
 * Write data to ring buffer.
 */
void *net_thread_function(void *arg)
{
	int ready = 0;

	THREAD_ARGS *args = (THREAD_ARGS *)arg;
	RING_BUFFER *pkt_buffer = args->pkt_buffer;

	RING_ITEM *this_slot = pkt_buffer->slots;
	RING_ITEM *next_slot = NULL;

	socket_t sock = setup_network_listener();
	void *buffer = NULL;
	int flags = 0;
	SA_in addr; // packet source's address
	socklen_t addr_len = sizeof(addr);
	ssize_t num_bytes = 0;

	debug_fprintf(stderr, "Entering network thread loop.\n");

	/*
	 * loop forever:
	 *   update relevant local pointers,
	 *   wait for next free buffer slot,
	 *   grab current buffer slot write_mutex,
	 *   read data from network into the slot,
	 *   validate received data,
	 *   release the buffer slot read_mutex,
	 *   advance write pointer to next buffer slot.
	 */
	while (run_net_thread)
	{
		// Responses:
		// 1: socket has data
		// 0: nothing to read; keep waiting
		// -1: read error or Ctrl+C
		ready = wait_for_readable(sock, SELECT_TIMEOUT_SEC);
		if (ready == 0)
		{
			continue;
		}
		if (ready == -1)
		{
			perror("select");
			break;
		}

		next_slot = this_slot->next;
		buffer = this_slot->payload;

		slot_wait_writable(this_slot);
		num_bytes = recvfrom(sock, buffer, MAX_PAYLOAD_SIZE, flags, (SA *)&addr, &addr_len);

		if (num_bytes == -1)
		{
			slot_mark_writable(this_slot);

			// signal arrived between select() and recvfrom();
			// return to start of loop and exit cleanly.
			if (errno == EINTR)
				continue;

			perror("Unable to receive packet");
			break;
		}

		this_slot->size = num_bytes;
		slot_mark_readable(this_slot);

		debug_fprintf(stderr, "[net thread] Received %zd bytes.\n", num_bytes);

		this_slot = next_slot;
	} // end while

	debug_fprintf(stderr, "Exiting network thread loop.\n");

	close(sock);

	// Post a zero-size sentinel to tell the hdd thread to shut
	// down. It will drain all remaining buffered slots first,
	// then hit this sentinel and exit.
	this_slot->size = 0;
	slot_mark_readable(this_slot);

	return NULL;
}

/*
 * Read data from ring buffer.
 * Write data to file on disk.
 */
void *hdd_thread_function(void *arg)
{
	THREAD_ARGS *args = (THREAD_ARGS *)arg;
	RING_BUFFER *pkt_buffer = args->pkt_buffer;

	RING_ITEM *this_slot = pkt_buffer->slots;
	RING_ITEM *next_slot = NULL;

	int fd = open_output_file(CAPTURE_FILE);
	ssize_t num_bytes = 0;

	debug_fprintf(stderr, "Entering hard disk thread loop.\n");

	/*
	 * loop forever:
	 *   update relevant local pointers,
	 *   wait for next full buffer slot,
	 *   grab current buffer slot read_mutex,
	 *   (exit if the slot is a zero-size sentinel),
	 *   write data from the buffer to a file,
	 *   validate written data based on length,
	 *   release the buffer slot write_mutex,
	 *   advance read pointer to next buffer slot.
	 */
	while (true)
	{
		next_slot = this_slot->next;

		slot_wait_readable(this_slot);

		// A zero-size sentinel means the net thread finished,
		// and the hdd thread has drained all remaining data.
		if (this_slot->size == 0)
		{
			slot_mark_writable(this_slot);
			break;
		}

		num_bytes = write(fd, this_slot->payload, this_slot->size);

		if (num_bytes == -1)
		{
			slot_mark_writable(this_slot);
			perror("Unable to write packet");
			exit(1);
		}
		else if (num_bytes != this_slot->size)
		{
			slot_mark_writable(this_slot);
			fprintf(stderr, "Short write: %zd of %zu bytes.\n", num_bytes, this_slot->size);
			exit(1);
		}

		slot_mark_writable(this_slot);

		debug_fprintf(stderr, "[hdd thread] Wrote %zd bytes.\n", num_bytes);

		this_slot = next_slot;
	} // end while

	debug_fprintf(stderr, "Exiting hard disk thread loop.\n");

	close(fd);

	return NULL;
}

/*
 * Poll ring buffer counters and draw a live status line on stderr.
 */
void *vis_thread_function(void *arg)
{
	THREAD_ARGS *args = (THREAD_ARGS *)arg;
	RING_BUFFER *pkt_buffer = args->pkt_buffer;
	size_t num_slots = pkt_buffer->num_slots;

	size_t i;
	size_t filled = 0;
	size_t rx = 0;
	size_t wr = 0;

	fprintf(stderr, "Receiving packets. Ctrl+C to quit.\n");

	while (run_net_thread)
	{
		usleep(VIS_SLEEP_US);

		filled = atomic_load(&pkt_buffer->slots_filled);
		rx = atomic_load(&pkt_buffer->total_produced);
		wr = atomic_load(&pkt_buffer->total_consumed);

		fprintf(stderr, "\rbuf [");
		for (i = 0; i < num_slots; i++)
		{
			fputc(i < filled ? '#' : '.', stderr);
		}
		fprintf(stderr, "] %zu/%zu  received: %zu  written: %zu\033[K", filled, num_slots, rx, wr);
	}

	fprintf(stderr, "\n");

	return NULL;
}

/*
 * Bind to a port and listen for incoming data.
 */
int setup_network_listener()
{
	int sock = -1;
	struct sockaddr_in my_addr; // server's address information
	int ret = 0;

	// create a new UDP socket descriptor
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		perror("Unable to set socket descriptor");
		exit(1);
	}

	// initialize local address struct
	my_addr.sin_family = AF_INET; // host byte order
	my_addr.sin_port = htons(LISTEN_PORT); // short, network byte order
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listen on all interfaces
	memset(my_addr.sin_zero, 0, sizeof(my_addr.sin_zero));

	// prevent "address already in use" errors
	const int on = 1;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));
	if (ret == -1)
	{
		perror("Unable to reuse addresses");
		exit(1);
	}

	// request a larger kernel socket receive buffer
	const int rcvbuf = RECV_BUF_SIZE;
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuf, sizeof(rcvbuf));
	if (ret == -1)
	{
		perror("Unable to set receive buffer size");
	}

	// bind socket to local address
	ret = bind(sock, (SA *)&my_addr, sizeof(my_addr));
	if (ret == -1)
	{
		perror("Unable to bind to socket");
		exit(1);
	}

	debug_fprintf(stderr, "Listening on IP address %s on port %i\n", inet_ntoa(my_addr.sin_addr), LISTEN_PORT);

	return sock;
}

/*
 * Wait for socket to become readable.
 * Returns 1 if readable, 0 on timeout or signal, -1 on error.
 */
int wait_for_readable(socket_t sock, int timeout_sec)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	struct timeval tv = {
		.tv_sec = timeout_sec,
		.tv_usec = 0,
	};

	int ret = select(sock + 1, &readfds, NULL, NULL, &tv);
	if (ret == -1 && errno == EINTR)
		return 0; // treat signal like a timeout
	return ret;
}

/*
 * Open a file on disk for writing ring buffer data.
 */
int open_output_file(const char *path)
{
	int flags = O_CREAT|O_TRUNC|O_WRONLY;
	mode_t mode = S_IREAD | S_IWUSR;
	int fd = -1;

	fd = open(path, flags, mode);
	if (fd == -1)
	{
		perror("Unable to open capture file");
		exit(1);
	}

	return fd;
}

/*
 * Ctrl-C handler.
 */
void cleanup(int signal)
{
	run_net_thread = 0;
}
