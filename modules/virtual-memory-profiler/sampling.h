/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SAMPLING_H
#define SAMPLING_H

int kickstart_sampling(void);
void stop_sampling(const char *ctx);

struct sample {
	unsigned long jiffies;
	unsigned long min_flts;
	unsigned long maj_flts;
	unsigned long cpu_util;
};

#endif
