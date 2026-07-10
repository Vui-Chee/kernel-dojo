/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PROCESS_H
#define PROCESS_H

#include <linux/list.h>
#include <linux/types.h>

extern spinlock_t pcbs_lock;
extern struct list_head pcbs;

 // track process only
struct _pcb {
	pid_t pid;

	struct list_head list;
};

int reg_proc(pid_t pid);
int unreg_proc(pid_t pid);
void free_pcbs(void);

#endif
