// SPDX-License-Identifier: GPL-2.0
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include "buffer.h"
#include "sampling.h"

#define SAMPLE_SIZE 16000

static struct delayed_work sampling;
static struct ring_buffer *rb;

static unsigned long next_tick; // ok to be unset

static void handler(struct work_struct *work)
{
	// TODO: harvest metrics using get_cpu_use()
	// TODO: vmalloc ring buffer
	// sample every 1/20th of a second

#ifdef DEBUG
	if (time_after_eq(jiffies, next_tick)) {
		pr_debug("Tick!!!\n");
		next_tick = jiffies + secs_to_jiffies(1);
	}
#endif

	schedule_delayed_work(&sampling, msecs_to_jiffies(50));
}

int kickstart_sampling(void)
{
	pr_debug("First pcb, initializing delayed monitoring.\n");

	rb = init_shared_buffer(SAMPLE_SIZE, sizeof(struct sample));

	INIT_DELAYED_WORK(&sampling, handler);
	schedule_delayed_work(&sampling, msecs_to_jiffies(50));
	return 0;
}

void stop_sampling(const char *ctx)
{
	pr_debug("%s. Stopping sampling...\n", ctx);
	cancel_delayed_work_sync(&sampling);
	if (rb)
		free_shared_buffer(rb);
}
