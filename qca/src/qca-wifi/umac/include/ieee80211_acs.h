/*
 * Copyright (c) 2011, 2017,2019,2021 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 *  Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef _IEEE80211_ACS_H
#define _IEEE80211_ACS_H

typedef struct ieee80211_acs    *ieee80211_acs_t;

typedef struct _ieee80211_chan_neighbor_list {
    ieee80211_acs_t  acs;        /* acs reference */
    uint16_t         freq;       /* ieee channel frequency */
    u_int8_t         nbss;       /*number of bss per channel */
    ieee80211_neighbor_info   *neighbor_list; /* List of Neighbor per channel */
    int32_t         neighbor_size;
    int32_t         status;
} ieee80211_chan_neighbor_list;

struct ieee80211_acs_chan_neighbor_list {
    bool fill_rssi;
    ieee80211_chan_neighbor_list *acs_neighbor_list;
};

enum ieee80211_acs_config {
    IEEE80211_ACS_CONFIG_NONE,
    IEEE80211_ACS_CTRLFLAG,
    IEEE80211_ACS_ENABLE_BK_SCANTIMER,
    IEEE80211_ACS_SCANTIME,
    IEEE80211_ACS_SNRVAR,
    IEEE80211_ACS_CHLOADVAR,
    IEEE80211_ACS_LIMITEDOBSS,
    IEEE80211_ACS_DEBUGTRACE,
    IEEE80211_ACS_BLOCK_MODE,
    IEEE80211_ACS_TX_POWER_OPTION,
    IEEE80211_ACS_2G_ALL_CHAN,
    IEEE80211_ACS_SRLOADVAR,
    IEEE80211_ACS_RANK,
    IEEE80211_ACS_NF_THRESH,
    IEEE80211_ACS_CHAN_GRADE_ALGO,
    IEEE80211_ACS_CHAN_EFFICIENCY_VAR,
    IEEE80211_ACS_OBSS_NEAR_RANGE_WEIGHTAGE,
    IEEE80211_ACS_OBSS_MID_RANGE_WEIGHTAGE,
    IEEE80211_ACS_OBSS_FAR_RANGE_WEIGHTAGE,
    IEEE80211_ACS_SEC20_OBSS_WEIGHTAGE,
#ifdef WLAN_FEATURE_11BE
    IEEE80211_ACS_PUNCTURE_LEGACY_WEIGHTAGE,
    IEEE80211_ACS_PUNCTURE_NOFDMA_WEIGHTAGE,
    IEEE80211_ACS_PUNCTURE_OFDMA_WEIGHTAGE,
#endif /* WLAN_FEATURE_11BE */
    IEEE80211_ACS_6G_ONLY_PSC,
    IEEE80211_ACS_6G_NON_PRIO_CHAN_PENALTY,
    IEEE80211_ACS_ADJ_CHAN_INTERFERENCE_BLOCKING,
    IEEE80211_ACS_CONFIG_MAX,
};

#define ACS_CHAN_STATS 0
#define ACS_CHAN_STATS_NF 1
#define ACS_CHAN_STATS_DIFF 3
#define IEEE80211_HAL_OFFLOAD_ARCH 0
#define IEEE80211_HAL_DIRECT_ARCH 1

#if UMAC_SUPPORT_ACS
/*
 * ieee80211_acs_is_scan_in_progress:
 * Check if an ACS scan is in progress
 *
 * @acs: Pointer to the ACS structure
 *
 * Return:
 * true: Scan is in progress
 * false: Scan is not in progress
 */
bool ieee80211_acs_is_scan_in_progress(ieee80211_acs_t acs);

QDF_STATUS wlan_get_acs_inprogress(struct wlan_objmgr_vdev *vdev,
                                   bool *is_acs_inprogress);

int ieee80211_acs_attach(ieee80211_acs_t *acs,
                          struct ieee80211com *ic,
                          osdev_t             osdev);

int ieee80211_acs_init(ieee80211_acs_t *acs,
                          struct ieee80211com *ic,
                          osdev_t osdev);
void ieee80211_acs_deinit(ieee80211_acs_t *acs);
int ieee80211_acs_detach(ieee80211_acs_t *acs);
int ieee80211_acs_startscantime(struct ieee80211com *ic);
int ieee80211_acs_state(struct ieee80211_acs *acs);

void ieee80211_acs_stats_update(ieee80211_acs_t acs,
                                u_int8_t flags,
                                uint16_t ieee_freq,
                                int16_t chan_nf,
                                struct ieee80211_chan_stats *chan_stats);

int ieee80211_acs_set_param(ieee80211_acs_t acs, int param , int val);
int ieee80211_acs_get_param(ieee80211_acs_t acs, int param );
#else
static inline
bool ieee80211_acs_is_scan_in_progress(ieee80211_acs_t acs)
{
    return false;
}

static inline
QDF_STATUS wlan_get_acs_inprogress(struct wlan_objmgr_vdev *vdev,
                                   bool *is_acs_inprogress)
{
    *is_acs_inprogress = false;
    return QDF_STATUS_SUCCESS;
}

static inline
int ieee80211_acs_attach(ieee80211_acs_t *acs,
                          struct ieee80211com *ic,
                          osdev_t             osdev)
{
    return 0;
}

static inline
int ieee80211_acs_init(ieee80211_acs_t *acs,
                          struct ieee80211com *ic,
                          osdev_t osdev)
{
    return 0;
}

static inline
void ieee80211_acs_deinit(ieee80211_acs_t *acs)
{
    return;
}

static inline
int ieee80211_acs_detach(ieee80211_acs_t *acs)
{
    return 0;
}

static inline
int ieee80211_acs_startscantime(struct ieee80211com *ic)
{
    return 0;
}

static inline
int ieee80211_acs_state(struct ieee80211_acs *acs)
{
    return -EINVAL;
}

static inline
void ieee80211_acs_stats_update(ieee80211_acs_t acs,
                                u_int8_t flags,
                                uint16_t ieee_freq,
                                int16_t chan_nf,
                                struct ieee80211_chan_stats *chan_stats)
{
    return;
}

static inline
int ieee80211_acs_set_param(ieee80211_acs_t acs, int param, int val)
{
    return EOK;
}

static inline
int ieee80211_acs_get_param(ieee80211_acs_t acs, int param)
{
    return 0;
}
#endif /* UMAC_SUPPORT_ACS */

#if UMAC_SUPPORT_CFG80211
int wlan_acs_cfg80211_start_scan_report(wlan_if_t vap,
                        cfg80211_hostapd_acs_params *cfg_acs_params);
#endif

#if UMAC_SUPPORT_ACS
/* List of function APIs used by other modules(like CBS) to control ACS */
int ieee80211_acs_api_prepare(struct ieee80211vap *vap, ieee80211_acs_t acs, enum ieee80211_phymode mode,
                              uint32_t *chan_list, uint32_t *nchans);
int ieee80211_acs_api_rank(ieee80211_acs_t acs, uint8_t top);
int ieee80211_acs_api_complete(ieee80211_acs_t acs);
int ieee80211_acs_api_flush(struct ieee80211vap *vap);
int ieee80211_acs_api_update(struct ieee80211com *ic, enum scan_event_type type, uint32_t freq);
uint16_t ieee80211_acs_api_get_ranked_chan(ieee80211_acs_t acs, int rank);
int ieee80211_acs_api_resttime(struct ieee80211_acs *acs);
int ieee80211_acs_api_dwelltime(struct ieee80211_acs *acs);

uint16_t ieee80211_acs_get_chan_idx(ieee80211_acs_t acs, uint16_t freq);
uint16_t ieee80211_acs_get_chan_idx_from_channel(ieee80211_acs_t acs,
        struct ieee80211_ath_channel *channel);
#else
static inline
int ieee80211_acs_api_prepare(struct ieee80211vap *vap, ieee80211_acs_t acs, enum ieee80211_phymode mode,
                              uint32_t *chan_list, uint32_t *nchans)
{
    return -EINVAL;
}

static inline
int ieee80211_acs_api_rank(ieee80211_acs_t acs, uint8_t top)
{
    return 0;
}

static inline
int ieee80211_acs_api_complete(ieee80211_acs_t acs)
{
    return 0;
}

static inline
int ieee80211_acs_api_flush(struct ieee80211vap *vap)
{
    return EOK;
}

static inline
int ieee80211_acs_api_update(struct ieee80211com *ic, enum scan_event_type type, uint32_t freq)
{
    return 0;
}

static inline
uint16_t ieee80211_acs_api_get_ranked_chan(ieee80211_acs_t acs, int rank)
{
    return 0;
}

static inline
int ieee80211_acs_api_resttime(struct ieee80211_acs *acs)
{
    return 0;
}

static inline
int ieee80211_acs_api_dwelltime(struct ieee80211_acs *acs)
{
    return 0;
}

static inline
uint16_t ieee80211_acs_get_chan_idx(ieee80211_acs_t acs, uint16_t freq)
{
    return 0;
}

static inline
uint16_t ieee80211_acs_get_chan_idx_from_channel(ieee80211_acs_t acs,
        struct ieee80211_ath_channel *channel)
{
    return 0;
}
#endif /* UMAC_SUPPORT_ACS */

/* awgn_interference_freq: awgn interference frequencies derived from the interference
 *                        bitmap
 * @chan_low: start of the interference
 * @chan_high: end of the interference
 */
struct awgn_interference_freq {
    qdf_freq_t chan_low;
    qdf_freq_t chan_high;
};

/*
 * acs_random_chan_config:
 * List of configurations that can be used by the caller requesting for
 * random channel selection.
 *
 * @freq_list           : Pointer to a list of frequency values to consider for
 *                        channel selection. If empty, the random channel
 *                        selection algorithm will pull all the supported
 *                        channel lists offered by regulatory.
 * @freq_count          : Number of entries in the list of frequencies pointed
 *                        by freq_list.
 * @allow_wideband      : Consider wideband channels for the 5-7GHz wideband
 *                        radios.
 * @allow_curchan       : Consider the current channel as part of the random
 *                        channel selection algorithm.
 * @allow_max_power_only: Consider only channels with the maximum power.
 *                        Priority will be PSD > EIRP
 * @adj_chan_buffer     : Block channels to the left and right of the current
 *                        operating channel
 * @allow_awgn_interference_overlap_check : awgn interference detected frequency check
 * @intf_freq           : awgn interference frequencies derived from the interference
 *                        bitmap
 */
struct acs_random_chan_config {
    const uint32_t *freq_list;
    uint32_t freq_count;
    bool allow_wideband;
    bool allow_curchan;
    bool allow_max_power_only;
    uint32_t adj_chan_buffer;
    bool allow_awgn_interference_overlap_check;
    struct awgn_interference_freq intf_freq[16];
};

#if UMAC_SUPPORT_ACS
/*
 * ieee80211_acs_find_random_channel:
 * Find the best random channel from a given intersection of supported
 * channel list and input channel list.
 *
 * @vap   : Input - Pointer to the VAP structure
 * @config: Input - Pointer to the random channel selection configuration
 *
 * Return:
 * Randomly selected channel
 */
struct ieee80211_ath_channel
ieee80211_acs_find_random_channel(struct ieee80211vap *vap,
                                  struct acs_random_chan_config *config);

/*
 * ieee80211_acs_channel_weightage:
 * Manage the channel weightage receives from ACS callers
 *
 * @vap   : Pointer to the VAP structure
 * @chlist: Pointer to the channel list
 *
 * Return:
 * QDF_STATUS_SUCCESS: Success
 * QDF_STATUS_E_*    : Failure
 */
QDF_STATUS
ieee80211_acs_channel_weightage(struct ieee80211vap *vap,
                                struct acs_chan_weightage_list *chlist);
#else
static inline struct ieee80211_ath_channel
ieee80211_acs_find_random_channel(struct ieee80211vap *vap,
                                  struct acs_random_chan_config *config)
{
    struct ieee80211_ath_channel channel = empty_channel;

    return channel;
}

static inline QDF_STATUS
ieee80211_acs_channel_weightage(struct ieee80211vap *vap,
                                struct acs_chan_weightage_list *chlist)
{
    return -EINVAL;
}
#endif /* UMAC_SUPPORT_ACS */
#endif
