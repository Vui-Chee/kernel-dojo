// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/sched/task.h>
#include <linux/slab.h>
#include <linux/timer.h>

#include "process.h"
#include "scheduler.h"

struct kmem_cache *task_cache;

/* Statically init the list. */
LIST_HEAD(processes);

struct task *ms_current_task;

/* pid_t is signed int. */
struct task_struct *find_task_by_pid(int nr)
{
	struct task_struct *task; /* kernel task */

	rcu_read_lock();
	task = pid_task(find_vpid(nr), PIDTYPE_PID);
	rcu_read_unlock();
	return task;
};

static void task_ctor(void *obj)
{
	struct task *t = obj;

	INIT_LIST_HEAD(&t->list);
}

int process_init(void)
{
	task_cache = kmem_cache_create(
		"processes_task_cache",   /* cache name */
		sizeof(struct task),      /* object size */
		0,                        /* alignment (0 = default) */
		SLAB_HWCACHE_ALIGN,       /* flags */
		task_ctor                 /* constructor */
	);
	if (!task_cache)
		return -ENOMEM;
	return 0;
}

void process_teardown(void)
{
	struct task *t, *tmp;

	list_for_each_entry_safe(t, tmp, &processes, list) {
		int pending = timer_delete_sync(&t->wakeup_timer);

		pr_debug("(teardown) PID %d. Timer deleted, pending callbacks: %d\n", t->pid, pending);
		list_del_init(&t->list);
		kmem_cache_free(task_cache, t);
	}
	kmem_cache_destroy(task_cache);
	task_cache = NULL; /* prevent dangling pointers */
}

static void wakeup_timer_handler(struct timer_list *t)
{
	struct task *tk;

	tk = container_of(t, struct task, wakeup_timer);

	pr_debug("PID %d. Fire timer\n", tk->pid);
	spin_lock(&processes_lock);
	tk->state = READY;
	wake_up_interruptible(&dispatch_wq);
	spin_unlock(&processes_lock);
}

void register_task(pid_t pid, u32 period, u32 processing_time)
{
	struct task *tk;

	tk = kmem_cache_alloc(task_cache, GFP_KERNEL);
	if (!tk) {
		pr_err("Failed to cache alloc process with pid %d.\n", pid);
		return;
	}

	struct task_struct *k_tk = find_task_by_pid(pid);

	if (!k_tk) {
		pr_err("Cannot find task with pid %d.\n", pid);
		return;
	}

	tk->linux_task = get_task_struct(k_tk); /* ref count to prevent kernel from freeing prematurely */
	tk->pid = pid;
	tk->period = period;
	tk->processing_time = processing_time;
	tk->state = SLEEPING;

	/* Fire timer period(ms) from now. */
	timer_setup(&tk->wakeup_timer, wakeup_timer_handler, 0);
	mod_timer(&tk->wakeup_timer, jiffies + msecs_to_jiffies(period));

	spin_lock_bh(&processes_lock);
	list_add_tail(&tk->list, &processes);
	pr_debug("Add item to list, count = %ld\n", list_count_nodes(&processes));
	spin_unlock_bh(&processes_lock);
}

/* TODO: Can we use a map<pid, task ptr> to reduce deletion to O(1). */
void deregister_task(pid_t pid)
{
	struct task *t, *tmp;
	struct task *found = NULL;

	spin_lock_bh(&processes_lock);
	list_for_each_entry_safe(t, tmp, &processes, list) {
		if (t->pid == pid) {
			list_del(&t->list);
			found = t;
			break;
		}
	}
	spin_unlock_bh(&processes_lock);

	if (found) {
		int pending = timer_delete_sync(&found->wakeup_timer);

		pr_debug("PID %d. Timer deleted, pending callbacks: %d\n", found->pid, pending);
		put_task_struct(found->linux_task);
		kmem_cache_free(task_cache, found);
	}
}

void yield_task(pid_t pid)
{
	struct task *t, *found = NULL;

	spin_lock_bh(&processes_lock);
	list_for_each_entry(t, &processes, list) {
		if (t->pid == pid) {
			found = t;
			found->state = SLEEPING;
			break;
		}
	}
	spin_unlock_bh(&processes_lock);

	if (found) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule();
	}
}
