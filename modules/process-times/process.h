/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PROCESS_H
#define PROCESS_H

#include <linux/list.h>

void process_list_init(void);

void clear_process_list(void);

static DEFINE_MUTEX(ll_mutex);

struct process_time_info {
	pid_t pid;
	u64 cpu_time;
	bool stopped;
	struct list_head list;
};

extern struct process_time_info *ll;

#endif
