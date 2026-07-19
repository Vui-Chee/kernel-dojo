// SPDX-License-Identifier: GPL-2.0
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include "buffer.h"

#define EMPTY -1

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

	memset(rb->entries, EMPTY, entries_size);

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
EXPORT_SYMBOL_GPL(init_shared_buffer);

/* For simplicity, overwrite tail when full. */
void insert_buffer(struct ring_buffer *rb, void *entry)
{
	void *dst;

	spin_lock(&rb->lock);

	if (rb->head == rb->tail)
		rb->tail = (rb->tail + 1) % rb->capacity;

	dst = rb->entries + rb->head * rb->el_size;
	memcpy(dst, entry, rb->el_size);
	rb->tail = (rb->head + 1) % rb->capacity;
}

void free_shared_buffer(struct ring_buffer *rb)
{
	size_t entries_size, buf_size;

	if (!rb)
		return;

	entries_size = array_size(rb->capacity, rb->el_size);
	buf_size = struct_size(rb, entries, entries_size);

	for (int i = 0; i < DIV_ROUND_UP(buf_size, PAGE_SIZE); i++) {
		struct page *page = vmalloc_to_page((char *)rb + i * PAGE_SIZE);

		if (page)
			ClearPageReserved(page);
	}

	vfree(rb);
}
EXPORT_SYMBOL_GPL(free_shared_buffer);
