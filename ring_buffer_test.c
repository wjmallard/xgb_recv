/* file: ring_buffer_test.c
 * auth: William Mallard
 * mail: wjm@berkeley.edu
 * date: 2008-10-20
 */

#include <stdio.h>
#include "ring_buffer.h"

int main (int argc, char **argv)
{
	int item_cnt = 4;
	int buf_size = 8192;

	printf("Creating %d byte ring buffer with %d items.\n", buf_size, item_cnt);
	RING_BUFFER *rb = ring_buffer_create(item_cnt, buf_size);
	printf("Success!\n");
	printf("\n");

	// write sequential numbers into each slot
	printf("Writing sequential values to each slot ...\n");
	RING_ITEM *item = rb->list_ptr;
	int i;
	for (i = 0; i < item_cnt; i++)
	{
		item->data[0] = i + 1;
		item->size = 1;
		item = item->next;
	}

	// read back and verify each slot still has its own value
	printf("Reading back and verifying ...\n");
	item = rb->list_ptr;
	for (i = 0; i < item_cnt; i++)
	{
		if (item->data[0] != (uint8_t)(i + 1) || item->size != 1)
		{
			printf("FAIL: slot %d data=%d size=%zu, expected data=%d size=1\n",
				i, item->data[0], item->size, i + 1);
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
