/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup thingset_mesh_priv
 * @{
 */

#ifndef THINGSET_MESH_PRIV_H_
#define THINGSET_MESH_PRIV_H_

#include "thingset_mesh.h"

#include "../ts_time.h"
#include "../ts_buf.h"
#include "../ts_port.h"

/** ThingSet Mesh protocol version */
#define TSM_VERSION 0

/** Maximum node sequence number before roll over. */
#define TSM_NODE_SEQNO_MAX  23

/** Invalid node sequence number */
#define TSM_NODE_SEQNO_INVALID  0xFFU

/** Invalid node name mapping id */
#define TSM_NODE_NAME_MAPPING_ID_INVALID   UINT32_MAX

/* ThingSet Mesh protocol function codes */
#define TSM_BIN_GET         0x10U
#define TSM_BIN_APPEND      0x11U
#define TSM_BIN_ACTIVATE    TS_MESH_BIN_APPEND
#define TSM_BIN_DELETE      0x12U
#define TSM_BIN_FETCH       0x13U
#define TSM_BIN_UPDATE      0x14U
#define TSM_BIN_RESPONSE    0x15U
#define TSM_BIN_STATEMENT   0x16U

/* ThingSet Mesh protocol function text ids */
#define TSM_TXT_GET         'G'
#define TSM_TXT_APPEND      'A'
#define TSM_TXT_ACTIVATE    TS_MESH_TXT_APPEND
#define TSM_TXT_DELETE      'D'
#define TSM_TXT_FETCH       'F'
#define TSM_TXT_UPDATE      'U'
#define TSM_TXT_RESPONSE    'R'
#define TSM_TXT_STATEMENT   'S'

/* ThingSet Mesh protocol data object ids and names */
#define TSM_DO_HEARTBEAT_ID                 0x08U
#define TSM_DO_HEARTBEAT_NAME               ".tsmHeartbeat"
#define TSM_DO_ORIGINATOR_ID                0x09U
#define TSM_DO_ORIGINATOR_NAME              ".tsmOriginator"
#define TSM_DO_NAME_ID                      0x17U
#define TSM_DO_NAME_NAME                    ".name"
#define TSM_DO_HEARTBEAT_VERSION_ID         0x8000U
#define TSM_DO_HEARTBEAT_VERSION_NAME       ".tsmHeartbeatVersion"
#define TSM_DO_HEARTBEAT_PERIOD_ID          0x8001U
#define TSM_DO_HEARTBEAT_PERIOD_NAME        ".tsmHeartbeatPeriod_s"
#define TSM_DO_HEARTBEAT_NAME_MAPPING_ID    0x8002U
#define TSM_DO_HEARTBEAT_NAME_MAPPING_NAME  ".tsmHeartbeatNameMappingID"
#define TSM_DO_ORIGINATOR_VERSION_ID        0x8003U
#define TSM_DO_ORIGINATOR_VERSION_NAME      ".tsmOriginatorVersion"
#define TSM_DO_ORIGINATOR_AGE_ID            0x8004U
#define TSM_DO_ORIGINATOR_AGE_NAME          ".tsmOriginatorAge_ms"
#define TSM_DO_ORIGINATOR_NAME_MAPPING_ID   0x8005U
#define TSM_DO_ORIGINATOR_NAME_MAPPING_NAME ".tsmOriginatorNameMappingID"
#define TSM_DO_ORIGINATOR_ROUTER_NODE_ID    0x8006U
#define TSM_DO_ORIGINATOR_ROUTER_NODE_NAME  ".tsmOriginatorRouterNodeID"
#define TSM_DO_ORIGINATOR_THROUGHPUT_ID     0x8007U
#define TSM_DO_ORIGINATOR_THROUGHPUT_NAME   ".tsmOriginatorThroughput"

/**
 * @brief Define heartbeat statement data objects.
 *
 * The heartbeat data objects have to be part of the node's ThingSet data objects.
 *
 * @param tsm_do_config_name Name of immutable data for Thingset Mesh protocol data objects.
 * @param tsm_do_data_name Name of mutable data for Thingset Mesh protocol data objects.
 */
#define TSM_DO_HEARTBEAT(tsm_do_config_name, tsm_do_data_name)                              \
    TS_GROUP(TSM_DO_HEARTBEAT_ID, TSM_DO_HEARTBEAT_NAME, TS_NO_CALLBACK, ID_ROOT),          \
    TS_ITEM_UINT16(TSM_DO_HEARTBEAT_VERSION_ID, TSM_DO_HEARTBEAT_VERSION_NAME,              \
                   &tsm_do_config_name.version, TSM_DO_HEARTBEAT_ID,                        \
                   TS_ANY_R, SUBSET_REPORT),                                                \
    TS_ITEM_UINT16(TSM_DO_HEARTBEAT_PERIOD_ID, TSM_DO_HEARTBEAT_PERIOD_NAME,                \
                   &tsm_do_data_name.node_heartbeat_period_s, TSM_DO_HEARTBEAT_ID,          \
                   TS_ANY_R, SUBSET_REPORT),                                                \
    TS_ITEM_UINT16(TSM_DO_HEARTBEAT_NAME_MAPPING_ID, TSM_DO_HEARTBEAT_NAME_MAPPING_NAME,    \
                   &tsm_do_data_name.node_name_mapping_id, TSM_DO_HEARTBEAT_ID,             \
                   TS_ANY_R, SUBSET_REPORT)

/**
 * @brief Define originator statement data objects.
 *
 * The originator data objects have to be part of the node's ThingSet data objects.
 *
 * @param tsm_do_config_name Name of immutable data for Thingset Mesh protocol data objects.
 * @param tsm_do_data_name Name of mutable data for Thingset Mesh protocol data objects.
 */
#define TSM_DO_ORIGINATOR(tsm_do_config_name, tsm_do_data_name)                             \
    TS_GROUP(TSM_DO_ORIGINATOR_ID, TSM_DO_ORIGINATOR_NAME, TS_NO_CALLBACK, ID_ROOT),        \
    TS_ITEM_UINT16(TSM_DO_ORIGINATOR_VERSION_ID, TSM_DO_ORIGINATOR_VERSION_NAME,            \
                   &tsm_do_config_name.version, TSM_DO_ORIGINATOR_ID,                       \
                   TS_ANY_R, SUBSET_REPORT),                                                \
    TS_ITEM_UINT32(TSM_DO_ORIGINATOR_AGE_ID, TSM_DO_ORIGINATOR_NAME,                        \
                   &tsm_do_data_name.originator_age_ms, TSM_DO_ORIGINATOR_ID,               \
                   TS_ANY_R, SUBSET_REPORT),                                                \
    TS_ITEM_UINT16(TSM_DO_ORIGINATOR_NAME_MAPPING_ID, TSM_DO_ORIGINATOR_NAME_MAPPING_NAME,  \
                   &tsm_do_data_name.originator_node_name_mapping_id, TSM_DO_ORIGINATOR_ID, \
                   TS_ANY_R, SUBSET_REPORT),                                                \
    TS_ITEM_UINT64(TSM_DO_ORIGINATOR_ROUTER_NODE_ID, TSM_DO_ORIGINATOR_ROUTER_NODE_NAME,    \
                   &tsm_do_config_name.node_id, TSM_DO_ORIGINATOR_ID,                       \
                   TS_ANY_R, SUBSET_REPORT),                                                \
    TS_ITEM_UINT16(TSM_DO_ORIGINATOR_THROUGHPUT_ID, TSM_DO_ORIGINATOR_THROUGHPUT_NAME,      \
                   &tsm_do_data_name.originator_throughput, TSM_DO_ORIGINATOR_ID,           \
                   TS_ANY_R, SUBSET_REPORT)

/**
 * @brief ThingSet Mesh node sequence number type.
 */
typedef uint8_t tsm_node_seqno_t;

/**
 * @brief Maximum node sequence number before roll over.
 */
#define TSM_NODE_SEQNO_MAX  23

/**
 * @brief Name mapping identifier type.
 */
typedef uint32_t tsm_name_mapping_id_t;

/**
 * @brief ThingSet Mesh port identifier type.
 *
 * Mesh port identifiers are specifc to a mesh node.
 */
typedef uint8_t tsm_port_id_t;

/**
 * @brief A ThingSet Mesh communication port.
 *
 * Runtime port structure (in ROM) per port instance.
 */
struct tsm_port {
    /**
     * @brief ThingSet port
     *
     * @note Shall be first in struct to allow to use tsm_port and ts_port interchangeable.
     */
    struct ts_port port;

    /**
     * @brief Get transmission throughput.
     *
     * @return Throughput in data rate range.
     */
    uint8_t (*transmit_throughput)(void);
};

/**
 * @brief Protection Window
 */
struct tsm_protect_window {
    /** @brief Cache of node sequence numbers. */
    tsm_node_seqno_t node_seqno_cache[TSM_NODE_SEQNO_CACHE_SIZE];
    /** @brief Index of latest sequence number inserted. */
    uint8_t last_idx;
    /** @brief Time the latest sequence number was inserted into the caches. */
    ts_time_ms_t last_seen_time;
};

/**
 * @brief Neighbour table element.
 */
struct tsm_neighbour {
    /**
     * @brief Period configuration of last heartbeat statement received from neighbour.
     *
     * @note Keep first in struct. 0xFFU denotes empty element.
     */
    uint8_t heartbeat_period_s;
    /** @brief Id of port the neighbour was seen */
    tsm_port_id_t port_id;
};

/**
 * @brief Originator table element.
 */
struct tsm_originator {
    /**
     * @brief Throughput by the router towards originator.
     *
     * @note Keep first in struct. 0xFFU denotes empty element.
     */
    uint8_t throughput;
    /** @brief Index of of best next hop neighbour in node table */
    uint16_t router_idx;
};

/** @brief Index of best next hop reference within node path references */
#define TSM_NODE_PATHS_BEST 0

/**
 * @brief Node table element.
 *
 * Node information of other node known to this node.
 *
 * An empty element has no node sequence numbers stored in the protection window. This can be
 * detected by the last_idx of the cache is out of the the sequence number cache range.
 */
struct tsm_node {
    /** @brief Node Id of neighbour or originator or phantom */
    ts_nodeid_t node_id;
    /** @brief Name mapping id */
    tsm_name_mapping_id_t name_mapping_id;
    /** @brief Protection window context */
    struct tsm_protect_window protect_window;
    /**
     * @brief List of references to neighbour/ originator table entries.
     *
     * Path reference with index TSM_NODE_PATHS_BEST references the best next hop node.
     */
    uint16_t paths_refs[TSM_NODE_PATHS_MAX];
};

/**
 * @brief Node table.
 */
struct tsm_node_table {
    /**
     * @brief Node table elements.
     */
    struct tsm_node nodes[TSM_NODE_COUNT];

    /**
     * @brief Node path information.
     *
     * Neighbour table starts at index 0. Table fill starts from first element.
     * Originator table starts at originator_start_idx. Table fill starts from last element.
     */
    union {
        struct tsm_neighbour neighbour;
        struct tsm_originator originator;
    } paths[TSM_NODE_COUNT];

    /** @brief Starting index of orginator table within paths array. */
    uint16_t originator_start_idx;
};

/**
 * @brief Translation table element.
 */
struct tsm_translation {
    /** Node specific identifier. */
    uint16_t id;
    /**
     * @brief Index of node(s) in node table
     *
     * Index UINT8_MAX is taken for empty.
     */
    uint8_t node_idx[8];
    /**
     * @brief Node specific name.
     *
     * If we got the name in our .name object this will point to there.
     * Otherwise space for name is allocated from a predefined buffer.
     */
    const char *name;
};

/**
 * @brief Translation table.
 */
struct tsm_translation_table {
    /**
     * @brief Translation table elements.
     */
    struct tsm_translation translations[TSM_TRANSLATION_COUNT];
};

/**
 * @brief Mesh ports info table element.
 */
struct tsm_port_info {
    /**
     * @brief Index into node table of last node announced.
     *
     * Index into node table to last node that was announced at this
     * port.
     */
    uint16_t last_node_idx;
};

/**
 * @brief Immutable data for Thingset Mesh protocol data objects.
 *
 * The structure holds the constant data associated to the ThingSet Mesh protocol that is part of
 * the ThingSet data objects of a node.
 */
struct tsm_do_config {
    /** @brief ThingSet Mesh protocol version. */
    uint16_t version;
    /** @brief ThingSet Mesh node identifier */
    ts_nodeid_t node_id;
};

/**
 * @brief Mutable data for Thingset Mesh protocol data objects.
 *
 * The structure holds the mutable data associated to the ThingSet Mesh protocol that is part of
 * the ThingSet data objects of a node.
 */
struct tsm_do_data {
    /* Data used for heartbeat statement and configuration */
    /** @brief Node sequence number. */
    tsm_node_seqno_t node_seqno;
    /** @brief Node heartbeat refresh interval. */
    uint16_t node_heartbeat_period_s;
    /** @brief Node name mapping identifier. */
    uint16_t node_name_mapping_id;

    /* Data used for current/ next originator statement */
    /** @brief Current originator statement age value */
    uint32_t originator_age_ms;
    /** @brief Current originator statement name mapping identifier value. */
    uint16_t originator_node_name_mapping_id;
    /** @brief Current originator statement throughput value */
    uint16_t originator_throughput;
};

/**
 * @brief A ThingSet Mesh context.
 *
 * A ThingSet Mesh context handles ThingSet Mesh messaging for a node.
 */
struct tsm_context {
    /**
     * @brief ThingSet context of the mesh node.
     */
    struct ts_context *ts;

    /** @brief Immutable data for Thingset Mesh protocol data objects. */
    const struct tsm_do_config *do_config;

    /** @brief Mutable data for Thingset Mesh protocol data objects. */
    struct tsm_do_data *do_data;

    /**
     * @brief Ports table.
     *
     * The ports this mesh instance has access to.
     */
    const struct tsm_port *ports;

    /**
     * @brief Ports info table.
     */
    struct tsm_port_info *ports_info;

    /**
     * @brief Number of ports this mesh instance has access to.
     */
    tsm_port_id_t port_count;

    /**
     * @brief Node table.
     */
    struct tsm_node_table node_table;

    /**
     * @brief Translation table.
     */
    struct tsm_translation_table translation_table;
};

/*
 * ThingSet Mesh context handling
 * ------------------------------
 */

/**
 * @brief Get the node identifier of this mesh node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @return Pointer to node identifier
 */
const ts_nodeid_t *tsm_node_id(struct tsm_context* tsm);

/**
 * @brief Get the node sequence number of this mesh node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @return sequence number
 */
tsm_node_seqno_t tsm_seqno(struct tsm_context* tsm);

/**
 * @brief Get the port of this mesh node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] port_idx Index into ports table.
 * @param[out] port Pointer to return value to be filled with port address.
 * @returns 0 on success, <0 otherwise.
 */
int tsm_port_get(struct tsm_context* tsm, tsm_port_id_t port_id, const struct tsm_port** port);

/**
 * @brief Get the heartbeat statement period.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @return period in seconds
 */
uint8_t tsm_heartbeat_period_s(struct tsm_context *tsm);

/*
 * ThingSet Mesh node handling
 * ---------------------------
 */

/**
 * @brief Init node table entry to phantom node.
 *
 * The node shall be initialized with protection window and name mapping id set to invalid.
 * Also no neighbour nor orinator path shall be attached to the node table entry.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_idx Index of node table element.
 * @param[in] node_id Ponter to node id of neighbour/ originator node.
 */
void tsm_node_init_phantom(struct tsm_context *tsm, uint16_t node_idx, const ts_nodeid_t *node_id);

/**
 * @brief Best next hop node of node table entry.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_idx Index of node table element.
 * @param[out] hop_idx Index of best next hop node table element.
 * @param[out] throughput Throughput by the hop towards the node.
 * @returns 0 on success, <0 otherwise.
 */
int tsm_node_best_next_hop(struct tsm_context *tsm, uint16_t node_idx, uint16_t *hop_idx,
                           uint8_t *throughput);

/**
 * @brief Get a node table entry.
 *
 * If the node table entry already exists return the existing one.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of neighbour/ originator node.
 * @param[out] node_idx Index of node table element.
 * @returns 0 on success, <0 otherwise.
 */
int tsm_node_get(struct tsm_context* tsm, const ts_nodeid_t* node_id, uint16_t* node_idx);

/**
 * @brief Search best candidate node for eviction from node table.
 *
 * Best candidate is a node that was not seen for a long time.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @return node_idx of best candidate node for eviction.
 */
uint16_t tsm_node_evict(struct tsm_context *tsm);

/**
 * @brief Free a node table entry.
 *
 * Mark node information and the associated neighbour/ originator path information unused.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_idx Index of node table element.
 */
void tsm_node_free(struct tsm_context* tsm, uint16_t node_idx);

/**
 * @brief Lookup the node table element of a mesh node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of neighbour/ originator node.
 * @param[out] node_idx Index of node table element.
 * @returns 0 on success, <0 otherwise.
 */
int tsm_node_lookup(struct tsm_context* tsm, const ts_nodeid_t* node_id, uint16_t* node_idx);

/*
 * ThingSet Mesh neighbour node handling
 * -------------------------------------
 */

/**
 * @brief Get a free neighbour element in node table.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of the neighbour.
 * @param[in] port_id The port the node is detected on.
 * @param[out] node_idx Index of node table element.
 * @param[out] neighbour_idx Index of neighbour table element.
 * @returns 0 on success, < 0 otherwise.
 * @return -EBUSY if neighbour node is in node table but no space left in node table element's path
 *                reference table.
 * @return -ENOMEM if neighbour node is in node table but no space left in neighbour table.
 */
int tsm_neighbour_get(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                      tsm_port_id_t port_id, uint16_t *node_idx, uint16_t *neighbour_idx);

/**
 * @brief Find a neighbour at given port
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of the neighbour.
 * @param[in] port_id The port the node is detected on.
 * @param[out] node_idx Index of node table element.
 * @param[out] neighbour_idx Index of neighbour table element.
 * @returns 0 on success, < 0 otherwise.
 */
int tsm_neighbour_lookup(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                         tsm_port_id_t port_id, uint16_t *node_idx, uint16_t *neighbour_idx);

/**
 * @brief Update neighbour info.
 *
 * Update the neighbour info within the node table.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_seqno Node sequence number in heartbeat statement of neighbour.
 * @param[in] node_id Ponter to node id of the neighbour.
 * @param[in] version Version of the ThingSet Mesh protocol.
 * @param[in] period_s Period used by neighbour node to increment it's node sequence number.
 * @param[in] name_mapping_id Name mapping id indicated in the heartbeat statement.
 * @param[in] port_id The port the neighbour is detected on.
 * @returns 0 on success, < 0 otherwise.
 */
int tsm_neighbour_update(struct tsm_context *tsm, tsm_node_seqno_t node_seqno,
                         const ts_nodeid_t *node_id, uint8_t version, uint8_t period_s,
                         uint32_t name_mapping_id, tsm_port_id_t port_id);

/*
 * ThingSet Mesh originator node handling
 * --------------------------------------
 */

/**
 * @brief Get a free originator element in node table.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of the originator node.
 * @param[in] router_node_idx Index of node table element of router node (the last hop neighbour).
 * @param[out] node_idx Index of node table element.
 * @param[out] originator_idx Index of originator table element.
 * @returns 0 on success, < 0 otherwise.
 * @return -EBUSY if originator node is in node table but no space left in node table element's path
 *                reference table.
 * @return -ENOMEM if originator node is in node table but no space left in originator table.
 */
int tsm_originator_get(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                       uint16_t router_node_idx, uint16_t *node_idx, uint16_t *originator_idx);

/**
 * @brief Find an originator.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of the originator.
 * @param[out] node_idx Index of node table element.
 * @param[out] originator_idx Index of originator table element.
 * @returns 0 on success, < 0 otherwise.
 */
int tsm_originator_lookup(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                          uint16_t *node_idx, uint16_t *originator_idx);

/**
 * @brief Update originator info.
 *
 * Update the originator info within the node table.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_seqno Originator node sequence number given in the originator statement.
 * @param[in] node_id Ponter to node id of the originator node.
 * @param[in] version Version of the ThingSet Mesh protocol.
 * @param[in] age_ms Milliseconds since originator node was last seen by router.
 * @param[in] name_mapping_id Name mapping id of originator node.
 * @param[in] router_node_id Pointer to node id of router node (a neighbour).
 * @param[in] port_id The port the originator is detected on.
 * @returns 0 on success, < 0 otherwise.
 */
int tsm_originator_update(struct tsm_context *tsm, tsm_node_seqno_t node_seqno,
                          const ts_nodeid_t *node_id, uint8_t version, ts_time_ms_t age_ms,
                          uint32_t name_mapping_id, const ts_nodeid_t *router_node_id,
                          uint8_t throughput, tsm_port_id_t port_id);

/**
 * @brief Receive ThingSet Mesh message.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[out] port_id The port the message was received on.
 * @param[out] hop_node_id The last hop that transmitted this message.
 * @param[out] message Pointer to the message buffer.
 * @param[in] callback_on_received If callback is NULL receive returns on the
 *                             next message. If the callback is set receive
 *                             immediatedly returns and the callback is
 *                             called on the reception of the message.
 *                             Beware even in this case the callback may be
 *                             called before the receive function returns.
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 * @returns 0 on success, <0 otherwise
 */
int tsm_receive(struct tsm_context *tsm, tsm_port_id_t *port_id, ts_nodeid_t **hop_node_id,
                struct ts_buf **message,
                int (*callback_on_rx)(struct tsm_context *tsm, tsm_port_id_t *port_id,
                                      ts_nodeid_t **hop_node_id, struct ts_buf **message),
                ts_time_ms_t timeout_ms);

/**
 * @brief Transmit allocated message.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node. Must be the same as used for
 *                 message allocation.
 * @param[in] port_id The port the message shall be transmitted on.
 * @param[in] hop_node_id The hop to transmit this message to.
 * @param[in] message Pointer to the message. Must be the same as returned by
 *                    message allocation.
 * @param[in] callback_on_tx If callback is NULL transmit returns on the
 *                         next message. If the callback is set transmit
 *                         immediatedly returns and the callback is called
 *                         after the transmission of the message. Beware
 *                         even in this case the callback may be called
 *                         before the transmit function returns.
 * @param[in] timeout_ms maximum time to wait in milliseconds.
 * @returns 0 on success, <0 otherwise
 */
int tsm_transmit(struct tsm_context *tsm, tsm_port_id_t *port_id, ts_nodeid_t *hop_node_id,
                 struct ts_buf *message,
                 int (*callback_on_tx)(struct tsm_context *tsm,
                                       tsm_port_id_t *port_id,
                                       ts_nodeid_t *hop_node_id,
                                       struct ts_buf *message),
                 ts_time_ms_t timeout_ms);


/*
 * Protection Window handling
 * --------------------------
 */

/**
 * @brief Get the latest known node sequence number of a node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_idx Index of node table element.
 * @param[out] node_seqno Latest known node sequence number.
 * @returns 0 on success, <0 if there is no valid node sequence number.
 */
int tsm_node_seqno(struct tsm_context *tsm, uint16_t node_idx, tsm_node_seqno_t *node_seqno);

/**
 * @brief Update protection window for node.
 *
 * Only update if protection window allows to update.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_idx Index of node table element.
 * @param[in] node_seqno Node sequence number.
 * @returns 0 on success, <0 if update was not possible.
 * @return -EALREADY if node sequence number was already seen.
 * @return -EINVAL if node sequence number is out of protection window.
 */
int tsm_node_protect_window_update(struct tsm_context *tsm,  uint16_t node_idx,
                                   tsm_node_seqno_t node_seqno);

/**
 * @brief Check protection window.
 *
 * To distinguish valid ThingSet Mesh messages from out of date/ delayed/ doubled messages a
 * sliding window pinned to the last received valid node sequence number is used. The sliding window
 * is called the *Protection Window*.
 *
 * A valid node sequence number has to be received within the last TSM_NODE_SEQNO_MAX_AGE_S seconds.
 *
 * If a valid last node sequence number exists, a valid message has to have a node sequence number
 * that is new compared to the chached sequence numbers of messages received before. Additional it
 * has to be  greater than the last valid node sequence number reduced by
 * TSM_NODE_SEQNO_EXPECTED_RANGE and be  less than the last valid node sequence number advanced by
 * TSM_NODE_SEQNO_EXPECTED_RANGE taking into account roll over.
 *
 * If the last valid sequence number gets out-dated the protection window is de-activated and all
 * cached node sequence numbers are deleted. Any new node sequence number will be accepted
 * afterwards and the protection window activated again.
 *
 * The node sequence number cache size is limited to TSM_NODE_SEQNO_CACHE_SIZE entries per node. If
 * a node sends a lot of messages in a short time multiple copies of a message may still not be
 * detectable.
 *
 * @note If the check passes the protection window is updated by the new node sequence number.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_id Ponter to node id of the message source node.
 * @param[in] node_seqno Source node sequence number.
 * @param[out] node_idx Index of node table element.
 * @returns 0 on success, <0 otherwise.
 */
int tsm_node_protect_window_check(struct tsm_context *tsm, const ts_nodeid_t *node_id,
                                  tsm_node_seqno_t node_seqno, uint16_t *node_idx);

/*
 * Name mapping handling
 * ---------------------
 */

/**
 * @brief Get the latest known name mapping id of a node.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] node_idx Index of node table element.
 * @param[out] name_mapping_id Latest known name mapping id.
 * @returns 0 on success, <0 if there is no valid node sequence number.
 */
int tsm_node_name_mapping_id(struct tsm_context *tsm, uint16_t node_idx,
                             tsm_name_mapping_id_t *name_mapping_id);

/*
 * Message creation
 * ----------------
 */

/**
 * @brief Generate ThingSet Mesh statement message in JSON format.
 *
 * Generate ThingSet Mesh statement message in JSON format based on pointer to group or subset.
 *
 * @note This is the fastest method to generate a statement as it does not require to search through
 *       the entire data objects array.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] object Group or subset object specifying the items to be published
 * @param[out] message Pointer to message buffer.
 * @returns 0 on success, <0 otherwise
 */
int tsm_txt_statement(struct tsm_context *tsm, struct ts_data_object *object,
                      struct ts_buf **message);

/**
 * @brief Generate ThingSet Mesh statement message in CBOR format.
 *
 * Generate ThingSet Mesh statement message in CBOR format based on pointer to group or subset.
 *
 * @note This is the fastest method to generate a statement as it does not require to search through
 *       the entire data objects array.
 *
 * @param[in] tsm Pointer to ThingSet Mesh context of this node.
 * @param[in] object Group or subset object specifying the items to be published
 * @param[out] message Pointer to message buffer.
 * @returns 0 on success, <0 otherwise
 */
int tsm_bin_statement(struct tsm_context *tsm, struct ts_data_object *object,
                      struct ts_buf **message);

/*
 * Helpers
 * -------
 */

/**
 * @brief Check for node id equal.
 *
 * @param node_id_a Pointer to node id of a node.
 * @param node_id_b Pointer to node id of a node.
 * @returns true on equal, false otherwise.
 */
bool tsm_node_id_equal(const ts_nodeid_t *node_id_a, const ts_nodeid_t *node_id_b);

/**
 * @brief Check for same port.
 *
 * @param port_a Pointer to port.
 * @param port_b Pointer to port.
 * @returns true on same, false otherwise.
 */
bool tsm_port_same(const struct tsm_port *port_a, const struct tsm_port *port_b);

/**
 * @brief Convert throughput in bytes per second to data rate range.
 *
 * @param[in] throughput_bps Throughput in bytes per second
 * @return data rate range
 */
uint8_t tsm_throughput_bps_to_data_rate_range(uint32_t throughput_bps);

#endif /* THINGSET_MESH_PRIV_H_ */

/** @} thingset_mesh_priv */
