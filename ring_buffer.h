/* file: ring_buffer.h
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-04-02
 */

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Structure Definitions
 */

typedef struct ring_item {
	struct ring_item *next;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool readable;
	uint8_t *payload;
	size_t size;
} RING_ITEM;

typedef struct ring_buffer {
	void *payloads;
	struct ring_item *slots;
	size_t num_slots;
} RING_BUFFER;

/*
 * Function Declarations
 */

RING_BUFFER *ring_buffer_create(size_t num_slots, size_t payload_size);
void ring_buffer_delete(RING_BUFFER *rb);

void slot_mark_writable(RING_ITEM *slot);
void slot_wait_writable(RING_ITEM *slot);
void slot_mark_readable(RING_ITEM *slot);
void slot_wait_readable(RING_ITEM *slot);

#endif // _RING_BUFFER_H_
