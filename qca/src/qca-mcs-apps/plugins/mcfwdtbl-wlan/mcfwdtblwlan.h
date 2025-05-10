/*
 * @File: mcfwdtblwlan.h
 *
 * @Abstract: WLAN Multicast forwarding database plugin
 *
 * Copyright (c)  2014-2015, 2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2014-2015 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#ifndef MCFWDTBLWLAN_H_
#define MCFWDTBLWLAN_H_

#include "mcif.h"

int WLAN_InitForwardTablePlugin(interface_t *iface);
int WLAN_UpdateForwardTable(interface_t *iface, void *table, u_int32_t size);
int WLAN_FlushForwardTable(interface_t *iface);

#endif /* MCFWDTBLWLAN_H_ */
