/*
 * Copyright (c) 2011-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 */

#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include <ieee80211_api.h>
#include <ieee80211_rateset.h>
#include <dfs_postnol_ucfg.h>
#if UNIFIED_SMARTANTENNA
#include <wlan_sa_api_utils_api.h>
#endif
#include <qdf_lock.h>
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <ol_if_athvar.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_lmac_if_api.h>
#include <osif_private.h>
#include <wlan_reg_services_api.h>
#include <ieee80211_regdmn_dispatcher.h>
#include <wlan_mlme_dispatcher.h>
#include <wlan_vdev_mlme_api.h>
#include <wlan_utility.h>
#include <ieee80211_objmgr_priv.h>
#include <wlan_reg_channel_api.h>
#ifdef CONFIG_HOST_FIND_CHAN
#include <ieee80211_regdmn.h>
#endif
#include <ieee80211_acs.h>
#if ATH_ACS_DEBUG_SUPPORT
#include "acs_debug.h"
#endif
#include <wlan_repeater_api.h>
#include "wlan_mbss.h"
#include <wlan_mlme_if.h>
#include <cfg_ucfg_api.h>
#include <wlan_reg_ucfg_api.h>
#ifdef CONFIG_AFC_SUPPORT
#include <ieee80211_cfg80211.h>
#endif
#include <reg_services_common.h>

#define BW_40 40
#define BW_20 20
int ieee80211_ucfg_set_chanswitch(wlan_if_t vaphandle, struct chan_params *chan_p, u_int8_t tbtt, u_int16_t ch_width);
bool ieee80211_is_mode_lower(wlan_if_t vap, enum ieee80211_phymode mode);
bool ieee80211_is_mode_higher(wlan_if_t vap, enum ieee80211_phymode mode);
#ifdef WLAN_FEATURE_11BE
enum ieee80211_phymode ieee80211_get_derived_mode(wlan_if_t vap, enum ieee80211_phymode mode, enum ieee80211_mode change_mode, struct chan_params *chan_p);
#endif
enum ieee80211_mode ieee80211_mixed_modes_config(wlan_if_t vap, enum ieee80211_phymode mode);
static struct ieee80211_ath_channel
ieee80211_find_dot11_channel_internal(struct ieee80211com *ic,
                                      uint16_t freq,
                                      uint16_t freq2,
                                      enum ieee80211_phymode mode,
                                      enum supported_6g_pwr_types in_6g_pwr_mode,
                                      uint16_t input_puncture_bitmap);

static struct ieee80211_ath_channel
ieee80211_find_channel_internal(struct ieee80211com *ic,
                                int freq, uint16_t des_cfreq2,
                                u_int64_t flags,
                                enum supported_6g_pwr_types in_6g_pwr_mode,
                                uint16_t input_puncture_bitmap);

static int ieee80211_get_chan_width_from_phy_chwidth(enum phy_ch_width ch_width);
#ifdef WLAN_FEATURE_11BE
static enum ieee80211_phymode
ieee80211_get_11bea_phymode(uint64_t chan_flag);
#endif

const char *ieee80211_phymode_name[] = {
    "auto",             /* IEEE80211_MODE_AUTO */
    "11a",              /* IEEE80211_MODE_11A */
    "11b",              /* IEEE80211_MODE_11B */
    "11g",              /* IEEE80211_MODE_11G */
    "FH",               /* IEEE80211_MODE_FH */
    "turboA",           /* IEEE80211_MODE_TURBO_A */
    "turboG",           /* IEEE80211_MODE_TURBO_G */
    "11naht20",         /* IEEE80211_MODE_11NA_HT20 */
    "11nght20",         /* IEEE80211_MODE_11NG_HT20 */
    "11naht40plus",     /* IEEE80211_MODE_11NA_HT40PLUS */
    "11naht40minus",    /* IEEE80211_MODE_11NA_HT40MINUS */
    "11nght40plus",     /* IEEE80211_MODE_11NG_HT40PLUS */
    "11nght40minus",    /* IEEE80211_MODE_11NG_HT40MINUS */
    "11nght40",         /* IEEE80211_MODE_11NG_HT40 */
    "11naht40",         /* IEEE80211_MODE_11NA_HT40 */
    "11acvht20",        /* IEEE80211_MODE_11AC_VHT20 */
    "11acvht40plus",    /* IEEE80211_MODE_11AC_VHT40PLUS */
    "11acvht40minus",   /* IEEE80211_MODE_11AC_VHT40MINUS */
    "11acvht40",        /* IEEE80211_MODE_11AC_VHT40 */
    "11acvht80",        /* IEEE80211_MODE_11AC_VHT80 */
    "11acvht160",       /* IEEE80211_MODE_11AC_VHT160 */
    "11acvht80_80",     /* IEEE80211_MODE_11AC_VHT80_80 */
    "11axahe20",        /* IEEE80211_MODE_11AXA_HE20 */
    "11axghe20",        /* IEEE80211_MODE_11AXG_HE20 */
    "11axahe40plus",    /* IEEE80211_MODE_11AXA_HE40PLUS */
    "11axahe40minus",   /* IEEE80211_MODE_11AXA_HE40MINUS */
    "11axghe40plus",    /* IEEE80211_MODE_11AXG_HE40PLUS */
    "11axghe40minus",   /* IEEE80211_MODE_11AXG_HE40MINUS */
    "11axahe40",        /* IEEE80211_MODE_11AXA_HE40 */
    "11axghe40",        /* IEEE80211_MODE_11AXG_HE40 */
    "11axahe80",        /* IEEE80211_MODE_11AXA_HE80 */
    "11axahe160",       /* IEEE80211_MODE_11AXA_HE160 */
    "11axahe80_80",     /* IEEE80211_MODE_11AXA_HE80_80 */
#ifdef WLAN_FEATURE_11BE
    "11beaeht20",        /* IEEE80211_MODE_11BEA_EHT20 */
    "11begeht20",        /* IEEE80211_MODE_11BEG_EHT20 */
    "11beaeht40plus",    /* IEEE80211_MODE_11BEA_EHT40PLUS */
    "11beaeht40minus",   /* IEEE80211_MODE_11BEA_EHT40MINUS */
    "11begeht40plus",    /* IEEE80211_MODE_11BEG_EHT40PLUS */
    "11begeht40minus",   /* IEEE80211_MODE_11BEG_EHT40MINUS */
    "11beaeht40",        /* IEEE80211_MODE_11BEA_EHT40 */
    "11begeht40",        /* IEEE80211_MODE_11BEG_EHT40 */
    "11beaeht80",        /* IEEE80211_MODE_11BEA_EHT80 */
    "11beaeht160",       /* IEEE80211_MODE_11BEA_EHT160 */
    "11beaeht320",       /* IEEE80211_MODE_11BEA_EHT320 */
#endif /* WLAN_FEATURE_11BE */
};

/*
 * Return the phy mode for with the specified channel.
 */
enum ieee80211_phymode
ieee80211_chan2mode(const struct ieee80211_ath_channel *chan)
{
    if (!chan) {
        return IEEE80211_MODE_AUTO;
    }
    if (IEEE80211_IS_CHAN_108G(chan))
        return IEEE80211_MODE_TURBO_G;
    else if (IEEE80211_IS_CHAN_TURBO(chan))
        return IEEE80211_MODE_TURBO_A;
    else if (IEEE80211_IS_CHAN_A(chan))
        return IEEE80211_MODE_11A;
    else if (IEEE80211_IS_CHAN_ANYG(chan))
        return IEEE80211_MODE_11G;
    else if (IEEE80211_IS_CHAN_B(chan))
        return IEEE80211_MODE_11B;
    else if (IEEE80211_IS_CHAN_FHSS(chan))
        return IEEE80211_MODE_FH;
    else if (IEEE80211_IS_CHAN_11NA_HT20(chan))
        return IEEE80211_MODE_11NA_HT20;
    else if (IEEE80211_IS_CHAN_11NG_HT20(chan))
        return IEEE80211_MODE_11NG_HT20;
    else if (IEEE80211_IS_CHAN_11NA_HT40PLUS(chan))
        return IEEE80211_MODE_11NA_HT40PLUS;
    else if (IEEE80211_IS_CHAN_11NA_HT40MINUS(chan))
        return IEEE80211_MODE_11NA_HT40MINUS;
    else if (IEEE80211_IS_CHAN_11NG_HT40PLUS(chan))
        return IEEE80211_MODE_11NG_HT40PLUS;
    else if (IEEE80211_IS_CHAN_11NG_HT40MINUS(chan))
        return IEEE80211_MODE_11NG_HT40MINUS;
    else if (IEEE80211_IS_CHAN_11AC_VHT20(chan))
        return IEEE80211_MODE_11AC_VHT20;
    else if (IEEE80211_IS_CHAN_11AC_VHT40PLUS(chan))
        return IEEE80211_MODE_11AC_VHT40PLUS;
    else if (IEEE80211_IS_CHAN_11AC_VHT40MINUS(chan))
        return IEEE80211_MODE_11AC_VHT40MINUS;
    else if (IEEE80211_IS_CHAN_11AC_VHT80(chan))
        return IEEE80211_MODE_11AC_VHT80;
    else if (IEEE80211_IS_CHAN_11AC_VHT160(chan))
        return IEEE80211_MODE_11AC_VHT160;
    else if (IEEE80211_IS_CHAN_11AC_VHT80_80(chan))
        return IEEE80211_MODE_11AC_VHT80_80;
     else if (IEEE80211_IS_CHAN_11AXA_HE20(chan))
         return IEEE80211_MODE_11AXA_HE20;
     else if (IEEE80211_IS_CHAN_11AXG_HE20(chan))
         return IEEE80211_MODE_11AXG_HE20;
     else if (IEEE80211_IS_CHAN_11AXA_HE40PLUS(chan))
         return IEEE80211_MODE_11AXA_HE40PLUS;
     else if (IEEE80211_IS_CHAN_11AXA_HE40MINUS(chan))
         return IEEE80211_MODE_11AXA_HE40MINUS;
     else if (IEEE80211_IS_CHAN_11AXG_HE40PLUS(chan))
         return IEEE80211_MODE_11AXG_HE40PLUS;
     else if (IEEE80211_IS_CHAN_11AXG_HE40MINUS(chan))
         return IEEE80211_MODE_11AXG_HE40MINUS;
     else if (IEEE80211_IS_CHAN_11AXA_HE80(chan))
         return IEEE80211_MODE_11AXA_HE80;
     else if (IEEE80211_IS_CHAN_11AXA_HE160(chan))
         return IEEE80211_MODE_11AXA_HE160;
     else if (IEEE80211_IS_CHAN_11AXA_HE80_80(chan))
         return IEEE80211_MODE_11AXA_HE80_80;
#ifdef WLAN_FEATURE_11BE
     else if (IEEE80211_IS_CHAN_11BEA_EHT20(chan))
         return IEEE80211_MODE_11BEA_EHT20;
     else if (IEEE80211_IS_CHAN_11BEG_EHT20(chan))
         return IEEE80211_MODE_11BEG_EHT20;
     else if (IEEE80211_IS_CHAN_11BEA_EHT40PLUS(chan))
         return IEEE80211_MODE_11BEA_EHT40PLUS;
     else if (IEEE80211_IS_CHAN_11BEA_EHT40MINUS(chan))
         return IEEE80211_MODE_11BEA_EHT40MINUS;
     else if (IEEE80211_IS_CHAN_11BEG_EHT40PLUS(chan))
         return IEEE80211_MODE_11BEG_EHT40PLUS;
     else if (IEEE80211_IS_CHAN_11BEG_EHT40MINUS(chan))
         return IEEE80211_MODE_11BEG_EHT40MINUS;
     else if (IEEE80211_IS_CHAN_11BEA_EHT80(chan))
         return IEEE80211_MODE_11BEA_EHT80;
     else if (IEEE80211_IS_CHAN_11BEA_EHT160(chan))
         return IEEE80211_MODE_11BEA_EHT160;
     else if (IEEE80211_IS_CHAN_11BEA_EHT320(chan))
         return IEEE80211_MODE_11BEA_EHT320;
#endif /* WLAN_FEATURE_11BE */

    /* NB: should not get here */
     if (printk_ratelimit()) {
         qdf_info("cannot map channel to mode; freq %u flags 0x%llx",
                  chan->ic_freq, chan->ic_flags);
     }
    return IEEE80211_MODE_11B;
}

char
ieee80211_get_vap_mode(wlan_if_t vaphandle)
{
    /* 11AX TODO - Add 11ax processing here if required in the future. Currently,
     * 11ac too is not processed.
     */
    /* 11be avoided as 11ax modes are not processed */
    int mode = vaphandle->iv_cur_mode;
    switch (mode) {
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NA_HT40:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NA_HT40PLUS:
            return 'n';
        case IEEE80211_MODE_11A:
            return 'a';
        case IEEE80211_MODE_11G:
            return 'g';
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NG_HT40:
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40PLUS:
            return 'n';
        case IEEE80211_MODE_11B:
            return 'b';
        default:
            return 'f';
    }
}

/*
 * Check for Channel and Mode consistency. If channel and mode mismatches, return error.
 */
#define IEEE80211_MODE_TURBO_STATIC_A   IEEE80211_MODE_MAX
int
ieee80211_check_chan_mode_consistency(struct ieee80211com *ic,int mode,struct ieee80211_ath_channel *c)
{
    if (!IEEE80211_IS_CHAN_VALID(c)) return 0;

    switch (mode)
    {
    case IEEE80211_MODE_11B:
        if(IEEE80211_IS_CHAN_B(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11G:
        if(IEEE80211_IS_CHAN_ANYG(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11A:
        if(IEEE80211_IS_CHAN_A(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_TURBO_STATIC_A:
        if(IEEE80211_IS_CHAN_A(c) && IEEE80211_IS_CHAN_STURBO(c) )
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_AUTO:
        return 0;
        break;

    case IEEE80211_MODE_11NG_HT20:
        if(IEEE80211_IS_CHAN_11NG_HT20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NG_HT40PLUS:
        if(IEEE80211_IS_CHAN_11NG_HT40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NG_HT40MINUS:
        if(IEEE80211_IS_CHAN_11NG_HT40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NG_HT40:
        if(IEEE80211_IS_CHAN_11NG_HT40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NA_HT20:
        if(IEEE80211_IS_CHAN_11NA_HT20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NA_HT40PLUS:
        if(IEEE80211_IS_CHAN_11NA_HT40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NA_HT40MINUS:
        if(IEEE80211_IS_CHAN_11NA_HT40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11NA_HT40:
        if(IEEE80211_IS_CHAN_11NA_HT40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT20:
        if(IEEE80211_IS_CHAN_11AC_VHT20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT40PLUS:
        if(IEEE80211_IS_CHAN_11AC_VHT40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT40MINUS:
        if(IEEE80211_IS_CHAN_11AC_VHT40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT40:
        if(IEEE80211_IS_CHAN_11AC_VHT40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT80:
        if(IEEE80211_IS_CHAN_11AC_VHT80(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT160:
        if(IEEE80211_IS_CHAN_11AC_VHT160(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AC_VHT80_80:
        if(IEEE80211_IS_CHAN_11AC_VHT80_80(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE20:
        if(IEEE80211_IS_CHAN_11AXA_HE20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXG_HE20:
        if(IEEE80211_IS_CHAN_11AXG_HE20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE40PLUS:
        if(IEEE80211_IS_CHAN_11AXA_HE40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE40MINUS:
        if(IEEE80211_IS_CHAN_11AXA_HE40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXG_HE40PLUS:
        if(IEEE80211_IS_CHAN_11AXG_HE40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXG_HE40MINUS:
        if(IEEE80211_IS_CHAN_11AXG_HE40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE40:
        if(IEEE80211_IS_CHAN_11AXA_HE40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXG_HE40:
        if(IEEE80211_IS_CHAN_11AXG_HE40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE80:
        if(IEEE80211_IS_CHAN_11AXA_HE80(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE160:
        if(IEEE80211_IS_CHAN_11AXA_HE160(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11AXA_HE80_80:
        if(IEEE80211_IS_CHAN_11AXA_HE80_80(c))
            return 0;
        else
            return -EINVAL;
        break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT20:
        if(IEEE80211_IS_CHAN_11BEA_EHT20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEG_EHT20:
        if(IEEE80211_IS_CHAN_11BEG_EHT20(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEA_EHT40PLUS:
        if(IEEE80211_IS_CHAN_11BEA_EHT40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEA_EHT40MINUS:
        if(IEEE80211_IS_CHAN_11BEA_EHT40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEG_EHT40PLUS:
        if(IEEE80211_IS_CHAN_11BEG_EHT40PLUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEG_EHT40MINUS:
        if(IEEE80211_IS_CHAN_11BEG_EHT40MINUS(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEA_EHT40:
        if(IEEE80211_IS_CHAN_11BEA_EHT40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEG_EHT40:
        if(IEEE80211_IS_CHAN_11BEG_EHT40(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEA_EHT80:
        if(IEEE80211_IS_CHAN_11BEA_EHT80(c))
            return 0;
        else
            return -EINVAL;
        break;

    case IEEE80211_MODE_11BEA_EHT160:
        if(IEEE80211_IS_CHAN_11BEA_EHT160(c))
            return 0;
        else
            return -EINVAL;
        break;
    case IEEE80211_MODE_11BEA_EHT320:
        if(IEEE80211_IS_CHAN_11BEA_EHT320(c))
            return 0;
        else
            return -EINVAL;
        break;
#endif /* WLAN_FEATURE_11BE */
    }

    return -EINVAL;
}
#undef  IEEE80211_MODE_TURBO_STATIC_A

void
ieee80211_channel_notify_to_app(struct ieee80211com *ic)
{
    char buf[128];

    snprintf(buf, sizeof(buf),
            "DFS_UPDATE: Radar found, is_chan_available = %u", !ic->no_chans_available);

    osif_update_dfs_info_to_app(ic, buf);
}

#ifdef WLAN_FEATURE_11BE

uint16_t get_afc_puncture_bimtap(struct ieee80211com *ic, struct chan_params *chan_p, enum ieee80211_phymode mode)
{
    uint16_t center_freq = 0;
    enum phy_ch_width ch_width;
    uint32_t bw = 0;
    uint16_t afc_bitmap = PUNCTURE_NONE;
    uint16_t i;
    qdf_freq_t subchannels[IEE80211_MAX_20M_SUB_CH];
    int nchans;

    if (!chan_p) {
        qdf_err("chan_p is NULL");
        return PUNCTURE_INVALID;
    }

    ch_width = ieee80211_get_chan_width_from_phymode(mode);
    bw = ieee80211_get_chan_width_from_phy_chwidth(ch_width);
    if (bw < CHWIDTH_80) {
        return PUNCTURE_NONE;
    }

    center_freq = chan_p->cfreq2 ?  chan_p->cfreq2 :
                                    ieee80211_find_ch_center(ic, chan_p->freq, chan_p->cfreq2, bw, 0);
    nchans = ieee80211_get_subchannels_with_width_centerfreq(center_freq, ch_width, subchannels);

    for (i = 0; i < nchans; i++) {
        if (!wlan_reg_is_freq_txable(ic->ic_pdev_obj, subchannels[i], REG_AP_SP)) {
            afc_bitmap |= 1 << i;
        }
    }

    return afc_bitmap;
}

struct ieee80211_ath_channel
ieee80211_find_channel(struct ieee80211com *ic, struct chan_params *chan_p, u_int64_t flags)
{
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;
    uint16_t afc_pp = 0;
    enum ieee80211_phymode mode = 0;

    if (!chan_p) {
        return empty_channel;
    }

    if ((chan_p->input_6g_pwr_mode == REG_AP_SP) && IEEE80211_IS_FLAG_EHT(flags)) {
        mode = ieee80211_get_11bea_phymode(flags);
        afc_pp = get_afc_puncture_bimtap(ic, chan_p, mode);
        chan_p->puncture_bitmap |= afc_pp;
    }

    *chan = ieee80211_find_channel_internal(ic,
                                            chan_p->freq,
                                            chan_p->cfreq2,
                                            flags,
                                            chan_p->input_6g_pwr_mode,
                                            chan_p->puncture_bitmap);
    if (!IEEE80211_IS_CHAN_VALID(chan)) {
        return empty_channel;
    }

    chan_p->puncture_strict = ic->ic_puncture_strict;
    return ieee80211_find_puncture_bitmap(ic, chan, chan_p);
}
#else
struct ieee80211_ath_channel
ieee80211_find_channel(struct ieee80211com *ic, struct chan_params *chan_p, u_int64_t flags)
{
    return ieee80211_find_channel_internal(ic,
                                           chan_p->freq,
                                           chan_p->cfreq2,
                                           flags,
                                           chan_p->input_6g_pwr_mode,
                                           NO_SCHANS_PUNC);
}
#endif /* */

#ifdef __linux__
//#ifndef ATH_WLAN_COMBINE
#endif

#ifdef WLAN_FEATURE_11BE
void ieee80211_copy_ch_params(struct ch_params *tgt_ch_params,
                              struct ch_params *src_ch_params)
{
    tgt_ch_params->ch_width = src_ch_params->ch_width;
    tgt_ch_params->sec_ch_offset = src_ch_params->sec_ch_offset;
    tgt_ch_params->center_freq_seg0 = src_ch_params->center_freq_seg0;
    tgt_ch_params->center_freq_seg1 = src_ch_params->center_freq_seg1;
    tgt_ch_params->mhz_freq_seg0 = src_ch_params->mhz_freq_seg0;
    tgt_ch_params->mhz_freq_seg1 = src_ch_params->mhz_freq_seg1;
    tgt_ch_params->reg_punc_bitmap = src_ch_params->reg_punc_bitmap;
}
#endif

struct ieee80211_ath_channel
ieee80211_doth_findchan(struct ieee80211vap *vap, struct chan_params *chan_p)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_ath_channel c;
    u_int64_t flags;

    /* NB: try first to preserve turbo */
    flags = vap->iv_bsschan->ic_flags & IEEE80211_CHAN_ALL;
    c = ieee80211_find_channel(ic, chan_p, flags);
    if (!IEEE80211_IS_CHAN_VALID(&c)) {
        chan_p->cfreq2 = 0;
        c = ieee80211_find_channel(ic, chan_p, 0);
    }
    return c;
}

uint64_t ieee80211_get_band_flag(uint16_t freq)
{
    uint64_t band_flag = 0;

    if (WLAN_REG_IS_24GHZ_CH_FREQ(freq)) {
        band_flag = IEEE80211_CHAN_2GHZ;
    } else if (WLAN_REG_IS_5GHZ_CH_FREQ(freq)) {
        band_flag = IEEE80211_CHAN_5GHZ;
    } else if (WLAN_REG_IS_6GHZ_CHAN_FREQ(freq)) {
        band_flag = IEEE80211_CHAN_6GHZ;
    } else if (WLAN_REG_IS_49GHZ_FREQ (freq)) {
        band_flag = IEEE80211_CHAN_5GHZ;
    }

    return band_flag;
}

#ifdef CONFIG_HOST_FIND_CHAN
/*
 * ieee80211_fill_sec_2g_freq() - Compute sec_ch_2g_freq for 2G channels
 * based on the user phymode.
 * @primary_freq: Primary operating frequency.
 * @mode: User given phymode.
 *
 * Return: sec_ch_2g_freq in MHZ.
 */
qdf_freq_t
ieee80211_fill_sec_2g_freq (qdf_freq_t primary_freq,
                            enum ieee80211_phymode mode)
{
    qdf_freq_t sec_ch_2g_freq;

    switch(mode) {
        case IEEE80211_MODE_11AXG_HE40PLUS:
        case IEEE80211_MODE_11NG_HT40PLUS:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEG_EHT40PLUS:
#endif
            sec_ch_2g_freq = primary_freq + CHAN_HT40_OFFSET;
            break;
        case IEEE80211_MODE_11AXG_HE40MINUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEG_EHT40MINUS:
#endif
            sec_ch_2g_freq = primary_freq - CHAN_HT40_OFFSET;
            break;
        default:
            sec_ch_2g_freq = 0;
    }
    return sec_ch_2g_freq;
}

/**
 * ieee80211_find_mode_from_chwidth_and_regphymode() - Find combined mode from
 * 80211mode, bandwidth and primary frequency.
 * @r_phymode: Input 80211 mode.
 * @ch_width: Input bandwidth.
 * @freq: Input frequency.
 */
static enum ieee80211_phymode
ieee80211_find_mode_from_chwidth_and_regphymode(enum reg_phymode r_phymode,
                                                enum phy_ch_width ch_width,
                                                qdf_freq_t freq)
{
    uint64_t band = ieee80211_get_band_flag(freq);

    switch (r_phymode) {
    case REG_PHYMODE_11B:
        if (ch_width > CH_WIDTH_20MHZ || !IEEE80211_IS_FLAG_2GHZ(band))
            return IEEE80211_MODE_MAX;
        return IEEE80211_MODE_11B;
    case REG_PHYMODE_11G:
        if (ch_width > CH_WIDTH_20MHZ || !IEEE80211_IS_FLAG_2GHZ(band))
            return IEEE80211_MODE_MAX;
        return IEEE80211_MODE_11G;
    case REG_PHYMODE_11A:
        if (ch_width > CH_WIDTH_20MHZ || !IEEE80211_IS_FLAG_5GHZ(band))
            return IEEE80211_MODE_MAX;
        return IEEE80211_MODE_11A;
    case REG_PHYMODE_11N:
        if (IEEE80211_IS_FLAG_2GHZ(band)) {
            if (ch_width == CH_WIDTH_20MHZ)
                return IEEE80211_MODE_11NG_HT20;
            if (ch_width == CH_WIDTH_40MHZ)
                return IEEE80211_MODE_11NG_HT40;
            return IEEE80211_MODE_MAX;
        }
        if (IEEE80211_IS_FLAG_5GHZ(band)) {
            if (ch_width == CH_WIDTH_20MHZ)
                return IEEE80211_MODE_11NA_HT20;
            if (ch_width == CH_WIDTH_40MHZ)
                return IEEE80211_MODE_11NA_HT40;
            return IEEE80211_MODE_MAX;
        }
        return IEEE80211_MODE_MAX;
    case REG_PHYMODE_11AC:
        if (IEEE80211_IS_FLAG_5GHZ(band)) {
            switch (ch_width) {
            case CH_WIDTH_20MHZ:
                return IEEE80211_MODE_11AC_VHT20;
            case CH_WIDTH_40MHZ:
                return IEEE80211_MODE_11AC_VHT40;
            case CH_WIDTH_80MHZ:
                return IEEE80211_MODE_11AC_VHT80;
            case CH_WIDTH_160MHZ:
                return IEEE80211_MODE_11AC_VHT160;
            case CH_WIDTH_80P80MHZ:
                return IEEE80211_MODE_11AC_VHT80_80;
            default:
                return IEEE80211_MODE_MAX;
            }
        }
        return IEEE80211_MODE_MAX;
    case REG_PHYMODE_11AX:
        if (IEEE80211_IS_FLAG_5GHZ(band) || IEEE80211_IS_FLAG_6GHZ(band)) {
            switch (ch_width) {
            case CH_WIDTH_20MHZ:
                return IEEE80211_MODE_11AXA_HE20;
            case CH_WIDTH_40MHZ:
                return IEEE80211_MODE_11AXA_HE40;
            case CH_WIDTH_80MHZ:
                return IEEE80211_MODE_11AXA_HE80;
            case CH_WIDTH_160MHZ:
                return IEEE80211_MODE_11AXA_HE160;
            case CH_WIDTH_80P80MHZ:
                return IEEE80211_MODE_11AXA_HE80_80;
            default:
                return IEEE80211_MODE_MAX;
            }
        }
        if (IEEE80211_IS_FLAG_2GHZ(band)) {
            if (ch_width == CH_WIDTH_20MHZ)
                return IEEE80211_MODE_11AXG_HE20;
            if (ch_width == CH_WIDTH_40MHZ)
                return IEEE80211_MODE_11AXG_HE40;
            return IEEE80211_MODE_MAX;
        }
        return IEEE80211_MODE_MAX;
#ifdef WLAN_FEATURE_11BE
    case REG_PHYMODE_11BE:
        if (IEEE80211_IS_FLAG_5GHZ(band) || IEEE80211_IS_FLAG_6GHZ(band)) {
            switch (ch_width) {
            case CH_WIDTH_20MHZ:
                return IEEE80211_MODE_11BEA_EHT20;
            case CH_WIDTH_40MHZ:
                return IEEE80211_MODE_11BEA_EHT40;
            case CH_WIDTH_80MHZ:
                return IEEE80211_MODE_11BEA_EHT80;
            case CH_WIDTH_160MHZ:
                return IEEE80211_MODE_11BEA_EHT160;
            case CH_WIDTH_320MHZ:
                return IEEE80211_MODE_11BEA_EHT320;
            default:
                return IEEE80211_MODE_MAX;
            }
        }
        if (IEEE80211_IS_FLAG_2GHZ(band)) {
            if (ch_width == CH_WIDTH_20MHZ)
                return IEEE80211_MODE_11BEG_EHT20;
            if (ch_width == CH_WIDTH_40MHZ)
                return IEEE80211_MODE_11BEG_EHT40;
            return IEEE80211_MODE_MAX;
        }
        return IEEE80211_MODE_MAX;
#endif
    default:
        return IEEE80211_MODE_MAX;
    }
}

/**
 * ieee80211_conv_mode_for_40modes() - Convert mode for 40PLUS/40MINUS.
 * @mode: input mode.
 * @primary_freq: Primary frequency.
 * @center_freq: Center frequency.
 *
 * Determine if the 40MHz channel is 40PLUS or 40MINUS and return mode
 * accordingly.
 */
static enum ieee80211_phymode
ieee80211_conv_mode_for_40modes(enum ieee80211_phymode mode,
                                qdf_freq_t primary_freq,
                                qdf_freq_t center_freq)
{
    bool is_plus = true;

    if (primary_freq > center_freq)
        is_plus = false;

    switch (mode) {
    case IEEE80211_MODE_11NG_HT40:
        if (is_plus)
            return IEEE80211_MODE_11NG_HT40PLUS;
        else
            return IEEE80211_MODE_11NG_HT40MINUS;
    case IEEE80211_MODE_11NA_HT40:
        if (is_plus)
            return IEEE80211_MODE_11NA_HT40PLUS;
        else
            return IEEE80211_MODE_11NA_HT40MINUS;
    case IEEE80211_MODE_11AC_VHT40:
        if (is_plus)
            return IEEE80211_MODE_11AC_VHT40PLUS;
        else
            return IEEE80211_MODE_11AC_VHT40MINUS;
    case IEEE80211_MODE_11AXA_HE40:
        if (is_plus)
            return IEEE80211_MODE_11AXA_HE40PLUS;
        else
            return IEEE80211_MODE_11AXA_HE40MINUS;
    case IEEE80211_MODE_11AXG_HE40:
        if (is_plus)
            return IEEE80211_MODE_11AXG_HE40PLUS;
        else
            return IEEE80211_MODE_11AXG_HE40MINUS;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT40:
        if (is_plus)
            return IEEE80211_MODE_11BEA_EHT40PLUS;
        else
            return IEEE80211_MODE_11BEA_EHT40MINUS;
    case IEEE80211_MODE_11BEG_EHT40:
        if (is_plus)
            return IEEE80211_MODE_11BEG_EHT40PLUS;
        else
            return IEEE80211_MODE_11BEG_EHT40MINUS;
#endif
    default:
        return mode;
    }
}

#define R_PHYMODE 0
#define CH_WIDTH 1
uint8_t phymode_decoupler[IEEE80211_MODE_MAX][2] =
{
    { REG_PHYMODE_11AX, CH_WIDTH_160MHZ },   /* IEEE80211_MODE_AUTO */
    { REG_PHYMODE_11A, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_11A */
    { REG_PHYMODE_11B, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_11B */
    { REG_PHYMODE_11G, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_11G */
    { REG_PHYMODE_11A, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_FH */
    { REG_PHYMODE_11A, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_TURBO_A */
    { REG_PHYMODE_11G, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_TURBO_G */
    { REG_PHYMODE_11N, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_11NA_HT20 */
    { REG_PHYMODE_11N, CH_WIDTH_20MHZ },     /* IEEE80211_MODE_11NG_HT20 */
    { REG_PHYMODE_11N, CH_WIDTH_40MHZ },     /* IEEE80211_MODE_11NA_HT40PLUS */
    { REG_PHYMODE_11N, CH_WIDTH_40MHZ },     /* IEEE80211_MODE_11NA_HT40MINUS */
    { REG_PHYMODE_11N, CH_WIDTH_40MHZ },     /* IEEE80211_MODE_11NG_HT40PLUS */
    { REG_PHYMODE_11N, CH_WIDTH_40MHZ },     /* IEEE80211_MODE_11NG_HT40MINUS */
    { REG_PHYMODE_11N, CH_WIDTH_40MHZ },     /* IEEE80211_MODE_11NG_HT40 */
    { REG_PHYMODE_11N, CH_WIDTH_40MHZ },     /* IEEE80211_MODE_11NA_HT40 */
    { REG_PHYMODE_11AC, CH_WIDTH_20MHZ },    /* IEEE80211_MODE_11AC_VHT20 */
    { REG_PHYMODE_11AC, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AC_VHT40PLUS */
    { REG_PHYMODE_11AC, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AC_VHT40MINUS */
    { REG_PHYMODE_11AC, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AC_VHT40 */
    { REG_PHYMODE_11AC, CH_WIDTH_80MHZ },    /* IEEE80211_MODE_11AC_VHT80 */
    { REG_PHYMODE_11AC, CH_WIDTH_160MHZ },   /* IEEE80211_MODE_11AC_VHT160 */
    { REG_PHYMODE_11AC, CH_WIDTH_80P80MHZ }, /* IEEE80211_MODE_11AC_VHT80_80 */
    { REG_PHYMODE_11AX, CH_WIDTH_20MHZ },    /* IEEE80211_MODE_11AXA_HE20 */
    { REG_PHYMODE_11AX, CH_WIDTH_20MHZ },    /* IEEE80211_MODE_11AXG_HE20 */
    { REG_PHYMODE_11AX, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AXA_HE40PLUS */
    { REG_PHYMODE_11AX, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AXA_HE40MINUS */
    { REG_PHYMODE_11AX, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AXG_HE40PLUS */
    { REG_PHYMODE_11AX, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AXG_HE40MINUS */
    { REG_PHYMODE_11AX, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AXA_HE40 */
    { REG_PHYMODE_11AX, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11AXG_HE40 */
    { REG_PHYMODE_11AX, CH_WIDTH_80MHZ },    /* IEEE80211_MODE_11AXA_HE80 */
    { REG_PHYMODE_11AX, CH_WIDTH_160MHZ },   /* IEEE80211_MODE_11AXA_HE160 */
    { REG_PHYMODE_11AX, CH_WIDTH_80P80MHZ }, /* IEEE80211_MODE_11AXA_HE80_80 */
#ifdef WLAN_FEATURE_11BE
    { REG_PHYMODE_11BE, CH_WIDTH_20MHZ },    /* IEEE80211_MODE_11BEA_EHT20 */
    { REG_PHYMODE_11BE, CH_WIDTH_20MHZ },    /* IEEE80211_MODE_11BEG_EHT20 */
    { REG_PHYMODE_11BE, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11BEA_EHT40PLUS */
    { REG_PHYMODE_11BE, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11BEA_EHT40MINUS */
    { REG_PHYMODE_11BE, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11BEG_EHT40PLUS */
    { REG_PHYMODE_11BE, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11BEG_EHT40MINUS */
    { REG_PHYMODE_11BE, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11BEA_EHT40 */
    { REG_PHYMODE_11BE, CH_WIDTH_40MHZ },    /* IEEE80211_MODE_11BEG_EHT40 */
    { REG_PHYMODE_11BE, CH_WIDTH_80MHZ },    /* IEEE80211_MODE_11BEA_EHT80 */
    { REG_PHYMODE_11BE, CH_WIDTH_160MHZ },   /* IEEE80211_MODE_11BEA_EHT160 */
    { REG_PHYMODE_11BE, CH_WIDTH_320MHZ },   /* IEEE80211_MODE_11BEA_EHT320 */
#endif
};

/*
 * ieee80211_find_next_lower_phymode:
 * Find the next lower phymode from the input mode.
 * NOTE: If mode is already lowest (i.e., 20MHz), then return NONE. This
 * API can be used in iterations along with the condition that the mode shall
 * not return NONE.
 *
 * @mode: Value of the given phymode
 *
 * Return:
 * IEEE80211_MODE_*   : Success
 * IEEE80211_MODE_AUTO: Failure
 */
enum ieee80211_phymode
ieee80211_find_next_lower_phymode(enum ieee80211_phymode mode)
{
    enum phy_ch_width ch_width = phymode_decoupler[mode][CH_WIDTH];
    enum reg_phymode r_phymode = phymode_decoupler[mode][R_PHYMODE];
    uint32_t ix;

    /*
     * If the given phymode is 20MHz, then we cannot lower any further.
     * Return MODE_NONE to notify the caller of failed downgrade
     */
    if (ch_width == CH_WIDTH_40MHZ) {
        ch_width = CH_WIDTH_20MHZ;
    } else if (ch_width == CH_WIDTH_80MHZ) {
        ch_width = CH_WIDTH_40MHZ;
    } else if (ch_width == CH_WIDTH_160MHZ) {
        ch_width = CH_WIDTH_80MHZ;
#ifdef WLAN_FEATURE_11BE
    } else if (ch_width == CH_WIDTH_320MHZ) {
        ch_width = CH_WIDTH_160MHZ;
#endif
    } else {
        return IEEE80211_MODE_NONE;
    }

    /*
     * Iterate from the given phymode to the lowest non-auto phymode.
     */
    for (ix = mode; ix > IEEE80211_MODE_AUTO; ix--) {
        if (r_phymode != phymode_decoupler[ix][R_PHYMODE]) {
            continue;
        }

        if ((ieee80211_is_phymode_2g(mode) &&
             !ieee80211_is_phymode_2g(ix)) ||
            (ieee80211_is_phymode_5g_or_6g(mode) &&
             !ieee80211_is_phymode_5g_or_6g(ix))) {
            continue;
        }

        if (ch_width == (phymode_decoupler[ix][CH_WIDTH])) {
            break;
        }
    }

    return ((ix == IEEE80211_MODE_AUTO) ? IEEE80211_MODE_NONE : ix);
}

#ifdef CONFIG_AFC_SUPPORT
/*
 * ieee80211_is_afc_capable:
 * Return AFC capability for a given radio and phymode
 *
 * @ic: Pointer to ic
 * @mode: Desired phymode
 *
 * Return:
 * Boolean value
 */
bool ieee80211_is_afc_capable(struct ieee80211com *ic,
                              enum ieee80211_phymode mode)
{
    if (!ic)
        return false;

    /*
     * In order for a radio to be AFC capable, it needs to satisfy the following
     * conditions:
     * (1) The radio must be 6GHz capable
     * (2) The desired phymode must be 11AXA (11AX + 5/6GHz)
     * (3) The AFC power event should be present in the regulatory sub-system
     */
    if (!wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_6G) ||
        (!ieee80211_is_phymode_11ax(mode)
#ifdef WLAN_FEATURE_11BE
        && !ieee80211_is_phymode_11be(mode)
#endif /* WLAN_FEATURE_11BE */
        ) ||
        !ieee80211_is_phymode_5g_or_6g(mode) ||
        (!wlan_reg_is_afc_power_event_received(ic->ic_pdev_obj))) {
        return false;
    }

    return true;
}
#endif /* CONFIG_AFC_SUPPORT */

bool ieee80211_is_phymode_supported_by_channel(struct ieee80211com *ic,
                                               qdf_freq_t freq,
                                               enum ieee80211_phymode mode,
                                               enum supported_6g_pwr_types in_6g_pwr_mode,
                                               uint16_t puncture_bitmap)
{
    enum phy_ch_width ch_width = phymode_decoupler[mode][CH_WIDTH];
    enum reg_phymode r_phymode = phymode_decoupler[mode][R_PHYMODE];


    if (!IEEE80211_SUPPORT_PHY_MODE(ic, mode))
        return false;

    if (!wlan_reg_is_phymode_chwidth_allowed(ic->ic_pdev_obj, r_phymode,
                                             ch_width, freq,
                                             in_6g_pwr_mode,
                                             puncture_bitmap))
        return false;

    return true;
}

int ieee80211_get_subchannels(struct ieee80211com *ic,
                              qdf_freq_t freq,
                              enum ieee80211_phymode mode,
                              qdf_freq_t *subchannels)
{
    struct ch_params chan_params = {0};
    enum phy_ch_width ch_width = phymode_decoupler[mode][CH_WIDTH];
    qdf_freq_t sec_ch_2g_freq = 0;
    int nchans;
    uint16_t center_freq;

    chan_params.ch_width = ch_width;
    if (ch_width == CH_WIDTH_40MHZ && WLAN_REG_IS_24GHZ_CH_FREQ(freq))
        sec_ch_2g_freq =
            ieee80211_fill_sec_2g_freq(freq, mode);

    ieee80211_regdmn_get_channel_params(ic->ic_pdev_obj,
                                        freq,
                                        sec_ch_2g_freq,
                                        &chan_params,
                                        REG_CURRENT_PWR_MODE);

    if (!chan_params.mhz_freq_seg0 || ch_width != chan_params.ch_width)
        return 0;

    center_freq = chan_params.mhz_freq_seg1 ? chan_params.mhz_freq_seg1 :
                                              chan_params.mhz_freq_seg0;

    nchans = ieee80211_get_subchannels_with_width_centerfreq(center_freq,
            ch_width, subchannels);

    return nchans;
}

int ieee80211_get_subchannels_with_width_centerfreq(uint16_t center_freq,
        enum phy_ch_width ch_width, qdf_freq_t *subchannels)
{
    int nchans;

    switch (ch_width) {
        case CH_WIDTH_20MHZ:
            nchans = 1;
            subchannels[0] = center_freq;
            break;
        case CH_WIDTH_40MHZ:
            nchans = 2;
            subchannels[0] = center_freq - 10;
            subchannels[1] = center_freq + 10;
            break;
        case CH_WIDTH_80MHZ:
            nchans = 4;
            subchannels[0] = center_freq - 30;
            subchannels[1] = center_freq - 10;
            subchannels[2] = center_freq + 10;
            subchannels[3] = center_freq + 30;
            break;
        case CH_WIDTH_160MHZ:
            nchans = 8;
            subchannels[0] = center_freq - 70;
            subchannels[1] = center_freq - 50;
            subchannels[2] = center_freq - 30;
            subchannels[3] = center_freq - 10;
            subchannels[4] = center_freq + 10;
            subchannels[5] = center_freq + 30;
            subchannels[6] = center_freq + 50;
            subchannels[7] = center_freq + 70;
            break;
#ifdef WLAN_FEATURE_11BE
            /* Consider puncturing. */
        case CH_WIDTH_320MHZ:
            nchans = 16;
            subchannels[0]  = center_freq - 150;
            subchannels[1]  = center_freq - 130;
            subchannels[2]  = center_freq - 110;
            subchannels[3]  = center_freq - 90;
            subchannels[4]  = center_freq - 70;
            subchannels[5]  = center_freq - 50;
            subchannels[6]  = center_freq - 30;
            subchannels[7]  = center_freq - 10;
            subchannels[8]  = center_freq + 10;
            subchannels[9]  = center_freq + 30;
            subchannels[10] = center_freq + 50;
            subchannels[11] = center_freq + 70;
            subchannels[12] = center_freq + 90;
            subchannels[13] = center_freq + 110;
            subchannels[14] = center_freq + 130;
            subchannels[15] = center_freq + 150;
            break;
#endif
        default:
            nchans = 0;
            break;
    }

    return nchans;
}

wlan_if_t ieee80211_find_first_active_vap(struct ieee80211com *ic)
{
        if (!TAILQ_EMPTY(&ic->ic_vaps)) {
                return TAILQ_FIRST(&(ic)->ic_vaps);
        } else {
                IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_MLME,
                        "%s: ic_vaps is NULL, Returning\n",__func__);
                    return NULL;
        }
}

static bool
ieee80211_reg_is_no_ofdm_set(struct ieee80211com *ic,
                             qdf_freq_t primary_freq,
                             struct regulatory_channel *reg_chan)
{
        if (reg_chan->center_freq == primary_freq &&
            reg_chan->chan_flags & REGULATORY_CHAN_NO_OFDM) {
            return true;
        }
    return false;
}

/**
 * ieee80211_get_mode_info_from_mode() - Find the best supported phymode and
 * chwidth, if auto. Else check if provided phymode is supported.
 * @ic: Pointer to ic.
 * @primary_freq: Primary frequency.
 * @mode: Input combined mode of 80211 and bandwidth.
 * @halfquarter: Flags indicating half and quarter.
 * @mode_info: Output structure to fill the mode information.
 * @in_6g_pwr_mode: Input 6G power type which will determine the 6G channel
 * list on which the primary_freq is searched for.
 * @reg_chan_list: regulatory channel list.
 * @input_puncture_bitmap: Input puncture bitmap
 */
static int
ieee80211_get_mode_info_from_mode(struct ieee80211com *ic,
                                  qdf_freq_t primary_freq,
                                  enum ieee80211_phymode mode,
                                  uint64_t halfquarter,
                                  struct ieee80211_mode_info *mode_info,
                                  enum supported_6g_pwr_types in_6g_pwr_mode,
                                  struct regulatory_channel *reg_chan_list,
                                  uint16_t input_puncture_bitmap)
{
    int status = EOK;
    enum phy_ch_width ch_width = CH_WIDTH_INVALID;
    enum reg_phymode r_phymode = REG_PHYMODE_INVALID;
    uint16_t max_bw_5g = ieee80211_get_max5gbw(ic);

    if (ieee80211_reg_is_no_ofdm_set(ic, primary_freq, reg_chan_list)) {
        mode = IEEE80211_MODE_11B;
        ch_width = CH_WIDTH_20MHZ;
        r_phymode = REG_PHYMODE_11B;

        if (!wlan_reg_is_phymode_chwidth_allowed(ic->ic_pdev_obj,
                                                 r_phymode,
                                                 ch_width,
                                                 primary_freq,
                                                 in_6g_pwr_mode,
                                                 input_puncture_bitmap))
            return -EINVAL;

        goto exit;
    }

    if (mode == IEEE80211_MODE_AUTO) {
        wlan_if_t vap = ieee80211_find_first_active_vap(ic);

        if (!vap)
            return -EINVAL;

        mode_info->is_auto = true;
        mode = vap->iv_des_hw_mode;

        if (WLAN_REG_IS_49GHZ_FREQ(primary_freq)) {
            mode = IEEE80211_MODE_11A;
            ch_width = CH_WIDTH_20MHZ;
            r_phymode = REG_PHYMODE_11A;

            if (!wlan_reg_is_phymode_chwidth_allowed(ic->ic_pdev_obj,
                                                     r_phymode,
                                                     ch_width,
                                                     primary_freq,
                                                     in_6g_pwr_mode,
                                                     input_puncture_bitmap))
                return -EINVAL;

            goto exit;
        }

        r_phymode = phymode_decoupler[mode][R_PHYMODE];
        ch_width = phymode_decoupler[mode][CH_WIDTH];
        if (mode == IEEE80211_MODE_AUTO ||
            !wlan_reg_is_phymode_chwidth_allowed(ic->ic_pdev_obj,
                                                 r_phymode,
                                                 ch_width,
                                                 primary_freq,
                                                 in_6g_pwr_mode,
                                                 input_puncture_bitmap))
            status = wlan_reg_get_max_phymode_and_chwidth(ic->ic_pdev_obj,
                                                          &r_phymode,
                                                          &ch_width);
        if (status)
            return status;

    } else {
        uint16_t min_ch_bw, max_ch_bw;

        r_phymode = phymode_decoupler[mode][R_PHYMODE];
        ch_width = phymode_decoupler[mode][CH_WIDTH];
        mode_info->is_auto = false;
        min_ch_bw = wlan_reg_get_min_chwidth(ic->ic_pdev_obj, primary_freq);
        max_ch_bw = wlan_reg_get_max_chwidth(ic->ic_pdev_obj, primary_freq);

        if (WLAN_REG_IS_49GHZ_FREQ(primary_freq)) {
            if (r_phymode != REG_PHYMODE_11A) {
                qdf_debug("Mode (%u) not supported by 4.9Ghz band for freq %u",
                          mode, primary_freq);
                return -EINVAL;
            }
            if (BW_WITHIN(min_ch_bw, FULL_BW, max_ch_bw) &&
                !IEEE80211_IS_FLAG_HALF(halfquarter) &&
                !IEEE80211_IS_FLAG_QUARTER(halfquarter))
                ch_width = CH_WIDTH_20MHZ;
            else if (BW_WITHIN(min_ch_bw, HALF_BW, max_ch_bw) &&
                     IEEE80211_IS_FLAG_HALF(halfquarter))
                ch_width = CH_WIDTH_10MHZ;
            else if (BW_WITHIN(min_ch_bw, QRTR_BW, max_ch_bw) &&
                     IEEE80211_IS_FLAG_QUARTER(halfquarter))
                ch_width = CH_WIDTH_5MHZ;
            else {
                qdf_debug("Mode (%u) not supported by device for freq %u",
                          mode, primary_freq);
                return -EINVAL;
            }
            /* For 4.9 Ghz channels the regulatory support for the bandwidth
             * is already done by 'min_ch_bw' and 'max_ch_bw'. Therefore, skip
             * the following bandwidth check (in function
             * 'wlan_reg_is_phymode_chwidth_allowed').
             */
            goto exit;
        }

        if (ch_width == CH_WIDTH_80P80MHZ && max_bw_5g < BW_160_MHZ)
            return -EINVAL;

        if (!wlan_reg_is_phymode_chwidth_allowed(ic->ic_pdev_obj,
                                                 r_phymode,
                                                 ch_width,
                                                 primary_freq,
                                                 in_6g_pwr_mode,
                                                 input_puncture_bitmap)) {
            qdf_debug("Mode (%d) not supported by device for freq %d",
                      mode, primary_freq);
            return -EINVAL;
        }
    }

exit:
    mode_info->ch_width = ch_width;
    mode_info->r_phymode = r_phymode;
    mode_info->combined_mode = mode;
    return 0;
}

/**
 * ieee80211_find_des_cfreq2() - Find the secondary frequencies for 80P80MHz.
 * @ic: Pointer to ieee80211com.
 * @ch_params: channel params to be filled.
 * @secondary_freq: Input secondary center frequency.
 */
static int ieee80211_find_des_cfreq2(struct ieee80211com *ic,
                                     struct ch_params *ch_params,
                                     qdf_freq_t secondary_freq)
{
    if (secondary_freq) {
        ch_params->mhz_freq_seg1 = secondary_freq;
    }

    if (ch_params->mhz_freq_seg1)
        ch_params->center_freq_seg1 = wlan_reg_freq_to_chan(ic->ic_pdev_obj,
                                                            ch_params->mhz_freq_seg1);
    return 0;
}

/**
 * ieee80211_get_combined_mode() - Get combined mode from channel width
 * and phymode.
 * @chwidth: Input channel width enum.
 * @r_phymode: Input phymode (80211 modes).
 * @primary_freq: Primary frequency of the channel.
 */
static enum ieee80211_phymode
ieee80211_get_combined_mode(enum phy_ch_width chwidth,
                            enum reg_phymode r_phymode,
                            qdf_freq_t primary_freq)
{
    return ieee80211_find_mode_from_chwidth_and_regphymode(r_phymode,
                                                           chwidth,
                                                           primary_freq);
}

static int
ieee80211_is_proper_ht40(enum ieee80211_phymode mode,
                         uint8_t sec_ch_offset)
{
    switch (mode) {
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11AC_VHT40PLUS:
        case IEEE80211_MODE_11AXA_HE40PLUS:
        case IEEE80211_MODE_11AXG_HE40PLUS:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT40PLUS:
        case IEEE80211_MODE_11BEG_EHT40PLUS:
#endif
            if (sec_ch_offset == HIGH_PRIMARY_CH)
                return -EINVAL;
            break;
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11AC_VHT40MINUS:
        case IEEE80211_MODE_11AXA_HE40MINUS:
        case IEEE80211_MODE_11AXG_HE40MINUS:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT40MINUS:
        case IEEE80211_MODE_11BEG_EHT40MINUS:
#endif
            if (sec_ch_offset == LOW_PRIMARY_CH)
                return -EINVAL;
            break;
        default:
            break;
    }

    return 0;
}

/**
 * ieee80211_fill_ch_param_seg1_to_chan_list() - Fill out_ch_params
 * structure with seg1 frequency/IEEE. This seg1 value is used as an input
 * wlan_reg_fill_channel_list_for_pwrmode to compute the channel params.
 * @in_ch_params: Pointer to struct ch_params
 * @out_ch_params: Pointer to struct ch_params
 */

#ifdef WLAN_FEATURE_11BE
static void
ieee80211_fill_ch_param_seg1_to_chan_list(struct ch_params *out_ch_params,
                                          struct ch_params *in_ch_params)
{

        out_ch_params->center_freq_seg1 = in_ch_params->center_freq_seg1;
        out_ch_params->mhz_freq_seg1 = in_ch_params->mhz_freq_seg1;

}
#else
static void
ieee80211_fill_ch_param_seg1_to_chan_list(struct ch_params *out_ch_params,
                                          struct ch_params *in_ch_params)
{
}
#endif

#ifdef WLAN_FEATURE_11BE
static void
ieee80211_fill_ch_param_pp_to_chan_list(struct reg_channel_list *chan_list,
                                        struct ch_params *in_ch_params)
{
    uint8_t i;

    for (i = 0; i < MAX_NUM_CHAN_PARAM; i++) {
        chan_list->chan_param[i].input_punc_bitmap = in_ch_params->input_punc_bitmap;
    }
}
#endif

/* ieee80211_find_chan_with_ch_params(): Find a channel for the given primary,
 * secondary frequencies and mode. Fill the ch_params structure and return to
 * the caller.
 * Return - 0 on success and error if a channel could not be found.
 * @ic: Pointer to struct ieee80211com
 * @ch_params: Pointer to  struct ch_params to be filled.(O/P)
 * @mode: User given phymode. (I/P)
 * @primary_freq: User configured primary frequency in MHZ.(I/P)
 * @secondary_freq: User configured secondary frequency in MHZ.
 * @in_6g_pwr_mode: Input 6G power type which will determine the 6G channel
 * list on which the primary_freq is searched for.
 *
 */
static int
ieee80211_find_chan_with_ch_params(struct ieee80211com *ic,
                                   struct ch_params *ch_params,
                                   struct ieee80211_mode_info *mode_info,
                                   qdf_freq_t primary_freq,
                                   qdf_freq_t secondary_freq,
                                   enum supported_6g_pwr_types in_6g_pwr_mode)
{
    qdf_freq_t sec_ch_2g_freq = 0;
    bool is_auto_mode = mode_info->is_auto;
    enum ieee80211_phymode out_mode = mode_info->combined_mode;
    int status;
    enum phy_ch_width in_width = mode_info->ch_width;
    struct wlan_objmgr_psoc *psoc;
#ifdef WLAN_FEATURE_11BE
    struct reg_channel_list chan_list = {0};
    qdf_freq_t center_320;
#endif

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        qdf_err("%s : psoc is null", __func__);
        return -EINVAL;
    }

    ch_params->ch_width = in_width;

    /* In case of 2G channel and width of HT40, fill the sec_ch_2g_freq.
     * sec_ch_2g_freq will be an input to reg_set_channel_params_for_pwrmode().
     */
    if (in_width == CH_WIDTH_40MHZ &&
        WLAN_REG_IS_24GHZ_CH_FREQ(primary_freq))
        sec_ch_2g_freq =
            ieee80211_fill_sec_2g_freq(primary_freq, mode_info->combined_mode);

    if (in_width == CH_WIDTH_80P80MHZ) {
        status = ieee80211_find_des_cfreq2(ic, ch_params, secondary_freq);
        if (status)
            return status;
    }

    /* ch_params is both input and output variable. The API fills the target
     * channel params in ch_params and returns.
     * Also, the current implementation of ieee80211_regdmn_get_channel_params()
     * does a fallback to lower width if a channel of given width is not
     * available. Hence we need to check the input and output width.
     */
    if  (!WLAN_REG_IS_49GHZ_FREQ(primary_freq) ) {
        if (ch_params->ch_width == CH_WIDTH_80P80MHZ &&
            !ch_params->mhz_freq_seg1) {
            if(!ieee80211_is_80p80mhz_supported(ic, primary_freq,
                                                &ch_params->mhz_freq_seg1,
                                                &ch_params->center_freq_seg1,
                                                true,
                                                in_6g_pwr_mode))
                return -EINVAL;
        }
#ifdef WLAN_FEATURE_11BE
        center_320 = (ch_params->ch_width == CH_WIDTH_320MHZ) ?
                      secondary_freq : 0;
        ieee80211_fill_ch_param_seg1_to_chan_list(&chan_list.chan_param[0],
                                                  ch_params);
        ieee80211_fill_ch_param_pp_to_chan_list(&chan_list, ch_params);
        ieee80211_regdmn_reg_fill_channel_list(ic->ic_pdev_obj,
                                               primary_freq, sec_ch_2g_freq,
                                               ch_params->ch_width,
                                               center_320,
                                               &chan_list,
                                               in_6g_pwr_mode, false);
        ieee80211_copy_ch_params(ch_params, &chan_list.chan_param[0]);
#else
        ieee80211_regdmn_get_channel_params(ic->ic_pdev_obj,
                                            primary_freq,
                                            sec_ch_2g_freq,
                                            ch_params,
                                            in_6g_pwr_mode);
#endif

        if (ch_params->ch_width == CH_WIDTH_80P80MHZ) {
            if (abs(ch_params->mhz_freq_seg0 - ch_params->mhz_freq_seg1) <= 80)
                return -EINVAL;
            if (wlan_psoc_nif_fw_ext_cap_get(psoc,
                                             WLAN_SOC_RESTRICTED_80P80_SUPPORT) &&
                !(CHAN_WITHIN_RESTRICTED_80P80(ch_params->mhz_freq_seg0,
                                               ch_params->mhz_freq_seg1)))
                return -EINVAL;
        }
    } else {
        /* Regulatory set channel params does not work for 4.9G. */
        ch_params->center_freq_seg0 = wlan_reg_freq_to_chan(ic->ic_pdev_obj,
                                                            primary_freq);
        ch_params->center_freq_seg1 = 0;
        ch_params->mhz_freq_seg0 = primary_freq;
        ch_params->mhz_freq_seg1 = 0;
    }

    if (!ch_params->mhz_freq_seg0) {
        qdf_debug("Unable to find a channel with the given input params");
        qdf_debug("primary_freq = %d, channel width = %d", primary_freq,
                  in_width);
        return -EINVAL;
    }

    if ((in_width != ch_params->ch_width) && !is_auto_mode) {
        qdf_debug("Channel could not found in user requested width");
        qdf_debug("input Width = %d, output Width = %d",
                  in_width, ch_params->ch_width);
        return -EINVAL;
    }

    if (!is_auto_mode) {
        status = ieee80211_is_proper_ht40(out_mode, ch_params->sec_ch_offset);
        if (status)
            return status;
    }

    if (is_auto_mode)
        out_mode = ieee80211_get_combined_mode(ch_params->ch_width,
                                               mode_info->r_phymode,
                                               primary_freq);

    if (out_mode == IEEE80211_MODE_MAX)
        return -EINVAL;

    /*
     * The function ieee80211_conv_mode_for_ht40 is pass-through for
     * non-40 modes
     */
    out_mode = ieee80211_conv_mode_for_40modes(out_mode, primary_freq,
                                            ch_params->mhz_freq_seg0);

    mode_info->combined_mode = out_mode;
    return 0;
}

/**
 * regdmn_apply_punc_pattern() - Apply the regulatory puncturing pattern
 * to ic channel.
 * @icv: Pointer to ieee80211_ath_channel
 * @ch_params: Pointer to ch_params
 *
 * Return: None
 */
#ifdef WLAN_FEATURE_11BE
static inline void
regdmn_apply_punc_pattern(struct ieee80211_ath_channel *icv,
                          struct ch_params *ch_params)
{
    icv->reg_punc_bitmap = ch_params->reg_punc_bitmap;
}
#else
static inline void
regdmn_apply_punc_pattern(struct ieee80211_ath_channel *icv,
                          struct ch_params *ch_params)
{
}
#endif

/* ieee80211_fill_ath_channel_from_ch_params() - Fill ieee80211_ath_channel
 * structure from ch_params.
 * @ic: Pointer to struct ieee80211com.
 * @ch_params: Pointer to struct ch_params.
 * @combined_mode: User configured phymode.
 * @primary_freq: User configured frequency in MHZ.
 * @chan: Pointer to ieee80211_ath_channel to be filled.(O/P)
 * @in_6g_pwr_mode: Input 6G power type which will determine the 6G channel
 *                  list on which the primary_freq is searched for.
 * @reg_chan_list: regulatory channel list.
 */
void
ieee80211_fill_ath_channel_from_ch_params(struct ieee80211com *ic,
                                          struct ch_params ch_params,
                                          enum ieee80211_phymode combined_mode,
                                          qdf_freq_t primary_freq,
                                          struct ieee80211_ath_channel *chan,
                                          enum supported_6g_pwr_types in_6g_pwr_mode,
                                          struct regulatory_channel *reg_chan_list)
{
    uint64_t band_flag = ieee80211_get_band_flag(primary_freq);
    uint32_t txpower = 0;
    uint8_t ant_gain = 0;
    uint16_t flags_ext = 0;
    qdf_freq_t sec_freq = 0;
    int8_t max_tx_power = 0, min_tx_power = 0;

    chan->ic_freq = primary_freq;

    chan->ic_ieee = reg_chan_list->chan_num;
    chan->ic_vhtop_ch_num_seg1 = ch_params.center_freq_seg0;
    chan->ic_vhtop_ch_num_seg2 = ch_params.center_freq_seg1;
    chan->ic_vhtop_freq_seg1 = ch_params.mhz_freq_seg0;
    chan->ic_vhtop_freq_seg2 = ch_params.mhz_freq_seg1;
#ifdef WLAN_FEATURE_11BE
    chan->reg_punc_bitmap = ch_params.reg_punc_bitmap;
#endif

    if (ch_params.ch_width == CH_WIDTH_5MHZ)
        chan->ic_flags |= IEEE80211_CHAN_QUARTER;

    if (ch_params.ch_width == CH_WIDTH_10MHZ)
        chan->ic_flags |= IEEE80211_CHAN_HALF;
    /*
     * Get the txpower and antenna gain for the primary frequency from
     * regulatory.
     */
    wlan_reg_get_txpow_ant_gain(ic->ic_pdev_obj, primary_freq, &txpower,
                                &ant_gain,
                                reg_chan_list);
    chan->ic_maxregpower = txpower;
    chan->ic_antennamax = ant_gain;

    if (ic->ic_get_min_and_max_power)
            ic->ic_get_min_and_max_power(ic, &max_tx_power, &min_tx_power);

    chan->ic_maxpower = max_tx_power;
    chan->ic_minpower = min_tx_power;

    /* Filling channel flags from mode and band. */
    chan->ic_flags |= ieee80211_mode_to_chanflags(combined_mode) | band_flag;

#define FREQ_OFFSET_10MHZ 10
    /* Find the flags_ext from regulatory (DFS/PSC etc.). */
    if (combined_mode == IEEE80211_MODE_11AXA_HE160 ||
        combined_mode == IEEE80211_MODE_11AC_VHT160 ||
        combined_mode == IEEE80211_MODE_11AXA_HE80_80 ||
        combined_mode == IEEE80211_MODE_11AC_VHT80_80
#ifdef WLAN_FEATURE_11BE
        ||
        combined_mode == IEEE80211_MODE_11BEA_EHT160 ||
        combined_mode == IEEE80211_MODE_11BEA_EHT320
#endif
        ) {
        if (primary_freq > ch_params.mhz_freq_seg1)
            sec_freq = ch_params.mhz_freq_seg1 - FREQ_OFFSET_10MHZ;
        else
            sec_freq = ch_params.mhz_freq_seg1 + FREQ_OFFSET_10MHZ;
    } else if (ch_params.mhz_freq_seg1) {
        sec_freq = ch_params.mhz_freq_seg1 - FREQ_OFFSET_10MHZ;
    }

    wlan_reg_get_chan_flags(ic->ic_pdev_obj, primary_freq, sec_freq,
                            &flags_ext, &chan->ic_flags,in_6g_pwr_mode,
                            reg_chan_list);

    /*
     * The flags in the component dev and flags in the umac are one-to-one
     * mapped.
     * e.g.-  WLAN_CHAN_DFS_CFREQ2 <-> IEEE80211_CHAN_DFS_CFREQ2
     */
    chan->ic_flagext = flags_ext;
    regdmn_apply_punc_pattern(chan, &ch_params);
#undef FREQ_OFFSET_10MHZ
}

/**
 * ieee80211_reg_find_a_valid_freq_based_on_mode() - Find the first valid freq
 * from the cur chan list based on mode.
 * @pdev: Pointer to pdev.
 * @in_6g_pwr_mode: Input 6g power mode.
 * @first_valid_freq: Pointer to channel center frequency output.
 * @bw: Bandwidth.
 * @sec_40_offset: 40 MHz channel's secondary offset
 * @mode: Input phymode.
 *
 * Return: None
 */
static void
ieee80211_reg_find_a_valid_freq_based_on_mode(struct wlan_objmgr_pdev *pdev,
                                              enum supported_6g_pwr_types
                                              in_6g_pwr_mode,
                                              qdf_freq_t *first_valid_freq,
                                              int bw, int sec_40_offset,
                                              enum ieee80211_phymode mode)
{
    enum channel_enum start_chan, end_chan;

    if (ieee80211_is_phymode_2g(mode)) {
        start_chan = MIN_24GHZ_CHANNEL;
        end_chan = MAX_24GHZ_CHANNEL;
    } else if (ieee80211_is_phymode_5g_or_6g(mode)) {
        start_chan = BAND_5GHZ_START_CHANNEL;
        end_chan = MAX_6GHZ_CHANNEL;
    } else {
        start_chan = 0;
        end_chan = NUM_CHANNELS - 1;
    }

    wlan_reg_get_first_valid_freq(pdev, in_6g_pwr_mode, first_valid_freq, bw,
                                  sec_40_offset, start_chan, end_chan);
}

qdf_freq_t
ieee80211_reg_find_first_valid_freq(struct ieee80211com *ic,
                                    enum ieee80211_phymode mode,
                                    enum supported_6g_pwr_types in_6g_pwr_mode)
{
    qdf_freq_t first_valid_freq = 0;
    int8_t sec_40_offset = 0;
    int bw = 0;

    if (mode != IEEE80211_MODE_AUTO) {
        enum phy_ch_width ch_width = phymode_decoupler[mode][CH_WIDTH];
        enum reg_phymode r_phymode = phymode_decoupler[mode][R_PHYMODE];

        if(r_phymode == REG_PHYMODE_11A) {
            if (IEEE80211_IS_FLAG_HALF(ic->ic_chanbwflag))
                ch_width = CH_WIDTH_10MHZ;
            else if (IEEE80211_IS_FLAG_QUARTER(ic->ic_chanbwflag))
                ch_width = CH_WIDTH_5MHZ;
        }

        bw = ieee80211_get_chwidth_val(ch_width);
    }

    if (ieee80211_is_phymode_40plus(mode)) {
        sec_40_offset = CHAN_HT40_OFFSET;
    } else if (ieee80211_is_phymode_40minus(mode)) {
        sec_40_offset = -CHAN_HT40_OFFSET;
    }

    ieee80211_reg_find_a_valid_freq_based_on_mode(ic->ic_pdev_obj,
                                                  in_6g_pwr_mode,
                                                  &first_valid_freq,
                                                  bw, sec_40_offset,
                                                  mode);

    /*
     * If we cannot find a valid frequency in the given 2 or 5/6 GHz band,
     * try to search for a frequency in auto mode in any band.
     */
    if (!first_valid_freq) {
        if (mode == IEEE80211_MODE_AUTO) {
            qdf_rl_debug("Unable to find any valid channel in Auto Mode in channel list");
            return first_valid_freq;
        }

        qdf_rl_debug("Trying to find a valid channel in auto mode in any band "
                "from current channel list.");
        qdf_rl_debug("Input phymode: %d Input 6g pwr mode: %d", mode, in_6g_pwr_mode);
        ieee80211_reg_find_a_valid_freq_based_on_mode(ic->ic_pdev_obj,
                                                      in_6g_pwr_mode,
                                                      &first_valid_freq,
                                                      bw, sec_40_offset,
                                                      IEEE80211_MODE_AUTO);
    }

    return first_valid_freq;
}


/**
 * regdmn_set_default_punc_pat() - Apply default puncturing pattern to
 * chan params. By default, all the sub-channels of a given mode are
 * enabled indicated by NO_PUNCTURE_BITMASK (all 0s)
 *
 *@ch_params: Pointer to struct ch_params.
 *@input_puncture_bitmap: Input puncture bitmap
 */
#ifdef WLAN_FEATURE_11BE
static inline void regdmn_set_default_punc_pat(struct ch_params *ch_params,
                                               uint16_t input_puncture_bitmap)
{

    ch_params->reg_punc_bitmap = NO_PUNCTURE_BITMASK;
    ch_params->input_punc_bitmap = input_puncture_bitmap;

}
#else
static inline void regdmn_set_default_punc_pat(struct ch_params *ch_params,
                                               uint16_t input_puncture_bitmap)
{
}
#endif

#ifdef WLAN_FEATURE_11BE
static const uint16_t valid_puncture_pattern_80[]  = {0x1, 0x2, 0x4, 0x8, 0x0};
static const uint16_t valid_puncture_pattern_160[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x03, 0x0C, 0x30, 0xC0, 0x0};
static const uint16_t valid_puncture_pattern_320[] = {0x0003, 0x000C, 0x0030, 0x00C0, 0x0300, 0x0C00, 0x3000, 0xC000, 0x000F, 0x00F0, 0x0F00, 0xF000, 0xF003, 0xF00C, 0xF030, 0xF0C0, 0xF300, 0xFC00, 0x003F, 0x00CF, 0x030F, 0x0C0F, 0x300F, 0xC00F, 0x0};

bool validate_primary_channel_in_pp(uint16_t pp, struct ieee80211_ath_channel *chan)
{
   uint8_t center_chan = 0;
   uint8_t start_chan = 0;
   uint32_t pri_chan_bit = 0;
   uint32_t bw = 0;

   if (!chan) {
       qdf_err("chan is NULL");
       return 1;
   }

   bw = ieee80211_get_chan_width(chan);

   center_chan = chan->ic_vhtop_ch_num_seg2 ? chan->ic_vhtop_ch_num_seg2 : chan->ic_vhtop_ch_num_seg1;

   start_chan = center_chan - (((bw/40) - 1) * 4) - 2;

   pri_chan_bit = ((chan->ic_ieee - start_chan) >> 2);

   if ((1 << pri_chan_bit) & pp) {
       return 1;
   }

   return 0;
}

uint16_t const *get_valid_puncture_pattern_arr(struct ieee80211_ath_channel *chan, int *len, uint16_t *mask)
{
    uint16_t const *pp_ptr = NULL;

    switch (chan->ic_flags & IEEE80211_CHAN_WIDTH_MASK) {
        case IEEE80211_CHAN_320MHZ:
            pp_ptr = valid_puncture_pattern_320;
            *len = QDF_ARRAY_SIZE(valid_puncture_pattern_320);
            *mask = PUNCTURE_320MHZ_MASK;
            break;
        case IEEE80211_CHAN_160MHZ:
            pp_ptr = valid_puncture_pattern_160;
            *len = QDF_ARRAY_SIZE(valid_puncture_pattern_160);
            *mask = PUNCTURE_160MHZ_MASK;
            break;
        case IEEE80211_CHAN_80MHZ:
            pp_ptr = valid_puncture_pattern_80;
            *len = QDF_ARRAY_SIZE(valid_puncture_pattern_80);
            *mask = PUNCTURE_80MHZ_MASK;
            break;
        default:
            return NULL;
    }
    return pp_ptr;
}

uint16_t derive_bw_static_puncture_pattaren(uint16_t pp, struct ieee80211_ath_channel *chan)
{
    uint16_t const *pp_ptr = NULL;
    int len;
    uint16_t mask = PUNCTURE_NONE;

    pp_ptr = get_valid_puncture_pattern_arr(chan, &len, &mask);

    return (pp & mask);
}

#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
int8_t ieee80211_check_punc_pattern_power(struct ieee80211com *ic,
                                          uint16_t input_pp,
                                          struct ieee80211_ath_channel *chan)
{
    uint16_t bw;
    qdf_freq_t cen_freq;
    int8_t eirp = CHAN_MIN_EIRP_POWER;

    /* Get EIRP for the PP and compare with threshold, if above tx power threshold
     * return the power otherwise 0 */
    if ((ieee80211_get_chan_width(chan) >= CHWIDTH_80)
            && ieee80211_is_afc_capable(ic, ieee80211_chan2mode(chan))) {

        bw = ieee80211_get_chan_width(chan);
        cen_freq = ieee80211_get_chan_centre_freq(ic, chan);

        /* Get the EIRP power with provided input puncture pattern*/
        eirp = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj, chan->ic_freq,
                                     cen_freq, bw,
                                     REG_STANDARD_POWER_AP, input_pp,
                                     false, REG_MAX_CLIENT_TYPE, false);

        if (eirp > ic->eirp_threshold) {
            qdf_debug("freq %d cfreq %d bw %d pp %x eirp %d",
                     chan->ic_freq, cen_freq, bw, input_pp, eirp);
            return eirp;
        }
    }

    return eirp;
}

#define NUM_20PP_PUNC_80MHZ 4
#define NUM_20PP_PUNC_160MHZ 8
#define NUM_40PP_PUNC_160MHZ 4
#define NUM_40PP_PUNC_320MHZ 8
#define NUM_80PP_PUNC_320MHZ 4
#define NUM_40P80PP_PUNC_320MHZ 12
static void get_num_puncture_types(uint16_t bw,
        uint8_t *num_punc_type1, uint8_t *num_punc_type2, uint8_t *num_punc_type3)
{
    *num_punc_type1 = 0;
    *num_punc_type2 = 0;
    *num_punc_type3 = 0;

    switch (bw) {
        case CHWIDTH_320:
            *num_punc_type1 = NUM_40PP_PUNC_320MHZ;
            *num_punc_type2 = NUM_80PP_PUNC_320MHZ;
            *num_punc_type3 = NUM_40P80PP_PUNC_320MHZ;
            break;
        case CHWIDTH_160:
            *num_punc_type1 = NUM_20PP_PUNC_160MHZ;
            *num_punc_type2 = NUM_40PP_PUNC_160MHZ;
            break;
        case CHWIDTH_80:
            *num_punc_type1 = NUM_20PP_PUNC_80MHZ;
            break;
    }
}
#endif

bool validate_static_puncture_pattern(struct ieee80211com *ic, uint16_t pp,
       struct ieee80211_ath_channel *chan, struct chan_params *chan_p)
{
    uint16_t const *pp_ptr = NULL;
    int i;
    int len;
    uint16_t mask = PUNCTURE_NONE;

    pp_ptr = get_valid_puncture_pattern_arr(chan, &len, &mask);
    if (!pp_ptr)
        return 0;

#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
    if (IEEE80211_IS_CHAN_6GHZ(chan) &&
        ((chan_p->input_6g_pwr_mode == REG_BEST_PWR_MODE) || (chan_p->input_6g_pwr_mode == REG_AP_SP)) &&
        !chan_p->puncture_strict && chan_p->puncture_low_pwr) {
        return 1;
    }
#endif

    /* Intersect input puncture pattern with legitimate patterns
     * Due to uniqueness pf valid puncture patterns intersection
     * will give nearest best possible puncture pattern.
     */
    for  (i = 0; i < len; i++) {
        if (pp_ptr[i] == (pp & mask)) {
            return 0;
        }
    }

    return 1;
}

#if defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
int16_t
ieee80211_find_eirp_pwr_for_6ghz_pwr_modes(struct ieee80211com *ic,
                                           struct ieee80211_ath_channel *chan,
                                           int bw,
                                           enum supported_6g_pwr_types in_6g_pwr_mode,
                                           uint16_t pp,
                                           int16_t *sp_eirp,
                                           int16_t *non_sp_eirp)
{
    qdf_freq_t cen_freq;

    cen_freq = ieee80211_get_chan_centre_freq(ic, chan);
    *sp_eirp = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj, chan->ic_freq, cen_freq, bw,
                                     REG_STANDARD_POWER_AP , pp, false, REG_MAX_CLIENT_TYPE, false);
    if (in_6g_pwr_mode == REG_BEST_PWR_MODE) {
        *non_sp_eirp = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj, chan->ic_freq, cen_freq, bw,
                                             REG_INDOOR_AP, pp, false, REG_MAX_CLIENT_TYPE, false);

        if (*non_sp_eirp == 0) {
            *non_sp_eirp = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj, chan->ic_freq, cen_freq, bw,
                                                 REG_VERY_LOW_POWER_AP, pp, false, REG_MAX_CLIENT_TYPE, false);

        }
    } else {
        *non_sp_eirp = 0;
    }

    if (*sp_eirp)
        return *sp_eirp;

    return *non_sp_eirp;
}
#endif

uint16_t derive_nearest_static_puncture_pattern(struct ieee80211com *ic, uint16_t pp,
               struct ieee80211_ath_channel *chan, struct chan_params *chan_p)
{
    uint16_t const *pp_ptr = NULL;
    uint16_t mask = PUNCTURE_NONE;
    uint16_t pp_final;
    int i;
    int len;
#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
    int16_t eirp, initial_eirp = CHAN_MIN_EIRP_POWER;
    int16_t eirp_tmp, sp_eirp = CHAN_MIN_EIRP_POWER, non_sp_eirp = CHAN_MIN_EIRP_POWER;
    uint8_t punc_type1;
    uint8_t punc_type2;
    uint8_t punc_type3;
    int bw;
#endif

    pp_ptr = get_valid_puncture_pattern_arr(chan, &len, &mask);
    if (!pp_ptr)
       return PUNCTURE_INVALID;

    /* Initialize final PP to invalid only if PP is provided */
    pp_final = pp ? PUNCTURE_INVALID : pp;

    /* This is to derive a valid puncture pattern if the user puncture
     * pattern is invalid.
     */
    if (pp_final == PUNCTURE_INVALID) {
        /* Record the first valid PP, we need to use this PP
         * if best power PP intersection is not possible
         */
        for  (i = 0; i < len; i++) {
            if (pp_ptr[i] == ((pp | pp_ptr[i]) & mask) &&
                    !validate_primary_channel_in_pp(pp_ptr[i], chan)) {
                pp = pp_ptr[i];
                pp_final = pp_ptr[i];
                break;
            }
        }
    }

    if (pp_final == PUNCTURE_INVALID)
        return PUNCTURE_INVALID;

#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
    bw = ieee80211_get_chan_width(chan);
    get_num_puncture_types(bw, &punc_type1, &punc_type2, &punc_type3);

    /* If best power PP is requested, then find best power PP with minimal
     * possible puncture which intersects with input PP */
    if (IEEE80211_IS_CHAN_6GHZ(chan) &&
        ((chan_p->input_6g_pwr_mode == REG_BEST_PWR_MODE) || (chan_p->input_6g_pwr_mode == REG_AP_SP)) &&
        !chan_p->puncture_strict && chan_p->puncture_low_pwr) {

        initial_eirp = ieee80211_find_eirp_pwr_for_6ghz_pwr_modes(ic,
                                                                  chan,
                                                                  bw,
                                                                  chan_p->input_6g_pwr_mode,
                                                                  pp,
                                                                  &sp_eirp,
                                                                  &non_sp_eirp);

        if (initial_eirp == 0) {
            pp_final = PUNCTURE_INVALID;
            qdf_err("invalid pp in all modes");
        }

        /* If user configured EIRP threshold is valid, use it for final comparison,
         * or else use calculated initial EIRP.
         */
        if (ic->eirp_threshold) {
            /* If SP EIRP is greater than configured threshold, return input pp*/
            if (sp_eirp > ic->eirp_threshold)
                return pp;
            initial_eirp = ic->eirp_threshold;
        }

        eirp = initial_eirp;
        for  (i = 0; i < len; i++) {
            if (pp_ptr[i] == ((pp | pp_ptr[i]) & mask)
                && !validate_primary_channel_in_pp(pp_ptr[i], chan)) {

                eirp_tmp = ieee80211_check_punc_pattern_power(ic, pp_ptr[i], chan);
                if (eirp_tmp > eirp) {
                    eirp = eirp_tmp;
                    pp_final = pp_ptr[i];
                }
                if(((i == (punc_type1 - 1))
                    || (i == (punc_type1 + punc_type2 - 1))
                    || (i == (len - 2)))
                   && (eirp > initial_eirp)) {
                    break;
                }
            }
        }

        if (eirp > initial_eirp)
            qdf_info("Freq %d BW %d input PP %d final PP %x EIRP %d initial_eirp %d",
                     chan->ic_freq, bw, pp, pp_final, eirp, initial_eirp);

        if (chan_p->input_6g_pwr_mode == REG_BEST_PWR_MODE) {
            QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_DEBUG,
                      "Input pp %x Initial EIRP %d SP Eirp %d non-SP EIRP %d"
                      " Final EIRP %d Final pp %x ",
                      pp, initial_eirp, sp_eirp, non_sp_eirp, eirp, pp_final);
        }

        if (pp_final == PUNCTURE_INVALID)
            return PUNCTURE_INVALID;

        if ((chan_p->input_6g_pwr_mode == REG_AP_SP) ||
                ((eirp > initial_eirp) && (eirp > non_sp_eirp))) {
            return pp_final;
        }
    }
#endif

    return pp;
}

uint16_t ieee80211_find_legitimate_punture_pattern(struct ieee80211com *ic, uint16_t pp,
           struct ieee80211_ath_channel *chan, struct chan_params *chan_p)
{
    pp = derive_bw_static_puncture_pattaren(pp, chan);

    /* check if primary channel is not punctured */
    if (validate_primary_channel_in_pp(pp, chan)) {
        return PUNCTURE_INVALID;
    }

    if (validate_static_puncture_pattern(ic, pp, chan, chan_p)) {
        /* if pp is not valid derive nearest pattern */
        pp = derive_nearest_static_puncture_pattern(ic, pp, chan, chan_p);
    }

    return pp;
}

/**
 * ieee80211_find_puncture_bitmap: finds operating puncture_bitmap for channel with input as chan_p
 * chan: input channel
 * chan_p: chan_params for finding the puncture_bitmap
 *
 * return: returns channel with updated operating puncture_bitmap
 */
struct ieee80211_ath_channel
ieee80211_find_puncture_bitmap(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, struct chan_params *chan_p)
{
    uint16_t pp = 0;
    uint16_t leg_pp = 0;

    if (!IEEE80211_IS_CHAN_11BE(chan)) {
        chan_p->op_puncture_bitmap = PUNCTURE_INVALID;
        chan->op_puncture_bitmap = PUNCTURE_INVALID;
        return *chan;
    }

    if (IEEE80211_IS_CHAN_20MHZ(chan) || IEEE80211_IS_CHAN_40MHZ(chan)) {
	chan_p->op_puncture_bitmap = PUNCTURE_NONE;
	chan->op_puncture_bitmap = PUNCTURE_NONE;
	return *chan;
    }

    if (chan_p->op_puncture_bitmap &&
        (chan_p->op_puncture_bitmap != PUNCTURE_INVALID)) {
        /* puncturing already derived */
        chan->op_puncture_bitmap = chan_p->op_puncture_bitmap;
        return *chan;
    }

    if (!chan_p->puncture_bitmap) {
        pp = PUNCTURE_NONE;
    } else {
        pp = chan_p->puncture_bitmap;
    }

    pp |= chan->reg_punc_bitmap;

    /* Phase1 derive legitimate puncture pattern */
    leg_pp  = ieee80211_find_legitimate_punture_pattern(ic, pp, chan, chan_p);

    if (leg_pp == PUNCTURE_INVALID) {
        qdf_rl_debug("Invalid/unsupported pp %x", pp);
        return empty_channel;
    }

    if (chan_p->puncture_strict && (leg_pp != pp)) {
        qdf_rl_debug("Strict puncturing enabled - "
                "Legitimate puncture pattern is different from input - "
                "input: %x, legitimate: %x", pp, leg_pp);
        return empty_channel;
    }

    chan_p->op_puncture_bitmap = leg_pp;
    chan->op_puncture_bitmap = leg_pp;

    return *chan;
}

/**
 * ieee80211_find_dot11_channel_pp: finds channel along with operating puncture pattern
 * ic: Pointer to ic
 * chan_p: chan_params for finding the channel
 * mode: phymode oof the channel
 * return: returns channel with operating puncture_bitmap
 */
struct ieee80211_ath_channel
ieee80211_find_dot11_channel(struct ieee80211com *ic, struct chan_params *chan_p, enum ieee80211_phymode mode)
{
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;
    uint16_t afc_pp = 0;

    if (!chan_p) {
        return empty_channel;
    }

    if ((chan_p->input_6g_pwr_mode == REG_AP_SP) && ieee80211_is_phymode_11be(mode)) {
        afc_pp = get_afc_puncture_bimtap(ic, chan_p, mode);
        chan_p->puncture_bitmap |= afc_pp;
    }

    *chan = ieee80211_find_dot11_channel_internal(ic, chan_p->freq, chan_p->cfreq2, mode, chan_p->input_6g_pwr_mode, chan_p->puncture_bitmap);

    if (!IEEE80211_IS_CHAN_VALID(chan)) {
	return empty_channel;
    }

    chan_p->puncture_strict = ic->ic_puncture_strict;
    *chan = ieee80211_find_puncture_bitmap(ic, chan, chan_p);

    return *chan;
}

uint16_t ieee80211_intersect_puncture_bitmap(struct ieee80211_ath_channel *bss_chan,
        enum ieee80211_phymode des_mode, enum ieee80211_phymode bss_mode)
{
    if (bss_chan->op_puncture_bitmap == PUNCTURE_INVALID) {
        return PUNCTURE_NONE;
    }

    switch (des_mode) {
        case IEEE80211_MODE_11BEA_EHT320:
            return bss_chan->op_puncture_bitmap;
        case IEEE80211_MODE_11BEA_EHT160:
            if (bss_mode == IEEE80211_MODE_11BEA_EHT320) {
                if (bss_chan->ic_freq > bss_chan->ic_vhtop_freq_seg2) {
                    return (bss_chan->op_puncture_bitmap >> 8) & PUNCTURE_160MHZ_MASK;
                }
            }
            return bss_chan->op_puncture_bitmap & PUNCTURE_160MHZ_MASK;
        case IEEE80211_MODE_11BEA_EHT80:
            if (bss_mode == IEEE80211_MODE_11BEA_EHT320) {
                if (bss_chan->ic_freq > bss_chan->ic_vhtop_freq_seg2) {
                    if (bss_chan->ic_freq > bss_chan->ic_vhtop_freq_seg1) {
                        return (bss_chan->op_puncture_bitmap >> 12) & PUNCTURE_80MHZ_MASK;
                    } else {
                        return (bss_chan->op_puncture_bitmap >> 8) & PUNCTURE_80MHZ_MASK;
                    }
                } else if (bss_chan->ic_freq > bss_chan->ic_vhtop_freq_seg1) {
                    return (bss_chan->op_puncture_bitmap >> 4) & PUNCTURE_80MHZ_MASK;
                }
            } else if (bss_mode == IEEE80211_MODE_11BEA_EHT160) {
                if (bss_chan->ic_freq > bss_chan->ic_vhtop_freq_seg2) {
                    return (bss_chan->op_puncture_bitmap >> 4) & PUNCTURE_80MHZ_MASK;
                }
            }

            return bss_chan->op_puncture_bitmap & PUNCTURE_80MHZ_MASK;
        default:
            return PUNCTURE_NONE;
            break;
    }
}

uint16_t ieee80211_get_pp_from_find_chan(struct ieee80211com *ic,
                                         struct chan_params *chan_p,
                                         u_int64_t flags)
{
    ieee80211_find_channel(ic, chan_p, flags);
    return ieee80211_get_chan_param_op_punc_bitmap(chan_p);
}

#else

struct ieee80211_ath_channel
ieee80211_find_dot11_channel(struct ieee80211com *ic, struct chan_params *chan_p, enum ieee80211_phymode mode)
{
    return ieee80211_find_dot11_channel_internal(ic, chan_p->freq, chan_p->cfreq2, mode, chan_p->input_6g_pwr_mode, NO_PUNCTURE_BITMASK);
}
#endif

/*
 * ieee80211_find_dot11_channel_internal() - Locate the channel given channel number,
 * secondary center frequency and mode.
 * @ic: Pointer to ic.
 * @primary_freq: Input primary frequency.
 * @des_cfreq2: Input secondary center frequency.
 * @mode: Input combined mode (80211mode + bandwidth).
 * @in_6g_pwr_mode: Input 6G power type which will determine the 6G channel
 *                  list on which the freq is searched for.
 * @input_puncture_bitmap: Input puncture bitmap
 *
 * Return: an object of type struct ieee80211_ath_channel. Is not allocated
 * and is intended to be copied directly to an allocated object, by the caller.
 */
static struct ieee80211_ath_channel
ieee80211_find_dot11_channel_internal(struct ieee80211com *ic, qdf_freq_t primary_freq,
                             qdf_freq_t des_cfreq2, enum ieee80211_phymode mode,
                             enum supported_6g_pwr_types in_6g_pwr_mode,
                             uint16_t input_puncture_bitmap)
{
    struct ch_params ch_params = {0};
    struct ieee80211_ath_channel chan = {0};
    int retval;
    struct ieee80211_mode_info mode_info;
    struct regulatory_channel reg_chan_obj = {0};
    struct regulatory_channel *reg_chan = &reg_chan_obj;

    /* Check if the mode provided is proper */
    if (mode > IEEE80211_MODE_MAX)
        return chan;

    /* Check here if the user given mode is supported by our chip.
     * If not return an empty channel object to the caller.
     */
    if (!(ic->ic_modecaps & (1ULL << mode)))
        return chan;

    if (!primary_freq)
         primary_freq = ieee80211_reg_find_first_valid_freq(ic, mode,
                                                            in_6g_pwr_mode);

    if (!primary_freq)
         return chan;

    *reg_chan = wlan_reg_get_reg_chan_list_based_on_freq(ic->ic_pdev_obj,
                                                         primary_freq,
                                                         in_6g_pwr_mode);

    if (wlan_reg_is_chan_disabled_and_not_nol(reg_chan))
        return chan;

    retval = ieee80211_get_mode_info_from_mode(ic, primary_freq, mode,
                                               ic->ic_chanbwflag,
                                               &mode_info, in_6g_pwr_mode,
                                               reg_chan,
                                               input_puncture_bitmap);

    if (retval)
       return chan;

    regdmn_set_default_punc_pat(&ch_params, input_puncture_bitmap);
    retval = ieee80211_find_chan_with_ch_params(ic, &ch_params, &mode_info,
                                                primary_freq, des_cfreq2,
                                                in_6g_pwr_mode);
    if (retval)
       return chan;

    /* Convert ch_params to ieee_ath_channel and return */
    ieee80211_fill_ath_channel_from_ch_params(ic, ch_params,
                                              mode_info.combined_mode,
                                              primary_freq, &chan,
                                              in_6g_pwr_mode,
                                              reg_chan);
    return chan;
}

/*
 * ieee80211_find_channel_internal() - Locate the channel given channel number,
 * secondary center frequency and flags.
 * @ic: Pointer to ic.
 * @primary_freq: Input primary frequency.
 * @des_cfreq2: Input secondary center frequency.
 * @flags: Input channel flags.
 * @in_6g_pwr_mode: Input 6G power type which will determine the 6G channel
 * list on which the freq is searched for.
 * @input_puncture_bitmap: Input puncture bitmap
 *
 * Return: an object of type struct ieee80211_ath_channel. Is not allocated
 * and is intended to be copied directly to an allocated object, by the caller.
 */
static struct ieee80211_ath_channel
ieee80211_find_channel_internal(struct ieee80211com *ic, int freq, uint16_t des_cfreq2, u_int64_t flags, enum supported_6g_pwr_types in_6g_pwr_mode, uint16_t input_puncture_bitmap)
{
    struct ieee80211_ath_channel chan = {0};
    enum ieee80211_phymode mode;
    struct ieee80211_mode_info mode_info;
    struct ch_params ch_params = {0};
    int retval;
    struct regulatory_channel reg_chan_obj = {0};
    struct regulatory_channel *reg_chan = &reg_chan_obj;

    if (!freq)
        freq = ieee80211_reg_find_first_valid_freq(ic,
                                                   IEEE80211_MODE_AUTO,
                                                   in_6g_pwr_mode);

    if (!freq)
        return chan;

    *reg_chan = wlan_reg_get_reg_chan_list_based_on_freq(ic->ic_pdev_obj,
                                                              freq,
                                                              in_6g_pwr_mode);
    if (!flags)
        return chan;

    if (wlan_reg_is_chan_disabled_and_not_nol(reg_chan))
        return chan;

    if (!WLAN_REG_IS_49GHZ_FREQ(freq) &&
        (IEEE80211_IS_FLAG_HALF(flags) || IEEE80211_IS_FLAG_QUARTER(flags)))
        return chan;

    flags &= IEEE80211_CHAN_ALLTURBO;
    chan.ic_flags = flags;
    mode = ieee80211_chan2mode(&chan);

    chan = empty_channel;
    if (!(ic->ic_modecaps & (1ULL << mode)))
        return chan;

    retval = ieee80211_get_mode_info_from_mode(ic, freq, mode,
                                               flags, &mode_info,
                                               in_6g_pwr_mode, reg_chan,
                                               input_puncture_bitmap);

    if (retval)
        return chan;

    regdmn_set_default_punc_pat(&ch_params, input_puncture_bitmap);
    retval = ieee80211_find_chan_with_ch_params(ic, &ch_params, &mode_info,
                                                freq, des_cfreq2,
                                                in_6g_pwr_mode);
    if (retval)
        return chan;

    /* Convert ch_params to ieee_ath_channel and return */
    ieee80211_fill_ath_channel_from_ch_params(ic, ch_params,
                                              mode_info.combined_mode,
                                              freq, &chan,
                                              in_6g_pwr_mode,
                                              reg_chan);
    return chan;
}

struct ieee80211_ath_channel
ieee80211_ath_get_first_channel(struct ieee80211com *ic)
{
    qdf_freq_t freq;
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;
    struct ch_params ch_param = {0};
    struct ieee80211_ath_channel chan = {0};
    struct regulatory_channel reg_chan_obj = {0};
    struct regulatory_channel *reg_chan = &reg_chan_obj;

    freq = ieee80211_reg_find_first_valid_freq(ic, IEEE80211_MODE_AUTO,
                                               REG_CURRENT_PWR_MODE);

    if (!freq) {
        qdf_err("Can't find any valid frequency in the radio!");
        return chan;
    }

    *reg_chan = wlan_reg_get_reg_chan_list_based_on_freq(ic->ic_pdev_obj,
                                                              freq,
                                                              REG_CURRENT_PWR_MODE);

    if (WLAN_REG_IS_24GHZ_CH_FREQ(freq))
        mode = IEEE80211_MODE_11B;
    else if (WLAN_REG_IS_49GHZ_FREQ(freq) || WLAN_REG_IS_5GHZ_CH_FREQ(freq))
        mode = IEEE80211_MODE_11A;
    else if (WLAN_REG_IS_6GHZ_CHAN_FREQ(freq))
        mode = IEEE80211_MODE_11AXA_HE20;

    ch_param.ch_width = CH_WIDTH_20MHZ;
    ch_param.center_freq_seg0 = wlan_reg_freq_to_chan(ic->ic_pdev_obj, freq);
    ch_param.mhz_freq_seg0 = freq;

    ieee80211_fill_ath_channel_from_ch_params(ic, ch_param,
                                              mode, freq, &chan,
                                              REG_CURRENT_PWR_MODE,
                                              reg_chan);

    return chan;
}
#endif

enum ieee80211_phymode
ieee80211_get_non_auto_des_mode(struct chan_params *chan_p, struct ieee80211vap *vap)
{
    struct ieee80211_ath_channel chan = {0};
    enum ieee80211_phymode des_mode = vap->iv_des_mode;

    if (ieee80211_is_phymode_auto(des_mode)) {
        chan = ieee80211_find_dot11_channel(vap->iv_ic,
                                            chan_p,
                                            IEEE80211_MODE_AUTO);
        if (!IEEE80211_IS_CHAN_VALID(&chan)) {
            ieee80211_note(vap, IEEE80211_MSG_NODE, " %s Valid channel not found"
                    " in Auto desmode \n", __func__);
            return IEEE80211_MODE_NONE;
        }
        des_mode = ieee80211_chan2mode(&chan);
    }

    return des_mode;
}

struct ieee80211_ath_channel
ieee80211_find_dot11_channel_without_radar(struct ieee80211com *ic,
                                           struct chan_params *chan_p,
                                           enum ieee80211_phymode phymode,
                                           bool *is_input_phymode_radar)
{
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *new_chan = &chan_obj;

    *new_chan = ieee80211_find_dot11_channel(ic, chan_p, phymode);

    if (!IEEE80211_IS_CHAN_VALID(new_chan)) {
        return empty_channel;
    }

#ifdef WLAN_FEATURE_11BE
    if (IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, new_chan,
                                                       new_chan->op_puncture_bitmap))
#else
    if (IEEE80211_IS_CHAN_RADAR(ic, new_chan))
#endif
    {
        struct ch_params ch_params = {0};
        enum phy_ch_width ch_width;
        uint16_t new_punc_bmap;

        if (is_input_phymode_radar)
            *is_input_phymode_radar = true;

        /* If the input phymode is Auto, then start from the phymode of the radar affected channel */
        if (phymode == IEEE80211_MODE_AUTO)
            phymode = ieee80211_chan2mode(new_chan);

        ch_width = ieee80211_get_chan_width_from_phymode(phymode);
        if (ch_width == CH_WIDTH_INVALID)
            return empty_channel;

        ch_params.ch_width = ch_width;
        /* Try to find a non-Radar channel with the input channel frequency and
         * chan params. If not found, this API will downgrade and try to find
         * a valid channel with a reduced valid channel width.
         */
        wlan_reg_set_channel_params_for_pwrmode(ic->ic_pdev_obj,
                                                new_chan->ic_freq,
                                                0,
                                                &ch_params,
                                                REG_CURRENT_PWR_MODE);
        if (ch_params.ch_width == CH_WIDTH_INVALID) {
            phymode = IEEE80211_MODE_MAX;
        } else {
            phymode = ieee80211_get_composite_phymode(get_mode_from_phymode(phymode),
                                                      ieee80211_get_cwm_width(ch_params.ch_width),
                                                      ch_params.sec_ch_offset);
        }

        if (!ieee80211_is_phymode_valid(phymode)) {
            return empty_channel;
        }

        chan_p->cfreq2 = ch_params.mhz_freq_seg1;
        new_punc_bmap = ieee80211_intersect_puncture_bitmap(new_chan, phymode, ieee80211_chan2mode(new_chan));
        set_chan_params_puncture_bitmap(chan_p, new_punc_bmap);

        *new_chan = ieee80211_find_dot11_channel(ic, chan_p, phymode);
    }

    return *new_chan;
}

#define IS_CHANNEL_WEATHER_RADAR(freq) ((freq >= 5600) && (freq <= 5650))
#define ADJACENT_WEATHER_RADAR_CHANNEL   5580
#define CH100_START_FREQ                 5490
#define CH100                            100
/*
 * Print a console message with the device name prepended.
 */
static void
if_printf( osdev_t dev, const char *fmt, ...)
{
    va_list ap;
    char buf[512];              /* XXX */

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    qdf_nofl_info("\n %s\n", buf);
}

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
/*
 * Function to check the given frequency is present in the allowed
 * primary frequency list or not
 *
 */
bool ieee80211_check_allowed_prim_freqlist(struct ieee80211com *ic,
                uint16_t freq)
{
        u_int8_t i = 0;
        struct primary_allowed_chanlist *chanlist = ic->ic_primary_chanlist;

        for(i = 0; i < chanlist->n_freq; i++)
        {
                if(freq == chanlist->freq[i]) {
                        return true;
                }
        }

        return false;
}
#endif

/*
 * Check if two channels are in the same frequency band.
 */
bool
ieee80211_is_same_frequency_band(const struct ieee80211_ath_channel *chan1, const struct ieee80211_ath_channel *chan2)
{
    if (chan2 == NULL) {
        return false;
    }

    if (IEEE80211_IS_CHAN_2GHZ(chan1)) {
        /*
         * Channel1 is 2GHz, return TRUE only if channel2 is also 2GHz
         */
        return IEEE80211_IS_CHAN_2GHZ(chan2);
    }
    else if (IEEE80211_IS_CHAN_5GHZ(chan1)) {
        /*
         * Channel1 is 5GHz, return TRUE only if channel2 is also 5GHz
         */
        return IEEE80211_IS_CHAN_5GHZ(chan2);
    } else {
        /*
         * Channel1 is 6GHz, return TRUE only if channel2 is also 6GHz
         */
        return IEEE80211_IS_CHAN_6GHZ(chan2);
    }
}

void wlan_update_current_mode_caps(struct ieee80211com *ic)
{
    ic->ic_current_modecaps = ieee80211_update_modecaps(ic, false);
}

/**
 * modes_conv - temporary structure to define a conversion array from
 * ieee80211_phymode to the wireless_modes value from the tgt caps.
 * @mode: 64 bit integer, set bit indicating the mode.
 * @phymode: Phymode enum.
 */
struct modes_conv {
    uint64_t mode;
    enum ieee80211_phymode phymode;
};

/* Conversion array, from ieee80211_phymode to wireless_modes from tgt. */
static const struct modes_conv m_conv[] = {
#ifndef ATH_NO_5G_SUPPORT
    { HOST_REGDMN_MODE_11A,                 IEEE80211_MODE_11A},
#endif

    { HOST_REGDMN_MODE_11B,                 IEEE80211_MODE_11B},
    { HOST_REGDMN_MODE_11G,                 IEEE80211_MODE_11G},
    { HOST_REGDMN_MODE_11NG_HT20,           IEEE80211_MODE_11NG_HT20},
    { HOST_REGDMN_MODE_11NG_HT40PLUS,       IEEE80211_MODE_11NG_HT40PLUS},
    { HOST_REGDMN_MODE_11NG_HT40MINUS,      IEEE80211_MODE_11NG_HT40MINUS},
#ifndef ATH_NO_5G_SUPPORT
    { HOST_REGDMN_MODE_11NA_HT20,           IEEE80211_MODE_11NA_HT20},
    { HOST_REGDMN_MODE_11NA_HT40PLUS,       IEEE80211_MODE_11NA_HT40PLUS},
    { HOST_REGDMN_MODE_11NA_HT40MINUS,      IEEE80211_MODE_11NA_HT40MINUS},
    { HOST_REGDMN_MODE_11AC_VHT20,          IEEE80211_MODE_11AC_VHT20},
    { HOST_REGDMN_MODE_11AC_VHT40PLUS,      IEEE80211_MODE_11AC_VHT40PLUS},
    { HOST_REGDMN_MODE_11AC_VHT40MINUS,     IEEE80211_MODE_11AC_VHT40MINUS},
    { HOST_REGDMN_MODE_11AC_VHT80,          IEEE80211_MODE_11AC_VHT80},
    { HOST_REGDMN_MODE_11AC_VHT160,         IEEE80211_MODE_11AC_VHT160},
    { HOST_REGDMN_MODE_11AC_VHT80_80,       IEEE80211_MODE_11AC_VHT80_80},
    { HOST_REGDMN_MODE_11AXG_HE20,          IEEE80211_MODE_11AXG_HE20},
    { HOST_REGDMN_MODE_11AXA_HE20,          IEEE80211_MODE_11AXA_HE20},
    { HOST_REGDMN_MODE_11AXG_HE40PLUS,      IEEE80211_MODE_11AXG_HE40PLUS},
    { HOST_REGDMN_MODE_11AXG_HE40MINUS,     IEEE80211_MODE_11AXG_HE40MINUS},
    { HOST_REGDMN_MODE_11AXA_HE40PLUS,      IEEE80211_MODE_11AXA_HE40PLUS},
    { HOST_REGDMN_MODE_11AXA_HE40MINUS,     IEEE80211_MODE_11AXA_HE40MINUS},
    { HOST_REGDMN_MODE_11AXA_HE80,          IEEE80211_MODE_11AXA_HE80},
    { HOST_REGDMN_MODE_11AXA_HE160,         IEEE80211_MODE_11AXA_HE160},
    { HOST_REGDMN_MODE_11AXA_HE80_80,       IEEE80211_MODE_11AXA_HE80_80},
#endif
#ifdef WLAN_FEATURE_11BE
    { HOST_REGDMN_MODE_11BEG_EHT20,          IEEE80211_MODE_11BEG_EHT20},
    { HOST_REGDMN_MODE_11BEA_EHT20,          IEEE80211_MODE_11BEA_EHT20},
    { HOST_REGDMN_MODE_11BEG_EHT40PLUS,      IEEE80211_MODE_11BEG_EHT40PLUS},
    { HOST_REGDMN_MODE_11BEG_EHT40MINUS,     IEEE80211_MODE_11BEG_EHT40MINUS},
    { HOST_REGDMN_MODE_11BEA_EHT40PLUS,      IEEE80211_MODE_11BEA_EHT40PLUS},
    { HOST_REGDMN_MODE_11BEA_EHT40MINUS,     IEEE80211_MODE_11BEA_EHT40MINUS},
    { HOST_REGDMN_MODE_11BEA_EHT80,          IEEE80211_MODE_11BEA_EHT80},
    { HOST_REGDMN_MODE_11BEA_EHT160,         IEEE80211_MODE_11BEA_EHT160},
    { HOST_REGDMN_MODE_11BEA_EHT320,         IEEE80211_MODE_11BEA_EHT320},
#endif
};

bool ieee80211_is_any_80p80_supported(struct ieee80211com *ic,
                                      bool include_nol_chan)
{
    struct regulatory_channel *cur_chan_list;
    int i;
    qdf_freq_t sec_cfreq;
    uint8_t sec_cieee;

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list) {
        qdf_err("malloc failed");
        return false;
    }

    if (wlan_reg_get_current_chan_list(ic->ic_pdev_obj, cur_chan_list) !=
        QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        qdf_mem_free(cur_chan_list);
        return false;
    }

    for (i = 0; i < NUM_CHANNELS; i++) {
        qdf_freq_t freq = cur_chan_list[i].center_freq;

        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
            continue;

        if (ieee80211_is_80p80mhz_supported(ic, freq, &sec_cfreq, &sec_cieee,
                                            include_nol_chan,
                                            REG_CURRENT_PWR_MODE)) {
            qdf_mem_free(cur_chan_list);
            return true;
        }
    }

    qdf_mem_free(cur_chan_list);
    return false;
}
qdf_export_symbol(ieee80211_is_any_80p80_supported);

uint64_t ieee80211_update_modecaps(struct ieee80211com *ic,
				   bool include_nol_chan)
{
    uint64_t user_mode = 0, chip_mode, mode_select;
    uint16_t phybitmap;
    uint64_t mcapmask = (1ULL << IEEE80211_MODE_MAX) -1;
    uint64_t mcaps = 0;
    const struct modes_conv *conv;
#ifdef CONFIG_WIFI_EMULATION_WIFI_3_0
    struct ol_ath_softc_net80211 *scn;
    bool emu = false;
#endif

    mcaps &= (~mcapmask);
    mcaps |= 1<<IEEE80211_MODE_AUTO;

#ifdef CONFIG_WIFI_EMULATION_WIFI_3_0
    scn =  OL_ATH_SOFTC_NET80211(ic);
    emu = cfg_get(scn->soc->psoc_obj, CFG_OL_EMULATION);
    if (emu) {
         mcaps |= mcapmask;
         return mcaps;
    }
#endif

    if(ic->ic_get_modeSelect) {
        user_mode = ic->ic_get_modeSelect(ic);
    }

    ieee80211_regdmn_get_chip_mode(ic->ic_pdev_obj, &chip_mode);

    mode_select = (user_mode & chip_mode);
    ieee80211_regdmn_get_phybitmap(ic->ic_pdev_obj, &phybitmap);
    ieee80211_regdmn_remove_phybitmap_from_modeselect(&mode_select, phybitmap);

    wlan_reg_get_wmodes_and_max_chwidth(ic->ic_pdev_obj, &mode_select,
                                        include_nol_chan);

    if (!ieee80211_is_any_80p80_supported(ic, include_nol_chan))
        mode_select &= (~WIRELESS_80P80_MODES);

    for (conv = m_conv; conv < &m_conv[QDF_ARRAY_SIZE(m_conv)]; conv++) {
        if ((conv->mode & mode_select) == 0) {
            continue;
        }

        if (!ic->ic_reg_parm.enable2GHzHt40Cap &&
            (conv->phymode == IEEE80211_MODE_11NG_HT40PLUS ||
             conv->phymode == IEEE80211_MODE_11NG_HT40MINUS))
            continue;

        mcaps |= 1ULL<<(conv->phymode);

        if (conv->phymode == IEEE80211_MODE_11NA_HT40PLUS ||
            conv->phymode == IEEE80211_MODE_11NA_HT40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11NA_HT40;

        if (conv->phymode == IEEE80211_MODE_11NG_HT40PLUS ||
            conv->phymode == IEEE80211_MODE_11NG_HT40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11NG_HT40;

        if (conv->phymode == IEEE80211_MODE_11AC_VHT40PLUS ||
            conv->phymode == IEEE80211_MODE_11AC_VHT40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11AC_VHT40;

        if (conv->phymode == IEEE80211_MODE_11AXA_HE40PLUS ||
            conv->phymode == IEEE80211_MODE_11AXA_HE40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11AXA_HE40;

        if (conv->phymode == IEEE80211_MODE_11AXG_HE40PLUS ||
            conv->phymode == IEEE80211_MODE_11AXG_HE40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11AXG_HE40;

#ifdef WLAN_FEATURE_11BE
        if (conv->phymode == IEEE80211_MODE_11BEA_EHT40PLUS ||
            conv->phymode == IEEE80211_MODE_11BEA_EHT40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11BEA_EHT40;

        if (conv->phymode == IEEE80211_MODE_11BEG_EHT40PLUS ||
            conv->phymode == IEEE80211_MODE_11BEG_EHT40MINUS)
            mcaps |= 1ULL<<IEEE80211_MODE_11BEG_EHT40;
#endif
    }

    return mcaps;
}

void ieee80211_update_channel_avail_bits(struct ieee80211com *ic)
{
    struct regulatory_channel *cur_chan_list;
    int i;

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list) {
        qdf_err("malloc failed");
        return;
    }

    if (wlan_reg_get_current_chan_list(ic->ic_pdev_obj, cur_chan_list) !=
        QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        qdf_mem_free(cur_chan_list);
        return;
    }

    OS_MEMZERO(ic->ic_chan_avail_2g_5g, sizeof(ic->ic_chan_avail_2g_5g));
    OS_MEMZERO(ic->ic_chan_avail_6g, sizeof(ic->ic_chan_avail_6g));


    for (i = 0; i < NUM_CHANNELS; i++) {
        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
            continue;

        if (WLAN_REG_IS_24GHZ_CH_FREQ(cur_chan_list[i].center_freq) ||
            WLAN_REG_IS_49GHZ_FREQ(cur_chan_list[i].center_freq) ||
            WLAN_REG_IS_5GHZ_CH_FREQ(cur_chan_list[i].center_freq))
            setbit(ic->ic_chan_avail_2g_5g, cur_chan_list[i].chan_num);


        if (WLAN_REG_IS_6GHZ_CHAN_FREQ(cur_chan_list[i].center_freq))
            setbit(ic->ic_chan_avail_6g, cur_chan_list[i].chan_num);
    }

    qdf_mem_free(cur_chan_list);
}

/*
 * Update channel list and associated PHY mode bitmask
 */
int
ieee80211_update_channellist(struct ieee80211com *ic, int exclude_11d,
                             bool no_chanchange)
{
    ic->ic_modecaps = ieee80211_update_modecaps(ic, true);

    ieee80211_update_channel_avail_bits(ic);

    /* initialize candidate channels to all available */
    OS_MEMCPY(ic->ic_chan_active_2g_5g, ic->ic_chan_avail_2g_5g,
              sizeof(ic->ic_chan_avail_2g_5g));
    OS_MEMCPY(ic->ic_chan_active_6g, ic->ic_chan_avail_6g,
              sizeof(ic->ic_chan_avail_6g));

    if (exclude_11d)
        wlan_scan_update_channel_list(ic);

    /* bug #103186
     * after re-setting the country code, if curchan is
     * not availble anymore in the new country, then
     * let's find another channel.
     */
    if ((exclude_11d) && IEEE80211_IS_CHAN_VALID(ic->ic_curchan)) {
        struct ieee80211_ath_channel first_valid_c = {0};
        struct ieee80211_ath_channel curchan = {0};

        curchan = ieee80211_find_dot11_channel_internal(ic, ic->ic_curchan->ic_freq,
                                               ic->ic_curchan->ic_vhtop_freq_seg2,
                                               ieee80211_chan2mode(ic->ic_curchan),
                                               REG_CURRENT_PWR_MODE, NO_PUNCTURE_BITMASK);

        if (IEEE80211_IS_CHAN_VALID(&curchan))
            return 0;

        first_valid_c = ieee80211_find_dot11_channel_internal(ic, 0, 0, 0, REG_CURRENT_PWR_MODE, NO_PUNCTURE_BITMASK);
        if(!no_chanchange && IEEE80211_IS_CHAN_VALID(&first_valid_c)) {
            /* the ic_curchan is not valid, set a valid channel */
            ieee80211_set_channel(ic, &first_valid_c);
        }
    }
    return 0;
}
qdf_export_symbol(ieee80211_update_channellist);

/*
 * Set current channel
 */
int
ieee80211_set_channel(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    int error;
    struct ieee80211_ath_channel tempchan;
    uint16_t in_punc_pattern;
    enum reg_6g_ap_type cur_pwr_type;
    uint16_t temp_power;
#ifdef ATH_SUPPORT_DFS
    struct wlan_objmgr_pdev *pdev;
#endif
#if UNIFIED_SMARTANTENNA
    QDF_STATUS status = QDF_STATUS_E_FAILURE;
#endif

    if(!IEEE80211_IS_CHAN_VALID(chan))
        return -1;

    tempchan = *ic->ic_prevchan;
    *ic->ic_prevchan = *ic->ic_curchan;
    *ic->ic_curchan = *chan;
    in_punc_pattern = ieee80211_get_chan_punc_bitmap(ic->ic_curchan);

    if (wlan_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &cur_pwr_type)) {
        qdf_err("error in getting current power mode");
        return -1;
    }

    temp_power = ic->ic_curchan_eirp_power;
    if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
        ic->ic_curchan_eirp_power = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj, ic->ic_curchan->ic_freq,
                ic->ic_curchan->ic_vhtop_freq_seg2,
                ieee80211_get_chan_width(ic->ic_curchan),
                cur_pwr_type,
                in_punc_pattern,
                false, REG_MAX_CLIENT_TYPE, false);
    }

    error = ic->ic_set_channel(ic);
    if (error) {
        *ic->ic_curchan = *ic->ic_prevchan;
        *ic->ic_prevchan = tempchan;
        ic->ic_curchan_eirp_power = temp_power;
        return error;
    }
    ic->ic_curmode = ieee80211_chan2mode(ic->ic_curchan);
#ifdef ATH_SUPPORT_DFS
    if (IEEE80211_IS_CHAN_VALID(ic->ic_prevchan) &&
        IEEE80211_IS_CHAN_VALID(ic->ic_curchan)) {
        pdev = ic->ic_pdev_obj;
        if(pdev == NULL) {
            qdf_print("%s : pdev is null", __func__);
            return -1;
        }

        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }
        mlme_dfs_cac_valid_reset_for_freq(pdev,
                ic->ic_prevchan->ic_freq,
                ic->ic_prevchan->ic_flags);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    }
#endif
#if UNIFIED_SMARTANTENNA
    if (ic->ic_opmode == IEEE80211_M_STA) {
        ic->sta_not_connected_cfg = FALSE;
    }
    {
        status = wlan_objmgr_pdev_try_get_ref(ic->ic_pdev_obj, WLAN_SA_API_ID);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_print("%s, %d unable to get reference", __func__, __LINE__);
        } else {
            wlan_sa_api_channel_change(ic->ic_pdev_obj);
            wlan_objmgr_pdev_release_ref(ic->ic_pdev_obj, WLAN_SA_API_ID);
        }
    }
#endif
    /* Note: In VHT mode the right channel center frequency index will be available
     * from the regulatory domain table */

    /*
     * When there is radar detect in Repeater, repeater sends RCSAs, CSAs and
     * switches to new next channel, in ind rpt case repeater AP could start
     * beaconing before Root comes up, next channel needs to be changed
     *
     * For offload, resmgr will trigger channel change and fw resp of vdev up will
     * update dfs channel
     */
    return error;
}
/*
 * Set the current phy mode.
 */
int ieee80211_setmode(struct ieee80211com *ic,
                      enum ieee80211_phymode mode,
                      enum ieee80211_opmode opmode)
{
    ieee80211_reset_erp(ic, mode, opmode); /* reset ERP state */
    ic->ic_curmode = mode;      /* NB: must do post reset_erp */
    return 0;
}

const char *ieee80211_phymode_to_name( enum ieee80211_phymode mode)
{
    static const char unknown_mode[]="UNKNWON";
    if (mode >= sizeof(ieee80211_phymode_name)/sizeof(char *)) {
        return unknown_mode;
    }
    return (ieee80211_phymode_name[mode]);
}

void
update_maxampdu_vhtmaxampdu (wlan_if_t vaphandle,
                             uint32_t ic_maxampdu,
                             uint32_t ic_vhtmaxampdu)
{
    uint32_t target_vhtmaxampdu, target_maxampdu, retval;
    target_vhtmaxampdu = wlan_get_param(vaphandle, IEEE80211_VHT_MAX_AMPDU);
    target_maxampdu = wlan_get_param(vaphandle, IEEE80211_MAX_AMPDU);

    if (ic_vhtmaxampdu < target_vhtmaxampdu) {
        retval = wlan_set_param(vaphandle, IEEE80211_VHT_MAX_AMPDU, ic_vhtmaxampdu);
        if (retval) {
             qdf_nofl_info("ic vhtmaxampdu set failed\n");
        }
    }

    if (ic_maxampdu < target_maxampdu) {
        retval = wlan_set_param(vaphandle, IEEE80211_MAX_AMPDU, ic_maxampdu);
        if (retval) {
            qdf_nofl_info("ic maxampdu set failed\n");
        }
    }
}

int
wlan_get_supported_phymodes(wlan_dev_t devhandle,
                            enum ieee80211_phymode *modes,
                            u_int16_t *nmodes,
                            u_int16_t len)
{
    struct ieee80211com *ic = devhandle;
    int n = 0;
#define ADD_PHY_MODE(_m) do {                       \
    if (IEEE80211_SUPPORT_PHY_MODE(ic, (_m))) {     \
        if (len < (n+1))                            \
            goto bad;                               \
        modes[n++] = (_m);                          \
    }                                               \
} while (0)

    /*
     * NB: we fill in the modes array in certain order to be
     * compatible with Win 7/Vista SP1 driver.
     */
    ADD_PHY_MODE(IEEE80211_MODE_11B);
    ADD_PHY_MODE(IEEE80211_MODE_11A);
    ADD_PHY_MODE(IEEE80211_MODE_11G);
    ADD_PHY_MODE(IEEE80211_MODE_11NG_HT40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11NG_HT40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11NG_HT40);
    ADD_PHY_MODE(IEEE80211_MODE_11NG_HT20);
    ADD_PHY_MODE(IEEE80211_MODE_11NA_HT40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11NA_HT40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11NA_HT40);
    ADD_PHY_MODE(IEEE80211_MODE_11NA_HT20);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT80_80);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT160);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT80);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT40);
    ADD_PHY_MODE(IEEE80211_MODE_11AC_VHT20);
    ADD_PHY_MODE(IEEE80211_MODE_11AXG_HE40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11AXG_HE40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11AXG_HE20);
    ADD_PHY_MODE(IEEE80211_MODE_11AXA_HE80_80);
    ADD_PHY_MODE(IEEE80211_MODE_11AXA_HE160);
    ADD_PHY_MODE(IEEE80211_MODE_11AXA_HE80);
    ADD_PHY_MODE(IEEE80211_MODE_11AXA_HE40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11AXA_HE40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11AXA_HE20);
#ifdef WLAN_FEATURE_11BE
    ADD_PHY_MODE(IEEE80211_MODE_11BEG_EHT20);
    ADD_PHY_MODE(IEEE80211_MODE_11BEG_EHT40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11BEG_EHT40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11BEA_EHT20);
    ADD_PHY_MODE(IEEE80211_MODE_11BEA_EHT40PLUS);
    ADD_PHY_MODE(IEEE80211_MODE_11BEA_EHT40MINUS);
    ADD_PHY_MODE(IEEE80211_MODE_11BEA_EHT80);
    ADD_PHY_MODE(IEEE80211_MODE_11BEA_EHT160);
    ADD_PHY_MODE(IEEE80211_MODE_11BEA_EHT320);
#endif /* WLAN_FEATURE_11BE */

    *nmodes = n;
    return 0;

bad:
    *nmodes = IEEE80211_MODE_MAX;
    return -EOVERFLOW;
#undef ADD_PHY_MODE
}

int
wlan_set_desired_phylist(wlan_if_t vaphandle, enum ieee80211_phymode *phylist, u_int16_t nphy)
{
    struct ieee80211vap *vap = vaphandle;
    int i;

    if (nphy > IEEE80211_MODE_MAX || nphy < 1) {
        return -EINVAL;
    }

    vap->iv_des_modecaps = 0;

    for (i = 0; i < nphy; i++) {
        if (phylist[i] == IEEE80211_MODE_AUTO) {
            vap->iv_des_modecaps = (1 << IEEE80211_MODE_AUTO);
            return 0;
        }

        if (!IEEE80211_SUPPORT_PHY_MODE(vap->iv_ic, phylist[i])) {
            return -EINVAL;
        }

        vap->iv_des_modecaps |= (1 << phylist[i]);
    }

    return 0;
}

int
wlan_get_desired_phylist(wlan_if_t vaphandle,
                         enum ieee80211_phymode *phylist,
                         u_int16_t *nphy,
                         u_int16_t len)
{
    struct ieee80211vap *vap = vaphandle;
    enum ieee80211_phymode m;
    u_int16_t count;

    if (len < 1) {
        *nphy = IEEE80211_MODE_MAX;
        return -EOVERFLOW;
    }

    /* return AUTO if we accept any PHY modes */
    if (IEEE80211_ACCEPT_ANY_PHY_MODE(vap)) {
        phylist[0] = IEEE80211_MODE_AUTO;
        *nphy = 1;
        return 0;
    }

    count = 0;
    for (m = IEEE80211_MODE_AUTO + 1; m < IEEE80211_MODE_MAX; m++) {
        if (IEEE80211_ACCEPT_PHY_MODE(vap, m)) {
            /* is input big enough */
            if (len < (count+1))
                return -EINVAL;

            phylist[count++] = m;
        }
    }

    *nphy = count;
    return 0;
}

enum ieee80211_phymode
wlan_get_desired_phymode(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    return vap->iv_des_mode;
}

int
wlan_set_desired_phymode(wlan_if_t vaphandle, enum ieee80211_phymode mode)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211vap *tmpvap = NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_ath_channel chan_obj = wlan_get_current_channel(vap, false);
    wlan_chan_t chan = &chan_obj;
    int numvaps_up = 0;
    wlan_if_t first_vap;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct chan_params chan_p = {0};
    uint32_t ic_vhtmaxampdu, ic_maxampdu;

    if (!IEEE80211_SUPPORT_PHY_MODE(vap->iv_ic, mode)) {
            qdf_nofl_info("Rejecting mode %d: This mode may be invalid or may be unavailable for the requested values of parameters such as channel and chainmasks, ic_modecaps:%llx\n", mode, ic->ic_modecaps);
            return -EINVAL;
    }

    if (!(ic->ic_current_modecaps & (1ULL << mode))) {
#if defined(WLAN_FEATURE_11BE) && defined(QCA_DFS_BW_PUNCTURE)
        bool is_dfs_punc_en;

        ucfg_dfs_get_dfs_puncture(ic->ic_pdev_obj, &is_dfs_punc_en);
        if (!(IEEE80211_IS_CHAN_11BE(chan) && is_dfs_punc_en))
#endif
        {
            first_vap = TAILQ_FIRST(&(vap->iv_ic)->ic_vaps);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s : mode = %d is not available in the current_modecaps (%llx), override vap = %d (%s) mode as %d\n",
                              __func__, mode, ic->ic_current_modecaps, vaphandle->iv_unit, vaphandle->iv_netdev_name, first_vap->iv_des_mode);
            mode = first_vap->iv_des_mode;
        }
    }

    /* Check for the vdev status to really validate if the null channel is due
     * to an error. This is because iv_des_chan is initialised to empty channel
     * which will result in mode not being set at the initial bringup.
     * We really only need to check the validity of ic_curchan (which will be
     * used here only if this is an active VAP).
     */
    if (!IEEE80211_IS_CHAN_VALID(chan) &&
        (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
        qdf_nofl_info("chan is NULL for mode: %d\n", mode);
        return -EINVAL;
    }

    ic_maxampdu = wlan_get_param(vaphandle, IEEE80211_MAX_AMPDU);
    ic_vhtmaxampdu = wlan_get_param(vaphandle, IEEE80211_VHT_MAX_AMPDU);

    if (ic->ic_update_target_caps)
        ic->ic_update_target_caps(ic, mode);

    update_maxampdu_vhtmaxampdu(vaphandle, ic_maxampdu, ic_vhtmaxampdu);

    /* in multivap case, should we check if this is in agreement with other vaps */
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        /* If any vap is running, find the same frequency channel with vap desired mode */
        numvaps_up = ieee80211_get_num_vaps_up(ic);

        if ((numvaps_up) && !IEEE80211_ARE_CHANS_EQUAL(ic->ic_curchan, chan)) {
            *chan = *(ic->ic_curchan);
        }

	chan_p.freq = chan->ic_freq;
#ifdef WLAN_FEATURE_11BE
        if (ieee80211_is_phymode_320(mode)) {
	    chan_p.cfreq2 = 0;
        } else
#endif
        {
	    chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
        }
        set_chan_params_puncture_bitmap(&chan_p, chan->op_puncture_bitmap);
        *chan = ieee80211_find_dot11_channel(ic, &chan_p, mode);
        if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL) && ieee80211_get_num_ap_vaps_up(ic) &&
            !IEEE80211_IS_CHAN_VALID(chan)) {

            qdf_err("ERROR!! Mode %d not compatible with current channel\n",
                    mode);
            return -EINVAL;
        }
    }

    if (IEEE80211_IS_CHAN_VALID(chan)) {
#ifdef WLAN_FEATURE_11BE
        if (IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, chan,
                                                           chan->op_puncture_bitmap))
#else
            /* don't allow to change to channel with radar found */
            if (IEEE80211_IS_CHAN_RADAR(ic, chan))
#endif
            {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_DFS,
                                  "%s:Cannot move to a RADAR detected channel\n",
                                  __func__);
                return -EINVAL;
            }
    }

    if ((mode == IEEE80211_MODE_AUTO) || (vap->iv_opmode == IEEE80211_M_STA) || ic->ic_diff_mode || vap->iv_repurpose) {
        if (vap->iv_des_mode != mode)
           vap->iv_wme_reset = 1;
        vap->iv_des_mode = mode;
        vap->iv_des_hw_mode = mode;
        if (IEEE80211_IS_CHAN_VALID(chan)) {
            *(vap->iv_des_chan[mode]) = *chan;
        }
#ifdef WLAN_FEATURE_11BE
    } else if (ieee80211_mixed_modes_config(vap, mode)) {
        enum ieee80211_phymode max_supported_lower_mode = 0;
        enum ieee80211_mode lower_mode = ieee80211_mixed_modes_config(vap, mode);

        if (ieee80211_is_mode_higher(vap, mode)) {
            max_supported_lower_mode = ieee80211_get_derived_mode(vap, mode, lower_mode, NULL);

            IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE, "%s: Max lower supported mode %d", __func__, max_supported_lower_mode);
            /**
             * Change the mode for all 11be vaps and change
             * it for 11ax vaps if max supported lower mode
             * is less than existing 11ax vap's desired mode.
             */
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                if (ieee80211_is_mode_higher(vap, tmpvap->iv_des_mode) || (tmpvap == vap)) {
                    if (tmpvap->iv_des_mode != mode)
                        tmpvap->iv_wme_reset = 1;
                    tmpvap->iv_des_mode = mode;
                    tmpvap->iv_des_hw_mode = mode;
                    if (IEEE80211_IS_CHAN_VALID(chan)) {
                        *(tmpvap->iv_des_chan[mode]) = *chan;
                    }
                } else if (ieee80211_is_mode_lower(vap, tmpvap->iv_des_mode) && (tmpvap->iv_des_mode > max_supported_lower_mode)) {
                    tmpvap->iv_wme_reset = 1;
                    tmpvap->iv_des_mode = max_supported_lower_mode;
                    tmpvap->iv_des_hw_mode = max_supported_lower_mode;
                    if (IEEE80211_IS_CHAN_VALID(chan)) {
                        struct ieee80211_ath_channel nchan = ieee80211_find_dot11_channel(ic, &chan_p, max_supported_lower_mode);
                        if (IEEE80211_IS_CHAN_VALID(&nchan))
                            *(tmpvap->iv_des_chan[max_supported_lower_mode]) = nchan;
                    }
                }
            }
        } else if (ieee80211_is_mode_lower(vap, mode)) {
            /**
             * Change the mode for 11ax vaps only if
             * new mode is less than or equal to max
             * supported lower mode.
             */
            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                if (tmpvap == vap)
                    continue;
                if (ieee80211_is_mode_higher(tmpvap, tmpvap->iv_des_mode)) {
                    if (!max_supported_lower_mode)
                        max_supported_lower_mode = ieee80211_get_derived_mode(vap, tmpvap->iv_des_mode, lower_mode, NULL);
                    if (wlan_vdev_mlme_is_active(tmpvap->vdev_obj) == QDF_STATUS_SUCCESS) {
                        max_supported_lower_mode = ieee80211_get_derived_mode(vap, tmpvap->iv_cur_mode, lower_mode, NULL);
                        break;
                    }
                }
            }

            if (mode > max_supported_lower_mode) {
                qdf_info("Changing the mode to max supported phymode %d\n", max_supported_lower_mode);
                mode = max_supported_lower_mode;
            }

            TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
                if ((tmpvap == vap) || ieee80211_is_mode_lower(vap, tmpvap->iv_des_mode)) {
                    if (tmpvap->iv_des_mode != mode)
                        tmpvap->iv_wme_reset = 1;
                    tmpvap->iv_des_mode = mode;
                    tmpvap->iv_des_hw_mode = mode;
                    if (IEEE80211_IS_CHAN_VALID(chan)) {
                        struct ieee80211_ath_channel nchan = ieee80211_find_dot11_channel(ic, &chan_p, mode);
                        if (IEEE80211_IS_CHAN_VALID(&nchan))
                            *(tmpvap->iv_des_chan[mode]) = nchan;
                    }
                }
            }
        }
#endif
    } else {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (tmpvap->iv_des_mode != mode)
               tmpvap->iv_wme_reset = 1;
            tmpvap->iv_des_mode = mode;
            tmpvap->iv_des_hw_mode = mode;
            if (IEEE80211_IS_CHAN_VALID(chan)) {
                *(tmpvap->iv_des_chan[mode]) = *chan;
            }
        }
    }

    if (ieee80211_is_phymode_not_basic(mode))
        ieee80211_vap_wme_set(vap);

    return 0;
}

int
wlan_set_desired_ibsschan(wlan_if_t vaphandle, struct chan_params *chan_p)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_ath_channel channel_obj;
    struct ieee80211_ath_channel *channel = &channel_obj;
    struct ieee80211com *ic = vap->iv_ic;

    if (chan_p == NULL) {
        return -EINVAL;
    }

    if (chan_p->freq > IEEE80211_FREQ_MAX)
        return -EINVAL;

    if(chan_p->freq == (uint16_t)IEEE80211_FREQ_ANY) {
            *channel = *(vap->iv_des_chan[vap->iv_des_mode]);
            if(!IEEE80211_IS_CHAN_VALID(channel))
                return -EINVAL;
            chan_p->freq = channel->ic_freq;
    } else {
        *channel = ieee80211_find_dot11_channel(ic, chan_p, vap->iv_des_mode);
        if (!IEEE80211_IS_CHAN_VALID(channel)) {
	    chan_p->cfreq2 = 0;
            *channel = ieee80211_find_dot11_channel(ic, chan_p, IEEE80211_MODE_AUTO);
            if (!IEEE80211_IS_CHAN_VALID(channel))
                return -EINVAL;
        }
    }

    if (IEEE80211_IS_CHAN_RADAR(ic, channel)) {
        return -EINVAL;
    }

    vap->iv_des_ibss_chan_freq = chan_p->freq;

    return 0;
}


enum ieee80211_phymode
wlan_get_current_phymode(wlan_if_t vaphandle)
{
    if (ieee80211_ic_rpt_max_phy_is_set(vaphandle->iv_ic) || vaphandle->iv_ic->ic_diff_mode || ieee80211_mixed_modes_config(vaphandle, vaphandle->iv_des_mode))
        return vaphandle->iv_cur_mode;
    return ieee80211_get_current_phymode(vaphandle->iv_ic);
}

enum ieee80211_phymode
wlan_get_bss_phymode(wlan_if_t vaphandle)
{
    return ieee80211_chan2mode(vaphandle->iv_bsschan);
}

enum ieee80211_phymode
wlan_get_des_phymode(struct ieee80211_ath_channel *des_chan)
{
    return ieee80211_chan2mode(des_chan);
}

struct ieee80211_ath_channel
wlan_get_current_channel(wlan_if_t vaphandle, bool hwChan)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    bool is_spoof_check_enabled = 0;
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    int dfs_region;

    if (ic->ic_is_host_dfs_check_enabled)
        ic->ic_is_host_dfs_check_enabled(pdev, &is_spoof_check_enabled);
    ieee80211_regdmn_get_dfs_region(pdev, (enum dfs_reg *)&dfs_region);
#endif /* HOST_DFS_SPOOF_TEST */

    /*
     * If the VAP is in RUN state, return the current channel.
     * Otherwise, return the desired channel of the desired phymode.
     */
    if ((wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) || hwChan) {
        return *ic->ic_curchan;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    } else if (is_spoof_check_enabled && mlme_dfs_is_spoof_check_failed(pdev)
            && dfs_region == DFS_FCC_DOMAIN && ieee80211_get_num_vaps_up(ic)) {
        return wlan_get_dev_current_channel(ic);
#endif /* HOST_DFS_SPOOF_TEST */
    } else {
        return *vap->iv_des_chan[vap->iv_des_mode];
    }
}

struct ieee80211_ath_channel
wlan_get_dev_current_channel(wlan_dev_t devhandle)
{
    struct ieee80211com *ic = devhandle;

    return (!IEEE80211_IS_CHAN_VALID(ic->ic_curchan)) ? empty_channel:
        *ic->ic_curchan;
}

struct ieee80211_ath_channel
wlan_get_bss_channel(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;

    return *vap->iv_bsschan;
}

struct ieee80211_ath_channel
wlan_get_des_channel(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    struct ieee80211com *ic = vap->iv_ic;
    int dfs_region;
    bool is_spoof_check_enabled = 0;
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;

    if (ic->ic_is_host_dfs_check_enabled)
        ic->ic_is_host_dfs_check_enabled(pdev, &is_spoof_check_enabled);

    ieee80211_regdmn_get_dfs_region(pdev, (enum dfs_reg *)&dfs_region);

    if (is_spoof_check_enabled && mlme_dfs_is_spoof_check_failed(pdev)
            && dfs_region == DFS_FCC_DOMAIN && ieee80211_get_num_vaps_up(ic)) {
        return wlan_get_dev_current_channel(ic);
    }
#endif /* HOST_DFS_SPOOF_TEST */
    return *vap->iv_des_chan[vap->iv_des_mode];
}


int
wlan_get_channel_list(wlan_dev_t devhandle, uint32_t band_bitmap,
                      qdf_freq_t chanlist[], u_int32_t n,
                      enum supported_6g_pwr_types input_6g_pwr_mode)
{
    struct ieee80211com *ic = devhandle;
    struct regulatory_channel *cur_chan_list;
    int i;
    int nchans = 0;

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list) {
        qdf_err("malloc failed");
        return nchans;
    }

    if (wlan_reg_get_current_chan_list(ic->ic_pdev_obj, cur_chan_list) !=
        QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        qdf_mem_free(cur_chan_list);
        return nchans;
    }

    for (i = 0; i < NUM_CHANNELS; i++) {
        if (n < (nchans + 1)) {
            qdf_mem_free(cur_chan_list);
            return -EOVERFLOW;
        }

        if (!wlan_reg_is_freq_enabled(ic->ic_pdev_obj,
                                      cur_chan_list[i].center_freq,
                                      input_6g_pwr_mode)) {
            continue;
        }

        if (!(band_bitmap & BIT(REG_BAND_2G)) &&
            WLAN_REG_IS_24GHZ_CH_FREQ(cur_chan_list[i].center_freq))
            continue;

        if (!(band_bitmap & BIT(REG_BAND_5G)) &&
            WLAN_REG_IS_5GHZ_CH_FREQ(cur_chan_list[i].center_freq))
            continue;

        if (!(band_bitmap & BIT(REG_BAND_6G)) &&
            WLAN_REG_IS_6GHZ_CHAN_FREQ(cur_chan_list[i].center_freq))
            continue;

        chanlist[nchans++] = cur_chan_list[i].center_freq;
    }

    qdf_mem_free(cur_chan_list);
    return nchans;
}

void
wlan_get_noise_floor(wlan_if_t vaphandle, int16_t *nfBuf)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    ic->ic_get_chainnoisefloor(ic, ic->ic_curchan, nfBuf);
}

int16_t
wlan_get_chan_noise_floor(wlan_if_t vaphandle, u_int16_t freq, u_int64_t flags)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_ath_channel chan;

    chan.ic_freq  = freq;
    chan.ic_flags = flags;

    return ic->ic_get_noisefloor(ic, &chan, 0);
}

u_int32_t
wlan_channel_frequency(wlan_chan_t chan)
{
    return (!IEEE80211_IS_CHAN_VALID(chan)) ? IEEE80211_FREQ_ANY : chan->ic_freq;
}

u_int32_t
wlan_channel_ieee(wlan_chan_t chan)
{
    return (!IEEE80211_IS_CHAN_VALID(chan)) ? IEEE80211_CHAN_ANY : chan->ic_ieee;
}

#ifdef WLAN_FEATURE_11BE
u_int16_t
wlan_channel_puncture_bitmap(wlan_chan_t chan)
{
    return (!IEEE80211_IS_CHAN_VALID(chan)) ? 0 : chan->op_puncture_bitmap;
}

u_int16_t
wlan_channel_reg_punc_bitmap(wlan_chan_t chan)
{
    return (!IEEE80211_IS_CHAN_VALID(chan)) ? 0 : chan->reg_punc_bitmap;
}
#endif

enum ieee80211_phymode
wlan_channel_phymode(wlan_chan_t chan)
{
    return ieee80211_chan2mode(chan);
}

int8_t
wlan_channel_maxpower(wlan_chan_t chan)
{
    return chan->ic_maxregpower;
}

u_int64_t
wlan_channel_flags(wlan_chan_t chan)
{
    return chan->ic_flags;
}

bool
wlan_channel_is_passive(wlan_chan_t chan)
{
    return (IEEE80211_IS_CHAN_PASSIVE(chan));
}

bool
wlan_channel_is_5GHzOdd(wlan_chan_t chan)
{
    return (IEEE80211_IS_CHAN_ODD(chan));
}

bool
wlan_channel_is_dfs(wlan_chan_t chan, bool flagOnly)
{
    if (flagOnly)
        return (IEEE80211_IS_CHAN_DFSFLAG(chan));
    else
        return (IEEE80211_IS_CHAN_DFS(chan));
}

#ifdef CONFIG_AFC_SUPPORT
/**
 * ieee80211_dcs_afc_trigger:
 * Send a request for channel change to DCS for AFC power limit changes.
 *
 * @ic             : Pointer to the ic structure
 *
 * Return:
 * QDF_STATUS_SUCCESS    : Successful
 * QDF_STATUS_E_INVAL    : Failed due parameter error
 * QDF_STATIS_E_NOSUPPORT: Failed due to no ACS support
 */
QDF_STATUS ieee80211_dcs_afc_trigger(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn;

    if (!ic) {
        qdf_err("Invalid ic pointer");
        return QDF_STATUS_E_INVAL;
    }
    scn = OL_ATH_SOFTC_NET80211(ic);

    qdf_info("Received AFC trigger to DCS/ACS");
    return ic->ic_trigger_dcs(ic, NULL, CAP_DCS_AFC);
}
#endif

/**
 * ieee80211_dcs_acs_event_handler:
 * ACS event handler for the DCS algorithm.
 *
 * @arg: Opaque pointer to the osif handler
 * @channel: Channel selected by the algorithm
 *
 * Return: None
 */
void ieee80211_dcs_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);
    wlan_if_t tmpvap = NULL;
    struct ieee80211com *ic = NULL;
    int error = 0;
    wlan_chan_t channel = (wlan_chan_t)arg;
    enum ieee80211_phymode orig_mode;
    struct net_device *tmpdev;

    if (!vap) {
        qdf_err("Received NULL vap");
        goto done;
    }

    orig_mode = vap->iv_des_mode;

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("Invalid ic");
        goto done;
    }

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        qdf_err("Invalid channel");
        error = -1;
        goto done;
    }

#ifdef CONFIG_AFC_SUPPORT
    if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(channel))) {
        error = ieee80211_update_mode_and_power_type(vap,
                                                     channel,
                                                     ieee80211_chan2mode(channel));
        if (error) {
            qdf_err("Failed to set AP power type for ACS channel - Freq %d Mode %d pp %x",
                    channel->ic_freq, ieee80211_chan2mode(channel),
                    ieee80211_get_chan_punc_bitmap(channel));
            goto done;
        }
    }
#endif /* CONFIG_AFC_SUPPORT */

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        /* Start monitor VAPs from this path */
        tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
        if ((tmpvap->iv_opmode == IEEE80211_M_MONITOR) &&
            IS_UP(tmpdev) &&
            (wlan_vdev_is_up_active_state(tmpvap->vdev_obj) !=
                                                    QDF_STATUS_SUCCESS)) {
            qdf_info("Starting monitor VDEV (%d) from DCS ACS evhandler",
                     wlan_vdev_get_id(tmpvap->vdev_obj));
            osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpdev, RESCAN);
        }
    }

    error = wlan_set_ieee80211_channel(vap, channel);
    if (error) {
        qdf_err("Failed to set channel. Error: %d", error);
        goto done;
    }

done:
#ifdef CONFIG_AFC_SUPPORT
    if (error &&
        ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(vap->iv_ic->ic_curchan))) {
        ieee80211_update_mode_and_power_type(vap, vap->iv_ic->ic_curchan,
                                             orig_mode);
        vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
    }
#endif /* CONFIG_AFC_SUPPORT */

    if(ic && ic->ic_dcs_restore) {
        ic->ic_dcs_restore(ic);
    }
}

/*
 * Auto Channel Select handler used for interface up.
 */
static void spectral_eacs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
    struct ieee80211vap *vap = wlan_vdev_get_mlme_ext_obj(vdev);
    wlan_chan_t channel = (wlan_chan_t)arg;
    struct ieee80211com *ic;
    int error = 0;
    enum ieee80211_phymode orig_mode;

    if (!vap) {
        qdf_info("vap is null");
        goto done;
    }
    ic = vap->iv_ic;
    orig_mode = vap->iv_des_mode;

    /* Skip channel set, if channel is ANYC */
    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        /* Restore dcs state so that interference detection will work*/
        error = -1;
        goto done;
    }

#ifdef CONFIG_AFC_SUPPORT
    if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(channel))) {
        error = ieee80211_update_mode_and_power_type(vap, channel,
                                              ieee80211_chan2mode(channel));
        if (error) {
            qdf_err("Failed to change regulatory AP power type");
            goto done;
        }
    }
#endif /* CONFIG_AFC_SUPPORT */

    error = wlan_set_ieee80211_channel(vap, channel);
    if (error !=0) {
        /* Restore dcs state so that interference detection will work*/
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
            "%s : failed to set channel with error code %d\n",
            __func__, error);
        goto done;
    }

done:
#ifdef CONFIG_AFC_SUPPORT
    if (error) {
        if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(vap->iv_ic->ic_curchan))) {
            ieee80211_update_mode_and_power_type(vap, vap->iv_ic->ic_curchan,
                                                 orig_mode);
            vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
        }

        if(ic->ic_dcs_restore) {
            ic->ic_dcs_restore(ic);
        }
    }
#endif /* CONFIG_AFC_SUPPORT */
    return;
}

#ifdef WLAN_FEATURE_11BE

uint16_t ieee80211_get_radar_punc_bitmap(struct ieee80211com *ic,
                                            struct ieee80211_ath_channel *chan)
{
    return ieee80211_reg_get_radar_punc_bitmap(ic, chan);
}

qdf_export_symbol(ieee80211_get_radar_punc_bitmap);

bool
ieee80211_is_user_punc_same_as_radar_bitmap(struct ieee80211com *ic,
                                            struct ieee80211_ath_channel *chan,
                                            uint16_t usr_punc_bitmap)
{
    uint16_t radar_punc_bitmap;

    if (!IEEE80211_IS_CHAN_PUNCTURABLE(chan))
        return false;

    radar_punc_bitmap = ieee80211_reg_get_radar_punc_bitmap(ic, chan);

    /* If there is no radar punctured channels, we can allow the user
     * configured puncture pattern.
     */
    if (!radar_punc_bitmap)
        return true;

    return ((radar_punc_bitmap & usr_punc_bitmap) == radar_punc_bitmap);
}
#else
static inline bool
ieee80211_is_user_punc_same_as_radar_bitmap(struct ieee80211com *ic,
                                            struct ieee80211_ath_channel *chan,
                                            uint16_t usr_punc_bitmap)
{
    return false;
}
#endif

/**
* @brief            set known ieee802111_channel to vap
*
* @param vaphandle  handle to vap for which channel has to be set
* @param channel    handle to ieee80211_channel
*
* @return:          0 for success, -ve error code other wise.
*/
int
wlan_set_ieee80211_channel(wlan_if_t vaphandle,
                           struct ieee80211_ath_channel *channel)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211vap *tmp_vap = NULL;
    struct chan_params chan_p = {0};
    bool bridge_vap_present = false;

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                "%s: vap-%d NULL channel specified\n", __func__, vap->iv_unit);
        return -EINVAL;
    }

/* The default value of this variable is false.When NOL violation is reported
 * by FW in vap's start response, during restart of the vap, it will be reset
 * to true in dfs_action. After this if user again tries to set another NOL
 * channel using iwconfig athx chan "NOL_chan", if is variable is remains to
 * set to true, no action will be taken on vap's start failure from FW. Hence
 * resetting it here.
 */
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    if (vap->vap_start_failure_action_taken)
        vap->vap_start_failure_action_taken = false;
#endif

    if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
        (ieee80211_check_chan_mode_consistency(ic,vap->iv_des_mode,channel))) {
        return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE
    if (IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(ic, channel,
                                                       channel->op_puncture_bitmap))
#else
    /* don't allow to change to channel with radar found */
    if (IEEE80211_IS_CHAN_RADAR(ic, channel))
#endif
    {
        return -EINVAL;
    }

    /* we are ready to change channel, restore some ic flag , EV#133678 */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        IEEE80211_DISABLE_PROTECTION(ic);
        ieee80211_set_protmode(ic);
        if (ic->ic_caps & IEEE80211_C_SHPREAMBLE) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,"%s: restore short preamble setting\n", __func__);
                ieee80211com_set_flags(ic, IEEE80211_F_SHPREAMBLE);
                ieee80211com_clear_flags(ic, IEEE80211_F_USEBARKER);
        }
    }

    /* mark desired channel */
    *(vap->iv_des_chan[vap->iv_des_mode]) = *channel;
#ifdef QCA_DFS_BW_EXPAND
    /* Store the User configured channel and phymode for BW_Expand feature */
    utils_dfs_set_bw_expand_channel(ic->ic_pdev_obj,
                                    channel->ic_freq,
                                    phymode2convphymode[vap->iv_des_mode]);
#endif
    /*
     * 2.4 GHz Channel Switch Announcement would be disabled in default.
     * Due to maybe some STAs do not handle the CSA in 2.4 GHz very well.
     * Also make sure the channel change is not due to interference. In this
     * case, do not wait for CSA
     */
    if ((ieee80211_ic_2g_csa_is_set(ic)) &&
        (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)  && !(ic->cw_inter_found)) {
        chan_p.freq = channel->ic_freq;
        chan_p.cfreq2 = channel->ic_vhtop_freq_seg2;
        set_chan_params_puncture_bitmap(&chan_p, channel->op_puncture_bitmap);
        chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE;
        ieee80211_ucfg_set_chanswitch(vap, &chan_p, IEEE80211_2GCSA_TBTTCOUNT, 0);
        return 0;
    }
    if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
            /* In case of multi BSSID, make sure all the VAPs change channel */
            TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                /* This is needed to make the beacon is re-initlized */
                tmp_vap->channel_change_done = 1;
                IEEE80211_DPRINTF(tmp_vap, IEEE80211_MSG_STATE, "%s switch channel %d freq %d\n",
                                    __func__, ic->ic_curchan->ic_ieee,ic->ic_curchan->ic_freq);
            }
    }

    /*
     * Do a channel change only when we're already in the RUN or DFSWAIT state.
     * The MLME will pickup the desired channel later.
     */

    /*
     * TBD: If curchan = channel, still need to set channel again to pass
     * SendRecv_ext in ndistest.
     */
    TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
        if (IEEE80211_IS_MLO_BRIDGE_VAP(tmp_vap) &&
            (wlan_vdev_chan_config_valid(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS))
            bridge_vap_present = true;
    }

    if ((wlan_vdev_chan_config_valid(vap->vdev_obj) == QDF_STATUS_SUCCESS) ||
        (ic->cw_inter_found) || bridge_vap_present) {// && ic->ic_curchan != channel) {

        /* AP vap skips CAC if iv_bsschan != ic_curchan.
         * Therefore change the iv_bsschan per vap before channel change.
         */
        TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
            *(tmp_vap->iv_bsschan) = *channel;
            ieee80211_update_vdev_chan(tmp_vap->vdev_obj->vdev_mlme.des_chan,
                                       channel);
        }

        ieee80211_set_channel(ic, channel);
        {
            /* In case of multi BSSID, make sure all the VAPs change channel */
            TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                /* clear erpupdate bit */
                ieee80211_vap_erpupdate_clear(tmp_vap);
                /* This is needed to make the beacon is re-initlized */
                tmp_vap->channel_change_done = 1;
                IEEE80211_DPRINTF(tmp_vap, IEEE80211_MSG_STATE, "%s switch channel %d freq %d\n",
                                    __func__, ic->ic_curchan->ic_ieee,ic->ic_curchan->ic_freq);
            }
        }
    }
    return 0;
}

int
wlan_set_channel(wlan_if_t vaphandle, struct chan_params *chan_p)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_ath_channel channel_obj = {0};
    struct ieee80211_ath_channel *channel = &channel_obj;
    struct ieee80211vap *tmp_vap = NULL;
#if UMAC_SUPPORT_ACS
    struct wlan_mbss_ev_data mbss_ev = {0};
#endif /* UMAC_SUPPORT_ACS */

    if (chan_p == NULL) {
        return -EINVAL;
    }

    if ((chan_p->freq > IEEE80211_FREQ_MAX) && (chan_p->freq != (uint16_t)IEEE80211_FREQ_ANY))
        return -EINVAL;

    if (chan_p->freq == (uint16_t)IEEE80211_FREQ_ANY) {

        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {

#if UMAC_SUPPORT_ACS
            /* Prevent ACS for special vap, user is supposed to fix channel */
            if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap)) {
                qdf_err("ACS is not supported with special vap");
                return -EINVAL;
            }

            /* allow IEEE80211_CHAN_ANYC for auto channel select in AP mode*/
            *(vap->iv_des_chan[vap->iv_des_mode]) = empty_channel;
            /* Trigger EACS only when vap is ready or channel change issued by DCS module */
            if ((wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) || (ic->cw_inter_found)) {
              mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_ACS;
              mbss_ev.ext_cb = &spectral_eacs_event_handler;
              mbss_ev.source.acs_source = MBSS_ACS_SRC_SPECTRAL;
              wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
            }
#endif /* UMAC_SUPPORT_ACS */
            return 0;
        } else {
            /* select the desired channel for the desired PHY mode */
            TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                if (tmp_vap && (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP)) {
                    *channel = *(tmp_vap->iv_des_chan[vap->iv_des_mode]);
                    if (IEEE80211_IS_CHAN_VALID(channel)) {
                        break;
                    }
                }
            }
            if (!IEEE80211_IS_CHAN_VALID(channel))
                return -EINVAL;
        }
    } else {
        /*
         * find the corresponding channel object for the desired PHY mode.
         */
        *channel = ieee80211_find_dot11_channel(ic, chan_p, vap->iv_des_mode);
        if (!IEEE80211_IS_CHAN_VALID(channel)) {
            chan_p->cfreq2 = 0;
            *channel = ieee80211_find_dot11_channel(ic, chan_p, IEEE80211_MODE_AUTO);
            if (!IEEE80211_IS_CHAN_VALID(channel))
                return -EINVAL;
        }
    }

    return wlan_set_ieee80211_channel(vap, channel);
}

/* Structure used in the ieee80211_vap_iter_get_bss_chan() to get the BSS frequencies */
struct ieee80211_iter_vaps_get_bss_chan_arg {
    int         max_list_size;
    int         current_list_size;
    u_int32_t   *bss_freq_list;
    int         ret_param;
};

static void ieee80211_vap_iter_get_bss_chan(void *arg, wlan_if_t vap)
{
    struct ieee80211_iter_vaps_get_bss_chan_arg *params;

    params = (struct ieee80211_iter_vaps_get_bss_chan_arg *) arg;

    if (params->ret_param != EOK) {
        return;
    }

    if (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        /* Get the BSS channel and its frequency */
        wlan_chan_t     bsschan;
        int             i;
        u_int32_t       bss_freq;

        bsschan = vap->iv_bsschan;
        if (IEEE80211_IS_CHAN_VALID(bsschan)) {
            bss_freq = wlan_channel_frequency(bsschan);
        }
        else {
            return;
        }

        /* Check whether it is a duplicate */
        for (i = 0; i < params->current_list_size; i++) {
            if (params->bss_freq_list[i] == bss_freq) {
                /* A match */
                return;
            }
        }
        /* Else no match */
        params->current_list_size++;
        if (params->current_list_size >= params->max_list_size) {
            /* No more space */
            params->ret_param = -EOVERFLOW;
            return;
        }
        params->bss_freq_list[params->current_list_size - 1] = bss_freq;
    }
}

/*
 * Get the list of BSS channel used by other active VAPs. Note that as VAPs go up and down,
 * this list can change with time.
 * bss_freq_list contains an array of frequencies to return the BSS channels used by
 * other active VAPs. On function entry, list_size contains the size of array bss_freq_list.
 * On exit, list_size contains the size of BSS frequency list filled. If no other active VAPs,
 * then list_size will be zero.
 */
int
wlan_get_bss_chan_list(wlan_dev_t devhandle, u_int32_t *bss_freq_list, int *list_size)
{
    struct ieee80211_iter_vaps_get_bss_chan_arg params;

    if (*list_size <= 0) {
        /* Empty list */
        return -EOVERFLOW;
    }

    OS_MEMZERO(&params, sizeof(params));

    params.max_list_size = *list_size;
    params.bss_freq_list = bss_freq_list;
    params.ret_param = EOK;

    wlan_iterate_vap_list(devhandle, ieee80211_vap_iter_get_bss_chan,(void *) &params);

    /* Return the number of BSS channels found */
    *list_size = params.current_list_size;

    return params.ret_param;
}

/*
 * Calculate the channel width of the given umac channel.
 *
 * XXX verify that half/quarter rates are always 5/10MHz, or
 *     whether it should take the actual channel width
 *     (20/40/80) into account (ie, by dividing it by 2 or 4.)
 */
int
ieee80211_get_chan_width(struct ieee80211_ath_channel *chan)
{
    if (IEEE80211_IS_CHAN_QUARTER(chan))
        return CHWIDTH_5;
    else if (IEEE80211_IS_CHAN_HALF(chan))
        return CHWIDTH_10;

    switch (chan->ic_flags & IEEE80211_CHAN_WIDTH_MASK) {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CHAN_320MHZ:
            return CHWIDTH_320;
#endif /* WLAN_FEATURE_11BE */
        case IEEE80211_CHAN_80_80MHZ:
        case IEEE80211_CHAN_160MHZ:
            return CHWIDTH_160;
        case IEEE80211_CHAN_80MHZ:
            return CHWIDTH_80;
        case IEEE80211_CHAN_40PLUS:
        case IEEE80211_CHAN_40MINUS:
            return CHWIDTH_40;
        default:
            return CHWIDTH_20;
    }
}

/*
 * ieee80211_get_chan_width_from_phy_chwidth:
 * Get the CHWIDTH_20 integer from a given phy_ch_width value
 *
 * @ch_width: Value of ch_width
 *
 * Return:
 * Integer value of channel width
 */
static int ieee80211_get_chan_width_from_phy_chwidth(enum phy_ch_width ch_width)
{
    switch (ch_width) {
        case CH_WIDTH_20MHZ:
            return CHWIDTH_20;
        break;
        case CH_WIDTH_40MHZ:
            return CHWIDTH_40;
        break;
        case CH_WIDTH_80MHZ:
            return CHWIDTH_80;
        break;
        case CH_WIDTH_160MHZ:
            return CHWIDTH_160;
        break;
        case CH_WIDTH_80P80MHZ:
            return CHWIDTH_160;
        break;
        case CH_WIDTH_5MHZ:
            return CHWIDTH_5;
        break;
        case CH_WIDTH_10MHZ:
            return CHWIDTH_10;
        break;
#ifdef WLAN_FEATURE_11BE
        case CH_WIDTH_320MHZ:
            return CHWIDTH_320;
        break;
#endif /* WLAN_FEATURE_11BE */
        default:
            /* Invalid case */
            return 0;
        break;
    }
}

enum phy_ch_width
ieee80211_get_phy_chan_width(struct ieee80211_ath_channel *chan)
{
    if (IEEE80211_IS_CHAN_QUARTER(chan))
        return CH_WIDTH_5MHZ;
    else if (IEEE80211_IS_CHAN_HALF(chan))
        return CH_WIDTH_10MHZ;

    switch (chan->ic_flags & IEEE80211_CHAN_WIDTH_MASK) {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CHAN_320MHZ:
            return CH_WIDTH_320MHZ;
#endif /* WLAN_FEATURE_11BE */
        case IEEE80211_CHAN_160MHZ:
            return CH_WIDTH_160MHZ;
        case IEEE80211_CHAN_80_80MHZ:
            return CH_WIDTH_80P80MHZ;
        case IEEE80211_CHAN_80MHZ:
            return CH_WIDTH_80MHZ;
        case IEEE80211_CHAN_40PLUS:
        case IEEE80211_CHAN_40MINUS:
            return CH_WIDTH_40MHZ;
        default:
            return CH_WIDTH_20MHZ;
    }

    return CH_WIDTH_INVALID;
}
qdf_export_symbol(ieee80211_get_phy_chan_width);

enum ieee80211_cwm_width
ieee80211_get_cwm_width(enum phy_ch_width chwidth)
{
    switch (chwidth) {
        case CH_WIDTH_20MHZ:
            return IEEE80211_CWM_WIDTH20;
        case CH_WIDTH_40MHZ:
            return IEEE80211_CWM_WIDTH40;
        case CH_WIDTH_80MHZ:
            return IEEE80211_CWM_WIDTH80;
        case CH_WIDTH_160MHZ:
            return IEEE80211_CWM_WIDTH160;
#ifdef WLAN_FEATURE_11BE
        case CH_WIDTH_320MHZ:
            return IEEE80211_CWM_WIDTH320;
#endif
        default:
            return IEEE80211_CWM_WIDTH20;
    }
}

enum phy_ch_width
ieee80211_get_chan_width_from_phymode(enum ieee80211_phymode mode)
{
    switch(mode) {
    case IEEE80211_MODE_AUTO:
    case IEEE80211_MODE_11A:
    case IEEE80211_MODE_11B:
    case IEEE80211_MODE_11G:
    case IEEE80211_MODE_FH:
    case IEEE80211_MODE_TURBO_A:
    case IEEE80211_MODE_TURBO_G:
    case IEEE80211_MODE_11NA_HT20:
    case IEEE80211_MODE_11NG_HT20:
    case IEEE80211_MODE_11AC_VHT20:
    case IEEE80211_MODE_11AXA_HE20:
    case IEEE80211_MODE_11AXG_HE20:
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT20:
    case IEEE80211_MODE_11BEG_EHT20:
#endif /* WLAN_FEATURE_11BE */
        return CH_WIDTH_20MHZ;
    case IEEE80211_MODE_11NA_HT40PLUS:
    case IEEE80211_MODE_11NA_HT40MINUS:
    case IEEE80211_MODE_11NG_HT40PLUS:
    case IEEE80211_MODE_11NG_HT40MINUS:
    case IEEE80211_MODE_11NG_HT40:
    case IEEE80211_MODE_11NA_HT40:
    case IEEE80211_MODE_11AC_VHT40PLUS:
    case IEEE80211_MODE_11AC_VHT40MINUS:
    case IEEE80211_MODE_11AC_VHT40:
    case IEEE80211_MODE_11AXA_HE40PLUS:
    case IEEE80211_MODE_11AXA_HE40MINUS:
    case IEEE80211_MODE_11AXG_HE40PLUS:
    case IEEE80211_MODE_11AXG_HE40MINUS:
    case IEEE80211_MODE_11AXA_HE40:
    case IEEE80211_MODE_11AXG_HE40:
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT40PLUS:
    case IEEE80211_MODE_11BEA_EHT40MINUS:
    case IEEE80211_MODE_11BEG_EHT40PLUS:
    case IEEE80211_MODE_11BEG_EHT40MINUS:
    case IEEE80211_MODE_11BEA_EHT40:
    case IEEE80211_MODE_11BEG_EHT40:
#endif /* WLAN_FEATURE_11BE */
        return CH_WIDTH_40MHZ;
    case IEEE80211_MODE_11AC_VHT80:
    case IEEE80211_MODE_11AXA_HE80:
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT80:
#endif /* WLAN_FEATURE_11BE */
        return CH_WIDTH_80MHZ;
    case IEEE80211_MODE_11AC_VHT160:
    case IEEE80211_MODE_11AXA_HE160:
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT160:
#endif /* WLAN_FEATURE_11BE */
        return CH_WIDTH_160MHZ;
    case IEEE80211_MODE_11AC_VHT80_80:
    case IEEE80211_MODE_11AXA_HE80_80:
        return CH_WIDTH_80P80MHZ;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT320:
        return CH_WIDTH_320MHZ;
#endif /* WLAN_FEATURE_11BE */
    default:
        return CH_WIDTH_INVALID;
    }
}

/*
 * Calculate the centre frequency of the given channel.
 *
 * XXX doesn't know about VHT 80+80 yet!
 * XXX half/quarter rates!
 */
int
ieee80211_get_chan_centre_freq(struct ieee80211com *ic,
    struct ieee80211_ath_channel *chan)
{
    int chan_centre, chan_offset, chan_width;

    /*
     * XXX Only >= VHT80/HE80 channels have freq1/freq2 setup.
     */
    if (IEEE80211_IS_CHAN_80MHZ(chan) || IEEE80211_IS_CHAN_80_80MHZ(chan)) {
        /* 11AC/11AX, so cfreq1/cfreq2 are setup */

		/*
		 * XXX center frequency of only first segment is provided even for 80+80
		 */
		chan_centre = chan->ic_vhtop_freq_seg1;
    } else if (IEEE80211_IS_CHAN_160MHZ(chan)
#ifdef WLAN_FEATURE_11BE
               || IEEE80211_IS_CHAN_320MHZ(chan)
#endif
                ) {

        /* For 160 MHz and 320 MHz, centre frequency is ic_vhtop_ch_freq_seg2 */
        chan_centre = chan->ic_vhtop_freq_seg2;
    } else {
        /* 20 MHz and 40 MHz channels */

        /*
         * XXX this is hard-coded - it should be 5 or 10 for
         * half/quarter appropriately.
         */
        chan_width = 20;

        /* Grab default channel centre */
        chan_centre = ieee80211_chan2freq(ic, chan);

        /* Calculate offset based on HT40U/HT40D */
        if (IEEE80211_IS_CHAN_40PLUS(chan))
            chan_offset = chan_width;
        else if (IEEE80211_IS_CHAN_40MINUS(chan))
            chan_offset = -chan_width;
        else
            chan_offset = 0;

        /* Calculate new _real_ channel centre */
        chan_centre += (chan_offset / 2);
    }

    /*
     * XXX TODO: add half/quarter rate support!
     */

    return (chan_centre);
}

/*
 * Check whether cp falls within p1 and p2, exclusively.
 */
static int
ol_check_intersect_excl(int p1, int p2, int cp)
{

	return (cp > p1 && cp < p2);
}

/*
 * Check whether two frequency centre/channel width entries
 * overlap in any way.
 *
 * This is an exclusive overlap - ie, two touching edges
 * do not match.
 *
 * XXX very likely not optimal at all!
 */
int
ieee80211_check_overlap(int f1, int w1, int f2, int w2)
{
	int f1_h, f1_l;
	int f2_h, f2_l;
	int ret;

	/* Calculate low/high frequency ranges */
	f1_l = f1 - (w1 / 2);
	f1_h = f1 + (w1 / 2);

	f2_l = f2 - (w2 / 2);
	f2_h = f2 + (w2 / 2);

	/*
	 * Since the intersect check is exclusive, two
	 * exact overlaps won't actually work as each
	 * edge only touches the other edge.  It's not
	 * considered inside a range.  Hence why there's
	 * a separate "is totally equal" check.
	 */
	ret = (
	    ((f1 == f2) && (w1 == w2)) ||
	    ol_check_intersect_excl(f1_l, f1_h, f2_l) ||
	    ol_check_intersect_excl(f1_l, f1_h, f2_h) ||
	    ol_check_intersect_excl(f2_l, f2_h, f1_l) ||
	    ol_check_intersect_excl(f2_l, f2_h, f1_h)
	);

	return (ret);
}

void
ieee80211_get_extchaninfo(struct ieee80211com *ic,
        struct ieee80211_ath_channel *chan,
        struct ath_sub_chan_freq_list *chan_info)
{
    qdf_freq_t chan_center = chan->ic_vhtop_freq_seg1;

    switch (chan->ic_flags & IEEE80211_CHAN_WIDTH_MASK) {
    case IEEE80211_CHAN_20MHZ:
        chan_info->cl_nchans = 1;
        chan_info->sub_20chan_freq[0] = chan_center;
    break;
    case IEEE80211_CHAN_40PLUS:
    case IEEE80211_CHAN_40MINUS:
        chan_info->cl_nchans = 2;
        chan_info->sub_20chan_freq[0] = chan_center - 10;
        chan_info->sub_20chan_freq[1] = chan_center + 10;
    break;
    case IEEE80211_CHAN_80MHZ:
        chan_center = chan->ic_vhtop_freq_seg1;
        chan_info->cl_nchans = 4;
        chan_info->sub_20chan_freq[0] = chan_center - 30;
        chan_info->sub_20chan_freq[1] = chan_center - 10;
        chan_info->sub_20chan_freq[2] = chan_center + 10;
        chan_info->sub_20chan_freq[3] = chan_center + 30;
    break;
    case IEEE80211_CHAN_160MHZ:
        chan_center = chan->ic_vhtop_freq_seg2;
        chan_info->cl_nchans = 8;
        chan_info->sub_20chan_freq[0] = chan_center - 70;
        chan_info->sub_20chan_freq[1] = chan_center - 50;
        chan_info->sub_20chan_freq[2] = chan_center - 30;
        chan_info->sub_20chan_freq[3] = chan_center - 10;
        chan_info->sub_20chan_freq[4] = chan_center + 70;
        chan_info->sub_20chan_freq[5] = chan_center + 50;
        chan_info->sub_20chan_freq[6] = chan_center + 30;
        chan_info->sub_20chan_freq[7] = chan_center + 10;
    break;
    case IEEE80211_CHAN_80_80MHZ:
        chan_center = chan->ic_vhtop_freq_seg1;
        chan_info->cl_nchans = 8;
        chan_info->sub_20chan_freq[0] = chan_center - 30;
        chan_info->sub_20chan_freq[1] = chan_center - 10;
        chan_info->sub_20chan_freq[2] = chan_center + 10;
        chan_info->sub_20chan_freq[3] = chan_center + 30;

        chan_center = chan->ic_vhtop_freq_seg2;

        chan_info->sub_20chan_freq[4] = chan_center - 30;
        chan_info->sub_20chan_freq[5] = chan_center - 10;
        chan_info->sub_20chan_freq[6] = chan_center + 10;
        chan_info->sub_20chan_freq[7] = chan_center + 30;
    break;
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_CHAN_320MHZ:
        chan_center = chan->ic_vhtop_freq_seg2;
        chan_info->cl_nchans = 16;
        chan_info->sub_20chan_freq[0] = chan_center - 150;
        chan_info->sub_20chan_freq[1] = chan_center - 130;
        chan_info->sub_20chan_freq[2] = chan_center - 110;
        chan_info->sub_20chan_freq[3] = chan_center - 90;
        chan_info->sub_20chan_freq[4] = chan_center - 70;
        chan_info->sub_20chan_freq[5] = chan_center - 50;
        chan_info->sub_20chan_freq[6] = chan_center - 30;
        chan_info->sub_20chan_freq[7] = chan_center - 10;
        chan_info->sub_20chan_freq[8] = chan_center + 10;
        chan_info->sub_20chan_freq[9] = chan_center + 30;
        chan_info->sub_20chan_freq[10] = chan_center + 50;
        chan_info->sub_20chan_freq[11] = chan_center + 70;
        chan_info->sub_20chan_freq[12] = chan_center + 90;
        chan_info->sub_20chan_freq[13] = chan_center + 110;
        chan_info->sub_20chan_freq[14] = chan_center + 130;
        chan_info->sub_20chan_freq[15] = chan_center + 150;
    break;
#endif
    default:
        qdf_err("Invalid channel width flag %llx",
                chan->ic_flags & IEEE80211_CHAN_WIDTH_MASK);
        chan_info->cl_nchans = 0;
        break;
    }
}

/***
  Check whether missing extchan is 144
**/
u_int8_t ieee80211_is_extchan_144(struct ieee80211com *ic,
           struct ieee80211_ath_channel *chan, u_int8_t extchan)
{
   u_int16_t chan_center = 0;
   u_int16_t chan_center2 = 0;
   u_int16_t chan_freq = chan->ic_freq;

   if(IEEE80211_IS_CHAN_11AC_VHT80(chan)) {
      chan_center = chan->ic_vhtop_freq_seg1;
   }
   if(IEEE80211_IS_CHAN_11AC_VHT80_80(chan)) {
      chan_center = chan->ic_vhtop_freq_seg1;
      chan_center2 = chan->ic_vhtop_freq_seg2;
   }
   /* Check whether the channel freq is with in the range */
   /* if it is primary, extchan 144 index is 3 */
   /* if it is secondary, extchan 144 index is 7 */
   if(((chan_center == 5690) && (extchan == 3))||
      ((chan_center2 == 5690) && (extchan == 7))) {
      return 1;
   }
   /* For HT40+, extchan 144 index is 1 */
   if (chan_freq == 5700) {
      if((IEEE80211_IS_CHAN_11NA_HT40PLUS(chan) ||
            IEEE80211_IS_CHAN_11AC_VHT40PLUS(chan)) &&
         (extchan == 1)) {
         return 1;
      }
   }
   return 0;
}

/**
 * @brief finds the offset for secondary twenty channel
 *
 * @param chan: channel for which secondary offset to find
 */

enum sec20_chan_offset
ieee80211_secondary20_channel_offset(struct ieee80211_ath_channel *chan)
{

    int8_t pri_center_ch_diff, sec_level;
    u_int16_t pri_chan_40_center;

    if (!chan || IEEE80211_IS_CHAN_A(chan) ||
        IEEE80211_IS_CHAN_B(chan) || IEEE80211_IS_CHAN_G(chan) ||
        IEEE80211_IS_CHAN_PUREG(chan) || IEEE80211_IS_CHAN_ANYG(chan) ||
        IEEE80211_IS_CHAN_20MHZ(chan)) {
        /* No secondary channel */
        return EXT_CHAN_OFFSET_NA;
    }

    if (IEEE80211_IS_CHAN_40PLUS(chan))
        return EXT_CHAN_OFFSET_ABOVE;

    if (IEEE80211_IS_CHAN_40MINUS(chan))
        return EXT_CHAN_OFFSET_BELOW;

    if (IEEE80211_IS_CHAN_80MHZ(chan) ||
            IEEE80211_IS_CHAN_80_80MHZ(chan)) {
        /* The following logic generates the extension channel offset from
         * the primary channel(ic_ieee) and 80M channel central frequency.
         * The channelization for 80M is as following:
         * | 20M  20M  20M  20M | with the following example | 36 40 44 48 |
         * |         80M        |                |     80M     |
         * The central frequency is 42 in the example.
         * If the primary channel is 36 and 44, the extension channel is 40PLUS.
         * If the primary channel is 40 and 48 the extension channel is 40MINUS.
         */

        if (chan->ic_ieee < chan->ic_vhtop_ch_num_seg1) {
            if ((chan->ic_vhtop_ch_num_seg1 - chan->ic_ieee) > 4) {
                return EXT_CHAN_OFFSET_ABOVE;
            } else {
                return EXT_CHAN_OFFSET_BELOW;
            }
        } else {
            if ((chan->ic_ieee - chan->ic_vhtop_ch_num_seg1) > 4) {
                return EXT_CHAN_OFFSET_BELOW;
            } else {
                return EXT_CHAN_OFFSET_ABOVE;
            }
        }
    }

    if (IEEE80211_IS_CHAN_160MHZ(chan)) {
        /* The channelization of 160M is as following:
         * | 20M 20M 20M 20M 20M 20M 20M 20M | with following example |
         * 36 40 44 48 52 56 60 64 | The center frequency is 40 in this example.
         * If primary channel is 36, 44, 52 or 60, the extension channel is 40PLUS.
         * If primary channel is 40, 48, 56 or 64, the extension channel is 40MINUS.
         */

        pri_center_ch_diff = chan->ic_ieee - chan->ic_vhtop_ch_num_seg2;

        if (pri_center_ch_diff > 0) {
            sec_level = -1;
        } else {
            sec_level = 1;
        }

        if (sec_level*pri_center_ch_diff < -6) {
            pri_chan_40_center = chan->ic_vhtop_ch_num_seg2 - (2*sec_level*6);
        } else {
            pri_chan_40_center = chan->ic_vhtop_ch_num_seg2 - (2*sec_level*2);
        }

        if (pri_chan_40_center > chan->ic_ieee) {
            return EXT_CHAN_OFFSET_ABOVE;
        } else {
            return EXT_CHAN_OFFSET_BELOW;
        }
    }

#ifdef WLAN_FEATURE_11BE
    if (IEEE80211_IS_CHAN_320MHZ(chan)) {
        pri_center_ch_diff = chan->ic_ieee - chan->ic_vhtop_ch_num_seg1;

        if (pri_center_ch_diff > 0) {
            sec_level = -1;
        } else {
            sec_level = 1;
        }

        if (sec_level*pri_center_ch_diff < -6) {
            pri_chan_40_center = chan->ic_vhtop_ch_num_seg1 - (2*sec_level*6);
        } else {
            pri_chan_40_center = chan->ic_vhtop_ch_num_seg1 - (2*sec_level*2);
        }

        if (pri_chan_40_center > chan->ic_ieee) {
            return EXT_CHAN_OFFSET_ABOVE;
        } else {
            return EXT_CHAN_OFFSET_BELOW;
        }
    }
#endif /* WLAN_FEATURE_11BE */

    return EXT_CHAN_OFFSET_NA;
}

uint8_t
ieee80211_sec_chan_offset(struct ieee80211com *ic,
                          struct ieee80211_ath_channel *chan)
{
    int     offset      = 0;
    uint8_t secoffset   = 0;
    struct ieee80211_ath_channel target_chan;

    target_chan = *chan;

#ifdef WLAN_FEATURE_11BE
    if (IEEE80211_IS_CHAN_11BE(chan))
        target_chan = ieee80211_find_legacy_non11be_channel(ic, chan);
#endif

    offset = ieee80211_secondary20_channel_offset(&target_chan);
    switch (offset) {
        case EXT_CHAN_OFFSET_ABOVE:
            secoffset = IEEE80211_SEC_CHAN_OFFSET_SCA;
            break;
        case EXT_CHAN_OFFSET_BELOW:
            secoffset = IEEE80211_SEC_CHAN_OFFSET_SCB;
            break;
        case EXT_CHAN_OFFSET_NA:
        default:
            secoffset = IEEE80211_SEC_CHAN_OFFSET_SCN;
            break;
    }
    return secoffset;
}


/**
* @brief finds current mode of underlying
         radio interface (like a/b/g/n/ac/ax etc)
*
* @param chan: Pointer to the channel
*
* @return current phymode or invalid
*/
enum ieee80211_mode ieee80211_get_mode(const struct ieee80211_ath_channel *ch)
{
    enum ieee80211_mode mode = IEEE80211_MODE_INVALID;

    if (IEEE80211_IS_CHAN_A(ch)) {
        mode = IEEE80211_MODE_A;
    } else if (IEEE80211_IS_CHAN_B(ch)) {
        mode = IEEE80211_MODE_B;
    } else if (IEEE80211_IS_CHAN_G(ch) ||
            IEEE80211_IS_CHAN_PUREG(ch) ||
            IEEE80211_IS_CHAN_ANYG(ch)) {
        mode = IEEE80211_MODE_G;
    } else if (IEEE80211_IS_CHAN_TURBO(ch)) {
        mode = IEEE80211_MODE_TURBO;
    } else if (IEEE80211_IS_CHAN_11NG(ch) ||
               IEEE80211_IS_CHAN_11NA(ch)) {
        mode = IEEE80211_MODE_N;
    } else if (IEEE80211_IS_CHAN_VHT(ch)) {
        mode = IEEE80211_MODE_AC;
    } else if (IEEE80211_IS_CHAN_11AXG(ch) ||
               IEEE80211_IS_CHAN_11AXA(ch)) {
        mode = IEEE80211_MODE_AX;
#ifdef WLAN_FEATURE_11BE
    } else if (IEEE80211_IS_CHAN_11BEG(ch) ||
               IEEE80211_IS_CHAN_11BEA(ch)) {
        mode = IEEE80211_MODE_BE;
#endif /* WLAN_FEATURE_11BE */
    }

    return mode;
}

/**
 * get_mode_from_phymode() - get phy type from phymode
 * @phymode: phymode
 *
 * get phy type from phymode
 *
 * Return: ieee80211_mode corrosponding to the phy mode
 */
enum ieee80211_mode get_mode_from_phymode(enum ieee80211_phymode phymode) {
    enum ieee80211_mode mode = IEEE80211_MODE_INVALID;

    switch(phymode) {
        case IEEE80211_MODE_AUTO:
            mode = IEEE80211_MODE_INVALID;
            break;
        case IEEE80211_MODE_11A:
            mode = IEEE80211_MODE_A;
            break;
        case IEEE80211_MODE_11B:
            mode = IEEE80211_MODE_B;
            break;
        case IEEE80211_MODE_11G:
            mode = IEEE80211_MODE_G;
            break;
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NA_HT40:
            mode = IEEE80211_MODE_NA;
            break;
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40:
            mode = IEEE80211_MODE_NG;
            break;
        case IEEE80211_MODE_11AC_VHT20:
        case IEEE80211_MODE_11AC_VHT40PLUS:
        case IEEE80211_MODE_11AC_VHT40MINUS:
        case IEEE80211_MODE_11AC_VHT40:
        case IEEE80211_MODE_11AC_VHT80:
        case IEEE80211_MODE_11AC_VHT160:
        case IEEE80211_MODE_11AC_VHT80_80:
            mode = IEEE80211_MODE_AC;
            break;
        case IEEE80211_MODE_11AXA_HE20:
        case IEEE80211_MODE_11AXA_HE40PLUS:
        case IEEE80211_MODE_11AXA_HE40MINUS:
        case IEEE80211_MODE_11AXA_HE40:
        case IEEE80211_MODE_11AXA_HE80:
        case IEEE80211_MODE_11AXA_HE160:
        case IEEE80211_MODE_11AXA_HE80_80:
            mode = IEEE80211_MODE_AXA;
            break;
        case IEEE80211_MODE_11AXG_HE20:
        case IEEE80211_MODE_11AXG_HE40PLUS:
        case IEEE80211_MODE_11AXG_HE40MINUS:
        case IEEE80211_MODE_11AXG_HE40:
            mode = IEEE80211_MODE_AXG;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT20:
        case IEEE80211_MODE_11BEA_EHT40PLUS:
        case IEEE80211_MODE_11BEA_EHT40MINUS:
        case IEEE80211_MODE_11BEA_EHT40:
        case IEEE80211_MODE_11BEA_EHT80:
        case IEEE80211_MODE_11BEA_EHT160:
        case IEEE80211_MODE_11BEA_EHT320:
            mode = IEEE80211_MODE_BEA;
            break;
        case IEEE80211_MODE_11BEG_EHT20:
        case IEEE80211_MODE_11BEG_EHT40PLUS:
        case IEEE80211_MODE_11BEG_EHT40MINUS:
        case IEEE80211_MODE_11BEG_EHT40:
            mode = IEEE80211_MODE_BEG;
            break;
#endif /* WLAN_FEATURE_11BE */
        default:
            mode = IEEE80211_MODE_INVALID;
            break;
    }

    return mode;
}

qdf_export_symbol(get_mode_from_phymode);

/**
* @brief calclulates composite phymode from given mode,
         chwidth and secondary channel offset.
*
* @param mode       :   a/b/g/n/ac etc
* @param chwidth    :   channel width
* @param secchanoffset: secondary channel offset.
*
* @return ieee80211_phymode
*/
enum ieee80211_phymode ieee80211_get_composite_phymode (enum ieee80211_mode mode,
        enum ieee80211_cwm_width chwidth, uint8_t secchanoffset)
{
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;

    switch(mode) {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_BEA:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11BEA_EHT20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11BEA_EHT40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11BEA_EHT40MINUS;
                }
            } else if (chwidth == IEEE80211_CWM_WIDTH80) {
                phymode = IEEE80211_MODE_11BEA_EHT80;
            } else if (chwidth == IEEE80211_CWM_WIDTH160) {
                phymode = IEEE80211_MODE_11BEA_EHT160;
            } else if (chwidth == IEEE80211_CWM_WIDTH320) {
                phymode = IEEE80211_MODE_11BEA_EHT320;
            }
            break;
        }
        case IEEE80211_MODE_BEG:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11BEG_EHT20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11BEG_EHT40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11BEG_EHT40MINUS;
                }
            }
            break;
        }
#endif /* WLAN_FEATURE_11BE */
        case IEEE80211_MODE_AXA:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11AXA_HE20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11AXA_HE40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11AXA_HE40MINUS;
                }
            } else if (chwidth == IEEE80211_CWM_WIDTH80) {
                phymode = IEEE80211_MODE_11AXA_HE80;
            } else if (chwidth == IEEE80211_CWM_WIDTH160) {
                phymode = IEEE80211_MODE_11AXA_HE160;
            } else if (chwidth == IEEE80211_CWM_WIDTH80_80) {
                phymode = IEEE80211_MODE_11AXA_HE80_80;
            }
            break;
        }
        case IEEE80211_MODE_AXG:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11AXG_HE20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11AXG_HE40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11AXG_HE40MINUS;
                }
            }
            break;
        }
        case IEEE80211_MODE_AC:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11AC_VHT20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11AC_VHT40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11AC_VHT40MINUS;
                }
            } else if (chwidth == IEEE80211_CWM_WIDTH80) {
                phymode = IEEE80211_MODE_11AC_VHT80;
            } else if (chwidth == IEEE80211_CWM_WIDTH160) {
                phymode = IEEE80211_MODE_11AC_VHT160;
            } else if (chwidth == IEEE80211_CWM_WIDTH80_80) {
                phymode = IEEE80211_MODE_11AC_VHT80_80;
            }
            break;
        }
        case IEEE80211_MODE_NA:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11NA_HT20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11NA_HT40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11NA_HT40MINUS;
                }
            }
            break;
        }
        case IEEE80211_MODE_NG:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11NG_HT20;
            } else if (chwidth == IEEE80211_CWM_WIDTH40) {
                if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCA) {
                    phymode = IEEE80211_MODE_11NG_HT40PLUS;
                } else if (secchanoffset == IEEE80211_SEC_CHAN_OFFSET_SCB) {
                    phymode = IEEE80211_MODE_11NG_HT40MINUS;
                }
            }
            break;
        }
        case IEEE80211_MODE_A:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11A;
            }
            break;
        }
        case IEEE80211_MODE_G:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11G;
            }
            break;
        }
        case IEEE80211_MODE_B:
        {
            if (chwidth == IEEE80211_CWM_WIDTH20) {
                phymode = IEEE80211_MODE_11B;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return phymode;
}

enum ieee80211_cwm_width
ieee80211_get_cwm_width_from_channel(struct ieee80211_ath_channel *chan)
{
#ifdef WLAN_FEATURE_11BE
    if (IEEE80211_IS_CHAN_BW_320MHZ(chan)) {
        return IEEE80211_CWM_WIDTH320;
    }
#endif /* WLAN_FEATURE_11BE */
    if (IEEE80211_IS_CHAN_BW_80_80MHZ(chan)) {
        return IEEE80211_CWM_WIDTH80_80;
    }
    if (IEEE80211_IS_CHAN_BW_160MHZ(chan)) {
        return IEEE80211_CWM_WIDTH160;
    }
    if (IEEE80211_IS_CHAN_BW_80MHZ(chan)) {
        return IEEE80211_CWM_WIDTH80;
    }
    if (IEEE80211_IS_CHAN_BW_40MHZ(chan)) {
        return IEEE80211_CWM_WIDTH40;
    }
    if (IEEE80211_IS_CHAN_BW_20MHZ(chan)) {
        return IEEE80211_CWM_WIDTH20;
    }

    return IEEE80211_CWM_WIDTH20;
}

qdf_export_symbol(ieee80211_get_cwm_width_from_channel);

#ifdef WLAN_FEATURE_11BE
static enum ieee80211_phymode
ieee80211_get_11bea_phymode(uint64_t chan_flag)
{
    enum ieee80211_phymode mode;

    switch (chan_flag & IEEE80211_CHAN_BW_MASK) {
        case IEEE80211_CHAN_EHT20:
            mode = IEEE80211_MODE_11BEA_EHT20;
            break;
        case IEEE80211_CHAN_EHT40PLUS:
            mode = IEEE80211_MODE_11BEA_EHT40PLUS;
            break;
        case IEEE80211_CHAN_EHT40MINUS:
            mode = IEEE80211_MODE_11BEA_EHT40MINUS;
            break;
        case IEEE80211_CHAN_EHT80:
            mode = IEEE80211_MODE_11BEA_EHT80;
            break;
        case IEEE80211_CHAN_EHT160:
            mode = IEEE80211_MODE_11BEA_EHT160;
            break;
        case IEEE80211_CHAN_EHT320:
            mode = IEEE80211_MODE_11BEA_EHT320;
            break;
        default:
            mode = 0;
    }

    return mode;
}

static enum ieee80211_phymode
ieee80211_get_11beg_phymode(uint64_t chan_flag)
{
    enum ieee80211_phymode mode;
    chan_flag &= IEEE80211_CHAN_BW_MASK;

    switch (chan_flag) {
        case IEEE80211_CHAN_EHT20:
            mode = IEEE80211_MODE_11BEG_EHT20;
            break;
        case IEEE80211_CHAN_EHT40PLUS:
            mode = IEEE80211_MODE_11BEG_EHT40PLUS;
            break;
        case IEEE80211_CHAN_EHT40MINUS:
            mode = IEEE80211_MODE_11BEG_EHT40MINUS;
            break;
        default:
            mode = 0;
            break;
    }

    return mode;
}

#endif /* WLAN_FEATURE_11BE */

static enum ieee80211_phymode
ieee80211_get_11axa_phymode(uint64_t chan_flag)
{
    enum ieee80211_phymode mode;

    switch (chan_flag & IEEE80211_CHAN_BW_MASK) {
        case IEEE80211_CHAN_HE20:
            mode = IEEE80211_MODE_11AXA_HE20;
            break;
        case IEEE80211_CHAN_HE40PLUS:
            mode = IEEE80211_MODE_11AXA_HE40PLUS;
            break;
        case IEEE80211_CHAN_HE40MINUS:
            mode = IEEE80211_MODE_11AXA_HE40MINUS;
            break;
        case IEEE80211_CHAN_HE80:
            mode = IEEE80211_MODE_11AXA_HE80;
            break;
        case IEEE80211_CHAN_HE80_80:
            mode = IEEE80211_MODE_11AXA_HE80_80;
            break;
        case IEEE80211_CHAN_HE160:
            mode = IEEE80211_MODE_11AXA_HE160;
            break;
        default:
            mode = 0;
    }

    return mode;
}

static enum ieee80211_phymode
ieee80211_get_11axg_phymode(uint64_t chan_flag)
{
    enum ieee80211_phymode mode;
    chan_flag &= IEEE80211_CHAN_BW_MASK;

    switch (chan_flag) {
        case IEEE80211_CHAN_HE20:
            mode = IEEE80211_MODE_11AXG_HE20;
            break;
        case IEEE80211_CHAN_HE40PLUS:
            mode = IEEE80211_MODE_11AXG_HE40PLUS;
            break;
        case IEEE80211_CHAN_HE40MINUS:
            mode = IEEE80211_MODE_11AXG_HE40MINUS;
            break;
        default:
            mode = 0;
            break;
    }

    return mode;
}

static enum ieee80211_phymode
ieee80211_get_11ac_phymode(uint64_t chan_flag)
{
    enum ieee80211_phymode mode;
    chan_flag &= IEEE80211_CHAN_BW_MASK;

    switch (chan_flag) {
        case IEEE80211_CHAN_VHT20:
            mode = IEEE80211_MODE_11AC_VHT20;
            break;
        case IEEE80211_CHAN_VHT40PLUS:
            mode = IEEE80211_MODE_11AC_VHT40PLUS;
            break;
        case IEEE80211_CHAN_VHT40MINUS:
            mode = IEEE80211_MODE_11AC_VHT40MINUS;
            break;
        case IEEE80211_CHAN_VHT80:
            mode = IEEE80211_MODE_11AC_VHT80;
            break;
        case IEEE80211_CHAN_VHT80_80:
            mode = IEEE80211_MODE_11AC_VHT80_80;
            break;
        case IEEE80211_CHAN_VHT160:
            mode = IEEE80211_MODE_11AC_VHT160;
            break;
        default:
            mode = 0;
            break;
    }

    return mode;
}

static enum ieee80211_phymode
ieee80211_get_11n_phymode(uint64_t chan_flag)
{
    enum ieee80211_phymode mode = 0;
    chan_flag &= IEEE80211_CHAN_BW_MASK;

    if (IEEE80211_IS_FLAG_5GHZ(chan_flag)) {
        switch (chan_flag) {
            case IEEE80211_CHAN_HT20:
                mode = IEEE80211_MODE_11NA_HT20;
                break;
            case IEEE80211_CHAN_HT40PLUS:
                mode = IEEE80211_MODE_11NA_HT40PLUS;
                break;
            case IEEE80211_CHAN_HT40MINUS:
                mode = IEEE80211_MODE_11NA_HT40MINUS;
                break;
	}
    } else if (IEEE80211_IS_FLAG_2GHZ(chan_flag)) {
        switch (chan_flag) {
            case IEEE80211_CHAN_HT20:
                mode = IEEE80211_MODE_11NG_HT20;
                break;
            case IEEE80211_CHAN_HT40PLUS:
                mode = IEEE80211_MODE_11NG_HT40PLUS;
                break;
            case IEEE80211_CHAN_HT40MINUS:
                mode = IEEE80211_MODE_11NG_HT40MINUS;
                break;
	}
    }

    return mode;
}

enum ieee80211_phymode
ieee80211_get_phymode_from_chan_flag(
        struct ieee80211_ath_channel *curchan,
        uint64_t chan_flag)
{
    enum ieee80211_phymode mode = 0;

#ifdef WLAN_FEATURE_11BE
    if (IEEE80211_IS_CHAN_11BEA(curchan))
        mode = ieee80211_get_11bea_phymode(chan_flag);
    else
#endif /* WLAN_FEATURE_11BE */
    if (IEEE80211_IS_CHAN_11AXA(curchan))
        mode = ieee80211_get_11axa_phymode(chan_flag);
#ifdef WLAN_FEATURE_11BE
    else if (IEEE80211_IS_CHAN_11BEG(curchan))
        mode = ieee80211_get_11beg_phymode(chan_flag);
#endif /* WLAN_FEATURE_11BE */
    else if (IEEE80211_IS_CHAN_11AXG(curchan))
        mode = ieee80211_get_11axg_phymode(chan_flag);
    else if (IEEE80211_IS_CHAN_11AC(curchan))
        mode = ieee80211_get_11ac_phymode(chan_flag);
    else if (IEEE80211_IS_CHAN_11N(curchan))
        mode = ieee80211_get_11n_phymode(chan_flag);

    return mode;
}

int wlan_get_target_phymode(struct ieee80211com *ic, uint32_t phymode,
                            bool is_2gvht_en)
{
    if (ic && ic->ic_get_target_phymode)
        return ic->ic_get_target_phymode(ic, phymode, is_2gvht_en);
    return 0;
}

bool
ieee80211_update_custom_scan_chan_list(
        struct ieee80211vap *vap, bool associated)
{
    struct ieee80211com *ic = NULL;
    uint32_t chcount = 0;
    uint32_t *chanlist = NULL;
    struct chan_list *chan_list = NULL;
    enum wlan_band band;
    int i = 0;

#if ATH_SUPPORT_WRAP
    if (!vap || !vap->iv_ic || (wlan_rptr_vdev_is_psta(vap->vdev_obj) && !wlan_rptr_vdev_is_mpsta(vap->vdev_obj)))
        return false;
#endif

    ic = vap->iv_ic;
    if (ic->ic_use_custom_chan_list) {
        if (associated && ic->ic_custom_chanlist_assoc_size) {
            chcount = ic->ic_custom_chanlist_assoc_size;
            chanlist = ic->ic_custom_chan_list_associated;
        } else if (ic->ic_custom_chanlist_nonassoc_size) {
            chcount = ic->ic_custom_chanlist_nonassoc_size;
            chanlist = ic->ic_custom_chan_list_nonassociated;
        }
    }
    if (chcount) {
        chan_list = qdf_mem_malloc(sizeof(*chan_list));

        if (!chan_list) {
             qdf_err("chan_list is NULL");
             return false;
        }

        chan_list->num_chan =  chcount;
        for (i = 0; i < chcount; i++) {
            chan_list->chan[i].freq = chanlist[i];
            band = util_scan_scm_freq_to_band(chan_list->chan[i].freq);
            if (band == WLAN_BAND_2_4_GHZ)
                chan_list->chan[i].phymode = SCAN_PHY_MODE_11G;
            else
                chan_list->chan[i].phymode = SCAN_PHY_MODE_11A;
        }
    }
    ucfg_scan_set_custom_scan_chan_list(ic->ic_pdev_obj, chan_list);
    if (chan_list)
        qdf_mem_free(chan_list);

    return true;
}

/*
 * ieee80211_get_chan_width_from_mode:
 * Get channel width integer from a given phymode
 *
 * @mode: Given phymode
 *
 * Return:
 * Integer channel width value
 */
int ieee80211_get_chan_width_from_mode(enum ieee80211_phymode mode)
{
    enum phy_ch_width ch_width;

    ch_width = ieee80211_get_chan_width_from_phymode(mode);
    return ieee80211_get_chan_width_from_phy_chwidth(ch_width);
}

/*
 * ieee80211_get_chan_freq_range:
 * Get the channel ranges for a given channel
 *
 * @channel: Pointer to the channel
 * @chan_low: Pointer to store the low freq
 * @chan_mid_low: Pointer to store the mid low freq (for 80+80MHz)
 * @chan_mid_high: Pointer to store the mid high freq (for 80+80MHz)
 * @chan_high: Pointer to store the high freq
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successfully found freq range
 * QDF_STATUS_E_*    : Failed to get freq range
 */
QDF_STATUS ieee80211_get_chan_freq_range(struct ieee80211_ath_channel *channel,
                                         qdf_freq_t *chan_low,
                                         qdf_freq_t *chan_mid_low,
                                         qdf_freq_t *chan_mid_high,
                                         qdf_freq_t *chan_high)
{
    qdf_freq_t pri20_center_freq = channel->ic_freq;
    qdf_freq_t cfreq0            = channel->ic_vhtop_freq_seg1;
    qdf_freq_t cfreq1            = channel->ic_vhtop_freq_seg2;
    enum ieee80211_phymode mode  = ieee80211_chan2mode(channel);
    int chwidth;

    chwidth = ieee80211_get_chan_width_from_mode(mode);

    switch (chwidth) {
        case CHWIDTH_20:
            *chan_low  = pri20_center_freq;
            *chan_high = pri20_center_freq;
        break;

        case CHWIDTH_40:
            if (ieee80211_is_phymode_40plus(mode)) {
                if (!cfreq0)
                    cfreq0 = channel->ic_freq + IEEE80211_FREQ_CFREQ_OFFSET;
            } else if (ieee80211_is_phymode_40minus(mode)) {
                if (!cfreq0)
                    cfreq0 = channel->ic_freq - IEEE80211_FREQ_CFREQ_OFFSET;
            }

        /* fallthrough */
        case CHWIDTH_80:
            *chan_low  = (cfreq0 - ((chwidth/2)-10));
            *chan_high = (cfreq0 + ((chwidth/2)-10));
        break;

        case CHWIDTH_160:
#ifdef WLAN_FEATURE_11BE
        case CHWIDTH_320:
#endif /* WLAN_FEATURE_11BE */
            if (ieee80211_is_phymode_8080(mode)) {
                *chan_low      = (cfreq0 - ((chwidth/2)-10));
                *chan_mid_low  = (cfreq0 + ((chwidth/2)-10));
                *chan_mid_high = (cfreq1 - ((chwidth/2)-10));
                *chan_high     = (cfreq1 + ((chwidth/2)-10));
            } else {
                *chan_low      = (cfreq1 - ((chwidth/2)-10));
                *chan_high     = (cfreq1 + ((chwidth/2)-10));
            }
        break;

        default:
            /* Return false */
        break;
    }

    return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(ieee80211_get_chan_freq_range);

/*
 * ieee80211_is_chan_overlap:
 * Check if a channel is overlapping with another channel with an optional
 * buffer.
 *
 * @chan1 : Pointer to chan1
 * @chan2 : Pointer to chan2
 * @buffer: Value of buffer (20, 40, 80, 160 or 320MHz)
 *
 * Return:
 * QDF_STATUS_SUCCESS: Channel is overlapping
 * QDF_STATUS_E_*    : Channel is not overlapping
 */
QDF_STATUS ieee80211_is_chan_overlap(struct ieee80211_ath_channel *chan1,
                                     struct ieee80211_ath_channel *chan2,
                                     struct acs_random_chan_config *config)
{
    qdf_freq_t chan1_low = 0,
               chan1_mid_low = 0,
               chan1_mid_high = 0,
               chan1_high = 0,
               chan2_low = 0,
               chan2_mid_low = 0,
               chan2_mid_high = 0,
               chan2_high = 0;

    uint32_t buffer;
    uint16_t count = 0;

    if(!config) {
        qdf_err("config is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    buffer = config->adj_chan_buffer;

#ifdef WLAN_FEATURE_11BE
    if (buffer >= CHWIDTH_320) {
#else
    if (buffer >= CHWIDTH_160) {
#endif /* WLAN_FEATURE_11BE */
        qdf_err("Invalid buffer");
        return QDF_STATUS_E_INVAL;
    }

    if (ieee80211_get_chan_freq_range(chan1, &chan1_low, &chan1_mid_low, &chan1_mid_high, &chan1_high)) {
        qdf_err("Could not get chan_freq_range for chan1");
        return QDF_STATUS_E_FAILURE;
    }

    if (ieee80211_get_chan_freq_range(chan2, &chan2_low, &chan2_mid_low, &chan2_mid_high, &chan2_high)) {
        qdf_err("Could not get chan_freq_range for chan2");
        return QDF_STATUS_E_FAILURE;
    }

    do {

    if (config->allow_awgn_interference_overlap_check) {
         chan2_low = config->intf_freq[count].chan_low;
         chan2_high = config->intf_freq[count].chan_high;
	 qdf_debug("config->intf_freq[count].chan_low: %d", config->intf_freq[count].chan_low);
         qdf_debug("config->intf_freq[count].chan_high: %d", config->intf_freq[count].chan_high);
	 count++;

    }

    if (!IEEE80211_IS_CHAN_80_80MHZ(chan2)) {
        if (((chan1_low  >= (chan2_low - buffer)) && (chan1_low  <= (chan2_high + buffer))) ||
            ((chan1_high >= (chan2_low - buffer)) && (chan1_high <= (chan2_high + buffer)))) {
            return QDF_STATUS_SUCCESS;
        } else if (config->allow_awgn_interference_overlap_check) {
               /* This check is added to see if the segment is present
                * within the  chan1_low and chan2_high for awgn
                * */
            if ((chan1_low  <= (chan2_low - buffer)) && (chan1_high  >= (chan2_high + buffer))) {
                return QDF_STATUS_SUCCESS;
            }
        }

    } else {
        if (((chan1_low  >= (chan2_low - buffer)) &&
             (chan1_low  <= (chan2_mid_low + buffer))) ||
            ((chan1_low  >= (chan2_mid_high - buffer)) &&
             (chan1_low  <= (chan2_high + buffer))) ||
            ((chan1_high >= (chan2_low - buffer)) &&
             (chan1_high <= (chan2_mid_low + buffer))) ||
            ((chan1_high >= (chan2_mid_high - buffer)) &&
             (chan1_high <= (chan2_high + buffer)))) {
            return QDF_STATUS_SUCCESS;
        }
    }

    } while ((count < 16) &&
              (config->intf_freq[count].chan_low !=0) &&
              (config->intf_freq[count].chan_high !=0) &&
              (config->allow_awgn_interference_overlap_check));

    return QDF_STATUS_E_INVAL;
}

static bool
ieee80211_validate_chainmask_caps(struct ieee80211_chainmask_info chmask,
                                  enum ieee80211_phymode phymode) {
    enum ieee80211_cwm_width ch_width;

    /*
     * 1. check chain mask supports 2G/5G.
     * 2. check chain mask supports current phymode
     *      20, 40, 80, 160, 80P80, 320.
     */
    if (ieee80211_is_phymode_auto(phymode)) {
        if (!chmask.chain_mask_2G && !chmask.chain_mask_5G)
            return false;
        else
            return true;
    }

    /* check BAND for a given chain mask */
    if (ieee80211_is_phymode_2g(phymode)) {
        if (!chmask.chain_mask_2G)
            return false;
    }

    if (ieee80211_is_phymode_5g_or_6g(phymode)) {
        if (!chmask.chain_mask_5G)
            return false;
    }

    /* check channel width for a given chain mask */
    ch_width = get_chwidth_phymode(phymode);
    switch(ch_width)
    {
        case IEEE80211_CWM_WIDTH20:
            if (!chmask.supports_chan_width_20)
                return false;
            break;
        case IEEE80211_CWM_WIDTH40:
            if (!chmask.supports_chan_width_40)
                return false;
            break;
        case IEEE80211_CWM_WIDTH80:
            if (!chmask.supports_chan_width_80)
                return false;
            break;
        case IEEE80211_CWM_WIDTH160:
            if (!chmask.supports_chan_width_160)
                return false;
            break;
        case IEEE80211_CWM_WIDTH80_80:
            if (!chmask.supports_chan_width_80P80)
                return false;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320:
            if (!chmask.supports_chan_width_320)
                return false;
            break;
#endif /* WLAN_FEATURE_11BE */

        default:
            return false;
    }

    return true;

}

bool ieee80211_is_phymode_supported(struct ieee80211com *ic,
                                    enum ieee80211_phymode mode) {
    return (ieee80211_validate_chainmask_caps(ic->ic_txchainmask_info, mode) &&
            ieee80211_validate_chainmask_caps(ic->ic_rxchainmask_info, mode));
}

/*
 * ieee80211_is_freq_in_width:
 * Check if a given frequency is within a bandwidth
 * NOTE:
 * - Only widths greater than 20MHz is considered.
 *
 * @freq       : Given frequency (in MHz)
 * @channel    : Given channel pointer
 * @buffer     : Given buffer to the left and right of the bandwidth
 *
 * Return:
 * true : Frequency is present in width
 * false: Frequency is not present in width
 */
bool
ieee80211_is_freq_in_width(qdf_freq_t freq,
                           struct ieee80211_ath_channel *channel,
                           uint32_t buffer)
{
    qdf_freq_t pri20_center_freq = channel->ic_freq;
    qdf_freq_t cfreq0            = channel->ic_vhtop_freq_seg1;
    qdf_freq_t cfreq1            = channel->ic_vhtop_freq_seg2;
    enum ieee80211_phymode mode  = ieee80211_chan2mode(channel);
    int chwidth;

    chwidth = ieee80211_get_chan_width_from_mode(mode);

    switch (chwidth) {
        case CHWIDTH_20:
            if (freq == pri20_center_freq) {
                return true;
            }
        break;

        case CHWIDTH_40:
            if (ieee80211_is_phymode_40plus(mode)) {
                if (!cfreq0)
                    cfreq0 = channel->ic_freq + IEEE80211_FREQ_CFREQ_OFFSET;
            } else if (ieee80211_is_phymode_40minus(mode)) {
                if (!cfreq0)
                    cfreq0 = channel->ic_freq - IEEE80211_FREQ_CFREQ_OFFSET;
            }

            if (IEEE80211_IS_FREQ_IN_WIDTH(freq, cfreq0, chwidth, buffer)) {
                return true;
            }
        break;

        case CHWIDTH_80:
            if (IEEE80211_IS_FREQ_IN_WIDTH(freq, cfreq0, chwidth, buffer)) {
                return true;
            }
        break;

        case CHWIDTH_160:
#ifdef WLAN_FEATURE_11BE
        case CHWIDTH_320:
#endif /* WLAN_FEATURE_11BE */
            if (ieee80211_is_phymode_8080(mode)) {
                if (IEEE80211_IS_FREQ_IN_WIDTH(freq, cfreq0, chwidth, buffer) ||
                    IEEE80211_IS_FREQ_IN_WIDTH(freq, cfreq1, chwidth, buffer)) {
                    return true;
                }
            } else {
                if (IEEE80211_IS_FREQ_IN_WIDTH(freq, cfreq1, chwidth, buffer)) {
                    return true;
                }
            }
        break;

        default:
            /* Return false */
        break;
    }

    return false;
}

void ieee80211_update_vdev_chan(struct wlan_channel *vdev_chan,
        struct ieee80211_ath_channel *curchan)
{
    enum ieee80211_phymode phymode;
    if (!IEEE80211_IS_CHAN_VALID(curchan)) {
        qdf_mem_zero(vdev_chan, sizeof(struct wlan_channel));
    } else {
        vdev_chan->ch_freq = curchan->ic_freq;
        vdev_chan->ch_ieee = curchan->ic_ieee;
        vdev_chan->ch_flags = curchan->ic_flags;
        vdev_chan->ch_flagext = curchan->ic_flagext;
        vdev_chan->ch_maxpower = curchan->ic_maxpower;
        vdev_chan->ch_freq_seg1 = curchan->ic_vhtop_ch_num_seg1;
        vdev_chan->ch_freq_seg2 = curchan->ic_vhtop_ch_num_seg2;
        vdev_chan->ch_cfreq1 = curchan->ic_vhtop_freq_seg1;
        vdev_chan->ch_cfreq2 = curchan->ic_vhtop_freq_seg2;
        vdev_chan->ch_width = ieee80211_get_phy_chan_width(curchan);

        phymode = ieee80211_chan2mode(curchan);
        vdev_chan->ch_phymode = phymode2convphymode[phymode];
#ifdef WLAN_FEATURE_11BE
        vdev_chan->puncture_bitmap = curchan->op_puncture_bitmap;
#endif
    }
}

enum wlan_band_id ieee80211_get_ath_channel_band(qdf_freq_t freq)
{
    if (WLAN_REG_IS_24GHZ_CH_FREQ(freq)) {
        return WLAN_BAND_2GHZ;
    } else if ((WLAN_REG_IS_5GHZ_CH_FREQ(freq)) ||
               (WLAN_REG_IS_49GHZ_FREQ(freq))) {
        return WLAN_BAND_5GHZ;
    } else if (WLAN_REG_IS_6GHZ_CHAN_FREQ(freq)) {
        return WLAN_BAND_6GHZ;
    }

    return WLAN_BAND_UNSPECIFIED;
}

enum reg_wifi_band wlan_band_id_to_reg_wifi_band(
        enum wlan_band_id wlan_band)
{
    static uint32_t
        wlan_band_id_to_reg_wifi_band_map[] = {
            [WLAN_BAND_UNSPECIFIED] = BIT(REG_BAND_2G) | BIT(REG_BAND_5G),
            [WLAN_BAND_2GHZ] = BIT(REG_BAND_2G),
            [WLAN_BAND_5GHZ] = BIT(REG_BAND_5G),
            [WLAN_BAND_6GHZ] = BIT(REG_BAND_6G),
        };

        if ((wlan_band < WLAN_BAND_UNSPECIFIED) ||
            (wlan_band >= WLAN_BAND_MAX)) {
            qdf_warn("Invalid wlan_band %d", wlan_band);
            return -EINVAL;
        }

        return wlan_band_id_to_reg_wifi_band_map[wlan_band];
}

enum wlan_band_id reg_wifi_band_to_wlan_band_id(
        enum reg_wifi_band reg_wifi_band)
{
    enum wlan_band_id wlan_band;
    static uint32_t
        reg_wifi_band_to_wlan_band_id_map[] = {
            [REG_BAND_2G] = WLAN_BAND_2GHZ,
            [REG_BAND_5G] = WLAN_BAND_5GHZ,
            [REG_BAND_6G] = WLAN_BAND_6GHZ,
            [REG_BAND_UNKNOWN] = WLAN_BAND_MAX,
        };

        if ((reg_wifi_band < REG_BAND_2G) || (reg_wifi_band >= REG_BAND_UNKNOWN)) {
            qdf_warn("Invalid reg_wifi_band %d", reg_wifi_band);
            return -EINVAL;
        }

        wlan_band = reg_wifi_band_to_wlan_band_id_map[reg_wifi_band];
        if (wlan_band == WLAN_BAND_MAX) {
            qdf_warn("Invalid wlan_band_id %d, reg_wifi_band: %d", wlan_band, reg_wifi_band);
            return -EINVAL;
        }

        return wlan_band;
}

uint16_t wlan_get_wlan_band_id_chan_to_freq(
        struct wlan_objmgr_pdev *pdev,
        uint32_t chan, enum wlan_band_id wlan_band)
{
    enum reg_wifi_band reg_wifi_band;
    struct ieee80211com *ic;
    uint16_t freq = 0;

    if (!pdev)
        return 0;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return 0;

    if (!chan) {
        return freq;
    }

    if (chan > IEEE_MAX_CHAN_NUM) {
        qdf_err("Invalid channel number %d", chan);
        return 0;
    }

    if ((wlan_band < WLAN_BAND_UNSPECIFIED) ||
        (wlan_band >= WLAN_BAND_MAX))
        return 0;

    if (ic->ic_49ghz_enabled)
        reg_wifi_band = BIT(REG_BAND_5G);
    else
        reg_wifi_band = wlan_band_id_to_reg_wifi_band(wlan_band);

    freq = wlan_reg_chan_band_to_freq(pdev, (uint8_t)chan, reg_wifi_band);
    if (!freq) {
        qdf_warn("Invalid freq: %d, wlan_band: %d, reg_wifi_band: %d, chan: %d",
                freq, wlan_band, reg_wifi_band, chan);
    }

    return freq;
}

qdf_freq_t wlan_band_id_chan_to_freq_from_cur_chan_list(struct ieee80211com *ic,
                                                        uint32_t chan,
                                                        enum wlan_band_id wlan_band)
{
    enum reg_wifi_band reg_wifi_band;
    struct regulatory_channel *reg_chan_list;
    qdf_freq_t freq = 0;
    uint8_t i, num_reg_channels;

    if (!chan) {
        return freq;
    }

    if (chan > IEEE_MAX_CHAN_NUM) {
        qdf_err("Invalid channel number %d", chan);
        return 0;
    }

    if ((wlan_band < WLAN_BAND_UNSPECIFIED) ||
        (wlan_band >= WLAN_BAND_MAX))
        return 0;

    if (ic->ic_49ghz_enabled)
        reg_wifi_band = BIT(REG_BAND_5G);
    else
        reg_wifi_band = wlan_band_id_to_reg_wifi_band(wlan_band);

    reg_chan_list = qdf_mem_malloc(sizeof(struct regulatory_channel) *
                                   NUM_CHANNELS);

    if (!reg_chan_list)
        return 0;

    num_reg_channels = wlan_reg_get_band_channel_list(ic->ic_pdev_obj,
                                                      reg_wifi_band,
                                                      reg_chan_list);

    if (num_reg_channels > NUM_CHANNELS)
        num_reg_channels = NUM_CHANNELS;

    for (i = 0; i < num_reg_channels; i++) {
         if (reg_chan_list[i].chan_num == (uint8_t)chan) {
             freq = reg_chan_list[i].center_freq;
             break;
         }
    }

    if (!freq) {
        qdf_warn("Invalid freq: %d, wlan_band: %d, reg_wifi_band: %d, chan: %d",
                freq, wlan_band, reg_wifi_band, chan);
    }

    qdf_mem_free(reg_chan_list);

    return freq;
}

/*
 * ieee80211_chan2freq() - Convert channel to frequency value.
 * @ic: Pointer to ieee80211com structure
 * @c: Pointer to ieee80211_ath_channel structure
 *
 * Return: Frequency value on successful conversion
 *         "0" otherwise
 */
inline u_int16_t
ieee80211_chan2freq(struct ieee80211com *ic,
                    const struct ieee80211_ath_channel *c)
{
    if (!IEEE80211_IS_CHAN_VALID(c)) {
        qdf_warn("Invalid ptr to channel");
        return 0;
    }
    return c->ic_freq;
}

int ieee80211_is_freq_in_obj_valid(struct ieee80211com *ic,
                                   struct chan_params *chan_p,
				   uint64_t flags)
{
    struct ieee80211_ath_channel chan_obj;

    chan_obj = ieee80211_find_channel(ic, chan_p, flags);

    return chan_obj.ic_freq;
}

#ifdef WLAN_FEATURE_11BE
enum ieee80211_phymode
ieee80211_get_11bea_mode_from_bw(uint16_t chan_bw)
{
    switch(chan_bw)
    {
        case CHWIDTH_320:
            return IEEE80211_MODE_11BEA_EHT320;
        case CHWIDTH_160:
            return IEEE80211_MODE_11BEA_EHT160;
        case CHWIDTH_80:
            return IEEE80211_MODE_11BEA_EHT80;
        case CHWIDTH_40:
            return IEEE80211_MODE_11BEA_EHT40;
        case CHWIDTH_20:
            return IEEE80211_MODE_11BEA_EHT20;
        default:
            return IEEE80211_MODE_11BEA_EHT20;
    }
}

qdf_freq_t
ieee80211_find_ch_center(struct ieee80211com *ic, qdf_freq_t primary_freq,
                         qdf_freq_t center_320, uint16_t chan_bw,
                         uint16_t punc_pattern)
{
    struct chan_params chan_p = {0};
    struct ieee80211_ath_channel chan_obj;
    enum ieee80211_phymode mode;
    qdf_freq_t chan_center = 0;

    chan_p.freq = primary_freq;
    chan_p.cfreq2 = center_320;
    chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE;
    set_chan_params_puncture_bitmap(&chan_p, punc_pattern);
    mode = ieee80211_get_11bea_mode_from_bw(chan_bw);
    chan_obj = ieee80211_find_dot11_channel(ic, &chan_p, mode);
    if (IEEE80211_IS_CHAN_VALID(&chan_obj))
        chan_center = ieee80211_get_chan_centre_freq(ic, &chan_obj);

    return chan_center;
}
qdf_export_symbol(ieee80211_find_ch_center);
#endif

uint8_t ieee80211_get_sec_chan_offset_for_chanbw40(struct ieee80211com *ic,
                                                   struct chan_params *chan_p)
{
    const struct bonded_channel_freq *bonded_chan_ptr2 = NULL;
    enum channel_enum ch_indx;
    QDF_STATUS status;
    uint8_t sec_ch_offset = 0;
    uint16_t max_bw;

    ch_indx = reg_get_chan_enum_for_freq(chan_p->freq);

    if (reg_is_chan_enum_invalid(ch_indx))
        return sec_ch_offset;

    status = reg_get_min_max_bw_reg_chan_list(ic->ic_pdev_obj, ch_indx,
                                              REG_CURRENT_PWR_MODE,
                                              NULL, &max_bw);
    if (status != QDF_STATUS_SUCCESS)
        return sec_ch_offset;

     if (max_bw < BW_40)
         return sec_ch_offset;

    if (!wlan_reg_is_freq_enabled(ic->ic_pdev_obj, chan_p->freq,
                                  REG_CURRENT_PWR_MODE))
        return sec_ch_offset;

    bonded_chan_ptr2 = reg_get_bonded_chan_entry(chan_p->freq,
                                                 CH_WIDTH_40MHZ, 0);

    if (!bonded_chan_ptr2)
        return sec_ch_offset;

    if (chan_p->freq == bonded_chan_ptr2->start_freq)
        sec_ch_offset = LOW_PRIMARY_CH;
    else
        sec_ch_offset = HIGH_PRIMARY_CH;

    return sec_ch_offset;
}

bool ieee80211_is_freq_valid_for_chanbw20(struct ieee80211com *ic,
                                          struct chan_params *chan_p,
                                          uint64_t ch_flags)
{
    enum channel_enum ch_indx;
    QDF_STATUS status;
    uint16_t max_bw;

    if (WLAN_REG_IS_49GHZ_FREQ(chan_p->freq) && !IS_FLAG_11A(ch_flags))
        return false;

    ch_indx = reg_get_chan_enum_for_freq(chan_p->freq);

    if (reg_is_chan_enum_invalid(ch_indx))
        return false;

    status = reg_get_min_max_bw_reg_chan_list(ic->ic_pdev_obj, ch_indx,
                                              REG_CURRENT_PWR_MODE,
                                              NULL, &max_bw);

    if (status != QDF_STATUS_SUCCESS)
        return false;

    if (!wlan_reg_is_freq_enabled(ic->ic_pdev_obj, chan_p->freq,
                                  REG_CURRENT_PWR_MODE))
        return false;

     if (max_bw >= BW_20)
         return true;

    return false;
}
#ifdef WLAN_FEATURE_11BE
/*
 * ieee80211_get_punc_type:
 * Get the puncture type for the given channel
 *
 * @mode              : Channel phymode for input
 * @op_puncture_bitmap: Channel puncture pattern
 * @puncture_type     : Pointer to store output puncture type
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successfully retrieved puncture type
 * QDF_STATUS_E_*    : Failed to retrieve puncture type
 */
QDF_STATUS ieee80211_get_punc_type(enum ieee80211_phymode mode,
                                   uint16_t op_puncture_bitmap,
                                   enum ieee80211_punc_type *puncture_type)
{
    uint16_t puncture_pattern, mask;
    uint8_t  punctured_bits;

    if (!puncture_type) {
        qdf_err("Invalid pointer to store puncture type");
        return QDF_STATUS_E_INVAL;
    }

    /* If the channel is not punctured, then return none */
    if (!IEEE80211_IS_PATTERN_PUNCTURED(op_puncture_bitmap)) {
        *puncture_type = IEEE80211_PUNC_NONE;
        return QDF_STATUS_SUCCESS;
    }

    switch (get_chwidth_phymode(mode)) {
        case IEEE80211_CWM_WIDTH80:
            mask = PUNCTURE_80MHZ_MASK;
        break;
        case IEEE80211_CWM_WIDTH160:
            mask = PUNCTURE_160MHZ_MASK;
        break;
        case IEEE80211_CWM_WIDTH320:
            mask = PUNCTURE_320MHZ_MASK;
        break;
        default:
            *puncture_type = IEEE80211_PUNC_NONE;
            return QDF_STATUS_SUCCESS;
        break;
    }
    puncture_pattern = op_puncture_bitmap & mask;

    /*
     * Retrieve the number of set bits in the pattern. This will give the
     * number of punctured slices in the channel
     */
    punctured_bits = 0;
    while (puncture_pattern != 0) {
        punctured_bits++;
        puncture_pattern &= (puncture_pattern - 1);
    }

    if (punctured_bits >= IEEE80211_PUNC_INVALID) {
        qdf_err("Could not find puncture type");
        *puncture_type = IEEE80211_PUNC_INVALID;
        return QDF_STATUS_E_FAILURE;
    }

    /*
     * Reaching here implies that the punctured bits are valid as per the
     * ieee80211_punc_type enum. Therefore, return success.
     */
    *puncture_type = punctured_bits;
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef CONFIG_AFC_SUPPORT
/*
 * ieee80211_find_min_power_for_reg_chan:
 * Find the minimum PSD and EIRP for a given channel and mode.
 *
 * @ic               : Pointer to ic
 * @chan_list        : Pointer to the supported channel list
 * @in_6g_pwr_mode   : Type of power mode for 6Ghz channel
 * @channel          : Pointer to given channel pointer
 * @is_psd           : Flag for signifying if channels are PSD
 * @min_psd_power    : Pointer to store the minimum PSD power
 * @min_eirp_power   : Pointer to store the minimum EIRP power
 */
void
ieee80211_find_min_power_for_reg_chan(struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel,
                                      enum supported_6g_pwr_types in_6g_pwr_mode,
                                      bool *is_psd,
                                      int16_t  *min_psd_power,
                                      uint16_t *min_eirp_power)
{
    struct regulatory_channel *chan_list = NULL;
    enum ieee80211_phymode mode;
    uint32_t chan_ix;
    enum reg_6g_ap_type ap_pwr_type;
    bool get_curchan_list;
    uint16_t ch_width;

    if (!min_psd_power || !is_psd) {
        qdf_err("Invalid pointer to min_psd_power");
        return;
    }

    if (!min_eirp_power) {
        qdf_err("Invalid pointer to min_eirp_power");
        return;
    }

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        qdf_err("Invalid channel");
        return;
    }

    mode  = ieee80211_chan2mode(channel);
    if ((mode <= IEEE80211_MODE_AUTO) ||
        (mode >= IEEE80211_MODE_MAX)) {
        qdf_err("Invalid mode");
        return;
    }

    switch (in_6g_pwr_mode) {
        case REG_CURRENT_PWR_MODE:
            get_curchan_list = true;
            ap_pwr_type = REG_MAX_AP_TYPE;
        break;

        case REG_AP_SP:
            get_curchan_list = false;
            ap_pwr_type = REG_STANDARD_POWER_AP;
        break;

        case REG_AP_LPI:
            get_curchan_list = false;
            ap_pwr_type = REG_INDOOR_AP;
        break;

        case REG_AP_VLP:
            get_curchan_list = false;
            ap_pwr_type = REG_VERY_LOW_POWER_AP;
        break;

        default:
            qdf_err("Could not populate chan_list for in_pwr_mode");
            return;
        break;
    }

    chan_list = qdf_mem_malloc(sizeof(*chan_list) * NUM_CHANNELS);
    if (!chan_list) {
        qdf_err("Could not allocate memory for chan_list");
        return;
    }

    if (wlan_reg_get_ap_chan_list(ic->ic_pdev_obj,
                                  chan_list,
                                  get_curchan_list,
                                  ap_pwr_type)) {
        qdf_err("Could not populate chan_list for in_pwr_mode: %d", in_6g_pwr_mode);
        goto end;
    }

    *min_psd_power  = CHAN_MAX_PSD_POWER;
    *min_eirp_power = CHAN_MAX_EIRP_POWER;
    *is_psd         = true;
    for (chan_ix = 0; chan_ix < NUM_CHANNELS; chan_ix++) {
        if (!wlan_reg_is_freq_enabled(ic->ic_pdev_obj,
                                      chan_list[chan_ix].center_freq,
                                      in_6g_pwr_mode)) {
            continue;
        }

        /*
         * Skip channels which are not part of the given width
         */
        if (!ieee80211_is_freq_in_width(chan_list[chan_ix].center_freq,
                                        channel, 0)) {
            continue;
        }

        /*
         * If any single channel within range does not support a PSD value
         * then mark the is_psd flag as false to let the caller know of
         * If any single channel within range does not support a PSD value
         * then mark the is_psd flag as false to let the caller know of
         * invalid PSD values.
         */
        if ((*is_psd) && !chan_list[chan_ix].psd_flag) {
            (*is_psd) = false;
        }

        if ((int16_t)chan_list[chan_ix].psd_eirp < *min_psd_power) {
            *min_psd_power = (int16_t)chan_list[chan_ix].psd_eirp;
        }

        if (chan_list[chan_ix].tx_power < *min_eirp_power) {
            *min_eirp_power = chan_list[chan_ix].tx_power;
        }
    }

    if (*is_psd) {
        ch_width = ieee80211_get_chan_width_from_mode(ieee80211_chan2mode(channel));
        if (!ch_width) {
            qdf_err("Invalid channel width");
            return;
        }

        if (wlan_reg_psd_2_eirp(ic->ic_pdev_obj, *min_psd_power, ch_width,
                                min_eirp_power) != QDF_STATUS_SUCCESS) {
            qdf_err("Could not derive EIRP power for width");
            return;
        }
    }

end:
   if (chan_list) {
       qdf_mem_free(chan_list);
   }

   return;
}
#endif /* CONFIG_AFC_SUPPORT */

/*
 * ieee80211_is_input_or_best_pwr_mode_sp:
 * Check if the input power mode is SP or the best power mode for the given
 * channel is SP
 *
 * @ap_pwr_tupe: Input power mode
 * @pdev: Pointer to pdev
 * @freq: Input channel freq
 *
 * Return: True if input power mode or best power mode is SP, else false.
 */
static bool
ieee80211_is_input_or_best_pwr_mode_sp(enum supported_6g_pwr_types ap_pwr_type,
                                       struct wlan_objmgr_pdev *pdev,
                                       qdf_freq_t freq)
{
    if (ap_pwr_type == REG_AP_SP)
        return true;

    if (wlan_reg_get_best_6g_pwr_type(pdev, freq) == REG_AP_SP)
        return true;

    return false;
}

/*
 * ieee80211_is_6g_txable_chan_available:
 * Check if a channel for a given 6GHz power type is available for tx
 *
 * @ic: Pointer to ic
 * @ap_pwr_type: Value of the AP power type
 *
 * Return:
 * true: Power type is valid for tx
 * false: Power type is invalid for tx
 */
bool ieee80211_is_6g_txable_chan_available(struct ieee80211com *ic,
                                           enum supported_6g_pwr_types ap_pwr_type)
{
    struct regulatory_channel *chan_list = NULL;
    uint32_t chan_ix;
    bool status = false;
#ifdef CONFIG_AFC_SUPPORT
    enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif

    chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*chan_list));
    if (!chan_list) {
        qdf_err("Invalid channel list");
        status = false;
        goto exit;
    }

    if (wlan_reg_get_current_chan_list(ic->ic_pdev_obj,
                                       chan_list)) {
        qdf_err("Could not retrieve channel list");
        status = false;
        goto exit;
    }

#ifdef CONFIG_AFC_SUPPORT
    wlan_reg_get_afc_dev_deploy_type(ic->ic_pdev_obj, &reg_afc_deploy_type);
#endif

    for (chan_ix = 0; chan_ix < NUM_CHANNELS; chan_ix++) {
        if (!wlan_reg_is_freq_enabled(ic->ic_pdev_obj,
                                      chan_list[chan_ix].center_freq,
                                      ap_pwr_type)) {
            continue;
        }

        /* Skip all the non-6GHz channels in this list */
        if (!wlan_reg_is_6ghz_chan_freq(chan_list[chan_ix].center_freq)) {
            continue;
        }

#ifdef CONFIG_AFC_SUPPORT
    if ((reg_afc_deploy_type == AFC_DEPLOYMENT_OUTDOOR) &&
        ieee80211_is_input_or_best_pwr_mode_sp(ap_pwr_type,
                                               ic->ic_pdev_obj,
                                               chan_list[chan_ix].center_freq)) {
        if (!wlan_is_sup_chan_entry_afc_done(ic->ic_pdev_obj,
                                             chan_ix,
                                             REG_AP_SP))
            continue;
        }
#endif

        /* Returning true because there is atleast 1 valid 20MHz channel */
        status = true;
        break;
    }

exit:
    if (chan_list)
        qdf_mem_free(chan_list);

    return status;
}

qdf_export_symbol(ieee80211_is_6g_txable_chan_available);

#ifdef CONFIG_AFC_SUPPORT
#ifdef WLAN_FEATURE_11BE
static bool
ieee80211_are_punc_patterns_equal(struct ieee80211_ath_channel *chan1,
                                  struct ieee80211_ath_channel *chan2)
{
    if (IEEE80211_IS_CHAN_PUNCTURABLE(chan1))
        return (chan1->op_puncture_bitmap  == chan2->op_puncture_bitmap);

    return true;
}
#else
static inline bool
ieee80211_are_punc_patterns_equal(struct ieee80211_ath_channel *chan1,
                                  struct ieee80211_ath_channel *chan2)
{
    return true;
}
#endif

/*
 * ieee80211_verfy_bw_in_given_pwr_type: Verify the channel and its BW if it
 * is supported in the channel list of the given power mode.
 */
static QDF_STATUS
ieee80211_verfy_bw_in_given_pwr_type(struct ieee80211com *ic ,
                                     struct ieee80211_ath_channel *channel,
                                     enum reg_6g_ap_type ap_pwr_type)
{
    struct chan_params chan_p = {0};
    struct ieee80211_ath_channel out_chan;
    enum ieee80211_phymode mode = ieee80211_chan2mode(channel);

    chan_p.freq = channel->ic_freq;
    chan_p.cfreq2 = channel->ic_vhtop_freq_seg2;
#ifdef WLAN_FEATURE_11BE
    /* For a 11BE channel, puncture pattern is expected to be valid */
    if (IEEE80211_IS_CHAN_PUNCTURABLE(channel))
            chan_p.puncture_bitmap = channel->op_puncture_bitmap;
#endif
    chan_p.input_6g_pwr_mode = wlan_reg_conv_6g_ap_type_to_supported_6g_pwr_types(ap_pwr_type);

    qdf_info("freq: %u, cfreq2: %u, pwr: %d, mode: %u\n", chan_p.freq,
             chan_p.cfreq2, chan_p.input_6g_pwr_mode, mode);
#ifdef WLAN_FEATURE_11BE
    qdf_info("Puncture pattern: 0x%x\n", chan_p.puncture_bitmap);
#endif

    out_chan = ieee80211_find_dot11_channel(ic, &chan_p, mode);
    if (IEEE80211_IS_CHAN_VALID(&out_chan) &&
        ieee80211_are_punc_patterns_equal(&out_chan, channel)) {
        qdf_info("The input chan is valid in power mode %u\n", ap_pwr_type);
        return QDF_STATUS_SUCCESS;
    }
    return QDF_STATUS_E_FAILURE;
}

/*
 * __ieee80211_verfy_bw_in_given_pwr_type: Configure the channel with the given
 * 6G power mode. If the channel is not present fall back. The power mode
 * priority is SP, LPI and VLP.
 */
    static QDF_STATUS
__ieee80211_verfy_bw_in_given_pwr_type(struct ieee80211com *ic ,
                                       struct ieee80211_ath_channel *channel,
                                       enum reg_6g_ap_type *ap_pwr_type)
{
    QDF_STATUS status;
    switch (*ap_pwr_type) {
        case REG_STANDARD_POWER_AP:
            status = ieee80211_verfy_bw_in_given_pwr_type(ic, channel,
                                                          REG_STANDARD_POWER_AP);
            if (status == QDF_STATUS_SUCCESS)
                break;
        case REG_INDOOR_AP:
            status = ieee80211_verfy_bw_in_given_pwr_type(ic, channel,
                                                          REG_INDOOR_AP);
            if (status == QDF_STATUS_SUCCESS) {
                *ap_pwr_type = REG_INDOOR_AP;
                break;
            }
        case REG_VERY_LOW_POWER_AP:
            status = ieee80211_verfy_bw_in_given_pwr_type(ic, channel,
                                                          REG_VERY_LOW_POWER_AP);
            if (status == QDF_STATUS_SUCCESS) {
                *ap_pwr_type = REG_VERY_LOW_POWER_AP;
                break;
            }
        default:
            status = QDF_STATUS_E_FAILURE;
            break;
    }
    return status;
}

/*
 * ieee80211_change_ap_power_type:
 * Check the desired ath_channel and determine the best power mode.
 *
 * @vap: Pointer to the VAP structure
 * @channel: Pointer to the desired channel
 *
 * Return:
 * QDF_STATUS_SUCCESS: Successfully changed power mode.
 * QDF_STATUS_E_*    : Failed to determine a power mode for channel
 */
QDF_STATUS
ieee80211_change_ap_power_type(struct ieee80211vap *vap,
                               struct ieee80211_ath_channel *channel)
{
    struct ieee80211com *ic = NULL;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    uint16_t bw;
    enum reg_6g_ap_type new_ap_pwr_type;
    uint16_t in_punc_bitmap;

    if (!vap) {
        qdf_err("Invalid vap");
        return QDF_STATUS_E_INVAL;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("Invalid ic");
        return QDF_STATUS_E_INVAL;
    }

    if (ieee80211_acs_is_scan_in_progress(ic->ic_acs)) {
        qdf_err("There is an ACS scan in progress - cannot change AP power mode");
        return QDF_STATUS_SUCCESS;
    }

    /*
     * If there is a STA VAP and it is not connected, remain in LPI
     */
    if (ic->ic_sta_vap &&
        !ieee80211_vap_is_connected(ic->ic_sta_vap) &&
        !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {
        qdf_err("STA VAP is not connected - cannot change power mode");
        return QDF_STATUS_SUCCESS;
    }

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        qdf_err("Invalid channel");
        return QDF_STATUS_E_INVAL;
    }

    if (!IEEE80211_IS_CHAN_6GHZ(channel)) {
        /* If not a 6GHz channel then power type change is not required */
        return QDF_STATUS_SUCCESS;
    }

     bw = ieee80211_get_chan_width(channel);
     in_punc_bitmap = ieee80211_get_chan_punc_bitmap(channel);
     new_ap_pwr_type = wlan_reg_get_best_pwr_mode(ic->ic_pdev_obj,
                                                  channel->ic_freq,
                                                  channel->ic_vhtop_freq_seg2,
                                                  bw, in_punc_bitmap);

    status = __ieee80211_verfy_bw_in_given_pwr_type(ic, channel,
                                                    &new_ap_pwr_type);

    if (status == QDF_STATUS_SUCCESS) {
        bool send_tpc;

        if (!IEEE80211_IS_SPECIAL_VAP_ONLY(vap))
            qdf_info("Moving power mode to %u", new_ap_pwr_type);

        wlan_reg_set_ap_pwr_and_update_chan_list(ic->ic_pdev_obj,
                                                 new_ap_pwr_type);
        /*
         * Send TPC power command with new power values when the new channel
         * and current channel parameters are same.
         * No need to send TPC power command here if the channel parameters
         * are different as TPC power command will be sent when vap restarts
         * in the new channel.
         */
        send_tpc = IEEE80211_ARE_CHANS_EQUAL(ic->ic_curchan, channel);
        mlme_update_chan_list_and_restart_vaps(ic, send_tpc);

        /* Unconditionally update the power of the current channel here
         * based on new power mode and new AFC response.
         */
        ic->ic_curchan_eirp_power = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj,
                                                          channel->ic_freq,
                                                          channel->ic_vhtop_freq_seg2,
                                                          bw,
                                                          new_ap_pwr_type,
                                                          in_punc_bitmap,
                                                          false,
                                                          REG_MAX_CLIENT_TYPE,
                                                          false);
    }

    return status;
}

/*
 * ieee80211_update_mode_and_power_type:
 * Update the phymode and power type
 *
 * @vap: Pointer to VAP
 * @channel: Pointer to channel
 * @mode: Desired mode
 *
 * Return:
 * QDF_STATUS_SUCCESS: Mode and power type changed successfully
 * QDF_STATUS_E_INVAL: Mode/power type failed to be changed.
 */
QDF_STATUS ieee80211_update_mode_and_power_type(struct ieee80211vap *vap,
                                          struct ieee80211_ath_channel *channel,
                                          enum ieee80211_phymode mode)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    /* Change the desired mode only if different from the current mode */
    if (mode != vap->iv_des_mode) {
        vap->iv_afc_orig_mode = vap->iv_des_mode;
        status = wlan_set_desired_phymode(vap, mode);
        if (status) {
            qdf_err("Could not change the desired mode from %d to %d",
                    vap->iv_afc_orig_mode, mode);
            status = QDF_STATUS_E_INVAL;
            vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
            return status;
        }
    }

    status = ieee80211_change_ap_power_type(vap, channel);
    if (status) {
        qdf_err("Could not change AP power type");
        status = QDF_STATUS_E_INVAL;
        return status;
    }

    return status;
}
#endif /* CONFIG_AFC_SUPPORT */

enum phy_ch_width
ieee80211_convert_cwm_wd_to_phych_width(enum ieee80211_cwm_width cwm_width)
{
    switch(cwm_width) {
        case IEEE80211_CWM_WIDTH20:
            return CH_WIDTH_20MHZ;
        case IEEE80211_CWM_WIDTH40:
            return CH_WIDTH_40MHZ;
        case IEEE80211_CWM_WIDTH80:
            return CH_WIDTH_80MHZ;
        case IEEE80211_CWM_WIDTH160:
            return CH_WIDTH_160MHZ;
        case IEEE80211_CWM_WIDTH80_80:
            return CH_WIDTH_80P80MHZ;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320:
            return CH_WIDTH_320MHZ;
#endif
        case IEEE80211_CWM_WIDTH_MAX:
            return CH_WIDTH_MAX;
        case IEEE80211_CWM_WIDTHINVALID:
        default:
            return CH_WIDTH_INVALID;
    }
}

#ifdef WLAN_FEATURE_11BE
/*
 * ieee80211_find_std_channel:
 * Given a channel and ic, find the standard channel taking
 * into account new 5G 240MHz capabilities.
 *
 * @ic: Pointer to the ic structure
 * @channel: Pointer to the original channel
 *
 * Return:
 * 160/80MHz channel if 5G 240MHz is supported otherwise
 * return original channel
 */
struct ieee80211_ath_channel ieee80211_find_std_channel(
                                      struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel)
{
    struct ieee80211_ath_channel chan = empty_channel;

    if (IEEE80211_IS_CHAN_5GHZ_EHT240(channel)) {
        struct chan_params chan_p = {0};
        uint16_t pb = 0;
        chan_p.freq = channel->ic_freq;
        pb = ieee80211_intersect_puncture_bitmap(channel, IEEE80211_MODE_11BEA_EHT160, IEEE80211_MODE_11BEA_EHT320);
        set_chan_params_puncture_bitmap(&chan_p, pb);
        chan = ieee80211_find_dot11_channel(ic, &chan_p, IEEE80211_MODE_11BEA_EHT160);
        if (!IEEE80211_IS_CHAN_VALID(&chan)) {
            pb = ieee80211_intersect_puncture_bitmap(channel, IEEE80211_MODE_11BEA_EHT80, IEEE80211_MODE_11BEA_EHT320);
            set_chan_params_puncture_bitmap(&chan_p, pb);
            chan = ieee80211_find_dot11_channel(ic, &chan_p, IEEE80211_MODE_11BEA_EHT80);
            if (!IEEE80211_IS_CHAN_VALID(&chan)) {
                qdf_err("Could not find 160/80 MHz valid channel");
                return empty_channel;
            }
        }
    } else {
        chan = *channel;
    }

    return chan;
}
qdf_export_symbol(ieee80211_find_std_channel);

/*
 * ieee80211_find_non_punctured_channel:
 * Given a puncture pattern, find the a largest non-punctured segment including
 * the primary channel.
 *
 * @ic         : Pointer to the ic structure
 * @channel    : Pointer to EHT punctured channel
 *
 * Return:
 * Valid channel: Success
 * Empty channel: Failure
 */
struct ieee80211_ath_channel ieee80211_find_non_punctured_channel(
                                      struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel)
{
    enum ieee80211_cwm_width width = 0;
    enum ieee80211_phymode mode;
    uint8_t    sec_offset;
    uint16_t   puncture_pattern;
    qdf_freq_t primary_freq,
               center_freq80,
               center_freq160,
               center_freq320;
    struct chan_params chan_p = {0};

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        qdf_err("Invalid channel");
        return empty_channel;
    }

    /* If the channel is not punctured, return the original channel */
    if (!IEEE80211_IS_CHAN_PUNCTURED(channel)) {
        return *channel;
    }

    primary_freq = channel->ic_freq;
    if (IEEE80211_IS_CHAN_320MHZ(channel)) {
        center_freq160 = channel->ic_vhtop_freq_seg1;
        center_freq320 = channel->ic_vhtop_freq_seg2;
        center_freq80  = (primary_freq > center_freq160) ?
                             (center_freq160 + CHWIDTH_40) :
                             (center_freq160 - CHWIDTH_40);
    } else {
        center_freq80  = channel->ic_vhtop_freq_seg1;
        center_freq160 = channel->ic_vhtop_freq_seg2;
        center_freq320 = 0;
    }

    puncture_pattern = channel->op_puncture_bitmap;

    switch (channel->ic_flags & IEEE80211_CHAN_WIDTH_MASK) {
        case IEEE80211_CHAN_320MHZ:
            puncture_pattern &= PUNCTURE_320MHZ_MASK;
            if (!puncture_pattern) {
                break;
            }
            width = IEEE80211_CWM_WIDTH160;

            if (center_freq160 > center_freq320) {
                puncture_pattern >>= CHWIDTH_160/CHWIDTH_20;
            }
        /* fallthrough */

        case IEEE80211_CHAN_160MHZ:
            puncture_pattern &= PUNCTURE_160MHZ_MASK;
            if (!puncture_pattern) {
                break;
            }
            width = IEEE80211_CWM_WIDTH80;

            if (center_freq80 > center_freq160) {
                puncture_pattern >>= CHWIDTH_80/CHWIDTH_20;
            }
        /* fallthrough */

        case IEEE80211_CHAN_80MHZ:
            puncture_pattern &= PUNCTURE_80MHZ_MASK;
            if (!puncture_pattern) {
                break;
            }
            width = IEEE80211_CWM_WIDTH40;

            if (primary_freq > center_freq80) {
                puncture_pattern >>= CHWIDTH_40/CHWIDTH_20;
            }

            puncture_pattern &= (PUNCTURE_80MHZ_MASK >> (CHWIDTH_40/CHWIDTH_20));
            if (!puncture_pattern) {
                break;
            }
        /* fallthrough */

        default:
            width = IEEE80211_CWM_WIDTH20;
            break;
    }

    switch (ieee80211_secondary20_channel_offset(channel)) {
        case EXT_CHAN_OFFSET_ABOVE:
            sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCA;
            break;
        case EXT_CHAN_OFFSET_BELOW:
            sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCB;
            break;
        default:
            sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCN;
            break;
    }

    chan_p.freq   = channel->ic_freq;
    chan_p.cfreq2 = (width == IEEE80211_CWM_WIDTH320) ? center_freq320 : 0;
    mode          = ieee80211_get_composite_phymode(IEEE80211_MODE_BEA,
                                                    width,
                                                    sec_offset);
    return ieee80211_find_dot11_channel(ic, &chan_p, mode);
}
#else
struct ieee80211_ath_channel ieee80211_find_std_channel(
                                      struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel)
{

    return *channel;
}
qdf_export_symbol(ieee80211_find_std_channel);

struct ieee80211_ath_channel ieee80211_find_non_punctured_channel(
                                      struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel)
{
    return *channel;
}
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE
/*
 * ieee80211_find_legacy_non11be_channel:
 * Given a non punctured 320 MHz channel, downgrading its to possible
 * 160/80 MHz channel.If not able to find than return the original
 * channel.
 *
 * @ic         : Pointer to the ic structure
 * @channel    : Pointer to EHT punctured channel
 *
 * Return:
 * Valid 160/80 MHz channel: Success
 * Original 320 MHz  channel: Failure
 */
struct ieee80211_ath_channel ieee80211_find_legacy_non11be_channel(
                                      struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel)
{
    struct ieee80211_ath_channel chan = *channel;

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        qdf_err("Invalid channel");
        return empty_channel;
    }

    if (!ic) {
        qdf_err("ic is null");
        return empty_channel;
    }

        chan = ieee80211_find_non_punctured_channel(ic, channel);

        if (IEEE80211_IS_CHAN_320MHZ(&chan)) {
            struct chan_params chan_p = {0};
            chan_p.freq = chan.ic_freq;
            set_chan_params_puncture_bitmap(&chan_p, chan.op_puncture_bitmap);
            chan = ieee80211_find_dot11_channel(ic, &chan_p, IEEE80211_MODE_11BEA_EHT160);
            if (!IEEE80211_IS_CHAN_VALID(&chan)) {
                chan = ieee80211_find_dot11_channel(ic, &chan_p, IEEE80211_MODE_11BEA_EHT80);
                if (!IEEE80211_IS_CHAN_VALID(&chan)) {
                    qdf_err("Could not find 160/80 MHz valid channel");
                    chan = *channel;
                }
            }
        }

        return chan;
}
#else
struct ieee80211_ath_channel ieee80211_find_legacy_non11be_channel(
                                      struct ieee80211com *ic,
                                      struct ieee80211_ath_channel *channel)
{
    return *channel;
}
#endif /* WLAN_FEATURE_11BE */

#ifdef CONFIG_AFC_SUPPORT
/*
 * ieee80211_vendor_acs_find_random_channel:
 * Find the best random channel for External vendor ACS leveraging the
 * generic random channel selection API.
 *
 * @vap   : Input - Pointer to the VAP structure
 * @config: Input - Pointer to the random channel selection configuration
 *
 * Return:
 * Best random channel selected
 */
int ieee80211_vendor_acs_find_random_channel(struct ieee80211vap *vap,
                                             struct acs_random_chan_config *config)
{
    struct ieee80211_ath_channel best_chan = empty_channel;
    uint8_t channel_cnt = 1;
    uint8_t reason = QCA_WLAN_VENDOR_ACS_SELECT_REASON_AFC_TRIGGER;
    struct vendor_chan_info *channel_list;
    QDF_STATUS status;

    if (!vap) {
        qdf_err("vap is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* Find the best channel with random channel selection framework */

    best_chan = ieee80211_acs_find_random_channel(vap, config);
    if (!IEEE80211_IS_CHAN_VALID(&best_chan)) {
        return QDF_STATUS_E_INVAL;
    }

    /* Set the best channel selected, using CSA */
    channel_list = qdf_mem_malloc(sizeof(struct vendor_chan_info) *
                   channel_cnt);
    if (!channel_list){
        qdf_err("%s: qdf_mem_malloc failed.", __func__);
        return QDF_STATUS_E_NOMEM;
    }
    channel_list->chan_width = get_chwidth_phymode(ieee80211_chan2mode
                                                   (&best_chan));
    channel_list->pri_ch_freq = best_chan.ic_freq;
    channel_list->seg1_center_ch_freq = best_chan.ic_vhtop_freq_seg2;
    status = update_vendor_acs_channel(vap, reason, channel_cnt,
                                       channel_list);

    return status;
}
qdf_export_symbol(ieee80211_vendor_acs_find_random_channel);
#endif /* CONFIG_AFC_SUPPORT */

#ifdef CONFIG_BAND_6GHZ
static inline
qdf_freq_t ieee80211_find_left_edge_freq(uint16_t center_freq,
                                         uint16_t bandwidth)
{
    return (center_freq - bandwidth / 2);
}

static inline
bool ieee80211_is_freq_above_priority_threshold(uint16_t center_freq,
                                                uint16_t bandwidth,
                                                uint16_t priority_thresh_freq)
{
    qdf_freq_t left_edge_freq;

    left_edge_freq = ieee80211_find_left_edge_freq(center_freq, bandwidth);

    return (left_edge_freq > priority_thresh_freq);
}

static inline
qdf_freq_t ieee80211_get_chan_center(struct ieee80211_ath_channel *channel)
{
    if (IEEE80211_IS_CHAN_160MHZ(channel)
#ifdef WLAN_FEATURE_11BE
        || (IEEE80211_IS_CHAN_320MHZ(channel))
#endif /* WLAN_FEATURE_11BE */
        )
        return channel->ic_vhtop_freq_seg2;

    return channel->ic_vhtop_freq_seg1;
}

bool ieee80211_is_chan_priority(struct ieee80211com *ic,
                                struct ieee80211_ath_channel *channel)
{
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    bool is_pri_chan_priority;
    uint16_t bandwidth;
    uint16_t center_freq = ieee80211_get_chan_center(channel);
    uint16_t priority_threshold_freq = wlan_reg_get_thresh_priority_freq(pdev);

    if (IEEE80211_IS_CHAN_80_80MHZ(channel)) {
        bool is_sec_chan_priority;
        bandwidth = BW_80_MHZ;
        is_sec_chan_priority =
            ieee80211_is_freq_above_priority_threshold(channel->ic_vhtop_freq_seg2,
                                                       bandwidth,
                                                       priority_threshold_freq);
        if (!is_sec_chan_priority)
            return false;
    } else {
        bandwidth = ieee80211_get_chan_width(channel);
    }

    is_pri_chan_priority =
        ieee80211_is_freq_above_priority_threshold(center_freq,
                                                   bandwidth,
                                                   priority_threshold_freq);

    return is_pri_chan_priority;
}
#endif /* CONFIG_BAND_6GHZ */

#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)

#define INVALID_EDGE 0xFFF
#define INVALID_DBR 100
#define INVALID_PSD (-1270) /* -127 multiplied by 10 */

#define MAX_PUNC_MASK_LIMITS 3
struct pmask {
    int16_t offset[MAX_PUNC_MASK_LIMITS];
    int16_t dbr[MAX_PUNC_MASK_LIMITS];
};

enum puncture_type {
    PUNCTURE_TYPE_EDGE = 0,
    PUNCTURE_TYPE_INTERIM_20_PLUS,
    PUNCTURE_TYPE_INTERIM_20,
    PUNCTURE_TYPE_INVALID,
};

int16_t get_regmask_non_puncture(int16_t offset, uint16_t bw)
{
    uint16_t hbw = bw >> 1;
    int16_t mask;

    if (offset < 0) {
        offset = 0-offset;
    }

    if (offset >= ((bw * 3)/2)) {
        mask = -400;
    } else if (offset >= bw) {
        mask = -280 - ((120 * (offset - bw))/hbw);
    } else if (offset >= (hbw + 1)) {
        mask = -200 - ((80 * (offset - (hbw + 1)))/(hbw - 1));
    } else {
        mask = 0;
    }

    return mask;
}

int16_t pdbm1[3] = {0, -200, -280};
int16_t pdbm2[3] = {0, -200, -250};
int16_t pdbm3[3] = {0, -200, -230};

int16_t get_yval(int16_t x1, int16_t x2, int16_t y1, int16_t y2, int16_t x)
{
    int16_t num;
    int16_t den;
    static int i = 0;

    num = (y2 - y1) * 100;
    den = (x2 - x1);

    if (i < 0) {
        i++;
    }

    return ((y1 * 100) + (num*(x-x1))/den)/100;
}

int16_t get_regmask_puncture(int16_t offset, uint16_t bw, struct pmask *pmask)
{
    int16_t mask;

    offset *=10;

    if (offset <= pmask->offset[0]) {
        mask = pmask->dbr[0];
    } else if ((offset > pmask->offset[0]) && (offset < pmask->offset[1])) {
        mask = get_yval(pmask->offset[0], pmask->offset[1], pmask->dbr[0],pmask->dbr[1], offset);
    } else if (offset == pmask->offset[1]) {
        mask = pmask->dbr[1];
    } else if ((offset > pmask->offset[1]) && (offset < pmask->offset[2])) {
        mask = get_yval(pmask->offset[1], pmask->offset[2], pmask->dbr[1],pmask->dbr[2], offset);
    } else if (offset >= pmask->offset[2]) {
        mask = pmask->dbr[2];
    } else {
        mask = INVALID_DBR;
    }

    return mask;
}

int16_t get_regmask(int16_t offset, uint16_t bw, enum puncture_type punc_type,
                    struct pmask *pmask_le, struct pmask *pmask_l,
                    struct pmask *pmask_r, struct pmask *pmask_re)
{
    int16_t mask;
    int16_t mask_def;
    int16_t mask_le;
    int16_t mask_re;
    int16_t mask_l;
    int16_t mask_r;
    int16_t mask_punc;

    mask_def = get_regmask_non_puncture(offset, bw);
    if (punc_type == PUNCTURE_TYPE_INVALID)
        return mask_def;

    mask_le = get_regmask_puncture(offset, bw, pmask_le);
    mask_re = get_regmask_puncture(offset, bw, pmask_re);
    mask_l = get_regmask_puncture(offset, bw, pmask_l);
    mask_r = get_regmask_puncture(offset, bw, pmask_r);

    if (punc_type == PUNCTURE_TYPE_EDGE) {
        mask_punc = qdf_min(mask_le, mask_re);
    } else if (punc_type == PUNCTURE_TYPE_INTERIM_20_PLUS) {
        if ((pmask_l->offset[0] <= (offset * 10)) && ((offset * 10) <= pmask_r->offset[2]))
            mask_punc = __qdf_max(mask_l, mask_r);
        else
            mask_punc = qdf_min(mask_le, mask_re);
    } else if (punc_type == PUNCTURE_TYPE_INTERIM_20) {
        mask_punc = __qdf_max(mask_l, mask_r);
    }

    mask = qdf_min(mask_punc, mask_def);

    return mask;
}

int16_t get_psdlim(uint16_t freq,
        uint8_t num_freq_obj,
        struct afc_freq_obj *afc_freq_info)
{
    uint8_t i;
    int16_t min_psd = CHAN_MAX_PSD_POWER * EIRP_PWR_SCALE;
    bool chan_freq_found = false;

    for (i = 0; i < num_freq_obj; i++) {
        if ((freq >= afc_freq_info[i].low_freq)
            && (freq <= afc_freq_info[i].high_freq)) {
            chan_freq_found = true;
            if (afc_freq_info[i].max_psd < min_psd)
                min_psd = afc_freq_info[i].max_psd;
            /* Even though the frequency object is found, there may be more
             * matching frequency-object following it. Continue search until
             * the input frequency is out of range. */
            continue;
        }

        /* Assuming AFC payload is sorted in increasing order of frequencies,
         * Stop and return here
         */
        if (chan_freq_found)
            return min_psd / 10;
    }

    /* Handle for last frequency object */
    if (chan_freq_found)
            return min_psd / 10;

    return INVALID_PSD;
}

enum puncture_type get_pmask_limits(uint16_t bw, uint16_t puncture_bitmap,
       struct pmask *pmask_le, struct pmask *pmask_l,
       struct pmask *pmask_r, struct pmask *pmask_re)
{
    uint16_t punc_mask = 0;
    uint16_t pp = 0;
    int16_t i;
    int16_t num_valid_bits = 0;
    int16_t ledge = (bw >> 1) - bw;
    int16_t redge = bw - (bw >> 1);
    int16_t pledge = INVALID_EDGE;
    int16_t predge = INVALID_EDGE;
    int16_t pedge1 = INVALID_EDGE;
    int16_t pedge2 = INVALID_EDGE;
    int16_t punc_start = INVALID_EDGE;
    enum puncture_type punc_type;

    switch (bw) {
        case 80:
            punc_mask = PUNCTURE_80MHZ_MASK;
            num_valid_bits = 4;
            break;
        case 160:
            punc_mask = PUNCTURE_160MHZ_MASK;
            num_valid_bits = 8;
            break;
        case 320:
            punc_mask = PUNCTURE_320MHZ_MASK;
            num_valid_bits = 16;
            break;
        default:
            punc_mask = 0;
            qdf_debug("Bandwidth input invalid");
            return PUNCTURE_TYPE_INVALID;
    }

    pp = puncture_bitmap & punc_mask;
    if (!pp) {
        return PUNCTURE_TYPE_INVALID;
    }

    for (i = 0; i < num_valid_bits; i++) {
        if (!((1 << i) & pp) && (pledge == INVALID_EDGE)) {
            pledge = ledge + (i * 20);
        }
        if (!((1 << (num_valid_bits - 1 - i)) & pp) && (predge == INVALID_EDGE)) {
            predge = redge - (i * 20);
        }
        if ((punc_start != INVALID_EDGE) && (pedge1 == INVALID_EDGE) && !((1 << i) & pp)) {
            /* End of interim puncture */
            pedge1 = punc_start;
            pedge2 = ledge + (i * 20);
            punc_start = INVALID_EDGE;
        }
        if (((1 << i) & pp) && (punc_start == INVALID_EDGE) &&
            ((ledge + (i * 20)) > pledge)) {
            /* Start of interim puncture */
            punc_start = ledge + (i * 20);
        }
    }

    /* Find the puncture type */
    if ((pedge1 == INVALID_EDGE) && ((ledge != pledge) || (redge != predge))) {
        punc_type = PUNCTURE_TYPE_EDGE;
    } else if ((pedge2 - pedge1) >= 40) {
        punc_type = PUNCTURE_TYPE_INTERIM_20_PLUS;
    } else if ((pedge2 - pedge1) == 20){
        punc_type = PUNCTURE_TYPE_INTERIM_20;
    } else {
        punc_type = PUNCTURE_TYPE_INVALID;
    }

    pledge *= 10;
    predge *= 10;
    pedge1 *= 10;
    pedge2 *= 10;
    ledge *=10;
    redge *=10;

    if (punc_type == PUNCTURE_TYPE_EDGE) {
        qdf_debug("EDGE puncture\n");
        pmask_le->offset[0] = pledge - ((predge - pledge) / 2);
        pmask_le->dbr[0] = pdbm1[2];

        pmask_le->offset[1] = pledge - 5;
        pmask_le->dbr[1] = pdbm1[1];

        pmask_le->offset[2] = pledge;
        pmask_le->dbr[2] = pdbm1[0];

        pmask_re->offset[0] = predge;
        pmask_re->dbr[0] = pdbm1[0];

        pmask_re->offset[1] = predge + 5;
        pmask_re->dbr[1] = pdbm1[1];

        pmask_re->offset[2] = predge + ((predge - pledge) / 2);
        pmask_re->dbr[2] = pdbm1[2];
    } else if (punc_type == PUNCTURE_TYPE_INTERIM_20_PLUS) {
        /* type 2 mask */
        qdf_debug("INTERIM 20 PLUS puncture\n");
        /* Edge concurrent puncture */
        if ((ledge != pledge) || (redge != predge)) {
            pmask_le->offset[0] = pledge - ((pedge1 - pledge) / 2);
            pmask_le->dbr[0] = pdbm1[2];

            pmask_le->offset[1] = pledge - 5;
            pmask_le->dbr[1] = pdbm1[1];

            pmask_le->offset[2] = pledge;
            pmask_le->dbr[2] = pdbm1[0];

            pmask_re->offset[0] = predge;
            pmask_re->dbr[0] = pdbm1[0];

            pmask_re->offset[1] = predge + 5;
            pmask_re->dbr[1] = pdbm1[1];

            pmask_re->offset[2] = predge + ((predge - pedge2) / 2);
            pmask_re->dbr[2] = pdbm1[2];
        }

        pmask_l->offset[0] = pedge1;
        pmask_l->dbr[0] = pdbm2[0];

        pmask_l->offset[1] = pedge1 + 5;
        pmask_l->dbr[1] = pdbm2[1];

        pmask_l->offset[2] = pedge1 + ((pedge1 - pledge) >> 1);
        pmask_l->dbr[2] = pdbm2[2];

        pmask_r->offset[0] = pedge2 - ((predge - pedge2) >> 1);
        pmask_r->dbr[0] = pdbm2[2];

        pmask_r->offset[1] = pedge2 - 5;
        pmask_r->dbr[1] = pdbm2[1];

        pmask_r->offset[2] = pedge2;
        pmask_r->dbr[2] = pdbm2[0];
    } else if (punc_type == PUNCTURE_TYPE_INTERIM_20) {
        qdf_debug("INTERIM 20 puncture\n");
        pmask_l->offset[0] = pedge1;
        pmask_l->dbr[0] = pdbm3[0];

        pmask_l->offset[1] = pedge1 + 5;
        pmask_l->dbr[1] = pdbm3[1];

        pmask_l->offset[2] = pedge1 + 100;
        pmask_l->dbr[2] = pdbm3[2];

        pmask_r->offset[0] = pedge2 - 100;
        pmask_r->dbr[0] = pdbm3[2];

        pmask_r->offset[1] = pedge2 - 5;
        pmask_r->dbr[1] = pdbm3[1];

        pmask_r->offset[2] = pedge2;
        pmask_r->dbr[2] = pdbm3[0];
    } else {
        qdf_err("Investigate - Invalid puncture type!!!\n");
    }

    return punc_type;
}

QDF_STATUS reg_get_6g_min_psd(struct wlan_objmgr_pdev *pdev,
        qdf_freq_t freq, qdf_freq_t cfreq2,
        uint16_t puncture_bitmap, uint16_t bw, int16_t *min_psd)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    uint16_t freq_start;
    uint16_t freq_end;
    int16_t mask;
    int16_t psd_limit = 0;
    uint16_t hbw = bw >> 1;
    int16_t offset;
    uint16_t modoffset;
    uint16_t adj_freq_start;
    uint16_t adj_freq_end;
    struct pmask pmask_l = {0};
    struct pmask pmask_r = {0};
    struct pmask pmask_le = {0};
    struct pmask pmask_re = {0};
    enum puncture_type punc_type;
    struct afc_freq_obj *afc_info = NULL;
    uint8_t num_freq_obj;
    int i;

    if (!WLAN_REG_IS_6GHZ_CHAN_FREQ(cfreq2))
        return QDF_STATUS_E_INVAL;

    freq_start = cfreq2 - hbw;
    freq_end   = cfreq2 + hbw;

    adj_freq_start = __qdf_max(DEFAULT_LOW_6GFREQ, (cfreq2 - (3 * hbw)));
    adj_freq_end   = qdf_min((cfreq2 + (3 * hbw)), DEFAULT_HIGH_6GFREQ);

    *min_psd = 10000;

    punc_type = get_pmask_limits(bw, puncture_bitmap, &pmask_le, &pmask_l, &pmask_r, &pmask_re);

    status = wlan_reg_get_num_afc_freq_obj(pdev, &num_freq_obj);
    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("No frequency objects found!");
        return status;
    }

    afc_info = qdf_mem_malloc(num_freq_obj * sizeof(struct afc_freq_obj));
    if (!afc_info) {
        qdf_err("afc info malloc failed");
        return QDF_STATUS_E_NOMEM;
    }

    status = wlan_reg_get_afc_freq_range_and_psd_limits(pdev, num_freq_obj, afc_info);
    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("get afc info failed");
        goto err;
    }

    for (i = adj_freq_start; i <= adj_freq_end; i++) {
        offset = i - cfreq2;
        if (offset < 0) {
            modoffset = cfreq2 - i;
        } else {
            modoffset = i - cfreq2;
        }

        psd_limit = get_psdlim(i, num_freq_obj, afc_info);
        if (psd_limit == INVALID_PSD) {
            /* If PSD limit is invalid for usable freq and not punctured, return failure.
             * Other adjacent freq can be ignored.
             */
            if ((i >= freq_start) && (i < freq_end)) {
                if (!(puncture_bitmap & (1 << ((i - freq_start) / CHWIDTH_20)))) {
                    status = QDF_STATUS_E_INVAL;
                    goto err;
                }
            }
            continue;
        }

        if (modoffset <= ((bw * 3) >> 1)) {
            mask = get_regmask(offset, bw, punc_type, &pmask_le, &pmask_l, &pmask_r, &pmask_re);
            *min_psd = qdf_min((int16_t)(*min_psd), (int16_t)(psd_limit - mask));
        }
    }

err:
    if (afc_info)
        qdf_mem_free(afc_info);

    return status;
}
qdf_export_symbol(reg_get_6g_min_psd);

#endif /* WLAN_FEATURE_11BE && CONFIG_AFC_SUPPORT && CONFIG_BAND_6GHZ */

QDF_STATUS ieee80211_get_reg_phymode_ch_width(enum ieee80211_phymode phymode,
                                        enum reg_phymode *r_phymode,
                                        enum phy_ch_width *ch_width)
{
     uint16_t n_rows = QDF_ARRAY_SIZE(phymode_decoupler);

     if (phymode >= n_rows) {
         qdf_err("Phymode is invalid %d", phymode);
         return QDF_STATUS_E_FAILURE;
     }
     *r_phymode = phymode_decoupler[phymode][R_PHYMODE];
     *ch_width = phymode_decoupler[phymode][CH_WIDTH];
     return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ieee80211_get_reg_phymode_ch_width);
