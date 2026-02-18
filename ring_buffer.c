/* file: ring_buffer.c
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-04-02
 */

#include "ring_buffer.h"

/*
 * Construct and initialize a RING_BUFFER.
 */
RING_BUFFER *ring_buffer_create(size_t num_slots, size_t buf_size)
{
	// create payload buffer
	void *payloads = calloc(buf_size, 1);
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

	int i;
	for(i=0; i<num_slots; i++)
	{
		RING_ITEM *this_item = &slots[i];
		RING_ITEM *next_item = &slots[(i + 1) % num_slots];

		this_item->next = next_item;
		sem_init(&this_item->write_mutex, 0, 1);
		sem_init(&this_item->read_mutex, 0, 0);
		this_item->payload = (uint8_t *)payloads + i * (buf_size / num_slots);
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

	return rb;
}

/*
 * Destroy a RING_BUFFER and free its memory.
 */
void ring_buffer_delete(RING_BUFFER *rb)
{
	free(rb->payloads);

	int i;
	for(i=0; i<rb->num_slots; i++)
	{
		sem_destroy(&rb->slots[i].write_mutex);
		sem_destroy(&rb->slots[i].read_mutex);
	}
	free(rb->slots);

	free(rb);
}
