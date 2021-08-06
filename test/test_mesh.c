/*
 * Copyright (c) 2021 Bobby Noelte
 * SPDX-License-Identifier: Apache-2.0
 */

/* Exclude from default PlatformIO test build if not configured */
#ifdef CONFIG_THINGSET_MESH

#include "test.h"

#include "../src/mesh/thingset_mesh.h"
#include "../src/mesh/thingset_mesh_priv.h"

/* Mesh test data */
const ts_nodeid_t instance_node_id =  0xCAFFECABULL;
const ts_nodeid_t neighbour_node_id =  0xCAFFE7E1ULL;
const ts_nodeid_t originator_node_id =  0xCAFFE031ULL;

/* ThingSet Mesh context for mesh testing */
struct tsm_context tsm;

/* ThingSet context for mesh testing */
const struct tsm_do_config tsm_ts_do_config = {
    .version = TSM_VERSION,
    .node_id = instance_node_id,
};

struct tsm_do_data tsm_ts_do_data;

struct ts_data_object tsm_ts_data_objects[] = {

    // HEARTBEAT INFORMATION /////////////////////////////////////////////////////
    TSM_DO_HEARTBEAT(tsm_ts_do_config, tsm_ts_do_data),

    // ORIGINATOR INFORMATION ////////////////////////////////////////////////////
    TSM_DO_ORIGINATOR(tsm_ts_do_config, tsm_ts_do_data)
};

struct ts_context tsm_ts = {
    .data_objects = tsm_ts_data_objects,
    .num_objects = ARRAY_SIZE(tsm_ts_data_objects),
};

/* Thingset Mesh ports for mesh testing */
uint8_t mock_port_transmit_throughput_value = 1;
uint8_t mock_port_transmit_throughput(void)
{
    return mock_port_transmit_throughput_value;
}

const struct tsm_port tsm_ports[] = {
    {
        .transmit_throughput = mock_port_transmit_throughput
    },
    {
        .transmit_throughput = mock_port_transmit_throughput
    }
};

/**
 * @brief Test node init
 *
 * This test verifies mesh node context initialisation and access:
 * - tsm_node_init()
 * - tsm_node_id()
 * - tsm_seqno()
 * - tsm_port_get()
 */
void test_mesh_init(void)
{
    int ret;

    ret = tsm_node_init(&tsm, &tsm_ts, &tsm_ts_do_config, &tsm_ts_do_data,
                        &tsm_ports[0], ARRAY_SIZE(tsm_ports));
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_PTR(&tsm_ts, tsm.ts);
    TEST_ASSERT_EQUAL_PTR(&tsm_ts_do_config, tsm.do_config);
    TEST_ASSERT_EQUAL_PTR(&tsm_ts_do_data, tsm.do_data);
    TEST_ASSERT_EQUAL_PTR(&tsm_ports[0], tsm.ports);
    TEST_ASSERT_EQUAL_UINT(ARRAY_SIZE(tsm_ports), tsm.port_count);
    TEST_ASSERT_EQUAL_UINT(TSM_NODE_COUNT, ARRAY_SIZE(tsm.node_table.nodes));
    TEST_ASSERT_EQUAL_UINT(TSM_NODE_COUNT, ARRAY_SIZE(tsm.node_table.paths));
    TEST_ASSERT_EQUAL_UINT(TSM_TRANSLATION_COUNT, ARRAY_SIZE(tsm.translation_table.translations));
    for (uint16_t i = 0; i < TSM_NODE_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT8(TSM_NODE_SEQNO_CACHE_SIZE,
                                tsm.node_table.nodes[i].protect_window.last_idx);
    }
    for (uint16_t i = 0; i < TSM_NODE_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT8(UINT8_MAX, tsm.node_table.paths[i].neighbour.heartbeat_period_s);
        TEST_ASSERT_EQUAL_UINT8(UINT8_MAX, tsm.node_table.paths[i].originator.throughput);
    }
    TEST_ASSERT_LESS_THAN_UINT16(TSM_NODE_COUNT, tsm.node_table.originator_start_idx);
    for (uint16_t i = 0; i < TSM_TRANSLATION_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, tsm.translation_table.translations[i].id);
    }

    /* Check also ThingSet Mesh context access functions providing initialized values */
    TEST_ASSERT_EQUAL_UINT64(instance_node_id, *tsm_node_id(&tsm));
    TEST_ASSERT_EQUAL_UINT8(0, tsm_seqno(&tsm));
    const struct tsm_port *port;
    ret = tsm_port_get(&tsm, ARRAY_SIZE(tsm_ports), &port);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    ret = tsm_port_get(&tsm, ARRAY_SIZE(tsm_ports) - 1, &port);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_PTR(&tsm_ports[ARRAY_SIZE(tsm_ports) - 1], port);
    ret = tsm_port_get(&tsm, 0, &port);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_PTR(&tsm_ports[0], port);
    TEST_ASSERT_EQUAL_UINT64(0, tsm_heartbeat_period_s(&tsm));
}

/**
 * @brief Test mesh node table
 *
 * This test verifies device table usage:
 * - tsm_node_init_phantom() - called by tsm_node_get()
 * - tsm_node_get()
 * - tsm_node_lookup()
 */
void test_mesh_node_table(void)
{
    int ret;
    uint16_t node_idx;

    ret = tsm_node_lookup(&tsm, &neighbour_node_id, &node_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    node_idx = UINT16_MAX;
    ret = tsm_node_get(&tsm, &neighbour_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(0, node_idx);

    node_idx = UINT16_MAX;
    ret = tsm_node_lookup(&tsm, &neighbour_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(0, node_idx);

    node_idx = UINT16_MAX;
    ret = tsm_node_lookup(&tsm, &instance_node_id, &node_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, node_idx);

    /* Get already existing device table entry */
    ret = tsm_node_get(&tsm, &neighbour_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(0, node_idx);

    node_idx = UINT16_MAX;
    ret = tsm_node_get(&tsm, &originator_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(1, node_idx);

    node_idx = UINT16_MAX;
    ret = tsm_node_lookup(&tsm, &neighbour_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(0, node_idx);

    node_idx = UINT16_MAX;
    ret = tsm_node_lookup(&tsm, &originator_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(1, node_idx);

    /* Assure nodes are initialized to phantom state - no seqno, invalid name mapping id */
    tsm_node_seqno_t node_seqno;
    ret = tsm_node_seqno(&tsm, node_idx, &node_seqno);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    tsm_name_mapping_id_t name_mapping_id;
    ret = tsm_node_name_mapping_id(&tsm, node_idx, &name_mapping_id);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* Fake node 0 to be the oldest one */
    tsm.node_table.nodes[0].protect_window.last_seen_time = 0;
    for (uint16_t i = 1; i < ARRAY_SIZE(tsm.node_table.nodes); i++) {
        tsm.node_table.nodes[i].protect_window.last_seen_time = 1;
    }
    node_idx = tsm_node_evict(&tsm);
    TEST_ASSERT_EQUAL_UINT16(0, node_idx);

    /* Free all nodes - silently ignore if node is already freed */
    for (uint16_t i = 1; i < ARRAY_SIZE(tsm.node_table.nodes); i++) {
        tsm_node_free(&tsm, i);
        TEST_ASSERT_EQUAL_UINT16(TSM_NODE_SEQNO_CACHE_SIZE,
                                 tsm.node_table.nodes[i].protect_window.last_idx);
    }

    /* Aquire all nodes */
    ts_nodeid_t node_ids[ARRAY_SIZE(tsm.node_table.nodes)];
    for (uint16_t i = 1; i < ARRAY_SIZE(tsm.node_table.nodes); i++) {
        node_ids[i] = i;
        ret = tsm_node_get(&tsm, &node_ids[i], &node_idx);
        TEST_ASSERT_EQUAL(0, ret);
    }
    /* Additional aquire should not fail */
    ret = tsm_node_get(&tsm, &neighbour_node_id, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
}

/**
 * @brief Test mesh neighbour table
 *
 * This test verifies neighbour table usage:
 * - tsm_neighbour_get()
 * - tsm_neighbour_lookup()
 * - tsm_neighbour_update()
 */
void test_mesh_neighbour_table(void)
{
    int ret;
    ts_time_ms_t current_time_ms;
    tsm_port_id_t port_id = 0;
    tsm_name_mapping_id_t name_mapping_id = 0x12345678U;
    tsm_node_seqno_t node_seqno = 3;
    uint8_t period_s = 5;
    uint16_t neighbour_node_idx;
    uint16_t neighbour_idx;
    tsm_name_mapping_id_t neighbour_name_mapping_id;
    uint16_t hop_idx;
    uint8_t throughput;

    /* Version Check - silent fail */
    ret = tsm_neighbour_update(&tsm, node_seqno, &neighbour_node_id, TSM_VERSION + 1, period_s,
                               name_mapping_id, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    neighbour_node_idx = UINT16_MAX;
    neighbour_idx = UINT16_MAX;
    ret = tsm_neighbour_lookup(&tsm, &neighbour_node_id, port_id, &neighbour_node_idx, &neighbour_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, neighbour_node_idx);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, neighbour_idx);

    /* Own Device Check - silent fail */
    ret = tsm_neighbour_update(&tsm, node_seqno, &instance_node_id, TSM_VERSION, period_s,
                               name_mapping_id, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    neighbour_node_idx = UINT16_MAX;
    neighbour_idx = UINT16_MAX;
    ret = tsm_neighbour_lookup(&tsm, &neighbour_node_id, port_id, &neighbour_node_idx, &neighbour_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, neighbour_node_idx);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, neighbour_idx);

    /* Create new neighbour entry */
    current_time_ms = ts_time_ms();
    ret = tsm_neighbour_update(&tsm, node_seqno, &neighbour_node_id, TSM_VERSION, period_s,
                               name_mapping_id, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    neighbour_node_idx = UINT16_MAX;
    neighbour_idx = UINT16_MAX;
    ret = tsm_neighbour_lookup(&tsm, &neighbour_node_id, port_id, &neighbour_node_idx, &neighbour_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(0, neighbour_node_idx);
    TEST_ASSERT_EQUAL_UINT16(0, neighbour_idx);

    /* check context change */
    tsm_node_seqno_t neighbour_node_seqno = TSM_NODE_SEQNO_INVALID;
    ret = tsm_node_seqno(&tsm, neighbour_node_idx, &neighbour_node_seqno);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT8(node_seqno, neighbour_node_seqno);
    ret = tsm_node_name_mapping_id(&tsm, neighbour_node_idx, &neighbour_name_mapping_id);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT32(name_mapping_id, neighbour_name_mapping_id);
    ret = tsm_node_best_next_hop(&tsm, neighbour_node_idx, &hop_idx, &throughput);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(neighbour_node_idx, hop_idx);
    TEST_ASSERT_EQUAL(1, throughput);
    /* context not covered by access functions */
    TEST_ASSERT_EQUAL_UINT32(current_time_ms,
                             tsm.node_table.nodes[neighbour_node_idx].protect_window.last_seen_time);
    TEST_ASSERT_EQUAL_UINT16(neighbour_idx, tsm.node_table.nodes[neighbour_node_idx].paths_refs[0]);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, tsm.node_table.nodes[neighbour_node_idx].paths_refs[1]);
    TEST_ASSERT_EQUAL_UINT8(period_s,
                            tsm.node_table.paths[neighbour_idx].neighbour.heartbeat_period_s);
    TEST_ASSERT_EQUAL_UINT8(port_id, tsm.node_table.paths[neighbour_idx].neighbour.port_id);

    /* A get on already available node should provide just this node */
    uint16_t node_idx;
    uint16_t path_idx;
    ret = tsm_neighbour_get(&tsm, &neighbour_node_id, port_id, &node_idx, &path_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(neighbour_node_idx, node_idx);
    TEST_ASSERT_EQUAL_UINT16(neighbour_idx, path_idx);
}

/**
 * @brief Test mesh neighbour table
 *
 * This test verifies neighbour table usage:
 * - tsm_originator_get()
 * - tsm_originator_lookup()
 * - tsm_originator_update()
 */
void test_mesh_originator_table(void)
{
    int ret;
    tsm_port_id_t port_id = 0;
    tsm_name_mapping_id_t name_mapping_id = 0x12345678U;
    tsm_node_seqno_t node_seqno = 3;
    uint32_t age_ms = 1000;
    uint16_t originator_node_idx;
    uint16_t originator_idx;
    uint16_t neighbour_node_idx;
    uint16_t neighbour_idx;
    uint16_t hop_idx;
    uint8_t throughput = 1;

    /* Version Check - silent fail */
    ret = tsm_originator_update(&tsm, node_seqno, &originator_node_id, TSM_VERSION + 1,
                                age_ms, name_mapping_id, &neighbour_node_id,
                                throughput, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    originator_node_idx = UINT16_MAX;
    originator_idx = UINT16_MAX;
    ret = tsm_originator_lookup(&tsm, &originator_node_id, &originator_node_idx, &originator_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, originator_node_idx);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, originator_idx);

    /* OWN DEVICE CHECK - silent fail */
    ret = tsm_originator_update(&tsm, node_seqno, tsm_node_id(&tsm), TSM_VERSION,
                                age_ms, name_mapping_id, &neighbour_node_id,
                                throughput, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    ret = tsm_originator_lookup(&tsm, tsm_node_id(&tsm), &originator_node_idx, &originator_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* ORIGINATOR ROUTER CHECK - silent fail */
    ret = tsm_originator_update(&tsm, node_seqno, &originator_node_id, TSM_VERSION,
                                age_ms, name_mapping_id, &originator_node_id,
                                throughput, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    ret = tsm_originator_lookup(&tsm, &originator_node_id, &originator_node_idx, &originator_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    ret = tsm_originator_update(&tsm, node_seqno, &originator_node_id, TSM_VERSION,
                                age_ms, name_mapping_id, &neighbour_node_id,
                                throughput, port_id);
    TEST_ASSERT_EQUAL(0, ret);
    ret = tsm_originator_lookup(&tsm, &originator_node_id, &originator_node_idx, &originator_idx);
    TEST_ASSERT_EQUAL(0, ret);
    ret = tsm_neighbour_lookup(&tsm, &neighbour_node_id, port_id, &neighbour_node_idx,
                               &neighbour_idx);
    TEST_ASSERT_EQUAL(0, ret);
    /* Assure neighbour is set as router for originator */
    TEST_ASSERT_EQUAL_UINT16(neighbour_node_idx,
                             tsm.node_table.paths[originator_idx].originator.router_idx);
    /* This should be now also the bext next hop */
    ret = tsm_node_best_next_hop(&tsm, originator_node_idx, &hop_idx, &throughput);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(neighbour_node_idx, hop_idx);

    /* Remove the neighbour node */
    tsm_node_free(&tsm, neighbour_node_idx);
    /* Assure neighbour is removed from originator path info */
    ret = tsm_node_best_next_hop(&tsm, originator_node_idx, &hop_idx, &throughput);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    ret = tsm_originator_lookup(&tsm, &originator_node_id, &originator_node_idx, &originator_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}

/**
 * @brief Test mesh protection window
 *
 * This test verifies protection window limits:
 * - tsm_node_seqno()
 * - tsm_node_protect_window_update()
 * - tsm_node_protect_window_check()
 */
void test_mesh_protect_window(void)
{
    int ret;
    ts_time_ms_t current_time_ms;
    tsm_port_id_t port_id = 0;
    tsm_name_mapping_id_t name_mapping_id = 0x12345678U;
    tsm_node_seqno_t node_seqno = 3;
    uint8_t period_s = 5;
    uint16_t node_idx;
    uint16_t neighbour_idx;

    /* Create new neighbour entry */
    current_time_ms = ts_time_ms();
    ret = tsm_neighbour_update(&tsm, node_seqno, &neighbour_node_id, TSM_VERSION, period_s,
                               name_mapping_id, port_id);
    TEST_ASSERT_EQUAL(0, ret);

    /* Assure we are starting with the correct values */
    node_idx = UINT16_MAX;
    neighbour_idx = UINT16_MAX;
    ret = tsm_neighbour_lookup(&tsm, &neighbour_node_id, port_id, &node_idx, &neighbour_idx);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT16(0, node_idx);
    TEST_ASSERT_EQUAL_UINT16(0, neighbour_idx);
    tsm_node_seqno_t neighbour_node_seqno = TSM_NODE_SEQNO_INVALID;
    ret = tsm_node_seqno(&tsm, node_idx, &neighbour_node_seqno);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT8(node_seqno, neighbour_node_seqno);
    TEST_ASSERT_EQUAL_UINT32(current_time_ms,
                             tsm.node_table.nodes[node_idx].protect_window.last_seen_time);

    /* New node sequence number is out of range */
    node_seqno += TSM_NODE_SEQNO_EXPECTED_RANGE + 1;
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(TSM_NODE_SEQNO_MAX, node_seqno);
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* New node sequence number is on range border */
    node_seqno -= 1;
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(TSM_NODE_SEQNO_MAX, node_seqno);
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* New node sequence number is in range border but older than before */
    node_seqno -= 1;
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(TSM_NODE_SEQNO_MAX, node_seqno);
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* New node sequence number is on range border but older than before */
    if (node_seqno < TSM_NODE_SEQNO_EXPECTED_RANGE) {
        node_seqno += TSM_NODE_SEQNO_MAX - TSM_NODE_SEQNO_EXPECTED_RANGE;
    }
    else {
        node_seqno -= TSM_NODE_SEQNO_EXPECTED_RANGE;
    }
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(TSM_NODE_SEQNO_MAX, node_seqno);
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* New node sequence number is out of range border and older than before */
    if (node_seqno < TSM_NODE_SEQNO_EXPECTED_RANGE + 1) {
        node_seqno += TSM_NODE_SEQNO_MAX - TSM_NODE_SEQNO_EXPECTED_RANGE - 1;
    }
    else {
        node_seqno -= TSM_NODE_SEQNO_EXPECTED_RANGE + 1;
    }
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(TSM_NODE_SEQNO_MAX, node_seqno);
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* latest sequence number is out of date - try a high value to fake timeout */
    tsm.node_table.nodes[node_idx].protect_window.last_seen_time = ts_time_ms() + UINT32_MAX/2;
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* force to max seqno to get high limit roll over for later */
    node_seqno = TSM_NODE_SEQNO_MAX;
    tsm.node_table.nodes[node_idx].protect_window.last_seen_time = ts_time_ms() + UINT32_MAX/2;
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* New node sequence number is out of range - with high limit rollover */
    node_seqno = TSM_NODE_SEQNO_EXPECTED_RANGE + 1;
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* New node sequence number is on range border - with high limit rollover */
    node_seqno -= 1;
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* Update with same node sequence number as latest */
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* Update by 1 */
    node_seqno += 1;
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);

    /* Update with already received node sequence number - but not latest */
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno - 1);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* New node sequence number is out of range and older than before */
    if (node_seqno < TSM_NODE_SEQNO_EXPECTED_RANGE + 1) {
        node_seqno += TSM_NODE_SEQNO_MAX - TSM_NODE_SEQNO_EXPECTED_RANGE - 1;
    }
    else {
        node_seqno -= TSM_NODE_SEQNO_EXPECTED_RANGE + 1;
    }
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(TSM_NODE_SEQNO_MAX, node_seqno);
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_NOT_EQUAL(0, ret);

    /* force to min seqno to get low limit roll over for later */
    node_seqno = 0;
    tsm.node_table.nodes[node_idx].protect_window.last_seen_time = ts_time_ms() + UINT32_MAX/2;
    ret = tsm_node_protect_window_update(&tsm, node_idx, node_seqno);
    TEST_ASSERT_EQUAL(0, ret);
    ret = tsm_node_seqno(&tsm, node_idx, &neighbour_node_seqno);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT8(node_seqno, neighbour_node_seqno);

    /* check protection window - out of range with low limit roll over */
    node_seqno = TSM_NODE_SEQNO_MAX - TSM_NODE_SEQNO_EXPECTED_RANGE - 1;
    ret = tsm_node_protect_window_check(&tsm, &neighbour_node_id, node_seqno, &node_idx);
    TEST_ASSERT_NOT_EQUAL(0, ret);
    ret = tsm_node_seqno(&tsm, node_idx, &neighbour_node_seqno);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT8(0, neighbour_node_seqno);

    /* check protection window - on range boundary with low limit roll over */
    node_seqno += 1;
    ret = tsm_node_protect_window_check(&tsm, &neighbour_node_id, node_seqno, &node_idx);
    TEST_ASSERT_EQUAL(0, ret);
    ret = tsm_node_seqno(&tsm, node_idx, &neighbour_node_seqno);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL_UINT8(node_seqno, neighbour_node_seqno);
}

void test_mesh_helpers(void)
{
    TEST_ASSERT_EQUAL(true, tsm_node_id_equal(&originator_node_id, &originator_node_id));
    TEST_ASSERT_EQUAL(false, tsm_node_id_equal(&neighbour_node_id, &originator_node_id));

    TEST_ASSERT_EQUAL(true, tsm_port_same(&tsm_ports[0], &tsm_ports[0]));
    TEST_ASSERT_EQUAL(false, tsm_port_same(&tsm_ports[0], &tsm_ports[1]));
}

#endif /* CONFIG_THINGSET_MESH */
