/*
 * Copyright (c) 2011-2021, 2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 *
 */

#include <ieee80211_var.h>
#include <ieee80211_ucfg.h>
#include <ieee80211_channel.h>
#include <ieee80211_rateset.h>
#include <ieee80211_config.h>
#include <ieee80211_tsftimer.h>
#include <ieee80211_notify_tx_bcn.h>
#include <ieee80211P2P_api.h>
#include <ieee80211_wnm_proto.h>
#include <ieee80211_vi_dbg.h>
#include <ieee80211_bsscolor.h>
#include "ol_if_athvar.h"
#include "target_type.h"
#include <qdf_lock.h>
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_mlme_dp_dispatcher.h>
#include <wlan_utility.h>
#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif
#include <wlan_mlme_dispatcher.h>
#include <target_if.h>
#include <init_deinit_lmac.h>
#include <cfg_ucfg_api.h>
#include <wlan_vdev_mlme.h>
#include <wlan_pdev_mlme.h>
#include <include/wlan_psoc_mlme.h>
#include <wlan_objmgr_global_obj_i.h>
#include <wlan_mlme_if.h>
#include <wlan_reg_ucfg_api.h>
#include "ieee80211_crypto_nlshim_api.h"
#ifdef WLAN_SUPPORT_FILS
#include <wlan_fd_ucfg_api.h>
#endif /* WLAN_SUPPORT_FILS */
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_setup.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#include <ieee80211_proto_mlo.h>
#include <wlan_mlo_mgr_cmn.h>
#include <wlan_mlo_t2lm.h>
#include <wlan_mgmt_txrx_rx_reo_utils_api.h>
#endif /* WLAN_FEATURE_11BE_MLO */
#include <wlan_mbss.h>
#include "wlan_vdev_mgr_utils_api.h"
#include "wlan_rnr.h"
#include "ieee80211_api.h"
#include <ieee80211_cfg80211.h>
#include <_ieee80211.h>
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
#include "cdp_txrx_mlo.h"
#endif
#if defined(WLAN_FEATURE_11BE_MLO_3_LINK_TX) && \
    defined(CONFIG_SAWF)
#include "cdp_txrx_sawf.h"
#endif
#include <wlan_mlme_vdev_mgmt_ops.h>

/* Support for runtime pktlog enable/disable */
unsigned int enable_pktlog_support = 1; /*Enable By Default*/
module_param(enable_pktlog_support, uint, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(enable_pktlog_support,
        "Runtime pktlog enable/disable Support");

extern ol_ath_soc_softc_t *ol_global_soc[GLOBAL_SOC_SIZE];
extern unsigned long wlan_shortssid_table[];
extern int acfg_attach(struct ieee80211com *ic);
extern void acfg_detach(struct ieee80211com *ic);
uint32_t acfg_event_workqueue_init(osdev_t osdev);

#if UMAC_SUPPORT_ACFG
extern int acfg_diag_attach(struct ieee80211com *ic);
extern int acfg_diag_detach(struct ieee80211com *ic);
#endif


int module_init_wlan(void);
void module_exit_wlan(void);


void print_vap_msg(struct ieee80211vap *vap, unsigned category, const char *fmt, ...)
{
     va_list ap;
     va_start(ap, fmt);
     if (vap) {
        asf_vprint_category(&vap->iv_print, category, fmt, ap);
     } else {
        qdf_vprint(fmt, ap);
     }
     va_end(ap);
}

void print_vap_verbose_msg(struct ieee80211vap *vap, unsigned verbose, unsigned category, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (vap) {
        asf_vprint(&vap->iv_print, category, verbose, fmt, args);
    } else {
        qdf_vprint(fmt, args);
    }
    va_end(args);
}

/**
* ASF print support function to print based on category for vap print control object
* @param vap - object of struct ieee80211vap in which asf print control object is declared
* @param category - category of the print message
*/
void IEEE80211_DPRINTF(struct ieee80211vap *vap, unsigned category, const char *fmt, ...)
{
     char                   tmp_buf[OS_TEMP_BUF_SIZE], *tmp;
     va_list                ap;
     struct ieee80211com    *ic = NULL;

     if ((vap) && ieee80211_msg(vap, category)) {
         ic = vap->iv_ic;
         tmp = tmp_buf + snprintf(tmp_buf,OS_TEMP_BUF_SIZE, "[%s] vap-%d(%s):",
                             msg_type_to_str(category), vap->iv_unit, vap->iv_netdev_name);
#if DBG_LVL_MAC_FILTERING
        if (!vap->iv_print.dbgLVLmac_on) {
#endif
             va_start(ap, fmt);
             vsnprintf (tmp,(OS_TEMP_BUF_SIZE - (tmp - tmp_buf)), fmt, ap);
             va_end(ap);
             print_vap_msg(vap, category, (const char *)tmp_buf, ap);
             ic->ic_log_text(ic,tmp_buf);
             OS_LOG_DBGPRINT("%s\n", tmp_buf);
#if DBG_LVL_MAC_FILTERING
        }
#endif
    }

}


/**
* ASF print support function to print based on category for vap print control object
* @param vap - object of struct ieee80211vap in which asf print control object is declared
* @param verbose - verbose level of the print message
* @param category - category of the print message
*/
void IEEE80211_DPRINTF_VB(struct ieee80211vap *vap, unsigned verbose, unsigned category, const char *fmt, ...)
{
     char                   tmp_buf[OS_TEMP_BUF_SIZE], *tmp;
     va_list                ap;
     struct ieee80211com    *ic = NULL;

     if ((vap) && (verbose <= vap->iv_print.verb_threshold) && ieee80211_msg(vap, category)) {
         ic = vap->iv_ic;
         tmp = tmp_buf + snprintf(tmp_buf,OS_TEMP_BUF_SIZE, "[%s] vap-%d(%s):",
                             msg_type_to_str(category), vap->iv_unit, vap->iv_netdev_name);
         va_start(ap, fmt);
         vsnprintf (tmp,(OS_TEMP_BUF_SIZE - (tmp - tmp_buf)), fmt, ap);
         va_end(ap);
         print_vap_verbose_msg(vap, verbose, category, (const char *)tmp_buf, ap);
         ic->ic_log_text(ic,tmp_buf);
         OS_LOG_DBGPRINT("%s\n", tmp_buf);
    }

}

/**
* ASF print support function tp print based on category for ic print control object
* @param ic - object of struct ieee80211com in which asf print control object is declared
* @param category - category of the print message
*/
void IEEE80211_DPRINTF_IC_CATEGORY(struct ieee80211com *ic, unsigned category, const char *fmt, ...)
{
    va_list args;

    if ( (ic) && ieee80211_msg_ic(ic, category)) {
        va_start(args, fmt);
        if (ic) {
            asf_vprint_category(&ic->ic_print, category, fmt, args);
        } else {
            qdf_vprint(fmt, args);
        }
        va_end(args);
    }

}

/**
* ASF print support function tp print based on category and verbose for ic print control object
* @param ic - object of struct ieee80211com in which asf print control object is declared
* @param verbose - verbose level of the print message
* @param category - category of the print message
*/
void IEEE80211_DPRINTF_IC(struct ieee80211com *ic, unsigned verbose, unsigned category, const char *fmt, ...)
{
    va_list args;

    if ((ic) && (verbose <= ic->ic_print.verb_threshold) && ieee80211_msg_ic(ic, category)) {
        va_start(args, fmt);
        if (ic) {
            asf_vprint(&(ic)->ic_print, category, verbose, fmt, args);
        } else {
            qdf_vprint(fmt, args);
        }
        va_end(args);
    }

}

/*
 * With WEP and TKIP encryption algorithms:
 * Disable 11n if IEEE80211_FEXT_WEP_TKIP_HTRATE is not set.
 * Check for Mixed mode, if CIPHER is set to TKIP
 */
int ieee80211vap_htallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    int phymode;
    /* Disable HT if WMM/wme is disabled */
    if (!ieee80211_vap_wme_is_set(vap)) {
        return 0;
    }

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)
       phymode = vap->iv_cur_mode;
    else
       phymode = vap->iv_des_mode;

    switch (phymode) {
    case IEEE80211_MODE_11A:
    case IEEE80211_MODE_11B:
    case IEEE80211_MODE_11G:
    case IEEE80211_MODE_FH:
    case IEEE80211_MODE_TURBO_A:
    case IEEE80211_MODE_TURBO_G:
        return 0;
    default:
        break;
    }

    if (!ieee80211_ic_wep_tkip_htrate_is_set(ic) &&
        IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) &&
        is_weptkip_htallowed(vap, NULL))
        return 0;
    else if (vap->iv_opmode == IEEE80211_M_IBSS)
        return (ieee80211_ic_ht20Adhoc_is_set(ic) || ieee80211_ic_ht40Adhoc_is_set(ic));
    else
        return 1;
}

int ieee80211vap_vhtallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    int phymode;
    /* Don't allow VHT if HT is not allowed */
    if (!ieee80211vap_htallowed(vap)){
        return 0;
    }

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)
       phymode = vap->iv_cur_mode;
    else
       phymode = vap->iv_des_mode;

    /* Don't allow VHT if mode is HT only  */
    switch (phymode) {
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
            return 0;
        break;
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
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
             /*VHT is allowed in 2G if 256 QAM is supported */
           if(!ieee80211_vap_256qam_is_set(vap))
            return 0;

        default:
            break;
    }

    if (ic->ic_vhtcap) {
        return 1;
    }

    return 0;
}

int ieee80211vap_6g_heallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    int phymode;

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)
       phymode = vap->iv_cur_mode;
    else
       phymode = vap->iv_des_mode;

    switch (phymode) {
        case IEEE80211_MODE_AUTO:
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
            if (IEEE80211_IS_HECAP_MACINFO(ic->ic_he.hecap_macinfo)) {
                return 1;
            }
            break;
        default:
            return 0;
    }

    return 0;
}
#ifdef WLAN_FEATURE_11BE
#define IEEE80211_IS_HE_PHYMODE(phymode) \
    (ieee80211_is_phymode_11ax(phymode) || \
     ieee80211_is_phymode_11be(phymode))
#else
#define IEEE80211_IS_HE_PHYMODE(phymode) \
     ieee80211_is_phymode_11ax(phymode)
#endif

int ieee80211vap_heallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    int phymode;
    qdf_freq_t low_freq = 0, high_freq = 0;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status;

    pdev = ic->ic_pdev_obj;

    status = wlan_reg_get_freq_range(pdev, NULL, NULL, &low_freq, &high_freq);
    if (status)
        return 0;

    if (wlan_reg_is_range_only6g(low_freq, high_freq)) {
       return ieee80211vap_6g_heallowed(vap);
    }
    /* Don't allow HE if VHT & HT is not allowed */
    if (!ieee80211vap_htallowed(vap)){
        return 0;
    }

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)
       phymode = vap->iv_cur_mode;
    else
       phymode = vap->iv_des_mode;

    if (ieee80211_is_phymode_5g_or_6g(phymode) && !ieee80211vap_vhtallowed(vap))
        return 0;

    if (!IEEE80211_IS_HE_PHYMODE(phymode))
        return 0;

    if (IEEE80211_IS_HECAP_MACINFO(ic->ic_he.hecap_macinfo)) {
        return 1;
    }

    return 0;
}

#ifdef WLAN_FEATURE_11BE
int ieee80211vap_6g_ehtallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    int phymode;

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)
       phymode = vap->iv_cur_mode;
    else
       phymode = vap->iv_des_mode;

    switch (phymode) {
        case IEEE80211_MODE_AUTO:
        case IEEE80211_MODE_11BEA_EHT20:
        case IEEE80211_MODE_11BEA_EHT40PLUS:
        case IEEE80211_MODE_11BEA_EHT40MINUS:
        case IEEE80211_MODE_11BEA_EHT40:
        case IEEE80211_MODE_11BEA_EHT80:
        case IEEE80211_MODE_11BEA_EHT160:
        case IEEE80211_MODE_11BEA_EHT320:
            if (IEEE80211_IS_EHTCAP_MACINFO(ic->ic_eht.ehtcap_macinfo)) {
                return 1;
            }
            break;
        default:
            return 0;
    }

    return 0;
}

int ieee80211vap_ehtallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    int phymode;
    qdf_freq_t low_freq = 0, high_freq = 0;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status;

    pdev = ic->ic_pdev_obj;

    status = wlan_reg_get_freq_range(pdev, NULL, NULL, &low_freq, &high_freq);
    if (status)
        return 0;

    if (wlan_reg_is_range_only6g(low_freq, high_freq)) {
       return ieee80211vap_6g_ehtallowed(vap);
    }
    /* Don't allow EHT if HE is not allowed */
    if (!ieee80211vap_heallowed(vap)){
        return 0;
    }

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)
       phymode = vap->iv_cur_mode;
    else
       phymode = vap->iv_des_mode;

    if (!ieee80211_is_phymode_11be(phymode))
        return 0;

    if (IEEE80211_IS_EHTCAP_MACINFO(ic->ic_eht.ehtcap_macinfo)) {
        return 1;
    }

    return 0;
}
#endif /* WLAN_FEATURE_11BE */

IEEE80211_IS_PHYMODE_DEF(auto, AUTO);
IEEE80211_IS_PHYMODE_DEF(11a, 11A);
IEEE80211_IS_PHYMODE_DEF(turbo_a, TURBO_A);
IEEE80211_IS_PHYMODE_DEF(11b, 11B);
IEEE80211_IS_PHYMODE_DEF(11g, 11G);
IEEE80211_IS_PHYMODE_DEF(11na_ht20, 11NA_HT20);
IEEE80211_IS_PHYMODE_DEF(11na_ht40, 11NA_HT40);
IEEE80211_IS_PHYMODE_DEF(11ng_ht20, 11NG_HT20);
IEEE80211_IS_PHYMODE_DEF(11ng_ht40, 11NG_HT40);
IEEE80211_IS_PHYMODE_DEF(11axa_he40, 11AXA_HE40);
IEEE80211_IS_PHYMODE_DEF(11ac_vht40, 11AC_VHT40);
IEEE80211_IS_PHYMODE_DEF(11axg_he40, 11AXG_HE40);
IEEE80211_IS_PHYMODE_DEF(11ac_vht40minus, 11AC_VHT40MINUS);
IEEE80211_IS_PHYMODE_DEF(11ac_vht40plus, 11AC_VHT40PLUS);
IEEE80211_IS_PHYMODE_DEF(11axa_he40plus, 11AXA_HE40PLUS);
IEEE80211_IS_PHYMODE_DEF(11axa_he40minus, 11AXA_HE40MINUS);
IEEE80211_IS_PHYMODE_DEF(11ac_vht80, 11AC_VHT80);
IEEE80211_IS_PHYMODE_DEF(11ac_vht160, 11AC_VHT160);
IEEE80211_IS_PHYMODE_DEF(11ac_vht80_80, 11AC_VHT80_80);
IEEE80211_IS_PHYMODE_DEF(11axa_he80, 11AXA_HE80);
IEEE80211_IS_PHYMODE_DEF(11axa_he160, 11AXA_HE160);
IEEE80211_IS_PHYMODE_DEF(11axa_he80_80, 11AXA_HE80_80);
#ifdef WLAN_FEATURE_11BE
IEEE80211_IS_PHYMODE_DEF(11bea_eht40, 11BEA_EHT40);
IEEE80211_IS_PHYMODE_DEF(11beg_eht40, 11BEG_EHT40);
IEEE80211_IS_PHYMODE_DEF(11bea_eht40plus, 11BEA_EHT40PLUS);
IEEE80211_IS_PHYMODE_DEF(11bea_eht40minus, 11BEA_EHT40MINUS);
IEEE80211_IS_PHYMODE_DEF(11bea_eht80, 11BEA_EHT80);
IEEE80211_IS_PHYMODE_DEF(11bea_eht160, 11BEA_EHT160);
IEEE80211_IS_PHYMODE_DEF(11bea_eht320, 11BEA_EHT320);
#endif /* WLAN_FEATURE_11BE */

bool ieee80211_is_phymode_not_basic(uint32_t  mode)
{
    if (mode >= IEEE80211_MODE_11NA_HT20 && mode <=
#ifdef WLAN_FEATURE_11BE
            IEEE80211_MODE_11BEA_EHT320
#else
            IEEE80211_MODE_11AXA_HE80_80
#endif /* WLAN_FEATURE_11BE */
            ) {
        return true;
    }
    return false;
}
qdf_export_symbol(ieee80211_is_phymode_not_basic);

bool ieee80211_is_phymode_g40(uint32_t mode)
{
    if (mode == IEEE80211_MODE_11NG_HT40PLUS   ||
        mode == IEEE80211_MODE_11NG_HT40MINUS  ||
        mode == IEEE80211_MODE_11NG_HT40       ||
        mode == IEEE80211_MODE_11AXG_HE40PLUS  ||
        mode == IEEE80211_MODE_11AXG_HE40MINUS ||
        mode == IEEE80211_MODE_11AXG_HE40
#ifdef WLAN_FEATURE_11BE
        || mode == IEEE80211_MODE_11BEG_EHT40PLUS
        || mode == IEEE80211_MODE_11BEG_EHT40MINUS
        || mode == IEEE80211_MODE_11BEG_EHT40
#endif /* WLAN_FEATURE_11BE */
        ) {
        return true;
     }

     return false;
}
qdf_export_symbol(ieee80211_is_phymode_g40);

bool ieee80211_is_phymode_40(uint32_t mode)
{
    if (mode == IEEE80211_MODE_11NG_HT40 ||
        mode == IEEE80211_MODE_11NA_HT40 ||
        mode == IEEE80211_MODE_11AC_VHT40 ||
        mode == IEEE80211_MODE_11AXA_HE40 ||
        mode == IEEE80211_MODE_11AXG_HE40
#ifdef WLAN_FEATURE_11BE
        || mode == IEEE80211_MODE_11BEA_EHT40
        || mode == IEEE80211_MODE_11BEG_EHT40
#endif /* WLAN_FEATURE_11BE */
        ) {
       return true;
    }
    return false;
}
qdf_export_symbol(ieee80211_is_phymode_40);

bool ieee80211_is_phymode_40plus(uint32_t mode)
{
    if (
#ifdef WLAN_FEATURE_11BE
        mode == IEEE80211_MODE_11BEA_EHT40PLUS ||
        mode == IEEE80211_MODE_11BEG_EHT40PLUS ||
#endif /* WLAN_FEATURE_11BE */
        mode == IEEE80211_MODE_11AXA_HE40PLUS ||
        mode == IEEE80211_MODE_11AXG_HE40PLUS ||
        mode == IEEE80211_MODE_11AC_VHT40PLUS ||
        mode == IEEE80211_MODE_11NA_HT40PLUS ||
        mode == IEEE80211_MODE_11NG_HT40PLUS) {
        return true;
    }
    return false;
}
qdf_export_symbol(ieee80211_is_phymode_40plus);

bool ieee80211_is_phymode_40minus(uint32_t mode)
{

    if (
#ifdef WLAN_FEATURE_11BE
        mode == IEEE80211_MODE_11BEA_EHT40MINUS ||
        mode == IEEE80211_MODE_11BEG_EHT40MINUS ||
#endif /* WLAN_FEATURE_11BE */
        mode == IEEE80211_MODE_11AXA_HE40MINUS ||
        mode == IEEE80211_MODE_11AXG_HE40MINUS ||
        mode == IEEE80211_MODE_11AC_VHT40MINUS ||
        mode == IEEE80211_MODE_11NA_HT40MINUS ||
        mode == IEEE80211_MODE_11NG_HT40MINUS) {
        return true;
    }
    return false;
}
qdf_export_symbol(ieee80211_is_phymode_40minus);

bool ieee80211_is_phymode_11ac_160or8080(uint32_t mode)
{
    if (ieee80211_is_phymode_11ac_vht160(mode) ||
        ieee80211_is_phymode_11ac_vht80_80(mode)) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_11ac_160or8080);

bool ieee80211_is_phymode_11axa_160or8080(uint32_t mode)
{
    if (ieee80211_is_phymode_11axa_he160(mode) ||
        ieee80211_is_phymode_11axa_he80_80(mode)) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_11axa_160or8080);

bool ieee80211_is_phymode_320(uint32_t mode)
{
#ifdef WLAN_FEATURE_11BE
    if (ieee80211_is_phymode_11bea_eht320(mode)) {
        return true;
    }
#endif /* WLAN_FEATURE_11BE */

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_320);

bool ieee80211_is_phymode_160(uint32_t mode)
{
    if (ieee80211_is_phymode_11ac_vht160(mode) ||
        ieee80211_is_phymode_11axa_he160(mode)
#ifdef WLAN_FEATURE_11BE
        || ieee80211_is_phymode_11bea_eht160(mode)
#endif /* WLAN_FEATURE_11BE */
	) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_160);

bool ieee80211_is_phymode_80(uint32_t mode)
{
    if (ieee80211_is_phymode_11ac_vht80(mode) ||
        ieee80211_is_phymode_11axa_he80(mode)
#ifdef WLAN_FEATURE_11BE
        || ieee80211_is_phymode_11bea_eht80(mode)
#endif /* WLAN_FEATURE_11BE */
	) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_80);

bool ieee80211_is_phymode_8080(uint32_t mode)
{
    if (ieee80211_is_phymode_11ac_vht80_80(mode) ||
        ieee80211_is_phymode_11axa_he80_80(mode)) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_8080);

bool ieee80211_is_phymode_160_or_8080(uint32_t mode)
{
    if (ieee80211_is_phymode_11ac_160or8080(mode) ||
        ieee80211_is_phymode_11axa_160or8080(mode)
#ifdef WLAN_FEATURE_11BE
        || ieee80211_is_phymode_11bea_eht160(mode)
#endif /* WLAN_FEATURE_11BE */
	) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_160_or_8080);

bool ieee80211_is_phymode_11be(uint32_t mode)
{
#ifdef WLAN_FEATURE_11BE
    if (mode >= IEEE80211_MODE_11BEA_EHT20  &&
        mode <= IEEE80211_MODE_11BEA_EHT320) {
        return true;
    }
#endif /* WLAN_FEATURE_11BE */

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_11be);


bool ieee80211_is_phymode_160_or_320(uint32_t mode)
{
    if (ieee80211_is_phymode_160(mode)
        || ieee80211_is_phymode_320(mode)
	) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_160_or_320);

bool ieee80211_is_phymode_11ax(uint32_t mode)
{
    if (mode >= IEEE80211_MODE_11AXA_HE20  &&
        mode <= IEEE80211_MODE_11AXA_HE80_80) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_11ax);

bool ieee80211_is_phymode_11ac(uint32_t mode)
{
    if (mode >= IEEE80211_MODE_11AC_VHT20  &&
        mode <= IEEE80211_MODE_11AC_VHT80_80) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_11ac);

bool ieee80211_is_phymode_11na(uint32_t mode)
{
   switch(mode) {
      case IEEE80211_MODE_11NA_HT20:
      case IEEE80211_MODE_11NA_HT40PLUS:
      case IEEE80211_MODE_11NA_HT40MINUS:
      case IEEE80211_MODE_11NA_HT40:
           return true;
      default:
           return false;
   }
   return false;
}
qdf_export_symbol(ieee80211_is_phymode_11na);

bool ieee80211_is_phymode_11ng(uint32_t mode)
{
   switch(mode) {
      case IEEE80211_MODE_11NG_HT20:
      case IEEE80211_MODE_11NG_HT40PLUS:
      case IEEE80211_MODE_11NG_HT40MINUS:
      case IEEE80211_MODE_11NG_HT40:
           return true;
      default:
           return false;
   }
   return false;
}
qdf_export_symbol(ieee80211_is_phymode_11ng);

bool ieee80211_is_phymode_valid(uint32_t mode)
{
    if (mode > IEEE80211_MODE_AUTO && mode < IEEE80211_MODE_MAX) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_valid);


bool ieee80211_is_phymode_allowed(uint32_t mode)
{
    if (ieee80211_is_phymode_auto(mode) ||
        ieee80211_is_phymode_valid(mode)) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_allowed);

bool ieee80211_is_phymode_equal_or_above_11axa_he40plus(uint32_t mode)
{
    if ((mode >= IEEE80211_MODE_11AXA_HE40PLUS)
           && (mode <= IEEE80211_MODE_11AXA_HE80_80)) {
        return true;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_equal_or_above_11axa_he40plus);

bool ieee80211_is_phymode_equal_or_above_11bea_eht40plus(uint32_t mode)
{
#ifdef WLAN_FEATURE_11BE
    if ((mode >= IEEE80211_MODE_11BEA_EHT40PLUS)
           && (mode <= IEEE80211_MODE_11BEA_EHT320)) {
        return true;
    }
#endif

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_equal_or_above_11bea_eht40plus);

bool ieee80211_is_phymode_2g(uint32_t mode)
{
    switch (mode) {
        case IEEE80211_MODE_11B:
        case IEEE80211_MODE_11G:
        case IEEE80211_MODE_FH:
        case IEEE80211_MODE_TURBO_G:
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40:
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
                return true;
        default:
                return false;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_2g);

bool ieee80211_is_phymode_5g_or_6g(uint32_t mode)
{
    switch (mode) {
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
            return true;
        default:
            return false;
    }

    return false;
}
qdf_export_symbol(ieee80211_is_phymode_5g_or_6g);

static void ieee80211_vap_iter_mlme_inact_timeout(void *arg, struct ieee80211vap *vap)
{
    mlme_inact_timeout(vap);
}

void ieee80211_vap_mlme_inact_erp_timeout(struct ieee80211com *ic)
{
    wlan_iterate_vap_list(ic, ieee80211_vap_iter_mlme_inact_timeout, NULL);
}

int ieee80211_vap_update_wnm_bss_cap(struct ieee80211vap *vap, bool enable) {

    struct ieee80211com *ic = vap->iv_ic;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;
    struct vdev_mlme_inactivity_params *inact_params;
    int retv = 0;

    if(!ic) {
        qdf_err("IC is NULL");
        return -EINVAL;
    }

    if(!vdev_mlme) {
        qdf_err("Vdev MLME obj is NULL");
        return -EINVAL;
    }

    inact_params = &vdev_mlme->mgmt.inactivity_params;
    if(!inact_params) {
        qdf_err("Inactivity Params structure is NULL");
        return -EINVAL;
    }

    if(enable == 1 && (ieee80211_vap_wnm_is_set(vap) == 0)) {
        return -EINVAL;
    }
    if(ieee80211_wnm_bss_is_set(vap->wnm) == enable) {
        return -EINVAL;
    }

    if (enable == 0) {
        ic->ic_wnm_bss_count--;
        ieee80211_wnm_bss_clear(vap->wnm);
        if(ic->ic_wnm_bss_count == 0 && ic->ic_wnm_bss_active == 1) {
            if(ic->ic_opmode != IEEE80211_M_STA) {
                if (ic->ic_get_tgt_type(ic) < TARGET_TYPE_QCA8074) {
                    /* For pre-lithium targets, Max BSS Idle Timer
                     * is maintained in Host. Cancel the timer when
                     * the feature is disabled.
                     */
                    OS_CANCEL_TIMER(&ic->ic_bssload_timer);
                } else {
                    /* For lithium targets, update the Inactivity Timer WMIs
                     * with the saved inactivity timer values if MAx BSS
                     * Idle Time feature is being disabled.
                     */
                    retv = wlan_set_param(vap, IEEE80211_RUN_INACT_TIMEOUT,
                            inact_params->keepalive_max_unresponsive_time_secs);
                }
            }
            ic->ic_wnm_bss_active = 0;
        }
    } else {
        ieee80211_wnm_bss_set(vap->wnm);
        ic->ic_wnm_bss_count++;
        if(ic->ic_wnm_bss_active == 0) {
            if(ic->ic_opmode != IEEE80211_M_STA) {
                if (ic->ic_get_tgt_type(ic) < TARGET_TYPE_QCA8074) {
                    /* For pre-lithium targets, Max BSS Idle Timer
                     * is maintained in Host. Update the timer value when
                     * the feature is enabled.
                     */
                    OS_SET_TIMER(&ic->ic_bssload_timer, IEEE80211_BSSLOAD_WAIT);
                } else {
                    /* For lithium targets, update the Inactivity Timer WMIs
                     * with the BSS Idle period value when Max BSS Idle Time
                     * feature is enabled.
                     */
                    retv = wlan_set_param(vap, IEEE80211_RUN_INACT_TIMEOUT,
                                            vap->wnm->wnm_bss_max_idle_period);
                }
            }
            ic->ic_wnm_bss_active = 1;
        }
    }

    return retv;
}

static OS_TIMER_FUNC(ic_cw_timeout)
{
     struct ieee80211com *ic = NULL;
     OS_GET_TIMER_ARG(ic,struct ieee80211com *);
     ic->ic_bss_to40(ic);
}

static OS_TIMER_FUNC(ieee80211_obss_nb_ru_tolerence_timeout)
{
    struct ieee80211com *ic;

    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    if (ic->ic_initialized == 1) {
        ic->ru26_tolerant = true;
        ic->ic_set_ru26_tolerant(ic, ic->ru26_tolerant);
    }
}

static os_timer_func(ieee80211_csa_max_rx_wait_timer)
{
    struct ieee80211com *ic;

    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    if (!ic)
        return;

    /* unsubscribe to ppdu stats */
    if (ic->ic_subscribe_csa_interop_phy) {
        ic->ic_subscribe_csa_interop_phy(ic, false);
        ic->ic_csa_interop_subscribed = false;
    }
}

#if UMAC_SUPPORT_WNM
static OS_TIMER_FUNC(ieee80211_bssload_timeout)
{
    struct ieee80211com *ic;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);
    ieee80211_wnm_bss_validate_inactivity(ic);
    OS_SET_TIMER(&ic->ic_bssload_timer, IEEE80211_BSSLOAD_WAIT);
}
#endif

static void update_noise_stats_iter_cb(void *arg, struct ieee80211_node *ni)
{
    uint8_t bin_index, median_index, temp_variable;
    struct ieee80211com *ic;
    uint8_t *median_array = (uint8_t *) arg;

    ic = ni->ni_ic;

    if (ni->ni_associd != 0) {
        if (ic->bin_number != 0) {
            ni->ni_noise_stats[ic->bin_number].noise_value = ni->ni_snr;
            for (bin_index = 0;bin_index <= ic->bin_number;bin_index++) {
                median_array[bin_index] = ni->ni_noise_stats[bin_index].noise_value;
            }
            for (bin_index = 0;bin_index <= ic->bin_number;bin_index++) {
                for (median_index = 0;median_index < (ic->bin_number - bin_index); median_index++) {
                    if (median_array[median_index] >= median_array[median_index+1]) {
                        temp_variable = median_array[median_index];
                        median_array[median_index] = median_array[median_index+1];
                        median_array[median_index+1] = temp_variable;
                    }
                }
            }

            if ((ic->bin_number) %2 == 0) {
                ni->ni_noise_stats[ic->bin_number].median_value = median_array[ic->bin_number/2];
            } else {
                ni->ni_noise_stats[ic->bin_number].median_value = median_array[(ic->bin_number/2) + 1];
            }

            if (ni->ni_noise_stats[ic->bin_number].noise_value <= ni->ni_noise_stats[ic->bin_number-1].min_value) {
                ni->ni_noise_stats[ic->bin_number].min_value = ni->ni_noise_stats[ic->bin_number].noise_value;
            } else {
                ni->ni_noise_stats[ic->bin_number].min_value = ni->ni_noise_stats[ic->bin_number-1].min_value;
            }
            if (ni->ni_noise_stats[ic->bin_number].noise_value >= ni->ni_noise_stats[ic->bin_number-1].max_value) {
                ni->ni_noise_stats[ic->bin_number].max_value = ni->ni_noise_stats[ic->bin_number].noise_value;
            } else {
                ni->ni_noise_stats[ic->bin_number].max_value = ni->ni_noise_stats[ic->bin_number-1].max_value;
            }
        } else {
            ni->ni_noise_stats[ic->bin_number].noise_value = ni->ni_snr;
            ni->ni_noise_stats[ic->bin_number].min_value = ni->ni_noise_stats[ic->bin_number].noise_value;
            ni->ni_noise_stats[ic->bin_number].max_value = ni->ni_noise_stats[ic->bin_number].noise_value;
            ni->ni_noise_stats[ic->bin_number].median_value = ni->ni_noise_stats[ic->bin_number].noise_value;
        }
    }
}

/*
 * @brief description
 *  function executed from the timer context to update the noise stats
 *  like noise value min value, max value and median value
 *  at the end of each traffic rate.
 *
 */
void update_noise_stats(struct ieee80211com *ic)
{
    u_int8_t   *median_array;

    median_array = (u_int8_t *)OS_MALLOC(ic->ic_osdev, (ic->bin_number + 1) * sizeof(u_int8_t), GFP_KERNEL);
    if (median_array == NULL){
        qdf_nofl_info("Memory allocation for median array failed \n");
        return;
    }

    wlan_mlme_iterate_node_list(ic, update_noise_stats_iter_cb,
                                (void *)median_array,
                                IEEE80211_NODE_ITER_F_ASSOC_STA);
    OS_FREE(median_array);
}

/*
 * brief description
 * Timer function which is used to record the noise statistics of each node
 * timer is called ath the end of each traffic rate and is measured until
 * the end of traffic interval
 */
static OS_TIMER_FUNC(ieee80211_noise_stats_update)
{
    struct ieee80211com *ic;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);
    update_noise_stats(ic);
    ic->bin_number++;
    if(ic->bin_number < ic->traf_bins){
        OS_SET_TIMER(&ic->ic_noise_stats,ic->traf_rate * 1000);
    }
}

#if SUPPORT_11AX_D3
void
ieee80211_heop_init(struct ieee80211com * ic) {
    /* In future we may have to fill in the
     * values of individual fields in heop_param and heop_bsscolor_info */
    ic->ic_he.heop_param = 0;
    ic->ic_he.heop_bsscolor_info = 0;
}
#else
void
ieee80211_heop_param_init(struct ieee80211com * ic) {
    /* In future we may have to fill in the
     * values of individual fields in heop_param */
    ic->ic_he.heop_param = 0;
}
#endif

static
uint8_t ieee80211_mbssid_get_vdev_idx_grp(struct mbssid_context *mbss_ctx,
                                    struct mbssid_info *ic_mbss)
{
    uint8_t vdev_idx;

    for (vdev_idx = 0; vdev_idx < mbss_ctx->grp_sz; vdev_idx++) {
        if (ic_mbss->vdev_id_gp[vdev_idx] == IEEE80211_MBSSID_INVALID_VDEV_ID) {
            /* Found free slot for vdev in a vdev_id_gp array */
            break;
        }
    }
    return vdev_idx;
}

#if MESH_MODE_SUPPORT
static
int ieee80211_mbssid_alloc_group_sec(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t grp_id;
    int8_t b_pos;
    uint8_t vdev_idx;

    if (ic->is_mbssid_enabled) {
        mbss_ctx = ic->ic_mbss_ctx;
        if (!mbss_ctx)
            return -EINVAL;

        if (mbss_ctx->grp_sz <= 1)
            return -EINVAL;

        b_pos = qdf_ffz(mbss_ctx->sec_group_bitmap);

        if (b_pos == -1) {
            mbss_err("Invalid bit position from sec_group_bitmap");
            return -EINVAL;
        }

        grp_id = b_pos + mbss_ctx->max_ap_ngroups;

        if (ieee80211_is_invalid_sec_grp_id(mbss_ctx, grp_id)) {
            mbss_err("Invalid group_id for secondary vap derived for secondary vap");
            return -EINVAL;
        }

        vap->iv_mbss.group_id = grp_id;
        ic_mbss = &mbss_ctx->ic_mbss[grp_id];

        /* vdev id assigned to group */
        vdev_idx = ieee80211_mbssid_get_vdev_idx_grp(mbss_ctx, ic_mbss);

        /* Return error if not found free slot in vdev_id_gp array */
        if (vdev_idx >= mbss_ctx->grp_sz) {
            mbss_err("Index:%d reached max value !!No free index for vdev id:%d in vdev_id_gp",
                    vdev_idx, wlan_vdev_get_id(vap->vdev_obj));
            return -EINVAL;
        }

        ic_mbss->vdev_id_gp[vdev_idx] =  wlan_vdev_get_id(vap->vdev_obj);
        ic_mbss->vap_cnt_in_grp++;

        qdf_set_bit(b_pos, (unsigned long *)&mbss_ctx->sec_group_bitmap);
    }
    return 0;
}
#endif

int ieee80211_mbssid_alloc_group(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct mbssid_context *mbss_ctx;

#if MESH_MODE_SUPPORT
    if (vap->iv_mesh_vap_mode) {
        return ieee80211_mbssid_alloc_group_sec(vap);
    }
#endif
if (ic->is_mbssid_enabled || ic->ic_wideband_capable) {
        struct mbssid_info *ic_mbss;
        /* lsb 'n' bit of vap's bssid; 'n' is max-bssid inidicator */
        uint8_t bssid_lsb_n_bit;
        /* pointer to vap bssids */
        uint8_t *vap_bssid = NULL;
        uint8_t grp_id;
        uint8_t vdev_idx;

        mbss_ctx = ic->ic_mbss_ctx;
        if (!mbss_ctx)
            return -EINVAL;

        if (mbss_ctx->grp_sz <=1)
            return -EINVAL;

        vap_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
        bssid_lsb_n_bit = vap_bssid[ATH_BSSID_INDEX] &
                                ((1 << mbss_ctx->max_bssid) - 1);

        /* Algo base assumption is super-group size does not exceed "n=8" */
        /* Determine group-id as follows:
         * 1. mask msb-octet of bssid with n-bit value-mask (if n=4,
         * mask is 0xf)
         * 2. right shift the result with n-bit value of sub-group
         *
         * E.g. -> super_group_n = 3, sub_group_n = 1
         * vap_bssid = 02:03:7f:1a:23:15
         * bssid_lsb_n_bit = 0x15 & 0x7 = b101
         * grp_id = b101 >> 1 = 2
         *
         * similarly,
         * 02:03:7f:1a:23:14 -> 2
         * 02:03:7f:1a:23:15 -> 2
         * 02:03:7f:1a:23:16 -> 3
         * 02:03:7f:1a:23:17 -> 3
         * 02:03:7f:1a:23:18 -> 0
         * 02:03:7f:1a:23:19 -> 0
         * 02:03:7f:1a:23:1a -> 1
         * 02:03:7f:1a:23:1b -> 1
         */

        grp_id  = bssid_lsb_n_bit >> ilog2(mbss_ctx->grp_sz);

        if (grp_id >= mbss_ctx->max_ap_ngroups)
            return -EINVAL;

        ic_mbss = &mbss_ctx->ic_mbss[grp_id];

        /* Assign group_id to vap */
        vap->iv_mbss.group_id = grp_id;

        /* vdev id assigned to group */
        vdev_idx = ieee80211_mbssid_get_vdev_idx_grp(mbss_ctx, ic_mbss);

        /* Return error if not found free slot in vdev_id_gp array */
        if (vdev_idx >= mbss_ctx->grp_sz) {
            mbss_err("Index:%d reached max value !!No free index for vdev id:%d in vdev_id_gp",
                    vdev_idx, wlan_vdev_get_id(vap->vdev_obj));
            return -EINVAL;
        }

        ic_mbss->vdev_id_gp[vdev_idx] =  wlan_vdev_get_id(vap->vdev_obj);

        if (!ic_mbss->vap_cnt_in_grp)
            mbss_ctx->active_ap_ngroups++;

        ic_mbss->vap_cnt_in_grp++;

        mbss_debug("vdev_idx:%d vdev id:%d vap_cnt_in_grp:%d",
                vdev_idx, ic_mbss->vdev_id_gp[vdev_idx], ic_mbss->vap_cnt_in_grp);
    }

    return 0;
}

/**
 * ieee80211_mbssid_get_num_vaps_in_mbss_cache(): Retrieve total number
 * of vaps that is occupying a cache-entry - lock held context. User
 * must ensure that this API is not called under the same lock
 * @ic - ic pointer
 *
 * return: number of vaps in mbss-cache
 */
uint8_t ieee80211_mbssid_get_num_vaps_in_mbss_cache(struct ieee80211com *ic,
                                                    uint8_t group_id)
{
    uint8_t num_vaps = 0;

    if (ic->is_mbssid_enabled) {
        struct mbssid_context *mbss_ctx;
        struct mbssid_info *ic_mbss;

        mbss_ctx = ic->ic_mbss_ctx;
        if (!mbss_ctx)
            return num_vaps;

        ic_mbss = &mbss_ctx->ic_mbss[group_id];
        /* acquire mbss_cache_lock */
        qdf_spin_lock_bh(&ic_mbss->mbss_cache_lock);
        num_vaps = ic_mbss->num_vaps;
        /* release mbss cache lock */
        qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
        mbss_debug("num_vaps: %d", num_vaps);
    }

    return num_vaps;
}

uint8_t ieee80211_mbssid_get_num_vaps_across_ap_mbss_caches(struct ieee80211com *ic)
{
    uint8_t num_vaps = 0;

    if (ic->is_mbssid_enabled) {
        struct mbssid_context *mbss_ctx;
        uint8_t grp_id;

        mbss_ctx = ic->ic_mbss_ctx;
        if (!mbss_ctx)
            return num_vaps;

        for(grp_id = 0; grp_id < mbss_ctx->max_ap_ngroups; grp_id++) {
            num_vaps +=
                (ieee80211_mbssid_get_num_vaps_in_mbss_cache(ic, grp_id));
        }
    }
    return num_vaps;
}

void ieee80211_mbssid_update_mbssie_cache_entry(
        struct ieee80211vap *vap, uint8_t cache_entry)
{
    struct ieee80211com  *ic                            = vap->iv_ic;
    struct vdev_mlme_obj *vdev_mlme                     = vap->vdev_mlme;
    struct ieee80211_mbss_ie_cache_node *node           = NULL;
    struct ieee80211_mbss_non_tx_profile_sub_ie *ntx_pf = NULL;
    bool is_mbssid_enabled = ic->is_mbssid_enabled;
    bool is_tx_vap         = !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap);
    uint8_t grp_id         = vap->iv_mbss.group_id;
    const uint8_t ie_header_len = sizeof(struct ieee80211_ie_header);
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint32_t node_idx;

#if MBSS_CACHE_ENTRY_DEBUG
    uint8_t *p;
#endif

    if (!ic->ic_mbss_ctx) {
        mbss_debug("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
        return;
    }

    if (is_mbssid_enabled) {
        mbss_debug(":> vdev_id: %d cache_entry: %d",
                                vap->iv_unit, cache_entry);
        mbss_debug(":> is_tx_vap: %d ic_mbss->rot_factor: %d",
                                is_tx_vap, ic_mbss->rot_factor);

        /* Acquire mbss_cache_lock */
        qdf_spin_lock_bh(&ic_mbss->mbss_cache_lock);

        if (is_tx_vap) {
            node_idx = ieee80211_mbssid_get_tx_vap_node_idx(ic,
                       grp_id);
        } else {
            node_idx = ieee80211_mbssid_get_non_tx_vap_node_idx(vap);
        }

        if (node_idx >= mbss_ctx->grp_sz) {
            mbss_err("Invalid node_idx: %d", node_idx);
            /* Release mbss cache lock */
            qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
            return;
        }

        mbss_debug("node_idx: %d", node_idx);
        node        = &((struct ieee80211_mbss_ie_cache_node *)
                                     ic_mbss->mbss_cache)[node_idx];
        ntx_pf      = &node->non_tx_profile.ntx_pf;

        switch (cache_entry) {
            case MBSS_CACHE_ENTRY_POS:
            break;
            case MBSS_CACHE_ENTRY_CAP:
            {
                ieee80211_add_capability(
                        ntx_pf->cap_elem.non_tx_cap, vap->iv_bss);
            }
            break;
            case MBSS_CACHE_ENTRY_SSID:
            {
                void *start_of_relocatable_bytes;
                struct __ieee80211_mbss_ie_ssid *ssid_elem
                                                = &ntx_pf->ssid_elem;
                const uint8_t target_ssid_len   =
                                vdev_mlme->mgmt.generic.ssid_len;
                uint8_t target_ssid[WLAN_SSID_MAX_LEN + 1] = {0};
                const uint8_t offset_to_ssid_in_node =
                                ie_header_len +
                                sizeof(struct ieee80211_mbss_ie_capability);
                const uint8_t no_of_relocatable_bytes_in_node =
                                IEEE80211_MAX_NON_TX_PROFILE_SIZE -
                                (offset_to_ssid_in_node +
                                 /* ssid header */
                                 ie_header_len +
                                 target_ssid_len);

                mbss_debug("no_of_relocatable_bytes_in_node: %d",
                                    no_of_relocatable_bytes_in_node);

                /* Copy target_ssid */
                qdf_mem_copy(target_ssid,
                        vdev_mlme->mgmt.generic.ssid, target_ssid_len);

                /* Create hole of size ssid_elem->hdr.length.
                 * refer to the defn of
                 * struct ieee80211_mbss_ie_cache_node.
                 * ssid field in the ssid element is defined
                 * as a flexible array and hence a hole is
                 * required to be created to avoid overwriting
                 * data written previously
                 */
                if (!ssid_elem->hdr.length) {
                    start_of_relocatable_bytes = ssid_elem->ssid + 1;
                    if (*ssid_elem->ssid == IEEE80211_ELEMID_MBSSID_INDEX) {
                        /* In the case the cache entry was already updated
                         * for Hidden SSID case, the SSID element will only
                         * be 2bytes long moving the MBSSID index IE by 1byte.
                         * Re-adjust the start of relocatable bytes accordingly.
                         */
                        start_of_relocatable_bytes--;
                    }
                } else {
                    start_of_relocatable_bytes = ssid_elem->ssid +
                                                    ssid_elem->hdr.length;
                }

                if (!IEEE80211_VAP_IS_HIDESSID_ENABLED(vap)) {
                    if (target_ssid_len && target_ssid_len <= 32) {

                        /* Relocate existing bytes in node to make
                         * adjustment for ssid based flexible array
                         */
                        qdf_mem_move(ssid_elem->ssid +
                                     target_ssid_len,
                                     start_of_relocatable_bytes,
                                     no_of_relocatable_bytes_in_node);

                        /* Copy target-ssid in the accommodated space */
                        qdf_mem_copy(ssid_elem->ssid,
                                     target_ssid, target_ssid_len);
                        mbss_debug("ssid: %s", target_ssid);

                        /* Subtract the current ssid length from
                         * the total non-tx profile size
                         */
                        if (ssid_elem->hdr.length)
                            ntx_pf->sub_elem.length -= ssid_elem->hdr.length;

                        /* Update the ssid sub ie length in non-tx
                         * profile
                         */
                        ssid_elem->hdr.length = target_ssid_len;

                        /* Updated non-tx profile subelement
                         * header length */
                        ntx_pf->sub_elem.length += target_ssid_len;
                    } /* if (target_ssid_len && target_ssid_len <= 32) */
                } else {
                    mbss_debug("Hidden ssid enabled case");

                    /* Subtract the current ssid length from
                     * the total non-tx profile size
                     */
                    if (ssid_elem->hdr.length) {
                        ntx_pf->sub_elem.length -= ssid_elem->hdr.length;
                        ssid_elem->hdr.length = 0;
                    }

                    /* Copy the relocatable bytes starting from the ssid
                     * field as there is no ssid byte in case of hidden-
                     * ssid
                     */
                    qdf_mem_copy(ssid_elem->ssid,
                                    start_of_relocatable_bytes,
                                        no_of_relocatable_bytes_in_node);
                } /* End if (!IEEE80211_VAP_IS_HIDESSID_ENABLED(vap)) */
            }
            break;
            case MBSS_CACHE_ENTRY_IDX:
            {
#define MBSS_OFFSET_TO_DTIM_PERIOD_FIELD 1
#define MBSS_OFFSET_TO_DTIM_COUNT_FIELD  2
                uint8_t *bufp;

                if (ntx_pf->ssid_elem.hdr.length) {
                    bufp =  (uint8_t *) &ntx_pf->ssid_elem;
                    bufp += (ie_header_len +
                            ntx_pf->ssid_elem.hdr.length + ie_header_len);
                }
                else {
                    /* In the defn of struct __ieee80211_mbss_ie_ssid,
                     * after ie_header[tag 1 byte + length 1 byte], ssid[1]
                     * array holds 1 byte, in case of hidden ssid, subsequent
                     * IE bytes are relocated/left shift back by 1 byte
                     * i.e ssid[1] holds the IE tag id if next subelement IE
                     * Multiple BSSID index element tag
                     */
                    bufp = (uint8_t *) &ntx_pf->idx_elem.bssid_idx;
                    if (*(ntx_pf->ssid_elem.ssid) == IEEE80211_ELEMID_MBSSID_INDEX) {
                        /* In the case the cache entry was already updated
                         * for Hidden SSID case, the SSID element will only
                         * be 2bytes long. Moving the MBSSID index IE by 1byte.
                         */
                        bufp--;
                    }
                }

                *bufp = vdev_mlme->mgmt.mbss_11ax.profile_idx & 0xff;

                /* Update dtim-period field */
                *(bufp + MBSS_OFFSET_TO_DTIM_PERIOD_FIELD) =
                                        vdev_mlme->proto.generic.dtim_period;

                /* Update dtim-count field - fw requires the dtim count
                 * value to be 0 if the non-tx profile already exists in
                 * the beacon template. If a non-tx profile is being added
                 * for the first time then fw expects the dtim-count value
                 * to be 255 for that particular profile; so, we keep the
                 * dtim-count as 255 if the vap is in down state
                 */
                if (ieee80211_is_vap_state_running(vap))
                    *(bufp + MBSS_OFFSET_TO_DTIM_COUNT_FIELD) = 0;
                else
                    *(bufp + MBSS_OFFSET_TO_DTIM_COUNT_FIELD) = 255;

                mbss_debug("bssid_idx: %d dtim_period: %d"
                          " dtim_count: %d", *bufp,
                          *(bufp + MBSS_OFFSET_TO_DTIM_PERIOD_FIELD),
                          *(bufp + MBSS_OFFSET_TO_DTIM_COUNT_FIELD));
            }
            break;
            default:
                mbss_err("unknown cache_entry id: %d", cache_entry);
        } /* End switch */

#if MBSS_CACHE_ENTRY_DEBUG
        if (ntx_pf->ssid_elem.hdr.length) {
            p = (uint8_t *) &ntx_pf->ssid_elem;
            p += (2 + ntx_pf->ssid_elem.hdr.length + 2);
        }
        else
            p = (uint8_t *) &ntx_pf->idx_elem.bssid_idx;
        mbss_debug("bssid_idx: %d dtim_period: %d",*p, *(p + 1));
        mbss_debug("ntx_pf->sub_elem.length: %d", ntx_pf->sub_elem.length);
        mbss_debug("beacon_pos: %d", node->pos);
#endif

        /* Release mbss cache lock */
        qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
    } /* End if mbss_feature check block */

    mbss_debug(":<");
}

uint8_t ieee80211_mbssid_get_bssidx(struct ieee80211com *ic, uint8_t id)
{
    if (ic->ic_is_macreq_ext_active(ic))
        return id;

    return MBSSID_GET_BSSIDX(ic);
}

int ieee80211_mbssid_update_mbssie_cache(struct ieee80211vap *vap, bool add)
{
    struct ieee80211com  *ic                            = vap->iv_ic;
    struct vdev_mlme_obj *vdev_mlme                     = vap->vdev_mlme;
    struct ieee80211_mbss_ie_cache_node *node           = NULL;
    struct ieee80211_mbss_non_tx_profile_sub_ie *ntx_pf = NULL;
    struct ol_ath_softc_net80211 *scn;
    ol_ath_ema_pdev_ctx_t *ema_ctx;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint32_t node_idx;
    uint8_t grp_id = vap->iv_mbss.group_id;
    bool is_tx_vap;
    const size_t ie_header_len = sizeof(struct ieee80211_ie_header);

    mbss_debug(":>%d", add);

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
        mbss_err("Not AP mode. Return");
        return -1;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    ema_ctx = &scn->ema_ctx;

    is_tx_vap = !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap);
    /* If it is currently marked as non-Tx vap and if currently
     * no Tx-vap exists then check whether bssid-idx is invalid or
     * not. If invalid then we need to retrieve and clear the
     * corresponding node as Tx-vap as is not assigned a valid
     * bssid-idx yet. A valid bssid-idx will get assigned to this
     * vap only when a new Tx-vap is selected by user
     */
    if (!is_tx_vap) {
        if ((!ic_mbss->transmit_vap) &&
                (vdev_mlme->mgmt.mbss_11ax.profile_idx
                == IEEE80211_INVALID_MBSS_BSSIDX(ic))) {
            is_tx_vap = true;
        }
    }

    if (ic->is_mbssid_enabled) {
        /* acquire mbss_cache_lock */
        qdf_spin_lock_bh(&ic_mbss->mbss_cache_lock);

        if (is_tx_vap) {
            node_idx =
                    ieee80211_mbssid_get_tx_vap_node_idx(ic, grp_id);
        } else {
            node_idx =
                    ieee80211_mbssid_get_non_tx_vap_node_idx(vap);
        }

        if (node_idx >= mbss_ctx->grp_sz) {
            mbss_err("Invalid node_idx: %d", node_idx);
            /* release mbss_cache_lock */
            qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
            return -1;
        }

        mbss_debug("node_idx: %d", node_idx);

        node = &((struct ieee80211_mbss_ie_cache_node *)
                             ic_mbss->mbss_cache)[node_idx];
        ntx_pf = &node->non_tx_profile.ntx_pf;

        if (add) {
            if (node->used) {
                mbss_warn("mbss_ie_cache_node corresponding"
                        "to vdev_id: %d is already ocuppied",
                            wlan_vdev_get_id(vap->vdev_obj));
                /* release mbss_cache_lock */
                qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
                return -1;
            }

            /* position is already assigned at cache create
             * time during attach. By design position once
             * assigned does not change for the lifetime of
             * the node
             */
            node->used      = true;
            node->group_id  = vap->iv_mbss.group_id;
            node->vdev_id   = wlan_vdev_get_id(vap->vdev_obj);
            mbss_debug("node_idx: %d vdev_id: %d group_id:%d", node_idx, node->vdev_id, node->group_id);
            mbss_debug("beacon_pos: %d", node->pos);

            /* assign non-tx subelement id */
            ntx_pf->sub_elem.element_id
                            = IEEE80211_MBSSID_SUB_ELEMID;

            ntx_pf->cap_elem.hdr.element_id
                            = IEEE80211_ELEMID_MBSSID_NON_TRANS_CAP;
            /* initialize length field accommodating
             * 16 bit long basic capability field
             */
            ntx_pf->cap_elem.hdr.length = 2;

            /* ssid is assigned to a vap at the time
             * of start_ap call from HOSTAPD. Defer
             * the ssid assignment till then
             */
            ntx_pf->ssid_elem.hdr.element_id = IEEE80211_ELEMID_SSID;
            ntx_pf->ssid_elem.hdr.length = 0;
            *(ntx_pf->ssid_elem.ssid) = 0;

            ntx_pf->idx_elem.hdr.element_id
                            = IEEE80211_ELEMID_MBSSID_INDEX;
            /* initialize length field accommodating
             * subfields bssidx, dtim period and
             * dtim count each of length one octet
             */
            ntx_pf->idx_elem.hdr.length = 3;
            ntx_pf->idx_elem.bssid_idx
                            = vdev_mlme->mgmt.mbss_11ax.profile_idx;
            ntx_pf->idx_elem.dtim_period
                            = vdev_mlme->proto.generic.dtim_period;
            /* initialize to 255 as fw requires
             * dtim-count value to be 255 for a
             * non-tx profile that gets added to
             * a beacon template for the first
             * time; the corresponding value is
             * 0 if the profile already exists
             * in fw
             */
            ntx_pf->idx_elem.dtim_count = 255;

            ntx_pf->sub_elem.length
                            = ie_header_len + ntx_pf->cap_elem.hdr.length +
                              ie_header_len + ntx_pf->ssid_elem.hdr.length +
                              ie_header_len + ntx_pf->idx_elem.hdr.length;

            /* update num_vaps */
            ic_mbss->num_vaps++;
            if (ic_mbss->num_vaps > mbss_ctx->grp_sz) {
                mbss_err("Invalid number of mbss vaps %d - This should be less than group size %d", ic_mbss->num_vaps, mbss_ctx->grp_sz);
                qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
                return -1;
            }
        } else {
            uint8_t *bssid;
            /* Mark cache-entry as stale as the vap occupying
             * this particular entry is getting deleted
             */
            node->used = false;

            if (ic_mbss->num_vaps) {
                /* Update num_vaps */
                ic_mbss->num_vaps--;

                /* If this was the last ap vap in MBSSID mode
                 * then re-init relevant params anticipating
                 * new MBSSID group creation initialted by
                 * user subsequently
                 */
                qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
                if (!ieee80211_mbssid_get_num_vaps_across_ap_mbss_caches(ic)) {
                    /* Reassign ref_bssid if all ap-vaps are
                     * deleted
                     */
                    if (ic->ic_assign_mbssid_ref_bssid) {
                        /* Do not request partially random ref_bssid
                         * as we would like to adhere to base mac
                         * address (that assigned to the radio/physical
                         * interface) even after the MBSSID group is
                         * dissolved
                         */
                        ic->ic_assign_mbssid_ref_bssid(scn, false);
                    }

                    /* Reset rotation factor */
                    ic_mbss->rot_factor = 0;
                }
                qdf_spin_lock_bh(&ic_mbss->mbss_cache_lock);
            } /* end if (ic_mbss->num_vaps) */

            /* Retrieve non-tx vap bssid */
            wlan_vdev_obj_lock(vap->vdev_obj);
            bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
            wlan_vdev_obj_unlock(vap->vdev_obj);

            /* If the bssid of the vap being deleted is the
             * ref-bsssid then mark ref-bssid as unused
             */
            if (IEEE80211_ADDR_EQ(mbss_ctx->ref_bssid, bssid)) {
                ic->is_ref_bssid_used = false;
            }
        }

        /* release mbss_cache_lock */
        qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);

        mbss_debug("ntx_pf->sub_elem.length: %d used: %d",
                ntx_pf->sub_elem.length, node->used);
    } /* endif if (mbssid_enabled) */

    return 0;
}

static int ieee80211_mbssid_assign_and_sanitize_sub_grp_bssid_idx(
                                    struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;
    /* pointers to tx and non-tx vap bssids */
    uint8_t *tx_bssid = NULL, *non_tx_bssid;
    /* lsb n bit of tx vap; n is max bssid inidicator */
    uint8_t bssid_lsb_n_bit_tx;
    /* lsb n bit of non-tx vap; n is max bssid inidicator */
    uint8_t bssid_lsb_n_bit_non_tx;
    uint8_t bssid_idx;
    bool is_bssid_idx_occupied = false;
    int status = 0;

    if (!ic) {
        mbss_err("ic is null");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("ic_mbss_ctx is null");
        return -1;
    }

    if (ic->is_mbssid_enabled) {
        struct mbssid_context *mbss_ctx;
        struct mbssid_info *ic_mbss;
        uint8_t grp_id = vap->iv_mbss.group_id;

        mbss_ctx = ic->ic_mbss_ctx;
        if (!mbss_ctx)
            return -EINVAL;

        ic_mbss = &mbss_ctx->ic_mbss[grp_id];

        /* Retrieve tx-vap bssid if not already done so */
        tx_bssid = ic_mbss->tx_bssid;
        if (!tx_bssid)
            return -EINVAL;

        /* Retrieve non-tx vap bssid */
        wlan_vdev_obj_lock(vap->vdev_obj);
        non_tx_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
        wlan_vdev_obj_unlock(vap->vdev_obj);

        mbss_debug("tx vap bssid: %s", ether_sprintf(tx_bssid));
        mbss_debug("non-tx vap bssid: %s : ic_mbss->bssid_index_bmap : 0x%lx",
                ether_sprintf(non_tx_bssid), ic_mbss->bssid_index_bmap);

        /* Retrieve n bits of lsb octet of tx vap */
        bssid_lsb_n_bit_tx = tx_bssid[ATH_BSSID_INDEX] &
                                        (MBSSID_SUB_GROUP_SIZE(ic) - 1);
        /* Retrieve n bits of lsb octet of non-tx vap */
        bssid_lsb_n_bit_non_tx = non_tx_bssid[ATH_BSSID_INDEX] &
                                        (MBSSID_SUB_GROUP_SIZE(ic) - 1);
        /* Get new bssid_idx of non-tx vap wrt. new tx vap */
        IEEE80211_MBSSID_GET_BSSID_IDX(bssid_lsb_n_bit_tx,
                                       bssid_lsb_n_bit_non_tx,
                                       bssid_idx, false);

        /* Sanitize the bssid_idx here,
         * 1. bssid_idx is in range [1..2^n-1]
         * 2. bssid_idx is not already occupied in pool
         */
        if (!bssid_idx || (bssid_idx > (MBSSID_SUB_GROUP_SIZE(ic) - 1))) {
            mbss_err("bssid_idx(sub_grp): %d found to be outside valid range"
                     " - check non-tx vap bssid: %s",
                     bssid_idx, ether_sprintf(non_tx_bssid));
            return -1;
        }

        is_bssid_idx_occupied = qdf_test_bit((bssid_idx - 1),
                                            &ic_mbss->bssid_index_bmap);
        if (is_bssid_idx_occupied) {
            mbss_err("bssid_idx(sub_grp) already occupied. Non-tx vap"
                     " create failed");
            return -1;
        }

        /* Mark the bssid_idx as ocupied in the bssid_idx pool */
        qdf_set_bit(bssid_idx - 1,
                (unsigned long *) &ic_mbss->bssid_index_bmap);

        vdev_mlme->mgmt.mbss_11ax.profile_idx = bssid_idx;
        mbss_info("vdev_id: %d bssid_idx(sub_grp) is: %d",
                vap->iv_unit, bssid_idx);
    }

    return status;
}

static int ieee80211_mbssid_assign_and_sanitize_super_grp_bssid_idx(
                                    struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    /* pointers to ref bssid and non-tx vap bssids */
    uint8_t *ref_bssid = NULL, *non_tx_bssid;
    /* lsb n bit of ref-bssid; n is max bssid inidicator */
    uint8_t bssid_lsb_n_bit_ref;
    /* lsb n bit of non-tx vap; n is max bssid inidicator */
    uint8_t bssid_lsb_n_bit_non_tx;
    uint8_t bssid_idx;
    bool is_bssid_idx_occupied = false;
    struct mbssid_context *mbss_ctx;

    if (!ic) {
        mbss_err("ic is null");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("ic_mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;

    if(ieee80211_ema_is_sec_group(mbss_ctx, vap->iv_mbss.group_id)) {
       mbss_debug("Skip mbssid assign & sanitize super grp bssid idx for sec group");
       return 0;
    }

    /* Retrieve reference bssid of super-group */
    ref_bssid = mbss_ctx->ref_bssid;

    /* Retrieve non-tx vap bssid */
    wlan_vdev_obj_lock(vap->vdev_obj);
    non_tx_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
    wlan_vdev_obj_unlock(vap->vdev_obj);

    mbss_debug("reference bssid: %s", ether_sprintf(ref_bssid));
    mbss_debug("non-tx vap bssid: %s", ether_sprintf(non_tx_bssid));
    mbss_debug("mbss_ctx->bssid_index_bmap: 0x%lx", mbss_ctx->bssid_index_bmap);

    /* Retrieve n-bits of lsb octet of ref_bssid */
    bssid_lsb_n_bit_ref = ref_bssid[ATH_BSSID_INDEX] &
                                ((1 << mbss_ctx->max_bssid) - 1);
    /* Retrieve n-bits of lsb octet of non-tx vap */
    bssid_lsb_n_bit_non_tx = non_tx_bssid[ATH_BSSID_INDEX] &
                                    ((1 << mbss_ctx->max_bssid) - 1);

    /* Get new bssid_idx of non-tx vap wrt. super-group */
    IEEE80211_MBSSID_GET_BSSID_IDX(bssid_lsb_n_bit_ref,
                                   bssid_lsb_n_bit_non_tx,
                                   bssid_idx, true);

    /* Sanitize the bssid_idx here,
     * 1. bssid_idx is in range [1..2^n-1]
     * 2. bssid_idx is not already occupied in pool
     */
    if (!bssid_idx || (bssid_idx > (MBSSID_SUPER_GROUP_SIZE(ic)))) {
        mbss_err("bssid_idx(super_grp): %d found to be outside valid range"
                 " - check non-tx vap bssid: %s",
                 bssid_idx, ether_sprintf(non_tx_bssid));
        return -1;
    }

    is_bssid_idx_occupied = qdf_test_bit((bssid_idx - 1),
                                    &mbss_ctx->bssid_index_bmap);
    if (is_bssid_idx_occupied) {
        mbss_err("bssid_idx(super_grp) already occupied."
                 " Non-tx vap create failed");
        return -1;
    }

    /* Mark the bssid_idx as ocupied in the bssid_idx pool */
    qdf_set_bit(bssid_idx - 1,
                (unsigned long *) &mbss_ctx->bssid_index_bmap);

    /* Assign super-group bssid_idx to non-tx vap */
    vap->iv_mbss.bssid_idx_in_super_grp = bssid_idx;

    mbss_info("vdev_id: %d bssid_idx(super_grp) is: %d", vap->iv_unit, bssid_idx);
    return 0;
}

static void ieee80211_mbssid_init_vap_resource_profile(struct ieee80211vap *vap,
                                                       ol_ath_soc_softc_t *soc,
                                                       int mbssid_idx)
{
    int ven_ie_size = 0;
    struct ieee80211com *ic   = vap->iv_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;
    /* FR69640: uint8_t group_id = vap->iv_mbss.group_id; */
    bool is_mbssid_enabled =
        wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj, WLAN_PDEV_F_MBSS_IE_ENABLE);

    if(!is_mbssid_enabled) {
        qdf_debug("MBSSID disabled, no max-pp calc"
                  " or beacon pos mapping necessary");
        return;
    }

    if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
        if (!ema_ctx->support_wps_6ghz || !ema_ctx->ext_enabled) {
            if (mbssid_idx < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) {
                ven_ie_size = IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                ema_ctx->vendor_ie_config_low, mbssid_idx);
            } else {
                ven_ie_size = IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                ema_ctx->vendor_ie_config_high,
                                (mbssid_idx - IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX));
            }
        }

        /* Override for WPS */
        if (ema_ctx->support_wps_6ghz)
             ven_ie_size = ema_ctx->vendor_ie_size_limit;
    } else {
        ven_ie_size = IEEE80211_MAX_VENDOR_IE_SIZE_LIMIT;
    }

    vap->iv_mbss.total_vendor_ie_size = ven_ie_size;
    vap->iv_mbss.available_vendor_ie_space = ven_ie_size;
    vap->iv_mbss.total_optional_ie_size = ema_ctx->optional_ie_size;
    vap->iv_mbss.available_bcn_optional_ie_space = ema_ctx->optional_ie_size;
    vap->iv_mbss.available_prb_optional_ie_space = ema_ctx->optional_ie_size;
    return;
}

static bool ieee80211_mbssid_is_default_tx_vap_assigned_grp(struct ieee80211vap *vap)
{
    struct mbss_iter_func_args iter_func_args = {0};
    iter_func_args.purpose = MBSS_ITER_FUNC_PURPOSE_CHECK_DEFAULT_TX_VAP;
    iter_func_args.retval = QDF_STATUS_SUCCESS;
    iter_func_args.args = NULL;
    iter_func_args.is_default_tx_vap = 0;

    wlan_mbss_group_iterate_vdev_list(vap->iv_ic, vap->iv_mbss.group_id, &iter_func_args);

    if (iter_func_args.is_default_tx_vap) {
        return true;
    } else {
        return false;
    }
}
uint8_t ieee80211_cohosted_get_bssidx(struct ieee80211com *ic, uint8_t id)
{
    if (ic->ic_is_macreq_ext_active(ic))
        return id;

    return COHOSTED_GET_BSSIDX(ic);
}

int ieee80211_cohosted_bssid_setup(struct ieee80211vap *vap)
{
    struct ieee80211com *ic;
    uint32_t vdev_id;

    ic = vap->iv_ic;
    if(!ic) {
       qdf_err("ic is NULL");
       return -EINVAL;
    }

    vdev_id = wlan_vdev_get_id(vap->vdev_obj);

    if (!ieee80211_get_num_beacon_ap_vaps(ic)) {
        qdf_mem_zero((void *) &ic->ic_cohosted_bssid_index_bmap,
                sizeof(ic->ic_cohosted_bssid_index_bmap));
        QDF_TRACE(QDF_MODULE_ID_COHOSTED_BSS, QDF_TRACE_LEVEL_DEBUG, "COHOSTED: Intialize"
                  " ic_cohosted_bssid_index_bmap VAP id (%d)",vdev_id);
    }

    vap->cohosted_bss_idx = qdf_ffz(ic->ic_cohosted_bssid_index_bmap);
    if (qdf_unlikely(vap->cohosted_bss_idx == IEEE80211_INVALID_COHOSTED_BSSIDX)) {
        QDF_TRACE(QDF_MODULE_ID_COHOSTED_BSS, QDF_TRACE_LEVEL_ERROR, "COHOSTED BSSID index bitmap full VAP Id:%d",
                  vdev_id);
        return -1;
    }

    qdf_set_bit(vap->cohosted_bss_idx,
                (unsigned long *) &ic->ic_cohosted_bssid_index_bmap);

    QDF_TRACE(QDF_MODULE_ID_COHOSTED_BSS, QDF_TRACE_LEVEL_DEBUG, "COHOSTED: VAP Id(%d) with BSSID index:%d",
              vdev_id,vap->cohosted_bss_idx);

    return 0;
}
qdf_export_symbol(ieee80211_cohosted_bssid_setup);

int ieee80211_mbssid_setup(struct ieee80211vap *vap)
{
    struct ieee80211com *ic;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;
    struct wlan_objmgr_psoc *psoc;
    ol_ath_soc_softc_t *soc;
    struct ol_ath_softc_net80211 *scn;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t vap_count = 0;
    int ret = 0;
    bool is_sec = false;
    bool is_default_tx_vdev = false;

    ic = vap->iv_ic;
    if(!ic) {
        qdf_err("ic is NULL");
        return -EINVAL;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (psoc == NULL) {
        qdf_err("psoc is null");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn NULL");
        return -EINVAL;
    }

    soc = scn->soc;
    if (!soc) {
        qdf_err("Soc NULL");
        return -EINVAL;
    }

    /* If ieee80211_mbss_attach() or any of the core buffer alloc failed,
     * disallow VAP create
     */
    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss context is null");
        return -EINVAL;
    }
    mbss_ctx = ic->ic_mbss_ctx;

    ret = ieee80211_mbssid_alloc_group(vap);
    if (ret) {
        mbss_err("group alloc failed ret:%d ",ret);
        ret = -1;
        goto exit;
    }

    ic_mbss = &mbss_ctx->ic_mbss[vap->iv_mbss.group_id];
    if (!ic_mbss->mbss_cache) {
        mbss_err(" null mbss_cache ");
        ret = -1;
        goto exit;
    }

    is_sec = ieee80211_ema_is_sec_group(mbss_ctx, vap->iv_mbss.group_id);

    if (!is_sec &&
        (!ic_mbss->bcn_bo_mbss_ie || !ic_mbss->prb_po_mbss_ie)) {
        mbss_err("null bcn_bo_mbss_ie or prb_po_mbss_ie ");
        ret = -1;
        goto exit;
    }

    /* Treat first VAP created as the virtual tx-VAP
     * till user designates a tx-VAP
     */
    vap_count = ieee80211_mbssid_get_num_vaps_in_mbss_cache(ic,
                    vap->iv_mbss.group_id);
    if (vap_count) {
        /* Check if any vap is already marked as
         * default Tx-vap in this group
         */
        is_default_tx_vdev = ieee80211_mbssid_is_default_tx_vap_assigned_grp(vap);
    }

    if (!is_default_tx_vdev) {
        /* bssid of tx-vap */
        uint8_t *tx_bssid;

        vap->iv_mbss.bssid_idx_in_super_grp =
                                        IEEE80211_INVALID_MBSS_BSSIDX(ic);
        vdev_mlme->mgmt.mbss_11ax.profile_idx =
                                        IEEE80211_INVALID_MBSS_BSSIDX(ic);

        if (!wlan_psoc_nif_fw_ext_cap_get(psoc,
                    WLAN_SOC_CEXT_MBSS_PARAM_IN_START)) {
            ic_mbss->transmit_vap = vap;
            ic->ic_wifi_down_ind = 0;
            mbss_ctx->non_inherit_enable = 0;
            ic_mbss->prb_req_ssid_match_vap = NULL;
        }

        /* Assign group's tx-bssid */
        wlan_vdev_obj_lock(vap->vdev_obj);
        tx_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
        wlan_vdev_obj_unlock(vap->vdev_obj);

        if (!IEEE80211_ADDR_EQ(ic_mbss->tx_bssid, tx_bssid)) {
               /* Mark vap as tx-vap */
            IEEE80211_VAP_MBSS_NON_TRANS_DISABLE(vap);

            /* Reset the bssid-idx pool */
            qdf_mem_zero((void *)&ic_mbss->bssid_index_bmap, sizeof(unsigned long));

            /* Get cumulative rotation factor */
            ieee80211_mbssid_get_cumulative_rot_factor(vap);

            /* Refactor bssid idxes and bssid idx-pool with respect to
             * the bssid of the new tx vap
             */
            ieee80211_mbssid_refactor_bssid_idx(vap);

            /* Reassign tx_bssid */
            IEEE80211_ADDR_COPY(ic_mbss->tx_bssid, tx_bssid);
            /* Last node in MBSS cache will always occupy the beacon
             * position of the slot that the tx-vap held before getting
             * designated as the new tx-vap
             */
            ieee80211_mbssid_reassign_bcn_pos_of_last_mbss_node(vap);

        }
        mbss_info("tx_bssid: %s",ether_sprintf(ic_mbss->tx_bssid));
        IEEE80211_VAP_MBSS_NON_TRANS_DISABLE(vap);
        ieee80211_mbssid_init_vap_resource_profile(vap, soc,
                               IEEE80211_INVALID_MBSS_BSSIDX(ic));

        if (!IEEE80211_ADDR_EQ(mbss_ctx->ref_bssid, tx_bssid)) {
            if (ieee80211_mbssid_assign_and_sanitize_super_grp_bssid_idx(vap)) {
            mbss_err("failed super_grp_bssid_idx for tx vap : grp id :%d",
                    vap->iv_mbss.group_id);
                ret = -1;
                goto exit;
            }
        }

        mbss_info("Added default Tx-VAP - vdev_id: %d,"
                  " max_bssid_indicator:%d group_id: %d",
                    wlan_vdev_get_id(vap->vdev_obj), ic_mbss->max_bssid,
                    vap->iv_mbss.group_id);
    } else {
        /* bssid of non-tx vap */
        uint8_t *non_tx_bssid;

        /* This block should not come for secondary group of vaps
         * as there can be only 1 vap allowed in the group */
        qdf_assert(!is_sec);
        if (vap_count >= MBSSID_SUB_GROUP_SIZE(ic)) {
            mbss_err("Failed to create non-tx vap."
                     " Sub-group: %d size overflow",
                     vap->iv_mbss.group_id);
            ret = -1;
            goto exit;
        }

        if (vap_count > ic_mbss->max_non_transmit_vaps) {
            mbss_err("No space for non-Tx vap in tx-beacon"
                     " non-Tx vap not created Sub-group: %d",
                    vap->iv_mbss.group_id);
            ret = -1;
            goto exit;
        }

        vap_count = ieee80211_mbssid_get_num_vaps_across_ap_mbss_caches(ic);
        if (vap_count >= MBSSID_SUPER_GROUP_SIZE(ic)) {
            mbss_err("Failed to create non-tx vap."
                     " Super-group: %d size overflow",
                     vap->iv_mbss.group_id);
            ret = -1;
            goto exit;
        }

        wlan_vdev_obj_lock(vap->vdev_obj);
        non_tx_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
        wlan_vdev_obj_unlock(vap->vdev_obj);

        if (!IEEE80211_ADDR_EQ(mbss_ctx->ref_bssid, non_tx_bssid)) {
            /* Allocate a bssid_idx from bssid-idx pool if the bssid_idx
             * corresponding to the bssid of this non-tx vap is not alre
             * ady occupied. Fail vap create if bssid_idx is not found
             * free in bssid-idx pool
             */
            if (ieee80211_mbssid_assign_and_sanitize_super_grp_bssid_idx(vap)) {
                ret = -1;
                goto exit;
            }
        }

        if (ieee80211_mbssid_assign_and_sanitize_sub_grp_bssid_idx(vap)) {
            ret = -1;
            goto exit;
        }

        /* Mark vap as non-tx vap */
        IEEE80211_VAP_MBSS_NON_TRANS_ENABLE(vap);

        /* Init resource-profile */
        ieee80211_mbssid_init_vap_resource_profile(vap, soc,
                            (vdev_mlme->mgmt.mbss_11ax.profile_idx +
                             vap->iv_mbss.group_id * mbss_ctx->grp_sz - 1));

        mbss_info("Added non-transmitting VAP - vdev_id: %d"
                  " with bssid-idx(sub-grp):%d"
                  " group_id: %d",
                  wlan_vdev_get_id(vap->vdev_obj), vdev_mlme->mgmt.mbss_11ax.profile_idx,
                  vap->iv_mbss.group_id);
    }

    /* Allocate node from mbssie_cache */
    ret = ieee80211_mbssid_update_mbssie_cache(vap, true);

exit:
    if (ret) {
        vap->iv_mbss.group_id = IEEE80211_MBSSID_INVALID_GROUP_ID;
    }
    return ret;
}

void ieee80211_mbssid_get_cumulative_rot_factor(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    /* lsb n bit of previous tx vap; n is max bssid inidicator  */
    uint8_t bssid_lsb_n_bit_prev;
    /* lsb n bit of cur tx vap; n is max bssid inidicator  */
    uint8_t bssid_lsb_n_bit_cur;
    /* bssid of the new or current tx-vap */
    uint8_t *bssid;
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    /* Retrieve bssid */
    wlan_vdev_obj_lock(vap->vdev_obj);
    bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
    wlan_vdev_obj_unlock(vap->vdev_obj);

    /* Retrieve n bits of lsb octet of previous and new tx-vap
     * bssid idxes
     */
    bssid_lsb_n_bit_prev = ic_mbss->tx_bssid[ATH_BSSID_INDEX] &
                                    (MBSSID_SUB_GROUP_SIZE(ic) - 1);
    bssid_lsb_n_bit_cur  = bssid[ATH_BSSID_INDEX] &
                                    (MBSSID_SUB_GROUP_SIZE(ic) - 1);

    /* Calculate cumulative rotation factor(rf) based on previous and
     * current tx-vap
     *     1. Add new value to previous to get cumulative rf
     *     2. Wrap it around by num_max_vaps
     */
    if (bssid_lsb_n_bit_prev >= bssid_lsb_n_bit_cur) {
        ic_mbss->rot_factor += (bssid_lsb_n_bit_prev - bssid_lsb_n_bit_cur);
    } else {
        ic_mbss->rot_factor += (mbss_ctx->grp_sz -
                                abs(bssid_lsb_n_bit_prev - bssid_lsb_n_bit_cur));
    }
    ic_mbss->rot_factor %= mbss_ctx->grp_sz;

    mbss_info("Rotation factor is: %d", ic_mbss->rot_factor);
}

void ieee80211_mbssid_refactor_bssid_idx(struct ieee80211vap *vap)
{
    struct ieee80211vap *tmpvap;
    struct ieee80211com *ic = vap->iv_ic;
    struct vdev_mlme_obj *vdev_mlme;
    /* lsb n bit of tx vap; n is max bssid inidicator */
    uint8_t bssid_lsb_n_bit_tx;
    /* lsb n bit of non-tx vap; n is max bssid inidicator */
    uint8_t bssid_lsb_n_bit_non_tx;
    uint8_t bssid_idx;
    /* bssid of tmpvap */
    uint8_t *tmpvap_bssid;
    /* bssid of tx-vap */
    uint8_t *tx_bssid;
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    /* Retrieve bssid of tx vap */
    wlan_vdev_obj_lock(vap->vdev_obj);
    tx_bssid = wlan_vdev_mlme_get_macaddr(vap->vdev_obj);
    wlan_vdev_obj_unlock(vap->vdev_obj);

    /* Retrieve n bits of lsb octet of tx vap */
    bssid_lsb_n_bit_tx = tx_bssid[ATH_BSSID_INDEX] &
                                (MBSSID_SUB_GROUP_SIZE(ic) - 1);
    mbss_info("tx_vap: %s vdev_id: %d", vap->iv_netdev_name, wlan_vdev_get_id(vap->vdev_obj));
    mbss_info("bssid_lsb_n_bit_tx: 0x%x", bssid_lsb_n_bit_tx);

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if (ieee80211_mbss_is_beaconing_ap(tmpvap) &&
            tmpvap->iv_mbss.group_id == grp_id &&
            IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmpvap)) {

            /* Retrieve vdev_mlme pointer of non-tx vap */
            vdev_mlme = tmpvap->vdev_mlme;

            /* Retrieve bssid of non-tx vap */
            wlan_vdev_obj_lock(tmpvap->vdev_obj);
            tmpvap_bssid = wlan_vdev_mlme_get_macaddr(tmpvap->vdev_obj);
            wlan_vdev_obj_unlock(tmpvap->vdev_obj);

            /* Retrieve n bits of lsb octet of non-tx vap */
            bssid_lsb_n_bit_non_tx = tmpvap_bssid[ATH_BSSID_INDEX] &
                                         (MBSSID_SUB_GROUP_SIZE(ic) - 1);

            mbss_info("non_tx_vap: %s vdev_id: %d", tmpvap->iv_netdev_name, tmpvap->iv_unit);
            mbss_info("bssid_lsb_n_bit_non_tx: 0x%x", bssid_lsb_n_bit_non_tx);

            /* Get new bssid_idx of non-tx vap wrt. new tx vap */
            IEEE80211_MBSSID_GET_BSSID_IDX(bssid_lsb_n_bit_tx,
                                       bssid_lsb_n_bit_non_tx,
                                       bssid_idx, false);

            /* Assign new bssid_idx to non-tx vap */
            vdev_mlme->mgmt.mbss_11ax.profile_idx = bssid_idx;

            mbss_info("bssid_idx(sub-grp): %d", bssid_idx);

            /* Update cache_entry corresponding to non-tx vap
             * with new value of bssid_idx
             */
            ieee80211_mbssid_update_mbssie_cache_entry(tmpvap, MBSS_CACHE_ENTRY_IDX);

            /* Mark entry in bssid-pool as occupied */
            qdf_set_bit(bssid_idx - 1,
                        (unsigned long *) &ic_mbss->bssid_index_bmap);
        }
    }
}

static void ieee80211_mbssid_refactor_aid_pool(struct ieee80211vap *vap,
                                               bool set)
{
    struct ieee80211vap *tmpvap;
    struct ieee80211com *ic = vap->iv_ic;
    bool is_mbssid_enabled  = ic->is_mbssid_enabled;
    bool is_tx_vap = !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap);
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint16_t start_aid = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_objmgr_vdev *non_tx_vdev = NULL;
    struct wlan_vdev_aid_mgr *aid_mgr = NULL;
#else
    unsigned long bm_size;
#endif

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    if (is_mbssid_enabled && is_tx_vap) {
        if (set) {

            KASSERT(vap->iv_max_aid != 0, ("0 max aid"));
            KASSERT(vap->iv_mbss_max_aid != 0, ("0 mbss max aid"));

#ifdef WLAN_FEATURE_11BE_MLO
            aid_mgr = wlan_vdev_mlme_get_aid_mgr(vap->vdev_obj);
            if (!aid_mgr) {
                aid_mgr = wlan_vdev_aid_mgr_init(vap->iv_max_aid);
                if (!aid_mgr) {
                    mbss_err("AID bitmap allocation failed for VDEV%d",
                             wlan_vdev_get_id(vap->vdev_obj));
                    return;
                }
                wlan_vdev_mlme_set_aid_mgr(vap->vdev_obj, aid_mgr);
            }
#else
            bm_size = howmany(vap->iv_max_aid,
                          sizeof(unsigned long) * BITS_PER_BYTE);

            vap->iv_aid_bitmap = qdf_mem_malloc(bm_size *
                                              sizeof(unsigned long));

            if (vap->iv_aid_bitmap == NULL) {
                /* XXX no way to recover */
                qdf_warn("no memory for AID bitmap!");
                vap->iv_max_aid      = 0;
                vap->iv_mbss_max_aid = 0;
                return;
            }
        } else {
            /*
             * free the current aid-pool
             * NOTE: There is no need to reset iv_max_aid and iv_mbss_max_aid
             * during reset since it might be required on bring-up again for
             * the same VAP context.
             */
            if (vap->iv_aid_bitmap) {
                OS_FREE(vap->iv_aid_bitmap);
                vap->iv_aid_bitmap = NULL;
            }
#endif
        }
        start_aid = ieee80211_get_vap_start_aid(ic, vap);
#ifdef WLAN_FEATURE_11BE_MLO
        wlan_vdev_mlme_set_start_aid(vap->vdev_obj, start_aid - 1);
#endif

        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            /* set or reset aid-pool of non-tx vaps */
            if (ieee80211_mbss_is_beaconing_ap(tmpvap) &&
                tmpvap->iv_mbss.group_id == grp_id &&
                IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmpvap)) {
#ifdef WLAN_FEATURE_11BE_MLO
                non_tx_vdev = tmpvap->vdev_obj;
                mbss_info("mlo enabled for vdev %d, is_mlo_ap %d, %d",
                          wlan_vdev_get_id(non_tx_vdev),
                          wlan_vdev_mlme_is_mlo_ap(non_tx_vdev),
                          non_tx_vdev->mlo_dev_ctx?1:0);
                if (set) {
                    if (wlan_vdev_mlme_is_mlo_ap(non_tx_vdev) ||
                        non_tx_vdev->mlo_dev_ctx) {
                         wlan_mlo_vdev_init_mbss_aid_mgr(non_tx_vdev->mlo_dev_ctx,
                                                         non_tx_vdev,
                                                         vap->vdev_obj);
                    }
                    else {
                         wlan_mlme_vdev_init_mbss_aid_mgr(non_tx_vdev,
                                                          vap->vdev_obj);
                    }
                }
                else {
                    if (wlan_vdev_mlme_is_mlo_ap(non_tx_vdev) ||
                        non_tx_vdev->mlo_dev_ctx) {
                         wlan_mlo_vdev_deinit_mbss_aid_mgr(non_tx_vdev->mlo_dev_ctx,
                                                           non_tx_vdev,
                                                           vap->vdev_obj);
                    }
                    else {
                         wlan_mlme_vdev_deinit_mbss_aid_mgr(non_tx_vdev,
                                                          vap->vdev_obj);
                    }

                }
#else
                tmpvap->iv_aid_bitmap   = set ? vap->iv_aid_bitmap : NULL;
#endif
                tmpvap->iv_max_aid      = set ? vap->iv_max_aid :
                    (ic->ic_num_clients + start_aid  + 1);
                /*
                 * iv_mbss_max_aid should follow iv_max_aid unconditionally
                 * during refactoring.
                 */
                tmpvap->iv_mbss_max_aid = tmpvap->iv_max_aid;
#ifdef WLAN_FEATURE_11BE_MLO
                wlan_vdev_mlme_aid_mgr_max_aid_set(non_tx_vdev, tmpvap->iv_max_aid);
#endif
            }
        }
    } /* end if (is_mbssid_enabled && is_tx_vap) */
}

void ieee80211_mbssid_reassign_bcn_pos_of_last_mbss_node(
                                       struct ieee80211vap *vap)
{
    struct ieee80211_mbss_ie_cache_node *node, *node_tx_vap;
    struct ieee80211com *ic = vap->iv_ic;
    bool is_mbssid_enabled  = ic->is_mbssid_enabled;
    uint8_t idx;
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];
    idx      = mbss_ctx->grp_sz - 1;

    /* If rotation factor is 0 then vap in the last node
     * is the tx-vap. In that case avoid this processing
     */
    if (is_mbssid_enabled && ic_mbss->rot_factor) {
        /* acquire mbss_cache_lock */
        qdf_spin_lock_bh(&ic_mbss->mbss_cache_lock);

        /* access last node */
        node = &((struct ieee80211_mbss_ie_cache_node *)
                                ic_mbss->mbss_cache)[idx];
        if (node->used) {
            /* Derive node_idx of the tx-vap */
            idx = ieee80211_mbssid_get_tx_vap_node_idx(ic, grp_id);

            /* Derive tx-vap node */
            node_tx_vap = &((struct ieee80211_mbss_ie_cache_node *)
                                ic_mbss->mbss_cache)[idx];

            qdf_assert(node_tx_vap->vdev_id == wlan_vdev_get_id(vap->vdev_obj));
            if (node_tx_vap->vdev_id != wlan_vdev_get_id(vap->vdev_obj)) {
                /* release mbss cache lock */
                qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
                return;
            }

            /* Assign beacon position of the tx vap to the
             * non-tx vap in the last entry of the MBSS cache
             */
            node->pos = node_tx_vap->pos;
        } else {
            mbss_info("No entry in last node of MBSS cache");
        }

        /* release mbss cache lock */
        qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
        mbss_info("<< last node beacon_pos: %d", node->pos);
    }
}

/* Check and update the first created vap as non-tx
 * vap. First vap always resides in the last entry
 * of the MBSS cache
 */
static void ieee80211_mbssid_mark_default_tx_vap_as_non_tx(
                                      struct ieee80211com *ic,
                                      uint8_t grp_id)
{
    struct ieee80211vap *default_tx_vap = NULL;
    struct wlan_objmgr_vdev *default_tx_vdev;
    struct ieee80211_mbss_ie_cache_node *node;
    bool is_mbssid_enabled = ic->is_mbssid_enabled;
    uint8_t idx;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];
    idx      = ieee80211_mbssid_get_tx_vap_node_idx(ic, grp_id);

    if (is_mbssid_enabled) {
        /* acquire mbss_cache_lock */
        qdf_spin_lock_bh(&ic_mbss->mbss_cache_lock);

        /* access default tx-vap node */
        node = &((struct ieee80211_mbss_ie_cache_node *)
                                    ic_mbss->mbss_cache)[idx];

        if (node->used) {
            /* Mark this node as non-tx vap as it is not
             * same as the tx vap
             */
            default_tx_vdev = wlan_objmgr_get_vdev_by_id_from_pdev(
                            ic->ic_pdev_obj, node->vdev_id, WLAN_MLME_NB_ID);

            if (default_tx_vdev) {
                default_tx_vap = wlan_vdev_mlme_get_ext_hdl(default_tx_vdev);
                wlan_objmgr_vdev_release_ref(default_tx_vdev, WLAN_MLME_NB_ID);

                if (default_tx_vap) {
                    if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(default_tx_vap)) {
                        IEEE80211_VAP_MBSS_NON_TRANS_ENABLE(default_tx_vap);
                    } else {
                        /* Release mbss cache lock */
                        qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);
                        /* Previous tx-vap is already marked non-tx vap
                         * during tx-vap bring-down sequence
                         */
                        mbss_info("Previous tx-vap is already marked non-tx vap!!!!");
                        return;
                    }
                }
            }
        } /* end if(node->used) */

        /* release mbss cache lock */
        qdf_spin_unlock_bh(&ic_mbss->mbss_cache_lock);

        if (IS_MBSSID_EMA_EXT_ENABLED(ic)) {
            if (!default_tx_vap) {
                qdf_debug("VAP corresponding to last node is NULL!");
                return;
            }

            if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(default_tx_vap)) {
                default_tx_vap->iv_mbss.non_tx_pfl_ie_pool =
                    qdf_nbuf_alloc(NULL, IEEE80211_NTX_PFL_IE_POOL_SIZE,
                                                            0, 0, false);
                if (!default_tx_vap->iv_mbss.non_tx_pfl_ie_pool)
                    return;
                qdf_mem_zero(default_tx_vap->iv_mbss.non_tx_pfl_ie_pool,
                        sizeof(default_tx_vap->iv_mbss.non_tx_pfl_ie_pool));
                qdf_spinlock_create(&default_tx_vap->iv_mbss.non_tx_pfl_ie_pool_lock);
                default_tx_vap->iv_mbss.ntx_pfl_rollback_stats = 0;
                default_tx_vap->iv_mbss.backup_length = 0;
            }
            default_tx_vap->iv_mbss.ie_overflow_stats = 0;
            default_tx_vap->iv_mbss.ie_overflow = false;
        }
    } /* end if (is_mbssid_enabled) */
}

int ieee80211_mbssid_txvap_set(struct ieee80211vap *vap)
{
    struct ieee80211com *ic;
    struct wlan_objmgr_psoc *psoc;
    struct vdev_mlme_obj *vdev_mlme;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t grp_id;

    if (!vap) {
         mbss_err("vap is NULL");
         return -1;
    }
    ic = vap->iv_ic;
    vdev_mlme = vap->vdev_mlme;
    grp_id = vap->iv_mbss.group_id;

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (psoc == NULL) {
        qdf_err("psoc is null");
        return -1;
    }
    if (!wlan_psoc_nif_fw_ext_cap_get(psoc,
                WLAN_SOC_CEXT_MBSS_PARAM_IN_START)) {
        mbss_err("Tx vap set is not supported");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    /* This vap is already configured as Tx VDEV */
    if (ic_mbss->transmit_vap) {
        mbss_info("Transmit vap is already configured");
        return -1;
    }

    vdev_mlme->mgmt.mbss_11ax.profile_idx = IEEE80211_INVALID_MBSS_BSSIDX(ic);
    ieee80211_mbssid_update_mbssie_cache_entry(vap, MBSS_CACHE_ENTRY_IDX);

    ic->ic_wifi_down_ind = 0;
    ic_mbss->prb_req_ssid_match_vap = NULL;

    /* Designate new tx vap */
    ic_mbss->transmit_vap = vap;

    if (ic_mbss->transmit_vap)
        mbss_info("tx-vap is set for vap: %d", wlan_vdev_get_id(vap->vdev_obj));

    if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
        /* Initially, driver starts by marking a default vap
         * as the tx vap in ieee80211_mbssid_setup(). We must
         * reset it as non-tx if another vap has been selected
         * as tx vap
         */
        ieee80211_mbssid_mark_default_tx_vap_as_non_tx(ic, grp_id);
    } else {
        mbss_info("vap id %d is chosen as tx-vap for grp %d", wlan_vdev_get_id(vap->vdev_obj), grp_id);
#ifdef WLAN_FEATURE_11BE_MLO
        ieee80211_mbssid_refactor_aid_pool(vap, true);
        vap->iv_mbss_max_aid = vap->iv_max_aid;
        wlan_vdev_mlme_aid_mgr_max_aid_set(vap->vdev_obj, vap->iv_max_aid);
#else
        if (!vap->iv_aid_bitmap) {
            mbss_info("Initializing AID bit map grp %d", grp_id);
            ieee80211_mbssid_refactor_aid_pool(vap, true);
        }
#endif
        if (!ic_mbss->transmit_vap) {
            mbss_err("Transmit vap is NULL");
            return -1;
        }
        return 0;
    }

    /* Mark vap as tx-vap */
    IEEE80211_VAP_MBSS_NON_TRANS_DISABLE(vap);

    /* Reset the bssid-idx pool */
    qdf_mem_zero((void *)&ic_mbss->bssid_index_bmap, sizeof(unsigned long));

    /* Get cumulative rotation factor */
    ieee80211_mbssid_get_cumulative_rot_factor(vap);

    /* Refactor bssid idxes and bssid idx-pool with respect to
     * the bssid of the new tx vap
     */
    ieee80211_mbssid_refactor_bssid_idx(vap);

    /* Reassign ref_bssid. Not required to ensure 48-n bit
     * sanity here as user can set only an alreay created vap
     * as tx vap. And, all vap's bssid gets sanitized at create
     * time
     */
    wlan_vdev_obj_lock(vap->vdev_obj);
    IEEE80211_ADDR_COPY(ic_mbss->tx_bssid,
                        wlan_vdev_mlme_get_macaddr(vap->vdev_obj));
    wlan_vdev_obj_unlock(vap->vdev_obj);

    /* Last node in MBSS cache will always occupy the beacon
     * position of the slot that the tx-vap held before getting
     * designated as the new tx-vap
     */
    ieee80211_mbssid_reassign_bcn_pos_of_last_mbss_node(vap);

    /* Reset DTIM value */
    if (vap->iv_create_flags & IEEE80211_LP_IOT_VAP) {
        vdev_mlme->proto.generic.dtim_period = IEEE80211_DTIM_DEFAULT_LP_IOT;
    } else {
        vdev_mlme->proto.generic.dtim_period = qdf_roundup(vdev_mlme->proto.generic.dtim_period,
                                                           ic_mbss->max_pp);
    }
    /* Reset aid-pool */
    ieee80211_mbssid_refactor_aid_pool(vap, true);
#ifdef WLAN_FEATURE_11BE_MLO
    vap->iv_mbss_max_aid = vap->iv_max_aid;
    wlan_vdev_mlme_aid_mgr_max_aid_set(vap->vdev_obj, vap->iv_max_aid);
#endif

    /* Dealloc skb for non-Tx IE Pool when Tx-VAP changes.
     * If non_tx_pfl_ie_pool already exists when trying to
     * set Tx-VAP then this case is hit.
     */
    if ((mbss_ctx->ema_ext_enabled) &&
        (vap->iv_mbss.non_tx_pfl_ie_pool != NULL)) {
        IEEE80211_NTX_PFL_IE_POOL_LOCK(vap);
        qdf_nbuf_free(vap->iv_mbss.non_tx_pfl_ie_pool);
        vap->iv_mbss.non_tx_pfl_ie_pool = NULL;
        IEEE80211_NTX_PFL_IE_POOL_UNLOCK(vap);
        qdf_spinlock_destroy(&vap->iv_mbss.non_tx_pfl_ie_pool_lock);
        vap->iv_mbss.ie_overflow = false;
        vap->iv_mbss.ie_overflow_stats = 0;
    }

    mbss_info("Designated transmitting VAP (%pK), vdev_id: %d"
              " max_bssid_indicator: %d",
              vap, wlan_vdev_get_id(vap->vdev_obj), ic_mbss->max_bssid);

    if (!ic_mbss->transmit_vap) {
        mbss_err("Transmit vap is NULL");
        return -1;
    }
    return 0;
}

int ieee80211_mbssid_txvap_reset(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;
    struct wlan_objmgr_psoc *psoc;
    bool is_ema_ap_enabled = wlan_pdev_nif_feat_ext_cap_get(
                             ic->ic_pdev_obj, WLAN_PDEV_FEXT_EMA_AP_ENABLE);
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
#ifdef WLAN_FEATURE_11BE_MLO
    uint16_t start_aid = 0;
#endif

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (psoc == NULL) {
        qdf_err("psoc is null");
        return -1;
    }

    if (!wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_SOC_CEXT_MBSS_PARAM_IN_START)) {
        mbss_err("Tx vap reset is not supported");
        return -1;
    }

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    /* This vap is not configured as Tx VDEV */
    if (ic_mbss->transmit_vap != vap) {
        mbss_info(" vap%d is not configured as Tx VAP", vap->iv_unit);
        return -1;
    }

    if (ic_mbss->transmit_vap)
        mbss_info("Tx VAP is reset for vap%d", ic_mbss->transmit_vap->iv_unit);

    ic_mbss->transmit_vap = NULL;

    /* Reset aid-pool of all non-tx vaps */
    ieee80211_mbssid_refactor_aid_pool(vap, false);
#ifdef WLAN_FEATURE_11BE_MLO
    start_aid = ieee80211_get_vap_start_aid(vap->iv_ic, vap);
    vap->iv_max_aid = ic->ic_num_clients + start_aid  + 1;
    vap->iv_mbss_max_aid = vap->iv_max_aid;
    wlan_vdev_mlme_aid_mgr_max_aid_set(vap->vdev_obj, vap->iv_max_aid);
    qdf_debug("aid:%d, iv_max_aid:%d, vap:%s",start_aid, vap->iv_max_aid, vap->iv_netdev_name);
#endif

    /* For FW recovery case, vap params are copied to create vap again
       after FW load, so not setting Non-Tx VDEV flag to store it in
       vap profile */
    if (!ic->recovery_in_progress) {
        IEEE80211_VAP_MBSS_NON_TRANS_ENABLE(vap);
        vdev_mlme->mgmt.mbss_11ax.profile_idx
                = IEEE80211_INVALID_MBSS_BSSIDX(ic);

        /* When Tx-VAP becomes a non-Tx VAP, allocate skb for
         * non_tx_pfl_ie_pool and create lock.
         */
        if (mbss_ctx->ema_ext_enabled) {
            vap->iv_mbss.non_tx_pfl_ie_pool =
                qdf_nbuf_alloc(NULL, IEEE80211_NTX_PFL_IE_POOL_SIZE, 0, 0, false);
            if (vap->iv_mbss.non_tx_pfl_ie_pool) {
                qdf_mem_zero(vap->iv_mbss.non_tx_pfl_ie_pool,
                             sizeof(vap->iv_mbss.non_tx_pfl_ie_pool));
                qdf_spinlock_create(&vap->iv_mbss.non_tx_pfl_ie_pool_lock);
            } else {
                mbss_err("Alloc failed for non_tx_pfl_ie_pool");
            }

            vap->iv_mbss.ntx_pfl_rollback_stats = 0;
            vap->iv_mbss.ie_overflow_stats = 0;
            vap->iv_mbss.ie_overflow = false;
            vap->iv_mbss.backup_length = 0;
        }

        /* Clear Tx-vap capabilities
         *
         * 1. FILS cap
         */
#ifdef WLAN_SUPPORT_FILS
        ucfg_fils_disable(vap->vdev_obj);
#endif
    }

    /* Reset current-pp */
    ic_mbss->current_pp = 1;

    if (is_ema_ap_enabled) {
        uint8_t dtim_period;

        if (vap->iv_create_flags & IEEE80211_LP_IOT_VAP) {
            dtim_period = qdf_roundup(
                            IEEE80211_DTIM_DEFAULT_LP_IOT, ic_mbss->max_pp);
        } else {
            dtim_period = qdf_roundup(
                            IEEE80211_DTIM_DEFAULT, ic_mbss->max_pp);
        }

        /* Reset DTIM value as per non-tx EMA vap */
        vdev_mlme->proto.generic.dtim_period =
                            qdf_roundup(dtim_period, ic_mbss->max_pp);
    }

    return 0;
}

int ieee80211_mbssid_txvap_is_active(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211vap *txvap;
    bool is_mbssid_enabled = ic->is_mbssid_enabled;
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    if ((vap->iv_opmode != IEEE80211_M_HOSTAP)
         || !is_mbssid_enabled)
       return 1;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return 0;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

    if (is_mbssid_enabled && (ic_mbss->transmit_vap)) {
         txvap = ic_mbss->transmit_vap;
         if (wlan_vdev_is_up(txvap->vdev_obj) == QDF_STATUS_SUCCESS)
             return 1;
    }

    return 0;
}

bool ieee80211_mbss_is_beaconing_ap(struct ieee80211vap *vap)
{
    if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
            !IEEE80211_IS_SPECIAL_VAP(vap) && !IEEE80211_IS_MLO_BRIDGE_VAP(vap))
        return true;

    return false;
}

/*
 * ieee80211_mbssid_beacon_control:
 * Function to disable or enable beaconing for a MBSSID VAP.
 * It keeps track of number of times beaconing is  disabled/enabled.
 * If disabled multiple times, beaconing needs to be enabled equal number
 * of times. If it can't be enabled, then DISABLE is returned.
 *
 * @vap  : Pointer to the VAP structure
 * @bcn_cntrl : MBSS_BCN_ENABLE or MBSS_BCN_DISABLE
 *
 * Return: -1 if error, else state of VAP i.e. beaconing is enabled or disabled
 */

int ieee80211_mbssid_beacon_control(struct ieee80211vap *vap,
                                    uint8_t bcn_ctrl)
{
    struct ieee80211com *ic = vap->iv_ic;
    uint8_t grp_id = vap->iv_mbss.group_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    bool is_tx_vap;

    if (!ic->ic_mbss_ctx) {
        mbss_err("mbss_ctx is null");
        return -1;
    }
    mbss_ctx  = ic->ic_mbss_ctx;
    ic_mbss   = &mbss_ctx->ic_mbss[grp_id];
    is_tx_vap = !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap);

    /* check for tx vap */
    if (!ic->is_mbssid_enabled ||
        (ic->is_mbssid_enabled && is_tx_vap)) {
        qdf_err("Beacon control not supported in this mode");
        return -1;
    }

    switch (bcn_ctrl) {
    case MBSS_BCN_DISABLE:
        /* increment and return if disabled before */
        if (qdf_atomic_inc_return(&vap->iv_mbss.bcn_ctrl) > 1)
            return bcn_ctrl;
        break;
    case MBSS_BCN_ENABLE:
        if (qdf_atomic_read(&vap->iv_mbss.bcn_ctrl)) {
        /* decrement and return if still disabled,
         * otherwise resume beaconing */
            if (qdf_atomic_dec_return(&vap->iv_mbss.bcn_ctrl) >= 1)
                return MBSS_BCN_DISABLE;
        }
        break;
    default:
        return -1;;
    }

    /* send the template when going from DISABLE->ENABLE or ENABLE->DISABLE.
     * value of iv_mbss.bcn_ctrl goes 1->0 or 0->1 */
    vap->iv_mbss.mbssid_update_ie = true;
    ic->ic_vdev_beacon_template_update(vap);

    return bcn_ctrl;
}

bool wlan_mbss_beaconing_vdev_up(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211vap *vap;

    if (!vdev)
        return false;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap)
        return false;

    return (wlan_vdev_is_up(vdev) == QDF_STATUS_SUCCESS &&
            /* value of 0 indicates that beaconing is not suspended */
            !qdf_atomic_read(&vap->iv_mbss.bcn_ctrl));
}
qdf_export_symbol(wlan_mbss_beaconing_vdev_up);

static void ieee80211_user_rnr_init(struct ieee80211com *ic)
{
    ic->ic_user_neighbor_ap.running_length = 0;
    TAILQ_INIT(&ic->ic_user_neighbor_ap.user_rnr_data_list);
    qdf_spinlock_create(&ic->ic_user_neighbor_ap.user_rnr_lock);
}

void ieee80211_user_rnr_free(struct ieee80211com *ic)
{
    struct user_rnr_data *tmp_rnr_node = NULL, *tmp = NULL;

    ic->ic_user_neighbor_ap.running_length = 0;
    qdf_spinlock_destroy(&ic->ic_user_neighbor_ap.user_rnr_lock);
    TAILQ_FOREACH_SAFE(tmp_rnr_node, &ic->ic_user_neighbor_ap.user_rnr_data_list,
                       user_rnr_next_uid, tmp) {
        TAILQ_REMOVE(&ic->ic_user_neighbor_ap.user_rnr_data_list, tmp_rnr_node,
                     user_rnr_next_uid);
            qdf_mem_free(tmp_rnr_node->user_buf);
            qdf_mem_free(tmp_rnr_node);
    }
}

int
ieee80211_ifattach(struct ieee80211com *ic, IEEE80211_REG_PARAMETERS *ieee80211_reg_parm)
{
    u_int8_t bcast[QDF_MAC_ADDR_SIZE] = {0xff,0xff,0xff,0xff,0xff,0xff};
    int error = 0;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct target_psoc_info *psoc_info;

    psoc_info = wlan_psoc_get_tgt_if_handle(scn->soc->psoc_obj);

#define DEFAULT_TRAFFIC_INTERVAL 1800
#define DEFAULT_TRAFFIC_RATE     300

    /* set up broadcast address */
    IEEE80211_ADDR_COPY(ic->ic_broadcast, bcast);

    /* initialize channel list */
    ieee80211_update_channellist(ic, 0, false);

    /* initialize rate set */
    ieee80211_init_rateset(ic);

#if SUPPORT_11AX_D3
    ieee80211_heop_init(ic);
#else
    ieee80211_heop_param_init(ic);
#endif

#ifdef ATH_SUPPORT_DFS
    ic->ic_dfs_start_tx_rcsa_and_waitfor_rx_csa = ieee80211_dfs_start_tx_rcsa_and_waitfor_rx_csa;
#endif

    /* validate ic->ic_curmode */
    if (!IEEE80211_SUPPORT_PHY_MODE(ic, ic->ic_curmode))
        ic->ic_curmode = IEEE80211_MODE_AUTO;

    /* setup initial channel settings */
    ic->ic_curchan_obj = ieee80211_ath_get_first_channel(ic);
    ic->ic_curchan = &ic->ic_curchan_obj;
    ic->ic_prevchan_obj = empty_channel;
    ic->ic_prevchan = &ic->ic_prevchan_obj;

    ic->ic_tx_next_ch = &ic->ic_tx_next_ch_obj;
    ic->ic_chanchange_channel = &ic->ic_chanchange_channel_obj;
    ic->recovery_dfschan = &ic->recovery_dfschan_obj;
    ic->ic_radar_defer_chan = &ic->ic_radar_defer_chan_obj;

    /* Enable marking of dfs by default */
    IEEE80211_FEXT_MARKDFS_ENABLE(ic);

    if (ic->ic_reg_parm.htEnableWepTkip) {
        ieee80211_ic_wep_tkip_htrate_set(ic);
    } else {
        ieee80211_ic_wep_tkip_htrate_clear(ic);
    }

    if (ic->ic_reg_parm.htVendorIeEnable)
        IEEE80211_ENABLE_HTVIE(ic);

    /* whether to ignore 11d beacon */
    if (ic->ic_reg_parm.ignore11dBeacon)
        IEEE80211_ENABLE_IGNORE_11D_BEACON(ic);

    if (ic->ic_reg_parm.disallowAutoCCchange) {
        ieee80211_ic_disallowAutoCCchange_set(ic);
    }
    else {
        ieee80211_ic_disallowAutoCCchange_clear(ic);
    }

    (void) ieee80211_setmode(ic, ic->ic_curmode, ic->ic_opmode);

    ic->ic_he_bsscolor = IEEE80211_BSS_COLOR_INVALID;
    if (EOK != ieee80211_bsscolor_attach(ic)) {
        qdf_print("%s: BSS Color attach failed _investigate__ ",__func__);
    }

    ic->ic_intval = IEEE80211_BINTVAL_DEFAULT; /* beacon interval */
    ic->ic_set_beacon_interval(ic);

    ic->ic_lintval = 1;         /* listen interval */
    ic->ic_bmisstimeout = IEEE80211_BMISS_LIMIT * ic->ic_intval;
    TAILQ_INIT(&ic->ic_vaps);

    ic->ic_txpowlimit = IEEE80211_TXPOWER_MAX;

    /* Intialize WDS Auto Detect mode */
    ieee80211com_set_flags_ext(ic, IEEE80211_FEXT_WDS_AUTODETECT);

	/*
	** Enable the 11d country code IE by default
	*/

    ieee80211com_set_flags_ext(ic, IEEE80211_FEXT_COUNTRYIE);

    /* setup CWM configuration */
    ic->ic_cwm_set_mode(ic, ic->ic_reg_parm.cwmMode);
    ic->ic_cwm_set_extoffset(ic, ic->ic_reg_parm.cwmExtOffset);
    ic->ic_cwm_set_extprotmode(ic, ic->ic_reg_parm.cwmExtProtMode);
    ic->ic_cwm_set_extprotspacing(ic, ic->ic_reg_parm.cwmExtProtSpacing);

    ic->ic_cwm_set_enable(ic, ic->ic_reg_parm.cwmEnable);
    ic->ic_cwm_set_extbusythreshold(ic, ic->ic_reg_parm.cwmExtBusyThreshold);

    ic->ic_enable2GHzHt40Cap = ic->ic_reg_parm.enable2GHzHt40Cap;

    ic->ic_ignoreDynamicHalt = ic->ic_reg_parm.ignoreDynamicHalt;

    /* default to auto ADDBA mode */
    ic->ic_addba_mode = ADDBA_MODE_AUTO;
    wlan_pdev_set_addba_mode(ic->ic_pdev_obj, ADDBA_MODE_AUTO);

    if (ic->ic_reg_parm.ht20AdhocEnable) {
        /*
         * Support HT rates in Ad hoc connections.
         */
        if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT20) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT20)) {
            ieee80211_ic_ht20Adhoc_set(ic);

            if (ic->ic_reg_parm.htAdhocAggrEnable) {
                ieee80211_ic_htAdhocAggr_set(ic);
            }
        }
    }

    if (ic->ic_reg_parm.ht40AdhocEnable) {
        /*
         * Support HT rates in Ad hoc connections.
         */
        if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT40PLUS) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT40MINUS) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT40PLUS) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT40MINUS)) {
            ieee80211_ic_ht40Adhoc_set(ic);

            if (ic->ic_reg_parm.htAdhocAggrEnable) {
                ieee80211_ic_htAdhocAggr_set(ic);
            }
        }
    }

    OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_cw_timer), ic_cw_timeout,(void *) (ic), QDF_TIMER_TYPE_WAKE_APPS);
    ic->ic_obss_nb_ru_tolerance_time = IEEE80211_OBSS_NB_RU_TOLERANCE_TIME_DEFVAL;
    OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_obss_nb_ru_tolerance_timer), ieee80211_obss_nb_ru_tolerence_timeout, (void *) (ic), QDF_TIMER_TYPE_WAKE_APPS);
#if UMAC_SUPPORT_WNM
    /* Enable Host tracking of Max BSS Idle Timeout only for pre-lithium targets */
    if (target_psoc_get_target_type(psoc_info) < TARGET_TYPE_QCA8074) {
        OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_bssload_timer), ieee80211_bssload_timeout, (void *) (ic), QDF_TIMER_TYPE_WAKE_APPS);
    }
#endif

    OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_noise_stats), ieee80211_noise_stats_update, (void *) (ic), QDF_TIMER_TYPE_WAKE_APPS);

    qdf_timer_init(NULL, &ic->ic_csa_max_rx_wait_timer,
            ieee80211_csa_max_rx_wait_timer, (void *)(ic),
            QDF_TIMER_TYPE_WAKE_APPS);

    /* Initialization of traffic interval and traffic rate is 1800 and 300 seconds respectively */
    ic->traf_interval = DEFAULT_TRAFFIC_INTERVAL;
    ic->traf_rate = DEFAULT_TRAFFIC_RATE;
    ic->traf_bins = ic->traf_interval/ic->traf_rate;
    if (ic->ic_reg_parm.disable2040Coexist) {
        ieee80211com_set_flags(ic, IEEE80211_F_COEXT_DISABLE);
    } else {
        ieee80211com_clear_flags(ic, IEEE80211_F_COEXT_DISABLE);
    }

    /* setup other modules */

    /* The TSF Timer module is required when P2P or Off-channel support are required */
    ic->ic_tsf_timer = ieee80211_tsf_timer_attach(ic);

    ieee80211_p2p_attach(ic);
    ieee80211_node_attach(ic);
    ieee80211_proto_attach(ic);
    ieee80211_power_attach(ic);
    ieee80211_mlme_attach(ic);
#ifdef WLAN_FEATURE_11BE_MLO
    /* Intialize cu_vdev_ids with invalid VDEV ID */
    qdf_mem_set(&ic->cu_vdev_ids, sizeof(ic->cu_vdev_ids),
            IEEE80211_CU_INVALID_VDEV_ID);
#endif

#if ATH_SUPPORT_DFS
    ic->set_country = (struct assoc_sm_set_country*)qdf_mem_malloc(
            sizeof(struct assoc_sm_set_country));

    ATH_CREATE_WORK(&ic->dfs_cac_timer_start_work,
            ieee80211_dfs_cac_timer_start_async,
            (void *)ic);
    ATH_CREATE_WORK(&ic->assoc_sm_set_country_code,
            ieee80211_set_country_code_assoc_sm,
            (void *)ic);
    OS_INIT_TIMER(ic->ic_osdev,
            &(ic->ic_dfs_tx_rcsa_and_nol_ie_timer),
            ieee80211_dfs_tx_rcsa_task,
            (void *)(ic),
            QDF_TIMER_TYPE_WAKE_APPS);
    OS_INIT_TIMER(ic->ic_osdev,
            &(ic->ic_dfs_waitfor_csa_timer),
            ieee80211_dfs_waitfor_csa_task,
            (void *)(ic),
            QDF_TIMER_TYPE_WAKE_APPS);
#if QCA_DFS_NOL_VAP_RESTART
    OS_INIT_TIMER(ic->ic_osdev,
            &(ic->ic_dfs_nochan_vap_restart_timer),
            ieee80211_dfs_nochan_vap_restart,
            (void *)(ic),
            QDF_TIMER_TYPE_WAKE_APPS);
#endif
#endif /* ATH_SUPPORT_DFS */
    ATH_CREATE_WORK(&ic->ic_cfg80211_update_channel_list_start_work,
            ieee80211_cfg80211_update_channel_list,
            (void *)ic);

    /*
     * By default overwrite probe response with beacon IE in scan entry.
     */
    ieee80211_ic_override_proberesp_ie_set(ic);

    ic->ic_resmgr = ieee80211_resmgr_create(ic, IEEE80211_RESMGR_MODE_SINGLE_CHANNEL);

#if ATH_ACS_DEBUG_SUPPORT
    ic->ic_acs_debug_support = 0;
#endif

    error = ieee80211_acs_attach(&(ic->ic_acs),
                          ic,
                          ic->ic_osdev);
    if (error) {
        /* detach and free already allocated memory for scan */
        ieee80211_node_detach(ic);
        return error;
    }

#if UMAC_SUPPORT_CBS
    error = ieee80211_cbs_attach(&(ic->ic_cbs),
                          ic,
                          ic->ic_osdev);
    if (error) {
        /* detach and free already allocated memory for scan */
        ieee80211_acs_detach(&(ic->ic_acs));
        ieee80211_node_detach(ic);
        return error;
    }
#endif

    ic->ic_notify_tx_bcn_mgr = ieee80211_notify_tx_bcn_attach(ic);
#if UMAC_SUPPORT_VI_DBG
    ieee80211_vi_dbg_attach(ic);
#endif
    ieee80211_quiet_attach(ic);
	ieee80211_admctl_attach(ic);

    /*
     * Perform steps that require multiple objects to be initialized.
     * For example, cross references between objects such as ResMgr and Scanner.
     */
    ieee80211_resmgr_create_complete(ic->ic_resmgr);

    ic->ic_get_ext_chan_info = ieee80211_get_extchaninfo;

#if ACFG_NETLINK_TX
    acfg_attach(ic);
#elif UMAC_SUPPORT_ACFG || UMAC_SUPPORT_ACFG_RECOVERY
    acfg_event_workqueue_init(ic->ic_osdev);
#endif
#if UMAC_SUPPORT_ACFG
    acfg_diag_attach(ic);
#endif

    ic->ic_chan_stats_th = IEEE80211_CHAN_STATS_THRESOLD;
    ic->ic_chan_switch_cnt = IEEE80211_RADAR_11HCOUNT;
    ic->ic_wb_subelem = 1;
    ic->ic_sec_offsetie = 1;

    /* initialization complete */
    ic->ic_initialized = 1;

    ic->ic_nr_share_radio_flag = 0;
    ic->ic_nr_share_enable = 0;
    if (ic->ic_cfg80211_config) {
        ic->ic_roaming = IEEE80211_ROAMING_MANUAL;
    }

    ic->ic_is_restart_on_same_chan = false;
    if (ic->ic_is_target_lithium(scn->soc->psoc_obj)) {
        ieee80211_user_rnr_init(ic);
    }

    if (wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj, WLAN_PDEV_F_MBSS_IE_ENABLE)) {
        ic->ic_mbss_ctx->ema_ext_enabled = scn->ema_ctx.ext_enabled;
        ic->ic_mbss_ctx->mbss_split_profile_enabled =
                            scn->ema_ctx.mbss_split_profile_enabled;
    }

    return 0;
#undef DEFAULT_TRAFFIC_INTERVAL
#undef DEFAULT_TRAFFIC_RATE
}

void
ieee80211_ifdetach(struct ieee80211com *ic)
{
    if (!ic->ic_initialized) {
        return;
    }

    /* Setting zero to aviod re-arming of ic_inact_timer timer */
    ic->ic_initialized = 0;

    /*
     * Preparation for detaching objects.
     * For example, remove and cross references between objects such as those
     * between ResMgr and Scanner.
     */
    ieee80211_resmgr_delete_prepare(ic->ic_resmgr);

    qdf_timer_free(&ic->ic_csa_max_rx_wait_timer);

    OS_FREE_TIMER(&ic->ic_inact_timer);
#if UMAC_SUPPORT_WNM
    OS_FREE_TIMER(&ic->ic_bssload_timer);
#endif

    OS_FREE_TIMER(&ic->ic_obss_nb_ru_tolerance_timer);

    OS_FREE_TIMER(&ic->ic_noise_stats);

#ifdef ATH_SUPPORT_DFS
    OS_CANCEL_TIMER(&ic->ic_dfs_tx_rcsa_and_nol_ie_timer);
    if(ic->ic_dfs_waitfor_csa_sched) {
        OS_CANCEL_TIMER(&ic->ic_dfs_waitfor_csa_timer);
        ic->ic_dfs_waitfor_csa_sched = 0;
    }
#endif

#if ATH_SUPPORT_DFS && QCA_DFS_NOL_VAP_RESTART
    OS_CANCEL_TIMER(&ic->ic_dfs_nochan_vap_restart_timer);
#endif

    /* all the vaps should have been deleted now */
    ASSERT(TAILQ_FIRST(&ic->ic_vaps) == NULL);

    ieee80211_node_detach(ic);
    ieee80211_quiet_detach(ic);
	ieee80211_admctl_detach(ic);

    ieee80211_proto_detach(ic);
    ieee80211_power_detach(ic);
    ieee80211_mlme_detach(ic);
#ifdef WLAN_FEATURE_11BE_MLO
    /* Reset cu_vdev_ids with invalid VDEV ID */
    qdf_mem_set(&ic->cu_vdev_ids, sizeof(ic->cu_vdev_ids),
            IEEE80211_CU_INVALID_VDEV_ID);
#endif

    ieee80211_notify_tx_bcn_detach(ic->ic_notify_tx_bcn_mgr);
    ieee80211_resmgr_delete(ic->ic_resmgr);
    ieee80211_p2p_detach(ic);
#if UMAC_SUPPORT_CBS
    ieee80211_cbs_detach(&(ic->ic_cbs));
#endif
    ieee80211_acs_detach(&(ic->ic_acs));

    if( EOK != ieee80211_bsscolor_detach(ic)){
        qdf_print("%s: BSS Color detach failed ",__func__);
    }
#if UMAC_SUPPORT_VI_DBG
    ieee80211_vi_dbg_detach(ic);
#endif

#if UMAC_SUPPORT_ACFG
    acfg_diag_detach(ic);
#endif

#if ACFG_NETLINK_TX || UMAC_SUPPORT_ACFG_RECOVERY
    acfg_detach(ic);
#endif

    /* Detach TSF timer at the end to avoid assertion */
    if (ic->ic_tsf_timer) {
        ieee80211_tsf_timer_detach(ic->ic_tsf_timer);
        ic->ic_tsf_timer = NULL;
    }

#if QCA_SUPPORT_GPR
    if(ic->ic_gpr_enable) {
        qdf_hrtimer_kill(&ic->ic_gpr_timer);
        qdf_mem_free(ic->acfg_frame);
        ic->acfg_frame = NULL;
        qdf_err("\nStopping GPR timer as this is last vap with gpr \n");
    }
#endif

    spin_lock_destroy(&ic->ic_lock);
    spin_lock_destroy(&ic->ic_main_sta_lock);
    spin_lock_destroy(&ic->ic_chan_change_lock);
    spin_lock_destroy(&ic->ic_addba_lock);
    IEEE80211_STATE_LOCK_DESTROY(ic);
    spin_lock_destroy(&ic->ic_beacon_alloc_lock);
    spin_lock_destroy(&ic->ic_diag_lock);
    spin_lock_destroy(&ic->ic_radar_found_lock);
    spin_lock_destroy(&ic->ic_radar_mode_switch_lock);
    qdf_spinlock_destroy(&ic->ic_channel_stats.lock);
#ifdef WLAN_FEATURE_11BE_MLO
    spin_lock_destroy(&ic->ic_cu_vdev_map_lock);
#endif
}

/*
 * Start this IC
 */
void ieee80211_start_running(struct ieee80211com *ic)
{
    OS_SET_TIMER(&ic->ic_inact_timer, IEEE80211_INACT_WAIT*1000);
}

/*
 * Stop this IC
 */
void ieee80211_stop_running(struct ieee80211com *ic)
{
    OS_CANCEL_TIMER(&ic->ic_inact_timer);
}

int ieee80211com_register_event_handlers(struct ieee80211com *ic,
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{
    int i;
    /* unregister if there exists one already */
    ieee80211com_unregister_event_handlers(ic,event_arg,evtable);
    IEEE80211_COMM_LOCK(ic);
    for (i=0;i<IEEE80211_MAX_DEVICE_EVENT_HANDLERS; ++i) {
        if ( ic->ic_evtable[i] == NULL) {
            ic->ic_evtable[i] = evtable;
            ic->ic_event_arg[i] = event_arg;
            IEEE80211_COMM_UNLOCK(ic);
            return 0;
        }
    }
    IEEE80211_COMM_UNLOCK(ic);
    return -ENOMEM;


}

int ieee80211com_unregister_event_handlers(struct ieee80211com *ic,
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{
    int i;
    IEEE80211_COMM_LOCK(ic);
    for (i=0;i<IEEE80211_MAX_DEVICE_EVENT_HANDLERS; ++i) {
        if ( ic->ic_evtable[i] == evtable &&  ic->ic_event_arg[i] == event_arg) {
            ic->ic_evtable[i] = NULL;
            ic->ic_event_arg[i] = NULL;
            IEEE80211_COMM_UNLOCK(ic);
            return 0;
        }
    }
    IEEE80211_COMM_UNLOCK(ic);
    return -EEXIST;
}

/* Clear user defined ADDBA response codes for all nodes. */
static void
ieee80211_addba_clearresponse(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211com *ic = (struct ieee80211com *) arg;
    ic->ic_addba_clearresponse(ni);
}

int wlan_device_register_event_handlers(wlan_dev_t devhandle,
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{

    return ieee80211com_register_event_handlers(devhandle,event_arg,evtable);
}


int wlan_device_unregister_event_handlers(wlan_dev_t devhandle,
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{
    return ieee80211com_unregister_event_handlers(devhandle,event_arg,evtable);
}


int wlan_set_device_param(wlan_dev_t ic, ieee80211_device_param param, u_int32_t val)
{
    int retval=EOK;
    switch(param) {
    case IEEE80211_DEVICE_TX_CHAIN_MASK:
    case IEEE80211_DEVICE_TX_CHAIN_MASK_LEGACY:
        if(ic->ic_set_chain_mask(ic,param,val) == 0) {
            ic->ic_tx_chainmask = val;
            wlan_objmgr_update_txchainmask_to_allvdevs(ic);
            if (ic->ic_update_chainmask_caps)
                ic->ic_update_chainmask_caps(ic, TX_CHAINMASK);
        } else {
            retval=EINVAL;
        }
        break;
    case IEEE80211_DEVICE_RX_CHAIN_MASK:
    case IEEE80211_DEVICE_RX_CHAIN_MASK_LEGACY:
        if(ic->ic_set_chain_mask(ic,param,val) == 0) {
            ic->ic_rx_chainmask = val;
            wlan_objmgr_update_rxchainmask_to_allvdevs(ic);
            if (ic->ic_update_chainmask_caps)
                ic->ic_update_chainmask_caps(ic, RX_CHAINMASK);
        } else {
            retval=EINVAL;
        }
        break;

    case IEEE80211_DEVICE_PROTECTION_MODE:
        if (val > IEEE80211_PROT_RTSCTS) {
	    retval=EINVAL;
        } else {
	   ic->ic_protmode = val;
        }
        break;
    case IEEE80211_DEVICE_NUM_TX_CHAIN:
    case IEEE80211_DEVICE_NUM_RX_CHAIN:
    case IEEE80211_DEVICE_COUNTRYCODE:
       /* read only */
	retval=EINVAL;
        break;
    case IEEE80211_DEVICE_BMISS_LIMIT:
    	ic->ic_bmisstimeout = val * ic->ic_intval;
        break;
    case IEEE80211_DEVICE_GREEN_AP_ENABLE_PRINT:
        ic->ic_green_ap_set_print_level(ic, val);
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTMODE:
        if (val < IEEE80211_CWM_EXTPROTMAX) {
            ic->ic_cwm_set_extprotmode(ic, val);
        } else {
            retval = EINVAL;
        }
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTSPACING:
        if (val < IEEE80211_CWM_EXTPROTSPACINGMAX) {
            ic->ic_cwm_set_extprotspacing(ic, val);
        } else {
            retval = EINVAL;
        }
        break;
    case IEEE80211_DEVICE_CWM_ENABLE:
        ic->ic_cwm_set_enable(ic, val);
        break;
    case IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD:
        ic->ic_cwm_set_extbusythreshold(ic, val);
        break;
    case IEEE80211_DEVICE_DOTH:
        if (val == 0) {
            ieee80211_ic_doth_clear(ic);
        } else {
            ieee80211_ic_doth_set(ic);
        }
        break;
    case IEEE80211_DEVICE_ADDBA_MODE:
        ic->ic_addba_mode = val;
        wlan_pdev_set_addba_mode(ic->ic_pdev_obj, val);
        /*
        * Clear any user defined ADDBA response codes before switching modes.
        */
        wlan_mlme_iterate_node_list(ic, ieee80211_addba_clearresponse,
                                    (void *)ic,
                                    (IEEE80211_NODE_ITER_F_ASSOC_STA |
                                     IEEE80211_NODE_ITER_F_UNASSOC_STA));
        break;
    case IEEE80211_DEVICE_MULTI_CHANNEL:
        if (!val) {
            /* Disable Multi-Channel */
            retval = ieee80211_resmgr_setmode(ic->ic_resmgr, IEEE80211_RESMGR_MODE_SINGLE_CHANNEL);
        }
        else if (ic->ic_caps_ext & IEEE80211_CEXT_MULTICHAN) {
            retval = ieee80211_resmgr_setmode(ic->ic_resmgr, IEEE80211_RESMGR_MODE_MULTI_CHANNEL);
        }
        else {
            qdf_nofl_info("%s: Unable to enable Multi-Channel Scheduling since device/driver don't support it.\n", __func__);
            retval = EINVAL;
        }
        break;
    case IEEE80211_DEVICE_MAX_AMSDU_SIZE:
        ic->ic_amsdu_max_size = val;
        wlan_pdev_set_amsdu_max_size(ic->ic_pdev_obj, val);
        break;
    case IEEE80211_DEVICE_PWRTARGET:
        ieee80211com_set_curchanmaxpwr(ic, val);
        break;
    case IEEE80211_DEVICE_P2P:
        if (val == 0) {
            ieee80211_ic_p2pDevEnable_clear(ic);
        }
        else if (ic->ic_caps_ext & IEEE80211_CEXT_P2P) {
            ieee80211_ic_p2pDevEnable_set(ic);
        }
        else {
            qdf_nofl_info("%s: Unable to enable P2P since device/driver don't support it.\n", __func__);
            retval = EINVAL;
        }
        break;

    case IEEE80211_DEVICE_OVERRIDE_SCAN_PROBERESPONSE_IE:
      if (val) {
          ieee80211_ic_override_proberesp_ie_set(ic);
      } else {
          ieee80211_ic_override_proberesp_ie_clear(ic);
      }
      break;
    case IEEE80211_DEVICE_2G_CSA:
        if (val == 0) {
            ieee80211_ic_2g_csa_clear(ic);
        } else {
            ieee80211_ic_2g_csa_set(ic);
        }
        break;

    default:
        qdf_nofl_info("%s: Error: invalid param=%d.\n", __func__, param);
    }
    return retval;

}

u_int32_t wlan_get_device_param(wlan_dev_t ic, ieee80211_device_param param)
{

    switch(param) {
    case IEEE80211_DEVICE_NUM_TX_CHAIN:
        return (ic->ic_num_tx_chain);
        break;
    case IEEE80211_DEVICE_NUM_RX_CHAIN:
        return (ic->ic_num_rx_chain);
        break;
    case IEEE80211_DEVICE_TX_CHAIN_MASK:
        return (ic->ic_tx_chainmask);
        break;
    case IEEE80211_DEVICE_RX_CHAIN_MASK:
        return (ic->ic_rx_chainmask);
        break;
    case IEEE80211_DEVICE_PROTECTION_MODE:
	return (ic->ic_protmode );
        break;
    case IEEE80211_DEVICE_BMISS_LIMIT:
    	return (ic->ic_bmisstimeout / ic->ic_intval);
        break;
    case IEEE80211_DEVICE_GREEN_AP_ENABLE_PRINT:
        return ic->ic_green_ap_get_print_level(ic);
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTMODE:
        return ic->ic_cwm_get_extprotmode(ic);
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTSPACING:
        return ic->ic_cwm_get_extprotspacing(ic);
        break;
    case IEEE80211_DEVICE_CWM_ENABLE:
        return ic->ic_cwm_get_enable(ic);
        break;
    case IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD:
        return ic->ic_cwm_get_extbusythreshold(ic);
        break;
    case IEEE80211_DEVICE_DOTH:
        return (ieee80211_ic_doth_is_set(ic));
        break;
    case IEEE80211_DEVICE_ADDBA_MODE:
        return ic->ic_addba_mode;
        break;
    case IEEE80211_DEVICE_COUNTRYCODE:
        return ieee80211_getCurrentCountry(ic);
        break;
    case IEEE80211_DEVICE_MULTI_CHANNEL:
        return (ieee80211_resmgr_getmode(ic->ic_resmgr)
                == IEEE80211_RESMGR_MODE_MULTI_CHANNEL);
        break;
    case IEEE80211_DEVICE_MAX_AMSDU_SIZE:
        return(ic->ic_amsdu_max_size);
        break;
    case IEEE80211_DEVICE_PWRTARGET:
        return (ieee80211com_get_curchanmaxpwr(ic));
        break;
    case IEEE80211_DEVICE_P2P:
        return (ieee80211_ic_p2pDevEnable_is_set(ic));
        break;
    case IEEE80211_DEVICE_OVERRIDE_SCAN_PROBERESPONSE_IE:
        return  ieee80211_ic_override_proberesp_ie_is_set(ic);
        break;
    case IEEE80211_DEVICE_2G_CSA:
        return (ieee80211_ic_2g_csa_is_set(ic));
        break;
    default:
        return 0;
    }
}

int wlan_get_device_mac_addr(wlan_dev_t ic, u_int8_t *mac_addr)
{
   IEEE80211_ADDR_COPY(mac_addr, ic->ic_myaddr);
   return EOK;
}

void wlan_device_note(struct ieee80211com *ic, const char *fmt, ...)
{
     char                   tmp_buf[OS_TEMP_BUF_SIZE];
     va_list                ap;
     va_start(ap, fmt);
     vsnprintf (tmp_buf,OS_TEMP_BUF_SIZE, fmt, ap);
     va_end(ap);
     qdf_nofl_info("%s",tmp_buf);
     ic->ic_log_text(ic,tmp_buf);
}

void wlan_get_vap_opmode_count(wlan_dev_t ic,
                               struct ieee80211_vap_opmode_count *vap_opmode_count)
{
    ieee80211_get_vap_opmode_count(ic, vap_opmode_count);
}

static void ieee80211_vap_iter_active_vaps(void *arg, struct ieee80211vap *vap)
{
    u_int16_t *pnactive = (u_int16_t *)arg;
       /* active vap check is used for assigning/updating vap channel with ic_curchan
	 so, it considers active vaps, and Vaps which are in CAC period */
    if (wlan_vdev_chan_config_valid(vap->vdev_obj) == QDF_STATUS_SUCCESS)
        ++(*pnactive);

}

static void ieee80211_vap_iter_vaps_up(void *arg, struct ieee80211vap *vap)
{
    u_int8_t *pnvaps_up = (u_int8_t *)arg;
    /* active vap check is used for assigning/updating vap channel with ic_curchan
       so, it considers active vaps, and Vaps which are in CAC period */
    if (vap->iv_opmode == IEEE80211_M_STA){
        if (wlan_vdev_chan_config_valid(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
            ++(*pnvaps_up);
        }
    } else {
        if (wlan_vdev_mlme_is_active(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
            ++(*pnvaps_up);
        }
    }
}

/* Returns the number of AP vaps up */
static void ieee80211_vap_iter_ap_vaps_up(void *arg, struct ieee80211vap *vap)
{
    u_int8_t *pnvaps_up = (u_int8_t *)arg;
    if ((wlan_vdev_mlme_is_active(vap->vdev_obj) == QDF_STATUS_SUCCESS)  &&
        (vap->iv_opmode == IEEE80211_M_HOSTAP)) {
        ++(*pnvaps_up);
    }
}

/* Returns the number of STA vaps up */
static void ieee80211_vap_iter_sta_vaps_up(void *arg, struct ieee80211vap *vap)
{
    u_int8_t *pnvaps_up = (u_int8_t *)arg;
    if ((wlan_vdev_mlme_is_active(vap->vdev_obj) == QDF_STATUS_SUCCESS) &&
        (vap->iv_opmode == IEEE80211_M_STA)) {
        ++(*pnvaps_up);
    }
}

/*
 * returns number of vaps active.
 */
u_int16_t
ieee80211_vaps_active(struct ieee80211com *ic)
{
    u_int16_t nactive=0;
    wlan_iterate_vap_list_lock(ic,ieee80211_vap_iter_active_vaps,(void *) &nactive);
    return nactive;
}

/*
 * returns number of vaps active and up.
 */
u_int8_t
ieee80211_get_num_vaps_up(struct ieee80211com *ic)
{
    u_int8_t nvaps_up=0;

    if (ic)
        wlan_iterate_vap_list_lock(ic,
                ieee80211_vap_iter_vaps_up,(void *) &nvaps_up);

    return nvaps_up;
}

u_int32_t
ieee80211_get_rnr_count(struct ieee80211com *ic)
{
    struct wlan_6ghz_rnr_global_cache *rnr;
    u_int32_t rnr_cnt;

    if (!ic) {
        qdf_err("IC is NULL");
        return 0;
    }
    if (!ic->ic_pdev_obj) {
        qdf_err("Pdev obj is NULL");
        return 0;
    }
    rnr = ieee80211_get_6ghz_cache(ic->ic_pdev_obj, RNR_6GHZ_CACHE_INDEX_LEGACY);
    rnr_cnt = rnr->rnr_cnt;

    return rnr_cnt;
}

void
ieee80211_display_rnr_stats(struct ieee80211com *ic)
{
    struct wlan_6ghz_rnr_global_cache *rnr;
    ieee80211_rnr_nbr_ap_info_t *ap_info;
    ieee80211_rnr_tbtt_info_set_t *p_tbtt_info;
    ieee80211_mlo_rnr_tbtt_info_set_t *p_tbtt_info_mlo;
    uint32_t rnr_cnt;
    uint8_t *p_rnr_buf;
    uint8_t count;
    /* Use mlo-rnr cache in case of 11be mode */
    bool is_mlo;
    int idx;

    if (!ic->ic_pdev_obj) {
        qdf_err("Pdev obj is NULL");
        return;
    }
    for (idx = RNR_6GHZ_CACHE_INDEX_0; idx < RNR_6GHZ_CACHE_INDEX_MAX; idx++) {
        count = 0;
        is_mlo = (idx == RNR_6GHZ_CACHE_INDEX_MLO);
        /* This function is invoked for 6 GHz radio */
        rnr = ieee80211_get_6ghz_cache(ic->ic_pdev_obj, is_mlo);
        rnr_cnt = rnr->rnr_cnt;

        p_rnr_buf = rnr->rnr_buf;
        p_rnr_buf += 2;

        /* Skip by Element Id and Element len */
        ap_info = (ieee80211_rnr_nbr_ap_info_t *) p_rnr_buf;
        p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *)
            ((uint8_t *)ap_info + RNR_NBR_AP_INFO_HDR_SIZE);

        qdf_info("RNR NBR AP INFO RNR_CNT[%d] [HDR]:", rnr_cnt);
        qdf_info("   F_TYPE: %d, F_NBR_AP: %d, RESERV: %d, I_CNT: %d, I_LEN: %d",
                ap_info->hdr_field_type,
                ap_info->hdr_filtered_nbr_ap,
                ap_info->hdr_reserved,
                ap_info->hdr_info_cnt,
                ap_info->hdr_info_len);

        qdf_info("RNR NBR AP INFO [OP_CLASS, CHANNEL]: [0x%x, 0x%x]\n",
                ap_info->op_class, ap_info->channel);
        if (!rnr->rnr_cnt)
            continue;

        while (count < rnr->rnr_cnt)
        {
            qdf_info(" RNR TBTT INFO SET [TBTT_OFFSET]: [0x%x]",
                    p_tbtt_info->tbtt_offset);
            qdf_info(" RNR TBTT INFO SET [BSSID]: [%pM]",
                    p_tbtt_info->bssid);
            qdf_info(" RNR TBTT INFO SET [S_SSID]: [0x%x]",
                    p_tbtt_info->short_ssid);
            qdf_info(" RNR TBTT INFO SET [BSS_PARAM]:");
            qdf_info(" OCT_REC: %d, S_SSID: %d, MBSSID_S: %d, TX_BSSID: %d",
                    p_tbtt_info->bss_params.oct_recommended,
                    p_tbtt_info->bss_params.same_ssid,
                    p_tbtt_info->bss_params.mbssid_set,
                    p_tbtt_info->bss_params.tx_bssid);
            qdf_info(" COLOC_LB: %d, PRB_R_20TU_A: %d, COLOC_AP: %d, RESERV: %d\n",
                    p_tbtt_info->bss_params.colocated_lower_band_ess,
                    p_tbtt_info->bss_params.probe_resp_20tu_active,
                    p_tbtt_info->bss_params.co_located_ap,
                    p_tbtt_info->bss_params.reserved);

            if (is_mlo) {
                /* Assign base structure pointer to deriver structure pointer */
                p_tbtt_info_mlo = (ieee80211_mlo_rnr_tbtt_info_set_t *) p_tbtt_info;

                p_tbtt_info_mlo++;
                /* Assign derived structure pointer back to base pointer with
                 * the assumption that base pointer will only stick to accessing
                 * fields from base structure and not from the derived structure
                 */
                p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *) p_tbtt_info_mlo;
            } else {
                p_tbtt_info++;
            }

            count++;
        }
    } /* for loop end */
}

#define GRP_ID_SHIFT_BITS 8
#define NVAPS_MASK    0x00ff
#define GROUP_ID_MASK 0xff00
/* ieee80211_vap_iter_beaconing_ap_vaps_up: iter function
 * to tell if AP is UP and beaconing
 * @arg: overloaded ip/op. MSB 8-bit is input group-id
 * and LSB 8-bit is output counter value to be updated
 * @vap: vap pointer of vap to check
 * Return: None
 */
static void ieee80211_vap_iter_beaconing_ap_vaps_up(void *arg, wlan_if_t vap)
{
    uint16_t *p_arg = (uint16_t *)arg;
    uint8_t cnt     = *p_arg & NVAPS_MASK;
    uint8_t grp_id  = *p_arg >> GRP_ID_SHIFT_BITS;

    if ((wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) &&
        (vap->iv_mbss.group_id == grp_id) &&
        ieee80211_mbss_is_beaconing_ap(vap)) {
        cnt++;
    }

    *p_arg = (*p_arg & GROUP_ID_MASK) | cnt;
}

/* ieee80211_mbssid_get_num_beaconing_ap_vaps_up: get count of AP VAPs
 * that are UP and HOSTAP and not special or monitor VAPs
 * @ic: ic handle
 * @grp_id: group-id
 * Return: count of beaconing AP vaps that are UP
 */
u_int8_t ieee80211_mbssid_get_num_beaconing_ap_vaps_up(struct ieee80211com *ic,
                                                uint8_t grp_id)
{
    uint8_t nvaps_up = 0;
    uint16_t overloaded_ip_op = (grp_id << GRP_ID_SHIFT_BITS);

    if (!ic->is_mbssid_enabled || !ic->ic_mbss_ctx)
        return nvaps_up;

    if (grp_id < ic->ic_mbss_ctx->max_ngroups) {
        wlan_iterate_vap_list_lock(ic,
                                   ieee80211_vap_iter_beaconing_ap_vaps_up,
                                   (void *) &overloaded_ip_op);
        nvaps_up = overloaded_ip_op & NVAPS_MASK;
    }
    return nvaps_up;
}

/*
 * Returns number of ap vaps active and up.
 */
u_int8_t
ieee80211_get_num_ap_vaps_up(struct ieee80211com *ic)
{
    u_int8_t nvaps_up=0;
    wlan_iterate_vap_list_lock(ic,ieee80211_vap_iter_ap_vaps_up,(void *) &nvaps_up);
    return nvaps_up;
}

/*
 * Returns number of sta vaps active and up.
 */
u_int8_t ieee80211_get_num_sta_vaps_up(struct ieee80211com *ic)
{
    u_int8_t nvaps_up=0;
    wlan_iterate_vap_list_lock(ic, ieee80211_vap_iter_sta_vaps_up,
            (void *)&nvaps_up);
    return nvaps_up;
}

static void
ieee80211_iter_vap_opmode(void *arg, struct ieee80211vap *vaphandle)
{
    struct ieee80211_vap_opmode_count    *vap_opmode_count = arg;
    enum ieee80211_opmode                opmode = ieee80211vap_get_opmode(vaphandle);

    vap_opmode_count->total_vaps++;

    switch (opmode) {
    case IEEE80211_M_IBSS:
        vap_opmode_count->ibss_count++;
        break;

    case IEEE80211_M_STA:
        vap_opmode_count->sta_count++;
        break;

    case IEEE80211_M_WDS:
        vap_opmode_count->wds_count++;
        break;

    case IEEE80211_M_AHDEMO:
        vap_opmode_count->ahdemo_count++;
        break;

    case IEEE80211_M_HOSTAP:
        vap_opmode_count->ap_count++;
        break;

    case IEEE80211_M_MONITOR:
        vap_opmode_count->monitor_count++;
        break;

    case IEEE80211_M_BTAMP:
        vap_opmode_count->btamp_count++;
        break;

    default:
        vap_opmode_count->unknown_count++;

        qdf_nofl_info("%s vap=%pK unknown opmode=%d\n",
            __func__, vaphandle, opmode);
        break;
    }
}

void
ieee80211_get_vap_opmode_count(struct ieee80211com *ic,
                               struct ieee80211_vap_opmode_count *vap_opmode_count)
{
    wlan_iterate_vap_list_lock(ic, ieee80211_iter_vap_opmode, (void *) vap_opmode_count);
}

static void
ieee80211_vap_iter_associated(void *arg, struct ieee80211vap *vap)
{
    u_int8_t *pis_sta_associated = (u_int8_t *)arg;
    if (vap->iv_opmode == IEEE80211_M_STA) {
        if (wlan_vdev_allow_connect_n_tx(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
                (*pis_sta_associated) = 1;
        }
    }
}

/*
 * returns 1 if STA vap is not in associated state else 0
 */
u_int8_t
ieee80211_sta_assoc_in_progress(struct ieee80211com *ic)
{
    u_int8_t in_progress = 0;
    uint8_t num_sta;

    num_sta = wlan_mbss_num_sta(ic->ic_pdev_obj);
    if (num_sta) {
        u_int8_t is_sta_associated = 0;

        wlan_iterate_vap_list_lock(ic,ieee80211_vap_iter_associated,(void *) &is_sta_associated);
        if (!is_sta_associated)
            in_progress = 1;
    }

    return in_progress;
}

struct ieee80211_iter_vaps_ready_arg {
    u_int8_t num_sta_vaps_ready;
    u_int8_t num_ibss_vaps_ready;
    u_int8_t num_ap_vaps_ready;
};

static void ieee80211_vap_iter_up_vaps(void *arg, wlan_if_t vap)
{
    struct ieee80211_iter_vaps_ready_arg *params = (struct ieee80211_iter_vaps_ready_arg *) arg;
    if (wlan_vdev_is_up_active_state(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        switch(ieee80211vap_get_opmode(vap)) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_BTAMP:
            params->num_ap_vaps_ready++;
            break;

        case IEEE80211_M_IBSS:
            params->num_ibss_vaps_ready++;
            break;

        case IEEE80211_M_STA:
            params->num_sta_vaps_ready++;
            break;

        default:
            break;

        }
    }
}

static void ieee80211_vap_iter_ready_vaps(void *arg, wlan_if_t vap)
{
    struct ieee80211_iter_vaps_ready_arg *params = (struct ieee80211_iter_vaps_ready_arg *) arg;
    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        switch(ieee80211vap_get_opmode(vap)) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_BTAMP:
            params->num_ap_vaps_ready++;
            break;

        case IEEE80211_M_IBSS:
            params->num_ibss_vaps_ready++;
            break;

        case IEEE80211_M_STA:
            params->num_sta_vaps_ready++;
            break;

        default:
            break;

        }
    }
}

/*
 * returns number of vaps up
 */
u_int16_t
ieee80211_vaps_up(struct ieee80211com *ic, enum ieee80211_opmode opmode)
{
    struct ieee80211_iter_vaps_ready_arg params;
    u_int16_t nready = 0;
    OS_MEMZERO(&params, sizeof(params));
    wlan_iterate_vap_list_lock(ic,ieee80211_vap_iter_up_vaps,(void *) &params);
    switch(opmode) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_BTAMP:
            nready = params.num_ap_vaps_ready;
            break;

        case IEEE80211_M_IBSS:
            nready = params.num_ibss_vaps_ready;
            break;

        case IEEE80211_M_STA:
            nready = params.num_sta_vaps_ready;
            break;

        default:
            break;
    }
    return nready;
}

/*
 * returns number of vaps ready.
 */
u_int16_t
ieee80211_vaps_ready(struct ieee80211com *ic, enum ieee80211_opmode opmode)
{
    struct ieee80211_iter_vaps_ready_arg params;
    u_int16_t nready = 0;
    OS_MEMZERO(&params, sizeof(params));
    wlan_iterate_vap_list_lock(ic,ieee80211_vap_iter_ready_vaps,(void *) &params);
    switch(opmode) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_BTAMP:
            nready = params.num_ap_vaps_ready;
            break;

        case IEEE80211_M_IBSS:
            nready = params.num_ibss_vaps_ready;
            break;

        case IEEE80211_M_STA:
            nready = params.num_sta_vaps_ready;
            break;

        default:
            break;
    }
    return nready;
}

static struct ath_fips_cmd* ol_ath_set_default_disa_buf(void)
{
    struct ath_fips_cmd *fips_buf = NULL;
    u_int8_t default_key[] = { 0xc9, 0x7c, 0x1f, 0x67,
                               0xce, 0x37, 0x11, 0x85,
                               0x51, 0x4a, 0x8a, 0x19,
                               0xf2, 0xbd, 0xd5, 0x2f
                             };
    u_int8_t default_pn[] = {0xB5, 0x03, 0x97, 0x76, 0xE7, 0x0C};
    u_int8_t default_mac_header[] = { 0x08, 0x48, 0xc3, 0x2c,
                                      0x0f, 0xd2, 0xe1, 0x28,
                                      0xa5, 0x7c, 0x50, 0x30,
                                      0xf1, 0x84, 0x44, 0x08,
                                      0xab, 0xae, 0xa5, 0xb8,
                                      0xfc, 0xba, 0x80, 0x33,
                                      0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00
                                    };
    u_int8_t default_payload[] = { 0xf8, 0xba, 0x1a, 0x55,
                                   0xd0, 0x2f, 0x85, 0xae,
                                   0x96, 0x7b, 0xb6, 0x2f,
                                   0xb6, 0xcd, 0xa8, 0xeb,
                                   0x7e, 0x78, 0xa0, 0x50
                                 };

    fips_buf = (struct ath_fips_cmd *) qdf_mem_malloc(sizeof(struct ath_fips_cmd)
                + (sizeof(default_payload) - sizeof(u_int32_t)));
    if (!fips_buf)  {
        return NULL;
    }
    fips_buf->fips_cmd = 1;    /* 1 - encrypt/ 2 - decrypt*/
    fips_buf->mode = 2;
    fips_buf->key_idx = 0;
    fips_buf->key_cipher = 4;
    fips_buf->key_len = sizeof(default_key);
    memcpy(fips_buf->key, default_key, fips_buf->key_len);
    fips_buf->header_len = sizeof(default_mac_header);
    memcpy(fips_buf->header, default_mac_header, fips_buf->header_len);
    memcpy(fips_buf->pn, default_pn, sizeof(default_pn));
    fips_buf->data_len = sizeof(default_payload);
    memcpy(fips_buf->data, default_payload, fips_buf->data_len);

    return fips_buf;
}

static struct ath_fips_cmd* ol_ath_set_default_fips_buf(void)
{
    struct ath_fips_cmd *fips_buf = NULL;
    u_int8_t default_key[] =  { 0x2b, 0x7e, 0x15, 0x16,
                                0x28, 0xae, 0xd2, 0xa6,
                                0xab, 0xf7, 0x15, 0x88,
                                0x09, 0xcf, 0x4f, 0x3c
                              };
    u_int8_t default_data[] = { 0xf0, 0xf1, 0xf2, 0xf3,
                                0xf4, 0xf5, 0xf6, 0xf7,
                                0xf8, 0xf9, 0xfa, 0xfb,
                                0xfc, 0xfd, 0xfe, 0xff
                              };
    u_int8_t default_iv[] =   { 0xf0, 0xf1, 0xf2, 0xf3,
                                0xf4, 0xf5, 0xf6, 0xf7,
                                0xf8, 0xf9, 0xfa, 0xfb,
                                0xfc, 0xfd, 0xfe, 0xff
                              };
    fips_buf = (struct ath_fips_cmd *) qdf_mem_malloc(sizeof(struct ath_fips_cmd)
                + (sizeof(default_data) - sizeof(u_int32_t)));
    if (!fips_buf)  {
        return NULL;
    }
    fips_buf->fips_cmd = 1;    /* 1 - encrypt/ 2 - decrypt*/
    fips_buf->mode = 1;
    fips_buf->key_len = sizeof(default_key);
    memcpy(fips_buf->key, default_key, fips_buf->key_len);
    fips_buf->data_len = sizeof(default_data);
    memcpy(fips_buf->data, default_data, fips_buf->data_len);
    memcpy(fips_buf->iv, default_iv, sizeof(default_iv));
    return fips_buf;
}

int wlan_set_fips(wlan_if_t vap, void *args)
{

    struct ath_fips_cmd *fips_buf = (struct ath_fips_cmd *)args;
    struct ieee80211com *ic = vap->iv_ic;
    int retval = -1;

    if (!ic || !ic->ic_fips_test) {
        qdf_nofl_info("\n %s:%d fips_test function not supported", __func__, __LINE__);
        return -EINVAL;
    }

    if (fips_buf != NULL) {
        if (fips_buf->mode == 1) {
            if(!*(fips_buf->key)) {
                qdf_nofl_info("\n %s:%d missing key", __func__, __LINE__);
                return -EINVAL;
            }
        } else if (fips_buf->mode == 2) {
            if (!*(fips_buf->key)) {
                qdf_nofl_info("\n %s:%d disa key missing", __func__, __LINE__);
                return -EINVAL;
            }
        }
    }
    retval = ic->ic_fips_test(ic, vap, fips_buf);
    return retval;
}

int wlan_set_fips_extend(wlan_if_t vap, void *args)
{

    struct ath_fips_extend_cmd *fips_buf = (struct ath_fips_extend_cmd *)args;
    struct ieee80211com *ic = vap->iv_ic;
    int retval = -1;

    if (!ic || !ic->ic_fips_extend_test) {
        qdf_nofl_info("\n %s:%d fips_test function not supported", __func__, __LINE__);
        return -EINVAL;
    }

    if (fips_buf != NULL) {
        if(!*(fips_buf->cmd_params.key)) {
            qdf_nofl_info("\n %s:%d missing key", __func__, __LINE__);
            return -EINVAL;
        }
    }
    retval = ic->ic_fips_extend_test(ic, vap, fips_buf);
    return retval;
}

int wlan_fips_mode_set(wlan_if_t vap, uint32_t fips_mode_set)
{
    struct ieee80211com *ic = vap->iv_ic;
    int retval = -1;

    if (!ic || !ic->ic_fips_mode_set) {
        qdf_nofl_info("\n %s:%d fips_mode_set function not supported", __func__, __LINE__);
        return -EINVAL;
    }
    if (ic->ic_is_fips_mode == fips_mode_set) {
        qdf_nofl_info("\n %s:%d ic_is_fips_mode:%d already",
                      __func__, __LINE__, fips_mode_set);
        return 0;
    }

    retval = ic->ic_fips_mode_set(ic, vap, fips_mode_set);
    if (retval == 0) {
        ic->ic_is_fips_mode = fips_mode_set;
        qdf_nofl_info("\n %s:%d ic_is_fips_mode:%d updated",
                      __func__, __LINE__, fips_mode_set);
    }
    return retval;
}

static QDF_STATUS config_cmd_resp_show(qdf_debugfs_file_t file, void *arg)
{
    int index;
    int pos, count = MAX_CONFIG_COMMAND;
    uint64_t secs, usecs;
    config_cmd_log_cxt_t* config_cmd_cxt = (config_cmd_log_cxt_t*)arg;

    if(!config_cmd_cxt->init_flag)
        return QDF_STATUS_E_FAILURE;

    qdf_semaphore_acquire(&config_cmd_cxt->entry_lock);

    if(config_cmd_cxt->entry_count == 0) {
        qdf_semaphore_release(&config_cmd_cxt->entry_lock);
        qdf_debugfs_printf(file, "No entries in buffer\n");
        return QDF_STATUS_SUCCESS;
    }

    index = config_cmd_cxt->entry_index;
    if(config_cmd_cxt->entry_count < MAX_CONFIG_COMMAND){
        pos = index - 1;
        count = config_cmd_cxt->entry_count;
    } else {
        if(index == 0)
            pos = MAX_CONFIG_COMMAND - 1;
        else
            pos = index - 1;
    }

    /* Iterate through circular buffer and print entries*/
    while(count){
        config_entry_t* entry = &config_cmd_cxt->entry[pos];

        qdf_log_timestamp_to_secs(entry->time, &secs, &usecs);
        qdf_debugfs_printf(file, "\n TIME = [%llu.%06llu]\n",
                           secs, usecs);
        if(entry->type == CONFIG_TYPE_CMD)
            qdf_debugfs_printf(file, "CMD ");
        else if(entry->type == CONFIG_TYPE_RESP)
            qdf_debugfs_printf(file, "RESP ");

        qdf_debugfs_printf(file, "%s param %d val %d\n", entry->interface,
                           entry->param, entry->val);

        /* Reached the end, roll over */
        if(pos == 0)
            pos = MAX_CONFIG_COMMAND - 1;
        else
            pos--;

        count--;
    }

    qdf_semaphore_release(&config_cmd_cxt->entry_lock);

    return QDF_STATUS_SUCCESS;
}

void config_cmd_resp_log(ol_ath_soc_softc_t *soc, uint8_t type, char* interface, int id, int val)
{
    int index;
    config_entry_t* entry;
    config_cmd_log_cxt_t* config_cmd_cxt = &soc->config_cmd_cxt;

    if(!config_cmd_cxt->init_flag)
        return;

    qdf_semaphore_acquire(&config_cmd_cxt->entry_lock);
    index = config_cmd_cxt->entry_index;

    if(index == MAX_CONFIG_COMMAND) {
        index = 0;
        /* Reached end of circular buffer */
        config_cmd_cxt->entry_index = 0;
    }

    entry = &config_cmd_cxt->entry[index];

    entry->type = type;
    entry->param = id;
    entry->val = val;
    snprintf(entry->interface, sizeof(entry->interface), interface);
    entry->time = qdf_get_log_timestamp();

    config_cmd_cxt->entry_index++;
    config_cmd_cxt->entry_count++;
    qdf_semaphore_release(&config_cmd_cxt->entry_lock);
}

qdf_export_symbol(config_cmd_resp_log);

void config_cmd_resp_init(ol_ath_soc_softc_t *soc)
{
    char file[32];
    char dir[32];
    qdf_dentry_t ret;
    int  perm = (QDF_FILE_USR_READ | QDF_FILE_GRP_READ | QDF_FILE_OTH_READ);
    config_cmd_log_cxt_t* config_cmd_cxt = &soc->config_cmd_cxt;

    /* Check if feature is supported */
    config_cmd_cxt->feature_init = cfg_get(soc->psoc_obj, CFG_COMMAND_LOGGING_SUPPORT);
    if (!config_cmd_cxt->feature_init)
        return;

    config_cmd_cxt->entry = qdf_mem_malloc(sizeof(config_entry_t)*MAX_CONFIG_COMMAND);
    if(!config_cmd_cxt->entry)
        return;

    snprintf(dir, sizeof(dir), "CONFIG");
    snprintf(file, sizeof(file), "cmdresp_SOC%d", soc->soc_idx);

    if (ol_global_soc[0] && ol_global_soc[0]->config_cmd_cxt.config_log_debugfs_dir)
        config_cmd_cxt->config_log_debugfs_dir =
            ol_global_soc[0]->config_cmd_cxt.config_log_debugfs_dir;
    else
        config_cmd_cxt->config_log_debugfs_dir = qdf_debugfs_create_dir(dir, NULL);

    config_cmd_cxt->ops.show = config_cmd_resp_show;
    config_cmd_cxt->ops.priv = config_cmd_cxt;

    ret = qdf_debugfs_create_file_simplified(file, perm,
                    config_cmd_cxt->config_log_debugfs_dir,
                    &config_cmd_cxt->ops);

    qdf_semaphore_init(&config_cmd_cxt->entry_lock);
    config_cmd_cxt->init_flag = 1;
}

void config_cmd_resp_deinit(ol_ath_soc_softc_t *soc)
{
    config_cmd_log_cxt_t* config_cmd_cxt = &soc->config_cmd_cxt;

    if(!config_cmd_cxt->init_flag)
        return;

    qdf_mem_free(config_cmd_cxt->entry);
    if (ol_global_soc[0] && ol_global_soc[0]->config_cmd_cxt.config_log_debugfs_dir) {
        qdf_debugfs_remove_dir_recursive(config_cmd_cxt->config_log_debugfs_dir);
        ol_global_soc[0]->config_cmd_cxt.config_log_debugfs_dir = NULL;
    }

    qdf_semaphore_release(&config_cmd_cxt->entry_lock);
}
qdf_export_symbol(config_cmd_resp_init);
qdf_export_symbol(config_cmd_resp_deinit);

/*
 * This function returns true if phymode is set as 11be for the
 * vap.
 */
bool wlan_is_6g_11be_mlo(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    bool is_11be_mlo = false;
    struct wlan_channel *des_chan = NULL;

    if (!ic) {
        qdf_err("ic is NULL");
        goto exit;
    }
    /* In general, all vdevs in a radio must have the same desired phymode mode.
     * As part of repurpose mixed mode configuration, vaps can have different.
     * phymode either 11BE or 11ax.
     * In MBSSID enabled, mixed mode allowed at set of vdevs in a group.
     * Mixed mode configuration allowed when all vdevs in a set configured
     * with same phymode. So considered tx vdev phymode as reference.
     */
    if (ic->is_mbssid_enabled && ic->ic_mbss_ctx) {
        struct mbssid_info *ic_mbss = &ic->ic_mbss_ctx->ic_mbss[vap->iv_mbss.group_id];
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            des_chan = wlan_vdev_get_active_channel(ic_mbss->transmit_vap->vdev_obj);
        } else {
            des_chan = wlan_vdev_get_active_channel(vap->vdev_obj);
        }
        if (des_chan)
            is_11be_mlo = ieee80211_is_phymode_11be(wlan_vdev_get_ieee_phymode(des_chan->ch_phymode));
    }

exit:
    qdf_debug("<< %d", is_11be_mlo);
    return is_11be_mlo;
}

#ifdef WLAN_FEATURE_11BE_MLO
static QDF_STATUS
t2lm_info_check_non_default_mappings(struct wlan_t2lm_info *t2lm_info,
                                     int t2lm_info_size,
                                     uint8_t link_id)
{
    struct wlan_t2lm_info *info;
    int t2lm_dir, tid;

    for (t2lm_dir = 0; t2lm_dir < t2lm_info_size; t2lm_dir++) {
        info = &t2lm_info[t2lm_dir];

        /* Invalidate this entry */
        if (info->direction == WLAN_T2LM_INVALID_DIRECTION)
            continue;

        /* We only are interested in non-default mappings */
        if (info->default_link_mapping)
            continue;

        for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++)
            if (info->ieee_link_map_tid[tid] & BIT(link_id))
                return QDF_STATUS_SUCCESS;
    }

    return QDF_STATUS_E_NOENT;
}

/**
 * check_peer_level_t2lm_non_default_mappings(): Check if the given MLO peer has
 * any peer level non-default mappings set to given link
 * @ml_dev: ML device
 * @iter_ml_peer: ML peer
 * @arg: pointer to link_id
 *
 * Return: If non-default mappings are found, then QDF_STATUS_SUCCESS.
 *         Else, QDF_STATUS_E_NOENT.
 */
static QDF_STATUS
check_peer_level_t2lm_non_default_mappings(struct wlan_mlo_dev_context *ml_dev,
                                           void *iter_ml_peer,
                                           void *arg)
{
    uint8_t link_id = *(uint8_t *)arg;
    struct wlan_mlo_peer_context *ml_peer = iter_ml_peer;
    QDF_STATUS status;

    /* Check negotiated mappings */
    status = t2lm_info_check_non_default_mappings(
                 ml_peer->t2lm_policy.t2lm_negotiated_info.t2lm_info,
                 WLAN_T2LM_MAX_DIRECTION,
                 link_id);
    if (QDF_IS_STATUS_SUCCESS(status)) {
        mlo_err("Negotiated non-default mapping found on MLO peer %s",
                ether_sprintf(ml_peer->peer_mld_addr.bytes));
        return QDF_STATUS_SUCCESS;
    }

    /* Check ongoing mappings */
    status = t2lm_info_check_non_default_mappings(
                 ml_peer->t2lm_policy.ongoing_tid_to_link_mapping.t2lm_info,
                 WLAN_T2LM_MAX_DIRECTION,
                 link_id);
    if (QDF_IS_STATUS_SUCCESS(status)) {
        mlo_err("Ongoing non-default mapping found on MLO peer %s",
                ether_sprintf(ml_peer->peer_mld_addr.bytes));
        return QDF_STATUS_SUCCESS;
    }

    return QDF_STATUS_E_NOENT;
}

/**
 * validate_link_removal_cmd_t2lm_peer_level(): Validate link removal command
 * against peer level T2LM mappings
 * @vdev: vdev corresponding to the link to be removed
 *
 * Return: QDF_STATUS_SUCCESS if validation passes, else QDF_STATUS_E_FAILURE
 */
static QDF_STATUS
validate_link_removal_cmd_t2lm_peer_level(struct wlan_objmgr_vdev *vdev)
{
    uint8_t removal_link_id;
    QDF_STATUS status;

    removal_link_id = wlan_vdev_get_link_id(vdev);

    /* Go over all MLO peers on this MLD and check for non-default mappings. */
    status = wlan_mlo_iterate_ml_peerlist(
                 vdev->mlo_dev_ctx,
                 check_peer_level_t2lm_non_default_mappings,
                 &removal_link_id);

    /* If there are no non-default mappings to this link, it can be removed */
    if (status == QDF_STATUS_E_NOENT)
       return QDF_STATUS_SUCCESS;

    return QDF_STATUS_E_FAILURE;
}

/**
 * validate_link_removal_cmd_t2lm_global(): Validate link removal command
 * against globally advertised T2LM mappings
 * @vdev: vdev corresponding to the link to be removed
 *
 * Return: QDF_STATUS_SUCCESS if validation passes, else QDF_STATUS_E_FAILURE
 */
static QDF_STATUS
validate_link_removal_cmd_t2lm_global(struct wlan_objmgr_vdev *vdev)
{
    struct wlan_mlo_dev_context *mlo_dev_ctx;
    uint8_t removal_link_id;
    QDF_STATUS status;

    mlo_dev_ctx = vdev->mlo_dev_ctx;
    if (!mlo_dev_ctx) {
        qdf_err("MLO dev context is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    removal_link_id = wlan_vdev_get_link_id(vdev);

    /* Check the established mappings */
    status = t2lm_info_check_non_default_mappings(
                 &mlo_dev_ctx->t2lm_ctx.established_t2lm.t2lm,
                 1,
                 removal_link_id);
    if (QDF_IS_STATUS_SUCCESS(status)) {
        mlo_err("Non-default mapping found in the established T2LM");
        return QDF_STATUS_E_FAILURE;
    }

    /* Check the upcoming mappings */
    status = t2lm_info_check_non_default_mappings(
                 &mlo_dev_ctx->t2lm_ctx.upcoming_t2lm.t2lm,
                 1,
                 removal_link_id);
    if (QDF_IS_STATUS_SUCCESS(status)) {
        mlo_err("Non-default mapping found in the upcoming T2LM");
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

/**
 * validate_link_removal_cmd_t2lm(): Validate link removal command wrt T2LM
 * @vdev: vdev corresponding to the link to be removed
 *
 * Return: QDF_STATUS_SUCCESS if validation passes, else QDF_STATUS_E_FAILURE
 */
static QDF_STATUS
validate_link_removal_cmd_t2lm(struct wlan_objmgr_vdev *vdev)
{
    QDF_STATUS status;

    /* Validate against peer level T2LM mappings */
    status = validate_link_removal_cmd_t2lm_peer_level(vdev);
    if (QDF_IS_STATUS_ERROR(status)) {
        mlo_err("Peer level non-default mapping(s) are found on the link to be "
                "removed. Hence, this link can't be removed.");
        return status;
    }

    /* Validate against globally advertised T2LM mappings */
    status = validate_link_removal_cmd_t2lm_global(vdev);
    if (QDF_IS_STATUS_ERROR(status)) {
        mlo_err("Globally advertised non-default mapping(s) are found on the "
                "link to be removed. Hence, this link can't be removed.");
    }

    return status;
}

QDF_STATUS wlan_mlo_validate_link_removal_cmd(wlan_if_t vap,
                    uint32_t tbtt_count, bool non_tx_vdev_exception)
{
    struct wlan_objmgr_vdev *vdev;
    uint16_t vdev_count;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    int vdev_dix = 0;
    struct wlan_objmgr_vdev *partner_vdev;
    QDF_STATUS status, ref_status;
    wlan_if_t partner_vap;

#define AP_REMOVAL_TIMER_TBTT_MAX (0xFFFF)
    if (tbtt_count > AP_REMOVAL_TIMER_TBTT_MAX ) {
        mlo_err("TBTT count should be less than %u", AP_REMOVAL_TIMER_TBTT_MAX);
        return QDF_STATUS_E_RANGE;
    }

    if (!vap) {
        mlo_err("vap is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        mlo_err("vdev is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
        mlo_err("vdev is not an MLO AP");
        return QDF_STATUS_E_INVAL;
    }

    if (wlan_vdev_chan_config_valid(vdev) != QDF_STATUS_SUCCESS) {
        mlo_err("vdev is not active");
        return QDF_STATUS_E_INVAL;
    }

    mlo_ap_get_active_vdev_list(vdev, &vdev_count, wlan_vdev_list);

    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu,"
                "investigate the possible corruption",
                vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
        status = QDF_STATUS_E_FAILURE;
        goto release_ref;
    }

    if (vdev_count <= 1) {
        mlo_err("Link removal can only be used on MLD with atleast 2 active AP VAPs");
        status = QDF_STATUS_E_PERM;
        goto release_ref;
    }

    /* Reject link removal request if it is already ongoing on any of
     * the links in the target MLD.
     */
    for (; vdev_dix < vdev_count; ++vdev_dix) {
        partner_vdev = wlan_vdev_list[vdev_dix];
        partner_vap = wlan_vdev_mlme_get_ext_hdl(partner_vdev);

        if (!partner_vap) {
            mlo_err("partner vap corresponding to vdev:%pK is null", partner_vdev);
            status = QDF_STATUS_E_FAILURE;
            goto release_ref;
        }

        if (wlan_vdev_mlme_op_flags_get(
                partner_vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS)) {
            /* Provide an expection to non_tx_vdev if the link-removal
             * request from user is for a Tx-vap (as Tx-vap removal triggers
             * link-removal for non-Tx vap as well - it is by design to resend
             * the link-removal req for this non-Tx vap even if it was sent
             * earlier as part of the ongoing removal-procedure.)
             *
             * Note that the second condition ensures that this exception
             * is provided only for the non-Tx vap itself and not its partners.
             */
            if (!(non_tx_vdev_exception && vdev == partner_vdev)) {
                mlo_err("Link removal request is in progress on partner link %s",
                        partner_vap->iv_netdev_name);
                status = QDF_STATUS_E_BUSY;
                goto release_ref;
            }
        }

        if (wlan_vdev_mlme_op_flags_get(partner_vdev, WLAN_VDEV_OP_MLO_LINK_ADD)) {
            mlo_err("Link add request is already in progress on partner link %s",
                    partner_vap->iv_netdev_name);
            status = QDF_STATUS_E_NOSUPPORT;
            goto release_ref;
        }
    }

    status = validate_link_removal_cmd_t2lm(vdev);

release_ref:
    if (vdev_count) {
        ref_status = ieee80211_mlo_release_vdev_list_refs(
                           wlan_vdev_list,
                           QDF_ARRAY_SIZE(wlan_vdev_list),
                           vdev_count, 0);
        if (QDF_IS_STATUS_ERROR(ref_status)) {
            mlo_err("Couldn't release refs of ML vdevs");
            wlan_vdev_mlme_op_flags_clear(
                vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS);
            return ref_status;
        }
    }

    return status;
}

static void*
ieee80211_ucfg_get_apremove_ml_reconfig_ie(wlan_if_t vap,
                                  uint16_t tbtt_count,
                                  uint16_t *ml_reconfig_ie_len)
{
    struct ieee80211_framing_extractx extractx;
    uint8_t *ml_reconfig_ie;
    uint8_t *retptr;

    *ml_reconfig_ie_len = 0;

    ml_reconfig_ie = qdf_mem_malloc(IEEE80211_ML_RV_MAX_LEN);
    if (!ml_reconfig_ie) {
        mlo_err("Memory allocation for ML reconfig IE failed");
        return NULL;
    }

    extractx.fectx_mlreconfreq_purpose = IEEE80211_MLRECONFREQ_PURPOSE_APREMOVE;
    extractx.fectx_mlreconfreq_removetimer.num_links = 1;
    extractx.fectx_mlreconfreq_removetimer.link_info[0].removetimer_tbtt =
        tbtt_count;
    extractx.fectx_mlreconfreq_removetimer.link_info[0].linkid =
                                           wlan_vdev_get_link_id(vap->vdev_obj);

    retptr = ieee80211_add_multilink_ie(ml_reconfig_ie, IEEE80211_ML_RV_MAX_LEN,
                                        NULL, 0, vap->iv_bss, vap->iv_ic,
                                        IEEE80211_FC0_SUBTYPE_BEACON, true,
                                        WLAN_ML_VARIANT_RECONFIG,
                                        &extractx, NULL, NULL);
    if (!retptr) {
        mlo_err("Unable to construct ML reconfiguration IE");
        qdf_mem_free(ml_reconfig_ie);
        return NULL;
    }

    *ml_reconfig_ie_len = (uintptr_t)retptr - (uintptr_t)ml_reconfig_ie;

    return ml_reconfig_ie;
}

QDF_STATUS wlan_mlo_rmsl_link_remove_update(wlan_if_t vap)
{
    struct wlan_objmgr_vdev *vdev;
    struct ieee80211com *ic;
    int retv = 0;
    uint16_t mld_vdev_cnt = 0;
    struct wlan_objmgr_vdev *mlo_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    struct wlan_mlo_dev_context *ml_dev;
    uint16_t num_links = 0;
    struct ieee80211vap *tmp_vap = NULL;
    uint8_t i;

    if (!vap) {
        mlo_err("VAP is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    ic = vap->iv_ic;

    if (!ic) {
        mlo_err("IC Object is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    vdev = vap->vdev_obj;

    if (!vdev) {
        mlo_err("VDEV object is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    ml_dev = vdev->mlo_dev_ctx;

    if (!ml_dev) {
        mlo_err("MLD DEV CONTEXT is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    ieee80211_mlo_ap_get_beaconing_vdev_list(vdev, &mld_vdev_cnt, mlo_vdev_list);
    if ((ieee80211_mlo_release_vdev_list_refs(mlo_vdev_list,
                                              QDF_ARRAY_SIZE(mlo_vdev_list),
                                              mld_vdev_cnt, 0)) != QDF_STATUS_SUCCESS) {
         mlo_err("Error while trying to release VDEV references - investigate. This could lead to system issues.");
         return QDF_STATUS_E_FAILURE;
    }

    if (!mld_vdev_cnt) {
        mlo_err("Number of VDEVs under MLD is reported as 0");
        return QDF_STATUS_E_FAILURE;
    }

    mlo_debug("mld_vdev_cnt: %d", mld_vdev_cnt);

    /* Decrement the max recommended active links when the value is
     * greater than the vdev count and greater than default value
     */
    if (mld_vdev_cnt == ml_dev->mlo_max_recom_simult_links) {

        /* If there is only one VDEV in the AP-MLD after link delete,
         * AP-MLD will advertise the value as "0"
         */
        if (mld_vdev_cnt == WLAN_MLO_RMSL_MIN_VDEV_COUNT) {
            ml_dev->mlo_max_recom_simult_links = WLAN_MLO_RMSL_SINGLE_LINK;
        } else {
            ml_dev->mlo_max_recom_simult_links--;
        }

    } else {
        ml_dev->mlo_max_recom_simult_links =
                                             MIN(mld_vdev_cnt,
                                                 ml_dev->mlo_max_recom_simult_links);
    }

    /* Send RMSL value update to FW on all VDEV's of AP-MLD */

    mlo_ap_get_active_vdev_list(vdev, &num_links, mlo_vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return QDF_STATUS_E_INVAL;
    }

    if (num_links > QDF_ARRAY_SIZE(mlo_vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                num_links, QDF_ARRAY_SIZE(mlo_vdev_list));
        for (i = 0; i < QDF_ARRAY_SIZE(mlo_vdev_list); i++) {
             mlo_release_vdev_ref(mlo_vdev_list[i]);
        }
        return QDF_STATUS_E_INVAL;
    }

    for (i =0; i < num_links; i++) {

         tmp_vap = wlan_vdev_get_mlme_ext_obj(mlo_vdev_list[i]);

         if (!tmp_vap) {
             mlo_release_vdev_ref(mlo_vdev_list[i]);
             continue;
         }

         retv = ic->ic_vap_set_param(tmp_vap,
                                     IEEE80211_CONFIG_MLO_MAX_RECOM_ACTIVE_LINKS,
                                     ml_dev->mlo_max_recom_simult_links);
         if (QDF_IS_STATUS_ERROR(retv)) {
             mlo_err("Error Sending RMSL link delete update WMI to FW for VAP: %s",
                      tmp_vap->iv_netdev_name);
             mlo_release_vdev_ref(mlo_vdev_list[i]);
             continue;
         }
         mlo_release_vdev_ref(mlo_vdev_list[i]);
    }

    return QDF_STATUS_SUCCESS;

}

QDF_STATUS wlan_mlo_link_removal_cmd(wlan_if_t vap,
                        uint32_t tbtt_count, bool non_tx_vdev_exception)
{
    QDF_STATUS status;
    struct wlan_objmgr_vdev *vdev;
    struct mlo_link_removal_cmd_params params;
    void *ml_reconfig_ie;
    uint16_t ml_reconfig_ie_len;
    struct ieee80211com *ic;
    struct mlo_link_removal_ctx *removal_ctx;
    bool skip_pumac_incremented = false;

    /* Following variable is closely-coupled with non_tx_vdev_exception
     * and marks if there is a removal already ongoing for a particular
     * non-Tx vap when non_tx_vdev_exception is true.
     */
    bool is_non_tx_removal_ongoing = false;

    if (!vap || !vap->vdev_obj) {
        mlo_err("vap or vdev_obj is NULL!!");
        return QDF_STATUS_E_FAILURE;
    }

    vdev = vap->vdev_obj;
    /* Mark if non-Tx vap removal already ongoing when non_tx_vdev_exception
     * is true
     */
    is_non_tx_removal_ongoing = non_tx_vdev_exception &&
        wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS);

    /**
     * Exclude this VDEV from further primary TQM selections.
     * An MBSS non-Tx VDEV can exercise this path twice if there was a non-Tx
     * VDEV removal followed by Tx VDEV removal. In such case, ensure to not
     * call wlan_vdev_inc_skip_pumac_cnt() twice as this API is not idempotent.
     */
    if (!is_non_tx_removal_ongoing) {
        wlan_vdev_inc_skip_pumac_cnt(vdev);
        skip_pumac_incremented = true;
    }

#ifdef QCA_SUPPORT_WDS_EXTENDED
    status = osif_mlo_bridge_peer_wds_ext_migrate(vap);
    if (QDF_IS_STATUS_ERROR(status)) {
         mlo_err("Unable to migrate bridge peer wds_ext for this link: %s",
                 vap->iv_netdev_name);
         return status;
    }
#endif
#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
    /**
     * Before proceeding to the link removal, initiate the migration of the
     * primary TQM of the ML peers that have the link to be removed as primary.
     * Note that this is not a blocking call, hence, primary TQM migrtion and
     * link removal can happen in parallel.
     */
    status = ieee80211_ucfg_mlreconfig_set_link_ptqm_layer_migrate(vap);
    if (QDF_IS_STATUS_ERROR(status)) {
        mlo_err("Unable to migrate the PTQM from this link: %s",
                vap->iv_netdev_name);
        return status;
    }
#endif /* QCA_SUPPORT_PRIMARY_LINK_MIGRATE */

    ml_reconfig_ie = ieee80211_ucfg_get_apremove_ml_reconfig_ie(vap, tbtt_count,
                                                       &ml_reconfig_ie_len);
    if (!ml_reconfig_ie) {
        mlo_err("Couldn't retrieve ML reconfiguration IE");
        status = QDF_STATUS_E_FAILURE;
        goto dec_skip_pumac_count;
    }

    params.vdev_id = wlan_vdev_get_id(vdev);
    params.reconfig_ml_ie = ml_reconfig_ie;
    params.reconfig_ml_ie_size = ml_reconfig_ie_len;

    ic = vap->iv_ic;
    if (!ic->ic_send_mlo_link_remove_cmd) {
        mlo_err("ic_send_mlo_link_remove_cmd is not set");
        status = QDF_STATUS_NOT_INITIALIZED;
        goto free_ml_ie;
    }

    status = ic->ic_send_mlo_link_remove_cmd(wlan_vdev_get_psoc(vdev), &params);
    if (QDF_IS_STATUS_ERROR(status)) {
        mlo_err("Couldn't send the MLO link removal command to the target");
        goto free_ml_ie;
    }

    status = wlan_mlo_rmsl_link_remove_update(vap);
    if (QDF_IS_STATUS_ERROR(status)) {
         mlo_err("Couldn't send Max Recommended links during link removal command to the target");
         goto free_ml_ie;
    }

    removal_ctx = &vap->link_reconfig_ctx.removal_ctx;

    /* non_tx_vdev_exception marks the removal as part of Tx-vap removal.
     * Under this condition do not expect the TBTT_START event again if
     * the link removal for the non-Tx vap is already ongoing. The remove
     * command is sent again to FW to update the tbtt_count in case the
     * tbtt_count for the TX-vap is less than that requested for the non-Tx
     * vap already.
     *
     * Similarly, do not update the tbtt_count.
     */
    if (!is_non_tx_removal_ongoing) {
        removal_ctx->tbtt_count = tbtt_count;
        removal_ctx->expected_event_type = MLO_LINK_REMOVAL_EVENT_TYPE_TBTT_START;
    }

    status = QDF_STATUS_SUCCESS;

    /* No errors are seen so far, set the vdev op flag */
    wlan_vdev_mlme_op_flags_set(vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS);

free_ml_ie:
    qdf_mem_free(ml_reconfig_ie);

dec_skip_pumac_count:
    if (QDF_IS_STATUS_ERROR(status) && skip_pumac_incremented)
        wlan_vdev_dec_skip_pumac_cnt(vdev);

    return status;
}

static QDF_STATUS mlo_link_add_validate_batch(wlan_if_t vap)
{
    uint16_t batch_size = 0;
    QDF_STATUS status;

    status = mlme_mlo_link_add_get_current_batch_size(vap, &batch_size);
    if (QDF_IS_STATUS_ERROR(status)) {
        mlo_err("Unable to retrieve current batch size");
        return status;
    }

    if (batch_size != 1) {
        mlo_err("Link addition req should only contain 1 link in a batch,"
                "but user provided %u links in this batch", batch_size);
        return QDF_STATUS_E_NOSUPPORT;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlo_validate_link_add_request(wlan_if_t vap)
{
    struct qdf_mac_addr *mld_addr;
    struct wlan_mlo_dev_context *mlo_dev_ctx;
    struct wlan_objmgr_vdev *vdev, *temp_vdev;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct ol_ath_softc_net80211 *scn = NULL;
    uint16_t vdev_count, vdev_idx = 0;
    wlan_if_t temp_vap;
    QDF_STATUS status;

    if (!vap) {
        mlo_err("vap is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    vdev = vap->vdev_obj;

    /* VAP should have an MLD address */
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
    if (qdf_is_macaddr_zero(mld_addr)) {
        mlo_err("MLD MAC address is null");
        status = QDF_STATUS_E_INVAL;
        goto fail;
    }

    /* validate if there exists an MLD with the configured MLD address */
    mlo_dev_ctx = wlan_mlo_get_mld_ctx_by_mldaddr(mld_addr);
    if (!mlo_dev_ctx) {
        mlo_err("MLD doesn't exist with the MAC address configured in vap");
        status = QDF_STATUS_E_INVAL;
        goto fail;
    }

    /* validate if it is an AP MLD */
    if (!mlo_dev_ctx->ap_ctx) {
        mlo_err("AP MLD doesn't exist with the MAC address configured in vap");
        status = QDF_STATUS_E_INVAL;
        goto fail;
    }

    /* VAP should not be active (either on this MLD, or other MLD, or even stand-alone */
    if (QDF_IS_STATUS_SUCCESS(wlan_vdev_chan_config_valid(vdev))) {
        mlo_err("vap is aleady active");
        status = QDF_STATUS_E_NOSUPPORT;
        goto fail;
    }

    /* Don't try to add the same VAP while it is already being added */
    if (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_ADD)) {
        mlo_err("Link add request is already in progress on the same vap");
        status = QDF_STATUS_E_BUSY;
        goto fail;
    }

    /* Don't try to add the VAP while it is already being removed */
    if (wlan_vdev_mlme_op_flags_get(
           vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS)) {
        mlo_err("Link removal request is in progress on the same vap");
        status = QDF_STATUS_E_NOSUPPORT;
        goto fail;
    }

    mlo_ap_get_active_vdev_list(vdev, &vdev_count, vdev_list);

    /* Possible memory corruption */
    if (vdev_count > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu,"
                "investigate the possible corruption",
                vdev_count, QDF_ARRAY_SIZE(vdev_list));
        status = QDF_STATUS_E_FAILURE;
        goto release_ref;
    }

    /* If the MLD is down (i.e. no active VDEVs), reject the request */
    if (!vdev_count) {
        mlo_err("Link addition is supported only on an MLD with atleast one active link");
        status = QDF_STATUS_E_NOSUPPORT;
        goto release_ref;
    }

    for (;vdev_idx < vdev_count; vdev_idx++) {
        temp_vdev = vdev_list[vdev_idx];
        temp_vap = wlan_vdev_mlme_get_ext_hdl(temp_vdev);

        if (!temp_vap) {
            mlo_err("partner vap corresponding to vdev:%pK is null", temp_vdev);
            status = QDF_STATUS_E_FAILURE;
            goto release_ref;
        }

        /**
         * If an active VDEV from the same PDEV is already part of this MLD,
         * reject the request.
         */
        if (wlan_vdev_get_pdev(vdev) == wlan_vdev_get_pdev(temp_vdev)) {
            mlo_err("This MLD already contains an active vdev %s from the same pdev (psoc ID %d, pdev ID %d)",
                     temp_vap->iv_netdev_name,
                     wlan_vdev_get_psoc_id(temp_vdev),
                     wlan_objmgr_pdev_get_pdev_id(wlan_vdev_get_pdev(temp_vdev)));
            status = QDF_STATUS_E_PERM;
            goto release_ref;
        }

        /* If there is an ongoing link addition on this MLD, reject the request */
        if (((scn->soc->standby_mode) != WLAN_STANDBY_MODE_INITIATE_EXIT) &&
            (wlan_vdev_mlme_op_flags_get(temp_vdev, WLAN_VDEV_OP_MLO_LINK_ADD))) {
            mlo_err("Link add request is already in progress on partner link %s",
                    temp_vap->iv_netdev_name);
            status = QDF_STATUS_E_NOSUPPORT;
            goto release_ref;
        }

        /* If there is an ongoing link removal on this MLD, reject the request */
	if (wlan_vdev_mlme_op_flags_get(
               temp_vdev, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS)) {
		mlo_err("Link removal request is in progress on partner link %s",
                        temp_vap->iv_netdev_name);
		status = QDF_STATUS_E_NOSUPPORT;
		goto release_ref;
	}
    }

    if ((scn->soc->standby_mode) != WLAN_STANDBY_MODE_INITIATE_EXIT) {
        status = mlo_link_add_validate_batch(vap);
        if (QDF_IS_STATUS_ERROR(status)) {
            mlo_err("Link add batch validation failed");
            goto release_ref;
        }
    }

    /* No errors seen so far */
    status = QDF_STATUS_SUCCESS;

release_ref:
    if (vdev_count)
        ieee80211_mlo_release_vdev_list_refs(vdev_list,
                                             QDF_ARRAY_SIZE(vdev_list),
                                             vdev_count, 0);
fail:
    if (QDF_IS_STATUS_ERROR(status))
        mlo_err("Link add request failed on vap %s", vap->iv_netdev_name);

    return status;
}
#endif

void wlan_find_vdev_by_bssid_pdev_cb(struct wlan_objmgr_psoc *psoc,
                void *msg,
                uint8_t index)
{
    int id = 0;
    wlan_dev_t ic;
    struct wlan_objmgr_psoc_objmgr *psoc_objmgr;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct vdev_bssid_obj *vdev_bssid = (struct vdev_bssid_obj *)msg;

    psoc_objmgr = &psoc->soc_objmgr;
    /* Get pdev from pdev list */
    for (id=0;id<WLAN_UMAC_MAX_PDEVS;id++) {
        pdev = psoc_objmgr->wlan_pdev_list[id];
        if (pdev) {
            ic = wlan_pdev_get_mlme_ext_obj(pdev);
            if (ic && IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
                vdev_bssid->vdev = wlan_objmgr_get_vdev_by_macaddr_from_pdev(pdev,
                                       vdev_bssid->bssid, WLAN_MLME_NB_ID);
                break;
            }
        }
    }
}

struct wlan_objmgr_vdev *wlan_find_vdev_from_psocs_by_macaddr(uint8_t* bssid)
{
    struct vdev_bssid_obj vdev_bssid;

    qdf_mem_copy(vdev_bssid.bssid, bssid, QDF_MAC_ADDR_SIZE);
    vdev_bssid.vdev = NULL;

    wlan_objmgr_iterate_psoc_list(wlan_find_vdev_by_bssid_pdev_cb, &vdev_bssid, WLAN_MLME_NB_ID);

    return vdev_bssid.vdev;
}
qdf_export_symbol(wlan_find_vdev_from_psocs_by_macaddr);

QDF_STATUS wlan_pdev_wait_to_bringdown_vdevs(struct ieee80211com *ic,
                                                 enum bring_updown_mode mode)
{
    qdf_bitmap(bringdown_pend_vdev_arr, WLAN_UMAC_PSOC_MAX_VDEVS);
    qdf_bitmap(vdev_gp_dst_bmap, WLAN_UMAC_PSOC_MAX_VDEVS);
    qdf_bitmap(tmp_vdev_gp_bmap, WLAN_UMAC_PSOC_MAX_VDEVS);
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    uint16_t waitcnt;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t vdev_cnt, group_id = IEEE80211_MBSSID_INVALID_GROUP_ID;
    QDF_STATUS status;

    if (ic->is_mbssid_enabled) {
        if (!ic->ic_mbss_ctx) {
            mbss_err("mbss context is null");
            return QDF_STATUS_E_INVAL;
        }
        mbss_ctx = ic->ic_mbss_ctx;

        /* map vdev_ids of vdevs in a group to temp bit map */
        if (mode == ALL_AP_VDEVS_PER_GP) {
            group_id = mbss_ctx->flags &
                IEEE80211_MBSSID_MBSS_CTX_FLAGS_BRINGDOWN_VDEVS_GROUP_ID_MASK;
            ic_mbss = &mbss_ctx->ic_mbss[group_id];
            qdf_mem_zero(tmp_vdev_gp_bmap, sizeof(tmp_vdev_gp_bmap));
            for (vdev_cnt = 0; vdev_cnt < mbss_ctx->grp_sz; vdev_cnt++) {
                if (ic_mbss->vdev_id_gp[vdev_cnt] != IEEE80211_MBSSID_INVALID_VDEV_ID) {
                    qdf_set_bit(ic_mbss->vdev_id_gp[vdev_cnt], tmp_vdev_gp_bmap);
                }
            }
        }
    }

    /* wait for vap stop event before letting the caller go */
    waitcnt = 0;
    while(waitcnt < OSIF_MAX_STOP_VAP_TIMEOUT_CNT) {
        /* Reset the bitmap */
        qdf_mem_zero(bringdown_pend_vdev_arr,
                     sizeof(bringdown_pend_vdev_arr));
        /* Reset per group bitmap */
        qdf_mem_zero(vdev_gp_dst_bmap, sizeof(vdev_gp_dst_bmap));

        if (mode == ALL_AP_VDEVS || mode == ALL_AP_VDEVS_PER_GP) {
              wlan_pdev_chan_change_pending_ap_vdevs_down(pdev,
                      bringdown_pend_vdev_arr,
                      WLAN_MLME_SB_ID);
        } else {
              wlan_pdev_chan_change_pending_vdevs_down(pdev,
                      bringdown_pend_vdev_arr,
                      WLAN_MLME_SB_ID);
        }

        /* If all the pending vdevs goes down, this would fail,
           otherwise start timer */
        if (mode != ALL_AP_VDEVS_PER_GP) {
            if (!wlan_util_map_is_any_index_set(bringdown_pend_vdev_arr,
                                                sizeof(bringdown_pend_vdev_arr)))
                return QDF_STATUS_SUCCESS;
        } else {
             if (group_id == IEEE80211_MBSSID_INVALID_GROUP_ID)
                 return QDF_STATUS_SUCCESS;

            /* Check if all the pending vdevs in group are down.
             * Check the bits corresponding to the vdevs belonging to
             * the target group only.
             * If all the bits corresponding to the vdevs in the target
             * group are reset then return success */
             qdf_bitmap_and(vdev_gp_dst_bmap, bringdown_pend_vdev_arr,
                    tmp_vdev_gp_bmap,
                    QDF_CHAR_BIT * sizeof(tmp_vdev_gp_bmap));

            if (!wlan_util_map_is_any_index_set(vdev_gp_dst_bmap,
                        sizeof(vdev_gp_dst_bmap))) {

                return QDF_STATUS_SUCCESS;
            }

        }

        if (ic->recovery_in_progress) {
            qdf_err("Target in power down/unstable state do not wait for vdev down response");
            break;
        }

        /*
         * Wait for the VDEV down event completion i.e. until the VDEV down event is set by
         * another function.
         */
        status = qdf_wait_for_event_completion(&ic->wait_event_for_down, OSIF_STOP_VAP_TIMEOUT);
        if (QDF_IS_STATUS_SUCCESS(status)) {
            /* Reset the QDF wait event for other VAPs in a multivap scenario */
            status = qdf_event_reset(&ic->wait_event_for_down);
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("Unable to reset VDEV down event");
                return QDF_STATUS_E_FAILURE;
            }
        }
        waitcnt++;
    }

    if (waitcnt >= OSIF_MAX_STOP_VAP_TIMEOUT_CNT) {
        qdf_err("VDEVs are not stopped, bitmap is as follows");
        if (mode != ALL_AP_VDEVS_PER_GP) {
            qdf_trace_hex_dump(
                    QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_ERROR,
                    bringdown_pend_vdev_arr, sizeof(bringdown_pend_vdev_arr));
        } else {
            qdf_trace_hex_dump(
                    QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_ERROR,
                    vdev_gp_dst_bmap, sizeof(vdev_gp_dst_bmap));

        }
    }

    if (ic->recovery_in_progress) {
        qdf_err("FW Crash observed ...returning");
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(wlan_pdev_wait_to_bringdown_vdevs);

int ieee80211_bringup_all_vaps(struct ieee80211com *ic,
                               enum bring_updown_mode mode)
{
    wlan_if_t tmpvap;
    struct net_device *tmpdev = NULL;
    int retval = EOK;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if ((mode == ALL_AP_VDEVS) &&
            (tmpvap->iv_opmode != IEEE80211_M_HOSTAP)) {
                continue;
        }

        tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
        if (tmpvap->iv_opmode == IEEE80211_M_STA) {
            retval = IS_UP(tmpdev) ?
                     wlan_mlme_cm_start(tmpvap->vdev_obj, CM_OSIF_CONNECT) : 0;
        } else {
            retval = (IS_UP(tmpdev) && (tmpvap->iv_novap_reset == 0)) ?
                       wlan_mlme_start_vdev(tmpvap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE) : 0;
        }

        if (retval) {
            qdf_err("Could not bring up vdev %d", tmpvap->iv_unit);
            break;
        }
    }

    return retval;
}

int ieee80211_bringdown_all_vaps(struct ieee80211com *ic,
                                 enum bring_updown_mode mode)
{
    struct ieee80211vap *tmpvap = NULL;
    int retval = EOK;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if ((mode == ALL_AP_VDEVS) &&
            (tmpvap->iv_opmode != IEEE80211_M_HOSTAP)) {
                continue;
        }

        if (tmpvap->iv_opmode == IEEE80211_M_STA) {
            retval = wlan_mlme_cm_stop(tmpvap->vdev_obj, CM_OSIF_CFG_DISCONNECT,
                                       REASON_STA_LEAVING, false);
        } else {
            retval = wlan_mlme_stop_vdev(tmpvap->vdev_obj, 0, 0);
        }

        if (retval) {
             qdf_err("Could not stop vdev %d", tmpvap->iv_unit);
             break;
        }
     }

    if (!retval && wlan_pdev_wait_to_bringdown_vdevs(ic, mode)) {
        qdf_err("Could not bring down VAPs");
        retval = -EINVAL;
    }

    if (!retval &&
        (((mode == ALL_AP_VDEVS) && ieee80211_get_num_ap_vaps_up(ic)) ||
         ((mode == ALL_VDEVS) && ieee80211_get_num_vaps_up(ic)))) {
        qdf_err("VAPs are still active");
        retval = -EINVAL;
    }

    return retval;
}

/* wlan_fetch_inter_soc_rnr_cache: Find a valid RNR cache by doing
 *  * lookup across all SoCs and return valid rnr cache
 *   *
 *    * @rnr: RNR cache associated with any SoC
 *     */
void wlan_fetch_inter_soc_rnr_cache(void *rnr, struct wlan_objmgr_psoc *psoc_6g, uint8_t pdev_id, bool is_mlo)
{
    uint8_t index = 0;
    struct wlan_6ghz_rnr_global_cache **psoc_rnr;
    struct psoc_mlme_obj *mlme_psoc_priv_obj;

    if (rnr) {
        psoc_rnr = (struct wlan_6ghz_rnr_global_cache **)rnr;

        while (index < WLAN_OBJMGR_MAX_DEVICES) {
            if (g_umac_glb_obj->psoc[index] &&
                (psoc_6g == g_umac_glb_obj->psoc[index])) {
                mlme_psoc_priv_obj = wlan_get_psoc_mlme_obj(g_umac_glb_obj->psoc[index]);
                if (mlme_psoc_priv_obj) {
                    if (is_mlo)
                        *psoc_rnr = &mlme_psoc_priv_obj->rnr_6ghz_cache[pdev_id];
                    else
                        *psoc_rnr = &mlme_psoc_priv_obj->rnr_6ghz_cache_legacy[pdev_id];

                    if ((*psoc_rnr && (*psoc_rnr)->rnr_cnt > 0)) {
                        break;
                    }
                }
            }
            index++;
        }
    }
}

struct wlan_6ghz_rnr_global_cache *ieee80211_get_6ghz_cache(struct wlan_objmgr_pdev *pdev, bool is_mlo)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(pdev);
    struct psoc_mlme_obj *mlme_psoc_priv_obj = wlan_get_psoc_mlme_obj(psoc);
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    if (is_mlo)
        return &mlme_psoc_priv_obj->rnr_6ghz_cache[pdev_id];

    return &mlme_psoc_priv_obj->rnr_6ghz_cache_legacy[pdev_id];
}

/**
 * wlan_get_psd_from_curchan_eirp: Compute PSD from the EIRP of the current channel
 * for the default client type
 *
 * @vap: Pointer to vap
 * @max_reg_eirp_psd_pwr: Pointer to max_reg_eirp_psd_pwr
 *
 * Return - QDF_STATUS
 */
static QDF_STATUS wlan_get_psd_from_curchan_eirp(struct ieee80211vap *vap,
                                           int16_t *max_reg_eirp_psd_pwr)
{
    enum reg_6g_ap_type ap_pwr_type;
    int8_t max_eirp_arr[IEEE80211_TPE_NUM_POWER_SUPPORTED];
    struct ieee80211com *ic = vap->iv_ic;
    QDF_STATUS status;

    status = ucfg_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &ap_pwr_type);
    if (QDF_IS_STATUS_ERROR(status))
        return QDF_STATUS_E_FAILURE;

    status = ieee80211_reg_get_eirp_arr_for_6g(vap, ap_pwr_type, true,
                                               REG_DEFAULT_CLIENT,
                                               max_eirp_arr,
                                               ic->ic_curchan, false);
    ieee80211_get_psd_from_eirp(vap, max_reg_eirp_psd_pwr, max_eirp_arr, false);

    return status;
}

void
ieee80211_downgrade_320mhz_chwidth_rnr_opclass(struct ieee80211com *ic,
                                               uint16_t *chan_width)
{
#ifdef WLAN_FEATURE_11BE
    if (ic->ic_downgrade_320mhz_opclass && *chan_width == BW_320_MHZ) {
        *chan_width = BW_160_MHZ;
    }
#endif
}

/**
 * wlan_update_6ghz_rnr_cache: Add 6Ghz AP info to RNR IE cache
 *
 * @vap: 6Ghz Vap to be added to cache
 * @is_vap_add: Indicate if vap is being added or change in RNR IE field
 *              1 - new vap added
 *              0 - Existing vap info change
 */
int wlan_update_6ghz_rnr_cache(struct ieee80211vap *vap,
                               uint8_t is_vap_add)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_6ghz_rnr_global_cache *rnr;
    ieee80211_rnr_nbr_ap_info_t *ap_info = NULL;
    ieee80211_rnr_tbtt_info_set_t *p_tbtt_info;
    ieee80211_mlo_rnr_tbtt_info_set_t *p_tbtt_info_mlo = NULL;
    uint8_t *p_rnr_buf;
    uint8_t *p_org_rnr_buf;
    int count;
    uint16_t chwidth, behav_lim;
    bool global_lookup = true;
    uint8_t op_class, prim_chan;
    bool is_psd_pwr;
    uint16_t max_reg_psd_pwr;
    int16_t max_reg_eirp_psd_pwr;
    uint16_t hdr_info_len;
    /* Use mlo-rnr cache in case of 11be mode */
    bool is_mlo;
    uint8_t size_of_tbbt_info_field;
    int idx;

    if ((ieee80211vap_get_opmode(vap) != IEEE80211_M_HOSTAP) ||
            IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        qdf_err("Skip rnr update for vap-%u(%s)",
                vap->iv_unit, vap->iv_netdev_name);
        return -1;
    }

    if (!ic) {
        qdf_debug("IC is NULL");
        return -1;
    }

    if (!ic->ic_pdev_obj) {
        qdf_debug("Pdev obj is NULL");
        return -1;
    }

    /* Remove if already present before adding updated entry */
    if (is_vap_add)
        wlan_remove_vap_from_6ghz_rnr_cache(vap);

    /*
     * Add both rnr cache to support legacy and 11BE TBTT information.
     * Non 11BE: Updated RNR entries for legacy cache with bytes TBTT info.
     * 11BE: Updated RNR entries for 11BE MLO cache with 16 bytes TBTT info.
     */
    for (idx = RNR_6GHZ_CACHE_INDEX_0; idx < RNR_6GHZ_CACHE_INDEX_MAX; idx++) {
    count = 1;
    /* Idx RNR_6GHZ_CACHE_INDEX_0 for legacy RNR cache,
     * RNR_6GHZ_CACHE_INDEX_MLO is for 1BE RNR cache.
     * is_mlo true for 11BE RNR cache update.
     */
    is_mlo = (idx == RNR_6GHZ_CACHE_INDEX_MLO);
    /* If 6G radio configured in 11BE add both rnr cache to support legacy
     * and 11BE TBTT information.
     * If it is configured in non 11BE, then add RNR entry only to legacy cache.
     * Before updating RNR entry to mlo 11BE cache,
     * check 6G radio configured in 11BE or not.
     */
    if (is_mlo && !ieee80211_is_phymode_11be(ieee80211_get_current_phymode(ic))) {
        qdf_debug("6G configured mode:%d non 11BE Mode",
                ieee80211_get_current_phymode(ic));
        continue;
    }
    size_of_tbbt_info_field = is_mlo ? MLO_TBTT_INFO_FIELD_SIZE:
                              TBTT_INFO_FIELD_SIZE;
    rnr = ieee80211_get_6ghz_cache(ic->ic_pdev_obj, is_mlo);
    hdr_info_len = sizeof(ieee80211_rnr_tbtt_info_set_t);

    p_rnr_buf = rnr->rnr_buf;
    p_org_rnr_buf = p_rnr_buf;

    /* Skip by Element Id and Element len */
    p_rnr_buf += 2;
    ap_info = (ieee80211_rnr_nbr_ap_info_t *) p_rnr_buf;

    wlan_get_bw_and_behav_limit(ic->ic_curchan,
                        &chwidth, &behav_lim);
    /* 11BE Draft4.0 requirement:
     * Reduce 320MHz channel width to downgraded opclass  in RNR tbtt info.
     */
    ieee80211_downgrade_320mhz_chwidth_rnr_opclass(ic, &chwidth);
    wlan_reg_freq_width_to_chan_op_class_auto(ic->ic_pdev_obj,
                        ic->ic_curchan->ic_freq, chwidth,
                        global_lookup, behav_lim,
                        &op_class, &prim_chan);

    p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *)
                    ((uint8_t *)ap_info + RNR_NBR_AP_INFO_HDR_SIZE);
    ap_info->op_class = op_class;
    ap_info->channel = prim_chan;

    if (!is_vap_add) {
       while (count <= rnr->rnr_cnt) {
           if (!vap->iv_he_6g_bcast_prob_rsp) {
               p_tbtt_info->bss_params.probe_resp_20tu_active = 0;
           } else {
               p_tbtt_info->bss_params.probe_resp_20tu_active =
                   ic->ic_6ghz_rnr_unsolicited_prb_resp_active;
           }
           count++;
           p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *)
                           ((uint8_t *)p_tbtt_info + size_of_tbbt_info_field);
       }
       continue;
    }

    if (rnr->rnr_cnt < RNR_MAX_VAP_ADVERTISED) {
        p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *)
                        ((uint8_t *)p_tbtt_info +
                         (rnr->rnr_cnt * size_of_tbbt_info_field));
    } else {
        qdf_debug("%s:%d: RNR Cache full", __func__, __LINE__);
        continue;
    }

    p_tbtt_info->tbtt_offset = RNR_TBTT_OFFSET_UNKNOWN;
    IEEE80211_ADDR_COPY(p_tbtt_info->bssid, vap->iv_myaddr);
    p_tbtt_info->short_ssid = htole32(ieee80211_construct_shortssid
                              (vap->iv_bss->ni_essid, vap->iv_bss->ni_esslen));
    p_tbtt_info->bss_params.oct_recommended = 0;
    p_tbtt_info->bss_params.same_ssid = 0;
    p_tbtt_info->bss_params.mbssid_set = 0;
    if (ic && ic->is_mbssid_enabled) {
        p_tbtt_info->bss_params.mbssid_set = 1;
    }
    p_tbtt_info->bss_params.tx_bssid =
                 ((IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) ? 0 : 1);
    p_tbtt_info->bss_params.colocated_lower_band_ess = 0;
    p_tbtt_info->bss_params.co_located_ap = 1;
    p_tbtt_info->bss_params.probe_resp_20tu_active = ic->ic_6ghz_rnr_unsolicited_prb_resp_active;

    wlan_reg_get_client_power_for_6ghz_ap(ic->ic_pdev_obj,
                                          REG_DEFAULT_CLIENT,
                                          ic->ic_curchan->ic_freq,
                                          &is_psd_pwr, &max_reg_psd_pwr,
                                          &max_reg_eirp_psd_pwr);
    if (!is_psd_pwr) {
        if (QDF_IS_STATUS_ERROR(wlan_get_psd_from_curchan_eirp(vap, &max_reg_eirp_psd_pwr))) {
            qdf_err("PSD not available");
            continue;
        }
    }

    p_tbtt_info->psd_20mhz = max_reg_eirp_psd_pwr * 2;

    if (is_mlo) {
        p_tbtt_info_mlo = (ieee80211_mlo_rnr_tbtt_info_set_t *) p_tbtt_info;

        /* Fill MLO params */

        /* Initialize MLD-id w/ unknown MLD-id */
        p_tbtt_info_mlo->mld_params.mld_id = IEEE80211_MLD_ID_UNKNOWN;
        /* Populate link-id */
        IEEE80211_SET_BITS(p_tbtt_info_mlo->mld_params.mld_partial_params, RNR_MLD_PARAMS_LINK_ID, (wlan_vdev_get_link_id(vap->vdev_obj) & 0xf));
        /* Initialize to 0. To be filled in FW */
        IEEE80211_SET_BITS(p_tbtt_info_mlo->mld_params.mld_partial_params, RNR_MLD_PARAMS_BSS_PARAM_CHG_CNT, 0);
    }

    if (rnr->rnr_cnt == 0) {
        ap_info->op_class = op_class;
        ap_info->channel = prim_chan;
        ap_info->hdr_field_type = 0;
        ap_info->hdr_filtered_nbr_ap = 0;
        ap_info->hdr_info_len = size_of_tbbt_info_field;
    }

    rnr->rnr_cnt++;
    ap_info->hdr_info_cnt = rnr->rnr_cnt - 1;
    *p_org_rnr_buf = IEEE80211_ELEMID_REDUCED_NBR_RPT;

    /* RNR size has length of neighbor AP field and NOT element id
     * + length field itself */
    rnr->rnr_size  = rnr->rnr_cnt * size_of_tbbt_info_field +
                           RNR_NBR_AP_INFO_HDR_SIZE + IE_LEN_ID_LEN;

    if (rnr->rnr_size > (IEEE80211_MAX_IE_LEN + IE_LEN_ID_LEN)) {
        uint8_t rnr_count_1st_elmt;

        rnr_count_1st_elmt = ((IEEE80211_MAX_IE_LEN -
                               RNR_NBR_AP_INFO_HDR_SIZE) / size_of_tbbt_info_field);
        *(p_org_rnr_buf+1) = rnr_count_1st_elmt * size_of_tbbt_info_field;
        qdf_rl_debug("rnr_count_1st_elmt: %d size: %d", rnr_count_1st_elmt, *(p_org_rnr_buf+1));
    } else {
        *(p_org_rnr_buf+1) = rnr->rnr_size - IE_LEN_ID_LEN;
    }

    qdf_rl_debug("rnr_size: %u", rnr->rnr_size);
    } /* End of for loop */

    return 0;
}

/**
 * wlan_remove_vap_from_6ghz_rnr_cache: Remove 6Ghz AP info from RNR IE cache
 *
 * @vap: 6Ghz Vap to be removed from cache
 */
int wlan_remove_vap_from_6ghz_rnr_cache(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_6ghz_rnr_global_cache *rnr;
    ieee80211_rnr_nbr_ap_info_t *ap_info = NULL;
    ieee80211_rnr_tbtt_info_set_t *p_tbtt_info = NULL;
    uint8_t *p_rnr_buf;
    uint8_t *p_org_rnr_buf;
    int count;
    uint16_t hdr_info_len;
    /* Use mlo-rnr cache in case of 11be mode */
    bool is_mlo;
    uint8_t size_of_tbbt_info_field;
    int idx;

    if (!ic->ic_pdev_obj) {
        qdf_err("Pdev obj is NULL");
        return -1;
    }

    if (IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        return -1;
    }
    /* Remove entries from both rnr cache. */
    for (idx = RNR_6GHZ_CACHE_INDEX_0; idx < RNR_6GHZ_CACHE_INDEX_MAX; idx++) {
        count = 1;
        is_mlo = (idx == RNR_6GHZ_CACHE_INDEX_MLO);
        size_of_tbbt_info_field = is_mlo ? MLO_TBTT_INFO_FIELD_SIZE:TBTT_INFO_FIELD_SIZE;

        rnr = ieee80211_get_6ghz_cache(ic->ic_pdev_obj, is_mlo);
        if (rnr->rnr_cnt == 0) {
            qdf_debug("%s:%d: No Vaps in RNR to delete", __func__, __LINE__);
            continue;
        }
        hdr_info_len = sizeof (ieee80211_rnr_tbtt_info_set_t);

        p_rnr_buf = rnr->rnr_buf;
        p_org_rnr_buf = p_rnr_buf;

        /* Skip by Element Id and Element len */
        p_rnr_buf += 2;

        ap_info = (ieee80211_rnr_nbr_ap_info_t *)p_rnr_buf;
        p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *)
            ((uint8_t *)ap_info + RNR_NBR_AP_INFO_HDR_SIZE);

        /* TBTT size obtained based on is_mlo check should be same as TBTT size
         * added in RNR cache. Below check re-assures the TBTT size trying to
         * remove is same as added. This would not be same when 6G mode changed
         * dynamically from 11bea to 11axa or vice versa.
         */
        if (ap_info->hdr_info_len != 0 && (ap_info->hdr_info_len !=
                    size_of_tbbt_info_field))
            size_of_tbbt_info_field = ap_info->hdr_info_len;

        while (count <= rnr->rnr_cnt)
        {
            if (!IEEE80211_ADDR_EQ(vap->iv_myaddr, p_tbtt_info->bssid)) {
                p_tbtt_info = (ieee80211_rnr_tbtt_info_set_t *)
                    ((uint8_t *)p_tbtt_info + size_of_tbbt_info_field);
                count++;
            } else {
                OS_MEMZERO(p_tbtt_info, size_of_tbbt_info_field);
                OS_MEMCPY(p_tbtt_info,
                        ((uint8_t *)p_tbtt_info + size_of_tbbt_info_field),
                        (rnr->rnr_cnt - count) *
                        size_of_tbbt_info_field);
                rnr->rnr_cnt--;
                rnr->rnr_size -= size_of_tbbt_info_field;
                ap_info->hdr_info_cnt = rnr->rnr_cnt - 1;
                break;
            }
        }

        /* Update overall IE size without element id and length fields */
        *(p_org_rnr_buf + 1) -= size_of_tbbt_info_field;
    } /* End of for loop */
    return 0;
}

uint8_t num_chain_from_chain_mask(uint32_t mask)
{
    uint8_t num_rf_chain = 0;
    mask &= 0xFF;

    while (mask) {
        if (mask & 0x1)
            num_rf_chain++;

        mask >>= 1;
    }

    return num_rf_chain;
}
qdf_export_symbol(num_chain_from_chain_mask);

void ieee80211_reset_user_rnr_list(struct ieee80211com *ic)
{
    struct user_rnr_data *user_rnr_uid;

    TAILQ_FOREACH(user_rnr_uid, &(ic->ic_user_neighbor_ap.user_rnr_data_list), user_rnr_next_uid) {
        user_rnr_uid->is_copied = false;
        user_rnr_uid->uid_ap_remaining = 0;
        user_rnr_uid->uid_ap_copied_cnt = 0;
    }
}

/*
 * ieee80211_get_first_ap_vap_iter_func:
 * Iteration function for finding the first AP VAP (capable of beaconing)
 * and/or the first AP VAP (capable of beaconing) which is UP as per the
 * netdevice flag
 *
 * @pdev  : Pointer to the PDEV structure
 * @object: Opaque handle to the current VAP
 * @arg   : Opaque handle to VAP list to populate
 *
 */
static void ieee80211_get_first_ap_vap_iter_func(struct wlan_objmgr_pdev *pdev,
                                                 void *object,
                                                 void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    struct ieee80211vap *vap = NULL;
    struct mbss_first_apvap_iter_func_args *func_args =
        (struct mbss_first_apvap_iter_func_args *) arg;

    struct ieee80211vap **vap_list = (struct ieee80211vap **)func_args->vap_list;
    struct net_device *dev = NULL;
    uint8_t grp_id = func_args ->group_id;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap || !ieee80211_mbss_is_beaconing_ap(vap) || (grp_id != vap->iv_mbss.group_id)) {
        return;
    }

    if (!vap_list[VAPLIST_FIRST_VAP]) {
        vap_list[VAPLIST_FIRST_VAP] = vap;
    }

    if (vap_list[VAPLIST_FIRST_UP_VAP]) {
        return;
    }

    dev = ((osif_dev *)vap->iv_ifp)->netdev;
    if (!dev) {
        return;
    }

    if (IS_UP(dev)) {
        vap_list[VAPLIST_FIRST_UP_VAP] = vap;
    }

    return;
}

/*
 * ieee80211_mbss_get_first_ap_vap:
 * Find the first AP VAP capable of beaconing in the list or the first AP
 * VAP which is marked as UP in the netdevice structure.
 *
 * @ic: Pointer to the ic structure
 * @first_first_up: Flag to find the first UP VAP.
 *
 * Return:
 * NULL: Could not find any VAP in the list
 * Else: Found either first UP VAP or the first VAP in the list
 */
struct ieee80211vap *ieee80211_mbss_get_first_ap_vap(struct ieee80211com *ic,
                                         bool find_first_up, uint8_t grp_id)
{
    struct ieee80211vap *vap_list[VAPLIST_MAX_SIZE] = {0};
    struct mbss_first_apvap_iter_func_args first_apvap_args = {0};

    first_apvap_args.group_id = grp_id;
    first_apvap_args.vap_list = vap_list;

    wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                                      ieee80211_get_first_ap_vap_iter_func,
                                      &first_apvap_args, 0, WLAN_MLME_SB_ID);

    if (find_first_up && vap_list[VAPLIST_FIRST_UP_VAP]) {
        return vap_list[VAPLIST_FIRST_UP_VAP];
    } else if (vap_list[VAPLIST_FIRST_VAP]) {
        return vap_list[VAPLIST_FIRST_VAP];
    }

    return NULL;
}

/*
 * ieee80211_mbss_update_iter_func:
 * Iterate function per-VAP for update VAP parameters for EMA mode during
 * mode switching operations.
 *
 * @pdev  : Pointer to the PDEV structure
 * @object: Opaque handle to the VAP
 * @arg   : Opaque handle to the return value
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successfully updated VAP parameters
 * QDF_STATUS_E_INVAL: Could not setup MBSSID entries
 */
static void ieee80211_mbss_update_iter_func(struct wlan_objmgr_pdev *pdev,
                                            void *object,
                                            void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    struct ieee80211vap *vap = NULL;
    struct mbss_iter_func_args *mbss_args = (struct mbss_iter_func_args *)arg;
    struct mbss_iter_func_mode_switch_ema_args *mode_switch_ema_args = NULL;
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_ath_ema_ctx_t *ema_ctx;
    uint32_t aid_bitmap_size;
    uint32_t nontx_key;
    int *grp_id = NULL;
    uint16_t start_aid = 0;

    if (!pdev) {
        /* Invalid pdev */
        return;
    }

    if (!vdev) {
        /* Invalid vdev */
        return;
    }

    if (!mbss_args) {
        /* Invalid arguments */
        return;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap || (vap->iv_opmode != IEEE80211_M_HOSTAP) ||
        IEEE80211_IS_SPECIAL_VAP(vap)) {
        return;
    }

    if (mbss_args->retval) {
        /* Return value already marked as error */
        return;
    }

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    ema_ctx = &scn->soc->ema_ctx;

    switch (mbss_args->purpose) {
        case MBSS_ITER_FUNC_PURPOSE_CACHE_INVALIDATION:
            grp_id = (int *)mbss_args->args;
            if ( *grp_id  != vap->iv_mbss.group_id)
                return;
            mbss_args->retval = ieee80211_mbssid_update_mbssie_cache(vap, false);
            if (mbss_args->retval) {
                mbss_err("Failed to clear MBSSIE cache for VDEV%d", vap->iv_unit);
                return;
            }
        break;

        case MBSS_ITER_FUNC_PURPOSE_MODE_SWITCH_EMA:
            if (!vap->iv_ic->ic_mbss_ctx) {
                mbss_err("ic_mbss_ctx is null");
                return;
            }

            if (!mbss_args->args) {
                mbss_err("Invalid args for EMA mode switch");
                return;
            }
            mode_switch_ema_args =
                  (struct mbss_iter_func_mode_switch_ema_args *)mbss_args->args;

            if (mode_switch_ema_args->tx_vap->iv_mbss.group_id  != vap->iv_mbss.group_id)
                return;
            /*
             * MBSSID cache entry update can happen in 2 ways:
             * (1) If the cache is empty, then MBSSID setup should be called to
             *     create new cache entries.
             * (2) If cache entry is not empty, then this iteration is called as
             *     part of the rollback operation. In that case, update the
             *     cache entry for the VAPs without sanitization since ref-BSSID
             *     is not changed and BSSID indices are the same.
             */

            if (mode_switch_ema_args->is_cache_empty) {
                IEEE80211_VAP_MBSS_NON_TRANS_DISABLE(vap);
                if (ieee80211_mbssid_setup(vap)) {
                    mbss_err("Could not setup MBSSID entry for VDEV%d",
                            vap->iv_unit);
                    mbss_args->retval = QDF_STATUS_E_INVAL;
                    return;
                }
            } else {
                /* roll back */
                if (ieee80211_mbssid_get_num_vaps_in_mbss_cache(vap->iv_ic, vap->iv_mbss.group_id) <
                        ieee80211_get_num_beacon_ap_vaps(vap->iv_ic)) {
                    if (vap == mode_switch_ema_args->tx_vap) {
                        IEEE80211_VAP_MBSS_NON_TRANS_DISABLE(vap);
                    } else {
                        IEEE80211_VAP_MBSS_NON_TRANS_ENABLE(vap);
                    }

                    if (ieee80211_mbssid_update_mbssie_cache(vap, true)) {
                        mbss_err("Could not update MBSSIE cache - not recoverable");
                        mbss_args->retval = QDF_STATUS_E_INVAL;
                        return;
                    }
                }
            }
            /*
             * Update the IE pool for non-TX profiles
             */
            if (IS_MBSSID_EMA_EXT_ENABLED(vap->iv_ic)) {
                if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
                    vap->iv_mbss.non_tx_pfl_ie_pool = qdf_nbuf_alloc(NULL,
                                                 IEEE80211_NTX_PFL_IE_POOL_SIZE,
                                                 0, 0, false);

                    if (!vap->iv_mbss.non_tx_pfl_ie_pool) {
                        mbss_args->retval = QDF_STATUS_E_NOMEM;
                        return;
                    }

                    qdf_spinlock_create(&vap->iv_mbss.non_tx_pfl_ie_pool_lock);
                    qdf_mem_zero(vap->iv_mbss.non_tx_pfl_ie_pool,
                                 sizeof(vap->iv_mbss.non_tx_pfl_ie_pool));
                    vap->iv_mbss.ntx_pfl_rollback_stats = 0;
                    vap->iv_mbss.backup_length = 0;
                }
                vap->iv_mbss.ie_overflow = false;
                vap->iv_mbss.ie_overflow_stats = 0;
            }

            /*
             * Update the AID parameters:
             * (1) Update max AID value
             * (2) Update max MBSS AID value (per-VAP limit in MBSS group)
             * (3) Free the AID bitmaps for co-hosted VAPs (txvap_set() will
             *     refactor AID pool)
             */
            start_aid = ieee80211_get_vap_start_aid(vap->iv_ic, vap);
            vap->iv_max_aid = vap->iv_mbss_max_aid =
                 (vap->iv_ic->ic_num_clients +
                 start_aid + 1);
#ifdef WLAN_FEATURE_11BE_MLO
            if (!(wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) ||
                vap->vdev_obj->mlo_dev_ctx)) {
#endif
               if (vap->iv_aid_bitmap) {
                   qdf_mem_free(vap->iv_aid_bitmap);
                   vap->iv_aid_bitmap = NULL;
               }
#ifdef WLAN_FEATURE_11BE_MLO
            }
#endif
            qdf_debug("aid:%d, iv_max_aid:%d, vap:%s",start_aid, vap->iv_max_aid, vap->iv_netdev_name);

            /*
             * Update the DTIM period for both non-Tx VAPs and the Tx VAP
             */
            if (vap != mode_switch_ema_args->tx_vap) {
                vap->vdev_mlme->proto.generic.dtim_period =
                              (vap->iv_create_flags & IEEE80211_LP_IOT_VAP) ?
                                    roundup(IEEE80211_DTIM_DEFAULT_LP_IOT,
                                            ema_ctx->max_pp) :
                                    roundup(IEEE80211_DTIM_DEFAULT,
                                            ema_ctx->max_pp);
            } else {
               vap->vdev_mlme->proto.generic.dtim_period =
                             (vap->iv_create_flags & IEEE80211_LP_IOT_VAP) ?
                                            IEEE80211_DTIM_DEFAULT_LP_IOT :
                                            IEEE80211_DTIM_DEFAULT;
            }

            /*
             * Check the key management flags of the non-Tx VAP and compare it
             * with that of the selected Tx VAP. If non-TX VAP is open and the
             * Tx VAP is not open, then non-inheritance should be enabled for
             * the MBSS group.
             * This is to happen only if the cache entry was empty. If
             * not empty, non-inheritance was not updated in the first place so
             * there is no requirement for a check.
             */
            nontx_key = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_KEY_MGMT);
            if (mode_switch_ema_args->is_cache_empty &&
                (nontx_key & (1 << WLAN_CRYPTO_KEY_MGMT_NONE)) &&
                !(mode_switch_ema_args->tx_key &
                                           (1 << WLAN_CRYPTO_KEY_MGMT_NONE)) &&
                !vap->iv_ic->ic_mbss_ctx->non_inherit_enable) {
                vap->iv_ic->ic_mbss_ctx->non_inherit_enable = true;
            }
        break;

        case MBSS_ITER_FUNC_PURPOSE_MODE_SWITCH_COHOSTED:
            grp_id = (int *)mbss_args->args;
            if (*grp_id != vap->iv_mbss.group_id)
                return;

            /* Clear all IE pool data structures from VAPs */
            if (IS_MBSSID_EMA_EXT_ENABLED(vap->iv_ic) &&
                IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) &&
                (vap->iv_mbss.non_tx_pfl_ie_pool != NULL)) {
                IEEE80211_NTX_PFL_IE_POOL_LOCK(vap);
                qdf_nbuf_free(vap->iv_mbss.non_tx_pfl_ie_pool);
                vap->iv_mbss.non_tx_pfl_ie_pool = NULL;
                IEEE80211_NTX_PFL_IE_POOL_UNLOCK(vap);
                qdf_spinlock_destroy(&vap->iv_mbss.non_tx_pfl_ie_pool_lock);
                vap->iv_mbss.ie_overflow = false;
                vap->iv_mbss.ie_overflow_stats = 0;
            }

            /*
             * Reset VDEV MBSSID flags. These flags should be disabled for
             * co-hosted mode operation.
             */
            IEEE80211_VAP_MBSS_NON_TRANS_DISABLE(vap);

            /* Update DTIM period as per co-hosted mode requirement */
            vap->vdev_mlme->proto.generic.dtim_period =
                          (vap->iv_create_flags & IEEE80211_LP_IOT_VAP) ?
                                         IEEE80211_DTIM_DEFAULT_LP_IOT :
                                         IEEE80211_DTIM_DEFAULT;

            /* Reset max AID limit */
            vap->iv_max_aid = vap->iv_mbss_max_aid = (vap->iv_ic->ic_num_clients + 1);
#ifdef WLAN_FEATURE_11BE_MLO
            if (!(wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) ||
                  vap->vdev_obj->mlo_dev_ctx)) {
#endif
                aid_bitmap_size = (howmany(vap->iv_max_aid,
                                           (sizeof(unsigned long) *
                                            BITS_PER_BYTE)) *
                                   sizeof(unsigned long));

                /* Allocate a new AID bitmap for each VAP in co-hosted mode */
                vap->iv_aid_bitmap = qdf_mem_malloc(aid_bitmap_size);
                if (!vap->iv_aid_bitmap) {
                    mbss_err("Could not allocate memory for AID bitmap for "
                            "VDEV%d - Rolling back", vap->iv_unit);
                    mbss_args->retval = QDF_STATUS_E_NOMEM;
                    return;
                }
#ifdef WLAN_FEATURE_11BE_MLO
            }
#endif
        break;

        default:
            mbss_err("Invalid purpose (%d)", mbss_args->purpose);
    }

    return;
}

/*
 * ieee80211_mbss_update_flags:
 * Update the EMA/MBSSID flags for 6GHz PDEVs.
 *
 * @ic: Pointer to the ic structure
 * @target_mode: Value of the desired target mode
 *
 */
static inline void ieee80211_mbss_update_flags(struct ieee80211com *ic,
                                               mbss_mode_t target_mode)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    scn->ema_ctx.disable_6ghz_mbssid = !!target_mode;
    if (target_mode == MBSS_MODE_COHOSTED) {
        wlan_pdev_nif_feat_cap_clear(ic->ic_pdev_obj,
                                   WLAN_PDEV_F_MBSS_IE_ENABLE);
        wlan_pdev_nif_feat_ext_cap_clear(ic->ic_pdev_obj,
                                   WLAN_PDEV_FEXT_EMA_AP_ENABLE);
        ic->is_mbssid_enabled = false;
    } else {
        wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
                                   WLAN_PDEV_F_MBSS_IE_ENABLE);
        wlan_pdev_nif_feat_ext_cap_set(ic->ic_pdev_obj,
                                       WLAN_PDEV_FEXT_EMA_AP_ENABLE);
        ic->is_mbssid_enabled = true;
    }
}

/*
 * ieee80211_mbss_switch_mode_to_ema:
 * Switch the MBSS mode to legacy EMA mode.
 *
 * @ic: Pointer to the ic structure
 * @target_txvap: Pointer to the current Tx-VAP
 *
 * Return:
 * MBSS_SWITCH_SUCCESS  : Successful switch to co-hosted mode
 * MBSS_SWITCH_E_RECOVER: Failed to switch but recovered to original mode
 * MBSS_SWITCH_GOTO_ROLLBACK: Notify caller for rollback operation (if applicable)
 */
static inline
mbss_switch_status_t ieee80211_mbss_switch_mode_to_ema(struct ieee80211com *ic,
                                              struct ieee80211vap *target_txvap, int grp_id)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_ath_soc_softc_t *soc = NULL;
    struct mbss_iter_func_args iter_func_args = {0};
    struct mbss_iter_func_mode_switch_ema_args mode_switch_ema_args = {0};
    struct mbssid_context *mbss_ctx = NULL;

    if (!ic) {
        qdf_err("ic is invalid, cannot recover");
        return MBSS_SWITCH_E_RECOVER;
    }

    if(!ic->ic_mbss_ctx) {
        qdf_err("ic_mbss_ctx is null");
        return MBSS_SWITCH_E_RECOVER;
    }
    mbss_ctx = ic->ic_mbss_ctx;

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc = scn->soc;

    /*
     * If the given target_txvap is NULL, if the caller could not find a Tx
     * VAP or if it is a rollback operation for a switch called when there is
     * no Tx VAP (if Tx VAP is down or deleted), the find for the first up Tx
     * VAP again. If a VAP is still not found, return with a rollback indication
     */
    if (!target_txvap) {
        /* Find first VAP for a given group */
        target_txvap = ieee80211_mbss_get_first_ap_vap(ic, true, grp_id);
        if (!target_txvap) {
            qdf_err("Could not find a Tx VAP");
            return MBSS_SWITCH_GOTO_ROLLBACK;
        }
    }

    mbss_info("Groupid: %d TxvapID: %d", grp_id, target_txvap->iv_unit);
    /*
     * The following arguments are required by the iteration function:
     * (1) purpose       : Type of iteration routine for MBSS operation
     * (2) retval        : Return value to indicate success/failure/rollback.
     * (3) tx_vap        : VDEV ID of the selected Tx VAP for updating the DTIM period.
     * (4) tx_key        : Key management flags for the selected Tx VAP.
     * (5) is_cache_empty: Flag to allow iteration API to decide if cache entries
     *                     should be setup or added without sanitization.
     * (6) is_first_vap  : Flag to detect the first VAP in the iteration for
     *                     reassigning the reference BSSID to the first VAP setup.
     */
    iter_func_args.purpose       = MBSS_ITER_FUNC_PURPOSE_MODE_SWITCH_EMA;
    iter_func_args.retval        = QDF_STATUS_SUCCESS;
    mode_switch_ema_args.tx_vap  = target_txvap;
    mode_switch_ema_args.tx_key  = wlan_crypto_get_param(target_txvap->vdev_obj,
                                                    WLAN_CRYPTO_PARAM_KEY_MGMT);
    mode_switch_ema_args.is_cache_empty =
                        !ieee80211_mbssid_get_num_vaps_in_mbss_cache(ic, grp_id);
    mode_switch_ema_args.is_first_vap = true;
    iter_func_args.args          = (void *)&mode_switch_ema_args;

    /*
     * Setup/Rollback the MBSSID cache entries, update AID bitmap, update DTIM
     * period and check for non-inheritance for each VAP.
     */
    wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                                 ieee80211_mbss_update_iter_func,
                                 &iter_func_args, 0, WLAN_MLME_SB_ID);
    if (iter_func_args.retval) {
        if (iter_func_args.retval == QDF_STATUS_E_NOMEM) {
            return MBSS_SWITCH_E_NORECOVER_NOMEM;
        } else {
            return MBSS_SWITCH_GOTO_ROLLBACK;
        }
    }

    /*
     * Set Tx VAP:
     * Setting Tx VAP takes care of allocating AID pool for Tx VAP and
     * arrange for non-Tx vaps to share the same. It also takes care of
     * BSSID index re-assignment of each non-Tx VAP as per the new Tx VAP.
     */
    if (!mbss_ctx->ic_mbss[target_txvap->iv_mbss.group_id].transmit_vap &&
            ieee80211_ucfg_set_txvap(target_txvap)) {
        qdf_err("Could not set Tx VAP");
        return MBSS_SWITCH_GOTO_ROLLBACK;
    }

    return MBSS_SWITCH_SUCCESS;
}

/*
 * wlan_mbss_group_iterate_vdev_list
 * iterator API to execute specific funtion for all VAPs
 * in the mbss group
 *
 * @ic: Pointer to the ic structure
 * @grp_id: group id of the MBSS group
 * @arg: argument to function
 *
 * Return: None
 */
void
wlan_mbss_group_iterate_vdev_list(struct ieee80211com *ic,
                                  uint8_t grp_id, void *arg)
{
    bool restart_vap = true;
    osif_dev  *osifp;
    struct net_device *dev;
    int vap_cnt, retv, curr_dtim_period = 0;
    struct ieee80211vap *tmp_vap;
    struct wlan_objmgr_vdev *tmp_vdev;
    struct mbssid_info *ic_mbss = NULL;
    struct wlan_vdev_mgr_cfg mlme_cfg;
    struct mbss_iter_func_args *mbss_args = (struct mbss_iter_func_args *)arg;
    uint8_t num_vaps = 0;

    if (!ic->ic_mbss_ctx) {
        mbss_err("ic_mbss_ctx NULL");
        return;
    }

    if (!mbss_args || mbss_args->retval) {
        return;
    }

    ic_mbss = &(ic->ic_mbss_ctx->ic_mbss[grp_id]);
    for (vap_cnt = 0; vap_cnt < ic->ic_mbss_ctx->grp_sz; vap_cnt++) {

         if (ic_mbss->vdev_id_gp[vap_cnt] == IEEE80211_MBSSID_INVALID_VDEV_ID)
             continue;

         tmp_vdev = wlan_objmgr_get_vdev_by_id_from_pdev(
                            ic->ic_pdev_obj, ic_mbss->vdev_id_gp[vap_cnt],
                            WLAN_MLME_NB_ID);
         if (!tmp_vdev) {
             mbss_debug("vdev obj retrieval failed for vap %d",
                          ic_mbss->vdev_id_gp[vap_cnt]);
             continue;
         }

         tmp_vap = wlan_vdev_mlme_get_ext_hdl(tmp_vdev);
         if (!tmp_vap) {
             mbss_debug("vap retrieval failed for vap %d",
                          ic_mbss->vdev_id_gp[vap_cnt]);
             wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
             continue;
         }
         switch (mbss_args->purpose) {
            case MBSS_ITER_FUNC_PURPOSE_RESTART_VAP:
            {
               osifp = (osif_dev *)wlan_vap_get_registered_handle(tmp_vap);
               dev = osifp->netdev;

               if (!IS_UP(dev)) {
                   wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                   continue;
               }

               if (mbss_args->args)
                   restart_vap = *(bool*)mbss_args->args;

               if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP &&
                      ic->ic_is_vdev_restart_sup && restart_vap)
                   retv = osif_vdev_restart(tmp_vap);
               else
                   retv = osif_vap_init(osifp, dev, RESCAN);

               if (retv) {
                   mbss_err("vap restart failed for vap %d", tmp_vap->iv_unit);
                   mbss_args->retval = QDF_STATUS_E_INVAL;
                   wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                   return;
               }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_UPDATE_DTIM_INTL:
            {
               if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP &&
                   IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap)) {
                   wlan_util_vdev_mlme_get_param(tmp_vap->vdev_mlme,
                             WLAN_MLME_CFG_DTIM_PERIOD, &curr_dtim_period);

                   /*
                    * if current dtim period is multiple of max pp
                    * then no dtim update required.
                    */
                   if (!(curr_dtim_period % ic_mbss->max_pp)) {
                       wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                       continue;
                   }

                   mlme_cfg.value = ic_mbss->max_pp;
                   wlan_util_vdev_mlme_set_param(tmp_vap->vdev_mlme,
                                                 WLAN_MLME_CFG_DTIM_PERIOD,
                                                 mlme_cfg);
               }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_CHECK_DEFAULT_TX_VAP:
            {
                if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP &&
                        !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap)) {
                    mbss_args->is_default_tx_vap = true;
                    mbss_debug("default_tx_vap grp_id:%d vdev_idx:%d vdev id:%d",
                                tmp_vap->iv_mbss.group_id, vap_cnt,
                                ic_mbss->vdev_id_gp[vap_cnt]);
                    wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                    return;
                }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_REPURPOSE_ALLOWED:
            {
                if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP &&
                        (wlan_vdev_mlme_is_active(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
                    *((bool*)mbss_args->args) = true;
                    mbss_info("grp_id:%d vap-%d(%s) is active",
                            tmp_vap->iv_mbss.group_id,
                            tmp_vap->iv_unit, tmp_vap->iv_netdev_name);
                    wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                    return;
                }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_REPURPOSE_PHYMODE_CHECK:
            {
                if (mbss_args->args) {
                    enum ieee80211_phymode repurpose_vap_des_mode =
                                   *((enum ieee80211_phymode *)mbss_args->args);
                    if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP &&
                            (tmp_vap->iv_des_mode != repurpose_vap_des_mode)) {
                        qdf_info("Mismatch in repurpose vap des mode:%d and tmp_vap-%d(%s) tmp_vap des mode:%d",
                                repurpose_vap_des_mode,tmp_vap->iv_unit,
                                tmp_vap->iv_netdev_name, tmp_vap->iv_des_mode);
                        mbss_args->retval = QDF_STATUS_E_INVAL;
                        wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                        return;
                    }
                }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_ADD_MLO_RNR:
            {
#ifdef WLAN_FEATURE_11BE_MLO
                bool mlo_rnr_added = false, is_mlo_vap_present = false, is_20tu = false;
                struct mlo_rnr_param *param = (struct mlo_rnr_param *) mbss_args->args;
                uint8_t *s_frm, *frm = NULL;

                if (param) {
                    s_frm = param->frm;
                    is_20tu = param->is_20tu_prb ? true : false;

                    if (wlan_vdev_mlme_is_mlo_ap(tmp_vap->vdev_obj)) {
                        /* Add MLO Partner info in RNR for
                         * 1. A Transmitted BSSID Or
                         * 2. A Non-transmitted BSSID which is in `UP` state
                         */
                        if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap) ||
                            (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap) &&
                             wlan_mbss_beaconing_vdev_up(tmp_vap->vdev_obj))) {
                            frm = ieee80211_add_basic_mlo_rnr_ie(param->frm,
                                    param->bo, tmp_vap, (uint8_t **)param->temp_bo,
                                    param->subtype, param->rs, is_20tu);

                            if (frm > s_frm) {
                                param->frm = frm;
                                mlo_rnr_added = true;
                            }

                            is_mlo_vap_present = true;
                        }
                    }

                    qdf_trace_hex_dump(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_DEBUG, s_frm, (param->frm - s_frm));

                    if (is_mlo_vap_present) {
                        if ((vap_cnt == ic->ic_mbss_ctx->grp_sz - 1) &&
                                !mlo_rnr_added) {
                            param->frm = s_frm;
                        }
                    } else {
                        param->frm = s_frm;
                    }
                }
#endif
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_POPULATE_MLO_RNR_MLD_ID_LB:
            {
#ifdef WLAN_FEATURE_11BE_MLO
                struct mlo_rnr_mld_id_param *param = (struct mlo_rnr_mld_id_param *) mbss_args->args;
                if (param) {
                    populate_mld_id_and_bpcc(param->ic_6g, tmp_vap,
                            (ieee80211_mlo_rnr_tbtt_info_set_t *)param->p_tbtt_info,
                            param->subtype, param->rs);
                }
#endif
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_GET_BSSID_INDEX_VAP:
            {
#ifdef WLAN_FEATURE_11BE_MLO
                struct ieee80211_framing_extractx *extractx = (struct ieee80211_framing_extractx *) mbss_args->args;
                if (extractx) {
                    if (extractx->ml_preq_info.mlid == tmp_vap->vdev_mlme->mgmt.mbss_11ax.profile_idx) {
                        extractx->non_tx_vap = tmp_vap;
                        mbss_debug("Found the matching vap: %p vdev_idx:%d vdev id: %d ",
                                    tmp_vap, vap_cnt, ic_mbss->vdev_id_gp[vap_cnt]);
                        wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                        return;
                    }
                }
#endif
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_SEND_NON_TX_VAP_T2LM_CMD:
#ifdef WLAN_FEATURE_11BE
            if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap))
                break;

            if (!wlan_vdev_mlme_is_mlo_ap(tmp_vdev)) {
                t2lm_debug("non-MLO tmp vdev:%d:%p",
                        wlan_vdev_get_id(tmp_vdev), tmp_vdev);
                break;
            }

            retv = ieee80211_send_non_tx_vap_t2lm_link_map_config(tmp_vap);
            if (retv) {
                t2lm_err("failed to send T2LM config to FW, vdev:%d",
                        wlan_vdev_get_id(tmp_vdev));
                mbss_args->retval = QDF_STATUS_E_FAILURE;
            }
#endif
            break;
            case MBSS_ITER_FUNC_PURPOSE_HANDLE_TX_VAP_UP:
            {
                /* Handle below items for already up non-tx vaps.
                 *  1. Update RNR cache of each non-Tx 6G vap.
                 *  2. Handle MLO Link Add
                 *     a. Set CU CAT-2 bit on each non-tx vap.
                 *     b. Update beacon template on partner links of each non-tx vap
                 */
                if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) && tmp_vap->iv_ic &&
                    IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap) &&
                    ieee80211_is_vap_state_running(tmp_vap)) {
                    if (IEEE80211_IS_CHAN_6GHZ(tmp_vap->iv_ic->ic_curchan)) {
                        wlan_update_6ghz_rnr_cache(tmp_vap, 1);
                        if (!tmp_vap->iv_6g_band_count) {
                            tmp_vap->iv_6g_band_count = 1;
                            wlan_rnr_6ghz_vdev_inc();
                        }
                        *((bool*)mbss_args->args) = true;
                    }
#ifdef WLAN_FEATURE_11BE_MLO
                    if (ic_mbss->transmit_vap &&
                        wlan_vdev_mlme_op_flags_get(ic_mbss->transmit_vap->vdev_obj,
                                WLAN_VDEV_OP_MLO_LINK_ADD)) {
                        wlan_vdev_mlme_op_flags_set(tmp_vap->vdev_obj, WLAN_VDEV_OP_CU_CAT2);
                        ieee80211_update_beacon_tmpl_for_partner_link(tmp_vap->vdev_obj);
                    }
#endif /* WLAN_FEATURE_11BE_MLO */
                }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_GET_NUM_VAP_PER_GRP:
            {
                 if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                     ieee80211_is_vap_state_running(tmp_vap)){
                     num_vaps += 1;
                     *((uint8_t*)mbss_args->args) = num_vaps;
                 }
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_LINK_REMOVAL_VALIDATION:
            {
#ifdef WLAN_FEATURE_11BE_MLO
                uint32_t tbtt_count = *((uint32_t *) mbss_args->args);

                if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap) &&
                    (QDF_STATUS_SUCCESS == wlan_vdev_chan_config_valid(tmp_vap->vdev_obj))) {
                    if ((mbss_args->retval =
                        wlan_mlo_validate_link_removal_cmd(tmp_vap, tbtt_count, true))) {
                        mbss_err("Validation of MLO link removal command failed for Vap: %s",
                                 tmp_vap->iv_netdev_name);
                        wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                        return;
                    }
                }
#endif
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_LINK_REMOVE:
            {
#ifdef WLAN_FEATURE_11BE_MLO
                uint32_t tbtt_count = *((uint32_t *) mbss_args->args);

                /* Skip for Tx-vap */
                if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap) &&
                    (QDF_STATUS_SUCCESS == wlan_vdev_chan_config_valid(tmp_vap->vdev_obj))) {
                    if((mbss_args->retval = wlan_mlo_link_removal_cmd(tmp_vap, tbtt_count, true))) {
                        mbss_err("Couldn't send the MLO link removal command to the target"
                                 " for non-Tx vap id: %d", tmp_vap->iv_unit);
                        wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
                        return;
                    }
                }
#endif
            }
            break;
            case MBSS_ITER_FUNC_PURPOSE_UPDATE_LINK_DEL_FLAG:
            {
#ifdef WLAN_FEATURE_11BE_MLO
                bool is_link_del_flag_set = *((bool *) mbss_args->args);

                if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmp_vap)) {
                    if (is_link_del_flag_set) {
                        if (wlan_vdev_chan_config_valid(tmp_vap->vdev_obj)
                                == QDF_STATUS_SUCCESS) {
                            /* Set link-del vdev-flag */
                            wlan_vdev_mlme_op_flags_set(tmp_vap->vdev_obj,
                                    WLAN_VDEV_OP_MLO_STOP_LINK_DEL);
                            tmp_vap->ml_ap_removal_in_progress = true;

                            /* Reset this flag not to send deauth during link removal */
                            tmp_vap->iv_send_deauth = 0;
                        }
                    } else {
                            tmp_vap->ml_ap_removal_in_progress = false;
                            /* Clear link-remove onging flag */
                            wlan_vdev_mlme_op_flags_clear(tmp_vap->vdev_obj,
                                                WLAN_VDEV_OP_MLO_STOP_LINK_DEL);

                            /* Clear link-remove tbtt-complete mark */
                            wlan_vdev_mlme_op_flags_clear(tmp_vap->vdev_obj,
                                            WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE);
                            if (wlan_vdev_mlme_op_flags_get(tmp_vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS))
                                wlan_vdev_dec_skip_pumac_cnt(tmp_vap->vdev_obj);
                            wlan_vdev_mlme_op_flags_clear(tmp_vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS);
                    }
                }
#endif
            }
            break;
            default:
                mbss_err("Invalid purpose (%d)", mbss_args->purpose);
         }
         wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
    }
}

/*
 * ieee80211_mbss_switch_mode_to_cohosted:
 * Switch the MBSS mode to legacy co-hosted mode.
 *
 * @ic: Pointer to the ic structure
 * @current_txvap: Pointer to the current Tx-VAP
 *
 * Return:
 * MBSS_SWITCH_SUCCESS  : Successful switch to co-hosted mode
 * MBSS_SWITCH_E_RECOVER: Failed to switch but recovered to original mode
 * MBSS_SWITCH_GOTO_ROLLBACK: Notify caller for rollback operation (if applicable)
 */
static inline
mbss_switch_status_t ieee80211_mbss_switch_mode_to_cohosted(struct ieee80211com *ic,
                                             struct ieee80211vap *current_txvap,
                                             int grp_id)
{
    QDF_STATUS retval = QDF_STATUS_SUCCESS;
    struct mbss_iter_func_args iter_func_args = {0};
    uint8_t temp_ref_bssid[IEEE80211_ADDR_LEN];
    struct mbssid_context *mbss_ctx = NULL;
    struct mbssid_info   *ic_mbss = NULL;

    if (!ic) {
        qdf_err("ic is invalid, cannot recover");
        return MBSS_SWITCH_E_RECOVER;
    }

    if(!ic->ic_mbss_ctx) {
        qdf_err("ic_mbss_ctx is null");
        return MBSS_SWITCH_E_RECOVER;
    }
    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss = &mbss_ctx->ic_mbss[grp_id];

    if (!current_txvap) {
        qdf_err("current_txvap is null");
        return MBSS_SWITCH_E_RECOVER;
    }

    /*
     * Reset the Tx VAP, if set:
     * Future switches to EMA mode requires a NULL transmit_vap pointer.
     * If transmit_vap is not NULL, current VAP should be used to reset the
     * Tx VAP.
     */
    if (ic_mbss->transmit_vap
            && ieee80211_ucfg_reset_txvap(current_txvap, true)) {
        qdf_err("Could not reset Tx-VAP");
        return MBSS_SWITCH_E_RECOVER;
    }

    /* Reset the bssid-idx pool */
    qdf_mem_zero((void *) &ic_mbss->bssid_index_bmap,
		    sizeof(ic_mbss->bssid_index_bmap));

    /*Set vdev_id_gp to IEEE80211_MBSSID_INVALID_VDEV_ID */
    qdf_mem_set(&ic_mbss->vdev_id_gp, sizeof(ic_mbss->vdev_id_gp),
            IEEE80211_MBSSID_INVALID_VDEV_ID);
    /*
     * Save the current ref-BSSID to reassign later on:
     * Creating VAPs in co-hosted mode should align to ref-BSSID rules of the
     * first VAP created in EMA mode.
     */
    IEEE80211_ADDR_COPY(temp_ref_bssid, mbss_ctx->ref_bssid);

    /*
     * Mark all the cache entries of the created VAPs are stale.
     * If invalidation fails, the sequence can rollback to original EMA mode.
     */
    iter_func_args.purpose = MBSS_ITER_FUNC_PURPOSE_CACHE_INVALIDATION;
    iter_func_args.retval  = QDF_STATUS_SUCCESS;
    iter_func_args.args    = (void *) &grp_id;
    wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                                      ieee80211_mbss_update_iter_func,
                                      &iter_func_args, 0, WLAN_MLME_SB_ID);
    if (retval) {
        return MBSS_SWITCH_GOTO_ROLLBACK;
    }

    /*
     * Put back the original ref-BSSID:
     * MBSSID cache update API will reassign the ref-BSSID to a random MAC
     * after the last VAP is cleared. This will cause future created VAPs to
     * have a complete different MAC address than the rest of the MBSS group
     * members.
     */
    IEEE80211_ADDR_COPY(mbss_ctx->ref_bssid, temp_ref_bssid);

    /*
     * Iterate through all VAPs and update the AID bitmap and DTIM periods
     * as per co-hosted norms. If update fails, the sequence cannot rollback to
     * original EMA mode.
     */
    qdf_mem_zero(&iter_func_args, sizeof(struct mbss_iter_func_args));
    iter_func_args.purpose = MBSS_ITER_FUNC_PURPOSE_MODE_SWITCH_COHOSTED;
    iter_func_args.retval  = QDF_STATUS_SUCCESS;
    iter_func_args.args    = (void *) &grp_id;
    wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                            ieee80211_mbss_update_iter_func,
                            &iter_func_args, 0, WLAN_MLME_SB_ID);
    if (retval) {
        return MBSS_SWITCH_E_NORECOVER_NOMEM;
    }

    /*
     * Reset the non-inherit flag when moving back to co-hosted mode.
     */
    mbss_ctx->non_inherit_enable = false;

    return MBSS_SWITCH_SUCCESS;
}

/*
 * ieee80211_mbss_switch_mode:
 * Switch the multi-BSS mode between co-hosted and EMA mode.
 *
 * @ic         : Pointer to the ic structure.
 * @target_mode: Value of the target multi-BSS mode.
 *
 * Return:
 * MBSS_SWITCH_SUCCESS: Successful
 * MBSS_SWITCH_E_RECOVER: Failed but rollback
 * MBSS_SWITCH_E_NORECOVER: Failed and rollback failed
 */
mbss_switch_status_t ieee80211_mbss_switch_mode(struct ieee80211com *ic,
                                                mbss_mode_t target_mode,
                                                bool is_wfa_test)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_ath_soc_softc_t *soc = NULL;
    mbss_mode_t current_mode;
    bool old_noninherit_enable = false;
    struct ieee80211vap *txvap = NULL;
    struct mbssid_context  *mbss_ctx = NULL;
    mbss_switch_status_t retval = MBSS_SWITCH_SUCCESS;
    int grp_id = 0, i = 0;
    if (!ic) {
        qdf_err("Invalid ic");
        return MBSS_SWITCH_E_RECOVER;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc = scn->soc;

    if (!ic->ic_wideband_capable && !is_wfa_test) {
        qdf_err("Wideband is not supported - Aborting");
        return MBSS_SWITCH_E_RECOVER;
    }

    if (ic->is_mbssid_enabled) {
        /*
         * Wideband radio requires EMA to be applied alongside the
         * MBSSID-IE flag. Since this function currently supports only the
         * wideband radio, MBSSID-IE support also implies EMA.
         */
        current_mode = MBSS_MODE_MBSSID_EMA;
    } else {
        current_mode = MBSS_MODE_COHOSTED;
    }

    if (target_mode == current_mode) {
        qdf_err("Current and target MBSS modes are the same - Skipping");
        return MBSS_SWITCH_SUCCESS;
    }

    mbss_ctx = ic->ic_mbss_ctx;
    if (mbss_ctx == NULL) {
        qdf_err("mbss_ctx is NULL");
        return MBSS_SWITCH_E_RECOVER;
    }

    /*
     * If current mode is COHOSTED  : Tx VAP points to target Tx VAP
     * If current mode is MBSSID_EMA: Tx VAP points to current Tx VAP
     */

    if (target_mode == MBSS_MODE_MBSSID_EMA) {
        ieee80211_mbss_update_flags(ic, target_mode);
    }

    for (grp_id = 0; grp_id < mbss_ctx->max_ngroups; grp_id++) {

        txvap = ieee80211_ucfg_get_txvap(ic, grp_id, current_mode);
        if (!txvap && (target_mode == MBSS_MODE_MBSSID_EMA)) {
            qdf_err("Could not find Tx-VAP for a given group %d ", grp_id);
            continue;
        }
        if (!txvap && (target_mode == MBSS_MODE_COHOSTED)) {
            qdf_err("Tx-VAP is NULL and target mode is cohosted, Skip group: %d", grp_id);
            continue;
        }

        /* Capture the old non-inherit state in case rollback in invoked */
        old_noninherit_enable = mbss_ctx->non_inherit_enable;

        if (target_mode == MBSS_MODE_COHOSTED) {
            mbss_debug("groupid: %d Moving to cohosted for txvap: %d", grp_id, txvap->iv_unit);
            retval = ieee80211_mbss_switch_mode_to_cohosted(ic, txvap, grp_id);
            if ((retval == MBSS_SWITCH_E_RECOVER) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_INVAL) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_NOMEM)) {
                return retval;
            } else if (retval == MBSS_SWITCH_GOTO_ROLLBACK) {
                goto exit_backto_ema;
            }

        } else {
            /* Co-hosted to EMA switch */
            qdf_debug("groupid: %d Moving to ema for txvap: %d", grp_id, txvap->iv_unit);
            retval = ieee80211_mbss_switch_mode_to_ema(ic, txvap, grp_id);
            if ((retval == MBSS_SWITCH_E_RECOVER) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_INVAL) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_NOMEM)) {
                return retval;
            } else if (retval == MBSS_SWITCH_GOTO_ROLLBACK) {
                goto exit_backto_cohost;
            }
        }
    }

    ieee80211_mbss_update_flags(ic, target_mode);
    /* reset bssid_index_bmap for super group */
    if (target_mode == MBSS_MODE_COHOSTED) {
        qdf_mem_zero((void *)&mbss_ctx->bssid_index_bmap,
                sizeof(mbss_ctx->bssid_index_bmap));

        qdf_mem_zero((void *)&mbss_ctx->sec_group_bitmap,
                sizeof(mbss_ctx->sec_group_bitmap));
    }
    qdf_err("Successfully switched to mode %d", target_mode);
    return MBSS_SWITCH_SUCCESS;

        /* Rollback as per current mode */
exit_backto_cohost: /* Failed co-hosted to EMA switch. Try to revert back to co-hosted */
    if (current_mode == MBSS_MODE_COHOSTED) {
        /*
         * loop for grp_id = 0 to n_groups
         * find TX VAP for a given group with EMA Mode as argument
         * if tx vap is present that group is in EMA mode; so move to
         * ieee80211_mbss_switch_mode_to_cohosted
         * if tx vap is NULL skip as that group is in co hosted mode
         *
         */

        qdf_err("Roll back to CoHosted mode");

        for (i = 0; i < ic->ic_mbss_ctx->max_ngroups; i++) {
            txvap = ieee80211_ucfg_get_txvap(ic, i, MBSS_MODE_MBSSID_EMA);
            if (txvap == NULL) {
                continue;
            }
            retval = ieee80211_mbss_switch_mode_to_cohosted(ic, txvap, i);
            if ((retval == MBSS_SWITCH_E_RECOVER) ||
                    (retval == MBSS_SWITCH_GOTO_ROLLBACK) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_INVAL) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_NOMEM)) {
                return MBSS_SWITCH_E_NORECOVER_INVAL;
            }
        }
    }
    ieee80211_mbss_update_flags(ic, current_mode);

exit_backto_ema: /* Failed switch from EMA to co-hosted, move back to EMA */
    if (current_mode == MBSS_MODE_MBSSID_EMA) {
        /*
         * loop for grp_id = 0 to n_groups
         * In EMA Mode TX VAP non NULL for a group means already group formed; so skip
         * TX VAP is NULL for group mean that group is moved to cohosted; call
         * ieee80211_mbss_switch_mode_to_ema
         *
         */
        qdf_err("Roll back to EMA mode");
        for (i = 0; i < ic->ic_mbss_ctx->max_ngroups; i++) {
            txvap = ieee80211_ucfg_get_txvap(ic, i, current_mode);
            if (txvap)
                continue;

            retval = ieee80211_mbss_switch_mode_to_ema(ic, txvap, i);
            if ((retval == MBSS_SWITCH_E_RECOVER) ||
                    (retval == MBSS_SWITCH_GOTO_ROLLBACK) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_INVAL) ||
                    (retval == MBSS_SWITCH_E_NORECOVER_NOMEM)) {
                return MBSS_SWITCH_E_NORECOVER_INVAL;
            }
        }
    }
    /* Revert the non_inherit_enable flag back to original value. */
    ic->ic_mbss_ctx->non_inherit_enable = old_noninherit_enable;
    mbss_info("Rollback to mode %d successful", current_mode);
    return MBSS_SWITCH_E_RECOVER;
}

/*
 * ieee80211_mbss_mode_switch_sanity:
 * Check if the current configuration supports switching the multi-VAP mode
 *
 * @ic: Pointer to the ic structure.
 * @value: Value of the desired target mode.
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_INVAL: Failure
 */
QDF_STATUS ieee80211_mbss_mode_switch_sanity(struct ieee80211com *ic,
                                             uint32_t value, bool is_wfa_test)
{
    bool is_mbssid_enabled = false;

    if (!ic) {
        qdf_err("Invalid ic pointer");
        return QDF_STATUS_E_INVAL;
    }

    if (value > MBSS_MODE_MBSSID_EMA) {
        qdf_err("Invalid input value (%d)", value);
        return QDF_STATUS_E_INVAL;
    }

    /* Dynamic mode change is supported for wideband target only.
     * Only in case of wfa test it is allowed on non wideband targets.
     */
    if (!ic->ic_wideband_capable && !is_wfa_test) {
        qdf_err("Radio does not support wideband (5-7GHz), cannot perform "
                "mode switch");
        return QDF_STATUS_E_INVAL;
    }

    is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                          WLAN_PDEV_F_MBSS_IE_ENABLE);
    if (((value == MBSS_MODE_COHOSTED) && !is_mbssid_enabled) ||
        ((value == MBSS_MODE_MBSSID_EMA) && is_mbssid_enabled)) {
        qdf_err("Current mode and target mode are the same, skipping mode "
                "switch");
        return QDF_STATUS_E_INVAL;
    }

    if ((value == MBSS_MODE_MBSSID_EMA) &&
        ieee80211_get_num_beacon_ap_vaps(ic) &&
        wlan_cfg80211_mbssid_security_admission_control_sanity(ic, false)) {
        qdf_err("Security validation for mode switch failed");
        return QDF_STATUS_E_INVAL;
    }

    qdf_debug("Sanity check for mode switch to mode %d passed", value);
    return QDF_STATUS_SUCCESS;
}

/*
 * ieee80211_mbss_handle_mode_switch:
 * Handle the mode switch including the VDEV state changes involved.
 *
 * @ic: Pointer to the ic structure
 * @target_mode: Value of the desired target MBSS mode
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_INVAL:
 * QDF_STATUS_E_NOMEM: Failed to switch
 */
QDF_STATUS ieee80211_mbss_handle_mode_switch(struct ieee80211com *ic,
                                             mbss_mode_t target_mode,
                                             bool is_wfa_test)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_ath_soc_softc_t *soc = NULL;
    uint8_t retval = QDF_STATUS_SUCCESS;

    if (!ic) {
        qdf_err("Invalid ic pointer");
        return QDF_STATUS_E_INVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc = scn->soc;

    if (!ieee80211_get_num_beacon_ap_vaps(ic)) {
        /*
         * If the VAP list is empty, perform the switch silently
         * by only changing the flags.
         */
        ieee80211_mbss_update_flags(ic, target_mode);
        qdf_info("Silently switched MBSS mode to %d", target_mode);
        return QDF_STATUS_SUCCESS;
    }

    if (ieee80211_bringdown_all_vaps(ic, ALL_AP_VDEVS)) {
        qdf_err("Could not bringdown AP vdevs");
        return QDF_STATUS_E_INVAL;
    }

    retval = ieee80211_mbss_switch_mode(ic, target_mode, is_wfa_test);

    switch (retval) {
        case MBSS_SWITCH_E_NORECOVER_INVAL:
            qdf_err("Could not recover from failed mode switch to %d"
                    "due to invalid params", target_mode);
            return QDF_STATUS_E_INVAL;
        break;
        case MBSS_SWITCH_E_NORECOVER_NOMEM:
            qdf_err("Could not recover from failed mode switch to %d "
                    "due to no memory", target_mode);
            return QDF_STATUS_E_NOMEM;
        break;
        case MBSS_SWITCH_E_RECOVER:
            qdf_err("Recovered from failed mode switch to %d", target_mode);
            retval = QDF_STATUS_E_INVAL;
        break;
        case MBSS_SWITCH_SUCCESS:
            qdf_debug("Successfully switched mode to %d", target_mode);
            retval = QDF_STATUS_SUCCESS;
        break;
        default:
            qdf_debug("Invalid return value: %d", retval);
            return QDF_STATUS_E_INVAL;
    }

    if (ieee80211_bringup_all_vaps(ic, ALL_AP_VDEVS)) {
        qdf_err("Could not bring up VAPs");
        retval = QDF_STATUS_E_INVAL;
    }

    return retval;
}

/*
 * ieee80211_get_beacon_vap_iter_func:
 * Iteration function for each VAP to check for number of beaconing AP VAPs.
 *
 * @arg: Opaque handle to the variable holding the count
 * @vap: Pointer to the current VAP structure
 */
static void ieee80211_get_beacon_vap_iter_func(void *arg,
                                               struct ieee80211vap *vap)
{
    uint8_t *beacon_vaps = (uint8_t *)arg;

    if (!vap || !vap->vdev_obj) {
        return;
    }

    if (!ieee80211_mbss_is_beaconing_ap(vap)) {
        return;
    }

    ++(*beacon_vaps);
}

/*
 * ieee80211_get_num_beacon_ap_vaps:
 * Get the number of created AP VAPs that are capable of beaconing
 * That is, all HOSTAP VAPs excluding special_mode_vap and smart_ap_monitor
 *
 * @ic: Pointer to the ic structure
 *
 * Return:
 * Number of VAPs
 */
uint8_t ieee80211_get_num_beacon_ap_vaps(struct ieee80211com *ic)
{
    uint8_t num_beacon_ap_vaps = 0;

    wlan_iterate_vap_list(ic,
                          ieee80211_get_beacon_vap_iter_func,
                          (void *)&num_beacon_ap_vaps);

    return num_beacon_ap_vaps;
}

static QDF_STATUS get_6g_pdev(void *ic, void *mlo_non_mlo_6Ghz_pdev_cnt)
{
    bool is_mlo = false;
    uint32_t mode = ieee80211_get_current_phymode(ic);

    is_mlo = ieee80211_is_phymode_11be(mode);

    if(is_mlo){
         *((uint8_t *)mlo_non_mlo_6Ghz_pdev_cnt + 0) += 1;
    }else{
         *((uint8_t *)mlo_non_mlo_6Ghz_pdev_cnt + 1) += 1;
    }

    return QDF_STATUS_SUCCESS;
}

void ieee80211_get_6g_mlo_and_non_mlo_cnt(uint8_t *mlo_non_mlo_6Ghz_pdev_cnt)
{
     wlan_rnr_6ghz_iterator(get_6g_pdev, mlo_non_mlo_6Ghz_pdev_cnt, NULL);
}

int ieee80211_get_max_user_rnr_size_allowed(struct ieee80211com *ic)
{
    int max_size = 0;
    /* mlo_non_mlo_6Ghz_pdev_cnt[0]  is mlo 6GHz Pdev count
     * mlo_non_mlo_6Ghz_pdev_cnt[1]  is non-mlo 6GHz Pdev count
     */
    uint8_t  mlo_non_mlo_6Ghz_pdev_cnt[2] = {0};
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                WLAN_PDEV_F_MBSS_IE_ENABLE);
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    uint16_t reserved_6Ghz_rnr;
    uint16_t reserved_6Ghz_mlo_rnr;
    uint16_t reserved_6Ghz_non_mlo_rnr;
    uint32_t mode = ieee80211_get_current_phymode(ic);
    int8_t num_active_links;

#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t grp_id;
    grp_id = wlan_mlo_get_psoc_group_id(scn->soc->psoc_obj);
    num_active_links = ieee80211_mlo_get_num_active_links(grp_id);
    if (!num_active_links || num_active_links == WLAN_MLO_INVALID_NUM_LINKS) {
        /* Assign default active links to 1 */
        num_active_links = 1;
    }
#else
    num_active_links = 1;
#endif

    /* Max user rnr size allowed for Mbss case is determined by INI
     * (rnr_field_size_limit). This INI takes into account 2 bytes
     * per IE for tag and tag length. Subtract this tag, length field from
     * rnr_field_size_limit to get max user rnr allowed.
     */
    if (is_mbssid_enabled && IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
        if (ieee80211_is_phymode_11be(mode)){
            reserved_6Ghz_rnr = MLO_RESERVED_6GHZ_RNR;
            if (ic->ic_mbss_ctx) {
                reserved_6Ghz_rnr = (((ic->ic_mbss_ctx->grp_sz - 1) *
                        MLO_TBTT_INFO_FIELD_SIZE) + RNR_NBR_AP_INFO_HDR_SIZE) +
                        (ic->ic_mbss_ctx->grp_sz * (num_active_links -1) *
                        (MLO_TBTT_INFO_FIELD_SIZE + RNR_NBR_AP_INFO_HDR_SIZE));
            }
        } else {
            reserved_6Ghz_rnr = RESERVED_6GHZ_RNR;
            if (ic->ic_mbss_ctx) {
                reserved_6Ghz_rnr = ((ic->ic_mbss_ctx->grp_sz - 1) *
                        TBTT_INFO_FIELD_SIZE) + RNR_NBR_AP_INFO_HDR_SIZE;
            }
        }

        max_size = scn->ema_ctx.rnr_field_size_limit -
                   reserved_6Ghz_rnr - (2 * scn->soc->max_rnr_ie_allowed);
    } else {
        reserved_6Ghz_mlo_rnr = (MLO_RESERVED_6GHZ_RNR + ((num_active_links -1) *
                           MLO_TBTT_INFO_FIELD_SIZE));
        reserved_6Ghz_non_mlo_rnr = RESERVED_6GHZ_RNR;

        ieee80211_get_6g_mlo_and_non_mlo_cnt(mlo_non_mlo_6Ghz_pdev_cnt);

        reserved_6Ghz_rnr = (reserved_6Ghz_mlo_rnr * mlo_non_mlo_6Ghz_pdev_cnt[0]) +
               (reserved_6Ghz_non_mlo_rnr * mlo_non_mlo_6Ghz_pdev_cnt[1]);

        max_size = (scn->soc->max_rnr_ie_allowed * IEEE80211_MAX_IE_LEN) -
              reserved_6Ghz_rnr;
    }
    return max_size;
}

int wlan_do_iter_fun_mbss_group(wlan_dev_t ic, uint8_t grp_id,
                                mbss_iter_func_purpose_t purpose,
                                void *args)
{
    int retv = 0;
    struct mbss_iter_func_args iter_func_args = {0};

    iter_func_args.purpose = purpose;
    iter_func_args.retval = QDF_STATUS_SUCCESS;
    iter_func_args.args = args;

    wlan_mbss_group_iterate_vdev_list(ic, grp_id, &iter_func_args);

    if (iter_func_args.retval !=QDF_STATUS_SUCCESS)
        retv = -EINVAL;

    return retv;
}


void ieee80211_init_rnr_ie_ctx(struct ieee80211vap *vap, u_int8_t subtype, bool is_20tu_prb)
{
        struct rnr_ie_ctx *rnr_ctx = NULL;

        switch (subtype) {

                case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
                        if (!is_20tu_prb) {
                            rnr_ctx = &vap->prb_rnr_ie;
                        } else {
                            rnr_ctx = &vap->prb_20tu_rnr_ie;
                        }
                        break;
                default:
                        rnr_ctx = &vap->bcn_rnr_ie;
                        break;
        }
        rnr_ctx->curr_rnr_ie = 0;
        rnr_ctx->curr_rnr_available_space = IEEE80211_MAX_IE_LEN;
        rnr_ctx->curr_rnr_start_frm = NULL;

}
void ieee80211_update_rnr_ie_ctx(struct ieee80211vap *vap,
                                 uint8_t curr_rnr_ie, uint8_t curr_rnr_available_space,
                                 uint8_t *curr_rnr_start_frm, u_int8_t subtype, bool is_20tu_prb)
{
        struct rnr_ie_ctx *rnr_ctx = NULL;

        switch(subtype)  {

                case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
                        if (!is_20tu_prb) {
                            rnr_ctx = &vap->prb_rnr_ie;
                        } else {
                            rnr_ctx = &vap->prb_20tu_rnr_ie;
                        }
                        break;
                default:
                        rnr_ctx = &vap->bcn_rnr_ie;
                        break;
        }
        rnr_ctx->curr_rnr_ie = curr_rnr_ie;
        rnr_ctx->curr_rnr_available_space = curr_rnr_available_space;
        rnr_ctx-> curr_rnr_start_frm = curr_rnr_start_frm;
}

bool ieee80211_is_rnr_space_available_for_one_tbtt_entry(struct ieee80211vap *vap, bool is_mlo,
                                                         u_int8_t subtype, bool is_20tu_prb)
{
        struct rnr_ie_ctx *rnr_ctx = NULL;

        switch (subtype) {

                case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
                        if (!is_20tu_prb) {
                            rnr_ctx = &vap->prb_rnr_ie;
                        } else {
                            rnr_ctx = &vap->prb_20tu_rnr_ie;
                        }
                        break;
                default:
                        rnr_ctx = &vap->bcn_rnr_ie;
                        break;
        }

        if (is_mlo) {
            return ((RNR_NBR_AP_INFO_HDR_SIZE + MLO_TBTT_INFO_FIELD_SIZE) <=
                    rnr_ctx->curr_rnr_available_space);
        } else {
            return ((RNR_NBR_AP_INFO_HDR_SIZE + TBTT_INFO_FIELD_SIZE) <=
                    rnr_ctx->curr_rnr_available_space);
        }
}

struct rnr_ie_ctx *ieee80211_get_rnr_ctx(struct ieee80211vap *vap,
                                         u_int8_t subtype, bool is_20tu_prb)
{
        switch (subtype) {
                case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
                        if (!is_20tu_prb)
                            return &vap->prb_rnr_ie;

                        return &vap->prb_20tu_rnr_ie;
                default:
                        return &vap->bcn_rnr_ie;
        }
}

QDF_STATUS wlan_pdev_set_config_params(struct wlan_objmgr_pdev *pdev,
                                     config_params params,
                                     uint32_t value)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    struct pdev_mlme_obj *pdev_mlme = NULL;

    pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
    if (!pdev_mlme) {
         qdf_err("pdev mlme struct is NULL");
         return QDF_STATUS_E_INVAL;
    }

    scn = (struct ol_ath_softc_net80211 *)
            pdev_mlme->ext_pdev_ptr;

    if (!scn) {
         qdf_err("scn is NULL");
         return QDF_STATUS_E_INVAL;
    }

    switch(params) {
       case WLAN_CONFIG_NUM_VDEVS:
             scn->max_params.num_vdevs = value;
             break;
       case WLAN_CONFIG_NUM_PEERS:
             scn->max_params.num_peers = value;
             break;
       case WLAN_CONFIG_NUM_MONITOR_VAPS:
             scn->max_params.num_monitor_vaps = value;
             break;
       case WLAN_CONFIG_NUM_QWRAP_VDEVS:
             scn->max_params.num_qwrap_vdevs = value;
             break;
       case WLAN_CONFIG_NUM_QWRAP_PEERS:
             scn->max_params.num_qwrap_peers = value;
             break;
       case WLAN_CONFIG_VP_CORE_MASK:
             scn->vp_core_mask = value;
             break;
       case WLAN_CONFIG_HOST_TRIGGER_TXBF_SOUNDING:
             scn->host_trig_txbf_sounding = value;
             break;
#if defined(IPA_OFFLOAD) && defined(IPA_OFFLOAD_LOW_MEM)
       case WLAN_CONFIG_NUM_IPA_RX_BUFF:
             scn->num_ipa_rx_buff = value;
             break;
       case WLAN_CONFIG_NUM_IPA_TX_BUFF:
             scn->num_ipa_tx_buff = value;
             break;
#endif
#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
	case WLAN_CONFIG_TX_PKT_CAP_CUSTOM_CLASSIFY_PROTO:
	     scn->tx_pkt_cap_proto = value;
	     break;
#endif
       default:
              return QDF_STATUS_E_INVAL;
             break;
    }

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(wlan_pdev_set_config_params);

uint32_t wlan_pdev_get_config_params(struct wlan_objmgr_pdev *pdev,
                                     config_params params)
{

    struct ol_ath_softc_net80211 *scn = NULL;
    struct pdev_mlme_obj *pdev_mlme = NULL;
    uint32_t value = 0;

    pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
    if (!pdev_mlme) {
         qdf_err("pdev mlme struct is NULL");
         return value;
    }

    scn = (struct ol_ath_softc_net80211 *)
            pdev_mlme->ext_pdev_ptr;

    if (!scn) {
         qdf_err("scn is NULL");
         return value;
    }

    switch(params) {
       case WLAN_CONFIG_NUM_VDEVS:
             value = scn->max_params.num_vdevs;
             break;
       case WLAN_CONFIG_NUM_PEERS:
             value = scn->max_params.num_peers;
             break;
       case WLAN_CONFIG_NUM_MONITOR_VAPS:
             value = scn->max_params.num_monitor_vaps;
             break;
       case WLAN_CONFIG_NUM_QWRAP_VDEVS:
             value = scn->max_params.num_qwrap_vdevs;
             break;
       case WLAN_CONFIG_NUM_QWRAP_PEERS:
             value = scn->max_params.num_qwrap_peers;
             break;
       case WLAN_CONFIG_VP_CORE_MASK:
             value = scn->vp_core_mask;
	     break;
       case WLAN_CONFIG_HOST_TRIGGER_TXBF_SOUNDING:
             value  = scn->host_trig_txbf_sounding;
             break;
       default:
             break;
    }

    return value;
}
qdf_export_symbol(wlan_pdev_get_config_params);

#ifdef WLAN_FEATURE_11BE_MLO
bool ieee80211_mlo_is_mld_ap(struct wlan_objmgr_vdev *vdev)
{
    if (!vdev) {
        mlo_err("vdev is null");
        return false;
    }

    return wlan_vdev_mlme_is_mlo_ap(vdev);
}

void ieee80211_mlo_ap_get_vdev_list(struct wlan_objmgr_vdev *vdev,
                                    uint16_t *vdev_count,
                                    struct wlan_objmgr_vdev **wlan_vdev_list)
{
    if (!vdev) {
        mlo_err("vdev is null");
        return;
    }

    mlo_ap_get_active_vdev_list(vdev, vdev_count, wlan_vdev_list);
}

uint16_t ieee80211_mlo_get_pdev_hw_link_id(struct wlan_objmgr_pdev *pdev)
{
    if (!pdev) {
        mlo_err("vdev is null");
        return 0;
    }
    return wlan_mlo_get_pdev_hw_link_id(pdev);
}

int8_t ieee80211_mlo_get_num_active_links(uint8_t grp_id)
{
    return wlan_mlo_get_num_active_links(grp_id);
}

void ieee80211_mlo_release_vdev_ref(struct wlan_objmgr_vdev *vdev)
{
    if (!vdev) {
        mlo_err("vdev is null");
        return;
    }

    mlo_release_vdev_ref(vdev);
}

QDF_STATUS ieee80211_mlo_release_vdev_list_refs(
    struct wlan_objmgr_vdev **wlan_vdev_list, size_t wlan_vdev_list_max_size,
    u_int16_t vdev_count, u_int16_t start_idx)
{
    u_int16_t i = 0;

    if (wlan_vdev_list == NULL) {
        mlo_err("wlan_vdev_list is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (wlan_vdev_list_max_size == 0) {
        mlo_err("wlan_vdev_list_max_size is 0");
        return QDF_STATUS_E_INVAL;
    }

    if (vdev_count == 0) {
        mlo_err("vdev_count is 0");
        return QDF_STATUS_E_INVAL;
    }

    if (vdev_count > wlan_vdev_list_max_size) {
        mlo_err("vdev_count %u is higher than wlan_vdev_list_max_size %zu",
                vdev_count, wlan_vdev_list_max_size);
        return QDF_STATUS_E_INVAL;
    }

    if (start_idx > (wlan_vdev_list_max_size - 1)) {
        mlo_err("start_idx %u goes beyond index corresponding to wlan_vdev_list_max_size %zu",
                start_idx, wlan_vdev_list_max_size - 1);
        return QDF_STATUS_E_INVAL;
    }

    if (start_idx > (vdev_count - 1)) {
        mlo_err("start_idx %u goes beyond index corresponding to vdev_count %u", start_idx,
                vdev_count - 1);
        return QDF_STATUS_E_INVAL;
    }

    for (i = start_idx; i < vdev_count; i++)
    {
        if (wlan_vdev_list[i] == NULL) {
            mlo_err_rl("VDEV pointer at position %u is NULL.", i);
            continue;
        }

        ieee80211_mlo_release_vdev_ref(wlan_vdev_list[i]);
    }

    return QDF_STATUS_SUCCESS;
}

#if WLAN_SUPPORT_GREEN_AP
void ieee80211_mlo_green_ap_add_sta(struct wlan_objmgr_vdev *vdev, u_int16_t aid)
{
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_vdev *partner_vdev = NULL;
    struct ieee80211_node *ni;
    int i = 0;

    if (!vdev) {
        mlo_err("VDEV passed is NULL");
        return;
    }

    mlo_peer_ctx = wlan_mlo_get_mlpeer_by_aid(vdev->mlo_dev_ctx, aid);
    if (!mlo_peer_ctx) {
        mlo_err("MLO peer context passed is NULL");
        return;
    }

    mlo_peer_lock_acquire(mlo_peer_ctx);

    for (i=0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &mlo_peer_ctx->peer_list[i];

        if (!peer_entry->link_peer)
            continue;

        partner_vdev = wlan_peer_get_vdev(peer_entry->link_peer);
        if (!partner_vdev) {
            mlo_err("Partner vdev not found");
            break;
        }

        if (vdev == partner_vdev)
            continue;

        ni = wlan_peer_get_mlme_ext_obj(peer_entry->link_peer);
        if (!ni) {
            mlo_err("ni is NULL for linked peer");
            break;
        }

        if (!ni->is_assoc_resp_completed) {
            wlan_green_ap_add_sta(wlan_vdev_get_pdev(partner_vdev));
            if (wlan_node_get_max_nss(ni) > 1) {
                wlan_green_ap_add_multistream_sta(wlan_vdev_get_pdev(partner_vdev));
            }
            ni->is_assoc_resp_completed = 1;
        }
    }

    mlo_peer_lock_release(mlo_peer_ctx);

    return;
}
#endif /* WLAN_SUPPORT_GREEN_AP */

/*
 * Generate hash value for HT/VHT/HE/EHT ops IE param
 */
u_int32_t ieee80211_gen_hash_ops_ie(u_int8_t *ops_ie, u_int8_t ops_ie_len)
{
    u_int32_t ops_ie_hash = 0xffffffff;
    int i = 0;

    if (!ops_ie || ops_ie_len > CU_MAX_ELEM_LENGTH)
        return ops_ie_hash;

    for (i = 0; i < ops_ie_len; i++)
        ops_ie_hash = wlan_shortssid_table[(ops_ie_hash ^ ops_ie[i]) & 0xff] ^ (ops_ie_hash >> 8);

    return (ops_ie_hash ^ 0xffffffff);
}

QDF_STATUS ieee80211_get_t2lm_enable(struct ieee80211com *ic,
                                     uint8_t *t2lm_enable)
{
    if (ic->ic_get_t2lm_enabled)
        return ic->ic_get_t2lm_enabled(ic, t2lm_enable);

    return QDF_STATUS_E_FAILURE;
}

QDF_STATUS ieee80211_get_default_t2lm_response(struct ieee80211com *ic,
                                               uint8_t *default_t2lm_resp)
{
    if (ic->ic_get_default_t2lm_response)
        return ic->ic_get_default_t2lm_response(ic, default_t2lm_resp);

    return QDF_STATUS_E_FAILURE;
}

bool
ieee80211_is_tsf_recalculation_required(
    struct wlan_objmgr_vdev **mld_partner_vdev_list,
    uint16_t mld_partner_vdev_count)
{
    uint16_t vdev_index;
    bool tsf_recalculation_required = false;

    if (!mld_partner_vdev_list) {
        qdf_err("MLD partner vdev list is null");
        return false;
    }

    if (mld_partner_vdev_count > WLAN_UMAC_MLO_MAX_VDEVS) {
        qdf_err("MLD partner vdev count greater than threshold");
        return false;
    }

    for (vdev_index = 0; vdev_index < mld_partner_vdev_count; vdev_index++) {
        struct ieee80211vap *vap;
        struct vap_tsf_offset_calc_params *tsf_offset_calc_params;

        vap = wlan_vdev_get_vap(mld_partner_vdev_list[vdev_index]);
        if (!vap) {
            qdf_err("vap is null");
            return false;
        }

        tsf_offset_calc_params = &vap->iv_tsf_offset_calc_params;
        tsf_recalculation_required |= tsf_offset_calc_params->tsf_recalculation_required;
    }

    return tsf_recalculation_required;
}

QDF_STATUS
ieee80211_recalculate_concurrent_tsf_snapshot_for_all_vdevs_mld(
    struct wlan_objmgr_vdev **mld_partner_vdev_list,
    uint16_t mld_partner_vdev_count)
{
    uint16_t vdev_index;

    if (!mld_partner_vdev_list) {
        qdf_err("MLD partner vdev list is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (mld_partner_vdev_count > WLAN_UMAC_MLO_MAX_VDEVS) {
        qdf_err("MLD partner vdev count greater than threshold");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* Read qtimer for each partner vdev */
    for (vdev_index = 0; vdev_index < mld_partner_vdev_count; vdev_index++) {
        struct ieee80211vap *vap;

        vap = wlan_vdev_mlme_get_ext_hdl(mld_partner_vdev_list[vdev_index]);
        if (qdf_unlikely(!vap)) {
            qdf_err("vap is null");
            return QDF_STATUS_E_NULL_VALUE;
        }

        vap->iv_tsf_offset_calc_params.qtimer_snapshot_host =
            vap->iv_ic->ic_get_q6_time((OL_ATH_SOFTC_NET80211(vap->iv_ic))->soc->sc_osdev->device);
    }

    for (vdev_index = 0; vdev_index < mld_partner_vdev_count; vdev_index++) {
        struct ieee80211vap *vap;
        struct vap_tsf_offset_calc_params *tsf_offset_calc_params;
        uint64_t qtimer_delta;

        if (QDF_IS_STATUS_SUCCESS(wlan_vdev_is_dfs_cac_wait(mld_partner_vdev_list[vdev_index])))
            continue;

        vap = wlan_vdev_get_vap(mld_partner_vdev_list[vdev_index]);
        if (!vap) {
            qdf_err("vap is null");
            return QDF_STATUS_E_NULL_VALUE;
        }

        tsf_offset_calc_params = &vap->iv_tsf_offset_calc_params;

        if (!tsf_offset_calc_params->fw_snapshots_valid) {
            qdf_err("Invalid FW snapshots of qtimer and TSF");
            return QDF_STATUS_E_INVAL;
        }
        qtimer_delta = tsf_offset_calc_params->qtimer_snapshot_host - tsf_offset_calc_params->qtimer_snapshot_fw;

        tsf_offset_calc_params->last_calculated_tsf_host = tsf_offset_calc_params->tsf_snapshot_fw + qtimer_delta;
        tsf_offset_calc_params->last_calculated_tsf_host_valid = true;
        tsf_offset_calc_params->tsf_recalculation_required = false;

    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
ieee80211_get_concurrent_tsf_snapshot_for_all_vdevs_mld(
    struct wlan_objmgr_vdev **mld_partner_vdev_list,
    uint16_t mld_partner_vdev_count,
    struct tsf_snapshot *concurrent_tsf_snapshot_list,
    uint16_t concurrent_tsf_snapshot_list_size)
{
    uint16_t vdev_index;
    struct wlan_mlo_dev_context *mlo_dev_ctx;

    if (!mld_partner_vdev_list) {
        qdf_err("MLD partner vdev list null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!concurrent_tsf_snapshot_list) {
        qdf_err("TSF snapshot list is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!mld_partner_vdev_count) {
        qdf_err("MLD partner vdev count zero");
        return QDF_STATUS_E_INVAL;
    }

    if (!concurrent_tsf_snapshot_list_size) {
        qdf_err("TSF snapshot list size zero");
        return QDF_STATUS_E_INVAL;
    }

    if (mld_partner_vdev_count > WLAN_UMAC_MLO_MAX_VDEVS) {
        qdf_err("MLD partner vdev count greater than threshold");
        return QDF_STATUS_E_INVAL;
    }

    if (concurrent_tsf_snapshot_list_size > WLAN_UMAC_MLO_MAX_VDEVS) {
        qdf_err("TSF snapshot list size greater than threshold");
        return QDF_STATUS_E_INVAL;
    }

    if (mld_partner_vdev_count > concurrent_tsf_snapshot_list_size) {
        qdf_err(" MLD partner vdev count > TSF snapshot list size");
        return QDF_STATUS_E_INVAL;
    }

    mlo_dev_ctx = mld_partner_vdev_list[0]->mlo_dev_ctx;
    if (!mlo_dev_ctx) {
        qdf_err("MLO dev context is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    tsf_recalculation_lock_acquire(mlo_dev_ctx);

    if (ieee80211_is_tsf_recalculation_required(mld_partner_vdev_list,
                                                mld_partner_vdev_count)) {
        QDF_STATUS status;

        status = ieee80211_recalculate_concurrent_tsf_snapshot_for_all_vdevs_mld(
                            mld_partner_vdev_list, mld_partner_vdev_count);
        if (QDF_IS_STATUS_ERROR(status)) {
            tsf_recalculation_lock_release(mlo_dev_ctx);
            qdf_err("Failed to recalculate TSF for vdevs of MLD");
            return QDF_STATUS_E_FAILURE;
        }
    }

    for (vdev_index = 0; vdev_index < mld_partner_vdev_count; vdev_index++) {
        struct ieee80211vap *vap;

        vap = wlan_vdev_get_vap(mld_partner_vdev_list[vdev_index]);
        if (!vap) {
            tsf_recalculation_lock_release(mlo_dev_ctx);
            qdf_err("vap is null");
            return QDF_STATUS_E_NULL_VALUE;
        }

        concurrent_tsf_snapshot_list[vdev_index].tsf =
            vap->iv_tsf_offset_calc_params.last_calculated_tsf_host;
        concurrent_tsf_snapshot_list[vdev_index].valid =
            vap->iv_tsf_offset_calc_params.last_calculated_tsf_host_valid;
    }

    tsf_recalculation_lock_release(mlo_dev_ctx);

    return QDF_STATUS_SUCCESS;
}

void ieee80211_update_beacon_tmpl_for_partner_link(struct wlan_objmgr_vdev *vdev)
{
    uint16_t vdev_count, i;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    struct ieee80211vap *co_mld_vap;
    struct ieee80211com *ic;

    if (!vdev) {
        mlo_err("vdev is null");
        return;
    }

    mlo_ap_get_active_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    if (!vdev_count) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0!");
        return;
    }

    for (i = 0; i < vdev_count; i++) {
        co_mld_vap = wlan_vdev_get_mlme_ext_obj(wlan_vdev_list[i]);
        if ((wlan_vdev_list[i] == vdev) || !co_mld_vap || !co_mld_vap->iv_ic) {
            ieee80211_mlo_release_vdev_ref(wlan_vdev_list[i]);
            continue;
        }
        ic = co_mld_vap->iv_ic;

        /* Do beacon tmplate update for Tx-VAP or lowerband vap */
        if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(co_mld_vap)) {
            co_mld_vap->iv_reinit_beacon = 1;
            ic->ic_vdev_beacon_template_update(co_mld_vap);
        }

        ieee80211_mlo_release_vdev_ref(wlan_vdev_list[i]);
    }
}

bool ieee80211_is_mlo_partner_lower_band_ap(struct wlan_objmgr_vdev *vdev)
{
    uint16_t vdev_count, i;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    struct ieee80211vap *vap;
    struct ieee80211com *ic;
    bool ret = false;

    if (!vdev) {
        mlo_err("vdev is null");
        return ret;
    }
    mlo_ap_get_partner_vdev_list_from_mld(vdev, &vdev_count, wlan_vdev_list);

    for (i = 0; i < vdev_count; i++) {
        /* Once ret becomes true, continue to release vdev refs */
        if (!ret) {
            vap = wlan_vdev_get_mlme_ext_obj(wlan_vdev_list[i]);
            if (!vap)
                goto relese_refs;
            ic = vap->iv_ic;
            if (!ic)
                goto relese_refs;

            /* Check partner link vap is up and running on lower band */
            if (ieee80211_is_vap_state_running(vap) &&
                !IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan))
                ret = true;
        }

relese_refs:
        ieee80211_mlo_release_vdev_ref(wlan_vdev_list[i]);
    }

    return ret;
}

bool ieee80211_is_ubpr_disallowed_on_mlo_vap(struct ieee80211vap *vap)
{
    struct ieee80211com *ic;
    struct ieee80211vap *temp_vap;

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is NULL!");
        return false;
    }

    /* Loop for all vaps in a MBSS group and check it's partner links */
    TAILQ_FOREACH(temp_vap, &ic->ic_vaps, iv_next) {
        if ((temp_vap->iv_mbss.group_id == vap->iv_mbss.group_id) &&
            ieee80211_mlo_is_mld_ap(temp_vap->vdev_obj) &&
            ieee80211_is_mlo_partner_lower_band_ap(temp_vap->vdev_obj))
            return true;
    }

    return false;
}
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef WLAN_FEATURE_11BE
int sched_mode_probe_resp_handler(struct wlan_objmgr_psoc *psoc,
                                  struct wlan_host_sched_mode_probe_resp_event *evt_params)
{
    struct wlan_objmgr_vdev *vdev;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_host_sched_mode_probe_resp_event cfg_evt_params = {0};
    wlan_if_t vap;
    uint8_t hw_link_id;
    int ret = 0;

    if (!evt_params) {
        qdf_err("evt_params is NULL");
        return -EINVAL;
    }

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, evt_params->vdev_id,
                                                WLAN_OBJMGR_ID);

    if (!vdev) {
        qdf_err("vdev is NULL for vdev ID: %d psoc Id %d",
                evt_params->vdev_id, wlan_psoc_get_id(psoc));
        return -EINVAL;
    }

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap) {
        qdf_err("vap is NULL for vdev ID: %d psoc ID: %d",
                evt_params->vdev_id, wlan_psoc_get_id(psoc));
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OBJMGR_ID);
        return -EINVAL;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        qdf_err("pdev is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OBJMGR_ID);
        return -EINVAL;
    }

    hw_link_id = ((wlan_get_mlo_grp_id_from_pdev(pdev) << 8) | wlan_mlo_get_pdev_hw_link_id(pdev));
    cfg_evt_params.vdev_id = evt_params->vdev_id;
    cfg_evt_params.tput_mbps_on = evt_params->tput_mbps_on;
    cfg_evt_params.tput_mbps_off = evt_params->tput_mbps_off;
    cfg_evt_params.hw_link_id = hw_link_id;

    qdf_debug("Vdev ID: %d, Tput ON: %d, Tput Off: %d, PDEV ID: %d",
             evt_params->vdev_id, evt_params->tput_mbps_on,
             evt_params->tput_mbps_off, hw_link_id);

    ret = wlan_cfg80211_sched_mode_probe_resp_handler(vap, &cfg_evt_params);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OBJMGR_ID);

    return ret;
}

QDF_STATUS deter_sched_mode_probe_resp_rx_ops(struct wlan_lmac_if_rx_ops *rx_ops)
{
    if (!rx_ops) {
        qdf_err("rx_ops is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    rx_ops->sched_mode_rx_ops.sched_mode_probe_resp_handler = sched_mode_probe_resp_handler;

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(deter_sched_mode_probe_resp_rx_ops);
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE_MLO
/* Store vdev id in ascending order of vdev ids in ic->cu_vdev_ids array.
 * The cu_vdev_ids array and cu_vdev_map from FW is one to one mapping.
 * This cu_vdev_ids array is used to get the vdev based on the
 * cu_vdev_map bitmap from FW.  Set the CU flag in capability info
 * of the probe response Corresponding VDEVs.
 * Update the respective VDEVs BPCC values in RNR and BMLE element
 * in probe and assoc response.
 */
QDF_STATUS ieee80211_add_vdev_ids(struct ieee80211com *ic, uint8_t vdev_id)
{
    int i, pos = 0;

    spin_lock(&ic->ic_cu_vdev_map_lock);
    for (i = 0; i < MAX_AP_MLDS_PER_LINK; i++) {
        if (vdev_id < ic->cu_vdev_ids[i]) {
            pos = i;
            break;
        }
        if (vdev_id > ic->cu_vdev_ids[MAX_AP_MLDS_PER_LINK - 1]) {
            spin_unlock(&ic->ic_cu_vdev_map_lock);
            qdf_err("Invalid: pos:%d for vdev_id:%d", i, vdev_id);
            return QDF_STATUS_E_FAILURE;
        }
    }
    if (i == MAX_AP_MLDS_PER_LINK) {
        spin_unlock(&ic->ic_cu_vdev_map_lock);
        qdf_err("VDEV count reached MAX_AP_MLDS_PER_LINK vdev_id:%d", vdev_id);
        return QDF_STATUS_E_FAILURE;
    }

    for(i =  MAX_AP_MLDS_PER_LINK - 1; i > pos; i--) {
        ic->cu_vdev_ids[i] = ic->cu_vdev_ids[i-1];
    }
    ic->cu_vdev_ids[pos] = vdev_id;
    spin_unlock(&ic->ic_cu_vdev_map_lock);

    qdf_info("vdev_id:%d pos:%d", vdev_id, i);

    return QDF_STATUS_SUCCESS;
}

/*
 * Function to get the vdev position of an AP MLD in the cu_vdev_ids.
 */
uint8_t cu_get_mlo_vdev_pos(struct ieee80211com *ic, uint8_t vdev_id) {
    uint8_t i;

    for (i=0; i < MAX_AP_MLDS_PER_LINK; i++) {
        if (ic->cu_vdev_ids[i] == vdev_id) {
            return i;
        }
    }
    qdf_err("vdev_id :%d not part of cu_vdev_ids", vdev_id);

    return 0;
}

/*
 * Function to get the BPCC value of a specific AP MLD
 * in corresponding MLO link from vdev_bpcc.
 */
uint8_t cu_get_mlo_vdev_bpcc(struct ieee80211com *ic,
                             uint8_t hw_link_id, uint8_t vdev_id_pos,
                             struct ieee80211_rx_status *rs)
{
    uint8_t pos = (hw_link_id * MAX_AP_MLDS_PER_LINK) + vdev_id_pos;

    if (rs)
        return (rs->rs_cu.vdev_bpcc[pos]);

    return (ic->ic_cu.vdev_bpcc[pos]);
}

/*
 * Function to get the BPCC value of an AP MLD.
 */
uint8_t iee80211_cu_get_bssparamchangecnt(struct ieee80211vap *vap,
        struct ieee80211_rx_status *rs)
{

    uint8_t hw_link_id, vdev_pos, bssparamchangecnt = 0;

    if (vap) {
        hw_link_id = wlan_mlo_get_pdev_hw_link_id(vap->iv_ic->ic_pdev_obj);
        vdev_pos = cu_get_mlo_vdev_pos(vap->iv_ic, vap->iv_unit);
        bssparamchangecnt = cu_get_mlo_vdev_bpcc(vap->iv_ic, hw_link_id, vdev_pos, rs);

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_CU,
                "%s:BPCC value for vdev_id:%d hw_link_id:%d"
                "vdev_pos:%d is bssparamchangecnt:%d",
                __func__, vap->iv_unit, hw_link_id,
                vdev_pos, bssparamchangecnt);
    }

    return bssparamchangecnt;
}

/* Set the Nontransmitted BSSIDs Critical Update Flag subfield
 * of the Capability Information field CUF2 to 1 if ongoing
 * cu_vdev_map from FW is set for AP MLD of non Tx VDEV.
 *
 * This Function return true if any non Tx VDEV AP MLD
 * cu_vdev_map is set.
 * Based on the return value Tx VDEV set the CUF2 flag in
 * Capability informatin.
 */
bool cu_vdev_map_is_set_non_txvdev(struct ieee80211vap *vap,
        uint16_t cu_vdev_map)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_vdev *tmp_vdev;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;
    uint8_t vap_cnt;

    mbss_ctx = ic->ic_mbss_ctx;
    ic_mbss  = &mbss_ctx->ic_mbss[vap->iv_mbss.group_id];

    for (vap_cnt = 0; vap_cnt < ic->ic_mbss_ctx->grp_sz; vap_cnt++) {
        if ((ic_mbss->vdev_id_gp[vap_cnt] == IEEE80211_MBSSID_INVALID_VDEV_ID) ||
                (ic_mbss->vdev_id_gp[vap_cnt] == vap->iv_unit)) {
            continue;
        }

        tmp_vdev = wlan_objmgr_get_vdev_by_id_from_pdev(
                ic->ic_pdev_obj, ic_mbss->vdev_id_gp[vap_cnt],
                WLAN_MLME_NB_ID);
        if (!tmp_vdev) {
            continue;
        }
        if (wlan_vdev_mlme_is_mlo_ap(tmp_vdev) &&
                ((cu_vdev_map >>
                 cu_get_mlo_vdev_pos(vap->iv_ic, ic_mbss->vdev_id_gp[vap_cnt])) & 0x1)) {
            /* Release vdev reference */
            wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
            return true;
        }
        /* Release vdev reference */
        wlan_objmgr_vdev_release_ref(tmp_vdev, WLAN_MLME_NB_ID);
    }
    return false;
}

/**
 * mlo_get_link_removal_event_type() - Get the type of the link removal event
 * @configured_tbtt: AP removal timer TBTT configured by the user-space
 * @received_tbtt: AP removal timer TBTT received in the event
 *
 * Return: Type of the link removal event
 */
static enum mlo_link_removal_event_type
mlo_get_link_removal_event_type(uint16_t configured_tbtt, uint16_t received_tbtt)
{
    if (received_tbtt == configured_tbtt)
        return MLO_LINK_REMOVAL_EVENT_TYPE_TBTT_START;
    else if (received_tbtt == 0)
        return MLO_LINK_REMOVAL_EVENT_TYPE_TBTT_COMPLETE;
    else
        return MLO_LINK_REMOVAL_EVENT_TYPE_INVALID;
}

/**
 * mlo_validate_link_removal_event() - Validate the incoming link removal event
 * @vdev: VDEV corresponding to the link under going removal
 * @evt_params: parameters corresponding to this event
 *
 * Return: True incase of successful validation, else False
 */
static bool mlo_validate_link_removal_event(
                           struct wlan_objmgr_vdev *vdev,
                           struct mlo_link_removal_evt_params *evt_params)
{
    struct mlo_link_removal_ctx *removal_ctx;
    enum mlo_link_removal_event_type cur_event;
    wlan_if_t vap;

    if (!vdev || !evt_params) {
        mlo_err("Null argument(s)");
        return false;
    }

    if (!wlan_vdev_mlme_is_mlo_ap(vdev)) {
        mlo_err("vdev is not an MLO AP");
        return false;
    }

    if (!wlan_vdev_mlme_op_flags_get(vdev,
               WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS)) {
        mlo_err("Received link removal event when link removal is not in progress");
        return false;
    }

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap) {
        mlo_err("VAP is null");
        return false;
    }

    removal_ctx = &vap->link_reconfig_ctx.removal_ctx;
    cur_event = mlo_get_link_removal_event_type(
                                  removal_ctx->tbtt_count,
                                  evt_params->tbtt_info.tbtt_count);

    if (cur_event != removal_ctx->expected_event_type) {
        mlo_err("Received unexpected event type: %d expected: %d",
                cur_event, removal_ctx->expected_event_type);
        mlo_err("Configured TBTT %d recevied TBTT: %d",
                removal_ctx->tbtt_count, evt_params->tbtt_info.tbtt_count);
        return false;
    }

    return true;
}

static QDF_STATUS
mlo_link_removal_switch_primary_mcast_vdev(struct wlan_objmgr_vdev *vdev)
{
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    uint16_t vdev_count;
    struct wlan_objmgr_vdev *partner_vdev;
    int vdev_idx = 0;
    wlan_if_t partner_vap;
    QDF_STATUS status, ref_status;

    if (!vdev) {
        mlo_err("null vdev");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* If this is not a primay mcast vdev, then no action on link removal */
    if (!wlan_vdev_mlme_is_mlo_mcast_vdev(vdev))
        return QDF_STATUS_SUCCESS;

    mlo_ap_get_active_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu,"
                "investigate the possible corruption",
                vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
        status = QDF_STATUS_E_FAILURE;
        goto release_ref;
    }

    if (vdev_count <= 1) {
        mlo_err("Link removal can only be used on MLD with atleast 2 active AP"
                "VAPs. Investigate the possible corruption");
        status = QDF_STATUS_E_FAILURE;
        goto release_ref;
    }

    /* Assign the first active partner link as primary mcast vdev */
    for (; vdev_idx < vdev_count; ++vdev_idx) {
        partner_vdev = wlan_vdev_list[vdev_idx];

        if (partner_vdev == vdev)
            continue;

        partner_vap = wlan_vdev_mlme_get_ext_hdl(partner_vdev);
        if (!partner_vap) {
            mlo_err("partner vap corresponding to vdev:%pK is null",
                    partner_vdev);
            status = QDF_STATUS_E_FAILURE;
            goto release_ref;
        }

        mlo_info("Switching primary multicast vdev to %s",
                 partner_vap->iv_netdev_name);
        wlan_mlme_mlo_set_mcast_vdev(partner_vdev, true);
        break;
    }

    status = QDF_STATUS_SUCCESS;

release_ref:
    if (vdev_count) {
        ref_status = ieee80211_mlo_release_vdev_list_refs(
                           wlan_vdev_list,
                           QDF_ARRAY_SIZE(wlan_vdev_list),
                           vdev_count, 0);
        if (QDF_IS_STATUS_ERROR(ref_status)) {
            mlo_err("Couldn't release refs of ML vdevs");
            return ref_status;
        }
    }

    return status;
}

static QDF_STATUS
mlo_link_removal_dp_update_partner_list(struct wlan_objmgr_vdev *vdev)
{
    wlan_if_t vap;
    struct wlan_objmgr_psoc *psoc;
    struct qdf_mac_addr *mld_addr;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap) {
        mlo_err("VAP is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        mlo_err("null vdev");
        return QDF_STATUS_E_NULL_VALUE;
    }

    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);

    if (cdp_mlo_dev_ctxt_detach(wlan_psoc_get_dp_handle(psoc),
                                wlan_vdev_get_id(vdev),
                                (uint8_t *)mld_addr)
                                != QDF_STATUS_SUCCESS) {
        mlo_err("Failed to detach DP vap %s from DP MLO Dev ctxt",
                vap->iv_netdev_name);
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

/**
 * mlo_update_link_removal_event_sm() - Update the link removal state machine
 * @vdev: VDEV corresponding to the link under going removal
 * @evt_params: parameters corresponding to the current link removal event
 *
 * Return: None
 */
static void mlo_update_link_removal_event_sm(
                           struct wlan_objmgr_vdev *vdev,
                           struct mlo_link_removal_evt_params *evt_params)
{
    struct mlo_link_removal_ctx *removal_ctx;
    enum mlo_link_removal_event_type cur_event;
    wlan_if_t vap;

    if (!vdev || !evt_params) {
        mlo_err("Null argument(s)");
        return;
    }

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap) {
        mlo_err("VAP is null");
        return;
    }

    removal_ctx = &vap->link_reconfig_ctx.removal_ctx;
    cur_event = mlo_get_link_removal_event_type(
                                  removal_ctx->tbtt_count,
                                  evt_params->tbtt_info.tbtt_count);

    switch (cur_event) {
    case MLO_LINK_REMOVAL_EVENT_TYPE_TBTT_START:
        removal_ctx->expected_event_type =
                             MLO_LINK_REMOVAL_EVENT_TYPE_TBTT_COMPLETE;

        mlo_link_removal_switch_primary_mcast_vdev(vdev);
        break;

    case MLO_LINK_REMOVAL_EVENT_TYPE_TBTT_COMPLETE:
        removal_ctx->expected_event_type =
                             MLO_LINK_REMOVAL_EVENT_TYPE_NONE;

        /* Set op flag to be used by MLME to block assocations on this link */
        wlan_vdev_mlme_op_flags_set(vdev, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE);

        /* Update vdev list in DP MLO context */
        mlo_link_removal_dp_update_partner_list(vdev);
        break;

    default:
        mlo_err("Unexpected event: %d, resetting the state machine", cur_event);
        removal_ctx->expected_event_type =
                             MLO_LINK_REMOVAL_EVENT_TYPE_NONE;
        break;
    }
}

#if defined(WLAN_FEATURE_11BE_MLO_3_LINK_TX) && \
    defined(CONFIG_SAWF)
static inline uint32_t
mlo_get_link_maxphyrate_by_hw_link_id(struct wlan_mlo_peer_context *ml_peer,
                                      uint16_t hw_link_id)
{
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_peer *link_peer = NULL;
    struct ieee80211_node *ni = NULL;
    uint32_t maxphyrate = 0;
    int i = 0;

    if (!ml_peer) {
        qdf_err("invalid mlpeer");
        return 0;
    }

    if (hw_link_id == INVALID_HW_LINK_ID) {
        qdf_err("invalid hw link id is passed %p", ml_peer);
        return 0;
    }

    mlo_peer_lock_acquire(ml_peer);
    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry)
            continue;

        link_peer = peer_entry->link_peer;
        if (!link_peer)
            continue;

        ni = wlan_peer_get_mlme_ext_obj(link_peer);
        if (!ni)
            continue;

        if (hw_link_id != wlan_mlo_get_pdev_hw_link_id(ni->ni_ic->ic_pdev_obj))
            continue;

        maxphyrate = ni->ni_ic->ic_get_maxphyrate(ni->ni_ic, ni);
        break;
    }
    mlo_peer_lock_release(ml_peer);

    return maxphyrate;
}

#define GET_3_LINK_TX_HW_LINK_ID(bitmap) (bitmap >> 1)

/**
 * mlo_3_link_tlt_selection_event_handler() - Main handler for 3 link MLO tlt event
 * @psoc: PSOC on which the event is received
 * @evt_params: parameters corresponding to this event
 *
 * Return: QDF_STATUS of operation
 */
static QDF_STATUS mlo_3_link_tlt_selection_event_handler(
                           struct wlan_objmgr_psoc *psoc,
                           struct mlo_tlt_selection_evt_params *evt_params)
{
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_mlo_dev_context *mld_dev = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    uint8_t tid_weight[CDP_DATA_TID_MAX] = {0};
    uint8_t primary_tid_weight = 0;
    uint8_t secondary_tid_weight = 0;
    uint32_t primary_tid_capacity = 0;
    uint32_t secondary_tid_capacity = 0;
    uint64_t total_capacity = 0;
    int i = 0;
    uint8_t tid_bitmap = 0;
    uint8_t common_link_tid_bitmap = 0;
    uint32_t common_link_capacity = 0;

    /* check whether link bitmap is valid or not */
    if ((evt_params->link_bmap[0] == 0) || (evt_params->link_bmap[1] == 0)) {
        qdf_err("invalid link bitmap received "
                "primary tid bitmap = %d secondary tid bitmap = %d",
                evt_params->link_bmap[0], evt_params->link_bmap[1]);
        return QDF_STATUS_E_INVAL;
    }

    ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr((struct qdf_mac_addr *)&evt_params->mld_addr, &mld_dev);
    if (!ml_peer || !mld_dev) {
        qdf_err("invalid ml_peer or mldev");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* non 3 link association */
    if (ml_peer->link_peer_cnt != 3)
        return QDF_STATUS_E_INVAL;

    /* find the common link */
    common_link_tid_bitmap = evt_params->link_bmap[0] & evt_params->link_bmap[1];
    if (common_link_tid_bitmap) {
        common_link_capacity = mlo_get_link_maxphyrate_by_hw_link_id(
                                   ml_peer,
                                   GET_3_LINK_TX_HW_LINK_ID(common_link_tid_bitmap));
        /* distribute the common link capacity */
        if (common_link_capacity) {
            primary_tid_capacity = common_link_capacity / 2;
            secondary_tid_capacity = common_link_capacity / 2;
        }
    }

    /* calulate primary tid capacity */
    for (i = 0; i < 8; i++) {
        /* skip the common link capacity addition */
        tid_bitmap = ((evt_params->link_bmap[0])  & (1 << i));
        if (tid_bitmap && (tid_bitmap != common_link_tid_bitmap)) {
            primary_tid_capacity += mlo_get_link_maxphyrate_by_hw_link_id(
                                        ml_peer,
                                        GET_3_LINK_TX_HW_LINK_ID(tid_bitmap));
        }
    }

    /* calulate secondary tid capacity */
    for (i = 0; i < 8; i++) {
        /* skip the common link capacity addition */
        tid_bitmap = ((evt_params->link_bmap[1])  & (1 << i));
        if (tid_bitmap && (tid_bitmap != common_link_tid_bitmap)) {
            secondary_tid_capacity += mlo_get_link_maxphyrate_by_hw_link_id(
                                        ml_peer,
                                        GET_3_LINK_TX_HW_LINK_ID(tid_bitmap));
        }
    }

    if (!primary_tid_capacity || !secondary_tid_capacity) {
        qdf_err("Invalid link phy rate peer mld mac address "QDF_MAC_ADDR_FMT
                " link_priority = %d:%d:%d primary tid bitmap = %d"
                " secondary tid bitmap = %d common link bit map = %d",
                QDF_MAC_ADDR_REF(evt_params->mld_addr.bytes),
                evt_params->link_priority[0], evt_params->link_priority[1],
                evt_params->link_priority[2], evt_params->link_bmap[0],
                evt_params->link_bmap[1], common_link_tid_bitmap);
        return QDF_STATUS_E_INVAL;
    }

    total_capacity = primary_tid_capacity + secondary_tid_capacity;

    /* percentage calculation */
    primary_tid_weight = div64_u64((uint64_t)(primary_tid_capacity * 100),
                                   total_capacity);
    secondary_tid_weight = div64_u64((uint64_t)(secondary_tid_capacity * 100),
                                   total_capacity);

    mlo_debug("peer mld mac address "QDF_MAC_ADDR_FMT "link_priority = %d:%d:%d"
              " primary_link_capacity = %d secondary_link_capacity = %d"
              " primary_tid_weight = %d secondary_tid_weight = %d"
              " primary tid bitmap = %d secondary tid bitmap = %d"
              " common link capacity = %d common link bit map = %d",
              QDF_MAC_ADDR_REF(evt_params->mld_addr.bytes),
              evt_params->link_priority[0], evt_params->link_priority[1],
              evt_params->link_priority[2], primary_tid_capacity,
              secondary_tid_capacity, primary_tid_weight, secondary_tid_weight,
              evt_params->link_bmap[0], evt_params->link_bmap[1],
              common_link_capacity, common_link_tid_bitmap);

    /* best effort */
    tid_weight[0] = primary_tid_weight;
    tid_weight[3] = secondary_tid_weight;

    /* background */
    tid_weight[1] = primary_tid_weight;
    tid_weight[2] = secondary_tid_weight;

    /* video */
    tid_weight[4] = primary_tid_weight;
    tid_weight[5] = secondary_tid_weight;

    /* voice */
    tid_weight[6] = primary_tid_weight;
    tid_weight[7] = secondary_tid_weight;

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_txrx_handle) {
        qdf_err(" soc_txrx_hander is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    return cdp_sawf_3_link_peer_set_tid_weight(soc_txrx_handle,
                                               evt_params->mld_addr.bytes,
                                               CDP_INVALID_PEER,
                                               tid_weight);
}

QDF_STATUS mlo_mgr_register_rx_ops_3_link_tlt_selection(
                                          struct wlan_lmac_if_rx_ops *rx_ops)
{
    if (!rx_ops) {
        qdf_err("rx_ops is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    rx_ops->mlo_rx_ops.mlo_3_link_tlt_selection_handler =
                                  mlo_3_link_tlt_selection_event_handler;

    return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(mlo_mgr_register_rx_ops_3_link_tlt_selection);
#endif

/**
 * mlo_link_remove_event_handler() - Main handler for MLO link removal event
 * @psoc: PSOC on which the event is received
 * @evt_params: parameters corresponding to this event
 *
 * Return: QDF_STATUS of operation
 */
static QDF_STATUS mlo_link_remove_event_handler(
                           struct wlan_objmgr_psoc *psoc,
                           struct mlo_link_removal_evt_params *evt_params)
{
    struct wlan_objmgr_vdev *vdev;
    QDF_STATUS status;
    struct cfg80211_mlo_link_removal_evt_params cfg_evt_params;
    wlan_if_t vap;
    struct wlan_objmgr_pdev *pdev;
    struct qdf_mac_addr *mld_addr = NULL;

    if (!evt_params) {
        mlo_err("evt_params is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, evt_params->vdev_id,
                                                WLAN_MLO_MGR_ID);
    if (!vdev) {
        mlo_err("vdev is NULL for vdev ID: %d psoc Id %d",
                evt_params->vdev_id, wlan_psoc_get_id(psoc));
        return QDF_STATUS_E_NULL_VALUE;
    }

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap) {
        mlo_err("vap is NULL for vdev ID: %d psoc ID: %d",
                evt_params->vdev_id, wlan_psoc_get_id(psoc));
        status = QDF_STATUS_E_FAILURE;
        goto release_ref;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        mlo_err("pdev is NULL");
        status = QDF_STATUS_E_NULL_VALUE;
        goto release_ref;
    }

    if (!mlo_validate_link_removal_event(vdev, evt_params)) {
        mlo_err("Link removal event validation failed for vap %s",
                vap->iv_netdev_name);
        status = QDF_STATUS_E_FAILURE;
        goto release_ref;
    }

    mlo_info("Received link removal event for vap %s with "
             "TBTT: %u tsf: %llu Q-Timer reading: %llu",
             vap->iv_netdev_name,
             evt_params->tbtt_info.tbtt_count,
             evt_params->tbtt_info.tsf,
             evt_params->tbtt_info.qtimer_reading);

    mlo_update_link_removal_event_sm(vdev, evt_params);

    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);

    cfg_evt_params.tbtt_count = evt_params->tbtt_info.tbtt_count;
    cfg_evt_params.qtimer_reading = evt_params->tbtt_info.qtimer_reading;
    cfg_evt_params.tsf = evt_params->tbtt_info.tsf;
    cfg_evt_params.beacon_interval = wlan_get_param(vap, IEEE80211_BEACON_INTVAL);
    cfg_evt_params.hw_link_id = ((wlan_get_mlo_grp_id_from_pdev(pdev) << 8) | wlan_mlo_get_pdev_hw_link_id(pdev));
    qdf_mem_copy(cfg_evt_params.mld_addr, mld_addr, QDF_MAC_ADDR_SIZE);

    status = wlan_cfg80211_mlo_link_removal_evt_handler(vap, &cfg_evt_params);
    if (QDF_IS_STATUS_ERROR(status))
        mlo_err("Couldn't inform link removal event for vap %s",
                vap->iv_netdev_name);

release_ref:
    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLO_MGR_ID);

    return status;
}

QDF_STATUS mlo_mgr_register_rx_ops_legacy(struct wlan_lmac_if_rx_ops *rx_ops)
{
    if (!rx_ops) {
        qdf_err("rx_ops is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    rx_ops->mlo_rx_ops.mlo_link_removal_handler = mlo_link_remove_event_handler;

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(mlo_mgr_register_rx_ops_legacy);

void is_mlo_link_removal_in_progress_vdev_iter(struct wlan_objmgr_pdev *pdev,
                                               void *object,
                                               void *arg)
{
    struct wlan_objmgr_vdev *vdev = object;
    bool *is_in_progress = arg;

    if (wlan_vdev_mlme_op_flags_get(vdev,
               WLAN_VDEV_OP_MLO_LINK_REMOVAL_IN_PROGRESS))
        *is_in_progress = true;
}

QDF_STATUS is_mlo_link_removal_in_progress_on_radio(
    struct ieee80211com *ic,
    bool *is_in_progress)
{
    if (!ic) {
        mlo_err("ic is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    *is_in_progress = false;
    if (QDF_IS_STATUS_ERROR(wlan_objmgr_pdev_try_get_ref(ic->ic_pdev_obj,
                                                         WLAN_MLO_MGR_ID))) {
        mlo_err("Unable to get pdev reference");
        return QDF_STATUS_E_FAILURE;
    }

    wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                                      is_mlo_link_removal_in_progress_vdev_iter,
                                      is_in_progress, 0, WLAN_MLO_MGR_ID);
    wlan_objmgr_pdev_release_ref(ic->ic_pdev_obj, WLAN_MLO_MGR_ID);

    return QDF_STATUS_SUCCESS;
}

void ieee80211_mlo_ap_get_beaconing_vdev_list(struct wlan_objmgr_vdev *vdev,
                                              uint16_t *bc_vdev_count,
                                              struct wlan_objmgr_vdev **wlan_bc_vdev_list)
{
    struct wlan_objmgr_vdev *mld_vdev;
    struct ieee80211vap *mld_vap;
    struct ieee80211com *ic;
    struct ieee80211vap *transmit_vap;
    uint8_t grp_id, i;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS];
    uint16_t vdev_count = 0;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss;

    mlo_ap_get_active_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    if (!vdev_count) {
        mloie_err("Number of VDEVs under AP-MLD is reported as 0");
        *bc_vdev_count = vdev_count;
        return;
    }

    /* Note: Once API changes to mlo_ap_get_vdev_list() are available to handle
     * array size requirements, remove this condition.
     */
    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
        mloie_err("PSOC %d, VDEV %d Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                  wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev),
                  vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
        goto retnolinfo_release_refs;
    }

    for(i = 0; i < vdev_count; i++) {
        mld_vdev = wlan_vdev_list[i];
        if (mld_vdev == NULL) {
            mloie_err("PSOC %d, VDEV %d:  VDEV in MLD VDEV list is NULL",
                      wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
            goto retnolinfo_release_refs;
        }

        mld_vap = wlan_vdev_get_vap(mld_vdev);
        if (mld_vap == NULL) {
            mloie_err("PSOC %d, VDEV %d: Co-MLD VAP is NULL",
                      wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
            goto retnolinfo_release_refs;
        }

        ic = mld_vap->iv_ic;
        if (ic == NULL) {
            mloie_err("PSOC %d, VDEV %d: Co-MLD IC is NULL",
                      wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
            goto retnolinfo_release_refs;
        }
        if (!ic->is_mbssid_enabled) {
           continue;
        }

        if (!ic->ic_mbss_ctx) {
            mloie_err("PSOC %d, VDEV %d: ic_mbss_ctx is NULL",
                      wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
            goto retnolinfo_release_refs;
        }
        mbss_ctx = ic->ic_mbss_ctx;
        grp_id = mld_vap->iv_mbss.group_id;
        ic_mbss  = &mbss_ctx->ic_mbss[grp_id];

        transmit_vap = ic_mbss->transmit_vap;
        if (!transmit_vap) {
            mbss_warn("Transmitting VAP can't be NULL!");
            mloie_err("PSOC %d, VDEV %d: grp_id %d, ic_mbss pdev id %d Transmitting VAP can't be NULL",
                      wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev),
                      grp_id, wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj));
            goto retnolinfo_release_refs;
        }
        if (wlan_vdev_is_up(transmit_vap->vdev_obj) != QDF_STATUS_SUCCESS) {
            wlan_vdev_list[i] = NULL;
            mlo_release_vdev_ref(mld_vap->vdev_obj);
            qdf_rl_debug("transmit vap is not up");
        }
    }
    for(i = 0; i < vdev_count; i++) {
        mld_vdev = wlan_vdev_list[i];
        if (mld_vdev != NULL) {
            wlan_bc_vdev_list[*bc_vdev_count] = mld_vdev;
            (*bc_vdev_count) += 1;
        }
    }

    return;
retnolinfo_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    0)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger
         * issues.
         */
        mloie_err("PSOC %d, VDEV %d: Error while trying to release VDEV references - investigate.",
                  wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
        return;
    }
    *bc_vdev_count = 0;
}
#endif

int
module_init_wlan(void)
{
    return 0;
}

void
module_exit_wlan(void)
{
}
