/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

/**
 * DOC: This file contains protocol definitions specific to IEEE 802.11be
 * Multi-Link Operation (MLO).
 */
#ifndef _IEEE80211_MLO_H_
#define _IEEE80211_MLO_H_

#include <wlan_cmn_ieee80211.h>

#ifdef WLAN_FEATURE_11BE_MLO

/* Minimum number of VDEVs to be considered for MLO related IE processing for
 * reported STAs.
 */
#define WLAN_UMAC_REPORTED_MLO_IE_PROC_MIN_VDEVS                        2


/* Max length of Common Info field in Basic variant Multi-Link IE in Beacon or
 * non-ML Probe Response
 */
#define IEEE80211_BCNNONMLPROBERESP_ML_BV_CINFO_MAX_LEN                        \
    (QDF_MAC_ADDR_SIZE + WLAN_ML_BV_CINFO_LINKIDINFO_SIZE +                    \
     WLAN_ML_BSSPARAMCHNGCNT_SIZE +                                            \
     WLAN_ML_BV_CINFO_MEDMSYNCDELAYINFO_SIZE + WLAN_ML_BV_CINFO_EMLCAP_SIZE +  \
     WLAN_ML_BV_CINFO_MLDCAPANDOP_SIZE)

/* Max length of STA info field in per-STA Profile subelement in Basic variant
 * Multi-Link IE in Beacon or non-ML Probe Response
 */
#define IEEE80211_BCNNONMLPROBERESP_ML_BV_LINFO_PERSTAPROF_STAINFO_MAX_LEN  0

/* Max length of STA profile field in per-STA Profile subelement in Basic
 * variant Multi-Link IE in Beacon or non-ML Probe Response
 */
/* Note:
 *
 * - We assume that 'Enhanced Channel Switch Announcement element' in IEEE
 * 802.11be D1.0 actually refers to 'Extended Channel Switch Announcement
 * element' (as rectified later in D1.2)
 *
 * - In case Quiet Channel IE is implemented within the driver (and not just as
 * a copy from the app IE buffer), then IEEE80211_QUIET_CHANNEL_IE_MAX_LEN can
 * be replaced by the relevant sizeof() based value.
 */
#define IEEE80211_BCNNONMLPROBERESP_ML_BV_LINFO_PERSTAPROF_STAPROF_MAX_LEN    \
    (sizeof(struct ieee80211_ath_channelswitch_ie) +                          \
     sizeof(struct ieee80211_extendedchannelswitch_ie) +                      \
     sizeof(struct ieee80211_max_chan_switch_time_ie) +                       \
     sizeof(struct ieee80211_ath_quiet_ie) +                                  \
     IEEE80211_QUIET_CHANNEL_IE_MAX_LEN)

/* Max length of Basic variant Multi-Link IE in Beacon or non-ML Probe Response
 */
#define IEEE80211_BCNNONMLPROBERESP_ML_BV_MAX_LEN                             \
    (sizeof(struct wlan_ie_multilink) +                                       \
     IEEE80211_BCNNONMLPROBERESP_ML_BV_CINFO_MAX_LEN +                        \
     ((WLAN_UMAC_MLO_MAX_VDEVS - 1) * (                                       \
        sizeof(struct wlan_ml_bv_linfo_perstaprof) +                          \
        IEEE80211_BCNNONMLPROBERESP_ML_BV_LINFO_PERSTAPROF_STAINFO_MAX_LEN +  \
        IEEE80211_BCNNONMLPROBERESP_ML_BV_LINFO_PERSTAPROF_STAPROF_MAX_LEN)))

/* Max length of Reconfiguration variant Multi-Link IE */
#define IEEE80211_ML_RV_MAX_LEN                                               \
    (sizeof(struct wlan_ie_multilink) +                                       \
     WLAN_ML_RV_CINFO_LENGTH_MAX +                                            \
     (WLAN_UMAC_MLO_MAX_VDEVS * (                                             \
        sizeof(struct wlan_ml_rv_linfo_perstaprof) +                          \
        WLAN_ML_RV_LINFO_PERSTAPROF_STAINFO_LENGTH_MAX)))

/*
 * Definitions related to MLO specific aspects of Reduced Neighbor Report
 * element.
 */

#define IEEE80211_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_LEN                        3

/**
 * struct ieee80211_rnr_mld_tbtt_info - TBTT Information field present as part
 * of TBTT Information Set in Neighbor AP Information field of Reduced Neighbor
 * Report element.
 * @tbtt_offset: Neighbor AP TBTT Offset
 * @bssid: BSSID
 * @short_ssid: Short SSID
 * @bss_params: BSS parameters
 * @psd_20mhz: 20 MHz PSD
 * @mld_params: MLD parameters
 */
struct ieee80211_rnr_mld_tbtt_info {
    u_int8_t  tbtt_offset;
    u_int8_t  bssid[IEEE80211_ADDR_LEN];
    u_int32_t short_ssid;
    struct  ieee80211_rnr_bss_params bss_params;
    u_int8_t  psd_20mhz;
    u_int8_t  mld_params[IEEE80211_RNR_NBRAPINFO_TBTTINFO_MLDPARAMS_LEN];
} __packed;

/*
 * End of definitions related to MLO specific aspects of Reduced Neighbor Report
 * element.
 */

/* Definitions related to MLO specific protocol processing for nodes */

/**
 * enum ieee80211_mlo_rptingrole - IEEE802.11be MLO reporting role for a given
 * STA
 * @IEEE80211_MLO_RPTINGROLE_NA: IEEE802.11be MLO reporting role is not
 * applicable
 * @IEEE80211_MLO_RPTINGROLE_REPORTINGSTA: The STA is an IEEE802.11be MLO
 * reporting STA
 * @IEEE80211_MLO_RPTINGROLE_REPORTEDSTA: The STA is an IEEE802.11be MLO
 * reported STA
 * @IEEE80211_MLO_RPTINGROLE_INVALID: Start of invalid IEEE802.11be MLO
 * reporting role values (this is different from IEEE80211_MLO_RPTINGROLE_NA,
 * since IEEE80211_MLO_RPTINGROLE_NA is a valid value that simply signifies that
 * the reporting role does not apply, e.g. because the STA is a pre-802.11be
 * device)
 */
enum ieee80211_mlo_rptingrole {
    IEEE80211_MLO_RPTINGROLE_NA = 0,
    IEEE80211_MLO_RPTINGROLE_REPORTINGSTA = 1,
    IEEE80211_MLO_RPTINGROLE_REPORTEDSTA = 2,
    IEEE80211_MLO_RPTINGROLE_INVALID,
};

/*End of definitions related to MLO specific protocol processing for nodes */

#endif /* WLAN_FEATURE_11BE_MLO */

#endif /* _IEEE80211_MLO_H_ */
