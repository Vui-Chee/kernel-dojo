// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>

#include "cpu.h"
#include "process.h"

LIST_HEAD(pcbs);

/* Assumes pcbs are allocated with kmalloc. */
void free_pcbs(void)
{
	struct _pcb *entry, *next;

	list_for_each_entry_safe(entry, next, &pcbs, list) {
		list_del(&entry->list);
		kfree(entry);
	}
}
