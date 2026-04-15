// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include "kernel_internal.h"

static void sched_monitor_anomaly_test(struct kunit *test)
{
    struct sched_event event = { 0 };

    g_state.config.flags = SCHED_FLAG_ANOMALY_DETECT;
    g_state.config.threshold_ns = 1000;
    event.delay_ns = 1000;
    KUNIT_EXPECT_TRUE(test, config_check_anomaly(&event));

    event.delay_ns = 999;
    KUNIT_EXPECT_FALSE(test, config_check_anomaly(&event));
}

static void sched_monitor_filter_test(struct kunit *test)
{
    struct sched_event event = { 0 };

    g_state.config.flags = SCHED_FLAG_FILTER_PID;
    g_state.config.filter_pid = 123;

    event.prev_pid = 123;
    event.next_pid = 1;
    KUNIT_EXPECT_TRUE(test, config_check_filter(&event));

    event.prev_pid = 1;
    event.next_pid = 2;
    KUNIT_EXPECT_FALSE(test, config_check_filter(&event));
}

static void sched_monitor_fault_injection_test(struct kunit *test)
{
    int ret;

    atomic_set(&g_state.fi_alloc_fail_once, 1);
    g_state.config.buffer_size = DEFAULT_BUFFER_SIZE;

    ret = ring_buffer_init();
    KUNIT_EXPECT_LT(test, ret, 0);
    KUNIT_EXPECT_PTR_EQ(test, g_state.buffers, NULL);
    ring_buffer_exit();
    atomic_set(&g_state.fi_alloc_fail_once, 0);
}

static struct kunit_case sched_monitor_test_cases[] = {
    KUNIT_CASE(sched_monitor_anomaly_test),
    KUNIT_CASE(sched_monitor_filter_test),
    KUNIT_CASE(sched_monitor_fault_injection_test),
    {}
};

static struct kunit_suite sched_monitor_test_suite = {
    .name = "sched_monitor_kunit",
    .test_cases = sched_monitor_test_cases,
};

kunit_test_suite(sched_monitor_test_suite);

MODULE_LICENSE("GPL");
