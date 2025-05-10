/*
 * Copyright (c) 2011,2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 *  Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 */

/*
 * Regulatory domain and DFS implementation
 */

#include <osdep.h>
#include <ieee80211_regdmn.h>
#include <ieee80211_channel.h>
#include <ieee80211_ucfg.h>
#if UMAC_SUPPORT_CFG80211
#include <ieee80211_cfg80211.h>
#endif
#include <wlan_mlme_dispatcher.h>
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <wlan_reg_ucfg_api.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <reg_services_public_struct.h>
#include <wlan_reg_services_api.h>
#include <ieee80211_mlme_priv.h>
#include <wlan_lmac_if_api.h>
#include "ieee80211_regdmn_dispatcher.h"
#include <wlan_reg_services_api.h>
#include <wlan_reg_channel_api.h>
#include <wlan_reg_tgt_api.h>
#include <ol_if_athvar.h>
#include <wlan_mlme_if.h>
#if ATH_ACS_DEBUG_SUPPORT
#include <acs_debug.h>
#endif
#include <reg_services_common.h>
#include <ieee80211_ioctl.h>

/* Difference between two channel numbers. Ex: 40 - 36 = 4 */
#define CHAN_DIFF    4
/* Size of triplets in the Country IE */
#define TRIPLET_SIZE 3
#define MAX_BUF_SIZE 115
#define MAX_BUF1_SIZE 6
#define MAX_BUF3_SIZE 10

static int
ieee80211_set_countrycode(struct ieee80211com *ic, char isoName[3], u_int16_t cc, enum ieee80211_clist_cmd cmd);
static void regdmn_populate_channel_list_from_map(const struct reg_dmn_op_class_map_t *map,
                          u_int8_t reg_class, struct ieee80211_node *ni);
static void wlan_notify_country_changed(void *arg, wlan_if_t vap)
{
    char *country = (char*)arg;
    IEEE80211_DELIVER_EVENT_COUNTRY_CHANGE(vap, country);
}

int ieee80211_reg_program_opclass_tbl(struct ieee80211com *ic, uint8_t opclass)
{
    if (opclass > OPCLASS_TBL_MAX) {
        qdf_err("invalid opclass table index");
        return -EINVAL;
    }

    ic->ic_opclass_tbl_idx = opclass;
    return 0;
}
qdf_export_symbol(ieee80211_reg_program_opclass_tbl);

int ieee80211_reg_get_opclass_tbl(struct ieee80211com *ic, uint8_t *opclass)
{
    *opclass = ic->ic_opclass_tbl_idx;
    return 0;
}
qdf_export_symbol(ieee80211_reg_get_opclass_tbl);

int ieee80211_reg_program_cc(struct ieee80211com *ic,
        char *isoName, u_int16_t cc)
{
    struct cc_regdmn_s rd;

    qdf_mem_zero(&rd, sizeof(rd));

    rd.flags = INVALID_CC;
    if(isoName && isoName[0] && isoName[1]) {
        /* Map the ISO name ' ', 'I', 'O' */
        if (isoName[2] == 'O') {
            ic->ic_is_radio_outdoor = true;
            ic->ic_is_radio_indoor  = false;
            ic->ic_opclass_tbl_idx = 0;
        }
        else if (isoName[2] == 'I') {
            ic->ic_is_radio_indoor  = true;
            ic->ic_is_radio_outdoor = false;
            ic->ic_opclass_tbl_idx = 0;
        }
        else if (ic->ic_opclass_tbl_idx) {
            ic->ic_is_radio_outdoor = false;
            ic->ic_is_radio_indoor  = false;
            isoName[2] = ic->ic_opclass_tbl_idx;
        }
        else if ((isoName[2] == ' ') || (isoName[2] == 0)) {
            ic->ic_is_radio_outdoor = false;
            ic->ic_is_radio_indoor  = false;
            ic->ic_opclass_tbl_idx = 0;
        }
        else if ((isoName[2] >= '1') && (isoName[2] <= '9')) {
            ic->ic_is_radio_outdoor = false;
            ic->ic_is_radio_indoor  = false;
            ic->ic_opclass_tbl_idx = isoName[2] - '0';
            /* Convert ascii value to numeric one */
            isoName[2] = isoName[2] - '0';
        }
        rd.cc.alpha[0] = isoName[0];
        rd.cc.alpha[1] = isoName[1];
        rd.cc.alpha[2] = isoName[2];
        rd.flags = ALPHA_IS_SET;
    } else if (cc) {
        rd.cc.country_code = cc;
        rd.flags = CC_IS_SET;
    }

    return ieee80211_regdmn_program_cc(ic->ic_pdev_obj, &rd);
}
qdf_export_symbol(ieee80211_reg_program_cc);

/**
 * ieee80211_is_non_dfs_chans_available() - Compute the number of non-DFS.
 * channels available in the current regdomain.
 * @ic: Pointer to radio object.
 *
 * Return: Number of non-DFS channels.
 */
static int ieee80211_is_non_dfs_chans_available(struct ieee80211com *ic)
{
    int i, num_non_dfs_chans = 0;
    struct regulatory_channel *cur_chan_list;

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));

    if (!cur_chan_list)
        return num_non_dfs_chans;

    if (wlan_reg_get_current_chan_list(
            ic->ic_pdev_obj, cur_chan_list) != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        qdf_mem_free(cur_chan_list);
        return num_non_dfs_chans;
    }

    for (i = 0; i < NUM_CHANNELS; i++) {
        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
            continue;
        if (!wlan_reg_is_dfs_for_freq(ic->ic_pdev_obj,
                                      cur_chan_list[i].center_freq))
            num_non_dfs_chans ++;
    }

    qdf_mem_free(cur_chan_list);
    return num_non_dfs_chans;
}

/*
 * Remove wireless mode flags from mode_select based on input phybitmap.
 * @mode_select: Pointer to mode_select.
 * @phybitmap: Phy bitmap retrieved from regulatory pdev_priv_obj.
 *
 */
#ifdef WLAN_FEATURE_11BE
void
ieee80211_regdmn_remove_phybitmap_from_modeselect(uint64_t *mode_select,
                                                  uint16_t phybitmap)
{
    if (phybitmap & REGULATORY_PHYMODE_NO11AX) {
        *mode_select &= ~(WIRELESS_11AX_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11AC) {
        *mode_select &= ~(WIRELESS_11AC_MODES);
    }

    if (phybitmap & REGULATORY_CHAN_NO11N) {
        *mode_select &= ~(WIRELESS_11N_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11G) {
        *mode_select &= ~(WIRELESS_11G_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11B) {
        *mode_select &= ~(WIRELESS_11B_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11A) {
        *mode_select &= ~(WIRELESS_11A_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11BE) {
        *mode_select &= ~(WIRELESS_11BE_MODES);
    }

}
#else
void
ieee80211_regdmn_remove_phybitmap_from_modeselect(uint64_t *mode_select,
                                                  uint16_t phybitmap)
{
    if (phybitmap & REGULATORY_PHYMODE_NO11AX) {
        *mode_select &= ~(WIRELESS_11AX_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11AC) {
        *mode_select &= ~(WIRELESS_11AC_MODES);
    }

    if (phybitmap & REGULATORY_CHAN_NO11N) {
        *mode_select &= ~(WIRELESS_11N_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11G) {
        *mode_select &= ~(WIRELESS_11G_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11B) {
        *mode_select &= ~(WIRELESS_11B_MODES);
    }

    if (phybitmap & REGULATORY_PHYMODE_NO11A) {
        *mode_select &= ~(WIRELESS_11A_MODES);
    }

}
#endif

int ieee80211_reg_get_current_chan_list(
        struct ieee80211com *ic,
        struct regulatory_channel *reg_chan_list)
{
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    uint8_t num_non_dfs_chans;
#endif

#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    /* If the spoof dfs check has failed, AP should come up only in NON-DFS
     * channels. "Setregdomain and setcountry" cmd will rebuild the ic channel list
     * (could include DFS and non-DFS chans). The following code will ensure
     * that ic channel list still consists of non-DFS channels if spoof check
     * failed.
     */
    ic->dfs_spoof_test_regdmn = 1;
    if (ic->ic_rebuilt_chanlist)
        ic->ic_rebuilt_chanlist = 0;

    ieee80211_dfs_non_dfs_chan_config(ic);

#endif /* HOST_DFS_SPOOF_TEST */

#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    /* In case of scan failure event from FW due to dfs violation,
     * rebuilding channel list to have only non-DFS channel. If a regdomain
     * has only DFS channel, then bring down the vaps.
     */
    if (ic->ic_is_dfs_scan_violation) {
        num_non_dfs_chans = ieee80211_is_non_dfs_chans_available(ic);

        if (num_non_dfs_chans == 0) {
            ic->no_chans_available = 1;
            ieee80211_bringdown_vaps(ic);
            return -1;
        }
    }
#endif
    if (!ieee80211_reg_find_first_valid_freq(ic, IEEE80211_MODE_AUTO,
                                             REG_CURRENT_PWR_MODE)) {
        ic->no_chans_available = 1;
        ieee80211_bringdown_vaps(ic);
    } else {
        ic->no_chans_available = 0;
    }
    return 0;
}
qdf_export_symbol(ieee80211_reg_get_current_chan_list);

#ifdef WLAN_REG_PARTIAL_OFFLOAD
int ieee80211_reg_create_ieee_chan_list(
        struct ieee80211com *ic)
{
    struct regulatory_channel *curr_chan_list;
    int err;

    curr_chan_list = qdf_mem_malloc(NUM_CHANNELS*sizeof(struct regulatory_channel));
    if(curr_chan_list == NULL) {
        qdf_print("%s: fail to alloc", __func__);
        return -1;
    }

    ieee80211_regdmn_get_current_chan_list(ic->ic_pdev_obj, curr_chan_list);
    err = ieee80211_reg_get_current_chan_list(ic, curr_chan_list);
    qdf_mem_free(curr_chan_list);

    return err;
}
qdf_export_symbol(ieee80211_reg_create_ieee_chan_list);
#endif

/**
 * ieee80211_get_max5gbw() - Get maximum 5G bandwidth supported by
 * the country/regdomain
 * @ic: Pointer to ieee80211com structure
 *
 * Return : maximum 5G bandwidth supported or 0
 */
uint16_t ieee80211_get_max5gbw(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    uint16_t cc, regdomain;
    uint16_t max_bw_5g = 0;
    bool offload_enabled = 0;

    cc = ieee80211_getCurrentCountry(ic);
    offload_enabled = wlan_reg_is_regdb_offloaded(psoc);

    if (cc) {
        if (offload_enabled)
             max_bw_5g = wlan_reg_get_max_bw_5G_for_fo(ic->ic_pdev_obj);
        else
             wlan_reg_get_max_5g_bw_from_country_code(ic->ic_pdev_obj,
                                                      cc, &max_bw_5g);
    } else {
       regdomain = ieee80211_get_regdomain(ic);
    if (regdomain) {
         if (offload_enabled)
              max_bw_5g = wlan_reg_get_max_bw_5G_for_fo(ic->ic_pdev_obj);
          else
              wlan_reg_get_max_5g_bw_from_regdomain(ic->ic_pdev_obj,
                                                    regdomain, &max_bw_5g);
       }
    }

    return max_bw_5g;
}

uint16_t ieee80211_getCurrentCountry(struct ieee80211com *ic)
{
    struct cc_regdmn_s rd;

    qdf_mem_set(&rd, sizeof(struct cc_regdmn_s), 0);
    rd.flags = CC_IS_SET;
    ieee80211_regdmn_get_current_cc(ic->ic_pdev_obj, &rd);

    return rd.cc.country_code;
}
qdf_export_symbol(ieee80211_getCurrentCountry);

uint16_t ieee80211_getCurrentCountryISO(struct ieee80211com *ic, char *str)
{
    struct cc_regdmn_s rd;

    rd.flags = ALPHA_IS_SET;
    ieee80211_regdmn_get_current_cc(ic->ic_pdev_obj, &rd);
    qdf_mem_copy(str, rd.cc.alpha, sizeof(rd.cc.alpha));

    if (str[0] && str[1]) {
        if (ic->ic_is_radio_outdoor)
            str[2] = 'O';
        else if (ic->ic_is_radio_indoor)
            str[2] = 'I';
        else if (ic->ic_opclass_tbl_idx)
            str[2] = ic->ic_opclass_tbl_idx;
        else
            str[2] = ' ';
    }

    return 0;
}
qdf_export_symbol(ieee80211_getCurrentCountryISO);

uint32_t ieee80211_get_regdomain(struct ieee80211com *ic)
{
    struct cc_regdmn_s rd;
    uint32_t regdomain = 0;
    qdf_freq_t low_5g, high_5g;

    qdf_mem_set(&rd, sizeof(struct cc_regdmn_s), 0);
    rd.flags = REGDMN_IS_SET;
    ieee80211_regdmn_get_current_cc(ic->ic_pdev_obj, &rd);
    ieee80211_regdmn_get_freq_range(ic->ic_pdev_obj, NULL, NULL, &low_5g, &high_5g);

    REG_SET_BITS(regdomain, 0, 16, rd.cc.regdmn.reg_2g_5g_pair_id);
    if (wlan_reg_is_range_overlap_6g(low_5g, high_5g))
        REG_SET_BITS(regdomain, 16, 16, rd.cc.regdmn.sixg_superdmn_id);

    return regdomain;
}

qdf_export_symbol(ieee80211_get_regdomain);

/*
 * Set country code
 */
int
ieee80211_set_ctry_code(struct ieee80211com *ic, char *isoName, u_int16_t cc, enum ieee80211_clist_cmd cmd)
{
    int  error = 0;
    uint8_t ctry_iso[REG_ALPHA2_LEN + 1];

    ieee80211_getCurrentCountryISO(ic, ctry_iso);
    if (!cc) {
        if (isoName == NULL) {

        } else if((ctry_iso[0] == isoName[0]) &&
                (ctry_iso[1] == isoName[1]) &&
                (ctry_iso[2] == isoName[2])) {
            return 0;
        }
    }

    IEEE80211_DISABLE_11D(ic);

    error = ic->ic_set_country(ic, isoName, cc, cmd);

    return error;
}

int ieee80211_set_ctry_code_continue(struct ieee80211com *ic,
                             bool no_chanchange)
{
    uint8_t ctry_iso[REG_ALPHA2_LEN + 1];
    int err;
    struct wlan_objmgr_pdev *pdev;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null", __func__);
        return -1;
    }

    /* update the country information for 11D */
    ieee80211_getCurrentCountryISO(ic, ctry_iso);

    /* update channel list */
    err = ieee80211_update_channellist(ic, 1, no_chanchange);
    if (err)
        return err;

    ieee80211_build_countryie_all(ic, ctry_iso);

    if (IEEE80211_IS_COUNTRYIE_ENABLED(ic)) {
        IEEE80211_ENABLE_11D(ic);
    }

    if (ic->ic_flags_ext2 & IEEE80211_FEXT2_RESET_PRECACLIST) {
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                QDF_STATUS_SUCCESS) {
            return -1;
        }

        mlme_dfs_reset_precaclists(pdev);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    }
    /* notify all vaps that the country changed */
    wlan_iterate_vap_list(ic, wlan_notify_country_changed, (void *)&ctry_iso);

    return 0;
}

/**
 * ieee80211_try_finding_a_valid_chan - Try to find a channel based on input channel
 * params. If not found, fallback and try to find any valid channel.
 * @ic: Pointer to ic.
 * @chan_p: chan_params for finding the channel.
 * @new_chan: Pointer to new channel output.
 * @mode: Input phymode.
 *
 * Return: True if a valid channel is found. False otherwise.
 */
static bool
ieee80211_try_finding_a_valid_chan(struct ieee80211com *ic,
                                   struct chan_params *chan_p,
                                   struct ieee80211_ath_channel *new_chan,
                                   uint64_t flags,
                                   enum ieee80211_phymode mode)
{
    /*
     * Try to find if the current channel is available with current channel params and ic_flags
     */
    *new_chan = ieee80211_find_channel(ic, chan_p, flags);
    if (IEEE80211_IS_CHAN_VALID(new_chan)) {
        return true;
    }

    /*
     * If not found, fallback and try to find any valid channel in the current
     * mode (may even be in different band)
     */
    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_MLME,
            "Current channel not supported. Configuring to a random channel\n");
    chan_p->freq = 0;
    chan_p->cfreq2 = 0;
    set_chan_params_puncture_bitmap(chan_p, 0);

    *new_chan = ieee80211_find_dot11_channel(ic, chan_p, mode);
    if (IEEE80211_IS_CHAN_VALID(new_chan)) {
        return true;
    }

    /*
     * If no channel is found in current mode, try to find a valid channel in Auto mode
     */
    qdf_info("No channel found for the given Mode [%d] - Finding in auto mode", mode);
    *new_chan = ieee80211_find_dot11_channel(ic, chan_p, 0);
    if (IEEE80211_IS_CHAN_VALID(new_chan)) {
        return true;
    }

    return false;
}

/**
 * ieee80211_update_desmode_and_set_channel - Update the des_mode of the vaps, if
 * the new channel mode is different from the current channel mode.
 * @ic: Pointer to ic.
 * @chan: New channel.
 * @input_mode: Current channel operating modeo.
 *
 * Return: None.
 */
static void
ieee80211_update_desmode_and_set_channel(struct ieee80211com *ic,
                                         struct ieee80211_ath_channel *chan,
                                         enum ieee80211_phymode input_mode)
{
    wlan_if_t tmpvap;
    enum ieee80211_phymode chan_mode;

    chan_mode = ieee80211_chan2mode(chan);
    if (input_mode != chan_mode) {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            tmpvap->iv_des_mode = chan_mode;
            tmpvap->iv_des_hw_mode = chan_mode;
        }
    }

    ieee80211_set_channel_for_cc_change(ic, chan);
}

int ieee80211_setctry_tryretaining_curchan(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    struct chan_params chan_p = {0};
    uint64_t flags;
    uint32_t mode;
    int32_t err = 0;
    struct ieee80211_ath_channel new_chan_obj;
    struct ieee80211_ath_channel *new_chan = &new_chan_obj;
    bool is_valid_chan_found;

    chan_p.freq = ieee80211_chan2freq(ic, chan);
    chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
    flags = chan->ic_flags;
    mode = ieee80211_chan2mode(chan);

    /* Reset precac channel list when user changes the country code */
    ic->ic_flags_ext2 |= IEEE80211_FEXT2_RESET_PRECACLIST;
    err = ieee80211_set_ctry_code(ic, ic->ic_set_ctry_params->isoName, ic->ic_set_ctry_params->cc, ic->ic_set_ctry_params->cmd);
    if (err)
        return err;

    err = ieee80211_set_ctry_code_continue(ic, ic->ic_set_ctry_params->no_chanchange_during_cc);
    if (err)
        return err;

    if (!ic->ic_set_ctry_params->no_chanchange_during_cc){
        qdf_nofl_debug("[%d] Skip current channel retention during"
                       "set_country operation", __LINE__);
        return EOK;
    }

    is_valid_chan_found = ieee80211_try_finding_a_valid_chan(ic, &chan_p, new_chan, flags, mode);
    if (is_valid_chan_found)
        ieee80211_update_desmode_and_set_channel(ic, new_chan, mode);
    else
        qdf_err("No valid channel found in new country");

    ic->ic_flags_ext2 &= ~IEEE80211_FEXT2_RESET_PRECACLIST;
    return 0;
}

int ieee80211_set_channel_for_cc_change(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    wlan_if_t tmpvap;
    struct chan_params chan_p = {0};

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        if(tmpvap) {
            if (IEEE80211_IS_CHAN_VALID(chan)) {
                chan_p.freq = chan->ic_freq;
                if (ieee80211_chan2mode(chan) != tmpvap->iv_des_mode)
                     chan_p.cfreq2 = tmpvap->iv_des_cfreq2_80_80;
                else
                     chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
		set_chan_params_puncture_bitmap(&chan_p, 0);

                if (!IEEE80211_IS_MLO_BRIDGE_VAP(tmpvap))
                    wlan_set_channel(tmpvap, &chan_p);
            } else {
                IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL,
                            IEEE80211_MSG_MLME,
                            "No valid channel to be selected in current country\n");
                return -EINVAL;
            }
        }
    }
    return 0;
 }

int
ieee80211_set_country_code(struct ieee80211com *ic, char *isoName, u_int16_t cc, enum ieee80211_clist_cmd cmd)
{
    struct ieee80211_ath_channel *chan;
    int ret;

    ic->ic_set_ctry_params = (struct ieee80211_set_country_params*)
        qdf_mem_malloc(sizeof(struct ieee80211_set_country_params));

    if (!ic->ic_set_ctry_params)
        return -ENOMEM;

    /* Store the set country parameters */
    ic->ic_set_ctry_params->cc = cc;
    if(isoName)
        qdf_mem_copy(ic->ic_set_ctry_params->isoName, isoName, REG_ALPHA2_LEN+1);
    ic->ic_set_ctry_params->cmd = cmd;
    chan = ieee80211_get_current_channel(ic);

    if (ieee80211_get_num_active_vaps(ic) != 0)
        ic->ic_set_ctry_params->no_chanchange_during_cc = true;

    /* Bring the vaps down, set the country and then bring the vaps up.*/
    ret = osif_restart_for_config(ic, ieee80211_setctry_tryretaining_curchan, chan);

    qdf_mem_free(ic->ic_set_ctry_params);
    return ret;
}

void ieee80211_set_country_code_assoc_sm(void *data)
{
    struct ieee80211com *ic = (struct ieee80211com *)data;
    struct assoc_sm_set_country *set_country = ic->set_country;
    struct ieee80211_node *ni = set_country->ni;
    struct ieee80211_mlme_priv    *mlme_priv = set_country->vap->iv_mlme_priv;

    if (ieee80211_set_ctry_code(ic, (char*)ni->ni_cc,
                set_country->cc, set_country->cmd) == 0) {
        if (ni->ni_capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT)
            ieee80211_ic_doth_set(ic);
    }

    OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);
    IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_SET_COUNTRY(set_country->vap, 0);
}

void ieee80211_cfg80211_update_channel_list(void *data)
{
    struct ieee80211com *ic = (struct ieee80211com *)data;

    wlan_cfg80211_update_channel_list(ic);
}

#ifdef QCA_R2P_UPDATE_ENABLED
void ieee80211_read_set_home_chan_r2p_table(void *data)
{
    struct ieee80211com *ic = (struct ieee80211com *)data;

    ic->ic_read_set_home_chan_r2p_table(ic);
}
#endif

void
ieee80211_update_spectrumrequirement(struct ieee80211vap *vap,
        bool *thread_started)
{
    /*
     * 1. If not multiple-domain capable, check to update the country IE.
     * 2. If multiple-domain capable,
     *    a. If the country has been set by using desired country,
     *       then it is done, the ie has been updated.
     *       For IBSS or AP mode, if we are DFS owner, then need to enable Radar detect.
     *    b. If the country is not set, if no AP or peer country info,
     *       just assuming legancy mode.
     *       If we have AP or peer country info, using default to see if AP accept for now.
     */
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni = vap->iv_bss;
    uint8_t ctry_iso[REG_ALPHA2_LEN + 1];

    if (!ieee80211_ic_2g_csa_is_set(vap->iv_ic))
        ieee80211_ic_doth_clear(ic);
    ieee80211_getCurrentCountryISO(ic, ctry_iso);

    if (ic->ic_country.isMultidomain == 0) {
        if (ni->ni_capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT) {
            if (!IEEE80211_IS_COUNTRYIE_ENABLED(ic)) {
                ieee80211_build_countryie_all(ic, ctry_iso);
            }
            ieee80211_ic_doth_set(ic);
        }

        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_SET_COUNTRY(vap,0);
        return;
    }

    if (IEEE80211_HAS_DESIRED_COUNTRY(ic)) {
        /* If the country has been set, enabled the flag */
        if( (ctry_iso[0] == ni->ni_cc[0]) &&
            (ctry_iso[1] == ni->ni_cc[1]) &&
            (ctry_iso[2] == ni->ni_cc[2])) {
            if (ni->ni_capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT) {
                ieee80211_ic_doth_set(ic);
            }

            IEEE80211_ENABLE_11D(ic);
            IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_SET_COUNTRY(vap,0);
            return;
        }
    }

    if ((ni->ni_cc[0] == 0)   ||
        (ni->ni_cc[1] == 0)   ||
        (ni->ni_cc[0] == ' ') ||
        (ni->ni_cc[1] == ' ')) {
        if (ni->ni_capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT) {
            ieee80211_ic_doth_set(ic);
        }

        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_SET_COUNTRY(vap,0);
        return;
    }

    // Update the cc only for platforms that request this : Currently, only Windows.
    if (ieee80211_ic_disallowAutoCCchange_is_set(ic)) {
        if ((ni->ni_cc[0] == ctry_iso[0] &&
             ni->ni_cc[1] == ctry_iso[1] &&
             ni->ni_cc[2] == ctry_iso[2]))
        {
            ieee80211_build_countryie_all(ic, ctry_iso);
            if (ni->ni_capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT) {
                ieee80211_ic_doth_set(ic);
            }
        }

        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_SET_COUNTRY(vap,0);
    }
    else {
        // If ignore11dBeacon, using the original reg. domain setting.
        if (!IEEE80211_IS_11D_BEACON_IGNORED(ic)) {
            ic->set_country->cc = 0;
            ic->set_country->cmd = CLIST_UPDATE;
            ic->set_country->ni = ni;
            ic->set_country->vap = vap;
            qdf_sched_work(NULL, &ic->assoc_sm_set_country_code);
            *thread_started = true;
        } else {
            /* Post an event to move to next state. */
            IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_SET_COUNTRY(vap,0);
        }
    }
}

#ifdef WLAN_REG_PARTIAL_OFFLOAD
void
ieee80211_set_regclassids(struct ieee80211com *ic, const u_int8_t *regclassids, u_int nregclass)
{
    int i;

    if (nregclass >= IEEE80211_REGCLASSIDS_MAX)
        nregclass = IEEE80211_REGCLASSIDS_MAX;

    ic->ic_nregclass = nregclass;

    for (i = 0; i < nregclass; i++)
        ic->ic_regclassids[i] = regclassids[i];
}
#endif

#ifdef BEACON_CHANLIST_COMPRESSION
bool
ieee80211_chanlist_compression_possible(struct regulatory_channel *curchan,
                                        u_int8_t prevchan,
                                        struct country_ie_triplet *pTriplet)
{
    bool cmp_possible = false;
    uint8_t chan_ieee_separation;

    if (wlan_reg_is_24ghz_ch_freq(curchan->center_freq)) {
        chan_ieee_separation = 1;
        if ((curchan->chan_num == prevchan + chan_ieee_separation) &&
            (pTriplet->maxtxpwr == curchan->tx_power))
            cmp_possible = true;
    } else if (wlan_reg_is_5ghz_ch_freq(curchan->center_freq) ||
               wlan_reg_is_6ghz_chan_freq(curchan->center_freq)) {
        if (WLAN_REG_IS_49GHZ_FREQ(curchan->center_freq)) {
            chan_ieee_separation = 1;
            if ((curchan->chan_num == prevchan + chan_ieee_separation) &&
                (pTriplet->maxtxpwr == curchan->tx_power))
                cmp_possible = true;
        } else {
            chan_ieee_separation = 4;
            if ((curchan->chan_num == prevchan + chan_ieee_separation) &&
                (pTriplet->maxtxpwr == curchan->tx_power))
                cmp_possible = true;
        }
    }
    return cmp_possible;
}
#endif

#ifdef CONFIG_BAND_6GHZ
/**
 * ieee80211_set_6G_opclass_triplets() - Set ic_enable_additional_triplets with the input max_bw.
 * @ic: Pointer to ic.
 * @max_bw: Maximum bandwidth.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ieee80211_set_6G_opclass_triplets(struct ieee80211com *ic,
                                             uint16_t max_bw)
{
   if ((max_bw != BW_20_MHZ) && (max_bw != BW_40_MHZ) &&
       (max_bw != BW_80_MHZ) && (max_bw != BW_160_MHZ)
#ifdef WLAN_FEATURE_11BE
       && max_bw != BW_320_MHZ
#endif
      )
       return QDF_STATUS_E_FAILURE;

   ic->ic_enable_additional_triplets = max_bw;

   return QDF_STATUS_SUCCESS;
}

/**
 * ieee80211_add_additional_triplets() - Increment num_opclass based on the
 * value present in ic_enable_additional_triplets, to enable additional
 * operating triplets in the country IE.
 * @ic: Pointer to ic.
 * @num_opclass: Pointer to num_opclass.
 *
 * Return: void
 */
static void ieee80211_add_additional_triplets(struct ieee80211com *ic,
                                              uint8_t *num_opclass)
{
   switch(ic->ic_enable_additional_triplets) {
          case 0:
          case BW_20_MHZ:
                         break;
          case BW_40_MHZ:
                         /* Since opclass 132 appears twice in the array "sixg_opclss_arr",
                          * (First time for IEEE80211_MODE_11AXA_HE40PLUS  and second time
                          * for IEEE80211_MODE_11AXA_HE40MINUS) if the device supports both
			  * 40+ and 40-, the opclass will appear twice in the country IE.
			  * To avoid that, increment *num_opclass by 2, to add 132 only once.
                          */
                         *num_opclass += 2;
                         break;
          case BW_80_MHZ:
                         *num_opclass += 3;
                         break;
          case BW_160_MHZ:
                          *num_opclass += 4;
                          break;
#ifdef WLAN_FEATURE_11BE
          case BW_320_MHZ:
                          *num_opclass += 5;
                          break;
#endif
          default:
                  qdf_debug("%d is an invalid input bandwidth",
                            ic->ic_enable_additional_triplets);
                  break;
   }
}

uint8_t ieee80211_get_num_pwr_levels(struct wlan_objmgr_pdev *pdev,
                                     qdf_freq_t freq,
                                     enum ieee80211_cwm_width ch_width,
                                     bool is_psd)
{
    uint8_t num_pwr_levels;

    if (is_psd) {
        switch (ch_width) {
                case IEEE80211_CWM_WIDTH20:
                     num_pwr_levels = 1;
                     break;
                case IEEE80211_CWM_WIDTH40:
                     num_pwr_levels = 2;
                      break;
                case IEEE80211_CWM_WIDTH80:
                     num_pwr_levels = 4;
                     break;
                case IEEE80211_CWM_WIDTH160:
                case IEEE80211_CWM_WIDTH80_80:
                     num_pwr_levels = 8;
                     break;
#ifdef WLAN_FEATURE_11BE
                case IEEE80211_CWM_WIDTH320:
                     num_pwr_levels = 16;
                     break;
#endif
                default:
                     return 1;
         }
     } else {
        switch (ch_width) {
                case IEEE80211_CWM_WIDTH20:
                     num_pwr_levels = 1;
                     break;
                case IEEE80211_CWM_WIDTH40:
                     num_pwr_levels = 2;
                     break;
                case IEEE80211_CWM_WIDTH80:
                     num_pwr_levels = 3;
                           break;
                case IEEE80211_CWM_WIDTH160:
                case IEEE80211_CWM_WIDTH80_80:
                     num_pwr_levels = 4;
                     break;
#ifdef WLAN_FEATURE_11BE
                case IEEE80211_CWM_WIDTH320:
                     num_pwr_levels = 5;
                     break;
#endif
                default:
                     return 1;
       }
     }

     return num_pwr_levels;
}

#ifdef CONFIG_AFC_SUPPORT
QDF_STATUS
ieee80211_send_afc_cmd(struct ieee80211com *ic,
                       enum reg_afc_serv_resp_format serv_resp_format,
                       enum reg_afc_cmd_type cmd_type)
{
    struct reg_afc_resp_rx_ind_info afc_ind_obj;

    afc_ind_obj.cmd_type = cmd_type;
    afc_ind_obj.serv_resp_format = serv_resp_format;

    return ucfg_reg_send_afc_resp_rx_ind(ic->ic_pdev_obj, &afc_ind_obj);
}

/** regdmn_get_power_from_afc_list() - For a given frequency, fetch the EIRP
 *  power and PSD from the AFC channel list
 *  @afc_chan_list: Pointer to afc_chan_list.
 *  @freq - Channel frequency in MHz.
 *  @reg_eirp: EIRP power
 *  @reg_psd - PSD.
 */
static void regdmn_get_power_from_afc_list(struct regulatory_channel *afc_chan_list,
                                           qdf_freq_t freq,uint16_t *reg_eirp,
                                           uint16_t *reg_psd)
{
    enum channel_enum chan_enum;

    *reg_eirp = 0;

    for (chan_enum = 0; chan_enum < NUM_6GHZ_CHANNELS; chan_enum++) {
         if (afc_chan_list[chan_enum].center_freq == freq) {
             *reg_eirp = afc_chan_list[chan_enum].tx_power;
             *reg_psd = afc_chan_list[chan_enum].psd_eirp;
             return;
         }
    }
}
#else
static void regdmn_get_power_from_afc_list(struct regulatory_channel *afc_chan_list,
                                           qdf_freq_t freq,uint16_t *reg_eirp,
                                           uint16_t *reg_psd)
{
     *reg_eirp = 0;
     *reg_psd = 0;
}
#endif

void ieee80211_send_tpc_power_cmd(struct ieee80211vap *vap)
{
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_reg_tx_ops *tx_ops;

    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);
    tx_ops = wlan_reg_get_tx_ops(psoc);

    if (!tx_ops || !tx_ops->set_tpc_power) {
        qdf_err("No regulatory tx_ops");
        return;
    }

    if (ieee80211_fill_reg_tpc_obj(vap) != QDF_STATUS_SUCCESS) {
        qdf_err(" Filling regulatory tpc object failed");
        return;
    }
    tx_ops->set_tpc_power(psoc, vap->vdev_obj->vdev_objmgr.vdev_id,
                          &vap->vdev_mlme->reg_tpc_obj);
}

/**
 * psd_array_idx_from_bw() - Given the BW convert into the index that needs
 * to be referenced in the input PSD array.
 * @bw: Input bandwidth in MHz.
 *
 * Return: Index of the array that needs to be indexed in the PSD array.
 */
static int8_t psd_array_idx_from_bw(uint16_t bw)
{
    switch(bw) {
    case BW_20_MHZ  : return 0;
    case BW_40_MHZ  : return 1;
    case BW_80_MHZ  : return 2;
    case BW_160_MHZ : return 3;
#ifdef WLAN_FEATURE_11BE
    case BW_320_MHZ : return 4;
#endif
    }

    return -1;
}

/**
 * sub_chan_idx_from_freq() - Given a frequency, and sub-channel frequency array
 * output the index in which the frequency was found.
 * @freq: Input frequency.
 * @subchannels: Sub-channel frequency array.
 * @nchans: Number of sub-channels.
 *
 * Return: Index of the array in which freq was found.
 */
static int8_t
sub_chan_idx_from_freq(qdf_freq_t freq, qdf_freq_t *subchannels, uint8_t nchans)
{
    uint8_t i;

    for (i = 0; i < nchans; i++) {
        if (freq == subchannels[i])
            return i;
    }

    return -1;
}

/**
 * fill_in_psd_val_in_out_psd_array() - Fill the input PSD value for a given
 * bandwidth in the array indices of the output PSD array. This output PSD array
 * is the buffer that contains the PSD values in the WMI TPC command when both
 * PSD and EIRP are preferred for SP and Client SP power types.
 * @start_freq: Start frequency.
 * @end_freq: End frequency.
 * @subchannels: Sub-channel array.
 * @nchans: Number of sub-channels.
 * @in_psd_val_for_bw: Input PSD value for a given bandwidth.
 * @out_psd: Output PSD array.
 *
 * Return: Void.
 */
static void
fill_in_psd_val_in_out_psd_array(qdf_freq_t start_freq, qdf_freq_t end_freq,
                                 qdf_freq_t *subchannels, uint8_t nchans,
                                 int16_t in_psd_val_for_bw, int16_t *out_psd)
{
    uint8_t j, k;
    uint8_t start_idx;

    start_idx = sub_chan_idx_from_freq(start_freq, subchannels, nchans);
    if (start_idx == -1) {
        qdf_err("Invalid start frequency %u and not found in sub-channel array", start_freq);
        return;
    }

    for (j = start_idx, k = 0; start_freq + (k * BW_20_MHZ) <= end_freq; j++, k++) {
        out_psd[j] = in_psd_val_for_bw;
    }
}

/**
 * fill_psd_obj() - Given a PSD array for each bandwidth, convert into the
 * PSD array format in an expanded view with respect to the channel bonding.
 *
 * @ic: Pointer to the ic.
 * @chan: Pointer to the channel.
 * @in_psd: PSD values arranged in the order of 20MHz, 40MHz, 80MHz, 160MHz and
 * 320MHz.
 * @n_psds: Number of PSD values in the array in_psd.
 * @out_psd: in_psd array converted to expanded view with respect to primary
 * channel's channel bonding for each bandwidth.
 *
 * For example,
 * Convert the in_psd array of PSDs like below,
 * ---------------------------------
 * | PSD | PSD | PSD | PSD  | PSD  |
 * | 20M | 40M | 80M | 160M | 320M |
 * ---------------------------------
 * and assuming primary 20MHz is the left most sub-channel in the array then the
 * out_psd is filled as follows,
 * -------------------------------------------------------------------------------------------------------------
 * |20MHz|40MHz|80MHz|80MHz|160MHz|160MHz|160MHz|160MHz|320MHz|320MHz|320MHz|320MHz|320MHz|320MHz|320MHz|320MHz|
 * |Pri  |     |     |     |      |      |      |      |      |      |      |      |      |      |      |      |
 * -------------------------------------------------------------------------------------------------------------
 *
 * Return: void.
 */
static void
fill_psd_obj(struct ieee80211com *ic, struct ieee80211_ath_channel *chan,
             int16_t *in_psd, uint8_t *n_psds, int16_t *out_psd)
{
    uint8_t i;
    uint8_t nchans;
    uint16_t bw, max_bw = ieee80211_get_chan_width(chan);
    qdf_freq_t subchannels[IEE80211_MAX_20M_SUB_CH];
    qdf_freq_t cfreq = ieee80211_get_chan_centre_freq(ic, chan);
    enum ieee80211_phymode mode = ieee80211_chan2mode(chan);
    enum phy_ch_width ch_width;
    uint8_t psd_idx_for_bw;

    ch_width = ieee80211_get_chan_width_from_phymode(mode);
    nchans = ieee80211_get_subchannels_with_width_centerfreq(cfreq, ch_width,
                                                             subchannels);

    for (i = 0, bw = max_bw; bw >= BW_20_MHZ; bw /= 2) {
        const struct bonded_channel_freq *range;
        enum phy_ch_width cur_ch_width = wlan_reg_find_chwidth_from_bw(bw);
        qdf_freq_t start_freq, end_freq;

        psd_idx_for_bw = psd_array_idx_from_bw(bw);
        if (psd_idx_for_bw == -1) {
             qdf_err("Invalid bandwidth %u", bw);
             return;
        }

        if (bw != BW_20_MHZ) {
            range = wlan_reg_get_bonded_chan_entry(chan->ic_freq, cur_ch_width, cfreq);
            if (!range) {
                qdf_err("Invalid bonded chan entry for %u and ch_width %u", chan->ic_freq, cur_ch_width);
                return;
            }

            start_freq = range->start_freq;
            end_freq = range->end_freq;
        } else {
            start_freq = end_freq = chan->ic_freq;
        }

        fill_in_psd_val_in_out_psd_array(start_freq, end_freq, subchannels, nchans,
                                         in_psd[psd_idx_for_bw], out_psd);
    }
}

static void ieee80211_reg_get_sp_psd_for_unpunc_bw(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq, qdf_freq_t center_freq_for_320, uint16_t bw, int16_t *psd)
{
    QDF_STATUS status;

    status = reg_get_6g_min_psd(pdev, freq, center_freq_for_320, NO_PUNCTURE_BITMASK, bw, psd);

    if(QDF_IS_STATUS_ERROR(status)) {
        *psd = -640;
    }
}

static void
ieee80211_fill_reg_sp_psd_tpc_obj(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, int16_t *psd, uint8_t *n_psds)
{
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    uint8_t i;
    uint16_t bw, max_bw = ieee80211_get_chan_width(chan);
    enum ieee80211_phymode phymode = ieee80211_chan2mode(chan);
    const struct bonded_channel_freq *bonded_chan_ptr = NULL;

    for (i = 0, bw = BW_20_MHZ; bw <= max_bw; i++, bw *= 2) {
        int16_t oobe_psd;
        QDF_STATUS status;
        uint16_t intersected_punc_pattern =
                             ieee80211_get_punc_pattern_for_bw(chan, phymode, bw);
        qdf_freq_t center_320;
        qdf_freq_t chan_center = 0;

#ifdef WLAN_FEATURE_11BE
        center_320 = (bw == BW_320_MHZ) ? chan->ic_vhtop_freq_seg2 : 0;
#else
        center_320 = 0;
#endif
        bonded_chan_ptr = wlan_reg_get_bonded_chan_entry(chan->ic_freq, wlan_reg_find_chwidth_from_bw(bw), center_320);
        if (bw == BW_20_MHZ) {
            chan_center = chan->ic_freq;
        } else if (bonded_chan_ptr) {
            chan_center = (bonded_chan_ptr->start_freq + bonded_chan_ptr->end_freq) / 2;
        }

        status = reg_get_6g_min_psd(pdev, chan->ic_freq, chan_center,
                                    intersected_punc_pattern, bw, &oobe_psd);
        if(QDF_IS_STATUS_ERROR(status))
            oobe_psd = -640;

        psd[i] = oobe_psd;
        psd[i] /= 10;
    }

    *n_psds = i;
}

/**
 * ieee80211_find_start_index_for_intersected_bw() - Find TPE index for AP/STA
 * operating BW
 * @bonded_start_freq_ap: Bonded pair start freq of AP's BW
 * @bonded_start_freq_sta: Bonded pair start freq of STA's BW
 *
 * A negative return value is an error
 *
 * Return: TPE index to read
 */
static inline int
ieee80211_find_start_index_for_intersected_bw(qdf_freq_t bonded_start_freq_ap,
                                              qdf_freq_t bonded_start_freq_sta)
{
    return (bonded_start_freq_sta - bonded_start_freq_ap) / BW_20_MHZ;
}

int8_t
ieee80211_get_start_index_for_sta_bw(uint16_t ap_width,
                                     uint16_t sta_width,
                                     qdf_freq_t primary_freq,
                                     qdf_freq_t cen320_freq)
{
    const struct bonded_channel_freq *bonded_chan_ptr_ap = NULL;
    const struct bonded_channel_freq *bonded_chan_ptr_sta = NULL;
    enum phy_ch_width sta_phy_chwidth, ap_phy_chwidth;

    /* 20 MHz BW does not have any bonding. Both AP and STA index is 0 */
    ap_phy_chwidth = wlan_reg_find_chwidth_from_bw(ap_width);
    if (ap_phy_chwidth == CH_WIDTH_20MHZ)
        return 0;

    sta_phy_chwidth = wlan_reg_find_chwidth_from_bw(sta_width);

    bonded_chan_ptr_ap = wlan_reg_get_bonded_chan_entry(primary_freq,
                                                        ap_phy_chwidth,
                                                        cen320_freq);
    if (!bonded_chan_ptr_ap)
        return -EINVAL;

    /* AP is operating on a BW greater than 20 MHz whereas STA is operating on
     * 20 MHz.
     * For 20MHz, bonded channel entry returns NULL, therefore, use
     * the primary channel as the start frequency.
     */
    if (sta_phy_chwidth == CH_WIDTH_20MHZ)
        return ieee80211_find_start_index_for_intersected_bw(bonded_chan_ptr_ap->start_freq,
                                                           primary_freq);

    bonded_chan_ptr_sta = wlan_reg_get_bonded_chan_entry(primary_freq,
                                                         sta_phy_chwidth,
                                                         cen320_freq);
    if (!bonded_chan_ptr_sta)
        return -EINVAL;

    return ieee80211_find_start_index_for_intersected_bw(bonded_chan_ptr_ap->start_freq,
                                                         bonded_chan_ptr_sta->start_freq);
}

/**
 * ieee80211_reg_get_psd_for_punc_chans() - Get psd power values for punctured
 * channels
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @vap: Pointer to struct ieee80211vap
 * @ap_reg_psd: psd power to be filled
 *
 * Return: QDF_STATUS
 */
#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT)
static QDF_STATUS
ieee80211_reg_get_psd_for_punc_chans(struct wlan_objmgr_pdev *pdev,
                                     struct ieee80211vap *vap,
                                     uint16_t *ap_reg_psd)
{
    QDF_STATUS status;
    struct ieee80211_ath_channel *bss_chan = vap->iv_bsschan;
    qdf_freq_t center_freq = ieee80211_get_chan_centre_freq(vap->iv_ic, bss_chan);
    enum ieee80211_phymode phymode = ieee80211_chan2mode(bss_chan);
    uint16_t bw = ieee80211_get_chan_width_from_mode(phymode);

    if (bw < CHWIDTH_80)
        return QDF_STATUS_E_FAILURE;

    status = reg_get_6g_min_psd(pdev, bss_chan->ic_freq,
                                center_freq,
                                bss_chan->op_puncture_bitmap,
                                bw, ap_reg_psd);

    if (status == QDF_STATUS_SUCCESS)
        *ap_reg_psd = *ap_reg_psd / 10;

    return status;
}
#else
static inline QDF_STATUS
ieee80211_reg_get_psd_for_punc_chans(struct wlan_objmgr_pdev *pdev,
                                     struct ieee80211vap *vap,
                                     uint16_t *ap_reg_psd)
{
    return QDF_STATUS_E_FAILURE;
}
#endif

/* ieee80211_determinte_6ghz_client_type() - Set the current regulatory client
 * type to "default" when connected to SP/VLP root. Set it to "subordinate" when
 * connected to LP root.
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @ap_power_type_6g - 6g ap power type.
 */
static void ieee80211_determinte_6ghz_client_type(struct wlan_objmgr_pdev *pdev,
                                                  enum reg_6g_ap_type ap_power_type_6g)
{
    enum reg_6g_client_type client_type = REG_MAX_CLIENT_TYPE;

    wlan_reg_get_cur_6g_client_type(pdev, &client_type);
    if (client_type == REG_SUBORDINATE_CLIENT &&
        (ap_power_type_6g == REG_STANDARD_POWER_AP || ap_power_type_6g == REG_VERY_LOW_POWER_AP)) {
        wlan_reg_set_cur_6ghz_client_type(pdev, REG_DEFAULT_CLIENT);
    } else if (client_type == REG_DEFAULT_CLIENT && ap_power_type_6g == REG_INDOOR_AP) {
        /* When the connected root AP switches its power mode from SP
         * to LP on the fly, the client type should be reset to "subordinate"
         */
        wlan_reg_set_cur_6ghz_client_type(pdev, REG_SUBORDINATE_CLIENT);
    }
}

#define TWO_POW(n) (0x1 << (n))
/* ieee80211_reg_get_subchan_punc_info() - Find the subchannel indexes which
 * have been punctured for the Full BW.
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @ni_chan: Pointer to struct ieee80211_ath_channel
 * @ni_chanwidth: Channel bandwidth
 * @is_subchan_punctured: Output array to indicate which subchannels are punctured.
 *
 * Return: None
 */
static void
ieee80211_reg_get_subchan_punc_info(struct wlan_objmgr_pdev *pdev,
                                    struct ieee80211_ath_channel *ni_chan,
                                    uint16_t ni_chanwidth,
                                    bool *is_subchan_punctured)
{
    enum phy_ch_width ch_width = wlan_reg_find_chwidth_from_bw(ni_chanwidth);
    uint8_t num_pwr_levels = ieee80211_get_num_pwr_levels(pdev, 0, ieee80211_get_cwm_width(ch_width), true);
    int i;
    uint16_t puncture_bitmap = ieee80211_get_chan_punc_bitmap(ni_chan);

    for (i = 0; i < num_pwr_levels; i++) {
        if (puncture_bitmap & BIT(i)) {
            is_subchan_punctured[i] = true;
        }
    }
}

/* ieee80211_get_min_psd_for_bw_index() - Find the minimum PSD value among all
 * subchans for a particular BW. Also return the number of punctured subchannel
 * for the BW.
 * @psd_start_index: Start index for the input BW
 * @bw_index: Iterator indicating the current BW.
 * @is_subchan_punctured: Input array containing full BW puncture information.
 * @min_psd: Output PSD power, which is the minimum amoung all subchans excluding
 * punctured subchans
 *
 * Return: Number of Puntured subchannels in the target BW.
 */
static uint8_t
ieee80211_get_min_psd_for_bw_index(struct ieee80211_node *ni,
                                   uint8_t psd_start_index,
                                   uint8_t bw_index,
                                   bool *is_subchan_punctured,
                                   int8_t *min_psd)
{
    int i;
    uint8_t psd_end_index = psd_start_index + TWO_POW(bw_index);
    uint8_t punc_subchan_count = 0;

    for (i = psd_start_index; i < psd_end_index; i++) {
        if (is_subchan_punctured[i]) {
            punc_subchan_count++;
            continue;
        }

        if (ni->ni_eirppsd_limit[i] < *min_psd)
            *min_psd = ni->ni_eirppsd_limit[i];
    }

    return punc_subchan_count;
}

/* ieee80211_reg_get_eirp_from_tpe_limited_by_psd() - Find the EIRP power values
 * obtained from the TPE IE from beacon and limit the TPE EIRP by the EIRP
 * calculated from PSD IE in TPE.
 * @ni: Pointer to structure ieee80211_node
 * @tpe_eirp_limit: Output array to store EIPR values.
 *
 * Return: None
 *
 * Consider the following example for 80 MHz. Here, (P) indicated primary channel
 * (X) indicated channel which is punctured.
 *
 * ------------------------------
 * |Chan#| 1 | 5(X) | 9(P) | 13 |
 * |----------------------------|
 * |PSD  | 5 |  -6  | 4    | 18 |
 * ------------------------------
 *
 * Min PSD for each BW is calculated by finding the mimimum PSD within the BW
 * based on primary channel and channel bonding.
 *
 * Thus,
 * Minimum_PSD for 20 BW:
 *   Subchannels in 20 BW: #9(P)
 *   Min_PSD             : 4
 *
 * Minimum_PSD for 40 BW:
 *   Subchannels in 40 BW: #9(P) and #13
 *   Min_PSD             : MIN (4, 18) = 4
 *
 * Minimum_PSD for 80 BW:
 *   Subchannels in 40 BW: #1, #5(X), #9(P) and #13
 *   Min_PSD             : MIN (5, 4, 18) = 4 (Ignoring Punctured channel PSD)
 */
static void
ieee80211_reg_get_eirp_from_tpe_limited_by_psd(struct ieee80211_node *ni,
                                               int8_t *tpe_eirp_limit)
{
    struct chan_params chan_p = {0};
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_ath_channel ni_chan;
    int i;
    uint16_t ni_chanwidth;
    uint16_t bw;
    uint8_t psd_start_index;
    int8_t min_psd = CHAN_MAX_PSD_POWER;
    int16_t eirp_from_min_psd;
    bool is_subchan_punctured[MAX_PSD_VALS] = {false};
    uint8_t punc_subchan_count = 0;
    bool is_chan_punctured;

    chan_p.freq = ni->ni_ch_param.ni_freq;
    chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
    set_chan_params_puncture_bitmap(&chan_p, ni->ni_ch_param.puncture_bitmap);
    ni_chan = ieee80211_find_dot11_channel(ni->ni_ic,
                                           &chan_p,
                                           ni->ni_ch_param.ni_bss_phymode);

    ni_chanwidth = ieee80211_get_chan_width(&ni_chan);
#ifdef WLAN_FEATURE_11BE
    is_chan_punctured = IEEE80211_IS_CHAN_PUNCTURED(&ni_chan);
#else
    is_chan_punctured = false;
#endif

    if (is_chan_punctured) {
        ieee80211_reg_get_subchan_punc_info(ic->ic_pdev_obj,
                                            &ni_chan,
                                            ni_chanwidth,
                                            is_subchan_punctured);
    }

    for (i = 0, bw = BW_20_MHZ; bw <= ni_chanwidth; i++, bw *= 2) {
        psd_start_index = ieee80211_get_start_index_for_sta_bw(ni_chanwidth,
                                                               bw,
                                                               ni_chan.ic_freq,
                                                               ni_chan.ic_vhtop_freq_seg2);
        if (bw == BW_20_MHZ) {
            /* For 20 MHz BW */
            min_psd = ni->ni_eirppsd_limit[psd_start_index];
        } else {
            /* For Higher BW */
            punc_subchan_count = ieee80211_get_min_psd_for_bw_index(ni,
                                                                    psd_start_index,
                                                                    i,
                                                                    is_subchan_punctured,
                                                                    &min_psd);
        }
        wlan_reg_psd_2_eirp(ic->ic_pdev_obj,
                            (int16_t)min_psd,
                            bw - punc_subchan_count * BW_20_MHZ,
                            &eirp_from_min_psd);

        QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_DEBUG,
                  "BW %d min_psd %d eirp_from_min_psd %d "
                  "punc_subchan_count %d ni_eirp[BW] %d", bw, min_psd, eirp_from_min_psd,
                  punc_subchan_count, ni->ni_eirp_limit[i]);
        tpe_eirp_limit[i] = (int8_t) qdf_min(eirp_from_min_psd, (int16_t)ni->ni_eirp_limit[i]);
    }
}

/* ieee80211_reg_assign_root_tpe_pwr() - Fill the root_tpe_pwr array
 * @ic - pointer to ic
 * @root_tpe_pwr - pointer to root_tpe_pwr
 * @limit_by_tpe_psd - flag to check whether eirp has to be limited by tpe PSD.
 */
static void ieee80211_reg_assign_root_tpe_pwr(struct ieee80211com *ic,
                                              int8_t *root_tpe_pwr,
                                              bool limit_by_tpe_psd)
{
   /*
    * TPE power values from Root AP are parsed as part of 'ieee80211_setup_node'
    * When this API is called for a STA vap after 'ieee80211_setup_node', then
    * valid TPE power values are already stored in ni. Therefore use the stored
    * power values as the limit.
    * Before connection, then "ni_eirp_limit" should have MAX_TX_POWER
    * already. If there is no STA vap then this device it not connected to
    * a rootap and therefore there is no power limit, thus use max power
    * limit.
    */
   STA_VAP_DOWNUP_LOCK(ic);
   if (ic->ic_sta_vap) {
       struct ieee80211_node *ni;
       int8_t tpe_eirp_limit[MAX_EIRP_VALS];

       ni = ic->ic_sta_vap->iv_bss;
       if (ni == NULL) {
           qdf_debug("ni is NULL");
           qdf_mem_set(root_tpe_pwr, MAX_EIRP_VALS * sizeof(uint8_t), MAX_TX_POWER);
       } else {
           if (limit_by_tpe_psd) {
               ieee80211_reg_get_eirp_from_tpe_limited_by_psd(ni, tpe_eirp_limit);
               qdf_mem_copy(root_tpe_pwr, tpe_eirp_limit, sizeof(tpe_eirp_limit));
           } else {
               qdf_mem_copy(root_tpe_pwr, ni->ni_eirp_limit, sizeof(ni->ni_eirp_limit));
           }
       }
   } else {
       qdf_mem_set(root_tpe_pwr, MAX_EIRP_VALS * sizeof(uint8_t), MAX_TX_POWER);
   }
   STA_VAP_DOWNUP_UNLOCK(ic);
}

/* ieee80211_fill_eirp_from_psd() - Fill EIRP values from psd
 * @ic - pointer to ic.
 * @psd - Power spectral density
 * @eirp_arr - Pointer to eirp_val
 * @num_eirp_val - Number of EIRP values
 * @max_reg_eirp - Reguatory max EIRP value
 */
static void ieee80211_fill_eirp_from_psd(struct ieee80211com *ic,
                                         int16_t psd,
                                         int8_t *eirp_arr,
                                         uint8_t num_eirp_val,
                                         uint16_t max_reg_eirp)
{
    uint8_t i;
    uint16_t bw = BW_20_MHZ;

    for (i = 0; i < num_eirp_val; i++) {
         int16_t eirp;

         if (wlan_reg_psd_2_eirp(ic->ic_pdev_obj, psd, bw, &eirp)
              != QDF_STATUS_SUCCESS) {
             qdf_err("Could not compute EIRP for input PSD");
             break;
         }

         eirp_arr[i] = QDF_MIN(eirp, max_reg_eirp);
         bw *= 2;
    }

}

/* ieee80211_get_max_eirp() - Get max EIRP power value from the given AP and STA
 * EIRP values arrays
 * @pdev - Pointer to pdev
 * @num_pwr_levels - Number of power levels
 * @ap_max_eirp_arr - Max EIRP AP array
 * @sta_max_eirp_arr - Max EIRP STA array
 * @root_tpe_pwr - TPE power of root
 * @apvap_count - AP VAP count
 * @ap_power_type_6g - 6 GHz AP power type
 */
static int8_t ieee80211_get_max_eirp(struct wlan_objmgr_pdev *pdev,
                                      uint8_t num_pwr_levels,
                                      uint8_t *ap_max_eirp_arr,
                                      uint8_t *sta_max_eirp_arr,
                                      uint8_t *root_tpe_pwr,
                                      uint8_t apvap_count,
                                      enum reg_6g_ap_type ap_power_type_6g)
{
    uint8_t count;
    int8_t max_eirp_power = MIN_TX_POWER;

    for (count = 0; count < num_pwr_levels; count++) {
         int8_t sta_tx_pwr;
         int8_t tx_power, max_of_ap_sta_tx_pwr, ap_tx_pwr;

         ap_tx_pwr = apvap_count ? ap_max_eirp_arr[count] : 0;
         sta_tx_pwr = sta_max_eirp_arr[count];
         /* Generally, 6GHz client power is less than 6GHz AP power.
          * In repeater, we have access tp both client and AP power.
          * Therefore, take advantage of the maximum of AP and client power.
          */
         max_of_ap_sta_tx_pwr = (int8_t)QDF_MAX(ap_tx_pwr, sta_tx_pwr);
         if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
             wlan_reg_is_afc_power_event_received(pdev))
             tx_power = max_of_ap_sta_tx_pwr;
         else
             tx_power = (int8_t)QDF_MIN(root_tpe_pwr[count], max_of_ap_sta_tx_pwr);

         if (tx_power > max_eirp_power)
             max_eirp_power = tx_power;
    }

    return max_eirp_power;
}

/* ieee80211_compute_max_eirp_for_tpc_obj - Compute max EIRP power value to be
 * filled in TPC object. Max EIRP array is first found by taking the minimum
 * of AFC EIRP and Regulatory EIRP. Maximum EIRP value from this array is returned
 * as the max EIRP value.
 * @vap - pointer to vap.
 * @ap_power_type_6g - 6g ap power type.
 * @start_freq - center frequency of the first sub-channel in the current
 * operating channel.
 * @max_eirp_power - Max EIRP power to be computed
 * @in_puncture_bitmap - Input puncture bitmap
 * @num_pwr_levels - Number of power levels
 */
static void ieee80211_compute_max_eirp_for_tpc_obj(struct ieee80211vap *vap,
                                                   enum reg_6g_ap_type ap_power_type_6g,
                                                   qdf_freq_t start_freq,
                                                   int8_t *max_eirp_power,
                                                   uint16_t in_puncture_bitmap,
                                                   uint8_t num_pwr_levels)
{
    uint8_t apvap_count = 0;
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    enum reg_6g_client_type client_type = REG_MAX_CLIENT_TYPE;
    int8_t ap_max_eirp_arr[MAX_EIRP_VALS], sta_max_eirp_arr[MAX_EIRP_VALS], root_tpe_pwr[MAX_EIRP_VALS];

    if (num_pwr_levels > MAX_EIRP_VALS) {
        qdf_err("num_pwr_levels should not be greater than MAX_EIRP_VALS");
        return;
    }

    ieee80211_reg_assign_root_tpe_pwr(ic, root_tpe_pwr, false);

    ieee80211_determinte_6ghz_client_type(ic->ic_pdev_obj, ap_power_type_6g);
    wlan_reg_get_cur_6g_client_type(ic->ic_pdev_obj, &client_type);

    apvap_count = ieee80211_get_apvaps_count(ic);
    ieee80211_reg_get_eirp_arr_for_6g(vap, ap_power_type_6g, false,
                                      client_type, ap_max_eirp_arr,
                                      ic->ic_curchan, false);
    /* In case of a Non-AFC capable SP client, calculate the EIRP values from regulatory client PSD */
    if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
        !wlan_reg_is_afc_power_event_received(pdev)) {
        bool is_psd = wlan_reg_is_6g_psd_power(ic->ic_pdev_obj);
        uint16_t max_reg_eirp = 0, max_reg_client_psd = 0;

        wlan_reg_get_client_power_for_connecting_ap(ic->ic_pdev_obj, ap_power_type_6g,
                                                    start_freq, is_psd,
                                                    &max_reg_eirp,
                                                    &max_reg_client_psd);
        ieee80211_fill_eirp_from_psd(ic, max_reg_client_psd, sta_max_eirp_arr, num_pwr_levels, max_reg_eirp);
    } else {
        ieee80211_reg_get_eirp_arr_for_6g(vap, ap_power_type_6g, true,
                                          client_type, sta_max_eirp_arr,
                                          ic->ic_curchan, false);
    }

    *max_eirp_power = ieee80211_get_max_eirp(pdev, num_pwr_levels, ap_max_eirp_arr,
                                             sta_max_eirp_arr, root_tpe_pwr, apvap_count,
                                             ap_power_type_6g);
}

#define REG_SP_CLIENT_TYPE 3
#define INVALID_PSD_VAL ((int16_t)(-127))

/* ieee80211_fill_psd_power_for_tpc_obj() - Fill the tpc object with PSD power
 * values
 * @vap - pointer to vap.
 * @reg_tpc_objp - pointer to reg_tpc_obj.
 * @ap_power_type_6g - 6g ap power type.
 * @start_freq - center frequency of the first sub-channel in the current
 * operating channel.
 * @is_psd_power - Flag to indicate if PSD is supported by the radio.
 */
static QDF_STATUS
 ieee80211_fill_psd_power_for_tpc_obj(struct ieee80211vap *vap,
                                      struct reg_tpc_power_info *reg_tpc_objp,
                                      enum reg_6g_ap_type ap_power_type_6g,
                                      qdf_freq_t start_freq,
                                      bool is_psd_power)
{
   uint8_t count;
   struct ieee80211com *ic = vap->iv_ic;
   int8_t tpe_tx_pwr;
   enum QDF_OPMODE opmode;
   int8_t max_tx_power[MAX_PSD_VALS];
   uint16_t ap_reg_tx_pwr = 0, sta_reg_tx_pwr = 0, ap_reg_psd = 0, sta_reg_psd = 0;
   struct wlan_objmgr_pdev *pdev = vap->iv_ic->ic_pdev_obj;
   qdf_freq_t center_freq;
   uint8_t num_pwr_levels;
   struct ieee80211_node *ni = vap->iv_bss;
#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT)
   int16_t reg_punc_psd = INVALID_PSD_VAL;
   QDF_STATUS status;
#endif
#ifdef CONFIG_AFC_SUPPORT
   enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif
   uint16_t in_puncture_bitmap;
   int8_t max_eirp_power = 0, num_eirp_power_levels;
   qdf_freq_t freq;
   uint16_t chwidth;

   num_pwr_levels = reg_tpc_objp->num_pwr_levels;
   opmode = wlan_vdev_mlme_get_opmode(vap->vdev_obj);

#ifdef WLAN_FEATURE_11BE
   in_puncture_bitmap = ni->ni_ch_param.puncture_bitmap;
#else
   in_puncture_bitmap = NO_PUNCTURE_BITMASK;
#endif

#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT)
   if (in_puncture_bitmap && ap_power_type_6g == REG_STANDARD_POWER_AP) {
       status = ieee80211_reg_get_psd_for_punc_chans(pdev, vap, &reg_punc_psd);
       if (status != QDF_STATUS_SUCCESS)
           reg_punc_psd = INVALID_PSD_VAL;
   }
#endif

   for (count = 0; count < num_pwr_levels; count++) {
        if (opmode == QDF_STA_MODE) {
            uint8_t apvap_count;

            ieee80211_determinte_6ghz_client_type(pdev, ap_power_type_6g);
            wlan_reg_get_client_power_for_connecting_ap(pdev, ap_power_type_6g,
                                                        start_freq + BW_20_MHZ *
                                                        count, is_psd_power,
                                                        &sta_reg_tx_pwr, &sta_reg_psd);
            /** If the opmode is STA, then take the minimum of the power
             *  received in the TPE, and the regulatory max power.
             */
            tpe_tx_pwr = ni->ni_eirppsd_limit[count];
            /* If there is an AP VAP the power values should be
             * MAX(STAPower, AP Power)
             */
            /* When the radio is AFC capable disregrad the TPE limit */
            apvap_count = ieee80211_get_apvaps_count(ic);

            if (apvap_count && (ap_power_type_6g == REG_STANDARD_POWER_AP) &&
                wlan_reg_is_afc_power_event_received(pdev)) {
                struct regulatory_channel *afc_chan_list;

                afc_chan_list = qdf_mem_malloc(NUM_6GHZ_CHANNELS * sizeof(*afc_chan_list));

                if (!afc_chan_list)
                    return QDF_STATUS_E_FAILURE;

                wlan_reg_get_6g_afc_chan_list(pdev, afc_chan_list);
                /* Fetch power from the AFC list */
                regdmn_get_power_from_afc_list(afc_chan_list,
                start_freq + BW_20_MHZ * count,
                &ap_reg_tx_pwr, &ap_reg_psd);
#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT)
                if (in_puncture_bitmap && reg_punc_psd != INVALID_PSD_VAL) {
                    ap_reg_psd = reg_punc_psd;
                }
#endif
                max_tx_power[count] = QDF_MAX((int8_t)sta_reg_psd, (int8_t)ap_reg_psd);
                qdf_mem_free(afc_chan_list);
             } else {
                if (wlan_reg_is_afc_power_event_received(pdev)) {
                    if (ap_power_type_6g == REG_STANDARD_POWER_AP) {
                        struct regulatory_channel *afc_chan_list;

                        afc_chan_list = qdf_mem_malloc(NUM_6GHZ_CHANNELS * sizeof(*afc_chan_list));

                        if (!afc_chan_list)
                            return QDF_STATUS_E_FAILURE;

                        wlan_reg_get_6g_afc_chan_list(pdev, afc_chan_list);
                        regdmn_get_power_from_afc_list(afc_chan_list, start_freq + BW_20_MHZ * count,
                                                       &sta_reg_tx_pwr, &sta_reg_psd);
#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT)
                        if (in_puncture_bitmap && reg_punc_psd != INVALID_PSD_VAL) {
                            sta_reg_psd = reg_punc_psd;
                        }
#endif
                        max_tx_power[count] = (int8_t)sta_reg_psd;
                        qdf_mem_free(afc_chan_list);
                     } else {
                        max_tx_power[count] = QDF_MAX((int8_t)sta_reg_psd, (int8_t)ap_reg_psd);
                     }

                } else {
                    max_tx_power[count] = QDF_MIN((int8_t)sta_reg_psd, tpe_tx_pwr);
                    if (ap_power_type_6g == REG_STANDARD_POWER_AP)
                        reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;
                }

            }

        } else {
            struct ieee80211vap *stavap = NULL;

            if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
                wlan_reg_is_afc_power_event_received(pdev)) {
                struct regulatory_channel *afc_chan_list;

                afc_chan_list = qdf_mem_malloc(NUM_6GHZ_CHANNELS * sizeof(*afc_chan_list));

                if (!afc_chan_list)
                    return QDF_STATUS_E_FAILURE;

                wlan_reg_get_6g_afc_chan_list(pdev, afc_chan_list);
                regdmn_get_power_from_afc_list(afc_chan_list, start_freq + BW_20_MHZ * count,
                                               &ap_reg_tx_pwr, &ap_reg_psd);
#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT)
                if (in_puncture_bitmap && reg_punc_psd != INVALID_PSD_VAL) {
                    ap_reg_psd = reg_punc_psd;
                }
#endif
                qdf_mem_free(afc_chan_list);
            } else {
                wlan_reg_get_6g_chan_ap_power(pdev, start_freq + BW_20_MHZ *
                                              count, &is_psd_power, &ap_reg_tx_pwr,
                                              &ap_reg_psd);

            }


            /* The 'limit in the TPE from the root AP' has to be found out by
             * traverseing through the VAP list and fiding the first STA vap
             */
            STA_VAP_DOWNUP_LOCK(ic);
            stavap = ic->ic_sta_vap;
            if (stavap) {
                int16_t reg_psd;

                wlan_reg_get_client_power_for_connecting_ap(pdev, ap_power_type_6g,
                                                            start_freq + BW_20_MHZ *
                                                            count, is_psd_power,
                                                            &sta_reg_tx_pwr,
                                                            &sta_reg_psd);
                if(wlan_reg_is_afc_power_event_received(pdev)) {
                   max_tx_power[count] = QDF_MAX(sta_reg_psd, ap_reg_psd);
                   STA_VAP_DOWNUP_UNLOCK(ic);
                   /* When the radio is AFC capable disregard the TPE limit */
                } else {
                   reg_psd = QDF_MIN((int8_t)sta_reg_psd, (int8_t)ap_reg_psd);
                   max_tx_power[count] = QDF_MIN(reg_psd, stavap->iv_bss->ni_eirppsd_limit[count]);
                    if (ap_power_type_6g == REG_STANDARD_POWER_AP)
                        reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;
                   STA_VAP_DOWNUP_UNLOCK(ic);
                }
            } else {
                STA_VAP_DOWNUP_UNLOCK(ic);
                max_tx_power[count] = ap_reg_psd;
            }
        }

        center_freq = start_freq + (BW_20_MHZ * count);

        reg_tpc_objp->chan_power_info[count].chan_cfreq = center_freq;
        reg_tpc_objp->chan_power_info[count].tx_power = max_tx_power[count];
   }

#ifdef CONFIG_AFC_SUPPORT
   /* In case of a Non-AFC capable Outdoor SP repeater, fill the power_type as REG_SP_CLIENT_TYPE */
   wlan_reg_get_afc_dev_deploy_type(pdev, &reg_afc_deploy_type);
   if ((ieee80211_reg_is_repeater_pdev(ic) && (vap == ic->ic_sta_vap) &&
       wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq) &&
       !wlan_reg_is_afc_done(pdev, ic->ic_curchan->ic_freq) &&
       (reg_afc_deploy_type == AFC_DEPLOYMENT_OUTDOOR)))
       reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;
#endif

   freq = vap->iv_bsschan->ic_freq;
   chwidth = wlan_vdev_get_chwidth(vap->vdev_obj);
   num_eirp_power_levels = ieee80211_get_num_pwr_levels(pdev, freq, chwidth, false);
   ieee80211_compute_max_eirp_for_tpc_obj(vap, ap_power_type_6g, start_freq, &max_eirp_power,
                                          in_puncture_bitmap, num_eirp_power_levels);
   reg_tpc_objp->eirp_power = max_eirp_power;
   return QDF_STATUS_SUCCESS;
}

/**
 * ieee80211_fill_txpower() - Fill max_txpower for bandwidths in the tpc obj.
 * @chan_power_info: Pointer to chan_power_info
 * @max_tx_power: Pointer to max_tx_power
 * @num_pwr_levels: Number of power levels
 * Return - None
 */
static void ieee80211_fill_txpower(struct chan_power_info *chan_power_info,
                                   uint8_t *max_tx_power,
                                   uint8_t num_pwr_levels)
{
   uint8_t i;

   for (i = 0; i < num_pwr_levels; i++)
        chan_power_info[i].tx_power = max_tx_power[i];
}

#ifdef WLAN_FEATURE_11BE
static inline qdf_freq_t
ieee80211_get_320_center_freq(struct ieee80211_ath_channel *chan, uint16_t bw)
{
    return (bw == BW_320_MHZ) ? chan->ic_vhtop_freq_seg2 : 0;
}
#else
static qdf_freq_t
ieee80211_get_320_center_freq(struct ieee80211_ath_channel *chan, uint16_t bw)
{
    return 0;
}
#endif

/**
 * ieee80211_fill_band_centers() - Fill channel center freq. Find out center frequency
 * of all possible BW for the given primary freq and fill the tpc obj.
 * @vap: Pointer to Struct ieee80211vap.
 * @chan_power_info: Pointer to the TPC power info object.
 * @num_pwr_levels: Maximum number of bandwidths to fill.
 * @band_freq: A frequency inside the band, in MHz units
 * Return: QDF_STATUS_SUCCESS on success.
 */
static QDF_STATUS
ieee80211_fill_band_centers(struct ieee80211vap *vap,
                            struct chan_power_info *chan_power_info,
                            uint8_t num_pwr_levels,
                            qdf_freq_t band_freq)
{
    const struct bonded_channel_freq *bonded_chan_ptr = NULL;
    uint8_t i;
    uint16_t bw;
    qdf_freq_t center_320;

    for (i = 0, bw = BW_20_MHZ; i < num_pwr_levels ; i++, bw *= 2) {
        if (bw == BW_20_MHZ) {
            chan_power_info[i].chan_cfreq = band_freq;
        } else {
            center_320 = ieee80211_get_320_center_freq(vap->iv_bsschan, bw);
            bonded_chan_ptr = wlan_reg_get_bonded_chan_entry(band_freq,
                                                             wlan_reg_find_chwidth_from_bw(bw),
                                                             center_320);
            if (bonded_chan_ptr) {
                chan_power_info[i].chan_cfreq = (bonded_chan_ptr->start_freq + bonded_chan_ptr->end_freq) / 2;
            } else {
                return QDF_STATUS_E_FAILURE;
            }
        }
    }

    return QDF_STATUS_SUCCESS;
}

/* ieee80211_fill_eirp_power_for_tpc_obj() - Fill the tpc object with EIRP power
 * values
 * @vap - pointer to vap.
 * @reg_tpc_objp - pointer to reg_tpc_obj.
 * @ap_power_type_6g - 6g ap power type.
 * @primary_freq - Primary frequency in MHz
 * operating channel.
 */
static QDF_STATUS
ieee80211_fill_eirp_power_for_tpc_obj(struct ieee80211vap *vap,
                                      struct reg_tpc_power_info *reg_tpc_objp,
                                      enum reg_6g_ap_type ap_power_type_6g,
                                      qdf_freq_t primary_freq)
{
   uint8_t count;
   enum QDF_OPMODE opmode;
   uint8_t num_pwr_levels, apvap_count = 0;
   struct ieee80211com *ic = vap->iv_ic;
   struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
#ifdef CONFIG_AFC_SUPPORT
   enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif
   enum reg_6g_client_type client_type = REG_MAX_CLIENT_TYPE;
   int8_t ap_max_eirp_arr[MAX_EIRP_VALS], sta_max_eirp_arr[MAX_EIRP_VALS],
   root_tpe_pwr[MAX_EIRP_VALS];
   int8_t max_tx_power[MAX_EIRP_VALS];

   num_pwr_levels = reg_tpc_objp->num_pwr_levels;

   if (num_pwr_levels > MAX_EIRP_VALS) {
       qdf_err("num_pwr_levels should not be greater than MAX_EIRP_VALS");
       return QDF_STATUS_E_FAILURE;
   }

   ieee80211_reg_assign_root_tpe_pwr(ic, root_tpe_pwr, false);
   opmode = wlan_vdev_mlme_get_opmode(vap->vdev_obj);

   ieee80211_determinte_6ghz_client_type(ic->ic_pdev_obj, ap_power_type_6g);
   wlan_reg_get_cur_6g_client_type(ic->ic_pdev_obj, &client_type);

   apvap_count = ieee80211_get_apvaps_count(ic);
   ieee80211_reg_get_eirp_arr_for_6g(vap, ap_power_type_6g, false,
                                     client_type, ap_max_eirp_arr,
                                     ic->ic_curchan, false);
   /* In case of a Non-AFC capable SP client, calculate the EIRP values from regulatory client PSD */
   if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
       !wlan_reg_is_afc_power_event_received(pdev)) {
       bool is_psd = wlan_reg_is_6g_psd_power(ic->ic_pdev_obj);
       uint16_t max_reg_eirp = 0, max_reg_client_psd = 0;

       wlan_reg_get_client_power_for_connecting_ap(ic->ic_pdev_obj, ap_power_type_6g,
                                                   primary_freq, is_psd,
                                                   &max_reg_eirp,
                                                   &max_reg_client_psd);
       ieee80211_fill_eirp_from_psd(ic, max_reg_client_psd, sta_max_eirp_arr, num_pwr_levels, max_reg_eirp);
   } else {
       ieee80211_reg_get_eirp_arr_for_6g(vap, ap_power_type_6g, true,
                                         client_type, sta_max_eirp_arr,
                                         ic->ic_curchan, false);
  }

   for (count = 0; count < num_pwr_levels; count++) {
       int8_t sta_tx_pwr;
       int8_t tx_power, max_of_ap_sta_tx_pwr, ap_tx_pwr;

       ap_tx_pwr = apvap_count ? ap_max_eirp_arr[count] : 0;
       sta_tx_pwr = sta_max_eirp_arr[count];
       /* Generally, 6GHz client power is less than 6GHz AP power.
        * In repeater, we have access tp both client and AP power.
        * Therefore, take advantage of the maximum of AP and client power.
        */
       max_of_ap_sta_tx_pwr = QDF_MAX(ap_tx_pwr, sta_tx_pwr);
       if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
           wlan_reg_is_afc_power_event_received(pdev))
           tx_power = max_of_ap_sta_tx_pwr;
       else
           tx_power = QDF_MIN(root_tpe_pwr[count], max_of_ap_sta_tx_pwr);

       max_tx_power[count] = tx_power;
   }

   /* In case of a Non-AFC capable SP client, fill the power_type as REG_SP_CLIENT_TYPE */
   if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
       !wlan_reg_is_afc_power_event_received(pdev))
       reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;

#ifdef CONFIG_AFC_SUPPORT
   /* In case of a Non-AFC capable Outdoor SP repeater, fill the power_type as REG_SP_CLIENT_TYPE */
   wlan_reg_get_afc_dev_deploy_type(pdev, &reg_afc_deploy_type);
   if ((ieee80211_reg_is_repeater_pdev(ic) && (vap == ic->ic_sta_vap) &&
       wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq) &&
       !wlan_reg_is_afc_done(pdev, ic->ic_curchan->ic_freq) &&
       (reg_afc_deploy_type == AFC_DEPLOYMENT_OUTDOOR)))
       reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;
#endif

   if (ieee80211_fill_band_centers(vap,
                                   reg_tpc_objp->chan_power_info,
                                   num_pwr_levels,
                                   primary_freq) != QDF_STATUS_SUCCESS) {
      qdf_err("Selecting band centers failed");
      return QDF_STATUS_E_FAILURE;
   }

   ieee80211_fill_txpower(reg_tpc_objp->chan_power_info,
                          max_tx_power, num_pwr_levels);

   /* If is_psd_power is false the following "eirp_power" is invalid */
   reg_tpc_objp->eirp_power = 0;

   return QDF_STATUS_SUCCESS;
}

/* ieee80211_decide_psd_or_eirp_mode() - Decide if power values in TPC object should be in
 * PSD or EIRP format based on the EIRP support flag, PSD support and puncturing.
 * @pdev - Pointer to pdev.
 * @ic_curchan - Pointer to cur chan.
 * @is_reg_eirp_support_preferred - EIRP support flag set by FW.
 *
 * Return - True for PSD format and False for EIRP format
 *
 */
static bool ieee80211_decide_psd_or_eirp_mode(struct wlan_objmgr_pdev *pdev,
                                              struct ieee80211_ath_channel *ic_curchan,
                                              bool is_reg_eirp_support_preferred)
{
   bool is_country_support_psd = false;

   is_country_support_psd = wlan_reg_is_6g_psd_power(pdev);

   if (is_reg_eirp_support_preferred) {
       if (is_country_support_psd) {
           bool is_punctured = false;

#ifdef WLAN_FEATURE_11BE
           is_punctured = IEEE80211_IS_CHAN_PUNCTURED(ic_curchan);
#endif
           /* If EIRP support flag is set, country supports PSD and current channel
            * is punctured, use PSD format, else use EIRP format.
            */
           return is_punctured;
       } else {
           /* If country doesn't support PSD, use EIRP format irrespective of puncturing */
           return false;
       }
   } else  {
       /* If EIRP support flag is not set and the country supports PSD, use PSD format,
        * else use EIRP format.
        */
       return is_country_support_psd;
   }
}

/**
 * ieee80211_get_punc_pattern_for_bw() - Given a channel with a certain
 * puncturing pattern and bandwidth, find the puncturing pattern for a reduced
 * bandwidth.
 *
 * @chan: Pointer to channel.
 * @input_phymode: Phymode of the channel.
 * @target_bw: Target bandwidth for which the input channel's puncturing pattern
 * has to be reduced.
 *
 * Return : Punturing pattern of the reduced bandwidth.
 */
#ifdef WLAN_FEATURE_11BE
uint16_t
ieee80211_get_punc_pattern_for_bw(struct ieee80211_ath_channel *chan,
                                  enum ieee80211_phymode input_phymode,
                                  uint16_t target_bw)
{
    enum ieee80211_phymode des_phymode = ieee80211_get_11bea_mode_from_bw(target_bw);

    if (!IEEE80211_IS_CHAN_PUNCTURED(chan))
        return NO_SCHANS_PUNC;

    if (target_bw == BW_40_MHZ)
        return ieee80211_intersect_puncture_bitmap_for_40(chan, input_phymode);

    return ieee80211_intersect_puncture_bitmap(chan, des_phymode, input_phymode);
}
#endif

/* ieee80211_fill_sp_eirp_power_for_tpc_obj() - Fill the EIRP power values for SP
 * power mode in the vdev's regulatory TPC object.
 * @vap: Pointer to structure ieee80211vap
 * @reg_tpc_objp: Pointer to structure reg_tpc_power_info
 * @primary_freq: Primary frequency of the vdev.
 *
 * Return: None
 */
static
void ieee80211_fill_sp_eirp_power_for_tpc_obj(struct ieee80211vap *vap,
                                              struct reg_tpc_power_info *reg_tpc_objp,
                                              qdf_freq_t primary_freq)
{
    struct ieee80211com *ic = vap->iv_ic;
    uint8_t i;
    int8_t tpe_eirp_pwr[MAX_EIRP_VALS];
    enum reg_6g_ap_type ap_power_type_6g = REG_STANDARD_POWER_AP;
    int8_t afc_eirp_pwr_arr[MAX_EIRP_VALS];
    int8_t sta_max_eirp_pwr_arr[MAX_EIRP_VALS];
    int16_t sta_reg_eirp_pwr, sta_reg_psd_pwr;
    bool is_afc_done = wlan_reg_is_afc_power_event_received(ic->ic_pdev_obj);
    uint8_t num_pwr_levels = reg_tpc_objp->num_eirp_pwr_levels;

    /* Initialise to minimum power for later comparison */
    qdf_mem_set(afc_eirp_pwr_arr, num_pwr_levels * sizeof(uint8_t), CHAN_MIN_EIRP_POWER);
    qdf_mem_set(sta_max_eirp_pwr_arr, num_pwr_levels * sizeof(uint8_t), CHAN_MIN_EIRP_POWER);
    qdf_mem_set(tpe_eirp_pwr, num_pwr_levels * sizeof(uint8_t), CHAN_MIN_EIRP_POWER);

    if (is_afc_done) {
        /* AP AFC power Limited by AP Reg Power
         * If AFC done, both AP and STA can operate in AP_SP power.
         * Hence limit AFC power by AP_reg power
         */
	  ieee80211_reg_get_eirp_arr_for_6g(vap, REG_STANDARD_POWER_AP,
                                          false, REG_MAX_CLIENT_TYPE,
                                          afc_eirp_pwr_arr, vap->iv_bsschan, false);
    } else {
        ieee80211_determinte_6ghz_client_type(ic->ic_pdev_obj, ap_power_type_6g);
        ieee80211_reg_assign_root_tpe_pwr(ic, tpe_eirp_pwr, true);
        /* For non-AFC cases, Find REG Client PSD and EIRP, then convert PSD to EIRP
         * for each BW and limit per-BW EIRP by red_eirp.
         */
        if (ic->ic_sta_vap) {
            wlan_reg_get_client_power_for_connecting_ap(ic->ic_pdev_obj,
                                                        ap_power_type_6g,
                                                        primary_freq,
                                                        wlan_reg_is_6g_psd_power(ic->ic_pdev_obj),
                                                        &sta_reg_eirp_pwr,
                                                        &sta_reg_psd_pwr);
            ieee80211_fill_eirp_from_psd(ic,
                                         sta_reg_psd_pwr,
                                         sta_max_eirp_pwr_arr,
                                         num_pwr_levels,
                                         sta_reg_eirp_pwr);
        }
    }

    for (i = 0; i < num_pwr_levels; i++) {
        if (is_afc_done) {
            reg_tpc_objp->chan_eirp_power_info[i].tx_power = afc_eirp_pwr_arr[i];
        } else {
            reg_tpc_objp->chan_eirp_power_info[i].tx_power = QDF_MIN((int16_t)tpe_eirp_pwr[i], (int16_t)sta_max_eirp_pwr_arr[i]);
        }
    }

    ieee80211_fill_band_centers(vap,
                                reg_tpc_objp->chan_eirp_power_info,
                                num_pwr_levels,
                                primary_freq);
}

/* ieee80211_fill_sp_psd_power_for_tpc_obj() - Fill the PSD power values for SP
 * power mode in the vdev's regulatory TPC object.
 * @vap: Pointer to structure ieee80211vap
 * @reg_tpc_objp: Pointer to structure reg_tpc_power_info
 * @primary_freq: Primary frequency of the vdev.
 *
 * Return: None
 */
static
void ieee80211_fill_sp_psd_power_for_tpc_obj(struct ieee80211vap *vap,
                                             struct reg_tpc_power_info *reg_tpc_objp,
                                             qdf_freq_t start_freq)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    enum reg_6g_ap_type ap_power_type_6g = REG_STANDARD_POWER_AP;
    int16_t reg_eirp_pwr, reg_psd_pwr = CHAN_MIN_PSD_POWER;
    int16_t max_psd_pwr_arr[MAX_PSD_VALS];
    bool is_afc_done = wlan_reg_is_afc_power_event_received(pdev);
    uint8_t i;
    qdf_freq_t sub_chan_freq;
    int16_t afc_psd_pwr[MAX_EIRP_VALS];
    uint8_t num_psds;

    for (i = 0; i < reg_tpc_objp->num_psd_pwr_levels; i++) {
        max_psd_pwr_arr[i] = CHAN_MIN_PSD_POWER;
    }

    if (is_afc_done) {
        bool is_psd_pwr = true;

        /* Get Per-BW OOBE PSD and fill into per 20 subchannel array
         * based on channel bonding information.
         */
        ieee80211_fill_reg_sp_psd_tpc_obj(ic, vap->iv_bsschan, afc_psd_pwr, &num_psds);
        fill_psd_obj(ic, vap->iv_bsschan, afc_psd_pwr, &num_psds, max_psd_pwr_arr);
        for (i = 0; i < reg_tpc_objp->num_psd_pwr_levels; i++) {
            sub_chan_freq = start_freq + BW_20_MHZ * i;

            /* If AFC done, both AP and STA can operate in AP_SP power, Hence limit AFC power by AP_reg power*/
            wlan_reg_get_6g_chan_ap_power(pdev,
                                          sub_chan_freq,
                                          &is_psd_pwr,
                                          &reg_eirp_pwr,
                                          &reg_psd_pwr);

            reg_tpc_objp->chan_psd_power_info[i].chan_cfreq = start_freq + BW_20_MHZ * i;
            reg_tpc_objp->chan_psd_power_info[i].tx_power = QDF_MIN(reg_psd_pwr, max_psd_pwr_arr[i]);
        }
    } else {
        int16_t sta_tpe_psd_pwr = CHAN_MAX_PSD_POWER;

        /* For both STA vap and RE's AP vap when AFC not done */
        if (ic->ic_sta_vap) {
            ni = ic->ic_sta_vap->iv_bss;
        }
        ieee80211_determinte_6ghz_client_type(pdev, ap_power_type_6g);
        for (i = 0; i < reg_tpc_objp->num_psd_pwr_levels; i++) {
            sub_chan_freq = start_freq + BW_20_MHZ * i;

            if (ic->ic_sta_vap) {
                /* For non_AFC, limit TPE by STA Regulatory power */
                sta_tpe_psd_pwr = ni->ni_eirppsd_limit[i];
                wlan_reg_get_client_power_for_connecting_ap(pdev,
                                                            ap_power_type_6g,
                                                            sub_chan_freq,
                                                            true,
                                                            &reg_eirp_pwr,
                                                            &reg_psd_pwr);
            }
            reg_tpc_objp->chan_psd_power_info[i].chan_cfreq = sub_chan_freq;
            reg_tpc_objp->chan_psd_power_info[i].tx_power = QDF_MIN(reg_psd_pwr, sta_tpe_psd_pwr);
        }
    }
}

/* ieee80211_print_psd_eirp_reg_tpc_obj() - Print the TPC WMi objects within a
 * controlled debug prints.
 * @vap: Pointer to structure ieee80211vap
 *
 * Return: None
 */
static void
ieee80211_print_psd_eirp_reg_tpc_obj(struct ieee80211vap *vap)
{
    struct reg_tpc_power_info *tpc_power_info_param = &vap->vdev_mlme->reg_tpc_obj;
    int idx;

    QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
              "Printing TPC power info object");
    QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
              "6g Power Type - %d",
              tpc_power_info_param->power_type_6g);

    QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
              "PSD Array");
    for (idx = 0; idx < tpc_power_info_param->num_psd_pwr_levels; ++idx) {
        QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
                  "sub-chan freq = %d PSD power = %d",
                  tpc_power_info_param->chan_psd_power_info[idx].chan_cfreq,
                  tpc_power_info_param->chan_psd_power_info[idx].tx_power);
    }

    QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
              "EIRP Array");
    for (idx = 0; idx < tpc_power_info_param->num_eirp_pwr_levels; ++idx) {
        QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
                  "Center freq = %d EIRP power = %d",
                  tpc_power_info_param->chan_eirp_power_info[idx].chan_cfreq,
                  tpc_power_info_param->chan_eirp_power_info[idx].tx_power);
    }
}


/* ieee80211_fill_reg_tpc_obj_for_sp() - Fill the PSD and EIRP power values for SP
 * power mode in the vdev's regulatory TPC object.
 * @vap: Pointer to structure ieee80211vap
 * @reg_tpc_objp: Pointer to structure reg_tpc_power_info
 *
 * Return: QDF_STATUS_SUCCESS on success
 */
static QDF_STATUS
ieee80211_fill_reg_tpc_obj_for_sp(struct ieee80211vap *vap, struct reg_tpc_power_info *reg_tpc_objp)
{
    enum reg_6g_ap_type ap_power_type_6g = REG_STANDARD_POWER_AP;
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_pdev *pdev = vap->iv_ic->ic_pdev_obj;
    const struct bonded_channel_freq *bonded_chan_ptr = NULL;
    qdf_freq_t start_freq, primary_freq;
    enum phy_ch_width chwidth;
    enum QDF_OPMODE opmode;
#ifdef CONFIG_AFC_SUPPORT
    enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif

    opmode = wlan_vdev_mlme_get_opmode(vap->vdev_obj);

    /* SP power mode not allowed for non-AFC capable standalone AP vap */
    if (opmode == QDF_SAP_MODE && !wlan_reg_is_afc_power_event_received(ic->ic_pdev_obj)
                && !ic->ic_sta_vap) {
        return QDF_STATUS_E_FAILURE;
    }

    chwidth = wlan_vdev_get_ch_width(vap->vdev_obj);
    primary_freq = vap->iv_bsschan->ic_freq;
    bonded_chan_ptr = wlan_reg_get_bonded_chan_entry(primary_freq,
                                                     chwidth,
                                                     vap->iv_bsschan->ic_vhtop_freq_seg2);

    start_freq = bonded_chan_ptr ? bonded_chan_ptr->start_freq : primary_freq;

    if (wlan_reg_is_6g_psd_power(pdev)) {
        reg_tpc_objp->num_psd_pwr_levels = ieee80211_get_num_pwr_levels(pdev,
                                                                        primary_freq,
                                                                        ieee80211_get_cwm_width(chwidth),
                                                                        true);
        ieee80211_fill_sp_psd_power_for_tpc_obj(vap,
                                                reg_tpc_objp,
                                                start_freq);
    } else {
        qdf_err("Regulatory does not support PSD.");
        return QDF_STATUS_E_FAILURE;
    }

    reg_tpc_objp->num_eirp_pwr_levels = ieee80211_get_num_pwr_levels(pdev,
                                                                     primary_freq,
                                                                     ieee80211_get_cwm_width(chwidth),
                                                                     false);
    ieee80211_fill_sp_eirp_power_for_tpc_obj(vap, reg_tpc_objp, primary_freq);

    /* In case of a Non-AFC capable SP client, fill the power_type as REG_SP_CLIENT_TYPE */
    if (ap_power_type_6g == REG_STANDARD_POWER_AP &&
            !wlan_reg_is_afc_power_event_received(pdev)) {
        reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;
        reg_tpc_objp->is_power_type_client_sp = true;
    }

#ifdef CONFIG_AFC_SUPPORT
    /* In case of a Non-AFC capable Outdoor SP repeater, fill the power_type as REG_SP_CLIENT_TYPE */
    wlan_reg_get_afc_dev_deploy_type(pdev, &reg_afc_deploy_type);
    if ((ieee80211_reg_is_repeater_pdev(ic) && (vap == ic->ic_sta_vap) &&
                wlan_reg_is_6ghz_chan_freq(primary_freq) &&
                !wlan_reg_is_afc_done(pdev, primary_freq) &&
                (reg_afc_deploy_type == AFC_DEPLOYMENT_OUTDOOR))) {
        reg_tpc_objp->power_type_6g = REG_SP_CLIENT_TYPE;
        reg_tpc_objp->is_power_type_client_sp = true;
    }
#endif

    ieee80211_print_psd_eirp_reg_tpc_obj(vap);
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ieee80211_fill_reg_tpc_obj(struct ieee80211vap *vap)
{
   enum reg_6g_ap_type ap_power_type_6g;
   struct wlan_objmgr_pdev *pdev = vap->iv_ic->ic_pdev_obj;
   const struct bonded_channel_freq *bonded_chan_ptr = NULL;
   qdf_freq_t start_freq, primary_freq;
   struct vdev_mlme_obj *vdev_mlme;
   struct reg_tpc_power_info *reg_tpc_objp;
   uint16_t chwidth;
   struct wlan_objmgr_psoc *psoc;
   bool is_reg_eirp_support_preferred = false;
   bool is_both_eirp_psd_supported = false;
   QDF_STATUS status;

   vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vap->vdev_obj);
   if (!vdev_mlme)
       return QDF_STATUS_E_FAILURE;

   psoc = wlan_pdev_get_psoc(pdev);
   if (!psoc)
       return QDF_STATUS_E_FAILURE;

   reg_tpc_objp = &vdev_mlme->reg_tpc_obj;
   qdf_mem_zero(reg_tpc_objp, sizeof(struct reg_tpc_power_info));

   ucfg_reg_get_cur_6g_ap_pwr_type(pdev, &ap_power_type_6g);
   reg_tpc_objp->power_type_6g = ap_power_type_6g;

#ifdef CONFIG_AFC_SUPPORT
   tgt_reg_get_both_psd_eirp_preferred_support(psoc, &is_both_eirp_psd_supported);
#endif
   if (is_both_eirp_psd_supported &&  ap_power_type_6g == REG_STANDARD_POWER_AP) {
       return ieee80211_fill_reg_tpc_obj_for_sp(vap, reg_tpc_objp);
   }

   primary_freq = vap->iv_bsschan->ic_freq;
   chwidth = wlan_vdev_get_chwidth(vap->vdev_obj);
#ifdef WLAN_FEATURE_11BE
   if (chwidth == IEEE80211_CWM_WIDTH320) {
       wlan_reg_get_chan_state_for_320(pdev, primary_freq,
                                       vap->iv_bsschan->ic_vhtop_freq_seg2,
                                       chwidth,
                                       &bonded_chan_ptr,
                                       REG_CURRENT_PWR_MODE, true,
                                       NO_PUNCTURE_BITMASK);
   }
#endif
   if (chwidth <= IEEE80211_CWM_WIDTH80_80) {
       wlan_reg_get_5g_bonded_channel_and_state_for_pwrmode(pdev, primary_freq, chwidth,
                                                            &bonded_chan_ptr,
                                                            REG_CURRENT_PWR_MODE,
                                                            NO_PUNCTURE_BITMASK);
   }

   start_freq = bonded_chan_ptr ? bonded_chan_ptr->start_freq : primary_freq;

#ifdef CONFIG_AFC_SUPPORT
   tgt_reg_get_eirp_preferred_support(psoc, &is_reg_eirp_support_preferred);
#endif

   reg_tpc_objp->is_psd_power = ieee80211_decide_psd_or_eirp_mode(pdev,
                                                                  vap->iv_ic->ic_curchan,
                                                                  is_reg_eirp_support_preferred);
   reg_tpc_objp->num_pwr_levels =
                       ieee80211_get_num_pwr_levels(pdev, primary_freq, chwidth,
                                                    reg_tpc_objp->is_psd_power);

   if (reg_tpc_objp->is_psd_power) {
       status = ieee80211_fill_psd_power_for_tpc_obj(vap, reg_tpc_objp,
                                                     ap_power_type_6g,
                                                     start_freq,
                                                     reg_tpc_objp->is_psd_power);
   } else {
       status = ieee80211_fill_eirp_power_for_tpc_obj(vap, reg_tpc_objp,
                                                      ap_power_type_6g,
                                                      primary_freq);
   }

   return status;
}

#define SIXG_CHAN_NUM_2    2
#define CHAN_FREQ_5935  5935
#define OP_CLASS_136     136
#define OP_EXT_ID        201

/**
 * ieee80211_fill_operating_subband_triplets_for_opclass136() - Fill 6G
 * operating/subband triplets for the 6G opclass 136.
 * @vap: Pointer to vap.
 * @pTriplet: Pointer to *pTriplet.
 *
 * Return: void
 */
static void
ieee80211_fill_operating_subband_triplets_for_opclass136(struct ieee80211vap *vap,
                                                         struct country_ie_triplet **pTriplet)
{
    struct country_ie_triplet *sixg_pTriplet;

    sixg_pTriplet = (*pTriplet)++;
    sixg_pTriplet->regextid = OP_EXT_ID;
    sixg_pTriplet->regclass = OP_CLASS_136;
    sixg_pTriplet->coverageclass = 0;
    vap->iv_country_ie_data.country_len += TRIPLET_SIZE;
    sixg_pTriplet = (*pTriplet)++;
    sixg_pTriplet->schan = SIXG_CHAN_NUM_2;
    sixg_pTriplet->nchan = 1;
    sixg_pTriplet->maxtxpwr = 0;
    vap->iv_country_ie_data.country_len += TRIPLET_SIZE;
}

/**
 * ieee80211_fill_20mhz_6gsubband_triplets() - Fill 6G subband triplets for 20mhz opclass.
 * @vap: Pointer to vap.
 * @pTriplet: Pointer to *pTriplet.
 *
 * Return: void
 */
static void
ieee80211_fill_20mhz_6gsubband_triplets(struct ieee80211vap *vap,
                                        struct country_ie_triplet **pTriplet)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct country_ie_triplet *sixg_pTriplet;
    struct regulatory_channel *chan_list_6g;
    uint8_t j;
    bool isnewband = true;
    uint8_t curchan, prevchan, num_6g_channels;
    bool is_client_sp_pwrmode = false;
    enum reg_6g_ap_type ap_pwr_type, reg_cur_6g_ap_pwr_type;
    enum supported_6g_pwr_types sup_ap_pwr_type;
    bool is_afc_done = wlan_reg_is_afc_power_event_received(ic->ic_pdev_obj);

    ucfg_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &reg_cur_6g_ap_pwr_type);
    if ((reg_cur_6g_ap_pwr_type == REG_STANDARD_POWER_AP) && (!is_afc_done))
        is_client_sp_pwrmode = true;

    chan_list_6g = qdf_mem_malloc(NUM_6GHZ_CHANNELS *
                                  sizeof(struct regulatory_channel));
    if (!chan_list_6g)
        return;

    if (is_client_sp_pwrmode) {
        ap_pwr_type = ieee80211_fill_regulatory_info(ic);
        sup_ap_pwr_type = wlan_reg_conv_6g_ap_type_to_supported_6g_pwr_types(ap_pwr_type);
        num_6g_channels = wlan_reg_get_band_channel_list_for_pwrmode(ic->ic_pdev_obj,
                                                                     BIT(REG_BAND_6G),
                                                                     chan_list_6g,
                                                                     sup_ap_pwr_type, false);
    } else {
        num_6g_channels = wlan_reg_get_band_channel_list(ic->ic_pdev_obj,
                                                         BIT(REG_BAND_6G),
                                                         chan_list_6g);
    }

    if (num_6g_channels > NUM_6GHZ_CHANNELS)
        num_6g_channels = NUM_6GHZ_CHANNELS;

    for (j = 0; j < num_6g_channels; j++) {
         if (chan_list_6g[j].chan_num == SIXG_CHAN_NUM_2)
             continue;
         curchan = chan_list_6g[j].chan_num;
         if (isnewband) {
             isnewband = false;
             sixg_pTriplet = (*pTriplet)++;
         } else if (curchan == (prevchan + CHAN_DIFF)) {
             sixg_pTriplet->nchan++;
             prevchan = curchan;
             continue;
         } else {
             sixg_pTriplet = (*pTriplet)++;
         }

         prevchan = curchan;
         sixg_pTriplet->schan = curchan;
         sixg_pTriplet->nchan = 1;
         /* "The Maximum Transmit Power Level field is reserved if it
          * is within an Operating/Subband Sequence field with the
          * Operating class for which the Channel starting frequency
          * (GHz) column in Table E-4 is greater than or
          * equal to 5.925 and less than or equal to 7.125"
          */
         sixg_pTriplet->maxtxpwr = 0;
         vap->iv_country_ie_data.country_len += TRIPLET_SIZE;
    }

    qdf_mem_free(chan_list_6g);

}

/**
 * ieee80211_fill_country_ie_for_6g() - Fill Country IE for the 6G channels.
 * @vap: Pointer to vap.
 * @pTriplet: Pointer to *pTriplet.
 *
 * Return: Void
 */
static void
ieee80211_fill_country_ie_for_6g(struct ieee80211vap *vap,
                                 struct country_ie_triplet **pTriplet)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct country_ie_triplet *sixg_pTriplet;
    uint8_t i;
    const u_int8_t sixg_opclss_arr[][2] = {{131, IEEE80211_MODE_11AXA_HE20},
                                           {132, IEEE80211_MODE_11AXA_HE40PLUS},
                                           {132, IEEE80211_MODE_11AXA_HE40MINUS},
                                           {133, IEEE80211_MODE_11AXA_HE80},
                                           {134, IEEE80211_MODE_11AXA_HE160},
#ifdef WLAN_FEATURE_11BE
                                           {137, IEEE80211_MODE_11BEA_EHT320},
#endif
                                           };

    /* By default (if not enabled) only 20Mhz opclass will be present */
    uint8_t num_opclass = 1;

   /* Rule-1 :- No regular Subband Triplet
    * When the Country element is included in a frame transmitted in the
    * 6 GHz band, the Triplet field is composed of zero subband Triplet
    * fields, and only has one or more Operating/subband Sequences.
    *
    * Rule-2: Fill Operating/Subband Sequences for 6g
    * Fill Operating/Subband Sequences for every BW 20/40/80/80+80/160
    * Fill for 20 Operating Triplet+Subband Triplet sequences
    * OP_EXT_ID can be anything greater than 200 and lesser than or equal to 23
    */

    ieee80211_add_additional_triplets(ic, &num_opclass);

    /* Fill for 20/40/80/160/320 */
    for (i = 0; i < num_opclass; i++) {
         if (IEEE80211_SUPPORT_PHY_MODE(ic, sixg_opclss_arr[i][1])) {
             sixg_pTriplet = (*pTriplet)++;
             sixg_pTriplet->regextid = OP_EXT_ID;
             sixg_pTriplet->regclass = sixg_opclss_arr[i][0];
             sixg_pTriplet->coverageclass = 0;
             vap->iv_country_ie_data.country_len += TRIPLET_SIZE;
             /* Since opclass 132 appear twice in the array "sixg_opclss_arr"
              * (First time for IEEE80211_MODE_11AXA_HE40PLUS  and second time
              * for IEEE80211_MODE_11AXA_HE40MINUS) appear twice in the array,
              * if the device supports both 40+ and 40-, the opclass will
              * appear twice in the country IE. To avoid that do not add 132
              * if it is already added.
              */
             if (ieee80211_is_phymode_11axa_he40plus(sixg_opclss_arr[i][1])) {
                 /* if  132 already added do not add it again */
                 i = i + 1;
             }

             /* Subband Triplet sequences is mandatory only for 20 MHz */
             if (i == 0)
                 ieee80211_fill_20mhz_6gsubband_triplets(vap, pTriplet);

         }
    }
    /*
     * In 6Ghz band, IEEE channel 2 (5935MHz) is a special channel because
     * it is placed before IEEE channel 1 (5955MHz). Therefore, it does not
     * follow the natural order of numbers and is being handled in a
     * separate function instead of being handled in the for loop above.
     */
    if (wlan_reg_is_freq_present_in_cur_chan_list(ic->ic_pdev_obj,
                                                  CHAN_FREQ_5935) &&
        IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11AXA_HE20)) {
        ieee80211_fill_operating_subband_triplets_for_opclass136(vap, pTriplet);
    }

}

QDF_STATUS
ieee80211_reg_get_eirp_arr_for_6g(struct ieee80211vap *vap,
                                  enum reg_6g_ap_type ap_pwr_type,
                                  bool is_client_pwr_needed,
                                  enum reg_6g_client_type client_type,
                                  int8_t *max_eirp_arr,
                                  struct ieee80211_ath_channel *channel,
                                  bool is_twice_power)
{
    struct ieee80211com *ic = vap->iv_ic;
    uint16_t bw, max_bw;
    uint8_t i;
    uint16_t input_punc_bitmap = NO_SCHANS_PUNC;
    enum ieee80211_phymode phymode;
    int16_t punc_subchan_pwr = CHAN_MIN_PSD_POWER;

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TPE_IE, IEEE80211_MSG_TPE_STR "Empty channel\n");
        return QDF_STATUS_E_FAILURE;
    }
    max_bw = ieee80211_get_chan_width(channel);
    phymode = ieee80211_chan2mode(channel);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TPE_IE, IEEE80211_MSG_TPE_STR "max_bw: %u, phymode: %d\n", max_bw, phymode);

    for (i = 0, bw = BW_20_MHZ; bw <= max_bw; i++, bw *= 2) {
#ifdef WLAN_FEATURE_11BE
        enum phy_ch_width ch_width = wlan_reg_find_chwidth_from_bw(bw);
#endif
        bool is_punc_bitmap_valid = false;

#ifdef WLAN_FEATURE_11BE
        if (IEEE80211_IS_CHAN_PUNCTURED(channel)) {
            input_punc_bitmap  = ieee80211_get_punc_pattern_for_bw(channel,
                                                                   phymode, bw);
            is_punc_bitmap_valid = wlan_reg_is_punc_bitmap_valid(ch_width, input_punc_bitmap);
        }
#endif
        if (!input_punc_bitmap  || is_punc_bitmap_valid) {
            max_eirp_arr[i] = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj,
                                                    channel->ic_freq,
                                                    channel->ic_vhtop_freq_seg2,
                                                    bw,
                                                    ap_pwr_type,
                                                    input_punc_bitmap,
                                                    is_client_pwr_needed,
                                                    client_type, is_twice_power);
        } else {
            /* If the intersected puncture_bitmap for a bandwidth is invalid,
             * For example:If the AP is operating on channel 69 HT320 with a
             * puncture pattern of 0xFC00, following are the EIRP powers for
             * different bandwidths:
             * 20 MHz: EIRP power of 69.
             * 40 MHz: EIRP power of 40 MHz pair 65 and 69
             * 80 MHz: EIRP power of 80 MHz is not possible since the secondary
             *         40 MHz of the 80 MHz is punctured.
             * 160 MHz: EIRP power of 160 MHz is not possible since the
             *          secondary 80 MHz of the 160 MHz is punctured.
             * 320 MHz: PSD power of channel 69 + 10 log10 (320 – 120)
             * Hence, for 160 and 80 MHz BW, fill the effective BW EIRP value.
             */
            max_eirp_arr[i] = punc_subchan_pwr;
        }
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TPE_IE, IEEE80211_MSG_TPE_STR "ap_type: %u, client_type: %u, bw: %u, input_punc: %x, max_eirp_arr[%u] = %d\n",
                          ap_pwr_type, client_type, bw, input_punc_bitmap, i, max_eirp_arr[i]);
    }
    return QDF_STATUS_SUCCESS;;
}

QDF_STATUS
ieee80211_reg_get_eirp_arr_for_6g_compap(struct ieee80211vap *vap,
                                         enum reg_6g_ap_type ap_pwr_type,
                                         bool is_client_pwr_needed,
                                         enum reg_6g_client_type client_type,
                                         qdf_freq_t freq,
                                         int8_t *max_eirp_arr,
                                         struct ieee80211_ath_channel *chan)
{
    uint16_t bw, max_bw = ieee80211_get_chan_width(chan);
    int8_t max_eirp_arr_lpi[IEEE80211_TPE_NUM_POWER_SUPPORTED];
    int8_t max_eirp_arr_sp[IEEE80211_TPE_NUM_POWER_SUPPORTED];
    uint8_t i;
    QDF_STATUS status;

    status =  ieee80211_reg_get_eirp_arr_for_6g(vap, ap_pwr_type,
                                                is_client_pwr_needed,
                                                client_type,
                                                max_eirp_arr_lpi, chan, true);
    if (status != QDF_STATUS_SUCCESS) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TPE_IE, IEEE80211_MSG_TPE_STR "Error fetching LPI power for composite AP\n");
        return status;
    }

    status = ieee80211_reg_get_eirp_arr_for_6g(vap, REG_STANDARD_POWER_AP,
                                               true,
                                               client_type,
                                               max_eirp_arr_sp, chan, true);
    if (status != QDF_STATUS_SUCCESS) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TPE_IE, IEEE80211_MSG_TPE_STR "Error fetching SP power for composite AP\n");
        return status;
    }

    for(i = 0, bw = BW_20_MHZ; bw <= max_bw; i++, bw *= 2) {
        max_eirp_arr[i] = QDF_MAX(max_eirp_arr_lpi[i], max_eirp_arr_sp[i]);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TPE_IE, IEEE80211_MSG_TPE_STR "COMPOSITE AP: max_eirp_arr_lpi[%u] = %d, max_eirp_arr_sp[%u] = %d, max_eirp_arr[%d] = %d\n",
                          i, max_eirp_arr_lpi[i], i, max_eirp_arr_sp[i], i, max_eirp_arr[i]);
    }

    return QDF_STATUS_SUCCESS;
}

void
ieee80211_get_psd_from_eirp(struct ieee80211vap *vap,
                            int16_t *max_reg_psd_pwr,
                            int8_t *max_eirp_arr,
                            bool is_twice_eirp_power)
{
    int16_t max_psd = CHAN_MIN_PSD_POWER;
    struct ieee80211com *ic = vap->iv_ic;
    uint16_t bw = ieee80211_get_chan_width(ic->ic_curchan);
    uint8_t i, num_pwr_levels;
    enum ieee80211_cwm_width chwidth;
    bool is_psd_pwr;
    int8_t eirp_power;

    is_psd_pwr = wlan_reg_is_6g_psd_power(ic->ic_pdev_obj);
    chwidth = wlan_vdev_get_chwidth(vap->vdev_obj);
    num_pwr_levels = ieee80211_get_num_pwr_levels(ic->ic_pdev_obj,
                                                  ic->ic_curchan->ic_freq,
                                                  chwidth, is_psd_pwr);
    bw = BW_20_MHZ;
    for (i = 0; i < num_pwr_levels; i++) {
         int16_t reg_psd = CHAN_MAX_PSD_POWER;

         eirp_power = max_eirp_arr[i];
         if (is_twice_eirp_power)
             eirp_power /= 2;

         wlan_reg_eirp_2_psd(ic->ic_pdev_obj, bw, eirp_power, &reg_psd);
         if (reg_psd > max_psd)
             max_psd = reg_psd;

         bw *= 2;
    }

    *max_reg_psd_pwr = max_psd;
}
#endif

static bool ieee80211_regdmn_check_weather_radar_channel(qdf_freq_t freq)
{
    return ((freq >= 5600) && (freq <= 5640));
}

static struct
country_ie_triplet *ieee80211_fill_2g_5g_triplets(struct ieee80211vap *vap,
                                                  struct country_ie_triplet *pTriplet,
                                                  uint32_t dfs_reg)
{
    struct regulatory_channel *cur_chan_list;
    struct ieee80211com *ic = vap->iv_ic;
    u_int8_t prevchan = 0;
    int i, isnewband = 1;

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list) {
        /* Default, no country is set */
        vap->iv_country_ie_data.country_len = 0;
        IEEE80211_DISABLE_COUNTRYIE(ic);
        return NULL;
   }

   if (wlan_reg_get_current_chan_list(ic->ic_pdev_obj, cur_chan_list) !=
       QDF_STATUS_SUCCESS) {
       qdf_err("Failed to get cur_chan list");
       vap->iv_country_ie_data.country_len = 0;
       IEEE80211_DISABLE_COUNTRYIE(ic);
       qdf_mem_free(cur_chan_list);
       return NULL;
   }

   for (i = 0; i < NUM_CHANNELS; i++) {
        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
                continue;
       /* We assume the following sequence in the channel list:
        * |All 2.4Ghz channels| |All 5Ghz channels| |All 6Ghz channels|
        */
#ifdef CONFIG_BAND_6GHZ
        if (wlan_reg_is_6ghz_chan_freq(cur_chan_list[i].center_freq))
            break;
#endif
        if (ic->ic_no_weather_radar_chan &&
            (ieee80211_regdmn_check_weather_radar_channel(cur_chan_list[i].center_freq))
                && (DFS_ETSI_DOMAIN  == dfs_reg))
        {
            /* skipping advertising weather radar channels */
            continue;
        }

        if (isnewband) {
            isnewband = 0;
#ifdef BEACON_CHANLIST_COMPRESSION
        } else if (ieee80211_chanlist_compression_possible(&cur_chan_list[i],
                                                           prevchan,
                                                           pTriplet))
        {
            pTriplet->nchan++;
            prevchan = cur_chan_list[i].chan_num;
            continue;
#else
        } else if ((pTriplet->maxtxpwr == cur_chan_list[i].tx_power) &&
                   (cur_chan_list[i].chan_num == prevchan + 1)) {
            pTriplet->nchan++;
            prevchan = cur_chan_list[i].chan_num;
            continue;
#endif
        } else {
            pTriplet++;
        }

        prevchan = cur_chan_list[i].chan_num;
        pTriplet->schan = cur_chan_list[i].chan_num;
        pTriplet->nchan = 1; /* init as 1 channel */
        pTriplet->maxtxpwr = cur_chan_list[i].tx_power;
        vap->iv_country_ie_data.country_len += TRIPLET_SIZE;
    }
    qdf_mem_free(cur_chan_list);

    return pTriplet;
}

/*
 * Build the country information element.
 */
void
ieee80211_build_countryie(struct ieee80211vap *vap, uint8_t *country_iso)
{
    struct country_ie_triplet *pTriplet;
    struct country_ie_triplet *first_pTriplet;
    u_int64_t chanflags = 0;
    struct ieee80211com *ic = vap->iv_ic;
    uint32_t dfs_reg = 0;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null", __func__);
        return;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (psoc == NULL) {
        qdf_print("%s : psoc is null", __func__);
        return;
    }

    reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
    if (reg_rx_ops == NULL) {
        qdf_err("%s : rx_ops is null", __func__);
        return;
    }
    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_REGULATORY_SB_ID) !=
            QDF_STATUS_SUCCESS) {
        return;
    }
    reg_rx_ops->get_dfs_region(pdev, &dfs_reg);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);

    if (!country_iso[0] || !country_iso[1] || !country_iso[2]) {
        /* Default, no country is set */
        vap->iv_country_ie_data.country_len = 0;
        IEEE80211_DISABLE_COUNTRYIE(ic);
        return;
    }

    IEEE80211_ENABLE_COUNTRYIE(ic);

    /*
     * Construct the country IE:
     * 1. The country string come first.
     * 2. Then construct the channel triplets from lowest channel to highest channel.
     * 3. If we support the regulatory domain class (802.11J)
     *    then add the class triplets before the channel triplets of each band.
     */
    OS_MEMZERO(&vap->iv_country_ie_data, sizeof(vap->iv_country_ie_data));
    vap->iv_country_ie_data.country_id = IEEE80211_ELEMID_COUNTRY;
    vap->iv_country_ie_data.country_len = 3;

    vap->iv_country_ie_data.country_str[0] = country_iso[0];
    vap->iv_country_ie_data.country_str[1] = country_iso[1];
    vap->iv_country_ie_data.country_str[2] = country_iso[2];

    if (wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq)) {
        ic->ic_opclass_tbl_idx = OPCLS_TAB_IDX_GLOBAL;
        vap->iv_country_ie_data.country_str[2] = ic->ic_opclass_tbl_idx;
    }

    pTriplet = (struct country_ie_triplet*)&vap->iv_country_ie_data.country_triplet;
    first_pTriplet = pTriplet;

    if (wlan_reg_is_24ghz_ch_freq(ic->ic_curchan->ic_freq))
        chanflags = IEEE80211_CHAN_2GHZ;
    else if (wlan_reg_is_5ghz_ch_freq(ic->ic_curchan->ic_freq)
#ifdef CONFIG_49GHZ_CHAN
             || wlan_reg_is_49ghz_freq(ic->ic_curchan->ic_freq)
#endif
             )
             chanflags = IEEE80211_CHAN_5GHZ;
    else if (wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq))
             chanflags = IEEE80211_CHAN_6GHZ;

    vap->iv_country_ie_chanflags = chanflags;

    if (chanflags == IEEE80211_CHAN_5GHZ || chanflags == IEEE80211_CHAN_2GHZ)
        pTriplet = ieee80211_fill_2g_5g_triplets(vap, pTriplet, dfs_reg);

    if (pTriplet == NULL)
        return;

#ifdef CONFIG_BAND_6GHZ
    if (chanflags == IEEE80211_CHAN_6GHZ) {
        if (first_pTriplet != pTriplet)
            pTriplet++;

        ieee80211_fill_country_ie_for_6g(vap, &pTriplet);
    }
#endif
    /* pad */
    if (vap->iv_country_ie_data.country_len & 1) {
        vap->iv_country_ie_data.country_triplet[vap->iv_country_ie_data.country_len] = 0;
        vap->iv_country_ie_data.country_len++;
    }

}

static void
ieee80211_build_countryie_vap(uint8_t *ctry_iso, struct ieee80211vap *vap, bool is_last_vap)
{
    if (IEEE80211_IS_COUNTRYIE_ENABLED(vap->iv_ic) &&
        ieee80211_vap_country_ie_is_set(vap)) {
        ieee80211_build_countryie(vap, ctry_iso);
    }
}

/*
* update the country ie in all vaps.
*/
void
ieee80211_build_countryie_all(struct ieee80211com *ic, uint8_t *ctry_iso)
{
    u_int8_t num_vaps;
    ieee80211_iterate_vap_list_internal(ic,ieee80211_build_countryie_vap,(void *)ctry_iso,num_vaps);
}
qdf_export_symbol(ieee80211_build_countryie_all);

static int
ieee80211_set_countrycode(struct ieee80211com *ic, char isoName[3], u_int16_t cc, enum ieee80211_clist_cmd cmd)
{
    struct ol_ath_softc_net80211 *scn;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (ieee80211_set_country_code(ic, isoName, cc, cmd)) {
        IEEE80211_CLEAR_DESIRED_COUNTRY(ic);
        return -EINVAL;
    }

    if (!cc) {
        if ((isoName == NULL) || (isoName[0] == '\0') || (isoName[1] == '\0')) {
            IEEE80211_CLEAR_DESIRED_COUNTRY(ic);
        } else {
            IEEE80211_SET_DESIRED_COUNTRY(ic);
        }
    } else {
        IEEE80211_SET_DESIRED_COUNTRY(ic);
    }
#if UMAC_SUPPORT_CFG80211
    wlan_cfg80211_update_channel_list(ic);
#endif

    if (wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_STRICT_CHANNEL)) {
        wlan_if_t tmpvap;
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if(tmpvap) {
                struct net_device *tmpdev;
                tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
                if (!IS_UP(tmpdev)) {
                    IEEE80211_DELIVER_EVENT_CHANNEL_CHANGE(tmpvap, tmpvap->iv_des_chan[tmpvap->iv_des_mode]);
                }
            }
        }
    }

    return 0;
}

int
ieee80211_regdmn_reset(struct ieee80211com *ic)
{
    char cc[3];

    /* Reset to default country if any. */
    cc[0] = cc[1] = cc[2] = 0;
    ieee80211_set_countrycode(ic, cc, 0, CLIST_UPDATE);
    ic->ic_multiDomainEnabled = 0;

    return 0;
}


int
wlan_set_countrycode(wlan_dev_t devhandle, char isoName[3], u_int16_t cc, enum ieee80211_clist_cmd cmd)
{
    return ieee80211_set_countrycode(devhandle, isoName, cc, cmd);
}

u_int16_t
wlan_get_regdomain(wlan_dev_t devhandle)
{
    return devhandle->ic_country.regDmnEnum;
}

int
wlan_set_regdomain(wlan_dev_t devhandle, uint32_t regdmn)
{
    struct ieee80211com *ic = devhandle;
    struct cc_regdmn_s rd;

    rd.cc.regdmn.reg_2g_5g_pair_id = REG_GET_BITS(regdmn, 0, 16);
    rd.cc.regdmn.sixg_superdmn_id = REG_GET_BITS(regdmn, 16, 16);
    rd.flags = REGDMN_IS_SET;

    return ieee80211_regdmn_program_cc(ic->ic_pdev_obj, &rd);
}

u_int8_t
wlan_get_ctl_by_country(wlan_dev_t devhandle, u_int8_t *country, bool is2G)
{
    struct ieee80211com *ic = devhandle;
    return ic->ic_get_ctl_by_country(ic, country, is2G);
}

/**
 * regdmn_get_supp_opclass_list - Get the maximum txpower of the current operating channel.
 * @pdev - Pointer to pdev.
 *
 * Return - return the 2's complement of maximum tx power of the current operating channel.
 */
int regdmn_get_current_chan_txpower(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);

   if (ic == NULL) {
       qdf_err("ic is NULL");
       return -EINVAL;
   }

    return ~(wlan_channel_maxpower(ic->ic_curchan)) + 1;
}

/**
 * regdmn_get_supp_opclass_list - Get the current operating channel number and operating class.
 * @vdev - Pointer to vdev.
 * @chan_num - Pointer to channel number.
 * @opclass - Pointer to opclass.
 *
 * Return - void
 */
void regdmn_get_curr_chan_and_opclass(struct wlan_objmgr_vdev *vdev,
                                      uint8_t *chan_num,
                                      uint8_t *opclass)
{
   wlan_if_t vap;

   vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
   if (vap == NULL) {
       qdf_err("VAP is NULL");
       return;
   }

   if (opclass)
       *opclass = wlan_get_opclass(vdev);

   if (chan_num)
       *chan_num = vap->iv_ic->ic_curchan->ic_ieee;
}

/**
 * regdmn_get_supp_opclass_list - Get supported opclass list and number of supported opclasses.
 * @pdev - Pointer to pdev.
 * @opclass_list - Pointer to opclass list.
 * @num_supp_op_class - Pointer to number of supported opclass.
 * @global_tbl_lookup - Global table lookup.
 *
 * Return - void
 */
void regdmn_get_supp_opclass_list(struct wlan_objmgr_pdev *pdev,
                                  uint8_t *opclass_list,
                                  uint8_t *num_supp_op_class,
                                  bool global_tbl_lookup)
{
   struct regdmn_ap_cap_opclass_t *reg_ap_cap =  NULL;
   QDF_STATUS status;
   uint8_t index = 0, idx = 0;

   reg_ap_cap = qdf_mem_malloc(REG_MAX_SUPP_OPER_CLASSES * sizeof(*reg_ap_cap));

   if (!reg_ap_cap) {
       qdf_err("reg_ap_cap is NULL");
       return;
   }

   status = wlan_reg_get_opclass_details(pdev, reg_ap_cap,
                                         &index, REG_MAX_SUPP_OPER_CLASSES,
                                         global_tbl_lookup,
                                         REG_CURRENT_PWR_MODE);

  if (status != QDF_STATUS_SUCCESS) {
      qdf_err("Failed to get opclass details");
      qdf_mem_free(reg_ap_cap);
      return;
  }

  while (reg_ap_cap[idx].op_class && (idx < index)) {
         if(opclass_list)
            opclass_list[idx] = reg_ap_cap[idx].op_class;

         idx++;
  }

  *num_supp_op_class  = idx;
  qdf_mem_free(reg_ap_cap);
}

static bool
ieee80211_chansort(const void *chan1, const void *chan2)
{
    const struct ieee80211_ath_channel *tmp_chan1 = chan1;
    const struct ieee80211_ath_channel *tmp_chan2 = chan2;

    return (tmp_chan1->ic_freq == tmp_chan2->ic_freq) ?
        (ieee80211_get_mode(tmp_chan1) > ieee80211_get_mode(tmp_chan2)):
        tmp_chan1->ic_freq > tmp_chan2->ic_freq;
}

/*
 * Insertion sort.
 */
#define ieee80211_swap(_chan1, _chan2, _size) {     \
    u_int8_t *tmp_chan2 = _chan2;                   \
    int i = _size;                                  \
    do {                                            \
        u_int8_t tmp = *_chan1;                     \
        *_chan1++ = *tmp_chan2;                     \
        *tmp_chan2++ = tmp;                         \
    } while (--i);                                  \
    _chan1 -= _size;                                \
}

static void ieee80211_channel_sort(void *chans, uint32_t next, uint32_t size)
{
    uint8_t *tmp_chan = chans;
    uint8_t *ptr1, *ptr2;

    for (ptr1 = tmp_chan + size; --next >= 1; ptr1 += size)
        for (ptr2 = ptr1; ptr2 > tmp_chan; ptr2 -= size) {
            uint8_t *index = ptr2 - size;
            if (!ieee80211_chansort(index, ptr2))
                break;
            ieee80211_swap(index, ptr2, size);
        }
}

enum {
    CHANNEL_40_NO,
    CHANNEL_40_PLUS,
    CHANNEL_40_MINUS,
};

struct reg_cmode {
    uint64_t mode;
    uint64_t flags;
    uint32_t bw;
    uint32_t chan_ext;
};

static const struct reg_cmode modes[] = {
    { HOST_REGDMN_MODE_TURBO,               IEEE80211_CHAN_ST,
        CH_WIDTH_20MHZ, CHANNEL_40_NO }, /* TURBO means 11a Static Turbo */
#ifndef ATH_NO_5G_SUPPORT
    { HOST_REGDMN_MODE_11A,                 IEEE80211_CHAN_A,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
#endif

    { HOST_REGDMN_MODE_11B,                 IEEE80211_CHAN_B,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11G,                 IEEE80211_CHAN_PUREG,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },

    { HOST_REGDMN_MODE_11NG_HT20,           IEEE80211_CHAN_HT20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11NG_HT40PLUS,       IEEE80211_CHAN_HT40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11NG_HT40MINUS,      IEEE80211_CHAN_HT40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },

#ifndef ATH_NO_5G_SUPPORT
    { HOST_REGDMN_MODE_11NA_HT20,           IEEE80211_CHAN_HT20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11NA_HT40PLUS,       IEEE80211_CHAN_HT40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11NA_HT40MINUS,      IEEE80211_CHAN_HT40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },

    { HOST_REGDMN_MODE_11AC_VHT20,          IEEE80211_CHAN_VHT20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AC_VHT40PLUS,      IEEE80211_CHAN_VHT40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11AC_VHT40MINUS,     IEEE80211_CHAN_VHT40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },
    { HOST_REGDMN_MODE_11AC_VHT80,          IEEE80211_CHAN_VHT80,
        CH_WIDTH_80MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AC_VHT160,         IEEE80211_CHAN_VHT160,
        CH_WIDTH_160MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AC_VHT80_80,       IEEE80211_CHAN_VHT80_80,
        CH_WIDTH_80P80MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AXG_HE20,          IEEE80211_CHAN_HE20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AXA_HE20,          IEEE80211_CHAN_HE20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AXG_HE40PLUS,      IEEE80211_CHAN_HE40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11AXG_HE40MINUS,     IEEE80211_CHAN_HE40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },
    { HOST_REGDMN_MODE_11AXA_HE40PLUS,      IEEE80211_CHAN_HE40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11AXA_HE40MINUS,     IEEE80211_CHAN_HE40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },
    { HOST_REGDMN_MODE_11AXA_HE80,          IEEE80211_CHAN_HE80,
        CH_WIDTH_80MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AXA_HE160,         IEEE80211_CHAN_HE160,
        CH_WIDTH_160MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11AXA_HE80_80,       IEEE80211_CHAN_HE80_80,
        CH_WIDTH_80P80MHZ, CHANNEL_40_NO },
    /* 802.11BE Modes and Flags */
#ifdef WLAN_FEATURE_11BE
    { HOST_REGDMN_MODE_11BEG_EHT20,          IEEE80211_CHAN_EHT20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11BEA_EHT20,          IEEE80211_CHAN_EHT20,
        CH_WIDTH_20MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11BEG_EHT40PLUS,      IEEE80211_CHAN_EHT40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11BEG_EHT40MINUS,     IEEE80211_CHAN_EHT40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },
    { HOST_REGDMN_MODE_11BEA_EHT40PLUS,      IEEE80211_CHAN_EHT40PLUS,
        CH_WIDTH_40MHZ, CHANNEL_40_PLUS },
    { HOST_REGDMN_MODE_11BEA_EHT40MINUS,     IEEE80211_CHAN_EHT40MINUS,
        CH_WIDTH_40MHZ, CHANNEL_40_MINUS },
    { HOST_REGDMN_MODE_11BEA_EHT80,          IEEE80211_CHAN_EHT80,
        CH_WIDTH_80MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11BEA_EHT160,         IEEE80211_CHAN_EHT160,
        CH_WIDTH_160MHZ, CHANNEL_40_NO },
    { HOST_REGDMN_MODE_11BEA_EHT320,         IEEE80211_CHAN_EHT320,
        CH_WIDTH_320MHZ, CHANNEL_40_NO },
#endif
#endif
};

static const u_int64_t chanflags[] = {
    0,                              /* IEEE80211_MODE_AUTO */
    IEEE80211_CHAN_A,               /* IEEE80211_MODE_11A */
    IEEE80211_CHAN_B,               /* IEEE80211_MODE_11B */
    IEEE80211_CHAN_PUREG,           /* IEEE80211_MODE_11G */
    IEEE80211_CHAN_FHSS,            /* IEEE80211_MODE_FH */
    IEEE80211_CHAN_108A,            /* IEEE80211_MODE_TURBO_A */
    IEEE80211_CHAN_108G,            /* IEEE80211_MODE_TURBO_G */
    IEEE80211_CHAN_HT20,       /* IEEE80211_MODE_11NA_HT20 */
    IEEE80211_CHAN_HT20,       /* IEEE80211_MODE_11NG_HT20 */
    IEEE80211_CHAN_HT40PLUS,   /* IEEE80211_MODE_11NA_HT40PLUS */
    IEEE80211_CHAN_HT40MINUS,  /* IEEE80211_MODE_11NA_HT40MINUS */
    IEEE80211_CHAN_HT40PLUS,   /* IEEE80211_MODE_11NG_HT40PLUS */
    IEEE80211_CHAN_HT40MINUS,  /* IEEE80211_MODE_11NG_HT40MINUS */
    0,                              /* IEEE80211_MODE_11NG_HT40 */
    0,                              /* IEEE80211_MODE_11NA_HT40 */
    IEEE80211_CHAN_VHT20,      /* IEEE80211_MODE_11AC_VHT20 */
    IEEE80211_CHAN_VHT40PLUS,  /* IEEE80211_MODE_11AC_VHT40PLUS */
    IEEE80211_CHAN_VHT40MINUS, /* IEEE80211_MODE_11AC_VHT40MINUS */
    0,                              /* IEEE80211_MODE_11AC_VHT40 */
    IEEE80211_CHAN_VHT80,      /* IEEE80211_MODE_11AC_VHT80 */
    IEEE80211_CHAN_VHT160,     /* IEEE80211_MODE_11AC_VHT160 */
    IEEE80211_CHAN_VHT80_80,   /* IEEE80211_MODE_11AC_VHT80_80 */
    IEEE80211_CHAN_HE20,      /* IEEE80211_MODE_11AXA_HE20 */
    IEEE80211_CHAN_HE20,      /* IEEE80211_MODE_11AXG_HE20 */
    IEEE80211_CHAN_HE40PLUS,  /* IEEE80211_MODE_11AXA_HE40PLUS */
    IEEE80211_CHAN_HE40MINUS, /* IEEE80211_MODE_11AXA_HE40MINUS */
    IEEE80211_CHAN_HE40PLUS,  /* IEEE80211_MODE_11AXG_HE40PLUS */
    IEEE80211_CHAN_HE40MINUS, /* IEEE80211_MODE_11AXG_HE40MINUS */
    0,                              /* IEEE80211_MODE_11AXA_HE40 */
    0,                              /* IEEE80211_MODE_11AXG_HE40 */
    IEEE80211_CHAN_HE80,      /* IEEE80211_MODE_11AXA_HE80 */
    IEEE80211_CHAN_HE160,     /* IEEE80211_MODE_11AXA_HE160 */
    IEEE80211_CHAN_HE80_80,   /* IEEE80211_MODE_11AXA_HE80_80 */
#ifdef WLAN_FEATURE_11BE
    IEEE80211_CHAN_EHT20,      /* IEEE80211_MODE_11BEA_EHT20 */
    IEEE80211_CHAN_EHT20,      /* IEEE80211_MODE_11BEG_EHT20 */
    IEEE80211_CHAN_EHT40PLUS,  /* IEEE80211_MODE_11BEA_EHT40PLUS */
    IEEE80211_CHAN_EHT40MINUS, /* IEEE80211_MODE_11BEA_EHT40MINUS */
    IEEE80211_CHAN_EHT40PLUS,  /* IEEE80211_MODE_11BEG_EHT40PLUS */
    IEEE80211_CHAN_EHT40MINUS, /* IEEE80211_MODE_11BEG_EHT40MINUS */
    0,                         /* IEEE80211_MODE_11BEA_EHT40 */
    0,                         /* IEEE80211_MODE_11BEG_EHT40 */
    IEEE80211_CHAN_EHT80,      /* IEEE80211_MODE_11BEA_EHT80 */
    IEEE80211_CHAN_EHT160,     /* IEEE80211_MODE_11BEA_EHT160 */
    IEEE80211_CHAN_EHT320,     /* IEEE80211_MODE_11BEA_EHT320 */
#endif /* WLAN_FEATURE_11BE */
    IEEE80211_CHAN_ST,              /* IEEE80211_MODE_TURBO_STATIC_A */
};

/*
 * ieee80211_mode_to_chanflags(): Maps user configured phymode to ic_flags.
 * @mode: User configured phymode of type ieee80211_phymode.
 * Return - Equivalent channel ic_flags of given mode.
 */
u_int64_t ieee80211_mode_to_chanflags(enum ieee80211_phymode mode)
{
    return chanflags[mode];
}

static bool
regdmn_duplicate_channel(struct ieee80211_ath_channel *chan,
        struct ieee80211_ath_channel *list, int size)
{
    uint16_t i;

    for (i=0; i<size; i++) {
        if (chan->ic_freq == list[i].ic_freq &&
                chan->ic_flags == list[i].ic_flags &&
                chan->ic_flagext == list[i].ic_flagext &&
                chan->ic_vhtop_freq_seg1 == list[i].ic_vhtop_freq_seg1 &&
                chan->ic_vhtop_freq_seg2 == list[i].ic_vhtop_freq_seg2)
            return true;
    }

    return false;
}

u_int
regdmn_mhz2ieee(u_int freq, u_int flags)
{
#define IS_CHAN_IN_PUBLIC_SAFETY_BAND(_c) ((_c) > 4940 && (_c) < 4990)

    if (freq == 2484)
        return 14;
    if (freq < 2484)
        return (freq - 2407) / 5;
    if (freq < 5000) {
        if (IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
            return ((freq * 10) +
                    (((freq % 5) == 2) ? 5 : 0) - 49400)/5;
        } else if (freq > 4900) {
            return (freq - 4000) / 5;
        } else {
            return 15 + ((freq - 2512) / 20);
        }
    }
    return (freq - 5000) / 5;
}

/**
 * reg_is_6ghz_supported_mode() - Check if 6GHZ channels support
 * the given phymode and flag combination.
 * @chan_flags: Channel flags
 *
 * Return: True if channel mode is 11AX/11BE. False otherwise.
 *
 */
#ifdef WLAN_FEATURE_11BE
static bool reg_is_6ghz_supported_mode(uint64_t chan_flags)
{
    if(!((chan_flags == IEEE80211_CHAN_HE20) ||
         (chan_flags   == IEEE80211_CHAN_HE40MINUS) ||
         (chan_flags   == IEEE80211_CHAN_HE40PLUS) ||
         (chan_flags   == IEEE80211_CHAN_HE80) ||
         (chan_flags   == IEEE80211_CHAN_HE80_80) ||
         (chan_flags   == IEEE80211_CHAN_HE160) ||
         (chan_flags   == IEEE80211_CHAN_EHT20) ||
         (chan_flags   == IEEE80211_CHAN_EHT40PLUS) ||
         (chan_flags   == IEEE80211_CHAN_EHT40MINUS) ||
         (chan_flags   == IEEE80211_CHAN_EHT80) ||
         (chan_flags   == IEEE80211_CHAN_EHT160)||
         (chan_flags   == IEEE80211_CHAN_EHT320 )))
        return false;
    else
        return true;

}
#else
static bool reg_is_6ghz_supported_mode(uint64_t chan_flags)
{
    if(!((chan_flags == IEEE80211_CHAN_HE20) ||
         (chan_flags   == IEEE80211_CHAN_HE40MINUS) ||
         (chan_flags   == IEEE80211_CHAN_HE40PLUS) ||
         (chan_flags   == IEEE80211_CHAN_HE80) ||
         (chan_flags   == IEEE80211_CHAN_HE80_80) ||
         (chan_flags   == IEEE80211_CHAN_HE160)))
        return false;
    else
        return true;

}
#endif

void
ieee80211_set_chan_flags(struct ieee80211_ath_channel *chan,
			 enum channel_state state1,
			 enum channel_state state2,
			 qdf_freq_t freq)
{
    if (state1 == CHANNEL_STATE_DFS) {
	chan->ic_flags |= IEEE80211_CHAN_PASSIVE;
	if (WLAN_REG_IS_5GHZ_CH_FREQ(freq)) {
	    chan->ic_flags &= ~IEEE80211_CHAN_DFS_RADAR;
	    chan->ic_flagext |= IEEE80211_CHAN_DFS;
	    chan->ic_flagext &= ~IEEE80211_CHAN_DFS_RADAR_FOUND;
	}
    }

    if (state2 == CHANNEL_STATE_DFS)
	chan->ic_flagext |= IEEE80211_CHAN_DFS_CFREQ2;
    else
	chan->ic_flagext &= ~IEEE80211_CHAN_DFS_CFREQ2;

    /* Check for ad-hoc allowableness */
    /* To be done: DISALLOW_ADHOC_11A_TURB should allow ad-hoc */
    if (chan->ic_flagext & IEEE80211_CHAN_DFS ||
	chan->ic_flagext & IEEE80211_CHAN_DFS_CFREQ2) {
	chan->ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC;
    }

    if (WLAN_REG_IS_6GHZ_PSC_CHAN_FREQ(freq)) {
	chan->ic_flagext |= IEEE80211_CHAN_PSC;
    }

}

enum channel_state regdmn_reg_get_bonded_channel_state_for_freq(
        struct wlan_objmgr_pdev *pdev,
        qdf_freq_t freq,
        qdf_freq_t sec_ch_freq_2g,
        struct ch_params *ch_params)
{
        if (WLAN_REG_IS_5GHZ_CH_FREQ(freq))
            return ieee80211_regdmn_get_5g_bonded_channel_state_for_freq(pdev,
                                                                         freq,
                                                                         ch_params);
        else if  (WLAN_REG_IS_24GHZ_CH_FREQ(freq))
                  return ieee80211_regdmn_get_2g_bonded_channel_state_for_freq(pdev, freq, sec_ch_freq_2g, ch_params->ch_width);

    return CHANNEL_STATE_INVALID;
}

#define FREQ_OFFSET_80MHZ 80
#define CHAN_80MHZ_NUM QDF_ARRAY_SIZE(mhz80_5g_chan_list)
#define CHAN_80MHZ_NUM_5G QDF_ARRAY_SIZE(mhz80_5g_freq_list)

#ifdef CONFIG_BAND_6GHZ
#define CHAN_80MHZ_NUM_6G QDF_ARRAY_SIZE(mhz80_6g_freq_list)
#else
#define CHAN_80MHZ_NUM_6G 0
#endif

const uint16_t mhz80_5g_chan_list[] = {
    42,
    58,
    106,
    122,
    138,
    155
};

const uint16_t mhz80_5g_freq_list[] = {
    5210, /* 42 */
    5290, /* 58 */
    5530, /* 106 */
    5610, /* 122 */
    5690, /* 138 */
    5775, /* 155 */
};
#ifdef CONFIG_BAND_6GHZ
const uint16_t mhz80_6g_chan_list[] = {
    7 ,
    23,
    39,
    55,
    71,
    87,
    103,
    119,
    135,
    151,
    167,
    183,
    199,
    215,
};

const uint16_t mhz80_6g_freq_list[] = {
    5985, /* 7 */
    6065, /* 23 */
    6145, /* 39 */
    6225, /* 55 */
    6305, /* 71 */
    6385, /* 87 */
    6465, /* 103 */
    6545, /* 119 */
    6625, /* 135 */
    6705, /* 151 */
    6785, /* 167 */
    6865, /* 183 */
    6945, /* 199 */
    7025, /* 215 */
};
#endif

#ifdef WLAN_FEATURE_11BE
static bool regdmn_is_cm_bw_320(const struct reg_cmode *cm)
{
    if (cm->bw == CH_WIDTH_320MHZ)
        return true;
    else
        return false;
}
#else
static bool regdmn_is_cm_bw_320(const struct reg_cmode *cm)
{
    return false;
}
#endif

uint8_t
regdmn_get_band_cap_from_op_class(uint8_t num_of_opclass,
                                  const uint8_t *opclass)
{
    uint8_t  supported_band;
    uint8_t country_iso[REG_ALPHA2_LEN + 1] = {};

    country_iso[2] = OP_CLASS_GLOBAL;
    supported_band = wlan_reg_get_band_cap_from_op_class(country_iso,
                                                         num_of_opclass,
                                                         opclass);

    if (!supported_band) {
        qdf_err("None of the Operating class is found");
        return supported_band;
    }

    if (supported_band & BIT(REG_BAND_2G))
        supported_band |= BIT(IEEE80211_2G_BAND);
    else if (supported_band & BIT(REG_BAND_5G))
             supported_band |= BIT(IEEE80211_5G_BAND);
    else if (supported_band & BIT(REG_BAND_6G))
             supported_band |= BIT(IEEE80211_6G_BAND);
    else
             qdf_err("Unknown band %X", supported_band);

    return supported_band;
}

/**
 * regdmn_get_min_6ghz_chan_freq - Retrieve the minimum 6G channel frequency
 *
 * Return - Return the 6G minimum channel frequency if found, else zero.
 */
uint16_t regdmn_get_min_6ghz_chan_freq(void)
{
    return wlan_reg_min_6ghz_chan_freq();
}
qdf_export_symbol(regdmn_get_min_6ghz_chan_freq);

/**
 * regdmn_get_max_6ghz_chan_freq - Retrieve the maximum 6G channel frequency
 *
 * Return - Return the 6G maximum channel frequency if found, else zero.
 */
uint16_t regdmn_get_max_6ghz_chan_freq(void)
{
    return wlan_reg_max_6ghz_chan_freq();
}
qdf_export_symbol(regdmn_get_max_6ghz_chan_freq);

/**
 * regdmn_get_min_5ghz_chan_freq - Retrieve the minimum 5G channel frequency
 *
 * Return - Return the 5G minimum channel frequency if found, else zero.
 */
uint16_t regdmn_get_min_5ghz_chan_freq(void)
{
    return wlan_reg_min_5ghz_chan_freq();
}
qdf_export_symbol(regdmn_get_min_5ghz_chan_freq);

/**
 * regdmn_get_max_5ghz_chan_freq - Retrieve the maximum 5G channel frequency
 *
 * Return - Return the 5G maximum channel frequency if found, else zero.
 */
uint16_t regdmn_get_max_5ghz_chan_freq(void)
{
    return wlan_reg_max_5ghz_chan_freq();
}
qdf_export_symbol(regdmn_get_max_5ghz_chan_freq);

void regdmn_populate_channel_list_from_map(const struct reg_dmn_op_class_map_t *map,
                                  u_int8_t reg_class, struct ieee80211_node *ni) {
    uint8_t chanidx = 0;

    if(!map)
        return;

    while (map->op_class) {
        if (map->op_class == reg_class) {
            for (chanidx = 0; chanidx < MAX_CHANNELS_PER_OPERATING_CLASS &&
                                map->channels[chanidx] != 0; chanidx++) {
                IEEE80211_MBO_CHAN_BITMAP_SET(ni->ni_supp_op_cl.channels_supported, map->channels[chanidx]);
            }
            ni->ni_supp_op_cl.num_chan_supported += chanidx;
            break;
        }
        map++;
    }
    return;
}

void
regdmn_get_channel_list_from_op_class(uint8_t reg_class,
                                      struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    uint8_t c_idx = 0;
    uint8_t idx = 0, n_opclasses = 0, max_supp_op_class = REG_MAX_SUPP_OPER_CLASSES;
    struct regdmn_ap_cap_opclass_t *reg_ap_cap =  NULL;
    const struct reg_dmn_op_class_map_t *map =  NULL;
    bool global_tbl_lookup = true;
    QDF_STATUS status;

    reg_ap_cap = qdf_mem_malloc(max_supp_op_class * sizeof(*reg_ap_cap));
    if(reg_ap_cap == NULL) {
        qdf_err("Malloc failed for reg_ap_cap");
        return;
    }

    /* Get the regdmn map corresponding to country code */
    status = wlan_reg_get_opclass_details(ic->ic_pdev_obj, reg_ap_cap,
                                          &n_opclasses, max_supp_op_class,
                                          global_tbl_lookup,
                                          REG_CURRENT_PWR_MODE);

    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get opclass details");
        qdf_mem_free(reg_ap_cap);
        return;
    }

    /* Update the supported channel list for the peer based on intersection
     * between AP's and STA's Supported Operating Class list.
     */
    while (AP_CAP.op_class && (idx < n_opclasses)) {
        if (AP_CAP.op_class == reg_class) {
            while(c_idx < AP_CAP.num_supported_chan) {
                IEEE80211_MBO_CHAN_BITMAP_SET(
                    ni->ni_supp_op_cl.channels_supported,
                    AP_CAP.sup_chan_list[c_idx]);
                c_idx++;
            }

            c_idx = 0;
            while(c_idx < AP_CAP.num_non_supported_chan) {
                IEEE80211_MBO_CHAN_BITMAP_SET(
                    ni->ni_supp_op_cl.channels_supported,
                    AP_CAP.non_sup_chan_list[c_idx]);
                c_idx++;
            }
        }
        idx++;
    }

    wlan_reg_get_opclass_from_map(&map, global_tbl_lookup);

    /* Check for global operating class and mark those channels also as true to
     * pass MBO related testcase.
     */
    regdmn_populate_channel_list_from_map(map, reg_class, ni);
    qdf_mem_free(reg_ap_cap);

    return;
}

uint8_t regdmn_get_opclass(uint8_t *country_iso,
                           struct ieee80211_ath_channel *channel)
{

    uint16_t chan_width, behav_limit;
    uint8_t opclass = 0;

    wlan_get_bw_and_behav_limit(channel, &chan_width, &behav_limit);
    opclass = wlan_reg_get_opclass_from_freq_width(country_iso,
                                                   channel->ic_freq,
                                                   chan_width,
                                                   behav_limit);

     if (!opclass)
         qdf_err("Operating class is 0");

     return opclass;
}

#define CHAN_TO_FREQ_SCALE 5

static uint16_t
regdmn_chan_to_freq(struct regdmn_ap_cap_opclass_t *reg_ap_cap, uint8_t chan_num, uint8_t idx)
{
    return reg_ap_cap[idx].start_freq + (chan_num * CHAN_TO_FREQ_SCALE);
}

static void regdmn_fill_hwcap(struct wlan_radio_basic_capabilities *hwcap,
                              struct regdmn_ap_cap_opclass_t *reg_ap_cap,
                              uint8_t idx,
                              uint8_t *total_n_sup_opclass)
{
    HW_OP_CLASS.opclass = AP_CAP.op_class;
    HW_OP_CLASS.max_tx_pwr_dbm = AP_CAP.max_tx_pwr_dbm;
    HW_OP_CLASS.num_non_oper_chan = AP_CAP.num_non_supported_chan;
    qdf_mem_copy(HW_OP_CLASS.non_oper_chan_num,
                 AP_CAP.non_sup_chan_list,
                 AP_CAP.num_non_supported_chan);
    hwcap->wlan_radio_basic_capabilities_valid = 1;
    (*total_n_sup_opclass)++;
}

/**
 * regdmn_fill_apcap() - Fill apcap structure from reg_ap_cap.
 * @apcap: Pointer to wlan_radio_basic_capabilities
 * @reg_ap_cap: Pointer to regdmn_ap_cap_opclass_t
 * @idx: Index
 * @total_n_sup_opclass: Number of supported opclass
 *
 * Return - None.
 */
static void regdmn_fill_apcap(wlan_radio_basic_capabilities *hwcap,
                              struct regdmn_ap_cap_opclass_t *reg_ap_cap,
                              uint8_t idx,
                              uint8_t *total_n_sup_opclass)
{
    HW_OP_CLASS.opclass = AP_CAP.op_class;
    HW_OP_CLASS.max_tx_pwr_dbm = AP_CAP.max_tx_pwr_dbm;
    HW_OP_CLASS.num_non_oper_chan = AP_CAP.num_non_supported_chan;
    qdf_mem_copy(HW_OP_CLASS.non_oper_chan_num,
                 AP_CAP.non_sup_chan_list,
                 AP_CAP.num_non_supported_chan);
    hwcap->wlan_radio_basic_capabilities_valid = 1;
    (*total_n_sup_opclass)++;
}

#define EHT320_FREQ_OFFSET 150
#define VHT160_FREQ_OFFSET 70
#define VHT80_FREQ_OFFSET 30
#define VHT20_FREQ_OFFSET 0

/**
 * regdmn_find_leftmost_primary_freq_for_center() - Find a HT20
 * primary frequency given a center frequency and channel width.
 * @center_freq: Center frequency in MHZ
 * @ch_width: Channel width
 *
 * Return: Primary frequency in MHZ.
 */
static qdf_freq_t
regdmn_find_leftmost_primary_freq_for_center(qdf_freq_t center_freq,
                                            uint8_t ch_width)
{
    uint8_t freq_offset = VHT20_FREQ_OFFSET;
    qdf_freq_t primary_freq;

    switch(ch_width) {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320:
            freq_offset = EHT320_FREQ_OFFSET;
#endif
        case IEEE80211_CWM_WIDTH160:
            freq_offset = VHT160_FREQ_OFFSET;
            break;
        case IEEE80211_CWM_WIDTH80:
            freq_offset = VHT80_FREQ_OFFSET;
            break;
        case IEEE80211_CWM_WIDTH40:
        case IEEE80211_CWM_WIDTH20:
        default:
            freq_offset = VHT20_FREQ_OFFSET;
            break;
    }

    primary_freq = center_freq - freq_offset;
    return primary_freq;
}

static void regdmn_fill_op_chan(struct wlan_objmgr_pdev *pdev,
                                    struct wlan_op_chan *op_chan,
                                    struct regdmn_ap_cap_opclass_t *reg_ap_cap,
                                    uint8_t idx,
                                    uint8_t *total_n_sup_opclass,
                                    bool dfs_required)
{
    if (dfs_required) {
        uint8_t chan_idx = 0, i = 0;

        MAP_OP_CHAN.opclass = AP_CAP.op_class;
        switch(AP_CAP.ch_width) {
               case BW_20_MHZ:
                              MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH20;
                              break;
               case BW_25_MHZ:
                              MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH20;
                              break;
               case BW_40_MHZ:
                              MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH40;
                              break;
               case BW_80_MHZ:
                              if (AP_CAP.behav_limit == BIT(BEHAV_BW80_PLUS))
                                  MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH80_80;
                              else
                                  MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH80;

                              break;
              case BW_160_MHZ:
                              MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH160;
                              break;
#ifdef WLAN_FEATURE_11BE
              case BW_320_MHZ:
                              MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTH320;
                              break;
#endif
              default:
                      MAP_OP_CHAN.ch_width = IEEE80211_CWM_WIDTHINVALID;
                      break;
       }

       while (AP_CAP.sup_chan_list[chan_idx]) {
              qdf_freq_t primary_freq;
              qdf_freq_t search_freq =
              regdmn_chan_to_freq(reg_ap_cap,
                                  AP_CAP.
                                  sup_chan_list[chan_idx],
                                  idx);

              primary_freq =
                  regdmn_find_leftmost_primary_freq_for_center(search_freq,
                                                               MAP_OP_CHAN.ch_width);

              if (wlan_reg_is_freq_width_dfs(pdev, primary_freq,
                                             MAP_OP_CHAN.ch_width))
                  MAP_OP_CHAN.oper_chan_num[i++] =
                      AP_CAP.sup_chan_list[chan_idx++];
              else
                  chan_idx++;

       }

       MAP_OP_CHAN.num_oper_chan = i;

       if (i)
           (*total_n_sup_opclass)++;

    } else if (AP_CAP.op_class == op_chan->opclass) {
               op_chan->ch_width = AP_CAP.ch_width;
               switch(AP_CAP.ch_width) {
                      case BW_20_MHZ:
                                     op_chan->ch_width =
                                                          IEEE80211_CWM_WIDTH20;
                                     break;
                      case BW_25_MHZ:
                                     op_chan->ch_width =
                                                          IEEE80211_CWM_WIDTH20;
                                     break;
                      case BW_40_MHZ:
                                     op_chan->ch_width =
                                                         IEEE80211_CWM_WIDTH40;
                                     break;
                      case BW_80_MHZ:
                                     if (AP_CAP.behav_limit == BIT(BEHAV_BW80_PLUS))
                                         op_chan->ch_width =
                                                       IEEE80211_CWM_WIDTH80_80;
                                     else
                                         op_chan->ch_width =
                                                          IEEE80211_CWM_WIDTH80;
                                     break;
                      case BW_160_MHZ:
                                      op_chan->ch_width =
                                                         IEEE80211_CWM_WIDTH160;
                                      break;
#ifdef WLAN_FEATURE_11BE
                      case BW_320_MHZ:
                                      op_chan->ch_width =
                                                         IEEE80211_CWM_WIDTH320;
                                      break;
#endif
                      default:
                              op_chan->ch_width = IEEE80211_CWM_WIDTHINVALID;
                              break;
                }

                op_chan->num_oper_chan = AP_CAP.num_supported_chan;
                qdf_mem_copy(op_chan->oper_chan_num,
                             AP_CAP.sup_chan_list,
                             AP_CAP.num_supported_chan);
    }
}

static void regdmn_fill_op_class(struct wlan_op_class *op_class,
                                       struct regdmn_ap_cap_opclass_t *reg_ap_cap,
                                       uint8_t idx)
{
    uint8_t chan_idx = 0, i = 0;

    if (op_class->opclass != AP_CAP.op_class)
        return;

    switch(AP_CAP.ch_width) {
           case BW_20_MHZ:
                   op_class->ch_width = IEEE80211_CWM_WIDTH20;
                   break;
           case BW_25_MHZ:
                   op_class->ch_width = IEEE80211_CWM_WIDTH20;
                   break;
           case BW_40_MHZ:
                   op_class->ch_width = IEEE80211_CWM_WIDTH40;
                   break;
           case BW_80_MHZ:
                   if (AP_CAP.behav_limit == BIT(BEHAV_BW80_PLUS))
                       op_class->ch_width = IEEE80211_CWM_WIDTH80_80;
                   else
                       op_class->ch_width = IEEE80211_CWM_WIDTH80;
                   break;
           case BW_160_MHZ:
                   op_class->ch_width = IEEE80211_CWM_WIDTH160;
                   break;
#ifdef WLAN_FEATURE_11BE
           case BW_320_MHZ:
                   op_class->ch_width = IEEE80211_CWM_WIDTH320;
                   break;
#endif
           default:
                   op_class->ch_width = IEEE80211_CWM_WIDTHINVALID;
                   break;
    }

    switch(AP_CAP.behav_limit) {
           case BIT(BEHAV_NONE):
                                op_class->sc_loc =
                                                 IEEE80211_SEC_CHAN_OFFSET_SCN;
                                break;
            case BIT(BEHAV_BW40_LOW_PRIMARY):
                                op_class->sc_loc =
                                                  IEEE80211_SEC_CHAN_OFFSET_SCA;
                                break;
            case BIT(BEHAV_BW40_HIGH_PRIMARY):
                                op_class->sc_loc =
                                                  IEEE80211_SEC_CHAN_OFFSET_SCB;
                                break;
            case BIT(BEHAV_BW80_PLUS):
                                op_class->sc_loc =
                                                  IEEE80211_SEC_CHAN_OFFSET_SCN;
                                break;
            default:
                    op_class->sc_loc = IEEE80211_SEC_CHAN_OFFSET_SCN;
            break;
    }

    op_class->num_chan = AP_CAP.num_supported_chan + AP_CAP.num_non_supported_chan;

    while(chan_idx < AP_CAP.num_supported_chan)
          op_class->channels[i++] = AP_CAP.sup_chan_list[chan_idx++];
    chan_idx = 0;

    while(chan_idx < AP_CAP.num_non_supported_chan)
          op_class->channels[i++] = AP_CAP.non_sup_chan_list[chan_idx++];

}

/**
 * @brief Populate AP capabilities with opclass and operable channels for MultiAP
 *
 * @param pdev Pointer to pdev
 * @param apcap Pointer to structure to populate AP capabilities
 * @param map_op_chan Pointer to structure to populate operable channel
 * @param map_op_class Pointer to structure to populate operating class
 * @param global_tbl_lookup Whether to lookup global op class tbl
 *
 * @return Total number of operating opclass, else return 0 in case of failure
 */
uint8_t regdmn_get_map_opclass(struct wlan_objmgr_pdev *pdev,
                               struct wlan_radio_basic_capabilities *hwcap,
                               struct wlan_op_chan *op_chan,
                               struct wlan_op_class *op_class,
                               bool global_tbl_lookup,
                               bool dfs_required)
{
    uint8_t total_n_sup_opclass = 0;
    uint8_t idx = 0, n_opclasses = 0, max_supp_op_class = REG_MAX_SUPP_OPER_CLASSES;
    struct regdmn_ap_cap_opclass_t *reg_ap_cap =  NULL;
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);
    QDF_STATUS status;

    if (!ic) {
        qdf_err("ic is NULL");
        return 0;
    }

    reg_ap_cap = qdf_mem_malloc(max_supp_op_class * sizeof(*reg_ap_cap));

    if (!reg_ap_cap) {
        qdf_err("Failed to allocate reg_ap_cap");
        return 0;
    }

    status = wlan_reg_get_opclass_details(pdev, reg_ap_cap, &n_opclasses,
                                          max_supp_op_class,
                                          global_tbl_lookup,
                                          REG_CURRENT_PWR_MODE);

    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get opclass details");
        return 0;
    }
    /* Only one of 'hwcap' and 'op_chan' is expected to be not-NULL at a time*/
    if (hwcap && op_chan) {
        qdf_err("Both hwcap and op_chan should not be allocated at the same time");
        return 0;
    }

    while (AP_CAP.op_class && (idx < n_opclasses)) {
           bool is_160_supported = false;
           bool is_80p80_supported = false;
#ifdef WLAN_FEATURE_11BE
           bool is_320_supported = false;
#endif

           if (ic->ic_modecaps & ((1 << IEEE80211_MODE_11AC_VHT160) |
               (1 << IEEE80211_MODE_11AXA_HE160)
#ifdef WLAN_FEATURE_11BE
               | (1ULL << IEEE80211_MODE_11BEA_EHT160)
#endif
               ))
                is_160_supported = true;

           if (ic->ic_modecaps & ((1 << IEEE80211_MODE_11AC_VHT80_80) |
               (1ULL << IEEE80211_MODE_11AXA_HE80_80)))
                is_80p80_supported = true;

#ifdef WLAN_FEATURE_11BE
           if (ic->ic_modecaps & (1ULL << IEEE80211_MODE_11BEA_EHT320))
               is_320_supported = true;
#endif

           /* check radio capability and skip unsupported channel width */
           if (((AP_CAP.ch_width == BW_160_MHZ) && !(is_160_supported)) ||
               ((AP_CAP.behav_limit == BIT(BEHAV_BW80_PLUS)) && !(is_80p80_supported))
#ifdef WLAN_FEATURE_11BE
               || (AP_CAP.ch_width == BW_320_MHZ && !is_320_supported)
#endif
              ) {
               idx++;
               continue;
           }

           if (hwcap != NULL) {
               regdmn_fill_hwcap(hwcap, reg_ap_cap, idx, &total_n_sup_opclass);
               hwcap->num_supp_op_classes = total_n_sup_opclass;
           } else if (op_chan != NULL) {
               regdmn_fill_op_chan(pdev, op_chan, reg_ap_cap, idx,
                                       &total_n_sup_opclass, dfs_required);
               op_chan->num_supp_op_classes = total_n_sup_opclass;
           }

           if (op_class != NULL) {
               regdmn_fill_op_class(op_class, reg_ap_cap, idx);
           }
           idx++;
    }

    qdf_mem_free(reg_ap_cap);
    return total_n_sup_opclass;
}

/**
 * regdmn_convert_chanflags_to_chanwidth() - Convert chan flag to channel width
 * @chan: Pointer to current channel.
 *
 * Return: Channel width.
 */
uint32_t regdmn_convert_chanflags_to_chanwidth(
        struct ieee80211_ath_channel *chan)
{
    uint32_t chanwidth = CH_WIDTH_INVALID;

    if (IEEE80211_IS_CHAN_ST(chan) ||
            IEEE80211_IS_CHAN_A(chan) ||
            IEEE80211_IS_CHAN_B(chan) ||
            IEEE80211_IS_CHAN_PUREG(chan) ||
            IEEE80211_IS_CHAN_20MHZ(chan))
        chanwidth = CH_WIDTH_20MHZ;
    else if (IEEE80211_IS_CHAN_40MHZ(chan))
        chanwidth = CH_WIDTH_40MHZ;
    else if (IEEE80211_IS_CHAN_80MHZ(chan))
        chanwidth = CH_WIDTH_80MHZ;
    else if (IEEE80211_IS_CHAN_160MHZ(chan))
        chanwidth = CH_WIDTH_160MHZ;
    else if (IEEE80211_IS_CHAN_80_80MHZ(chan))
        chanwidth = CH_WIDTH_80P80MHZ;
#ifdef WLAN_FEATURE_11BE
    else if (IEEE80211_IS_CHAN_320MHZ(chan))
        chanwidth = CH_WIDTH_320MHZ;
#endif

    return chanwidth;
}

/**
 * regdmn_get_sec_ch_offset() - Get second channel offset.
 * @chan: Pointer to current channel.
 *
 * Return: Second channel offset.
 */
static inline uint8_t regdmn_get_sec_ch_offset(
        struct ieee80211_ath_channel *chan)
{
    uint8_t sec_ch_offset = 0;

    if (IEEE80211_IS_CHAN_40PLUS(chan))
        sec_ch_offset = chan->ic_ieee + CHAN_DIFF;
    else if (IEEE80211_IS_CHAN_40MINUS(chan))
        sec_ch_offset = chan->ic_ieee - CHAN_DIFF;

    return sec_ch_offset;
}

void ieee80211_regdmn_get_des_chan_params(struct ieee80211vap *vap,
        struct ch_params *ch_params)
{
    struct ieee80211_ath_channel *chan;
    chan = vap->iv_des_chan[vap->iv_des_hw_mode];
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        ch_params->ch_width = ieee80211_get_chan_width_from_phymode(vap->iv_des_hw_mode);
        ch_params->center_freq_seg0 = chan->ic_vhtop_ch_num_seg1;
        ch_params->center_freq_seg1 = chan->ic_vhtop_ch_num_seg2;
        ch_params->mhz_freq_seg0 = chan->ic_vhtop_freq_seg1;
        ch_params->mhz_freq_seg1 = chan->ic_vhtop_freq_seg2;
        ch_params->sec_ch_offset = regdmn_get_sec_ch_offset(chan);
    } else {
        ieee80211_regdmn_get_chan_params(vap->iv_ic, ch_params);
    }
}

void ieee80211_regdmn_get_chan_params(struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    ch_params->ch_width = regdmn_convert_chanflags_to_chanwidth(ic->ic_curchan);
    ch_params->center_freq_seg0 = ic->ic_curchan->ic_vhtop_ch_num_seg1;
    ch_params->center_freq_seg1 = ic->ic_curchan->ic_vhtop_ch_num_seg2;
    ch_params->mhz_freq_seg0 = ic->ic_curchan->ic_vhtop_freq_seg1;
    ch_params->mhz_freq_seg1 = ic->ic_curchan->ic_vhtop_freq_seg2;
    ch_params->sec_ch_offset = regdmn_get_sec_ch_offset(ic->ic_curchan);
}

/**
 * ieee80211_convert_width_to_11ngflags() - Convert channel width to 11NG flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11NG flags.
 */
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11ngflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11NG_HT20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11NG_HT40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11NG_HT40PLUS;
            break;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}

/**
 * ieee80211_convert_width_to_11naflags() - Convert channel width to 11NA flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11NA flags.
 */
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11naflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11NA_HT20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11NA_HT40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11NA_HT40PLUS;
            break;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}

/**
 * ieee80211_convert_width_to_11acflags() - Convert channel width to 11AC flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11AC flags.
 */
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11acflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11AC_VHT20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11AC_VHT40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11AC_VHT40PLUS;
            break;
        case CH_WIDTH_80MHZ:
            mode = IEEE80211_MODE_11AC_VHT80;
            break;
        case CH_WIDTH_160MHZ:
            mode = IEEE80211_MODE_11AC_VHT160;
            break;
        case CH_WIDTH_80P80MHZ:
            mode = IEEE80211_MODE_11AC_VHT80_80;
            break;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}

/**
 * ieee80211_convert_width_to_11axgflags() - Convert chan width to 11AXG flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11AXG flags.
 */
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11axgflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11AXG_HE20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11AXG_HE40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11AXG_HE40PLUS;
            break;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}

/**
 * ieee80211_convert_width_to_11begflags() - Convert chan width to 11BEG flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11BEG flags.
 */
#ifdef WLAN_FEATURE_11BE
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11begflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11BEG_EHT20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11BEG_EHT40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11BEG_EHT40PLUS;
            break;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}
#endif

/**
 * ieee80211_convert_width_to_11axaflags() - Convert chan width to 11AXA flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11AXA flags.
 */
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11axaflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11AXA_HE20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11AXA_HE40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11AXA_HE40PLUS;
            break;
        case CH_WIDTH_80MHZ:
            mode = IEEE80211_MODE_11AXA_HE80;
            break;
        case CH_WIDTH_160MHZ:
            mode = IEEE80211_MODE_11AXA_HE160;
            break;
        case CH_WIDTH_80P80MHZ:
            mode = IEEE80211_MODE_11AXA_HE80_80;
            break;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}

/**
 * ieee80211_convert_width_to_11beaflags() - Convert chan width to 11BEA flags.
 * @ic: Pointer to ieee80211com structure.
 * @ch_params: Pointer to channel params structure.
 *
 * Return: Return 11BEA flags.
 */
#ifdef WLAN_FEATURE_11BE
static inline enum ieee80211_phymode
ieee80211_convert_width_to_11beaflags(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;

    switch (ch_params->ch_width) {
        case CH_WIDTH_20MHZ:
            mode = IEEE80211_MODE_11BEA_EHT20;
            break;
        case CH_WIDTH_40MHZ:
            if (ch_params->sec_ch_offset == HIGH_PRIMARY_CH)
                mode = IEEE80211_MODE_11BEA_EHT40MINUS;
            else if (ch_params->sec_ch_offset == LOW_PRIMARY_CH)
                mode = IEEE80211_MODE_11BEA_EHT40PLUS;
            break;
        case CH_WIDTH_80MHZ:
            mode = IEEE80211_MODE_11BEA_EHT80;
            break;
        case CH_WIDTH_160MHZ:
            mode = IEEE80211_MODE_11BEA_EHT160;
            break;
        case CH_WIDTH_320MHZ:
            mode = IEEE80211_MODE_11BEA_EHT320;
        default:
            mode = IEEE80211_MODE_AUTO;
            break;
    }

    return mode;
}
#endif

enum ieee80211_phymode
ieee80211_get_target_channel_mode(
        struct ieee80211com *ic,
        struct ch_params *ch_params)
{
    enum ieee80211_phymode mode;

    if (IEEE80211_IS_CHAN_11NG(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11ngflags(ic, ch_params);
    else if (IEEE80211_IS_CHAN_11NA(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11naflags(ic, ch_params);
    else if (IEEE80211_IS_CHAN_11AC(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11acflags(ic, ch_params);
    else if (IEEE80211_IS_CHAN_11AXG(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11axgflags(ic, ch_params);
    else if (IEEE80211_IS_CHAN_11AXA(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11axaflags(ic, ch_params);
#ifdef WLAN_FEATURE_11BE
    else if (IEEE80211_IS_CHAN_11BEA(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11beaflags(ic, ch_params);
    else if (IEEE80211_IS_CHAN_11BEG(ic->ic_curchan))
        mode = ieee80211_convert_width_to_11begflags(ic, ch_params);
#endif
    else
        mode = IEEE80211_MODE_AUTO;

    return mode;
}

#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
int ieee80211_dfs_rebuild_chan_list_with_non_dfs_channels(struct ieee80211com *ic)
{
    int num_non_dfs_chans = 0;

    if (ic->ic_rebuilt_chanlist == 1) {
        qdf_print("%s: Channel list already rebuilt, avoiding another iteration",__func__);
        return 0;
    }
    /* Iterate through the ic_nchans and find num of non-DFS chans.*/
    num_non_dfs_chans = ieee80211_is_non_dfs_chans_available(ic);

    if (num_non_dfs_chans == 0) {
        qdf_print("Current country: 0x%x does not support any"
                "non-DFS channels",ic->ic_country.countryCode);
        return 1;
    }
    /* Ensure that we save the current channel's flags (HT/VHT mode) before
     * we do a memzero of ic channel list. This is to replenish to the same
     * mode after re-starting vaps with non-DFS channels.
     */
    ic->ic_curchan_flags = (ic->ic_curchan->ic_flags) & IEEE80211_CHAN_ALL;
    ic->ic_tmp_ch_width = regdmn_convert_chanflags_to_chanwidth(ic->ic_curchan);
    ic->ic_tmp_center_freq_seg0 = ic->ic_curchan->ic_vhtop_ch_num_seg1;
    ic->ic_tmp_center_freq_seg1 = ic->ic_curchan->ic_vhtop_ch_num_seg2;
    ic->ic_tmp_sec_ch_offset = regdmn_get_sec_ch_offset(ic->ic_curchan);

    ucfg_reg_enable_dfs_channels(ic->ic_pdev_obj, false);
    *ic->ic_curchan = ieee80211_ath_get_first_channel(ic);
    if (!IEEE80211_IS_CHAN_VALID(ic->ic_curchan)) {
        *ic->ic_curchan = empty_channel;
        return 1;
    }
    ic->ic_rebuilt_chanlist = 1;
    ic->ic_tempchan = 1;
    return 0;
}
qdf_export_symbol(ieee80211_dfs_rebuild_chan_list_with_non_dfs_channels);
#endif /* HOST_DFS_SPOOF_TEST */

static int bw_val[CH_WIDTH_MAX+1] = {
    20,  /* CH_WIDTH_20MHZ */
    40,  /* CH_WIDTH_40MHZ */
    80,  /* CH_WIDTH_80MHZ */
    160, /* CH_WIDTH_160MHZ */
    80,  /* CH_WIDTH_80P80MHZ */
    5,   /* CH_WIDTH_5MHZ */
    10,  /* CH_WIDTH_10MHZ */
#ifdef WLAN_FEATURE_11BE
    320, /* CH_WIDTH_320MHZ */
#endif /* WLAN_FEATURE_11BE*/
    0,   /* CH_WIDTH_INVALID */
    0,   /* CH_WIDTH_MAX */
};

int ieee80211_get_chwidth_val(enum phy_ch_width ch_width)
{
    return bw_val[ch_width];
}

/**
 * ieee80211_is_80p80mhz_supported() - Find if 80p80mhz mode is supported
 * by the frequency and the radio.
 * @ic: Pointer to ieee80211com.
 * @primary_freq: Primary frequency to check for 80p80 supported.
 * @sec_cfreq: Output secondary center frequency.
 * @sec_cieee: Output secondary center IEEE channel.
 *
 * Return true if there exists another 80MHz non contiguous channel for the
 * given frequency, else false.
 */
bool ieee80211_is_80p80mhz_supported(struct ieee80211com *ic,
                                     qdf_freq_t primary_freq,
                                     qdf_freq_t *sec_cfreq,
                                     uint8_t *sec_cieee,
                                     bool include_nol_chan,
                                     enum supported_6g_pwr_types in_6g_pwr_mode)
{
    struct ch_params tmp_ch_params = {0};
    int loop, index = 0;
    bool found_80p80mhz = false;
    struct wlan_objmgr_psoc *psoc;
    const uint16_t *mhz80_freq_list, *mhz80_chan_list;
    struct wlan_lmac_if_reg_tx_ops *tx_ops;
    uint8_t phy_id, pdev_id;

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc) {
        qdf_err("%s : psoc is null", __func__);
        return found_80p80mhz;
    }

    tx_ops = wlan_reg_get_tx_ops(psoc);
    pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    if (tx_ops->get_phy_id_from_pdev_id)
        tx_ops->get_phy_id_from_pdev_id(psoc, pdev_id, &phy_id);
    else
        phy_id = pdev_id;

    /* 80P80 is not supported on WKK and other 11BE platforms */
    if (tx_ops->is_chip_11be(psoc, phy_id))
        return false;

#ifdef CONFIG_BAND_6GHZ
    if (WLAN_REG_IS_6GHZ_CHAN_FREQ(primary_freq)) {
        return found_80p80mhz;
    } else
#endif
        if (!(wlan_psoc_nif_fw_ext_cap_get(psoc,
                                           WLAN_SOC_RESTRICTED_80P80_SUPPORT))) {
            return found_80p80mhz;
        } else {
            loop = CHAN_80MHZ_NUM_5G;
            mhz80_freq_list = mhz80_5g_freq_list;
            mhz80_chan_list = mhz80_5g_chan_list;
        }

    while (loop--) {
        tmp_ch_params.center_freq_seg1 = mhz80_chan_list[index];
        tmp_ch_params.mhz_freq_seg1 = mhz80_freq_list[index];
        tmp_ch_params.ch_width = CH_WIDTH_80P80MHZ;
        index++;

        if (include_nol_chan)
            ieee80211_regdmn_get_channel_params(ic->ic_pdev_obj,
                                                primary_freq,
                                                0,
                                                &tmp_ch_params,
                                                in_6g_pwr_mode);
        else
            ieee80211_regdmn_set_channel_params_for_freq(ic->ic_pdev_obj,
                                                         primary_freq,
                                                         0,
                                                         &tmp_ch_params,
                                                         in_6g_pwr_mode);


        if (tmp_ch_params.ch_width != CH_WIDTH_80P80MHZ)
            continue;

        /*
         * If restricted 80p80 is enabled, check if the 80P80 channels
         * are within the 165MHz band, else do not select the 80P80 channel.
         */
        if (wlan_psoc_nif_fw_ext_cap_get(psoc,
                                         WLAN_SOC_RESTRICTED_80P80_SUPPORT) &&
            !(CHAN_WITHIN_RESTRICTED_80P80(tmp_ch_params.mhz_freq_seg0,
                                           tmp_ch_params.mhz_freq_seg1)))
            continue;

        /* Check if the centers are same or contiguous 80MHz centers. */
        if (abs(tmp_ch_params.mhz_freq_seg0 -
                tmp_ch_params.mhz_freq_seg1) <= 80)
            continue;

        found_80p80mhz = true;
        *sec_cieee = tmp_ch_params.center_freq_seg1;
        *sec_cfreq = tmp_ch_params.mhz_freq_seg1;
        break;
    }

    return found_80p80mhz;
}
qdf_export_symbol(ieee80211_is_80p80mhz_supported);

/**
 * ieee80211_is_ht40_ext_supported() - Find if the HT40 ext mode (Plus/Minus)
 * is supported by the channel.
 *
 * @ic: Pointer to ieee80211com.
 * @chan_ext: Channel extension flag (PLUS/MINUS).
 * @primary_freq: Primary_frequency to check.
 *
 * Return true if the given ext flag is supported, else false.
 */
static bool ieee80211_is_ht40_ext_supported(struct ieee80211com *ic,
                                            uint32_t chan_ext,
                                            qdf_freq_t primary_freq,
                                            enum supported_6g_pwr_types in_6g_pwr_mode)
{
    qdf_freq_t sec_ch_2g_freq = 0;
    struct ch_params tmp_ch_params = {0};

    tmp_ch_params.ch_width = CH_WIDTH_40MHZ;
    if (WLAN_REG_IS_24GHZ_CH_FREQ(primary_freq)) {
        switch (chan_ext) {
            case CHANNEL_40_PLUS:
                sec_ch_2g_freq = primary_freq + CHAN_HT40_OFFSET;
                break;
            case CHANNEL_40_MINUS:
                sec_ch_2g_freq = primary_freq - CHAN_HT40_OFFSET;
                break;
            default:
                return false;
        }
    }

    ieee80211_regdmn_get_channel_params(ic->ic_pdev_obj,
                                        primary_freq,
                                        sec_ch_2g_freq,
                                        &tmp_ch_params,
                                        in_6g_pwr_mode);

    if (tmp_ch_params.ch_width != CH_WIDTH_40MHZ)
        return false;

    if ((primary_freq < tmp_ch_params.mhz_freq_seg0) &&
        (chan_ext != CHANNEL_40_PLUS))
        return false;

    if ((primary_freq > tmp_ch_params.mhz_freq_seg0) &&
        (chan_ext != CHANNEL_40_MINUS))
        return false;

    return true;
}

static void ieee80211_set_max_supported_bw(struct ieee80211com *ic,
                                           qdf_freq_t primary_freq,
                                           struct regulatory_channel *reg_chan,
                                           enum supported_6g_pwr_types in_6g_pwr_mode)
{
    enum phy_ch_width ch_width = wlan_reg_find_chwidth_from_bw(reg_chan->max_bw);

    ch_width = wlan_reg_get_max_channel_width(ic->ic_pdev_obj,
                                              primary_freq,
                                              ch_width,
                                              in_6g_pwr_mode,
                                              NO_PUNCTURE_BITMASK);

    reg_chan->max_bw = wlan_reg_get_bw_value(ch_width);
}

void  wlan_reg_get_channel_flags(struct ieee80211com *ic,
                                 qdf_freq_t primary_freq,
                                 uint32_t *flags_ext,
                                 uint64_t *flags,
                                 enum supported_6g_pwr_types in_6g_pwr_mode,
                                 struct regulatory_channel *reg_chan_list)
{
    uint64_t t_flags = 0;
    uint16_t t_flags_ext = 0;

    wlan_reg_get_chan_flags(ic->ic_pdev_obj, primary_freq, 0,
                                &t_flags_ext, &t_flags, in_6g_pwr_mode,
                                reg_chan_list);

    if (t_flags & WLAN_CHAN_PASSIVE)
        *flags |= IEEE80211_CHAN_PASSIVE;

    if (t_flags_ext & WLAN_CHAN_DFS)
        *flags_ext |= IEEE80211_CHAN_DFS;

    if (t_flags_ext & WLAN_CHAN_PSC)
        *flags_ext |= IEEE80211_CHAN_PSC;

    if (t_flags_ext & WLAN_CHAN_DISALLOW_ADHOC)
        *flags_ext |= IEEE80211_CHAN_DISALLOW_ADHOC;

    if (t_flags_ext & WLAN_CHAN_DFS_CFREQ2)
        *flags_ext |= IEEE80211_CHAN_DFS_CFREQ2;
}

/**
 * ieee80211_set_modeflag_for_chan() - Get all supported flags
 * for the channel.
 * @ic: Pointer to ieee80211com.
 * @mode_supported: All modes supported by the device.
 * @chan_params: Channel params to be filled after getting supported modes.
 * @regchan: Regulatory channel information for the channel.
 * @flag_160: To fill 160/80MHz channel information.
 * @in_supp_pwr_type: Input 6g power type.
 *
 * Return: flags indicating all supported modes for the channel.
 */
static uint64_t
ieee80211_set_modeflag_for_chan(struct ieee80211com *ic,
                                uint64_t mode_supported,
                                struct ch_params *chan_params,
                                struct regulatory_channel *regchan,
                                bool flag_160,
                                uint64_t *chaninfo_flags,
                                enum supported_6g_pwr_types in_supp_pwr_type)
{
    const struct reg_cmode *cm;
    uint64_t flags = 0;
    qdf_freq_t primary_freq = regchan->center_freq;

    chan_params->ch_width = CH_WIDTH_INVALID;

    /* If regulatory says no OFDM, ignore all the flags filled. */
    if (regchan->chan_flags & REGULATORY_CHAN_NO_OFDM) {
        if (chaninfo_flags)
            *chaninfo_flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_B);
        return IEEE80211_CHAN_B;
    }

    for (cm = modes; cm < &modes[QDF_ARRAY_SIZE(modes)]; cm++) {
        /* Check if curr mode is supported by the device and regulatory. */
        if (!(cm->mode & mode_supported)) {
            continue;
        }

        /* Check if current mode is valid for the frequency to be filled. */
        if (WLAN_REG_IS_6GHZ_CHAN_FREQ(primary_freq) &&
            (!(cm->mode & WIRELESS_6G_MODES)))
            continue;

        if (WLAN_REG_IS_5GHZ_CH_FREQ(primary_freq) &&
            (!(cm->mode & WIRELESS_5G_MODES)))
            continue;

        if (WLAN_REG_IS_24GHZ_CH_FREQ(primary_freq) &&
            (!(cm->mode & WIRELESS_2G_MODES)))
            continue;

        if (WLAN_REG_IS_49GHZ_FREQ(primary_freq) &&
            (!(cm->mode & WIRELESS_49G_MODES)))
            continue;

        /* Check if the current bandwidth is supported by the frequency. */
        if (!BW_WITHIN(regchan->min_bw, bw_val[cm->bw], regchan->max_bw))
            continue;

        /* Find if any other valid 80MHz exists if bandwidth is 80p80MHZ. */
        if (cm->bw == CH_WIDTH_80P80MHZ) {
            if (!ieee80211_is_80p80mhz_supported(ic,
                                                 primary_freq,
                                                 &chan_params->mhz_freq_seg1,
                                                 &chan_params->center_freq_seg1,
                                                 true,
                                                 in_supp_pwr_type))
                continue;
        }

        /* See if the channel supports HT40PLUS or HT40MINUS. */
        if (cm->chan_ext != CHANNEL_40_NO) {
            if (!ieee80211_is_ht40_ext_supported(ic,
                                                 cm->chan_ext,
                                                 primary_freq,
                                                 in_supp_pwr_type))
                continue;
        }

        flags |= cm->flags;
        if (chaninfo_flags)
            *chaninfo_flags |= (uint64_t)wlan_get_channel_flags(cm->flags);

#ifndef WLAN_FEATURE_11BE
        /* Fill 160MHz channel information only if the flag is set. */
        if (flag_160 && cm->bw == CH_WIDTH_160MHZ)
            chan_params->ch_width = CH_WIDTH_160MHZ;

        /* Fill the channel information for the bw (maximum of 80/80p80). */
        if (!flag_160) {
            if (cm->bw == CH_WIDTH_80P80MHZ)
                chan_params->ch_width = CH_WIDTH_80P80MHZ;
            else if (cm->bw == CH_WIDTH_80MHZ &&
                     chan_params->ch_width != CH_WIDTH_80P80MHZ)
                chan_params->ch_width = CH_WIDTH_80MHZ;
        }
#endif
    }

    return flags;
}

/**
 * ieee80211_fill_athchan() - Fill the ath channel structure based on the
 * regulatory channel information.
 * @regchan: Regulatory channel information.
 * @chan_list: Ath channel structure to be filled.
 * @min_tx_power: Minimum tx power for the channel.
 * @max_tx_power: Maximum tx power for the channel.
 * @flags: Flags indicating supported modes by the channel.
 */
static void
ieee80211_fill_athchan(struct regulatory_channel *regchan,
                       struct ieee80211_ath_channel *chan_list,
                       int8_t min_tx_power,
                       int8_t max_tx_power,
                       uint64_t flags)
{
    chan_list->ic_ieee = regchan->chan_num;
    chan_list->ic_freq = regchan->center_freq;
    chan_list->ic_maxregpower = regchan->tx_power;
    chan_list->ic_antennamax = regchan->ant_gain;
    chan_list->ic_minpower = min_tx_power;
    chan_list->ic_maxpower = max_tx_power;
    chan_list->ic_flags = flags;
}

/**
 * ieee80211_set_seg_centers() - Fill the ath channel structure with segment
 * center IEEEs and frequencies.
 * @ic: Pointer to ieee80211com.
 * @chan_params: Channel params that will have the channel mode to be filled.
 * @chan_list: Ath channel structure to be filled.
 * @primary_freq: Primary frequency of the channel.
 */
static void
ieee80211_set_seg_centers(struct ieee80211com *ic,
                          struct ch_params *chan_params,
                          struct ieee80211_ath_channel *chan_list,
                          qdf_freq_t primary_freq,
                          enum supported_6g_pwr_types ap_pwr_type)
{
    ieee80211_regdmn_get_channel_params(ic->ic_pdev_obj,
                                        primary_freq,
                                        0,
                                        chan_params,
                                        ap_pwr_type);
    chan_list->ic_vhtop_ch_num_seg1 = chan_params->center_freq_seg0;
    chan_list->ic_vhtop_ch_num_seg2 = chan_params->center_freq_seg1;
    chan_list->ic_vhtop_freq_seg1 = chan_params->mhz_freq_seg0;
    chan_list->ic_vhtop_freq_seg2 = chan_params->mhz_freq_seg1;
}

#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_set_320_seg_centers() - Fill the ath channel structure with segment
 * center IEEEs and frequencies.
 * @chan_params: Channel params that will have the channel mode to be filled.
 * @ath_chan: Ath channel structure to be filled.
 */
static void
ieee80211_set_320_seg_centers(struct ch_params *chan_params,
                              struct ieee80211_ath_channel *ath_chan)
{
    ath_chan->ic_vhtop_ch_num_seg1 = chan_params->center_freq_seg0;
    ath_chan->ic_vhtop_ch_num_seg2 = chan_params->center_freq_seg1;
    ath_chan->ic_vhtop_freq_seg1 = chan_params->mhz_freq_seg0;
    ath_chan->ic_vhtop_freq_seg2 = chan_params->mhz_freq_seg1;

}
#endif /* WLAN_FEATURE_11BE*/

static void ieee80211_fill_channel_info_list(
        struct ieee80211_channel_info *chan_info,
        uint8_t chan_num,
        qdf_freq_t primary_freq,
        uint8_t ch_num_seg1, uint8_t ch_num_seg2)
{
    chan_info->ieee = chan_num;
    chan_info->freq = primary_freq;
    chan_info->vhtop_ch_num_seg1 = ch_num_seg1;
    chan_info->vhtop_ch_num_seg2 = ch_num_seg2;
}

static void ieee80211_fill_channel_params(
        struct ieee80211_channel_params *chan_info,
        uint8_t chan_num,
        qdf_freq_t primary_freq,
        uint8_t ch_num_seg1, uint8_t ch_num_seg2, uint8_t seg_index)
{
    chan_info->ieee = chan_num;
    chan_info->freq = primary_freq;
    chan_info->vhtop_ch_num_seg1 = ch_num_seg1;
    if (seg_index != INVALID_SEG2_CENTER_INDEX)
            chan_info->vhtop_ch_num_seg2[seg_index] = ch_num_seg2;
}

#define MAX_TX_POW (63) /* dBm units */
#define MIN_TX_POW (-64) /* dBm units */

#ifdef WLAN_FEATURE_11BE
static void
ieee80211_fill_320_channel_info(struct ieee80211com *ic,
                                qdf_freq_t primary_freq,
                                int *nchans,
                                struct ieee80211_channel_params *chan_info,
                                struct ieee80211_ath_channel *chan_list,
                                struct regulatory_channel *cur_chan_list,
                                enum supported_6g_pwr_types supp_pwr_type)
{
    struct reg_channel_list chan_params_320 = {0};
    uint8_t j;
    uint8_t seg_index = INDEX_320_SEG2_CENTER1;

    ieee80211_regdmn_reg_fill_channel_list(ic->ic_pdev_obj,
                                           primary_freq,
                                           0,
                                           CH_WIDTH_320MHZ, 0,
                                           &chan_params_320,
                                           supp_pwr_type, false);

    for (j = 0; j < chan_params_320.num_ch_params ; j++) {

        struct ch_params *tmp_param = &chan_params_320.chan_param[j];

        if (tmp_param->ch_width == CH_WIDTH_320MHZ) {
            /* Fill the segment centres in ath_channel structure */
            ieee80211_set_320_seg_centers(tmp_param,
                                          &chan_list[*nchans]);
            if (chan_info) {
                ieee80211_fill_channel_params(&chan_info[*nchans],
                                              cur_chan_list->chan_num,
                                              primary_freq,
                                              tmp_param->center_freq_seg0,
                                              tmp_param->center_freq_seg1,
                                              seg_index);
                seg_index ++;
            }
        }
    }
}
#endif

static bool ieee80211_is_chan_160_supported(uint64_t flags)
{
    bool is_160_supported = false;
    if(
#ifdef WLAN_FEATURE_11BE
        (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT160)) ||
#endif
        (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE160))||
        (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT160)) {
        is_160_supported = true;
    }
    return is_160_supported;
}

static bool ieee80211_is_chan_80_supported(uint64_t flags)
{
    bool is_80_supported = false;
    if(
#ifdef WLAN_FEATURE_11BE
        (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT80)) ||
#endif
        (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80))||
        (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80)) {
        is_80_supported = true;
    }
    return is_80_supported;
}

/**
 * ieee80211_fill_chan_info_and_centers() - Fill channel info and segment
 * centers for the given input freq and width.
 * @ic: Pointer to struct ieee80211com
 * @chan_params: Pointer to struct ch_params
 * @chan_list: Pointer to struct ieee80211_ath_channel
 * @primary_freq: Primary frequency in MHZ
 * @chan_info: Pointer to struct ieee80211_channel_params
 * @nchans: Pointer to num channels
 * @cur_chan_list: Pointer to reg chan
 * @in_supp_pwr_type: Input 6g power type.
 *
 * Return - None
 */
static void
ieee80211_fill_chan_info_and_centers(struct ieee80211com *ic,
                                     struct ch_params *chan_params,
                                     struct ieee80211_ath_channel *chan_list,
                                     qdf_freq_t primary_freq,
                                     struct ieee80211_channel_params *chan_info,
                                     int *nchans,
                                     struct regulatory_channel *cur_chan_list,
                                     enum supported_6g_pwr_types in_supp_pwr_type)
{
    uint8_t seg_index = INVALID_SEG2_CENTER_INDEX;
    bool is_chan_80_supported = 0;
    bool is_chan_160_supported = 0;

    is_chan_80_supported = ieee80211_is_chan_80_supported(chan_info[*nchans].flags);
    is_chan_160_supported = ieee80211_is_chan_160_supported(chan_info[*nchans].flags);

    if (is_chan_160_supported) {
        chan_params->ch_width = CH_WIDTH_160MHZ;
        seg_index = INDEX_160_SEG2_CENTER;
    } else if (is_chan_80_supported) {
        chan_params->ch_width = CH_WIDTH_80MHZ;
    }

    if (is_chan_160_supported || is_chan_80_supported) {
        ieee80211_set_seg_centers(ic, chan_params, &chan_list[*nchans],
                                  primary_freq, in_supp_pwr_type);

        ieee80211_fill_channel_params(&chan_info[*nchans],
                                     cur_chan_list->chan_num,
                                     primary_freq,
                                     chan_params->center_freq_seg0,
                                     chan_params->center_freq_seg1,
                                     seg_index);
    }

#ifdef WLAN_FEATURE_11BE
    if((chan_info[*nchans].flags &
        VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT320))) {
        chan_params->ch_width = CH_WIDTH_320MHZ;

        ieee80211_fill_320_channel_info(ic, primary_freq, nchans, chan_info,
                                        chan_list, cur_chan_list, in_supp_pwr_type);
    }
#endif
    /* Filling channel info flags for channels that support less than 80BW */
    ieee80211_fill_channel_params(&chan_info[*nchans],
                                  cur_chan_list->chan_num,
                                  primary_freq,
                                  chan_params->center_freq_seg0,
                                  chan_params->center_freq_seg1,
                                  INVALID_SEG2_CENTER_INDEX);
}

void ieee80211_get_radio_channel_list(
        struct ieee80211com *ic,
        struct ieee80211_ath_channel *chan_list,
        struct ieee80211_channel_params *chan_info,
        int *nchans)
{
    uint64_t user_mode = 0, chip_mode, modes_supported;
    uint16_t phybitmap;
    struct regulatory_channel *cur_chan_list;
    int8_t max_tx_power = MAX_TX_POW, min_tx_power = MIN_TX_POW;
    int i;
    enum reg_6g_ap_type ap_pwr_type;
    bool get_cur_chan_list;
    enum supported_6g_pwr_types supp_pwr_type;
#ifdef CONFIG_AFC_SUPPORT
    enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif

    if (ic->ic_get_modeSelect) {
        user_mode = ic->ic_get_modeSelect(ic);
    }

    ieee80211_regdmn_get_chip_mode(ic->ic_pdev_obj, &chip_mode);

    modes_supported = (user_mode & chip_mode);

    /* Get the phymodes that are not supported by regulatory. */
    ieee80211_regdmn_get_phybitmap(ic->ic_pdev_obj, &phybitmap);
    ieee80211_regdmn_remove_phybitmap_from_modeselect(&modes_supported,
                                                      phybitmap);

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list)
        return;

    ap_pwr_type = REG_MAX_AP_TYPE;
    get_cur_chan_list = true;
#ifdef CONFIG_AFC_SUPPORT
    wlan_reg_get_afc_dev_deploy_type(ic->ic_pdev_obj, &reg_afc_deploy_type);

    /* The logic of power mode selection should be revisited if SP and LPI
     * lists have mutually exclusive channels. (Or VLP and LPI lists have
     * mutually exclusive channels). Currently, the logic assumes that SP
     * is a subset of LPI/VLP. (Or VLP is subset of LPI).
     */
    if (reg_afc_deploy_type != AFC_DEPLOYMENT_OUTDOOR) {
        if (wlan_reg_get_num_rules_of_ap_pwr_type(ic->ic_pdev_obj, REG_INDOOR_AP))
            ap_pwr_type = REG_INDOOR_AP;
        else if (wlan_reg_get_num_rules_of_ap_pwr_type(ic->ic_pdev_obj, REG_VERY_LOW_POWER_AP))
            ap_pwr_type = REG_VERY_LOW_POWER_AP;
        else if (wlan_reg_get_num_rules_of_ap_pwr_type(ic->ic_pdev_obj, REG_STANDARD_POWER_AP))
            ap_pwr_type = REG_STANDARD_POWER_AP;
    } else {
        if (wlan_reg_get_num_rules_of_ap_pwr_type(ic->ic_pdev_obj, REG_VERY_LOW_POWER_AP))
            ap_pwr_type = REG_VERY_LOW_POWER_AP;
        else
            ap_pwr_type = REG_STANDARD_POWER_AP;
    }

    if (ieee80211_is_afc_capable(ic, ieee80211_chan2mode(ic->ic_curchan))) {
        get_cur_chan_list = false;
    }
#endif

    supp_pwr_type = wlan_reg_conv_6g_ap_type_to_supported_6g_pwr_types(ap_pwr_type);
    if (wlan_reg_get_pwrmode_chan_list(ic->ic_pdev_obj,
                                       cur_chan_list,
                                       supp_pwr_type) != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        goto exit;
    }

    if (ic->ic_get_min_and_max_power)
        ic->ic_get_min_and_max_power(ic, &max_tx_power, &min_tx_power);

    for (i = 0; i < NUM_CHANNELS; i++)
    {
        struct ch_params chan_params = {0};
        uint64_t flags = 0;
        uint64_t mode_flags, band_flags;
        uint32_t flag_ext = 0;
        qdf_freq_t primary_freq = cur_chan_list[i].center_freq;

        /*
         * See if the channel is enabled or disabled due to radar temporarily.
         * If it's disabled by default (not by radar), ignore the channel.
         */
        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
            continue;

        /* Fill the band flag for the channel. */
        if (WLAN_REG_IS_6GHZ_CHAN_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_6G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_6GHZ;
        } else if (WLAN_REG_IS_24GHZ_CH_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_2G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_2GHZ;
        } else if (WLAN_REG_IS_5GHZ_CH_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_5G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_5GHZ;
        } else if (WLAN_REG_IS_49GHZ_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_49G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_5GHZ;

            /* If 4.9G Half and Quarter rates are supported by the channel,
               update them as separate entries to the list */
            if (BW_WITHIN(cur_chan_list[i].min_bw, HALF_BW,
                          cur_chan_list[i].max_bw)) {
                flags = IEEE80211_CHAN_HALF | IEEE80211_CHAN_A;
                ieee80211_fill_athchan(&cur_chan_list[i], &chan_list[*nchans],
                                       min_tx_power, max_tx_power, flags);

                if (chan_info) {
                    ieee80211_fill_channel_params(&chan_info[*nchans],
                            cur_chan_list[i].chan_num, primary_freq, 0, 0, INVALID_SEG2_CENTER_INDEX);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_HALF);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_A);
                }

                if (++(*nchans) >= IEEE80211_CHAN_MAX)
                    break;
            }
            if (BW_WITHIN(cur_chan_list[i].min_bw, QRTR_BW,
                          cur_chan_list[i].max_bw)) {
                flags = IEEE80211_CHAN_QUARTER | IEEE80211_CHAN_A;
                ieee80211_fill_athchan(&cur_chan_list[i], &chan_list[*nchans],
                                       min_tx_power, max_tx_power, flags);

                if (chan_info) {
                    ieee80211_fill_channel_params(&chan_info[*nchans],
                                                  cur_chan_list[i].chan_num, primary_freq, 0, 0, INVALID_SEG2_CENTER_INDEX);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_QUARTER);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_A);
                }

                if (++(*nchans) >= IEEE80211_CHAN_MAX)
                    break;
            }
        } else {
            continue;
        }

        ieee80211_set_max_supported_bw(ic,
                                       primary_freq,
                                       &cur_chan_list[i],
                                       supp_pwr_type);

        mode_flags = ieee80211_set_modeflag_for_chan(
                ic, modes_supported, &chan_params,
                &cur_chan_list[i], 0,
                (chan_info) ? &chan_info[*nchans].flags : NULL, supp_pwr_type);

        if (!mode_flags)
            continue;

        wlan_reg_get_channel_flags(ic, primary_freq, &flag_ext, &flags,
                                   supp_pwr_type, &cur_chan_list[i]);

        if (chan_info) {
            chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(band_flags);
            chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(flags);
            chan_info[*nchans].flags_ext |= wlan_get_channel_flags_ext(flag_ext);
            /* Mode flags for 160 MHz channel widths are being set incorrectly for
             * 5 GHz channel numbers 132 to 144. Reset these to ensure that the
             * application receives correct channel information.
             */
            if (primary_freq >= CHAN_FREQ_5660 && primary_freq <= CHAN_FREQ_5720) {
                chan_info[*nchans].flags &=
                        ~(VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE160) |
                          QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT160
#ifdef WLAN_FEATURE_11BE
                          | VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT160)
#endif
                         );
	    }
        }

        flags |= mode_flags | band_flags;

        ieee80211_fill_athchan(&cur_chan_list[i], &chan_list[*nchans],
                               min_tx_power, max_tx_power, flags);

        if (chan_info) {
            ieee80211_fill_chan_info_and_centers(ic, &chan_params,
                                                 chan_list,
                                                 primary_freq,
                                                 chan_info,
                                                 nchans,
                                                 &cur_chan_list[i],
                                                 supp_pwr_type);
        }

#ifdef CONFIG_AFC_SUPPORT
        if (!get_cur_chan_list && chan_info) {
             for (ap_pwr_type = REG_INDOOR_AP; ap_pwr_type < REG_CURRENT_MAX_AP_TYPE;
                  ap_pwr_type++) {
                  if (ieee80211_get_afc_channel_info
                               (ic, &chan_info[*nchans], primary_freq, ap_pwr_type)) {
                      qdf_err("Unable to get AFC channel information");
                      goto exit;
                  }
             }
        }
#endif

        chan_list[*nchans].ic_flagext = flag_ext;

        if (++(*nchans) >= IEEE80211_CHAN_MAX)
            break;
    }

exit:
    qdf_mem_free(cur_chan_list);
    return;
}

#ifdef CONFIG_AFC_SUPPORT
QDF_STATUS ieee80211_get_afc_channel_info(
                   struct ieee80211com *ic,
                   struct ieee80211_channel_params *chan_info,
                   qdf_freq_t primary_freq,
                   enum reg_6g_ap_type pwr_type)
{
    struct regulatory_channel *chan_list = NULL;
    int i;

    if (pwr_type >= REG_MAX_AP_TYPE) {
        qdf_err("Invalid power type %d for AFC", pwr_type);
        return QDF_STATUS_E_FAILURE;
    }

    chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*chan_list));
    if (!chan_list)
        return QDF_STATUS_E_FAILURE;

    if (wlan_reg_get_ap_chan_list(ic->ic_pdev_obj,
                                  chan_list,
                                  false,
                                  pwr_type) != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        qdf_mem_free(chan_list);
        return QDF_STATUS_E_FAILURE;
    }

    for (i = 0; i < NUM_CHANNELS; i++) {
        if (primary_freq == chan_list[i].center_freq) {
            if ((chan_list[i].state == CHANNEL_STATE_DISABLE) ||
                (chan_list[i].chan_flags & REGULATORY_CHAN_DISABLED))
                    break;
            chan_info->supp_power_modes |= (1 << pwr_type);
            chan_info->reg_powers[pwr_type].tx_power = chan_list[i].tx_power;
            break;
        }
    }

    qdf_mem_free(chan_list);
    return QDF_STATUS_SUCCESS;
}
#endif /* CONFIG_AFC_SUPPORT */

/*
 * Non-11BE legacy handler for channel list
 */
void ieee80211_get_channel_list(
        struct ieee80211com *ic,
        struct ieee80211_ath_channel *chan_list,
        struct ieee80211_channel_info *chan_info,
        int *nchans,
        bool flag_160)
{
    uint64_t user_mode = 0, chip_mode, modes_supported;
    uint16_t phybitmap;
    struct regulatory_channel *cur_chan_list;
    int8_t max_tx_power = MAX_TX_POW, min_tx_power = MIN_TX_POW;
    int i;
    enum reg_6g_ap_type ap_pwr_type;
    bool get_cur_chan_list;
#ifdef CONFIG_AFC_SUPPORT
    enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif

    if (ic->ic_get_modeSelect) {
        user_mode = ic->ic_get_modeSelect(ic);
    }

    ieee80211_regdmn_get_chip_mode(ic->ic_pdev_obj, &chip_mode);

    modes_supported = (user_mode & chip_mode);

    /* Get the phymodes that are not supported by regulatory. */
    ieee80211_regdmn_get_phybitmap(ic->ic_pdev_obj, &phybitmap);
    ieee80211_regdmn_remove_phybitmap_from_modeselect(&modes_supported,
                                                      phybitmap);

    cur_chan_list = qdf_mem_malloc(NUM_CHANNELS * sizeof(*cur_chan_list));
    if (!cur_chan_list)
        return;

    ap_pwr_type = REG_MAX_AP_TYPE;
    get_cur_chan_list = true;
#ifdef CONFIG_AFC_SUPPORT
    wlan_reg_get_afc_dev_deploy_type(ic->ic_pdev_obj, &reg_afc_deploy_type);

    if (ieee80211_is_afc_capable(ic, ieee80211_chan2mode(ic->ic_curchan)) &&
        reg_afc_deploy_type != AFC_DEPLOYMENT_OUTDOOR) {
        /* Populate LPI channel list if AFC capable */
        ap_pwr_type = REG_INDOOR_AP;
        get_cur_chan_list = false;
    }
#endif

    if (wlan_reg_get_ap_chan_list(ic->ic_pdev_obj,
                                  cur_chan_list,
                                  get_cur_chan_list,
                                  ap_pwr_type) != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get cur_chan list");
        goto exit;
    }

    if (ic->ic_get_min_and_max_power)
        ic->ic_get_min_and_max_power(ic, &max_tx_power, &min_tx_power);

    for (i = 0; i < NUM_CHANNELS; i++)
    {
        struct ch_params chan_params = {0};
        uint64_t flags = 0;
        uint64_t mode_flags, band_flags;
        uint32_t flag_ext = 0;
        qdf_freq_t primary_freq = cur_chan_list[i].center_freq;

        /*
         * See if the channel is enabled or disabled due to radar temporarily.
         * If it's disabled by default (not by radar), ignore the channel.
         */
        if (wlan_reg_is_chan_disabled_and_not_nol(&cur_chan_list[i]))
            continue;

        /* Fill the band flag for the channel. */
        if (WLAN_REG_IS_6GHZ_CHAN_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_6G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_6GHZ;
        } else if (WLAN_REG_IS_24GHZ_CH_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_2G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_2GHZ;
        } else if (WLAN_REG_IS_5GHZ_CH_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_5G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_5GHZ;
        } else if (WLAN_REG_IS_49GHZ_FREQ(primary_freq)) {
            if (!(modes_supported & WIRELESS_49G_MODES))
                continue;
            band_flags = IEEE80211_CHAN_5GHZ;

            /* If 4.9G Half and Quarter rates are supported by the channel,
               update them as separate entries to the list */
            if (BW_WITHIN(cur_chan_list[i].min_bw, HALF_BW,
                          cur_chan_list[i].max_bw)) {
                flags = IEEE80211_CHAN_HALF | IEEE80211_CHAN_A;
                ieee80211_fill_athchan(&cur_chan_list[i], &chan_list[*nchans],
                                       min_tx_power, max_tx_power, flags);

                if (chan_info) {
                    ieee80211_fill_channel_info_list(&chan_info[*nchans],
                            cur_chan_list[i].chan_num, primary_freq, 0, 0);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_HALF);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_A);
                }

                if (++(*nchans) >= IEEE80211_CHAN_MAX)
                    break;
            }
            if (BW_WITHIN(cur_chan_list[i].min_bw, QRTR_BW,
                          cur_chan_list[i].max_bw)) {
                flags = IEEE80211_CHAN_QUARTER | IEEE80211_CHAN_A;
                ieee80211_fill_athchan(&cur_chan_list[i], &chan_list[*nchans],
                                       min_tx_power, max_tx_power, flags);

                if (chan_info) {
                    ieee80211_fill_channel_info_list(&chan_info[*nchans],
                            cur_chan_list[i].chan_num, primary_freq, 0, 0);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_QUARTER);
                    chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(IEEE80211_CHAN_A);
                }

                if (++(*nchans) >= IEEE80211_CHAN_MAX)
                    break;
            }
        } else {
            continue;
        }

        mode_flags = ieee80211_set_modeflag_for_chan(
                ic, modes_supported, &chan_params,
                &cur_chan_list[i], flag_160,
                (chan_info) ? &chan_info[*nchans].flags : NULL,
                REG_CURRENT_PWR_MODE);

        /**
         * If no modes are supported for the channel (possible in only half and
         * quarter rate supported 4.9G channels), avoid adding a new entry
         * as the half and quarter rate are handled earlier in this API.
         **/
        if (!mode_flags)
            continue;

        wlan_reg_get_channel_flags(ic, primary_freq, &flag_ext, &flags,
                                   REG_CURRENT_PWR_MODE, &cur_chan_list[i]);

        if (chan_info) {
            chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(band_flags);
            chan_info[*nchans].flags |= (uint64_t)wlan_get_channel_flags(flags);
            chan_info[*nchans].flags_ext |= wlan_get_channel_flags_ext(flag_ext);
        }

        flags |= mode_flags | band_flags;

        ieee80211_fill_athchan(&cur_chan_list[i], &chan_list[*nchans],
                               min_tx_power, max_tx_power, flags);

        if (chan_params.ch_width != CH_WIDTH_INVALID) {
#ifdef CONFIG_AFC_SUPPORT
            if (ieee80211_is_afc_capable(ic, ieee80211_chan2mode(ic->ic_curchan)))
                ieee80211_set_seg_centers(ic, &chan_params, &chan_list[*nchans],
                                          primary_freq, REG_BEST_PWR_MODE);
            else
#endif /* CONFIG_AFC_SUPPORT */
                ieee80211_set_seg_centers(ic, &chan_params, &chan_list[*nchans],
                                          primary_freq, REG_CURRENT_PWR_MODE);
        }

        if (chan_info) {
            ieee80211_fill_channel_info_list(&chan_info[*nchans],
                    cur_chan_list[i].chan_num, primary_freq, chan_params.center_freq_seg0,
                    chan_params.center_freq_seg1);
        }

        chan_list[*nchans].ic_flagext = flag_ext;

        if (++(*nchans) >= IEEE80211_CHAN_MAX)
            break;
    }

exit:
    qdf_mem_free(cur_chan_list);
    return;
}

static void
ieee80211_get_subchan(struct ieee80211_ath_channel *chan, qdf_freq_t *sub_chan_freqs, uint8_t *nchans)
{
	qdf_freq_t center_freq;
	uint16_t ch_width = ieee80211_get_chan_width(chan);
	switch (ch_width) {
		case 20:
			center_freq = chan->ic_vhtop_freq_seg1;
			sub_chan_freqs[0] = center_freq;
			*nchans = 1;
			break;
		case 40:
			center_freq = chan->ic_vhtop_freq_seg1;
			sub_chan_freqs[0] = center_freq - 10;
			sub_chan_freqs[1] = center_freq + 10;
			*nchans = 2;
			break;
		case 80:
			center_freq = chan->ic_vhtop_freq_seg1;
			sub_chan_freqs[0] = center_freq - 30;
			sub_chan_freqs[1] = center_freq - 10;
			sub_chan_freqs[2] = center_freq + 10;
			sub_chan_freqs[3] = center_freq + 30;
			*nchans = 4;
			break;
		case 160:
			if (IEEE80211_IS_CHAN_MODE_160(chan))
			{
				center_freq = chan->ic_vhtop_freq_seg2;
				sub_chan_freqs[0] = center_freq - 70;
				sub_chan_freqs[1] = center_freq - 50;
				sub_chan_freqs[2] = center_freq - 30;
				sub_chan_freqs[3] = center_freq - 10;
				sub_chan_freqs[4] = center_freq + 10;
				sub_chan_freqs[5] = center_freq + 30;
				sub_chan_freqs[6] = center_freq + 50;
				sub_chan_freqs[7] = center_freq + 70;
				*nchans = 8;
			} else {
				center_freq = chan->ic_vhtop_freq_seg1;
				sub_chan_freqs[0] = center_freq - 30;
				sub_chan_freqs[1] = center_freq - 10;
				sub_chan_freqs[2] = center_freq + 10;
				sub_chan_freqs[3] = center_freq + 30;
				center_freq = chan->ic_vhtop_freq_seg2;
				sub_chan_freqs[4] = center_freq - 30;
				sub_chan_freqs[5] = center_freq - 10;
				sub_chan_freqs[6] = center_freq + 10;
				sub_chan_freqs[7] = center_freq + 30;
				*nchans = 8;
			}
			break;
#ifdef WLAN_FEATURE_11BE
		case 320:
				center_freq = chan->ic_vhtop_freq_seg2;
				sub_chan_freqs[0] = center_freq - 150;
				sub_chan_freqs[1] = center_freq - 130;
				sub_chan_freqs[2] = center_freq - 110;
				sub_chan_freqs[3] = center_freq - 90;
				sub_chan_freqs[4] = center_freq - 70;
				sub_chan_freqs[5] = center_freq - 50;
				sub_chan_freqs[6] = center_freq - 30;
				sub_chan_freqs[7] = center_freq - 10;
				sub_chan_freqs[8] = center_freq + 10;
				sub_chan_freqs[9] = center_freq + 30;
				sub_chan_freqs[10] = center_freq + 50;
				sub_chan_freqs[11] = center_freq + 70;
				*nchans = 12;
				break;
#endif
	}
}

bool ieee80211_is_chan_radar(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
	qdf_freq_t sub_chan_freqs[MAX_NUM_20MHZ_SUBCHANS];
	uint8_t nchans;
	uint8_t i;

	if (!IEEE80211_IS_CHAN_VALID(chan) || !IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan))
		return false;
	ieee80211_get_subchan(chan, sub_chan_freqs, &nchans);
	for (i = 0; i < nchans; i++) {
		if(wlan_reg_is_nol_for_freq(ic->ic_pdev_obj, sub_chan_freqs[i]))
			return true;
	}
	return false;
}

bool ieee80211_is_chan_nol_history(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
	qdf_freq_t sub_chan_freqs[MAX_NUM_20MHZ_SUBCHANS];
	uint8_t nchans;
	uint8_t i;

	if (!IEEE80211_IS_CHAN_VALID(chan) || !IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan))
		return false;
	ieee80211_get_subchan(chan, sub_chan_freqs, &nchans);
	for (i = 0; i < nchans; i++) {
		if(wlan_reg_is_nol_hist_for_freq(ic->ic_pdev_obj, sub_chan_freqs[i]))
			return true;
	}
	return false;
}

void ieee80211_set_chan_40intol(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    enum ht40_intol ht40intol_flags;

    if (IEEE80211_IS_CHAN_40PLUS(chan))
        ht40intol_flags = HT40_INTOL_PLUS;
    else if (IEEE80211_IS_CHAN_40MINUS(chan))
        ht40intol_flags = HT40_INTOL_MINUS;
    else
        return;

    wlan_reg_set_chan_ht40intol(ic->ic_pdev_obj, chan->ic_freq, ht40intol_flags);
}

void ieee80211_clear_chan_40intol(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    enum ht40_intol ht40intol_flags;

    if (IEEE80211_IS_CHAN_40PLUS(chan))
        ht40intol_flags = HT40_INTOL_PLUS;
    else if (IEEE80211_IS_CHAN_40MINUS(chan))
        ht40intol_flags = HT40_INTOL_MINUS;
    else
        return;

    wlan_reg_clear_chan_ht40intol(ic->ic_pdev_obj, chan->ic_freq, ht40intol_flags);
}

bool ieee80211_is_chan_40intol(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    enum ht40_intol ht40intol_flags;

    if (IEEE80211_IS_CHAN_40PLUS(chan))
        ht40intol_flags = HT40_INTOL_PLUS;
    else if (IEEE80211_IS_CHAN_40MINUS(chan))
        ht40intol_flags = HT40_INTOL_MINUS;
    else
        return false;

    return wlan_reg_is_chan_ht40intol(ic->ic_pdev_obj, chan->ic_freq, ht40intol_flags);
}

void ieee80211_get_default_psd_power(struct ieee80211vap *vap,
                                     enum reg_6g_client_type client_type,
                                     uint8_t *psd_pwr)
{
   struct ieee80211com *ic = vap->iv_ic;
   uint16_t chwidth;
   enum reg_6g_ap_type ap_device_type;
   enum phy_ch_width phy_chan_width;
   enum ieee80211_cwm_width cwm_width;

   if(vap->iv_chwidth != IEEE80211_CWM_WIDTHINVALID) {
      cwm_width = vap->iv_chwidth;
   } else {
      cwm_width = ic->ic_cwm_get_width(ic);
   }

   /* Update with actual BW setting value.
    * For instance:
    * chwidth = 20 in case AP BW is 20MHz
    */
   phy_chan_width = ieee80211_convert_cwm_wd_to_phych_width(cwm_width);
   chwidth = wlan_reg_get_bw_value(phy_chan_width);
   ucfg_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &ap_device_type);
   wlan_reg_get_max_txpower_for_6g_tpe(ic->ic_pdev_obj,
                                       ic->ic_curchan->ic_freq,
                                       chwidth, ap_device_type,
                                       client_type, true,
                                       psd_pwr);
}

#if defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
static bool
mlme_check_freq_in_afc_chan_list(struct regulatory_channel *afc_chan_list,
                                 qdf_freq_t freq)
{
    enum channel_enum chan_enum;

    for (chan_enum = 0; chan_enum < NUM_6GHZ_CHANNELS; chan_enum++)
        if (afc_chan_list[chan_enum].state == CHANNEL_STATE_ENABLE)
            if (afc_chan_list[chan_enum].center_freq == freq)
                return true;

    return false;
}

QDF_STATUS
mlme_check_curchan_and_set_ap_pwr_type(struct ieee80211com *ic,
                                       struct ieee80211_ath_channel *chan)
{
    enum ieee80211_cwm_width ch_width;
    uint16_t bw;
    enum reg_6g_ap_type new_ap_pwr_type;
    enum ieee80211_phymode phymode;
    enum phy_ch_width phy_chan_width;
    /* Do we need to update power info/mode in the beacon
     * and do we need to update fw tx power
     */
    bool is_update_bcnpwrinfo_and_fwtxpwr;
    uint16_t in_punc_bitmap;
    bool is_sup_chanlist_null;

    if (!ieee80211_allow_power_mode_change(ic)) {
#if UMAC_SUPPORT_CFG80211
        qdf_sched_work(NULL,&ic->ic_cfg80211_update_channel_list_start_work);
#endif
        /* update channel list */
        ieee80211_update_channellist(ic, 1, true);
        return QDF_STATUS_SUCCESS;
    }

    phymode = ieee80211_chan2mode(chan);
    ch_width = get_chwidth_phymode(phymode);
    if (ch_width == IEEE80211_CWM_WIDTHINVALID)
        ch_width = ic->ic_cwm_get_width(ic);
    phy_chan_width = ieee80211_convert_cwm_wd_to_phych_width(ch_width);
    bw = wlan_reg_get_bw_value(phy_chan_width);
    in_punc_bitmap = ieee80211_get_chan_punc_bitmap(chan);
    new_ap_pwr_type = wlan_reg_get_best_pwr_mode(ic->ic_pdev_obj,
                                                 chan->ic_freq,
                                                 chan->ic_vhtop_freq_seg2,
                                                 bw,
                                                 in_punc_bitmap);
    is_sup_chanlist_null =
             !ieee80211_is_6g_txable_chan_available(ic, REG_BEST_PWR_MODE);
    if (is_sup_chanlist_null ||
        wlan_reg_is_6g_ap_type_invalid(new_ap_pwr_type)) {
        qdf_info("Cannot change power mode as 6Ghz chan is not available."
                 "cur power type = %u", new_ap_pwr_type);
        qdf_info("Still the computing the current channel "
                 "to remove old channels from it");
        wlan_reg_recompute_current_chan_list(NULL, ic->ic_pdev_obj);
    } else {
        qdf_info("Moving power mode to %u", new_ap_pwr_type);
        wlan_reg_set_ap_pwr_and_update_chan_list(ic->ic_pdev_obj,
                                                 new_ap_pwr_type);
    }
    is_update_bcnpwrinfo_and_fwtxpwr =
                 ieee80211_is_6g_txable_chan_available(ic, REG_BEST_PWR_MODE);
    mlme_update_chan_list_and_restart_vaps(ic, is_update_bcnpwrinfo_and_fwtxpwr);

    ic->ic_curchan_eirp_power = wlan_reg_get_eirp_pwr(ic->ic_pdev_obj,
                                                      chan->ic_freq,
                                                      chan->ic_vhtop_freq_seg2,
                                                      ieee80211_get_chan_width(chan),
                                                      new_ap_pwr_type,
                                                      in_punc_bitmap,
                                                      false,
                                                      REG_MAX_CLIENT_TYPE, false);
    return QDF_STATUS_SUCCESS;
}

/**
 * ieee80211_vap_start_needed_in_afc_enterprise() - Checks if the vap needs
 * to restart on setting an input power mode via ap_power_mode command.
 * @ic: Pointer to struct ieee80211com
 *
 * Return: True if vap needs to restart, false otherwise.
 */
static bool
ieee80211_vap_start_needed_in_afc_enterprise(struct ieee80211com *ic)
{

	if (ieee80211_vaps_ready(ic, IEEE80211_M_HOSTAP)) {
		qdf_info("vaps are already up, honor power mode change\n");
		return true;
	}
	/* If AFC response is received and SP channels are available, update
	 * cfg80211 and hostapd with NO_IR flag cleared. This ensures
	 * hostapd invokes start_ap with the uci configured channel parameters
	 * to bring up the vap. If we allow ucfg_check_curchan_and_set_ap_
	 * pwr_type() to proceed further for vap bringup, it assigns
	 * iv_des_hw_mode with ic->curchan mode which will be the
	 * default 11AHE20 channel set during if_attach. This causes vap
	 * bringup failure in MLO and the start_ap params do not take effect.
	 * Hence do not allow ucfg_check_curchan_and_set_ap_pwr_type() to
	 * proceed further for bring up.
	 */
	wlan_reg_recompute_current_chan_list(NULL, ic->ic_pdev_obj);
	wlan_cfg80211_update_channel_list(ic);
	wlan_scan_update_channel_list(ic);

	return false;
}

static QDF_STATUS
ieee80211_setchan_and_wait_for_up(struct ieee80211com *ic,
                                  struct ieee80211vap *vap,
                                  struct ieee80211_ath_channel *newchan)
{
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    QDF_STATUS status;

    status = wlan_vdev_chan_config_valid(vdev);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("chan config valid for vdev");
        return status;
    }

    /*
     * Channel switch time is a critical KPI for special vap mode.
     * Use vdev restart sequence instead of vdev stop-start sequence,
     * for changing channel of a special vap.
     */
    if (vdev->vdev_mlme.des_chan)
        ieee80211_update_vdev_chan(vdev->vdev_mlme.des_chan, newchan);
    wlan_iterate_vap_list(ic, ieee80211_vap_iter_set_des_channel, (void *)newchan);

    qdf_event_reset(&vap->wait_event_vdev_up_state);
    osif_pdev_restart_vaps(ic);
    status = osif_vap_wait_for_vdev_up_state(vap);
    if (QDF_IS_STATUS_ERROR(status))
        qdf_err("Failed to wait for special vap to go to up state");

    return status;
}

/* ieee80211_validate_chan_bw_in_afc_response() - Checks if the channel's BW
 * opclass is present in the AFC response Channel object.
 * @ic: Pointer to struct ieee80211com
 * @chan: Pointer to the channel to be validated.
 * @new_ap_pwr_type: New AP power type.
 *
 * Return: True if the channel's BW opclass is present.
 */
static bool
ieee80211_validate_chan_bw_in_afc_response(struct ieee80211com *ic,
                                           struct ieee80211_ath_channel *chan,
                                           enum reg_6g_ap_type new_ap_pwr_type)
{
    qdf_freq_t center_320 = 0;
    uint16_t bw = ieee80211_get_chan_width(chan);

    /* For non-SP power modes, checking regulatory channel list is enough */
    if (new_ap_pwr_type != REG_STANDARD_POWER_AP)
            return true;

#ifdef WLAN_FEATURE_11BE
    /* For punctured channel, we consider only the
     * frequency object in AFC payload. Presence of
     * frequency range is already validated by regulatory
     * as part of ieee80211_find_channel API.
     */
    if (IEEE80211_IS_CHAN_PUNCTURED(chan))
        return true;

    center_320 = (bw == BW_320_MHZ) ? chan->ic_vhtop_freq_seg2 : 0;
#endif

    return wlan_reg_validate_freq_in_afc_chan_obj(ic->ic_pdev_obj,
                                                  chan->ic_freq,
                                                  center_320,
                                                  bw);
}

/**
 * ucfg_check_curchan_and_set_ap_pwr_type() - Checks if input channel 'chan'
 * is present in new_ap_pwr_type's channel list. It changes the power mode if
 * the channel is present and is same as current operating channel, by sending
 * the power values in TPC command. If the channel is different from the current
 * channel, then the power mode is still set in the regulatory but TPC command
 * is not sent. A restart request is sent which will send the TPC command. This
 * restart request command is a blocking call and will wait until the vap is in
 * up state.If a valid channel was not found, then the channel change sequence
 * is followed by setting the best power mode for the current channel followed
 * by blocking restart request.
 */
static QDF_STATUS
ucfg_check_curchan_and_set_ap_pwr_type_special_vap(struct ieee80211com *ic,
                                        struct ieee80211vap *vap,
                                        struct ieee80211_ath_channel *chan,
                                        enum reg_6g_ap_type new_ap_pwr_type)
{
    struct chan_params chan_p = {0};
    struct ieee80211_ath_channel chan_obj = {0};
    struct ieee80211_ath_channel *newchan = &chan_obj;
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    QDF_STATUS status;
    uint16_t in_punc_pattern;

    /* Store the channel parameters of the input channel */
    chan_p.freq = ieee80211_chan2freq(ic, chan);
    chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
#ifdef WLAN_FEATURE_11BE
    set_chan_params_puncture_bitmap(&chan_p, chan->op_puncture_bitmap);
#endif
    chan_p.input_6g_pwr_mode =
          wlan_reg_conv_6g_ap_type_to_supported_6g_pwr_types(new_ap_pwr_type);

    /* Use the channel parameters and in the input power mode to find a new channel */
    *newchan = ieee80211_find_channel(ic, &chan_p, chan->ic_flags);

    /* If a valid channel is found */
    if (IEEE80211_IS_CHAN_VALID(newchan)) {
        if (!ieee80211_validate_chan_bw_in_afc_response(ic, newchan, new_ap_pwr_type)) {
            qdf_err("chan %d bw %d not present in AFC payload"
                    " for SP operation", newchan->ic_freq,
                    ieee80211_get_chan_width(chan));
            return QDF_STATUS_E_FAILURE;
        }

        /* If the new channel is same as the current channel, then set the
         * new 6G AP power type, change the channel lists and restart all
         * the vaps.
         */
        wlan_quick_reg_set_ap_pwr_and_update_chan_list(pdev,
                                                       new_ap_pwr_type);
        if (IEEE80211_ARE_CHANS_EQUAL(chan, newchan)) {
            mlme_quick_update_chan_list_and_send_tpc(ic, true);
            status = QDF_STATUS_SUCCESS;
        } else {
            /* If the new channel is different from the current channel, then
             * set the new AP power type, update the channel list and change
             * the current channel to the newly found channel by sending restart
             * request.
             */
            mlme_quick_update_chan_list_and_send_tpc(ic, false);
            status = ieee80211_setchan_and_wait_for_up(ic, vap, newchan);
        }
    } else {
        /* If a valid channel was not found set the best power mode */
        chan_p.input_6g_pwr_mode = REG_CURRENT_PWR_MODE;
        in_punc_pattern = ieee80211_get_chan_param_op_punc_bitmap(&chan_p);

        ieee80211_quick_apply_best_6g_ap_pwr_mode(ic, chan_p.freq,
                                                  chan_p.cfreq2,
                                                  false,
                                                  vap->iv_des_mode,
                                                  in_punc_pattern);
        status = ieee80211_setchan_and_wait_for_up(ic, vap, chan);
    }
    return status;
}

bool
ieee80211_allow_power_mode_change(struct ieee80211com *ic)
{
    uint16_t num_ap_vaps_up = 0;
    struct ieee80211vap *vap;

    vap = TAILQ_FIRST(&ic->ic_vaps);
    if (!vap) {
        QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_DEBUG,
                  "No vaps found in radio");
        return false;
    }

    num_ap_vaps_up = ieee80211_vaps_up(ic, IEEE80211_M_HOSTAP);
    if (!ic->ic_sta_vap && !num_ap_vaps_up) {
        QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_DEBUG,
                  "No AP vaps are up. Cannot change power mode.");
        return false;
    }

    if (ic->ic_sta_vap && !ieee80211_vap_is_connected(ic->ic_sta_vap) &&
        !num_ap_vaps_up) {
        QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_DEBUG,
                  "STA VAP is not connected / No AP vaps are up in RE. Cannot change power mode");
        return false;
    }

    return true;
}

QDF_STATUS
ucfg_check_curchan_and_set_ap_pwr_type(struct ieee80211com *ic,
                                       struct ieee80211_ath_channel *chan,
                                       enum reg_6g_ap_type new_ap_pwr_type)
{
	struct chan_params chan_p = {0};
	struct ieee80211_ath_channel chan_obj = {0};
	struct ieee80211_ath_channel *newchan = &chan_obj;
	struct ieee80211vap *tmp_vap = TAILQ_FIRST(&ic->ic_vaps);

	if (tmp_vap && IEEE80211_IS_SPECIAL_VAP_ONLY(tmp_vap))
	    return ucfg_check_curchan_and_set_ap_pwr_type_special_vap(ic, tmp_vap, chan, new_ap_pwr_type);

	/*
	 * If there is a STA VAP and it is not connected, remain in LPI.
	 * Should we allow this power mode change in repeater case?
	 */
	if (ic->ic_sta_vap &&
	    !ieee80211_vap_is_connected(ic->ic_sta_vap) &&
	    !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {
		qdf_err("STA VAP is not connected - cannot change power mode");
		return QDF_STATUS_E_FAILURE;
	}

	if (wlan_reg_is_noaction_on_afc_pwr_evt(ic->ic_pdev_obj)) {
		if (!ieee80211_vap_start_needed_in_afc_enterprise(ic)) {
			QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_DEBUG,
				"vaps are not up, it is the responsibility of"
				" hostapd to bring up vaps in E-mode\n");
			return QDF_STATUS_SUCCESS;
		}
	}

	/* Store the channel parameters of the input channel */
	chan_p.freq = ieee80211_chan2freq(ic, chan);
	chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
#ifdef WLAN_FEATURE_11BE
	set_chan_params_puncture_bitmap(&chan_p, chan->op_puncture_bitmap);
#endif
	chan_p.input_6g_pwr_mode = wlan_reg_conv_6g_ap_type_to_supported_6g_pwr_types(new_ap_pwr_type);

	/* Use the channel parameters and in the input power mode to find a new channel */
	*newchan = ieee80211_find_channel(ic, &chan_p, chan->ic_flags);

	/* If a valid channel is found */
	if (IEEE80211_IS_CHAN_VALID(newchan)) {
		if (!ieee80211_validate_chan_bw_in_afc_response(ic, newchan, new_ap_pwr_type)) {
			qdf_err("chan %d bw %d not present in AFC payload"
				" for SP operation", newchan->ic_freq,
				ieee80211_get_chan_width(chan));
			return QDF_STATUS_E_FAILURE;
		}

		/* If the new channel is same as the current channel, then set the
		 * new 6G AP power type, change the channel lists and restart all
		 * the vaps.
		 */
		if (IEEE80211_ARE_CHANS_EQUAL(chan, newchan)) {
			wlan_reg_set_ap_pwr_and_update_chan_list(ic->ic_pdev_obj, new_ap_pwr_type);
			mlme_update_chan_list_and_restart_vaps(ic, true);
		} else {
			/* If the new channel is different from the current channel, then
			 * set the new AP power type, update the channel list and change
			 * the current channel to the newly found channel by sending CSA.
			 */
			struct ieee80211vap *tmp_vap = TAILQ_FIRST(&ic->ic_vaps);

			wlan_reg_set_ap_pwr_and_update_chan_list(ic->ic_pdev_obj, new_ap_pwr_type);
			mlme_update_chan_list_and_restart_vaps(ic, false);
			if (!tmp_vap) {
			    qdf_err("vap is NULL\n");
			    return QDF_STATUS_E_FAILURE;
			}
			ieee80211_ucfg_set_chanswitch(tmp_vap, &chan_p, 5, ieee80211_get_chan_width(newchan));
		}
	} else {
		/* If a valid channel was not found set the best power mode */
		struct ieee80211vap *tmp_vap = TAILQ_FIRST(&ic->ic_vaps);
		uint16_t bw;
		enum ieee80211_cwm_width ch_width;
		enum phy_ch_width phy_chan_width;
		uint16_t in_punc_bitmap;

		ch_width = ic->ic_cwm_get_width(ic);
		phy_chan_width = ieee80211_convert_cwm_wd_to_phych_width(ch_width);
		bw = wlan_reg_get_bw_value(phy_chan_width);
		in_punc_bitmap = ieee80211_get_chan_param_punc_bitmap(&chan_p);
		new_ap_pwr_type = wlan_reg_get_best_pwr_mode(ic->ic_pdev_obj,
							     chan_p.freq,
							     chan_p.cfreq2, bw,
							     in_punc_bitmap);
		if (new_ap_pwr_type == REG_MAX_AP_TYPE)
		    return QDF_STATUS_E_FAILURE;

		wlan_reg_set_ap_pwr_and_update_chan_list(ic->ic_pdev_obj, new_ap_pwr_type);
		mlme_update_chan_list_and_restart_vaps(ic, false);
		if (!tmp_vap) {
		    qdf_err("vap is NULL\n");
		    return QDF_STATUS_E_FAILURE;
		}
		chan_p.input_6g_pwr_mode = new_ap_pwr_type;
		ieee80211_ucfg_set_chanswitch(tmp_vap, &chan_p, 5, ieee80211_get_chan_width(chan));
		/* send a user space event saying power mode has changed from what the
		 * user had expected */
	}
    return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(ieee80211_allow_power_mode_change);
qdf_export_symbol(mlme_check_curchan_and_set_ap_pwr_type);
qdf_export_symbol(ucfg_check_curchan_and_set_ap_pwr_type);

#endif

enum ieee80211_phymode ieee80211_get_phymode_for_ch_flags(uint64_t ch_flags)
{
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;
    bool is_2g_chan = 0;

    is_2g_chan = ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_2GHZ;

    /* 11AX channels */
    if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80_80))
        phymode = IEEE80211_MODE_11AXA_HE80_80;
    else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE160))
        phymode = IEEE80211_MODE_11AXA_HE160;
    else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80))
        phymode = IEEE80211_MODE_11AXA_HE80;
    else if (is_2g_chan) {
        if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE40MINUS)
            phymode = IEEE80211_MODE_11AXG_HE40MINUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE40PLUS)
            phymode = IEEE80211_MODE_11AXG_HE40PLUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE20)
            phymode = IEEE80211_MODE_11AXG_HE20;
    } else {
        if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE40MINUS)
            phymode = IEEE80211_MODE_11AXA_HE40MINUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE40PLUS)
            phymode = IEEE80211_MODE_11AXA_HE40PLUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE20)
            phymode = IEEE80211_MODE_11AXA_HE20;
    }

    if (phymode != IEEE80211_MODE_AUTO)
        return phymode;

    /* 11AC channels */
    if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80_80)
        phymode = IEEE80211_MODE_11AC_VHT80_80;
    else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT160)
        phymode = IEEE80211_MODE_11AC_VHT160;
    else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80)
        phymode = IEEE80211_MODE_11AC_VHT80;
    else if (!is_2g_chan) {
        if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT40MINUS)
            phymode = IEEE80211_MODE_11AC_VHT40MINUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT40PLUS)
            phymode = IEEE80211_MODE_11AC_VHT40PLUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT20)
            phymode = IEEE80211_MODE_11AC_VHT20;
    }

    if (phymode != IEEE80211_MODE_AUTO)
        return phymode;

    /* 11NG, 11G, 11B, 11NA and 11A channels*/
    if (is_2g_chan) {
        if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40MINUS)
            phymode = IEEE80211_MODE_11NG_HT40MINUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40PLUS)
            phymode = IEEE80211_MODE_11NG_HT40PLUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT20)
            phymode = IEEE80211_MODE_11NG_HT20;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_G))
            phymode = IEEE80211_MODE_11G;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_B))
            phymode = IEEE80211_MODE_11B;
    } else {
        if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40MINUS)
            phymode = IEEE80211_MODE_11NA_HT40MINUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40PLUS)
            phymode = IEEE80211_MODE_11NA_HT40PLUS;
        else if (ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT20)
            phymode = IEEE80211_MODE_11NA_HT20;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_A))
            phymode = IEEE80211_MODE_11A;
    }

    return phymode;
}

#ifdef WLAN_FEATURE_11BE
enum ieee80211_phymode ieee80211_get_phymode_for_channel_flags(
        uint64_t ch_flags)
{
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;
    bool is_2g_chan = 0;

    is_2g_chan = ch_flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_2GHZ;

    /* 11BE channels */
    if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT320))
        phymode = IEEE80211_MODE_11BEA_EHT320;
    else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT160))
        phymode = IEEE80211_MODE_11BEA_EHT160;
    else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT80))
        phymode = IEEE80211_MODE_11BEA_EHT80;
    else if (is_2g_chan) {
        if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40MINUS))
            phymode = IEEE80211_MODE_11BEG_EHT40MINUS;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40PLUS))
            phymode = IEEE80211_MODE_11BEG_EHT40PLUS;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT20))
            phymode = IEEE80211_MODE_11BEG_EHT20;
    } else {
        if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40MINUS))
            phymode = IEEE80211_MODE_11BEA_EHT40MINUS;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40PLUS))
            phymode = IEEE80211_MODE_11BEA_EHT40PLUS;
        else if (ch_flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT20))
            phymode = IEEE80211_MODE_11BEA_EHT20;
    }

    if (phymode == IEEE80211_MODE_AUTO)
        phymode = ieee80211_get_phymode_for_ch_flags(ch_flags);

    return phymode;
}
#endif /* WLAN_FEATURE_11BE */

#if defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_REG_6G_PWRMODE)
/**
 * ieee80211_is_best_pwr_mode_not_needed() - Check if best power mode calculation
 * is not needed.
 * @pdev: Pointer to pdev.
 * @ic_sta_vap: Pointer to STAVAP
 *
 * Return: True if AFC response is not received and it either is a standalone
 * wds client or a repeater. False for the rest of the scenarios.
 */
static bool ieee80211_is_best_pwr_mode_not_needed(struct wlan_objmgr_pdev *pdev,
                                                  struct ieee80211vap *ic_sta_vap)
{
    bool is_afc_resp_not_received = !wlan_reg_is_afc_power_event_received(pdev);
    bool has_a_sta_vap = ic_sta_vap;

    return is_afc_resp_not_received && has_a_sta_vap;
}

QDF_STATUS
ieee80211_apply_best_6g_ap_pwr_mode(struct ieee80211com *ic,
                                    qdf_freq_t freq,
                                    qdf_freq_t cen320,
                                    bool restart_vaps,
                                    enum ieee80211_phymode phymode,
                                    uint16_t in_puncture_bitmap)
{
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    enum reg_6g_ap_type new_ap_pwr_type;
    uint16_t bw;
    enum ieee80211_cwm_width ch_width;
    enum phy_ch_width phy_chan_width;

    /* Incase of Non-AFC capable repeater or standalone client, follow Root AP's power mode */
    if (ieee80211_is_best_pwr_mode_not_needed(pdev, ic->ic_sta_vap))
        return QDF_STATUS_SUCCESS;

    ch_width = get_chwidth_phymode(phymode);
    if (ch_width == IEEE80211_CWM_WIDTHINVALID)
        ch_width = ic->ic_cwm_get_width(ic);

    phy_chan_width = ieee80211_convert_cwm_wd_to_phych_width(ch_width);

    bw = wlan_reg_get_bw_value(phy_chan_width);
    new_ap_pwr_type = wlan_reg_get_best_pwr_mode(ic->ic_pdev_obj, freq, cen320, bw, in_puncture_bitmap);
    if (new_ap_pwr_type == REG_MAX_AP_TYPE)
        return QDF_STATUS_E_FAILURE;

    QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
              "New AP power type = %d", new_ap_pwr_type);
    /* Set the AP power type in regulatory and update the channel list */
    if (wlan_reg_set_ap_pwr_and_update_chan_list(pdev, new_ap_pwr_type) != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to set ap pwr type %d", new_ap_pwr_type);
        return QDF_STATUS_E_FAILURE;
    }

    /* Update the other channel lists and based on the input restart the vaps */
    mlme_update_chan_list_and_restart_vaps(ic, restart_vaps);

    return QDF_STATUS_SUCCESS;
}
#endif /* CONFIG_AFC_SUPPORT */

QDF_STATUS
ieee80211_quick_apply_best_6g_ap_pwr_mode(struct ieee80211com *ic,
                                          qdf_freq_t freq,
                                          qdf_freq_t cen320,
                                          bool send_tpc,
                                          enum ieee80211_phymode phymode,
                                          uint16_t in_puncture_bitmap)
{
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;
    enum reg_6g_ap_type new_ap_pwr_type;
    enum reg_6g_ap_type cur_6g_ap_pwr_type;
    uint16_t bw;
    enum ieee80211_cwm_width ch_width;
    enum phy_ch_width phy_chan_width;

    ch_width = get_chwidth_phymode(phymode);
    if (ch_width == IEEE80211_CWM_WIDTHINVALID)
        ch_width = ic->ic_cwm_get_width(ic);

    phy_chan_width = ieee80211_convert_cwm_wd_to_phych_width(ch_width);

    bw = wlan_reg_get_bw_value(phy_chan_width);
    new_ap_pwr_type = wlan_reg_get_best_pwr_mode(ic->ic_pdev_obj, freq, cen320, bw, in_puncture_bitmap);
    if (new_ap_pwr_type == REG_MAX_AP_TYPE) {
        return QDF_STATUS_E_FAILURE;
    }

    wlan_reg_get_cur_6g_ap_pwr_type(pdev, &cur_6g_ap_pwr_type);

    if (new_ap_pwr_type == cur_6g_ap_pwr_type)
        return QDF_STATUS_SUCCESS;

    QDF_TRACE(QDF_MODULE_ID_AFC, QDF_TRACE_LEVEL_INFO_LOW,
              "New AP power type = %d", new_ap_pwr_type);
    /* Set the AP power type in regulatory and update the channel list */
    if (wlan_quick_reg_set_ap_pwr_and_update_chan_list(pdev, new_ap_pwr_type) != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to set ap pwr type %d", new_ap_pwr_type);
        return QDF_STATUS_E_FAILURE;
    }

    mlme_quick_update_chan_list_and_send_tpc(ic, send_tpc);

    return QDF_STATUS_SUCCESS;
}

/**
 * reg_get_max_chwidth_for_cur_hwmode() - Get maximum channel width
 * for the current hwmode.
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @max_chwidth: Maximum channel width to be filled
 * Return - 0 on success, non-zero on failure
 */
QDF_STATUS
reg_get_max_chwidth_for_cur_hwmode(struct wlan_objmgr_pdev *pdev,
                                   enum phy_ch_width *max_chwidth)
{
    enum ieee80211_mode mode;
    enum phy_ch_width max_pdev_chwidth, reg_max_bw, min_of_reg_and_chip_bw;
    enum phy_ch_width max_hwmode_width;
    uint64_t mode_select = 0;
    enum reg_phymode phymode;
    uint16_t max_reg_bw;

    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);

    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (!ic->ic_curchan) {
        qdf_err("ic curchan is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    wlan_reg_get_max_phymode_and_chwidth(pdev, &phymode, &max_pdev_chwidth);

    max_reg_bw = wlan_reg_get_wmodes_and_max_chwidth(pdev, &mode_select,
                                                     true);
    reg_max_bw = reg_find_chwidth_from_bw(max_reg_bw);

    if (reg_max_bw == CH_WIDTH_INVALID) {
        qdf_err("Unable to fetch the max reg bw\n");
        return QDF_STATUS_E_FAILURE;
    }

    min_of_reg_and_chip_bw = qdf_min(max_pdev_chwidth, reg_max_bw);
    mode = get_mode_from_phymode(ieee80211_chan2mode(ic->ic_curchan));

    switch (mode) {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_BEG:
#endif
        case IEEE80211_MODE_AXG:
            max_hwmode_width = CH_WIDTH_40MHZ;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_BEA:
            max_hwmode_width = CH_WIDTH_320MHZ;
            break;
#endif
        case IEEE80211_MODE_AXA:
            max_hwmode_width = CH_WIDTH_160MHZ;
            break;
        case IEEE80211_MODE_AC:
            max_hwmode_width = CH_WIDTH_160MHZ;
            break;
        case IEEE80211_MODE_N:
        case IEEE80211_MODE_NG:
        case IEEE80211_MODE_NA:
            max_hwmode_width = CH_WIDTH_40MHZ;
            break;
        case IEEE80211_MODE_G:
        case IEEE80211_MODE_B:
        case IEEE80211_MODE_A:
            max_hwmode_width = CH_WIDTH_20MHZ;
            break;
        default:
            max_hwmode_width = CH_WIDTH_INVALID;
    }

    if (max_hwmode_width == CH_WIDTH_INVALID)
        return QDF_STATUS_E_FAILURE;

    *max_chwidth = qdf_min(max_hwmode_width, min_of_reg_and_chip_bw);

    return QDF_STATUS_SUCCESS;

}

/**
 * reg_is_80p80_supp_by_modecaps() - Find out if 80p80 is supported
 * by the modecaps.
 * @ pdev: Pointer to struct wlan_objmgr_pdev
 * Return - True if 80p80 is supported, false otherwise.
 */
static bool
reg_is_80p80_supp_by_modecaps(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);

    if (!ic) {
        qdf_err("ic is NULL");
        return false;
    }

    if ((ic->ic_modecaps & (1 << IEEE80211_MODE_11AC_VHT80_80)) ||
        (ic->ic_modecaps & (1ULL << IEEE80211_MODE_11AXA_HE80_80)))
        return true;

    return false;

}

void regdmn_get_opclass_for_hwmode(struct wlan_objmgr_pdev *pdev,
                                   wlan_radio_basic_capabilities *apcap,
                                   bool global_tbl_lookup)
{
    uint8_t total_n_sup_opclass = 0;
    uint8_t idx = 0, n_opclasses = 0, max_supp_op_class = REG_MAX_SUPP_OPER_CLASSES;
    struct regdmn_ap_cap_opclass_t *reg_ap_cap =  NULL;
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);
    QDF_STATUS status;
    bool is_80p80_supp;
    enum phy_ch_width max_chwidth_for_cur_hwmode;
    uint8_t vdev_count;

    if (!ic) {
        qdf_err("ic is NULL");
        return;
    }

    reg_ap_cap = qdf_mem_malloc(max_supp_op_class * sizeof(*reg_ap_cap));

    if (!reg_ap_cap) {
        qdf_err("Failed to allocate reg_ap_cap");
        return;
    }

    status = reg_get_max_chwidth_for_cur_hwmode(pdev,
                                                &max_chwidth_for_cur_hwmode);

    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get max chan width for curhwmode");
        qdf_mem_free(reg_ap_cap);
        return;
    }

    is_80p80_supp = reg_is_80p80_supp_by_modecaps(pdev);

    status = wlan_reg_get_opclass_for_cur_hwmode(pdev, reg_ap_cap, &n_opclasses,
                                                 max_supp_op_class,
                                                 global_tbl_lookup,
                                                 max_chwidth_for_cur_hwmode,
                                                 is_80p80_supp,
                                                 REG_CURRENT_PWR_MODE);

    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to get opclass details");
        qdf_mem_free(reg_ap_cap);
        return;
    }

    while (AP_CAP.op_class && (idx < n_opclasses)) {
        if (apcap)
            regdmn_fill_apcap(apcap, reg_ap_cap, idx,
                              &total_n_sup_opclass);

        idx++;
    }

    if (apcap) {
        apcap->num_supp_op_classes = total_n_sup_opclass;
        vdev_count = wlan_pdev_get_max_vdev_count(pdev);
        apcap->max_supp_bss = vdev_count ? vdev_count - 1 : 0;
    }

    qdf_mem_free(reg_ap_cap);
    return;
}

int ieee80211_reg_get_num_ap_vaps(struct ieee80211com *ic)
{
    struct ieee80211vap *vap;
    int num_ap_vaps = 0;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
        if (vap->iv_opmode == IEEE80211_M_HOSTAP)
            num_ap_vaps++;

    return num_ap_vaps;
}

bool ieee80211_reg_is_pdev_ap_no_sta(struct ieee80211com *ic)
{
    int num_ap_vaps = ieee80211_reg_get_num_ap_vaps(ic);

    return (num_ap_vaps > 0) && !ic_is_sta_vap(ic);
}

bool ieee80211_reg_is_repeater_pdev(struct ieee80211com *ic)
{
    int num_ap_vaps = ieee80211_reg_get_num_ap_vaps(ic);

    return (num_ap_vaps > 0) && ic_is_sta_vap(ic);
}

#ifdef CONFIG_AFC_SUPPORT
int ieee80211_get_dev_deployment_type(
        struct wlan_objmgr_pdev *pdev,
        enum reg_afc_dev_deploy_type *reg_afc_dev_type)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_reg_rx_ops *reg_rx_ops = NULL;
    int ret = -EINVAL;

    if (!pdev || !reg_afc_dev_type) {
        qdf_err("pdev or reg_afc_dev_type is null");
        goto exit;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        qdf_err("psoc is null");
        goto exit;
    }

    reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
    if (!(reg_rx_ops && reg_rx_ops->reg_get_afc_dev_type)) {
        qdf_err("reg_rx_ops is NULL");
        goto exit;
    }

    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_REGULATORY_SB_ID) !=
            QDF_STATUS_SUCCESS) {
        qdf_err("unable to take ref for pdev");
        goto exit;
    }

    if (reg_rx_ops->reg_get_afc_dev_type(psoc, reg_afc_dev_type) ==
            QDF_STATUS_SUCCESS)
        ret = 0;

    wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);

exit:
    return ret;
}
qdf_export_symbol(ieee80211_get_dev_deployment_type);
#endif

int regdmn_enable_disable_opclass_chans(struct ieee80211com *ic,
                                        bool is_disable,
                                        uint8_t opclass,
                                        uint8_t *ieee_chan_list,
                                        uint8_t chan_list_size)
{
     QDF_STATUS status;

     status = ucfg_reg_enable_disable_opclass_chans(ic->ic_pdev_obj,
                                                    is_disable, opclass,
                                                    ieee_chan_list,
                                                    chan_list_size,
                                                    true);
     if (status != QDF_STATUS_SUCCESS)
         return -EINVAL;

     ieee80211_update_channellist(ic, 1, false);
#if UMAC_SUPPORT_CFG80211
     wlan_cfg80211_update_channel_list(ic);
#endif

     return EOK;
}

#ifdef QCA_R2P_UPDATE_ENABLED
/**
 * parse_r2p_cmn_header: Parse the common part of r2p table info to be sent
 * through WMI
 * @r2p_table: Pointer to r2p table
 * @buff: Input buff with home chan r2p table
 * @buff_size_read: Size of contents read from the buffer so far
 *
 * Return - void
 */
static void parse_r2p_cmn_header(ath_power_table_t *r2p_table, char **buff,
                                 u_int32_t *buff_size_read)
{
    memcpy(&r2p_table->freq_band, *buff, sizeof(r2p_table->freq_band));
    *buff += sizeof(r2p_table->freq_band);
    *buff_size_read += sizeof(r2p_table->freq_band);
    memcpy(&r2p_table->target_type, *buff, sizeof(r2p_table->target_type));
    *buff += sizeof(r2p_table->target_type);
    *buff_size_read += sizeof(r2p_table->target_type);
}

/**
 * parse_r2p_sub_band_info: Parse the sub band info part of r2p table info to
 * to be sent through WMI
 * @r2p_table: Pointer to r2p table
 * @buff: Input buff with home chan r2p table
 * @buff_size_read: Size of contents read from the buffer so far
 *
 * Return - void
 */
static void parse_r2p_sub_band_info(ath_power_table_t *r2p_table, char **buff,
                                    u_int32_t *buff_size_read)
{
    memcpy(&r2p_table->sub_band, *buff, sizeof(r2p_table->sub_band));
    *buff += sizeof(r2p_table->sub_band);
    *buff_size_read += sizeof(r2p_table->sub_band);
    memcpy(&r2p_table->is_ext, *buff, sizeof(r2p_table->is_ext));
    *buff += sizeof(r2p_table->is_ext);
    *buff_size_read += sizeof(r2p_table->is_ext);
    memcpy(&r2p_table->end_of_update, *buff, sizeof(r2p_table->end_of_update));
    /**
     * Set the end_of_update flag as 0 to apply the power values only when
     * switching to home channel.
     */
    r2p_table->end_of_update = 0;
    *buff += sizeof(r2p_table->end_of_update);
    *buff_size_read += sizeof(r2p_table->end_of_update);
    memcpy(&r2p_table->len, *buff, sizeof(r2p_table->len));
    *buff += sizeof(r2p_table->len);
    *buff_size_read += sizeof(r2p_table->len);
    memcpy(&r2p_table->pwr_tbl[0], *buff, r2p_table->len);
    *buff += r2p_table->len;
    *buff_size_read += r2p_table->len;
}

QDF_STATUS ieee80211_send_homechan_r2p_table(struct ol_ath_softc_net80211 *scn,
                                             char *buff, u_int32_t buff_size,
                                             ol_ath_ucfg_rate2power_set_cb rate2power_set_cb)
{
    ath_power_table_t *r2p_table;
    u_int32_t buff_size_read = 0;

    if (!buff_size)
        return QDF_STATUS_E_FAILURE;

    r2p_table = qdf_mem_malloc(sizeof(ath_power_table_t));
    if (!r2p_table)
        return QDF_STATUS_E_NOMEM;

    parse_r2p_cmn_header(r2p_table, &buff, &buff_size_read);
    do {
        parse_r2p_sub_band_info(r2p_table, &buff, &buff_size_read);
        rate2power_set_cb(scn, r2p_table);
    } while (r2p_table->end_of_update != 1 && buff_size_read < buff_size);
    qdf_mem_free(r2p_table);

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ieee80211_send_homechan_r2p_table);
#endif

#ifdef WLAN_FEATURE_11BE
uint16_t ieee80211_reg_get_radar_punc_bitmap(struct ieee80211com *ic,
                                             struct ieee80211_ath_channel *chan)
{
    qdf_freq_t sub_chan_freqs[MAX_NUM_20MHZ_SUBCHANS] = {};
    uint8_t nchans;
    uint8_t i;
    uint16_t radar_punc_bitmap = PUNCTURE_NONE;

    if (!IEEE80211_IS_CHAN_VALID(chan) || !IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan))
        return radar_punc_bitmap;

    ieee80211_get_subchan(chan, sub_chan_freqs, &nchans);
    for (i = 0; i < nchans; i++) {
        if (wlan_reg_is_nol_for_freq(ic->ic_pdev_obj, sub_chan_freqs[i])) {
            radar_punc_bitmap |=  1 << i;
        }
    }
    return radar_punc_bitmap;
}
#endif

static void
ieee80211_print_legacy_ht_vht_chan_flags(uint64_t flags, char *buf, size_t buf_size)
{
    if (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_FHSS)) {
        if(strlcat(buf, " FHSS", buf_size) >= buf_size)
           goto too_long;
    }

    if ((flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_5GHZ) &&
       ((flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT20) ||
       (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40PLUS) ||
       (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40MINUS))) {
        if(strlcat(buf, " 11na", buf_size) >= buf_size)
           goto too_long;
    } else if (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_A)) {
               if(strlcat(buf, " 11a", buf_size) >= buf_size)
                  goto too_long;
    }else if ((flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_2GHZ) &&
             ((flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT20) ||
             (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40PLUS) ||
             (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40MINUS))) {
              if(strlcat(buf, " 11ng", buf_size) >= buf_size)
                 goto too_long;
    } else if ((flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_G)) ||
              (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_PUREG))) {
               /* XXX 11g schizophrenia */
               if(strlcat(buf, " 11g", buf_size) >= buf_size)
                  goto too_long;
    } else if (flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_B)) {
               if(strlcat(buf, " 11b", buf_size) >= buf_size)
                  goto too_long;
    }

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_TURBO)
        if(strlcat(buf, " Turbo", buf_size) >= buf_size)
           goto too_long;

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT20)
        if(strlcat(buf, " C", buf_size) >= buf_size)
           goto too_long;

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40PLUS)
        if(strlcat(buf, " CU", buf_size) >= buf_size)
           goto too_long;

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HT40MINUS)
        if(strlcat(buf, " CL", buf_size) >= buf_size)
           goto too_long;

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT20)
        if(strlcat(buf, " V", buf_size) >= buf_size)
           goto too_long;

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT40PLUS)
        if(strlcat(buf, " VU", buf_size) >= buf_size)
           goto too_long;

    if (flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT40MINUS)
        if(strlcat(buf, " VL", buf_size) >= buf_size)
           goto too_long;
    return;
too_long:
    qdf_print("Output buffer size insufficient\n");
}

static void ieee80211_print_he_chan_flags(uint64_t flags, char *buf, size_t buf_size)
{
    if(flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE20)
       if(strlcat(buf, " H", buf_size) >= buf_size)
          goto too_long;

    if(flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE40PLUS)
       if(strlcat(buf, " HU", buf_size) >= buf_size)
          goto too_long;

    if(flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE40MINUS)
       if(strlcat(buf, " HL", buf_size) >= buf_size)
          goto too_long;
    return;

too_long:
    qdf_print("Output buffer size insufficient\n");
}

#ifdef WLAN_FEATURE_11BE
static void ieee80211_print_channels(struct ieee80211_channel_params *c)
{
    char buf[MAX_BUF_SIZE];
    char buf1[MAX_BUF1_SIZE];
    char buf3[MAX_BUF3_SIZE];
    size_t buf_size = sizeof(buf);

    buf[0] = '\0';

    ieee80211_print_legacy_ht_vht_chan_flags(c->flags, buf, buf_size);

    if (c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80) {
        if(strlcat(buf, " V80-", sizeof(buf)) >= sizeof(buf))
           goto too_long;
        snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg1);
        if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
           goto too_long;
    }

    if (c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT160) {
        if (c->vhtop_ch_num_seg2) {
            if(strlcat(buf, " V160-", sizeof(buf)) >= sizeof(buf))
               goto too_long;
            snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg2[INDEX_160_SEG2_CENTER]);
            if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
               goto too_long;
        }
    }

    ieee80211_print_he_chan_flags(c->flags, buf, buf_size);

    if(c->flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80)) {
        if(strlcat(buf, " H80-", sizeof(buf))  >= sizeof(buf))
           goto too_long;
        snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg1);
        if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
           goto too_long;
    }

    if(c->flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE160)) {
        if (c->vhtop_ch_num_seg2) {
            if(strlcat(buf, " H160-", sizeof(buf))  >= sizeof(buf))
               goto too_long;
            snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg2[INDEX_160_SEG2_CENTER]);
            if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
               goto too_long;
        }
    }
    if(c->flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT20))
        if(strlcat(buf, " E", sizeof(buf)) >= sizeof(buf))
           goto too_long;

    if(c->flags &
            VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40PLUS))
        if(strlcat(buf, " EU", sizeof(buf)) >= sizeof(buf))
           goto too_long;

    if(c->flags &
            VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40MINUS))
        if(strlcat(buf, " EL", sizeof(buf)) >= sizeof(buf))
           goto too_long;

    if(c->flags &
            VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT80)) {
        if(strlcat(buf, " E80-", sizeof(buf)) >= sizeof(buf))
           goto too_long;
        snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg1);
        if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
           goto too_long;
    }

    if(c->flags &
            VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT160)) {
        if (c->vhtop_ch_num_seg2) {
            if(strlcat(buf, " E160-", sizeof(buf)) >= sizeof(buf))
               goto too_long;
            snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg2[INDEX_160_SEG2_CENTER]);
            if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
               goto too_long;
        }
    }

    if(c->flags &
            VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT320)) {
        if (c->vhtop_ch_num_seg2) {
            if(strlcat(buf, " E320-", sizeof(buf)) >= sizeof(buf))
               goto too_long;
            snprintf(buf1, sizeof(buf1), "%3u",c->vhtop_ch_num_seg2[INDEX_320_SEG2_CENTER1]);
            if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
               goto too_long;
        }
        if (c->vhtop_ch_num_seg2) {
            snprintf(buf3, sizeof(buf3), ", %3u",c->vhtop_ch_num_seg2[INDEX_320_SEG2_CENTER2]);
            if(strlcat(buf, buf3, sizeof(buf)) >= sizeof(buf))
               goto too_long;
        }
    }
    /* XXX For now we use an optimized column width of 60. But can be
     * increased to say, 72 if required in the future.
     */
    qdf_nofl_print("Channel %3u : %u%c%c%c Mhz%-90.90s",
           c->ieee, c->freq,
           ((c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HALF) ? 'H' :
           ((c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_QUARTER) ? 'Q' :  ' ')),
           (c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_PASSIVE) ? '*' : ' ',
           (c->flags_ext & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EXT_DFS) ?'~':' ', buf);
    return;
too_long:
    qdf_print("Output buffer size insufficient\n");
}
#else
static void ieee80211_print_channels(struct ieee80211_channel_info *c,
                                     struct ieee80211_channel_info *c_160)
{
    char buf[MAX_BUF_SIZE];
    char buf1[MAX_BUF1_SIZE];
    size_t buf_size = sizeof(buf);

    buf[0] = '\0';
    ieee80211_print_legacy_ht_vht_chan_flags(c->flags, buf, buf_size);

    if (c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80) {
        if(strlcat(buf, " V80-", sizeof(buf)) >= sizeof(buf))
           goto too_long;
        snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg1);
        if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
           goto too_long;
    }

    if (c_160->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT160) {
            if(strlcat(buf, " V160-", sizeof(buf)) >= sizeof(buf))
               goto too_long;
            snprintf(buf1, sizeof(buf1), "%3u", c_160->vhtop_ch_num_seg2);
            if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
               goto too_long;
    }

    ieee80211_print_he_chan_flags(c->flags, buf, buf_size);

    if(c->flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80)) {
        if(strlcat(buf, " H80-", sizeof(buf))  >= sizeof(buf))
           goto too_long;
        snprintf(buf1, sizeof(buf1), "%3u", c->vhtop_ch_num_seg1);
        if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
           goto too_long;
    }

    if(c_160->flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE160)) {
            if(strlcat(buf, " H160-", sizeof(buf))  >= sizeof(buf))
               goto too_long;
            snprintf(buf1, sizeof(buf1), "%3u", c_160->vhtop_ch_num_seg2);
            if(strlcat(buf, buf1, sizeof(buf)) >= sizeof(buf))
               goto too_long;
    }
    /* XXX For now we use an optimized column width of 60. But can be
     * increased to say, 72 if required in the future.
     */
    qdf_nofl_print("Channel %3u : %u%c%c%c Mhz%-90.90s",
          c->ieee, c->freq,
          ((c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HALF) ? 'H' :
          ((c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_QUARTER) ? 'Q' :  ' ')),
          (c->flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_PASSIVE) ? '*' : ' ',
          (c->flags_ext & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EXT_DFS) ?'~':' ', buf);
    return;
too_long:
    qdf_print("Output buffer size insufficient\n");
}
#endif

#ifdef WLAN_FEATURE_11BE
void ieee80211_display_channel(struct ieee80211com *ic)
{
      struct ieee80211_ath_channel chan;
      uint16_t freq;
      enum wlan_band_id band;
      uint32_t i;
      struct ieee80211req_chaninfo *chans = NULL;
      struct ieee80211_channel_list_info *chanlist_info = NULL;
      u_int8_t is_vht80p80_available = 0;
      u_int8_t is_he80p80_available = 0;

      chan = wlan_get_dev_current_channel(ic);
      if (!IEEE80211_IS_CHAN_VALID(&chan)) {
          qdf_err("Invalid channel");
          return;
      } else {
          freq = wlan_channel_frequency(&chan);
      }
      band = ieee80211_get_ath_channel_band(freq);

      chans = (struct ieee80211req_chaninfo *)qdf_mem_malloc(sizeof(*chans));
      if (!chans) {
          qdf_err("NULL chinfo");
          return;
      }

      chanlist_info = (struct ieee80211_channel_list_info *)qdf_mem_malloc(
            sizeof(*chanlist_info));
      if (!chanlist_info) {
          qdf_err("NULL chanlist_info");
          qdf_mem_free(chans);
          return;
      }

      ieee80211_get_radio_channel_list(ic, chans->ic_chans, chanlist_info->chans,
                                       &chanlist_info->nchans);

      for( i = 0; i < chanlist_info->nchans; i++ )
      {
          if (band == ieee80211_get_ath_channel_band(chanlist_info->chans[i].freq))
              ieee80211_print_channels(&chanlist_info->chans[i]);

          if (chanlist_info->chans[i].flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80_80)
              is_vht80p80_available = 1;

          if (chanlist_info->chans[i].flags &
              VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80_80))
               is_he80p80_available = 1;
      }

      if (is_vht80p80_available) {
          qdf_nofl_print("\nV80_80 capable. Pri20: Any V80 pri20 above. Sec80 centre: "
                         "Centre of any V80 block discontiguous with primary 80.\n");
      }

      if (is_he80p80_available) {
          if (!is_vht80p80_available) {
              /* Note that this is improbable. But check added for completeness.
               */
              qdf_nofl_print("\n");
          }

          qdf_nofl_print("H80_80 capable. Pri20: Any H80 pri20 above. Sec80 centre: "
                "Centre of any H80 block discontiguous with primary 80.\n");
      }

      qdf_mem_free(chans);
      qdf_mem_free(chanlist_info);
}
#else
void ieee80211_display_channel(struct ieee80211com *ic)
{
      struct ieee80211_ath_channel chan;
      uint16_t freq;
      enum wlan_band_id band;
      uint32_t i;
      struct ieee80211req_chaninfo *chans = NULL;
      struct ieee80211req_channel_list *chlist = NULL;
      struct ieee80211req_chaninfo *chans_160 = NULL;
      struct ieee80211req_channel_list *chlist_160 = NULL;
      u_int8_t is_vht80p80_available = 0;
      u_int8_t is_he80p80_available = 0;

      chan = wlan_get_dev_current_channel(ic);
      if (!IEEE80211_IS_CHAN_VALID(&chan)) {
          qdf_err("Invalid channel");
          return;
      } else {
          freq = wlan_channel_frequency(&chan);
      }
      band = ieee80211_get_ath_channel_band(freq);

      chans = (struct ieee80211req_chaninfo *)qdf_mem_malloc(sizeof(*chans));
      if (!chans) {
          qdf_err("NULL chinfo");
          return;
      }

      chlist = (struct ieee80211req_channel_list *)qdf_mem_malloc(sizeof(*chlist));
      if (!chlist) {
          qdf_err("NULL chlist");
          qdf_mem_free(chans);
          return;
      }

      ieee80211_get_channel_list(ic, chans->ic_chans, chlist->chans,
                                 &chlist->nchans, 0);

      chans_160 = (struct ieee80211req_chaninfo *)qdf_mem_malloc(sizeof(*chans_160));
      if (!chans_160) {
          qdf_err("NULL chinfo");
          return;
      }

      chlist_160 = (struct ieee80211req_channel_list *)qdf_mem_malloc(sizeof(*chlist_160));
      if (!chlist) {
          qdf_err("NULL chlist");
          qdf_mem_free(chans_160);
          return;
      }

      ieee80211_get_channel_list(ic, chans_160->ic_chans, chlist_160->chans,
                                 &chlist_160->nchans, 1);

      for( i = 0; i < chlist->nchans; i++ )
      {
          if (band == ieee80211_get_ath_channel_band(chlist->chans[i].freq))
              ieee80211_print_channels(&chlist->chans[i], &chlist_160->chans[i]);

          if (chlist->chans[i].flags & QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_VHT80_80)
              is_vht80p80_available = 1;

          if (chlist->chans[i].flags & VENDOR_CHAN_FLAG2(QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_HE80_80))
              is_he80p80_available = 1;
      }

      if (is_vht80p80_available) {
          qdf_nofl_print("\nV80_80 capable. Pri20: Any V80 pri20 above. Sec80 centre: "
                       "Centre of any V80 block discontiguous with primary 80.\n");
      }

      if (is_he80p80_available) {
          if (!is_vht80p80_available) {
              /* Note that this is improbable. But check added for completeness.
               */
              qdf_nofl_print("\n");
          }

           qdf_nofl_print("H80_80 capable. Pri20: Any H80 pri20 above. Sec80 centre: "
                "Centre of any H80 block discontiguous with primary 80.\n");
      }

      qdf_mem_free(chans);
      qdf_mem_free(chlist);
      qdf_mem_free(chans_160);
      qdf_mem_free(chlist_160);
}
#endif
qdf_export_symbol(ieee80211_display_channel);

#ifdef WLAN_FEATURE_11BE
uint16_t
ieee80211_intersect_puncture_bitmap_for_40(struct ieee80211_ath_channel *chan,
                                           enum ieee80211_phymode input_phymode)
{
    uint16_t punc_40_bitmap;
    uint16_t punc_80_bitmap =
        ieee80211_intersect_puncture_bitmap(chan, IEEE80211_MODE_11BEA_EHT80,
                                            input_phymode);
    const struct bonded_channel_freq *bonded_chan_ptr;
    qdf_freq_t chan_80_center;

    if (!punc_80_bitmap)
        return PUNCTURE_NONE;

    bonded_chan_ptr = wlan_reg_get_bonded_chan_entry(chan->ic_freq, CH_WIDTH_80MHZ, 0);
    if (!bonded_chan_ptr) {
        qdf_nofl_print("NULL pointer for frequency %u for channel width 80MHz", chan->ic_freq);
        return PUNCTURE_NONE;
    }

    chan_80_center = (bonded_chan_ptr->start_freq + bonded_chan_ptr->end_freq) / 2;
    if (chan->ic_freq < chan_80_center)
        punc_40_bitmap = punc_80_bitmap & 0x3;
    else
        punc_40_bitmap = (punc_80_bitmap >> 2) & 0x3;

    return punc_40_bitmap;
}
#endif

uint8_t ieee80211_is_chwidth_supported_by_reg(struct ieee80211com *ic,
                                             enum phy_ch_width in_chwidth)
{
    QDF_STATUS status;
    enum phy_ch_width max_reg_chwidth;
    int in_chwidth_val, max_reg_chwidth_val;

    if ((!ieee80211_is_chwidth_valid(in_chwidth))) {
        return false;
    }

    status = reg_get_max_chwidth_for_cur_hwmode(ic->ic_pdev_obj,&max_reg_chwidth);
    if ((status != QDF_STATUS_SUCCESS)
        || (!ieee80211_is_chwidth_valid(max_reg_chwidth))) {
        return false;
    }

    in_chwidth_val = ieee80211_get_chwidth_val(in_chwidth);
    max_reg_chwidth_val = ieee80211_get_chwidth_val(max_reg_chwidth);
    if (max_reg_chwidth_val >= in_chwidth_val) {
        return true;
    }

    return false;

}
