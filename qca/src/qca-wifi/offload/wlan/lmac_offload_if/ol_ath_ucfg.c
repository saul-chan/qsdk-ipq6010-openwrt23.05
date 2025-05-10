/*
 * Copyright (c) 2016-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/utsname.h>
#include <linux/if_arp.h>       /* XXX for ARPHRD_ETHER */

#include <asm/uaccess.h>

#include <osif_private.h>
#include <wlan_opts.h>

#include "qdf_mem.h"
#include "qdf_types.h"
#include "ieee80211_var.h"
#include "ol_if_athvar.h"
#if OBSS_PD
#include "ol_if_obss.h"
#endif
#include "if_athioctl.h"
#include "init_deinit_lmac.h"
#include "fw_dbglog_api.h"
#include "ol_regdomain.h"
#if UNIFIED_SMARTANTENNA
#include <target_if_sa_api.h>
#endif /* UNIFIED_SMARTANTENNA */
#include <wlan_gpio_tgt_api.h>
#include "target_if.h"
#include "fw_dbglog_api.h"

#include <acfg_api_types.h>
#include "ol_txrx_stats.h"
#include <ol_if_stats.h>
#include "cdp_txrx_ctrl.h"
#if CONFIG_SAWF_DEF_QUEUES
#include "cdp_txrx_sawf.h"
#endif
#include "ol_ath_ucfg.h"
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_if.h>
#endif
#include <wlan_lmac_if_api.h>
#if WLAN_SPECTRAL_ENABLE
#include <wlan_spectral_ucfg_api.h>
#endif

#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif

#include <ieee80211_ucfg.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_peer.h>
#endif
#include "wlan_utility.h"
#include "cfg_ucfg_api.h"
#include <ieee80211_channel.h>
#include <wlan_ioctl_ftm.h>
#include <wlan_cfg80211_ftm.h>
#include "target_type.h"
#include <ieee80211_cfg80211.h>
#include "wlan_mbss.h"
#ifdef CONFIG_SAWF_DEF_QUEUES
#include "wlan_sawf.h"
#endif
#ifdef QCA_R2P_UPDATE_ENABLED
#include "qdf_file.h"
#include "ieee80211_regdmn.h"
#endif

#define FLAG_PARTIAL_OL 1
#define FLAG_LITHIUM 2
#define WBM_RELEASE_RING 5
#define VOW_STATS_ENABLE 1

extern void acfg_convert_to_acfgprofile (struct ieee80211_profile *profile,
                acfg_radio_vap_info_t *acfg_profile);
extern int wlan_get_vap_info(struct ieee80211vap *vap,
                struct ieee80211vap_profile *vap_profile,
                void *handle);
extern int ol_ath_target_start(ol_ath_soc_softc_t *soc);
extern int osif_ol_ll_vap_hardstart(struct sk_buff *skb, struct net_device *dev);
extern int osif_ol_vap_hardstart_wifi3(struct sk_buff *skb, struct net_device *dev);
extern int osif_ol_vap_send_exception_wifi3(struct sk_buff *skb, struct net_device *dev, void *mdata);

#ifdef QCA_SUPPORT_WDS_EXTENDED
extern int osif_wds_ext_peer_hardstart_wifi3(struct sk_buff *skb, struct net_device *dev);
extern void osif_deliver_wds_ext_data_ol(ol_osif_peer_handle osif, struct sk_buff *skb_list);
#if !QCA_NSS_PLATFORM
extern void osif_deliver_wds_ext_data_wifi2(ol_osif_peer_handle osif, struct sk_buff *skb_list);
extern int osif_wds_ext_peer_hardstart_wifi2(struct sk_buff *skb, struct net_device *dev);
#endif
#endif

#if ATH_SUPPORT_ICM

#define SPECTRAL_PHY_CCA_NOM_VAL_AR9888_2GHZ    (-108)
#define SPECTRAL_PHY_CCA_NOM_VAL_AR9888_5GHZ    (-105)

/**
 * ol_get_nominal_nf() - Get nominal noise floor
 * @ic: Pointer to struct ieee80211com
 *
 * XXX Since we do not have a way to extract nominal noisefloor value from
 * firmware, we are exporting the values from the Host layers. These values are
 * taken from the file ar6000_reset.h. Can be replaced later.
 *
 * Return: Nominal noise floor
 */

int ol_get_nominal_nf(struct ieee80211com *ic)
{
    u_int32_t channel_freq;
    enum band_info band;
    int16_t nominal_nf = 0;

    channel_freq = ic->ic_curchan->ic_freq;
    band = (channel_freq > 4000)? BAND_5G : BAND_2G;

    if (band == BAND_5G) {
        nominal_nf = SPECTRAL_PHY_CCA_NOM_VAL_AR9888_5GHZ;
    } else {
        nominal_nf = SPECTRAL_PHY_CCA_NOM_VAL_AR9888_2GHZ;
    }

    return nominal_nf;
}
#else
int ol_get_nominal_nf(struct ieee80211com *ic)
{
	return 0;
}
#endif /* ATH_SUPPORT_ICM */

static inline bool mbssid_tx_vap_deletion_sanity_check(
        struct ieee80211com *ic, const int param)
{
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(
            ic->ic_pdev_obj, WLAN_PDEV_F_MBSS_IE_ENABLE);
    int par = param;

    if (par & OL_ATH_PARAM_SHIFT) {
        par -= OL_ATH_PARAM_SHIFT;
    } else {
        return true;
    }

    if (par == OL_ATH_PARAM_WIFI_DOWN_IND){
        return !(!is_mbssid_enabled || !ic->ic_mbss_ctx
                || ic->ic_wifi_down_ind);
    }
    return true;
}

static void ol_ath_vap_iter_update_shpreamble(void *arg, wlan_if_t vap)
{
    bool val = (*(u_int32_t *)arg)?1:0;

    ol_ath_wmi_send_vdev_param(vap->vdev_obj, wmi_vdev_param_preamble,
            (val) ? WMI_HOST_VDEV_PREAMBLE_SHORT : WMI_HOST_VDEV_PREAMBLE_LONG);
}

/* ol_ath_vap_iter_update_wifi_3_0_fastpath() - Update the FASTPATH flag
 * for wifi transmit based on the arguement passed which is the status of ol_stats.
 * @arg : status of ol_stats
 * @vap : VAP interface
 */
static void ol_ath_vap_iter_update_wifi_3_0_fastpath(void *arg, wlan_if_t vap)
{
	osif_vap_tx_activity_update(vap);
}


/* ol_ath_update_cfg_chanlist_modecaps() - Update cfg80211's channel list,
 * ic mode capability and FW channel list after current channel list update.
 * This is to ensure that all components (FW, CFG80211, Host) have a
 * consistent channel list.
 * @ic : Pointer to struct ieee80211com.
 */
static void ol_ath_update_cfg_chanlist_modecaps(struct ieee80211com *ic)
{
    ieee80211_update_channellist(ic, 1, false);
#if UMAC_SUPPORT_CFG80211
    wlan_cfg80211_update_channel_list(ic);
#endif

}

/*
 * strict_mode_cc_change_check() - checks whether the new country is compatible
 *                                 with previous channel 3-tuple
 *                                 (freq/cfreq2/mode)
 * @ic: ic object handle
 * @freq: frequency
 * @cfreq2: cfreq2
 * @mode: phymode
 *
 * This function checks whether the new country is compatible with the
 * previous channel 3-tuple (passed as parameters to this function)
 *
 * Return: 0 if meets compatibility check
 * Return: -EINVAL if compatibility check fails and the driver needs to go
 *         back to the older country and channel
 */
static int strict_mode_cc_change_check(struct ieee80211com *ic,
	                               struct chan_params *chan_p,
                                       u_int32_t mode)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211_ath_channel chan;

    scn = OL_ATH_SOFTC_NET80211(ic);

    if (!wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj,
                                   WLAN_SOC_F_STRICT_CHANNEL))
        return 0;

    if (osif_num_ap_up_vaps(ic) == 0)
        return 0;

    chan = ieee80211_find_dot11_channel(&scn->sc_ic, chan_p, mode);
    if (IEEE80211_IS_CHAN_VALID(&chan))
        return 0;

    return -EINVAL;
}

int ol_ath_ucfg_setparam(void *vscn, int param, int value, int *extra_buffer)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    bool restart_vaps = FALSE;
    bool is_mbssid_enabled = FALSE;
    int retval = 0;
    uint16_t orig_cc;
#if QCA_11AX_STUB_SUPPORT
    uint32_t orig_rd;
#endif
    struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wmi_unified *wmi_handle;
    struct wmi_unified *pdev_wmi_handle;
    void *dbglog_handle;
    struct target_psoc_info *tgt_psoc_info;
    ol_txrx_soc_handle soc_txrx_handle;
    cdp_config_param_type val = {0};

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                                   WLAN_PDEV_F_MBSS_IE_ENABLE);

    if (!ol_ath_is_ifce_allowed_in_dynamic_hw_mode(scn)) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("IF %s blocked - hw-mode: %d "
                                  "hw-mode-switch-in-progress %s"),
                                  scn->netdev->name,
                                  scn->soc->hw_mode_ctx.current_mode,
                                  scn->soc->hw_mode_ctx.is_switch_in_progress ?
                                                                "YES" : "NO");
        return -EINVAL;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    qdf_spin_lock_bh(&scn->soc->soc_tgt_lock);
    if (qdf_atomic_test_bit(SOC_RESET_IN_PROGRESS_BIT,
                            &scn->soc->reset_in_progress)) {
        qdf_info("Soc %d Radio %s Reset in progress, return",
                 wlan_psoc_get_id(scn->soc->psoc_obj), scn->netdev->name);
        qdf_spin_unlock_bh(&scn->soc->soc_tgt_lock);
        return -1;
    }
    qdf_spin_unlock_bh(&scn->soc->soc_tgt_lock);

    /* In MBSSID case, allow setting wifi_down_ind for transmitting VAP
     * only once
     */
    if (!mbssid_tx_vap_deletion_sanity_check(ic, param)) {
        return -1;
    }

    if ( (!(param & OL_ATH_PARAM_SHIFT)) ||
       (((param - OL_ATH_PARAM_SHIFT) != OL_ATH_PARAM_STANDBY_ACTIVE_MODE) &&
         (param - OL_ATH_PARAM_SHIFT) != OL_ATH_PARAM_DYNAMIC_WSI_REMAP)) {
       if (ol_ath_check_and_start_target(scn))
           return -1;
    }

    wmi_handle = lmac_get_wmi_hdl(scn->soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(scn->soc->psoc_obj);
    if (tgt_psoc_info == NULL) {
        qdf_print("%s: target_psoc_info is null ", __func__);
        return -EINVAL;
    }


    /*
     ** Code Begins
     ** Since the parameter passed is the value of the parameter ID, we can call directly
     */
    if ( param & OL_ATH_PARAM_SHIFT )
    {
        /*
         ** It's an ATH value.  Call the  ATH configuration interface
         */

        param -= OL_ATH_PARAM_SHIFT;
        config_cmd_resp_log(scn->soc, CONFIG_TYPE_CMD, ic->ic_netdev->name, param, value);
        retval = ol_ath_set_config_param(scn, (enum _ol_ath_param_t)param,
                &value, &restart_vaps, extra_buffer);
        config_cmd_resp_log(scn->soc, CONFIG_TYPE_RESP, ic->ic_netdev->name, param, retval);
    }
    else if ( param & OL_SPECIAL_PARAM_SHIFT )
    {
        if (!(dbglog_handle = target_psoc_get_dbglog_hdl(tgt_psoc_info))) {
            qdf_err("%s: dbglog_handle is null ", __func__);
            return -EINVAL;
        }

        param -= OL_SPECIAL_PARAM_SHIFT;

        switch (param) {
            case OL_SPECIAL_PARAM_COUNTRY_ID:
            {
                struct ieee80211_ath_channel *orig_chan;
                struct ol_ath_softc_net80211 *scn;
		struct chan_params orig_chan_p = {0};
                u_int32_t orig_mode;

                scn = OL_ATH_SOFTC_NET80211(ic);
                orig_cc = ieee80211_getCurrentCountry(ic);
                orig_chan = ieee80211_get_current_channel(ic);

                orig_chan_p.freq = ieee80211_chan2freq(&scn->sc_ic, orig_chan);
                orig_chan_p.cfreq2 = orig_chan->ic_vhtop_freq_seg2;
		set_chan_params_puncture_bitmap(&orig_chan_p, 0);
		/* check pp */
                orig_mode = ieee80211_chan2mode(orig_chan);

                retval = wlan_set_countrycode(&scn->sc_ic, NULL, value, CLIST_NEW_COUNTRY);
                if (retval) {
                    qdf_print("%s: Unable to set country code ",__func__);
                    retval = wlan_set_countrycode(&scn->sc_ic, NULL,
                                    orig_cc, CLIST_NEW_COUNTRY);
                } else if (strict_mode_cc_change_check(ic, &orig_chan_p,
                                                       orig_mode)) {

                        /*
                         * Check if the original channel is available in
                         * the new Reg-domain. If not, revert back to the
                         * original reg-domain and also restore the original
                         * channel
                         */

                        qdf_err("ERROR!! Strict mode: "
                                "The new country code does not support current"
                                " channel/phymode combination\n");

                        wlan_set_countrycode(&scn->sc_ic, NULL,
                                    orig_cc, CLIST_NEW_COUNTRY);
                        osif_restart_for_config(ic, ieee80211_set_channel_for_cc_change, orig_chan);
                        return -1;
                }
                break;
            }
            case OL_SPECIAL_DBGLOG_REPORT_SIZE:
                fwdbg_set_report_size(dbglog_handle, scn, value);
                break;
            case OL_SPECIAL_DBGLOG_TSTAMP_RESOLUTION:
                fwdbg_set_timestamp_resolution(dbglog_handle, scn, value);
                break;
            case OL_SPECIAL_DBGLOG_REPORTING_ENABLED:
                fwdbg_reporting_enable(dbglog_handle, scn, value);
                break;
            case OL_SPECIAL_DBGLOG_LOG_LEVEL:
                fwdbg_set_log_lvl(dbglog_handle, scn, value);
                break;
            case OL_SPECIAL_DBGLOG_VAP_ENABLE:
                fwdbg_vap_log_enable(dbglog_handle, scn, value, TRUE);
                break;
            case OL_SPECIAL_DBGLOG_VAP_DISABLE:
                fwdbg_vap_log_enable(dbglog_handle, scn, value, FALSE);
                break;
            case OL_SPECIAL_DBGLOG_MODULE_ENABLE:
                fwdbg_module_log_enable(dbglog_handle, scn, value, TRUE);
                break;
            case OL_SPECIAL_DBGLOG_MODULE_DISABLE:
                fwdbg_module_log_enable(dbglog_handle, scn, value, FALSE);
                break;
            case OL_SPECIAL_PARAM_DISP_TPC:
                wmi_unified_pdev_get_tpc_config_cmd_send(pdev_wmi_handle, value);
                break;
            case OL_SPECIAL_PARAM_ENABLE_CH_144:
                pdev =  ic->ic_pdev_obj;
                if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_REGULATORY_SB_ID) !=
                        QDF_STATUS_SUCCESS) {
                    qdf_print("%s, %d unable to get reference", __func__, __LINE__);
                    return -EINVAL;
                }

                psoc = wlan_pdev_get_psoc(pdev);
                if (psoc == NULL) {
                    qdf_print("%s: psoc is NULL", __func__);
                    wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);
                    return -EINVAL;
                }

                reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
                if (!(reg_rx_ops && reg_rx_ops->reg_set_chan_144)) {
                    qdf_print("%s : reg_rx_ops is NULL", __func__);
                    wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);
                    return -EINVAL;
                }

                reg_rx_ops->reg_set_chan_144(pdev, value);
                wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);

#ifdef WLAN_REG_PARTIAL_OFFLOAD
                if (!wmi_service_enabled(wmi_handle, wmi_service_regulatory_db)) {
                    ieee80211_reg_create_ieee_chan_list(&scn->sc_ic);
                }
#endif
                ol_ath_update_cfg_chanlist_modecaps(ic);
                break;
            case OL_SPECIAL_PARAM_ENABLE_CH144_EPPR_OVRD:
                ol_regdmn_set_ch144_eppovrd(scn->ol_regdmn_handle, value);
                orig_cc = ieee80211_getCurrentCountry(ic);
                retval = wlan_set_countrycode(&scn->sc_ic, NULL, orig_cc, CLIST_NEW_COUNTRY);
                break;
            case OL_SPECIAL_PARAM_REGDOMAIN:
                {
                    struct ieee80211_ath_channel chan_obj = ieee80211_get_current_channel_obj(&scn->sc_ic);
                    struct ieee80211_ath_channel *chan = &chan_obj;
                    u_int64_t flags = chan->ic_flags;
                    struct chan_params chan_p = {0};
                    u_int32_t mode = ieee80211_chan2mode(chan);
                    wlan_if_t tmpvap;
                    bool no_chanchange = false;
                    bool is_strict_channel_mode =
                    wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj,
                                              WLAN_SOC_F_STRICT_CHANNEL);

                    chan_p.freq = ieee80211_chan2freq(&scn->sc_ic, chan);
                    chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
		    set_chan_params_puncture_bitmap(&chan_p, 0);
		    /*check pp */
                    /*
                     * After this command, the channel list would change.
                     * must set ic_curchan properly.
                     */
                     /* skip channel change if any vdev is active, it is handled in below code */
                      if (ieee80211_get_num_active_vaps(ic) != 0)
                          no_chanchange = true;

#if QCA_11AX_STUB_SUPPORT
                    orig_rd = ieee80211_get_regdomain(ic);
                    retval = ol_ath_set_regdomain(ic, value, no_chanchange);
                    if (retval) {
                        qdf_print("%s: Unable to set regdomain, restore orig_rd = %d",__func__, orig_rd);
                        retval = ol_ath_set_regdomain(ic, orig_rd, no_chanchange);
                        if (retval) {
                            qdf_print("%s: Unable to set regdomain", __func__);
                            return -1;
                        }
                    } else if (strict_mode_cc_change_check(ic, &chan_p, mode)) {

                        /*
                         * Check if the original channel is available in
                         * the new regdomain. If not, revert back to the
                         * original regdomain and also restore the original
                         * channel.
                         */

                        qdf_err("ERROR!! Strict mode: "
                                "The new regdomain does not support current"
                                " channel/phymode combination\n");
                        ol_ath_set_regdomain(ic, orig_rd, no_chanchange);
                        return -1;
                    }
#endif
		    if ((ieee80211_get_num_active_vaps(ic) != 0) ||
			 is_strict_channel_mode) {
			*chan = ieee80211_find_channel(&scn->sc_ic, &chan_p, flags); /* cfreq2 arguement will be ignored for non VHT80+80 mode */
			if (!IEEE80211_IS_CHAN_VALID(chan)) {
			    qdf_nofl_info("Current channel not supported in new RD. Configuring to a random channel\n");
                            chan_p.freq = 0;
			    chan_p.cfreq2 = 0;
		            set_chan_params_puncture_bitmap(&chan_p, 0);
			    *chan = ieee80211_find_dot11_channel(&scn->sc_ic, &chan_p, mode);
			    if (!IEEE80211_IS_CHAN_VALID(chan)) {
				*chan = ieee80211_find_dot11_channel(&scn->sc_ic, &chan_p, 0);
				if(!IEEE80211_IS_CHAN_VALID(chan))
				    return -1;
			    }
			    if(IEEE80211_IS_CHAN_VALID(chan)) {
				mode = ieee80211_chan2mode(chan);
				TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
				    tmpvap->iv_des_mode = mode;
				    tmpvap->iv_des_hw_mode = mode;
				}
			    }
			}
			*(scn->sc_ic.ic_curchan) = *chan;
			/* After re-building the channel list, both curchan and
			 * prevchan may point to same base address. Hence force
			 * restart is required.
			 */
			osif_restart_for_config(ic, &ieee80211_set_channel_for_cc_change, chan);
		    }
		    if (is_strict_channel_mode) {
			    struct net_device *tmpdev;
			    wlan_if_t tmpvap;
			    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
				    tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
				    if (!IS_UP(tmpdev)) {
					    IEEE80211_DELIVER_EVENT_CHANNEL_CHANGE(tmpvap, tmpvap->iv_des_chan[tmpvap->iv_des_mode]);
				    }
			    }
		    }
		}
                break;
            case OL_SPECIAL_PARAM_ENABLE_OL_STATS:
                pdev =  ic->ic_pdev_obj;
                if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_FTM_ID) !=
                    QDF_STATUS_SUCCESS) {
                        qdf_err("Unable to get pdev reference\n");
                        return -EINVAL;
                }

                psoc = wlan_pdev_get_psoc(pdev);
                if (!psoc) {
                        qdf_err("psoc is NULL\n");
                        wlan_objmgr_pdev_release_ref(pdev, WLAN_FTM_ID);
                        return -EINVAL;
                }

                if (value && wlan_psoc_nif_feat_cap_get(psoc,
							WLAN_SOC_F_TESTMODE_ENABLE)) {
                        qdf_err("Enabling of config enable_ol_stats not supported in testmode");
                        wlan_objmgr_pdev_release_ref(pdev, WLAN_FTM_ID);
                        return -EINVAL;
                }

                if (scn->sc_ic.ic_ath_enable_ap_stats) {
                    retval = scn->sc_ic.ic_ath_enable_ap_stats(&scn->sc_ic, value);
                    wlan_iterate_vap_list(&scn->sc_ic, ol_ath_vap_iter_update_wifi_3_0_fastpath, &value);
#if defined(OL_ATH_SUPPORT_LED) && defined(OL_ATH_SUPPORT_LED_POLL)
                    if (scn->soc->led_blink_rate_table && value) {
                        OS_SET_TIMER(&scn->scn_led_poll_timer, LED_POLL_TIMER);
                    }
#endif
                }
                wlan_objmgr_pdev_release_ref(pdev, WLAN_FTM_ID);
                break;
            case OL_SPECIAL_PARAM_ENABLE_OL_STATSv2:
                scn->enable_statsv2 = value;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                if (ic->nss_radio_ops) {
                    ic->nss_radio_ops->ic_nss_ol_enable_ol_statsv2(scn, value);
                }
#endif
                break;
            case OL_SPECIAL_PARAM_ENABLE_OL_STATSv3:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                if (ic->nss_radio_ops) {
                    ic->nss_radio_ops->ic_nss_ol_enable_v3_stats(scn, value);
                }
#endif
                /* cdp set should be called for vow stats enable/disable
                 * enable_statsv3 1 --> for vow stats,
                 * enable_statsv3 2 --> for peer jitter stats
                 * so value check is added
                 */
                if (!value || value == VOW_STATS_ENABLE) {
                    val.cdp_pdev_param_cfg_vow = value;
                    cdp_txrx_set_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                                            CDP_CONFIG_VOW, val);
                }
                wlan_iterate_vap_list(&scn->sc_ic, ol_ath_vap_iter_update_wifi_3_0_fastpath, NULL);
                break;
            case OL_SPECIAL_PARAM_SET_TX_LATENCY:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                if (ic->nss_radio_ops) {
                    ic->nss_radio_ops->ic_nss_ol_set_tx_latency_threshold(scn, value);
                }
#endif
                break;

            case OL_SPECIAL_PARAM_SET_TX_LATENCY_PKTLOG:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                 if (ic->nss_radio_ops) {
                     ic->nss_radio_ops->ic_nss_ol_set_tx_latency_pktlog_threshold(scn, value);
                 }
#endif
                  break;
            case OL_SPECIAL_PARAM_SET_V3_TID:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                 if (ic->nss_radio_ops) {
                     ic->nss_radio_ops->ic_nss_ol_set_v3_tx_drop_debug_tid(scn, value);
                 }
#endif
                 break;
            case OL_SPECIAL_PARAM_SET_TX_DROP_THRESHOLD:
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                 if (ic->nss_radio_ops) {
                     ic->nss_radio_ops->ic_nss_ol_set_tx_drop_threshold(scn, value);
                 }
#endif
                 break;
            case OL_SPECIAL_PARAM_ENABLE_MAC_REQ:
                /*
                 * The Mesh mode has a limitation in OL FW, where the VAP ID
                 * should be between 0-7. Since Static MAC request feature
                 * can send a VAP ID more than 7, we stop this by returning
                 * an error to the user.
                 * enable_macreq feature is not supported with WB enabled
                 */
                if (scn->sc_ic.ic_mesh_vap_support ||
                    ic->ic_wideband_capable) {
                    retval = -EINVAL;
                    break;
                }

                if(!TAILQ_EMPTY(&ic->ic_vaps)) {
                    qdf_err("Macreq state cannot be changed when ic has VAPs "
                            "available");
                    retval = -EINVAL;
                    break;
                }

                scn->macreq_enabled = value;
                qdf_info("Macreq feature: %d \n", scn->macreq_enabled);
                break;
            case OL_SPECIAL_PARAM_WLAN_PROFILE_ID_ENABLE:
                {
                    struct wlan_profile_params param;
                    qdf_mem_set(&param, sizeof(param), 0);
                    param.profile_id = value;
                    param.enable = 1;
                    return wmi_unified_wlan_profile_enable_cmd_send(
                                                pdev_wmi_handle, &param);
                }
            case OL_SPECIAL_PARAM_WLAN_PROFILE_TRIGGER:
                {
                    struct wlan_profile_params param;
                    qdf_mem_set(&param, sizeof(param), 0);
                    param.enable = value;

                    return wmi_unified_wlan_profile_trigger_cmd_send(
                                               pdev_wmi_handle, &param);
                }

#if ATH_DATA_TX_INFO_EN
            case OL_SPECIAL_PARAM_ENABLE_PERPKT_TXSTATS:
                if(value == 1){
                    scn->enable_perpkt_txstats = 1;
                }else{
                    scn->enable_perpkt_txstats = 0;
                }
                val.cdp_pdev_param_en_perpkt_txstats = value;
                cdp_txrx_set_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                                        CDP_CONFIG_ENABLE_PERPKT_TXSTATS, val);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                if (ic->nss_radio_ops) {
                    ic->nss_radio_ops->ic_nss_ol_set_perpkt_txstats(scn);
                }
#endif
                break;
#endif
            case OL_SPECIAL_PARAM_ENABLE_SHPREAMBLE:
                if (value) {
                    IEEE80211_ENABLE_SHPREAMBLE(ic);
                } else {
                    IEEE80211_DISABLE_SHPREAMBLE(ic);
                }
                wlan_iterate_vap_list(ic, ol_ath_vap_iter_update_shpreamble, &value);
                restart_vaps = true;
                break;
            case OL_SPECIAL_PARAM_ENABLE_SHSLOT:
                if (value)
                    ieee80211_set_shortslottime(&scn->sc_ic, 1);
                else
                    ieee80211_set_shortslottime(&scn->sc_ic, 0);
                wlan_pdev_beacon_update(ic);
                break;
            case OL_SPECIAL_PARAM_RADIO_MGMT_RETRY_LIMIT:
                /* mgmt retry limit 1-64 */
                if( value < OL_MGMT_RETRY_LIMIT_MIN || value > OL_MGMT_RETRY_LIMIT_MAX ){
                    qdf_nofl_info("mgmt retry limit invalid, should be in (1-64)\n");
                    retval = -EINVAL;
                }else{
                    retval = ic->ic_set_mgmt_retry_limit(scn->sc_pdev, value);
                }
                break;
            case OL_SPECIAL_PARAM_SENS_LEVEL:
                qdf_nofl_info("%s[%d] PARAM_SENS_LEVEL \n", __func__,__LINE__);
                retval = ol_ath_pdev_set_param(scn->sc_pdev,
                                               wmi_pdev_param_sensitivity_level,
                                               value);
                if (!retval) {
                    scn->rxsop_sens_lvl = (int32_t)value;
                }
                break;
            case OL_SPECIAL_PARAM_TX_POWER_5G:
                qdf_nofl_info("%s[%d] OL_SPECIAL_PARAM_TX_POWER_5G \n", __func__,__LINE__);
                retval = ol_ath_pdev_set_param(scn->sc_pdev,
                                               wmi_pdev_param_signed_txpower_5g,
                                               value);
                break;
            case OL_SPECIAL_PARAM_TX_POWER_2G:
                qdf_nofl_info("%s[%d] OL_SPECIAL_PARAM_TX_POWER_2G \n", __func__,__LINE__);
                retval = ol_ath_pdev_set_param(scn->sc_pdev,
                                               wmi_pdev_param_signed_txpower_2g,
                                               value);
                break;
            case OL_SPECIAL_PARAM_CCA_THRESHOLD:
                qdf_nofl_info("%s[%d] PARAM_CCA_THRESHOLD \n", __func__,__LINE__);
                retval = ol_ath_pdev_set_param(scn->sc_pdev,
                                               wmi_pdev_param_cca_threshold,
                                               value);
                if (retval == EOK)
                    scn->cca_threshold = (int32_t)value;
                break;
            case OL_SPECIAL_PARAM_BSTA_FIXED_IDMASK:
                scn->sc_bsta_fixed_idmask = value;
                break;
            default:
                retval = -EOPNOTSUPP;
                break;
        }
    }
    else
    {
        retval = (int) ol_hal_set_config_param(scn, (enum _ol_hal_param_t)param, &value);
    }

    if (restart_vaps == TRUE) {
        retval = wlan_mbss_stop_start_ap_monitor_vdevs(scn->sc_pdev, NULL);
    }

    return retval;
}

int ol_ath_ucfg_getparam(void *vscn, int param, int *val)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    struct ieee80211com *ic;
    int retval = 0;
    struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc = NULL;

    /*
     ** Code Begins
     ** Since the parameter passed is the value of the parameter ID, we can call directly
     */
    ic = &scn->sc_ic;
    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (scn->soc->down_complete) {
        if(cfg_get(psoc, CFG_OL_DISABLE_AUTO_TARGET_BRINGUP) == 0) {
           qdf_info("Starting the target before sending the command");
           if (ol_ath_target_start(scn->soc)) {
               qdf_info("failed to start the target");
               return -1;
           }
        } else {
           qdf_info("The radio is down");
           return -1;
        }
    }

    if ( param & OL_ATH_PARAM_SHIFT )
    {
        /*
         ** It's an ATH value.  Call the  ATH configuration interface
         */

        param -= OL_ATH_PARAM_SHIFT;
        if (ol_ath_get_config_param(scn, (enum _ol_ath_param_t)param, (void *)val))
        {
            retval = -EOPNOTSUPP;
        }
    }
    else if ( param & OL_SPECIAL_PARAM_SHIFT )
    {
        if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_COUNTRY_ID) ) {
            val[0] = ieee80211_getCurrentCountry(ic);
        } else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_ENABLE_CH_144) ) {
            pdev =  ic->ic_pdev_obj;
            if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_REGULATORY_SB_ID) !=
                    QDF_STATUS_SUCCESS) {
                qdf_print("%s, %d unable to get reference", __func__, __LINE__);
                return -EINVAL;
            }

            psoc = wlan_pdev_get_psoc(pdev);
            if (psoc == NULL) {
                qdf_print("%s: psoc is NULL", __func__);
                wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);
                return -EINVAL;
            }

            reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
            if (!(reg_rx_ops && reg_rx_ops->reg_get_chan_144)) {
                qdf_print("%s : reg_rx_ops is NULL", __func__);
                wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);
                return -EINVAL;
            }

            val[0] = reg_rx_ops->reg_get_chan_144(pdev);
            wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);

        } else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_REGDOMAIN) ) {
            *val = ieee80211_get_regdomain(ic);
        } else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_ENABLE_SHPREAMBLE) ) {
            val[0] = (scn->sc_ic.ic_caps & IEEE80211_C_SHPREAMBLE) != 0;
        } else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_ENABLE_SHSLOT) ) {
            val[0] = IEEE80211_IS_SHSLOT_ENABLED(&scn->sc_ic) ? 1 : 0;
        } else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_RADIO_MGMT_RETRY_LIMIT) ) {
            val[0] = scn->scn_mgmt_retry_limit;
        } else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_ENABLE_OL_STATS) ) {
#ifdef QCA_SUPPORT_CP_STATS
            *val = pdev_cp_stats_ap_stats_tx_cal_enable_get(ic->ic_pdev_obj);
#endif
        }
#if ATH_DATA_TX_INFO_EN
        else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_ENABLE_PERPKT_TXSTATS) ) {
            *val = scn->enable_perpkt_txstats;
        }
#endif
        else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_SENS_LEVEL) ) {
            qdf_nofl_info("%s[%d] PARAM_SENS_LEVEL %d\n", __func__,__LINE__,scn->rxsop_sens_lvl);
            *val = scn->rxsop_sens_lvl;
        }
        else if ( param == (OL_SPECIAL_PARAM_SHIFT | OL_SPECIAL_PARAM_MLO_OPER_MODE ) ) {
            cdp_config_param_type cdp_val = {0};

            cdp_txrx_get_psoc_param(wlan_psoc_get_dp_handle(psoc),
                                    CDP_CFG_GET_MLO_OPER_MODE,
                                    &cdp_val);
            *val = (uint32_t)cdp_val.cdp_psoc_param_mlo_oper_mode;
        }
        else {
            retval = -EOPNOTSUPP;
        }
    }
    else
    {
        if ( ol_hal_get_config_param(scn, (enum _ol_hal_param_t)param, (void *)val))
        {
            retval = -EOPNOTSUPP;
        }
    }

    return retval;
}

/**
 * ol_ath_ucfg_get_user_position() - Sends WMI command to FW to
 * request for user position of a peer in different MU-MIMO group
 * @vaphandle: handle to vap
 * @value: value
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_ucfg_get_user_position(wlan_if_t vaphandle, uint32_t value)
{
    struct ieee80211com *ic;
    struct wmi_unified *pdev_wmi_handle;
    QDF_STATUS status = QDF_STATUS_E_INVAL;

    if (!vaphandle) {
        qdf_info("get_user_pos:vap not available");
        return -EINVAL;
    }

    ic = vaphandle->iv_ic;
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    if (ieee80211_validate_aid(ic, value))
        status = wmi_send_get_user_position_cmd(pdev_wmi_handle, value);
    else
        qdf_info("Invalid AID value");

    return qdf_status_to_os_return(status);
}

/**
 * ol_ath_ucfg_reset_peer_mumimo_tx_count() - Sends WMI command to FW
 * to reset MU-MIMO tx count for a peer
 * @vaphandle: handle to vap
 * @value: value
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_ucfg_reset_peer_mumimo_tx_count(wlan_if_t vaphandle, uint32_t value)
{
    struct ieee80211com *ic;
    struct wmi_unified *pdev_wmi_handle;
    QDF_STATUS status = QDF_STATUS_E_INVAL;

    if (!vaphandle) {
        qdf_info("reset_peer_mumimo_tx_count:vap not available");
        return -EINVAL;
    }

    ic = vaphandle->iv_ic;
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    if (ieee80211_validate_aid(ic, value))
        status = wmi_send_reset_peer_mumimo_tx_count_cmd(pdev_wmi_handle,
                                                         value);
    else
        qdf_info("Invalid AID value");

    return qdf_status_to_os_return(status);
}

/**
 * ol_ath_ucfg_get_peer_mumimo_tx_count() - Send WMI command to FW
 * to request for Mu-MIMO packets transmitted for a peer
 * @vaphandle: handle to vap
 * @value: value
 *
 * Return: 0 on success, other value on failure
 */
int ol_ath_ucfg_get_peer_mumimo_tx_count(wlan_if_t vaphandle, uint32_t value)
{
    struct ieee80211com *ic;
    struct wmi_unified *pdev_wmi_handle;
    QDF_STATUS status = QDF_STATUS_E_INVAL;

    if (!vaphandle) {
        qdf_info("mumimo_tx_count:vap not available");
        return A_ERROR;
    }

    ic = vaphandle->iv_ic;
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    if (ieee80211_validate_aid(ic, value))
        status = wmi_send_get_peer_mumimo_tx_count_cmd(pdev_wmi_handle, value);
    else
        qdf_info("Invalid AID value");

    return qdf_status_to_os_return(status);
}

int ol_ath_ucfg_set_country(void *vscn, char *cntry)
{
    int retval;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    uint16_t orig_cc;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (scn->soc->down_complete) {
        if(cfg_get(psoc, CFG_OL_DISABLE_AUTO_TARGET_BRINGUP) == 0) {
           if (ol_ath_target_start(scn->soc)) {
               qdf_err("failed to start the target");
               return -1;
           }
        } else {
           qdf_info("The radio is down");
           return -1;
        }
    }

    if (&scn->sc_ic) {
        orig_cc = ieee80211_getCurrentCountry(&scn->sc_ic);
        retval=  wlan_set_countrycode(&scn->sc_ic, cntry, 0, CLIST_NEW_COUNTRY);
        if (retval) {
            qdf_print("%s: Unable to set country code", __func__);
            retval = wlan_set_countrycode(&scn->sc_ic, NULL, orig_cc, CLIST_NEW_COUNTRY);
        }
    } else {
        retval = -EOPNOTSUPP;
    }

    return retval;
}

int ol_ath_ucfg_get_country(void *vscn, char *str)
{
    int retval = 0;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    struct ieee80211com *ic = &scn->sc_ic;

    ieee80211_getCurrentCountryISO(ic, str);

    return retval;
}

int ol_ath_ucfg_set_mac_address(void *vscn, char *addr)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    struct net_device *dev = scn->netdev;
    struct ieee80211com *ic = &scn->sc_ic;
    struct sockaddr sa;
    int retval;
    bool is_mbssid_enabled = FALSE;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                                   WLAN_PDEV_F_MBSS_IE_ENABLE);
    if (!IEEE80211_ADDR_IS_VALID(addr)) {
        qdf_print("%s : Configured invalid mac address", __func__);
        return -1;
    }

    if (scn->soc->down_complete) {
        if(cfg_get(psoc, CFG_OL_DISABLE_AUTO_TARGET_BRINGUP) == 0) {
           if (ol_ath_target_start(scn->soc)) {
               qdf_err("failed to start the target");
               return -1;
           }
        } else {
           qdf_info("The radio is down");
           return -1;
        }
    }

    if ( !TAILQ_EMPTY(&ic->ic_vaps) ) {
        retval = -EBUSY; //We do not set the MAC address if there are VAPs present
    } else {
        IEEE80211_ADDR_COPY(&sa.sa_data, addr);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
        retval = dev->netdev_ops->ndo_set_mac_address(dev, &sa);
        if (is_mbssid_enabled || (ic->ic_mbss_ctx && ic->ic_wideband_capable)) {
            /* Do not request partially random ref_bssid.
             * Currently, we rely on user to set mac address
             * such that it is sufficiently different (variant
             * at least in last n+1 bits of the LSB octets, n
             * being max-bssid indicator) across different
             * physical interfaces
             */
            ol_ath_assign_mbssid_ref_bssid(scn, false);
        } else if (ic->ic_cohosted_bss_support) {
            ol_ath_assign_cohosted_ref_bssid(scn, false);
        }
#else
        retval = dev->set_mac_address(dev, &sa);
#endif
    }

    return retval;
}

#define BTCOEX_MAX_PERIOD   2000   /* 2000 msec */
int ol_ath_ucfg_btcoex_duty_cycle(void *vscn, u_int32_t bt_period, u_int32_t bt_duration)

{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    int period,duration;
    uint8_t btcoex_support;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;

    btcoex_support = wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj,
					WLAN_SOC_F_BTCOEX_SUPPORT);
    period = (int)bt_period;
    duration = (int)bt_duration;
    if (btcoex_support && scn->soc->btcoex_duty_cycle) {
        if ( period  > BTCOEX_MAX_PERIOD ) {
            qdf_print("Invalid period : %d ",period);
            qdf_print("Allowed max period is 2000ms ");
            return -EINVAL;
        }
        if ( period < 0 || duration < 0) {
            qdf_print("Invalid values. Both period and must be +ve values ");
            return -EINVAL;
        }
        if( period < duration ) { /* period must be >= duration */
            qdf_print("Invalid values. period must be >= duration. period:%d duration:%d ",
                    period, duration);
            return -EINVAL;
        }

        if (period == 0 && duration == 0) {
            qdf_print("Both period and duration set to 0. Disabling this feature. ");
        }
        if (ol_ath_btcoex_duty_cycle(scn->soc, bt_period, bt_duration) == EOK ) {
            scn->soc->btcoex_duration = duration;
            scn->soc->btcoex_period = period;
        } else {
            qdf_print("BTCOEX Duty Cycle configuration is not success. ");
        }
    } else {
        qdf_print("btcoex_duty_cycle service not started. btcoex_support:%d btcoex_duty_cycle:%d ",
                btcoex_support, scn->soc->btcoex_duty_cycle);
        return -EPERM;
    }
    return 0;
}

int ol_ath_ucfg_send_egid_info(void *vscn, u_int32_t *egid_info)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;

    if (ol_ath_target_start(scn->soc)) {
        qdf_info("failed to start the target");
        return -1;
    }
    if (ol_ath_send_egid_info_cmd(scn->soc, egid_info) == 0)
        qdf_info("ESL EGID Info is sent successfully ");
    else
        return -EPERM;

    return 0;
}

#if UNIFIED_SMARTANTENNA
int ol_ath_ucfg_set_smart_antenna_param(void *vscn, char *val)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status;
    int ret = -1;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *obj_pdev;
    struct wlan_objmgr_psoc *psoc;

    obj_pdev = ic->ic_pdev_obj;
    if (obj_pdev == NULL) {
       qdf_err("%s : obj_pdev is NULL", __func__);
       return -EINVAL;
    }


    psoc = wlan_pdev_get_psoc(obj_pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;

    pdev = scn->sc_pdev;
    status = wlan_objmgr_pdev_try_get_ref(pdev, WLAN_SA_API_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_print("%s, %d unable to get pdev reference", __func__, __LINE__);
        return ret;
    }

    ret = target_if_sa_api_ucfg_set_param(pdev, val);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_SA_API_ID);
    return ret;
}

int ol_ath_ucfg_get_smart_antenna_param(void *vscn, char *val)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status;
    int ret = -1;

    pdev = scn->sc_pdev;
    status = wlan_objmgr_pdev_try_get_ref(pdev, WLAN_SA_API_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_print("%s, %d unable to get pdev reference", __func__, __LINE__);
        return ret;
    }

    ret = target_if_sa_api_ucfg_get_param(pdev, val);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_SA_API_ID);
    return ret;
}
#endif

#if PEER_FLOW_CONTROL
void ol_ath_ucfg_txrx_peer_stats(void *vscn, char *addr)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    cdp_per_peer_stats(soc_txrx_handle, addr);
}
#endif

void ol_ath_get_dp_fw_peer_stats(void *vscn, char *addr, uint8_t caps)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return;
    }

    if (ol_ath_check_and_start_target(scn))
        return;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_get_dp_fw_peer_stats(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev), addr, caps, 0);
}

void ol_ath_set_ba_timeout(void *vscn, uint8_t ac, uint32_t value)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_set_ba_timeout(soc_txrx_handle, ac, value);
}

void ol_ath_get_ba_timeout(void *vscn, uint8_t ac, uint32_t *value)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    ol_txrx_soc_handle soc_txrx_handle;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_get_ba_timeout(soc_txrx_handle, ac, value);
}

void ol_ath_get_dp_htt_stats(void *vscn, void* data, uint32_t data_len)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return;
    }

    if (scn->soc->down_complete) {
        if(cfg_get(psoc, CFG_OL_DISABLE_AUTO_TARGET_BRINGUP) == 0) {
           if (ol_ath_target_start(scn->soc)) {
               qdf_err("failed to start the target");
               return;
           }
        } else {
           qdf_info("The radio is down");
           return;
        }
    }

    if (!(soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj)))
        return;

    cdp_get_dp_htt_stats(soc_txrx_handle,
                         wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                         data, data_len);
}

void ol_ath_get_cp_wmi_stats(void *vscn, void *buf_ptr, uint32_t buf_len)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);

    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return;
    }

    if (ol_ath_check_and_start_target(scn))
        return;

    scn->soc->cp_stats_ic = &scn->sc_ic;
    if(wmi_unified_send_cp_stats_cmd(pdev_wmi_handle,
                                buf_ptr, buf_len)) {
        scn->soc->cp_stats_ic = NULL;
        qdf_nofl_info("%s:Unable to send wmi cmd\n", __func__);
        return;
    }
}

int ol_ath_get_target_pdev_id(void *vscn, uint32_t *val)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    struct wmi_unified *pdev_wmi_handle;
    uint32_t target_pdev_id;
    uint32_t host_pdev_id;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_err("%s : pdev is NULL ", __func__);
        return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -1;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;

    host_pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    if (wmi_convert_pdev_id_host_to_target(
            pdev_wmi_handle, host_pdev_id,
            &target_pdev_id) != QDF_STATUS_SUCCESS) {
        qdf_info("failed to convert host pdev id to target");
        return -1;
    }
    *val = target_pdev_id;
    return 0;
}

void ol_ath_get_cp_halphy_stats (void *vscn, void *buf_ptr, uint32_t buf_len)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);

    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return;
    }

    if (ol_ath_target_start(scn->soc)) {
        qdf_err("failed to start the target");
        return;
    }

    scn->soc->cp_stats_ic = &scn->sc_ic;
    if(wmi_unified_send_halphy_stats_cmd(pdev_wmi_handle,
                                buf_ptr, buf_len)) {
        scn->soc->cp_stats_ic = NULL;
        qdf_nofl_info("%s:Unable to send wmi cmd\n", __func__);
        return;
    }

}

int ol_ath_ucfg_create_vap(struct ol_ath_softc_net80211 *scn, struct ieee80211_clone_params *cp, char *dev_name)
{
    struct net_device *dev = scn->netdev;
    struct ifreq ifr;
    int status;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (scn->soc->sc_in_delete) {
        return -ENODEV;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;
 
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_data = (void *) cp;

    /*
     * If the driver is compiled with FAST_PATH option
     * and the driver mode is offload, we override the
     * fast path entry point for Tx
     */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if(scn->nss_radio.nss_rctx) {
        scn->sc_osdev->vap_hardstart = osif_nss_ol_vap_hardstart;
    } else
#endif
    {
        if (ol_target_lithium(scn->soc->psoc_obj)) {
            scn->sc_osdev->vap_hardstart = osif_ol_vap_hardstart_wifi3;
        } else {
            scn->sc_osdev->vap_hardstart = osif_ol_ll_vap_hardstart;
        }
    }
#endif
    status = osif_ioctl_create_vap(dev, &ifr, cp, scn->sc_osdev);

    /* return final device name */
    strlcpy(dev_name, ifr.ifr_name, IFNAMSIZ);

    return status;
}

/*
 * Function to handle UTF commands from QCMBR and FTM daemon
 */
int ol_ath_ucfg_utf_unified_cmd(void *data, int cmd, char *userdata, unsigned int length)
{
    int error = -1;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)data;
    struct ieee80211com *ic = NULL;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    ic = &scn->sc_ic;
    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;

    switch (cmd)
    {
#ifdef QCA_WIFI_FTM
#ifdef QCA_WIFI_FTM_IOCTL
        case ATH_XIOCTL_UNIFIED_UTF_CMD: /* UTF command from QCMBR */
        case ATH_XIOCTL_UNIFIED_UTF_RSP: /* UTF command to send response to QCMBR */
            {
                error = wlan_ioctl_ftm_testmode_cmd(pdev, cmd, (u_int8_t*)userdata);
            }
            break;
#endif
#ifdef QCA_WIFI_FTM_NL80211
        case ATH_FTM_UTF_CMD: /* UTF command from FTM daemon */
            {
                if (length > MAX_UTF_LENGTH) {
                    QDF_TRACE(QDF_MODULE_ID_CONFIG, QDF_TRACE_LEVEL_ERROR, "length: %d, max: %d \n",
                            length, MAX_UTF_LENGTH);
                    return -EFAULT;
                }

                error = wlan_cfg80211_ftm_testmode_cmd(pdev, (u_int8_t*)userdata, length);
            }
            break;
#endif
#endif
        default:
            qdf_print("FTM not supported\n");
    }

    return error;
}

int ol_ath_ucfg_get_ath_stats(void *vscn, void *vasc)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ath_stats_container *asc = (struct ath_stats_container *)vasc;
    struct net_device *dev = scn->netdev;
    struct ol_stats *stats;
    uint32_t size = MAX(sizeof(struct ol_stats), sizeof(struct cdp_pdev_stats));
    int error=0;
    ol_txrx_soc_handle soc_txrx_handle;
    struct ieee80211com *ic = NULL;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    ic = &scn->sc_ic;
    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if(((dev->flags & IFF_UP) == 0)){
        return -ENXIO;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    if (!soc_txrx_handle) {
        qdf_err("psoc dp handle %pK is NULL",
                soc_txrx_handle);
        return -EFAULT;
    }

    stats = OS_MALLOC(&scn->sc_osdev, size, GFP_KERNEL);
    if (stats == NULL)
        return -ENOMEM;

    if (asc->size == 0 || asc->address == NULL || asc->size < size) {
        error = -EFAULT;
    } else {
        if (ol_ath_check_and_start_target(scn))
            return -1;
 
        if (ol_target_lithium(scn->soc->psoc_obj)) { /* lithium */

            stats->txrx_stats_level =
                cdp_stats_publish(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                        (void *)stats);
            if (stats->txrx_stats_level  == TXRX_STATS_LEVEL_OFF) {
                OS_FREE(stats);
                return -EFAULT;
            }
            asc->offload_if = FLAG_LITHIUM;
            asc->size = sizeof(struct cdp_pdev_stats);
        }
        else {
            stats->txrx_stats_level =
                cdp_stats_publish(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                    (void *)stats);
            if (stats->txrx_stats_level  == TXRX_STATS_LEVEL_OFF) {
                OS_FREE(stats);
                return -EFAULT;
            }
	        /*
             * TODO: This part of the code is specific to Legacy and should
             * should be moved to Legacy DP (ol_txrx) layer
	         */
            ol_get_radio_stats(scn,&stats->interface_stats);
            asc->offload_if = FLAG_PARTIAL_OL;
            asc->size = sizeof(struct ol_stats);

            if(asc->flag_ext & EXT_TXRX_FW_STATS) {  /* fw stats */
                struct ieee80211vap *vap = NULL;
                struct ol_txrx_stats_req req = {0};

                vap = ol_ath_vap_get(scn, 0);
                if (vap == NULL) {
                    qdf_nofl_info("%s, vap not found!\n", __func__);
                    return -ENXIO;
                }

                req.wait.blocking = 1;
                req.stats_type_upload_mask = 1 << (TXRX_FW_STATS_TID_STATE - 2);
                req.copy.byte_limit = sizeof(struct wlan_dbg_tidq_stats);
                req.copy.buf = OS_MALLOC(&scn->sc_osdev,
                            sizeof(struct wlan_dbg_tidq_stats), GFP_KERNEL);
                if(req.copy.buf == NULL) {
                    qdf_nofl_info("%s, no memory available!\n", __func__);
                    ol_ath_release_vap(vap);
                    return -ENOMEM;
                }

                if (cdp_fw_stats_get(soc_txrx_handle, wlan_vdev_get_id(vap->vdev_obj), &req, PER_RADIO_FW_STATS_REQUEST, 0) != 0) {
                    OS_FREE(req.copy.buf);
                    ol_ath_release_vap(vap);
                    return -EIO;
                }

                OS_MEMCPY(&stats->tidq_stats, req.copy.buf,
                        sizeof(struct wlan_dbg_tidq_stats));
                OS_FREE(req.copy.buf);
                ol_ath_release_vap(vap);
            } /* fw stats */
        } /* lithium */

        if (_copy_to_user(asc->address, stats, size))
            error = -EFAULT;
        else
            error = 0;
    }

    OS_FREE(stats);

    return error;
}

int ol_ath_ucfg_get_vap_iface_names(struct ol_ath_softc_net80211 *scn,
				    struct acfg_vap_iface_names *profile)
{
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211vap *vap = NULL;
    osif_dev *osif = NULL;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
        if (osif == NULL) {
            qdf_err("osif null");
            return -1;
        }

        if (IEEE80211_IS_MLO_BRIDGE_VAP(vap))
            continue;

        if (profile->vap_count >= ACFG_MAX_VAPS) {
            qdf_err("max vap limit exceeded.Returning!!");
            return -1;
        }

        if (strlcpy(profile->name[profile->vap_count],
                    osif->netdev->name, IFNAMSIZ) >= IFNAMSIZ) {
            qdf_err("source too long");
            return -1;
        }
        profile->vap_count++;
    }
    return 0;
}

int ol_ath_ucfg_get_vap_info(struct ol_ath_softc_net80211 *scn,
                                struct ieee80211_profile *profile)
{
    struct net_device *dev = scn->netdev;
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211vap *vap = NULL;
    struct ieee80211vap_profile *vap_profile;
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;

    strlcpy(profile->radio_name, dev->name, IFNAMSIZ);
    wlan_get_device_mac_addr(ic, profile->radio_mac);
    profile->cc = (u_int16_t)wlan_get_device_param(ic,
                                IEEE80211_DEVICE_COUNTRYCODE);
    *chan = wlan_get_dev_current_channel(ic);
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        profile->channel = chan->ic_ieee;
        profile->freq = chan->ic_freq;
    }
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        vap_profile = &profile->vap_profile[profile->num_vaps];
        wlan_get_vap_info(vap, vap_profile, (void *)scn->sc_osdev);
        profile->num_vaps++;
    }
    return 0;
}

int ol_ath_ucfg_get_nf_dbr_dbm_info(struct ol_ath_softc_net80211 *scn)
{
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    if(wmi_unified_nf_dbr_dbm_info_get_cmd_send(pdev_wmi_handle,
                                lmac_get_pdev_idx(scn->sc_pdev))) {
        qdf_nofl_info("%s:Unable to send request to get NF dbr dbm info\n", __func__);
        return -1;
    }
    return 0;
}

int ol_ath_ucfg_get_packet_power_info(struct ol_ath_softc_net80211 *scn,
                                      struct packet_power_info_params *param)
{
    if (ol_ath_packet_power_info_get(scn->sc_pdev, param)) {
        qdf_info("Unable to send request to get packet power info");
        return -1;
    }
    return 0;
}

#if defined(ATH_SUPPORT_DFS) || defined(WLAN_SPECTRAL_ENABLE)
int ol_ath_ucfg_phyerr(void *vscn, void *vad)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ath_diag *ad = (struct ath_diag *)vad;
    struct ieee80211com *ic = &scn->sc_ic;
    void *indata=NULL;
    void *outdata=NULL;
    int error = -EINVAL;
    u_int32_t insize = ad->ad_in_size;
    u_int32_t outsize = ad->ad_out_size;
#if WLAN_SPECTRAL_ENABLE
    QDF_STATUS status;
    struct spectral_cp_request sscan_req;
#endif /* WLAN_SPECTRAL_ENABLE */
    u_int id= ad->ad_id & ATH_DIAG_ID;
#if ATH_SUPPORT_DFS
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_dfs_rx_ops *dfs_rx_ops;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is NULL", __func__);
        return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);

    if (psoc == NULL) {
        qdf_print("%s : psoc is NULL", __func__);
        return -EINVAL;
    }

    dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
#endif

    if (ol_ath_check_and_start_target(scn))
        return -1;

    if (ad->ad_id & ATH_DIAG_IN) {
        /*
         * Copy in data.
         */
        indata = OS_MALLOC(scn->sc_osdev,insize, GFP_KERNEL);
        if (indata == NULL) {
            error = -ENOMEM;
            goto bad;
        }
        if (__xcopy_from_user(indata, ad->ad_in_data, insize)) {
            error = -EFAULT;
            goto bad;
        }
        id = id & ~ATH_DIAG_IN;
    }
    if (ad->ad_id & ATH_DIAG_DYN) {
        /*
         * Allocate a buffer for the results (otherwise the HAL
         * returns a pointer to a buffer where we can read the
         * results).  Note that we depend on the HAL leaving this
         * pointer for us to use below in reclaiming the buffer;
         * may want to be more defensive.
         */
        outdata = OS_MALLOC(scn->sc_osdev, outsize, GFP_KERNEL);
        if (outdata == NULL) {
            error = -ENOMEM;
            goto bad;
        }
        id = id & ~ATH_DIAG_DYN;
    }

#if ATH_SUPPORT_DFS
    if (dfs_rx_ops && dfs_rx_ops->dfs_control) {
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                QDF_STATUS_SUCCESS) {
            return -EINVAL;
        }

        dfs_rx_ops->dfs_control(pdev, id, indata, insize, outdata, &outsize, &error);

        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    }
#endif

#if WLAN_SPECTRAL_ENABLE
    if (error ==  -EINVAL ) {
        /* Set normal Spectral as the default mode */
        sscan_req.ss_mode = SPECTRAL_SCAN_MODE_NORMAL;
        sscan_req.req_id = id;
        status = ucfg_spectral_create_cp_req(&sscan_req, indata, insize);
        if (QDF_IS_STATUS_ERROR(status)) {
            error = -EINVAL;
            goto bad;
        }

        error = ucfg_spectral_control(ic->ic_pdev_obj, &sscan_req);

        status = ucfg_spectral_extract_response(&sscan_req, outdata, &outsize);
        if (QDF_IS_STATUS_ERROR(status)) {
            error = -EINVAL;
            goto bad;
        }
    }
#endif

    if (outsize < ad->ad_out_size)
        ad->ad_out_size = outsize;

    if (outdata &&
            _copy_to_user(ad->ad_out_data, outdata, ad->ad_out_size))
        error = -EFAULT;
bad:
    if ((ad->ad_id & ATH_DIAG_IN) && indata != NULL)
        OS_FREE(indata);
    if ((ad->ad_id & ATH_DIAG_DYN) && outdata != NULL)
        OS_FREE(outdata);

    return error;
}
#endif

int ol_ath_ucfg_ctl_set(struct ol_ath_softc_net80211 *scn, ath_ctl_table_t *ptr)
{
	struct ctl_table_params param;
	struct ieee80211com *ic = &scn->sc_ic;
	struct wmi_unified *pdev_wmi_handle;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev_wmi_handle is null");
		return -EINVAL;
	}
	if (!ptr)
		return -1;

	qdf_mem_set(&param, sizeof(param), 0);
	qdf_debug("%s[%d] Mode %d CTL table length %d\n", __func__,__LINE__,
			ptr->band, ptr->len);

	param.pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
	param.ctl_band = ptr->band;
	param.ctl_array = &ptr->ctl_tbl[0];
	param.ctl_cmd_len = ptr->len + sizeof(uint32_t);
	param.target_type = lmac_get_tgt_type(scn->soc->psoc_obj);
	if (IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan)) {
		param.is_2g = TRUE;
	} else {
		param.is_2g = FALSE;
	}

	if(QDF_STATUS_E_FAILURE ==
		wmi_unified_set_ctl_table_cmd_send(pdev_wmi_handle, &param)) {
		qdf_info("%s:Unable to set CTL table\n", __func__);
		return -1;
	}

	return 0;
}

int ol_ath_ucfg_sta_max_pwr_set(struct ol_ath_softc_net80211 *scn, ath_sta_pwr_table_t *ptr)
{
    struct sta_max_pwr_table_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    if (!ptr)
        return -1;

    qdf_mem_set(&param, sizeof(param), 0);
    qdf_info("%s[%d] num_peers %d", __func__,__LINE__,
               ptr->num_peers);
    param.num_peers = ptr->num_peers;
    param.peer_list = (struct sta_peer_table_list *)ptr->sta_pwr_table_list;
    if(QDF_STATUS_E_FAILURE ==
       wmi_unified_set_sta_max_pwr_table_cmd_send(pdev_wmi_handle, &param)) {
       qdf_err("%s:Unable to set STA max pwr table", __func__);
       return -1;
    }

    return 0;
}

int ol_ath_ucfg_rate2power_set(struct ol_ath_softc_net80211 *scn, ath_power_table_t *ptr)
{
    struct rate2power_table_params param;
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211com *ic = &scn->sc_ic;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    if (!ptr) {
        qdf_err("ath_power_table_t ptr is null");
        return -1;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
    param.freq_band = ptr->freq_band;
    param.sub_band = ptr->sub_band;
    param.pwr_array = &ptr->pwr_tbl[0];
    param.pwr_cmd_len = ptr->len;
    param.end_of_update = ptr->end_of_update;
    param.target_type = ptr->target_type;
    param.is_ext = ptr->is_ext;

    qdf_event_reset(&ic->ic_wait_for_r2p_table_set_response);
    if(QDF_STATUS_E_FAILURE ==
       wmi_unified_set_power_table_cmd_send(pdev_wmi_handle, &param)) {
       qdf_err("%s:Unable to set target rate2power table", __func__);
       return -1;
    }

    if (qdf_wait_single_event(&ic->ic_wait_for_r2p_table_set_response,
                              REG_R2P_PWR_TAB_RX_TIMEOUT)) {
        qdf_err("Did not receive response for rate2power table update");
        return -1;
    }

    return 0;
}

static void ol_ath_ucfg_copy_rates(wlan_if_t vap,
        struct ieee80211_rateset *op_rs,
        enum ieee80211_phymode mode)
{
    int i =0;

    for (i = 0; i < op_rs->rs_nrates; i++)
        vap->iv_op_rates[mode].rs_rates[i] = op_rs->rs_rates[i];
    vap->iv_op_rates[mode].rs_nrates = op_rs->rs_nrates;
}

/* Copy the supported rates to dependent modes */
static void ol_ath_ucfg_copy_supported_rates_to_lower_mode(wlan_if_t vap,
        enum ieee80211_phymode mode)
{
    struct ieee80211_rateset *op_rs = &(vap->iv_op_rates[mode]);

    switch(mode) {
        case IEEE80211_MODE_11NG_HT40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11NG_HT40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11NG_HT40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11NG_HT20);
            break;
        case IEEE80211_MODE_11NA_HT40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11NA_HT40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11NA_HT40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11NA_HT20);
            break;
        case IEEE80211_MODE_11AC_VHT40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AC_VHT40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AC_VHT40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AC_VHT40);
            break;
        case IEEE80211_MODE_11AXA_HE40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AXA_HE40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AXA_HE40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AXA_HE20);
            break;
        case IEEE80211_MODE_11AXG_HE40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AXG_HE40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AXG_HE40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11AXG_HE20);
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11BEA_EHT40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11BEA_EHT40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11BEA_EHT20);
            break;
        case IEEE80211_MODE_11BEG_EHT40:
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11BEG_EHT40PLUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11BEG_EHT40MINUS);
            ol_ath_ucfg_copy_rates(vap, op_rs, IEEE80211_MODE_11BEG_EHT20);
            break;
#endif /* WLAN_FEATURE_11BE */
        default:
            return;
    }
}

/*
 * @brief set basic & supported rates in beacon,
 * and use lowest basic rate as mgmt mgmt/bcast/mcast rates by default.
 * target_rates: an array of supported rates with bit7 set for basic rates.
 */
static int
ol_ath_ucfg_set_vap_op_support_rates(wlan_if_t vap, struct ieee80211_rateset *target_rs)
{
    struct ieee80211_node *bss_ni = vap->iv_bss;
    enum ieee80211_phymode mode = wlan_get_desired_phymode(vap);
    struct ieee80211_rateset *bss_rs = &(bss_ni->ni_rates);
    struct ieee80211_rateset *op_rs = &(vap->iv_op_rates[mode]);
    struct ieee80211_rateset *ic_supported_rs = &(vap->iv_ic->ic_sup_rates[mode]);
    uint8_t num_of_rates, num_of_basic_rates, i, j, rate_found=0;
    uint8_t basic_rates[IEEE80211_RATE_MAXSIZE];
    int32_t retv=0, min_rate=0;

    if (vap->iv_disabled_legacy_rate_set) {
        qdf_nofl_info("%s: need to unset iv_disabled_legacy_rate_set!",__FUNCTION__);
        return -EINVAL;
    }

    num_of_rates = target_rs->rs_nrates;
    if(num_of_rates > ACFG_MAX_RATE_SIZE){
        num_of_rates = ACFG_MAX_RATE_SIZE;
    }

    /* Check if the new rates are supported by the IC */
    for (i=0; i < num_of_rates; i++) {
        rate_found = 0;
        for (j=0; j < (ic_supported_rs->rs_nrates); j++) {
            if((target_rs->rs_rates[i] & IEEE80211_RATE_VAL) ==
                    (ic_supported_rs->rs_rates[j] & IEEE80211_RATE_VAL)){
                rate_found  = 1;
                break;
            }
        }
        if(!rate_found){
            qdf_nofl_info("Error: rate %d not supported in phymode %s !\n",
                    (target_rs->rs_rates[i]&IEEE80211_RATE_VAL)/2,ieee80211_phymode_name[mode]);
            return -EINVAL;
        }
    }

    /* Update BSS rates and VAP supported rates with the new rates */
    for (i=0; i < num_of_rates; i++) {
        bss_rs->rs_rates[i] = target_rs->rs_rates[i];
        op_rs->rs_rates[i] = target_rs->rs_rates[i];
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_XRATE, "rate %d (%d kbps) added\n",
                target_rs->rs_rates[i], (target_rs->rs_rates[i]&IEEE80211_RATE_VAL)*1000/2);
    }
    bss_rs->rs_nrates = num_of_rates;
    op_rs->rs_nrates = num_of_rates;

    if (IEEE80211_VAP_IS_PUREG_ENABLED(vap)) {
        /*For pureg mode, all 11g rates are marked as Basic*/
        ieee80211_setpuregbasicrates(op_rs);
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_XRATE,
            "%s Mark flag so that new rates will be used in next beacon update\n",__func__);
    vap->iv_flags_ext2 |= IEEE80211_FEXT2_BR_UPDATE;

    /* Find all basic rates */
    num_of_basic_rates = 0;
    for (i=0; i < bss_rs->rs_nrates; i++) {
        if(bss_rs->rs_rates[i] & IEEE80211_RATE_BASIC){
            basic_rates[num_of_basic_rates] = bss_rs->rs_rates[i];
            num_of_basic_rates++;
        }
    }
    if(!num_of_basic_rates){
        qdf_nofl_info("%s: Error, no basic rates set. \n",__FUNCTION__);
        return -EINVAL;
    }

    /* Find lowest basic rate */
    min_rate = basic_rates[0];
    for (i=0; i < num_of_basic_rates; i++) {
        if ( min_rate > basic_rates[i] ) {
            min_rate = basic_rates[i];
        }
    }

    /*
     * wlan_set_param supports actual rate in unit of kbps
     * min: 1000 kbps max: 300000 kbps
     */
    min_rate = ((min_rate&IEEE80211_RATE_VAL)*1000)/2;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_XRATE,
            "%s Set default mgmt/bcast/mcast rates to %d Kbps\n",__func__,min_rate);

    /* Use lowest basic rate as mgmt mgmt/bcast/mcast rates by default */
    retv = wlan_set_param(vap, IEEE80211_MGMT_RATE, min_rate);
    if(retv){
        return retv;
    }

    retv = wlan_set_param(vap, IEEE80211_BCAST_RATE, min_rate);
    if(retv){
        return retv;
    }

    retv = wlan_set_param(vap, IEEE80211_MCAST_RATE, min_rate);
    if(retv){
        return retv;
    }

    retv = wlan_set_param(vap, IEEE80211_BEACON_RATE_FOR_VAP, min_rate);
    if (retv) {
        return retv;
    }

    /* Use the lowest basic rate as RTS and CTS rates by default */
    retv = wlan_set_param(vap, IEEE80211_RTSCTS_RATE, min_rate);

    /* WAR:
     * When the AP is brought up in 2G with 11ng HT40 mode and acfg command is
     * used to set the basic rate, the supported rates are not reflected in
     * management packets.
     * User disables the radio and configures the supported rates using acfg
     * command. The acfg function updates the rates only for iv_des_mode
     * (11NG_HT40).
     * When user enables the radio, ieee80211_init_node_rates() is called to
     * update ni_rates from supported rates of current channel mode
     * (11NG_HT40PLUS/ 11NG_HT40MINUS). Since the user configured rates are not
     * copied to current channel mode (11NG_HT40PLUS/11NG_HT40MINUS), ni_rates
     * are updated with all the supported rates. Therefore, management frames
     * carry all the supported rates.
     * To address this, when user configures the supported rates for HT40 mode,
     * copy the configured rates to HT40+ and HT40-.
     * When there is an interference, the mode falls back to HT20. Therefore,
     * copy the configured rates to HT20 mode also.
     * This fix is applicable for 11NA_HT40, 11AC_VHT40, 11AXA_HE40 and
     * 11AXG_HE40 modes.
     */
    ol_ath_ucfg_copy_supported_rates_to_lower_mode(vap, mode);

    return retv;
}

int ol_ath_ucfg_set_op_support_rates(void *scn, struct ieee80211_rateset *target_rs)
{
    struct ol_ath_softc_net80211 *pscn = (struct ol_ath_softc_net80211 *)scn;
    struct ieee80211com *ic = &pscn->sc_ic;
    wlan_if_t tmpvap=NULL;
    int32_t retv = -EINVAL;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if(!tmpvap){
            return retv;
        }
        retv = ol_ath_ucfg_set_vap_op_support_rates(tmpvap, target_rs);
        if(retv){
            qdf_nofl_info("%s: Set VAP basic rates failed, retv=%d\n",__FUNCTION__,retv);
            return retv;
        }
    }

    return 0;
}

int ol_ath_ucfg_get_radio_supported_rates(struct ol_ath_softc_net80211 *scn,
        enum ieee80211_phymode mode,
        struct ieee80211_rateset *target_rs)
{
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211_rateset *rs = NULL;
    uint8_t j=0;

    if (!IEEE80211_SUPPORT_PHY_MODE(ic, mode)) {
        qdf_nofl_info("%s: The radio doesn't support this phymode: %d\n",__FUNCTION__,mode);
        return -EINVAL;
    }

    rs = &ic->ic_sup_rates[mode];
    if(!rs){
        return -EINVAL;
    }

    for (j=0; j<(rs->rs_nrates); j++) {
        target_rs->rs_rates[j] = rs->rs_rates[j];
    }
    target_rs->rs_nrates = rs->rs_nrates;

    return 0;
}

int ol_ath_ucfg_set_atf_sched_dur(void *vscn, uint32_t ac, uint32_t duration)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = NULL;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    ic = &scn->sc_ic;
    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;

    if (ac < WME_AC_BE || ac > WME_AC_VO) {
        qdf_err("Input AC value range out between 0 and 3!! ");
        return -1;
    }

    if ((duration < 0) || (duration > (1 << (30 - 1)))) {
        qdf_err("Input sched duration val range out of between 0 and 2^30-1");
        return -1;
    }

    return ol_ath_pdev_set_param(scn->sc_pdev,
                                 wmi_pdev_param_atf_sched_duration,
                                 (ac&0x03) << 30 | (0x3fffffff & duration));
}

int ol_ath_ucfg_set_aggr_burst(void *vscn, uint32_t ac, uint32_t duration)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;
    int retval = 0;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    pdev = ic->ic_pdev_obj;
    if (pdev == NULL) {
       qdf_err("%s : pdev is NULL", __func__);
       return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_err("%s: psoc is NULL", __func__);
        return -EINVAL;
    }

    if (!ic->ic_aggr_burst_support) {
        return -1;
    }

    if (ol_ath_check_and_start_target(scn))
        return -1;

    if (ac < WME_AC_BE || ac > WME_AC_VO) {
        return -1;
    }

    retval = ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_aggr_burst,
                                   (ac&0x0f) << 24 | (0x00ffffff & duration));

    if (EOK == retval)
        scn->aggr_burst_dur[ac] = duration;

    return retval;
}

int ol_ath_ucfg_set_pcp_tid_map(void *vscn, uint32_t pcp, uint32_t tid)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;
    int retval = 0;

    if ((pcp > OL_PCP_MAX) || (tid > OL_TID_MAX)) {
        qdf_nofl_err("Invalid input");
        return -EINVAL;
    }

    retval = ol_ath_set_default_pcp_tid_map(ic->ic_pdev_obj, pcp, tid);
    if (EOK == retval)
        ic->ic_pcp_tid_map[pcp] = tid;

    return retval;
}

int ol_ath_ucfg_get_pcp_tid_map(void *vscn, uint32_t pcp, uint32_t *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;
    int retval = 0;

    if (pcp > OL_PCP_MAX) {
        qdf_nofl_err("Invalid input");
        return -EINVAL;
    }

    *value = ic->ic_pcp_tid_map[pcp];

    return retval;
}

int ol_ath_ucfg_set_tidmap_prty(void *vscn, uint32_t value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;
    int retval = 0;
    uint32_t target_type = ic->ic_get_tgt_type(ic);

    /*
     * The priority value needs to be set for allowing the Vlan-pcp
     * to be used for deciding the TID number. The DSCP-based TID
     * mapping is the default value and doesnt need to be configured
     * explicitly.
     */
    switch (target_type) {
        case TARGET_TYPE_QCA8074:
        case TARGET_TYPE_QCA8074V2:
        case TARGET_TYPE_QCA9574:
        case TARGET_TYPE_QCA6018:
        case TARGET_TYPE_QCA5018:
        case TARGET_TYPE_QCN6122:
        case TARGET_TYPE_QCN9160:
            if (value > OL_TIDMAP_PRTY_HLOS_DSCP_CVLAN ) {
                qdf_err("Permissible value is 0-11");
                return -EINVAL;
            }
        break;
    }
    retval = ol_ath_set_default_tidmap_prty(ic->ic_pdev_obj, value);
    if (EOK == retval) {
        ic->ic_tidmap_prty = (uint8_t)value;
    }

    return retval;
}

int ol_ath_ucfg_get_tidmap_prty(void *vscn, uint32_t *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;
    int retval = 0;

    *value = (int)ic->ic_tidmap_prty;

    return retval;
}

#if OBSS_PD
/**
 * struct he_srp_ie_srg_bitmap_config - SRG bitmap config for HE SRP IE
 * @param: SRG bitmap config parameter
 * @val: pointer to the value to be configured
 */
struct he_srp_ie_srg_bitmap_config {
    uint32_t param;
    uint32_t *val;
};

/**
 * ol_ath_iter_vap_set_he_srg_bitmap() - Callback function to set SRG bitmap in
 * HE SRP IE for a given VAP
 * @arg: Pointer to he_srp_ie_srg_bitmap_config object
 * @vap: VAP for which this function is getting called
 *
 * Return: void
 */
static void ol_ath_iter_vap_set_he_srg_bitmap(void *arg, wlan_if_t vap)
{
    struct he_srp_ie_srg_bitmap_config *config =
        (struct he_srp_ie_srg_bitmap_config*)arg;
    vap->iv_ic->ic_vap_set_he_srg_bitmap(vap, config->val, config->param);
}

int
ol_ath_ucfg_set_he_srg_bitmap(void *vscn,
                    uint32_t *val,
                    uint32_t param)
{
    int retv = 0;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    struct he_srp_ie_srg_bitmap_config config;

    if (!ic) {
        qdf_err("IC is null");
        return -EINVAL;
    }

    if (!ic->ic_he_sr_enable) {
        qdf_err("Spatial Reuse Parameter Set Element is not enabled on this radio");
        return -EINVAL;
    }

    if (!ic->ic_he_srctrl_srg_info_present) {
        qdf_err("SRG based OBSS PD is not enabled in SRP IE");
        return -EINVAL;
    }

    switch(param) {
        case HE_SRP_IE_SRG_BSS_COLOR_BITMAP:
            ic->ic_he_srp_ie_srg_bss_color_bitmap[0] = val[0];
            ic->ic_he_srp_ie_srg_bss_color_bitmap[1] = val[1];
        break;

        case HE_SRP_IE_SRG_PARTIAL_BSSID_BITMAP:
            ic->ic_he_srp_ie_srg_partial_bssid_bitmap[0] = val[0];
            ic->ic_he_srp_ie_srg_partial_bssid_bitmap[1] = val[1];
        break;
        default:
            qdf_err("Unsupported Param: %d", param);
            retv = EINVAL;
        break;
    }

    config.val = val;
    config.param = param;

    /* Copy the radio level configuration to all the VAPs.
       Each VAP will take care of updatng IE in its management frames */
    if (ic->ic_vap_set_he_srg_bitmap)
        wlan_iterate_vap_list(ic, ol_ath_iter_vap_set_he_srg_bitmap,
                              (void *)&config);
    return retv;
}

int
ol_ath_ucfg_get_he_srg_bitmap(void *vscn, uint32_t *val, uint32_t param)
{
    int retv = 0;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;

    if (!ic) {
        qdf_err("IC is null");
        return -EINVAL;
    }

    if (!ic->ic_he_sr_enable) {
        qdf_err("Spatial Reuse Parameter Set Element is not enabled on this radio");
        return -EINVAL;
    }

    if (!ic->ic_he_srctrl_srg_info_present) {
        qdf_err("SRG based OBSS PD is not enabled in SRP IE");
        return -EINVAL;
    }

    switch(param) {
        case HE_SRP_IE_SRG_BSS_COLOR_BITMAP:
            val[0] = ic->ic_he_srp_ie_srg_bss_color_bitmap[0];
            val[1] = ic->ic_he_srp_ie_srg_bss_color_bitmap[1];
        break;

        case HE_SRP_IE_SRG_PARTIAL_BSSID_BITMAP:
            val[0] = ic->ic_he_srp_ie_srg_partial_bssid_bitmap[0];
            val[1] = ic->ic_he_srp_ie_srg_partial_bssid_bitmap[1];
        break;

        default:
            qdf_err("Unsupported Param: %d", param);
            retv = EINVAL;
        break;
    }

    return retv;
}

/**
 * struct he_srp_ie_config - Config for HE SRP IE
 * @param: HE SRP IE config parameter
 * @value , @data1, @data2: Values to be configured.
 */
struct he_srp_ie_config {
    uint32_t param;
    uint32_t value;
    uint32_t data1;
    uint32_t data2;
};

/**
 * ol_ath_iter_vap_set_he_sr_config() - Callback function to set HE SR config
 * for a given VAP
 * @arg: Pointer to he_srp_ie_config object
 * @vap: VAP for which this function is getting called
 *
 * Return: void
 */
static void ol_ath_iter_vap_set_he_sr_config(void *arg, wlan_if_t vap)
{
    struct he_srp_ie_config *config = (struct he_srp_ie_config *)arg;
    vap->iv_ic->ic_vap_set_he_sr_config(vap, config->param, config->value,
                    config->data1, config->data2);
}

#endif /* OBSS PD */

static int
ol_ath_ucfg_update_he_bsscolor(struct ieee80211com *ic,
                                uint8_t value,
                                uint8_t ovrride)
{
    struct ieee80211_bsscolor_handle *bsscolor_hdl = &ic->ic_bsscolor_hdl;
    bool bss_color_disabled = ((bsscolor_hdl->state ==
                                IEEE80211_BSS_COLOR_CTX_STATE_COLOR_USER_DISABLED) ||
                               (bsscolor_hdl->state ==
                                IEEE80211_BSS_COLOR_CTX_STATE_COLOR_DISABLED));
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                                WLAN_PDEV_F_MBSS_IE_ENABLE);

    if(ovrride > 1) {
        qdf_err("Override argument should be either 0 or 1");
        return -EINVAL;
    }

    if (!ieee80211_is_bcca_ongoing_for_any_vap(ic)) {
        /* Set the user set BSS color and the override
         * flag values, update the heop params fields.
         */
        if(value <= IEEE80211_HE_BSS_COLOR_MAX) {
            /* In disabled state, only need to check if non-zero
             * value is requested and enable feature if so.
             */
            if (bss_color_disabled) {
                if(value) {
                    ic->ic_he_bsscolor          = value;
                    ic->ic_he_bsscolor_override = ovrride;

                    /* By design, color selection algorithm exits
                     * with failure if it is already in user-disabled
                     * state. Change state to disabled if it is
                     * in user-disabled state so that select-API
                     * call executes and user can set a new color
                     * even if it disabled it in previous step.
                     */

                    if (bsscolor_hdl->state ==
                        IEEE80211_BSS_COLOR_CTX_STATE_COLOR_USER_DISABLED) {
                        bsscolor_hdl->state =
                            IEEE80211_BSS_COLOR_CTX_STATE_COLOR_DISABLED;
                    }
                    ieee80211_select_new_bsscolor(ic);
                }
            } else {
                /* If new non-zero color is selected update accordingly */
                if(ic->ic_he_bsscolor != value) {
                    if(value) {
                        ic->ic_he_bsscolor          = value;
                        ic->ic_he_bsscolor_override = ovrride;

                        ieee80211_select_new_bsscolor(ic);
                    } else {
                        bsscolor_hdl->state =
                            IEEE80211_BSS_COLOR_CTX_STATE_COLOR_USER_DISABLED;
                        bsscolor_hdl->prev_bsscolor =
                            bsscolor_hdl->selected_bsscolor;
                        ieee80211_set_ic_heop_bsscolor_disabled_bit(ic, true);
                    }
                } else {
                    QDF_TRACE(QDF_MODULE_ID_BSSCOLOR, QDF_TRACE_LEVEL_ERROR,
                        "HE BSS Color already set with this value=%d", value);
                    return EOK;

                }
            }
        } else {
                QDF_TRACE(QDF_MODULE_ID_BSSCOLOR, QDF_TRACE_LEVEL_ERROR,
                    "HE BSS color should be less then 63");
            return -EINVAL;
        }
    } else {
        QDF_TRACE(QDF_MODULE_ID_BSSCOLOR, QDF_TRACE_LEVEL_ERROR,
            "User update for BSS Color is prohibited"
             "during BSS Color Change Announcement");
        return -EINVAL;
    }

    if (ic->ic_mesh_mode || is_mbssid_enabled) {
        osif_pdev_restart_vaps(ic);
    }

    return EOK;
}

int
ol_ath_ucfg_set_he_bss_color(void *vscn, uint8_t value, uint8_t ovrride)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;

    if (scn->soc->down_complete) {
        qdf_err("SOC is not completely up, returning");
        return -EBUSY;
    }

    return ol_ath_ucfg_update_he_bsscolor(ic, value, ovrride);
}

int
ol_ath_ucfg_get_he_bss_color(void *vscn, int *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;
    bool staonlymode                  = true;
    struct ieee80211vap *vap;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            staonlymode = false;
            break;
        }
    }

    if (staonlymode) {
        /* cfg80211tool wifix get_he_bsscolor reflects the bss
         * color from an ic level variable. The bsscolor design
         * is such that for a particular radio all the AP VAPs
         * will have same bss color but a STA vap will assume
         * the bsscolor as advertised by the BSS it is associated to.
         */
        qdf_err("This cmd is not applicable in STA only mode");
        return -EINVAL;
    }

    value[0] = ic->ic_he_bsscolor;
    value[1] = ic->ic_he_bsscolor_override;

    return 0;
}

int
ol_ath_ucfg_set_he_mesh_config(void *vscn, void *args)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    uint8_t *mesh_config_args = (uint8_t *)args;
#if OBSS_PD
    struct he_srp_ie_config srg_args;
#endif /* OBSS_PD */
    uint8_t enable_mesh_mode, bsscolor;
    int retval = EOK;
    uint8_t num_ap, num_sta;

    num_ap = wlan_mbss_num_ap(scn->sc_pdev);
    num_sta = wlan_mbss_num_sta(scn->sc_pdev);

    enable_mesh_mode = mesh_config_args[0];
    bsscolor = mesh_config_args[1];

    if (enable_mesh_mode > 1) {
        qdf_err("Enable flag setting should be < 1");
        return -EINVAL;
    }

    if (enable_mesh_mode != ic->ic_mesh_mode) {
        ic->ic_mesh_mode = enable_mesh_mode;

        if (bsscolor == 0) {
            bsscolor = IEEE80211_HE_BSS_COLOR_DEFAULT;
            qdf_info("Disabling BSS color not supported with"
                    "'enable_mesh_mode' cmd. Use 'he_bsscolor' cmd\n"
                    "Setting default BSS color: 63");
        }

        if (ic->ic_mesh_mode) {
            /* Update bsscolor */
            ol_ath_ucfg_update_he_bsscolor(ic, bsscolor, 1);

#if OBSS_PD
            /* Enable OBSS PD for repeater scenario */
            if (num_ap && num_sta) {
                retval = ol_ath_set_obss_pd_enable_bit(
                    ic, 1, scn, SR_TYPE_NON_SRG_OBSS_PD);
            }
        }
        /* Disable non-SRG OBSS PD if mesh mode is enabled.
         * When mesh mode is disabled, reset the value to default.
         */
        srg_args.param = HE_SR_NON_SRG_OBSSPD_ENABLE;
        srg_args.value = (enable_mesh_mode) ? 0 :
                            ic->ic_he_srctrl_non_srg_obsspd_disallowed;
        srg_args.data1 = ic->ic_he_non_srg_obsspd_max_offset;
        srg_args.data2 = 0;

        wlan_iterate_vap_list(ic, ol_ath_iter_vap_set_he_sr_config,
                              (void *)&srg_args);
#else
        }
#endif /* OBSS_PD */
        retval = ol_ath_pdev_set_param(scn->sc_pdev,
                                    wmi_pdev_param_set_mesh_params,
                                    enable_mesh_mode);
    } else {
        qdf_info("Mesh mode setting is already set to %d",
                    enable_mesh_mode);
        return 0;
    }

    return retval;
}

int
ol_ath_ucfg_get_he_mesh_config(void *vscn, int *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;

    value[0] = ic->ic_mesh_mode;
    value[1] = ic->ic_he_bsscolor;

    return 0;
}

#ifdef WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG
int ol_ath_ucfg_set_rx_pkt_protocol_tagging(void *vscn,
                          struct ieee80211_rx_pkt_protocol_tag *rx_pkt_protocol_tag_info)
{
    struct wmi_rx_pkt_protocol_routing_info param;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com     *ic = &scn->sc_ic;
    ol_txrx_soc_handle soc_handle;
    struct wmi_unified *pdev_wmi_handle;
    bool update_protocol_mask = false;

    soc_handle = (ol_txrx_soc_handle) wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_handle) {
        qdf_err("psoc handle is NULL");
        return -EFAULT;
    }

    /* Get the PDEV ID and REO destination ring for this PDEV */
    param.pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    if (rx_pkt_protocol_tag_info->pkt_type == RECV_PKT_TYPE_EAP ||
	rx_pkt_protocol_tag_info->pkt_type == RECV_PKT_TYPE_IGMP_V4 ||
	rx_pkt_protocol_tag_info->pkt_type == RECV_PKT_TYPE_MLD_V6 ||
	ic->ic_pdev_obj->standby_active) {
        param.dest_ring = WBM_RELEASE_RING;
        param.dest_ring_handler = PDEV_WIFIRXCCE_USE_CCE_E;
    } else if (rx_pkt_protocol_tag_info->pkt_type == RECV_PKT_TYPE_VLAN) {
        param.dest_ring = cdp_host_reo_dest_ring_3;
        param.dest_ring_handler = PDEV_WIFIRXCCE_USE_CCE_E;
    } else {
        param.dest_ring = cdp_get_pdev_reo_dest(soc_handle, param.pdev_id);
        param.dest_ring_handler = PDEV_WIFIRXCCE_USE_FT_E;
    }

    /*
     * Mask need to be updated for packets received in REO ring not needed
     * for packets routed to WBM error ring
     */
    if (param.dest_ring != WBM_RELEASE_RING)
        update_protocol_mask = true;

    if (rx_pkt_protocol_tag_info->op_code == RX_PKT_TAG_OPCODE_ADD)
    {
      param.op_code = ADD_PKT_ROUTING;
      /* Add a constant offset to protocol type before passing it as metadata */
      /*
      * The reason for passing on the packet_type instead of actual tag as metadata is
      * to increment the corresponding protocol type tag counter in stats structure. The
      * packet type to metadata lookup is faster and easier than metadata to packet type
      * lookup.
      */
      if (ic->ic_pdev_obj->standby_active) {
           param.meta_data = rx_pkt_protocol_tag_info->pkt_type_metadata;
           update_protocol_mask = true;
      } else {
           param.meta_data = rx_pkt_protocol_tag_info->pkt_type + RX_PROTOCOL_TAG_START_OFFSET;
      }

      /* Update the driver protocol tag mask. This maintains bitmask of protocols
       * for which protocol tagging is enabled */
      if (update_protocol_mask)
          ic->rx_pkt_protocol_tag_mask |= (1 << rx_pkt_protocol_tag_info->pkt_type);
    }
    else
    {
      if (!(ic->rx_pkt_protocol_tag_mask & (1 << rx_pkt_protocol_tag_info->pkt_type)))
      {
        qdf_err("Unable to delete RX packet type TAG: %d", rx_pkt_protocol_tag_info->pkt_type);
        return 0;
      }

      param.op_code = DEL_PKT_ROUTING;
      /* Disable the protocol in bitmask */
      if (update_protocol_mask)
          ic->rx_pkt_protocol_tag_mask &= ~(1 << rx_pkt_protocol_tag_info->pkt_type);
      rx_pkt_protocol_tag_info->pkt_type_metadata = 0;
    }

    /* Save the user provided metadata in CDP layer. This value will be looked up by
     * CDP layer for tagging the same onto QDF packet. */
    cdp_update_pdev_rx_protocol_tag(soc_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
              ic->rx_pkt_protocol_tag_mask, rx_pkt_protocol_tag_info->pkt_type,
              rx_pkt_protocol_tag_info->pkt_type_metadata);

    /* Generate the protocol bitmap based on the packet type provided */
    switch (rx_pkt_protocol_tag_info->pkt_type)
    {
      case RECV_PKT_TYPE_ARP:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_ARP_IPV4);
        break;
      case RECV_PKT_TYPE_NS:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_NS_IPV6);
        break;
      case RECV_PKT_TYPE_IGMP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_IGMP_IPV4);
        break;
      case RECV_PKT_TYPE_MLD_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_MLD_IPV6);
        break;
      case RECV_PKT_TYPE_DHCP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_DHCP_IPV4);
        break;
      case RECV_PKT_TYPE_DHCP_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_DHCP_IPV6);
        break;
      case RECV_PKT_TYPE_DNS_TCP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_DNS_TCP_IPV4);
        break;
      case RECV_PKT_TYPE_DNS_TCP_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_DNS_TCP_IPV6);
        break;
      case RECV_PKT_TYPE_DNS_UDP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_DNS_UDP_IPV4);
        break;
      case RECV_PKT_TYPE_DNS_UDP_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_DNS_UDP_IPV6);
        break;
      case RECV_PKT_TYPE_ICMP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_ICMP_IPV4);
        break;
      case RECV_PKT_TYPE_ICMP_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_ICMP_IPV6);
        break;
      case RECV_PKT_TYPE_TCP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_TCP_IPV4);
        break;
      case RECV_PKT_TYPE_TCP_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_TCP_IPV6);
        break;
      case RECV_PKT_TYPE_UDP_V4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_UDP_IPV4);
        break;
      case RECV_PKT_TYPE_UDP_V6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_UDP_IPV6);
        break;
      case RECV_PKT_TYPE_IPV4:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_IPV4);
        break;
      case RECV_PKT_TYPE_IPV6:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_IPV6);
        break;
      case RECV_PKT_TYPE_EAP:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_EAP);
        break;
      case RECV_PKT_TYPE_VLAN:
        param.routing_type_bitmap = (1 << PDEV_PKT_TYPE_VLAN);
        break;
      default:
        qdf_err("Invalid packet type: %u", rx_pkt_protocol_tag_info->pkt_type);
        return -1;
    }

    qdf_debug("Set RX packet type TAG, opcode : %d, pkt_type : %d, metadata : 0x%x,"
                  "pdev_id = %u, REO dest ring = %d\n",
                  rx_pkt_protocol_tag_info->op_code, rx_pkt_protocol_tag_info->pkt_type,
                  rx_pkt_protocol_tag_info->pkt_type_metadata, param.pdev_id, param.dest_ring);

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    return wmi_unified_set_rx_pkt_type_routing_tag(pdev_wmi_handle, &param);
}

#ifdef WLAN_SUPPORT_RX_TAG_STATISTICS
int ol_ath_ucfg_dump_rx_pkt_protocol_tag_stats(void *vscn, uint32_t protocol_type)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    ol_txrx_soc_handle soc_handle;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    uint32_t nss_soc_cfg = cfg_get(scn->soc->psoc_obj, CFG_NSS_WIFI_OL);

    if (nss_soc_cfg)
    {
      qdf_info("RX Protocol Tag not supported when NSS offload is enabled");
      return 0;
    }
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

    soc_handle = (ol_txrx_soc_handle) wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_handle) {
        qdf_err("psoc handle is NULL");
        return -EFAULT;
    }

    cdp_dump_pdev_rx_protocol_tag_stats(soc_handle,
                  wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev), protocol_type);

    return 0;
}
#endif /* WLAN_SUPPORT_RX_TAG_STATISTICS */
#endif /* WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG */

#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
int ol_ath_ucfg_set_tx_pkt_cap_custom_classify(void *vscn,
                          struct ieee80211_tx_pkt_cap_params *tx_cap_info)
{
    struct wmi_tx_pkt_cap_custom_classify_info param;
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wmi_unified *pdev_wmi_handle;

    param.pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    param.pkt_type_bitmap = tx_cap_info->pkt_type_bitmap;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is null");
        return -EINVAL;
    }

    qdf_debug("Set TX packet cap classify, pdev_id: %u, bitmap: %u",
              param.pdev_id, param.pkt_type_bitmap);

    return wmi_unified_set_tx_pkt_cap_custom_classify(pdev_wmi_handle, &param);
}
#endif /* WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY */

#ifdef WLAN_SUPPORT_RX_FLOW_TAG
int ol_ath_ucfg_rx_flow_tag_op(void *vscn,  struct ieee80211_rx_flow_tag *rx_flow_tag_info)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    ol_txrx_soc_handle soc_handle;
    struct cdp_rx_flow_info dp_flow_info;
    QDF_STATUS status = 0;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    uint32_t nss_soc_cfg = cfg_get(scn->soc->psoc_obj, CFG_NSS_WIFI_OL);

    if (nss_soc_cfg) {
        qdf_info("RX FlowTag not supported when NSS offload is enabled");
        return 0;
    }
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

    soc_handle = (ol_txrx_soc_handle) wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_handle) {
        return -EFAULT;
    }

    memset(&dp_flow_info, 0, sizeof(struct cdp_rx_flow_info));

    if (IP_VER_4 == rx_flow_tag_info->ip_ver)
        dp_flow_info.is_addr_ipv4 = true;
    else
        dp_flow_info.is_addr_ipv4 = false;

    if (rx_flow_tag_info->op_code == RX_FLOW_TAG_OPCODE_ADD) {
        dp_flow_info.fse_metadata = rx_flow_tag_info->flow_metadata;
        dp_flow_info.op_code = CDP_FLOW_FST_ENTRY_ADD;
    } else if (rx_flow_tag_info->op_code == RX_FLOW_TAG_OPCODE_DEL) {
        dp_flow_info.fse_metadata = 0;
        dp_flow_info.op_code = CDP_FLOW_FST_ENTRY_DEL;
    } else if (rx_flow_tag_info->op_code == RX_FLOW_TAG_OPCODE_DUMP_STATS) {
        dp_flow_info.fse_metadata = 0;
    }

    if (L4_PROTOCOL_TYPE_TCP == rx_flow_tag_info->flow_tuple.protocol)
        dp_flow_info.flow_tuple_info.l4_protocol = CDP_FLOW_PROTOCOL_TYPE_TCP;
    else if (L4_PROTOCOL_TYPE_UDP == rx_flow_tag_info->flow_tuple.protocol)
        dp_flow_info.flow_tuple_info.l4_protocol = CDP_FLOW_PROTOCOL_TYPE_UDP;

    if (IP_VER_4 == rx_flow_tag_info->ip_ver) {
        dp_flow_info.flow_tuple_info.dest_ip_31_0 = rx_flow_tag_info->flow_tuple.dest_ip[3];
        dp_flow_info.flow_tuple_info.src_ip_31_0 = rx_flow_tag_info->flow_tuple.source_ip[3];
    } else {
        dp_flow_info.flow_tuple_info.dest_ip_127_96 = rx_flow_tag_info->flow_tuple.dest_ip[0];
        dp_flow_info.flow_tuple_info.dest_ip_95_64 = rx_flow_tag_info->flow_tuple.dest_ip[1];
        dp_flow_info.flow_tuple_info.dest_ip_63_32 = rx_flow_tag_info->flow_tuple.dest_ip[2];
        dp_flow_info.flow_tuple_info.dest_ip_31_0 = rx_flow_tag_info->flow_tuple.dest_ip[3];

        dp_flow_info.flow_tuple_info.src_ip_127_96 = rx_flow_tag_info->flow_tuple.source_ip[0];
        dp_flow_info.flow_tuple_info.src_ip_95_64 = rx_flow_tag_info->flow_tuple.source_ip[1];
        dp_flow_info.flow_tuple_info.src_ip_63_32 = rx_flow_tag_info->flow_tuple.source_ip[2];
        dp_flow_info.flow_tuple_info.src_ip_31_0 = rx_flow_tag_info->flow_tuple.source_ip[3];
    }

    dp_flow_info.flow_tuple_info.dest_port = rx_flow_tag_info->flow_tuple.dest_port;
    dp_flow_info.flow_tuple_info.src_port = rx_flow_tag_info->flow_tuple.source_port;


    dp_flow_info.svc_id = rx_flow_tag_info->flow_info.svc_id;
    dp_flow_info.tid = rx_flow_tag_info->flow_info.tid;
    dp_flow_info.dest_mac = (uint8_t *)&rx_flow_tag_info->flow_info.dstmac;
    dp_flow_info.drop = rx_flow_tag_info->flow_info.drop_flow;
    dp_flow_info.drop = rx_flow_tag_info->flow_info.ring_id;

    qdf_debug("Flow Tag - Opcode: %d, v4: %d, Metadata: %u", dp_flow_info.op_code,
                                 dp_flow_info.is_addr_ipv4, dp_flow_info.fse_metadata);

    qdf_debug("Dest IP address %x:%x:%x:%x",
        dp_flow_info.flow_tuple_info.dest_ip_127_96,
        dp_flow_info.flow_tuple_info.dest_ip_95_64,
        dp_flow_info.flow_tuple_info.dest_ip_63_32,
        dp_flow_info.flow_tuple_info.dest_ip_31_0);
    qdf_debug("Source IP address %x:%x:%x:%x",
        dp_flow_info.flow_tuple_info.src_ip_127_96,
        dp_flow_info.flow_tuple_info.src_ip_95_64,
        dp_flow_info.flow_tuple_info.src_ip_63_32,
        dp_flow_info.flow_tuple_info.src_ip_31_0);
    qdf_debug("Dest port %u, Src Port %u, Protocol %u",
        dp_flow_info.flow_tuple_info.dest_port,
        dp_flow_info.flow_tuple_info.src_port,
        dp_flow_info.flow_tuple_info.l4_protocol);

    if (rx_flow_tag_info->op_code == RX_FLOW_TAG_OPCODE_ADD ||
        rx_flow_tag_info->op_code == RX_FLOW_TAG_OPCODE_DEL)
        status = cdp_set_rx_flow_tag(soc_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev), &dp_flow_info);
    else if (rx_flow_tag_info->op_code == RX_FLOW_TAG_OPCODE_DUMP_STATS)
        status = cdp_dump_rx_flow_tag_stats(soc_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev), &dp_flow_info);

    if (QDF_STATUS_SUCCESS != status) {
        qdf_err("Flow Tag opcode request %d failed, error_code = %u",
                         rx_flow_tag_info->op_code, status);
        return -EFAULT;
    }
    return 0;
}
#endif //WLAN_SUPPORT_RX_FLOW_TAG

int
ol_ath_ucfg_set_nav_override_config(void *vscn, uint8_t value,
                                        uint32_t threshold)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    uint32_t config_val = 0;

    if(value > IEEE80211_NAV_CTRL_STATUS_MAX) {
        qdf_err("%s: Update value should be less than %d", __func__, IEEE80211_NAV_CTRL_STATUS_MAX);
        return -EINVAL;
    }

    config_val = ((threshold << IEEE80211_NAV_CTRL_STATUS_BITS) | value);

    if (ol_ath_pdev_set_param(scn->sc_pdev, wmi_pdev_param_nav_override_config,
                              config_val) != EOK) {
        qdf_err("NAV config value %d could not be set", config_val);
        return -EINVAL;
    } else {
        ic->ic_nav_override_config = config_val;
    }
    return 0;
}

int
ol_ath_ucfg_get_nav_override_config(void *vscn, int *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;

    value[0] = ic->ic_nav_override_config & IEEE80211_NAV_CTRL_STATUS_MASK;
    value[1] = ic->ic_nav_override_config >> IEEE80211_NAV_CTRL_STATUS_BITS;

    return 0;
}

#if OBSS_PD
int ol_ath_ucfg_set_he_sr_config(void *vscn,
                                 uint8_t param, uint8_t value, uint8_t data1, uint8_t data2)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;
    bool sr_ie_len_update             = false;
    uint8_t min_offset, max_offset;
    struct he_srp_ie_config config;

    if (!ic) {
        qdf_err("IC is null");
        return -EINVAL;
    }

    if (!ic->ic_he_sr_enable) {
        qdf_err("Spatial Reuse Parameter Set Element is not enabled on this radio");
        return -EINVAL;
    }

    switch (param) {

        case HE_SR_PSR_ENABLE:
            if (value > 1) {
                qdf_err("%s: SR ctrl PSR_disallowed field can either be 0 or 1\n",
                        __func__);
                return -EINVAL;
            }

            /* Skip updating the IE as the current value matches user
             * request. Note that this ic variable holds a 'NOT' of
             * user-requested value.
             */
            if(value == ic->ic_he_srctrl_psr_disallowed) {
                ic->ic_is_spatial_reuse_updated = true;
                ic->ic_he_srctrl_psr_disallowed = !value;
            } else {
                qdf_debug("PSR Enabled already set to: %d", value);
            }
        break;

        case HE_SR_NON_SRG_OBSSPD_ENABLE:
            /*
             * This command configures the following fields in IC
             * 1. Non-SRG OBSS PD SR Disallowed
             * 2. Non-SRG OBSS PD Max offset
             */
            if (value > 1) {
                qdf_err("%s: SR ctrl non_srg_obss_pd_disallowed field can"
                        "  either be 0 or 1\n",
                        __func__);
                return -EINVAL;
            }

            max_offset = data1;

            if(max_offset > HE_SR_NON_SRG_OBSS_PD_MAX_THRESH_OFFSET_VAL) {
                qdf_err("%s: Max OBSS PD Threshold Offset value must be"
                " <= %u", __func__, HE_SR_NON_SRG_OBSS_PD_MAX_THRESH_OFFSET_VAL);
                return -EINVAL;
            }

            /* Skip updating the IE as the current value matches user
             * request. Note that this ic variable holds a 'NOT' of
             * user-requested value.
             */
            if (value == ic->ic_he_srctrl_non_srg_obsspd_disallowed) {
                sr_ie_len_update = true;
                ic->ic_he_srctrl_non_srg_obsspd_disallowed = !value;
            }

            if (value) {
                if(max_offset != ic->ic_he_non_srg_obsspd_max_offset) {
                    if (!sr_ie_len_update) {
                            ic->ic_is_spatial_reuse_updated = true;
                    }
                    ic->ic_he_non_srg_obsspd_max_offset = max_offset;
                }
            } else {
                qdf_debug("As Non-SRG OBSS PD Enable is being set to 0, forcing Non-SRG OBSS PD max offset to 0");
                ic->ic_he_non_srg_obsspd_max_offset = 0;
            }
        break;

        case HE_SR_SR15_ENABLE:
            if (value > 1) {
                qdf_err("%s: SR ctrl SR15_allowed field can either be 0 or 1\n",
                        __func__);
                return -EINVAL;
            }

            if(value != ic->ic_he_srctrl_sr15_allowed) {
                ic->ic_is_spatial_reuse_updated = true;
                ic->ic_he_srctrl_sr15_allowed = value;
            } else {
                qdf_debug("SR15 Allowed Value already set to: %d", value);
            }

        break;

        case HE_SR_SRG_OBSSPD_ENABLE:
            /*
             * This command configures the following 3 fields in IC
             * 1. SRG information present
             * 2. SRG OBSS PD Min offset
             * 3. SRG OBSS PD Max offset
             */
            if (value > 1) {
                qdf_err("%s: SRG INFO PRESENT field can either be 0 or 1\n",
                        __func__);
                return -EINVAL;
            }

            min_offset = data1;
            max_offset = data2;

            if (min_offset > max_offset) {
                qdf_err("SRG OBSS PD min offset(%d) must be <= SRG OBSS"
                        "PD max offset(%d)", min_offset, max_offset);
                return -EINVAL;
            }

            if (min_offset > HE_SR_SRG_OBSS_PD_MAX_ALLOWED_OFFSET_VAL) {
                qdf_err("SRG OBSS PD min offset value must be <= maximum allowed offset(%d)",
                        HE_SR_SRG_OBSS_PD_MAX_ALLOWED_OFFSET_VAL);
                return -EINVAL;
            }

            if (max_offset > HE_SR_SRG_OBSS_PD_MAX_ALLOWED_OFFSET_VAL) {
                qdf_err("SRG OBSS PD max offset value must be <= maximum allowed offset(%d)",
                        HE_SR_SRG_OBSS_PD_MAX_ALLOWED_OFFSET_VAL);
                return -EINVAL;
            }

            if (value != ic->ic_he_srctrl_srg_info_present) {
                sr_ie_len_update = true;
                ic->ic_he_srctrl_srg_info_present = value;
            }

            if (value) {
                /* SRG OBSS PD Min offset */
                if(min_offset != ic->ic_he_srctrl_srg_obsspd_min_offset) {
                    ic->ic_is_spatial_reuse_updated = true;
                    ic->ic_he_srctrl_srg_obsspd_min_offset = min_offset;
                }

                /* SRG OBSS PD Max offset */
                if(max_offset != ic->ic_he_srctrl_srg_obsspd_max_offset) {
                    ic->ic_is_spatial_reuse_updated = true;
                    ic->ic_he_srctrl_srg_obsspd_max_offset = max_offset;
                }
            } else {
                qdf_debug("As SRG OBSS PD Enable is being set to 0, forcing SRG OBSS PD min and max offsets to 0");
                ic->ic_he_srctrl_srg_obsspd_min_offset = 0;
                ic->ic_he_srctrl_srg_obsspd_max_offset = 0;
            }
            break;

        default:
            qdf_info("Unhandled SR config command");
            return -EINVAL;
    }

    config.param = param;
    config.value = value;
    config.data1 = data1;
    config.data2 = data2;

    /* Copy the radio level configuration to all the VAPs.
       Each VAP will take care of updatng IE in its management frames */
    if (ic->ic_vap_set_he_sr_config)
        wlan_iterate_vap_list(ic, ol_ath_iter_vap_set_he_sr_config,
                              (void *)&config);

    return 0;
}

int ol_ath_ucfg_get_he_sr_config(void *vscn, uint8_t param, uint32_t *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;

    if (!ic) {
        qdf_err("IC is null");
        return -EINVAL;
    }

    if (!ic->ic_he_sr_enable) {
        qdf_err("Spatial Reuse Parameter Set Element is not enabled on this radio");
        return -EINVAL;
    }

    switch (param) {

        case HE_SR_PSR_ENABLE:
            value[0] = !ic->ic_he_srctrl_psr_disallowed;
        break;

        case HE_SR_NON_SRG_OBSSPD_ENABLE:
            value[0] = !ic->ic_he_srctrl_non_srg_obsspd_disallowed;
            value[1] = ic->ic_he_non_srg_obsspd_max_offset;
        break;

        case HE_SR_SR15_ENABLE:
            value[0] = ic->ic_he_srctrl_sr15_allowed;
        break;

        case HE_SR_SRG_OBSSPD_ENABLE:
            value[0] = ic->ic_he_srctrl_srg_info_present;
            value[1] = ic->ic_he_srctrl_srg_obsspd_min_offset;
            value[2] = ic->ic_he_srctrl_srg_obsspd_max_offset;
        break;

        default:
            qdf_info("Unhandled SR config command");
            return -EINVAL;
    }

    return 0;
}

int ol_ath_ucfg_set_sr_self_config(void *vscn, uint32_t param,
	void *data, uint32_t data_len, uint32_t value, uint32_t flags)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;
    int retval =    0;
    int8_t pdev_id;
    struct wmi_unified *pdev_wmi_handle;

    pdev_id = lmac_get_pdev_idx(scn->sc_pdev);
    if(pdev_id < 0)
            return -1;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle)
	    return -1;

    switch (param)
    {
        case SR_SELF_OBSS_PD_TX_ENABLE:
            retval = ol_ath_set_obss_pd_enable_bit(
                ic, value, scn, ol_ath_extact_sr_type(data, data_len));
            break;

        case SR_SELF_OBSS_PD_THRESHOLD_DB:
            if (ic->self_srg_psr_support) {
                qdf_err("dB units are not supported for OBSS PD threshold on this radio, try the dBm command");
                return -EOPNOTSUPP;
            }

            retval = ol_ath_set_obss_pd_threshold(
                ic, value, scn, ol_ath_extact_sr_type(data, data_len));
            break;

        case SR_SELF_OBSS_PD_THRESHOLD_DBM:
            if (!ic->self_srg_psr_support) {
                qdf_err("dBm units are not supported for OBSS PD threshold on this radio, try the dB command");
                return -EOPNOTSUPP;
            }

            retval = ol_ath_set_obss_pd_threshold(
                ic, value, scn, ol_ath_extact_sr_type(data, data_len));
            break;

        case SR_SELF_SRG_BSS_COLOR_BITMAP:
            retval = ol_ath_set_self_srg_bss_color_bitmap(
                        ic, value, *(uint32_t*)data);
            break;

        case SR_SELF_SRG_PARTIAL_BSSID_BITMAP:
            retval = ol_ath_set_self_srg_partial_bssid_bitmap(
                        ic, value, *(uint32_t*)data);
            break;

        case SR_SELF_ENABLE_PER_AC:
            retval = ol_ath_set_sr_per_ac(ic, value,
                                        ol_ath_extact_sr_type(data, data_len));
            break;

        case SR_SELF_HESIGA_SR15_ENABLE:
            retval = ol_ath_set_self_hesiga_sr15_enable(ic, *(uint8_t*)data);
            break;

        case SR_SELF_SRG_OBSS_COLOR_ENABLE_BITMAP:
            if (!ic->self_srg_psr_support) {
                qdf_err("SRG based Spatial Reuse is not supported on this target");
                return -EINVAL;
            }

            /* Send WMI */
            retval = wmi_unified_send_self_srg_obss_color_enable_bitmap_cmd(
                        pdev_wmi_handle, value, *(uint32_t*)data, pdev_id);

            /* Configure IC if WMI is success */
            if(retval) {
                qdf_err("WMI send failed, discarding the configuration");
            } else {
                ic->ic_srg_obss_color_enable_bitmap[0] = value;
                ic->ic_srg_obss_color_enable_bitmap[1] = *(uint32_t*)data;
            }
            break;

        case SR_SELF_SRG_OBSS_BSSID_ENABLE_BITMAP:
            if (!ic->self_srg_psr_support) {
                qdf_err("SRG based Spatial Reuse is not supported on this target");
                return -EINVAL;
            }

            /* Send WMI */
            retval = wmi_unified_send_self_srg_obss_bssid_enable_bitmap_cmd(
                        pdev_wmi_handle, value, *(uint32_t*)data, pdev_id);

            /* Configure IC if WMI is success */
            if(retval) {
                qdf_err("WMI send failed, discarding the configuration");
            } else {
                ic->ic_srg_obss_bssid_enable_bitmap[0] = value;
                ic->ic_srg_obss_bssid_enable_bitmap[1] = *(uint32_t*)data;
            }
            break;

        case SR_SELF_NON_SRG_OBSS_COLOR_ENABLE_BITMAP:
            if (!ic->self_srg_psr_support) {
                qdf_err("SRG based Spatial Reuse is not supported on this target");
                return -EINVAL;
            }

            /* Send WMI */
            retval = wmi_unified_send_self_non_srg_obss_color_enable_bitmap_cmd(
                        pdev_wmi_handle, value, *(uint32_t*)data, pdev_id);

            /* Configure IC if WMI is success */
            if(retval) {
                qdf_err("WMI send failed, discarding the configuration");
            } else {
                ic->ic_non_srg_obss_color_enable_bitmap[0] = value;
                ic->ic_non_srg_obss_color_enable_bitmap[1] = *(uint32_t*)data;
            }
            break;

        case SR_SELF_NON_SRG_OBSS_BSSID_ENABLE_BITMAP:
            if (!ic->self_srg_psr_support) {
                qdf_err("SRG based Spatial Reuse is not supported on this target");
                return -EINVAL;
            }

            /* Send WMI */
            retval = wmi_unified_send_self_non_srg_obss_bssid_enable_bitmap_cmd(
                        pdev_wmi_handle, value, *(uint32_t*)data, pdev_id);

            /* Configure IC if WMI is success */
            if(retval) {
                qdf_err("WMI send failed, discarding the configuration");
            } else {
                ic->ic_non_srg_obss_bssid_enable_bitmap[0] = value;
                ic->ic_non_srg_obss_bssid_enable_bitmap[1] = *(uint32_t*)data;
            }
            break;

        case SR_SELF_PSR_TX_ENABLE:
            retval = ol_ath_set_self_psr_tx_enable(ic, *(uint8_t*)data);
            break;

        case SR_SELF_SAFETY_MARGIN_PSR:
            retval = ol_ath_set_self_safety_margin_psr(ic, *(uint8_t*)data);
            break;

        case SR_SELF_OBSS_PD_CTRL_FRAME:
            retval = ol_ath_configure_obss_pd_ctrl_frame(
                ic, value, flags, scn, ol_ath_extact_sr_type(data, data_len));
            break;

        case SR_SELF_ENABLE_TRUNCATION:
            retval = ol_ath_set_self_sr_enable_truncation(ic, *(uint8_t*)data);
            break;

        case SR_SELF_OBSS_PPDU_MIN_DUR:
            retval = ol_ath_set_self_sr_obss_ppdu_min_dur(ic, *(uint32_t*)data);
            break;

        default:
            qdf_info("Unhandled SR config command");
            return -EINVAL;
    }

    return retval;
}

int ol_ath_ucfg_get_sr_self_config(void *vscn, uint8_t param, char value[], size_t length)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic           = &scn->sc_ic;
    uint8_t srg_val = 0, non_srg_val = 0;
    uint8_t srg_enable = 0, non_srg_enable = 0;
    uint8_t enable_per_ac_obss_pd = 0, enable_per_ac_psr = 0;

    switch (param) {
        case SR_SELF_OBSS_PD_TX_ENABLE:
            if (get_obss_pd_enable_bit(ic->ic_ap_obss_pd_thresh,
                                       SR_TYPE_NON_SRG_OBSS_PD, &non_srg_enable))
                return -EINVAL;

            if (get_obss_pd_enable_bit(ic->ic_ap_obss_pd_thresh,
                                       SR_TYPE_SRG_OBSS_PD, &srg_enable))
                return -EINVAL;

            snprintf(value, length, " non-srg: %d, srg: %d",
                     non_srg_enable, srg_enable);
            break;

        case SR_SELF_OBSS_PD_THRESHOLD_DB:
            if (ic->self_srg_psr_support) {
                qdf_err("dB units are not supported for OBSS PD threshold on this radio, try the dBm command");
                return -EOPNOTSUPP;
            }

            if (get_obss_pd_threshold(ic->ic_ap_obss_pd_thresh,
                                      SR_TYPE_NON_SRG_OBSS_PD, &non_srg_val))
                return -EINVAL;

            if (get_obss_pd_threshold(ic->ic_ap_obss_pd_thresh,
                                      SR_TYPE_SRG_OBSS_PD, &srg_val))
                return -EINVAL;


            snprintf(value, length, " non-srg: %d, srg: %d",
                     (int8_t)non_srg_val, (int8_t)srg_val);
            break;

        case SR_SELF_OBSS_PD_THRESHOLD_DBM:
            if (!ic->self_srg_psr_support) {
                qdf_err("dBm units are not supported for OBSS PD threshold on this radio, try the dB command");
                return -EOPNOTSUPP;
            }

            if (get_obss_pd_threshold(ic->ic_ap_obss_pd_thresh,
                                      SR_TYPE_NON_SRG_OBSS_PD, &non_srg_val))
                return -EINVAL;

            if (get_obss_pd_threshold(ic->ic_ap_obss_pd_thresh,
                                      SR_TYPE_SRG_OBSS_PD, &srg_val))
                return -EINVAL;


            snprintf(value, length, " non-srg: %d, srg: %d",
                     (int8_t)non_srg_val, (int8_t)srg_val);
            break;

        case SR_SELF_SRG_BSS_COLOR_BITMAP:
            snprintf(value, length, "0x%08X %08X",
                     ic->ic_srg_bss_color_bitmap[1],
                     ic->ic_srg_bss_color_bitmap[0]);
            break;

        case SR_SELF_SRG_PARTIAL_BSSID_BITMAP:
            snprintf(value, length, "0x%08X %08X",
                     ic->ic_srg_partial_bssid_bitmap[1],
                     ic->ic_srg_partial_bssid_bitmap[0]);
            break;

        case SR_SELF_ENABLE_PER_AC:
            if (get_sr_per_ac(ic->ic_he_sr_enable_per_ac, SR_TYPE_OBSS_PD,
                              &enable_per_ac_obss_pd))
                return -EINVAL;

            if (get_sr_per_ac(ic->ic_he_sr_enable_per_ac, SR_TYPE_PSR,
                              &enable_per_ac_psr))
                return -EINVAL;

            snprintf(value, length, " obss_pd: 0x%X psr: 0x%X",
                     enable_per_ac_obss_pd, enable_per_ac_psr);
            break;

        case SR_SELF_HESIGA_SR15_ENABLE:
            snprintf(value, length, "%d", ic->ic_hesiga_sr15_enable);
            break;

        case SR_SELF_SRG_OBSS_COLOR_ENABLE_BITMAP:
            snprintf(value, length, "0x%08X %08X",
                     ic->ic_srg_obss_color_enable_bitmap[1],
                     ic->ic_srg_obss_color_enable_bitmap[0]);
            break;

        case SR_SELF_SRG_OBSS_BSSID_ENABLE_BITMAP:
            snprintf(value, length, "0x%08X %08X",
                     ic->ic_srg_obss_bssid_enable_bitmap[1],
                     ic->ic_srg_obss_bssid_enable_bitmap[0]);
            break;

        case SR_SELF_NON_SRG_OBSS_COLOR_ENABLE_BITMAP:
            snprintf(value, length, "0x%08X %08X",
                     ic->ic_non_srg_obss_color_enable_bitmap[1],
                     ic->ic_non_srg_obss_color_enable_bitmap[0]);
            break;

        case SR_SELF_NON_SRG_OBSS_BSSID_ENABLE_BITMAP:
            snprintf(value, length, "0x%08X %08X",
                     ic->ic_non_srg_obss_bssid_enable_bitmap[1],
                     ic->ic_non_srg_obss_bssid_enable_bitmap[0]);
            break;

        case SR_SELF_PSR_TX_ENABLE:
            snprintf(value, length, "%d", ic->ic_psr_tx_enable);
            break;

        case SR_SELF_SAFETY_MARGIN_PSR:
            snprintf(value, length, "%d", ic->ic_safety_margin_psr);
            break;

        case SR_SELF_ENABLE_TRUNCATION:
            snprintf(value, length, "%d", ic->ic_enable_sr_truncate);
            break;

        case SR_SELF_OBSS_PPDU_MIN_DUR:
            snprintf(value, length, "%u", ic->ic_min_obss_ppdu_dur_sr);
            break;

        case SR_SELF_OBSS_PD_CTRL_FRAME:
            if (get_obss_pd_enable_bit(ic->ic_ap_obss_pd_thresh_ctrl_frame,
                                       SR_TYPE_NON_SRG_OBSS_PD, &non_srg_enable))
                return -EINVAL;

            if (get_obss_pd_threshold(ic->ic_ap_obss_pd_thresh_ctrl_frame,
                                      SR_TYPE_NON_SRG_OBSS_PD, &non_srg_val))
                return -EINVAL;

            if (get_obss_pd_enable_bit(ic->ic_ap_obss_pd_thresh_ctrl_frame,
                                       SR_TYPE_SRG_OBSS_PD, &srg_enable))
                return -EINVAL;

            if (get_obss_pd_threshold(ic->ic_ap_obss_pd_thresh_ctrl_frame,
                                      SR_TYPE_SRG_OBSS_PD, &srg_val))
                return -EINVAL;

            snprintf(value, length,
                     " non-srg:{enable:%d threshold:%d},"
                     " srg:{enable:%d threshold:%d}",
                     non_srg_enable, (int8_t)non_srg_val,
                     srg_enable, (int8_t)srg_val);
            break;

        default:
            qdf_info("Unhandled SR config command");
            return -EINVAL;
    }

    return 0;
}
#endif /* OBSS PD */

int ol_ath_ucfg_set_muedca_mode(void *vscn, uint8_t mode)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;

    ic->ic_muedca_mode_state = mode;

    switch (mode) {

        case HEMUEDCA_MANUAL_MODE:
        case HEMUEDCA_HOST_DYNAMIC_MODE:
            /* Send mode 0 to FW to switch off dynamic selection */
            mode = FW_MUEDCA_DYNAMIC_MODE_DISABLE;
            break;
        case HEMUEDCA_FW_DYNAMIC_MODE:
            /* Send mode 1 to FW to switch on FW dynamic selection */
            mode = FW_MUEDCA_DYNAMIC_MODE_ENABLE;
            break;
        default:
            qdf_err("Invalid parameter");
        return -EINVAL;
    }
    if (ol_ath_pdev_set_param(scn->sc_pdev,
                              wmi_pdev_param_enable_fw_dynamic_he_edca,
                              mode)) {
          qdf_err("Error sending WMI for EDCA FW dynamic mode");
          return -EINVAL;
    }
    return EOK;
}

int ol_ath_ucfg_get_muedca_mode(void *vscn, int *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;

    *value = ic->ic_muedca_mode_state;

    return EOK;
}

int ol_ath_ucfg_set_non_ht_dup(void *vscn, uint8_t frametype, bool enable)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    struct ieee80211com *ic = NULL;
    struct ieee80211vap *vap = NULL;
    int ret = 0;
    uint8_t ic_non_ht_dup;

    if(vscn == NULL) {
        qdf_err("SCN is NULL");
        return -EINVAL;
    }

    scn = (struct ol_ath_softc_net80211 *)vscn;
    ic = &scn->sc_ic;
    ic_non_ht_dup = ic->ic_non_ht_dup;

    switch(frametype) {

        case NON_HT_DUP_BEACON:
            ic_non_ht_dup &= ~IEEE80211_NON_HT_DUP_BEACON_M;
            ic_non_ht_dup |= (enable << IEEE80211_NON_HT_DUP_BEACON_S);
            break;
        case NON_HT_DUP_BCAST_PROBE_RESP:
            ic_non_ht_dup &= ~IEEE80211_NON_HT_DUP_BCAST_PROBE_RESP_M;
            ic_non_ht_dup |= (enable << IEEE80211_NON_HT_DUP_BCAST_PROBE_RESP_S);
            break;
        case NON_HT_DUP_FILS_DISCOVERY:
            ic_non_ht_dup &= ~IEEE80211_NON_HT_DUP_FILS_DISCOVERY_M;
            ic_non_ht_dup |= (enable << IEEE80211_NON_HT_DUP_FILS_DISCOVERY_S);
            break;
        default:
            qdf_err("Invalid input");
            return -EINVAL;
    }

    if (ic_non_ht_dup == ic->ic_non_ht_dup) {
       return ret;
    }

    if (ic->is_mbssid_enabled) {
        struct ieee80211vap *txvap;
        struct mbssid_context *mbss_ctx;
        struct mbssid_info *ic_mbss;
        uint8_t gp_id;
        ic->ic_non_ht_dup = ic_non_ht_dup;
        if (!ic->ic_mbss_ctx) {
            mbss_err("mbss_ctx is null");
            return -EINVAL;
        }
        mbss_ctx = ic->ic_mbss_ctx;
        /* Update txvap of all groups */
        for (gp_id = 0; gp_id < mbss_ctx->max_ngroups; gp_id++) {
            ic_mbss = &mbss_ctx->ic_mbss[gp_id];
            txvap = ic_mbss->transmit_vap;

            if (txvap && (wlan_vdev_is_up(txvap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                ret = ic->ic_vap_set_param(txvap,
                        IEEE80211_CONFIG_6GHZ_NON_HT_DUP, ic->ic_non_ht_dup);
                if (ret < 0) {
                    qdf_err("WMI send failed");
                    return -EINVAL;
                } else {
                    wlan_vdev_beacon_update(txvap);
                    /* Update the user config flag */
                    ic->ic_non_ht_dup_user_config = 1;
                }
            }
        }
    } else {
       if (ic->ic_vap_set_param) {
          ic->ic_non_ht_dup = ic_non_ht_dup;
          TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
             if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                ret = ic->ic_vap_set_param(vap,
                            IEEE80211_CONFIG_6GHZ_NON_HT_DUP,
                            ic->ic_non_ht_dup);
                if (ret < 0) {
                    qdf_err("WMI send failed");
                    return -EINVAL;
                } else {
                    wlan_vdev_beacon_update(vap);
                    /* Update the user config flag */
                    ic->ic_non_ht_dup_user_config = 1;
                }
             }
          }
       }
    }

    return ret;
}

int ol_ath_ucfg_get_non_ht_dup(void *vscn, uint8_t frametype, uint8_t *value)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    struct ieee80211com *ic = NULL;

    if(vscn == NULL) {
        qdf_err("SCN is NULL");
        return -EINVAL;
    }

    scn = (struct ol_ath_softc_net80211 *)vscn;
    ic = &scn->sc_ic;

    switch(frametype) {

        case NON_HT_DUP_BEACON:
            *value = ((ic->ic_non_ht_dup & IEEE80211_NON_HT_DUP_BEACON_M) >>
                                           IEEE80211_NON_HT_DUP_BEACON_S);
            break;
        case NON_HT_DUP_BCAST_PROBE_RESP:
            *value = ((ic->ic_non_ht_dup &
                        IEEE80211_NON_HT_DUP_BCAST_PROBE_RESP_M) >>
                        IEEE80211_NON_HT_DUP_BCAST_PROBE_RESP_S);
            break;
        case NON_HT_DUP_FILS_DISCOVERY:
            *value = ((ic->ic_non_ht_dup &
                        IEEE80211_NON_HT_DUP_FILS_DISCOVERY_M) >>
                        IEEE80211_NON_HT_DUP_FILS_DISCOVERY_S);
            break;
        default:
            qdf_err("Invalid input");
            return -EINVAL;
    }

    return EOK;
}

int ol_ath_ucfg_set_col_6ghz_rnr(void *vscn, uint8_t usr_sel, uint8_t frm_val)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    struct ieee80211vap *vap;

    switch (usr_sel) {

        case RNR_6GHZ_EN:
            /* Set the frame type bits and set usr mode */
            WLAN_6GHZ_RNR_USR_MODE_SET(ic->ic_6ghz_rnr_enable);
            WLAN_6GHZ_ADV_USER_SET(ic->ic_6ghz_rnr_enable, frm_val);
            break;
        case RNR_6GHZ_DIS:
            /* Clear frame type and set usr mode */
            WLAN_6GHZ_ADV_USER_CLEAR(ic->ic_6ghz_rnr_enable, frm_val);
            WLAN_6GHZ_RNR_USR_MODE_SET(ic->ic_6ghz_rnr_enable);
            break;
        case RNR_6GHZ_DRIVER_MODE:
            /* Switch to driver mode and unset all frm bits.
             * User to enable frm bits upon choosing to enable
             * RNR advertisement */
            WLAN_6GHZ_ADV_USER_CLEAR(ic->ic_6ghz_rnr_enable, WLAN_RNR_FRM_MAX);
            WLAN_6GHZ_RNR_DRIVER_MODE_SET(ic->ic_6ghz_rnr_enable);
            break;
        default:
            qdf_err("Invalid parameter");
        return -EINVAL;
    }
    /* Update the beacons of the vdevs in this radio */
    if (!pdev) {
        qdf_err("Pdev is NUll, beacon update failed for RNR");
        return -EINVAL;
    }

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap && !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) &&
            vap->iv_opmode == IEEE80211_M_HOSTAP &&
            ieee80211_is_vap_state_running(vap)) {
            vap->iv_oob_update = 1;
            wlan_vdev_beacon_update(vap);
            vap->iv_oob_update = 0;
#ifdef WLAN_SUPPORT_FILS
            if (ic->ic_fd_tmpl_update)
                ic->ic_fd_tmpl_update(vap->vdev_obj);
#endif
            if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
                if (vap->iv_he_6g_bcast_prob_rsp) {
                    struct ol_ath_vap_net80211 *avn;
                    struct ieee80211_node *ni;
                    ni = vap->iv_bss;
                    avn = OL_ATH_VAP_NET80211(vap);
                    avn->av_pr_rsp_wbuf = ieee80211_prb_rsp_alloc_init(ni,
                                          &avn->av_prb_rsp_offsets);
                    if (avn->av_pr_rsp_wbuf) {
                       if (QDF_STATUS_SUCCESS != ic->ic_prb_rsp_tmpl_send(vap->vdev_obj))
                           qdf_err("20TU prb rsp send failed");
                    }
                }
            }
        }
    }
    return EOK;
}

int ol_ath_ucfg_get_col_6ghz_rnr(void *vscn, uint8_t *value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic =  &scn->sc_ic;

    *value = ic->ic_6ghz_rnr_enable;

    return EOK;
}

#if DBG_LVL_MAC_FILTERING
void dbgLVLmac_print_vap_peers(void *arg, struct ieee80211_node *ni)
{
    int *cnt = arg;

    if (ni->ni_dbgLVLmac_on) {
        (*cnt)++;
        qdf_info("\t%s\n", ether_sprintf(ni->ni_macaddr));
    }
}

int ol_ath_ucfg_set_dbglvlmac(struct ieee80211vap *vap, uint8_t *mac_addr,
                              uint8_t mac_addr_len, uint8_t value)
{
    struct ieee80211com *ic           = vap->iv_ic;
    struct ieee80211_node *ni;
    int retval = EINVAL, vap_id = wlan_vdev_get_id(vap->vdev_obj);

    ni = ieee80211_find_node(ic, mac_addr, WLAN_MLME_SB_ID);
    switch (value) {
        case DBGLVLMAC_ENABLE:
            qdf_info("Enabling ni_dbgLVLmac for peer[%s]",
                     ether_sprintf(mac_addr));
            if (!vap->dbgmac_peer_list) {
                vap->dbgmac_peer_list = dbgmac_peer_list_alloc();
                if (!vap->dbgmac_peer_list) {
                    qdf_err("Peer list allocation failed!");
                    return retval;
                }
            }
            if (ni) {
                ni->ni_dbgLVLmac_on = 1;
            } else {
                retval = dbgmac_peer_add(mac_addr, vap->dbgmac_peer_list);
                if (retval) {
                    qdf_err("Add MAC to hash table failed.");
                    return retval;
                }
            }
            vap->iv_print.dbgLVLmac_on_cnt++;
            if (!vap->iv_print.dbgLVLmac_on) {
                qdf_info("Enabling dbgLVmac for vap[%d]", vap_id);
                vap->iv_print.dbgLVLmac_on = 1;
            }
            retval = EOK;
            break;
        case DBGLVLMAC_DISABLE:
            qdf_info("Disabling ni_dbgLVLmac for peer [%s]",
                     ether_sprintf(mac_addr));
            dbgmac_peer_del(mac_addr, vap->dbgmac_peer_list);
            if (ni) {
                ni->ni_dbgLVLmac_on = 0;
            }
            vap->iv_print.dbgLVLmac_on_cnt--;
            if (vap->iv_print.dbgLVLmac_on_cnt == 0) {
                qdf_info("dbgLVLmac disabled for all, disable it [vap%d]",
                         vap->iv_unit);
                vap->iv_print.dbgLVLmac_on = 0;
            }
            retval = EOK;
            break;
        default:
            qdf_err("Invalid parameter");
            break;
    }

    /* Free node to avoid increasing ref count */
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return retval;
}

int
ol_ath_ucfg_get_dbglvlmac(struct ieee80211vap *vap, uint8_t value)
{
    int cnt = 0;

    switch (value) {
        case DBGLVLMAC_LIST:
            if (!vap->dbgmac_peer_list)
                return 0;
            qdf_info("Active dbgLVLmac clients:\n");
            wlan_iterate_station_list(vap,
                        (ieee80211_sta_iter_func)dbgLVLmac_print_vap_peers,
                        (void *)&cnt);
            dbgmac_peer_list_dump(vap->dbgmac_peer_list);
            break;
        default:
            qdf_err("Invalid parameter");
            break;
    }

    return cnt;
}
#endif /* DBG_LVL_MAC_FILTERING */

int
ol_ath_ucfg_set_halphy_cal(void *vscn, uint32_t value, uint32_t chan_sel)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    struct ieee80211com *ic = &scn->sc_ic;
    int retval = 0;
    int32_t pdev_id;
    struct wmi_unified *pdev_wmi_handle;
    struct wmi_host_send_set_halphy_cal_info set_halphy_cal_info;

    pdev_id = lmac_get_pdev_idx(scn->sc_pdev);
    if(pdev_id < 0)
            return -1;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if(!pdev_wmi_handle)
            return -1;

    if((value > 1023) || (chan_sel > 2)) {
        qdf_err("Invalid input values");
        return -1;
    }

    set_halphy_cal_info.value = value;
    set_halphy_cal_info.chan_sel = chan_sel;
    /* pdev_id will be uint8_t data type if not for Error value */
    set_halphy_cal_info.pdev_id = (uint8_t) pdev_id;

    retval = wmi_unified_send_set_halphy_cal(
                        pdev_wmi_handle, &set_halphy_cal_info);

    /* Configure IC if WMI is success */
    if(retval) {
        qdf_err("WMI send failed, discarding the configuration");
    } else {
        ic->ic_halphy_cal = value;
        ic->ic_halphy_chan_sel = chan_sel;
    }

    return retval;
}

#ifdef CONFIG_SAWF
int ol_ath_dp_sawf_peer_msduq_report(void *vscn, char *mac)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    QDF_STATUS status;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    status = cdp_sawf_peer_get_msduq_info(soc_txrx_handle, mac);
    return qdf_status_to_os_return(status);
}
#endif

#if CONFIG_SAWF_DEF_QUEUES
int ol_ath_dp_sawf_peer_svcid_map(void *vscn, char *mac, uint8_t svc_id)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    QDF_STATUS status;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    status = cdp_sawf_peer_svcid_map(soc_txrx_handle, mac, svc_id);
    return qdf_status_to_os_return(status);
}

int ol_ath_dp_sawf_peer_unmap(void *vscn, char *mac, uint8_t svc_id)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    QDF_STATUS status;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    status = cdp_sawf_peer_unmap(soc_txrx_handle, mac, svc_id);
    return qdf_status_to_os_return(status);
}

int ol_ath_dp_sawf_peer_map_report(void *vscn, char *mac)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    QDF_STATUS status;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    status = cdp_sawf_peer_get_map_conf(soc_txrx_handle, mac);
    return qdf_status_to_os_return(status);
}

int
ol_ath_ucfg_set_rate_upper_cap(void *vscn, uint8_t nss, uint8_t mcs,
                               uint8_t en_nss_cap, uint8_t en_mcs_cap)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    return ol_ath_pdev_set_rate_upper_cap(scn->sc_pdev, nss, mcs,
                                          en_nss_cap, en_mcs_cap);
}

int
ol_ath_ucfg_set_rate_retry_mcs_drop(void *vscn, uint8_t retry_thresh,
                                    uint8_t mcs_drop,
                                    uint8_t en_retry_thresh,
                                    uint8_t en_mcs_drop)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    return ol_ath_pdev_set_rate_retry_mcs_drop(scn->sc_pdev,
                                               retry_thresh, mcs_drop,
                                               en_retry_thresh, en_mcs_drop);
}

int
ol_ath_ucfg_set_mcs_probe_intvl(void *vscn, uint16_t min, uint16_t max)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    return ol_ath_pdev_set_mcs_probe_intvl(scn->sc_pdev, min, max);
}

int
ol_ath_ucfg_set_nss_probe_intvl(void *vscn, uint16_t min, uint16_t max)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    return ol_ath_pdev_set_nss_probe_intvl(scn->sc_pdev, min, max);
}
int ol_ath_ucfg_set_sawf_params(void *vscn, void *param)
{
     return ol_ath_set_sawf_params((struct wlan_objmgr_pdev *)vscn,
                                   (struct wlan_sawf_svc_class_params *)
                                    param);
}

int ol_ath_ucfg_disable_sawf(uint8_t svc_id)
{

   if (!wlan_service_id_valid(svc_id))
       return -EINVAL;

   return ol_ath_disable_sawf(svc_id);
}

#ifdef CONFIG_SAWF
int ol_ath_deactivate_peer_msduq(void *soc, uint8_t pdev_id,
                                 bool is_mlo, void *params)
{
    uint32_t prim_ifindex;
    struct wlan_objmgr_pdev *pdev;
    struct cdp_ctrl_objmgr_psoc *cdp_soc;
    struct ieee80211com *ic;
    struct ieee80211_node *ni = NULL;
    struct wlan_objmgr_psoc *psoc;
    struct ieee80211vap *vap = NULL;

    struct qca_sawf_flow_deprioritize_params *result_params;

    cdp_soc = (struct cdp_ctrl_objmgr_psoc *)soc;
    result_params = (struct qca_sawf_flow_deprioritize_params *)params;
    psoc = (struct wlan_objmgr_psoc *)cdp_soc;
    pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_SAWF_ID);
    ic = wlan_pdev_get_mlme_ext_obj(pdev);

    if (!ic) {
        qdf_err("invalid ic");
        wlan_objmgr_pdev_release_ref(pdev, WLAN_SAWF_ID);
        return QDF_STATUS_E_FAILURE;
    }

    wlan_objmgr_pdev_release_ref(pdev, WLAN_SAWF_ID);
#ifdef WLAN_FEATURE_11BE_MLO
    if (is_mlo) {
        struct wlan_mlo_peer_context *ml_peer = NULL;
        struct wlan_mlo_dev_context *mld_dev = NULL;

        ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr((struct qdf_mac_addr *)
                                                     result_params->peer_mac,
                                                     &mld_dev);
        if (mld_dev && ml_peer)
            ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    } else
#endif
    {
        ni = ieee80211_find_node(ic, result_params->peer_mac, WLAN_MLME_NB_ID);
    }

    if (!ni) {
        qdf_err("Node not found");
        return QDF_STATUS_E_FAILURE;
    }

#ifdef QCA_SUPPORT_WDS_EXTENDED
    if (ni->is_wds_client) {
        prim_ifindex = ni->netdev_if_index;
        qdf_mem_copy(result_params->netdev_mac, ni->netdev_mac_addr, QDF_MAC_ADDR_SIZE);
    } else
#endif
    {
        vap = ni->ni_vap;
        if (!vap) {
            ieee80211_free_node(ni, WLAN_MLME_NB_ID);
            return QDF_STATUS_E_FAILURE;
        }
#ifdef WLAN_FEATURE_11BE_MLO
        if (wlan_peer_is_mlo(ni->peer_obj)) {
            uint8_t prim_peer_link_id;

            prim_peer_link_id = wlan_mlo_peer_get_primary_peer_link_id(ni->peer_obj);
            prim_ifindex = wlan_get_ifidx_by_link_id(vap, prim_peer_link_id);
        } else
#endif
        {
            osif_dev *osif = NULL;
            osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
            prim_ifindex = osif->netdev->ifindex;
        }
    }

    ieee80211_free_node(ni, WLAN_MLME_NB_ID);

    result_params->netdev_info_valid = true;
    result_params->netdev_ifindex = prim_ifindex;

    qdf_info("peer_mac:%pM | mark_metadata:0x%x | netdev_info_valid:%d | netdev_ifindex:%d | netdev_mac:%pM",
             result_params->peer_mac, result_params->mark_metadata,
             result_params->netdev_info_valid, result_params->netdev_ifindex,
             result_params->netdev_mac);

    wlan_sawf_flow_deprioritize(result_params);

    return QDF_STATUS_SUCCESS;
}
#endif

#define STREAMING_STATS_BASIC    1
#define STREAMING_STATS_EXTD     2

int ol_ath_dp_sawf_stats_enable(void *vscn, uint8_t mask)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    ol_txrx_soc_handle soc_txrx_handle;
    QDF_STATUS status;
    uint8_t enable;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle)
        return -EINVAL;

    enable = (mask & STREAMING_STATS_BASIC) == STREAMING_STATS_BASIC;
    status = cdp_sawf_mpdu_stats_req(soc_txrx_handle, enable);
    if (QDF_IS_STATUS_ERROR(status))
        qdf_err("sawf stats type %u enable %u request failed",
                 STREAMING_STATS_BASIC, enable);

    enable = (mask & STREAMING_STATS_EXTD) == STREAMING_STATS_EXTD;
    status = cdp_sawf_mpdu_details_stats_req(soc_txrx_handle, enable);
    if (QDF_IS_STATUS_ERROR(status))
        qdf_err("sawf stats type %u enable %u request failed",
                 STREAMING_STATS_EXTD, enable);

    return qdf_status_to_os_return(status);
}
#endif

int
ol_ath_ucfg_set_disable_mode(void *vscn, uint32_t disabled_modes)
{
	struct ol_ath_softc_net80211 *scn =
			(struct ol_ath_softc_net80211 *)vscn;
	return ol_ath_pdev_set_disable_mode(scn->sc_pdev, disabled_modes);
}

int
ol_ath_ucfg_set_peer_disable_mode(void *vscn, uint8_t *peer_mac,
				  uint32_t disabled_modes)
{
	struct ol_ath_softc_net80211 *scn =
			(struct ol_ath_softc_net80211 *)vscn;
	struct wmi_unified *pdev_wmi_handle;

	struct wlan_objmgr_pdev *pdev = scn->sc_pdev;
	uint8_t pdev_id;

	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);

	if (!pdev_wmi_handle) {
		qdf_err("pdev_wmi_handle is null");
		return -EINVAL;
	}

	return ol_ath_pdev_set_peer_disable_mode(pdev_wmi_handle, peer_mac,
						 pdev_id,
						 disabled_modes);
}

void ol_ath_set_pn_mgmt_rxfilter(struct ieee80211vap *vap, uint32_t pn_rxfilter)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;
    struct wmi_unified *pdev_wmi_handle;
    struct vdev_pn_mgmt_rxfilter_params params;

    if (!vap || !vap->iv_ic)
        return;

    ic = vap->iv_ic;
    scn = OL_ATH_SOFTC_NET80211(ic);

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle)
        return;

    params.vdev_id = vap->iv_unit;
    params.pn_rxfilter = pn_rxfilter;

    /* Set iv pn_rxfilter if WMI send command is successful */
    if (wmi_unified_pn_mgmt_rxfilter_send_cmd(pdev_wmi_handle, &params)) {
        qdf_err("WMI send cmd failed, not setting pn_rxfilter from app_filter");
    } else {
        qdf_debug("PN RxFilter set to %u", pn_rxfilter);
        vap->iv_pn_rxfilter = pn_rxfilter;
    }
}

void ol_ath_ucfg_get_peer_extd_rate_link_stats(void *vscn, char *mac_addr)
{
    struct ol_ath_softc_net80211 *scn = ( struct ol_ath_softc_net80211*) vscn;
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    cdp_get_peer_extd_rate_link_stats(soc_txrx_handle, mac_addr);
}

#ifdef DP_UMAC_HW_RESET_SUPPORT
/**
 * ol_ath_ucfg_umac_reset_to_cdp_umac_reset() - API to convert OL specific umac
 *                                              reset commands to CDP.
 * @cmd: OL umac reset command
 * @param: Pointer to save CDP umac reset command
 *
 * Return: success ot failure
 */
static int ol_ath_ucfg_umac_reset_to_cdp_umac_reset(
        enum ol_ath_umac_reset_cmds cmd, enum cdp_psoc_param_type *param)
{
    int ret = 0;

    switch(cmd) {
    case OL_ATH_UMAC_RESET_STATS:
        *param = CDP_UMAC_RESET_STATS;
        break;
    case OL_ATH_UMAC_RESET_SKEL:
        *param = CDP_UMAC_RST_SKEL_ENABLE;
        break;
    default:
        ret = -EINVAL;
    }

    return ret;
}

/**
 * ol_ath_ucfg_set_cdp_config_param_val() - API to configure the umac reset
 *                                          values in CDP structures.
 * @cmd: CDP umac reset command
 * @param: Pointer to CDP config structure
 * @value: User configured value
 *
 * Return: none
 */
static void ol_ath_ucfg_set_cdp_config_param_val(enum cdp_psoc_param_type cmd,
                                                 cdp_config_param_type *param,
                                                 int value)
{
    switch (cmd) {
    case CDP_UMAC_RST_SKEL_ENABLE:
        param->cdp_umac_rst_skel = !!value;
        return;
    default:
        return;
    }
}

/**
 * ol_ath_ucfg_cdp_txrx_get_psoc_param() - API to read the umac reset related
 *                                         values from CDP structure
 * @soc_txrx_handle: Pointer to DP SOC structure
 * @cdp_param: CDP param
 * @val: pointer to CDP structure to read the umac reset related value
 *
 * Return: None
 */
static void ol_ath_ucfg_cdp_txrx_get_psoc_param(
        ol_txrx_soc_handle soc_txrx_handle, enum cdp_psoc_param_type cdp_param,
        cdp_config_param_type *val)
{
    switch(cdp_param) {
    case CDP_UMAC_RST_SKEL_ENABLE:
        val->cdp_umac_rst_skel = 0;
        cdp_txrx_get_psoc_param(soc_txrx_handle, cdp_param, val);
        qdf_nofl_info("cdp_umac_rst_skel = %d", val->cdp_umac_rst_skel);
        break;
    default:
        break;
    }
}

int ol_ath_ucfg_set_umac_reset_debug(void *vscn, uint32_t cmd, uint32_t value)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    enum cdp_psoc_param_type cdp_param_type;
    cdp_config_param_type cdp_config_param = {0};
    ol_txrx_soc_handle soc_txrx_handle;
    int retval = 0;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    retval = ol_ath_ucfg_umac_reset_to_cdp_umac_reset(cmd, &cdp_param_type);
    if (retval) {
        qdf_err("Invalid UMAC reset command\n");
        return retval;
    }

    ol_ath_ucfg_set_cdp_config_param_val(cdp_param_type, &cdp_config_param,
                                         value);

    cdp_txrx_set_psoc_param(soc_txrx_handle, cdp_param_type, cdp_config_param);

    return retval;
}

int ol_ath_ucfg_get_umac_reset_debug(void *vscn, uint32_t cmd)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)vscn;
    enum cdp_psoc_param_type cdp_param_type;
    cdp_config_param_type cdp_config_param = {0};
    ol_txrx_soc_handle soc_txrx_handle;
    int retval = 0;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    retval = ol_ath_ucfg_umac_reset_to_cdp_umac_reset(cmd, &cdp_param_type);
    if (retval) {
        qdf_err("Invalid UMAC reset command\n");
        return -EINVAL;
    }

    ol_ath_ucfg_cdp_txrx_get_psoc_param(soc_txrx_handle, cdp_param_type,
                                        &cdp_config_param);

    return retval;
}
#endif

#ifdef QCA_R2P_UPDATE_ENABLED
void ol_ath_set_home_chan_r2p_table_async(void *data)
{
    int status;
    char *buff;
    u_int32_t buff_size;
    struct ieee80211com *ic = (struct ieee80211com *)data;
    struct ol_ath_softc_net80211 *scn;

    scn = OL_ATH_SOFTC_NET80211(ic);
    status = qdf_file_read_bytes(ATH_HOME_CHAN_R2P_FILE, &buff, &buff_size);
    if (QDF_IS_STATUS_SUCCESS(status)) {
        status = ieee80211_send_homechan_r2p_table(scn, buff, buff_size,
                                                   ol_ath_ucfg_rate2power_set);
        qdf_file_buf_free(buff);
    }
    qdf_info("Home chan r2p table update status: %d", status);
}

qdf_export_symbol(ol_ath_set_home_chan_r2p_table_async);
#endif /* QCA_R2P_UPDATE_ENABLED */
