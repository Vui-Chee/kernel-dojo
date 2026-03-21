// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cleanup.h>
#include <linux/err.h>
#include <linux/mm.h>

#include "file.h"
#include "process.h"

const struct proc_ops my_fops = {
	.proc_read = on_proc_read,
	.proc_write = on_proc_write,
};

ssize_t on_proc_read(struct file *file, char __user *ubuf, size_t count,
		     loff_t *ppos)
{
	/* TODO: print process information */
	return 0;
}

ssize_t on_proc_write(struct file *file, const char __user *ubuf,
		      size_t count, loff_t *ppos)
{
	if (*ppos != 0)
		return -EINVAL;

	if (count < 1 || count > PAGE_SIZE)
		return -EINVAL;

	/* Copy data from user space to kernel space safely (including null-termination). */
	char *kbuf __free(kfree) = memdup_user_nul(ubuf, count);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	pid_t pid;
	char op = kbuf[0];

	switch (op) {
	case 'R': {
		u32 period;
		u32 processing_time;
		int ret = sscanf(kbuf, "%c,%d,%u,%u", &op, &pid, &period, &processing_time);

		if (ret != 4) {
			pr_err("Register operation accepts 4 values. Got %d values.\n", ret);
			break;
		}
		pr_debug("Registering process: %c,%d,%u,%u\n", op, pid, period, processing_time);
		register_task(pid, period, processing_time);
		break;
	}
	case 'D': {
		int ret = sscanf(kbuf, "%c,%d", &op, &pid);

		if (ret != 2) {
			pr_err("De-register operation accepts 2 values. Got %d values.\n", ret);
			break;
		}
		pr_debug("De-registering process: %c,%d\n", op, pid);
		deregister_task(pid);
		break;
	}
	case 'Y': {
		int ret = sscanf(kbuf, "%c,%d", &op, &pid);

		if (ret != 2) {
			pr_err("Yield operation accepts 2 values. Got %d values.\n", ret);
			break;
		}
		pr_debug("Yield: %c,%d\n", op, pid);
		yield_task(pid);
		break;
	}
	default:
		pr_err("Unknown operation: %c\n", op);
	}

	return count;
}
