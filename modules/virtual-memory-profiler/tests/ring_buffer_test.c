// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include "buffer.h"

static void ring_buffer_init_test(struct kunit *test)
{
	struct ring_buffer *rb;

	rb = init_shared_buffer(16, sizeof(int));

	KUNIT_ASSERT_NOT_NULL(test, rb);

	KUNIT_EXPECT_EQ(test, rb->capacity, 16U);
	KUNIT_EXPECT_EQ(test, rb->head, 0U);
	KUNIT_EXPECT_EQ(test, rb->tail, 0U);

	free_shared_buffer(rb);
}

static struct kunit_case ring_buffer_test_cases[] = {
	KUNIT_CASE(ring_buffer_init_test),
	{}
};

static struct kunit_suite ring_buffer_test_suite = {
	.name = "ring_buffer_test",
	.test_cases = ring_buffer_test_cases,
};

kunit_test_suite(ring_buffer_test_suite);

MODULE_DESCRIPTION("Virtual Memory Profiler Kunit tests");
MODULE_LICENSE("GPL v2");
