/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

GENERAL DESCRIPTION
  This file contains the current version for the Lowi Interface

Copyright (c) 2016, 2021-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/

#ifndef _LOWI_VERSION_H_
#define _LOWI_VERSION_H_


#if LOWI_MAJOR_VERSION >= 2
  #define LOWI_IF_MAJOR_VERSION_SUPPORTED 2
#else
  #define LOWI_IF_MAJOR_VERSION_SUPPORTED 1
#endif
#define LOWI_IF_MINOR_VERSION_SUPPORTED   1
#if LOWI_MICRO_VERSION == 1
  #define LOWI_IF_MICRO_VERSION_SUPPORTED 1
#else
  #define LOWI_IF_MICRO_VERSION_SUPPORTED 0
#endif

// lowi version supported: LOWI-INTERFACE-a.b.c.d
// where,
// a: LOWI_IF_MAJOR_VERSION_SUPPORTED
// b: LOWI_IF_MINOR_VERSION_SUPPORTED
// c: LOWI_IF_MICRO_VERSION_SUPPORTED
// d: changes and bug fixes (rolling counter)
const char* const LOWI_INTERFACE_VERSION = "LOWI-INTERFACE-LOWI-9.0.0.27";

#endif /* _LOWI_VERSION_H_ */
