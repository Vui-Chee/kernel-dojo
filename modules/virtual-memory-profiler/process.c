// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>

#include "cpu.h"
#include "process.h"

LIST_HEAD(pcbs);

int reg_proc(pid_t pid)
{
	// No point storing any metrics on first registration since no work is done.
	int not_found = get_cpu_use(pid, NULL, NULL, NULL);

	if (not_found)
		return not_found;

	pr_debug("\n");

	struct _pcb *new_pcb;

	new_pcb = kmalloc(sizeof(*new_pcb), GFP_KERNEL);
	if (!new_pcb)
		return -ENOMEM;

	// TODO: add lock

	new_pcb->pid = pid;
	list_add_tail(&new_pcb->list, &pcbs);

#ifdef DEBUG
	size_t size = list_count_nodes(&pcbs);

	pr_debug("pcbs size (register): %zu\n", size);
#endif

	return 0;
}

int unreg_proc(pid_t pid)
{
	struct _pcb *t, *tmp;
	struct _pcb *found = NULL;

	list_for_each_entry_safe(t, tmp, &pcbs, list) {
#ifdef DEBUG
		pr_debug("un-register: iterating pid %d\n", t->pid);
#endif
		if (t->pid == pid) {
			found = t;
			break;
		}
	}

	if (found) {
		list_del(&found->list);
		kfree(found);
		goto count_check;
	} else
		return -1;

count_check: {
#ifdef DEBUG
	size_t size = list_count_nodes(&pcbs);

	pr_debug("pcbs size (unregister): %zu\n", size);
#endif
}
	return 0;
}

/* Assumes pcbs are allocated with kmalloc. */
void free_pcbs(void)
{
	struct _pcb *entry, *next;

	list_for_each_entry_safe(entry, next, &pcbs, list) {
		list_del(&entry->list);
		kfree(entry);
	}
}
