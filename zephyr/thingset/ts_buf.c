/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ts_com
#define LOG_LEVEL CONFIG_THINGSET_COM_LOG_LEVEL
#include "../../src/ts_env.h"

#include <net/buf.h>

#include "../../src/ts_config.h"
#include "../../src/ts_buf.h"

/**
 * @brief Device's communication buffers pool.
 *
 * Pool of ThingSet communication buffers used by (all) ThingSet communication of the device.
 */
NET_BUF_POOL_VAR_DEFINE(ts_buf_pool, TS_BUF_COUNT, TS_BUF_DATA_SIZE, NULL);


int ts_buf_alloc(uint16_t size, ts_time_ms_t timeout_ms, struct ts_buf **buffer)
{
    struct net_buf *buf = net_buf_alloc_len(&ts_buf_pool, (size_t)size,
                                            K_MSEC(timeout_ms));
    if (buf == NULL) {
        return -ENOMEM;
    }
    *buffer = (struct ts_buf *)buf;
    return 0;
}

int ts_buf_unref(struct ts_buf *buffer)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    if (buf->ref == 0) {
        /*
         * Add some extra safety here as it is not tracked by Zephyr.
         * Will not work in all cases (aka. re-allocation of same message
         * in between).
         */
        LOG_ERR("ThingSet marks already unused buffer unused.");
        return -EALREADY;
    }

    net_buf_unref(buf);

    return 0;
}

int ts_buf_ref(struct ts_buf *buffer)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    net_buf_ref(buf);
    return 0;
}

uint16_t ts_buf_size(struct ts_buf *buffer)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return buf->size;
}

uint16_t ts_buf_len(struct ts_buf *buffer)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return buf->len;
}

uint8_t *ts_buf_data(struct ts_buf *buffer)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return buf->data;
}

uint8_t *ts_buf_tail(struct ts_buf *buffer)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return net_buf_tail(buf);
}

uint8_t *ts_buf_add(struct ts_buf *buffer, uint16_t len)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return (uint8_t *)net_buf_add(buf, (size_t)len);
}

uint8_t *ts_buf_remove(struct ts_buf *buffer, uint16_t len)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return (uint8_t *)net_buf_remove_mem(buf, (size_t)len);
}

uint8_t *ts_buf_push(struct ts_buf *buffer, uint16_t len)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return (uint8_t *)net_buf_push(buf, (size_t)len);
}

uint8_t *ts_buf_pull(struct ts_buf *buffer, uint16_t len)
{
    struct net_buf *buf = (struct net_buf *)buffer;

    return (uint8_t *)net_buf_pull(buf, (size_t)len);
}
