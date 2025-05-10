/*
 * Copyright (c) 2011, 2017-2019, 2021, 2023-2024 Qualcomm Innovation Center, Inc.
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

#ifndef _NET80211_IEEE80211_CHAN_H
#define _NET80211_IEEE80211_CHAN_H

#include <ieee80211_var.h>

/**
 * ieee80211_copy_ch_params(): copy ch_params from source to target.
 * @tgt_ch_params: target ch_params.
 * @src_ch_params: source ch_params.
 *
 * return: nothing.
 */
#ifdef WLAN_FEATURE_11BE
void ieee80211_copy_ch_params(struct ch_params *tgt_ch_params,
                              struct ch_params *src_ch_params);
#else
static inline
void ieee80211_copy_ch_params(struct ch_params *tgt_ch_params,
                              struct ch_params *src_ch_params)
{
}
#endif

/* enum to define secondary 20 channel offset
 * @EXT_CHAN_OFFSET_NA: no extension channel is present
 * @EXT_CHAN_OFFSET_ABOVE: above control channel
 * @EXT_CHAN_OFFSET_BELOW: below control channel
 */
enum sec20_chan_offset {
        EXT_CHAN_OFFSET_NA    = 0,
        EXT_CHAN_OFFSET_ABOVE = 1,
        EXT_CHAN_OFFSET_BELOW = -1
};

/* 1:- chan is punctured, 0:- chan available */
#define NO_PUNCTURE_BITMASK 0x0000

#define CHAN_MIN_PSD_POWER  -128
#define CHAN_MAX_PSD_POWER   127
#define CHAN_MIN_EIRP_POWER (CHAN_MIN_PSD_POWER)
#define CHAN_MAX_EIRP_POWER (CHAN_MAX_PSD_POWER)

#define IS_CHAN_BIT_PUNCTURED(_punc_bitmap, chan_idx) \
    ((_punc_bitmap) & BIT(chan_idx))
#define NUM_20_MHZ_CHAN_IN_160_MHZ    8

/*
 * Internal API's for channel/freq/PHY_mode handling
 */

enum ieee80211_phymode ieee80211_chan2mode(const struct ieee80211_ath_channel *chan);

/**
 * ieee80211_find_channel: finds channel along with operating puncture pattern
 * @ic: Pointer to ic
 * @chan_p: chan_params for finding the channel
 * @flags: channel flags
 *
 * return: returns channel with operating puncture_bitmap
 */
struct ieee80211_ath_channel ieee80211_find_channel(struct ieee80211com *ic, struct chan_params *chan_p, u_int64_t flags);
/**
 * wlan_update_current_mode_caps() - Update current mode capabilities based on
 * ic_channels list. Do not populate the radar found channel capabilities.
 * @ic: Pointer to ieee80211com structure.
 */
void wlan_update_current_mode_caps(struct ieee80211com *ic);

/*
 * ieee80211_find_channel_for_ap_pwr_type:
 * Retrieve a channel from a given channel list
 *
 * @ic: Pointer to ic
 * @chan_list: Pointer to chan_list
 * @freq: Value of the frequency
 * @cfreq2: Value of the segment 2 center frequency
 * @mode: Value of the given phymode
 *
 * Return:
 * ieee80211_ath_channel structure
 */
struct ieee80211_ath_channel
ieee80211_find_channel_for_ap_pwr_type(struct ieee80211com *ic,
                                       bool use_cur_chan_list,
                                       enum reg_6g_ap_type ap_pwr_type,
                                       struct chan_params *chan_p,
                                       enum ieee80211_phymode mode);

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
bool ieee80211_check_allowed_prim_freqlist(struct ieee80211com *ic, uint16_t freq);
#endif

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
                                           enum supported_6g_pwr_types ap_pwr_type);

enum ieee80211_phymode ieee80211_get_phy_mode(struct ieee80211com *ic,
                                        struct ieee80211_ath_channel *bss_chan,
                                        enum ieee80211_phymode des_mode, enum ieee80211_phymode bss_mode);

/* ieee80211_find_connect_channel() - Find the channel for connection by
 * intersecting scan chan params and desired mode. Additionally for 5GHz band,
 * this API tries to find a non-radar channel with reduced channel width.
 *
 * @vap: Pointer to structure ieee80211vap.
 * @ni_chan: Input scan channel
 * @intersected_phymode: Output intersected phymode for connection.
 * @is_input_phymode_radar: 5GHz specific flag to indicate if scan channel
 * was affected by radar
 * @ieee80211_msg_debug_id: ieee80211 message level
 *
 * Return: Channel with intersected channel params.
 */
struct ieee80211_ath_channel
ieee80211_find_connect_channel(struct ieee80211vap *vap,
                               struct ieee80211_ath_channel *ni_chan,
                               enum ieee80211_phymode *intersected_phymode,
                               bool *is_input_phymode_radar,
                               uint8_t ieee80211_msg_debug_id);

/*
 * ieee80211_find_dot11_channel_without_radar() - Try and find a channel
 * without radar for given channel params and phymode
 *
 * @ic: Pointer to struct ieee80211com
 * @chan_p: Pointer to chan params. Center Frequency and input puncture bitmap
 * fields may be modified by this API.
 * @in_phymode: Input Phymode
 * @is_input_phymode_radar: Set to true if the input phymode has radar channel
 *
 * Return:
 * Channel with input chan params.
 * empty_channel if no valid channel found.
 */
struct ieee80211_ath_channel
ieee80211_find_dot11_channel_without_radar(struct ieee80211com *ic,
                                           struct chan_params *chan_p,
                                           enum ieee80211_phymode phymode,
                                           bool *is_input_phymode_radar);

/* ieee80211_get_non_auto_des_mode() - Get the final desired mode based on scan channel
 * params and vap desired mode. If the Vap desired mode is Auto, return the
 * maximum mode which is supported by regulatory
 *
 * @chan_p: Chan parms filled from scan entry.
 * @vap: Pointer to structure ieee80211vap.
 *
 * Return: Non-AUTO phymode.
 */
enum ieee80211_phymode
ieee80211_get_non_auto_des_mode(struct chan_params *chan_p, struct ieee80211vap *vap);

/**
 * ieee80211_find_ch_center() - Find the channel center frequency for
 * the given input primary freq and BW
 * @ic: Pointer to struct ieee80211com
 * @primary_freq: Primary frequency in MHz
 * @center_320: 320 MHz center frequency
 * @chan_bw: Channel BW in MHz
 * @punc_pattern: Puncture pattern
 *
 * Return: Channel center frequency in MHz
 */
#ifdef WLAN_FEATURE_11BE
qdf_freq_t
ieee80211_find_ch_center(struct ieee80211com *ic, qdf_freq_t primary_freq,
                         qdf_freq_t center_320, uint16_t chan_bw,
                         uint16_t punc_pattern);
#else
static inline qdf_freq_t
ieee80211_find_ch_center(struct ieee80211com *ic, qdf_freq_t primary_freq,
                         qdf_freq_t center_320, uint16_t chan_bw,
                         uint16_t punc_pattern)
{
    return 0;
}
#endif

void ieee80211_dcs_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *channel);

#ifdef CONFIG_AFC_SUPPORT
QDF_STATUS ieee80211_dcs_afc_trigger(struct ieee80211com *ic);

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
                              enum ieee80211_phymode mode);
#endif

void ieee80211_channel_notify_to_app(struct ieee80211com *ic);

/**
 * ieee80211_verify_bw_for_channel:
 * Verify if a given channel can form a given bandwidth
 *
 * @chan_list        : Pointer to a given channel list
 * @channel          : Pointer to a given channel
 *
 * Return:
 * QDF_STATUS_SUCCESS: Channel can form the given bandwidth
 * QDF_STATUS_E_*    : Channel cannot form the given bandwidth
 */
QDF_STATUS ieee80211_verify_bw_for_channel(struct regulatory_channel *chan_list,
                                           struct ieee80211_ath_channel *channel);

struct ieee80211_ath_channel ieee80211_doth_findchan(struct ieee80211vap *vap, struct chan_params *chan_p);

/**
 * ieee80211_find_dot11_channel: finds channel along with operating puncture pattern
 * ic: Pointer to ic
 * chan_p: chan_params for finding the channel
 * mode: phymode oof the channel
 * return: returns channel with operating puncture_bitmap
 */
struct ieee80211_ath_channel ieee80211_find_dot11_channel(struct ieee80211com *ic, struct chan_params *chan_p, enum ieee80211_phymode mode);
int ieee80211_set_channel_for_cc_change(struct ieee80211com *ic, struct ieee80211_ath_channel *chan);
struct ieee80211_ath_channel ieee80211_find_puncture_bitmap(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, struct chan_params *chan_p);

/**
 * ieee80211_reg_find_first_valid_freq: finds first valid freq.
 * ic: Pointer to ic
 * mode: phymode oof the channel
 * @in_6g_pwr_mode: Input 6G power type which will determine the 6G channel
 *                  list on which the primary_freq is searched for.
 *
 * return: returns first valid freq.
 */
qdf_freq_t
ieee80211_reg_find_first_valid_freq(struct ieee80211com *ic,
                                    enum ieee80211_phymode mode,
                                    enum supported_6g_pwr_types in_6g_pwr_mode);

/**
 * ieee80211_setctry_tryretaining_curchan() - Set the country and try to retain
 * the input channel.
 * @ic: Pointer to ieee80211com structure.
 * @chan: Pointer to the channel that needs to be retained after setting the
 * country.
 */
int ieee80211_setctry_tryretaining_curchan(struct ieee80211com *ic, struct ieee80211_ath_channel *chan);

bool ieee80211_is_same_frequency_band(const struct ieee80211_ath_channel *chan1, const struct ieee80211_ath_channel *chan2);
int ieee80211_update_channellist(struct ieee80211com *ic, int exclude_11d,
                                 bool no_chanchange);
bool ieee80211_update_custom_scan_chan_list(struct ieee80211vap *vap, bool associated);
struct ieee80211_ath_channel *ieee80211_autoselect_adhoc_channel(struct ieee80211com *ic);
int ieee80211_set_channel(struct ieee80211com *ic, struct ieee80211_ath_channel *chan);
int ieee80211_setmode(struct ieee80211com *ic, enum ieee80211_phymode mode, enum ieee80211_opmode opmode);

/*
 * ieee80211_chan2freq() - Convert channel to frequency value.
 * @ic: Pointer to ieee80211com structure
 * @c: Pointer to ieee80211_ath_channel structure
 *
 * Return: Frequency value on successful conversion
 *         "0" otherwise
 */
u_int16_t
ieee80211_chan2freq(struct ieee80211com *ic, const struct ieee80211_ath_channel *c);
struct ieee80211_ath_channel ieee80211_get_extchan(struct ieee80211com *ic);

extern int ieee80211_get_chan_width(struct ieee80211_ath_channel *chan);
enum phy_ch_width
ieee80211_get_phy_chan_width(struct ieee80211_ath_channel *chan);

enum phy_ch_width ieee80211_get_phy_ch_width(enum ieee80211_cwm_width chwidth);
enum ieee80211_cwm_width ieee80211_get_cwm_width(enum phy_ch_width chwidth);

extern int ieee80211_get_chan_centre_freq(struct ieee80211com *ic,
    struct ieee80211_ath_channel *chan);
extern void    ieee80211_get_extchaninfo(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, struct ath_sub_chan_freq_list *chan_info);
int
ieee80211_check_chan_mode_consistency(struct ieee80211com *ic,int mode,struct ieee80211_ath_channel *c);
u_int8_t ieee80211_is_extchan_144(struct ieee80211com *ic,struct ieee80211_ath_channel *chan, u_int8_t extchan);
enum sec20_chan_offset ieee80211_secondary20_channel_offset(struct ieee80211_ath_channel *chan);

enum ieee80211_mode ieee80211_get_mode(const struct ieee80211_ath_channel *ch);

enum ieee80211_phymode ieee80211_get_composite_phymode (enum ieee80211_mode mode,
       enum ieee80211_cwm_width chwidth, uint8_t secchanoffset);

enum ieee80211_mode get_mode_from_phymode(enum ieee80211_phymode phymode);

enum ieee80211_cwm_width
ieee80211_get_cwm_width_from_channel(struct ieee80211_ath_channel *chan);

int ieee80211_check_overlap(int f1,
                     int w1,
                     int f2,
                     int w2);

void ieee80211_dfs_channel_change_by_precac(struct ieee80211com *ic);

void ieee80211_update_dfs_next_channel(struct ieee80211com *ic);

void ieee80211_vap_iter_update_des_chan(void *arg, struct ieee80211vap *vap);

#if defined(QCA_DFS_BW_PUNCTURE)
/*
 * ieee80211_unpunc_chan_switch() - Send VDEV restart after DFS Unpuncturing.
 * @ic:               Pointer to ieee80211com structure.
 * @new_punc_pattern: New puncture pattern after DFS Unpuncturing.
 */

void ieee80211_unpunc_chan_switch(struct ieee80211com *ic,
                                  uint16_t new_punc_pattern);
#else
static inline
void ieee80211_unpunc_chan_switch(struct ieee80211com *ic,
                                  uint16_t new_punc_pattern)
{
}
#endif

void ieee80211_postnol_chan_switch(struct ieee80211com *ic,
                                   uint16_t des_freq,
                                   uint16_t des_cfreq2,
                                   enum ieee80211_phymode des_mode);

void ieee80211_dfs_nol_timeout_notification(struct ieee80211com *ic);

int ieee80211_dfs_get_cac_timeout(struct ieee80211com *ic,
                                  struct ieee80211_ath_channel *chan);

int ieee80211_reg_get_current_chan_list(struct ieee80211com *ic,
        struct regulatory_channel *curr_chan_list);

int ieee80211_reg_create_ieee_chan_list(struct ieee80211com *ic);

int ieee80211_reg_program_cc(struct ieee80211com *ic,
        char *isoName, u_int16_t cc);

/**
 * ieee80211_reg_program_opclass_tbl() - Set opclass table value.
 * @ic: Pointer to struct ieee80211com.
 * @opclass: Opclass table value.
 *
 * Return: 0 is success, else -EINVAL.
 */
int ieee80211_reg_program_opclass_tbl(struct ieee80211com *ic, uint8_t opclass);

/**
 * ieee80211_reg_get_opclass_tbl() - Get opclass table value.
 * @ic: Pointer to struct ieee80211com.
 * @opclass: Opclass table value.
 *
 * Return: 0 is success, else -EINVAL.
 */
int ieee80211_reg_get_opclass_tbl(struct ieee80211com *ic, uint8_t *opclass);

void ieee80211_get_current_regdomain(struct ieee80211com *ic,
        struct cur_regdmn_info *cur_regdmn);

enum ieee80211_phymode
ieee80211_get_phymode_from_chan_flag(
        struct ieee80211_ath_channel *curchan,
        uint64_t chan_flag);

bool ieee80211_is_phymode_supported(struct ieee80211com *ic,
                                    enum ieee80211_phymode mode);

INLINE static u_int64_t
ieee80211_chan_flags(struct ieee80211_ath_channel *chan)
{
    return chan->ic_flags;
}

INLINE static u_int8_t
ieee80211_chan_flagext(struct ieee80211_ath_channel *chan)
{
    return chan->ic_flagext;
}

extern const char *ieee80211_phymode_name[];

/*
 * Convert channel to IEEE channel number.
 */
static INLINE u_int8_t
ieee80211_chan2ieee(struct ieee80211com *ic, const struct ieee80211_ath_channel *c)
{
    if (!IEEE80211_IS_CHAN_VALID(c)) {
        return 0;       /* XXX */
    }
    return (c == IEEE80211_CHAN_ANYC ?
            (u_int8_t)IEEE80211_CHAN_ANY : c->ic_ieee);
}

const char *ieee80211_phymode_to_name( enum ieee80211_phymode mode);

#define ieee80211_get_current_channel_obj(_ic) ((_ic)->ic_curchan_obj)
#define ieee80211_get_current_channel(_ic)  ((_ic)->ic_curchan)
#define ieee80211_get_home_channel(_vap)     ((_vap)->iv_bsschan)

/*
 * Get current operating PHY mode
 */
static INLINE enum ieee80211_phymode
ieee80211_get_current_phymode(struct ieee80211com *ic)
{
    return ieee80211_chan2mode(ic->ic_curchan);
}

/*
 * Set number of channels
 */
static INLINE void
ieee80211_set_nchannels(struct ieee80211com *ic, int nchans)
{
    ic->ic_nchans = nchans;
}

/* ieee80211_is_chwidth_valid: Validate the input channel width
 *              paramter.
 *
 * @in_chwidth: input channel width
 * Returns    : true if the input channel width (chwidth)
 *              is valid else false.
 */

static INLINE bool
ieee80211_is_chwidth_valid(enum phy_ch_width chwidth)
{
    return (!(chwidth < CH_WIDTH_20MHZ ||  chwidth >= CH_WIDTH_INVALID));

}

int wlan_get_target_phymode(struct ieee80211com *ic, uint32_t phymode,
                            bool is_2gvht_en);

/*
 * Set the channel
 */
#define IEEE80211_CHAN_SETUP(_c, _ieee, _freq, _flags, _extflags,  \
                            _maxregp, _maxp, _minp, _id)\
    do {                                                \
        (_c)->ic_freq = (_freq);                        \
        (_c)->ic_ieee = (_ieee);                        \
        (_c)->ic_flags = (_flags);                      \
        (_c)->ic_flagext = (_extflags);                 \
        (_c)->ic_maxregpower = (_maxregp);              \
        (_c)->ic_maxpower = (_maxp);                    \
        (_c)->ic_minpower = (_minp);                    \
        (_c)->ic_regClassId = (_id);                    \
    } while (0)

/*
 * Compare two channels
 */
#define IEEE80211_CHAN_MATCH(_c, _freq, _flags, _mask)      \
    (((_c)->ic_freq == (_freq)) &&                          \
     (((_c)->ic_flags & (_mask)) == ((_flags) & (_mask))))

#define IEEE80211_IS_PHYMODE_SUPPORTED(_ic, _mode)  \
    (((_ic)->ic_modecaps & (1ULL << (_mode))) && \
    (ieee80211_is_phymode_supported((_ic), (_mode))))

/*Check if the bandwidth
 * is supported by the current regulatory domain
 */

#define IEEE80211_IS_REGDOMAIN_SUPPORTED_320(_ic) \
     (ieee80211_is_chwidth_supported_by_reg(_ic, CH_WIDTH_320MHZ))

#define IEEE80211_IS_REGDOMAIN_SUPPORTED_160(_ic) \
     (ieee80211_is_chwidth_supported_by_reg(_ic, CH_WIDTH_160MHZ))

#define IEEE80211_IS_REGDOMAIN_SUPPORTED_80_80(_ic) \
     (ieee80211_is_chwidth_supported_by_reg(_ic, CH_WIDTH_80P80MHZ))

#define IEEE80211_IS_REGDOMAIN_SUPPORTED_80(_ic) \
     (ieee80211_is_chwidth_supported_by_reg(_ic, CH_WIDTH_80MHZ))

#define IEEE80211_IS_REGDOMAIN_SUPPORTED_40(_ic) \
     (ieee80211_is_chwidth_supported_by_reg(_ic, CH_WIDTH_40MHZ))

#define IEEE80211_IS_REGDOMAIN_SUPPORTED_20(_ic) \
     (ieee80211_is_chwidth_supported_by_reg(_ic, CH_WIDTH_20MHZ))

/* This section contains conditinal checks to check if the
 * current channel mode is supported by the phymode
 * derived from ic_modecaps and
 * is supported by the current regulatory domain
 */


/*Check if the 320 MHz is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */
#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_320(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_320(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_320(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP320(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_EHT320(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_320(_ic)))

/*Check if the 160 MHz is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */
#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_160(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_160(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_160(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP160(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_EHT160(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_160(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAP160(_ic) \
    (((IEEE80211_IS_PHYMODE_SUPPORTED_HE160(_ic)) || \
    (IEEE80211_IS_PHYMODE_SUPPORTED_EHT160(_ic))) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_160(_ic)))

/*Check if the 80+80 MHz is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_80_80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_80_80(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_80_80(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAP80_80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_HE80_80(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_80_80(_ic)))

/*Check if the 40 MHz is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_40(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_40(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_11AXG_HECAP40(_ic) \
    (((IEEE80211_IS_PHYMODE_SUPPORTED_11AXG_HE40(_ic)) || \
    (IEEE80211_IS_PHYMODE_SUPPORTED_11BEG_EHT40(_ic))) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_40(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_11AXA_HECAP40(_ic) \
    (((IEEE80211_IS_PHYMODE_SUPPORTED_11AXA_HE40(_ic)) || \
    (IEEE80211_IS_PHYMODE_SUPPORTED_11BEA_EHT40(_ic))) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_40(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_EHT40(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_40(_ic)))

/*Check if the 80 MHz is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_80(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_80(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAP80(_ic) \
    (((IEEE80211_IS_PHYMODE_SUPPORTED_HE80(_ic)) || \
    (IEEE80211_IS_PHYMODE_SUPPORTED_EHT80(_ic))) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_80(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_EHT80(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_80(_ic)))

/*Check if the 20 MHz is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAP20(_ic) \
    (((IEEE80211_IS_PHYMODE_SUPPORTED_HE20(_ic)) || \
    (IEEE80211_IS_PHYMODE_SUPPORTED_EHT20(_ic))) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_20(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP20(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_EHT20(_ic)) && \
    (IEEE80211_IS_REGDOMAIN_SUPPORTED_20(_ic)))

/*Check if the given channel mode - 320, 160, 80_80 is supported by the phymode
 * derived from ic_modecaps and
 * supported by the current regulatory domain
 */

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_160_or_320(_ic) \
    ((IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_320(_ic)) || \
    (IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_160(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_GT80(_ic) \
    ((IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_160(_ic)) \
    || (IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_320(_ic)) \
    || (IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_80_80(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAPGT80(_ic) \
    ((IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAP160(_ic)) \
    || (IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_320(_ic)) \
    || (IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_HECAP80_80(_ic)))

#define IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAPGT80(_ic) \
    ((IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP160(_ic)) \
    || (IEEE80211_IS_PHYMODE_AND_REGDOMAIN_SUPPORTED_EHTCAP320(_ic)))

/*
 * Check whether a phymode is supported for current channel
 */

#define IEEE80211_SUPPORT_PHY_MODE(_ic, _mode)  \
    (((_ic)->ic_modecaps & (1ULL << (_mode))) && \
    (((_ic)->ic_support_phy_mode == NULL) || \
    ((_ic)->ic_support_phy_mode((_ic), (_mode)))))

#define IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED(_cur_chan) \
        ((IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_HT(_cur_chan)) || \
        (IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_VHT(_cur_chan)) || \
        (IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_HE(_cur_chan)) || \
        (IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_EHT(_cur_chan)))

#define IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_HT(_cur_chan) \
        (IEEE80211_IS_CHAN_11N_HT40(_cur_chan))

#define IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_VHT(_cur_chan) \
        ((IEEE80211_IS_CHAN_11AC_VHT40(_cur_chan)) || \
        (IEEE80211_IS_CHAN_11AC_VHT80(_cur_chan)) || \
        (IEEE80211_IS_CHAN_11AC_VHT160(_cur_chan)) || \
        (IEEE80211_IS_CHAN_11AC_VHT80_80(_cur_chan)))

#define IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_HE(_cur_chan) \
        ((IEEE80211_IS_CHAN_11AX_HE40(_cur_chan)) || \
        (IEEE80211_IS_CHAN_11AXA_HE80(_cur_chan)) || \
        (IEEE80211_IS_CHAN_11AXA_HE160(_cur_chan)) || \
        (IEEE80211_IS_CHAN_11AXA_HE80_80(_cur_chan)))

#define IEEE80211_IS_CUR_PHYMODE_GT80(_cur_mode) \
        (IEEE80211_IS_CUR_PHYMODE_HE_GT80(_cur_mode) || \
        (IEEE80211_IS_CUR_PHYMODE_EHT_GT80(_cur_mode)))

#define IEEE80211_IS_CUR_PHYMODE_HE_GT80(_cur_mode) \
        (ieee80211_is_phymode_11axa_160or8080(_cur_mode))

#define IEEE80211_IS_HECAP_PHY_CHWIDTH_11AXA_HE160(_ni) \
        ((_ni)->ni_he_width_set_org & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE160)

#define IEEE80211_IS_HECAP_PHY_CHWIDTH_11AXA_HE80_80(_ni) \
        ((_ni)->ni_he_width_set_org & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE80_80)

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_EHT(_cur_chan) \
        ((IEEE80211_IS_CHAN_11BE_EHT40(&cur_chan)) || \
        (IEEE80211_IS_CHAN_11BEA_EHT80(&cur_chan)) || \
        (IEEE80211_IS_CHAN_11BEA_EHT160(&cur_chan)) || \
        (IEEE80211_IS_CHAN_11BEA_EHT320(&cur_chan)))

#define IEEE80211_IS_CUR_PHYMODE_EHT_GT80(_cur_mode) \
        (ieee80211_is_phymode_11be(_cur_mode) && \
          (ieee80211_is_phymode_160(_cur_mode) || \
          ieee80211_is_phymode_320(_cur_mode)))

#else

#define IEEE80211_IS_CUR_CHAN_40_MODE_SUPPORTED_EHT(_cur_chan) 0
#define IEEE80211_IS_CUR_PHYMODE_EHT_GT80(_cur_mode) 0
#endif

/*
 * Check whether a phymode is supported irrespective of current channel bandwidth
 */


/* Check if the 320 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT320(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED_320(_ic))

/* Check if the 160 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_160(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_VHT160(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_HE160(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_EHT160(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_VHT160(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AC_VHT160))

#define IEEE80211_IS_PHYMODE_SUPPORTED_HE160(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE160))

/* Check if the 80+80 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_80_80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_VHT80_80(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_HE80_80(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_HE80_80(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE80_80))

#define IEEE80211_IS_PHYMODE_SUPPORTED_VHT80_80(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AC_VHT80_80))

/* Check if the 40 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_HT40(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_VHT40(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_HE40(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_EHT40(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_HT40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11NA_HT40)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11NG_HT40)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_VHT40(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AC_VHT40))

#define IEEE80211_IS_PHYMODE_SUPPORTED_HE40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_11AXG_HE40(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_11AXA_HE40(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_11AXG_HE40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXG_HE40PLUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXG_HE40MINUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXG_HE40)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_11AXA_HE40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE40PLUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE40MINUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE40)))

/* Check if the 80 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_VHT80(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_HE80(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_EHT80(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_VHT80(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AC_VHT80))

#define IEEE80211_IS_PHYMODE_SUPPORTED_HE80(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE80))

/* Check if the 20 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_HE20(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXG_HE20)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11AXA_HE20)))

/* Check if 320, 160, 80_80 MHz is supported by the phymode
 * derived from ic_modecaps
 */

#define IEEE80211_IS_PHYMODE_SUPPORTED_160_or_320(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_160(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_320(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_160_or_80_80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_160(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_80_80(_ic)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_GT80(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED_160(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_320(_ic)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED_80_80(_ic)))


/* Check if 320, 160, 80, 40, 20 MHz is supported by the phymode
 * derived from ic_modecaps for 11BE EHT.
 */


#ifdef WLAN_FEATURE_11BE

#define IEEE80211_IS_PHYMODE_SUPPORTED_320(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT320))

#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT160(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT160))

#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT80(_ic) \
    (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT80))

#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT40)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEG_EHT40)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_11BEG_EHT40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEG_EHT40PLUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEG_EHT40MINUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEG_EHT40)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_11BEA_EHT40(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT40PLUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT40MINUS)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT40)))

#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT20(_ic) \
    ((IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEA_EHT20)) \
    || (IEEE80211_IS_PHYMODE_SUPPORTED(_ic, IEEE80211_MODE_11BEG_EHT20)))

#else

#define IEEE80211_IS_PHYMODE_SUPPORTED_320(_ic)         (0)
#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT160(_ic)      (0)
#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT80(_ic)       (0)
#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT40(_ic)       (0)
#define IEEE80211_IS_PHYMODE_SUPPORTED_11BEG_EHT40(_ic) (0)
#define IEEE80211_IS_PHYMODE_SUPPORTED_11BEA_EHT40(_ic) (0)
#define IEEE80211_IS_PHYMODE_SUPPORTED_EHT20(_ic)       (0)

#endif

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_SUPPORT_11BEA_PHY_MODE(_ic)  \
    (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT20) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT40) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT40PLUS) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT40MINUS) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT80) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT160) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEA_EHT320))

#define IEEE80211_SUPPORT_11BEG_PHY_MODE(_ic)  \
    (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEG_EHT20) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEG_EHT40) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEG_EHT40PLUS) || \
     IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11BEG_EHT40MINUS))
#endif /* WLAN_FEATURE_11BE */

/*
 * Check whether a phymode is accepted
 */
#define IEEE80211_ACCEPT_PHY_MODE(_vap, _mode)  \
    ((_vap)->iv_des_modecaps & (1 << (_mode)))

#define IEEE80211_ACCEPT_ANY_PHY_MODE(_vap)     \
    ((_vap)->iv_des_modecaps & (1 << IEEE80211_MODE_AUTO))

#define IEEE80211_ACCEPT_PHY_MODE_11G(_vap)     \
    ((_vap)->iv_des_modecaps & (1 << IEEE80211_MODE_11G))

#define IEEE80211_ACCEPT_PHY_MODE_11A(_vap)     \
    ((_vap)->iv_des_modecaps & (1 << IEEE80211_MODE_11A))

/*
 * ieee80211_restart_vaps_with_non_dfs_channels() - Restart vaps with a non DFS
 * channel.
 * @ic - pointer to ieee80211com structure.
 * @no_chans_avail - 1 indicates no non-dfs channel is available.
 */
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
void ieee80211_restart_vaps_with_non_dfs_channels(struct ieee80211com *ic,
        int no_chans_avail);
#endif /* HOST_DFS_SPOOF_TEST */

enum phy_ch_width
ieee80211_get_chan_width_from_phymode(enum ieee80211_phymode mode);

int ieee80211_get_chan_width_from_mode(enum ieee80211_phymode mode);

/*
 * ieee80211_bringdown_vaps():
 * If there are no valid channels for vap to come up
 * (Reasons could be:
 * 1.Regdmn had only DFS channels and all DFS channels are in NOL.
 * 2.DFS Violation and hence it is mandated to have only non-DFS channel in
 * the radio and regdmn does not support non-DFS channels.)
 * bringdown all the vaps.
 * @ic - pointer to ieee80211com structure.
 */

void ieee80211_bringdown_vaps(struct ieee80211com *ic);

/*
 * ieee80211_bringdown_sta_vap();
 * If the sta vap has to be brought down.
 * @ic - pointer to ieee80211com structure.
 */

void ieee80211_bringdown_sta_vap(struct ieee80211com *ic);

void ieee80211_update_vdev_chan(struct wlan_channel *vdev_chan,
        struct ieee80211_ath_channel *curchan);

/*
 * ieee80211_dfs_deliver_event: Deliver DFS radar hit event
 * @ic: ic object
 * @freq: Frequency radar hit on
 * @event: Event being sent
 */
void ieee80211_dfs_deliver_event(struct ieee80211com *ic, u_int16_t freq, enum WLAN_DFS_EVENTS event);

/* ieee80211_dfs_allocate_nol: Allocate a persistent memory for DFS NOL lists.
 * @ic: pointer to ieee80211com structure.
 * @dfs_mm_nolinfo: Double pointer to structure dfsreq_nolinfo.
 */
void ieee80211_dfs_allocate_nol(struct ieee80211com *ic, struct dfsreq_nolinfo **dfs_mm_nolinfo);

/*
 * ieee80211_dfs_deliver_events: Deliver DFS radar hit event to channel list pointed by @chan
 * @ic: ic object
 * @chan: Channel list to send events to
 * @event: Event being sent
 */

void ieee80211_dfs_deliver_events(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, enum WLAN_DFS_EVENTS event);

enum wlan_band_id ieee80211_get_ath_channel_band(qdf_freq_t freq);

enum reg_wifi_band wlan_band_id_to_reg_wifi_band(enum wlan_band_id wlan_band);

enum wlan_band_id reg_wifi_band_to_wlan_band_id(enum reg_wifi_band reg_wifi_band);

uint16_t wlan_get_wlan_band_id_chan_to_freq(struct wlan_objmgr_pdev *pdev,
        uint32_t chan, enum wlan_band_id wlan_band);

/*
 * wlan_band_id_chan_to_freq_from_cur_chan_list : Get IEEE Channel number and
 * band as the input and give channel center frequency as the output by searching
 * the regulatory current channel list.
 * @ic: Pointer to ic.
 * @chan: IEEE channel number
 * @wlan_band: band
 */
qdf_freq_t wlan_band_id_chan_to_freq_from_cur_chan_list(struct ieee80211com *ic,
                                                        uint32_t chan,
                                                        enum wlan_band_id wlan_band);
uint64_t ieee80211_get_band_flag(uint16_t freq);

#define IEEE80211_ARE_CHANS_INTERWIDEBAND(__chan1, __chan2)   \
                      ((IEEE80211_IS_CHAN_5GHZ(__chan1) &&    \
                        IEEE80211_IS_CHAN_6GHZ(__chan2)) ||   \
                       (IEEE80211_IS_CHAN_6GHZ(__chan1) &&    \
                        IEEE80211_IS_CHAN_5GHZ(__chan2)))

#define IEEE80211_ARE_CHANS_INTRABAND(__chan1, __chan2, band) \
                      (IEEE80211_IS_CHAN_##band(__chan1) &&   \
                       IEEE80211_IS_CHAN_##band(__chan2))

/*
 * wlan_dcs_wideband_policy_t:
 * Specfies the policy codes for the DCS wideband policy.
 * The policy needs to be used in the OL layer for DCS and UMAC layer for the
 * channel selection algorithm.
 */
typedef enum {
	DCS_WIDEBAND_POLICY_INTRABAND = 0,
	DCS_WIDEBAND_POLICY_INTERBAND = 1,
	DCS_WIDEBAND_POLICY_INVALID,
} wlan_dcs_wideband_policy_t;

/**
 * ieee80211_is_phymode_supported_by_channel() - Check the regulatory and device
 * support for given mode and channel combination.
 * @ic: Pointer to ieee80211com.
 * @freq: Frequency to check.
 * @mode: Phymode to check.
 * @in_6g_pwr_mode: 6G power mode that determines the 6G channel list to search
 * freq for.
 * @puncture_bitmap: Puncture bitmap
 *
 * Return true if channel supports given phymode, else false.
 */
bool ieee80211_is_phymode_supported_by_channel(struct ieee80211com *ic,
                                               qdf_freq_t freq,
                                               enum ieee80211_phymode mode,
                                               enum supported_6g_pwr_types in_6g_pwr_mode,
                                               uint16_t puncture_bitmap);

/**
 * ieee80211_get_subchannels() - Find the subchannels given a primary frequency
 * and mode from regulatory.
 * @ic: Pointer to ieee80211com.
 * @freq: Primary frequency of the channel.
 * @mode: Phymode of the channel.
 * @subchannels: Array of 20MHZ frequencies in the channel.
 *
 * Return the count of subchannels, 0 if none found.
 */
int ieee80211_get_subchannels(struct ieee80211com *ic,
                              qdf_freq_t freq,
                              enum ieee80211_phymode mode,
                              qdf_freq_t *subchannels);

/**
 * ieee80211_get_subchannels_with_width_centerfreq() - Find the subchannels given a
 * center frequency and bandwidth.
 * @center_freq: Center frequency.
 * @ch_width: channel bandwidth in form of enum.
 * @subchannels: Array of 20MHZ frequencies in the channel.
 *
 * Return the count of subchannels, 0 if none found.
 */
int ieee80211_get_subchannels_with_width_centerfreq(uint16_t center_freq,
                                                    enum phy_ch_width ch_width,
                                                    qdf_freq_t *subchannels);

int ieee80211_is_freq_in_obj_valid(struct ieee80211com *ic,
                                   struct chan_params *chan_p,
                                   uint64_t flags);

/**
 * ieee80211_get_sec_chan_offset_for_chanbw40() - Get the sec channel offset
 * for channel bw 40.
 * @ic: Pointer to ieee80211com.
 * @chan_params: Pointer to channel params.
 *
 * Return the secondary chan offset for channel bw 40.
 */
uint8_t ieee80211_get_sec_chan_offset_for_chanbw40(struct ieee80211com *ic,
                                                   struct chan_params *chan_p);

/**
 * ieee80211_get_subchannels() - Find if frequency valid for channel bw 20.
 * @ic: Pointer to ieee80211com.
 * @chan_params: Pointer to channel params.
 * @ch_flags: channel flags.
 *
 * Return true if channel valid.
 */
bool ieee80211_is_freq_valid_for_chanbw20(struct ieee80211com *ic,
                                          struct chan_params *chan_p,
                                          uint64_t ch_flags);

struct ieee80211_ath_channel
ieee80211_ath_get_first_channel(struct ieee80211com *ic);

void ieee80211_update_channel_avail_bits(struct ieee80211com *ic);

uint64_t ieee80211_update_modecaps(struct ieee80211com *ic,
                                   bool include_nol_chan);

bool ieee80211_is_any_80p80_supported(struct ieee80211com *ic,
                                      bool include_nol_chan);

void
ieee80211_regdmn_remove_phybitmap_from_modeselect(uint64_t *mode_select,
                                                  uint16_t phybitmap);
#ifdef WLAN_FEATURE_11BE
/**
 * ieee80211_intersect_puncture_bitmap() - Intersect puncture pattern for
 * lower bandwidth from higher bandwidth based on channel primary frequency.
 * @bss_chan: Pointer to ieee80211_ath_channel corresponding to higher BW.
 * @des_mode: Desired ieee80211_phymode for lower BW.
 * @bss_mode: Desired mode for bss_chan BW.
 *
 * Return: Intersected puncture pattern (may not be a IEEE defined pattern
 * for target BW)
 */
uint16_t ieee80211_intersect_puncture_bitmap(struct ieee80211_ath_channel *bss_chan,
                enum ieee80211_phymode des_mode, enum ieee80211_phymode bss_mode);

static inline void
set_chan_params_puncture_bitmap(struct chan_params *chan_p,
                                uint16_t puncture_bitmap)
{
    if (puncture_bitmap == PUNCTURE_INVALID) {
        chan_p->puncture_bitmap = PUNCTURE_NONE;
    } else {
        chan_p->puncture_bitmap = puncture_bitmap;
    }
    chan_p->op_puncture_bitmap = PUNCTURE_INVALID;
}
#else
#define \
set_chan_params_puncture_bitmap(_a, _b)

static inline
uint16_t ieee80211_intersect_puncture_bitmap(struct ieee80211_ath_channel *bss_chan,
                                             enum ieee80211_phymode des_mode,
                                             enum ieee80211_phymode bss_mode)
{
    return 0;
}

#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE
static inline uint16_t
ieee80211_get_chan_punc_bitmap(struct ieee80211_ath_channel *chan)
{
    return chan->op_puncture_bitmap;
}
#else
static inline uint16_t
ieee80211_get_chan_punc_bitmap(struct ieee80211_ath_channel *chan)
{
    return 0;
}
#endif

#ifdef WLAN_FEATURE_11BE
static inline uint16_t
ieee80211_get_chan_param_punc_bitmap(struct chan_params *chan_p)
{
    return chan_p->puncture_bitmap;
}
#else
static inline uint16_t
ieee80211_get_chan_param_punc_bitmap(struct chan_params *chan)
{
    return 0;
}
#endif

#ifdef WLAN_FEATURE_11BE
static inline uint16_t
ieee80211_get_chan_param_op_punc_bitmap(struct chan_params *chan_p)
{
    return chan_p->op_puncture_bitmap;
}

/*
 * ieee80211_get_pp_from_find_chan:
 * Derive a valid puncture bitmap for the input channel params and mode flags.
 *
 * @ic     : Pointer to ieee80211com.
 * @chan_p : Pointer to channel params.
 * @flags  : Channel flags.
 *
 * Return:
 * Puncture pattern derived from find channel API.
 */
uint16_t ieee80211_get_pp_from_find_chan(struct ieee80211com *ic,
                                         struct chan_params *chan_p,
                                         u_int64_t flags);

#else
static inline uint16_t
ieee80211_get_chan_param_op_punc_bitmap(struct chan_params *chan)
{
    return 0;
}

static inline uint16_t
ieee80211_get_pp_from_find_chan(struct ieee80211com *ic,
                                struct chan_params *chan_p,
                                u_int64_t flags)
{
    return 0;
}
#endif
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
                                     struct acs_random_chan_config *config);

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
                           uint32_t buffer);

#define IEEE80211_FREQ_CFREQ_OFFSET 10
#define IEEE80211_IS_FREQ_IN_WIDTH(freq, cfreq, width, buffer)       \
    ((freq >= (cfreq - ((width/2)-10) - buffer)) && \
     (freq <= (cfreq + ((width/2)-10) + buffer)))

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
ieee80211_find_next_lower_phymode(enum ieee80211_phymode mode);

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
                                   enum ieee80211_punc_type *puncture_type);
#endif /* WLAN_FEATURE_11BE */

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
                                      uint16_t *min_eirp_power);

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
                                          enum ieee80211_phymode mode);

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
                               struct ieee80211_ath_channel *channel);
#endif

/*
 * ieee80211_find_non_punctured_channel:
 * Given a puncture pattern, find the a largest non-punctured segment including
 * the primary channel. If the 11BE feature flag is not enabled, return the
 * original channel unconditionally.
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
                                      struct ieee80211_ath_channel *channel);

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
        struct ieee80211_ath_channel *channel);

#ifdef CONFIG_BAND_6GHZ
/*
 * ieee80211_is_chan_priority:
 * Check if the value of channel frequency is greater than the frequency
 * received from FW Event stored in 6G priority frequency.
 *
 * @ic         : Pointer to the ic structure
 * @channel    : Pointer to EHT punctured channel
 *
 * Return:
 * True: If channel frequency is higher than 6G Priority frequency.
 * False: If channel frequency is lower than 6G Priority frequency.
 */
bool ieee80211_is_chan_priority(struct ieee80211com *ic,
                                struct ieee80211_ath_channel *channel);
#else
static inline
bool ieee80211_is_chan_priority(struct ieee80211com *ic,
                                struct ieee80211_ath_channel *channel)
{
    return true;
}
#endif /* CONFIG_BAND_6GHZ */

/*
 * ieee80211_convert_cwm_wd_to_phych_width() - Convert cwm_width to
 * phy_ch_width.
 * @cwm_width: Channel width of type ieee80211_cwm_width
 *
 * Return - Channel width of type phy_ch_width
 */
enum phy_ch_width
ieee80211_convert_cwm_wd_to_phych_width(enum ieee80211_cwm_width cwm_width);

#ifdef WLAN_FEATURE_11BE
bool validate_primary_channel_in_pp(uint16_t pp, struct ieee80211_ath_channel *chan);
#else
static inline bool
validate_primary_channel_in_pp(uint16_t pp, struct ieee80211_ath_channel *chan)
{
    return 0;
}
#endif

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
                                             struct acs_random_chan_config *config);
#endif

#if defined(WLAN_FEATURE_11BE) && defined(CONFIG_AFC_SUPPORT) && defined(CONFIG_BAND_6GHZ)
/*
 * reg_get_6g_min_psd:
 * Get the PSD limit for the given 6GHz center frequency, bandwidth
 * and puncture bitmap.
 *
 * @pdev   : Input - Pointer to the pdev object
 * @freq   : Input - Primary frequency
 * @cfreq2 : Input - Center frequency of the given bandwidth
 * @puncture_bitmap : Input - Puncture bitmap for the given bandwidth
 * @bw     : Input - Bandwidth of the channel
 * @min_psd: Output - PSD limit for the given bandwidth when AFC is complete
 *                    otherwise invalid PSD is returned
 *
 * Return:
 * QDF status of the API.
 */
QDF_STATUS reg_get_6g_min_psd(struct wlan_objmgr_pdev *pdev,
        qdf_freq_t freq, qdf_freq_t cfreq2,
        uint16_t puncture_bitmap, uint16_t bw, int16_t *min_psd);
#else
static inline QDF_STATUS
reg_get_6g_min_psd(struct wlan_objmgr_pdev *pdev,
                   qdf_freq_t freq, qdf_freq_t cfreq2,
                   uint16_t puncture_bitmap, uint16_t bw, int16_t *min_psd)
{
    return QDF_STATUS_E_INVAL;
}
#endif /* WLAN_FEATURE_11BE && CONFIG_AFC_SUPPORT && CONFIG_BAND_6GHZ */

/*
 * ieee80211_is_user_punc_same_as_radar_bitmap() - Return true if the usr
 * punc bitmap is same as chan's puncture bitmap if the chan is radar infected.
 *
 * @ic: Pointer to struct ieee80211com
 * @chan: Pointer to struct ieee80211_ath_channel
 * @usr_punc_bitmap: User puncture bitmap
 *
 */
#ifdef WLAN_FEATURE_11BE

uint16_t ieee80211_get_radar_punc_bitmap(struct ieee80211com *ic,
                                            struct ieee80211_ath_channel *chan);
bool
ieee80211_is_user_punc_same_as_radar_bitmap(struct ieee80211com *ic,
                                            struct ieee80211_ath_channel *chan,
                                            uint16_t usr_punc_bitmap);
#endif

/**
 * ieee80211_sec_chan_offset(): Find the secondary channel offset IE of the
 * given channel. If the input channel is punctured, find the non_11bea
 * channel and compute the secondary channel offset IE from the non-11bea
 * channel.
 * @ic: Pointer to struct ieee80211com
 * @chan: Pointer to struct ieee80211_ath_channel
 *
 * Return: Secondary channel offset IE
 */
uint8_t ieee80211_sec_chan_offset(struct ieee80211com *ic,
                                  struct ieee80211_ath_channel *chan);

/**
 * ieee80211_get_reg_phymode_ch_width(): Get Reg Phymode and channel width
 * from phymode.
 * @phymode: Phymode.
 * @r_phymode: Pointer to reg phymode
 * @ch_width: Pointer to channel width
 *
 * Return: QDF_STATUS
 */
QDF_STATUS ieee80211_get_reg_phymode_ch_width(enum ieee80211_phymode phymode,
                                              enum reg_phymode *r_phymode,
                                              enum phy_ch_width *ch_width);

QDF_STATUS ieee80211_get_chan_freq_range(struct ieee80211_ath_channel *channel,
                                         qdf_freq_t *chan_low,
                                         qdf_freq_t *chan_mid_low,
                                         qdf_freq_t *chan_mid_high,
                                         qdf_freq_t *chan_high);

/**
 * ieee80211_get_11bea_mode_from_bw(): Given a channel bandwidth convert it to
 * the 11be phymode.
 * @chan_bw: Channel bandwidth in MHz.
 *
 * Return: phymode equivalent of the channel width.
 */
enum ieee80211_phymode
ieee80211_get_11bea_mode_from_bw(uint16_t chan_bw);

/**
 * ieee80211_find_first_active_vap() - Find the first active vap in the ic.
 * @ic: Pointer to ieee80211com.
 *
 * Return: Pointer to first VAP structure, NULL if none found.
 */
wlan_if_t ieee80211_find_first_active_vap(struct ieee80211com *ic);

/**
 * ieee80211_fill_regulatory_info() - Fill regulatory info field of HEOP.
 * @ic: Pointer to struct ieee80211com
 *
 * Return: 6GHz AP power type
 */
enum reg_6g_ap_type
ieee80211_fill_regulatory_info(struct ieee80211com *ic);
#endif /* _NET80211_IEEE80211_CHAN_H */
