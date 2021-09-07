/*
 * Copyright (c) 2021 Bobby Noelte
 * SPDX-License-Identifier: Apache-2.0
 */

/* Exclude from default PlatformIO test build if not configured */
#ifdef CONFIG_THINGSET_COM

#include "test.h"

/**
 * @brief Test communication buffer
 *
 * This test verifies mesh buffer usage:
 * - ts_buf_alloc()
 * - ts_buf_ref()
 * - ts_buf_unref()
 *
 */
void test_buf(void)
{
    int ret;
    struct ts_buf *buffer = NULL;

    /* Check buffer pool size for testing */
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(16,  TS_BUF_COUNT);
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(1024, TS_BUF_DATA_SIZE);

    ret = ts_buf_alloc(10, 10, &buffer);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(buffer);

    ret = ts_buf_unref(buffer);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect second unref to fail */
    ret = ts_buf_unref(buffer);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    ret = ts_buf_alloc(10, 10, &buffer);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_NOT_NULL(buffer);

    ret = ts_buf_ref(buffer);
    TEST_ASSERT_EQUAL(0, ret);

    ret = ts_buf_unref(buffer);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect second unref to pass due to extra ts_buf_ref() */
    ret = ts_buf_unref(buffer);
    TEST_ASSERT_EQUAL(0, ret);

    /* Expect third unref to fail */
    ret = ts_buf_unref(buffer);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

#endif /* CONFIG_THINGSET_COM */
