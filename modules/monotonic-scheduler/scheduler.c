// SPDX-License-Identifier: GPL-2.0
#define DEBUG
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include "file.h"
#include "process.h"
#include "scheduler.h"

struct proc_dir_entry *dir;

struct task_struct *dispatch_thread;

/* Currently running task. */
/* TODO: Can we track min. period READY task? So we can avoid linear search. */
struct task *ms_current_task;

void wakeup_task(struct task_struct *task)
{
	struct sched_attr attr;

	wake_up_process(task);
	attr.sched_policy = SCHED_FIFO;
	attr.sched_priority = 99;
	sched_setattr_nocheck(task, &attr);
}

void preempt_task(struct task_struct *task)
{
	struct sched_attr attr;

	attr.sched_policy = SCHED_NORMAL;
	attr.sched_priority = 0;
	sched_setattr_nocheck(task, &attr);
}

/**
 * Returns new task in READY state with highest priority.
 *
 * Highest priority for now means task with the lowest period.
 */
static void sched_best_task(void)
{
	struct task *t;

	spin_lock_bh(&processes_lock);
	if (list_empty(&processes)) {
		spin_unlock_bh(&processes_lock);
		return;
	}

	/* Need to check if it's in READY state */
	struct task *best_tk;

	list_for_each_entry(t, &processes, list) {
		if (best_tk == NULL && t->state == READY) {
			best_tk = t;
			continue;
		} else if (t->state == READY && t->period < best_tk->period)
			best_tk = t;
	}
	spin_unlock_bh(&processes_lock);

	/* Schedule new task if:
	 *	1) new task has higher priority than current running task.
	 *	2) no current task, so pick highest READY task.
	 *	3) current task is sleeping, pick highest READY task.
	 */
	if (best_tk) {
		if (ms_current_task == NULL) {
			pr_debug("PID %d: first running task\n", best_tk->pid);
			wakeup_task(best_tk->linux_task);
			ms_current_task = best_tk;
			ms_current_task->state = RUNNING;
		} else if (ms_current_task != NULL && ms_current_task->state == RUNNING && ms_current_task->period > best_tk->period) {
			pr_debug("PID %d: RUNNING current task is replaced by another task.\n", best_tk->pid);
			preempt_task(ms_current_task->linux_task);
			ms_current_task->state = READY;

			wakeup_task(best_tk->linux_task);
			ms_current_task = best_tk;
			ms_current_task->state = RUNNING;
		} else if (ms_current_task != NULL && ms_current_task->state == SLEEPING) {
			pr_debug("PID %d: current task is SLEEPING.\n", best_tk->pid);
			preempt_task(ms_current_task->linux_task);
			/* NOTE: We do not modify the state of prev task. */

			wakeup_task(best_tk->linux_task);
			ms_current_task = best_tk;
			ms_current_task->state = RUNNING;
		} else if (ms_current_task != NULL && best_tk->state == READY) {
			pr_debug("PID %d: current task is READY.\n", best_tk->pid);
			wakeup_task(best_tk->linux_task);
			ms_current_task = best_tk;
			ms_current_task->state = RUNNING;
		} else {
			pr_warn("PID %d: unknown state trasition with READY task.\n", best_tk->pid);
			if (ms_current_task)
				pr_warn("Curr task %d, state = %d\n", ms_current_task->pid, ms_current_task->state);
		}
		// TODO: RUNNING -> SLEEPING (current task is finished)
	} else {
		/* We will still preempt the task, even though there is no new READY task. */
		if (ms_current_task != NULL && ms_current_task->state == SLEEPING) {
			pr_debug("PID %d: current task is SLEEPING and no READY task.\n", ms_current_task->pid);
			preempt_task(ms_current_task->linux_task);
			ms_current_task = NULL;
		} else {
			pr_warn("Unknown state trasition with no READY task.\n");
		}
	}
}

static int dispatch_fn(void *data)
{
	pr_debug("Dispatch thread started\n");

	while (!kthread_should_stop()) {

		/* sleep until condition becomes true */
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (kthread_should_stop())
			break;

		pr_debug("dispatcher woke up, find next best task to run\n");
		sched_best_task();
	}

	pr_warn("dispatch thread exiting\n");
	return 0;
}

static int __init init_scheduler(void)
{
	pr_debug("Init scheduler\n");
	dir = proc_mkdir(PROC_TIME_DIR, NULL);
	if (!dir) {
		pr_err("Could not create %s\n", PROC_TIME_DIR);
		return -ENOMEM;
	}

	struct proc_dir_entry *our_proc_file;

	our_proc_file = proc_create(PROCFS_FILE, 0644, dir, &my_fops);
	if (!our_proc_file)
		return -ENOMEM;

	int errno = process_init();

	if (errno != 0) {
		pr_err("Error init process. Errno = %d\n", errno);
		return errno;
	}

	dispatch_thread = kthread_run(dispatch_fn, NULL, "dispatcher");
	if (IS_ERR(dispatch_thread))
		return PTR_ERR(dispatch_thread);

	pr_debug("Module initialized success.\n");
	return 0;
}

static void __exit exit_scheduler(void)
{
	pr_debug("Stopping scheduler. Deregistering all processes.\n");

	if (dispatch_thread)
		kthread_stop(dispatch_thread);

	process_teardown();
	pr_warn("Finished process teardown\n");

	remove_proc_entry(PROCFS_FILE, dir);
	pr_warn("Removed file %s\n", PROCFS_FILE);
	remove_proc_entry(PROC_TIME_DIR, NULL);
	pr_warn("Removed folder %s\n", PROC_TIME_DIR);
}

module_init(init_scheduler);
module_exit(exit_scheduler);

MODULE_DESCRIPTION("Rate Monotonic Scheduler");
MODULE_LICENSE("GPL v2");
