/*
 * @File: mesh_app.h
 *
 * @Abstract: Mesh Application Daemon main header file
 *
 * @Notes:
 *
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef mesh_app__h /*once only*/
#define mesh_app__h

#include <string.h>
#include <sys/types.h>

#ifndef _BYTE_ORDER
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#define _BYTE_ORDER _BIG_ENDIAN
#endif
#endif  // _BYTE_ORDER

#include <ieee80211_external.h>

#define MESH_FRAME_LEN_MAX 2048
#define MESH_API_PORT 7788

#define MESH_API_VERSION_STR MAD_SON_VERSION "," \
    MAD_MAP_VERSION

#define __meshMidx(_arg, _i) (((u_int8_t *)_arg)[_i])

/*
 * meshMACAddFmt - Format a MAC address (use with (s)dbgf)
 */
#define meshMACAddFmt(_sep) "%02X" _sep "%02X" _sep "%02X" _sep "%02X" _sep "%02X" _sep "%02X"

/*
 * meshMACAddData - MAC Address data octets
 */
#define meshMACAddData(_arg) __meshMidx(_arg, 0), __meshMidx(_arg, 1), __meshMidx(_arg, 2), __meshMidx(_arg, 3), __meshMidx(_arg, 4), __meshMidx(_arg, 5)


typedef enum
{
    MESH_OK = 0,
    MESH_NOK = -1
} MESH_STATUS;

typedef enum
{
    MESH_FALSE = 0,
    MESH_TRUE = !MESH_FALSE
} MESH_BOOL;

enum MeshModeType
{
    IEEE1905_MODE = 1,
    DE_MODE = 2,
};
#endif
