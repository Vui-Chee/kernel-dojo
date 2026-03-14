// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cleanup.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/jiffies.h>

#include "file.h"
// #include "process.h"

const struct proc_ops my_fops = {
	.proc_read = on_proc_read,
	.proc_write = on_proc_write,
};

ssize_t on_proc_read(struct file *file, char __user *ubuf, size_t count,
		     loff_t *ppos)
{
	// TODO: print process information
	return 0;
}

ssize_t on_proc_write(struct file *file, const char __user *ubuf,
		      size_t count, loff_t *ppos)
{
	// Prevent user from flooding kernel with large writes.
	if (count > PAGE_SIZE)
		return -EINVAL;

	// Copy data from user space to kernel space safely (including null-termination).
	char *kbuf __free(kfree) = memdup_user_nul(ubuf, count);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	pr_debug("Received from user: %s\n", kbuf);

	// int ret;

	// TODO: parse kbuf for different commands from user.
	//
	// REGISTRATION: R,PID,PERIOD,COMPUTATION
	// DE-REGISTRATION: D,PID
	// 
	// TODO: Do later
	// YIELD: Y,PID
	return 0;
}
