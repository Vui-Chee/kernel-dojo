/* SPDX-License-Identifier: GPL-2.0 */
#ifndef FILE_H
#define FILE_H

#include <linux/proc_fs.h>

#define PROC_TIME_DIR "vm_profiler"
#define PROCFS_FILE "status"

ssize_t on_proc_write(struct file *file, const char __user *ubuf, size_t count,
		      loff_t *ppos);

extern const struct proc_ops fileops;

#endif
