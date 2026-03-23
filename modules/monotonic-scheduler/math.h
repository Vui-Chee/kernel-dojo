/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MATH_H
#define MATH_H

#include <linux/reciprocal_div.h>

#define FIXED_SHIFT 12

static u32 scaled_div(u32 a, u32 b)
{
	return reciprocal_divide((u64)a << FIXED_SHIFT, reciprocal_value(b));
}

#endif
