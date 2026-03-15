// SPDX-License-Identifier: GPL-2.0
#define DEBUG
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#include "file.h"
#include "process.h"

#define PROC_TIME_DIR "monotonic_sched"
#define PROCFS_FILE "status"

struct proc_dir_entry *dir;

static int __init init_scheduler(void)
{
	pr_debug("Init scheduler\n");
	dir = proc_mkdir(PROC_TIME_DIR, NULL);
	if (!dir) {
		pr_err("Could not create %s\n", PROC_TIME_DIR);
		return -ENOMEM;
	}

	struct proc_dir_entry *our_proc_file;

	our_proc_file = proc_create(PROCFS_FILE, 0644, dir, &my_fops);
	if (!our_proc_file)
		return -ENOMEM;

	int errno = process_init();
	if (errno != 0) {
		pr_err("Error init process. Errno = %d\n", errno);
		return errno;
	}

	pr_debug("Module initialized success.\n");
	return 0;
}

static void __exit exit_scheduler(void)
{
	pr_debug("Stopping scheduler. Deregistering all processes.\n");
	process_teardown();

	remove_proc_entry(PROCFS_FILE, dir);
	remove_proc_entry(PROC_TIME_DIR, NULL);
	pr_debug("Removed folder %s\n", PROC_TIME_DIR);
}

module_init(init_scheduler);
module_exit(exit_scheduler);

MODULE_DESCRIPTION("Rate Monotonic Scheduler");
MODULE_LICENSE("GPL v2");
