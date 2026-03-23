// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/sched/task.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched/signal.h>

#include "process.h"
#include "scheduler.h"
#include "math.h"

#define ADMISSION_CONSTANT (693 << FIXED_SHIFT)

spinlock_t admission_lock; /* locks admission_sum during register/de-register */
u32 admission_sum;

struct kmem_cache *task_cache;

/* Statically init the list. */
LIST_HEAD(processes);

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

		pr_warn("(teardown) PID %d. Timer deleted, pending callbacks: %d\n", t->pid, pending);
		send_sig(SIGKILL, t->linux_task, 1); /* kill the task to release the file */
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

	pr_debug("PID %d. Fire timer at %u\n", tk->pid, jiffies_to_msecs(tk->last_release));
	spin_lock(&processes_lock);
	tk->state = READY;
	tk->last_release = jiffies; /* tracks actual last release */
	spin_unlock(&processes_lock);

	wake_up_process(dispatch_thread);
}

void register_task(pid_t pid, u32 period, u32 processing_time)
{
	spin_lock_bh(&admission_lock);
	u32 curr_sum = admission_sum + scaled_div(processing_time, period);

	if (curr_sum * 1000 > ADMISSION_CONSTANT) {
		spin_unlock_bh(&admission_lock);
		pr_warn("PID %d is not allowed admission. curr_sum = %u.\n", pid, curr_sum);
		return;
	}
	admission_sum = curr_sum;
	spin_unlock_bh(&admission_lock);

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
	tk->last_release = jiffies;

	/* Only setup. Timer does not fire immediately. */
	timer_setup(&tk->wakeup_timer, wakeup_timer_handler, 0);

	spin_lock_bh(&processes_lock);
	list_add_tail(&tk->list, &processes);
	pr_debug("PID %d added to list, count = %ld. Admission sum = %u.\n", pid, list_count_nodes(&processes), curr_sum);
	spin_unlock_bh(&processes_lock);
}

void deregister_task(pid_t pid)
{
	struct task *t, *tmp;
	struct task *found = NULL;
	u32 sum_after = 0;

	spin_lock_bh(&processes_lock);
	list_for_each_entry_safe(t, tmp, &processes, list) {
		if (t->pid == pid) {
			list_del(&t->list);
			found = t;

			spin_lock_bh(&admission_lock);
			admission_sum -= scaled_div(found->processing_time, found->period);
			sum_after = admission_sum;
			spin_unlock_bh(&admission_lock);

			if (found == ms_current_task)
				ms_current_task = NULL;
			break;
		}
	}
	spin_unlock_bh(&processes_lock);

	if (found) {
		int pending = timer_delete_sync(&found->wakeup_timer);

		pr_debug("PID %d. Timer deleted, pending callbacks: %d. Admission sum = %u.\n", found->pid, pending, sum_after);
		put_task_struct(found->linux_task);
		kmem_cache_free(task_cache, found);

		/* Schedule other tasks. Otherwise, tasks will be idle. */
		wake_up_process(dispatch_thread);
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
		unsigned long next_release = found->last_release + msecs_to_jiffies(found->period);

		/* Arm the timer if next release exceeds current time. */
		if (time_after(next_release, jiffies))
			mod_timer(&found->wakeup_timer, next_release);

		wake_up_process(dispatch_thread);

		pr_debug("Putting task %d to sleep. State = %d\n", found->pid, found->state);
		set_current_state(TASK_KILLABLE); /* allow sleep plus reaping if rmmod early */
		schedule();
	}
}
