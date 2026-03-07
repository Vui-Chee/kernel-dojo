/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/cputime.h>

static int get_cpu_use(pid_t nr, u64 *cpu_use)
{
	struct task_struct *task;
	struct pid *pid_struct;
	u64 utime, stime;
	int ret = -1;

	rcu_read_lock();

	// 1. Find the 'struct pid' (This function IS exported)
	pid_struct = find_vpid(nr);

	if (pid_struct) {
		// 2. Get the 'task_struct' (This is an inline function in pid.h)
		task = pid_task(pid_struct, PIDTYPE_PID);

		if (task) {
			// 3. Get the adjusted time (Standard for 6.x kernels)
			task_cputime_adjusted(task, &utime, &stime);
			*cpu_use = utime + stime;
			ret = 0;
		}
	}

	rcu_read_unlock();
	return ret;
}
