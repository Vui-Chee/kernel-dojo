/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <uapi/linux/sched/types.h>

#define PROC_TIME_DIR "monotonic_sched"
#define PROCFS_FILE "status"

extern struct task_struct *dispatch_thread;

void wakeup_task(struct task_struct *task);

void preempt_task(struct task_struct *task);

#endif
