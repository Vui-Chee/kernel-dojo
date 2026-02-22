/* SPDX-License-Identifier: GPL-2.0 */
#ifndef INTERVAL_H
#define INTERVAL_H

#include <linux/workqueue.h>

void work_function(struct work_struct *work);

void proc_timer_callback(struct timer_list *t);

void proc_interval_init(unsigned int interval);

void clear_proc_interval(void);

extern struct work_struct work;

extern struct workqueue_struct *wq;

extern struct timer_list proc_timer;

#endif
