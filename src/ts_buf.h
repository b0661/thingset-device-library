/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ThingSet communication buffer handling.
 */

#ifndef TS_BUF_H_
#define TS_BUF_H_

#include <stdint.h>

#include "ts_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Thingset communication buffer.
 *
 * @note To be provided by the implementation.
 */
struct ts_buf;

/**
 * @brief Allocate a ThingSet communication buffer from the buffer pool.
 *
 * The communication buffer is allocated with reference count set to 1.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] size The size of the buffer
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 * @param[out] buffer Pointer to buffer
 * @return 0 on success, <0 otherwise
 */
int ts_buf_alloc(uint16_t size, ts_time_ms_t timeout_ms, struct ts_buf **buffer);

/**
 * @brief Mark ThingSet communication buffer unused.
 *
 * Decrement the reference count of a buffer. The buffer is put back into the
 * pool if the reference count reaches zero.
 *
 * @note The buffer shall not be accessed after it is marked unused.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @return 0 on success, <0 otherwise
 */
int ts_buf_unref(struct ts_buf *buffer);

/**
 * @brief Mark ThingSet communication buffer used.
 *
 * Increment the reference count of a buffer.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @return 0 on success, <0 otherwise
 */
int ts_buf_ref(struct ts_buf *buffer);

/**
 * @brief Amount of data that this ThingSet communication buffer can store.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @return Amount of data that this buffer can store.
 */
uint16_t ts_buf_size(struct ts_buf *buffer);

/**
 * @brief Amount of data that is stored in this ThingSet communication buffer.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @return Amount of data that is stored in this buffer.
 */
uint16_t ts_buf_len(struct ts_buf *buffer);

/**
 * @brief Get the data pointer for a ThingSet communication buffer.
 *
 * Data pointer points to the first data stored in buffer.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @return data pointer of buffer.
 */
uint8_t *ts_buf_data(struct ts_buf *buffer);

/**
 * @brief Get the tail pointer for a ThingSet communication buffer.
 *
 * Tail pointer points after the last data stored in buffer.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @return tail pointer of buffer.
 */
uint8_t *ts_buf_tail(struct ts_buf *buffer);

/**
 * @brief Prepare data to be added at the end of the buffer.
 *
 * Increments the data length of a buffer to account for more data at the end.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @param[in] len Number of bytes to increment the length with.
 * @return The original tail of the buffer, before incremented by len.
 */
uint8_t *ts_buf_add(struct ts_buf *buffer, uint16_t len);

/**
 * @brief Remove data from the end of the buffer.
 *
 * Removes data from the end of the buffer by modifying the buffer length (not the size).
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @param[in] len Number of bytes to remove.
 * @return The new tail of the buffer.
 */
uint8_t *ts_buf_remove(struct ts_buf *buffer, uint16_t len);

/**
 * @brief Prepare data to be added at start of the buffer.
 *
 * Modifies the data pointer and buffer length to account for more data in the beginning of the
 * buffer.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @param[in] len Number of bytes to add to the beginning.
 * @return The new beginning of the buffer data.
 */
uint8_t *ts_buf_push(struct ts_buf *buffer, uint16_t len);

/**
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data pointer and buffer length.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] buffer Pointer to the buffer.
 * @param[in] len Number of bytes to remove.
 * @return New beginning of the buffer data.
 */
uint8_t *ts_buf_pull(struct ts_buf *buffer, uint16_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TS_BUF_H_ */
