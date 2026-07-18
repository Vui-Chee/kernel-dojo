// SPDX-License-Identifier: GPL-2.0
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include "buffer.h"

struct ring_buffer *init_shared_buffer(unsigned int capacity, size_t el_size)
{
	struct ring_buffer *rb;
	size_t entries_size = array_size(capacity, el_size);
	size_t buf_size = struct_size(rb, entries, entries_size);
	int i;
	struct page *page;

	rb = vmalloc(buf_size);

	if (!rb)
		return NULL;

	memset(rb->entries, -1, entries_size);

	rb->capacity = capacity;
	rb->el_size = el_size;
	rb->head = 0;
	rb->tail = 0;
	spin_lock_init(&rb->lock);

	for (i = 0; i < DIV_ROUND_UP(buf_size, PAGE_SIZE); i++) {
		page = vmalloc_to_page((char *)rb + i * PAGE_SIZE);

		/* set reserved pages */
		if (page)
			SetPageReserved(page);
	}

	return rb;
}

void free_shared_buffer(struct ring_buffer *rb)
{
	size_t entries_size, buf_size;

	if (!rb)
		panic("Cannot free NULL ring buffer!\n");

	entries_size = array_size(rb->capacity, rb->el_size);
	buf_size = struct_size(rb, entries, entries_size);

	for (int i = 0; i < DIV_ROUND_UP(buf_size, PAGE_SIZE); i++) {
		struct page *page = vmalloc_to_page((char *)rb + i * PAGE_SIZE);

		if (page)
			ClearPageReserved(page);
	}

	vfree(rb);
}
