// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>

#include "file.h"
#include "process.h"
#include "interval.h"

#define PROC_TIME_DIR "proc_time"
#define PROCFS_FILE "status"

struct proc_dir_entry *dir;

// Runs when you use 'insmod'
static int __init proctime_tracker_init(void)
{
	dir = proc_mkdir(PROC_TIME_DIR, NULL);
	if (!dir) {
		pr_err("Could not create %s\n", PROC_TIME_DIR);
		return -ENOMEM;
	}

	struct proc_dir_entry *our_proc_file;

	our_proc_file = proc_create(PROCFS_FILE, 0644, dir, &my_fops);
	if (!our_proc_file)
		return -ENOMEM;

	process_list_init();

	proc_interval_init(1000);

	pr_debug("module initialized success\n");
	return 0;
}

// Runs when you use 'rmmod'
static void __exit proctime_tracker_exit(void)
{
	// remove timer + queue
	clear_proc_interval();

	// remove process list
	clear_process_list();

	// remove file first
	remove_proc_entry(PROCFS_FILE, dir);
	remove_proc_entry(PROC_TIME_DIR, NULL);
	pr_debug("Removed folder %s\n", PROC_TIME_DIR);
}

module_init(proctime_tracker_init);
module_exit(proctime_tracker_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Process Time Tracker module");
