/*
 * Copyright (c) 2011-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
 * All Rights Reserved.

 */

#include "ieee80211_mlme_priv.h"
#include "ieee80211_bssload.h"
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_dfs_ioctl.h>
#include <wlan_utility.h>
#include <wlan_reg_services_api.h>
#if QCN_IE
#include <ol_if_athvar.h>
#endif
#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif
#ifdef WLAN_FEATURE_11BE_MLO
#include <ieee80211_proto_mlo.h>
#include <wlan_mlo_mgr_sta.h>
#include <wlan_mlo_mgr_peer.h>
#endif

#define UNSIGNED_MAX_24BIT 0xFFFFFF

#include <wlan_repeater_api.h>
#include <wlan_mbss.h>
#include <wlan_mlo_mgr_cmn.h>
#include <wlan_mlme_if.h>
#include <ieee80211_node_priv.h>
#include <wlan_vdev_mgr_utils_api.h>
#include <ieee80211_ucfg.h>

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#endif

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlme_dispatcher.h>
#include <wlan_mlme_vdev_mgmt_ops.h>
#endif

#if QCN_IE
extern ol_ath_soc_softc_t *ol_global_soc[GLOBAL_SOC_SIZE];
extern int ol_num_global_soc;
#endif

static OS_TIMER_FUNC(ieee80211_synced_channel_switch);
static OS_TIMER_FUNC(ieee80211_synced_disconnect_sta);
int
ieee80211_recv_asresp(struct ieee80211_node *ni,
                      wbuf_t wbuf,
                      int subtype)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_frame *wh;
    u_int8_t *frm, *efrm;
    u_int16_t capinfo, associd, status;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    frm = (u_int8_t *)&wh[1];
    efrm = wbuf_header(wbuf) + wbuf_get_pktlen(wbuf);

    if (vap->iv_opmode != IEEE80211_M_STA && vap->iv_opmode != IEEE80211_M_BTAMP) {
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_rx_mgmt_discard_inc(vap->vdev_obj, 1);
#endif
        return -EINVAL;
    }

    IEEE80211_VERIFY_ADDR(ni);

    /*
     * asresp frame format
     *  [2] capability information
     *  [2] status
     *  [2] association ID
     *  [tlv] supported rates
     *  [tlv] extended supported rates
     *  [tlv] WME
     *  [tlv] HT
     *  [tlv] VHT
     *  [tlv] Bandwidth NSS mapping
     */
    IEEE80211_VERIFY_LENGTH(efrm - frm, 6);
    capinfo = le16toh(*(u_int16_t *)frm); frm += 2;
    status = le16toh(*(u_int16_t *)frm); frm += 2;
    associd = le16toh(*(u_int16_t *)frm); frm += 2;

    return ieee80211_mlme_recv_assoc_response(ni, subtype, capinfo, status,  associd, frm, efrm - frm, wbuf);
}

static bool
ieee80211_phymode_match(ieee80211_scan_entry_t scan_entry, struct ieee80211_node *ni)
{
    u_int32_t    se_phy_mode;

    se_phy_mode = wlan_util_scan_entry_phymode(ni->ni_ic, scan_entry);

    return (se_phy_mode == ni->ni_ch_param.ni_bss_phymode);
}

static bool
ieee80211_ssid_match(ieee80211_scan_entry_t scan_entry, struct ieee80211_node *ni)
{
    u_int8_t    ssid_len;
    u_int8_t    *ssid;

    /*
     * If beacon/probe response contains SSID, accept it only if it matches
     * the node's SSID;
     * If beacon/probe response has no SSID, accept it to reduce incidence of
     * timeouts.
     */
    ssid_len = util_scan_entry_ssid(scan_entry)->length;
    ssid = util_scan_entry_ssid(scan_entry)->ssid;
    if (ssid_len != 0) {
        return (OS_MEMCMP(ssid, ni->ni_essid, ssid_len) == 0);
    }

    // SSID is NULL - accept it.
    return true;
}

void ieee80211_scs_vattach(struct ieee80211vap *vap) {
   struct ieee80211com *ic = vap->iv_ic;
   osdev_t os_handle = ic->ic_osdev;
   vap->iv_cswitch_rxd = 0;

   OS_INIT_TIMER(os_handle, &vap->iv_cswitch_timer, ieee80211_synced_channel_switch, vap, QDF_TIMER_TYPE_WAKE_APPS);
   OS_INIT_TIMER(os_handle, &vap->iv_disconnect_sta_timer, ieee80211_synced_disconnect_sta, vap, QDF_TIMER_TYPE_WAKE_APPS);
}

void ieee80211_scs_vdetach(struct ieee80211vap *vap) {
   OS_FREE_TIMER(&vap->iv_cswitch_timer);
   OS_FREE_TIMER(&vap->iv_disconnect_sta_timer);
}

static OS_TIMER_FUNC(ieee80211_synced_disconnect_sta)
{
    struct ieee80211vap                          *vap = NULL;
    struct ieee80211_node                        *ni = NULL;

    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

    ni = vap->iv_ni;

    if (ni)
        ieee80211_mlme_recv_csa(ni, IEEE80211_RADAR_DETECT_DEFAULT_DELAY,true);

    OS_CANCEL_TIMER(&vap->iv_disconnect_sta_timer);
}

int ieee80211_get_num_tx_vaps_in_up_state(struct ieee80211vap *vap)
{
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    struct ieee80211vap *transmit_vap;
    uint8_t num_tx_vaps_running = 0;
    uint8_t grp_id;

    mbss_ctx = vap->iv_ic->ic_mbss_ctx;
    if (!mbss_ctx) {
        mbss_debug("mbss_ctx is null");
        return num_tx_vaps_running;
    }

    for (grp_id = 0; grp_id < mbss_ctx->max_ngroups; grp_id++) {
        ic_mbss  = &mbss_ctx->ic_mbss[grp_id];
        transmit_vap = ic_mbss->transmit_vap;

        if (transmit_vap &&
            (wlan_vdev_is_up(transmit_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
            num_tx_vaps_running++;
        }
    }

    return num_tx_vaps_running;
}

void wlan_mlme_dispatch_vdev_restart(struct wlan_objmgr_pdev *pdev,
                                    struct wlan_objmgr_vdev *vdev,
                                    struct ieee80211_ath_channel *chan)
{
    qdf_info("Dispatch VDEV restart for Vdev %d Psoc %d",
             wlan_vdev_get_id(vdev),
             wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));

    if (vdev->vdev_mlme.des_chan)
        ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan, chan);

    wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_FW_VDEV_RESTART,
                                  0, NULL);
}

/**
 * ieee80211_is_seamless_chan_change_possible() - Return true if tpe eirp ext is present
 * false otherwise.
 * @vap: Pointer to vap
 * @chan: Pointer to chan
 * @chan_switch_time: Channel switch time
 * Return : True, if seamless channel change is possible, else false.
 */
static bool ieee80211_is_seamless_chan_change_possible(struct ieee80211vap *vap,
                                                       struct ieee80211_ath_channel *chan,
                                                       uint32_t chan_switch_time)
{
    struct ieee80211com *ic = vap->iv_ic;
    bool is_cac_not_required = false, is_cac_continuable = false;
    bool has_rootap_done_cac = false;

    /*
     * The target channel is a subset of the current channel.
     * This indicates that CAC is done on the target channel.
     */
    is_cac_not_required = !mlme_dfs_is_cac_required(ic->ic_pdev_obj, chan,
                                                    ic->ic_curchan, &is_cac_continuable);
    if (is_cac_not_required)
        return true;
     /* If the Root AP has done CAC. This is indicated by the mcst */
    has_rootap_done_cac = (ic->ic_mcst_of_rootap <= chan_switch_time);

    return has_rootap_done_cac;
}

static void ieee80211_update_node_chan_params(struct ieee80211vap *vap,
                                              struct ieee80211_ath_channel *chan)
{
    struct ieee80211_node *ni = vap->iv_ni;
    struct ieee80211com *ic = vap->iv_ic;
    uint16_t chanchange_chwidth;

    switch(ic->ic_chanchange_chwidth) {
        case CHWIDTH_20:
            chanchange_chwidth = IEEE80211_CWM_WIDTH20;
            break;
        case CHWIDTH_40:
            chanchange_chwidth = IEEE80211_CWM_WIDTH40;
            break;
        case CHWIDTH_80:
            chanchange_chwidth = IEEE80211_CWM_WIDTH80;
            break;
        case CHWIDTH_160:
            chanchange_chwidth = IEEE80211_CWM_WIDTH160;
            break;
#ifdef WLAN_FEATURE_11BE
        case CHWIDTH_320:
            chanchange_chwidth = IEEE80211_CWM_WIDTH320;
            break;
#endif /* WLAN_FEATURE_11BE */
        default:
            chanchange_chwidth = ic->ic_cwm_get_width(ic);
            qdf_err("Invalid change channel width specified Using current channel width %d\n", chanchange_chwidth);
            break;
    }

    if (chanchange_chwidth != ni->ni_chwidth)
    {
        ni->ni_chwidth = chanchange_chwidth;
        /* ni_phymode is updated based on iv_cur_mode when
         * channel width is 160. Hence update iv_cur_mode
         * based on new target channel's width.
         */
        ni->ni_vap->iv_cur_mode = ieee80211_chan2mode(chan);
        ieee80211_update_phymode(ic, ni);
        /* Updated phymode is sent to FW as part of
         * ol_ath_send_peer_assoc() */
    }

    /* Update node channel setting */
    ieee80211_node_set_chan(ni);
}

static void ieee80211_perform_seamless_chan_change_per_vdev(
        struct wlan_objmgr_pdev *pdev, void *object, void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    struct ieee80211_ath_channel *chan = (struct ieee80211_ath_channel *)arg;
    struct ieee80211vap *vap = NULL;

    if (!vdev) {
        qdf_err("null vdev");
        return;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("null vap");
        return;
    }

    *vap->iv_bsschan  = *chan;
    *vap->iv_des_chan[vap->iv_des_mode] = *chan;
    vap->iv_cur_mode = ieee80211_chan2mode(chan);

    if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
        /* More than one STA vap is present in QWRAP scenario. Handle the CSA
         * completion for all the STA vdevs by calling seamless channel change.
         */
        if (wlan_vdev_is_up(vdev) != QDF_STATUS_SUCCESS)
            return;

        *vap->iv_cswitch_chan = *chan;

        ieee80211_update_node_chan_params(vap, chan);

        qdf_err("Seamless chan change triggered for vdev:%u",
                wlan_vdev_get_id(vdev));
        wlan_pdev_mlme_vdev_sm_seamless_chan_change(pdev, vdev, chan);
    } else {
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) &&
            (wlan_vdev_mlme_is_csa_restart(vdev) == QDF_STATUS_SUCCESS)) {
            /* Dispatch CSA complete event if non-tx vap is in CSA RESTART state
             */
            qdf_info("Dispatch CSA complete for Vdev %d Psoc %d",
                    wlan_vdev_get_id(vdev),
                    wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
            wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_CSA_COMPLETE, 0,
                                          NULL);
        } else {
            /* Dispatch VDEV restart event for AP vdevs in the below scenarios.
             * 1) If the AP vap is in MLO_SYNC_WAIT state.
             * 2) When the Tx vap is in MLO_SYNC_WAIT state the non-tx vaps can
             *    be in UP state. Hence, post WLAN_VDEV_SM_EV_FW_VDEV_RESTART to
             *    move the non-tx vaps from UP state to START_RESTART_PROGRESS
             *    state.
             */
            wlan_mlme_dispatch_vdev_restart(pdev, vdev, chan);
        }
    }
}

/**
 * ieee80211_perform_seamless_chan_change() - Perform seamless channel change on
 * the vap with the input channel
 * false otherwise.
 * @vap: Pointer to vap
 * @chan: Pointer to chan
 * Return : Void
 */
static void ieee80211_perform_seamless_chan_change(struct ieee80211vap *vap,
                                                   struct ieee80211_ath_channel *chan)
{
    struct ieee80211com *ic = vap->iv_ic;

    ieee80211_update_node_chan_params(vap, chan);

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        qdf_err("Seamless chan change triggered for vdev:%u",
                wlan_vdev_get_id(vap->vdev_obj));
        wlan_pdev_mlme_vdev_sm_seamless_chan_change(ic->ic_pdev_obj,
                vap->vdev_obj, chan);

        /* Handle CSA complete for other vdevs */
        wlan_objmgr_pdev_iterate_obj_list(
                        ic->ic_pdev_obj, WLAN_VDEV_OP,
                        ieee80211_perform_seamless_chan_change_per_vdev,
                        chan, 0, WLAN_MLO_MGR_ID);
    }
}

static OS_TIMER_FUNC(ieee80211_synced_channel_switch)
{
    struct ieee80211vap          *vap  = NULL;
    struct ieee80211com          *ic   = NULL;
    struct ieee80211_node        *ni   = NULL;
    struct ieee80211_ath_channel *chan = NULL;
    int err = EOK;
    uint32_t chan_switch_time = 0;
    QDF_STATUS status;

    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

    ic = vap->iv_ic;

    ni = vap->iv_ni;
    if (!ni) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                          "%s: Invalid node structure. Cannot service CSA if there is no BSS peer",
                          __func__);
        goto exit;
    }

    chan = vap->iv_cswitch_chan;

    if (chan != NULL) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                "%s: Prev chan=%u freq=%d New Chan=%u freq %d, active AP=%u rx CSA=%u, num_beacon_ap:%u\n",
                __func__,ic->ic_curchan->ic_ieee, ic->ic_curchan->ic_freq, chan->ic_ieee, chan->ic_freq,
                ieee80211_num_apvap_running(ic),
                vap->iv_cswitch_rxd, ieee80211_get_num_beaconing_vaps(ic));
    }

    /*
     * Service the CSA in this API only if the following conditions hold true:
     * (1) CSA IE was received.
     * (2) There is no running AP vap (vap not in UP state or in
     *     RESTART_PROGRESS state)
     *     If running AP VAP is present, channel switch is handled in ieee80211_beacon_update().
     * (3) If MBSSID feature is enabled and none of the Tx vaps are in UP state.
     * (4) STA is active (if STA disconnects before the timer expires, cancel the CSA.
     */

    if (vap->iv_cswitch_rxd &&
        (!ieee80211_num_apvap_running(ic) ||
         !ieee80211_get_num_beaconing_vaps(ic) ||
         (ic->is_mbssid_enabled &&
          !ieee80211_get_num_tx_vaps_in_up_state(vap))) &&
        (wlan_util_is_pdev_restart_progress(ic->ic_pdev_obj, WLAN_MLME_SB_ID) !=
         QDF_STATUS_SUCCESS) &&
        (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) &&
        (ic->ic_flags & IEEE80211_F_CHANSWITCH)) {
        /*
         * For Station, just switch channel right away.
         */
        if (!IEEE80211_IS_CHAN_SWITCH_STARTED(ic) && (chan != NULL)) {

            IEEE80211_CHAN_SWITCH_START(ic);

            if (err == EOK) {
                /* chan_switch_time does not include CAC duration */
                status = wlan_util_vdev_mgr_get_csa_channel_switch_time(vap->vdev_obj,
                        &chan_switch_time);
                if (QDF_IS_STATUS_ERROR(status)) {
                    qdf_err("Failed to get CSA channel switch time");
                    goto exit;
                }

                if (!ieee80211_is_seamless_chan_change_possible(vap, chan, chan_switch_time))
                    ieee80211_mlme_recv_csa(ni, IEEE80211_RADAR_DETECT_DEFAULT_DELAY,true);
                else
                    ieee80211_perform_seamless_chan_change(vap, chan);

                ieee80211com_clear_flags(ic, IEEE80211_F_CHANSWITCH);
                IEEE80211_CHAN_SWITCH_END(ic);
                /* Resetting max_chan_switch_time to zero */
                ic->ic_mcst_of_rootap = 0;
                /* Resetting has_rootap_done_cac to false */
                ic->ic_has_rootap_done_cac = false;

            } else if (err == EBUSY) {
                err = EOK;
            }
        } /* End of if (!IEEE80211_IS_CHAN_SWITCH_STARTED(ic) && (chan != NULL))  */
        if (err != EOK) {
            /*
             * If failed to switch the channel, mark the AP as radar detected and disconnect from the AP.
             */
            ieee80211_mlme_recv_csa(ni, IEEE80211_RADAR_DETECT_DEFAULT_DELAY, true);
        }

    }

exit:
    vap->iv_cswitch_rxd = 0;
}

#ifdef QCA_PROCESS_UPLINK_CSA
/**
 * ieee80211_is_uplink_csa_allowed() - Check if uplink CSA is received and
 * processed in the Root AP.
 * An uplink CSA is not as per 802.11 spec. But it is an implementation based on
 * customer requirement. An uplink CSA is received as an action frame in the
 * Root AP(radio without any STA vap). It is processed when,
 * i) the configuration to process uplink csa is enabled.
 * ii) the CSA is received by a Root AP from a connected STA vap.
 * iii) the radio does not contain any STA vap.
 *
 * Return: True, if the uplink CSA has to be processed in Root AP; False,
 * otherwise.
 */
static inline bool
ieee80211_is_uplink_csa_allowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;

    if ((ic->ic_process_uplink_csa) && (vap->iv_opmode == IEEE80211_M_HOSTAP) &&
        (!ic->ic_sta_vap))
        return true;

    return false;

}
#else
static inline bool
ieee80211_is_uplink_csa_allowed(struct ieee80211vap *vap)
{
    return false;
}
#endif

void ieee80211_process_dynamic_chan_change(struct ieee80211_node *ni,
                struct ieee80211_ath_channel *chan,
                uint8_t tbtt,
                uint8_t *update_beacon)
{
        struct ieee80211com *ic = ni->ni_ic;
        struct ieee80211vap *vap = ni->ni_vap;
        struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
        struct ieee80211_quiet_param quiet_ic = {0};

        if (pdev == NULL) {
                qdf_print("%s : pdev is null", __func__);
                return;
        }

        ic->ic_curchan->ic_flagext |= IEEE80211_CHAN_CSA_RECEIVED;

        if (((vap->iv_opmode == IEEE80211_M_STA) && !(ic->ic_flags & IEEE80211_F_CHANSWITCH)) ||
            (ieee80211_is_uplink_csa_allowed(vap))) {

                ieee80211_dfs_cancel_waitfor_csa_timer(ic);
                /*
                 * This VAP is in WDS mode and the received beacon is from
                 * our AP which contains a CSA.
                 */
                ic->ic_chanchange_params.freq = chan->ic_freq;
                ic->ic_chanchange_params.cfreq2 = chan->ic_vhtop_freq_seg2;
                set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, chan->op_puncture_bitmap);

                if (tbtt)
                    ic->ic_chanchange_tbtt = tbtt;
                else
                    ic->ic_chanchange_tbtt = IEEE80211_DEFAULT_CHANSWITCH_COUNT;

                *vap->iv_bsschan  = *chan;
                *vap->iv_cswitch_chan = *chan;

                /* Override the iv_bsschan and ic_chanchange_chan
                 * if ignore CSA_DFS is set
                 */

                if (IEEE80211_IS_CSH_IGNORE_CSA_DFS_ENABLED(ic) &&
                                IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan)) {
                        struct chan_params chan_p = {0};
                        struct ieee80211_ath_channel ptarget_channel_obj;
                        struct ieee80211_ath_channel *ptarget_channel = &ptarget_channel_obj;
                        struct ch_params ch_params = {0};
                        enum ieee80211_phymode chan_mode;

                        ieee80211_regdmn_get_chan_params(ic, &ch_params);

                        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                                        QDF_STATUS_SUCCESS) {
                                return;
                        }
                        chan_p.freq = mlme_dfs_random_channel(
                                        pdev, &ch_params, DFS_RANDOM_CH_FLAG_NO_DFS_CH);
                        if(!chan_p.freq) {
                                chan_p.freq = mlme_dfs_random_channel(pdev,
                                                &ch_params, 0);
                        }
                        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);

                        if (chan_p.freq) {
                                chan_mode = ieee80211_get_target_channel_mode(ic,
                                                &ch_params);
                                chan_p.cfreq2 = ch_params.mhz_freq_seg1;
                                *ptarget_channel = ieee80211_find_dot11_channel(ic,
                                                                                &chan_p,
                                                                                chan_mode);
                                if (IEEE80211_IS_CHAN_VALID(ptarget_channel)) {
                                        *chan = *ptarget_channel;
                                        *vap->iv_bsschan = *ptarget_channel;
                                        ic->ic_chanchange_params.freq = ptarget_channel->ic_freq;
                                        ic->ic_chanchange_params.cfreq2 = ptarget_channel->ic_vhtop_freq_seg2;
                                        set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, 0);
                                        *vap->iv_cswitch_chan = *chan;
                                }
                        } else {
                                ic->no_chans_available = 1;
                                qdf_print("%s: vap-%d(%s) channel is not available, bringdown all the AP vaps",
                                                __func__,vap->iv_unit,vap->iv_netdev_name);
                                wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_PDEV_STOP);
                        }
                }
                *ic->ic_chanchange_channel = *chan;
                ic->ic_chanchange_chwidth = ieee80211_get_chan_width(chan);
                ic->ic_chanchange_secoffset = ieee80211_sec_chan_offset(ic, chan);
                ic->ic_chanchange_chanflag = chan->ic_flags;
                /* Update node channel setting */
                ieee80211_node_set_chan(ni);
                qdf_info("vap-%u(%s) Num ap vaps running :%u, Num beaconning vaps: %u is_mbssid_enabled:%d num Tx vaps in up state:%d",
                         vap->iv_unit, vap->iv_netdev_name,
                         ieee80211_num_apvap_running(ic),
                         ieee80211_get_num_beaconing_vaps(ic),
                         ic->is_mbssid_enabled,
                         ieee80211_get_num_tx_vaps_in_up_state(vap));

                if (ieee80211_get_num_beaconing_vaps(ic) &&
                    ieee80211_num_apvap_running(ic) &&
                    (!ic->is_mbssid_enabled ||
                     ieee80211_get_num_tx_vaps_in_up_state(vap))) {
                    wlan_pdev_mlme_vdev_sm_csa_restart(pdev, chan);
                    /* update beacon only if AP vdev is running */
                    if (update_beacon)
                        *update_beacon = 1;
                }
                /* The IEEE80211_F_CHANSWITCH flag in ic will be cleared by
                 * the VAP that changes channel. Set it unconditionally.
                 */
                ic->ic_radio_set_bcn_offload_quiet_mode(ic, &quiet_ic);
                ieee80211com_set_flags(ic, IEEE80211_F_CHANSWITCH);
        }

        vap->iv_cswitch_rxd = 1;
        ni->ni_chanswitch_tbtt = tbtt;
        vap->iv_ni = ni;
        vap->channel_switch_state = 1; /* channel switch in progress */
        OS_SET_TIMER(&vap->iv_cswitch_timer,IEEE80211_TU_TO_MS(ni->ni_chanswitch_tbtt*ni->ni_intval));
}

void ieee80211_process_beacon_op_channel_change(struct ieee80211_node *ni,
                struct ieee80211_ath_channel *chan,
                uint8_t *update_beacon)
{
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_MLME, "beacon channel change detected !");

        if (!(ni->ni_ic->ic_flags & IEEE80211_F_CHANSWITCH)) {
            ni->ni_vap->cfg80211_channel_started_count = IEEE80211_DEFAULT_CHANSWITCH_COUNT;
            /* Notify the applications that CSA channel change is started */
            IEEE80211_DELIVER_EVENT_CHANNEL_SWITCH_STARTED(ni->ni_vap, chan);
        }

        ieee80211_process_dynamic_chan_change(ni, chan, 0, update_beacon);
}

void ieee80211_process_csa(struct ieee80211_node *ni,
                struct ieee80211_ath_channel *chan,
                struct ieee80211_ath_channelswitch_ie *chanie,
                struct ieee80211_extendedchannelswitch_ie *echanie, u_int8_t *update_beacon)
{
        uint8_t recv_tbtt;

        if (!chanie && !echanie) {
            qdf_err("chanie and echanie is NULL");
            return;
        }

        recv_tbtt = (chanie != NULL) ? chanie->tbttcount : echanie->tbttcount;

        ieee80211_process_dynamic_chan_change(ni, chan, recv_tbtt, update_beacon);
}

/**
 * find_min_val_in_arr() - Find the minimum value in a given integer array.
 *
 * @min: Pointer to the output minimum value.
 * @arr: Input integer array.
 * @size: Size of the input integer array.
 * Return: void.
 */
static void find_min_val_in_arr(int8_t *min, int8_t *arr, uint8_t size)
{
    int i;

    *min = arr[0];

    for (i = 0; i < size; ++i)
        if (*min < arr[i])
            *min = arr[i];

}

#define MIN_6G_CHANWIDTH 20
int8_t
iee80211_tpe_log2(uint8_t value)
{
    int8_t log_out;

    switch (value) {
        case 1:
            log_out = 0;
            break;
        case 2:
            log_out = 1;
            break;
        case 4:
            log_out = 2;
            break;
        case 8:
            log_out = 3;
            break;
        case 16:
            log_out = 4;
            break;
        default:
            log_out = -1;
    }
    return log_out;
}

/**
 * ieee80211_scale_down_power() - The power received in beacon is in 0.5dBm units.
 * Scale down the power to 1dBm scale before processing it.
 *
 * @arr: The input array which contains power values in 0.5dBm, is converted to
 * the output array which contains power value in 1dBm scale.
 * @num: Number of elements of the array to be converted.
 * Return: void.
 */
static void
ieee80211_scale_down_power(int8_t *arr, uint8_t num)
{
    int i;

    for (i = 0; i < num; i++) {
        arr[i] /= 2;
    }
}

/**
 * ieee80211_get_num_ext_elements() - Get number of TPE EXT elements
 * @tpe: Pointer to struct ieee80211_ie_vht_txpwr_env
 * @tpe_count: 11ax tpe count
 *
 * Return: number of power values in TPE extension
 */
#ifdef WLAN_FEATURE_11BE
static uint16_t
ieee80211_get_num_ext_elements(struct ieee80211_ie_vht_txpwr_env *tpe,
                               uint8_t tpe_count, bool is_psd)
{
    uint8_t tpe_info_len = sizeof(uint8_t);
    uint8_t tpe_ext_len;
    struct ieee80211_tpe_ext_psd_payload *tpe_ext_psd;
    uint8_t tpe_legacy_txpow_len = sizeof(tpe->tpe_payload.local_max_txpwr[0]) * tpe_count;
    uint8_t he_tpe_info_len = tpe_legacy_txpow_len + tpe_info_len;

    tpe_ext_len = qdf_min(tpe->elem_len - he_tpe_info_len, IEEE80211_TPE_NUM_MAX_EXT_PSD_VALS);
    if (is_psd && tpe_ext_len > 0) {
        tpe_ext_psd = (struct ieee80211_tpe_ext_psd_payload *)((uint8_t *)tpe + IEEE80211_IE_HDR_LEN + he_tpe_info_len);

        return tpe_ext_psd->ext_count;
    }

    return tpe_ext_len;

}
#else
static uint16_t
ieee80211_get_num_ext_elements(struct ieee80211_ie_vht_txpwr_env *tpe,
                               uint8_t tpe_count, bool is_psd)
{
    return 0;
}
#endif

/**
 * ieee80211_compute_ni_chan() - Compute ni_chan from ni_ch_params
 * @ni: Pointer to struct ieee80211_node
 * @ni_chan: Pointer to struct ieee80211_ath_channel
 *
 * Return: None
 */
static void
ieee80211_compute_ni_chan(struct ieee80211_node *ni,
                          struct ieee80211_ath_channel *ni_chan)
{
    struct chan_params chan_p = {0};

    chan_p.freq = ni->ni_ch_param.ni_freq;
    chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
    set_chan_params_puncture_bitmap(&chan_p, ni->ni_ch_param.puncture_bitmap);

    *ni_chan = ieee80211_find_dot11_channel(ni->ni_ic, &chan_p,
                                            ni->ni_ch_param.ni_bss_phymode);
}

/**
 * ieee80211_is_tpe_eirp_ext_present() - Return true if tpe eirp ext is present
 * false otherwise.
 * @tpe: Pointer to struct ieee80211_ie_vht_txpwr_env
 * @tpe_count: tpe count
 * Return : Return true if tpe eirp ext is present, false otherwise
 */
#ifdef WLAN_FEATURE_11BE
static bool
ieee80211_is_tpe_eirp_ext_present(struct ieee80211_ie_vht_txpwr_env *tpe,
                                  uint8_t tpe_count)
{
    if (ieee80211_get_num_ext_elements(tpe, tpe_count, false))
        return true;

    return false;
}
#else
static inline bool
ieee80211_is_tpe_eirp_ext_present(struct ieee80211_ie_vht_txpwr_env *tpe,
                                  uint8_t tpe_count)
{
    return false;
}
#endif

/**
 * ieee80211_process_tpe_eirp() - Parse and store the count(in the TPE IE)
 * number of eirp power values received from the TPE.
 *
 * When the count is 0 - The power value indicates maximum tx power for 20MHz.
 * When the count is 1 - The power value indicates maximum tx power for 20MHz,
 *                       and 40MHz.
 * When the count is 2 - The power value indicates maximum tx power for 20MHZ,
 *                       40MHz and 80MHz.
 * When the count is 3 - The power value indicates maximum tx power for 20MHZ,
 *                       40MHz,80MHz, 80P80MHz and 160MHz.
 * @ni: Pointer to the node that received the TPE.
 * @tpe: Input TPE IE.
 * Return: True if the EIRP power values have changed from the
 * previously stored value, else false.
 */
static bool
ieee80211_process_tpe_eirp(struct ieee80211_node *ni,
                           struct ieee80211_ie_vht_txpwr_env *tpe)
{
    u_int8_t tpe_info_cnt;
    int8_t n_ch_widths;
    int8_t *local_max_txpwr;
#ifdef WLAN_FEATURE_11BE
    bool is_txpwr_ext_present;
#endif
    bool is_power_changed;
    uint8_t n_subchans;
    uint8_t n_elem_to_cmp;
    struct ieee80211_ath_channel sta_ni_chan;
    uint16_t cur_chan_width;

    ieee80211_compute_ni_chan(ni, &sta_ni_chan);
    if (!IEEE80211_IS_CHAN_VALID(&sta_ni_chan))
        return false;

    tpe_info_cnt = tpe->tpe_payload.tpe_info_cnt;
    cur_chan_width = ieee80211_get_chan_width(&sta_ni_chan);
    n_subchans = cur_chan_width / MIN_6G_CHANWIDTH;
    n_ch_widths = iee80211_tpe_log2(n_subchans);
    n_ch_widths = (n_ch_widths == -1) ? tpe_info_cnt : n_ch_widths;

    local_max_txpwr = tpe->tpe_payload.local_max_txpwr;

#ifdef WLAN_FEATURE_11BE

    /* If AP transmits 320 MHz TPE in the extensible IE field and the station is
     * currently operating on 320 MHz channel width, parse the 320 MHz
     * EIRP power, else ignore it.
     */
    is_txpwr_ext_present =
        ieee80211_is_tpe_eirp_ext_present(tpe, tpe_info_cnt + 1);

    /* If we have the extensible tx power element in the TPE (EIRP),
     * then we need to compare one more element.
     * In that case we can say tpe_info_cnt = tpe_info_cnt +1
     */
    if (is_txpwr_ext_present && cur_chan_width == CHWIDTH_320) {
        tpe_info_cnt += 1;
    }
#endif
    ieee80211_scale_down_power(local_max_txpwr, tpe_info_cnt + 1);
    /* memcomparison of mode and old values from ni */
    n_elem_to_cmp = qdf_min(n_ch_widths + 1, tpe_info_cnt + 1);
    is_power_changed = !!qdf_mem_cmp(ni->ni_eirp_limit, local_max_txpwr, n_elem_to_cmp);

    if (is_power_changed) {
        /* Apply the power change and send the command to FW. */
        qdf_mem_copy(ni->ni_eirp_limit, local_max_txpwr, n_elem_to_cmp);
        /* write a minimum function to send only 1 value or store all the values */
        find_min_val_in_arr(&ni->ni_eirp_min_lim, local_max_txpwr, n_elem_to_cmp);
    }

    return is_power_changed;
}

/**
 * ieee80211_get_non11be_bw() - Given the scan_entry of the root ap,
 * find the root ap's non11be operating channel width by parsing the scan entry.
 *
 * @scan_entry: Pointer to ieee80211_scan_entry_t
 * @ni: Pointer to struct ieee80211_node
 *
 * Return: Non-11BE channel bandwidth
 *
 */
static uint16_t
ieee80211_get_non11be_bw(ieee80211_scan_entry_t  scan_entry,
                         struct ieee80211_node *ni)
{
    struct ieee80211_ath_channel ni_chan;
    enum ieee80211_phymode ap_phymode;
    enum phy_ch_width chan_width_of_ap;

    ni_chan = wlan_util_scan_entry_channel(ni->ni_ic, scan_entry, false);
    if (IEEE80211_IS_CHAN_VALID(&ni_chan)) {
        if (IEEE80211_IS_CHAN_11BE(&ni_chan)) {
            ni_chan = ieee80211_find_legacy_non11be_channel(ni->ni_ic, &ni_chan);
        }
        ap_phymode = ieee80211_chan2mode(&ni_chan);
        chan_width_of_ap = ieee80211_get_chan_width_from_phymode(ap_phymode);
    } else {
        chan_width_of_ap = 0;
    }

    return wlan_reg_get_bw_value(chan_width_of_ap);
}

/**
 * ieee80211_fill_ext_tpe_psd_elements() - Fill the ext_tpe_elem array with
 * PSD power values read from TPE extension field.
 * @tpe: Pointer to struct ieee80211_ie_vht_txpwr_env
 * @he_tpe_count: number of power values received as a part of TPE IE (excluding the TPE EXT)
 * @ext_tpe_elem: Array that is filled with ext tpe elements
 * @num_ext_tpe_pwr_to_read: Number of ext tpe pwr to read based on STA's BW
 *
 * Return - None
 */
static void
ieee80211_fill_ext_tpe_psd_elements(struct ieee80211_ie_vht_txpwr_env *tpe,
                                    uint8_t he_tpe_count,
                                    int8_t *ext_tpe_elem,
                                    uint8_t num_ext_tpe_pwr_to_read)
{
    uint8_t i;
    uint8_t tpe_info_len = sizeof(uint8_t);
    uint8_t tpe_legacy_txpow_len = sizeof(tpe->tpe_payload.local_max_txpwr[0]) * he_tpe_count;
    struct ieee80211_tpe_ext_psd_payload *tpe_ext_psd;
    uint8_t he_tpe_info_len = tpe_legacy_txpow_len + tpe_info_len;

    tpe_ext_psd = (struct ieee80211_tpe_ext_psd_payload *)((uint8_t *)tpe + IEEE80211_IE_HDR_LEN + he_tpe_info_len);

    for (i = 0 ; i < num_ext_tpe_pwr_to_read; i++) {
        ext_tpe_elem[i] = tpe_ext_psd->ext_max_txpwr[i];
    }
}

/**
 * ieee80211_sort_and_fill_tpe_psd_pwr() - If AP is operating on a punctured
 * BW, the legacy TPE IE field contains PSD power values corresponding to the
 * HE BSS operating channel width of the AP.
 * PSD values corresponding to (Number of 20MHZ in EHT BSS – Number of 20MHZ in HE BSS)
 * is sent in the extension IE. Hence, if we want to parse the extension TPE,
 * the TPE values received in the IE should be ordered as per the operating
 * channel BW. Re-order the TPE IE values and put them in tpe_psd_pwr array.
 *
 * Eg 1:
 * AP is operating in EHT160 with a 40 MHz puncturing.
 * EHT STA is also connected in 160 MHz BW.
 * PC indicates punctured channel.
 * Primary channel is 25.
 * In this case, HE AP BW is 40 MHz, and HE STA BW is also 40 MHz.
 * |-----|---|---|---|---|-------|-------|------|--|
 * |Chan#| 1 | 5 | 9 | 13| 17(PC)| 21(PC)| 25(P)|29|
 * | ----|---|---|---|---|-------|-------|------|--|
 * |PSD  | 5 | 6 | 4 |18 |  127  |  127  |   9  |10|
 * | ----|---|---|---|---|-------|-------|------|--|
 *
 * Hence following is TPE IE advertised by the AP:
 * Count: 2
 * Legacy PSD values: 9 10
 * EXT TPE values: 5 6 4 18 127 127
 * IE Contents: TPE_INFO | 9 10 | Ext Info | 5 6 4 18 127 127
 *
 * This TPE IE values has to be ordered as per the channel bonding for
 * AP operating BW.
 *
 * Eg 2:
 * AP is operating in EHT160 with a 40 MHz puncturing.
 * EHT STA is also connected in 160 MHz BW.
 * PC indicates punctured channel
 * Primary channel is 9.
 * In this case, HE AP BW is 80 MHz.
 *
 * |-----|---|---|-----|---|-------|-------|-------|--|
 * |Chan#| 1 | 5 | 9(P)| 13| 17(PC)| 21(PC)| 25    |29|
 * | ----|---|---|---  |---|-------|-------|-------|--|
 * |PSD  | 5 | 6 | 4   |18 | 127   | 127   |   9   |10|
 * | ----|---|---|-----|---|-------|-------|-------|--|
 *
 * Hence following is TPE IE advertised by the AP:
 * Count: 2
 * Legacy PSD values: 5 6 4 18
 * EXT TPE values: 127 127 9 10
 * IE Contents: TPE_INFO | 5 6 4 18 | Ext Info | 127 127 9 10
 *
 *
 * The contents of the IE are not sorted w.r.t the bonded channel of the AP.
 * Hence sort the TPE IE power values and read only the PSD values
 * corresponding to the STA BW.
 *
 * @ni: Pointer to the node that received the TPE.
 * @scan_entry: Pointer to ieee80211_scan_entry_t.
 * @he_chanwidth_of_sta: STA's HE channel width in MHz.
 * @n_he_subchans_in_ie: Number of channels in HE TPE IE.
 * @local_max_txpwr: PSD array consisting of HE psd power.
 * @n_ext_txpwr_elem: Number of array elements of ext_tpe_elem array
 * @ext_tpe_elem: PSD array consisting of 11be psd power.
 * @n_elem_to_cmp: Total number of power values to read for the client's BW.
 * @ni_chan: Pointer to STA ath_channel.
 * @sta_tpe_psd_pwr: Output PSD array for STA's BW.
 *
 * Return: None
 */
static void
ieee80211_sort_and_fill_ext_tpe_psd_pwr(struct ieee80211_node *ni,
                                        ieee80211_scan_entry_t  scan_entry,
                                        uint16_t he_chanwidth_of_sta,
                                        uint8_t n_he_subchans_in_ie,
                                        int8_t *local_max_txpwr,
                                        uint8_t n_ext_txpwr_elem,
                                        int8_t *ext_tpe_elem,
                                        uint8_t n_elem_to_cmp,
                                        struct ieee80211_ath_channel *ni_chan,
                                        int8_t *sta_tpe_psd_pwr)
{
    uint8_t i, j;
    struct ieee80211_ath_channel scan_chan;
    uint16_t eht_ap_chan_width, eht_chanwidth_of_sta;
    uint8_t is_index_filled[IEEE80211_TPE_NUM_POWER_SUPPORTED] = {0};
    int8_t local_ext_tpe_psd_pwr[IEEE80211_TPE_NUM_POWER_SUPPORTED] = {0};
    int8_t tpe_start_index;

    scan_chan = wlan_util_scan_entry_channel(ni->ni_ic, scan_entry, false);
    eht_ap_chan_width = ieee80211_get_chan_width(&scan_chan);
    eht_chanwidth_of_sta = ieee80211_get_chan_width(ni_chan);

    /* Find the index of HE TPE entries within the bonded channel order
     * for EHT BW and fill the HE TPE entries in its appropriate position
     * in EHT TPE array
     */
    tpe_start_index = ieee80211_get_start_index_for_sta_bw(eht_ap_chan_width,
                                                           he_chanwidth_of_sta,
                                                           ni_chan->ic_freq,
                                                           scan_entry->channel.cfreq1);

    for (i = tpe_start_index, j = 0; i < tpe_start_index + n_he_subchans_in_ie; i++, j++) {
        is_index_filled[i] = true;
        local_ext_tpe_psd_pwr[i] = local_max_txpwr[j];
    }

    /* Fill the extension TPE elements for EHT BW in order from
     * beginning and ignore already filled HE TPE elements
     */
    for (i = 0, j = 0; i < IEEE80211_TPE_NUM_POWER_SUPPORTED && j < n_ext_txpwr_elem; i++) {
        if (is_index_filled[i])
            continue;
        is_index_filled[i] = true;
        local_ext_tpe_psd_pwr[i] = ext_tpe_elem[j++];
    }

    /* Find the start index of EHT STA BW within EHT AP's BW
     * and copy only the EHT STA BW to the final PSD array.
     */
    tpe_start_index = ieee80211_get_start_index_for_sta_bw(eht_ap_chan_width,
                                                           eht_chanwidth_of_sta,
                                                           ni_chan->ic_freq,
                                                           scan_entry->channel.cfreq1);

    for (i = 0, j = tpe_start_index; i < tpe_start_index + n_elem_to_cmp; i++, j++) {
        sta_tpe_psd_pwr[i] = local_ext_tpe_psd_pwr[j];
    }
}

static inline uint8_t
ieee80211_get_num_ext_tpe_pwr_to_read(uint8_t n_elems_to_read_for_client_bw,
                                      uint8_t he_tpe_count,
                                      uint8_t n_ext_txpwr_elem_in_tpe)
{
    int8_t num_ext_tpe_pwr_to_read = n_elems_to_read_for_client_bw - he_tpe_count;

    if (num_ext_tpe_pwr_to_read < 0)
        return 0;

    return qdf_min((uint8_t)num_ext_tpe_pwr_to_read, n_ext_txpwr_elem_in_tpe);
}

#define TWO_POW(n) (0x1 << (n))

static uint8_t
ieee80211_extract_tpe_psd_pwr(struct ieee80211_node *ni,
                              struct ieee80211_ath_channel *sta_ni_chan,
                              struct ieee80211_ie_vht_txpwr_env *tpe,
                              ieee80211_scan_entry_t  scan_entry,
                              int8_t *tpe_psd_pwr)
{
    u_int8_t tpe_he_count;
    int8_t *local_max_txpwr;
    uint8_t n_he_subchans_in_ie, n_ext_txpwr_elem = 0;
    uint8_t n_max_subchans, n_elem_to_cmp;
    struct ieee80211_ath_channel legacy_ni_chan;
    uint16_t chanwidth_of_sta = 0, he_chanwidth_of_sta = 0;
    bool is_txpwr_ext_present = false, is_ext_parse_needed = false;
    int8_t ext_tpe_elem[IEEE80211_TPE_NUM_MAX_EXT_PSD_VALS] = {0};
    uint8_t total_tpe_count = 0;

    chanwidth_of_sta = ieee80211_get_chan_width(sta_ni_chan);
    if (IEEE80211_IS_CHAN_11BE(sta_ni_chan)) {
        legacy_ni_chan = ieee80211_find_legacy_non11be_channel(ni->ni_ic, sta_ni_chan);
        he_chanwidth_of_sta = ieee80211_get_chan_width(&legacy_ni_chan);

        /* We have to parse the extension TPE if the legacy
         * 11AX HE STA's BW is different from the 11BE EHT STA's BW
         */
        if (chanwidth_of_sta != he_chanwidth_of_sta) {
            is_ext_parse_needed = true;
        }
    } else {
        /* For non-11BE HE STA, parsing HE TPE is enough.
         */
        he_chanwidth_of_sta = chanwidth_of_sta;
    }

    n_max_subchans = chanwidth_of_sta / MIN_6G_CHANWIDTH;

    tpe_he_count = tpe->tpe_payload.tpe_info_cnt;

    local_max_txpwr = tpe->tpe_payload.local_max_txpwr;

    n_he_subchans_in_ie = tpe_he_count ? TWO_POW(tpe_he_count-1) : 1;

    n_ext_txpwr_elem = ieee80211_get_num_ext_elements(tpe,
                                                      n_he_subchans_in_ie,
                                                      true);
    if (n_ext_txpwr_elem) {
        is_txpwr_ext_present = true;
        ieee80211_fill_ext_tpe_psd_elements(tpe, n_he_subchans_in_ie,
                                            ext_tpe_elem,
                                            n_ext_txpwr_elem);
    }

    total_tpe_count = n_he_subchans_in_ie + n_ext_txpwr_elem;

    n_elem_to_cmp = qdf_min(total_tpe_count, n_max_subchans);

    if (n_elem_to_cmp > MAX_PSD_VALS)
        n_elem_to_cmp = MAX_PSD_VALS;

    if (is_ext_parse_needed && is_txpwr_ext_present) {
        ieee80211_sort_and_fill_ext_tpe_psd_pwr(ni,
                                                scan_entry,
                                                he_chanwidth_of_sta,
                                                n_he_subchans_in_ie,
                                                local_max_txpwr,
                                                n_ext_txpwr_elem,
                                                ext_tpe_elem,
                                                n_elem_to_cmp,
                                                sta_ni_chan,
                                                tpe_psd_pwr);
    } else {
        int8_t tpe_start_index;
        uint16_t he_ap_chan_width = ieee80211_get_non11be_bw(scan_entry, ni);
        uint8_t i, j;

        /* Find the start index of HE STA BW within AP's HE TPE elements
         * and copy only the HE STA BW to the final PSD array.
         */
        tpe_start_index = ieee80211_get_start_index_for_sta_bw(he_ap_chan_width,
                                                               he_chanwidth_of_sta,
                                                               sta_ni_chan->ic_freq,
                                                               scan_entry->channel.cfreq1);
        if (tpe_start_index < 0)
            return 0;

        /* There is only 1 value of PSD in the TPE IE for N=0. Hence start index is 0.
         *  Most of the time the AP sends only one PSD value, hence the optimization
         */
        if (n_he_subchans_in_ie == 1)
            tpe_start_index = 0;

        for (i = tpe_start_index, j = 0; i < tpe_start_index + n_elem_to_cmp ; i++, j++) {
            tpe_psd_pwr[j] = local_max_txpwr[i];
        }
    }

    return n_elem_to_cmp;
}

/**
 * ieee80211_process_tpe_eirppsd() - Parse and store the count(in the TPE IE)
 * number of eirppsd power values received from the TPE.
 *
 * When the count is 0 - The power value indicates maximum tx power for any PPDU.
 * When the count is 1 - The power value indicates maximum tx power for the
 * 20MHz channel.
 * When the count is 2 - The power values indicate maximum tx powers for two
 * consecutive 20MHz-subchannels of the 40Mhz channel.
 * When the count is 3 - The power values indicate maximum tx powers for four
 * consecutive 20MHz-subchannels of the 80Mhz channel.
 * When the count is 4 - The power values indicate maximum tx powers for eight
 * 20MHz-subchannels of the 160MHz/80P80Mhz channel.
 *
 * @ni: Pointer to the node that received the TPE.
 * @tpe: Input TPE IE.
 * Return: True if the PSD power values have changed from the
 * previously stored value, else false.
 */
static bool
ieee80211_process_tpe_eirppsd(struct ieee80211_node *ni,
                              struct ieee80211_ie_vht_txpwr_env *tpe,
                              ieee80211_scan_entry_t  scan_entry)
{
    uint8_t n_elem_to_cmp, n_max_subchans;
    bool is_power_changed;
    int8_t tpe_psd_pwr[IEEE80211_TPE_NUM_POWER_SUPPORTED] = {0};
    uint16_t chanwidth_of_sta = 0;
    struct ieee80211_ath_channel sta_ni_chan;

    ieee80211_compute_ni_chan(ni, &sta_ni_chan);
    if (!IEEE80211_IS_CHAN_VALID(&sta_ni_chan))
        return false;

    chanwidth_of_sta = ieee80211_get_chan_width(&sta_ni_chan);
    n_max_subchans = chanwidth_of_sta / MIN_6G_CHANWIDTH;

    n_elem_to_cmp = ieee80211_extract_tpe_psd_pwr(ni, &sta_ni_chan, tpe, scan_entry, tpe_psd_pwr);
    ieee80211_scale_down_power(tpe_psd_pwr, n_elem_to_cmp);

    /* memcomparison of new and old values from ni */
    is_power_changed = !!qdf_mem_cmp(ni->ni_eirppsd_limit, tpe_psd_pwr, n_elem_to_cmp);

    if (is_power_changed) {
        if (n_elem_to_cmp == 1 && n_max_subchans > 1) {
            qdf_mem_set(ni->ni_eirppsd_limit, n_max_subchans * sizeof(uint8_t), tpe_psd_pwr[0]);
        } else {
            qdf_mem_copy(ni->ni_eirppsd_limit, tpe_psd_pwr, n_elem_to_cmp);
        }
        /* Apply the power change and send the command to FW. */
        /* write a minimum function to send only 1 value or store all the values */
        find_min_val_in_arr(&ni->ni_eirppsd_min_lim, tpe_psd_pwr, n_elem_to_cmp);
    }

    return is_power_changed;
}

void
ieee80211_process_tpe_ie(struct ieee80211_node *ni, u_int8_t **input_tpe,
                         ieee80211_scan_entry_t  scan_entry)
{
    u_int8_t intrprt;
    u_int8_t category;
    enum reg_6g_client_type local_sta_category;
    int i;
    bool is_eirp_changed = false, is_psd_changed = false;
    struct ieee80211_ie_vht_txpwr_env *tpe[WLAN_MAX_NUM_TPE_IE];

    if (!input_tpe)
        return;

    if (!wlan_reg_is_6ghz_chan_freq(ni->ni_ch_param.ni_freq))
        return;

    /* If the radio is AFC capable and the AFC power event has been processed,
     * ignore the TPE.
     */
    if (wlan_reg_is_afc_power_event_received(ni->ni_ic->ic_pdev_obj))
        return;

    if (ni->ni_ap_power_type != REG_STANDARD_POWER_AP)
        return;

    /* For an SP root AP, subordinate clients are not supported.
     * Since a WIN client by default is a Subordinate client, allow it
     * to connect to the SP root AP as a Default client. Since currently
     * we are parsing TPE only for SP power type, use REG_DEFAULT_CLIENT
     * as the TPE category for parsing TPE.
     */
    local_sta_category = REG_DEFAULT_CLIENT;

    for (i = 0; i < WLAN_MAX_NUM_TPE_IE; i++) {
        tpe[i] = (struct ieee80211_ie_vht_txpwr_env *) input_tpe[i];

        if (!tpe[i])
            continue;
        category = tpe[i]->tpe_payload.tpe_info_cat;

        if (category != local_sta_category)
            continue;

        intrprt = tpe[i]->tpe_payload.tpe_info_intrpt;

        /* If the STA VAP's mobility type/category does not match the
         * TPE element's category then ignore the TPE.
         * NOTE:- The root AP may send more than one TPE IEs in the
         * same beacon/Probe Response.
         */
        switch (intrprt) {
            case MLME_IEEE80211_TPE_LOCAL_EIRP:
            case MLME_IEEE80211_TPE_REG_EIRP:
                /* If the AP is a composite AP and the client is SP certified client,
                 * the client need not parse the LPI TPE (sent with intrprt as MLME_IEEE80211_TPE_REG_EIRP)
                 */
                if (ni->ni_ap_regulatory_info == REG_INDOOR_SP_AP)
                    break;
            case MLME_IEEE80211_TPE_ADDITIONAL_REG_EIRP:
                is_eirp_changed = ieee80211_process_tpe_eirp(ni, tpe[i]);
                break;
            case MLME_IEEE80211_TPE_LOCAL_EIRP_PSD:
            case MLME_IEEE80211_TPE_REG_EIRP_PSD:
                /* If the AP is a composite AP and the client is SP certified client,
                 * the client need not parse the LPI TPE (sent with intrprt as MLME_IEEE80211_TPE_REG_EIRP_PSD)
                 */
                if (ni->ni_ap_regulatory_info == REG_INDOOR_SP_AP)
                    break;
            case MLME_IEEE80211_TPE_ADDITIONAL_REG_EIRP_PSD:
                is_psd_changed = ieee80211_process_tpe_eirppsd(ni, tpe[i], scan_entry);
                break;
            default:
                /* Maximum Transmit Power interpretation 4-7 is Reserved*/
                break;
        }
    }

    if ((is_eirp_changed || is_psd_changed) && ieee80211_vap_is_connected(ni->ni_vap)) {
        ieee80211_send_tpc_power_cmd(ni->ni_vap);
    }
}

uint8_t ieee80211_get_snr(ieee80211_scan_entry_t  scan_entry, struct ieee80211vap *vap)
{
    uint8_t                                      snr;
    qdf_list_t                                   *list;
    struct scan_filter                           *filter;
    struct scan_cache_entry                      *se = NULL;
    qdf_list_node_t                              *se_list = NULL;
    struct scan_cache_node                       *se_node;

    filter = qdf_mem_malloc(sizeof(*filter));
    if (!filter) {
        snr= util_scan_entry_snr(scan_entry);
        return snr;
    } else {
        filter->num_of_bssid = 1;
        filter->ssid_list[0].length = util_scan_entry_ssid(scan_entry)->length;
        if (filter->ssid_list[0].length) {
            filter->num_of_ssid = 1;
            OS_MEMCPY(filter->ssid_list[0].ssid,
                      util_scan_entry_ssid(scan_entry)->ssid,
                      util_scan_entry_ssid(scan_entry)->length);
        } else {
            filter->num_of_ssid = 0;
        }

        OS_MEMCPY((filter->bssid_list), util_scan_entry_bssid(scan_entry), QDF_MAC_ADDR_SIZE);

        list = ucfg_scan_get_result(wlan_vap_get_pdev(vap), filter);
        if (!list) {
            snr= util_scan_entry_snr(scan_entry);
            goto free_filter;
        }
        qdf_list_peek_front(list, &se_list);
        if (se_list) {
            se_node = qdf_container_of(se_list,
                    struct scan_cache_node, node);
            se = se_node->entry;
        }
        if(!se) {
            snr= util_scan_entry_snr(scan_entry);
        } else {
            snr= util_scan_entry_snr(se);
        }
    }
    ucfg_scan_purge_results(list);
free_filter:
    qdf_mem_free(filter);

    return snr;
}

#if DBDC_REPEATER_SUPPORT
static void
ieee80211_bringdown_secondary_stavap(struct ieee80211com *ic)
{
    int i = 0;
    struct ieee80211com *tmp_ic;
    struct global_ic_list *ic_list = ic->ic_global_list;
    wlan_if_t tmp_stavap;

    if (ic_list->num_stavaps_up != 2) {
        return;
    }

    for (i = 0; i < MAX_RADIO_CNT; i++) {
       GLOBAL_IC_LOCK_BH(ic_list);
       tmp_ic = ic_list->global_ic[i];
       GLOBAL_IC_UNLOCK_BH(ic_list);
       if(tmp_ic && tmp_ic->ic_sta_vap) {
           tmp_stavap = tmp_ic->ic_sta_vap;
           if (wlan_is_connected(tmp_stavap) && (tmp_stavap != ic_list->max_priority_stavap_up)) {
                qdf_print("RootAP access status changed, bring down secondary stavap:%s",ether_sprintf(tmp_stavap->iv_myaddr));
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(tmp_stavap, tmp_stavap->iv_bss->ni_macaddr, 0, IEEE80211_REASON_UNSPECIFIED, &tmp_stavap->iv_bss->peer_obj->mldaddr[0]);
#else
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(tmp_stavap, tmp_stavap->iv_bss->ni_macaddr, 0, IEEE80211_REASON_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                break;
           }
       }
    }
}
#endif

#ifdef WLAN_FEATURE_11BE_MLO
void ieee80211_process_partner_link_csa(struct wlan_objmgr_vdev *vdev,
        ieee80211_scan_entry_t scan_entry)
{
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct ieee80211_ath_channelswitch_ie *csaie = NULL;
    struct ieee80211_extendedchannelswitch_ie *ecsaie = NULL;
    struct ieee80211_max_chan_switch_time_ie *mcstie = NULL;
    struct partner_link_info *link_info = NULL;
    int i = 0;
    uint32_t chan_switch_time = 0;
    QDF_STATUS status;
    struct ieee80211com *co_mld_ic = NULL;
    struct ieee80211_ath_channel chan_obj = empty_channel;
    struct ieee80211_ath_channel *chan = &chan_obj;
    struct ieee80211_node *ni = NULL;
    uint8_t update_beacon;
    uint8_t primary_chan = 0;

    for (i = 0; i < scan_entry->ml_info.num_links; i++) {
        link_info = &scan_entry->ml_info.link_info[i];
        csaie = (struct ieee80211_ath_channelswitch_ie *)link_info->csa_ie;
        ecsaie = (struct ieee80211_extendedchannelswitch_ie *)link_info->ecsa_ie;
        mcstie = (struct ieee80211_max_chan_switch_time_ie *)link_info->max_cst_ie;
        update_beacon = 0;

        /* Skip the link if CSA, ECSA and MCST IEs are not present */
        if (!(csaie || ecsaie || mcstie))
            continue;

        co_mld_vdev = mlo_get_vdev_by_link_id(vdev, link_info->link_id,
                                              WLAN_MLO_MGR_ID);
        if (!co_mld_vdev) {
            mlo_debug("co_mld_vdev is NULL");
            continue;
        }

        co_mld_vap = wlan_vdev_get_mlme_ext_obj(co_mld_vdev);
        if (!co_mld_vap) {
            mlo_err("co_mld_vap is NULL");
            wlan_objmgr_vdev_release_ref(co_mld_vdev, WLAN_MLO_MGR_ID);
            continue;
        }

        co_mld_ic = co_mld_vap->iv_ic;
        if (!co_mld_ic) {
            mlo_err("co_mld_ic is NULL");
            wlan_objmgr_vdev_release_ref(co_mld_vdev, WLAN_MLO_MGR_ID);
            continue;
        }

        if (csaie) {
            primary_chan = csaie->newchannel;
        } else if (ecsaie) {
            primary_chan = ecsaie->newchannel;
        }

        /* Skip the CSA if it already received on the CSA triggered link */
        if (co_mld_ic->ic_chanchange_channel &&
            co_mld_ic->ic_chanchange_channel->ic_ieee == primary_chan) {
            wlan_objmgr_vdev_release_ref(co_mld_vdev, WLAN_MLO_MGR_ID);
            continue;
        }

        if (mcstie) {
            co_mld_ic->ic_mcst_of_rootap =
                ieee80211_get_max_chan_switch_time(mcstie);
        } else {
            if (IEEE80211_IS_CHAN_VALID(chan) &&
                !IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan)) {
                co_mld_ic->ic_mcst_of_rootap = 0;
            } else {
                co_mld_ic->ic_mcst_of_rootap = UNSIGNED_MAX_24BIT;
            }
        }

        status = wlan_util_vdev_mgr_get_csa_channel_switch_time(co_mld_vdev,
                &chan_switch_time);
        if (QDF_IS_STATUS_ERROR(status)) {
            mlo_err("Failed to get CSA channel switch time");
            wlan_objmgr_vdev_release_ref(co_mld_vdev, WLAN_MLO_MGR_ID);
            continue;
        }

        /* If ic_has_rootap_done_cac is true, then on CSA channel change WDS STA
         * does not send the Disassoc frame to RootAP, STA vap will be restarted
         * in the new channel and remains connected to the RootAP. Otherwise,
         * WDS STA sends the Disassoc frame to the RootAP. If the Agile-DFS/
         * preCAC feature is enabled, and if the CSA channel is a DFS channel,
         * then the RootAP does not add CAC duration to Max channel switch
         * time to indicate that CAC is already done in the new DFS channel.
         *
         * For MLO STA case, yet to handle the max channel switch time IE.
         * Even if the RootAP includes CAC duration in the max channel switch
         * time value, STA should not get disconnected from the RootAP.
         */
        if (co_mld_ic->ic_mcst_of_rootap <= chan_switch_time)
            co_mld_ic->ic_has_rootap_done_cac = true;

        if (csaie || ecsaie) {
            ni = co_mld_vap->iv_bss;
            *chan = ieee80211_get_new_sw_chan(ni, csaie, ecsaie, NULL, NULL,
                                              NULL);
            if (IEEE80211_IS_CHAN_VALID(chan) &&
#ifdef WLAN_FEATURE_11BE
                !(IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(co_mld_ic, chan, chan->op_puncture_bitmap))
#else
                !(IEEE80211_IS_CHAN_RADAR(co_mld_ic, chan))
#endif
               ) {
                IEEE80211_DPRINTF_IC(co_mld_ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                                     "%s: CSA received via per-STA profile, new chan:%d freq:%d, mode/flags:0x%llx\n",
                                     __func__, chan->ic_ieee, chan->ic_freq,
                                     chan->ic_flags);
            } else {
                IEEE80211_DPRINTF_IC(co_mld_ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                                     "%s: Channel is not available, disconnect the STA vap.\n",
                                     __func__);
                co_mld_vap->iv_ni = ni;
                OS_SET_TIMER(&co_mld_vap->iv_disconnect_sta_timer,
                             IEEE80211_TU_TO_MS(ni->ni_chanswitch_tbtt * ni->ni_intval));
            }

            ieee80211_mgmt_sta_send_csa_rx_nl_msg(co_mld_ic, co_mld_vap->iv_bss,
                                                  chan, csaie, ecsaie, NULL,
                                                  NULL, NULL);

            if (IEEE80211_IS_CHAN_VALID(chan) &&
                !IEEE80211_ARE_CHANS_EQUAL(chan, co_mld_ic->ic_curchan) &&
                ieee80211_ic_doth_is_set(co_mld_ic) &&
                (wlan_vdev_is_up(co_mld_vdev) == QDF_STATUS_SUCCESS)) {
                ieee80211_process_csa(ni, chan, csaie, ecsaie, &update_beacon);
            }
        }

        if (update_beacon)
            wlan_pdev_beacon_update(co_mld_ic);

        wlan_objmgr_vdev_release_ref(co_mld_vdev, WLAN_MLO_MGR_ID);
    }
}
#else
void ieee80211_process_partner_link_csa(struct wlan_objmgr_vdev *vdev,
        ieee80211_scan_entry_t scan_entry)
{
    return;
}
#endif /* WLAN_FEATURE_11BE_MLO */

void ieee80211_recv_beacon_sta(struct ieee80211_node *ni, wbuf_t wbuf, int subtype,
                               struct ieee80211_rx_status *rs,
			       ieee80211_scan_entry_t  scan_entry)
{
    struct ieee80211vap                          *tmpvap, *vap = ni->ni_vap;
    struct ieee80211com                          *ic = ni->ni_ic;
    u_int16_t                                    capinfo, erp;
    u_int8_t                                     *tim_elm = NULL;
    struct ieee80211_ath_channelswitch_ie            *chanie = NULL;
    struct ieee80211_extendedchannelswitch_ie    *echanie = NULL;
    struct ieee80211_ie_wide_bw_switch           *widebwie = NULL;
    struct ieee80211_ie_sec_chan_offset          *secchanoff = NULL;
    struct ieee80211_max_chan_switch_time_ie     *mcst = NULL;
    u_int8_t                                     *cswrp = NULL;
    struct ieee80211_ath_channel                 chan_obj = empty_channel;
    struct ieee80211_ath_channel*                chan = &chan_obj;
    u_int8_t                                     *htcap = NULL;
    u_int8_t                                     *htinfo = NULL;
    u_int8_t                                     *wme;
    struct ieee80211_frame                       *wh;
    u_int64_t                                    tsf;
    systime_t                                    previous_beacon_time;
    u_int8_t                                     *vhtcap = NULL;
    u_int8_t                                     *vhtop = NULL;
    u_int8_t                                     *opmode = NULL;
    u_int8_t                                     *bwnss_map = NULL;
    enum ieee80211_cwm_width                     ochwidth = ni->ni_chwidth;
#ifdef WLAN_FEATURE_11BE
    uint16_t                                     opp = ni->ni_ch_param.puncture_bitmap;
#endif
    u_int8_t                                     *hecap = NULL;
    u_int8_t                                     *hecap_6g = NULL;
    u_int8_t                                     *heop = NULL;
    u_int8_t                                     *muedca = NULL;
    u_int8_t                                     *mdie = NULL;
    u_int8_t                                     **tpe = NULL;
    struct wlan_objmgr_pdev                      *pdev;
    uint8_t                                      snr;
    struct wlan_beacon_frm_info                  beacon_info = {0};
    bool                                         peer_update_required = false;
#if REPEATER_SAME_SSID
    uint8_t disconnect_sec_stavap 		= 0;
#endif
    u_int8_t                                     nss;
    u_int8_t update_beacon                       = 0;
#ifdef WLAN_FEATURE_11BE
    uint8_t                                     *ehtcap = NULL;
    uint8_t                                     *ehtop = NULL;
    struct ieee80211_ath_channel eht_chan = empty_channel;
    bool trigger_csa = false;
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t t2lm_enable = 0;
    uint8_t *t2lm_ie;
    int retval = 0;
    struct wlan_t2lm_context rx_t2lm = {0};
#endif
#endif /* WLAN_FEATURE_11BE */
    struct ieee80211_ath_channel ni_chan;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
    uint32_t chan_switch_time = 0;
    QDF_STATUS status;
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t *ml_rv_ie = NULL;
    qdf_size_t ml_ie_len = 0;
    ieee80211_scan_entry_t  scan_cache_entry;
    struct mlo_partner_info partner_info = {0};
    struct wlan_mlo_dev_context *mlo_dev_ctx;
#endif

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null", __func__);
        return;
    }

    wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    OS_MEMCPY((u_int8_t *)&tsf, util_scan_entry_tsf(scan_entry), sizeof(tsf));
    capinfo = util_scan_entry_capinfo(scan_entry).value;

    /* FIX for EV# 98854: Preserving last successful beacon */
    previous_beacon_time = vap->iv_last_beacon_time;
    /*
     * When operating in station mode, check for state updates.
     * Be careful to ignore beacons received while doing a
     * background scan.  We consider only 11g/WMM stuff right now.
     */
    if ((ieee80211_node_get_associd(ni) != 0) &&
	  IEEE80211_ADDR_EQ(util_scan_entry_macaddr(scan_entry),ieee80211_node_get_bssid(ni))) {
        /*
         * record tsf of last beacon
         */
        OS_MEMCPY(ni->ni_tstamp.data, &tsf, sizeof(ni->ni_tstamp));

        /*
         * record absolute time of last beacon
         */
        vap->iv_last_beacon_time = OS_GET_TIMESTAMP();

        /*
         * check for ERP change
         */
        erp = util_scan_entry_erpinfo(scan_entry);
        if ((ni->ni_erp != erp) && (ieee80211_vaps_ready(ic, IEEE80211_M_HOSTAP) == 0)) {
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                           "erp change: was 0x%x, now 0x%x",
                           ni->ni_erp, erp);
            if (erp & IEEE80211_ERP_USE_PROTECTION)
                IEEE80211_ENABLE_PROTECTION(ic);
            else
                IEEE80211_DISABLE_PROTECTION(ic);
            IEEE80211_COMM_LOCK(ic);
            TAILQ_FOREACH(tmpvap, &(ic)->ic_vaps, iv_next) {
                ieee80211_vap_erpupdate_set(tmpvap);
            }
            IEEE80211_COMM_UNLOCK(ic);
            ieee80211_set_protmode(ic);
            ni->ni_erp = erp;
            if (ni->ni_erp & IEEE80211_ERP_LONG_PREAMBLE)
                IEEE80211_ENABLE_BARKER(ic);
            else
                IEEE80211_DISABLE_BARKER(ic);
        }

        /*
         * check for slot time change
         */
        if ((ni->ni_capinfo ^ capinfo) & IEEE80211_CAPINFO_SHORT_SLOTTIME) {
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                           "capabilities change: was 0x%x, now 0x%x",
                           ni->ni_capinfo, capinfo);
            /*
             * NB: we assume short preamble doesn't
             *     change dynamically
             */
            ieee80211_set_shortslottime(ic,
                                        IEEE80211_IS_CHAN_A(vap->iv_bsschan) ||
                                        IEEE80211_IS_CHAN_11NA(vap->iv_bsschan) ||
                                        (capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));
            ni->ni_capinfo &= ~IEEE80211_CAPINFO_SHORT_SLOTTIME;
            ni->ni_capinfo |= capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME;
        }

        /*
         * check for tim
         */
        if (wlan_scan_can_transmit(wlan_vdev_get_pdev(vap->vdev_obj))) {
            tim_elm = util_scan_entry_tim(scan_entry);
            if (tim_elm != NULL) {
                struct ieee80211_ath_tim_ie    *tim = (struct ieee80211_ath_tim_ie *) tim_elm;
                struct vdev_mlme_proto_generic *mlme_generic = &ni->ni_vap->vdev_mlme->proto.generic;

                /* Also update vap's iv_dtim_period. OS may querry */
                mlme_generic->dtim_period = tim->tim_period;
                /* Only field iv_dtim_period was being updated, while iv_dtim_count was never changed.
                 * It was fine while nobody used it, but TDLS does.
                 * (AP code has a private copy of beacon to be transmitted, and updates DTIM count that
                 * copy instead of updating/reading the val    ue from the VAP
                 */
                ni->ni_vap->iv_dtim_count = tim->tim_count;
            }
        }

        /*
         * check for WMM parameters
         */
        if (((wme = util_scan_entry_wmeparam(scan_entry)) != NULL) ||
            ((wme = util_scan_entry_wmeinfo(scan_entry))  != NULL)) {
            int         _retval;
            u_int8_t    qosinfo;

            /* Node is WMM-capable if WME IE (either subtype) is present */
            ni->ni_ext_caps |= IEEE80211_NODE_C_QOS;

            ni->ni_wme_miss_threshold = 0;

            if (vap->iv_opmode != IEEE80211_M_BTAMP) {
            /* Parse IE according to subtype */
            if (iswmeparam(wme)) {
                _retval = ieee80211_parse_wmeparams(vap, wme, &qosinfo, 0);
            }
            else {
                _retval = ieee80211_parse_wmeinfo(vap, wme, &qosinfo);
            }

            /* Check whether WME parameters changed */
            if (_retval > 0) {
                /*
                 * Do not update U-APSD capability; this is done
                 * during association request.
                 */

                /* Check whether node is in a WMM connection */
                if (ni->ni_flags & IEEE80211_NODE_QOS) {
                    /* Update WME parameters  */
                    if (iswmeparam(wme)) {
                        ieee80211_wme_updateparams(vap);
                    }
                    else {
                        ieee80211_wme_updateinfo(vap);
                    }
                }

                /*
                 * If association used U-APSD and AP no longer
                 * supports it we must disassociate and renegotiate
                 * U-APSD support.
                 *
                 * We don't need to do anything if association did not
                 * use U-APSD and AP now supports it.
                 */
                if (ni->ni_flags & IEEE80211_NODE_UAPSD) {
                    if ((qosinfo & WME_CAPINFO_UAPSD_EN) == 0) {
                        ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);

                        /* Disassociate for unspecified QOS-related reason. */
                        ieee80211_mlme_recv_deauth(ni, IEEE80211_REASON_QOS);
                    }
                }
            }
            }
        } else {
            /* If WME IE not present node is not WMM capable */
            ni->ni_wme_miss_threshold++;
            if(ni->ni_wme_miss_threshold > 3 ) {
                ni->ni_ext_caps &= ~IEEE80211_NODE_C_QOS;

            /*
             * If association used U-APSD and AP no longer
             * supports it we must disassociate and renegotiate
             * U-APSD support.
             *
             * We don't need to do anything if association did not
             * use U-APSD and AP now supports it.
             */
                if (ni->ni_flags & IEEE80211_NODE_UAPSD) {
                    ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);

                    /* Disassociate for unspecified QOS-related reason. */
                    //ieee80211_mlme_recv_deauth(ni, IEEE80211_REASON_QOS);
                    qdf_nofl_info("AP no longer supports UAPSD, disconnecting from AP\n");
                    wlan_mlme_deauth_request(vap, ni->ni_macaddr, IEEE80211_REASON_QOS);
                }
            }
        }

        /*
         * check for spectrum management
         */
        if ((capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT) || ieee80211_ic_2g_csa_is_set(ic)) {
            chanie  = (struct ieee80211_ath_channelswitch_ie *) util_scan_entry_csa(scan_entry);
            echanie = (struct ieee80211_extendedchannelswitch_ie *) util_scan_entry_xcsa(scan_entry);
            widebwie = (struct ieee80211_ie_wide_bw_switch *) util_scan_entry_widebw(scan_entry);
            mcst = (struct ieee80211_max_chan_switch_time_ie *)util_scan_entry_mcst(scan_entry);
            secchanoff = (struct ieee80211_ie_sec_chan_offset *) util_scan_entry_secchanoff(scan_entry);
            cswrp = util_scan_entry_cswrp(scan_entry);

            if (chanie || echanie) {
                *chan = ieee80211_get_new_sw_chan (ni, chanie, echanie, secchanoff, widebwie, cswrp);
                if (IEEE80211_IS_CHAN_VALID(chan) &&
#ifdef WLAN_FEATURE_11BE
                    !(IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, chan, chan->op_puncture_bitmap))
#else
                    !(IEEE80211_IS_CHAN_RADAR(ic, chan))
#endif
                   ) {
                    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                                         "%s: Channel switch announcement received, switching to chan: %d freq: %d, mode/flags: 0x%llx\n",
                                         __func__, chan->ic_ieee, chan->ic_freq, chan->ic_flags);
                } else {
                    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                                         "%s: Channel is not available, disconnect the STA vap.\n", __func__);
                    vap->iv_ni = ni;
                    OS_SET_TIMER(&vap->iv_disconnect_sta_timer,IEEE80211_TU_TO_MS(ni->ni_chanswitch_tbtt*ni->ni_intval));
                }

#ifdef WLAN_FEATURE_11BE_MLO
                /* Disconnect the STA vap if all the links of the MLD is not
                 * connected to the AP.
                 */
                if (!ucfg_mlo_is_mld_connected(vap->vdev_obj)) {
                    vap->iv_ni = ni;
                    qdf_err("vap:%s, Disconnect the STA vap as MLD is not connected", vap->iv_netdev_name);
                    OS_SET_TIMER(&vap->iv_disconnect_sta_timer,
                                 IEEE80211_TU_TO_MS(ni->ni_chanswitch_tbtt * ni->ni_intval));
                    return;
                }
#endif

                ieee80211_mgmt_sta_send_csa_rx_nl_msg(ic, ni, chan, chanie, echanie, secchanoff, widebwie, cswrp);

                /* MCST IE is sent along with chanie/echanie. Hence process mcst
                 * only if chanie or echanie is present.
                 * If MCSTIE is not sent by Root AP, set max_chanswitch_time to:
                 * 1. A very big value if target channel is DFS. This sets
                 * "ic->ic_has_rootap_done_cac" to false and RE AP does CAC on
                 * DFS chan.
                 * 2. Zero if target channel is non-DFS. This sets
                 * "ic->ic_has_rootap_done_cac" to true and ensures that RE's
                 * STA vap does not disconnect and scan.
                 */
                if (mcst) {
                    ic->ic_mcst_of_rootap =
                        ieee80211_get_max_chan_switch_time(mcst);
                } else {
                    if (IEEE80211_IS_CHAN_VALID(chan) &&
                        !IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan)) {
                        ic->ic_mcst_of_rootap = 0;
                    } else {
                        ic->ic_mcst_of_rootap = UNSIGNED_MAX_24BIT;
                    }
                }

                /* chan_switch_time does not include the DFS CAC duration*/
                status = wlan_util_vdev_mgr_get_csa_channel_switch_time(vap->vdev_obj,
                        &chan_switch_time);
                if (QDF_IS_STATUS_ERROR(status)) {
                    qdf_err("Failed to get CSA channel switch time");
                    return;
                }

                if(ic->ic_mcst_of_rootap <= chan_switch_time) {
                    ic->ic_has_rootap_done_cac = true;
                }
            }
            ni->ni_capinfo |= IEEE80211_CAPINFO_SPECTRUM_MGMT;
        } else {
            ni->ni_capinfo &= ~IEEE80211_CAPINFO_SPECTRUM_MGMT;
        }

        /*
         * check HT IEs
         * Parse HT capabilities with VHT
         */
        if (IEEE80211_NODE_USE_HT(ni)) {
            htcap  = util_scan_entry_htcap(scan_entry);
            htinfo = util_scan_entry_htinfo(scan_entry);
            if (htcap) {
                ieee80211_parse_htcap(ni, htcap, &peer_update_required);
            }
            if (htinfo) {
                ieee80211_parse_htinfo(ni, htinfo);
            }
        }

        bwnss_map = util_scan_entry_bwnss_map(scan_entry);
        if (bwnss_map) {
            ni->ni_bw160_nss = IEEE80211_GET_BW_NSS_FWCONF_160(*(u_int32_t *)bwnss_map);
            ni->ni_bw80p80_nss = 0;
            ni->ni_prop_ie_used = true;
        } else {
            ni->ni_bw160_nss = 0;
            ni->ni_bw80p80_nss = 0;
        }

        if (IEEE80211_NODE_USE_VHT(ni)) {
            vhtcap = util_scan_entry_vhtcap(scan_entry);
            vhtop = util_scan_entry_vhtop(scan_entry);
            opmode = util_scan_entry_omn(scan_entry);

            if ((vhtcap != NULL) && (vhtop != NULL) && htinfo) {
                ieee80211_parse_vhtcap(ni, vhtcap, &peer_update_required);
                ieee80211_parse_vhtop(ni, vhtop, htinfo);
            }

            if (opmode) {
                ieee80211_parse_opmode_notify(ni, opmode, subtype);
            }
        }

        if (IEEE80211_NODE_USE_HE(ni)) {

            hecap = util_scan_entry_hecap(scan_entry);
            heop  = util_scan_entry_heop(scan_entry);
            hecap_6g = util_scan_entry_he_6g_cap(scan_entry);

            if (hecap != NULL) {
                ieee80211_parse_hecap(ni, hecap, subtype);
            }

            if (hecap_6g != NULL) {
                ieee80211_parse_he_6g_bandcap(ni, hecap_6g, subtype);
            }

            if (heop != NULL) {
                u_int32_t saved_ni_heop_param = ni->ni_he.heop_param;
                u_int32_t heop_info;
#if SUPPORT_11AX_D3
                u_int8_t saved_ni_heop_bsscolor_info =
                    ni->ni_he.heop_bsscolor_info;
#endif

                ieee80211_parse_heop(ni, heop, subtype, &update_beacon);

#if SUPPORT_11AX_D3
                if ((ni->ni_he.heop_param != saved_ni_heop_param) &&
                    (ni->ni_he.heop_bsscolor_info) == saved_ni_heop_bsscolor_info) {
                    heop_info = (ni->ni_he.heop_param |
                            (ni->ni_he.heop_bsscolor_info << HEOP_PARAM_S));
#else
                if (ni->ni_he.heop_param != saved_ni_heop_param) {
                    /* There is a change in the AP side HE OP params.
                     * We need to update target with these new params.
                     */
                    heop_info = ni->ni_he.heop_param;
#endif
                    if(ic->ic_vap_set_param &&
                        (ic->ic_vap_set_param(vap,
                          IEEE80211_CONFIG_HE_OP, heop_info) != EOK)) {

                        qdf_print("%s WARNING!!! heop set to target failed", __func__);
                    }
                }
            }
        }

        if((muedca = util_scan_entry_muedca(scan_entry)) != NULL) {

            ieee80211_parse_muedcaie(vap, muedca, IEEE80211_PARSE_MUEDCA);
        }
    }
#if QCN_IE
    if (scan_entry->ie_list.qcn != NULL)
        ieee80211_parse_qcnie(scan_entry->ie_list.qcn, NULL, ni, NULL, subtype);
#endif
#if WLAN_FEATURE_11BE
        if (IEEE80211_NODE_USE_EHT(ni)) {

            ehtcap = util_scan_entry_ehtcap(scan_entry);
            ehtop  = util_scan_entry_ehtop(scan_entry);

            if (ehtcap != NULL) {
                ieee80211_parse_ehtcap(ni, ehtcap, subtype);
            }

            if (ehtop != NULL) {
                ieee80211_parse_ehtop(ni, ehtop, subtype, &update_beacon);
            }
        }
#ifdef WLAN_FEATURE_11BE_MLO
        mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vap->vdev_obj);

        /* Process the broadcast T2LM IE if,
         * STA is connected to AP, and
         * - Critical bit is set, or
         * - STA connected to AP when AP is still advertising the
         * broadcast T2LM (default mapping in established T2LM is set to
         * 1 and ongoing mapping does not have any T2LM IE info).
         */
        if (ieee80211_node_is_authorized(ni) &&
            ((capinfo & IEEE80211_CAPINFO_PBCC) ||
             (wlan_peer_is_mlo(ni->peer_obj) && mlo_dev_ctx &&
              (mlo_dev_ctx->t2lm_ctx.established_t2lm.t2lm.default_link_mapping &&
               !mlo_dev_ctx->t2lm_ctx.upcoming_t2lm.t2lm.mapping_switch_time_present)))) {
            ieee80211_get_t2lm_enable(ic, &t2lm_enable);
            if ((t2lm_enable == WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) &&
                 ni->ml_info.num_partner_links > 1) {
                t2lm_ie = util_scan_entry_t2lm(scan_entry);

                if (t2lm_ie) {
                    retval = ieee80211_parse_bcn_prbresp_t2lm_ie(ni, &rx_t2lm,
                            t2lm_ie, util_scan_entry_t2lm_len(scan_entry));
                    if (retval) {
                        t2lm_err("failed to parse the T2LM IE");
                        return;
                    }
                    wlan_process_bcn_prbrsp_t2lm_ie(vap->vdev_obj, &rx_t2lm, tsf);
                }
            }
        }
#endif
#endif

    /*
     * Check BBSID before updating our beacon configuration to make
     * sure the received beacon is really from our AP.
     */
    if ((ni == vap->iv_bss) &&
        (IEEE80211_ADDR_EQ( util_scan_entry_bssid(scan_entry), ieee80211_node_get_bssid(ni)))) {
        struct ieee80211_ath_channel scan_chan = wlan_util_scan_entry_channel(vap->iv_ic,
                                                                   scan_entry,
                                                                   false);
        //
        // Check ssid and phymode to make sure ESS/phymode broadcasted by the AP is the same to
        // which we're trying to connect. AP may have been reconfigured but
        // old entry remains in the scan list until aged out. Only perform phymode and channel check
        // if we are operating in auto mode.Perform the channel check and ignore the phymode check
        // if we are operating in forced mode.
        //
        struct chan_params chan_p = {0};

        chan_p.freq = ni->ni_ch_param.ni_freq;
        chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
        set_chan_params_puncture_bitmap(&chan_p, ni->ni_ch_param.puncture_bitmap);
        chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE;

        ni_chan = ieee80211_find_dot11_channel(ni->ni_ic, &chan_p,
                                               ni->ni_ch_param.ni_bss_phymode);

        if (!IEEE80211_IS_CHAN_VALID(&ni_chan))
            return;

        if (ieee80211_ssid_match(scan_entry, ni)  &&
            ((!ieee80211_is_phymode_auto(vap->iv_des_mode) &&
              (ni->ni_ch_param.ni_freq == scan_entry->channel.chan_freq)) ||
              (ieee80211_phymode_match(scan_entry, ni) &&
               IEEE80211_ARE_CHANS_EQUAL(&ni_chan, &scan_chan))) ) {
            /* If fast bss transition enabled then track mdie in beacons */
            if (vap->iv_roam.iv_ft_enable) {
                if((mdie = util_scan_entry_mdie(scan_entry)) != NULL) {
                        qdf_mem_copy(vap->iv_roam.iv_mdie, mdie, MD_IE_LEN);
                } else {
                    if (vap->iv_roam.iv_mdie[1])
                        qdf_mem_zero(vap->iv_roam.iv_mdie, MD_IE_LEN);
                }
            } else {
                if (vap->iv_roam.iv_mdie[1])
                    qdf_mem_zero(vap->iv_roam.iv_mdie, MD_IE_LEN);
            }

            vap->iv_lastbcn_phymode_mismatch = 0;
            ieee80211_mlme_join_complete_infra(ni);
        }
        else {
         /* EV# 98854 Reverting the Beacon time stamp if scan_entry
            channel mismatches  */
            vap->iv_lastbcn_phymode_mismatch = 1;
            vap->iv_last_beacon_time = previous_beacon_time;
        }

        if ((wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) &&
           (subtype == IEEE80211_FC0_SUBTYPE_BEACON)) {
            ic->ic_beacon_update(ni, rs->rs_snr);
            ieee80211_beacon_chanutil_update(vap);
        }

    snr = ieee80211_get_snr(scan_entry, vap);
    nss = ni->ni_maxrxstreams;
    beacon_info.se_sonadv = util_scan_entry_sonie(scan_entry);
    beacon_info.snr = snr;
    beacon_info.nss = nss;
    psoc = wlan_vdev_get_psoc(vap->vdev_obj);

    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }

    if (rx_ops && rx_ops->son_rx_ops.process_mgmt_frame) {
        rx_ops->son_rx_ops.process_mgmt_frame(vap->vdev_obj, ni->peer_obj, subtype, wbuf_header(wbuf),
                                              wbuf_get_pktlen(wbuf), &beacon_info);
    }
#if REPEATER_SAME_SSID
    if (wlan_rptr_global_is_s_ssid()) {
        update_beacon = wlan_rptr_update_extender_info(vap->vdev_obj, scan_entry, &disconnect_sec_stavap, wh->i_addr3);
        if (disconnect_sec_stavap) {
#if DBDC_REPEATER_SUPPORT
            ieee80211_bringdown_secondary_stavap(ic);
#endif
        }
    }
#endif
        ieee80211_vap_compute_tsf_offset(vap, wbuf, rs);
        tpe = util_scan_entry_tpe(scan_entry);
        ieee80211_process_tpe_ie(ni, tpe, scan_entry);

        /*
        * Set or clear flag indicating reception of channel switch announcement
        * in this channel. This flag should be set before notifying the scan
        * algorithm.
        * We should not send probe requests on a channel on which CSA was
        * received until we receive another beacon without the said flag.
        */
#ifdef WLAN_FEATURE_11BE
        if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
            enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;
            struct wlan_channel des_chan = {0};

            eht_chan = wlan_util_scan_entry_channel(ic, scan_entry, false);

            chan_p.freq = eht_chan.ic_freq;
            chan_p.cfreq2 = eht_chan.ic_vhtop_freq_seg2;
            set_chan_params_puncture_bitmap(&chan_p, wlan_util_scan_entry_puncture_bitmap(ic, scan_entry));

            phymode = ieee80211_chan2mode(&eht_chan);

            phymode = ieee80211_get_phy_mode(ic, &eht_chan, vap->iv_des_mode,
                    phymode);

            /*
             * Derive the appropriate phymode after checking the 2.4GHz
             * 20/40MHz intolerance flags for the desired channel.
             */
            des_chan.ch_phymode = phymode2convphymode[phymode];
            des_chan.ch_freq = eht_chan.ic_freq;
            des_chan.ch_flags = eht_chan.ic_flags;
            phymode = ieee80211_coex_derive_phymode(ic, &des_chan);

            if (ieee80211_is_phymode_11be(phymode)) {
                bool is_input_phymode_radar = false;
                struct chan_params eht_chan_p;

                /* currently doing dynamic channel change only for eht phymode
                   other modes to be added later */

                set_chan_params_puncture_bitmap(&chan_p, ieee80211_intersect_puncture_bitmap(&eht_chan, phymode, ieee80211_chan2mode(&eht_chan)));
                eht_chan_p = chan_p;
                eht_chan = ieee80211_find_dot11_channel_without_radar(ic, &chan_p, phymode, &is_input_phymode_radar);
                if (is_input_phymode_radar && IEEE80211_IS_CHAN_VALID(&eht_chan) &&
                        (!vap->iv_dfs_tx_allowed || ic->ic_dfs_waitfor_csa_sched)) {
                    /* Radar action in progress. Do not trigger Beacon param channel change here
                     * and allow radar action to change channel.
                     */
                    qdf_info("Radar Action in progress. Defer beacon channel change");
                    eht_chan = ieee80211_find_dot11_channel(ic, &eht_chan_p, phymode);
                }

                if (IEEE80211_IS_CHAN_VALID(&eht_chan)) {

                    ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(&eht_chan),
                            eht_chan.ic_freq,
                            eht_chan.ic_vhtop_freq_seg1,
                            eht_chan.ic_vhtop_freq_seg2,
                            eht_chan.op_puncture_bitmap
                            );


                    if (((ochwidth != ni->ni_chwidth) && (ni->ni_chwidth <= vap->iv_sta_negotiated_ch_width))
                            || (opp != ni->ni_ch_param.puncture_bitmap)) {
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE, "eht chan changed, ochwidth:%d ni_chwidth:%d iv_sta_negotiated_ch_width:%d current puncture bitmap:%x  new puncture_bitmap:%x",
                                ochwidth, ni->ni_chwidth, vap->iv_sta_negotiated_ch_width, opp, ni->ni_ch_param.puncture_bitmap);
                    }
                }
            } else {
                eht_chan  = empty_channel;
            }
        }
#endif

        if (((chanie != NULL) || (echanie != NULL)) &&
            IEEE80211_IS_CHAN_VALID(chan) &&
            !IEEE80211_ARE_CHANS_EQUAL(chan, ic->ic_curchan) &&
#ifdef WLAN_FEATURE_11BE
                    !(IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, chan, chan->op_puncture_bitmap))
#else
                    !(IEEE80211_IS_CHAN_RADAR(ic, chan))
#endif
                   ) {
                /* Current code do not handle CSA properly when DOTH is not enabled
                 * In this case, drop CSA to avoid undefined behavior.
                 */
                if ((!(ieee80211_num_beaconing_ap_vaps_up(ic)) || ieee80211_ic_doth_is_set(ic)) &&
                      (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS))
                    ieee80211_process_csa(ni, chan, chanie, echanie, &update_beacon);
#ifdef WLAN_FEATURE_11BE
        } else if (IEEE80211_IS_CHAN_VALID(&eht_chan)) {
            if (ieee80211_ic_rpt_max_phy_is_set(ic)) {
                if (!IEEE80211_ARE_CHANS_EQUAL(&eht_chan, &ni_chan)) {
                    qdf_info("new chan: freq %d cfreq2 %d phymode %d pp %x",
                            eht_chan.ic_freq, eht_chan.ic_vhtop_freq_seg2,
                            ieee80211_chan2mode(&eht_chan), eht_chan.op_puncture_bitmap);

                    qdf_info("ni_chan: freq %d cfreq2 %d phymode %d pp %x",
                            ni_chan.ic_freq, ni_chan.ic_vhtop_freq_seg2,
                            ieee80211_chan2mode(&ni_chan), ni_chan.op_puncture_bitmap);

                    trigger_csa = true;
                }
            } else if (!IEEE80211_ARE_CHANS_EQUAL(&eht_chan, ic->ic_curchan)) {
                qdf_info("new chan: freq %d cfreq2 %d phymode %d pp %x",
                        eht_chan.ic_freq, eht_chan.ic_vhtop_freq_seg2,
                        ieee80211_chan2mode(&eht_chan), eht_chan.op_puncture_bitmap);

                qdf_info("ic_curchan: freq %d cfreq2 %d phymode %d pp %x",
                        ic->ic_curchan->ic_freq, ic->ic_curchan->ic_vhtop_freq_seg2,
                        ieee80211_chan2mode(ic->ic_curchan), ic->ic_curchan->op_puncture_bitmap);

                trigger_csa = true;
            } else {
                ic->ic_curchan->ic_flagext &= ~IEEE80211_CHAN_CSA_RECEIVED;
            }

            if (trigger_csa) {
#ifdef WLAN_FEATURE_11BE_MLO
                /* Disconnect the STA vap if all the links of the MLD is not
                 * connected to the AP.
                 */
                if (!ucfg_mlo_is_mld_connected(vap->vdev_obj)) {
                    vap->iv_ni = ni;
                    qdf_err("vap:%s, Disconnect the STA vap as MLD is not connected", vap->iv_netdev_name);
                    ieee80211_mlme_recv_csa(ni, IEEE80211_RADAR_DETECT_DEFAULT_DELAY, true);
                    return;
                }
#endif

                if ((!(ieee80211_num_apvap_running(ic)) || ieee80211_ic_doth_is_set(ic)) &&
                    (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS))
                    ieee80211_process_beacon_op_channel_change(ni, &eht_chan, &update_beacon);
            }
#endif
        } else {
            ic->ic_curchan->ic_flagext &= ~IEEE80211_CHAN_CSA_RECEIVED;
        }

        if (IEEE80211_NODE_USE_HT(ni) ||
                IEEE80211_NODE_USE_VHT(ni) ||
                IEEE80211_NODE_USE_HE(ni)
#ifdef WLAN_FEATURE_11BE
                || IEEE80211_NODE_USE_EHT(ni)
#endif /* WLAN_FEATURE_11BE */
           ) {

            if (IEEE80211_IS_CHAN_80MHZ(vap->iv_bsschan) || IEEE80211_IS_CHAN_160MHZ(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
                    || IEEE80211_IS_CHAN_320MHZ(vap->iv_bsschan)
#endif
                    ) {
                /* Transitions: 80->40, 80->20, 40->80, 40->20, 20->80, 20->40 */
                if  (((ochwidth != ni->ni_chwidth) && (ni->ni_chwidth <= vap->iv_sta_negotiated_ch_width))
#ifdef WLAN_FEATURE_11BE
                        || (opp != ni->ni_ch_param.puncture_bitmap)
#endif
                        ) {
                    ic->ic_chwidth_change(ni);
                }
            } else if (IEEE80211_IS_CHAN_11N_HT40(vap->iv_bsschan) ||
                    IEEE80211_IS_CHAN_11AC_VHT40(vap->iv_bsschan)||
                    IEEE80211_IS_CHAN_11AX_HE40(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
                    || IEEE80211_IS_CHAN_11BE_EHT40(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
                    ) {

                /* Transitions: 40->20, 20->40 */

                /*
                 * Check for AP changing channel width.
                 * Verify the AP is capable of 40 MHz as sometimes the AP
                 * can get into an inconsistent state (see EV 72018)
                 */
                if ((ni->ni_htcap & IEEE80211_HTCAP_C_CHWIDTH40) &&
                        (ochwidth != ni->ni_chwidth) && (ni->ni_chwidth <= vap->iv_sta_negotiated_ch_width)) {
                    u_int32_t  rxlinkspeed, txlinkspeed; /* bits/sec */
                    ic->ic_chwidth_change(ni);
                    mlme_get_linkrate(ni, &rxlinkspeed, &txlinkspeed);
                    IEEE80211_DELIVER_EVENT_LINK_SPEED(vap, rxlinkspeed, txlinkspeed);
                }
            }
        }

	ieee80211_process_beacon_for_ru26_update(vap, scan_entry);

        if (peer_update_required) {
           /*
            * Based on the HT and VHT capability IEs, update the target with
            * with updated peer info.
            */
           if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
                if (ieee80211_validate_assoc_info(ni) == QDF_STATUS_SUCCESS) {
                    ic->ic_newassoc(ni, 0);
                } else {
                    qdf_rl_err("Not updating assoc parameters since assoc info validation failed");
                }
           }
        }

	if (update_beacon) {
	    wlan_pdev_beacon_update(ic);
        }
    }

    /* Process the Channel switch IEs for partner links*/
    ieee80211_process_partner_link_csa(vap->vdev_obj, scan_entry);

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     * Check BBSID to make sure the received beacon is really from our AP.
     */
    if (ucfg_mlo_is_mld_connected(vap->vdev_obj) && (ni == vap->iv_bss) &&
        (IEEE80211_ADDR_EQ(util_scan_entry_bssid(scan_entry), ieee80211_node_get_bssid(ni)))) {
         /* Process ML Reconfig IE */
         if (scan_entry->ie_list.multi_link_rv) {
             ml_rv_ie = scan_entry->ie_list.multi_link_rv;
             ml_ie_len = MIN_IE_LEN + scan_entry->ie_list.multi_link_rv[TAG_LEN_POS];
         }
         scan_cache_entry = wlan_mlme_get_assoc_sm_scan_entry(vap->vdev_obj);
         if (scan_cache_entry)
             util_scan_get_ml_partner_info(scan_cache_entry, &partner_info);
         mlo_process_ml_reconfig_ie(vap->vdev_obj, scan_entry, ml_rv_ie,
                                    ml_ie_len, &partner_info);
     }
#endif
}

/**
 * check wpa/rsn ie is present in the ie buffer passed in.
 */
bool ieee80211_check_wpaie(struct ieee80211vap *vap, u_int8_t *iebuf, u_int32_t length)
{
    struct wlan_crypto_params tmp_crypto_params;
    bool add_wpa_ie = true;
    u_int8_t *iebuf_end = iebuf + length;

    qdf_mem_zero(&tmp_crypto_params, sizeof(tmp_crypto_params));

    while (add_wpa_ie && ((iebuf+1) < iebuf_end )) {
        if (iebuf[0] == IEEE80211_ELEMID_VENDOR) {
            if (iswpaoui(iebuf) &&  (wlan_crypto_wpaie_check((struct wlan_crypto_params *)&tmp_crypto_params, iebuf) == 0)) {
                /* found WPA IE */
                add_wpa_ie = false;
            }
        } else if (iebuf[0] == IEEE80211_ELEMID_RSN) {
            if (wlan_crypto_rsnie_check((struct wlan_crypto_params *)&tmp_crypto_params, iebuf) == 0) {
                /* found RSN IE */
                add_wpa_ie = false;
            }
        }
        iebuf += iebuf[1] + 2;
        continue;
    }

    if (!add_wpa_ie) {
        struct wlan_crypto_params *vdev_crypto_params;
        vdev_crypto_params = wlan_crypto_vdev_get_crypto_params(vap->vdev_obj);
        if (!vdev_crypto_params)
            qdf_mem_copy(vdev_crypto_params, &tmp_crypto_params, sizeof(struct wlan_crypto_params));
    }
    return add_wpa_ie;
}

#if QCN_IE
static void
ieee80211_get_qcn_bsscolor_rept_info(struct ieee80211vap *vap)
{
    ol_ath_soc_softc_t *soc = NULL;
    struct wlan_objmgr_psoc_objmgr *psoc_objmgr = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211com *ic = NULL,*tmpic = NULL;
    struct ieee80211vap *tmp_vap = NULL;
    int soc_idx;
    bool apvap_on_other_radio = 0;
    bool apvap_on_same_radio = 0;
    int id =0;

    ic = vap->iv_ic;
    for (soc_idx = 0; soc_idx < ol_num_global_soc; soc_idx++) {
        soc = ol_global_soc[soc_idx];
        if (!soc || !(ic->ic_is_target_lithium(soc->psoc_obj))) {
            continue;
        }
        psoc_objmgr = &soc->psoc_obj->soc_objmgr;
        for (id=0;id<WLAN_UMAC_MAX_PDEVS;id++) {
            pdev = psoc_objmgr->wlan_pdev_list[id];
            if (pdev) {
                tmpic = wlan_pdev_get_mlme_ext_obj(pdev);
                if (tmpic) {
                    TAILQ_FOREACH(tmp_vap, &tmpic->ic_vaps, iv_next) {
                        if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) {
                            if (ic == tmpic) {
                                apvap_on_same_radio = 1;
                            } else {
                                apvap_on_other_radio = 1;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    if (apvap_on_same_radio && ic->ic_rept_clients) {
        vap->iv_bsscolor_rptr_info = QCN_REPT_BH_APVAP_ON_SAME_RADIO;
    } else if (apvap_on_same_radio && !ic->ic_rept_clients) {
        vap->iv_bsscolor_rptr_info = QCN_REPT_FH_APVAP_ON_SAME_RADIO;
    } else if (apvap_on_other_radio) {
        vap->iv_bsscolor_rptr_info = QCN_REPT_APVAP_ON_OTHER_RADIO;
    } else {
        vap->iv_bsscolor_rptr_info = QCN_REPT_ONLY_STAVAP;
    }
    return;
}
#endif

#if defined(WLAN_FEATURE_11BE) && defined(WLAN_FEATURE_11BE_MLO)
struct ieee80211vap *ieee80211_assoc_get_t2lm_test_cmd_vap(
        struct ieee80211_node *ni)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211vap *co_mld_vap = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t vdev_count = 0;
    uint16_t i = 0;

    if (ni->ni_vap->t2lm_test_cmd) {
        t2lm_debug("t2lm_test_cmd is set for vap:%s", ni->ni_vap->iv_netdev_name);
        return ni->ni_vap;
    }

    vdev = wlan_peer_get_vdev(ni->peer_obj);
    if (!vdev) {
        t2lm_err("null vdev");
        return NULL;
    }

    mlo_get_ml_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    for (i = 0; i < vdev_count; i++) {
        co_mld_vdev = wlan_vdev_list[i];
        if (!co_mld_vdev) {
            t2lm_err("co_mld_vdev is null");
            break;
        }

        co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
        if (!co_mld_vap) {
            t2lm_err("co_mld_vap is null");
            break;
        }

        if (co_mld_vap->t2lm_test_cmd) {
            t2lm_debug("t2lm_test_cmd is set for vap:%s", co_mld_vap->iv_netdev_name);
            vap = co_mld_vap;
            break;
        }
    }

    if (vdev_count) {
        if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list),
                    vdev_count, 0) != QDF_STATUS_SUCCESS)
            qdf_err("failed to release mlo vdev ref");
    }

    return vap;
}
#else
struct ieee80211vap *ieee80211_assoc_get_t2lm_test_cmd_vap(
        struct ieee80211_node *ni)
{
    return NULL;
}
#endif /* defined(WLAN_FEATURE_11BE) && defined(WLAN_FEATURE_11BE_MLO) */

/*
 * Setup an association/reassociation request,
 * and returns the frame length.
 */
static u_int16_t
ieee80211_setup_assoc(
    struct ieee80211_node *ni,
    struct ieee80211_frame *wh,
    int reassoc,
    u_int8_t *previous_bssid,
    uint32_t frm_len)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    u_int8_t *frm;
    u_int16_t capinfo = 0;
    struct ieee80211_bwnss_map nssmap;
    u_int16_t ie_len;
    u_int8_t subtype = reassoc ? IEEE80211_FC0_SUBTYPE_REASSOC_REQ :
        IEEE80211_FC0_SUBTYPE_ASSOC_REQ;
    bool  add_wpa_ie;
    u_int8_t rx_chainmask = ieee80211com_get_rx_chainmask(ic);
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t *ml_ie_pos_ptr = NULL, *ml_ie_start_ptr = NULL, *ml_ie_end_ptr = NULL;
    size_t curr_frmlen = 0;
#endif
#ifdef WLAN_FEATURE_11BE
    uint8_t t2lm_enable = 0;
    struct ieee80211vap *tmp_vap = NULL;
    struct wlan_t2lm_onging_negotiation_info *ongoing_t2lm = NULL;
    uint8_t dir = 0;
#endif /* WLAN_FEATURE_11BE */
    int32_t authmode = 0;

    qdf_mem_zero(&nssmap, sizeof(nssmap));

    ieee80211_send_setup(vap, ni, wh,
                         (IEEE80211_FC0_TYPE_MGT | subtype),
                         vap->iv_myaddr, ni->ni_macaddr, ni->ni_bssid);
    frm = (u_int8_t *)&wh[1];

    /*
     * asreq frame format
     *[2] capability information
     *[2] listen interval
     *[6*] current AP address (reassoc only)
     *[tlv] ssid
     *[tlv] supported rates
     *[4] power capability
     *[28] supported channels element
     *[tlv] extended supported rates
     *[tlv] WME [if enabled and AP capable]
     *[tlv] HT Capabilities
     *[tlv] VHT Capabilities
     *[tlv] Atheros advanced capabilities
     *[tlv] user-specified ie's
     *[tlv] Bandwidth-NSS mapping
     */
    if (vap->iv_opmode == IEEE80211_M_STA || vap->iv_opmode == IEEE80211_M_BTAMP)
        capinfo |= IEEE80211_CAPINFO_ESS;
    else
        ASSERT(0);

    if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap))
        capinfo |= IEEE80211_CAPINFO_PRIVACY;
    /*
     * NB: Some 11a AP's reject the request when
     *     short premable is set.
     */
    if ((ic->ic_flags & IEEE80211_F_SHPREAMBLE) &&
        IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan))
        capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;
    if ((ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME) &&
        (ic->ic_flags & IEEE80211_F_SHSLOT))
        capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;
    if (ieee80211_ic_doth_is_set(ic) && ieee80211_vap_doth_is_set(vap))
        capinfo |= IEEE80211_CAPINFO_SPECTRUM_MGMT;
    if (ieee80211_vap_rrm_is_set(vap))
        capinfo |= IEEE80211_CAPINFO_RADIOMEAS;

    *(u_int16_t *)frm = htole16(capinfo);
    frm += 2;

    *(u_int16_t *)frm = htole16(ic->ic_lintval);
    frm += 2;

    if (reassoc) {
        IEEE80211_ADDR_COPY(frm, previous_bssid);
        frm += QDF_MAC_ADDR_SIZE;
    }

    frm = ieee80211_add_ssid(frm,
                             ni->ni_essid,
                             ni->ni_esslen);
    frm = ieee80211_add_rates(vap, frm, &ni->ni_rates);

    /*
     * DOTH elements
     */
    if (ieee80211_ic_doth_is_set(ic) && ieee80211_vap_doth_is_set(vap)) {
        frm = ieee80211_add_doth(frm, vap);
    }

    if (wlan_rptr_vdev_is_wds(vap->vdev_obj)) {
        ieee80211_add_or_retrieve_ie_from_app_opt_ies(vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
                                                      IEEE80211_ELEMID_VENDOR, IEEE80211_ELEMID_VENDOR_SON_REPT,
                                                      &frm, TYPE_APP_IE_BUF, NULL, true);
    }
    add_wpa_ie=true;
    /*
     * check if os shim has setup RSN IE it self.
     */
    IEEE80211_VAP_LOCK(vap);
    if (vap->iv_opt_ie.length) {
        add_wpa_ie = ieee80211_check_wpaie(vap, vap->iv_opt_ie.ie,
                                           vap->iv_opt_ie.length);
    }
    if (add_wpa_ie) {
        /* Continue looking for the WPA IE */
        add_wpa_ie = ieee80211_mlme_app_ie_check_wpaie(vap,IEEE80211_FRAME_TYPE_ASSOCREQ);
    }
    IEEE80211_VAP_UNLOCK(vap);

    if (add_wpa_ie) {
        if (wlan_crypto_vdev_has_auth_mode(vap->vdev_obj, (1 << WLAN_CRYPTO_AUTH_RSNA))) {
            frm = wlan_crypto_build_rsnie(vap->vdev_obj, frm, (struct qdf_mac_addr*)ni->ni_bssid);
            if (!frm) {
                return 0;
            }
        }
    }

    /*Add RM Enabled capabilities IE*/
    if (vap->iv_rrm_cap_ie)
        frm = ieee80211_add_rrm_cap_ie(frm, ni);

#if ATH_SUPPORT_WAPI
    if (wlan_crypto_vdev_has_auth_mode(vap->vdev_obj, (1 << WLAN_CRYPTO_AUTH_WAPI)))
    {
        frm = ieee80211_setup_wapi_ie(vap, frm);
        if (!frm) {
            return 0;
        }
    }
#endif

    frm = ieee80211_add_xrates(vap, frm, &ni->ni_rates);
	if (!reassoc) {
	    wlan_set_tspecActive(vap, 0);
	}

    if (vap->iv_ccx_evtable && vap->iv_ccx_evtable->wlan_ccx_set_vperf) {
        vap->iv_ccx_evtable->wlan_ccx_set_vperf(vap->iv_ccx_arg, 0);
    }


    /*
     * XXX: don't do WMM association with safe mode enabled, because
     * Vista OS doesn't know how to decrypt QoS frame.
     */
    if (ieee80211_vap_wme_is_set(vap) &&
        (ni->ni_ext_caps & IEEE80211_NODE_C_QOS) &&
        !IEEE80211_VAP_IS_SAFEMODE_ENABLED(vap)) {
        struct ieee80211_wme_tspec *sigtspec = &ni->ni_ic->ic_sigtspec;
        struct ieee80211_wme_tspec *datatspec = &ni->ni_ic->ic_datatspec;
        u_int8_t    tsrsiev[16];
        u_int8_t    tsrsvlen = 0;
        u_int32_t   minphyrate;

        frm = ieee80211_add_wmeinfo(frm, ni, WME_INFO_OUI_SUBTYPE, NULL, 0);
        if (iswmetspec((u_int8_t *)sigtspec)) {
            frm = ieee80211_add_wmeinfo(frm, ni, WME_TSPEC_OUI_SUBTYPE,
                                        &sigtspec->ts_tsinfo[0],
                                        sizeof (struct ieee80211_wme_tspec) - offsetof(struct ieee80211_wme_tspec, ts_tsinfo));
#if AH_UNALIGNED_SUPPORTED
            minphyrate = __get32(&sigtspec->ts_min_phy[0]);
#else
            minphyrate = *((u_int32_t *) &sigtspec->ts_min_phy[0]);
#endif
            if (vap->iv_ccx_evtable && vap->iv_ccx_evtable->wlan_ccx_fill_tsrsie) {
                vap->iv_ccx_evtable->wlan_ccx_fill_tsrsie(vap->iv_ccx_arg,
                                ((struct ieee80211_tsinfo_bitmap *) &sigtspec->ts_tsinfo[0])->tid,
                                minphyrate, &tsrsiev[0], &tsrsvlen);
            }
            if (tsrsvlen > 0) {
                *frm++ = IEEE80211_ELEMID_VENDOR;
                *frm++ = tsrsvlen;
                qdf_mem_copy(frm, &tsrsiev[0], tsrsvlen);
                frm += tsrsvlen;
            }
        }
        if (iswmetspec((u_int8_t *)datatspec)){
            frm = ieee80211_add_wmeinfo(frm, ni, WME_TSPEC_OUI_SUBTYPE,
                                        &datatspec->ts_tsinfo[0],
                                        sizeof (struct ieee80211_wme_tspec) - offsetof(struct ieee80211_wme_tspec, ts_tsinfo));
#if AH_UNALIGNED_SUPPORTED
            minphyrate = __get32(&datatspec->ts_min_phy[0]);
#else
            minphyrate = *((u_int32_t *) &datatspec->ts_min_phy[0]);
#endif
            if (vap->iv_ccx_evtable && vap->iv_ccx_evtable->wlan_ccx_fill_tsrsie) {
                vap->iv_ccx_evtable->wlan_ccx_fill_tsrsie(vap->iv_ccx_arg,
                                ((struct ieee80211_tsinfo_bitmap *) &datatspec->ts_tsinfo[0])->tid,
                                minphyrate, &tsrsiev[0], &tsrsvlen);
            }
            if (tsrsvlen > 0) {
                *frm++ = IEEE80211_ELEMID_VENDOR;
                *frm++ = tsrsvlen;
                qdf_mem_copy(frm, &tsrsiev[0], tsrsvlen);
                frm += tsrsvlen;
            }
    }
    }

#if UMAC_SUPPORT_WNM
    if (ieee80211_vap_wnm_is_set(vap)) {
        frm = ieee80211_add_timreq_ie(vap, ni, frm);
    }
#endif

    if ((ni->ni_flags & IEEE80211_NODE_HT) &&
        (!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) &&
        (IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan) ||
         IEEE80211_IS_CHAN_11AC(ic->ic_curchan) ||
         IEEE80211_IS_CHAN_11N(ic->ic_curchan)) && ieee80211vap_htallowed(vap)) {
        frm = ieee80211_add_htcap(frm, ni, subtype);

        if (IEEE80211_IS_HTVIE_ENABLED(ic)) {
            frm = ieee80211_add_htcap_vendor_specific(frm, ni, subtype);
        }
    }

    /* Add extended capbabilities, if applicable */
    frm = ieee80211_add_extcap(frm, ni, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);

    /*
     * VHT IEs
     */
      /* Add vht cap for 2.4G mode, if 256QAM is enabled */
    if ((IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan) ||
         IEEE80211_IS_CHAN_11AC(ic->ic_curchan) ||
         IEEE80211_IS_CHAN_11NG(ic->ic_curchan))) {

        if(!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan) &&
                ieee80211vap_vhtallowed(vap)) {
            /* Add VHT capabilities IE */
            frm = ieee80211_add_vhtcap(frm, ni, ic,
                                IEEE80211_FC0_SUBTYPE_ASSOC_REQ, NULL, NULL);
            if (!frm)
                return 0;
        }

        /* Add optional VHT opmode notify if sta is configured to operate on VHT40/VHT20 */
        switch(vap->iv_des_mode) {
            case IEEE80211_MODE_11AC_VHT20:
            case IEEE80211_MODE_11AC_VHT40PLUS:
            case IEEE80211_MODE_11AC_VHT40MINUS:
            case IEEE80211_MODE_11AC_VHT40:
            case IEEE80211_MODE_11AXA_HE20:
            case IEEE80211_MODE_11AXA_HE40PLUS:
            case IEEE80211_MODE_11AXA_HE40MINUS:
            case IEEE80211_MODE_11AXA_HE40:
#ifdef WLAN_FEATURE_11BE
            case IEEE80211_MODE_11BEA_EHT20:
            case IEEE80211_MODE_11BEA_EHT40PLUS:
            case IEEE80211_MODE_11BEA_EHT40MINUS:
            case IEEE80211_MODE_11BEA_EHT40:
#endif
                frm = ieee80211_add_opmode_notify(frm, ni, ic, subtype);
                break;

            default:
                /* Not adding opmode notify IE */
                break;
        }
    }

    /*
     * VHT Interop IEs for 2.4NG mode, if 256QAM & 11NG vht interop is enabled
     */
    if (IEEE80211_IS_CHAN_11NG(ic->ic_curchan) && ieee80211vap_vhtallowed(vap)
                        && ieee80211vap_11ng_vht_interopallowed(vap)) {
        /* Add Vendor specific VHT Interop IE with Vht cap & Vht op IE*/
        frm = ieee80211_add_interop_vhtcap(frm, ni, ic, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);
    }

    if (ieee80211_vap_wme_is_set(vap) &&  IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan)
         && ieee80211vap_heallowed(vap)) {

        /* Add HE Capabilities IE */
        frm = ieee80211_add_hecap(frm, ni, ic, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);

        if(IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
            /* Add HE 6GHz Band Capbilities IE */
            frm = ieee80211_add_6g_bandcap(frm, ni, ic,
                            IEEE80211_FC0_SUBTYPE_ASSOC_REQ);
        }

    }

#ifdef WLAN_FEATURE_11BE
    if (ieee80211_vap_wme_is_set(vap) &&  IEEE80211_IS_CHAN_11BE(ic->ic_curchan)
         && ieee80211vap_ehtallowed(vap)) {
        if (mlo_is_mld_sta(vap->vdev_obj) &&
                !wlan_vdev_mlme_is_mlo_link_vdev(vap->vdev_obj)) {
            ml_ie_pos_ptr = frm;
        }

        /* Add EHT Capabilities IE */
        frm = ieee80211_add_ehtcap(frm, ni, ic, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);
    }
#endif /* WLAN_FEATURE_11BE */

    authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
    if ( authmode == -1 ) {
        qdf_err("crypto_err while getting authmode params\n");
        return -1;
    }

#ifdef WLAN_FEATURE_11BE
    ieee80211_get_t2lm_enable(ic, &t2lm_enable);

    if ((t2lm_enable == WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) &&
            wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {

        tmp_vap = ieee80211_assoc_get_t2lm_test_cmd_vap(ni);
        /* If T2LM test command is issued, then use the TID-to-link mapping from
         * vap structure for the given MLD peer.
         */
        if (tmp_vap && tmp_vap->t2lm_test_cmd &&
            !qdf_mem_cmp(tmp_vap->t2lm_peer_mld_mac,
                ni->peer_obj->mlo_peer_ctx->peer_mld_addr.bytes,
                QDF_MAC_ADDR_SIZE) &&
            ieee80211_ucfg_is_valid_t2lm_link_map(tmp_vap) &&
            ((authmode & (1 << WLAN_CRYPTO_AUTH_OPEN)) ||
             ieee80211_is_tids_mapped_to_assoc_link(vap,
                 &tmp_vap->t2lm_negotiation))) {
            if (tmp_vap->t2lm_negotiation.category ==
                    WLAN_T2LM_CATEGORY_REQUEST) {
                frm = ieee80211_add_t2lm_ie(frm, ni,
                        &tmp_vap->t2lm_negotiation);
                if (!frm) {
                    return 0;
                }
                wlan_copy_configured_t2lm_to_ml_peer_ctx(
                        ni->peer_obj->mlo_peer_ctx,
                        &tmp_vap->t2lm_negotiation, 0);
            }
        } else {
            ongoing_t2lm =
                &ni->peer_obj->mlo_peer_ctx->t2lm_policy.ongoing_tid_to_link_mapping;
            /* Initialize the direction to invalid */
            for(dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
                ongoing_t2lm->t2lm_info[dir].direction =
                    WLAN_T2LM_INVALID_DIRECTION;
            }

            /* Set the default mapping to BIDI */
            ongoing_t2lm->t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction =
                WLAN_T2LM_BIDI_DIRECTION;
            ongoing_t2lm->t2lm_info[WLAN_T2LM_BIDI_DIRECTION].default_link_mapping =
                true;
        }
    }
#endif /* WLAN_FEATURE_11BE */

    /* Insert ieee80211_ie_ath_extcap IE to beacon */
    if ((ni->ni_flags & IEEE80211_NODE_ATH) &&
        ic->ic_ath_extcap &&
        (vap->iv_ena_vendor_ie & IEEE80211_ENABLE_ATH_EXTCAP_VENDOR_IE)) {
        u_int16_t ath_extcap = 0;
        u_int8_t  ath_rxdelim = 0;

        if (ieee80211_has_weptkipaggr(ni)) {
                ath_extcap |= IEEE80211_ATHEC_WEPTKIPAGGR;
                ath_rxdelim = ic->ic_weptkipaggr_rxdelim;
        }

        if ((ni->ni_flags & IEEE80211_NODE_OWL_WDSWAR) &&
            (ic->ic_ath_extcap & IEEE80211_ATHEC_OWLWDSWAR)) {
                ath_extcap |= IEEE80211_ATHEC_OWLWDSWAR;
        }

        if (ieee80211com_has_extradelimwar(ic))
            ath_extcap |= IEEE80211_ATHEC_EXTRADELIMWAR;

        frm = ieee80211_add_athextcap(frm, ath_extcap, ath_rxdelim);
    }

    /*
     * add wpa/rsn ie if os did not setup one.
     */
    if (add_wpa_ie) {
        if (authmode & (1 << WLAN_CRYPTO_AUTH_WPA)) {
            frm = wlan_crypto_build_wpaie(vap->vdev_obj, frm);
            if (!frm) {
                return 0;
            }
        }

        if (authmode & (1 << WLAN_CRYPTO_AUTH_CCKM)) {
            /*
             * CCKM AKM can be either added to WPA IE or RSN IE,
             * depending on the AP's configuration
             */
            if (authmode & (1 << WLAN_CRYPTO_AUTH_RSNA))
                frm = wlan_crypto_build_rsnie(vap->vdev_obj, frm, (struct qdf_mac_addr*)ni->ni_bssid);
            else if (authmode & (1 << WLAN_CRYPTO_AUTH_WPA))
                frm = wlan_crypto_build_wpaie(vap->vdev_obj, frm);

            if (!frm) {
                return 0;
            }
        }
    }

    if (IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan) &&
        ieee80211_is_phymode_160(vap->iv_sta_max_ch_cap ?
                vap->iv_des_mode : vap->iv_cur_mode) &&
        !(vap->iv_ext_nss_support) && !(ic->ic_disable_bwnss_adv) &&
        !ieee80211_get_bw_nss_mapping(vap, &nssmap, rx_chainmask) &&
        (nssmap.flag != IEEE80211_NSSMAP_SAME_NSS_FOR_ALL_BW)) {
        frm = ieee80211_add_bw_nss_maping(frm, &nssmap);
    }

    IEEE80211_VAP_LOCK(vap);

    if (!reassoc) {
        if (vap->iv_roam.iv_ft_enable) {
            if(vap->iv_roam.iv_mdie[1]) {
                qdf_mem_copy(frm, vap->iv_roam.iv_mdie, MD_IE_LEN);
                frm += MD_IE_LEN;
            }
        }
    }

    if (vap->iv_opt_ie.length){
        if(!vap->iv_roam.iv_ft_roam) {
            OS_MEMCPY(frm, vap->iv_opt_ie.ie,
                      vap->iv_opt_ie.length);
            frm += vap->iv_opt_ie.length;
        }
    }

    /* Add the Application IE's */
    frm = ieee80211_mlme_app_ie_append(vap, IEEE80211_FRAME_TYPE_ASSOCREQ, frm);

    /* Add SON IE */
    if (wlan_rptr_vdev_is_wds(vap->vdev_obj)) {
        ie_len = __ieee80211_add_or_retrieve_ie_from_app_opt_ies(vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
                                                               IEEE80211_ELEMID_VENDOR, IEEE80211_ELEMID_VENDOR_SON_AP,
                                                               &frm, TYPE_APP_IE_BUF, true);
    }
    IEEE80211_VAP_UNLOCK(vap);
#if REPEATER_SAME_SSID
    if (wlan_rptr_global_is_s_ssid() && (vap == ic->ic_sta_vap)) {
        /* Add the Extender IE */
        frm = wlan_rptr_add_extender_ie(vap->vdev_obj, IEEE80211_FRAME_TYPE_ASSOCREQ, frm);
    }
#endif

#if QCN_IE
    if ((vap == ic->ic_sta_vap) && wlan_rptr_vdev_is_wds(vap->vdev_obj)) {
        ieee80211_get_qcn_bsscolor_rept_info(vap);
    }
    frm = ieee80211_add_qcn_info_ie(frm, vap, &ie_len,
            QCN_MAC_PHY_PARAM_IE_TYPE, NULL);
    if (vap->iv_bsscolor_rptr_info) {
        /*clear the flag, to avoid adding bsscolor attribute on other frames*/
        vap->iv_bsscolor_rptr_info = 0;
    }
#endif

    frm = ieee80211_add_generic_vendor_capabilities_ie(frm, ic);
    if (!frm)
        return 0;

#ifdef WLAN_FEATURE_11BE_MLO
    if (ieee80211_vap_wme_is_set(vap) &&
            IEEE80211_IS_CHAN_11BE(ic->ic_curchan) &&
            ieee80211vap_ehtallowed(vap) &&
            mlo_is_mld_sta(vap->vdev_obj) &&
            !wlan_vdev_mlme_is_mlo_link_vdev(vap->vdev_obj)) {
        mloie_debug("Adding ML IE for assoc req");

        curr_frmlen =  frm - (u_int8_t *)wh;

        ml_ie_start_ptr = qdf_mem_malloc(frm_len - curr_frmlen);
        if (!ml_ie_start_ptr) {
            qdf_err("Unable to allocate memory for Multi-Link IE");
            return 0;
        }

        if ((ml_ie_end_ptr = ieee80211_add_multilink_ie(
                            ml_ie_start_ptr,
                            frm_len - curr_frmlen,
                            (u_int8_t *)wh,
                            curr_frmlen,
                            ni, ic, subtype,
                            false, 0,
                            NULL, NULL, NULL)) != NULL) {
        } else {
           qdf_err("Failed to add Multi-Link IE");
           return 0;
        }

        /*
         * Since the multi-link IE depends on the formation of the other
         * IEs within the assoc response frame, it is required to generate the
         * IE contents at the end. However, memory operations need to be
         * done to move the ML IE to the standard position within the assoc
         * response.
         */
        if (ml_ie_pos_ptr && ml_ie_start_ptr && ml_ie_end_ptr &&
                (ml_ie_end_ptr > ml_ie_start_ptr)) {

            uint16_t reposition_len = frm - ml_ie_pos_ptr;
            uint16_t ml_ie_len = ml_ie_end_ptr - ml_ie_start_ptr;

            qdf_mem_move(ml_ie_pos_ptr + ml_ie_len, ml_ie_pos_ptr, reposition_len);

            qdf_mem_copy(ml_ie_pos_ptr, ml_ie_start_ptr, ml_ie_len);

            qdf_mem_free(ml_ie_start_ptr);

            frm += ml_ie_len;
        } else {
            qdf_err("Could not reposition ML IE %p %p %p", ml_ie_pos_ptr, ml_ie_start_ptr, ml_ie_end_ptr);
        }
    }
#endif /* WLAN_FEATURE_11BE_MLO */

    return (frm - (u_int8_t *)wh);
}

#ifdef WLAN_FEATURE_11BE_MLO
static
uint16_t ieee80211_mlo_setup_link_assoc(struct ieee80211_node *ni, wbuf_t *wbuf)
{
    struct ieee80211com *ic = NULL;
    uint16_t length = 0;

    if (!ni)
        return 0;

    ic = ni->ni_ic;

    *wbuf = wbuf_alloc(ic->ic_osdev, WBUF_TX_MGMT, MAX_TX_RX_PACKET_SIZE);
    if (*wbuf == NULL)
        return 0;

    length = ieee80211_setup_assoc(ni, (struct ieee80211_frame *)wbuf_header(*wbuf),
                                   0, NULL, wbuf_get_len(*wbuf));
    if (!length) {
        wbuf_release(ic->ic_osdev, *wbuf);
        *wbuf = NULL;
        return 0;
    }

    wbuf_set_pktlen(*wbuf, length);

    return length;
}

struct scan_cache_entry *
ieee80211_get_scan_entry_by_ssid_and_bssid(struct wlan_objmgr_vdev *vdev,
                                  struct wlan_ssid ssid,
                                  struct qdf_mac_addr *bssid)
{
    struct scan_filter *scan_filter;
    qdf_list_t *list = NULL;
    struct scan_cache_node *first_node = NULL;
    qdf_list_node_t *cur_node = NULL;
    struct scan_cache_entry *entry = NULL, *scan_entry = NULL;

    if (!vdev)
        return NULL;

    scan_filter = qdf_mem_malloc(sizeof(*scan_filter));
    if (!scan_filter)
        return NULL;

    scan_filter->num_of_ssid = 1;
    scan_filter->num_of_bssid = 1;

    qdf_mem_copy(&scan_filter->ssid_list[0], &ssid, sizeof(ssid));
    qdf_mem_copy(scan_filter->bssid_list[0].bytes,
                 bssid, sizeof(struct qdf_mac_addr));

    list = wlan_scan_get_result(wlan_vdev_get_pdev(vdev), scan_filter);
    qdf_mem_free(scan_filter);

    if (!list || (!qdf_list_size(list))) {
        mlo_debug("scan list empty");
        goto error;
    }

    qdf_list_peek_front(list, &cur_node);
    first_node = qdf_container_of(cur_node, struct scan_cache_node,
                                  node);
    if (first_node && first_node->entry) {
        entry = first_node->entry;
        scan_entry = util_scan_copy_cache_entry(entry);
     }
error:
    if (list)
        wlan_scan_purge_results(list);

    return scan_entry;
}

qdf_nbuf_t ieee80211_mlo_get_link_assoc(struct wlan_objmgr_peer *peer,
                                        uint8_t link_ix)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap *vap = NULL;
    struct mlo_partner_info partner_info = {0};
    uint8_t i = 0, link_id;
    struct wlan_objmgr_vdev *link_vdev = NULL;
    uint16_t length = 0;
    struct qdf_mac_addr link_addr;
    struct scan_cache_entry *link_scan_entry = NULL, *active_scan_entry = NULL;
    struct ieee80211_node *tmp_ni = NULL;
    struct ieee80211vap *link_vap = NULL;
    qdf_nbuf_t assocbuf = NULL;
    struct chan_params chan_p = {0};
    struct ieee80211_ath_channel ni_chan, ic_chan;
    uint8_t idx = 0;

    if (!peer)
        return NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)
        return NULL;

    vap = ni->ni_vap;

    active_scan_entry = mlme_cm_get_active_scan_entry(vap);
    if (!active_scan_entry)
        return NULL;

    util_scan_get_ml_partner_info(active_scan_entry, &partner_info);

    if (!partner_info.num_partner_links) {
        qdf_err("No partner links found in scan entry");
        return NULL;
    }

    for (i = 0; i < partner_info.num_partner_links; i++) {
         link_id = partner_info.partner_link_info[i].link_id;
         if (link_id != link_ix) {
             qdf_err("link id mismatch, requested %d found %d", link_ix, link_id);
             continue;
         }

         qdf_mem_copy(&link_addr, &partner_info.partner_link_info[i].link_addr,
                      QDF_MAC_ADDR_SIZE);
         link_vdev = mlo_get_vdev_by_link_id(vap->vdev_obj, link_id,
                                             WLAN_MLO_MGR_ID);
         if (!link_vdev) {
             qdf_err("links vdev not found for link id %d", link_id);
             continue;
         }

         idx = mlo_get_link_vdev_ix(link_vdev->mlo_dev_ctx, link_vdev);
         if (idx >= WLAN_UMAC_MLO_MAX_VDEVS) {
             qdf_err("link vdev index not found in ml dev list");
             mlo_release_vdev_ref(link_vdev);
             goto error;
         }

         if (qdf_test_bit(
                 idx,
                 link_vdev->mlo_dev_ctx->sta_ctx->wlan_connect_req_links)) {
             link_scan_entry = ieee80211_get_scan_entry_by_ssid_and_bssid(
                     link_vdev, active_scan_entry->ssid, &link_addr);
             if (!link_scan_entry) {
                 qdf_err("no link scan entry found");
                 mlo_release_vdev_ref(link_vdev);
                 goto error;
             }

             link_vap = wlan_vdev_mlme_get_ext_hdl(link_vdev);
             if (!link_vap) {
                 util_scan_free_cache_entry(link_scan_entry);
                 mlo_release_vdev_ref(link_vdev);
                 goto error;
             }

             tmp_ni = ieee80211_mlo_tmp_node(link_vap, link_addr.bytes);
             if (!tmp_ni) {
                 util_scan_free_cache_entry(link_scan_entry);
                 mlo_release_vdev_ref(link_vdev);
                 qdf_err("failed to created mlo temp node");
                 goto error;
             }

             ieee80211_setup_node(tmp_ni, link_scan_entry);
             ieee80211_set_ctry_code_continue(link_vap->iv_ic, false);
             chan_p.freq = tmp_ni->ni_ch_param.ni_freq;
             chan_p.cfreq2 = tmp_ni->ni_ch_param.ni_cfreq2;
             set_chan_params_puncture_bitmap(&chan_p, tmp_ni->ni_ch_param.puncture_bitmap);
             ni_chan = ieee80211_find_dot11_channel(link_vap->iv_ic, &chan_p,
                                                    tmp_ni->ni_ch_param.ni_bss_phymode);
             ieee80211_update_vdev_chan(link_vap->vdev_obj->vdev_mlme.des_chan, &ni_chan);
             ic_chan = *link_vap->iv_ic->ic_curchan;
             *link_vap->iv_ic->ic_curchan = ni_chan;
             *link_vap->iv_bsschan = *link_vap->iv_ic->ic_curchan;

             /* Caller should free the wbuf which is allocated inside this API
              * if this API return success
              */

             length = ieee80211_mlo_setup_link_assoc(tmp_ni, &assocbuf);
             *link_vap->iv_ic->ic_curchan = ic_chan;
             if (!length) {
                 wlan_objmgr_delete_node(tmp_ni);
                 util_scan_free_cache_entry(link_scan_entry);
                 mlo_release_vdev_ref(link_vdev);
                 qdf_err("link assoc setup failed");
                 goto error;
             } else {
                 qdf_err("link assoc len %d", length);
             }

             wlan_objmgr_delete_node(tmp_ni);
             util_scan_free_cache_entry(link_scan_entry);
         }
         mlo_release_vdev_ref(link_vdev);
         return assocbuf;
    }

error:
    return NULL;
}
#endif

/*
 * Send an assoc/reassoc request frame
 */
int
ieee80211_send_assoc(struct ieee80211_node *ni,
                     int reassoc, u_int8_t *prev_bssid)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    wbuf_t wbuf;
    u_int16_t length;

    wbuf = wbuf_alloc(ic->ic_osdev, WBUF_TX_MGMT, MAX_TX_RX_PACKET_SIZE);
    if (wbuf == NULL)
        return -ENOMEM;

    length = ieee80211_setup_assoc(ni, (struct ieee80211_frame *)wbuf_header(wbuf),
                                   reassoc, prev_bssid, wbuf_get_len(wbuf));
    if (!length) {
        wbuf_release(ic->ic_osdev, wbuf);
        return -EINVAL;
    }

    wbuf_set_pktlen(wbuf, length);
    /* Callback to allow OS layer to copy assoc/reassoc frame (Vista requirement) */
    IEEE80211_DELIVER_EVENT_MLME_ASSOC_REQ(vap, wbuf);

    return ieee80211_send_mgmt(vap,ni,wbuf,false);
}

int
wlan_send_addts(wlan_if_t vaphandle, u_int8_t *macaddr, ieee80211_tspec_info *tsinfo)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node *ni;
    struct ieee80211_action_mgt_args addts_args;
    struct ieee80211_action_mgt_buf  addts_buf;
    struct ieee80211_tsinfo_bitmap *tsflags;
    struct ieee80211_wme_tspec *tspec;

    ni = ieee80211_find_txnode(vap, macaddr, WLAN_MGMT_HANDLER_ID);
    if (ni == NULL) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_OUTPUT,
                          "%s: could not send ADDTS, no node found for %s\n",
                          __func__, ether_sprintf(macaddr));
        return -EINVAL;
    }

    /*
     * ieee80211_action_mgt_args is a generic structure. TSPEC IE
     * is filled in the buf area.
     */
    addts_args.category = IEEE80211_ACTION_CAT_WMM_QOS;
    addts_args.action   = IEEE80211_WMM_QOS_ACTION_SETUP_REQ;
    addts_args.arg1     = IEEE80211_WMM_QOS_DIALOG_SETUP; /* dialogtoken */
    addts_args.arg2     = 0; /* status code */
    addts_args.arg3     = sizeof(struct ieee80211_wme_tspec);
    tspec = (struct ieee80211_wme_tspec *) &addts_buf.buf;
    tsflags = (struct ieee80211_tsinfo_bitmap *) &(tspec->ts_tsinfo);
    tsflags->direction = tsinfo->direction;
    tsflags->psb = tsinfo->psb;
    tsflags->dot1Dtag = tsinfo->dot1Dtag;
    tsflags->tid = tsinfo->tid;
    tsflags->reserved3 = tsinfo->aggregation;
    tsflags->one = tsinfo->acc_policy_edca;
    tsflags->zero = tsinfo->acc_policy_hcca;
    tsflags->reserved1 = tsinfo->traffic_type;
    tsflags->reserved2 = tsinfo->ack_policy;
#if AH_UNALIGNED_SUPPORTED
    __put16(&tspec->ts_nom_msdu[0], tsinfo->norminal_msdu_size);
    __put16(&tspec->ts_max_msdu[0], tsinfo->max_msdu_size);
    __put32(&tspec->ts_min_svc[0], tsinfo->min_srv_interval);
    __put32(&tspec->ts_max_svc[0], tsinfo->max_srv_interval);
    __put32(&tspec->ts_inactv_intv[0], tsinfo->inactivity_interval);
    __put32(&tspec->ts_susp_intv[0], tsinfo->suspension_interval);
    __put32(&tspec->ts_start_svc[0], tsinfo->srv_start_time);
    __put32(&tspec->ts_min_rate[0], tsinfo->min_data_rate);
    __put32(&tspec->ts_mean_rate[0], tsinfo->mean_data_rate);
    __put32(&tspec->ts_max_burst[0], tsinfo->max_burst_size);
    __put32(&tspec->ts_min_phy[0], tsinfo->min_phy_rate);
    __put32(&tspec->ts_peak_rate[0], tsinfo->peak_data_rate);
    __put32(&tspec->ts_delay[0], tsinfo->delay_bound);
    __put16(&tspec->ts_surplus[0], tsinfo->surplus_bw);
    __put16(&tspec->ts_medium_time[0], 0);
#else
    *((u_int16_t *) &tspec->ts_nom_msdu) = tsinfo->norminal_msdu_size;
    *((u_int16_t *) &tspec->ts_max_msdu) = tsinfo->max_msdu_size;
    *((u_int32_t *) &tspec->ts_min_svc) = tsinfo->min_srv_interval;
    *((u_int32_t *) &tspec->ts_max_svc) = tsinfo->max_srv_interval;
    *((u_int32_t *) &tspec->ts_inactv_intv) = tsinfo->inactivity_interval;
    *((u_int32_t *) &tspec->ts_susp_intv) = tsinfo->suspension_interval;
    *((u_int32_t *) &tspec->ts_start_svc) = tsinfo->srv_start_time;
    *((u_int32_t *) &tspec->ts_min_rate) = tsinfo->min_data_rate;
    *((u_int32_t *) &tspec->ts_mean_rate) = tsinfo->mean_data_rate;
    *((u_int32_t *) &tspec->ts_max_burst) = tsinfo->max_burst_size;
    *((u_int32_t *) &tspec->ts_min_phy) = tsinfo->min_phy_rate;
    *((u_int32_t *) &tspec->ts_peak_rate) = tsinfo->peak_data_rate;
    *((u_int32_t *) &tspec->ts_delay) = tsinfo->delay_bound;
    *((u_int16_t *) &tspec->ts_surplus) = tsinfo->surplus_bw;
    *((u_int16_t *) &tspec->ts_medium_time) = 0;
#endif

    ieee80211_send_action(ni, &addts_args, &addts_buf);
    ieee80211_free_node(ni, WLAN_MGMT_HANDLER_ID);    /* reclaim node */
    return 0;
}

