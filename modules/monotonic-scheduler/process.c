// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/slab.h>

#include "process.h"

struct kmem_cache *task_cache;

// Statically init the list.
LIST_HEAD(processes);

// pid_t is signed int.
struct task_struct *find_task_by_pid(int nr)
{
	struct task_struct *task;

	rcu_read_lock();
	task = pid_task(find_vpid(nr), PIDTYPE_PID);
	rcu_read_unlock();
	return task;
};

void process_init(void) {
	task_cache = KMEM_CACHE(task, SLAB_HWCACHE_ALIGN);
}

void process_teardown(void) {
	if (task_cache) {
		kmem_cache_destroy(task_cache);
		task_cache = NULL; // Good practice to prevent dangling pointers
	}
}

// TODO:
// slab allocate struct for new task
// A timer callback wakes up the dispatcher thread.
void register_task(pid_t pid, u32 period, u32 processing_time) {
}
