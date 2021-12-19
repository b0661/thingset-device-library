/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ThingSet application
 */

#include "ts_bufq.h"
#include "ts_port.h"
#include "ts_ctx.h"
#include "ts_app.h"


int ts_app_port_init(const struct thingset_port *port)
{
    const struct thingset_app *app = (const struct thingset_app *)port->config;
    struct ts_app_port_data *data = (struct ts_app_port_data *)port->data;

    int ret = ts_bufq_init(&data->rx_bufq);
    if (ret != 0) {
        return ret;
    }

    ret = app->init(app);
    return ret;
}

int ts_app_port_run(const struct thingset_port *port)
{
    const struct thingset_app *app = (const struct thingset_app *)port->config;

    return app->run(app);
}

int ts_app_port_transmit(const struct thingset_port *port, struct thingset_msg *msg,
                         thingset_time_ms_t timeout_ms)
{
    struct ts_app_port_data *data = (struct ts_app_port_data *)port->data;

    return ts_bufq_put(&data->rx_bufq, (struct ts_buf *)msg);
}

THINGSET_PORT_TYPE(ts_app_port, ts_app_port_init, ts_app_port_run, ts_app_port_transmit);

int ts_app_request(thingset_appid_t app_id, struct thingset_msg *request,
                   thingset_time_ms_t timeout_ms, struct thingset_msg **response)
{
    ts_msg_proc_set_port_src(request, app_id);

    /* Let context process the request generated by the application */
    int ret = thingset_process(thingset_app_ctx(app_id), request);
    if (ret != 0) {
        return ret;
    }

    /* Get response of request */
    THINGSET_PORT_DATA_STRUCT(ts_app_port) *port_data = THINGSET_PORT_DATA(ts_app_port,
                                                                           ts_port_by_id(app_id));
    return ts_bufq_get(&port_data->rx_bufq, timeout_ms, (struct ts_buf **)response);
}

const char *thingset_app_name(thingset_appid_t app_id)
{
    return ts_port_by_id(app_id)->name;
};

thingset_locid_t thingset_app_ctx(thingset_appid_t app_id)
{
    return ts_port_by_id(app_id)->loc_id;
}