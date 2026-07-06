/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PROCESS_H
#define PROCESS_H

#include <linux/list.h>
#include <linux/types.h>

extern struct list_head pcbs;

struct _pcb {
	unsigned long cpu_util;
	unsigned long min_flt;
	unsigned long max_flt;

	struct list_head list;
};

#endif
