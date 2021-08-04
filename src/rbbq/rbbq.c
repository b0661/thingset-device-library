/**
 * Copyright (c) 2020, 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Design idea taken from https://blog.systems.ethz.ch/blog/2019/the-design-and-implementation-of-a-lock-free-ring-buffer-with-contiguous-reservations.html
 */

/* The main implementation file of the thingset rbbq library. */
#define LOG_MAIN 1
#define LOG_MODULE_NAME rbbq

#include "rbbq_priv.h"

#define RBBQ_MESSAGE_ALLOC_NOMEM        (-1)
#define RBBQ_MESSAGE_ALLOC_AT_END       0
#define RBBQ_MESSAGE_ALLOC_AT_START     1
#define RBBQ_MESSAGE_ALLOC_AT_MIDDLE    2
#define RBBQ_MESSAGE_FREE_NOMEM         (-1)
#define RBBQ_MESSAGE_FREE_TO_WRITE      0
#define RBBQ_MESSAGE_FREE_TO_WATERMARK  1
#define RBBQ_MESSAGE_CORRUPTED          (-2)

/**
 * @brief Head of linked list of registered buffers.
 */
static struct rbbq *rbbq_buffers = 0;

static inline int rbbq_alloc_avail(const struct rbbq *buffer, uint16_t size)
{
    uint16_t write_idx = buffer->alloc_write_idx;
    uint16_t read_idx = ts_atomic_get(&buffer->tx_shadow_read_idx);
    uint16_t end_idx = buffer->device->tx_data_size;

    if (write_idx >= read_idx) {
        if (size <= (end_idx - write_idx)) {
            return RBBQ_MESSAGE_ALLOC_AT_END;
        } else if (size < read_idx) {
            return RBBQ_MESSAGE_ALLOC_AT_START;
        }
    } else /* write_idx < read_idx */ {
        if (size < (read_idx - write_idx)) {
            return RBBQ_MESSAGE_ALLOC_AT_MIDDLE;
        }
    }
    return RBBQ_MESSAGE_ALLOC_NOMEM;
}

int rbbq_alloc(struct rbbq *buffer, uint16_t channel, uint16_t size,
               uint8_t **message, uint32_t timeout_ms)
{
    int ret;

    if (rbbq_state(buffer) != RBBQ_BUFFER_STATE_RUNNING) {
        return -EAGAIN;
    }

    ret = ts_mutex_lock(&buffer->alloc_mutex, timeout_ms);
    if (ret != 0) {
        /* cannot lock alloc..transmit operation */
        return ret;
    }

    uint8_t *raw_message;
    int alloc_mode = rbbq_alloc_avail(buffer, size + sizeof(struct rbbq_message_header));
    switch (alloc_mode) {
    case RBBQ_MESSAGE_ALLOC_AT_END:
        raw_message = buffer->device->tx_data + buffer->alloc_write_idx;
        rbbq_message_channel_set(raw_message, channel);
        rbbq_message_payload_size_set(raw_message, size);
        *message = rbbq_message_payload(raw_message);

        buffer->alloc_message = raw_message;
        buffer->alloc_watermark_idx = buffer->device->tx_data_size;
        buffer->alloc_write_idx += rbbq_message_size(raw_message);
        break;
    case RBBQ_MESSAGE_ALLOC_AT_START:
        raw_message = buffer->device->tx_data;
        rbbq_message_channel_set(raw_message, channel);
        rbbq_message_payload_size_set(raw_message, size);
        *message = rbbq_message_payload(raw_message);

        buffer->alloc_message = raw_message;
        buffer->alloc_watermark_idx = buffer->alloc_write_idx;
        buffer->alloc_write_idx = rbbq_message_size(raw_message);
        break;
    case RBBQ_MESSAGE_ALLOC_AT_MIDDLE:
        raw_message = buffer->device->tx_data + buffer->alloc_write_idx;
        rbbq_message_channel_set(raw_message, channel);
        rbbq_message_payload_size_set(raw_message, size);
        *message = rbbq_message_payload(raw_message);

        buffer->alloc_message = raw_message;
        buffer->alloc_write_idx += rbbq_message_size(raw_message);
        break;
    case RBBQ_MESSAGE_ALLOC_NOMEM:
        ret = -ENOMEM;
        k_mutex_unlock(&buffer->alloc_mutex);
        break;
    default:
        /* should never happend */
        ret = -EFAULT;
        ts_mutex_unlock(&buffer->alloc_mutex);
        break;
    }
    return ret;
}


int rbbq_transmit(struct rbbq *buffer, uint8_t *message)
{
    if (rbbq_state(buffer) != RBBQ_BUFFER_STATE_RUNNING) {
        return -EAGAIN;
    }

    if ((message == NULL) || (message != rbbq_message_payload(buffer->alloc_message))) {
        return -EINVAL;
    }

    /* Inform device that a new transmit message is available. */
    int ret =  buffer->device->device_api->transmit(buffer);

    buffer->alloc_message = NULL;

    ts_mutex_unlock(&buffer->alloc_mutex);

    return ret;
}


/**
 * @brief Is there a messsage received that can be freed.
 *
 * Two possible memory configurations
 * - write leads and read follows (write ≥ read),
 *   the valid data (written, but not yet processed by the reader)
 *   is in the section of the buffer after read and before write;
 * - read leads and write follows (read > write),
 *   the valid data is after read, till the watermark, and from the
 *   start of the buffer till write.
 */
static inline int rbbq_message_free_avail_unprotected(const struct rbbq *buffer)
{
    int ret;
    uint16_t avail_size;
    uint16_t write_idx;
    uint16_t watermark_idx;
    uint16_t read_idx;

    /* Read from rx control */
    uint16_t other_read_idx;
    rbbq_control(buffer->device->rx_control, &write_idx, &watermark_idx, &other_read_idx);

    /* We work on this read_idx (not other) */
    read_idx = buffer->free_read_idx;

    if (write_idx >= read_idx) {
        avail_size = write_idx - read_idx;
        ret = RBBQ_MESSAGE_FREE_TO_WRITE;
    } else /* write_idx < read_idx */ {
        avail_size = watermark_idx - read_idx;
        ret = RBBQ_MESSAGE_FREE_TO_WATERMARK;
    }
    if (avail_size == 0) {
        ret = RBBQ_MESSAGE_FREE_NOMEM;
    } else {
        uint8_t *raw_message = &buffer->device->rx_data[read_idx];
        if (rbbq_message_size(raw_message) > avail_size) {
            /* corrupted data */
            ret = RBBQ_MESSAGE_CORRUPTED;
        }
    }
    return ret;
}


int rbbq_receive(struct rbbq *buffer, uint16_t *channel, uint16_t *size,
                 uint8_t **message, uint32_t timeout_ms)
{
    if (rbbq_state(buffer) != RBBQ_BUFFER_STATE_RUNNING) {
        LOG_WRN("%s receive request on buffer not running (state: %d)",
            rbbq_name(buffer), (int)rbbq_state(buffer));
        return -EAGAIN;
    }

    int ret;
    do {
        ret = rbbq_receive_lock(buffer, timeout_ms);
        if (ret != 0) {
            /* cannot lock receive..free operation */
            return ret;
        }
        int free_mode = rbbq_message_free_avail_unprotected(buffer);
        if (free_mode == RBBQ_MESSAGE_FREE_NOMEM) {
            /* no message available */
            rbbq_receive_unlock(buffer);
            ret = rbbq_wait_receive(buffer, timeout_ms);
            if (ret == 0) {
                continue;
            }
            ret = -ENOMEM;
        } else if (free_mode == RBBQ_MESSAGE_CORRUPTED) {
            rbbq_receive_unlock(buffer);
            ret = -EFAULT;
        } else {
            ret = 0;
        }
    } while (false);
    if (ret == 0) {
        uint8_t *raw_message = &buffer->device->rx_data[buffer->free_read_idx];
        *channel = rbbq_message_channel(raw_message);
        *size = rbbq_message_payload_size(raw_message);
        *message = rbbq_message_payload(raw_message);

        buffer->free_message = raw_message;
        buffer->free_read_idx += rbbq_message_size(raw_message);
    }

    return ret;
}


int rbbq_free(struct rbbq *buffer, uint8_t *message)
{
    if (rbbq_state(buffer) == RBBQ_BUFFER_STATE_NONE) {
        return -ENODEV;
    }

    if (buffer->free_message == NULL) {
        /* double free */
        return -ENOMEM;
    }
    if ((message == NULL) || (message != rbbq_message_payload(buffer->free_message))) {
        return -EINVAL;
    }

    /* Inform device that a received message was freed. */
    int ret = buffer->device->device_api->receive(buffer);
    buffer->free_message = NULL;

    rbbq_receive_unlock(buffer);

    return ret;
}


int rbbq_init(struct rbbq *buffer)
{
    if (!rbbq_state_cas(buffer, RBBQ_BUFFER_STATE_NONE, RBBQ_BUFFER_STATE_INIT)) {
        return -EEXIST;
    }

    ts_signal_init(&buffer->signal_device_receive);
    ts_signal_init(&buffer->signal_device_transmit);
    (void)ts_mutex_init(&buffer->alloc_mutex);
    buffer->alloc_message = NULL;
    buffer->alloc_write_idx = 0;
    buffer->alloc_watermark_idx = 0xFFFF;
    (void)ts_mutex_init(&buffer->receive_mutex);
    buffer->free_message = NULL;
    buffer->free_read_idx = 0;

    /* call the device to finalise initialisation */
    int ret = buffer->device->device_api->init(buffer);
    if (ret != 0) {
        rbbq_state_set(buffer, RBBQ_BUFFER_STATE_NONE);
        return ret;
    }

    /* sync alloc and free markers to size of buffer set by device */
    rbbq_control(buffer->device->tx_control,
                 &buffer->alloc_write_idx,
                 &buffer->alloc_watermark_idx,
                 &buffer->free_read_idx);

    rbbq_state_set(buffer, RBBQ_BUFFER_STATE_READY);
    return ret;
}


int rbbq_start(struct rbbq *buffer)
{
    atomic_val_t last_state;

    if (!rbbq_state_cas(buffer, RBBQ_BUFFER_STATE_READY, RBBQ_BUFFER_STATE_START)) {
        if (!rbbq_state_cas(buffer, RBBQ_BUFFER_STATE_SUSPENDED, RBBQ_BUFFER_STATE_START)) {
            return -EBUSY;
        } else {
            last_state = RBBQ_BUFFER_STATE_SUSPENDED;
        }
    } else {
        last_state = RBBQ_BUFFER_STATE_READY;
    }

    /* Request device to start message exchange. */
    int ret = buffer->device->device_api->start(buffer);
    if (ret != 0) {
        rbbq_state_set(buffer, last_state);
    } else {
        rbbq_state_set(buffer, RBBQ_BUFFER_STATE_RUNNING);
    }

    return ret;
}


int rbbq_stop(struct rbbq *buffer)
{
    if (rbbq_state(buffer) == RBBQ_BUFFER_STATE_SUSPENDED) {
        /* Buffer already stopped */
        return 0;
    }
    if (!rbbq_state_cas(buffer, RBBQ_BUFFER_STATE_RUNNING,
                                RBBQ_BUFFER_STATE_STOP)) {
        return -EBUSY;
    }

    /* Request device to stop message exchange. */
    int ret = buffer->device->device_api->stop(buffer);
    if (ret != 0) {
        rbbq_state_set(buffer, RBBQ_BUFFER_STATE_RUNNING);
    } else {
        rbbq_state_set(buffer, RBBQ_BUFFER_STATE_SUSPENDED);
    }

    return ret;
}


int rbbq_wait_receive(struct rbbq *buffer, uint32_t timeout_ms)
{
    ts_event_t event;

    ts_event_init(&event, &buffer->signal_device_receive);

    int ret = ts_event_poll(&event, timeout_ms);

    return ret;
}


int rbbq_wait_transmit(struct rbbq *buffer, uint32_t timeout_ms)
{
    ts_event_t event;

    ts_event_init(&event, &buffer->signal_device_transmit);

    int ret = ts_event_poll(&event, timeout_ms);

    return ret;
}


int rbbq_monitor(struct rbbq *buffer)
{
    return buffer->device->device_api->monitor(buffer);
}


const char *rbbq_name(struct rbbq *buffer)
{
    return buffer->name;
}


struct rbbq *rbbq_get_binding(const char *name)
{
    struct rbbq *buffer = rbbq_buffers;
    while (buffer != NULL) {
        if (strcmp(buffer->name, name) == 0) {
            return buffer;
        }
        buffer = buffer->next;
    }
    return NULL;
}

/*
 * -----------------------------------------------
 * INTERNAL interface
 * -----------------------------------------------
 */

void rbbq_event_raise_receive(struct rbbq *buffer)
{
    /*
     * We got a receive buffer update.
     * Store read_idx_other from receive buffer to shadow store to make it
     * available even if the receive buffer may be locked due to corrupted
     * data or receive operation later on.
     */
    ts_atomic_set(&buffer->tx_shadow_read_idx, rbbq_control_other_read_idx(buffer->device->rx_control));

    ts_signal_raise(&buffer->signal_device_receive, 0);
}


void rbbq_event_raise_transmit(struct rbbq *buffer)
{
    ts_signal_raise(&buffer->signal_device_transmit, 0);
}


int rbbq_register_binding(struct rbbq *new_buffer)
{
    if ((new_buffer->next != NULL)
        || (new_buffer->name == NULL)
        || (new_buffer->device == NULL)) {
        return -EINVAL;
    }

    if (rbbq_buffers == NULL) {
        /* first buffer */
        rbbq_buffers = new_buffer;
    } else {
        struct rbbq *buffer = rbbq_buffers;
        while (buffer != NULL) {
            if (buffer == new_buffer) {
                /* Already registered */
                return -EALREADY;
            }
            if (buffer->next == NULL) {
                buffer->next = new_buffer;
                new_buffer->next = NULL;
                break;
            }
            buffer = buffer->next;
        }
    }
    return 0;
}
