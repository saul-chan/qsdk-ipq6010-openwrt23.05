/*
 * Copyright (c) 2016-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/* This is the unified configuration file for iw, acfg and netlink cfg, etc. */
#include <wlan_ipa_ucfg_api.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/utsname.h>
#include <linux/if_arp.h>       /* XXX for ARPHRD_ETHER */
#include <net/iw_handler.h>

#include <asm/uaccess.h>

#include "if_media.h"
#include "_ieee80211.h"
#include <osif_private.h>
#include <wlan_opts.h>
#include <ieee80211_var.h>
#include "ieee80211_rateset.h"
#include "ieee80211_vi_dbg.h"
#include "../vendor/generic/ioctl/ioctl_vendor_generic.h"
#include <ol_if_athvar.h>
#include <ol_txrx_dbg.h>
#include "if_athproto.h"
#include "base/ieee80211_node_priv.h"
#include "mlme/ieee80211_mlme_priv.h"
#include "mlme/wlan_mlme_vdev_mgmt_ops.h"
#include <wlan_psoc_mlme_api.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_vdev_if.h>
#endif
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link.h>
#endif
#include "ieee80211_crypto_nlshim_api.h"

#include "target_type.h"
#include "ieee80211_ucfg.h"
#include <dp_me.h>
#include <ieee80211_acl.h>
#include "ieee80211_mlme_dfs_dispatcher.h"

#include <dfs_postnol_ucfg.h>
#include <wlan_mlme_dispatcher.h>
#include <cdp_txrx_ctrl.h>

#include "rrm/ieee80211_rrm_priv.h"
#include <ieee80211_cfg80211.h>
#include <wlan_mlme_if.h>

#include <qdf_time.h>

#if QCA_AIRTIME_FAIRNESS
#include <wlan_atf_ucfg_api.h>
#endif /* QCA_AIRTIME_FAIRNESS */

#include <wlan_cmn.h>
#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>

#ifdef WLAN_SUPPORT_FILS
#include <wlan_fd_ucfg_api.h>
#include <wlan_fd_utils_api.h>
#endif /* WLAN_SUPPORT_FILS */

#include "wlan_crypto_global_def.h"
#include "wlan_crypto_global_api.h"

#if WLAN_SUPPORT_SPLITMAC
#include <wlan_splitmac.h>
#endif
#ifdef WLAN_SUPPORT_GREEN_AP
#include <wlan_green_ap_ucfg_api.h>
#endif
#if WLAN_CFR_ENABLE
#include <wlan_cfr_utils_api.h>
#include <wlan_cfr_ucfg_api.h>
#include <ieee80211_ioctl.h>
#endif

#ifdef WLAN_WIFI_RADAR_ENABLE
#include <wlan_wifi_radar_utils_api.h>
#include <wlan_wifi_radar_ucfg_api.h>
#include <wlan_wifi_radar_public_structs.h>
#endif

#if QCA_SUPPORT_GPR
#include "ieee80211_ioctl_acfg.h"
#endif

#include <wlan_vdev_mgr_utils_api.h>

#if ATH_ACS_DEBUG_SUPPORT
#include "acs_debug.h"
#endif

#ifdef WLAN_CFR_ENABLE
#include <wlan_cfr_public_structs.h>
#endif
#include <scheduler_api.h>
#include <wlan_cm_api.h>
#include <ieee80211_rtt.h>
#include <dp_extap.h>
#include <wlan_repeater_api.h>
#include <wlan_rnr.h>
#include <wlan_mlo_mgr_sta.h>

#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_cmn.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#include <ieee80211_proto_mlo.h>
#endif
#ifdef CONFIG_SAWF_DEF_QUEUES
#include <wlan_sawf.h>
#endif
#ifdef CONFIG_SAWF
#include <cdp_txrx_sawf.h>
#endif
#include <wlan_telemetry_agent.h>

#if defined(WLAN_SUPPORT_TWT) && defined(WLAN_TWT_CONV_SUPPORTED)
#include <wlan_twt_ucfg.h>
#endif

#include "cfg_ucfg_api.h"

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#include <wlan_mlo_epcs.h>
#endif
#include <net/genetlink.h>

#define ONEMBPS 1000
#define HIGHEST_BASIC_RATE 24000
#define THREE_HUNDRED_FIFTY_MBPS 350000

#define APCHAN_RPT_SSID_FILTER 0b01
#define APCHAN_RPT_OPCLASS_FILTER 0b10

/* Combine two 32bit array entry to form single 64bit entry */
#define RX_FILTER_GET64(_val) (((uint64_t)_val[2] << 32) | _val[1])

#if UMAC_SUPPORT_QUIET
/*
 * Quiet ie enable flag has 3 bits for now.
 * bit0-enable/disable, bit1-single-shot/continuos & bit3-include/skip
 * quiet ie in swba. So the maximum possible value for this flag is 7 for now.
 */
#define MAX_QUIET_ENABLE_FLAG 7
#endif
extern void set_quality(void *iq, u_int rssi, int16_t chan_nf);
extern int ol_ath_ucfg_get_user_position(wlan_if_t vaphandle, uint32_t aid);
extern int ol_ath_ucfg_get_peer_mumimo_tx_count(wlan_if_t vaphandle,
                                                uint32_t aid);
extern int ol_ath_ucfg_reset_peer_mumimo_tx_count(wlan_if_t vaphandle,
                                                  uint32_t aid);
extern int ieee80211_rate_is_valid_basic(struct ieee80211vap *, u_int32_t);
#if WLAN_SER_UTF
extern void wlan_ser_utf_main(struct wlan_objmgr_vdev *vdev, u_int8_t,
                              u_int32_t);
#endif
#if WLAN_SER_DEBUG
extern void wlan_ser_print_history(struct wlan_objmgr_vdev *vdev, u_int8_t,
                             u_int32_t);
#endif
#if WLAN_SCHED_HISTORY_SIZE
extern void sched_history_print(void);
#endif
#if QCA_PPE_VP
extern int osif_pltfrm_update_ppe_vp(osif_dev *osif);
#endif
#if SM_ENG_HIST_ENABLE
extern void wlan_mlme_print_all_sm_history(void);
#endif
#if MESH_MODE_SUPPORT
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
extern int wlan_update_rawsim_config(struct ieee80211vap *vap);
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
int ieee80211_add_localpeer(wlan_if_t vap, char *params);
int ieee80211_authorise_local_peer(wlan_if_t vap, char *params);
#endif

#if IEEE80211_DEBUG_NODELEAK
void wlan_debug_dump_nodes_tgt(void);
#endif

#ifdef WLAN_DFS_PRECAC_AUTO_CHAN_SUPPORT
extern void ieee80211_autochan_switch_chan_change_csa(struct ieee80211com *ic,
                                                      uint16_t freq,
                                                      uint16_t cfreq2,
                                                      uint16_t ch_width);
#endif

#if defined(CONFIG_SAWF_DEF_QUEUES) && defined(WLAN_SUPPORT_SCS)
QDF_STATUS ieee80211_add_service_class(struct ieee80211com *ic,
                                       struct ieee80211_qos_attr *qos_attr);
QDF_STATUS ieee80211_disable_service_class(struct ieee80211com *ic,
                                           uint8_t svc_id);
void ieee80211_check_existing_svc_qos_attributes_match(
                         struct wlan_sawf_svc_class_params *temp_svclass_params,
                         struct sawf_ctx *sawf_ctx);
#endif

extern void wlan_send_omn_action(void *arg, wlan_node_t node);

#ifdef QCA_PARTNER_PLATFORM
extern int wlan_pltfrm_set_param(wlan_if_t vaphandle, u_int32_t val);
extern int wlan_pltfrm_get_param(wlan_if_t vaphandle);
#endif
int ieee80211_config_oce_ipsubnet_id(wlan_if_t vap, char *params);
int ieee80211_config_oce_ess_rpt_param(wlan_if_t vap, char *params);
int ieee80211_config_oce_tx_power(wlan_if_t vap, char *params);
#ifdef ASSOC_REJECT_SUPPORT_ENABLED
int ieee80211_set_assoc_reject(wlan_if_t vap, char *params);
#endif

static const int basic_11b_rate[] = {1000, 2000, 5500, 11000 };
static const int basic_11bgn_rate[] = {1000, 2000, 5500, 6000, 11000, 12000, 24000 };
static const int basic_11na_rate[] = {6000, 12000, 24000 };
static const int basic_11ax_6g_rate[] = {6000, 7300, 12000, 14600, 21900, 24000, 29300, 43900, 58500, 65800, 73100 };
static const int basic_he_rate[] = {7300, 14600, 21900, 29300, 43900, 58500, 65800, 73100 };

#define HIGHEST_BASIC_RATE 24000    /* Highest rate that can be used for mgmt frames */

#if defined(WLAN_CFR_ENABLE) && defined(WLAN_ENH_CFR_ENABLE)
/**
 * rcc_filter : Flags to indicate the state of RCC filter
 * @rcc_filter_disable : All RCC capture methods are disabled
 * @rcc_filter_enable  : At least one RCC capture method is enabled
 * @rcc_filter_failure : While trying to commit the configuration for any
 *                       of the RCC capture methods, if the commit fails due to
 *                       any reason.
 */
enum rcc_filter {
    rcc_filter_disable = 0,
    rcc_filter_enable = 1,
    rcc_filter_failure = 2,
};
#endif

int ieee80211_config_rsnx(wlan_if_t vap, char *params);

/* Get mu tx blacklisted count for the peer */
void wlan_get_mu_tx_blacklist_cnt(ieee80211_vap_t vaphandle,int value)
{
    bool found = FALSE;

    /**
     * Currently there are no counters to know how long/how many times
     * the peer with the specified AID value has been blacklisted from MU
     * transmission. So as of now, check just if there exists a peer with
     * the specified AID value or not.
     */
    found = ieee80211_validate_aid(vaphandle->iv_ic, value);

    if (!found)
        qdf_err("Invalid AID value");
}

int ieee80211_ucfg_set_essid(wlan_if_t vap, ieee80211_ssid *data, bool is_vap_restart_required)
{
    osif_dev *osifp = (osif_dev *)vap->iv_ifp;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    ieee80211_ssid   tmpssid;
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);

    if (osifp->is_delete_in_progress)
        return -EINVAL;

    if (opmode == IEEE80211_M_WDS)
        return -EOPNOTSUPP;

    if(data->len <= 0)
        return -EINVAL;

    OS_MEMZERO(&tmpssid, sizeof(ieee80211_ssid));

    if (data->len > IEEE80211_NWID_LEN)
        data->len = IEEE80211_NWID_LEN;

    tmpssid.len = data->len;
    OS_MEMCPY(tmpssid.ssid, data->ssid, data->len);
    /*
     * Deduct a trailing \0 since iwconfig passes a string
     * length that includes this.  Unfortunately this means
     * that specifying a string with multiple trailing \0's
     * won't be handled correctly.  Not sure there's a good
     * solution; the API is botched (the length should be
     * exactly those bytes that are meaningful and not include
     * extraneous stuff).
     */
     if (data->len > 0 &&
            tmpssid.ssid[data->len-1] == '\0')
        tmpssid.len--;

    wlan_set_desired_ssidlist(vap,1,&tmpssid);

    if (tmpssid.len) {
        qdf_nofl_info("DES SSID SET=%s", tmpssid.ssid);
    }


#ifdef ATH_SUPPORT_P2P
    /* For P2P supplicant we do not want start connnection as soon as ssid is set */
    /* The difference in behavior between non p2p supplicant and p2p supplicant need to be fixed */
    /* see EV 73753 for more details */
    if ((osifp->os_opmode == IEEE80211_M_P2P_CLIENT
                || osifp->os_opmode == IEEE80211_M_STA
                || osifp->os_opmode == IEEE80211_M_P2P_GO) && !vap->auto_assoc)
        return 0;
#endif

    if (!is_vap_restart_required) {
        return 0;
    }

    return (dev && IS_UP(dev) &&
            ((osifp->os_opmode == IEEE80211_M_HOSTAP) || /* Call vap init for AP mode if netdev is UP */
             (vap->iv_ic->ic_roaming != IEEE80211_ROAMING_MANUAL))) ? osif_vap_init(osifp, dev, RESCAN) : 0;
}

int ieee80211_ucfg_get_essid(wlan_if_t vap, ieee80211_ssid *data, int *nssid)
{
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);

    if (opmode == IEEE80211_M_WDS)
        return -EOPNOTSUPP;

    *nssid = wlan_get_desired_ssidlist(vap, data, 1);
    if (*nssid <= 0)
    {
        if (opmode == IEEE80211_M_HOSTAP)
            data->len = 0;
        else
            wlan_get_bss_essid(vap, data);
    }

    return 0;
}

int ieee80211_ucfg_get_freq(wlan_if_t vap)
{
    osif_dev *osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osif);
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;
    int freq;

    if (dev && (dev->flags & (IFF_UP | IFF_RUNNING))) {
        *chan = wlan_get_bss_channel(vap);
    } else {
        *chan = wlan_get_current_channel(vap, true);
    }
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        freq = (int)chan->ic_freq;
    } else {
        freq = 0;
    }

    return freq;
}

int ieee80211_ucfg_set_freq(wlan_if_t vap, struct chan_params *chan_p)
{
    int ret;
    struct ieee80211com *ic = vap->iv_ic;
    int waitcnt = 0;
    qdf_event_t wait_event;

    /* Channel change from user and radar are serialized using IEEE80211_CHANCHANGE_MARKRADAR flag.
     */
    IEEE80211_CHAN_CHANGE_LOCK(ic);
    if (!IEEE80211_CHANCHANGE_STARTED_IS_SET(ic) && !IEEE80211_CHANCHANGE_MARKRADAR_IS_SET(ic)) {
        IEEE80211_CHANCHANGE_STARTED_SET(ic);
        IEEE80211_CHANCHANGE_MARKRADAR_SET(ic);
        IEEE80211_CHAN_CHANGE_UNLOCK(ic);
        ret = ieee80211_ucfg_set_freq_internal(vap, chan_p);
        IEEE80211_CHANCHANGE_STARTED_CLEAR(ic);
        IEEE80211_CHANCHANGE_MARKRADAR_CLEAR(ic);
    } else {
        IEEE80211_CHAN_CHANGE_UNLOCK(ic);

        /* CSA channel change is already in progress. Hence, wait for the
         * channel change to complete to proceed with this iwconfig/cfg80211tool
         * command channel change.
         */
        qdf_mem_zero(&wait_event, sizeof(wait_event));
        qdf_event_create(&wait_event);
        qdf_event_reset(&wait_event);

        while(waitcnt <
              (ic->ic_chanchange_tbtt + OSIF_MAX_CHAN_CHANGE_TIMEOUT_OFFSET)) {
            IEEE80211_CHAN_CHANGE_LOCK(ic);
            if (!IEEE80211_CHANCHANGE_STARTED_IS_SET(ic) &&
                !IEEE80211_CHANCHANGE_MARKRADAR_IS_SET(ic)) {
                /* Channel change is not in progress, hence trigger the channel
                 * change.
                 */
                IEEE80211_CHANCHANGE_STARTED_SET(ic);
                IEEE80211_CHANCHANGE_MARKRADAR_SET(ic);
                IEEE80211_CHAN_CHANGE_UNLOCK(ic);
                ret = ieee80211_ucfg_set_freq_internal(vap, chan_p);
                IEEE80211_CHANCHANGE_STARTED_CLEAR(ic);
                IEEE80211_CHANCHANGE_MARKRADAR_CLEAR(ic);
                break;
            }
            IEEE80211_CHAN_CHANGE_UNLOCK(ic);

            qdf_wait_single_event(&wait_event, ic->ic_intval);
            waitcnt++;
        }

        qdf_event_destroy(&wait_event);

        if (waitcnt ==
            (ic->ic_chanchange_tbtt + OSIF_MAX_CHAN_CHANGE_TIMEOUT_OFFSET)) {
            qdf_err("Channel Change is already on, Please try later");
            ret = -EBUSY;
        }
    }

    return ret;
}

#ifdef WLAN_DFS_PRECAC_AUTO_CHAN_SUPPORT
static int ieee80211_switch_to_intercac_chan(wlan_if_t vap, uint16_t *freq)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    bool use_inter_chan = false, is_11ax_80_160 = false, is_11ac_80_160 = false, is_11be_80_160 = false;
    bool is_interchan_using_rcac = false;

    pdev = vap->iv_ic->ic_pdev_obj;

    if(pdev == NULL) {
        qdf_err("%s : pdev is null", __func__);
        return -EINVAL;
    }

    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
        QDF_STATUS_SUCCESS) {
        return -EINVAL;
    }

    if (!freq)
        return -EINVAL;

    /*
     * Intermediate chan usage is currently supported for 80MHz only using RCAC
     */
    if (mlme_dfs_is_agile_rcac_enabled(pdev) &&
        (ieee80211_get_chan_width_from_phymode(vap->iv_des_mode) == CH_WIDTH_80MHZ))
        is_interchan_using_rcac = true;

    if (ieee80211_is_phymode_11ac_vht80(vap->iv_des_mode) ||
        ieee80211_is_phymode_11ac_vht160(vap->iv_des_mode)) {
        is_11ac_80_160 = true;
    } else if (ieee80211_is_phymode_11axa_he80(vap->iv_des_mode) ||
               ieee80211_is_phymode_11axa_he160(vap->iv_des_mode)) {
               is_11ax_80_160 = true;
    }
#ifdef WLAN_FEATURE_11BE
    else if (ieee80211_is_phymode_11bea_eht160(vap->iv_des_mode) ||
             ieee80211_is_phymode_11bea_eht80(vap->iv_des_mode)) {
             is_11be_80_160 = true;
    }
#endif
    if (*freq &&
	(mlme_dfs_is_agile_precac_enabled(pdev) || is_interchan_using_rcac) &&
        (ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP) &&
        (is_11ax_80_160 || is_11ac_80_160 || is_11be_80_160)) {
        use_inter_chan =
            mlme_dfs_decide_precac_preferred_chan_for_freq(pdev,
                                                           freq,
                                                           phymode2convphymode[vap->iv_des_mode]);
        /*
         * If channel change is triggered in 160MHz,
         * Change mode to 80MHz to use intermediate channel, start precac
         * on configured channel. Send OMN to notify mode change.
         */
         if (ieee80211_is_phymode_160(vap->iv_des_mode) &&
             use_inter_chan) {
             qdf_info("Use intermediate channel in VHT80 mode");
             if (is_11ac_80_160)
                 wlan_set_desired_phymode(vap, IEEE80211_MODE_11AC_VHT80);
             else if (is_11ax_80_160)
                      wlan_set_desired_phymode(vap, IEEE80211_MODE_11AXA_HE80);
#ifdef WLAN_FEATURE_11BE
             else if (is_11be_80_160)
                      wlan_set_desired_phymode(vap, IEEE80211_MODE_11BEA_EHT80);
#endif

             if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
                 uint16_t ifreq = *freq;

                 /* Send broadcast OMN*/
                 wlan_set_param(vap, IEEE80211_OPMODE_NOTIFY_ENABLE, 1);
                 /* Send unicast OMN*/
                 wlan_iterate_station_list(vap, wlan_send_omn_action, NULL);
                 if (is_11ac_80_160) {
                     ieee80211_autochan_switch_chan_change_csa(vap->iv_ic,
                                                               ifreq, 0,
                                                               IEEE80211_MODE_11AC_VHT80);
                 } else if (is_11ax_80_160) {
                            ieee80211_autochan_switch_chan_change_csa(vap->iv_ic,
                                                                      ifreq, 0,
                                                                      IEEE80211_MODE_11AXA_HE80);
                 }
#ifdef WLAN_FEATURE_11BE
                 else if (is_11be_80_160) {
                          ieee80211_autochan_switch_chan_change_csa(vap->iv_ic,
                                                                    ifreq, 0,
                                                                    IEEE80211_MODE_11BEA_EHT80);
                 }
#endif
                 wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
                 return EOK;
             }
         }
     } else {
         /* Disable interCAC feature in case of unsupported phymode */
         utils_dfs_reset_intercac(pdev);
     }
     wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
     return 1;
}
#else
static inline int ieee80211_switch_to_intercac_chan(wlan_if_t vap, uint16_t *freq)
{
    return 1;
}
#endif

bool ieee80211_is_dot11_channel_mode_valid(wlan_if_t vap,
                                           struct chan_params *chan_p,
                                           int mode)
{
    struct ieee80211_ath_channel tmp_chan;
    struct ieee80211com *ic = vap->iv_ic;

    if (WLAN_REG_IS_6GHZ_CHAN_FREQ(chan_p->freq)) {
        chan_p->input_6g_pwr_mode = REG_BEST_PWR_MODE;
    }

    tmp_chan = ieee80211_find_dot11_channel(ic, chan_p, mode);
    return IEEE80211_IS_CHAN_VALID(&tmp_chan);
}

/*
 * ieee80211_ucfg_wideband_channel_switch_sanity:
 * Check if wideband channel switch is supported.
 *
 * @vap        : Pointer to the ic structure
 * @target_freq: Value of the target frequency
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successful
 * QDF_STATUS_E_INVAL: Failure
 */
QDF_STATUS ieee80211_ucfg_wideband_channel_switch_sanity(struct ieee80211vap *vap,
                                      struct ieee80211_ath_channel *target_chan)
{
    struct ieee80211com *ic = NULL;

    if (!vap) {
        qdf_err("vap is invalid");
        return QDF_STATUS_E_INVAL;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is invalid");
        return QDF_STATUS_E_INVAL;
    }

    if (IEEE80211_SKIP_WIDEBAND_SWITCH(vap, target_chan)) {
        qdf_debug("Wideband channel switch is not required");
        return QDF_STATUS_SUCCESS;
    }

    if (!ic->ic_wideband_capable) {
        qdf_err("Wideband not supported on radio");
        return QDF_STATUS_E_INVAL;
    }

    if (!IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan) ||
        !IEEE80211_IS_CHAN_11AX_SUPPORTED(target_chan)) {
        qdf_err("Current and/or target channel not in 11AXA/BE HE mode");
        return QDF_STATUS_E_INVAL;
    }

    /* The wideband security admission control is applicable only for
     * wideband switches from 5GHz to 6GHz */
    if (IEEE80211_IS_CHAN_6GHZ(target_chan) &&
        IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan) &&
        wlan_cfg80211_mbssid_security_admission_control_sanity(ic, true)) {
        qdf_err("Wideband security admission control failed - "
                "sanity check failed");
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}

/*
 * ieee80211_ucfg_handle_wideband_switch:
 * Handle the wideband channel switch.
 *
 * @vap   : Pointer to the ic structure.
 * @target: Pointer to the desired target channel.
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_NOMEM:
 * QDF_STATUS_E_INVAL: Failure
 */
QDF_STATUS ieee80211_ucfg_handle_wideband_channel_switch(struct ieee80211vap *vap,
                                      struct ieee80211_ath_channel *target_chan)
{
    int retval = 0;
    mbss_mode_t target_mode;
    struct ieee80211com *ic = NULL;

    if (!vap) {
        qdf_err("vap is invalid");
        return QDF_STATUS_E_INVAL;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("Invalid ic pointer");
        return QDF_STATUS_E_INVAL;
    }

    if (IEEE80211_SKIP_WIDEBAND_SWITCH(vap, target_chan)) {
        qdf_debug("Wideband channel switch is not required");
        return QDF_STATUS_SUCCESS;
    }

    target_mode = IEEE80211_IS_CHAN_6GHZ(target_chan) ? MBSS_MODE_MBSSID_EMA :
                                                        MBSS_MODE_COHOSTED;

    retval = ieee80211_mbss_switch_mode(ic, target_mode, false);
    switch (retval) {
        case MBSS_SWITCH_E_NORECOVER_INVAL:
            qdf_err("Could not recover from failed mode switch to %d"
                    "due to invalid params", target_mode);
            retval = QDF_STATUS_E_INVAL;
        break;
        case MBSS_SWITCH_E_NORECOVER_NOMEM:
            qdf_err("Could not recover from failed mode switch to %d "
                    "due to no memory", target_mode);
            retval = QDF_STATUS_E_NOMEM;
        break;
        case MBSS_SWITCH_E_RECOVER:
            qdf_err("Recovered from failed mode switch to %d", target_mode);
            ieee80211_bringup_all_vaps(ic, ALL_VDEVS);
            retval = QDF_STATUS_E_INVAL;
        break;
        case MBSS_SWITCH_SUCCESS:
            qdf_debug("Successfully switched mode to %d", target_mode);
            retval = QDF_STATUS_SUCCESS;
        break;
        default:
            qdf_debug("Invalid return value: %d", retval);
            retval = QDF_STATUS_E_INVAL;
    }

    return retval;
}

#define MAX_PHYMODE_STRLEN 30
static void print_radar_chan_info(struct ieee80211_ath_channel *channel)
{
    char mode_str[MAX_PHYMODE_STRLEN] = {0};
    uint16_t modestr_len;

    ieee80211_convert_phymode_to_string(ieee80211_chan2mode(channel),
                                        mode_str, &modestr_len);

    if (IEEE80211_IS_CHAN_80_80MHZ(channel)) {
        qdf_err("Radar found on chan mode: (%s), prim_freq: %d "
                "prim_80_cfreq: %d, sec_80_cfreq = %d\n",
                mode_str, channel->ic_freq,
                channel->ic_vhtop_freq_seg1, channel->ic_vhtop_freq_seg2);
    } else {
        qdf_err("Radar found on chan mode: (%s), prim_freq: %d\n", mode_str,
                channel->ic_freq);
    }
}

int ieee80211_ucfg_set_freq_internal(wlan_if_t vap, struct chan_params *chan_p)
{
    struct ieee80211com *ic = vap->iv_ic;
    osif_dev *osnetdev = (osif_dev *)vap->iv_ifp;
    int retval;
    struct ieee80211vap *tmpvap = NULL;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct net_device *tmpdev = NULL;
    struct ieee80211_ath_channel channel_obj = empty_channel;
    struct ieee80211_ath_channel *channel = &channel_obj;
    struct ieee80211_ath_channel tmp_chan_obj = empty_channel;
    struct ieee80211_ath_channel *tmp_chan = &tmp_chan_obj;
    uint16_t in_punc_bitmap;

    if ((chan_p->freq == 0) && IEEE80211_IS_SPECIAL_VAP(vap)) {
        /* ACS is not supported for special vap */
        if (!IEEE80211_IS_SMART_MON_VAP(vap)) {
            qdf_err("ACS is not supported with special vap");
            return -EINVAL;
        }

        if (wlan_autoselect_in_progress(vap)) {
            qdf_info("vap = %d, acs in progress, return", vap->iv_unit);
            return 0;
        }
    }

    if (osnetdev->is_delete_in_progress)
        return -EINVAL;

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
    if(ic->ic_primary_allowed_enable && (chan_p->freq != 0) &&
                    !(ieee80211_check_allowed_prim_freqlist(ic, chan_p->freq))) {
            qdf_err("channel chan_p->freq %d is not a primary allowed channel", chan_p->freq);
            return -EINVAL;
    }
#endif

    if (chan_p->freq == 0)
        chan_p->freq = IEEE80211_FREQ_ANY;

    retval = ieee80211_switch_to_intercac_chan(vap, (uint16_t *)&chan_p->freq);
    if (retval != 1)
        return retval;

    if (vap->iv_opmode == IEEE80211_M_IBSS) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : IBSS desired channel freq(%d) cfreq2(%d)\n",
                __func__, chan_p->freq, chan_p->cfreq2);
        return wlan_set_desired_ibsschan(vap, chan_p);
    }
    else if (vap->iv_opmode == IEEE80211_M_HOSTAP || vap->iv_opmode == IEEE80211_M_MONITOR)
    {
#if ATH_CHANNEL_BLOCKING
        struct ieee80211_ath_channel tmp_chan_obj;
        struct ieee80211_ath_channel *tmp_channel = &tmp_chan_obj;
#endif
        if (chan_p->freq != (uint16_t)IEEE80211_FREQ_ANY) {
            *channel = ieee80211_find_dot11_channel(ic, chan_p, vap->iv_des_mode);
            if (!IEEE80211_IS_CHAN_VALID(channel))
            {
                if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL) &&
                    ieee80211_get_num_ap_vaps_up(ic)) {
                    qdf_err("ERROR!! Channel(frequency) %d not compatible with current mode %d\n",
                            chan_p->freq, vap->iv_des_mode);
                    return -EINVAL;
                }
		chan_p->cfreq2 = 0;
                *channel = ieee80211_find_dot11_channel(ic, chan_p, IEEE80211_MODE_AUTO);
                if (!IEEE80211_IS_CHAN_VALID(channel))
                    return -EINVAL;
            }

            /* Always choose the best power mode on channel change */
            if (WLAN_REG_IS_6GHZ_CHAN_FREQ(chan_p->freq)) {
                in_punc_bitmap = ieee80211_get_chan_param_op_punc_bitmap(chan_p);
                if (!IEEE80211_IS_SPECIAL_VAP_ONLY(vap)) {
                    if (ieee80211_apply_best_6g_ap_pwr_mode(ic,
                                                            chan_p->freq,
                                                            channel->ic_vhtop_freq_seg2,
                                                            false,
                                                            ieee80211_chan2mode(channel),
                                                            in_punc_bitmap) != QDF_STATUS_SUCCESS) {
                        qdf_err("Error applying best power mode");
                        return -EINVAL;
                    }
                } else {
                    if (ieee80211_quick_apply_best_6g_ap_pwr_mode(ic,
                                                                  chan_p->freq,
                                                                  channel->ic_vhtop_freq_seg2,
                                                                  false,
                                                                  ieee80211_chan2mode(channel),
                                                                  in_punc_bitmap) != QDF_STATUS_SUCCESS) {
                        qdf_err("Error applying best power mode for scan radio");
                        return -EINVAL;
                    }
                }
            }

            if (ieee80211_check_chan_mode_consistency(ic, vap->iv_des_mode, channel)) {
                if (IEEE80211_VAP_IS_PUREG_ENABLED(vap))
                    IEEE80211_VAP_PUREG_DISABLE(vap);
                if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
                    qdf_err("Chan mode consistency failed %d freq %d\n setting to LOWEST mode", vap->iv_des_mode,chan_p->freq);
                } else {
                    qdf_err("Chan mode consistency failed %d freq %d\n setting to AUTO mode", vap->iv_des_mode,chan_p->freq);
                }

                if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL) && ieee80211_get_num_ap_vaps_up(ic)) {
                    qdf_err("ERROR!! Channel(frequency) %d not compatible with current mode %d \
                            failed consistency check\n",
                            chan_p->freq, vap->iv_des_mode);

                    return -EINVAL;
                }

                TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                    if (ic->ic_diff_mode) {
                        wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_AUTO);
#ifdef WLAN_FEATURE_11BE
                    } else if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
                        if (ieee80211_is_mode_lower(tmpvap, tmpvap->iv_des_mode)) {
                            if (ieee80211_is_phymode_11ax(tmpvap->iv_des_mode)) {
                                if (ieee80211_is_phymode_2g(tmpvap->iv_des_mode))
                                    wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_11AXG_HE20);
                                else
                                    wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_11AXA_HE20);
                            } else if (ieee80211_is_phymode_11ac(tmpvap->iv_des_mode)) {
                                wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_11AC_VHT20);
                            } else {
                                wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_11NG_HT20);
                            }
                        } else {
                            if (ieee80211_is_phymode_2g(tmpvap->iv_des_mode))
                                wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_11BEG_EHT20);
                            else
                                wlan_set_desired_phymode(tmpvap, IEEE80211_MODE_11BEA_EHT20);
                        }
#endif
                    } else {
                        tmpvap->iv_des_mode = IEEE80211_MODE_AUTO;
                    }
                    tmpvap->iv_diff_mode = 0;
                    if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL)) {
                        /*
                         * This needs to be set here.
                         * Start-ap checks iv_des_hw_mode and if it is anything
                         * other than AUTO, start_ap will use iv_des_hw_mode
                         * instead of the value coming through start_ap
                         *
                         * If iv_des_hw_mode is not set to AUTO, it remains in
                         * the older value. Thus when start_ap routine is
                         * called, it will fetch this older value and call
                         * the set_desired_phymode function thus resulting in
                         * failure in case of strict-mode
                         *
                         * Reaching here implies that all VAPs are in the down
                         * state. Refer the osif_num_ap_up_vaps call above
                         *
                         * This problem is unique to changing channel when VAP
                         * is in down state. If the VAP is in up state, then
                         * set_desired_phymode will be called in the MLME state
                         * machine, thereby making iv_des_mode and
                         * iv_des_hw_mode the same
                         *
                         */
                        tmpvap->iv_des_hw_mode = IEEE80211_MODE_AUTO;
                    }
                }
                ic->ic_diff_mode = 0;
            }

#if ATH_CHANNEL_BLOCKING
            *tmp_channel = *channel;
            *channel = wlan_acs_channel_allowed(vap, channel, vap->iv_des_mode);
            if (!IEEE80211_IS_CHAN_VALID(channel))
            {
                qdf_print("channel blocked by acs");
                return -EINVAL;
            }

            if(!IEEE80211_ARE_CHANS_EQUAL(tmp_channel, channel) &&
               ieee80211_check_chan_mode_consistency(ic,vap->iv_des_mode,channel))
            {
                qdf_err("Chan mode consistency failed %x freq %d %d", vap->iv_des_mode, chan_p->freq ,channel->ic_freq);
                return -EINVAL;
            }
#endif

#ifdef WLAN_FEATURE_11BE
            if (IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, channel,
                                                               chan_p->puncture_bitmap))
#else
            if (IEEE80211_IS_CHAN_RADAR(ic, channel))
#endif
            {
                /*
                 * Update the des channel based on running vap.
                 * This will ignore the channel param received from hostapd.
                 */
                 if (osif_get_num_running_vaps(ic)) {
                     *tmp_chan = ieee80211_derive_chan(vap);
                     if (!IEEE80211_IS_CHAN_VALID(tmp_chan)) {
                         qdf_err("Channel Freq %d Cfreq %d power mode %d"
                                 "is not vaild for %s psoc:%u vdev:%u",
                                 chan_p->freq, chan_p->cfreq2,
                                 chan_p->input_6g_pwr_mode,
                                 vap->iv_netdev_name,
                                 wlan_vdev_get_psoc_id(vap->vdev_obj),
                                 wlan_vdev_get_id(vap->vdev_obj));
                         return -EINVAL;
                      }
                     qdf_info("Update the des channel based on running" 
                               "vap %s psoc:%u vdev:%u from des channel"
                               " %d to new des channel %d",
                                 vap->iv_netdev_name,
                                 wlan_vdev_get_psoc_id(vap->vdev_obj),
                                 wlan_vdev_get_id(vap->vdev_obj),
                                 chan_p->freq, tmp_chan->ic_freq);
                               
                      chan_p->freq = tmp_chan->ic_freq;
                      chan_p->cfreq2 = tmp_chan->ic_vhtop_freq_seg2;
                      *channel = *tmp_chan;
                  } else {
                    print_radar_chan_info(channel);
                    return -EINVAL;
                  }
            }

	    if (ic->ic_obss_done_freq != channel->ic_freq) {
                ic->ic_obss_done_freq = 0;
	    }
        }

        /*
         * In Monitor mode for auto channel, first valid channel is taken for configured mode.
         * In case of mutiple vaps with auto channel, AP VAP channel will be configured to monitor VAP.
         */

        if ((vap->iv_opmode == IEEE80211_M_MONITOR) && ((int)chan_p->freq == IEEE80211_FREQ_ANY)) {
            retval = wlan_set_channel(vap, chan_p);
            return retval;
        }

        if (IEEE80211_IS_CHAN_VALID(channel)) {
            struct ieee80211_ath_channel *cur_channel;

            if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap))
                cur_channel = ic->ic_curchan;
            else
                cur_channel = vap->iv_des_chan[vap->iv_des_mode];

            if (IEEE80211_ARE_CHANS_EQUAL(cur_channel, channel)) {
                qdf_print("\n Channel is configured already!!");
                /*
                 * In MBSSID mode with monitor VAPs, monitor VAPS might
                 * remain unbroughtup with new desired frequency when chan
                 * set to auto. So consider to restart the monitor VAPs with
                 * new desired channel as same as this VAP
                 */
                if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
                    retval = wlan_set_channel(vap, chan_p);
                } else {
                    retval = EOK;
                    if (WLAN_REG_IS_6GHZ_CHAN_FREQ(channel->ic_freq))
                        mlme_update_chan_list_and_restart_vaps(ic, true);
                }
                if (vap->iv_opmode != IEEE80211_M_MONITOR)
                    goto set_mon_des_chan;
                return retval;
            } else if (IEEE80211_ARE_CHANS_EQUAL(ic->ic_curchan, channel) &&
                       !IEEE80211_ARE_CHANS_EQUAL(vap->iv_des_chan[vap->iv_des_mode], channel) &&
                       (wlan_vdev_chan_config_valid(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
                retval = wlan_set_channel(vap, chan_p);
                wlan_iterate_vap_list(ic, ieee80211_vap_iter_set_des_channel, (void *)channel);
                return retval;
            }
        }

        /*
         * Check wideband channel switch sanity only if channel is not NULL.
         */
        if (ieee80211_ucfg_wideband_channel_switch_sanity(vap, channel)) {
            qdf_err("Wideband channel sanity failed - cannot continue with channel change");
            return -EINVAL;
        }

        if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap) &&
            wlan_vdev_chan_config_valid(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
            /*
             * Channel switch time is a critical KPI for special vap mode.
             * Use vdev restart sequence instead of vdev stop-start sequence,
             * for changing channel of a special vap.
             */
            if (vap->vdev_obj->vdev_mlme.des_chan)
                ieee80211_update_vdev_chan(vap->vdev_obj->vdev_mlme.des_chan, channel);
            wlan_iterate_vap_list(ic, ieee80211_vap_iter_set_des_channel, (void *)channel);

            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                qdf_event_reset(&tmpvap->wait_event_vdev_up_state);
            }

            return osif_pdev_restart_vaps(ic);
        }

        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (tmpvap->iv_opmode == IEEE80211_M_STA) {
                vap->iv_roam_inprogress = FALSE;
                /*
                 * Expectation is STA vap must go down during channel change.
                 * But if STA VAP is in connecting state, DISASSOC event is invalid
                 * event to bring down the vap. Beacon miss event helps to recover
                 * from longer STA Bringdown wait time.
                 */
                IEEE80211_DELIVER_EVENT_BEACON_MISS(tmpvap);
            } else if (!IEEE80211_IS_MLO_BRIDGE_VAP(tmpvap)) {
                wlan_mlme_stop_vdev(tmpvap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
            }
        }

        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (!tmpvap->iv_channel_switch_stats.block_for_vdev_down)
                tmpvap->iv_channel_switch_stats.block_for_vdev_down =
                        qdf_get_log_timestamp();
        }

        retval = wlan_pdev_wait_to_bringdown_vdevs(ic, ALL_VDEVS);
        if (retval == QDF_STATUS_E_INVAL)
            return -EINVAL;

        /*
         * Handle wideband mode switch only after VDEVs down and are applicable
         * only if wideband mode is supported.
         */
        retval = ieee80211_ucfg_handle_wideband_channel_switch(vap, channel);
        if (retval) {
            return retval;
        }

        retval = wlan_set_channel(vap, chan_p);

        if(!retval) {
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                /* Set the desired chan for other VAP to same a this VAP */
                /* Set des chan for all VAPs to be same */
                if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP ||
                        tmpvap->iv_opmode == IEEE80211_M_MONITOR ) {
                    *tmpvap->iv_des_chan[tmpvap->iv_des_mode] =
                        *vap->iv_des_chan[vap->iv_des_mode];
                }
                if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP && (ic->ic_diff_mode || ieee80211_mixed_modes_config(vap, vap->iv_des_mode))) {
                    *tmpvap->iv_des_chan[tmpvap->iv_des_mode] =
                        ieee80211_find_desired_chan(tmpvap,vap->iv_des_chan[vap->iv_des_mode],tmpvap->iv_des_mode);
                }
            }
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
                if (IEEE80211_IS_MLO_BRIDGE_VAP(tmpvap)) {
                    continue;
                }
                /* Set the desired chan for other VAP to same a this VAP */
                /* Set des chan for all VAPs to be same */
                if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP ||
                   tmpvap->iv_opmode == IEEE80211_M_MONITOR) {
                    if (IS_UP(tmpdev) && (vap->iv_novap_reset == 0)) {
                        qdf_event_reset(&tmpvap->wait_event_vdev_up_state);
                        retval = osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpdev, RESCAN);
                    } else if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL) && (!IS_UP(tmpdev))) {
                        struct net_device *tmpdev;

                        tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
                        IEEE80211_DELIVER_EVENT_CHANNEL_CHANGE(tmpvap, tmpvap->iv_des_chan[tmpvap->iv_des_mode]);
                        retval = 0;
                    } else {
                        retval = 0;
                    }
                }
            }
        }
        return retval;
    } else {
        retval = wlan_set_channel(vap, chan_p);
        return retval;
    }

set_mon_des_chan:
    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if (tmpvap->iv_opmode == IEEE80211_M_MONITOR) {
            wlan_mlme_stop_vdev(tmpvap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
        }
    }

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if (tmpvap->iv_opmode == IEEE80211_M_MONITOR) {
            tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
            *tmpvap->iv_des_chan[vap->iv_des_mode] = *vap->iv_des_chan[vap->iv_des_mode];
            retval = (IS_UP(tmpdev) && (vap->iv_novap_reset == 0)) ? osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpdev, RESCAN) : EOK;
        }
    }
    return retval;
}

/**
 * ieee80211_get_csa_chwidth_from_cwm: Helper function to get internal CSA width
 * representation from CWM width
 * @cwm_width: CWM width
 * @pcsa_ch_width: Pointer to location where value of internal CSA width is to
 * be written (the value will be valid only if QDF_STATUS_SUCCESS is returned)
 *
 * Return: QDF_STATUS_SUCCESS on success, or a QDF_STATUS value providing the
 * error
 */
QDF_STATUS ieee80211_get_csa_chwidth_from_cwm(
        enum ieee80211_cwm_width cwm_width, u_int16_t *pcsa_ch_width)
{
    QDF_STATUS status = QDF_STATUS_E_FAILURE;

    if (NULL == pcsa_ch_width) {
        status = QDF_STATUS_E_INVAL;
        goto done;
    }

    if (cwm_width >= IEEE80211_CWM_WIDTH_MAX) {
        status = QDF_STATUS_E_INVAL;
        goto done;
    }

    switch (cwm_width) {
        case IEEE80211_CWM_WIDTH20:
            *pcsa_ch_width = CHWIDTH_20;
            status = QDF_STATUS_SUCCESS;
            break;
        case IEEE80211_CWM_WIDTH40:
            *pcsa_ch_width = CHWIDTH_40;
            status = QDF_STATUS_SUCCESS;
            break;
        case IEEE80211_CWM_WIDTH80:
            *pcsa_ch_width = CHWIDTH_80;
            status = QDF_STATUS_SUCCESS;
            break;
        case IEEE80211_CWM_WIDTH160:
        case IEEE80211_CWM_WIDTH80_80:
            *pcsa_ch_width = CHWIDTH_160;
            status = QDF_STATUS_SUCCESS;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320:
            *pcsa_ch_width = CHWIDTH_320;
            status = QDF_STATUS_SUCCESS;
            break;
#endif /* WLAN_FEATURE_11BE */
        default:
            status = QDF_STATUS_E_INVAL;
            break;
    }

done:
    return status;
}

/*
 * ieee80211_validate_wideband_security_mode:
 * Ensure security mode is valid for wideband channel switching.
 *
 * Params:
 * ic: Pointer to ic
 *
 * Return:
 * 0        : Valid security mode
 * Otherwise: Invalid security mode
 */
QDF_STATUS ieee80211_validate_wideband_security_mode(struct ieee80211com *ic)
{
    struct ieee80211vap *tmpvap = NULL;
    int retval = 0;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if (ic->ic_wideband_csa_support &&
            wlan_cfg80211_6ghz_security_check(tmpvap) != EOK) {
            retval = QDF_STATUS_E_INVAL;
        }
    }

    return retval;
}

void ieee80211_ucfg_set_chan_csa(wlan_if_t vap, struct ieee80211_ath_channel *chan)
{
     struct ieee80211com *ic = vap->iv_ic;
     struct ieee80211vap *tmpvap = NULL;
	 int retval;

     TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
         if (tmpvap->iv_opmode == IEEE80211_M_STA) {
             struct ieee80211_node *ni = tmpvap->iv_bss;
             u_int16_t associd = ni->ni_associd;
             vap->iv_roam_inprogress = FALSE;
#ifdef WLAN_FEATURE_11BE_MLO
             IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(tmpvap, ni->ni_macaddr, associd, IEEE80211_STATUS_UNSPECIFIED, &ni->peer_obj->mldaddr[0]);
#else
             IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(tmpvap, ni->ni_macaddr, associd, IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
         } else {
             wlan_mlme_stop_vdev(tmpvap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
         }
     }

     retval = wlan_pdev_wait_to_bringdown_vdevs(ic, ALL_VDEVS);
     if (retval == QDF_STATUS_E_INVAL)
         return;

     wlan_set_desired_phymode(vap, ieee80211_chan2mode(chan));
     retval = wlan_set_ieee80211_channel(vap, chan);

     if(!retval) {
         TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
             /* Set the desired phymode for other VAP to same a this VAP */
             if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP ||
                     tmpvap->iv_opmode == IEEE80211_M_MONITOR ) {
                 wlan_set_desired_phymode(tmpvap, vap->iv_des_mode);
             }
         }
         /* This needs to be invoked again, as set_desired_phymode
            has reset the channel back to previous channel
          */
         wlan_set_ieee80211_channel(vap, chan);
         /* Set des chan for all VAPs to be same */
         TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
             /* Set the desired chan for other VAP to same a this VAP */
             /* Set des chan for all VAPs to be same */
             if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP ||
                     tmpvap->iv_opmode == IEEE80211_M_MONITOR ) {
                 *tmpvap->iv_des_chan[vap->iv_des_mode] =
                     *vap->iv_des_chan[vap->iv_des_mode];
             }
         }
         TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
             struct net_device *tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
             /* Set the desired chan for other VAP to same a this VAP */
             /* Set des chan for all VAPs to be same */
             if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP ||
                     tmpvap->iv_opmode == IEEE80211_M_MONITOR ) {
                 retval = (IS_UP(tmpdev) && (vap->iv_novap_reset == 0)) ? osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpdev, RESCAN) : 0;
             }
         }
     }
}

void ieee80211_vap_iter_set_des_channel(void *arg, wlan_if_t vap)
{
    wlan_chan_t channel;

    if (arg == NULL)
        return;

    channel = (wlan_chan_t)arg;

    if(vap->iv_opmode == IEEE80211_M_HOSTAP ||
       vap->iv_opmode == IEEE80211_M_MONITOR ) {
        *vap->iv_des_chan[vap->iv_des_mode] = *channel;
    }
}

static void ieee80211_node_iter_band_compatibility(void *arg, wlan_node_t node)
{
    uint8_t WIDE_BAND_MASK = BIT(REG_BAND_5G) | BIT(REG_BAND_6G);
    uint8_t *wideband_unsupported_sta_present = (uint8_t *)arg;

    if (*wideband_unsupported_sta_present) {
        return;
    }

    if ((node->ni_supp_op_cl.bands_supported & WIDE_BAND_MASK)
            != WIDE_BAND_MASK) {
        *wideband_unsupported_sta_present = 1;
        IEEE80211_DPRINTF_IC(node->ni_ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s[%d] Wideband unsupported station present, mac = %s,"
                " ni_supp_op_cl.bands_supported = 0x%x\n", __func__, __LINE__,
                ether_sprintf(node->ni_macaddr), node->ni_supp_op_cl.bands_supported);
        return;
    }

    if (!(ieee80211_is_phymode_11ax(node->ni_phymode)
#ifdef WLAN_FEATURE_11BE
          || ieee80211_is_phymode_11be(node->ni_phymode)
#endif
         )) {
        *wideband_unsupported_sta_present = 1;
        IEEE80211_DPRINTF_IC(node->ni_ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s[%d] Sta phy mode is not compatible for switch, mac = %s, phymode = %d",
                __func__, __LINE__, ether_sprintf(node->ni_macaddr), node->ni_phymode);
        return;
    }

    return;
}

static void ieee80211_vap_iter_band_compatibility(void *arg, struct ieee80211vap *vap)
{
    uint8_t *wideband_unsupported_sta_present = (uint8_t *)arg;

    /* Loop only if unsupported sta present is not found yet */
    if (!(*wideband_unsupported_sta_present)) {
        wlan_iterate_station_list(vap, ieee80211_node_iter_band_compatibility, arg);
    }

    return;
}

static QDF_STATUS validate_node_compatibility(struct ieee80211com *ic)
{
    uint8_t wideband_unsupported_sta_present = 0;

    if (ic->ic_wideband_csa_support == WIDEBAND_CSA_DISABLED) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s[%d] Wideband is disabled\n", __func__, __LINE__);
        return QDF_STATUS_E_INVAL;
    }

    /*
     * If there is a STA VAP on the radio (i.e., repeater config),
     * ensure forced mode.
     */
    if (ic->ic_sta_vap) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL,
                         IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                         "%s[%d] STA VAP detected. Using forced mode\n",
                         __func__, __LINE__);
        return QDF_STATUS_SUCCESS;
    }

    /* If compatible switch is enabled, check if all stations support wideband */
    if (ic->ic_wideband_csa_support == WIDEBAND_CSA_COMPATIBILITY) {
        /* Assume unsupported stations are not present, in the beginning */

        wlan_iterate_vap_list(ic, ieee80211_vap_iter_band_compatibility,
                              (void*)(&wideband_unsupported_sta_present));
        if (wideband_unsupported_sta_present) {
            return QDF_STATUS_E_INVAL;
        }
    }

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS validate_mbssid_mode_compatibility(struct ieee80211com *ic)
{
    /* MBSSID check only applies if number of transmitting vaps is > 1 */
    if (ieee80211_get_num_ap_vaps_up(ic) > 1) {
        /* Switch only if MBSSID mode is EMA */
        if (wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj, WLAN_PDEV_F_MBSS_IE_ENABLE)
            && (wlan_pdev_nif_feat_ext_cap_get(ic->ic_pdev_obj, WLAN_PDEV_FEXT_EMA_AP_ENABLE))) {
            return QDF_STATUS_SUCCESS;
        } else {
            return QDF_STATUS_E_INVAL;
        }
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS check_inter_band_switch_compatibility(struct ieee80211com *ic)
{
    /* Validate current phymode of radio */
    if (!IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s[%d] Switch is not allowed in current mode, flag = %llx\n",
                __func__, __LINE__, ic->ic_curchan->ic_flags);
        return QDF_STATUS_E_INVAL;
    }

    /* If applicable, validate whether all nodes
     * support wideband and in proper phymode
     */
    if (validate_node_compatibility(ic) != 0) {
        return QDF_STATUS_E_INVAL;
    }

    /* If AP is in MBSSID mode, check if it is EMA.
     * As EMA is mandatory on 6GHz, MBSSID mode must be EMA for switch to happen
     */
    if (validate_mbssid_mode_compatibility(ic) != 0) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s[%d] EMA/MBSSID is disabled, Dropping the request\n",
                 __func__, __LINE__);
        return QDF_STATUS_E_INVAL;
    }

    /* If AP is not in a valid security mode, cancel the CSA */
    if (ieee80211_validate_wideband_security_mode(ic)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s[%d] Security mode is invalid, dropping the request\n",
                 __func__, __LINE__);
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}

int ieee80211_ucfg_set_chanswitch(wlan_if_t vaphandle, struct chan_params *chan_p, uint8_t tbtt, uint16_t ch_width)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    u_int64_t flags = 0;
    struct ieee80211_ath_channel radar_channel_obj;
    struct ieee80211_ath_channel *radar_channel = &radar_channel_obj;
    struct wlan_objmgr_pdev *pdev;
    enum reg_wifi_band target_band;
    int is_inter_band_switch = 0, retval = 0;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s : pdev is null", __func__);
        return -EINVAL;
    }

    if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap)) {
        qdf_info("CSA not supported with special vap");
        return -EINVAL;
    }

    /* doth_chswitch is not allowed when doth is disabled */
    if (!ic->ic_doth) {
            qdf_err("doth is disabled, channel switch is not allowed");
            return -EINVAL;
    }

    if (!chan_p) {
        qdf_err("chan_p NULL");
        return -EINVAL;
    }

    if (mlme_dfs_is_ap_cac_timer_running(pdev)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s: CAC timer is running, doth_chanswitch is not allowed\n", __func__);
        return -EINVAL;
    }

    /* Ensure previous channel switch is not pending */
    if (ic->ic_flags & IEEE80211_F_CHANSWITCH) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_EXTIOCTL_CHANSWITCH,
                "%s: Error: Channel change already in progress\n", __func__);
        return -EALREADY;
    }

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
    if(ic->ic_primary_allowed_enable &&
                    !ieee80211_check_allowed_prim_freqlist(ic, chan_p->freq)) {
            qdf_err("channel freq %d is not a primary allowed channel", chan_p->freq);
            return -EINVAL;
    }
#endif

    /* 11AX TODO: Recheck future 802.11ax drafts (>D1.0) on channel switching
     * rules */

    if (ic->ic_strict_doth && ic->ic_non_doth_sta_cnt) {
        qdf_err("strict_doth is enabled and non_doth_sta_cnt is %d"
                " Discarding channel switch request",
                ic->ic_non_doth_sta_cnt);
        return -EAGAIN;
    }

    target_band = wlan_reg_freq_to_band(chan_p->freq);
    if (target_band != wlan_reg_freq_to_band(ic->ic_curchan->ic_freq)) {
        is_inter_band_switch = 1;
    }

    *ic->ic_chanchange_channel = empty_channel;
    flags = ieee80211_get_band_flag(chan_p->freq);

    if ((ch_width == 0) && (!is_inter_band_switch) &&
        !ieee80211_is_freq_in_obj_valid(ic, chan_p, ic->ic_curchan->ic_flags)) {
        /* Switching between different modes is not allowed, print ERROR */
        qdf_err("Channel capabilities do not match, chan flags 0x%llx",
                ic->ic_curchan->ic_flags);
        return -EINVAL;
    } else {
        if (is_inter_band_switch) {
            retval = check_inter_band_switch_compatibility(ic);

            if (retval) {
                qdf_err("Interband switch restricted, err code = %d", retval);
                return -EINVAL;
            }
            if (ch_width != 0) {
                switch (ch_width) {
                    case CHWIDTH_20:
                        flags |= IEEE80211_CHAN_HE20;
                        break;
                    case CHWIDTH_40:
                        flags |= IEEE80211_CHAN_HE40PLUS;
                        if (!ieee80211_is_freq_in_obj_valid(ic, chan_p, flags)) {
                            /* HE40PLUS is no good, try minus */
                            flags = ieee80211_get_band_flag(chan_p->freq);
                            flags |= IEEE80211_CHAN_HE40MINUS;
                        }
                        break;
                    case CHWIDTH_80:
                        flags |= IEEE80211_CHAN_HE80;
                        break;
                    case CHWIDTH_160:
                            flags |= IEEE80211_CHAN_HE160;
                        break;
                    default:
                        qdf_err("Invalid channel width input, exiting");
                        return -EINVAL;
                        break;
                }
            } else {
                if (IEEE80211_IS_CHAN_11AXA_HE20(ic->ic_curchan)) {
                    flags |= IEEE80211_CHAN_HE20;
                } else if (IEEE80211_IS_CHAN_11AXA_HE40(ic->ic_curchan)) {
                    flags |= IEEE80211_CHAN_HE40PLUS;
                    if (!ieee80211_is_freq_in_obj_valid(ic, chan_p, flags)) {
                        /* HE40PLUS is no good, try minus */
                        flags = ieee80211_get_band_flag(chan_p->freq);
                        flags |= IEEE80211_CHAN_HE40MINUS;
                    }
                } else if (IEEE80211_IS_CHAN_11AXA_HE80(ic->ic_curchan)) {
                    flags |= IEEE80211_CHAN_HE80;
                } else if (IEEE80211_IS_CHAN_11AXA_HE160(ic->ic_curchan)) {
                    flags |= IEEE80211_CHAN_HE160;
                } else if (IEEE80211_IS_CHAN_11AXA_HE80_80(ic->ic_curchan)) {
                    flags |= IEEE80211_CHAN_HE80_80;
                }
            }
        } else if(ch_width != 0) {
            /* Set channel, chanflag, channel width from ch_width value */
#ifdef WLAN_FEATURE_11BE
            if (IEEE80211_IS_CHAN_11BE(ic->ic_curchan)) {
                switch(ch_width){
                    case CHWIDTH_20:
                        flags |= IEEE80211_CHAN_EHT20;
                        break;
                    case CHWIDTH_40:
                        flags |= IEEE80211_CHAN_EHT40PLUS;
                        if (!ieee80211_is_freq_in_obj_valid(ic, chan_p, flags)) {
                            /* HE40PLUS is no good, try minus */
                            flags = ieee80211_get_band_flag(chan_p->freq);
                            flags |= IEEE80211_CHAN_EHT40MINUS;
                        }
                        break;
                    case CHWIDTH_80:
                        flags |= IEEE80211_CHAN_EHT80;
                        break;
                    case CHWIDTH_160:
                            flags |= IEEE80211_CHAN_EHT160;
                        break;
                    case CHWIDTH_320:
                            flags |= IEEE80211_CHAN_EHT320;
                        break;
                    default:
                        qdf_err("Invalid channel width input, exiting");
                        return -EINVAL;
                        break;
                }
            } else
#endif /* WLAN_FEATURE_11BE */
            if (IEEE80211_IS_CHAN_11AX(ic->ic_curchan)) {
                switch(ch_width){
                    case CHWIDTH_20:
                        flags |= IEEE80211_CHAN_HE20;
                        break;
                    case CHWIDTH_40:
                        flags |= IEEE80211_CHAN_HE40PLUS;
                        if (!ieee80211_is_freq_in_obj_valid(ic, chan_p, flags)) {
                            /* HE40PLUS is no good, try minus */
                            flags = ieee80211_get_band_flag(chan_p->freq);
                            flags |= IEEE80211_CHAN_HE40MINUS;
                        }
                        break;
                    case CHWIDTH_80:
                        flags |= IEEE80211_CHAN_HE80;
                        break;
                    case CHWIDTH_160:
                            flags |= IEEE80211_CHAN_HE160;
                        break;
                    default:
                        qdf_err("Invalid channel width input, exiting");
                        return -EINVAL;
                        break;
                }
            } else if (IEEE80211_IS_CHAN_VHT(ic->ic_curchan)){
                switch(ch_width){
                    case CHWIDTH_20:
                        flags |= IEEE80211_CHAN_VHT20;
                        break;
                    case CHWIDTH_40:
                        flags |= IEEE80211_CHAN_VHT40PLUS;
                        if (!ieee80211_is_freq_in_obj_valid(ic,
                                                            chan_p,
                                                            flags)) {
                            /*VHT40PLUS is no good, try minus*/
                            flags = ieee80211_get_band_flag(chan_p->freq);
                            flags |= IEEE80211_CHAN_VHT40MINUS;
                        }
                        break;
                    case CHWIDTH_80:
                        flags |= IEEE80211_CHAN_VHT80;
                        break;
                    case CHWIDTH_160:
                            flags |= IEEE80211_CHAN_VHT160;
                        break;
                    default:
                        qdf_err("Invalid channel width input, exiting");
                        return -EINVAL;
                        break;
                }
            } else if(IEEE80211_IS_CHAN_11N(ic->ic_curchan)){
                switch(ch_width){
                    case CHWIDTH_20:
                        flags |= IEEE80211_CHAN_HT20;
                        break;
                    case CHWIDTH_40:
                        flags |= IEEE80211_CHAN_HT40PLUS;
                        if (!ieee80211_is_freq_in_obj_valid(ic, chan_p,
                                                           flags)) {
                            /*HT40PLUS is no good, try minus*/
                            flags = ieee80211_get_band_flag(chan_p->freq);
                            flags |= IEEE80211_CHAN_HT40MINUS;
                        }
                        break;
                    default:
                        qdf_err("Invalid channel width input, exiting");
                        return -EINVAL;
                        break;
                }
            } else {
                /*legacy doesn't support channel width change*/
                qdf_err("Legacy doesn't support channel width change");
                return -EINVAL;
            }
        } else {
            /* In the case of channel switch only, flags will be same as previous
             * channel */
            flags = ic->ic_curchan->ic_flags;
        }
    }

    *ic->ic_chanchange_channel = ieee80211_find_channel(ic, chan_p, flags);

    if (!IEEE80211_IS_CHAN_VALID(ic->ic_chanchange_channel)) {
        /* Channel is not available for the ch_width */
        qdf_err("Channel is not available for the ch_width");
        return -EINVAL;
    }

    if (WLAN_REG_IS_6GHZ_CHAN_FREQ(chan_p->freq)) {
        if (chan_p->input_6g_pwr_mode == REG_BEST_PWR_MODE) {
            struct ieee80211_ath_channel temp_chan = {0};
            enum ieee80211_phymode phymode;
            uint16_t in_punc_bitmap;

            temp_chan.ic_flags = flags;
            phymode = ieee80211_chan2mode(&temp_chan);
            in_punc_bitmap = ieee80211_get_chan_param_op_punc_bitmap(chan_p);

            if (ieee80211_apply_best_6g_ap_pwr_mode(ic, chan_p->freq,
                                                    ic->ic_chanchange_channel->ic_vhtop_freq_seg2,
                                                    false, phymode, in_punc_bitmap)) {
                qdf_err("Error applying best power mode");
                return -EINVAL;
            }
        }
    }

    if (IEEE80211_ARE_CHANS_EQUAL(ic->ic_chanchange_channel, ic->ic_curchan)) {
        /* If the new and old channels are the same, we are abandoning
         * the channel switch routine and returning without error. */
        qdf_err("Destination and current channels are the same. Exiting without error.");
        *ic->ic_chanchange_channel = empty_channel;
        /* DCS restore needs to be done here, since no beacon update is done in case the
         * old and new channels are same.
         */
        if (ic->ic_dcs_restore)
            ic->ic_dcs_restore(ic);
        return 0;
    }

    if (IEEE80211_IS_CHAN_VALID(ic->ic_chanchange_channel)) {
        *radar_channel = *ic->ic_chanchange_channel;
    } else {
        *radar_channel = ieee80211_find_channel(ic, chan_p, ic->ic_curchan->ic_flags);
    }

    if (IEEE80211_IS_CHAN_VALID(radar_channel)) {
#ifdef WLAN_FEATURE_11BE
        if (IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, radar_channel,
                                                           chan_p->puncture_bitmap))
#else
        if (IEEE80211_IS_CHAN_RADAR(ic, radar_channel))
#endif
        {
        qdf_err("Channel is Radar channel");
        /* Before returning channel switch failure, mark ic_chanchange_channel
         * as NULL.
         */
        *ic->ic_chanchange_channel = empty_channel;
            return -EINVAL;
        }
    } else {
        qdf_err("Channel is null");
        return -EINVAL;
    }

    /* Trigger full channel change if,
     * 1) None of the vaps are in UP state, or
     * 2) If the MBSSID feature is enabled and none of the Tx vaps are in UP
     *    state.
     */
    if (!ieee80211_vaps_ready(ic, IEEE80211_M_HOSTAP) ||
        (ic->is_mbssid_enabled && !ieee80211_get_num_tx_vaps_in_up_state(vap))) {
       qdf_info("Full channel change is invoked");
       ieee80211_ucfg_set_chan_csa(vap, radar_channel);
       *ic->ic_chanchange_channel = empty_channel;
       return 0;
    }

    /* Find destination channel width */
    *ic->ic_chanchange_channel = *radar_channel;
    ic->ic_chanchange_chwidth = ieee80211_get_chan_width(ic->ic_chanchange_channel);
    ic->ic_chanchange_chanflag  = flags;
    ic->ic_chanchange_secoffset = ieee80211_sec_chan_offset(ic, ic->ic_chanchange_channel);

    /*  flag the beacon update to include the channel switch IE */
    ic->ic_chanchange_params.freq = chan_p->freq;
    ic->ic_chanchange_params.cfreq2 = chan_p->cfreq2;
#ifdef WLAN_FEATURE_11BE
    set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, chan_p->op_puncture_bitmap);
#endif
    if (tbtt) {
        ic->ic_chanchange_tbtt = tbtt;
    } else {
        ic->ic_chanchange_tbtt = IEEE80211_RADAR_11HCOUNT;
    }

    if (in_interrupt()) {
        /* In case of interrupt context deliver event directly
         * to vdev mlme sm, avoid serialization as it demands
         * a mutex to be held which is not possible in interrupt
         * context.Need revisit to better handle this scenario.*/
        qdf_info("Invoking CSA from interrupt ctx");
        wlan_pdev_mlme_vdev_sm_csa_restart(pdev, radar_channel);
    } else {
        /* Iterate over VAPs of the pdev and trigger CSA restart procedure */
        qdf_info("Invoking CSA from user ctx");
        wlan_mlme_pdev_csa_restart(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
    }

    /* Set des chan for all VAPs to be same */
    wlan_iterate_vap_list(ic, ieee80211_vap_iter_set_des_channel,
                          (void *)radar_channel);

/* The default value of this variable is false.When NOL violation is reported
 * by FW in vap's start response, during restart of the vap, it will be reset
 * to true in dfs_action. After this if user again tries to set another NOL
 * channel using iwpriv athx doth_chanswitch "NOL_chan" "tbtt_cnt", if is
 * variable is remains to set to true, no action will be taken on vap's start
 * failure from FW. Hence resetting it here.
 */
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    if (vap->vap_start_failure_action_taken)
        vap->vap_start_failure_action_taken = false;
#endif

    return 0;
}

struct ieee80211_ath_channel ieee80211_ucfg_get_current_channel(wlan_if_t vaphandle, bool hwChan)
{
    return wlan_get_current_channel(vaphandle, hwChan);
}

struct ieee80211_ath_channel ieee80211_ucfg_get_bss_channel(wlan_if_t vaphandle)
{
    return wlan_get_bss_channel(vaphandle);
}

int ieee80211_ucfg_delete_vap(wlan_if_t vap)
{
    int status = -1;
    osif_dev *osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osif);

    if (dev) {
#ifdef WLAN_FEATURE_11BE_MLO
        status = osif_ioctl_delete_vap(dev, vap->link_id);
#else
        status = osif_ioctl_delete_vap(dev, INVALID_LINK_ID);
#endif
    }
    return status;
}

int ieee80211_ucfg_set_rts(wlan_if_t vap, u_int32_t val)
{
    osif_dev *osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osif);
    u_int32_t curval;

    curval = wlan_get_param(vap, IEEE80211_RTS_THRESHOLD);
    if (val != curval)
    {
        wlan_set_param(vap, IEEE80211_RTS_THRESHOLD, val);
        if (dev && (IS_UP(dev)))
            return osif_vap_init(osif, dev, RESCAN);
    }

    return 0;
}

int ieee80211_ucfg_set_frag(wlan_if_t vap, u_int32_t val)
{
    osif_dev *osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osif);
    u_int32_t curval;

    if(wlan_get_desired_phymode(vap) < IEEE80211_MODE_11NA_HT20)
    {
        curval = wlan_get_param(vap, IEEE80211_FRAG_THRESHOLD);
        if (val != curval)
        {
            wlan_set_param(vap, IEEE80211_FRAG_THRESHOLD, val);
            if (dev && (IS_UP(dev)))
                return osif_vap_init(osif, dev, RESCAN);
        }
    } else {
        qdf_print("WARNING: Fragmentation with HT mode NOT ALLOWED!!");
        return -EINVAL;
    }

    return 0;
}

int ieee80211_ucfg_set_txpow(wlan_if_t vaphandle, int txpow)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int is2GHz = 0;
    int fixed = (ic->ic_flags & IEEE80211_F_TXPOW_FIXED) != 0;
    struct ieee80211_ath_channel *vap_cur_des_chan = vap->iv_des_chan[vap->iv_des_mode];
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    if (ic->ic_is_ifce_allowed_in_dynamic_mode &&
        !ic->ic_is_ifce_allowed_in_dynamic_mode(scn)) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("IF %s blocked - hw-mode: %d "
                                  "hw-mode-switch-in-progress %s"),
                                  scn->netdev->name,
                                  scn->soc->hw_mode_ctx.current_mode,
                                  scn->soc->hw_mode_ctx.is_switch_in_progress ?
                                                                "YES" : "NO");
        return -EINVAL;
    }

    if (txpow > 0) {
        if ((ic->ic_caps & IEEE80211_C_TXPMGT) == 0)
            return -EINVAL;
        /*
         * txpow is in dBm while we store in 0.5dBm units
         */
        if(ic->ic_set_txPowerLimit) {
        /* The channel is not initialized yet therefore the band(2.4Ghz or 5Ghz)
         * is unknown. Set both the 2.4Ghz and 5Ghz limits.
         */
            if (!IEEE80211_IS_CHAN_VALID(vap_cur_des_chan)) {
                ic->ic_set_txPowerLimit(ic->ic_pdev_obj, 2*txpow, 2*txpow, 1);
                ic->ic_set_txPowerLimit(ic->ic_pdev_obj, 2*txpow, 2*txpow, 0);
            }
            else {
                is2GHz = IEEE80211_IS_CHAN_2GHZ(vap_cur_des_chan);
                ic->ic_set_txPowerLimit(ic->ic_pdev_obj, 2*txpow, 2*txpow, is2GHz);
            }
        }
        ieee80211com_set_flags(ic, IEEE80211_F_TXPOW_FIXED);
    }
    else {
        if (!fixed) return EOK;

        if (!IEEE80211_IS_CHAN_VALID(vap_cur_des_chan)) {
            qdf_info("The channel is not set. Not setting the power value");
            return -EINVAL;
        }

        is2GHz = IEEE80211_IS_CHAN_2GHZ(vap_cur_des_chan);
        if(ic->ic_set_txPowerLimit)
           ic->ic_set_txPowerLimit(ic->ic_pdev_obj,
                                   2 * vap_cur_des_chan->ic_maxregpower,
                                   2 * vap_cur_des_chan->ic_maxregpower, is2GHz);

        ieee80211com_clear_flags(ic, IEEE80211_F_TXPOW_FIXED);
    }
    return EOK;
}

int ieee80211_ucfg_get_txpow(wlan_if_t vaphandle, int *txpow, int *fixed)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    *txpow = (vap->iv_bss) ? ((int16_t)vap->iv_bss->ni_txpower/2) : 0;
    *fixed = (ic->ic_flags & IEEE80211_F_TXPOW_FIXED) != 0;
    return 0;
}

int ieee80211_ucfg_get_txpow_fraction(wlan_if_t vaphandle, int *txpow, int *fixed)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    if (vap->iv_bss)
    qdf_print("vap->iv_bss->ni_txpower = %d", vap->iv_bss->ni_txpower);
   *txpow = (vap->iv_bss) ? ((vap->iv_bss->ni_txpower*100)/2) : 0;
   *fixed = (ic->ic_flags & IEEE80211_F_TXPOW_FIXED) != 0;
    return 0;
}


int ieee80211_ucfg_set_ap(wlan_if_t vap, u_int8_t (*des_bssid)[QDF_MAC_ADDR_SIZE])
{
    osif_dev *osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    u_int8_t zero_bssid[] = { 0,0,0,0,0,0 };
    int status = 0;

    if (wlan_vap_get_opmode(vap) != IEEE80211_M_STA &&
        (u_int8_t)osifp->os_opmode != IEEE80211_M_P2P_DEVICE &&
        (u_int8_t)osifp->os_opmode != IEEE80211_M_P2P_CLIENT) {
        return -EINVAL;
    }

    if (IEEE80211_ADDR_EQ(des_bssid, zero_bssid)) {
        wlan_aplist_init(vap);
    } else {
        status = wlan_aplist_set_desired_bssidlist(vap, 1, des_bssid);
    }
    if (dev && (IS_UP(dev)))
        return osif_vap_init(osifp, dev, RESCAN);

    return status;
}

int ieee80211_ucfg_get_ap(wlan_if_t vap, u_int8_t *addr)
{
    int status = 0;

    static const u_int8_t zero_bssid[QDF_MAC_ADDR_SIZE];
    u_int8_t bssid[QDF_MAC_ADDR_SIZE];

    if(wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        status = wlan_vap_get_bssid(vap, bssid);
        if(status == EOK) {
            IEEE80211_ADDR_COPY(addr, bssid);
        }
    } else {
        IEEE80211_ADDR_COPY(addr, zero_bssid);
    }

    return status;
}
extern  A_UINT32 dscp_tid_map[64];

static const struct
    {
        char *name;
        int mode;
        int elementconfig;
    } mappings[] = {
    {"AUTO",IEEE80211_MODE_AUTO,0x090C09},
    {"11A",IEEE80211_MODE_11A,0x010C09},
    {"11B",IEEE80211_MODE_11B,0x090C09},
    {"11G",IEEE80211_MODE_11G,0x000C09},
    {"FH",IEEE80211_MODE_FH,0x090C09},
    {"TA",IEEE80211_MODE_TURBO_A,0x090C09},
    {"TG",IEEE80211_MODE_TURBO_G,0x090C09},
    {"11NAHT20",IEEE80211_MODE_11NA_HT20,0x010009},
    {"11NGHT20",IEEE80211_MODE_11NG_HT20,0x000009},
    {"11NAHT40PLUS",IEEE80211_MODE_11NA_HT40PLUS,0x010101},
    {"11NAHT40MINUS",IEEE80211_MODE_11NA_HT40MINUS,0x0101FF},
    {"11NGHT40PLUS",IEEE80211_MODE_11NG_HT40PLUS,0x000101},
    {"11NGHT40MINUS",IEEE80211_MODE_11NG_HT40MINUS,0x0001FF},
    {"11NGHT40",IEEE80211_MODE_11NG_HT40,0x000100},
    {"11NAHT40",IEEE80211_MODE_11NA_HT40,0x010100},
    {"11ACVHT20",IEEE80211_MODE_11AC_VHT20,0x010209},
    {"11ACVHT40PLUS",IEEE80211_MODE_11AC_VHT40PLUS,0x010301},
    {"11ACVHT40MINUS",IEEE80211_MODE_11AC_VHT40MINUS,0x0103FF},
    {"11ACVHT40",IEEE80211_MODE_11AC_VHT40,0x010300},
    {"11ACVHT80",IEEE80211_MODE_11AC_VHT80,0x010400},
    {"11ACVHT160",IEEE80211_MODE_11AC_VHT160,0x010500},
    {"11ACVHT80_80",IEEE80211_MODE_11AC_VHT80_80,0x010600},
    {"11AXA_HE20",IEEE80211_MODE_11AXA_HE20,0x010709},
    {"11AXG_HE20",IEEE80211_MODE_11AXG_HE20,0x000709},
    {"11AXA_HE40PLUS",IEEE80211_MODE_11AXA_HE40PLUS,0x010801},
    {"11AXA_HE40MINUS",IEEE80211_MODE_11AXA_HE40MINUS,0x0108FF},
    {"11AXG_HE40PLUS",IEEE80211_MODE_11AXG_HE40PLUS,0x000801},
    {"11AXG_HE40MINUS",IEEE80211_MODE_11AXG_HE40MINUS,0x0008FF},
    {"11AXA_HE40",IEEE80211_MODE_11AXA_HE40,0x010800},
    {"11AXG_HE40",IEEE80211_MODE_11AXG_HE40,0x000800},
    {"11AXA_HE80",IEEE80211_MODE_11AXA_HE80,0x010900},
    {"11AXA_HE160",IEEE80211_MODE_11AXA_HE160,0x010A00},
    {"11AXA_HE80_80",IEEE80211_MODE_11AXA_HE80_80,0x010B00},
#ifdef WLAN_FEATURE_11BE
    {"11BEA_EHT20",IEEE80211_MODE_11BEA_EHT20,0x010d09},
    {"11BEG_EHT20",IEEE80211_MODE_11BEG_EHT20,0x000d09},
    {"11BEA_EHT40PLUS",IEEE80211_MODE_11BEA_EHT40PLUS,0x010e01},
    {"11BEA_EHT40MINUS",IEEE80211_MODE_11BEA_EHT40MINUS,0x010eFF},
    {"11BEG_EHT40PLUS",IEEE80211_MODE_11BEG_EHT40PLUS,0x000e01},
    {"11BEG_EHT40MINUS",IEEE80211_MODE_11BEG_EHT40MINUS,0x000eFF},
    {"11BEA_EHT40",IEEE80211_MODE_11BEA_EHT40,0x010e00},
    {"11BEG_EHT40",IEEE80211_MODE_11BEG_EHT40,0x000e00},
    {"11BEA_EHT80",IEEE80211_MODE_11BEA_EHT80,0x010f00},
    {"11BEA_EHT160",IEEE80211_MODE_11BEA_EHT160,0x011000},
    {"11BEA_EHT320",IEEE80211_MODE_11BEA_EHT320,0x011100},
#endif /* WLAN_FEATURE_11BE */
};

struct elements{
#if _BYTE_ORDER == _BIG_ENDIAN
    char padd;
    char band;
    char bandwidth;
    char extchan;
#else
    char  extchan ;
    char  bandwidth;
    char  band;
    char  padd;
#endif
}  __attribute__ ((packed));

enum  {
      G = 0x0,
      A,
      B = 0x9,
};
enum  {
      HT20 =0x0,
      HT40 =1,
      VHT20 = 2,
      VHT40= 3,
      VHT80=4,
      VHT160=5,
      VHT80_80=6,
      HE20=7,
      HE40=8,
      HE80=9,
      HE160=0xa,
      HE80_80=0xb,
      NONHT=0xc,
#ifdef WLAN_FEATURE_11BE
      EHT20=0xd,
      EHT40=0xe,
      EHT80=0xf,
      EHT160=0x10,
      EHT320=0x11,
#endif /* WLAN_FEATURE_11BE */
};

#define EXT_CHAN_PLUS 0x1
#define EXT_CHAN_MINUS 0xFF
#define INVALID_ELEMENT 0x9
#define DEFAULT_EXT_CHAN 0x0
#ifdef WLAN_FEATURE_11BE
#define MAX_SUPPORTED_MODES 44
#else
#define MAX_SUPPORTED_MODES 33
#endif /* WLAN_FEATURE_11BE */

static int ieee80211_ucfg_set_extchan( wlan_if_t vap, int extchan )
{
      int elementconfig;
      struct elements *elem;
      int i =0;
      enum ieee80211_phymode  phymode;
      phymode = wlan_get_desired_phymode(vap);
      elementconfig = mappings[phymode].elementconfig ;
      elem = (struct elements *)&elementconfig;
      elem->extchan = extchan;
      for( i = 0; i< MAX_SUPPORTED_MODES ; i ++){
          if( elementconfig == mappings[i].elementconfig)
              break;
      }
      if (i == MAX_SUPPORTED_MODES) {
          qdf_print("unsupported config ");
          return -1;
      }

      phymode=i;
      return wlan_set_desired_phymode(vap,phymode);
}

static int ieee80211_ucfg_set_bandwidth( wlan_if_t vap, int bandwidth)
{
      int elementconfig;
      struct elements *elem;
      int i =0;
      enum ieee80211_phymode  phymode;
      struct chan_params chan_p = {0};

      phymode = wlan_get_desired_phymode(vap);
      elementconfig = mappings[phymode].elementconfig ;
      elem = (struct elements *)&elementconfig;
      elem->bandwidth = bandwidth;

      if ((bandwidth == HT20)  ||
          (bandwidth == VHT20) ||
          (bandwidth == HE20)
#ifdef WLAN_FEATURE_11BE
          || (bandwidth == EHT20)
#endif /* WLAN_FEATURE_11BE */
         ) {
          elem->extchan = INVALID_ELEMENT;
      } else if ((bandwidth == HT40)  ||
                 (bandwidth == VHT40) ||
                 (bandwidth == HE40)
#ifdef WLAN_FEATURE_11BE
                 || (bandwidth == EHT40)
#endif /* WLAN_FEATURE_11BE */
                ) {
          if (elem->extchan == INVALID_ELEMENT) {
              elem->extchan = DEFAULT_EXT_CHAN;
          }
      } else if ((bandwidth == VHT80) || (bandwidth == HE80) ||
                 (bandwidth == VHT160) || (bandwidth == HE160) ||
                 (bandwidth == VHT80_80) || (bandwidth == HE80_80)
#ifdef WLAN_FEATURE_11BE
                 || (bandwidth == EHT80) || (bandwidth == EHT160)
                 || (bandwidth == EHT320)
#endif /* WLAN_FEATURE_11BE */
                ) {
          /* If current des_phy_mode is 40 +/- the extchan field of elem will be
             EXT_CHAN_PLUS/EXT_CHAN_MINUS. And when set BW is issued with BW =
             80/160/80P80/320 MHz, extchan should be set to DEFAULT_EXT_CHAN.
             Otherwise BW switch will fail due to invalid/unsupported
             configuration */
          elem->extchan = DEFAULT_EXT_CHAN;
      }

      if (bandwidth == NONHT) {
          elem->extchan = INVALID_ELEMENT;
      }

      for (i = 0; i< MAX_SUPPORTED_MODES; i++) {
          if (elementconfig == mappings[i].elementconfig)
              break;
      }
      if (i == MAX_SUPPORTED_MODES) {
          qdf_print("unsupported config ");
          return -1;
      }

      phymode=i;

      if (ieee80211_is_phymode_40(phymode)) {
          struct ieee80211com *ic = vap->iv_ic;
          struct ieee80211_ath_channel chan_obj = wlan_get_current_channel(vap, false);
          wlan_chan_t chan = &chan_obj;
          int numvaps_up = 0;

          if (!IEEE80211_IS_CHAN_VALID(chan)) {
              qdf_nofl_info("chan is NULL for mode: %d\n", phymode);
              return -EINVAL;
          }

          if (IEEE80211_IS_CHAN_VALID(chan)) {
              numvaps_up = ieee80211_get_num_vaps_up(ic);
              if ((numvaps_up) &&
                  !IEEE80211_ARE_CHANS_EQUAL(chan, ic->ic_curchan)) {
                  *chan = *(ic->ic_curchan);
              }
              chan_p.freq = chan->ic_freq;
              chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
              set_chan_params_puncture_bitmap(&chan_p, 0);
              *chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                                   phymode);
              if (IEEE80211_IS_CHAN_VALID(chan)) {
                  phymode = ieee80211_chan2mode(chan);
              }
          }
      }
      return wlan_set_desired_phymode(vap, phymode);
}

static int ieee80211_ucfg_set_band( wlan_if_t vap, int band )
{
      int elementconfig;
      struct elements *elem;
      int i =0;
      enum ieee80211_phymode  phymode;
      phymode = wlan_get_desired_phymode(vap);
      elementconfig = mappings[phymode].elementconfig ;
      elem = (struct elements *)&elementconfig;
      elem->band = band;

      if((elem->bandwidth == VHT40 || elem->bandwidth == VHT80 || elem->bandwidth == VHT160 || elem->bandwidth == VHT80_80 )&& band == G)
      {
          elem->bandwidth = HT40;
      }
      if(elem->bandwidth == VHT20 && band == G)
      {
          elem->bandwidth = HT20;
      }
      if( band == B )
      {
          elem->bandwidth = NONHT;
          elem->extchan = INVALID_ELEMENT;
      }
      for( i = 0; i< MAX_SUPPORTED_MODES ; i ++){
          if( elementconfig == mappings[i].elementconfig)
              break;
      }
      if (i == MAX_SUPPORTED_MODES) {
          qdf_print("unsupported config ");
          return -1;
      }
      phymode=i;
      return wlan_set_desired_phymode(vap,phymode);
}

int ieee80211_ucfg_get_bandwidth( wlan_if_t vap)
{
      int elementconfig;
      struct elements *elem;
      enum ieee80211_phymode  phymode;
      phymode = wlan_get_current_phymode(vap);
      elementconfig = mappings[phymode].elementconfig ;
      elem = (struct elements *)&elementconfig;
      return(elem->bandwidth);
}
#if ATH_SUPPORT_DSCP_OVERRIDE
int ieee80211_ucfg_vap_get_dscp_tid_map(wlan_if_t vap, u_int8_t tos)
{
#ifdef IPA_OFFLOAD
	return vap->iv_ic->ic_dscp_tid_map[vap->iv_dscp_map_id][(tos >> IP_DSCP_SHIFT) & IP_DSCP_MASK];
#else
     if (vap->iv_dscp_map_id)
         return vap->iv_ic->ic_dscp_tid_map[vap->iv_dscp_map_id][(tos >> IP_DSCP_SHIFT) & IP_DSCP_MASK];
     else
	 return dscp_tid_map[(tos >> IP_DSCP_SHIFT) & IP_DSCP_MASK];
#endif
}

#endif
int ieee80211_ucfg_get_band( wlan_if_t vap)
{
      int elementconfig;
      struct elements *elem;
      enum ieee80211_phymode  phymode;
      phymode = wlan_get_current_phymode(vap);
      elementconfig = mappings[phymode].elementconfig ;
      elem = (struct elements *)&elementconfig;
      return(elem->band);
}

int ieee80211_ucfg_get_extchan( wlan_if_t vap)
{
      int elementconfig;
      struct elements *elem;
      enum ieee80211_phymode  phymode;
      phymode = wlan_get_desired_phymode(vap);
      elementconfig = mappings[phymode].elementconfig ;
      elem = (struct elements *)&elementconfig;
      return(elem->extchan);
}

struct find_wlan_node_req {
    wlan_node_t node;
    int assoc_id;
};

static void
find_wlan_node_by_associd(void *arg, wlan_node_t node)
{
    struct find_wlan_node_req *req = (struct find_wlan_node_req *)arg;
    if (req->assoc_id == IEEE80211_AID(wlan_node_get_associd(node))) {
        req->node = node;
    }
}

#define IEEE80211_BINTVAL_IWMAX       3500   /* max beacon interval */
#define IEEE80211_BINTVAL_IWMIN       40     /* min beacon interval */
#define IEEE80211_BINTVAL_LP_IOT_IWMIN 25    /* min beacon interval for LP IOT */
#define IEEE80211_SUBTYPE_TXPOW_SHIFT   8     /* left shift 8 bit subtype + txpower as combined value  */
#define IEEE80211_FRAMETYPE_TXPOW_SHIFT   16


int ieee80211_ucfg_set_beacon_interval(wlan_if_t vap, struct ieee80211com *ic,
        int value, bool is_vap_restart_required)
{
    int retv = 0;
    if (vap->iv_create_flags & IEEE80211_LP_IOT_VAP) {
        if (value > IEEE80211_BINTVAL_IWMAX || value < IEEE80211_BINTVAL_LP_IOT_IWMIN) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                    "BEACON_INTERVAL should be within %d to %d\n",
                    IEEE80211_BINTVAL_LP_IOT_IWMIN,
                    IEEE80211_BINTVAL_IWMAX);
            return -EINVAL;
        }
    } else if (ieee80211_vap_oce_check(vap)) {
        if (value > IEEE80211_BINTVAL_MAX || value < IEEE80211_BINTVAL_MIN) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                    "BEACON_INTERVAL should be within %d to %d\n",
                    IEEE80211_BINTVAL_MIN, IEEE80211_BINTVAL_MAX);
            return -EINVAL;
        }
    } else if (value > IEEE80211_BINTVAL_IWMAX || value < IEEE80211_BINTVAL_IWMIN) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                "BEACON_INTERVAL should be within %d to %d\n",
                IEEE80211_BINTVAL_IWMIN,
                IEEE80211_BINTVAL_IWMAX);
        return -EINVAL;
    }
    retv = wlan_set_param(vap, IEEE80211_BEACON_INTVAL, value);
    if (retv == EOK) {
        wlan_if_t tmpvap;
        u_int8_t lp_vap_is_present = 0;
        u_int16_t lp_bintval = ic->ic_intval;

        /* Iterate to find if a LP IOT vap is there */
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (tmpvap->iv_create_flags & IEEE80211_LP_IOT_VAP) {
                lp_vap_is_present = 1;
                /* If multiple lp iot vaps are present pick the least */
                if (lp_bintval > tmpvap->iv_bss->ni_intval)  {
                    lp_bintval = tmpvap->iv_bss->ni_intval;
                }
            }
        }

        /* Adjust regular beacon interval in ic to be a multiple of lp_iot beacon interval */
        if (lp_vap_is_present) {
            UP_CONVERT_TO_FACTOR_OF(ic->ic_intval, lp_bintval);
        }

        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            /* Adjust regular beacon interval in ni to be a multiple of lp_iot beacon interval */
            if (lp_vap_is_present) {
                if (!(tmpvap->iv_create_flags & IEEE80211_LP_IOT_VAP)) {
                    /* up convert vap beacon interval to a factor of LP vap */
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                            "Current beacon interval %d: Checking if up conversion is needed as lp_iot vap is present. ", ic->ic_intval);
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                            "New beacon interval  %d \n", ic->ic_intval);
                    UP_CONVERT_TO_FACTOR_OF(tmpvap->iv_bss->ni_intval, lp_bintval);
                }
            }
        }

        if (is_vap_restart_required
#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
               && !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)
#endif
                 )
            retv = osif_pdev_restart_vaps(ic);
        else
            retv = 0;
    }

    return retv;
}
static uint32_t ieee80211_get_prbrsp_en_period(uint32_t value1, uint32_t value2)
{
    if (value1) {
        value1 = (1 << WLAN_BCAST_PRB_RSP_ENABLE_BIT);
        value2 &= WLAN_BCAST_PRB_RSP_PERIOD_MASK;
    } else {
        value2 = 0;
    }
    return (value1 | value2);
}

void osif_radio_activity_update(struct ol_ath_softc_net80211 *scn)
{
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211vap *vap;
    ol_txrx_soc_handle soc_txrx_handle;
    cdp_config_param_type value = {0};
    struct global_ic_list *ic_list = NULL;
    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_txrx_get_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
            CDP_CONFIG_DELAY_STATS, &value);

    ic_list = ic->ic_global_list;
#if DBDC_REPEATER_SUPPORT
    if (!value.cdp_pdev_param_cfg_delay_stats &&
	((ic_list->num_stavaps_up <= 1) ||
	wlan_mlo_get_sta_mld_ctx_count() <= 1))
#else
    if (!value.cdp_pdev_param_cfg_delay_stats)
#endif
       return;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
        if (vap) {
             ((osif_dev *)vap->iv_ifp)->wifi3_0_rx_fast_path = 0;
             ((osif_dev *)vap->iv_ifp)->wifi3_0_fast_path = 0;
	}
}
qdf_export_symbol(osif_radio_activity_update);

static inline uint8_t
osif_vap_derive_fast_path_flags(wlan_if_t vap,
                                struct wlan_objmgr_vdev *vdev,
                                osif_dev *osdev_priv)
{
    uint8_t wifi3_0_fast_path = 0;
    enum QDF_OPMODE opmode;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    cdp_config_param_type value = {0};
#if DBDC_REPEATER_SUPPORT
    struct ieee80211com *tmp_ic = NULL;
    uint8_t i = 0;
   struct global_ic_list *ic_list = NULL;
#endif

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        qdf_info("pdev is NULL");
        return 0;
    }

    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        qdf_info("psoc is NULL");
        return 0;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    wifi3_0_fast_path = 0;

    opmode = wlan_vdev_mlme_get_opmode(vdev);
    if ((opmode == QDF_SAP_MODE) || (opmode == QDF_STA_MODE))
        wifi3_0_fast_path = WIFI_3_0_FASTPATH | WIFI_3_0_ULTRA_FASTPATH;
#if ATH_SUPPORT_WRAP
    if (wlan_rptr_vdev_is_wrap(vap->vdev_obj) ||
        wlan_rptr_vdev_is_mpsta(vap->vdev_obj) ||
        wlan_rptr_vdev_is_psta(vap->vdev_obj)) {
        wifi3_0_fast_path = 0;
    }
#endif

#if MESH_MODE_SUPPORT
    if (vap->iv_mesh_vap_mode) {
        wifi3_0_fast_path = 0;
    }
#endif
    if ((opmode == QDF_STA_MODE) && wlan_rptr_vdev_is_extap(vap->vdev_obj)) {
        wifi3_0_fast_path = 0;
    }

#ifdef QCA_PEER_EXT_STATS
    cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_CFG_PEER_EXT_STATS, &value);
    if (value.cdp_psoc_param_pext_stats) {
        wifi3_0_fast_path = 0;
    }
#endif /* QCA_PEER_EXT_STATS */

    if (wlan_get_param(vap, IEEE80211_VAP_TX_ENCAP_TYPE) !=
                       htt_cmn_pkt_type_ethernet) {
        wifi3_0_fast_path = 0;
    }

    if (wlan_get_param(vap, IEEE80211_CONFIG_ENABLE_MULTI_GROUP_KEY)) {
        wifi3_0_fast_path = 0;
    }

    if (wlan_get_param(vap, IEEE80211_WNM_TFS_CAP)) {
        wifi3_0_fast_path = 0;
    }

#if ATH_SUPPORT_NAC
    if (IEEE80211_IS_SMART_MON_VAP(vap)) {
        wifi3_0_fast_path = 0;
    }
#endif

    if (IEEE80211_IS_SPECIAL_VAP(vap)) {
        wifi3_0_fast_path = 0;
    }

#if DBDC_REPEATER_SUPPORT
    ic_list = vap->iv_ic->ic_global_list;
    if ((vap->iv_opmode == IEEE80211_M_STA) &&
        (ic_list->num_stavaps_up > 1) &&
        ((!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj)) ||
        wlan_mlo_get_sta_mld_ctx_count() > 1)) {
        for (i = 0; i < MAX_RADIO_CNT; i++) {
            tmp_ic = ic_list->global_ic[i];
            if (tmp_ic && tmp_ic->ic_sta_vap) {
                osif_dev *dev;
                void *osif_dev_handle;
                struct vdev_osif_priv *tmp_vdev_osifp = NULL;
                tmp_vdev_osifp = wlan_vdev_get_ospriv(tmp_ic->ic_sta_vap->vdev_obj);
                osif_dev_handle = tmp_vdev_osifp->legacy_osif_priv;
                dev = (osif_dev *)osif_dev_handle;
                dev->wifi3_0_fast_path = 0;
            }
        }
    }
#endif

    cdp_txrx_get_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(pdev),
                            CDP_CONFIG_DELAY_STATS, &value);
    if (value.cdp_pdev_param_cfg_delay_stats) {
        wifi3_0_fast_path = 0;
    }

    cdp_txrx_get_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(pdev),
                            CDP_CONFIG_VOW, &value);
    if (value.cdp_pdev_param_cfg_vow) {
        wifi3_0_fast_path = 0;
    }

    cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_ENABLE_RATE_STATS, &value);
    if (value.cdp_psoc_param_en_rate_stats) {
        wifi3_0_fast_path = 0;
    }

    cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_CFG_PEER_JITTER_STATS, &value);
    if (value.cdp_psoc_param_jitter_stats) {
        wifi3_0_fast_path = 0;
    }
    cdp_txrx_get_vdev_param(soc_txrx_handle, wlan_vdev_get_id(vap->vdev_obj),
                            CDP_ENABLE_NAWDS, &value);
    if (value.cdp_vdev_param_nawds) {
        wifi3_0_fast_path &= ~WIFI_3_0_ULTRA_FASTPATH;
    }

#ifdef VDEV_PEER_PROT_COUNT
    if (osdev_priv->peer_protocol_cnt) {
        wifi3_0_fast_path = 0;
    }
#endif

#ifdef CONFIG_SAWF
    if (cfg_get(psoc, CFG_DP_SAWF_STATS)) {
        wifi3_0_fast_path = 0;
    }
#endif

#if UMAC_VOW_DEBUG
    if (osdev_priv->vow_dbg_en) {
        wifi3_0_fast_path = 0;
    }
#endif

#if UMAC_SUPPORT_VI_DBG
    if (osdev_priv->vi_dbg) {
        wifi3_0_fast_path = 0;
    }
#endif
    qdf_debug("Updating FAST tx flag to %u for vdev id:%u",
              wifi3_0_fast_path, vdev->vdev_objmgr.vdev_id);
    return wifi3_0_fast_path;
}

void osif_vap_tx_activity_update(wlan_if_t vap)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct vdev_osif_priv *vdev_osifp = NULL;
    void *osifp_handle = NULL;
    osif_dev *osdev_priv = NULL;
    uint8_t fast_path_flags;

    vdev = vap->vdev_obj;

    if (!vdev) {
        qdf_info("vdev is NULL");
        return;
    }

    vdev_osifp = wlan_vdev_get_ospriv(vdev);
    osifp_handle = vdev_osifp->legacy_osif_priv;
    osdev_priv = (osif_dev *)osifp_handle;
    if (!osdev_priv) {
        qdf_info("osdev_priv is NULL for vdev id: %u",
                 vdev->vdev_objmgr.vdev_id);
        return;
    }

    fast_path_flags = osif_vap_derive_fast_path_flags(vap, vdev, osdev_priv);

    if (osdev_priv->wifi3_0_fast_path != fast_path_flags)
        osdev_priv->wifi3_0_fast_path = fast_path_flags;
}

qdf_export_symbol(osif_vap_tx_activity_update);

void osif_vap_activity_update(wlan_if_t vap)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct global_ic_list *ic_list = NULL;
    struct ieee80211com *ic = NULL;
    uint32_t target_type;
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    cdp_config_param_type value = {0};
    enum QDF_OPMODE opmode;
    struct vdev_osif_priv *vdev_osifp = NULL;
    void *osifp_handle;
    osif_dev *osdev;
#if DBDC_REPEATER_SUPPORT
    struct ieee80211com *tmp_ic = NULL;
    uint8_t i = 0;
#endif

    ic = vap->iv_ic;
    vdev = vap->vdev_obj;

    if (!vdev) {
        qdf_info("vdev is NULL");
        return;
    }

    vdev_osifp = wlan_vdev_get_ospriv(vdev);
    osifp_handle = vdev_osifp->legacy_osif_priv;
    osdev = (osif_dev *)osifp_handle;
    psoc = wlan_vdev_get_psoc(vdev);

    if (!psoc || !osdev) {
        qdf_info("psoc or osdev is NULL");
        return;
    }

    target_type = ic->ic_get_tgt_type(ic);
    opmode = wlan_vdev_mlme_get_opmode(vdev);
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_CFG_PEER_EXT_STATS, &value);
    osdev->wifi3_0_rx_fast_path = 0;

    /*
     * 1. This sets up rx_fast path flag for features which are known
     *    at ol_ath_vap_create_post_init. These features need to be setup
     *    along with vap up eg: iv_wrap, iv_mesh_vap_mode, iv_smart_monitor_vap.
     * 2. We will call this function from multiple iwprivs to enable/disable
     *    this flag for features which are controlled on the fly using iwprivs.
     */
    if ((target_type != TARGET_TYPE_QCA8074V2) &&
        (target_type != TARGET_TYPE_QCA9574) &&
        (target_type != TARGET_TYPE_QCA6018) &&
        (target_type != TARGET_TYPE_QCA5332) &&
        (target_type != TARGET_TYPE_QCA5018) &&
        (target_type != TARGET_TYPE_QCN9000) &&
        (target_type != TARGET_TYPE_QCN6432) &&
        (target_type != TARGET_TYPE_QCN9224)) {
            osdev->wifi3_0_rx_fast_path = 0;
            return;
    }

    if ((opmode == QDF_SAP_MODE || opmode == QDF_STA_MODE))
        osdev->wifi3_0_rx_fast_path = 1;

#if ATH_SUPPORT_WRAP
    if (wlan_rptr_vdev_is_wrap(vap->vdev_obj) || wlan_rptr_vdev_is_mpsta(vap->vdev_obj) || wlan_rptr_vdev_is_psta(vap->vdev_obj)) {
        osdev->wifi3_0_rx_fast_path = 0;
    }
#endif

#if MESH_MODE_SUPPORT
    if (vap->iv_mesh_vap_mode) {
        osdev->wifi3_0_rx_fast_path = 0;
    }
#endif

#if ATH_SUPPORT_NAC
    if (IEEE80211_IS_SMART_MON_VAP(vap)) {
        osdev->wifi3_0_rx_fast_path = 0;
    }
#endif

#if CONFIG_DP_TRACE || QCA_PARTNER_PLATFORM || ATH_DATA_RX_INFO_EN
    osdev->wifi3_0_rx_fast_path = 0;
#endif

    if (osdev->vlanID)
        osdev->wifi3_0_rx_fast_path = 0;

#if UMAC_VOW_DEBUG
    if(osdev->vow_dbg_en)
        osdev->wifi3_0_rx_fast_path = 0;
#endif

    if(wlan_rptr_vdev_is_extap(vdev))
        osdev->wifi3_0_rx_fast_path = 0;

#ifdef QCA_PEER_EXT_STATS
    if (value.cdp_psoc_param_pext_stats) {
        osdev->wifi3_0_rx_fast_path = 0;
    }
#endif

#ifdef CONFIG_SAWF
    if (cfg_get(psoc, CFG_DP_SAWF_STATS))
        osdev->wifi3_0_fast_path = 0;
#endif /* CONFIG_SAWF */

    ic_list = vap->iv_ic->ic_global_list;
#if DBDC_REPEATER_SUPPORT
    if ((vap->iv_opmode == IEEE80211_M_STA) &&
        (ic_list->num_stavaps_up > 1) &&
        ((!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj)) ||
        wlan_mlo_get_sta_mld_ctx_count() > 1)) {
        for (i = 0; i < MAX_RADIO_CNT; i++) {
            tmp_ic = ic_list->global_ic[i];
            if (tmp_ic && tmp_ic->ic_sta_vap) {
                osif_dev *dev;
                void *osif_dev_handle;
                struct vdev_osif_priv *tmp_vdev_osifp = NULL;
                tmp_vdev_osifp = wlan_vdev_get_ospriv(tmp_ic->ic_sta_vap->vdev_obj);
                osif_dev_handle = tmp_vdev_osifp->legacy_osif_priv;
                dev = (osif_dev *)osif_dev_handle;
                dev->wifi3_0_rx_fast_path = 0;
            }
        }
    }
#endif
}
qdf_export_symbol(osif_vap_activity_update);

QDF_STATUS ieee80211_configure_rtt_modes(struct vdev_mlme_obj *vdev_mlme,
        int value, struct wlan_vdev_mgr_cfg mlme_cfg)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    if (value & RTT_RESPONDER_MODE) {
        status = vdev_mlme_set_param(vdev_mlme,
                WLAN_MLME_CFG_ENABLE_DISABLE_RTT_RESPONDER_ROLE, mlme_cfg);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("Failed to send RTT responder value to FW, value = %d",
                    mlme_cfg.value);
            return status;
        }
    }

    if (value & RTT_INITIATOR_MODE) {
        status = vdev_mlme_set_param(vdev_mlme,
                WLAN_MLME_CFG_ENABLE_DISABLE_RTT_INITIATOR_ROLE, mlme_cfg);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("Failed to send RTT initiator value to FW, value = %d",
                    mlme_cfg.value);
            return status;
        }
    }

    return status;
}

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
uint32_t ieee80211_get_responder_role(struct wlan_objmgr_psoc *psoc)
{
    uint8_t enable_11az;
    int value = 0;

    /* Enable the 11mc responde mode if it is enabled in the ini config */
    value = cfg_get(psoc, CFG_RESPONDER_11MC_SUPPORT);

    enable_11az = cfg_get(psoc, CFG_RESPONDER_11AZ_SUPPORT);

    /* Enable the 11az NTB responder mode if,
     * 1) 11az NTB responder mode is enabled using ini config, and
     * 2) FW supports the 11az NTB responder mode
     */
    if ((enable_11az & CFG_RESPONDER_11AZ_NTB_SUPPORT) &&
        wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_RTT_11AZ_NTB_SUPPORT))
        value |= CFG_RESPONDER_11AZ_NTB_SUPPORT << 1;

    /* Enable the 11az TB responder mode if,
     * 1) 11az TB responder mode is enabled using ini config, and
     * 2) FW supports the 11az TB responder mode
     */
    if ((enable_11az & CFG_RESPONDER_11AZ_TB_SUPPORT) &&
        wlan_psoc_nif_fw_ext2_cap_get(psoc, WLAN_RTT_11AZ_TB_RSTA_SUPPORT))
        value |= CFG_RESPONDER_11AZ_TB_SUPPORT << 1;

    return value;
}
#else
uint32_t ieee80211_get_responder_role(struct wlan_objmgr_psoc *psoc)
{
    return cfg_get(psoc, CFG_RESPONDER_11MC_SUPPORT);
}
#endif
qdf_export_symbol(ieee80211_get_responder_role);

QDF_STATUS ieee80211_configure_rtt_responder_role(
        struct ieee80211vap *vap, uint32_t value)
{
    struct wlan_objmgr_psoc *psoc;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;
    struct wlan_vdev_mgr_cfg mlme_cfg = {0};
    uint32_t responder_role = 0;
    QDF_STATUS status;

    if (!vdev_mlme) {
        qdf_err("null vdev_mlme");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!vap->iv_ic || !vap->iv_ic->ic_pdev_obj) {
        qdf_err("null ic/pdev");
        return QDF_STATUS_E_NULL_VALUE;
    }

    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);
    if (!psoc) {
        qdf_err("null psoc");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* Get the ini configured and FW advertised RTT responder modes */
    responder_role = ieee80211_get_responder_role(psoc);

    /* Send the 11mc responder role to FW if 11mc is enabled using ini config,
     * FW and it is enabled using set_rtt_responder_role command.
     */
    if (responder_role & value & RTT_11MC_RESPONDER_ROLE)
        mlme_cfg.value = RTT_11MC_RESPONDER_ROLE;

    /* Send the 11az NTB responder role to FW if 11az NTB is enabled using ini
     * config, FW and it is enabled using set_rtt_responder_role command.
     */
    if (responder_role & value & RTT_11AZ_NTB_RESPONDER_ROLE)
        mlme_cfg.value |= RTT_11AZ_NTB_RESPONDER_ROLE;

    /* Send the 11az TB responder role to FW if 11az TB is enabled using ini
     * config, FW and it is enabled using set_rtt_responder_role command.
     */
    if (responder_role & value & RTT_11AZ_TB_RESPONDER_ROLE)
        mlme_cfg.value |= RTT_11AZ_TB_RESPONDER_ROLE;

    vap->rtt_responder_role = mlme_cfg.value;

    status = vdev_mlme_set_param(vdev_mlme,
            WLAN_MLME_CFG_ENABLE_DISABLE_RTT_RESPONDER_ROLE, mlme_cfg);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Failed to send RTT responder value to FW, value = %d",
                mlme_cfg.value);
        return status;
    }

    qdf_info("vap:%s configured RTT responder role is: 0x%x",
             vap->iv_netdev_name, mlme_cfg.value);

    return QDF_STATUS_SUCCESS;
}

#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
bool ieee80211_ucfg_is_mbss_common_param(wlan_if_t vap, int param)
{
	switch (param)
	{
		case IEEE80211_PARAM_SHORT_GI:
		case IEEE80211_PARAM_SHORT_GI20:
		case IEEE80211_PARAM_SHORT_GI40:
		case IEEE80211_PARAM_BEACON_INTERVAL:
		case IEEE80211_PARAM_COUNTRY_IE:
		case IEEE80211_PARAM_AMPDU:
		case IEEE80211_PARAM_AMSDU:
		case IEEE80211_PARAM_HT40_INTOLERANT:
		case IEEE80211_PARAM_VAP_COUNTRY_IE:
		case IEEE80211_PARAM_MAX_AMPDU:
		case IEEE80211_PARAM_VHT_MAX_AMPDU:
		case IEEE80211_PARAM_11N_TX_AMSDU:
		case IEEE80211_PARAM_VHT_MCS:
		case IEEE80211_PARAM_NSS:
		case IEEE80211_PARAM_LDPC:
		case IEEE80211_PARAM_TX_STBC:
		case IEEE80211_PARAM_RX_STBC:
		case IEEE80211_PARAM_VHT_SGIMASK:
		case IEEE80211_PARAM_VHT80_RATEMASK:
		case IEEE80211_PARAM_VHT_STS_CAP:
		case IEEE80211_PARAM_VHT_SOUNDING_DIM:
		case IEEE80211_PARAM_VHT_SUBFEE:
		case IEEE80211_PARAM_VHT_MUBFEE:
		case IEEE80211_PARAM_VHT_SUBFER:
		case IEEE80211_PARAM_VHT_MUBFER:
		case IEEE80211_PARAM_IMPLICITBF:
		case IEEE80211_PARAM_BW_NSS_RATEMASK:
		case IEEE80211_PARAM_VHT_TX_MCSMAP:
		case IEEE80211_PARAM_VHT_RX_MCSMAP:
		case IEEE80211_PARAM_AMPDU_DENSITY_OVERRIDE:
		case IEEE80211_PARAM_DISABLE_SELECTIVE_HTMCS_FOR_VAP:
		case IEEE80211_PARAM_CONFIGURE_SELECTIVE_VHTMCS_FOR_VAP:
		case IEEE80211_PARAM_CHANNEL_SWITCH_MODE:
		case IEEE80211_PARAM_ENABLE_ECSA_IE:
		case IEEE80211_PARAM_ECSA_OPCLASS:
		case IEEE80211_PARAM_HE_MCS:
		case IEEE80211_PARAM_HE_EXTENDED_RANGE:
		case IEEE80211_PARAM_HE_DCM:
		case IEEE80211_PARAM_HE_FRAGMENTATION:
		case IEEE80211_PARAM_HE_DL_MU_OFDMA:
		case IEEE80211_PARAM_HE_UL_MU_MIMO:
		case IEEE80211_PARAM_HE_UL_MU_OFDMA:
		case IEEE80211_PARAM_HE_SU_BFEE:
		case IEEE80211_PARAM_HE_SU_BFER:
		case IEEE80211_PARAM_HE_MU_BFEE:
		case IEEE80211_PARAM_HE_MU_BFER:
		case IEEE80211_PARAM_EXT_NSS_SUPPORT:
		case IEEE80211_PARAM_HE_LTF:
		case IEEE80211_PARAM_HE_RTSTHRSHLD:
		case IEEE80211_PARAM_HE_TX_MCSMAP:
		case IEEE80211_PARAM_HE_RX_MCSMAP:
		case IEEE80211_PARAM_HE_AR_GI_LTF:
		case IEEE80211_PARAM_HE_SOUNDING_MODE:
		case IEEE80211_PARAM_HE_HT_CTRL:
		case IEEE80211_PARAM_HE_UL_SHORTGI:
		case IEEE80211_PARAM_HE_UL_LTF:
		case IEEE80211_PARAM_HE_UL_NSS:
		case IEEE80211_PARAM_HE_UL_PPDU_BW:
		case IEEE80211_PARAM_HE_UL_LDPC:
		case IEEE80211_PARAM_HE_UL_STBC:
		case IEEE80211_PARAM_HE_UL_FIXED_RATE:
		case IEEE80211_PARAM_HE_BSR_SUPPORT:
		case IEEE80211_PARAM_HE_AMSDU_IN_AMPDU_SUPRT:
		case IEEE80211_PARAM_HE_SUBFEE_STS_SUPRT:
		case IEEE80211_PARAM_HE_4XLTF_800NS_GI_RX_SUPRT:
		case IEEE80211_PARAM_HE_1XLTF_800NS_GI_RX_SUPRT:
		case IEEE80211_PARAM_HE_MAX_NC_SUPRT:
		case IEEE80211_PARAM_TWT_RESPONDER_SUPRT:
		case IEEE80211_PARAM_HE_MULTI_TID_AGGR:
		case IEEE80211_PARAM_HE_MULTI_TID_AGGR_TX:
		case IEEE80211_PARAM_HE_MAX_AMPDU_LEN_EXP:
		case IEEE80211_PARAM_HE_SU_PPDU_1X_LTF_800NS_GI:
		case IEEE80211_PARAM_HE_SU_MU_PPDU_4X_LTF_800NS_GI:
		case IEEE80211_PARAM_HE_MAX_FRAG_MSDU:
		case IEEE80211_PARAM_HE_MIN_FRAG_SIZE:
		case IEEE80211_PARAM_HE_NDP_4X_LTF_3200NS_GI:
		case IEEE80211_PARAM_HE_ER_SU_PPDU_1X_LTF_800NS_GI:
		case IEEE80211_PARAM_HE_ER_SU_PPDU_4X_LTF_800NS_GI:
		case IEEE80211_PARAM_HE_AR_LDPC:
		case IEEE80211_PARAM_HE_DL_MU_OFDMA_BFER:
		case IEEE80211_PARAM_6G_HE_OP_MIN_RATE:
		case IEEE80211_PARAM_HE_ER_SU_DISABLE:
		case IEEE80211_PARAM_HE_1024QAM_LT242RU_RX_ENABLE:
		case IEEE80211_PARAM_HE_UL_MU_DATA_DIS_RX_SUPP:
		case IEEE80211_PARAM_HE_FULL_BW_UL_MUMIMO:
		case IEEE80211_PARAM_HE_DCM_MAX_CONSTELLATION_RX:
		case IEEE80211_PARAM_SM_GAP_PS_ENABLE:
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_PARAM_EHT_NDP_4X_EHT_LTF_AND_320NSGI:
		case IEEE80211_PARAM_EHT_SU_BFMR:
		case IEEE80211_PARAM_EHT_SU_BFME:
		case IEEE80211_PARAM_EHT_BFME_SS:
		case IEEE80211_PARAM_EHT_NUM_SD:
		case IEEE80211_PARAM_EHT_4X_EHT_LTF_AND_800NS_GI:
		case IEEE80211_PARAM_EHT_MAX_NC:
		case IEEE80211_PARAM_EHT_RX_1024_AND_4096_QAM_LS_242_TONE_RU:
		case IEEE80211_PARAM_EHT_MU_BFMR:
                case IEEE80211_PARAM_EHT_UL_NSS:
                case IEEE80211_PARAM_EHT_UL_PPDU_BW:
		case IEEE80211_PARAM_EHT_UL_SHORTGI:
		case IEEE80211_PARAM_EHT_UL_LDPC:
		case IEEE80211_PARAM_EHT_UL_STBC:
		case IEEE80211_PARAM_EHT_UL_FIXED_RATE:
		case IEEE80211_PARAM_EHT_UL_LTF:
		case IEEE80211_PARAM_EHT_LTF:
		case IEEE80211_PARAM_EHT_SUP_MCS15:
		case IEEE80211_PARAM_EHT_MCS14_DUP_IN_6GHZ:
		case IEEE80211_PARAM_EHT_CONFIG_CCFS0:
		case IEEE80211_PARAM_EHT_MAX_SUPP_LTF:
#endif
			return true;
		default:
			return false;
	}
}
#endif

/* ieee80211_update_vap_resource_profile: Update space reserved for
 * vendor and optional IEs for non-Tx VAP in MBSSID non-Tx profile.
 * @vap:             pointer to vap handle
 * @des_ven_ie_size: desired vendor IE size in bytes
 * @des_opt_ie_size: desired optional IE size in bytes
 * Return: 0 on success else error val
 */
static int ieee80211_update_vap_resource_profile(struct ieee80211vap *vap,
                                                 int des_ven_ie_size,
                                                 int des_opt_ie_size)
{
    uint32_t max_allowed_vendor_ie_size;
    struct ieee80211com *ic = vap->iv_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    uint32_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;

    if (!ic->ic_mbss_ctx) {
        qdf_err("mbss_ctx is null!");
        return -EINVAL;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    max_allowed_vendor_ie_size = ema_ctx->vendor_ie_size_limit;

    if(!ic->is_mbssid_enabled) {
        qdf_info("MBSSID disabled so no non-Tx vap config necessary");
        return -EINVAL;
    }

    if (ieee80211_mbssid_get_num_beaconing_ap_vaps_up(ic, grp_id)) {
        qdf_err("All Vaps must be in down state");
        return -EINVAL;
    } else if (vap == ic_mbss->transmit_vap) {
        qdf_err("Vap resource profile update only allowed for non-tx vaps");
        return -EINVAL;
    } else if (des_ven_ie_size > max_allowed_vendor_ie_size) {
        qdf_err("Vendor IE size should be 0 <= val <= %d",
                max_allowed_vendor_ie_size);
        return -EINVAL;
    } else if (des_ven_ie_size + des_opt_ie_size >
                   mbss_ctx->non_tx_profile_size) {
        qdf_err("Vendor IE size + Optional IE size"
                " must be <= max profile size: %d",
                mbss_ctx->non_tx_profile_size);
        return -EINVAL;
    } else {
        vap->iv_mbss.total_vendor_ie_size = des_ven_ie_size;
        vap->iv_mbss.total_optional_ie_size = des_opt_ie_size;
        IEEE80211_EMA_MBSS_FLAGS_SET(vap->iv_mbss.flags,
                             IEEE80211_EMA_MBSS_FLAGS_USER_CONFIGD_RSRC_PFL);
        vap->iv_mbss.ie_overflow = false;
    }
    return 0;
}

/* This API returns true if configured RTT mode is supported. Else, false.
 */
bool ieee80211_is_configured_rtt_mode_allowed(struct wlan_objmgr_psoc *psoc,
        struct ieee80211com *ic, uint32_t value)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    /* Responder mode is supported in both burst and staggerd beacon mode.
     * Hence, allow the responder mode.
     */
    if ((value == DISABLE_RTT_RESPONDER_AND_INITIATOR_MODE) ||
        (value == RTT_RESPONDER_MODE))
        return true;

    /* Initiator support in beacon burst and staggered mode controlled by
     * FW. Hence, check the FW provided service bit.
     */
    if (((scn->bcn_mode == BEACON_TX_MODE_BURST) &&
         wlan_psoc_nif_fw_ext_cap_get(psoc,
             WLAN_SOC_RTT_AP_INITIATOR_BURSTED_MODE_SUPPORTED)) ||
        ((scn->bcn_mode == BEACON_TX_MODE_STAGGERED) &&
         wlan_psoc_nif_fw_ext_cap_get(psoc,
             WLAN_SOC_RTT_AP_INITIATOR_STAGGERED_MODE_SUPPORTED)))
        return true;

    return false;
}

int ieee80211_ucfg_set_peer_tid_noack_config(struct ieee80211com *ic,
                                             struct wlan_objmgr_pdev *pdev,
                                             uint32_t vdev_id,
                                             uint8_t *macaddr,
                                             uint16_t noack_map)
{
    if (!ic)
        return -EINVAL;

    return ic->ic_set_peer_tid_noack_config(pdev, vdev_id, macaddr, noack_map);
}

int ieee80211_ucfg_setparam_util(wlan_if_t vap, int param, int value,
                                 char *extra, bool is_cmn_param, bool is_tx_vap)
{
    osif_dev  *osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    wlan_dev_t ic = wlan_vap_get_devhandle(vap);
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct wlan_objmgr_psoc *psoc;
    int retv = 0;
    int error = 0;
    int prev_state = 0;
    int new_state = 0;
    int *val = (int*)extra;
    int basic_valid_mask = 0;
    struct _rate_table {
        int *rates;
        int nrates;
    }rate_table;
    int found = 0;
    uint8_t frame_type ;
    u_int8_t transmit_power = 0;
    u_int32_t loop_index;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status;
    uint8_t skip_restart;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    uint32_t prb_rsp_en_period = 0;
#ifdef WLAN_SUPPORT_FILS
    uint32_t fils_en_period = 0;
#endif /* WLAN_SUPPORT_FILS */
    uint32_t ldpc = 0;
    uint8_t restart_vap = false;
    struct ieee80211vap *tmpvap = NULL;
    cdp_config_param_type buf = {0};
    struct wlan_vdev_mgr_cfg mlme_cfg = {0};
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;
    bool is_mbssid_enabled = false;
    uint16_t start_aid = 0;
    bool twt_responder = false;
    bool broadcast_twt = false;
#ifdef WLAN_FEATURE_11BE
    bool restricted_twt = false;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_vdev_aid_mgr *aid_mgr = NULL;
#endif
    bool is_ndev_up = false;

    pdev = ic->ic_pdev_obj;
    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);

    is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(pdev, WLAN_PDEV_F_MBSS_IE_ENABLE);
    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap))
        config_cmd_resp_log(scn->soc, CONFIG_TYPE_CMD, vap->iv_netdev_name, param, value);

    switch (param)
    {
    case IEEE80211_PARAM_PEER_TX_MU_BLACKLIST_COUNT:
        if (value <= 0) {
           qdf_print("Invalid AID value.");
           return -EINVAL;
        }
        wlan_get_mu_tx_blacklist_cnt(vap,value);
        break;
    case IEEE80211_PARAM_PEER_TX_COUNT:
        if (ic->ic_vap_set_param) {
	   retv = ic->ic_vap_set_param(vap, IEEE80211_PEER_TX_COUNT_SET, (u_int32_t)value);
        }
        break;
    case IEEE80211_PARAM_PEER_MUMIMO_TX_COUNT_RESET:
	if (ic->ic_vap_set_param) {
            retv = ic->ic_vap_set_param(vap, IEEE80211_PEER_MUMIMO_TX_COUNT_RESET_SET, (u_int32_t)value);
        }
        break;
    case IEEE80211_PARAM_RTT_11AZ_TB_MAX_SESSION_EXPIRY:
        if (ic->ic_vap_set_param) {
            retv = ic->ic_vap_set_param(vap, IEEE80211_RTT_11AZ_TB_MAX_SESSION_EXPIRY, (u_int32_t)value);
        }
        break;
    case IEEE80211_PARAM_RTT_11AZ_NTB_MAX_TIME_BW_MEAS:
        if (ic->ic_vap_set_param) {
            retv = ic->ic_vap_set_param(vap, IEEE80211_RTT_11AZ_NTB_MAX_TIME_BW_MEAS, (u_int32_t)value);
        }
        break;
    case IEEE80211_PARAM_RTT_11AZ_NTB_MIN_TIME_BW_MEAS:
        if (ic->ic_vap_set_param) {
            retv = ic->ic_vap_set_param(vap, IEEE80211_RTT_11AZ_NTB_MIN_TIME_BW_MEAS, (u_int32_t)value);
        }
        break;
    case IEEE80211_PARAM_PEER_POSITION:
        if (ic->ic_vap_set_param) {
	    retv = ic->ic_vap_set_param(vap, IEEE80211_PEER_POSITION_SET, (u_int32_t)value);
        }
	break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_TPE_COMMON_PSD:
        if (value < 0 || value > 1) {
            qdf_err("Invalid value. Value should be in the range of 0/1");
            return -EINVAL;
        }
        vap->iv_ucfg_tpe_common_psd = value;
        break;
    case IEEE80211_PARAM_TPE_PWR_UNIT:
        if (value < IEEE80211_TPE_REG_EIRP_PSD || value > IEEE80211_TPE_REG_EIRP) {
            qdf_err("Invalid value. Value should be 0 for PSD and 1 for EIRP");
            return -EINVAL;
        }
        vap->iv_ucfg_tpe_pwr_unit = value;
        break;
    case IEEE80211_PARAM_TPE_PUNC_PWR:
        vap->iv_ucfg_tpe_punc_pwr = value;
        qdf_info("Puncture PSD power: %d\n", value);
        break;
#endif
    case IEEE80211_PARAM_SET_TXPWRADJUST:
        wlan_set_param(vap, IEEE80211_SET_TXPWRADJUST, value);
        break;
    case IEEE80211_PARAM_MAXSTA:
        /*
         * Set the maximum number of STAs that can associate with the specified
         * VAP. The value needs to be greater than 1 to ensure that at least
         * 1 client can associate with the VAP. The value also needs to be
         * less than or equal to the total STA support for the radio.
         *
         * The behavior of this command is different for EMA/MBSSID and legacy
         * cases.
         * - For EMA/MBSSID, the command will not reinitialize the AID
         * bitmap since the bitmap is shared by all VAPs in the MBSS group and
         * the changing the bitmap size will indirectly affect the AID space
         * for other VAPs as well. Therefore, the bitmap will remain as it was
         * during create time and only the soft-limit will change while
         * iv_max_aid will remain the same.
         * - For legacy cases, the command will reinitialize both the TIM and
         * the AID bitmap if the client limits are changed. This is possible
         * because the bitmaps are independent even in multi-VAP scenarios.
         */
        if ((value > ic->ic_num_clients) || (value < 1)) {
            qdf_err("Value beyond acceptable range of 1 and %d", ic->ic_num_clients);
            return -EINVAL;
        }

        if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
            qdf_err("Command is supported for AP mode only");
            return -EINVAL;
        }

        if (value < vap->iv_sta_assoc) {
            qdf_err("Value is lower than current num of connected clients (%d)",
                    vap->iv_sta_assoc);
            return -EINVAL;
        }

        if (is_mbssid_enabled) {
#ifdef WLAN_FEATURE_11BE_MLO
            aid_mgr = wlan_vdev_mlme_get_aid_mgr(vap->vdev_obj);
            if (!vap->iv_max_aid || !aid_mgr) {
#else
            if (!vap->iv_max_aid || !vap->iv_aid_bitmap) {
#endif
                /* This is an unlikely scenario */
                qdf_err("AID bitmap is empty");
                return -EINVAL;
            }

            /*
             * Value should range from (2 + (1 << ic->ic_mbss.max_bssid)) to
             * (ic_num_clients + 1 + (start_aid))
             */
            start_aid = ieee80211_get_vap_start_aid(ic, vap);
            vap->iv_max_aid = vap->iv_mbss_max_aid = (value + start_aid + 1);
#ifdef WLAN_FEATURE_11BE_MLO
            wlan_vdev_mlme_aid_mgr_max_aid_set(vap->vdev_obj, vap->iv_max_aid);
            wlan_vdev_mlme_set_start_aid(vap->vdev_obj, start_aid - 1);
#endif
            qdf_debug("aid:%d, iv_max_aid:%d, vap:%s",start_aid, vap->iv_max_aid, vap->iv_netdev_name);
        } else {
            u_int16_t old_max_aid = vap->iv_max_aid;
            u_int16_t old_len = howmany(vap->iv_max_aid, 32) * sizeof(unsigned long);

            /*
             * Reject station when associated aid >= iv_max_aid, such that
             * max associated station should be value + 1
             */
            vap->iv_max_aid = value + 1;

            /* If interface is up, may need to reallocation bitmap(tim, aid) */
            if (dev && (IS_UP(dev))) {
                if (vap->iv_alloc_tim_bitmap) {
                    error = vap->iv_alloc_tim_bitmap(vap);
                }

                if(!error) {
                    if (wlan_node_alloc_aid_bitmap(vap, old_len)) {
                        qdf_err("Setting Max Stations fail");
                        vap->iv_max_aid = old_max_aid;
                        return -ENOMEM;
                    }
                }
            }
        }

        if(!error) {
            qdf_info("Setting Max Stations:%d", value);
        }
        break;
    case IEEE80211_PARAM_AUTO_ASSOC:
        wlan_set_param(vap, IEEE80211_AUTO_ASSOC, value);
        break;
    case IEEE80211_PARAM_VAP_COUNTRY_IE:
        wlan_set_param(vap, IEEE80211_FEATURE_COUNTRY_IE, value);
        break;
    case IEEE80211_PARAM_VAP_DOTH:
        wlan_set_param(vap, IEEE80211_FEATURE_DOTH, value);
        break;
    case IEEE80211_PARAM_HT40_INTOLERANT:
        wlan_set_param(vap, IEEE80211_HT40_INTOLERANT, value);
        break;
    case IEEE80211_PARAM_BSS_CHAN_INFO:
        if (value < BSS_CHAN_INFO_READ || value > BSS_CHAN_INFO_READ_AND_CLEAR)
        {
            qdf_print("Setting Param value to 1(read only)");
            value = BSS_CHAN_INFO_READ;
        }
        if (ic->ic_ath_bss_chan_info_stats)
            ic->ic_ath_bss_chan_info_stats(ic, value);
        else {
            qdf_print("Not supported for DA");
            return -EINVAL;
        }
        break;

    case IEEE80211_PARAM_CHWIDTH:
        retv = wlan_set_param(vap, IEEE80211_CHWIDTH, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_CHEXTOFFSET:
        wlan_set_param(vap, IEEE80211_CHEXTOFFSET, value);
        break;
#ifdef ATH_SUPPORT_QUICK_KICKOUT
    case IEEE80211_PARAM_STA_QUICKKICKOUT:
            wlan_set_param(vap, IEEE80211_STA_QUICKKICKOUT, value);
        break;
#endif
#if ATH_SUPPORT_DSCP_OVERRIDE
    case IEEE80211_PARAM_VAP_DSCP_PRIORITY:
        retv = wlan_set_vap_priority_dscp_tid_map(vap,value);
        if(retv == EOK)
            retv = wlan_set_param(vap, IEEE80211_VAP_DSCP_PRIORITY, value);
        break;
#endif
    case IEEE80211_PARAM_CHSCANINIT:
        wlan_set_param(vap, IEEE80211_CHSCANINIT, value);
        break;

    case IEEE80211_PARAM_COEXT_DISABLE:
        ic->ic_user_coext_disable = value;
        skip_restart = 0;
        if (value)
        {
            if (!(ic->ic_flags & IEEE80211_F_COEXT_DISABLE))
                 ieee80211com_set_flags(ic, IEEE80211_F_COEXT_DISABLE);
            else
                 skip_restart = 1;
        }
        else
        {
            if (ic->ic_flags & IEEE80211_F_COEXT_DISABLE)
                 ieee80211com_clear_flags(ic, IEEE80211_F_COEXT_DISABLE);
            else
                 skip_restart = 1;
        }
        if (!skip_restart) {
            ic->ic_need_vap_reinit = 1;
            if (ieee80211_vaps_active(ic)) {
                osif_restart_for_config(ic, NULL, NULL);
            }
        }

        break;

    case IEEE80211_PARAM_NR_SHARE_RADIO_FLAG:
#if UMAC_SUPPORT_RRM
        retv = ieee80211_set_nr_share_radio_flag(vap,value);
#endif
        break;

    case IEEE80211_PARAM_AUTHMODE:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_AUTHMODE to %s\n",
        (value == IEEE80211_AUTH_WPA) ? "WPA" : (value == IEEE80211_AUTH_8021X) ? "802.1x" :
        (value == IEEE80211_AUTH_OPEN) ? "open" : (value == IEEE80211_AUTH_SHARED) ? "shared" :
        (value == IEEE80211_AUTH_AUTO) ? "auto" : "unknown" );

        /* Note: The PARAM_AUTHMODE will not handle WPA , that will be taken
	 * care by PARAM_WPA, so we check and update authmode for AUTO as a
	 * combination of Open and Shared , and for all others we update the value
	 * except for WPA
	 */
        if (value == IEEE80211_AUTH_AUTO) {
            error = wlan_crypto_set_vdev_param(vap->vdev_obj,
                         WLAN_CRYPTO_PARAM_AUTH_MODE,
                         (uint32_t)((1 << WLAN_CRYPTO_AUTH_OPEN) | (1 << WLAN_CRYPTO_AUTH_SHARED)));
        } else if (value != IEEE80211_AUTH_WPA) {
            error = wlan_crypto_set_vdev_param(vap->vdev_obj,
                         WLAN_CRYPTO_PARAM_AUTH_MODE,
                         (uint32_t)(1 << value));
        }

        if (error == 0) {
            if (osifp->os_opmode != IEEE80211_M_STA || !vap->iv_roam.iv_ft_enable) {
                retv = ENETRESET;
            }
        } else {
            retv = error;
        }
        if (retv == ENETRESET)
            restart_vap = true;
        break;
    case IEEE80211_PARAM_MCASTKEYLEN:
        retv = 0;
        break;
    case IEEE80211_PARAM_UCASTCIPHERS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_UCASTCIPHERS (0x%x) %s %s %s %s %s %s %s\n",
                value, (value & 1<<IEEE80211_CIPHER_WEP) ? "WEP" : "",
                (value & 1<<IEEE80211_CIPHER_TKIP) ? "TKIP" : "",
                (value & 1<<IEEE80211_CIPHER_AES_OCB) ? "AES-OCB" : "",
                (value & 1<<IEEE80211_CIPHER_AES_CCM) ? "AES-CCMP 128" : "",
                (value & 1<<IEEE80211_CIPHER_AES_CCM_256) ? "AES-CCMP 256" : "",
                (value & 1<<IEEE80211_CIPHER_AES_GCM) ? "AES-GCMP 128" : "",
                (value & 1<<IEEE80211_CIPHER_AES_GCM_256) ? "AES-GCMP 256" : "",
                (value & 1<<IEEE80211_CIPHER_CKIP) ? "CKIP" : "",
                (value & 1<<IEEE80211_CIPHER_WAPI) ? "WAPI" : "",
                (value & 1<<IEEE80211_CIPHER_NONE) ? "NONE" : "");
        {
            int count=0;
            if (value & 1<<IEEE80211_CIPHER_WEP)
                osifp->uciphers[count++] = IEEE80211_CIPHER_WEP;
            if (value & 1<<IEEE80211_CIPHER_TKIP)
                osifp->uciphers[count++] = IEEE80211_CIPHER_TKIP;
            if (value & 1<<IEEE80211_CIPHER_AES_CCM)
                osifp->uciphers[count++] = IEEE80211_CIPHER_AES_CCM;
            if (value & 1<<IEEE80211_CIPHER_AES_CCM_256)
                osifp->uciphers[count++] = IEEE80211_CIPHER_AES_CCM_256;
            if (value & 1<<IEEE80211_CIPHER_AES_GCM)
                osifp->uciphers[count++] = IEEE80211_CIPHER_AES_GCM;
            if (value & 1<<IEEE80211_CIPHER_AES_GCM_256)
                osifp->uciphers[count++] = IEEE80211_CIPHER_AES_GCM_256;
            if (value & 1<<IEEE80211_CIPHER_CKIP)
                osifp->uciphers[count++] = IEEE80211_CIPHER_CKIP;
#if ATH_SUPPORT_WAPI
            if (value & 1<<IEEE80211_CIPHER_WAPI)
                osifp->uciphers[count++] = IEEE80211_CIPHER_WAPI;
#endif
            if (value & 1<<IEEE80211_CIPHER_NONE)
                osifp->uciphers[count++] = IEEE80211_CIPHER_NONE;
            error = wlan_set_ucast_ciphers(vap,osifp->uciphers,count);
            if (error == 0) {
                if (osifp->os_opmode != IEEE80211_M_STA || !vap->iv_roam.iv_ft_enable)
                    error = ENETRESET;
            }
            else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s Warning: wlan_set_ucast_cipher failed. cache the ucast cipher\n", __func__);
                error=0;
            }
            osifp->u_count=count;
        }
        retv = error;
        if (retv == ENETRESET)
            restart_vap = true;
        break;
    case IEEE80211_PARAM_UCASTCIPHER:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_UCASTCIPHER to %s\n",
                (value == IEEE80211_CIPHER_WEP) ? "WEP" :
                (value == IEEE80211_CIPHER_TKIP) ? "TKIP" :
                (value == IEEE80211_CIPHER_AES_OCB) ? "AES OCB" :
                (value == IEEE80211_CIPHER_AES_CCM) ? "AES CCM 128" :
                (value == IEEE80211_CIPHER_AES_CCM_256) ? "AES CCM 256" :
                (value == IEEE80211_CIPHER_AES_GCM) ? "AES GCM 128" :
                (value == IEEE80211_CIPHER_AES_GCM_256) ? "AES GCM 256" :
                (value == IEEE80211_CIPHER_CKIP) ? "CKIP" :
                (value == IEEE80211_CIPHER_WAPI) ? "WAPI" :
                (value == IEEE80211_CIPHER_NONE) ? "NONE" : "unknown");
        {
            ieee80211_cipher_type ctypes[1];
            ctypes[0] = (ieee80211_cipher_type) value;
            error = wlan_set_ucast_ciphers(vap,ctypes,1);
            /* save the ucast cipher info */
            osifp->uciphers[0] = ctypes[0];
            osifp->u_count=1;
            if (error == 0) {
                retv = ENETRESET;
                restart_vap = true;
            }
            else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s Warning: wlan_set_ucast_cipher failed. cache the ucast cipher\n", __func__);
                error=0;
            }
        }
        retv = error;
        break;
    case IEEE80211_PARAM_MCASTCIPHER:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_MCASTCIPHER to %s\n",
                        (value == IEEE80211_CIPHER_WEP) ? "WEP" :
                        (value == IEEE80211_CIPHER_TKIP) ? "TKIP" :
                        (value == IEEE80211_CIPHER_AES_OCB) ? "AES OCB" :
                        (value == IEEE80211_CIPHER_AES_CCM) ? "AES CCM 128" :
                        (value == IEEE80211_CIPHER_AES_CCM_256) ? "AES CCM 256" :
                        (value == IEEE80211_CIPHER_AES_GCM) ? "AES GCM 128" :
                        (value == IEEE80211_CIPHER_AES_GCM_256) ? "AES GCM 256" :
                        (value == IEEE80211_CIPHER_CKIP) ? "CKIP" :
                        (value == IEEE80211_CIPHER_WAPI) ? "WAPI" :
                        (value == IEEE80211_CIPHER_NONE) ? "NONE" : "unknown");
        {
            ieee80211_cipher_type ctypes[1];
            ctypes[0] = (ieee80211_cipher_type) value;
            error = wlan_set_mcast_ciphers(vap, ctypes, 1);
            /* save the mcast cipher info */
            osifp->mciphers[0] = ctypes[0];
            osifp->m_count=1;
            if (error) {
                /*
                * ignore the error for now.
                * both the ucast and mcast ciphers
                * are set again when auth mode is set.
                */
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"%s", "Warning: wlan_set_mcast_cipher failed. cache the mcast cipher  \n");
                error=0;
            }
        }
        retv = error;
        break;
    case IEEE80211_PARAM_UCASTKEYLEN:
        retv = 0;
        break;
    case IEEE80211_PARAM_PRIVACY:
        retv = wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY,value);
        break;
    case IEEE80211_PARAM_COUNTERMEASURES:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_COUNTER_MEASURES, value);
        break;
    case IEEE80211_PARAM_HIDESSID:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_HIDE_SSID, value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_APBRIDGE:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_APBRIDGE, value);
	ucfg_ipa_set_ap_ibss_fwd(pdev,
				 wlan_vdev_get_id(vap->vdev_obj),
				 !value);
        break;
    case IEEE80211_PARAM_KEYMGTALGS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_KEYMGTALGS (0x%x) %s %s\n",
        value, (value & WLAN_ASE_8021X_UNSPEC) ? "802.1x Unspecified" : "",
        (value & WLAN_ASE_8021X_PSK) ? "802.1x PSK" : "");
         wlan_crypto_set_vdev_param(vap->vdev_obj,
				  WLAN_CRYPTO_PARAM_KEY_MGMT,
					value);
        retv = error;
        break;
    case IEEE80211_PARAM_RSNCAPS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_RSNCAPS to 0x%x\n", value);
        if (value & WLAN_CRYPTO_RSN_CAP_MFP_ENABLED) {
            /*
             * 802.11w PMF is enabled so change hw MFP QOS bits
             */
            wlan_crypto_set_hwmfpQos(vap, 1);
        }
	error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_RSN_CAP, value);
	retv = error;
        break;
    case IEEE80211_PARAM_RSNXCAPS:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_RSNXCAPS to 0x%x\n", value);
        error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_RSNX_CAP, value);
        retv = error;
        break;
    case IEEE80211_PARAM_WPA:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set IEEE80211_IOC_WPA to %s\n",
        (value == 1) ? "WPA" : (value == 2) ? "RSN" :
        (value == 3) ? "WPA and RSN" : (value == 0)? "off" : "unknown");
        if (value > 3) {
            error = -EINVAL;
            break;
        } else {
	    uint32_t authmode;
            if (osifp->os_opmode == IEEE80211_M_STA ||
                osifp->os_opmode == IEEE80211_M_P2P_CLIENT) {
		error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_KEY_MGMT, (uint8_t)(WLAN_ASE_8021X_PSK));
                if (!error) {
                    if ((value == 3) || (value == 2)) { /* Mixed mode or WPA2 */
                        authmode = (1 << IEEE80211_AUTH_RSNA);
                    } else { /* WPA mode */
                        authmode = (1 << IEEE80211_AUTH_WPA);
                    }
                }
            } else {
                if (value == 3) {
			authmode =  (1<< IEEE80211_AUTH_WPA) | (1 << IEEE80211_AUTH_RSNA);
                } else if (value == 2) {
                    authmode =  (1<< IEEE80211_AUTH_RSNA);
                } else {
                    authmode =  (1<< IEEE80211_AUTH_WPA);
                }
            }
            error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE, authmode);
        }
        retv = error;
        break;

    case IEEE80211_PARAM_CLR_APPOPT_IE:
        retv = wlan_set_clr_appopt_ie(vap);
        break;

    /*
    ** The setting of the manual rate table parameters and the retries are moved
    ** to here, since they really don't belong in iwconfig
    */

    case IEEE80211_PARAM_11N_RATE:
        retv = wlan_set_param(vap, IEEE80211_FIXED_RATE, value);
        break;

    case IEEE80211_PARAM_VHT_MCS:
        retv = wlan_set_param(vap, IEEE80211_FIXED_VHT_MCS, value);
    break;

    case IEEE80211_PARAM_HE_MCS:
        /* if ldpc is disabled then as per 802.11ax
         * specification, D2.0 (section 28.1.1) we
         * can not allow mcs values 10 and 11
         */
        ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_LDPC, &ldpc);
        if (ieee80211vap_ishemode(vap) &&
            (ldpc == IEEE80211_HTCAP_C_LDPC_NONE) &&
            (value >= 10))
        {
            qdf_print("MCS 10 and 11 are not allowed in \
                    HE mode if LDPC is already diabled");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_FIXED_HE_MCS, value);
    break;

#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_EHT_MCS:
        /*
         * If LDPC is disabled then as per 802.11 spec, we cannot allow MCS
         * 10, 11, 12, 13 (to be updated based on latest 802.11be amendment)
         */
        ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_LDPC, &ldpc);
        if (ieee80211_is_phymode_11be(vap->iv_des_mode) &&
            (ldpc == IEEE80211_HTCAP_C_LDPC_NONE) &&
            (value >= (IEEE80211_HE_MCS_IDX_MAX - 1)) &&
            (value <= IEEE80211_HE_MCS_12_13_IDX_MAX))
        {
            qdf_err("MCS 10, 11, 12 and 13 are not allowed in \
                      EHT mode if LDPC is already diabled");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_FIXED_EHT_MCS, value);
    break;
#endif /* WLAN_FEATURE_11BE */

    case IEEE80211_PARAM_HE_MULTI_TID_AGGR:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_MULTI_TID_AGGR, value);

        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_MULTI_TID_AGGR_TX:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_MULTI_TID_AGGR_TX, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_MAX_AMPDU_LEN_EXP:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_MAX_AMPDU_LEN_EXP, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_SU_PPDU_1X_LTF_800NS_GI:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_SU_PPDU_1X_LTF_800NS_GI, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_SU_MU_PPDU_4X_LTF_800NS_GI:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_SU_MU_PPDU_4X_LTF_800NS_GI, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_MAX_FRAG_MSDU:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_MAX_FRAG_MSDU, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_MIN_FRAG_SIZE:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_MIN_FRAG_SIZE, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_OMI:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_OMI, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_NDP_4X_LTF_3200NS_GI:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
                        IEEE80211_CONFIG_HE_NDP_4X_LTF_3200NS_GI, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_ER_SU_PPDU_1X_LTF_800NS_GI:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
                        IEEE80211_CONFIG_HE_ER_SU_PPDU_1X_LTF_800NS_GI, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_ER_SU_PPDU_4X_LTF_800NS_GI:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
                        IEEE80211_CONFIG_HE_ER_SU_PPDU_4X_LTF_800NS_GI, value);
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

case IEEE80211_PARAM_NSS:
        if (value <= 0) {
            qdf_err("Invalid value for NSS");
            return -EINVAL;
        }

        /* if ldpc is disabled then as per 802.11ax
         * specification, D2.0 (section 28.1.1) we
         * can not allow nss value > 4
         */
        ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_LDPC, &ldpc);
        if ((ldpc == IEEE80211_HTCAP_C_LDPC_NONE) &&
            ((ieee80211vap_ishemode(vap)
#ifdef WLAN_FEATURE_11BE
              || (ieee80211vap_isehtmode(vap))
#endif /* WLAN_FEATURE_11BE */
              ) && (value > 4))) {
            qdf_print("NSS greater than 4 is not allowed in \
                    HE/EHT mode if LDPC is already diabled");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_FIXED_NSS, value);
        if (!retv) {
            wlan_vdev_mlme_set_nss(vdev, value);
        }
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next)
                tmpvap->iv_set_vht_mcsmap = false;
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_HE_UL_SHORTGI:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL Shortgi setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_SHORTGI, value);
        break;

    case IEEE80211_PARAM_HE_UL_LTF:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL LTF setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_LTF, value);
        break;

    case IEEE80211_PARAM_HE_UL_NSS:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL NSS setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_NSS, value);
        break;

    case IEEE80211_PARAM_HE_UL_PPDU_BW:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL PPDU BW setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_PPDU_BW, value);
        break;

    case IEEE80211_PARAM_HE_UL_LDPC:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL LDPC setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_LDPC, value);
        break;

    case IEEE80211_PARAM_HE_UL_STBC:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL STBC setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_STBC, value);

        break;

    case IEEE80211_PARAM_HE_UL_FIXED_RATE:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("UL MCS setting is not allowed in current mode.");
            return -EPERM;
        }

        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_FIXED_RATE, value);
        break;

    case IEEE80211_PARAM_HE_AMSDU_IN_AMPDU_SUPRT:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("HE AMSDU in AMPDU support is not allowed in current mode.");
            return -EPERM;
        }

        if (value >= 0 && value <= 1)
            vap->iv_he_amsdu_in_ampdu_suprt = value;
        else {
            qdf_err("Invalid value");
            return -EPERM;
        }
        break;

    case IEEE80211_PARAM_HE_SUBFEE_STS_SUPRT:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("HE BFEE STS support is not allowed in current mode.");
            return -EPERM;
        }

        if (!vap->iv_he_su_bfee) {
            qdf_err("HE SUBFEE_STS fields are reserved if HE SU BFEE"
                    " role is not supported");
            return -EPERM;
        }

        if ((val[1] >= 0 && val[1] <= 7) &&
            (val[2] >= 0 && val[2] <=7)) {
            vap->iv_he_subfee_sts_lteq80 = val[1];
            vap->iv_he_subfee_sts_gt80   = val[2];
        } else {
            qdf_err("Invalid value");
            return -EPERM;
        }

        qdf_info("HE SUBFEE_STS: lteq80: %d gt80: %d", val[1], val[2]);
        break;

    case IEEE80211_PARAM_HE_4XLTF_800NS_GI_RX_SUPRT:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("HE 4x LTF & 800ns GI support is not allowed in current mode.");
            return -EPERM;
        }

        if (value >= 0 && value <= 1)
            vap->iv_he_4xltf_800ns_gi = value;
        else {
            qdf_err("Invalid value");
            return -EPERM;
        }
        break;

    case IEEE80211_PARAM_HE_1XLTF_800NS_GI_RX_SUPRT:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("HE 1x LTF & 800ns GI support is not allowed in current mode.");
            return -EPERM;
        }

        if (value >= 0 && value <= 1)
            vap->iv_he_1xltf_800ns_gi = value;
        else {
            qdf_err("Invalid value");
            return -EPERM;
        }
        break;

    case IEEE80211_PARAM_HE_MAX_NC_SUPRT:

        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("HE Max Nc GI support is not allowed in current mode.");
            return -EPERM;
        }

        if (value && !vap->iv_he_su_bfee) {
            qdf_err("HE MAX_NC fields are reserved if HE SU BFEE"
                    " role is not supported");
            return -EPERM;
        }

        if (value >= 0 && value <= 7)
            vap->iv_he_max_nc = value;
        else {
            qdf_err("Invalid value");
            return -EPERM;
        }
        break;

    case IEEE80211_PARAM_TWT_RESPONDER_SUPRT:

        if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
            qdf_err("TWT support is not allowed for non ap vap.");
            return -EPERM;
        }

        if (vap->iv_special_vap_mode || vap->iv_smart_monitor_vap) {
            qdf_err("TWT support is not allowed for special vap or smart monitor vap.");
            return -EPERM;
        }

        if (vap->iv_twt_rsp == value){
            qdf_err("Input value is already configured");
            return 0;
        }

        if (value < 0 || value > 3) {
            qdf_err("Invalid value \nValue should be within 0 to 3, 0-Disable TWT, 1-Enable TWT, 2-Enable TWT and BTWT, 3-Enable TWT, BTWT and RTWT");
            return -EPERM;
        }
        if(!ieee80211vap_ishemode(vap)) {
            qdf_err("HE TWT support is not allowed in current mode.");
            return -EPERM;
        }

#if WLAN_TWT_CONV_SUPPORTED
        ucfg_twt_cfg_get_responder(psoc, &twt_responder);
        ucfg_twt_cfg_get_bcast(psoc, &broadcast_twt);
#else
        twt_responder = wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_SOC_CEXT_TWT_RESPONDER);
        broadcast_twt = wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_BCAST_TWT);
#endif

        switch (value) {

        case IEEE80211_TWT_DISABLE:
            vap->iv_twt_rsp = IEEE80211_TWT_DISABLE;
            break;

        case IEEE80211_TWT_ENABLE:
            if (!twt_responder) {
                qdf_err("TWT support disabled");
                return -EPERM;
            }
            vap->iv_twt_rsp = IEEE80211_TWT_ENABLE;
            break;

        case IEEE80211_TWT_BTWT_ENABLE:
            if (!twt_responder || !broadcast_twt) {
                qdf_err("twt_responder: %d\n broadcast_twt: %d",twt_responder,broadcast_twt);
                return -EPERM;
            }
            vap->iv_twt_rsp = IEEE80211_TWT_BTWT_ENABLE;
            break;

        case IEEE80211_TWT_BTWT_RTWT_ENABLE:

#ifdef WLAN_FEATURE_11BE
            if (!ieee80211vap_isehtmode(vap)) {
                 qdf_err("EHT TWT support is not allowed in current mode.");
                 return -EPERM;
            }

#if WLAN_TWT_CONV_SUPPORTED
             ucfg_twt_cfg_get_rtwt_requestor(psoc, &restricted_twt);
#else
             restricted_twt = wlan_psoc_nif_fw_ext2_cap_get(psoc, WLAN_SOC_F_RESTRICTED_TWT);
#endif

             if (!twt_responder || !broadcast_twt || !restricted_twt) {
                  qdf_err("twt_responder: %d\n broadcast_twt: %d\n restricted_twt: %d",twt_responder,broadcast_twt,restricted_twt);
                  return -EPERM;
             }
             vap->iv_twt_rsp = IEEE80211_TWT_BTWT_RTWT_ENABLE;
#endif
             break;

        default:
             qdf_err("Invalid value \nValue should be within 0 to 3, 0-Disable TWT, 1-Enable TWT, 2-Enable TWT and BTWT, 3-Enable TWT, BTWT and RTWT");
             return -EINVAL;
         }
        if (ic->ic_send_twt_param)
            retv = ic->ic_send_twt_param(vap,value);
        if (retv == 0)
        {
            retv = ENETRESET;
            restart_vap = true;
        }
         break;

    case IEEE80211_PARAM_NO_VAP_RESET:
        vap->iv_novap_reset = !!value;
    break;

    case IEEE80211_PARAM_OPMODE_NOTIFY:
        retv = wlan_set_param(vap, IEEE80211_OPMODE_NOTIFY_ENABLE, value);
    break;

    case IEEE80211_PARAM_VHT_SGIMASK:
        retv = wlan_set_param(vap, IEEE80211_VHT_SGIMASK, value);
        if (retv == 0)
            retv = ENETRESET;
    break;

    case IEEE80211_PARAM_VHT80_RATEMASK:
        retv = wlan_set_param(vap, IEEE80211_VHT80_RATEMASK, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_BW_NSS_RATEMASK:
        retv = wlan_set_param(vap, IEEE80211_BW_NSS_RATEMASK, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_LDPC:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_LDPC, value);
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_TX_STBC:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_TX_STBC, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_VAP_DIFF_MODE:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_VAP_DIFF_MODE, value);
    break;

    case IEEE80211_PARAM_RX_STBC:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_RX_STBC, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_VHT_TX_MCSMAP:
        retv = wlan_set_param(vap, IEEE80211_VHT_TX_MCSMAP, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_VHT_RX_MCSMAP:
        retv = wlan_set_param(vap, IEEE80211_VHT_RX_MCSMAP, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_11N_RETRIES:
        if (value)
            retv = wlan_set_param(vap, IEEE80211_FIXED_RETRIES, value);
        break;
    case IEEE80211_PARAM_SHORT_GI :
        retv = wlan_set_param(vap, IEEE80211_SHORT_GI, value);
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_SHORT_GI20:
        retv = wlan_set_param(vap, IEEE80211_SHORT_GI20, value);
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_SHORT_GI40:
        retv = wlan_set_param(vap, IEEE80211_SHORT_GI40, value);
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_BANDWIDTH :
        retv = ieee80211_ucfg_set_bandwidth(vap, value);
        break;
    case IEEE80211_PARAM_FREQ_BAND :
         retv = ieee80211_ucfg_set_band(vap, value);
         break;

    case IEEE80211_PARAM_EXTCHAN :
         retv = ieee80211_ucfg_set_extchan(vap, value);
         break;

    case IEEE80211_PARAM_SECOND_CENTER_FREQ :
         retv = wlan_set_param(vap, IEEE80211_SECOND_CENTER_FREQ, value);
         break;

    case IEEE80211_PARAM_ATH_SUPPORT_VLAN :
         if( value == 0 || value ==1) {
             vap->vlan_set_flags = value;
             if(value == 1) {
                 dev->features &= ~NETIF_F_HW_VLAN;
#ifdef WLAN_FEATURE_11BE_MLO
                 if (osifp->mldev && osifp->mldev->mld_dev)
                      osifp->mldev->mld_dev->features &= ~NETIF_F_HW_VLAN;
#endif
             }
             if(value == 0) {
                 dev->features |= NETIF_F_HW_VLAN;
#ifdef WLAN_FEATURE_11BE_MLO
                 if (osifp->mldev && osifp->mldev->mld_dev)
                      osifp->mldev->mld_dev->features |= NETIF_F_HW_VLAN;
#endif
             }
         }
         break;

    case IEEE80211_DISABLE_BCN_BW_NSS_MAP :
         if(value >= 0) {
             ic->ic_disable_bcn_bwnss_map = (value ? 1: 0);
             retv = EOK;
         }
         else
             retv = EINVAL;
         break;

    case IEEE80211_DISABLE_STA_BWNSS_ADV:
         if (value >= 0) {
             ic->ic_disable_bwnss_adv = (value ? 1: 0);
	     retv = EOK;
         } else
             retv = EINVAL;
	 break;
#if DBG_LVL_MAC_FILTERING
    case IEEE80211_PARAM_DBG_LVL_MAC:
          /* This takes 8 bytes as arguments <set/clear> <mac addr> <enable/disable>
          *  e.g. dbgLVLmac 1 0xaa 0xbb 0xcc 0xdd 0xee 0xff 1
          */
         retv = wlan_set_debug_mac_filtering_flags(vap, (unsigned char *)extra);
         break;
#endif

    case IEEE80211_PARAM_UMAC_VERBOSE_LVL:
        if (((value >= IEEE80211_VERBOSE_FORCE) && (value <= IEEE80211_VERBOSE_TRACE)) || (value == IEEE80211_VERBOSE_OFF)){
            wlan_set_umac_verbose_level(value);
        } else
            retv = EINVAL;
        break;

    case IEEE80211_PARAM_CONFIG_CATEGORY_VERBOSE:
         retv = wlan_set_shared_print_ctrl_category_verbose(value);
         break;

    case IEEE80211_PARAM_SET_VLAN_TYPE:
         ieee80211_ucfg_set_vlan_type(osifp, val[1], val[2]);
         break;

    case IEEE80211_PARAM_LOG_FLUSH_TIMER_PERIOD:
         retv = wlan_set_qdf_flush_timer_period(value);
         break;

    case IEEE80211_PARAM_LOG_FLUSH_ONE_TIME:
         wlan_set_qdf_flush_logs();
         break;

    case IEEE80211_PARAM_LOG_DUMP_AT_KERNEL_ENABLE:
         wlan_set_log_dump_at_kernel_level(value);
         break;

#ifdef QCA_OL_DMS_WAR
    case IEEE80211_PARAM_DMS_AMSDU_WAR:
         if(value < 0 || value > 1)
         {
             qdf_print("INVALID value. Please use 1 to enable and 0 to disable");
             break;
         }
         vap->dms_amsdu_war = value;
         /* Reset pkt_type set in CDP vdev->hdr_cache */
         if (!value) {
             wlan_set_param(vap, IEEE80211_VAP_TX_ENCAP_TYPE, htt_cmn_pkt_type_ethernet);
         } else {
             osifp->wifi3_0_fast_path = 0;
         }

         if ((ic->ic_get_tgt_type(ic) == TARGET_TYPE_QCN9224) ||
             (ic->ic_get_tgt_type(ic) == TARGET_TYPE_QCA5332) ||
             (ic->ic_get_tgt_type(ic) == TARGET_TYPE_QCN6432))
             vap->iv_hw_dms_support_enabled = value;
         break;
#endif

    case IEEE80211_PARAM_DBG_LVL:
         /*
          * NB: since the value is size of integer, we could only set the 32
          * LSBs of debug mask
          */
         if (vap->iv_csl_support && LOG_CSL_BASIC) {
             value |= IEEE80211_MSG_CSL;
         }
         {
             u_int64_t old_val = wlan_get_debug_flags(vap);
             retv = wlan_set_debug_flags(vap,(old_val & 0xffffffff00000000) | (u_int32_t)(value));
         }
         break;

    case IEEE80211_PARAM_DBG_LVL_HIGH:
        /*
         * NB: This sets the upper 32 LSBs
         */
        {
            u_int64_t old = wlan_get_debug_flags(vap);

            if (value == 1 << (IEEE80211_MSG_TPE_IE - 32))
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Enabling TPE IE prints may flood the console, use with caution\n");
            retv = wlan_set_debug_flags(vap, (old & 0xffffffff) | ((u_int64_t) value << 32));
        }
        break;
	case IEEE80211_PARAM_WEATHER_RADAR_CHANNEL:
        retv = wlan_set_param(vap, IEEE80211_WEATHER_RADAR, value);
        /* Making it zero so that it gets updated in Beacon */
        if ( EOK == retv)
            vap->iv_country_ie_chanflags = 0;
		break;
    case IEEE80211_PARAM_SEND_DEAUTH:
        retv = wlan_set_param(vap,IEEE80211_SEND_DEAUTH,value);
        break;
    case IEEE80211_PARAM_WEP_KEYCACHE:
        retv = wlan_set_param(vap, IEEE80211_WEP_KEYCACHE, value);
        break;
    case IEEE80211_PARAM_SIFS_TRIGGER:
        if (ic->ic_vap_sifs_trigger) {
            retv = ic->ic_vap_sifs_trigger(vap, value);
        }
        break;
    case IEEE80211_PARAM_BEACON_INTERVAL:
        if (!(vap->iv_create_flags & IEEE80211_LP_IOT_VAP)) {
            retv = ieee80211_ucfg_set_beacon_interval(vap, ic, value, 1);
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                "Not updating beacon interval on IoT AP vap\n");
                retv = 0;
        }
        break;
#if ATH_SUPPORT_AP_WDS_COMBO
    case IEEE80211_PARAM_NO_BEACON:
        retv = wlan_set_param(vap, IEEE80211_NO_BEACON, value);
        break;
#endif
    case IEEE80211_PARAM_PUREG:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_PUREG, value);
        /* NB: reset only if we're operating on an 11g channel */
        if (retv == 0) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan) &&
                (IEEE80211_IS_CHAN_ANYG(&chan) ||
                IEEE80211_IS_CHAN_11NG(&chan)))
                retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_PUREN:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_PURE11N, value);
        /* Reset only if we're operating on a 11ng channel */
        if (retv == 0) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan) &&
                IEEE80211_IS_CHAN_11NG(&chan))
                retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_PURE11AC:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_PURE11AC, value);
        /* Reset if the channel is valid */
        if (retv == EOK) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan))
                retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_PURE11AX:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_PURE11AX, value);
        /* Reset if the channel is valid */
        if (retv == EOK) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan))
                retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_STRICT_BW:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_STRICT_BW, value);
        /* Reset if the channel is valid */
        if (retv == EOK) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan))
                retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_WDS:
#ifdef HOST_OFFLOAD
        if(ic->ic_rx_intr_mitigation != NULL)
            ic->ic_rx_intr_mitigation(ic, value);
#else
        if (vap->iv_opmode == IEEE80211_M_STA)
        {
            if(ic->ic_rx_intr_mitigation != NULL)
                ic->ic_rx_intr_mitigation(ic, value);
        }
#endif
        ieee80211_update_vap_target(vap);
        retv = wlan_rptr_vdev_ucfg_config(vap->vdev_obj, IEEE80211_PARAM_WDS, value);
        break;
    case IEEE80211_PARAM_DA_WAR_ENABLE:
        if ((ic->ic_get_tgt_type(ic) == TARGET_TYPE_QCA8074) &&
           (wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP)) {
            buf.cdp_vdev_param_da_war = value;
            if (cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc),
                                    wlan_vdev_get_id(vap->vdev_obj), CDP_ENABLE_DA_WAR,
                                    buf) != QDF_STATUS_SUCCESS)
                retv = EINVAL;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "Valid only in HKv1 AP mode\n");
            retv = EINVAL;
        }
        break;
#if WDS_VENDOR_EXTENSION
    case IEEE80211_PARAM_WDS_RX_POLICY:
        retv = wlan_set_param(vap, IEEE80211_WDS_RX_POLICY, value);
        break;
#endif
    case IEEE80211_PARAM_VAP_PAUSE_SCAN:
        if (wlan_rptr_pdev_is_eir(pdev)) {
            vap->iv_pause_scan = value ;
            retv = 0;
        } else retv =  EINVAL;
        break;
#if ATH_GEN_RANDOMNESS
    case IEEE80211_PARAM_RANDOMGEN_MODE:
        if(value < 0 || value > 2)
        {
         qdf_print("INVALID mode please use between modes 0 to 2");
         break;
        }
        ic->random_gen_mode = value;
        break;
#endif
    case IEEE80211_PARAM_VAP_ENHIND:
        wlan_rptr_vdev_ucfg_config(vap->vdev_obj, IEEE80211_PARAM_VAP_ENHIND, value);
        break;

#if ATH_SUPPORT_WAPI
    case IEEE80211_PARAM_SETWAPI:
        retv = wlan_setup_wapi(vap, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_WAPIREKEY_USK:
        retv = wlan_set_wapirekey_unicast(vap, value);
        break;
    case IEEE80211_PARAM_WAPIREKEY_MSK:
        retv = wlan_set_wapirekey_multicast(vap, value);
        break;
    case IEEE80211_PARAM_WAPIREKEY_UPDATE:
        retv = wlan_set_wapirekey_update(vap, (unsigned char*)&extra[4]);
        break;
#endif
#if WLAN_SUPPORT_GREEN_AP
    case IEEE80211_IOCTL_GREEN_AP_PS_ENABLE:
#ifdef WLAN_FEATURE_11BE
        if (wlan_mlme_is_rm_energy_service_enabled())
        {
           qdf_warn("Energy service is running in RM, cannot enable Green AP");
           break;
        }
#endif
        if (value != 0 && value != WLAN_GREEN_AP_MODE_NO_STA && value != WLAN_GREEN_AP_MODE_NUM_STREAM)
        {
            qdf_warn("INVALID mode, please use mode as 0, 1 or 2\n");
            break;
        }
        if ((ic_is_sta_vap(ic) && !ieee80211_is_vap_down(ic->ic_sta_vap))
            && ((vap->iv_opmode == IEEE80211_M_HOSTAP) || !vap->iv_sm_gap_ps) && value!=0)
        {
            qdf_warn("Green AP can't be enabled if SM Powersave on STA vap is disabled\n");
            break;
        }
        ic->ic_ant_ps_on = value;
        if (ieee80211_vaps_up(ic, IEEE80211_M_HOSTAP))
            ucfg_green_ap_config(pdev, value);
        retv = 0;
        break;

    case IEEE80211_IOCTL_GREEN_AP_PS_TIMEOUT:
        ucfg_green_ap_set_transition_time(pdev, ((value > 20) && (value < 0xFFFF)) ? value : 20);
        retv = 0;
        break;

    case IEEE80211_IOCTL_GREEN_AP_ENABLE_PRINT:
        ucfg_green_ap_enable_debug_prints(pdev, value?1:0);
        break;
#endif
    case IEEE80211_PARAM_WPS:
        retv = wlan_set_param(vap, IEEE80211_WPS_MODE, value);
        break;
    case IEEE80211_PARAM_EXTAP:
        retv = wlan_rptr_vdev_ucfg_config(vap->vdev_obj, IEEE80211_PARAM_EXTAP, value);

       /* during ppe_vp update netdev should not be a slave
        * so before the update unslave the netdev
        * salve it back again once update is done.
        */
#ifdef CONFIG_BOND_MOD_SUPPORT
        if (osifp->mldev)
            osif_mld_link_delete(osifp);
#endif
#if QCA_PPE_VP
        if (value != 0)
            osif_pltfrm_update_ppe_vp(osifp);
#endif
#ifdef CONFIG_BOND_MOD_SUPPORT
        if (osifp->mldev)
            osif_mld_link_add(osifp->mldev->mld_dev, osifp, vap->link_id);
#endif
        break;
    case IEEE80211_PARAM_STA_FORWARD:
    retv = wlan_set_param(vap, IEEE80211_FEATURE_STAFWD, value);
    break;

    case IEEE80211_PARAM_DYN_BW_RTS:
        retv = ic->ic_vap_dyn_bw_rts(vap, value);
        if (retv == EOK) {
            vap->dyn_bw_rts = value;
        } else {
            return -EINVAL;
        }
        break;

    case IEEE80211_PARAM_CWM_EXTPROTMODE:
        if (value >= 0) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_EXTPROTMODE, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        } else {
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_CWM_EXTPROTSPACING:
        if (value >= 0) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_EXTPROTSPACING, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        }
        else {
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_CWM_ENABLE:
        if (value >= 0) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_ENABLE, value);
            if ((retv == EOK) && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
            }
        } else {
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_CWM_EXTBUSYTHRESHOLD:
        if (value >=0 && value <=100) {
            retv = wlan_set_device_param(ic,IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD, value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        } else {
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_DOTH:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_DOTH, value);
        if (retv == EOK) {
            retv = osif_pdev_restart_vaps(ic);
        }
        break;
    case IEEE80211_PARAM_SETADDBAOPER:
        if (value > 1 || value < 0) {
            return -EINVAL;
        }

        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_ADDBA_MODE, value);
        if((!retv) && (ic->ic_vap_set_param)) {
            retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_ADDBA_MODE,
                                        value);
        }
        break;
    case IEEE80211_PARAM_PROTMODE:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_PROTECTION_MODE, value);
        /* NB: if not operating in 11g this can wait */
        if (retv == EOK) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan) &&
                (IEEE80211_IS_CHAN_ANYG(&chan) ||
                IEEE80211_IS_CHAN_11NG(&chan))) {
                retv = ENETRESET;
                restart_vap = true;
            }
        }
        break;
    case IEEE80211_PARAM_ROAMING:
        if (!(IEEE80211_ROAMING_DEVICE <= value &&
            value <= IEEE80211_ROAMING_MANUAL))
            return -EINVAL;
        ic->ic_roaming = value;
        if(value == IEEE80211_ROAMING_MANUAL)
            IEEE80211_VAP_AUTOASSOC_DISABLE(vap);
        else
            IEEE80211_VAP_AUTOASSOC_ENABLE(vap);
        break;
    case IEEE80211_PARAM_DROPUNENCRYPTED:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_DROP_UNENC, value);
        break;
    case IEEE80211_PARAM_DRIVER_CAPS:
        retv = wlan_set_param(vap, IEEE80211_DRIVER_CAPS, value); /* NB: for testing */
        break;
    case IEEE80211_PARAM_STA_MAX_CH_CAP:
        /* This flag will be enabled only on a STA VAP */
        if (vap->iv_opmode == IEEE80211_M_STA) {
            vap->iv_sta_max_ch_cap = !!value;
        } else {
            qdf_err("Config is for STA VAP only");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_OBSS_NB_RU_TOLERANCE_TIME:
        if ((value >= IEEE80211_OBSS_NB_RU_TOLERANCE_TIME_MIN) &&
            (value <= IEEE80211_OBSS_NB_RU_TOLERANCE_TIME_MAX)) {
            ic->ic_obss_nb_ru_tolerance_time = value;
        } else {
            qdf_err("Invalid value for NB RU tolerance time\n");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_MAX_CAP_AP:
        /* This flag will be enabled only on a STA VAP */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            vap->iv_he_max_cap_ap = !!value;
        } else {
            qdf_err("Config is for AP VAP only");
            return -EINVAL;
        }
        break;
/*
* Support for Mcast Enhancement
*/
#if ATH_SUPPORT_IQUE
    case IEEE80211_PARAM_ME:
        wlan_set_param(vap, IEEE80211_ME, value);
        break;
    case IEEE80211_PARAM_IGMP_ME:
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            wlan_set_param(vap, IEEE80211_IGMP_ME, value);
	} else
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "Feature only valid in AP mode");
        break;
#endif
    case IEEE80211_PARAM_ME6_WAR:
        wlan_set_param(vap, IEEE80211_ME6_WAR, value);
        break;

    case IEEE80211_PARAM_4ADDR_EAPOL:
        wlan_set_param(vap, IEEE80211_4ADDR_EAPOL, value);
        break;

    case IEEE80211_PARAM_SCANVALID:
        if (QDF_IS_STATUS_ERROR(wlan_scan_set_aging_time(psoc, value))) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "Unable to set scanvalid time\n");
            retv = -EINVAL;
        }
        break;

    case IEEE80211_PARAM_DTIM_PERIOD:
        if (!(osifp->os_opmode == IEEE80211_M_HOSTAP))
            return -EINVAL;

        if (value > IEEE80211_DTIM_MAX ||
            value < IEEE80211_DTIM_MIN) {

            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "DTIM_PERIOD should be within %d to %d\n",
                              IEEE80211_DTIM_MIN,
                              IEEE80211_DTIM_MAX);
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_DTIM_INTVAL, value);
        if (retv == EOK) {
            if (is_mbssid_enabled &&
                !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
                retv = wlan_do_iter_fun_mbss_group(ic,
                                      vap->iv_mbss.group_id,
                                      MBSS_ITER_FUNC_PURPOSE_RESTART_VAP, NULL);
                if (retv != EOK) {
                    qdf_err("DTIM counter reset fails");
                    return -EINVAL;
                }
            } else {
                retv = ENETRESET;
                restart_vap = true;
            }
        }

        break;
    case IEEE80211_PARAM_MACCMD:
        wlan_set_acl_policy(vap, value, IEEE80211_ACL_FLAG_ACL_LIST_1);
        break;
    case IEEE80211_PARAM_ENABLE_OL_STATS:
        /* This param should be eventually removed and re-used */
        qdf_print("Issue this command on parent device, like wifiX");
        break;
    case IEEE80211_PARAM_RTT_ENABLE:
        if (!IEEE80211_IS_VALID_ENABLE_RTT_VAL(value)) {
            qdf_err("RTT value should be within 0 to 3,"
                    " 0-Disable RTT, 1-Enable RTT responder, 2-Enable RTT initiator,"
                    " 3-Enable either RTT responder or initiator mode");
            return -EINVAL;
        }

        /* Vdev level enable_rtt is allowed only for,
         * 1. To disable the current configured pdev level RTT mode on a vdev.
         * 2. To enable the current configured pdev level RTT mode on a vdev.
         * This check makes sure that uniform RTT mode is configured on all the
         * vdevs of a pdev.
         */
        if ((value != DISABLE_RTT_RESPONDER_AND_INITIATOR_MODE) &&
            (ic->rtt_enable != value)) {
            qdf_err("RTT mode %d is not same as pdev level enable_rtt %d mode",
                    value, ic->rtt_enable);
            return -EINVAL;
        }

        if (!ieee80211_is_configured_rtt_mode_allowed(psoc, ic, value)) {
            qdf_err("RTT mode %d is not supported in %s beacon mode",
                    value, scn->bcn_mode? "burst" : "staggered");
            return -EINVAL;
        }

        if (vap->rtt_enable == value) {
            qdf_info("rtt_enable = %d already configured, skip configuration",
                    value);
            return 0;
        }

        /* Clear the previous configuration */
        mlme_cfg.value = 0;
        status = ieee80211_configure_rtt_modes(vdev_mlme, vap->rtt_enable,
                                               mlme_cfg);
        if (QDF_IS_STATUS_ERROR(status))
            return -EINVAL;

        /* Clear previous configured switch_rtt_role value */
        vap->current_rtt_role = 0;

        /* Configure the new RTT modes */
        vap->rtt_enable = value;
        if (value != RTT_RESPONDER_OR_INITIATOR_MODE) {
            if (value == RTT_RESPONDER_MODE) {
                mlme_cfg.value = ieee80211_get_responder_role(psoc);
                vap->rtt_responder_role = mlme_cfg.value;
            } else {
                mlme_cfg.value = !!value;
                vap->rtt_responder_role = 0;
            }

            status = ieee80211_configure_rtt_modes(vdev_mlme, vap->rtt_enable,
                                                   mlme_cfg);
            if (QDF_IS_STATUS_ERROR(status))
                return -EINVAL;
        } else {
            qdf_info("enable_rtt:%d, issue 'switch_rtt_role' command to set either responder or initiator mode",
                    value);
            vap->rtt_responder_role = ieee80211_get_responder_role(psoc);
        }

        if (ic->ic_rtt_skip_vdev_restart) {
            /* Set iv_reinit_beacon flag as true for the vap.
             * This is required to update the FTM responder/initiator bits
             * in the extended capabilities IE when ieee80211_beacon_update
             * function is called as part of wlan_vdev_beacon_update().
             */
            vap->iv_reinit_beacon = true;
            wlan_vdev_beacon_update(vap);
        } else {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_SWITCH_RTT_ROLE:
        if (vap->rtt_enable != RTT_RESPONDER_OR_INITIATOR_MODE) {
            qdf_err("responder or initiator mode (value=3) is not set in enable_rtt command");
            return -EINVAL;
        }

        if (!IEEE80211_IS_VALID_SWITCH_RTT_ROLE_VAL(value)) {
            qdf_err("RTT value should be within 0 and 2 ,"
                    " 0-Disable RTT, 1-Enable RTT responder, 2-Enable RTT initiator");
            return -EINVAL;
        }

        /* Vdev level switch_rtt_role is allowed only for,
         * 1. To disable the current configured pdev level switch_rtt_role on a
         *    vdev.
         * 2. To enable the current configured pdev level switch_rtt_role on a
         *    vdev.
         * This check makes sure that uniform switch_rtt_role is configured on
         * all the vdevs of a pdev.
         */
        if ((value != DISABLE_RTT_RESPONDER_AND_INITIATOR_MODE) &&
            (ic->current_rtt_role != value)) {
            qdf_err("switch_rtt_role %d is not same as pdev level switch_rtt_role %d mode",
                    value, ic->current_rtt_role);
            return -EINVAL;
        }

        if (vap->current_rtt_role == value) {
            qdf_info("RTT role %d already configured, skip configuration", value);
            return 0;
        }

        /* Clear previous configuration */
        mlme_cfg.value = 0;
        status = ieee80211_configure_rtt_modes(vdev_mlme, vap->current_rtt_role, mlme_cfg);
        if (QDF_IS_STATUS_ERROR(status))
            return -EINVAL;

        /* Configure new RTT role */
        vap->current_rtt_role = value;

        if (value == RTT_RESPONDER_MODE) {
            mlme_cfg.value = ieee80211_get_responder_role(psoc);
            vap->rtt_responder_role = mlme_cfg.value;
        } else {
            mlme_cfg.value = !!value; /* Send either 0 or 1 to target */
            vap->rtt_responder_role = 0;
        }

        status = ieee80211_configure_rtt_modes(vdev_mlme, vap->current_rtt_role, mlme_cfg);
        if (QDF_IS_STATUS_ERROR(status))
            return -EINVAL;

        break;
    case IEEE80211_PARAM_SET_RTT_RESPONDER_ROLE:
        if (vap->rtt_enable != RTT_RESPONDER_MODE) {
            qdf_err("RTT responder mode is not enabled");
            return -EINVAL;
        }

        if (value < 0 ||
            value > (RTT_11MC_RESPONDER_ROLE | RTT_11AZ_NTB_RESPONDER_ROLE |
                     RTT_11AZ_TB_RESPONDER_ROLE)) {
            qdf_err("Value should be within 0 to 0x7, bit0:11mc, bit1:11az NTB, bit2:11az TB");
            return -EINVAL;
        }

        status = ieee80211_configure_rtt_responder_role(vap, value);
        if (QDF_IS_STATUS_ERROR(status))
            return -EINVAL;

        if (ic->ic_rtt_skip_vdev_restart) {
            /* Set iv_reinit_beacon flag as true for the vap.
             * This is required to update the FTM responder/initiator bits
             * in the extended capabilities IE when ieee80211_beacon_update
             * function is called as part of wlan_vdev_beacon_update().
             */
            vap->iv_reinit_beacon = true;
            wlan_vdev_beacon_update(vap);
        } else {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_LCI_ENABLE:
        qdf_print("KERN_DEBUG\n setting the lci enble flag");
        vap->lci_enable = !!value;
        retv = ENETRESET;
        break;
    case IEEE80211_PARAM_LCR_ENABLE:
        qdf_print("KERN_DEBUG\n setting the lcr enble flag");
        vap->lcr_enable = !!value;
        retv = ENETRESET;
        break;
    case IEEE80211_PARAM_MCAST_RATE:
        /*
        * value is rate in units of Kbps
        * min: 1Mbps max: 350Mbps
        */
        if (value < ONEMBPS || value > THREE_HUNDRED_FIFTY_MBPS)
            retv = -EINVAL;
        else {
            retv = wlan_set_param(vap, IEEE80211_MCAST_RATE, value);
        }
        break;
    case IEEE80211_PARAM_BCAST_RATE:
        /*
        * value is rate in units of Kbps
        * min: 1Mbps max: 350Mbps
        */
        if (value < ONEMBPS || value > THREE_HUNDRED_FIFTY_MBPS)
            retv = -EINVAL;
        else {
            retv = wlan_set_param(vap, IEEE80211_BCAST_RATE, value);
        }
        break;
    case IEEE80211_PARAM_MGMT_RATE:
        if(!ieee80211_rate_is_valid_basic(vap,value)){
            qdf_nofl_info("%s: rate %d is not valid. ",__func__,value);
            retv = -EINVAL;
            break;
        }
       /*
        * Do not allow the setting of HE mgmt rate.
        */
        if (ieee80211_is_he_rate(value)) {
            qdf_nofl_info("%s: HE mgmt rate %d is not allowed. ",__func__,value);
            retv = -EINVAL;
            break;
        }
       /*
        * value is rate in units of Kbps
        * min: 1000 kbps max: 300000 kbps
        */
        if (value < 1000 || value > 300000)
            retv = -EINVAL;
        else {
            retv = wlan_set_param(vap, IEEE80211_MGMT_RATE, value);
            /* Set beacon rate through separate vdev param */
            retv = wlan_set_param(vap, IEEE80211_BEACON_RATE_FOR_VAP, value);
        }
        break;
    case IEEE80211_PARAM_RTSCTS_RATE:
        retv = wlan_set_param(vap, IEEE80211_NON_BASIC_RTSCTS_RATE, value);
        break;
    case IEEE80211_PARAM_PRB_RATE:
        if(!ieee80211_rate_is_valid_basic(vap,value)){
            qdf_print("%s: rate %d is not valid. ", __func__, value);
            retv = -EINVAL;
            break;
        }
       /*
        * value is rate in units of Kbps
        * min: 1000 kbps max: 300000 kbps
        */
        if (value < 1000 || value > 300000)
            retv = -EINVAL;
        else {
            retv = wlan_set_param(vap, IEEE80211_PRB_RATE, value);
        }
        break;
    case IEEE80211_PARAM_PRB_RETRY:
       /*
        * value is retry limit count
        * min: 1 max: 15
        */
        if (value < 1 || value > 15)
            retv = -EINVAL;
        else {
            retv = wlan_set_param(vap, IEEE80211_PRB_RETRY, value);
        }
        break;
    case IEEE80211_RTSCTS_RATE:
        if (!ieee80211_rate_is_valid_basic(vap,value)) {
            qdf_nofl_info("%s: Rate %d is not valid. ",__func__,value);
            retv = -EINVAL;
            break;
        }
       /*
        * Here value represents rate in Kbps.
        * min: 1000 kbps max: 24000 kbps
        */
        if (value < ONEMBPS || value > HIGHEST_BASIC_RATE)
            retv = -EINVAL;
        else {
            retv = wlan_set_param(vap, IEEE80211_RTSCTS_RATE, value);
        }
        break;
    case IEEE80211_PARAM_CCMPSW_ENCDEC:
        if (value) {
            IEEE80211_VAP_CCMPSW_ENCDEC_ENABLE(vap);
        } else {
            IEEE80211_VAP_CCMPSW_ENCDEC_DISABLE(vap);
        }
        break;
    case IEEE80211_PARAM_NETWORK_SLEEP:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s set IEEE80211_IOC_POWERSAVE parameter %d \n",
                          __func__,value );
        do {
            ieee80211_pwrsave_mode ps_mode = IEEE80211_PWRSAVE_NONE;
            switch(value) {
            case 0:
                ps_mode = IEEE80211_PWRSAVE_NONE;
                break;
            case 1:
                ps_mode = IEEE80211_PWRSAVE_LOW;
                break;
            case 2:
                ps_mode = IEEE80211_PWRSAVE_NORMAL;
                break;
            case 3:
                ps_mode = IEEE80211_PWRSAVE_MAXIMUM;
                break;
            }
            error= wlan_set_powersave(vap,ps_mode);
        } while(0);
        break;

#if UMAC_SUPPORT_WNM
    case IEEE80211_PARAM_WNM_SLEEP:
        if (wlan_wnm_vap_is_set(vap) && ieee80211_wnm_sleep_is_set(vap->wnm)) {
            ieee80211_pwrsave_mode ps_mode = IEEE80211_PWRSAVE_NONE;
            if (value > 0)
                ps_mode = IEEE80211_PWRSAVE_WNM;
            else
                ps_mode = IEEE80211_PWRSAVE_NONE;

            if (wlan_vap_get_opmode(vap) == IEEE80211_M_STA)
                vap->iv_wnmsleep_intval = value > 0 ? value : 0;
            error = wlan_set_powersave(vap,ps_mode);
            qdf_print("set IEEE80211_PARAM_WNM_SLEEP mode = %d", ps_mode);
        } else
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: WNM not supported\n", __func__);
	break;

    case IEEE80211_PARAM_WNM_SMENTER:
        if (!wlan_wnm_vap_is_set(vap) || !ieee80211_wnm_sleep_is_set(vap->wnm)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: WNM not supported\n", __func__);
            return -EINVAL;
        }

        if (value % 2 == 0) {
            /* HACK: even interval means FORCE WNM Sleep: requires manual wnmsmexit */
            vap->iv_wnmsleep_force = 1;
        }

        ieee80211_wnm_sleepreq_to_app(vap, IEEE80211_WNMSLEEP_ACTION_ENTER, value);
        break;

    case IEEE80211_PARAM_WNM_SMEXIT:
        if (!wlan_wnm_vap_is_set(vap) || !ieee80211_wnm_sleep_is_set(vap->wnm)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: WNM not supported\n", __func__);
            return -EINVAL;
        }
        vap->iv_wnmsleep_force = 0;
        ieee80211_wnm_sleepreq_to_app(vap, IEEE80211_WNMSLEEP_ACTION_EXIT, value);
	    break;
#endif

#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    case IEEE80211_PARAM_PERIODIC_SCAN:
        if (wlan_vap_get_opmode(vap) == IEEE80211_M_STA) {
            if (osifp->os_periodic_scan_period != value){
                if (value && (value < OSIF_PERIODICSCAN_MIN_PERIOD))
                    osifp->os_periodic_scan_period = OSIF_PERIODICSCAN_MIN_PERIOD;
                else
                    osifp->os_periodic_scan_period = value;

                retv = ENETRESET;
            }
        }
        break;
#endif
    case IEEE80211_PARAM_VENDOR_FRAME_FWD_MASK:
        osifp->wlan_vendor_fwd_mgmt_mask = value;
        break;

#if ATH_SW_WOW
    case IEEE80211_PARAM_SW_WOW:
        if (wlan_vap_get_opmode(vap) == IEEE80211_M_STA) {
            retv = wlan_set_wow(vap, value);
        }
        break;
#endif

    case IEEE80211_PARAM_UAPSDINFO:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_UAPSD, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
	break ;
#if defined(UMAC_SUPPORT_STA_POWERSAVE) || defined(ATH_PERF_PWR_OFFLOAD)
    /* WFD Sigma use these two to do reset and some cases. */
    case IEEE80211_PARAM_SLEEP:
        /* XXX: Forced sleep for testing. Does not actually place the
         *      HW in sleep mode yet. this only makes sense for STAs.
         */
        /* enable/disable force  sleep */
        wlan_pwrsave_force_sleep(vap,value);
        break;
#endif
     case IEEE80211_PARAM_COUNTRY_IE:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_IC_COUNTRY_IE, value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_2G_CSA:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_2G_CSA, value);
        break;
#if UMAC_SUPPORT_QBSSLOAD
    case IEEE80211_PARAM_QBSS_LOAD:
        if (value > 1 || value < 0) {
            return -EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_QBSS_LOAD, value);
            if (retv == EOK) {
                retv = ENETRESET;
                restart_vap = true;
            }
        }
        break;
#if ATH_SUPPORT_HS20
    case IEEE80211_PARAM_HC_BSSLOAD:
        retv = wlan_set_param(vap, IEEE80211_HC_BSSLOAD, value);
        break;
    case IEEE80211_PARAM_OSEN:
        if (value > 1 || value < 0)
            return -EINVAL;
        else
            wlan_set_param(vap, IEEE80211_OSEN, value);
        break;
#endif /* ATH_SUPPORT_HS20 */
#endif /* UMAC_SUPPORT_QBSSLOAD */
#if UMAC_SUPPORT_XBSSLOAD
    case IEEE80211_PARAM_XBSS_LOAD:
        if (value > 1 || value < 0) {
            return -EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_XBSS_LOAD, value);
            if (retv == EOK) {
                retv = ENETRESET;
                restart_vap = true;
            }
        }
        break;
#endif
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    case IEEE80211_PARAM_CHAN_UTIL_ENAB:
        if (value > 1 || value < 0) {
            return -EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_CHAN_UTIL_ENAB, value);
            if (retv == EOK) {
                retv = ENETRESET;
                restart_vap = true;
            }
        }
        break;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */
#if UMAC_SUPPORT_QUIET
    case IEEE80211_PARAM_QUIET_PERIOD:
        if (vap->iv_bcn_offload_enable) {
            if (value > MAX_QUIET_ENABLE_FLAG || value < 0) {
                return -EINVAL;
            } else {
                retv = wlan_quiet_set_param(vap, value);
            }
        } else {
            if (value > 1 || value < 0) {
                return -EINVAL;
            } else {
                retv = wlan_quiet_set_param(vap, value);
                if (retv == EOK) {
                    retv = ENETRESET;
                    restart_vap = true;
                }
            }
        }
        break;
#endif /* UMAC_SUPPORT_QUIET */
    case IEEE80211_PARAM_START_ACS_REPORT:
        retv = wlan_set_param(vap, IEEE80211_START_ACS_REPORT, !!value);
        break;
    case IEEE80211_PARAM_MIN_DWELL_ACS_REPORT:
        retv = wlan_set_param(vap, IEEE80211_MIN_DWELL_ACS_REPORT, value);
        break;
    case IEEE80211_PARAM_MAX_DWELL_ACS_REPORT:
        retv = wlan_set_param(vap, IEEE80211_MAX_DWELL_ACS_REPORT, value);
        break;
    case IEEE80211_PARAM_SCAN_MIN_DWELL:
        retv = wlan_set_param(vap, IEEE80211_SCAN_MIN_DWELL, value);
       break;
    case IEEE80211_PARAM_SCAN_MAX_DWELL:
        retv = wlan_set_param(vap, IEEE80211_SCAN_MAX_DWELL, value);
       break;
    case IEEE80211_PARAM_MAX_SCAN_TIME_ACS_REPORT:
        retv = wlan_set_param(vap, IEEE80211_MAX_SCAN_TIME_ACS_REPORT, value);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_LONG_DUR:
        retv = wlan_set_param(vap,IEEE80211_ACS_CH_HOP_LONG_DUR, value);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_NO_HOP_DUR:
        retv = wlan_set_param(vap,IEEE80211_ACS_CH_HOP_NO_HOP_DUR,value);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_CNT_WIN_DUR:
        retv = wlan_set_param(vap,IEEE80211_ACS_CH_HOP_CNT_WIN_DUR, value);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_NOISE_TH:
        retv = wlan_set_param(vap,IEEE80211_ACS_CH_HOP_NOISE_TH,value);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_CNT_TH:
        retv = wlan_set_param(vap,IEEE80211_ACS_CH_HOP_CNT_TH, value);
        break;
    case IEEE80211_PARAM_ACS_ENABLE_CH_HOP:
        retv = wlan_set_param(vap,IEEE80211_ACS_ENABLE_CH_HOP, value);
        break;
    case IEEE80211_PARAM_MBO:
        retv = wlan_set_param(vap, IEEE80211_MBO, !!value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_MBO_ASSOC_DISALLOW:
        retv = wlan_set_param(vap, IEEE80211_MBO_ASSOC_DISALLOW,value);
        break;
    case IEEE80211_PARAM_MBO_CELLULAR_PREFERENCE:
        retv = wlan_set_param(vap,IEEE80211_MBO_CELLULAR_PREFERENCE,value);
        break;
    case IEEE80211_PARAM_MBO_TRANSITION_REASON:
        retv  = wlan_set_param(vap,IEEE80211_MBO_TRANSITION_REASON,value);
        break;
    case IEEE80211_PARAM_MBO_ASSOC_RETRY_DELAY:
        retv  = wlan_set_param(vap,IEEE80211_MBO_ASSOC_RETRY_DELAY,value);
        break;
    case IEEE80211_PARAM_MBO_CAP:
        retv = wlan_set_param(vap, IEEE80211_MBOCAP, value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_OCE:
        retv = wlan_set_param(vap, IEEE80211_OCE, !!value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_OCE_ASSOC_REJECT:
        retv = wlan_set_param(vap, IEEE80211_OCE_ASSOC_REJECT, !!value);
        break;
    case IEEE80211_PARAM_OCE_ASSOC_MIN_RSSI:
        retv = wlan_set_param(vap, IEEE80211_OCE_ASSOC_MIN_RSSI, value);
        break;
    case IEEE80211_PARAM_OCE_ASSOC_RETRY_DELAY:
        retv = wlan_set_param(vap, IEEE80211_OCE_ASSOC_RETRY_DELAY, value);
        break;
    case IEEE80211_PARAM_OCE_WAN_METRICS:
        retv = wlan_set_param(vap, IEEE80211_OCE_WAN_METRICS, ((val[1] & 0xFFFF) << 16) | (val[2] & 0xFFFF));
        break;
    case IEEE80211_PARAM_OCE_HLP:
         retv = wlan_set_param(vap, IEEE80211_OCE_HLP, !!value);
         break;
    case IEEE80211_PARAM_ASSOC_MIN_RSSI:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ASSOC_MIN_RSSI, value);
        break;
    case IEEE80211_PARAM_NBR_SCAN_PERIOD:
        vap->nbr_scan_period = value;
        break;
    case IEEE80211_PARAM_RNR:
        vap->rnr_enable = !!value;
        if (vap->iv_bcn_offload_enable && !vap->rnr_enable)
            ieee80211vap_set_flag_ext2(vap, IEEE80211_FEXT2_MBO);
        break;
    case IEEE80211_PARAM_RNR_FD:
        vap->rnr_enable_fd = !!value;
        break;
    case IEEE80211_PARAM_RNR_TBTT:
        vap->rnr_enable_tbtt = !!value;
        break;
    case IEEE80211_PARAM_NEIG_RPT:
        wlan_set_param(vap, IEEE80211_NEIGH_RPT, value);
        break;
    case IEEE80211_PARAM_AP_CHAN_RPT:
        vap->ap_chan_rpt_enable = value;
        if (vap->iv_bcn_offload_enable && !vap->ap_chan_rpt_enable)
            ieee80211vap_set_flag_ext2(vap, IEEE80211_FEXT2_MBO);
        break;
    case IEEE80211_PARAM_AP_CHAN_RPT_FILTER:
        if (value & APCHAN_RPT_SSID_FILTER) {
            vap->ap_chan_rpt_ssid_filter = TRUE;
        } else {
            vap->ap_chan_rpt_ssid_filter = FALSE;
        }
        if (value & APCHAN_RPT_OPCLASS_FILTER) {
            vap->ap_chan_rpt_opclass_filter = TRUE;
        } else {
            vap->ap_chan_rpt_opclass_filter = FALSE;
        }
        wlan_vdev_beacon_update(vap);
        break;
    case IEEE80211_PARAM_RRM_CAP:
        retv = wlan_set_param(vap, IEEE80211_RRM_CAP, !!value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_RRM_FILTER:
        retv = wlan_set_param(vap, IEEE80211_RRM_FILTER, !!value);
        break;
    case IEEE80211_PARAM_RRM_DEBUG:
        retv = wlan_set_param(vap, IEEE80211_RRM_DEBUG, value);
        break;
    case IEEE80211_PARAM_RRM_STATS:
        retv = wlan_set_param(vap, IEEE80211_RRM_STATS, !!value);
	break;
    case IEEE80211_PARAM_WNM_STATS:
        retv = wlan_set_param(vap, IEEE80211_WNM_STATS, !!value);
	break;
    case IEEE80211_PARAM_RRM_SLWINDOW:
        retv = wlan_set_param(vap, IEEE80211_RRM_SLWINDOW, !!value);
        break;
    case IEEE80211_PARAM_RRM_CAP_IE:
        retv = wlan_set_param(vap, IEEE80211_RRM_CAP_IE, !!value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
#if UMAC_SUPPORT_WNM
    case IEEE80211_PARAM_WNM_CAP:
        if (value > 1 || value < 0) {
            qdf_print(" ERR :- Invalid value %d Value to be either 0 or 1 ", value);
            return -EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_WNM_CAP, value);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
         if (ic->nss_vops)
            ic->nss_vops->ic_osif_nss_vdev_set_cfg(osifp, OSIF_NSS_WIFI_VDEV_CFG_WNM_CAP);
#endif
            if (retv == EOK) {
                retv = ENETRESET;
                restart_vap = true;
            }
        }
        break;
     case IEEE80211_PARAM_WNM_FILTER:
         retv = wlan_set_param(vap, IEEE80211_WNM_FILTER, !!value);
         break;
     case IEEE80211_PARAM_WNM_BSS_CAP: /* WNM Max BSS idle */
         if (value > 1 || value < 0) {
             qdf_print(" ERR :- Invalid value %d Value to be either 0 or 1 ", value);
             return -EINVAL;
         } else {
             retv = wlan_set_param(vap, IEEE80211_WNM_BSS_CAP, value);
             if (retv == EOK) {
                 retv = ENETRESET;
                restart_vap = true;
             }
         }
         break;
     case IEEE80211_PARAM_WNM_TFS_CAP:
         if (value > 1 || value < 0) {
             qdf_print(" ERR :- Invalid value %d Value to be either 0 or 1 ", value);
             return -EINVAL;
         } else {
             retv = wlan_set_param(vap, IEEE80211_WNM_TFS_CAP, value);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
         if (ic->nss_vops)
             ic->nss_vops->ic_osif_nss_vdev_set_cfg(osifp, OSIF_NSS_WIFI_VDEV_CFG_WNM_TFS);
#endif
             if (retv == EOK) {
                 retv = ENETRESET;
                restart_vap = true;
             }
         }
         break;
     case IEEE80211_PARAM_WNM_TIM_CAP:
         if (value > 1 || value < 0) {
             qdf_print(" ERR :- Invalid value %d Value to be either 0 or 1 ", value);
             return -EINVAL;
         } else {
             retv = wlan_set_param(vap, IEEE80211_WNM_TIM_CAP, value);
             if (retv == EOK) {
                 retv = ENETRESET;
                restart_vap = true;
             }
         }
         break;
     case IEEE80211_PARAM_WNM_SLEEP_CAP:
         if (value > 1 || value < 0) {
             qdf_print(" ERR :- Invalid value %d Value to be either 0 or 1 ", value);
             return -EINVAL;
         } else {
             retv = wlan_set_param(vap, IEEE80211_WNM_SLEEP_CAP, value);
             if (retv == EOK) {
                 retv = ENETRESET;
                 restart_vap = true;
             }
         }
         break;
    case IEEE80211_PARAM_WNM_FMS_CAP:
        if (value > 1 || value < 0) {
            return -EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_WNM_FMS_CAP, value);
            if (retv == EOK) {
                retv = ENETRESET;
                restart_vap = true;
             }
        }
        break;
#endif
    case IEEE80211_PARAM_FWD_ACTION_FRAMES_TO_APP:
        if (value > 1 || value < 0) {
            qdf_err("Invalid value %d, should be either 0 or 1", value);
            return -EINVAL;
        } else {
            retv = wlan_set_param(vap, IEEE80211_FWD_ACTION_FRAMES_TO_APP, value);
        }
        break;
    case IEEE80211_PARAM_PWRTARGET:
        retv = wlan_set_device_param(ic, IEEE80211_DEVICE_PWRTARGET, value);
        break;
    case IEEE80211_PARAM_WMM:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_WMM, value);
        if (retv != EOK)
            break;

        /* AMPDU should reflect changes to WMM */
        if (value && ic->ic_set_config_enable(vap)) {
            if (vap->iv_des_mode <= IEEE80211_MODE_11AXA_HE80_80)
                value = CUSTOM_AGGR_MAX_AMPDU_SIZE;
            else
                value = CUSTOM_AGGR_MAX_AMPDU_SIZE_BE;
        }
        else if (value)
            value = IEEE80211_AMPDU_SUBFRAME_MAX;

        /* Notice fallthrough */
    case IEEE80211_PARAM_AMPDU:
        if (osifp->osif_is_mode_offload) {
            uint32_t ampdu;

            ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_AMPDU, &ampdu);
            /* configure the max ampdu subframes */
                prev_state = ampdu ? 1:0;
            retv = wlan_set_param(vap, IEEE80211_AMPDU_SET, value);
            if (retv == -EINVAL)
                return -EINVAL;
            else {
                ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_AMPDU, &ampdu);
                new_state = ampdu ? 1:0;
            }
        }

        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }

        break;
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
    case IEEE80211_PARAM_REJOINT_ATTEMP_TIME:
        retv = wlan_set_param(vap,IEEE80211_REJOINT_ATTEMP_TIME,value);
        break;
#endif


    case IEEE80211_PARAM_RX_AMSDU:
        /*
         * API supports only maximum data tid values i.e. 0-7.
         */
        if (val[1] > 7 || !(val[2] == 0 || val[2] == 1)) {
            qdf_err("Usage: iwpriv athx tid <0/1>");
            return -EINVAL;
        }
        ieee80211com_set_rx_amsdu(ic, val[1], val[2]);
        break;

    case IEEE80211_PARAM_AMSDU:
#if defined(TEMP_AGGR_CFG)
        if (!value) {
            wlan_vdev_mlme_feat_cap_clear(vap->vdev_obj, WLAN_VDEV_FEXT_AMSDU);
        } else {
            wlan_vdev_mlme_feat_cap_set(vap->vdev_obj, WLAN_VDEV_FEXT_AMSDU);
        }

        if (osifp->osif_is_mode_offload) {
            /* configure the max amsdu subframes */
            retv = wlan_set_param(vap, IEEE80211_AMSDU_SET, value);
        }
#endif
        break;

        case IEEE80211_PARAM_RATE_DROPDOWN:
            if (osifp->osif_is_mode_offload) {
                if ((value >= 0) && (value <= RATE_DROPDOWN_LIMIT)) {
                        vap->iv_ratedrop = value;
                        retv = ic->ic_vap_set_param(vap, IEEE80211_RATE_DROPDOWN_SET, value);
                } else {
                        qdf_print("Rate Control Logic is [0-7]");
                        retv = -EINVAL;
                }
            } else {
                qdf_print("This Feature is Supported for Offload Mode Only");
                return -EINVAL;
            }
        break;

    case IEEE80211_PARAM_11N_TX_AMSDU:
        /* Enable/Disable Tx AMSDU for HT clients. Sanitise to 0 or 1 only */
        vap->iv_disable_ht_tx_amsdu = !!value;
        break;

    case IEEE80211_PARAM_CTSPROT_DTIM_BCN:
        retv = wlan_set_vap_cts2self_prot_dtim_bcn(vap, !!value);
        if (!retv)
            retv = EOK;
        else
            retv = -EINVAL;
       break;

    case IEEE80211_PARAM_VSP_ENABLE:
        /* Enable/Disable VSP for VOW */
        vap->iv_enable_vsp = !!value;
        break;

    case IEEE80211_PARAM_SHORTPREAMBLE:
        retv = wlan_set_param(vap, IEEE80211_SHORT_PREAMBLE, value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
       break;

    case IEEE80211_PARAM_SHORT_SLOT:
        retv = wlan_set_param(vap, IEEE80211_SHORT_SLOT, value);
        break;

    case IEEE80211_PARAM_CHANBW:
    {
        struct chan_params chan_p = {0};
        uint64_t flags = 0;
        struct ieee80211_ath_channel channel;

        chan_p.freq = wlan_get_param(vap, IEEE80211_DESIRED_CHANNEL);
        chan_p.cfreq2 = vap->iv_des_cfreq2_80_80;

        if (chan_p.freq) {
            flags = ieee80211_get_band_flag(chan_p.freq);
            flags |= IEEE80211_CHAN_OFDM;
            switch (value) {
                case 0:
                    flags |= IEEE80211_CHAN_20MHZ;
                    break;
                case 1:
                    flags |= IEEE80211_CHAN_HALF;
                    break;
                case 2:
                    flags |= IEEE80211_CHAN_QUARTER;
                    break;
                default:
                    retv = -EINVAL;
                    break;
            }

            if (!retv) {
                channel = ieee80211_find_channel(ic, &chan_p, flags);
                if (!IEEE80211_IS_CHAN_VALID(&channel)) {
                    qdf_err("CHANBW not supported on current channel");
                    retv = -EINVAL;
                }
            }
        }

        /*
         * bandwidth change need reselection of channel based on the chanbwflag
         * This is required if the command is issued after the freq has been set
         * neither the chanbw param does not take effect
         */
        if (!retv) {
            switch (value) {
                case 0:
                    ic->ic_chanbwflag = 0;
                    break;
                case 1:
                    ic->ic_chanbwflag = IEEE80211_CHAN_HALF;
                    break;
                case 2:
                    ic->ic_chanbwflag = IEEE80211_CHAN_QUARTER;
                    break;
                default:
                    retv = -EINVAL;
                    break;
            }
            if (!retv && chan_p.freq) {
                retv  = wlan_set_channel(vap, &chan_p);
                if (!retv)
                    retv = ENETRESET;
            }
        }

        wlan_scan_update_channel_list(ic);
#if UMAC_SUPPORT_CFG80211
        wlan_cfg80211_update_channel_list(ic);
#endif
    }
        break;

    case IEEE80211_PARAM_INACT:
        if (value > 4200) {
            qdf_err("Max allowed value is 4200");
            return -EINVAL;
        }

        if(ieee80211_vap_wnm_is_set(vap) && (vap->wnm) &&
                (ieee80211_wnm_bss_is_set(vap->wnm)) &&
                (ic->ic_get_tgt_type(ic) >= TARGET_TYPE_QCA8074)) {
            /* If Max BSS Idle Time feature is enabled, the max inactive timer
             * value should be updated using 'setbssmax' command to update
             * the new value in Max BSS Idle Time IE
             */
            qdf_info("Max BSS Idle Time feature is enabled!\n"
                     "'inact' command is disallowed\n"
                     "Use 'setbssmax' command instead to increase inactivity timer.");
        } else {
            wlan_set_param(vap, IEEE80211_RUN_INACT_TIMEOUT, value);
        }
        break;
    case IEEE80211_PARAM_INACT_AUTH:
        wlan_set_param(vap, IEEE80211_AUTH_INACT_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_INACT_INIT:
        wlan_set_param(vap, IEEE80211_INIT_INACT_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_SESSION_TIMEOUT:
        wlan_set_param(vap, IEEE80211_SESSION_TIMEOUT, value);
        break;
    case IEEE80211_PARAM_WDS_AUTODETECT:
        wlan_set_param(vap, IEEE80211_WDS_AUTODETECT, value);
        break;
    case IEEE80211_PARAM_WEP_TKIP_HT:
		wlan_set_param(vap, IEEE80211_WEP_TKIP_HT, value);
        retv = ENETRESET;
        break;
    case IEEE80211_PARAM_IGNORE_11DBEACON:
        wlan_set_param(vap, IEEE80211_IGNORE_11DBEACON, value);
        break;
    case IEEE80211_PARAM_MFP_TEST:
        wlan_set_param(vap, IEEE80211_FEATURE_MFP_TEST, value);
        break;

#ifdef QCA_PARTNER_PLATFORM
    case IEEE80211_PARAM_PLTFRM_PRIVATE:
        retv = wlan_pltfrm_set_param(vap, value);
        if ( retv == EOK) {
                retv = ENETRESET;
        }
        break;
#endif

    case IEEE80211_PARAM_NO_STOP_DISASSOC:
        if (value)
                osifp->no_stop_disassoc = 1;
        else
                osifp->no_stop_disassoc = 0;
        break;
#if UMAC_SUPPORT_VI_DBG

    case IEEE80211_PARAM_DBG_CFG:
        osifp->vi_dbg = value;
        ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_DBG_CFG, value);
        break;

    case IEEE80211_PARAM_RESTART:
        ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RESTART, value);
        break;
    case IEEE80211_PARAM_RXDROP_STATUS:
        ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RXDROP_STATUS, value);
        break;
#endif
    case IEEE80211_IOC_WPS_MODE:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                        "set IEEE80211_IOC_WPS_MODE to 0x%x\n", value);
        retv = wlan_set_param(vap, IEEE80211_WPS_MODE, value);
        break;

    case IEEE80211_IOC_SCAN_FLUSH:
#if ATH_SUPPORT_WRAP
        /*Avoid flushing scan results in proxysta case as proxysta needs to
          use the scan results of main-proxysta */
        if (wlan_rptr_vdev_is_no_event(vap->vdev_obj)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set %s\n",
                "Bypass IEEE80211_IOC_SCAN_FLUSH for non main-proxysta");
            break;
        }
#endif
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "set %s\n",
                        "IEEE80211_IOC_SCAN_FLUSH");
        status = wlan_objmgr_pdev_try_get_ref(pdev, WLAN_OSIF_SCAN_ID);
        if (QDF_IS_STATUS_ERROR(status)) {
            scan_info("unable to get reference");
            retv = -EBUSY;
            break;
        }
        ucfg_scan_flush_results(pdev, NULL);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_OSIF_SCAN_ID);
        retv = 0; /* success */
        break;

#ifdef ATH_SUPPORT_TxBF
    case IEEE80211_PARAM_TXBF_AUTO_CVUPDATE:
        wlan_set_param(vap, IEEE80211_TXBF_AUTO_CVUPDATE, value);
        ic->ic_set_config(vap);
        break;
    case IEEE80211_PARAM_TXBF_CVUPDATE_PER:
        wlan_set_param(vap, IEEE80211_TXBF_CVUPDATE_PER, value);
        ic->ic_set_config(vap);
        break;
#endif
    case IEEE80211_PARAM_SCAN_BAND:
        if ((value == OSIF_SCAN_BAND_2G_ONLY  && IEEE80211_SUPPORT_PHY_MODE(ic,IEEE80211_MODE_11G)) ||
            (value == OSIF_SCAN_BAND_5G_ONLY  && IEEE80211_SUPPORT_PHY_MODE(ic,IEEE80211_MODE_11A)) ||
            (value == OSIF_SCAN_BAND_ALL))
        {
            osifp->os_scan_band = value;
        }
        retv = 0;
        break;

    case IEEE80211_PARAM_SCAN_CHAN_EVENT:
        if (osifp->osif_is_mode_offload &&
            wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) {
            osifp->is_scan_chevent = !!value;
            retv = 0;
        } else {
            qdf_print("IEEE80211_PARAM_SCAN_CHAN_EVENT is valid only for 11ac "
                   "offload, and in IEEE80211_M_HOSTAP(Access Point) mode");
            retv = -EOPNOTSUPP;
        }
        break;

#ifdef UMAC_SUPPORT_PROXY_ARP
    case IEEE80211_PARAM_PROXYARP_CAP:
        wlan_set_param(vap, IEEE80211_PROXYARP_CAP, value);
	    break;
#if UMAC_SUPPORT_DGAF_DISABLE
    case IEEE80211_PARAM_DGAF_DISABLE:
        wlan_set_param(vap, IEEE80211_DGAF_DISABLE, value);
        break;
#endif
#endif
#if UMAC_SUPPORT_HS20_L2TIF
    case IEEE80211_PARAM_L2TIF_CAP:
        value = value ? 0 : 1;
        wlan_set_param(vap, IEEE80211_FEATURE_APBRIDGE, value);
       break;
#endif
    case IEEE80211_PARAM_EXT_ACS_IN_PROGRESS:
        wlan_set_param(vap, IEEE80211_EXT_ACS_IN_PROGRESS, value);
        break;

    case IEEE80211_PARAM_SEND_ADDITIONAL_IES:
        wlan_set_param(vap, IEEE80211_SEND_ADDITIONAL_IES, value);
        break;

    case IEEE80211_PARAM_APONLY:
#if UMAC_SUPPORT_APONLY
        vap->iv_aponly = value ? true : false;
        ic->ic_aponly = vap->iv_aponly;
#else
        qdf_print("APONLY not enabled");
#endif
        break;
    case IEEE80211_PARAM_ONETXCHAIN:
        vap->iv_force_onetxchain = value ? true : false;
        break;

    case IEEE80211_PARAM_SET_CABQ_MAXDUR:
        if (value > 0 && value < 100)
            wlan_set_param(vap, IEEE80211_SET_CABQ_MAXDUR, value);
        else
            qdf_print("Percentage should be between 0 and 100");
        break;
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    case IEEE80211_PARAM_NOPBN:
        wlan_set_param(vap, IEEE80211_NOPBN, value);
        break;
#endif
#if ATH_SUPPORT_DSCP_OVERRIDE
    case IEEE80211_PARAM_DSCP_MAP_ID:
        qdf_nofl_info("Set DSCP override %d",value);
        wlan_set_param(vap, IEEE80211_DSCP_MAP_ID, value);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_vops)
        ic->nss_vops->ic_osif_nss_vdev_set_cfg(osifp, OSIF_NSS_WIFI_VDEV_CFG_DSCP_OVERRIDE);
#endif
        break;
    case IEEE80211_PARAM_DSCP_TID_MAP:
        wlan_set_vap_dscp_tid_map(osifp->os_if, val[1], val[2]);
        break;
    case IEEE80211_PARAM_GDSCP_TID_MAP:
        qdf_nofl_err("DSCP    TID");
        qdf_nofl_err("====    ===");
        qdf_nofl_err("0x%x     %d", val[1],
                     ieee80211_ucfg_vap_get_dscp_tid_map(vap, val[1]));
        break;
#endif
    case IEEE80211_PARAM_TXRX_VAP_STATS:
	    retv = ic->ic_vap_set_param(vap, IEEE80211_TXRX_VAP_STATS, value);
        break;
    case IEEE80211_PARAM_TXRX_DBG:
	    retv = ic->ic_vap_set_param(vap, IEEE80211_TXRX_DBG_SET, value);
        break;
    case IEEE80211_PARAM_VAP_TXRX_FW_STATS:
        retv = ic->ic_vap_set_param(vap, IEEE80211_VAP_TXRX_FW_STATS, value);
        break;
    case IEEE80211_PARAM_TXRX_FW_STATS:
        retv = ic->ic_vap_set_param(vap, IEEE80211_TXRX_FW_STATS, value);
        break;
    case IEEE80211_PARAM_TXRX_FW_MSTATS:
        retv = ic->ic_vap_set_param(vap, IEEE80211_TXRX_FW_MSTATS, value);
        break;
    case IEEE80211_PARAM_VAP_TXRX_FW_STATS_RESET:
       retv = ic->ic_vap_set_param(vap, IEEE80211_VAP_TXRX_FW_STATS_RESET, value);
       break;
    case IEEE80211_PARAM_TXRX_FW_STATS_RESET:
       retv = ic->ic_vap_set_param(vap, IEEE80211_TXRX_FW_STATS_RESET, value);
       break;
    case IEEE80211_PARAM_TXRX_DP_STATS:
       retv = ic->ic_vap_set_param(vap, IEEE80211_TXRX_DP_STATS, value);
       break;
    case IEEE80211_PARAM_TX_PPDU_LOG_CFG:
        retv = ic->ic_vap_set_param(vap, IEEE80211_TX_PPDU_LOG_CFG_SET, value);
        break;
    case IEEE80211_PARAM_MAX_SCANENTRY:
        retv = wlan_set_param(vap, IEEE80211_MAX_SCANENTRY, value);
        break;
    case IEEE80211_PARAM_SCANENTRY_TIMEOUT:
        retv = wlan_set_param(vap, IEEE80211_SCANENTRY_TIMEOUT, value);
        break;
#if ATH_PERF_PWR_OFFLOAD && QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    case IEEE80211_PARAM_CLR_RAWMODE_PKT_SIM_STATS:
        retv = wlan_set_param(vap, IEEE80211_CLR_RAWMODE_PKT_SIM_STATS, value);
        break;
#endif /* ATH_PERF_PWR_OFFLOAD && QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
    default:
        retv = -EOPNOTSUPP;
        if (retv) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s parameter 0x%x is "
                            "not supported retv=%d\n", __func__, param, retv);
        }
        break;

   case IEEE80211_PARAM_DFS_CACTIMEOUT:
#if ATH_SUPPORT_DFS
    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
            QDF_STATUS_SUCCESS) {
        return -1;
    }
    retv = mlme_dfs_override_cac_timeout(pdev, value);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    if (retv != 0)
        retv = -EOPNOTSUPP;
    break;
#else
        retv = -EOPNOTSUPP;
    break;
#endif /* ATH_SUPPORT_DFS */

   case IEEE80211_PARAM_ENABLE_RTSCTS:
       retv = wlan_set_param(vap, IEEE80211_ENABLE_RTSCTS, value);
   break;
    case IEEE80211_PARAM_MAX_AMPDU:
        if ((value >= IEEE80211_MAX_AMPDU_MIN) &&
            (value <= IEEE80211_MAX_AMPDU_MAX)) {
            retv = wlan_set_param(vap, IEEE80211_MAX_AMPDU, value);
 	        if ( retv == EOK ) {
                retv = ENETRESET;
                restart_vap = true;
            }
        } else {
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_VHT_MAX_AMPDU:
        if ((value >= IEEE80211_VHT_MAX_AMPDU_MIN) &&
            (value <= IEEE80211_VHT_MAX_AMPDU_MAX)) {
            retv = wlan_set_param(vap, IEEE80211_VHT_MAX_AMPDU, value);
            if ( retv == EOK ) {
                retv = ENETRESET;
                restart_vap = true;
            }
        } else {
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_IMPLICITBF:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_IMPLICITBF, value);
        if ( retv == EOK ) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_VHT_SUBFEE:
        if (value == 0 ) {
            /* if SU is disabled, disable MU as well */
            wlan_set_param(vap, IEEE80211_VHT_MUBFEE, value);
        }
        retv = wlan_set_param(vap, IEEE80211_VHT_SUBFEE, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_VHT_MUBFEE:
        if (value == 1) {
            /* if MU is enabled, enable SU as well */
            wlan_set_param(vap, IEEE80211_VHT_SUBFEE, value);
        }
        retv = wlan_set_param(vap, IEEE80211_VHT_MUBFEE, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_VHT_SUBFER:
        if (value == 0 ) {
            /* if SU is disabled, disable MU as well */
            wlan_set_param(vap, IEEE80211_VHT_MUBFER, value);
        }
        retv = wlan_set_param(vap, IEEE80211_VHT_SUBFER, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_VHT_MUBFER:
        if (value == 1 ) {
            /* if MU is enabled, enable SU as well */
            wlan_set_param(vap, IEEE80211_VHT_SUBFER, value);
        }
        retv = wlan_set_param(vap, IEEE80211_VHT_MUBFER, value);
        if (retv == 0 && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_VHT_STS_CAP:
        retv = wlan_set_param(vap, IEEE80211_VHT_BF_STS_CAP, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_VHT_SOUNDING_DIM:
        retv = wlan_set_param(vap, IEEE80211_VHT_BF_SOUNDING_DIM, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_VHT_MCS_10_11_SUPP:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_VHT_MCS_10_11_SUPP, value);
        if(retv == EOK) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_VHT_MCS_10_11_NQ2Q_PEER_SUPP:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_VHT_MCS_10_11_NQ2Q_PEER_SUPP, value);
        if(retv == EOK) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_MCAST_RC_STALE_PERIOD:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_MCAST_RC_STALE_PERIOD, value);
        break;
    case IEEE80211_PARAM_ENABLE_MCAST_RC:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ENABLE_MCAST_RC, value);
        break;
    case IEEE80211_PARAM_RC_NUM_RETRIES:
        retv = wlan_set_param(vap, IEEE80211_RC_NUM_RETRIES, value);
        break;
    case IEEE80211_PARAM_256QAM_2G:
        retv = wlan_set_param(vap, IEEE80211_256QAM, value);
        if (retv == EOK) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_11NG_VHT_INTEROP:
        if (osifp->osif_is_mode_offload) {
            retv = wlan_set_param(vap, IEEE80211_11NG_VHT_INTEROP , value);
            if (retv == EOK) {
                retv = ENETRESET;
            }
        } else {
            qdf_print("Not supported in this vap ");
        }
        break;
#if UMAC_VOW_DEBUG
    case IEEE80211_PARAM_VOW_DBG_ENABLE:
        {
            osifp->vow_dbg_en = value;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (ic->nss_vops)
            ic->nss_vops->ic_osif_nss_vdev_set_cfg(osifp, OSIF_NSS_WIFI_VDEV_VOW_DBG_MODE);
#endif
        }
        break;
#endif

#if WLAN_SUPPORT_SPLITMAC
    case IEEE80211_PARAM_SPLITMAC:
        splitmac_set_enabled_flag(vdev, !!value);

        if (splitmac_is_enabled(vdev)) {
            osifp->app_filter =  IEEE80211_FILTER_TYPE_ALL;
            wlan_set_param(vap, IEEE80211_TRIGGER_MLME_RESP, 1);
#ifdef WLAN_FEATURE_11BE_MLO
            if (vdev->mlo_dev_ctx && vdev->mlo_dev_ctx->ap_ctx)
                vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject = 0;
#endif
        } else {
            osifp->app_filter =  0;
            wlan_set_param(vap, IEEE80211_TRIGGER_MLME_RESP, 0);
        }
        break;
#endif
#if ATH_PERF_PWR_OFFLOAD
    case IEEE80211_PARAM_VAP_TX_ENCAP_TYPE:
        retv = wlan_set_param(vap, IEEE80211_VAP_TX_ENCAP_TYPE, value);
        if(retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_VAP_RX_DECAP_TYPE:
        retv = wlan_set_param(vap, IEEE80211_VAP_RX_DECAP_TYPE, value);
        if(retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    case IEEE80211_PARAM_RAWMODE_SIM_TXAGGR:
        retv = wlan_set_param(vap, IEEE80211_RAWMODE_SIM_TXAGGR, value);
        break;
    case IEEE80211_PARAM_RAWMODE_SIM_DEBUG_LEVEL:
        retv = wlan_set_param(vap, IEEE80211_RAWMODE_SIM_DEBUG_LEVEL, value);
        break;
    case IEEE80211_PARAM_RAWSIM_DEBUG_NUM_ENCAP_FRAMES:
        retv = wlan_set_param(vap, IEEE80211_RAWSIM_DEBUG_NUM_ENCAP_FRAMES, value);
        break;
    case IEEE80211_PARAM_RAWSIM_DEBUG_NUM_DECAP_FRAMES:
        retv = wlan_set_param(vap, IEEE80211_RAWSIM_DEBUG_NUM_DECAP_FRAMES, value);
        break;
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
#endif /* ATH_PERF_PWR_OFFLOAD */
    case IEEE80211_PARAM_STA_FIXED_RATE:
        /* set a fixed data rate for an associated STA on a AP vap.
         * assumes that vap is already enabled for fixed rate, and
         * this setting overrides the vap's setting.
         *
         * encoding (legacey)  : ((aid << 8)  |
         *                          (preamble << 6) | ((nss-1) << 4) | mcs)
         * encoding (he_target): ((aid << 16) |
         *                          (preamble << 8) | ((nss-1) << 5) | mcs)
         * preamble (OFDM) = 0x0
         * preamble (CCK)  = 0x1
         * preamble (HT)   = 0x2
         * preamble (VHT)  = 0x3
         * preamble (HE)   = 0x4
         */
        if (osifp->os_opmode != IEEE80211_M_HOSTAP) {
            return -EINVAL;
        }

        if (IEEE80211_VAP_IN_FIXED_RATE_MODE(vap)) {
            struct find_wlan_node_req req;
            if (!ic->ic_he_target) {
                req.assoc_id = ((value >> RATECODE_LEGACY_RC_SIZE)
                                        & IEEE80211_ASSOC_ID_MASK);
            } else {
                req.assoc_id = ((value >> RATECODE_V1_RC_SIZE)
                                        & IEEE80211_ASSOC_ID_MASK);
            }
            req.node = NULL;
            wlan_iterate_station_list(vap, find_wlan_node_by_associd, &req);
            if (req.node) {
                uint32_t fixed_rate;
                if (!ic->ic_he_target) {
                    fixed_rate = value & RATECODE_LEGACY_RC_MASK;
                } else {
                    fixed_rate = value & RATECODE_V1_RC_MASK;
                    /* V1 rate code format is ((((1) << 28) |(pream) << 8)
                     * | ((nss) << 5) | (rate)). With this command, user
                     * will send us  16 bit _rate = (((_pream) << 8) | ((nss)
                     * << 5) | rate). We need to assemble user sent _rate as
                     * V1 rate = (((1) << 28) | _rate).
                     */
                    fixed_rate = V1_RATECODE_FROM_RATE(fixed_rate);
                }
                retv = wlan_node_set_fixed_rate(req.node, fixed_rate);
            } else {
                return -EINVAL;
            }
        }
        break;

#if QCA_AIRTIME_FAIRNESS
    case IEEE80211_PARAM_ATF_TXBUF_SHARE:
        retv = ucfg_atf_set_txbuf_share(vap->vdev_obj, value);
        break;
    case IEEE80211_PARAM_ATF_TXBUF_MAX:
        ucfg_atf_set_max_txbufs(vap->vdev_obj, value);
        break;
    case IEEE80211_PARAM_ATF_TXBUF_MIN:
        ucfg_atf_set_min_txbufs(vap->vdev_obj, value);
        break;
    case  IEEE80211_PARAM_ATF_OPT:
        if (value > 1) {
            qdf_print("update commit value(1) wrong ");
            return -EINVAL;
        }
        if (value)
            retv = ucfg_atf_set(vap->vdev_obj);
        else
            retv = ucfg_atf_clear(vap->vdev_obj);
        break;
    case IEEE80211_PARAM_ATF_OVERRIDE_AIRTIME_TPUT:
        ucfg_atf_set_airtime_tput(vap->vdev_obj, value);
        break;
    case  IEEE80211_PARAM_ATF_PER_UNIT:
        ucfg_atf_set_per_unit(ic->ic_pdev_obj);
        break;
    case  IEEE80211_PARAM_ATF_MAX_CLIENT:
        retv = ucfg_atf_set_maxclient(ic->ic_pdev_obj, value);
        break;
    case  IEEE80211_PARAM_ATF_SSID_GROUP:
        ucfg_atf_set_ssidgroup(ic->ic_pdev_obj, vap->vdev_obj, value);
        break;
    case  IEEE80211_PARAM_ATF_VIP_INFRA:
        ucfg_atf_set_vip_infra(vap->vdev_obj, value);
        break;
    case IEEE80211_PARAM_ATF_SSID_SCHED_POLICY:
        retv = ucfg_atf_set_ssid_sched_policy(vap->vdev_obj, value);
        break;
#if QCA_SUPPORT_ATF_STATS
    case IEEE80211_PARAM_ATF_ENABLE_STATS:
        if (ic->ic_is_target_lithium && ic->ic_is_target_lithium(psoc)) {
            retv = ucfg_atf_enable_stats(ic->ic_pdev_obj, value);
        } else {
            qdf_err("ATF STATS is not supported in this Target!\n");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_ATF_STATS_TIMEOUT:
        if (ic->ic_is_target_lithium && ic->ic_is_target_lithium(psoc)) {
            retv = ucfg_atf_set_stats_timeout(ic->ic_pdev_obj, value);
        } else {
            qdf_err("ATF STATS is not supported in this Target!\n");
            return -EINVAL;
        }
        break;
#endif /* QCA_SUPPORT_ATF_STATS */
#endif /* QCA_AIRTIME_FAIRNESS */
#if QCA_SUPPORT_SSID_STEERING
    case IEEE80211_PARAM_VAP_SSID_CONFIG:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_VAP_SSID_STEERING, value);
        break;
#endif
    case IEEE80211_PARAM_RX_FILTER_MONITOR:
        if(IEEE80211_M_MONITOR != vap->iv_opmode && !IEEE80211_IS_SPECIAL_VAP(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Not monitor VAP or Smart monitor VAP!\n");
            return -EINVAL;
        }

        if (ic->ic_monitor_version == MONITOR_VERSION_2 && IEEE80211_IS_SMART_MON_VAP(vap)) {
            qdf_print("Skipped advance monitor filter for smart monitor!\n");
            return -EINVAL;
        }

        ic->ic_os_monrxfilter = RX_FILTER_GET64(val);

        if (wlan_set_param(vap, IEEE80211_RX_FILTER_MONITOR, value))
            return -EINVAL;
        break;
    case  IEEE80211_PARAM_RX_FILTER_NEIGHBOUR_PEERS_MONITOR:
        /* deliver configured bss peer packets, associated to smart
         * monitor vap and filter out other valid/invalid peers
         */
        if(!IEEE80211_IS_SMART_MON_VAP(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Not smart monitor VAP!\n");
            return -EINVAL;
        }
        retv = wlan_set_param(vap, IEEE80211_RX_FILTER_NEIGHBOUR_PEERS_MONITOR, value);
        break;
    case IEEE80211_PARAM_AMPDU_DENSITY_OVERRIDE:
        if(value < 0) {
            ic->ic_mpdudensityoverride = 0;
        } else if(value <= IEEE80211_HTCAP_MPDUDENSITY_MAX) {
            /* mpdudensityoverride
             * Bits
             * 7 --  4   3  2  1    0
             * +------------------+----+
             * |       |  MPDU    |    |
             * | Rsvd  | DENSITY  |E/D |
             * +---------------+-------+
             */
            ic->ic_mpdudensityoverride = 1;
            ic->ic_mpdudensityoverride |= ((u_int8_t)value & 0x07) << 1;
        } else {
            qdf_print("Usage:\n"
                    "-1 - Disable mpdu density override \n"
                    "%d - No restriction \n"
                    "%d - 1/4 usec \n"
                    "%d - 1/2 usec \n"
                    "%d - 1 usec \n"
                    "%d - 2 usec \n"
                    "%d - 4 usec \n"
                    "%d - 8 usec \n"
                    "%d - 16 usec ",
                    IEEE80211_HTCAP_MPDUDENSITY_NA,
                    IEEE80211_HTCAP_MPDUDENSITY_0_25,
                    IEEE80211_HTCAP_MPDUDENSITY_0_5,
                    IEEE80211_HTCAP_MPDUDENSITY_1,
                    IEEE80211_HTCAP_MPDUDENSITY_2,
                    IEEE80211_HTCAP_MPDUDENSITY_4,
                    IEEE80211_HTCAP_MPDUDENSITY_8,
                    IEEE80211_HTCAP_MPDUDENSITY_16);
            retv = EINVAL;
        }
        /* Reset VAP */
        if(retv != EINVAL) {
            struct ieee80211_ath_channel chan = wlan_get_bss_channel(vap);
            if (IEEE80211_IS_CHAN_VALID(&chan)) {
                retv = ENETRESET;
                restart_vap = true;
            }
        }
        break;

    case IEEE80211_PARAM_SMART_MESH_CONFIG:
        retv = wlan_set_param(vap, IEEE80211_SMART_MESH_CONFIG, value);
        break;
#if MESH_MODE_SUPPORT
    case IEEE80211_PARAM_MESH_CAPABILITIES:
        retv = wlan_set_param(vap, IEEE80211_MESH_CAPABILITIES, value);
        break;
    case IEEE80211_PARAM_ADD_LOCAL_PEER:
        if (vap->iv_mesh_vap_mode) {
            qdf_print("adding local peer ");
            retv = ieee80211_add_localpeer(vap,extra);
        } else {
            return -EPERM;
        }
        break;
    case IEEE80211_PARAM_SET_MHDR:
        if(vap && vap->iv_mesh_vap_mode) {
            qdf_print("setting mhdr %x",value);
            vap->mhdr = value;
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
            wlan_update_rawsim_config(vap);
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
        } else {
            return -EPERM;
        }
        break;
    case IEEE80211_PARAM_ALLOW_DATA:
        if (vap->iv_mesh_vap_mode)
            retv = ieee80211_authorise_local_peer(vap,extra);
        else
            return -EPERM;
        break;
    case IEEE80211_PARAM_SET_MESHDBG:
        if(vap && vap->iv_mesh_vap_mode) {
            qdf_print("mesh dbg %x",value);
            vap->mdbg = value;
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
            wlan_update_rawsim_config(vap);
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
        } else {
            return -EPERM;
        }
        break;
    case IEEE80211_PARAM_CONFIG_MGMT_TX_FOR_MESH:
        if (vap->iv_mesh_vap_mode) {
          retv = wlan_set_param(vap, IEEE80211_CONFIG_MGMT_TX_FOR_MESH, value);
        } else {
            return -EPERM;
        }
        break;

    case IEEE80211_PARAM_MESH_MCAST:
        if (vap->iv_mesh_vap_mode) {
          retv = wlan_set_param(vap, IEEE80211_CONFIG_MESH_MCAST, value);
        } else {
            return -EPERM;
        }
        break;

#if ATH_ACS_DEBUG_SUPPORT
    case IEEE80211_PARAM_ACS_DEBUG_SUPPORT:
        ic->ic_acs_debug_support = value ? 1 : 0;

        if (!value) {
            acs_debug_cleanup(ic->ic_acs);
        }
        break;
#endif

    case IEEE80211_PARAM_CONFIG_RX_MESH_FILTER:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_RX_MESH_FILTER, value);
        break;

#if ATH_DATA_RX_INFO_EN
    case IEEE80211_PARAM_RXINFO_PERPKT:
        vap->rxinfo_perpkt = !!value;
        break;
#endif
#endif

#ifdef VDEV_PEER_PROTOCOL_COUNT
    case IEEE80211_PARAM_VDEV_PEER_PROTOCOL_COUNT:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_VDEV_PEER_PROTOCOL_COUNT,
                              value);
        osifp->peer_protocol_cnt = value;
        break;
    case IEEE80211_PARAM_VDEV_PEER_PROTOCOL_DROP_MASK:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_VDEV_PEER_PROTOCOL_DROP_MASK,
                              value);
        break;
#endif

    case IEEE80211_PARAM_CONFIG_ASSOC_WAR_160W:
        if ((value == 0) || ASSOCWAR160_IS_VALID_CHANGE(value))
            retv = wlan_set_param(vap, IEEE80211_CONFIG_ASSOC_WAR_160W, value);
        else {
            qdf_print("Invalid value %d. Valid bitmap values are 0:Disable, 1:Enable VHT OP, 3:Enable VHT OP and VHT CAP",value);
            return -EINVAL;
        }
 break;
    case IEEE80211_PARAM_RAWMODE_PKT_SIM:
        retv = wlan_set_param(vap, IEEE80211_RAWMODE_PKT_SIM, value);
        break;
    case IEEE80211_PARAM_CONFIG_RAW_DWEP_IND:
        if ((value == 0) || (value == 1))
            retv = wlan_set_param(vap, IEEE80211_CONFIG_RAW_DWEP_IND, value);
        else {
            qdf_print("Invalid value %d. Valid values are 0:Disable, 1:Enable",value);
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_CUSTOM_CHAN_LIST:
        retv = wlan_set_param(vap,IEEE80211_CONFIG_PARAM_CUSTOM_CHAN_LIST, value);
        break;
#if UMAC_SUPPORT_ACFG
    case IEEE80211_PARAM_DIAG_WARN_THRESHOLD:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_DIAG_WARN_THRESHOLD, value);
        break;
    case IEEE80211_PARAM_DIAG_ERR_THRESHOLD:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_DIAG_ERR_THRESHOLD, value);
        break;
#endif
     case IEEE80211_PARAM_CONFIG_REV_SIG_160W:
        if(!wlan_get_param(vap, IEEE80211_CONFIG_ASSOC_WAR_160W)){
            if ((value == 0) || (value == 1))
                retv = wlan_set_param(vap, IEEE80211_CONFIG_REV_SIG_160W, value);
            else {
                qdf_print("Invalid value %d. Valid values are 0:Disable, 1:Enable",value);
                return -EINVAL;
            }
        } else {
            qdf_print("revsig160 not supported with assocwar160");
            return -EINVAL;
        }
        break;
     case IEEE80211_PARAM_CONFIG_MU_CAP_WAR:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_MU_CAP_WAR, value);
        break;
     case IEEE80211_PARAM_CONFIG_MU_CAP_TIMER:
        if ((value >= 1) && (value <= 300))
            retv = wlan_set_param(vap, IEEE80211_CONFIG_MU_CAP_TIMER, value);
        else {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_IOCTL,
                            "Invalid value %d. Valid value is between 1 and 300\n",value);
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_DISABLE_SELECTIVE_HTMCS_FOR_VAP:
        if(vap->iv_opmode == IEEE80211_M_HOSTAP) {
            retv = wlan_set_param(vap, IEEE80211_CONFIG_DISABLE_SELECTIVE_HTMCS, value);
            if(retv == 0) {
                retv = ENETRESET;
                restart_vap = true;
            }
        } else {
            qdf_print("This iwpriv option disable_htmcs is valid only for AP mode vap");
        }
        break;
    case IEEE80211_PARAM_CONFIGURE_SELECTIVE_VHTMCS_FOR_VAP:
        if(vap->iv_opmode == IEEE80211_M_HOSTAP) {
            if(value != 0) {
                retv = wlan_set_param(vap, IEEE80211_CONFIG_CONFIGURE_SELECTIVE_VHTMCS, value);
                if(retv == 0) {
                    retv = ENETRESET;
                    restart_vap = true;
                }
            }
        } else {
            qdf_print("This iwpriv option conf_11acmcs is valid only for AP mode vap");
        }
        break;
    case IEEE80211_PARAM_RDG_ENABLE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_RDG_ENABLE, value);
        break;
    case IEEE80211_PARAM_CLEAR_QOS:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_CLEAR_QOS,value);
        break;
    case IEEE80211_PARAM_TRAFFIC_STATS:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_TRAFFIC_STATS,value);
        break;
    case IEEE80211_PARAM_TRAFFIC_RATE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_TRAFFIC_RATE,value);
        break;
    case IEEE80211_PARAM_TRAFFIC_INTERVAL:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_TRAFFIC_INTERVAL,value);
        break;
    case IEEE80211_PARAM_WATERMARK_THRESHOLD:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_WATERMARK_THRESHOLD,value);
        break;
    case IEEE80211_PARAM_ENABLE_VENDOR_IE:
        if ((value >= IEEE80211_DISABLE_ATH_VENDOR_IES) &&
            (value <= IEEE80211_ENABLE_ATH_VENDOR_IES)) {
            if (vap->iv_ena_vendor_ie != value) {
                vap->iv_update_vendor_ie = 1;
            }
            vap->iv_ena_vendor_ie = value;
            /* Update beacon template on toggle to support dynamic config */
            if (vap->iv_update_vendor_ie) {
                if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
                    vap->iv_mbss.non_tx_profile_change = true;
                    vap->iv_mbss.mbssid_update_ie = true;
                }
                wlan_vdev_beacon_update(vap);
            }

        } else {
            qdf_err("Enter 0: Disable Ath vendor IEs\n"
                    "    0x1: Enable Ath Advanced Capabilities vendor IE\n"
                    "    0x2: Enable Ath Extended Capabilities vendor IE\n"
                    "    0x3: Enable Ath vendor IEs");
        }
        break;
    case IEEE80211_PARAM_CONFIG_ASSOC_DENIAL_NOTIFY:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ASSOC_DENIAL_NOTIFICATION, value);
        break;
    case IEEE80211_PARAM_MACCMD_SEC:
        retv = wlan_set_acl_policy(vap, value, IEEE80211_ACL_FLAG_ACL_LIST_2);
        break;
    case IEEE80211_PARAM_CONFIG_MON_DECODER:
        if (IEEE80211_M_MONITOR != vap->iv_opmode) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Not Monitor VAP!\n");
            return -EINVAL;
        }
        /* monitor vap decoder header type: radiotap=0(default) prism=1 */
        if (value != 0 && value != 1) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Invalid value! radiotap=0(default) prism=1 \n");
            return -EINVAL;
        }

        if (value == 0)
            dev->type = ARPHRD_IEEE80211_RADIOTAP;
        else
            dev->type = ARPHRD_IEEE80211_PRISM;

        wlan_set_param(vap, IEEE80211_CONFIG_MON_DECODER, value);
        break;
    case IEEE80211_PARAM_SIFS_TRIGGER_RATE:
        if (ic->ic_vap_set_param) {
           retv = ic->ic_vap_set_param(vap, IEEE80211_SIFS_TRIGGER_RATE, (u_int32_t)value);
        }
        break;
    case IEEE80211_PARAM_BEACON_RATE_FOR_VAP:
        if(vap->iv_opmode == IEEE80211_M_HOSTAP) {
            int *rate_kbps = NULL;
            int i,j,rateKbps = 0;

            switch (vap->iv_des_mode)
            {
                case IEEE80211_MODE_11B:
                    {
                        rate_table.nrates = sizeof(basic_11b_rate)/sizeof(basic_11b_rate[0]);
                        rate_table.rates = (int *)&basic_11b_rate;
                        /* Checking the boundary condition for the valid rates */
                        if ((value < 1000) || (value > 11000)) {
                            qdf_nofl_info("%s : WARNING: Please try a valid rate between 1000 to 11000 kbps.",__func__);
                            return -EINVAL;
                        }
                    }
                    break;

                case IEEE80211_MODE_TURBO_G:
                case IEEE80211_MODE_11NG_HT20:
                case IEEE80211_MODE_11NG_HT40PLUS:
                case IEEE80211_MODE_11NG_HT40MINUS:
                case IEEE80211_MODE_11NG_HT40:
                case IEEE80211_MODE_11G:
                case IEEE80211_MODE_11AXG_HE20:
                case IEEE80211_MODE_11AXG_HE40PLUS:
                case IEEE80211_MODE_11AXG_HE40MINUS:
                case IEEE80211_MODE_11AXG_HE40:
#ifdef WLAN_FEATURE_11BE
                case IEEE80211_MODE_11BEG_EHT20:
                case IEEE80211_MODE_11BEG_EHT40PLUS:
                case IEEE80211_MODE_11BEG_EHT40MINUS:
                case IEEE80211_MODE_11BEG_EHT40:
#endif /* WLAN_FEATURE_11BE */
                    {
                        rate_table.nrates = sizeof(basic_11bgn_rate)/sizeof(basic_11bgn_rate[0]);
                        rate_table.rates = (int *)&basic_11bgn_rate;
                        /* Checking the boundary condition for the valid rates */
                        if ((value < 1000) || (value > 24000)){
                            qdf_nofl_info("%s : WARNING: Please try a valid rate between 1000 to 24000 kbps.",__func__);
                            return -EINVAL;
                        }
                    }
                    break;

                case IEEE80211_MODE_11A:
                case IEEE80211_MODE_TURBO_A:
                case IEEE80211_MODE_11NA_HT20:
                case IEEE80211_MODE_11NA_HT40PLUS:
                case IEEE80211_MODE_11NA_HT40MINUS:
                case IEEE80211_MODE_11NA_HT40:
                case IEEE80211_MODE_11AC_VHT20:
                case IEEE80211_MODE_11AC_VHT40PLUS:
                case IEEE80211_MODE_11AC_VHT40MINUS:
                case IEEE80211_MODE_11AC_VHT40:
                case IEEE80211_MODE_11AC_VHT80:
                case IEEE80211_MODE_11AC_VHT160:
                case IEEE80211_MODE_11AC_VHT80_80:
                case IEEE80211_MODE_11AXA_HE20:
                case IEEE80211_MODE_11AXA_HE40PLUS:
                case IEEE80211_MODE_11AXA_HE40MINUS:
                case IEEE80211_MODE_11AXA_HE40:
                case IEEE80211_MODE_11AXA_HE80:
                case IEEE80211_MODE_11AXA_HE160:
                case IEEE80211_MODE_11AXA_HE80_80:
#ifdef WLAN_FEATURE_11BE
                case IEEE80211_MODE_11BEA_EHT20:
                case IEEE80211_MODE_11BEA_EHT40PLUS:
                case IEEE80211_MODE_11BEA_EHT40MINUS:
                case IEEE80211_MODE_11BEA_EHT40:
                case IEEE80211_MODE_11BEA_EHT80:
                case IEEE80211_MODE_11BEA_EHT160:
                case IEEE80211_MODE_11BEA_EHT320:
#endif /* WLAN_FEATURE_11BE */
                    {
                        rate_table.nrates = (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) ?
                            sizeof(basic_11ax_6g_rate)/sizeof(basic_11ax_6g_rate[0]) :
                            sizeof(basic_11na_rate)/sizeof(basic_11na_rate[0]);
                        /* For 6GHz, check if the beacon rate setting is one of
                         * the basic rates or the allowed HE rates
                         */
                        rate_table.rates = (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) ?
                           ((int *)&basic_11ax_6g_rate) : ((int *)&basic_11na_rate);

                        /* Checking the boundary condition for the valid rates */
                        if ((value < 6000) || (value > 73100)){
                            qdf_nofl_info("%s : WARNING: Please try a valid rate between 6000 to 73100 kbps.",__func__);
                            return -EINVAL;
                        }
                    }
                    break;

                default:
                    {
                        qdf_nofl_info("%s : WARNING:Invalid channel mode.",__func__);
                        return -EINVAL;
                    }
            }
            rate_kbps = rate_table.rates;

            /* Check if the rate given by user is a valid basic rate.
             * If it is valid basic rate then go with that rate or else
             * if the rate passed by the user is not valid basic rate but
             * it falls inside the valid rate boundary corresponding to the
             * phy mode, then opt for the next valid basic rate.
             */
            for (i = 0; i < rate_table.nrates; i++) {
                if (value == *rate_kbps) {
                    rateKbps = *rate_kbps;
                } else if (value < *rate_kbps) {
                    rateKbps = *rate_kbps;
                    qdf_nofl_info("%s : MSG: Not a valid basic rate.",__func__);
                    qdf_nofl_info("Since the requested rate is below 24Mbps, moving forward and selecting the next valid basic rate : %d (Kbps)",rateKbps);
                }
                if (rateKbps) {
                    /* use the iv_op_rates, the VAP may not be up yet */
                    for (j = 0; j < vap->iv_op_rates[vap->iv_des_mode].rs_nrates; j++) {
                        if (rateKbps == (((vap->iv_op_rates[vap->iv_des_mode].rs_rates[j] & IEEE80211_RATE_VAL)* 1000) / 2)) {
                            found = 1;
                            break;
                        }
                    }

                    if(!found) {
                        /* Check if it is a valid 6GHz rate */
                        found = ieee80211_is_6g_valid_rate(vap, rateKbps);
                    }

                    if (!found) {
                        if (rateKbps == HIGHEST_BASIC_RATE) {
                            qdf_nofl_info("%s : MSG: Reached end of the table.",__func__);
                        } else {
                            value = *(rate_kbps+1);
                            qdf_nofl_info("%s : MSG: User opted rate is disabled. Hence going for the next rate: %d (Kbps)",__func__,value);
                        }
                    } else {
                        qdf_nofl_info("%s : Rate to be configured for beacon is: %d (Kbps)",__func__,rateKbps);
                        break;
                    }
                }
                rate_kbps++;
            }

            /* Suppose user has passed one rate and along with that rate the
             * higher rates are also not available in the rate table then
             * go with the lowest available basic rate.
             */
            if (!found) {
                uint32_t mgmt_rate;

                wlan_util_vdev_mlme_get_param(vap->vdev_mlme,
                        WLAN_MLME_CFG_TX_MGMT_RATE, &mgmt_rate);
                rateKbps = mgmt_rate;
                qdf_nofl_info("%s: MSG: The opted rate or higher rates are not available in node rate table.",__func__);
                qdf_nofl_info("MSG: Hence choosing the lowest available rate : %d (Kbps)",rateKbps);
            }
            retv = wlan_set_param(vap, IEEE80211_BEACON_RATE_FOR_VAP,rateKbps);
            if(retv == 0) {
                retv = ENETRESET;
                if (ieee80211_is_he_bcn_rate(vap)) {
                    vap->iv_ic->ic_non_ht_dup &=
                        ~(1 << IEEE80211_NON_HT_DUP_BEACON_S);
                    vap->iv_ic->ic_vap_set_param(vap,
                        IEEE80211_CONFIG_6GHZ_NON_HT_DUP,
                        vap->iv_ic->ic_non_ht_dup);
                }
            }
        } else {
            qdf_nofl_info("%s : WARNING: Setting beacon rate is allowed only for AP VAP ",__func__);
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_DISABLE_SELECTIVE_LEGACY_RATE_FOR_VAP:
        if(vap->iv_opmode == IEEE80211_M_HOSTAP) {

            switch (vap->iv_des_mode)
            {
                case IEEE80211_MODE_11A:
                case IEEE80211_MODE_TURBO_A:
                case IEEE80211_MODE_11NA_HT20:
                case IEEE80211_MODE_11NA_HT40PLUS:
                case IEEE80211_MODE_11NA_HT40MINUS:
                case IEEE80211_MODE_11NA_HT40:
                case IEEE80211_MODE_11AC_VHT20:
                case IEEE80211_MODE_11AC_VHT40PLUS:
                case IEEE80211_MODE_11AC_VHT40MINUS:
                case IEEE80211_MODE_11AC_VHT40:
                case IEEE80211_MODE_11AC_VHT80:
                case IEEE80211_MODE_11AC_VHT160:
                case IEEE80211_MODE_11AC_VHT80_80:
                case IEEE80211_MODE_11AXA_HE20:
                case IEEE80211_MODE_11AXA_HE40PLUS:
                case IEEE80211_MODE_11AXA_HE40MINUS:
                case IEEE80211_MODE_11AXA_HE40:
                case IEEE80211_MODE_11AXA_HE80:
                case IEEE80211_MODE_11AXA_HE160:
                case IEEE80211_MODE_11AXA_HE80_80:
#ifdef WLAN_FEATURE_11BE
                case IEEE80211_MODE_11BEA_EHT20:
                case IEEE80211_MODE_11BEA_EHT40PLUS:
                case IEEE80211_MODE_11BEA_EHT40MINUS:
                case IEEE80211_MODE_11BEA_EHT40:
                case IEEE80211_MODE_11BEA_EHT80:
                case IEEE80211_MODE_11BEA_EHT160:
                case IEEE80211_MODE_11BEA_EHT320:
#endif /* WLAN_FEATURE_11BE */
                   /* Mask has been set for rates: 24, 12 and 6 Mbps */
                    basic_valid_mask = 0x0150;
                    break;
                case IEEE80211_MODE_11B:
                   /* Mask has been set for rates: 11, 5.5, 2 and 1 Mbps */
                    basic_valid_mask = 0x000F;
                    break;
                case IEEE80211_MODE_TURBO_G:
                case IEEE80211_MODE_11NG_HT20:
                case IEEE80211_MODE_11NG_HT40PLUS:
                case IEEE80211_MODE_11NG_HT40MINUS:
                case IEEE80211_MODE_11NG_HT40:
                case IEEE80211_MODE_11AXG_HE20:
                case IEEE80211_MODE_11AXG_HE40PLUS:
                case IEEE80211_MODE_11AXG_HE40MINUS:
                case IEEE80211_MODE_11AXG_HE40:
                case IEEE80211_MODE_11G:
#ifdef WLAN_FEATURE_11BE
                case IEEE80211_MODE_11BEG_EHT20:
                case IEEE80211_MODE_11BEG_EHT40PLUS:
                case IEEE80211_MODE_11BEG_EHT40MINUS:
                case IEEE80211_MODE_11BEG_EHT40:
#endif /* WLAN_FEATURE_11BE */
                   /* Mask has been set for rates: 24, 12, 6, 11, 5.5, 2 and 1 Mbps */
                    basic_valid_mask = 0x015F;
                    break;
                default:
                    break;
            }
           /*
            * Mask has been set as per the desired mode so that user will not be able to disable
            * all the supported basic rates.
            */

            if ((value & basic_valid_mask) != basic_valid_mask) {
                retv = wlan_set_param(vap, IEEE80211_CONFIG_DISABLE_SELECTIVE_LEGACY_RATE, value);
                if(retv == 0) {
                    retv = ENETRESET;
                    restart_vap = true;
                }
            } else {
                qdf_print("%s : WARNING: Disabling all basic rates is not permitted.",__func__);
            }
        } else {
            qdf_print("%s : WARNING: This iwpriv option dis_legacy is valid only for the VAP in AP mode",__func__);
        }
        break;

    case IEEE80211_PARAM_CONFIG_NSTSCAP_WAR:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_NSTSCAP_WAR,value);
    break;

    case IEEE80211_PARAM_TXPOW_MGMT:
    {
        uint8_t frame_subtype = val[1];
        int tx_power = val[2];

        if(!tx_pow_mgmt_valid(frame_subtype,&tx_power))
            return -EINVAL;
        transmit_power = (tx_power &  0xFF);
        vap->iv_txpow_mgt_frm[(frame_subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)] = transmit_power;
        retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_VAP_TXPOW_MGMT, frame_subtype);
    }
    break;
     case IEEE80211_PARAM_TXPOW:
     {
        uint8_t frame_subtype = val[1];
        int tx_power = val[2];

        frame_type = val[1]>>IEEE80211_SUBTYPE_TXPOW_SHIFT;
        frame_subtype = (val[1]&0xff);
        if (!tx_pow_valid(frame_type, frame_subtype, &tx_power))
            return -EINVAL;

        transmit_power = (tx_power &  0xFF);
        if (0xff == frame_subtype) {
            for (loop_index = 0; loop_index < MAX_NUM_TXPOW_MGT_ENTRY; loop_index++) {
                vap->iv_txpow_frm[frame_type>>IEEE80211_FC0_TYPE_SHIFT][loop_index] = transmit_power;
            }
        } else {
            vap->iv_txpow_frm[frame_type >> IEEE80211_FC0_TYPE_SHIFT][(frame_subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)] = transmit_power;
        }

        qdf_print("TPC offload called");
        if (IEEE80211_FC0_TYPE_MGT == frame_type) {
            if (0xff != frame_subtype) {
                if ((IEEE80211_FC0_SUBTYPE_ACTION == frame_subtype) || (IEEE80211_FC0_SUBTYPE_PROBE_REQ == frame_subtype)) {
                    retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_VAP_TXPOW,
                                                (frame_type << IEEE80211_FRAMETYPE_TXPOW_SHIFT) + (frame_subtype << IEEE80211_SUBTYPE_TXPOW_SHIFT) + transmit_power);
                } else {
                    vap->iv_txpow_mgt_frm[(frame_subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)] = transmit_power;
                    retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_VAP_TXPOW_MGMT, frame_subtype);
                }
            } else {
                for (loop_index=0 ; loop_index < MAX_NUM_TXPOW_MGT_ENTRY; loop_index++) {
                    vap->iv_txpow_mgt_frm[loop_index] = transmit_power;
                    retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_VAP_TXPOW_MGMT, loop_index<<IEEE80211_FC0_SUBTYPE_SHIFT);
                }
                retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_VAP_TXPOW,
                                            (frame_type << IEEE80211_FRAMETYPE_TXPOW_SHIFT) + (frame_subtype << IEEE80211_SUBTYPE_TXPOW_SHIFT) + transmit_power);
            }
        } else {
            retv = ic->ic_vap_set_param(vap, IEEE80211_CONFIG_VAP_TXPOW,
                                        (frame_type << IEEE80211_FRAMETYPE_TXPOW_SHIFT) + (frame_subtype << IEEE80211_SUBTYPE_TXPOW_SHIFT) + transmit_power);
        }
    }
    break;
    case IEEE80211_PARAM_CHANNEL_SWITCH_MODE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_CHANNEL_SWITCH_MODE, value);
        break;

    case IEEE80211_PARAM_ENABLE_ECSA_IE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ECSA_IE, value);
        break;

    case IEEE80211_PARAM_SAE_PWID:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_SAE_PWID, value);
        break;

    case IEEE80211_PARAM_OCE_TX_POWER:
        retv = ieee80211_config_oce_tx_power(vap, extra);
        break;

    case IEEE80211_PARAM_OCE_IP_SUBNET_ID:
        retv = ieee80211_config_oce_ipsubnet_id(vap, extra);
        break;

    case IEEE80211_PARAM_OCE_ADD_ESS_RPT:
        retv = ieee80211_config_oce_ess_rpt_param(vap, extra);
        break;

    case IEEE80211_PARAM_RSNX_OVERRIDE:
        retv = ieee80211_config_rsnx(vap, extra);
        break;

    case IEEE80211_PARAM_ECSA_OPCLASS:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ECSA_OPCLASS, value);
        break;

    case IEEE80211_PARAM_BACKHAUL:
        retv = wlan_set_param(vap, IEEE80211_FEATURE_BACKHAUL, value);
        break;

#if DYNAMIC_BEACON_SUPPORT
    case IEEE80211_PARAM_DBEACON_EN:
        if (value > 1  ||  value < 0) {
            qdf_print("Invalid value! value should be either 0 or 1 ");
            return -EINVAL;
        }
        if (IEEE80211_VAP_IS_HIDESSID_ENABLED(vap)) {
            if (vap->iv_dbeacon != value) {
                if (value == 0) { /*  value 0 */
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "Resume beacon \n");
                    qdf_spin_lock_bh(&vap->iv_dbeacon_lock);
                    OS_CANCEL_TIMER(&vap->iv_dbeacon_suspend_beacon);
                    if (ieee80211_mlme_beacon_suspend_state(vap)) {
                        ieee80211_mlme_set_beacon_suspend_state(vap, false);
                    }
                    vap->iv_dbeacon = value;
                    qdf_spin_unlock_bh(&vap->iv_dbeacon_lock);
                } else { /*  value 1 */
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "Suspend beacon \n");
                    qdf_spin_lock_bh(&vap->iv_dbeacon_lock);
                    /*
                     * When suspending the beacon, set iv_dbeacon and continue
                     * with NETREST. Wait until mlme_ext_vap_up() to send the
                     * suspend command because FW will always reset beaconing
                     * to TX_ENABLE after VDEV restart. iv_dbeacon will ensure
                     * the suspend path is activated in the mlme_ext_vap_up()
                     * path. The resume path will remain since the host suspend
                     * state will need to be reset before iv_dbeacon is
                     * disabled.
                     */
                    vap->iv_dbeacon = value;
                    qdf_spin_unlock_bh(&vap->iv_dbeacon_lock);
                }
                retv = ENETRESET;
                break;
            }
            retv = EOK;
        } else {
            qdf_print("%s:%d: Dynamic beacon not allowed for vap-%d(%s)) as hidden ssid not configured. ",
                    __func__,__LINE__,vap->iv_unit,vap->iv_netdev_name);
            qdf_print("Enable hidden ssid before enable dynamic beacon ");
            return -EINVAL;
        }
        break;

    case IEEE80211_PARAM_DBEACON_SNR_THR:
        if (value < 10  ||  value > 100) { /* min:10  max:100 */
            qdf_print("Invalid value %d. Valid values are between 10  to 100 ",value);
            return -EINVAL;
        }
        vap->iv_dbeacon_snr_thr = (int8_t)value;
        break;

    case IEEE80211_PARAM_DISABLE_LPI_ANT_OPTIMIZATION:
        if (ic->ic_vap_set_param) {
            retv = ic->ic_vap_set_param(vap, IEEE80211_DISABLE_LPI_ANT_OPTIMIZATION, value);
        }
    break;
    case IEEE80211_PARAM_DBEACON_TIMEOUT:
        if (value < 30  ||  value > 3600) { /* min:30 secs  max:1hour */
            qdf_print("Invalid value %d. Valid values are between 30secs to 3600secs ",value);
            return -EINVAL;
        }
        vap->iv_dbeacon_timeout = value;
        break;

#endif
    case IEEE80211_PARAM_CONFIG_TX_CAPTURE:
        if (wlan_set_param(vap, IEEE80211_CONFIG_TX_CAPTURE, value))
            return -EINVAL;
        break;
#ifdef WLAN_SUPPORT_FILS
    case IEEE80211_PARAM_ENABLE_FILS:
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
          qdf_err("FILS feature cannot be enabled for Non-Transmitting MBSS VAP!");
          return -EPERM;
        }
        if((!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) ||
               (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan) && IS_UP(dev))) {
            fils_en_period = ucfg_fd_get_enable_period(val[1], val[2]);
            retv = wlan_set_param(vap, IEEE80211_FEATURE_FILS, fils_en_period);
        }
        break;
#endif /* WLAN_SUPPORT_FILS */
    /* 11AX TODO ( Phase II) . Check ENETRESET
     * is really needed for all HE commands
     */
    case IEEE80211_PARAM_HE_EXTENDED_RANGE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_EXTENDED_RANGE, value);
        if (retv == 0)
            retv = ENETRESET;
    break;
    case IEEE80211_PARAM_HE_DCM:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_DCM, value);
        if (retv == 0) {
            retv = ENETRESET;
        }
    break;
    case IEEE80211_PARAM_HE_FRAGMENTATION:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_FRAGMENTATION, value);
        if (retv == 0) {
            retv = ENETRESET;
        }
    break;
    case IEEE80211_PARAM_HE_MU_EDCA:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_MU_EDCA, value);
        if(retv == 0) {
            retv = ENETRESET;
        }
    break;
    case IEEE80211_PARAM_HE_DYNAMIC_MU_EDCA:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_DYNAMIC_MU_EDCA, value);
    break;
    case IEEE80211_PARAM_HE_SU_BFEE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_SU_BFEE, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_HE_SU_BFER:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_SU_BFER, value);
        if ( retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_HE_MU_BFEE:
        if(vap->iv_opmode == IEEE80211_M_STA) {
            retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_MU_BFEE, value);
            if ( retv == EOK && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
                restart_vap = true;
            }
        } else {
            qdf_print("HE MU BFEE only supported in STA mode ");
            return -EINVAL;
        }
        break;

    case IEEE80211_PARAM_HE_MU_BFER:
        if(vap->iv_opmode == IEEE80211_M_HOSTAP) {
            retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_MU_BFER, value);
            if ( retv == EOK && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
                restart_vap = true;
            }
        } else {
            qdf_print("HE MU BFER only supported in AP mode ");
            return -EINVAL;
        }
        break;

    case IEEE80211_PARAM_HE_DL_MU_OFDMA:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_DL_MU_OFDMA, value);
        if ( retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_HE_DL_MU_OFDMA_BFER:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_DL_MU_OFDMA_BFER, value);
        if ( retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
        }
        break;

    case IEEE80211_PARAM_HE_UL_MU_OFDMA:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_MU_OFDMA, value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_HE_UL_MU_MIMO:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_UL_MU_MIMO, value);
        if ( retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
    break;

    case IEEE80211_PARAM_6G_HE_OP_MIN_RATE:
        ic->ic_is_heop_param_updated = true;
        retv = wlan_set_param(vap, IEEE80211_CONFIG_6G_HE_OP_MIN_RATE, value);
        ic->ic_is_heop_param_updated = false;
    break;

    case IEEE80211_PARAM_EXT_NSS_SUPPORT:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EXT_NSS_SUPPORT, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
#if QCN_IE
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_ENABLE:
        /* Enable broadcast probe response feature only if its in HOSTAP mode
         * and hidden SSID is not enabled for this vap.
         */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && !IEEE80211_VAP_IS_HIDESSID_ENABLED(vap)) {
            if (!!value != vap->iv_bpr_enable) {
                vap->iv_bpr_enable = !!value;
                wlan_set_param(vap, IEEE80211_CONFIG_BCAST_PROBE_RESPONSE, vap->iv_bpr_enable);
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "Set the bpr_enable to %d\n", vap->iv_bpr_enable);
            }
        } else {
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                     "Invalid. Allowed only in HOSTAP mode & non-hidden SSID mode\n");
        }
        break;
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_DELAY:
        if (value < 1 || value > 255) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "Invalid value! Broadcast response delay should be between 1 and 255 \n");
            return -EINVAL;
        }
        vap->iv_bpr_delay = value;
        break;
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_LATENCY_COMPENSATION:
        if (value < 1 || value > 10) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Invalid value!"
                "Broadcast response latency compensation should be between 1 and 10 \n");
            return -EINVAL;
        }
        ic->ic_bpr_latency_comp = value;
        break;
    case IEEE80211_PARAM_BEACON_LATENCY_COMPENSATION:
        if (value < 1 || value > 10) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Invalid value!"
                "Beacon latency compensation should be between 1 and 10 \n");
            return -EINVAL;
        }
        ic->ic_bcn_latency_comp = value;
        break;
#endif
#if ATH_ACL_SOFTBLOCKING
    case IEEE80211_PARAM_SOFTBLOCK_WAIT_TIME:
        if (value >= SOFTBLOCK_WAIT_TIME_MIN && value <= SOFTBLOCK_WAIT_TIME_MAX) {
            vap->iv_softblock_wait_time = value;
        }
        else {
            qdf_print("Allowed value between (%d, %d)",SOFTBLOCK_WAIT_TIME_MIN, SOFTBLOCK_WAIT_TIME_MAX);
            retv = -EINVAL;
        }
        break;

    case IEEE80211_PARAM_SOFTBLOCK_ALLOW_TIME:
        if (value >= SOFTBLOCK_ALLOW_TIME_MIN && value <= SOFTBLOCK_ALLOW_TIME_MAX) {
            vap->iv_softblock_allow_time = value;
        }
        else {
            qdf_print("Allowed value between (%d, %d)", SOFTBLOCK_ALLOW_TIME_MIN, SOFTBLOCK_ALLOW_TIME_MAX);
            retv = -EINVAL;
        }
        break;
#endif

    case IEEE80211_PARAM_QOS_ACTION_FRAME_CONFIG:
        ic->ic_qos_acfrm_config = value;
        break;

    case IEEE80211_PARAM_HE_LTF:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_LTF, value);
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
        }
        break;

    case IEEE80211_PARAM_HE_AR_GI_LTF:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_AR_GI_LTF, value);
        break;

    case IEEE80211_PARAM_HE_AR_LDPC:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_AR_LDPC, value);
        break;

    case IEEE80211_PARAM_HE_RTSTHRSHLD:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_RTSTHRSHLD, value);
        if (retv == 0) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;

    case IEEE80211_PARAM_DFS_INFO_NOTIFY_APP:
        ic->ic_dfs_info_notify_channel_available = value;
        break;
    case IEEE80211_PARAM_RSN_OVERRIDE:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_RSN_OVERRIDE, value);
        break;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    case IEEE80211_PARAM_NSSOL_VAP_INSPECT_MODE:
        if (osifp->nss_wifiol_ctx && ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_vdev_set_inspection_mode(osifp, (uint32_t)value);
        }
        break;
#endif

    case IEEE80211_PARAM_DISABLE_CABQ :
        retv = wlan_set_param(vap, IEEE80211_FEATURE_DISABLE_CABQ, value);
        break;

    case IEEE80211_PARAM_CSL_SUPPORT:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_CSL_SUPPORT, value);
        break;

    case IEEE80211_PARAM_TIMEOUTIE:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_TIMEOUTIE, value);
        break;

    case IEEE80211_PARAM_PMF_ASSOC:
        retv = wlan_set_param(vap, IEEE80211_SUPPORT_PMF_ASSOC, value);
        break;

    case IEEE80211_PARAM_HE_TX_MCSMAP:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_TX_MCSMAP, value);
        if (retv == 0)
            retv = ENETRESET;
        break;

    case IEEE80211_PARAM_HE_RX_MCSMAP:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_RX_MCSMAP, value);
        if (retv == 0)
            retv = ENETRESET;
        break;

#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_EHT_TX_MCSMAP:
        /* iv_eht_tx_mcsnssmap has storage for 3 indexes 20only/le80, 160 and 320 */
        vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0] = val[1];
        vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX1] = val[2];
        vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX2] = val[3];
        retv = ENETRESET;
        break;

    case IEEE80211_PARAM_EHT_RX_MCSMAP:
        /* iv_eht_rx_mcsnssmap has storage for 3 indexes 20only/le80, 160 and 320 */
        vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0] = val[1];
        vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX1] = val[2];
        vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX2] = val[3];
        retv = ENETRESET;
        break;
#endif /* WLAN_FEATURE_11BE */

    case IEEE80211_PARAM_CONFIG_M_COPY:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_M_COPY, value);
        break;

    case IEEE80211_PARAM_NSSOL_VAP_READ_RXPREHDR:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_READ_RXPREHDR, value);
        break;

    case IEEE80211_PARAM_CONFIG_CAPTURE_LATENCY_ENABLE:
        wlan_set_param(vap, IEEE80211_CONFIG_CAPTURE_LATENCY_ENABLE, value);
        break;

    case IEEE80211_PARAM_BA_BUFFER_SIZE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_BA_BUFFER_SIZE, value);
        break;

    case IEEE80211_PARAM_HE_SOUNDING_MODE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_SOUNDING_MODE, value);
        break;

    case IEEE80211_PARAM_HE_HT_CTRL:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_HT_CTRL, value);
        if (retv == 0) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_WIFI_DOWN_IND:
         retv = wlan_set_param(vap, IEEE80211_CONFIG_WIFI_DOWN_IND, value);
         break;

    case IEEE80211_PARAM_FT_ENABLE:
        if (!ic->ic_cfg80211_config) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                "FT supported only with cfg80211\n");
            return -EINVAL;
        }

        if(vap->iv_opmode == IEEE80211_M_STA) {
            if (value > 1  ||  value < 0) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                    "Invalid value! value should be either 0 or 1\n");
                return -EINVAL;
            }
            retv = wlan_set_param(vap, IEEE80211_CONFIG_FT_ENABLE, value);
            if (retv == 0) {
                retv = ENETRESET;
            }
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                "Valid only in STA mode\n");
            return -EINVAL;
        }
        break;
#if WLAN_SER_DEBUG
    case IEEE80211_PARAM_WLAN_SER_HISTORY:
        wlan_ser_print_history(vap->vdev_obj, value, val[2]);
        break;
#endif
    case IEEE80211_PARAM_WLAN_PRINT_RL:
        qdf_rl_print_count_set(value);
        qdf_rl_print_time_set(val[2]);
    case IEEE80211_PARAM_WLAN_SCHED_TIMEOUT:
        if (value < (10 * 1000)) {
           qdf_info("Please enter a scheduler timeout of more than 10 seconds");
        } else {
            scheduler_set_watchdog_timeout(value);
        }
        break;
    case IEEE80211_PARAM_BCN_STATS_RESET:
        if(vap->reset_bcn_stats)
            vap->reset_bcn_stats(vap);
        break;
#if WLAN_SER_UTF
    case IEEE80211_PARAM_WLAN_SER_TEST:
        wlan_ser_utf_main(vap->vdev_obj, value, val[2]);
        break;
#endif
    case IEEE80211_PARAM_RAWMODE_OPEN_WAR:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_RAWMODE_OPEN_WAR, !!value);
        break;
    case IEEE80211_PARAM_EXTERNAL_AUTH_STATUS:
        qdf_timer_sync_cancel(&vap->iv_sta_external_auth_timer);
        vap->iv_mlme_priv->im_request_type = 0;
        osifp->app_filter &= ~(IEEE80211_FILTER_TYPE_AUTH);
        IEEE80211_DELIVER_EVENT_MLME_AUTH_COMPLETE(vap, NULL, value);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH, "external auth status %d",value);
        break;
    case IEEE80211_PARAM_SAE_AUTH_ATTEMPTS:
        vap->iv_sae_max_auth_attempts = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_DPP_VAP_MODE:
        vap->iv_dpp_vap_mode = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_HE_BSR_SUPPORT:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_BSR_SUPPORT, value);
        if((retv == 0) && (vap->iv_novap_reset == 0)){
            retv = ENETRESET;
        }
    break;
    case IEEE80211_PARAM_UNIFORM_RSSI:
        ic->ic_uniform_rssi = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_CSA_INTEROP_PHY:
        vap->iv_csa_interop_phy = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_CSA_INTEROP_BSS:
        vap->iv_csa_interop_bss = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_CSA_INTEROP_AUTH:
        vap->iv_csa_interop_auth = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_DEC_BCN_LOSS:
        vap->iv_dec_bcn_loss = !!value;
        retv = 0;
        break;
    case IEEE80211_PARAM_ENABLE_MULTI_GROUP_KEY:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ENABLE_MULTI_GROUP_KEY, value);
#if QCA_PPE_VP
        if (value != 0)
            osif_pltfrm_update_ppe_vp(osifp);
#endif
        retv = 0;
        break;
    case IEEE80211_PARAM_MAX_GROUP_KEYS:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_MAX_GROUP_KEYS, value);
        retv = 0;
        break;
    case IEEE80211_PARAM_MAX_MTU_SIZE:
       if (value > IEEE80211_MTU_MIN && value < IEEE80211_MTU_MAX) {
           retv = wlan_set_param(vap, IEEE80211_CONFIG_MAX_MTU_SIZE, value);
       } else {
           qdf_err("Invalid value for MTU size: %d", value);
           retv = -EINVAL;
       }

       if(retv == 0)
           retv = ENETRESET;
       break;
    case IEEE80211_PARAM_HE_6GHZ_BCAST_PROB_RSP:
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            qdf_err("Bcast Probe resp setting is valid only for Tx vap");
            return -EINVAL;
        }
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            prb_rsp_en_period = ieee80211_get_prbrsp_en_period(val[1], val[2]);
            retv = wlan_set_param(vap, IEEE80211_CONFIG_6GHZ_BCAST_PROB_RSP, prb_rsp_en_period);
        } else {
            qdf_err("20TU Bcast Probe response only supported in AP mode ");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_VHT_MCS_12_13_SUPP:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_HE_MCS_12_13_SUPP, value);
        break;
    case IEEE80211_PARAM_SET_STATS_UPDATE_PERIOD:
        retv = wlan_set_param(vap, IEEE80211_STATS_UPDATE_PERIOD, value);
        break;
    case IEEE80211_PARAM_SEND_PROBE_REQ:
        ieee80211_ucfg_send_probereq(vap, value);
        break;
    case IEEE80211_PARAM_ENABLE_MSCS:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ENABLE_MSCS, value);
        break;
    case IEEE80211_PARAM_MCAST_STEER:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_MCAST_STEER, value);
        break;
#ifdef WLAN_SUPPORT_SCS
    case IEEE80211_PARAM_ENABLE_SCS:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_ENABLE_SCS, value);
        break;
#endif
#if WLAN_SUPPORT_DAP
    case IEEE80211_PARAM_DSCP_ACTION_POLICY:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_DSCP_ACTION_POLICY, value);
        break;
#endif

    case IEEE80211_PARAM_OCE_VERSION_OVERRIDE:
        retv = wlan_set_param(vap, IEEE80211_OCE_VERSION_OVERRIDE, value);
        break;
    case IEEE80211_PARAM_6G_SECURITY_COMP:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_6G_SECURITY_COMP, value);
        break;
    case IEEE80211_PARAM_6G_KEYMGMT_MASK:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_6G_KEYMGMT_MASK, value);
        break;
    case IEEE80211_PARAM_MBSS_TXVDEV:
        if (!is_mbssid_enabled) {
            if (ic->ic_wideband_capable) {
                /*
                 * If MBSSID is disabled for wideband radio, set a target
                 * Tx VAP and use it if the wideband radio successfully
                 * enables MBSSID/EMA mode of operation
                 */
                if (value) {
                    qdf_err("Setting Tx-VAP for later EMA use");
                    ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].target_transmit_vap = vap;
                } else {
                    ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].target_transmit_vap = NULL;
                }
            } else {
                qdf_err("MBSSID is not enabled");
                return -EINVAL;
            }
        } else {
#if UMAC_SUPPORT_NAWDS
            if (vap->iv_nawds.mode != IEEE80211_NAWDS_DISABLED) {
                qdf_err("Assigning nawds vap as Tx vap is not supported!");
                return -EINVAL;
            }
#endif
            if (value) {
                retv = ieee80211_ucfg_set_txvap(vap);
                if (retv)
                    return -EINVAL;
            } else {
                retv = ieee80211_ucfg_reset_txvap(vap, 0);
                if (retv)
                    return -EINVAL;
            }
        }
        break;
    case IEEE80211_PARAM_HLOS_TID_OVERRIDE:
         retv = ieee80211_ucfg_set_hlos_tid_override(osifp, value, false);
         break;
    case IEEE80211_PARAM_HE_ER_SU_DISABLE:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_ER_SU_DISABLE, value);
        break;
    case IEEE80211_PARAM_HE_1024QAM_LT242RU_RX_ENABLE:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_1024QAM_LT242RU_RX_ENABLE, value);
        break;
    case IEEE80211_PARAM_HE_UL_MU_DATA_DIS_RX_SUPP:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_UL_MU_DATA_DIS_RX_SUPP, value);
        break;
    case IEEE80211_PARAM_HE_FULL_BW_UL_MUMIMO:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_FULL_BW_UL_MUMIMO, value);
        break;
    case IEEE80211_PARAM_HE_DCM_MAX_CONSTELLATION_RX:
        if (!ieee80211vap_ishemode(vap)) {
            qdf_err("Invalid setting for current mode");
            return -EINVAL;
        }
        retv = wlan_set_param(vap,
            IEEE80211_CONFIG_HE_DCM_MAX_CONSTELLATION_RX, value);
        break;
#if WLAN_OBJMGR_REF_ID_TRACE
    case IEEE80211_PARAM_VDEV_REF_LEAK_TEST:
        vap->iv_ref_leak_test_flag = !!value;
        break;
#endif  /* WLAN_OBJMGR_REF_ID_TRACE */
    case IEEE80211_PARAM_DISABLE_INACT_PROBING:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_DISABLE_INACT_PROBING, value);
        break;
    case IEEE80211_PARAM_RTS:
        if (IEEE80211_RTS_MIN <= value && value <= IEEE80211_RTS_MAX) {
            int curval = wlan_get_param(vap, IEEE80211_RTS_THRESHOLD);

            if (value != curval) {
                retv = wlan_set_param(vap, IEEE80211_RTS_THRESHOLD, value);
                if (retv == 0) {
                    retv = ENETRESET;
                    restart_vap = true;
                }
            }
        } else {
            qdf_err("Invalid args!!");
            retv = -EINVAL;
        }
        break;
    case IEEE80211_PARAM_SM_GAP_PS_ENABLE:
        if (vap->iv_sm_gap_ps == value) {
            qdf_warn("Value already set to %d\n", value);
            break;
        }
        /* Allow the smps feature only for either RootAP or Repeater STA */
        if ((ic_is_sta_vap(ic) && (vap->iv_opmode == IEEE80211_M_HOSTAP)) ||
            (!(vap->iv_opmode == IEEE80211_M_STA) &&
            !(vap->iv_opmode == IEEE80211_M_HOSTAP))) {
            qdf_err("Invalid setting for current mode");
            break;
        }
        vap->iv_sm_gap_ps = !!value;
        vap->iv_static_mimo_ps = vap->iv_sm_gap_ps;
        if (ic_is_sta_vap(ic) && !vap->iv_sm_gap_ps)
            wlan_green_ap_stop(pdev);
        if (vap->iv_pwrsave_smps)
            ieee80211_pwrsave_smps_set_timer(vap->iv_pwrsave_smps);
        retv = 0;
        break;
#ifdef WLAN_SUPPORT_MESH_LATENCY
    case IEEE80211_PARAM_VAP_MESH_TID_CONFIG:
         retv = ieee80211_ucfg_set_vap_mesh_tid(osifp, val[1]);
        break;
    case IEEE80211_PARAM_VAP_MESH_LATENCY_CONFIG:
    {
         uint8_t latency_tid = 0;
         uint8_t dl_ul_latency_enable = 0;
         uint32_t service_interval = 0;
         uint32_t burst_size = 0;

         service_interval = val[1];
         burst_size = val[2];
         /*
          * Check if per peer latency tid config is enabled
          */
         if (!ieee80211_ucfg_get_peer_tid_latency_enable(osifp)) {
             ieee80211_ucfg_get_vap_mesh_tid(osifp, &latency_tid, &dl_ul_latency_enable);
             retv = ic->ic_vap_config_tid_latency_param(vap,
                    service_interval, burst_size, latency_tid, dl_ul_latency_enable);
             if (retv == EOK) {
                 retv = ENETRESET;
                 restart_vap = true;
             }
         }
    }
         break;
    case IEEE80211_PARAM_PEER_TID_LATENCY_ENABLE:
         retv = ieee80211_ucfg_set_peer_tid_latency_enable(osifp, value);
         break;
#endif
    case IEEE80211_PARAM_AP_MAX_AUTH_FAIL:
         retv = wlan_set_param(vap, IEEE80211_CONFIG_AP_MAX_AUTH_FAIL, value);
         break;
    case IEEE80211_PARAM_VAP_PROFILE_CONFIG:
        retv = ieee80211_update_vap_resource_profile(vap, value, val[2]);
        break;
   case IEEE80211_PARAM_STEALTHDOWN:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_STEALTHDOWN, value);
        break;
    case IEEE80211_PARAM_SPL_VAP_SCAN:
        if (IEEE80211_IS_SPECIAL_VAP(vap) && vap->iv_spl_vap_scan != !!value) {
            vap->iv_spl_vap_scan = !!value;
            qdf_warn("%s special vap scan on cur_chan",
                     vap->iv_spl_vap_scan ? "Enable" : "Disable");
            retv = EOK;
        } else if (!IEEE80211_IS_SPECIAL_VAP(vap)) {
            qdf_err("This feature is applicable only for special VAP (ap_monitor)");
            retv = -EINVAL;
        } else {
            qdf_err("special vap scan on cur_chan already %s",
                    vap->iv_spl_vap_scan ? "enabled" : "disabled");
            retv = EOK;
        }
        break;
    case IEEE80211_PARAM_MBO_BSTM_REQ:
	vap->iv_mbo_bstm = !!value;
	retv = 0;
	break;
    case IEEE80211_PARAM_DISC_FRM_CLEAR_USR_OVERRIDE:
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) ||
            (!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan))) {
            qdf_err("Valid only for Tx vap and 6ghz radio");
            return -EINVAL;
        }

        if (!ieee80211_is_fils_20tu_allowed_vap(vap)) {
            qdf_err("Not supported on this vap");
            return -EINVAL;
        }

        if (value < 0 || value > 1) {
            retv = -EINVAL;
        } else {
           /* Cmd from user is to clear user override.
            * The override flag is active when user enables/disables
            * FILS/UBPRBR via cmd. This sets discfrm_6g_override flag.
            * To clear this and fall back to driver logic, we have to clear
            * this flag. This is done by issuing,
            * discfrm_6g_clear_usr_override = 1. This clears the
            * usr override of driver logic set by enabling
            * fils/20tu prb Tx.
            */
            if (!ic->ic_mbss_ctx) {
                qdf_err("ic mbss context is NULL");
                retv = -EINVAL;
            } else {
                ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].discfrm_6g_override = !value;
                if (!ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].discfrm_6g_override) {
                    enum wlan_frm_update_reason update_reason;

                    if (0 == wlan_lower_band_ap_cnt_get()) {
                        update_reason = ENABLE_FILS_UBPRBR_6GHZ;
                        wlan_tmpl_update_6ghz_frm(ic, &update_reason);
                    }
                    else {
                        update_reason = DISABLE_FILS_UBPRBR_6GHZ;
                        wlan_tmpl_update_6ghz_frm(ic, &update_reason);
                    }
                }
            } /* ic_mbss_ctx */
        }
        break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_EPCS_PRIORITY_ACCESS:
         retv = wlan_set_param(vap, IEEE80211_EHT_NSEP_PRIORITY_ACCESS, value);
         break;
    case IEEE80211_PARAM_OM_CONTROL:
         retv = wlan_set_param(vap, IEEE80211_EHT_OM_CONTROL, value);
         break;
    case IEEE80211_PARAM_TRIGGERED_TXOP_SHARING:
         retv = wlan_set_param(vap, IEEE80211_EHT_TRIGGERED_TXOP_SHARING, value);
         break;
    case IEEE80211_PARAM_EHT_NDP_4X_EHT_LTF_AND_320NSGI:
         if (!ieee80211vap_isehtmode(vap)) {
             qdf_err("Invalid setting for current mode");
             return -EINVAL;
         }

         retv = wlan_set_param(vap, IEEE80211_EHT_NDP_4X_EHT_LTF_AND_320NSGI, value);
         if ((retv == 0) && (vap->iv_novap_reset == 0)) {
             retv = ENETRESET;
             restart_vap = true;
         }
         break;
    case IEEE80211_PARAM_EHT_SU_BFMR:
         retv = wlan_set_param(vap, IEEE80211_EHT_SU_BFMR, value);
         if ((retv == 0) && (vap->iv_novap_reset == 0)) {
             retv = ENETRESET;
             restart_vap = true;
         }
         break;
    case IEEE80211_PARAM_EHT_SU_BFME:
         if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
             qdf_err("configuration not allowed for non-ap sta");
             return -EINVAL;
         }
         retv = wlan_set_param(vap, IEEE80211_EHT_SU_BFME, value);
         if ((retv == 0) && (vap->iv_novap_reset == 0)) {
             retv = ENETRESET;
             restart_vap = true;
         }
         break;
    case IEEE80211_PARAM_EHT_BFME_SS:
         if (!ieee80211vap_isehtmode(vap)) {
             qdf_err("Invalid setting for current mode");
             return -EINVAL;
         }

         if (!vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfme) {
             qdf_err("configuration allowed only if EHT BFME is enabled");
         }

         if ((val[1] < 3) || (val[2] < 3) || (val[3] < 3)) {
             qdf_err("min value should be 3");
             return -EINVAL;
         }
         wlan_set_param(vap, IEEE80211_EHT_BFME_SS_LT_80MHZ, val[1]);
         wlan_set_param(vap, IEEE80211_EHT_BFME_SS_160MHZ, val[2]);
         wlan_set_param(vap, IEEE80211_EHT_BFME_SS_320MHZ, val[3]);
         break;
    case IEEE80211_PARAM_EHT_NUM_SD:
         if (!ieee80211vap_isehtmode(vap)) {
             qdf_err("Invalid setting for current mode");
             return -EINVAL;
         }

         if (!vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfmr) {
             qdf_err("configuration allowed only if EHT BFMR is enabled");
         }

         retv = wlan_set_param(vap, IEEE80211_EHT_NUM_SOUND_LT_80MHZ, val[1]);
         if (!retv)
             retv  = wlan_set_param(vap, IEEE80211_EHT_NUM_SOUND_160MHZ, val[2]);
         if (!retv)
             retv = wlan_set_param(vap, IEEE80211_EHT_NUM_SOUND_320MHZ, val[3]);

         if ((retv == 0) && (vap->iv_novap_reset == 0)) {
             retv = ENETRESET;
             restart_vap = true;
         }
         break;

    case IEEE80211_PARAM_EHT_4X_EHT_LTF_AND_800NS_GI:
         if (!ieee80211vap_isehtmode(vap)) {
             qdf_err("Invalid setting for current mode");
             return -EINVAL;
         }

         retv = wlan_set_param(vap, IEEE80211_EHT_4X_EHT_MU_LTF_AND_800NS_GI, value);
         if ((retv == 0) && (vap->iv_novap_reset == 0)) {
             retv = ENETRESET;
             restart_vap = true;
         }
         break;
    case IEEE80211_PARAM_EHT_MAX_NC:
         if(!ieee80211vap_isehtmode(vap)) {
             qdf_err("EHT Max NC support is not allowed in current mode.");
             return -EPERM;
         }

         if (!vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfme) {
             qdf_err("EHT MAX NC field is reserved if EHT SU BFEE"
                     " role is not supported");
             return -EPERM;
         }

         if ((value < 0) || (value > 7)) {
             qdf_err("Invalid value");
             return -EINVAL;
         }

         retv = wlan_set_param(vap, IEEE80211_EHT_MAX_NC, value);
         break;
    case IEEE80211_PARAM_EHT_RX_1024_AND_4096_QAM_LS_242_TONE_RU:
         if(!ieee80211vap_isehtmode(vap)) {
             qdf_err("EHT RX 1024 AND 4096 QAM LS 242 TONE RU support is not allowed in current mode.");
             return -EPERM;
         }

         retv = wlan_set_param(vap, IEEE80211_EHT_RX_1024_AND_4096_QAM_LT_242_TONE_RU, value);
         if ((retv == 0) && (vap->iv_novap_reset == 0)) {
             retv = ENETRESET;
             restart_vap = true;
         }
         break;
    case IEEE80211_PARAM_EHT_MU_BFMR:
         {
             bool eht_mu_bfmr_lte_80mhz;
             bool eht_mu_bfmr_160mhz;
             bool eht_mu_bfmr_320mhz;
             if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
                 qdf_err("configuration not allowed for non-ap sta");
                 return -EINVAL;
             }

             eht_mu_bfmr_lte_80mhz = EHT_MU_BFMR_GET_LT_80MHZ_SUPPORT(value);
             eht_mu_bfmr_160mhz = EHT_MU_BFMR_GET_160MHZ_SUPPORT(value);
             eht_mu_bfmr_320mhz = EHT_MU_BFMR_GET_320MHZ_SUPPORT(value);

             if ((eht_mu_bfmr_160mhz > eht_mu_bfmr_lte_80mhz) || (eht_mu_bfmr_320mhz > eht_mu_bfmr_160mhz)) {
                 qdf_err("Invalid configuration value for different bandwidths 0x%x", value);
                 return -EINVAL;
             }

             retv = wlan_set_param(vap, IEEE80211_EHT_MU_BFMR_LT_80MHZ, eht_mu_bfmr_lte_80mhz);
             if ((retv == 0) && (vap->iv_novap_reset == 0)) {
                 retv = ENETRESET;
                 restart_vap = true;
             }
             wlan_set_param(vap, IEEE80211_EHT_MU_BFMR_160MHZ, eht_mu_bfmr_160mhz);
             wlan_set_param(vap, IEEE80211_EHT_MU_BFMR_320MHZ, eht_mu_bfmr_320mhz);
         }
         break;
    case IEEE80211_PARAM_EHT_MU_BFEE:
        if (vap->iv_opmode == IEEE80211_M_STA) {
            retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_MU_BFEE, value);
            if (retv == EOK && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
                restart_vap = true;
            }
        } else {
            qdf_info("EHT MU BFEE only supported in STA mode ");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_EHT_DL_MU_OFDMA:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_DL_OFDMA, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_EHT_UL_MU_OFDMA:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_OFDMA, value);
        if (retv == EOK) {
            retv = ENETRESET;
            restart_vap = true;
        }
        break;
    case IEEE80211_PARAM_EHT_MU_MIMO:
        {
            bool eht_mu_mimo_lte_80mhz;
            bool eht_mu_mimo_160mhz;
            bool eht_mu_mimo_320mhz;

            eht_mu_mimo_lte_80mhz = EHT_MU_MIMO_GET_LT_80MHZ_SUPPORT(value);
            eht_mu_mimo_160mhz = EHT_MU_MIMO_GET_160MHZ_SUPPORT(value);
            eht_mu_mimo_320mhz = EHT_MU_MIMO_GET_320MHZ_SUPPORT(value);

            if ((eht_mu_mimo_160mhz > eht_mu_mimo_lte_80mhz) || (eht_mu_mimo_320mhz > eht_mu_mimo_160mhz)) {
                qdf_err("Invalid configuration value for different bandwidths 0x%x", value);
                return -EINVAL;
            }

            retv = wlan_set_param(vap, IEEE80211_EHT_NON_OFDMA_UL_MU_MIMO_LT_80MHZ, eht_mu_mimo_lte_80mhz);
            if (retv == EOK && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
                restart_vap = true;
            }
            wlan_set_param(vap, IEEE80211_EHT_NON_OFDMA_UL_MU_MIMO_160MHZ, eht_mu_mimo_160mhz);
            wlan_set_param(vap, IEEE80211_EHT_NON_OFDMA_UL_MU_MIMO_320MHZ, eht_mu_mimo_320mhz);
        }
        break;
    case IEEE80211_PARAM_EHT_DL_OFDMA_TXBF:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_DL_OFDMA_TXBF, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_EHT_DL_OFDMA_MUMIMO:
        if (vap->iv_opmode == IEEE80211_M_STA) {
            retv = wlan_set_param(vap, IEEE80211_EHT_PART_BW_DL_MU_MIMO, value);
            if (retv == EOK && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
            }
        } else {
            qdf_info("EHT DL OFDMA MU-MIMO only supported in STA mode.");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_EHT_UL_OFDMA_MUMIMO:
        if (vap->iv_opmode == IEEE80211_M_STA) {
            retv = wlan_set_param(vap, IEEE80211_EHT_PARTIAL_BW_UL_MU, value);
            if (retv == EOK && (vap->iv_novap_reset == 0)) {
                retv = ENETRESET;
            }
        } else {
            qdf_info("EHT UL OFDMA MU-MIMO only supported in STA mode.");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_EHT_UL_NSS:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL NSS setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_NSS, value);
        break;
    case IEEE80211_PARAM_EHT_UL_PPDU_BW:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL PPDU BW setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_PPDU_BW, value);
        break;
    case IEEE80211_PARAM_EHT_UL_SHORTGI:
        if(!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL Shortgi setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_SHORTGI, value);
        break;
    case IEEE80211_PARAM_EHT_UL_LDPC:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL LDPC setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_LDPC, value);
        break;
    case IEEE80211_PARAM_EHT_UL_STBC:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL STBC setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_STBC, value);
        break;
    case IEEE80211_PARAM_EHT_UL_FIXED_RATE:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL MCS setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_FIXED_RATE, value);
        break;
    case IEEE80211_PARAM_EHT_UL_LTF:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT UL LTF setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_UL_LTF, value);
        break;
    case IEEE80211_PARAM_EHT_LTF:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_EHT_LTF, value);
        /* if the novap reset is set for debugging
         * purpose we are not resetting the VAP
         */
        if ((retv == 0) && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_EHT_MAX_SUPP_LTF:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT MAX LTF setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_EHT_MAX_NUM_EHT_LTF, value);
        break;
    case IEEE80211_PARAM_EHT_ML_PROBE_REQ:
        vap->ml_preq_enable = value & ML_PREQ_EN_MASK;
        vap->ml_preq_mldid = (value & ML_PREQ_MLDID_VALUE_MASK) >> 8;
        vap->ml_preq_linkids = (value & ML_PREQ_LINKIDS_VALUE_MASK) >> 16;
        retv = EOK;
        break;
    case IEEE80211_PARAM_EHT_SUP_MCS15:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT MCS15 setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_EHT_SUP_MCS15, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_EHT_MCS14_DUP_IN_6GHZ:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT MCS14 duplicate in 6GHz setting is not allowed in current mode.");
            return -EPERM;
        }
        retv = wlan_set_param(vap, IEEE80211_EHT_EHT_DUP_IN_6GHZ, value);
        if (retv == EOK && (vap->iv_novap_reset == 0)) {
            retv = ENETRESET;
        }
        break;
    case IEEE80211_PARAM_EHT_CONFIG_CCFS0:
        if (!ieee80211vap_isehtmode(vap)) {
            qdf_err("EHT CONFIG CCFS0 is not allowed in current mode.");
            return -EPERM;
        }
        ic->eht_config_ccfs0 = value;
        break;
#endif
    case IEEE80211_PARAM_DROP_3ADDR_MCAST:
        if (value < 0 || value > 1) {
            qdf_err("Invalid value. Set/reset 1/0");
            return -EINVAL;
        }
        buf.cdp_drop_3addr_mcast = value;
        cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc), wlan_vdev_get_id(vap->vdev_obj),
                CDP_DROP_3ADDR_MCAST, buf);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (ic->nss_vops && ic->nss_vops->ic_osif_nss_vdev_set_cfg) {
            ic->nss_vops->ic_osif_nss_vdev_set_cfg((osif_dev *)vap->iv_ifp, OSIF_NSS_WIFI_VDEV_DROP_3ADDR_MCAST);
        }
#endif
        break;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    case IEEE80211_PARAM_DROP_TX_MCAST:
        if (value < 0 || value > 1) {
            qdf_err("Invalid value. Set/reset 1/0");
            return -EINVAL;
        }
        buf.cdp_drop_tx_mcast = value;
        cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc), wlan_vdev_get_id(vap->vdev_obj),
                CDP_DROP_TX_MCAST, buf);
        break;
    case IEEE80211_PARAM_WDS_EXT_AP_BRIDGE:
        if (value < 0 || value > 1) {
            qdf_err("Invalid value. Set/reset 1/0");
            return -EINVAL;
        }
        buf.cdp_vdev_paran_wds_ext_ap_bridge = value;
        cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc), wlan_vdev_get_id(vap->vdev_obj),
                CDP_WDS_EXT_AP_BRIDGE, buf);
        break;
#endif
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
	case IEEE80211_PARAM_TELEMETRY_AGENT_DEBUG_LEVEL:
		telemetry_agent_set_param(param, value);
	break;
#endif
    case IEEE80211_PARAM_EMLMR_CONFIG:
        qdf_nofl_err("EMLMR set operation not permitted");
        retv =-EOPNOTSUPP;
        break;
    case IEEE80211_PARAM_EMLSR_CONFIG:
        qdf_nofl_err("EMLSR set operation not permitted");
        retv =-EOPNOTSUPP;
        break;
    case IEEE80211_PARAM_STR_CONFIG:
        qdf_nofl_err("STR set-operation not permitted");
        retv =-EOPNOTSUPP;
        break;
    case IEEE80211_PARAM_NSTR_CONFIG:
        qdf_nofl_err("NSTR set-operation not permitted");
        retv =-EOPNOTSUPP;
        break;
#ifdef WLAN_FEATURE_11BE_MLO
    case IEEE80211_PARAM_WLAN_PEER_MESH_OVERRIDE:
        {
            if (value < 0 || value >= WLAN_PEER_MESH_NODE_OVERRIDE_END) {
                qdf_err("Invalid value. Expected value{0 - 2}");
                return -EINVAL;
            }
            vap->wlan_peer_mesh_override_flag = value;
        }
        break;
    case IEEE80211_PARAM_MLO_MAX_RECOM_ACTIVE_LINKS:
        {
            struct wlan_objmgr_vdev *vdev;
            struct wlan_mlo_dev_context *ml_dev;
            uint16_t mld_vdev_cnt = 0;
            struct wlan_objmgr_vdev *mlo_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];

            vdev = vap->vdev_obj;
            if (!vdev) {
                    qdf_err("VDEV is null");
                    return -EINVAL;
            }

            ml_dev = vdev->mlo_dev_ctx;

            if (!ml_dev) {
                    qdf_err("ML_DEV is null");
                    return -EINVAL;
            }
            if (!ml_dev->mlo_extmld_cap_advertisement) {
                qdf_err("RMSL advertisement is disabled");
                return -EINVAL;
            }

            ieee80211_mlo_ap_get_beaconing_vdev_list(vdev, &mld_vdev_cnt, mlo_vdev_list);
            if ((ieee80211_mlo_release_vdev_list_refs(mlo_vdev_list,
                                                      QDF_ARRAY_SIZE(mlo_vdev_list),
                                                      mld_vdev_cnt, 0)) != QDF_STATUS_SUCCESS) {
                 mlo_err("Error while trying to release VDEV references - investigate.");
                 return -EINVAL;
            }

            qdf_debug("mld_vdev_cnt: %d", mld_vdev_cnt);

            if (!ml_dev->mlo_extmld_cap_advertisement) {
                qdf_err("RMSL advertisement is disabled");
                return -EINVAL;
            }

            /* Check if passed user value is valid */
            if (value < 0 || value == 1 || value > mld_vdev_cnt) {
                qdf_err("Invalid value. Expected value ( > 1 or < Max Beaconing VDEV's in the MLD)");
                return -EINVAL;
            }

            retv = wlan_set_param(vap,
                                  IEEE80211_CONFIG_MLO_MAX_RECOM_ACTIVE_LINKS,
                                  value);
        }
        break;
        case IEEE80211_PARAM_MLO_EXTMLDCAPOP_FLAG:
        {
            struct wlan_objmgr_vdev *vdev;
            struct wlan_mlo_dev_context *ml_dev;

            if (value < WLAN_MLO_EXTMLDCAPOP_DISABLE ||
                value > WLAN_MLO_EXTMLDCAPOP_ENABLE) {
                qdf_err("Invalid value. Expected value{0 or 1}");
                return -EINVAL;
            }

            vdev = vap->vdev_obj;

            if (!vdev) {
                qdf_err("VDEV is null");
                return -EINVAL;
            }

            ml_dev = vdev->mlo_dev_ctx;

            if (!ml_dev) {
                qdf_err("ML_DEV is null");
                return -EINVAL;
            }

            retv = wlan_set_param(vap,
                                  IEEE80211_CONFIG_MLO_EXTMLDCAP_FLAG,
                                  value);

        }
        break;
        case IEEE80211_PARAM_MLO_LINK_REJ_FLAG:
        {
            struct wlan_objmgr_vdev *vdev;
            int link_rej = 0;

            if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
                mlo_err("configuration not allowed for non-ap sta ML");
                return -EINVAL;
            }

            if (value < MLO_LINK_REJ_DISABLED ||
                value > MLO_LINK_REJ_THROUGHPUT) {
                mlo_err("Invalid value. Expected value{%u to %u}",
                        MLO_LINK_REJ_DISABLED, MLO_LINK_REJ_THROUGHPUT);
                return -EINVAL;
            }

            vdev = vap->vdev_obj;
            if (!vdev) {
                mlo_err("VDEV is null");
                return -EINVAL;
            }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            mlo_info("link rejection is not supported");
            return -ENOTSUPP;
#endif
            link_rej = wlan_mlme_mlo_link_rej_init(vdev, value);
            mlo_info("ML: %s link rejection set to %d",
                     ether_sprintf(vdev->mlo_dev_ctx->mld_addr.bytes), link_rej);
        }
        break;
        case IEEE80211_PARAM_MLO_LINK_REJ_TEST:
        {
            if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
                mlo_err("configuration not allowed for non-ap sta ML");
                return -EINVAL;
            }

            vap->lre_test_num_assoc_link = val[1];
            vap->lre_test_num_peers = val[2];

            mlo_info("LRE num_assoc_link:%u, link_rejected %s:%u",
                     vap->lre_test_num_assoc_link, vap->iv_netdev_name,
                     vap->lre_test_num_peers);
        }
        break;
#endif
#ifdef ASSOC_REJECT_SUPPORT_ENABLED
    case IEEE80211_PARAM_ASSOC_REJECT:
        retv = ieee80211_set_assoc_reject(vap,extra);
        break;
#endif
    case IEEE80211_PARAM_GTX_ENABLE:
        retv = wlan_set_param(vap, IEEE80211_CONFIG_GTX_ENABLE, value);
        break;
    case IEEE80211_PARAM_HWCTS2SELF_OFDMA:
        {
            if (!vdev) {
                qdf_err("VDEV is null");
                return -EINVAL;
            }
            if (value != 0 || value != 1) {
                qdf_err("Invalid Value %d", value);
                return -EINVAL;
            }
            if (!ic->ic_send_hwcts2self_command) {
                qdf_err("ic_send_hwcts2self_command NULL");
                return -EINVAL;
            }
            ic->ic_send_hwcts2self_command(vdev, value);
        }
        break;
    }

    osif_vap_tx_activity_update(vap);
    osif_vap_activity_update(vap);

    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap))
        config_cmd_resp_log(scn->soc, CONFIG_TYPE_RESP, vap->iv_netdev_name, param, retv);

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (((osif_dev *)vap->iv_ifp)->mldev && vap->iv_is_up)
        is_ndev_up = true;
    else if (!((osif_dev *)vap->iv_ifp)->mldev && IS_UP(dev))
        is_ndev_up = true;
#else
    if (IS_UP(dev))
        is_ndev_up = true;
#endif

    /*
     * ic_is_vdev_restart_sup indicates vdev restart is supported by this
     * radio and retv set to ENETRESET indicates this config requires a vdev
     * restart.
     * restart_vap indicates that vdev restart optimization for config changes
     * is supported by FW.
     */
    if (retv == ENETRESET && is_ndev_up && !is_cmn_param) {
        if (ic->ic_is_vdev_restart_sup &&
            (vap->iv_opmode == IEEE80211_M_HOSTAP) && restart_vap)
            retv = osif_vdev_restart(vap);
        else
            retv = osif_vap_init(osifp, dev, RESCAN);
#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
    } else if (retv == ENETRESET && is_cmn_param && is_tx_vap) {
        return wlan_do_iter_fun_mbss_group(ic, vap->iv_mbss.group_id,
                                            MBSS_ITER_FUNC_PURPOSE_RESTART_VAP,
                                            (void *)&restart_vap);
#endif
    } else {
        retv = 0;
    }

    return retv;
}

#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
/* ieee80211_ucfg_set_param_mbss_group: Update vdev param for all vaps
 * in mbss group
 *
 * @ic: pointer to ic
 * @param: vdev param
 * @value: param value
 * @extra: extra arguments
 * @grp_id: MBSS group id
 *
 * Return: 0 on success else error val
 */
int ieee80211_ucfg_set_param_mbss_group(wlan_dev_t ic, int param, int value,
                                       char *extra, uint8_t grp_id)
{
    int ret, vap_cnt;
    unsigned long vdev_bmap = 0;
    wlan_if_t tmp_vap, tx_vap = NULL, first_vap = NULL;
    struct wlan_objmgr_vdev *tmp_vdev, *first_vdev = NULL;
    struct vdev_mlme_obj *vdev_mlme;
    struct vdev_mlme_mbss_11ax *mbss;
    struct mbssid_info *ic_mbss = &(ic->ic_mbss_ctx->ic_mbss[grp_id]);

    for (vap_cnt = 0; vap_cnt < ic->ic_mbss_ctx->grp_sz; vap_cnt++) {
         if (ic_mbss->vdev_id_gp[vap_cnt] ==
                      IEEE80211_MBSSID_INVALID_VDEV_ID)
             continue;

         /* Set param for all non tx vap in MBSS group */
         tmp_vdev = wlan_objmgr_get_vdev_by_id_from_pdev(
                            ic->ic_pdev_obj, ic_mbss->vdev_id_gp[vap_cnt],
                            WLAN_MLME_NB_ID);
         if (!tmp_vdev) {
             mbss_err("vdev obj retrieval failed for vap %d",
                          ic_mbss->vdev_id_gp[vap_cnt]);
             return -EINVAL;
         }

         tmp_vap = wlan_vdev_mlme_get_ext_hdl(tmp_vdev);
         if (!tmp_vap) {
             qdf_err("vap retrieval failed for vap %d",
                            ic_mbss->vdev_id_gp[vap_cnt]);
             wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
             return -EINVAL;
         }

         if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap)) {
             tx_vap = tmp_vap;
             wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
             continue;
         }

         if (!first_vap && !tx_vap) {
             first_vap = tmp_vap;
             first_vdev = tmp_vdev;
         }

         qdf_set_bit(tmp_vap->iv_unit, &vdev_bmap);

         vdev_mlme = tmp_vap->vdev_mlme;
         mbss = &vdev_mlme->mgmt.mbss_11ax;
         mbss->is_cmn_param = true;

         ret = ieee80211_ucfg_setparam_util(tmp_vap, param,
                                            value, extra, true, false);
         if (ret) {
             qdf_err("set param fail id %d val %d vap id %d\n",
                                 param, value, tmp_vap->iv_unit);
             wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
             return ret;
         }

         if (first_vap != tmp_vap)
             wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
    }

    if (!first_vap && !tx_vap) {
        qdf_debug("No vap in MBSS group param %d value %d", param, value);
        return ENODEV;
    }

    if (!tx_vap)
        tx_vap = first_vap;

    /* Set param for Tx VAP of the MBSS group.Pass vdev bitmap for vdevs to
     * which this param need to be set
     */
    qdf_set_bit(tx_vap->iv_unit, &vdev_bmap);

    vdev_mlme = tx_vap->vdev_mlme;
    mbss = &vdev_mlme->mgmt.mbss_11ax;
    mbss->is_cmn_param = true;
    mbss->vdev_bmap = vdev_bmap;

    ret = ieee80211_ucfg_setparam_util(tx_vap, param,
                                       value, extra, true, true);
    if (first_vap)
        wlan_objmgr_vdev_release_ref(first_vdev, WLAN_MLME_NB_ID);

    return ret;
}
#endif

int ieee80211_ucfg_setparam(wlan_if_t vap, int param, int value, char *extra)
{
    osif_dev  *osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    wlan_dev_t ic = wlan_vap_get_devhandle(vap);
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_pdev *pdev;
#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    bool is_mbssid_enabled = false;
    bool is_mbss_cmn_param = false;
    uint8_t cur_mbss_grp_id;
#endif

    if (osifp->is_delete_in_progress)
        return -EINVAL;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
        qdf_err("%s : pdev is null", __func__);
        return -1;
    }

    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);
    if (psoc == NULL) {
        qdf_err("psoc is null");
        return -1;
    }

#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
    is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(pdev,
                                                   WLAN_PDEV_F_MBSS_IE_ENABLE);

    is_mbss_cmn_param = ieee80211_ucfg_is_mbss_common_param(vap, param);

    if (is_mbssid_enabled && is_mbss_cmn_param &&
              (vap->iv_opmode == IEEE80211_M_HOSTAP)) {
        int status;
        struct mbssid_info *ic_mbss = NULL;

        if (!ic->ic_mbss_ctx) {
            qdf_err("ic_mbss_ctx NULL param %d value %d", param, value);
            return -1;
        }

        if (!ic->ic_mbss_ctx->ic_mbss) {
            qdf_err("ic_mbss NULL param %d value %d", param, value);
            return -1;
        }

        cur_mbss_grp_id = vap->iv_mbss.group_id;
        ic_mbss = &(ic->ic_mbss_ctx->ic_mbss[cur_mbss_grp_id]);

        if (!scn->ema_ctx.allow_mbss_cmn_all_vap &&
            IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            qdf_debug("Drop the CLI as the vap is not tx vap\n");
            return 0;
        }

        status = ieee80211_ucfg_set_param_mbss_group(ic, param, value,
                                                     extra, cur_mbss_grp_id);
        if (status != ENODEV)
            return status;
    }
#endif

    return ieee80211_ucfg_setparam_util(vap, param, value, extra, false, false);
}

static char num_to_char(u_int8_t n)
{
    if ( n >= 10 && n <= 15 )
        return n  - 10 + 'A';
    if ( n >= 0 && n <= 9 )
        return n  + '0';
    return ' '; //Blank space
}

/* convert MAC address in array format to string format
 * inputs:
 *     addr - mac address array
 * output:
 *     macstr - MAC address string format */
static void macaddr_num_to_str(u_int8_t *addr, char *macstr)
{
    int i, j=0;

    for ( i = 0; i < QDF_MAC_ADDR_SIZE; i++ ) {
        macstr[j++] = num_to_char(addr[i] >> 4);
        macstr[j++] = num_to_char(addr[i] & 0xF);
    }
}

bool ieee80211_is_he_rate(uint32_t rate)
{
    int i, num_rates;
    num_rates = sizeof(basic_he_rate)/sizeof(basic_he_rate[0]);
    for (i=0; i<num_rates; i++) {
         if (rate == basic_he_rate[i])
             return true;
    }
    return false;
}
qdf_export_symbol(ieee80211_is_he_rate);

bool ieee80211_is_he_mgmt_rate(wlan_if_t vap)
{
    uint32_t mgmt_rate = 0;

    if (IEEE80211_M_HOSTAP == vap->iv_opmode) {
        wlan_util_vdev_mlme_get_param(vap->vdev_mlme,
                WLAN_MLME_CFG_TX_MGMT_RATE, &mgmt_rate);
        return ieee80211_is_he_rate(mgmt_rate);
    }
    return false;
}
qdf_export_symbol(ieee80211_is_he_mgmt_rate);

bool ieee80211_is_he_bcn_rate(wlan_if_t vap)
{
    uint32_t bcn_rate = 0;

    if (IEEE80211_M_HOSTAP == vap->iv_opmode) {
        wlan_util_vdev_mlme_get_param(vap->vdev_mlme,
                WLAN_MLME_CFG_BCN_TX_RATE, &bcn_rate);
        return ieee80211_is_he_rate(bcn_rate);
    }
    return false;
}

#ifdef WLAN_SUPPORT_SCS
void ieee80211_dump_scs_rules(void *arg, struct ieee80211_node *ni)
{
    uint8_t session_index = 0, tclas_index = 0;
    struct tclas_tuple *tclas_tuple = NULL;
    struct ieee80211_scs_data *scs_data = NULL;

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    if (!ni->ni_scs || !ni->ni_scs_sessions) {
        qdf_spin_unlock_bh(&ni->ni_scs_lock);
        return;
    }

    scs_info("********* SCS rules for peer: "QDF_MAC_ADDR_FMT" ********", QDF_MAC_ADDR_REF(ni->ni_macaddr));
    scs_info("No. of SCS sessions: %u", ni->ni_scs_sessions);

    for (session_index = 0; session_index < ni->ni_scs_sessions; session_index++) {
        scs_data = ni->ni_scs[session_index];

        if (!scs_data)
            continue;

        scs_info("******** SCS_ID: %u ********", scs_data->scsid);
        scs_info("Rule_ID_Prefix: %u", scs_data->rule_id_prefix);
        scs_info("Access_Priority: %u", scs_data->access_priority);
        scs_info("State: %u", scs_data->scs_state);
        scs_info("TCLAS_Elements: %u", scs_data->tclas_elements);
        scs_info("TCLAS_Processing: %u", scs_data->tclas_processing.tclas_process);

        for (tclas_index = 0; tclas_index < scs_data->tclas_elements; tclas_index++) {
            tclas_tuple = &(scs_data->tclas_tuple[tclas_index]);

            if (tclas_tuple->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4) {
                scs_info("******** TCLAS4 Element ********");
                scs_info("Mask: %u", tclas_tuple->tclas.type4.mask);
                scs_info("Version: %u", tclas_tuple->tclas.type4.version);
                scs_info("SRC_Port: %u", tclas_tuple->tclas.type4.src_port);
                scs_info("DST_Port: %u", tclas_tuple->tclas.type4.dst_port);
                scs_info("DSCP: %u", tclas_tuple->tclas.type4.dscp);
                scs_info("Next_Header: %u", tclas_tuple->tclas.type4.next_header);

                if (tclas_tuple->tclas.type4.version == IEEE80211_WNM_TCLAS_CLAS14_VERSION_4) {
                    scs_info("SRC IPv4 Address: "QDF_IPV4_ADDR_STR, QDF_IPV4_ADDR_ARRAY(tclas_tuple->tclas.type4.src_ip.ipv4));
                    scs_info("DST IPv4 Address: "QDF_IPV4_ADDR_STR, QDF_IPV4_ADDR_ARRAY(tclas_tuple->tclas.type4.dst_ip.ipv4));
                } else {
                    scs_info("SRC IPv6 Address: "QDF_IPV6_ADDR_STR, QDF_IPV6_ADDR_ARRAY(tclas_tuple->tclas.type4.src_ip.ipv6));
                    scs_info("DST IPv6 Address: "QDF_IPV6_ADDR_STR, QDF_IPV6_ADDR_ARRAY(tclas_tuple->tclas.type4.dst_ip.ipv6));
                }
            } else if (tclas_tuple->type == IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE10){
                scs_info("******** TCLAS10 Element ********");
                scs_info("Protocol Instance: %u", tclas_tuple->tclas.type10.protocol_instance);
                scs_info("Protocol Number: %u", tclas_tuple->tclas.type10.protocol_number);
                scs_info("Filter Length: %u", tclas_tuple->tclas.type10.filter_len);
                scs_info("Filter Mask: ");
                qdf_trace_hex_dump(QDF_MODULE_ID_SCS, QDF_TRACE_LEVEL_INFO, tclas_tuple->tclas.type10.filter_mask, tclas_tuple->tclas.type10.filter_len);
                scs_info("Filter Value: ");
                qdf_trace_hex_dump(QDF_MODULE_ID_SCS, QDF_TRACE_LEVEL_INFO, tclas_tuple->tclas.type10.filter_val, tclas_tuple->tclas.type10.filter_len);
            }
        }

#ifdef WLAN_FEATURE_11BE
        if (scs_data->qos_attr) {
            struct ieee80211_qos_attr *qos_attr = scs_data->qos_attr;
            scs_info("******** QoS Characteristics ********");
            scs_info("Service Class ID: %u", qos_attr->service_class_id);
            scs_info("Direction: %u", qos_attr->direction);
            scs_info("TID/User Priority: %u", qos_attr->user_priority);
            scs_info("Bitmap: %u", qos_attr->bitmap);
            scs_info("Link_ID: %u", qos_attr->link_id);
            scs_info("Min_Service_Interval: %u microseconds", qos_attr->min_service_interval);
            scs_info("Max_Service_Interval: %u microseconds", qos_attr->max_service_interval);
            scs_info("Resultant_Service_Interval: %u microseconds", qos_attr->resultant_service_interval);
            scs_info("Min_Data_Rate: %u kbps", qos_attr->min_data_rate);
            scs_info("Delay_Bound: %u microseconds", qos_attr->delay_bound);
            scs_info("Max_MSDU_Size: %u bytes", qos_attr->max_msdu_size);
            scs_info("Service_Start_Time: %u microseconds", qos_attr->service_start_time);
            scs_info("Service_Start_Time_Link_ID: %u", qos_attr->service_start_time_link_id);
            scs_info("Mean_Data_Rate: %u kbps", qos_attr->mean_data_rate);
            scs_info("Burst_Size: %u bytes", qos_attr->burst_size);
            scs_info("Resultant_Burst_Size: %u bytes", qos_attr->resultant_burst_size);
            scs_info("MSDU_Lifetime: %u milliseconds", qos_attr->msdu_lifetime);
            scs_info("MSDU_Delivery_Ratio: %u", qos_attr->msdu_delivery_ratio);
            scs_info("MSDU_Count_Exponent: %u", qos_attr->msdu_count_exponent);
            scs_info("Medium_Time: %u (in units of 256 micro-seconds per second)", qos_attr->medium_time);
        }
#endif
    }

    scs_info("********************************************************");
    qdf_spin_unlock_bh(&ni->ni_scs_lock);
}

qdf_export_symbol(ieee80211_dump_scs_rules);
#endif

int ieee80211_ucfg_getparam(wlan_if_t vap, int param, int *value)
{
    osif_dev  *osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    wlan_dev_t ic = wlan_vap_get_devhandle(vap);
    struct wlan_objmgr_psoc *psoc;
    char *extra = (char *)value;
    int retv = 0;
    int *txpow_frm_subtype = value;
    u_int8_t frame_subtype;
    u_int8_t frame_type;
#if ATH_SUPPORT_DFS
    int tmp;
#endif
    struct wlan_objmgr_pdev *pdev;
#if WLAN_SUPPORT_SPLITMAC || defined(ATH_EXT_AP)
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
#endif
    struct ieee80211_quality iq;
    bool is_mbssid_enabled;
    uint16_t start_aid = 0;
    cdp_config_param_type val = {0};
#if SM_ENG_HIST_ENABLE
    cm_ext_t *cm_ext_handle = NULL;
#endif
    if (!osifp || osifp->is_delete_in_progress)
        return -EINVAL;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null", __func__);
        return -1;
    }

    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);
    if (psoc == NULL) {
             qdf_print("psoc is null");
             return -1;
    }

    is_mbssid_enabled= wlan_pdev_nif_feat_cap_get(pdev,
                                                  WLAN_PDEV_F_MBSS_IE_ENABLE);
    switch (param)
    {
    case IEEE80211_PARAM_MAXSTA:
        start_aid = ieee80211_get_vap_start_aid(ic, vap);
        *value = is_mbssid_enabled ?
                 (vap->iv_mbss_max_aid - 1 - (start_aid)):
                 (vap->iv_max_aid - 1);
        qdf_info("Getting Max Stations: %d", *value);
        qdf_info("aid:%d, iv_max_aid:%d, vap:%s",start_aid, vap->iv_max_aid, vap->iv_netdev_name);
        break;
    case IEEE80211_PARAM_AUTO_ASSOC:
        *value = wlan_get_param(vap, IEEE80211_AUTO_ASSOC);
        break;
    case IEEE80211_PARAM_UPTIME:
        {
            char c[128] = {0};
            u_int32_t jiffies_now = 0;
            u_int32_t jiffies_delta = 0;
            u_int32_t time_val = 0, hour_val = 0, min_val = 0, sec_val = 0;

            if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                if (wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS) {
                    snprintf(c, sizeof(c), "0");
                } else {
                    /*
                     * Identfy the duration when the VAP interface is up
                     * by subtracting the current time and the time when the
                     * vap was brought up
                     */
                    jiffies_now = OS_GET_TICKS();
                    jiffies_delta = jiffies_now - vap->vap_uptime;
                    time_val = (( qdf_system_ticks_to_msecs(jiffies_delta) ) /1000 );

                    /*
                     * Convert seconds to HH:MM:SS format
                     */
                    hour_val = time_val / 3600;
                    time_val = time_val % 3600;
                    min_val = time_val / 60;
                    sec_val = time_val % 60;

                    snprintf(c, sizeof(c), "%02u hrs %02u min %02u sec",
                             hour_val, min_val, sec_val);
                }
            } else {
                snprintf(c, sizeof(c), "Supported only for AP mode");
            }
            strlcpy(extra, c, sizeof(c));
        }
        break;
    case IEEE80211_PARAM_VAP_COUNTRY_IE:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_COUNTRY_IE);
        break;
    case IEEE80211_PARAM_VAP_DOTH:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_DOTH);
        break;
    case IEEE80211_PARAM_HT40_INTOLERANT:
        *value = wlan_get_param(vap, IEEE80211_HT40_INTOLERANT);
        break;

    case IEEE80211_PARAM_CHWIDTH:
        *value = wlan_get_param(vap, IEEE80211_CHWIDTH);
        break;

    case IEEE80211_PARAM_CHEXTOFFSET:
        *value = wlan_get_param(vap, IEEE80211_CHEXTOFFSET);
        break;
#ifdef ATH_SUPPORT_QUICK_KICKOUT
    case IEEE80211_PARAM_STA_QUICKKICKOUT:
        *value = wlan_get_param(vap, IEEE80211_STA_QUICKKICKOUT);
        break;
#endif
    case IEEE80211_PARAM_CHSCANINIT:
        *value = wlan_get_param(vap, IEEE80211_CHSCANINIT);
        break;

    case IEEE80211_PARAM_COEXT_DISABLE:
        *value = ((ic->ic_flags & IEEE80211_F_COEXT_DISABLE) != 0);
        break;

    case IEEE80211_PARAM_NR_SHARE_RADIO_FLAG:
        *value = ic->ic_nr_share_radio_flag;
        break;

    case IEEE80211_PARAM_AUTHMODE:
        //fixme how it used to be done: *value = osifp->authmode;
        {
            int32_t authmode;
            authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
            if ( authmode == -1 ) {
                qdf_err("crypto_err while getting authmode params\n");
                retv = -1;
                break;
            }
            *value = 0;
            if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_WPA) | (1 << WLAN_CRYPTO_AUTH_RSNA)))
                *value = WLAN_CRYPTO_AUTH_WPA;
            else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_OPEN) | (1 << WLAN_CRYPTO_AUTH_SHARED)))
               *value = WLAN_CRYPTO_AUTH_AUTO;
            else {
                if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_OPEN)))
                   *value = WLAN_CRYPTO_AUTH_OPEN;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_AUTO)))
                   *value = WLAN_CRYPTO_AUTH_AUTO;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_NONE)))
                   *value = WLAN_CRYPTO_AUTH_NONE;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_SHARED)))
                   *value = WLAN_CRYPTO_AUTH_SHARED;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_WPA)))
                   *value = WLAN_CRYPTO_AUTH_WPA;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_RSNA)))
                   *value = WLAN_CRYPTO_AUTH_RSNA;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_8021X)))
                   *value = WLAN_CRYPTO_AUTH_8021X;
                else if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_CCKM)))
                   *value = WLAN_CRYPTO_AUTH_CCKM;
                else if (authmode & (uint32_t)(1 << WLAN_CRYPTO_AUTH_SAE))
                   *value = WLAN_CRYPTO_AUTH_SAE;
            }
        }
        break;
     case IEEE80211_PARAM_BANDWIDTH:
         {
           *value=ieee80211_ucfg_get_bandwidth(vap);
           break;
         }
     case IEEE80211_PARAM_FREQ_BAND:
         {
           *value=ieee80211_ucfg_get_band(vap);
           break;
         }
     case IEEE80211_PARAM_EXTCHAN:
        {
           *value=ieee80211_ucfg_get_extchan(vap);
           break;
        }
     case IEEE80211_PARAM_SECOND_CENTER_FREQ:
        {
            if (ieee80211_is_phymode_8080(vap->iv_des_mode)
#ifdef WLAN_FEATURE_11BE
                || ieee80211_is_phymode_320(vap->iv_des_mode)
#endif
            ) {
                *value= vap->iv_bsschan->ic_vhtop_freq_seg2;
            }
            else {
                *value = 0;
                qdf_print(" center freq not present ");
            }
        }
        break;

     case IEEE80211_PARAM_ATH_SUPPORT_VLAN:
        {
            *value = vap->vlan_set_flags;   /* dev->flags to control VLAN tagged packets sent by NW stack */
            break;
        }

     case IEEE80211_DISABLE_BCN_BW_NSS_MAP:
        {
            *value = ic->ic_disable_bcn_bwnss_map;
            break;
        }
     case IEEE80211_DISABLE_STA_BWNSS_ADV:
        {
            *value = ic->ic_disable_bwnss_adv;
            break;
        }
     case IEEE80211_PARAM_MCS:
        {
            *value=-1;  /* auto rate */
            break;
        }
    case IEEE80211_PARAM_MCASTCIPHER:
        {
        int mcastcipher;
        mcastcipher = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_MCAST_CIPHER);
        if ( mcastcipher == -1 ) {
            qdf_err("crypto_err while getting mcastcipher params\n");
            retv = -1;
            break;
        }

            if (mcastcipher & 1<<IEEE80211_CIPHER_WEP)
                *value = IEEE80211_CIPHER_WEP;
            if (mcastcipher & 1<<IEEE80211_CIPHER_TKIP)
                *value = IEEE80211_CIPHER_TKIP;
            if (mcastcipher & 1<<IEEE80211_CIPHER_AES_CCM)
                *value = IEEE80211_CIPHER_AES_CCM;
            if (mcastcipher & 1<<IEEE80211_CIPHER_AES_CCM_256)
                *value = IEEE80211_CIPHER_AES_CCM_256;
            if (mcastcipher & 1<<IEEE80211_CIPHER_AES_GCM)
                *value = IEEE80211_CIPHER_AES_GCM;
            if (mcastcipher & 1<<IEEE80211_CIPHER_AES_GCM_256)
                *value = IEEE80211_CIPHER_AES_GCM_256;
            if (mcastcipher & 1<<IEEE80211_CIPHER_CKIP)
                *value = IEEE80211_CIPHER_CKIP;
#if ATH_SUPPORT_WAPI
            if (mcastcipher & 1<<IEEE80211_CIPHER_WAPI)
                *value = IEEE80211_CIPHER_WAPI;
#endif
            if (mcastcipher & 1<<IEEE80211_CIPHER_NONE)
                *value = IEEE80211_CIPHER_NONE;
        }
        break;
    case IEEE80211_PARAM_MCASTKEYLEN:
        qdf_print("Not supported in crypto convergence");
	*value = 0;
        break;
    case IEEE80211_PARAM_UCASTCIPHERS:
        *value = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER);
        if ( *value == -1 ) {
            qdf_err("crypto_error getting ucast params\n");
            retv = -1;
        }
        break;
    case IEEE80211_PARAM_UCASTCIPHER:
        *value = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER);
        if ( *value == -1 ) {
            qdf_err("crypto_error getting ucast params\n");
            retv = -1;
        }
        break;
    case IEEE80211_PARAM_UCASTKEYLEN:
        qdf_print("Not supported in crypto convergence");
	*value = 0;
        break;
    case IEEE80211_PARAM_PRIVACY:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_PRIVACY);
        break;
    case IEEE80211_PARAM_COUNTERMEASURES:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_COUNTER_MEASURES);
        break;
    case IEEE80211_PARAM_HIDESSID:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_HIDE_SSID);
        break;
    case IEEE80211_PARAM_APBRIDGE:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_APBRIDGE);
        break;
    case IEEE80211_PARAM_KEYMGTALGS:
        *value = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_KEY_MGMT);
        if ( *value == -1 ) {
            qdf_err("crypto_error getting key_mgmt params\n");
            retv = -1;
        }
        break;
    case IEEE80211_PARAM_RSNCAPS:
        *value = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_RSN_CAP);
        if ( *value == -1 ) {
            qdf_err("crypto_error getting rsn_cap params\n");
            retv = -1;
        }
        break;
    case IEEE80211_PARAM_WPA:
        {
            int32_t authmode;
            authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
            if ( authmode == -1 ) {
                qdf_err("crypto_err while getting authmode params\n");
                retv = -1;
                break;
            }

            *value = 0;
            if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_WPA)))
               *value |= 0x1;
            if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_RSNA)))
               *value |= 0x2;
        }
        break;
#if DBG_LVL_MAC_FILTERING
    case IEEE80211_PARAM_DBG_LVL_MAC:
        *value = vap->iv_print.dbgLVLmac_on;
        break;
#endif

    case IEEE80211_PARAM_DUMP_RA_TABLE:
        {
            uint32_t flag = 0;

#ifdef WLAN_FEATURE_11BE_MLO
            if ((IEEE80211_M_HOSTAP == osifp->os_opmode) &&
                (MLD_AP_HYBRID_NONBOND_MODE ==
                 cfg_get(wlan_vdev_get_psoc(vdev), CFG_OL_MLDEV_AP_MODE)))
                flag = DP_ME_MCTBL_MLD_VDEV;
#endif
            dp_show_me_ra_table(wlan_psoc_get_dp_handle(psoc),
                                wlan_vdev_get_id(vap->vdev_obj), flag);
         }
         break;

    case IEEE80211_PARAM_CONFIG_CATEGORY_VERBOSE:
         *value = wlan_show_shared_print_ctrl_category_verbose_table();
         break;

#ifdef QCA_OL_DMS_WAR
    case IEEE80211_PARAM_DMS_AMSDU_WAR:
         *value = vap->dms_amsdu_war;
         break;
#endif

    case IEEE80211_PARAM_DBG_LVL:
        {
            char c[128];
            *value = (u_int32_t)wlan_get_debug_flags(vap);
            snprintf(c, sizeof(c), "0x%x", *value);
            strlcpy(extra, c, sizeof(c));
        }
        break;
    case IEEE80211_PARAM_DBG_LVL_HIGH:
        /* no need to show IEEE80211_MSG_ANY to user */
        *value = (u_int32_t)((wlan_get_debug_flags(vap) & 0x7fffffff00000000ULL) >> 32);
        break;
    case IEEE80211_PARAM_MIXED_MODE:
        *value = vap->mixed_encryption_mode;
        break;
	case IEEE80211_PARAM_WEATHER_RADAR_CHANNEL:
        *value = wlan_get_param(vap, IEEE80211_WEATHER_RADAR);
        break;
    case IEEE80211_PARAM_SEND_DEAUTH:
        *value = wlan_get_param(vap, IEEE80211_SEND_DEAUTH);
        break;
    case IEEE80211_PARAM_WEP_KEYCACHE:
        *value = wlan_get_param(vap, IEEE80211_WEP_KEYCACHE);
	break;
	case IEEE80211_PARAM_GET_ACS:
        *value = wlan_get_param(vap,IEEE80211_GET_ACS_STATE);
    break;
	case IEEE80211_PARAM_GET_CAC:
        *value = wlan_get_param(vap,IEEE80211_GET_CAC_STATE);
	break;
    case IEEE80211_PARAM_SIFS_TRIGGER:
        *value = vap->iv_sifs_trigger_time;
        break;
    case IEEE80211_PARAM_BEACON_INTERVAL:
        *value = wlan_get_param(vap, IEEE80211_BEACON_INTVAL);
        break;
#if ATH_SUPPORT_AP_WDS_COMBO
    case IEEE80211_PARAM_NO_BEACON:
        *value = wlan_get_param(vap, IEEE80211_NO_BEACON);
        break;
#endif
    case IEEE80211_PARAM_PUREG:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_PUREG);
        break;
    case IEEE80211_PARAM_PUREN:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_PURE11N);
        break;
    case IEEE80211_PARAM_PURE11AC:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_PURE11AC);
        break;
    case IEEE80211_PARAM_PURE11AX:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_PURE11AX);
        break;
    case IEEE80211_PARAM_STRICT_BW:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_STRICT_BW);
        break;
    case IEEE80211_PARAM_WDS:
        *value = wlan_rptr_vdev_is_wds(vap->vdev_obj);
        break;
    case IEEE80211_PARAM_DA_WAR_ENABLE:
        cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
                        wlan_vdev_get_id(vap->vdev_obj), CDP_ENABLE_DA_WAR,
                        &val);
        *value = val.cdp_vdev_param_da_war;
        break;
#if WDS_VENDOR_EXTENSION
    case IEEE80211_PARAM_WDS_RX_POLICY:
        *value = wlan_get_param(vap, IEEE80211_WDS_RX_POLICY);
        break;
#endif
#if WLAN_SUPPORT_GREEN_AP
    case IEEE80211_IOCTL_GREEN_AP_PS_ENABLE:
        {
        uint8_t ps_enable;
        ucfg_green_ap_get_ps_config(pdev, &ps_enable);
        *value = ps_enable;
        }
        break;
    case IEEE80211_IOCTL_GREEN_AP_PS_TIMEOUT:
        {
        uint32_t trans_time;
        ucfg_green_ap_get_transition_time(pdev, &trans_time);
        *value = trans_time;
        }
        break;
    case IEEE80211_IOCTL_GREEN_AP_ENABLE_PRINT:
        *value = ucfg_green_ap_get_debug_prints(pdev);
        break;
#endif
    case IEEE80211_PARAM_WPS:
        *value = wlan_get_param(vap, IEEE80211_WPS_MODE);
        break;
    case IEEE80211_PARAM_EXTAP:
        *value = wlan_rptr_vdev_is_extap(vdev);
        break;
    case IEEE80211_PARAM_STA_FORWARD:
        *value  = wlan_get_param(vap, IEEE80211_FEATURE_STAFWD);
        break;

    case IEEE80211_PARAM_DYN_BW_RTS:
        *value = wlan_get_param(vap, IEEE80211_DYN_BW_RTS);
        break;

    case IEEE80211_PARAM_CWM_EXTPROTMODE:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_EXTPROTMODE);
        break;
    case IEEE80211_PARAM_CWM_EXTPROTSPACING:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_EXTPROTSPACING);
        break;
    case IEEE80211_PARAM_CWM_ENABLE:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_ENABLE);
        break;
    case IEEE80211_PARAM_CWM_EXTBUSYTHRESHOLD:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD);
        break;
    case IEEE80211_PARAM_DOTH:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_DOTH);
        break;
    case IEEE80211_PARAM_WMM:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_WMM);
        break;
    case IEEE80211_PARAM_PROTMODE:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_PROTECTION_MODE);
        break;
    case IEEE80211_PARAM_DRIVER_CAPS:
        *value = wlan_get_param(vap, IEEE80211_DRIVER_CAPS);
        break;
    case IEEE80211_PARAM_MACCMD:
        *value = wlan_get_acl_policy(vap, IEEE80211_ACL_FLAG_ACL_LIST_1);
        break;
    case IEEE80211_PARAM_DROPUNENCRYPTED:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_DROP_UNENC);
    break;
    case IEEE80211_PARAM_DTIM_PERIOD:
        *value = wlan_get_param(vap, IEEE80211_DTIM_INTVAL);
        break;
    case IEEE80211_PARAM_SHORT_GI:
        *value = wlan_get_param(vap, IEEE80211_SHORT_GI);
        break;
   case IEEE80211_PARAM_SHORTPREAMBLE:
        *value = wlan_get_param(vap, IEEE80211_SHORT_PREAMBLE);
        break;
   case IEEE80211_PARAM_SHORT_SLOT:
        *value = wlan_get_param(vap, IEEE80211_SHORT_SLOT);
        break;
   case IEEE80211_PARAM_CHAN_NOISE:
        *value = vap->iv_ic->ic_get_cur_chan_nf(vap->iv_ic);
        qdf_debug("Run-time average NF_dBr = %d", *value);
        break;
   case IEEE80211_PARAM_STA_MAX_CH_CAP:
        *value = vap->iv_sta_max_ch_cap;
        break;
    case IEEE80211_PARAM_OBSS_NB_RU_TOLERANCE_TIME:
        *value = ic->ic_obss_nb_ru_tolerance_time;
        break;
   case IEEE80211_PARAM_MAX_CAP_AP:
        *value = vap->iv_he_max_cap_ap;
        break;
   case IEEE80211_PARAM_NOACK_MAP:
	*value = vap->noack_map;
	qdf_info("noack_map:0x%04X", *value);
	break;
    /*
    * Support to Mcast Enhancement
    */
#if ATH_SUPPORT_IQUE
    case IEEE80211_PARAM_ME:
        *value = wlan_get_param(vap, IEEE80211_ME);
        break;
    case IEEE80211_PARAM_IGMP_ME:
        *value = wlan_get_param(vap, IEEE80211_IGMP_ME);
        break;
    case IEEE80211_PARAM_GETIQUECONFIG:
        *value = wlan_get_param(vap, IEEE80211_IQUE_CONFIG);
        break;
#endif /*ATH_SUPPORT_IQUE*/
    case IEEE80211_PARAM_ME6_WAR:
        *value = wlan_get_param(vap, IEEE80211_ME6_WAR);
        break;
    case IEEE80211_PARAM_4ADDR_EAPOL:
        *value = wlan_get_param(vap, IEEE80211_4ADDR_EAPOL);
        break;
    case IEEE80211_PARAM_SCANVALID:
        *value = 0;
        if (vap->iv_opmode == IEEE80211_M_STA) {
                *value = wlan_scan_get_aging_time(psoc);
        }
        break;
    case IEEE80211_PARAM_COUNTRYCODE:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_COUNTRYCODE);
        break;
    case IEEE80211_PARAM_11N_RATE:
        *value = wlan_get_param(vap, IEEE80211_FIXED_RATE);
        qdf_print("Getting Rate Series: %x",*value);
        break;
    case IEEE80211_PARAM_VHT_MCS:
        *value = wlan_get_param(vap, IEEE80211_FIXED_VHT_MCS);
        qdf_print("Getting VHT Rate set: %x",*value);
        break;
    case IEEE80211_PARAM_HE_MCS:
        *value = wlan_get_param(vap, IEEE80211_FIXED_HE_MCS);
        qdf_print("Getting HE MCS Rate set: %x",*value);
        break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_EHT_MCS:
        *value = wlan_get_param(vap, IEEE80211_FIXED_EHT_MCS);
        qdf_info("Getting EHT MCS Rate set: %x",*value);
        break;
    case IEEE80211_PARAM_PUNCTURE_BITMAP:
        if (ieee80211_is_phymode_11be(vap->iv_des_mode)) {
            if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
                *value = vap->iv_bsschan->op_puncture_bitmap;
            } else {
                *value = vap->iv_des_chan[vap->iv_des_mode]->op_puncture_bitmap;
            }
            qdf_debug("Getting puncture bitmap: 0x%x vap: %s", *value, vap->iv_netdev_name);
        } else {
            *value = 0;
            retv = -EOPNOTSUPP;
        }
        break;
#endif /* WLAN_FEATURE_11BE */
    case IEEE80211_PARAM_HE_MULTI_TID_AGGR:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MULTI_TID_AGGR);
        qdf_info("Getting HE Multi TID Aggr: %d", *value);
        break;
    case IEEE80211_PARAM_HE_MULTI_TID_AGGR_TX:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MULTI_TID_AGGR_TX);
        qdf_info("Getting HE Multi TID Aggr TX: %d", *value);
        break;
    case IEEE80211_PARAM_HE_MAX_AMPDU_LEN_EXP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MAX_AMPDU_LEN_EXP);
        qdf_info("Getting HE Max AMPDU Len Exp: %d", *value);
        break;
    case IEEE80211_PARAM_HE_SU_PPDU_1X_LTF_800NS_GI:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_SU_PPDU_1X_LTF_800NS_GI);
        qdf_info("Getting HE SU PPDU 1x LTF with 800ns GI: %d", *value);
        break;
    case IEEE80211_PARAM_HE_SU_MU_PPDU_4X_LTF_800NS_GI:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_SU_MU_PPDU_4X_LTF_800NS_GI);
        qdf_info("Getting HE SU MU PPDU 4x LTF with 800ns GI: %d", *value);
        break;
    case IEEE80211_PARAM_HE_MAX_FRAG_MSDU:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MAX_FRAG_MSDU);
        qdf_info("Getting HE Max Frag MSDU: %d", *value);
        break;
    case IEEE80211_PARAM_HE_MIN_FRAG_SIZE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MIN_FRAG_SIZE);
        qdf_info("Getting HE Min Frag Size: %d", *value);
        break;
    case IEEE80211_PARAM_HE_OMI:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_OMI);
        qdf_info("Getting HE OMI: %d", *value);
        break;
    case IEEE80211_PARAM_HE_NDP_4X_LTF_3200NS_GI:
        *value = wlan_get_param(vap,
                        IEEE80211_CONFIG_HE_NDP_4X_LTF_3200NS_GI);
        qdf_info("Getting HE NDP 4x HE-LTF with 3200ns GI: %d", *value);
        break;
    case IEEE80211_PARAM_HE_ER_SU_PPDU_1X_LTF_800NS_GI:
        *value = wlan_get_param(vap,
                        IEEE80211_CONFIG_HE_ER_SU_PPDU_1X_LTF_800NS_GI);
        qdf_info("Getting HE ER SU PPDU 1x HE-LTF with 800ns GI: %d", *value);
        break;
    case IEEE80211_PARAM_HE_ER_SU_PPDU_4X_LTF_800NS_GI:
        *value = wlan_get_param(vap,
                        IEEE80211_CONFIG_HE_ER_SU_PPDU_4X_LTF_800NS_GI);
        qdf_info("Getting HE ER SU PPDU 4x HE-LTF with 800ns GI: %d", *value);
        break;
    case IEEE80211_PARAM_NSS:
        *value = wlan_get_param(vap, IEEE80211_FIXED_NSS);
        qdf_print("Getting Nss: %x",*value);
        break;
    case IEEE80211_PARAM_HE_UL_SHORTGI:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_SHORTGI);
        qdf_info("Getting UL Short GI: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_UL_LTF:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_LTF);
        qdf_info("Getting UL HE LTF: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_UL_NSS:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_NSS);
        qdf_info("Getting UL HE NSS: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_UL_PPDU_BW:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_PPDU_BW);
        qdf_info("Getting UL HE PPDU BW: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_UL_LDPC:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_LDPC);
        qdf_info("Getting UL HE LDPC: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_UL_STBC:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_STBC);
        qdf_info("Getting UL HE STBC: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_UL_FIXED_RATE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_FIXED_RATE);
        qdf_info("Getting UL HE MCS: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_AMSDU_IN_AMPDU_SUPRT:
        *value = vap->iv_he_amsdu_in_ampdu_suprt;
        qdf_info("Getting HE AMSDU in AMPDU support: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_SUBFEE_STS_SUPRT:
    {
        char c[128];
        *value       = vap->iv_he_subfee_sts_lteq80;
        *(value + 1) = vap->iv_he_subfee_sts_gt80;

        qdf_info("Getting HE SUBFEE STS support lteq80: %d gt80: %d\n", *value, *(value + 1));

        snprintf(c, sizeof(c), "0x%x 0x%x", *value, *(value + 1));
        strlcpy(extra, c, sizeof(c));
    }
    break;

    case IEEE80211_PARAM_HE_4XLTF_800NS_GI_RX_SUPRT:
        *value = vap->iv_he_4xltf_800ns_gi;
        qdf_info("Getting HE 4xltf+800ns support: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_1XLTF_800NS_GI_RX_SUPRT:
        *value = vap->iv_he_1xltf_800ns_gi;
        qdf_info("Getting HE 1xltf+800ns support: %d\n", *value);
    break;

    case IEEE80211_PARAM_HE_MAX_NC_SUPRT:
        *value = vap->iv_he_max_nc;
        qdf_info("Getting HE max_nc support: %d\n", *value);
    break;

    case IEEE80211_PARAM_TWT_RESPONDER_SUPRT:
        *value = vap->iv_twt_rsp;
        qdf_info("Getting twt responder support: %d\n", *value);
    break;

    case IEEE80211_PARAM_STA_COUNT:
        {
            int sta_count =0;
            if(osifp->os_opmode == IEEE80211_M_STA)
                return -EINVAL;

            sta_count = wlan_iterate_station_list(vap, NULL,NULL);
            *value = sta_count;
            break;
        }
    case IEEE80211_PARAM_NO_VAP_RESET:
        *value = vap->iv_novap_reset;
        qdf_print("Getting VAP reset: %x",*value);
        break;

    case IEEE80211_PARAM_VHT_SGIMASK:
        *value = wlan_get_param(vap, IEEE80211_VHT_SGIMASK);
        qdf_print("Getting VHT SGI MASK: %x",*value);
        break;

    case IEEE80211_PARAM_VHT80_RATEMASK:
        *value = wlan_get_param(vap, IEEE80211_VHT80_RATEMASK);
        qdf_print("Getting VHT80 RATE MASK: %x",*value);
        break;

    case IEEE80211_PARAM_OPMODE_NOTIFY:
        *value = wlan_get_param(vap, IEEE80211_OPMODE_NOTIFY_ENABLE);
        qdf_print("Getting Notify element status: %x",*value);
        break;

    case IEEE80211_PARAM_VAP_DIFF_MODE:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_VAP_DIFF_MODE);
        break;

    case IEEE80211_PARAM_LDPC:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_LDPC);
        qdf_print("Getting LDPC: %x",*value);
        break;
    case IEEE80211_PARAM_TX_STBC:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_TX_STBC);
        qdf_print("Getting TX STBC: %x",*value);
        break;
    case IEEE80211_PARAM_RX_STBC:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_RX_STBC);
        qdf_print("Getting RX STBC: %x",*value);
        break;
    case IEEE80211_PARAM_VHT_TX_MCSMAP:
        *value = wlan_get_param(vap, IEEE80211_VHT_TX_MCSMAP);
        qdf_print("Getting VHT TX MCS MAP set: %x",*value);
        break;
    case IEEE80211_PARAM_VHT_RX_MCSMAP:
        *value = wlan_get_param(vap, IEEE80211_VHT_RX_MCSMAP);
        qdf_print("Getting VHT RX MCS MAP set: %x",*value);
        break;
    case IEEE80211_PARAM_11N_RETRIES:
        *value = wlan_get_param(vap, IEEE80211_FIXED_RETRIES);
        qdf_print("Getting Retry Series: %x",*value);
        break;
    case IEEE80211_PARAM_MCAST_RATE:
        *value = wlan_get_param(vap, IEEE80211_MCAST_RATE);
        break;
    case IEEE80211_PARAM_BCAST_RATE:
        *value = wlan_get_param(vap, IEEE80211_BCAST_RATE);
        break;
    case IEEE80211_PARAM_CCMPSW_ENCDEC:
        *value = vap->iv_ccmpsw_seldec;
        break;
    case IEEE80211_PARAM_UAPSDINFO:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_UAPSD);
        break;
    case IEEE80211_PARAM_STA_PWR_SET_PSPOLL:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_PSPOLL);
        break;
    case IEEE80211_PARAM_NETWORK_SLEEP:
        *value= (u_int32_t)wlan_get_powersave(vap);
        break;
#if UMAC_SUPPORT_WNM
    case IEEE80211_PARAM_WNM_SLEEP:
        *value= (u_int32_t)wlan_get_powersave(vap);
        break;
#endif
#if UMAC_SUPPORT_QBSSLOAD
    case IEEE80211_PARAM_QBSS_LOAD:
        *value = wlan_get_param(vap, IEEE80211_QBSS_LOAD);
	break;
#if ATH_SUPPORT_HS20
    case IEEE80211_PARAM_HC_BSSLOAD:
        *value = vap->iv_hc_bssload;
        break;
    case IEEE80211_PARAM_OSEN:
        *value = vap->iv_osen;
        break;
#endif /* ATH_SUPPORT_HS20 */
#endif /* UMAC_SUPPORT_QBSSLOAD */
#if UMAC_SUPPORT_XBSSLOAD
    case IEEE80211_PARAM_XBSS_LOAD:
        *value = wlan_get_param(vap, IEEE80211_XBSS_LOAD);
        qdf_info("Extended BSS Load: %s\n",*value? "Enabled":"Disabled");
        if (*value) {
            /* VHT IE is not present in 6Ghz mgmt frames. So,
             * rely on HE subfee capable to identify peer as
             * mu mimo capable or not.
             * HE cap does not have seperate cap for he mu bfee.
             * It is mandatory for HE peer to be dl mu mimo capable
             * if subfee is supported.
             */
            qdf_info("HE MU-MIMO capable STA Count: %d",
                         vap->iv_he_su_bformee_sta_assoc);
            qdf_info("VHT MU-MIMO capable STA Count: %d",
                         vap->iv_mu_bformee_sta_assoc);
#ifdef QCA_SUPPORT_CP_STATS
            qdf_info("Spatial Stream Underutilization: %d",
                     pdev_chan_stats_ss_under_util_get(ic->ic_pdev_obj));
            qdf_info("Observable secondary 20MHz Utilization: %d",
                     pdev_chan_stats_sec_20_util_get(ic->ic_pdev_obj));
            qdf_info("Observable secondary 40MHz Utilization: %d",
                     pdev_chan_stats_sec_40_util_get(ic->ic_pdev_obj));
            qdf_info("Observable secondary 80MHz Utilization: %d",
                     pdev_chan_stats_sec_80_util_get(ic->ic_pdev_obj));
#endif
        }
        break;
#endif /* UMAC_SUPPORT_XBSSLOAD */
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    case IEEE80211_PARAM_CHAN_UTIL_ENAB:
        *value = wlan_get_param(vap, IEEE80211_CHAN_UTIL_ENAB);
        break;
    case IEEE80211_PARAM_CHAN_UTIL:
        *value = wlan_get_param(vap, IEEE80211_CHAN_UTIL);
        break;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */
#if UMAC_SUPPORT_QUIET
    case IEEE80211_PARAM_QUIET_PERIOD:
        *value = wlan_quiet_get_param(vap);
        break;
#endif /* UMAC_SUPPORT_QUIET */
    case IEEE80211_PARAM_GET_OPMODE:
        *value = wlan_get_param(vap, IEEE80211_GET_OPMODE);
        break;
    case IEEE80211_PARAM_MBO:
        *value = wlan_get_param(vap, IEEE80211_MBO);
        break;
    case IEEE80211_PARAM_MBO_CAP:
        *value = wlan_get_param(vap, IEEE80211_MBOCAP);
        break;
    case IEEE80211_PARAM_MBO_ASSOC_DISALLOW:
        *value = wlan_get_param(vap,IEEE80211_MBO_ASSOC_DISALLOW);
        break;
    case IEEE80211_PARAM_MBO_CELLULAR_PREFERENCE:
        *value = wlan_get_param(vap,IEEE80211_MBO_CELLULAR_PREFERENCE);
        break;
    case IEEE80211_PARAM_MBO_TRANSITION_REASON:
        *value = wlan_get_param(vap,IEEE80211_MBO_TRANSITION_REASON);
        break;
    case IEEE80211_PARAM_MBO_ASSOC_RETRY_DELAY:
        *value = wlan_get_param(vap,IEEE80211_MBO_ASSOC_RETRY_DELAY);
        break;
    case IEEE80211_PARAM_OCE:
        *value = wlan_get_param(vap, IEEE80211_OCE);
        break;
    case IEEE80211_PARAM_OCE_ASSOC_REJECT:
        *value = wlan_get_param(vap, IEEE80211_OCE_ASSOC_REJECT);
        break;
    case IEEE80211_PARAM_OCE_ASSOC_MIN_RSSI:
        *value = wlan_get_param(vap, IEEE80211_OCE_ASSOC_MIN_RSSI);
        break;
    case IEEE80211_PARAM_OCE_ASSOC_RETRY_DELAY:
        *value = wlan_get_param(vap, IEEE80211_OCE_ASSOC_RETRY_DELAY);
        break;
    case IEEE80211_PARAM_ASSOC_MIN_RSSI:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ASSOC_MIN_RSSI);
        break;
    case IEEE80211_PARAM_OCE_WAN_METRICS:
        *value = wlan_get_param(vap, IEEE80211_OCE_WAN_METRICS);
        break;
    case IEEE80211_PARAM_OCE_HLP:
         *value = wlan_get_param(vap, IEEE80211_OCE_HLP);
         break;
    case IEEE80211_PARAM_NBR_SCAN_PERIOD:
        *value = vap->nbr_scan_period;
        break;
    case IEEE80211_PARAM_RNR:
        *value = vap->rnr_enable;
        break;
    case IEEE80211_PARAM_RNR_FD:
        *value = vap->rnr_enable_fd;
        break;
    case IEEE80211_PARAM_RNR_TBTT:
        *value = vap->rnr_enable_tbtt;
        break;
    case IEEE80211_PARAM_NEIG_RPT:
        *value = wlan_get_param(vap, IEEE80211_NEIGH_RPT);
        break;
    case IEEE80211_PARAM_AP_CHAN_RPT:
        *value = vap->ap_chan_rpt_enable;
        break;
    case IEEE80211_PARAM_AP_CHAN_RPT_FILTER:
        *value = vap->ap_chan_rpt_ssid_filter | vap->ap_chan_rpt_opclass_filter;
        break;
    case IEEE80211_PARAM_MGMT_RATE:
        *value = wlan_get_param(vap, IEEE80211_MGMT_RATE);
        break;
    case IEEE80211_PARAM_RTSCTS_RATE:
        *value = wlan_get_param(vap, IEEE80211_NON_BASIC_RTSCTS_RATE);
        break;
    case IEEE80211_PARAM_PRB_RATE:
        *value = wlan_get_param(vap, IEEE80211_PRB_RATE);
        break;
    case IEEE80211_PARAM_PRB_RETRY:
        *value = wlan_get_param(vap, IEEE80211_PRB_RETRY);
        break;
    case IEEE80211_PARAM_RRM_CAP:
        *value = wlan_get_param(vap, IEEE80211_RRM_CAP);
        break;
    case IEEE80211_PARAM_RRM_FILTER:
        *value = wlan_get_param(vap, IEEE80211_RRM_FILTER);
        break;
    case IEEE80211_PARAM_START_ACS_REPORT:
        *value = wlan_get_param(vap, IEEE80211_START_ACS_REPORT);
        break;
    case IEEE80211_PARAM_MIN_DWELL_ACS_REPORT:
        *value = wlan_get_param(vap, IEEE80211_MIN_DWELL_ACS_REPORT);
        break;
    case IEEE80211_PARAM_MAX_SCAN_TIME_ACS_REPORT:
        *value = wlan_get_param(vap, IEEE80211_MAX_SCAN_TIME_ACS_REPORT);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_LONG_DUR:
        *value = wlan_get_param(vap,IEEE80211_ACS_CH_HOP_LONG_DUR);
        break;
    case IEEE80211_PARAM_SCAN_MIN_DWELL:
        *value = wlan_get_param(vap, IEEE80211_SCAN_MIN_DWELL);
        break;
    case IEEE80211_PARAM_SCAN_MAX_DWELL:
        *value = wlan_get_param(vap, IEEE80211_SCAN_MAX_DWELL);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_NO_HOP_DUR:
        *value = wlan_get_param(vap, IEEE80211_ACS_CH_HOP_NO_HOP_DUR);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_CNT_WIN_DUR:
        *value = wlan_get_param(vap,IEEE80211_ACS_CH_HOP_CNT_WIN_DUR);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_NOISE_TH:
        *value = wlan_get_param(vap,IEEE80211_ACS_CH_HOP_NOISE_TH);
        break;
    case IEEE80211_PARAM_ACS_CH_HOP_CNT_TH:
        *value = wlan_get_param(vap,IEEE80211_ACS_CH_HOP_CNT_TH);
        break;
    case IEEE80211_PARAM_ACS_ENABLE_CH_HOP:
        *value = wlan_get_param(vap,IEEE80211_ACS_ENABLE_CH_HOP);
        break;
    case IEEE80211_PARAM_MAX_DWELL_ACS_REPORT:
        *value = wlan_get_param(vap, IEEE80211_MAX_DWELL_ACS_REPORT);
        break;
    case IEEE80211_PARAM_RRM_DEBUG:
        *value = wlan_get_param(vap, IEEE80211_RRM_DEBUG);
        break;
    case IEEE80211_PARAM_RRM_SLWINDOW:
        *value = wlan_get_param(vap, IEEE80211_RRM_SLWINDOW);
        break;
    case IEEE80211_PARAM_RRM_STATS:
        *value = wlan_get_param(vap, IEEE80211_RRM_STATS);
        break;
    case IEEE80211_PARAM_WNM_STATS:
        *value = wlan_get_param(vap, IEEE80211_WNM_STATS);
        break;
    case IEEE80211_PARAM_RRM_CAP_IE:
        *value = wlan_get_param(vap, IEEE80211_RRM_CAP_IE);
        break;
#if UMAC_SUPPORT_WNM
    case IEEE80211_PARAM_WNM_CAP:
        *value = wlan_get_param(vap, IEEE80211_WNM_CAP);
        break;
     case IEEE80211_PARAM_WNM_FILTER:
        *value = wlan_get_param(vap, IEEE80211_WNM_FILTER);
        break;
    case IEEE80211_PARAM_WNM_BSS_CAP:
        *value = wlan_get_param(vap, IEEE80211_WNM_BSS_CAP);
        break;
    case IEEE80211_PARAM_WNM_TFS_CAP:
        *value = wlan_get_param(vap, IEEE80211_WNM_TFS_CAP);
        break;
    case IEEE80211_PARAM_WNM_TIM_CAP:
        *value = wlan_get_param(vap, IEEE80211_WNM_TIM_CAP);
        break;
    case IEEE80211_PARAM_WNM_SLEEP_CAP:
        *value = wlan_get_param(vap, IEEE80211_WNM_SLEEP_CAP);
        break;
    case IEEE80211_PARAM_WNM_FMS_CAP:
        *value = wlan_get_param(vap, IEEE80211_WNM_FMS_CAP);
	break;
#endif
    case IEEE80211_PARAM_FWD_ACTION_FRAMES_TO_APP:
        *value = wlan_get_param(vap, IEEE80211_FWD_ACTION_FRAMES_TO_APP);
        break;
#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    case IEEE80211_PARAM_PERIODIC_SCAN:
        *value = osifp->os_periodic_scan_period;
        break;
#endif
    case IEEE80211_PARAM_VENDOR_FRAME_FWD_MASK:
        *value = osifp->wlan_vendor_fwd_mgmt_mask;
        break;
#if ATH_SW_WOW
    case IEEE80211_PARAM_SW_WOW:
        *value = wlan_get_wow(vap);
        break;
#endif
    case IEEE80211_PARAM_AMPDU:
        ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_AMPDU, value);
        break;
    case IEEE80211_PARAM_AMSDU:
        ucfg_wlan_vdev_mgr_get_param(vdev, WLAN_MLME_CFG_AMSDU, value);
        break;

    case IEEE80211_PARAM_RX_AMSDU:
        *value = ic->ic_rx_amsdu;
        break;

    case IEEE80211_PARAM_RATE_DROPDOWN:
        if (osifp->osif_is_mode_offload) {
           *value = vap->iv_ratedrop;
        } else {
           qdf_print("This Feature is Supported on Offload Mode Only");
           return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_11N_TX_AMSDU:
        *value = vap->iv_disable_ht_tx_amsdu;
        break;
    case IEEE80211_PARAM_CTSPROT_DTIM_BCN:
        *value = vap->iv_cts2self_prot_dtim_bcn;
        break;
    case IEEE80211_PARAM_VSP_ENABLE:
        *value = vap->iv_enable_vsp;
        break;
    case IEEE80211_PARAM_MAX_AMPDU:
        *value = wlan_get_param(vap, IEEE80211_MAX_AMPDU);
        break;
    case IEEE80211_PARAM_VHT_MAX_AMPDU:
        *value = wlan_get_param(vap, IEEE80211_VHT_MAX_AMPDU);
        break;
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
    case IEEE80211_PARAM_REJOINT_ATTEMP_TIME:
        *value = wlan_get_param(vap,IEEE80211_REJOINT_ATTEMP_TIME);
        break;
#endif
    case IEEE80211_PARAM_PWRTARGET:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_PWRTARGET);
        break;
    case IEEE80211_PARAM_COUNTRY_IE:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_IC_COUNTRY_IE);
        break;

    case IEEE80211_PARAM_2G_CSA:
        *value = wlan_get_device_param(ic, IEEE80211_DEVICE_2G_CSA);
        break;

    case IEEE80211_PARAM_CHANBW:
        switch(ic->ic_chanbwflag)
        {
        case IEEE80211_CHAN_HALF:
            *value = 1;
            break;
        case IEEE80211_CHAN_QUARTER:
            *value = 2;
            break;
        default:
            *value = 0;
            break;
        }
        break;
    case IEEE80211_PARAM_MFP_TEST:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_MFP_TEST);
        break;

    case IEEE80211_PARAM_INACT:
        *value = wlan_get_param(vap,IEEE80211_RUN_INACT_TIMEOUT );
        break;
    case IEEE80211_PARAM_INACT_AUTH:
        *value = wlan_get_param(vap,IEEE80211_AUTH_INACT_TIMEOUT );
        break;
    case IEEE80211_PARAM_INACT_INIT:
        *value = wlan_get_param(vap,IEEE80211_INIT_INACT_TIMEOUT );
        break;
    case IEEE80211_PARAM_SESSION_TIMEOUT:
        *value = wlan_get_param(vap,IEEE80211_SESSION_TIMEOUT );
        break;
    case IEEE80211_PARAM_COMPRESSION:
        *value = wlan_get_param(vap, IEEE80211_COMP);
        break;
    case IEEE80211_PARAM_FF:
        *value = wlan_get_param(vap, IEEE80211_FF);
        break;
    case IEEE80211_PARAM_TURBO:
        *value = wlan_get_param(vap, IEEE80211_TURBO);
        break;
    case IEEE80211_PARAM_BURST:
        *value = wlan_get_param(vap, IEEE80211_BURST);
        break;
    case IEEE80211_PARAM_AR:
        *value = wlan_get_param(vap, IEEE80211_AR);
        break;
#if UMAC_SUPPORT_STA_POWERSAVE
    case IEEE80211_PARAM_SLEEP:
        *value = wlan_get_param(vap, IEEE80211_SLEEP);
        break;
#endif
    case IEEE80211_PARAM_EOSPDROP:
        *value = wlan_get_param(vap, IEEE80211_EOSPDROP);
        break;
    case IEEE80211_PARAM_DFSDOMAIN:
        *value = wlan_get_param(vap, IEEE80211_DFSDOMAIN);
        break;
    case IEEE80211_PARAM_WDS_AUTODETECT:
        *value = wlan_get_param(vap, IEEE80211_WDS_AUTODETECT);
        break;
    case IEEE80211_PARAM_WEP_TKIP_HT:
        *value = wlan_get_param(vap, IEEE80211_WEP_TKIP_HT);
        break;
    /*
    ** Support for returning the radio number
    */
    case IEEE80211_PARAM_ATH_RADIO:
		*value = wlan_get_param(vap, IEEE80211_ATH_RADIO);
        break;
    case IEEE80211_PARAM_IGNORE_11DBEACON:
        *value = wlan_get_param(vap, IEEE80211_IGNORE_11DBEACON);
        break;
#if ATH_SUPPORT_WAPI
    case IEEE80211_PARAM_WAPIREKEY_USK:
        *value = wlan_get_wapirekey_unicast(vap);
        break;
    case IEEE80211_PARAM_WAPIREKEY_MSK:
        *value = wlan_get_wapirekey_multicast(vap);
        break;
#endif

#ifdef QCA_PARTNER_PLATFORM
    case IEEE80211_PARAM_PLTFRM_PRIVATE:
        *value = wlan_pltfrm_get_param(vap);
        break;
#endif
    case IEEE80211_PARAM_NO_STOP_DISASSOC:
        *value = osifp->no_stop_disassoc;
        break;
#if UMAC_SUPPORT_VI_DBG

    case IEEE80211_PARAM_DBG_CFG:
        *value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_DBG_CFG);
        break;

    case IEEE80211_PARAM_RESTART:
        *value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RESTART);
        break;
    case IEEE80211_PARAM_RXDROP_STATUS:
        *value = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_RXDROP_STATUS);
        break;
#endif

#ifdef ATH_SUPPORT_TxBF
    case IEEE80211_PARAM_TXBF_AUTO_CVUPDATE:
        *value = wlan_get_param(vap, IEEE80211_TXBF_AUTO_CVUPDATE);
        break;
    case IEEE80211_PARAM_TXBF_CVUPDATE_PER:
        *value = wlan_get_param(vap, IEEE80211_TXBF_CVUPDATE_PER);
        break;
#endif
    case IEEE80211_PARAM_SCAN_BAND:
        *value = osifp->os_scan_band;
        break;

    case IEEE80211_PARAM_SCAN_CHAN_EVENT:
        if (osifp->osif_is_mode_offload &&
            wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) {
            *value = osifp->is_scan_chevent;
        } else {
            qdf_print("IEEE80211_PARAM_SCAN_CHAN_EVENT is valid only for 11ac "
                   "offload, and in IEEE80211_M_HOSTAP(Access Point) mode");
            retv = EOPNOTSUPP;
            *value = 0;
        }
        break;
    case IEEE80211_PARAM_ROAMING:
        *value = ic->ic_roaming;
        break;
#ifdef UMAC_SUPPORT_PROXY_ARP
    case IEEE80211_PARAM_PROXYARP_CAP:
        *value = wlan_get_param(vap, IEEE80211_PROXYARP_CAP);
	    break;
#if UMAC_SUPPORT_DGAF_DISABLE
    case IEEE80211_PARAM_DGAF_DISABLE:
        *value = wlan_get_param(vap, IEEE80211_DGAF_DISABLE);
	    break;
#endif
#endif
#if UMAC_SUPPORT_HS20_L2TIF
    case IEEE80211_PARAM_L2TIF_CAP:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_APBRIDGE) ? 0 : 1;
        break;
#endif
    case IEEE80211_PARAM_EXT_ACS_IN_PROGRESS:
        *value = wlan_get_param(vap, IEEE80211_EXT_ACS_IN_PROGRESS);
        break;

    case IEEE80211_PARAM_SEND_ADDITIONAL_IES:
        *value = wlan_get_param(vap, IEEE80211_SEND_ADDITIONAL_IES);
        break;

    case IEEE80211_PARAM_DESIRED_CHANNEL:
        *value = wlan_get_param(vap, IEEE80211_DESIRED_CHANNEL);
        break;

    case IEEE80211_PARAM_DESIRED_PHYMODE:
        *value = wlan_get_param(vap, IEEE80211_DESIRED_PHYMODE);
        break;

    case IEEE80211_PARAM_GET_FREQUENCY:
        *value = ieee80211_ucfg_get_freq(vap);
        break;

    case IEEE80211_PARAM_APONLY:
#if UMAC_SUPPORT_APONLY
        *value = vap->iv_aponly;
#else
        qdf_print("APONLY not enabled");
#endif
        break;

#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    case IEEE80211_PARAM_NOPBN:
        *value = wlan_get_param(vap, IEEE80211_NOPBN);
	break;
#endif
#if ATH_SUPPORT_DSCP_OVERRIDE
	case IEEE80211_PARAM_DSCP_MAP_ID:
		*value = wlan_get_param(vap, IEEE80211_DSCP_MAP_ID);
	break;
	case IEEE80211_PARAM_DSCP_TID_MAP:
		*value = ieee80211_ucfg_vap_get_dscp_tid_map(vap, value[1]);
	break;
        case IEEE80211_PARAM_VAP_DSCP_PRIORITY:
                *value = wlan_get_param(vap, IEEE80211_VAP_DSCP_PRIORITY);
        break;
#endif
#if ATH_SUPPORT_WRAP
    case IEEE80211_PARAM_PARENT_IFINDEX:
        *value = osifp->os_comdev->ifindex;
        break;

    case IEEE80211_PARAM_PROXY_STA:
        *value = wlan_rptr_vdev_is_psta(vap->vdev_obj);
        break;
#endif
     case IEEE80211_PARAM_VDEV_TSF:
	ic->ic_vap_get_param(vap, IEEE80211_VDEV_TSF);
	break;
#if RX_CHECKSUM_OFFLOAD
    case IEEE80211_PARAM_RX_CKSUM_ERR_STATS:
	{
	    if(osifp->osif_is_mode_offload) {
                ic->ic_vap_get_param(vap, IEEE80211_RX_CKSUM_ERR_STATS_GET);
	    } else
		qdf_print("RX Checksum Offload Supported only for 11AC VAP ");
	    break;
	}
    case IEEE80211_PARAM_RX_CKSUM_ERR_RESET:
	{
	    if(osifp->osif_is_mode_offload) {
                ic->ic_vap_get_param(vap, IEEE80211_RX_CKSUM_ERR_RESET_GET);
	    } else
		qdf_print("RX Checksum Offload Supported only for 11AC VAP ");
	    break;
	}

#endif /* RX_CHECKSM_OFFLOAD */

#if HOST_SW_TSO_SG_ENABLE
    case IEEE80211_PARAM_TSO_STATS:
	{
	    if(osifp->osif_is_mode_offload) {
		ic->ic_vap_get_param(vap, IEEE80211_TSO_STATS_GET);
	    } else
		qdf_print("TSO Supported only for 11AC VAP ");
	    break;
	}
    case IEEE80211_PARAM_TSO_STATS_RESET:
	{
	    if(osifp->osif_is_mode_offload) {
                ic->ic_vap_get_param(vap, IEEE80211_TSO_STATS_RESET_GET);
	    } else
		qdf_print("TSO Supported only for 11AC VAP ");
	    break;
	}
#endif /* HOST_SW_TSO_SG_ENABLE */

#if HOST_SW_SG_ENABLE
    case IEEE80211_PARAM_SG_STATS:
	{
	    if(osifp->osif_is_mode_offload) {
		ic->ic_vap_get_param(vap, IEEE80211_SG_STATS_GET);
	    } else
		qdf_print("SG Supported only for 11AC VAP ");
	    break;
	}
    case IEEE80211_PARAM_SG_STATS_RESET:
	{
	    if(osifp->osif_is_mode_offload) {
		ic->ic_vap_get_param(vap, IEEE80211_SG_STATS_RESET_GET);
	    } else {
		qdf_print("SG Supported only for 11AC VAP ");
            }
	    break;
	}
#endif /* HOST_SW_SG_ENABLE */

    case IEEE80211_PARAM_MAX_SCANENTRY:
        *value = wlan_get_param(vap, IEEE80211_MAX_SCANENTRY);
        break;
    case IEEE80211_PARAM_SCANENTRY_TIMEOUT:
        *value = wlan_get_param(vap, IEEE80211_SCANENTRY_TIMEOUT);
        break;
#if ATH_PERF_PWR_OFFLOAD
    case IEEE80211_PARAM_VAP_TX_ENCAP_TYPE:
        *value = wlan_get_param(vap, IEEE80211_VAP_TX_ENCAP_TYPE);
        switch (*value)
        {
            case 0:
                qdf_print("Encap type: Raw");
                break;
            case 1:
                qdf_print("Encap type: Native Wi-Fi");
                break;
            case 2:
                qdf_print("Encap type: Ethernet");
                break;
            default:
                qdf_print("Encap type: Unknown");
                break;
        }
        break;
    case IEEE80211_PARAM_VAP_RX_DECAP_TYPE:
        *value = wlan_get_param(vap, IEEE80211_VAP_RX_DECAP_TYPE);
        switch (*value)
        {
            case 0:
                qdf_print("Decap type: Raw");
                break;
            case 1:
                qdf_print("Decap type: Native Wi-Fi");
                break;
            case 2:
                qdf_print("Decap type: Ethernet");
                break;
            default:
                qdf_print("Decap type: Unknown");
                break;
        }
        break;
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    case IEEE80211_PARAM_RAWMODE_SIM_TXAGGR:
        *value = wlan_get_param(vap, IEEE80211_RAWMODE_SIM_TXAGGR);
        break;
    case IEEE80211_PARAM_RAWMODE_PKT_SIM_STATS:
        *value = wlan_get_param(vap, IEEE80211_RAWMODE_PKT_SIM_STATS);
        break;
    case IEEE80211_PARAM_RAWMODE_SIM_DEBUG_LEVEL:
        *value = wlan_get_param(vap, IEEE80211_RAWMODE_SIM_DEBUG_LEVEL);
        break;
    case IEEE80211_PARAM_RAWSIM_DEBUG_NUM_ENCAP_FRAMES:
        *value = wlan_get_param(vap, IEEE80211_RAWSIM_DEBUG_NUM_ENCAP_FRAMES);
        break;
    case IEEE80211_PARAM_RAWSIM_DEBUG_NUM_DECAP_FRAMES:
        *value = wlan_get_param(vap, IEEE80211_RAWSIM_DEBUG_NUM_DECAP_FRAMES);
        break;
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
#endif /* ATH_PERF_PWR_OFFLOAD */
 case IEEE80211_PARAM_VAP_ENHIND:
        *value  = wlan_rptr_pdev_is_eir(ic->ic_pdev_obj);
        break;
    case IEEE80211_PARAM_VAP_PAUSE_SCAN:
        *value = vap->iv_pause_scan;
        break;
#if ATH_GEN_RANDOMNESS
    case IEEE80211_PARAM_RANDOMGEN_MODE:
        *value = ic->random_gen_mode;
        break;
#endif
    case IEEE80211_PARAM_RX_SIGNAL_DBM:
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"IEEE80211_PARAM_RX_SIGNAL_DBM is valid only for DA not supported for offload \n");
         retv = EOPNOTSUPP;
         *value = 0;
	break;
    default:
        retv = EOPNOTSUPP;
        break;

   case IEEE80211_PARAM_DFS_CACTIMEOUT:
#if ATH_SUPPORT_DFS
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }
        tmp = ieee80211_dfs_get_cac_timeout(ic, ic->ic_curchan);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
        if (tmp >= 0)
            *value = tmp;
        else
            retv = EOPNOTSUPP;
        break;
#else
        retv = EOPNOTSUPP;
        break;
#endif /* ATH_SUPPORT_DFS */

   case IEEE80211_PARAM_ENABLE_RTSCTS:
       *value = wlan_get_param(vap, IEEE80211_ENABLE_RTSCTS);
       break;

   case IEEE80211_PARAM_RC_NUM_RETRIES:
       *value = wlan_get_param(vap, IEEE80211_RC_NUM_RETRIES);
       break;
   case IEEE80211_PARAM_256QAM_2G:
       *value = wlan_get_param(vap, IEEE80211_256QAM);
       break;
   case IEEE80211_PARAM_11NG_VHT_INTEROP:
       if (osifp->osif_is_mode_offload) {
            *value = wlan_get_param(vap, IEEE80211_11NG_VHT_INTEROP);
       } else {
            qdf_print("Not supported in this Vap");
       }
       break;
#if UMAC_VOW_DEBUG
    case IEEE80211_PARAM_VOW_DBG_ENABLE:
        *value = (int)osifp->vow_dbg_en;
        break;
#endif
#if WLAN_SUPPORT_SPLITMAC
    case IEEE80211_PARAM_SPLITMAC:
        *value = splitmac_get_enabled_flag(vdev);
        break;
#endif
    case IEEE80211_PARAM_IMPLICITBF:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_IMPLICITBF);
        break;

    case IEEE80211_PARAM_VHT_SUBFEE:
        *value = wlan_get_param(vap, IEEE80211_VHT_SUBFEE);
        break;

    case IEEE80211_PARAM_VHT_MUBFEE:
        *value = wlan_get_param(vap, IEEE80211_VHT_MUBFEE);
        break;

    case IEEE80211_PARAM_VHT_SUBFER:
        *value = wlan_get_param(vap, IEEE80211_VHT_SUBFER);
        break;

    case IEEE80211_PARAM_VHT_MUBFER:
        *value = wlan_get_param(vap, IEEE80211_VHT_MUBFER);
        break;

    case IEEE80211_PARAM_VHT_STS_CAP:
        *value = wlan_get_param(vap, IEEE80211_VHT_BF_STS_CAP);
        break;

    case IEEE80211_PARAM_VHT_SOUNDING_DIM:
        *value = wlan_get_param(vap, IEEE80211_VHT_BF_SOUNDING_DIM);
        break;

    case IEEE80211_PARAM_VHT_MCS_10_11_SUPP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_VHT_MCS_10_11_SUPP);
        break;

    case IEEE80211_PARAM_VHT_MCS_10_11_NQ2Q_PEER_SUPP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_VHT_MCS_10_11_NQ2Q_PEER_SUPP);
        break;
    case IEEE80211_PARAM_MCAST_RC_STALE_PERIOD:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_MCAST_RC_STALE_PERIOD);
        break;
    case IEEE80211_PARAM_ENABLE_MCAST_RC:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ENABLE_MCAST_RC);
        break;

#if QCA_AIRTIME_FAIRNESS
    case IEEE80211_PARAM_ATF_TXBUF_SHARE:
        *value = ucfg_atf_get_txbuf_share(vap->vdev_obj);
        break;
    case IEEE80211_PARAM_ATF_TXBUF_MAX:
        *value = ucfg_atf_get_max_txbufs(vap->vdev_obj);
        break;
    case IEEE80211_PARAM_ATF_TXBUF_MIN:
        *value = ucfg_atf_get_min_txbufs(vap->vdev_obj);
        break;
    case  IEEE80211_PARAM_ATF_OPT:
        *value = ucfg_atf_get(vap->vdev_obj);
        break;
    case IEEE80211_PARAM_ATF_OVERRIDE_AIRTIME_TPUT:
        *value =  ucfg_atf_get_airtime_tput(vap->vdev_obj);
        break;
    case  IEEE80211_PARAM_ATF_PER_UNIT:
        *value = ucfg_atf_get_per_unit(ic->ic_pdev_obj);
        break;
    case  IEEE80211_PARAM_ATF_MAX_CLIENT:
        {
            int val = ucfg_atf_get_maxclient(ic->ic_pdev_obj);
            if (val < 0)
                retv = EOPNOTSUPP;
            else
                *value = val;
        }
        break;
    case  IEEE80211_PARAM_ATF_SSID_GROUP:
        *value = ucfg_atf_get_ssidgroup(ic->ic_pdev_obj);
        break;
    case IEEE80211_PARAM_ATF_SSID_SCHED_POLICY:
        *value = ucfg_atf_get_ssid_sched_policy(vap->vdev_obj);
        break;
    case IEEE80211_PARAM_ATF_VIP_INFRA:
        *value = ucfg_atf_get_vip_infra(vap->vdev_obj);
        break;
#if QCA_SUPPORT_ATF_STATS
    case IEEE80211_PARAM_ATF_ENABLE_STATS:
        *value = ucfg_atf_is_stats_enabled(ic->ic_pdev_obj);
        break;
    case IEEE80211_PARAM_ATF_STATS_TIMEOUT:
        *value = ucfg_atf_get_stats_timeout(ic->ic_pdev_obj);
        break;
#endif /* QCA_SUPPORT_ATF_STATS */
#endif /* QCA_AIRTIME_FAIRNESS */
#if QCA_SUPPORT_SSID_STEERING
    case IEEE80211_PARAM_VAP_SSID_CONFIG:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_VAP_SSID_STEERING);
        qdf_info("This VAP's configuration value is %d ( %d-PRIVATE %d-PUBLIC )",
                *value, SON_SSID_STEERING_PRIVATE_VDEV, SON_SSID_STEERING_PUBLIC_VDEV);
        break;
#endif
    case IEEE80211_PARAM_TX_MIN_POWER:
        *value = ic->ic_curchan->ic_minpower;
        qdf_print("Get IEEE80211_PARAM_TX_MIN_POWER *value=%d",*value);
        break;
    case IEEE80211_PARAM_TX_MAX_POWER:
        *value = ic->ic_curchan->ic_maxpower;
        qdf_print("Get IEEE80211_PARAM_TX_MAX_POWER *value=%d",*value);
        break;
    case IEEE80211_PARAM_AMPDU_DENSITY_OVERRIDE:
        if(ic->ic_mpdudensityoverride & 0x1) {
            *value = ic->ic_mpdudensityoverride >> 1;
        } else {
            *value = -1;
        }
        break;

    case IEEE80211_PARAM_SMART_MESH_CONFIG:
        *value = wlan_get_param(vap, IEEE80211_SMART_MESH_CONFIG);
        break;

#if MESH_MODE_SUPPORT
    case IEEE80211_PARAM_MESH_CAPABILITIES:
        *value = wlan_get_param(vap, IEEE80211_MESH_CAPABILITIES);
        break;

    case IEEE80211_PARAM_CONFIG_MGMT_TX_FOR_MESH:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_MGMT_TX_FOR_MESH);
         break;

    case IEEE80211_PARAM_MESH_MCAST:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_MESH_MCAST);
#endif

    case IEEE80211_PARAM_RX_FILTER_MONITOR:
        if(IEEE80211_M_MONITOR != vap->iv_opmode && !IEEE80211_IS_SPECIAL_VAP(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Not monitor VAP or Smart Monitor VAP!\n");
            return -EINVAL;
        }
        if(osifp->osif_is_mode_offload) {
           *value =  (ic->mon_filter_osif_mac ? MON_FILTER_TYPE_OSIF_MAC : 0)|
                       ic->ic_vap_get_param(vap, IEEE80211_RX_FILTER_MONITOR);
        }
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_IOCTL,
                  "osif MAC filter=%d\n", ic->mon_filter_osif_mac);
        break;

    case IEEE80211_PARAM_RX_FILTER_SMART_MONITOR:
        if(IEEE80211_IS_SMART_MON_VAP(vap)) {
            *value = 1;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Smart Monitor VAP!\n");
        } else {
            *value = 0;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Not Smart Monitor VAP!\n");
        }
        break;
    case IEEE80211_PARAM_CONFIG_ASSOC_WAR_160W:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ASSOC_WAR_160W);
        break;
     case IEEE80211_PARAM_CONFIG_MU_CAP_WAR:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_MU_CAP_WAR);
        break;
     case IEEE80211_PARAM_CONFIG_MU_CAP_TIMER:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_MU_CAP_TIMER);
        break;
    case IEEE80211_PARAM_RAWMODE_PKT_SIM:
        *value = wlan_get_param(vap, IEEE80211_RAWMODE_PKT_SIM);
        break;
    case IEEE80211_PARAM_CONFIG_RAW_DWEP_IND:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_RAW_DWEP_IND);
        break;
    case IEEE80211_PARAM_CUSTOM_CHAN_LIST:
        *value = wlan_get_param(vap,IEEE80211_CONFIG_PARAM_CUSTOM_CHAN_LIST);
        break;
#if UMAC_SUPPORT_ACFG
    case IEEE80211_PARAM_DIAG_WARN_THRESHOLD:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DIAG_WARN_THRESHOLD);
        break;
    case IEEE80211_PARAM_DIAG_ERR_THRESHOLD:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DIAG_ERR_THRESHOLD);
        break;
#endif
    case IEEE80211_PARAM_CONFIG_REV_SIG_160W:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_REV_SIG_160W);
        break;
    case IEEE80211_PARAM_DISABLE_SELECTIVE_HTMCS_FOR_VAP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DISABLE_SELECTIVE_HTMCS);
        break;
    case IEEE80211_PARAM_CONFIGURE_SELECTIVE_VHTMCS_FOR_VAP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_CONFIGURE_SELECTIVE_VHTMCS);
        break;
    case IEEE80211_PARAM_RDG_ENABLE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_RDG_ENABLE);
        break;
    case IEEE80211_PARAM_DFS_SUPPORT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DFS_SUPPORT);
        break;
    case IEEE80211_PARAM_DFS_ENABLE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DFS_ENABLE);
        break;
    case IEEE80211_PARAM_ACS_SUPPORT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ACS_SUPPORT);
        break;
    case IEEE80211_PARAM_SSID_STATUS:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_SSID_STATUS);
        break;
    case IEEE80211_PARAM_DL_QUEUE_PRIORITY_SUPPORT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DL_QUEUE_PRIORITY_SUPPORT);
        break;
    case IEEE80211_PARAM_CLEAR_MIN_MAX_SNR:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_CLEAR_MIN_MAX_SNR);
        break;
    case IEEE80211_PARAM_WATERMARK_THRESHOLD:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_WATERMARK_THRESHOLD);
        break;
    case IEEE80211_PARAM_WATERMARK_REACHED:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_WATERMARK_REACHED);
        break;
    case IEEE80211_PARAM_ASSOC_REACHED:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ASSOC_REACHED);
        break;
    case IEEE80211_PARAM_ENABLE_VENDOR_IE:
	 *value = vap->iv_ena_vendor_ie;
        break;
    case IEEE80211_PARAM_CONFIG_ASSOC_DENIAL_NOTIFY:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ASSOC_DENIAL_NOTIFICATION);
        break;
   case IEEE80211_PARAM_MACCMD_SEC:
        *value = wlan_get_acl_policy(vap, IEEE80211_ACL_FLAG_ACL_LIST_2);
        break;
    case IEEE80211_PARAM_CONFIG_MON_DECODER:
        if (IEEE80211_M_MONITOR != vap->iv_opmode && !IEEE80211_IS_SMART_MON_VAP(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Not Monitor VAP!\n");
            return -EINVAL;
        }
        /* monitor vap decoder header type: radiotap=0(default) prism=1 */
        *value = wlan_get_param(vap, IEEE80211_CONFIG_MON_DECODER);
        break;
    case IEEE80211_PARAM_BEACON_RATE_FOR_VAP:
        if(vap->iv_opmode == IEEE80211_M_HOSTAP) {
            *value = wlan_get_param(vap, IEEE80211_BEACON_RATE_FOR_VAP);
        }
        break;
    case IEEE80211_PARAM_SIFS_TRIGGER_RATE:
        *value = vap->iv_sifs_trigger_rate;
        break;
    case IEEE80211_PARAM_DISABLE_SELECTIVE_LEGACY_RATE_FOR_VAP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DISABLE_SELECTIVE_LEGACY_RATE);
        break;
    case IEEE80211_PARAM_CONFIG_NSTSCAP_WAR:
        *value = wlan_get_param(vap,IEEE80211_CONFIG_NSTSCAP_WAR);
        break;
    case IEEE80211_PARAM_CHANNEL_SWITCH_MODE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_CHANNEL_SWITCH_MODE);
        break;

    case IEEE80211_PARAM_ENABLE_ECSA_IE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ECSA_IE);
        break;

    case IEEE80211_PARAM_SAE_PWID:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_SAE_PWID);
        break;

    case IEEE80211_PARAM_OCE_TX_POWER:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_OCE_TX_POWER);
        break;

    case IEEE80211_PARAM_ECSA_OPCLASS:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ECSA_OPCLASS);
        break;

    case IEEE80211_PARAM_BACKHAUL:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_BACKHAUL);
        break;

#if DYNAMIC_BEACON_SUPPORT
    case IEEE80211_PARAM_DBEACON_EN:
        *value = vap->iv_dbeacon;
        break;

    case IEEE80211_PARAM_DBEACON_SNR_THR:
        *value = vap->iv_dbeacon_snr_thr;
        break;

    case IEEE80211_PARAM_DBEACON_TIMEOUT:
        *value = vap->iv_dbeacon_timeout;
        break;
#endif
#if QCN_IE
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_ENABLE:
        *value = vap->iv_bpr_enable;
        break;
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_LATENCY_COMPENSATION:
        *value = ic->ic_bpr_latency_comp;
        break;
    case IEEE80211_PARAM_BEACON_LATENCY_COMPENSATION:
        *value = ic->ic_bcn_latency_comp;
        break;
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_DELAY:
        *value = vap->iv_bpr_delay;
        break;
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_STATS:
        *value = 0;
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            QDF_TRACE(QDF_MODULE_ID_IOCTL, QDF_TRACE_LEVEL_FATAL,
                      "------------------------------------\n");
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR feature enabled        - %d  |\n", vap->iv_bpr_enable);

            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR Latency compensation   - %d ms |\n", ic->ic_bpr_latency_comp);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| Beacon Latency compensation- %d ms |\n", ic->ic_bcn_latency_comp);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR delay                  - %d ms |\n", vap->iv_bpr_delay);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| Current Timestamp          - %lld |\n",
                      qdf_ktime_to_ns(qdf_ktime_get()));
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| Next beacon Timestamp      - %lld |\n",
                      qdf_ktime_to_ns(vap->iv_next_beacon_tstamp));
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| Timer expires in           - %lld |\n",
                      qdf_ktime_to_ns(qdf_ktime_add(qdf_ktime_get(),
                      qdf_hrtimer_get_remaining(&vap->bpr_timer))));
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR timer start count      - %u |\n", vap->iv_bpr_timer_start_count);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR unicast probresp count - %u |\n", vap->iv_bpr_unicast_resp_count);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR timer resize count     - %u |\n", vap->iv_bpr_timer_resize_count);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR timer callback count   - %u |\n", vap->iv_bpr_callback_count);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "| BPR timer cancel count     - %u |\n", vap->iv_bpr_timer_cancel_count);
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "------------------------------------\n");
        } else {
            QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_FATAL,
                      "Invalid. Allowed only in HOSTAP mode\n");
        }
        break;
    case IEEE80211_PARAM_BCAST_PROBE_RESPONSE_STATS_CLEAR:
        *value = 0;
        vap->iv_bpr_timer_start_count  = 0;
        vap->iv_bpr_timer_resize_count = 0;
        vap->iv_bpr_callback_count     = 0;
        vap->iv_bpr_unicast_resp_count = 0;
        vap->iv_bpr_timer_cancel_count = 0;
        break;
#endif
    case IEEE80211_PARAM_HE_ER_SU_DISABLE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_ER_SU_DISABLE);
        break;
    case IEEE80211_PARAM_HE_1024QAM_LT242RU_RX_ENABLE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_1024QAM_LT242RU_RX_ENABLE);
        break;
    case IEEE80211_PARAM_HE_UL_MU_DATA_DIS_RX_SUPP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_MU_DATA_DIS_RX_SUPP);
        break;
    case IEEE80211_PARAM_HE_FULL_BW_UL_MUMIMO:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_FULL_BW_UL_MUMIMO);
        break;
    case IEEE80211_PARAM_HE_DCM_MAX_CONSTELLATION_RX:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_DCM_MAX_CONSTELLATION_RX);
        break;
    case IEEE80211_PARAM_DISABLE_INACT_PROBING:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DISABLE_INACT_PROBING);
        break;
    case IEEE80211_PARAM_TXPOW_MGMT:
        frame_subtype = txpow_frm_subtype[1];
        if (((frame_subtype & ~IEEE80211_FC0_SUBTYPE_MASK)!=0) || (frame_subtype < IEEE80211_FC0_SUBTYPE_ASSOC_REQ)
                || (frame_subtype > IEEE80211_FC0_SUBTYPE_DEAUTH) ) {
            qdf_print("Invalid value entered for frame subtype");
            return -EINVAL;
        }
        *value = vap->iv_txpow_mgt_frm[(frame_subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)];
        break;
    case IEEE80211_PARAM_TXPOW:
        frame_type = txpow_frm_subtype[1] >> IEEE80211_SUBTYPE_TXPOW_SHIFT;
        frame_subtype = (txpow_frm_subtype[1] & 0xff);
        if (((frame_subtype & ~IEEE80211_FC0_SUBTYPE_MASK)!=0) || (frame_subtype < IEEE80211_FC0_SUBTYPE_ASSOC_REQ)
                || (frame_subtype > IEEE80211_FC0_SUBTYPE_CF_END_ACK) || (frame_type > IEEE80211_FC0_TYPE_DATA) ) {
            return -EINVAL;
        }
        *value = vap->iv_txpow_frm[frame_type >>IEEE80211_FC0_TYPE_SHIFT][(frame_subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)];
        break;
    case IEEE80211_PARAM_CONFIG_TX_CAPTURE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_TX_CAPTURE);
        break;

    case IEEE80211_PARAM_ENABLE_FILS:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_FILS);
    break;
    case IEEE80211_PARAM_HE_EXTENDED_RANGE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_EXTENDED_RANGE);
    break;
    case IEEE80211_PARAM_HE_DCM:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_DCM);
    break;
    case IEEE80211_PARAM_HE_FRAGMENTATION:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_FRAGMENTATION);
    break;
    case IEEE80211_PARAM_HE_MU_EDCA:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MU_EDCA);
    break;
    case IEEE80211_PARAM_HE_DYNAMIC_MU_EDCA:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_DYNAMIC_MU_EDCA);
    break;
    case IEEE80211_PARAM_HE_DL_MU_OFDMA:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_DL_MU_OFDMA);
    break;
    case IEEE80211_PARAM_HE_DL_MU_OFDMA_BFER:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_DL_MU_OFDMA_BFER);
    break;
    case IEEE80211_PARAM_HE_UL_MU_MIMO:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_MU_MIMO);
    break;
    case IEEE80211_PARAM_6G_HE_OP_MIN_RATE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_6G_HE_OP_MIN_RATE);
    break;
    case IEEE80211_PARAM_HE_UL_MU_OFDMA:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_UL_MU_OFDMA);
    break;
    case IEEE80211_PARAM_HE_SU_BFEE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_SU_BFEE);
    break;
    case IEEE80211_PARAM_HE_SU_BFER:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_SU_BFER);
    break;
    case IEEE80211_PARAM_HE_MU_BFEE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MU_BFEE);
    break;
    case IEEE80211_PARAM_HE_MU_BFER:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MU_BFER);
    break;
    case IEEE80211_PARAM_EXT_NSS_SUPPORT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_EXT_NSS_SUPPORT);
    break;
    case IEEE80211_PARAM_QOS_ACTION_FRAME_CONFIG:
        *value = ic->ic_qos_acfrm_config;
    break;
    case IEEE80211_PARAM_HE_LTF:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_LTF);
    break;
    case IEEE80211_PARAM_HE_AR_GI_LTF:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_AR_GI_LTF);
    break;
    case IEEE80211_PARAM_HE_AR_LDPC:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_AR_LDPC);
        if(*value == IEEE80211_HE_AR_LDPC_DEFAULT){
          qdf_err("HE Auto Rate LDPC will be set by FW based on BW, NSS and MCS settings");
          return -EINVAL;
        }
    break;
    case IEEE80211_PARAM_HE_RTSTHRSHLD:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_RTSTHRSHLD);
    break;
    case IEEE80211_PARAM_DFS_INFO_NOTIFY_APP:
        *value = ic->ic_dfs_info_notify_channel_available;
    break;
    case IEEE80211_PARAM_DISABLE_CABQ:
        *value = wlan_get_param(vap, IEEE80211_FEATURE_DISABLE_CABQ);
        break;
#if ATH_ACL_SOFTBLOCKING
    case IEEE80211_PARAM_SOFTBLOCK_WAIT_TIME:
        *value = vap->iv_softblock_wait_time;
        break;
    case IEEE80211_PARAM_SOFTBLOCK_ALLOW_TIME:
        *value = vap->iv_softblock_allow_time;
        break;
#endif
    case IEEE80211_PARAM_CSL_SUPPORT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_CSL_SUPPORT);
    break;
    case IEEE80211_PARAM_TIMEOUTIE:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_TIMEOUTIE);
        break;
    case IEEE80211_PARAM_PMF_ASSOC:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_PMF_ASSOC);
        break;

    case IEEE80211_PARAM_HE_TX_MCSMAP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_TX_MCSMAP);
        qdf_print("Getting HE TX MCS MAP set: %x",*value);
        break;
    case IEEE80211_PARAM_HE_RX_MCSMAP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_RX_MCSMAP);
        qdf_print("Getting HE RX MCS MAP set: %x",*value);
        break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_EHT_TX_MCSMAP:
        *value = vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0];
        qdf_info("Getting EHT TX MCS MAP set: 80Mhz %x 160MHz %x 320Mhz %x",\
                vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0],
                vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX1],
                vap->iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX2]);
        break;
    case IEEE80211_PARAM_EHT_RX_MCSMAP:
        *value = vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0];
        qdf_info("Getting EHT RX MCS MAP set: 80Mhz %x 160MHz %x 320Mhz %x",\
                vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0],
                vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX1],
                vap->iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX2]);
        break;
#endif /* WLAN_FEATURE_11BE */
    case IEEE80211_PARAM_CONFIG_M_COPY:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_M_COPY);
        break;
    case IEEE80211_PARAM_CONFIG_CAPTURE_LATENCY_ENABLE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_CAPTURE_LATENCY_ENABLE);
        break;
    case IEEE80211_PARAM_BA_BUFFER_SIZE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_BA_BUFFER_SIZE);
        break;
    case IEEE80211_PARAM_NSSOL_VAP_READ_RXPREHDR:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_READ_RXPREHDR);
        break;
    case IEEE80211_PARAM_HE_SOUNDING_MODE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_SOUNDING_MODE);
        break;
    case IEEE80211_PARAM_RSN_OVERRIDE:
        *value = wlan_get_param(vap, IEEE80211_SUPPORT_RSN_OVERRIDE);
        break;
    case IEEE80211_PARAM_HE_HT_CTRL:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_HT_CTRL);
        break;
    case IEEE80211_PARAM_RAWMODE_OPEN_WAR:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_RAWMODE_OPEN_WAR);
        break;
    case IEEE80211_PARAM_MODE:
        *value = wlan_get_param(vap, IEEE80211_DRIVER_HW_CAPS);
        break;
    case IEEE80211_PARAM_MODE_EXT:
        *value = wlan_get_param(vap, IEEE80211_DRIVER_HW_CAPS_EXT);
        break;
    case IEEE80211_PARAM_FT_ENABLE:
        if(vap->iv_opmode == IEEE80211_M_STA) {
            *value = wlan_get_param(vap, IEEE80211_CONFIG_FT_ENABLE);
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                "Valid only in STA mode\n");
            return -EINVAL;
        }
        break;
    case IEEE80211_PARAM_SAE_AUTH_ATTEMPTS:
        *value = vap->iv_sae_max_auth_attempts;
        break;
    case IEEE80211_PARAM_DPP_VAP_MODE:
        *value = vap->iv_dpp_vap_mode;
        break;
    case IEEE80211_PARAM_HE_BSR_SUPPORT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_BSR_SUPPORT);
    break;
    case IEEE80211_PARAM_UNIFORM_RSSI:
        *value = ic->ic_uniform_rssi;
         break;
    case IEEE80211_PARAM_CSA_INTEROP_PHY:
        *value = vap->iv_csa_interop_phy;
        break;
    case IEEE80211_PARAM_CSA_INTEROP_BSS:
        *value = vap->iv_csa_interop_bss;
        break;
    case IEEE80211_PARAM_CSA_INTEROP_AUTH:
        *value = vap->iv_csa_interop_auth;
        break;
    case IEEE80211_PARAM_GET_RU26_TOLERANCE:
        *value = ic->ru26_tolerant;
        break;
#if SM_ENG_HIST_ENABLE
    case IEEE80211_PARAM_SM_HISTORY:
         wlan_mlme_print_all_sm_history();
         break;
#endif
#if WLAN_SCHED_HISTORY_SIZE
    case IEEE80211_PARAM_WLAN_SCHED_HISTORY:
        sched_history_print();
        break;
#endif
    case IEEE80211_PARAM_GET_MAX_RATE:
        *value = ieee80211_ucfg_get_maxphyrate(vap);
        break;
    case IEEE80211_PARAM_GET_SIGNAL_LEVEL:
        ieee80211_ucfg_get_quality(vap, (void *)&iq);
        *value = iq.level;
        break;
    case IEEE80211_PARAM_MAX_MTU_SIZE:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_MAX_MTU_SIZE);
        break;
    case IEEE80211_PARAM_HE_6GHZ_BCAST_PROB_RSP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_6GHZ_BCAST_PROB_RSP);
        break;
    case IEEE80211_PARAM_VHT_MCS_12_13_SUPP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_HE_MCS_12_13_SUPP);
        break;
#ifdef VDEV_PEER_PROTOCOL_COUNT
    case IEEE80211_PARAM_VDEV_PEER_PROTOCOL_COUNT:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_VDEV_PEER_PROTOCOL_COUNT);
        break;
    case IEEE80211_PARAM_VDEV_PEER_PROTOCOL_DROP_MASK:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_VDEV_PEER_PROTOCOL_DROP_MASK);
        break;
#endif
    case IEEE80211_PARAM_FILS_IS_ENABLE:
        {
#if WLAN_SUPPORT_FILS
        struct ieee80211vap *tx_vap;

        tx_vap = vap;
        if (is_mbssid_enabled) {
            if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
                tx_vap = ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].transmit_vap;
            }
        }

        if (tx_vap)
            *value = wlan_fils_is_enable(tx_vap->vdev_obj);
        else
            *value = 0;
#endif
        }
        break;
   case IEEE80211_PARAM_STEALTHDOWN:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_STEALTHDOWN);
        break;
   case IEEE80211_PARAM_OCE_VERSION_OVERRIDE:
        *value = wlan_get_param(vap, IEEE80211_OCE_VERSION_OVERRIDE);
        break;
    case IEEE80211_PARAM_ENABLE_MSCS:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ENABLE_MSCS);
        break;
#ifdef WLAN_SUPPORT_SCS
    case IEEE80211_PARAM_ENABLE_SCS:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_ENABLE_SCS);
        break;
    case IEEE80211_PARAM_SCS_RULES:
        {
            if (vap->iv_scs) {
                *value = wlan_iterate_station_list(vap,
                                      ieee80211_dump_scs_rules, NULL);
            }
            break;
        }
#endif
#if WLAN_SUPPORT_DAP
    case IEEE80211_PARAM_DSCP_ACTION_POLICY:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_DSCP_ACTION_POLICY);
        break;
#endif
    case IEEE80211_PARAM_CURRENT_PP:
        if (!is_mbssid_enabled) {
            *value = 0;
        }
        else {
            *value = ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].current_pp;
        }
        break;
    case IEEE80211_PARAM_NO_ACT_VAPS:
        *value = ieee80211_get_num_ap_vaps_up(ic);
        break;
    case IEEE80211_PARAM_TX_VAP:
        {
            if (is_mbssid_enabled) {
                struct ieee80211vap *tx_vap =
                    ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id].transmit_vap;

                if (tx_vap) {
                    *value = wlan_vdev_get_id(tx_vap->vdev_obj);
                } else {
                    *value = NO_TX_VAP;
                }
            }
        }
        break;
    case IEEE80211_PARAM_HLOS_TID_OVERRIDE:
         *value = ieee80211_ucfg_get_hlos_tid_override(osifp);
        break;
    case IEEE80211_PARAM_6G_SECURITY_COMP:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_6G_SECURITY_COMP);
        break;
    case IEEE80211_PARAM_6G_KEYMGMT_MASK:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_6G_KEYMGMT_MASK);
        break;
    case IEEE80211_PARAM_MBSS_TXVDEV:
        *value = 0;

        if (ic->ic_mbss_ctx) {
            uint8_t group_id = vap->iv_mbss.group_id;
            if (is_mbssid_enabled) {
                if ((ic->ic_mbss_ctx->ic_mbss[group_id].transmit_vap) &&
                    (ic->ic_mbss_ctx->ic_mbss[group_id].transmit_vap == vap))
                    *value = 1;
            } else {
                if (ic->ic_mbss_ctx->ic_mbss[group_id].target_transmit_vap &&
                    ic->ic_mbss_ctx->ic_mbss[group_id].target_transmit_vap == vap)
                    *value = 1;
            }
        }
        break;
    case IEEE80211_PARAM_MBSS_GROUP:
        *value = vap->iv_mbss.group_id;
        break;
#if SM_ENG_HIST_ENABLE
    case IEEE80211_PARAM_CM_HISTORY:
         if (wlan_vap_get_opmode(vap) == IEEE80211_M_STA) {
             cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);
             if (!cm_ext_handle)
                 break;
             wlan_cm_sm_history_print(vap->vdev_obj);
             wlan_mlme_cm_action_print_history(&cm_ext_handle->cm_action_history);
             wlan_cm_req_history_print(vap->vdev_obj);
         } else {
             qdf_err("Cmd supported for STA mode only");
         }
         break;
#endif
    case IEEE80211_PARAM_RTS:
        *value = wlan_get_param(vap, IEEE80211_RTS_THRESHOLD);
        break;
    case IEEE80211_PARAM_MCAST_STEER:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_MCAST_STEER);
        break;
    case IEEE80211_PARAM_SM_GAP_PS_ENABLE:
        *value = vap->iv_sm_gap_ps;
        break;
#ifdef WLAN_SUPPORT_MESH_LATENCY
    case IEEE80211_PARAM_PEER_TID_LATENCY_ENABLE:
         *value = ieee80211_ucfg_get_peer_tid_latency_enable(osifp);
         break;
#endif
    case IEEE80211_PARAM_AP_MAX_AUTH_FAIL:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_AP_MAX_AUTH_FAIL);
         break;
    case IEEE80211_PARAM_SPL_VAP_SCAN:
         *value = vap->iv_spl_vap_scan;
         break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_EPCS_PRIORITY_ACCESS:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_maccap_epcspriaccess_support;
         break;
    case IEEE80211_PARAM_OM_CONTROL:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_maccap_ehtomctrl_support;
         break;
    case IEEE80211_PARAM_TRIGGERED_TXOP_SHARING:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_maccap_trigtxop_sharing_mode1;
         break;
    case IEEE80211_PARAM_EHT_NDP_4X_EHT_LTF_AND_320NSGI:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_ndp4xehtltfand320nsgi;
         break;
    case IEEE80211_PARAM_EHT_SU_BFMR:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfmr;
         break;
    case IEEE80211_PARAM_EHT_SU_BFME:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfme;
         break;
    case IEEE80211_PARAM_EHT_BFME_SS:
         qdf_info("eht bfmee lt80 ss %d 160ss %d 320ss %d",
                    vap->vdev_mlme->proto.eht_cap_info.eht_phycap_bfmesslt80mhz,
                    vap->vdev_mlme->proto.eht_cap_info.eht_phycap_bfmess160mhz,
                    vap->vdev_mlme->proto.eht_cap_info.eht_phycap_bfmess320mhz);
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_bfmesslt80mhz;
         break;
    case IEEE80211_PARAM_EHT_NUM_SD:
         qdf_info("eht bfmer lt80 sd %d 160sd %d 320sd %d",
                    vap->vdev_mlme->proto.eht_cap_info.eht_phycap_numsoundlt80mhz,
                    vap->vdev_mlme->proto.eht_cap_info.eht_phycap_numsound160mhz,
                    vap->vdev_mlme->proto.eht_cap_info.eht_phycap_numsound320mhz);
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_numsoundlt80mhz;
         break;
    case IEEE80211_PARAM_EHT_4X_EHT_LTF_AND_800NS_GI:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_4xehtltfand800nsgi;
         break;
    case IEEE80211_PARAM_EHT_MAX_NC:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_maxnc;
         break;
    case IEEE80211_PARAM_EHT_RX_1024_AND_4096_QAM_LS_242_TONE_RU:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_rx1024and4096qamls242toneru;
         break;
    case IEEE80211_PARAM_EHT_MU_BFMR:
         qdf_info("eht bfmee: lt80mhz %d 160mhz %d 320mhz %d",
                      vap->vdev_mlme->proto.eht_cap_info.eht_phycap_mubfmrlt80mhz,
                      vap->vdev_mlme->proto.eht_cap_info.eht_phycap_mubfmr160mhz,
                      vap->vdev_mlme->proto.eht_cap_info.eht_phycap_mubfmr320mhz);
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_mubfmrlt80mhz;
         break;
    case IEEE80211_PARAM_EHT_MU_BFEE:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_MU_BFEE);
         break;
    case IEEE80211_PARAM_EHT_UL_MU_OFDMA:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_OFDMA);
         break;
    case IEEE80211_PARAM_EHT_DL_MU_OFDMA:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_DL_OFDMA);
         break;
    case IEEE80211_PARAM_EHT_MU_MIMO:
         qdf_info("eht mumimo: lt80mhz %d 160mhz %d 320mhz %d",
                 vap->vdev_mlme->proto.eht_cap_info.eht_phycap_nonofdmaulmumimolt80mhz,
                 vap->vdev_mlme->proto.eht_cap_info.eht_phycap_nonofdmaulmumimo160mhz,
                 vap->vdev_mlme->proto.eht_cap_info.eht_phycap_nonofdmaulmumimo320mhz);
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_nonofdmaulmumimolt80mhz;
         break;
    case IEEE80211_PARAM_EHT_DL_OFDMA_TXBF:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_DL_OFDMA_TXBF);
         break;
    case IEEE80211_PARAM_EHT_DL_OFDMA_MUMIMO:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_partbwdlmumimo;
         break;
    case IEEE80211_PARAM_EHT_UL_OFDMA_MUMIMO:
         *value = vap->vdev_mlme->proto.eht_cap_info.eht_phycap_partialbwulmu;
         break;
    case IEEE80211_PARAM_EHT_UL_NSS:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_NSS);
         qdf_info("Getting EHT UL HE NSS: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_UL_PPDU_BW:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_PPDU_BW);
         qdf_info("Getting EHT UL PPDU BW: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_UL_SHORTGI:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_SHORTGI);
         qdf_info("Getting EHT UL Short GI: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_UL_LDPC:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_LDPC);
         qdf_info("Getting EHT UL LDPC: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_UL_STBC:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_STBC);
         qdf_info("Getting UL EHT STBC: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_UL_FIXED_RATE:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_FIXED_RATE);
         qdf_info("Getting UL EHT MCS: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_UL_LTF:
         *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_UL_LTF);
         qdf_info("Getting UL EHT LTF: %d\n", *value);
         break;
    case IEEE80211_PARAM_EHT_LTF:
        *value = wlan_get_param(vap, IEEE80211_CONFIG_EHT_LTF);
        break;
    case IEEE80211_PARAM_EHT_MAX_SUPP_LTF:
        *value = wlan_get_param(vap, IEEE80211_EHT_MAX_NUM_EHT_LTF);
        break;
    case IEEE80211_PARAM_EHT_SUP_MCS15:
        *value = wlan_get_param(vap, IEEE80211_EHT_SUP_MCS15);
        break;
    case IEEE80211_PARAM_EHT_MCS14_DUP_IN_6GHZ:
        *value = wlan_get_param(vap, IEEE80211_EHT_EHT_DUP_IN_6GHZ);
        break;
    case IEEE80211_PARAM_EHT_CONFIG_CCFS0:
        *value = ic->eht_config_ccfs0;
        break;
#endif
    case IEEE80211_PARAM_DROP_3ADDR_MCAST:
        cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
            wlan_vdev_get_id(vap->vdev_obj), CDP_DROP_3ADDR_MCAST,
            &val);
        *value = val.cdp_drop_3addr_mcast;
        break;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    case IEEE80211_PARAM_DROP_TX_MCAST:
        cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
            wlan_vdev_get_id(vap->vdev_obj), CDP_DROP_TX_MCAST,
            &val);
        *value = val.cdp_drop_tx_mcast;
        break;
    case IEEE80211_PARAM_WDS_EXT_AP_BRIDGE:
        cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
            wlan_vdev_get_id(vap->vdev_obj), CDP_WDS_EXT_AP_BRIDGE,
            &val);
        *value = val.cdp_vdev_paran_wds_ext_ap_bridge;
        break;
    case IEEE80211_PARAM_WDS_EXT_EN:
        *value = wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_WDS_EXTENDED);
	break;
#endif
#ifdef QCA_PPE_VP
    case IEEE80211_PARAM_PPEVP_TYPE:
        {
            const char *ppevp_type = NULL;
            if (vap->cp.icp_flags & IEEE80211_PPE_VP_NO_ACCEL) {
                ppevp_type = "none";
            } else if (vap->cp.icp_flags & IEEE80211_PPE_VP_PASSIVE_ACCEL) {
                ppevp_type = "passive";
            } else if (vap->cp.icp_flags & IEEE80211_PPE_VP_ACTIVE_ACCEL) {
                ppevp_type = "active";
            } else if (vap->cp.icp_flags & IEEE80211_PPE_VP_DS_ACCEL) {
                ppevp_type = "ds";
            } else {
                ppevp_type = "not configured";
            }
            qdf_nofl_info("ppe vp user type is: %s", ppevp_type);
        }
        break;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
   case IEEE80211_PARAM_HW_LINK_ID:
       {
          if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
              *value = wlan_mlo_get_pdev_hw_link_id(ic->ic_pdev_obj);
          } else {
              qdf_nofl_info("command issued on non ml vap");
              *value = -1;
          }
       }
       break;

   case IEEE80211_PARAM_IEEE_LINK_ID:
       {
          if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
              *value = vap->vdev_obj->vdev_mlme.mlo_link_id;
          } else {
              qdf_nofl_info("command issued on non ml vap");
              *value = -1;
          }
       }
       break;
#endif

#ifdef CONFIG_MLO_SINGLE_DEV
   case IEEE80211_PARAM_GET_MLD_PEER:
       {
          struct osif_mldev *mldev = NULL;
          struct osifp_peer *peer = NULL;
          uint32_t index, i, j;
          osif_dev *osif_dev = NULL;

          if (!osifp->mldev) {
              qdf_nofl_info("no mld peer data");
              break;
          }
          mldev = osifp->mldev;
          qdf_info("failed packet count: %llu\n", mldev->pkt_fail_cnt);

          switch (mldev->dev_type)
          {
          case OSIF_NETDEV_TYPE_VAP:
               qdf_info("dev_type: OSIF_NETDEV_TYPE_VAP\n");
               break;
          case OSIF_NETDEV_TYPE_WDS_EXT:
               qdf_info("dev_type: OSIF_NETDEV_TYPE_WDS_EXT\n");
               break;
          case OSIF_NETDEV_TYPE_MLO:
               qdf_info("dev_type: OSIF_NETDEV_TYPE_MLO\n");
               break;
          default:
               qdf_info("invalid dev_type\n");
               break;
          }

          qdf_info("primary_chid_id:%u primary_pdev_id:%u\n", mldev->primary_chip_id, mldev->primary_pdev_id);
          qdf_info("-------------------link devices-------------------------\n");
          for (i = 0; i < MAX_MLO_CHIPS; i++) {
               for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
                    if (!mldev->link_dev[i][j])
                        continue;
                    osif_dev = mldev->link_dev[i][j];
                    qdf_info("dev_name:%s  chip_id:%u  pdev_id:%u\n", osif_dev->netdev->name, i, j);
               }
          }
          qdf_info("--------------------------------------------------------\n");

          for (i = 0; i < MAX_MLO_CHIPS; i++) {
               for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
                    if (!mldev->bridge_dev[i][j])
                        continue;
                    osif_dev = mldev->bridge_dev[i][j];
                    qdf_info("bridge dev: chip_id:%u  pdev_id:%u\n", i, j);
               }
          }

          qdf_spin_lock_bh(&mldev->peer_hash_lock);
          qdf_info("----------------------------------------------------------------------------------\n");
          for (index = 0; index <= mldev->peer_hash.mask; index++) {
               TAILQ_FOREACH(peer, &mldev->peer_hash.bins[index], hash_list_elem) {
                 qdf_info("mac_addr:%pM primary_chip_id:%u pdev_id:%u vdev_id:%u logically_deleted:%u is_bridge_peer = %u\n",
                          peer->mac_addr, peer->primary_chip_id, peer->pdev_id, peer->vdev_id, peer->logically_deleted,
                          peer->is_bridge_peer);
                 qdf_info("----------------------------------------------------------------------------------\n");
               }
          }
          qdf_spin_unlock_bh(&mldev->peer_hash_lock);
      }
      break;
#endif
    case IEEE80211_PARAM_MLD_NETDEV_NAME:
        {
#ifdef CONFIG_MLO_SINGLE_DEV
            if (osifp->mldev)
                strlcpy(extra, osifp->mldev->mld_dev->name, IFNAMSIZ);
#endif
        }
        break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_T2LM_CAPABILITY:
        {
            uint8_t t2lm_support;
            if (ic->ic_get_t2lm_enabled) {
                ic->ic_get_t2lm_enabled(ic, &t2lm_support);
                *value = (int)t2lm_support;
            } else
                retv = EOPNOTSUPP;
        }
        break;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    case IEEE80211_PARAM_FREQ_SEP_CAPABILITY:
        {
            struct wlan_mlo_mld_cap phy_mld_cap;
            if (ic->ic_get_mld_phy_cap) {
                if (ic->ic_get_mld_phy_cap(ic, &phy_mld_cap) ==
                    QDF_STATUS_SUCCESS)
                    *value = (int)phy_mld_cap.str_freq_sep;
                else
                    retv = EOPNOTSUPP;
            } else
                retv = EOPNOTSUPP;
        }
        break;
    case IEEE80211_PARAM_EMLSR_CAPABILITY:
        {
            struct wlan_mlo_eml_cap phy_eml_cap;
            if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                if (ic->ic_get_eml_phy_cap) {
                   if (ic->ic_get_eml_phy_cap(ic, &phy_eml_cap) ==
                       QDF_STATUS_SUCCESS)
                       *value = (int)phy_eml_cap.emlsr_supp;
                   else
                       retv = EOPNOTSUPP;
                } else
                    retv = EOPNOTSUPP;
            } else
                *value = 0;
        }
        break;
    case IEEE80211_PARAM_EMLMR_CAPABILITY:
        {
            struct wlan_mlo_eml_cap phy_eml_cap;
            if (ic->ic_get_eml_phy_cap) {
                if (ic->ic_get_eml_phy_cap(ic, &phy_eml_cap) ==
                    QDF_STATUS_SUCCESS)
                    *value = (int)phy_eml_cap.emlmr_supp;
                else
                    retv = EOPNOTSUPP;
            } else
                retv = EOPNOTSUPP;
        }
        break;
    case IEEE80211_PARAM_EMLSR_CONFIG:
        {
            struct wlan_mlo_eml_cap phy_eml_cap;
            bool is_eml_enabled = false;
            if (vap->iv_opmode == IEEE80211_M_HOSTAP &&
                ic->ic_get_eml_phy_cap) {
                if (ic->ic_get_eml_phy_cap(ic, &phy_eml_cap) ==
                    QDF_STATUS_SUCCESS) {
                    if (phy_eml_cap.emlsr_supp) {
                        if (ic->ic_get_enable_eml_cap) {
                            ic->ic_get_enable_eml_cap(ic, &is_eml_enabled);
                            *value = (int)is_eml_enabled;
                        } else
                            retv = EOPNOTSUPP;
                    } else
                        *value = 0;
                } else
                    retv = EOPNOTSUPP;
            } else
                retv = EOPNOTSUPP;
        }
        break;
    case IEEE80211_PARAM_EMLMR_CONFIG:
        {
            struct wlan_mlo_eml_cap phy_eml_cap;
            bool is_eml_enabled = false;
            if (ic->ic_get_eml_phy_cap) {
                if (ic->ic_get_eml_phy_cap(ic, &phy_eml_cap) ==
                    QDF_STATUS_SUCCESS) {
                    if (phy_eml_cap.emlmr_supp) {
                        if (ic->ic_get_enable_eml_cap) {
                            ic->ic_get_enable_eml_cap(ic, &is_eml_enabled);
                            *value = (int)is_eml_enabled;
                        } else
                            retv = EOPNOTSUPP;
                    } else
                        *value = 0;
                } else
                    retv = EOPNOTSUPP;
            } else
                retv = EOPNOTSUPP;
        }
        break;
#endif
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    case IEEE80211_PARAM_STR_CAPABILITY:
        {
            /* 802.11be chipsets support STR mode of operation.
             * However, as there is no ops API to fetch
             * the STR capability, check for MLO capability
             * to ensure correct value for non-MLO targets.
             */
            struct wlan_objmgr_psoc *psoc;
            bool is_mlo_capable;

            psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
            is_mlo_capable = ic->ic_mlo_capable(psoc);
            if (is_mlo_capable)
                *value = 1;
            else
                retv = EOPNOTSUPP;
        }
        break;
    case IEEE80211_PARAM_NSTR_CAPABILITY:
        {
            /* 802.11be chipsets are not capable of NSTR mode of operation */
            struct wlan_objmgr_psoc *psoc;
            bool is_mlo_capable;

            psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
            is_mlo_capable = ic->ic_mlo_capable(psoc);
            if (is_mlo_capable)
                *value = 0;
            else
                retv = EOPNOTSUPP;
        }
        break;
    case IEEE80211_PARAM_STR_CONFIG:
        {
            /* 802.11be chipsets support STR mode of operation.
             * However, as there is no ops API to fetch
             * the STR capability, check for MLO capability
             * to ensure correct value for non-MLO targets.
             */
            struct wlan_objmgr_psoc *psoc;
            bool is_mlo_capable;

            psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
            is_mlo_capable = ic->ic_mlo_capable(psoc);
            if (is_mlo_capable)
                *value = 1;
             else
                retv = EOPNOTSUPP;
        }
        break;
    case IEEE80211_PARAM_NSTR_CONFIG:
        {
            /* 802.11 be chipsets are incapable of NSTR mode of operation */
            struct wlan_objmgr_psoc *psoc;
            bool is_mlo_capable;

            psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
            is_mlo_capable = ic->ic_mlo_capable(psoc);
            if (is_mlo_capable)
                *value = 0;
            else
                retv = EOPNOTSUPP;
        }
        break;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    case IEEE80211_PARAM_WLAN_PEER_MESH_OVERRIDE:
        {
            switch (vap->wlan_peer_mesh_override_flag) {
                case WLAN_PEER_MESH_NODE_OVERRIDE_ENABLE:
                    qdf_info("Enforcing - flag properties would be always applied.");
                    break;
                case WLAN_PEER_MESH_NODE_OVERRIDE_NO_OP:
                    qdf_info("Inactive flag");
                    break;
                case WLAN_PEER_MESH_NODE_OVERRIDE_DISABLE:
                    qdf_info("Disabled - flag properties will never be set.");
                    break;
                default:
                    qdf_info("Invalid flag value");
                    break;
            }
            *value = vap->wlan_peer_mesh_override_flag;
        }
        break;
    case IEEE80211_PARAM_MLO_MAX_RECOM_ACTIVE_LINKS:
        {
            *value =
                    wlan_get_param(vap,
                                   IEEE80211_CONFIG_MLO_MAX_RECOM_ACTIVE_LINKS);
        }
    break;
    case IEEE80211_PARAM_MLO_EXTMLDCAPOP_FLAG:
        {
            *value =
                    wlan_get_param(vap,
                                   IEEE80211_CONFIG_MLO_EXTMLDCAP_FLAG);
        }
    break;
        case IEEE80211_PARAM_MLO_LINK_REJ_FLAG:
        {
            struct wlan_objmgr_vdev *vdev;

            if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
                mlo_err("configuration not allowed for non-ap sta ML");
                return -EINVAL;
            }

            vdev = vap->vdev_obj;
            if (!vdev) {
                mlo_err("VDEV is null");
                return -EINVAL;
            }

            if (vdev->mlo_dev_ctx && vdev->mlo_dev_ctx->ap_ctx)
                *value = vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject;
        }
        break;
        case IEEE80211_PARAM_MLO_LINK_REJ_TEST:
        {
            if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
                mlo_err("configuration not allowed for non-ap sta ML");
                return -EINVAL;
            }
            mlo_info("LRE num_assoc_link:%u, link_rejected %s:%u",
                     vap->lre_test_num_assoc_link, vap->iv_netdev_name,
                     vap->lre_test_num_peers);
        }
        break;
#endif
#ifdef ASSOC_REJECT_SUPPORT_ENABLED
    case IEEE80211_PARAM_ASSOC_REJECT:
        *value = wlan_get_param(vap,IEEE80211_CONFIG_ASSOC_REJECT);
        break;
#endif
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_PARAM_TPE_COMMON_PSD:
        *value = vap->iv_ucfg_tpe_common_psd;
        break;
    case IEEE80211_PARAM_TPE_PWR_UNIT:
        *value = vap->iv_ucfg_tpe_pwr_unit;
        break;
    case IEEE80211_PARAM_TPE_PUNC_PWR:
        *value = vap->iv_ucfg_tpe_punc_pwr;
        break;
#endif
    case IEEE80211_PARAM_VDEV_UP:
        {
            /* Return true if VDEV is in UP State */
            if (QDF_IS_STATUS_SUCCESS(wlan_vdev_chan_config_valid(vap->vdev_obj)))
                *value = 1;
            else
                *value = 0;
        }
        break;
    case IEEE80211_PARAM_SET_RTT_RESPONDER_ROLE:
        qdf_info("configured RTT responder role:0x%x", vap->rtt_responder_role);
        *value = vap->rtt_responder_role;
        break;
    }

    if (retv) {
        qdf_print("%s : parameter 0x%x not supported ", __func__, param);
        return -EOPNOTSUPP;
    }

    return retv;
}

int ieee80211_get_chan_nf(struct ieee80211com *ic, int16_t *nf_val)
{
    struct wlan_objmgr_psoc *psoc;

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        qdf_err("psoc is null");
        return -EINVAL;
    }

    if (!ic->ic_is_target_lithium) {
        qdf_err("ic_is_target_lithium is null");
        return -EINVAL;
    }

    if (ic->ic_is_target_lithium(psoc)) {
        if (!ic->ic_get_cur_hw_nf) {
            qdf_err("ic_get_cur_hw_nf is null");
            return -EINVAL;
        }
        *nf_val = ic->ic_get_cur_hw_nf(ic);
    } else {
        if (!ic->ic_get_cur_chan_nf) {
            qdf_err("ic_get_cur_chan_nf is null");
            return -EINVAL;
        }
        *nf_val = ic->ic_get_cur_chan_nf(ic);
    }

    return 0;
}

int ieee80211_ucfg_get_quality(wlan_if_t vap, void *iq)
{
    wlan_snr_info snr_info;
    int16_t nf_val = 0;

    wlan_getsnr(vap, &snr_info, WLAN_SNR_RX);

    if (ieee80211_get_chan_nf(vap->iv_ic, &nf_val) < 0) {
        qdf_err("failed to get chan_nf");
        return -EINVAL;
    }

    set_quality(iq, snr_info.avg_snr, nf_val);

    return 0;
}

u_int32_t ieee80211_ucfg_get_maxphyrate(wlan_if_t vaphandle)
{
 struct ieee80211vap *vap = vaphandle;
 struct ieee80211com *ic = vap->iv_ic;

 if (!vap->iv_bss)
     return 0;

 /* Rate should show 0 if VAP is not UP.
  * Rates are returned as Kbps to avoid
  * signed overflow when using HE modes
  * or larger values of NSS.
  * All applications should handle this.
  */
 return((wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS) ? 0: ic->ic_get_maxphyrate(ic, vap->iv_bss));
}

#define IEEE80211_MODE_TURBO_STATIC_A   IEEE80211_MODE_MAX
/**
 * ieee80211_is_mode_lower: Check if the given mode is lower mode between
 * 11be or 11ax. Enhance this in future if repurpose feature adds new specs.
 * @vap: Pointer to the vap
 * @mode: Mode to be checked
 *
 * Return: True for lower mode
 */
bool ieee80211_is_mode_lower(wlan_if_t vap, enum ieee80211_phymode mode)
{
    return (ieee80211_is_phymode_11ax(mode) || ieee80211_is_phymode_11ac(mode) || ieee80211_is_phymode_11ng(mode));
}

/**
 * ieee80211_is_mode_higher: Check if the given mode is higher mode between
 * 11be or 11ax. Enhance this in future if repurpose feature adds new specs.
 * @vap: Pointer to the vap
 * @mode: Mode to be checked
 *
 * Return: True for higher mode
 */
bool ieee80211_is_mode_higher(wlan_if_t vap, enum ieee80211_phymode mode)
{
#ifdef WLAN_FEATURE_11BE
    return ieee80211_is_phymode_11be(mode);
#else
    return FALSE;
#endif
}

/**
 * ieee80211_get_new_phymode: Get new phymode with given mode and bandwidth.
 * @mode: Old mode from which BW is taken
 * @change_mode: New mode to derive the phymode
 *
 * Return: New phymode
 */
enum ieee80211_phymode ieee80211_get_new_phymode(enum ieee80211_phymode mode, enum ieee80211_mode change_mode)
{
    enum ieee80211_cwm_width bw;
    int8_t sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCN;

    bw = get_chwidth_phymode(mode);
#ifdef WLAN_FEATURE_11BE
    if (bw == IEEE80211_CWM_WIDTH320)
        bw = IEEE80211_CWM_WIDTH160;
#endif
    if (change_mode == IEEE80211_MODE_AX) {
        if (ieee80211_is_phymode_2g(mode))
            change_mode = IEEE80211_MODE_AXG;
        else
            change_mode = IEEE80211_MODE_AXA;
    }

    if (ieee80211_is_phymode_40plus(mode))
        sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCA;
    else
        sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCB;

    return ieee80211_get_composite_phymode(change_mode, bw, sec_offset);
}

#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_get_derived_mode: Derive the phymode with given old phymode, new
 * mode and channel.
 * @vap: Pointer to the vap
 * @mode: Old phymode
 * @change_mode: New mode
 * @nchan_p: Channel parameters
 *
 * Return: New derived phymode
 */
enum ieee80211_phymode ieee80211_get_derived_mode(wlan_if_t vap, enum ieee80211_phymode mode, enum ieee80211_mode change_mode, struct chan_params *nchan_p)
{
    enum ieee80211_phymode res = IEEE80211_MODE_AUTO;
    struct ieee80211com *ic = vap->iv_ic;
    struct chan_params chan_p = {0};
    struct ieee80211vap *tmpvap = NULL;
    struct ieee80211_ath_channel chan_obj, res_chan;
    struct ieee80211_ath_channel *nchan = &chan_obj;

    if ((change_mode == IEEE80211_MODE_AX) || (change_mode == IEEE80211_MODE_AC) || (change_mode == IEEE80211_MODE_NG)) {
        int found = 0;
        if (ieee80211_is_mode_lower(vap, mode)) {
            qdf_err("Invalid mode passed\n");
            return res;
        }

        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (tmpvap == vap)
                continue;

            if (ieee80211_is_mode_higher(tmpvap, tmpvap->iv_des_mode) && (wlan_vdev_mlme_is_active(tmpvap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE, "%s: Existing 11be vap found", __func__);
                found = 1;
                break;
            }
        }
        if (found) {
            chan_p.freq = ieee80211_chan2freq(ic, tmpvap->iv_bsschan);
            chan_p.cfreq2 = tmpvap->iv_bsschan->ic_vhtop_freq_seg2;
            set_chan_params_puncture_bitmap(&chan_p, tmpvap->iv_bsschan->op_puncture_bitmap);
            if (nchan_p)
                chan_p = *nchan_p;
            *nchan = ieee80211_find_dot11_channel(ic, &chan_p, mode);
            if (IEEE80211_IS_CHAN_PUNCTURED(nchan)) {
                res_chan = ieee80211_find_legacy_non11be_channel(ic, nchan);
                res = ieee80211_chan2mode(&res_chan);
                res = ieee80211_get_new_phymode(res, change_mode);
                goto end;
            }
        }

        if (nchan_p) {
            if (ieee80211_is_mode_higher(vap, mode)) {
                *nchan = ieee80211_find_dot11_channel(ic, nchan_p, mode);
            } else {
                *nchan = ieee80211_find_dot11_channel(ic, nchan_p, IEEE80211_MODE_11BEA_EHT160);
            }
            if (IEEE80211_IS_CHAN_PUNCTURED(nchan)) {
                res_chan = ieee80211_find_legacy_non11be_channel(ic, nchan);
                res = ieee80211_chan2mode(&res_chan);
                res = ieee80211_get_new_phymode(res, change_mode);
                goto end;
            }
        }

        if (ieee80211_is_mode_higher(vap, mode)) {
            res = ieee80211_get_new_phymode(mode, change_mode);
        } else {
            res = ieee80211_get_new_phymode(IEEE80211_MODE_11BEA_EHT160, change_mode);
        }
    } else if (change_mode == IEEE80211_MODE_BE) {
        enum ieee80211_cwm_width bw = get_chwidth_phymode(mode);
        int8_t sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCN;

        if (ieee80211_is_mode_higher(vap, mode)) {
            qdf_err("Invalid mode passed\n");
            return res;
        }

        if (ieee80211_is_phymode_40(mode)) {
            if (ieee80211_is_phymode_40plus(mode))
                sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCA;
            else
                sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCB;
        }
        if (ieee80211_is_phymode_2g(mode))
            change_mode = IEEE80211_MODE_BEG;
        else
            change_mode = IEEE80211_MODE_BEA;
        res = ieee80211_get_composite_phymode(change_mode, bw, sec_offset);
    }

end:
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE, "Derived mode from %s is %d", __func__, res);
    return res;
}
#endif

/**
 * ieee80211_mixed_modes_config: Check if radio will be in different modes.
 * @vap: Pointer to the vap
 * @mode: New phymode for the vap
 *
 * Return: Lower mode if different modes are present
 */
enum ieee80211_mode ieee80211_mixed_modes_config(wlan_if_t vap, enum ieee80211_phymode mode)
{
#ifdef WLAN_FEATURE_11BE
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211vap *tmpvap = NULL;
    int vap_11be = 0, vap_11ax = 0, vap_11ac = 0, vap_11ng = 0;

    if (ieee80211_is_phymode_11be(mode))
        vap_11be = 1;
    else if (ieee80211_is_phymode_11ax(mode))
        vap_11ax = 1;
    else if (ieee80211_is_phymode_11ac(mode))
        vap_11ac = 1;
    else if (ieee80211_is_phymode_11ng(mode))
        vap_11ng = 1;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if (tmpvap == vap)
            continue;

        if (ieee80211_is_phymode_11be(tmpvap->iv_des_mode))
            vap_11be = 1;
        else if (ieee80211_is_phymode_11ax(tmpvap->iv_des_mode))
            vap_11ax = 1;
        else if (ieee80211_is_phymode_11ac(tmpvap->iv_des_mode))
            vap_11ac = 1;
        else if (ieee80211_is_phymode_11ng(tmpvap->iv_des_mode))
            vap_11ng = 1;
        if (vap_11be && vap_11ax)
            return IEEE80211_MODE_AX;
        if (vap_11be && vap_11ac)
            return IEEE80211_MODE_AC;
        if (vap_11be && vap_11ng)
            return IEEE80211_MODE_NG;
    }
#endif
    return IEEE80211_MODE_INVALID;
}

#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_repurposed_mode: This function is used to find the repurposed
 * mode for the vap based on user input.
 * @vap: Pointer to the vap
 * @mode: New mode
 *
 * Return: New phymode
 */
enum ieee80211_phymode ieee80211_repurposed_mode(wlan_if_t vap, const char *mode)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211vap *tmpvap = NULL;

    if (!strncmp(mode, "11BE", 4) || !strncmp(mode, "11be", 4)) {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (ieee80211_is_phymode_11be(tmpvap->iv_cur_mode))
                return tmpvap->iv_cur_mode;
        }
        return ieee80211_get_derived_mode(vap, vap->iv_cur_mode, IEEE80211_MODE_BE, NULL);
    } else if (!strncmp(mode, "11AX", 4) || !strncmp(mode, "11ax", 4)) {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (ieee80211_is_phymode_11ax(tmpvap->iv_cur_mode))
                return tmpvap->iv_cur_mode;
        }
        return ieee80211_get_derived_mode(vap, vap->iv_cur_mode, IEEE80211_MODE_AX, NULL);
    } else if (!strncmp(mode, "11AC", 4) || !strncmp(mode, "11ac", 4)) {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (ieee80211_is_phymode_11ac(tmpvap->iv_cur_mode))
                return tmpvap->iv_cur_mode;
        }
        return ieee80211_get_derived_mode(vap, vap->iv_cur_mode, IEEE80211_MODE_AC, NULL);
    } else {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (ieee80211_is_phymode_11ng(tmpvap->iv_cur_mode))
                return tmpvap->iv_cur_mode;
        }
        return ieee80211_get_derived_mode(vap, vap->iv_cur_mode, IEEE80211_MODE_NG, NULL);
    }
}
#endif

int ieee80211_convert_mode(const char *mode)
{
#define TOUPPER(c) ((((c) > 0x60) && ((c) < 0x7b)) ? ((c) - 0x20) : (c))
    static const struct
    {
        char *name;
        int mode;
    } mappings[] = {
        /* NB: need to order longest strings first for overlaps */
        { "11AST" , IEEE80211_MODE_TURBO_STATIC_A },
        { "AUTO"  , IEEE80211_MODE_AUTO },
        { "11A"   , IEEE80211_MODE_11A },
        { "11B"   , IEEE80211_MODE_11B },
        { "11G"   , IEEE80211_MODE_11G },
        { "FH"    , IEEE80211_MODE_FH },
		{ "0"     , IEEE80211_MODE_AUTO },
		{ "1"     , IEEE80211_MODE_11A },
		{ "2"     , IEEE80211_MODE_11B },
		{ "3"     , IEEE80211_MODE_11G },
		{ "4"     , IEEE80211_MODE_FH },
		{ "5"     , IEEE80211_MODE_TURBO_STATIC_A },
	    { "TA"      , IEEE80211_MODE_TURBO_A },
	    { "TG"      , IEEE80211_MODE_TURBO_G },
	    { "11NAHT20"      , IEEE80211_MODE_11NA_HT20 },
	    { "11NGHT20"      , IEEE80211_MODE_11NG_HT20 },
	    { "11NAHT40PLUS"  , IEEE80211_MODE_11NA_HT40PLUS },
	    { "11NAHT40MINUS" , IEEE80211_MODE_11NA_HT40MINUS },
	    { "11NGHT40PLUS"  , IEEE80211_MODE_11NG_HT40PLUS },
	    { "11NGHT40MINUS" , IEEE80211_MODE_11NG_HT40MINUS },
        { "11NGHT40" , IEEE80211_MODE_11NG_HT40},
        { "11NAHT40" , IEEE80211_MODE_11NA_HT40},
        { "11ACVHT20", IEEE80211_MODE_11AC_VHT20},
        { "11ACVHT40PLUS", IEEE80211_MODE_11AC_VHT40PLUS},
        { "11ACVHT40MINUS", IEEE80211_MODE_11AC_VHT40MINUS},
        { "11ACVHT40", IEEE80211_MODE_11AC_VHT40},
        { "11ACVHT80", IEEE80211_MODE_11AC_VHT80},
        { "11ACVHT160", IEEE80211_MODE_11AC_VHT160},
        { "11ACVHT80_80", IEEE80211_MODE_11AC_VHT80_80},
        { "11AHE20" , IEEE80211_MODE_11AXA_HE20},
        { "11GHE20" , IEEE80211_MODE_11AXG_HE20},
        { "11AHE40PLUS" , IEEE80211_MODE_11AXA_HE40PLUS},
        { "11AHE40MINUS" , IEEE80211_MODE_11AXA_HE40MINUS},
        { "11GHE40PLUS" , IEEE80211_MODE_11AXG_HE40PLUS},
        { "11GHE40MINUS" , IEEE80211_MODE_11AXG_HE40MINUS},
        { "11AHE40" , IEEE80211_MODE_11AXA_HE40},
        { "11GHE40" , IEEE80211_MODE_11AXG_HE40},
        { "11AHE80" , IEEE80211_MODE_11AXA_HE80},
        { "11AHE160" , IEEE80211_MODE_11AXA_HE160},
        { "11AHE80_80" , IEEE80211_MODE_11AXA_HE80_80},
#ifdef WLAN_FEATURE_11BE
        { "11AEHT20" , IEEE80211_MODE_11BEA_EHT20},
        { "11GEHT20" , IEEE80211_MODE_11BEG_EHT20},
        { "11AEHT40PLUS" , IEEE80211_MODE_11BEA_EHT40PLUS},
        { "11AEHT40MINUS" , IEEE80211_MODE_11BEA_EHT40MINUS},
        { "11GEHT40PLUS" , IEEE80211_MODE_11BEG_EHT40PLUS},
        { "11GEHT40MINUS" , IEEE80211_MODE_11BEG_EHT40MINUS},
        { "11AEHT40" , IEEE80211_MODE_11BEA_EHT40},
        { "11GEHT40" , IEEE80211_MODE_11BEG_EHT40},
        { "11AEHT80" , IEEE80211_MODE_11BEA_EHT80},
        { "11AEHT160" , IEEE80211_MODE_11BEA_EHT160},
        { "11AEHT320" , IEEE80211_MODE_11BEA_EHT320},
#endif /* WLAN_FEATURE_11BE */
        { NULL }
    };
    int i, j;
    const char *cp;

    for (i = 0; mappings[i].name != NULL; i++) {
        cp = mappings[i].name;
        for (j = 0; j < strlen(mode) + 1; j++) {
            /* convert user-specified string to upper case */
            if (TOUPPER(mode[j]) != cp[j])
                break;
            if (cp[j] == '\0')
                return mappings[i].mode;
        }
    }
    return -1;
#undef TOUPPER
}

int ieee80211_ucfg_set_phymode(wlan_if_t vap, char *modestr, int len, bool reset_vap)
{
    char s[30];      /* big enough for ``11nght40plus'' */
    int mode;
    uint32_t ldpc;
    int retval;
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    enum ieee80211_phymode preset_vap_mode = vap->iv_cur_mode;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;

    ic = vap->iv_ic;
    scn = OL_ATH_SOFTC_NET80211(ic);

    /* truncate the len if it shoots our buffer.
     * len does not include '\0'
     */
    if (len >= sizeof(s)) {
        len = sizeof(s) - 1;
    }

    /* Copy upto len characters and a '\0' enforced by strlcpy */
    if (strlcpy(s, modestr, len + 1) > sizeof(s)) {
        qdf_print("String too long to copy");
        return -EINVAL;
    }

    /*
    ** Convert mode name into a specific mode
    */

    if (vap->iv_repurpose) {
#ifdef WLAN_FEATURE_11BE_MLO
        struct qdf_mac_addr *mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
#endif
        osif_dev *osifp = (osif_dev *)vap->iv_ifp;
        struct net_device *dev = get_cfg80211_notification_ndev(osifp);

        vdev = vap->vdev_obj;
        if (!vdev) {
          qdf_info("vdev is NULL");
          return 0;
        }

        psoc = wlan_vdev_get_psoc(vdev);
        if (!psoc) {
          qdf_info("psoc is NULL");
          return 0;
        }
        /* Make sure vap is brough down before command is given to repurpose it*/
        if (dev && !(dev->flags & IFF_RUNNING))
            vap_wait_for_vdev_init_state(psoc, vdev, 0);

#ifdef WLAN_FEATURE_11BE
        if (((!strncmp(s, "11AX", 4) || !strncmp(s, "11ax", 4)) && ieee80211_is_phymode_11ax(vap->iv_cur_mode)) || ((!strncmp(s, "11BE", 4) || !strncmp(s, "11be", 4)) && ieee80211_is_phymode_11be(vap->iv_cur_mode)) ||
            ((!strncmp(s, "11AC", 4) || !strncmp(s, "11ac", 4)) && ieee80211_is_phymode_11ac(vap->iv_cur_mode)) || ((!strncmp(s, "11NG", 4) || !strncmp(s, "11ng", 4)) && ieee80211_is_phymode_11ng(vap->iv_cur_mode))) {
            qdf_err("Current mode of the vap is already %s\n",s);
            return 0;
        }
#ifdef WLAN_FEATURE_11BE_MLO
        /* If VAP MLD address is valid, but vdev MLD address is invalid, add VAP to MLD */
        if (qdf_is_macaddr_zero(mld_addr) && !qdf_is_macaddr_zero(&vap->mld_mac_addr) &&
            (!strncmp(s, "11BE", 4) || !strncmp(s, "11be", 4))) {
            qdf_info("Attaching link VDEV to MLD %s", vap->iv_netdev_name);
            if (wlan_mlo_mgr_mld_vdev_attach(vap->vdev_obj, &vap->mld_mac_addr)) {
                qdf_err("Link reconfig add link VDEV to MLD %s is failed", vap->iv_netdev_name);
                return -EINVAL;
            }
            if (dev)
                osif_mldev_reconfig_link_add(dev, (uint8_t *)&vap->mld_mac_addr, vap->link_id);
        }

        /* If vdev MLD address is valid and desired mode is 11ax or 11ac/ng, remove VAP from mld */
        if (!qdf_is_macaddr_zero(mld_addr) && (!strncmp(s, "11AX", 4) || !strncmp(s, "11ax", 4) || !strncmp(s, "11AC", 4) || !strncmp(s, "11ac", 4) || !strncmp(s, "11NG", 4) || !strncmp(s, "11ng", 4))) {
            qdf_info("Detaching link VDEV from MLD %s", vap->iv_netdev_name);
            if (wlan_mlo_mgr_mld_vdev_detach(vap->vdev_obj)) {
                qdf_err("Link reconfig remove link VDEV from MLD %s is failed", vap->iv_netdev_name);
                return -EINVAL;
            }
            if (dev)
                osif_mldev_reconfig_link_delete(dev, vap->link_id);
        }

        if (ieee80211_is_phymode_auto(vap->iv_cur_mode)) {
            return 0;
        }
#endif /* WLAN_FEATURE_11BE_MLO */
        mode = ieee80211_repurposed_mode(vap, s);
#else
        qdf_err("Repurpose feature not supported on non-11be\n");
        return -EINVAL;
#endif
    } else {
        mode = ieee80211_convert_mode(s);
    }
    if (mode < 0)
        return -EINVAL;

    /* if ldpc is disabled then as per 802.11ax
     * specification, D2.0 (section 28.1.1) we
     * can not allow mode greater than 20 MHz
     */
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj, WLAN_MLME_CFG_LDPC, &ldpc);
    if ((ldpc == IEEE80211_HTCAP_C_LDPC_NONE) &&
            (ieee80211_is_phymode_equal_or_above_11axa_he40plus(mode)
#ifdef WLAN_FEATURE_11BE
             || ieee80211_is_phymode_equal_or_above_11bea_eht40plus(mode)
#endif
            )) {
        qdf_print("Mode %s is not allowed if LDPC is "
                "already disabled", s);
        return -EINVAL;
    }

    retval = ieee80211_ucfg_set_wirelessmode(vap, mode);
    if (retval) {
        qdf_err("Set phymode failed");
        return retval;
    }

    /* If there is no channel set,  ACS will be triggered later */
    *chan = wlan_get_current_channel(vap, false);
    if ((ieee80211_convert_mode(s) == preset_vap_mode) ||
            !IEEE80211_IS_CHAN_VALID(chan) ||
            ieee80211_is_phymode_auto(preset_vap_mode))
       return retval;

    ic = vap->iv_ic;

    /* Restart all the vaps to make the change take into effect */
    if (reset_vap) {
        if (osif_restart_for_config(ic, NULL, NULL))
            return -EFAULT;
    }

    if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL)) {

        wlan_if_t tmpvap;
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if(tmpvap) {
                struct net_device *tmpdev;
                tmpdev = get_cfg80211_notification_ndev((osif_dev *)tmpvap->iv_ifp);
                if (!IS_UP(tmpdev)) {
                    IEEE80211_DELIVER_EVENT_CHANNEL_CHANGE(tmpvap, tmpvap->iv_des_chan[tmpvap->iv_des_mode]);
                }
            }
        }
    }

    return retval;
}

int ieee80211_ucfg_set_wirelessmode(wlan_if_t vap, int mode)
{
    struct ieee80211com *ic = vap->iv_ic;

    /* OBSS scanning should only be enabled in 40 Mhz 2.4G */

    /* 11AX TODO: Recheck future 802.11ax drafts (>D1.0) on coex rules */
    if (ieee80211_is_phymode_g40(mode)) {
        if (!ic->ic_user_coext_disable) {
            struct ieee80211vap *tmpvap = NULL;
            bool width40_vap_found = false;
            /*Check if already any VAP is configured with HT40 mode*/
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                if((vap != tmpvap) && ieee80211_is_phymode_g40(tmpvap->iv_des_mode)) {
                    width40_vap_found = true;
                    break;
                }
            }
            /*
             * If any VAP is already configured with 40 width,
             * no need to clear disable coext flag,
             * as disable coext flag may be set by other VAP
             */
            if(!width40_vap_found)
                ieee80211com_clear_flags(ic, IEEE80211_F_COEXT_DISABLE);
        }
    } else {
        ieee80211com_set_flags(ic, IEEE80211_F_COEXT_DISABLE);
    }

    return wlan_set_desired_phymode(vap, mode);
}

/*
* Get a key index from a request.  If nothing is
* specified in the request we use the current xmit
* key index.  Otherwise we just convert the index
* to be base zero.
*/
static int getkeyix(wlan_if_t vap, u_int16_t flags, u_int16_t *kix)
{
    int kid;

    kid = flags & IW_ENCODE_INDEX;
    if (kid < 1 || kid > IEEE80211_WEP_NKID)
    {
        kid = wlan_get_default_keyid(vap);
        if (kid == IEEE80211_KEYIX_NONE)
            kid = 0;
    }
    else
        --kid;
    if (0 <= kid && kid < IEEE80211_WEP_NKID)
    {
        *kix = kid;
        return 0;
    }
    else
        return -EINVAL;
}

/*
 * If authmode = IEEE80211_AUTH_OPEN, script apup would skip authmode setup.
 * Do default authmode setup here for OPEN mode.
 */
static int sencode_wep(struct net_device *dev)
{
    osif_dev            *osifp = ath_netdev_priv(dev);
    wlan_if_t           vap    = osifp->os_if;
    int                 error  = 0;
    error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE, 1 << WLAN_CRYPTO_AUTH_OPEN);
    if (error == 0 ) {
        error = wlan_set_param(vap, IEEE80211_FEATURE_PRIVACY, 0);
        osifp->uciphers[0] = osifp->mciphers[0] = IEEE80211_CIPHER_NONE;
        osifp->u_count = osifp->m_count = 1;
    }

    return IS_UP(dev) ? -osif_vap_init(osifp, dev, RESCAN) : 0;
}

int ieee80211_ucfg_set_encode(wlan_if_t vap, u_int16_t length, u_int16_t flags, void *keybuf)
{
    osif_dev *osifp = (osif_dev *)vap->iv_ifp;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    ieee80211_keyval key_val;
    u_int16_t kid;
    int error = -EOPNOTSUPP;
    u_int8_t keydata[IEEE80211_KEYBUF_SIZE];
    int wepchange = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t link_id = vap->link_id;
#else
    uint8_t link_id = INVALID_LINK_ID;
#endif

    if (!dev)
        return -EINVAL;

    if (ieee80211_crypto_wep_mbssid_enabled())
        wlan_set_param(vap, IEEE80211_WEP_MBSSID, 1);  /* wep keys will start from 4 in keycache for support wep multi-bssid */
    else
        wlan_set_param(vap, IEEE80211_WEP_MBSSID, 0);  /* wep keys will allocate index 0-3 in keycache */

    if ((flags & IW_ENCODE_DISABLED) == 0)
    {
        /*
         * Enable crypto, set key contents, and
         * set the default transmit key.
         */
        error = getkeyix(vap, flags, &kid);
        if (error)
            return error;
        if (length > IEEE80211_KEYBUF_SIZE)
            return -EINVAL;

        /* XXX no way to install 0-length key */
        if (length > 0)
        {

            /* WEP key length should be 40,104, 128 bits only */
            if(!((length == IEEE80211_KEY_WEP40_LEN) ||
                        (length == IEEE80211_KEY_WEP104_LEN) ||
                        (length == IEEE80211_KEY_WEP128_LEN)))
            {

                IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO, "WEP key is rejected due to key of length %d\n", length);
                osif_ioctl_delete_vap(dev, link_id);
                return -EINVAL;
            }

            /*
             * ieee80211_match_rsn_info() IBSS mode need.
             * Otherwise, it caused crash when tx frame find tx rate
             *   by node RateControl info not update.
             */
            if (osifp->os_opmode == IEEE80211_M_IBSS) {
                /* set authmode to IEEE80211_AUTH_OPEN */
                sencode_wep(dev);

                /* set keymgmtset to WLAN_ASE_NONE */
         wlan_crypto_set_vdev_param(vap->vdev_obj,
				  WLAN_CRYPTO_PARAM_KEY_MGMT,
					WLAN_ASE_NONE);
            }

            qdf_mem_copy(keydata, keybuf, length);
            qdf_mem_zero(&key_val, sizeof(ieee80211_keyval));
            key_val.keytype = IEEE80211_CIPHER_WEP;
            key_val.keydir = IEEE80211_KEY_DIR_BOTH;
            key_val.keylen = length;
            key_val.keydata = keydata;
            key_val.macaddr = (u_int8_t *)ieee80211broadcastaddr;

            if (wlan_set_key(vap, kid, &key_val) != 0) {
                /* Zero-out local key variables */
                qdf_mem_zero(keydata, IEEE80211_KEYBUF_SIZE);
                qdf_mem_zero(&key_val, sizeof(ieee80211_keyval));
                return -EINVAL;
            }
        }
        else
        {
            /*
             * When the length is zero the request only changes
             * the default transmit key.  Verify the new key has
             * a non-zero length.
             */
            if ( wlan_set_default_keyid(vap,kid) != 0  ) {
                qdf_print("\n Invalid Key is being Set. Bringing VAP down! ");
                osif_ioctl_delete_vap(dev, link_id);
                return -EINVAL;
            }
        }
        if (error == 0)
        {
            /*
             * The default transmit key is only changed when:
             * 1. Privacy is enabled and no key matter is
             *    specified.
             * 2. Privacy is currently disabled.
             * This is deduced from the iwconfig man page.
             */
            if (length == 0 ||
                    (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY)) == 0)
                wlan_set_default_keyid(vap,kid);
            wepchange = (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY)) == 0;
            wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY, 1);
        }
    }
    else
    {
        if (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY) == 0)
            return 0;
        wlan_set_param(vap,IEEE80211_FEATURE_PRIVACY, 0);
        wepchange = 1;
        error = 0;
    }
    if (error == 0)
    {
        /* Set policy for unencrypted frames */
        if ((flags & IW_ENCODE_OPEN) &&
                (!(flags & IW_ENCODE_RESTRICTED)))
        {
            wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 0);
        }
        else if (!(flags & IW_ENCODE_OPEN) &&
                (flags & IW_ENCODE_RESTRICTED))
        {
            wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 1);
        }
        else
        {
            /* Default policy */
            if (wlan_get_param(vap,IEEE80211_FEATURE_PRIVACY))
                wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 1);
            else
                wlan_set_param(vap,IEEE80211_FEATURE_DROP_UNENC, 0);
        }
    }
    if (error == 0 && IS_UP(dev) && wepchange)
    {
        /*
         * Device is up and running; we must kick it to
         * effect the change.  If we're enabling/disabling
         * crypto use then we must re-initialize the device
         * so the 802.11 state machine is reset.  Otherwise
         * the key state should have been updated above.
         */

        error = osif_vap_init(osifp, dev, RESCAN);
    }
    return error;
}

int ieee80211_ucfg_set_rate(wlan_if_t vap, int value)
{
    int retv;

    retv = wlan_set_param(vap, IEEE80211_FIXED_RATE, value);
    if (EOK == retv) {
        if (value != IEEE80211_FIXED_RATE_NONE) {
            /* set default retries when setting fixed rate */
            retv = wlan_set_param(vap, IEEE80211_FIXED_RETRIES, 4);
        }
        else {
            retv = wlan_set_param(vap, IEEE80211_FIXED_RETRIES, 0);
        }
    }
    return retv;
}

#define IEEE80211_MODE_TURBO_STATIC_A   IEEE80211_MODE_MAX
int ieee80211_ucfg_get_phymode(wlan_if_t vap, char *modestr, u_int16_t *length, int type)
{
    enum ieee80211_phymode  phymode;

    if(type == CURR_MODE){
        phymode = wlan_get_current_phymode(vap);
    }else if(type == PHY_MODE){
        phymode = wlan_get_desired_phymode(vap);
    }else{
        IEEE80211_DPRINTF(vap,  IEEE80211_MSG_ANY, "Function %s should be called with a valid type \n ",__func__);
        return -EINVAL;
    }

    ieee80211_convert_phymode_to_string(phymode, modestr, length);
    return 0;
}

void ieee80211_convert_phymode_to_string(enum ieee80211_phymode  phymode,
                                           char *modestr, u_int16_t *length)
{
    int i;
    static const struct
    {
        char *name;
        int mode;
    } mappings[] = {
        /* NB: need to order longest strings first for overlaps */
        { "11AST" , IEEE80211_MODE_TURBO_STATIC_A },
        { "AUTO"  , IEEE80211_MODE_AUTO },
        { "11A"   , IEEE80211_MODE_11A },
        { "11B"   , IEEE80211_MODE_11B },
        { "11G"   , IEEE80211_MODE_11G },
        { "FH"    , IEEE80211_MODE_FH },
        { "TA"      , IEEE80211_MODE_TURBO_A },
        { "TG"      , IEEE80211_MODE_TURBO_G },
        { "11NAHT20"        , IEEE80211_MODE_11NA_HT20 },
        { "11NGHT20"        , IEEE80211_MODE_11NG_HT20 },
        { "11NAHT40PLUS"    , IEEE80211_MODE_11NA_HT40PLUS },
        { "11NAHT40MINUS"   , IEEE80211_MODE_11NA_HT40MINUS },
        { "11NGHT40PLUS"    , IEEE80211_MODE_11NG_HT40PLUS },
        { "11NGHT40MINUS"   , IEEE80211_MODE_11NG_HT40MINUS },
        { "11NGHT40"        , IEEE80211_MODE_11NG_HT40},
        { "11NAHT40"        , IEEE80211_MODE_11NA_HT40},
        { "11ACVHT20"       , IEEE80211_MODE_11AC_VHT20},
        { "11ACVHT40PLUS"   , IEEE80211_MODE_11AC_VHT40PLUS},
        { "11ACVHT40MINUS"  , IEEE80211_MODE_11AC_VHT40MINUS},
        { "11ACVHT40"       , IEEE80211_MODE_11AC_VHT40},
        { "11ACVHT80"       , IEEE80211_MODE_11AC_VHT80},
        { "11ACVHT160"      , IEEE80211_MODE_11AC_VHT160},
        { "11ACVHT80_80"    , IEEE80211_MODE_11AC_VHT80_80},
        { "11AHE20"         , IEEE80211_MODE_11AXA_HE20},
        { "11GHE20"         , IEEE80211_MODE_11AXG_HE20},
        { "11AHE40PLUS"     , IEEE80211_MODE_11AXA_HE40PLUS},
        { "11AHE40MINUS"    , IEEE80211_MODE_11AXA_HE40MINUS},
        { "11GHE40PLUS"     , IEEE80211_MODE_11AXG_HE40PLUS},
        { "11GHE40MINUS"    , IEEE80211_MODE_11AXG_HE40MINUS},
        { "11AHE40"         , IEEE80211_MODE_11AXA_HE40},
        { "11GHE40"         , IEEE80211_MODE_11AXG_HE40},
        { "11AHE80"         , IEEE80211_MODE_11AXA_HE80},
        { "11AHE160"        , IEEE80211_MODE_11AXA_HE160},
        { "11AHE80_80"      , IEEE80211_MODE_11AXA_HE80_80},
#ifdef WLAN_FEATURE_11BE
        { "11AEHT20"        , IEEE80211_MODE_11BEA_EHT20},
        { "11GEHT20"        , IEEE80211_MODE_11BEG_EHT20},
        { "11AEHT40PLUS"    , IEEE80211_MODE_11BEA_EHT40PLUS},
        { "11AEHT40MINUS"   , IEEE80211_MODE_11BEA_EHT40MINUS},
        { "11GEHT40PLUS"    , IEEE80211_MODE_11BEG_EHT40PLUS},
        { "11GEHT40MINUS"   , IEEE80211_MODE_11BEG_EHT40MINUS},
        { "11AEHT40"        , IEEE80211_MODE_11BEA_EHT40},
        { "11GEHT40"        , IEEE80211_MODE_11BEG_EHT40},
        { "11AEHT80"        , IEEE80211_MODE_11BEA_EHT80},
        { "11AEHT160"       , IEEE80211_MODE_11BEA_EHT160},
        { "11AEHT320"       , IEEE80211_MODE_11BEA_EHT320},
#endif /* WLAN_FEATURE_11BE */
        { NULL }
    };

    for (i = 0; mappings[i].name != NULL ; i++)
    {
        if (phymode == mappings[i].mode)
        {
            *length = strlen(mappings[i].name);
            strlcpy(modestr, mappings[i].name, *length + 1);
            break;
        }
    }
}

void ieee80211_ucfg_get_opmode(wlan_if_t vap, char *modestr, u_int16_t *length)
{
    int i;
    enum ieee80211_opmode opmode = vap->iv_opmode;
    static const struct
    {
        char *name;
        int mode;
    } mappings[] = {
        {"STA", IEEE80211_M_STA},
        {"IBSS", IEEE80211_M_IBSS},
        {"AHDEMO", IEEE80211_M_AHDEMO},
        {"HOSTAP", IEEE80211_M_HOSTAP},
        {"MONITOR", IEEE80211_M_MONITOR},
        {"WDS", IEEE80211_M_WDS},
        {"BTAMP", IEEE80211_M_BTAMP},
        {"P2P_GO", IEEE80211_M_P2P_GO},
        {"P2P_CLIENT", IEEE80211_M_P2P_CLIENT},
        {"P2P_DEVICE", IEEE80211_M_P2P_DEVICE},
        {"OPMODE_MAX", IEEE80211_OPMODE_MAX},
        {"ANY", IEEE80211_M_ANY},
        {NULL}
    };

    for (i = 0; mappings[i].name != NULL ; i++)
    {
        if (opmode == mappings[i].mode)
        {
            strlcpy(modestr, mappings[i].name, *length-1);
            *length = strlen(modestr);
            return;
        }
    }
    *length = 0;
}
#undef IEEE80211_MODE_TURBO_STATIC_A

static size_t
sta_space(const wlan_node_t node, size_t *ielen, wlan_if_t vap)
{
    u_int8_t    ni_ie[IEEE80211_MAX_OPT_IE];
    u_int16_t ni_ie_len = IEEE80211_MAX_OPT_IE;
    u_int8_t *macaddr = wlan_node_getmacaddr(node);
    *ielen = 0;

#ifdef WLAN_FEATURE_11BE_MLO
    /* Do not send bridge peer details */
    if (wlan_peer_get_peer_type(node->peer_obj) == WLAN_PEER_MLO_BRIDGE)
        return 0;
#endif

    if(!wlan_node_getwpaie(vap, macaddr, ni_ie, &ni_ie_len)) {
        *ielen += ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if(!wlan_node_getwmeie(vap, macaddr, ni_ie, &ni_ie_len)) {
        *ielen += ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if(!wlan_node_getathie(vap, macaddr, ni_ie, &ni_ie_len)) {
        *ielen += ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if(!wlan_node_getwpsie(vap, macaddr, ni_ie, &ni_ie_len)) {
        *ielen += ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if(!wlan_node_get_suppchanie(vap, macaddr, ni_ie, &ni_ie_len)) {
        *ielen += ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if (!wlan_node_get_opclassie(vap, macaddr, ni_ie, &ni_ie_len)) {
        *ielen += ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }

    return roundup(sizeof(struct ieee80211req_sta_info) + *ielen,
        sizeof(u_int32_t));
}

void
get_sta_space(void *arg, wlan_node_t node)
{
    struct stainforeq *req = arg;
    size_t ielen;

    /* already ignore invalid nodes in UMAC */
    req->space += sta_space(node, &ielen, req->vap);
}

uint8_t get_phymode_from_chwidth(struct ieee80211com *ic,
                                        struct ieee80211_node *ni)
{
    enum ieee80211_phymode cur_mode = ni->ni_vap->iv_cur_mode;
    uint8_t curr_phymode = ni->ni_phymode;
    enum node_mode {
        NODE_MODE_LEGACY=0,
        NODE_MODE_HE=1,
        NODE_MODE_VHT=2,
        NODE_MODE_HT=3,
    	NODE_MODE_EHT=4,
    } mode = NODE_MODE_LEGACY;

#ifdef WLAN_FEATURE_11BE
    /* Mode will attain value of 4 for EHT */
    mode = !!(ni->ni_ext_flags & IEEE80211_NODE_EHT) ? NODE_MODE_EHT : NODE_MODE_LEGACY;
#endif /* WLAN_FEATURE_11BE */
    /* Mode will attain value of 1 for HE */
    mode = mode ? mode : !!(ni->ni_ext_flags & IEEE80211_NODE_HE);
    /* Mode will attain value of 2 for VHT and 3 for HT */
    mode = mode ? mode : (!!(ni->ni_flags & IEEE80211_NODE_VHT) ? NODE_MODE_VHT : (!!(ni->ni_flags &
           IEEE80211_NODE_HT) ? NODE_MODE_HT : mode));

    if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan) && !((mode == NODE_MODE_HE)
#ifdef WLAN_FEATURE_11BE
                || (mode == NODE_MODE_EHT)
#endif /* WLAN_FEATURE_11BE */
                )) {
        qdf_alert("Invalid mode %d for 6GHz", mode);
        return curr_phymode;
    }

    if (IEEE80211_IS_CHAN_5GHZ_6GHZ(ic->ic_curchan)) {
        switch(mode) {
#ifdef WLAN_FEATURE_11BE
            /* EHT mode */
            case NODE_MODE_EHT:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20:
                        curr_phymode = IEEE80211_MODE_11BEA_EHT20;
                    break;
                    case IEEE80211_CWM_WIDTH40:
                        curr_phymode = IEEE80211_MODE_11BEA_EHT40;
                    break;
                    case IEEE80211_CWM_WIDTH80:
                        curr_phymode = IEEE80211_MODE_11BEA_EHT80;
                    break;
                    case IEEE80211_CWM_WIDTH160:
                        curr_phymode = IEEE80211_MODE_11BEA_EHT160;
                    break;
                    case IEEE80211_CWM_WIDTH320:
                        curr_phymode = IEEE80211_MODE_11BEA_EHT320;
                    break;
                    default:
                        return curr_phymode;
                    break;
                }
            }
            break;
#endif /* WLAN_FEATURE_11BE */
            /* HE mode */
            case NODE_MODE_HE:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20:
                        curr_phymode = IEEE80211_MODE_11AXA_HE20;
                    break;
                    case IEEE80211_CWM_WIDTH40:
                        curr_phymode = IEEE80211_MODE_11AXA_HE40;
                    break;
                    case IEEE80211_CWM_WIDTH80:
                        curr_phymode = IEEE80211_MODE_11AXA_HE80;
                    break;
                    case IEEE80211_CWM_WIDTH160:
                        return curr_phymode;
                    break;
                    default:
                        return curr_phymode;
                    break;
                }
            }
            break;
            /* VHT mode */
            case NODE_MODE_VHT:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20:
                        curr_phymode = IEEE80211_MODE_11AC_VHT20;
                    break;
                    case IEEE80211_CWM_WIDTH40:
                        curr_phymode = IEEE80211_MODE_11AC_VHT40;
                    break;
                    case IEEE80211_CWM_WIDTH80:
                        curr_phymode = IEEE80211_MODE_11AC_VHT80;
                    break;
                    case IEEE80211_CWM_WIDTH160:
                        if (ieee80211_is_phymode_160_or_320(curr_phymode)) {
                            curr_phymode = IEEE80211_MODE_11AC_VHT160;
                        } else if (ieee80211_is_phymode_8080(curr_phymode)) {
                            curr_phymode = IEEE80211_MODE_11AC_VHT80_80;
                        } else {
                            qdf_print("%s: Warning: Unexpected negotiated "
                            "ni_chwidth=%d for cur_mode=%d. Investigate! "
                            "The system may no longer function "
                            "correctly.",
                            __func__, ni->ni_chwidth, cur_mode);
                            return curr_phymode;
                        }
                    break;
                    default:
                        return curr_phymode;
                    break;
                }
            }
            break;
            /* HT mode */
            case NODE_MODE_HT:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20:
                        curr_phymode = IEEE80211_MODE_11NA_HT20;
                    break;
                    case IEEE80211_CWM_WIDTH40:
                        curr_phymode = IEEE80211_MODE_11NA_HT40;
                    break;
                    default:
                        return curr_phymode;
                    break;
                }
            }
            break;
            default:
                return curr_phymode;
            break;
        }
    } else {
        switch (mode) {
#ifdef WLAN_FEATURE_11BE
            /* EHT mode */
            case NODE_MODE_EHT:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20 :
                        curr_phymode = IEEE80211_MODE_11BEG_EHT20;
                    break;
                    case IEEE80211_CWM_WIDTH40 :
                        curr_phymode = IEEE80211_MODE_11BEG_EHT40;
                    break;
                    default:
                        return curr_phymode;
                    break;
                }
            }
            break;
#endif /* WLAN_FEATURE_11BE */
            /* HE mode */
            case NODE_MODE_HE:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20 :
                        curr_phymode = IEEE80211_MODE_11AXG_HE20;
                    break;
                    case IEEE80211_CWM_WIDTH40 :
                        curr_phymode = IEEE80211_MODE_11AXG_HE40;
                    break;
                    default:
                        return curr_phymode;
                    break;
                }
            }
            break;
            /* HT mode */
            case NODE_MODE_HT:
            {
                switch (ni->ni_chwidth) {
                    case IEEE80211_CWM_WIDTH20 :
                        curr_phymode = IEEE80211_MODE_11NG_HT20;
                    break;
                    case IEEE80211_CWM_WIDTH40 :
                        curr_phymode = IEEE80211_MODE_11NG_HT40;
                    break;
                    default:
                       return curr_phymode;
                    break;
                }
            }
            break;
            default:
                return curr_phymode;
            break;
        }
    }
    return curr_phymode;
}
qdf_export_symbol(get_phymode_from_chwidth);

static bool
get_sta_he_mu_cap_info(uint8_t *hecap_phyinfo)
{
    bool he_mu_cap =  (HECAP_PHY_UL_MU_MIMO_GET_FROM_IE(&hecap_phyinfo) ||
                       /* The ULOFDMA macro is referring to b23 in HE PHY
                        * capability which corresponds to 'Partial BW UL
                        * MUMIMO' support */
                       HECAP_PHY_ULOFDMA_GET_FROM_IE(&hecap_phyinfo) ||
                       HECAP_PHY_DLMUMIMOPARTIALBW_GET_FROM_IE(&hecap_phyinfo));

    return he_mu_cap;
}

void
get_sta_info(void *arg, wlan_node_t node)
{
    struct stainforeq *req = arg;
    wlan_if_t vap = req->vap;
    struct ieee80211req_sta_info *si;
    size_t ielen, len, remaining_space;
    u_int8_t *cp;
    u_int8_t    ni_ie[IEEE80211_MAX_OPT_IE];
    u_int16_t ni_ie_len = IEEE80211_MAX_OPT_IE;
    u_int8_t *macaddr = wlan_node_getmacaddr(node);
    wlan_snr_info snr_info;
    struct ieee80211_ath_channel chan_obj = empty_channel;
    wlan_chan_t chan = &chan_obj;
    ieee80211_rate_info rinfo;
    u_int32_t jiffies_now=0, jiffies_delta=0, jiffies_assoc=0;
    struct wlan_objmgr_psoc *psoc;
    u_int32_t op_class=0, op_rates=0;
    ol_txrx_soc_handle soc_dp_handle;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status;
    cdp_peer_stats_param_t buf = {0};
    struct ni_chan_param ni_param;
    struct chan_params chan_p = {0};
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t i;
    struct mlo_partner_info ml_links = {0};
#endif /* WLAN_FEATURE_11BE_MLO */

    if (!vap || !vap->iv_ic)
        return;

#ifdef WLAN_FEATURE_11BE_MLO
    /* Do not add bridge peer to the STA list */
    if (wlan_peer_get_peer_type(node->peer_obj) == WLAN_PEER_MLO_BRIDGE)
        return;
#endif /* WLAN_FEATURE_11BE_MLO */

    pdev = vap->iv_ic->ic_pdev_obj;
    psoc = wlan_pdev_get_psoc(pdev);

    len = sta_space(node, &ielen, vap);
    if (len > req->space) {
        return;
    }

    if (wlan_node_get_chan_params(node, &ni_param) == QDF_STATUS_SUCCESS) {
        chan_p.freq = ni_param.ni_freq;
        chan_p.cfreq2 = ni_param.ni_cfreq2;
        set_chan_params_puncture_bitmap(&chan_p, ni_param.puncture_bitmap);

        *chan = ieee80211_find_dot11_channel(node->ni_ic,
                                             &chan_p,
                                             ni_param.ni_bss_phymode);

        if (!IEEE80211_IS_CHAN_VALID(chan)) {
            ieee80211_note(vap, IEEE80211_MSG_NODE,
                           "%s, cannot find a valid channel \n", __func__);
            return;
        }
    }

    si = req->si;
    si->awake_time = node->awake_time;
    si->ps_time = node->ps_time;
    /* if node state is currently in power save when the wlanconfig command is given,
       add time from previous_ps_time until current time to power save time */
    if(node->ps_state == 1)
    {
    si->ps_time += qdf_get_system_timestamp() - node->previous_ps_time;
    }
    /* if node state is currently in active state when the wlanconfig command is given,
       add time from previous_ps_time until current time to awake time */
    else if(node->ps_state == 0)
    {
    si->awake_time += qdf_get_system_timestamp() - node->previous_ps_time;
    }
    si->isi_assoc_time = wlan_node_get_assocuptime(node);
    jiffies_assoc = wlan_node_get_assocuptime(node);		/* Jiffies to timespec conversion for si->isi_tr069_assoc_time */
    jiffies_now = OS_GET_TICKS();
    jiffies_delta = jiffies_now - jiffies_assoc;
    si->isi_tr069_assoc_time.tv_sec = (( qdf_system_ticks_to_msecs(jiffies_delta) ) /1000 );
    si->isi_tr069_assoc_time.tv_nsec = qdf_system_ticks_to_nsecs(jiffies_delta);
    si->isi_len = len;
    si->isi_ie_len = ielen;
    si->isi_freq = wlan_channel_frequency(chan);
    si->isi_band = reg_wifi_band_to_wlan_band_id(wlan_reg_freq_to_band(si->isi_freq));
    if(!vap->iv_ic->ic_is_target_lithium){
        qdf_err("ic_is_target_lithium if null");
        return;
    }
    if(vap->iv_ic->ic_is_target_lithium(psoc)){
        if(!vap->iv_ic->ic_get_cur_hw_nf){
            qdf_err("ic_get_cur_hw_nf is null");
            return;
        }
        si->isi_nf = vap->iv_ic->ic_get_cur_hw_nf(vap->iv_ic);
    } else {
        if(!vap->iv_ic->ic_get_cur_chan_nf){
            qdf_err("ic_get_cur_hw_nf is null");
            return;
        }
        si->isi_nf = vap->iv_ic->ic_get_cur_chan_nf(vap->iv_ic);
    }
    si->isi_ieee = wlan_channel_ieee(chan);
    si->isi_flags = wlan_channel_flags(chan);
    si->isi_state = wlan_node_get_state_flag(node);
    si->isi_ps = node->ps_state;
    si->isi_authmode =  wlan_node_get_authmode(node);
    if (wlan_node_getsnr(node, &snr_info, WLAN_SNR_RX) == 0) {
        si->isi_rssi = snr_info.avg_snr;
        si->isi_min_rssi = node->ni_snr_min;
        si->isi_max_rssi = node->ni_snr_max;
    }
    si->isi_capinfo = wlan_node_getcapinfo(node);
    si->isi_pwrcapinfo = wlan_node_getpwrcapinfo(node);
#if UMAC_SUPPORT_RRM
    OS_MEMCPY(si->isi_rrm_caps, node->ni_rrm_caps, sizeof(si->isi_rrm_caps));
#endif
    si->isi_athflags = wlan_node_get_ath_flags(node);
    si->isi_erp = wlan_node_get_erp(node);
    si->isi_operating_bands = wlan_node_get_operating_bands(node);
    si->isi_beacon_measurement_support = wlan_node_has_extflag(node, IEEE80211_NODE_BCN_MEASURE_SUPPORT);
    IEEE80211_ADDR_COPY(si->isi_macaddr, macaddr);

    if (wlan_node_txrate_info(node, &rinfo) == 0) {
        si->isi_txratekbps = rinfo.rate;
        si->isi_maxrate_per_client = rinfo.maxrate_per_client;
        si->isi_avg_txratekbps = rinfo.avg_rate;
#if ATH_EXTRA_RATE_INFO_STA
        si->isi_tx_rate_mcs = rinfo.mcs;
        si->isi_tx_rate_flags = rinfo.flags;
#endif

    }

    /* supported operating classes */
    if (node->ni_supp_op_class_ie != NULL) {
        si->isi_curr_op_class = node->ni_supp_op_cl.curr_op_class;
        si->isi_num_of_supp_class = node->ni_supp_op_cl.num_of_supp_class;
        for(op_class = 0; op_class < node->ni_supp_op_cl.num_of_supp_class &&
            op_class < MAX_NUM_OPCLASS_SUPPORTED; op_class++) {
            si->isi_supp_class[op_class] = node->ni_supp_op_cl.supp_class[op_class];
        }
    }
    else {
          si->isi_num_of_supp_class = 0;
    }

    /* supported channels */
    if (node->ni_supp_chan_ie != NULL) {
        si->isi_first_channel = node->ni_first_channel;
        si->isi_nr_channels = node->ni_nr_channels;
    }
    else {
         si->isi_nr_channels = 0;
    }

    /* supported rates */
    for (op_rates = 0;op_rates < node->ni_rates.rs_nrates;op_rates++) {
         si->isi_rates[op_rates] = node->ni_rates.rs_rates[op_rates];
    }

    memset(&rinfo, 0, sizeof(rinfo));
    if (wlan_node_rxrate_info(node, &rinfo) == 0) {
        si->isi_rxratekbps = rinfo.rate;
	si->isi_avg_rxratekbps = rinfo.avg_rate;
#if ATH_EXTRA_RATE_INFO_STA
        si->isi_rx_rate_mcs = rinfo.mcs;
        si->isi_rx_rate_flags = rinfo.flags;
#endif

    }
    si->isi_associd = wlan_node_get_associd(node);
    si->isi_txpower = wlan_node_get_txpower(node);
    si->isi_vlan = wlan_node_get_vlan(node);
    si->isi_cipher = IEEE80211_CIPHER_NONE;
    if (wlan_get_param(vap, IEEE80211_FEATURE_PRIVACY)) {
        do {
            ieee80211_cipher_type uciphers[1];
            int count = 0;
            count = wlan_node_get_ucast_ciphers(node, uciphers, 1);
            if (count == 1) {
                si->isi_cipher |= 1<<uciphers[0];
            }
        } while (0);
    }
    wlan_node_get_txseqs(node, si->isi_txseqs, sizeof(si->isi_txseqs));
    wlan_node_get_rxseqs(node, si->isi_rxseqs, sizeof(si->isi_rxseqs));
    si->isi_uapsd = wlan_node_get_uapsd(node);
    si->isi_opmode = IEEE80211_STA_OPMODE_NORMAL;

    psoc = wlan_pdev_get_psoc(pdev);
    soc_dp_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_dp_handle)
        return;

    status = cdp_txrx_get_peer_stats_param(soc_dp_handle,
                                     wlan_vdev_get_id(node->peer_obj->peer_objmgr.vdev),
                                     node->peer_obj->macaddr, cdp_peer_tx_inactive_time,
                                     &buf);
    /*
     * CDP peer will not be created for PASN peers.
     * Hence, skip the return value check for PASN peers.
     */
    if (QDF_IS_STATUS_ERROR(status) &&
        (wlan_peer_get_peer_type(node->peer_obj) != WLAN_PEER_RTT_PASN))
        return;

    si->isi_inact = buf.tx_inactive_time;
    /* 11n */
    si->isi_htcap = wlan_node_get_htcap(node);
    si->isi_stamode= wlan_node_get_mode(node);
    si->isi_curr_mode = get_phymode_from_chwidth(vap->iv_ic, node);

#if ATH_SUPPORT_EXT_STAT
    si->isi_vhtcap = wlan_node_get_vhtcap(node);
    si->isi_chwidth = (u_int8_t) wlan_node_get_chwidth(node);
#endif

    /* Extended capabilities */
    si->isi_ext_cap = wlan_node_get_extended_capabilities(node);
    si->isi_ext_cap2 = wlan_node_get_extended_capabilities2(node);
    si->isi_ext_cap3 = wlan_node_get_extended_capabilities3(node);
    si->isi_ext_cap4 = wlan_node_get_extended_capabilities4(node);
    si->isi_nss = wlan_node_get_nss(node);
    si->isi_supp_nss = wlan_node_get_nss_capability(node);
    si->isi_is_256qam = wlan_node_get_256qam_support(node);
    si->isi_is_he = !!(IEEE80211_NODE_USE_HE(node));
    if (si->isi_is_he) {
        qdf_mem_copy(&si->isi_hecap_rxmcsnssmap,
                     &node->ni_he.hecap_rxmcsnssmap,
                     sizeof(u_int16_t) * HEHANDLE_CAP_TXRX_MCS_NSS_SIZE);
        qdf_mem_copy(&si->isi_hecap_txmcsnssmap,
                     &node->ni_he.hecap_txmcsnssmap,
                     sizeof(u_int16_t) * HEHANDLE_CAP_TXRX_MCS_NSS_SIZE);
        qdf_mem_copy(&si->isi_hecap_phyinfo,
                     &node->ni_he.hecap_phyinfo,
                     sizeof(u_int32_t) * HEHANDLE_CAP_PHYINFO_SIZE);
        si->isi_he_mu_capable = get_sta_he_mu_cap_info(
                        (uint8_t *)&si->isi_hecap_phyinfo[HECAP_PHYBYTE_IDX0]);
    }

#ifdef WLAN_FEATURE_11BE
    si->isi_is_eht = !!(IEEE80211_NODE_USE_EHT(node));
    if (si->isi_is_eht) {
        qdf_mem_copy(&si->isi_ehtcap_rxmcsnssmap,
                &node->ni_eht.ehtcap_rxmcsnssmap,
                sizeof(uint32_t) * EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
        qdf_mem_copy(&si->isi_ehtcap_txmcsnssmap,
                &node->ni_eht.ehtcap_txmcsnssmap,
                sizeof(uint32_t) * EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
        qdf_mem_copy(&si->isi_ehtcap_phyinfo,
                     &node->ni_eht.ehtcap_phyinfo,
                     sizeof(u_int32_t) * EHTHANDLE_CAP_PHYINFO_SIZE);
    }
#endif /* WLAN_FEATURE_11BE */
#ifdef WLAN_FEATURE_11BE_MLO
    si->isi_is_mlo = wlan_peer_is_mlo(node->peer_obj);

    if (si->isi_is_mlo) {
       WLAN_ADDR_COPY(si->isi_mldaddr,
                      wlan_peer_mlme_get_mldaddr(node->peer_obj));
       wlan_mlo_peer_get_partner_links_info(node->peer_obj, &ml_links);
       si->isi_num_links = 0;
       if (ml_links.num_partner_links <= IEEE80211_MAX_MLD_LINKS) {
          si->isi_num_links = ml_links.num_partner_links;
          for (i = 0; i < si->isi_num_links; i++) {
              si->isi_link_info[i].isi_link_id =
                             ml_links.partner_link_info[i].link_id;
              WLAN_ADDR_COPY(si->isi_link_info[i].isi_linkaddr,
                             &ml_links.partner_link_info[i].link_addr.bytes[0]);
              si->isi_link_info[i].isi_is_bridge = ml_links.partner_link_info[i].is_bridge;
          }
       }

       wlan_mlo_peer_get_str_capability(node->peer_obj, &si->isi_is_str);
       wlan_mlo_peer_get_eml_capability(node->peer_obj, &si->isi_is_emlsr,
                                        &si->isi_is_emlmr);
    }
#endif /* WLAN_FEATURE_11BE_MLO */

    cp = (u_int8_t *)(si+1);
    if (req->space < sizeof(struct ieee80211req_sta_info)) {
            qdf_err("no space to fill sta info %pM sta",
                    node->ni_macaddr);
            return;
    } else
        remaining_space = req->space - sizeof(struct ieee80211req_sta_info);

    if (!wlan_node_getwpaie(vap, macaddr, ni_ie, &ni_ie_len)) {
        if (ni_ie_len && (!remaining_space || (remaining_space < ni_ie_len))) {
            qdf_err("no space left for wpa IE for %pM sta",
                    node->ni_macaddr);
            return;
        }
        OS_MEMCPY(cp, ni_ie, ni_ie_len);
        cp += ni_ie_len;
        remaining_space -= ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if (!wlan_node_getwmeie(vap, macaddr, ni_ie, &ni_ie_len)) {
        if (ni_ie_len && (!remaining_space || (remaining_space < ni_ie_len))) {
            qdf_err("no space left for wme IE for %pM sta",
                    node->ni_macaddr);
            return;
        }
        OS_MEMCPY(cp, ni_ie, ni_ie_len);
        cp += ni_ie_len;
        remaining_space -= ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if (!wlan_node_getathie(vap, macaddr, ni_ie, &ni_ie_len)) {
        if (ni_ie_len && (!remaining_space || (remaining_space < ni_ie_len))) {
            qdf_err("no space left for ath IE for %pM sta",
                    node->ni_macaddr);
            return;
        }
        OS_MEMCPY(cp, ni_ie, ni_ie_len);
        cp += ni_ie_len;
        remaining_space -= ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if (!wlan_node_getwpsie(vap, macaddr, ni_ie, &ni_ie_len)) {
        if (ni_ie_len && (!remaining_space || (remaining_space < ni_ie_len))) {
            qdf_err("no space left for wps IE for %pM sta",
                    node->ni_macaddr);
            return;
        }
        OS_MEMCPY(cp, ni_ie, ni_ie_len);
        cp += ni_ie_len;
        remaining_space -= ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if (!wlan_node_get_suppchanie(vap, macaddr, ni_ie, &ni_ie_len)) {
        if (ni_ie_len && (!remaining_space || (remaining_space < ni_ie_len))) {
            qdf_err("no space left for suppchan IE for %pM sta",
                    node->ni_macaddr);
            return;
        }
        OS_MEMCPY(cp, ni_ie, ni_ie_len);
        cp += ni_ie_len;
        remaining_space -= ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }
    if (!wlan_node_get_opclassie(vap, macaddr, ni_ie, &ni_ie_len)) {
        if (ni_ie_len && (!remaining_space || (remaining_space < ni_ie_len))) {
            qdf_err("no space left for opclass IE for %pM sta",
                    node->ni_macaddr);
            return;
        }
        OS_MEMCPY(cp, ni_ie, ni_ie_len);
        cp += ni_ie_len;
        remaining_space -= ni_ie_len;
        ni_ie_len = IEEE80211_MAX_OPT_IE;
    }

    req->si = (
    struct ieee80211req_sta_info *)(((u_int8_t *)si) + len);
    req->space -= len;
}
int ieee80211_ucfg_getstaspace(wlan_if_t vap)
{
    struct stainforeq req;


    /* estimate space required for station info */
    req.space = sizeof(struct stainforeq);
    req.vap = vap;
    wlan_iterate_station_list(vap, get_sta_space, &req);

    return req.space;

}
int ieee80211_ucfg_getstainfo(wlan_if_t vap, struct ieee80211req_sta_info *si, uint32_t *len)
{
    struct stainforeq req;


    if (*len < sizeof(struct ieee80211req_sta_info))
        return -EFAULT;

    /* estimate space required for station info */
    req.space = sizeof(struct stainforeq);
    req.vap = vap;

    if (*len > 0)
    {
        size_t space = *len;

        if (si == NULL)
            return -ENOMEM;

        req.si = si;
        req.space = *len;

        wlan_iterate_station_list(vap, get_sta_info, &req);
        *len = space - req.space;
    }
    else
        *len = 0;

    return 0;
}

#if ATH_SUPPORT_IQUE
int ieee80211_ucfg_rcparams_setrtparams(wlan_if_t vap, uint8_t rt_index, uint8_t per, uint8_t probe_intvl)
{
    if ((rt_index != 0 && rt_index != 1) || per > 100 ||
        probe_intvl > 100)
    {
        goto error;
    }
    wlan_set_rtparams(vap, rt_index, per, probe_intvl);
    return 0;

error:
    qdf_print("usage: rtparams rt_idx <0|1> per <0..100> probe_intval <0..100>");
    return -EINVAL;
}

int ieee80211_ucfg_rcparams_setratemask(wlan_if_t vap, uint8_t preamble,
        uint32_t mask_lower32, uint32_t mask_higher32, uint32_t mask_lower32_2)
{
    osif_dev *osifp = (osif_dev *)vap->iv_ifp;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    bool eht_capab = false;
    int retv = -EINVAL;

    if(osifp->osif_is_mode_offload) {
        switch(preamble)
        {
            case IEEE80211_LEGACY_PREAMBLE:
                if ((mask_lower32 > 0xFFF) ||
                       (mask_higher32 != 0) ||
                       (mask_lower32_2 != 0)) {
                    qdf_print("Invalid ratemask for CCK/OFDM");
                    return retv;
                } else {
                    break;
                }
            case IEEE80211_HT_PREAMBLE:
                if((mask_higher32 != 0) || (mask_lower32_2 != 0)) {
                    qdf_print("Invalid ratemask for HT");
                    return retv;
                } else {
                    break;
                }
            case IEEE80211_VHT_PREAMBLE:
                /* For HE targets, we now support MCS0-11 for upto NSS 8 for VHT.
                 * But for legacy targets we have support till MCS0-9 NSS 4.
                 * Hence the below check ensures the correct bitmask is sent
                 * depending on the target. */
                if (!(ic->ic_he_target) && ((mask_higher32 > 0xFF) ||
                                            (mask_lower32_2 != 0))) {
                    qdf_print("Invalid ratemask for VHT");
                    return retv;
                } else {
                    break;
                }
            case IEEE80211_HE_PREAMBLE:
                if(!ic->ic_he_target){
                    qdf_print("HE preamble not supported for this target.");
                    return retv;
                } else {
                    break;
                }
            case IEEE80211_EHT_PREAMBLE:
                pdev = ic->ic_pdev_obj;
                if (pdev == NULL) {
                    qdf_err("%s : pdev is NULL ", __func__);
                    return retv;
                }

                psoc = wlan_pdev_get_psoc(pdev);
                if (psoc == NULL) {
                    qdf_err("%s: psoc is NULL", __func__);
                    return retv;
                }

                wlan_psoc_mlme_get_11be_capab(psoc, &eht_capab);
                if (!eht_capab) {
                    qdf_err("EHT preamble not supported for this target.");
                    return retv;
                } else {
                    break;
                }
            default:
                qdf_print("Invalid preamble type");
                return retv;
        }
        retv = ic->ic_vap_set_ratemask(vap, preamble, mask_lower32,
                mask_higher32, mask_lower32_2);
        /*
         * ic_is_vdev_restart_sup indicates vdev restart is supported by this
         * radio and retv set to ENETRESET indicates this config requires a vdev
         * restart.
         * restart_vap indicates that vdev restart optimization for config changes
         * is supported by FW.
         */

        if (retv == ENETRESET && dev && IS_UP(dev)) {
            if (ic->ic_is_vdev_restart_sup &&
                    (vap->iv_opmode == IEEE80211_M_HOSTAP))
                retv = osif_vdev_restart(vap);
            else
                retv = osif_vap_init(osifp, dev, RESCAN);
        } else {
            retv = 0;
        }
    }
    return retv;
}
#endif

static int
is_null_mac(char *addr)
{
	char nullmac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if (IEEE80211_ADDR_EQ(addr, nullmac))
		return 1;

	return 0;
}

#if WLAN_CFR_ENABLE
enum cfr_cwm_width ieee80211_convert_ieee_to_cfr_bw(
        enum ieee80211_cwm_width cwm_width)
{
    switch (cwm_width) {
        case IEEE80211_CWM_WIDTH20:
		return CFR_CWM_WIDTH20;
        case IEEE80211_CWM_WIDTH40:
		return CFR_CWM_WIDTH40;
        case IEEE80211_CWM_WIDTH80:
		return CFR_CWM_WIDTH80;
        case IEEE80211_CWM_WIDTH160:
		return CFR_CWM_WIDTH160;
        case IEEE80211_CWM_WIDTH80_80:
		return CFR_CWM_WIDTH80_80;
#ifdef WLAN_FEATURE_11BE
	case IEEE80211_CWM_WIDTH320:
		return CFR_CWM_WIDTH320;
#endif
        default:
	    return CFR_CWM_WIDTHINVALID;
    }
}
#endif

#ifdef CONFIG_SAWF
static int
ieee80211_ucfg_sawf_telemetry(struct wlan_objmgr_pdev *pdev,
			      struct sawf_wlanconfig_telemetry *sawf_telemetry)
{
	struct wlan_objmgr_psoc *psoc;

	if (!sawf_telemetry) {
		qdf_err("Telemetry config input is NULL");
		return  -EINVAL;
	}

	if (!pdev) {
		qdf_err("pdev is NULL");
		return  -EINVAL;
	}

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		qdf_err("psoc is NULL");
		return -EINVAL;
	}

	cdp_sawf_set_mov_avg_params(wlan_psoc_get_dp_handle(psoc),
				    sawf_telemetry->mv_avg_pkt,
				    sawf_telemetry->mv_avg_win);

	cdp_sawf_set_sla_params(wlan_psoc_get_dp_handle(psoc),
				sawf_telemetry->sla_num_pkt,
				sawf_telemetry->sla_time_sec);

	telemetry_sawf_set_mov_avg_params(sawf_telemetry->mv_avg_pkt,
					  sawf_telemetry->mv_avg_win);

	telemetry_sawf_set_sla_params(sawf_telemetry->sla_num_pkt,
				      sawf_telemetry->sla_time_sec);

	return 0;
}

#define SAWF_PARAM_DEFAULT(x, y) (x->y == 0 || x->y == DEF_SAWF_CONFIG_VALUE)
static int
ieee80211_ucfg_sawf_sla(struct wlan_objmgr_pdev *pdev,
			struct sawf_wlanconfig_sla *sawf_sla)
{
	struct wlan_sawf_svc_class_params *sawf_param;
	struct sawf_ctx *sawf_ctx;
	int ret = -EINVAL;

	sawf_ctx = wlan_get_sawf_ctx();
	if (!sawf_ctx) {
		qdf_err("SAWF context is NULL!");
		return -EINVAL;
	}

	qdf_spin_lock_bh(&sawf_ctx->lock);
	sawf_param = &sawf_ctx->svc_classes[sawf_sla->svc_id -1];
	if (!sawf_param->configured) {
		qdf_err("Service ID is not configured");
		goto err;
	}

	if (sawf_sla->min_thruput_rate &&
		SAWF_PARAM_DEFAULT(sawf_param, min_thruput_rate)) {
		qdf_err("min_thruput_rate is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	if (sawf_sla->max_thruput_rate &&
		SAWF_PARAM_DEFAULT(sawf_param, max_thruput_rate)) {
		qdf_err("max_thruput_rate is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	if (sawf_sla->burst_size &&
		SAWF_PARAM_DEFAULT(sawf_param, burst_size)) {
		qdf_err("burst_size is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	if (sawf_sla->service_interval &&
		SAWF_PARAM_DEFAULT(sawf_param, service_interval)) {
		qdf_err("service_interval is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	if (sawf_sla->delay_bound &&
		SAWF_PARAM_DEFAULT(sawf_param, delay_bound)) {
		qdf_err("delay_bound is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	if (sawf_sla->msdu_ttl &&
		SAWF_PARAM_DEFAULT(sawf_param, msdu_ttl)) {
		qdf_err("msdu_ttl is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	if (sawf_sla->msdu_rate_loss &&
		SAWF_PARAM_DEFAULT(sawf_param, msdu_rate_loss)) {
		qdf_err("msdu loss is not configured for svc_id:%d", sawf_sla->svc_id);
		goto err;
	}

	telemetry_sawf_set_sla_cfg(sawf_sla->svc_id,
				   sawf_sla->min_thruput_rate,
				   sawf_sla->max_thruput_rate,
				   sawf_sla->burst_size,
				   sawf_sla->service_interval,
				   sawf_sla->delay_bound,
				   sawf_sla->msdu_ttl,
				   sawf_sla->msdu_rate_loss);

	qdf_spin_unlock_bh(&sawf_ctx->lock);
	return  0;
err:
	qdf_err("SLA threshold cannot be configured");
	qdf_spin_unlock_bh(&sawf_ctx->lock);
	return ret;
}

static int
ieee80211_ucfg_sawf_sla_detect(struct wlan_objmgr_pdev *pdev,
			struct sawf_wlanconfig_sla_detect *sawf_sla_detect)
{
	telemetry_sawf_set_sla_detect_cfg(sawf_sla_detect->sla_detect,
					  sawf_sla_detect->min_thruput_rate,
					  sawf_sla_detect->max_thruput_rate,
					  sawf_sla_detect->burst_size,
					  sawf_sla_detect->service_interval,
					  sawf_sla_detect->delay_bound,
					  sawf_sla_detect->msdu_ttl,
					  sawf_sla_detect->msdu_rate_loss);

	return 0;
}
#endif

#ifdef CONFIG_SAWF_DEF_QUEUES

static
enum wmi_sched_disabled convert_str_to_enum(char *str, bool is_dl)
{
	if (is_dl) {
		if (strcmp(str, "MU_MIMO") == 0)
			return SCHED_MODE_DL_MU_MIMO;
		if (strcmp(str, "OFDMA") == 0)
			return SCHED_MODE_DL_OFDMA;
	} else {
		if (strcmp(str, "MU_MIMO") == 0)
			return SCHED_MODE_UL_MU_MIMO;
		if (strcmp(str, "OFDMA") == 0)
			return SCHED_MODE_UL_OFDMA;
	}

	return SCHED_MODE_INV_DMODE;
}

void
parse_sched_disabled_mode(char *disabled_mode,
			  uint32_t *wmi_disabled_modes, bool is_dl)
{
	char *delim = ",";
	char *mode_types;
	enum wmi_sched_disabled mode;

	mode_types = strsep(&disabled_mode, delim);
	while (mode_types != NULL) {
		mode = convert_str_to_enum(mode_types, is_dl);
		if (mode != SCHED_MODE_INV_DMODE)
			*wmi_disabled_modes |= (1 << mode);
		mode_types = strsep(&disabled_mode, delim);
	}
}

static int
ieee80211_ucfg_sawf_create(struct wlan_objmgr_pdev *pdev,
                           struct sawf_wlanconfig_create_param *param)
{
    struct sawf_ctx *sawf_ctx;
    struct wlan_sawf_svc_class_params *sawf_param;
    struct ieee80211com *ic;
    int ret = 0;
    bool svc_class_update = 0;
    uint32_t disabled_modes = 0;
    char ul_disabled_mode[DISABLED_MODE_MAX_LEN];
    char dl_disabled_mode[DISABLED_MODE_MAX_LEN];
    QDF_STATUS status;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    sawf_ctx = wlan_get_sawf_ctx();
    if (!sawf_ctx) {
        qdf_err("SAWF context is NULL!");
        return -EINVAL;
    }

    qdf_spin_lock_bh(&sawf_ctx->lock);
    if ((param->svc_id > SAWF_SVC_CLASS_MAX)) {
        qdf_err("SAWF: Max service classes configured");
        ret = -EINVAL;
        goto err;
    }

    if (!wlan_service_id_valid(param->svc_id)) {
        qdf_info("SAWF: Service ID(%d) is invalid", param->svc_id);
        ret = -EINVAL;
        goto err;
    }

    if (param->svc_id >= SAWF_STC_SVC_CLASS_MIN && param->svc_id <= SAWF_STC_SVC_CLASS_MAX) {
        param->svc_type = WLAN_SVC_TYPE_STC;
        param->priority = SAWF_STC_SVC_CLASS_DEFAULT_PRIORITY;
    }

    if (wlan_service_id_configured_nolock(param->svc_id)) {
        if ((param->svc_type == WLAN_SVC_TYPE_SAWF &&
            wlan_service_id_get_type_nolock(param->svc_id) != WLAN_SAWF_SVC_TYPE_DEF) ||
            (param->svc_type == WLAN_SVC_TYPE_STC &&
            wlan_service_id_get_type_nolock(param->svc_id) != WLAN_SAWF_SVC_TYPE_STC)) {
            qdf_info("SAWF: cmd svc_type(%d) does not match with svc class type(%d)",
            param->svc_type, wlan_service_id_get_type_nolock(param->svc_id));
            ret = -EINVAL;
            goto err;
        }

        if (wlan_service_id_tid_nolock((param->svc_id)) != param->tid) {
            qdf_info("Old TID:%d | New TID:%d",
                     wlan_service_id_tid_nolock(param->svc_id), param->tid);
            qdf_info("SAWF: Service ID(%d) is configured with different TID",
                     param->svc_id);
            ret = -EINVAL;
            goto err;
        }

        /*
         * SAWF/STC svc update is not allowed in if any other type is using
         * the service class which is indicated by ref_count != 1
         */
        if (wlan_service_id_get_ref_count_nolock(param->svc_id) != 1) {
            qdf_info("SAWF: Service ID(%d) is already in use by other types",
                     param->svc_id);
            ret = -EINVAL;
            goto err;
        }
        svc_class_update = true;
    }

    sawf_param = &sawf_ctx->svc_classes[param->svc_id - 1];

    if (!(strcmp(param->ul_disabled_modes, "X") == 0)) {
        qdf_str_lcopy(ul_disabled_mode, param->ul_disabled_modes,
                     DISABLED_MODE_MAX_LEN);
        parse_sched_disabled_mode(ul_disabled_mode, &disabled_modes, false);
        sawf_param->disabled_modes = disabled_modes;
    }

    if (!(strcmp(param->dl_disabled_modes, "X") == 0)) {
       qdf_str_lcopy(dl_disabled_mode, param->dl_disabled_modes,
		     DISABLED_MODE_MAX_LEN);
       parse_sched_disabled_mode(dl_disabled_mode, &disabled_modes, true);
       sawf_param->disabled_modes = disabled_modes;
    }

    sawf_param->svc_id = param->svc_id;
    qdf_str_lcopy(sawf_param->app_name, param->app_name,
                  WLAN_MAX_SVC_CLASS_NAME);
    sawf_param->min_thruput_rate = param->min_thruput_rate;
    sawf_param->max_thruput_rate = param->max_thruput_rate;
    sawf_param->burst_size = param->burst_size;
    sawf_param->service_interval = param->service_interval;
    sawf_param->delay_bound = param->delay_bound;
    sawf_param->msdu_ttl = param->msdu_ttl;
    sawf_param->tid = param->tid;
    sawf_param->msdu_rate_loss = param->msdu_rate_loss;
    sawf_param->configured = true;
    sawf_param->ul_burst_size = param->ul_burst_size;
    sawf_param->ul_min_tput = param->ul_min_tput;
    sawf_param->ul_max_latency = param->ul_max_latency;
    sawf_param->ul_service_interval = param->ul_service_interval;
    sawf_param->msduq_count = 0;
    sawf_param->enabled_param_mask = 0;
    sawf_param->priority = param->priority;
    if (param->svc_type == WLAN_SVC_TYPE_SAWF) {
        sawf_param->svc_type = WLAN_SAWF_SVC_TYPE_DEF;
    } else if (param->svc_type == WLAN_SVC_TYPE_STC) {
        sawf_param->svc_type = WLAN_SAWF_SVC_TYPE_STC;
    }

    sawf_param->ref_count = 1;

    status = wlan_validate_sawf_params(sawf_param);
    if (QDF_IS_STATUS_ERROR(status)) {
        if (svc_class_update) {
            qdf_info("SAWF:Invalid Params, Service ID:%d Updation failed",
                     param->svc_id);
        } else {
            qdf_info("SAWF:Invalid Params, Service ID:%d creation failed",
                     param->svc_id);
        }
        qdf_mem_zero(sawf_param, sizeof(struct wlan_sawf_svc_class_params));
        ret = -EINVAL;
        goto err;
    }

    telemetry_sawf_set_svclass_cfg(true, sawf_param->svc_id,
                                   param->min_thruput_rate,
                                   param->max_thruput_rate,
                                   param->burst_size,
                                   param->service_interval,
                                   param->delay_bound,
                                   param->msdu_ttl,
                                   param->msdu_rate_loss);

    if (ic->ic_cfg80211_radio_handler.set_sawf_params) {
        ret = ic->ic_cfg80211_radio_handler.set_sawf_params(
                (void*)pdev, sawf_param);
    }

err:
    qdf_spin_unlock_bh(&sawf_ctx->lock);
    return ret;
}

static int
ieee80211_ucfg_sawf_disable(struct wlan_objmgr_pdev *pdev,
                            struct sawf_wlanconfig_disable_param *param)
{
    struct sawf_ctx *sawf_ctx;
    struct wlan_sawf_svc_class_params *sawf_param;
    struct ieee80211com *ic;
    uint8_t service_id = param->svc_id;
    int ret = 0;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    sawf_ctx = wlan_get_sawf_ctx();
    if (!sawf_ctx) {
        qdf_err("SAWF context is NULL!");
        return -EINVAL;
    }

    qdf_spin_lock_bh(&sawf_ctx->lock);
    if (!wlan_service_id_valid(service_id)) {
        qdf_info("SAWF: Service ID(%d) is invalid", service_id);
        ret = -EINVAL;
        goto err;
    }

    if (!wlan_service_id_configured_nolock(service_id)) {
        ret = -EINVAL;
        goto err;
    }

    if (param->svc_id >= SAWF_STC_SVC_CLASS_MIN && param->svc_id <= SAWF_STC_SVC_CLASS_MAX)
        param->svc_type = WLAN_SVC_TYPE_STC;

    if ((param->svc_type == WLAN_SVC_TYPE_SAWF &&
        wlan_service_id_get_type_nolock(service_id) != WLAN_SAWF_SVC_TYPE_DEF) ||
        (param->svc_type == WLAN_SVC_TYPE_STC &&
        wlan_service_id_get_type_nolock(service_id) != WLAN_SAWF_SVC_TYPE_STC)) {
        qdf_info("SAWF: cmd svc_type(%d) does not match with svc class type(%d)",
                 param->svc_type, wlan_service_id_get_type_nolock(service_id));
        ret = -EINVAL;
        goto err;
    }

    /*
     * Allow Ref count decrement only when MSDUQ count is zero. Only when no
     * MSDUQ is mapped to this Service class, SAWF delete is allowed.
     */
    if (wlan_service_id_get_msduq_count_nolock(service_id)) {
        qdf_info("SAWF: The parameters of the service class ID:%d is currently "
                 "used by a MSDU queue. Disable not allowed.", service_id);
        ret = -EINVAL;
        goto err;
    }

    wlan_service_id_dec_ref_count_nolock(service_id);

    /*
     * SAWF delete command cannot be issued for a service class is used by
     * other types
     */
    if (wlan_service_id_get_ref_count_nolock(service_id)) {

        qdf_info("SAWF: Service ID(%d) is used by other types", service_id);
        ret = -EINVAL;
        goto err;
    }

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        ret = -EINVAL;
        goto err;
    }

    sawf_param = &sawf_ctx->svc_classes[service_id - 1];

    if (ic->ic_cfg80211_radio_handler.disable_sawf_svc) {
        ret = ic->ic_cfg80211_radio_handler.disable_sawf_svc(service_id);
    }

    qdf_info("Service class ID:%d is disabled", service_id);
    telemetry_sawf_set_svclass_cfg(false, sawf_param->svc_id,
                                   0, 0, 0, 0,
                                   0, 0, 0);
    qdf_mem_zero(sawf_param, sizeof(struct wlan_sawf_svc_class_params));

err:
    qdf_spin_unlock_bh(&sawf_ctx->lock);
    return ret;
}

int
ieee80211_ucfg_sawf_view(bool is_sawf)
{

    struct sawf_ctx *sawf_ctx;
    struct wlan_sawf_svc_class_params *sawf_param;
    uint8_t index = 0;

    sawf_ctx = wlan_get_sawf_ctx();
    if (!sawf_ctx) {
        qdf_err("SAWG context is NULL!");
        return -EINVAL;
    }

    qdf_spin_lock_bh(&sawf_ctx->lock);

    /* Print the Service class database when this function is called via:
     * 1. SAWF view command
     * 2. 11BE SCS Service class add/delete/update
     */
    if ((is_sawf) || (!is_sawf && qdf_print_is_verbose_enabled(qdf_get_pidx(),
                                  QDF_MODULE_ID_SCS, QDF_TRACE_LEVEL_TRACE))) {
        for(index = 0; index < SAWF_SVC_CLASS_MAX; index++) {
            sawf_param = &sawf_ctx->svc_classes[index];
            if (!sawf_param->configured)
                continue;
            wlan_print_service_class(sawf_param);
        }
    }
    qdf_spin_unlock_bh(&sawf_ctx->lock);

    return 0;
}

#if defined(CONFIG_SAWF) || defined(WLAN_SUPPORT_SCS)
void
ieee80211_ucfg_set_scs_qos_attr(struct ieee80211_qos_attr *qos_attr,
                                uint32_t min_tput, uint32_t service_interval,
                                uint32_t burst_size, uint32_t delay_bound,
                                uint32_t tid)
{
    /*
     * The units of service interval and delay bound are in millisecond for
     * both SAWF and SCS. Existing SCS functions handle these parameters in
     * microseconds. So convert both the paramters to microseconds for SCS.
     */
    qos_attr->min_data_rate = min_tput;
    qos_attr->resultant_service_interval = service_interval * 1000;
    qos_attr->resultant_burst_size = burst_size;
    qos_attr->delay_bound = delay_bound * 1000;
    qos_attr->tid = tid;
    qos_attr->access_category = TID_TO_WME_AC(qos_attr->tid);
    qos_attr->user_priority = qos_attr->tid;
    qos_attr->actual_rx_tid = qos_attr->tid;
}
#endif /* WLAN_SUPPORT_SCS || CONFIG_SAWF */

#ifdef WLAN_SUPPORT_SCS
static int
ieee80211_ucfg_scs_svc_create(struct wlan_objmgr_pdev *pdev,
                              struct sawf_wlanconfig_svc_create *param,
                              struct sawf_rsp_param *rsp_param)
{
    struct ieee80211com *ic;
    struct ieee80211_node *ni;
    struct ieee80211_qos_attr qos_attr = {0};
    uint8_t svc_id;
    QDF_STATUS status;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_dev_context *mld_dev = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
#endif

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr(
                (struct qdf_mac_addr *)param->peer_mac_addr, &mld_dev);

    if (mld_dev && ml_peer) {
        ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    } else
#endif
    {
        ni = ieee80211_find_node(ic, param->peer_mac_addr, WLAN_MLME_NB_ID);
    }

    if (!ni) {
        qdf_err("Unable to find ni "QDF_MAC_ADDR_FMT,
                QDF_MAC_ADDR_REF(param->peer_mac_addr));
        return -EINVAL;
    }

    if (param->direction == WLAN_SVC_DIR_UL) {
        uint32_t ul_svc_id;

        ieee80211_ucfg_set_scs_qos_attr(&qos_attr, param->ul_min_tput,
                                        param->ul_service_interval,
                                        param->ul_burst_size,
                                        param->ul_max_latency, param->tid);

        ul_svc_id = ieee80211_add_ul_svc_info(ni, &qos_attr);
        if (ul_svc_id == INVALID_SCS_SVC_ID) {
            wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
            return -EINVAL;
        }

        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);

        rsp_param->svc_id = ul_svc_id;

        return 0;
    }

    /*
     * For TID only requests, traffic is prioritzed by using HLOS TID
     * override method. TID is mapped to a service class ID
     * in the range [SAWF_SCS_SVC_CLASS_MIN SAWF_SCS_SVC_CLASS_MAX] and
     * the same TID is derrived from the service class ID in DP.
     */
    if (param->mask == (1 << WLAN_SVC_PARAM_TID)) {
        svc_id = param->tid + SAWF_SCS_SVC_CLASS_MIN;

        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);

        rsp_param->svc_id = svc_id;

        return 0;
    }

    ieee80211_ucfg_set_scs_qos_attr(&qos_attr, param->min_thruput_rate,
                                    param->service_interval, param->burst_size,
                                    param->delay_bound, param->tid);

    status = ieee80211_add_service_class(ic, &qos_attr);
    if (QDF_IS_STATUS_ERROR(status)) {
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    svc_id = qos_attr.service_class_id;

    status = ieee80211_add_dl_svc_info(ni, svc_id);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Unable to add dl svc %u", svc_id);
        ieee80211_disable_service_class(ic, svc_id);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);

    rsp_param->svc_id = svc_id;

    return 0;
}

static int
ieee80211_ucfg_scs_svc_delete(struct wlan_objmgr_pdev *pdev,
                              struct sawf_wlanconfig_svc_delete *param,
                              struct sawf_rsp_param *rsp_param)
{
    struct ieee80211com *ic;
    struct ieee80211_node *ni;
    uint8_t svc_id;
    QDF_STATUS status;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_dev_context *mld_dev = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
#endif

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr(
                (struct qdf_mac_addr *)param->peer_mac_addr, &mld_dev);

    if (mld_dev && ml_peer) {
        ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    } else
#endif
    {
        ni = ieee80211_find_node(ic, param->peer_mac_addr, WLAN_MLME_NB_ID);
    }

    if (!ni) {
        qdf_err("Unable to find ni "QDF_MAC_ADDR_FMT,
                QDF_MAC_ADDR_REF(param->peer_mac_addr));
        return -EINVAL;
    }

    if (param->svc_id > SAWF_SCS_SVC_CLASS_MAX) {
        status = ieee80211_delete_ul_svc_info(ni, param->svc_id,
                                              SAWF_INVALID_RX_TID);
        if (QDF_IS_STATUS_ERROR(status)) {
            wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
            return -EFAULT;
        }
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return 0;
    }

    if (param->svc_id >= SAWF_SCS_SVC_CLASS_MIN &&
        param->svc_id <= SAWF_SCS_SVC_CLASS_MAX) {
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return 0;
    }

    svc_id = param->svc_id;

    if (!wlan_service_id_valid(svc_id)) {
        qdf_err("Invalid service id %u", svc_id);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    status = ieee80211_delete_dl_svc_info(ni, svc_id);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Unable to delete dl svc %u", svc_id);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    status = ieee80211_disable_service_class(ic, svc_id);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Unable to disable dl svc %u", svc_id);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);

    return 0;
}

static int
ieee80211_ucfg_scs_svc_update(struct wlan_objmgr_pdev *pdev,
                              struct sawf_wlanconfig_svc_update *param,
                              struct sawf_rsp_param *rsp_param)
{
    struct sawf_ctx *sawf_ctx;
    struct ieee80211com *ic;
    struct ieee80211_node *ni;
    struct wlan_sawf_svc_class_params t_svc = {0}, *svc;
    uint8_t svc_id;
    uint8_t idx;
    QDF_STATUS status;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_dev_context *mld_dev = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
#endif

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr(
                (struct qdf_mac_addr *)param->peer_mac_addr, &mld_dev);

    if (mld_dev && ml_peer) {
        ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    } else
#endif
    {
        ni = ieee80211_find_node(ic, param->peer_mac_addr, WLAN_MLME_NB_ID);
    }

    if (!ni) {
        qdf_err("Unable to find ni "QDF_MAC_ADDR_FMT,
                QDF_MAC_ADDR_REF(param->peer_mac_addr));
        return -EINVAL;
    }

    if (param->direction == WLAN_SVC_DIR_UL) {
        struct ieee80211_qos_attr qos_attr = {0};
        uint32_t ul_svc_id;
        uint8_t idx;

        idx = ieee80211_find_ul_svc_node_index_lock(ni, param->svc_id, param->tid);

        if (idx == MAX_SCS_REQUEST_PER_DIRECTION) {
            qdf_err("svc id %d not found", param->svc_id);
            wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
            return -EINVAL;
        }

        ieee80211_ucfg_set_scs_qos_attr(&qos_attr, param->ul_min_tput,
                                        param->ul_service_interval,
                                        param->ul_burst_size,
                                        param->ul_max_latency, param->tid);

        qos_attr.service_class_id = param->svc_id;
        ul_svc_id = ieee80211_update_ul_svc_info(ni, &qos_attr);

        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);

        rsp_param->svc_id = ul_svc_id;

        return 0;
    }

    if (param->svc_id >= SAWF_SCS_SVC_CLASS_MIN &&
        param->svc_id <= SAWF_SCS_SVC_CLASS_MAX) {
        svc_id = param->tid + SAWF_SCS_SVC_CLASS_MIN;
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        rsp_param->svc_id = svc_id;
        return 0;
    }

    svc_id = param->svc_id;

    if (!wlan_service_id_valid(svc_id)) {
        qdf_err("Invalid service id %u", svc_id);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    idx = ieee80211_find_dl_svc_node_index_lock(ni, svc_id);
    if (idx == MAX_SCS_REQUEST_PER_DIRECTION) {
        qdf_err("svc id %d not found", svc_id);
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EFAULT;
    }

    sawf_ctx = wlan_get_sawf_ctx();
    if (!sawf_ctx) {
        qdf_err("SAWF context is NULL!");
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    svc = &t_svc;

    qdf_mem_copy(svc, &sawf_ctx->svc_classes[svc_id - 1], sizeof(*svc));

    if (param->mask & (1 << WLAN_SVC_PARAM_MIN_TPUT))
        svc->min_thruput_rate = param->min_thruput_rate;

    if (param->mask & (1 << WLAN_SVC_PARAM_MAX_TPUT))
        svc->max_thruput_rate = param->max_thruput_rate;

    if (param->mask & (1 << WLAN_SVC_PARAM_BURST_SIZE))
        svc->burst_size = param->burst_size;

    if (param->mask & (1 << WLAN_SVC_PARAM_SERVICE_INTERVAL))
        svc->service_interval = param->service_interval;

    if (param->mask & (1 << WLAN_SVC_PARAM_DELAY_BOUND))
        svc->delay_bound = param->delay_bound;

    if (param->mask & (1 << WLAN_SVC_PARAM_MSDU_TTL))
        svc->msdu_ttl = param->msdu_ttl;

    if (param->mask & (1 << WLAN_SVC_PARAM_PRIORITY))
        svc->priority = param->priority;

    if (param->mask & (1 << WLAN_SVC_PARAM_TID))
        svc->tid = param->tid;

    if (param->mask & (1 << WLAN_SVC_PARAM_MSDU_RATE_LOSS))
        svc->msdu_rate_loss = param->msdu_rate_loss;

    if (param->mask & (1 << WLAN_SVC_PARAM_UL_BURST_SIZE))
        svc->ul_burst_size = param->ul_burst_size;

    if (param->mask & (1 << WLAN_SVC_PARAM_UL_SERVICE_INTERVAL))
        svc->ul_service_interval = param->ul_service_interval;

    if (param->mask & (1 << WLAN_SVC_PARAM_UL_MIN_TPUT))
        svc->ul_min_tput = param->ul_min_tput;

    if (param->mask & (1 << WLAN_SVC_PARAM_UL_MAX_LATENCY))
        svc->ul_max_latency = param->ul_max_latency;

    status = wlan_validate_sawf_params(svc);
    if (QDF_IS_STATUS_ERROR(status)) {
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    /*
     * If the service class is used for multiple requests then find
     * a service class that exactly matches with the updated parameters.
     * If this fails then find the first available service id.
     */
    if (wlan_service_id_get_ref_count_nolock(svc_id) > 1) {
        ieee80211_check_existing_svc_qos_attributes_match(svc, sawf_ctx);
        if (svc->svc_id == SAWF_INVALID_SERVICE_CLASS_ID) {
             uint8_t idx;

             for (idx = 0; idx < SAWF_SVC_CLASS_MAX; idx++) {
                 if (!sawf_ctx->svc_classes[idx].configured) {
                     svc->svc_id = idx + 1;
                     break;
                 }
             }
        }
        svc_id = svc->svc_id;
    } else {
        svc_id = param->svc_id;
    }

    if (!svc_id || svc_id == SAWF_INVALID_SERVICE_CLASS_ID) {
        qdf_err("Unable to assign a service class");
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    if (param->svc_id != svc_id) {
        ieee80211_delete_dl_svc_info(ni, param->svc_id);
        status = ieee80211_add_dl_svc_info(ni, svc_id);
        if (QDF_IS_STATUS_ERROR(status)) {
            ieee80211_add_dl_svc_info(ni, param->svc_id);
            wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
            return -EFAULT;
        }
    }

    svc->svc_id = svc_id;
    svc->svc_type = WLAN_SAWF_SVC_TYPE_SCS;

    wlan_update_sawf_params_nolock(svc);

    svc = &sawf_ctx->svc_classes[svc_id - 1];
    svc->configured = true;

    if (param->svc_id != svc_id) {
        wlan_service_id_dec_ref_count_nolock(param->svc_id);
        wlan_service_id_inc_ref_count_nolock(svc->svc_id);
    }

    /* Configure service class in telemetry_agent */
    telemetry_sawf_set_svclass_cfg(true, svc->svc_id,
                                   svc->min_thruput_rate,
                                   svc->max_thruput_rate,
                                   svc->burst_size,
                                   svc->service_interval,
                                   svc->delay_bound,
                                   svc->msdu_ttl,
                                   svc->msdu_rate_loss);

    /* Set default SLA values for min_throughput, service_interval and
       delay_bound as these attributes are always received from STA in
       11BE SCS request */
    telemetry_sawf_set_sla_cfg(svc->svc_id,
                               SCS_SLA_DEFAULT_VALUE, 0, 0,
                               SCS_SLA_DEFAULT_VALUE,
                               SCS_SLA_DEFAULT_VALUE,
                               0, 0);

    if (ic->ic_cfg80211_radio_handler.set_sawf_params)
        ic->ic_cfg80211_radio_handler.set_sawf_params((void*)pdev, svc);

    wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);

    rsp_param->svc_id = svc_id;

    return 0;
}
#else /* WLAN_SUPPORT_SCS */
static int
ieee80211_ucfg_scs_svc_create(struct wlan_objmgr_pdev *pdev,
                              struct sawf_wlanconfig_svc_create *param,
                              struct sawf_rsp_param *rsp_param)
{
    return -EOPNOTSUPP;
}

static int
ieee80211_ucfg_scs_svc_delete(struct wlan_objmgr_pdev *pdev,
                              struct sawf_wlanconfig_svc_delete *param,
                              struct sawf_rsp_param *rsp_param)
{
    return -EOPNOTSUPP;
}

static int
ieee80211_ucfg_scs_svc_update(struct wlan_objmgr_pdev *pdev,
                              struct sawf_wlanconfig_svc_update *param,
                              struct sawf_rsp_param *rsp_param)
{
    return -EOPNOTSUPP;
}
#endif /* WLAN_SUPPORT_SCS */

static void
ieee80211_ucfg_set_sawf_param(struct sawf_wlanconfig_create_param *sawf_param,
                              uint8_t svc_id, char *app_name, uint32_t mask,
                              uint32_t min_tput, uint32_t max_tput,
                              uint32_t burst_size, uint32_t service_interval,
                              uint32_t delay_bound, uint32_t msdu_ttl,
                              uint32_t priority, uint8_t tid,
                              uint32_t msdu_rate_loss,
                              char *dl_disabled_modes, char *ul_disabled_modes,
                              uint32_t ul_burst_size,
                              uint32_t ul_service_interval,
                              uint32_t ul_min_tput,
                              uint32_t ul_max_latency, uint8_t svc_type)
{
    sawf_param->svc_id = svc_id;
    sawf_param->svc_type = svc_type;
    strlcpy(sawf_param->app_name, app_name, sizeof(sawf_param->app_name));
    sawf_param->param_set_mask = mask;
    sawf_param->min_thruput_rate = min_tput;
    sawf_param->max_thruput_rate = max_tput;
    sawf_param->burst_size = burst_size;
    sawf_param->service_interval = service_interval;
    sawf_param->delay_bound = delay_bound;
    sawf_param->msdu_ttl = msdu_ttl;
    sawf_param->priority = priority;
    sawf_param->tid = tid;
    sawf_param->msdu_rate_loss = msdu_rate_loss;
    sawf_param->ul_burst_size = ul_burst_size;
    sawf_param->ul_service_interval = ul_service_interval;
    sawf_param->ul_min_tput = ul_min_tput;
    sawf_param->ul_max_latency = ul_max_latency;
    strlcpy(sawf_param->dl_disabled_modes, dl_disabled_modes,
            sizeof(sawf_param->dl_disabled_modes));
    strlcpy(sawf_param->ul_disabled_modes, ul_disabled_modes,
            sizeof(sawf_param->ul_disabled_modes));
}

static int
ieee80211_ucfg_sawf_svc_create(struct wlan_objmgr_pdev *pdev,
                               struct sawf_wlanconfig_svc_create *param,
                               struct sawf_rsp_param *rsp_param)
{
    if (param->type == WLAN_SVC_TYPE_SCS)
        return ieee80211_ucfg_scs_svc_create(pdev, param, rsp_param);

    if (param->type == WLAN_SVC_TYPE_SAWF || param->type == WLAN_SVC_TYPE_STC) {
        struct sawf_ctx *sawf_ctx;
        struct wlan_sawf_svc_class_params *svc;
        struct sawf_wlanconfig_create_param sawf_param = {0};
        uint8_t idx, min_index, max_index;
        uint8_t svc_id = 0;
        int r;

        sawf_ctx = wlan_get_sawf_ctx();
        if (!sawf_ctx) {
            qdf_err("SAWF context is NULL!");
            return -EINVAL;
        }

        /*
         * Reusage of same service class id will be treated as service class
         * update request, so assign a new service class id for SAWF.
         */

        /*
         * User defined service class range: 1-120, Index range: 0-119
         * STC service class range: 121-128, Index range: 120-127
         */
        min_index = SAWF_SVC_CLASS_MIN - 1;
        max_index = SAWF_STC_SVC_CLASS_MIN - 2;

        for (idx = min_index; idx <= max_index; idx++) {
            svc = &sawf_ctx->svc_classes[idx];
            if (!svc->configured) {
                svc_id = idx + 1;
                break;
            }
        }

        if (!svc_id)
            return -EPERM;

        ieee80211_ucfg_set_sawf_param(&sawf_param, svc_id,
                                      param->app_name, param->mask,
                                      param->min_thruput_rate,
                                      param->max_thruput_rate,
                                      param->burst_size,
                                      param->service_interval,
                                      param->delay_bound, param->msdu_ttl,
                                      param->priority, param->tid,
                                      param->msdu_rate_loss,
                                      param->dl_disabled_modes,
                                      param->ul_disabled_modes,
                                      param->ul_burst_size,
                                      param->ul_service_interval,
                                      param->ul_min_tput,
                                      param->ul_max_latency,
                                      param->type);

        r = ieee80211_ucfg_sawf_create(pdev, &sawf_param);
        if (!r)
            rsp_param->svc_id = svc_id;

        return r;
    }

    return -EOPNOTSUPP;
}

static int
ieee80211_ucfg_sawf_svc_delete(struct wlan_objmgr_pdev *pdev,
                               struct sawf_wlanconfig_svc_delete *param,
                               struct sawf_rsp_param *rsp_param)
{
    if (param->type == WLAN_SVC_TYPE_SCS)
        return ieee80211_ucfg_scs_svc_delete(pdev, param, rsp_param);

    if (param->type == WLAN_SVC_TYPE_SAWF || param->type == WLAN_SVC_TYPE_STC) {
        struct sawf_wlanconfig_disable_param sawf_disable_param, *p;

        p = &sawf_disable_param;

        p->svc_id = param->svc_id;
        p->svc_type = param->type;

        return ieee80211_ucfg_sawf_disable(pdev, &sawf_disable_param);
    }

    return 0;
}

static int
ieee80211_ucfg_sawf_svc_update(struct wlan_objmgr_pdev *pdev,
                               struct sawf_wlanconfig_svc_update *param,
                               struct sawf_rsp_param *rsp_param)
{
    if (param->type == WLAN_SVC_TYPE_SCS)
        return ieee80211_ucfg_scs_svc_update(pdev, param, rsp_param);

    if (param->type == WLAN_SVC_TYPE_SAWF || param->type == WLAN_SVC_TYPE_STC) {
        struct sawf_ctx *sawf_ctx;
        struct wlan_sawf_svc_class_params *svc;
        struct sawf_wlanconfig_create_param sawf_param = {0};
        int r;

        sawf_ctx = wlan_get_sawf_ctx();
        if (!sawf_ctx) {
            qdf_err("SAWF context is NULL!");
            return -EINVAL;
        }

        svc = &sawf_ctx->svc_classes[param->svc_id - 1];
        if (!svc->configured) {
            qdf_err("Service ID %d is not configured", param->svc_id);
            return -EINVAL;
        }

        ieee80211_ucfg_set_sawf_param(&sawf_param, param->svc_id,
                                      param->app_name, param->mask,
                                      param->min_thruput_rate,
                                      param->max_thruput_rate,
                                      param->burst_size,
                                      param->service_interval,
                                      param->delay_bound, param->msdu_ttl,
                                      param->priority, param->tid,
                                      param->msdu_rate_loss,
                                      param->dl_disabled_modes,
                                      param->ul_disabled_modes,
                                      param->ul_burst_size,
                                      param->ul_service_interval,
                                      param->ul_min_tput,
                                      param->ul_max_latency,
                                      param->type);

        r = ieee80211_ucfg_sawf_create(pdev, &sawf_param);
        if (!r)
            rsp_param->svc_id = param->svc_id;

        return r;
    }

    return -EOPNOTSUPP;
}

int
ieee80211_ucfg_sawf_params(struct ieee80211com *ic,
                           struct ieee80211_wlanconfig *config,
                           struct sawf_rsp_param *rsp_param)
{

    struct sawf_wlanconfig_create_param *sawf_create_config;
    struct sawf_wlanconfig_disable_param *sawf_disable_config;
    struct sawf_wlanconfig_telemetry *sawf_telemetry;
    struct sawf_wlanconfig_sla *sawf_sla;
    struct sawf_wlanconfig_sla_detect *sawf_sla_detect;
    struct sawf_wlanconfig_svc_create *sawf_svc_create;
    struct sawf_wlanconfig_svc_delete *sawf_svc_delete;
    struct sawf_wlanconfig_svc_update *sawf_svc_update;
    struct wlan_objmgr_pdev *pdev = NULL;
    int ret = 0;

    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }
    pdev = ic->ic_pdev_obj;

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_SVC_CLASS_CREATE:
            sawf_create_config = &config->data.sawf_create_config;
            ret = ieee80211_ucfg_sawf_create(pdev, sawf_create_config);
            break;
        case IEEE80211_WLANCONFIG_SVC_CLASS_DISABLE:
            sawf_disable_config = &config->data.sawf_disable_config;
            ret = ieee80211_ucfg_sawf_disable(pdev, sawf_disable_config);
            break;
        case IEEE80211_WLANCONFIG_SVC_CLASS_VIEW:
            ret = ieee80211_ucfg_sawf_view(TRUE);
            break;
#ifdef CONFIG_SAWF
        case IEEE80211_WLANCONFIG_SAWF_TELEMETRY:
            sawf_telemetry = &config->data.sawf_telemetry;
            ret = ieee80211_ucfg_sawf_telemetry(pdev, sawf_telemetry);
            break;
        case IEEE80211_WLANCONFIG_SAWF_SLA:
            sawf_sla = &config->data.sawf_sla;
            ret = ieee80211_ucfg_sawf_sla(pdev, sawf_sla);
            break;
        case IEEE80211_WLANCONFIG_SAWF_SLA_DETECT:
            sawf_sla_detect = &config->data.sawf_sla_detect;
            ret = ieee80211_ucfg_sawf_sla_detect(pdev, sawf_sla_detect);
            break;
#endif
        case IEEE80211_WLANCONFIG_SVC_CREATE:
            sawf_svc_create = &config->data.sawf_svc_create;
            ret = ieee80211_ucfg_sawf_svc_create(pdev, sawf_svc_create,
                                                 rsp_param);
            break;
        case IEEE80211_WLANCONFIG_SVC_DELETE:
            sawf_svc_delete = &config->data.sawf_svc_delete;
            ret = ieee80211_ucfg_sawf_svc_delete(pdev, sawf_svc_delete,
                                                 rsp_param);
            break;
        case IEEE80211_WLANCONFIG_SVC_UPDATE:
            sawf_svc_update = &config->data.sawf_svc_update;
            ret = ieee80211_ucfg_sawf_svc_update(pdev, sawf_svc_update,
                                                 rsp_param);
            break;
        default:
            ret = -EINVAL;
            break;
    }

    return ret;
}
#endif
#ifdef CONFIG_SAWF
QDF_STATUS
ieee80211_ucfg_send_ul_sawf_params(struct wlan_objmgr_pdev *pdev,
                                   uint8_t *peer_mac_addr,
                                   uint32_t svc_id, uint8_t rx_tid,
                                   enum sawf_ul_op ul_op)
{
    struct ieee80211_qos_attr qos_attr = {0};
    struct ieee80211_node *ni;
    struct ieee80211com *ic;
    uint32_t svc_interval = 0, burst_size = 0;
    uint32_t min_tput = 0, max_latency = 0;
    uint8_t tid = 0, ul_tid;
    QDF_STATUS status;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_dev_context *mld_dev;
    struct wlan_mlo_peer_context *ml_peer;
#endif

    if (!(ul_op == SAWF_UL_OP_ADD || ul_op == SAWF_UL_OP_UPDATE ||
          ul_op == SAWF_UL_OP_DELETE)) {
        return QDF_STATUS_E_INVAL;
    }

    status = wlan_sawf_get_uplink_params(svc_id, &tid,
                                         &svc_interval, &burst_size,
                                         &min_tput, &max_latency);
    if (QDF_IS_STATUS_ERROR(status))
        return status;

    if (!svc_interval || !burst_size)
        return QDF_STATUS_E_FAILURE;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr(
                (struct qdf_mac_addr *)peer_mac_addr, &mld_dev);

    if (mld_dev && ml_peer) {
        struct wlan_objmgr_peer *assoc_peer;

        assoc_peer = wlan_mlo_peer_get_assoc_peer(ml_peer);
        if (!assoc_peer) {
            qdf_err("assoc peer is NULL");
            return QDF_STATUS_E_FAILURE;
        }

        status = wlan_objmgr_peer_try_get_ref(assoc_peer, WLAN_SAWF_ID);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("failed to get peer refcount");
            return QDF_STATUS_E_FAILURE;
        }

        ni = wlan_peer_get_mlme_ext_obj(assoc_peer);
        if (!ni) {
            qdf_err("ni is null");
            wlan_objmgr_peer_release_ref(assoc_peer, WLAN_SAWF_ID);
            return QDF_STATUS_E_FAILURE;
        }
    } else
#endif
    {
        ni = ieee80211_find_node(ic, peer_mac_addr, WLAN_SAWF_ID);
    }

    if (!ni) {
        qdf_err("Unable to find ni "QDF_MAC_ADDR_FMT,
                QDF_MAC_ADDR_REF(peer_mac_addr));
        return QDF_STATUS_E_FAILURE;
    }

    if (ul_op == SAWF_UL_OP_DELETE) {
        ieee80211_delete_ul_svc_info(ni, svc_id, rx_tid);
        ieee80211_free_node(ni, WLAN_SAWF_ID);
        return QDF_STATUS_SUCCESS;
    }


    if (ul_op == SAWF_UL_OP_UPDATE) {
        ieee80211_delete_ul_svc_info(ni, svc_id, tid);
        ul_tid = rx_tid;
    } else if (ul_op == SAWF_UL_OP_ADD) {
        ul_tid = tid;
    } else {
        ul_tid = rx_tid;
    }

    ieee80211_ucfg_set_scs_qos_attr(&qos_attr, min_tput, svc_interval,
                                    burst_size, max_latency, ul_tid);

    qos_attr.service_class_id = svc_id;
    qos_attr.actual_rx_tid = ul_tid;

    ieee80211_add_ul_svc_info(ni, &qos_attr);

    ieee80211_free_node(ni, WLAN_SAWF_ID);

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ieee80211_ucfg_send_ul_sawf_params);
#endif

#ifdef WLAN_WIFI_RADAR_ENABLE
int
ieee80211_ucfg_wifi_radar_params(struct ieee80211com *ic,
                                 struct ieee80211_wlanconfig *config)
{
    struct wifi_radar_wlanconfig_params *wifi_radar_params;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wifi_radar_command_params wifi_radar_req = {0};
    int retv = 0;
    struct wifi_radar_cal_info_params cal_params = {0};

    pdev = ic->ic_pdev_obj;
    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_WIFI_RADAR_ID) !=
                    QDF_STATUS_SUCCESS) {
        wifi_radar_err("Getting pdev ref failed");
        return -EINVAL;
    }

    if (wlan_wifi_radar_is_feature_disabled(pdev)) {
        wifi_radar_err("wifi radar feature is disabled");
        wlan_objmgr_pdev_release_ref(pdev, WLAN_WIFI_RADAR_ID);
        return -EINVAL;
    }
    wlan_objmgr_pdev_release_ref(pdev, WLAN_WIFI_RADAR_ID);

    wifi_radar_params = &config->data.wifi_radar_config;

    if (config->cmdtype == IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE)
        wifi_radar_req.periodicity = wifi_radar_params->periodicity;

    if ((config->cmdtype == IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE) ||
        (config->cmdtype == IEEE80211_WLANCONFIG_WIFI_RADAR_RX_CAL)) {
        if ((!wifi_radar_params->tx_chainmask) ||
            (!wifi_radar_params->rx_chainmask)) {
            wifi_radar_err("tx/rx chain mask can not be 0");
            return -EINVAL;
        }

        wifi_radar_req.bandwidth = wifi_radar_params->bandwidth;
        wifi_radar_req.tx_chainmask = wifi_radar_params->tx_chainmask;
        wifi_radar_req.rx_chainmask = wifi_radar_params->rx_chainmask;
        wifi_radar_req.num_ltf_tx = wifi_radar_params->num_ltf_tx;
        wifi_radar_req.num_skip_ltf_rx = wifi_radar_params->num_skip_ltf_rx;
        wifi_radar_req.num_ltf_accumulation =
                wifi_radar_params->num_ltf_accumulation;
    }

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE:
            wifi_radar_req.cmd_type = WIFI_RADAR_CMD_CAPTURE_START;
            break;
        case IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE_STOP:
            wifi_radar_req.cmd_type = WIFI_RADAR_CMD_CAPTURE_STOP;
            break;
        case IEEE80211_WLANCONFIG_WIFI_RADAR_RX_CAL:
            wifi_radar_req.cmd_type = WIFI_RADAR_CMD_RX_CAL;
            break;
        case IEEE80211_WLANCONFIG_WIFI_RADAR_TX_CAL:
            wifi_radar_req.cmd_type = WIFI_RADAR_CMD_TX_CAL;
            break;
        case IEEE80211_WLANCONFIG_WIFI_RADAR_GET_RX_CAL_STATUS:
            break;
        default:
            wifi_radar_err("Invalid wifi radar command");
            return -EINVAL;
    }

    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_WIFI_RADAR_ID) !=
                    QDF_STATUS_SUCCESS) {
        wifi_radar_err("Getting pdev ref failed");
        return -EINVAL;
    }

    if (config->cmdtype == IEEE80211_WLANCONFIG_WIFI_RADAR_GET_RX_CAL_STATUS) {
        retv = ucfg_wifi_radar_get_cal_param(pdev, &cal_params);
        qdf_mem_copy(&config->data.wifi_radar_cal_info,
                     &cal_params,
                     qdf_min(sizeof(config->data.wifi_radar_cal_info), sizeof(cal_params)));
    } else {
        retv = ucfg_wifi_radar_command(pdev, &wifi_radar_req);
    }
    wlan_objmgr_pdev_release_ref(pdev, WLAN_WIFI_RADAR_ID);
    return retv;
}
#endif

#if WLAN_CFR_ENABLE
int
ieee80211_ucfg_cfr_params(struct ieee80211com *ic, wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
    struct cfr_wlanconfig_param *cfr_params;
    struct ieee80211_node *ni = NULL;
    int retv = 0;
    struct cfr_capture_params cfr_req = {0};
    struct wlan_objmgr_pdev *pdev = NULL;
    struct qdf_mac_addr unassoc_mac = {0};

    pdev = vap->iv_ic->ic_pdev_obj;
    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_CFR_ID) !=
        QDF_STATUS_SUCCESS) {
            cfr_err("Getting pdev ref failed");
            return -EINVAL;
    }

    if (wlan_cfr_is_feature_disabled(pdev)) {
            cfr_err("cfr is disabled");
            wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
            return QDF_STATUS_E_NOSUPPORT;
    }
    wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);

    cfr_params = &config->data.cfr_config;
    qdf_mem_copy(&unassoc_mac, &cfr_params->mac[0], sizeof(struct qdf_mac_addr));

    switch (config->cmdtype) {

        case IEEE80211_WLANCONFIG_CFR_START:
            if(IEEE80211_IS_SPECIAL_VAP_ONLY(vap)){
                    cfr_err("This mode of CFR capture is not allowed as special vap is up\n");
                    return QDF_STATUS_E_NOSUPPORT;
                }
            ni = ieee80211_vap_find_node(vap, &(cfr_params->mac[0]), WLAN_MLME_SB_ID);
            if (ni == NULL) {
                if (cfr_params->capture_method == CFR_CAPTURE_METHOD_PROBE_RESPONSE ||
                    cfr_params->capture_method == CFR_CAPTURE_METHOD_AUTO) {
                    /* To remove complexity at user interface level, CFR capture
                     * for associated and un-associated clients use the same
                     * command. So if node is not found, trigger
                     * un-associated capture.
                     */
                    cfr_debug("Peer not found. Capture is enabled on Probe response");

                    cfr_req.period = cfr_params->periodicity;
                    cfr_req.bandwidth = cfr_params->bandwidth;
                    cfr_req.method = CFR_CAPTURE_METHOD_PROBE_RESPONSE;
                    pdev = vap->iv_ic->ic_pdev_obj;
                    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_CFR_ID) !=
                            QDF_STATUS_SUCCESS) {
                        cfr_err("Getting pdev ref failed");
                        return -EINVAL;
                    }

                    retv = ucfg_cfr_start_capture_probe_req(pdev, &unassoc_mac, &cfr_req);
                    wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
                    return retv;
                } else {
                    cfr_err("Peer not found");
                    return -EINVAL;
                }
            }

            if (cfr_params->bandwidth > ieee80211_convert_ieee_to_cfr_bw(ni->ni_chwidth)) {
                cfr_err("Invalid bandwidth\n");
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }

#ifdef WLAN_FEATURE_11BE
            if (cfr_params->bandwidth == ieee80211_convert_ieee_to_cfr_bw(IEEE80211_CWM_WIDTH320)) {
                cfr_err("320MHz bandwidth is not allowed, max supported is 160MHz\n");
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }
#endif
            if (cfr_params->capture_method >= CFR_CAPTURE_METHOD_MAX) {
                cfr_err("Invalid capture method\n");
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }

            pdev = vap->iv_ic->ic_pdev_obj;
            if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_CFR_ID) !=
                    QDF_STATUS_SUCCESS) {
                cfr_err("Getting pdev ref failed");
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }

            cfr_req.period = cfr_params->periodicity;
            cfr_req.bandwidth = cfr_params->bandwidth;

            if (cfr_params->capture_method == CFR_CAPTURE_METHOD_AUTO) {
                /* Update the highest method for connected clients here */
                cfr_req.method = CFR_CAPTURE_METHOD_QOS_NULL_WITH_PHASE;
            } else {
                /* If method is not auto, honour the input if valid */
                if (cfr_params->capture_method < CFR_CAPTURE_METHOD_LAST_VALID) {
                    cfr_req.method = cfr_params->capture_method;
                } else {
                    cfr_err("Invalid capture method");
                    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                    wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
                    return -EINVAL;
                }
            }

            retv = ucfg_cfr_start_capture(pdev, ni->peer_obj, &cfr_req);

            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);

            break;
        case IEEE80211_WLANCONFIG_CFR_STOP:
            if(IEEE80211_IS_SPECIAL_VAP_ONLY(vap)){
                    cfr_err("This mode of CFR capture is not allowed as special vap is up\n");
                    return QDF_STATUS_E_NOSUPPORT;
                }
            ni = ieee80211_vap_find_node(vap, &(cfr_params->mac[0]), WLAN_MLME_SB_ID);
            if (ni == NULL) {
                /* To remove complexity at user interface level, CFR capture for
                 * associated and un-associated clients use the same command.
                 * So if node is not found, stop un-associated capture.
                 */
                cfr_debug("Peer not found. Try disabling on Probe Req Ack");

                pdev = vap->iv_ic->ic_pdev_obj;
                if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_CFR_ID) !=
                    QDF_STATUS_SUCCESS) {
                    cfr_err("Getting pdev ref failed");
                    return -EINVAL;
                }
                retv = ucfg_cfr_stop_capture_probe_req(pdev, &unassoc_mac);
		wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
                return retv;
            }

            pdev = vap->iv_ic->ic_pdev_obj;
            if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_CFR_ID) !=
                    QDF_STATUS_SUCCESS) {
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }

            retv = ucfg_cfr_stop_capture(pdev, ni->peer_obj);

            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);

            break;
        case IEEE80211_WLANCONFIG_CFR_LIST_PEERS:
            break;
#ifdef WLAN_ENH_CFR_ENABLE
        case IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_FTM:
            {
                    /*
                     * m_directed_ftm capture mode is not supported in Cypress.
                     * Hence currently, we return -1 instead of enabling the
                     * capture mode. However, mode setting has not been removed,
                     * since this code can be used in future, if this capture
                     * mode is going to be supported in either Cypress or any
                     * other chipsets.
                     */
                cfr_err("As of now m_directed_ftm mode is not supported\n");
                return -EACCES;


            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_NDPA_NDP:
            {
                if(IEEE80211_IS_SPECIAL_VAP_ONLY(vap)){
                    cfr_err("This mode of CFR capture is not allowed as special vap is up\n");
                    return QDF_STATUS_E_NOSUPPORT;
                }
                retv = ucfg_cfr_set_rcc_mode(vap->vdev_obj,
                                             RCC_DIRECTED_NDPA_NDP_FILTER,
                                             (cfr_params->en_directed_ndpa_ndp)
                                             || !(cfr_params->dis_directed_ndpa_ndp));
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_FLITER:
            {
                retv = ucfg_cfr_set_rcc_mode(vap->vdev_obj, RCC_TA_RA_FILTER,
                                             (cfr_params->en_ta_ra_filter) ||
                                             !(cfr_params->dis_ta_ra_filter));
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_ALL_FTM_ACK:
            {

                    /*
                     * m_all_ftm_ack capture mode is not supported in Cypress.
                     * Hence currently, we return -1 instead of enabling the
                     * capture mode. However, mode setting has not been removed,
                     * since this code can be used in future, if this capture
                     * mode is going to be supported in either Cypress or any
                     * other chipsets.
                     */
                    cfr_err("As of now m_all_ftm_ack mode is not supported\n");
                    return -EACCES;

            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_NDPA_NDP_ALL:
            {

                retv = ucfg_cfr_set_rcc_mode(vap->vdev_obj,
                                             RCC_NDPA_NDP_ALL_FILTER,
                                             (cfr_params->en_ndpa_ndp_all) ||
                                             !(cfr_params->dis_ndpa_ndp_all));
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_ALL_PKT:
            {

                    /**
                     * m_all_pkt capture mode is not supported in case of
                     * Cypress.Hence currently, we return -1 instead of
                     * enabling the capture mode. However, mode setting has not
                     * been removed, since this code can be used in future, if
                     * this capture mode is going to be supported in either
                     * Cypress or any other chipsets.
                     */
                    cfr_err("As of now m_all_pkt mode is not supported.\n");
                    return -EACCES;

            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_ADDR:
            {

                    retv = ucfg_cfr_set_tara_config(vap->vdev_obj, cfr_params);

            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_BW_NSS:
            {
                    retv = ucfg_cfr_set_bw_nss(vap->vdev_obj, cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_SUBTYPE:
            {
                    retv = ucfg_cfr_set_frame_type_subtype(vap->vdev_obj,
                                                           cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_DUR:
            {
                    retv = ucfg_cfr_set_capture_duration(vap->vdev_obj,
                                                         cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL:
            {
                    retv = ucfg_cfr_set_capture_interval(vap->vdev_obj,
                                                         cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_TARA_FILTER_AS_FP:
            {
                    retv = ucfg_cfr_set_tara_filterin_as_fp(vap->vdev_obj,
                                                            cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_EN_CFG:
            {
                    retv = ucfg_cfr_set_en_bitmap(vap->vdev_obj, cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RESET_CFG:
            {
                    retv = ucfg_cfr_set_reset_bitmap(vap->vdev_obj, cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_UL_MU_USER_MASK:
            {
                    retv = ucfg_cfr_set_ul_mu_user_mask(vap->vdev_obj,
                                                        cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_FREEZE_TLV_DELAY_CNT:
            {
                    retv = ucfg_cfr_set_freeze_tlv_delay_cnt(vap->vdev_obj,
                                                             cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_DISABLE_ALL:
            {
                    retv = ucfg_cfr_set_rcc_mode(vap->vdev_obj,
                                                 RCC_DIS_ALL_MODE, 0);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_COUNT:
            {
                    retv = ucfg_cfr_set_capture_count(vap->vdev_obj,
                                                      cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL_MODE_SEL:
            {
                    retv = ucfg_cfr_set_capture_interval_mode_sel(vap->vdev_obj,
                                                                  cfr_params);
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_COMMIT:
            {
                if (ic->ic_mon_vap &&
                    (wlan_vdev_mlme_is_active(ic->ic_mon_vap->vdev_obj) ==
                     QDF_STATUS_SUCCESS)) {
                    cfr_err("Block RCC since Monitor VAP is present");
                    return QDF_STATUS_E_NOSUPPORT;
                }
                if (ucfg_cfr_get_rcc_enabled(vap->vdev_obj)) {
                    wlan_set_param(vap, IEEE80211_CONFIG_CFR_RCC,
                                   rcc_filter_enable);
                } else {
                    wlan_set_param(vap, IEEE80211_CONFIG_CFR_RCC,
                                   rcc_filter_disable);
                }

                retv = ucfg_cfr_committed_rcc_config(vap->vdev_obj);
                if (retv != QDF_STATUS_SUCCESS) {
                    wlan_set_param(vap, IEEE80211_CONFIG_CFR_RCC,
                                   rcc_filter_failure);
                }
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_GET_CFG:
            {
                    retv = ucfg_cfr_get_cfg(vap->vdev_obj);
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_DBG_COUNTERS:
            {
                    retv = ucfg_cfr_rcc_dump_dbg_counters(vap->vdev_obj);
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_CLR_COUNTERS:
            {
                    retv = ucfg_cfr_rcc_clr_dbg_counters(vap->vdev_obj);
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_DUMP_LUT:
            {
                    retv = ucfg_cfr_rcc_dump_lut(vap->vdev_obj);
            }
            break;
#endif

        default:
            cfr_err("Invalid CFR command: %d \n", config->cmdtype);
            return -EIO;
    }
    return retv;
}
#endif

int
ieee80211_ucfg_rtt_params(struct ieee80211com *ic, wlan_if_t vap,
                          struct ieee80211_wlanconfig *config)
{
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_LCR:
            if (!vap->lcr_enable) {
                qdf_err("enable_lcr command is disabled");
                return -1;
            }

            config->data.lcr_config.req_id = ic->ic_rtt_req_id;
            ic->ic_rtt_req_id++;

            return ic->ic_send_lcr_cmd(pdev, &config->data.lcr_config);
        case IEEE80211_WLANCONFIG_LCI:
            if (!vap->lci_enable) {
                qdf_err("enable_lci command is disabled");
                return -1;
            }

            config->data.lci_config.req_id = ic->ic_rtt_req_id;
            ic->ic_rtt_req_id++;

            return ic->ic_send_lci_cmd(pdev, &config->data.lci_config);
        case IEEE80211_WLANCONFIG_FTMRR:
            if (!(vap->rtt_enable & RTT_RESPONDER_MODE)) {
                qdf_err("Responder mode is disabled in enable_rtt command");
                return -1;
            }
            return ieee80211_send_ftmrr_frame(pdev, &config->data.ftmrr_config);
        default:
            qdf_err("Invalid RTT command : %d", config->cmdtype);
            return -EIO;
    }
}

int
ieee80211_ucfg_nawds(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_nawds *nawds;

    nawds = &config->data.nawds;
    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_NAWDS_SET_MODE:
            return wlan_nawds_set_param(vap,
                                        IEEE80211_NAWDS_PARAM_MODE,
                                        &nawds->mode);
        case IEEE80211_WLANCONFIG_NAWDS_SET_DEFCAPS:
#ifdef WLAN_FEATURE_11BE
            wlan_nawds_set_param(vap,
                                 IEEE80211_NAWDS_PARAM_DEF_PUNCTURE_BITMAP,
                                 &nawds->def_puncture_bitmap);
#endif
            return wlan_nawds_set_param(vap,
                                        IEEE80211_NAWDS_PARAM_DEFCAPS,
                                        &nawds->defcaps);

        case IEEE80211_WLANCONFIG_NAWDS_SET_OVERRIDE:
            return wlan_nawds_set_param(vap,
                                        IEEE80211_NAWDS_PARAM_OVERRIDE,
                                        &nawds->override);
        case IEEE80211_WLANCONFIG_NAWDS_SET_ADDR:
        {
            int status;

#ifdef WLAN_FEATURE_11BE
            status = wlan_nawds_config_mac(vap, nawds->mac,
                                           nawds->caps, nawds->puncture_bitmap);
#else
            status = wlan_nawds_config_mac(vap, nawds->mac, nawds->caps, 0);
#endif
            if (!status && wlan_nawds_check_if_auth_fail(vap, nawds->mac)) {
                 OS_SLEEP_INTERRUPTIBLE(NAWDS_AUTH_RETRY_DELAY);
                 wlan_nawds_node_authorize(vap, nawds->mac);
                 if (wlan_nawds_check_if_auth_fail(vap, nawds->mac))
                     qdf_err("nawds node auth retry failed! investigate...");
            }

            if(status == 0 ) {
               OS_SLEEP(250000);
            }
            return status;
        }
        case IEEE80211_WLANCONFIG_NAWDS_KEY:
            memcpy(vap->iv_nawds.psk, nawds->psk, strlen(nawds->psk));
            if (is_null_mac(nawds->mac)) {
                qdf_info("mac is NULL, psk set for learning RE\n");
                return 0;
            }
            return wlan_nawds_config_key(vap, nawds->mac, nawds->psk);
        case IEEE80211_WLANCONFIG_NAWDS_CLR_ADDR:
            return wlan_nawds_delete_mac(vap, nawds->mac);
        case IEEE80211_WLANCONFIG_NAWDS_GET:
            wlan_nawds_get_param(vap, IEEE80211_NAWDS_PARAM_MODE, &nawds->mode);
            wlan_nawds_get_param(vap, IEEE80211_NAWDS_PARAM_DEFCAPS,
                                 &nawds->defcaps);
            wlan_nawds_get_param(vap, IEEE80211_NAWDS_PARAM_OVERRIDE,
                                 &nawds->override);
            if (wlan_nawds_get_mac(vap, nawds->num, &nawds->mac[0],
                                   &nawds->caps)) {
                qdf_err("failed to get NAWDS entry %d", nawds->num);
            }
            wlan_nawds_get_param(vap, IEEE80211_NAWDS_PARAM_NUM, &nawds->num);
            config->status = IEEE80211_WLANCONFIG_OK;
            break;
        default :
            return -EIO;
    }
    return 0;
}

#ifdef UMAC_SUPPORT_MLNAWDS
int
ieee80211_ucfg_mlnawds(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_nawds *nawds;

    nawds = &config->data.nawds;
    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_MLNAWDS_SET_MODE:
            return wlan_mlnawds_set_param(vap,
                                          IEEE80211_MLNAWDS_PARAM_MODE,
                                          nawds);
        case IEEE80211_WLANCONFIG_MLNAWDS_SET_OVERRIDE:
            return wlan_mlnawds_set_param(vap,
                                          IEEE80211_MLNAWDS_PARAM_OVERRIDE,
                                          nawds);
        case IEEE80211_WLANCONFIG_MLNAWDS_SET_ADDR:
        {
            int status;

            status = wlan_mlnawds_config_mac(vap, nawds);
            if (status == 0) {
                OS_SLEEP(250000);
            }
            return status;
        }
        case IEEE80211_WLANCONFIG_MLNAWDS_KEY:
            return wlan_mlnawds_config_key(vap,
                                           nawds->peer_mld_mac,
                                           nawds->psk);
        case IEEE80211_WLANCONFIG_MLNAWDS_CLR_ADDR:
            return wlan_mlnawds_delete_mac(vap, nawds->peer_mld_mac);
        case IEEE80211_WLANCONFIG_MLNAWDS_GET:
            wlan_mlnawds_get_param(vap,
                                   IEEE80211_MLNAWDS_PARAM_MODE,
                                   &nawds->mode);
            wlan_mlnawds_get_param(vap,
                                   IEEE80211_MLNAWDS_PARAM_OVERRIDE,
                                   &nawds->override);
            if (wlan_mlnawds_get_mac(vap, nawds->num, nawds)) {
                qdf_info("failed to get NAWDS entry %d", nawds->num);
            }
            wlan_mlnawds_get_param(vap,
                                   IEEE80211_MLNAWDS_PARAM_NUM,
                                   &nawds->num);
            config->status = IEEE80211_WLANCONFIG_OK;
            break;
        case IEEE80211_WLANCONFIG_MLNAWDS_START:
            wlan_mlnawds_start(vap, nawds->peer_mld_mac);
            break;
        default :
            return -EIO;
    }
    return 0;
}
#endif /* UMAC_SUPPORT_MLNAWDS */

#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
int
ieee80211_ucfg_mesh(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_mesh *mesh;

    mesh = &config->data.mesh;
    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_MESH_ADDLOCALPEER:
        {
            int status = -1;
            uint8_t temp[IEEE80211_ADDR_LEN] = {0};

            if (!memcmp(mesh->peer_mld_mac, temp, IEEE80211_ADDR_LEN)) {
                IEEE80211_ADDR_COPY(vap->bssid_mesh,mesh->peers[0].peer_link_mac);
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
                wlan_update_rawsim_config(vap);
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
                if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
                    status = wlan_add_localpeer(vap, mesh->peers[0].peer_link_mac,
                                                mesh->peers[0].caps,
                                                mesh->peers[0].puncture_bitmap);
                }
            } else {
                if (mesh->num_links > 3) {
                    qdf_err("Only upto 3 link mesh addition is supported\n");
                    return -EIO;
                }
                status = wlan_add_localmlpeer(vap, mesh);
            }
            return status;
        }
        case IEEE80211_WLANCONFIG_MESH_ALLOWDATA:
        {
            struct ieee80211_node *ni = NULL;

            ni = ieee80211_find_node(vap->iv_ic, mesh->peer_mld_mac, WLAN_WDS_ID);
            if (ni && !ni->ml_info.num_partner_links) {
                ieee80211_free_node(ni, WLAN_WDS_ID);
                wlan_authorise_local_peer(vap, mesh->peer_mld_mac);
            } else {
                wlan_mesh_allowdata(vap, mesh->peer_mld_mac);
                if (ni)
                    ieee80211_free_node(ni, WLAN_WDS_ID);
            }
            break;
        }
        case IEEE80211_WLANCONFIG_MESH_DELLOCALPEER:
            return wlan_del_localmlpeer(vap, mesh->peer_mld_mac);
        default :
            return -EIO;
    }
    return 0;
}
#endif
#endif

#if ATH_SUPPORT_HYFI_ENHANCEMENTS
int
ieee80211_ucfg_me_list(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
	struct ieee80211_wlanconfig_me_list *me_list;
	ol_txrx_soc_handle soc = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	u_int8_t ret;
	u_int8_t pdev_id;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        struct ieee80211com *ic = vap->iv_ic;
#endif

	pdev = wlan_vdev_get_pdev(vap->vdev_obj);
	soc = wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(pdev));
	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
	me_list = &(config->data.me_list);
	switch (config->cmdtype) {
		case IEEE80211_WLANCONFIG_ME_LIST_ADD:
			if (me_list->me_list_type == IEEE80211_HMMC_LIST) {
				ret = dp_add_hmmc(soc, pdev_id, me_list->ip, me_list->mask);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
				if (!ret && ic->nss_vops) {
					ic->nss_vops->ic_osif_nss_vdev_me_add_hmmc_member((osif_dev *)vap->iv_ifp,
											  me_list->ip,
											  me_list->mask);
				}
#endif
			} else if (me_list->me_list_type == IEEE80211_DENY_LIST){
				ret = dp_add_deny_list(soc, pdev_id, me_list->ip, me_list->mask);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                                if (!ret && ic->nss_vops) {
					ic->nss_vops->ic_osif_nss_vdev_me_add_deny_member((osif_dev *)vap->iv_ifp,
											  me_list->ip,
											  me_list->mask);
                                }
#endif
			} else if (me_list->me_list_type == IEEE80211_HMMC_LIST_V6) {
				ret = dp_add_hmmc_v6(soc, pdev_id,
				                       me_list->ipv6, me_list->mask);
			} else if (me_list->me_list_type == IEEE80211_DENY_LIST_V6) {
				ret = dp_add_deny_list_v6(soc, pdev_id,
				                            me_list->ipv6, me_list->mask);
			}
			break;
		case IEEE80211_WLANCONFIG_ME_LIST_DEL:
			if (me_list->me_list_type == IEEE80211_HMMC_LIST) {
				ret = dp_del_hmmc(soc, pdev_id, me_list->ip, me_list->mask);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
				if (!ret && ic->nss_vops) {
					ic->nss_vops->ic_osif_nss_vdev_me_del_hmmc_member((osif_dev *)vap->iv_ifp,
											  me_list->ip,
											  me_list->mask);
				}
#endif
			} else if (me_list->me_list_type == IEEE80211_DENY_LIST) {
				ret = dp_del_deny_list(soc, pdev_id, me_list->ip, me_list->mask);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                                if (!ret && ic->nss_vops) {
					ic->nss_vops->ic_osif_nss_vdev_me_del_deny_member((osif_dev *)vap->iv_ifp, me_list->ip, me_list->mask);
                                }
#endif
			} else if (me_list->me_list_type == IEEE80211_HMMC_LIST_V6) {
				ret = dp_del_hmmc_v6(soc, pdev_id,
				                         me_list->ipv6, me_list->mask);
			} else if (me_list->me_list_type == IEEE80211_DENY_LIST_V6) {
				ret = dp_del_deny_list_v6(soc, pdev_id,
				                           me_list->ipv6, me_list->mask);
			}
			break;
		case IEEE80211_WLANCONFIG_ME_LIST_DUMP:
			if (me_list->me_list_type == IEEE80211_HMMC_LIST) {
				ret = dp_hmmc_dump(soc, pdev_id);
			} else if (me_list->me_list_type == IEEE80211_DENY_LIST) {
				ret = dp_deny_list_dump(soc, pdev_id);
			} else if (me_list->me_list_type == IEEE80211_HMMC_LIST_V6) {
				ret = dp_hmmc_dump_v6(soc, pdev_id);
			} else if (me_list->me_list_type == IEEE80211_DENY_LIST_V6) {
				ret = dp_deny_list_dump_v6(soc, pdev_id);
			}
			break;
		default:
			ret = -EOPNOTSUPP;
	}

	return 0;
}
#endif
int
ieee80211_ucfg_hmwds(wlan_if_t vap, struct ieee80211_wlanconfig *config, int buffer_len)
{
    struct ieee80211_wlanconfig_hmwds *hmwds;
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    struct ieee80211_wlanconfig_wds_table *wds_table;
#endif
    int ret = 0;
    hmwds = &(config->data.hmwds);
    switch (config->cmdtype) {
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
        case IEEE80211_WLANCONFIG_HMWDS_ADD_ADDR:
            ret = wlan_hmwds_add_addr(vap, hmwds->wds_ni_macaddr, hmwds->wds_macaddr, hmwds->wds_macaddr_cnt);
            break;
        case IEEE80211_WLANCONFIG_HMWDS_RESET_ADDR:
            ret = wlan_hmwds_reset_addr(vap, hmwds->wds_ni_macaddr);
            break;
        case IEEE80211_WLANCONFIG_HMWDS_RESET_TABLE:
            ret = wlan_hmwds_reset_table(vap);
            break;
        case IEEE80211_WLANCONFIG_HMWDS_READ_ADDR:
            hmwds->wds_macaddr_cnt = buffer_len - sizeof (*config);
            ret = wlan_hmwds_read_addr(vap, hmwds->wds_ni_macaddr, hmwds->wds_macaddr, &hmwds->wds_macaddr_cnt);
            if (ret)
                hmwds->wds_macaddr_cnt = 0;
            break;
        case IEEE80211_WLANCONFIG_HMWDS_READ_TABLE:
            wds_table = &config->data.wds_table;
            wds_table->wds_entry_cnt = buffer_len - sizeof (*config);
            ret = wlan_wds_read_table(vap, wds_table);
            if (ret)
                wds_table->wds_entry_cnt = 0;
            break;
        case IEEE80211_WLANCONFIG_HMWDS_SET_BRIDGE_ADDR:
            ret = wlan_hmwds_set_bridge_mac_addr(vap, hmwds->wds_macaddr);
            break;
        case IEEE80211_WLANCONFIG_HMWDS_REMOVE_ADDR:
            ret = wlan_hmwds_remove_addr(vap, hmwds->wds_macaddr);
            break;
        case IEEE80211_WLANCONFIG_HMWDS_DUMP_WDS_ADDR:
            ret = wlan_wds_dump_wds_addr(vap);
            break;
#endif
        default:
            return -ENXIO;
    }
    return ret;
}

#ifdef IPA_OFFLOAD
int
ieee80211_ucfg_ipa_wds(wlan_if_t vap, struct ieee80211_wlanconfig *config,
                       int buffer_len)
{
    struct ieee80211_wlanconfig_ipa_wds *wds_cfg;
    int ret = 0;
    wds_cfg = &(config->data.ipa_wds);

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_IPA_WDS_REMOVE_ADDR:
            ret = wlan_ipa_wds_remove_addr(vap, wds_cfg->wds_macaddr,
                                           wds_cfg->peer_macaddr);
            break;
        default:
            return -ENXIO;
    }
    return ret;
}
#endif

#if UMAC_SUPPORT_WNM
#ifdef WLAN_FEATURE_11BE_MLO
QDF_STATUS wlan_wnm_mlo_maxbss(struct wlan_objmgr_vdev *vdev_obj,
			     struct ieee80211_wlanconfig_wnm *wnm)
{
	struct wlan_objmgr_vdev *vdev;
	uint16_t vdev_count = 0;
	struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
	struct ieee80211vap *vap;
	int i;
	uint32_t status = QDF_STATUS_SUCCESS;

	mlo_get_ml_vdev_list(vdev_obj, &vdev_count, wlan_vdev_list);
	if (!vdev_count) {
		qdf_err("Number of VDEVs under MLD is reported as 0");
		return QDF_STATUS_E_INVAL;
	}

	for (i = 0; i < vdev_count; i++) {
		vdev = wlan_vdev_list[i];
		if (!vdev) {
			qdf_err("VDEV in MLD VDEV list is NULL");
			status = QDF_STATUS_E_INVAL;
			goto end;

		}
		vap = wlan_vdev_mlme_get_ext_hdl(vdev);
		if (!vap ||
		    wlan_wnm_set_bssmax(vap, &wnm->data.bssmax)) {
			qdf_err("Failed to configure bssmax for vdev_id %d",
				vdev->vdev_objmgr.vdev_id);
			status = QDF_STATUS_E_INVAL;
			goto end;
		}
	}
end:
	if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
						 QDF_ARRAY_SIZE(wlan_vdev_list),
						 vdev_count, 0) != QDF_STATUS_SUCCESS)
		qdf_err("failed to release mlo vdev ref");

	return status;
}
#endif /* WLAN_FEATURE_11BE_MLO */

int
ieee80211_ucfg_wnm(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
	struct ieee80211_wlanconfig_wnm *wnm;
	int status = 0;
	osif_dev *osifp = (osif_dev *)vap->iv_ifp;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);

	if (!wlan_wnm_vap_is_set(vap))
		return -EFAULT;

	wnm = &(config->data.wnm);
	switch (config->cmdtype) {
		case IEEE80211_WLANCONFIG_WNM_SET_BSSMAX:
#ifdef WLAN_FEATURE_11BE_MLO
			if (ieee80211_mlo_is_mld_ap(vap->vdev_obj))
				status = wlan_wnm_mlo_maxbss(vap->vdev_obj, wnm);
			else
#endif
				status = wlan_wnm_set_bssmax(vap, &wnm->data.bssmax);
			if (status == 0) {
				status = (dev && IS_UP(dev)) ? -osif_vap_init(osifp, dev, RESCAN) : 0;
			} else {
				return -EFAULT;
			}
			break;
		case IEEE80211_WLANCONFIG_WNM_GET_BSSMAX:
			status = wlan_wnm_get_bssmax(vap, &wnm->data.bssmax);
			config->status = (status == 0) ? IEEE80211_WLANCONFIG_OK : IEEE80211_WLANCONFIG_FAIL;
			break;
		case IEEE80211_WLANCONFIG_WNM_TFS_ADD:
			status = wlan_wnm_set_tfs(vap, &wnm->data.tfs);
			return status;

		case IEEE80211_WLANCONFIG_WNM_TFS_DELETE:
			/* since there is no tfs request elements its send the
			   TFS requestion action frame with NULL elements which
			   will delete the existing request on AP as per specification */
			status = wlan_wnm_set_tfs(vap, &wnm->data.tfs);
			return status;

		case IEEE80211_WLANCONFIG_WNM_FMS_ADD_MODIFY:
			status = wlan_wnm_set_fms(vap, &wnm->data.fms);
			return status;

		case IEEE80211_WLANCONFIG_WNM_SET_TIMBCAST:
			status = wlan_wnm_set_timbcast(vap, &wnm->data.tim);
			return status;

		case IEEE80211_WLANCONFIG_WNM_GET_TIMBCAST:
			status = wlan_wnm_get_timbcast(vap, &wnm->data.tim);
			config->status = (status == 0) ? IEEE80211_WLANCONFIG_OK : IEEE80211_WLANCONFIG_FAIL;
			break;

		case IEEE80211_WLANCONFIG_WNM_BSS_TERMINATION:

			/*
			 * For offload Architecture we have no way to get the MAC TSF as of now.
			 * Disabling this feature for offload untill we have a way to
			 * get the TSF.
			 */
			qdf_print("Disabled for Offload Architecture");
			return -EINVAL;

		default:
			break;
	}
	return 0;
}
#endif

int ieee80211_ucfg_addie(wlan_if_t vap, struct ieee80211_wlanconfig_ie *ie_buffer)
{
    int error = 0;

    switch (ie_buffer->ftype) {
        case IEEE80211_APPIE_FRAME_BEACON:
            wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_BEACON, (u_int8_t*)&ie_buffer->ie, ie_buffer->ie.len, ie_buffer->ie.elem_id);
            break;
        case IEEE80211_APPIE_FRAME_PROBE_RESP:
            wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_PROBERESP, (u_int8_t*)&ie_buffer->ie, ie_buffer->ie.len, ie_buffer->ie.elem_id);
            break;
        case IEEE80211_APPIE_FRAME_ASSOC_RESP:
            wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_ASSOCRESP, (u_int8_t*)&ie_buffer->ie, ie_buffer->ie.len, ie_buffer->ie.elem_id);
            break;
        default:
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_ERROR, "Frame type not supported\n");
            error = -ENXIO;
    }

    return error;
}

#if ATH_SUPPORT_DYNAMIC_VENDOR_IE
int
ieee80211_ucfg_vendorie(wlan_if_t vap, struct ieee80211_wlanconfig_vendorie *vie)
{
	int error;
	switch (vie->cmdtype) {
		case IEEE80211_WLANCONFIG_VENDOR_IE_ADD:
			error = wlan_set_vendorie(vap, IEEE80211_VENDOR_IE_PARAM_ADD, vie);
			break;

		case IEEE80211_WLANCONFIG_VENDOR_IE_UPDATE:
			error = wlan_set_vendorie(vap, IEEE80211_VENDOR_IE_PARAM_UPDATE, vie);
			break;

		case IEEE80211_WLANCONFIG_VENDOR_IE_REMOVE:
			error = wlan_set_vendorie(vap, IEEE80211_VENDOR_IE_PARAM_REMOVE, vie);
			break;

		default:
			error = -ENXIO;
	}
	return error;
}
#endif
#if ATH_SUPPORT_NAC_RSSI
int
ieee80211_ucfg_nac_rssi(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_nac_rssi *nac_rssi;
    nac_rssi = &(config->data.nac_rssi);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"%s:cmd_type=%d BSSID:%2x:%2x:%2x:%2x:%2x:%2x, "
        "client:%2x:%2x:%2x:%2x:%2x:%2x channel: %d\n", __func__,
         config->cmdtype, nac_rssi->mac_bssid[0], nac_rssi->mac_bssid[1],
         nac_rssi->mac_bssid[2], nac_rssi->mac_bssid[3], nac_rssi->mac_bssid[4],
         nac_rssi->mac_bssid[5], nac_rssi->mac_client[0], nac_rssi->mac_client[1],
         nac_rssi->mac_client[2], nac_rssi->mac_client[3], nac_rssi->mac_client[4],
         nac_rssi->mac_client[5], nac_rssi->chan_num);

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_NAC_RSSI_ADDR_ADD:
            return wlan_set_nac_rssi(vap, IEEE80211_NAC_RSSI_PARAM_ADD, nac_rssi);

        case IEEE80211_WLANCONFIG_NAC_RSSI_ADDR_DEL:
            return wlan_set_nac_rssi(vap, IEEE80211_NAC_RSSI_PARAM_DEL, nac_rssi);

        case IEEE80211_WLANCONFIG_NAC_RSSI_ADDR_LIST:
            return wlan_list_nac_rssi(vap, IEEE80211_NAC_RSSI_PARAM_LIST, nac_rssi);

        default:
            return -ENXIO;
   }
}

#endif
#if ATH_SUPPORT_NAC
int
ieee80211_ucfg_nac(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
	struct ieee80211_wlanconfig_nac *nac;
	nac = &(config->data.nac);

	switch (config->cmdtype) {
		case IEEE80211_WLANCONFIG_NAC_ADDR_ADD:
			return wlan_set_nac(vap, IEEE80211_NAC_PARAM_ADD, nac);

		case IEEE80211_WLANCONFIG_NAC_ADDR_DEL:
			return wlan_set_nac(vap, IEEE80211_NAC_PARAM_DEL, nac);

		case IEEE80211_WLANCONFIG_NAC_ADDR_LIST:
			wlan_list_nac(vap, IEEE80211_NAC_PARAM_LIST, nac);
			break;

		default:
			return -ENXIO;
	}
	return 0;

}
#endif

#if QCA_SUPPORT_PEER_ISOLATION
int
ieee80211_ucfg_isolation(wlan_if_t vap, struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_isolation *isolation = &(config->data.isolation);
    struct ieee80211_wlanconfig_isolation_list *list;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "cmd_type=%d mac:%pM\n",
                      config->cmdtype, isolation->mac);

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_ADD:
            return wlan_peer_isolation_add_mac(vap, isolation);

        case IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_DEL:
            return wlan_peer_isolation_del_mac(vap, isolation);

        case IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_LIST:
            list = (struct ieee80211_wlanconfig_isolation_list *)(config + 1);
            return wlan_peer_isolation_list(vap, list);

        case IEEE80211_WLANCONFIG_PEER_ISOLATION_FLUSH_LIST:
            return wlan_peer_isolation_flush(vap);

        case IEEE80211_WLANCONFIG_PEER_ISOLATION_NUM_CLIENT:
            list = (struct ieee80211_wlanconfig_isolation_list *)(config + 1);
            list->mac_cnt = vap->peer_isolation_list.num_peers;
            return 0;

        default:
            return -ENXIO;
   }
}
#endif

int ieee80211_ucfg_scanlist(wlan_if_t vap)
{
	int retv = 0;
	osif_dev *osifp = (osif_dev *)vap->iv_ifp;
	struct scan_start_request *scan_params = NULL;
	int time_elapsed = OS_SIWSCAN_TIMEOUT;
	bool scan_pause;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;

	vdev = osifp->ctrl_vdev;
	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_OSIF_SCAN_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		scan_info("unable to get reference");
		return -EBUSY;
	}
    pdev = wlan_vdev_get_pdev(vap->vdev_obj);

	/* Increase timeout value for EIR since a rpt scan itself takes 12 seconds */
	if (wlan_rptr_pdev_is_eir(pdev))
		time_elapsed = OS_SIWSCAN_TIMEOUT * SIWSCAN_TIME_ENH_IND_RPT;
	scan_pause =  (wlan_rptr_pdev_is_eir(pdev)) ?(vap->iv_pause_scan  ) : 0;
	/* start a scan */
	if ((time_after(OS_GET_TICKS(), osifp->os_last_siwscan + time_elapsed))
			&& (osifp->os_giwscan_count == 0) &&
			(vap->iv_opmode == IEEE80211_M_STA)&& !scan_pause) {
		if (wlan_rptr_pdev_is_eir(pdev)) {
			scan_params = qdf_mem_malloc(sizeof(*scan_params));
			if (!scan_params) {
				wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
				return -ENOMEM;
			}
			status = wlan_update_scan_params(vap, scan_params, wlan_vap_get_opmode(vap), true, true, true, true, 0, NULL, 0);
			if (status) {
				wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
				scan_err("scan param init failed with status: %d", status);
				qdf_mem_free(scan_params);
				return -EINVAL;
			}
			scan_params->scan_req.scan_f_forced = true;
			scan_params->scan_req.min_rest_time = MIN_REST_TIME;
			scan_params->scan_req.max_rest_time = MAX_REST_TIME;
			scan_params->scan_req.dwell_time_active = MAX_DWELL_TIME_ACTIVE;
			osifp->os_last_siwscan = OS_GET_TICKS();

			status = wlan_ucfg_scan_start(vap, scan_params, osifp->scan_requestor,
					SCAN_PRIORITY_LOW, &(osifp->scan_id), 0, NULL);
			if (status) {
				scan_err("scan_start failed with status: %d", status);
			}
		}
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
	osifp->os_giwscan_count = 0;
	return retv;
}

size_t
scan_space(wlan_scan_entry_t se, u_int16_t *ielen)
{
	u_int8_t ssid_len;
	uint8_t ssid_ie_len = 0;

	*ielen = 0;
	ssid_len = util_scan_entry_ssid(se)->length;
	*ielen =  util_scan_entry_ie_len(se);
	if (util_scan_entry_is_hidden_ap(se) && ssid_len) {
		ssid_ie_len = ssid_len + (sizeof(uint8_t) * 2);
	}
	return roundup(sizeof(struct ieee80211req_scan_result) +
			*ielen +  ssid_len + ssid_ie_len, sizeof(u_int32_t));
}

QDF_STATUS
get_scan_space(void *arg, wlan_scan_entry_t se)
{
	struct scanreq *req = arg;
	u_int16_t ielen;

	req->space += scan_space(se, &ielen);
	return 0;
}

QDF_STATUS
get_scan_space_rep_move(void *arg, wlan_scan_entry_t se)
{
	struct scanreq *req = arg;
	struct ieee80211vap *vap = req->vap;
	struct ieee80211com  *ic;
	u_int16_t ielen;
	u_int8_t *ssid;
	u_int8_t ssid_len;
	u_int8_t *bssid;
	struct wlan_ssid *rptr_ssid;
	struct qdf_mac_addr *rptr_bssid;

	ic = vap->iv_ic;
	ssid_len = util_scan_entry_ssid(se)->length;
	ssid = util_scan_entry_ssid(se)->ssid;
	bssid = util_scan_entry_bssid(se);

	rptr_ssid = wlan_rptr_move_get_ssid(ic->ic_pdev_obj);
	if (!rptr_ssid)
	    return QDF_STATUS_E_FAILURE;

	rptr_bssid = wlan_rptr_move_get_bssid(ic->ic_pdev_obj);
	if (!rptr_bssid)
	    return QDF_STATUS_E_FAILURE;

	/* Calculate scan space only for those scan entries that match
	 * the SSID of the new Root AP
	 */
	if (ssid) {
	    if (!(strncmp(ssid, rptr_ssid->ssid,
	                  rptr_ssid->length))) {
		if(!qdf_is_macaddr_zero(rptr_bssid)) {
		    if (IEEE80211_ADDR_EQ(bssid, rptr_bssid->bytes)) {
		        req->space += scan_space(se, &ielen);
		    }
		} else {
	            req->space += scan_space(se, &ielen);
	        }
	    }
	}

	return 0;
}

QDF_STATUS
get_scan_result(void *arg, wlan_scan_entry_t se)
{
	struct scanreq *req = arg;
	struct ieee80211req_scan_result *sr;
	u_int16_t ielen, len, nr, nxr;
	u_int8_t *cp;
	u_int8_t ssid_len;
        struct ieee80211vap *vap = req->vap;
        bool des_ssid_found = 0;
	u_int8_t *rates, *ssid, *bssid;
	struct ieee80211com  *ic;
	struct wlan_ssid *rptr_ssid;
	struct qdf_mac_addr *rptr_bssid;

	ic = vap->iv_ic;

	len = scan_space(se, &ielen);
	if (len > req->space)
		return 0;

	rptr_ssid = wlan_rptr_move_get_ssid(ic->ic_pdev_obj);
	if (!rptr_ssid)
	    return QDF_STATUS_E_FAILURE;

	rptr_bssid = wlan_rptr_move_get_bssid(ic->ic_pdev_obj);
	if (!rptr_bssid)
	    return QDF_STATUS_E_FAILURE;

	sr = req->sr;
	memset(sr, 0, sizeof(*sr));
	ssid_len = util_scan_entry_ssid(se)->length;
	ssid = util_scan_entry_ssid(se)->ssid;
	bssid = util_scan_entry_bssid(se);
	sr->isr_ssid_len = ssid_len;

        if (req->scanreq_type == SCANREQ_GIVE_ONLY_DESSIRED_SSID) {
            des_ssid_found = ieee80211_vap_match_ssid(vap->iv_ic->ic_sta_vap, ssid, ssid_len);
            if (!des_ssid_found) {
                return 0;
            } else {
                qdf_print("Getting desired ssid from scan result");
            }
        }
        if (req->scanreq_type == SCANREQ_GIVE_EXCEPT_DESSIRED_SSID) {
            des_ssid_found = ieee80211_vap_match_ssid(vap->iv_ic->ic_sta_vap, ssid, ssid_len);
            if (des_ssid_found) {
                return 0;
            }
        }

	if (wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_IN_PROGRESS) {
	    if (ssid == NULL)
	        return 0;

	    if (bssid == NULL)
	        return 0;

	    if ((strncmp(ssid, rptr_ssid->ssid,
	                 rptr_ssid->length)) != 0) {
	        return 0;
	    }

	    if(!qdf_is_macaddr_zero(rptr_bssid)) {
		if (!IEEE80211_ADDR_EQ(bssid, (uint8_t *)rptr_bssid->bytes))
		    return 0;
	    }
	}

	if (ielen > 65534 ) {
		ielen = 0;
	}
	sr->isr_ie_len = ielen;
	sr->isr_len = len;
	sr->isr_freq = se->channel.chan_freq;
	sr->isr_flags = 0;
	sr->isr_rssi = util_scan_entry_snr(se);
	sr->isr_intval =  util_scan_entry_beacon_interval(se);
	sr->isr_capinfo = util_scan_entry_capinfo(se).value;
	sr->isr_erp =  util_scan_entry_erpinfo(se);
	IEEE80211_ADDR_COPY(sr->isr_bssid, util_scan_entry_bssid(se));
	rates = util_scan_entry_rates(se);
	nr = 0;
	if (rates) {
		nr = min((int)rates[1], IEEE80211_RATE_MAXSIZE);
		memcpy(sr->isr_rates, rates+2, nr);
	}

	rates = util_scan_entry_xrates(se);
	nxr=0;
	if (rates) {
		nxr = min((int)rates[1], IEEE80211_RATE_MAXSIZE - nr);
		memcpy(sr->isr_rates+nr, rates+2, nxr);
	}
	sr->isr_nrates = nr + nxr;

	cp = (u_int8_t *)(sr+1);
	if (ssid) {
		memcpy(cp,ssid, sr->isr_ssid_len);
	}
	cp += sr->isr_ssid_len;

	/* If AP is hidden, insert SSID IE at front of IE list */
	if (util_scan_entry_is_hidden_ap(se) && ssid_len) {
		*cp++ = WLAN_ELEMID_SSID;
		*cp++ = ssid_len;
		qdf_mem_copy(cp, ssid, ssid_len);
		cp += ssid_len;
	}

	if (ielen) {
		util_scan_entry_copy_ie_data(se,cp,&ielen);
		cp += ielen;
	}

	req->space -= len;
	req->sr = (struct ieee80211req_scan_result *)(((u_int8_t *)sr) + len);

	return 0;
}

void ieee80211_ucfg_setmaxrate_per_client(void *arg, wlan_node_t node)
{
	struct ieee80211_wlanconfig_setmaxrate *smr =
		(struct ieee80211_wlanconfig_setmaxrate *)arg;
	struct ieee80211_node *ni = node;
	struct ieee80211com *ic = ni->ni_ic;
	int i, rate_updated = 0;

	if (IEEE80211_ADDR_EQ(ni->ni_macaddr, smr->mac)) {
		ni->ni_maxrate = smr->maxrate;
		if (ni->ni_maxrate == 0xff) {
			ni->ni_rates.rs_nrates = ni->ni_maxrate_legacy;
			ni->ni_htrates.rs_nrates = ni->ni_maxrate_ht;
			/* set the default vht max rate info */
			ni->ni_maxrate_vht = 0xff;
			rate_updated = 1;
			goto end;
		}
		/* legacy rate */
		if (!(ni->ni_maxrate & 0x80)) {

			/* For VHT/HT capable station, do not allow user to set legacy rate as max rate */
			if ((ni->ni_vhtcap) || (ni->ni_htcap))
				return;

			for (i = 0; i < ni->ni_rates.rs_nrates; i++) {
				if ((ni->ni_maxrate & IEEE80211_RATE_VAL)
						<= (ni->ni_rates.rs_rates[i] & IEEE80211_RATE_VAL))
				{
					rate_updated = 1;
					ni->ni_rates.rs_nrates = i + 1;
					ni->ni_htrates.rs_nrates = 0;
					ni->ni_maxrate_vht = 0;
					break;
				}
			}
		}
		/* HT rate */
		else if (ni->ni_maxrate < 0xc0) {
			if (!ni->ni_htcap) {
				return;
			}
			/* For VHT capable station, do not allow user to set HT rate as max rate */
			if (ni->ni_vhtcap) {
				return;
			}
			for (i = 0; i < ni->ni_htrates.rs_nrates; i++) {
				if ((ni->ni_maxrate & 0x7f) <= ni->ni_htrates.rs_rates[i]) {
					rate_updated = 1;
					ni->ni_htrates.rs_nrates = i + 1;
					ni->ni_maxrate_vht = 0;
					break;
				}
			}
		}
		/* VHT rate */
		else if (ni->ni_maxrate >= 0xc0 && ni->ni_maxrate <= 0xf9) {
#define VHT_MAXRATE_IDX_MASK    0x0F
#define VHT_MAXRATE_IDX_SHIFT   4
			u_int8_t maxrate_vht_idx = (ni->ni_maxrate & VHT_MAXRATE_IDX_MASK) + 1;
			u_int8_t maxrate_vht_stream = (((ni->ni_maxrate & ~VHT_MAXRATE_IDX_MASK) - 0xc0)
					>> VHT_MAXRATE_IDX_SHIFT) + 1;
			if (!ni->ni_vhtcap)
				return;
			/* b0-b3: vht rate idx; b4-b7: # stream */
			ni->ni_maxrate_vht = (maxrate_vht_stream << VHT_MAXRATE_IDX_SHIFT) | maxrate_vht_idx;
			rate_updated = 1;
#undef VHT_MAXRATE_IDX_MASK
#undef VHT_MAXRATE_IDX_SHIFT
		}
		else {
			qdf_print("Unknown max rate 0x%x", ni->ni_maxrate);
			return;
		}

end:
		/* Calling ath_net80211_rate_node_update() for Updating the node rate */
		if (rate_updated) {
			ni->ni_set_max_rate = 1;
			ic->ic_rate_node_update(ni, 0);
		}

		if (ni->ni_maxrate == 0xff) {
			qdf_print("rateset initialized to negotiated rates");
		}
	}
}

int ieee80211_ucfg_set_peer_nexthop(void *osif, uint8_t *mac, int32_t if_num)
{
    int retv = 0;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (ic->nss_vops) {
        ic->nss_vops->ic_osif_nss_vdev_set_peer_next_hop(osifp, mac, if_num);
        retv = 1;
    }

    return retv;
#else
    qdf_print("Setting peer nexthop is only supported in NSS Offload mode");
    return retv;
#endif
}

int ieee80211_ucfg_set_vlan_type(void *osif, uint8_t default_vlan, uint8_t port_vlan)
{
    int retv = 0;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (ic->nss_vops) {
        ic->nss_vops->ic_osif_nss_vdev_set_vlan_type(osifp, default_vlan, port_vlan);
        retv = 1;
    }

#endif
    return retv;
}

/*
 * ieee80211_ucfg_set_hlos_tid_override()
 *  This API set the config to enable/disable hlos tid override
 */
int ieee80211_ucfg_set_hlos_tid_override(void *osif, uint8_t val, bool is_mscs)
{
    cdp_config_param_type buf = {0};
    osif_dev *osifp = (osif_dev *)osif;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        return -1;
    }

    /*
     * Enable/disable hlos tid override in flag in dp vdev
     * Hlos tid override supported in both STA and AP vap for EasyMesh
     * Hlos tid override supported in only AP vap for MSCS
     */
    if ((wlan_vap_get_opmode(vap) != IEEE80211_M_HOSTAP) && is_mscs) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,"Valid only when VAP in AP mode and MSCS enabled\n");
        return 1;
    }

    if (val) {
        osifp->wifi3_0_fast_path |= WIFI_3_0_ULTRA_FASTPATH;
    }

    /*
     * Hlos tid override is valid for AP VAP
     */
    buf.cdp_vdev_param_hlos_tid_override = val;
    if (cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc),
            wlan_vdev_get_id(vap->vdev_obj), CDP_ENABLE_HLOS_TID_OVERRIDE,
            buf) != QDF_STATUS_SUCCESS) {
        return 1;

    }

   /*
    * Enable/disable hlos tid override in NSS FW
    */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_vops) {
        ic->nss_vops->ic_osif_nss_vdev_set_cfg(osifp, OSIF_NSS_WIFI_VDEV_ENABLE_HLOS_TID_OVERRIDE);
    }
#endif

    return 0;
}

/*
 * ieee80211_ucfg_get_hlos_tid_override()
 *  This API gets hlos tid override config
 */
int ieee80211_ucfg_get_hlos_tid_override(void *osif)
{
    cdp_config_param_type buf = {0};
    osif_dev *osifp = (osif_dev *)osif;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    if (!psoc) {
        return -1;
    }

    cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
                            wlan_vdev_get_id(vap->vdev_obj),
                            CDP_ENABLE_HLOS_TID_OVERRIDE, &buf);

    return buf.cdp_vdev_param_hlos_tid_override;
}

/*
 * ieee80211_ucfg_set_drop_3addr_mcast()
 *  This API set the config to enable/disable drop 3address mcast frames
 */
int ieee80211_ucfg_set_drop_3addr_mcast(struct ieee80211vap *vap, uint8_t value)
{
    cdp_config_param_type buf = {0};
    struct wlan_objmgr_psoc *psoc = NULL;
    struct ieee80211com *ic = vap->iv_ic;

    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        return -1;
    }

    if (value < 0 || value > 1) {
        qdf_err("Invalid value. Set/reset 1/0");
        return -1;
    }

    buf.cdp_drop_3addr_mcast = value;
    cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc),
                            wlan_vdev_get_id(vap->vdev_obj),
                            CDP_DROP_3ADDR_MCAST, buf);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_vops && ic->nss_vops->ic_osif_nss_vdev_set_cfg) {
        ic->nss_vops->ic_osif_nss_vdev_set_cfg((osif_dev *)vap->iv_ifp,
                                          OSIF_NSS_WIFI_VDEV_DROP_3ADDR_MCAST);
    }
#endif

    return 0;
}

#ifdef WLAN_SUPPORT_MESH_LATENCY
/*
 * ieee80211_ucfg_set_vap_mesh_tid()
 *  This API mesh latency tid in vap
 */
int ieee80211_ucfg_set_vap_mesh_tid(void *osif, uint8_t val)
{
    cdp_config_param_type buf = {0};
    osif_dev *osifp = (osif_dev *)osif;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        return -1;
    }

    buf.cdp_vdev_param_mesh_tid = val;
    if (cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc),
        wlan_vdev_get_id(vap->vdev_obj), CDP_SET_VAP_MESH_TID,
        buf) != QDF_STATUS_SUCCESS) {
        return 1;
    }
    return 0;
}

/*
 * ieee80211_ucfg_get_vap_mesh_tid()
 *  This API gets vap mesh tid
 */
int ieee80211_ucfg_get_vap_mesh_tid(void *osif,
    uint8_t *tid, uint8_t *dl_ul_enable)
{
    cdp_config_param_type buf = {0};
    osif_dev *osifp = (osif_dev *)osif;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    if (!psoc) {
        return -1;
    }

    cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
        wlan_vdev_get_id(vap->vdev_obj), CDP_SET_VAP_MESH_TID,
        &buf);
    *tid = (buf.cdp_vdev_param_mesh_tid >> 4) & 0xF;
    *dl_ul_enable = buf.cdp_vdev_param_mesh_tid & 0xF;
    return 0;
}

/*
 * ieee80211_ucfg_set_peer_tid_latency_enable()
 *  This API set the config to enable/disable peer tid latency
 */
int ieee80211_ucfg_set_peer_tid_latency_enable(void *osif, uint8_t val)
{
    cdp_config_param_type buf = {0};
    osif_dev *osifp = (osif_dev *)osif;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        return -1;
    }

    buf.cdp_vdev_param_peer_tid_latency_enable = val;
    if (cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc),
        wlan_vdev_get_id(vap->vdev_obj), CDP_ENABLE_PEER_TID_LATENCY,
        buf) != QDF_STATUS_SUCCESS) {
        return 1;

    }
    return 0;
}

/*
 * ieee80211_ucfg_get_peer_tid_latency_enable()
 *  This API gets peer tid latency
 */
int ieee80211_ucfg_get_peer_tid_latency_enable(void *osif)
{
    cdp_config_param_type buf = {0};
    osif_dev *osifp = (osif_dev *)osif;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic) {
        return -1;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    if (!psoc) {
        return -1;
    }

    cdp_txrx_get_vdev_param(wlan_psoc_get_dp_handle(psoc),
        wlan_vdev_get_id(vap->vdev_obj), CDP_ENABLE_PEER_TID_LATENCY,
        &buf);

    return buf.cdp_vdev_param_peer_tid_latency_enable;
}

#endif

int ieee80211_ucfg_setwmmparams(void *osif, int wmmparam, int ac, int bss, int value)
{
    int retv = 0;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
#if defined (QCA_NSS_WIFI_OFFLOAD_SUPPORT) && defined (UMAC_VOW_DEBUG)
    struct ieee80211com *ic = vap->iv_ic;
#endif

    switch (wmmparam)
    {
        case IEEE80211_WMMPARAMS_CWMIN:
            if (value < 0 ||  value > 15) {
                retv = -EINVAL;
            } else if (value > ieee80211_ucfg_getwmmparams(
                                     osif, IEEE80211_WMMPARAMS_CWMAX, ac, bss)) {
                qdf_info("CWMIN can't be greater than CWMAX.");
                retv = -EINVAL;
            }
            else {
                retv = wlan_set_wmm_param(vap, WLAN_WME_CWMIN,
                        bss, ac, value);
            }
            break;
        case IEEE80211_WMMPARAMS_CWMAX:
            if (value < 0 ||  value > 15) {
                retv = -EINVAL;
            } else if (value < ieee80211_ucfg_getwmmparams(
                                     osif, IEEE80211_WMMPARAMS_CWMIN, ac, bss)) {
                qdf_info("CWMAX can't be lesser than CWMIN.");
                retv = -EINVAL;
            }
            else {
                retv = wlan_set_wmm_param(vap, WLAN_WME_CWMAX,
                        bss, ac, value);
            }
            break;
        case IEEE80211_WMMPARAMS_AIFS:
            if (value < 0 ||  value > 15) {
                retv = -EINVAL;
            }
            else {
                retv = wlan_set_wmm_param(vap, WLAN_WME_AIFS,
                        bss, ac, value);
            }
            break;
        case IEEE80211_WMMPARAMS_TXOPLIMIT:
            if (value < 0 ||  value > (8192 >> 5)) {
                retv = -EINVAL;
            }
            else {
                retv = wlan_set_wmm_param(vap, WLAN_WME_TXOPLIMIT,
                        bss, ac, value);
            }
            break;
        case IEEE80211_WMMPARAMS_ACM:
            if (value < 0 ||  value > 1) {
                retv = -EINVAL;
            }
            else {
                retv = wlan_set_wmm_param(vap, WLAN_WME_ACM,
                        bss, ac, value);
            }
            break;
        case IEEE80211_WMMPARAMS_NOACKPOLICY:
            if (value < 0 ||  value > 1) {
                retv = -EINVAL;
            }
            else {
                retv = wlan_set_wmm_param(vap, WLAN_WME_ACKPOLICY,
                        bss, ac, value);
            }
            break;
#if UMAC_VOW_DEBUG
        case IEEE80211_PARAM_VOW_DBG_CFG:
            {
                if(ac >= MAX_VOW_CLIENTS_DBG_MONITOR ) {
                    qdf_print("Invalid Parameter: Acceptable index range [0 - %d]",
                            MAX_VOW_CLIENTS_DBG_MONITOR-1);
                    retv = -EINVAL;
                } else {
                    osifp->tx_dbg_vow_peer[ac][0] = bss;
                    osifp->tx_dbg_vow_peer[ac][1] = value;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                    if (osifp->nss_wifiol_ctx && ic->nss_vops) {
                        ic->nss_vops->ic_osif_nss_vdev_vow_dbg_cfg(osifp, ac);
                    }
#endif
                }
            }
            break;
#endif

        default:
            return retv;
    }
    /* Reinitialise the vaps to update the wme params during runtime configuration  */
    if (!bss && retv == EOK) {
        retv = IS_UP(osifp->netdev) ? osif_vap_init(osifp,osifp->netdev,0) : 0;
    }
    /*
     * As we are not doing VAP reinit in case of (bss & retv == EOK),
     * we need to update beacon template
     */
    if (bss && retv == EOK) {

        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            vap->iv_mbss.non_tx_profile_change = true;
            vap->iv_mbss.mbssid_update_ie = true;
        }

        wlan_vdev_beacon_update(vap);
    }
    return retv;
}


int ieee80211_ucfg_getwmmparams(void *osif, int wmmparam, int ac, int bss)
{
    int value = 0;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;

    switch (wmmparam)
    {
        case IEEE80211_WMMPARAMS_CWMIN:
            value = wlan_get_wmm_param(vap, WLAN_WME_CWMIN,
                    bss, ac);
            break;
        case IEEE80211_WMMPARAMS_CWMAX:
            value = wlan_get_wmm_param(vap, WLAN_WME_CWMAX,
                    bss, ac);
            break;
        case IEEE80211_WMMPARAMS_AIFS:
            value = wlan_get_wmm_param(vap, WLAN_WME_AIFS,
                    bss, ac);
            break;
        case IEEE80211_WMMPARAMS_TXOPLIMIT:
            value = wlan_get_wmm_param(vap, WLAN_WME_TXOPLIMIT,
                    bss, ac);
            break;
        case IEEE80211_WMMPARAMS_ACM:
            value = wlan_get_wmm_param(vap, WLAN_WME_ACM,
                    bss, ac);
            break;
        case IEEE80211_WMMPARAMS_NOACKPOLICY:
            value = wlan_get_wmm_param(vap, WLAN_WME_ACKPOLICY,
                    bss, ac);
            break;
        default:
            value = -EINVAL;
            break;
    }

    return value;

}

int ieee80211_ucfg_set_muedcaparams(void *osif, uint8_t muedcaparam,
        uint8_t ac, uint8_t value)
{

    int ret = 0;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;

    /* Check for the lower and upper bounds for paramID */
    if((muedcaparam < IEEE80211_MUEDCAPARAMS_ECWMIN) &&
            (muedcaparam > IEEE80211_MUEDCAPARAMS_TIMER)) {
        qdf_print("%s: paramID should be between %d-%d.",
                __func__, IEEE80211_MUEDCAPARAMS_ECWMIN,
                IEEE80211_MUEDCAPARAMS_TIMER);
        return -EINVAL;
    }

    /* Check for the lower and upper bounds for AC */
    if(ac >= MUEDCA_NUM_AC) {
        qdf_print("%s: AC should be less than %d.",
                __func__, MUEDCA_NUM_AC);
        return -EINVAL;
    }

    switch (muedcaparam)
    {

        case IEEE80211_MUEDCAPARAMS_ECWMIN:
            if(value > MUEDCA_ECW_MAX) {
                qdf_print("%s: ECWMIN should be less than %d.",
                        __func__, MUEDCA_ECW_MAX);
                ret =  -EINVAL;
            }
            else {
                ret = wlan_set_muedca_param(vap, WLAN_MUEDCA_ECWMIN,
                        ac, value);
            }
            break;

        case IEEE80211_MUEDCAPARAMS_ECWMAX:
            if(value > MUEDCA_ECW_MAX) {
                qdf_print("%s: ECWMAX should be less than equal %d.",
                        __func__, MUEDCA_ECW_MAX);
                ret =  -EINVAL;
            }
            else {
                ret = wlan_set_muedca_param(vap, WLAN_MUEDCA_ECWMAX,
                        ac, value);
            }
            break;

        case IEEE80211_MUEDCAPARAMS_AIFSN:
            if(value > MUEDCA_AIFSN_MAX) {
                qdf_print("%s: AIFSN should less than equal to %d.",
                        __func__, MUEDCA_AIFSN_MAX);
                ret =  -EINVAL;
            }
            else {
                ret = wlan_set_muedca_param(vap, WLAN_MUEDCA_AIFSN,
                        ac, value);
            }
            break;

        /* ACM value set to 1 signifies admission control is enabled,
         * while a value of 0 signifies admission control is disabled for
         * the corresponding AC. */
        case IEEE80211_MUEDCAPARAMS_ACM:
            if(value > 1) {
                qdf_print("%s:ACM value should be 0 or 1.", __func__);
                ret =  -EINVAL;
            }
            else {
                ret = wlan_set_muedca_param(vap, WLAN_MUEDCA_ACM,
                        ac, value);
            }
            break;

        case IEEE80211_MUEDCAPARAMS_TIMER:
            if(value > MUEDCA_TIMER_MAX) {
                qdf_print("%s:Timer value should be less than equal to %d.",
                        __func__, MUEDCA_TIMER_MAX);
                ret =  -EINVAL;
            }
            else {
                ret = wlan_set_muedca_param(vap, WLAN_MUEDCA_TIMER,
                        ac, value);
            }
            break;

        default:
            ret = -EINVAL;
            break;

    }

    if(vap->iv_he_muedca == IEEE80211_MUEDCA_STATE_ENABLE) {
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            vap->iv_mbss.mbssid_update_ie = true;
            vap->iv_mbss.non_tx_profile_change = true;
        }
        wlan_vdev_beacon_update(vap);
    }

    return ret;
}

int ieee80211_ucfg_get_muedcaparams(void *osif, uint8_t muedcaparam, uint8_t ac)
{
    int value = 0;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;

    /* Check for the lower and upper bounds for paramID */
    if((muedcaparam < IEEE80211_MUEDCAPARAMS_ECWMIN) &&
            (muedcaparam > IEEE80211_MUEDCAPARAMS_TIMER)) {
        qdf_print("%s: paramID should be between %d-%d.",
                __func__, IEEE80211_MUEDCAPARAMS_ECWMIN,
                IEEE80211_MUEDCAPARAMS_TIMER);
        return -EINVAL;
    }

    /* Check for the lower and upper bounds for AC */
    if(ac >= MUEDCA_NUM_AC) {
        qdf_print("%s: AC should be less than %d.",
                __func__, MUEDCA_NUM_AC);
        return -EINVAL;
    }

    switch (muedcaparam)
    {

        case IEEE80211_MUEDCAPARAMS_ECWMIN:
            value = wlan_get_muedca_param(vap, WLAN_MUEDCA_ECWMIN, ac);
            break;

        case IEEE80211_MUEDCAPARAMS_ECWMAX:
            value = wlan_get_muedca_param(vap, WLAN_MUEDCA_ECWMAX, ac);
            break;

        case IEEE80211_MUEDCAPARAMS_AIFSN:
            value = wlan_get_muedca_param(vap, WLAN_MUEDCA_AIFSN, ac);
            break;

        case IEEE80211_MUEDCAPARAMS_ACM:
            value = wlan_get_muedca_param(vap, WLAN_MUEDCA_ACM, ac);
            break;

        case IEEE80211_MUEDCAPARAMS_TIMER:
            value = wlan_get_muedca_param(vap, WLAN_MUEDCA_TIMER, ac);
            break;

        default:
            value = -EINVAL;
            break;
    }

    return value;

}

static void
domlme(void *arg, wlan_node_t node)
{
    struct mlmeop *op = (struct mlmeop *)arg;

    switch (op->mlme->im_op) {

        case IEEE80211_MLME_DISASSOC:
            ieee80211_try_mark_node_for_delayed_cleanup(node);
            wlan_mlme_disassoc_request(op->vap,wlan_node_getmacaddr(node),op->mlme->im_reason);
            break;
        case IEEE80211_MLME_DEAUTH:
            IEEE80211_DPRINTF(op->vap, IEEE80211_MSG_AUTH, "%s: sending DEAUTH to %s, domlme deauth reason %d\n",
                    __func__, ether_sprintf(wlan_node_getmacaddr(node)), op->mlme->im_reason);
            ieee80211_try_mark_node_for_delayed_cleanup(node);
            wlan_mlme_deauth_request(op->vap,wlan_node_getmacaddr(node),op->mlme->im_reason);
            break;
        case IEEE80211_MLME_ASSOC:
            wlan_mlme_assoc_resp(op->vap,wlan_node_getmacaddr(node),op->mlme->im_reason, 0, NULL);
            break;
        case IEEE80211_MLME_REASSOC:
            wlan_mlme_assoc_resp(op->vap,wlan_node_getmacaddr(node),op->mlme->im_reason, 1, NULL);
            break;
        default:
            break;
    }
}

int
ieee80211_ucfg_setmlme(struct ieee80211com *ic, void *osif, struct ieee80211req_mlme *mlme)
{
    struct ieee80211_app_ie_t optie;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    struct ieee80211vap *tempvap = NULL;
    struct ieee80211_node *ni = NULL;

    if ((vap == NULL) || (ic == NULL) ) {
        return -EINVAL;
    }

    optie.ie = &mlme->im_optie[0];
    optie.length = mlme->im_optie_len;

    switch (mlme->im_op) {
        case IEEE80211_MLME_ASSOC:
            /* set dessired bssid when in STA mode accordingly */
            if (wlan_vap_get_opmode(vap) != IEEE80211_M_STA &&
                    osifp->os_opmode != IEEE80211_M_P2P_DEVICE &&
                    osifp->os_opmode != IEEE80211_M_P2P_CLIENT) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                        "[%s] non sta mode, skip to set bssid\n", __func__);
            } else {
                u_int8_t des_bssid[QDF_MAC_ADDR_SIZE];

                if (!IS_NULL_ADDR(mlme->im_macaddr)) {
                    /*If AP mac to which our sta vap is trying to connect has
                    same mac as one of our ap vaps ,dont set that as sta bssid */
                    TAILQ_FOREACH(tempvap, &ic->ic_vaps, iv_next) {
                        if (tempvap->iv_opmode == IEEE80211_M_HOSTAP && IEEE80211_ADDR_EQ(tempvap->iv_myaddr,mlme->im_macaddr)) {
                                qdf_print("[%s] Mac collision for [%s]",__func__,ether_sprintf(mlme->im_macaddr));
                                return -EINVAL;
                        }
                    }
                    IEEE80211_ADDR_COPY(des_bssid, &mlme->im_macaddr[0]);
                    wlan_aplist_set_desired_bssidlist(vap, 1, &des_bssid);
                    qdf_print("[%s] set desired bssid %02x:%02x:%02x:%02x:%02x:%02x",__func__,des_bssid[0],
                            des_bssid[1],des_bssid[2],des_bssid[3],des_bssid[4],des_bssid[5]);
                }
            }

            if (osifp->os_opmode == IEEE80211_M_STA ||
                    (u_int8_t)osifp->os_opmode == IEEE80211_M_P2P_GO ||
                    (u_int8_t)osifp->os_opmode == IEEE80211_M_P2P_CLIENT ||
                    osifp->os_opmode == IEEE80211_M_IBSS) {
                vap->iv_mlmeconnect=1;
                if (wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE)
                                          & (uint32_t)((1 << WLAN_CRYPTO_AUTH_SAE))) {
                   vap->iv_sta_external_auth_enabled = true;
                   osifp->app_filter |= IEEE80211_FILTER_TYPE_AUTH;
                } else {
                   vap->iv_sta_external_auth_enabled = false;
                }
                osif_vap_init(osifp, osifp->netdev, 0);
            }
            else if (osifp->os_opmode ==  IEEE80211_M_HOSTAP) {
                /* NB: the broadcast address means do 'em all */
                if (!IEEE80211_ADDR_EQ(mlme->im_macaddr, ieee80211broadcastaddr)) {
                ni = ieee80211_vap_find_node(vap, mlme->im_macaddr, WLAN_MLME_SB_ID);
                if (ni == NULL)
                    return -EINVAL;
                else
                    ic = ni->ni_ic;
                if (ieee80211_is_pmf_enabled(vap, ni)
                        && vap->iv_opmode == IEEE80211_M_HOSTAP
                        && (vap->iv_skip_pmf_reassoc_to_hostap > 0)
                        && (ni->ni_flags & IEEE80211_NODE_AUTH)) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                        "[%s] drop assoc resp for pmf client from hostapd\n",
                        __func__);
                } else {
                    wlan_mlme_assoc_resp(vap,mlme->im_macaddr,mlme->im_reason, 0, &optie);
                }
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                } else {
                    struct mlmeop iter_arg;
                    iter_arg.mlme = mlme;
                    iter_arg.vap = vap;
                    wlan_iterate_station_list(vap,domlme,&iter_arg);
                }
            }
            else
                return -EINVAL;
            break;
        case IEEE80211_MLME_REASSOC:
            if (osifp->os_opmode == IEEE80211_M_STA ||
                    osifp->os_opmode == IEEE80211_M_P2P_GO ||
                    osifp->os_opmode == IEEE80211_M_P2P_CLIENT) {
                osif_vap_init(osifp, osifp->netdev, 0);
            }
            else if (osifp->os_opmode ==  IEEE80211_M_HOSTAP) {
                /* NB: the broadcast address means do 'em all */
                if (!IEEE80211_ADDR_EQ(mlme->im_macaddr, ieee80211broadcastaddr)) {
                ni = ieee80211_vap_find_node(vap, mlme->im_macaddr, WLAN_MLME_SB_ID);
                if (ni == NULL)
                    return -EINVAL;
                else
                    ic = ni->ni_ic;
                if(ieee80211_is_pmf_enabled(vap, ni) &&
                        vap->iv_opmode == IEEE80211_M_HOSTAP &&
                        (vap->iv_skip_pmf_reassoc_to_hostap > 0) &&
                        (ni->ni_flags & IEEE80211_NODE_AUTH))
                {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                        "[%s] drop assoc resp for pmf client from hostapd\n",
                        __func__);
                    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                    return 1; /* drop assoc resp for pmf client from hostapd */
                }
                    wlan_mlme_assoc_resp(vap,mlme->im_macaddr,mlme->im_reason, 1, &optie);
                    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                } else {
                    struct mlmeop iter_arg;
                    iter_arg.mlme = mlme;
                    iter_arg.vap = vap;
                    wlan_iterate_station_list(vap,domlme,&iter_arg);
                }
            }
            else
                return -EINVAL;
            break;
        case IEEE80211_MLME_AUTH_FILS:
        case IEEE80211_MLME_AUTH:
            if (osifp->os_opmode != IEEE80211_M_HOSTAP) {
                return -EINVAL;
            }
            /* NB: ignore the broadcast address */
            if (!IEEE80211_ADDR_EQ(mlme->im_macaddr, ieee80211broadcastaddr)) {
#if WLAN_SUPPORT_FILS
                if(mlme->im_op == IEEE80211_MLME_AUTH_FILS && wlan_fils_is_enable(vap->vdev_obj)) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_FILS,
                                "%s op : 0x%x ie.len : %d mac %s\n",
                                __func__, mlme->im_op, optie.length,
                                ether_sprintf(mlme->im_macaddr));
                    if (wlan_mlme_auth_fils(vap->vdev_obj, &mlme->fils_aad, mlme->im_macaddr)) {
                        qdf_print("%s: FILS crypto registration failed", __func__);
                    } else {
                        qdf_print("%s: FILS crypto registered successfully", __func__);
                    }
                }
#endif
                wlan_mlme_auth(vap,mlme->im_macaddr,mlme->im_seq,mlme->im_reason, NULL, 0, &optie);
            }
            break;
        case IEEE80211_MLME_DISASSOC:
        case IEEE80211_MLME_DEAUTH:
            switch (osifp->os_opmode) {
                case IEEE80211_M_STA:
                case IEEE80211_M_P2P_CLIENT:
                    //    if (mlme->im_op == IEEE80211_MLME_DISASSOC && !osifp->is_p2p_interface) {
                    //        return -EINVAL; /*fixme darwin does this, but linux did not before? */
                    //    }

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
                    if (mlo_is_mld_sta(vap->vdev_obj) &&
                        QDF_IS_STATUS_SUCCESS(wlan_mlme_mlo_is_stacac_running(vap->vdev_obj))) {
                        return -EINVAL;
                    } else if(wlan_mlme_is_stacac_running(vap)) {
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "Do not stop the BSS STA CAC is on\n");
                        return -EINVAL;
                    } else {
                        osif_vap_stop_wrapper(osifp);
                    }
#else
                    osif_vap_stop_wrapper(osifp);
#endif
                    break;
                case IEEE80211_M_HOSTAP:
                case IEEE80211_M_IBSS:
                    if (ic){
                        /*No need to put  any check for Broadcast as for braodcast ni == NULL */
                        if (IEEE80211_ADDR_EQ(mlme->im_macaddr, vap->iv_myaddr)) {
                            qdf_print("Cannot send Disassoc to self ");
                            return -EINVAL;
                        }
                        /* User requested for abrupt STA kickout.
                         * Skip sending deauth/disassoc to STA
                         */
                        if (mlme->im_reason == IEEE80211_REASON_ASSOC_LEAVE) {
                            ni = ieee80211_vap_find_node(vap, mlme->im_macaddr,
                                                         WLAN_MLME_SB_ID);
                            if (!ni) {
                                qdf_info("No peer found with mac "QDF_MAC_ADDR_FMT,
                                         QDF_MAC_ADDR_REF(mlme->im_macaddr));
                                break;
                            }
#ifdef WLAN_FEATURE_11BE_MLO
                            if (wlan_peer_is_mlo(ni->peer_obj))
                                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
#endif
                            IEEE80211_NODE_LEAVE(ni);
                            IEEE80211_DELIVER_EVENT_MLME_DISASSOC_COMPLETE(vap,
                                            mlme->im_macaddr,
                                            IEEE80211_REASON_LOCAL,
                                            IEEE80211_STATUS_SUCCESS, NULL);
                            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                            break;
                        }
#if WLAN_SUPPORT_SPLITMAC
                        /*  mark client for deletion for non-splitmac enabled.
                         *
                         *  For Split-mac enableded case, mark client for deletion
                         *  only or for deauth.
                         */
                        if (!splitmac_is_enabled(vap->vdev_obj) ||
                            mlme->im_op == IEEE80211_MLME_DEAUTH) {
#endif
                            ni = ieee80211_vap_find_node(vap, mlme->im_macaddr,
                                                         WLAN_MLME_SB_ID);
                            if (ni) {
                                /* If DA is non_bss unicast address, mark this node
                                 * delayed node cleanup candidate.
                                 */
                                ieee80211_try_mark_node_for_delayed_cleanup(ni);
                                /* claim node immediately */
                                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                            }
#if WLAN_SUPPORT_SPLITMAC
                        }
#endif
                    }
                    /* the 'break' statement for this case is intentionally removed, to make sure that this fall through next statement */
                case IEEE80211_M_P2P_GO:
                    /* NB: the broadcast address means do 'em all */
                    if (!IEEE80211_ADDR_EQ(mlme->im_macaddr, ieee80211broadcastaddr)) {
                        if (mlme->im_op == IEEE80211_MLME_DEAUTH) {
                            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH, "%s: sending DEAUTH to %s, mlme deauth reason %d\n",
                                    __func__, ether_sprintf(mlme->im_macaddr), mlme->im_reason);
                            wlan_mlme_deauth_request(vap,mlme->im_macaddr,mlme->im_reason);
                        }
                        if (mlme->im_op == IEEE80211_MLME_DISASSOC) {
                            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC, "%s: sending DISASSOC to %s, mlme disassoc reason %d\n",
                                    __func__, ether_sprintf(mlme->im_macaddr), mlme->im_reason);
                            wlan_mlme_disassoc_request(vap,mlme->im_macaddr,mlme->im_reason);
                        }
                    } else {
                        if (wlan_vap_is_pmf_enabled(vap)) {
                            if (mlme->im_op == IEEE80211_MLME_DEAUTH) {
                                IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH, "%s: sending DEAUTH to %s, mlme deauth reason %d\n",
                                        __func__, ether_sprintf(mlme->im_macaddr), mlme->im_reason);
                                wlan_mlme_deauth_request(vap,mlme->im_macaddr,mlme->im_reason);
                            }
                            if (mlme->im_op == IEEE80211_MLME_DISASSOC) {
                                wlan_mlme_disassoc_request(vap,mlme->im_macaddr,mlme->im_reason);
                            }
                        } else {
                            struct mlmeop iter_arg;
                            iter_arg.mlme = mlme;
                            iter_arg.vap = vap;
                            wlan_iterate_station_list(vap,domlme,&iter_arg);
                        }
                    }
                    break;
                default:
                    return -EINVAL;
            }
            break;
        case IEEE80211_MLME_AUTHORIZE:
        case IEEE80211_MLME_UNAUTHORIZE:
            if (osifp->os_opmode != IEEE80211_M_HOSTAP &&
                    osifp->os_opmode != IEEE80211_M_STA &&
                    osifp->os_opmode != IEEE80211_M_P2P_GO) {
                return -EINVAL;
            }
            if (mlme->im_op == IEEE80211_MLME_AUTHORIZE) {
                wlan_node_authorize(vap, 1, mlme->im_macaddr);
            } else {
                wlan_node_authorize(vap, 0, mlme->im_macaddr);
            }
            break;
        case IEEE80211_MLME_CLEAR_STATS:
#ifdef notyet

            if (vap->iv_opmode != IEEE80211_M_HOSTAP)
                return -EINVAL;
            ni = ieee80211_find_node(ic, mlme->im_macaddr, WLAN_MLME_SB_ID);
            if (ni == NULL)
                return -ENOENT;

            /* clear statistics */
            if (cdp_host_reset_peer_stats(wlan_pdev_get_psoc(ic->ic_pdev_obj),
                                      wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj)) != QDF_STATUS_SUCCESS) {
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }

            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
#endif
            break;

        case IEEE80211_MLME_STOP_BSS:
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
            if (mlo_is_mld_sta(vap->vdev_obj) &&
                QDF_IS_STATUS_SUCCESS(wlan_mlme_mlo_is_stacac_running(vap->vdev_obj))) {
                return -EINVAL;
            } else if(wlan_mlme_is_stacac_running(vap)) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "Do not stop the BSS STA CAC is on\n");
                return -EINVAL;
            } else {
                osif_vap_stop_wrapper(osifp);
            }
#else
            osif_vap_stop_wrapper(osifp);
#endif
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

#if QCA_SUPPORT_GPR
int ieee80211_ucfg_send_gprparams(wlan_if_t vap, uint8_t command)
{
    struct ieee80211com *ic = vap->iv_ic;
    int retv = -EINVAL;
#if UMAC_SUPPORT_ACFG
    acfg_netlink_pvt_t *acfg_nl;

    if ( !ic || !ic->ic_acfg_handle ) {
        return -EINVAL;
    }
    acfg_nl = (acfg_netlink_pvt_t *)ic->ic_acfg_handle;

    /* Give access to only one app on one radio */
    if(qdf_semaphore_acquire_intr(acfg_nl->sem_lock)){
        /*failed to acquire mutex*/
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
                 "%s(): failed to acquire mutex!\n", __func__);
        return -EAGAIN;
    }
#endif

    switch (command)
    {
        case IEEE80211_GPR_DISABLE:
            if (ic->ic_gpr_enable) {
                if (vap->iv_opmode == IEEE80211_M_HOSTAP ) {
                    if (vap->iv_gpr_enable == 1) {
                        vap->iv_gpr_enable = 0;
                        ic->ic_gpr_enable_count--;
                    } else {
                        qdf_err("GPR already stopped on this vap or not started yet !\n");
                    }
                    if (ic->ic_gpr_enable_count == 0) {
                        qdf_hrtimer_kill(&ic->ic_gpr_timer);
                        qdf_mem_free(ic->acfg_frame);
                        ic->acfg_frame = NULL;
                        ic->ic_gpr_enable = 0;
                        qdf_err("\nStopping GPR timer as this is last vap with gpr \n");
                    }
                    retv = EOK;
                } else {
                    qdf_err("Invalid! Allowed only in HOSTAP mode\n");
                }
            } else {
                qdf_err("GPR not started on any of vap, start GPR first \n");
            }
            break;
        case IEEE80211_GPR_ENABLE:
            if (ic->ic_gpr_enable) {
                if (vap->iv_opmode == IEEE80211_M_HOSTAP ) {
                    if (vap->iv_gpr_enable == 0) {
                        vap->iv_gpr_enable = 1;
                        ic->ic_gpr_enable_count++;
                        retv = EOK;
                    } else {
                        qdf_err("GPR already started on this vap \n");
                    }
                } else {
                    qdf_err("Invalid! Allowed only in HOSTAP mode\n");
                }
            } else {
                qdf_err("GPR not started on any of vap, start GPR first \n");
            }
            break;
        case IEEE80211_GPR_PRINT_STATS:
            if (ic->ic_gpr_enable) {
                qdf_hrtimer_data_t *gpr_hrtimer = &ic->ic_gpr_timer;
                qdf_err("Timer STATS for GPR \n");
                if (vap->iv_opmode == IEEE80211_M_HOSTAP ) {
                    qdf_err("------------------------------------\n");
                    qdf_err("| GPR on this vap enabled    - %d   |\n", vap->iv_gpr_enable);
                    qdf_err("| GPR on radio enabled       - %d   |\n", ic->ic_gpr_enable);
                    qdf_err("| GPR state                  - %s   |\n",
                            (qdf_hrtimer_active(&ic->ic_gpr_timer))?"Active":
                            "Dormant");
                    qdf_err("| GPR Minimum Period         - %d   |\n", DEFAULT_MIN_GRATITOUS_PROBE_RESP_PERIOD);
                    qdf_err("| Current Timestamp          - %lld |\n",
                            qdf_ktime_to_ns(qdf_ktime_get()));
                    qdf_err("| Timer expires in           - %lld |\n",
                            qdf_ktime_to_ns(qdf_ktime_add(qdf_ktime_get(),
                                     qdf_hrtimer_get_remaining(gpr_hrtimer))));
                    qdf_err("| GPR timer start count      - %u   |\n", ic->ic_gpr_timer_start_count);
                    qdf_err("| GPR timer resize count     - %u   |\n", ic->ic_gpr_timer_resize_count);
                    qdf_err("| GPR timer send count       - %u   |\n", ic->ic_gpr_send_count);
                    qdf_err("| GPR timer user period      - %u   |\n", ic->ic_period);
                    qdf_err("| GPR enabled vap count      - %u   |\n", ic->ic_gpr_enable_count);
                    qdf_err("------------------------------------\n");
                    retv = EOK;
                } else {
                    qdf_err("Invalid! Allowed only in HOSTAP mode\n");
                }
            } else {
                qdf_err("GPR not running on any of the Vaps\n");
            }
            break;
        case IEEE80211_GPR_CLEAR_STATS:
            if (ic->ic_gpr_enable) {
                qdf_err("%s %d Timer CLEAR STATS for GPR \n",__func__,__LINE__);
                ic->ic_gpr_timer_start_count  = 0;
                ic->ic_gpr_timer_resize_count = 0;
                ic->ic_gpr_send_count         = 0;
                retv = EOK;
            } else {
                qdf_err("GPR not running on any of the Vaps\n");
            }
            break;
        default:
            qdf_err("Invalid command type\n");
    }
#if UMAC_SUPPORT_ACFG
    qdf_semaphore_release(acfg_nl->sem_lock);
#endif
    return retv;
}
#endif

int ieee80211_ucfg_send_probereq(wlan_if_t vap, int val)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni = NULL;
    u_int8_t da[IEEE80211_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    u_int8_t bssid[IEEE80211_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    int retval = EOK;

    if (val == 1) {
        if (vap->iv_opmode == IEEE80211_M_STA) {
            if ((ic->ic_strict_pscan_enable &&
                IEEE80211_IS_CHAN_PASSIVE(ic->ic_curchan))) {
                    qdf_err("strict passive scan enabled cant sent probe ignoring");
                    retval = -EINVAL;
            } else {
                ni = ieee80211_ref_bss_node(vap, WLAN_OSIF_SCAN_ID);
                if (ni) {
                    retval = ieee80211_send_probereq(ni, vap->iv_myaddr, da,
                                bssid, ni->ni_essid, ni->ni_esslen,
                                vap->iv_opt_ie.ie, vap->iv_opt_ie.length);

                    ieee80211_free_node(ni, WLAN_OSIF_SCAN_ID);
                }
            }
        } else {
            qdf_err("VAP is not STA");
            retval = -EINVAL;
        }
    } else {
        qdf_err("Value must be 1");
        retval = -EINVAL;
    }

    return retval;
}

struct ieee80211vap *ieee80211_ucfg_get_txvap(struct ieee80211com *ic, int grp_id, mbss_mode_t current_mode)
{
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!ic) {
        return NULL;
    }

    mbss_ctx = ic->ic_mbss_ctx;
    if (!mbss_ctx) {
        return NULL;
    }
    ic_mbss = &mbss_ctx->ic_mbss[grp_id];

    if (current_mode == MBSS_MODE_COHOSTED) {
        if (ic->ic_wideband_capable) {
            if (ic_mbss->target_transmit_vap) {
                mbss_debug("target_txvap_set: %d", ic_mbss->target_transmit_vap->iv_unit);
                return ic_mbss->target_transmit_vap;
            }
            else
                goto find_first_ap_vap;
        } else {
            return NULL;
        }
    } else {
        return ic_mbss->transmit_vap;
    }

find_first_ap_vap:
    return ieee80211_mbss_get_first_ap_vap(ic, true, grp_id); /* get first vap in that group */
}

int ieee80211_ucfg_set_txvap(wlan_if_t vap)
{
    struct ieee80211com *ic;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!vap) {
        mbss_err("vap is NULL");
        return -1;
    }

    ic = vap->iv_ic;
    if (!ic) {
        mbss_err("ic is NULL");
        return -1;
    }

    if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
        mbss_debug("VAP%d is not AP VAP", vap->iv_unit);
        return 0;
    }
    else {
        if (IEEE80211_IS_SPECIAL_VAP(vap)) {
            mbss_err("VAP%d is either smart monitor %d or special vap %d, not configuring",
                     vap->iv_unit, IEEE80211_IS_SMART_MON_VAP(vap),
                     IEEE80211_IS_SPECIAL_VAP(vap));
            return 0;
        }
    }

    if (IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        mbss_err("Set transmit vap :%d  not applicable for bridge vap: %s",
                vap->iv_unit, vap->iv_netdev_name);
        return 0;
    }

    if (!wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                            WLAN_PDEV_F_MBSS_IE_ENABLE)) {
        mbss_debug("MBSSID is not enabled");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[vap->iv_mbss.group_id];

    if (ic_mbss->transmit_vap) {
        mbss_info("%s: vap%d is already configured as Tx vap",
                 ic_mbss->transmit_vap->iv_netdev_name,
                 ic_mbss->transmit_vap->iv_unit);
        if (vap == ic_mbss->transmit_vap)
            return 0;

        return -1;
    }

    if (ieee80211_mbssid_txvap_set(vap)) {
        mbss_err("vap%d MBSSID Tx vap setup failed", vap->iv_unit);
        return -1;
    }

    /* multi group notifications to user space */
    wlan_cfg80211_mbssid_multigroup_tx_vdev_notification(vap, 1);
    return 0;
}

int ieee80211_ucfg_reset_txvap(wlan_if_t vap, uint8_t force)
{
    struct ieee80211com *ic;
    struct ieee80211vap *tmpvap = NULL;
    uint8_t grp_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!vap) {
        mbss_err("vap is NULL");
        return -1;
    }

    ic = vap->iv_ic;
    if (!ic) {
        mbss_err("ic is NULL");
        return -1;
    }

    if (!wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                            WLAN_PDEV_F_MBSS_IE_ENABLE)) {
        mbss_err("MBSSID is not enabled");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    grp_id = vap->iv_mbss.group_id;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    if (!ic_mbss->transmit_vap) {
        mbss_err("No vap is configured as Tx vap");
        return -1;
    }

    if (IEEE80211_IS_MLO_BRIDGE_VAP(ic_mbss->transmit_vap)) {
        mbss_err("WARN: bridge vap cannot be transmit_vap, vap:%d :%s , transmit_vap:%d :%s ",
                vap->iv_unit, vap->iv_netdev_name,
                ic_mbss->transmit_vap->iv_unit, ic_mbss->transmit_vap->iv_netdev_name);
        return 0;
    }

    /* This if block prevents user to reset Tx VDEV if any VDEV's dev->flags
     * marked as IFF_UP, this is skipped, if tx vdev change requested
     * internally
     */
    if (!force) {
         TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
             if (tmpvap->iv_opmode == IEEE80211_M_HOSTAP) {
                 struct net_device *tmpdev = get_cfg80211_notification_ndev((osif_dev *)tmpvap->iv_ifp);
                 if (IS_UP(tmpdev) && (tmpvap->iv_mbss.group_id == grp_id)) {
                     mbss_err("VAP %d is in UP processing, can't reset Tx vap", tmpvap->iv_unit);
                     return -1;
                 }
             }
         }
    }

    if (ieee80211_mbssid_txvap_reset(vap)) {
        mbss_err("vap%d MBSSID Tx vap reset failed", vap->iv_unit);
        return -1;
    }

    /*
     * If successful and this VAP was the temp Tx VAP, then clear it
     * NOTE: This behavior is applicable to wideband radios only.
     */
    if (ic->ic_wideband_capable &&
            (ic_mbss->target_transmit_vap == vap)) {
        ic_mbss->target_transmit_vap = NULL;
    }

    return 0;
}

int ieee80211_ucfg_bringdown_txvap(wlan_if_t vap)
{
     struct ieee80211com *ic = vap->iv_ic;
     struct ieee80211vap *tmpvap = NULL;
     struct ieee80211vap *lastvap = NULL;
     int retval;
     uint8_t active_vap_cnt = 0;
     uint8_t grp_id = vap->iv_mbss.group_id;
     struct mbssid_context *mbss_ctx;
     struct mbssid_info *ic_mbss;

    if (!ic) {
        mbss_err("ic is NULL");
        return -1;
    }

    if (!wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                            WLAN_PDEV_F_MBSS_IE_ENABLE)) {
        mbss_debug("MBSSID is not enabled");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    if (ic_mbss->transmit_vap != vap) {
        mbss_debug("VAP %d is not configured as Tx vap", vap->iv_unit);
        return -1;
    }

    /* send notifications before dev_close */
    wlan_cfg80211_mbssid_multigroup_tx_vdev_notification(vap, 0);

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        struct net_device *tmpdev = get_cfg80211_notification_ndev((osif_dev *)tmpvap->iv_ifp);
        if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP) &&
            (tmpvap->iv_mbss.group_id == grp_id) &&
            ((vap == tmpvap) || (IS_UP(tmpdev)))) {
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_vdev_mlme_is_mlo_ap(tmpvap->vdev_obj))
                osif_vap_check_blocking_cmd_on_mlo(tmpvap->vdev_obj);
#endif
            wlan_mlme_stop_vdev(tmpvap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);

#ifdef WLAN_FEATURE_11BE_MLO
            if (!wlan_vdev_mlme_op_flags_get(vap->vdev_obj,
                                    WLAN_VDEV_OP_MLO_STOP_LINK_DEL))
                osif_vap_bringdown_mlo(tmpvap->vdev_obj);
#endif
            if ((ic_mbss->transmit_vap) &&
                (ic_mbss->transmit_vap != tmpvap)) {
                 lastvap = tmpvap;
                 mbss_err("last vap id %d", lastvap->iv_unit);
            }

            active_vap_cnt++;
        }

    }

    mbss_ctx->flags = grp_id;
    retval = wlan_pdev_wait_to_bringdown_vdevs(ic, ALL_AP_VDEVS_PER_GP);
    if (retval == QDF_STATUS_E_INVAL) {
        mbss_err("Bring down all AP VAP failed");
        return -1;
    }

    /*
     * The Tx-VAP params need to be restored in the return paths for fail cases
     * below
     */
    if (!ieee80211_ic_mbss_automode_is_clear(ic) && lastvap) {
        ieee80211_ucfg_copy_txvap_param(vap, lastvap);
    }

    if ((active_vap_cnt == 1) && (vap == lastvap))
        return 0;

    if (ieee80211_ic_mbss_automode_is_clear(ic) || !lastvap)
        return 0;

    if (ic->ic_wifi_down_ind) {
        mbss_info(" wifi down ind is set, new Tx vdev config is skipped");
        return 0;
    }

    if (ieee80211_ucfg_set_txvap(lastvap)) {
        mbss_err("set Tx VAP %d is failed", lastvap->iv_unit);
        return -1;
    }

    ic->ic_ema_config_init(lastvap);

    /* Bringup all remaining non tx vaps belongs to specific group */
    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        struct net_device *tmpdev = get_cfg80211_notification_ndev((osif_dev *)tmpvap->iv_ifp);
        if (tmpvap->iv_mbss.group_id == grp_id) {
            if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP) && (tmpvap != vap)) {
                retval = IS_UP(tmpdev) ? osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpdev, RESCAN) : 0;
            }
        }
    }

    return 0;
}

/**
 * ieee80211_ucfg_copy_txvap_param: Helper function to copy Tx VDEV dependent
 * params from previous Tx VDEV to new Tx VDEV
 * @last_txvap:: Previous Tx VDEV
 * @vap: New Tx VDEV
 *
 * Return: void
 */
void ieee80211_ucfg_copy_txvap_param(struct ieee80211vap *last_txvap,
                                     struct ieee80211vap *vap)
{
    uint32_t prb_rsp_en_period = 0;
#ifdef WLAN_SUPPORT_FILS
    uint32_t fils_enable = 0;
    uint32_t fils_period = 0;
    uint32_t fils_en_period = 0;
#endif
#ifdef WLAN_SUPPORT_FILS
      /* Copy FILS enable period */
     fils_enable = wlan_get_param(last_txvap, IEEE80211_FEATURE_FILS);
     if (fils_enable) {
          fils_period = wlan_fd_get_fd_period(last_txvap->vdev_obj);
          fils_en_period = ucfg_fd_get_enable_period(fils_enable, fils_period);
          wlan_set_param(vap, IEEE80211_FEATURE_FILS, fils_en_period);
     }
#endif
     /* Copy Probe response period */
     vap->iv_he_6g_bcast_prob_rsp = last_txvap->iv_he_6g_bcast_prob_rsp;
     vap->iv_he_6g_bcast_prob_rsp_intval = wlan_get_param(last_txvap,
                                          IEEE80211_CONFIG_6GHZ_BCAST_PROB_RSP);
     prb_rsp_en_period = ieee80211_get_prbrsp_en_period(
                                   vap->iv_he_6g_bcast_prob_rsp,
                                   vap->iv_he_6g_bcast_prob_rsp_intval);
     if (prb_rsp_en_period) {
         /* Send a template to FW if bcast probe response is to be enabled */
         wlan_set_param(vap, IEEE80211_CONFIG_6GHZ_BCAST_PROB_RSP,
                        prb_rsp_en_period);
     }

}

int ieee80211_ucfg_reset_nawds_txvap(wlan_if_t vap)
{
    struct ieee80211com *ic;
    struct ieee80211vap *tx_vap;
    struct ieee80211_nawds *nawds;
    struct net_device *tx_dev;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t grp_id;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct ieee80211vap *co_mld_vap;
    uint16_t num_links;
    uint8_t i;
#endif

    if (!vap) {
        mbss_err("vap is NULL");
        return -1;
    }

    ic = vap->iv_ic;
    if (!ic) {
        mbss_err("ic is NULL");
        return -1;
    }

    grp_id = vap->iv_mbss.group_id;
    if (!wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                    WLAN_PDEV_F_MBSS_IE_ENABLE)) {
        mbss_debug("MBSSID is not enabled");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    tx_vap = ic_mbss->transmit_vap;
    if (!tx_vap) {
        mbss_err("tx_vap is NULL");
        return -1;
    }

    nawds = &tx_vap->iv_nawds;
    tx_dev = ((osif_dev *)tx_vap->iv_ifp)->netdev;

    /* if vap's nawds mode is set to bridge or repeater (not DISABLED),
     * AND it is the first vap, then bring it down
     * and assign current (second) vap as tx vap. This avoids the scenario
     * of a non-beaconing tx vap since nawds vap can switch to
     * non-beaconing mode */
    if (IS_UP(tx_dev) &&
        (nawds->mode !=  IEEE80211_NAWDS_DISABLED))
    {
        int ret;
#ifdef WLAN_FEATURE_11BE_MLO
        bool mlo_nawds = ieee80211_mlo_is_mld_ap(tx_vap->vdev_obj);

        if (mlo_nawds)
            mlo_ap_get_active_vdev_list(tx_vap->vdev_obj, &num_links, vdev_list);
#endif

        /* bring down the tx vap */
        if (ieee80211_ucfg_bringdown_txvap(tx_vap))
            qdf_err("error with bringdown");

        /* set current vap as tx vap */
        ieee80211_ucfg_set_txvap(vap);

#ifdef WLAN_FEATURE_11BE_MLO
        if (mlo_nawds) {
            if (!num_links) {
                qdf_err("Error starting %s where MLO enabled, but 0 num links reported", tx_dev->name);
                return 0;
            }

            if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
                qdf_err("Error starting %s where MLO enabled, but invalid num links reported", tx_dev->name);

                if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                            QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
                    qdf_err("Error while trying to release VDEV references - investigate.");

                return 0;
            }

            /* Since partner vdevs are also brought down in ieee80211_ucfg_bringdown_txvap
             * reinit the partner vdevs as well here for MLO+NAWDS to work
             */
            for (i = 0; i < num_links; i++) {
                 if (!vdev_list[i])
                     continue;

                 co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
                 if (!co_mld_vap) {
                     mlo_release_vdev_ref(vdev_list[i]);
                     continue;
                 }

                 tx_dev = ((osif_dev *)co_mld_vap->iv_ifp)->netdev;
                 ret = osif_vap_init((osif_dev *)co_mld_vap->iv_ifp, tx_dev, RESCAN);
                 if (ret) {
                     qdf_warn("Error starting %s, ret: %d!!", tx_dev->name, ret);
                     mlo_release_vdev_ref(vdev_list[i]);
                     continue;
                 }

                 mlo_release_vdev_ref(vdev_list[i]);
            }
        } else {
#endif
            ret = osif_vap_init((osif_dev *)tx_vap->iv_ifp, tx_dev, RESCAN);
            if (ret)
                qdf_warn("Error starting %s, ret: %d!!", tx_dev->name, ret);
#ifdef WLAN_FEATURE_11BE_MLO
        }
#endif
        qdf_info("Done with bringing up nawds vap!");
    }

    return 0;
}

#define PARTIAL_BSSID_BIT_40_44_BYTE_INDEX 5
#define PARTIAL_BSSID_BIT_39_BYTE_INDEX 4

static void ieee80211_ucfg_iter_get_partial_bssid_bitmap(void *arg, wlan_if_t vap)
{
    u_int32_t  *value = (u_int32_t *) arg;
    uint8_t *vap_bssid = NULL;
    uint8_t partial_bssid_pos;
    uint32_t partial_bitmap;
    uint8_t bits_40_44, bit_39;

    if(vap && vap->iv_opmode == IEEE80211_M_HOSTAP) {
        /* Get vap bssid */
        wlan_vdev_obj_lock(vap->vdev_obj);
        vap_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
        wlan_vdev_obj_unlock(vap->vdev_obj);
        qdf_debug("vap_bssid:%s",ether_sprintf(vap_bssid));

        /* Get bits 40 to 44, the 5 LSB bits of 6th byte from vap bssid
         * Get bit 39, the MSB bit of 5th byte from vap bssid
         * Derive partial_bssid_pos using bits 39 to 44
         * Set respective bitmap according to partial_bssid_pos
         */
        bits_40_44 = QDF_GET_BITS(vap_bssid[PARTIAL_BSSID_BIT_40_44_BYTE_INDEX], 0, 5);
        bit_39 = QDF_GET_BITS(vap_bssid[PARTIAL_BSSID_BIT_39_BYTE_INDEX], 7, 1);
        partial_bssid_pos = (bits_40_44 << 1) | bit_39;

        /* Update LSB bitmap idx for partial_bssid_pos < 32 */
        if (partial_bssid_pos < 32) {
            partial_bitmap = 1L << partial_bssid_pos;
            value[0] |= partial_bitmap;
        }  else {
            /* Update MSB bitmap idx for partial_bssid_pos > 32 */
            partial_bitmap = 1L << (partial_bssid_pos - 32);
            value[1] |= partial_bitmap;
        }
        qdf_debug("bits_40_44:0x%x bit_39:0x%x partial_bssid_pos:%d"
                "partial_bitmap:0x%x bitmap idx LSB:0x%x bitmap idx MSB:0x%x",
                bits_40_44, bit_39, partial_bssid_pos,
                partial_bitmap, value[0], value[1]);
    }
}

int ieee80211_ucfg_get_partial_bssid_bitmap(struct ieee80211com *ic, uint32_t *value)
{
    if (!ic) {
        qdf_err("IC is null");
        return -EINVAL;
    }

    /* API to get the partial BSSIDs of all BSSIDs in MBSSID.
     * This is derived from bits 39 to 44 of BSSIDs of all ap vap interfaces in MBSSID.
     * Traverse through all BSSIDs in a radio and extract bits 39 to 44 from each BSSID.
     * Convert this 6-bit to an integer value, which is between 0 to 63. This value
     * indicates the bssid index or position in the partial BSSID bitmap that should
     * be set to 1.
     */
    /* Output will be 64-bit value represented in two 32 bits in Hex format.
     * The LSB indicates partial bssid position for 0 to 31
     * The MSB indicates partial bssid position for 32 to 63
     */

    wlan_iterate_vap_list(ic, ieee80211_ucfg_iter_get_partial_bssid_bitmap,(void *) value);

    qdf_debug(" value[0] LSB: 0x%x value[1] MSB:0x%x",value[0],value[1]);

    return 0;
}

int
ieee80211_ucfg_mu_snif(struct ieee80211com *ic, wlan_if_t vap,
		       struct ieee80211_wlanconfig *config)
{
	return ic->ic_vap_set_mu_snif(vap, &config->data.mu_snif);
}

#ifdef WLAN_FEATURE_11BE
bool ieee80211_ucfg_is_homogeneous_t2lm(
        struct wlan_t2lm_onging_negotiation_info *t2lm_info)
{
    uint16_t tid0_provisioned_links = 0;
    uint8_t dir = 0;
    uint8_t tid = 0;

    if (t2lm_info->category == WLAN_T2LM_CATEGORY_TEARDOWN)
        return true;

    if ((t2lm_info->category == WLAN_T2LM_CATEGORY_RESPONSE) &&
        (t2lm_info->t2lm_resp_type !=
         WLAN_T2LM_RESP_TYPE_PREFERRED_TID_TO_LINK_MAPPING))
        return true;

    for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++, tid = 0) {
        if (t2lm_info->t2lm_info[dir].direction == WLAN_T2LM_INVALID_DIRECTION)
            continue;

        if (t2lm_info->t2lm_info[dir].default_link_mapping)
            continue;

        tid0_provisioned_links =
            t2lm_info->t2lm_info[dir].hw_link_map_tid[tid];

        /* At least one link should be mapped to a given TID, else ignore the
         * T2LM config as it is a invalid config.
         */
        if (!tid0_provisioned_links) {
            t2lm_err("No links are mapped to the TIDs");
            return false;
        }

        /* Check if TID0 provisioned links are same as other TID's provisioned
         * links. If same, then it is a homogeneous mapping.
         */
        for (tid = 1; tid < T2LM_MAX_NUM_TIDS; tid++) {
            if (tid0_provisioned_links !=
                    t2lm_info->t2lm_info[dir].hw_link_map_tid[tid])
                return false;
        }
    }

    return true;
}

void ieee80211_parse_t2lm_config(
        struct wlan_t2lm_onging_negotiation_info *t2lm_negotiation,
        struct ieee80211_wlanconfig_t2lm *t2lm_config)
{
    struct t2lm_of_direction *t2lm_dir = NULL;
    uint8_t dir = 0;
    uint8_t tid = 0;
    uint8_t tid_num = 0;
    uint16_t link_map = 0;

    /* Copy the user provided DL/UL/BIDI T2LM config to the respective arrays */
    for (dir = 0; dir < T2LM_DIRECTION_MAX; dir++) {
        t2lm_dir = &t2lm_config->t2lm_direction[dir];
        if (t2lm_dir->direction > T2LM_DIRECTION_BIDI)
            continue;

        t2lm_negotiation->t2lm_info[t2lm_dir->direction].link_mapping_size =
            t2lm_dir->link_mapping_size;

        t2lm_negotiation->t2lm_info[t2lm_dir->direction].direction =
            t2lm_dir->direction;
        t2lm_negotiation->t2lm_info[t2lm_dir->direction].default_link_mapping =
            t2lm_dir->default_mapping;

        if (t2lm_dir->default_mapping)
            continue;

        for (tid = 0; tid < t2lm_dir->num_tids; tid++) {
            tid_num = t2lm_dir->map_tid_to_links[tid].tid_num;
            link_map = t2lm_dir->map_tid_to_links[tid].link_map;
            t2lm_negotiation->t2lm_info[t2lm_dir->direction].hw_link_map_tid[tid_num] =
                link_map;
        }
    }

    t2lm_negotiation->t2lm_info_present = true;
}

struct ieee80211_node *ieee80211_ucfg_get_mlo_assoc_link_node(
        struct wlan_mlo_peer_context *mlo_peer_ctx)
{
    struct wlan_objmgr_peer *assoc_peer = NULL;
    struct ieee80211_node *ni = NULL;
    QDF_STATUS status;

    if (!mlo_peer_ctx) {
        t2lm_err("mlo_peer_ctx is null");
        return NULL;
    }

    assoc_peer = wlan_mlo_peer_get_assoc_peer(mlo_peer_ctx);
    if (!assoc_peer) {
        t2lm_err("assoc_peer is null");
        return NULL;
    }

    status = wlan_objmgr_peer_try_get_ref(assoc_peer, WLAN_MLME_NB_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        t2lm_err("failed to get peer refcount");
        return NULL;
    }

    ni = wlan_peer_get_mlme_ext_obj(assoc_peer);
    if (!ni) {
        t2lm_err("ni is null");
        wlan_objmgr_peer_release_ref(assoc_peer, WLAN_MLME_NB_ID);
        return NULL;
    }

    return ni;
}
qdf_export_symbol(ieee80211_ucfg_get_mlo_assoc_link_node);

bool ieee80211_is_tids_mapped_to_assoc_link(struct ieee80211vap *vap,
        struct wlan_t2lm_onging_negotiation_info *t2lm)
{
    struct wlan_objmgr_pdev *pdev;
    int assoc_link_mask = 0;
    int dir = 0;
    int tid = 0;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    if (!pdev) {
        t2lm_err("null pdev");
        return false;
    }

    assoc_link_mask = BIT(ieee80211_mlo_get_pdev_hw_link_id(pdev));

    for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
        if (t2lm->t2lm_info[dir].direction > WLAN_T2LM_MAX_DIRECTION)
            continue;

        if (t2lm->t2lm_info[dir].default_link_mapping)
            continue;

        for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
            if ((t2lm->t2lm_info[dir].hw_link_map_tid[tid] &
                    assoc_link_mask) != assoc_link_mask) {
                return false;
            }
        }
    }

    return true;
}

bool ieee80211_ucfg_is_valid_t2lm_link_map(struct ieee80211vap *vap)
{
    struct wlan_t2lm_onging_negotiation_info *t2lm = &vap->t2lm_negotiation;
    enum wlan_t2lm_direction dir = WLAN_T2LM_INVALID_DIRECTION;
    uint8_t i = 0;
    uint16_t valid_link_mask = 0;
    int tid = 0;
    uint16_t provisioned_links = 0;
    bool is_valid_link_mask = false;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t vdev_count = 0;
    uint16_t removal_links = 0;

    t2lm = &vap->t2lm_negotiation;

    /* Get the valid hw_link_id map from ML vdev list */
    mlo_get_ml_vdev_list(vap->vdev_obj, &vdev_count, wlan_vdev_list);
    if (!vdev_count) {
        t2lm_err("Number of VDEVs under MLD is reported as 0");
        return false;
    }

    for (i = 0; i < vdev_count; i++) {
        vdev = wlan_vdev_list[i];
        if (!vdev) {
            t2lm_err("vdev is null");
            continue;
        }

        pdev = wlan_vdev_get_pdev(vdev);
        if (!pdev) {
            t2lm_err("pdev is null");
            continue;
        }

        if (((t2lm->category == WLAN_T2LM_CATEGORY_REQUEST) ||
             (t2lm->category == WLAN_T2LM_CATEGORY_RESPONSE &&
              t2lm->t2lm_resp_type == WLAN_T2LM_RESP_TYPE_PREFERRED_TID_TO_LINK_MAPPING)) &&
            (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS) ||
             wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE))) {
            /* VAP is in progress for link removal */
            removal_links |= BIT(ieee80211_mlo_get_pdev_hw_link_id(pdev));
        }

        valid_link_mask |= BIT(ieee80211_mlo_get_pdev_hw_link_id(pdev));
    }

    if (vdev_count) {
        if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list),
                    vdev_count, 0) != QDF_STATUS_SUCCESS) {
            t2lm_err("failed to release mlo vdev ref");
            return false;
        }
    }

    /* Check if the configured hw_link_id map is valid */
    for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
        if (t2lm->t2lm_info[dir].direction == WLAN_T2LM_INVALID_DIRECTION)
            continue;

        if (t2lm->t2lm_info[dir].default_link_mapping) {
            is_valid_link_mask = true;
            continue;
        }

        for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
            provisioned_links = t2lm->t2lm_info[dir].hw_link_map_tid[tid];
                /* compare removed & provisioned links */
            if (removal_links & provisioned_links) {
                    t2lm_err("Error!! Trying to map to link under removal process, removal_links:%x provisioned_links:%x",
                             removal_links, provisioned_links);
                    return false;
            }

            for (i = 0; i < WLAN_T2LM_MAX_NUM_LINKS; i++) {
                if (!(provisioned_links & (BIT(i))))
                    continue;

                if (valid_link_mask & (BIT(i))) {
                    is_valid_link_mask = true;
                    continue;
                } else {
                    return false;
                }
            }
        }
    }

    return is_valid_link_mask;
}

int ieee80211_ucfg_epcs(struct ieee80211com *ic, wlan_if_t vap,
                                       struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_epcs *epcs_config = &config->data.epcs;
    struct wlan_objmgr_vdev *vdev;
    struct ieee80211_node *ni = NULL;
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct qdf_mac_addr mac;
    int retval = 0;
    struct wlan_epcs_info epcs_info = {0};
    bool update_params = false;
    int iter = 0;
    uint8_t aci;
    struct wmeParams *wme;
    struct muedcaParams *muedca;
    char *aci_val;

    if (!vap->vdev_mlme->proto.eht_cap_info.eht_maccap_epcspriaccess_support) {
        epcs_err("EPCS not enabled on vap");
        return -1;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        epcs_err("vdev is NULL");
        return -1;
    }

    if (!wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        epcs_err("Vdev is not MLO");
        return -1;
    }

    if (epcs_config->authorize) {
        if (wlan_mlo_update_authorize_epcs_mac_addr(vdev, epcs_config->peer_mld_mac) != QDF_STATUS_SUCCESS) {
            epcs_err(" unable to authorize peer mld mac addr in database");
            return -1;
        }
        return retval;
    }

    if (epcs_config->deauthorize) {
        if (wlan_mlo_update_deauthorize_epcs_mac_addr(vdev, epcs_config->peer_mld_mac) != QDF_STATUS_SUCCESS) {
            epcs_err(" unable to deauthorize peer mld mac addr in database");
            return -1;
        }
        return retval;
    }

    if (epcs_config->set_edca) {
        wme = vap->iv_epcs_edcastate.wme_ChanParams.cap_wmeParams;
        aci = epcs_config->edca_param.aci;
        wme[aci].wmep_aifsn = epcs_config->edca_param.wmep_aifsn;
        wme[aci].wmep_logcwmin = epcs_config->edca_param.wmep_logcwmin;
        wme[aci].wmep_logcwmax = epcs_config->edca_param.wmep_logcwmax;
        wme[aci].wmep_txopLimit = epcs_config->edca_param.wmep_txopLimit;
        return retval;
    }

    if (epcs_config->get_edca) {
        wme = vap->iv_epcs_edcastate.wme_ChanParams.cap_wmeParams;
        epcs_info("edca param are:");
        for (iter = 0; iter < WME_NUM_AC; iter++) {
            if (iter == 0) {
                aci_val = "BK";
            } else if (iter == 1) {
                aci_val = "BE";
            } else if (iter == 2) {
                aci_val = "VI";
            } else {
                aci_val = "VO";
            }

            epcs_info("aci: %s, aifsn: %d, cwmin: %d, cwmax: %d, txopLimit: %d\n", aci_val, wme[iter].wmep_aifsn, wme[iter].wmep_logcwmin, wme[iter].wmep_logcwmax, wme[iter].wmep_txopLimit);
        }
        return retval;
    }

    if (epcs_config->set_muedca) {
        muedca = vap->iv_epcs_muedcastate.muedca_paramList;
        aci = epcs_config->muedca_param.aci;
        muedca[aci].muedca_aifsn = epcs_config->muedca_param.muedca_aifsn;
        muedca[aci].muedca_ecwmin = epcs_config->muedca_param.muedca_ecwmin;
        muedca[aci].muedca_ecwmax = epcs_config->muedca_param.muedca_ecwmax;
        muedca[aci].muedca_timer = epcs_config->muedca_param.muedca_timer;
        return retval;
    }

    if (epcs_config->get_muedca) {
        muedca = vap->iv_epcs_muedcastate.muedca_paramList;
        epcs_info("muedca param are:\n");
        for (iter = 0; iter < WME_NUM_AC; iter++) {
            if (iter == 0) {
                aci_val = "BK";
            } else if (iter == 1) {
                aci_val = "BE";
            } else if (iter == 2) {
                aci_val = "VI";
            } else {
                aci_val = "VO";
            }

            epcs_info("aci: %s, aifsn: %d, cwmin: %d, cwmax: %d, timer: %d\n", aci_val, muedca[iter].muedca_aifsn, muedca[iter].muedca_ecwmin, muedca[iter].muedca_ecwmax, muedca[iter].muedca_timer);
        }
        return retval;
    }

    qdf_mem_copy(mac.bytes, epcs_config->peer_mld_mac, QDF_MAC_ADDR_SIZE);
    if (vdev->mlo_dev_ctx) {
        mlo_peer_ctx = wlan_mlo_get_mlpeer(vdev->mlo_dev_ctx, &mac);
    }

    if (!mlo_peer_ctx) {
        epcs_err("mlo_peer_ctx is NULL");
        return -1;
    }

    /* Get the Assoc link to send EPCS action frame */
    ni = ieee80211_ucfg_get_mlo_assoc_link_node(mlo_peer_ctx);
    if (!ni) {
        return -1;
    }

    if (!ieee80211_node_is_authorized(ni)) {
        epcs_err("Unauthorized node:%p, try later", ni);
        retval = -1;
        goto exit;
    }

    /* check if node is authorised for EPCS */

    epcs_info.cat = epcs_config->enable ? WLAN_EPCS_CATEGORY_REQUEST : WLAN_EPCS_CATEGORY_TEARDOWN;
    retval = wlan_mlo_peer_rcv_cmd(mlo_peer_ctx, &epcs_info, &update_params);
    if (retval != QDF_STATUS_SUCCESS) {
        epcs_err("state change failed !!");
        retval = -1;
        goto exit;
    }

    retval = wlan_send_epcs_action_frame(ni, &epcs_info);
    if (retval)
        epcs_err("Failed to send EPCS action frame config:%d",
                  epcs_config->enable);

    if (update_params) {
        epcs_err("update params..");
    }

exit:
    if (ni->peer_obj) {
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
    }

    return retval;
}

int ieee80211_ucfg_tid_to_link_mapping(struct ieee80211com *ic, wlan_if_t vap,
                                       struct ieee80211_wlanconfig *config)
{
    struct ieee80211_wlanconfig_t2lm *t2lm_config = &config->data.t2lm;
    struct wlan_t2lm_onging_negotiation_info *t2lm_negotiation;
    struct wlan_objmgr_vdev *vdev;
    uint8_t t2lm_enable = 0;
    struct ieee80211_node *ni = NULL;
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct qdf_mac_addr mac;
    int retval = 0;
    uint8_t dir = 0;
    struct wlan_objmgr_peer *peer = NULL;
    QDF_STATUS status;

    if (ieee80211_get_t2lm_enable(ic, &t2lm_enable) != QDF_STATUS_SUCCESS) {
        t2lm_err("Failed to get t2lm_enable value");
        return -1;
    }

    if (t2lm_enable != WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) {
        t2lm_err("T2LM is not enabled");
        return -1;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        t2lm_err("vdev is NULL");
        return -1;
    }

    if (vdev->mlo_dev_ctx && (vdev->mlo_dev_ctx->wlan_vdev_count == 1)) {
        t2lm_err("Skip T2LM on single link MLO");
        return 0;
    }

    t2lm_negotiation = &vap->t2lm_negotiation;
    qdf_mem_zero(t2lm_negotiation, sizeof(*t2lm_negotiation));

    /* Initialize the direction field to invalid. The valid value will be set
     * when parsing the configured T2LM params.
     */
    for (dir = 0; dir < T2LM_DIRECTION_MAX; dir++) {
        t2lm_negotiation->t2lm_info[dir].direction =
            WLAN_T2LM_INVALID_DIRECTION;
    }

    qdf_mem_copy(vap->t2lm_peer_mld_mac, t2lm_config->peer_mld_mac,
            QDF_MAC_ADDR_SIZE);

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_T2LM_REQUEST:
            t2lm_negotiation->category = WLAN_T2LM_CATEGORY_REQUEST;

            ieee80211_parse_t2lm_config(t2lm_negotiation, t2lm_config);
            break;
        case IEEE80211_WLANCONFIG_T2LM_RESPONSE:
            t2lm_negotiation->category = WLAN_T2LM_CATEGORY_RESPONSE;
            t2lm_negotiation->t2lm_resp_type = t2lm_config->status;

            if (t2lm_config->status != T2LM_STATUS_PREFERRED_TID_TO_LINK_MAPPING)
                break;

            ieee80211_parse_t2lm_config(t2lm_negotiation, t2lm_config);
            break;
        case IEEE80211_WLANCONFIG_T2LM_TEARDOWN:
            /* Initialize the default mapping */
            t2lm_negotiation->t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction =
                WLAN_T2LM_BIDI_DIRECTION;
            t2lm_negotiation->t2lm_info[WLAN_T2LM_BIDI_DIRECTION].default_link_mapping =
                true;
            t2lm_negotiation->category = WLAN_T2LM_CATEGORY_TEARDOWN;
            break;
        default:
            t2lm_err("Invalid command");
            return -1;
    }

    /* As of now, AP/non-AP MLD supports mapping of all the TIDs to same link
     * set (WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) only. Hence, drop the
     * configured TID-to-link mapping if it is not homogeneous.
     */
    if (!ieee80211_ucfg_is_homogeneous_t2lm(&vap->t2lm_negotiation)) {
        t2lm_err("Configured T2LM is not homogenous");
        return -1;
    }

    vap->t2lm_test_cmd = true;

    if (!t2lm_config->assoc_frm) {

        if (!wlan_vdev_mlme_is_mlo_vdev(vdev)) {
            t2lm_err("Vdev is not MLO");
            vap->t2lm_test_cmd = false;
            return -1;
        }

        if ((t2lm_negotiation->category == WLAN_T2LM_CATEGORY_RESPONSE) &&
             !t2lm_config->unsolicited_t2lm_resp) {
            t2lm_info("Wait for T2LM request frame to send the T2LM response frame");
            return 0;
        }

        if ((t2lm_negotiation->category != WLAN_T2LM_CATEGORY_TEARDOWN) &&
            !ieee80211_ucfg_is_valid_t2lm_link_map(vap)) {
            t2lm_err("Invalid t2lm link map");
            vap->t2lm_test_cmd = false;
            return -1;
        }

        qdf_mem_copy(mac.bytes, vap->t2lm_peer_mld_mac, QDF_MAC_ADDR_SIZE);
        if (vdev->mlo_dev_ctx) {
            mlo_peer_ctx = wlan_mlo_get_mlpeer(vdev->mlo_dev_ctx, &mac);
        }

        if (!mlo_peer_ctx) {
            t2lm_err("mlo_peer_ctx is NULL");
            vap->t2lm_test_cmd = false;
            return -1;
        }

        peer = wlan_mlo_peer_get_first_active_peer(mlo_peer_ctx);
        if (!peer) {
            t2lm_err("peer is null");
            return -1;
        }

        status = wlan_objmgr_peer_try_get_ref(peer, WLAN_MLME_NB_ID);
        if (QDF_IS_STATUS_ERROR(status)) {
            t2lm_err("failed to get peer refcount");
            return -1;
        }

        ni = wlan_peer_get_mlme_ext_obj(peer);
        if (!ni) {
            vap->t2lm_test_cmd = false;
            t2lm_err("ni is null");
            wlan_objmgr_peer_release_ref(peer, WLAN_MLME_NB_ID);
            return -1;
        }

        if (!ieee80211_node_is_authorized(ni)) {
            t2lm_err("Unauthorized node:%p, try later", ni);
            if (ni->peer_obj)
                wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
            return -1;
        }

        retval = wlan_send_t2lm_action_frame(ni, &vap->t2lm_negotiation);
        if (retval)
            t2lm_err("Failed to send T2LM action frame, category:%d",
                    t2lm_negotiation->category);

        qdf_mem_zero(t2lm_negotiation, sizeof(*t2lm_negotiation));
        vap->t2lm_test_cmd = false;

        if (ni)
            ieee80211_free_node(ni, WLAN_MLME_NB_ID);
    }

    return retval;
}

/**
 * ieee80211_validate_t2lm_config() - API to validate the input T2LM config
 * @t2lm: Input T2LM config
 *
 * Return: success or failure
 */
int ieee80211_validate_t2lm_config(struct wlan_t2lm_info *t2lm)
{
    /* Do not send the T2LM IE to disable all the links. At least one link
     * should be available after T2LM mapping is established.
     * Only homogeneous T2LM mapping is supported now. Hence, checking any one
     * index of ieee link map tid array is sufficient.
     */
    if (!t2lm->ieee_link_map_tid[0]) {
        t2lm_err("ieee_link_map value cannot be zero");
        return -1;
    }

    if (!t2lm->expected_duration) {
        t2lm_err("Expected duration cannot be null");
        return -1;
    } else {
        t2lm->expected_duration_present = true;
    }

    if (t2lm->mapping_switch_time)
        t2lm->mapping_switch_time_present = true;

    if (t2lm->mapping_switch_time > 0xFFFF) {
        t2lm_err("Mapping switch time cannot be greater than 0xFFFF");
        return -1;
    }

    if (t2lm->expected_duration > 0xFFFFFF) {
        t2lm_err("Expected duration cannot be greater than 0xFFFFFF");
        return -1;
    }

    return 0;
}

/**
 * ieee80211_parse_link_map_config() - API to parse the input T2LM config from
 * the user command
 * @t2lm_link_map: User configured T2LM values
 * @t2lm_conf: Pointer to save the parsed T2LM values
 *
 * Return: none
 */
void ieee80211_parse_link_map_config(
        struct ieee80211_t2lm_link_map_value *t2lm_link_map,
        struct wlan_t2lm_info *t2lm_conf)
{
    int i;

    /* The Direction field of an advertised TID-To-Link Mapping element in
     * beacon/probe response shall be set to 2.
     */
    t2lm_conf->direction = WLAN_T2LM_BIDI_DIRECTION;
    t2lm_conf->mapping_switch_time = t2lm_link_map->map_switch_time;
    t2lm_conf->expected_duration = t2lm_link_map->expected_dur;
    t2lm_conf->link_mapping_size = t2lm_link_map->link_mapping_size;

    for (i = 0; i < T2LM_MAX_NUM_TIDS; i++) {
        t2lm_conf->ieee_link_map_tid[i] = t2lm_link_map->link_map_value;
    }

    t2lm_debug("dir:%d map_switch_time:%d exp_dur:%d link_map_value:%x link_map_size:%d",
            t2lm_conf->direction, t2lm_conf->mapping_switch_time,
            t2lm_conf->expected_duration, t2lm_link_map->link_map_value,
            t2lm_conf->link_mapping_size);
}

int ieee80211_send_non_tx_vap_t2lm_link_map_config(struct ieee80211vap *vap)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_t2lm_context *t2lm_ctx = NULL;
    int retval = 0;
    struct wlan_mlo_dev_context *mlo_dev_ctx;

    vdev = vap->vdev_obj;
    if (!vdev) {
        t2lm_err("null vdev");
        return -1;
    }

    mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vdev);
    if (!mlo_dev_ctx)
        return -1;

    t2lm_ctx = &mlo_dev_ctx->t2lm_ctx;
    t2lm_debug("vdev_id:%d t2lm_ctx:%p", wlan_vdev_get_id(vdev), t2lm_ctx);

    if (!vap->iv_ic->ic_t2lm_link_map_config_send) {
        t2lm_err("null ic_t2lm_link_map_config_send");
        return -1;
    }

    /* For non-tx vap, do not send the default mapping to FW if there is a
     * ongoing mapping is present. Else, send the default mapping to FW to avoid
     * inheritance.
     */
    t2lm_dev_lock_acquire(t2lm_ctx);

    if (t2lm_ctx->upcoming_t2lm.t2lm.mapping_switch_time_present) {
        t2lm_dev_lock_release(t2lm_ctx);
        retval = vap->iv_ic->ic_t2lm_link_map_config_send(vap, false);
    } else {
        t2lm_dev_lock_release(t2lm_ctx);
        retval = vap->iv_ic->ic_t2lm_link_map_config_send(vap, true);
    }


    if (retval)
        t2lm_err("Failed to send t2lm wmi command to FW, vap:%s:%d",
                vap->iv_netdev_name, vap->iv_unit);

    return retval;
}

static int ieee80211_iter_send_non_tx_vap_t2lm_link_map_config(
        struct ieee80211vap *vap)
{
    struct mbss_iter_func_args iter_func_args = {0};
    int retv = 0;

    iter_func_args.purpose = MBSS_ITER_FUNC_PURPOSE_SEND_NON_TX_VAP_T2LM_CMD;
    iter_func_args.retval = QDF_STATUS_SUCCESS;
    iter_func_args.args = NULL;
    iter_func_args.is_default_tx_vap = 0;

    wlan_mbss_group_iterate_vdev_list(vap->iv_ic,
            vap->iv_mbss.group_id, &iter_func_args);

    if (iter_func_args.retval != QDF_STATUS_SUCCESS)
        retv = -EINVAL;

    return retv;
}

int ieee80211_send_t2lm_link_map_config(struct ieee80211vap *vap,
                                        bool send_default_map)
{
    struct wlan_t2lm_context *t2lm_ctx;
    int retval = 0;
    struct wlan_mlo_dev_context *mlo_dev_ctx;

    if (!vap) {
        t2lm_err("null vap");
        return -1;
    }

    mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vap->vdev_obj);
    if (!mlo_dev_ctx)
        return -1;

    t2lm_ctx = &mlo_dev_ctx->t2lm_ctx;
    if(!t2lm_ctx) {
        t2lm_err("null T2LM ctx");
        return -1;
    }

    t2lm_debug("vap:%p is_6ghz:%d is_tx_vap:%d",
            vap, IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan),
            !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap));

    /* Non-Tx vaps which are part of 6GHz Tx vap inherits the T2LM IE from Tx
     * vap. The newly configured mapping may not be applicable for non-tx vaps
     * as they are part of some other MLD. Hence, send the T2LM WMI comamnd to
     * non-tx vap first with the already established, ongoing T2LM of the
     * non-tx vaps and then send the newly configured T2LM for Tx vap.
     */
    if (IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan) &&
        !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
        retval = ieee80211_iter_send_non_tx_vap_t2lm_link_map_config(vap);
        if (retval) {
            t2lm_err("Failed to send t2lm link map config");
            return retval;
        }
    }

    /* Send T2LM WMI command for the configured vap */
    if (vap->iv_ic->ic_t2lm_link_map_config_send)
        return vap->iv_ic->ic_t2lm_link_map_config_send(vap, send_default_map);

    return 0;
}

/**
 * ieee80211_vap_send_t2lm_link_map_config() - API to send the configured T2LM
 * values to FW.
 * @vap: Pointer to vap structure
 * @t2lm_conf: Pointer to T2LM configuration
 *
 * Return: success or failure
 */
int ieee80211_vap_send_t2lm_link_map_config(struct ieee80211vap *vap,
                                            struct wlan_mlo_t2lm_ie *t2lm_conf)
{
    struct wlan_t2lm_context *t2lm_ctx = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t vdev_count = 0;
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    uint32_t dtim_period = 0, min_dtim = 0;
    uint16_t ieee_link_id_mask = 0;
    int retval = 0;
    int i = 0, j = 0;
    struct ieee80211vap *co_mld_vap = NULL;
    uint16_t link_map_value = 0;
    struct wlan_mlo_t2lm_ie *upcoming_t2lm= NULL;
    struct wlan_mlo_dev_context *mlo_dev_ctx;

    if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
        t2lm_err("link map config is issued on a non AP vap");
        return -1;
    }

    if (!wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
        t2lm_err("vap:%s is not MLO", vap->iv_netdev_name);
        return -1;
    }

    if (vap->iv_bss->ml_info.num_partner_links == 1) {
        t2lm_err("Do not honor the link map config on a single link MLD");
        return -1;
    }

    mlo_get_ml_vdev_list(vap->vdev_obj, &vdev_count, wlan_vdev_list);

    if (!vdev_count) {
        t2lm_err("Number of VDEVs under MLD is reported as 0");
        return -1;
    }

    /* Get min DTIM of the MLD vaps, valid ieee80211 link map and 6GHz vap */
    for (i = 0; i < vdev_count; i++) {
        co_mld_vdev = wlan_vdev_list[i];
        if (!co_mld_vdev) {
            t2lm_err("co_mld_vdev is null");
            continue;
        }

        co_mld_vap = wlan_vdev_get_mlme_ext_obj(wlan_vdev_list[i]);
        if (!co_mld_vap) {
            t2lm_err("co_mld_vap is null");
            continue;
        }

        ucfg_wlan_vdev_mgr_get_param(co_mld_vdev,
                WLAN_MLME_CFG_DTIM_PERIOD, &dtim_period);

        if (!min_dtim || (min_dtim > dtim_period))
            min_dtim = dtim_period;

        ieee_link_id_mask |= BIT(co_mld_vdev->vdev_mlme.mlo_link_id);

        if (IEEE80211_IS_CHAN_6GHZ(co_mld_vap->iv_ic->ic_curchan))
            vap = co_mld_vap;
    }

    t2lm_debug("min_dtim:%d ieee_link_id_mask:%x vap:%s:%p",
               min_dtim, ieee_link_id_mask, vap->iv_netdev_name, vap);

    if (!min_dtim) {
        t2lm_err("min_dtim cannot be 0");
        retval = -1;
        goto exit;
    }

    /* Mapping switch time and expected duration should be multiple of min DITM.
     */
    if (t2lm_conf->t2lm.mapping_switch_time &&
        (t2lm_conf->t2lm.mapping_switch_time % min_dtim)) {
        t2lm_err("Mapping switch time (%d), expected duration (%d) should be multiple of DTIM (%d)",
                 t2lm_conf->t2lm.mapping_switch_time,
                 t2lm_conf->t2lm.expected_duration, min_dtim);
        retval = -1;
        goto exit;
    }

    t2lm_debug("Mapping switch time:%d, expected duration:%d",
            t2lm_conf->t2lm.mapping_switch_time,
            t2lm_conf->t2lm.expected_duration);

    /* Populate the hw_link_id map and disabled link bitmap if configured
     * link_map_value is valid.
     */
    for (i = 0; i < T2LM_MAX_NUM_TIDS; i++) {
        link_map_value = t2lm_conf->t2lm.ieee_link_map_tid[i];

        if ((ieee_link_id_mask & link_map_value) != link_map_value) {
            t2lm_err("Invalid link mask");
            retval = -1;
            goto exit;
        }

        t2lm_conf->t2lm.hw_link_map_tid[i] = ieee80211_get_hw_link_id_mask(
                wlan_vdev_list, vdev_count, link_map_value);

        /* Get the disabled link bitmap to update RNR IE */
        t2lm_conf->disabled_link_bitmap |= ~link_map_value;
    }

    /* Fill the default mapping flag */
    for (j = 0; j < T2LM_MAX_NUM_TIDS; j++) {
        if (t2lm_conf->t2lm.ieee_link_map_tid[j] == ieee_link_id_mask) {
            t2lm_conf->t2lm.default_link_mapping = true;
        } else {
            t2lm_conf->t2lm.default_link_mapping = false;
            break;
        }
    }

    /* The number of links in the disabled link bitmap should not be greater
     * than the configured number of links
     */
    t2lm_conf->disabled_link_bitmap &= ieee_link_id_mask;
    t2lm_debug("disabled_link_bitmap:%x", t2lm_conf->disabled_link_bitmap);

    mlo_dev_ctx = wlan_vdev_get_mlo_dev_ctx(vap->vdev_obj);
    if (!mlo_dev_ctx) {
        retval = -1;
        goto exit;
    }

    /* Copy the new T2LM config to T2LM context and send WMI command to FW */
    t2lm_ctx = &mlo_dev_ctx->t2lm_ctx;

    t2lm_dev_lock_acquire(t2lm_ctx);
    upcoming_t2lm = &t2lm_ctx->upcoming_t2lm;

    /* If AP is advertising the new T2LM (mapping_switch_time_present true),
     * then modify the T2LM IE and send the updated T2LM IE to FW. Else,
     * Populate the Ongoing T2LM IE in the T2LM context and send the T2LM IE to
     * FW.
     */
    if (upcoming_t2lm->t2lm.mapping_switch_time_present) {
        upcoming_t2lm->disabled_link_bitmap &= t2lm_conf->disabled_link_bitmap;
        upcoming_t2lm->t2lm.default_link_mapping = t2lm_conf->t2lm.default_link_mapping;
        upcoming_t2lm->t2lm.link_mapping_size = t2lm_conf->t2lm.link_mapping_size;

        /* Update mapping switch time only if it is provided in the new config
         * else use the mapping switch time from the ongoing T2LM.
         */
        if (t2lm_conf->t2lm.mapping_switch_time_present) {
            upcoming_t2lm->t2lm.mapping_switch_time_present = 1;
            upcoming_t2lm->t2lm.mapping_switch_time =
                t2lm_conf->t2lm.mapping_switch_time;
        }

        /* Update expected duration only if it is provided in the new config
         * else use the expected duration from ongoing T2LM.
         */
        if (t2lm_conf->t2lm.expected_duration_present) {
            upcoming_t2lm->t2lm.expected_duration_present = 1;
            upcoming_t2lm->t2lm.expected_duration =
                t2lm_conf->t2lm.expected_duration;
        }

        if (upcoming_t2lm->t2lm.default_link_mapping) {
            /* Clear the ieee_link_map_tid and hw_link_map_tid if the new
             * mapping is default mapping.
             */
            qdf_mem_zero(upcoming_t2lm->t2lm.ieee_link_map_tid,
                    sizeof(uint16_t) * T2LM_MAX_NUM_TIDS);
            qdf_mem_zero(upcoming_t2lm->t2lm.hw_link_map_tid,
                    sizeof(uint16_t) * T2LM_MAX_NUM_TIDS);
        } else {
            /* Update the ieee_link_map_tid and hw_link_map_tid with the mapping
             * from ongoing mapping and new config.
             */
            for (i = 0; i < T2LM_MAX_NUM_TIDS; i++) {
                upcoming_t2lm->t2lm.ieee_link_map_tid[i] |=
                    t2lm_conf->t2lm.ieee_link_map_tid[i];
                upcoming_t2lm->t2lm.hw_link_map_tid[i] |=
                     t2lm_conf->t2lm.hw_link_map_tid[i];
            }
        }
    } else {
        qdf_mem_copy(upcoming_t2lm, t2lm_conf, sizeof(struct wlan_mlo_t2lm_ie));
    }

    /* Update the expected duration of established T2LM IE */
    if (t2lm_ctx->established_t2lm.t2lm.expected_duration_present) {
        if (upcoming_t2lm->t2lm.mapping_switch_time <
            t2lm_ctx->established_t2lm.t2lm.expected_duration) {
            t2lm_ctx->established_t2lm.t2lm.expected_duration =
                upcoming_t2lm->t2lm.mapping_switch_time;
        } else {
            t2lm_err("Mapping switch time(%d) of the new mapping can not be greater than the expected duration(%d) of the old mapping",
                    upcoming_t2lm->t2lm.mapping_switch_time,
                    t2lm_ctx->established_t2lm.t2lm.expected_duration);
            qdf_mem_zero(upcoming_t2lm, sizeof(struct wlan_mlo_t2lm_ie));
            t2lm_dev_lock_release(t2lm_ctx);
            retval = -1;
            goto exit;
        }
    }

    if (upcoming_t2lm->disabled_link_bitmap == ieee_link_id_mask) {
        t2lm_err("Do not send the T2LM WMI command to disable all the links");
        qdf_mem_zero(upcoming_t2lm, sizeof(struct wlan_mlo_t2lm_ie));
        t2lm_dev_lock_release(t2lm_ctx);
        retval = -1;
        goto exit;
    }

    t2lm_debug("Established mapping: disabled_link_map:%x default_map:%d MSTP:%d EDP:%d MST:%d ED:%d LMS:%d",
            t2lm_ctx->established_t2lm.disabled_link_bitmap,
            t2lm_ctx->established_t2lm.t2lm.default_link_mapping,
            t2lm_ctx->established_t2lm.t2lm.mapping_switch_time_present,
            t2lm_ctx->established_t2lm.t2lm.expected_duration_present,
            t2lm_ctx->established_t2lm.t2lm.mapping_switch_time,
            t2lm_ctx->established_t2lm.t2lm.expected_duration,
            t2lm_ctx->established_t2lm.t2lm.link_mapping_size);

    for (i = 0; i < T2LM_MAX_NUM_TIDS; i++) {
        t2lm_rl_debug("tid:%d ieee_link_map_tid:%x hw_link_map_tid:%x",
                i, t2lm_ctx->established_t2lm.t2lm.ieee_link_map_tid[i],
                t2lm_ctx->established_t2lm.t2lm.hw_link_map_tid[i]);
    }

    t2lm_debug("Ongoing mapping: disabled_link_map:%x default_map:%d MSTP:%d EDP:%d MST:%d ED:%d LMS:%d",
            upcoming_t2lm->disabled_link_bitmap,
            upcoming_t2lm->t2lm.default_link_mapping,
            upcoming_t2lm->t2lm.mapping_switch_time_present,
            upcoming_t2lm->t2lm.expected_duration_present,
            upcoming_t2lm->t2lm.mapping_switch_time,
            upcoming_t2lm->t2lm.expected_duration,
            upcoming_t2lm->t2lm.link_mapping_size);

    for (i = 0; i < T2LM_MAX_NUM_TIDS; i++) {
        t2lm_rl_debug("tid:%d ieee_link_map_tid:%x hw_link_map_tid:%x", i,
                upcoming_t2lm->t2lm.ieee_link_map_tid[i],
                upcoming_t2lm->t2lm.hw_link_map_tid[i]);
    }

    t2lm_dev_lock_release(t2lm_ctx);
    retval = ieee80211_send_t2lm_link_map_config(vap, false);

exit:
    if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0) !=
            QDF_STATUS_SUCCESS) {
        t2lm_err("failed to release mlo vdev ref");
        return -1;
    }

    return retval;
}

/**
 * ieee80211_vap_config_t2lm_map() - Wrapper API to send configured T2LM values
 * to FW for the given vap.
 * @pdev: Pointer to pdev object
 * @object: Pointer to vdev object
 * @arg: Pointer to configured T2LM values
 *
 * Return: none
 */
void ieee80211_vap_config_t2lm_map(struct wlan_objmgr_pdev *pdev,
        void *object, void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    struct wlan_mlo_t2lm_ie *t2lm_conf = (struct wlan_mlo_t2lm_ie *)arg;
    struct ieee80211vap *vap = NULL;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        t2lm_err("null vap");
        return;
    }

    if (vdev->mlo_dev_ctx && (vdev->mlo_dev_ctx->wlan_vdev_count == 1)) {
        t2lm_err("Skip T2LM advertisement on single link MLO");
        return;
    }

    ieee80211_vap_send_t2lm_link_map_config(vap, t2lm_conf);
}

/**
 * ieee80211_configure_t2lm_link_map() - Wrapper API to send the configured T2lM
 * values to FW.
 * @ic: Pointer to ic structure
 * @vap: Pointer to vap structure
 * @t2lm_conf: Configured T2LM values
 *
 * If the T2LM values are configured per vap, then send these value to FW only
 * for that vap. Else, send the configured T2LM values to FW for all the vaps.
 *
 * Return: success to failure
 */
int ieee80211_configure_t2lm_link_map(
        struct ieee80211com *ic, struct ieee80211vap *vap,
        struct wlan_mlo_t2lm_ie *t2lm_conf)
{
    int retval = 0;

    retval = ieee80211_validate_t2lm_config(&t2lm_conf->t2lm);
    if (retval)
        return retval;

    if (vap) {
        return ieee80211_vap_send_t2lm_link_map_config(vap, t2lm_conf);
    } else {
        wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                ieee80211_vap_config_t2lm_map,
                t2lm_conf, 0, WLAN_MLME_NB_ID);
    }

    return retval;
}

int ieee80211_ucfg_link_map_config(struct ieee80211com *ic, wlan_if_t vap,
                                   struct ieee80211_wlanconfig *config)
{
    uint8_t t2lm_enable = 0;
    struct wlan_mlo_t2lm_ie t2lm_conf = {0};
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t vdev_count = 0;
    uint16_t removal_links = 0;
    int i = 0;

    if (vap) {
        vdev = vap->vdev_obj;
        if (!vdev) {
            t2lm_err("vdev is NULL");
            return -1;
        }

        if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
            t2lm_err("Vdev is not MLO");
            return -1;
        }

        if (vdev->mlo_dev_ctx && (vdev->mlo_dev_ctx->wlan_vdev_count == 1)) {
            t2lm_err("Skip T2LM advertisement on single link MLO");
            return 0;
        }

        mlo_get_ml_vdev_list(vap->vdev_obj, &vdev_count, wlan_vdev_list);
        if (!vdev_count) {
                t2lm_err("Number of VDEVs under MLD is reported as 0");
                return false;
        }

        for (i = 0; i < vdev_count; i++) {
                vdev = wlan_vdev_list[i];
                if (!vdev) {
                        t2lm_err("vdev is null");
                        continue;
                }

                pdev = wlan_vdev_get_pdev(vdev);
                if (!pdev) {
                        t2lm_err("pdev is null");
                        continue;
                }

                if ((wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS) ||
                    (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE)))) {
                        /* VAP is in progress for link removal */
                        removal_links |= BIT(ieee80211_mlo_get_pdev_hw_link_id(pdev));
                }
        }

        if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0) !=
                QDF_STATUS_SUCCESS) {
            t2lm_err("failed to release mlo vdev ref");
            return -1;
        }
    }

    if (ieee80211_get_t2lm_enable(ic, &t2lm_enable) != QDF_STATUS_SUCCESS) {
        t2lm_err("Failed to get t2lm_enable value");
        return -1;
    }

    if (t2lm_enable != WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) {
        t2lm_err("T2LM is not enabled");
        return -1;
    }
    /* Compare and check if trying to provision to link under removal process */
      if (removal_links & config->data.t2lm_link_map_config.link_map_value) {
              t2lm_err("Error!! Trying to map to link under removal process, removal_links:%x provisioned_links:%x",
                       removal_links, config->data.t2lm_link_map_config.link_map_value);
              return -1;
      }

    switch (config->cmdtype) {
        case IEEE80211_WLANCONFIG_T2LM_LINK_MAP_CONFIG:
            ieee80211_parse_link_map_config(&config->data.t2lm_link_map_config,
                                            &t2lm_conf.t2lm);
            break;
        default:
            t2lm_err("Invalid command");
            return -1;
    }

    return ieee80211_configure_t2lm_link_map(ic, vap, &t2lm_conf);
}
#endif /* WLAN_FEATURE_11BE */

int ieee80211_disabled_sched_mode(struct ieee80211com *ic, wlan_if_t vap,
                                  uint32_t value)
{
   if (ic->ic_vap_set_param) {
        return ic->ic_vap_set_param(vap, IEEE80211_DISABLED_SCHED_MODES,
                                    value);
   }

   return 0;
}

void ieee80211_parse_tdma_config(
        struct ieee80211com *ic,
        struct ieee80211_wlanconfig_tdma_schedule *tdma_config)
{
    int id = tdma_config->sched_id;
    struct wlan_objmgr_pdev *pdev;

    if ((id < 0) && (id >= IEEE80211_MAX_TDMA_SCHEDULES)) {
        qdf_err("Schedule id is not in range\n");
        return;
    }
    pdev = ic->ic_pdev_obj;
    if (!pdev)
        return;
    ic->tdma_info[id].pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
    ic->tdma_info[id].sched_type = tdma_config->sched_type;
    ic->tdma_info[id].sched_id = tdma_config->sched_id;
    qdf_mem_copy(ic->tdma_info[id].bssid, tdma_config->bssid, QDF_MAC_ADDR_SIZE);
    ic->tdma_info[id].start_time_tsf_low = tdma_config->start_time_tsf_low;
    ic->tdma_info[id].start_time_tsf_high = tdma_config->start_time_tsf_high;
    ic->tdma_info[id].num_busy_slots = tdma_config->num_busy_slots;
    ic->tdma_info[id].busy_slot_dur_ms = tdma_config->busy_slot_dur_ms;
    ic->tdma_info[id].busy_slot_intvl_ms = tdma_config->busy_slot_intvl_ms;
    ic->tdma_info[id].edca_params_valid = (bool)tdma_config->edca_params_valid;
    qdf_mem_copy(ic->tdma_info[id].aifsn, tdma_config->aifsn,
                 WLAN_MAX_AC * sizeof(uint16_t));
    qdf_mem_copy(ic->tdma_info[id].cwmin, tdma_config->cwmin,
                 WLAN_MAX_AC * sizeof(uint16_t));
    qdf_mem_copy(ic->tdma_info[id].cwmax, tdma_config->cwmax,
                 WLAN_MAX_AC * sizeof(uint16_t));
    if (ic->ic_send_tdma_sched != NULL) {
        ic->ic_send_tdma_sched(ic, &ic->tdma_info[id]);
    }

}

#ifdef WLAN_FEATURE_11BE_MLO
QDF_STATUS
ieee80211_ucfg_validate_mlo_link_removal_request(wlan_if_t vap,
                uint32_t tbtt_count)
{
     struct ieee80211com *ic = NULL;

     if (!vap) {
	mlo_err("VAP is NULL");
        return QDF_STATUS_E_INVAL;
     }

     ic = vap->iv_ic;
     if (!ic) {
	 mlo_err("IC is NULL");
         return QDF_STATUS_E_INVAL;
     }

    /* For Tx vap removal request validate MLO link removal request
     * for all non-Tx vaps in the MBSS group before proceeding.
     */
    if (ic->is_mbssid_enabled &&  !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
        if (wlan_do_iter_fun_mbss_group(ic, vap->iv_mbss.group_id,
                 MBSS_ITER_FUNC_PURPOSE_LINK_REMOVAL_VALIDATION, &tbtt_count)) {
            return QDF_STATUS_E_INVAL;
        }
    }
    if (wlan_mlo_validate_link_removal_cmd(vap, tbtt_count, false)) {
        mlo_err("Validation of MLO link removal command failed for Vap: %s",
                vap->iv_netdev_name);
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
ieee80211_ucfg_process_mlo_link_removal_cmd(wlan_if_t vap, uint32_t tbtt_count)
{
    QDF_STATUS status;
    struct ieee80211com *ic = NULL;

     if(!vap) {
	mlo_err("VAP is NULL");
        return QDF_STATUS_E_INVAL;
     }

     ic = vap->iv_ic;
     if (!ic) {
	 mlo_err("IC is NULL");
         return QDF_STATUS_E_INVAL;
     }

    if (ic->is_mbssid_enabled && !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
        /* If Tx-vap then iterate over each non-Tx vap and send
         * MLO link removal command to FW for each of these vaps.
         *
         * Current standard does not support Tx-vap role switch
         * when the same is brought down. This prompts to bring-
         * down of non-Tx vap as well along with the Tx-vap.
         */
        if (wlan_do_iter_fun_mbss_group(ic, vap->iv_mbss.group_id,
                        MBSS_ITER_FUNC_PURPOSE_LINK_REMOVE, &tbtt_count)) {
            return QDF_STATUS_E_INVAL;
        }
    }

    /* Send MLO link removal command to FW for cohosted VAP
     * or for Tx-vap in case of MBSSID
     */
    status = wlan_mlo_link_removal_cmd(vap, tbtt_count, false);

    return status;
}

#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
static void
ieee80211_ucfg_peer_ptqm_migrate_begin_cb(struct wlan_mlo_peer_context *ml_peer,
                                          void *user_data)
{
    qdf_info("%pM primary_umac_psoc_id %u", ml_peer->peer_mld_addr.bytes,
             ml_peer->primary_umac_psoc_id);
}

static void
ieee80211_ucfg_peer_ptqm_migrate_end_cb(
                struct wlan_mlo_peer_context *ml_peer,
                enum primary_link_peer_migration_evenr_status status,
                void *user_data)
{
    qdf_info("%pM status %u primary_umac_psoc_id %u",
             ml_peer->peer_mld_addr.bytes, status,
             ml_peer->primary_umac_psoc_id);
}

static void
ieee80211_ucfg_link_ptqm_migrate_begin_cb(struct wlan_objmgr_vdev *vdev,
                                          void *user_data)
{
    qdf_info("vdev_id %u", vdev->vdev_objmgr.vdev_id);
}

static void
ieee80211_ucfg_link_ptqm_migrate_end_cb(
                struct wlan_objmgr_vdev *vdev,
                QDF_STATUS status,
                void *user_data, struct ptqm_link_migration_rsp_params *params)
{
    qdf_info("vdev_id %u status %u total peer %u fail peer %u",
             vdev->vdev_objmgr.vdev_id, status,
             params->total_peer_count, params->fail_peer_count);
}

void wlan_ptqm_print_peer_context(struct wlan_mlo_peer_context *ml_peer);

#ifdef PTQM_LAYER_DEBUG
void wlan_ptqm_migration_op_set(uint8_t op);
#endif

QDF_STATUS
ieee80211_ucfg_set_peer_ptqm_migrate(struct ieee80211vap *vap,
                                     uint8_t *mac, uint32_t user_value)
{
    struct wlan_mlo_dev_context *ml_dev;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    struct wlan_mlo_peer_context *ml_peer;
    struct ptqm_peer_migrate_params params = {0};
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    uint32_t val = user_value;
    uint8_t dst_link_id;
    uint8_t src_link_id;
    uint8_t module_id;
    uint8_t op;

    if (!wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        qdf_err("Vdev is not MLO capable");
        return QDF_STATUS_E_INVAL;
    }

    ml_dev = vdev->mlo_dev_ctx;
    if (!ml_dev) {
        qdf_err("ML dev context is NULL");
        return QDF_STATUS_E_INVAL;
    }

    ml_peer = wlan_mlo_get_mlpeer(ml_dev, (struct qdf_mac_addr *)mac);
    if (!ml_peer) {
        qdf_err("ML peer not found");
        return QDF_STATUS_E_INVAL;
    }

    dst_link_id = val & 0xff;
    val >>= 8;
    src_link_id = val & 0xff;
    val >>= 8;
    module_id = val & 0xff;
    val >>= 8;
    op = val & 0xff;

    qdf_info("Op %u module_id %u src_link_id %d dst_link_id %d",
             op, module_id, src_link_id, dst_link_id);

    params.module_id = module_id;
    params.src_link_id = src_link_id;
    params.dst_link_id = dst_link_id;
    params.begin = ieee80211_ucfg_peer_ptqm_migrate_begin_cb;
    params.end = ieee80211_ucfg_peer_ptqm_migrate_end_cb;
    params.user_data = NULL;

    if (op == 0) {
        /* Add */
        status = wlan_ptqm_peer_migrate_req_add(vap->vdev_obj, ml_peer,
                                                &params);
    } else if (op == 1){
        /* Completion */
        uint8_t comp_status = user_value & 0xff;

        wlan_ptqm_peer_migrate_completion(ml_dev, ml_peer, comp_status);
    } else if (op == 2) {
        wlan_ptqm_print_peer_context(ml_peer);
    }
#ifdef PTQM_LAYER_DEBUG
    else if (op == 3) {
        uint8_t n = user_value & 0xff;

        wlan_ptqm_migration_op_set(n);
    }
#endif
    else {
        qdf_err("Invalid op %u", op);
    }

    return status;
}

QDF_STATUS
ieee80211_ucfg_set_link_ptqm_migrate(struct ieee80211vap *vap)
{
    struct ptqm_link_migrate_params params = {0};

    params.module_id = PTQM_MIGRATION_MODULE_TEST;
    params.src_link_id = HW_LINK_ID_ANY;
    params.begin = ieee80211_ucfg_link_ptqm_migrate_begin_cb;
    params.end = ieee80211_ucfg_link_ptqm_migrate_end_cb;
    params.user_data = NULL;
    params.link_disable = true;
    params.force_mig = true;

    return wlan_ptqm_link_migrate_req_add(vap->vdev_obj, &params);
}

QDF_STATUS
ieee80211_ucfg_mlreconfig_set_link_ptqm_layer_migrate(struct ieee80211vap *vap)
{
    struct ptqm_link_migrate_params params = {0};

    params.module_id = PTQM_MIGRATION_MODULE_MLR;
    params.src_link_id = HW_LINK_ID_ANY;
    params.begin = ieee80211_ucfg_link_ptqm_migrate_begin_cb;
    params.end = ieee80211_ucfg_link_ptqm_migrate_end_cb;
    params.user_data = NULL;
    params.link_disable = true;
    params.force_mig = true;

    return wlan_ptqm_link_migrate_req_add(vap->vdev_obj, &params);
}
#endif /* QCA_SUPPORT_PRIMARY_LINK_MIGRATE */
#endif /* WLAN_FEATURE_11BE_MLO */


#ifdef WLAN_SUPPORT_DPDK
int ieee80211_ucfg_cfgmgr_get_soc_info(struct dpdk_wlan_soc_info_cmd *req)
{
    struct dpdk_wlan_soc_info_event *rsp = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    uint8_t num_pdev = 0;
    uint8_t soc_id;
    int ret = 0;

    soc_id = req->soc_id;
    psoc = wlan_objmgr_get_psoc_by_id(soc_id, WLAN_MLME_NB_ID);
    if (!psoc) {
        qdf_err("Failed to get psoc for soc id %d", soc_id);
        return -EINVAL;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_txrx_handle) {
        qdf_err("Unable to get soc DP handle for soc id %d", soc_id);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    num_pdev = wlan_psoc_get_pdev_count(psoc);
    if (!num_pdev) {
        qdf_err("Num pdev is 0 for soc id %d", soc_id);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    rsp = qdf_mem_malloc(sizeof(*rsp) + (sizeof(struct dpdk_wlan_pdev_info) * num_pdev));
    if (!rsp) {
        qdf_err("Unable to allocate memory for resp %d", DPDK_WLAN_EVT_TYPE_SOC_INFO);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }
    ret = cdp_cfgmgr_get_soc_info(soc_txrx_handle, soc_id, rsp);
    if (!ret) {
        /* Send resp to cfgmgr */
        if (cfgmgr_wlan_send_msg((struct cfgmgr_cmn_msg *)rsp,
                                 sizeof(struct dpdk_wlan_soc_info_event), DPDK_WLAN_EVT_TYPE_SOC_INFO))
            qdf_err("Failed to send event %d", DPDK_WLAN_EVT_TYPE_SOC_INFO);
    } else {
        qdf_err("Failed to get soc info for soc id %d", soc_id);
    }
    wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
    qdf_mem_free(rsp);
    return ret;
}

int ieee80211_ucfg_cfgmgr_get_vdev_info(struct dpdk_wlan_vdev_info_cmd *req)
{
    struct dpdk_wlan_vdev_info_event *rsp = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    uint8_t soc_id;
    uint8_t num_vdev = 0;
    int ret = 0;

    soc_id = req->soc_id;
    psoc = wlan_objmgr_get_psoc_by_id(soc_id, WLAN_MLME_NB_ID);
    if (!psoc) {
        qdf_err("Failed to get psoc for soc id %d", soc_id);
        return -EINVAL;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_txrx_handle) {
        qdf_err("Unable to get soc DP handle for soc id %d", soc_id);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    num_vdev = psoc->soc_objmgr.wlan_vdev_count;
    if (!num_vdev) {
        qdf_err("Num pdev is 0 for soc id %d", soc_id);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    rsp = qdf_mem_malloc(sizeof(*rsp) + (sizeof(struct dpdk_wlan_vdev_info) * num_vdev));
    if (!rsp) {
        qdf_err("Unable to allocate memory for resp %d", DPDK_WLAN_EVT_TYPE_VDEV_INFO);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    ret = cdp_cfgmgr_get_vdev_info(soc_txrx_handle, soc_id, rsp);
    if (!ret) {
        /* Send resp to cfgmgr */
        if (cfgmgr_wlan_send_msg((struct cfgmgr_cmn_msg *)rsp,
                                 (sizeof(*rsp) + (sizeof(struct dpdk_wlan_vdev_info) * num_vdev)),
                                 DPDK_WLAN_EVT_TYPE_VDEV_INFO))
            qdf_err("Failed to send event %d", DPDK_WLAN_EVT_TYPE_VDEV_INFO);
    } else {
        qdf_err("Failed to get vdev info for soc id %d", soc_id);
    }
    wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
    qdf_mem_free(rsp);
    return ret;
}

int ieee80211_ucfg_cfgmgr_get_peer_info(struct dpdk_wlan_peer_info_cmd *req)
{
    struct dpdk_wlan_peer_info_event *rsp = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    uint8_t soc_id;
    uint8_t num_peer = 0;
    int ret = 0;
    struct dpdk_wlan_peer_info *peer_info;
    struct dpdk_wlan_peer_info *buf;
    uint16_t msg_len = 0;
    uint16_t max_entries_allowed = 0;
    uint16_t num_entries = 0;

    soc_id = req->soc_id;
    psoc = wlan_objmgr_get_psoc_by_id(soc_id, WLAN_MLME_NB_ID);
    if (!psoc) {
        qdf_err("Failed to get psoc for soc id %d", soc_id);
        return -EINVAL;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_txrx_handle) {
        qdf_err("Unable to get soc DP handle for soc id %d", soc_id);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    wlan_psoc_obj_lock(psoc);
    num_peer = wlan_psoc_get_peer_count(psoc);
    if (!num_peer) {
        qdf_err("Num peer is 0 for soc id %d", soc_id);
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    peer_info = (struct dpdk_wlan_peer_info *)qdf_mem_malloc(num_peer * sizeof(struct dpdk_wlan_peer_info *));
    if (!peer_info) {
        qdf_err("Failed to allocate buffer to peer info");
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    cdp_cfgmgr_get_peer_info(soc_txrx_handle, soc_id, peer_info);
    wlan_psoc_obj_unlock(psoc);
    max_entries_allowed = (GENLMSG_DEFAULT_SIZE - sizeof(struct dpdk_wlan_peer_info_event)) /
                           sizeof(struct dpdk_wlan_peer_info);

    if (max_entries_allowed > num_peer)
        max_entries_allowed = num_peer;

    buf = peer_info;
    while (num_peer) {
           if (num_peer < max_entries_allowed)
               num_entries = num_peer;
           else
               num_entries = max_entries_allowed;

           msg_len = sizeof(struct dpdk_wlan_peer_info_event) + (sizeof(struct dpdk_wlan_peer_info) * num_entries);
           rsp = qdf_mem_malloc(msg_len);

           if (!rsp) {
               qdf_err("Unable to allocate memory for resp %d", DPDK_WLAN_EVT_TYPE_PEER_INFO);
               ret = -EINVAL;
               goto exit;
           }

           /* Send resp to cfgmgr */
           qdf_mem_copy(rsp->peer_info, buf, (sizeof(struct dpdk_wlan_peer_info) * num_entries));
           buf += num_entries;
           num_peer -= num_entries;
           if (num_peer)
               rsp->more_msg = true;
           else
               rsp->more_msg = false;

           rsp->soc_id = soc_id;
           if (cfgmgr_wlan_send_msg((struct cfgmgr_cmn_msg *)rsp, msg_len, DPDK_WLAN_EVT_TYPE_PEER_INFO)) {
               qdf_err("Failed to send event %d", DPDK_WLAN_EVT_TYPE_PEER_INFO);
               qdf_mem_free(rsp);
               goto exit;
           }
           qdf_mem_free(rsp);
    }
exit:
    wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_NB_ID);
    qdf_mem_free(peer_info);
    return ret;
}
#endif

#ifdef WLAN_FEATURE_11BE_MLO
int ieee80211_get_link_id_bpcc_val(wlan_if_t vap, struct mlo_link_id_bpcc_count *get_bpcc_link_id)
{
        struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
        uint16_t vdev_count = 0, i;
        struct wlan_objmgr_vdev *co_mld_vdev = NULL;
        struct ieee80211vap *co_mld_vap = NULL;

        if (!vap)
            return -1;

        if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
            mlo_ap_get_active_vdev_list(vap->vdev_obj, &vdev_count, wlan_vdev_list);
            if (!vdev_count) {
                qdf_err("Number of VDEVs under AP-MLD is reported as 0");
                return -1;
            }
        } else {
            qdf_err("This is only applicable for MLO VAPs:%s",vap->iv_netdev_name);
            return -1;
        }

        qdf_nofl_info("|===========|==================|===========|");
        qdf_nofl_info("|    VAP    |      LINK_ID     |    BPCC   |");
        qdf_nofl_info("|===========|==================|===========|");

        for (i = 0; i < vdev_count; i++) {
             co_mld_vdev = wlan_vdev_list[i];
             if (co_mld_vdev == NULL) {
                 qdf_err("VDEV in MLD VDEV list is NULL");
                 goto err_rel_vdev_ref;
             }

             co_mld_vap = wlan_vdev_get_vap(co_mld_vdev);
             if (co_mld_vap == NULL || co_mld_vap->iv_ic == NULL) {
                 qdf_err("Co-MLD VAP is NULL");
                 goto err_rel_vdev_ref;
             }
             get_bpcc_link_id[i].bpcc_link_id[0] = wlan_vdev_get_link_id(co_mld_vdev);
             get_bpcc_link_id[i].bpcc_link_id[1] = iee80211_cu_get_bssparamchangecnt(co_mld_vap, NULL);
             qdf_nofl_info("|   %s      |         %d       |    %d     |",
                           co_mld_vap->iv_netdev_name, get_bpcc_link_id[i].bpcc_link_id[0],
                           get_bpcc_link_id[i].bpcc_link_id[1]);
             mlo_release_vdev_ref(wlan_vdev_list[i]);
        }
        qdf_nofl_info("|===========|==================|===========|");
        return vdev_count;

err_rel_vdev_ref:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
        QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count, 0)) !=
                       QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.*/
        qdf_err("Error while trying to release VDEV references investigate. This could lead to system issues.");
    }
    return -1;

}
#endif /* WLAN_FEATURE_11BE_MLO */
