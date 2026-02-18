/* file: ring_buffer_test.c
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-10-20
 */

#include <stdio.h>
#include "ring_buffer.h"

int main (int argc, char **argv)
{
	int num_slots = 4;
	int payload_size = 2048;

	printf("Creating ring buffer with %d slots of %d bytes.\n", num_slots, payload_size);
	RING_BUFFER *rb = ring_buffer_create(num_slots, payload_size);
	printf("Success!\n");
	printf("\n");

	// write sequential numbers into each slot
	printf("Writing sequential values to each slot ...\n");
	RING_ITEM *item = rb->slots;
	int i;
	for (i = 0; i < num_slots; i++)
	{
		item->payload[0] = i + 1;
		item->size = 1;
		item = item->next;
	}

	// read back and verify each slot still has its own value
	printf("Reading back and verifying ...\n");
	item = rb->slots;
	for (i = 0; i < num_slots; i++)
	{
		if (item->payload[0] != (uint8_t)(i + 1) || item->size != 1)
		{
			printf("FAIL: slot %d data=%d size=%zu, expected data=%d size=1\n",
				i, item->payload[0], item->size, i + 1);
			return 1;
		}
		item = item->next;
	}
	printf("Success!\n");
	printf("\n");

	printf("Deleting ring buffer.\n");
	ring_buffer_delete(rb);
	printf("Success!\n");

	return 0;
}
