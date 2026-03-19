/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PROCESS_H
#define PROCESS_H

#include <linux/list.h>
#include <linux/pid.h>
#include <linux/sched.h>

int process_init(void);

void process_teardown(void);

void register_task(pid_t pid, u32 period, u32 processing_time);

void deregister_task(pid_t pid);

struct task_struct *find_task_by_pid(int nr);

enum TASK_STATE {
	SLEEPING, /* By default, newly registered tasks sleep. */
	READY,
	RUNNING,
};

struct task {
	/* Use this as param for scheduling API. */
	struct task_struct *linux_task;
	/* Wakes the dispatcher thread. */
	struct timer_list wakeup_timer;

	/* Userspace process pid. */
	pid_t pid;
	/* Timer will periodically trigger the dispatcher thread. */
	u32 period;
	/* Use to calculate admission control. */
	u32 processing_time;
	/* Current state of the registered task. */
	enum TASK_STATE state;

	struct list_head list;
};

/* Guards access to tracked processes. TODO: swap out for spinlock */
static DEFINE_MUTEX(processes_mutex);

/* List of processes we track. */
extern struct list_head processes;

/* Currently running task. */
/* TODO: Can we track min. period READY task? So we can avoid linear search. */
extern struct task *ms_current_task;

#endif
