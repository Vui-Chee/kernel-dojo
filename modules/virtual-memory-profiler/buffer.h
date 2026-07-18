/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BUFFER_H
#define BUFFER_H

struct ring_buffer *init_shared_buffer(unsigned int capacity, size_t el_size);
void free_shared_buffer(struct ring_buffer *rb);

struct ring_buffer {
	size_t el_size;
	unsigned int capacity;
	unsigned int head;
	unsigned int tail;
	spinlock_t lock;

	/* trailing array for easy init */
	char entries[];
};

#endif
