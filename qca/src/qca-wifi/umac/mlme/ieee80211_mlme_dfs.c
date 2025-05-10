/*
 * Copyright (c) 2017, 2019-2021, 2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2002-2006, Atheros Communications Inc.
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

/*-
 * Copyright (c) 2007-2008 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <ieee80211_var.h>
#include <ieee80211_sme_api.h>
#include <ieee80211_channel.h>
#include "ieee80211_mlme_dfs_dispatcher.h"
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_lmac_if_api.h>
#include <osif_private.h>
#include <wlan_dfs_ioctl.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_dfs_utils_api.h>
#include <wlan_vdev_mlme_api.h>
#include <ieee80211_regdmn_dispatcher.h>
#include <ieee80211_ucfg.h>
#include "ieee80211_mlme_priv.h"
#include <wlan_utility.h>
#include <wlan_dfs_ioctl.h>
#include "_ieee80211.h"
#include <wlan_reg_services_api.h>
#include <wlan_reg_channel_api.h>
#include <wlan_mlme_if.h>
#include <wlan_mbss.h>
#include <dfs_postnol_ucfg.h>

#if WIFI_MEM_MANAGER_SUPPORT
#include "mem_manager.h"
#endif

/*
 * 60+2 seconds. Extra 2 Second to take care of external
 * test house setup issue.
 */
static int ieee80211_cac_timeout = 62;

/*
 * 600+2 seconds. Extra 2 Second to take care of external
 * test house setup issue.
 */
static int ieee80211_cac_weather_timeout = 602;

#ifdef WLAN_DFS_PRECAC_AUTO_CHAN_SUPPORT
extern int ieee80211_ucfg_set_chanswitch(wlan_if_t vaphandle,
		struct chan_params *chan_p, u_int8_t tbtt, u_int16_t ch_width);
#endif

extern void wlan_send_omn_action(void *arg, wlan_node_t node);

/* Get VAPS in DFS_WAIT state */
int
ieee80211_dfs_vaps_in_dfs_wait(struct ieee80211com *ic,
		struct ieee80211vap *curr_vap)
{
	struct ieee80211vap *vap;
	int dfs_wait_cnt = 0;
	IEEE80211_COMM_LOCK(ic);
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		if(vap == curr_vap) {
			continue;
		}
		if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
			continue;
		}

		if (wlan_vdev_mlme_get_state(vap->vdev_obj) ==
                                         WLAN_VDEV_S_DFS_CAC_WAIT) {
			dfs_wait_cnt++;
		}
	}
	IEEE80211_COMM_UNLOCK(ic);
	return dfs_wait_cnt;
}

static int
ieee80211_dfs_send_dfs_wait(struct ieee80211com *ic, struct ieee80211vap *vap)
{
	if (!vap)
              return 1;

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
	if ((vap->iv_opmode != IEEE80211_M_HOSTAP) &&
	    !((vap->iv_opmode == IEEE80211_M_STA)
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
		&& mlme_is_stacac_needed(vap))) {
#else
		)) {
#endif
		IEEE80211_DPRINTF_IC(ic,
				IEEE80211_VERBOSE_FORCE,
				IEEE80211_MSG_DFS,"%s[%d] NOT A HOSTAP/IBSS VAP , skip DFS (iv_opmode %d, iv_bsschan %d,  ic_freq %d)\n",
				__func__, __LINE__, vap->iv_opmode, vap->iv_bsschan->ic_freq, ic->ic_curchan->ic_freq);
                return 0;
	}
#endif
	if (!IEEE80211_ARE_CHANS_EQUAL(vap->iv_bsschan, ic->ic_curchan)) {
		IEEE80211_DPRINTF_IC(ic,
				IEEE80211_VERBOSE_FORCE,
				IEEE80211_MSG_DFS,"%s[%d] VAP chan mismatch vap %d ic %d\n", __func__, __LINE__,
				vap->iv_bsschan->ic_freq, ic->ic_curchan->ic_freq);
                return 0;
	}
        return 1;

}

/*
 * Initiate the CAC timer.  The driver is responsible
 * for setting up the hardware to scan for radar on the
 * channnel, we just handle timing things out.
 */
int
ieee80211_dfs_cac_start(struct ieee80211vap *vap)
{
	struct ieee80211com *ic = vap->iv_ic;
	bool check_if_vap_is_running = TRUE;
	struct wlan_objmgr_pdev *pdev;
	bool continue_current_cac = false;
	bool is_sta_cac_needed = false;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return -1;
	}

	if (!IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan))
            return 1;

	if (ic->ic_flags_ext2 & IEEE80211_FEXT2_CSA_WAIT) {
		/* CSA from chanswitch ioctl case */
		ic->ic_flags_ext2 &= ~IEEE80211_FEXT2_CSA_WAIT;
		check_if_vap_is_running = FALSE;
	}

	if (!IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(ic->ic_curchan)) {
		/* Consider a case where AP was up in a DFS channel and in CAC
		 * period (DFS-WAIT state) and the workque to initiate CAC is
		 * scheduled. At this time, real radar/spoof radar pulses from
		 * bottom-half/tasklet context are received which is given
		 * higher priority than Workque context. Due to radar, channel
		 * change happened, AP switched to non-DFS channel and curchan
		 * is non-DFS. The scheduled workque gets executed now, starts
		 * a CAC on non-DFS curchan(curchan has changed from DFS to
		 * non-DFS work queue func not aware of this !!). Though the
		 * vap state machine moves the state of the vap from DFS-WAIT
		 * to RUN state as curchan is non-DFS, the cac timer is not
		 * cancelled. CAC timer on non-DFS channel runs and it expires.
		 * So cancelling CAC here if chan is not DFS to avoid this
		 * unexpected run and expiry of CAC.
		 */
		mlme_dfs_cac_stop(ic->ic_pdev_obj);
		ath_vap_iter_cac(NULL, vap);
		return 1;
	}

	/* If any VAP is in active and running (excluding bridge vaps and
	 * smart monitor vaps),
	 * then no need to start cac timer as all the VAPs will be using
	 * same channel and currently running VAP has already done cac and
	 * actively monitoring channel.
	 */
	if(check_if_vap_is_running) {
		if (ieee80211_num_beaconing_ap_vaps_up(ic)) {
			QDF_TRACE(QDF_MODULE_ID_DFS, QDF_TRACE_LEVEL_DEBUG,
				  "Skip cac as there are running vaps");
			return 1;
                }
	}

	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return 1;
	}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
	if (vap->iv_opmode == IEEE80211_M_STA && mlme_is_stacac_running(vap))
	    is_sta_cac_needed = true;
#endif

	/* For a STA vap configured with "stadfsen" and associating to root AP
	 * on a NOL History channel, we need to do CAC.
	 * Hence need not invoke mlme_dfs_is_cac_required_on_dfs_curchan() API
	 * to check if cac is required.
	 */
	if (!is_sta_cac_needed &&
	    !mlme_dfs_is_cac_required_on_dfs_curchan(pdev,
						     &continue_current_cac,
						     vap->restart_txn)) {
	    /* CAC is not required. */
	    wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
	    return 1;
	}

	if (ieee80211_dfs_send_dfs_wait(ic, vap)) {
		if (!continue_current_cac) {
			STA_VAP_DOWNUP_LOCK(ic);
			mlme_dfs_set_cac_timer_running(pdev, 1);
			STA_VAP_DOWNUP_UNLOCK(ic);
#ifdef ATH_SUPPORT_DFS
			qdf_sched_work(NULL,&ic->dfs_cac_timer_start_work);
#endif
		}
	}

	mlme_dfs_deliver_cac_state_events(ic->ic_pdev_obj);
	/* Perform dfs cac routines */
	vap->iv_dfs_cac(vap);

	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
	return 0;
}

/*
 * Handle scan-cancel in process-context
 * so that any scheduling, like sleep, can
 * happen in scan cancel without any panic.
 * After scan cancel start the timer.
 */
void
ieee80211_dfs_cac_timer_start_async(void *data)
{
	struct ieee80211com *ic = (struct ieee80211com *)data;
	struct ieee80211vap *tmpvap ;
	struct wlan_objmgr_pdev *pdev;
	osif_dev *osdev;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	if (ic->ic_sta_vap) {
		tmpvap = ic->ic_sta_vap;
		/* Cancel all vdev scans before CAC start */
		osdev = (osif_dev *)tmpvap->iv_ifp;
		wlan_ucfg_scan_cancel(tmpvap, osdev->scan_requestor, 0,
				WLAN_SCAN_CANCEL_PDEV_ALL, true);
	}
	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return;
	}
	mlme_dfs_start_cac_timer(pdev);
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);

	/*Send a CAC start event to user space*/
	OSIF_RADIO_DELIVER_EVENT_CAC_START(ic);
}

int
ieee80211_dfs_cac_cancel(struct ieee80211com *ic)
{
	struct ieee80211vap *vap = NULL;
	struct ieee80211vap *stavap = NULL;
	struct wlan_objmgr_pdev *pdev;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return -1;
	}

	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return -1;
	}
	if(mlme_dfs_is_ap_cac_timer_running(pdev)) {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
		STA_VAP_DOWNUP_LOCK(ic);
		stavap = ic->ic_sta_vap;
		if(stavap) {
			if (mlme_get_active_req_type(stavap) == MLME_REQ_REPEATER_CAC)
				IEEE80211_DELIVER_EVENT_MLME_REPEATER_CAC_COMPLETE(stavap,1);
		}
		STA_VAP_DOWNUP_UNLOCK(ic);
	} else {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
	}

	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		if (wlan_vdev_mlme_get_state(vap->vdev_obj) !=
					WLAN_VDEV_S_DFS_CAC_WAIT) {
			continue;
		}
		ieee80211_dfs_cac_stop(vap, 1);
	}
	return 0;
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
int
ieee80211_dfs_stacac_cancel(struct ieee80211com *ic)
{
	struct ieee80211vap *vap;
	struct wlan_objmgr_pdev *pdev;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return -1;
	}

	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		if (vap->iv_opmode == IEEE80211_M_STA &&
				ieee80211com_has_cap_ext(ic,IEEE80211_CEXT_STADFS)) {
			if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
					QDF_STATUS_SUCCESS) {
				return -1;
			}
			mlme_dfs_stacac_stop(pdev);
			wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
			mlme_cancel_stacac_timer(vap);
			mlme_reset_mlme_req(vap);
			mlme_clear_mlme_req(vap);
			mlme_set_stacac_running(vap,0);
			mlme_set_stacac_valid(vap,1);
			break;
		}
	}
	return 0;
}
#endif

/*
 * Clear the CAC timer.
 */
void
ieee80211_dfs_cac_stop(struct ieee80211vap *vap,
		int force)
{
	struct wlan_objmgr_pdev *pdev;
	struct ieee80211com *ic = vap->iv_ic;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return;
	}
	if(!force && (!mlme_dfs_is_ap_cac_timer_running(pdev) ||
				ieee80211_dfs_vaps_in_dfs_wait(ic, vap))) {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
		return;
	}
	mlme_dfs_cac_stop(pdev);
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
}

/*
 * Fetch a mute test channel which matches the operational mode/flags of the
 * current channel.
 *
 * Simply returning '36' when the AP is in HT40D mode will fail; the channel
 * lookup will be done with the channel flags requiring HT40D and said lookup
 * won't find a channel.
 *
 * XXX TODO: figure out the correct mute channel to return for VHT operation.
 *   It may be that we instead have to return the actual full VHT channel
 *   configuration (freq1, freq2, legacy ctl/ext info) when it's time to do
 *   this.
 */

static int
ieee80211_get_test_mute_chan(struct ieee80211com *ic,
		const struct ieee80211_ath_channel *chan)
{
	if (!IEEE80211_IS_CHAN_VALID(chan))
		return IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11A;

	if (IEEE80211_IS_CHAN_VHT(chan))
		qdf_print("%s: VHT not yet supported here (please fix); "
				"freq=%d, flags=0x%016llx, "
				"falling through\n",
				__func__, chan->ic_freq, chan->ic_flags);

	if (IEEE80211_IS_CHAN_11N_HT40MINUS(chan))
		return IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11NHT40D;
	else if (IEEE80211_IS_CHAN_11N_HT40PLUS(chan))
		return IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11NHT40U;
	else if (IEEE80211_IS_CHAN_11N_HT20(chan))
		return IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11NHT20;
	else if (IEEE80211_IS_CHAN_A(chan))
		return IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11A;
	else {
		qdf_print("%s: unknown channel mode, freq=%d, flags=0x%016llx",
				__func__, chan->ic_freq, chan->ic_flags);
		return IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11A;
	}
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
/**
 * ieee80211_process_radar_during_stacac() - Process radar during staCAC
 * @ic: Pointer to ieee80211com radio obj.
 *
 * Return: QDF_STATUS_SUCCESS if radar is processed here, else
 * QDF_STATUS_E_FAILURE.
 */
static QDF_STATUS
ieee80211_process_radar_during_stacac(struct ieee80211com *ic) {
	struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	struct ieee80211vap *sta_vap;

	STA_VAP_DOWNUP_LOCK(ic);
	sta_vap = ic->ic_sta_vap;
	if (sta_vap) {
		/*
		 * If radar is found during staCAC, flush the scan results and
		 * bring down the connection and vdev SM.
		 */
		if (mlme_is_stacac_running(sta_vap)) {
			ieee80211_dfs_stacac_cancel(ic);
			if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			    QDF_STATUS_SUCCESS) {
				status = QDF_STATUS_SUCCESS;
				goto exit;
			}
			ucfg_scan_flush_results(pdev, NULL);
			ieee80211_mlme_join_infra_continue(sta_vap, EINVAL);
			wlan_vdev_mlme_sm_deliver_evt(sta_vap->vdev_obj,
						      WLAN_VDEV_SM_EV_DOWN,
						      0, NULL);
			wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
			status = QDF_STATUS_SUCCESS;
			goto exit;
		}
	}

exit:
	STA_VAP_DOWNUP_UNLOCK(ic);
	return status;
}
#else
static inline QDF_STATUS
ieee80211_process_radar_during_stacac(struct ieee80211com *ic) {
	return QDF_STATUS_E_FAILURE;
}
#endif

static QDF_STATUS
ieee80211_find_csa_chan_and_restart(struct ieee80211com *ic,
				    uint16_t freq,
				    uint16_t vhtop_freq_seg2,
				    uint64_t flags)
{
	ic->ic_chanchange_params.freq = freq;
	ic->ic_chanchange_params.cfreq2 = vhtop_freq_seg2;
	set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, 0);
	ic->ic_chanchange_tbtt = ic->ic_chan_switch_cnt;

	*ic->ic_chanchange_channel =
		ieee80211_find_channel(ic, &ic->ic_chanchange_params, flags);
	if (!IEEE80211_IS_CHAN_VALID(ic->ic_chanchange_channel))
		return QDF_STATUS_E_FAILURE;

	ic->ic_chanchange_secoffset =
		ieee80211_sec_chan_offset(ic, ic->ic_chanchange_channel);

	ic->ic_chanchange_chwidth =
		ieee80211_get_chan_width(ic->ic_chanchange_channel);

#ifdef MAGPIE_HIF_GMAC
	TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next)
		ic->ic_chanchange_cnt += ic->ic_chanchange_tbtt;
#endif

	wlan_iterate_vap_list(ic, ieee80211_vap_iter_update_des_chan,
			      (void *)ic->ic_chanchange_channel);

	wlan_pdev_mlme_vdev_sm_csa_restart(ic->ic_pdev_obj,
					   ic->ic_chanchange_channel);

	return QDF_STATUS_SUCCESS;
}

void ieee80211_mark_dfs(struct ieee80211com *ic,
		uint8_t ieee,
		uint16_t freq,
		uint16_t vhtop_freq_seg2,
		uint64_t flags,
		uint16_t dfs_radar_bitmap)
{
    struct ieee80211_ath_channel chan_obj;
	struct ieee80211_ath_channel *c = &chan_obj;
	struct ieee80211vap *vap;
#ifdef MAGPIE_HIF_GMAC
	struct ieee80211vap* tmp_vap = NULL;
#endif
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_lmac_if_rx_ops *rx_ops = NULL;
	u_int8_t radar_detected;
	struct chan_params chan_p = {0};

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	chan_p.freq = freq;
	chan_p.cfreq2 = vhtop_freq_seg2;
	set_chan_params_puncture_bitmap(&chan_p, 0);
	/* get an AP vap */
	vap = TAILQ_FIRST(&ic->ic_vaps);
	while (vap) {
		if ((vap->iv_opmode == IEEE80211_M_HOSTAP) && !IEEE80211_IS_SPECIAL_VAP(vap) &&
		    !IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
			break;
		}
		vap = TAILQ_NEXT(vap, iv_next);
	}

	if (vap)
		psoc = wlan_vdev_get_psoc(vap->vdev_obj);
	if (psoc)
		rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	/* Notify user space of radar detection via ALD Netlink */
	if (vap) {
		radar_detected = 1;
		if (rx_ops && rx_ops->son_rx_ops.deliver_event)
			rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_CAC_STATUS, &radar_detected);
	}

	if (ieee80211_process_radar_during_stacac(ic) == QDF_STATUS_SUCCESS)
		return;

	if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
		ieee80211_dfs_cac_cancel(ic);
		/* Mark the channel in the ic_chan list */
		/*
		 * XXX TODO: this isn't exactly correct.
		 * Specifically - it only marks the channels that match
		 * the given centre frequency as having DFS, rather than
		 * actually checking for channel overlap.  So it's not
		 * entirely correct behaviour for VHT or HT40 operation.
		 *
		 * In any case, it should eventually be taught to just
		 * use the channel centre and channel overlap code
		 * that now exists in umac/base/ieee80211_channel.c
		 * and flag them all.
		 *
		 * In actual reality, this also gets set correctly by
		 * the NOL dfs channel list update method.
		 */
		if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
				QDF_STATUS_SUCCESS) {
			return;
		}
		if ((ic->ic_flags_ext & IEEE80211_FEXT_MARKDFS) &&
				(mlme_dfs_get_usenol(pdev))) {
			wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
			/*
			 * IR: 115114 -- Primary Non-DFS channel is excluded
			 * from channel slection in HT80_80 mode
			 *
			 * The code that was used previously to set channel flag
			 * to indicate radar was found in the channel is discarded
			 * because of the following:
			 *
			 * 1. Redundant -- as flags are already set (marked) by combination
			 * of dfs_channel_mark_radar/dfs_nol_update/ic_dfs_clist_update
			 * functions.
			 *
			 * 2. The code does not work where we can mix DFS/Non-DFS channels
			 * and channel marking of radar. This can happen specially in
			 * in ht80_80 mode and ht160. The code (erroneously) prvents
			 * use of non-DFS primary 20 MHz control channel if radar is
			 * found.
			 */

			*c = ieee80211_find_channel(ic, &chan_p, flags);

			if (!IEEE80211_IS_CHAN_VALID(c)){
				return;
			}

			/*
			 * If the reported event is on the current channel centre
			 * frequency, begin the process of moving to another
			 * channel.
			 *
			 * Unfortunately for now, this is not entirely correct -
			 * If we report a radar event on a subchannel of the current
			 * channel, this test will fail and we'll end up not
			 * starting a CSA.
			 *
			 * XXX TODO: this API needs to change to take a radar event
			 * frequency/width, instead of a channel.  It's then up
			 * to the alternative umac implementations to write glue
			 * to correctly handle things.
			 */
			if (ic->ic_curchan->ic_freq == c->ic_freq) {
			    while (vap != NULL) {
				if ((vap->iv_ic == ic) &&
				    (wlan_vdev_chan_config_valid(vap->vdev_obj) ==
				     QDF_STATUS_SUCCESS) &&
				    vap->iv_opmode == IEEE80211_M_HOSTAP) {
				    break;
				}
				vap = TAILQ_NEXT(vap, iv_next);
			    }
			    if (vap == NULL) {
				return;
			    }
			    IEEE80211_RADAR_FOUND_LOCK(ic);
			    ieee80211_dfs_action(vap, NULL, false, dfs_radar_bitmap);
			    IEEE80211_RADAR_FOUND_UNLOCK(ic);
			}
		} else {
			wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
			/* Change to a radar free 11a channel for dfstesttime seconds */
			if (ieee80211_find_csa_chan_and_restart(ic,
								freq,
								vhtop_freq_seg2,
								flags) !=
						QDF_STATUS_SUCCESS) {
				return;
			}
			/* A timer is setup in the radar task if markdfs is not set and
			 * we are in hostap mode.
			 */
		}
	} else {
		/* Are we in sta mode? If so, send an action msg to ap saying we found  radar? */
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
		if (ic->ic_opmode == IEEE80211_M_STA) {
			ieee80211_dfs_stacac_cancel(ic);
			/* Mark the channel in the ic_chan list */
			/*
			 * XXX TODO: this isn't exactly correct.
			 * Specifically - it only marks the channels that match
			 * the given centre frequency as having DFS, rather than
			 * actually checking for channel overlap.  So it's not
			 * entirely correct behaviour for VHT or HT40 operation.
			 *
			 * In any case, it should eventually be taught to just
			 * use the channel centre and channel overlap code
			 * that now exists in umac/base/ieee80211_channel.c
			 * and flag them all.
			 *
			 * In actual reality, this also gets set correctly by
			 * the NOL dfs channel list update method.
			 */
			if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
					QDF_STATUS_SUCCESS) {
				return;
			}
			if ((ic->ic_flags_ext & IEEE80211_FEXT_MARKDFS) &&
					(mlme_dfs_get_usenol(pdev) == 1)) {
				*c = ieee80211_find_channel(ic, &chan_p, flags);

				if (!IEEE80211_IS_CHAN_VALID(c)) {
					wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
					return;
				}
			}
			vap = TAILQ_FIRST(&ic->ic_vaps);
			while (vap != NULL) {
				if(vap->iv_opmode == IEEE80211_M_STA) {
					ucfg_scan_flush_results(pdev, NULL);
					ieee80211_indicate_sta_radar_detect(vap->iv_bss);
					break;
				}
				vap = TAILQ_NEXT(vap, iv_next);
			}
			wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
			return;
		}
#endif
	} /* End of else for STA mode */
}

/*
 * Start the process of sending a CSA and doing a channel switch.
 *
 * By setting IEEE80211_F_CHANSWITCH, the beacon update code
 * will add a CSA IE.  Once the count reaches 0, the channel
 * switch will occur.
 */
QDF_STATUS
ieee80211_start_csa(struct ieee80211com *ic,
		uint8_t ieeeChan,
		uint16_t freq,
		uint16_t cfreq_80_mhz_seg2,
		uint64_t flags)
{
#ifdef MAGPIE_HIF_GMAC
	struct ieee80211vap *tmp_vap = NULL;
#endif

	if (!ieee80211_num_apvap_running(ic)) {
		qdf_err("No vaps up and running, cannot send CSA\n");
		return QDF_STATUS_E_FAILURE;
	}

	IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_DEBUG,
			"Beginning CSA to channel %d\n", ieeeChan);

	return ieee80211_find_csa_chan_and_restart(ic, freq, cfreq_80_mhz_seg2,
						   flags);
}

void ieee80211_dfs_start_tx_rcsa_and_waitfor_rx_csa(struct ieee80211com *ic)
{
	struct wlan_objmgr_pdev *pdev;
	bool is_rcsa_ie_sent;
	QDF_STATUS status;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s : pdev is null", __func__);
		return;
	}

	status = mlme_dfs_get_rcsa_flags(pdev, &is_rcsa_ie_sent, NULL);

	if (status == QDF_STATUS_E_FAILURE)
	{
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s: Could not fetch flags during RCSA\n", __func__);
			return;
	}
#ifdef ATH_SUPPORT_DFS
	if(!ic->ic_dfs_waitfor_csa_sched) {
		ic->ic_rcsa_count = RCSA_INIT_COUNT;

		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
				"%s: start the 0 sec timer, rcsa_count=%d\n",
				__func__, ic->ic_rcsa_count);
		/* Start timer for sending RCSA */
		OS_SET_TIMER(&ic->ic_dfs_tx_rcsa_and_nol_ie_timer,0);

		/* If we are not sending RCSA then do not need to wait for CSA and
		 * thus the wait timers should not be set.
		 *
		 * In case of ZeroCAC, if radar found in the second segment, we only
		 * send the Add-to-NOL IE to the Root AP and we do not send RCSA. */
		if(is_rcsa_ie_sent) {
			ic->ic_dfs_waitfor_csa_sched = 1;
			if(IEEE80211_IS_CSH_OPT_APRIORI_NEXT_CHANNEL_ENABLED(ic)) {
				/* Wait for 5 RCSAs to be propagated(500ms) + 100ms grace period*/
				OS_SET_TIMER(&ic->ic_dfs_waitfor_csa_timer,WAIT_FOR_RCSA_COMPLETION);
			} else {
				/* Assuming a chain of max 5 repeaters and a RCSA interval of 100ms,
				 * it will take at most 1sec for the CSA to come back. Also have
				 * additional grace period of 1 sec. Total wait period 2 secs.
				 * Wait for 2 sec to get back CSA from parent Repeater/Root.
				 * If within 2 sec CSA does not come back then call nofity radar
				 */
				OS_SET_TIMER(&ic->ic_dfs_waitfor_csa_timer,WAIT_FOR_CSA_TIME);
			}
		}
	}
#endif
}

void ieee80211_dfs_rx_rcsa(struct ieee80211com *ic)
{
	struct wlan_objmgr_pdev *pdev;
	bool is_nol_ie_recvd;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	if(IEEE80211_IS_CSH_PROCESS_RCSA_ENABLED(ic)) {
		if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
				QDF_STATUS_SUCCESS) {
			return;
		}
		mlme_dfs_radar_disable(pdev);
#if ATH_SUPPORT_ZERO_CAC_DFS
		mlme_dfs_second_segment_radar_disable(pdev);
#endif
		mlme_dfs_get_rcsa_flags(pdev, NULL, &is_nol_ie_recvd);
		/* Check if NOL has been updated by the parsing NOL IE.
		 * else, mark the channels as radar from here.
		 *
		 * This is only when uplink is present, processing in root
		 * is already taken care of.
		 */
		if (!is_nol_ie_recvd) {
			if (mlme_dfs_get_update_nol_flag(pdev) == false &&
				mlme_dfs_get_rn_use_nol(pdev) == 1) {
				ieee80211_dfs_channel_mark_radar(ic, ic->ic_curchan);
				mlme_dfs_set_update_nol_flag(pdev, true);
			}
		}
		wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
		ieee80211_dfs_start_tx_rcsa_and_waitfor_rx_csa(ic);
	}
}

void ieee80211_dfs_cancel_waitfor_csa_timer(struct ieee80211com *ic)
{
#ifdef ATH_SUPPORT_DFS
	if(ic->ic_dfs_waitfor_csa_sched) {
		OS_CANCEL_TIMER(&ic->ic_dfs_waitfor_csa_timer);
		ic->ic_dfs_waitfor_csa_sched = 0;
	}
#endif
}

/* Timer function to wait for CSA from uplink after sending RCSA */
OS_TIMER_FUNC(ieee80211_dfs_waitfor_csa_task)
{
	struct ieee80211com *ic = NULL;
	OS_GET_TIMER_ARG(ic, struct ieee80211com *);

	IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s: waited long enough now change the channel\n",__func__);
	ieee80211_mark_dfs(ic, ic->ic_curchan->ic_ieee,
			ic->ic_curchan->ic_freq,
			ic->ic_curchan->ic_vhtop_freq_seg2,
			ic->ic_curchan->ic_flags,
			0);
#ifdef ATH_SUPPORT_DFS
	ic->ic_dfs_waitfor_csa_sched = 0;
#endif
}

/* Function to append add_to_nol IE to the action frame. */
uint8_t *ieee80211_add_nol_ie(uint8_t *frm, struct ieee80211vap *vap,
			      struct ieee80211com *ic)
{
	struct vendor_add_to_nol_ie *nol_el =
				(struct vendor_add_to_nol_ie *)frm;
	uint8_t nol_elem_len = sizeof(struct vendor_add_to_nol_ie);
	uint8_t bandwidth;
	uint16_t startfreq;
	uint8_t bitmap;
	struct wlan_objmgr_pdev *pdev;
	QDF_STATUS status;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s : pdev is null", __func__);
		return frm;
	}

	status = mlme_dfs_fetch_nol_ie_info(pdev, &bandwidth,
					    &startfreq, &bitmap);
	if (status == QDF_STATUS_E_FAILURE)
	{
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s: Could not fetch NOL info during RCSA\n", __func__);
		return frm;
	}

	qdf_mem_zero(nol_el, nol_elem_len);
	nol_el->ie = IEEE80211_ELEMID_VENDOR;
	nol_el->len = sizeof(struct vendor_add_to_nol_ie)
		     - sizeof(struct ieee80211_ie_header);
	nol_el->bandwidth = bandwidth;
	nol_el->startfreq = htole16(startfreq);
	nol_el->bitmap = bitmap;

	return frm + nol_elem_len;
}

void ieee80211_dfs_send_rcsa(struct ieee80211com *ic,
		struct ieee80211vap *vap,
		uint8_t rcsa_count,
		uint16_t ieee_freq)
{
	struct ieee80211_action_mgt_args *actionargs;
	bool is_rcsa_ie_sent;
	bool is_nol_ie_sent;
	struct wlan_objmgr_pdev *pdev;
	QDF_STATUS status;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s : pdev is null", __func__);
		return;
	}

	status = mlme_dfs_get_rcsa_flags(pdev, &is_rcsa_ie_sent,
					 &is_nol_ie_sent);
	if (status == QDF_STATUS_E_FAILURE)
	{
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
			"%s: Could not fetch flags from dfs during RCSA\n", __func__);
		return;
	}

	if( NULL != vap) {
		actionargs = OS_MALLOC(vap->iv_ic->ic_osdev, sizeof(struct ieee80211_action_mgt_args) , GFP_KERNEL);
		if (actionargs == NULL) {
			IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
					"%s: Unable to alloc arg buf. Size=%d\n",
					__func__, sizeof(struct ieee80211_action_mgt_args));
			return ;
		}
		OS_MEMZERO(actionargs, sizeof(struct ieee80211_action_mgt_args));

		actionargs->category = IEEE80211_ACTION_CAT_VENDOR;

		/* Sending only NOL IE */
		if(!is_rcsa_ie_sent && is_nol_ie_sent) {
			actionargs->action   = IEEE80211_ELEMID_VENDOR;
			IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
					"%s: sending action frame category=%d action=%d\n",
					__func__,actionargs->category, actionargs->action);
			ieee80211_send_action(vap->iv_bss, actionargs, NULL);
		}
		/* Sending either RCSA + NOL IE or only RCSA IE */
		else if(is_rcsa_ie_sent) {
			actionargs->action   = IEEE80211_ACTION_CHAN_SWITCH;
			actionargs->arg1     = is_nol_ie_sent;
			IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
					"%s: seding action frame category=%d action=%d chan freq=%d\n",
					__func__,actionargs->category,actionargs->action,ic->ic_chanchange_params.freq);
			ieee80211_send_action(vap->iv_bss, actionargs, NULL);
		}
		else
			IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Can't send RCSA !!\n",
					__func__);
		OS_FREE(actionargs);
	}
}

bool ieee80211_dfs_start_rcsa(struct ieee80211com *ic)
{
	struct wlan_objmgr_pdev *pdev;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return false;
	}

	if(IEEE80211_IS_CSH_RCSA_TO_UPLINK_ENABLED(ic)) {
		/* Do not handle the radar detect here the Repeater sends RCSA
		 * to its parent Repeater/Root and wait for CSA. The Radar
		 * detect is handled after receiveing the CSA or wait for CSA
		 * timer expires.
		 */
		struct ieee80211vap *stavap = NULL;

		STA_VAP_DOWNUP_LOCK(ic);
		stavap = ic->ic_sta_vap;
		if(stavap &&
                   (wlan_vdev_is_up(stavap->vdev_obj) == QDF_STATUS_SUCCESS)) {
			mlme_dfs_radar_disable(pdev);
#if ATH_SUPPORT_ZERO_CAC_DFS
			mlme_dfs_second_segment_radar_disable(pdev);
#endif
			ieee80211_dfs_start_tx_rcsa_and_waitfor_rx_csa(ic);
			STA_VAP_DOWNUP_UNLOCK(ic);
			return true;
		} else {
			IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
					"%s: No uplink found so handle RADAR as before\n",__func__);
		}
		STA_VAP_DOWNUP_UNLOCK(ic);
	}

    return false;
}

/* Send 5 RCSAs to uplink every RCSA_INTVAL */
OS_TIMER_FUNC(ieee80211_dfs_tx_rcsa_task)
{
	struct ieee80211com *ic = NULL;
	struct ieee80211vap *stavap = NULL;
	struct wlan_objmgr_pdev *pdev;

	OS_GET_TIMER_ARG(ic, struct ieee80211com *);

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	STA_VAP_DOWNUP_LOCK(ic);
	stavap = ic->ic_sta_vap;
	if(stavap &&
           (wlan_vdev_is_up(stavap->vdev_obj) == QDF_STATUS_SUCCESS) &&
           ic->ic_rcsa_count) {
		IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
				"%s: sending action frame, rcsa_count=%d\n", __func__,ic->ic_rcsa_count);
		ieee80211_dfs_send_rcsa(ic, stavap, ic->ic_rcsa_count, ic->ic_curchan->ic_freq);
#ifdef ATH_SUPPORT_DFS
		OS_SET_TIMER(&ic->ic_dfs_tx_rcsa_and_nol_ie_timer, RCSA_INTVAL);
#endif
		ic->ic_rcsa_count--;
		if (ic->ic_rcsa_count == 0) {
			if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
					QDF_STATUS_SUCCESS) {
				return;
			}
			mlme_dfs_set_update_nol_flag(pdev, false);
			mlme_dfs_set_rcsa_flags(pdev, false, false);
			wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
		}
	}
	STA_VAP_DOWNUP_UNLOCK(ic);
}

/*
 * Mark a channel as having interference detected upon it.
 *
 * This adds the interference marker to both the primary and
 * extension channel.
 *
 * XXX TODO: make the NOL and channel interference logic a bit smarter
 * so only the channel with the radar event is marked, rather than
 * both the primary and extension.
 */
void
ieee80211_dfs_channel_mark_radar(struct ieee80211com *ic,
		struct ieee80211_ath_channel *chan)
{
	struct ath_sub_chan_freq_list chan_info;
	int dfs_nol_timeout = 0;
	int i;
	struct wlan_objmgr_pdev *pdev;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	/*
	 * If radar is detected in 40MHz mode, add both the primary and the
	 * extension channels to the NOL. chan is the channel data we return
	 * to the ath_dev layer which passes it on to the 80211 layer.
	 * As we want the AP to change channels and send out a CSA,
	 * we always pass back the primary channel data to the ath_dev layer.
	 */
	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return;
	}
	if ((mlme_dfs_get_rn_use_nol(pdev) == 1) &&
			(ic->ic_opmode == IEEE80211_M_HOSTAP
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
			|| ((ic->ic_opmode == IEEE80211_M_STA) &&
			(ieee80211com_has_cap_ext(ic,IEEE80211_CEXT_STADFS)))))
#else
		))
#endif
		{
		chan_info.cl_nchans= 0;
		ieee80211_get_extchaninfo (ic, chan, &chan_info);
		for (i = 0; i < chan_info.cl_nchans; i++) {
			if (!chan_info.sub_20chan_freq[i]) {
				IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
						"%s: NULL channel\n", __func__);
			} else {
				/* Do not add non-DFS segment of current channel to NOL */
				if (wlan_reg_is_dfs_for_freq(pdev, chan_info.sub_20chan_freq[i])) {
					mlme_dfs_get_nol_timeout(pdev, &dfs_nol_timeout);
					mlme_dfs_nol_addchan(pdev, chan_info.sub_20chan_freq[i],
							dfs_nol_timeout);
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
					mlme_dfs_reg_update_nol_ch_for_freq(pdev,
								   &chan_info.sub_20chan_freq[i],1,
								   DFS_NOL_SET);
					mlme_dfs_set_update_nol_flag(pdev,
								     true);
#endif
				} else {
					IEEE80211_DPRINTF_IC_CATEGORY(ic,IEEE80211_MSG_DFS,
							"%s: Will not add non-DFS channel (freq=%d) to"
							" NOL\n", __func__,
							chan_info.sub_20chan_freq[i]);
				}
			}
		}

		/* Update the umac/driver channels with the new NOL information. */
		mlme_dfs_nol_update(pdev);
		if (ic->ic_dfs_info_notify_channel_available) {
			ieee80211_channel_notify_to_app(ic);
		}
	}
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
}
qdf_export_symbol(ieee80211_dfs_channel_mark_radar);

void ieee80211_update_dfs_next_channel(struct ieee80211com *ic)
{
	uint16_t target_chan_freq = 0;
	struct ieee80211_ath_channel ptarget_channel_obj;
	struct ieee80211_ath_channel *ptarget_channel = &ptarget_channel_obj;
	struct wlan_objmgr_pdev *pdev;
	struct ch_params ch_params;
	enum ieee80211_phymode chan_mode;
	uint16_t flag = 0;
	struct wlan_objmgr_psoc *psoc;

	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		qdf_err("%s : psoc is null", __func__);
		return;
	}

	ieee80211_regdmn_get_chan_params(ic, &ch_params);

	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return;
	}

	if (IEEE80211_IS_CSH_NONDFS_RANDOM_ENABLED(ic)) {
		flag = DFS_RANDOM_CH_FLAG_NO_DFS_CH |
			DFS_RANDOM_CH_FLAG_NO_CURR_OPE_CH;
		if (wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_SOC_RESTRICTED_80P80_SUPPORT))
			flag |= DFS_RANDOM_CH_FLAG_RESTRICTED_80P80_ENABLED;

		target_chan_freq =
			mlme_dfs_random_channel(pdev, &ch_params, flag);
	}

	if (!target_chan_freq) {
		flag = DFS_RANDOM_CH_FLAG_NO_CURR_OPE_CH;
		if (wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_SOC_RESTRICTED_80P80_SUPPORT))
			flag |= DFS_RANDOM_CH_FLAG_RESTRICTED_80P80_ENABLED;

		target_chan_freq =
			mlme_dfs_random_channel(pdev, &ch_params, flag);
	}

	if (target_chan_freq) {
		struct chan_params chan_p = {0};
		chan_p.freq = target_chan_freq;
		chan_p.cfreq2 = ch_params.mhz_freq_seg1;
		set_chan_params_puncture_bitmap(&chan_p, 0);

		chan_mode = ieee80211_get_target_channel_mode(ic, &ch_params);
		*ptarget_channel = ieee80211_find_dot11_channel(ic,
				&chan_p,
				chan_mode);
	} else {
		*ptarget_channel = empty_channel;
	}

	if(!IEEE80211_IS_CHAN_VALID(ptarget_channel)) {
		/* should never come here? */
		qdf_print("Cannot change to any channel");
		*ic->ic_tx_next_ch = empty_channel;
	} else {
		*ic->ic_tx_next_ch = *ptarget_channel;
	}

	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
}
qdf_export_symbol(ieee80211_update_dfs_next_channel);

struct ieee80211_ath_channel ieee80211_get_extchan(struct ieee80211com *ic)
{
	int chan_offset = 0;
	struct chan_params chan_p = {0};

	if (IEEE80211_IS_CHAN_HT40PLUS_CAPABLE(ic->ic_curchan))
		chan_offset = 20;
	else if (IEEE80211_IS_CHAN_HT40MINUS_CAPABLE(ic->ic_curchan))
		chan_offset = -20;
	else
		return empty_channel;

	chan_p.freq = ic->ic_curchan->ic_freq + chan_offset;
	chan_p.cfreq2 = 0;
	return ieee80211_find_channel(ic, &chan_p,
			(IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_HT20));
}

void ieee80211_dfs_channel_change_by_precac(struct ieee80211com *ic)
{
	struct ieee80211vap *tmp_vap = NULL;

	TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
		if (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS) {
#if ATH_SUPPORT_ZERO_CAC_DFS
			tmp_vap->iv_pre_cac_timeout_channel_change = 1;
#endif
			wlan_vdev_mlme_sm_deliver_evt(tmp_vap->vdev_obj,
				WLAN_VDEV_SM_EV_FW_VDEV_RESTART, 0, NULL);
		}
	}
}
qdf_export_symbol(ieee80211_dfs_channel_change_by_precac);

#ifdef WLAN_DFS_PRECAC_AUTO_CHAN_SUPPORT
/**
 * ieee80211_autochan_switch_chan_change_csa() : Change channel to desired
                                                 channel after precac timeout.
 * @ic:       Pointer to ic.
 * @des_chan: Desired channel number.
 * @des_mode: Desired channel width.
 */
void ieee80211_autochan_switch_chan_change_csa(struct ieee80211com *ic,
                                               uint16_t freq,
                                               uint16_t cfreq2,
                                               enum ieee80211_phymode des_mode)
{
    struct ieee80211vap *vap = NULL;
    struct chan_params chan_p = {0};

    if (!TAILQ_EMPTY(&ic->ic_vaps))
        vap = TAILQ_FIRST(&ic->ic_vaps);
    if (!vap)
        return;

    qdf_info("Change channel to desired channel freq[%d] and mode to[%d]",
             freq, des_mode);
    /*
     * For des_mode being IEEE80211_MODE_11AC_VHT80, changing the channel
     * using CSA is sufficient since we are switching only channel from
     * intermediate to desired channel by remaining in same mode
     * (IEEE80211_MODE_11AC_VHT80). For des_mode=IEEE80211_MODE_11AC_VHT160,
     * both channel change from intermediate to desired channel and mode change
     * from IEEE80211_MODE_11AC_VHT80 to IEEE80211_MODE_11AC_VHT160 required.
     * Hence send OMN before changing channel in IEEE80211_MODE_11AC_VHT160.
     */
    chan_p.freq = freq;
    chan_p.cfreq2 = cfreq2;
    if (ieee80211_is_phymode_80(des_mode)) {
        ieee80211_ucfg_set_chanswitch(vap, &chan_p,
                                      ic->ic_chan_switch_cnt,
                                      CHWIDTH_80);
    } else if (ieee80211_is_phymode_160(des_mode)) {
        wlan_set_desired_phymode(vap, des_mode);
        /* Send broadcast OMN */
        wlan_set_param(vap, IEEE80211_OPMODE_NOTIFY_ENABLE, 1);
        /* Send unicast OMN */
        wlan_iterate_station_list(vap, wlan_send_omn_action, NULL);
        ieee80211_ucfg_set_chanswitch(vap, &chan_p,
                                      ic->ic_chan_switch_cnt,
                                      CHWIDTH_160);
    }
}
qdf_export_symbol(ieee80211_autochan_switch_chan_change_csa);
#endif

#if defined(QCA_DFS_BW_PUNCTURE)
void ieee80211_unpunc_chan_switch(struct ieee80211com *ic,
                                  uint16_t new_punc_pattern)
{
    struct ieee80211_ath_channel ptarget_channel_obj;
    struct ieee80211_ath_channel *ptarget_channel = &ptarget_channel_obj;

    *ptarget_channel = *ic->ic_curchan;
    *ic->ic_chanchange_channel = *ic->ic_curchan;
    ic->ic_chanchange_secoffset =
        ieee80211_sec_chan_offset(ic, ic->ic_chanchange_channel);
    ic->ic_chanchange_chwidth =
        ieee80211_get_chan_width(ic->ic_chanchange_channel);
    ic->ic_chanchange_params.freq = ptarget_channel->ic_freq;
    ic->ic_chanchange_params.cfreq2 = ptarget_channel->ic_vhtop_freq_seg2;
    ic->ic_chanchange_chanflag  = ic->ic_chanchange_channel->ic_flags;
    ptarget_channel->op_puncture_bitmap = new_punc_pattern;
    ic->ic_chanchange_channel->op_puncture_bitmap = new_punc_pattern;
    set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, new_punc_pattern);
    wlan_iterate_vap_list(ic, ieee80211_vap_iter_update_des_chan,
                                                ptarget_channel);
    wlan_pdev_mlme_vdev_sm_csa_restart(ic->ic_pdev_obj,
                                       ptarget_channel);
}
#endif

#if defined(QCA_SUPPORT_DFS_CHAN_POSTNOL) || defined(QCA_DFS_BW_EXPAND)
/**
 * ieee80211_postnol_chan_switch : Change channel to the desired channel post
 * NOL expiry.
 * @ic:         Pointer to ic.
 * @des_freq:   Desired channel frequency.
 * @des_cfreq2: Desired secondary center frequency.
 * @des_mode:   Desired channel width.
 */
void ieee80211_postnol_chan_switch(struct ieee80211com *ic,
                                   uint16_t des_freq,
				   uint16_t des_cfreq2,
                                   enum ieee80211_phymode des_mode)
{
    struct ieee80211vap *vap = NULL;
    enum phy_ch_width chwidth;
    struct chan_params chan_p = {0};

    if (!TAILQ_EMPTY(&ic->ic_vaps))
        vap = TAILQ_FIRST(&ic->ic_vaps);
    if (!vap)
        return;

    qdf_info("Change channel to desired channel freq[%d] and mode to[%d]",
             des_freq, des_mode);

    if (mlme_dfs_is_ap_cac_timer_running(ic->ic_pdev_obj)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s: CAC timer is running, doth_chanswitch is not allowed\n", __func__);
        return;
    }

    /* Ensure previous channel switch is not pending */
    if (ic->ic_flags & IEEE80211_F_CHANSWITCH) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s: Error: Channel change already in progress\n", __func__);
        return;
    }

    chwidth = ieee80211_get_chan_width_from_phymode(des_mode);

    chan_p.freq = des_freq;
    chan_p.cfreq2 = des_cfreq2;
    *ic->ic_chanchange_channel = ieee80211_find_dot11_channel(ic, &chan_p, des_mode);

    if (!IEEE80211_IS_CHAN_VALID(ic->ic_chanchange_channel)) {
        qdf_err("Channel to change not found!");
        return;
    } if (IEEE80211_ARE_CHANS_EQUAL(ic->ic_chanchange_channel, ic->ic_curchan)) {
        qdf_err("Channel to change same as current channel!");
        *ic->ic_chanchange_channel = empty_channel;
        return;
    }
    wlan_set_desired_phymode(vap, des_mode);

    if (des_cfreq2 && chwidth == CH_WIDTH_80P80MHZ)
        vap->iv_des_cfreq2_80_80 = des_cfreq2;

    ic->ic_chanchange_chwidth = ieee80211_get_chan_width(ic->ic_chanchange_channel);
    ic->ic_chanchange_chanflag  = ic->ic_chanchange_channel->ic_flags;
    ic->ic_chanchange_secoffset = ieee80211_sec_chan_offset(ic, ic->ic_chanchange_channel);

    /*  flag the beacon update to include the channel switch IE */
    ic->ic_chanchange_params.freq = des_freq;
    ic->ic_chanchange_params.cfreq2 = des_cfreq2;
    set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, 0);
    ic->ic_chanchange_tbtt = IEEE80211_RADAR_11HCOUNT;

    /* Set des chan for all VAPs to be same */
    wlan_iterate_vap_list(ic, ieee80211_vap_iter_update_des_chan,
                          (void *)ic->ic_chanchange_channel);

    if (in_interrupt()) {
        /* In case of interrupt context deliver event directly
         * to vdev mlme sm, avoid serialization as it demands
         * a mutex to be held which is not possible in interrupt
         * context.Need revisit to better handle this scenario.*/
        qdf_info("Invoking CSA from interrupt ctx");
        wlan_pdev_mlme_vdev_sm_csa_restart(ic->ic_pdev_obj,
                                           ic->ic_chanchange_channel);
    } else {
        /* Iterate over VAPs of the pdev and trigger CSA restart procedure */
        qdf_info("Invoking CSA from user ctx");
        wlan_mlme_pdev_csa_restart(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
    }
}
#endif

#if QCA_DFS_NOL_VAP_RESTART
#define VAP_RESTART_WAIT_TIME 2
#endif

void ieee80211_dfs_nol_timeout_notification(struct ieee80211com *ic)
{
    struct wlan_objmgr_pdev *pdev;
#if !(ATH_SUPPORT_DFS && QCA_DFS_NOL_VAP_RESTART)
    uint16_t target_chan_freq = 0;
    struct ieee80211_ath_channel ptarget_channel_obj;
    struct ieee80211_ath_channel *ptarget_channel = &ptarget_channel_obj;
    struct ch_params ch_params;
    enum ieee80211_phymode chan_mode;
#endif
    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null", __func__);
        return;
    }

    if (ic->no_chans_available == 1) {
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                QDF_STATUS_SUCCESS) {
            return;
        }
#if ATH_SUPPORT_DFS && QCA_DFS_NOL_VAP_RESTART
#define TIME_IN_MS 1000
        OS_SET_TIMER(&ic->ic_dfs_nochan_vap_restart_timer, VAP_RESTART_WAIT_TIME * TIME_IN_MS);
        ic->no_chans_available = 0;
        ic->ic_pause_stavap_scan = 1;
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
#undef TIME_IN_MS
#else
        ieee80211_regdmn_get_chan_params(ic, &ch_params);
        target_chan_freq = mlme_dfs_random_channel(pdev, &ch_params, 0);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
        if (target_chan_freq) {
	    struct chan_params chan_p = {0};
	    chan_p.freq = target_chan_freq;
	    chan_p.cfreq2 =ch_params.mhz_freq_seg1;
            chan_mode = ieee80211_get_target_channel_mode(ic,
                    &ch_params);
            *ptarget_channel = ieee80211_find_dot11_channel(ic,
                    &chan_p, chan_mode);
            *ic->ic_curchan = *ptarget_channel;
            ic->no_chans_available = 0;
            ieee80211_bringup_ap_vaps(ic);
        } else {
            ic->no_chans_available = 1;
        }
#endif
    }
}
qdf_export_symbol(ieee80211_dfs_nol_timeout_notification);

void ieee80211_dfs_clist_update(struct ieee80211com *ic,
		void *nollist,
		int nentries)
{
	if (IEEE80211_IS_CHAN_RADAR(ic, ic->ic_tx_next_ch))
		*ic->ic_tx_next_ch = empty_channel;
	wlan_scan_update_channel_list(ic);

	/*
	 * Notify upper layer about the radar update event. This may not be the
	 * right place to notify, but the NOL list here is more accurate.
	 */

	OSIF_RADIO_DELIVER_EVENT_RADAR_DETECTED(ic);

	/* Mark dfs channel switch pending only if radar is detected on
	 * current channel.
	 */
	if (IEEE80211_IS_CHAN_RADAR(ic, ic->ic_curchan)
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
	    || IEEE80211_IS_CHAN_HISTORY_RADAR(ic, ic->ic_curchan)
#endif
	   ) {
		ic->ic_flags |= IEEE80211_F_DFS_CHANSWITCH_PENDING;
	}
}

int ieee80211_dfs_get_cac_timeout(struct ieee80211com *ic,
		struct ieee80211_ath_channel *chan)
{
	int override_cactimeout = 0;
	struct wlan_objmgr_pdev *pdev;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;
	uint32_t dfs_reg = 0;

	pdev = ic->ic_pdev_obj;
	if(!pdev) {
		qdf_err("null pdev");
		return -1;
	}

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		qdf_err("null psoc");
		return -1;
	}

	if (!IEEE80211_IS_CHAN_VALID(chan)) {
		qdf_err("null chan");
		return -1;
	}

	/* CAC duration is 0 for non-DFS channels */
	if (!IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan))
		return 0;

	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
			QDF_STATUS_SUCCESS) {
		return -1;
	}

	mlme_dfs_get_override_cac_timeout(pdev, &override_cactimeout);
	wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);

	if (override_cactimeout	!= -1)
		return override_cactimeout;

	reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
	if (!reg_rx_ops) {
		qdf_err("null reg_rx_ops");
		return -1;
	}

	reg_rx_ops->get_dfs_region(pdev, &dfs_reg);

	if ((dfs_reg == DFS_ETSI_DOMAIN) &&
        (ieee80211_check_weather_radar_channel(chan)))
				return ieee80211_cac_weather_timeout;

	return ieee80211_cac_timeout;
}

void ieee80211_bringdown_vaps(struct ieee80211com *ic)
{
	struct ieee80211vap *vap = NULL;

	if (!TAILQ_EMPTY(&ic->ic_vaps)) {
	    vap = TAILQ_FIRST(&ic->ic_vaps);
	    if (vap) {
		IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
				     IEEE80211_MSG_DEBUG,
				     "vap-%d(%s) channel is not available "
				     "Brindown vaps\n",vap->iv_unit,
				     vap->iv_netdev_name);
		wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_PDEV_STOP);
	    }
	}
}
qdf_export_symbol(ieee80211_bringdown_vaps);

void ieee80211_bringdown_sta_vap(struct ieee80211com *ic)
{
    struct ieee80211vap *vap = NULL;

    if (!TAILQ_EMPTY(&ic->ic_vaps)) {
        vap = TAILQ_FIRST(&ic->ic_vaps);
        if (vap) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
                     IEEE80211_MSG_DEBUG,
                     "vap-%d(%s) Brindown vaps\n",vap->iv_unit,
                     vap->iv_netdev_name);
        wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_STA_VDEVS_STOP);
        }
    }
}
qdf_export_symbol(ieee80211_bringdown_sta_vap);

static char *dfs_event(u_int16_t event)
{
    switch (event) {
        case IEEE80211_EV_RADAR_DETECTED:
            return "RADAR_DETECT";
        case IEEE80211_EV_CAC_STARTED:
            return "CAC_START";
        case IEEE80211_EV_CAC_COMPLETED:
            return "CAC_COMPLETED";
        case IEEE80211_EV_NOL_STARTED:
            return "NOL_START";
        case IEEE80211_EV_NOL_FINISHED:
            return "NOL_FINISHED";
        case IEEE80211_EV_PRECAC_STARTED:
            return "PRECAC_STARTED";
        case IEEE80211_EV_PRECAC_COMPLETED:
            return "PRECAC_COMPLETED";
        default:
            break;
    }
    return "unknown";
}

#if WIFI_MEM_MANAGER_SUPPORT
static struct dfsreq_nolinfo *
ieee80211_dfs_get_or_allocate_persistent_nol_mem(int soc_idx, int pdev_idx, bool *is_already_allocated)
{
    *is_already_allocated = wifi_kmem_is_already_allocated(soc_idx, pdev_idx, KM_DFS_NOL);

    /* This mem_manager API makes a new allocation during first invocation.
     * During successive invocations, it returns the reference to previously
     * allocated memory. This API alone does not indicate whether this is
     * fresh allocation or not. The new function 'wifi_kmem_is_already_allocated'
     * can be used to check if the memory is being allocated for the first time
     * or not.
     */
    return (struct dfsreq_nolinfo *)(wifi_kmem_allocation(soc_idx, pdev_idx, KM_DFS_NOL, sizeof(struct dfsreq_nolinfo), 0));
}
#else
static inline
struct dfsreq_nolinfo * ieee80211_dfs_get_or_allocate_persistent_nol_mem(int soc_idx, int pdev_idx, bool *is_already_allocated)
{
    return NULL;
}
#endif

void ieee80211_dfs_allocate_nol(struct ieee80211com *ic, struct dfsreq_nolinfo **dfs_mm_nolinfo)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    int soc_idx = scn->soc->soc_idx;
    int pdev_idx;
    bool is_already_allocated;

    if (ic->ic_pdev_obj) {
        pdev_idx = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    } else {
        qdf_err("ic Pdev is NULL");
        return;
    }

    *dfs_mm_nolinfo = ieee80211_dfs_get_or_allocate_persistent_nol_mem(soc_idx, pdev_idx, &is_already_allocated);

    if (!*dfs_mm_nolinfo) {
      qdf_err("Cannot Allocate Memory");
      return;
    }

    if (!is_already_allocated) {
        qdf_info("New Allocation %p", *dfs_mm_nolinfo);
        OS_MEMZERO(*dfs_mm_nolinfo, sizeof(struct dfsreq_nolinfo));
    }
}

void ieee80211_dfs_deliver_event(struct ieee80211com *ic, uint16_t freq, enum WLAN_DFS_EVENTS event)
{
    struct net_device *dev = NULL;
    union iwreq_data wreq;

    utils_dfs_update_chan_state_array(ic->ic_pdev_obj, freq, event);
    switch(event) {
        case WLAN_EV_RADAR_DETECTED:
            event = IEEE80211_EV_RADAR_DETECTED;
            break;
        case WLAN_EV_CAC_STARTED:
            event = IEEE80211_EV_CAC_STARTED;
            break;
        case WLAN_EV_CAC_COMPLETED:
            event = IEEE80211_EV_CAC_COMPLETED;
            break;
        case WLAN_EV_NOL_STARTED:
            event = IEEE80211_EV_NOL_STARTED;
            break;
        case WLAN_EV_NOL_FINISHED:
            event = IEEE80211_EV_NOL_FINISHED;
            break;
        case WLAN_EV_PCAC_STARTED:
            event = IEEE80211_EV_PRECAC_STARTED;
            break;
        case WLAN_EV_PCAC_COMPLETED:
            event = IEEE80211_EV_PRECAC_COMPLETED;
            break;
        case WLAN_EV_CAC_RESET:
            return;
        default:
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
                    IEEE80211_MSG_DEBUG,
                    "%s: Invalid event %u being sent\n", __func__, event);
            return;
    }

    if(!(ic->ic_osdev)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
                IEEE80211_MSG_DEBUG,
                "%s: ic->ic_osdev is NULL. No dfs %s event.\n", __func__, dfs_event(event));
        return;
    }

    dev = (struct net_device *)(ic->ic_netdev);

    if(!dev) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
                IEEE80211_MSG_DEBUG,
                "%s: dev is NULL. No dfs %s event.\n", __func__, dfs_event(event));
        return;
    }

    if (osif_scn_event_filter_status_to_app(ic, EVENT_IWEVCUSTOM, event)) {
        OS_MEMSET(&wreq, 0, sizeof(wreq));
        wreq.data.flags = event;
        wreq.data.length = sizeof(freq);

        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (char *)&freq);
        QDF_TRACE(QDF_MODULE_ID_DFS, QDF_TRACE_LEVEL_INFO,
                  "%s: dfs %s event delivered on chan freq %d.\n",
                  __func__, dfs_event(event), freq);
    } else {
        QDF_TRACE(QDF_MODULE_ID_DFS, QDF_TRACE_LEVEL_INFO,
                  "%s: dfs %s event is blocked to userspace:%d.\n",
                  __func__, dfs_event(event), freq);
    }
}
qdf_export_symbol(ieee80211_dfs_deliver_event);

/**
 * ieee80211_is_ignore_cac_events() - Ignore DFS CAC events for NOL chans
 * @ic: Pointer to struct ieee80211com
 * @event: WLAN_DFS_EVENTS
 * @freq: Frequency in MHz
 *
 * Return: True/False.
 */
static bool
ieee80211_is_ignore_cac_events(struct wlan_objmgr_pdev *pdev,
                               enum WLAN_DFS_EVENTS event,
                               qdf_freq_t freq)
{
     return ((event == WLAN_EV_CAC_STARTED ||
              event == WLAN_EV_CAC_COMPLETED) &&
              wlan_reg_is_nol_for_freq(pdev, freq));
}

void ieee80211_dfs_deliver_events(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, enum WLAN_DFS_EVENTS event)
{
    struct ath_sub_chan_freq_list chan_info;
    int i;

    chan_info.cl_nchans= 0;
    ic->ic_get_ext_chan_info(ic, chan, &chan_info);
    for (i = 0; i < chan_info.cl_nchans; i++)
    {
        enum channel_enum chan_enum;

        if (!chan_info.sub_20chan_freq[i])
            continue;

        chan_enum = wlan_reg_get_chan_enum_for_freq(chan_info.sub_20chan_freq[i]);

        /* Do not send DFS events on invalid channels */
        if (chan_enum == INVALID_CHANNEL)
            continue;

        /* In case of mixed channels (channel 36 HT160) which includes both
         * DFS and non-DFS channels, DFS events are posted for the non-DFS
         * channels 36-48 as well. Hence filter out the non-dfs channels here.
         */
        if (!wlan_reg_is_dfs_for_freq(ic->ic_pdev_obj,
                                      chan_info.sub_20chan_freq[i]))
            continue;

        /*
         * The order of NOL START and NOL Complete events are mentioned below:
         * 1) The user space "NOL" is first sent on the radar infected channel
         *    and then the channel is marked as NOL in the regulatory.
         * 2) When removing from NOL, the channel is first unmarked NOL in the
         *    regulatory and then the user space event "NOL_COMPLETED" is sent.
         */

        if (ieee80211_is_ignore_cac_events(ic->ic_pdev_obj,
                                           event,
                                           chan_info.sub_20chan_freq[i]))
            continue;

        ieee80211_dfs_deliver_event(ic, chan_info.sub_20chan_freq[i], event);
    }
}
qdf_export_symbol(ieee80211_dfs_deliver_events);

#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
/*update the bss channel info for a  vap */

static void ieee80211_update_bss_channel_for_all_vaps(struct ieee80211com *ic,
		struct ieee80211_ath_channel *channel)
{
	qdf_info("channel change is called for all vaps");

	qdf_info("Chosen Random channel:%d freq %d", channel->ic_ieee,channel->ic_freq);

	wlan_pdev_mlme_vdev_sm_chan_change(ic->ic_pdev_obj, channel);
}

static void ieee80211_regdmn_retrive_chan_params(struct ieee80211com *ic, struct ch_params *chan_params)
{

	qdf_nofl_info("%s: ic ch_width=%d, seg0=%d, seg1=%d, ch_offset=%d\n", __func__, ic->ic_tmp_ch_width,ic->ic_tmp_center_freq_seg0,ic->ic_tmp_center_freq_seg1,ic->ic_tmp_sec_ch_offset);
	chan_params->ch_width         = ic->ic_tmp_ch_width;
	chan_params->mhz_freq_seg0    = ic->ic_tmp_center_freq_seg0;
	chan_params->mhz_freq_seg1    = ic->ic_tmp_center_freq_seg1;
	chan_params->sec_ch_offset    = ic->ic_tmp_sec_ch_offset;
	qdf_nofl_info("%s: chparam ch_width=%d, seg0=%d, seg1=%d, ch_offset=%d\n", __func__, chan_params->ch_width,chan_params->center_freq_seg0,chan_params->center_freq_seg1,chan_params->sec_ch_offset);
	ic->ic_tmp_ch_width         = 0;
	ic->ic_tmp_center_freq_seg0 = 0;
	ic->ic_tmp_center_freq_seg1 = 0;
	ic->ic_tmp_sec_ch_offset    = 0;
}

void ieee80211_restart_vaps_with_non_dfs_channels(struct ieee80211com *ic,
		int no_chans_avail)
{
	struct ieee80211_ath_channel channel_obj;
	struct ieee80211_ath_channel *channel;
	uint16_t target_chan_freq = 0;
	enum ieee80211_phymode chan_mode;
	struct ch_params chan_params = {0};
	struct wlan_objmgr_pdev *pdev;

	qdf_mem_zero(&channel_obj, sizeof(struct ieee80211_ath_channel));
	channel = &channel_obj;

	/* ic_chans at this point will have only non-DFS channels
	 * If non-DFS channels are available in the current regdmn.
	 * (no_chans_avail = 0),choose a random non-DFS channel from this list.
	 */
	pdev = ic->ic_pdev_obj;
	if(pdev == NULL) {
		qdf_print("%s : pdev is null", __func__);
		return;
	}
	/* Ensure that we choose a random channel only if the curchan is DFS or is chan 0 */
	if (ic->ic_tempchan == 1 ||
			IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(ic->ic_curchan)) {
		ieee80211_regdmn_retrive_chan_params(ic, &chan_params);
		target_chan_freq = mlme_dfs_random_channel(pdev, &chan_params,
				DFS_RANDOM_CH_FLAG_NO_DFS_CH);
		if (target_chan_freq && (no_chans_avail != 1)) {
		        struct chan_params chan_p = {0};
			chan_p.freq = target_chan_freq;
			chan_p.cfreq2 = chan_params.mhz_freq_seg1;
			chan_mode = ieee80211_get_target_channel_mode(ic, &chan_params);
			*channel = ieee80211_find_dot11_channel(ic, &chan_p,
					chan_mode);
		} else {
			/* No non-DFS channels available in this regdb or random channel fails
			 * to find a chanindex.  Bring down the vaps.
			 */
			ic->no_chans_available = 1;
			ieee80211_bringdown_vaps(ic);
		}
	} else {
		qdf_nofl_info("%s:Current Channel %d freq %d is already non-DFS, not chossing another one",
				__func__,ic->ic_curchan->ic_ieee,ic->ic_curchan->ic_freq);
		*channel = *ic->ic_curchan;
	}

	/* update the bss channel of all the vaps. */
	if (IEEE80211_IS_CHAN_VALID(channel)) {
		/* Use channel reset API */
		ieee80211_update_bss_channel_for_all_vaps(ic,channel);
	}
}
qdf_export_symbol(ieee80211_restart_vaps_with_non_dfs_channels);

void ieee80211_dfs_non_dfs_chan_config(void *arg)
{
    struct ieee80211com *ic = (struct ieee80211com *)arg;
    uint8_t skip_rebuild = 0;
    int dfs_region;
    int no_chans_avail = 0;
    struct chan_params chan_p = {0};
    struct ch_params ch_params;
    struct ieee80211_ath_channel new_chan;
    enum ieee80211_phymode chan_mode;

    ieee80211_regdmn_get_dfs_region(ic->ic_pdev_obj, (enum dfs_reg *)&dfs_region);
    if (ic->dfs_spoof_test_regdmn == 1)
        if (!(mlme_dfs_is_spoof_check_failed(ic->ic_pdev_obj) && dfs_region ==
                                                         DFS_FCC_DOMAIN))
              skip_rebuild = 1;

    if (!skip_rebuild) {
        no_chans_avail = ieee80211_dfs_rebuild_chan_list_with_non_dfs_channels(ic);
        if (!no_chans_avail && ic->dfs_spoof_test_regdmn)
            ic->no_chans_available = 0;
        ieee80211_restart_vaps_with_non_dfs_channels(ic,no_chans_avail);
    }

    /* Consider a case where a regdomain has only dfs channels.
     * All the DFS channels have been radar hit and added to NOL.
     * ieee80211_dfs_action will set no_chans_available as 1 and will
     * bring down the vaps. Now, user has configured another regdomain
     * where non-DFS channels are available. AP can choose a random
     * channel from this list and come up. The following code does this.
     */
    if (ic->dfs_spoof_test_regdmn && ic->no_chans_available == 1) {
        /* Call a DFS component function to get tmp_chan_params structure.*/
        ieee80211_regdmn_get_chan_params(ic, &ch_params);
        chan_p.freq = mlme_dfs_random_channel(ic->ic_pdev_obj, &ch_params, 0);
        chan_p.cfreq2 = ch_params.mhz_freq_seg1;
        chan_mode = ieee80211_get_target_channel_mode(ic, &ch_params);
        if (chan_p.freq != 0) {
            new_chan = ieee80211_find_dot11_channel(ic,
                                                    &chan_p,
                                                    chan_mode);
            if (!IEEE80211_IS_CHAN_VALID(&new_chan)) {
                return;
            }
            ic->no_chans_available = 0;
            if (!TAILQ_EMPTY(&ic->ic_vaps)) {
                ieee80211_update_bss_channel_for_all_vaps(ic, &new_chan);
            }
        }
    }
}
#endif /* HOST_DFS_SPOOF_TEST */

#if ATH_SUPPORT_DFS && QCA_DFS_NOL_VAP_RESTART
static
void ieee80211_bringup_ap_vaps_after_nol(struct ieee80211com *ic, struct ieee80211vap *vap)
{
    uint16_t target_chan_freq = 0;
    struct ieee80211_ath_channel ptarget_channel_obj;
    struct ieee80211_ath_channel *ptarget_channel = &ptarget_channel_obj;
    struct ch_params ch_params;
    enum ieee80211_phymode chan_mode;
    struct wlan_objmgr_pdev *pdev;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null\n", __func__);
        return;
    }

    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
            QDF_STATUS_SUCCESS) {
        return;
    }
    ieee80211_regdmn_get_des_chan_params(vap, &ch_params);
    target_chan_freq = mlme_dfs_random_channel(pdev, &ch_params, 0);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);

    if (target_chan_freq) {
	struct chan_params chan_p = {0};
	chan_p.freq = target_chan_freq;
	chan_p.cfreq2 = ch_params.mhz_freq_seg1;
        chan_mode = ieee80211_get_target_channel_mode(ic,
                &ch_params);
        *ptarget_channel = ieee80211_find_dot11_channel(ic, &chan_p,
                                                       chan_mode);
        *ic->ic_curchan = *ptarget_channel;
        ic->no_chans_available = 0;
        ieee80211_bringup_ap_vaps(ic);
        ic->ic_pause_stavap_scan = 0;
    } else {
        ic->no_chans_available = 1;
    }
}

OS_TIMER_FUNC(ieee80211_dfs_nochan_vap_restart)
{
    struct ieee80211com *ic;
    struct ieee80211vap *tmpvap = NULL;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    tmpvap = TAILQ_FIRST(&ic->ic_vaps);
    if (tmpvap) {
        ieee80211_bringup_ap_vaps_after_nol(ic,tmpvap);
    }
}
#endif

void ieee80211_dfs_reset_precaclists(struct ieee80211com *ic)
{
    struct wlan_objmgr_pdev *pdev;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_err("%s : pdev is null\n", __func__);
        return;
    }

    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
            QDF_STATUS_SUCCESS) {
        return;
    }
    mlme_dfs_reset_precaclists(pdev);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
}
qdf_export_symbol(ieee80211_dfs_reset_precaclists);

#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
static void ieee80211_is_vap_in_dfs_wait(struct wlan_objmgr_pdev *pdev,
					 void *obj, void *arg)
{
    uint8_t *vap_in_cac_wait = (uint8_t *)arg;
    struct wlan_objmgr_vdev *vdev = obj;

    if (wlan_vdev_mlme_get_opmode(vdev) != QDF_SAP_MODE)
	return;

    if (wlan_vdev_mlme_get_state(vdev) ==
	WLAN_VDEV_S_DFS_CAC_WAIT) {
	*vap_in_cac_wait = true;
    }
}

void ieee80211_proc_spoof_success(struct ieee80211com *ic)
{
    struct wlan_objmgr_pdev *pdev;
    bool vap_in_cac_wait = false;

    pdev = ic->ic_pdev_obj;
    if (!pdev) {
	qdf_print("%s : pdev is null", __func__);
	return;
    }
    /* Find out if there is atleast 1 AP vap in dfs cac wait state */
    wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
				      ieee80211_is_vap_in_dfs_wait ,
				      &vap_in_cac_wait,0,
				      WLAN_MLME_SB_ID);
    /* Once spoof test is successful, if the STA vap is already connected,
     * CAC should have been completed on that channel. Therefore, bring up
     * the AP vaps UP immediately. If there is no sta VAP (non-repeater case),
     * then restart the CAC timer to compensate for the delay
     * due to Spoof testing. */
    if (vap_in_cac_wait) {
	if (ic->ic_sta_vap && wlan_is_connected(ic->ic_sta_vap)) {
	    mlme_dfs_cac_stop(pdev);
	    ieee80211_dfs_proc_cac(ic);
	} else {
	    mlme_dfs_start_cac_timer(pdev);
	}
    }
}
#endif

#if defined(WLAN_DISP_CHAN_INFO)
/**
 * ieee80211_fill_chan_param() - Fill dfs channel param such as channel
 * state, cac and nol duration.
 * @ic: Pointer to struct ieee80211com
 * @dfs_ch_param: Pointer to ieee80211_dfs_ch_params to be filled
 * @freq: Frequency in MHz
 * @chan_ieee: IEEE channel number
 * @dfs_state: Channel state
 * @is_dfs: Bool to indicate if the freq is dfs
 * @dfs_index: DFS channel index
 *
 * Return: None
 */
static void
ieee80211_fill_chan_param(struct ieee80211com *ic,
                          struct ieee80211_dfs_ch_params *dfs_ch_param,
                          qdf_freq_t freq, uint8_t chan_ieee,
                          enum wlan_channel_dfs_state dfs_state,
                          bool is_dfs, int8_t dfs_index)

{
    dfs_ch_param->op_class = wlan_reg_get_opclass_from_freq_width(NULL, freq,
                                                                  BW_20_MHZ,
                                                                  BIT(BEHAV_NONE));
    dfs_ch_param->chan_num = chan_ieee;
    dfs_ch_param->chan_state = dfs_state;
    /* For non-DFS channels, remaining CAC/NOL and CAC completion time will be 0.
     */
    if (is_dfs)
        ucfg_dfs_get_cac_nol_time(ic->ic_pdev_obj, freq, dfs_index,
                                  &dfs_ch_param->remaining_cac_time,
                                  &dfs_ch_param->cac_completion_time,
                                  &dfs_ch_param->remaining_nol_time);
}

static void
display_5ghz_chan_info(struct ieee80211req_dfs_chan_list_info *dfs_ch_list)
{
    uint8_t i;
    struct ieee80211_dfs_ch_params *dfs_ch_param;

    if (dfs_ch_list->num_chans > QDF_ARRAY_SIZE(dfs_ch_list->dfs_chan_info)) {
        qdf_err("Number of channels: %u is greater than NUM_5GHZ_CHANS, limiting it\n",
                dfs_ch_list->num_chans);
        dfs_ch_list->num_chans = QDF_ARRAY_SIZE(dfs_ch_list->dfs_chan_info);
    }

    for (i = 0; i < dfs_ch_list->num_chans; i++) {
        dfs_ch_param = &dfs_ch_list->dfs_chan_info[i];
        qdf_nofl_info("chan_num: %u, opclass: %u, state: %s, rem_cac_time: %d "
                      "cac_comp_time: %llu, rem_nol_time: %d\n",dfs_ch_param->chan_num,
                      dfs_ch_param->op_class, show_dfs_state(dfs_ch_param->chan_state),
                      dfs_ch_param->remaining_cac_time, dfs_ch_param->cac_completion_time,
                      dfs_ch_param->remaining_nol_time);
    }
}

QDF_STATUS
ieee80211_list_5ghz_channel_info(struct wlan_objmgr_pdev *pdev,
                                 struct ieee80211req_dfs_chan_list_info *dfs_ch_list)
{
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);
    struct regulatory_channel *cur_chan_list;
    uint8_t i;
    enum channel_dfs_state dfs_chan_state_arr[NUM_DFS_CHANS];
    QDF_STATUS status;
    uint8_t chan_idx = 0;

    if (!dfs_ch_list) {
        qdf_err("dfs_chan list buffer is empty");
        return QDF_STATUS_E_FAILURE;
    }

    if (!ic) {
        qdf_err("ic is NULL\n");
        return QDF_STATUS_E_FAILURE;
    }

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(struct regulatory_channel));
    if (!cur_chan_list) {
        qdf_err("Cannot alloc memory");
        return QDF_STATUS_E_NOMEM;
    }

    status = wlan_reg_get_current_chan_list(ic->ic_pdev_obj, cur_chan_list);
    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Error fetching current channel list");
        qdf_mem_free(cur_chan_list);
        return QDF_STATUS_E_FAILURE;
    }

    status = utils_dfs_get_chan_dfs_state(ic->ic_pdev_obj, dfs_chan_state_arr);
    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Error retreiving channel state");
        qdf_mem_free(cur_chan_list);
        return QDF_STATUS_E_FAILURE;
    }

    for (i = 0; i < NUM_CHANNELS; i++) {
        qdf_freq_t freq = cur_chan_list[i].center_freq;
        uint8_t chan_ieee = cur_chan_list[i].chan_num;
        enum wlan_channel_dfs_state dfs_state;
        int8_t dfs_index = 0;
        struct ieee80211_dfs_ch_params *dfs_ch_param;
        bool is_dfs = false;

        if (!WLAN_REG_IS_5GHZ_CH_FREQ(freq))
            continue;

        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
            continue;

        if (chan_idx > NUM_5GHZ_CHANS - 1)
            break;

        dfs_ch_param = &dfs_ch_list->dfs_chan_info[chan_idx];

        if (!IS_CHAN_DFS(cur_chan_list[i].chan_flags)) {
            dfs_state = WLAN_CH_DFS_S_NON_DFS;
        } else {
            is_dfs = true;
            convert_freq_to_index(freq, &dfs_index);
            if (dfs_index == INVALID_INDEX)
                continue;
            dfs_state = dfs_chan_state_arr[dfs_index];
        }
        ieee80211_fill_chan_param(ic, dfs_ch_param, freq, chan_ieee,
                                  dfs_state, is_dfs, dfs_index);
        chan_idx++;
    }

    dfs_ch_list->num_chans = chan_idx;
    display_5ghz_chan_info(dfs_ch_list);

    qdf_mem_free(cur_chan_list);

    return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(ieee80211_list_5ghz_channel_info);
#endif
