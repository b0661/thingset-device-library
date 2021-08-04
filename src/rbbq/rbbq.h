/*
 * Copyright (c) 2020, 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RBBQ_H
#define RBBQ_H

#include <stdint.h>

/* forward declaration */
struct rbbq;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate a message in the transmit buffer.
 *
 * The message has to be transmitted to hand it over to the
 * remote side.
 *
 * @note A message that is allocated but not transmitted blocks allocation.
 *
 * @param[in] buffer Pointer to the message buffer
 * @param[in] channel Message channel
 * @param[in] size The size of the message
 * @param[out] message Pointer to memory allocated to the message
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 * @return 0 on success, <0 otherwise
 */
int rbbq_alloc(struct rbbq *buffer, uint16_t channel, uint16_t size,
               uint8_t **message, uint32_t timeout_ms);


/**
 * @brief Transmit allocated message.
 *
 * @note The message shall not be accessed after it is transmitted.
 *
 * @param[in] buffer Pointer to the message buffer
 * @param[in] message Pointer to the message
 * @return 0 on success, <0 otherwise
 */
int rbbq_transmit(struct rbbq *buffer, uint8_t *message);


/**
 * @brief Receive message.
 *
 * @param[in] buffer Pointer to the message buffer
 * @param[out] channel Message channel
 * @param[out] size The size of the message
 * @param[out] message Pointer to the message
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 * @return 0 on success, <0 otherwise
 */
int rbbq_receive(struct rbbq *buffer, uint16_t *channel, uint16_t *size,
                 uint8_t **message, uint32_t timeout_ms);


/**
 * @brief Free received message from the receive buffer.
 *
 * @note The message shall not be accessed after it is freed.
 *
 * @param[in] buffer Pointer to the message buffer
 * @param[in] message Pointer to the message
 * @return 0 on success, <0 otherwise
 */
int rbbq_free(struct rbbq *buffer, uint8_t *message);


/**
 * @brief Initialise a buffer.
 *
 * Buffer must be initailized before it can be used.
 *
 * @param[in] buffer Pointer to the message buffer
 * @return 0 on success, <0 otherwise
 */
int rbbq_init(struct rbbq *buffer);


/**
 * @brief Start message exchange on buffer.
 *
 * Buffer must be initailized before message exchange can be started.
 *
 * @param[in] buffer Pointer to the message buffer
 * @return 0 on success, <0 otherwise
 */
int rbbq_start(struct rbbq *buffer);


/**
 * @brief Stop message exchange on buffer.
 *
 * @param[in] buffer Pointer to the message buffer
 * @return 0 on success, <0 otherwise
 */
int rbbq_stop(struct rbbq *buffer);


/**
 * @brief Wait for next receive transfer.
 *
 * @param[in] buffer Pointer to the buffer
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 */
int rbbq_wait_receive(struct rbbq *buffer, uint32_t timeout_ms);


/**
 * @brief Wait for next transmit transfer.
 *
 * @param[in] buffer Pointer to the buffer
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 */
int rbbq_wait_transmit(struct rbbq *buffer, uint32_t timeout_ms);


/**
 * @brief Monitor rbbq communcation for health.
 *
 * @param[in] buffer Pointer to the message buffer
 * @return 0 on health, <0 otherwise
 */
int rbbq_monitor(struct rbbq *buffer);


/**
 * @brief Name of the buffer.
 *
 * @param[in] buffer Pointer to the buffer
 * @return Pointer to name, 0 if no name found.
 */
const char *rbbq_name(struct rbbq *buffer);


/**
 * @brief Retrieve the buffer by name.
 *
 * The call locks the buffer to the current thread.
 *
 * @param[in] name Buffer name to search for.
 * @return pointer to buffer structure; NULL if not found or cannot be used.
 */
struct rbbq *rbbq_get_binding(const char *name);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RBBQ_H */
