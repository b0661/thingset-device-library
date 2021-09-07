/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ThingSet communication time handling
 */

#ifndef TS_TIME_H_
#define TS_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ThingSet node system time in milliseconds.
 */
typedef uint32_t ts_time_ms_t;

/**
 * @brief ThingSet node maximum system time value in milliseconds.
 *
 * This is the last value before roll over.
 */
#define TS_TIME_MS_MAX UINT32_MAX

/**
 * @brief Get system time in milliseconds.
 *
 * @note To be provided by the implementation.
 *
 * @return system time in milliseconds
 */
ts_time_ms_t ts_time_ms(void);

/**
 * @brief Get elapsed system time in milliseconds.
 *
 * @note To be provided by the implementation.
 *
 * @param[in] reftime Reference time in milliseconds.
 * @return delta in milliseconds
 */
ts_time_ms_t ts_time_ms_delta(ts_time_ms_t reftime);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TS_TIME_H_ */
