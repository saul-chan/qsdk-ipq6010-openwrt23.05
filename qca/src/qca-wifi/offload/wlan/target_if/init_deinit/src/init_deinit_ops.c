/*
 * Copyright (c) 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2011, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: init_deinit_ops.c.c
 *
 * API to WIN ops
 */
#include <qdf_status.h>
#include <qdf_types.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <target_if.h>
#include <wlan_tgt_def_config.h>
#include <reg_services_public_struct.h>
#include <wlan_reg_ucfg_api.h>
#include <wlan_lmac_if_def.h>
#include <hif.h>
#include <hif_hw_version.h>
#include <target_type.h>
#include "init_event_handler.h"
#include "service_ready_util.h"
#include "init_cmd_api.h"
#include "init_deinit_lmac.h"
#include <init_deinit_ops.h>
#include <wlan_osif_priv.h>
#include <wlan_lmac_dispatcher.h>
#include <wlan_psoc_mlme_api.h>
#include "cfg_ucfg_api.h"
#include "ol_cfg.h"
#if WIFI_MEM_MANAGER_SUPPORT
#include "mem_manager.h"
#endif
#include <wlan_defs.h>
#if QCA_AIRTIME_FAIRNESS
#include <target_if_atf.h>
#endif
#if UNIFIED_SMARTANTENNA
#include <target_if_sa_api.h>
#endif /* UNIFIED_SMARTANTENNA */
#if WLAN_CFR_ENABLE
#include <target_if_cfr.h>
#endif
#if WLAN_WIFI_RADAR_ENABLE
#include <target_if_wifi_radar.h>
#endif
#ifdef WIFI_TARGET_TYPE_2_0
#include <sw_version.h>
#endif
#include <dp_txrx.h>
#include <dispatcher_init_deinit.h>
#include "cdp_txrx_cmn.h"
#include "cdp_txrx_ctrl.h"
#include "fw_dbglog_api.h"
#include "ol_regdomain_common.h"
#if ATH_SUPPORT_WRAP
#include "dp_wrap.h"
#endif
#include "dp_extap.h"
#include <wlan_repeater_api.h>
#include <wlan_rnr.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <pld_common.h>
#include "wlan_mlo_mgr_public_structs.h"
#endif

static uint32_t init_deinit_get_host_pltfrm_mode(uint32_t target_type)
{

	switch (target_type) {
	case TARGET_TYPE_AR6002:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_AR6003:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_AR6004:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_AR6006:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_AR9888:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_AR6320:
		return HOST_PLATFORM_LOW_PERF;
#if PEER_FLOW_CONTROL_FORCED_MODE0
	case TARGET_TYPE_AR900B:
		return HOST_PLATFORM_LOW_PERF_NO_FETCH;
	case TARGET_TYPE_QCA9984:
		return HOST_PLATFORM_LOW_PERF_NO_FETCH;
	case TARGET_TYPE_QCA9888:
		return HOST_PLATFORM_LOW_PERF_NO_FETCH;
#elif MIPS_LOW_PERF_SUPPORT
	case TARGET_TYPE_AR900B:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_QCA9984:
		return HOST_PLATFORM_LOW_PERF;
	case TARGET_TYPE_QCA9888:
		return HOST_PLATFORM_LOW_PERF;
#else
	case TARGET_TYPE_AR900B:
		return HOST_PLATFORM_HIGH_PERF;
	case TARGET_TYPE_QCA9984:
		return HOST_PLATFORM_HIGH_PERF;
	case TARGET_TYPE_QCA9888:
		return HOST_PLATFORM_HIGH_PERF;
#endif
	default:
		target_if_err("!!! Invalid Target Type %d !!!",
			target_type);
		return -EINVAL;
	}
	return -EINVAL;
}



static void init_deinit_coex_gpio_support(struct wlan_objmgr_psoc *psoc,
			 struct target_psoc_info *tgt_hdl, wmi_unified_t wmi_handle)
{

	struct tgt_info *info;

	if (!psoc) {
		target_if_err("psoc is null");
		return;
	}

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}
	info = (&tgt_hdl->info);

	if (wmi_service_enabled(wmi_handle, wmi_service_coex_gpio)) {
		info->wlan_ext_res_cfg.fw_feature_bitmap |=
			WMI_HOST_FW_FEATURE_COEX_GPIO_SUPPORT;
		info->wlan_ext_res_cfg.wlan_priority_gpio = 0;
		wlan_psoc_nif_feat_cap_set(psoc,
				WLAN_SOC_F_BTCOEX_SUPPORT);
	}
}

static QDF_STATUS init_deinit_ext_resource_config_enable(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;

	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);

	if (wmi_service_enabled(wmi_handle,
				wmi_service_ext_res_cfg_support)) {
		info->wlan_ext_res_cfg.host_platform_config =
			init_deinit_get_host_pltfrm_mode(info->target_type);
		if (info->wlan_ext_res_cfg.host_platform_config < 0) {
			target_if_err(
			"!!! Host Mode Selection for %d TGT Type FAILED !!!",
				info->target_type);
			return QDF_STATUS_E_FAILURE;
		}
		wlan_psoc_nif_fw_ext_cap_set(psoc, WLAN_SOC_CEXT_HYBRID_MODE);
		if (wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_LTEU_SUPPORT)) {
			info->wlan_ext_res_cfg.fw_feature_bitmap |=
					WMI_HOST_FW_FEATURE_LTEU_SUPPORT;
			target_if_info("LTEu enabled ");
		}
		init_deinit_coex_gpio_support(psoc, tgt_hdl, wmi_handle);
		info->wlan_ext_res_cfg.fw_feature_bitmap |=
				WMI_HOST_FW_FEATURE_VDEV_STATS;

		if (!ol_target_lithium(psoc)) {
			if (wlan_ucfg_get_config_param(psoc, CARRIER_VOW_CONFIG)) {
				target_if_info("CARRIER_VOW_CONFIG is enabled\n");
				info->wlan_ext_res_cfg.fw_feature_bitmap |= WMI_HOST_FW_FEATURE_VOW_FEATURES;
				if (wlan_ucfg_get_config_param(psoc, FW_VOW_STATS_ENABLE)) {
					target_if_info("FW_VOW_STATS is enabled\n");
					info->wlan_ext_res_cfg.fw_feature_bitmap |= WMI_HOST_FW_FEATURE_VOW_STATS;
				}
			}
		}

		wmi_send_ext_resource_config(wmi_handle,
					&info->wlan_ext_res_cfg);
	} else {
		wlan_psoc_nif_fw_ext_cap_clear(psoc, WLAN_SOC_CEXT_HYBRID_MODE);
	}

	return QDF_STATUS_SUCCESS;
}

static uint8_t init_deinit_get_total_num_ema_vaps(
		struct wlan_objmgr_psoc *psoc, uint8_t num_radios)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	uint8_t ema_max_vap_cnt = 0, pdev_id;
	ol_ath_ema_pdev_ctx_t *ema_ctx;
	struct ol_ath_softc_net80211 *scn = NULL;

	if (!psoc) {
		target_if_err("psoc is null");
		return QDF_STATUS_E_FAILURE;
	}

	target_if_info("num_radios: %d", num_radios);

	for (pdev_id = 0; pdev_id < num_radios; pdev_id++) {
		pdev = wlan_objmgr_get_pdev_by_id(psoc,
				pdev_id, WLAN_MLME_NB_ID);

		if (pdev) {
			scn = lmac_get_pdev_feature_ptr(pdev);
			if (!scn) {
				target_if_err("scn is null");
				wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
				return QDF_STATUS_E_FAILURE;
			}

			ema_ctx = &scn->ema_ctx;
			if (wlan_pdev_nif_feat_ext_cap_get(pdev,
			WLAN_PDEV_FEXT_EMA_AP_ENABLE)) {
				uint8_t num_vdevs = wlan_pdev_get_config_params(pdev, WLAN_CONFIG_NUM_VDEVS);
				ema_max_vap_cnt +=
				qdf_min(ema_ctx->num_max_vaps, num_vdevs);
			} /* end if */

			wlan_objmgr_pdev_release_ref(pdev,
					WLAN_MLME_NB_ID);
		} /* end if (pdev) */
	} /* end for */

	target_if_info("ema_max_vap_cnt: %d", ema_max_vap_cnt);
	return ema_max_vap_cnt;
}

#ifdef WLAN_FEATURE_11BE_MLO
static bool soc_check_group_for_bridge_vdevs(ol_ath_soc_softc_t *soc)
{
	uint8_t grp_id;
	struct cnss_mlo_group_info info;

	if (cfg_get(soc->psoc_obj, CFG_OL_BRIDGE_VAP_DISABLE)) {
	        target_if_debug("Bridge VAP support disabled via ini");
		return false;
	}

	grp_id = pld_get_mlo_group_id(soc->qdf_dev->dev);

        if (!pld_get_mlo_group_info(grp_id, &info)) {
	        target_if_debug("Unable to get mlo group info!");
		return false;
	}

	if (info.num_chips < MLO_NUM_CHIPS_FOR_BRIDGE_LINK) {
		target_if_debug("Num chips in MLO group%d: %d\n", grp_id, info.num_chips);
		return false;
	}

	return true;
}
#endif

static uint8_t init_deinit_get_total_num_vdevs_for_soc(struct wlan_objmgr_psoc *psoc,
		uint8_t num_radios)
{
	ol_ath_soc_softc_t *soc       = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	uint8_t num_vdevs             = 0, pdev_id;
	bool is_ema_enabled           = false;

	if (!psoc) {
		target_if_err("psoc is null");
		return 0;
	}

	soc = lmac_get_psoc_feature_ptr(psoc);
	if (!soc) {
		target_if_err("soc is null");
		return 0;
	}

	num_vdevs = init_deinit_get_total_num_ema_vaps(psoc, num_radios);

	for (pdev_id = 0; pdev_id < num_radios; pdev_id++) {
		pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);

		if (pdev) {
			is_ema_enabled = wlan_pdev_nif_feat_ext_cap_get
			(pdev, WLAN_PDEV_FEXT_EMA_AP_ENABLE);

			num_vdevs += wlan_pdev_get_config_params(pdev, WLAN_CONFIG_NUM_MONITOR_VAPS);
#ifdef WLAN_FEATURE_11BE_MLO
			if (soc_check_group_for_bridge_vdevs(soc))
				num_vdevs += MLO_MAX_BRIDGE_LINKS_PER_RADIO;
#endif
			if (!is_ema_enabled) {
				num_vdevs += wlan_pdev_get_config_params(pdev, WLAN_CONFIG_NUM_VDEVS);
			}

			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
		}

	} /* end for */

	if ((num_radios == 1) && (soc->device_id == AR9887_DEVICE_ID) &&
		(num_vdevs > CFG_OL_MAX_VDEVS_AR9887))
		num_vdevs = CFG_OL_MAX_VDEVS_AR9887;

	if ((num_radios == 1) && (soc->device_id == AR9888_DEVICE_ID) &&
		(num_vdevs > CFG_OL_MAX_VDEVS_AR9888))
		num_vdevs = CFG_OL_MAX_VDEVS_AR9888;

	target_if_info("num_vdevs: %d", num_vdevs);
	return num_vdevs;
}

static uint16_t init_deinit_get_total_num_peers_for_soc(struct wlan_objmgr_psoc *psoc,
		uint8_t num_radios)
{
	uint16_t num_peers = 0, pdev_id = 0;
	struct wlan_objmgr_pdev *pdev = NULL;

	for (pdev_id = 0; pdev_id < num_radios; pdev_id++) {
		pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);

		if (pdev) {
			num_peers += wlan_pdev_get_config_params(pdev, WLAN_CONFIG_NUM_PEERS);
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
		}
	} /* end for */

	return num_peers + init_deinit_get_total_num_vdevs_for_soc(psoc, num_radios);
}


static uint8_t init_deinit_get_total_beaconing_vdevs_for_soc(
		struct wlan_objmgr_psoc *psoc, uint8_t num_radios)
{
	uint32_t num_vdevs = 0, pdev_id = 0;
	struct wlan_objmgr_pdev *pdev = NULL;
	target_if_debug("num_radios: %d", num_radios);

	for (pdev_id = 0; pdev_id < num_radios; pdev_id++) {
		pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);
		if (pdev) {
			num_vdevs += wlan_pdev_get_config_params(pdev, WLAN_CONFIG_NUM_VDEVS);
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
		}
	} /* end for */

	return num_vdevs;
}

uint16_t init_deinit_get_total_peers_for_pdev_id(struct wlan_objmgr_psoc *psoc,
		uint16_t pdev_id)
{
	ol_ath_soc_softc_t *soc = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	uint16_t num_peers = 0;
	uint8_t num_vdevs = 0;
	bool is_ema_enabled = false;
	struct ol_ath_softc_net80211 *scn = NULL;

	soc = lmac_get_psoc_feature_ptr(psoc);
	if (!soc) {
		target_if_err("soc is null");
		return 0;
	}

	pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id,
					  WLAN_MLME_NB_ID);
	if (pdev) {
		scn = lmac_get_pdev_feature_ptr(pdev);
		if (!scn) {
			target_if_err("scn is null");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
			return QDF_STATUS_E_FAILURE;
		}

		num_peers = wlan_pdev_get_config_params(pdev,
							WLAN_CONFIG_NUM_PEERS);

		/* Each vdev has a corresponding self peer, hence include them
		 * also for total number of peers for a particular pdev
		 */
		is_ema_enabled = wlan_pdev_nif_feat_ext_cap_get(
			pdev, WLAN_PDEV_FEXT_EMA_AP_ENABLE);

		if (is_ema_enabled) {
			num_vdevs += scn->ema_ctx.num_max_vaps;
		} else {
			num_vdevs += wlan_pdev_get_config_params(
				pdev, WLAN_CONFIG_NUM_VDEVS);
		}

		num_vdevs += wlan_pdev_get_config_params(
			pdev, WLAN_CONFIG_NUM_MONITOR_VAPS);

		if (pdev_id == 0 && (soc->device_id == AR9888_DEVICE_ID) &&
			(num_vdevs > CFG_OL_MAX_VDEVS_AR9888))
			num_vdevs = CFG_OL_MAX_VDEVS_AR9888;

		num_peers = num_peers + num_vdevs;
		wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
	}

	return num_peers;
}

uint8_t init_deinit_get_total_vdevs_for_pdev_id(struct wlan_objmgr_psoc *psoc,
		uint16_t pdev_id)
{
	ol_ath_soc_softc_t *soc = NULL;
	uint8_t num_vdevs = 0;
	struct wlan_objmgr_pdev *pdev = NULL;
	bool is_ema_enabled = false;
	struct ol_ath_softc_net80211 *scn = NULL;

	soc = lmac_get_psoc_feature_ptr(psoc);
	if (!soc) {
		target_if_err("soc is null");
		return 0;
	}

	pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);
	if (pdev) {
		scn = (struct ol_ath_softc_net80211 *) lmac_get_pdev_feature_ptr(pdev);

		if (!scn) {
			target_if_err("scn is null");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
			return 0;
		}

		is_ema_enabled = wlan_pdev_nif_feat_ext_cap_get(
			pdev, WLAN_PDEV_FEXT_EMA_AP_ENABLE);

		if (is_ema_enabled) {
			num_vdevs += scn->ema_ctx.num_max_vaps;
		} else {
			num_vdevs += wlan_pdev_get_config_params(
				pdev, WLAN_CONFIG_NUM_VDEVS);
		}
#ifdef WLAN_FEATURE_11BE_MLO
		if (soc_check_group_for_bridge_vdevs(soc))
			num_vdevs += MLO_MAX_BRIDGE_LINKS_PER_RADIO;
#endif
		num_vdevs += wlan_pdev_get_config_params(
			pdev, WLAN_CONFIG_NUM_MONITOR_VAPS);

		if ((pdev_id == 0) && (soc->device_id == AR9888_DEVICE_ID) &&
			(num_vdevs > CFG_OL_MAX_VDEVS_AR9888))
			num_vdevs = CFG_OL_MAX_VDEVS_AR9888;

		wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
	}

	return num_vdevs;
}

uint8_t init_deinit_get_monitor_vdevs_for_pdev_id(struct wlan_objmgr_psoc *psoc,
		uint16_t pdev_id)
{
	uint8_t num_vdevs = 0;
	struct wlan_objmgr_pdev *pdev = NULL;

	pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);
	if (pdev) {
		num_vdevs = wlan_pdev_get_config_params(pdev, WLAN_CONFIG_NUM_MONITOR_VAPS);
		wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
	}

	return num_vdevs;
}
#if PEER_CACHEING_HOST_ENABLE
static void init_deinit_peer_cache_support(struct wlan_objmgr_psoc *psoc,
			 struct target_psoc_info *tgt_hdl, void *wmi_handle)
{
	struct tgt_info *info;
	uint32_t num_vdevs;
	uint32_t max_active_peers;
	uint32_t low_mem;

	info = (&tgt_hdl->info);

	low_mem = wlan_ucfg_get_config_param(psoc, LOW_MEMSYS);

	num_vdevs = info->wlan_res_cfg.num_vdevs;
	info->wlan_res_cfg.num_peers = ((low_mem) ?
					(CFG_TGT_NUM_QCACHE_PEERS_MAX_LOW_MEM) :
					(CFG_TGT_NUM_QCACHE_PEERS_MAX));
	info->wlan_res_cfg.num_peers += num_vdevs;

	max_active_peers = wlan_ucfg_get_config_param(psoc, MAX_ACTIVE_PEERS);

	if ((max_active_peers) && (!info->max_descs) &&
		(max_active_peers < CFG_TGT_QCACHE_ACTIVE_PEERS)) {
		info->wlan_res_cfg.num_active_peers = max_active_peers;
	} else if (info->max_descs) {
		info->wlan_res_cfg.num_active_peers = max_active_peers;
	} else {
		info->wlan_res_cfg.num_active_peers =
						CFG_TGT_QCACHE_ACTIVE_PEERS;
	}
	info->wlan_res_cfg.num_active_peers += num_vdevs;

	info->wlan_res_cfg.num_tids = info->wlan_res_cfg.num_active_peers * 2;
	target_if_info(
		"Peer Caching Enabled ; num_peers = %d, "
		"num_active_peers = %d num_tids = %d, num_vdevs = %d",

		info->wlan_res_cfg.num_peers,
		info->wlan_res_cfg.num_active_peers,
		info->wlan_res_cfg.num_tids, info->wlan_res_cfg.num_vdevs);
}
#endif

static void init_deinit_update_large_ap_config(
		struct wlan_objmgr_psoc *psoc,
		struct target_psoc_info *tgt_hdl, wmi_unified_t wmi_handle)
{
	struct tgt_info *info;

	info = (&tgt_hdl->info);

	info->wlan_res_cfg.num_peers = CFG_TGT_NUM_PEERS_MAX;

	if ((wmi_service_enabled(wmi_handle, wmi_service_rtt)) &&
		(info->wlan_res_cfg.num_peers > CFG_TGT_NUM_RTT_PEERS_MAX))
		info->wlan_res_cfg.num_peers = CFG_TGT_NUM_RTT_PEERS_MAX;

	/* Make sure that number of peers is not exceeding smart antenna's
	 * MAX suported
	 */
	if ((info->wlan_res_cfg.smart_ant_cap) &&
		(!(wlan_psoc_nif_feat_cap_get(psoc,
				WLAN_SOC_F_LTEU_SUPPORT))))
		info->wlan_res_cfg.num_peers =
			(info->wlan_res_cfg.num_peers >
				CFG_TGT_NUM_SMART_ANT_PEERS_MAX) ?
				CFG_TGT_NUM_SMART_ANT_PEERS_MAX :
					info->wlan_res_cfg.num_peers;

	if ((info->wlan_res_cfg.num_peers * 2) > CFG_TGT_NUM_TIDS_MAX)
		/* one data tid per peer */
		info->wlan_res_cfg.num_tids = info->wlan_res_cfg.num_peers;
	else if ((info->wlan_res_cfg.num_peers * 4) > CFG_TGT_NUM_TIDS_MAX)
		/* two tids per peer */
		info->wlan_res_cfg.num_tids = info->wlan_res_cfg.num_peers * 2;
	else
		/* four tids per peer */
		info->wlan_res_cfg.num_tids = info->wlan_res_cfg.num_peers * 4;

	info->wlan_res_cfg.num_vdevs = init_deinit_get_total_vdevs_for_pdev_id(psoc, 0);

	info->wlan_res_cfg.num_peers += info->wlan_res_cfg.num_vdevs;

	target_if_info("LARGE_AP enabled. num_peers %d, num_vdevs %d, num_tids %d",
			info->wlan_res_cfg.num_peers,
			info->wlan_res_cfg.num_vdevs,
			info->wlan_res_cfg.num_tids);
}

static int init_deinit_get_sta_num(struct wlan_objmgr_psoc *psoc,
			struct target_psoc_info *tgt_hdl,
			int msdu_desc_size, int peer_size)
{
	/* If VoW is enabled, memory for TOTAL_VOW_ALLOCABLE number of
		descriptors are reserved
	 * for VoW statsions. To accomadate this memory, number of peers
		is reduced to 16.
	 * Incase, vow is configured such that it doesn't need all those
		memory, it can be used
	 * to support more stations.
	 */
	int sta_num = 0;
	uint32_t vow_config = wlan_ucfg_get_config_param(psoc, VOW_CONFIG);

	if (lmac_is_target_ar900b(psoc)) {
		/* As of now no need to allocate any extra station for AR900B */
		sta_num = 0;
	} else if (tgt_hdl->info.target_type == TARGET_TYPE_AR9888) {
		int num_vi_sta = VOW_GET_NUM_VI_STA(vow_config);
		int num_vi_desc_per_sta = VOW_GET_DESC_PER_VI_STA(vow_config);
		int total_vow_desc = num_vi_sta * num_vi_desc_per_sta;
		int total_free_desc = TOTAL_VOW_ALLOCABLE - total_vow_desc;
		if (total_free_desc > 0) {
			int bytes_avail = total_free_desc * msdu_desc_size;
			sta_num = bytes_avail / peer_size;
		}
	}
	return sta_num;
}

/*
 * Update target config for VoW feature.
 *  input soc - soc data structure, update target config within this structure
 */
static void init_deinit_update_vow_config(
		struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, wmi_unified_t wmi_handle)
{
	struct tgt_info *info;
	uint8_t peer_caching = 0;

	info = (&tgt_hdl->info);
#if PEER_CACHEING_HOST_ENABLE
	peer_caching = wmi_service_enabled(wmi_handle,
					wmi_service_peer_caching);
#endif
		/*VoW enabled*/
	if (lmac_is_target_ar900b(psoc)) {
		info->wlan_res_cfg.num_vdevs = CFG_TGT_NUM_VDEV_VOW;

		if (peer_caching) {
			/* Configure VoW with Qcache enabled */
			info->wlan_res_cfg.num_peers =
					CFG_TGT_QCACHE_NUM_PEERS_VOW;
			info->wlan_res_cfg.num_active_peers =
					CFG_TGT_NUM_ACTIVE_PEERS_VOW +
						info->wlan_res_cfg.num_vdevs;
			info->wlan_res_cfg.num_tids =
				(2 * info->wlan_res_cfg.num_active_peers);
		} else {
			info->wlan_res_cfg.num_peers = CFG_TGT_NUM_PEERS_VOW +
					info->wlan_res_cfg.num_vdevs;
			info->wlan_res_cfg.num_tids = 2 *
						info->wlan_res_cfg.num_peers;
		}

	} else if (info->target_type == TARGET_TYPE_AR9888) {
		info->wlan_res_cfg.num_vdevs = CFG_TGT_NUM_VDEV_VOW;
		info->wlan_res_cfg.num_peers = CFG_TGT_NUM_PEERS_VOW +
			CFG_TGT_NUM_VDEV_VOW + init_deinit_get_sta_num(psoc,
				tgt_hdl, MSDU_DESC_SIZE, MEMORY_REQ_FOR_PEER);
		info->wlan_res_cfg.num_tids = 2 *
				(info->wlan_res_cfg.num_vdevs +
					info->wlan_res_cfg.num_peers);
		info->wlan_res_cfg.num_wds_entries = CFG_TGT_WDS_ENTRIES_VOW;
	}

	/* Both Host and Firmware has Smart Antenna support */
	if ((info->wlan_res_cfg.smart_ant_cap) &&
		(!(wlan_psoc_nif_feat_cap_get(psoc,
				WLAN_SOC_F_LTEU_SUPPORT))))
		info->wlan_res_cfg.num_peers =
			((info->wlan_res_cfg.num_peers >
				CFG_TGT_NUM_SMART_ANT_PEERS_MAX) ?
				CFG_TGT_NUM_SMART_ANT_PEERS_MAX :
					(info->wlan_res_cfg.num_peers));

	info->wlan_res_cfg.vow_config = wlan_ucfg_get_config_param(
							psoc, VOW_CONFIG);
	target_if_info("VoW Enabled: Num peers = %d Num vdevs = %d Num TIDs = %d",
			info->wlan_res_cfg.num_peers,
			info->wlan_res_cfg.num_vdevs,
			info->wlan_res_cfg.num_tids);
}

static void init_deinit_peer_config(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;
	uint32_t vow_config;
	uint8_t peer_caching = 0;
	uint8_t ratectrl_cache;
	uint8_t iram_tids;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}

	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);
#if PEER_CACHEING_HOST_ENABLE
	peer_caching = wmi_service_enabled(wmi_handle,
					wmi_service_peer_caching);
#endif
	vow_config = wlan_ucfg_get_config_param(psoc, VOW_CONFIG);

	if (peer_caching && !(vow_config >> 16)) {
#if PEER_CACHEING_HOST_ENABLE
		init_deinit_peer_cache_support(psoc, tgt_hdl, wmi_handle);
#endif
	} else {
		ratectrl_cache = wmi_service_enabled(wmi_handle,
						wmi_service_ratectrl_cache);
		iram_tids = wmi_service_enabled(wmi_handle,
						wmi_service_iram_tids);

		if (ratectrl_cache && iram_tids && !(vow_config >> 16)) {
			init_deinit_update_large_ap_config(psoc, tgt_hdl,
							wmi_handle);
		} else if (vow_config>>16) {
			init_deinit_update_vow_config(psoc, tgt_hdl,
							wmi_handle);
		}
	}
}

static void init_deinit_ast_override_config(
		 struct target_psoc_info *tgt_hdl, target_resource_config *tgt_cfg)
{
	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}

	tgt_cfg->ast_1_valid_mask_enable =
		CFG_TGT_AST_VALID_MASK;
	tgt_cfg->ast_2_valid_mask_enable =
		CFG_TGT_AST_VALID_MASK;
	tgt_cfg->ast_3_valid_mask_enable =
		CFG_TGT_AST_VALID_MASK;
	tgt_cfg->ast_0_flow_mask_enable =
		CFG_TGT_AST_0_FLOW_MASK;
	tgt_cfg->ast_1_flow_mask_enable =
		CFG_TGT_AST_1_FLOW_MASK;
	tgt_cfg->ast_2_flow_mask_enable =
		CFG_TGT_AST_2_FLOW_MASK;
	tgt_cfg->ast_3_flow_mask_enable =
		CFG_TGT_AST_3_FLOW_MASK;
	tgt_cfg->ast_tid_high_mask_enable =
		CFG_TGT_AST_TID_HIGH_MASK;
	tgt_cfg->ast_tid_low_mask_enable =
		CFG_TGT_AST_TID_LOW_MASK;
}

static void init_deinit_eapol_minrate_enable(
		struct wlan_objmgr_psoc *psoc,
		struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;
	uint32_t enable_eapol_minrate;
	uint32_t eapol_minrate_ac_select;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is NULL");
		return;
	}
	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);
	enable_eapol_minrate = wlan_ucfg_get_config_param(psoc, EAPOL_MINRATE_SET);
	eapol_minrate_ac_select = wlan_ucfg_get_config_param(psoc, EAPOL_MINRATE_AC_SET);

	if (enable_eapol_minrate) {
		info->wlan_res_cfg.eapol_minrate_set = enable_eapol_minrate;
		info->wlan_res_cfg.eapol_minrate_ac_set = eapol_minrate_ac_select;
	}
}

static void init_deinit_mesh_support_enable(
		struct wlan_objmgr_psoc *psoc,
		struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;
	uint32_t enable_mesh_support;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}
	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);
	enable_mesh_support = wlan_ucfg_get_config_param(psoc, MESH_SUPPORT);

	if (enable_mesh_support &&
		wmi_service_enabled(wmi_handle, wmi_service_mesh)) {
		info->wlan_res_cfg.alloc_frag_desc_for_data_pkt = 1;
		info->wlan_res_cfg.num_vdevs = cfg_get(psoc, CFG_OL_NUM_VDEV_MESH);
	}
}

#if UNIFIED_SMARTANTENNA
static void init_deinit_smart_antenna_enable(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	uint32_t enable_smart_antenna;
	uint32_t sa_validate_sw;
	bool smart_ant_enable;
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;
	uint8_t wmi_sa_sw;
	uint8_t wmi_sa_hw;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}
	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);

	enable_smart_antenna = target_if_sa_api_get_sa_supported(psoc);
	sa_validate_sw = target_if_sa_api_get_validate_sw(psoc);
	wmi_sa_sw = wmi_service_enabled(wmi_handle,
					wmi_service_smart_antenna_sw_support);
	wmi_sa_hw = wmi_service_enabled(wmi_handle,
					wmi_service_smart_antenna_hw_support);

	smart_ant_enable = ((wmi_sa_sw && wmi_sa_hw && enable_smart_antenna) ||
				(wmi_sa_sw && sa_validate_sw));

	if (smart_ant_enable) {
		info->wlan_res_cfg.smart_ant_cap = 1;
		target_if_sa_api_set_enable_sa(psoc, 1);
		QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
			  "Smart Antenna enabled; Host: (%d %d) FW: (%d %d)\n",
			  enable_smart_antenna, sa_validate_sw,
			  wmi_sa_sw, wmi_sa_hw);
	} else {
		QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_DEBUG,
			  "Smart Antenna disabled; Host: (%d %d) FW: (%d %d)\n",
			  enable_smart_antenna, sa_validate_sw,
			  wmi_sa_sw, wmi_sa_hw);
	}
}
#endif

#if WLAN_WIFI_RADAR_ENABLE
static void
init_deinit_wifi_radar_support_enable(struct wlan_objmgr_psoc *psoc,
				      struct target_psoc_info *tgt_hdl,
				      uint8_t *event)
{
	bool is_wifi_radar_supported;
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}
	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);

	is_wifi_radar_supported =
		wmi_service_enabled(wmi_handle,
				    wmi_service_wifi_radar_support);
	target_if_wifi_radar_set_support(psoc, is_wifi_radar_supported);
}
#endif

#if WLAN_CFR_ENABLE
static void init_deinit_cfr_support_enable(struct wlan_objmgr_psoc *psoc,
					   struct target_psoc_info *tgt_hdl,
					   uint8_t *event)
{
	bool is_cfr_supported;
	struct wmi_unified *wmi_handle;
	struct tgt_info *info;
	bool is_cap_interval_mode_sel_support;
	bool is_mo_marking_support;
	bool is_aoa_for_rcc_support;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}
	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);

	is_cfr_supported = wmi_service_enabled(wmi_handle,
				wmi_service_cfr_capture_support);

	if (is_cfr_supported) {
		target_if_cfr_set_cfr_support(psoc, 1);
		is_cap_interval_mode_sel_support =
			wmi_service_enabled(wmi_handle,
					wmi_service_cfr_capture_count_support);
		is_mo_marking_support =
			wmi_service_enabled(wmi_handle,
					wmi_service_cfr_ta_ra_as_fp_support);
		is_aoa_for_rcc_support =
			wmi_service_enabled(wmi_handle,
					wmi_service_aoa_for_rcc_supported);

		target_if_cfr_set_capture_count_support(psoc,
					is_cap_interval_mode_sel_support);
		target_if_cfr_set_mo_marking_support(psoc,
					is_mo_marking_support);
		target_if_cfr_set_aoa_for_rcc_support(psoc,
					is_aoa_for_rcc_support);
	}
}
#endif

#if QCA_AIRTIME_FAIRNESS
#if WLAN_ATF_INCREASED_STA
static void update_atf_max_client_512_cap(struct wlan_objmgr_psoc *psoc,
					  struct wmi_unified *wmi_handle)
{
	if (wmi_service_enabled(wmi_handle,
				wmi_service_atf_max_client_512_support))
		target_if_atf_set_fw_max_client_512_support(psoc, 1);
}
#else
static void update_atf_max_client_512_cap(struct wlan_objmgr_psoc *psoc,
					  struct wmi_unified *wmi_handle)
{
}
#endif

static void init_deinit_atf_config_enable(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	uint8_t atf_mode;
	struct tgt_info *info;
	struct wmi_unified *wmi_handle;
	uint32_t atf_max_vdevs;
	uint32_t atf_peers;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}

	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	info = (&tgt_hdl->info);

	if (!wmi_service_enabled(wmi_handle, wmi_service_atf))
		return;

	/*Need to add host input if support this feature, if yes, add sta
			number configuration for resoures as VoW????*/
	target_if_atf_set_fmcap(psoc, 1);
	atf_mode = target_if_atf_get_mode(psoc);

	if (atf_mode) {
		/*Before here, should add if host need to support this one*/
		info->wlan_res_cfg.atf_config = target_if_atf_get_fmcap(psoc);

		/* If ATF enabled then update FW cap for 512 clients support */
		update_atf_max_client_512_cap(psoc, wmi_handle);

		if (wmi_service_enabled(wmi_handle, wmi_service_peer_caching)) {
			/* For AR900B chips platform association can
			 * be supported for more than num_active_peers. So avoid
			 * the limitation for these platforms.
			 * Note that air time,
			 * however, is guaranteed only up to num_active_peers.
			 */
			if (!lmac_is_target_ar900b(psoc))
				info->wlan_res_cfg.num_peers =
					info->wlan_res_cfg.num_active_peers;
		}
		target_if_info(
			"Airtime Fairness: num_peers=%d num_active_peer=%d",
					info->wlan_res_cfg.num_peers,
					info->wlan_res_cfg.num_active_peers);
		if ((target_psoc_get_target_type(tgt_hdl) == TARGET_TYPE_AR9888)
			&& target_psoc_get_target_ver(tgt_hdl)
						== AR9888_REV2_VERSION) {
			atf_max_vdevs = target_if_atf_get_max_vdevs(psoc);
			atf_peers = target_if_atf_get_peers(psoc);
			if (atf_max_vdevs) {
				info->wlan_res_cfg.num_vdevs = atf_max_vdevs;
				if (atf_peers)
					info->wlan_res_cfg.num_peers =
						atf_peers +
						info->wlan_res_cfg.num_vdevs;
				else
					info->wlan_res_cfg.num_peers =
						CFG_TGT_NUM_PEERS_ATF +
						info->wlan_res_cfg.num_vdevs;
			} else if (atf_peers) {
				info->wlan_res_cfg.num_peers = atf_peers +
					init_deinit_get_total_vdevs_for_pdev_id(psoc, 0);
			} else {
				info->wlan_res_cfg.num_peers =
							CFG_TGT_NUM_PEERS_ATF +
							init_deinit_get_total_vdevs_for_pdev_id(psoc, 0);
			}

			target_if_info("ATF: peers = %d, vdevs = %d ",
					info->wlan_res_cfg.num_peers,
					info->wlan_res_cfg.num_vdevs);
		}
	}
}
#endif

static void init_deinit_qwrap_config_enable(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl)
{
	uint8_t qwrap_enable;
	struct tgt_info *info;
	int num_radios;
	struct wlan_objmgr_pdev *pdev;
	int num_vdevs = 0;
	int num_peers = 0;
	int num_peers_per_pdev = 0;
	int i;

	qwrap_enable = wlan_rptr_psoc_is_max_qwrap(psoc);

	if (!qwrap_enable)
		return;

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}

	num_radios = target_psoc_get_num_radios(tgt_hdl);
	info = (&tgt_hdl->info);

	info->wlan_res_cfg.qwrap_config = qwrap_enable;

	info->wlan_res_cfg.num_vdevs = 0;
	info->wlan_res_cfg.num_peers = 0;

	for(i = 0; i < num_radios; i++) {
		pdev = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_INIT_DEINIT_ID);
		if (pdev == NULL) {
			target_if_err("pdev obj is not allocated");
			return;
		}

#if ATH_SUPPORT_WRAP
		num_vdevs = wlan_rptr_get_qwrap_vdevs_for_pdev_id(psoc, i);
		num_peers = wlan_rptr_get_qwrap_peers_for_pdev_id(psoc, i);
#endif

		if(num_vdevs == 0) {
			target_if_err("vdev count is 0");
			return;
		}

		if(num_peers == 0) {
			target_if_err("peer count is 0");
			return;
		}

		info->wlan_res_cfg.num_vdevs += num_vdevs;
		/* Every sta vdev target need 2 peer ( self & bss beer )
		 * To no of peers in Qwrap =
		 * no of sta vdev (to no of vdev - 1 ap vdev ) * 2 peer
		 * + 1 ap vdev peer
		 * + max wireless peers supported in AP vdev
		 */
		num_peers_per_pdev = ((num_vdevs - 1) * 2) + 1 + num_peers;
		info->wlan_res_cfg.num_peers += num_peers_per_pdev;

		wlan_pdev_set_max_peer_count(pdev, num_peers_per_pdev);
		wlan_pdev_set_max_vdev_count(pdev, num_vdevs);

		wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
	}

	if(info->wlan_res_cfg.num_vdevs > WLAN_UMAC_PSOC_MAX_VDEVS)
		wlan_psoc_set_max_vdev_count(psoc,
					     WLAN_UMAC_PSOC_MAX_VDEVS);
	else
		wlan_psoc_set_max_vdev_count(psoc,
					     info->wlan_res_cfg.num_vdevs);
	wlan_psoc_set_max_peer_count(psoc, info->wlan_res_cfg.num_peers);

	/* Wrt Qwrap, num_peers & num_active_peers are same.
	 * Target recommends host to set the num_active_peers to
	 * 0 and target will reset num_active_peers accordingly
	 * based on target final total num_peers.
	 */
	info->wlan_res_cfg.num_active_peers = 0;

	info->wlan_res_cfg.num_tids = info->wlan_res_cfg.num_peers * 2;

	target_if_info(
		"Qwrap max client mode enabled: num_peers = %d, num_active_peers = %d num_tids = %d, num_vdevs = %d",
		info->wlan_res_cfg.num_peers,
		info->wlan_res_cfg.num_active_peers,
		info->wlan_res_cfg.num_tids, info->wlan_res_cfg.num_vdevs);
}

static void init_deinit_btcoex_config_enable(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	uint32_t val;
	uint32_t period;
	uint32_t duration;

	if (wlan_ucfg_get_btcoex_param(psoc, BTCOEX_ENABLE)) {
		val = WMI_HOST_PDEV_VI_PRIORITY_BIT |
			WMI_HOST_PDEV_BEACON_PRIORITY_BIT |
			WMI_HOST_PDEV_MGMT_PRIORITY_BIT;

		wlan_ucfg_set_btcoex_param(psoc, BTCOEX_WL_PRIORITY, val, 0);

		if (wlan_ucfg_get_btcoex_param(psoc, BTCOEX_DUTY_CYCLE)) {
			period = DEFAULT_PERIOD;
			duration = DEFAULT_WLAN_DURATION;
			wlan_ucfg_set_btcoex_param(psoc,
				BTCOEX_DURATION_PERIOD, duration, period);
		}
	}
}


static void init_deinit_lteu_ext_support_enable(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl, uint8_t *event)
{
	struct tgt_info *info;

	 if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return;
	}

	info = (&tgt_hdl->info);

	if (info->wlan_ext_res_cfg.fw_feature_bitmap &
			WMI_HOST_FW_FEATURE_LTEU_SUPPORT) {
		info->wlan_res_cfg.num_peers = 10;
		info->wlan_res_cfg.num_peers += info->wlan_res_cfg.num_vdevs;
		target_if_info("LTEu enabled. num_peers %d, num_vdevs %d, num_tids %d",
				info->wlan_res_cfg.num_peers,
				info->wlan_res_cfg.num_vdevs,
				info->wlan_res_cfg.num_tids);

	}
}

void init_deinit_overlap_freq_enable(struct wlan_objmgr_psoc *psoc,
                                     uint8_t pdev_id)
{
	struct target_psoc_info *tgt_psoc;
	struct wlan_psoc_host_mac_phy_caps *cap, *cap_arr;
	struct wlan_psoc_host_service_ext_param *svc_param;
	struct wlan_psoc_host_service_ext2_param *svc_ext2_param;
	struct wlan_objmgr_pdev *pdev;
	QDF_STATUS status;

	tgt_psoc = wlan_psoc_get_tgt_if_handle(psoc);

	pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_INIT_DEINIT_ID);
	if (!pdev) {
		target_if_err("pdev obj is not allocated");
		return;
	}

	cap_arr = target_psoc_get_mac_phy_cap(tgt_psoc);
	cap = &cap_arr[pdev_id];

	if (cap && cap->supported_bands == WMI_HOST_WLAN_5G_CAPABILITY) {
		svc_param = target_psoc_get_service_ext_param(tgt_psoc);
		svc_ext2_param = target_psoc_get_service_ext2_param(tgt_psoc);

		if (!svc_param || !svc_ext2_param) {
			qdf_err("service ext param is NULL!");
			goto exit;
		}

		/*
		 * check if freq. is given in svc ext2 event
		 * if so, compare the values with values in svc ext
		 */
		if (svc_ext2_param->low_5ghz_chan_ext && svc_ext2_param->high_5ghz_chan_ext) {

			if (cfg_get(psoc, CFG_RF_SWITCH_CONFIG) == SECONDARY_RF_BAND) {
				/* copy values from ext2 event */
				status = ucfg_reg_update_hal_reg_range_caps(psoc,
								     svc_ext2_param->low_2ghz_chan_ext,
								     svc_ext2_param->high_2ghz_chan_ext,
								     svc_ext2_param->low_5ghz_chan_ext,
								     svc_ext2_param->high_5ghz_chan_ext,
								     pdev_id);
				if (QDF_IS_STATUS_ERROR(status)) {
					target_if_err("Failed to update hal reg cap");
					wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
					return;
				}
			} else {
				/* copy values from ext event */
				status = ucfg_reg_update_hal_reg_range_caps(psoc,
								     svc_param->low_2ghz_chan,
								     svc_param->high_2ghz_chan,
								     svc_param->low_5ghz_chan,
								     svc_param->high_5ghz_chan,
								     pdev_id);
				if (QDF_IS_STATUS_ERROR(status)) {
					target_if_err("Failed to update hal reg cap");
					wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
					return;
				}
			}
			wlan_pdev_nif_feat_ext_cap_set(pdev,
						       WLAN_PDEV_FEXT_OVERLAPPING_FREQ);
			qdf_info("Overlapping frequency support enabled!");
		}
	} /* supported_bands == WMI_HOST_WLAN_5G_CAPABILITY */

exit:
	wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
	return;
}

static void init_deinit_afc_config_params(struct wlan_objmgr_psoc *psoc)
{
	if (psoc == NULL) {
		target_if_err("PSOC is NULL");
		return;
	}

	ucfg_reg_set_enable_6ghz_sp_mode_support(psoc,
		    wlan_ucfg_get_config_param(psoc, SP_6GHZ_PWRMODE_SUPP));
	ucfg_reg_set_afc_disable_timer_check(psoc,
		    wlan_ucfg_get_config_param(psoc, AFC_TIMER_CHECK_DIS));
	ucfg_reg_set_afc_disable_request_id_check(psoc,
		    wlan_ucfg_get_config_param(psoc, AFC_REQ_ID_CHECK_DIS));
	ucfg_reg_set_afc_no_action(psoc,
		    wlan_ucfg_get_config_param(psoc, AFC_REG_NO_ACTION));
}

static void init_deinit_set_init_cmd_dev_params(
		struct wlan_objmgr_psoc *psoc,
		struct target_psoc_info *tgt_hdl)
{

	struct tgt_info *info;
	target_resource_config *tgt_cfg;
	uint8_t num_vdevs = 0;

	if (!tgt_hdl) {
		targetif_nofl_info("psoc target_psoc_info is null");
		return;
	}

	info = (&tgt_hdl->info);

	tgt_cfg = &info->wlan_res_cfg;

	num_vdevs = init_deinit_get_total_vdevs_for_pdev_id(psoc, 0);

	if (info->target_type == TARGET_TYPE_AR9888) {
		tgt_cfg->tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_3SS;
		tgt_cfg->rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_3SS;
		tgt_cfg->num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR988X;
		tgt_cfg->ast_skid_limit = CFG_TGT_AST_SKID_LIMIT_AR988X;
	} else if (info->target_type == TARGET_TYPE_AR900B ||
			info->target_type == TARGET_TYPE_QCA9984) {
		tgt_cfg->num_vdevs = num_vdevs;
		/* need to reserve an additional peer for each VDEV */
		tgt_cfg->num_peers = CFG_TGT_NUM_PEERS + num_vdevs;
		tgt_cfg->num_tids += num_vdevs;
		tgt_cfg->tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_4SS;
		tgt_cfg->rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_4SS;
	} else if (info->target_type == TARGET_TYPE_QCA9888) {
		tgt_cfg->num_vdevs = num_vdevs;
		/* need to reserve an additional peer for each VDEV */
		tgt_cfg->num_peers = CFG_TGT_NUM_PEERS + num_vdevs;
		tgt_cfg->num_tids += num_vdevs;
		tgt_cfg->tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_2SS;
		tgt_cfg->rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_2SS;
	}

#if QCA_AIRTIME_FAIRNESS
	if (target_if_atf_get_mode(psoc) &&
		(target_psoc_get_target_type(tgt_hdl) == TARGET_TYPE_AR9888 &&
		target_psoc_get_target_ver(tgt_hdl) == AR9888_REV2_VERSION))
		tgt_cfg->num_msdu_desc =
			target_if_atf_get_num_msdu_desc(psoc);
#endif

	if (!wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_BCN_OFFLOAD))
		tgt_cfg->beacon_tx_offload_max_vdev = 0;

	if ((info->target_type == TARGET_TYPE_QCA8074) ||
		(info->target_type == TARGET_TYPE_QCA8074V2) ||
		(info->target_type == TARGET_TYPE_QCA9574) ||
		(info->target_type == TARGET_TYPE_QCN9000) ||
		(info->target_type == TARGET_TYPE_QCN9224) ||
		(info->target_type == TARGET_TYPE_QCN6122) ||
		(info->target_type == TARGET_TYPE_QCA5332) ||
		(info->target_type == TARGET_TYPE_QCN9160) ||
		(info->target_type == TARGET_TYPE_QCA5018) ||
		(info->target_type == TARGET_TYPE_QCA6018) ||
		(info->target_type == TARGET_TYPE_QCN6432))
		tgt_cfg->scheduler_params = wlan_ucfg_get_config_param(
							psoc, SCHED_PARAMS);

	info->wlan_ext_res_cfg.fw_feature_bitmap |=
			WMI_HOST_FW_FEATURE_BSS_CHANNEL_INFO_64;

#ifdef ATHR_WIN_NWF
	tgt_cfg->rx_decap_mode = CFG_TGT_RX_DECAP_MODE_NWIFI;
#else
	tgt_cfg->rx_decap_mode = CFG_TGT_RX_DECAP_MODE;

#endif
	tgt_cfg->rx_batchmode = 1;

	/* Allow FW to send target pdev id in VDEV start and multi-vdev
	 * restart responses
	 */
	tgt_cfg->use_pdev_id = 1;

	/*
	 * To make the IP header begins at dword aligned address,
	 * we make the decapsulation mode as Native Wifi.
	 */
	if (wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_HOST_80211_ENABLE))
		tgt_cfg->rx_decap_mode = CFG_TGT_RX_DECAP_MODE_NWIFI;

	/* Set the Min buffer free for each AC, from the module param values */
	tgt_cfg->BK_Minfree = wlan_ucfg_get_config_param(psoc, AC_BK_MINFREE);
	tgt_cfg->BE_Minfree = wlan_ucfg_get_config_param(psoc, AC_BE_MINFREE);
	tgt_cfg->VI_Minfree = wlan_ucfg_get_config_param(psoc, AC_VI_MINFREE);
	tgt_cfg->VO_Minfree = wlan_ucfg_get_config_param(psoc, AC_VI_MINFREE);

	targetif_nofl_debug
		("AC Minfree buf allocation through module param (umac.ko)");
	targetif_nofl_debug
		(" OL_ACBKMinfree : %d", tgt_cfg->BK_Minfree);
	targetif_nofl_debug
		(" OL_ACBEMinfree : %d", tgt_cfg->BE_Minfree);
	targetif_nofl_debug
		(" OL_ACVIMinfree : %d", tgt_cfg->VI_Minfree);
	targetif_nofl_debug
		(" OL_ACVOMinfree : %d", tgt_cfg->VO_Minfree);

	/* Configuring IP header padding, from module param value */
	if (lmac_is_target_ar900b(psoc))
		tgt_cfg->iphdr_pad_config = wlan_ucfg_get_config_param(psoc,
						IPHDR_PAD);
	else
		tgt_cfg->iphdr_pad_config = 0;

#if PEER_CACHEING_HOST_ENABLE
	if (!((info->target_type == TARGET_TYPE_QCA8074) ||
		(info->target_type == TARGET_TYPE_QCA8074V2) ||
		(info->target_type == TARGET_TYPE_QCA9574) ||
		(info->target_type == TARGET_TYPE_QCA6018) ||
		(info->target_type == TARGET_TYPE_QCA5332) ||
		(info->target_type == TARGET_TYPE_QCA5018) ||
		(info->target_type == TARGET_TYPE_QCN6122) ||
		(info->target_type == TARGET_TYPE_QCN9160) ||
		(info->target_type == TARGET_TYPE_QCN9224) ||
		(info->target_type == TARGET_TYPE_QCN9000) ||
		(info->target_type == TARGET_TYPE_QCN6432)))
		tgt_cfg->num_active_peers = 0;
#endif
	tgt_cfg->cce_disable = wlan_ucfg_get_config_param(psoc, CCE_STATE);
	tgt_cfg->re_ul_resp = wlan_ucfg_get_config_param(psoc, RE_UL_RESP);

	targetif_nofl_debug("num_peers %d, num_vdevs %d, num_tids %d",
		tgt_cfg->num_peers, tgt_cfg->num_vdevs, tgt_cfg->num_tids);

	init_deinit_afc_config_params(psoc);

	return;
}

QDF_STATUS init_deinit_alloc_pdevs(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl)
{
	struct tgt_info *info;
	uint16_t i;
	qdf_netdev_t pdev_netdev;
	struct pdev_osif_priv *osdev_priv;
	struct wlan_objmgr_pdev *pdev_umac;
	QDF_STATUS ret_val;
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
	uint32_t bypasswmi;
	QDF_STATUS is_service_ext_msg = QDF_STATUS_E_FAILURE;
	struct cdp_pdev_attach_params params = {0};
	bool scan_radio_support = false;
	cdp_config_param_type val = {0};

	if (!tgt_hdl) {
		target_if_err("psoc target_psoc_info is null");
		return QDF_STATUS_E_FAILURE;
	}

	info = (&tgt_hdl->info);
	bypasswmi = wlan_ucfg_get_config_param(psoc, BYPASSWMI);
	is_service_ext_msg = init_deinit_is_service_ext_msg(psoc, tgt_hdl);
	mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);

	if (!mac_phy_cap)
	    target_if_err("mac_phy cap is null");
        /*
         * Make num_radios 1 if user configures hw mode as 0.
         * This is needed for Napier as Napier does not support DBDC and
         * preferred_hw_mode 0 is considered as an invalid mode in cmn code.
         */
        if (target_psoc_get_preferred_hw_mode(tgt_hdl) == WMI_HOST_HW_MODE_SINGLE)
            target_psoc_set_num_radios(tgt_hdl, 1);

	for (i = 0; i < target_psoc_get_num_radios(tgt_hdl); i++) {
		pdev_umac = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_INIT_DEINIT_ID);
		if (pdev_umac != NULL) {
			wlan_objmgr_pdev_release_ref(pdev_umac, WLAN_INIT_DEINIT_ID);
			continue;
		}

		pdev_netdev = wlan_create_pdev_netdevice(psoc, i);
		if (pdev_netdev == NULL) {
			target_if_err("Cannot allocate softc");
			return QDF_STATUS_E_NOMEM;
		}

		osdev_priv = qdf_mem_malloc(sizeof(struct pdev_osif_priv));
		if (osdev_priv == NULL) {
			target_if_err("OSIF private member allocation failed");
			return QDF_STATUS_E_NOMEM;
		}

		osdev_priv->legacy_osif_priv = (void *)netdev_priv(pdev_netdev);

		pdev_umac = wlan_objmgr_pdev_obj_create(psoc, osdev_priv);
		if (pdev_umac == NULL) {
			target_if_err("PDEV (creation) failed");
			qdf_mem_free(osdev_priv);
			osdev_priv = NULL;
			return QDF_STATUS_E_FAILURE;
		}

		ret_val = target_if_alloc_pdev_tgt_info(pdev_umac);
		if (ret_val != QDF_STATUS_SUCCESS) {
			target_if_err("pdev tgt alloc failed");
			return ret_val;
		}
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
		if (mac_phy_cap && ol_ath_mlo_capable(psoc)) {
			target_pdev_set_hw_link_id(pdev_umac,
						   mac_phy_cap[i].hw_link_id);
			params.mlo_link_id = mac_phy_cap[i].hw_link_id;
		} else {
			target_pdev_set_hw_link_id(pdev_umac,
						   PDEV_INVALID_HW_LINK_ID);
			params.mlo_link_id = PDEV_INVALID_HW_LINK_ID;
		}
#endif

		wlan_pdev_update_feature_ptr(psoc, pdev_umac, pdev_netdev);

		ret_val = target_pdev_is_scan_radio_supported(pdev_umac,
							      &scan_radio_support);
		if (QDF_IS_STATUS_ERROR(ret_val))
			target_if_err("Error: %u Failed to read scan radio support", ret_val);
		val.cdp_scan_radio_support = scan_radio_support;
		cdp_txrx_set_psoc_param(wlan_psoc_get_dp_handle(psoc),
					CDP_SCAN_RADIO_SUPPORT, val);
		if (!bypasswmi) {
			if (ol_target_lithium(psoc)) {
				/*set LMAC ID in pdev before attach*/
				if ((is_service_ext_msg == QDF_STATUS_SUCCESS)
				    && mac_phy_cap) {
					cdp_soc_map_pdev_to_lmac(
						wlan_psoc_get_dp_handle(psoc),
						i, mac_phy_cap[i].lmac_id);
				}
				params.htc_handle = target_psoc_get_htc_hdl(tgt_hdl);
				params.qdf_osdev = wlan_psoc_get_qdf_dev(psoc);
				params.pdev_id = i;

				if (QDF_STATUS_SUCCESS != cdp_pdev_attach(
						wlan_psoc_get_dp_handle(psoc),
						&params)) {
					target_if_err("CDP PDEV ATTACH failed");
					return QDF_STATUS_E_FAILURE;
				}
				target_if_info("CDP PDEV ATTACH success");
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
				if (mac_phy_cap) {
					wlan_mlo_read_tsf2_offset(psoc,
						wlan_objmgr_pdev_get_pdev_id(pdev_umac),
						mac_phy_cap[i].lmac_id);
				}
#endif
			}
		}
	}
	return QDF_STATUS_SUCCESS;
}

static bool init_deinit_set_per_pdev_mbssid_config(
		struct wlan_objmgr_psoc *psoc) {
	uint8_t pdev_idx;
	struct ieee80211com *ic;
	struct wlan_objmgr_pdev *pdev;
	struct ol_ath_softc_net80211 *scn;
	struct ol_ath_soc_softc *soc;
	struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;
	struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
	struct target_psoc_info *tgt_hdl;
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
	struct wmi_unified *wmi_handle;
	/* extract MBSS enable/disable */
	uint32_t mbss_ie_enable;
	uint16_t ema_ap_enable;
	bool is_ema_ap = false;
	/* does pdev support 6Ghz frequency? */
	bool is_6ghz_pdev = false;
	bool is_fw_mbss_support = false;
	bool is_fw_ema_support  = false;
	bool is_fw_ema_multigroup_support  = false;
	bool is_fw_large_beacon_support  = false;
	uint32_t low_freq = 0, high_freq = 0;

	target_if_debug(":>");

	if (!psoc) {
		target_if_err("psoc is null");
		goto OUT;
	}

	reg_cap = ucfg_reg_get_hal_reg_cap(psoc);
	if (!reg_cap) {
		target_if_err("%s: invalid reg_cap", __func__);
		goto OUT;
	}

	tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null");
		goto OUT;
	}

	mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);

	if (!mac_phy_cap) {
		target_if_err("mac_phy_cap is null");
		goto OUT;
	}

	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
	if (!wmi_handle) {
		target_if_err("wmi_handle is null");
		goto OUT;
	}

	is_fw_mbss_support = wlan_psoc_nif_fw_ext_cap_get(psoc,
					WLAN_SOC_CEXT_MBSS_IE);

	is_fw_ema_support = wmi_service_enabled(wmi_handle,
				wmi_service_ema_ap_support);

	target_if_debug("is_fw_mbss_support: %d, is_fw_ema_support %d",
				is_fw_mbss_support, is_fw_ema_support);

	is_fw_ema_multigroup_support = wmi_service_enabled(wmi_handle,
				wmi_service_ema_multiple_group_supported);

	is_fw_large_beacon_support = wmi_service_enabled(wmi_handle,
				wmi_service_large_beacon_supported);

	target_if_debug("is_fw_ema_multigroup_support: %d, is_fw_large_beacon_support %d",
				is_fw_ema_multigroup_support, is_fw_large_beacon_support);


	/* retrieve reg_rx_ops */
	reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);

	for (pdev_idx = 0;
		pdev_idx < target_psoc_get_num_radios(tgt_hdl); pdev_idx++) {

		pdev = wlan_objmgr_get_pdev_by_id(psoc,
			       	pdev_idx, WLAN_MLME_NB_ID);
		if (!pdev) {
			target_if_err("pdev object (id: %d) is NULL", pdev_idx);
			continue;
		}

		scn = (struct ol_ath_softc_net80211 *)
		lmac_get_pdev_feature_ptr(pdev);
		if (!scn) {
			target_if_err("scn is NULL");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
			continue;
		}

		ic = &(scn->sc_ic);
		if (!ic) {
			target_if_err("ic is NULL");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
			continue;
		}

		soc = scn->soc;

		mbss_ie_enable = soc->ema_ctx.feature_config;
		/* mbss_ie_enable is a 32 bit config. The lower 16
		 * bit of this config controls enable/disable of the
		 * MBSSID feature. EMA gets enabled when MBSSID is
		 * enabled. The upper 16 bit of this config can
		 * change this default behavior to disable ema
		 */
		ema_ap_enable  = mbss_ie_enable >> 16;

		target_if_debug("mbss_ie_enable: 0x%x", mbss_ie_enable);
		target_if_debug("ema_ap_enable: 0x%x", ema_ap_enable);
		target_if_debug("soc->disable_6ghz_mbssid: %d",
						scn->ema_ctx.disable_6ghz_mbssid);

		low_freq = reg_cap[pdev_idx].low_5ghz_chan;
		high_freq = reg_cap[pdev_idx].high_5ghz_chan;

		/* is the pdev 6ghz only capable */
		if (mac_phy_cap[pdev_idx].supported_bands &
			WMI_HOST_WLAN_5G_CAPABILITY && reg_rx_ops) {
			is_6ghz_pdev = reg_rx_ops->reg_is_range_overlap_6g(
							low_freq, high_freq);
		}

		if (!is_fw_large_beacon_support &&
			(soc->ema_ctx.max_mgmt_frm_sz > mgmt_frm_size_1_5k)) {
			target_if_err("Large beacon not supported. !!Configured max_mgmt_frm_sz:%d",
			soc->ema_ctx.max_mgmt_frm_sz);
		}

		if (is_6ghz_pdev) {
			/* register psoc to rnr module for id mapping */
			if (!wlan_rnr_register_soc(soc)) {
				target_if_err("SOC registration to RNR module failed");
				wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
				continue;
			}

			/*
			 * Force EMA in wideband radio to maximize reusing the
			 * current framework of resource config allocation in
			 * the WMI init cmd and to have minimal change in the
			 * initialization path in the host. A generic approach
			 * for the same can be taken in future
			 */
			if (reg_rx_ops->reg_is_range_overlap_6g(
						low_freq, high_freq) &&
			    !(reg_rx_ops->reg_is_range_only6g(
						low_freq, high_freq))) {
				qdf_print("Wideband radio detected - "
					  "forcing EMA");
				scn->ema_ctx.disable_6ghz_mbssid = false;
			}

			if (!scn->ema_ctx.disable_6ghz_mbssid) {
				target_if_info("pdev supporting 6ghz");

				/* enable irrespective of whether the
				 * corresponding bit is set or not in INI
				 */
				if (!is_fw_mbss_support) {
					target_if_err("MBSS IE feature disabled but"
						      "	soc supports 6Ghz pdev");
					qdf_assert_always(0);
				}

				wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
						      WLAN_PDEV_F_MBSS_IE_ENABLE);
				target_if_info("MBSSID feature enabled "
					       "for 6ghz pdev  - pdev_idx: %d",
					       pdev_idx);

				if (!is_fw_ema_support) {
					target_if_err("FW does not support EMA AP!");
				} else {
					/* For 6ghz ema ap support is mandatory */
					wlan_pdev_nif_feat_ext_cap_set(ic->ic_pdev_obj,
						 WLAN_PDEV_FEXT_EMA_AP_ENABLE);
					target_if_info(
						"EMA AP feature enabled for 6Ghz pdev"
						" - pdev_idx: %d", pdev_idx);

					/* set is_ema_ap to true if any of the pdev
					 * supports ema */
					is_ema_ap = true;
				}

                soc->max_rnr_ie_allowed =
                    GET_RNR_IE_COUNT_FROM_SIZE(scn->ema_ctx.rnr_field_size_limit);
			} /* disable_6Ghz_mbssid */
		} else {
			target_if_debug("Not a 6ghz pdev");

			if (is_fw_mbss_support &&
				(((mbss_ie_enable & 0xffff)
				>> pdev_idx) & 1)) {
				wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
					      WLAN_PDEV_F_MBSS_IE_ENABLE);
				target_if_info("MBSSID feature enabled for "
						"pdev_idx: %d", pdev_idx);

				/* pdev_idxth bit indicates EMA AP is
				 * disabled or not for that pdev. value
				 * 1 means 'disable'
				 */
				if (!is_fw_ema_support) {
					target_if_err(
						"FW does not support EMA AP");
				} else if (!(ema_ap_enable >> pdev_idx) & 1) {
					wlan_pdev_nif_feat_ext_cap_set(
						ic->ic_pdev_obj,
						WLAN_PDEV_FEXT_EMA_AP_ENABLE);
					target_if_info("EMA AP feature enabled"
						" for pdev_idx: %d", pdev_idx);

					/* set is_ema_ap to true if any of
					 * the pdev supports ema */
					is_ema_ap = true;
				}
			} /* end if */
		}

		if (pdev)
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
	} /* end for */

OUT:
	target_if_debug("<:%d", is_ema_ap);
	return is_ema_ap;
}

/*
 * API to update scan radio related pdev capability flags
 */
static QDF_STATUS
init_deinit_update_scan_radio_caps(struct wlan_objmgr_pdev *pdev)
{
	QDF_STATUS status;
	bool scan_radio_support = false;

	status = target_pdev_is_scan_radio_supported(pdev,
						     &scan_radio_support);
	if (QDF_IS_STATUS_ERROR(status)) {
		target_if_err("Failed to check scan radio capability");
		return QDF_STATUS_E_FAILURE;
	}

	if (scan_radio_support) {
		bool scan_radio_dfs_en = true;
		bool blanking_en = false;

		wlan_pdev_nif_feat_ext_cap_set(pdev,
					       WLAN_PDEV_FEXT_SCAN_RADIO);

		status = target_pdev_scan_radio_is_dfs_enabled(
				pdev, &scan_radio_dfs_en);
		if (QDF_IS_STATUS_ERROR(status)) {
			target_if_err("Failed to check dfs enabled");
			return QDF_STATUS_E_FAILURE;
		}

		if (!scan_radio_dfs_en)
			wlan_pdev_nif_feat_ext_cap_set(
				pdev,
				WLAN_PDEV_FEXT_SCAN_RADIO_DFS_DIS);
		else
			wlan_pdev_nif_feat_ext_cap_clear(
				pdev,
				WLAN_PDEV_FEXT_SCAN_RADIO_DFS_DIS);

		status = target_is_scan_blanking_enabled(pdev, &blanking_en);
		if (QDF_IS_STATUS_ERROR(status)) {
			target_if_err("Failed to check scan blanking enabled");
			return QDF_STATUS_E_FAILURE;
		}
		if (blanking_en)
			wlan_pdev_nif_feat_ext_cap_set(
				pdev,
				WLAN_PDEV_FEXT_SCAN_BLANKING_EN);
		else
			wlan_pdev_nif_feat_ext_cap_clear(
				pdev,
				WLAN_PDEV_FEXT_SCAN_BLANKING_EN);
	} else {
		wlan_pdev_nif_feat_ext_cap_clear(
				pdev,
				WLAN_PDEV_FEXT_SCAN_RADIO);
		wlan_pdev_nif_feat_ext_cap_clear(
				pdev,
				WLAN_PDEV_FEXT_SCAN_RADIO_DFS_DIS);
		wlan_pdev_nif_feat_ext_cap_clear(
				pdev,
				WLAN_PDEV_FEXT_SCAN_BLANKING_EN);
	}

	return QDF_STATUS_SUCCESS;
}

static void init_deinit_parse_scanradio_pdev_section(
			 struct wlan_objmgr_psoc *psoc,
                         char *ini_buf)
{
	cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_SCAN_RADIO_SECTION);
}

#ifdef QCA_LOWMEM_CONFIG
static void init_deinit_parse_256M_pdev_section(
			 struct wlan_objmgr_psoc *psoc,
			 char *ini_buf,
			 uint8_t pdev_band)
{
	switch(pdev_band) {
	case WLAN_6GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_6G_SECTION);
		break;
	case WLAN_6GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_6G_LOW_SECTION);
		break;
	case WLAN_6GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_6G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_5G_LOW_SECTION);
		break;
	case WLAN_5GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_5G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_5G_SECTION);
		break;
	case WLAN_2GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_256M_2G_SECTION);
		break;
	default:
		break;
	}
}
#elif defined QCA_512M_CONFIG
#ifdef QCA_IPQ807X_E_BUILD
static void init_deinit_parse_512ME_pdev_section(
			 struct wlan_objmgr_psoc *psoc,
			 char *ini_buf,
			 uint8_t pdev_band)
{
	switch(pdev_band) {
	case WLAN_6GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_6G_SECTION);
		break;
	case WLAN_6GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_6G_LOW_SECTION);
		break;
	case WLAN_6GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_6G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_5G_LOW_SECTION);
		break;
	case WLAN_5GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_5G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_5G_SECTION);
		break;
	case WLAN_2GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512ME_2G_SECTION);
		break;
	default:
		break;
	}
}
#else
static void init_deinit_parse_512MP_pdev_section(
			 struct wlan_objmgr_psoc *psoc,
			 char *ini_buf,
			 uint8_t pdev_band)
{
	switch(pdev_band) {
	case WLAN_6GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_6G_SECTION);
		break;
	case WLAN_6GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_6G_LOW_SECTION);
		break;
	case WLAN_6GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_6G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_5G_LOW_SECTION);
		break;
	case WLAN_5GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_5G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_5G_SECTION);
		break;
	case WLAN_2GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_512MP_2G_SECTION);
		break;
	default:
		break;
	}
}
#endif
#else
static void init_deinit_parse_reduced_1G_pdev_section(
			 struct wlan_objmgr_psoc *psoc,
			 char *ini_buf,
			 uint8_t pdev_band)
{
	switch(pdev_band) {
	case WLAN_6GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_6G_SECTION);
		break;
	case WLAN_6GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_6G_LOW_SECTION);
		break;
	case WLAN_6GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_6G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_5G_LOW_SECTION);
		break;
	case WLAN_5GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_5G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_5G_SECTION);
		break;
	case WLAN_2GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_1G_2G_SECTION);
		break;
	default:
		break;
	}
}

static void init_deinit_parse_pdev_section(
			 struct wlan_objmgr_psoc *psoc,
			 char *ini_buf,
			 uint8_t pdev_band)
{
	switch(pdev_band) {
	case WLAN_6GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_6G_SECTION);
		break;
	case WLAN_6GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_6G_LOW_SECTION);
		break;
	case WLAN_6GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_6G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_LOW_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_5G_LOW_SECTION);
		break;
	case WLAN_5GHZ_HIGH_BAND_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_5G_HIGH_SECTION);
		break;
	case WLAN_5GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_5G_SECTION);
		break;
	case WLAN_2GHZ_PDEV:
		cfg_section_parse_to_psoc_store(psoc, ini_buf, CFG_2G_SECTION);
		break;
	default:
		break;
	}
}
#endif

static void init_deinit_parse_soc_phy_mode_section(
			 struct wlan_objmgr_psoc *psoc,
			 char *ini_buf,
			 uint8_t phy_mode)
{
	switch(phy_mode) {
	case WLAN_SOC_SINGLE_PHY_2G:
		cfg_section_parse_to_psoc_store(psoc, ini_buf,
						CFG_SOC_SINGLE_PHY_2G_SECTION);
		break;
	case WLAN_SOC_SINGLE_PHY_5G:
		cfg_section_parse_to_psoc_store(psoc, ini_buf,
						CFG_SOC_SINGLE_PHY_5G_SECTION);
		break;
	case WLAN_SOC_SINGLE_PHY_6G:
		cfg_section_parse_to_psoc_store(psoc, ini_buf,
						CFG_SOC_SINGLE_PHY_6G_SECTION);
		break;
	case WLAN_SOC_SPLIT_PHY_2G_5G_LOW:
		cfg_section_parse_to_psoc_store
			(psoc, ini_buf,	CFG_SOC_SPLIT_PHY_2G_5G_LOW_SECTION);
		break;
	case WLAN_SOC_SPLIT_PHY_6G_5G_HIGH:
		cfg_section_parse_to_psoc_store
			(psoc, ini_buf,	CFG_SOC_SPLIT_PHY_6G_5G_HIGH_SECTION);
		break;
	default:
		break;
	}
}

void init_deinit_ema_init(struct wlan_objmgr_psoc *psoc, struct wlan_objmgr_pdev *pdev)
{
	struct target_psoc_info *tgt_hdl;

	tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null");
		return;
	}

	if ((tgt_hdl->tif_ops) &&
	    (tgt_hdl->tif_ops->ema_init))
		return tgt_hdl->tif_ops->ema_init(pdev);

	return;
}

static void init_deinit_update_ini_params(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl)
{
	uint8_t num_radios;
	uint8_t pdev_idx;
	struct wlan_objmgr_pdev *pdev;
	uint32_t low_freq = 0, high_freq = 0;
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
	struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;
	struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
	struct ini_file ini;
	uint32_t target_type = 0;
	char ini_buf[128] = "internal/";
	uint32_t ret;
	bool section_parse = false;
	uint8_t soc_bands_bit_map = 0;

	reg_cap = ucfg_reg_get_hal_reg_cap(psoc);
	if (!reg_cap) {
		target_if_err("%s: invalid reg_cap", __func__);
		return;
	}

	mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);
	if (!mac_phy_cap) {
		target_if_err("%s: invalid mac_phy_cap", __func__);
		return;
	}

	target_type = target_psoc_get_target_type(tgt_hdl);

	ret = cfg_get_ini_file_name(target_type, &ini);

	if (QDF_IS_STATUS_SUCCESS(ret)) {
		if (strlcat(ini_buf, ini.internal, sizeof(ini_buf))
						   < sizeof(ini_buf)) {
			section_parse = true;
		} else {
			qdf_err("INI target file derivation failed for section parse");
		}
	}

	/* retrieve reg_rx_ops */
	reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
	num_radios = target_psoc_get_num_radios(tgt_hdl);

	for (pdev_idx = 0; pdev_idx < num_radios; pdev_idx++) {
		bool is_6ghz_pdev = false;
		uint8_t pdev_band = WLAN_PDEV_BAND_UNKNOWN;

		pdev = wlan_objmgr_get_pdev_by_id(psoc,
				pdev_idx, WLAN_MLME_NB_ID);
		if (!pdev) {
			target_if_debug("pdev object (id: %d) is NULL", pdev_idx);
			continue;
		}

		low_freq = reg_cap[pdev_idx].low_5ghz_chan;
		high_freq = reg_cap[pdev_idx].high_5ghz_chan;

		if (mac_phy_cap[pdev_idx].supported_bands &
			WMI_HOST_WLAN_2G_CAPABILITY) {
			pdev_band = WLAN_2GHZ_PDEV;
		}

		/* is the pdev 6ghz only capable */
		if (mac_phy_cap[pdev_idx].supported_bands &
			WMI_HOST_WLAN_5G_CAPABILITY && reg_rx_ops) {
			is_6ghz_pdev = reg_rx_ops->reg_is_range_overlap_6g(
							low_freq, high_freq);
			if (is_6ghz_pdev) {
				if (high_freq < WLAN_LOW_6G_BAND_HIGHIEST_FREQ)
					pdev_band = WLAN_6GHZ_LOW_BAND_PDEV;
				else if (low_freq >= WLAN_HIGH_6G_BAND_LOWEST_FREQ)
					pdev_band = WLAN_6GHZ_HIGH_BAND_PDEV;
				else
					pdev_band = WLAN_6GHZ_PDEV;
			}
		}

		if (!is_6ghz_pdev && (mac_phy_cap[pdev_idx].supported_bands &
			WMI_HOST_WLAN_5G_CAPABILITY)) {
			if (high_freq < WLAN_LOW_5G_BAND_HIGHIEST_FREQ)
				pdev_band = WLAN_5GHZ_LOW_BAND_PDEV;
			else if (low_freq >= WLAN_HIGH_5G_BAND_LOWEST_FREQ)
				pdev_band = WLAN_5GHZ_HIGH_BAND_PDEV;
			else
				pdev_band = WLAN_5GHZ_PDEV;
		}

		if (section_parse) {
			bool is_scan_radio_supported = 0;
			if (target_pdev_is_scan_radio_supported(pdev, &is_scan_radio_supported) != QDF_STATUS_SUCCESS)
				target_if_err("Failed to check scan radio capability");
			if (is_scan_radio_supported)
				init_deinit_parse_scanradio_pdev_section(psoc, ini_buf);
			else {
#ifdef QCA_LOWMEM_CONFIG
				init_deinit_parse_256M_pdev_section(psoc, ini_buf, pdev_band);
#elif defined QCA_512M_CONFIG
#ifdef QCA_IPQ807X_E_BUILD
				init_deinit_parse_512ME_pdev_section(psoc, ini_buf, pdev_band);
#else
				init_deinit_parse_512MP_pdev_section(psoc, ini_buf, pdev_band);
#endif
#else
				if (cfg_get(psoc, CFG_TGT_USE_REDUCED_NUM_VAP_PEER))
					init_deinit_parse_reduced_1G_pdev_section(psoc, ini_buf, pdev_band);
				else
					init_deinit_parse_pdev_section(psoc, ini_buf, pdev_band);
#endif
			}
		}

		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_VDEVS, cfg_get(psoc, CFG_OL_NUM_VDEVS));
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_PEERS, cfg_get(psoc, CFG_OL_NUM_PEERS));
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_MONITOR_VAPS, cfg_get(psoc, CFG_OL_MONITOR_VAPS));
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_QWRAP_VDEVS, cfg_get(psoc, CFG_OL_QWRAP_VDEVS));
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_QWRAP_PEERS, cfg_get(psoc, CFG_OL_QWRAP_PEERS));
#if defined(IPA_OFFLOAD) && defined(IPA_OFFLOAD_LOW_MEM)
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_IPA_RX_BUFF,
					    cfg_get(psoc, CFG_OL_NUM_IPA_RX_BUFF));
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_NUM_IPA_TX_BUFF,
					    cfg_get(psoc, CFG_OL_NUM_IPA_TX_BUFF));
#endif
#if QCA_PPE_VP
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_VP_CORE_MASK, cfg_get(psoc, CFG_DP_VP_CORE_MASK));
#endif
#if QCA_STANDALONE_SOUNDING_TRIGGER
		wlan_pdev_set_config_params(pdev, WLAN_CONFIG_HOST_TRIGGER_TXBF_SOUNDING,
                               cfg_get(psoc, CFG_OL_HOST_TRIG_TXBF_SOUNDING));
#endif
#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
		wlan_pdev_set_config_params(
			pdev, WLAN_CONFIG_TX_PKT_CAP_CUSTOM_CLASSIFY_PROTO,
			cfg_get(psoc, CFG_OL_TX_PKT_CAP_CUSTOM_CLASSIFY_PROTO));
#endif

		if (pdev)
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);

		soc_bands_bit_map |= (1 << pdev_band);
		init_deinit_ema_init(psoc, pdev);
	}

	init_deinit_parse_soc_phy_mode_section(psoc, ini_buf, soc_bands_bit_map);
}

/* The below max num TID value is derived for 2 radios. As of now, target
 * accepts max num_tids as 8540.
 * max_num_tids =  (8 * (num_peers_pdev_0 + num_peers_pdev_1) +
 *                  4 * num_vdevs_pdev_0 + 8)
 */
#define MAX_NUM_TIDS 8540
QDF_STATUS init_deinit_update_pdev_tgt_info(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl)
{
	struct tgt_info *info;
	struct target_pdev_info *pdev_tgt_info;
	struct wlan_objmgr_pdev *pdev;
	uint16_t i;
	uint16_t num_radios;
	void *wmi_handle;
	uint32_t bypasswmi;
	QDF_STATUS is_service_ext_msg = QDF_STATUS_E_FAILURE;
	QDF_STATUS status;
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
	ol_ath_soc_softc_t *soc;
	uint16_t num_peers = 0;
	uint8_t num_vdevs = 0;
	uint8_t num_monitor_vdevs = 0;
	int mode;
	ol_txrx_soc_handle soc_txrx_handle;
	dp_txrx_pdev_handle_t *dp_ext_hdl;
	bool is_ema_ap;
	uint32_t max_ngroups = 0;
	uint32_t num_max_vaps = 0;
	struct cdp_pdev_attach_params params = {0};

	if (!psoc) {
		target_if_err("psoc is null");
		return QDF_STATUS_E_FAILURE;
	}
	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null");
		return QDF_STATUS_E_FAILURE;
	}

	soc = lmac_get_psoc_feature_ptr(psoc);
	if (!soc) {
		target_if_err("soc is null");
		return QDF_STATUS_E_FAILURE;
	}

	info = (&tgt_hdl->info);
	is_service_ext_msg = init_deinit_is_service_ext_msg(psoc, tgt_hdl);

	bypasswmi = wlan_ucfg_get_config_param(psoc, BYPASSWMI);

	num_radios = target_psoc_get_num_radios(tgt_hdl);
	mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);

	if (!mac_phy_cap)
	    target_if_err("mac_phy cap is null");

	init_deinit_update_ini_params(psoc, tgt_hdl);
	mode =  target_psoc_get_preferred_hw_mode(tgt_hdl);

	/* enable mbssid and ema ap for the pdevs supported
	* in this psoc based on INI and FW support config
	*/
	is_ema_ap = init_deinit_set_per_pdev_mbssid_config(psoc);

	info->wlan_res_cfg.num_vdevs =
		init_deinit_get_total_num_vdevs_for_soc(psoc, num_radios);

	/*
	 * For lithium chipsets, host directly reads the num peers from
	 * ini and sends it to FW.
	 * For pre-lithium chipsets, host calculates the number of
	 * peers based on large_ap config, Mesh support, ATF config and
	 * QWRAP config.
	 */
	if (ol_target_lithium(psoc))
		info->wlan_res_cfg.num_peers =
			init_deinit_get_total_num_peers_for_soc(psoc, num_radios);

	if (ol_ath_is_beacon_offload_enabled(soc))
		info->wlan_res_cfg.beacon_tx_offload_max_vdev =
			init_deinit_get_total_beaconing_vdevs_for_soc(psoc, num_radios);
	else
		info->wlan_res_cfg.beacon_tx_offload_max_vdev = 0;


	/*
	 * For pre-lithium chipsets, host calculates the number of tids based
	 * on large_ap config, Mesh support, ATF config and QWRAP config.
	 */
	if (ol_target_lithium(psoc)) {
		info->wlan_res_cfg.num_tids =
			((8 * init_deinit_get_total_num_peers_for_soc(psoc, num_radios))+
			 (4 * init_deinit_get_total_num_vdevs_for_soc(psoc, 1)) + 8);

		if (info->wlan_res_cfg.num_tids > MAX_NUM_TIDS)
			info->wlan_res_cfg.num_tids = MAX_NUM_TIDS;
	}

	for (i = 0; i < num_radios; i++) {
		/*set LMAC ID in pdev before attach*/
		if ((is_service_ext_msg == QDF_STATUS_SUCCESS) && mac_phy_cap) {
			cdp_soc_map_pdev_to_lmac(
					wlan_psoc_get_dp_handle(psoc),
					i, mac_phy_cap[i].lmac_id);
		}
	}

	for (i = 0; i < num_radios; i++) {
		struct ol_ath_softc_net80211 *scn;
		pdev = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_INIT_DEINIT_ID);
		if (pdev == NULL) {
			target_if_err("pdev obj is not allocated");
			return QDF_STATUS_E_FAILURE;
		}

		scn = (struct ol_ath_softc_net80211 *)lmac_get_pdev_feature_ptr(pdev);
		if (scn == NULL) {
			target_if_err("pdev obj is not allocated");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
			return QDF_STATUS_E_FAILURE;
		}

		max_ngroups += scn->ema_ctx.max_ngroups;

		if (num_max_vaps < scn->ema_ctx.num_max_vaps) {
			num_max_vaps = scn->ema_ctx.num_max_vaps;
		}

		pdev_tgt_info = wlan_pdev_get_tgt_if_handle(pdev);
		if (pdev_tgt_info == NULL) {
			target_if_err("pdev tgt info is NULL");
		        wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
			return QDF_STATUS_E_FAILURE;
		}

		if ((is_service_ext_msg == QDF_STATUS_SUCCESS) && mac_phy_cap) {
			pdev_tgt_info->phy_idx  = mac_phy_cap[i].phy_id;
			pdev_tgt_info->pdev_idx = mac_phy_cap[i].pdev_id;
			if(mode == WMI_HOST_HW_MODE_2G_PHYB && num_radios == 1)
				pdev_tgt_info->pdev_idx = 0;
		} else {
			pdev_tgt_info->phy_idx = i;
			pdev_tgt_info->pdev_idx = 0;
		}
		target_if_debug("object manager pdev id %d, tgt pdev id %d",
					i, pdev_tgt_info->pdev_idx);

		wmi_handle = wmi_unified_get_pdev_handle(
				wmi_unified_get_soc_handle(
				(struct wmi_unified *)target_psoc_get_wmi_hdl(tgt_hdl)), i);
		if (wmi_handle == NULL) {
			target_if_err("Failed to get pdev wmi_handle");
		        wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
			return QDF_STATUS_E_FAILURE;
		} else {
		    qdf_debug("pdev id:%d wmi_handle:%pK", pdev->pdev_objmgr.wlan_pdev_id,
                            wmi_handle);
		}

		target_pdev_set_wmi_handle(pdev_tgt_info, wmi_handle);

		if (!bypasswmi) {
			if (ol_target_lithium(psoc)) {
				status = cdp_pdev_init(
						wlan_psoc_get_dp_handle(psoc),
						target_psoc_get_htc_hdl(tgt_hdl),
						wlan_psoc_get_qdf_dev(psoc), i);
				if (status == QDF_STATUS_E_FAILURE) {
					soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
					qdf_info("soc_txrx_handle %pK pdev_id: %u", soc_txrx_handle, i);
					cdp_pdev_detach(soc_txrx_handle, i, 0);
					target_if_err("CDP PDEV INIT failed");
					wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
					return QDF_STATUS_E_FAILURE;
				}
			} else {
				params.htc_handle = target_psoc_get_htc_hdl(tgt_hdl);
				params.qdf_osdev = wlan_psoc_get_qdf_dev(psoc);
				params.pdev_id = i;
				if (QDF_STATUS_SUCCESS != cdp_pdev_attach(
								wlan_psoc_get_dp_handle(psoc),
								&params)) {
					target_if_err("CDP PDEV ATTACH failed");
					wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
					return QDF_STATUS_E_FAILURE;
				}
			}

			dp_ext_hdl = qdf_mem_malloc(sizeof(dp_txrx_pdev_handle_t));
			cdp_pdev_set_dp_txrx_handle(
					wlan_psoc_get_dp_handle(psoc),
					i, dp_ext_hdl);
#if ATH_SUPPORT_WRAP
			if (dp_ext_hdl) {
				dp_wrap_attach(pdev);
			}
#endif
			if (dp_ext_hdl) {
				dp_extap_attach(pdev);
			}
			dp_pdev_me_setup(wlan_psoc_get_dp_handle(psoc),
					 wlan_objmgr_pdev_get_pdev_id(pdev));

			init_deinit_overlap_freq_enable(psoc, i);
		}

		/* Set max peers, vdevs for supported for each PDEV.
		 * For lithium chipsets, host directly reads the num peers and
		 * vdevs from ini and sends it to FW.
		 * For pre-lithium chipsets, host calculates the number of
		 * peers and vdevs based on large_ap config, Mesh support, ATF
		 * config and QWRAP config. Also, pre-lithium chipset has only
		 * one radio per psoc. Therefore set max peer count from
		 * wlan_res_config.
		 */
		if (ol_target_lithium(psoc)) {
			num_peers = init_deinit_get_total_peers_for_pdev_id(psoc, i);
			num_vdevs = init_deinit_get_total_vdevs_for_pdev_id(psoc, i);
		} else {
			num_peers = info->wlan_res_cfg.num_peers;
			num_vdevs = info->wlan_res_cfg.num_vdevs;
		}

		wlan_pdev_set_max_peer_count(pdev, num_peers);
		wlan_pdev_set_max_vdev_count(pdev, num_vdevs);

		/* Set max monitor vaps supported for each PDEV */
		num_monitor_vdevs = init_deinit_get_monitor_vdevs_for_pdev_id(psoc, i);
		wlan_pdev_set_max_monitor_vdev_count(pdev, num_monitor_vdevs);

#ifdef WLAN_FEATURE_11BE_MLO
		if (soc_check_group_for_bridge_vdevs(soc))
			wlan_pdev_set_max_bridge_vdev_count(pdev, MLO_MAX_BRIDGE_LINKS_PER_RADIO);
#endif

		qdf_debug("Pdev = %d Number of peers = %d vdevs = %d", i,
				wlan_pdev_get_max_peer_count(pdev),
				wlan_pdev_get_max_vdev_count(pdev));

		status = init_deinit_update_scan_radio_caps(pdev);
		if (QDF_IS_STATUS_ERROR(status)) {
			target_if_err("Failed to update scan radio caps");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
			return QDF_STATUS_E_FAILURE;
		}

		wlan_objmgr_pdev_release_ref(pdev, WLAN_INIT_DEINIT_ID);
		target_if_info("CDP PDEV INIT success");

	}

	/* Max bssid indicator is a power of 2 value
	 * which when raised to power of 2 indicates
	 * the size of the mbssid-group. If the
	 * configured value of num_max_vaps is
	 * not a value represented by 2 raised to some
	 * pwoer of 2 then we must bump the max bss
	 * indicator value up such that the size of
	 * the mbssid-group represented by this indi-
	 * cator value covers configured value of
	 * max vaps in case of ema ap
	 */
	if (num_max_vaps &&
		(num_max_vaps >
		(1 << info->service_ext_param.max_bssid_indicator))) {
		/* Due to 1500 byte buffer management frame size
		 * limitation fw advertisez max bssid indicator
		 * as 3 currently - max 8 vaps supported in case
		 * of mbssid mode. But we can overcome the limi-
		 * tation using ema mode and in that case the max
		 * bssid indicator value can grow beyond 3. We
		 * allow up to 4 as that sufficies our current
		 * system limitation of 16 vaps
		 */
		uint16_t max_bss_ind = 1;

		/* round ema_ctx->num_max_vaps up to power of 2
		 * and save in max_bss_ind temporarily to be used
		 * in below section
		 */
		while (max_bss_ind < num_max_vaps)
			max_bss_ind <<= 1;

		if (max_bss_ind && ilog2(max_bss_ind) <= 4) {
			max_bss_ind = ilog2(max_bss_ind);
			info->wlan_res_cfg.max_bssid_indicator = max_bss_ind;
		} else {
			info->wlan_res_cfg.max_bssid_indicator = 4;
		}
	} else {
		info->wlan_res_cfg.max_bssid_indicator =
			info->service_ext_param.max_bssid_indicator;
	}

	if (is_ema_ap) {
		mgmt_frm_size_t max_mgmt_frm_sz;

		info->wlan_res_cfg.ema_max_vap_cnt = max_ngroups;
		info->wlan_res_cfg.ema_max_profile_period = soc->ema_ctx.max_pp;

		if (soc->ema_ctx.max_mgmt_frm_sz == mgmt_frm_size_2_0k) {
		/* Though an intermediate step of 2K exists (between 1.5K
		* and 2.3K in host, FW still operates in 3 possible steps
		* 1.5K, 2.5K and 4K. The intermediate step of 2K provides
		* the flexibility to have a default configuration that
		* does not exceed the current WFA approved limit of 1.84K.
		*/
			max_mgmt_frm_sz = mgmt_frm_size_2_3k;
		} else {
			max_mgmt_frm_sz = soc->ema_ctx.max_mgmt_frm_sz;
		}
		info->wlan_res_cfg.ema_init_config = max_mgmt_frm_sz & 0xffff;
	}

	wlan_psoc_set_max_vdev_count(psoc, info->wlan_res_cfg.num_vdevs);
	wlan_psoc_set_max_peer_count(psoc, info->wlan_res_cfg.num_peers);

	init_deinit_qwrap_config_enable(psoc, tgt_hdl);

	cdp_txrx_intr_attach(wlan_psoc_get_dp_handle(psoc));

	/* Add delay before sending wmi_init to ensure pdev attach is
	 * reflected
	 */
	qdf_mdelay(100);

	return QDF_STATUS_SUCCESS;
}



#if WIFI_MEM_MANAGER_SUPPORT
uint32_t init_deinit_mem_mgr_alloc_chunk(struct wlan_objmgr_psoc *psoc,
			struct target_psoc_info *tgt_hdl,
			u_int32_t req_id, u_int32_t idx, u_int32_t num_units,
			u_int32_t unit_len, u_int32_t num_unit_info)
{
	qdf_dma_addr_t paddr;
	struct tgt_info *info;
	qdf_device_t qdf_dev;
	uint32_t soc_id;
	uint32_t target_type;

	info = (&tgt_hdl->info);

	if (!num_units  || !unit_len)  {
		return 0;
	}

	qdf_dev = wlan_psoc_get_qdf_dev(psoc);
	soc_id = wlan_ucfg_get_config_param(psoc, SOC_ID);
	tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
	target_type = target_psoc_get_target_type(tgt_hdl);

	info->mem_chunks[idx].vaddr = NULL ;

	if (req_id == IRAM_BKP_PADDR_REQ_ID &&
	    (target_type == TARGET_TYPE_QCA9984 ||
	     target_type == TARGET_TYPE_AR900B  ||
	     target_type == TARGET_TYPE_QCA9888)) {
		/*
		 * req_id for the address of target iram backup in the host.
		 * There is no need for any m/r allocation for this req_id.
		 * Only the paddr for the FW iram bkp in host needs to be
		 * passed to FW.
		 */
		idx = info->num_mem_chunks;
		info->mem_chunks[idx].paddr = wlan_ucfg_get_config_param(psoc,
							 TGT_IRAM_BKP_ADDR);
		info->mem_chunks[idx].vaddr = NULL;
		info->mem_chunks[idx].len = 0;
		info->mem_chunks[idx].req_id = 8;
		info->num_mem_chunks += 1;
		goto done;
	}

	/* reduce the requested allocation by half until allocation succeeds */
	while (info->mem_chunks[idx].vaddr == NULL && num_units) {
		int intr_ctxt = (in_interrupt() || irqs_disabled()) ? 1 : 0;
		info->mem_chunks[idx].vaddr =
			(uint32_t *) wifi_cmem_allocation(soc_id, 0,
				(CM_FWREQ + req_id), num_units*unit_len,
				(void *)qdf_dev->drv_hdl,
				&paddr, intr_ctxt);
		if (info->mem_chunks[idx].vaddr == NULL) {
			if (num_unit_info & HOST_CONTIGUOUS_MEM_CHUNK_REQUIRED)
				return 0;
			else  /* reduce length by half */
				num_units = (num_units >> 1);
		} else {
			info->mem_chunks[idx].paddr = paddr;
			info->mem_chunks[idx].len = num_units*unit_len;
			info->mem_chunks[idx].req_id =  req_id;
		}
	}
done:
	target_if_info(
		"req_id %d idx %d num_units %d unit_len %d",
		req_id, idx, num_units, unit_len);

	return num_units;
}


static QDF_STATUS init_deinit_mem_mgr_free_chunks(struct wlan_objmgr_psoc *psoc,
			struct target_psoc_info *tgt_hdl)
{
	struct tgt_info *info;
	uint32_t soc_id;
	uint32_t idx;

	info = (&tgt_hdl->info);
	soc_id = wlan_ucfg_get_config_param(psoc, SOC_ID);

	for (idx=0; idx< info->num_mem_chunks; ++idx) {
		wifi_cmem_free(soc_id, 0, info->mem_chunks[idx].req_id,
				(void *)info->mem_chunks[idx].vaddr);
	}
	info->num_mem_chunks = 0;

	return QDF_STATUS_SUCCESS;
}

#endif

#ifdef WLAN_FEATURE_11BE
void init_deinit_print_11be_support(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    targetif_nofl_info("11BE suport %x ",mac_phy_cap->supports_11be);
}
#else
void init_deinit_print_11be_support(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
}
#endif /* WLAN_FEATURE_11BE */

void init_deinit_print_service_ready_ex_params(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl)
{
	struct wlan_psoc_host_service_ext_param *service_ext_param;
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
	struct tgt_info *info;
	uint16_t i;
	uint8_t hdl_major_version;
	uint8_t hdl_minor_version;
	bool eht_capab = false;

	if (!tgt_hdl) {
		targetif_nofl_err("target_psoc_info is null");
		return;
	}

	if (!psoc) {
		targetif_nofl_err("psoc is null");
		return;
	}

	info = (&tgt_hdl->info);

	service_ext_param = &info->service_ext_param;

	/* HDL Major version: 0-5 bits of fw_build_vers_ext */
	hdl_major_version = service_ext_param->fw_build_vers_ext & ((1 << 6) - 1);

	/* HDL Minor version: 6-9 bits of fw_build_vers_ext */
	hdl_minor_version = (service_ext_param->fw_build_vers_ext >> 6) & ((1 << 4) -1);

	targetif_nofl_info("HE_CAP Info = %x", service_ext_param->he_cap_info);
	targetif_nofl_info("Num HW modes = %d",
		service_ext_param->num_hw_modes);
	targetif_nofl_info("Num PHY = %d", service_ext_param->num_phy);
	targetif_nofl_info("FW Build Version (Ext.) = %d", service_ext_param->fw_build_vers_ext);
	targetif_nofl_info("HDL Version: MajorVersion.MinorVersion = %d.%d",
		hdl_major_version, hdl_minor_version);
	targetif_nofl_debug("HW mode and MAC PHY CAP");
	mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);
	if (!mac_phy_cap)
	    target_if_err("mac_phy cap is null");

	for (i = 0;
		i < target_psoc_get_num_radios(tgt_hdl) && mac_phy_cap; i++) {
		targetif_nofl_debug
		("idx = %d hw_mode_id = %d pdev_id = %d phy_id = %d",
			i, mac_phy_cap->hw_mode_id, mac_phy_cap->pdev_id,
			mac_phy_cap->phy_id);
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
		targetif_nofl_debug(" hw_link_id = %d",
				    mac_phy_cap->hw_link_id);
#endif

		targetif_nofl_debug(" 11G/A/N/AC/AX  support = %x/%x/%x/%x/%x ",
			mac_phy_cap->supports_11g, mac_phy_cap->supports_11a,
			mac_phy_cap->supports_11n, mac_phy_cap->supports_11ac,
			mac_phy_cap->supports_11ax);

		wlan_psoc_mlme_get_11be_capab(psoc, &eht_capab);
		if (eht_capab) {
			init_deinit_print_11be_support(mac_phy_cap);
		}

		targetif_nofl_debug(" Supported bands = %x ",
					mac_phy_cap->supported_bands);
		targetif_nofl_debug(" Ampdu density = %d",
					mac_phy_cap->ampdu_density);

		if (mac_phy_cap->supported_bands &
			WMI_HOST_WLAN_2G_CAPABILITY) {
			targetif_nofl_debug(" Max BW supported 2G = %d ",
					mac_phy_cap->max_bw_supported_2G);
			targetif_nofl_debug(" 2G - HT cap info = %x ",
					mac_phy_cap->ht_cap_info_2G);
			targetif_nofl_debug(" 2G - VHT cap info = %x ",
					mac_phy_cap->vht_cap_info_2G);
			targetif_nofl_debug(" 2G - Supported VHT MCS =%x ",
					mac_phy_cap->vht_supp_mcs_2G);
			targetif_nofl_debug(" 2G - HE cap[0] = %x HE cap[1] = %x ",
					mac_phy_cap->he_cap_info_2G[0],
					mac_phy_cap->he_cap_info_2G[1]);
			targetif_nofl_debug(" 2G - HE supp MCS = %x ",
					mac_phy_cap->he_supp_mcs_2G);
			targetif_nofl_debug(" 2G - TX chain mask = %x ",
					mac_phy_cap->tx_chain_mask_2G);
			targetif_nofl_debug(" 2G - RX chain mask = %d",
					mac_phy_cap->rx_chain_mask_2G);
			targetif_nofl_debug(
			" HE_CAP_PHY_INFO_2G[0] = %x HE_CAP_PHY_INFO_2G[1]=%x HE_CAP_PHY_INFO_2G [2]=%x ",
					mac_phy_cap->he_cap_phy_info_2G[0],
					mac_phy_cap->he_cap_phy_info_2G[1],
					mac_phy_cap->he_cap_phy_info_2G[2]);
			targetif_nofl_debug(
			" 2G - HE_PPET5G_NUMSS_M1 =%d HE_PPET5G_RU_MASK=%x",
					mac_phy_cap->he_ppet2G.numss_m1,
					mac_phy_cap->he_ppet2G.ru_bit_mask);
		}

		if (mac_phy_cap->supported_bands &
			WMI_HOST_WLAN_5G_CAPABILITY) {
			targetif_nofl_debug(" 5G - Max BW supported = %d ",
					mac_phy_cap->max_bw_supported_5G);
			targetif_nofl_debug(" 5G - HT cap info = %x ",
					mac_phy_cap->ht_cap_info_5G);
			targetif_nofl_debug(" 5G - VHT cap info = %x ",
					mac_phy_cap->vht_cap_info_5G);
			targetif_nofl_debug(" 5G - Supported VHT MCS %x",
					mac_phy_cap->vht_supp_mcs_5G);
			targetif_nofl_debug(" 5G - HE cap[0] = %x HE cap[1] = %x ",
					mac_phy_cap->he_cap_info_5G[0],
					mac_phy_cap->he_cap_info_5G[1]);
			targetif_nofl_debug(" 5G - HE supp MCS = %x ",
					mac_phy_cap->he_supp_mcs_5G);
			targetif_nofl_debug(" 5G - tx chain mask = %x",
					mac_phy_cap->tx_chain_mask_5G);
			targetif_nofl_debug(" 5G - RX chain mask = %d",
					mac_phy_cap->rx_chain_mask_5G);
			targetif_nofl_debug(
			" HE_CAP_PHY_INFO_5G[0] = %x HE_CAP_PHY_INFO_5G[1]= %x HE_CAP_PHY_INFO_5G [2]=%x",
					mac_phy_cap->he_cap_phy_info_5G[0],
					mac_phy_cap->he_cap_phy_info_5G[1],
					mac_phy_cap->he_cap_phy_info_5G[2]);
			targetif_nofl_debug
			(" 5G - HE_PPET5G_NUMSS_M1 = %d HE_PPET5G_RU_MASK=%x",
				mac_phy_cap->he_ppet5G.numss_m1,
				mac_phy_cap->he_ppet5G.ru_bit_mask);
		}
		targetif_nofl_debug(" chain mask tableid= %d",
					mac_phy_cap->chainmask_table_id);
		mac_phy_cap++;
	}
	targetif_nofl_info("Preferred HW Mode = %d Num Radios = %d",
		info->preferred_hw_mode, target_psoc_get_num_radios(tgt_hdl));
}

#ifdef QCA_HOST_ADD_11AX_MODE_WAR
/* WAR: For lithium chipsets, Host receives the wireless modes only till
 * 11ACVHT80_80. This function is added as a WAR to populate 11AX wireless
 * modes. This is only a temporary mechanism. In future, some more flags
 * will be added as part of next specification. Do not copy/modify this
 * function to include those flags. Instead, FW should send those flags
 * as part of service ready related event such as service ready ext/ext2
 * or other suitable event defined for such purposes.
 */
static void add_11ax_mode_flags(
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
	uint64_t *i_wireless_modes)
{
	uint32_t wireless_modes = *i_wireless_modes;
	target_if_warn("11AX wireless modes are not populated by the target in reg capabilities, adding 11AX wireless modes");

	/* 11ax 2.4 GHz */

	/* We check to ensure max BW supported is not 5/10 MHz, just as an
	 * additional precaution.
	 */
	if ((mac_phy_cap->supported_bands & WMI_HOST_WLAN_2G_CAPABILITY)
		&& (mac_phy_cap->max_bw_supported_2G != WMI_HOST_CHAN_WIDTH_5)
		&& (mac_phy_cap->max_bw_supported_2G !=
					WMI_HOST_CHAN_WIDTH_10)) {
		if ((mac_phy_cap->max_bw_supported_2G >=
					WMI_HOST_CHAN_WIDTH_20) &&
			(wireless_modes &
					HOST_REGDMN_MODE_11NG_HT20)) {
			wireless_modes |=
					HOST_REGDMN_MODE_11AXG_HE20;
		}

		if (mac_phy_cap->max_bw_supported_2G >=
				WMI_HOST_CHAN_WIDTH_40) {
			if (wireless_modes &
					HOST_REGDMN_MODE_11NG_HT40PLUS)
				wireless_modes |=
					HOST_REGDMN_MODE_11AXG_HE40PLUS;

			if (wireless_modes &
					HOST_REGDMN_MODE_11NG_HT40MINUS)
				wireless_modes |=
					HOST_REGDMN_MODE_11AXG_HE40MINUS;
		}
	}

	/* 11ax 5 GHz */

	/* We check to ensure max BW supported is not 5/10 MHz, just as an
	 * additional precaution.
	 */
	if ((mac_phy_cap->supported_bands & WMI_HOST_WLAN_5G_CAPABILITY)
		&& (mac_phy_cap->max_bw_supported_5G != WMI_HOST_CHAN_WIDTH_5)
		&& (mac_phy_cap->max_bw_supported_5G !=
				WMI_HOST_CHAN_WIDTH_10)) {
		if ((mac_phy_cap->max_bw_supported_5G >= WMI_HOST_CHAN_WIDTH_20)
		   && (wireless_modes &
			HOST_REGDMN_MODE_11AC_VHT20)) {
			wireless_modes |=
						HOST_REGDMN_MODE_11AXA_HE20;
		}

		if (mac_phy_cap->max_bw_supported_5G >=
						WMI_HOST_CHAN_WIDTH_40) {
			if (wireless_modes &
					HOST_REGDMN_MODE_11AC_VHT40PLUS)
				wireless_modes |=
					HOST_REGDMN_MODE_11AXA_HE40PLUS;
			if (wireless_modes &
					HOST_REGDMN_MODE_11AC_VHT40MINUS)
				wireless_modes |=
					HOST_REGDMN_MODE_11AXA_HE40MINUS;
		}

		if ((mac_phy_cap->max_bw_supported_5G >=
					WMI_HOST_CHAN_WIDTH_80) &&
			(wireless_modes &
				HOST_REGDMN_MODE_11AC_VHT80)) {
			wireless_modes |=
				HOST_REGDMN_MODE_11AXA_HE80;
		}

		if ((mac_phy_cap->max_bw_supported_5G >=
					WMI_HOST_CHAN_WIDTH_160) &&
			(wireless_modes &
					HOST_REGDMN_MODE_11AC_VHT160)) {
			wireless_modes |=
				HOST_REGDMN_MODE_11AXA_HE160;
		}

		if ((mac_phy_cap->max_bw_supported_5G >=
					WMI_HOST_CHAN_WIDTH_80P80) &&
			(wireless_modes &
					HOST_REGDMN_MODE_11AC_VHT80_80)) {
			wireless_modes |=
				HOST_REGDMN_MODE_11AXA_HE80_80;
		}
	}

    *i_wireless_modes = wireless_modes;
}
#else
static void add_11ax_mode_flags(
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
	uint64_t *i_wireless_modes)
{
	target_if_warn("11AX modes are not added to wireless modes in the reg capability. Asserting");
	qdf_assert_always(0);
}
#endif /* QCA_HOST_ADD_11AX_MODE_WAR */

static void init_deinit_add_11ax_modes(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl)
{
	struct tgt_info *info;
	uint16_t i, idx, j;
	uint32_t phy_id;
	uint8_t  num_radios, num_hw_modes;
	uint32_t num_phy_reg_cap;
	/* per-phy reg_cap info from WMI_HAL_REG_CAPABILITIES_EXT */
	struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
	/* per-mode per-phy reg_cap info from WMI_MAC_PHY_CAPABILITIES */
	struct wlan_psoc_host_hal_reg_cap_ext *reg_cap_ext;
	struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;

	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null");
		return;
	}

	reg_cap = ucfg_reg_get_hal_reg_cap(psoc);
	if (reg_cap == NULL) {
		target_if_err("reg cap is NULL");
		return;
	}

	info = (&tgt_hdl->info);

	num_phy_reg_cap = info->service_ext_param.num_phy;
	num_radios      = target_psoc_get_num_radios(tgt_hdl);

	/* num_radios corresponding to current hw-mode mac_phy_cap
	* entries can't exceed num_phy_reg_cap which is correspondig
	* to hal_reg_cap_ext tlvs
	*/
	qdf_assert(num_radios <= num_phy_reg_cap);

	mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);
	if (!mac_phy_cap) {
		target_if_err("mac_phy cap is NULL!");
	}

	/* For lithium chipsets, FW should provide 11AX wireless modes. Since
	 * older FW didn't provide this info, use host WAR to populate it within
	 * host.
	 */
	for (i = 0; i < num_phy_reg_cap && mac_phy_cap; i++) {
		phy_id = reg_cap[i].phy_id;
		for( j = 0 ; j < num_phy_reg_cap ; j++ ) {
			if ((mac_phy_cap[j].phy_id == phy_id) && mac_phy_cap[j].supports_11ax &&
			    ((reg_cap[i].wireless_modes & WIRELESS_MODES_11AX) == 0)) {
				add_11ax_mode_flags(&mac_phy_cap[j],
						&reg_cap[i].wireless_modes);
				target_if_info("phy_id = %d wireless modes = %llx",
						phy_id,	(&reg_cap[i])->wireless_modes);
			}
		}
	}

	/* fill 11ax mode in per-mode per-phy reg_cap info
	* corresponding to per-mode per-phy mac_phy_tlvs
	*/
	num_hw_modes = info->hw_modes.num_modes;
	for (i = 0; i < num_hw_modes; i++) {
		num_radios  = target_psoc_get_num_radios_for_mode
				(tgt_hdl, info->hw_modes.hw_mode_ids[i]);
		mac_phy_cap = target_psoc_get_mac_phy_cap_for_mode
				(tgt_hdl, info->hw_modes.hw_mode_ids[i]);

		if (mac_phy_cap) {
			for (idx = 0; idx < num_radios; idx++) {
			    reg_cap_ext = &mac_phy_cap[idx].reg_cap_ext;
				if (mac_phy_cap[idx].supports_11ax &&
				    (reg_cap_ext->wireless_modes & WIRELESS_MODES_11AX) == 0) {
					add_11ax_mode_flags(&mac_phy_cap[idx],
							&reg_cap_ext->wireless_modes);
				}

			    target_if_info("hw_mode_id = %d phy_id = %d"
					   " wireless modes = %llx",
					    info->hw_modes.hw_mode_ids[i],
					    mac_phy_cap[idx].phy_id,
					    reg_cap_ext->wireless_modes);
			}
		}
	}
}

#ifndef REMOVE_PKT_LOG
static void init_deinit_set_pktlog_checksum(struct wlan_objmgr_pdev *pdev,
					    uint32_t checksum)
{
	wlan_pktlog_set_checksum(pdev, checksum);
}
#endif

/* Target config used for Hawkeye */
target_resource_config  tgt_cfg_qca8074 = {
	.num_offload_peers = CFG_TGT_NUM_OFFLOAD_PEERS_QCA8074,
	.num_offload_reorder_buffs = CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS_QCA8074,
	.num_peer_keys = CFG_TGT_NUM_PEER_KEYS,
	.ast_skid_limit = CFG_TGT_AST_SKID_LIMIT_QCA8074,
	.tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_4SS,
	.rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_4SS,
	.rx_timeout_pri = { CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_HI_PRI },
	.scan_max_pending_req = CFG_TGT_DEFAULT_SCAN_MAX_REQS,
	.bmiss_offload_max_vdev = CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
	.roam_offload_max_vdev = CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
	.roam_offload_max_ap_profiles =
		CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
	.num_mcast_groups = CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
	.num_mcast_table_elems = CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
	.mcast2ucast_mode = CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
	.tx_dbg_log_size = CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
	.num_wds_entries = CFG_TGT_WDS_ENTRIES,
	.dma_burst_size = CFG_TGT_DEFAULT_DMA_BURST_SIZE,
	.mac_aggr_delim = CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
	.rx_skip_defrag_timeout_dup_detection_check =
	     CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
	.vow_config = CFG_TGT_DEFAULT_VOW_CONFIG,
	.gtk_offload_max_vdev = CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV,
	.num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR900B,
	.max_frag_entries = 6,
#ifdef WLAN_SUPPORT_TWT
	.twt_ap_pdev_count = CFG_TGT_TWT_AP_PDEV_COUNT,
	.twt_ap_sta_count = CFG_TGT_TWT_AP_STA_COUNT,
#endif
	.peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V2,
};

/* Target config used for qcn9000 */
target_resource_config  tgt_cfg_qcn9000 = {
	.num_offload_peers = CFG_TGT_NUM_OFFLOAD_PEERS_QCN9000,
	.num_offload_reorder_buffs = CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS_QCA8074,
	.num_peer_keys = CFG_TGT_NUM_PEER_KEYS,
	.ast_skid_limit = CFG_TGT_AST_SKID_LIMIT_QCA8074,
	.tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_4SS,
	.rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_4SS,
	.rx_timeout_pri = { CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_HI_PRI },
	.scan_max_pending_req = CFG_TGT_DEFAULT_SCAN_MAX_REQS,
	.bmiss_offload_max_vdev = CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
	.roam_offload_max_vdev = CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
	.roam_offload_max_ap_profiles =
		CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
	.num_mcast_groups = CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
	.num_mcast_table_elems = CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
	.mcast2ucast_mode = CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
	.tx_dbg_log_size = CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
	.num_wds_entries = CFG_TGT_WDS_ENTRIES,
	.dma_burst_size = CFG_TGT_DEFAULT_DMA_BURST_SIZE,
	.mac_aggr_delim = CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
	.rx_skip_defrag_timeout_dup_detection_check =
	     CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
	.vow_config = CFG_TGT_DEFAULT_VOW_CONFIG,
	.gtk_offload_max_vdev = CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV,
	.num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR900B,
	.max_frag_entries = 6,
#ifdef WLAN_SUPPORT_TWT
	.twt_ap_pdev_count = CFG_TGT_TWT_AP_PDEV_COUNT,
	.twt_ap_sta_count = CFG_TGT_TWT_AP_STA_COUNT,
#endif
	.peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V2,
};

/* Target config used for qcn9224 */
target_resource_config  tgt_cfg_qcn9224 = {
	.num_offload_peers = CFG_TGT_NUM_OFFLOAD_PEERS_QCN9224,
	.num_offload_reorder_buffs = CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS_QCA8074,
	.num_peer_keys = CFG_TGT_NUM_PEER_KEYS,
	.ast_skid_limit = CFG_TGT_AST_SKID_LIMIT_QCA8074,
	.tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_4SS,
	.rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_4SS,
	.rx_timeout_pri = { CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_HI_PRI },
	.scan_max_pending_req = CFG_TGT_DEFAULT_SCAN_MAX_REQS,
	.bmiss_offload_max_vdev = CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
	.roam_offload_max_vdev = CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
	.roam_offload_max_ap_profiles =
		CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
	.num_mcast_groups = CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
	.num_mcast_table_elems = CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
	.mcast2ucast_mode = CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
	.tx_dbg_log_size = CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
	.num_wds_entries = CFG_TGT_WDS_ENTRIES,
	.dma_burst_size = CFG_TGT_DEFAULT_DMA_BURST_SIZE,
	.mac_aggr_delim = CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
	.rx_skip_defrag_timeout_dup_detection_check =
	     CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
	.vow_config = CFG_TGT_DEFAULT_VOW_CONFIG,
	.gtk_offload_max_vdev = CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV,
	.num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR900B,
	.max_frag_entries = 6,
#ifdef WLAN_SUPPORT_TWT
	.twt_ap_pdev_count = CFG_TGT_TWT_AP_PDEV_COUNT,
	.twt_ap_sta_count = CFG_TGT_TWT_AP_STA_COUNT,
#endif
#ifdef MAP_UNMAP_VERSION_V3
	.peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V3,
#else
	.peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V2,
#endif
};

/* Target config used for qca5332 */
target_resource_config  tgt_cfg_qca5332 = {
        .num_offload_peers = CFG_TGT_NUM_OFFLOAD_PEERS_QCN9224,
        .num_offload_reorder_buffs = CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS_QCA8074,
        .num_peer_keys = CFG_TGT_NUM_PEER_KEYS,
        .ast_skid_limit = CFG_TGT_AST_SKID_LIMIT_QCA8074,
        .tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_2SS,
        .rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_2SS,
        .rx_timeout_pri = { CFG_TGT_RX_TIMEOUT_LO_PRI,
                                CFG_TGT_RX_TIMEOUT_LO_PRI,
                                CFG_TGT_RX_TIMEOUT_LO_PRI,
                                CFG_TGT_RX_TIMEOUT_HI_PRI },
        .scan_max_pending_req = CFG_TGT_DEFAULT_SCAN_MAX_REQS,
        .bmiss_offload_max_vdev = CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
        .roam_offload_max_vdev = CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
        .roam_offload_max_ap_profiles =
                CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
        .num_mcast_groups = CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
        .num_mcast_table_elems = CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
        .mcast2ucast_mode = CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
        .tx_dbg_log_size = CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
        .num_wds_entries = CFG_TGT_WDS_ENTRIES,
        .dma_burst_size = CFG_TGT_DEFAULT_DMA_BURST_SIZE,
        .mac_aggr_delim = CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
        .rx_skip_defrag_timeout_dup_detection_check =
             CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
        .vow_config = CFG_TGT_DEFAULT_VOW_CONFIG,
        .gtk_offload_max_vdev = CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV,
        .num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR900B,
        .max_frag_entries = 6,
#ifdef WLAN_SUPPORT_TWT
        .twt_ap_pdev_count = CFG_TGT_TWT_AP_PDEV_COUNT,
        .twt_ap_sta_count = CFG_TGT_TWT_AP_STA_COUNT,
#endif
#ifdef MAP_UNMAP_VERSION_V3
        .peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V3,
#else
        .peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V2,
#endif
};

/* Target config used for qca5018 */
target_resource_config  tgt_cfg_qca5018 = {
	.num_offload_peers = CFG_TGT_NUM_OFFLOAD_PEERS_QCN9000,
	.num_offload_reorder_buffs = CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS_QCA8074,
	.num_peer_keys = CFG_TGT_NUM_PEER_KEYS,
	.ast_skid_limit = CFG_TGT_AST_SKID_LIMIT_QCA8074,
	.tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_2SS,
	.rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_2SS,
	.rx_timeout_pri = { CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_HI_PRI },
	.scan_max_pending_req = CFG_TGT_DEFAULT_SCAN_MAX_REQS,
	.bmiss_offload_max_vdev = CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
	.roam_offload_max_vdev = CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
	.roam_offload_max_ap_profiles =
		CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
	.num_mcast_groups = CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
	.num_mcast_table_elems = CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
	.mcast2ucast_mode = CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
	.tx_dbg_log_size = CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
	.num_wds_entries = CFG_TGT_WDS_ENTRIES,
	.dma_burst_size = CFG_TGT_DEFAULT_DMA_BURST_SIZE,
	.mac_aggr_delim = CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
	.rx_skip_defrag_timeout_dup_detection_check =
	     CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
	.vow_config = CFG_TGT_DEFAULT_VOW_CONFIG,
	.gtk_offload_max_vdev = CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV,
	.num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR900B,
	.max_frag_entries = 6,
#ifdef WLAN_SUPPORT_TWT
	.twt_ap_pdev_count = CFG_TGT_TWT_AP_PDEV_COUNT,
	.twt_ap_sta_count = CFG_TGT_TWT_AP_STA_COUNT,
#endif
	.peer_map_unmap_version = CFG_TGT_MAP_UNMAP_VERSION_V2,
};

target_resource_config  tgt_cfg_legacy = {
	/* need to reserve an additional peer for each VDEV */
	.num_offload_peers = CFG_TGT_NUM_OFFLOAD_PEERS,
	.num_offload_reorder_buffs = CFG_TGT_NUM_OFFLOAD_REORDER_BUFFS,
	.num_peer_keys = CFG_TGT_NUM_PEER_KEYS,
	.num_tids = CFG_TGT_NUM_TIDS,
	.ast_skid_limit = CFG_TGT_AST_SKID_LIMIT,
	.tx_chain_mask = CFG_TGT_DEFAULT_TX_CHAIN_MASK_4SS,
	.rx_chain_mask = CFG_TGT_DEFAULT_RX_CHAIN_MASK_4SS,
	.rx_timeout_pri = { CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_LO_PRI,
				CFG_TGT_RX_TIMEOUT_HI_PRI },
	.scan_max_pending_req = CFG_TGT_DEFAULT_SCAN_MAX_REQS,
	.bmiss_offload_max_vdev = CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV,
	.roam_offload_max_vdev = CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV,
	.roam_offload_max_ap_profiles =
		CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES,
	.num_mcast_groups = CFG_TGT_DEFAULT_NUM_MCAST_GROUPS,
	.num_mcast_table_elems = CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS,
	.mcast2ucast_mode = CFG_TGT_DEFAULT_MCAST2UCAST_MODE,
	.tx_dbg_log_size = CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE,
	.num_wds_entries = CFG_TGT_WDS_ENTRIES,
	.dma_burst_size = CFG_TGT_DEFAULT_DMA_BURST_SIZE,
	.mac_aggr_delim = CFG_TGT_DEFAULT_MAC_AGGR_DELIM,
	.rx_skip_defrag_timeout_dup_detection_check =
	    CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK,
	.vow_config = CFG_TGT_DEFAULT_VOW_CONFIG,
	.gtk_offload_max_vdev = CFG_TGT_DEFAULT_GTK_OFFLOAD_MAX_VDEV,
	.num_msdu_desc = CFG_TGT_NUM_MSDU_DESC_AR900B,
	.max_frag_entries = 0,
	.max_peer_ext_stats = CFG_TGT_DEFAULT_MAX_PEER_EXT_STATS,
	.smart_ant_cap = 0,
	.BK_Minfree = 0,
	.BE_Minfree = 0,
	.VI_Minfree = 0,
	.VO_Minfree = 0,
	.rx_batchmode = 0,
	.tt_support = 0,
	.atf_config = 0,
	.iphdr_pad_config = 1,
};

wmi_host_ext_resource_config tgt_ext_cfg = {
	.host_platform_config = 0,
	.fw_feature_bitmap = 0,
};

void init_deinit_set_default_tgt_config(
		struct wlan_objmgr_psoc *psoc,
		struct target_psoc_info *tgt_hdl)
{
	target_resource_config *tgt_cfg;
	struct tgt_info *info;
	int max_num_group_keys = 0;
	int ase_override_enabled = 0;

	if (!tgt_hdl) {
                target_if_err("target_psoc_info is null in lteu ext support");
		return;
	}

	info = (&tgt_hdl->info);

	max_num_group_keys = wlan_ucfg_get_config_param(psoc, MAX_GROUP_KEYS);
	if ( max_num_group_keys > MAX_VLAN )
		max_num_group_keys = MAX_VLAN;

#if defined(QCA_WIFI_QCA8074) || defined(QCA_WIFI_QCA6018) || \
    defined(QCA_WIFI_QCA5018) || defined(QCA_WIFI_QCN9000) || \
    defined(QCA_WIFI_QCA9574) || defined(QCA_WIFI_QCA5332)
	{
		if (info->target_type == TARGET_TYPE_QCA8074) {
			tgt_cfg = &tgt_cfg_qca8074;
		} else if (info->target_type == TARGET_TYPE_QCA8074V2) {
			tgt_cfg = &tgt_cfg_qca8074;
			/*
			 * Enable Ast flow override support
			 */
			ase_override_enabled = cfg_get(psoc, CFG_OL_ASE_OVERRIDE);
			if (ase_override_enabled) {
				init_deinit_ast_override_config(tgt_hdl, tgt_cfg);
			}
		} else if (info->target_type == TARGET_TYPE_QCA9574) {
			tgt_cfg = &tgt_cfg_qca8074;
			/*
			 * Enable Ast flow override support
			 */
			ase_override_enabled = cfg_get(psoc, CFG_OL_ASE_OVERRIDE);
			if (ase_override_enabled) {
				init_deinit_ast_override_config(tgt_hdl, tgt_cfg);
			}
		} else if (info->target_type == TARGET_TYPE_QCA6018) {
			tgt_cfg = &tgt_cfg_qca8074;
		} else if (info->target_type == TARGET_TYPE_QCN9000) {
			tgt_cfg = &tgt_cfg_qcn9000;
			/*
			 * Enable Ast flow override support for QCN9000
			 */
			ase_override_enabled = cfg_get(psoc, CFG_OL_ASE_OVERRIDE);
			if (ase_override_enabled) {
				init_deinit_ast_override_config(tgt_hdl, tgt_cfg);
			}
		} else if (info->target_type == TARGET_TYPE_QCN6122) {
			tgt_cfg = &tgt_cfg_qca8074;
		} else if (info->target_type == TARGET_TYPE_QCN9160) {
			tgt_cfg = &tgt_cfg_qca8074;
		} else if (info->target_type == TARGET_TYPE_QCA5018) {
			tgt_cfg = &tgt_cfg_qca5018;
		} else if ((info->target_type == TARGET_TYPE_QCA5332) ||
                   (info->target_type == TARGET_TYPE_QCN6432)) {
			tgt_cfg = &tgt_cfg_qca5332;
			/*
			* Enable Ast flow override support for QCA5332
			*/
			ase_override_enabled = cfg_get(psoc, CFG_OL_ASE_OVERRIDE);
			if (ase_override_enabled) {
			   init_deinit_ast_override_config(tgt_hdl, tgt_cfg);
			}
		} else if (info->target_type == TARGET_TYPE_QCN9224) {
			tgt_cfg = &tgt_cfg_qcn9224;
			/*
			 * Enable Ast flow override support for QCN9224
			 */
			ase_override_enabled = cfg_get(psoc, CFG_OL_ASE_OVERRIDE);
			if (ase_override_enabled) {
				init_deinit_ast_override_config(tgt_hdl, tgt_cfg);
			}
		} else {
			tgt_cfg = &tgt_cfg_legacy;
		}

		tgt_cfg->max_num_group_keys = max_num_group_keys;
	}
#else
        tgt_cfg = &tgt_cfg_legacy;
#endif

	/* num_msdu_desc is initialized to CFG_TGT_NUM_MSDU_DESC_AR900B
	 * here it is re-initialized to user configured value.
	 */
	if (info->max_descs)
		tgt_cfg->num_msdu_desc = info->max_descs;

#ifdef CONFIG_SAWF
	if (!ase_override_enabled) {
		tgt_cfg->sawf = wlan_ucfg_get_config_param(psoc, SAWF);
		targetif_nofl_debug("SAWF Enabled: %d", tgt_cfg->sawf);
	}
#endif
	tgt_cfg->fw_ast_indication_disable = wlan_ucfg_get_config_param
		(psoc, TGT_AST_INDICATION_DISABLE);

	tgt_cfg->carrier_vow_optimization = cfg_get(psoc,
						    CFG_OL_CARRIER_VOW_OPTIMIZATION);
	tgt_cfg->carrier_profile_config = cfg_get(psoc,
						  CFG_OL_CARRIER_PROFILE_CONFIG);
	tgt_cfg->rf_path = cfg_get(psoc, CFG_RF_SWITCH_CONFIG);

	qdf_mem_copy(&info->wlan_res_cfg, tgt_cfg,
			sizeof(target_resource_config));

	qdf_mem_copy(&info->wlan_ext_res_cfg, &tgt_ext_cfg,
			sizeof(wmi_host_ext_resource_config));

	target_if_set_init_cmd_dev_param(psoc, tgt_hdl);

}
#ifdef WIFI_TARGET_TYPE_2_0
QDF_STATUS init_deinit_sw_version_check(
		 struct wlan_objmgr_psoc *psoc,
		 struct target_psoc_info *tgt_hdl,
		 uint8_t *evt_buf)
{
	QDF_STATUS ver_match = QDF_STATUS_SUCCESS;
	struct wmi_host_fw_ver fw_ver;
	struct wmi_unified *wmi_handle;

	if (!tgt_hdl) {
                target_if_err("psoc target_psoc_info is null in version check");
		return QDF_STATUS_E_INVAL;
	}

	wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);

	if (wmi_extract_fw_version(wmi_handle, evt_buf, &fw_ver) ==
					QDF_STATUS_SUCCESS) {
		target_if_info("Firmware_Build_Number:%d",
				VER_GET_BUILD_NUM(fw_ver.sw_version_1));

		/* Check if the host driver is compatible with the RAM fw
		   version. If any of the Major/Minor/Patch/BuildNum
		   mismatches, fail attach.
		 */
		if (VER_GET_MAJOR(fw_ver.sw_version) != __VER_MAJOR_) {
			target_if_err(
				"host/RAM_fw Major Ver Mismatch: H:0x%X, F:0x%X !",
				__VER_MAJOR_, VER_GET_MAJOR(fw_ver.sw_version));
			ver_match = QDF_STATUS_E_FAILURE;
		}

		if (VER_GET_MINOR(fw_ver.sw_version) != __VER_MINOR_) {
			target_if_err(
			"host/RAM_fw Minor Ver Mismatch: H:0x%X, F:0x%X !",
			__VER_MINOR_, VER_GET_MINOR(fw_ver.sw_version));
			ver_match = QDF_STATUS_E_FAILURE;
		}

		if (VER_GET_RELEASE(fw_ver.sw_version_1) != __VER_RELEASE_) {
			target_if_err(
			"host/RAM_fw Patch Ver Mismatch: H:0x%X, F:0x%X !",
			__VER_RELEASE_, VER_GET_RELEASE(fw_ver.sw_version_1));
			ver_match = QDF_STATUS_E_FAILURE;
		}

		if (VER_GET_BUILD_NUM(fw_ver.sw_version_1) != __BUILD_NUMBER_) {
			target_if_err(
			"host/RAM_fw Build Ver Mismatch: H:0x%X, F:0x%X !",
			__BUILD_NUMBER_,
			VER_GET_BUILD_NUM(fw_ver.sw_version_1));
			ver_match = QDF_STATUS_E_FAILURE;
		}

		/* update the version info in the soc for the OS-es to check
		*/
		tgt_hdl->info.version.wlan_ver = fw_ver.sw_version;
		tgt_hdl->info.version.wlan_ver_1 = fw_ver.sw_version_1;

	}

	if(ver_match != QDF_STATUS_SUCCESS)
                target_if_err(
                "host/RAM_fw uses same Ver: Major:0x%X, Minor:0x%X,"
                "Release:0x%X, Build:0x%X",
		 __VER_MAJOR_, __VER_MINOR_, __VER_RELEASE_,  __BUILD_NUMBER_);

	return ver_match;
}
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void init_deinit_send_mlo_setup_done_event(struct wlan_objmgr_psoc *psoc)
{
	struct pdev_op_args oper;

	oper.type = PDEV_ITER_ML_REINIT_DONE;
	wlan_objmgr_iterate_obj_list(psoc, WLAN_PDEV_OP, wlan_pdev_operation,
				     &oper, 0, WLAN_INIT_DEINIT_ID);

	wlan_mlo_read_tqm_offset(psoc);
}
#endif

static struct target_ops targ_ops = {
	.ext_resource_config_enable = init_deinit_ext_resource_config_enable,
	.peer_config = init_deinit_peer_config,
	.mesh_support_enable = init_deinit_mesh_support_enable,
#if UNIFIED_SMARTANTENNA
	.smart_antenna_enable = init_deinit_smart_antenna_enable,
#else
	.smart_antenna_enable = NULL,
#endif
#if WLAN_CFR_ENABLE
        .cfr_support_enable = init_deinit_cfr_support_enable,
#else
        .cfr_support_enable = NULL,
#endif
#if QCA_AIRTIME_FAIRNESS
	.atf_config_enable = init_deinit_atf_config_enable,
#else
	.atf_config_enable = NULL,
#endif
	.btcoex_config_enable = init_deinit_btcoex_config_enable,
	.lteu_ext_support_enable = init_deinit_lteu_ext_support_enable,
	.set_init_cmd_dev_based_params = init_deinit_set_init_cmd_dev_params,
	.alloc_pdevs = init_deinit_alloc_pdevs,
	.update_pdev_tgt_info = init_deinit_update_pdev_tgt_info,
#if WIFI_MEM_MANAGER_SUPPORT
	.mem_mgr_alloc_chunk = init_deinit_mem_mgr_alloc_chunk,
	.mem_mgr_free_chunks = init_deinit_mem_mgr_free_chunks,
#else
	.mem_mgr_alloc_chunk = NULL,
	.mem_mgr_free_chunks = NULL,
#endif
	.print_svc_ready_ex_param = init_deinit_print_service_ready_ex_params,
	.add_11ax_modes = init_deinit_add_11ax_modes,
	.set_default_tgt_config = init_deinit_set_default_tgt_config,
#ifdef WIFI_TARGET_TYPE_2_0
	.sw_version_check = init_deinit_sw_version_check,
#endif
	.eapol_minrate_enable = init_deinit_eapol_minrate_enable,
#ifndef REMOVE_PKT_LOG
	.set_pktlog_checksum = init_deinit_set_pktlog_checksum,
#else
	.set_pktlog_checksum = NULL,
#endif
	.csa_switch_count_status = ol_ath_pdev_csa_status_event_handler,
	.ema_init = ol_ath_ema_init_ctx,
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
	.mlo_capable = ol_ath_mlo_capable,
	.mlo_get_group_id = ol_ath_mlo_get_group_id,
	.mlo_setup_done_event = init_deinit_send_mlo_setup_done_event,
#endif
#if WLAN_WIFI_RADAR_ENABLE
	.wifi_radar_support_enable = init_deinit_wifi_radar_support_enable,
#else
	.wifi_radar_support_enable = NULL,
#endif

};

QDF_STATUS init_deinit_register_featurs_ops(struct wlan_objmgr_psoc *psoc)
{
	struct target_psoc_info *tgt_hdl;

	if (!psoc) {
		target_if_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
	if (!tgt_hdl) {
		target_if_err("target_psoc_info is null");
		return QDF_STATUS_E_INVAL;
	}

	tgt_hdl->tif_ops = &targ_ops;

	return QDF_STATUS_SUCCESS;
}
