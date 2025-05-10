/*
 * Copyright (c) 2018-2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

/*
 * enum values for vendor sub commands.
 * These are shared between application and driver.
 */

#ifndef CFG80211_EXTERNAL_H_
#define CFG80211_EXTERNAL_H_
enum qca_nl80211_vendor_subcmds_internal {
    QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS = 200,
    QCA_NL80211_VENDOR_SUBCMD_GET_PARAMS = 201,
    QCA_NL80211_VENDOR_SUBCMD_NAWDS_PARAMS = 203,
    QCA_NL80211_VENDOR_SUBCMD_HMWDS_PARAMS = 204,
    QCA_NL80211_VENDOR_SUBCMD_ALD_PARAMS = 205,
    QCA_NL80211_VENDOR_SUBCMD_ATF = 206,
    QCA_NL80211_VENDOR_SUBCMD_WNM = 207,
    QCA_NL80211_VENDOR_SUBCMD_ME_LIST = 208,
    QCA_NL80211_VENDOR_SUBCMD_SET_MAXRATE = 209,
    QCA_NL80211_VENDOR_SUBCMD_VENDORIE = 210,
    QCA_NL80211_VENDOR_SUBCMD_NAC = 211,
    QCA_NL80211_VENDOR_SUBCMD_LIST_SCAN = 212,
    QCA_NL80211_VENDOR_SUBCMD_LIST_CAP = 213,
    QCA_NL80211_VENDOR_SUBCMD_LIST_STA = 214,
    QCA_NL80211_VENDOR_SUBCMD_ACTIVECHANLIST = 215,
    QCA_NL80211_VENDOR_SUBCMD_LIST_CHAN = 216,
    QCA_NL80211_VENDOR_SUBCMD_LIST_CHAN160 = 217,
    QCA_NL80211_VENDOR_SUBCMD_DBGREQ     = 218,
    QCA_NL80211_VENDOR_SUBCMD_PHYSTATS = 219,
    QCA_NL80211_VENDOR_SUBCMD_ATHSTATS = 220,
    QCA_NL80211_VENDOR_SUBCMD_PHYSTATSCUR = 221,
    QCA_NL80211_VENDOR_SUBCMD_EXTENDEDSTATS = 222,
    QCA_NL80211_VENDOR_SUBCMD_80211STATS = 223,
    QCA_NL80211_VENDOR_SUBCMD_STA_STATS = 224,
    QCA_NL80211_VENDOR_SUBCMD_CLONEPARAMS = 225,

    QCA_NL80211_VENDORSUBCMD_ADDMAC = 226,
    QCA_NL80211_VENDORSUBCMD_DELMAC = 227,
    QCA_NL80211_VENDORSUBCMD_KICKMAC = 228,
    QCA_NL80211_VENDORSUBCMD_CHAN_SWITCH = 229,
    QCA_NL80211_VENDORSUBCMD_WIRELESS_MODE = 230,
    QCA_NL80211_VENDORSUBCMD_AC_PARAMS = 231,
    QCA_NL80211_VENDORSUBCMD_RC_PARAMS_SETRTPARAMS = 232,
    QCA_NL80211_VENDORSUBCMD_SETFILTER = 233,
    QCA_NL80211_VENDORSUBCMD_MAC_COMMANDS = 234,
    QCA_NL80211_VENDORSUBCMD_WMM_PARAMS = 235,
    QCA_NL80211_VENDORSUBCMD_COUNTRY_CONFIG = 236,
    QCA_NL80211_VENDORSUBCMD_HWADDR_CONFIG = 237,
    QCA_NL80211_VENDORSUBCMD_AGGR_BURST_CONFIG = 238,
    QCA_NL80211_VENDORSUBCMD_ATF_SCHED_DURATION_CONFIG = 239,
    QCA_NL80211_VENDORSUBCMD_TXRX_PEER_STATS = 240,
    QCA_NL80211_VENDORSUBCMD_CHANNEL_CONFIG = 241,
    QCA_NL80211_VENDORSUBCMD_SSID_CONFIG = 242,
    QCA_NL80211_VENDOR_SUBCMD_PHYERR = 243,
    QCA_NL80211_VENDORSUBCMD_MGMT_RSSI_THR = 244,
    QCA_NL80211_VENDORSUBCMD_GET_SSID = 245,
    QCA_NL80211_VENDOR_SUBCMD_GET_ACLMAC = 246,
    QCA_NL80211_VENDORSUBCMD_DP_FW_PEER_STATS = 247,
    QCA_NL80211_VENDORSUBCMD_SEND_MGMT = 248,
    QCA_NL80211_VENDORSUBCMD_CHECK_11H = 249,
    QCA_NL80211_VENDOR_SUBCMD_HTTSTATS = 250,
#if ATH_ACL_SOFTBLOCKING
    QCA_NL80211_VENDORSUBCMD_ADDMAC_SEC = 252,
    QCA_NL80211_VENDORSUBCMD_DELMAC_SEC = 253,
    QCA_NL80211_VENDORSUBCMD_GET_ACLMAC_SEC = 254,
#endif
    QCA_NL80211_VENDOR_SUBCMD_NAC_RSSI = 255,
    QCA_NL80211_VENDOR_SUBCMD_SET_SU_SOUNDING_INT = 256,
    QCA_NL80211_VENDOR_SUBCMD_SET_MU_SOUNDING_INT = 257,
    QCA_NL80211_VENDOR_SUBCMD_ENABLE_SOUNDING_INT = 258,
    QCA_NL80211_VENDOR_SUBCMD_SCHED_MU_ENABLE = 259,
    QCA_NL80211_VENDOR_SUBCMD_SCHED_OFDMA_ENABLE = 260,
    QCA_NL80211_VENDOR_SUBCMD_HE_MESH_CONFIG = 261,
    QCA_NL80211_VENDOR_SUBCMD_FWD_MGMT_FRAME = 262,
    QCA_NL80211_VENDOR_SUBCMD_MU_EDCA_PARAMS = 263,
#if ATH_ACS_DEBUG_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_ACSDBGTOOL_ADD_BCN = 264,
    QCA_NL80211_VENDOR_SUBCMD_ACSDBGTOOL_ADD_CHAN = 265,
#endif
#if WLAN_DFS_CHAN_HIDDEN_SSID
    QCA_NL80211_VENDORSUBCMD_CONF_BSSID = 266,
    QCA_NL80211_VENDORSUBCMD_GET_CONF_BSSID = 267,
#endif /* WLAN_DFS_CHAN_HIDDEN_SSID */
    QCA_NL80211_VENDORSUBCMD_BSSID = 268,
    QCA_NL80211_VENDORSUBCMD_SET_PEER_NEXTHOP = 269,
    QCA_NL80211_VENDORSUBCMD_RC_PARAMS_SETRATEMASK = 270,
    QCA_NL80211_VENDORSUBCMD_SET_BLOCK_MGT = 271,
    QCA_NL80211_VENDORSUBCMD_CLR_BLOCK_MGT = 272,
    QCA_NL80211_VENDORSUBCMD_GET_BLOCK_MGT = 273,
    QCA_NL80211_VENDOR_SUBCMD_SET_BA_TIMEOUT = 274,
    QCA_NL80211_VENDOR_SUBCMD_GET_BA_TIMEOUT = 275,
    QCA_NL80211_VENDOR_SUBCMD_ADDIE = 276,
    QCA_NL80211_VENDORSUBCMD_SET_VLAN_TYPE = 277,
#if WLAN_CFR_ENABLE
    QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG = 278,
#endif
    QCA_NL80211_VENDORSUBCMD_DEFAULT_PCP_TID_MAP = 279,
    QCA_NL80211_VENDORSUBCMD_DEFAULT_TIDMAP_PRTY = 280,
#if UMAC_SUPPORT_ACFG
    QCA_NL80211_VENDOR_SUBCMD_ACFG = 284,
#endif
#if SM_ENG_HIST_ENABLE
    QCA_NL80211_VENDOR_SUBCMD_SM_HISTORY = 285,
#endif
    QCA_NL80211_VENDOR_SUBCMD_GET_PRI20_BLOCKCHANLIST = 286,
#ifdef WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG
    QCA_NL80211_VENDOR_SUBCMD_RX_PKT_PROTOCOL_TAG = 288,
#ifdef WLAN_SUPPORT_RX_TAG_STATISTICS
    QCA_NL80211_VENDOR_SUBCMD_RX_PKT_PROTOCOL_TAG_STATS = 289,
#endif /* WLAN_SUPPORT_RX_TAG_STATISTICS */
#endif /* WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG */
#if UNIFIED_SMARTANTENNA
    QCA_NL80211_VENDOR_SUBCMD_SA_PARAMS = 290,
#endif
    QCA_NL80211_VENDOR_SUBCMD_SET_BTCOEX_DUTY_CYCLE = 291,
    QCA_NL80211_VENDOR_SUBCMD_GET_CHAN_RF_CHARACTERIZATION_INFO = 292,
    QCA_NL80211_VENDOR_SUBCMD_HE_SR_CONFIG = 293,
    QCA_NL80211_VENDOR_SUBCMD_GET_QLD_ENTRY = 294,
    QCA_NL80211_VENDOR_SUBCMD_NAV_OVERRIDE_CONFIG = 295,
#if OBSS_PD
    QCA_NL80211_VENDOR_SUBCMD_HE_SRG_BITMAP = 296,
#endif /* OBSS PD */
#ifdef WLAN_SUPPORT_RX_FLOW_TAG
    QCA_NL80211_VENDOR_SUBCMD_RX_FLOW_TAG_OP = 299,
#endif /* WLAN_SUPPORT_RX_FLOW_TAG */
#if defined(WLAN_TX_PKT_CAPTURE_ENH) || defined(WLAN_RX_PKT_CAPTURE_ENH)
    QCA_NL80211_VENDOR_SUBCMD_SET_PEER_PKT_CAPTURE_PARAMS = 300,
#endif /* WLAN_TX_PKT_CAPTURE_ENH || WLAN_RX_PKT_CAPTURE_ENH */
    QCA_NL80211_VENDORSUBCMD_CHAN_WIDTHSWITCH = 301,
    QCA_NL80211_VENDOR_SUBCMD_FWD_RRM_RPT = 302,
    QCA_NL80211_VENDORSUBCMD_LIST_SUPPORTED_BANDS = 303,
    QCA_NL80211_VENDOR_SUBCMD_FWD_BTM_RPT = 304,
    /* Set/Get Mu edca dynamic/Manual selection */
    QCA_NL80211_VENDOR_SUBCMD_MUEDCA_MODE = 305,
    /* Enable/disable non-HT duplicate Beacon/FILS/Broadcast Prb Resp frames */
    QCA_NL80211_VENDOR_SUBCMD_NON_HT_DUP_EN = 306,
    /* Get Regulatory Opclass params for SON */
    QCA_NL80211_VENDOR_SUBCMD_SON_REG_PARAMS = 307,
    QCA_NL80211_VENDOR_SUBCMD_SET_AP = 308,
    QCA_NL80211_VENDOR_SUBCMD_VAP_VENDOR_PARAM = 309,
    /* Self Spatial reuse configuration */
    QCA_NL80211_VENDOR_SUBCMD_SR_SELF_CONFIG = 310,
    QCA_NL80211_VENDOR_SUBCMD_PEER_ISOLATION = 311,
    QCA_NL80211_VENDOR_SUBCMD_WMISTATS = 312,
    QCA_NL80211_VENDOR_SUBCMD_GET_TARGET_PDEVID = 313,
    QCA_NL80211_VENDOR_SUBCMD_RNR_6GHZ = 314,
#if DBG_LVL_MAC_FILTERING
    QCA_NL80211_VENDOR_SUBCMD_DBGLVLMAC = 315,
#endif /* DBG_LVL_MAC_FILTERING */
    QCA_NL80211_VENDOR_SUBCMD_MESH_CONFIGURATION = 316,
    QCA_NL80211_VENDOR_SUBCMD_EMA_CONFIG = 317,

    /* Temporary internal command; will be removed after upstream is complete */
    QCA_NL80211_VENDOR_SUBCMD_GET_ACS_PRECAC_SUPPORT = 321,

    /* Temporary internal command; will be removed after upstream is complete */
    QCA_NL80211_VENDOR_SUBCMD_WIDEBAND_SUPPORT = 322,
    QCA_NL80211_VENDOR_SUBCMD_VDEV_TSF = 323,
    QCA_NL80211_VENDOR_SUBCMD_RTT_CONFIG = 324,
    /* Tuple command to set channel/phymode/cfreq2/band in one go */
    QCA_NL80211_VENDOR_SUBCMD_CHANNEL_TUPLE = 325,

    /* NL command to accumulate vap/radio level stats in driver */
    QCA_NL80211_VENDOR_SUBCMD_VAP_STATS = 326,
    QCA_NL80211_VENDOR_SUBCMD_RADIO_STATS = 327,

#if !(defined REMOVE_PKT_LOG) && (defined PKTLOG_DUMP_UPLOAD_SSR)
    /* NL command to enable pktlog collection during SSR */
    QCA_NL80211_VENDOR_SUBCMD_PKTLOG_SSR = 328,
#endif
    QCA_NL80211_VENDORSUBCMD_GET_CHANNEL = 329,
    QCA_NL80211_VENDOR_SUBCMD_SMPS_UPDATE = 330,
    QCA_NL80211_VENDOR_SUBCMD_OPMODE_UPDATE = 331,
    QCA_NL80211_VENDOR_SUBCMD_GET_NOISE_FLOOR = 332,
    QCA_NL80211_VENDOR_SUBCMD_HE_BSS_COLOR = 333,
#if WLAN_SUPPORT_TELEMETRY
    QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA = 334,
#endif /* WLAN_SUPPORT_TELEMETRY */
    QCA_NL80211_VENDOR_SUBCMD_GET_OPMODE=335,
    QCA_NL80211_VENDOR_SUBCMD_HALPHY_CAL = 336,
    QCA_NL80211_VENDOR_SUBCMD_GET_OPER_REG_INFO = 337,
    QCA_NL80211_VENDOR_SUBCMD_GET_PARTIAL_BSSID_BITMAP = 338,
    QCA_NL80211_VENDOR_SUBCMD_SET_KEY = 402,
    QCA_NL80211_VENDOR_SUBCMD_GET_KEY = 403,
    QCA_NL80211_VENDOR_SUBCMD_DEL_KEY = 404,
    QCA_NL80211_VENDOR_SUBCMD_MU_SNIF = 405,
#ifdef UMAC_SUPPORT_MLNAWDS
    QCA_NL80211_VENDOR_SUBCMD_MLNAWDS_PARAMS = 406,
#endif
#if QCA_SUPPORT_LITE_MONITOR
    QCA_NL80211_VENDOR_SUBCMD_LITE_MONITOR = 407,
#endif
    QCA_NL80211_VENDOR_SUBCMD_AFC_RESET = 408,
#ifdef CONFIG_SAWF_DEF_QUEUES
    QCA_NL80211_VENDORSUBCMD_SET_RC_PARAMS_RATE_UPPER_CAP = 409,
    QCA_NL80211_VENDORSUBCMD_SET_RC_PARAMS_RATE_RETRY_MCS_DROP = 410,
    QCA_NL80211_VENDORSUBCMD_SET_RC_PARAMS_MCS_PROBE_INTVL = 411,
    QCA_NL80211_VENDORSUBCMD_SET_RC_PARAMS_NSS_PROBE_INTVL = 412,
    QCA_NL80211_VENDORSUBCMD_DP_SAWF_PEER_SVCID_MAP = 413,
    QCA_NL80211_VENDORSUBCMD_DP_SAWF_PEER_SVCID_UNMAP = 414,
    QCA_NL80211_VENDORSUBCMD_DP_SAWF_PEER_MAP_REPORT = 415,
#endif
#ifdef QCA_UNDECODED_METADATA_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_SET_PHYRX_ERROR_MASK = 416,
    QCA_NL80211_VENDOR_SUBCMD_GET_PHYRX_ERROR_MASK = 417,
#endif
    QCA_NL80211_VENDOR_SUBCMD_MCTBL_UPDATE = 418,
    QCA_NL80211_VENDOR_SUBCMD_SAWF_CONFIG = 419,
    QCA_NL80211_VENDOR_SUBCMD_GET_6G_CHAN_LIST = 420,
#ifdef WLAN_MGMT_RX_REO_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_MGMT_RX_REO_SIMULATION = 421,
#endif
#ifdef WLAN_FEATURE_11BE
    QCA_NL80211_VENDOR_SUBCMD_T2LM = 422,
#endif /* WLAN_FEATURE_11BE */
    QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG = 423,
#ifdef WLAN_MGMT_RX_REO_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_MGMT_RX_REO_INGRESS_FRAME_STATS = 424,
    QCA_NL80211_VENDOR_SUBCMD_MGMT_RX_REO_INGRESS_PER_FRAME_INFO = 425,
    QCA_NL80211_VENDOR_SUBCMD_MGMT_RX_REO_EGRESS_FRAME_STATS = 426,
    QCA_NL80211_VENDOR_SUBCMD_MGMT_RX_REO_EGRESS_PER_FRAME_INFO = 427,
#endif /* WLAN_MGMT_RX_REO_SUPPORT */
#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_DUMP_MLO_GLOBAL_H_SHMEM_ARENA = 428,
#endif /* WLAN_MLO_GLOBAL_SHMEM_SUPPORT */
    QCA_NL80211_VENDOR_SUBCMD_PROBE_WH_CONFIG = 429,
    /* NL command to Delete WDS ast entry */
    QCA_NL80211_VENDOR_SUBCMD_DELETE_WDS_AST = 430,
    QCA_NL80211_VENDOR_SUBCMD_HALPHY_STATS = 431,
    QCA_NL80211_VENDORSUBCMD_GET_PEER_EXTD_RATE_LINK_STATS = 432,
#ifdef CONFIG_AFC_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_GET_DEV_DEPLOYMENT_TYPE = 433,
#endif /* CONFIG_AFC_SUPPORT */
#ifdef WLAN_FEATURE_11BE
    QCA_NL80211_VENDOR_SUBCMD_T2LM_LINK_MAP_CONFIG=434,
#endif /* WLAN_FEATURE_11BE */
#ifdef CONFIG_AFC_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_AFC_GET_REG_EIRP = 435,
#endif /* CONFIG_AFC_SUPPORT */
#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
    QCA_NL80211_VENDOR_SUBCMD_MESH_PARAMS = 436,
#endif
#endif
    QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_REMOVAL = 437,
    QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_BTMREQ = 438,
    QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_REMOVAL_TBTT_UPDATE = 439,
    QCA_NL80211_VENDOR_SUBCMD_TDMA_SCHEDULE_CONFIG = 440,
    QCA_NL80211_VENDORSUBCMD_DISABLE_SCHED_MODES = 441,
    QCA_NL80211_VENDOR_SUBCMD_UMAC_RESET_DEBUG = 442,
#ifdef CONFIG_SAWF
    QCA_NL80211_VENDOR_SUBCMD_SAWF_PEER_MSDUQ_REPORT = 443,
#endif
    QCA_NL80211_VENDOR_SUBCMD_SET_FW_HANG = 444,
    QCA_NL80211_VENDOR_SUBCMD_FIPS_EVENT = 445,
    QCA_NL80211_VENDOR_SUBCMD_SEND_EGID = 446,
    QCA_NL80211_VENDOR_SUBCMD_PEER_DISABLE_SCHED_MODES = 448,
    QCA_NL80211_VENDOR_SUBCMD_EPCS = 449,
#ifdef CONFIG_AFC_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_AFC_PAYLOAD_RESET = 450,
#endif /* CONFIG_AFC_SUPPORT */
#ifdef WLAN_FEATURE_11BE_MLO
    QCN_NL80211_LINK_RECOMMENDATION_CMD = 451,
    QCA_NL80211_VENDOR_SUBCMD_PTQM_PEER_MIGRATE = 452,
    QCA_NL80211_VENDOR_SUBCMD_PTQM_LINK_MIGRATE = 453,
#endif
    QCA_NL80211_VENDOR_SUBCMD_MLD_LINK_ADD_DEL = 454,
    QCA_NL80211_VENDOR_SUBCMD_ENABLE_OL_STATS = 455,
    QCA_NL80211_VENDOR_SUBCMD_LINK_INFO = 456,
    QCA_NL80211_VENDOR_SUBCMD_AWGN_INTERFERENCE_UPDATE = 457,
    QCA_NL80211_VENDOR_SUBCMD_GET_BSS_PEER = 458,
    QCA_NL80211_VENDOR_SUBCMD_STA_VAP_STATS = 459,
    QCA_NL80211_VENDOR_SUBCMD_GET_MLDEV_PARAMS = 460,
    QCA_NL80211_VENDOR_SUBCMD_ULOFDMA_RTD = 461,
    QCA_NL80211_VENDOR_SUBCMD_LIST_PASN_PEER = 462,
    QCA_NL80211_VENDOR_SUBCMD_DEAUTHMAC = 463,
    QCA_NL80211_VENDOR_SUBCMD_TTLM_LINK_UPDATE_EVENT = 464,
#ifdef WLAN_SUPPORT_SCS_API
    QCA_NL80211_VENDOR_SUBCMD_SCS_API_MSG = 465,
#endif
#ifdef SAWF_ADMISSION_CONTROL
#ifdef WLAN_SUPPORT_SCS
    QCA_NL80211_VENDOR_SUBCMD_SCS_ADM_CTRL_MSG = 466,
#endif
    QCA_NL80211_VENDOR_SUBCMD_SAWF_PEER_MSDUQ_EVENT = 467,
#endif
    QCA_NL80211_VENDOR_SUBCMD_MON_FCS_CAP = 468,
#ifdef QCA_BLOCK_EVENT_TO_USERSPACE
    QCA_NL80211_VENDOR_SUBCMD_SET_EVENT_FILTER = 469,
#endif /* QCA_BLOCK_EVENT_TO_USERSPACE */
#ifdef WLAN_WIFI_RADAR_ENABLE
     QCA_NL80211_VENDOR_SUBCMD_WIFI_RADAR_CONFIG = 470,
#endif
#ifdef SAWF_ADMISSION_CONTROL
    QCA_NL80211_VENDOR_SUBCMD_SAWF_FLOW_DEPRIORITIZE_MSG = 471,
#endif
    QCA_NL80211_VENDOR_SUBCMD_MU_TPUT_INFO = 472,
#ifdef QCA_SUPPORT_EAPOL_OVER_CONTROL_PORT
    QCA_NL80211_VENDOR_SUBCMD_EAPOL_OVER_NL = 473,
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    QCA_NL80211_VENDOR_SUBCMD_GET_LINK_ID_BPCC_VAL = 474,
#endif /* WLAN_FEATURE_11BE_MLO */
#ifdef QCA_SUPPORT_EAPOL_OVER_CONTROL_PORT
    QCA_NL80211_VENDOR_SUBCMD_VAP_EAPOL_OVER_NL = 475,
#endif
    QCA_NL80211_VENDORSUBCMD_SEND_ACS_REPORT = 476
};

#ifdef CONFIG_AFC_SUPPORT
#define QCA_NL80211_AFC_REQ_RESP_BUF_MAX_SIZE       5000

/*
 * Opclass, channel and EIRP information attribute length
 * Refer kernel doc explanation for attribute
 * QCA_WLAN_VENDOR_ATTR_AFC_RESP_OPCLASS_CHAN_EIRP_INFO
 * to understand the minimum length calculation.
 */
#define QCA_WLAN_AFC_RESP_OPCLASS_CHAN_EIRP_INFO_MIN_LEN            \
        NLA_ALIGN((NLA_HDRLEN + sizeof(uint8_t)) +                  \
                  ((3 * NLA_HDRLEN) + sizeof(uint8_t) +             \
                   sizeof(uint32_t)))

/*
 * Frequency/PSD information attribute length
 * Refer kernel doc explanation for attribute
 * QCA_WLAN_VENDOR_ATTR_AFC_RESP_FREQ_PSD_INFO
 * to understand the minimum length calculation.
 */
#define QCA_WLAN_AFC_RESP_FREQ_PSD_INFO_INFO_MIN_LEN                \
        NLA_ALIGN((3 * NLA_HDRLEN) +                                \
                  (2 * sizeof(uint16_t)) +                          \
                   sizeof(uint32_t))

#define QCA_WLAN_AFC_RESP_DESC_FIELD_START_OCTET 14
#define QCA_WLAN_AFC_RESP_DESC_FIELD_END_OCTET   30

/**
 * enum qca_wlan_vendor_attr_afc_reg_eirp: Defines attributes to be used with
 * the vendor command QCA_NL80211_VENDOR_SUBCMD_AFC_GET_REG_EIRP.
 *
 * @QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_POWER_TYPE: Required u8 attribute.
 * This attribute defines the Power type for the which the Regulatory Power
 * limits are being queried for. The valid values for this attribute are defined
 * in enum qca_wlan_vendor_reg_power_type.
 *
 * @QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_CLIENT_TYPE: Required u8 attribute.
 * This attribute defines the Client types for which the Regulatory Power
 * limits are being queried for. The valid values for this attribute are defined
 * in enum qca_wlan_vendor_afc_client_type.
 */
enum qca_wlan_vendor_attr_afc_reg_eirp
{
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_POWER_TYPE,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_CLIENT_TYPE,

    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_MAX =
        QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_afc_reg_eirp_update: Defines the attributes that
 * are used to send the EIRP power values in response to vendor command
 * QCA_NL80211_VENDOR_SUBCMD_AFC_GET_REG_EIRP.
 *
 * @QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_CENTER_FREQ: Required u32 attribute
 * This attribute mentions the center frequency value of the 6GHz spectrum
 * for which the EIRP values are being sent.
 *
 * @QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_CHAN_NUM: Required u8 attribute
 * This attribute mentions the center frequency value of the 6GHz spectrum
 * for which the EIRP values are being sent.
 *
 * @QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_TX_POWER: Required u8 attribute
 * This attribute mentions the EIRP values are being sent.
 */
enum qca_wlan_vendor_attr_afc_reg_eirp_update
{
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_CENTER_FREQ,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_CHAN_NUM,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_TX_POWER,

    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_MAX =
        QCA_WLAN_VENDOR_ATTR_AFC_REG_EIRP_UPDATE_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_afc_client_type: Defines the different Regulatory client
 * types.
 */
enum qca_wlan_vendor_afc_client_type
{
    QCA_WLAN_VENDOR_AFC_CLIENT_TYPE_DEFAULT = 0,
    QCA_WLAN_VENDOR_AFC_CLIENT_TYPE_SUBORDINATE = 1,
    QCA_WLAN_VENDOR_AFC_CLIENT_TYPE_MAX,
};

/**
 * enum qca_wlan_vendor_reg_power_type: Defines the different Regulatory Power
 * types.
 */
enum qca_wlan_vendor_reg_power_type
{
    QCA_WLAN_VENDOR_REG_LOW_POWER_INDOOR = 0,
    QCA_WLAN_VENDOR_REG_STANDARD_POWER = 1,
    QCA_WLAN_VENDOR_REG_VERY_LOW_POWER = 2,
    QCA_WLAN_VENDOR_REG_MAX,
};
#endif /* CONFIG_AFC_SUPPORT */

/**
  * enum hw_status_category: Category of feature to be sent as HW status
  * through NL event
  */
enum hw_status_category
{
    QCA_WLAN_VENDOR_HW_STATUS_INVALID = 0,
    QCA_WLAN_VENDOR_HW_STATUS_RAW_MODE = 1,
};

enum qca_nl_son_reg_params
{
    QCA_NL80211_SON_REG_PARAMS_NUM_OPCLASS = 0,
    QCA_NL80211_SON_REG_PARAMS_CURR_CHAN_NUM= 1,
    QCA_NL80211_SON_REG_PARAMS_CURR_OPCLASS_TXPOWER = 2,
    QCA_NL80211_SON_REG_PARAMS_OP_CLASS_LIST = 3,
};

#if QLD
/* Flags to indication of start and end
   of data while sending vendor command */
enum qca_nl_vendor_subcmd_get_qld_entry_flags {
    QLD_DATA_START_FLAG,
    QLD_DATA_END_FLAG,
};
#endif

#if WLAN_SUPPORT_TELEMETRY
/**
 * enum qca_wlan_vendor_attr_stats: Defines attributes to be used in response of
 * QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA vendor command.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_LEVEL: Used for stats levels like Basic or
 * Advance or Debug.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_OBJECT: Required (u8)
 * Used with the command, carrying stats, to specify for which stats_object_e.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID: Used for Object ID like for STA MAC
 * address or for VAP or Radio or SoC respective interface name.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_SERVICEID: Used for sawf levels stats like per
 * peer or per peer per serviceclass.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_REQUEST_ID: Used to denote the request ID
 * of asynchronous stats request.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_PARENT_IF: Used for Parent Object interface name
 * like for STA VAP name, for VAP Radio interface name and for Radio SoC
 * interface name.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_TYPE: Used for stats types like Data or
 * control.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE: Required (NESTED Flag)
 * Used with the command to specify the nested stats.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_MULTI_REPLY: Set this flag if current reply
 * messageis holding data from previous reply.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_LINK_ID: Used to pass the link_id of the mld
 * object.
 *
 * @QCA_WLAN_VENDOR_ATTR_STATS_MAX: Defines maximum attriutes can be used in
 * QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA vendor command response.
 */
enum qca_wlan_vendor_attr_stats {
    QCA_WLAN_VENDOR_ATTR_STATS_LEVEL = 1,
    QCA_WLAN_VENDOR_ATTR_STATS_OBJECT,
    QCA_WLAN_VENDOR_ATTR_STATS_OBJ_ID,
    QCA_WLAN_VENDOR_ATTR_STATS_SERVICEID,
    QCA_WLAN_VENDOR_ATTR_STATS_REQUEST_ID,
    QCA_WLAN_VENDOR_ATTR_STATS_PARENT_IF,
    QCA_WLAN_VENDOR_ATTR_STATS_TYPE,
    QCA_WLAN_VENDOR_ATTR_STATS_RECURSIVE,
    QCA_WLAN_VENDOR_ATTR_STATS_MULTI_REPLY,
    QCA_WLAN_VENDOR_ATTR_STATS_LINK_ID,
    QCA_WLAN_VENDOR_ATTR_STATS_MAX,
};

/**
 * enum qca_wlan_vendor_attr_feat: Defines nested attributes to be used in
 * response of QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA vendor command.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_ME: Used for Multicast Enhancement stats for a
 * particular stats object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_RX: Used for Rx stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_TX: Used for Tx stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_AST: Used for AST stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_CFR: Used for CFR stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_FWD: Used for BSS stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_RAW: Used for RAW mode stats for a particular
 * object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_TSO: Used for TSO stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_TWT: Used for TWT stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_VOW: Used for VOW  stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_WDI: Used for WDI stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_WMI: Used for WMI stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_IGMP: Used for IGMP stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_LINK: Used for Link related stats for a particular
 * object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_MESH: Used for Mesh related stats for a particular
 * object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_RATE: Used for Rate stats for a particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS: Used for NAWDS related stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_DELAY: Used for DELAY related stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_JITTER: Used for JITTER related stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_TXCAP: Used for TXCAP realted stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_MONITOR: Used for MONITOR realted stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_SAWFDELAY: Used for SAWFDELAY related stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_SAWFTX: Used for SAWFTX related stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_DETER: Used for DETERMINISTIC related stats for a
 * particular object.
 *
 * @QCA_WLAN_VENDOR_ATTR_FEAT_MAX: Defines Maximum count of feature attributes.
 */
enum qca_wlan_vendor_attr_feat {
    QCA_WLAN_VENDOR_ATTR_FEAT_ME = 1,
    QCA_WLAN_VENDOR_ATTR_FEAT_RX,
    QCA_WLAN_VENDOR_ATTR_FEAT_TX,
    QCA_WLAN_VENDOR_ATTR_FEAT_AST,
    QCA_WLAN_VENDOR_ATTR_FEAT_CFR,
    QCA_WLAN_VENDOR_ATTR_FEAT_FWD,
    QCA_WLAN_VENDOR_ATTR_FEAT_RAW,
    QCA_WLAN_VENDOR_ATTR_FEAT_TSO,
    QCA_WLAN_VENDOR_ATTR_FEAT_TWT,
    QCA_WLAN_VENDOR_ATTR_FEAT_VOW,
    QCA_WLAN_VENDOR_ATTR_FEAT_WDI,
    QCA_WLAN_VENDOR_ATTR_FEAT_WMI,
    QCA_WLAN_VENDOR_ATTR_FEAT_IGMP,
    QCA_WLAN_VENDOR_ATTR_FEAT_LINK,
    QCA_WLAN_VENDOR_ATTR_FEAT_MESH,
    QCA_WLAN_VENDOR_ATTR_FEAT_RATE,
    QCA_WLAN_VENDOR_ATTR_FEAT_NAWDS,
    QCA_WLAN_VENDOR_ATTR_FEAT_DELAY,
    QCA_WLAN_VENDOR_ATTR_FEAT_JITTER,
    QCA_WLAN_VENDOR_ATTR_FEAT_TXCAP,
    QCA_WLAN_VENDOR_ATTR_FEAT_MONITOR,
    QCA_WLAN_VENDOR_ATTR_FEAT_SAWFDELAY,
    QCA_WLAN_VENDOR_ATTR_FEAT_SAWFTX,
    QCA_WLAN_VENDOR_ATTR_FEAT_DETER,
    QCA_WLAN_VENDOR_ATTR_FEAT_WMM,
    /**
     * New attribute must be add before this.
     * Also define the corresponding feature
     * index in stats_feat_index_e.
     */
    QCA_WLAN_VENDOR_ATTR_FEAT_MAX,
};

/**
 * enum qca_wlan_vendor_attr_telemetric: Defines attributes to be used in
 * request message of QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA vendor command.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_LEVEL: Defines stats levels like Basic or
 * Advance or Debug.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_OBJECT: Defines stats objects like STA or
 * VAP or Radio or SoC.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_TYPE: Defines stats types like Data or
 * control.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_INFO: Defines other parameters of stats
 * request like aggregation flag for driver agrregation, STA Resolution flag
 * and asynchronous request flag as bitmaps. Other bits are reserved.
 * Bit definitions:
 * Bit 0: Aggregation flag
 * Bit 1: STA Resolution in driver
 * Bit 2: Asynchronous request indication
 * Bit 3-7: Reserved
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_FEATURE_FLAG: Defines feature flags for
 * which stats is requested.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_STA_MAC: Defines STA MAC Address if the
 * request is for particular STA object.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_SERVICEID: Defines serviceid for sawf stats.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_MLD_LINK: Defines if stats request is for
 * interface which is part of MLD.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_PEER_TYPE: Define peer type for peer request
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_REQUEST_ID: Define request ID for non-blocking
 * stats request.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_LINK_ID: Used to pass the link_id of the mld
 * object.
 *
 * @QCA_WLAN_VENDOR_ATTR_TELEMETRIC_MAX: Defines maximum attribute counts to be
 * used in QCA_NL80211_VENDOR_SUBCMD_TELEMETRIC_DATA vendor command request.
 */
enum qca_wlan_vendor_attr_telemetric {
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_LEVEL = 1,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_OBJECT,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_TYPE,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_INFO,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_FEATURE_FLAG,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_STA_MAC,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_SERVICEID,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_MLD_LINK,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_PEER_TYPE,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_REQUEST_ID,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_LINK_ID,
    QCA_WLAN_VENDOR_ATTR_TELEMETRIC_MAX,
};

#endif /* WLAN_SUPPORT_TELEMETRY */

/**
 * enum qca_wlan_vendor_attr_mlo_link_removal: Defines attributes to be used in
 * QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_REMOVAL vendor command
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_REMOVAL_TBTT: Defines the AP removal timer
 * TBTT value to be used as part of the MLO link removal
 */
enum qca_wlan_vendor_attr_mlo_link_removal {
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_REMOVAL_TBTT = 1,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_REMOVAL_MAX,
};

/**
 * enum qca_wlan_vendor_attr_mlo_link_btm: Defines attributes to be used in
 * request message of QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_BTMREQ. The
 * attributes specify the contents of the BTM request that the AP will
 * send out
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_MAC: Destination to which BTM request
 * is to be sent
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_DISASSOC: Number of TBTT intervals
 * after which AP will send disassoc
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_BSSTERM: TSF time at which the BSS
 * will be terminated
 */
enum qca_wlan_vendor_attr_mlo_link_btm {
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_MAC = 1,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_DISASSOC = 2,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_BSSTERM = 3,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_BTM_MAX,
};

/**
 * enum qca_wlan_vendor_attr_app_generic_category: Represents the Generic
 * mapping frame category value.
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_INVALID: Generic mapping catefory
 * invalid.
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_APP_INIT: The driver includes this
 * category in the event  sent to the userspace when it receives a APP INIT
 * request frame.
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_LINK_BW_NSS_CHANGE: Represents the
 * notification message that will be sent to the RM APP for changes observed
 * in the BW and NSS values at AP side. Array of nested attributes are defined
 * in enum qca_wlan_vendor_attr_link_bw_nss_change_info
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO: The driver
 * includes this category in the event sent to the userspace when it receives a
 * Assoc request frame from the STA without T2LM IE.
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_WITH_T2LM_INFO: The driver
 * includes this category in the event sent to the userspace when it receives a
 * Assoc request frame from the STA with T2LM IE.
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO: The driver includes
 * this category in the event sent to the userspace when it receives a Operating
 * mode change notification from the STA.
 *
 * @QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_DISASSOC: The driver includes this
 * category in the event sent to the userspace when it receives a disassoc from
 * the connected STA.
 *
 */
enum qca_wlan_vendor_attr_app_generic_category {
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_APP_INIT = 1,
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_LINK_BW_NSS_CHANGE = 2,
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO = 3,
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_WITH_T2LM_INFO = 4,
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO = 5,
    QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_DISASSOC = 6,
};

#ifdef WLAN_FEATURE_11BE
#define CURRENT_T2LM_APP_VERSION 1
#define MAX_T2LM_LINKS 3
#define T2LM_EVENT_BUFFER 500
#define MAX_TLT_NUM_LINKS 6

#define QCA_NL80211_VENDOR_SUBCMD_RM_GENERIC 206
#define QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY 207
#define QCA_NL80211_VENDOR_SUBCMD_QCOM_HW_STATUS 209

/**
 * enum qca_wlan_vendor_attr_rm_generic - Represents the attributes required
 * to register a Resource Manager (RM) with the driver.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_APP_VERSION: u8, represents the current version of
 * Resource Manager Application. The application sends this to the driver during initial
 * registration handshake command. Driver prints this value on the console for
 * the debug purpose.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_DRIVER_VERSION: u8, represents the current version
 * of the driver. The driver sends this to the userspace in the registration event.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_NUM_SOC_DEVICES: u8, represents the number of
 * SOC devices available in the driver. The driver sends this to the userspace
 * in the registration event.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SOC_DEVICE_INFO: Represents SOC device information
 * available in the driver. The driver sends this to the userspace in the
 * registration event. Array of nested attributes are defined in enum
 * qca_wlan_vendor_attr_soc_device_info.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_TTLM_MAPPING: Represents the TID-to-link
 * mapping configuration supported by the driver. The driver sends this to the
 * userspace in the registration event. Values are defined in enum
 * qca_wlan_vendor_t2lm_mapping.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_RELAYFS_FILE_NAME_PMLO: string, represents relayfs
 * filename used for PMLO feature. The driver sends this to userspace to tell what
 * file to listen on for statistics.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_LINK_BW_NSS_CHANGE: Represents the
 * notification message that will be sent to the userspace for changes observed in
 * the BW and NSS values at AP side. Array of nested attributes are defined in
 * enum qca_wlan_vendor_attr_link_bw_nss_change_info
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_RELAYFS_FILE_NAME_DETSCHED: string, represents relayfs
 * filename used for Deterministic scheduler. The driver sends this to userspace to tell what
 * file to listen on for statistics.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_CATEGORY: Represents the RM Generic message
 * category. Values are defined in enum qca_wlan_vendor_attr_rm_generic
 * This is included in the commands sent from the userspace to the driver and
 * used in the events sent from the driver to the userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ASSOC_NUM_LINKS: u8, represents the number of
 * MLO peer links are included in the commands sent from the userspace to the
 * driver and in the events sent from the driver to the usersapce.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ASSOC_PEER_LINK_ENTRY: Represents the
 * information of each MLO peer link. This is included in the commands sent by
 * the userspace to the driver and in the events sent by the driver to the
 * userspace. Each MLO link has list of attributes specified in
 * enum qca_wlan_vendor_attr_t2lm_mlo_peer_link_info.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ASSOC_TTLM_INFO: Represents the TID-to-link
 * mapping of all the TIDs for the frames transmitted in downlink, uplink and
 * bidirectional. The driver includes this when sending Assoc event with
 * TTLM mapping to user space.
 * A list of attributes required for downlink, uplink, BiDi is listed in
 * enum qca_wlan_vendor_attr_t2lm_info.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ERP: Nested attribute used for commands
 * and events related to ErP, see @enum qca_wlan_vendor_attr_erp_ath
 * for details.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_MLD_MAC_ADDR: 6 bytes MAC address represents the
 * peer MLD mac address. This is included in the commands sent from the
 * userspace to the driver and used in the events sent from the driver to the
 * userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SERVICE_ID: Represents the service id
 * for which RM GENERIC sent/received.
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SERVICE_DATA: Represents the service specific
 * data sent during app init by the service with service id present in
 * QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SERVICE_ID
 *
 * @QCA_WLAN_VENDOR_ATTR_RM_GENERIC_DYNAMIC_INIT_CONF: u8, Represents if the
 * service is dynamically loaded/unloaded or not. All the valid values for
 * QCA_WLAN_VENDOR_ATTR_RM_APP_DYNAMIC_INIT_CONF are defined in enum
 * qca_wlan_vendor_dynamic_init_conf.
 */
enum qca_wlan_vendor_attr_rm_generic {
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_APP_VERSION = 1,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_DRIVER_VERSION = 2,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_NUM_SOC_DEVICES = 3,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SOC_DEVICE_INFO = 4,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_TTLM_MAPPING = 5,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_RELAYFS_FILE_NAME_PMLO = 6,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_LINK_BW_NSS_CHANGE = 7,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_RELAYFS_FILE_NAME_DETSCHED = 8,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_CATEGORY = 9,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ASSOC_NUM_LINKS = 10,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ASSOC_PEER_LINK_ENTRY = 11,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ASSOC_TTLM_INFO = 12,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_ERP = 13,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_MLD_MAC_ADDR = 14,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SERVICE_ID = 15,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_SERVICE_DATA = 16,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_DYNAMIC_INIT_CONF = 17,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_RM_GENERIC_MAX =
        QCA_WLAN_VENDOR_ATTR_RM_GENERIC_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_soc_device_info - Represents the SOC device
 * information available in the driver. The driver will send this information
 * to the userspace as part of the registration event.
 *
 * @QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_SOC_ID: u8, represents the SOC device ID.
 *
 * @QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_NUM_LINKS: u8, represents the number of
 * links present in the SOC device.
 *
 * @QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_LINK_INFO: represents the link level
 * information. Array of nested attributes are defined in enum
 * qca_wlan_vendor_attr_link_info
 */
enum qca_wlan_vendor_attr_soc_device_info {
    QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_SOC_ID = 1,
    QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_NUM_LINKS = 2,
    QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_LINK_INFO = 3,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_INFO_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_INFO_MAX =
        QCA_WLAN_VENDOR_ATTR_SOC_DEVICE_INFO_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_link_bw_nss_change_info - Conatins info
 * about the change in BW and NSS values of a link.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_HW_LINK_ID: u16, represents the hardware link
 * ID.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_NEW_CHAN_BW: u8, represents the updated channel
 * bandwidth, values are defined in enum qca_wlan_vendor_channel_width.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_NEW_CHAN_NSS: u8, represents the updated link
 * NSS value.
 */
enum qca_wlan_vendor_attr_link_bw_nss_change_info {
    QCA_WLAN_VENDOR_ATTR_LINK_HW_LINK_ID = 1,
    QCA_WLAN_VENDOR_ATTR_LINK_NEW_CHAN_BW = 2,
    QCA_WLAN_VENDOR_ATTR_LINK_NEW_CHAN_NSS = 3,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LINK_CHANGE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LINK_CHANGE_MAX =
        QCA_WLAN_VENDOR_ATTR_LINK_CHANGE_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_link_info - Represents the link level information
 * available in the driver. The driver will send this information to the
 * userspace as part of the registration event.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_INFO_HW_LINK_ID: u16, represents the hardware link
 * ID.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_MAC: 6 byte MAC address represents the Link
 * MAC address.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_CHAN_BW: u8, represents the channel bandwidth,
 * values are defined in enum qca_wlan_vendor_channel_width
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_CHAN_FREQ: u16, represents the channel frequency
 * in MHz.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_BAND_CAP: Channel band capability, values are
 * defined in enum qca_wlan_vendor_link_band_caps
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_TX_CHAIN_MASK: u8, represents the max tx chainmask
 * value.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_RX_CHAIN_MASK: u8, represents the max rx chainmask
 * value.
 */
enum qca_wlan_vendor_attr_link_info {
    QCA_WLAN_VENDOR_ATTR_LINK_INFO_HW_LINK_ID = 1,
    QCA_WLAN_VENDOR_ATTR_LINK_MAC = 2,
    QCA_WLAN_VENDOR_ATTR_LINK_CHAN_BW = 3,
    QCA_WLAN_VENDOR_ATTR_LINK_CHAN_FREQ = 4,
    QCA_WLAN_VENDOR_ATTR_LINK_BAND_CAP = 5,
    QCA_WLAN_VENDOR_ATTR_LINK_TX_CHAIN_MASK = 6,
    QCA_WLAN_VENDOR_ATTR_LINK_RX_CHAIN_MASK = 7,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LINK_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LINK_MAX =
        QCA_WLAN_VENDOR_ATTR_LINK_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_mlo_hw_status_event - Defines the attributes
 * used in the QCA_NL80211_VENDOR_SUBCMD_QCOM_EVENT_HW_STATUS subcommand.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_CATEGORY: u8 attribute,
 * category of the event for HW status
 * @QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_MACADDR: 6 byte MAC address
 * of the MLD peer
 * @QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_LINK_MACADDR: 6 byte MAC address
 * of the link peer
 * @QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_PARAM: u8 attribute,
 * used to indicate parameters of the event
 */
enum qca_wlan_vendor_attr_mlo_hw_status_event {
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_CATEGORY = 1,
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_MACADDR = 2,
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_LINK_MACADDR = 3,
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_PARAM = 4,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_MAX =
	QCA_WLAN_VENDOR_ATTR_MLO_HW_STATUS_EVENT_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_channel_width - Represents the channel bandwidth in MHz
 * available in the driver. The driver will send this information to the
 * userspace as part of the registration event.
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_INVALID: Invalid channel bandwidth
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_20MHZ: 20 MHz channel bandwidth
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_40MHZ: 40 MHz channel bandwidth
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_80MHZ: 80 MHz channel bandwidth
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_160MZ: 160 MHz channel bandwidth
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_80_80MHZ: 80+80 MHz channel bandwidth
 *
 * @QCA_WLAN_VENDOR_CHAN_WIDTH_320MHZ: 320 MHz channel bandwidth
 */
enum qca_wlan_vendor_channel_width {
    QCA_WLAN_VENDOR_CHAN_WIDTH_INVALID = 0,
    QCA_WLAN_VENDOR_CHAN_WIDTH_20MHZ = 1,
    QCA_WLAN_VENDOR_CHAN_WIDTH_40MHZ = 2,
    QCA_WLAN_VENDOR_CHAN_WIDTH_80MHZ = 3,
    QCA_WLAN_VENDOR_CHAN_WIDTH_160MZ = 4,
    QCA_WLAN_VENDOR_CHAN_WIDTH_80_80MHZ = 5,
    QCA_WLAN_VENDOR_CHAN_WIDTH_320MHZ = 6,
};

/**
 * enum qca_wlan_vendor_link_band_caps - Represents the band capability of
 * a link. The driver will send this information to the userspace as part of the
 * registration event.
 *
 * @QCA_WLAN_VENDOR_LINK_BAND_INVALID: Invalid band
 * @QCA_WLAN_VENDOR_LINK_BAND_2GHZ: 2GHz link
 * @QCA_WLAN_VENDOR_LINK_BAND_5GHZ: 5GHz link
 * @QCA_WLAN_VENDOR_LINK_BAND_5GHZ_LOW: 5GHz Low band link
 * @QCA_WLAN_VENDOR_LINK_BAND_5GHZ_HIGH: 5GHz High band link
 * @QCA_WLAN_VENDOR_LINK_BAND_6GHZ: 6GHz link
 * @QCA_WLAN_VENDOR_LINK_BAND_6GHZ_LOW: 6GHz Low band link
 * @QCA_WLAN_VENDOR_LINK_BAND_6GHZ_HIGH: 6GHz High band link
 */
enum qca_wlan_vendor_link_band_caps {
    QCA_WLAN_VENDOR_LINK_BAND_INVALID = 0,
    QCA_WLAN_VENDOR_LINK_BAND_2GHZ = 1,
    QCA_WLAN_VENDOR_LINK_BAND_5GHZ = 2,
    QCA_WLAN_VENDOR_LINK_BAND_5GHZ_LOW = 3,
    QCA_WLAN_VENDOR_LINK_BAND_5GHZ_HIGH = 4,
    QCA_WLAN_VENDOR_LINK_BAND_6GHZ = 5,
    QCA_WLAN_VENDOR_LINK_BAND_6GHZ_LOW = 6,
    QCA_WLAN_VENDOR_LINK_BAND_6GHZ_HIGH = 7,
};

/**
 * enum qca_wlan_vendor_dynamic_init_conf - This enum defines the different
 * dynamic app init/deinit configurations.
 *
 * @QCA_WLAN_VENDOR_DYNAMIC_INIT_CONF_RM_APP_START: Indicates to driver this
 * is the initial app init and not an individual service dynamic init/de-init.
 *
 * @QCA_WLAN_VENDOR_DYNAMIC_INIT_CONF_SERVICE_START: Indicates to driver this
 * is dynamic service init/start.
 *
 * @QCA_WLAN_VENDOR_DYNAMIC_INIT_CONF_SERVICE_STOP: Indicates to driver this
 * is dynamic service de-init/stop.
 */
enum qca_wlan_vendor_dynamic_init_conf {
    QCA_WLAN_VENDOR_DYNAMIC_INIT_CONF_RM_APP_START = 0,
    QCA_WLAN_VENDOR_DYNAMIC_INIT_CONF_SERVICE_START = 1,
    QCA_WLAN_VENDOR_DYNAMIC_INIT_CONF_SERVICE_STOP = 2,
};

/**
 * enum qca_wlan_vendor_t2lm_mapping - Represents the TID-to-link mapping (T2LM)
 * supported value. The driver will send this information to the userspace as
 * part of the registration event.
 *
 * @QCA_WLAN_VENDOR_T2LM_INVALID: Invalid value.
 *
 * @QCA_WLAN_VENDOR_T2LM_NOT_SUPPORTED: T2LM is not supported
 *
 * @QCA_WLAN_VENDOR_T2LM_MAP_EACH_TID_TO_SAME_OR_DIFFERENT_LINK_SET: Map
 * each TID to same or different link set.
 *
 * @QCA_WLAN_VENDOR_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET: Map all the TIDs to
 * same link set.
 *
 */
enum qca_wlan_vendor_t2lm_mapping {
    QCA_WLAN_VENDOR_T2LM_INVALID = 0,
    QCA_WLAN_VENDOR_T2LM_NOT_SUPPORTED = 1,
    QCA_WLAN_VENDOR_T2LM_MAP_EACH_TID_TO_SAME_OR_DIFFERENT_LINK_SET = 2,
    QCA_WLAN_VENDOR_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET = 3,
};

/**
 * enum qca_wlan_vendor_attr_t2lm_policy - Represents,
 * 1. The attributes required to send T2LM request, T2LM teardown and BTM
 *    request commands from the userspace to the driver.
 * 2. The attributes required to send T2LM request, T2LM response, T2LM
 *    teardown, disassoc, BTM response and MLO peer information during
 *    (Re)Assoc, MLO peer operation mode change and Tx completion events
 *    from the driver to the userspace.
 * 3. The attributes required to send the T2LM response from the userspace to
 *    to the driver for the received T2LM request event from the driver.
 * These attributes are used in QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MLD_MAC: 6 bytes MAC address represents the
 * peer MLD mac address. This is included in the commands sent from the
 * userspace to the driver and used in the events sent from the driver to the
 * userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_CATEGORY: Represents the provisioning
 * category. Values are defined in enum qca_wlan_vendor_t2lm_category.
 * This is included in the commands sent from the userspace to the driver and
 * used in the events sent from the driver to the userspace.
 *
* @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_NUM_LINKS: u8, represents the number of
 * MLO peer links are included in the commands sent from the userspace to the
 * driver and in the events sent from the driver to the usersapce.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MLO_PEER_LINK_ENTRY: Represents the
 * information of each MLO peer link. This is included in the commands sent by
 * the userspace to the driver and in the events sent by the driver to the
 * userspace. Each MLO link has list of attributes specified in
 * enum qca_wlan_vendor_attr_t2lm_mlo_peer_link_info.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_T2LM_INFO: Represents the TID-to-link
 * mapping of all the TIDs for the frames transmitted in downlink, uplink and
 * bidirectional. This is included in the T2LM request and T2LM response event
 * sent from the driver to the userspace and in the T2LM request and T2LM
 * response command sent from the userspace to the driver.
 * A list of attributes required for downlink, uplink, BiDi is listed in
 * enum qca_wlan_vendor_attr_t2lm_info.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_TX_STATUS: Represents the status of the
 * transmitted T2LM frame (request/response/teardown) that was received from
 * the userspace to the driver. These status codes are sent by the driver to
 * the usersapce and these are defined in enum qca_wlan_vendor_t2lm_tx_status.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MAPPING: Represents TID-to-link mapping
 * configuration supported by the STA. The driver sends this value to the
 * userspace as part of T2LM request event. The values are defined in enum
 * qca_wlan_vendor_t2lm_mapping.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_RESPONSE_TYPE: Represents status value
 * of the TID-to-link mapping response frame. The driver includes this attribute
 * in the event sent to the userspace when it receives a T2LM response frame
 * from the STA. The userspace includes this attribute in the command sent to
 * the driver for a received T2LM request event from the driver.
 * The Response status values are defined in enum
 * qca_wlan_vendor_t2lm_response_frame_type.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_LINK_QUALIFIER: Represents link preference
 * to be set when tids are mapped to more than one link for the connected STA.
 * This may be included in T2LM request and teardown commands and
 * must be included in Link Preference command sent from user space to
 * the driver. The attribute values are defined in
 * enum qca_wlan_vendor_attr_link_qualifier.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_BTM_REQ_INFO: Represents link
 * reommendation information to be set when trying to provision the clients
 * to certain subset of links using BTM framework. This information will be used
 * by the driver to form corresponding BTM request to be sent to the client.
 * The attribute values are defined in enum qca_wlan_vendor_attr_btm_reqinfo.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_DIALOG_TOKEN: Represents dialog token
 * value to be sent from the driver to the userspace on T2LM resquest and
 * receive from userspace on T2LM response.
 */
enum qca_wlan_vendor_attr_t2lm_policy {
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MLD_MAC = 1,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_CATEGORY = 2,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_NUM_LINKS = 3,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MLO_PEER_LINK_ENTRY = 4,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_T2LM_INFO = 5,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_TX_STATUS = 6,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MAPPING = 7,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_RESPONSE_TYPE = 8,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_LINK_QUALIFIER = 9,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_BTM_REQ_INFO = 10,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_DIALOG_TOKEN = 11,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_MAX =
        QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_btm_reqinfo - Represents the attributes to be
 * added under the nested attribute
 * QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_BTM_REQ_INFO when the
 * QCA_WLAN_VENDOR_ATTR_T2LM_POLICY_CATEGORY is
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_BTM_REQUEST
 *
 * @QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_DISASSOC_IMMINENT: u8, Required attribute.
 * This attribute denotes the value of Disassociation Imminent field to be
 * set in the BTM request when trying to provision a client.
 *
 * @QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_DISASSOC_TIMER: u16, Required attribute.
 * This attribute denotes the value of Disassociation timer field to be set in
 * the BTM request frame when trying to provision a client.
 *
 * @QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_DISASSOC_ENFORCE: u8, Required attribute.
 * This attribute signals to the driver whether the client should be actually
 * dis-associated from the AP after the expiry of the Disassociation timer.
 *
 * @QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_RECOMM_LINK_ID: Nested optional attribute.
 * This attribute will have an array of uint8 denoting link IDs of all the link
 * that are to be recommended for the client to be provisioned in. This
 * attribute will be present only when AP is trying to provision the client on
 * a subset of connected links. In case of moving the client to AMLMR from
 * provisioned mode, this attribute will not be present.
 *
 * @QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_LINK_ID_PREF: Nested optional attribute.
 * This attribute will have an array of uint8 denoting the preference of the
 * link IDs denoted by QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_RECOMM_LINK_ID. This
 * attribute will be present only when the attribute
 * QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_RECOMM_LINK_ID is included.
 */
enum qca_wlan_vendor_attr_btm_reqinfo{
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_DISASSOC_IMMINENT = 1,
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_DISASSOC_TIMER = 2,
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_DISASSOC_ENFORCE = 3,
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_RECOMM_LINK_ID = 4,
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_LINK_ID_PREF = 5,

    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_MAX =
        QCA_WLAN_VENDOR_ATTR_BTM_REQINFO_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_t2lm_category - Represents the provisioning category
 * value.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_INVALID: TID-to-link mapping category invalid.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE: The driver includes this category in
 * the event sent to the userspace when it includes the Tx status value for the
 * T2LM transmitted frame.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST: The driver includes this category
 * in the event sent to the userspace when it receives a T2LM request frame from
 * the STA or Assoc request was received with the T2LM IE from the STA.
 * Userspace includes this category in the command sent to the driver when user-
 * space wants to trigger the T2LM request frame.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE: The driver includes this category in
 * the event sent to the userspace when it receives a T2LM response frame from
 * the STA. Userspace includes this category in the command sent to the driver
 * as part of T2LM response for a received T2LM request from the driver.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_TEARDOWN: The driver includes this category in
 * the event sent to the userspace when it receives a T2LM teardown frame from
 * the STA. Userspace includes this category in the command sent to the driver
 * if it wants to teardown the already established TID-to-link mapping.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_LINK_QUALIFIER: The userspace includes this
 * category in the command send to the driver to set the link preference for a
 * connected STA without provisioning the STA.
 *
 * @QCA_WLAN_VENDOR_T2LM_CATEGORY_BTM_REQUEST: The userspace includes this
 * category in the command sent to the driver to try and provision clients
 * using BTM request.
 */
enum qca_wlan_vendor_t2lm_category {
    QCA_WLAN_VENDOR_T2LM_CATEGORY_INVALID = 0,
    QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE = 1,
    QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST = 2,
    QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE = 3,
    QCA_WLAN_VENDOR_T2LM_CATEGORY_TEARDOWN = 4,
    QCA_WLAN_VENDOR_T2LM_CATEGORY_LINK_QUALIFIER = 5,
    QCA_WLAN_VENDOR_T2LM_CATEGORY_BTM_REQUEST = 6,
};

/**
 * enum qca_wlan_vendor_attr_t2lm_mlo_peer_link_info - Represents the MLO peer
 * link inforamtion.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_HW_LINK_ID: u16, represents the hardware
 * link id of the MLO peer link. This is included in the commands sent from the
 * userspace to the driver and used in the events sent from the driver to the
 * userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_PEER_MAC: 6 byte MAC address represents
 * the MLO peer mac address. This is included in the commands sent from the
 * userspace to the driver and used in the events sent from the driver to the
 * userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_MLO_LINK_ID: u8, represents the mlo peer
 * link index. This is included in the commands sent from the userspace to the
 * driver and used in the events sent from the driver to the userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_IS_ASSOC_LINK: u8, this is included in
 * the event sent from the driver to the userspace to identify the Assoc request
 * received link. Userspace includes this in all the commands sent to the driver
 * to identify the Assoc request received list.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_CHAN_BW: u8, this is included in the
 * event sent from the driver to the userspace to indicate the STA's channel
 * bandwidth. The values are defined in enum qca_wlan_vendor_channel_width.
 * The driver includes this attribute in the event sent for
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_CHAN_FREQ: u16, this is included in the
 * event sent from the driver to the userspace to indicate the STA's channel
 * frequency in MHz. The driver includes this attribute in the event sent for
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_AVAILABLE_AIRTIME: u16, this is included
 * in the event sent from the driver to the userspace to indicate MLO peer
 * link's available airtime value (unit is percentage). The driver includes this
 * attribute in the event sent for QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_RSSI: s8, this is included in the event
 * sent from the driver to the userspace to indicate MLO peer link's (Assoc
 * request received link) RSSI value in dBm. The driver includes this attribute
 * in the event sent for
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_EHT_CAPS: This is included in the event
 * sent from the driver to the userspace to indicate MLO peer link's EHT
 * capabilities. Values are defined in enum
 * qca_wlan_vendor_attr_eht_peer_capabilities.
 * The driver includes this attribute in the event sent for
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_BAND_CAP: This is included in the event
 * sent from the driver to the userspace to indicate MLO peer link's band
 * capabilities. Values are defined in enum qca_wlan_vendor_link_band_caps.
 * The driver includes this attribute in the event sent for
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_VDEV_ID: u8, represents the vdev id.
 * This is included in the commands sent from the userspace to the driver
 * and used in the events sent from the driver to the userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_AP_MLD_MAC: 6 byte MAC address represents
 * the vdev mld mac address.
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_CAPS: u16, represents peer capabilities.
 */
enum qca_wlan_vendor_attr_t2lm_mlo_peer_link_info {
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_HW_LINK_ID = 1,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_PEER_MAC = 2,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_MLO_LINK_ID = 3,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_IS_ASSOC_LINK = 4,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_CHAN_BW = 5,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_CHAN_FREQ = 6,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_AVAILABLE_AIRTIME = 7,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_RSSI = 8,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_EHT_PEER_CAPS = 9,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_BAND_CAP = 10,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_EFF_CHAN_BW = 11,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_VDEV_ID = 12,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_AP_MLD_MAC = 13,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_CAPS = 14,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_MAX =
        QCA_WLAN_VENDOR_ATTR_MLO_PEER_LINK_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_eht_peer_capabilities -  Represents the EHT
 * capabilities of the link MLO peer link.
 *
 * @QCA_WLAN_VENDOR_ATTR_EHTCAP_RX_MCS_NSS_MAP: An array of 4 x u32 value,
 * represents the MCS NSS MAP for index values defined in enum
 * qca_wlan_vendor_eht_per_bw_txrx_mcs_nss_map_idx.
 *
 * @QCA_WLAN_VENDOR_ATTR_EHTCAP_TX_MCS_NSS_MAP: An array of 4 x u32 value,
 * represents the MCS NSS MAP for index values defined in enum
 * qca_wlan_vendor_eht_per_bw_txrx_mcs_nss_map_idx.
 */
enum qca_wlan_vendor_attr_eht_peer_capabilities {
    QCA_WLAN_VENDOR_ATTR_EHTCAP_RX_MCS_NSS_MAP = 1,
    QCA_WLAN_VENDOR_ATTR_EHTCAP_TX_MCS_NSS_MAP = 2,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_EHTCAP_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_EHTCAP_MAX =
        QCA_WLAN_VENDOR_ATTR_EHTCAP_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_eht_per_bw_txrx_mcs_nss_map_idx - Represents the EHTCAP
 * TXRX MCS NSS map index values
 * @QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_ONLY20: MCS NSS value for 20MHz
 * bandwidth.
 *
 * @QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_80: MCS NSS value for 40MHz and
 * 80MHz bandwidth.
 *
 * @QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_160: MCS NSS value for 160MHz
 * bandwidth.
 *
 * @QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_320: MCS NSS value for 320MHz
 * bandwidth.
 *
 * @QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_MAX: MAX index value.
 */
enum qca_wlan_vendor_eht_per_bw_txrx_mcs_nss_map_idx {
    QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_ONLY20 = 1,
    QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_80 = 2,
    QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_160 = 3,
    QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_320 = 4,
    QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_MAX = 4,
};

/**
 * enum qca_wlan_vendor_attr_link_qualifier - Represents the link preference
 * information set for all the links in MLD.
 * The userspace includes this in the command sent to the driver as part of
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_TEARDOWN and
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_LINK_QUALIFIER.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_LINKS: u8, represents the number of
 * links for which preference is being set.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_ORDER: u8, An array of MAX_PREFERRED_LINKS x u8 value,
 * containing the HW link IDs for links in order of preference
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_TIMEOUT: An array of MAX_PREFERRED_LINKS x u32 value,
 * represents the timeout in milliseconds to be maintained for a
 * preferred link by FW, before moving on to next preferred link.
 * Expected timeout to be estimated in Firmware if zero
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_TLT_BITMAP: u32, represents the link bitmap
 * for creation of TLT. This attribute is included in the command sent from
 * the userspace to the driver.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_FLAGS: u32, represents the LQ specific
 * flags. Bit definition for the flags.
 * Bit 0: TLT bit
 * Bit 1: Preferred Link bit
 * Bit 2-31: Reserved
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_MAC_ADDR: u8, represent the STA mld mac addr
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_T2LM: u8, non-zero value indicates TID-to-link mapping
 * It indicates for how many directions, the TID-to-link mapping is present.
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_HOMOGENEOUS_MAPPING: i8, non-zero value indicates
 * the provided mapping is homogeneous
 *
 * @QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_T2LM_OF_TIDS: An structure represent Valid TID-to-link
 * mapping for the directions
 *
 */
enum qca_wlan_vendor_attr_link_qualifier {
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_LINKS = 1,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_ORDER = 2,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_TIMEOUT = 3,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_TLT_BITMAP = 4,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_FLAGS = 5,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_MAC_ADDR = 6,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_NUM_T2LM = 7,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_HOMOGENEOUS_MAPPING = 8,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_T2LM_OF_TIDS = 9,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_MAX =
        QCA_WLAN_VENDOR_ATTR_LINK_QUALIFIER_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_t2lm_info - Represents the TID-to-link mapping
 * information for all the TIDS in uplink, downlink and bidirection.
 * The driver includes this in the event sent to the userspace as part of
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST and
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE.
 * The userspace includes this in the command sent to the driver as part of
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST and
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_DIRECTION: represents the direction for which
 * the TID-to-link mapping is applicable. Values are listed in enum
 * qca_wlan_vendor_t2lm_direction.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_DEFAULT_LINK_MAPPING: u8, represents default TID-
 * to-link mapping is used. Provisioned mapping will not be present when default
 * mapping is set.
 *
 * @QCA_WLAN_VENDOR_ATTR_T2LM_PROVISIONED_LINKS: An array of 8 x u16 value,
 * represents the link mapping for each TID. This is valid when default mapping
 * set set to zero.
 */
enum qca_wlan_vendor_attr_t2lm_info {
    QCA_WLAN_VENDOR_ATTR_T2LM_INFO_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_T2LM_DIRECTION = 1,
    QCA_WLAN_VENDOR_ATTR_T2LM_DEFAULT_LINK_MAPPING = 2,
    QCA_WLAN_VENDOR_ATTR_T2LM_PROVISIONED_LINKS = 3,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_T2LM_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_T2LM_MAX =
        QCA_WLAN_VENDOR_ATTR_T2LM_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_qualifier_t2lm_of_tids - Represents the TID-to-link mapping
 * information for all the TIDS in uplink, downlink and bidirection.
 * The driver includes this in the event sent to the userspace as part of
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST and
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE.
 * The userspace includes this in the command sent to the driver as part of
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST and
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE.$
 *
 * @QCA_WLAN_VENDOR_ATTR_QUALIFIER_DIRECTION: represents the direction for which
 * the TID-to-link mapping is applicable. Values are listed in enum
 * qca_wlan_vendor_t2lm_direction.
 *
 * @QCA_WLAN_VENDOR_ATTR_QUALIFIER_PROVISIONED_LINKS: An array of 8 x u16 value,
 * represents the link mapping for each TID.
 */
enum qca_wlan_vendor_attr_qualifier_t2lm_of_tids {
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_DIRECTION = 1,
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_PROVISIONED_LINKS = 2,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_T2LM_OF_TIDS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_T2LM_OF_TIDS_MAX =
        QCA_WLAN_VENDOR_ATTR_QUALIFIER_T2LM_OF_TIDS_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_qualifier_order - Represent the HW link IDs
 * for links in order of preference
 *
 * @QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER: A bitmap of preferred link to
 * HW link IDs mapping
 */
enum qca_wlan_vendor_attr_qualifier_order{
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER = 1,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER_MAX =
        QCA_WLAN_VENDOR_ATTR_QUALIFIER_ORDER_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_timeout - Represents the timeout in milliseconds
 * to be maintained for a preferred link by FW
 *
 * @QCA_WLAN_VENDOR_ATTR_TIMEOUT: A bitmap of preferred link to timeout mapping
 */
enum qca_wlan_vendor_attr_timeout{
    QCA_WLAN_VENDOR_ATTR_TIMEOUT = 1,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TIMEOUT_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TIMEOUT_MAX =
        QCA_WLAN_VENDOR_ATTR_TIMEOUT_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_tid_map -  Represents the hw link
 * map of a TID.
 *
 * @QCA_WLAN_VENDOR_ATTR_TID_MAP: A bitmap of links for TID mapping
 */
enum qca_wlan_vendor_attr_tid_map {
    QCA_WLAN_VENDOR_ATTR_TID_MAP = 1,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TID_MAP_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TID_MAP_MAX =
        QCA_WLAN_VENDOR_ATTR_TID_MAP_AFTER_LAST - 1,
};
/**
 * enum qca_wlan_vendor_t2lm_direction - Represends the direction used for
 * TID-to-link mapping.
 *
 * @QCA_WLAN_VENDOR_T2LM_INVALID_DIRECTION: Represents the invalid direction.
 *
 * @QCA_WLAN_VENDOR_T2LM_DOWNLINK_DIRECTION: Represents the TID-to-link mapping
 * used for the downlink direction.
 *
 * @QCA_WLAN_VENDOR_T2LM_UPLINK_DIRECTION: Represents the TID-to-link mapping
 * used for the uplink direction.
 *
 * @QCA_WLAN_VENDOR_T2LM_BIDI_DIRECTION: Represents the TID-to-link mapping used
 * for both the directions.
 *
 * @QCA_WLAN_VENDOR_T2LM_MAX_VALID_DIRECTION: Represents the value same as
 * QCA_WLAN_VENDOR_T2LM_BIDI_DIRECTION.
 */
enum qca_wlan_vendor_t2lm_direction {
    QCA_WLAN_VENDOR_T2LM_INVALID_DIRECTION = 0,
    QCA_WLAN_VENDOR_T2LM_DOWNLINK_DIRECTION = 1,
    QCA_WLAN_VENDOR_T2LM_UPLINK_DIRECTION = 2,
    QCA_WLAN_VENDOR_T2LM_BIDI_DIRECTION = 3,
    QCA_WLAN_VENDOR_T2LM_MAX_VALID_DIRECTION =
        QCA_WLAN_VENDOR_T2LM_BIDI_DIRECTION,
};

/**
 * enum qca_wlan_vendor_t2lm_tx_status - Represents the status of the
 * transmitted TID-to-link mapping request or response frame.
 *
 * @QCA_WLAN_VENDOR_T2LM_TX_STATUS_INVALID: Represents the invalid status code.
 *
 * @QCA_WLAN_VENDOR_T2LM_TX_STATUS_NONE: Status code none. This value will be
 * set in the commands sent from the userspace to the driver and in the events
 * sent from the driver to the userspace with
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST,
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE,
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_TEARDOWN,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO,
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO, and
 * QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_DISASSOC.
 *
 * @QCA_WLAN_VENDOR_T2LM_TX_STATUS_SUCCESS: Diver includes this value in the
 * event sent to the userspace with QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE to
 * indicate successful transmission of the TID-to-link mapping request or
 * response frame.
 *
 * @QCA_WLAN_VENDOR_T2LM_TX_STATUS_FAILURE: Diver includes this value in the
 * event sent to the userspace with QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE to
 * indicate transmission of TID-to-link mapping request or response frame as
 * failed.
 *
 * @QCA_WLAN_VENDOR_T2LM_TX_STATUS_RX_TIMEOUT: Diver includes this value in the
 * event sent to the userspace with QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE to
 * indicate that no response is received from the STA for the successfully
 * transmitted TID-to-link mapping request or response frame.
 */
enum qca_wlan_vendor_t2lm_tx_status {
    QCA_WLAN_VENDOR_T2LM_TX_STATUS_INVALID = 0,
    QCA_WLAN_VENDOR_T2LM_TX_STATUS_NONE = 1,
    QCA_WLAN_VENDOR_T2LM_TX_STATUS_SUCCESS = 2,
    QCA_WLAN_VENDOR_T2LM_TX_STATUS_FAILURE = 3,
    QCA_WLAN_VENDOR_T2LM_TX_STATUS_RX_TIMEOUT = 4,
};

/**
 * enum qca_wlan_vendor_t2lm_response_frame_type - Represents the TID-to-link
 * mapping response frame status. Driver includes this in the event sent to the
 * userspace as part of QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE.
 * Userspace includes this in the command/response sent to the driver as part of
 * QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE.
 *
 * @QCA_WLAN_VENDOR_T2LM_RESPONSE_FRAME_STATUS_INVALID: Status is invalid.
 *
 * @QCA_WLAN_VENDOR_T2LM_RESPONSE_FRAME_STATUS_SUCCESS: TID-to-link mapping is
 * accepted.
 *
 * @QCA_WLAN_VENDOR_T2LM_RESPONSE_FRAME_STATUS_DENIED_TID_TO_LINK_MAPPING:
 * TID-to-link mapping is denied.
 *
 * @QCA_WLAN_VENDOR_T2LM_RESPONSE_FRAME_STATUS_PREFERRED_TID_TO_LINK_MAPPING:
 * TID-to-link mapping is denied and new mapping is provided.
 */
enum qca_wlan_vendor_t2lm_response_frame_type {
    QCA_WLAN_VENDOR_T2LM_RESP_FRAME_STATUS_INVALID = 0,
    QCA_WLAN_VENDOR_T2LM_RESP_FRAME_STATUS_SUCCESS = 1,
    QCA_WLAN_VENDOR_T2LM_RESP_FRAME_STATUS_DENIED_TID_TO_LINK_MAPPING = 2,
    QCA_WLAN_VENDOR_T2LM_RESP_FRAME_STATUS_PREFERRED_TID_TO_LINK_MAPPING = 3,
};

#endif /* WLAN_FEATURE_11BE */

#ifdef CONFIG_SAWF
#define QCA_NL80211_VENDOR_SUBCMD_SAWF_SLA_BREACH 208

/**
 * enum qca_vendor_attr_sla_param_type - Represents SLA param type
 * This enum defines the valid set of SLA params. These
 * values are used by attribute
 * %QCA_WLAN_VENDOR_ATTR_SLA_PARAM_TYPE_INVALID.
 *
 * @QCA_WLAN_VENDOR_ATTR_SLA_MIN_THROUGHPUT: represents min throughput SLA.
 * @QCA_WLAN_VENDOR_ATTR_SLA_MAX_THROUGHPUT: represents max throughput SLA
 * @QCA_WLAN_VENDOR_ATTR_SLA_BURST_SIZE: represents burst size SLA.
 * @QCA_WLAN_VENDOR_ATTR_SLA_SERVICE_INTERVAL: represents service interval SLA.
 * @QCA_WLAN_VENDOR_ATTR_SLA_DELAY_BOUND: represents delay bound SLA.
 * @QCA_WLAN_VENDOR_ATTR_SLA_MSDU_TTL: represents MSDU TTL drop SLA.
 * @QCA_WLAN_VENDOR_ATTR_SLA_MSDU_RATE_LOSS: represents MSDU loss rate SLA.
 */
enum qca_vendor_attr_sla_param_type {
	QCA_WLAN_VENDOR_ATTR_SLA_PARAM_TYPE_INVALID= 0,

	QCA_WLAN_VENDOR_ATTR_SLA_MIN_THROUGHPUT,
	QCA_WLAN_VENDOR_ATTR_SLA_MAX_THROUGHPUT,
	QCA_WLAN_VENDOR_ATTR_SLA_BURST_SIZE,
	QCA_WLAN_VENDOR_ATTR_SLA_SERVICE_INTERVAL,
	QCA_WLAN_VENDOR_ATTR_SLA_DELAY_BOUND,
	QCA_WLAN_VENDOR_ATTR_SLA_MSDU_TTL,
	QCA_WLAN_VENDOR_ATTR_SLA_MSDU_RATE_LOSS,
};

/**
 * enum qca_wlan_vendor_attr_sawf_sla_params - This enum defines
 * attributes required for QCA_NL80211_VENDOR_SUBCMD_SLA
 * QCA_NL80211_VENDOR_SUBCMD_PEER_STATS_CACHE_FLUSH
 * Attributes are required to notify SLA breaches from driver to
 * user application.
 *
 * @QCA_WLAN_VENDOR_ATTR_SLA_PEER_MAC: Unsigned 8-bit array
 * of size 6, representing peer mac address.
 * @QCA_WLAN_VENDOR_ATTR_SLA_SVC_ID: Unsigned 8-bit representing service ID
 * @QCA_WLAN_VENDOR_ATTR_SLA_PARAM: Unsigned 8-bit indicaing service class
 * from qca_vendor_attr_sla_param_type
 * @QCA_WLAN_VENDOR_ATTR_SLA_PEER_MLD_MAC: Unsigned 8-bit array
 * of size 6, representing peer mld mac address. Filled only for ML capable
 * peers
 * @QCA_WLAN_VENDOR_ATTR_SLA_AC: Unsigned 8-bit representing AC for breach
 * @QCA_WLAN_VENDOR_ATTR_SLA_MSDUQ_ID: Unsigned 8-bit representing MSDUQ ID
 * @QCA_WLAN_VENDOR_ATTR_SLA_HW_LINK_ID: Unsigned 16-bit representing hardware
 * link id on which station is connected
 *
 */
enum qca_wlan_vendor_attr_sawf_sla_params {
	QCA_WLAN_VENDOR_ATTR_SLA_INVALID = 0,

	QCA_WLAN_VENDOR_ATTR_SLA_PEER_MAC = 1,
	QCA_WLAN_VENDOR_ATTR_SLA_SVC_ID = 2,
	QCA_WLAN_VENDOR_ATTR_SLA_PARAM = 3,
	QCA_WLAN_VENDOR_ATTR_SLA_SET_CLEAR = 4,
	QCA_WLAN_VENDOR_ATTR_SLA_PEER_MLD_MAC = 5,
	QCA_WLAN_VENDOR_ATTR_SLA_AC = 6,
	QCA_WLAN_VENDOR_ATTR_SLA_MSDUQ_ID = 7,
	QCA_WLAN_VENDOR_ATTR_SLA_HW_LINK_ID = 8,

	/* Keep last */
	QCA_WLAN_VENDOR_ATTR_SLA_LAST,
	QCA_WLAN_VENDOR_ATTR_SLA_MAX =
		QCA_WLAN_VENDOR_ATTR_SLA_LAST - 1
};
#endif

/**
 * enum qca_wlan_vendor_attr_mlo_link_tbtt_update_event - This enum defines
 * attributes required for
 * QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_REMOVAL_TBTT_UPDATE
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_TSF: u64 value indicating
 * the TSF value in the first beacon sent with ML Reconfig IE
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_TBTT - u16 value indicating the
 * AP removal timer TBTT value as part of the ongoing link removal procedure
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_BCN_INT - u16 value indicating
 * the current beacon interval that is configured at the driver
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_HW_LINK_ID - u16 value indicating
 * the hardware link id of the corresponding link
 *
 * @QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_AP_MLD_ADDR - 6 bytes value indicating
 * the AP mld addr of the MLD.
 *
 */
enum qca_wlan_vendor_attr_mlo_link_tbtt_update_event
{
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_EVENT_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_TSF,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_TBTT,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_BCN_INT,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_HW_LINK_ID,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_AP_MLD_ADDR,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_MAX =
    QCA_WLAN_VENDOR_ATTR_MLO_LINK_TBTT_UPDATE_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_add_send_egid This enum is used to
 * update the 128-bit EGID Activity Info.
 *
 * @QQCA_WLAN_VENDOR_ATTR_SEND_EGID_1: Required and type is u32.
 * This attribute is used to indicates 0 - 31bits infomation
 * of EGID Activity.
 *
 * @QQCA_WLAN_VENDOR_ATTR_SEND_EGID_2: Required and type is u32.
 * This attribute is used to indicates 32-63 bits infomation
 * of EGID Activity.
 *
 * @QQCA_WLAN_VENDOR_ATTR_SEND_EGID_3: Required and type is u32.
 * This attribute is used to indicates 64-95bit infomation
 * of EGID Activity.
 *
 * @QQCA_WLAN_VENDOR_ATTR_SEND_EGID_4: Required and type is u32.
 * This attribute is used to indicate 96-127 bit infomation
 * of EGID Activity.
 */

enum qca_wlan_vendor_attr_add_send_egid {
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_1,
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_2,
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_3,
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_4,
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_SEND_EGID_MAX =
        QCA_WLAN_VENDOR_ATTR_SEND_EGID_AFTER_LAST - 1
};

/**
 * enum qca_wlan_vendor_attr_ttlm_link_update_event - This enum defines
 * attributes required for the command
 * %QCA_NL80211_VENDOR_SUBCMD_TTLM_LINK_UPDATE_EVENT
 *
 * @QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_MLO_LINK_BITMAP: u16
 * value indicating the bitmap of currently enabled links of the
 * corresponding wdev. Each bit of the bitmap refers to the status the
 * mlo link. If the bit is set to 0, it means that the mlo link of that
 * bit-position is disabled. If the bit is set to 1, it means that the
 * mlo link of that bit-position is enabled. The userspace can read
 * this bitmap and identify between enabled and disabled links.
 *
 * @QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_AP_MLD_ADDR: 6 bytes value
 * indicating the MAC address of the AP MLD. On a given SOC, there can be
 * multiple AP MLDs and user can decide on disabling one or more links
 * for any one of these MLDs. The attribute indicates the MAC address of the
 * AP MLD whose links are currently being disabled or enabled.
 *
 **/

enum qca_wlan_vendor_attr_ttlm_link_update_event {
       QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_INVALID = 0,
       QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_MLO_LINK_BITMAP = 1,
       QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_AP_MLD_ADDR = 2,

       /* keep last */
       QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_AFTER_LAST,
       QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_MAX =
       QCA_WLAN_VENDOR_ATTR_TTLM_LINK_UPDATE_AFTER_LAST - 1,
};

#ifdef WLAN_SUPPORT_SCS_API

/**
 * enum qca_wlan_vendor_attr_scs_api_msg_type - Used by vendor command
 * QCA_NL80211_VENDOR_SUBCMD_SCS_API_MSG to exchange SCS parameters defined in
 * enum qca_wlan_vendor_attr_scs_api_msg between driver and userspace
 * application.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_REGISTRATION: Message type used to
 * send SCS application registration data from userspace application to driver.
 * The following attribute is used with this operation:
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_SCS_REGISTRATION_DATA
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST: Message type used to
 * forward SCS request from driver to userspace application.
 * The following attributes are used with this operation:
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_PEER_MAC
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MLD_MAC in case of MLO
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_SCS_REQUEST
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST_PARSED_DATA: Message type
 * used to send the parsed SCS data from userspace application to driver.
 * The following attributes are used with this operation:
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_PEER_MAC
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MLD_MAC in case of MLO
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DIALOG_TOKEN
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_SCS_PARSED_DATA
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_DRIVER_RESPONSE: Message type used to
 * send the driver response parameters from driver to userspace application.
 * The following attributes are used with this operation:
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_PEER_MAC
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MLD_MAC in case of MLO
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DIALOG_TOKEN
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TOTAL_SCS_IDX
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DRIVER_RESPONSE
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_RESPONSE: Message type used to
 * send SCS response parameters from userspace application to driver.
 * The following attributes are used with this operation:
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_PEER_MAC
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MLD_MAC in case of MLO
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DIALOG_TOKEN
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TOTAL_SCS_IDX
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_SCS_RESPONSE
 */
enum qca_wlan_vendor_attr_scs_api_msg_type {
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_REGISTRATION = 1,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST = 2,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST_PARSED_DATA = 3,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_DRIVER_RESPONSE = 4,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_RESPONSE = 5,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_MAX,
};

/**
 * enum qca_wlan_vendor_attr_scs_api_msg_params - Attributes used in SCS message
 * exchanges between driver and userspace application for the message types
 * defined in the enum qca_wlan_vendor_attr_scs_api_msg_type.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE: Mandatory u8 attribute for all
 * message exchanges.
 * Represents the message type for SCS exchanges between driver and userspace
 * application.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_PEER_MAC: Mandatory attribute in case of
 * the below SCS message exchanges.
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST_PARSED_DATA
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_DRIVER_RESPONSE
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_RESPONSE
 * Represents the Link MAC address of the station which sent the SCS request.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MLD_MAC: Mandatory attribute only in case
 * of MLO configuration for the below SCS message exchanges.
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST_PARSED_DATA
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_DRIVER_RESPONSE
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_RESPONSE
 * Represents the MLD MAC address of the station which sent the SCS request.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DIALOG_TOKEN: Mandatory u8 attribute in
 * case of the below SCS message exchanges.
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_REQUEST_PARSED_DATA
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_DRIVER_RESPONSE
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_RESPONSE
 * Represents the dialog token value of the SCS request frame received from the
 * station.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TOTAL_SCS_IDX: Mandatory u8 attribute in
 * case of the below SCS message exchanges.
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_DRIVER_RESPONSE
 * %QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE_SCS_RESPONSE
 * Represents the total number of SCS descriptors present in the single SCS
 * request frame received from the station.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DATA: Mandatory attribute for all
 * message exchanges.
 * Represents the data of the SCS msg type exchanged between driver and
 * userspace application.
 *
 */
enum qca_wlan_vendor_attr_scs_api_msg_params {
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_INVALID = 0,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TYPE = 1,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_PEER_MAC = 2,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MLD_MAC = 3,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DIALOG_TOKEN = 4,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_TOTAL_SCS_IDX = 5,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_DATA = 6,

        /* Keep last */
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_AFTER_LAST,
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_MAX =
        QCA_WLAN_VENDOR_ATTR_SCS_API_MSG_AFTER_LAST - 1,
};
#endif /* WLAN_SUPPORT_SCS_API */

#ifdef SAWF_ADMISSION_CONTROL

#ifdef WLAN_SUPPORT_SCS

/**
 * enum qca_wlan_vendor_attr_scs_data_params - Attributes used for SCS specific
 * data and this is embedded within SCS Admission Control Request and Response
 * msg QCA_NL80211_VENDOR_SUBCMD_SCS_ADM_CTRL_MSG.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_SCSID: u8 attribute
 * Represents SCS ID.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_ACCESS_CATEGORY: u8 attrbiute
 * Represents access category.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_TID: u8 attribute
 * Represents traffic identifier.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_SERVICE_INTERVAL: u32 attribute
 * Represents service interval (in milliseconds).
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_DELAY_BOUND: u32 attribute
 * Represents delay bound (in milliseconds).
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_BURST_SIZE: u32 attribute
 * Represents burst size (in bytes).
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_MIN_DATA_RATE: u32 attribute
 * Represents minimum throughout (in kbps).
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_DATA_STATUS: u8 attribute
 * Represents status (status/failure) and this is present in response msg
 * from RM to WLAN Host driver.
 */
enum qca_wlan_vendor_attr_scs_data_params {
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_INVALID = 0,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_SCSID,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_ACCESS_CATEGORY,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_TID,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_SERVICE_INTERVAL,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_DELAY_BOUND,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_BURST_SIZE,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_MIN_DATA_RATE,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_STATUS,

       /* Keep last */
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_LAST,
       QCA_WLAN_VENDOR_ATTR_SCS_DATA_MAX =
                       QCA_WLAN_VENDOR_ATTR_SCS_DATA_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_scs_adm_ctrl_params - Attributes used in SCS
 * Admission Control Reqest & Response Msg (QCA_NL80211_VENDOR_SUBCMD_SCS_ADM_CTRL_MSG)
 * exchanged between WLAN Host Driver and User Space RM Application.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_DIALOG_TOKEN: u8 attribute
 * Represents dialog token value as received from station in 11BE SCS request
 * action frame.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_HW_LINK_ID: u16 attribute
 * Represents the hardware link id of the radio on which station is connected.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_PEER_MAC: 6 bytes mac address attribute
 * Represents mac address of the station. For MLO capable station, this
 * attribute represents link mac address on which SCS request is received.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_PEER_MLD_MAC: 6 bytes mac address attribute
 * Represents MLD mac address of the station if it is MLO capable.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_NUM_SCS_REQS: u8 attribute
 * Represents no. of SCS requests/responses present in the msg.
 *
 * @QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_SCS_DATA: Nested attribute specified by
 * enum qca_wlan_vendor_attr_scs_data_params which represents SCS specific
 * information in request/response msg.
 *
 */
enum qca_wlan_vendor_attr_scs_adm_ctrl_params {
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_INVALID = 0,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_DIALOG_TOKEN,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_HW_LINK_ID,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_PEER_MAC,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_PEER_MLD_MAC,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_NUM_SCS_REQS,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_SCS_DATA,

       /* Keep last */
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_LAST,
       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_MAX =
                       QCA_WLAN_VENDOR_ATTR_SCS_ADM_CTRL_LAST - 1,
};
#endif /* WLAN_SUPPORT_SCS */

/**
 * enum qca_wlan_vendor_attr_sawf_peer_msduq_params - Attributes used in
 * Peer MSDUQ event NL Msg (QCA_NL80211_VENDOR_SUBCMD_SAWF_PEER_MSDUQ_EVENT)
 * which is exchanged between WLAN Host Driver and User Space RM application.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_HW_LINK_ID: u16 attribute
 * Represents hardware link id on which the station is connected.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MAC: 6bytes mac address attribute
 * Represents link mac address of the station.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MLD_MAC: 6bytes mac address attribute
 * Represents mld mac address of the station if it is MLO capable.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_ID: u8 attribute
 * Represents msduq id for which notification is being sent.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_EVENT_TYPE: u8 attribute
 * Represents event type (add/delete/update)
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SVC_ID: u8 attribute
 * Represents service class id.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SVC_TYPE: u8 attribute
 * Represents service class type.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SVC_PRIORITY: u8 attribute
 * Represents service class priority.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_TID: u8 attribute
 * Represents service class traffic identifier (TID).
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_AC: u8 attribute
 * Represents service class access category (AC).
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MARK_METADATA: u32 attribute
 * Represents mark metadata.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SERVICE_INTERVAL: u32 attribute
 * Represents service interval (in milliseconds).
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_BURST_SIZE: u32 attribute
 * Represents burst size (in bytes).
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_DELAY_BOUND: u32 attribute
 * Represents delay bound (in milliseconds).
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MIN_THROUGHTPUT: u32 attribute
 * Represents minimum throughput (in kbps).
 */
enum qca_wlan_vendor_attr_sawf_peer_msduq_params {
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDQU_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_HW_LINK_ID,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MAC,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MLD_MAC,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_ID,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_EVENT_TYPE,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SVC_ID,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SVC_TYPE,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SVC_PRIORITY,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_TID,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_AC,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MARK_METADATA,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_SERVICE_INTERVAL,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_BURST_SIZE,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_DELAY_BOUND,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MIN_THROUGHTPUT,

	/* Keep last */
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_LAST,
	QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_MAX =
		QCA_WLAN_VENDOR_ATTR_SAWF_PEER_MSDUQ_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_sawf_flow_deprioritize_msg_params - Attributes
 * used in SAWF flow deprioritization NL command sent by user space RM
 * application to WLAN host driver.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_PEER_MAC: 6 bytes mac
 * address attribute
 * Represents STA link mac address whose flow is to be deprioritized.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MLD_MAC: 6 bytes mac
 * address attribute
 * Represents STA MLD mac address if it is MLO capable.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MSDUQ_ID: u8 attribute
 * Represents msdu queue id on which flow is ongoing.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MARK_METADATA: u32 attribute
 * Represents sawf mark metadata of the flow.
 *
 * @QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_SVC_ID : u8 attribute
 * Represents service class id of the flow.
 *
 * QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_SVC_TYPE: u8 attribute
 * Represents service class type of the flow.
 *
 * QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_HW_LINK_ID: u16 attribute
 * Represents hw_link_id of the radio on which client is connected.
 */
enum qca_wlan_vendor_attr_sawf_flow_deprioritize_msg_params {
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_INVALID = 0,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_PEER_MAC,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MLD_MAC,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MSDUQ_ID,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MARK_METADATA,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_SVC_ID,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_SVC_TYPE,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_HW_LINK_ID,

        /* Keep Last */
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_LAST,
        QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_MAX =
                QCA_WLAN_VENDOR_ATTR_SAWF_FLOW_DEPRIORITIZE_MSG_LAST - 1,
};
#endif /* SAWF_ADMISSION_CONTROL */

/**
 * enum qca_nl80211_vendor_subcmds_index_internal - Events defined
 * outside qca_nl80211_vendor_subcmds_index enum events.
 *
 * @QCA_NL80211_VENDOR_SUBCMD_AFC_EVT_INDEX: int attribute
 * Represents AFC event NL Msg index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_RM_GENERIC_INDEX: int attribute
 * Represents T2LM APP reply generic response NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY_INDEX: int attribute
 * Represents send T2LM teardown NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_SAWF_SLA_BREACH_INDEX: int attribute
 * Represents SAWF send breach NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_REMOVAL_TBTT_UPDATE_INDEX: int attribute
 * Represents send link removal TBTT update NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_QCOM_HW_STATUS_INDEX: int attribute
 * Represents HW status notification NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_SCS_API_MSG_INDEX: int attribute
 * Represents send SCS API msg NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_SCS_ADM_CTRL_INDEX:
 * Represents send SCS ADM CTRL req NL event index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_SAWF_PEER_MSDUQ_EVENT_INDEX: int attribute
 * Represents Peer MSDUQ NL event Index
 *
 * @QCA_NL80211_VENDOR_SUBCMD_INDEX_MAX: int attribute
 * Represents max count of cfg vendor specific events.
 */
enum qca_nl80211_vendor_subcmds_index_internal {
    /* START INDEX is same as first index defined outside
     * qca_nl80211_vendor_subcmds_index enum events
     */
#ifdef CONFIG_AFC_SUPPORT
    QCA_NL80211_VENDOR_SUBCMD_AFC_EVT_INDEX = 100,
#endif
#ifdef WLAN_FEATURE_11BE
    QCA_NL80211_VENDOR_SUBCMD_RM_GENERIC_INDEX = 101,
    QCA_NL80211_VENDOR_SUBCMD_PEER_T2LM_POLICY_INDEX = 102,
#endif
#ifdef CONFIG_SAWF
    QCA_NL80211_VENDOR_SUBCMD_SAWF_SLA_BREACH_INDEX = 103,
#endif
    QCA_NL80211_VENDOR_SUBCMD_MLO_LINK_REMOVAL_TBTT_UPDATE_INDEX = 104,
#ifdef WLAN_FEATURE_11BE
    QCA_NL80211_VENDOR_SUBCMD_QCOM_HW_STATUS_INDEX = 105,
#endif
#ifdef WLAN_SUPPORT_SCS_API
    QCA_NL80211_VENDOR_SUBCMD_SCS_API_MSG_INDEX = 106,
#endif
#ifdef SAWF_ADMISSION_CONTROL
#ifdef WLAN_SUPPORT_SCS
    QCA_NL80211_VENDOR_SUBCMD_SCS_ADM_CTRL_INDEX = 107,
#endif
    QCA_NL80211_VENDOR_SUBCMD_SAWF_PEER_MSDUQ_EVENT_INDEX = 108,
#endif

    /* Keep last */
    QCA_NL80211_VENDOR_SUBCMD_INDEX_LAST,
    QCA_NL80211_VENDOR_SUBCMD_INDEX_MAX =
        QCA_NL80211_VENDOR_SUBCMD_INDEX_LAST - 1,
};

/*
 * enum qca_wlan_vendor_pasn_create_delete_action: Action to authenticate (and generate keys
 * for) or drop existing PASN security association for the listed the peers. or to create or
 * delete pasn peer.
 *
 * @QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_AUTH:  Initiate PASN handshake with the peer
 * devices indicated with %QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_MAC_ADDR.
 * @QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_DELETE_SECURE_RANGING_CONTEXT: Indication from
 * the driver to userspace to inform that the existing PASN keys of the peer devices
 * not valid anymore.
 * @QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_PEER_CREATE: To create the peer.
 * @QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_PEER_DELETE: To delete the peer.
 */
enum qca_wlan_vendor_pasn_create_delete_action {
    QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_AUTH,
    QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_DELETE_SECURE_RANGING_CONTEXT,
    QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_PEER_CREATE,
    QCA_WLAN_VENDOR_PASN_CREATE_DELETE_ACTION_PEER_DELETE,
};

/*
 * enum qca_wlan_vendor_attr_create_delete_pasn_peer: Defines the nested attributes used in
 * QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEERS.
 *
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_SRC_ADDR: This attribute is optional in
 * create or delete the pasn peer commands.
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_MAC_ADDR: Indicates the MAC address of the
 * peer device or responder mac addr to create or delete the pasn peer.
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_LTF_KEYSEED_REQUIRED: NLA_FLAG attribute
 * Indicates that LTF keyseed is reuired or not at the time of create pasn peer.
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_STATUS_SUCCESS: NLA_FLAG attribute. This
 * attribute is used in the command response from userspace to the driver. If present,
 * it indicates the successful PASN handshake with the peer or the key is installed
 * successfully on the initiator FW.
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_TYPE: u32 attribute to indicate the peer
 * type in pasn create command. Type can pe unsecure or secure.
 * 0 - unsecure
 * 1 - secure
 * any other value - invalid
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_CONTROL_FLAG: u16 attribute,
 * defines the flag type at the the time of delete pasn peer.
 * BIT(0) : Peer is already deleted at target. Cleanup the host objmgr peer
 * BIT(1) : Flush the derived keys for this peer at userspace.
 */
enum qca_wlan_vendor_attr_create_delete_pasn_peer {
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_SRC_ADDR = 1,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_MAC_ADDR = 2,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_LTF_KEYSEED_REQUIRED = 3,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_STATUS_SUCCESS = 4,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_TYPE = 5,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_CONTROL_FLAG = 6,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_MAX =
        QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEER_AFTER_LAST - 1,
};

/*
 * enum qca_wlan_vendor_attr_create_delete_pasn: Defines the attributes used in the
 * QCA_NL80211_VENDOR_SUBCMD_PASN command to create or delete the PASN peer.
 *
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_ACTION: u32 attribute, possible values are
 * defined in enum qca_wlan_vendor_pasn_create_delete_action.
 * @QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEERS: Nested attribute, used to pass PASN peer
 * details for each peer.
 * The nested attributes used inside QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEERS
 * are defined in enum qca_wlan_vendor_attr_create_delete_pasn_peer.
 */
enum qca_wlan_vendor_attr_create_delete_pasn {
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_ACTION = 1,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_PEERS = 2,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_MAX =
        QCA_WLAN_VENDOR_ATTR_CREATE_DELETE_PASN_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_mu_tput_info - This enum
 * defines attributes used by %QCA_NL80211_VENDOR_SUBCMD_MU_TPUT_INFO to send
 * MU throughput information to userspace.
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_VIF_ID: u8 value
 * indicates the VIF ID for which MU modes were enabled and disabled.
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_HW_LINK_ID: u8 value
 * indicates the Radio ID.
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_SCHED_MODE: u8 value
 * Represents the mode for which MU throughput is calculated.
 * From one of the following modes we can specify the mode for which
 * throughput needs to be calculated:
 * 1. DL MU-MIMO
 * 2. UL MU-MIMO
 * 3. MU DL OFDMA
 * 4. MU UL OFDMA
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_ACTIVE_DURATION: u32 attribute.
 * Represents the active MU duration (in ms) for which throughput is calculated.
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_INACTIVE_DURATION: u32 attribute.
 * Represents the inactive MU duration (in ms) for which throughput is
 * calculated.
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_ACTIVE_TPUT: u32 value represents
 * throughput (in mbps) value for specified active duration (in ms).
 *
 * @QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_INACTIVE_TPUT: u32 value represents
 * inactive MU throughput (in mbps) or SU throughput.
 *
 */
enum qca_wlan_vendor_attr_mu_tput_info {
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_VIF_ID = 1,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_HW_LINK_ID = 2,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_SCHED_MODE = 3,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_ACTIVE_DURATION = 4,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_INACTIVE_DURATION = 5,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_ACTIVE_TPUT = 6,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_INACTIVE_TPUT = 7,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_MAX =
	QCA_WLAN_VENDOR_ATTR_MU_TPUT_INFO_AFTER_LAST - 1,
};

#endif
