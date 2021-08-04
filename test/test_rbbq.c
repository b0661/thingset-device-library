/*
 * Copyright (c) 2020, 2021 Bobby Noelte
 * SPDX-License-Identifier: Apache-2.0
 */

/* Exclude from default PlatformIO test build if not configured */
#ifdef CONFIG_THINGSET_RBBQ

#include "test.h"

const char rbbq_a_binding[] = "RBBQ_LOCAL0_A";
const char rbbq_b_binding[] = "RBBQ_LOCAL0_B";

#if CONFIG_THINGSET_ZEPHYR
#define RBBQ_A_TRANSMIT_STACK_SIZE    1024
K_THREAD_STACK_DEFINE(rbbq_a_transmit_stack, RBBQ_A_TRANSMIT_STACK_SIZE);
static struct k_thread rbbq_a_transmit_thread;
#endif

/**
 * @brief Helper thread for rbbq message transfer
 *
 * @return N/A
 */
void rbbq_a_transmit_fn(void *arg1, void *arg2, void *arg3)
{
    int ret;

    struct rbbq *rbbq_a;
    uint8_t *rbbq_a_tx_msg;

    rbbq_a = rbbq_get_binding(rbbq_a_binding);
    TEST_ASSERT_NOT_NULL_MESSAGE(rbbq_a, rbbq_a_binding);

    ret = rbbq_alloc(rbbq_a, 1, 1, &rbbq_a_tx_msg, 10);
    TEST_ASSERT_EQUAL(0, ret);

    *rbbq_a_tx_msg = 0xFF;

    ret = rbbq_transmit(rbbq_a, rbbq_a_tx_msg);
    TEST_ASSERT_EQUAL(0, ret);
}

/**
 * @brief Test local rbbq
 *
 * This test verifies local rbbq usage.
 *
 */
void test_rbbq_local(void)
{
    int ret;

    struct rbbq *rbbq_a;
    const char *rbbq_a_name;
    uint8_t *rbbq_a_tx_msg;

    struct rbbq *rbbq_b;
    const char *rbbq_b_name;
    uint8_t *rbbq_b_rx_msg;
    uint16_t rbbq_b_rx_channel;
    uint16_t rbbq_b_rx_size;

    /* Init rbbq a */
    rbbq_a = rbbq_get_binding(rbbq_a_binding);
    TEST_ASSERT_NOT_NULL_MESSAGE(rbbq_a, rbbq_a_binding);

    rbbq_a_name = rbbq_name(rbbq_a);
    TEST_ASSERT_NOT_NULL_MESSAGE(rbbq_a_name, rbbq_a_binding);
    TEST_ASSERT_EQUAL_STRING(rbbq_a_name, rbbq_a_binding);

    ret = rbbq_init(rbbq_a);
    TEST_ASSERT_EQUAL(0, ret);

    ret = rbbq_start(rbbq_a);
    TEST_ASSERT_EQUAL(0, ret);

    /* Init rbbq b */
    rbbq_b = rbbq_get_binding(rbbq_b_binding);
    TEST_ASSERT_NOT_NULL_MESSAGE(rbbq_b, rbbq_b_binding);

    rbbq_b_name = rbbq_name(rbbq_b);
    TEST_ASSERT_NOT_NULL_MESSAGE(rbbq_b_name, rbbq_b_binding);
    TEST_ASSERT_EQUAL_STRING(rbbq_b_name, rbbq_b_binding);

    ret = rbbq_init(rbbq_b);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect error on receive wait request on suspended buffer */
    ret = rbbq_wait_receive(rbbq_b, 10);
    TEST_ASSERT_EQUAL(-EAGAIN, ret);

    /* Expect error on receive request on suspended buffer */
    ret = rbbq_receive(rbbq_b, &rbbq_b_rx_channel,
                       &rbbq_b_rx_size, &rbbq_b_rx_msg, 10);
    TEST_ASSERT_EQUAL(-EAGAIN, ret);

    ret = rbbq_start(rbbq_b);
    TEST_ASSERT_EQUAL(0, ret);

#if CONFIG_THINGSET_ZEPHYR
    /* launch helper thread to transmit something */
    int current_prio = k_thread_priority_get(k_current_get());
    k_thread_create(&rbbq_a_transmit_thread,
            rbbq_a_transmit_stack, K_THREAD_STACK_SIZEOF(rbbq_a_transmit_stack),
            rbbq_a_transmit_fn, NULL, NULL, NULL,
            current_prio - 1, 0, K_NO_WAIT);
#endif

    /* receive */
    ret = rbbq_wait_receive(rbbq_b, 10);
    TEST_ASSERT_EQUAL(0, ret);

    ret = rbbq_receive(rbbq_b, &rbbq_b_rx_channel, &rbbq_b_rx_size,
                       &rbbq_b_rx_msg, 10);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(1, rbbq_b_rx_channel);
    TEST_ASSERT_EQUAL_UINT16(1, rbbq_b_rx_size);
    TEST_ASSERT_EQUAL_UINT8(0xFFU, *rbbq_b_rx_msg);

    /* expect no errors detected by local rbbq monitor */
    ret = rbbq_monitor(rbbq_b);
    TEST_ASSERT_EQUAL(0, ret);

    ret = rbbq_free(rbbq_b, rbbq_b_rx_msg);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect error on double free */
    ret = rbbq_free(rbbq_b, rbbq_b_rx_msg);
    TEST_ASSERT_EQUAL(-ENOMEM, ret);

    /* Stop receive */
    ret = rbbq_stop(rbbq_b);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect success on second stop */
    ret = rbbq_stop(rbbq_b);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect success on buffer allocation at still running transmit buffer */
    ret = rbbq_alloc(rbbq_a, 1, 1, &rbbq_a_tx_msg, 10);
    TEST_ASSERT_EQUAL(0, ret);

    /* Request transmit despite receive is already suspended */
    *rbbq_a_tx_msg = 0xFF;
    ret = rbbq_transmit(rbbq_a, rbbq_a_tx_msg);
    TEST_ASSERT_EQUAL(0, ret);

    /* Should not be transmitted - receive is already suspended */
    ret = rbbq_wait_transmit(rbbq_a, 10);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* Stop transmit */
    ret = rbbq_stop(rbbq_a);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect error on buffer allocation at suspended buffer */
    ret = rbbq_alloc(rbbq_a, 1, 1, &rbbq_a_tx_msg, 10);
    TEST_ASSERT_EQUAL(-EAGAIN, ret);
}

#endif /* CONFIG_THINGSET_RBBQ */
