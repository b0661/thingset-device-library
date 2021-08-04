/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef THINGSET_ZEPHYR_H_
#define THINGSET_ZEPHYR_H_

#if !CONFIG_THINGSET_ZEPHYR
#error "You need to define CONGIG_THINGSET_ZEPHYR."
#endif

/* Logging */
#ifndef LOG_MODULE_NAME
#define LOG_MODULE_NAME thingset
#define LOG_LEVEL CONFIG_THINGSET_LOG_LEVEL
#endif
#include <logging/log.h>

#if LOG_MAIN
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#else
LOG_MODULE_DECLARE(LOG_MODULE_NAME);
#endif

#define LOG_ALLOC_STR(str)	((str == NULL) ? log_strdup("null") : \
                                                log_strdup(str))

#include <zephyr.h>

/* --------------------
 * CONFIG_MINIMAL_LIBC
 * --------------------
 */
#if CONFIG_MINIMAL_LIBC
/*
 * Zephyr's minimal libc is missing some functions.
 * Provide !!sufficient!! replacements here.
 */
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define isnan(value) __builtin_isnan(value)
#define isinf(value) __builtin_isinf(value)

inline long long int llroundf(float x)
{
    return __builtin_llroundf(x);
};

double ts_strtod(const char * string, char **endPtr);
inline double strtod(const char * string, char **endPtr)
{
    return ts_strtod(string, endPtr);
};

inline long long strtoll(const char *str, char **endptr, int base)
{
    /* XXX good enough for thingset uses ?*/
    return (long long)strtol(str, endptr, base);
};

inline unsigned long long strtoull(const char *str, char **endptr, int base)
{
    /* XXX good enough for thingset uses ?*/
    return (unsigned long long)strtoul(str, endptr, base);
};
#endif

/* --------------------
 * CONFIG_THINGSET_RBBQ
 * --------------------
 */
#if CONFIG_THINGSET_RBBQ

#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/mutex.h>
#include <kernel.h>

typedef struct k_poll_signal ts_signal_t;
typedef struct k_poll_event ts_event_t;
typedef struct k_mutex ts_mutex_t;
typedef atomic_t ts_atomic_t;
typedef atomic_val_t ts_atomic_val_t;

static inline int ts_atomic_get(const int* target)
{
    return atomic_get(target);
};

static inline int ts_atomic_set(int* target, int value)
{
    return atomic_set(target, value);
};

static inline bool ts_atomic_cas(int* target, int old_value, int new_value)
{
    return atomic_cas(target, old_value, new_value);
};

static inline int ts_mutex_lock(ts_mutex_t* mutex, uint32_t timeout_ms)
{
    return k_mutex_lock(mutex, K_MSEC(timeout_ms));
};

static inline int ts_mutex_unlock(ts_mutex_t* mutex)
{
    return k_mutex_unlock(mutex);
};

static inline int ts_mutex_init(ts_mutex_t* mutex)
{
     return k_mutex_init(mutex);
};

static inline void ts_signal_init(ts_signal_t* sig)
{
    k_poll_signal_init(sig);
}

static inline int ts_signal_raise(ts_signal_t *sig, int result)
{
    return k_poll_signal_raise(sig, result);
}

static inline void ts_event_init(ts_event_t* event, void* obj)
{
    k_poll_event_init(event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, obj);

    event->signal->signaled = 0;
    event->state = K_POLL_STATE_NOT_READY;
};

static inline int ts_event_poll(ts_event_t* event, uint32_t timeout_ms)
{
    return k_poll(event, 1, K_MSEC(timeout_ms));
};

#endif /* CONFIG_THINGSET_RBBQ */

/* --------------------
 * CONFIG_ZTEST
 * --------------------
 */
#if CONFIG_ZTEST
/*
 * Thingset unit tests are basically made for the Unity test framework.
 * Provide some adaptations to make them run under the ztest framework.
 */
#include "ztest/ztest_unity.h"
#endif

#endif /* THINGSET_ZEPHYR_H_ */
