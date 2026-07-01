// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/workqueue.h>
#include <linux/timer.h>

#include "interval.h"
#include "proctime.h"
#include "process.h"

struct work_struct work;

struct workqueue_struct *wq;

struct timer_list proc_timer;

// How often to update process times (in milliseconds)
unsigned int interval_ms = 5000;

// This function runs in Process Context (Safe to sleep!)
void work_function(struct work_struct *work)
{
	int res;
	struct process_time_info *entry;

	mutex_lock(&ll_mutex);
	list_for_each_entry(entry, &ll->list, list) {
		if (entry->stopped)
			continue;
		res = get_cpu_use(entry->pid, &entry->cpu_time);
		if (res < 0)
			entry->stopped = true;
	}
	mutex_unlock(&ll_mutex);
}

// Top-half, prepares work for bottom-half.
void proc_timer_callback(struct timer_list *t)
{
	// Prepare work
	queue_work(wq, &work);

	// This tells the kernel: "Put me back on the schedule for 5 seconds from now."
	mod_timer(t, jiffies + msecs_to_jiffies(interval_ms));
}

void proc_interval_init(unsigned int interval)
{
	interval_ms = interval;

	// prepare workqueue first
	INIT_WORK(&work, work_function);
	wq = alloc_workqueue("proc_time_workqueue", WQ_UNBOUND, 1);
	if (!wq) {
		pr_err("Could not create workqueue\n");
		return;
	}

	timer_setup(&proc_timer, proc_timer_callback, 0);
	// Schedule for 5 seconds from now
	mod_timer(&proc_timer, jiffies + msecs_to_jiffies(interval_ms));
}

void clear_proc_interval(void)
{
	int pending;

	pending = timer_delete_sync(&proc_timer);
	pr_debug("Timer deleted, pending callbacks: %d\n", pending);
	if (wq)
		destroy_workqueue(wq);
}
