/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ts_com
#define LOG_LEVEL CONFIG_THINGSET_COM_LOG_LEVEL
#include "../../src/ts_env.h"

#include <kernel.h>

#include "../../src/ts_time.h"

ts_time_ms_t ts_time_ms(void)
{
    return k_uptime_get_32();
}

ts_time_ms_t ts_time_ms_delta(ts_time_ms_t reftime)
{
    ts_time_ms_t curtime = k_uptime_get_32();

    if (curtime >= reftime) {
        return curtime - reftime;
    }
    return (TS_TIME_MS_MAX - reftime) + curtime;
}
