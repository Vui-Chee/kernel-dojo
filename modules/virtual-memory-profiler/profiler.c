// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#include "file.h"
#include "process.h"

struct proc_dir_entry *dir;

static int __init vm_profiler_init(void)
{
	int errno;

	dir = proc_mkdir(PROC_TIME_DIR, NULL);
	if (!dir) {
		errno = -ENOMEM;
		goto failed_dir;
	}

	if (!proc_create(PROCFS_FILE, 0644, dir, &fileops)) {
		errno = -ENOMEM;
		goto remove_dir;
	}

	pr_info("Virtual Memory Profiler successfully init.\n");

	return 0;

remove_dir:
	remove_proc_entry(PROC_TIME_DIR, NULL);
	pr_debug("Removed folder %s\n", PROC_TIME_DIR);
failed_dir:
	pr_debug("Failed to create directory\n");
	return errno;
}

static void __exit vm_profiler_exit(void)
{
	pr_info("Stopping Virtual Memory Profiler...\n");
	remove_proc_entry(PROCFS_FILE, dir);
	pr_debug("Removed file %s\n", PROCFS_FILE);
	remove_proc_entry(PROC_TIME_DIR, NULL);
	pr_debug("Removed folder %s\n", PROC_TIME_DIR);
	free_pcbs();
	pr_debug("Freed PCBs\n");
}

module_init(vm_profiler_init);
module_exit(vm_profiler_exit);

MODULE_DESCRIPTION("Virtual Memory Profiler");
MODULE_LICENSE("GPL v2");
