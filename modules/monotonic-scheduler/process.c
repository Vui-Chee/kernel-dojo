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
	struct task_struct *task; // kernel task

	rcu_read_lock();
	task = pid_task(find_vpid(nr), PIDTYPE_PID);
	rcu_read_unlock();
	return task;
};

static void task_ctor(void *obj)
{
    struct task *t = obj;
    INIT_LIST_HEAD(&t->list);
}

int process_init(void) {
	task_cache = kmem_cache_create(
	        "processes_task_cache",   // cache name
	        sizeof(struct task),      // object size
	        0,                        // alignment (0 = default)
	        SLAB_HWCACHE_ALIGN,       // flags
	        task_ctor                 // constructor
	);
	if (!task_cache)
		return -ENOMEM;
	return 0;
}

void process_teardown(void) {
	struct task *t, *tmp;
	
	list_for_each_entry_safe(t, tmp, &processes, list) {
		list_del_init(&t->list);
		kmem_cache_free(task_cache, t);
	}

	if (task_cache) {
		kmem_cache_destroy(task_cache);
		task_cache = NULL; // prevent dangling pointers
	}
}

void register_task(pid_t pid, u32 period, u32 processing_time) {
	struct task *tk;

	tk = kmem_cache_alloc(task_cache, GFP_KERNEL);
	if (!tk)
		pr_err("Failed to cache alloc process with pid %d.\n", pid);
		return;

	struct task_struct *k_tk = find_task_by_pid(pid);
	if (!k_tk) {
		pr_err("Cannot find task with pid %d.\n", pid);
		return;
	}

	// TODO: setup timer.
	tk->linux_task = k_tk;
	tk->pid = pid;
	tk->period = period;
	tk->processing_time = processing_time;

	mutex_lock(&processes_mutex);
	list_add_tail(&tk->list, &processes);
	pr_debug("Add item to list, count = %ld\n", list_count_nodes(&processes));
	mutex_unlock(&processes_mutex);
}

void deregister_task(pid_t pid) {
	struct task *t, *tmp;

	mutex_lock(&processes_mutex);
	list_for_each_entry_safe(t, tmp, &processes, list) {
		if (t->pid == pid) {
			// Rewire pointers.
			list_del(&t->list);
			kmem_cache_free(task_cache, t);
			pr_debug("Removed item to list, count = %ld\n", list_count_nodes(&processes));
			goto done;
		}
	}

done:
	mutex_unlock(&processes_mutex);
}
