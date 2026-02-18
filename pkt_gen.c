#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc != 6) {
		fprintf(stderr, "Usage: pkt_gen <host> <port> <pkt_size> <pkt_count> <pkt_interval_ms>\n");
		return 1;
	}

	const char *host = argv[1];
	const char *port = argv[2];
	int pkt_size = atoi(argv[3]);
	int pkt_count = atoi(argv[4]);
	int pkt_interval_ms = atoi(argv[5]);

	if (pkt_size < 4) {
		fprintf(stderr, "error: pkt_size must be >= 4\n");
		return 1;
	}
	if (pkt_count < 0) {
		fprintf(stderr, "error: pkt_count must be >= 0\n");
		return 1;
	}
	if (pkt_interval_ms < 0) {
		fprintf(stderr, "error: pkt_interval_ms must be >= 0\n");
		return 1;
	}

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int err = getaddrinfo(host, port, &hints, &res);
	if (err) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return 1;
	}

	int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd < 0) {
		perror("socket");
		freeaddrinfo(res);
		return 1;
	}

	unsigned char *buf = calloc(pkt_size, 1);
	if (!buf) {
		perror("calloc");
		close(fd);
		freeaddrinfo(res);
		return 1;
	}

	for (int i = 0; i < pkt_count; i++) {
		uint32_t seq = htonl((uint32_t)i);
		memcpy(buf, &seq, 4);

		ssize_t sent = sendto(fd, buf, pkt_size, 0, res->ai_addr, res->ai_addrlen);
		if (sent < 0) {
			perror("sendto");
			free(buf);
			close(fd);
			freeaddrinfo(res);
			return 1;
		}

		if (pkt_interval_ms > 0)
			usleep(pkt_interval_ms * 1000);
	}

	fprintf(stderr, "sent %d packets, %lld bytes total\n",
		pkt_count, (long long)pkt_count * pkt_size);

	free(buf);
	close(fd);
	freeaddrinfo(res);
	return 0;
}
