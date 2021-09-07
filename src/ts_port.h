/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ThingSet communication port handling
 */

#ifndef TS_PORT_H_
#define TS_PORT_H_

#include <stdint.h>

#include "ts_time.h"
#include "ts_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ThingSet node identifier.
 *
 * A node identifier identifies a specific node that can be accessed by a port. The translation
 * of the node identifier toport specific adressing (e.g. CAN ID) has to be done by the port
 * implementation.
 */
typedef uint64_t ts_nodeid_t;

/**
 * @brief A ThingSet communication port.
 *
 * Runtime port structure (in ROM) per port instance.
 */
struct ts_port {

    int (*open)(const struct ts_port *port);

    int (*close)(const struct ts_port *port);

    /**
     * @brief Receive buffer on port.
     *
     * @param[in] port Port to receive at.
     * @param[in] msg Pointer to communication buffer to be used to receive.
     * @param[in] callback_on_received If callback is NULL receive returns on
     *                the next buffer. If the callback is set receive
     *                immediatedly returns and the callback is called on the
     *                reception of the buffer. Beware even in this case the
     *                callback may be called before the receive function
     *                returns.
     * @param[in] timeout_ms maximum time to wait in milliseconds.
     * @param[out] node_id Node ID of node the buffer was recieved from.
     */
    int (*receive)(const struct ts_port *port, struct ts_buf *msg,
                   int (*callback_on_received)(const struct ts_port *port,
                                               const ts_nodeid_t *node_id,
                                               struct ts_buf *msg),
                    ts_time_ms_t timeout_ms, ts_nodeid_t *node_id);

    /**
     * @brief Transmit buffer on port.
     *
     * @param[in] port Port to send at.
     * @param[in] msg Pointer to communication buffer to be send.
     * @param[in] node_id Node ID of node to send the buffer to.
     * @param[in] callback_on_sent If callback is NULL send returns on the
     *                         next buffer. If the callback is set send
     *                         immediatedly returns and the callback is called
     *                         after the transmission of the buffer. Beware
     *                         even in this case the callback may be called
     *                         before the send function returns.
     * @param[in] timeout_ms maximum time to wait in milliseconds.
     */
    int (*transmit)(const struct ts_port *port, struct ts_buf *msg, const ts_nodeid_t *node_id,
                    int (*callback_on_sent)(const struct ts_port *port,
                                            const ts_nodeid_t *node_id,
                                            struct ts_buf *msg),
                    ts_time_ms_t timeout_ms);
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TS_PORT_H_ */
