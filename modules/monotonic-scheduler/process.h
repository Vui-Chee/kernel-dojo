/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PROCESS_H
#define PROCESS_H

#include <linux/list.h>
#include <linux/pid.h>
#include <linux/sched.h>

void register_task(pid_t pid, u32 period, u32 processing_time);

struct task_struct *find_task_by_pid(int nr);

enum TASK_STATE {
	RUNNING,
	READY,
	SLEEPING // register task first with this state
};

struct task {
	struct task_struct *linux_task;
	struct timer_list wakeup_timer;

	pid_t pid;
	u32 period;
	u32 processing_time;
	enum TASK_STATE state;

	struct list_head list;
};

// Guards access to the linked list
static DEFINE_MUTEX(processes_mutex);
// List of processes we track.
extern struct list_head processes;

#endif
