/*
 * Copyright (c) 2011-2014,2017-2020 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * copyright (c) 2011 Atheros Communications Inc.
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

#include <ol_if_utility.h>

#define GI_0_8_MAX_OFFSET 96

QDF_STATUS ol_ath_get_phymode(struct wlan_objmgr_vdev *vdev,
                                     struct ieee80211_ath_channel *chan)
{
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;
    struct ol_ath_softc_net80211 *scn = NULL;
    struct vdev_mlme_obj *vdev_mlme = NULL;
    int chan_mode;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is null");
        return QDF_STATUS_E_FAILURE;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is null");
        return QDF_STATUS_E_FAILURE;
    }

    vdev_mlme = vap->vdev_mlme;
    if (!vdev_mlme) {
        qdf_err("vdev mlme is null");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is null");
        return QDF_STATUS_E_FAILURE;
    }

    chan_mode = ieee80211_chan2mode(chan);
    vdev_mlme->mgmt.generic.phy_mode =
                 ol_get_phymode_info(scn, chan_mode, vap->iv_256qam);

    return QDF_STATUS_SUCCESS;
}

/*
 * legacy rate table for the MCAST/BCAST rate. This table is specific to peregrine
 * chip, so its implemented here in the ol layer instead of the ieee layer.

 * This table is created according to the discription mentioned in the
 * wmi_unified.h file.

 * Here the left hand side specify the rate and the right hand side specify the
 * respective values which the target understands.
 */

static const int legacy_11b_rate_ol[][2] = {
    {1000, 0x083},
    {2000, 0x082},
    {5500, 0x081},
    {11000, 0x080},
};

static const int legacy_11a_rate_ol[][2] = {
    {6000, 0x003},
    {9000, 0x007},
    {12000, 0x002},
    {18000, 0x006},
    {24000, 0x001},
    {36000, 0x005},
    {48000, 0x000},
    {54000, 0x004},
};

static const int legacy_11bg_rate_ol[][2] = {
    {1000, 0x083},
    {2000, 0x082},
    {5500, 0x081},
    {6000, 0x003},
    {9000, 0x007},
    {11000, 0x080},
    {12000, 0x002},
    {18000, 0x006},
    {24000, 0x001},
    {36000, 0x005},
    {48000, 0x000},
    {54000, 0x004},
};

static const int ht20_11n_rate_ol[][2] = {
    {6500,  0x100},
    {13000, 0x101},
    {19500, 0x102},
    {26000, 0x103},
    {39000, 0x104},
    {52000, 0x105},
    {58500, 0x106},
    {65000, 0x107},

    {13000,  0x110},
    {26000,  0x111},
    {39000,  0x112},
    {52000,  0x113},
    {78000,  0x114},
    {104000, 0x115},
    {117000, 0x116},
    {130000, 0x117},

    {19500,  0x120},
    {39000,  0x121},
    {58500,  0x122},
    {78000,  0x123},
    {117000, 0x124},
    {156000, 0x125},
    {175500, 0x126},
    {195000, 0x127},

    {26000,  0x130},
    {52000,  0x131},
    {78000,  0x132},
    {104000, 0x133},
    {156000, 0x134},
    {208000, 0x135},
    {234000, 0x136},
    {260000, 0x137},
};

static const int ht20_11ac_rate_ol[][2] = {
/* VHT MCS0-9 NSS 1 20 MHz */
    { 6500, 0x180},
    {13000, 0x181},
    {19500, 0x182},
    {26000, 0x183},
    {39000, 0x184},
    {52000, 0x185},
    {58500, 0x186},
    {65000, 0x187},
    {78000, 0x188},
    {86500, 0x189},

/* VHT MCS0-9 NSS 2 20 MHz */
    { 13000, 0x190},
    { 26000, 0x191},
    { 39000, 0x192},
    { 52000, 0x193},
    { 78000, 0x194},
    {104000, 0x195},
    {117000, 0x196},
    {130000, 0x197},
    {156000, 0x198},
    {173000, 0x199},

 /* VHT MCS0-9 NSS 3 20 MHz */
    { 19500, 0x1a0},
    { 39000, 0x1a1},
    { 58500, 0x1a2},
    { 78000, 0x1a3},
    {117000, 0x1a4},
    {156000, 0x1a5},
    {175500, 0x1a6},
    {195000, 0x1a7},
    {234000, 0x1a8},
    {260000, 0x1a9},

 /* VHT MCS0-9 NSS 4 20 MHz */
    { 26000, 0x1b0},
    { 52000, 0x1b1},
    { 78000, 0x1b2},
    {104000, 0x1b3},
    {156000, 0x1b4},
    {208000, 0x1b5},
    {234000, 0x1b6},
    {260000, 0x1b7},
    {312000, 0x1b8},
    {344000, 0x1b9},
};

static const int ht20_11ax_rate_ol[][2] = {
/* HE rates for 0.8us GI */

/* HE MCS0-11 NSS 1 20 MHz */
      {8600, 0x200},
     {17200, 0x201},
     {25800, 0x202},
     {34400, 0x203},
     {51600, 0x204},
     {68800, 0x205},
     {77400, 0x206},
     {86000, 0x207},
    {103200, 0x208},
    {114700, 0x209},
    {129000, 0x20a},
    {143400, 0x20b},

/* HE MCS0-11 NSS 2 20 MHz */
     {17200, 0x210},
     {34400, 0x211},
     {51600, 0x212},
     {68800, 0x213},
    {103200, 0x214},
    {137600, 0x215},
    {154900, 0x216},
    {172100, 0x217},
    {206500, 0x218},
    {229400, 0x219},
    {258100, 0x21a},
    {286800, 0x21b},

/* HE MCS0-11 NSS 3 20 MHz */
     {25800, 0x220},
     {51600, 0x221},
     {77400, 0x222},
    {103200, 0x223},
    {154900, 0x224},
    {206500, 0x225},
    {232300, 0x226},
    {258100, 0x227},
    {309700, 0x228},
    {344100, 0x229},
    {387100, 0x22a},
    {430100, 0x22b},

/* HE MCS0-11 NSS 4 20 MHz */
     {34400, 0x230},
     {68800, 0x231},
    {103200, 0x232},
    {137600, 0x233},
    {206500, 0x234},
    {275300, 0x235},
    {309700, 0x236},
    {344100, 0x237},
    {412900, 0x238},
    {458800, 0x239},
    {516300, 0x23a},
    {573500, 0x23b},

/* HE MCS0-11 NSS 5 20 MHz */
     {43000, 0x240},
     {86000, 0x241},
    {129000, 0x242},
    {172100, 0x243},
    {258100, 0x244},
    {344100, 0x245},
    {387100, 0x246},
    {430100, 0x247},
    {516200, 0x248},
    {573500, 0x249},
    {645200, 0x24a},
    {716900, 0x24b},

/* HE MCS0-11 NSS 6 20 MHz */
     {51600, 0x250},
    {103200, 0x251},
    {154900, 0x252},
    {206500, 0x253},
    {309700, 0x254},
    {412900, 0x255},
    {464600, 0x256},
    {516200, 0x257},
    {619400, 0x258},
    {688200, 0x259},
    {774300, 0x25a},
    {860300, 0x25b},

/* HE MCS0-11 NSS 7 20 MHz */
     {60200, 0x260},
    {120400, 0x261},
    {180700, 0x262},
    {240900, 0x263},
    {361300, 0x264},
    {481800, 0x265},
    {542000, 0x266},
    {602200, 0x267},
    {722600, 0x268},
    {802900, 0x269},
    {903300, 0x26a},
   {1003700, 0x26b},

/* HE MCS0-11 NSS 8 20 MHz */
     {68800, 0x270},
    {137600, 0x271},
    {206500, 0x272},
    {275300, 0x273},
    {412900, 0x274},
    {550600, 0x275},
    {619400, 0x276},
    {688200, 0x277},
    {825900, 0x278},
    {917600, 0x279},
   {1032400, 0x27a},
   {1147100, 0x27b},

/* HE rates for 3.2us GI */

/* HE MCS0-11 NSS 1 20 MHz */
      {7300, 0x200},
     {14600, 0x201},
     {21900, 0x202},
     {29300, 0x203},
     {43900, 0x204},
     {58500, 0x205},
     {65800, 0x206},
     {73100, 0x207},
     {87800, 0x208},
     {97500, 0x209},
    {109700, 0x20a},
    {121900, 0x20b},

};

#ifdef WLAN_FEATURE_11BE
static const int ht20_11be_rate_ol[][2] = {
/* HE MCS0-11 NSS 1 20 MHz */
      {8600, 0x200},
     {17200, 0x201},
     {25800, 0x202},
     {34400, 0x203},
     {51600, 0x204},
     {68800, 0x205},
     {77400, 0x206},
     {86000, 0x207},
    {103200, 0x208},
    {114700, 0x209},
    {129000, 0x20a},
    {143400, 0x20b},

/* HE MCS0-11 NSS 2 20 MHz */
     {17200, 0x210},
     {34400, 0x211},
     {51600, 0x212},
     {68800, 0x213},
    {103200, 0x214},
    {137600, 0x215},
    {154900, 0x216},
    {172100, 0x217},
    {206500, 0x218},
    {229400, 0x219},
    {258100, 0x21a},
    {286800, 0x21b},

/* HE MCS0-11 NSS 3 20 MHz */
     {25800, 0x220},
     {51600, 0x221},
     {77400, 0x222},
    {103200, 0x223},
    {154900, 0x224},
    {206500, 0x225},
    {232300, 0x226},
    {258100, 0x227},
    {309700, 0x228},
    {344100, 0x229},
    {387100, 0x22a},
    {430100, 0x22b},

/* HE MCS0-11 NSS 4 20 MHz */
     {34400, 0x230},
     {68800, 0x231},
    {103200, 0x232},
    {137600, 0x233},
    {206500, 0x234},
    {275300, 0x235},
    {309700, 0x236},
    {344100, 0x237},
    {412900, 0x238},
    {458800, 0x239},
    {516300, 0x23a},
    {573500, 0x23b},

/* HE MCS0-11 NSS 5 20 MHz */
     {43000, 0x240},
     {86000, 0x241},
    {129000, 0x242},
    {172100, 0x243},
    {258100, 0x244},
    {344100, 0x245},
    {387100, 0x246},
    {430100, 0x247},
    {516200, 0x248},
    {573500, 0x249},
    {645200, 0x24a},
    {716900, 0x24b},

/* HE MCS0-11 NSS 6 20 MHz */
     {51600, 0x250},
    {103200, 0x251},
    {154900, 0x252},
    {206500, 0x253},
    {309700, 0x254},
    {412900, 0x255},
    {464600, 0x256},
    {516200, 0x257},
    {619400, 0x258},
    {688200, 0x259},
    {774300, 0x25a},
    {860300, 0x25b},

/* HE MCS0-11 NSS 7 20 MHz */
     {60200, 0x260},
    {120400, 0x261},
    {180700, 0x262},
    {240900, 0x263},
    {361300, 0x264},
    {481800, 0x265},
    {542000, 0x266},
    {602200, 0x267},
    {722600, 0x268},
    {802900, 0x269},
    {903300, 0x26a},
   {1003700, 0x26b},

/* HE MCS0-11 NSS 8 20 MHz */
     {68800, 0x270},
    {137600, 0x271},
    {206500, 0x272},
    {275300, 0x273},
    {412900, 0x274},
    {550600, 0x275},
    {619400, 0x276},
    {688200, 0x277},
    {825900, 0x278},
    {917600, 0x279},
   {1032400, 0x27a},
   {1147100, 0x27b},
};
#endif /* WLAN_FEATURE_11BE */

#define NUM_RATE_TABS 4
int ol_get_rate_code(struct ieee80211_ath_channel *chan, int val)
{
    uint32_t chan_mode;
    int i = 0, j = 0, found = 0, array_size = 0;
    int *rate_code = NULL;

    struct ol_rate_table {
        int *table;
        int size;
    } rate_table [NUM_RATE_TABS];

    if (!IEEE80211_IS_CHAN_VALID(chan)) {
        qdf_err("Channel is NULL\n");
        return EINVAL;
    }

    OS_MEMZERO(&rate_table[0], sizeof(rate_table));

    chan_mode = ieee80211_chan2mode(chan);

    switch (chan_mode)
    {
        case IEEE80211_MODE_11B:
            {
                /* convert rate to index */
                rate_table[3].size = sizeof(legacy_11b_rate_ol)/sizeof(legacy_11b_rate_ol[0]);
                rate_table[3].table = (int *)&legacy_11b_rate_ol;
            }
            break;

        case IEEE80211_MODE_11G:
        case IEEE80211_MODE_TURBO_G:
            {
                /* convert rate to index */
                rate_table[3].size = sizeof(legacy_11bg_rate_ol)/sizeof(legacy_11bg_rate_ol[0]);
                rate_table[3].table = (int *)&legacy_11bg_rate_ol;
            }
            break;

        case IEEE80211_MODE_11A:
        case IEEE80211_MODE_TURBO_A:
            {
                /* convert rate to index */
                rate_table[3].size = sizeof(legacy_11a_rate_ol)/sizeof(legacy_11a_rate_ol[0]);
                rate_table[3].table = (int *)&legacy_11a_rate_ol;
            }
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEG_EHT20      :
        case IEEE80211_MODE_11BEA_EHT40PLUS  :
        case IEEE80211_MODE_11BEA_EHT20      :
        case IEEE80211_MODE_11BEA_EHT40MINUS :
        case IEEE80211_MODE_11BEG_EHT40PLUS  :
        case IEEE80211_MODE_11BEG_EHT40MINUS :
        case IEEE80211_MODE_11BEA_EHT40      :
        case IEEE80211_MODE_11BEG_EHT40      :
        case IEEE80211_MODE_11BEA_EHT80      :
        case IEEE80211_MODE_11BEA_EHT160     :
        case IEEE80211_MODE_11BEA_EHT320     :
            {
                if (IEEE80211_IS_CHAN_6GHZ(chan)) {
                    rate_table[2].size = sizeof(ht20_11be_rate_ol)/sizeof(ht20_11be_rate_ol[0]);
                    rate_table[2].table = (int *)&ht20_11be_rate_ol;
                } else {
                    rate_table[0].size = sizeof(ht20_11be_rate_ol)/sizeof(ht20_11be_rate_ol[0]);
                    rate_table[0].table = (int *)&ht20_11be_rate_ol;
                }
            }
#endif /* WLAN_FEATURE_11BE */
            /* Fall Through */
            /* In gcc version > 9.0, fall through comment is expected when */
            /* break statement is not used. */
        case IEEE80211_MODE_11AXG_HE20      :
        case IEEE80211_MODE_11AXA_HE40PLUS  :
        case IEEE80211_MODE_11AXA_HE20      :
        case IEEE80211_MODE_11AXA_HE40MINUS :
        case IEEE80211_MODE_11AXG_HE40PLUS  :
        case IEEE80211_MODE_11AXG_HE40MINUS :
        case IEEE80211_MODE_11AXA_HE40      :
        case IEEE80211_MODE_11AXG_HE40      :
        case IEEE80211_MODE_11AXA_HE80      :
        case IEEE80211_MODE_11AXA_HE160     :
        case IEEE80211_MODE_11AXA_HE80_80   :
            {
                if (IEEE80211_IS_CHAN_6GHZ(chan)) {
                /* The table ht20_11ax_rate_ol has 9 sets of HE rates; 0.8 us
                 * MCS 0-11 rates for NSS 1-8 and 3.2us GI MCS 0-11 rates for
                 * NSS1.
                 */
                    rate_table[2].size = sizeof(ht20_11ax_rate_ol)/sizeof(ht20_11ax_rate_ol[0])/9;
                    rate_table[2].table = (int *)&ht20_11ax_rate_ol[GI_0_8_MAX_OFFSET];
                } else {
                    rate_table[0].size = sizeof(ht20_11ax_rate_ol)/sizeof(ht20_11ax_rate_ol[0]);
                    rate_table[0].table = (int *)&ht20_11ax_rate_ol;
                }
            }
            /* Fall Through */
            /* In gcc version > 9.0, fall through comment is expected when */
            /* break statement is not used. */
        case IEEE80211_MODE_11AC_VHT20:
        case IEEE80211_MODE_11AC_VHT40PLUS:
        case IEEE80211_MODE_11AC_VHT40MINUS:
        case IEEE80211_MODE_11AC_VHT40:
        case IEEE80211_MODE_11AC_VHT80:
        case IEEE80211_MODE_11AC_VHT160:
        case IEEE80211_MODE_11AC_VHT80_80:
            {
                if (!IEEE80211_IS_CHAN_6GHZ(chan)) {
                    rate_table[1].size = sizeof(ht20_11ac_rate_ol)/sizeof(ht20_11ac_rate_ol[0]);
                    rate_table[1].table = (int *)&ht20_11ac_rate_ol;
                }
            }
            /* Fall Through */
            /* In gcc version > 9.0, fall through comment is expected when */
            /* break statement is not used. */
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NG_HT40:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NA_HT40:
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
            {
                if (!IEEE80211_IS_CHAN_6GHZ(chan)) {
                    rate_table[2].size = sizeof(ht20_11n_rate_ol)/sizeof(ht20_11n_rate_ol[0]);
                    rate_table[2].table = (int *)&ht20_11n_rate_ol;
                }
            }

            if (IEEE80211_IS_CHAN_5GHZ(chan)) {
                rate_table[3].size = sizeof(legacy_11a_rate_ol)/sizeof(legacy_11a_rate_ol[0]);
                rate_table[3].table = (int *)&legacy_11a_rate_ol;
            } else {
                rate_table[3].size = sizeof(legacy_11bg_rate_ol)/sizeof(legacy_11bg_rate_ol[0]);
                rate_table[3].table = (int *)&legacy_11bg_rate_ol;
            }

            break;

        default:
        {
            qdf_info("Invalid channel mode 0x%x", chan_mode);
            break;
        }
    }

    for (j = NUM_RATE_TABS - 1; ((j >= 0) && !found) && rate_table[j].table; j--) {
        array_size = rate_table[j].size;
        rate_code = rate_table[j].table;
        for (i = 0; i < array_size; i++) {
            /* Array Index 0 has the rate and 1 has the rate code.
             * The variable rate has the rate code which must be converted to actual rate*/
            if (val == *rate_code) {
                val = *(rate_code + 1);
                found = 1;
                break;
            }
            rate_code += 2;
        }
    }

    if(!found) {
        qdf_debug("Rate code not found\n");
        return EINVAL;
    }
    return val;
}

void ol_ath_ema_sanitize_vendor_ie_config(struct ol_ath_softc_net80211 *scn)
{
    uint8_t i;
    uint8_t ven_ie_size_nibble;
    uint16_t vendor_ie_size = 0;
    uint32_t *p_ema_ap_vendor_ie_config;
    uint32_t vendor_ie_size_limit;
    uint32_t max_non_tx_pgmbl_space;
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;

    if (ema_ctx->max_non_tx_sz <=
        ema_ctx->max_non_tx_profile_sz_with_rsn) {
        ema_ctx->vendor_ie_size_limit = 0;
        return;
    }
    max_non_tx_pgmbl_space =
        ema_ctx->max_non_tx_sz -
        ema_ctx->max_non_tx_profile_sz_with_rsn;

    if (ema_ctx->ext_enabled) {
        vendor_ie_size_limit = IEEE80211_MAX_VENDOR_IE_SIZE_LIMIT_EXT_ENABLED;
    } else {
        vendor_ie_size_limit = IEEE80211_MAX_VENDOR_IE_SIZE_LIMIT;
    }

    /* Set allowed vendor-ie limit to driver default */
    if (max_non_tx_pgmbl_space > vendor_ie_size_limit) {
        ema_ctx->vendor_ie_size_limit = vendor_ie_size_limit;
    } else {
        ema_ctx->vendor_ie_size_limit =
                                rounddown_pow_of_two(max_non_tx_pgmbl_space);
    }

    /* Masking out the higher bits and update vendore-ie
     * max-limit from the msb-nibble of vendor_ie_config
     * INI
     */
    if (ema_ctx->num_max_vaps <= IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) {
        /* Mask the higher nibbles only if max_vap reqd is <8 */
        if (ema_ctx->num_max_vaps < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX)
            ema_ctx->vendor_ie_config_low &=
                               ((1 << (ema_ctx->num_max_vaps * 4)) - 1);
        ema_ctx->vendor_ie_config_high = 0;
        vendor_ie_size_limit = IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                               ema_ctx->vendor_ie_config_low,
                               ema_ctx->num_max_vaps - 1);
    } else {
        vendor_ie_size_limit = IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                               ema_ctx->vendor_ie_config_high,
                               IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX - 1);
    }

    /* Update limit if required */
    if (vendor_ie_size_limit < ema_ctx->vendor_ie_size_limit) {
        ema_ctx->vendor_ie_size_limit = vendor_ie_size_limit;
    } else {
        vendor_ie_size_limit = ema_ctx->vendor_ie_size_limit;
    }

    /* Override limit for the case of (ema_ext && WPS) */
    if (ema_ctx->ext_enabled && ema_ctx->support_wps_6ghz) {
        if (max_non_tx_pgmbl_space >
                IEEE80211_EMA_VENDOR_IE_SECTION_BOUND_WITH_WPS) {
            ema_ctx->vendor_ie_size_limit =
                IEEE80211_EMA_VENDOR_IE_SECTION_BOUND_WITH_WPS;
        } else {
            ema_ctx->vendor_ie_size_limit = max_non_tx_pgmbl_space;
        }

    }

    for (i = 0; i < ema_ctx->num_max_vaps; i++) {
        uint8_t _i = i;
        if (i < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) {
            ven_ie_size_nibble =
                ((ema_ctx->vendor_ie_config_low >> (i << 2)) & 0xf);
        } else {
            _i = i - IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX;
            ven_ie_size_nibble =
                ((ema_ctx->vendor_ie_config_high >> (_i << 2)) & 0xf);
        }

        if (ema_ctx->support_wps_6ghz) {
            vendor_ie_size = (1 << ven_ie_size_nibble);
            /* Reserved space exclusively for WPS IE. This
             * will get overidden with
             * IEEE80211_EMA_VENDOR_IE_SECTION_BOUND_WITH_WPS
             * in case of ema-ext enabled
             */
            vendor_ie_size += IEEE80211_EMA_WPS_IE_OCTET_RESERVATION_BOUND;
            vendor_ie_size = qdf_get_pwr2(vendor_ie_size);
            ven_ie_size_nibble = ilog2(vendor_ie_size);
        }

        mbss_info("vendor_ie_size: %d ven_ie_size_nibble: %d",
                vendor_ie_size, ven_ie_size_nibble);

        if (ven_ie_size_nibble > ilog2(vendor_ie_size_limit))
            ven_ie_size_nibble = ilog2(vendor_ie_size_limit);

        if (i < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) {
            p_ema_ap_vendor_ie_config = &ema_ctx->vendor_ie_config_low;
        } else {
            p_ema_ap_vendor_ie_config = &ema_ctx->vendor_ie_config_high;
        }

        *p_ema_ap_vendor_ie_config &= ~(0xf << (_i << 2));
        *p_ema_ap_vendor_ie_config |= (ven_ie_size_nibble & 0xf) << (_i << 2);
    }

    mbss_info("vendor_ie_config_low: 0x%x",
                    ema_ctx->vendor_ie_config_low);
    mbss_info("vendor_ie_config_high: 0x%x",
                    ema_ctx->vendor_ie_config_high);
    mbss_info("vendor_ie_size_limit: %d", ema_ctx->vendor_ie_size_limit);
}

void ol_ath_ema_sanitize_optional_ie_config(struct ol_ath_softc_net80211 *scn)
{
    int i, max_ven_ie_size = 0, ven_ie_size;
    int ema_max_opt_ie_size;
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;

    if (!ema_ctx->max_non_tx_sz) {
        ema_ctx->optional_ie_size = 0;
        return;
    }

    if (ema_ctx->ext_enabled) {
        if (!ema_ctx->support_wps_6ghz) {
            for (i = 0; i < ema_ctx->num_max_vaps - 1; i++) {
                if (i < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) {
                    ven_ie_size = IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                    ema_ctx->vendor_ie_config_low, i);
                } else {
                    ven_ie_size = IEEE80211_EMA_GET_VENDOR_IE_SIZE_FROM_NTX_IDX(
                                    ema_ctx->vendor_ie_config_high,
                                    (i - IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX));
                }
                if (ven_ie_size > max_ven_ie_size) {
                    max_ven_ie_size = ven_ie_size;
                }
            }
        } else {
            max_ven_ie_size = IEEE80211_EMA_VENDOR_IE_SECTION_BOUND_WITH_WPS;
        }

        if (max_ven_ie_size > ema_ctx->vendor_ie_size_limit) {
            max_ven_ie_size = ema_ctx->vendor_ie_size_limit;
        }

        /* Get max optional IE section size for a non-tx vap */
        ema_max_opt_ie_size = ema_ctx->max_non_tx_sz - max_ven_ie_size -
                              ema_ctx->max_non_tx_profile_sz_with_rsn;
        if (ema_max_opt_ie_size < 0) {
            ema_max_opt_ie_size = 0;
        }

        if (ema_ctx->optional_ie_size > ema_max_opt_ie_size) {
            ema_ctx->optional_ie_size = ema_max_opt_ie_size;
        }
        /* set max optional_ie_size_limit */
        ema_ctx->optional_ie_size_limit = ema_ctx->optional_ie_size;
    } else {
        ema_ctx->optional_ie_size = 0;
    }

    mbss_debug("ema_ap_optional_ie_config_size: %d", ema_ctx->optional_ie_size);
    mbss_info("optional_ie_size_limit: %d", ema_ctx->optional_ie_size_limit);
}

static uint16_t ol_ath_ema_get_max_pp_single_group(struct ol_ath_softc_net80211 *scn )
{
	uint8_t i, ven_ie_size_nibble = 0;
	/* by design this should not exceed 256 bytes */
	uint16_t vendor_ie_size = 0;
	uint16_t ema_max_beacon_static_size;
	uint16_t ema_max_beacon_space_for_non_tx_vaps;
	uint16_t total_non_tx_profile_size;
	uint16_t current_non_tx_profile_size = 0;
	ol_ath_ema_pdev_ctx_t *ema_ctx;
	ol_ath_ema_ctx_t *soc_ema_ctx;

	if (!scn) {
		target_if_err("soc is null");
		return QDF_STATUS_E_FAILURE;
	}

	ema_ctx = &scn->ema_ctx;
	soc_ema_ctx = &scn->soc->ema_ctx;

	ema_max_beacon_static_size = ema_ctx->beacon_common_part_size +
					             ema_ctx->rnr_field_size_limit +
					             soc_ema_ctx->mbssie_headroom;
	target_if_debug("ema_max_beacon_static_size: %d",
				ema_max_beacon_static_size);

	if (ema_max_beacon_static_size < soc_ema_ctx->max_mgmt_frm_sz) {
		ema_max_beacon_space_for_non_tx_vaps = soc_ema_ctx->max_mgmt_frm_sz -
                                                    ema_max_beacon_static_size;
	} else {
		target_if_err("Invalid EMA configuration"
				" - ema_max_beacon_static_size >"
				" IEEE80211_MAX_MGMT_SIZE_LIMIT");
		return QDF_STATUS_E_FAILURE;
	}

	target_if_debug("ema_max_beacon_space_for_non_tx_vaps: %d",
				ema_max_beacon_space_for_non_tx_vaps);

	/* initialize to 1 beacon */
	soc_ema_ctx->max_pp = 1;
	total_non_tx_profile_size = 0;

	/* MSB 4 bits in INI config mask belongs to the Tx vap.
	* So, keep it out of the count
	*/
	for (i = 0; i < ema_ctx->num_max_vaps - 1; i++) {
		if (!ema_ctx->support_wps_6ghz || !ema_ctx->ext_enabled) {
			/* 1. shift config bitmask by 4*i bits
			 * 2. mask with 0xf to derive the exponent x
			 * of 2 for i th vap
			 */
			if (i < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) {
				ven_ie_size_nibble =
				((ema_ctx->vendor_ie_config_low >> (i << 2)) & 0xf);
			} else  {
				/* apply same logic as above to the high
				 * 32 bits of the mask */
				ven_ie_size_nibble =
				((ema_ctx->vendor_ie_config_high >>
				 ((i - IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) << 2)) & 0xf);
			} /* if (i < IEEE80211_MBSSID_VENDOR_CFG_LOW_MAX_IDX) */
		}

		/* Initialize it considering that user may
		 * experiment with 0 value of vendor ie size
		 * for this particular position
		 */
		current_non_tx_profile_size =
			ema_ctx->max_non_tx_profile_sz_with_rsn;

		if (ema_ctx->support_wps_6ghz) {
            /* raise 2 to the power of x to get the
             * required vendor ie size for vap i
             */
			while (ven_ie_size_nibble) {
				vendor_ie_size = (1 << ven_ie_size_nibble);
				current_non_tx_profile_size = (vendor_ie_size +
				ema_ctx->max_non_tx_profile_sz_with_rsn);

				if (current_non_tx_profile_size >
					ema_max_beacon_space_for_non_tx_vaps) {
					ven_ie_size_nibble--;
				} else {
					break;
				}
			}
		} else {
			current_non_tx_profile_size = (ema_ctx->vendor_ie_size_limit +
                                ema_ctx->max_non_tx_profile_sz_with_rsn);
		}

		/* If requested Optional IE size is greater than
		 * available non-tx profile space, truncate
		 * so it fits
		 */
		if ((ema_max_beacon_space_for_non_tx_vaps -
		     current_non_tx_profile_size) <
		    ema_ctx->optional_ie_size) {
		    ema_ctx->optional_ie_size =
				    ema_max_beacon_space_for_non_tx_vaps -
				    current_non_tx_profile_size;
		}
		current_non_tx_profile_size += ema_ctx->optional_ie_size;

		if ((ema_max_beacon_space_for_non_tx_vaps - total_non_tx_profile_size) <
				current_non_tx_profile_size) {
			soc_ema_ctx->max_pp++;
			total_non_tx_profile_size = current_non_tx_profile_size;
		} else {
			total_non_tx_profile_size += current_non_tx_profile_size;
		}
	} /* end for */

	/* if odd then icrement by 1) */
	if ((soc_ema_ctx->max_pp > IEEE80211_ALLOWED_MAX_ODD_MAX_PP) &&
		(soc_ema_ctx->max_pp & 1)) {
		/* ema_max_pp algorithm requires max pp to be even
		 * value so that intermediate current PP values can
		 * be determined based on factors of max pp
		 */
		target_if_debug("odd ema_ap_max_pp: %d. move to a even value"
				" as per ema max_pp algorithm requirement",
				soc_ema_ctx->max_pp);
		soc_ema_ctx->max_pp++;
	}

	target_if_info("ema_ap_max_pp: %d", soc_ema_ctx->max_pp);
	return soc_ema_ctx->max_pp;
}

void ol_ath_ema_determine_max_ngroup(struct ol_ath_softc_net80211 *scn)
{
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;
    /* Number of non-Tx vaps in single beacon */
    uint8_t no_of_ntx_vap_in_1_bcn;
    /* Number of non-Tx vaps in PP set of beacons */
    uint8_t no_of_ntx_vap_in_pp_bcns;
    /* Number of total vaps in single group */
    uint8_t no_of_vaps_in_1_grp;
    uint8_t max_bssid_ind;
    ol_ath_ema_ctx_t *soc_ema_ctx = &scn->soc->ema_ctx;

    /* ema_ctx->max_ngroups = f(pp, 656, ema_ctx->max_mgmt_frm_sz,
                                          ema_ctx->num_max_vaps);
                            = f(pp, 656, ema_ctx->max_non_tx_sz);
     */

    qdf_assert(ema_ctx->max_non_tx_sz > ema_ctx->max_non_tx_sz_limit);

    mbss_info("max mgmt frame size :%d",soc_ema_ctx->max_mgmt_frm_sz);
    mbss_info("common_part_sz      :%d",ema_ctx->beacon_common_part_size);
    mbss_info("max_non_tx_sz       :%d",ema_ctx->max_non_tx_sz);
    mbss_info("max_non_tx_sz_limit :%d",ema_ctx->max_non_tx_sz_limit);
    mbss_info("max_pp              :%d",soc_ema_ctx->max_pp);

    if (ema_ctx->max_non_tx_sz > ema_ctx->max_non_tx_sz_limit) {
        no_of_ntx_vap_in_1_bcn   = qdf_do_div(ema_ctx->max_non_tx_sz,
                                   ema_ctx->max_non_tx_sz_limit);
        no_of_ntx_vap_in_pp_bcns = no_of_ntx_vap_in_1_bcn * soc_ema_ctx->max_pp;
        no_of_vaps_in_1_grp      = no_of_ntx_vap_in_pp_bcns + 1;
        max_bssid_ind            = ilog2(no_of_vaps_in_1_grp);
        no_of_vaps_in_1_grp      = 1 << max_bssid_ind;
        /* Max capacity of the group */
        ema_ctx->grp_sz          = no_of_vaps_in_1_grp;
        /* Cut down group size as per ema_ctx->num_max_vaps */
        if (ema_ctx->grp_sz > ema_ctx->num_max_vaps)
            ema_ctx->grp_sz      = ema_ctx->num_max_vaps;

        ema_ctx->max_ngroups     = DIV_ROUND_UP(ema_ctx->num_max_vaps, ema_ctx->grp_sz);

        mbss_info("no_of_ntx_vap_in_1_bcn:   %d", no_of_ntx_vap_in_1_bcn);
        mbss_info("no_of_ntx_vap_in_pp_bcns: %d", no_of_ntx_vap_in_pp_bcns);
        mbss_info("no_of_vaps_in_1_grp:      %d", no_of_vaps_in_1_grp);
    } else {
        ema_ctx->max_ngroups     = ema_ctx->num_max_vaps/2;
        ema_ctx->grp_sz          = 2; /* 1 tx + 1 non-tx */
    }

    mbss_info("max_ngroup:               %d", ema_ctx->max_ngroups);
    mbss_info("grp_sz:                   %d", ema_ctx->grp_sz);
}

void ol_ath_ema_determine_max_pp(struct ol_ath_softc_net80211 *scn)
{
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;
    uint32_t total_spc_reqd_for_all_ntx_vaps;
    ol_ath_ema_ctx_t *soc_ema_ctx = &scn->soc->ema_ctx;

    /* ema_ctx->max_pp = f(n_groups, 656,
     *                     ema_ctx->max_mgmt_frm_sz, ema_ctx->num_max_vaps);
     *                 = f(n_groups, 656, ema_ctx->max_non_tx_sz);
     *
     *  total_space_avl_per_group = pp * ema_ctx->max_non_tx_sz;
     *
     *  total_space_avl = total_space_avl_per_group * ema_ctx->max_ngroups;
     *                  = pp * ema_ctx->max_non_tx_sz * ema_ctx->max_ngroups;
     *
     *  total_space_reqd = (ema_ctx->num_max_vaps - ema_ctx->max_ngroups) * 656;
     *
     *  By, total_space_avl = total_space_reqd
     *  pp = ((ema_ctx->num_max_vaps - ema_ctx->max_ngroups) * 656)/
     *          (ema_ctx->max_non_tx_sz * ema_ctx->max_ngroups);
     */
    if (ema_ctx->max_ngroups == 1) {
         ol_ath_ema_get_max_pp_single_group(scn);
         ema_ctx->grp_sz = ema_ctx->num_max_vaps;
    } else {
        if (ema_ctx->num_max_vaps < ema_ctx->max_ngroups) {
            soc_ema_ctx->max_pp = 1;
        } else {
            total_spc_reqd_for_all_ntx_vaps =
                (ema_ctx->num_max_vaps - ema_ctx->max_ngroups) * 656;
            soc_ema_ctx->max_pp = total_spc_reqd_for_all_ntx_vaps/
                               (ema_ctx->max_non_tx_sz * ema_ctx->max_ngroups);
        }
    }

    mbss_info("max_ngroup: %d", ema_ctx->max_ngroups);
    mbss_info("grp_sz:     %d", ema_ctx->grp_sz);
    mbss_info("max_pp:     %d", soc_ema_ctx->max_pp);
}
