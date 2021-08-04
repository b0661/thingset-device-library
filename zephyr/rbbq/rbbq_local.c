/*
 * Copyright (c) 2020, 2021 Bobby Noelte
 * SPDX-License-Identifier: Apache-2.0
 */

/* Part of the thingset rbbq library. */
#define LOG_MODULE_NAME rbbq

#include "../../src/rbbq/rbbq_priv.h"

#include <device.h>
#include <init.h>

static int rbbq_local_sys_init(const struct device * dev)
{
    ARG_UNUSED(dev);

    return rbbq_local_register_bindings();
}

SYS_INIT(rbbq_local_sys_init, APPLICATION, 0);
