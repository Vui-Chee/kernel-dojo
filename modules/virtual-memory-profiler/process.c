// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>

#include "cpu.h"
#include "process.h"
#include "sampling.h"

DEFINE_SPINLOCK(pcbs_lock);
LIST_HEAD(pcbs);

int reg_proc(pid_t pid)
{
	struct _pcb *new_pcb;
	// check if pid is valid process
	int not_found = get_cpu_use(pid, NULL, NULL, NULL);

	if (not_found)
		return not_found;

	new_pcb = kmalloc(sizeof(*new_pcb), GFP_KERNEL);
	if (!new_pcb)
		return -ENOMEM;

	new_pcb->pid = pid;

	spin_lock(&pcbs_lock);
	if (list_empty(&pcbs))
		kickstart_sampling();
	list_add_tail(&new_pcb->list, &pcbs);

#ifdef DEBUG
	size_t size = list_count_nodes(&pcbs);

	pr_debug("pcbs size (register): %zu\n", size);
#endif

	spin_unlock(&pcbs_lock);
	return 0;
}

int unreg_proc(pid_t pid)
{
	struct _pcb *t, *tmp;
	struct _pcb *found = NULL;

	spin_lock(&pcbs_lock);
	list_for_each_entry_safe(t, tmp, &pcbs, list) {
		if (t->pid == pid) {
			found = t;
			break;
		}
	}

	if (!found) {
		spin_unlock(&pcbs_lock);
		return -1;
	}

	list_del(&found->list);
	kfree(found);
#ifdef DEBUG
	size_t size = list_count_nodes(&pcbs);

	pr_debug("pcbs size (unregister): %zu\n", size);
#endif

	if (list_empty(&pcbs))
		stop_sampling("Last item removed");

	spin_unlock(&pcbs_lock);
	return 0;
}

void free_pcbs(void)
{
	struct _pcb *entry, *next;

	spin_lock(&pcbs_lock);
	list_for_each_entry_safe(entry, next, &pcbs, list) {
		list_del(&entry->list);
		kfree(entry);
	}
	if (list_empty(&pcbs))
		stop_sampling("Freed pcbs");
	spin_unlock(&pcbs_lock);
}
