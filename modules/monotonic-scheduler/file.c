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
	struct task *t;
	int buf_len = 32;	/* enough for one pid and newline */
	char pid_str[buf_len];
	int total_len = 0;
	int len = 0;

	spin_lock_bh(&processes_lock);
	list_for_each_entry(t, &processes, list) {
		len = scnprintf(pid_str, buf_len, "%d: %u, %u\n", t->pid,
				t->period, t->processing_time);

		if (copy_to_user(ubuf, pid_str, len))
			return -EFAULT;
		ubuf += len;
		total_len += len;
	}
	spin_unlock_bh(&processes_lock);

	// Stop reading.
	if (*ppos >= total_len)
		return 0;

	*ppos += total_len;
	return total_len;	/* return number of bytes read */
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
