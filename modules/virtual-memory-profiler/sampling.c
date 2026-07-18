// SPDX-License-Identifier: GPL-2.0
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include "sampling.h"

#define NUM_PAGES    128
#define BUFFER_SIZE  (NUM_PAGES * PAGE_SIZE)

static struct delayed_work sampling;
static struct sample *ring_buffer; // write samples to this buffer

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

int kickstart_sampling(void)
{
	pr_debug("First pcb, initializing delayed monitoring.\n");

	void *raw_ptr = vmalloc(BUFFER_SIZE);

	if (!raw_ptr)
		return -ENOMEM;
	memset(raw_ptr, -1, BUFFER_SIZE);
	ring_buffer = (struct sample *) raw_ptr;

	unsigned long offset;

	for (offset = 0; offset < BUFFER_SIZE; offset += PAGE_SIZE) {
		struct page *page = vmalloc_to_page((void *)((unsigned long)raw_ptr + offset));

		/* set reserved pages */
		if (page)
			SetPageReserved(page);
	}

	INIT_DELAYED_WORK(&sampling, handler);
	schedule_delayed_work(&sampling, msecs_to_jiffies(50));

	return 0;
}

void stop_sampling(const char *ctx)
{
	pr_debug("%s. Stopping sampling...\n", ctx);
	cancel_delayed_work_sync(&sampling);
	if (ring_buffer) {
		unsigned long offset;

		for (offset = 0; offset < BUFFER_SIZE; offset += PAGE_SIZE) {
			struct page *page = vmalloc_to_page((void *)((unsigned long)ring_buffer + offset));

			/* set reserved pages */
			if (page)
				ClearPageReserved(page);
		}
		vfree(ring_buffer);
		ring_buffer = NULL;
	}
}
