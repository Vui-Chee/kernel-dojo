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
#include "process.h"

const struct proc_ops my_fops = {
	.proc_read = on_proc_read,
	.proc_write = on_proc_write,
};

// Goal is to use `cat` command to read the tracked process times.
ssize_t on_proc_read(struct file *file, char __user *ubuf, size_t count,
		     loff_t *ppos)
{
	struct process_time_info *entry;
	int buf_len = 32; // enough for one pid and newline
	char pid_str[buf_len];
	int total_len = 0;
	int len = 0;
	u64 cpu_time_ms = 0;

	mutex_lock(&ll_mutex);
	list_for_each_entry(entry, &ll->list, list) {
		cpu_time_ms =
			jiffies_to_msecs(nsecs_to_jiffies(entry->cpu_time));
		len = scnprintf(pid_str, buf_len, "%d: %llu\n", entry->pid,
				cpu_time_ms);
		if (copy_to_user(ubuf, pid_str, len))
			return -EFAULT;
		ubuf += len;
		total_len += len;
	}
	mutex_unlock(&ll_mutex);

	// Stop reading.
	if (*ppos >= total_len)
		return 0;

	*ppos += total_len;
	return total_len; // return number of bytes read
}

ssize_t on_proc_write(struct file *file, const char __user *ubuf, size_t count,
		      loff_t *ppos)
{
	// Prevent user from flooding kernel with large writes.
	if (count > PAGE_SIZE)
		return -EINVAL;

	// Copy data from user space to kernel space safely (including null-termination).
	char *kbuf __free(kfree) = memdup_user_nul(ubuf, count);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	pr_debug("Received from user: %s\n", kbuf);

	int new_pid;
	int ret;

	ret = kstrtoint(kbuf, 10, &new_pid);
	if (ret < 0) {
		pr_err("Invalid input: %s (error %d)\n", kbuf, ret);
		return ret;
	}

	struct process_time_info *new_proc_entry;

	// Add new PID to linked list (assuming no dups)
	new_proc_entry = kmalloc(sizeof(*new_proc_entry), GFP_KERNEL);
	if (!new_proc_entry)
		return -ENOMEM;

	new_proc_entry->pid = new_pid;

	mutex_lock(&ll_mutex);
	list_add_tail(&new_proc_entry->list, &ll->list);
	mutex_unlock(&ll_mutex);

	pr_debug("Added new PID %d to tracking list, current size = %ld\n",
		 new_pid, list_count_nodes(&ll->list));

	return count;
}
