/*
 * @File: mcfwdtblwlan.c
 *
 * @Abstract: WIFI Multicast forwarding database plugin
 *
 * @Notes:
 *
 * Copyright (c)  2014-2015, 2020-2023 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * 2014-2015 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <dbg.h>

#include "wlanManager.h"
#include "qassert.h"
#include "mcif.h"

static struct mcWlanwifiState_t {
	u_int32_t IsInit;	/* overall initialization done */
	struct dbgModule *DebugModule;	/* debug message context */
} mcWLANState;

#define mcWLANDebug(level, ...) \
                 dbgf(mcWLANState.DebugModule,(level),__VA_ARGS__)

/*========================================================================*/
/*============ Internal handling =========================================*/
/*========================================================================*/

int WLAN_InitForwardTablePlugin(interface_t *iface)
{
	if (mcWLANState.IsInit)
		return 0;

	if (!iface || iface->type != interfaceType_WLAN)
		return -1;

	mcWLANState.DebugModule = dbgModuleFind("mcWlanwifi");
	mcWLANState.IsInit = 1;

	/* Finish processing for non-QCA interfaces */
	if (iface->flags & INTERFACE_FLAGS_NON_QCA)
		return 0;

	return 0;
}

int WLAN_UpdateForwardTable(interface_t *iface, void *table, u_int32_t size)
{

	int wifimsg;
	struct cfg80211_data buffer;
	wifi_cfg80211_context *ctxt;
	struct mcManagerIFTable *mctbl = (struct mcManagerIFTable *) table;

	if (!mcWLANState.IsInit)
		return -1;

	if (!iface) {
		mcWLANDebug(DBGERR, "%s: error, iface is NULL!", __func__);
		return -1;
	}

	if (iface->type != interfaceType_WLAN)
		return -1;

	if (iface->flags & INTERFACE_FLAGS_NON_QCA)
		return 0;

	buffer.data = table;

	/* Just pass the active MC groups data. */
	buffer.length = sizeof(int32_t) + sizeof(struct __mc_iftbl_entry) * (table? mctbl->entryCnt: 0);
	buffer.callback = NULL;
	buffer.parse_data = 0;
	buffer.flags = 0;
	ctxt = wlanManager_GetCfg80211ctxt();
        if (!ctxt) {
                mcWLANDebug(DBGERR, "%s: cfg80211 context not initialized!", __func__);
                return -1;
        }

	wifimsg = wifi_cfg80211_send_mctbl(ctxt, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MCTBL_UPDATE, iface->name, (char *) &buffer, size);
	if (wifimsg < 0) {
		mcWLANDebug(DBGERR, "%s: Update WLAN Multicast forwarding table!", __func__);
		return -1;
	}
	mcWLANDebug(DBGDEBUG, "%s: Successfully updated WLAN Multicast forwarding table",
		__func__);

	return 0;
}

int WLAN_FlushForwardTable(interface_t *iface)
{
	return WLAN_UpdateForwardTable(iface, NULL, 0);
}
