/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <uapi/linux/sched/types.h>

static void wakeup_task(struct task_struct *task)
{
	struct sched_attr attr;

	wake_up_process(task);
	attr.sched_policy = SCHED_FIFO;
	attr.sched_priority = 99;
	sched_setattr_nocheck(task, &attr);
}

static void preempt_task(struct task_struct *task)
{
	struct sched_attr attr;

	attr.sched_policy = SCHED_NORMAL;
	attr.sched_priority = 0;
	sched_setattr_nocheck(task, &attr);
}

#endif
