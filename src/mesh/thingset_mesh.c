/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */


#include <string.h>

/* The main implementation file of the thingset mesh library. */
#define LOG_MAIN 1
#define LOG_MODULE_NAME ts_mesh
#define LOG_LEVEL CONFIG_THINGSET_MESH_LOG_LEVEL
#include "../ts_env.h"

#include "thingset_mesh_priv.h"

const ts_nodeid_t *tsm_node_id(struct tsm_context* tsm)
{
    return &tsm->do_config->node_id;
}

tsm_node_seqno_t tsm_seqno(struct tsm_context *tsm)
{
    return tsm->do_data->node_seqno;
}

int tsm_port_get(struct tsm_context *tsm, tsm_port_id_t port_id, const struct tsm_port **port)
{
    if (port_id >= tsm->port_count) {
        return -EINVAL;
    }
        *port = &tsm->ports[port_id];
        return 0;
}

uint8_t tsm_heartbeat_period_s(struct tsm_context *tsm)
{
    return (uint8_t)tsm->do_data->node_heartbeat_period_s;
}

static uint8_t tsm_neighbour_link_throughput(struct tsm_context *tsm, uint16_t neighbour_idx)
{
    __ASSERT(neighbour_idx < tsm->node_table.originator_start_idx,
             "Unexpected neighbour index: %u", (unsigned int)neighbour_idx);

    uint8_t port_id = tsm->node_table.paths[neighbour_idx].neighbour.port_id;
    __ASSERT(port_id < tsm->port_count, "Unexpected port id: %u", (unsigned int)port_id);

    const struct tsm_port *port = &tsm->ports[port_id];
    __ASSERT(port->transmit_throughput != NULL,
             "port %u transmit_throughput function not available", (unsigned int)port_id);

    return port->transmit_throughput();
}

int tsm_node_best_next_hop(struct tsm_context *tsm, uint16_t node_idx, uint16_t *hop_idx,
                           uint8_t *throughput)
{
    struct tsm_node *node = &tsm->node_table.nodes[node_idx];
    uint16_t path_idx = node->paths_refs[TSM_NODE_PATHS_BEST];

    if (path_idx >= ARRAY_SIZE(tsm->node_table.paths)) {
        /* There is no best next hop path */
        /** @todo try to find another best next hop before erroring out */
        return -ENAVAIL;
    }

    if (path_idx < tsm->node_table.originator_start_idx) {
        /* This is a neighbour entry, the node is itself the best next hop */
        *hop_idx = node_idx;
        *throughput = tsm_neighbour_link_throughput(tsm, path_idx);
    }
    else {
        /* This is an originator entry */
        *hop_idx = tsm->node_table.paths[path_idx].originator.router_idx;
        *throughput = tsm->node_table.paths[path_idx].originator.throughput;
    }
    return 0;
}

static inline void tsm_node_path_free(struct tsm_context* tsm, uint16_t path_idx)
{
    __ASSERT(path_idx < ARRAY_SIZE(tsm->node_table.paths), "Unexpected path index: %u",
             (unsigned)path_idx);

    /* Let compiler optimization remove double write - assures both union types are initialized. */
    tsm->node_table.paths[path_idx].neighbour.heartbeat_period_s = UINT8_MAX;
    tsm->node_table.paths[path_idx].originator.throughput = UINT8_MAX;
}

void tsm_node_free(struct tsm_context* tsm, uint16_t node_idx)
{
    struct tsm_node *node = &tsm->node_table.nodes[node_idx];

    if (node->protect_window.last_idx >= TSM_NODE_SEQNO_CACHE_SIZE) {
        /* Already unused */
        LOG_DBG("Multiple free for node with index %u", (unsigned)node_idx);
        return;
    }

    /* Mark associated path info unused */
    for (uint8_t refs_i = 0; refs_i < ARRAY_SIZE(node->paths_refs); refs_i++) {
        uint8_t path_idx = node->paths_refs[refs_i];
        if (path_idx < tsm->node_table.originator_start_idx) {
            /*
             * This is a neighbour node - any reference to the node by an originator (using it as a
             * router) has to be removed.
             */
            for (uint16_t other_node_i = 0; other_node_i < ARRAY_SIZE(tsm->node_table.nodes);
                 other_node_i++) {
                struct tsm_node *other_node = &tsm->node_table.nodes[other_node_i];
                if ((other_node->protect_window.last_idx >= TSM_NODE_SEQNO_CACHE_SIZE) /* empty */
                    || (other_node_i == node_idx) /* ourself */ ) {
                    continue;
                }
                for (uint8_t other_refs_i = 0; other_refs_i < ARRAY_SIZE(other_node->paths_refs);
                     other_refs_i++) {
                    uint8_t other_path_idx = other_node->paths_refs[other_refs_i];
                    if (other_path_idx >= ARRAY_SIZE(tsm->node_table.paths)) {
                        /* unused */
                        continue;
                    }
                    if (other_path_idx >= tsm->node_table.originator_start_idx) {
                        /* Other node has an originator table entry */
                        struct tsm_originator *originator =
                            &tsm->node_table.paths[other_path_idx].originator;
                        if (originator->router_idx == node_idx) {
                            /* Remove originator reference with node as router from other node */
                            other_node->paths_refs[other_refs_i] = UINT16_MAX;
                            tsm_node_path_free(tsm, other_path_idx);
                            if (other_refs_i == TSM_NODE_PATHS_BEST) {
                                /* We are removing the best next hop node of the other node */
                                LOG_DBG("Best next hop of node index %u - orphaned",
                                        (unsigned int)other_node_i);
                            }
                        }
                    }
                }
            }
        }
        if (path_idx < ARRAY_SIZE(tsm->node_table.paths)) {
            tsm_node_path_free(tsm, path_idx);
        }
    }
    /* Mark node table entry unused */
    node->protect_window.last_idx = TSM_NODE_SEQNO_CACHE_SIZE;
}

void tsm_node_init_phantom(struct tsm_context *tsm, uint16_t node_idx, const ts_nodeid_t *node_id)
{
    struct tsm_node *node = &tsm->node_table.nodes[node_idx];

    node->node_id = *node_id;
    node->name_mapping_id = TSM_NODE_NAME_MAPPING_ID_INVALID;
    /* Mark entry used, still no seqno due to cache initialized to INVALID sequence numbers */
    node->protect_window.last_idx = 0;
    node->protect_window.last_seen_time = 0;
    for (uint8_t i = 0; i < ARRAY_SIZE(node->protect_window.node_seqno_cache); i++) {
        node->protect_window.node_seqno_cache[i] = TSM_NODE_SEQNO_INVALID;
    }
    for (uint8_t i = 0; i < ARRAY_SIZE(node->paths_refs); i++) {
        node->paths_refs[i] = UINT16_MAX;
    }
}

uint16_t tsm_node_evict(struct tsm_context *tsm)
{
    uint16_t oldest_idx = UINT16_MAX;
    ts_time_ms_t last_seen_time = ts_time_ms();

    for (uint16_t i = 0; i < ARRAY_SIZE(tsm->node_table.nodes); i++) {
        struct tsm_node *node = &tsm->node_table.nodes[i];
        /* Search oldest entry */
        if (node->protect_window.last_seen_time <= last_seen_time) {
            last_seen_time = node->protect_window.last_seen_time;
            oldest_idx = i;
        }
        /* search lowest throughput entry */
        /** @todo Improve eviction candidate search by lowest throughput */
    }

    __ASSERT(oldest_idx != UINT16_MAX, "At least one entry must match.");

    return oldest_idx;
}

int tsm_node_get(struct tsm_context *tsm, const ts_nodeid_t *node_id, uint16_t *node_idx)
{
    uint16_t match_idx = UINT16_MAX;
    uint16_t empty_idx = UINT16_MAX; /* just in case we need it */

    for (uint16_t i = 0; i < ARRAY_SIZE(tsm->node_table.nodes); i++) {
        struct tsm_node *node = &tsm->node_table.nodes[i];
        if (node->protect_window.last_idx >= TSM_NODE_SEQNO_CACHE_SIZE) {
            /* empty */
            if (empty_idx == UINT16_MAX) {
                empty_idx = i;
            }
        }
        else if (tsm_node_id_equal(node_id, &node->node_id)) {
            /* match */
            match_idx = i;
            break;
        }
    }
    if (match_idx == UINT16_MAX) {
        /* Node is missing in node table */
        if (empty_idx == UINT16_MAX) {
            /* No space left in node table - search node to be evicted */
            match_idx = tsm_node_evict(tsm);
            tsm_node_free(tsm, match_idx);
        }
        else {
            match_idx = empty_idx;
        }
        /* Initialise node table entry */
        tsm_node_init_phantom(tsm, match_idx, node_id);
    }
    *node_idx = match_idx;
    return 0;
}

int tsm_node_lookup(struct tsm_context *tsm, const ts_nodeid_t *node_id, uint16_t *node_idx)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(tsm->node_table.nodes); i++) {
        struct tsm_node *node = &tsm->node_table.nodes[i];
        if (node->protect_window.last_idx >= TSM_NODE_SEQNO_CACHE_SIZE) {
            /* empty */
            continue;
        }
        else if (tsm_node_id_equal(node_id, &node->node_id)) {
            /* match */
            *node_idx = i;
            return 0;
        }
    }
    return -ENODEV;
}

int tsm_neighbour_get(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                      tsm_port_id_t port_id, uint16_t *node_idx, uint16_t *neighbour_idx)
{
    __ASSERT(port_id < tsm->port_count, "Unexpected port id: %u", (unsigned)port_id);
    __ASSERT(tsm->node_table.originator_start_idx <= ARRAY_SIZE(tsm->node_table.paths),
             "Neighbour/ originator table management error (originator start idx: %u > %u)",
             (unsigned int)tsm->node_table.originator_start_idx,
             (unsigned int)ARRAY_SIZE(tsm->node_table.paths));

    int ret;
    uint16_t match_idx;
    if ((ret = tsm_node_get(tsm, node_id, &match_idx)) < 0) {
        return ret;
    }

    /* Get neighbour table entry */
    uint16_t nb_idx = UINT16_MAX;
    uint16_t empty_idx = UINT16_MAX; /* just in case we need it */
    struct tsm_node *node = &tsm->node_table.nodes[match_idx];
    for (uint8_t i = 0; i < ARRAY_SIZE(node->paths_refs); i++) {
        uint16_t path_idx = node->paths_refs[i];
        if (path_idx == UINT16_MAX) {
            /* empty */
            if (empty_idx == UINT16_MAX) {
                empty_idx = i;
            }
        }
        else if (path_idx < tsm->node_table.originator_start_idx) {
            /* Reference to neighbour */
            struct tsm_neighbour *neighbour = &tsm->node_table.paths[path_idx].neighbour;
            if (neighbour->port_id == port_id) {
                /* matching port for neighbour */
                nb_idx = path_idx;
                break;
            }
        }
    }
    if (nb_idx == UINT16_MAX) {
        /* Node does not have an associated neighbour table entry */
        if (empty_idx == UINT16_MAX) {
            /* No space left in node table entry for path ref */
            return -EBUSY;
        }
        for (uint16_t i = 0; i <= tsm->node_table.originator_start_idx; i++) {
            if (i == tsm->node_table.originator_start_idx) {
                /* we are at the end of the neighbour table */
                if (i >= ARRAY_SIZE(tsm->node_table.paths)) {
                    /* no space left in combined neighbour/ originator table */
                    return -ENOMEM;
                }
                if (tsm->node_table.paths[i].originator.throughput != UINT8_MAX) {
                    /* No more space */
                    return -ENOMEM;
                }
                /* Resize neighbour table towards originator table */
                tsm->node_table.originator_start_idx = i + 1;
                nb_idx = i;
                break;
            }
            if (tsm->node_table.paths[i].neighbour.heartbeat_period_s == UINT8_MAX) {
                /* unused */
                nb_idx = i;
                break;
            }
        }
        /* Initialise node table entry */
        node->paths_refs[empty_idx] = nb_idx;
        /* Initialise associated neighbour table entry */
        struct tsm_neighbour *neighbour = &tsm->node_table.paths[nb_idx].neighbour;
        neighbour->heartbeat_period_s = 0;
        neighbour->port_id = port_id;
    }

    *node_idx = match_idx;
    *neighbour_idx = nb_idx;
    return 0;
}

int tsm_neighbour_lookup(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                         tsm_port_id_t port_id, uint16_t *node_idx, uint16_t *neighbour_idx)
{
    __ASSERT(port_id < tsm->port_count, "Unexpected port id: %u", (unsigned)port_id);

    int ret;
    uint16_t match_idx = UINT16_MAX;

    ret = tsm_node_lookup(tsm, node_id, &match_idx);
    if (ret != 0) {
        return ret;
    }

    /* Lookup neighbour table entry */
    struct tsm_node *node = &tsm->node_table.nodes[match_idx];
    for (uint8_t i = 0; i < ARRAY_SIZE(node->paths_refs); i++) {
        uint16_t path_idx = node->paths_refs[i];
        if (path_idx < tsm->node_table.originator_start_idx) {
            /* Reference to neighbour */
            struct tsm_neighbour *neighbour = &tsm->node_table.paths[path_idx].neighbour;
            if (neighbour->port_id == port_id) {
                /* Neigbour port matches */
                *node_idx = match_idx;
                *neighbour_idx = path_idx;
                return 0;
            }
        }
    }
    return -ENOLINK;
}

static void tsm_neighbour_update_best_next_hop(struct tsm_context *tsm, uint16_t node_idx,
                                                uint16_t neighbour_idx)
{
    __ASSERT(neighbour_idx < tsm->node_table.originator_start_idx,
             "Unexpected neighbour index: %u", (unsigned int)neighbour_idx);

    /* Update best next hop if necessary */
    struct tsm_node *node = &tsm->node_table.nodes[node_idx];
    uint16_t best_next_hop_path_idx = node->paths_refs[TSM_NODE_PATHS_BEST];

    __ASSERT(best_next_hop_path_idx < ARRAY_SIZE(tsm->node_table.paths),
             "Unexpected best next hop path index: %u", (unsigned int)best_next_hop_path_idx);

    if (best_next_hop_path_idx == neighbour_idx) {
        /* Best next hop is already the requested one */
        return;
    }

    /* Best next hop must be an originator - a neighbour can only be neighbour_idx */
    __ASSERT(best_next_hop_path_idx >= tsm->node_table.originator_start_idx,
             "Unexpected best next hop path index: %u", (unsigned int)best_next_hop_path_idx);

    uint8_t throughput = tsm_neighbour_link_throughput(tsm, neighbour_idx);
    struct tsm_originator *originator = &tsm->node_table.paths[best_next_hop_path_idx].originator;
    uint8_t best_next_hop_throughput = originator->throughput;

    if (throughput > best_next_hop_throughput) {
        /* Exchange current best next hop path idx and neighbour idx in path references */
        for (uint8_t i = TSM_NODE_PATHS_BEST + 1; i < ARRAY_SIZE(node->paths_refs); i++) {
            if (node->paths_refs[i] == neighbour_idx) {
                node->paths_refs[i] = best_next_hop_path_idx;
                node->paths_refs[TSM_NODE_PATHS_BEST] = neighbour_idx;
                break;
            }
        }
        __ASSERT(node->paths_refs[TSM_NODE_PATHS_BEST] == neighbour_idx,
                 "Unexpected best next hop path index: %u - expected: %u",
                 (unsigned int)node->paths_refs[TSM_NODE_PATHS_BEST], (unsigned int)neighbour_idx);
    }
}

int tsm_neighbour_update(struct tsm_context *tsm, tsm_node_seqno_t node_seqno,
                         const ts_nodeid_t *node_id, uint8_t version, uint8_t period_s,
                         uint32_t name_mapping_id, tsm_port_id_t port_id)
{
    int ret;

    /* VERSION CHECK */
    if (version != TSM_VERSION) {
        return 0;
    }
    /* OWN DEVICE CHECK - generally done on statement reception */
    if (tsm_node_id_equal(node_id, tsm_node_id(tsm))) {
        LOG_DBG("Called with own node id - should be checked on statement reception");
        return 0;
    }

    uint16_t node_idx = UINT16_MAX;
    uint16_t neighbour_idx = UINT16_MAX;
    ret = tsm_neighbour_get(tsm, node_id, port_id, &node_idx, &neighbour_idx);
    if (ret != 0) {
        return ret;
    }

    /* Update node protection window */
    (void)tsm_node_protect_window_update(tsm, node_idx, node_seqno);

    /* Update node name mapping info */
    tsm->node_table.nodes[node_idx].name_mapping_id = name_mapping_id;

    /* Update neighbour info */
    struct tsm_neighbour *neighbour = &tsm->node_table.paths[neighbour_idx].neighbour;
    neighbour->port_id = port_id;
    neighbour->heartbeat_period_s = period_s;

    /* Update route */
    tsm_neighbour_update_best_next_hop(tsm, node_idx, neighbour_idx);
    return 0;
}

int tsm_originator_get(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                       uint16_t router_node_idx, uint16_t *node_idx, uint16_t *originator_idx)
{
    __ASSERT(tsm->node_table.originator_start_idx <= ARRAY_SIZE(tsm->node_table.paths),
             "Neighbour/ originator table management error (originator start idx: %u > %u)",
             (unsigned int)tsm->node_table.originator_start_idx,
             (unsigned int)ARRAY_SIZE(tsm->node_table.paths));

    int ret;
    uint16_t match_idx;

    ret = tsm_node_get(tsm, node_id, &match_idx);
    if (ret != 0) {
        return ret;
    }
    __ASSERT(match_idx != router_node_idx,
             "Originator table entry requested for own node as router");

    /* Get originator table entry */
    uint16_t orig_idx = UINT16_MAX;
    uint16_t empty_idx = UINT16_MAX; /* just in case we need it */
    struct tsm_node *node = &tsm->node_table.nodes[match_idx];
    for (uint8_t i = 0; i < ARRAY_SIZE(node->paths_refs); i++) {
        uint16_t path_idx = node->paths_refs[i];
        if (path_idx == UINT16_MAX) {
            /* empty */
            if (empty_idx == UINT16_MAX) {
                empty_idx = i;
            }
        }
        else if (path_idx >= tsm->node_table.originator_start_idx) {
            /* Reference to a originator table entry */
            struct tsm_originator *originator = &tsm->node_table.paths[path_idx].originator;
            if (originator->router_idx == router_node_idx) {
                /* matching router node in originator table element */
                orig_idx = path_idx;
                break;
            }
        }
    }
    if (orig_idx == UINT16_MAX) {
        /* Node does not have an associated originator table entry */
        if (empty_idx == UINT16_MAX) {
            /* No space left in node table entry for path ref */
            return -EBUSY;
        }
        /* Originator table start index may be on path table end if originator table is empty */
        for (uint16_t i = ARRAY_SIZE(tsm->node_table.paths);
             i >= tsm->node_table.originator_start_idx; i--) {
            if ((i < ARRAY_SIZE(tsm->node_table.paths))
                && (tsm->node_table.paths[i].originator.throughput == UINT8_MAX)) {
                /* unused */
                orig_idx = i;
                break;
            }
            if (i == tsm->node_table.originator_start_idx) {
                /* we are at the start of the originator table */
                if (i == 0) {
                    /* no space left in combined neighbour/ originator table */
                    return -ENOMEM;
                }
                if (tsm->node_table.paths[tsm->node_table.originator_start_idx - 1]
                    .neighbour.heartbeat_period_s != UINT8_MAX) {
                    /* No more space */
                    return -ENOMEM;
                }
                /* Resize originator table towards neighbour table */
                tsm->node_table.originator_start_idx = i - 1;
                orig_idx = tsm->node_table.originator_start_idx;
                break;
            }

        }
        /* Initialise node table entry */
        node->paths_refs[empty_idx] = orig_idx;
        /* Initialise associated originator table entry */
        struct tsm_originator *originator = &tsm->node_table.paths[orig_idx].originator;
        originator->router_idx = router_node_idx;
        originator->throughput = 0;
    }

    *node_idx = match_idx;
    *originator_idx = orig_idx;
    return 0;
}

int tsm_originator_lookup(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                          uint16_t *node_idx, uint16_t *originator_idx)
{
    int ret;
    uint16_t match_idx;

    ret = tsm_node_lookup(tsm, node_id, &match_idx);
    if (ret != 0) {
        return ret;
    }

    /* Lookup originator table entry */
    struct tsm_node *node = &tsm->node_table.nodes[match_idx];
    for (uint8_t i = 0; i < ARRAY_SIZE(node->paths_refs); i++) {
        uint16_t path_idx = node->paths_refs[i];
        if (path_idx >= tsm->node_table.originator_start_idx
            && path_idx < ARRAY_SIZE(tsm->node_table.paths)) {
            /* Reference to originator */
            *node_idx = match_idx;
            *originator_idx = path_idx;
            return 0;
        }
    }
    return -ENOLINK;
}

static void tsm_originator_update_best_next_hop(struct tsm_context *tsm, uint16_t node_idx,
                                                uint16_t originator_idx, uint8_t throughput)
{
    __ASSERT((originator_idx >= tsm->node_table.originator_start_idx)
              && (originator_idx < ARRAY_SIZE(tsm->node_table.paths)),
             "Unexpected originator index: %u", (unsigned int)originator_idx);

    struct tsm_originator *originator = &tsm->node_table.paths[originator_idx].originator;

    /* Update throughput just in case it changed or the originator entry was just created */
    originator->throughput = throughput;

    /* Update best next hop if necessary */
    struct tsm_node *node = &tsm->node_table.nodes[node_idx];
    uint16_t best_next_hop_path_idx = node->paths_refs[TSM_NODE_PATHS_BEST];
    __ASSERT(best_next_hop_path_idx < ARRAY_SIZE(tsm->node_table.paths),
             "Unexpected best next hop path index: %u", (unsigned int)best_next_hop_path_idx);

    if (best_next_hop_path_idx == originator_idx) {
        /* Best next hop is already the requested one */
        return;
    }
    uint8_t best_next_hop_throughput;
    if (best_next_hop_path_idx < tsm->node_table.originator_start_idx) {
        /* This is a neighbour entry, current best next hop is the neighbour node itself */
        best_next_hop_throughput = tsm_neighbour_link_throughput(tsm, best_next_hop_path_idx);
    }
    else {
        /* This is an originator entry */
        struct tsm_originator *originator = &tsm->node_table.paths[best_next_hop_path_idx].originator;
        best_next_hop_throughput = originator->throughput;
    }
    if (throughput > best_next_hop_throughput) {
        /* Exchange current best next hop path idx and originator idx in path references */
        for (uint8_t i = TSM_NODE_PATHS_BEST + 1; i < ARRAY_SIZE(node->paths_refs); i++) {
            if (node->paths_refs[i] == originator_idx) {
                node->paths_refs[i] = best_next_hop_path_idx;
                node->paths_refs[TSM_NODE_PATHS_BEST] = originator_idx;
                break;
            }
        }
        __ASSERT(node->paths_refs[TSM_NODE_PATHS_BEST] == originator_idx,
                 "Unexpected best next hop path index: %u - expected: %u",
                 (unsigned int)node->paths_refs[TSM_NODE_PATHS_BEST], (unsigned int)originator_idx);
    }
}

int tsm_originator_update(struct tsm_context *tsm, tsm_node_seqno_t node_seqno,
                          const ts_nodeid_t *node_id, uint8_t version, ts_time_ms_t age_ms,
                          uint32_t name_mapping_id, const ts_nodeid_t *router_node_id,
                          uint8_t throughput, tsm_port_id_t port_id)
{
    int ret;
    uint16_t router_node_idx;
    uint16_t router_neighbour_idx;

    /* VERSION CHECK */
    if (version != TSM_VERSION) {
        /* Version does not fit - silently drop */
        LOG_DBG("Unexpected originator statement - version is unknown");
        return 0;
    }
    /* OWN DEVICE CHECK - generally done on statement reception */
    if (tsm_node_id_equal(node_id, tsm_node_id(tsm))) {
        LOG_DBG("Unexpected originator statement - originator is own node");
        return 0;
    }
    /* OWN MESSAGE CHECK */
    if (tsm_node_id_equal(router_node_id, tsm_node_id(tsm))) {
        /* Dont't process our own statements - silently drop */
        LOG_DBG("Unexected originator statement - router is own node");
        return 0;
    }
    /* ORIGINATOR ROUTER CHECK */
    if (tsm_node_id_equal(router_node_id, node_id)) {
        /* Dont't process invalid originator statements where the originator is the router */
        LOG_DBG("Invalid originator statement - originator equals router");
        return 0;
    }
    /* ROUTER NODE CHECK */
    ret = tsm_neighbour_get(tsm, router_node_id, port_id, &router_node_idx, &router_neighbour_idx);
    if (ret != 0) {
        /* The router neighbour entry is not available and can not be established */
        return ret;
    }

    uint16_t node_idx = UINT16_MAX;
    uint16_t originator_idx = UINT16_MAX;
    ret = tsm_originator_get(tsm, node_id, router_node_idx, &node_idx, &originator_idx);
    if (ret == -EBUSY) {
        /* originator node is in node table but no space in path reference table */
    }
    else if (ret == -ENOMEM) {
        /* originator node is in node table but no space left in originator table. */
    }
    else if (ret != 0) {
        return ret;
    }

    /* PROTECTION WINDOW OOR CHECK */
    ret = tsm_node_protect_window_update(tsm, node_idx, node_seqno);
    if (ret == -EINVAL) {
        /* Node sequence number is out of protection window - silently drop */
        return 0;
    }
    /* LOOP AND BEST PATH CHECK */
    if (ret == -EALREADY) {
        /* Node sequence number was already seen */
        uint16_t best_next_hop_idx;
        uint8_t best_next_hop_throughput;
        ret = tsm_node_best_next_hop(tsm, node_idx, &best_next_hop_idx, &best_next_hop_throughput);
        if ((ret == 0) && (best_next_hop_throughput >= throughput)) {
            /* we already have a better or same quality router */
            return 0;
        }
    }

    /* Update internal stats */
    tsm->node_table.nodes[router_node_idx].protect_window.last_seen_time = ts_time_ms();
    /* Assure node's time stamp is updated even if the sequence number was already received */
    tsm->node_table.nodes[node_idx].protect_window.last_seen_time = ts_time_ms();
    tsm->node_table.nodes[node_idx].name_mapping_id = name_mapping_id;

    /* Calculate throughput */
    /* LINK TRANSMISSION RATE LIMITATION */
    uint8_t link_throughput = tsm_neighbour_link_throughput(tsm, router_neighbour_idx);
    if (link_throughput < throughput) {
        throughput = link_throughput;
    }
    /* LINK HALF DUPLEX LIMITATION */
    /** @todo calculate throughput with half duplex limitation */
    /* HOP PENALTY */
    if ((node_idx != router_node_idx) && (throughput > 1)) {
        throughput -= 1;
    }

    /* update route */
    tsm_originator_update_best_next_hop(tsm, node_idx, originator_idx, throughput);
    return 0;
}

int tsm_node_seqno(struct tsm_context *tsm,  uint16_t node_idx, tsm_node_seqno_t *node_seqno)
{
    __ASSERT(node_idx < ARRAY_SIZE(tsm->node_table.nodes),
             "Unexpected node index: %u", (unsigned)node_idx);

    struct tsm_node *node = &tsm->node_table.nodes[node_idx];

    if (node->protect_window.last_idx >= TSM_NODE_SEQNO_CACHE_SIZE) {
        /* No (valid) sequence number in protection window seqno chache */
        return -ENAVAIL;
    }
    if (ts_time_ms_delta(node->protect_window.last_seen_time) >= TSM_NODE_SEQNO_MAX_AGE_S * 1000) {
        /* The latest sequence number is out-dated */
        return -ETIMEDOUT;
    }
    tsm_node_seqno_t seqno = node->protect_window.node_seqno_cache[node->protect_window.last_idx];
    if (seqno == TSM_NODE_SEQNO_INVALID) {
        /* Sequence number not yet set */
        return -EINVAL;
    }
    *node_seqno = seqno;
    return 0;
}

int tsm_node_protect_window_update(struct tsm_context *tsm,  uint16_t node_idx,
                                   tsm_node_seqno_t node_seqno)
{
    __ASSERT(node_seqno <= TSM_NODE_SEQNO_MAX, "Invalid sequence number: %u", (unsigned)node_seqno);
    __ASSERT(node_idx < ARRAY_SIZE(tsm->node_table.nodes), "Invalid node index: %u",
             (unsigned)node_idx);

    tsm_node_seqno_t latest_seqno;
    int ret = tsm_node_seqno(tsm, node_idx, &latest_seqno);

    /* Only update if protection window allows */
    struct tsm_node *node = &tsm->node_table.nodes[node_idx];
    if (ret == 0) {
        /* the latest sequence number is actual - check protection window for expected range */
        __ASSERT(latest_seqno <= TSM_NODE_SEQNO_MAX, "Invalid latest sequence number: %u",
                 (unsigned)latest_seqno);
        tsm_node_seqno_t high_seqno;
        tsm_node_seqno_t low_seqno;
        if (latest_seqno >  TSM_NODE_SEQNO_MAX - TSM_NODE_SEQNO_EXPECTED_RANGE) {
            /* high mark rollover */
            high_seqno = TSM_NODE_SEQNO_MAX - latest_seqno;
            high_seqno = TSM_NODE_SEQNO_EXPECTED_RANGE - high_seqno;
        }
        else {
            high_seqno = latest_seqno + TSM_NODE_SEQNO_EXPECTED_RANGE;
        }
        if (latest_seqno < TSM_NODE_SEQNO_EXPECTED_RANGE) {
            /* low mark roll over */
            low_seqno = TSM_NODE_SEQNO_EXPECTED_RANGE - latest_seqno;
            low_seqno = TSM_NODE_SEQNO_MAX - low_seqno;
        }
        else {
            low_seqno = latest_seqno - TSM_NODE_SEQNO_EXPECTED_RANGE;
        }
        if (low_seqno < high_seqno) {
            if ((node_seqno < low_seqno) || (node_seqno > high_seqno)) {
                return -EINVAL;
            }
        }
        else {
            if ((node_seqno > high_seqno) && (node_seqno < low_seqno)) {
                return -EINVAL;
            }
        }
        /* check whether we received this sequence number already */
        for (uint8_t i = 0; i < ARRAY_SIZE(node->protect_window.node_seqno_cache); i++) {
            if (node->protect_window.node_seqno_cache[i] == node_seqno) {
                /* cache already contains the sequence number */
                return -EALREADY;
            }
        }
    }
    else if (ret == -ETIMEDOUT) {
        /* protection window is out-dated - clear node sequence number cache */
        for (uint8_t i = 0; i < ARRAY_SIZE(node->protect_window.node_seqno_cache); i++) {
            node->protect_window.node_seqno_cache[i] = TSM_NODE_SEQNO_INVALID;
        }
    }

    node->protect_window.last_idx++;
    if (node->protect_window.last_idx >= TSM_NODE_SEQNO_CACHE_SIZE) {
        node->protect_window.last_idx = 0;
    }
    node->protect_window.last_seen_time = ts_time_ms();
    node->protect_window.node_seqno_cache[node->protect_window.last_idx] = node_seqno;
    return 0;
}

int tsm_node_protect_window_check(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                                  tsm_node_seqno_t node_seqno, uint16_t *node_idx)
{
    int ret;
    uint16_t idx;

    ret = tsm_node_get(tsm, node_id, &idx);
    if (ret != 0) {
        return ret;
    }

    /* update protection window */
    ret = tsm_node_protect_window_update(tsm, idx, node_seqno);

    if (ret == 0) {
        /* return also node index just in case it is needed by the caller */
        *node_idx = idx;
    }
    return ret;
}

int tsm_node_name_mapping_id(struct tsm_context *tsm, uint16_t node_idx,
                             tsm_name_mapping_id_t *name_mapping_id)
{
    __ASSERT(node_idx < ARRAY_SIZE(tsm->node_table.nodes),
             "Unexpected node index: %u", (unsigned)node_idx);

    struct tsm_node *node = &tsm->node_table.nodes[node_idx];

    if (node->name_mapping_id == TSM_NODE_NAME_MAPPING_ID_INVALID) {
        /* No (valid) name mapping id */
        return -ENAVAIL;
    }
    *name_mapping_id = node->name_mapping_id;
    return 0;
}

/* helper functions */

bool tsm_node_id_equal(const ts_nodeid_t *node_id_a,
                        const ts_nodeid_t *node_id_b)
{
    if (node_id_a == node_id_b) {
        return true;
    }
    return *node_id_a == *node_id_b;
}

bool tsm_port_same(const struct tsm_port *port_a, const struct tsm_port *port_b)
{
    return (port_a == port_b);
}

int tsm_node_init(struct tsm_context *tsm, struct ts_context *ts,
                  const struct tsm_do_config *do_config, struct tsm_do_data *do_data,
                  const struct tsm_port *ports, uint8_t port_count)
{
    tsm->ts = ts;
    tsm->do_config = do_config;
    tsm->do_data = do_data;
    tsm->ports = ports;
    tsm->port_count = port_count;

    /* Node table */
    for (uint16_t node_idx = 0; node_idx < ARRAY_SIZE(tsm->node_table.nodes); node_idx++) {
        struct tsm_node *node = &tsm->node_table.nodes[node_idx];
        node->protect_window.last_idx = TSM_NODE_SEQNO_CACHE_SIZE;
        node->protect_window.last_seen_time = 0;
    }
    for (uint16_t i = 0; i < ARRAY_SIZE(tsm->node_table.paths); i++) {
        tsm_node_path_free(tsm, i);
    }
    tsm->node_table.originator_start_idx = ARRAY_SIZE(tsm->node_table.paths)/4;

    /* Translation table */
    for (uint16_t i = 0; i < ARRAY_SIZE(tsm->translation_table.translations); i++) {
        tsm->translation_table.translations[i].id = UINT16_MAX;
    }

    return 0;
}
