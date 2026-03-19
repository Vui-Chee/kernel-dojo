/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <uapi/linux/sched/types.h>

extern wait_queue_head_t dispatch_wq;

void wakeup_task(struct task_struct *task);

void preempt_task(struct task_struct *task);

#endif
