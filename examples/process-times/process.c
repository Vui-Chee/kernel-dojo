// SPDX-License-Identifier: GPL-2.0
#define DEBUG

#include <linux/slab.h>

#include "process.h"

struct process_time_info *ll;

void process_list_init(void)
{
	ll = kmalloc(sizeof(*ll), GFP_KERNEL);
	if (ll) {
		INIT_LIST_HEAD(&ll->list);
		pr_debug("linked list initialized success. Count = %ld\n",
			 list_count_nodes(&ll->list));
	}
}

void clear_process_list(void)
{
	struct process_time_info *entry, *tmp;
	int count = 0;

	mutex_lock(&ll_mutex);
	list_for_each_entry_safe(entry, tmp, &ll->list, list) {
		list_del(&entry->list);
		kfree(entry);
		count++;
	}
	ll = NULL;
	mutex_unlock(&ll_mutex);

	pr_debug("cleared linked list. Removed %d nodes\n", count);
}
