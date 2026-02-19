/* file: xgb_recv.h
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-12-22
 */

#ifndef _XGB_RECV_H_
#define _XGB_RECV_H_

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ring_buffer.h"
#include "debug_macros.h"

/*
 * Useful typedefs
 */

typedef int socket_t;
typedef struct sockaddr_in SA_in;
typedef struct sockaddr SA;

/*
 * Structure Definitions
 */

typedef struct {
	RING_BUFFER *pkt_buffer;
} THREAD_ARGS;

/*
 * Function Declarations
 */

int receive_packets();
void *net_thread_function(void *arg);
void *hdd_thread_function(void *arg);
void *vis_thread_function(void *arg);

socket_t setup_network_listener();
int wait_for_readable(socket_t sock, int timeout_sec);
int open_output_file(const char *path);
void cleanup(int signal);

extern volatile sig_atomic_t run_net_thread;

#endif /* _XGB_RECV_H_ */
