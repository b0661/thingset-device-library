/*
 * Copyright (c) 2017 Martin Jäger / Libre Solar
 * Copyright (c) 2021 Bobby Noelte.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TS_CONFIG_H_
#define TS_CONFIG_H_

/*
 * Enable legacy C++ interface.
 *
 * This option enables the legacy C++ interface of the
 * ThingSet protocol library. Enable if your C++ code uses
 * DataNode or ArrayInfo instead of ThingSetDataNode or ThingSetArrayInfo.
 */
#if defined(__cplusplus) && !defined(CONFIG_THINGSET_CPP_LEGACY)
#define CONFIG_THINGSET_CPP_LEGACY 1
#endif

/*
 * Maximum number of expected JSON tokens (i.e. arrays, map keys, values,
 * primitives, etc.)
 *
 * Thingset throws an error if maximum number of tokens is reached in a
 * request or response.
 */
#if !defined(TS_NUM_JSON_TOKENS) && !defined(CONFIG_THINGSET_NUM_JSON_TOKENS)
#define TS_NUM_JSON_TOKENS 50
#elif !defined(TS_NUM_JSON_TOKENS)
#define TS_NUM_JSON_TOKENS CONFIG_THINGSET_NUM_JSON_TOKENS
#endif

/*
 * If verbose status messages are switched on, a response in text-based mode
 * contains not only the status code, but also a message.
 */
#if !defined(TS_VERBOSE_STATUS_MESSAGES) && !defined(CONFIG_THINGSET_VERBOSE_STATUS_MESSAGES)
#define TS_VERBOSE_STATUS_MESSAGES 1
#elif !defined(TS_VERBOSE_STATUS_MESSAGES)
#define TS_VERBOSE_STATUS_MESSAGES CONFIG_THINGSET_VERBOSE_STATUS_MESSAGES
#endif

/*
 * Switch on support for 64 bit variable types (uint64_t, int64_t, double)
 *
 * This should be disabled for most 8-bit microcontrollers to increase
 * performance
 */
#if !defined(TS_64BIT_TYPES_SUPPORT) && !defined(CONFIG_THINGSET_64BIT_TYPES_SUPPORT)
#define TS_64BIT_TYPES_SUPPORT 0        // default: no support
#elif !defined(TS_64BIT_TYPES_SUPPORT)
#define TS_64BIT_TYPES_SUPPORT CONFIG_THINGSET_64BIT_TYPES_SUPPORT
#endif

/*
 * Switch on support for CBOR decimal fraction data type which stores a decimal mantissa
 * and a constant decimal exponent. This allows to use e.g. millivolts internally instead
 * of floating point numbers, while still communicating the SI base unit (volts).
 */
#if !defined(TS_DECFRAC_TYPE_SUPPORT) && !defined(CONFIG_THINGSET_DECFRAC_TYPE_SUPPORT)
#define TS_DECFRAC_TYPE_SUPPORT 0        // default: no support
#elif !defined(TS_DECFRAC_TYPE_SUPPORT)
#define TS_DECFRAC_TYPE_SUPPORT CONFIG_THINGSET_DECFRAC_TYPE_SUPPORT
#endif

/*
 * Switch on support for CBOR byte strings, which can store any sort of binary data and
 * can be used e.g. for firmware upgrades. Byte strings are not supported by JSON.
 */
#if !defined(TS_BYTE_STRING_TYPE_SUPPORT) && !defined(CONFIG_THINGSET_BYTE_STRING_TYPE_SUPPORT)
#define TS_BYTE_STRING_TYPE_SUPPORT 0        // default: no support
#elif !defined(TS_BYTE_STRING_TYPE_SUPPORT)
#define TS_BYTE_STRING_TYPE_SUPPORT CONFIG_THINGSET_BYTE_STRING_TYPE_SUPPORT
#endif

/**
 * @def TS_BUF_COUNT
 * @brief Number of buffers in the ThingSet communication buffer pool.
 */
#if !defined(TS_BUF_COUNT) && !defined(CONFIG_THINGSET_COM_BUF_COUNT)
#define TS_BUF_COUNT 16
#elif !defined(TS_BUF_COUNT)
#define TS_BUF_COUNT CONFIG_THINGSET_COM_BUF_COUNT
#endif

/**
 * @def TS_BUF_DATA_SIZE
 * @brief Data block size for ThingSet communication buffers.
 */
#if !defined(TS_BUF_DATA_SIZE) \
    && !defined(CONFIG_THINGSET_COM_BUF_DATA_SIZE)
#define TS_BUF_DATA_SIZE 1024
#elif !defined(TS_BUF_DATA_SIZE)
#define TS_BUF_DATA_SIZE CONFIG_THINGSET_COM_BUF_DATA_SIZE
#endif

#endif /* TS_CONFIG_H_ */
