/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __CPU_H__
#define __CPU_H__

#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

static int get_cpu_use(int pid, unsigned long *min_flt, unsigned long *maj_flt,
		       unsigned long *cpu_utilization)
{
	struct task_struct *task;
	struct pid *pid_struct;

	pid_struct = find_get_pid(pid);
	if (!pid_struct)
		return -1;

	task = get_pid_task(pid_struct, PIDTYPE_PID);
	put_pid(pid_struct); // drop ref to prevent memory leaks

	if (!task)
		return -1;

	// modern Kernels (6.x): read the absolute accumulated values
	if (min_flt)
		*min_flt = task->min_flt;

	if (maj_flt)
		*maj_flt = task->maj_flt;

	// combine both user and kernel time gives full cpu time of process.
	if (cpu_utilization)
		*cpu_utilization = (unsigned long)(nsecs_to_jiffies(task->utime) +
					   	   nsecs_to_jiffies(task->stime));

	return 0;
}

#endif
