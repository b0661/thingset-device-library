/*
 * Copyright (c) 2017 Martin Jäger / Libre Solar
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ThingSet adaptations to environment
 */

#ifndef TS_ENV_H_
#define TS_ENV_H_

#if CONFIG_THINGSET_ZEPHYR
#include "../zephyr/thingset_zephyr.h"
#else /* ! CONFIG_THINGSET_ZEPHYR */

/*
 * Default environment
 * -------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG 0

#define LOG_DBG(...) printf(__VA_ARGS__)
#define LOG_ERR(...) printf(__VA_ARGS__)
#define LOG_ALLOC_STR(str) str

#include <stdio.h>

#ifdef UNIT_TEST
#include <unity.h>
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* Default environment */

#endif /* TS_ENV_H_ */
