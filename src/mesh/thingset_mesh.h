/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup thingset_mesh
 * @{
 */

#ifndef THINGSET_MESH_H_
#define THINGSET_MESH_H_

#include "../thingset.h"
#include "../ts_port.h"

#include "tsm_config.h"

/* forward declaration */
struct tsm_context;
struct tsm_do_config;
struct tsm_do_data;
struct tsm_port;

/**
 * @brief Initialize ThingSet Mesh node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] ts Pointer to ThingSet context of this node.
 * @param[in] do_config Pointer to immutable data for Thingset Mesh protocol data objects.
 * @param[in] do_data Pointer to mutable data for Thingset Mesh protocol data objects.
 * @param[in] ports Pointer to array of ports the ThingSet mesh node uses as interfaces.
 * @param[in] port_count Number of ports in the port array.
 * @returns 0 on success, <0 otherwise
 */
int tsm_node_init(struct tsm_context *tsm, struct ts_context *ts,
                  const struct tsm_do_config *do_config, struct tsm_do_data *do_data,
                  const struct tsm_port *ports, uint8_t port_count);

/**
 * @brief Start message exchange on ThingSet Mesh node.
 *
 * The ThingSet Mesh node must be initialized before message exchange
 * can be started.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @returns 0 on success, <0 otherwise
 */
int tsm_node_start(struct tsm_context *tsm);

/**
 * @brief Stop message exchange on ThingSet Mesh node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @returns 0 on success, <0 otherwise.
 */
int tsm_node_stop(struct tsm_context *tsm);

/**
 * @brief Monitor ThingSet Mesh communcation for health.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @returns 0 on health, <0 otherwise
 */
int tsm_node_monitor(struct tsm_context *tsm);


#endif /* THINGSET_MESH_H_ */

/** @} thingset_mesh */
