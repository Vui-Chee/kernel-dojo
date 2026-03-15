// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/slab.h>

#include "process.h"

// Statically init the list.
LIST_HEAD(processes);

// pid_t is signed int.
struct task_struct* find_task_by_pid(int nr)
{
    struct task_struct* task;
    rcu_read_lock();
    task=pid_task(find_vpid(nr), PIDTYPE_PID);
    rcu_read_unlock();

    return task;
};
