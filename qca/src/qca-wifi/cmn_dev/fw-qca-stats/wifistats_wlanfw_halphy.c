/*
 * Copyright (c) 2020-2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <wifistats.h>
#include <a_types.h>
#include <wmi_unified.h>
#include <wmi_tlv_helper.h>
#include <wmi_tlv_defs.h>

#include <stdlib.h> /* malloc, free */
#include <errno.h>

#define PRIMARY 0
#define SECONDARY 1
#define WMI_HALPHY_STATS_BUFF_SIZE_MAX 2048

/* POWER_LIMIT is the maximum power limit, in 0.25 dBm units */
#define POWER_LIMIT    126

#define HALPHY_STATS_PRINT(level, fmt, ...) \
        do { \
            printf(fmt,##__VA_ARGS__); \
        } while (0)

#define WMI_HALPHY_STATS_BAND_2GHZ 1
#define WMI_HALPHY_STATS_BAND_5GHZ 0
#define WMI_HALPHY_STATS_BAND_6GHZ 2

#define WMI_HALPHY_STATS_GET_BAND(freq) \
    (((freq) > 5920) ? WMI_HALPHY_STATS_BAND_6GHZ : \
        (((freq) > 4800) ? WMI_HALPHY_STATS_BAND_5GHZ : \
            WMI_HALPHY_STATS_BAND_2GHZ))

#define WMI_HALPHY_STATS_RATE_MCS_CCK 4
#define WMI_HALPHY_STATS_RATE_MCS_OFDM 8
#define WMI_HALPHY_STATS_RATE_MCS_HT 8
#define WMI_HALPHY_STATS_RATE_MCS_VHT 12
#define WMI_HALPHY_STATS_RATE_MCS_HE 12
#define WMI_HALPHY_STATS_RATE_EXTRA_MCS_HE 14
#define WMI_HALPHY_STATS_RATE_MCS_EHT 16

#define HALPHY_STATS_RTBL_RATECODE(__pream, __nss, __rate) \
    ((((__pream) & 0x7) << 8) | (((__nss) & 0x7) << 5) | ((__rate) & 0x1F))

typedef enum
{
    WMI_HALPHY_STATS_CCK = 0,
    WMI_HALPHY_STATS_OFDM,
    WMI_HALPHY_STATS_HT20,
    WMI_HALPHY_STATS_HT40,
    WMI_HALPHY_STATS_VHT20,
    WMI_HALPHY_STATS_VHT40,
    WMI_HALPHY_STATS_VHT80,
    WMI_HALPHY_STATS_VHT160,
    WMI_HALPHY_STATS_HE20,
    WMI_HALPHY_STATS_HE40,
    WMI_HALPHY_STATS_HE80,
    WMI_HALPHY_STATS_HE160,
    WMI_HALPHY_STATS_EHT20,
    WMI_HALPHY_STATS_EHT40,
    WMI_HALPHY_STATS_EHT60,
    WMI_HALPHY_STATS_EHT80,
    WMI_HALPHY_STATS_EHT120,
    WMI_HALPHY_STATS_EHT140,
    WMI_HALPHY_STATS_EHT160,
    WMI_HALPHY_STATS_EHT200,
    WMI_HALPHY_STATS_EHT240,
    WMI_HALPHY_STATS_EHT280,
    WMI_HALPHY_STATS_EHT320,
    WMI_HALPHY_STATS_MAX_PREAM
} WMI_HALPHY_STATS_BWS;

typedef enum
{
    WMI_HALPHY_STATS_MODE_OFDM = 0,
    WMI_HALPHY_STATS_MODE_CCK,
    WMI_HALPHY_STATS_MODE_HT,
    WMI_HALPHY_STATS_MODE_VHT,
    WMI_HALPHY_STATS_MODE_HE,
    WMI_HALPHY_STATS_MODE_EHT,
} WMI_HALPHY_STATS_MODES;

typedef enum
{
    WMI_HALPHY_STATS_NSS1 = 1,
    WMI_HALPHY_STATS_NSS2,
    WMI_HALPHY_STATS_NSS3,
    WMI_HALPHY_STATS_NSS4,
    WMI_HALPHY_STATS_NSS5,
    WMI_HALPHY_STATS_NSS6,
    WMI_HALPHY_STATS_NSS7,
    WMI_HALPHY_STATS_NSS8,
} WMI_HALPHY_STATS_SPATIAL_STREAMS;

typedef enum
{
    WMI_HALPHY_STATS_CTL_LEGACY_5G_6G = 0,
    WMI_HALPHY_STATS_CTL_HT_VHT20_5G_6G = 1,
    WMI_HALPHY_STATS_CTL_HE_EHT20_5G_6G = 2,
    WMI_HALPHY_STATS_CTL_HT_VHT40_5G_6G = 3,
    WMI_HALPHY_STATS_CTL_HE_EHT40_5G_6G = 4,
    WMI_HALPHY_STATS_CTL_VHT80_5G_6G = 5,
    WMI_HALPHY_STATS_CTL_HE_EHT80_5G_6G = 6,
    WMI_HALPHY_STATS_CTL_VHT160_5G_6G = 7,
    WMI_HALPHY_STATS_CTL_HE_EHT160_5G_6G = 8,
    WMI_HALPHY_STATS_CTL_HE_EHT320_5G_6G = 9,
    WMI_HALPHY_STATS_CTL_CCK_2G = 10,
    WMI_HALPHY_STATS_CTL_LEGACY_2G = 11,
    WMI_HALPHY_STATS_CTL_HT20_2G = 12,
    WMI_HALPHY_STATS_CTL_HT40_2G = 13,

    WMI_HALPHY_STATS_CTL_EHT80_SU_PUNC20 = 23,
    WMI_HALPHY_STATS_CTL_EHT160_SU_PUNC20,
    WMI_HALPHY_STATS_CTL_EHT320_SU_PUNC40,
    WMI_HALPHY_STATS_CTL_EHT320_SU_PUNC80,
    WMI_HALPHY_STATS_CTL_EHT320_SU_PUNC120,
} WMI_HALPHY_STATS_CTL_MODES_BE;

typedef enum
{
    WMI_HALPHY_STATS_CTL_LEGACY_5G_6G_AX = 0,
    WMI_HALPHY_STATS_CTL_HT_VHT_HE20_5G_6G_AX = 1,
    WMI_HALPHY_STATS_CTL_HT_VHT_HE40_5G_6G_AX = 2,
    WMI_HALPHY_STATS_CTL_VHT_HE80_5G_6G_AX = 3,
} WMI_HALPHY_STATS_CTL_MODES_5G_AX;

typedef enum
{
    WMI_HALPHY_STATS_CTL_CCK_2G_AX = 0,
    WMI_HALPHY_STATS_CTL_LEGACY_2G_AX = 1,
    WMI_HALPHY_STATS_CTL_HT20_2G_AX = 2,
    WMI_HALPHY_STATS_CTL_HT40_2G_AX = 3,
} WMI_HALPHY_STATS_CTL_MODES_2G_AX;

typedef enum {
    WMI_HALPHY_STATS_REG_POW_REGULAR =   0,
    WMI_HALPHY_STATS_REG_POW_MAX_TYPES,
} WMI_HALPHY_STATS_REG_POW_TYPES;

typedef enum {
    WMI_HALPHY_STATS_TGT_POW_ARR_REGULAR =   0,
    WMI_HALPHY_STATS_TGT_POW_ARR2 = 1,
    WMI_HALPHY_STATS_TGT_POW_MAX_TYPES,
} WMI_HALPHY_STATS_TGT_POW_TYPES;

typedef enum {
    WMI_HALPHY_STATS_CTL_POW_REGULAR     =   0,
    WMI_HALPHY_STATS_CTL_POW_160 = 1,
    WMI_HALPHY_STATS_CTL_POW_MAX_TYPES,
} WMI_HALPHY_STATS_CTL_POW_TYPES;

typedef enum {
    WMI_HALPHY_STATSID_PDEV_TPC = 0x1,
    WMI_HALPHY_STATSID_MAX,
} WMI_HALPHY_STATSIDS;

typedef enum {
    WMI_HALPHY_STATS_ACTION_GET = 1,
    WMI_HALPHY_STATS_ACTION_RESET = 2,
} WMI_HALPHY_STATS_ACTION_TYPES;

typedef enum {
    WMI_HALPHY_PDEV_TX_SU_STATS = 0,
    WMI_HALPHY_PDEV_TX_SUTXBF_STATS,
    WMI_HALPHY_PDEV_TX_MU_STATS,
    WMI_HALPHY_PDEV_TX_MUTXBF_STATS,
} WMI_HALPHY_PDEV_TX_STATS_SUBID;

typedef enum {
    WMI_HALPHY_STATS_SUPPORT_160 = 0,
    WMI_HALPHY_STATS_SUPPORT_320,
    WMI_HALPHY_STATS_SUPPORT_AX,
    WMI_HALPHY_STATS_SUPPORT_AX_EXTRA_MCS,
    WMI_HALPHY_STATS_SUPPORT_BE,
    WMI_HALPHY_STATS_SUPPORT_BE_PUNC,
    WMI_HALPHY_TPC_STATS_CTL_DESIGN_1,
} WMI_HALPHY_STATS_SUPPORT_BITFIELDS;

typedef enum
{
    SUCCESS = 0,
    FAILURE = 1,
} STATUS;

/*
 * Provide a forward declaration of halphystats.
 * (The definition is at the bottom of this file.)
 */
static struct wifistats_module halphystats;

void __attribute__ ((constructor)) halphystats_init(void);
void __attribute__ ((destructor)) halphystats_fini(void);

#define WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, str_modes) \
    A_MEMCPY(pream_str, str_modes, wmi_halphy_get_min16( \
        (A_INT16) sizeof(pream_str), (A_INT16) sizeof(str_modes)))
typedef struct
{
    wmi_tpc_configs tpc_cfg;
    wmi_max_reg_power_allowed reg_pow_config;
    A_INT16 *reg_pow;
    wmi_tpc_rates_array rates_config;
    A_INT16 *rates_arr;
    A_INT16 *rates_arr2;
    wmi_tpc_ctl_pwr_table ctl_pow_config;
    int8_t *ctl_pow;
    wmi_tpc_ctl_pwr_table ctl_pow_config160;
    int8_t *ctl_pow160;
} wmi_halphy_tpc_stats_event;

static A_UINT32 limit_len = 0;
static A_UINT32 status = LISTEN_CONTINUE;
static A_UINT16 lg_halphyid = ~0;
static A_UINT16 lg_statsid = ~0;
static A_UINT16 lg_stats_action = ~0;

static A_CHAR pream_str[7] = {0};
static wmi_halphy_tpc_stats_event halphy_tpc_stats;

void halphy_stats_usage(void)
{
    HALPHY_STATS_PRINT(FATAL,
        "\n================= WMI HALPHY STATS USAGE ===================\n");
    HALPHY_STATS_PRINT(FATAL,"\twifistats - Dump the FW stats\n");
    HALPHY_STATS_PRINT(FATAL,
        "\tUSAGE: wifistats wifiX <stats_id> <optional args> "
        "--wmi --halphy_subid <subid>\n");
    HALPHY_STATS_PRINT(FATAL,"\tOPTIONs:\n\t\tstats_id - Type of stats\n");
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t%d - PDEV Tx stats\n", WMI_HALPHY_STATSID_PDEV_TPC);
    HALPHY_STATS_PRINT(FATAL,"\t\toptional args\n");
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t--action - Followed by action to be performed, Default 1\n");
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t\t%d - get stats\n", WMI_HALPHY_STATS_ACTION_GET);
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t\t%d - Reset stats\n", WMI_HALPHY_STATS_ACTION_RESET);
    HALPHY_STATS_PRINT(FATAL,
        "\t\t--wmi --halphy_subid - Display the HALPHY stats using "
        "wmi ctrl path\n");
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t<subid> - HALPHY stats of Requested stats_id\n");
}

void halphy_usage_pdevtx_stats(void)
{
    HALPHY_STATS_PRINT(FATAL,
        "\n================= PDEV TX STATS HALPHY_SUBIDS "
        "===================\n");
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\toption %d - Reset stats are not supported for stats_id %d\n",
        WMI_HALPHY_STATS_ACTION_RESET, WMI_HALPHY_STATSID_PDEV_TPC);
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t%d - SU Powers Display\n", WMI_HALPHY_PDEV_TX_SU_STATS);
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t%d - SU TXBF Powers Display\n", WMI_HALPHY_PDEV_TX_SUTXBF_STATS);
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t%d - MU Powers Display\n", WMI_HALPHY_PDEV_TX_MU_STATS);
    HALPHY_STATS_PRINT(FATAL,
        "\t\t\t%d - MU TXBF Powers Display\n", WMI_HALPHY_PDEV_TX_MUTXBF_STATS);
    HALPHY_STATS_PRINT(FATAL,
        "\n================================"
        "=================================\n");
}

void halphy_stats_help(A_INT32 argc, A_CHAR *argv[])
{
    halphy_stats_usage();
    if (argc >= 3) {
        A_UINT32 stats_id = atoi(argv[2]);
        if (stats_id < WMI_HALPHY_STATSID_PDEV_TPC ||
            stats_id >= WMI_HALPHY_STATSID_MAX)
        {
            return;
        }
        if (stats_id == WMI_HALPHY_STATSID_PDEV_TPC) {
            halphy_usage_pdevtx_stats();
        }
    }
}

void wmi_halphy_tpc_stats_buffer_free(wmi_halphy_tpc_stats_event *free_struct)
{
    free(free_struct->reg_pow);
    free(free_struct->rates_arr);
    free(free_struct->ctl_pow);
    free_struct->reg_pow = NULL;
    free_struct->rates_arr = NULL;
    free_struct->ctl_pow = NULL;
}

A_UINT16 wmi_halphy_get_ratecode(A_UINT16 pream_idx, A_UINT16 nss, A_UINT16 mcs_rate)
{
    A_UINT16 mode_type = ~0;

    /* Below assignments are just for printing purpose only */
    switch (pream_idx) {
        case WMI_HALPHY_STATS_CCK:
            /* pream_idx of CCK in RateTbl is 1 */
            mode_type = WMI_HALPHY_STATS_MODE_CCK;
            break;
        case WMI_HALPHY_STATS_OFDM:
            /* pream_idx of OFDM in RateTbl is 0 */
            mode_type = WMI_HALPHY_STATS_MODE_OFDM;
            break;
        case WMI_HALPHY_STATS_HT20:
        case WMI_HALPHY_STATS_HT40:
            mode_type = WMI_HALPHY_STATS_MODE_HT;
            break;
        case WMI_HALPHY_STATS_VHT20:
        case WMI_HALPHY_STATS_VHT40:
        case WMI_HALPHY_STATS_VHT80:
        case WMI_HALPHY_STATS_VHT160:
            mode_type = WMI_HALPHY_STATS_MODE_VHT;
            break;
        case WMI_HALPHY_STATS_HE20:
        case WMI_HALPHY_STATS_HE40:
        case WMI_HALPHY_STATS_HE80:
        case WMI_HALPHY_STATS_HE160:
            mode_type = WMI_HALPHY_STATS_MODE_HE;
            break;

        case WMI_HALPHY_STATS_EHT20:
        case WMI_HALPHY_STATS_EHT40:
        case WMI_HALPHY_STATS_EHT60:
        case WMI_HALPHY_STATS_EHT80:
        case WMI_HALPHY_STATS_EHT120:
        case WMI_HALPHY_STATS_EHT140:
        case WMI_HALPHY_STATS_EHT160:
        case WMI_HALPHY_STATS_EHT200:
        case WMI_HALPHY_STATS_EHT240:
        case WMI_HALPHY_STATS_EHT280:
        case WMI_HALPHY_STATS_EHT320:
            mode_type = WMI_HALPHY_STATS_MODE_EHT;
            if (mcs_rate == 0 || mcs_rate == 1){
                mcs_rate += 14;
            } else {
                mcs_rate -= 2;
            }
            break;
        default:
            HALPHY_STATS_PRINT(FATAL,
                "%s %d: Out of range pream_idx\n", __func__, __LINE__);
            return mode_type;
    }
    /* This rate code will be aligned with RC in whalGetRateTable */
    return HALPHY_STATS_RTBL_RATECODE(mode_type,nss, mcs_rate);
}

/*
 * Below Func is used to find the min pwr value
 * @a: Input Pow1
 * @b: Input Pow2
 * return: min(a, b)
 */
A_UINT16 wmi_halphy_get_min16(A_INT16 a, A_INT16 b)
{
    A_INT16 min;
    min = a < b ? a : b;
    return min;
}

A_UINT8 get_mode_ctl_pwr(A_UINT8 pream_idx, A_UINT8 band)
{
    A_UINT8 mode_idx = ~0;
    wmi_tpc_configs tpc_cfgs = halphy_tpc_stats.tpc_cfg;
    if (band == WMI_HALPHY_STATS_BAND_5GHZ ||
        band == WMI_HALPHY_STATS_BAND_6GHZ)
    {
        if ((tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_BE) & 0x1) {
            switch(pream_idx) {
                case WMI_HALPHY_STATS_HT20:
                case WMI_HALPHY_STATS_VHT20:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT_VHT20_5G_6G;
                    break;
                case WMI_HALPHY_STATS_HE20:
                case WMI_HALPHY_STATS_EHT20:
                    mode_idx = WMI_HALPHY_STATS_CTL_HE_EHT20_5G_6G;
                    break;
                case WMI_HALPHY_STATS_HT40:
                case WMI_HALPHY_STATS_VHT40:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT_VHT40_5G_6G;
                    break;
                case WMI_HALPHY_STATS_HE40:
                case WMI_HALPHY_STATS_EHT40:
                    mode_idx = WMI_HALPHY_STATS_CTL_HE_EHT40_5G_6G;
                    break;
                case WMI_HALPHY_STATS_VHT80:
                    mode_idx = WMI_HALPHY_STATS_CTL_VHT80_5G_6G;
                    break;
                case WMI_HALPHY_STATS_EHT60:
                    mode_idx = WMI_HALPHY_STATS_CTL_EHT80_SU_PUNC20;
                    break;
                case WMI_HALPHY_STATS_HE80:
                case WMI_HALPHY_STATS_EHT80:
                    mode_idx = WMI_HALPHY_STATS_CTL_HE_EHT80_5G_6G;
                    break;
                case WMI_HALPHY_STATS_VHT160:
                    mode_idx = WMI_HALPHY_STATS_CTL_VHT160_5G_6G;
                    break;
                case WMI_HALPHY_STATS_HE160:
                case WMI_HALPHY_STATS_EHT160:
                    mode_idx = WMI_HALPHY_STATS_CTL_HE_EHT160_5G_6G;
                    break;
                case WMI_HALPHY_STATS_EHT120:
                case WMI_HALPHY_STATS_EHT140:
                    mode_idx = WMI_HALPHY_STATS_CTL_EHT160_SU_PUNC20;
                    break;
                case WMI_HALPHY_STATS_EHT200:
                    mode_idx = WMI_HALPHY_STATS_CTL_EHT320_SU_PUNC120;
                    break;
                case WMI_HALPHY_STATS_EHT240:
                    mode_idx = WMI_HALPHY_STATS_CTL_EHT320_SU_PUNC80;
                    break;
                case WMI_HALPHY_STATS_EHT280:
                    mode_idx = WMI_HALPHY_STATS_CTL_EHT320_SU_PUNC40;
                    break;
                case WMI_HALPHY_STATS_EHT320:
                    mode_idx = WMI_HALPHY_STATS_CTL_HE_EHT320_5G_6G;
                    break;
                    case WMI_HALPHY_STATS_OFDM:
                default:
                    /* for 5G and 6G, default case will be for OFDM */
                    mode_idx = WMI_HALPHY_STATS_CTL_LEGACY_5G_6G;
                    break;
            }
        } else {
            switch(pream_idx){
                case WMI_HALPHY_STATS_HT20:
                case WMI_HALPHY_STATS_VHT20:
                case WMI_HALPHY_STATS_HE20:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT_VHT_HE20_5G_6G_AX;
                    break;
                case WMI_HALPHY_STATS_HT40:
                case WMI_HALPHY_STATS_VHT40:
                case WMI_HALPHY_STATS_HE40:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT_VHT_HE40_5G_6G_AX;
                    break;
                case WMI_HALPHY_STATS_VHT80:
                case WMI_HALPHY_STATS_HE80:
                    mode_idx = WMI_HALPHY_STATS_CTL_VHT_HE80_5G_6G_AX;
                    break;
                default:
                    /* for 5G and 6G, default case will be for OFDM */
                    mode_idx = WMI_HALPHY_STATS_CTL_LEGACY_5G_6G_AX;
                    break;
            }
        }
    } else {
        if ((tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_BE) & 0x1) {
            switch(pream_idx){
                case WMI_HALPHY_STATS_OFDM:
                    mode_idx = WMI_HALPHY_STATS_CTL_LEGACY_2G;
                    break;
                case WMI_HALPHY_STATS_HT20:
                case WMI_HALPHY_STATS_VHT20:
                case WMI_HALPHY_STATS_HE20:
                case WMI_HALPHY_STATS_EHT20:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT20_2G;
                    break;
                case WMI_HALPHY_STATS_HT40:
                case WMI_HALPHY_STATS_VHT40:
                case WMI_HALPHY_STATS_HE40:
                case WMI_HALPHY_STATS_EHT40:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT40_2G;
                    break;
                    case WMI_HALPHY_STATS_CCK:
                default:
                    /* for 2G, default case will be CCK */
                    mode_idx = WMI_HALPHY_STATS_CTL_CCK_2G;
                    break;
            }
        } else {
            switch(pream_idx) {
                case WMI_HALPHY_STATS_OFDM:
                    mode_idx = WMI_HALPHY_STATS_CTL_LEGACY_2G_AX;
                    break;
                case WMI_HALPHY_STATS_HT20:
                case WMI_HALPHY_STATS_VHT20:
                case WMI_HALPHY_STATS_HE20:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT20_2G_AX;
                    break;
                case WMI_HALPHY_STATS_HT40:
                case WMI_HALPHY_STATS_VHT40:
                case WMI_HALPHY_STATS_HE40:
                    mode_idx = WMI_HALPHY_STATS_CTL_HT40_2G_AX;
                    break;
                default:
                    /* for 2G, default case will be CCK */
                    mode_idx = WMI_HALPHY_STATS_CTL_CCK_2G_AX;
                    break;
            }
        }
    }

    return mode_idx;
}

A_CHAR *preamidx_to_str(A_UINT8 pream_idx)
{
    switch (pream_idx)
    {
        case WMI_HALPHY_STATS_OFDM:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "OFDM");
            break;
        case WMI_HALPHY_STATS_HT20:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "HT20");
            break;
        case WMI_HALPHY_STATS_HT40:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "HT40");
            break;
        case WMI_HALPHY_STATS_VHT20:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "VHT20");
            break;
        case WMI_HALPHY_STATS_VHT40:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "VHT40");
            break;
        case WMI_HALPHY_STATS_VHT80:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "VHT80");
            break;
        case WMI_HALPHY_STATS_VHT160:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "VHT160");
            break;
        case WMI_HALPHY_STATS_HE20:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "HE20");
            break;
        case WMI_HALPHY_STATS_HE40:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "HE40");
            break;
        case WMI_HALPHY_STATS_HE80:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "HE80");
            break;
        case WMI_HALPHY_STATS_HE160:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "HE160");
            break;
        case WMI_HALPHY_STATS_EHT20:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT20");
            break;
        case WMI_HALPHY_STATS_EHT40:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT40");
            break;
        case WMI_HALPHY_STATS_EHT60:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT60");
            break;
        case WMI_HALPHY_STATS_EHT80:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT80");
            break;
        case WMI_HALPHY_STATS_EHT120:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT120");
            break;
        case WMI_HALPHY_STATS_EHT140:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT140");
            break;
        case WMI_HALPHY_STATS_EHT160:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT160");
            break;
        case WMI_HALPHY_STATS_EHT200:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT200");
            break;
        case WMI_HALPHY_STATS_EHT240:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT240");
            break;
        case WMI_HALPHY_STATS_EHT280:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT280");
            break;
        case WMI_HALPHY_STATS_EHT320:
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "EHT320");
            break;
        default:
            /* In general default case will be CCK */
            WMI_HALPHY_STATS_PREAM_STR_SET(pream_str, "CCK");
            break;
    }

    return pream_str;
}

STATUS validate_halphy_tpc_stats_ptr(void)
{

    if (halphy_tpc_stats.reg_pow == NULL ||
        halphy_tpc_stats.rates_arr == NULL ||
        halphy_tpc_stats.ctl_pow == NULL)
    {
        return FAILURE;
    }
    return SUCCESS;
}

/* According to the pream_idx provided notsuported() api
will check if whether BW/AX/BE/ supported or not. Please
have a look at enum WMI_HALPHY_STATS_SUPPORT_BITFIELDS
what all features it validates and tell whether underlying
fw supports or not
*/
A_BOOL isSupported(A_UINT16 pream_idx)
{
    A_BOOL support_160 = 0, support_320 = 0, support_be = 0, support_ax = 0, support_be_punc = 0;
    wmi_tpc_configs tpc_cfgs = halphy_tpc_stats.tpc_cfg;
    A_BOOL retval = ~0;

    support_160 = (tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_160) & 0x1;
    support_320 = (tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_320) & 0x1;
    support_be = (tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_BE) & 0x1;
    support_ax = (tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_AX) & 0x1;
    support_be_punc = (tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_BE) & 0x1;

    if (pream_idx == WMI_HALPHY_STATS_VHT160 ||
        pream_idx == WMI_HALPHY_STATS_HE160)
    {
        if (pream_idx == WMI_HALPHY_STATS_HE160) {
            retval = support_160 && support_ax;
            return retval;
        }
        return support_160;
    } else if (pream_idx >= WMI_HALPHY_STATS_EHT120 &&
               pream_idx <= WMI_HALPHY_STATS_EHT160)
    {
        if (pream_idx == WMI_HALPHY_STATS_EHT160) {
            retval = (support_160 && support_be);
            return retval;
        }
        retval = (support_160 && support_be && support_be_punc);
        return retval;
    } else if (pream_idx >= WMI_HALPHY_STATS_EHT200 &&
               pream_idx <= WMI_HALPHY_STATS_EHT320)
    {
        if (pream_idx == WMI_HALPHY_STATS_EHT320) {
            retval = support_320 && support_be;
            return retval;
        }
        retval = support_320 && support_be && support_be_punc;
        return retval;
    } else if (pream_idx >= WMI_HALPHY_STATS_EHT20 &&
               pream_idx <= WMI_HALPHY_STATS_EHT80)
    {
        if (pream_idx == WMI_HALPHY_STATS_EHT60) {
            retval = support_be && support_be_punc;
            return retval;
        }
        return support_be;
    } else if ((pream_idx >= WMI_HALPHY_STATS_HE20) &&
               (pream_idx <= WMI_HALPHY_STATS_HE80))
    {
        return support_ax;
    }

    return retval;
}

A_BOOL wmi_halphy_tpc_stats_print(wmi_halphy_ctrl_path_stats_event_fixed_param *fparam)
{
    wmi_tpc_configs tpc_cfgs = halphy_tpc_stats.tpc_cfg;
    A_UINT8 txbf = 0, mu = 0, pream_idx = ~0, band; //disabled
    A_UINT16 total_rates = 0, nss = 0, rix = 0, max_rix = 0, arr_rix = 0, chains = tpc_cfgs.numTxChain, nSpatialStream = tpc_cfgs.nss;
    A_UINT16 ratecode = 0x0, each_nss = 0, mcs = 0;
    A_INT16 pow_per_chain = 0;
    A_INT16 dup_var = 0, reg_pwr = 0;
    A_UINT16 tot_nss, txbf_d3, mode, p, q;
    int8_t ctl_pwr;
    A_BOOL support_ax_extra_mcs;
    A_INT8 *str;

    A_UINT8 max_nss[WMI_HALPHY_STATS_MAX_PREAM] = {
        WMI_HALPHY_STATS_NSS1, WMI_HALPHY_STATS_NSS1,
        WMI_HALPHY_STATS_NSS4, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS8, WMI_HALPHY_STATS_NSS8,
        WMI_HALPHY_STATS_NSS8, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS8, WMI_HALPHY_STATS_NSS8,
        WMI_HALPHY_STATS_NSS8, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS4, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS4, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS4, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS4, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS4, WMI_HALPHY_STATS_NSS4,
        WMI_HALPHY_STATS_NSS4
    };
    A_UINT8 max_rates[WMI_HALPHY_STATS_MAX_PREAM] = {
        WMI_HALPHY_STATS_RATE_MCS_CCK, WMI_HALPHY_STATS_RATE_MCS_OFDM,
        WMI_HALPHY_STATS_RATE_MCS_HT, WMI_HALPHY_STATS_RATE_MCS_HT,
        WMI_HALPHY_STATS_RATE_MCS_VHT, WMI_HALPHY_STATS_RATE_MCS_VHT,
        WMI_HALPHY_STATS_RATE_MCS_VHT, WMI_HALPHY_STATS_RATE_MCS_VHT,
        WMI_HALPHY_STATS_RATE_MCS_HE, WMI_HALPHY_STATS_RATE_MCS_HE,
        WMI_HALPHY_STATS_RATE_MCS_HE, WMI_HALPHY_STATS_RATE_MCS_HE,
        WMI_HALPHY_STATS_RATE_MCS_EHT, WMI_HALPHY_STATS_RATE_MCS_EHT,
        WMI_HALPHY_STATS_RATE_MCS_EHT, WMI_HALPHY_STATS_RATE_MCS_EHT,
        WMI_HALPHY_STATS_RATE_MCS_EHT, WMI_HALPHY_STATS_RATE_MCS_EHT,
        WMI_HALPHY_STATS_RATE_MCS_EHT, WMI_HALPHY_STATS_RATE_MCS_EHT,
        WMI_HALPHY_STATS_RATE_MCS_EHT, WMI_HALPHY_STATS_RATE_MCS_EHT,
        WMI_HALPHY_STATS_RATE_MCS_EHT
    };

    if (validate_halphy_tpc_stats_ptr() == FAILURE) {
        return FAILURE;
    }

    HALPHY_STATS_PRINT(FATAL,
        "*********************** TPC CONFIG ***********************\n"
        "*Powers are in 0.25 dbm Steps\n");
    HALPHY_STATS_PRINT(FATAL,
        "reg-domain-%d\t\tchanfreq-%d\n\n",
        tpc_cfgs.regDomain, tpc_cfgs.chanFreq);
    HALPHY_STATS_PRINT(FATAL,
        "power limit-%d\t\tMax Reg Domain Power-%d\n\n",
        (tpc_cfgs.twiceMaxRDPower)/2, tpc_cfgs.powerLimit);

    /* check whether AX_EXTRA_MCS is supported */
    support_ax_extra_mcs =
        tpc_cfgs.support_bits & (1 << WMI_HALPHY_STATS_SUPPORT_AX_EXTRA_MCS);

    if (support_ax_extra_mcs) {
        for (A_UINT8 i = WMI_HALPHY_STATS_HE20;i <= WMI_HALPHY_STATS_HE160; i++) {
            max_rates[i] = WMI_HALPHY_STATS_RATE_EXTRA_MCS_HE;
        }
    }

    for (A_UINT8 i = 0 ; i < WMI_HALPHY_STATS_MAX_PREAM; i++) {
        nss = (max_nss[i] < nSpatialStream?
               max_nss[i] : nSpatialStream);
        total_rates += max_rates[i] * nss;
    }

    HALPHY_STATS_PRINT(FATAL,
        "No. of Tx Chains-%d\tNo.of rates-%d\n",
        tpc_cfgs.numTxChain, total_rates);

    band = WMI_HALPHY_STATS_GET_BAND(tpc_cfgs.chanFreq);
    if (lg_halphyid == WMI_HALPHY_PDEV_TX_SUTXBF_STATS ||
        lg_halphyid == WMI_HALPHY_PDEV_TX_MUTXBF_STATS)
    {
        txbf = 1;
    }
    if (lg_halphyid ==  WMI_HALPHY_PDEV_TX_MU_STATS ||
        lg_halphyid == WMI_HALPHY_PDEV_TX_MUTXBF_STATS)
    {
        mu = 1;
    }

    if (mu) {
        if (txbf) {
            str = "MU WITH BF";
        } else {
            str = "MU";
        }
        pream_idx = WMI_HALPHY_STATS_VHT20; // VHT20
        for (A_UINT16 i = WMI_HALPHY_STATS_CCK; i <= WMI_HALPHY_STATS_HT40; i++) {
            nss = max_nss[i] < nSpatialStream ?
                max_nss[i] : nSpatialStream;
            arr_rix += nss * max_rates[i];
            max_rix += max_nss[i] * max_rates[i];
        }
    } else {
        if (txbf) {
            str = "SU WITH BF";
        } else {
            str = "SU";
        }
        pream_idx = WMI_HALPHY_STATS_CCK; // CCK
    }

    HALPHY_STATS_PRINT(FATAL,
        "***************************%s****************************\n\n\n"
        "\t\tTPC values for Active chains\n\n\n"
        "Rateidx\t\tPreamble\tRate code\t", str);

    for (A_UINT8 k = 1; k <= tpc_cfgs.numTxChain; k++) {
        /* singlechain, doublechain..... */
        HALPHY_STATS_PRINT(FATAL, "%d-chain\t\t", k);
    }

    HALPHY_STATS_PRINT(FATAL, "\n");

    /* below for loop used to fill all the rate indices,rc and tpc pwr values */
    for (A_UINT16 i = pream_idx; i < WMI_HALPHY_STATS_MAX_PREAM; i++) {
        nss = max_nss[i] < nSpatialStream ? max_nss[i] : nSpatialStream;

        if (i >= WMI_HALPHY_STATS_VHT160 && i < WMI_HALPHY_STATS_MAX_PREAM) {
            if (!isSupported(i)) {
                max_rix += max_nss[i] * max_rates[i];
                continue;
            }
        }
        if (band == WMI_HALPHY_STATS_BAND_2GHZ) {
            if (i == WMI_HALPHY_STATS_VHT80 ||
                i == WMI_HALPHY_STATS_VHT160 ||
                i == WMI_HALPHY_STATS_HE80 ||
                i == WMI_HALPHY_STATS_HE160 ||
                ((i >= WMI_HALPHY_STATS_EHT60) &&
                (i <= WMI_HALPHY_STATS_EHT320)))
            {
                arr_rix += nss * max_rates[i];
                max_rix += max_nss[i] * max_rates[i];
                continue;
            }
        } else {
            if (i == WMI_HALPHY_STATS_CCK) {
                arr_rix += nss * max_rates[i];
                max_rix += max_nss[i] * max_rates[i];
                continue;
            }
        }

        each_nss = 0, mcs = 0;

        rix = max_rix + nss * max_rates[i];
        for (A_UINT32 j = max_rix; j < rix; j++, arr_rix++) {
            if (mcs >= max_rates[i]) {
                mcs = 0, each_nss++;
            }
            ratecode = wmi_halphy_get_ratecode(i, each_nss, mcs);
            HALPHY_STATS_PRINT(FATAL,
                "\n%d\t\t%s\t\t0x%x\t", j, preamidx_to_str(i), ratecode);
            for (A_UINT8 k = 0; k < chains; k++) {
                if (each_nss > k) {
                    HALPHY_STATS_PRINT(FATAL, "\tNA\t");
                    continue;
                }
                reg_pwr = halphy_tpc_stats.reg_pow[k];
                /* tgt pwr extracted using below piece of code */
                if (((tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_AX) & 0x1) && !((tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_BE) & 0x1))
                {
                    if (k > 3) {
                        if (mu) {
                            dup_var = halphy_tpc_stats.rates_arr2[arr_rix];
                            dup_var = dup_var >> 8 & 0xff; /* 15 to 7 bits */
                        } else {
                            dup_var = halphy_tpc_stats.rates_arr2[arr_rix];
                            dup_var = dup_var & 0xff;
                        }
                    } else {
                        if (mu) {
                            dup_var = halphy_tpc_stats.rates_arr[arr_rix];
                            dup_var = dup_var >> 8 & 0xff; /* 15 to 7 bits */
                        } else {
                            dup_var = halphy_tpc_stats.rates_arr[arr_rix];
                            dup_var = dup_var & 0xff;
                        }
                    }
                } else {
                    if (mu) {
                        dup_var = halphy_tpc_stats.rates_arr[arr_rix];
                        dup_var = dup_var >> 8 & 0xff; /* 15 to 7 bits */
                    } else {
                        dup_var = halphy_tpc_stats.rates_arr[arr_rix];
                        dup_var = dup_var & 0xff;
                    }
                }

                if (((tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_AX) & 0x1) && !((tpc_cfgs.support_bits >> WMI_HALPHY_STATS_SUPPORT_BE) & 0x1))
                {
                    if (i == WMI_HALPHY_STATS_VHT160 ||
                        i == WMI_HALPHY_STATS_HE160)
                    {
                        if (!(tpc_cfgs.support_bits >> WMI_HALPHY_TPC_STATS_CTL_DESIGN_1) & 0x1)
                        {
                            A_UINT16 r;
                            r=k;
                            r=r/2;
                            tot_nss = halphy_tpc_stats.ctl_pow_config160.d1;
                            txbf_d3 = halphy_tpc_stats.ctl_pow_config160.d2;
                            mode = halphy_tpc_stats.ctl_pow_config160.d3;
                            p = tot_nss * mode * txbf_d3;
                            q = tot_nss * mode;

                            ctl_pwr = wmi_halphy_get_min16(
                                halphy_tpc_stats.ctl_pow160[r*p + txbf*q + PRIMARY*tot_nss + each_nss],
                                halphy_tpc_stats.ctl_pow160[r*p + txbf*q + SECONDARY*tot_nss + each_nss]);
                        } else {
                            tot_nss = halphy_tpc_stats.ctl_pow_config160.d1;
                            txbf_d3 = halphy_tpc_stats.ctl_pow_config160.d2;
                            mode = halphy_tpc_stats.ctl_pow_config160.d3;
                            p = tot_nss * mode * txbf_d3;
                            q = tot_nss * mode;

                            ctl_pwr = wmi_halphy_get_min16(
                                halphy_tpc_stats.ctl_pow160[k*p + txbf*q + PRIMARY*tot_nss + each_nss],
                                halphy_tpc_stats.ctl_pow160[k*p + txbf*q + SECONDARY*tot_nss + each_nss]);
                        }
                    } else {
                        /* Handling CTL pwr for 11AX <=80MHz*/

                        A_UINT8 mode_ctl = get_mode_ctl_pwr(i, band);
                        tot_nss = halphy_tpc_stats.ctl_pow_config.d1;
                        mode = halphy_tpc_stats.ctl_pow_config.d2;
                        txbf_d3 = halphy_tpc_stats.ctl_pow_config.d3;
                        p = tot_nss * mode * txbf_d3;
                        q = tot_nss * mode;
                        ctl_pwr = halphy_tpc_stats.ctl_pow[k*p + txbf*q + mode_ctl*tot_nss + each_nss];
                    }
                } else {
                    A_UINT8 mode_ctl = get_mode_ctl_pwr(i, band);
                    tot_nss = halphy_tpc_stats.ctl_pow_config.d1;
                    mode = halphy_tpc_stats.ctl_pow_config.d2;
                    txbf_d3 = halphy_tpc_stats.ctl_pow_config.d3;
                    p = tot_nss * mode * txbf_d3;
                    q = tot_nss * mode;

                    ctl_pwr = halphy_tpc_stats.ctl_pow[k*p + txbf*q + mode_ctl*tot_nss + each_nss];
                }

                //HALPHY_STATS_PRINT(FATAL, "reg_pwr %u tgt_pwr %u ctl_pwr %u", reg_pwr, dup_var, ctl_pwr);
                pow_per_chain = wmi_halphy_get_min16(wmi_halphy_get_min16(ctl_pwr, reg_pwr), dup_var);
                pow_per_chain = wmi_halphy_get_min16(pow_per_chain, POWER_LIMIT);

                /*
                 * Comparing with least pow -40 supported by FW,
                 * it is referred as minNegativePower in WHAL_TPC_CONFIG
                 */

                pow_per_chain = pow_per_chain < -40 ? -40 : pow_per_chain;
                HALPHY_STATS_PRINT(FATAL,"\t%d\t", pow_per_chain);
            }
            HALPHY_STATS_PRINT(FATAL, "\n");
            mcs++;
        }
        max_rix += max_nss[i] * max_rates[i];
    }
    return SUCCESS;
}

A_INT32 wmi_halphy_tpc_stats_parser(wmi_halphy_ctrl_path_stats_event_fixed_param *fparam, A_UINT8 **buf_ptr, A_UINT32 len, A_UINT32 tag)
{
    if (tag == WMITLV_TAG_STRUC_wmi_tpc_configs) {
        memcpy(
            &halphy_tpc_stats.tpc_cfg,
            (wmi_tpc_configs *)*buf_ptr,
            sizeof(halphy_tpc_stats.tpc_cfg));
        *buf_ptr += sizeof(wmi_tpc_configs);
        limit_len += sizeof(wmi_tpc_configs);
    } else if (tag == WMITLV_TAG_STRUC_wmi_max_reg_power_allowed) {
        for (A_UINT8 i = 0; i < WMI_HALPHY_STATS_REG_POW_MAX_TYPES; i++) {
            if (((wmi_max_reg_power_allowed *) *buf_ptr)->reg_power_type == WMI_HALPHY_STATS_REG_POW_REGULAR) {
                memcpy(
                    &halphy_tpc_stats.reg_pow_config,
                    (wmi_max_reg_power_allowed *) *buf_ptr,
                    sizeof(halphy_tpc_stats.reg_pow_config));
                *buf_ptr += sizeof(wmi_max_reg_power_allowed);
                limit_len += sizeof(wmi_max_reg_power_allowed);
            }
        }
        *buf_ptr += WMI_TLV_HDR_SIZE; //passing INT16 TLV header
        limit_len += WMI_TLV_HDR_SIZE; //passing INT16 TLV header

        halphy_tpc_stats.reg_pow = malloc(
            halphy_tpc_stats.reg_pow_config.reg_power_array_len);

        if (!halphy_tpc_stats.reg_pow) {
            HALPHY_STATS_PRINT(FATAL,
                "Unable to allocate the memory for reg_pow\n");
            return -ENOMEM;
        }
        for (A_UINT8 i = 0; i < WMI_HALPHY_STATS_REG_POW_MAX_TYPES; i++) {
            if ((i == halphy_tpc_stats.reg_pow_config.reg_power_type)) {
                memcpy(
                    halphy_tpc_stats.reg_pow,
                    (A_INT16 *)*buf_ptr,
                    halphy_tpc_stats.reg_pow_config.reg_power_array_len);
                *buf_ptr += halphy_tpc_stats.reg_pow_config.reg_power_array_len;
                limit_len += halphy_tpc_stats.reg_pow_config.reg_power_array_len;
            }
        }
    } else if (tag == WMITLV_TAG_STRUC_wmi_tpc_rates_array) {
        wmi_tpc_rates_array local_rates_config;
        memcpy(
            &local_rates_config,
            &halphy_tpc_stats.rates_config,
            sizeof(halphy_tpc_stats.rates_config));
        memcpy(
            &halphy_tpc_stats.rates_config,
            (wmi_tpc_rates_array *)*buf_ptr,
            sizeof(halphy_tpc_stats.rates_config));
        *buf_ptr += sizeof(wmi_tpc_rates_array);
        limit_len += sizeof(wmi_tpc_rates_array);
        if (halphy_tpc_stats.rates_config.rate_array_type == WMI_HALPHY_STATS_TGT_POW_ARR_REGULAR) {
            *buf_ptr += WMI_TLV_HDR_SIZE; /* passing INT16 TLV header */
            limit_len += WMI_TLV_HDR_SIZE; /* passing INT16 TLV header */

            halphy_tpc_stats.rates_arr =
                malloc(halphy_tpc_stats.rates_config.rate_array_len);
            if (!halphy_tpc_stats.rates_arr) {
                free(halphy_tpc_stats.reg_pow);
                halphy_tpc_stats.reg_pow = NULL;
                HALPHY_STATS_PRINT(FATAL,
                    "Unable to allocate the memory for tgt_pw1\n");
                return -ENOMEM;
            }
            memcpy(
                halphy_tpc_stats.rates_arr,
                (A_INT16 *)*buf_ptr,
                halphy_tpc_stats.rates_config.rate_array_len);
        } else if (halphy_tpc_stats.rates_config.rate_array_type == WMI_HALPHY_STATS_TGT_POW_ARR2) {
            if ((halphy_tpc_stats.tpc_cfg.support_bits >> WMI_HALPHY_TPC_STATS_SUPPORT_BE) & 0x1)
            {
                if (!halphy_tpc_stats.rates_arr) {
                    HALPHY_STATS_PRINT(FATAL, "tgt_pw2 isnot allocated\n");
                    return -FAILURE;
                }
                A_INT16 size_hearray = 0;
                A_INT16 *dup_rate_arr = NULL;

                size_hearray = local_rates_config.rate_array_len/sizeof(*(halphy_tpc_stats.rates_arr));

                //dup_rate_arr = halphy_tpc_stats.rates_arr;

                //halphy_tpc_stats.rates_arr = realloc(halphy_tpc_stats.rates_arr, local_rates_config.rate_array_len + halphy_tpc_stats.rates_config.rate_array_len);
                dup_rate_arr = malloc(
                    local_rates_config.rate_array_len +
                    halphy_tpc_stats.rates_config.rate_array_len);
                if (!dup_rate_arr) {
                    free(halphy_tpc_stats.reg_pow);
                    free(halphy_tpc_stats.rates_arr);
                    halphy_tpc_stats.reg_pow = NULL;
                    halphy_tpc_stats.rates_arr = NULL;
                    HALPHY_STATS_PRINT(FATAL,
                        "Unable to Re-allocate the memory for tgt pwr 2\n");
                    return -ENOMEM;
                }
                memcpy(
                    dup_rate_arr,
                    halphy_tpc_stats.rates_arr,
                    local_rates_config.rate_array_len);
                free(halphy_tpc_stats.rates_arr);
                halphy_tpc_stats.rates_arr = dup_rate_arr;

                /*if (halphy_tpc_stats.rates_arr != dup_rate_arr) {
                    printf("%s %d %p %p\n", __func__, __LINE__, halphy_tpc_stats.rates_arr, dup_rate_arr);
                    free(dup_rate_arr);
                    printf("%s %d\n", __func__, __LINE__);
                    halphy_tpc_stats.rates_arr = NULL;
                    printf("%s %d\n", __func__, __LINE__);
                }*/

                *buf_ptr += WMI_TLV_HDR_SIZE; /* passing INT16 TLV header */
                limit_len += WMI_TLV_HDR_SIZE; /* passing INT16 TLV header */

                memcpy(
                    halphy_tpc_stats.rates_arr + size_hearray,
                    ((A_INT16 *)*buf_ptr),
                    halphy_tpc_stats.rates_config.rate_array_len);
            }
            if ((halphy_tpc_stats.tpc_cfg.support_bits >> WMI_HALPHY_TPC_STATS_SUPPORT_AX) & 0x1)
            {
                *buf_ptr += WMI_TLV_HDR_SIZE;
                limit_len += WMI_TLV_HDR_SIZE;
                if (!halphy_tpc_stats.rates_arr) {
                    free(halphy_tpc_stats.reg_pow);
                    halphy_tpc_stats.reg_pow = NULL;
                    HALPHY_STATS_PRINT(FATAL,
                        "Unable to allocate the memory for tgt_pw1\n");
                    return -ENOMEM;
                }
                halphy_tpc_stats.rates_arr2 = malloc(
                    halphy_tpc_stats.rates_config.rate_array_len);
                if (!halphy_tpc_stats.rates_arr2) {
                    free(halphy_tpc_stats.reg_pow);
                    free(halphy_tpc_stats.rates_arr);
                    halphy_tpc_stats.reg_pow = NULL;
                    halphy_tpc_stats.rates_arr = NULL;
                    HALPHY_STATS_PRINT(FATAL,
                        "Unable to allocate the memory for tgt_pw2\n");
                    return -ENOMEM;
                }
                memcpy(
                    halphy_tpc_stats.rates_arr2,
                    (A_INT16 *)*buf_ptr,
                    halphy_tpc_stats.rates_config.rate_array_len);
            }
        }
        *buf_ptr += halphy_tpc_stats.rates_config.rate_array_len;
        limit_len += halphy_tpc_stats.rates_config.rate_array_len;
    } else if (tag == WMITLV_TAG_STRUC_wmi_tpc_ctl_pwr_table) {
        memcpy(
            &halphy_tpc_stats.ctl_pow_config,
            (wmi_tpc_ctl_pwr_table *)*buf_ptr,
            sizeof(halphy_tpc_stats.ctl_pow_config));
        *buf_ptr += sizeof(wmi_tpc_ctl_pwr_table);
        limit_len += sizeof(wmi_tpc_ctl_pwr_table);
        if (halphy_tpc_stats.ctl_pow_config.end_of_ctl_pwr == 0) {
            memcpy(
                &halphy_tpc_stats.ctl_pow_config160,
                (wmi_tpc_ctl_pwr_table *)*buf_ptr,
                sizeof(halphy_tpc_stats.ctl_pow_config160));
            *buf_ptr += sizeof(wmi_tpc_ctl_pwr_table);
            limit_len += sizeof(wmi_tpc_ctl_pwr_table);
        }
        if (halphy_tpc_stats.ctl_pow_config.ctl_array_type == WMI_HALPHY_STATS_CTL_POW_REGULAR) {
            *buf_ptr += WMI_TLV_HDR_SIZE; /* passing ARRAY_BYTE TLV header */
            limit_len += WMI_TLV_HDR_SIZE; /* passing ARRAY_BYTE TLV header */

            halphy_tpc_stats.ctl_pow =
                malloc(halphy_tpc_stats.ctl_pow_config.ctl_array_len);

            if (!halphy_tpc_stats.ctl_pow) {
                HALPHY_STATS_PRINT(FATAL,
                    "Unable to allocate the memory for reg_pow\n");
                free(halphy_tpc_stats.reg_pow);
                free(halphy_tpc_stats.rates_arr);
                halphy_tpc_stats.reg_pow = NULL;
                halphy_tpc_stats.rates_arr = NULL;
                return -ENOMEM;
            }
            memcpy(
                halphy_tpc_stats.ctl_pow,
                (int8_t *)*buf_ptr,
                halphy_tpc_stats.ctl_pow_config.ctl_array_len);
            *buf_ptr += halphy_tpc_stats.ctl_pow_config.ctl_array_len;
            limit_len += halphy_tpc_stats.ctl_pow_config.ctl_array_len;
        }
        if (halphy_tpc_stats.ctl_pow_config160.ctl_array_type == WMI_HALPHY_STATS_CTL_POW_160) {
            /* If CTL160 is present, following would be the WMI packing.[wmi_tpc_ctl_pwr_table for regular][wmi_tpc_ctl_pwr_table for CTL160] [ARRAY_BYTE][CTL_Regular Array][CTL 160 array] */
            halphy_tpc_stats.ctl_pow160 =
                malloc(halphy_tpc_stats.ctl_pow_config160.ctl_array_len);

            if (!halphy_tpc_stats.ctl_pow160) {
                HALPHY_STATS_PRINT(FATAL,
                    "Unable to allocate the memory for reg_pow\n");
                return -ENOMEM;
            }
            memcpy(
                halphy_tpc_stats.ctl_pow160,
                (int8_t *)*buf_ptr,
                halphy_tpc_stats.ctl_pow_config160.ctl_array_len);

            *buf_ptr += halphy_tpc_stats.ctl_pow_config160.ctl_array_len;
            limit_len += halphy_tpc_stats.ctl_pow_config160.ctl_array_len;
        }
    }
    return SUCCESS;
}

A_INT32 wmi_halphy_print_stats(wmi_halphy_ctrl_path_stats_event_fixed_param *fparam, A_UINT8 **buf_ptr, A_UINT32 len, A_UINT32 tag)
{
    A_INT32 retval = SUCCESS;
    if (tag == WMITLV_TAG_STRUC_wmi_tpc_configs ||
        tag == WMITLV_TAG_STRUC_wmi_max_reg_power_allowed ||
        tag == WMITLV_TAG_STRUC_wmi_tpc_rates_array ||
        tag == WMITLV_TAG_STRUC_wmi_tpc_ctl_pwr_table)
    {
        retval = wmi_halphy_tpc_stats_parser(fparam, buf_ptr, len, tag);
    }
    return retval;
}

/*
 * Note that this function and the functions called by this function
 * make assumptions that the sizes of the halphy stats TLV structs
 * match the sizeof(halphy TLV struct) that the host was compiled with.
 * This requires that buff passed into this function have been processed
 * by wmitlv_check_event_tlv_params or some similar function, to expand
 * (by padding with 0x0) any TLV structs provided by the target whose size
 * is less than the size expected by the host, and truncate any TLV structs
 * provided by the target whose size is larger than the size expected by
 * the host.
 */
static A_INT32 halphy_stats_handler(void *buff, A_INT32 len)
{
    A_UINT8* buf_ptr = (A_UINT8*)buff;
    wmi_halphy_ctrl_path_stats_event_fixed_param *fparam;
    A_INT32 retval = SUCCESS;

    A_UINT32 tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
    A_UINT32 tag_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));

    if (tag == WMITLV_TAG_STRUC_wmi_halphy_ctrl_path_stats_event_fixed_param) {
        limit_len = 0;
        fparam = (wmi_halphy_ctrl_path_stats_event_fixed_param *)buf_ptr;

        if (fparam->end_of_event) {
            status = LISTEN_DONE;
        }

        buf_ptr += sizeof(wmi_halphy_ctrl_path_stats_event_fixed_param);
        limit_len += sizeof(wmi_halphy_ctrl_path_stats_event_fixed_param);
    } else {
        HALPHY_STATS_PRINT(FATAL,"%d: InValid Tag ID\n Invalid Buffer!\n", tag);
        status = LISTEN_DONE;
        return status;
    }

    while ((limit_len < len) && (tag >= WMITLV_TAG_FIRST_ARRAY_ENUM)) {
        tag = WMITLV_GET_TLVTAG(WMITLV_GET_HDR(buf_ptr));
        tag_len = WMITLV_GET_TLVLEN(WMITLV_GET_HDR(buf_ptr));
        if (!tag_len || tag == WMITLV_TAG_ARRAY_STRUC
            || tag == WMITLV_TAG_ARRAY_BYTE || tag == WMITLV_TAG_ARRAY_INT16) {
            buf_ptr += WMI_TLV_HDR_SIZE;
            limit_len += WMI_TLV_HDR_SIZE;
        } else {
            retval = wmi_halphy_print_stats(fparam, &buf_ptr, len, tag);
            if (retval) {
                status = LISTEN_DONE;
                return status;
            }
        }
    }

    if (status == LISTEN_DONE) {
        switch (lg_statsid)
        {
            case WMI_HALPHY_STATSID_PDEV_TPC:
                wmi_halphy_tpc_stats_print(fparam);
                wmi_halphy_tpc_stats_buffer_free(&halphy_tpc_stats);
                break;
            default:
                HALPHY_STATS_PRINT(FATAL,
                    "Invalid stats_id: %u\n", (unsigned) lg_statsid);
                break;
        }
    }

    return status;
}

/*
 * halphy_stats_buff_free:
 * @buff - Bufferto be freed
 * return:none
 */
static void halphy_stats_buff_free(void *buff)
{
    free(buff);
}

/*
 * halphy_stats_cookie_generate: Generate cookie
 * return process id of the current process;
 */
static A_INT32 halphy_stats_cookie_generate(void)
{
    return getpid();
}

/*
 * wmi_stats_buff_alloc: Allocate maximum size buffer
 *
 * return: pointer to a buffer
 */
static void *halphy_stats_buff_alloc(A_INT32 *buff_len)
{
    void *buff = malloc(WMI_HALPHY_STATS_BUFF_SIZE_MAX);

    if (!buff) {
        return NULL;
    }

    memset(buff, 0x0, WMI_HALPHY_STATS_BUFF_SIZE_MAX);
    *buff_len = WMI_HALPHY_STATS_BUFF_SIZE_MAX;
    return buff;
}

/*
 * halphy_stats_cookie_get: Will get the cookie
 *
 * return: Request_id of current process
 */
static A_INT32 halphy_stats_cookie_get(void *buff, A_INT32 len)
{
    wmi_halphy_ctrl_path_stats_event_fixed_param  *ev;
    ev = (wmi_halphy_ctrl_path_stats_event_fixed_param *) buff;

    return ev->request_id;
}

void pdev_stats_init(void)
{
    memset(&halphy_tpc_stats, 0x0, sizeof(wmi_halphy_tpc_stats_event));
}

/*
 * halphy_stats_input_parse: Parse the input command given by the user
 * buff: Pointer to the user cmd
 * argc: num of CLA
 * argv: CLA given by user
 * buff_len: Length of the cmd
 * pdev_id: Iface, stats requested for
 * return: None, Parse the CLA and enable/assign respective cmd fields
 */
static A_INT32 halphy_stats_input_parse(void *buff, A_INT32 argc, A_CHAR *argv[], A_INT32 *buff_len, A_INT32 pdev_id)
{
    wmi_request_halphy_ctrl_path_stats_cmd_fixed_param  *user_cmd = (wmi_request_halphy_ctrl_path_stats_cmd_fixed_param *) buff;
    A_UINT32 *pdev_ids = (A_UINT32 *)(user_cmd + sizeof(wmi_request_halphy_ctrl_path_stats_cmd_fixed_param) + WMI_TLV_HDR_SIZE);

    //A_UINT32 *vdev_ids, *mac_addr_array;
    A_UINT32 num_pdev_ids = 0;
    A_UINT32 num_vdev_ids = 0;
    A_UINT32 num_mac_addr_list = 0;
    A_UINT32 stats_id = atoi(argv[2]);
    A_UINT32 i = 3;
    A_UINT8 *buf_ptr=NULL;

    if (argc < 5 || argc > 8) {
        //Considering the cmd as per needed to pdev Tx stats
        // Min cmd is wifistats wifix <stats_id> --wmi --halphy
        // Max cmd is wifistats wifix <stats_id> --action <action_id> --wmi --halphy_subid <subid>
        return -1;
    } else if (stats_id < WMI_HALPHY_STATSID_PDEV_TPC ||
               stats_id >= WMI_HALPHY_STATSID_MAX)
    {
        HALPHY_STATS_PRINT(FATAL, "%d: Invalid stats_id\n", __LINE__);
        return -1;
    }

    /* since vdev_ids and mac_addr_array have not being used commenting out
    * enable when vdev_ids and mac_addr_array have been added to the param_tlv
    * But reserved the space for vdev_ids and mac_addr_array
    vdev_ids = pdev_ids + WMI_TLV_HDR_SIZE;
    mac_addr_array = vdev_ids + WMI_TLV_HDR_SIZE;
    */

    user_cmd->request_id = halphy_stats_cookie_generate();
    lg_statsid = user_cmd->stats_id_mask = stats_id;
    user_cmd->action = WMI_HALPHY_STATS_ACTION_GET;
    lg_halphyid = user_cmd->halphy_subid = WMI_HALPHY_PDEV_TX_SU_STATS;

    while (i < argc) {
        if (!strcmp(argv[i], "--action")) {
            lg_stats_action = user_cmd->action = atoi(argv[++i]);
            i++;
        } else if ((!strcmp(argv[i++], "--wmi")) &&
                   (!strcmp(argv[i++], "--halphy_subid")))
        {
            //i = i + 2; //pointing to halphy_subid
            if (argv[i]) {
                lg_halphyid = user_cmd->halphy_subid = atoi(argv[i++]);
                if (!((1 << user_cmd->halphy_subid) & 0xF)) {
                    HALPHY_STATS_PRINT(FATAL,
                        "%d: Invalid halphy_subid\n", __LINE__);
                    return -1;
                }
            }
        } else {
            HALPHY_STATS_PRINT(FATAL,
                "%d: Invalid number of args %d %d\n",
                __LINE__, i,
                ((!strcmp(argv[3], "--wmi")) &&
                 (!strcmp(argv[4], "--halphy_subid"))));
            return -1;
        }
    }

    if (user_cmd->action == WMI_HALPHY_STATS_ACTION_RESET &&
        stats_id == WMI_HALPHY_STATSID_PDEV_TPC)
    {
        /* As the globals will be updated. */
        HALPHY_STATS_PRINT(FATAL,
            "Reset stats are not supported for stats_id %u\n", stats_id);
        return -1;
    }

    switch(stats_id) {
        case WMI_HALPHY_STATSID_PDEV_TPC:
            pdev_ids[num_pdev_ids++] = pdev_id;
            pdev_stats_init();
            /*halphy_tpc_stats = malloc(sizeof(wmi_halphy_tpc_stats_event));
            printf("%s %d: halphy_tpc_stats %p\n", __func__, __LINE__, halphy_tpc_stats);
            if (!halphy_tpc_stats) {
                HALPHY_STATS_PRINT(FATAL, "Unable to create a stats buff");
                return -1;
            }*/
            break;
    }

    if (user_cmd->action == WMI_HALPHY_STATS_ACTION_RESET) {
        halphystats.timeout = 0;
    }

    WMITLV_SET_HDR(
        &user_cmd->tlv_header,
        WMITLV_TAG_STRUC_wmi_request_halphy_ctrl_path_stats_cmd_fixed_param,
        WMITLV_GET_STRUCT_TLVLEN(
            wmi_request_halphy_ctrl_path_stats_cmd_fixed_param));
    buf_ptr = buff + sizeof(wmi_request_halphy_ctrl_path_stats_cmd_fixed_param);

    WMITLV_SET_HDR(
        buf_ptr, WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32) * num_pdev_ids);
    buf_ptr += WMI_TLV_HDR_SIZE;
    memcpy(buf_ptr, pdev_ids, sizeof(A_UINT32) * num_pdev_ids);
    /* Setting tlv header for vdev id arrays */
    buf_ptr += sizeof(A_UINT32) * num_pdev_ids;

    WMITLV_SET_HDR(
        buf_ptr, WMITLV_TAG_ARRAY_UINT32, sizeof(A_UINT32) * num_vdev_ids);
    /* Setting tlv header for mac addr arrays */
    buf_ptr = buf_ptr + WMI_TLV_HDR_SIZE +(sizeof(A_UINT32) * num_vdev_ids);

    WMITLV_SET_HDR(
        buf_ptr,
        WMITLV_TAG_ARRAY_FIXED_STRUC,
        sizeof(A_UINT32) * num_mac_addr_list);
    /* Calculate total buffer length */
    *buff_len =
        sizeof(wmi_request_halphy_ctrl_path_stats_cmd_fixed_param) +
        WMI_TLV_HDR_SIZE + (num_pdev_ids * sizeof(A_UINT32)) +
        WMI_TLV_HDR_SIZE + (num_vdev_ids * sizeof(A_UINT32)) +
        WMI_TLV_HDR_SIZE + (num_mac_addr_list * sizeof(A_UINT32));

    return 0;
}

static struct wifistats_module halphystats = {
    .name                   =   "wmi_halphy_stats",
    .help                   =   halphy_stats_help,
    .input_buff_alloc       =   halphy_stats_buff_alloc,
    .input_parse            =   halphy_stats_input_parse,
    .input_cookie_generate  =   halphy_stats_cookie_generate,
    .input_buff_free        =   halphy_stats_buff_free,
    .output_handler         =   halphy_stats_handler,
    .output_cookie_get      =   halphy_stats_cookie_get,
    .timeout                =   2000,
};

void halphystats_init(void)
{
    wifistats_module_register(&halphystats, sizeof(halphystats));
}

void halphystats_fini(void)
{
    wifistats_module_unregister(&halphystats, sizeof(halphystats));
}
