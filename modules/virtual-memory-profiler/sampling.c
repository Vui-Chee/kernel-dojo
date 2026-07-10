// SPDX-License-Identifier: GPL-2.0
#include <linux/workqueue.h>

#include "sampling.h"

// #define BUFFER_SIZE (128 * 4096)

static struct delayed_work sampling;
// static struct sample *buffer = NULL; // write telemetry data to this buffer

static unsigned long next_tick; // ok to be unset

static void handler(struct work_struct *work)
{
	// TODO: harvest metrics using get_cpu_use()
	// TODO: vmalloc ring buffer
	// sample every 1/20th of a second

#ifdef DEBUG
	// sampling check
	if (time_after_eq(jiffies, next_tick)) {
		pr_debug("Tick!!!\n");
		next_tick = jiffies + secs_to_jiffies(1);
	}
#endif

	schedule_delayed_work(&sampling, msecs_to_jiffies(50));
}

void kickstart_sampling(void)
{
	pr_debug("First pcb, initializing delayed monitoring.\n");
	INIT_DELAYED_WORK(&sampling, handler);
	schedule_delayed_work(&sampling, msecs_to_jiffies(50));
}

void stop_sampling(const char *ctx)
{
	pr_debug("%s. Stopping sampling...\n", ctx);
	cancel_delayed_work_sync(&sampling);
}
