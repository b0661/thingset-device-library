/*
 * Copyright (c) 2020, 2021 Bobby Noelte
 * SPDX-License-Identifier: Apache-2.0
 */

/* Part of the thingset rbbq library. */
#define LOG_MODULE_NAME rbbq
#include "rbbq_priv.h"

struct rbbq_device_local {
    /* Must be first to make pointer interchangable with pure rbbq device pointer */
    struct rbbq_device device;
    struct rbbq *other;
};


static int rbbq_local_init(struct rbbq *buffer)
{
    /* Setup TX control */
    buffer->device->tx_control->this_write_idx = 0;
    buffer->device->tx_control->this_watermark_idx = buffer->device->tx_data_size;

    /* Setup RX control */
    buffer->device->tx_control->other_read_idx = 0;

    return 0;
}

static int rbbq_local_start(struct rbbq *buffer)
{
    return 0;
}

static int rbbq_local_stop(struct rbbq *buffer)
{
    return 0;
}

static int rbbq_local_transmit(struct rbbq *buffer)
{
    struct rbbq_device_local *device_local = (struct rbbq_device_local *)buffer->device;

    buffer->device->tx_control->this_watermark_idx = buffer->alloc_watermark_idx;
    buffer->device->tx_control->this_write_idx = buffer->alloc_write_idx;

    rbbq_event_raise_transmit(buffer);
    /* Inform other buffer about new data transmitted */
    rbbq_event_raise_receive(device_local->other);

    return 0;
}

static int rbbq_local_receive(struct rbbq *buffer)
{
    struct rbbq_device_local *device_local = (struct rbbq_device_local *)buffer->device;

    buffer->device->tx_control->other_read_idx = buffer->free_read_idx;
    /* Inform other buffer about receive data read */
    rbbq_event_raise_receive(device_local->other);

    return 0;
}

static int rbbq_local_monitor(struct rbbq *buffer)
{
    return 0;
}

static const struct rbbq_device_api rbbq_local_device_api = {
    .init = rbbq_local_init,
    .start = rbbq_local_start,
    .stop = rbbq_local_stop,
    .transmit = rbbq_local_transmit,
    .receive = rbbq_local_receive,
    .monitor = rbbq_local_monitor,
};

#if CONFIG_THINGSET_RBBQ_LOCAL0

static struct rbbq rbbq_local0_a;
static struct rbbq rbbq_local0_b;

static uint8_t rbbq_local0_transfer_a2b[CONFIG_THINGSET_RBBQ_LOCAL0_TRANSFER_BUFFER_SIZE];
static uint8_t rbbq_local0_transfer_b2a[CONFIG_THINGSET_RBBQ_LOCAL0_TRANSFER_BUFFER_SIZE];

static const struct rbbq_device_local rbbq_local0_device_a = {
    .device.device_api = &rbbq_local_device_api,
    .device.tx_control = (struct rbbq_control *)&rbbq_local0_transfer_a2b[sizeof(rbbq_local0_transfer_a2b) - sizeof(struct rbbq_control)],
    .device.rx_control = (struct rbbq_control *)&rbbq_local0_transfer_b2a[sizeof(rbbq_local0_transfer_b2a) - sizeof(struct rbbq_control)],
    .device.tx_data_size = sizeof(rbbq_local0_transfer_a2b) - sizeof(struct rbbq_control),
    .device.tx_data = &rbbq_local0_transfer_a2b[0],
    .device.rx_data_size = sizeof(rbbq_local0_transfer_b2a) - sizeof(struct rbbq_control),
    .device.rx_data = &rbbq_local0_transfer_b2a[0],
    .other = &rbbq_local0_b,
};

static const struct rbbq_device_local rbbq_local0_device_b = {
    .device.device_api = &rbbq_local_device_api,
    .device.tx_control = (struct rbbq_control *)&rbbq_local0_transfer_b2a[sizeof(rbbq_local0_transfer_b2a) - sizeof(struct rbbq_control)],
    .device.rx_control = (struct rbbq_control *)&rbbq_local0_transfer_a2b[sizeof(rbbq_local0_transfer_a2b) - sizeof(struct rbbq_control)],
    .device.tx_data_size = sizeof(rbbq_local0_transfer_b2a) - sizeof(struct rbbq_control),
    .device.tx_data = &rbbq_local0_transfer_b2a[0],
    .device.rx_data_size = sizeof(rbbq_local0_transfer_a2b) - sizeof(struct rbbq_control),
    .device.rx_data = &rbbq_local0_transfer_a2b[0],
    .other = &rbbq_local0_a,
};

#endif /* CONFIG_THINGSET_RBBQ_LOCAL0 */

#if CONFIG_THINGSET_RBBQ_LOCAL1

static struct rbbq rbbq_local1_a;
static struct rbbq rbbq_local1_b;

static uint8_t rbbq_local1_transfer_a2b[CONFIG_THINGSET_RBBQ_LOCAL1_TRANSFER_BUFFER_SIZE];
static uint8_t rbbq_local1_transfer_b2a[CONFIG_THINGSET_RBBQ_LOCAL1_TRANSFER_BUFFER_SIZE];

static const struct rbbq_device_local rbbq_local1_device_a = {
    .device.device_api = &rbbq_local_device_api,
    .device.tx_control = (struct rbbq_control *)&rbbq_local1_transfer_a2b[sizeof(rbbq_local1_transfer_a2b) - sizeof(struct rbbq_control)],
    .device.rx_control = (struct rbbq_control *)&rbbq_local1_transfer_b2a[sizeof(rbbq_local1_transfer_b2a) - sizeof(struct rbbq_control)],
    .device.tx_data_size = sizeof(rbbq_local1_transfer_a2b) - sizeof(struct rbbq_control),
    .device.tx_data = &rbbq_local1_transfer_a2b[0],
    .device.rx_data_size = sizeof(rbbq_local1_transfer_b2a) - sizeof(struct rbbq_control),
    .device.rx_data = &rbbq_local1_transfer_b2a[0],
    .other = &rbbq_local1_b,
};

static const struct rbbq_device_local rbbq_local1_device_b = {
    .device.device_api = &rbbq_local_device_api,
    .device.tx_control = (struct rbbq_control *)&rbbq_local1_transfer_b2a[sizeof(rbbq_local1_transfer_b2a) - sizeof(struct rbbq_control)],
    .device.rx_control = (struct rbbq_control *)&rbbq_local1_transfer_a2b[sizeof(rbbq_local1_transfer_a2b) - sizeof(struct rbbq_control)],
    .device.tx_data_size = sizeof(rbbq_local1_transfer_b2a) - sizeof(struct rbbq_control),
    .device.tx_data = &rbbq_local1_transfer_b2a[0],
    .device.rx_data_size = sizeof(rbbq_local1_transfer_a2b) - sizeof(struct rbbq_control),
    .device.rx_data = &rbbq_local1_transfer_a2b[0],
    .other = &rbbq_local1_a,
};

#endif /* CONFIG_THINGSET_RBBQ_LOCAL1 */


int rbbq_local_register_bindings(void)
{
    int ret;

#if CONFIG_THINGSET_RBBQ_LOCAL0
    rbbq_local0_a = ((struct rbbq) {
        .next = NULL,
        .name = "RBBQ_LOCAL0_A",
        .device = &rbbq_local0_device_a.device,
    });
    ret = rbbq_register_binding(&rbbq_local0_a);
    __ASSERT(ret == 0, "Could not register RBBQ_LOCAL0_A");

    rbbq_local0_b = ((struct rbbq) {
        .next = NULL,
        .name = "RBBQ_LOCAL0_B",
        .device = &rbbq_local0_device_b.device,
    });
    ret = rbbq_register_binding(&rbbq_local0_b);
    __ASSERT(ret == 0, "Could not register RBBQ_LOCAL0_B");
#endif
#if CONFIG_THINGSET_RBBQ_LOCAL1
    rbbq_local1_a = ((struct rbbq) {
        .next = NULL,
        .name = "RBBQ_LOCAL1_A",
        .device = &rbbq_local1_device_a.device,
    });
    ret = rbbq_register_binding(&rbbq_local1_a);
    __ASSERT(ret == 0, "Could not register RBBQ_LOCAL1_A");

    rbbq_local1_b = ((struct rbbq) {
        .next = NULL,
        .name = "RBBQ_LOCAL1_B",
        .device = &rbbq_local1_device_b.device,
    });
    ret = rbbq_register_binding(&rbbq_local1_b);
    __ASSERT(ret == 0, "Could not register RBBQ_LOCAL1_B");
#endif

    return ret;
}
