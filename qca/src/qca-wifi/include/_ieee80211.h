/*
 * Copyright (c) 2017-2023 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Notifications and licenses are retained for attribution purposes only
 *
 * Copyright (c) 2008 Atheros Communications Inc.
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

#ifndef _NET80211__IEEE80211_H_
#define _NET80211__IEEE80211_H_
#include "ieee80211_rrm.h"
#include "ieee80211_wnm_proto.h"
#include "ieee80211_phytype.h"
#if QCA_SUPPORT_SSID_STEERING
#include "ath_ssid_steering.h"
#endif
#if ATH_PARAMETER_API
#include "ieee80211_parameter_api.h"
#endif

/**
 * enum wlan_band_id - Operational wlan band id
 * @WLAN_BAND_UNSPECIFIED: Band id not specified. Default to 2GHz/5GHz band
 * @WLAN_BAND_2GHZ: Band 2.4 GHz
 * @WLAN_BAND_5GHZ: Band 5 GHz
 * @WLAN_BAND_6GHZ: Band 6 GHz
 * @WLAN_BAND_MAX:  Max supported band
 */
enum wlan_band_id {
    WLAN_BAND_UNSPECIFIED = 0,
    WLAN_BAND_2GHZ = 1,
    WLAN_BAND_5GHZ = 2,
    WLAN_BAND_6GHZ = 3,
    /* Add new band definitions here */
    WLAN_BAND_MAX,
};

/**
 * struct wlan_supported_freq_bands - supported frequency bands
 * @band_2ghz: Band 2.4 GHz supported
 * @band_5ghz: Band 5 GHz supported
 * @band_6ghz: Band 6 GHz supported
 */
struct wlan_supported_freq_bands {
    u_int8_t band_2ghz:1,
             band_5ghz:1,
             band_6ghz:1;
};

/* FW is not supporting all rates for the current mode,
 * but only rates specific to current NSS setting.
 * Ideally, FW should support all the rates for the current mode
 * regardless of any NSS setting, Channel Bandwidth, Until FW
 * claims to support all rates, this value should be zero
 */
/* #define ALL_POSSIBLE_RATES_SUPPORTED 1 */

/* These defines should match the table from ah_internal.h */
typedef enum {
	DFS_UNINIT_DOMAIN	= 0,	/* Uninitialized dfs domain */
	DFS_FCC_DOMAIN		= 1,	/* FCC3 dfs domain */
	DFS_ETSI_DOMAIN		= 2,	/* ETSI dfs domain */
	DFS_MKK4_DOMAIN		= 3	/* Japan dfs domain */
}HAL_DFS_DOMAIN;


enum ieee80211_mode {
    IEEE80211_MODE_INVALID = 0,
    IEEE80211_MODE_A,
    IEEE80211_MODE_B,
    IEEE80211_MODE_G,
    IEEE80211_MODE_TURBO,
    IEEE80211_MODE_NA,
    IEEE80211_MODE_NG,
    IEEE80211_MODE_N,
    IEEE80211_MODE_AC,
    IEEE80211_MODE_AXA,
    IEEE80211_MODE_AXG,
    IEEE80211_MODE_AX,
#ifdef WLAN_FEATURE_11BE
    IEEE80211_MODE_BEA,
    IEEE80211_MODE_BEG,
    IEEE80211_MODE_BE,
#endif /* WLAN_FEATURE_11BE */
    /* New modes should be added here */
    IEEE80211_MODE_MAX,
};


/* XXX not really a mode; there are really multiple PHY's */
enum ieee80211_phymode {
    IEEE80211_MODE_AUTO             = 0,    /* autoselect */
    IEEE80211_MODE_11A              = 1,    /* 5GHz, OFDM */
    IEEE80211_MODE_11B              = 2,    /* 2GHz, CCK */
    IEEE80211_MODE_11G              = 3,    /* 2GHz, OFDM */
    IEEE80211_MODE_FH               = 4,    /* 2GHz, GFSK */
    IEEE80211_MODE_TURBO_A          = 5,    /* 5GHz, OFDM, 2x clock dynamic turbo */
    IEEE80211_MODE_TURBO_G          = 6,    /* 2GHz, OFDM, 2x clock dynamic turbo */
    IEEE80211_MODE_11NA_HT20        = 7,    /* 5Ghz, HT20 */
    IEEE80211_MODE_11NG_HT20        = 8,    /* 2Ghz, HT20 */
    IEEE80211_MODE_11NA_HT40PLUS    = 9,    /* 5Ghz, HT40 (ext ch +1) */
    IEEE80211_MODE_11NA_HT40MINUS   = 10,   /* 5Ghz, HT40 (ext ch -1) */
    IEEE80211_MODE_11NG_HT40PLUS    = 11,   /* 2Ghz, HT40 (ext ch +1) */
    IEEE80211_MODE_11NG_HT40MINUS   = 12,   /* 2Ghz, HT40 (ext ch -1) */
    IEEE80211_MODE_11NG_HT40        = 13,   /* 2Ghz, Auto HT40 */
    IEEE80211_MODE_11NA_HT40        = 14,   /* 5Ghz, Auto HT40 */
    IEEE80211_MODE_11AC_VHT20       = 15,   /* 5Ghz, VHT20 */
    IEEE80211_MODE_11AC_VHT40PLUS   = 16,   /* 5Ghz, VHT40 (Ext ch +1) */
    IEEE80211_MODE_11AC_VHT40MINUS  = 17,   /* 5Ghz  VHT40 (Ext ch -1) */
    IEEE80211_MODE_11AC_VHT40       = 18,   /* 5Ghz, VHT40 */
    IEEE80211_MODE_11AC_VHT80       = 19,   /* 5Ghz, VHT80 */
    IEEE80211_MODE_11AC_VHT160      = 20,   /* 5Ghz, VHT160 */
    IEEE80211_MODE_11AC_VHT80_80    = 21,   /* 5Ghz, VHT80_80 */
    IEEE80211_MODE_11AXA_HE20       = 22,   /* 5GHz, HE20 */
    IEEE80211_MODE_11AXG_HE20       = 23,   /* 2GHz, HE20 */
    IEEE80211_MODE_11AXA_HE40PLUS   = 24,   /* 5GHz, HE40 (ext ch +1) */
    IEEE80211_MODE_11AXA_HE40MINUS  = 25,   /* 5GHz, HE40 (ext ch -1) */
    IEEE80211_MODE_11AXG_HE40PLUS   = 26,   /* 2GHz, HE40 (ext ch +1) */
    IEEE80211_MODE_11AXG_HE40MINUS  = 27,   /* 2GHz, HE40 (ext ch -1) */
    IEEE80211_MODE_11AXA_HE40       = 28,   /* 5GHz, HE40 */
    IEEE80211_MODE_11AXG_HE40       = 29,   /* 2GHz, HE40 */
    IEEE80211_MODE_11AXA_HE80       = 30,   /* 5GHz, HE80 */
    IEEE80211_MODE_11AXA_HE160      = 31,   /* 5GHz, HE160 */
    IEEE80211_MODE_11AXA_HE80_80    = 32,   /* 5GHz, HE80_80 */
#ifdef WLAN_FEATURE_11BE
    IEEE80211_MODE_11BEA_EHT20       = 33,   /* 5GHz, EHT20 */
    IEEE80211_MODE_11BEG_EHT20       = 34,   /* 2GHz, EHT20 */
    IEEE80211_MODE_11BEA_EHT40PLUS   = 35,   /* 5GHz, EHT40 (ext ch +1) */
    IEEE80211_MODE_11BEA_EHT40MINUS  = 36,   /* 5GHz, EHT40 (ext ch -1) */
    IEEE80211_MODE_11BEG_EHT40PLUS   = 37,   /* 2GHz, EHT40 (ext ch +1) */
    IEEE80211_MODE_11BEG_EHT40MINUS  = 38,   /* 2GHz, EHT40 (ext ch -1) */
    IEEE80211_MODE_11BEA_EHT40       = 39,   /* 5GHz, EHT40 */
    IEEE80211_MODE_11BEG_EHT40       = 40,   /* 2GHz, EHT40 */
    IEEE80211_MODE_11BEA_EHT80       = 41,   /* 5GHz, EHT80 */
    IEEE80211_MODE_11BEA_EHT160      = 42,   /* 5GHz, EHT160 */
    IEEE80211_MODE_11BEA_EHT320      = 43,   /* 5GHz, EHT320 */
#endif /* WLAN_FEATURE_11BE */
};

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_MODE_MAX      (IEEE80211_MODE_11BEA_EHT320 + 1)
#else
#define IEEE80211_MODE_MAX      (IEEE80211_MODE_11AXA_HE80_80 + 1)
#endif /* WLAN_FEATURE_11BE */
#define IEEE80211_MODE_NONE     (IEEE80211_MODE_MAX + 1)
#define IEEE80211_SUPPORT_11AX  1


enum ieee80211_opmode {
    IEEE80211_M_STA         = 1,                 /* infrastructure station */
    IEEE80211_M_IBSS        = 0,                 /* IBSS (adhoc) station */
    IEEE80211_M_AHDEMO      = 3,                 /* Old lucent compatible adhoc demo */
    IEEE80211_M_HOSTAP      = 6,                 /* Software Access Point */
    IEEE80211_M_MONITOR     = 8,                 /* Monitor mode */
    IEEE80211_M_WDS         = 2,                 /* WDS link */
    IEEE80211_M_BTAMP       = 9,                 /* VAP for BT AMP */

    IEEE80211_M_P2P_GO      = 33,                /* P2P GO */
    IEEE80211_M_P2P_CLIENT  = 34,                /* P2P Client */
    IEEE80211_M_P2P_DEVICE  = 35,                /* P2P Device */


    IEEE80211_OPMODE_MAX    = IEEE80211_M_BTAMP, /* Highest numbered opmode in the list */

    IEEE80211_M_ANY         = 0xFF               /* Any of the above; used by NDIS 6.x */
};

/*
 * 802.11n
 */
#define IEEE80211_CWM_EXTCH_BUSY_THRESHOLD 30

enum ieee80211_cwm_mode {
    IEEE80211_CWM_MODE20,
    IEEE80211_CWM_MODE2040,
    IEEE80211_CWM_MODE40,
    IEEE80211_CWM_MODEMAX

};

enum ieee80211_cwm_extprotspacing {
    IEEE80211_CWM_EXTPROTSPACING20,
    IEEE80211_CWM_EXTPROTSPACING25,
    IEEE80211_CWM_EXTPROTSPACINGMAX
};

#ifdef WLAN_FEATURE_11BE
/*
 * ieee80211_punc_type:
 * Type of puncturing denoting the number of bits that are punctured.
 * Each bit represents a 20MHz channel and therefore, each enum represents
 * the number of 20MHz channels that are punctured.
 */
enum ieee80211_punc_type {
    IEEE80211_PUNC_NONE        = 0,
    IEEE80211_PUNC_MINUS20MHZ  = 1,
    IEEE80211_PUNC_MINUS40MHZ  = 2,
    IEEE80211_PUNC_MINUS60MHZ  = 3,
    IEEE80211_PUNC_MINUS80MHZ  = 4,
    IEEE80211_PUNC_MINUS100MHZ = 5,
    IEEE80211_PUNC_MINUS120MHZ = 6,
    IEEE80211_PUNC_INVALID,
};
#endif /* WLAN_FEATURE_11BE */

enum ieee80211_cwm_width {
    IEEE80211_CWM_WIDTH20,
    IEEE80211_CWM_WIDTH40,
    IEEE80211_CWM_WIDTH80,
    IEEE80211_CWM_WIDTH160,
    IEEE80211_CWM_WIDTH80_80,
#ifdef WLAN_FEATURE_11BE
    IEEE80211_CWM_WIDTH320,
#endif /* WLAN_FEATURE_11BE */

    IEEE80211_CWM_WIDTH_MAX,
    IEEE80211_CWM_WIDTHINVALID = 0xff    /* user invalid value */
};

/* sub  20Mhz channel offsets from center chan of 320Mhz channel */
enum chan_offset_320 {
    SUB20_CHAN_320_1 = -30,
    SUB20_CHAN_320_2 = -26,
    SUB20_CHAN_320_3 = -22,
    SUB20_CHAN_320_4 = -18,
    SUB20_CHAN_320_5 = -14,
    SUB20_CHAN_320_6 = -10,
    SUB20_CHAN_320_7 = -6,
    SUB20_CHAN_320_8 = -2,
    SUB20_CHAN_320_9 = 2,
    SUB20_CHAN_320_10 = 6,
    SUB20_CHAN_320_11 = 10,
    SUB20_CHAN_320_12 = 14,
    SUB20_CHAN_320_13 = 18,
    SUB20_CHAN_320_14 = 22,
    SUB20_CHAN_320_15 = 26,
    SUB20_CHAN_320_16 = 30,
};

static inline enum ieee80211_cwm_width get_chwidth_phymode(enum ieee80211_phymode mode)
{
    enum ieee80211_cwm_width channel_width = IEEE80211_CWM_WIDTHINVALID;
    switch (mode) {
        case IEEE80211_MODE_11B:
        case IEEE80211_MODE_11G:
        case IEEE80211_MODE_11A:
        case IEEE80211_MODE_FH:
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
            channel_width = IEEE80211_CWM_WIDTH20;
            break;
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
            channel_width= IEEE80211_CWM_WIDTH40;
            break;
        case IEEE80211_MODE_11AC_VHT80:
        case IEEE80211_MODE_11AXA_HE80:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT80:
#endif /* WLAN_FEATURE_11BE */
            channel_width = IEEE80211_CWM_WIDTH80;
            break;
        case IEEE80211_MODE_11AC_VHT160:
        case IEEE80211_MODE_11AXA_HE160:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT160:
#endif /* WLAN_FEATURE_11BE */
            channel_width = IEEE80211_CWM_WIDTH160;
            break;
        case IEEE80211_MODE_11AC_VHT80_80:
        case IEEE80211_MODE_11AXA_HE80_80:
            channel_width = IEEE80211_CWM_WIDTH80_80;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT320:
            channel_width = IEEE80211_CWM_WIDTH320;
            break;
#endif /* WLAN_FEATURE_11BE */
        default:
            channel_width = IEEE80211_CWM_WIDTHINVALID;
            break;
    }
    return channel_width;
}

enum ieee80211_cwm_extprotmode {
    IEEE80211_CWM_EXTPROTNONE,      /* no protection */
    IEEE80211_CWM_EXTPROTCTSONLY,   /* CTS to self */
    IEEE80211_CWM_EXTPROTRTSCTS,    /* RTS-CTS */
    IEEE80211_CWM_EXTPROTMAX
};

enum ieee80211_fixed_rate_mode {
    IEEE80211_FIXED_RATE_NONE   = 0,
    IEEE80211_FIXED_RATE_MCS    = 1, /* HT rates */
    IEEE80211_FIXED_RATE_LEGACY = 2, /* legacy rates */
    IEEE80211_FIXED_RATE_VHT    = 3, /* VHT rates */
    IEEE80211_FIXED_RATE_HE     = 4, /*  HE rates */
#ifdef WLAN_FEATURE_11BE
    IEEE80211_FIXED_RATE_EHT    = 5, /* EHT rates */
#endif /* WLAN_FEATURE_11BE */
};

/* Holds the fixed rate information for each VAP */
struct ieee80211_fixed_rate {
    enum ieee80211_fixed_rate_mode  mode;
    u_int32_t                       series;
    u_int32_t                       retries;
};

/*
 * 802.11g protection mode.
 */
enum ieee80211_protmode {
    IEEE80211_PROT_NONE     = 0,    /* no protection */
    IEEE80211_PROT_CTSONLY  = 1,    /* CTS to self */
    IEEE80211_PROT_RTSCTS   = 2,    /* RTS-CTS */
};

/*
 * Roaming mode is effectively who controls the operation
 * of the 802.11 state machine when operating as a station.
 * State transitions are controlled either by the driver
 * (typically when management frames are processed by the
 * hardware/firmware), the host (auto/normal operation of
 * the 802.11 layer), or explicitly through ioctl requests
 * when applications like wpa_supplicant want control.
 */
enum ieee80211_roamingmode {
    IEEE80211_ROAMING_DEVICE= 0,    /* driver/hardware control */
    IEEE80211_ROAMING_AUTO  = 1,    /* 802.11 layer control */
    IEEE80211_ROAMING_MANUAL= 2,    /* application control */
};

#define IEEE80211_FRAME_INJECTOR_MAX_DUR    32000 /* Max duration field value in usecs */

enum ieee80211_frame_injector_type {
    IEEE80211_FRAME_INJECTOR_TYPE_QOSNULL = 0,      /* QoSNULL frame type */
    IEEE80211_FRAME_INJECTOR_TYPE_CTS_TO_SELF = 1,  /* CTS to self frame type */
    IEEE80211_FRAME_INJECTOR_TYPE_MGMT = 2,         /* Mgmt frame type */
    IEEE80211_FRAME_INJECTOR_TYPE_MAX,
};

enum ieee80211_injector_frame_protection_mode {
    IEEE80211_UNPROTECTED_FRAME= 0,                 /* Unprotected frame */
    IEEE80211_PROTECTED_CORRECT_KEY_FRAME = 1,      /* Protected(correct key) frame */
    IEEE80211_PROTECTED_INCORRECT_KEY_FRAME= 2,     /* Protected(incorrect key) frame */
    IEEE80211_PROTECTED_FRAME_TYPE_MAX,
};

/*
 * Scanning mode controls station scanning work; this is
 * used only when roaming mode permits the host to select
 * the bss to join/channel to use.
 */
enum ieee80211_scanmode {
    IEEE80211_SCAN_DEVICE   = 0,    /* driver/hardware control */
    IEEE80211_SCAN_BEST     = 1,    /* 802.11 layer selects best */
    IEEE80211_SCAN_FIRST    = 2,    /* take first suitable candidate */
};

#ifdef WLAN_FEATURE_11BE
#define PUNCTURE_INVALID 0xFFFF
#define PUNCTURE_NONE    0x0000 /* in the bitmap 0 indicates no puncturing and 1 indicated that sub channel is punctured */
#define PUNCTURE_80MHZ_MASK 0xF
#define PUNCTURE_160MHZ_MASK 0xFF
#define PUNCTURE_320MHZ_MASK 0xFFFF

#define IEEE80211_IS_CHAN_PUNCTURABLE(__chan)              \
    (IEEE80211_IS_CHAN_EHT(__chan)    &&                   \
     !IEEE80211_IS_CHAN_20MHZ(__chan) &&                   \
     !IEEE80211_IS_CHAN_40MHZ(__chan) &&                   \
     !IEEE80211_IS_CHAN_80_80MHZ(__chan))

#define IEEE80211_IS_PATTERN_PUNCTURED(pattern)            \
    ((pattern) ? true : false)

#define IEEE80211_IS_CHAN_PUNCTURED(__chan)                       \
    (IEEE80211_IS_CHAN_PUNCTURABLE(__chan) &&                      \
     IEEE80211_IS_PATTERN_PUNCTURED((__chan)->op_puncture_bitmap))
#endif /* WLAN_FEATURE_11BE */

typedef struct ieee80211_mib_cycle_cnts periodic_chan_stats_t;

/*
 * Channels are specified by frequency and attributes.
 */
struct ieee80211_ath_channel {
    u_int64_t       ic_flags;       /* see below */
    int8_t          ic_maxregpower; /* maximum regulatory tx power in dBm */
    int8_t          ic_maxpower;    /* maximum tx power in dBm */
    int8_t          ic_minpower;    /* minimum tx power in dBm */
    u_int16_t       ic_freq;        /* setting in Mhz */
    u_int16_t       ic_flagext;     /* see below */
    uint16_t        ic_vhtop_freq_seg1;           /* seg1 Center Channel frequency */
    uint16_t        ic_vhtop_freq_seg2;           /* Seg2 center Channel frequency index for 80+80MHz mode or
						   * center Channel frequency of operating span for 160Mhz mode */
#ifdef WLAN_FEATURE_11BE
    uint16_t        reg_punc_bitmap;             /* Regulatory puncturing bitmap */
    uint16_t        op_puncture_bitmap;           /* Operating puncture bitmap obtained after applying the user configured puncturing and other puncturing restrictions */
#endif
    u_int8_t        ic_ieee;        /* IEEE channel number */
    u_int8_t        ic_regClassId;  /* regClassId of this channel */
    u_int8_t        ic_antennamax;  /* antenna gain max from regulatory */

    /* 11AX TODO (Phase II) - Rename ic_vhtop_ch_freq_seg1/seg2 to make them
     * agnostic of VHT. This would have to take regulatory convergence into
     * consideration, which may touch the channel structure to an extent.
     */
    u_int8_t        ic_vhtop_ch_num_seg1;         /* Seg1 center Channel index */
    u_int8_t        ic_vhtop_ch_num_seg2;         /* Seg2 center Channel index for 80+80MHz mode or
						   * center Channel index of operating span for 160Mhz mode */
};

static const struct ieee80211_ath_channel empty_channel = {};

enum wlan_afc_power_type {
    AFC_POWER_TYPE_LP,
    AFC_POWER_TYPE_SP,
    AFC_POWER_TYPE_VLP,
    AFC_POWER_TYPE_MAX,
};

struct wlan_chan_6g {
    uint8_t chan_idx;
    uint8_t cfreq2;
    uint16_t puncture_bitmap;
    int16_t psd_pwr;
    int32_t eirp_pwr;
};

/* Rounded value */
#define NUM_6GHZ_CHANS 60

struct wlan_chan_list_6g {
    uint16_t num_valid_chan[IEEE80211_CWM_WIDTH_MAX];
    struct wlan_chan_6g chan[IEEE80211_CWM_WIDTH_MAX][NUM_6GHZ_CHANS];
};

/**
  * chan_params: Chanel parameters required to identify channel
  * freq: Primary 20MHz frequency
  * cfreq2: Center frequency of continuous bandwidth channel
  *           or center frequency of secondary 80 of 80+80 channel
  * puncture_low_pwr: Boolean flag that determines whether driver can modify puncture pattern
  *                   if eirp power is less than threshold
  * puncture_bitmap: Configured puncture bitmap
  * op_puncture_bitmap: 0 indicates Operating puncture bitmap not derived where as value here indicates derived operating puncture bitmap.
  *                     currently this is used to optimize derivation of puncturing bitmap once it is calculated
  * puncture_strict: Boolean flag that determines if invalid puncturing can
  *                    be modified by the driver or puncturing should be
  *                    denied within the driver if the user config is invalid
  * input_6g_pwr_mode: Channel parameter that is specific for 6G channels. It indicates the 6G power type.
  */
struct chan_params {
    uint16_t freq;
    uint16_t cfreq2;
#ifdef WLAN_FEATURE_11BE
    uint8_t  puncture_low_pwr;
    uint8_t  puncture_strict;
    uint16_t puncture_bitmap;
    uint16_t op_puncture_bitmap;
#endif
    int32_t input_6g_pwr_mode;
};

/* The first DFS channel number is 52 and the last DFS channel number is 161(in
 * case of ETSI EN302502). So, the array size is taken as (161 - 52) / 4 ~= 30.
 */
#define NUM_DFS_CHANS      30
/* Number of 5 GHz channels (inclusive of both DFS and non-DFS).
 * This is used as a index in ieee80211_dfs_ch_params.
 * The first 5 GHz channel number is 36 and the last channel number is 177
 * (if 5.9 GHz band is present).  So, the array size is taken as
 * (177 - 36) / 4 ~= 40.
*/
#define NUM_5GHZ_CHANS     40
#define IEEE_MAX_CHAN_NUM 255

/**
 * enum channel_dfs_state - DFS  channel states.
 * @WLAN_CH_DFS_S_INVALID: The DFS state for invalid channel numbers that are not
 *  WLAN_                  part of the radio's channel list.
 * @WLAN_CH_DFS_S_CAC_REQ: Indicates that the CAC/Off-channel CAC has to performed
 *  WLAN_                  before Tx on the DFS channel.
 * @WLAN_CH_DFS_S_CAC_STARTED: Indicates that the CAC has been started for the DFS
 *  WLAN_                      channel.
 * @WLAN_CH_DFS_S_CAC_COMPLETED: Indicates that the CAC has been completed for the
 *  WLAN_                        DFS channel.
 * @WLAN_CH_DFS_S_NOL: Indicates that the DFS channel is in NOL.
 * @WLAN_CH_DFS_S_PRECAC_STARTED: Indicates that the PreCAC has been started for the
 *  WLAN_                         DFS channel.
 * @WLAN_CH_DFS_S_PRECAC_COMPLETED: Indicates that the PreCAC has been completed for
 *                             the DFS channel.
 * @WLAN_CH_DFS_S_NON_DFS: Indicates that it is a non-DFS channel.
 */
enum wlan_channel_dfs_state {
	WLAN_CH_DFS_S_INVALID,
	WLAN_CH_DFS_S_CAC_REQ,
	WLAN_CH_DFS_S_CAC_STARTED,
	WLAN_CH_DFS_S_CAC_COMPLETED,
	WLAN_CH_DFS_S_NOL,
	WLAN_CH_DFS_S_PRECAC_STARTED,
	WLAN_CH_DFS_S_PRECAC_COMPLETED,
	WLAN_CH_DFS_S_NON_DFS,
};

struct ieee80211_vap_info {
    struct ieee80211_ath_channel *chan;
    u_int8_t no_cac;
};

#ifdef WLAN_FEATURE_11BE
#define IEE80211_MAX_20M_SUB_CH 16 /* In case of EHT320, we can have 16 20Mhz channels */
#else
#define IEE80211_MAX_20M_SUB_CH 8 /* In case of VHT160, we can have 8 20Mhz channels */
#endif

#define	IEEE80211_NWID_LEN	32

#ifdef CONFIG_BAND_6GHZ
#define IEEE80211_CHAN_MAX      2047
#else
#define IEEE80211_CHAN_MAX      1023
#endif

#define IEEE80211_FREQ_MAX      7125
#define IEEE80211_CHAN_BYTES    128      /* howmany(IEEE80211_CHAN_MAX, NBBY) */ //can be 32
#define IEEE80211_CHAN_ANY      (-1)    /* token for ``any channel'' */
#define IEEE80211_FREQ_ANY      (-1)    /* token for ``any frequency'' */
#define IEEE80211_CHAN_ANYC \
        ((struct ieee80211_ath_channel *) IEEE80211_CHAN_ANY)

#define IEEE80211_CHAN_DEFAULT          2462
#define IEEE80211_CHAN_DEFAULT_11A      5260
#define IEEE80211_CHAN_ADHOC_DEFAULT1   2457
#define IEEE80211_CHAN_ADHOC_DEFAULT2   2462

#define IEEE80211_RADAR_11HCOUNT        5
#define IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11A      5180      /* Move to channel 36 for mute test */
#define IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11NHT20  5180
#define IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11NHT40U 5180
#define IEEE80211_RADAR_TEST_MUTE_CHAN_FREQ_11NHT40D 5200      /* Move to channel 40 for HT40D mute test */
#define IEEE80211_RADAR_DETECT_DEFAULT_DELAY    60000   /* STA ignore AP beacons during this period in millisecond */

#define IEEE80211_2GCSA_TBTTCOUNT        3
#define IEEE80211_MAX_CSA_TBTTCOUNT       255
#define IEEE80211_OBSS_CSA_TBTTCOUNT      1

/* bits 0-3 are for private use by drivers */
/* channel attributes */
#define IEEE80211_CHAN_20MHZ            0x0000000000000100
#define IEEE80211_CHAN_40PLUS           0x0000000000000200
#define IEEE80211_CHAN_40MINUS          0x0000000000000300
#define IEEE80211_CHAN_80MHZ            0x0000000000000400
#define IEEE80211_CHAN_160MHZ           0x0000000000000500
#define IEEE80211_CHAN_80_80MHZ         0x0000000000000600
#ifdef WLAN_FEATURE_11BE
#define IEEE80211_CHAN_320MHZ           0x0000000000000700
#endif /* WLAN_FEATURE_11BE */

#define IEEE80211_CHAN_HTCAP            0x0000000000001000
#define IEEE80211_CHAN_VHTCAP           0x0000000000002000
#define IEEE80211_CHAN_HECAP            0x0000000000003000
#ifdef WLAN_FEATURE_11BE
#define IEEE80211_CHAN_EHTCAP           0x0000000000004000
#endif /* WLAN_FEATURE_11BE */

#define IEEE80211_CHAN_2GHZ             0x0000000000000010 /* 2 GHz spectrum channel */
#define IEEE80211_CHAN_5GHZ             0x0000000000000020 /* 5 GHz spectrum channel */
#define IEEE80211_CHAN_6GHZ             0x0000000000000030 /* 6 GHz spectrum channel */

#define IEEE80211_CHAN_HT20             (IEEE80211_CHAN_HTCAP | IEEE80211_CHAN_20MHZ)   /* HT 20 channel */
#define IEEE80211_CHAN_HT40PLUS         (IEEE80211_CHAN_HTCAP | IEEE80211_CHAN_40PLUS)  /* HT 40 with extension channel above */
#define IEEE80211_CHAN_HT40MINUS        (IEEE80211_CHAN_HTCAP | IEEE80211_CHAN_40MINUS) /* HT 40 with extension channel below */

#define IEEE80211_CHAN_VHT20            (IEEE80211_CHAN_VHTCAP | IEEE80211_CHAN_20MHZ)    /* VHT 20 channel */
#define IEEE80211_CHAN_VHT40PLUS        (IEEE80211_CHAN_VHTCAP | IEEE80211_CHAN_40PLUS)   /* VHT 40 with extension channel above */
#define IEEE80211_CHAN_VHT40MINUS       (IEEE80211_CHAN_VHTCAP | IEEE80211_CHAN_40MINUS)  /* VHT 40 with extension channel below */
#define IEEE80211_CHAN_VHT80            (IEEE80211_CHAN_VHTCAP | IEEE80211_CHAN_80MHZ)    /* VHT 80 channel */
#define IEEE80211_CHAN_VHT160           (IEEE80211_CHAN_VHTCAP | IEEE80211_CHAN_160MHZ)   /* VHT 160 channel */
#define IEEE80211_CHAN_VHT80_80         (IEEE80211_CHAN_VHTCAP | IEEE80211_CHAN_80_80MHZ) /* VHT 80_80 channel */

#define IEEE80211_CHAN_HE20             (IEEE80211_CHAN_HECAP | IEEE80211_CHAN_20MHZ)    /* HE 20 channel */
#define IEEE80211_CHAN_HE40PLUS         (IEEE80211_CHAN_HECAP | IEEE80211_CHAN_40PLUS)   /* HE 40 with extension channel above */
#define IEEE80211_CHAN_HE40MINUS        (IEEE80211_CHAN_HECAP | IEEE80211_CHAN_40MINUS)  /* HE 40 with extension channel below */
#define IEEE80211_CHAN_HE80             (IEEE80211_CHAN_HECAP | IEEE80211_CHAN_80MHZ)    /* HE 80 channel */
#define IEEE80211_CHAN_HE160            (IEEE80211_CHAN_HECAP | IEEE80211_CHAN_160MHZ)   /* HE 160 channel */
#define IEEE80211_CHAN_HE80_80          (IEEE80211_CHAN_HECAP | IEEE80211_CHAN_80_80MHZ) /* HE 80_80 channel */

#define IEEE80211_CHAN_PASSIVE          0x0000000000100000 /* Only passive scan allowed */
#define IEEE80211_CHAN_CCK              0x0000000000200000 /* CCK channel */
#define IEEE80211_CHAN_OFDM             0x0000000000400000 /* OFDM channel */
#define IEEE80211_CHAN_DYN              0x0000000000800000 /* Dynamic CCK-OFDM channel */
#define IEEE80211_CHAN_GFSK             0x0000000001000000 /* GFSK channel (FHSS PHY) */
#define IEEE80211_CHAN_DFS_RADAR        0x0000000002000000 /* Radar found on channel */
#define IEEE80211_CHAN_HALF             0x0000000004000000 /* Half rate channel */
#define IEEE80211_CHAN_QUARTER          0x0000000008000000 /* Quarter rate channel */
#define IEEE80211_CHAN_BLOCKED          0x0000000010000000 /* channel temporarily blocked due to noise */

#define IEEE80211_CHAN_TURBO            0x0000000020000000 /* Turbo channel */
#define IEEE80211_CHAN_STURBO           0x0000000040000000 /* 11a static turbo channel only */
/* Do not add anything here unrelated to PHY mode and width processing. */

#define IEEE80211_CHAN_WIDTH_MASK       0x0000000000000F00
#define IEEE80211_CHAN_CAP_MASK         0x000000000000F000

#define IEEE80211_CHAN_BAND_MASK        0x00000000000000F0 /* Mask only band */
#define IEEE80211_CHAN_BW_MASK          0x000000000000FF00 /* Mask only bandwidths */
#define IEEE80211_CHAN_BAND_BW_MASK     (IEEE80211_CHAN_BAND_MASK | IEEE80211_CHAN_BW_MASK) /* Mask both band and bandwidth */

#define IEEE80211_CHAN_FHSS \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_GFSK)
#define IEEE80211_CHAN_A \
    (IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM)
#define IEEE80211_CHAN_B \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_CCK)
#define IEEE80211_CHAN_PUREG \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM)
#define IEEE80211_CHAN_G \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_DYN)
#define IEEE80211_CHAN_108A \
    (IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_TURBO)
#define IEEE80211_CHAN_108G \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_TURBO)
#define IEEE80211_CHAN_ST \
    (IEEE80211_CHAN_108A | IEEE80211_CHAN_STURBO)

#define IEEE80211_IS_CHAN_2GHZ(_c)              IEEE80211_IS_FLAG_2GHZ((_c)->ic_flags)
#define IEEE80211_IS_CHAN_5GHZ(_c)              IEEE80211_IS_FLAG_5GHZ((_c)->ic_flags)
#define IEEE80211_IS_CHAN_6GHZ(_c)              IEEE80211_IS_FLAG_6GHZ((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HT20(_c)           IEEE80211_IS_FLAG_HT20((_c)->ic_flags)

#define IEEE80211_IS_CHAN_BW_HT40PLUS(_c)       IEEE80211_IS_FLAG_HT40PLUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HT40MINUS(_c)      IEEE80211_IS_FLAG_HT40MINUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HT40(_c)           IEEE80211_IS_FLAG_HT40((_c)->ic_flags)

#define IEEE80211_IS_CHAN_BW_VHT20(_c)          IEEE80211_IS_FLAG_VHT20((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT40PLUS(_c)      IEEE80211_IS_FLAG_VHT40PLUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT40MINUS(_c)     IEEE80211_IS_FLAG_VHT40MINUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT40(_c)          IEEE80211_IS_FLAG_VHT40((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT80(_c)          IEEE80211_IS_FLAG_VHT80((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT160(_c)         IEEE80211_IS_FLAG_VHT160((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT80_80(_c)       IEEE80211_IS_FLAG_VHT80_80((_c)->ic_flags)

#define IEEE80211_IS_CHAN_BW_HE20(_c)           IEEE80211_IS_FLAG_HE20((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE40PLUS(_c)       IEEE80211_IS_FLAG_HE40PLUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE40MINUS(_c)      IEEE80211_IS_FLAG_HE40MINUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE40(_c)           IEEE80211_IS_FLAG_HE40((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE80(_c)           IEEE80211_IS_FLAG_HE80((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE160(_c)          IEEE80211_IS_FLAG_HE160((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE80_80(_c)        IEEE80211_IS_FLAG_HE80_80((_c)->ic_flags)

#define IEEE80211_IS_CHAN_BW_20MHZ(_c)          IEEE80211_IS_FLAG_20((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_40PLUS(_c)         IEEE80211_IS_FLAG_40PLUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_40MINUS(_c)        IEEE80211_IS_FLAG_40MINUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_40MHZ(_c)          IEEE80211_IS_FLAG_40((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_80MHZ(_c)          IEEE80211_IS_FLAG_80((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_160MHZ(_c)         IEEE80211_IS_FLAG_160((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_80_80MHZ(_c)       IEEE80211_IS_FLAG_80_80((_c)->ic_flags)

#define IEEE80211_IS_CHAN_BW_HT(_c)             IEEE80211_IS_FLAG_HT((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_VHT(_c)            IEEE80211_IS_FLAG_VHT((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_HE(_c)             IEEE80211_IS_FLAG_HE((_c)->ic_flags)

#define IEEE80211_IS_CHAN_PASSIVE(_c)           IEEE80211_IS_FLAG_PASSIVE((_c)->ic_flags)
#define IEEE80211_IS_CHAN_CCK(_c)               IEEE80211_IS_FLAG_CCK((_c)->ic_flags)
#define IEEE80211_IS_CHAN_OFDM(_c)              IEEE80211_IS_FLAG_OFDM((_c)->ic_flags)
#define IEEE80211_IS_CHAN_DYN(_c)               IEEE80211_IS_FLAG_DYN((_c)->ic_flags)
#define IEEE80211_IS_CHAN_GFSK(_c)              IEEE80211_IS_FLAG_GFSK((_c)->ic_flags)
#define IEEE80211_IS_CHAN_HALF(_c)              IEEE80211_IS_FLAG_HALF((_c)->ic_flags)
#define IEEE80211_IS_CHAN_QUARTER(_c)           IEEE80211_IS_FLAG_QUARTER((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BLOCKED(_c)           IEEE80211_IS_FLAG_BLOCKED((_c)->ic_flags)
#define IEEE80211_IS_CHAN_TURBO(_c)             IEEE80211_IS_FLAG_TURBO((_c)->ic_flags)
#define IEEE80211_IS_CHAN_STURBO(_c)            IEEE80211_IS_FLAG_STURBO((_c)->ic_flags)

#define IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) \
    ( IEEE80211_IS_CHAN_5GHZ(_c) || IEEE80211_IS_CHAN_6GHZ(_c) )

#define IEEE80211_IS_FLAG_2GHZ(_flag) \
    (((_flag) & IEEE80211_CHAN_BAND_MASK) == IEEE80211_CHAN_2GHZ)
#define IEEE80211_IS_FLAG_5GHZ(_flag) \
    (((_flag) & IEEE80211_CHAN_BAND_MASK) == IEEE80211_CHAN_5GHZ)
#define IEEE80211_IS_FLAG_6GHZ(_flag) \
    (((_flag) & IEEE80211_CHAN_BAND_MASK) == IEEE80211_CHAN_6GHZ)

#define IEEE80211_IS_FLAG_HT20(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HT20)
#define IEEE80211_IS_FLAG_HT40PLUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HT40PLUS)
#define IEEE80211_IS_FLAG_HT40MINUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HT40MINUS)
#define IEEE80211_IS_FLAG_HT40(_flag) \
    ( IEEE80211_IS_FLAG_HT40PLUS(_flag) || IEEE80211_IS_FLAG_HT40MINUS(_flag) )

#define IEEE80211_IS_FLAG_VHT20(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_VHT20)
#define IEEE80211_IS_FLAG_VHT40PLUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_VHT40PLUS)
#define IEEE80211_IS_FLAG_VHT40MINUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_VHT40MINUS)
#define IEEE80211_IS_FLAG_VHT80(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_VHT80)
#define IEEE80211_IS_FLAG_VHT160(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_VHT160)
#define IEEE80211_IS_FLAG_VHT80_80(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_VHT80_80)
#define IEEE80211_IS_FLAG_VHT40(_flag) \
    ( IEEE80211_IS_FLAG_VHT40PLUS(_flag) || IEEE80211_IS_FLAG_VHT40MINUS(_flag) )

#define IEEE80211_IS_FLAG_HE20(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HE20)
#define IEEE80211_IS_FLAG_HE40PLUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HE40PLUS)
#define IEEE80211_IS_FLAG_HE40MINUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HE40MINUS)
#define IEEE80211_IS_FLAG_HE80(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HE80)
#define IEEE80211_IS_FLAG_HE160(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HE160)
#define IEEE80211_IS_FLAG_HE80_80(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_HE80_80)
#define IEEE80211_IS_FLAG_HE40(_flag) \
   ( IEEE80211_IS_FLAG_HE40PLUS(_flag) || IEEE80211_IS_FLAG_HE40MINUS(_flag) )

#define IEEE80211_IS_FLAG_PASSIVE(_flag) \
    ((_flag) & IEEE80211_CHAN_PASSIVE)
#define IEEE80211_IS_FLAG_CCK(_flag) \
    ((_flag) &  IEEE80211_CHAN_CCK)
#define IEEE80211_IS_FLAG_OFDM(_flag) \
    ((_flag) & IEEE80211_CHAN_OFDM)
#define IEEE80211_IS_FLAG_DYN(_flag) \
    ((_flag) & IEEE80211_CHAN_DYN)
#define IEEE80211_IS_FLAG_GFSK(_flag) \
    ((_flag) & IEEE80211_CHAN_GFSK)
#define IEEE80211_IS_FLAG_HALF(_flag) \
    ((_flag) & IEEE80211_CHAN_HALF)
#define IEEE80211_IS_FLAG_QUARTER(_flag) \
    ((_flag) & IEEE80211_CHAN_QUARTER)
#define IEEE80211_IS_FLAG_BLOCKED(_flag) \
    ((_flag) &  IEEE80211_CHAN_BLOCKED)

#define IEEE80211_IS_FLAG_TURBO(_flag) \
    ((_flag) & IEEE80211_CHAN_TURBO)
#define IEEE80211_IS_FLAG_STURBO(_flag) \
    ((_flag) & IEEE80211_CHAN_STURBO)

#define IEEE80211_IS_FLAG_20(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_20MHZ)
#define IEEE80211_IS_FLAG_40PLUS(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_40PLUS)
#define IEEE80211_IS_FLAG_40MINUS(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_40MINUS)
#define IEEE80211_IS_FLAG_40(_flag) \
    (IEEE80211_IS_FLAG_40MINUS(_flag) || IEEE80211_IS_FLAG_40PLUS(_flag))
#define IEEE80211_IS_FLAG_80(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_80MHZ)
#define IEEE80211_IS_FLAG_160(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_160MHZ)
#define IEEE80211_IS_FLAG_80_80(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_80_80MHZ)

#define IEEE80211_IS_FLAG_HT(_flag) \
    (((_flag) & IEEE80211_CHAN_CAP_MASK) == IEEE80211_CHAN_HTCAP)
#define IEEE80211_IS_FLAG_VHT(_flag) \
    (((_flag) & IEEE80211_CHAN_CAP_MASK) == IEEE80211_CHAN_VHTCAP)
#define IEEE80211_IS_FLAG_HE(_flag) \
    (((_flag) & IEEE80211_CHAN_CAP_MASK) == IEEE80211_CHAN_HECAP)

#define IS_FLAG_11A(_flag) \
    ((_flag & IEEE80211_CHAN_A) == IEEE80211_CHAN_A)

/* flagext */
#define	IEEE80211_CHAN_DFS_RADAR_FOUND    0x01
#define IEEE80211_CHAN_DFS              0x0002  /* DFS required on channel */
#define IEEE80211_CHAN_DFS_CFREQ2       0x0004  /* DFS required on channel for 2nd band of 80+80*/
#define IEEE80211_CHAN_DFS_CLEAR        0x0008  /* if channel has been checked for DFS */
#define IEEE80211_CHAN_11D_EXCLUDED     0x0010  /* excluded in 11D */
#define IEEE80211_CHAN_CSA_RECEIVED     0x0020  /* Channel Switch Announcement received on this channel */
#define IEEE80211_CHAN_DISALLOW_ADHOC   0x0040  /* ad-hoc is not allowed */
#define IEEE80211_CHAN_DISALLOW_HOSTAP  0x0080  /* Station only channel */
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
#define IEEE80211_CHAN_HISTORY_RADAR    0x0100  /* DFS radar history for slave device(STA mode) */
#define IEEE80211_CHAN_CAC_VALID        0x0200  /* DFS CAC valid for  slave device(STA mode) */
#endif
#define IEEE80211_CHAN_PSC              0x0400  /* Channel is Preffered Scanning Channel */

/*
 * Useful combinations of channel characteristics.
 */

#define IEEE80211_CHAN_ALL \
        (IEEE80211_CHAN_BAND_MASK |  IEEE80211_CHAN_GFSK | \
         IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_DYN | \
         IEEE80211_CHAN_WIDTH_MASK | IEEE80211_CHAN_CAP_MASK | \
         IEEE80211_CHAN_HALF | IEEE80211_CHAN_QUARTER)

#define IEEE80211_CHAN_ALLTURBO \
    (IEEE80211_CHAN_ALL | IEEE80211_CHAN_TURBO | IEEE80211_CHAN_STURBO)

#define IEEE80211_IS_CHAN_FHSS(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_FHSS) == IEEE80211_CHAN_FHSS)
#define IEEE80211_IS_CHAN_A(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_A) == IEEE80211_CHAN_A)
#define IEEE80211_IS_CHAN_B(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_B) == IEEE80211_CHAN_B)
#define IEEE80211_IS_CHAN_PUREG(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_PUREG) == IEEE80211_CHAN_PUREG)
#define IEEE80211_IS_CHAN_G(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_G) == IEEE80211_CHAN_G)
#define IEEE80211_IS_CHAN_ANYG(_c) \
    (IEEE80211_IS_CHAN_PUREG(_c) || IEEE80211_IS_CHAN_G(_c))
#define IEEE80211_IS_CHAN_ST(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_ST) == IEEE80211_CHAN_ST)
#define IEEE80211_IS_CHAN_108A(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_108A) == IEEE80211_CHAN_108A)
#define IEEE80211_IS_CHAN_108G(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_108G) == IEEE80211_CHAN_108G)


#define IEEE80211_IS_CHAN_WEATHER_RADAR(_c) \
    ((((_c)->ic_freq >= 5600) && ((_c)->ic_freq <= 5650)) \
     || ((IEEE80211_IS_CHAN_BW_HT40PLUS(_c)) && (5580 == (_c)->ic_freq)))

#define IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(_c) \
    (IEEE80211_IS_CHAN_DFS(_c) ||       \
     ((IEEE80211_IS_CHAN_11AC_VHT160(_c) || \
       IEEE80211_IS_CHAN_11AC_VHT80_80(_c) || \
       IEEE80211_IS_CHAN_11AXA_HE160(_c) || \
       IEEE80211_IS_CHAN_11AXA_HE80_80(_c) || \
       IEEE80211_IS_CHAN_11BEA_EHT160(_c))\
      && IEEE80211_IS_CHAN_DFS_CFREQ2(_c)))

#define IEEE80211_IS_CHAN_DFS(_c) \
    (((_c)->ic_flagext & (IEEE80211_CHAN_DFS|IEEE80211_CHAN_DFS_CLEAR)) == IEEE80211_CHAN_DFS)
#define IEEE80211_IS_CHAN_DFS_CFREQ2(_c) \
    (((_c)->ic_flagext & (IEEE80211_CHAN_DFS_CFREQ2|IEEE80211_CHAN_DFS_CLEAR)) == IEEE80211_CHAN_DFS_CFREQ2)
#define IEEE80211_IS_CHAN_DFSFLAG(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DFS) == IEEE80211_CHAN_DFS)
#define IEEE80211_IS_CHAN_DFSFLAG_CFREQ2(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DFS_CFREQ2) == IEEE80211_CHAN_DFS_CFREQ2)
#define IEEE80211_IS_CHAN_DISALLOW_ADHOC(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DISALLOW_ADHOC) != 0)
#define IEEE80211_IS_CHAN_11D_EXCLUDED(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_11D_EXCLUDED) != 0)
#define IEEE80211_IS_CHAN_CSA(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_CSA_RECEIVED) != 0)
#define IEEE80211_IS_CHAN_ODD(_c) \
    (((_c)->ic_freq == 5170) || ((_c)->ic_freq == 5190) || \
     ((_c)->ic_freq == 5210) || ((_c)->ic_freq == 5230))
#define IEEE80211_IS_CHAN_DISALLOW_HOSTAP(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DISALLOW_HOSTAP) != 0)
#define IEEE80211_IS_CHAN_PSC(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_PSC) != 0)

#define IEEE80211_IS_CHAN_11NG_HT20(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HT20(_c))
#define IEEE80211_IS_CHAN_11NA_HT20(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_HT20(_c))
#define IEEE80211_IS_CHAN_11NG_HT40(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HT40(_c))
#define IEEE80211_IS_CHAN_11NG_HT40PLUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HT40PLUS(_c))
#define IEEE80211_IS_CHAN_11NG_HT40MINUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HT40MINUS(_c))
#define IEEE80211_IS_CHAN_11NA_HT40(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_HT40(_c))
#define IEEE80211_IS_CHAN_11NA_HT40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_HT40PLUS(_c))
#define IEEE80211_IS_CHAN_11NA_HT40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_HT40MINUS(_c))

#define IEEE80211_IS_CHAN_11N(_c) \
    (IEEE80211_IS_CHAN_BW_HT20(_c) || IEEE80211_IS_CHAN_BW_HT40MINUS(_c) || IEEE80211_IS_CHAN_BW_HT40PLUS(_c))
#define IEEE80211_IS_CHAN_11N_HT20(_c) \
    (IEEE80211_IS_CHAN_BW_HT20(_c))
#define IEEE80211_IS_CHAN_11N_HT40(_c) \
    (IEEE80211_IS_CHAN_BW_HT40(_c))
#define IEEE80211_IS_CHAN_11NG(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_11N(_c))
#define IEEE80211_IS_CHAN_11NA(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_11N(_c))
#define IEEE80211_IS_CHAN_11N_HT40PLUS(_c) \
    (IEEE80211_IS_CHAN_BW_HT40PLUS(_c))
#define IEEE80211_IS_CHAN_11N_HT40MINUS(_c) \
    (IEEE80211_IS_CHAN_BW_HT40MINUS(_c))

#define IEEE80211_IS_CHAN_HT20_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HT20(_c))
#define IEEE80211_IS_CHAN_HT40PLUS_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HT40PLUS(_c))
#define IEEE80211_IS_CHAN_HT40MINUS_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HT40MINUS(_c))
#define IEEE80211_IS_CHAN_HT40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HT40PLUS_CAPABLE(_c) || IEEE80211_IS_CHAN_HT40MINUS_CAPABLE(_c))
#define IEEE80211_IS_CHAN_HT_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HT20_CAPABLE(_c) || IEEE80211_IS_CHAN_HT40_CAPABLE(_c))
#define IEEE80211_IS_CHAN_11N_CTL_CAPABLE(_c)  IEEE80211_IS_CHAN_HT20_CAPABLE(_c)
#define IEEE80211_IS_CHAN_11N_CTL_U_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HT40PLUS(_c))
#define IEEE80211_IS_CHAN_11N_CTL_L_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HT40MINUS(_c))
#define IEEE80211_IS_CHAN_11N_CTL_40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_11N_CTL_U_CAPABLE(_c) || IEEE80211_IS_CHAN_11N_CTL_L_CAPABLE(_c))

#define IEEE80211_IS_CHAN_VHT(_c) IEEE80211_IS_CHAN_BW_VHT(_c)

#define IEEE80211_IS_CHAN_11AC(_c) \
    ( IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_VHT(_c) )

#define IEEE80211_IS_CHAN_11AC_VHT20(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT20(_c))
#define IEEE80211_IS_CHAN_11AC_VHT40(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT40(_c))
#define IEEE80211_IS_CHAN_11AC_VHT40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT40PLUS(_c))
#define IEEE80211_IS_CHAN_11AC_VHT40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT40MINUS(_c))
#define IEEE80211_IS_CHAN_11AC_VHT80(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT80(_c))
#define IEEE80211_IS_CHAN_11AC_VHT160(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT160(_c))
#define IEEE80211_IS_CHAN_11AC_VHT80_80(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_VHT80_80(_c))

#define IEEE80211_IS_CHAN_HE(_c)  IEEE80211_IS_CHAN_BW_HE(_c)

#define IEEE80211_IS_CHAN_11AX(_c) \
    ((IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) || IEEE80211_IS_CHAN_2GHZ(_c)) && \
     IEEE80211_IS_CHAN_HE(_c))

#define IEEE80211_IS_CHAN_11AXG_HE20(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HE20(_c))
#define IEEE80211_IS_CHAN_11AXA_HE20(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE20(_c))
#define IEEE80211_IS_CHAN_11AXG_HE40PLUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HE40PLUS(_c))
#define IEEE80211_IS_CHAN_11AXG_HE40MINUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HE40MINUS(_c))
#define IEEE80211_IS_CHAN_11AXA_HE40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE40PLUS(_c))
#define IEEE80211_IS_CHAN_11AXA_HE40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE40MINUS(_c))
#define IEEE80211_IS_CHAN_11AXA_HE80(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE80(_c))
#define IEEE80211_IS_CHAN_11AXA_HE160(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE160(_c))
#define IEEE80211_IS_CHAN_11AXA_HE80_80(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE80_80(_c))

#define IEEE80211_IS_CHAN_11AX_HE20(_c) \
    (IEEE80211_IS_CHAN_BW_HE20(_c))
#define IEEE80211_IS_CHAN_11AX_HE40(_c) \
    (IEEE80211_IS_CHAN_BW_HE40(_c))

#define IEEE80211_IS_CHAN_11AXG(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_11AX(_c))
#define IEEE80211_IS_CHAN_11AXA(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_11AX(_c))
#define IEEE80211_IS_CHAN_11AX_HE40PLUS(_c) \
    (IEEE80211_IS_CHAN_BW_HE40PLUS(_c))
#define IEEE80211_IS_CHAN_11AX_HE40MINUS(_c) \
    (IEEE80211_IS_CHAN_BW_HE40MINUS(_c))
#define IEEE80211_IS_CHAN_11AXG_HE40(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_HE40(_c))
#define IEEE80211_IS_CHAN_11AXA_HE40(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_HE40(_c))
#define IEEE80211_IS_CHAN_11AX_HE80(_c) \
    (IEEE80211_IS_CHAN_BW_HE80(_c))
#define IEEE80211_IS_CHAN_11AX_HE160(_c) \
    (IEEE80211_IS_CHAN_BW_HE160(_c))
#define IEEE80211_IS_CHAN_11AX_HE80_80(_c) \
    (IEEE80211_IS_CHAN_BW_HE80_80(_c))

#define IEEE80211_IS_CHAN_HE20_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE20(_c))
#define IEEE80211_IS_CHAN_HE40PLUS_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE40PLUS(_c))
#define IEEE80211_IS_CHAN_HE40MINUS_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE40MINUS(_c))
#define IEEE80211_IS_CHAN_HE40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HE40PLUS_CAPABLE(_c) || \
     IEEE80211_IS_CHAN_HE40MINUS_CAPABLE(_c))
#define IEEE80211_IS_CHAN_HE80_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE80(_c))
#define IEEE80211_IS_CHAN_HE160_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE160(_c))
#define IEEE80211_IS_CHAN_HE80_80_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE80_80(_c))

#define IEEE80211_IS_CHAN_HE_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HE20_CAPABLE(_c) ||  \
     IEEE80211_IS_CHAN_HE40_CAPABLE(_c) ||  \
     IEEE80211_IS_CHAN_HE80_CAPABLE(_c) ||  \
     IEEE80211_IS_CHAN_HE160_CAPABLE(_c) || \
     IEEE80211_IS_CHAN_HE80_80_CAPABLE(_c))

#define IEEE80211_IS_CHAN_20MHZ(_c)  IEEE80211_IS_CHAN_BW_20MHZ(_c)
#define IEEE80211_IS_CHAN_40MHZ(_c)   IEEE80211_IS_CHAN_BW_40MHZ(_c)
#define IEEE80211_IS_CHAN_40PLUS(_c)  IEEE80211_IS_CHAN_BW_40PLUS(_c)
#define IEEE80211_IS_CHAN_40MINUS(_c)   IEEE80211_IS_CHAN_BW_40MINUS(_c)
#define IEEE80211_IS_CHAN_80MHZ(_c)  IEEE80211_IS_CHAN_BW_80MHZ(_c)
#define IEEE80211_IS_CHAN_160MHZ(_c)  IEEE80211_IS_CHAN_BW_160MHZ(_c)
#define IEEE80211_IS_CHAN_80_80MHZ(_c)  IEEE80211_IS_CHAN_BW_80_80MHZ(_c)

#ifdef WLAN_FEATURE_11BE

#define IEEE80211_CHAN_EHT20             (IEEE80211_CHAN_EHTCAP | IEEE80211_CHAN_20MHZ)    /* EHT 20 channel */
#define IEEE80211_CHAN_EHT40PLUS         (IEEE80211_CHAN_EHTCAP | IEEE80211_CHAN_40PLUS)   /* EHT 40 with extension channel above */
#define IEEE80211_CHAN_EHT40MINUS        (IEEE80211_CHAN_EHTCAP | IEEE80211_CHAN_40MINUS)  /* EHT 40 with extension channel below */
#define IEEE80211_CHAN_EHT80             (IEEE80211_CHAN_EHTCAP | IEEE80211_CHAN_80MHZ)    /* EHT 80 channel */
#define IEEE80211_CHAN_EHT160            (IEEE80211_CHAN_EHTCAP | IEEE80211_CHAN_160MHZ)   /* EHT 160 channel */
#define IEEE80211_CHAN_EHT320            (IEEE80211_CHAN_EHTCAP | IEEE80211_CHAN_320MHZ)   /* EHT 320 channel */

#define IEEE80211_IS_CHAN_BW_EHT20(_c)           IEEE80211_IS_FLAG_EHT20((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_EHT40PLUS(_c)       IEEE80211_IS_FLAG_EHT40PLUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_EHT40MINUS(_c)      IEEE80211_IS_FLAG_EHT40MINUS((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_EHT40(_c)           IEEE80211_IS_FLAG_EHT40((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_EHT80(_c)           IEEE80211_IS_FLAG_EHT80((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_EHT160(_c)          IEEE80211_IS_FLAG_EHT160((_c)->ic_flags)
#define IEEE80211_IS_CHAN_BW_EHT320(_c)          IEEE80211_IS_FLAG_EHT320((_c)->ic_flags)


#define IEEE80211_IS_CHAN_BW_EHT(_c)             IEEE80211_IS_FLAG_EHT((_c)->ic_flags)

#define IEEE80211_IS_FLAG_EHT20(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_EHT20)
#define IEEE80211_IS_FLAG_EHT40PLUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_EHT40PLUS)
#define IEEE80211_IS_FLAG_EHT40MINUS(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_EHT40MINUS)
#define IEEE80211_IS_FLAG_EHT40(_flag) \
   ( IEEE80211_IS_FLAG_EHT40PLUS(_flag) || IEEE80211_IS_FLAG_EHT40MINUS(_flag) )
#define IEEE80211_IS_FLAG_EHT80(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_EHT80)
#define IEEE80211_IS_FLAG_EHT160(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_EHT160)
#define IEEE80211_IS_FLAG_EHT320(_flag) \
    (((_flag) & IEEE80211_CHAN_BW_MASK) == IEEE80211_CHAN_EHT320)

#define IEEE80211_IS_FLAG_320(_flag) \
    (((_flag) & IEEE80211_CHAN_WIDTH_MASK) == IEEE80211_CHAN_320MHZ)

#define IEEE80211_IS_FLAG_EHT(_flag) \
    (((_flag) & IEEE80211_CHAN_CAP_MASK) == IEEE80211_CHAN_EHTCAP)

#define IEEE80211_IS_CHAN_11BE(_c) \
    ((IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) || IEEE80211_IS_CHAN_2GHZ(_c)) && \
     IEEE80211_IS_CHAN_EHT(_c))

#define IEEE80211_IS_CHAN_11BEG_EHT20(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT20(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT20(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT20(_c))
#define IEEE80211_IS_CHAN_11BEG_EHT40PLUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT40PLUS(_c))
#define IEEE80211_IS_CHAN_11BEG_EHT40MINUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT40MINUS(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT40PLUS(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT40MINUS(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT80(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT80(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT160(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT160(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT320(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT320(_c))
#define IEEE80211_IS_CHAN_5GHZ_EHT240(_c) \
    (IEEE80211_IS_CHAN_5GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT320(_c))

#define IEEE80211_IS_CHAN_11BE_EHT20(_c) \
    (IEEE80211_IS_CHAN_BW_EHT20(_c))
#define IEEE80211_IS_CHAN_11BE_EHT40(_c) \
    (IEEE80211_IS_CHAN_BW_EHT40(_c))

#define IEEE80211_IS_CHAN_11BEG(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_11BE(_c))
#define IEEE80211_IS_CHAN_11BEA(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_11BE(_c))
#define IEEE80211_IS_CHAN_11BE_EHT40PLUS(_c) \
    (IEEE80211_IS_CHAN_BW_EHT40PLUS(_c))
#define IEEE80211_IS_CHAN_11BE_EHT40MINUS(_c) \
    (IEEE80211_IS_CHAN_BW_EHT40MINUS(_c))
#define IEEE80211_IS_CHAN_11BEG_EHT40(_c) \
    (IEEE80211_IS_CHAN_2GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT40(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT40(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) && IEEE80211_IS_CHAN_BW_EHT40(_c))
#define IEEE80211_IS_CHAN_11BE_EHT80(_c) \
    (IEEE80211_IS_CHAN_BW_EHT80(_c))
#define IEEE80211_IS_CHAN_11BE_EHT160(_c) \
    (IEEE80211_IS_CHAN_BW_EHT160(_c))
#define IEEE80211_IS_CHAN_11BE_EHT320(_c) \
    (IEEE80211_IS_CHAN_BW_EHT320(_c))

#define IEEE80211_IS_CHAN_EHT(_c)  IEEE80211_IS_CHAN_BW_EHT(_c)
#define IEEE80211_IS_CHAN_BW_320MHZ(_c) IEEE80211_IS_FLAG_320((_c)->ic_flags)
#define IEEE80211_IS_CHAN_320MHZ(_c) IEEE80211_IS_CHAN_BW_320MHZ(_c)

/* check if we can remove this */
#define IEEE80211_IS_CHAN_EHT20_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_EHT20(_c))
#define IEEE80211_IS_CHAN_EHT40PLUS_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_EHT40PLUS(_c))
#define IEEE80211_IS_CHAN_EHT40MINUS_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_EHT40MINUS(_c))

#define IEEE80211_IS_CHAN_EHT40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_EHT40PLUS_CAPABLE(_c) || \
     IEEE80211_IS_CHAN_EHT40MINUS_CAPABLE(_c))
#define IEEE80211_IS_CHAN_EHT80_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_EHT80(_c))
#define IEEE80211_IS_CHAN_EHT160_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_EHT160(_c))
#define IEEE80211_IS_CHAN_EHT320_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_EHT320(_c))
#define IEEE80211_IS_CHAN_EHT_CAPABLE(_c) IEEE80211_IS_CHAN_EHT(_c)

#define IEEE80211_IS_CHAN_11AX_SUPPORTED(_c) (IEEE80211_IS_CHAN_11AX(_c) || IEEE80211_IS_CHAN_11BE(_c))
#else

#define IEEE80211_IS_CHAN_11AX_SUPPORTED(_c) (IEEE80211_IS_CHAN_11AX(_c))
#define IEEE80211_IS_CHAN_11BEA_EHT160(_c)   false
#define IEEE80211_IS_CHAN_11BE(_c) false

#endif /* WLAN_FEATURE_11BE */

/* The below are based on equivalent 11n definitions.
 * 11AX TODO (Phase II) - Once more details emerge in the 802.11ax
 * specification, in case it turns out the below CTL based definitions are not
 * necessary, then remove.
 */
#define IEEE80211_IS_CHAN_11AX_CTL_CAPABLE(_c)  \
    IEEE80211_IS_CHAN_HE20_CAPABLE(_c)
#define IEEE80211_IS_CHAN_11AX_CTL_U_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE40PLUS(_c))
#define IEEE80211_IS_CHAN_11AX_CTL_L_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_BW_HE40MINUS(_c))
#define IEEE80211_IS_CHAN_11AX_CTL_40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_11AX_CTL_U_CAPABLE(_c) || \
     IEEE80211_IS_CHAN_11AX_CTL_L_CAPABLE(_c))

#define IEEE80211_CH_HOPPING_SET_CHAN_BLOCKED(_c)    \
    ((_c)->ic_flags |= IEEE80211_CHAN_BLOCKED)
#define IEEE80211_CH_HOPPING_IS_CHAN_BLOCKED(_c)    \
    (((_c)->ic_flags & IEEE80211_CHAN_BLOCKED) == IEEE80211_CHAN_BLOCKED)
#define IEEE80211_CH_HOPPING_CLEAR_CHAN_BLOCKED(_c)    \
    ((_c)->ic_flags &= ~IEEE80211_CHAN_BLOCKED)
#define IEEE80211_IS_CHAN_RADAR ieee80211_is_chan_radar

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_IS_CHAN_RADAR_AND_RADARCHAN_NOT_PUNC(_ic, _chan, _punc) \
    (ieee80211_is_chan_radar(_ic, _chan) && \
     !(ieee80211_is_user_punc_same_as_radar_bitmap(_ic, _chan, _punc)))
#endif

#define IEEE80211_CHAN_SET_RADAR(_c)    \
    ((_c)->ic_flags |= IEEE80211_CHAN_DFS_RADAR)
#define IEEE80211_CHAN_CLR_RADAR(_c)    \
    ((_c)->ic_flags &= ~IEEE80211_CHAN_DFS_RADAR)
#define IEEE80211_CHAN_SET_DISALLOW_ADHOC(_c)   \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC)
#define IEEE80211_CHAN_SET_DISALLOW_HOSTAP(_c)   \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DISALLOW_HOSTAP)
#define IEEE80211_CHAN_SET_DFS(_c)  \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DFS)
#define IEEE80211_CHAN_SET_DFS_CLEAR(_c)  \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DFS_CLEAR)
#define IEEE80211_CHAN_EXCLUDE_11D(_c)  \
    ((_c)->ic_flagext |= IEEE80211_CHAN_11D_EXCLUDED)

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
#define IEEE80211_IS_CHAN_HISTORY_RADAR ieee80211_is_chan_nol_history
#define IEEE80211_CHAN_SET_HISTORY_RADAR(_c)    \
    ((_c)->ic_flagext |= IEEE80211_CHAN_HISTORY_RADAR)
#define IEEE80211_CHAN_CLR_HISTORY_RADAR(_c)    \
    ((_c)->ic_flagext &= ~IEEE80211_CHAN_HISTORY_RADAR)

#define IEEE80211_IS_CHAN_CAC_VALID(_c)    \
    (((_c)->ic_flagext & IEEE80211_CHAN_CAC_VALID) == IEEE80211_CHAN_CAC_VALID)
#define IEEE80211_CHAN_SET_CAC_VALID(_c)    \
    ((_c)->ic_flagext |= IEEE80211_CHAN_CAC_VALID)
#define IEEE80211_CHAN_CLR_CAC_VALID(_c)    \
    ((_c)->ic_flagext &= ~IEEE80211_CHAN_CAC_VALID)
#endif

#define IEEE80211_SET_CHAN_40INTOL ieee80211_set_chan_40intol
#define IEEE80211_CLEAR_CHAN_40INTOL ieee80211_clear_chan_40intol
#define IEEE80211_IS_CHAN_40INTOL ieee80211_is_chan_40intol

/* channel encoding for FH phy */
#define IEEE80211_FH_CHANMOD            80
#define IEEE80211_FH_CHAN(set,pat)      (((set)-1)*IEEE80211_FH_CHANMOD+(pat))
#define IEEE80211_FH_CHANSET(chan)      ((chan)/IEEE80211_FH_CHANMOD+1)
#define IEEE80211_FH_CHANPAT(chan)      ((chan)%IEEE80211_FH_CHANMOD)

/*
 * 802.11 rate set.
 */
#define IEEE80211_RATE_SIZE     8               /* 802.11 standard */
#define IEEE80211_RATE_MAXSIZE  44              /* max rates we'll handle */
#define IEEE80211_HT_RATE_SIZE  128
#define IEEE80211_RATE_SINGLE_STREAM_MCS_MAX     7  /* MCS7 */

#define IEEE80211_RATE_MCS      0x8000
#define IEEE80211_RATE_MCS_VAL  0x7FFF

#define IEEE80211_RATE_IDX_ENTRY(val, idx) (((val&(0xff<<(idx*8)))>>(idx*8)))

/*
 * RSSI range
 */
#define IEEE80211_RSSI_MAX           -10   /* in db */
#define IEEE80211_RSSI_MIN           -200

/*
 * 11n A-MPDU & A-MSDU limits
 */
#define IEEE80211_AMPDU_LIMIT_MIN           (1 * 1024)
#define IEEE80211_AMPDU_LIMIT_MAX           (64 * 1024 - 1)
#define IEEE80211_AMPDU_LIMIT_DEFAULT       IEEE80211_AMPDU_LIMIT_MAX
#define IEEE80211_AMPDU_SUBFRAME_MIN        2
#define IEEE80211_AMPDU_SUBFRAME_MAX        64
#define IEEE80211_AMPDU_SUBFRAME_DEFAULT    32
#define IEEE80211_AMSDU_LIMIT_MAX           4096
#define IEEE80211_RIFS_AGGR_DIV             10
#define IEEE80211_MAX_AMPDU_MIN             0
#define IEEE80211_MAX_AMPDU_MAX             3

#define CUSTOM_AGGR_MAX_AMPDU_SIZE 255 /* Maximum Frames for Custom AMPDU Size */
#define CUSTOM_AGGR_MAX_AMSDU_SIZE 7   /* Maximum Frames for Custom AMSDU Size */

/*
 * 11be limits
 */
#define CUSTOM_AGGR_MAX_AMPDU_SIZE_BE 1023

/*
 * 11ac A-MPDU limits
 */
#define IEEE80211_VHT_MAX_AMPDU_MIN         0
#define IEEE80211_VHT_MAX_AMPDU_MAX         7

#define WAPI_MCAST_SUITE_POLICY     BIT(0)
#define WAPI_UCAST_SUITE_POLICY     BIT(1)
#define WAPI_KEYMGT_ALGS_POLICY     (BIT(2)|BIT(3))
#define WAPI_PREAUTH_POLICY         BIT(4)

/*
 * Chipsets for which 80+80 EMI WAR is applicable.
 */
#define IS_EMIWAR_80P80_APPLICABLE(target_type) ((target_type == TARGET_TYPE_QCA9984) || (target_type == TARGET_TYPE_QCA9888))

enum emiwar_80p80_mode {
    EMIWAR_80P80_DISABLE    = 0,    /* disable EMIWAR 80P80 */
    EMIWAR_80P80_BANDEDGE   = 1,    /* Enable EMIWAR for BandEdge(i.e fc1:5775, and fc2:5210) frequency limitation */
    EMIWAR_80P80_FC1GTFC2   = 2,    /* Enable EMIWAR for fc1 > fc2 frequency limitation */
    EMIWAR_80P80_MAX,
};
/* Default value of 80+80 EMI WAR for QCA9984 */
#define EMIWAR_80P80_DEFVAL_QCA9984     (EMIWAR_80P80_DISABLE)
/* Default value of 80+80 EMI WAR for QCA9888 */
#define EMIWAR_80P80_DEFVAL_QCA9888     (EMIWAR_80P80_FC1GTFC2)

/* Max Number of supported class/channel for MBO */
#define MAX_NUM_CHANNEL_SUPPORTED       255
#define MAX_NUM_OPCLASS_SUPPORTED       64

 /*
  * Check if revised signalling is being used for VHT160 in vhtop
  */
#define IS_REVSIG_VHT160(vhtop) (((vhtop)->vht_op_chwidth == IEEE80211_VHTOP_CHWIDTH_REVSIG_160) && \
        ((vhtop)->vht_op_ch_freq_seg2 != 0) && \
        (abs((vhtop)->vht_op_ch_freq_seg2 - (vhtop)->vht_op_ch_freq_seg1) == 8))

/*
 * Check if revised signalling is being used for VHT80p80 in vhtop
 */
#define IS_REVSIG_VHT80_80(vhtop) (((vhtop)->vht_op_chwidth == IEEE80211_VHTOP_CHWIDTH_REVSIG_80_80) && \
        ((vhtop)->vht_op_ch_freq_seg2 != 0) && \
        (abs((vhtop)->vht_op_ch_freq_seg2 - (vhtop)->vht_op_ch_freq_seg1) > 16))

#define HTINFO_CCFS2_GET(htinfo) ((((htinfo)->hi_ccfs2_2)  << IEEE80211_HTINFO_CCFS2_GET_S) | (htinfo)->hi_ccfs2_1)
#define HTINFO_CCFS2_SET(x, htinfo) (htinfo)->hi_ccfs2_1 = ((x) & 0x7); (htinfo)->hi_ccfs2_2 = (((x) >> IEEE80211_HTINFO_CCFS2_SET_S) & 0x1F)

struct ieee80211_rateset {
    u_int8_t                rs_nrates;
    u_int8_t                rs_rates[IEEE80211_RATE_MAXSIZE];
};

/**
 * struct ni_chan_param: Channel Structure of ni.
 * @ni_freq: Primary operating channel frequency in MHZ.
 * @ni_cfreq1: Center frequency of segment 0 in MHZ.
 * @ni_cfreq2: Center frequency of segment 1 in MHZ.
 * @ni_bss_phymode: Phymode of the operating channel.
 * @puncture_bitmap: Puncture bitmap of the channel
 */
struct ni_chan_param {
    uint16_t ni_freq;
    uint16_t ni_cfreq1;
    uint16_t ni_cfreq2;
#ifdef WLAN_FEATURE_11BE
    uint16_t puncture_bitmap;
#endif
    enum ieee80211_phymode ni_bss_phymode;
};

#if QCN_IE
struct qcn_240_mhz_param {
    uint8_t ch_num_seg1;
    uint8_t ch_num_seg2;
#ifdef WLAN_FEATURE_11BE
    uint16_t puncture_bitmap;
#endif
    u_int16_t   is_240mhzin5ghz              :1,
                ehtcap_bfmess320mhz          :3,
                ehtcap_numsound320mhz        :3,
                ehtcap_nonofdmaulmumimo320mhz:1,
                ehtcap_mubfmr320mhz          :1;
    u_int32_t   ehtcap_mcs_map_320_mhz;
};
#endif

#define IEEE80211_MBO_CHAN_BITMAP_WORD_SIZE  (32) /* needs to be a power of 2 */
#define IEEE80211_MBO_CHAN_BITMAP_LOG2_WORD_SIZE 5 /* log2(32) == 5 */
#define IEEE80211_MBO_CHAN_BITMAP_WORD_MASK (IEEE80211_MBO_CHAN_BITMAP_WORD_SIZE-1)
/* IEEE80211_MBO_CHAN_BITMAP_SET, IEEE80211_MBO_CHAN_BITMAP_CLR -
 * Find the word by dividing the index by the number of bits per word.
 * Find the bit within the word by masking out all but the LSBs of the index.
 * Set or clear the relevant bit within the relevant word.
 */
#define IEEE80211_MBO_CHAN_BITMAP_SET(bitmap, chan)           \
    (bitmap[chan >> IEEE80211_MBO_CHAN_BITMAP_LOG2_WORD_SIZE] |= \
    (1 << (chan & IEEE80211_MBO_CHAN_BITMAP_WORD_MASK)))
#define IEEE80211_MBO_CHAN_BITMAP_CLR(bitmap, chan)              \
    (bitmap[chan >> IEEE80211_MBO_CHAN_BITMAP_LOG2_WORD_SIZE] &= \
    ~(1 << (chan & IEEE80211_MBO_CHAN_BITMAP_WORD_MASK)))
#define IEEE80211_MBO_CHAN_BITMAP_IS_SET(bitmap, chan)    \
    ((bitmap[chan >> IEEE80211_MBO_CHAN_BITMAP_LOG2_WORD_SIZE] & \
    (1 << (chan & IEEE80211_MBO_CHAN_BITMAP_WORD_MASK))) != 0)
/*
 * We expect IEEE80211_MBO_CHAN_WORDS to be a multiple of IEEE80211_MBO_CHAN_BITMAP_WORD_SIZE,
 * but just in case it isn't, round up.
 */
#define IEEE80211_MBO_CHAN_WORDS \
    ((IEEE80211_CHAN_MAX + IEEE80211_MBO_CHAN_BITMAP_WORD_SIZE - 1) / IEEE80211_MBO_CHAN_BITMAP_WORD_SIZE)

/* MBO :- supported class */
struct ieee80211_supp_op_class {
    u_int8_t               curr_op_class;
    u_int8_t               num_of_supp_class;
    u_int8_t               supp_class[MAX_NUM_OPCLASS_SUPPORTED];
    u_int8_t               num_chan_supported;
    u_int8_t               bands_supported;
    u_int32_t              channels_supported[IEEE80211_MBO_CHAN_WORDS];
};
struct ieee80211_beacon_info{
    u_int8_t    essid[IEEE80211_NWID_LEN+1];
    u_int8_t    esslen;
    u_int8_t    snr_ctl_0;
    u_int8_t    snr_ctl_1;
    u_int8_t    snr_ctl_2;
    int         numchains;
};

struct ieee80211_ibss_peer_list{
    u_int8_t    bssid[6];
};

struct ieee80211_roam {
    int8_t                  rssi11a;        /* rssi thresh for 11a bss */
    int8_t                  rssi11b;        /* for 11g sta in 11b bss */
    int8_t                  rssi11bOnly;    /* for 11b sta */
    u_int8_t                pad1;
    u_int8_t                rate11a;        /* rate thresh for 11a bss */
    u_int8_t                rate11b;        /* for 11g sta in 11b bss */
    u_int8_t                rate11bOnly;    /* for 11b sta */
    u_int8_t                pad2;
};

#ifdef WLAN_FEATURE_11BE_MLO
enum eid_cu_param_list {
    CU_IEEE80211_ELEMID_EXT_EHTOP = 0,
    CU_IEEE80211_ELEMID_EXT_HEOP,
    CU_IEEE80211_ELEMID_VHTOP,
    CU_IEEE80211_ELEMID_HTINFO_ANA,
    CU_IEEE80211_ELEMID_EXT_BSSCOLOR_CHG,
    CU_IEEE80211_ELEMID_EXT_UORA_PARAM,
    CU_IEEE80211_ELEMID_VHT_PSD_DEFAULT_TPE,
    CU_IEEE80211_ELEMID_VHT_PSD_SUB_ORDINATE_TPE,
    CU_IEEE80211_ELEMID_VHT_EIRP_DEFAULT_TPE,
    CU_IEEE80211_ELEMID_VHT_ADDITIONAL_PSD_DEFAULT_TPE,
    CU_IEEE80211_ELEMID_VHT_ADDITIONAL_EIRP_DEFAULT_TPE,

    CU_EID_MAX,
};

#define CU_MAX_ELEM_LENGTH 64
struct eid_cu_param {
    int8_t      length;
    u_int32_t   elem_hash;
};
#endif

#define IEEE80211_TID_SIZE      17 /* total number of TIDs */
#define IEEE80211_NON_QOS_SEQ   16 /* index for non-QoS (including management) sequence number space */
#define IEEE80211_SEQ_MASK      0xfff   /* sequence generator mask*/
#define MIN_SW_SEQ              0x100   /* minimum sequence for SW generate packect*/

#define IEEE80211_ADDR_LEN  6       /* size of 802.11 address */
#define IEEE80211_SHORT_SSID_LEN 4   /* size of shortssid length */

/* crypto related defines*/
#define IEEE80211_KEYBUF_SIZE   32
#define IEEE80211_MICBUF_SIZE   (8+8)   /* space for both tx+rx keys */
#define IEEE80211_KEY_WEP40_LEN  5    /* 40 bit key */
#define IEEE80211_KEY_WEP104_LEN  13  /* 104 bit key */
#define IEEE80211_KEY_WEP128_LEN  16  /* 128 bit key */

enum ieee80211_clist_cmd {
    CLIST_UPDATE,
    CLIST_DFS_UPDATE,
    CLIST_NEW_COUNTRY,
    CLIST_NOL_UPDATE
};

enum ieee80211_nawds_param {
    IEEE80211_NAWDS_PARAM_NUM = 0,
    IEEE80211_NAWDS_PARAM_MODE,
    IEEE80211_NAWDS_PARAM_DEFCAPS,
    IEEE80211_NAWDS_PARAM_OVERRIDE,
#ifdef WLAN_FEATURE_11BE
    IEEE80211_NAWDS_PARAM_DEF_PUNCTURE_BITMAP,
#endif
#ifdef UMAC_SUPPORT_MLNAWDS
    IEEE80211_MLNAWDS_PARAM_NUM,
    IEEE80211_MLNAWDS_PARAM_MODE,
    IEEE80211_MLNAWDS_PARAM_OVERRIDE,
#endif
};

struct ieee80211_mib_cycle_cnts {
    u_int32_t   tx_frame_count;
    u_int32_t   rx_frame_count;
    u_int32_t   rx_clear_count;
    u_int32_t   rx_clear_ext_count;
    u_int32_t   rx_clear_ext40_count;
    u_int32_t   rx_clear_ext80_count;
    u_int32_t   cycle_count;
    u_int32_t   my_bss_rx_cycle_count;
    u_int8_t    is_rx_active;
    u_int8_t    is_tx_active;
};

struct ieee80211_chanutil_info {
    u_int32_t    rx_clear_count;
    u_int32_t    cycle_count;
    u_int32_t    beacon_count;
    u_int8_t     value;
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    // Flag indicating if the util value is new since the last read
    u_int8_t     new_value;
#endif
};

enum ieee80211_vendor_ie_param {
    IEEE80211_VENDOR_IE_PARAM_ADD = 0,
    IEEE80211_VENDOR_IE_PARAM_UPDATE,
    IEEE80211_VENDOR_IE_PARAM_REMOVE,
    IEEE80211_VENDOR_IE_PARAM_LIST,
};

enum ieee80211_nac_param {
    IEEE80211_NAC_PARAM_ADD = 1,
    IEEE80211_NAC_PARAM_DEL ,
    IEEE80211_NAC_PARAM_LIST ,
};
#if ATH_SUPPORT_NAC_RSSI
enum ieee80211_nac_rssi_param {
    IEEE80211_NAC_RSSI_PARAM_ADD = 1,
    IEEE80211_NAC_RSSI_PARAM_DEL ,
    IEEE80211_NAC_RSSI_PARAM_LIST ,
};
#endif
static __inline int
ieee80211_check_weather_radar_channel(struct ieee80211_ath_channel *chan)
{
    if (!(IEEE80211_IS_CHAN_5GHZ(chan))) {
        return 0;
    }

    switch ((chan->ic_flags) & IEEE80211_CHAN_BW_MASK)
    {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CHAN_EHT320:
            return ((chan->ic_freq >= 5500) && (chan->ic_freq <= 5730));
#endif /* WLAN_FEATURE_11BE */
        case IEEE80211_CHAN_HT40PLUS:
        case IEEE80211_CHAN_VHT40PLUS:
        case IEEE80211_CHAN_HT40MINUS:
        case IEEE80211_CHAN_VHT40MINUS:
        case IEEE80211_CHAN_VHT80:
        case IEEE80211_CHAN_VHT80_80:
        case IEEE80211_CHAN_HE40PLUS:
        case IEEE80211_CHAN_HE40MINUS:
        case IEEE80211_CHAN_HE80:
        case IEEE80211_CHAN_HE80_80:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CHAN_EHT40PLUS:
        case IEEE80211_CHAN_EHT40MINUS:
        case IEEE80211_CHAN_EHT80:
#endif /* WLAN_FEATURE_11BE */
            return ((chan->ic_freq >= 5580) && (chan->ic_freq <= 5650));

        case IEEE80211_CHAN_VHT160:
        case IEEE80211_CHAN_HE160:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CHAN_EHT160:
#endif /* WLAN_FEATURE_11BE */
            return ((chan->ic_freq >= 5500) && (chan->ic_freq <= 5650));

        case IEEE80211_CHAN_HT20:
        case IEEE80211_CHAN_VHT20:
        case IEEE80211_CHAN_HE20:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CHAN_EHT20:
#endif /* WLAN_FEATURE_11BE */
        default: /* neither HT40+ nor HT40-, finish this call */
            return ((chan->ic_freq >= 5600) && (chan->ic_freq <= 5650));
    }

    return 0;

}

typedef struct cfg80211_hostapd_acs_params_t {
    /* Selected mode (HOSTAPD_MODE_*) */
    enum ieee80211_phymode  hw_mode;

    /* Indicates whether HT, HT40, VHT is enabled */
    u_int16_t    ht_enabled:1,
                ht40_enabled:1,
                vht_enabled:1;

    /* Configured ACS channel width */
    u_int16_t ch_width;

    /* ACS channel list info */
    u_int16_t ch_list_len;
    const u_int8_t *ch_list;
    const uint32_t *freq_list;
    u_int8_t link_id; /* LinkID */
}cfg80211_hostapd_acs_params;

#define IEEE80211_SUPPORT_BTM_QUERY          1

/**
 * enum virtual_interface_type: applicable types of virtual interfaces
 * @VIF_TYPE_INVALID: Invalid VAP
 * @VIF_TYPE_LEGACY: Non-11BE VAP
 * @VIF_TYPE_MLD_WITH_MAC: MLD Device with MAC
 * @VIF_TYPE_MLD_WITH_OUT_MAC: MLD Device with Zero MAC
 * @VIF_TYPE_MLD_LINK_VAP: Link VAP of MLD
 * @VIF_TYPE_MLD_LEGACY_VAP: Legacy VAP on MLD
 */
enum virtual_interface_type {
    VIF_TYPE_INVALID,
    VIF_TYPE_LEGACY,
    VIF_TYPE_MLD_WITH_MAC,
    VIF_TYPE_MLD_WITH_OUT_MAC,
    VIF_TYPE_MLD_LINK_VAP,
    VIF_TYPE_MLD_LEGACY_VAP,
};

#endif /* __NET80211__IEEE80211_H_ */
