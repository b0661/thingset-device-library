# Mesh Concept

The ThingSet protocol provides a consistent, standardized way to configure,
monitor and control ressource-constrained devices via different communication
interfaces. It specifies the higher layers (5 to 7) of the [OSI (Open Systems
Interconnection) model](https://en.wikipedia.org/wiki/OSI_model). The payload
data is independent of the underlying lower layer protocol or interface, which
can be CAN, USB, LoRa, WiFi, Bluetooth, UART (serial) or similar.

The ThingSet Mesh extends the explicit communication link definition of the
standard ThingSet protocol by mesh communication functionality. It is designed
to operate also independent of the underlying lower layer protocol or interface.

Many ideas are taken from the [B.A.T.M.A.N. Advanced Network](https://www.open-mesh.org/projects/batman-adv/wiki/Understand-your-batman-adv-network) protocol.

# Definitions

device
:   A pyhsical device that hosts one (or several) ThingSet Mesh node(s).

node
:   A ThingSet Mesh instance that utilizes the ThingSet Mesh protocol at least at one port.

    - neighbour: A node within single hop distance that broadcasts heartbeat statements.

    - originator: A node within multi hop distance that is announced by a neighbour´s originator
                  statement.

    - phantom: A node from whome we got a message but no heartbeat statement nor was it announced
               by a neighbour´s originator statement.

port
:   An interface utilized by a ThingSet Mesh node for ThingSet Mesh communication.

role
:   The role of a node within the ThingSet Mesh:

    - simple node: A node with a single port. Typically a sensor node.

    - router node: A node with multiple ports. A router node provides to it´s neighbours a
                   potential, loop-free next hop for forwarding data packets towards a specific
                   originator. May be a sensor with additional routing capabilities or a pure
                   router.

    - gateway node: A node with a single port that provides a gateway to other protocols or
                    networks. It may connect the ThingSet Mesh to LoRaWan or MQTT or ...

throughput
:   Data rate from a node´s port to another node´s port.

    - link throughput: Data rate from a node´s port to a neighbour´s port.

    - path throughput: Data rate from a node´s port to an originator´s port.

# Mesh Protocol

## Path

In mesh communication it is impractical to set the source or sink of a
communication by explicit requests. Therefor the object path is extended to
include the source or destination node identifier.

### Text Mode

Mesh object path:

    path = "/" [ user "@" ] node-id  "/" object-name [ "/" object-name ]

General object path:

    path = object-name [ "/" object-name ]

### Binary Mode

Mesh object path - CBOR string (same as text mode):

    path = "/" [ user "@" ] node-id  "/" object-name [ "/" object-name ]

Mesh object path - CBOR array (user-id: uint, node-id: uint, node-object-id: uint):

    path = [ user-id ] node-id node-object-id

General object path - CBOR string (same as text mode):

    path = object-name [ "/" object-name ]

Node object path - CBOR uint (node object ID instead of node object path):

    path = node-object-id

## Node Sequence Number

ThingSet Mesh messages carry a node sequence number. The node sequence number range is 0..23 to fit
into a single byte CBOR uint. The node sequence number rolls over.

A ThingSet Mesh node increments it's node sequence number on every message that originates from the
node and is transmitted at one of it's ports.

## Protection Window Check

To distinguish valid ThingSet Mesh messages from out of date/ delayed/ doubled messages a sliding
window pinned to the last received valid node sequence number is used. The sliding window is called
the *Protection Window*.

A valid node sequence number has to be received within the last TSM_NODE_SEQNO_MAX_AGE_S seconds.

If a valid last node sequence number exists, a valid message has to have a node sequence number that
is new compared to the chached sequence numbers of messages received before. Additional it has to be
greater than the last valid node sequence number reduced by TSM_NODE_SEQNO_EXPECTED_RANGE and be
less than the last valid node sequence number advanced by TSM_NODE_SEQNO_EXPECTED_RANGE taking into
account roll over.

If the last valid sequence number gets out-dated the protection window is de-activated and all
cached node sequence numbers are deleted. Any new node sequence number will be accepted afterwards
and the protection window activated again.

The node sequence number cache size is limited to TSM_NODE_SEQNO_CACHE_SIZE entries per node. If a
node sends a lot of messages in a short time multiple copies of a message may still not be
detectable.

## Request

A request is an unicast message. It includes a destination path and a source path.
The destination path is the mesh object path the request is targeting. The source path is the mesh
object path the response shall be sent to.

    mesh-bin-request node-seqno
                     dest-path(node-id node-object-id) source-path(node-id node-object-id)
                     ...


The ThingSet Mesh protocol supports the typical [CRUD operations](https://en.wikipedia.org/wiki/Create,_read,_update_and_delete).
ThingSet Mesh protocol request codes and text ids differ from the standard ThingSet protocol:

| Code | Text ID | Method | Description                                                          |
|------|---------|--------|----------------------------------------------------------------------|
| 0x10 | G       | GET    | Get all data from a path                                             |
| 0x11 | A       | POST   | Append data to an object (which may be created) or activate function |
| 0x12 | D       | DELETE | Delete data from an object                                           |
| 0x13 | F       | FETCH  | Fetch a subset of data from a path                                   |
| 0x14 | U       | iPATCH | Update (overwrite) data of a path                                    |

Codes 0x0A, 0x0D and 0x20-0x7F are reserved, as they represent the ASCII characters for readable
text including LF and CR.

### Text mode request

Each request message consists of a first character as the request method identifier, a path
specifying the destination endpoint of the request, a path specifiying the source endpoint of the
request and a JSON string for the payload data (if applicable).

    mesh-txt-request  = mesh-txt-get / mesh-txt-append / mesh-txt-activate / mesh-txt-delete /
                        mesh-txt-fetch / mesh-txt-update

    mesh-txt-get      = "G" node-seqno dest-path ":" source-path                 ; CoAP equivalent: GET request

    mesh-txt-append   = "A" node-seqno dest-path ":" source-path " " json-value  ; CoAP equivalent: POST request

    mesh-txt-activate = "A" node-seqno dest-path ":" source-path " " json-value  ; CoAP equivalent: POST request

    mesh-txt-delete   = "D" node-seqno dest-path ":" source-path " " json-value  ; CoAP equivalent: DELETE request

    mesh-txt-fetch    = "F" node-seqno dest-path ":" source-path " " json-array  ; CoAP equivalent: FETCH request

    mesh-txt-update   = "U" node-seqno dest-path ":" source-path " " json-object ; CoAP equivalent: iPATCH request

    path = "/" [ user "@" ] node-id  "/" object-name [ "/" object-name ]

    object-name = ALPHA / DIGIT / "." / "_" / "-"   ; compatible to URIs (RFC 3986)

The path to access a specific data object is a JSON pointer ([RFC 6901](https://tools.ietf.org/html/rfc6901)).
The useable characters for object names are further restricted to allow un-escaped usage in URLs.

## Response

A response is also an unicast message. It includes a destination path only. The destination path is
the mesh object source path received by the request.

    mesh-bin-response node-seqno dest-path(node-id node-object-id)
                      ...

The ThingSet Mesh protocol reponse code and text id differ from the standard ThingSet protocol:

| Code | Text ID | Description         |
|------|---------|---------------------|
| 0x15 | R       | Response message    |

The ThingSet Mesh protocol status code is the same as the standard ThingSet protocol status code:

| Code | CoAP | HTTP | Description                | Comment                                       |
|------|------|------|----------------------------|-----------------------------------------------|
| 0x81 | 2.01 | 201  | Created                    | Answer to POST requests appending data        |
| 0x82 | 2.02 | 204  | Deleted                    | Answer to DELETE request                      |
| 0x83 | 2.03 | 200  | Valid                      | Answer to POST requests to exec objects       |
| 0x84 | 2.04 | 204  | Changed                    | Answer to PATCH requests                      |
| 0x85 | 2.05 | 200  | Content                    | Answer to GET / FETCH requests                |
| 0xA0 | 4.00 | 400  | Bad Request                |                                               |
| 0xA1 | 4.01 | 401  | Unauthorized               | Authentication needed                         |
| 0xA3 | 4.03 | 403  | Forbidden                  | Trying to write read-only value               |
| 0xA4 | 4.04 | 404  | Not Found                  |                                               |
| 0xA5 | 4.05 | 405  | Method Not Allowed         | If e.g. DELETE is not allowed for that object |
| 0xA8 | 4.08 | 400  | Request Entity Incomplete  |                                               |
| 0xA9 | 4.09 | 409  | Conflict                   | Configuration conflicts with other settings   |
| 0xAD | 4.13 | 413  | Request Entity Too Large   |                                               |
| 0xAF | 4.15 | 415  | Unsupported Content-Format | If trying to assign a string to an int        |
| 0xC0 | 5.00 | 500  | Internal Server Error      |                                               |
| 0xC1 | 5.01 | 501  | Not Implemented            |                                               |

### Text mode response

The response starts with an `R` followed by the destination path, and the status code.

    mesh-txt-response = "R" node-seqno dest-path " " status-code [ " " json-value ]

    status-code = 2( hex )

    hex = DIGIT / %x41-46                           ; upper-case HEXDIG

    path = "/" [ user "@" ] node-id  "/" object-name [ "/" object-name ]

    object-name = ALPHA / DIGIT / "." / "_" / "-"   ; compatible to URIs (RFC 3986)


## Statement

A statement is a broadcast message. It does not have a destination path.

    mesh-bin-statement node-seqno source-path(node-id node-object-id)
                       ...

The ThingSet Mesh protocol statement code and text id differ from the standard ThingSet protocol:

| Code | Text ID | Description         |
|------|---------|---------------------|
| 0x16 | S       | Statement message   |

### Text mode statement

A statement starts with an `S`, the actual node sequence number and a source path, followed by a
whitespace and the map of actual payload data as name/value pairs.

    mesh-txt-statement = "S" node-seqno source-path " " json-object

The object path is either a group (e.g. `meas`) or a subset object containing references to other
data items as an array (e.g. `report`).

## Routing metrics

### Throughput data ranges

Throughput is specified in data rate ranges:

    -  0: None
    -  1: >    0     B/s & <  10.0   B/s: Serial 75 bit/s 8N1
    -  2: >=  10.0   B/s & <  12.5   B/s: Serial 110 bit/s 8N1
    -  3: >=  12.5   B/s & <  25.0   B/s:
    -  4: >=  25.0   B/s & <  50.0   B/s: Serial 300 bit/s 8N1
    -  5: >=  50.0   B/s & <  75.0   B/s:
    -  6: >=  75.0   B/s & < 100.0   B/s:
    -  7: >= 100.0   B/s & < 125.0   B/s: Serial 1200 bit/s 8N1
    -  8: >= 125.0   B/s & < 250.0   B/s: Serial 2400 bit/s 8N1
    -  9: >= 250.0   B/s & < 500.0   B/s: Serial 4800 bit/s 8N1
    - 10: >= 500.0   B/s & < 750.0   B/s:
    - 11: >= 750.0   B/s & <   1.0  kB/s: Serial 9600 bit/s 8N1
    - 12: >=   1.0  kB/s & <   1.25 kB/s:
    - 13: >=   1.25 kB/s & <   2.5  kB/s: Serial 19200 bit/s 8N1
    - 14: >=   2.5  kB/s & <   5.0  kB/s: Serial 38400 bit/s 8N1
    - 15: >=   5.0  kB/s & <   7.5  kB/s: Serial 57600 bit/s 8N1
    - 16: >=   7.5  kB/s & <  10.0  kB/s:
    - 17: >=  10.0  kB/s & <  12.5  kB/s: CAN 125 kbit/s, Serial 115200 bit/s 8N1
    - 18: >=  12.5  kB/s & <  25.0  kB/s:
    - 19: >=  25.0  kB/s & <  50.0  kB/s:
    - 20: >=  50.0  kB/s & <  75.0  kB/s:
    - 21: >=  75.0  kB/s & < 100.0  kB/s:
    - 22: >= 100.0  kB/s & < 125.0  kB/s: BT 1.1, CAN 1 Mbit/s
    - 23: >= 125.0  kB/s & < 250.0  kB/s:
    - 24: >= 250.0  kB/s & < 500.0  kB/s: BT 2.0, I2C
    - 25: >= 500.0  kB/s & < 750.0  kB/s:
    - 26: >= 750.0  kB/s & <   1.0  MB/s:
    - 27: >=   1.0  MB/s & <   1.25 MB/s:
    - 28: >=   1.25 MB/s & <   2.5  MB/s:
    - 29: >=   2.5  MB/s & <   5.0  MB/s: BT 3.0, BT 4.0
    - 30: >=   5.0  MB/s & <   7.5  MB/s: BT 5.0
    - 31: >=   7.5  MB/s & <  10.0  MB/s:
    - 32: >=  10.0  MB/s & <  12.5  MB/s:
    - 33: >=  12.5  MB/s & <  25.0  MB/s:
    - 34: >=  25.0  MB/s & <  50.0  MB/s:
    - 35: >=  50.0  MB/s & <  75.0  MB/s: WiFi 4
    - 36: >=  75.0  MB/s & < 100.0  MB/s:
    - 37: >= 100.0  MB/s & < 125.0  MB/s: GigaBit Ethernet
    - 38: >= 125.0  MB/s & < 250.0  MB/s:
    - 39: >= 250.0  MB/s & < 500.0  MB/s:
    - 40: >= 500.0  MB/s & < 750.0  MB/s:
    - 41: >= 750.0  MB/s & <   1.0  GB/s: WiFi 5
    - 42: >=   1.0  GB/s & <   1.25 GB/s:
    - 43: >=   1.25 GB/s & <   2.5  GB/s: WiFi 6
    - 44: >=   2.5  GB/s & <   5.0  GB/s:
    - 45: >=   5.0  GB/s & <   7.5  GB/s:
    - 45: >=   7.5  GB/s & <  10.0  GB/s:
    - 46: >=  10.0  GB/s & <  12.5  GB/s:
    - 47: >=  12.5  GB/s & <  25.0  GB/s:
    - 48: >=  25.0  GB/s & <  50.0  GB/s:
    - 49: >=  50.0  GB/s & <  75.0  GB/s;
    - 50: >=  75.0  GB/s & < 100.0  GB/s;
    - 51: >= 100.0  GB/s & < 125.0  GB/s:
    - 52: >= 125.0  GB/s & < 250.0  GB/s:
    - 53: >= 250.0  GB/s & < 500.0  GB/s:
    - 54: ...

For comparison see this [list of interface bit rates](https://en.wikipedia.org/wiki/List_of_interface_bit_rates).

### Hop penalties

In certain network setups the link throughput between neighbours is very similar whereas the number
of hops is not. In these scenarios it is desirable to chose the shortest path to reduce latency and
to safe bandwidth (especially on wireless mediums). The hop penalty is a value greater than zero
and defined in data rate steps. It is a fixed value defined by TS_MESH_HOP_PENALTY. The hop penalty
is applied on an outgoing originator statement in the followig way:

    Outgoing originator statement throuput value = path throughput - hop penalty

## Protocol tweaking constants

  - TS_MESH_HOP_PENALTY: 1
  - TS_MESH_NODE_SEQNO_EXPECTED_RANGE: 10
  - TS_MESH_NODE_SEQNO_MAX_AGE_S: 3

# Mesh Protocol Procedures

## Mesh topology detection and thoughput based routing

### Broadcasting own heartbeat statements

Each mesh node periodically sends heartbeat statements:

    mesh-bin-statement node-seqno source-path(node-id TSM_DO_HEARTBEAT_ID)
                       cbor-array(version, period-s, name-mapping-id)

  - The node sequence number is the current sequence number of the mesh node. The node sequence
    number is incremented before use.
  - The version is set to protocol version 0.
  - The period is set to the actual period of heartbeat statements. It is defined in units of 1
    second.
  - The name-mapping-id is set to a number that uniquely identifies the node's name map content.

The heartbeat statement is transmitted on all ports of the mesh node.

The heartbeat statement refresh interval should be adapted to the throughput at the outgoing port:

  1. heartbeat refresh interval >= 100 * statement length (in Bytes) / throughput at port (in B/sec)

If rule 1. (< 1% throughput consumption for heartbeat) can not be fulfilled at all ports the
heartbeat refresh interval should be prolonged up to an acceptable compromise value between faster
change detection and less data rate consumption.

### Processing heartbeat statements

Upon processing a neighbour's heartbeat statement a node shall perform the following preliminary
checks before the statement is further processed:

  - *VERSION CHECK*: If the heartbeat statement contains a version which is
    different to the own internal version the message shall be silently dropped
    (thus, it shall not be further processed).
  - There is no protection window check as no out of date/ delayed heartbeat statements are assumed
    on the link to a neighbour. Node sequence number jumps are assumed to be due to neighbour
    restarts or by intention.

For each neighbour heartbeat statement having passed the preliminary checks the
following actions shall be performed:

  - An entry for the (node id) node shall be created in the node table, if not available.
  - An entry for the (port) neighbour shall be created in the neighbour table, if not
    available.
  - The last seen time of this (node id, port) neighbour entry in the node
    table shall be updated.
  - The heartbeat period of this this (node id, port) neighbour entry in the node table shall
    be updated with the period in the received neighbour´s heartbeat statement.
  - The node sequence number of this (node id, port) neighbour entry in the node table shall be
    updated with the node sequence number in the received neighbour´s heartbeat statement.
  - The name-mapping-id shall be updated.

#### Heartbeat route update

The heartbeat statement will be considered for a best next hop update:

  - If the best next hop node of the neighbour node is the neighbour node, no change is necessary.
  - If no best next hop node has been selected yet, the received neighbour node becomes the selected
    router node.
  - If the link throughput to the neighbour node is higher than the throughput via the selected
    router node, the received neighbour node becomes the selected router node.

```C
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
```

### Broadcasting originator statements for new neighbours

If a neighbour heartbeat statement is received by a mesh node with several ports and the neighbour
is new to this mesh node an originator statement for the new entry in the node table shall be
broadcasted on all ports. The broadcast, limited to the single node table entry of the enighbour,
shall be performed immediatedly following the description given in
[broadcasting originator statements](#broadcasting-originator-statements).

### Broadcasting originator statements

The nodes that are registered in the node table shall be periodically published at every port. The
period shall be dependent on the ports throughput. A maximum of 1% of the available throughput shall
be used for these originator statements.

To get a valid node entry for publishing at an outgoing port the node shall
perform the following preliminary checks before the originator statement is
transmitted at the outgoing port:

  - *NEIGHBOUR CHECK*: If the node table entry is a neighbour at the outgoing port then skip the
     entry for this port.
  - *ROUTER CHECK*: If the node table entry is an originator and it's router node is a neighbour at
     the outgoing port then skip the entry for this port.

On a valid node table entry:

  - Create an originator statement from the node table entry and send at the port:

        mesh-bin-statement originator-node-seqno path(originator-node-id TSM_DO_ORIGINATOR_ID)
                           cbor-array(version, originator-age-ms, originator-name-mapping-id,
                                      router-node-id, throughput)

  - The values for originator-node-seqno, originator-age-ms (mlliseconds since node last seen),
    originator-name-mapping-id and throughput  shall be taken from the node table entry.
  - The router-node-id shall be set to the node id of this node.
  - The originator statement shall be transmitted on the port.

  - Step on to next node table entry.

### Processing originator statements

Upon processing an originator statement a node shall perform the following preliminary checks before
the statement is further processed:

  - *VERSION CHECK*: If the originator statement contains a version which is
    different to the own internal version the message shall be silently dropped
    (thus, it shall not be further processed).
  - *OWN MESSAGE CHECK*: If the last hop node id of the originator statement
    is our own the message shall be silently dropped.
  - *ORIGINATOR ROUTER CHECK*: If the last hop node id of the originator statement is the same as
    the originator node id the message shall be silently dropped. This combination would indicate a
    neighbour which is announced by [heartbeat statements](#broadcasting-own-heartbeat-statements).
  - *ROUTER NODE CHECK*: If the last hop node is not available within the node table, then a
    node table entry shall be created with protection window and name mapping id set to
    invalid. A neighbour entry shall be attached to the node table entry with the hartbeat period
    set to 0 for infinite and port set to the port the originator statement was received on.

For each originator statement having passed the preliminary checks the
following actions shall be performed:

  - If a (originator node id) node table entry does not exist in the node table, create it.
    - The last seen time of this node table entry shall be set.
    - The node sequence number of this node table entry shall be set with the node sequence number
      in the received originator statement.
    - The name-mapping-id shall be set.
  - If an associated originator table entry does not exist in the originator table, create it and
    reference it from the node table entry.
    - The next best hop node shall be marked not set.

The following steps check whether the (last hop node, port) neighbour we received the originator
statement from is a next best hop node - a router.

#### Metric update

The following checks are performed before updating the metric:

  - *PROTECTION WINDOW OOR CHECK*: If the originator statement does not pass the
    [protection window check](#protection-window-check) due to the sequence number out of the
    protection window, then the statement shall be silently dropped. If the [protection window check](#protection-window-check) is not passed due to the node sequence number was already seen the
    originator statement shall be further processed.
  - *LOOP AND BEST PATH CHECK*: If the node sequence number is equal to the originator node table
    entry´s node sequence number and the throughput is lower than the originator node table entry's
    next best hop throughput then silently drop the statement.

If the initial checks above have passed, the internal stats are updated:

  - the last seen timestamps of the (last hop node id, port) neighbour entry
    and the originator entry are updated.
  - the node sequence number of the originator entry is updated
  - the name-mapping-id of the originator entry is updated.

Calculate throughput to the originator by the (last hop node, port) neighbour:

  - *LINK TRANSMISSION RATE LIMITATION*: If the port throughput to the (last hop node id, port)
    neighbour is *lower* than the path throughput of the originator statement, then this lower port
    throughput is adopted.
  - *LINK HALF DUPLEX LIMITATION*: If the incoming and considered outgoing port is the same half
    duplex port and the reported throughput is larger than 250 kB/s, the throughput is reduced
    by 50%.
  - *HOP PENALTY*: If the orginator is not the (last hop node id, port) neighbour a forward hop
    penalty is applied and the throughput is reduced by the according value (default: [one throughput data rate range step](#throuput-data-ranges)).
    date rate step). This is especially useful for "perfect" networks to create a decreasing metric
    over multiple hops. As this would limit the number of possible hops the throughput is never
    reduced below [throughput data rate range 1](#throuput-data-ranges).
  - the throughput value with the penalties applied is remembered for the [route update](#originator-route-update).

#### Originator route update

The originator statement will be considered for a best next hop update:

  - if the originator statement has been received from the current router node,
    no change is necessary
  - if no router node has been selected yet, the received last hop node becomes the selected
    router node.
  - if the throughput from the received last hop node is higher than the
    throughput via the selected router node, the received last hop node becomes the selected
    router node.

```C
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
 * @return 0 on success, < 0 otherwise.
 */
int tsm_originator_update(struct tsm_context *tsm, tsm_node_seqno_t node_seqno,
                          const tsm_node_id_t *node_id, uint8_t version, tsm_time_ms_t age_ms,
                          uint32_t name_mapping_id, const tsm_node_id_t *router_node_id,
                          uint8_t throughput, tsm_port_id_t port_id);
```

### Forwarding Requests and Responses

Upon forwarding a request or response a node shall perform the following preliminary checks before
the request or response is further processed:

  - *ROUTER CHECK*: If the node has only one port forwarding shall be disabled.

### Forwarding Statements

Upon forwarding a statement a node shall perform the following preliminary checks before the
statement is forwarded:

  - *PORT CHECK*: If the statement was received at the port we wan´t to forward on, then do not
      forward.
  - *NEIGHBOUR CHECK*: If the statement source node is a neighbour at the port we wan´t to forward
      on, then do not forward.

If the initial checks above have passed, the statement is forwarded at the port.

The statement is forwarded on all ports of a node following the procedure above.

## Statement messaging

### Receiving statements

Upon receiving a statement a node shall perform the following preliminary checks before the
statement is further processed:

  - *OWN MESSAGE CHECK*: If the source node id of the statement is our own the statement shall be
      silently dropped.
  - *PROTOCOL CHECK*: If the statement is a heartbeat statement or an originator statement it shall
      be processed by the protocol stack and dropped from normal processing. See special processing
      for [heartbeat statements](#processing-heartbeat-statements) and
      [originator statements](#processing-originator-statements). Forwarding to protocol stack
      processing shall be done before the *NODE CHECK* and *PROTECTION WINDOW CHECK* as the protocol
      processing may have it's own way to handle that.
  - *PHANTOM NODE CHECK*: If the source node is not available within the node table, then a
      phantom node table entry shall be created with protection window and name mapping id set to
      invalid. Also no neighbour nor originator entry shall be attached to the node table entry.
      All this is to allow receiving statements without full knowledge of all nodes and to prevent
      forwarding multiple same seqno statements of a bubbling idiot and thus unduely flooding the
      net.
  - *PROTECTION WINDOW CHECK*: If the statement does not pass the
      [protection window check](#protection-window-check), then the statement shall be silently
      dropped.

If the initial checks above have passed, the ThingSet Mesh protection window is updated:

  - The received node sequence number is added to the sequence number cache of the source node
    and the last cache entry insertion time is updated.

Finally the statement is passed on to [forwarding](#forwarding-statements) and to any user that is
registered for the specific statement.

### Sending statememnts


## Request/ Response messaging

### Receiving Requests

Upon receiving a request a node shall perform the following preliminary checks before the request
is further processed:

  - *DEST DEVICE CHECK*: If the destination node id of the request is not our own the message shall
      be passed on to [forwarding](#forwaring-requests-and-responses). It shall be dropped from
      normal processing.

### Sending Requests

A node that sends a request shall provide a reponse object as source object path within the request.

Upon sending a request a node shall perform the following preliminary checks before the request is
further processed:


### Sending Responses

### Receiving Responses

Upon receiving a response a node shall perform the following preliminary checks before the response
is further processed:

  - *DEST DEVICE CHECK*: If the destination node id of the response is not our own the message
                         shall be passed on to `Forwarding Requests and Responses`.

# Mesh Data Structures

## Mesh Node Context

The mesh node context is the overarching data structure that holds all data related to a ThingSet
Mesh node. There is one data structure per node. The context data structure is fixed and allocated
at compile time.

```C
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

    /**
     * @brief Node id of the mesh node.
     */
    const ts_nodeid_t *node_id;

    /**
     * @brief Node sequence_number.
     */
    tsm_node_seqno_t node_seqno;

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
```

## Neighbour Table

Each mesh node caches info of all single hop neighbors it detects. The info is part of the [node table](#node-table).

```C
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
    /** Id of port the neighbour was seen */
    tsm_port_id_t port_id;
};
```

## Originator Table

Each mesh node caches info of all other nodes in the network and remembers at which port to send the
packets if data should be transmitted. The port manifests itself in the form of the "best next hop"
which basically is the next step towards the destination. The info is part of the [node table](#node-table).

```C
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
    /** Index of router neighbour in node table */
    uint16_t router_idx;
};
```

## Node table

Each ThingSet Mesh node context holds a table of all other nodes known to this node. A node may be
known as neighbour or originator or phantom.

```C
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
```

## Ports Table

Each ThingSet Mesh node holds a table of all mesh ports it can send and receive messages. The ports
table is made up of mesh port structures that are fixed and allocated at compile time. The mesh
port structure is an extension of the general ThingSet Port structure.

```C
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
```

Within the ThingSet Mesh node context the ports are referenced by their port identifier, which is
the port's index in the ports table.

```C
/**
 * @brief Mesh port identifier.
 *
 * Mesh port identifiers are specifc to a mesh instance.
 */
typedef uint8_t ts_port_id_t;
```

## Ports Info Table

The mesh instance also holds a table of port related management information,
the ports info table.

```C
/**
 * @brief Mesh ports info table element.
 */
struct ts_port_info {
    /**
     * @brief Sequence number for originator.
     *
     * Sequence number to use for originator
     * statements at this port.
     */
    uint8_t sequence_number;

    /**
     * @@brief Index into node table of last node announced.
     *
     * Index into node table to last node that was announced at this
     * port.
     */
    uint16_t last_node_idx;
};
```


## Translation Table

To use the binary mode a ThingSet mesh nodes holds a translation table to
translate from object path to object identifier. Most object identifiers are
specific to the node that holds the object. An exception are the fixed object
identifiers in the range 0x10-0x1F and >= 0x8000.

User identifiers are specific to a node, too. The translation table is also
used to translate between user names and user identifiers.

A node holds it's identifier to name mapping in the .name object (ID 0x17).

Whenever there is a translation request by an application or by the mesh stack
and the translation is not available in the translation table the node
request the translation from the remote node:
  - if the ID is given

    bin-post path=(node-id 0x17) cbor-array(ID...)
    bin-statement path=(node-id 0x17) cbor-map(id : name, ...)

  - if the name is given

    bin-post path=(node-id 0x17) cbor-array(name...)
    bin-statement path=(node-id 0x17) cbor-map(id : name, ...)

On the reception of the statement any node updates it's translation table if
possible (space available or entries outdated/ unused).

The translation table stores translations that are specific to a
name-mapping-id.

## Name Mapping Table

The name mapping table provides node object identifiers to name translation.
It is itself a data object with ID 0x17 named ".name".

### Name Mapping ID

The Name Mapping ID uniquely identifies the current content of the name mapping
table excluding any predefined mapping (like 0x17 <-> ".name"). Nodes of the
same type may most probably share the same name-mapping-id.

# Node data structure

The node data structure must contain mandatory mesh related data objects. The
mesh related data objects support the ThingSet Mesh protocol procedures:

| Data Object Name            | Data Object ID                    | fixed ID | Description                    |
|-----------------------------|-----------------------------------|----------|--------------------------------|
| .tsmHeartbeat               | TSM_DO_HEARTBEAT_ID               | 0x08     | Heartbeat data object          |
| .tsmOriginator              | TSM_DO_ORIGINATOR_ID              | 0x09     | Originator data object         |
| .name                       | TSM_DO_NAME_ID                    | 0x17     | Name mapping data object       |
| .tsmHeartbeatVersion        | TSM_DO_HEARTBEAT_VERSION_ID       | 0x8000   | ThingSet Mesh protocol version |
| .tsmHeartbeatPeriod_s       | TSM_DO_HEARTBEAT_PERIOD_ID        | 0x8001   | Node heartbeat period          |
| .tsmHeartbeatNameMappingID  | TSM_DO_HEARTBEAT_NAME_MAPPING_ID  | 0x8002   | Node name mapping ID           |
| .tsmOriginatorVersion       | TSM_DO_ORIGINATOR_VERSION_ID      | 0x8003   | Current originator statement   |
| .tsmOriginatorAge_ms        | TSM_DO_ORIGINATOR_AGE_ID          | 0x8004   | Current originator statement   |
| .tsmOriginatorNameMappingID | TSM_DO_ORIGINATOR_NAME_MAPPING_ID | 0x8005   | Current originator statement   |
| .tsmOriginatorRouterNodeID  | TSM_DO_ORIGINATOR_ROUTER_NODE_ID  | 0x8006   | Current originator statement   |
| .tsmOriginatorThroughput    | TSM_DO_ORIGINATOR_THROUGHPUT_ID   | 0x8007   | Current originator statement   |


```JSON
{
    ".tsmHeartbeat": {                          // TSM_DO_HEARTBEAT_ID
        ".tsmHeartbeatVersion": 0,              // TSM_DO_HEARTBEAT_VERSION_ID
        ".tsmHeartbeatPeriod_s": 2,             // TSM_DO_HEARTBEAT_PERIOD_ID
        ".tsmHeartbeatNameMappingID": 1234,     // TSM_DO_HEARTBEAT_NAME_MAPPING_ID
    },
    ".tsmOriginator": {                         // TSM_DO_ORIGINATOR_ID
        ".tsmOriginatorVersion": 0,             // TSM_DO_ORIGINATOR_VERSION_ID
        ".tsmOriginatorAge_ms" : 1234,          // TSM_DO_ORIGINATOR_AGE_ID
        ".tsmOriginatorNameMappingID": 1234,    // TSM_DO_ORIGINATOR_NAME_MAPPING_ID
        ".tsmOriginatorRouterNodeID": 123456,   // TSM_DO_ORIGINATOR_ROUTER_NODE_ID
        ".tsmOriginatorThroughput" : 1,         // TSM_DO_ORIGINATOR_THROUGHPUT_ID
    },
    ".name": {                                  // TSM_DO_NAME_ID
        // ...
        "8": ".tsmHeartbeat",
        "9": ".tsmOriginator",
        // ...
        "23": ".name",
        // ...
        "2048": ".tsmHeartbeatVersion",
        "2049": ".tsmHeartbeatPeriod_s",
        "2050": ".tsmHeartbeatNameMappingID",
        "2051": ".tsmOriginatorVersion",
        "2052": ".tsmOriginatorAge_ms",
        "2053": ".tsmOriginatorNameMappingID",
        "2054": ".tsmOriginatorRouterNodeID",
        "2055": ".tsmOriginatorThroughput",
    }
}
```

# ThingSet Mesh stack

## Communication buffers

Communication buffers are a core concept of how the ThingSet Mesh stack passes data around.

Every device holds a pool of buffers that can be utilized by all ThingSet Mesh nodes that are
hosted on the device. The buffers and the buffers pool are modeled after (or uses) the Zephyr [network buffers](https://docs.zephyrproject.org/latest/reference/networking/net_buf.html?highlight=network%20buffer).
By using a single buffer pool the buffers can easily be transfered from a node´s mesh context to
ports and virtual ports (aka. another node´s mesh context on the same device) and vice
versa.

## Mesh ports

Mesh ports allow to communicate with other mesh instances via physical or
logical interfaces. Physical interfaces may be RS232, RS485, SPI, CAN, WiFi,
Ethernet, ... . Logical interfaces may be everything that translates between the
ThingSet Mesh and other worlds. Ports for logical interfaces are called virtual
ports.

```C
struct ts_port;
typedef uint8_t ts_port_id_t;
```

# Q&A

## Storing the routing tables of all other nodes can be difficult for nodes with little RAM.

Storing routing tables is only necessary for router nodes - node that have more than one port.
Or would this only be required for mesh gateways?

## I'm still not fully convinced that message routing should be part of an application layer protocol. I feel that it violates the network protocol layer structure.

ThingSet Mesh
