/*
 * Copyright (c) 2018-2020 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

#include "qca_ol_if.h"
#include "cdp_txrx_cmn_reg.h"
#include <wlan_objmgr_global_obj.h>
#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
#include <mlo_global_h_shmem_arena_api.h>
#endif
#ifdef WLAN_MLO_MULTI_CHIP
#include "cdp_txrx_mlo.h"
#endif

extern void pktlog_init_3_0(struct ol_ath_softc_net80211 *scn);
extern void *diag_event_log_attach(void);
extern void diag_event_log_detach(void *dbglog_handle);
extern void
ol_update_dp_stats(void *soc, void *stats, uint16_t id, uint8_t type);

#if ALL_POSSIBLE_RATES_SUPPORTED
extern int dp_get_supported_rates(int mode, int shortgi, int **rates);
extern int dp_get_kbps_to_mcs(int kbps_rate, int shortgi, int htflag);
#else
extern int dp_get_supported_rates(int mode, int shortgi, int nss,
				  int ch_width, int **rates);
extern int dp_get_kbps_to_mcs(int kbps_rate, int shortgi, int htflag,
			      int nss, int ch_width);
#endif


static inline void *
ol_if_dp_soc_attach(struct cdp_ctrl_objmgr_psoc *psoc,
		struct cdp_soc_attach_params *params)
{
	return dp_soc_attach_wifi3(psoc, params);
}

static inline void *
ol_if_dp_soc_init(ol_txrx_soc_handle soc, u_int16_t devid,
		void *hif_handle, struct cdp_ctrl_objmgr_psoc *psoc,
		HTC_HANDLE htc_handle,
		qdf_device_t qdf_dev, struct ol_if_ops *dp_ol_if_ops)
{
	return dp_soc_init_wifi3(soc, psoc, (struct hif_opaque_softc *)hif_handle, htc_handle,
			qdf_dev, dp_ol_if_ops, devid);
}

static struct ol_if_offload_ops wifi3_0_ops = {
	.cdp_soc_attach = &ol_if_dp_soc_attach,
	.cdp_soc_init = &ol_if_dp_soc_init,
#ifndef REMOVE_PKT_LOG
	.pktlog_init = &pktlog_init_3_0,
#endif
	.dbglog_attach = &diag_event_log_attach,
	.dbglog_detach = &diag_event_log_detach,
	.update_dp_stats = &ol_update_dp_stats,
#if defined(WLAN_MGMT_RX_REO_SUPPORT) && defined(WLAN_MLO_GLOBAL_SHMEM_SUPPORT)
	.mgmt_rx_reo_register_low_level_ops = mgmt_rx_reo_register_wifi3_0_ops,
#endif
        .kbps_to_mcs = dp_get_kbps_to_mcs,
	.get_supported_rates = dp_get_supported_rates,
#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
	.shmem_register_local_ops = global_shmem_register_wifi3_0_ops,
#endif
};

void ol_if_unregister_wifi3_0(void)
{

	qdf_nofl_info("=======%s==========\n",__func__);
}

void ol_if_register_wifi3_0(void)
{

	qdf_nofl_info("=======%s==========\n",__func__);
	ol_if_offload_ops_registration(OL_WIFI_3_0, &wifi3_0_ops);
}
