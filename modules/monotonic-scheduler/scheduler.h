/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <uapi/linux/sched/types.h>

#define PROC_TIME_DIR "monotonic_sched"
#define PROCFS_FILE "status"

extern struct task *ms_current_task;

extern struct task_struct *dispatch_thread;

#endif
