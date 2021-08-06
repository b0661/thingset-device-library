/*
 * Copyright (c) 2021 Bobby Noelte.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TSM_CONFIG_H_
#define TSM_CONFIG_H_

/**
 * @def TS_MESH
 * @brief Enable ThingSet Mesh networking support.
 */
#if !defined(TS_MESH) && !defined(CONFIG_THINGSET_MESH)
#define TS_MESH 0        // default: no support
#elif !defined(TS_MESH)
#define TS_MESH CONFIG_THINGSET_MESH
#endif

#if TS_MESH

/**
 * @def TSM_BUF_COUNT
 * @brief Number of buffers in the ThingSet Mesh buffer pool.
 */
#if !defined(TSM_BUF_COUNT) && !defined(CONFIG_THINGSET_MESH_BUF_COUNT)
#define TSM_BUF_COUNT 16
#elif !defined(TSM_BUF_COUNT)
#define TSM_BUF_COUNT CONFIG_THINGSET_MESH_BUF_COUNT
#endif

/**
 * @def TSM_BUF_DATA_SIZE
 * @brief Data block size for ThingSet Mesh network messages.
 */
#if !defined(TSM_BUF_DATA_SIZE) \
    && !defined(CONFIG_THINGSET_MESH_BUF_DATA_SIZE)
#define TSM_BUF_DATA_SIZE 1024
#elif !defined(TSM_BUF_DATA_SIZE)
#define TSM_BUF_DATA_SIZE CONFIG_THINGSET_MESH_BUF_DATA_SIZE
#endif

/**
 * @def TSM_NODE_COUNT
 * @brief Number of node entries in the node table.
 */
#if !defined(TSM_NODE_COUNT) \
    && !defined(CONFIG_THINGSET_MESH_NODE_COUNT)
#define TSM_NODE_COUNT 16
#elif !defined(TSM_NODE_COUNT)
#define TSM_NODE_COUNT CONFIG_THINGSET_MESH_NODE_COUNT
#endif

/**
 * @def TSM_NODE_PATHS_MAX
 * @brief Maximum number of routing entries in the device table.
 */
#if !defined(TSM_NODE_PATHS_MAX) \
    && !defined(CONFIG_THINGSET_MESH_NODE_PATHS_MAX)
#define TSM_NODE_PATHS_MAX 2
#elif !defined(TSM_NODE_PATHS_MAX)
#define TSM_NODE_PATHS_MAX CONFIG_THINGSET_MESH_NODE_PATHS_MAX
#endif

/**
 * @def TSM_TRANSLATION_COUNT
 * @brief Number of translation entries in the translation table.
 */
#if !defined(TSM_TRANSLATION_COUNT) \
    && !defined(CONFIG_THINGSET_MESH_TRANSLATION_COUNT)
#define TSM_TRANSLATION_COUNT 16
#elif !defined(TSM_TRANSLATION_COUNT)
#define TSM_TRANSLATION_COUNT CONFIG_THINGSET_MESH_TRANSLATION_COUNT
#endif

/**
 * @def TSM_NODE_SEQNO_EXPECTED_RANGE
 * @brief Expected node sequence number range for valid messages.
 *
 * A valid message has to have a node sequence number that is greater than the last valid node
 * sequence number reduced by TSM_NODE_SEQNO_EXPECTED_RANGE and is less than the last valid node
 * sequence number advanced by TSM_NODE_SEQNO_EXPECTED_RANGE taking into account roll over.
 */
#if !defined(TSM_NODE_SEQNO_EXPECTED_RANGE) \
    && !defined(CONFIG_THINGSET_MESH_NODE_SEQNO_EXPECTED_RANGE)
#define TSM_NODE_SEQNO_EXPECTED_RANGE 10
#elif !defined(TSM_NODE_SEQNO_EXPECTED_RANGE)
#define TSM_NODE_SEQNO_EXPECTED_RANGE CONFIG_THINGSET_MESH_NODE_SEQNO_EXPECTED_RANGE
#endif

/**
 * @def TSM_NODE_SEQNO_MAX_AGE_S
 * @brief Maximum age of a valid message and it's node sequence number.
 *
 * Only if a node's sequence number is younger than TSM_NODE_SEQNO_MAX_AGE_S it is regarded for the
 * protection window.
 */
#if !defined(TSM_NODE_SEQNO_MAX_AGE_S) \
    && !defined(CONFIG_THINGSET_MESH_NODE_SEQNO_MAX_AGE_S)
#define TSM_NODE_SEQNO_MAX_AGE_S 3
#elif !defined(TSM_NODE_SEQNO_MAX_AGE_S)
#define TSM_NODE_SEQNO_EXPECTED_RANGE CONFIG_THINGSET_MESH_NODE_SEQNO_MAX_AGE_S
#endif

/**
 * @def TSM_NODE_SEQNO_CACHE_SIZE
 * @brief Size of cache for node sequence numbers for protection window.
 */
#if !defined(TSM_NODE_SEQNO_CACHE_SIZE) \
    && !defined(CONFIG_THINGSET_MESH_NODE_SEQNO_CACHE_SIZE)
#define TSM_NODE_SEQNO_CACHE_SIZE 8
#elif !defined(TSM_NODE_SEQNO_CACHE_SIZE)
#define TSM_NODE_SEQNO_EXPECTED_RANGE CONFIG_THINGSET_MESH_NODE_SEQNO_CACHE_SIZE
#endif

#endif /* TS_MESH */

#endif /* TSM_CONFIG_H_ */
