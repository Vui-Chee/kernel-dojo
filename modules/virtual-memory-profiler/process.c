// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>

#include "cpu.h"
#include "process.h"

DEFINE_SPINLOCK(pcbs_lock);
LIST_HEAD(pcbs);

static struct delayed_work monitoring;
static unsigned long next_tick; // ok to be unset

static void monitoring_handler(struct work_struct *work)
{
	// TODO: harvest metrics using get_cpu_use()
	// TODO: vmalloc ring buffer
	// sample every 1/20th of a second

#ifdef DEBUG
	if (time_after_eq(jiffies, next_tick)) {
		pr_debug("Tick!!!\n");
		next_tick = jiffies + secs_to_jiffies(1);
	}
#endif

	schedule_delayed_work(&monitoring, msecs_to_jiffies(50));
}

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

	new_pcb->pid = pid;

	spin_lock(&pcbs_lock);
	// first pcb, init work queue
	if (list_empty(&pcbs)) {
		pr_debug("First pcb, initializing delayed monitoring.\n");
		INIT_DELAYED_WORK(&monitoring, monitoring_handler);
		schedule_delayed_work(&monitoring, msecs_to_jiffies(50));
	}
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

	// last item removed, delete work queue
	if (list_empty(&pcbs)) {
		pr_debug("Last pcb removed, stopping monitoring...\n");
		cancel_delayed_work_sync(&monitoring);
	}

	spin_unlock(&pcbs_lock);
	return 0;
}

/* Assumes pcbs are allocated with kmalloc. */
void free_pcbs(void)
{
	struct _pcb *entry, *next;

	spin_lock(&pcbs_lock);
	list_for_each_entry_safe(entry, next, &pcbs, list) {
		list_del(&entry->list);
		kfree(entry);
	}
	if (list_empty(&pcbs)) {
		pr_debug("All pcbs freed, stopping monitoring...\n");
		cancel_delayed_work_sync(&monitoring); // idempotent
	}
	spin_unlock(&pcbs_lock);
}
