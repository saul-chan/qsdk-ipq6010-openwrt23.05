/*
 * Copyright (c) 2011-2021,2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifdef IPA_OFFLOAD
#include "wlan_ipa_ucfg_api.h"
#endif
#include <ieee80211_mlme_priv.h>
#include <ieee80211_ucfg.h>
#include <ieee80211_cfg80211.h>
#include <include/wlan_mlme_cmn.h>
#include <include/wlan_pdev_mlme.h>
#include <include/wlan_vdev_mlme.h>
#include <ieee80211_mlme_dfs_dispatcher.h>
#if UNIFIED_SMARTANTENNA
#include <wlan_sa_api_utils_api.h>
#include <wlan_sa_api_utils_defs.h>
#endif
#include <wlan_mlme_dbg.h>
#include <wlan_utility.h>
#include "core/inc/vdev_mlme_sm_actions.h"
#include "vdev_mgr/core/src/vdev_mgr_ops.h"
#include <wlan_osif_priv.h>
#include <wlan_mlme_if.h>
#include <wlan_mlme_vdev_mgmt_ops.h>
#include <cfg_ucfg_api.h>
#include "vdev_mgr/core/src/vdev_mlme_sm.h"
#include <wlan_psoc_mlme.h>
#include <wlan_psoc_mlme_main.h>
#include <wlan_rnr.h>
#include <wlan_cm_api.h>
#ifdef WLAN_SUPPORT_FILS
#include <wlan_fd_ucfg_api.h>
#endif /* WLAN_SUPPORT_FILS */
#include <wlan_repeater_api.h>
#include <target_type.h>
#include <wlan_mlo_mgr_sta.h>
#include <wlan_mlo_mgr_peer.h>
#include <cdp_txrx_ctrl.h>
#include <wlan_vdev_mgr_utils_api.h>
#include <wlan_mgmt_txrx_rx_reo_utils_api.h>
#include <wlan_mlo_mgr_setup.h>

QDF_STATUS mlme_register_ops(struct vdev_mlme_obj *vdev_mlme);
static os_timer_func(mlme_vdev_peer_del_timeout_handler);

enum mlme_sm_cmd_type {
    MLME_SM_CMD_PDEV_RADAR_DETECT,
};

struct mlme_sm_cmd_sched_data {
    struct wlan_objmgr_pdev *pdev;
    enum mlme_sm_cmd_type cmd_type;
    struct ieee80211_ath_channel *new_chan;
};

/*
 * mlme_sm_cmd_sched_cb(): cb function from scheduler context
 * @msg: scheduler msg data
 *
 * This cb will be called when a msg posted to the scheduler queue
 * has to be flushed
 *
 * Return: Success
 */
QDF_STATUS mlme_sm_cmd_flush_cb(struct scheduler_msg *msg)
{
	struct mlme_sm_cmd_sched_data *req = msg->bodyptr;

	switch (req->cmd_type) {
	case MLME_SM_CMD_PDEV_RADAR_DETECT:
		wlan_pdev_mlme_op_clear(req->pdev,
					WLAN_PDEV_OP_RADAR_DETECT_DEFER);
	break;
	}

	wlan_objmgr_pdev_release_ref(req->pdev, WLAN_SCHEDULER_ID);
	qdf_mem_free(req);
	return QDF_STATUS_SUCCESS;
}

/*
 * mlme_sm_cmd_sched_cb(): cb function from scheduler context
 * @msg: scheduler msg data
 *
 * This is the cb from the scheduler context where MBSSID scenario
 * related vdev cmds are processed
 *
 * Return: Success if vdev cmd is processed else error.
 */
QDF_STATUS mlme_sm_cmd_sched_cb(struct scheduler_msg *msg)
{
	struct mlme_sm_cmd_sched_data *req = msg->bodyptr;
	wlan_dev_t ic = wlan_pdev_get_mlme_ext_obj(req->pdev);

	if (!ic) {
		qdf_err("NULL ic");
		wlan_objmgr_pdev_release_ref(req->pdev, WLAN_SCHEDULER_ID);
		qdf_mem_free(req);
		return QDF_STATUS_SUCCESS;
	}

	switch (req->cmd_type) {
	case MLME_SM_CMD_PDEV_RADAR_DETECT:
		wlan_pdev_mlme_vdev_sm_notify_radar_ind(req->pdev,
							req->new_chan);
	break;
	default:
		qdf_err("Unknown cmd type");
	}

	wlan_objmgr_pdev_release_ref(req->pdev, WLAN_SCHEDULER_ID);
	qdf_mem_free(req);

	return QDF_STATUS_SUCCESS;
}

/*
 * mlme_sm_cmd_schedule_req(): Form a scheduler msg to be posted for
 * vdev operations in MBSSID scenario
 * @vdev: Object manager vdev
 * @cmd_type: Serialization command type
 *
 * Return: Success if the request is posted to the scheduler, else error
 */
QDF_STATUS mlme_sm_cmd_schedule_req(struct wlan_objmgr_pdev *pdev,
				    enum mlme_sm_cmd_type cmd_type,
				    struct ieee80211_ath_channel *chan)
{
	struct scheduler_msg msg = {0};
	struct mlme_sm_cmd_sched_data *req;
	QDF_STATUS ret = QDF_STATUS_E_FAILURE;

	if (!pdev)
		return QDF_STATUS_E_INVAL;

	req = qdf_mem_malloc(sizeof(*req));
	if (!req) {
		qdf_err("req is NULL");
		return ret;
	}

	ret = wlan_objmgr_pdev_try_get_ref(pdev, WLAN_SCHEDULER_ID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		qdf_err("unable to get reference");
		goto sched_error;
	}

	req->pdev = pdev;
	req->cmd_type = cmd_type;
	req->new_chan = chan;

	msg.bodyptr = req;
	msg.callback = mlme_sm_cmd_sched_cb;
	msg.flush_callback = mlme_sm_cmd_flush_cb;

	ret = scheduler_post_message(QDF_MODULE_ID_OS_IF,
				QDF_MODULE_ID_OS_IF, QDF_MODULE_ID_OS_IF, &msg);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_SCHEDULER_ID);
		qdf_err("failed to post scheduler_msg");
		goto sched_error;
	}
	return ret;

sched_error:
	qdf_mem_free(req);
	return ret;
}

QDF_STATUS mlme_vdev_reset_proto_params_cb(
                        struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_channel *bss_chan;

	if (!vdev_mlme) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;

	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	bss_chan = wlan_vdev_mlme_get_bss_chan(vdev);
	if (bss_chan)
		ieee80211_update_vdev_chan(bss_chan, NULL);
	else
		mlme_err("(vdev-id:%d) BSS chan is not allocated",
						    wlan_vdev_get_id(vdev));

	if (vdev_mlme->mgmt.generic.ssid_len) {
		qdf_mem_zero(vdev_mlme->mgmt.generic.ssid, WLAN_SSID_MAX_LEN+1);
		vdev_mlme->mgmt.generic.ssid_len = 0;
	}

	return QDF_STATUS_SUCCESS;
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
static QDF_STATUS mlme_sta_cac_start(struct ieee80211com *ic,
                                        struct ieee80211vap *vap)
{
	int cac_timeout;
	if (mlme_is_stacac_needed(vap)) {
		qdf_info(
			"STACAC_start chan %d timeout %d sec, curr time:%d sec",
			ic->ic_curchan->ic_freq,
			ieee80211_dfs_get_cac_timeout(ic, ic->ic_curchan),
			(qdf_system_ticks_to_msecs(qdf_system_ticks()) / 1000));
		mlme_set_stacac_running(vap,1);
		/* start CAC timer */
		ieee80211_dfs_cac_start(vap);
		mlme_reset_mlme_req(vap);
		mlme_clear_mlme_req(vap);
		cac_timeout = ieee80211_dfs_get_cac_timeout(ic, ic->ic_curchan);
		wlan_cfg80211_dfs_cac_start(vap, cac_timeout);
		IEEE80211_DELIVER_EVENT_CAC_STARTED(vap,
					ic->ic_curchan->ic_freq, cac_timeout);
		return QDF_STATUS_SUCCESS;
	}
	return QDF_STATUS_E_FAILURE;
}
#endif

int
ieee80211_vap_wme_param_update(struct ieee80211com *ic, wlan_if_t vaphandle)
{
    enum ieee80211_phymode mode;
    int i;
    struct ieee80211_wme_state *wme;

    wme = &vaphandle->iv_wmestate;
    mode = ieee80211_chan2mode(vaphandle->iv_bsschan);
    for(i = 0;i< WME_NUM_AC;i++){
        ic->phyParamForAC[i][mode].logcwmin = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmin;
        ic->phyParamForAC[i][mode].logcwmax = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_logcwmax;
        ic->phyParamForAC[i][mode].aifsn = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_aifsn;
        ic->phyParamForAC[i][mode].txopLimit = wme->wme_wmeChanParams.cap_wmeParams[i].wmep_txopLimit;
        ic->bssPhyParamForAC[i][mode].logcwmin = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_logcwmin;
        ic->bssPhyParamForAC[i][mode].logcwmax = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_logcwmax;
        ic->bssPhyParamForAC[i][mode].aifsn = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_aifsn;
        ic->bssPhyParamForAC[i][mode].txopLimit = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_txopLimit;
        ic->bssPhyParamForAC[i][mode].acm = wme->wme_wmeBssChanParams.cap_wmeParams[i].wmep_acm;
    }
    return 0;
}

static inline void ieee80211_update_chan_history(struct ieee80211com  *ic)
{
    ic->ic_chanhist[ic->ic_chanidx].chanid = (ic->ic_curchan)->ic_ieee;
    ic->ic_chanhist[ic->ic_chanidx].chanband = wlan_reg_freq_to_band(ic->ic_curchan->ic_freq);
    ic->ic_chanhist[ic->ic_chanidx].chanjiffies = OS_GET_TIMESTAMP();
    ic->ic_chanidx == (IEEE80211_CHAN_MAXHIST - 1) ? ic->ic_chanidx = 0 : ++(ic->ic_chanidx);
}

/**
 * ieee80211_coex_derive_phymode:
 * Check if the current selected channel is marked as intolerant due to
 * 20/40MHz coexistence in the 2.4GHz band and return the correct phymode.
 *
 * Parameters:
 * ic      : Pointer to ic structure
 * des_chan: Pointer to the wlan_channel structure for the vdev des chan
 *
 * Return:
 * Desired phymode: Success
 * AUTO           : Failure
 */
enum ieee80211_phymode ieee80211_coex_derive_phymode(struct ieee80211com *ic,
						     struct wlan_channel *des_chan)
{
	enum ieee80211_phymode phymode;
	struct ieee80211_ath_channel c = {0};

	if (!ic) {
		qdf_err("Invalid ic");
		return IEEE80211_MODE_AUTO;
	}

	if (!des_chan) {
		qdf_err("Invalid channel");
		return IEEE80211_MODE_AUTO;
	}

	phymode = wlan_vdev_get_ieee_phymode(des_chan->ch_phymode);

	c.ic_freq = des_chan->ch_freq;
	c.ic_flags = des_chan->ch_flags;

	/*
	 * If 20/40MHz coexistence is enabled, move the phymode down to
	 * 20MHz for HT40 and HE40 modes.
	 */
	if (!(ic->ic_flags & IEEE80211_F_COEXT_DISABLE) &&
	    (IEEE80211_IS_CHAN_40INTOL(ic, &c))) {
		switch(phymode) {
			case IEEE80211_MODE_11NG_HT40:
			case IEEE80211_MODE_11NG_HT40PLUS:
			case IEEE80211_MODE_11NG_HT40MINUS:
                            phymode = IEEE80211_MODE_11NG_HT20;
			break;

			case IEEE80211_MODE_11AXG_HE40:
			case IEEE80211_MODE_11AXG_HE40PLUS:
			case IEEE80211_MODE_11AXG_HE40MINUS:
                            phymode = IEEE80211_MODE_11AXG_HE20;
			break;
#ifdef WLAN_FEATURE_11BE
			case IEEE80211_MODE_11BEG_EHT40:
			case IEEE80211_MODE_11BEG_EHT40PLUS:
			case IEEE80211_MODE_11BEG_EHT40MINUS:
                            phymode = IEEE80211_MODE_11BEG_EHT20;
			break;
#endif /* WLAN_FEATURE_11BE */
			default:
			break;
		}
	}

	return phymode;
}

/*
 * ieee80211_reset_dcsim:
 * Reset DCS interference management depending on if the VAP was
 * brought up with fixed channel configuration or ACS.
 * Don't enable DCS WLAN IM for fixed channel configuration in order to avoid
 * moving to a channel other than the preset one.
 *
 * Parameters:
 * @ic: Pointer to the ic structure
 */
static void ieee80211_reset_dcsim(struct ieee80211com *ic)
{
	if (!ic->ic_eacs_done && !ieee80211_get_num_ap_vaps_up(ic)) {
		/* disable dcs when channel is not chosen by EACS */
		ic->ic_disable_dcsim(ic);
	}

	return;
}

void ieee80211_update_peer_cw(struct ieee80211com *ic,
			      struct ieee80211vap *vap)
{
	struct node_chan_width_switch_params pi = {0};

	/* Update new channel and width for associated STA's */
	if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
		pi.max_peers = wlan_vdev_get_peer_count(vap->vdev_obj);
		pi.chan_width_peer_list = qdf_mem_malloc(sizeof(struct node_chan_width_switch_info) *
							 pi.max_peers);

		if (!pi.chan_width_peer_list) {
			qdf_err("Error in allocating peer chwidth list");

		/* Sanity check is done within ieee80211_node_update_chan_and_phymode
		 * This is to ensure, in the case of failure to update target info
		 * downgrades will still be supported (as before) */
		}

		wlan_iterate_station_list(vap, ieee80211_node_update_chan_and_phymode, &pi);

		if (pi.chan_width_peer_list) {
			/* Peer information will be sent to the target only if
			 * the necessary space was allocated */
			if (pi.num_peers > 0) {
				ic->ic_node_chan_width_switch(&pi, vap);
			}

			qdf_mem_free(pi.chan_width_peer_list);
		}
	}
}

QDF_STATUS mlme_ext_restart_bmiss_sched_cb(struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *vdev = msg->bodyptr;
	struct ieee80211vap *vap;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
		return QDF_STATUS_E_FAILURE;
	}

	IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_ext_restart_bmiss_flush_cb(struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *vdev = msg->bodyptr;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);

	return QDF_STATUS_SUCCESS;
}

static void
mlme_vdev_sched_restart_bmiss(struct wlan_objmgr_vdev *vdev)
{
	struct scheduler_msg msg = {0};
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	msg.bodyptr = vdev;
	msg.callback = mlme_ext_restart_bmiss_sched_cb;
	msg.flush_callback =
		mlme_ext_restart_bmiss_flush_cb;
	wlan_objmgr_vdev_try_get_ref(vdev,
			WLAN_SCHEDULER_ID);
	ret = scheduler_post_message(QDF_MODULE_ID_MLME,
			QDF_MODULE_ID_MLME,
			QDF_MODULE_ID_MLME, &msg);
	if (QDF_IS_STATUS_ERROR(ret)) {
		mlme_err("Failed to post scheduler msg");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
	}
}


static void
ieee80211_set_ul_mumimo(struct ieee80211vap *vap)
{
	struct ieee80211com *ic;
	bool is_ul_mumimo_supported = false;

	if (!vap || vap->iv_disable_mumimo)
		return;

	ic = vap->iv_ic;

	if (vap->iv_he_ul_mumimo &&
	    (ic->ic_get_ul_mumimo_support(vap, &is_ul_mumimo_supported) == QDF_STATUS_SUCCESS) &&
	     !is_ul_mumimo_supported) {
		qdf_warn("Overriding UL MUMIMO configuration to %u as per Target Capability for VDEV %u",
			 is_ul_mumimo_supported, wlan_vdev_get_id(vap->vdev_obj));
		vap->iv_he_ul_mumimo = is_ul_mumimo_supported;
		vap->iv_he_full_bw_ulmumimo = is_ul_mumimo_supported;
		if (vap->iv_ic->ic_vap_set_param) {
			vap->iv_ic->ic_vap_set_param(vap,
						     IEEE80211_CONFIG_HE_UL_MU_MIMO,
						     vap->iv_he_ul_mumimo);
			vap->iv_ic->ic_vap_set_param(vap,
						     IEEE80211_CONFIG_HE_FULL_BW_UL_MUMIMO,
						     vap->iv_he_full_bw_ulmumimo);
		}
	}
}

bool ieee80211_vap_mode_is_ic(struct ieee80211vap *vap) {
	struct ieee80211com *ic = vap->iv_ic;
	enum ieee80211_phymode vap_phymode, ic_phymode;
	struct ieee80211vap *tmpvap = NULL;

	ic_phymode = ieee80211_get_current_phymode(ic);
	TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
		vap_phymode = wlan_get_desired_phymode(tmpvap);
		if (get_mode_from_phymode(vap_phymode) == get_mode_from_phymode(ic_phymode))
			return TRUE;
	}
	return FALSE;
}

QDF_STATUS mlme_vdev_start_continue_cb(struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_pdev *pdev;
	struct ieee80211com *ic;
	struct ieee80211_node *ni;
	struct ieee80211vap *vap;
	struct ieee80211vap *tmpvap;
	struct ieee80211_ath_channel chan_obj, des_ic_chan_obj, ni_chan;
	struct ieee80211_ath_channel *chan = &chan_obj;
	struct ieee80211_ath_channel *des_ic_chan = &des_ic_chan_obj;
	struct wlan_channel *des_chan;
	enum QDF_OPMODE opmode;
	uint8_t numvaps_up = 0;
	struct ieee80211_wme_state wme_zero = {0};
	uint8_t restart = 0;
	int8_t error;
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
	int32_t nss_val = 0;
	struct link_change_info change_info;
#endif
	struct cdp_soc_t *soc_txrx_handle;
	enum ieee80211_phymode phymode;
	struct wlan_cm_vdev_reassoc_req req = {0};
	struct chan_params chan_p = {0};
	u_int8_t *curbssid = NULL;
#ifdef IPA_OFFLOAD
	enum wlan_ipa_wlan_event wlan_event;
	struct wlan_objmgr_psoc *psoc = NULL;
#endif
	enum ieee80211_phymode ic_phymode = 0, vap_phymode = 0;
	bool update_chan = true;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	if (pdev == NULL) {
		mlme_err("(vdev-id:%d) PDEV is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic) {
		mlme_err("(vdev-id:%d) ic is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	soc_txrx_handle = wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(pdev));

	if (!soc_txrx_handle) {
		mlme_err("Failed to get DP handles");
		return QDF_STATUS_E_FAILURE;
	}

	if (event_data_len == 1)
		restart = *((uint8_t *)event_data);

	des_chan = wlan_vdev_mlme_get_des_chan(vdev);
	opmode = wlan_vdev_mlme_get_opmode(vdev);

	/*
	 * Derive the appropriate phymode after checking the 2.4GHz
	 * 20/40MHz intolerance flags for the desired channel.
	 */
	phymode = ieee80211_coex_derive_phymode(ic, des_chan);

	chan_p.freq = des_chan->ch_freq;
	chan_p.cfreq2 = des_chan->ch_cfreq2;
	set_chan_params_puncture_bitmap(&chan_p, des_chan->puncture_bitmap);

	*des_ic_chan = ieee80211_find_dot11_channel(ic, &chan_p,
                              wlan_vdev_get_ieee_phymode(des_chan->ch_phymode));

#ifdef CONFIG_AFC_SUPPORT
	if ((vap->iv_afc_orig_mode > IEEE80211_MODE_AUTO) &&
            (vap->iv_afc_orig_mode < IEEE80211_MODE_MAX)) {
		/*
		 * If the afc original mode is present, reset the desired mode
		 * back to this mode. Once done, reset to AUTO to avoid
		 * resetting multiple times.
		 */
                wlan_set_desired_phymode(vap, vap->iv_afc_orig_mode);
                vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
	}
#endif /* CONFIG_AFC_SUPPORT */

	*chan =  ieee80211_find_dot11_channel(ic,
					     &chan_p, phymode);
	numvaps_up = ieee80211_get_num_ap_vaps_up(ic);
	if (numvaps_up) {
		TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
			if (tmpvap && numvaps_up &&
			    IEEE80211_IS_MLO_BRIDGE_VAP(tmpvap) &&
			    (QDF_STATUS_SUCCESS == wlan_vdev_mlme_is_active(tmpvap->vdev_obj))) {
				numvaps_up--;
			}
		}
	}
	if (!IEEE80211_IS_CHAN_VALID(chan)) {
		if (!numvaps_up) {
			mlme_err("(vdev-id:%d) chan is NULL (ieee: %d)",
					wlan_vdev_get_id(vdev), des_chan->ch_ieee);
			return QDF_STATUS_E_FAILURE;
		} else {
			/*
			 * There are UP vaps, go ahead and use the ic's
			 * current operating channel.
			 */
			*chan = *ic->ic_curchan;
		}
	}

	switch (opmode) {

	case QDF_MONITOR_MODE:
		/* Handle same as HOSTAP */
	case QDF_SAP_MODE:
		ni = vap->iv_bss;
		/* Skip re-assigning ni_intval for LP IOT vap */
		if (!(vap->iv_create_flags & IEEE80211_LP_IOT_VAP))
			ni->ni_intval = ic->ic_intval;

		if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
		    IEEE80211_IS_SPECIAL_VAP(vap) && !restart) {
		    if (cdp_set_monitor_mode(soc_txrx_handle,
					     wlan_vdev_get_id(vap->vdev_obj),
					     IEEE80211_IS_SMART_MON_VAP(vap))) {
			mlme_err("Unable to configure monitor ring buffers for special vap");
			return QDF_STATUS_E_FAILURE;
		    }
            /* Set undecoded metadata capture mode filter */
            mlme_restore_undecoded_capture_mode_filter(ic);
		    vap->iv_special_vap_is_monitor = 1;
#ifdef QCA_SUPPORT_SCAN_SPCL_VAP_STATS
		    wlan_reset_scan_spcl_vap_stats(vap);
#endif
		}

		/* Update RNR cache during channel switch */
		if(!IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan) &&
		   wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)){
			wlan_co_mld_vap_bcn_tmpl_update(vap);
		}

		if (restart && vdev->vdev_mlme.bss_chan &&
		    ieee80211_is_fils_20tu_allowed_vap(vap)) {
			/* vdev->vdev_mlme.bss_chan is the previous channel for
			 * the vap and chan is the destination channel for CSA
			 */
			if (wlan_reg_freq_to_band(vdev->vdev_mlme.bss_chan->ch_freq) == REG_BAND_6G) {
				if (IEEE80211_IS_CHAN_6GHZ(chan)) {
					/* This is a case where we are only
					 * changing channel but band is still 6G
					 * So simple cache update is sufficient
					 */
					wlan_update_6ghz_rnr_cache(vap, 0);
				} else if (IEEE80211_IS_CHAN_5GHZ(chan)) {
					/* This is a case where we are changing
					 * band from 6G to 5G. So vap must be
					 * removed from cache.
					 */
					wlan_remove_vap_from_6ghz_rnr_cache(vap);
				}
			} else if (wlan_reg_freq_to_band(vdev->vdev_mlme.bss_chan->ch_freq) == REG_BAND_5G) {
				if (IEEE80211_IS_CHAN_6GHZ(chan)) {
					/* This is a case where we are switching
					 * from 5G band to 6G. So vap must be
					 * added to cache.
					 */
					wlan_update_6ghz_rnr_cache(vap, 1);
				}
			}
		}

#ifdef WLAN_SUPPORT_FILS
		/*
		 * At this point 'vdev_mlme.bss_chan' refers to the previous
		 * channel while 'chan' points to new channel. When moving from
		 * 6GHz to 5GHz, FILS offload will need to be disabled by
		 * updating the FW with zero period to disable FILS or broadcast
		 * probe responses.
		 */
		if (vdev->vdev_mlme.bss_chan &&
		    (wlan_reg_freq_to_band(vdev->vdev_mlme.bss_chan->ch_freq) == REG_BAND_6G) &&
		    IEEE80211_IS_CHAN_5GHZ(chan)) {
			wlan_vdev_mlme_feat_ext_cap_clear(vap->vdev_obj,
						WLAN_VDEV_FEXT_FILS_DISC_6G_SAP);
			ucfg_fils_config(vap->vdev_obj, 0);
		}
#endif /* WLAN_SUPPORT_FILS */

		vap_phymode = wlan_get_desired_phymode(vap);
		ic_phymode = ieee80211_get_current_phymode(ic);
		/*
		 * if there is a vap already running.
		 * ignore the desired channel and use the
		 * operating channel of the other vap.
		 */
		if ((numvaps_up == 0) || ((vap_phymode > ic_phymode) || (!ieee80211_vap_mode_is_ic(vap)))) {
			 /* 20/40 Mhz coexistence  handler */
			*ic->ic_prevchan = *ic->ic_curchan;
			*ic->ic_curchan = *chan;

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
			/* Check pdev mlo capabilities, before notifying RM about BW/NSS change */
			change_info.ml_grp_id = wlan_get_mlo_grp_id_from_pdev(pdev);

			if (mlo_psoc_get_grp_id(wlan_pdev_get_psoc(pdev), &(change_info.ml_grp_id))) {
				mlme_debug(" Valid mlo_grp_id, Fetching updated BW/NSS values..\n");
				/* Fetching updated bw & nss values */
				wlan_util_vdev_mlme_get_param(vdev_mlme, WLAN_MLME_CFG_NSS, &nss_val);
				change_info.hw_id   = wlan_mlo_get_pdev_hw_link_id(pdev);
				change_info.new_nss = nss_val;
				change_info.new_bw = wlan_cfg80211_get_qca_vendor_channel_width(ic);

				/* Notify RM App about the change */

#if UMAC_SUPPORT_CFG80211
                                if(vap && vap->iv_cfg80211_create)
				   wlan_cfg80211_t2lm_app_reply_generic_response(&change_info, (uint8_t)QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_LINK_BW_NSS_CHANGE, 0);
#endif
			}
#endif /* WLAN_CONFIG_TELEMETRY_AGENT */
                        /* Disable mu_mimo for HKv2 in 160MHz*/
                        if (ic->ic_get_tgt_type(ic) == TARGET_TYPE_QCA8074V2 &&
                            ic->ic_cwm_get_width(ic) == IEEE80211_CWM_WIDTH160) {
                            if (!vap->iv_disable_mumimo) {
                                vap->iv_disable_mumimo = TRUE;

                                if (vap->iv_ic->ic_vap_set_param)
                                    vap->iv_ic->ic_vap_set_param(vap,
                                                                 IEEE80211_CONFIG_HE_UL_MU_MIMO,
                                                                 1);
                            }
                        } else {
                            if (vap->iv_disable_mumimo) {
                                vap->iv_disable_mumimo = FALSE;
                                if (vap->iv_ic->ic_vap_set_param)
                                    vap->iv_ic->ic_vap_set_param(vap,
                                                                 IEEE80211_CONFIG_HE_UL_MU_MIMO,
                                                                 1);
                            }
                        }
			/* update max channel power to max regpower of current channel */
			ieee80211com_set_curchanmaxpwr(ic, chan->ic_maxregpower);
			/* Update the channel for monitor mode path */
			cdp_set_curchan(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(pdev), chan->ic_freq);

			if(!ic->ic_diff_mode && !ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
				/* ieee80211 Layer - Default Configuration */
				*vap->iv_bsschan = *ic->ic_curchan;
			} else {
				if (ic->ic_diff_mode)
					*vap->iv_bsschan = ieee80211_find_desired_chan(vap,ic->ic_curchan,vap->iv_des_mode);
				else
					*vap->iv_bsschan = ieee80211_find_desired_chan(vap,ic->ic_curchan,wlan_vdev_get_ieee_phymode(des_chan->ch_phymode));
				if (!IEEE80211_IS_CHAN_VALID(vap->iv_bsschan))
					*vap->iv_bsschan = *ic->ic_curchan;
			}

			/* This function was being called before "vap->iv_bsschan = ic->ic_curchan" and now moved here.
			 * This fixes a bug where the wme parameters are set to FW before the current channel gets
			 * updated("vap->iv_bsschan"). The "ic_flags" in "iv_bsschan" is used to get the corresponding
			 * phy mode with respect to the channel (ieee80211_chan2mode), and this phy mode is used to set the
			 * wme parameters to FW. IR-088422.
			 */
			if (vap->iv_wme_reset) {
			   ieee80211_wme_initparams_locked(vap);
			   vap->iv_wme_reset = 0;
			} else {
			   ieee80211_wme_updateparams_locked(vap);
			}

			if (memcmp(&wme_zero, &vap->iv_wmestate, sizeof(wme_zero)) != 0) {
				ieee80211_vap_wme_param_update(ic, vap);
			}
			/* reset erp state */
			ieee80211_reset_erp(ic, ic->ic_curmode, vap->iv_opmode);

			/* reset DCS WLAN IM */
			ieee80211_reset_dcsim(ic);
                        ic->ic_eacs_done = 0;
		} else {
			/* Copy the wme params from ic to vap structure if vaps are up for the first time */
			if(qdf_mem_cmp(&wme_zero, &vap->iv_wmestate,
				 	sizeof(wme_zero)) == 0)
				qdf_mem_copy(&vap->iv_wmestate,
					     &ic->ic_wme, sizeof(ic->ic_wme));

			if(!ic->ic_diff_mode && !ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
				/* ieee80211 Layer - Default Configuration */
				*vap->iv_bsschan = *ic->ic_curchan;
			} else {
				if (ic->ic_diff_mode)
					*vap->iv_bsschan = ieee80211_find_desired_chan(vap,ic->ic_curchan,vap->iv_des_mode);
				else
					*vap->iv_bsschan = ieee80211_find_desired_chan(vap,ic->ic_curchan,wlan_vdev_get_ieee_phymode(des_chan->ch_phymode));
				if (!IEEE80211_IS_CHAN_VALID(vap->iv_bsschan)) {
					*vap->iv_bsschan = *ic->ic_curchan;
                                }
			}

			if (vap->iv_wme_reset) {
			   ieee80211_wme_initparams_locked(vap);
			   vap->iv_wme_reset = 0;
			} else {
			   ieee80211_wme_updateparams_locked(vap);
			}
		}

		if (ieee80211_get_num_ap_vaps_up(ic) == 0) {
			/*update channel history*/
			ieee80211_update_chan_history(ic);
		}

		if (vdev->vdev_mlme.bss_chan) {
		    QDF_TRACE(QDF_MODULE_ID_DFS, QDF_TRACE_LEVEL_INFO,
				       "vdev[%d] ieee chan:%d freq:%d",
				       wlan_vdev_get_id(vdev),
				       vap->iv_bsschan->ic_ieee,
				       vap->iv_bsschan->ic_freq);
			ieee80211_update_vdev_chan(vdev->vdev_mlme.bss_chan,
						   vap->iv_bsschan);
		}
		if (vap->iv_bss) {
		     ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(vap->iv_bsschan),
						   vap->iv_bsschan->ic_freq,
						   vap->iv_bsschan->ic_vhtop_freq_seg1,
						   vap->iv_bsschan->ic_vhtop_freq_seg2
#ifdef WLAN_FEATURE_11BE
						   , vap->iv_bsschan->op_puncture_bitmap
#endif
						   );
		}

		ieee80211_set_ul_mumimo(vap);
		ic->ic_opmode = ieee80211_new_opmode(vap, true);

		vap->iv_enable_radar_table(ic, vap, 1, 1);
		vap->iv_peer_idle_time = 0;

		if(IEEE80211_IS_CHAN_DFS(ic->ic_curchan))
		{
			/*
			 * When there is radar detect in Repeater, repeater sends RCSAs, CSAs and
			 * switches to new next channel, in ind rpt case repeater AP could start
			 * beaconing before Root comes up, next channel needs to be changed
			 */
			if(!IEEE80211_IS_CHAN_VALID(ic->ic_tx_next_ch) ||
			   IEEE80211_ARE_CHANS_EQUAL(ic->ic_curchan, ic->ic_tx_next_ch))
				ieee80211_update_dfs_next_channel(ic);
		}
		else {
			*ic->ic_tx_next_ch = empty_channel;
		}
		// Tell the vap that the channel change has happened.
		/* For Band steering enabled:- it will be sent from ieee80211_state_event*/
		IEEE80211_DELIVER_EVENT_CHANNEL_CHANGE(vap, ic->ic_curchan);
		ieee80211com_clear_flags(ic, IEEE80211_F_DFS_CHANSWITCH_PENDING);
		vap->channel_switch_state = 0;

		if (restart) {
			uint8_t peer_ext_nss_update_count = 0;
			/* ni_phymode is updated based on iv_cur_mode if new channel
			* width is 160. Hence
			* update iv_cur_mode based on new target channel's
			* width.
			*/
			vap->iv_cur_mode = ieee80211_chan2mode(chan);
			ieee80211_update_peer_cw(ic, vap);
			wlan_iterate_station_list(vap,
				update_peer_nss_based_on_cap, &peer_ext_nss_update_count);
			if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
				/* Restart dtim-count in case of mbssid vap */
				ieee80211_mbssid_update_mbssie_cache_entry(
						vap, MBSS_CACHE_ENTRY_IDX);
			}
		}

		/*
		 * Update the ni->ni_txpower value with the maximum allowed
		 * txpower for the channel from regulatory. The Transmit power
		 * of the tpc report IE is filled with ni->ni_txpower.
		 */
		if (vap->iv_bss) {
			ieee80211node_set_txpower(vap->iv_bss,
						  ic->ic_curchan->ic_maxregpower * 2);
		}

		if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
			enum reg_6g_ap_type cur_pwr_type = REG_MAX_AP_TYPE;

			wlan_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &cur_pwr_type);
			ic->ic_curchan_eirp_power =
				wlan_reg_get_eirp_pwr(ic->ic_pdev_obj,
						      ic->ic_curchan->ic_freq,
						      ic->ic_curchan->ic_vhtop_freq_seg2,
						      ieee80211_get_chan_width(ic->ic_curchan),
						      cur_pwr_type,
						      ieee80211_get_chan_punc_bitmap(ic->ic_curchan),
						      false,
						      REG_MAX_CLIENT_TYPE, false);
		}

		if (opmode == QDF_SAP_MODE) {
			error = ieee80211_mlme_create_infra_continue(vap);
#ifdef IPA_OFFLOAD
			if ((!restart) && (!error)) {
				if (ucfg_ipa_is_enabled()) {
					psoc = wlan_pdev_get_psoc(pdev);
					wlan_event = WLAN_IPA_AP_CONNECT;
					ucfg_ipa_event_wq(psoc, vdev->vdev_nif.osdev->wdev->netdev->dev_addr,
							  vdev, wlan_event);
				}
			}
#endif
		} else {
			if (vap->iv_bss) {
				vap->iv_cur_mode = vap->iv_bss->ni_ch_param.ni_bss_phymode;
			}

			if (IEEE80211_IS_CHAN_VALID(ic->ic_curchan) &&
			    wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq))
				ieee80211_send_tpc_power_cmd(vap);
			error = wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
					WLAN_VDEV_SM_EV_START_SUCCESS, 0, NULL);
		}
		if (!error)
			IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(
					vap, IEEE80211_STATUS_SUCCESS);
		break;
	case QDF_STA_MODE:
		ni = vap->iv_bss;
		if (!ni) {
			if (restart) {
				mlme_vdev_sched_restart_bmiss(vdev);
				return QDF_STATUS_SUCCESS;
			}
			ieee80211_mlme_join_infra_continue(vap, EINVAL);
			return QDF_STATUS_E_FAILURE;
		}

		if (ni) {
		    chan_p.freq = ni->ni_ch_param.ni_freq;
		    chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
		    set_chan_params_puncture_bitmap(&chan_p, ni->ni_ch_param.puncture_bitmap);

		    ni_chan = ieee80211_find_dot11_channel(ic, &chan_p,
							   ni->ni_ch_param.ni_bss_phymode);

		    if (!IEEE80211_IS_CHAN_VALID(&ni_chan)) {
			return QDF_STATUS_E_FAILURE;
		    }
		}

		/*
		 * The channel sent for vdev restart/start is different from the
		 * BSS channel. Proceeding further results in invalid channel
		 * set for the STA. Stop connection SM and return failure. */
		if (!IEEE80211_ARE_CHANS_EQUAL(des_ic_chan, &ni_chan)) {
			if (restart) {
				mlme_vdev_sched_restart_bmiss(vdev);
				return QDF_STATUS_SUCCESS;
			}
			ieee80211_mlme_join_infra_continue(vap, EINVAL);
			return QDF_STATUS_E_FAILURE;
		}

		ieee80211_update_vdev_chan(vdev->vdev_mlme.bss_chan, chan);

		/* In case of rpt_max_phy for ind rpt, update curchan only
		 * if there are no active AP VAPs. If AP VAPs are active,
		 * curchan always points to max capable chan irrespective
		 * of STA VAP. Hence no update is required.
		 */
		IEEE80211_COMM_LOCK(ic);
		TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next)
			if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP) &&
			    (QDF_STATUS_SUCCESS != wlan_vdev_mlme_is_init_state(tmpvap->vdev_obj))) {
				update_chan = false;
				break;
			}
		IEEE80211_COMM_UNLOCK(ic);
		if (!ieee80211_ic_rpt_max_phy_is_set(ic)||
		    !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) ||
                    update_chan) {
			*ic->ic_prevchan = *ic->ic_curchan;
			*ic->ic_curchan = *chan;
		}

		*vap->iv_bsschan = *chan;
		if(restart && ni) {
		/* Updated chwidth & phymode is sent to FW as part of ol_ath_send_peer_assoc */
			ni->ni_chwidth = ic->ic_cwm_get_width(ic);
			ieee80211_update_phymode(ic, ni);

			/* Ensure that the mitigation logic for Tx/Rx Supported
			 * HE-MCS and NSS validation failures for 80+80/160 MHz
			 * applied in ieee80211_hecap_parse_mcs_nss() is
			 * preserved here. If the validation failure had
			 * occurred for HE80, HE capability would have been
			 * marked as unavailable in ni flags by
			 * ieee80211_hecap_parse_mcs_nss(), and the ni would
			 * not have an HE PHY mode, hence there is no need to
			 * consider HE80 below.
			*/
			switch(ni->ni_phymode) {
			case IEEE80211_MODE_11AXA_HE80_80:
				if (!ni->ni_he.he_basic_txrxmcsnss_req_met_80_80) {
					qdf_info("Valid Tx/Rx Supported HE-MCS and NSS info for 80+80 MHz not found - adjusting ni chwidth and phymode to 160 MHz");
					ni->ni_chwidth = IEEE80211_CWM_WIDTH160;
					ni->ni_phymode =
						IEEE80211_MODE_11AXA_HE160;
				}
				/* Fall through */
			case IEEE80211_MODE_11AXA_HE160:
				if (!ni->ni_he.he_basic_txrxmcsnss_req_met_160) {
					qdf_info("Valid Tx/Rx Supported HE-MCS and NSS info for 160 MHz not found - adjusting ni chwidth and phymode to 80 MHz");
					ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
					ni->ni_phymode =
						IEEE80211_MODE_11AXA_HE80;
				}
				break;
#ifdef WLAN_FEATURE_11BE
			case IEEE80211_MODE_11BEA_EHT320:
				if (!ni->ni_eht.eht_basic_txrxmcsnss_req_met_320) {
					qdf_info("Valid Tx/Rx Supported EHT-MCS and NSS info for 320 MHz not found - adjusting ni chwidth and phymode to 160 MHz");
					ni->ni_chwidth = IEEE80211_CWM_WIDTH160;
					ni->ni_phymode =
						IEEE80211_MODE_11BEA_EHT160;
				}
				/* Fall through */
			case IEEE80211_MODE_11BEA_EHT160:
				if (!ni->ni_eht.eht_basic_txrxmcsnss_req_met_160) {
					qdf_info("Valid Tx/Rx Supported EHT-MCS and NSS info for 160 MHz not found - adjusting ni chwidth and phymode to 80 MHz");
					ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
					ni->ni_phymode =
						IEEE80211_MODE_11BEA_EHT80;
				}
				break;
#endif /* WLAN_FEATURE_11BE */
			}
		}

		/* update max channel power to max regpower of current channel */
		ieee80211com_set_curchanmaxpwr(ic, chan->ic_maxregpower);
		/* Update the channel for monitor mode path */
		cdp_set_curchan(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(pdev), chan->ic_freq);

		/* ieee80211 Layer - Default Configuration */
		if (!ieee80211_ic_rpt_max_phy_is_set(ic)||
		    !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) ||
                    update_chan)
			*vap->iv_bsschan = *ic->ic_curchan;

		ieee80211_set_ul_mumimo(vap);

		/* XXX reset erp state */
		ieee80211_reset_erp(ic, ic->ic_curmode, vap->iv_opmode);
		ieee80211_wme_initparams(vap);

		vap->iv_enable_radar_table(ic, vap, 0, 0);
		if (wlan_cm_get_active_req_type(vap->vdev_obj) == CM_ROAM_ACTIVE)
			wlan_cm_get_active_reassoc_req(vap->vdev_obj, &req);

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
		if (mlme_sta_cac_start(ic, vap) == QDF_STATUS_SUCCESS) {
			wlan_mlme_inc_act_cmd_timeout(vdev,
					WLAN_SER_CMD_VDEV_CONNECT);
			wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
				WLAN_VDEV_SM_EV_DFS_CAC_WAIT, 0, NULL);
		}
		else
#endif
		{
			if (mlo_is_sta_bridge_vdev(vdev)) {
#ifdef WLAN_FEATURE_11BE_MLO
				/* Acquire lock as required by wlan_vdev_mlme_get_mldaddr() */
				wlan_vdev_obj_lock(vap->vdev_obj);
				curbssid = wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
				wlan_vdev_obj_unlock(vap->vdev_obj);
#endif
				error = wlan_assoc_sm_start(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj),
							    NULL,
							    curbssid);
			} else {

				error = wlan_assoc_sm_start(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj),
							    mlme_cm_get_active_scan_entry(vap),
							    req.prev_bssid.bytes);
			}
			if (error != EOK && !restart)
				qdf_err("VDEV:%d Assoc sm start failure",
					wlan_vdev_get_id(vdev));

			ieee80211_mlme_join_infra_continue(vap,EOK);
		}

		if (!vap->iv_quick_reconnect && restart) {
			qdf_err("VDEV:%d Restart, send start success",
				wlan_vdev_get_id(vdev));
			wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
				WLAN_VDEV_SM_EV_START_SUCCESS, 0, NULL);
		}
		// Tell the vap that the channel change has happened.
		/* For Band steering enabled:- it will be sent from ieee80211_state_event*/
		IEEE80211_DELIVER_EVENT_CHANNEL_CHANGE(vap, ic->ic_curchan);
		ieee80211com_clear_flags(ic, IEEE80211_F_DFS_CHANSWITCH_PENDING);
		vap->channel_switch_state = 0;
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_sta_conn_start_cb(struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct ieee80211vap *vap;
	struct ieee80211_mlme_priv *mlme_priv;
	struct wlan_objmgr_vdev *vdev;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	mlme_priv = vap->iv_mlme_priv;
	if (!mlme_priv) {
		mlme_err("(vdev-id:%d) mlme_priv  is NULL",
			 wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
	mlme_set_stacac_valid(vap,1);
#endif
	wlan_assoc_sm_start(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj),
			    mlme_cm_get_active_scan_entry(vap), NULL);
	mlme_priv->im_request_type = MLME_REQ_JOIN_INFRA;
	ieee80211_mlme_join_infra_continue(vap,EOK);
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
	mlme_priv->im_is_stacac_running = 0;
#endif
	qdf_info("STACAC expired");

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_sta_disconn_start_cb(struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct ieee80211vap *vap;
	struct wlan_objmgr_vdev *vdev;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	if (vap->iv_opmode == IEEE80211_M_STA)
		wlan_assoc_sm_stop(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj), 0);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_notify_link_update_event_cb(struct wlan_objmgr_vdev *vdev, void *t2lm)
{
	enum QDF_OPMODE opmode;

	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	opmode = wlan_vdev_mlme_get_opmode(vdev);
	if (opmode == QDF_SAP_MODE) {
		mlme_info("Attempting to send NL event to userspace");
		if(wlan_cfg80211_send_link_unavailable_update_event(vdev, (struct wlan_t2lm_info *)t2lm) != QDF_STATUS_SUCCESS) {
			return QDF_STATUS_E_FAILURE;
		}
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_start_req_failed_cb(struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct ieee80211vap *vap;
	struct wlan_objmgr_vdev *vdev;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	if (vap->iv_opmode == IEEE80211_M_STA) {
		ieee80211_reset_bss(vap);
		ieee80211_mlme_join_infra_continue(vap, EINVAL);
	}
	else if (vap->iv_opmode == IEEE80211_M_HOSTAP)
		ieee80211_mlme_create_infra_continue_async(vap, EINVAL);

	return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_11BE_MLO
void mlme_mlo_vdev_set_mcast_to_ucast(struct ieee80211vap *vap)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
	struct ieee80211vap *link_vap = NULL;
	uint16_t vdev_count = 0;
	int i = 0;
	int ret = 0;
	int mode = 0;
	int curr_mode = 0;

	if (!vap)
		return;

	vdev = vap->vdev_obj;
	mode = vap->iv_mcast_to_ucast_mode;

	if (mode) {
		if (wlan_vdev_mlme_is_mlo_vdev(vdev)) {
			mlo_get_ml_vdev_list(vap->vdev_obj, &vdev_count, wlan_vdev_list);
			for (i = 0; i < vdev_count; i++) {
				vdev = wlan_vdev_list[i];
				if (!vdev)
					return;

				link_vap = wlan_vdev_mlme_get_ext_hdl(vdev);
				if (!link_vap)
					break;

				ieee80211_ucfg_getparam(link_vap,
						IEEE80211_PARAM_ME, &curr_mode);
				/*set mode(5)for any link_vap with mode(0)*/
				if (!curr_mode) {
					ieee80211_ucfg_setparam_on_partner_vap(
						vap, IEEE80211_PARAM_ME,
						mode, NULL);
					break;
				}
			}

			if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
						QDF_ARRAY_SIZE(wlan_vdev_list),
						vdev_count, 0) != QDF_STATUS_SUCCESS) {
				mlme_err("failed to release mlo vdev ref");
			}
		}
		/*set mode on curr vap*/
		ret = ieee80211_ucfg_setparam(vap, IEEE80211_PARAM_ME, mode, NULL);
	}

	if (ret < 0) {
		mlme_err("failed to set mcast_to_ucast mode:%d on (vdev-id:%d) vap",
						mode, wlan_vdev_get_id(vdev));
	}
}
#endif

QDF_STATUS mlme_vdev_notify_up_complete_cb(
                        struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_vdev *vdev;
	struct ieee80211vap *vap;
	ieee80211_vap_event evt;
	struct wlan_channel *iter_des_chan = NULL;
	struct ieee80211vap *tmpvap;
	struct ieee80211com *ic;
	uint8_t restart_progress = 0;
	uint8_t issue_evt = 0;
	enum wlan_vdev_sm_evt event;
	cm_ext_t *cm_ext_handle = NULL;
	int value = 0;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	evt.type = IEEE80211_VAP_UP;
	ieee80211_vap_deliver_event(vap, &evt);

	/* Signal the event when VDEV is brought up completely. */
	qdf_event_set(&vap->wait_event_vdev_up_state);

	IEEE80211_DELIVER_EVENT_VDEV_STATE_UP_ACTIVE_INDICATION(vap);

	ic = vap->iv_ic;

	if (vap->iv_opmode == IEEE80211_M_HOSTAP)
           wlan_vdev_mlme_op_flags_clear(vdev, WLAN_VDEV_OP_MLO_LINK_ADD);

	/* If any VDEV has moved to RESTART_PROGRESS substate, but this VDEV
	 * missed the event, then move to RESTART_PROGRESS sub state.
	 * Or, if Channel switch announcement is triggered and announcement is
	 * still in progress, but this vdev is missed the event, then move to
	 * CSA_RESTART sub state.
	 */
	if ((wlan_pdev_mlme_op_get(ic->ic_pdev_obj,
				   WLAN_PDEV_OP_RESTART_INPROGRESS) ||
	     (ic->ic_flags & IEEE80211_F_CHANSWITCH)) &&
	    (!wlan_pdev_mlme_op_get(ic->ic_pdev_obj,
				    WLAN_PDEV_OP_RADAR_DETECT_DEFER))) {
		TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
			if (wlan_vdev_is_restart_progress(tmpvap->vdev_obj) ==
						QDF_STATUS_SUCCESS) {
				iter_des_chan = wlan_vdev_mlme_get_des_chan(
							tmpvap->vdev_obj);
				restart_progress = 1;
				event = WLAN_VDEV_SM_EV_FW_VDEV_RESTART;
				mlme_err("vdev (%d)Issuing vdev SM_EV_FW_VDEV_RESTART",
					 wlan_vdev_get_id(vdev));
				issue_evt = 1;
				break;
			}
		}

		if (!restart_progress && (ic->ic_flags & IEEE80211_F_CHANSWITCH)) {
			mlme_err("Starting MVR for Pdev %d Psoc %d",
					wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj),
					wlan_psoc_get_id(wlan_pdev_get_psoc(ic->ic_pdev_obj)));

			wlan_pdev_mlme_op_set(ic->ic_pdev_obj, WLAN_PDEV_OP_MBSSID_RESTART);
			restart_progress = 1;
			vap->iv_chanchange_count = 0;
			issue_evt = 1;

			if (vdev->vdev_mlme.des_chan)
				ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan,
						ic->ic_chanchange_channel);

			if (vap->iv_opmode == IEEE80211_M_STA) {
				mlme_err("vap:%s vdev:%u Issuing SM_EV_FW_VDEV_RESTART",
					vap->iv_netdev_name,
					wlan_vdev_get_id(vap->vdev_obj));
				event = WLAN_VDEV_SM_EV_FW_VDEV_RESTART;
			} else {
				mlme_err("vdev (%d)Issuing vdev SM_EV_CSA_RESTART",
					 wlan_vdev_get_id(vdev));
				event = WLAN_VDEV_SM_EV_CSA_RESTART;
			}
		}
	}

	/* If any VDEV has moved to CSA_RESTART sub state, but this non-sta VDEV
	 * missed event, then move to CSA_RESTART sub state
	 */
	if ((!restart_progress) &&
	    (wlan_vdev_mlme_get_opmode(vdev) != QDF_STA_MODE)) {
		TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
			if (wlan_vdev_mlme_is_csa_restart(tmpvap->vdev_obj) ==
						QDF_STATUS_SUCCESS) {
				iter_des_chan = wlan_vdev_mlme_get_des_chan(
							tmpvap->vdev_obj);
				event = WLAN_VDEV_SM_EV_CSA_RESTART;
				mlme_err("vdev (%d)Issuing vdev SM_EV_CSA_RESTART",
				 wlan_vdev_get_id(vdev));
				issue_evt = 1;
				break;
			}
		}
	}
	if (iter_des_chan)
		wlan_chan_copy(vdev->vdev_mlme.des_chan, iter_des_chan);

	if (issue_evt)
		wlan_vdev_mlme_sm_deliver_evt_sync(vdev, event, 0, NULL);

	if (vap->iv_opmode == IEEE80211_M_STA) {
		cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
		if (!cm_ext_handle)
			return QDF_STATUS_E_FAILURE;

		cm_ext_handle->cm_conn_rsp.connect_status = QDF_STATUS_SUCCESS;
		wlan_mlme_dispatch_cm_resp(vap, CM_CONNECT_RESP);
		return QDF_STATUS_SUCCESS;
	}
#ifdef WLAN_FEATURE_11BE_MLO
	/*set multicast_to_unicast mode based on UCI config*/
	mlme_mlo_vdev_set_mcast_to_ucast(vap);
#else
	if (vap->iv_mcast_to_ucast_mode)
		ieee80211_ucfg_setparam(vap, IEEE80211_PARAM_ME,
					vap->iv_mcast_to_ucast_mode, NULL);
#endif
	wlan_mlme_release_vdev_req(vdev_mlme->vdev,
				   WLAN_SER_CMD_VDEV_START_BSS, EOK);

	ieee80211_ucfg_getparam(vap, IEEE80211_PARAM_VDEV_TSF, &value);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_update_beacon_cb(struct vdev_mlme_obj *vdev_mlme,
                        enum beacon_update_op op,
			uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_pdev *pdev;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
        struct ieee80211_quiet_param quiet_ic = {0};
	QDF_STATUS status;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;

	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	if (pdev == NULL) {
		mlme_err("(vdev-id:%d) PDEV is NULL",
				wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic) {
		mlme_err("(vdev-id:%d) ic is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	switch (op) {
	case BEACON_INIT:
		/* handled as part of vap->iv_up() */
		break;
	case BEACON_UPDATE:
		break;
	case BEACON_CSA:
		/* If there is at least one active AP VAP then it
		 * will finish the CSA and post the following event
		 * (WLAN_VDEV_SM_EV_CSA_COMPLETE) to the monitor/MESH VAP's
		 * state machine. */
		if ((vap->iv_opmode == IEEE80211_M_MONITOR)
		    || wlan_is_non_beaconing_ap_vap(vap)) {
			if (!ieee80211_get_num_beaconing_vaps(ic) ||
			    !ieee80211_num_apvap_running(ic)) {
				/* In case of only vaps in monitor or/and no-beacon
				 * mesh mode, no CSA is required,
				 * move to RESTART state. Also if monitor vdev has missed CSA_COMPLETE
				 * due to delay in processing EV_CSA_RESTART, then deliver EV_CSA_COMPLETE
				 * to move to RESTART state
				 */
				qdf_err("vap:%s vdev:%d pdev:%d psoc:%d state:%d substate:%d opmode:%d iv_special_vap:%d iv_smart_monitor:%d",
					vap->iv_netdev_name, wlan_vdev_get_id(vdev),
					wlan_objmgr_pdev_get_pdev_id(pdev),
					wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)),
					wlan_vdev_mlme_get_state(vdev),
					wlan_vdev_mlme_get_substate(vdev),
					vap->iv_opmode, vap->iv_special_vap_mode,
					vap->iv_smart_monitor_vap);

				wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
					WLAN_VDEV_SM_EV_CSA_COMPLETE, 0, NULL);

				if (ic->ic_sta_vap &&
				    QDF_IS_STATUS_SUCCESS(wlan_vdev_is_up(ic->ic_sta_vap->vdev_obj))) {
					status = wlan_objmgr_pdev_try_get_ref(pdev, WLAN_OSIF_SCAN_ID);
					if (QDF_IS_STATUS_ERROR(status)) {
						qdf_info("unable to get reference");
					} else {
						ucfg_scan_flush_results(pdev, NULL);
						wlan_objmgr_pdev_release_ref(pdev, WLAN_OSIF_SCAN_ID);
					}

					ieee80211_indicate_sta_radar_detect(ic->ic_sta_vap->iv_bss);
				}
			} else if(wlan_pdev_mlme_op_get(pdev, WLAN_PDEV_OP_RESTART_INPROGRESS)) {
				/* If monitor vdev has missed CSA_COMPLETE
				 * due to delay in processing EV_CSA_RESTART, then deliver EV_CSA_COMPLETE
				 * to move to RESTART state
				 */
				qdf_err("vap:%s vdev:%d pdev:%d psoc:%d state:%d substate:%d opmode:%d iv_special_vap:%d iv_smart_monitor:%d",
					vap->iv_netdev_name, wlan_vdev_get_id(vdev),
					wlan_objmgr_pdev_get_pdev_id(pdev),
					wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)),
					wlan_vdev_mlme_get_state(vdev),
					wlan_vdev_mlme_get_substate(vdev),
					vap->iv_opmode, vap->iv_special_vap_mode,
					vap->iv_smart_monitor_vap);
				wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
					WLAN_VDEV_SM_EV_CSA_COMPLETE, 0, NULL);
			}
		}
		else {
                        ic->ic_radio_set_bcn_offload_quiet_mode(ic, &quiet_ic);
			ic->ic_flags |= IEEE80211_F_CHANSWITCH;
			ic->ic_flags_ext2 |= IEEE80211_FEXT2_CSA_WAIT;
			wlan_vdev_beacon_update(vap);
		}
		break;
	case BEACON_FREE:
		ic->ic_beacon_free(vap);
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

static
void mlme_vdev_peer_timeout_rel_handler(struct ieee80211vap *vap)
{
	qdf_list_t *logical_del_peerlist;
	struct wlan_objmgr_peer *peer;
	qdf_list_node_t *peerlist;
	struct wlan_logically_del_peer *temp_peer = NULL;
	struct ieee80211_node *ni;

	if (!vap || !vap->iv_peer_rel_ref) {
		mlme_err("Null vap, vap: 0x%pK", vap);
		return;
	}

	logical_del_peerlist =
		wlan_objmgr_vdev_get_log_del_peer_list(vap->vdev_obj,
						       WLAN_MLME_SB_ID);
	if (!logical_del_peerlist)
		return;

	while (QDF_IS_STATUS_SUCCESS(qdf_list_remove_front(logical_del_peerlist,
				     &peerlist))) {
		temp_peer = qdf_container_of(peerlist,
					     struct wlan_logically_del_peer,
					     list);
		peer = temp_peer->peer;
		ni = wlan_peer_get_mlme_ext_obj(peer);

		mlme_err("peer mac: %s", ether_sprintf(peer->macaddr));

		if (vap->iv_peer_rel_ref(vap, ni, peer->macaddr))
			mlme_err("Failed to handle peer del failure");
		/*
		 * Release the ref taken during wlan_objmgr_vdev_get_log_del_peer_list
		 */
		wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
		qdf_mem_free(temp_peer);
	}
	qdf_list_destroy(logical_del_peerlist);
	qdf_mem_free(logical_del_peerlist);
}

static
void mlme_print_peer_refs_checking_mlo_peer_delete_allowed(struct ieee80211vap *vap,
							   bool assert)
{
	if (wlan_vdev_get_peer_count(vap->vdev_obj) > 1) {
		if (wlan_mlo_peer_delete_is_not_allowed(vap->vdev_obj)) {
			if (wlan_vdev_get_legacy_peer_count(vap->vdev_obj) > 1) {
				if (vap->iv_ic->ic_print_peer_refs)
					vap->iv_ic->ic_print_peer_refs(vap->vdev_obj, assert);
			}
		} else {
			if (vap->iv_ic->ic_print_peer_refs)
				vap->iv_ic->ic_print_peer_refs(vap->vdev_obj, assert);
		}
	}
}

static os_timer_func(mlme_vdev_peer_del_timeout_handler)
{
	struct ieee80211vap *vap;
	struct wlan_objmgr_psoc *psoc;
	ol_txrx_soc_handle soc_dp_handle;
	struct ol_ath_softc_net80211 *scn;
	ol_ath_soc_softc_t *soc;

	OS_GET_TIMER_ARG(vap, struct ieee80211vap *);
	if (wlan_vdev_get_peer_count(vap->vdev_obj) <= 1)
		return;

	psoc = wlan_vdev_get_psoc(vap->vdev_obj);
	if (!psoc)
		return;

	soc_dp_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_dp_handle)
		return;

	scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
	soc = scn->soc;
	if (!soc)
		return;

	/* Restart the timer if UMAC reset is inprogress */
	if (cdp_umac_reset_is_inprogress(soc_dp_handle)) {
		qdf_info("soc_id:%d Umac reset is in progress, restart the peer delete timer",
			 psoc->soc_objmgr.psoc_id);
		qdf_timer_mod(&vap->peer_del_wait_timer, soc->peer_del_wait_time);
		return;
	}

	mlme_err("Print peer refs timed out for vdev:%u",
		 wlan_vdev_get_id(vap->vdev_obj));
#if VDEV_ASSERT_MANAGEMENT
	/*
	 * Release references held for pending peer delete response
	 */
	mlme_vdev_peer_timeout_rel_handler(vap);

	mlme_print_peer_refs_checking_mlo_peer_delete_allowed(vap, false);
#else
	/*
	 * Print any leaked peer refs and do FW assert if there are any missing
	 * peer delete responses or mgmt completions for STA peers
	 */
	mlme_print_peer_refs_checking_mlo_peer_delete_allowed(vap, true);
#endif

	return;
}

static os_timer_func(mlme_vdev_peer_del_bss_tid_flush)
{
	struct ieee80211vap *vap;
	struct wlan_objmgr_vdev *vdev;
	struct ol_ath_softc_net80211 *scn;
	ol_ath_soc_softc_t *soc;

	OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

	vdev = vap->vdev_obj;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return;
	}

	mlme_err("Peer del wait timed out for vdev:%u, flush bss peer tids",
		 wlan_vdev_get_id(vdev));


	if (wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_SUSPEND) {
		mlme_err("VDEV %u: is not in SUSPEND state, current state %d",
			 wlan_vdev_get_id(vdev),
			 wlan_vdev_mlme_get_state(vdev));
		return;
	}

	/* flush bss peer tids */
	mlme_ext_vap_flush_bss_peer_tids(vap);

	if (wlan_vdev_get_peer_count(vdev) <= 1)
		return;

	/* Applicable for only lithium platform */
	if (vap->iv_ic->ic_print_peer_refs) {
		scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
		soc = scn->soc;

		qdf_timer_mod(&vap->peer_del_wait_timer,
			      soc->peer_del_wait_time);
	}

	return;
}

static void mlme_vdev_cleanup_peers(struct ieee80211vap *vap)
{
	if (wlan_vdev_get_peer_count(vap->vdev_obj) == 1)
		return;

	/* Flush any pending mgmt frame in host queue for this vap */
	ieee80211_flush_vap_mgmt_queue(vap, false);
#ifdef WLAN_FEATURE_11BE_MLO
	/* cleanup all partner sta bridge peers, if needed */
	wlan_iterate_station_list(vap, cleanup_sta_partner_bridge_peer, NULL);
#endif
	/* cleanup all associated sta peers on this vap */
	wlan_iterate_station_list(vap, cleanup_sta_peer, NULL);
	/* cleanup all unassociated sta peers on this vap */
	wlan_iterate_unassoc_sta_list(vap, cleanup_sta_peer, NULL);
	/* flush bss peer tids */
	mlme_ext_vap_flush_bss_peer_tids(vap);

	return;
}

static os_timer_func(mlme_vdev_force_cleanup_peers)
{
	struct ieee80211vap *vap;

	OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

	mlme_vdev_cleanup_peers(vap);
}

static void mlme_vdev_get_temp_peer_count(
					struct wlan_objmgr_vdev *vdev,
					void *object, void *arg)
{
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
	uint16_t *temp_peers = arg;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_STA_TEMP ||
	    wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_TEMP)
		(*temp_peers)++;
}

/* Get logically deleted per count */
static uint16_t
mlme_vdev_get_lstate_peer_count(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_logically_del_peer *temp_peer = NULL;
	qdf_list_t *logical_del_peerlist = NULL;
	qdf_list_node_t *peerlist = NULL;
	uint16_t lstate_peer_cnt = 0;
	struct wlan_objmgr_peer *peer;

	logical_del_peerlist =
		wlan_objmgr_vdev_get_log_del_peer_list(vdev,
						       WLAN_MLME_SB_ID);
	if (!logical_del_peerlist)
		return 0;

	while (QDF_IS_STATUS_SUCCESS(qdf_list_remove_front(logical_del_peerlist,
				     &peerlist))) {
		temp_peer = qdf_container_of(peerlist,
					     struct wlan_logically_del_peer,
					     list);
		peer = temp_peer->peer;
		lstate_peer_cnt++;
		wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
		qdf_mem_free(temp_peer);
	}
	qdf_list_destroy(logical_del_peerlist);
	qdf_mem_free(logical_del_peerlist);
	return lstate_peer_cnt;
}

QDF_STATUS mlme_vdev_send_deauth(struct ieee80211vap *vap,
				 bool discon_legacy_only)
{
	bool send_bcast_deauth = false;
	bool send_ucast_deauth = false;
	struct vdev_mlme_obj *vdev_mlme;
	struct wlan_objmgr_psoc *psoc;
	uint8_t peer_del_req_send = MLME_INDIVIDUAL_PEER_CLEANUP;
	extern void sta_deauth (void *arg, struct ieee80211_node *ni);
	struct vdev_response_timer *vdev_rsp;
	struct psoc_mlme_obj *psoc_mlme;
	uint8_t vdev_id;
	uint16_t temp_peers = 0;
	uint16_t lstate_peers = 0;
	int16_t total_peers = 0;
	bool mlo_reconfig = false;

	if (vap->iv_csa_deauth_mode == CSA_DEAUTH_MODE_UNICAST)
		send_ucast_deauth = true;
	else if (vap->iv_csa_deauth_mode == CSA_DEAUTH_MODE_BROADCAST)
		send_bcast_deauth = true;
#ifdef MESH_MODE_SUPPORT
        else if (vap->iv_mesh_vap_mode) {
		send_ucast_deauth = true;
        }
#endif
	else {
		if (g_unicast_deauth_on_stop)
			send_ucast_deauth = true;
		else
			send_bcast_deauth = true;
	}

	mlo_reconfig = wlan_vdev_mlme_op_flags_get(vap->vdev_obj,
						   WLAN_VDEV_OP_MLO_STOP_LINK_DEL);
        /* Note: Remove mlo_reconifg check once primary UMAC migration is
           implemented in FW
         */
	if ((discon_legacy_only) &&
	    (wlan_vdev_get_legacy_peer_count(vap->vdev_obj) <= 1)) {
		return QDF_STATUS_SUCCESS;
	}

	if (send_ucast_deauth || wlan_vap_is_pmf_enabled(vap) ||
	    mlo_reconfig || discon_legacy_only) {
		mlme_nofl_info("Sending ucast %s to %s associated stas",
			       vap->iv_send_deauth ? "deauth" : "disassoc",
			       wlan_vap_is_pmf_enabled(vap) ? "PMF" : "ALL");

		if (!vap->iv_stealthdown) {
#ifdef WLAN_FEATURE_11BE_MLO
                        wlan_iterate_station_list(vap, cleanup_sta_partner_bridge_peer,
                                                  NULL);
#endif
			if (vap->iv_send_deauth)
				wlan_iterate_station_list(vap, sta_deauth,
							  &peer_del_req_send);
			else
				wlan_iterate_station_list(vap, sta_disassoc,
							  &peer_del_req_send);

			wlan_iterate_unassoc_sta_list(vap, sta_deauth,
						      &peer_del_req_send);
		} else {
			/* cleanup all sta peers on this vap */
			mlme_vdev_cleanup_peers(vap);
		}
	} else {
		/* send peer delete all only when fw capability is set */
		if (wlan_pdev_nif_feat_cap_get(
					wlan_vdev_get_pdev(vap->vdev_obj),
					WLAN_PDEV_F_DELETE_ALL_PEER))
			peer_del_req_send = MLME_MULTIPLE_PEER_CLEANUP;

		vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vap->vdev_obj);
		psoc = wlan_vdev_get_psoc(vap->vdev_obj);
		if (!psoc) {
			mlme_nofl_err("PSOC is NULL");
			return QDF_STATUS_E_FAILURE;
		}

		wlan_objmgr_iterate_peerobj_list(
					vap->vdev_obj,
					mlme_vdev_get_temp_peer_count,
					&temp_peers,
					WLAN_MLME_SB_ID);

		lstate_peers = mlme_vdev_get_lstate_peer_count(vap->vdev_obj);
		total_peers = wlan_vdev_get_peer_count(vap->vdev_obj) - temp_peers - lstate_peers;
		mlme_nofl_info("PSOC_%d VDEV_%d Sending bcast deauth and %s, clients: %d",
                               wlan_psoc_get_id(psoc),
			       wlan_vdev_get_id(vap->vdev_obj),
			       (peer_del_req_send == MLME_MULTIPLE_PEER_CLEANUP ?
			       "DELETE_ALL_PEER":"PEER_DELETE"),
			       total_peers);
		if (!(vap->iv_stealthdown || mlo_reconfig)) {
			if (vap->iv_send_deauth) {
				/* Send broadcast deauth frames twice */
				ieee80211_send_deauth(vap->iv_bss,
						      IEEE80211_REASON_AUTH_LEAVE);
				ieee80211_send_deauth(vap->iv_bss,
						      IEEE80211_REASON_AUTH_LEAVE);
			} else {
				/* Send broadcast deauth frames twice */
				ieee80211_send_disassoc_with_callback(vap->iv_bss,
							IEEE80211_REASON_ASSOC_LEAVE,
							ieee80211_mlme_frame_complete_handler,
							vap->iv_bss);
				ieee80211_send_disassoc_with_callback(vap->iv_bss,
							IEEE80211_REASON_ASSOC_LEAVE,
							ieee80211_mlme_frame_complete_handler,
							vap->iv_bss);
			}
		}
		/* cleanup all associated sta peers on this vap */
		wlan_iterate_station_list(vap, cleanup_sta_peer,
					  &peer_del_req_send);
		/* cleanup all unassociated sta peers on this vap */
		wlan_iterate_unassoc_sta_list(vap, cleanup_sta_peer,
					      &peer_del_req_send);

		vdev_id = wlan_vdev_get_id(vap->vdev_obj);
		psoc_mlme = mlme_psoc_get_priv(psoc);
		if (!psoc_mlme) {
			mlme_err("VDEV_%d PSOC_%d PSOC_MLME is NULL", vdev_id,
					wlan_psoc_get_id(psoc));
			return QDF_STATUS_E_FAILURE;
		}

		vdev_rsp =  &psoc_mlme->psoc_vdev_rt[vdev_id];

		/* send peer delete all cmd */
		if (peer_del_req_send == MLME_MULTIPLE_PEER_CLEANUP &&
		    (total_peers > 1) &&
		    !qdf_atomic_test_bit(PEER_DELETE_ALL_RESPONSE_BIT,
					 &vdev_rsp->rsp_status)) {
			if (vdev_mgr_peer_delete_all_send(vdev_mlme)
							== QDF_STATUS_SUCCESS) {
#ifdef QCA_SUPPORT_CP_STATS
			    vdev_cp_stats_peer_delete_all_req_inc(
							vap->vdev_obj, 1);
#endif
			} else {
				mlme_vdev_peer_timeout_rel_handler(vap);
			}
		}
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_disconnect_peers_cb(
                        struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len,void *event_data,
			bool discon_legacy_only)
{
	struct wlan_objmgr_vdev *vdev;
	struct ieee80211_mlme_priv *mlme_priv;
	struct ieee80211vap *vap;
	struct ol_ath_softc_net80211 *scn;
	ol_ath_soc_softc_t *soc = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_lmac_if_rx_ops *rx_ops = NULL;
	struct wlan_vdev_state_event event;
	uint8_t flags = 0;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}
	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL",
			wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	mlme_priv = vap->iv_mlme_priv;
	switch(vap->iv_opmode) {
	case IEEE80211_M_HOSTAP:
	case IEEE80211_M_BTAMP:
		/* Reset connection state, so that no new connections occur */
		mlme_priv->im_connection_up = 0;
		/* disassoc/deauth all stations */
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME|IEEE80211_MSG_AUTH,
			"%s: disassoc/deauth all stations. peer_cnt: %d\n",
			__func__, wlan_vdev_get_peer_count(vdev));

		/* flush mgmt frames for this vap to make down faster */
		ieee80211_flush_vap_mgmt_queue(vap, false);

		if (wlan_vdev_get_peer_count(vdev) == 1) {
			mlme_vdev_sm_notify_peers_disconnected(vap);
			return QDF_STATUS_SUCCESS;
		}
		if (discon_legacy_only) {
			wlan_vdev_mlme_op_flags_set(vdev,
						    WLAN_VDEV_OP_MLME_LEGACY_PEER_DISCON_TRIG);
		} else {
			wlan_vdev_mlme_op_flags_clear(vdev,
						      WLAN_VDEV_OP_MLME_LEGACY_PEER_DISCON_TRIG);
		}
		if ((discon_legacy_only) &&
		    (wlan_vdev_get_legacy_peer_count(vdev) == 1)) {
			mlme_vdev_sm_notify_peers_disconnected(vap);
			return QDF_STATUS_SUCCESS;
		}
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME|IEEE80211_MSG_AUTH,
			"%s: sending %s deauth to all stations.\n",
			__func__, g_unicast_deauth_on_stop ? "UNICAST" : "BROADCAST");

		if (vap->force_cleanup_peers)
			mlme_vdev_cleanup_peers(vap);
		else
			mlme_vdev_send_deauth(vap, discon_legacy_only);

		vap->force_cleanup_peers = 0;

		qdf_timer_mod(&vap->peer_cleanup_timer, 1000);

		scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
		soc = scn->soc;
		qdf_timer_mod(&vap->peer_del_bss_tid_flush,
			      soc->peer_del_wait_time);
		break;

	case IEEE80211_M_STA:
		psoc = wlan_vdev_get_psoc(vdev);
		if (psoc) {
			rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
		}
		/* There should be no mlme requests pending */
		ASSERT(vap->iv_mlme_priv->im_request_type == MLME_REQ_NONE);

		/* Reset state variables */
		mlme_priv->im_connection_up = 0;
		mlme_sta_swbmiss_timer_stop(vap);
		event.state = VDEV_STATE_DISCONNECTED;
		if (rx_ops && rx_ops->son_rx_ops.deliver_event)
			rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_VDEV_STATE, &event);

		 /* Station BSS peer is deleted after sending STOP request, here
		  * connection sm gives event to move to STOP state*/

		/* Send disconnect complete to VDEV SM, if the connection SM
		 * moved to init state, and the disconnect complete sent from
		 * connection SM is not handled, since we can move to RESTART
		 * before moving to SUSPEND and we need disconnect event to
		 * move from SUSPEND to STOP state
		 */
		if (vdev_mlme->ml_reconfig_started != true)
			flags |= IEEE80211_ASSOC_SM_STOP_DISASSOC;
		if (wlan_assoc_sm_stop(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj),
				       flags))
			wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
				WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE, 0, NULL);
		break;

	case IEEE80211_M_MONITOR:
		wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
			WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE, 0, NULL);
		break;
	default:
		break;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_dfs_cac_timer_stop_cb(
		struct vdev_mlme_obj *vdev_mlme,
		uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_pdev *pdev;
	struct ieee80211vap *vap;
	struct ieee80211com *ic;
	uint8_t force = 0;

	vap = wlan_vdev_get_mlme_ext_obj(vdev_mlme->vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL",
			 wlan_vdev_get_id(vdev_mlme->vdev));
		return QDF_STATUS_E_FAILURE;
	}
	ic = vap->iv_ic;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		mlme_err("pdev is null");
		return QDF_STATUS_E_FAILURE;
	}

	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return QDF_STATUS_E_FAILURE;
	}
	if(!force && (!mlme_dfs_is_ap_cac_timer_running(pdev) ||
				ieee80211_dfs_vaps_in_dfs_wait(ic, vap))) {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
		return QDF_STATUS_E_FAILURE;
	}
	mlme_dfs_cac_stop(pdev);
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);

	return QDF_STATUS_SUCCESS;
}

static void mlme_vdev_cleanup(struct ieee80211vap *vap)
{
    struct ieee80211vap *tmpvap;
    struct ieee80211com *ic = vap->iv_ic;

#if WLAN_SUPPORT_FILS
        /* Free FILS FD buffer on receiving stopped event */
     vap->iv_cleanup(vap);
#endif
     /* 6Ghz 20 tu probe response free */
     if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan))
         ic->ic_prb_rsp_free(vap);

     if (vap->iv_he_bsscolor_detcn_configd_vap) {
         /* if bsscolor collision detection
          * event was registered for this
          * vap then disable color detection
          * for this vap
          */
         QDF_TRACE(QDF_MODULE_ID_BSSCOLOR,
                   QDF_TRACE_LEVEL_INFO,
                   "Disabling bsscolor collision detection "
                   "for stopping vap. vdev-id: 0x%x",
                   wlan_vdev_get_id(vap->vdev_obj));
         vap->iv_config_bss_color_offload(vap, true);
         vap->iv_he_bsscolor_detcn_configd_vap = false;

          /* configure collision detection
           * for the next avaailable ap vap
           */
         TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
             if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP)
                     && (wlan_vdev_is_up(tmpvap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                 QDF_TRACE(QDF_MODULE_ID_BSSCOLOR,
                           QDF_TRACE_LEVEL_INFO,
                           "Configuring fw for AP mode bsscolor collision "
                           "detection for the next active vap. vdev-id: 0x%x",
                           wlan_vdev_get_id(tmpvap->vdev_obj));
                 tmpvap->iv_he_bsscolor_detcn_configd_vap = true;
                 tmpvap->iv_config_bss_color_offload(tmpvap, false);
                 break;
             }
         }
     } /* end check if (vap->iv_he_bsscolor_detcn_configd_vap) */
}

QDF_STATUS mlme_vdev_stop_continue_cb(struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct ieee80211vap *vap;
	struct ieee80211com *ic;
	struct wlan_objmgr_vdev *vdev;
	struct wlan_vdev_state_event event;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_lmac_if_rx_ops *rx_ops = NULL;
	struct mbssid_context *mbss_ctx;
	struct mbssid_info *ic_mbss;
#ifdef IPA_OFFLOAD
	enum QDF_OPMODE opmode;
	enum wlan_ipa_wlan_event wlan_event;
#endif
	if (!vdev_mlme) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}
	vdev = vdev_mlme->vdev;
	if (!vdev) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL",
			wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc) {
		mlme_err("PSOC is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	ic = vap->iv_ic;

    mbss_ctx = NULL;
    ic_mbss  = NULL;
    if (ic->is_mbssid_enabled) {
	    if (!ic->ic_mbss_ctx) {
		    mbss_err("mbssid context is null in mbssid mode");
		    return QDF_STATUS_E_FAILURE;
	    }
	    mbss_ctx = ic->ic_mbss_ctx;
	    ic_mbss = &mbss_ctx->ic_mbss[vap->iv_mbss.group_id];
    }

#ifdef IPA_OFFLOAD
	if (ucfg_ipa_is_enabled()) {
		opmode = wlan_vdev_mlme_get_opmode(vdev);
		if (opmode == QDF_SAP_MODE) {
			wlan_event = WLAN_IPA_AP_DISCONNECT;
			ucfg_ipa_event_wq(psoc, vdev->vdev_nif.osdev->wdev->netdev->dev_addr,
					  vdev, wlan_event);
		}
	}
#endif

    if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan) &&
        ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP &&
        ieee80211_is_fils_20tu_allowed_vap(vap)) {
        /* Remove this AP info in rnr_buf in soc and notify lower band AP
         * to advertize updated RNR cache
         */
        wlan_remove_vap_from_6ghz_rnr_cache(vap);
        wlan_tmpl_update_lower_band_vdevs(wlan_pdev_get_psoc(ic->ic_pdev_obj));
        /* wlan_vdev_is_up this check fails to decrement the 6G vap count,
         * cause vap will already be in STOP_STATE when this api is invoked,
         * Replacing wlan_vdev_is_up with iv_is_up flag.
         */
        if (vap->iv_is_up && vap->iv_6g_band_count) {
            wlan_rnr_6ghz_vdev_dec();
            vap->iv_6g_band_count = 0;
        }
    }

    /* If last lower band vdev goes down, notify 6Ghz Vdevs
     * to update template based on 6Ghz only AP case scenario
     */
    if (!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan) &&
        ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP &&
        ieee80211_is_fils_20tu_allowed_vap(vap)) {
        if (vap->iv_in_lower_band_cnt)
            wlan_rnr_lower_band_vdev_dec();
        if (0 == wlan_lower_band_ap_cnt_get()) {
            enum wlan_frm_update_reason update_reason = ENABLE_FILS_UBPRBR_6GHZ;

            /* Do the following for each 6ghz pdev */
            wlan_rnr_6ghz_iterator(wlan_tmpl_update_6ghz_frm, &update_reason, NULL);
        }
    }

    vap->iv_in_lower_band_cnt = 0;
    if (ic->is_mbssid_enabled) {
        if (mbss_ctx && mbss_ctx->ema_ext_enabled &&
            (ic_mbss && ic_mbss->transmit_vap &&
             (ic_mbss->transmit_vap->iv_unit == vap->iv_unit))) {
            vap->iv_mbss.ie_overflow = false;
        }

        /* update the particular node's dtim_count
        * to value 255 to meet the fw requiremnt
        * that a new profile added should have
        * value 255 in the dtim_count field in the
        * template sent from host to fw. If the
        * profile already exists then the corresponding
        * value should be 0
        */
        ieee80211_mbssid_update_mbssie_cache_entry(vap, MBSS_CACHE_ENTRY_IDX);
    }

	if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
		if (ic_mbss && ic_mbss->transmit_vap) {
			/* Delete VAP profile from MBSS IE */
			ieee80211_mbssid_del_profile(vap);
		} else {
			mbss_warn("Transmitting VAP has been deleted!");
		}
	} else {
		mlme_vdev_update_beacon(vdev_mlme, BEACON_FREE,
						event_data_len, event_data);
	}

	event.state = VDEV_STATE_STOPPED;
	if (rx_ops && rx_ops->son_rx_ops.deliver_event)
		rx_ops->son_rx_ops.deliver_event(vdev, NULL, MLME_EVENT_VDEV_STATE, &event);

	mlme_vdev_cleanup(vap);

	/*
	 * In case of AP, peer bss will be reset only at the time of vdev delete,
	 * But in case of STA, BSS peer delete request will be sent to FW for
	 * every instance of vdev up/down.
	 */
	if (vap->iv_opmode == IEEE80211_M_STA)
		wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_DELETE_IND);
	else
		ieee80211_node_reset(vap->iv_bss);


	if (((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
             (wlan_vdev_get_peer_count(vdev) == 1)) ||
	    ((vap->iv_opmode != IEEE80211_M_HOSTAP) &&   /* If !AP and !STA */
	     (vap->iv_opmode != IEEE80211_M_STA))) {
		wlan_vdev_mlme_sm_deliver_evt_sync(vap->vdev_obj,
					WLAN_VDEV_SM_EV_MLME_DOWN_REQ, 0, NULL);
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_notify_down_complete_cb(
				struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_vdev *vdev;
	struct ieee80211vap *vap;
	struct ieee80211com *ic;
	cm_ext_t *cm_ext_handle = NULL;

	vdev = vdev_mlme->vdev;
	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	ic = vap->iv_ic;
	if (!ic) {
		mlme_err("ic is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	if (vap->iv_opmode == IEEE80211_M_STA) {
		cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
		if (!cm_ext_handle)
			return QDF_STATUS_E_FAILURE;

		if (wlan_cm_get_active_req_type(vdev) == CM_CONNECT_ACTIVE)
			wlan_mlme_dispatch_cm_resp(vap, CM_CONNECT_RESP);
		else if (wlan_cm_get_active_req_type(vdev) == CM_ROAM_ACTIVE &&
			 QDF_IS_STATUS_ERROR(cm_ext_handle->cm_conn_rsp.connect_status))
			wlan_mlme_dispatch_cm_resp(vap, CM_CONNECT_RESP);
		else
			wlan_mlme_dispatch_cm_resp(vap, CM_DISCONNECT_RESP);
		return QDF_STATUS_SUCCESS;
	}
	wlan_vdev_mlme_op_flags_clear(vdev_mlme->vdev,
				      WLAN_VDEV_OP_MLME_LEGACY_PEER_DISCON_TRIG);
	wlan_mlme_release_vdev_req(vdev_mlme->vdev,
				   WLAN_SER_CMD_VDEV_STOP_BSS, EOK);

	/* Signal the event when VDEV is brought down completely. */
	qdf_event_set(&ic->wait_event_for_down);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_vdev_notify_down_complete(struct wlan_objmgr_vdev *vdev)
{
	struct vdev_mlme_obj *vdev_mlme;

	vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vdev);
	if (!vdev_mlme)
		return QDF_STATUS_E_FAILURE;

	mlme_vdev_notify_down_complete_cb(vdev_mlme, 0, NULL);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_notify_start_state_exit_cb(struct vdev_mlme_obj *vdev_mlme)
{
	struct wlan_objmgr_pdev *pdev;
	struct ieee80211vap *vap;
	struct ieee80211com *ic;

	vap = wlan_vdev_get_mlme_ext_obj(vdev_mlme->vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL",
			 wlan_vdev_get_id(vdev_mlme->vdev));
		return QDF_STATUS_E_FAILURE;
	}
	ic = vap->iv_ic;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		mlme_err("pdev is null");
		return QDF_STATUS_E_FAILURE;
	}

	if (!wlan_pdev_mlme_op_get(pdev, WLAN_PDEV_OP_RADAR_DETECT_DEFER))
		return QDF_STATUS_SUCCESS;

	/* Invoke RADAR detect defer with last VDEV which moves out of Start
	 * state
	 */
	if (wlan_util_is_pdev_restart_progress(pdev, WLAN_MLME_SB_ID) ==
					QDF_STATUS_SUCCESS)
		return QDF_STATUS_SUCCESS;

	mlme_sm_cmd_schedule_req(pdev, MLME_SM_CMD_PDEV_RADAR_DETECT,
				 ic->ic_radar_defer_chan);

	return QDF_STATUS_SUCCESS;
}


QDF_STATUS mlme_vdev_is_newchan_no_cac_cb(struct vdev_mlme_obj *vdev_mlme)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_channel *des_chan;
	struct ieee80211_ath_channel new_chan_obj;
	struct ieee80211_ath_channel *new_chan = &new_chan_obj;
	struct ieee80211vap *vap;
	struct ieee80211com *ic;
	struct wlan_objmgr_pdev *pdev;
	bool is_cac_continuable;
        struct chan_params chan_p = {0};

	if (!vdev_mlme) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;
	if (!vdev) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	if (!pdev) {
		mlme_err("(vdev-id:%d) PDEV is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic) {
		mlme_err("(vdev-id:%d) ic is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	des_chan = wlan_vdev_mlme_get_des_chan(vdev);

	chan_p.freq = des_chan->ch_freq;
	chan_p.cfreq2 = des_chan->ch_cfreq2;
	set_chan_params_puncture_bitmap(&chan_p, des_chan->puncture_bitmap);

	*new_chan = ieee80211_find_dot11_channel(ic, &chan_p,
			wlan_vdev_get_ieee_phymode(des_chan->ch_phymode));
	if (!IEEE80211_IS_CHAN_VALID(new_chan)) {
		mlme_err("(vdev-id:%d) des chan(%d) is NULL",
			  wlan_vdev_get_id(vdev), des_chan->ch_ieee);
		return QDF_STATUS_E_FAILURE;
	}

	if (mlme_dfs_is_cac_required(pdev,
				     new_chan,
				     ic->ic_curchan,
				     &is_cac_continuable)) {
		mlme_debug("(vdev-id:%d) des chan(%d) needs CAC",
			  wlan_vdev_get_id(vdev), des_chan->ch_ieee);

		vap->force_cleanup_peers = 1;
		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_dfs_cac_wait_notify_cb(struct vdev_mlme_obj *vdev_mlme)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_pdev *pdev;
	struct pdev_mlme_obj *pdev_mlme;

	if (!vdev_mlme) {
		wlan_mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_INVAL;
	}

	vdev = vdev_mlme->vdev;
	if (!vdev) {
		wlan_mlme_err("VDEV is NULL");
		return QDF_STATUS_E_INVAL;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
	if (!pdev_mlme) {
		wlan_mlme_err("Null input");
		return QDF_STATUS_E_INVAL;
	}
	/*
	 * For CSA due to any south bound request such as radar detect,
	 * fallback to multivdev restart as there is no CSA_RESTART
	 * command enqueued at serialization.
	 */
	if (!pdev_mlme->pdev_restart.vdev) {
		wlan_mlme_debug("SB triggered CSA restart");
		return QDF_STATUS_E_NOSUPPORT;
	}

	wlan_mlme_release_vdev_req(vdev,
				   WLAN_SER_CMD_PDEV_CSA_RESTART, EINVAL);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
mlme_vdev_notify_mlo_sync_wait_entry_cb(struct vdev_mlme_obj *vdev_mlme)
{
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_vdev *vdev;
	struct ieee80211com *ic;
	struct ieee80211vap *tmpvap;
	struct wlan_channel *iter_des_chan = NULL;

	if (!vdev_mlme) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_INVAL;
	}

	vdev = vdev_mlme->vdev;
	pdev = wlan_vdev_get_pdev(vdev);
	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic) {
		mlme_err("(vdev-id:%d) ic is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	/* If any VDEV has moved to RESTART_PROGRESS substate, but this VDEV
	 * missed the event, then move to RESTART_PROGRESS sub state
	 */
	if ((wlan_pdev_mlme_op_get(pdev,
				   WLAN_PDEV_OP_RESTART_INPROGRESS)) &&
	    (!wlan_pdev_mlme_op_get(pdev,
				    WLAN_PDEV_OP_RADAR_DETECT_DEFER))) {
		TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
			if (wlan_vdev_is_restart_progress(tmpvap->vdev_obj) ==
						QDF_STATUS_SUCCESS) {
				iter_des_chan = wlan_vdev_mlme_get_des_chan(
							tmpvap->vdev_obj);
				break;
			}
		}
	}

	if (iter_des_chan) {
		mlme_err("psoc:%d vdev:%d Issuing vdev SM_EV_RADAR_DETECTED",
			 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)),
			 wlan_vdev_get_id(vdev));
		wlan_chan_copy(vdev->vdev_mlme.des_chan, iter_des_chan);
		wlan_vdev_mlme_sm_deliver_evt_sync(vdev,
				WLAN_VDEV_SM_EV_RADAR_DETECTED,
				0, NULL);
	}

	return QDF_STATUS_SUCCESS;
}

/* Translation table for conversion from QDF mode to IEEE80211 opmode */
uint8_t qdf_opmode2ieee80211_opmode[QDF_MAX_NO_OF_MODE+1] = {
	IEEE80211_M_STA,         /* QDF_STA_MODE,        */
	IEEE80211_M_HOSTAP,      /* QDF_SAP_MODE,        */
	IEEE80211_M_P2P_CLIENT,  /* QDF_P2P_CLIENT_MODE, */
	IEEE80211_M_P2P_GO,      /* QDF_P2P_GO_MODE,     */
	IEEE80211_M_ANY,         /* QDF_FTM_MODE,        */
	IEEE80211_M_IBSS,        /* QDF_IBSS_MODE,       */
	IEEE80211_M_MONITOR,     /* QDF_MONITOR_MODE,    */
	IEEE80211_M_P2P_DEVICE,  /* QDF_P2P_DEVICE_MODE, */
	IEEE80211_M_ANY,         /* QDF_OCB_MODE,        */
	IEEE80211_M_ANY,         /* QDF_EPPING_MODE,     */
	IEEE80211_M_ANY,         /* QDF_QVIT_MODE,       */
	IEEE80211_M_ANY,         /* QDF_NDI_MODE,        */
	IEEE80211_M_WDS,         /* QDF_WDS_MODE,        */
	IEEE80211_M_BTAMP,       /* QDF_BTAMP_MODE,      */
	IEEE80211_M_AHDEMO,      /* QDF_AHDEMO_MODE      */
	IEEE80211_M_ANY,         /* QDF_TDLS_MODE,       */
	IEEE80211_M_ANY,         /* QDF_NAN_DISC_MODE,   */
	IEEE80211_M_ANY,         /* QDF_MAX_NO_OF_MODE   */
};

struct ieee80211com *wlan_sc_get_ic(struct ol_ath_softc_net80211 *scn)
{
	return &scn->sc_ic;
}

QDF_STATUS mlme_pdev_ext_obj_create(struct pdev_mlme_obj *pdev_mlme)
{
	struct ieee80211com *ic;
	struct pdev_osif_priv *osif_priv;
	struct wlan_objmgr_pdev *pdev;

	if (!pdev_mlme) {
		mlme_err("PDEV MLME is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = pdev_mlme->pdev;
	if (!pdev) {
		mlme_err("PDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	/*
	* From scn, get ic pointer.
	* since ic is static member of scn, so to have minimal changes
	* using ic from scn instead of allocating
	*/
	osif_priv = wlan_pdev_get_ospriv(pdev);

	ic = wlan_sc_get_ic((struct ol_ath_softc_net80211 *)
				osif_priv->legacy_osif_priv);
	/* store back pointer in pdev */
	ic->ic_pdev_obj = pdev;
	pdev_mlme->ext_pdev_ptr = ic;
	pdev_mlme->mlme_register_ops = mlme_register_ops;

	mlme_restart_timer_init(pdev_mlme);


	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_pdev_ext_obj_destroy(struct pdev_mlme_obj *pdev_mlme)
{
	struct ieee80211com *ic;
	struct wlan_objmgr_pdev *pdev;

	if (!pdev_mlme) {
		mlme_err("PDEV MLME is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = pdev_mlme->pdev;
	if (!pdev) {
		mlme_err("PDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if(ic != NULL) {
		mlme_restart_timer_delete(pdev_mlme);
		ic->ic_pdev_obj = NULL;
	}
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_ext_obj_create(struct vdev_mlme_obj *vdev_mlme)
{
	enum ieee80211_opmode opmode;
	uint8_t *bssid;
	uint8_t *mataddr;
	uint32_t flags;
	struct wlan_objmgr_pdev *pdev;
	wlan_dev_t devhandle;
	wlan_if_t vap;
	struct wlan_objmgr_vdev *vdev;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;

	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}
	/* get MAC address from VDEV */
	bssid = wlan_vdev_mlme_get_macaddr(vdev);
	/* get opmode from VDEV */
	opmode = qdf_opmode2ieee80211_opmode[wlan_vdev_mlme_get_opmode(vdev)];
	if (opmode == IEEE80211_M_ANY) {
		mlme_err("qdf opmode %d is not supported",
			 wlan_vdev_mlme_get_opmode(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	mataddr = wlan_vdev_mlme_get_mataddr(vdev);
	flags = vdev->vdev_objmgr.c_flags;
	pdev = wlan_vdev_get_pdev(vdev);
	if(pdev == NULL) {
		mlme_err("PDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	devhandle = (wlan_dev_t )wlan_pdev_get_mlme_ext_obj(pdev);
	if(devhandle == NULL) {
		mlme_err("ic is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vap_create(devhandle, opmode, 0, flags, bssid, mataddr,
			      vdev_mlme);
	if(vap == NULL) {
		mlme_err(" VDEV MLME legacy obj creation failed");
		return QDF_STATUS_E_FAILURE;
	}
	vdev_mlme->ext_vdev_ptr = vap;

	return QDF_STATUS_SUCCESS;
}


QDF_STATUS mlme_vdev_ext_obj_post_create(struct vdev_mlme_obj *vdev_mlme)
{
	wlan_if_t vap;
	enum QDF_OPMODE opmode;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = vdev_mlme->ext_vdev_ptr;
	if(vap == NULL) {
		mlme_err("Legacy VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

#if ATH_PERF_PWR_OFFLOAD
	opmode = wlan_vdev_mlme_get_opmode(vap->vdev_obj);
        /* If lp_iot_mode vap then skip turning on BF */
        if ((opmode == QDF_SAP_MODE) &&
                (vap->iv_ic->ic_implicitbf) &&
                !(vap->iv_create_flags & IEEE80211_LP_IOT_VAP)) {
             wlan_set_param(vap, IEEE80211_SUPPORT_IMPLICITBF, 1);
        }
#endif
	ieee80211_vap_attach(vap);
	qdf_timer_init(NULL, &vap->peer_cleanup_timer,
		       mlme_vdev_force_cleanup_peers, (void *)(vap),
		       QDF_TIMER_TYPE_WAKE_APPS);

	qdf_timer_init(NULL, &vap->peer_del_wait_timer,
		       mlme_vdev_peer_del_timeout_handler, (void *)(vap),
		       QDF_TIMER_TYPE_WAKE_APPS);

	qdf_timer_init(NULL, &vap->peer_del_bss_tid_flush,
		       mlme_vdev_peer_del_bss_tid_flush, (void *)(vap),
		       QDF_TIMER_TYPE_WAKE_APPS);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_ext_obj_destroy(struct vdev_mlme_obj *vdev_mlme)
{
	wlan_if_t vap;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vap = vdev_mlme->ext_vdev_ptr;
	if (vap != NULL) {
		qdf_timer_stop(&vap->peer_cleanup_timer);
		qdf_timer_stop(&vap->peer_del_wait_timer);
		qdf_timer_stop(&vap->peer_del_bss_tid_flush);
		status = mlme_ext_vap_delete(vap);
		if (QDF_IS_STATUS_ERROR(status))
			mlme_err("VAP delete is not Successful!!");
		ieee80211_vap_free(vap);
		vdev_mlme->ext_vdev_ptr = NULL;
	}

	return status;
}

static QDF_STATUS mlme_vdev_up_send_cb(struct vdev_mlme_obj *vdev_mlme,
			uint16_t event_data_len, void *event_data)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_pdev *pdev;
	struct ieee80211com *ic;
	struct ieee80211vap *vap;
	bool restart = false;

	if (vdev_mlme == NULL) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev = vdev_mlme->vdev;

	if (vdev == NULL) {
		mlme_err("VDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	if (pdev == NULL) {
		mlme_err("(vdev-id:%d) PDEV is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic) {
		mlme_err("(vdev-id:%d) ic is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return QDF_STATUS_E_FAILURE;
	}

	if (vap->restart_txn)
		restart = true;

	mlme_ext_vap_up(vap, restart);

        vap->force_cleanup_peers = 0;
	vap->restart_txn = 0;

        /* Send peer authorize command to FW after channel switch announcement */
        if(restart && vap->iv_opmode == IEEE80211_M_STA) {
               if(vap->iv_root_authorize(vap, 1)) {
                      qdf_err("Unable to authorize peer");
               }
               if (wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_START) {
                   IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);
                   wlan_rptr_move_set_state(ic->ic_pdev_obj, WLAN_REPEATER_MOVE_IN_PROGRESS);
               }
        }

	/* Send the CAC Complete Event to the stavap if it is waiting
	* in state REPEATER_CAC
	*/
	{
		struct ieee80211vap *stavap = NULL;
		STA_VAP_DOWNUP_LOCK(ic);
		stavap = ic->ic_sta_vap;
		if(stavap) {
			if (mlme_get_active_req_type(stavap) == MLME_REQ_REPEATER_CAC)
				IEEE80211_DELIVER_EVENT_MLME_REPEATER_CAC_COMPLETE(stavap,0);
		}
		STA_VAP_DOWNUP_UNLOCK(ic);
	}
#if UNIFIED_SMARTANTENNA
	if ((vap->iv_opmode == IEEE80211_M_HOSTAP) && (ieee80211_get_num_active_vaps(ic) == 1)) {
		QDF_STATUS status;
		status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SA_API_ID);
		if (QDF_IS_STATUS_ERROR(status)) {
			mlme_err("unable to get vdev reference (smartantenna");
		} else {
			wlan_sa_api_stop(pdev, vdev, SMART_ANT_RECONFIGURE);
			wlan_sa_api_start(pdev, vdev, SMART_ANT_RECONFIGURE);
			wlan_objmgr_vdev_release_ref(vdev, WLAN_SA_API_ID);
		}
	}
#endif

	/* Set default mcast rate */
	ieee80211_set_mcast_rate(vap);
	return QDF_STATUS_SUCCESS;
}

static void wlan_vdev_set_restart_flag(struct wlan_objmgr_pdev *pdev,
				       void *object,
				       void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	unsigned long *send_array = (unsigned long *)arg;
	struct ieee80211vap *vap;

	if (wlan_util_map_index_is_set(send_array, wlan_vdev_get_id(vdev)) ==
					false)
		return;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return;
	}

	vap->restart_txn = 1;
}

QDF_STATUS mlme_ext_restart_fail_sched_cb(struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *vdev = msg->bodyptr;

	wlan_vdev_mlme_sm_deliver_evt(vdev,
				      WLAN_VDEV_SM_EV_RESTART_REQ_FAIL,
				      0, NULL);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_ext_restart_fail_flush_cb(struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *vdev = msg->bodyptr;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);

	return QDF_STATUS_SUCCESS;
}

static inline
void mlme_vdev_multivdev_restart_fw_send_fail(struct wlan_objmgr_pdev *pdev,
					      uint32_t *vdev_ids,
					      uint32_t num_vdevs)
{
	uint8_t i = 0;
	struct wlan_objmgr_vdev *tmp_vdev = NULL;
	struct scheduler_msg msg = {0};
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	for (i = 0; i < num_vdevs; i++) {
		tmp_vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev,
				vdev_ids[i], WLAN_SCHEDULER_ID);
		if (tmp_vdev == NULL)
			continue;
		mlme_info("Multivdev restart failure vdev:%u",
			  wlan_vdev_get_id(tmp_vdev));
		msg.bodyptr = tmp_vdev;
		msg.callback = mlme_ext_restart_fail_sched_cb;
		msg.flush_callback = mlme_ext_restart_fail_flush_cb;
		ret = scheduler_post_message(QDF_MODULE_ID_MLME,
				QDF_MODULE_ID_MLME,
				QDF_MODULE_ID_MLME, &msg);
		if (QDF_IS_STATUS_ERROR(ret)) {
			mlme_err("Failed to post scheduler msg");
			wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_SCHEDULER_ID);
		}
	}

}

QDF_STATUS mlme_vdev_multivdev_restart_fw_send_cb(struct wlan_objmgr_pdev *pdev)
{
	struct ieee80211com *ic;
	struct wlan_channel *des_chan;
	struct ieee80211_ath_channel curchan_obj;
	struct ieee80211_ath_channel *curchan = &curchan_obj;
	struct wlan_objmgr_vdev *vdev, *tmp_vdev;
	uint32_t *vdev_ids;
	struct vdev_mlme_mvr_param *mvr_param;
	uint32_t num_vdevs = 0, i;
	struct pdev_mlme_obj *pdev_mlme;
	struct vdev_mlme_obj *vdev_mlme;
	uint32_t max_vdevs = 0;
	QDF_STATUS status;
	struct chan_params chan_p = {0};
	uint32_t mvr_param_arr_sz;
	uint8_t mvr_bmap_enabled;
	enum wlan_phymode ch_phymode = 0;
	struct ieee80211vap *vap;
	bool is_target_scan_radio = false;

	if (pdev == NULL) {
		mlme_err("PDEV is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
	if (!pdev_mlme) {
		mlme_err(" PDEV MLME is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic) {
		mlme_err("ic is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	is_target_scan_radio = wlan_pdev_nif_feat_ext_cap_get(pdev,
					WLAN_PDEV_FEXT_SCAN_RADIO);

	max_vdevs = wlan_psoc_get_max_vdev_count(wlan_pdev_get_psoc(pdev));
	/* MVR bitmap considers same phymode/tx-rx-streams
	 * for all vdevs in a pdev;
	 * So if mvr_bmap is used we allocate 1 mvr_param
	 */
	mvr_bmap_enabled = wlan_pdev_nif_feat_cap_get(pdev,
			WLAN_PDEV_F_MULTIVDEV_RESTART_BMAP);
	mvr_param_arr_sz = mvr_bmap_enabled ? 1 : max_vdevs;

	vdev_ids = qdf_mem_malloc(sizeof(*vdev_ids) * max_vdevs);
	if (!vdev_ids) {
		mlme_err("vdev_ids alloc failed");
		return QDF_STATUS_E_FAILURE;
	}
	mvr_param = qdf_mem_malloc(sizeof(*mvr_param) * mvr_param_arr_sz);
	if (!mvr_param) {
		mlme_err("mvr_param alloc failed");
		qdf_mem_free(vdev_ids);
		return QDF_STATUS_E_FAILURE;
	}

	for (i = 0; i < max_vdevs; i++) {
		if (wlan_util_map_index_is_set(
			pdev_mlme->restart_send_vdev_bmap, i)
					== false)
			continue;

		tmp_vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, i,
							WLAN_MLME_NB_ID);
		if (!tmp_vdev) {
			mlme_err("objmgr vdev not found for id %d", i);
			status = QDF_STATUS_E_FAILURE;
			goto end;
		}
		vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(tmp_vdev);
		if (!vdev_mlme) {
			wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
			continue;
		}
		vdev_ids[num_vdevs] = i;

		vap = wlan_vdev_get_mlme_ext_obj(tmp_vdev);
		if (!vap) {
			mlme_err("vap is null");
			wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
			continue;
		}

		des_chan = wlan_vdev_mlme_get_des_chan(tmp_vdev);
		if (ch_phymode < des_chan->ch_phymode) {
			ch_phymode = des_chan->ch_phymode;
		}

		chan_p.freq = des_chan->ch_freq;
		chan_p.cfreq2 = des_chan->ch_cfreq2;
		set_chan_params_puncture_bitmap(&chan_p, des_chan->puncture_bitmap);

		*curchan =  ieee80211_find_dot11_channel(ic, &chan_p,
				wlan_vdev_get_ieee_phymode(des_chan->ch_phymode));
		if (!IEEE80211_IS_CHAN_VALID(curchan) ||
		    (vap->iv_get_phymode(tmp_vdev, curchan) != QDF_STATUS_SUCCESS)) {
			mlme_err("(vdev-id:%d) des chan(%d) is NULL",
				  wlan_vdev_get_id(tmp_vdev), des_chan->ch_ieee);
			wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
			continue;
		}

		if (!mvr_bmap_enabled)
			mvr_param[num_vdevs].phymode = vdev_mlme->mgmt.generic.phy_mode;

		num_vdevs++;
		wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
	}

	if (num_vdevs == 0) {
		status = QDF_STATUS_E_FAILURE;
		goto end;
	}

	vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, vdev_ids[0],
						    WLAN_MLME_NB_ID);
	if (!vdev) {
		status = QDF_STATUS_E_FAILURE;
		goto end;
	}

	des_chan = wlan_vdev_mlme_get_des_chan(vdev);

	chan_p.freq = des_chan->ch_freq;
	chan_p.cfreq2 = des_chan->ch_cfreq2;
	set_chan_params_puncture_bitmap(&chan_p, des_chan->puncture_bitmap);

	*curchan =  ieee80211_find_dot11_channel(ic, &chan_p,
			wlan_vdev_get_ieee_phymode(ch_phymode));

	if (is_target_scan_radio) {
		mlme_debug("vdev:%d pdev:%d psoc:%d des_chan:%d",
			   wlan_vdev_get_id(vdev), wlan_objmgr_pdev_get_pdev_id(pdev),
			   wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)), des_chan->ch_ieee);
	} else {
		mlme_err("vdev:%d pdev:%d psoc:%d des_chan:%d",
			 wlan_vdev_get_id(vdev), wlan_objmgr_pdev_get_pdev_id(pdev),
			 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)), des_chan->ch_ieee);
	}

	if (!IEEE80211_IS_CHAN_VALID(curchan)) {
		mlme_err("(vdev-id:%d) des chan(%d) is NULL",
			  wlan_vdev_get_id(vdev), des_chan->ch_ieee);
		mlme_vdev_multivdev_restart_fw_send_fail(pdev, vdev_ids, num_vdevs);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);
		status = QDF_STATUS_E_FAILURE;
		goto end;
	}

	*ic->ic_prevchan = *ic->ic_curchan;
	*ic->ic_curchan = *curchan;
	wlan_objmgr_pdev_iterate_obj_list(pdev,
			WLAN_VDEV_OP,
			wlan_vdev_set_restart_flag,
			pdev_mlme->restart_send_vdev_bmap, 0,
			WLAN_MLME_NB_ID);

#ifdef WLAN_SUPPORT_GREEN_AP
	/* Stop GreenAP state machine, if GreenAP feature is enabled */
	if (ic->ic_ant_ps_on)
		ucfg_green_ap_config(pdev,0);
#endif

	status = mlme_ext_multi_vdev_restart(ic, vdev_ids, num_vdevs, mvr_param);
	if (QDF_IS_STATUS_ERROR(status))
		mlme_vdev_multivdev_restart_fw_send_fail(pdev, vdev_ids, num_vdevs);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);
	ieee80211_send_chanswitch_complete_event(ic);

	/* Restoring DCS value after channel change */
	if ((!IEEE80211_ARE_CHANS_EQUAL(ic->ic_prevchan, ic->ic_curchan)) && ic->ic_dcs_restore) {
		ic->ic_dcs_restore(ic);
	}

	status = QDF_STATUS_SUCCESS;

end:
	qdf_mem_free(mvr_param);
	qdf_mem_free(vdev_ids);

	return status;
}

static QDF_STATUS mlme_vdev_peer_del_all_response_cb(
				     struct vdev_mlme_obj *vdev_mlme,
                                     struct peer_delete_all_response *rsp)
{
   return mlme_ext_peer_delete_all_response_event_handler(vdev_mlme, rsp);
}

/**
 * mlme_log_stop_vdev_resp_event_timestamp(): Log vap channel switch stats
 *                                            for vdev stop response received.
 *
 * @vdev: VDEV Object pointer
 *
 * Return: None
 */
static void
mlme_log_stop_vdev_resp_event_timestamp(struct wlan_objmgr_vdev *vdev)
{
	struct ieee80211vap *vap;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("vap is null");
		return;
	}

	vap->iv_channel_switch_stats.stop_response_rx = qdf_get_log_timestamp();
}

static QDF_STATUS mlme_vdev_ext_stop_response_cb(
				     struct vdev_mlme_obj *vdev_mlme,
                                     struct vdev_stop_response *rsp)
{
   struct ieee80211vap *vap = vdev_mlme->ext_vdev_ptr;

   mlme_log_stop_vdev_resp_event_timestamp(vdev_mlme->vdev);
   wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj, WLAN_VDEV_SM_EV_STOP_RESP,
		   		 0, NULL);

   return QDF_STATUS_SUCCESS;
}

/**
 * mlme_log_start_vdev_resp_event_timestamp(): Log vap channel switch stats
 *                                             for vdev start response received.
 *
 * @vdev: VDEV Object pointer
 *
 * Return: None
 */
static void
mlme_log_start_vdev_resp_event_timestamp(struct wlan_objmgr_vdev *vdev)
{
	struct ieee80211vap *vap;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("vap is null");
		return;
	}

	vap->iv_channel_switch_stats.start_response_rx =
			qdf_get_log_timestamp();
}

static QDF_STATUS mlme_vdev_ext_start_response_cb(
				     struct vdev_mlme_obj *vdev_mlme,
                                     struct vdev_start_response *rsp)
{
   mlme_log_start_vdev_resp_event_timestamp(vdev_mlme->vdev);
   return mlme_ext_vap_start_response_event_handler(rsp, vdev_mlme);
}

static void
mlme_log_muli_vdev_resp_event_timestamp(struct wlan_objmgr_vdev *vdev,
					uint64_t timestamp)
{
	struct ieee80211vap *vap;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("vap is null");
		return;
	}

	vap->iv_channel_switch_stats.mvr_response_rx = timestamp;
}

static QDF_STATUS mlme_multi_vdev_restart_resp_cb(
		struct wlan_objmgr_psoc *psoc,
		struct multi_vdev_restart_resp *resp)
{
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_vdev *vdev;
	struct vdev_mlme_obj *vdev_mlme;
	struct vdev_start_response vdev_rsp;
	uint8_t max_vdevs, vdev_idx;

	pdev = wlan_objmgr_get_pdev_by_id(psoc, resp->pdev_id,
					  WLAN_MLME_SB_ID);
	if (!pdev) {
		mlme_err("PSOC_%d PDEV_%d is NULL", wlan_psoc_get_id(psoc),
			 resp->pdev_id);
		return QDF_STATUS_E_FAILURE;
	}

	max_vdevs = wlan_psoc_get_max_vdev_count(psoc);
	for (vdev_idx = 0; vdev_idx < max_vdevs; vdev_idx++) {
		if (!qdf_test_bit(vdev_idx, resp->vdev_id_bmap))
			continue;

		mlme_debug("PSOC_%d VDEV_%d Received restart resp",
			   wlan_psoc_get_id(psoc), vdev_idx);

		vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, vdev_idx,
							    WLAN_MLME_SB_ID);
		if (vdev == NULL) {
			mlme_err("PSOC_%d VDEV_%d VDEV is NULL",
				 wlan_psoc_get_id(psoc), vdev_idx);
			continue;
		}

		vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vdev);
		if (!vdev_mlme) {
			mlme_err("PSOC_%d VDEV_%d VDEV_MLME is NULL",
				 wlan_psoc_get_id(psoc), vdev_idx);
			wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
			continue;
		}

		vdev_rsp.vdev_id = vdev_idx;
		vdev_rsp.status = resp->status;
		vdev_rsp.resp_type = WLAN_MLME_HOST_VDEV_RESTART_RESP_EVENT;
		mlme_log_muli_vdev_resp_event_timestamp(vdev, resp->timestamp);
		mlme_ext_vap_start_response_event_handler(&vdev_rsp, vdev_mlme);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
	}

	wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
	return QDF_STATUS_SUCCESS;
}

void mlme_vdev_reconfig_timer_complete_cb(struct vdev_mlme_obj *vdev_mlme)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_peer *peer;
	struct ieee80211_node *ni;
#ifdef WLAN_FEATURE_11BE_MLO
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_vdev *co_mld_vdev = NULL;
	uint16_t vdev_count = 0;
	struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
	uint8_t idx = 0;
	struct wlan_mlo_peer_context *ml_peer;
	struct wlan_mlo_link_peer_entry *peer_entry;
	struct wlan_objmgr_peer *link_peer;
	struct wlan_objmgr_vdev *temp_vdev;
	struct ieee80211vap *vap;
	int i;
#endif

	if (!vdev_mlme) {
		mlme_err("VDEV MLME obj is NULL");
		return;
	}

	if (!vdev_mlme->ml_reconfig_started) {
		mlme_debug("Reconfig timer already stopped, return");
		/* Vdev already disconnected or reconfig timer canceller */
		return;
	}

	vdev = vdev_mlme->vdev;
	if (!vdev) {
		mlme_err("VDEV is NULL");
		return;
	}

	if (!mlo_is_mld_sta(vdev))
		return;

	if (!wlan_cm_is_vdev_connected(vdev)) {
		mlme_err("Vdev is not yet connected, disconnect MLD");
		/* Link is not connected, hence bring down full MLD*/
		wlan_mlme_cm_stop(vdev, CM_SB_DISCONNECT,
				  REASON_DEAUTH_NETWORK_LEAVING, false);
		return;
	}

	peer = wlan_vdev_get_bsspeer(vdev);
	if (!peer) {
		mlme_err("bss peer is null");
		return;
	}

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni) {
		mlme_err("ni is null");
		return;
	}

	if (!ieee80211_node_is_authorized(ni)) {
		/* Link is not authorized, hence bring down full MLD*/
		mlme_err("Link is not authorized, disconnect MLD");
		wlan_mlme_cm_stop(vdev, CM_SB_DISCONNECT,
				  REASON_DEAUTH_NETWORK_LEAVING, false);
		return;
	}

#ifdef WLAN_FEATURE_11BE_MLO
	ml_peer = peer->mlo_peer_ctx;

	if (wlan_mlo_peer_get_primary_peer_link_id(peer) ==
	    wlan_vdev_get_link_id(vdev) &&
	    !ml_peer->primary_umac_migration_in_progress)
	{
		/**
		 * Removal of primary link.
		 * Disconnect full MLD if there is no ongoing PTQM migration.
		 */
		wlan_mlme_cm_stop(vdev, CM_SB_DISCONNECT,
				  REASON_DEAUTH_NETWORK_LEAVING, false);
		return;
	}

	mlo_get_ml_vdev_list(vdev, &vdev_count, wlan_vdev_list);
	if (vdev_count > 1) {
		for (idx = 0; idx < vdev_count; idx++) {
			co_mld_vdev = wlan_vdev_list[idx];
			if (!co_mld_vdev) {
				mlme_debug("VDEV in MLD VDEV list is NULL");
				goto err_release_refs;
			}
			if (co_mld_vdev == vdev)
				continue;
			wlan_mlme_update_assoc_sm_scan_entry(co_mld_vdev);
		}
	}

	/*
	 * If vdev count is 2 and one is going down, we will send notification
	 * to enable AMSDU for handling MLO address translation issues in Be
	 * First where 2 vdevs and no bridge vap is present and one vdev is going down
	 * second when 3 links including a bridge vap is present and since one vdev
	 * is going down, so we will be left with single link MLO
	 */
	if (((vdev_count == 2) && !mlo_sta_bridge_exists(vdev)) ||
	    ((vdev_count == 3) && mlo_sta_bridge_exists(vdev))) {
		for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
			peer_entry = &ml_peer->peer_list[i];
			link_peer = peer_entry->link_peer;

			if (!link_peer)
				continue;

			if ((link_peer != peer)) {
				temp_vdev = wlan_peer_get_vdev(link_peer);
				vap = wlan_vdev_get_mlme_ext_obj(temp_vdev);
				psoc = wlan_vdev_get_psoc(temp_vdev);
				if ((!cfg_get(psoc, CFG_OL_ADV_RAW_MODE_WAR_ENABLE) ||
				    (qdf_mem_cmp(link_peer->macaddr, link_peer->mldaddr,
				    QDF_MAC_ADDR_SIZE) == 0)) && (vap &&
				    vap->iv_tx_encap_type == htt_cmn_pkt_type_raw))
					/*
					 * For cases when ini enable_advance_rawmode_war is true, then send enable
					 * event only when peer mld mac is same as peer link mac
					 */
					wlan_cfg80211_hw_status_notification(vap, link_peer, true);
			}
		}
	}
err_release_refs:
	for (idx = 0; idx < vdev_count; idx++)
		mlo_release_vdev_ref(wlan_vdev_list[idx]);

#endif
	wlan_cm_disconnect(vdev, CM_SB_DISCONNECT,
			   REASON_HOST_TRIGGERED_LINK_DELETE, NULL);
}

static struct vdev_mlme_ops mlme_ops = {
	.mlme_vdev_reset_proto_params = mlme_vdev_reset_proto_params_cb,
	.mlme_vdev_start_continue = mlme_vdev_start_continue_cb,
	.mlme_vdev_sta_conn_start = mlme_vdev_sta_conn_start_cb,
	.mlme_vdev_start_req_failed = mlme_vdev_start_req_failed_cb,
	.mlme_vdev_up_send = mlme_vdev_up_send_cb,
	.mlme_vdev_notify_up_complete = mlme_vdev_notify_up_complete_cb,
	.mlme_vdev_update_beacon = mlme_vdev_update_beacon_cb,
	.mlme_vdev_disconnect_peers = mlme_vdev_disconnect_peers_cb,
	.mlme_vdev_dfs_cac_timer_stop = mlme_vdev_dfs_cac_timer_stop_cb,
	.mlme_vdev_stop_continue = mlme_vdev_stop_continue_cb,
	.mlme_vdev_notify_down_complete = mlme_vdev_notify_down_complete_cb,
	.mlme_vdev_ext_stop_rsp = mlme_vdev_ext_stop_response_cb,
	.mlme_vdev_ext_start_rsp = mlme_vdev_ext_start_response_cb,
	.mlme_vdev_notify_start_state_exit =
					mlme_vdev_notify_start_state_exit_cb,
	.mlme_vdev_is_newchan_no_cac = mlme_vdev_is_newchan_no_cac_cb,
	.mlme_vdev_ext_peer_delete_all_rsp = mlme_vdev_peer_del_all_response_cb,
	.mlme_vdev_dfs_cac_wait_notify = mlme_vdev_dfs_cac_wait_notify_cb,
	.mlme_vdev_sta_disconn_start = mlme_vdev_sta_disconn_start_cb,
	.mlme_vdev_reconfig_timer_complete = mlme_vdev_reconfig_timer_complete_cb,
	.mlme_vdev_notify_mlo_sync_wait_entry =
				mlme_vdev_notify_mlo_sync_wait_entry_cb,
	.mlme_vdev_notify_link_update_event =
				mlme_vdev_notify_link_update_event_cb,
};

struct mlme_ext_ops glbl_ops_ext = {
	.mlme_pdev_ext_hdl_create = mlme_pdev_ext_obj_create,
	.mlme_pdev_ext_hdl_destroy = mlme_pdev_ext_obj_destroy,
	.mlme_vdev_ext_hdl_create = mlme_vdev_ext_obj_create,
	.mlme_vdev_ext_hdl_post_create = mlme_vdev_ext_obj_post_create,
	.mlme_vdev_ext_hdl_destroy = mlme_vdev_ext_obj_destroy,
	.mlme_vdev_start_fw_send = mlme_ext_vap_start,
	.mlme_vdev_stop_fw_send = mlme_ext_vap_stop,
	.mlme_vdev_down_fw_send = mlme_ext_vap_down,
	.mlme_multivdev_restart_fw_send =
				mlme_vdev_multivdev_restart_fw_send_cb,
	.mlme_multi_vdev_restart_resp =
				mlme_multi_vdev_restart_resp_cb,
	.mlme_vdev_enqueue_exp_cmd = NULL,
	.mlme_cm_ext_connect_start_ind_cb = wlan_mlme_cm_connect_start,
	.mlme_cm_ext_bss_select_ind_cb = wlan_mlme_cm_connect_active,
	.mlme_cm_ext_bss_peer_create_req_cb = wlan_mlme_cm_ext_bss_peer_create_req,
	.mlme_cm_ext_connect_req_cb = wlan_mlme_cm_join_start,
	.mlme_cm_ext_connect_complete_ind_cb = wlan_mlme_cm_connect_complete,
	.mlme_cm_ext_disconnect_start_ind_cb = wlan_mlme_cm_disconnect_start,
	.mlme_cm_ext_disconnect_req_cb = wlan_mlme_cm_disconnect_active,
	.mlme_cm_ext_bss_peer_delete_req_cb = wlan_mlme_cm_bss_peer_delete_req,
	.mlme_cm_ext_disconnect_complete_ind_cb = wlan_mlme_cm_disconnect_complete,
	.mlme_cm_ext_vdev_down_req_cb = wlan_mlme_cm_vdev_down,
	.mlme_cm_ext_roam_start_ind_cb = NULL,
	.mlme_cm_ext_reassoc_req_cb = wlan_mlme_cm_reassoc_join_start,
	.mlme_cm_ext_hdl_create_cb = wlan_mlme_cm_ext_hdl_create,
	.mlme_cm_ext_hdl_destroy_cb = wlan_mlme_cm_ext_hdl_destroy,
	.mlme_ext_get_acs_inprogress = wlan_get_acs_inprogress,
};

QDF_STATUS mlme_register_ops(struct vdev_mlme_obj *vdev_mlme)
{
	if (!vdev_mlme) {
		mlme_err("VDEV MLME obj is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	vdev_mlme->ops = &mlme_ops;

	mlme_register_cmn_ops(vdev_mlme);

	return QDF_STATUS_SUCCESS;
}

struct mlme_ext_ops *mlme_get_global_ops(void)
{
	return &glbl_ops_ext;
}

QDF_STATUS mlme_vdev_sm_peers_discon_sched_cb(struct scheduler_msg *msg)
{
	struct wlan_objmgr_vdev *vdev = msg->bodyptr;
	struct ieee80211vap *vap;

	if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
		vap = wlan_vdev_get_mlme_ext_obj(vdev);
		if (!vap) {
			wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
			return QDF_STATUS_E_FAILURE;
		}

		if ((wlan_cm_get_active_req_type(vdev) == CM_DISCONNECT_ACTIVE) &&
		    (wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_INIT)) {
			wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_DELETE_SUCCESS_RESP);
		} else {
			wlan_vdev_mlme_sm_deliver_evt(vdev,
					WLAN_VDEV_SM_EV_MLME_DOWN_REQ, 0, NULL);
		}

	} else {
		if (wlan_mlo_peer_delete_is_not_allowed(vdev)) {
			if (wlan_vdev_get_legacy_peer_count(vdev) == 1) {
				wlan_vdev_mlme_sm_deliver_evt(
					vdev,
					WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE,
					0, NULL);
				wlan_vdev_mlme_op_flags_clear(
					vdev,
					WLAN_VDEV_OP_MLME_LEGACY_PEER_DISCON_TRIG);
			}
		} else {
			if (wlan_vdev_get_peer_count(vdev) == 1) {
				wlan_vdev_mlme_sm_deliver_evt(
					vdev,
					WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE,
					0, NULL);
			}
		}
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);

	return QDF_STATUS_SUCCESS;
}

void mlme_vdev_sm_peers_discon_post_sched_msg(struct wlan_objmgr_vdev *vdev)
{
	struct scheduler_msg msg = {0};
	QDF_STATUS ret = QDF_STATUS_SUCCESS;

	/* Here both scheduler callback and msg flush callback are the same,
	 * since the event has to be dispatched to VDEV SM even if scheduler
	 * message is flushed. Else it would lead to VDEV SM stuck in same
	 * state.
	 */
	msg.bodyptr = vdev;
	msg.callback = mlme_vdev_sm_peers_discon_sched_cb;
	msg.flush_callback = mlme_vdev_sm_peers_discon_sched_cb;

	ret = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SCHEDULER_ID);
	if (QDF_IS_STATUS_ERROR(ret)) {
		wlan_mlme_err("Try get ref failed for vdev:%d",
			      wlan_vdev_get_id(vdev));
		QDF_BUG(0);
	}

	ret = scheduler_post_message(QDF_MODULE_ID_MLME,
			QDF_MODULE_ID_MLME,
			QDF_MODULE_ID_MLME, &msg);

	if (QDF_IS_STATUS_ERROR(ret)) {
		wlan_mlme_err("Failed to post scheduler msg");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
		QDF_BUG(0);
	}
}

void mlme_vdev_sm_notify_peers_disconnected(struct ieee80211vap *vap)
{
	struct wlan_objmgr_vdev *vdev = vap->vdev_obj;

	if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
		mlme_vdev_sm_peers_discon_post_sched_msg(vdev);
	} else {
		if (wlan_mlo_peer_delete_is_not_allowed(vdev)) {
			if (wlan_vdev_get_legacy_peer_count(vdev) == 1) {
				qdf_timer_stop(&vap->peer_cleanup_timer);
				qdf_timer_stop(&vap->peer_del_wait_timer);
				qdf_timer_stop(&vap->peer_del_bss_tid_flush);
				mlme_vdev_sm_peers_discon_post_sched_msg(vdev);
			}
		} else {
			if (wlan_vdev_get_peer_count(vdev) == 1) {
				qdf_timer_stop(&vap->peer_cleanup_timer);
				qdf_timer_stop(&vap->peer_del_wait_timer);
				qdf_timer_stop(&vap->peer_del_bss_tid_flush);
				mlme_vdev_sm_peers_discon_post_sched_msg(vdev);
			}
		}
	}
}

void mlme_vdev_sm_notify_conn_sm_init_state(struct ieee80211vap *vap)
{
	struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
	enum wlan_vdev_state state;
	enum wlan_vdev_state substate;
	enum wlan_vdev_sm_evt event;

	if (vap->iv_opmode != IEEE80211_M_STA)
		return;

	state = wlan_vdev_mlme_get_state(vdev);
	substate = wlan_vdev_mlme_get_substate(vdev);

	if (state == WLAN_VDEV_S_INIT) {
		event = WLAN_VDEV_SM_EV_DOWN;
	}
        else {
		if (state == WLAN_VDEV_S_START) {
		    if (substate == WLAN_VDEV_SS_START_DISCONN_PROGRESS)
			event = WLAN_VDEV_SM_EV_CONNECTION_FAIL;
		    else if (substate == WLAN_VDEV_SS_START_RESTART_PROGRESS ||
		             substate == WLAN_VDEV_SS_START_START_PROGRESS)
		        event = WLAN_VDEV_SM_EV_DOWN;
                    else
			event = WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE;
                }
		else {
			event = WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE;
                }
	}

	wlan_vdev_mlme_sm_deliver_evt(vdev, event, 0, NULL);
}

static void mlme_vdev_sm_radar_notify(struct wlan_objmgr_pdev *pdev,
				      void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	struct ieee80211_ath_channel *new_chan = arg;
	struct ieee80211vap *vap = wlan_vdev_get_mlme_ext_obj(vdev);
        struct ieee80211com *ic =  wlan_pdev_get_mlme_ext_obj(pdev);
        struct ieee80211vap *tmp_vap = NULL;

	if (!vap) {
		mlme_err("(vdev-id:%d) vap is NULL", wlan_vdev_get_id(vdev));
		return;
	}

	if (wlan_vdev_mlme_get_opmode(vdev) != QDF_STA_MODE) {
		struct ieee80211_ath_channel new_vap_chan_obj;
		struct ieee80211_ath_channel *new_vap_chan = &new_vap_chan_obj;
#ifdef WLAN_FEATURE_11BE
		struct chan_params chan_p = {0};
#endif

		if (vdev->vdev_mlme.des_chan) {
			*new_vap_chan = *new_chan;
#ifdef WLAN_FEATURE_11BE
			if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode) && ieee80211_is_mode_lower(vap, vap->iv_des_mode)) {
				chan_p.freq = ieee80211_chan2freq(vap->iv_ic, new_chan);
				chan_p.cfreq2 = new_chan->ic_vhtop_freq_seg2;
				set_chan_params_puncture_bitmap(&chan_p, new_chan->op_puncture_bitmap);

				*new_vap_chan = ieee80211_find_dot11_channel(vap->iv_ic, &chan_p,
									     ieee80211_get_derived_mode(vap, ieee80211_chan2mode(new_chan), ieee80211_mixed_modes_config(vap, vap->iv_des_mode), &chan_p));
			}
#endif
			ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan,
						   new_vap_chan);
		}

		/* For a non-beaconing ap vap, deliver FW VDEV Restart event,
		 * if there are no other beaconing vaps in the pdev.
		 */
		if ((IEEE80211_IS_MLO_BRIDGE_VAP(vap) && wlan_vdev_is_dfs_cac_wait(vdev) != QDF_STATUS_SUCCESS) ||
			(wlan_is_non_beaconing_ap_vap(vap) && !ieee80211_num_beaconing_ap_vaps_up(ic)))
			wlan_vdev_mlme_sm_deliver_evt(vdev,
						      WLAN_VDEV_SM_EV_FW_VDEV_RESTART,
						      0, NULL);
		else
			wlan_vdev_mlme_sm_deliver_evt(vdev,
						      WLAN_VDEV_SM_EV_RADAR_DETECTED,
						      0, NULL);
	} else {
		/*
		 * For STA VDEV, dispatch beacon miss event.
		 * So that the assoc_sm is brought down and same is notified to
		 * conn_sm and vdev_sm.
		 */
		mlme_err("Bring down STA vdev id:%d", wlan_vdev_get_id(vdev));
		IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);

		/* If RADAR is found on operating channel, SM triggers MVR.
		 * During MVR, SM waits till all VAP's are in down state.
		 * If this wait time expires, host assert gets triggered.
		 * with rpt_max_phy feature, it is possible that rpt ap vap's
		 * BW is more than that rpt STA. If RADAR is found on rpt ap
		 * vap's BW, SM triggers MVR but since rpt STA did not found
		 * radar in its BW, it will never be brought down which causes
		 * host assert during MVR.
		 * Bring down rpt STA here to address above scenario.
		 */
		if (ic && ieee80211_ic_rpt_max_phy_is_set(ic)) {
			TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
				if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
				    (wlan_vdev_is_up(tmp_vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
					ieee80211_bringdown_sta_vap(ic);
					break;
				}
			}
		}
	}
}

void
wlan_pdev_mlme_vdev_sm_notify_radar_ind(struct wlan_objmgr_pdev *pdev,
					struct ieee80211_ath_channel *new_chan)
{
	struct ieee80211com *ic;
	if (!pdev)
		return;
	ic = wlan_pdev_get_mlme_ext_obj(pdev);

	if (!ic) {
	    mlme_err("ic is NULL");
	    return;
	}

	if (wlan_util_is_pdev_restart_progress(pdev, WLAN_MLME_SB_ID) ==
					QDF_STATUS_SUCCESS) {
		if (wlan_pdev_mlme_op_get(pdev,
					  WLAN_PDEV_OP_RESTART_INPROGRESS)) {
			mlme_info("Channel change is in progress, ignore radar detect event");
			return;
		}
		*ic->ic_radar_defer_chan = *new_chan;

		/* Defer RADAR detection as RESTART is in progress */
		wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_RADAR_DETECT_DEFER);
	} else {
		mlme_err("Starting MVR for Pdev %d Psoc %d",
			 wlan_objmgr_pdev_get_pdev_id(pdev),
			 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
		wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_MBSSID_RESTART);

		wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
					  mlme_vdev_sm_radar_notify,
					  new_chan, 0, WLAN_MLME_SB_ID);
		/* clear defer RADAR detection as RESTART is triggered */
		wlan_pdev_mlme_op_clear(pdev, WLAN_PDEV_OP_RADAR_DETECT_DEFER);
		*ic->ic_radar_defer_chan = empty_channel;
	}
}


static void mlme_vdev_sm_nol_chan_change(struct wlan_objmgr_pdev *pdev,
				      void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	struct ieee80211_ath_channel *new_chan = arg;
	struct ieee80211vap *vap;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return;
	}

	vap->vap_start_failure_action_taken = true;
	if (vdev->vdev_mlme.des_chan)
		ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan,
					   new_chan);

	/* If VDEV is in RESTART_PROGRESS substate, do not notify radar */
	if (wlan_vdev_is_restart_progress(vdev) == QDF_STATUS_SUCCESS)
		return;

	/* For STA, it should do beacon miss */
	if ((vap->iv_opmode == IEEE80211_M_STA) &&
	    (wlan_vdev_is_up(vdev) == QDF_STATUS_SUCCESS)) {
		wlan_scan_update_channel_list(vap->iv_ic);
		IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);
	} else {
		if (IEEE80211_IS_MLO_BRIDGE_VAP(vap))
			wlan_vdev_mlme_sm_deliver_evt(vdev,
						      WLAN_VDEV_SM_EV_FW_VDEV_RESTART,
						      0, NULL);
		else
			wlan_vdev_mlme_sm_deliver_evt(vdev,
						      WLAN_VDEV_SM_EV_RADAR_DETECTED,
						      0, NULL);
	}
}

void
wlan_pdev_mlme_vdev_sm_notify_chan_failure(struct wlan_objmgr_pdev *pdev,
					struct ieee80211_ath_channel *new_chan)
{
	if (!pdev)
		return;

	mlme_err("Starting MVR for Pdev %d Psoc %d",
		 wlan_objmgr_pdev_get_pdev_id(pdev),
		 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
	wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_MBSSID_RESTART);
	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
					  mlme_vdev_sm_nol_chan_change,
					  new_chan, 0, WLAN_MLME_SB_ID);
}

static void mlme_vdev_sm_csa_restart_helper(struct ieee80211vap *vap,
					struct ieee80211_ath_channel *new_chan)
{
	struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
#ifdef WLAN_FEATURE_11BE
	struct chan_params chan_p = {0};
#endif
	struct ieee80211_ath_channel new_vap_chan_obj;
	struct ieee80211_ath_channel *new_vap_chan = &new_vap_chan_obj;

	if (!vdev)
		return;

	*new_vap_chan = *new_chan;
#ifdef WLAN_FEATURE_11BE
	if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode) && ieee80211_is_mode_lower(vap, vap->iv_des_mode)) {
		chan_p.freq = ieee80211_chan2freq(vap->iv_ic, new_chan);
		chan_p.cfreq2 = new_chan->ic_vhtop_freq_seg2;
		set_chan_params_puncture_bitmap(&chan_p, new_chan->op_puncture_bitmap);

		*new_vap_chan = ieee80211_find_dot11_channel(vap->iv_ic, &chan_p,
							     ieee80211_get_derived_mode(vap, ieee80211_chan2mode(new_chan), ieee80211_mixed_modes_config(vap, vap->iv_des_mode), &chan_p));
	}
#endif

	ieee80211vap_set_flag(vap, IEEE80211_F_CHANSWITCH);
	vap->iv_chanchange_count = 0;
	if (vdev->vdev_mlme.des_chan)
		ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan,
					   new_vap_chan);
	wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_CSA_RESTART,
				      0, NULL);
}

static void mlme_vdev_sm_csa_restart(struct wlan_objmgr_pdev *pdev,
				      void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	struct ieee80211_ath_channel *new_chan = arg;
	struct ieee80211vap *vap;
	struct ieee80211com *ic;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap is NULL", wlan_vdev_get_id(vdev));
		return;
	}

	ic = vap->iv_ic;
	if (!ic) {
		mlme_err("ic is NULL");
		return;
	}

	if (ic->is_mbssid_enabled &&
        (vap == ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].transmit_vap)) {
		/* In MBSSID case, deliver CSA_RESTART event for tx-vap
		 * only after delivering the ones for non-tx vaps. This
		 * is required to ensure that all the non-tx vaps are in
		 * SUSPEND_CSA_RESTART substate when the only csa_status
		 * _complete_event is recieved for the tx-vap
		 */
		mbss_debug("Skipping csa_restart_helper for (vdev-id:%d)", wlan_vdev_get_id(vdev));
		return;
	}

	if(vap->iv_opmode == IEEE80211_M_HOSTAP ||
	   vap->iv_opmode == IEEE80211_M_MONITOR) {
		mlme_vdev_sm_csa_restart_helper(vap, new_chan);
	}
}

void
wlan_pdev_mlme_vdev_sm_csa_restart(struct wlan_objmgr_pdev *pdev,
				   struct ieee80211_ath_channel *new_chan)
{
	struct ieee80211com *ic;
	int gp_id = 0;

	if (!pdev)
		return;

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic)
		return;

	mlme_err("Starting MVR for Pdev %d Psoc %d",
			wlan_objmgr_pdev_get_pdev_id(pdev),
			wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
	wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_MBSSID_RESTART);
	/*
	 * Do mlme_vdev_sm_csa_restart for each vdev.
	 */
	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
			mlme_vdev_sm_csa_restart,
			new_chan, 0, WLAN_MLME_SB_ID);

	/* In MBSSID case, deliver CSA_RESTART event for all tx-vaps
	 * at the end after all non-tx vaps
	 */

	/* Loop for all Groups and TX vaps */
	if (ic->is_mbssid_enabled) {
		for (gp_id = 0; gp_id < ic->ic_mbss_ctx->max_ngroups; gp_id++) {
			if (ic->ic_mbss_ctx->ic_mbss[gp_id].transmit_vap) {
				mbss_debug("calling csa_restart_helper for TX VAP (vdev-id:%d)",
						ic->ic_mbss_ctx->ic_mbss[gp_id].transmit_vap->iv_unit);
				mlme_vdev_sm_csa_restart_helper(ic->ic_mbss_ctx->ic_mbss[gp_id].transmit_vap,
						new_chan);
			}
		}
	}
}

static void mlme_vdev_sm_chan_change(struct wlan_objmgr_pdev *pdev,
				     void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	struct ieee80211_ath_channel *new_chan = arg;
	struct ieee80211vap *vap;
	enum wlan_vdev_sm_evt event;

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		mlme_err("(vdev-id:%d) vap  is NULL", wlan_vdev_get_id(vdev));
		return;
	}

	if(vap->iv_opmode == IEEE80211_M_HOSTAP ||
	   vap->iv_opmode == IEEE80211_M_MONITOR) {
		if (vdev->vdev_mlme.des_chan)
			ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan,
						   new_chan);

		if (wlan_vdev_is_up(vdev) == QDF_STATUS_SUCCESS) {
			if (IEEE80211_IS_MLO_BRIDGE_VAP(vap))
				event = WLAN_VDEV_SM_EV_FW_VDEV_RESTART;
			else
				event = WLAN_VDEV_SM_EV_SUSPEND_RESTART;
		} else
			event = WLAN_VDEV_SM_EV_RADAR_DETECTED;

		wlan_vdev_mlme_sm_deliver_evt(vdev, event, 0, NULL);
	} else if (vap->iv_opmode == IEEE80211_M_STA) {
		wlan_scan_update_channel_list(vap->iv_ic);
		IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);
	}
}

static void wlan_vdev_sm_chan_config_valid(struct wlan_objmgr_pdev *pdev,
					   void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	bool *restart = (bool *)arg;

	if (wlan_vdev_chan_config_valid(vdev) == QDF_STATUS_SUCCESS)
		*restart = true;
}

void
wlan_pdev_mlme_vdev_sm_chan_change(struct wlan_objmgr_pdev *pdev,
				   struct ieee80211_ath_channel *new_chan)
{
	bool do_restart = false;

	if (!pdev)
		return;

	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
					  wlan_vdev_sm_chan_config_valid,
					  &do_restart, 0, WLAN_MLME_SB_ID);

	if (do_restart) {
		mlme_err("Starting MVR for Pdev %d Psoc %d",
			 wlan_objmgr_pdev_get_pdev_id(pdev),
			 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
		wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_MBSSID_RESTART);
		wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
						  mlme_vdev_sm_chan_change,
						  new_chan, 0, WLAN_MLME_SB_ID);
	}
}

static void mlme_vdev_send_fw_vdev_restart(struct wlan_objmgr_vdev *vdev,
				  struct ieee80211_ath_channel *new_chan)
{
	if (vdev->vdev_mlme.des_chan)
		ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan, new_chan);

	wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_FW_VDEV_RESTART, 0,
				      NULL);
}

void wlan_pdev_mlme_vdev_sm_seamless_chan_change
				(struct wlan_objmgr_pdev *pdev,
				 struct wlan_objmgr_vdev *vdev,
				 struct ieee80211_ath_channel *new_chan)
{
	if (!vdev)
		return;

	if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) ||
	    !(wlan_pdev_mlme_op_get(pdev, WLAN_PDEV_OP_MBSSID_RESTART))) {
		mlme_err("Starting MVR Vdev %d Pdev %d Psoc %d",
			 wlan_vdev_get_id(vdev),
			 wlan_objmgr_pdev_get_pdev_id(pdev),
			 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
		wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_MBSSID_RESTART);
	}

	if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) &&
	    wlan_vdev_is_up(vdev) != QDF_STATUS_SUCCESS) {
		mlme_err("Vdev not up, skip MVR for Vdev %d Psoc %d",
			 wlan_vdev_get_id(vdev),
			 wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
		return;
	}

	mlme_vdev_send_fw_vdev_restart(vdev, new_chan);
}

void mlme_vdev_sm_deliver_csa_complete(struct wlan_objmgr_vdev *vdev)
{
#ifdef WLAN_FEATURE_11BE_MLO
        /* MLO reconfig: While link removal is in progress, CSA complete
         * sequence would lead to sending RESTART REQ and UP command on
         * link vap is in removal progress, to avoid it, sending EV_DOWN to SM
         * would bring down the VAP
         */
	if (wlan_vdev_mlme_op_flags_get(vdev,
					WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
	    wlan_vdev_mlme_op_flags_get(vdev,
					WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE)) {
		wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_DOWN, 0,
					      NULL);
	} else {
		wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_CSA_COMPLETE,
					      0, NULL);
	}
#else
	wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_CSA_COMPLETE, 0,
				      NULL);
#endif
}

qdf_export_symbol(mlme_vdev_sm_deliver_csa_complete);
