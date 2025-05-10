/*
 * Copyright (c) 2017-2023 Qualcomm Innovation Center, Inc.
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

#include <ol_if_athvar.h>
#include <init_deinit_lmac.h>
#include <cdp_txrx_ctrl.h>
#include <ieee80211_ucfg.h>
#include <ieee80211_channel.h>
#include <wlan_reg_channel_api.h>
#include "wlan_mbss.h"
#include <wlan_repeater_api.h>
#include <target_type.h>

#if UMAC_SUPPORT_CFG80211
#include <ieee80211_cfg80211.h>
#endif /* UMAC_SUPPORT_CFG80211 */

void dcs_change_channel_workq_cb(void *context);

/*
 * ol_ath_trigger_dcs:
 * Trigger DCS from within the driver (used by the UMAC handle)
 *
 * @ic          : Pointer to ic handle
 * @intf_info   : Pointer to the interference information
 * @trigger_type: Type of trigger
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_*    : Failure
 */
QDF_STATUS ol_ath_trigger_dcs(struct ieee80211com *ic,
			      void *intf_info,
			      int trigger_type)
{
	struct ol_ath_softc_net80211 *scn;
#ifdef CONFIG_AFC_SUPPORT
	struct ieee80211vap *vap;
#endif /* CONFIG_AFC_SUPPORT */

	if (!ic) {
		qdf_err("Invalid ic");
		return QDF_STATUS_E_INVAL;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("Invalid scn");
		return QDF_STATUS_E_INVAL;
	}

	if ((trigger_type < CAP_DCS_NONE) &&
	    (trigger_type >= CAP_DCS_MASK)) {
		qdf_err("Invalid trigger type");
		return QDF_STATUS_E_INVAL;
	}

	if (!OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, trigger_type)) {
		qdf_err("Trigger not enabled");
		return QDF_STATUS_E_NOSUPPORT;
	}

#ifdef CONFIG_AFC_SUPPORT
	vap = TAILQ_FIRST(&ic->ic_vaps);
	if (!vap) {
		qdf_err("No valid VAP found");
		return QDF_STATUS_E_FAILURE;
	}

        if (trigger_type == CAP_DCS_AFC) {
            if (wlan_pdev_scan_in_progress(ic->ic_pdev_obj) ||
                    wlan_mbss_acs_in_progress(vap->vdev_obj)) {
                qdf_err("VAPs are not ready or scan/ACS in progress - "
                        "triggering a timer to handle AFC after VAPs are "
                        "ready - timer: %dms",
                        DCS_AFC_TRGR_DELAY_DEFAULT_TIME);
                qdf_timer_mod((&scn->scn_dcs.dcs_afc_trgr_delay_timer),
                        DCS_AFC_TRGR_DELAY_DEFAULT_TIME);
                return QDF_STATUS_E_BUSY;
            } else if (!ieee80211_vaps_up(ic, IEEE80211_M_HOSTAP)) {
                qdf_err("rejecting AFC trigger as no vaps are up");
                return QDF_STATUS_E_INVAL;
            }
        }
#endif /* CONFIG_AFC_SUPPORT */

	return ol_ath_dcs_generic_interference_handler(scn,
						       intf_info,
						       trigger_type);
}

void ol_ath_reset_dcs_params(struct ol_ath_softc_net80211 *scn)
{
        int target_type;

	if (!scn) {
		qdf_err("scn null");
		return;
	}

        target_type = lmac_get_tgt_type(scn->soc->psoc_obj);

	scn->scn_dcs.dcs_enable          = 0;
	scn->scn_dcs.coch_intr_thresh    = DCS_COCH_INTR_THRESHOLD ;
	scn->scn_dcs.tx_err_thresh       = DCS_TXERR_THRESHOLD;
	scn->scn_dcs.phy_err_threshold   = DCS_PHYERR_THRESHOLD ;
	scn->scn_dcs.user_max_cu         = DCS_USER_MAX_CU;
	scn->scn_dcs.dcs_debug           = DCS_DEBUG_DISABLE;
	scn->scn_dcs.dcs_re_enable_time  = DCS_ENABLE_TIME;
	scn->scn_dcs.dcs_trigger_ts[0]   = 0;
	scn->scn_dcs.dcs_trigger_ts[1]   = 0;
	scn->scn_dcs.dcs_trigger_ts[2]   = 0;
	scn->scn_dcs.dcs_trigger_count   = 0;
	scn->scn_dcs.is_enable_timer_set = 0;
	qdf_timer_stop(&(scn->scn_dcs.dcs_enable_timer));

	/* Enable AWGN interference and AFC trigger management by default */
	ol_ath_ctrl_dcsawgn(&scn->sc_ic, &scn->scn_dcs.dcs_enable, true);
#ifdef CONFIG_AFC_SUPPORT
	qdf_timer_stop(&(scn->scn_dcs.dcs_afc_trgr_delay_timer));
        OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_enable, CAP_DCS_AFC);
	scn->scn_dcs.dcs_afc_chan_sel             = DCS_AFC_CHAN_SEL_ALL;
	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AFC);
#endif /* CONFIG_AFC_SUPPORT */
	OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AWGNIM);
	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_bw_reduction_ctrl, CAP_DCS_AWGNIM);

        if (target_type == TARGET_TYPE_QCN6122)
		scn->scn_dcs.dcs_adj_bw_block    = CHWIDTH_80;
}

/**
 * dcs_enable_wlan_im() - enable dcs wlan im functionality
 * @ic: ic handle
 *
 * Return: none
 */
static void dcs_enable_wlan_im(struct ieee80211com *ic)
{
	/* Enable DCS WLAN_IM functionality */
	ic->ic_enable_dcsim(ic);
}

/**
 * dcs_disable_wlan_im() - disable dcs wlan im functionality
 * @ic: ic handle
 *
 * Return: none
 */
static void dcs_disable_wlan_im(struct ieee80211com *ic)
{
	/* Disable DCS WLAN_IM functionality */
	ic->ic_disable_dcsim(ic);
}

#ifdef CONFIG_AFC_SUPPORT
/*
 * dcs_afc_trgr_delay_timer_fn:
 * Timer function to handle a delay in the AFC trigger if there are no VAPs
 * ready.
 *
 * @arg: Opaque handle to the scn pointer
 */
void dcs_afc_trgr_delay_timer_fn(void *arg)
{
	struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)arg;
	struct ieee80211com *ic = NULL;
	struct ieee80211vap *vap = NULL;
	uint32_t cur_random_chan_en = 0;

	if (!scn) {
		qdf_err("scn null");
		return;
	}

	ic = &scn->sc_ic;
	if (!ic) {
		qdf_err("ic null");
		return;
	}

	/*
	 * Check if all VAPs are ready and if scan is not in progress and if
	 * ACS is not in progress, then skip
	 */
	vap = TAILQ_FIRST(&ic->ic_vaps);
	if (!vap) {
		qdf_err("Invalid vap");
		goto exit;
	}
	if (wlan_pdev_scan_in_progress(ic->ic_pdev_obj) ||
	    wlan_mbss_acs_in_progress(vap->vdev_obj) ||
	    !ieee80211_vaps_up(ic, IEEE80211_M_HOSTAP)) {
		if ((scn->scn_dcs.dcs_afc_trgr_delay_timer_retry_cnt) >=
			DCS_AFC_TRGR_DELAY_RETRY_CNT) {
			qdf_err("Retry limit reached - Cannot trigger "
				"DCS - %d",
				scn->scn_dcs.dcs_afc_trgr_delay_timer_retry_cnt);
			goto exit;
		}

		qdf_timer_mod((&scn->scn_dcs.dcs_afc_trgr_delay_timer),
			      DCS_AFC_TRGR_DELAY_DEFAULT_TIME);
		(scn->scn_dcs.dcs_afc_trgr_delay_timer_retry_cnt)++;
		qdf_debug("VAP's not ready - retrying counter - %d",
			scn->scn_dcs.dcs_afc_trgr_delay_timer_retry_cnt);
		return;
	}

	qdf_info("AFC trigger delay timer activated after %d ms",
		 DCS_AFC_TRGR_DELAY_DEFAULT_TIME);

	/*
	 * Force full-ACS scanning and channel selection to take place to
	 * avoid race condition with start_ap() received from other VAPs.
	 * After the handling is complete, reset back to the original
	 * configuration
	 */
	cur_random_chan_en = scn->scn_dcs.dcs_random_chan_en;
	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AFC);
	ol_ath_dcs_generic_interference_handler(scn, NULL, CAP_DCS_AFC);
	OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_random_chan_en, cur_random_chan_en);

exit:
	scn->scn_dcs.dcs_afc_trgr_delay_timer_retry_cnt = 0;
	qdf_timer_stop(&scn->scn_dcs.dcs_afc_trgr_delay_timer);
	return;
}
#endif /* CONFIG_AFC_SUPPORT */

void dcs_enable_timer_fn(void *arg)
{
	struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)arg;
	struct ieee80211com *ic = NULL;

	if (!scn) {
		qdf_err("scn null");
		return;
	}

	ic = &scn->sc_ic;
	if (!ic) {
		qdf_err("ic null");
		return;
	}

	qdf_info("In DCS timer");
	scn->scn_dcs.is_enable_timer_set = 0;
	scn->scn_dcs.dcs_trigger_count = 0;
	/* Enable DCS WLAN_IM functionality */
	dcs_enable_wlan_im(ic);
	/* Deleting DCS enable timer and resetting timer flag to 0
	 * once DCS WLAN_IM is enabled
	 */
	qdf_timer_stop(&scn->scn_dcs.dcs_enable_timer);
}

void ol_ath_dcs_attach(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
	int target_type;

	if (!scn) {
		return;
	}

        target_type = lmac_get_tgt_type(scn->soc->psoc_obj);

	/* Enable AWGN interference detection by default */
	scn->scn_dcs.dcs_enable                   = 0;
	OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable);
	scn->scn_dcs.phy_err_penalty              = DCS_PHYERR_PENALTY;
	scn->scn_dcs.phy_err_threshold            = DCS_PHYERR_THRESHOLD ;
	scn->scn_dcs.radar_err_threshold          = DCS_RADARERR_THRESHOLD;
	scn->scn_dcs.coch_intr_thresh             = DCS_COCH_INTR_THRESHOLD;
	scn->scn_dcs.tx_err_thresh                = DCS_TXERR_THRESHOLD;
	scn->scn_dcs.user_max_cu                  = DCS_USER_MAX_CU;
	scn->scn_dcs.intr_detection_threshold     = DCS_INTR_DETECTION_THR;
	scn->scn_dcs.intr_detection_window        = DCS_SAMPLE_SIZE;
	scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt = 0;
	scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt = 0;
	scn->scn_dcs.dcs_debug                    = DCS_DEBUG_DISABLE;
	scn->scn_dcs.dcs_re_enable_time           = DCS_ENABLE_TIME;
	scn->scn_dcs.dcs_trigger_ts[0]            = 0;
	scn->scn_dcs.dcs_trigger_ts[1]            = 0;
	scn->scn_dcs.dcs_trigger_ts[2]            = 0;
	scn->scn_dcs.dcs_trigger_count            = 0;
	scn->scn_dcs.is_enable_timer_set          = 0;
	scn->scn_dcs.dcs_wideband_policy          = DCS_WIDEBAND_POLICY_INTRABAND;
	scn->scn_dcs.dcs_csa_tbtt                 = DCS_CSA_TBTT_DEFAULT;
	OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AWGNIM);
	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_bw_reduction_ctrl, CAP_DCS_AWGNIM);

	qdf_timer_init(ic->ic_osdev, &(scn->scn_dcs.dcs_enable_timer),
		       dcs_enable_timer_fn,
		       (void *)(scn), QDF_TIMER_TYPE_WAKE_APPS);

	/* Enable AWGN interference and AFC trigger management by default */
	ol_ath_ctrl_dcsawgn(&scn->sc_ic, &scn->scn_dcs.dcs_enable, true);
#ifdef CONFIG_AFC_SUPPORT
	scn->scn_dcs.dcs_afc_chan_sel             = DCS_AFC_CHAN_SEL_ALL;
	qdf_timer_init(ic->ic_osdev, &(scn->scn_dcs.dcs_afc_trgr_delay_timer),
		       dcs_afc_trgr_delay_timer_fn,
		       (void *)(scn), QDF_TIMER_TYPE_WAKE_APPS);
	OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_enable, CAP_DCS_AFC);
	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AFC);
#endif /* CONFIG_AFC_SUPPORT */
        if (target_type == TARGET_TYPE_QCN6122) {
		scn->scn_dcs.dcs_adj_bw_block    = CHWIDTH_80;
        }

	qdf_create_work(0, &ic->dcs_chan_change.dcs_chan_change_workq,
		        dcs_change_channel_workq_cb, ic);

	return;
}

void ol_ath_dcs_dettach(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

	if (!scn)
		return;

#ifdef CONFIG_AFC_SUPPORT
	qdf_timer_free(&(scn->scn_dcs.dcs_afc_trgr_delay_timer));
#endif /* CONFIG_AFC_SUPPORT */

	qdf_timer_free(&(scn->scn_dcs.dcs_enable_timer));

	qdf_flush_work(&ic->dcs_chan_change.dcs_chan_change_workq);
	qdf_disable_work(&ic->dcs_chan_change.dcs_chan_change_workq);

	return;
}

extern void target_if_spectral_send_intf_found_msg(struct wlan_objmgr_pdev *dev,
						   uint16_t cw_int,
						   uint32_t dcs_enabled);

#if UMAC_SUPPORT_CFG80211
/**
 * ol_ath_req_ext_dcs_trigger_cfg80211 - Request sending of DCS trigger to
 * external handler using cfg80211
 * @scn: Pointer to net80211 softc object
 * @interference_type: Interference type
 *
 * Return: 0 on success, negative error number on failure
 */
static
int ol_ath_req_ext_dcs_trigger_cfg80211(struct ol_ath_softc_net80211 *scn,
					uint32_t interference_type)
{
	struct ieee80211com *ic = NULL;
	struct ieee80211vap *vap = NULL;

	if (!scn) {
		qdf_err("scn is NULL");
		return -EINVAL;
	}

	ic = &scn->sc_ic;

	if (!OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK)) {
		qdf_err("DCS is not enabled. Ext DCS trigger not applicable");
		return -EINVAL;
	}

	qdf_info("Requesting DCS trigger to external handler using cfg80211");
	qdf_info("DCS flag = %hhu", OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK));

	/* We trigger external DCS on the first active HostAP VAP. Even if we
	 * encounter an error in trying to trigger external DCS on any of the
	 * VAPs that fulfill these criteria, we try our best to execute the
	 * trigger by trying all the VAPs until we are successful.
	 */
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
			if ((wlan_vdev_mlme_is_active(vap->vdev_obj) ==
			    QDF_STATUS_SUCCESS) &&
			    !wlan_cfg80211_do_dcs_trigger(vap,
			    interference_type)) {
				return 0;
			}
		}
	}

	return -EINVAL;
}
#endif /* UMAC_SUPPORT_CFG80211 */

#if WLAN_SPECTRAL_ENABLE
/**
 * ol_ath_req_ext_dcs_trigger_custom - Request sending of DCS trigger to
 * external handler using custom mechanism
 * @scn: Pointer to net80211 softc object
 * @interference_type: Interference type
 *
 * Request sending of DCS trigger to external handler using custom Spectral
 * based interference notification mechanism. This is for legacy support
 * purposes. This may be deprecated in the future
 *
 * Return: 0 on success, negative error number on failure
 */
static int ol_ath_req_ext_dcs_trigger_custom(struct ol_ath_softc_net80211 *scn,
					     uint32_t interference_type)
{
	struct ieee80211com *ic = NULL;
	uint32_t dcs_enabled;

	if (!scn) {
		qdf_err("scn is NULL");
		return -EINVAL;
	}

	ic = &scn->sc_ic;

	if (!OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK)) {
		qdf_err("DCS is not enabled. Ext DCS trigger not applicable");
		return -EINVAL;
	}

	qdf_info("Requesting DCS trig to ext handler using custom mechanism");
	qdf_info("DCS flag = %hhu", OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK));

	spin_lock(&ic->ic_lock);

	dcs_enabled = OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK);

	if (interference_type == CAP_DCS_CWIM) {
		target_if_spectral_send_intf_found_msg(ic->ic_pdev_obj, 1,
						       dcs_enabled);
	} else if (interference_type == CAP_DCS_WLANIM) {
		target_if_spectral_send_intf_found_msg(ic->ic_pdev_obj, 0,
						       dcs_enabled);
	} else if (interference_type == CAP_DCS_AWGNIM) {
		target_if_spectral_send_intf_found_msg(ic->ic_pdev_obj, 1,
						       dcs_enabled);
        }

	spin_unlock(&ic->ic_lock);

	return 0;
}
#else /* WLAN_SPECTRAL_ENABLE */
static int ol_ath_req_ext_dcs_trigger_custom(struct ol_ath_softc_net80211 *scn,
					     uint32_t interference_type)
{
	return -ENOSYS;
}
#endif /* WLAN_SPECTRAL_ENABLE */

#if UMAC_SUPPORT_CFG80211
int ol_ath_req_ext_dcs_trigger(struct ol_ath_softc_net80211 *scn,
			       uint32_t interference_type)
{
	if (!scn) {
		qdf_err("scn is NULL");
		return -EINVAL;
	}

	if (scn->sc_ic.ic_cfg80211_config)
		return ol_ath_req_ext_dcs_trigger_cfg80211(scn,
							   interference_type);
	else
		return ol_ath_req_ext_dcs_trigger_custom(scn,
							 interference_type);
}
#else
int ol_ath_req_ext_dcs_trigger(struct ol_ath_softc_net80211 *scn,
			       uint32_t interference_type)
{
	if (!scn) {
		qdf_err("scn is NULL");
		return -EINVAL;
	}

	return ol_ath_req_ext_dcs_trigger_custom(scn, interference_type);
}
#endif /* UMAC_SUPPORT_CFG80211 */

void wlan_dcs_im_copy_stats(wmi_host_dcs_im_tgt_stats_t *prev_stats,
			    wmi_host_dcs_im_tgt_stats_t *curr_stats)
{
	/* right now no other actions are required beyond memcopy,
	 * if required the rest of the code would follow
	 */
	qdf_mem_copy(prev_stats, curr_stats,
		     sizeof(wmi_host_dcs_im_tgt_stats_t));
}

void wlan_dcs_im_print_stats(wmi_host_dcs_im_tgt_stats_t *prev_stats,
			     wmi_host_dcs_im_tgt_stats_t *curr_stats)
{
	/* debug, dump all received stats first */
	qdf_info("tgt_curr/tsf,%u", curr_stats->reg_tsf32);
	qdf_info(",tgt_curr/last_ack_rssi,%u", curr_stats->last_ack_rssi);
	qdf_info(",tgt_curr/tx_waste_time,%u", curr_stats->tx_waste_time);
	qdf_info(",tgt_curr/dcs_rx_time,%u", curr_stats->rx_time);
	qdf_info(",tgt_curr/listen_time,%u", curr_stats->mib_stats.listen_time);
	qdf_info(",tgt_curr/tx_frame_cnt,%u",
		 curr_stats->mib_stats.reg_tx_frame_cnt);
	qdf_info(",tgt_curr/rx_frame_cnt,%u",
		 curr_stats->mib_stats.reg_rx_frame_cnt);
	qdf_info(",tgt_curr/rxclr_cnt,%u", curr_stats->mib_stats.reg_rxclr_cnt);
	qdf_info(",tgt_curr/reg_cycle_cnt,%u",
		 curr_stats->mib_stats.reg_cycle_cnt);
	qdf_info(",tgt_curr/rxclr_ext_cnt,%u",
		 curr_stats->mib_stats.reg_rxclr_ext_cnt);
	qdf_info(",tgt_curr/ofdm_phyerr_cnt,%u",
		 curr_stats->mib_stats.reg_ofdm_phyerr_cnt);
	qdf_info(",tgt_curr/cck_phyerr_cnt,%u",
		 curr_stats->mib_stats.reg_cck_phyerr_cnt);

	qdf_info("tgt_prev/tsf,%u", prev_stats->reg_tsf32);
	qdf_info(",tgt_prev/last_ack_rssi,%u", prev_stats->last_ack_rssi);
	qdf_info(",tgt_prev/tx_waste_time,%u", prev_stats->tx_waste_time);
	qdf_info(",tgt_prev/rx_time,%u", prev_stats->rx_time);
	qdf_info(",tgt_prev/listen_time,%u", prev_stats->mib_stats.listen_time);
	qdf_info(",tgt_prev/tx_frame_cnt,%u",
		 prev_stats->mib_stats.reg_tx_frame_cnt);
	qdf_info(",tgt_prev/rx_frame_cnt,%u",
		 prev_stats->mib_stats.reg_rx_frame_cnt);
	qdf_info(",tgt_prev/rxclr_cnt,%u", prev_stats->mib_stats.reg_rxclr_cnt);
	qdf_info(",tgt_prev/reg_cycle_cnt,%u",
		 prev_stats->mib_stats.reg_cycle_cnt);
	qdf_info(",tgt_prev/rxclr_ext_cnt,%u",
		 prev_stats->mib_stats.reg_rxclr_ext_cnt);
	qdf_info(",tgt_prev/ofdm_phyerr_cnt,%u",
		 prev_stats->mib_stats.reg_ofdm_phyerr_cnt);
	qdf_info(",tgt_prev/cck_phyerr_cnt,%u",
		 prev_stats->mib_stats.reg_cck_phyerr_cnt);
}

/*
 * ieee80211_are_chans_intraband - Compare if two input frequencies are present
 * in the same band.
 * @freq1 - Channel Frequency.
 * @freq2 - Channel Frequency.
 * @band - Regulatory Band.
 */
static bool ieee80211_are_chans_intraband(qdf_freq_t freq1,
                                          qdf_freq_t freq2,
					  enum reg_wifi_band band)
{
    bool is_same_band;

    switch (band) {
        case REG_BAND_6G:
            is_same_band = wlan_reg_is_6ghz_chan_freq(freq1) &&
                                wlan_reg_is_6ghz_chan_freq(freq2);
            break;
        case REG_BAND_5G:
            is_same_band = wlan_reg_is_5ghz_ch_freq(freq1) &&
                                wlan_reg_is_5ghz_ch_freq(freq2);
            break;
        case REG_BAND_2G:
            is_same_band = wlan_reg_is_24ghz_ch_freq(freq1) &&
                                wlan_reg_is_24ghz_ch_freq(freq2);
            break;
        default:
            is_same_band = false;
            break;

    }

    return  is_same_band;
}

int wlan_dcs_send_acs_request(struct ieee80211vap *vap,
                              bool allow_curchan,
                              uint32_t adj_chan_buffer)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = NULL;
    cfg80211_hostapd_acs_params cfg_acs_params = {0};
    uint32_t *freq_list = NULL;
    enum reg_wifi_band current_band;
    struct regulatory_channel *cur_chan_list;
    enum channel_enum chan_ix;
    bool is_wideband_csa_allowed;
    enum supported_6g_pwr_types ap_pwr_type;
    struct wlan_mbss_ev_data mbss_ev = {0};
    qdf_freq_t chan_freq;
    struct chan_params chan_p = {0};
    struct ieee80211_ath_channel channel;
    struct acs_random_chan_config config;



    if (!vap) {
        qdf_err("VAP NULL");
        return -EINVAL;
    }

    ic  = vap->iv_ic;
    if (!ic) {
        qdf_err("ic NULL");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn NULL");
        return -EINVAL;
    }

    cfg_acs_params.ch_list_len = 0;
    freq_list = qdf_mem_malloc(sizeof(uint32_t) * ACS_MAX_CHANNEL_COUNT);
    if (!freq_list) {
        qdf_err("Could not allocate memory for the frequency list");
        return -EINVAL;
    }

    current_band = wlan_reg_freq_to_band(ic->ic_curchan->ic_freq);
    config.adj_chan_buffer = adj_chan_buffer;

    if (check_inter_band_switch_compatibility(ic) == QDF_STATUS_SUCCESS) {
        is_wideband_csa_allowed = true;
    } else {
        is_wideband_csa_allowed = false;
    }

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list)
        return -EINVAL;

   ap_pwr_type = REG_CURRENT_PWR_MODE;
#ifdef CONFIG_AFC_SUPPORT
   if (ieee80211_is_afc_capable(ic, ieee80211_chan2mode(ic->ic_curchan))) {
       /* Always populate the LPI channel list into ACS */
       ap_pwr_type = REG_BEST_PWR_MODE;
   }
#endif /* CONFIG_AFC_SUPPORT */

   if (wlan_reg_get_current_chan_list(ic->ic_pdev_obj,
                                      cur_chan_list)) {
       qdf_err("Failed to get cur_chan list");
       qdf_mem_free(cur_chan_list);
       return -EINVAL;
    }

    for (chan_ix = 0; chan_ix < NUM_CHANNELS; chan_ix++) {
        if (!wlan_reg_is_freq_enabled(ic->ic_pdev_obj,
                                      cur_chan_list[chan_ix].center_freq,
                                      ap_pwr_type)) {
            continue;
        }

        chan_freq = cur_chan_list[chan_ix].center_freq;
        chan_p.freq   = chan_freq;
        chan_p.cfreq2 = 0;
        chan_p.input_6g_pwr_mode = ap_pwr_type;
        channel = ieee80211_find_dot11_channel(ic, &chan_p,
                                               ieee80211_chan2mode(ic->ic_curchan));
        if (!IEEE80211_IS_CHAN_VALID(&channel)) {
            continue;
        }

        /*
         * If the caller has requested that current channel be ignored then
         * all frequencies that form the current channel with the current mode
         * are ignored. Example, random channel selection for AWGN detection
         * will require the current channel to be skipped due to interference
         * detection.
         */
        if (!allow_curchan &&
            ieee80211_is_chan_overlap(&channel,
                                      ic->ic_curchan,
                                      &config) == QDF_STATUS_SUCCESS) {
            continue;
        }

        if (is_wideband_csa_allowed) {
            switch(scn->scn_dcs.dcs_wideband_policy) {
                case DCS_WIDEBAND_POLICY_INTERBAND:
                    /*
                     * If interband is selected, consider all supported
                     * channels.
                     */
                    OL_ATH_DCS_FREQ_TO_LIST(freq_list,
                                            &cfg_acs_params.ch_list_len,
                                            chan_freq);
                    break;

                case DCS_WIDEBAND_POLICY_INTRABAND:
                default:
                    /*
                     * Add only channels within current operating
                     * band if policy is set to intraband or if invalid
                     * policy is set.
                     */
         if (ieee80211_are_chans_intraband(chan_freq, ic->ic_curchan->ic_freq,
                                           REG_BAND_5G) ||
             ieee80211_are_chans_intraband(chan_freq, ic->ic_curchan->ic_freq,
                                           REG_BAND_6G)) {
                        OL_ATH_DCS_FREQ_TO_LIST(freq_list,
                                                &cfg_acs_params.ch_list_len,
                                                chan_freq);
                    }
                    break;
            }
        } else if (ieee80211_are_chans_intraband(chan_freq,
                                                 ic->ic_curchan->ic_freq,
                                                 REG_BAND_2G) ||
                   ieee80211_are_chans_intraband(chan_freq,
                                                 ic->ic_curchan->ic_freq,
                                                 REG_BAND_5G) ||
                   ieee80211_are_chans_intraband(chan_freq,
                                                 ic->ic_curchan->ic_freq,
                                                 REG_BAND_6G))  {
            /*
             * If wideband CSA is disabled, or if channel is not 11AXA, or if
             * channel is not 5/6GHz, then stay within current band.
             */
                OL_ATH_DCS_FREQ_TO_LIST(freq_list,
                                        &cfg_acs_params.ch_list_len,
                                        chan_freq);
        }

        if (cfg_acs_params.ch_list_len == (ACS_MAX_CHANNEL_COUNT - 1)) {
            /* Channel list full */
            break;
        }

    }

    qdf_mem_free(cur_chan_list);

    if (!cfg_acs_params.ch_list_len) {
        qdf_err("Could not find any channels");

        if (freq_list) {
           qdf_mem_free(freq_list);
        }

        return -EINVAL;
    }

    cfg_acs_params.hw_mode = vap->iv_des_mode;

    switch(get_chwidth_phymode(cfg_acs_params.hw_mode)) {
        case IEEE80211_CWM_WIDTH20:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_20;
            break;

        case IEEE80211_CWM_WIDTH40:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_40;
            break;

        case IEEE80211_CWM_WIDTH80:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_80;
            break;

        case IEEE80211_CWM_WIDTH160:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_160;
            break;

        case IEEE80211_CWM_WIDTH80_80:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_80P80;
            break;

#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_320;
            break;
#endif /* WLAN_FEATURE_11BE */

        default:
            cfg_acs_params.ch_width = NL80211_CHAN_WIDTH_20_NOHT;
            break;
    }

    cfg_acs_params.freq_list = (const uint32_t *)freq_list;

    mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_ACS;
    mbss_ev.ext_cb = &ieee80211_dcs_acs_event_handler;
    mbss_ev.source.acs_source = MBSS_ACS_SRC_DCS;
    mbss_ev.arg = &cfg_acs_params;

    wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

    if (freq_list) {
        qdf_mem_free(freq_list);
    }

    return 0;
}


struct ieee80211vap *dcs_get_vap(struct ieee80211com *ic)
{
    struct ieee80211vap *vap = NULL, *tmpvap = NULL;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if (tmpvap->iv_opmode != IEEE80211_M_HOSTAP)
            continue;

        if (!vap)
            vap = tmpvap;

        if ((tmpvap != vap) && (tmpvap->iv_des_mode < vap->iv_des_mode))
            vap  = tmpvap;
    }

    if (!vap)
        qdf_err("Could not find any valid VAPs - dropping interference event");

    return vap;
}

/*
 * dcs_change_channel_workq_cb:
 * Callback API to change the channel as per the flag sent by the argument.
 *
 * @context: Pointer to the new channel and DCS command
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_*    : Error
 */
void dcs_change_channel_workq_cb(void *context)
{
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;
    struct ol_ath_softc_net80211 *scn = NULL;
    struct ieee80211_dcs_cmd_sched_data *req = NULL;
    struct ieee80211_ath_channel *new_channel = NULL;
    uint32_t ch_width = 0;
    struct chan_params chan_p = {0};
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    uint8_t tbtt_value = 0;

    ic = (struct ieee80211com *)context;
    if (!ic) {
        qdf_err("null ic");
        status = QDF_STATUS_E_NULL_VALUE;
        goto exit;
    }

    req = &ic->dcs_chan_change.dcs_req;
    if (!req) {
        qdf_err("null req");
        status = QDF_STATUS_E_NULL_VALUE;
        goto exit;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("null scn");
        status = QDF_STATUS_E_NULL_VALUE;
        goto exit;
    }

    vap = dcs_get_vap(ic);
    if (!vap) {
        qdf_err("null vap");
        status = QDF_STATUS_E_NULL_VALUE;
        goto exit;
    }

    new_channel = &(req->new_channel);
    if (!IEEE80211_IS_CHAN_VALID(new_channel)) {
        qdf_err("Invalid channel");
        status = QDF_STATUS_E_NULL_VALUE;
        goto exit;
    }

    switch (get_chwidth_phymode(ieee80211_chan2mode(new_channel))) {
        case IEEE80211_CWM_WIDTH20   : ch_width = CHWIDTH_20;  break;
        case IEEE80211_CWM_WIDTH40   : ch_width = CHWIDTH_40;  break;
        case IEEE80211_CWM_WIDTH80   : ch_width = CHWIDTH_80;  break;
        case IEEE80211_CWM_WIDTH160  :
        case IEEE80211_CWM_WIDTH80_80: ch_width = CHWIDTH_160; break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320  : ch_width = CHWIDTH_320; break;
#endif /* WLAN_FEATURE_11BE */
        default                      : status = QDF_STATUS_E_INVAL; goto exit;
    }

    chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE;
    chan_p.freq = new_channel->ic_freq;
    chan_p.cfreq2 = new_channel->ic_vhtop_freq_seg2;
    set_chan_params_puncture_bitmap(&chan_p, new_channel->op_puncture_bitmap);

#ifdef CONFIG_AFC_SUPPORT
    /* Store the original desired mode to reset it during bring-up */
    if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(new_channel)) &&
        (vap->iv_des_mode != ieee80211_chan2mode(new_channel)))
        vap->iv_afc_orig_mode = vap->iv_des_mode;
#endif /* CONFIG_AFC_SUPPORT */

    switch (req->cmd_type) {
        case DCS_CHANSWITCH_CSA:
            if(req->interference_type == CAP_DCS_OBSSIM) {
                tbtt_value = IEEE80211_OBSS_CSA_TBTTCOUNT;
            } else {
                tbtt_value = scn->scn_dcs.dcs_csa_tbtt;
            }
            status = ieee80211_ucfg_set_chanswitch(vap, &chan_p,
                                                   tbtt_value,
                                                   ch_width);
        break;

        case DCS_CHANSWITCH_HARD:
            status = ieee80211_ucfg_set_freq(vap, &chan_p);
        break;
    }

exit:
    if (status == QDF_STATUS_SUCCESS) {
#ifdef CONFIG_AFC_SUPPORT
        /*
         * If the channel is 6GHz, change the AP power type based on the channel
         * that is requested.
         */
        ieee80211_change_ap_power_type(vap, new_channel);
#endif /* CONFIG_AFC_SUPPORT */
        qdf_info("DCS: interference_handling completed");
    } else {
#ifdef CONFIG_AFC_SUPPORT
        if (ic && vap && req && (req->interference_type == CAP_DCS_AFC)) {
            /* When DCS channel change is triggered from AFC,
             * ol_ath_dcs_channel_change() triggers work queue to start the
             * channel change and returns the status as success to AFC. The work
             * queue should make sure that the channel change is successful.
             */
            if (ic->dcs_chan_change.dcs_chan_change_retry_count <
                    IEEE80211_MAX_DCS_CHAN_CHANGE_RETRY) {
                ic->dcs_chan_change.dcs_chan_change_retry_count++;
                qdf_alert("DCS channel change is failed, try again");
                qdf_sched_work(NULL, &vap->iv_ic->dcs_chan_change.dcs_chan_change_workq);
                return;
            }
        }
#endif /* CONFIG_AFC_SUPPORT */
        if (ic)
            ol_ath_dcs_restore(ic);
    }
    if(ic)
       atomic_set(&ic->dcs_chan_change.dcs_in_progress, false);
}

/*
 * ol_ath_dcs_channel_change:
 * Queue a channel change request from the DCS module.
 *
 * @vap        : Pointer to the VAP structure
 * @new_channel: Pointer to the new channel to set
 * @cmd_type   : Type of channel change to trigger
 * @interference_type: Interference type
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successfully queued the message
 * QDF_STATUS_E_*    : Failed to queue a message
 */
QDF_STATUS ol_ath_dcs_channel_change(struct ieee80211vap *vap,
        struct ieee80211_ath_channel *new_channel,
        enum ieee80211_dcs_chanswitch_type cmd_type,
        enum ieee80211_cap_dcs_type interference_type)
{
    struct ieee80211_dcs_cmd_sched_data *req;

    if (!vap)
        return QDF_STATUS_E_INVAL;

    if (!IEEE80211_IS_CHAN_VALID(new_channel)) {
        return QDF_STATUS_E_INVAL;
    }

    if (IEEE80211_ARE_CHANS_EQUAL(new_channel, vap->iv_ic->ic_curchan)) {
        qdf_info("Current and new channel are the same, skipping chan change");
        ol_ath_dcs_restore(vap->iv_ic);
        return QDF_STATUS_SUCCESS;
    }

    req = &vap->iv_ic->dcs_chan_change.dcs_req;

    req->cmd_type    = cmd_type;
    req->new_channel = *new_channel;
    req->interference_type = interference_type;

    atomic_set(&vap->iv_ic->dcs_chan_change.dcs_in_progress, true);

    qdf_sched_work(NULL, &vap->iv_ic->dcs_chan_change.dcs_chan_change_workq);

    return QDF_STATUS_SUCCESS;
}

/*
 * ol_ath_dcs_check_and_reduce_bandwidth:
 * Check if channel change can be avoided by a bandwidth reduction and find
 * a new channel with the new bandwidth.
 *
 * @vap      : Pointer to the VAP structure
 * @awgn_info: Pointer to the AWGN information
 *
 * Return:
 * New channel
 */
static struct ieee80211_ath_channel
ol_ath_dcs_check_and_reduce_bandwidth(struct ieee80211vap *vap,
                                      struct wmi_host_dcs_awgn_info *awgn_info)
{
    struct ieee80211com *ic;
    enum ieee80211_cwm_width current_chan_width = IEEE80211_CWM_WIDTHINVALID;
    enum ieee80211_phymode target_mode = IEEE80211_MODE_AUTO;
    struct chan_params chan_p = {0};

    if (!vap) {
        qdf_err("vap NULL");
        return empty_channel;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic NULL");
        return empty_channel;
    }

    if (!IEEE80211_IS_CHAN_VALID(ic->ic_curchan) ||
        !IEEE80211_IS_CHAN_5GHZ_6GHZ(ic->ic_curchan)) {
        qdf_err("Current channel is invalid");
        return empty_channel;
    }

    if (!awgn_info) {
        qdf_err("Invalid AWGN information received");
        return empty_channel;
    }

    if (!awgn_info->chan_bw_intf_bitmap) {
        qdf_err("Bitmap is empty - cannot perform bandwidth reduction");
        return empty_channel;
    }

    if (OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, PRI20)) {
        /* If pri20 contains interference, then do full channel change */
        qdf_err("Primary 20MHz channel interference detected - cannot "
                "perform bandwidth reduction");
        return empty_channel;
    }

    current_chan_width = get_chwidth_phymode(ieee80211_chan2mode(ic->ic_curchan));

#ifdef WLAN_FEATURE_11BE
    if ((current_chan_width > IEEE80211_CWM_WIDTH160) &&
        !OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, SEC80) &&
        !OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, SEC40) &&
        !OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, SEC20)) {
        if (IEEE80211_IS_CHAN_11BEA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11BEA_EHT160;
        }
    } else
#endif /* WLAN_FEATURE_11BE */
    if ((current_chan_width > IEEE80211_CWM_WIDTH80) &&
        !OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, SEC40) &&
        !OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, SEC20)) {
        /* If the current channel width is greater than 80MHz, check if 80MHz
         * bandwidth reduction is possible */
#ifdef WLAN_FEATURE_11BE
        if (IEEE80211_IS_CHAN_11BEA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11BEA_EHT80;
        } else
#endif /* WLAN_FEATURE_11BE */
        if (IEEE80211_IS_CHAN_11AXA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11AXA_HE80;
        } else if (IEEE80211_IS_CHAN_11AC(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11AC_VHT80;
        }
    } else if ((current_chan_width > IEEE80211_CWM_WIDTH40) &&
               !OL_ATH_DCS_GET_BITMAP_IDX(awgn_info, SEC20)) {
        /* For bandwidth reduction to 40MHz, ignore PLUS/MINUS since 5/6GHz
         * is assumed and all channels are bonded */
#ifdef WLAN_FEATURE_11BE
        if (IEEE80211_IS_CHAN_11BEA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11BEA_EHT40;
        } else
#endif /* WLAN_FEATURE_11BE */
        if (IEEE80211_IS_CHAN_11AXA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11AXA_HE40;
        } else if (IEEE80211_IS_CHAN_11AC(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11AC_VHT40;
        } else if (IEEE80211_IS_CHAN_11NA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11NA_HT40;
        }
    } else {
#ifdef WLAN_FEATURE_11BE
        if (IEEE80211_IS_CHAN_11BEA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11BEA_EHT20;
        } else
#endif /* WLAN_FEATURE_11BE */
        if (IEEE80211_IS_CHAN_11AXA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11AXA_HE20;
        } else if (IEEE80211_IS_CHAN_11AC(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11AC_VHT20;
        } else if (IEEE80211_IS_CHAN_11NA(ic->ic_curchan)) {
            target_mode = IEEE80211_MODE_11NA_HT20;
        } else {
            target_mode = IEEE80211_MODE_11A;
        }
    }

    chan_p.freq = ic->ic_curchan->ic_freq;
    chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE;
    set_chan_params_puncture_bitmap(&chan_p, 0);
    return ieee80211_find_dot11_channel(ic, &chan_p, target_mode);
}

#ifdef CONFIG_AFC_SUPPORT
/*
 * ol_ath_dcs_validate_afc_trigger:
 * Validate if the AFC trigger can be serviced. As part of this check,
 * the difference between the current channel's regulatory power and the
 * AFC max power limit for the channel is compared again the configuration
 * flag.
 *
 * @ic: Pointer to ic
 *
 * Return:
 * QDF_STATUS_SUCCESS    : AFC trigger allowed
 * QDF_STATUS_E_FAILURE  : AFC trigger not allowed, don't change power mode
 * QDF_STATUS_E_NOSUPPORT: AFC trigger not allowed, request power mode change
 */
QDF_STATUS ol_ath_dcs_validate_afc_trigger(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn;
	int8_t afc_cur_eirp, reg_cur_eirp;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
        enum reg_6g_ap_type cur_pwr_type;
	uint16_t in_punc_pattern;

	if (!ic) {
		qdf_err("Invalid ic");
		return QDF_STATUS_E_INVAL;
	}
	scn = OL_ATH_SOFTC_NET80211(ic);

	/*
	 * If the current channel is not 6GHz, then AFC is not supported
	 */
	if (!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
		qdf_err("AFC trigger is not available for current band");
		return QDF_STATUS_E_NOSUPPORT;
	}

	in_punc_pattern = ieee80211_get_chan_punc_bitmap(ic->ic_curchan);

	/*
	 * For repeaters, skip the channel change but notify regulatory
	 * if the STA is connected. If not connected, ensure we remain in
	 * LPI
	 */
	if (ic->ic_sta_vap) {
		if (!ieee80211_vap_is_connected(ic->ic_sta_vap)) {
			if (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {
				qdf_err("Cannot service AFC DCS trigger for repeaters "
				"with disconnected STA - skipping power mode "
				"change");
				return QDF_STATUS_E_FAILURE;
			}
		} else {
			qdf_err("Cannot service AFC DCS trigger for repeaters "
				"with connected STA - attempting to promote "
				"power mode for change");
			return QDF_STATUS_E_NOSUPPORT;
		}
	}

	/*
	 * If the device is not AFC capable in the current state, then DCS
	 * for AFC cannot be supported.
	 */
	if (!ieee80211_is_afc_capable(ic,
				      ieee80211_chan2mode(ic->ic_curchan))) {
		qdf_err("Device is not capable of AFC in current state");
		return QDF_STATUS_E_NOSUPPORT;
	}

	/* Check if AFC channel selection is enabled */
	if (!scn->scn_dcs.dcs_afc_chan_sel) {
		qdf_err("DCS does not support AFC trigger");
		return QDF_STATUS_E_NOSUPPORT;
	}

	afc_cur_eirp = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj, ic->ic_curchan->ic_freq,
					     ic->ic_curchan->ic_vhtop_freq_seg2,
					     ieee80211_get_chan_width(ic->ic_curchan),
					     REG_STANDARD_POWER_AP,
					     in_punc_pattern,
					     false, REG_MAX_CLIENT_TYPE, false);
        if (wlan_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &cur_pwr_type)) {
            qdf_err("error in getting current power mode");
	    return QDF_STATUS_E_NOSUPPORT;
        }

	reg_cur_eirp = ic->ic_curchan_eirp_power;

	/*
	 * Check if the AFC channel selection is allowed for the given
	 * configuration.
	 * Compare with PSD if both current channel power and AFC power have
	 * have PSD values. If not, compare with EIRP as a fallback
	 */
	status = QDF_STATUS_E_NOSUPPORT;
	qdf_info("DCS AFC Channel Selection Configuration: "
                 "cur pwr type %d eirp: cur (%d), afc (%d)", cur_pwr_type,
		 reg_cur_eirp, afc_cur_eirp);

        if ((reg_cur_eirp == afc_cur_eirp) &&
                (scn->scn_dcs.dcs_afc_chan_sel & DCS_AFC_CHAN_SEL_CUR_PWR_EQ_AFC_PWR)) {
            status = QDF_STATUS_SUCCESS;
        } else if ((reg_cur_eirp < afc_cur_eirp) &&
                (scn->scn_dcs.dcs_afc_chan_sel & DCS_AFC_CHAN_SEL_CUR_PWR_LT_AFC_PWR)) {
            status = QDF_STATUS_SUCCESS;
        } else if ((reg_cur_eirp > afc_cur_eirp) &&
                (scn->scn_dcs.dcs_afc_chan_sel & DCS_AFC_CHAN_SEL_CUR_PWR_GT_AFC_PWR)) {
            status = QDF_STATUS_SUCCESS;
        }

	return status;
}
#endif /* CONFIG_AFC_SUPPORT */

/*
 * ol_ath_dcs_validate_awgn_info:
 * Validate AWGN information received from WMI
 *
 * @ic       : Pointer to the ic structure
 * @awgn_info: Pointer to the AWGN information
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_*    : Error
 */
static inline QDF_STATUS ol_ath_dcs_validate_awgn_info(struct ieee80211com *ic,
                                       struct wmi_host_dcs_awgn_info *awgn_info)
{
    /* If the channel in the DCS event doesn't match the current channel,
     * drop the event.
     */
    if (ic->ic_curchan->ic_freq != awgn_info->center_freq) {
        qdf_err("Current channel and WMI channel are not the same - "
                "cur_freq: %d, "
                "awgn_info_freq: %d",
                ic->ic_curchan->ic_freq,
                awgn_info->center_freq);
        return QDF_STATUS_E_INVAL;
    }

    switch(awgn_info->channel_width) {
        case WMI_HOST_CHAN_WIDTH_20:
            if (awgn_info->chan_bw_intf_bitmap > OL_ATH_DCS_SEG_PRI20) {
                return QDF_STATUS_E_INVAL;
            }
        break;

        case WMI_HOST_CHAN_WIDTH_40:
            if (awgn_info->chan_bw_intf_bitmap > OL_ATH_DCS_SEG_SEC20) {
                return QDF_STATUS_E_INVAL;
            }
        break;

        case WMI_HOST_CHAN_WIDTH_80:
            if (awgn_info->chan_bw_intf_bitmap > OL_ATH_DCS_SEG_SEC40) {
                return QDF_STATUS_E_INVAL;
            }
        break;

        case WMI_HOST_CHAN_WIDTH_160:
        case WMI_HOST_CHAN_WIDTH_80P80:
            if (awgn_info->chan_bw_intf_bitmap > OL_ATH_DCS_SEG_SEC80) {
                return QDF_STATUS_E_INVAL;
            }
        break;

        case WMI_HOST_CHAN_WIDTH_320:
            if (awgn_info->chan_bw_intf_bitmap > OL_ATH_DCS_SEG_SEC160) {
                return QDF_STATUS_E_INVAL;
            }
        break;

        default:
            return QDF_STATUS_E_INVAL;
        break;
    }

    return QDF_STATUS_SUCCESS;
}


/*
 * ol_ath_get_obssintf_puncturebitmap_80:
 * The OBSS interference bitmap sent by FW is in the order of
 * primary 20 to secondary 20 and so on. This bitmap is converted
 * into a bitmap which represents in the order of increasing
 * frequecy. This converted bitmap is returned for 80MHz mode.
 *
 * @target_mode    : current phymode
 * @pri_chan_freq  : primary channel frequency in MHz
 * @center_chan_80 : center channel frequency for primary 80MHz segment
 * @obss_intf_info: Pointer to the OBSS information
 *
 * Return:
 * Converted OBSS interference bitmap for 80 MHz
 */
uint32_t
ol_ath_get_obssintf_puncturebitmap_80(enum ieee80211_phymode target_mode,
                                     uint16_t pri_chan_freq,
                                     uint16_t center_chan_80,
                                     wmi_host_dcs_obss_intf_info *obss_intf_info)
{
    int32_t  pri_center_chan_diff = 0;
    uint32_t host_bitmap = 0;

    if (!center_chan_80) {
        qdf_err("Invalid center chan for 80MHz for mode (%d) - "
                "pri_chan_freq (%3d)",
                target_mode, pri_chan_freq);
        return 0;
    }

    pri_center_chan_diff = center_chan_80 - pri_chan_freq;
    if (pri_center_chan_diff > 0) {
        if (pri_center_chan_diff > 20) {
            host_bitmap |= (OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, PRI20))
                | (OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC20));
        } else if (pri_center_chan_diff < 20) {
            host_bitmap |= ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, PRI20)) << 1)
                | ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC20)) >> 1);
        }
        host_bitmap |= OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC40);
    } else {
        if (pri_center_chan_diff > (-20)) {
            host_bitmap |= ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, PRI20)) << 2)
                | ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC20)) << 2);
        } else if (pri_center_chan_diff < (-20)) {
            host_bitmap |= ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, PRI20)) << 3)
                | ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC20)) << 1);
        }
        host_bitmap |= (OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC40) >> 2);
    }

    return host_bitmap;
}

/*
 * ol_ath_get_obssintf_puncturebitmap_160:
 * The OBSS interference bitmap sent by FW is in the order of
 * primary 20 to secondary 20 and so on. This bitmap is converted
 * into a bitmap which represents in the order of increasing
 * frequecy. This converted bitmap is returned for 160MHz mode.
 *
 * @target_mode    : current phymode
 * @pri_chan_freq  : primary channel frequency in MHz
 * @center_chan_80 : center channel frequency for primary 80MHz segment
 * @center_chan_160: center channel frequency for primary 160MHZ segment
 * @obss_intf_info: Pointer to the OBSS information
 *
 * Return:
 * Converted OBSS interference bitmap for 160 MHz
 */
uint32_t
ol_ath_get_obssintf_puncturebitmap_160(enum ieee80211_phymode target_mode,
                                     uint16_t pri_chan_freq,
                                     uint16_t center_chan_80,
                                     uint16_t center_chan_160,
                                     wmi_host_dcs_obss_intf_info *obss_intf_info)
{
    int32_t  pri_center_chan_diff = 0;
    uint32_t host_bitmap = 0;

    host_bitmap = ol_ath_get_obssintf_puncturebitmap_80(target_mode, pri_chan_freq, center_chan_80,
                                                        obss_intf_info);
    host_bitmap &= 0xF;
    if (!center_chan_160) {
        qdf_err("Invalid center chan for 160MHz for mode (%d) - "
                "pri_chan_freq (%3d)",
                target_mode, pri_chan_freq);
        return 0;
    }

    pri_center_chan_diff = center_chan_160 - pri_chan_freq;
    if (pri_center_chan_diff > 0) {
        host_bitmap |= OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC80);
    } else {
        host_bitmap = ((host_bitmap & 0xF) << 4)
            | ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC80))>> 4);
    }

    return host_bitmap;
}

/*
 * ol_ath_get_obssintf_puncturebitmap_320:
 * The OBSS interference bitmap sent by FW is in the order of
 * primary 20 to secondary 20 and so on. This bitmap is converted
 * into a bitmap which represents in the order of increasing
 * frequecy. This converted bitmap is returned for 320MHz mode.
 *
 * @target_mode    : current phymode
 * @pri_chan_freq  : primary channel frequency in MHz
 * @center_chan_80 : center channel frequency for primary 80MHz segment
 * @center_chan_160: center channel frequency for primary 160MHZ segment
 * @center_chan_320: center channel frequency for 320MHZ segment
 * @obss_intf_info: Pointer to the OBSS information
 *
 * Return:
 * Converted OBSS interference bitmap for 320 MHz
 */
uint32_t
ol_ath_get_obssintf_puncturebitmap_320(enum ieee80211_phymode target_mode,
                                     uint16_t pri_chan_freq,
                                     uint16_t center_chan_80,
                                     uint16_t center_chan_160,
                                     uint16_t center_chan_320,
                                     wmi_host_dcs_obss_intf_info *obss_intf_info)
{
    int32_t  pri_center_chan_diff = 0;
    uint32_t host_bitmap = 0;

    host_bitmap = ol_ath_get_obssintf_puncturebitmap_160(target_mode, pri_chan_freq,
                                                         center_chan_80, center_chan_160,
                                                         obss_intf_info);
    if (!center_chan_320) {
        qdf_err("Invalid center chan for 320MHz for mode (%d) - "
                "pri_chan_freq (%3d)",
                target_mode, pri_chan_freq);
        return 0;
    }

    host_bitmap &= 0xFF;
    pri_center_chan_diff = center_chan_320 - pri_chan_freq;
    if (pri_center_chan_diff > 0) {
        host_bitmap |= OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC160);
    } else {
        host_bitmap = ((host_bitmap & 0xFF) << 8)
            | ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC160))>> 8);
    }

    return host_bitmap;
}

/*
 * ol_ath_get_obss_puncture_bitmap:
 * The OBSS interference bitmap sent by FW is in the order of
 * primary 20 to secondary 20 and so on. This bitmap is converted
 * into a bitmap which represents in the order of increasing
 * frequecy. This converted bitmap is returned.
 *
 * @vap           : Pointer to the VAP structure
 * @obss_intf_info: Pointer to the OBSS information
 *
 * Return:
 * Converted OBSS interference bitmap
 */

uint32_t ol_ath_get_obss_puncture_bitmap(struct ieee80211vap *vap,
        wmi_host_dcs_obss_intf_info *obss_intf_info)
{
    /*
     * center_freq refers to the center frequency of the primary 80MHz
     * in the case of VHT80, VHT80+80 and VHT160MHz
     */
    uint16_t center_chan_80= 0, center_chan_160 = 0;
#ifdef WLAN_FEATURE_11BE
    uint16_t center_chan_320 = 0;
#endif
    uint16_t pri_chan_freq = 0;
    enum ieee80211_phymode target_mode;
    struct ieee80211com *ic;
    uint32_t host_bitmap = 0;
    uint32_t current_channel_width = 0;

    if (!obss_intf_info) {
        qdf_err("obss_intf_info is NULL");
        return 0;
    }

    if (!vap) {
        qdf_err("vap is NULL");
        return 0;
    }

    target_mode = wlan_get_current_phymode(vap);
    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is NULL");
        return 0;
    }

    current_channel_width = obss_intf_info->channel_width;
    center_chan_80 = ic->ic_curchan->ic_vhtop_freq_seg1;
    center_chan_160 = ic->ic_curchan->ic_vhtop_freq_seg2;
    pri_chan_freq = ic->ic_curchan->ic_freq;
#ifdef WLAN_FEATURE_11BE
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,"%s: center_freq of seg1: %d, "
            "center_freq of seg2: %d, "
            "primary channel frequency: %d", __func__, center_chan_80, center_chan_160, pri_chan_freq);
#endif
    if (ieee80211_is_phymode_40minus(target_mode)) {
        host_bitmap |= ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, PRI20)) << 1)
                | ((OL_ATH_DCS_GET_BITMAP_IDX(obss_intf_info, SEC20)) >> 1);

    } else if (ieee80211_is_phymode_40plus(target_mode)) {
        host_bitmap |= obss_intf_info->chan_bw_intf_bitmap;

    } else if ((ieee80211_is_phymode_80(target_mode)) || (ieee80211_is_phymode_8080(target_mode))) {
        host_bitmap = ol_ath_get_obssintf_puncturebitmap_80(target_mode, pri_chan_freq, center_chan_80,
                                                            obss_intf_info);
        /* NOTE: Even for 80+80MHz, secondary 80MHz is not derived */

    } else if (ieee80211_is_phymode_160(target_mode)) {
        host_bitmap = ol_ath_get_obssintf_puncturebitmap_160(target_mode, pri_chan_freq, center_chan_80,
                                                            center_chan_160, obss_intf_info);
#ifdef WLAN_FEATURE_11BE
    } else if (ieee80211_is_phymode_320(target_mode)) {
        center_chan_160 = ic->ic_curchan->ic_vhtop_freq_seg1;
        center_chan_320 = ic->ic_curchan->ic_vhtop_freq_seg2;
        center_chan_80  = (pri_chan_freq > center_chan_160) ?
                               (center_chan_160 + (CHWIDTH_40)) :
                               (center_chan_160 - (CHWIDTH_40));
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT, "%s:center_freq of primary 80MHz: %d",
                          __func__, center_chan_80);
        host_bitmap = ol_ath_get_obssintf_puncturebitmap_320(target_mode, pri_chan_freq, center_chan_80,
                                                            center_chan_160, center_chan_320,
                                                            obss_intf_info);
#endif
    } else {
        host_bitmap = 0;
    }

    return host_bitmap;
}

#ifdef WLAN_FEATURE_11BE

/*
 * ol_ath_dcs_get_obss_puncture_channel:
 * Check if the puncturing pattern being applied is valid or not.
 * Apply the new bitmap over the user defined puncturing bitmap.
 * The new channel with the final bitmap is returned.
 *
 * @vap           : Pointer to the VAP structure
 * @obss_intf_info: Pointer to the OBSS information
 *
 * Return:
 * New channel
 */
static struct ieee80211_ath_channel
ol_ath_dcs_get_obss_puncture_channel(struct ieee80211vap *vap,
                                 wmi_host_dcs_obss_intf_info *obss_intf_info)
{
        struct ieee80211com *ic;
        enum ieee80211_cwm_width current_chan_width = IEEE80211_CWM_WIDTHINVALID;
        enum ieee80211_phymode target_mode = IEEE80211_MODE_AUTO;
        struct chan_params chan_p = {0};
        uint32_t final_bitmap = 0;
        uint16_t radar_punc_bitmap = 0;

        if (!vap) {
            qdf_err("vap NULL");
            return empty_channel;
        }

        ic = vap->iv_ic;
        if (!ic) {
            qdf_err("ic NULL");
            return empty_channel;
        }

        if (!IEEE80211_IS_CHAN_VALID(ic->ic_curchan) ||
            !IEEE80211_IS_CHAN_5GHZ_6GHZ(ic->ic_curchan)) {
            qdf_err("Current channel is invalid");
            return empty_channel;
        }

        if (!obss_intf_info) {
            qdf_err("Invalid OBSS information received");
            return empty_channel;
        }

        current_chan_width = get_chwidth_phymode(ieee80211_chan2mode(ic->ic_curchan));
        target_mode = wlan_get_current_phymode(vap);

        chan_p.freq = ic->ic_curchan->ic_freq;
        chan_p.cfreq2 = ic->ic_curchan->ic_vhtop_freq_seg2;
        chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE; /*set the current power mode*/
        if((ic->ic_userconfiguredchan_obj.ic_freq == ic->ic_curchan->ic_freq)
                && (ic->ic_userconfiguredchan_obj.ic_vhtop_freq_seg1 == ic->ic_curchan->ic_vhtop_freq_seg1)
                && (ic->ic_userconfiguredchan_obj.ic_vhtop_freq_seg2 == ic->ic_curchan->ic_vhtop_freq_seg2)
                && (ic->ic_userconfiguredchan_obj.ic_ieee == ic->ic_curchan->ic_ieee)) {
            final_bitmap = (ic->ic_userconfiguredchan_obj.op_puncture_bitmap)
                        | obss_intf_info->chan_bw_intf_bitmap;
        } else {
            final_bitmap = obss_intf_info->chan_bw_intf_bitmap;
        }

        /*Applying the radar punc bitmap*/
        if (ieee80211_is_chan_radar(ic, ic->ic_curchan)) {
            radar_punc_bitmap = ieee80211_get_radar_punc_bitmap(ic, ic->ic_curchan);
            final_bitmap |=  radar_punc_bitmap;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,
                    "%s:Radar punctured pattern: 0x%x",
                    __func__, radar_punc_bitmap);
        }

        set_chan_params_puncture_bitmap(&chan_p, final_bitmap);
        return ieee80211_find_dot11_channel(ic, &chan_p, target_mode);
}

static inline QDF_STATUS
ol_ath_dcs_validate_obss_intf_info(struct ieee80211vap *vap, struct ieee80211com *ic,
                                   wmi_host_dcs_obss_intf_info *obss_intf_info)
{
    /* If the channel in the DCS event doesn't match the current channel,
     * drop the event.
     */
    if(!obss_intf_info) {
        qdf_err("obss_intf_info is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (ic->ic_curchan->ic_freq != obss_intf_info->center_freq) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT, "%s:Current channel and WMI channel are not the same - "
                "cur_freq: %d, "
                "obss_intf_info_freq: %d",
                __func__,
                ic->ic_curchan->ic_freq,
                obss_intf_info->center_freq);
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}

#endif

/*
 * ol_ath_dcs_get_awgn_intf_frequencies:
 * get the awgn interference frequencies
 * from the interference bitmap.
 *
 * @vap           : Pointer to the VAP structure
 * @awgn_info     : Pointer to the AWGN information
 * @ic            : Pointer to ic structure
 * @chansel_config: Pointer to channel selection config
 *
 * Return:
 * NA
 */

void
ol_ath_dcs_get_awgn_intf_frequencies(struct ieee80211vap *vap,
                                     struct ieee80211com *ic,
                                     struct wmi_host_dcs_awgn_info *awgn_info,
                                     struct acs_random_chan_config *chansel_config)
{
        uint32_t host_awgn_bitmap = 0;
        uint32_t entrybit = 0;
        uint32_t counter = 0;
        uint32_t interference_start = 0;
        uint32_t interference_stop = 0;
        qdf_freq_t chan_low = 0,
               chan_mid_low = 0,
               chan_mid_high = 0,
               chan_high = 0;
        uint32_t index = 0;

        host_awgn_bitmap = ol_ath_get_obss_puncture_bitmap(vap, awgn_info);

        if (ieee80211_get_chan_freq_range(ic->ic_curchan, &chan_low, &chan_mid_low, &chan_mid_high, &chan_high)) {
            qdf_err("Could not get chan_freq_range for chan2");
            return;
        }

        do {
            entrybit = host_awgn_bitmap & 0x1;
            if (entrybit && !interference_start && !interference_stop) {
                interference_start = (counter*20) + chan_low - 10;
                /*chan_low gives the centre freq of channel hence -10 for correct start freq*/
            }

            if(!entrybit && interference_start && !interference_stop) {
                interference_stop = ((counter)*20) + chan_low - 10;
                /*chan_low gives the centre freq of channel hence -10 for correct start freq*/
                chansel_config->intf_freq[index].chan_low = interference_start;
                chansel_config->intf_freq[index].chan_high = interference_stop;
                qdf_debug("chansel_config->intf_freq[index].chan_low: %d", chansel_config->intf_freq[index].chan_low);
                qdf_debug("chansel_config->intf_freq[index].chan_high: %d", chansel_config->intf_freq[index].chan_high);
                index++;
                interference_start = 0;
                interference_stop = 0;
            }
            counter++;
            host_awgn_bitmap = host_awgn_bitmap >> 1;
        } while(host_awgn_bitmap && (counter < IEE80211_MAX_20M_SUB_CH));

        if (interference_start && !interference_stop) {
                interference_stop = ((counter)*20) + chan_low - 10;
                /*chan_low gives the centre freq of channel hence -10 for correct start freq*/
                chansel_config->intf_freq[index].chan_low = interference_start;
                chansel_config->intf_freq[index].chan_high = interference_stop;
                qdf_debug("chansel_config->intf_freq[index].chan_low: %d", chansel_config->intf_freq[index].chan_low);
                qdf_debug("chansel_config->intf_freq[index].chan_high: %d", chansel_config->intf_freq[index].chan_high);
                index++;
                interference_start = 0;
                interference_stop = 0;
        }
        if (index < IEE80211_MAX_20M_SUB_CH) {
            /*Set the value of array for the next index to 0 to indicate the end of the interference freq array*/
            chansel_config->intf_freq[index].chan_low = 0;
            chansel_config->intf_freq[index].chan_high = 0;
        }
}

QDF_STATUS
ol_ath_dcs_generic_interference_handler(struct ol_ath_softc_net80211 *scn,
					void *intf_info,
					enum ieee80211_cap_dcs_type interference_type)
{
	struct ieee80211vap *vap = NULL, *tmpvap = NULL;
	struct ieee80211com *ic = NULL;
	wlan_host_dcs_params_t *dcs = NULL;
	uint32_t nowms = 0;
	bool disable_dcs = false;
	int ix = 0;
        struct wmi_host_dcs_awgn_info *awgn_info = NULL;
	struct ieee80211_ath_channel new_channel = empty_channel;
        uint32_t adj_bw_block = 0;
        bool allow_curchan = true;
#if UMAC_SUPPORT_CBS
	int cbs_csa = 0;
#endif
	struct acs_random_chan_config chansel_config = {0};
#ifdef WLAN_FEATURE_11BE
	wmi_host_dcs_obss_intf_info *obss_intf_info = NULL;
	uint32_t modified_host_bitmap = 0;
#endif

	if (!scn) {
		qdf_err("scn null");
		return QDF_STATUS_E_INVAL;
	}

	ic = &scn->sc_ic;
	dcs = &scn->scn_dcs;

	if (!dcs || !ic) {
		qdf_err("dcs %pK ic %pK", dcs, ic);
		return QDF_STATUS_E_INVAL;
	}

#if UMAC_SUPPORT_CBS
	cbs_csa = ieee80211_cbs_get_param(ic->ic_cbs, IEEE80211_CBS_ENABLE);
#endif
	/* Check if CW Interference is already been found and being handled */
	if (ic->cw_inter_found) {
		return QDF_STATUS_E_BUSY;
	}

	if (atomic_read(&ic->dcs_chan_change.dcs_in_progress)) {
		return QDF_STATUS_E_BUSY;
	}

	qdf_info("DCS: inteference_handler for type %d - start", interference_type);

	spin_lock(&ic->ic_lock);

	/*
	 * mark this channel as cw_interference is found
	 * Set the CW interference flag so that ACS does not bail out this flag
	 * would be reset in ieee80211_beacon.c:ieee80211_beacon_update()
	 */
	ic->cw_inter_found = 1;
	ic->interference_type = interference_type;

	/* Before triggering the channel change, turn off the dcs until the
	 * channel change completes, to avoid repeated reports.
	 */
	if (ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_dcs, 0))
		qdf_err("set pdev param for dcs failed");

	qdf_info("DCS ch change triggered, Disabling until ch change complete");

	OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable);
	spin_unlock(&ic->ic_lock);

        if (OL_IS_DCS_ENABLED(interference_type, CAP_DCS_AWGNIM)
#ifdef CONFIG_AFC_SUPPORT
            || OL_IS_DCS_ENABLED(interference_type, CAP_DCS_AFC)
#endif /* CONFIG_AFC_SUPPORT */
            || OL_IS_DCS_ENABLED(interference_type, CAP_DCS_OBSSIM)
            ) {
            /* Get the lowest desired mode greater than 0 among all VAPs
             * Channel switch operations will be uniformly applied to all VAPs based
             * the lowest desired mode and the corresponding VAP */
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                    IEEE80211_DPRINTF(tmpvap, IEEE80211_MSG_DEBUG,
                                "%s: Checking VAP: (%s), status of the VAP: %s, desired wireless mode of VAP: %d",
                                __func__,
                                tmpvap->iv_netdev_name,
                                (wlan_vdev_mlme_is_active(tmpvap->vdev_obj)? "Not Active" : "Active"),
                                tmpvap->iv_des_mode);
                if ((tmpvap->iv_opmode != IEEE80211_M_HOSTAP)
                    || (((IEEE80211_IS_SPECIAL_VAP(tmpvap))
                    || (wlan_vdev_mlme_is_active(tmpvap->vdev_obj) != QDF_STATUS_SUCCESS))
#ifdef CONFIG_AFC_SUPPORT
                    && (interference_type != CAP_DCS_AFC)
#endif /* CONFIG_AFC_SUPPORT */
                    )) {
                        IEEE80211_DPRINTF(tmpvap, IEEE80211_MSG_DEBUG,"%s: VAP is invalid",__func__);
                    continue;
                }

                if (!vap) {
                    vap = tmpvap;
                }

                if ((tmpvap != vap) &&
                    (tmpvap->iv_des_mode < vap->iv_des_mode)) {
                    vap  = tmpvap;
                }
            }

            if (!vap) {
                qdf_err("Could not find any valid VAPs - dropping "
                        "interference event");
                ol_ath_dcs_restore(ic);
                return QDF_STATUS_E_FAILURE;
            } else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG,
                                "%s: VAP (%s) is selected to send event to userspace",
                                __func__,vap->iv_netdev_name);
            }
        }

        /*
         * For AWGN detection, skip bcast deauth and instead select a
         * random channel within the same context as the trigger event
         */
        if (interference_type == CAP_DCS_AWGNIM) {

            awgn_info = (struct wmi_host_dcs_awgn_info *)intf_info;

            IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG,
                                "%s: Mesh Agent notification support for AWGN Interference is set to %d",
                                __func__, ic->ic_awgn_event_toezmesh);
#if QCA_SUPPORT_SON
            if(ic->ic_awgn_event_toezmesh)
            {
                struct ieee80211_awgn_update_data awgn_update_event_data = {0};
                /*Fill the awgn_info from the fw handler*/
                if (awgn_info && (!(ol_ath_dcs_validate_awgn_info(ic, awgn_info)))) {
                    awgn_update_event_data.primary_freq = awgn_info->center_freq;
                    awgn_update_event_data.center_freq = awgn_info->center_freq1;
                    awgn_update_event_data.bitmap = awgn_info->chan_bw_intf_bitmap;
                } else { /*In the case of dcs-sim add primary freq as current chan freq*/
                    awgn_update_event_data.primary_freq = ic->ic_curchan->ic_freq;
                }

                awgn_update_event_data.bandwidth = get_chwidth_phymode(ieee80211_chan2mode(ic->ic_curchan));
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG,
                                "%s: Sending event to userspace for awgn detection", __func__);
                IEEE80211_DELIVER_EVENT_AWGNINTERFERENCE_UPDATE(vap, &awgn_update_event_data, sizeof(struct ieee80211_awgn_update_data));
            }
#endif

            /*
             * If there is a STA VAP present (i.e., repeater mode), then the following
             * conditions must be met to allow the AWGN event
             * (1) Repeater must be independent
             * (2) STA VAP must not be connected
             */
            if (ic->ic_sta_vap) {
                if (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) ||
                    ieee80211_vap_is_connected(ic->ic_sta_vap)) {

                    qdf_err("Invalid repeater mode for AWGN event - "
                        "is_eir: %d, "
                        "is_sta_connected: %d",
                    wlan_rptr_pdev_is_eir(ic->ic_pdev_obj),
                    ieee80211_vap_is_connected(ic->ic_sta_vap));
                    ol_ath_dcs_restore(ic);
                    return QDF_STATUS_E_INVAL;
                }
            }

            /* If the AWGN information is invalid, then move directly to
             * channel change through random channel selection or ACS/ICM/CBS.
             */
            if (awgn_info) {
                if (ol_ath_dcs_validate_awgn_info(ic, awgn_info)) {
                    qdf_err("AWGN validation failed. Dropping event");
                    ol_ath_dcs_restore(ic);
                    return QDF_STATUS_E_INVAL;
                }

                if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_bw_reduction_ctrl,
                                      CAP_DCS_AWGNIM)) {
                    new_channel = ol_ath_dcs_check_and_reduce_bandwidth(vap,
                                                                    awgn_info);
                    if (IEEE80211_IS_CHAN_VALID(&new_channel)) {
                        return ol_ath_dcs_channel_change(vap, &new_channel,
                                DCS_CHANSWITCH_CSA, interference_type);
                    }
                } else {
                    qdf_err("Bandwidth reduction control for AWGN is "
                            "disabled - performing channel change");
                }
            }

            if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AWGNIM)) {
                if (!IEEE80211_IS_CHAN_80_80MHZ(ic->ic_curchan)) {
                    /* For 80+80MHz, secondary 80MHz host bitmap is not derived
                     * Hence, the whole 80+80 is ignored while random channel selection*/
                    chansel_config.allow_awgn_interference_overlap_check = true;
                    ol_ath_dcs_get_awgn_intf_frequencies(vap, ic, awgn_info, &chansel_config);
                }
                chansel_config.allow_wideband       = dcs->dcs_wideband_policy;
                chansel_config.allow_curchan        = false;
                chansel_config.allow_max_power_only = true;
                chansel_config.freq_list            = NULL;
                chansel_config.freq_count           = 0;
                chansel_config.adj_chan_buffer       = dcs->dcs_adj_bw_block;
                new_channel = ieee80211_acs_find_random_channel(vap, &chansel_config);

                if (IEEE80211_IS_CHAN_VALID(&new_channel)) {
                    return ol_ath_dcs_channel_change(vap, &new_channel,
                            DCS_CHANSWITCH_CSA, interference_type);
                } else {
                    /* If no channels available for random channel in case of current channel with
                     * AWGN interference, according to regulatory rules vap should go down, doing this
                     * with marking channels as NO IR and send event to hostapd */
                    if (awgn_info) {
                        ic->acs_chan_awgn_info.awgn_channel_width = awgn_info->channel_width;
                        ic->acs_chan_awgn_info.awgn_center_freq = awgn_info->center_freq;
                    } else {
                        ic->acs_chan_awgn_info.awgn_channel_width = get_chwidth_phymode(ieee80211_chan2mode(ic->ic_curchan));
                        ic->acs_chan_awgn_info.awgn_center_freq  = ic->ic_curchan->ic_freq;
                    }
                    qdf_atomic_set(&ic->acs_chan_awgn_info.awgn_info, true);
                    qdf_err("No channel available in random channel selection, making vap as NO IR");
                    wlan_cfg80211_send_wiphy_regd_sync_event(ic);
                    return QDF_STATUS_SUCCESS;
                }
            }
        }

        else if (interference_type == CAP_DCS_OBSSIM) {
#ifdef WLAN_FEATURE_11BE
            if (!vap) {
                qdf_err("vap is null");
                ol_ath_dcs_restore(ic);
                return QDF_STATUS_E_NULL_VALUE;
            }
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,
                "%s:Checking vap: (%s)", __func__, vap->iv_netdev_name);
            /*
             * If there is a STA VAP present (i.e., repeater mode),
             * the OBSS event is dropped
             */
            if (ic->ic_sta_vap) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,
                "%s:Repeater mode: dropping obss interference event", __func__);
                ol_ath_dcs_restore(ic);
                return QDF_STATUS_E_INVAL;
            }

            obss_intf_info = (wmi_host_dcs_obss_intf_info *)intf_info;
            if (obss_intf_info &&
               (ol_ath_dcs_validate_obss_intf_info(vap, ic, obss_intf_info) == QDF_STATUS_SUCCESS)) {
                //Ignore the primary20 Frequency
                obss_intf_info->chan_bw_intf_bitmap &= (~OL_ATH_DCS_SEG_PRI20);

                modified_host_bitmap = ol_ath_get_obss_puncture_bitmap(vap, obss_intf_info);
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,
                        "%s:Wmi interference bitmap: 0x%x, modified bit map: 0x%x",
                        __func__, obss_intf_info->chan_bw_intf_bitmap,
                        modified_host_bitmap);
                obss_intf_info->chan_bw_intf_bitmap = modified_host_bitmap;
                new_channel = ol_ath_dcs_get_obss_puncture_channel(vap, obss_intf_info);

                if (IEEE80211_IS_CHAN_VALID(&new_channel)) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,
                                      "%s:user defined puncturing pattern: 0x%x\n"
                                      "final puncturing pattern being applied: 0x%x",
                                      __func__, ic->ic_userconfiguredchan_obj.op_puncture_bitmap,
                                      new_channel.op_puncture_bitmap);
                    return ol_ath_dcs_channel_change(vap, &new_channel,
                            DCS_CHANSWITCH_CSA, interference_type);

                } else {
                    obss_intf_info->chan_bw_intf_bitmap &= ic->ic_curchan->op_puncture_bitmap;
                    new_channel = ol_ath_dcs_get_obss_puncture_channel(vap, obss_intf_info);
                    if (IEEE80211_IS_CHAN_VALID(&new_channel)) {
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_EHT,
                                          "%s:intersected puncturing pattern: 0x%x\n"
                                          "final puncturing pattern being applied: 0x%x",
                                          __func__, obss_intf_info->chan_bw_intf_bitmap,
                                          new_channel.op_puncture_bitmap);
                        return ol_ath_dcs_channel_change(vap, &new_channel,
                                DCS_CHANSWITCH_CSA, interference_type);
                    } else {
                        ol_ath_dcs_restore(ic);
                        return QDF_STATUS_E_FAILURE;
                    }
                }

            } else {
                qdf_err("checking vap: (%s), obss_intf_info is null, continuing in the same channel",
                        vap->iv_netdev_name);
                ol_ath_dcs_restore(ic);
                return QDF_STATUS_SUCCESS;
            }
#endif
        }

#ifdef CONFIG_AFC_SUPPORT
        else if (interference_type == CAP_DCS_AFC) {
            /*
             * For AFC, check if AFC channel selection is enabled
             * and if the channel lists support ACS.
             */
            if (ol_ath_dcs_validate_afc_trigger(ic)) {
                qdf_err("AFC trigger cannot be processed");
                ol_ath_dcs_restore(ic);
                return QDF_STATUS_E_NOSUPPORT;
            }

            if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_random_chan_en, CAP_DCS_AFC)) {
                chansel_config.allow_wideband       = false;
                chansel_config.allow_curchan        = true;
                chansel_config.allow_max_power_only = true;
                chansel_config.freq_list            = NULL;
                chansel_config.freq_count           = 0;

                if (ic->ic_extacs_obj.icm_active) {
                    qdf_info("ICM is active, requesting random channel selection");
                    if (!ieee80211_vendor_acs_find_random_channel
                                               (vap, &chansel_config)) {
                        goto done;
                    }
                    ol_ath_dcs_restore(ic);
                    return QDF_STATUS_E_NOSUPPORT;
                }
                new_channel = ieee80211_acs_find_random_channel(vap,
                                                                &chansel_config);

                if (IEEE80211_IS_CHAN_VALID(&new_channel)) {
                    if (new_channel.ic_freq != ic->acs_chan_awgn_info.awgn_center_freq ||
                            get_chwidth_phymode(ieee80211_chan2mode(&new_channel)) != ic->acs_chan_awgn_info.awgn_channel_width)
                        qdf_atomic_set(&ic->acs_chan_awgn_info.awgn_info, false);
                    return ol_ath_dcs_channel_change(vap, &new_channel,
                            DCS_CHANSWITCH_CSA, interference_type);
                }
            }

#if UMAC_SUPPORT_CBS
            if (cbs_csa) {
                qdf_err("AFC trigger does not support CBS at this time");
                ol_ath_dcs_restore(ic);
                return QDF_STATUS_E_NOSUPPORT;
            }
#endif
        }
#endif /* CONFIG_AFC_SUPPORT */

#if UMAC_SUPPORT_CBS
        if (!cbs_csa) {
#endif
		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
			if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
				qdf_info("De-authenticating all the nodes ");
				qdf_info("before channel change");
				wlan_deauth_all_stas(vap);
			}
		}
#if UMAC_SUPPORT_CBS
	}
#endif

	if (ic->ic_extacs_obj.icm_active) {
		qdf_info("ICM is active, requesting external DCS trigger");

		if (ol_ath_req_ext_dcs_trigger(scn, interference_type)) {
			qdf_err("DCS: External trigger failed");
		}
	} else {
		/* Loop through and figure the first VAP on this radio */
		/* FIXME
		 * There could be some issue in mbssid mode. It does look like
		 * if wlan_set_channel fails on first vap, it tries on the
		 * second vap again. Given that all vaps on same radio, we may
		 * need to do this. Need a test case for this.
		 * Leaving the code as it is.
		 */
#if UMAC_SUPPORT_CBS
		if (!cbs_csa ||
		    (ieee80211_cbs_api_change_home_channel(ic->ic_cbs,
                                   scn->scn_dcs.dcs_wideband_policy) != EOK)) {
#endif
			TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
				if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
				    (wlan_vdev_mlme_is_active(vap->vdev_obj) ==
				    QDF_STATUS_SUCCESS)) {
					vap->channel_switch_state = 1;
				}
			}

                        allow_curchan = true;
                        adj_bw_block = 0;
                        if (interference_type == CAP_DCS_AWGNIM) {
                            allow_curchan = false;
                            adj_bw_block = scn->scn_dcs.dcs_adj_bw_block;
                        }

			TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
				if (vap->iv_opmode == IEEE80211_M_HOSTAP && !wlan_autoselect_in_progress(vap)) {
					if ((wlan_vdev_mlme_is_active(
					    vap->vdev_obj)
					    == QDF_STATUS_SUCCESS) &&
                                            !wlan_dcs_send_acs_request(vap,
                                               allow_curchan,
                                               adj_bw_block)) {
					     /* ACS is done on per radio, so
					      * calling it once is good
					      * enough
					      */
					     goto done;
					 }
				}
			}
#if UMAC_SUPPORT_CBS
		} else
			goto done;
#endif
		spin_lock(&ic->ic_lock);
		/*
		 * reset cw_interference found flag since ACS is not triggered,
		 * so it can change the channel on next CW intf detection
		 */
		ic->cw_inter_found = 0;
		ic->interference_type = CAP_DCS_NONE;

		spin_unlock(&ic->ic_lock);
	}

	if (!ic->ic_extacs_obj.icm_active) {
	    qdf_err("DCS: ACS Trigger failed");
	}

	/* Should not come here (if ICM is not active), something is not right,
	 * hope something better happens next time the flag is set
	 */

done:
	if (interference_type == CAP_DCS_WLANIM) {
		nowms = (uint32_t)
			 CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());

		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_CRITICAL))
			qdf_info("DCS: dcs trigger count %d current ts %d",
				 dcs->dcs_trigger_count, nowms);

		if (dcs->dcs_trigger_count >= (DCS_MAX_TRIGGERS - 1)) {
			if ((nowms - dcs->dcs_trigger_ts[0]) < DCS_AGING_TIME) {
				disable_dcs = true;
			} else {
				for (ix = 0; ix < (dcs->dcs_trigger_count - 1);
				     ix++) {
					dcs->dcs_trigger_ts[ix] =
						dcs->dcs_trigger_ts[ix + 1];
				}
				dcs->dcs_trigger_count--;
			}
			/* To avoid frequent channel change,if channel change is
			 * triggered three times in last 5 mins, disable dcs.
			 */
			if (unlikely
			    (scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
				qdf_info("DCS: disable %d dcs trig count %d",
					 disable_dcs, dcs->dcs_trigger_count);
				for (ix = 0; ix < dcs->dcs_trigger_count;
				     ix++) {
					qdf_info("trigger num %d, ts %d\n",
						 ix, dcs->dcs_trigger_ts[ix]);
				}
			}
		}

		if (disable_dcs) {
			dcs_disable_wlan_im(ic);
			if (!dcs->is_enable_timer_set) {
				qdf_info("DCS:reenable timer started time %d",
					 dcs->dcs_re_enable_time);
				qdf_timer_mod(&dcs->dcs_enable_timer,
					      dcs->dcs_re_enable_time * 1000);
				dcs->is_enable_timer_set = true;
				dcs->dcs_trigger_count = 0;
			}
		} else if (dcs->dcs_trigger_count < DCS_MAX_TRIGGERS) {
			dcs->dcs_trigger_ts[dcs->dcs_trigger_count] = nowms;
			dcs->dcs_trigger_count++;
		}
	}

	qdf_info("DCS: interference_handling completed");
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_awgn_interference_hanlder(struct ol_ath_softc_net80211 *scn,
                                             void *intf_info)
{
        QDF_STATUS channel_change_status = 0;
        channel_change_status = ol_ath_dcs_generic_interference_handler(scn, NULL, CAP_DCS_AWGNIM);
        if(!channel_change_status) {
                qdf_info("AWGN Interference: Channel Change is completed");
        } else {
                qdf_err("AWGN Interference: Failed to change the channel");
        }
        return QDF_STATUS_SUCCESS;
}

/*
 * ol_ath_dcs_interference_handler() - dcs interference handler
 * @sc: soc object
 * @data: data pointer
 * @datalen: len of data
 *
 * Return: none
 */
static int
ol_ath_dcs_interference_handler(ol_soc_t sc, uint8_t *data, uint32_t datalen)
{
	ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *)sc;
	struct ieee80211com *ic;
	struct ol_ath_softc_net80211 *scn;
	periodic_chan_stats_t new_stats;
	periodic_chan_stats_t *prev_stats = NULL;
	wmi_host_dcs_im_tgt_stats_t wlan_stat = {0};
	struct wmi_host_dcs_awgn_info awgn_info = {0};
	wmi_host_dcs_obss_intf_info obss_intf_info = {0};
	struct wmi_host_dcs_interference_param dcs_param = {0};
	struct wmi_unified *wmi_hdl;
	struct wlan_objmgr_pdev *pdev;
	cdp_config_param_type value = {0};

	wmi_hdl = lmac_get_wmi_hdl(soc->psoc_obj);
	if (!wmi_hdl) {
		qdf_err("wmi_handle is null");
		return -EINVAL;
	}

	/* Extract interference type */
	if (wmi_extract_dcs_interference_type(wmi_hdl, data, &dcs_param) !=
					      QDF_STATUS_SUCCESS) {
		qdf_info("Unable to extract dcs interference type");
		return -1;
	}

	/* Get pdev from pdev_id */
	pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj,
					  PDEV_UNIT(dcs_param.pdev_id),
					  WLAN_MLME_SB_ID);
	if (!pdev) {
		qdf_err("pdev object (id: %d) is NULL",
			 PDEV_UNIT(dcs_param.pdev_id));
		return -1;
	}

	scn = lmac_get_pdev_feature_ptr(pdev);
	if (!scn) {
		qdf_err("scn (id: %d) is NULL",
			 PDEV_UNIT(dcs_param.pdev_id));
		wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
		return -1;
	}

	ic = &scn->sc_ic;

	/*
	 * If none of the VAPs are ready, then ignore the DCS event since
	 * a channel is not active.
	 * NOTE: Receiving a DCS event when VAPs are not ready is incorrect
	 * behavior and needs to be investigated.
	 */
	if (!ieee80211_vaps_up(ic, IEEE80211_M_HOSTAP)) {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
		return -1;
	}

	/* This event is extended to provide periodic channel stats to user
	 * space irrespective of DCS eneble or disable.
	 * update periodic stats before handling DCS.
	 */
	if (dcs_param.interference_type == CAP_DCS_WLANIM) {
		ol_txrx_soc_handle soc_txrx_handle;
		soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
		if (!soc_txrx_handle) {
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
			return -1;
		}

		/* periodic channel stats */
		if (wmi_extract_dcs_im_tgt_stats(wmi_hdl, data, &wlan_stat) !=
						 QDF_STATUS_SUCCESS) {
			qdf_info("Unable to extract WLAN IM stats");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
			return -1;
		}

		new_stats.tx_frame_count = wlan_stat.mib_stats.reg_tx_frame_cnt;
		new_stats.rx_frame_count = wlan_stat.mib_stats.reg_rx_frame_cnt;
		new_stats.rx_clear_count = wlan_stat.mib_stats.reg_rxclr_cnt;
		new_stats.cycle_count = wlan_stat.mib_stats.reg_cycle_cnt;
		new_stats.my_bss_rx_cycle_count =
					wlan_stat.my_bss_rx_cycle_count;
		new_stats.rx_clear_ext_count =
					wlan_stat.mib_stats.reg_rxclr_ext_cnt;
		new_stats.rx_clear_ext40_count = wlan_stat.reg_rxclr_ext40_cnt;
		new_stats.rx_clear_ext80_count = wlan_stat.reg_rxclr_ext80_cnt;

		/* update noise floor information */
		scn->chan_nf = wlan_stat.chan_nf;
		value.cdp_pdev_param_chn_noise_flr = scn->chan_nf;
		cdp_txrx_set_pdev_param(soc_txrx_handle,
				wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
				CDP_CHAN_NOISE_FLOOR, value);

		prev_stats = &scn->scn_dcs.chan_stats;

		/* process channel stats first*/
		if (!wlan_pdev_scan_in_progress(ic->ic_pdev_obj)) {
			/*
			 * During scan our hardware and software counters keep
			 * incrementing although they are tracking the stats of
			 * foreign channel. Don't send periodic home channel
			 * stats while scan is in progress.
			 */
			ol_chan_stats_event(ic, prev_stats, &new_stats);
			/* Update the counter vauses with latest one */
			scn->scn_dcs.chan_stats = new_stats;
		} else {
			ol_ath_invalidate_channel_stats(ic);
		}
	} else if (dcs_param.interference_type == CAP_DCS_AWGNIM) {
		if (wmi_extract_dcs_awgn_info(wmi_hdl, data, &awgn_info)) {
			qdf_info("Unable to extract AWGN stats");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
			return -EINVAL;
		}
	}
	else if (dcs_param.interference_type == CAP_DCS_OBSSIM) {
		if (wmi_extract_dcs_obss_intf_info(wmi_hdl, data, &obss_intf_info)) {
			qdf_debug("Unable to extract OBSS stats");
			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
			return -EINVAL;
		}
	}

	/*
	 * Do not handle any thing if host is in disabled state
	 * This shall not happen, provide extra safty for against any delays
	 * causing any kind of races.
	 */
	if (!(OL_IS_DCS_RUNNING(scn->scn_dcs.dcs_enable))) {
		wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
		return 0;
	}

	switch (dcs_param.interference_type) {
	case CAP_DCS_CWIM: /* cw interferecne*/
		if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_CWIM)) {
			ol_ath_dcs_generic_interference_handler(scn,
						NULL,
						dcs_param.interference_type);
		}
	break;

	case CAP_DCS_WLANIM: /* wlan interference stats*/
		if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_WLANIM)) {
			ol_ath_wlan_interference_handler(scn, &wlan_stat,
						dcs_param.interference_type);
		}
	break;
        case CAP_DCS_AWGNIM: /* AWGN interference */
                if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_AWGNIM)) {
                        ol_ath_dcs_generic_interference_handler(scn,
						&awgn_info,
                                                dcs_param.interference_type);
                }
        break;
        case CAP_DCS_OBSSIM: /* OBSS interference */
                if (OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_OBSSIM)) {
                        ol_ath_dcs_generic_interference_handler(scn,
						&obss_intf_info,
                                                dcs_param.interference_type);
                }
        break;
	default:
		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_CRITICAL)) {
			qdf_info("DCS:unidentified interference type reported");
		}
	break;
	}
	wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
	return 0;
}

void ol_ath_disable_dcsim(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

	/* clear the run state, only when cwim is not set */
	if (!(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_CWIM)) &&
            !(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_AWGNIM))
#ifdef CONFIG_AFC_SUPPORT
	    && !(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_AFC))
#endif /* CONFIG_AFC_SUPPORT */
	    && !(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_OBSSIM))
           ) {
		OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable);
	}

	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_enable, CAP_DCS_WLANIM);

	/* send target to disable and then disable in host */
	if (ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_dcs,
				  scn->scn_dcs.dcs_enable))
		qdf_err("set pdev param disable dcsim failed");
}

void ol_ath_enable_dcsim(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

	/* Enable wlanim for DCS */
	OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_enable, CAP_DCS_WLANIM);

	QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
		       QDF_TRACE_LEVEL_INFO,
		       "DCS: state %x", scn->scn_dcs.dcs_enable);

	/* send target to enable and then enable in host */
	if (ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_dcs,
				  scn->scn_dcs.dcs_enable & CAP_DCS_MASK))
		qdf_err("set pdev param enable dcsim failed");

	(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK)) ?
		(OL_ATH_DCS_SET_RUNSTATE(scn->scn_dcs.dcs_enable)) :
		(OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable));
}

void ol_ath_ctrl_dcsawgn(struct ieee80211com *ic, uint32_t *flag, bool enable)
{
	struct ol_ath_softc_net80211 *scn = NULL;
	struct wmi_unified *wmi_hdl = NULL;

	if (!ic) {
		return;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		return;
	}

	wmi_hdl = lmac_get_wmi_hdl(scn->soc->psoc_obj);
	if (!wmi_hdl) {
		return;
	}

	if (!flag) {
		return;
	}

	*flag &= (~CAP_DCS_AWGNIM);
	if (enable &&
            wmi_service_enabled(wmi_hdl, wmi_service_dcs_awgn_int_support)) {
                *flag |= CAP_DCS_AWGNIM;
	}

	return;
}

void ol_ath_ctrl_dcsobss(struct ieee80211com *ic, uint32_t *flag, bool enable)
{
	struct ol_ath_softc_net80211 *scn = NULL;
	struct wmi_unified *wmi_hdl = NULL;

	if (!ic) {
		qdf_err("ic is null");
		return;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("scn is null");
		return;
	}

	wmi_hdl = lmac_get_wmi_hdl(scn->soc->psoc_obj);
	if (!wmi_hdl) {
		qdf_err("wmi_hdl is null");
		return;
	}

	if (!flag) {
		qdf_err("flag is NULL");
		return;
	}

	*flag &= (~CAP_DCS_OBSSIM);
	if (enable &&
            wmi_service_enabled(wmi_hdl, wmi_service_dcs_obss_int_support)) {
                *flag |= CAP_DCS_OBSSIM;
		qdf_info("OBSS interference detection is enabled");
	} else {
		qdf_info("OBSS interference detection is disabled\n"
			 "WMI OBSS int support bit set to: %d",
			 wmi_service_enabled(wmi_hdl, wmi_service_dcs_obss_int_support));
	}

	return;
}

void ol_ath_disable_dcscw(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
	uint8_t dcs_enable = scn->scn_dcs.dcs_enable;

	qdf_info("DCS: state %x", scn->scn_dcs.dcs_enable);

	if (!(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_CWIM))) {
		return;
	}

	OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_enable, CAP_DCS_CWIM);
	/* send target to disable and then disable in host */
	if (ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_dcs,
				  scn->scn_dcs.dcs_enable) != EOK) {
		OL_ATH_DCS_ENABLE(scn->scn_dcs.dcs_enable, dcs_enable);
		qdf_err("Error in disabling CWIM");
	}
}

void ol_ath_dcs_restore(struct ieee80211com *ic)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
	spin_lock(&ic->ic_lock);
	if (ic->cw_inter_found) {
		ic->cw_inter_found = 0;
		ic->interference_type = CAP_DCS_NONE;
	}

	if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE))
		qdf_info("DCS: state %x", scn->scn_dcs.dcs_enable);

	/* once the channel change is complete, turn on the dcs,
	 * use the same state as what the current enabled state of the dcs. Also
	 * set the run state accordingly.
	 */
	if (ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_dcs,
				  scn->scn_dcs.dcs_enable & CAP_DCS_MASK))
		qdf_err("set pdev param enable dcs failed");

	(OL_IS_DCS_ENABLED(scn->scn_dcs.dcs_enable, CAP_DCS_MASK)) ?
		(OL_ATH_DCS_SET_RUNSTATE(scn->scn_dcs.dcs_enable)) :
		(OL_ATH_DCS_CLR_RUNSTATE(scn->scn_dcs.dcs_enable));
	spin_unlock(&ic->ic_lock);
}

void ol_ath_soc_dcs_attach(ol_ath_soc_softc_t *soc)
{
	struct wmi_unified *wmi_handle;

	wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);

	wmi_unified_register_event_handler((wmi_unified_t)wmi_handle,
					   wmi_dcs_interference_event_id,
					   ol_ath_dcs_interference_handler,
					   WMI_RX_UMAC_CTX);
}

void ol_ath_set_dcs_param(struct ieee80211com *ic,
			  enum ol_ath_dcs_params param,
			  uint32_t value)
{
	struct ol_ath_softc_net80211 *scn;

	if (!ic) {
		qdf_err("Invalid ic");
		return;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("Invalid scn");
		return;
	}

	switch(param) {
		case OL_ATH_DCS_PARAM_RANDOM_CHAN_EN:
                        value &= CAP_DCS_MASK;
			scn->scn_dcs.dcs_random_chan_en = value;
		break;

		case OL_ATH_DCS_PARAM_CSA_TBTT:
			scn->scn_dcs.dcs_csa_tbtt = value ? value :
							   DCS_CSA_TBTT_DEFAULT;
			qdf_info("DCS CSA TBTT set as %d",
				 scn->scn_dcs.dcs_csa_tbtt);
		break;

#ifdef CONFIG_AFC_SUPPORT
		case OL_ATH_DCS_PARAM_AFC_CHAN_SEL_CONFIG:
			value &= DCS_AFC_CHAN_SEL_ALL;
			scn->scn_dcs.dcs_afc_chan_sel = value;
			qdf_info("DCS AFC channel selection: "
				 "Curchan Reg Power < Curchan AFC Power: %d, "
				 "Curchan Reg Power = Curchan AFC Power: %d, "
				 "Curchan Reg Power > Curchan AFC Power: %d",
				 !!(value & DCS_AFC_CHAN_SEL_CUR_PWR_LT_AFC_PWR),
				 !!(value & DCS_AFC_CHAN_SEL_CUR_PWR_EQ_AFC_PWR),
				 !!(value & DCS_AFC_CHAN_SEL_CUR_PWR_GT_AFC_PWR));
		break;
#endif /* CONFIG_AFC_SUPPORT */

		case OL_ATH_DCS_PARAM_BANDWIDTH_REDUCTION_CONTROL:
			value &= CAP_DCS_MASK;
			scn->scn_dcs.dcs_bw_reduction_ctrl = value;
			qdf_info("DCS bandwidth reduction set as %d",
				 scn->scn_dcs.dcs_bw_reduction_ctrl);
		break;

		default:
			qdf_info("Invalid param: %d", param);
		break;
	}

	return;
}

