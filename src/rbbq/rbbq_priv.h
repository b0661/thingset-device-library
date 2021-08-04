/*
 * Copyright (c) 2020, 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RBBQ_PRIV_H
#define RBBQ_PRIV_H

#include "rbbq.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ThingSet adaptations to environment */
#if CONFIG_THINGSET_ZEPHYR
#include "../../zephyr/thingset_zephyr.h"
#else /* ! CONFIG_THINGSET_ZEPHYR */
#include "../thingset_default.h"
#endif /* CONFIG_THINGSET_ZEPHYR */


/* Buffer states */
#define RBBQ_BUFFER_STATE_NONE          0
#define RBBQ_BUFFER_STATE_INIT          1
#define RBBQ_BUFFER_STATE_READY         2
#define RBBQ_BUFFER_STATE_START         3
#define RBBQ_BUFFER_STATE_RUNNING       4
#define RBBQ_BUFFER_STATE_STOP          5
#define RBBQ_BUFFER_STATE_SUSPENDED     6

/*
 * Buffer architecture.
 *
 * Transmit buffer:
 * - buffer data
 *   - raw messages
 *     - header (big endian)
 *     - payload == message
 * - buffer control (big endian)
 *
 * Receive buffer:
 * - buffer data
 *   - raw messages
 *     - header (big endian)
 *     - payload == message
 * - buffer control (big endian)
 */


/**
 * @brief Buffer control.
 *
 * Buffer control is part of the transmit buffer and the receive buffer.
 * It is transmitted to the other side of the communication together with the
 * data.
 *
 * @note Shall be initialised by the device on init.
 */
struct rbbq_control {
    uint16_t this_write_idx;    /**< write index on this buffer */
    uint16_t this_watermark_idx;    /**< watermark index on this buffer */
    uint16_t other_read_idx;    /**< read index on other buffer */
};

/*
 * Device architectur:
 *
 * Buffers work on devices.
 */

/**
 * @brief Device API
 */
struct rbbq_device_api {
    /**
     * @brief buffer requests device to initialise.
     */
    int (*init)(struct rbbq *buffer);
    /**
     * @brief buffer requests device to start message exchange.
     */
    int (*start)(struct rbbq *buffer);
    /**
     * @brief buffer requests device to stop message exchange.
     */
    int (*stop)(struct rbbq *buffer);
    /**
     * @brief buffer informs device that a new transmit message is available.
     *
     * The device shall take:
     * - buffer->alloc_watermark_idx
     * - buffer->alloc_write_idx
     * and update at a convenient time (if transmit buffer is not locked by transfer op):
     * - buffer->device->tx_control->this_watermark_idx
     * - buffer->device->tx_control->this_write_idx
     *
     * The call shall be non-blocking.
     */
    int (*transmit)(struct rbbq *buffer);
    /**
     * @brief buffer informs device that a received message was freed.
     *
     * The device shall take:
     * - buffer->free_read_idx
     * and update at a convenient time (if transmit buffer is not locked by transfer op):
     * - buffer->device->tx_control->other_read_idx
     *
     * The call shall be non-blocking.
     */
    int (*receive)(struct rbbq *buffer);
    /**
     * @brief buffer requests device to monitor message exchange.
     */
    int (*monitor)(struct rbbq *buffer);
};

/**
 * @brief Device management structure.
 */
struct rbbq_device {
    const struct rbbq_device_api *device_api;
    struct rbbq_control *tx_control;
    struct rbbq_control *rx_control;
    uint16_t tx_data_size;
    uint8_t  *tx_data;
    uint16_t rx_data_size;
    uint8_t  *rx_data;
};

/**
 * @brief Buffer management structure.
 *
 * @note Shall be initialised by the buffer on init.
 */
struct rbbq {
    struct rbbq *next;          /**< Mange all buffers */
    const char *name;           /**< Name of the buffer */

    ts_signal_t signal_device_receive;
    ts_signal_t signal_device_transmit;

    const struct rbbq_device *device;

    ts_mutex_t alloc_mutex;         /**< Mutex to lock allocation operation up to transmit */
    uint8_t *alloc_message;         /**< Buffer currently allocated */
    uint16_t alloc_write_idx;       /**< write index of allocated data on tx buffer */
    uint16_t alloc_watermark_idx;   /**< watermark index of allocated data on tx buffer */
    ts_atomic_t tx_shadow_read_idx; /**< Shadow read idx on tx buffer */

    ts_mutex_t receive_mutex;       /**< Mutex to lock receive operation from receive to free */
    uint8_t *free_message;          /**< Buffer currently allocated */
    uint16_t free_read_idx;         /**< read index of freed data on rx buffer */

    ts_atomic_t state;              /**< Buffer state. */
};


/*
 * Message architectur:
 *
 * Header:
 * - channel: 2 bytes in big endian byte order
 * - payload size: 2 bytes in big endian byte order
 * Payload:
 * - data: N bytes
 */

/**
 * @brief Message header
 */
struct rbbq_message_header {
    uint16_t channel;
    uint16_t size;
};

/**
 * @brief Get the state of the buffer.
 *
 * Atomic get of the buffer state.
 */
static inline atomic_val_t rbbq_state(struct rbbq *buffer)
{
    return ts_atomic_get(&buffer->state);
}


/**
 * @brief Set the state of the buffer.
 *
 * Atomic set of the buffer state.
 */
static inline void rbbq_state_set(struct rbbq *buffer, atomic_val_t state)
{
    ts_atomic_set(&buffer->state, state);
}


/**
 * @brief Compare and set the state of the buffer.
 *
 * This routine performs an atomic compare-and-set on the buffer state.
 *
 * If the current value of the buffer state equals old_state, the buffer state is set to new_state.
 * If the current value of the buffer state does not equal old_state, the buffer state
 * is left unchanged.
 *
 * @param buffer
 * @param old_state Original state to compare against.
 * @param new_state New state to store.
 * @return True if buffer state is set to new_state, false otherwise.
 */
static inline bool rbbq_state_cas(struct rbbq *buffer, atomic_val_t old_state,
                                  atomic_val_t new_state)
{
    return ts_atomic_cas(&buffer->state, old_state, new_state);
}


/**
 * @brief Get the channel of the raw message.
 */
static inline uint16_t rbbq_message_channel(uint8_t *raw_message)
{
    struct rbbq_message_header *header = (struct rbbq_message_header *)raw_message;

    return sys_get_be16((const uint8_t *)&header->channel);
}


/**
 * @brief Set the channel of the raw message.
 */
static inline void rbbq_message_channel_set(uint8_t *raw_message, uint16_t channel)
{
    struct rbbq_message_header *header = (struct rbbq_message_header *)raw_message;

    sys_put_be16(channel, (uint8_t *)&header->channel);
}


/**
 * @brief Get the payload size of the raw message.
 */
static inline uint16_t rbbq_message_payload_size(uint8_t *raw_message)
{
    struct rbbq_message_header *header = (struct rbbq_message_header *)raw_message;

    return sys_get_be16((const uint8_t *)&header->size);
}


/**
 * @brief Set the payload size of the raw message.
 */
static inline void rbbq_message_payload_size_set(uint8_t *raw_message,
                                                 uint16_t size)
{
    struct rbbq_message_header *header = (struct rbbq_message_header *)raw_message;

    sys_put_be16(size, (uint8_t *)&header->size);
}


/**
 * @brief Get the payload of the raw message.
 */
static inline uint8_t *rbbq_message_payload(uint8_t *raw_message)
{
    return raw_message + sizeof(struct rbbq_message_header);
}


/**
 * @brief Get the size of the raw message.
 */
static inline uint16_t rbbq_message_size(uint8_t *raw_message)
{
    struct rbbq_message_header *header = (struct rbbq_message_header *)raw_message;

    return sys_get_be16((const uint8_t *)&header->size) + sizeof(struct rbbq_message_header);
}


/**
 * @brief Calculate the size of the raw message.
 *
 * @param payload_size The size of the payload == user message.
 */
static inline uint16_t rbbq_message_size_calc(uint16_t payload_size)
{
    return payload_size + sizeof(struct rbbq_message_header);
}

/**
 * @brief Get rbbq control.
 *
 * @param[in] control Pointer to RBBQ control struct
 * @param[out] this_write_idx Write index of this buffer
 * @param[out] this_watermark_idx Watermark index of this buffer
 * @param[out] other_read_idx Read index of the other buffer
 */
static inline void rbbq_control(const struct rbbq_control *control,
                uint16_t *this_write_idx,
                uint16_t *this_watermark_idx,
                uint16_t *other_read_idx)
{
    *this_write_idx = sys_get_be16((const uint8_t *)&control->this_write_idx);
    *this_watermark_idx = sys_get_be16((const uint8_t *)&control->this_watermark_idx);
    *other_read_idx = sys_get_be16((const uint8_t *)&control->other_read_idx);
}

/**
 * @brief Get other read index of rbbq control.
 *
 * @param[in] control Pointer to RBBQ control struct
 * @return Read index of the other buffer
 */
static inline uint16_t rbbq_control_other_read_idx(const struct rbbq_control *control)
{
    return sys_get_be16((const uint8_t *)&control->other_read_idx);
}


/**
 * @brief Set rbbq control.
 *
 * @param control Pointer to RBBQ control struct
 * @param this_write_idx Write index of this buffer
 * @param this_watermark_idx Watermark index of this buffer
 * @param other_read_idx Read index of the other buffer
 */
static inline void rbbq_control_set(struct rbbq_control *control,
                    uint16_t this_write_idx,
                    uint16_t this_watermark_idx,
                    uint16_t other_read_idx)
{
    sys_put_be16(this_write_idx, (uint8_t *)&control->this_write_idx);
    sys_put_be16(this_watermark_idx, (uint8_t *)&control->this_watermark_idx);
    sys_put_be16(other_read_idx, (uint8_t *)&control->other_read_idx);
}

/**
 * @brief Lock receive buffer.
 *
 * Used internally but also to be called by a rbbq device
 * if the receive buffer can temporarily not be accessed
 * (e.g. due to a non lock free update).
 *
 * @param[in] buffer Pointer to the message buffer
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 */
static inline int rbbq_receive_lock(struct rbbq *buffer, uint32_t timeout_ms)
{
    return ts_mutex_lock(&buffer->receive_mutex, timeout_ms);
}


/**
 * @brief Unlock receive buffer.
 */
static inline void rbbq_receive_unlock(struct rbbq *buffer)
{
    ts_mutex_unlock(&buffer->receive_mutex);
}


/**
 * @brief Callback on receive.
 *
 * To be called by rbbq device after new message data was received.
 *
 * @param[in] buffer Pointer to the message buffer
 */
void rbbq_event_raise_receive(struct rbbq *buffer);


/**
 * @brief Callback on transmit.
 *
 * To be called by rbbq device after message was transmitted.
 *
 * @param[in] buffer Pointer to the buffer
 */
void rbbq_event_raise_transmit(struct rbbq *buffer);


/**
 * @brief Register the buffer to make it available to rbbq_get_binding.
 *
 * The buffer struct must at least be partly filled:
 * - .next = NULL
 * - .buffer = "A buffer name"
 * - .device = pointer to device struct
 *
 * @param[in] buffer Pointer to the buffer
 * @return 0 on success, <0 otherwise
 */
int rbbq_register_binding(struct rbbq *buffer);


/**
 * @brief Register all local RBBQ buffers.
 */
int rbbq_local_register_bindings(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RBBQ_PRIV_H */
