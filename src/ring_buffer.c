/* file: ring_buffer.c
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-04-02
 */

#include "ring_buffer.h"

/*
 * Construct and initialize a RING_BUFFER.
 */
RING_BUFFER *ring_buffer_create(size_t num_slots, size_t payload_size)
{
	// create payload buffer
	void *payloads = calloc(num_slots, payload_size);
	if (payloads == NULL)
	{
		perror("Failed to allocate ring buffer data");
		return NULL;
	}

	// create slots
	RING_ITEM *slots = (RING_ITEM *)calloc(num_slots, sizeof(RING_ITEM));
	if (slots == NULL)
	{
		perror("Failed to allocate ring buffer items");
		free(payloads);
		return NULL;
	}

	size_t i;
	for(i = 0; i < num_slots; i++)
	{
		RING_ITEM *this_item = &slots[i];
		RING_ITEM *next_item = &slots[(i + 1) % num_slots];

		this_item->next = next_item;
		pthread_mutex_init(&this_item->mutex, NULL);
		pthread_cond_init(&this_item->cond, NULL);
		this_item->readable = false;
		this_item->payload = (uint8_t *)payloads + i * payload_size;
		this_item->size = 0;
	}

	// create ring buffer
	RING_BUFFER *rb = (RING_BUFFER *)calloc(1, sizeof(RING_BUFFER));
	if (rb == NULL)
	{
		perror("Failed to allocate ring buffer struct");
		free(payloads);
		free(slots);
		return NULL;
	}

	rb->payloads = payloads;
	rb->slots = slots;
	rb->num_slots = num_slots;

	atomic_init(&rb->slots_filled, 0);
	atomic_init(&rb->total_produced, 0);
	atomic_init(&rb->total_consumed, 0);

	for(i = 0; i < num_slots; i++)
	{
		slots[i].parent = rb;
	}

	return rb;
}

/*
 * Destroy a RING_BUFFER and free its memory.
 */
void ring_buffer_delete(RING_BUFFER *rb)
{
	free(rb->payloads);

	size_t i;
	for(i = 0; i < rb->num_slots; i++)
	{
		pthread_mutex_destroy(&rb->slots[i].mutex);
		pthread_cond_destroy(&rb->slots[i].cond);
	}
	free(rb->slots);

	free(rb);
}

/*
 * Mark slot as writable (not readable) and signal waiters.
 */
void slot_mark_writable(RING_ITEM *slot)
{
	pthread_mutex_lock(&slot->mutex);
	slot->readable = false;
	pthread_cond_signal(&slot->cond);
	pthread_mutex_unlock(&slot->mutex);

	RING_BUFFER *rb = slot->parent;
	atomic_fetch_sub(&rb->slots_filled, 1);
	atomic_fetch_add(&rb->total_consumed, 1);
}

/*
 * Wait until slot is writable (not readable).
 */
void slot_wait_writable(RING_ITEM *slot)
{
	pthread_mutex_lock(&slot->mutex);
	while (slot->readable)
		pthread_cond_wait(&slot->cond, &slot->mutex);
	pthread_mutex_unlock(&slot->mutex);
}

/*
 * Mark slot as readable and signal waiters.
 */
void slot_mark_readable(RING_ITEM *slot)
{
	pthread_mutex_lock(&slot->mutex);
	slot->readable = true;
	pthread_cond_signal(&slot->cond);
	pthread_mutex_unlock(&slot->mutex);

	RING_BUFFER *rb = slot->parent;
	atomic_fetch_add(&rb->slots_filled, 1);
	atomic_fetch_add(&rb->total_produced, 1);
}

/*
 * Wait until slot is readable.
 */
void slot_wait_readable(RING_ITEM *slot)
{
	pthread_mutex_lock(&slot->mutex);
	while (!slot->readable)
		pthread_cond_wait(&slot->cond, &slot->mutex);
	pthread_mutex_unlock(&slot->mutex);
}
