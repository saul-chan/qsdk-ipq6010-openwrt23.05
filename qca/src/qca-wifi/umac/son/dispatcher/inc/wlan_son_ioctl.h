/*
* Copyright (c) 2020 Qualcomm Innovation Center, Inc.
*
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Innovation Center, Inc.
*/

#ifndef _SON_IOCTL_H_
#define _SON_IOCTL_H_

#include <wlan_son_band_steering_api.h>
#include <ieee80211.h>
#include "wlan_son_ald_external.h"

#ifdef CONFIG_BAND_6GHZ
#define MAP_MAX_OPERATING_CLASSES 22
#else
#define MAP_MAX_OPERATING_CLASSES 17
#endif


#ifdef CONFIG_BAND_6GHZ
#define MAP_MAX_CHANNELS_PER_OP_CLASS  70
#else
#define MAP_MAX_CHANNELS_PER_OP_CLASS  25
#endif

#define MAP_MAX_CAC_MODES 1

typedef struct client_assoc_req_acl_t {
    /// STA MAC
    u_int8_t stamac[IEEE80211_ADDR_LEN];

    /// Validity Period
    u_int16_t validity_period;
} client_assoc_req_acl_t;


// client cap type is identical to map service
// any change here must be reflect in map service.
#define MAP_SERVICE_MAX_ASSOC_FRAME_SZ 1024
typedef struct map_client_cap_t {
    /// Size of the (Re)Assoc frame in bytes
    u_int16_t frameSize;

    /// The frame body of the most recently received
    /// (Re)Association Request frame
    u_int8_t assocReqFrame[MAP_SERVICE_MAX_ASSOC_FRAME_SZ];
} map_client_cap_t;


typedef struct map_rssi_policy_t {
    /// STA metrics reporting RSSI threshold
    u_int8_t rssi;

    /// STA Metrics Reporting RSSI Hysteresis Margin Override
    u_int8_t rssi_hysteresis;
} map_rssi_policy_t;


/**
 * @brief Parameters to store CAC capabilities for supported op classes
 */
typedef struct mapv2_cac_cap_t {
    /// Numer of types of CAC the radio can perform
    u_int8_t num_cac_type;

    // cac type defined by cac mode + time to complete cac
    struct {

        /// cac mode
        u_int8_t cac_mode_supported;

        /// time to complete cac in seconds
        u_int8_t secRequiredForCAC[3];

        /// Number of classes supported
        u_int8_t num_classes_supported;

        /// Info for each supported operating class
        struct {
            /// Operating class for which capability is being described
            u_int8_t opclass;

            /// Number of channels supported in the operating class
            u_int8_t num_chan;

            /// Single channel number for which capability is being described
            u_int8_t supported_chan_num[MAP_MAX_CHANNELS_PER_OP_CLASS];
        } opclass[MAP_MAX_OPERATING_CLASSES];
    } cac_type[MAP_MAX_CAC_MODES];
} mapv2_cac_cap_t;


typedef struct mapv2_cac_info_t {
    u_int8_t cac_capabilities_valid;
    mapv2_cac_cap_t cac_cap;
} mapv2_cac_info_t;


#define MAP_WIFI6_MAX_TID 0x04
typedef struct map_wifi6_stastats_t {
    /// MAC address of the associated STA.
    u_int8_t macaddr[6];
    u_int8_t n;

    struct {
        u_int8_t tid;
        u_int8_t queueSize;
    } tidinfo[MAP_WIFI6_MAX_TID];
} map_wifi6_stastats_t;

#define MAP_WIFI7_MAX_TID 0x04
typedef struct map_wifi7_stastats_t {
    /// MAC address of the associated STA.
    u_int8_t macaddr[6];
    u_int8_t n;

    struct {
        u_int8_t tid;
        u_int8_t queueSize;
    } tidinfo[MAP_WIFI7_MAX_TID];
} map_wifi7_stastats_t;

typedef struct map_ap_radio_basic_capabilities_t {
    /// Maximum number of BSSes supported by this radio
    u_int8_t max_supported_bss;

    /// Number of operating classes supported for this radio
    u_int8_t num_supported_op_classes;

    /// Info for each supported operating class
    struct {
        /// Operating class that this radio is capable of operating on
        /// as defined in Table E-4.
        u_int8_t opclass;

        /// Maximum transmit power EIRP the radio is capable of transmitting
        /// in the current regulatory domain for the operating class.
        /// The field is coded as 2's complement signed dBm.
        int8_t max_tx_pwr_dbm;

        /// Number of statically non-operable channels in the operating class
        /// Other channels from this operating class which are not listed here
        /// are supported by this radio.
        u_int8_t num_non_oper_chan;

        /// Channel number which is statically non-operable in the operating class
        /// (i.e. the radio is never able to operate on this channel)
        u_int8_t non_oper_chan_num[MAP_MAX_CHANNELS_PER_OP_CLASS];
    } opclasses[MAP_MAX_OPERATING_CLASSES];
} map_ap_radio_basic_capabilities_t;


/**
 * @brief The HT capabilities for a specific radio on an AP
 */
typedef struct map_ap_ht_capabilities_t {
    /// Maximum number of supported Tx spatial streams
    u_int8_t max_tx_nss;

    /// Maximum number of supported Rx spatial streams
    u_int8_t max_rx_nss;

    /// Short GI support for 20 MHz
    u_int8_t short_gi_support_20_mhz : 1,

    /// Short GI support for 40 MHz
    short_gi_support_40_mhz : 1,

    // HT support for 40 MHz
    ht_support_40_mhz : 1,

    reserved : 5;
} map_ap_ht_capabilities_t;


/**
 * @brief The VHT capabilities for a specific radio on an AP
 */
typedef struct map_ap_vht_capabilities_t {
    /// Supported VHT Tx MCS
    /// Supported set to VHT MCSs that can be received.
    /// Set to Tx VHT MCS Map field per Figure 9-562.
    u_int16_t supported_tx_mcs;

    /// Supported VHT Rx MCS
    /// Supported set to VHT MCSs that can be transmitted.
    /// Set to Rx VHT MCS Map field per Figure 9-562.
    u_int16_t supported_rx_mcs;

    /// Maximum number of supported Tx spatial streams
    u_int8_t max_tx_nss;

    /// Maximum number of supported Rx spatial streams
    u_int8_t max_rx_nss;

    /// Short GI support for 80 MHz
    u_int8_t short_gi_support_80_mhz : 1,

    /// Short GI support for 160 and 80+80 MHz
    short_gi_support_160_mhz_80p_80_mhz : 1,

    /// VHT support for 80+80 MHz
    support_80p_80_mhz : 1,

    /// VHT support for 160 MHz
    support_160_mhz : 1,

    /// SU beamformer capable
    su_beam_former_capable : 1;

    /// MU beamformer capable
    u_int8_t mu_beam_former_capable : 1,

    reserved : 2;
} map_ap_vht_capabilities_t;


#define MAP_MAX_HE_MCS 6
/**
 * @brief The HE capabilities for a specific radio on an AP
 */
typedef struct map_ap_he_capabilities_t {
    /// Number of supported HE MCS entries
    u_int8_t num_mcs_entries;

    /// Supported HE MCS indicating set of supported HE Tx and Rx MCS
    u_int16_t supported_he_mcs[MAP_MAX_HE_MCS];

    /// Maximum number of supported Tx spatial streams
    u_int8_t max_tx_nss;

    /// Maximum number of supported Rx spatial streams
    u_int8_t max_rx_nss;

    /// HE support for 80+80 MHz
    u_int8_t support_80p_80_mhz : 1,

    /// HE support for 160 MHz
    support_160_mhz : 1,

    /// SU beamformer capable
    su_beam_former_capable : 1,

    /// MU beamformer capable
    mu_beam_former_capable : 1,

    /// UL MU-MIMO capable
    ul_mu_mimo_capable : 1,

    /// UL MU-MIMO + OFDMA capable
    ul_mu_mimo_ofdma_capable : 1,

    /// DL MU-MIMO + OFDMA capable
    dl_mu_mimo_ofdma_capable : 1,

    /// UL OFDMA capable
    ul_ofdma_capable : 1;

    /// DL OFDMA capable
    u_int8_t dl_ofdma_capable : 1,

    reserved : 7;
} map_ap_he_capabilities_t;

// Station or AP
#define IEEE1905_MAX_ROLES 2

typedef struct map_ap_wifi6_capabilities_t {
    u_int8_t radioid[6];
    u_int8_t numofroles;
    struct {
        /// 0: wi-fi 6 support info for the ap role
        /// 1: wi-fi 6 support info for the non-ap sta role
        /// 2-3: reserved
        u_int8_t role : 2;
        /// support for he 160 mhz
        /// 0: not supported
        /// 1: supported
        u_int8_t he160 : 1;
        /// support for he 80+80 mhz
        /// 0: not supported
        /// 1: supported
        u_int8_t he80plus80 : 1;
        /// supported he mcs indicating set of supported he tx and rx mcs
        u_int16_t supported_he_mcs[MAP_MAX_HE_MCS];
        /// support for su beamformer.
        u_int8_t subeamformer : 1;
        /// support for su beamformee
        u_int8_t subeamformee : 1;
        /// support for mu beamformer status
        u_int8_t mu_beam_former_status : 1;
        /// support for beamformee sts ≤ 80 mhz
        u_int8_t beam_formee_sts_less_than_80supported : 1;
        /// support for beamformee sts > 80 mhz
        u_int8_t beam_formee_sts_more_than_80supported : 1;
        /// support for ul mu-mimo
        u_int8_t ulmumimosupported : 1;
        /// support for ul ofdma.
        u_int8_t ulofdmasupported : 1;
        /// support for dl ofdma
        u_int8_t dlofdmasupported : 1;
        /// max number of users supported per
        /// dl mu-mimo tx in an ap role
        uint8_t maxuser_per_dl_mumimotxap : 4;
        /// max number of users supported per
        /// dl mu-mimo rx in an ap role
        uint8_t max_user_per_dl_mumimorxap : 4;
        /// max number of users supported per dl ofdma tx in an ap role
        uint8_t maxuserdlofdmatxap;
        /// max number of users supported per ul ofdma rx in an ap role
        uint8_t maxuserdlofdmarxap;
        /// support for rts
        u_int8_t rtssupported : 1;
        /// support for mu rts
        u_int8_t murtssupported : 1;
        /// support for multi-bssid
        u_int8_t multibssidsupported : 1;
        /// support for mu edca
        u_int8_t muedcasupported : 1;
        /// support for twt requester
        u_int8_t twtrequestersupprted : 1;
        /// support for twt responder
        u_int8_t twtrespondersupported : 1;
    } role_cap[IEEE1905_MAX_ROLES];
} map_ap_wifi6_capabilities_t;

#define MAP_MAX_EHT_MCS 8
/**
 * @brief The EHT capabilities for a specific radio on an AP
 */
typedef struct map_ap_eht_capabilities_t {
    /// Number of supported EHT MCS entries
    u_int8_t num_mcs_entries;

    /// Supported EHT MCS indicating set of supported EHT Tx and Rx MCS
    u_int32_t supported_eht_mcs[MAP_MAX_EHT_MCS];

    /// Maximum number of supported Tx spatial streams
    u_int8_t max_tx_nss;

    /// Maximum number of supported Rx spatial streams
    u_int8_t max_rx_nss;

    /// EHT support for 80+80 MHz
    u_int8_t support_80p_80_mhz : 1,

    /// EHT support for 160 MHz
    support_160_mhz : 1,

    /// SU beamformer capable
    su_beam_former_capable : 1,

    /// MU beamformer capable
    mu_beam_former_capable : 1,

    /// UL MU-MIMO capable
    ul_mu_mimo_capable : 1,

    /// UL MU-MIMO + OFDMA capable
    ul_mu_mimo_ofdma_capable : 1,

    /// DL MU-MIMO + OFDMA capable
    dl_mu_mimo_ofdma_capable : 1,

    /// UL OFDMA capable
    ul_ofdma_capable : 1;

    /// DL OFDMA capable
    u_int8_t dl_ofdma_capable : 1,

    reserved : 7;
} map_ap_eht_capabilities_t;

typedef struct map_ap_wifi7_capabilities_t {
    u_int8_t radioid[6];
    u_int8_t numofroles;
    struct {
        /// 0: wi-fi 7 support info for the ap role
        /// 1: wi-fi 7 support info for the non-ap sta role
        /// 2-3: reserved
        u_int8_t role : 2;
        /// support for eht 160 mhz
        /// 0: not supported
        /// 1: supported
        u_int8_t eht160 : 1;
        /// support for eht 80+80 mhz
        /// 0: not supported
        /// 1: supported
        u_int8_t eht80plus80 : 1;
        /// supported eht mcs indicating set of supported eht tx and rx mcs
        u_int32_t supported_eht_mcs[MAP_MAX_EHT_MCS];
        /// support for su beamformer.
        u_int8_t subeamformer : 1;
        /// support for su beamformee
        u_int8_t subeamformee : 1;
        /// support for mu beamformer status
        u_int8_t mu_beam_former_status : 1;
        /// support for beamformee sts ≤ 80 mhz
        u_int8_t beam_formee_sts_less_than_80supported : 1;
        /// support for beamformee sts > 80 mhz
        u_int8_t beam_formee_sts_more_than_80supported : 1;
        /// support for ul mu-mimo
        u_int8_t ulmumimosupported : 1;
        /// support for ul ofdma.
        u_int8_t ulofdmasupported : 1;
        /// support for dl ofdma
        u_int8_t dlofdmasupported : 1;
        /// max number of users supported per
        /// dl mu-mimo tx in an ap role
        uint8_t maxuser_per_dl_mumimotxap : 4;
        /// max number of users supported per
        /// dl mu-mimo rx in an ap role
        uint8_t max_user_per_dl_mumimorxap : 4;
        /// max number of users supported per dl ofdma tx in an ap role
        uint8_t maxuserdlofdmatxap;
        /// max number of users supported per ul ofdma rx in an ap role
        uint8_t maxuserdlofdmarxap;
        /// support for rts
        u_int8_t rtssupported : 1;
        /// support for mu rts
        u_int8_t murtssupported : 1;
        /// support for multi-bssid
        u_int8_t multibssidsupported : 1;
        /// support for mu edca
        u_int8_t muedcasupported : 1;
        /// support for twt requester
        u_int8_t twtrequestersupprted : 1;
        /// support for twt responder
        u_int8_t twtrespondersupported : 1;
    } role_cap[IEEE1905_MAX_ROLES];
} map_ap_wifi7_capabilities_t;

typedef struct mapapcap_t {
    map_ap_radio_basic_capabilities_t hwcap;
    map_ap_ht_capabilities_t htcap;
    map_ap_vht_capabilities_t vhtcap;
    map_ap_he_capabilities_t hecap;
    map_ap_wifi6_capabilities_t wifi6cap;
    map_ap_eht_capabilities_t ehtcap;
    map_ap_wifi7_capabilities_t wifi7cap;

    // Basic Radio capabilities are valid
    u_int8_t map_ap_radio_basic_capabilities_valid : 1,

            // HT capabilities are valid
            map_ap_ht_capabilities_valid : 1,

            // VHT capabilities are valid
           map_ap_vht_capabilities_valid : 1,

           // HE capabilities are valid
           map_ap_he_capabilities_valid : 1,

           map_ap_wifi6_capabilites_valid : 1,

           // EHT capabilities are valid
           map_ap_eht_capabilities_valid : 1,

           map_ap_wifi7_capabilites_valid : 1,

           reserved : 1;
} mapapcap_t;


/**
 * @brief Access Category Subfield Encoding
 */
typedef enum map_service_access_category_e {
    map_service_ac_bk,
    map_service_ac_be,
    map_service_ac_vi,
    map_service_ac_vo,
    map_service_ac_max,  // always last
} map_service_access_category_e;


/**
 * @brief Estimated Service Parameters Information Field. store the data Info
 * the natural (aka. native) representation rather than the OTA encoding and
 * convert them to/from the OTA encoding at the messaging layer (mapServiceMsg).
 */
typedef struct map_esp_info_t {
    struct {
        /// Whether the ESP Info for this AC is included
        u_int32_t include_esp_info : 1;

        /// Access Category to which the remaning parameters belong to.
        /// Encoding is AC_BK = 0, AC_BE = 1, AC_VI = 2 AC_VO = 3.
        /// Refer Table 9-260
        u_int8_t ac : 2;

        /// Data format encoding is as in Table 9-261.
        u_int8_t data_format : 2;

        /// BA window size indicates the size of Block Ack window for
        /// corresponding access category. Refer Table 9-262.
        u_int8_t ba_window_size;

        /// The Estimated Air Time Fraction (as a percentage from 0 - 100)
        u_int8_t est_air_time_fraction;

        /// The Data PPDU Duration Target (in microseconds)
        u_int16_t data_ppdu_dur_target;
    } esp_info[map_service_ac_max];
} map_esp_info_t;


typedef struct map_op_chan_t {
    /// Operating class that this radio is capable of operating on
    /// as defined in Table E-4.
    u_int8_t opclass;

    /// operating channel width
    enum ieee80211_cwm_width ch_width;

    /// Number of statically operable channels in the operating class
    u_int8_t num_oper_chan;

    /// Channel number which is statically operable in the operating class
    u_int8_t oper_chan_num[MAP_MAX_CHANNELS_PER_OP_CLASS];
} map_op_chan_t;


/**
 * @brief Parameters to store the channel set information for an op class
 */
typedef struct map_op_class_t {
    /// Operating class that this radio is capable of operating on
    /// as defined in Table E-4.
    u_int8_t opclass;

    /// operating channel width
    enum ieee80211_cwm_width ch_width;

    /// secondary channel location w.r.t. primary channel
    /// The location is defined by constants IEEE80211_SEC_CHAN_OFFSET_SC{N,A,B}
    u_int8_t sc_loc;

    /// Number of channels in the operating class
    u_int8_t num_chan;

    /// Channel numbers defined in the regulatory domain
    u_int8_t channels[MAP_MAX_CHANNELS_PER_OP_CLASS];
} map_op_class_t;


typedef struct ieee80211_bsteering_innetwork_2g_req_t{
    /// index
    int32_t index;

    /// memory address
    void *data_addr;

    /// channel
    int8_t ch;
} ieee80211_bsteering_innetwork_2g_req_t;

struct mesh_ald_sta {
    u_int8_t  macaddr[IEEE80211_ADDR_LEN];
    u_int32_t enable;
};

struct mesh_ald_get_sta_link_cap {
    u_int8_t  macaddr[IEEE80211_ADDR_LEN];
    linkcapacity_t link_stats;
};

typedef enum {
    MESH_ALD_STA_ENABLE,
    MESH_ALD_GET_STA_LINK_CAP,
} MESH_ALD_CMDTYPE;

typedef struct mesh_ald_req {
    MESH_ALD_CMDTYPE cmdtype;  /* sub-command */
    union {
        struct mesh_ald_sta ald_sta;
        struct mesh_ald_get_sta_link_cap ald_linkcap;
    } data;
} mesh_ald_req;

/* Structure to handle all the MESH commands */
typedef struct mesh_dbg_req_t {
    u_int8_t mesh_cmd;
    u_int8_t needs_reply;
    u_int8_t dstmac[IEEE80211_ADDR_LEN];
    union {
        u_long param[4];
        u_int8_t value;
        u_int32_t bsteering_sta_stats_update_interval_da;
        ieee80211_bsteering_param_t bsteering_param;
        ieee80211_bsteering_datarate_info_t bsteering_datarate_info;
        client_assoc_req_acl_t client_assoc_req_acl;
        mapapcap_t mapapcap;
        map_client_cap_t mapclientcap;
        map_op_chan_t map_op_chan;
        map_esp_info_t map_esp_info;
        map_rssi_policy_t map_rssi_policy;
        mapv2_cac_info_t mapv2_cac_info;
        map_op_class_t map_op_class;
        map_wifi6_stastats_t map_wifi6_sta_stats;
        map_wifi7_stastats_t map_wifi7_sta_stats;
        ieee80211_bsteering_dbg_param_t bsteering_dbg_param;
        mesh_ald_req ald_req;
        ieee80211_bsteering_innetwork_2g_req_t innetwork_2g_req;
	ieee80211_bsteering_rssi_req_t bsteering_rssi_req;
	struct ieee80211req_dfs_chan_list_info  dfs_ch_list;
        u_int32_t bsteering_event_delay_interval;
        u_int32_t app_init;
    } mesh_data;
} mesh_dbg_req_t;

enum son_ioctl {
    MESH_BSTEERING_ENABLE = 1,
    MESH_BSTEERING_ENABLE_EVENTS = 2,
    MESH_BSTEERING_SET_PARAMS = 3,
    MESH_BSTEERING_GET_RSSI = 5,
    MESH_BSTEERING_SET_OVERLOAD = 6,
    MESH_BSTEERING_LOCAL_DISASSOCIATION = 7,
    MESH_BSTEERING_SET_PROBE_RESP_WH = 8,
    MESH_BSTEERING_SET_AUTH_ALLOW = 9,
    MESH_BSTEERING_GET_DATARATE_INFO = 10,
    MESH_BSTEERING_SET_STEERING = 11,
    MESH_BSTEERING_SET_PROBE_RESP_ALLOW_24G = 12,
    MESH_BSTEERING_GET_PEER_CLASS_GROUP = 13,
    MESH_ADD_MAC_VALIDITY_TIMER_ACL = 14,
    MESH_BSTEERING_ENABLE_ACK_RSSI = 15,
    MESH_MAP_RADIO_HWCAP = 16,
    MESH_MAP_CLIENT_CAP = 17,
    MESH_MAP_GET_OP_CHANNELS = 18,
    MESH_MAP_GET_ESP_INFO = 19,
    MESH_BSTEERING_MAP_SET_RSSI = 20,
    MESH_MAPV2_GET_RADIO_CAC_CAP = 21,
    MESH_MAP_GET_OP_CLASS_INFO = 22,
    MESH_MAP_WIFI6_STA_STATS = 23,
    MESH_BSTEERING_GET_PARAMS = 24,
    MESH_BSTEERING_SET_PEER_CLASS_GROUP = 25,
    MESH_BSTEERING_GET_OVERLOAD = 26,
    MESH_BSTEERING_GET_PROBE_RESP_WH = 27,
    MESH_BSTEERING_SET_DBG_PARAMS = 28,
    MESH_BSTEERING_GET_DBG_PARAMS = 29,
    MESH_BSTEERING_SET_DA_STAT_INTV = 30,
    MESH_WHC_APINFO_ROOT_DIST = 31,
    MESH_WHC_APINFO_UPLINK_RATE = 32,
    MESH_WHC_APINFO_OTHERBAND_BSSID = 33,
    MESH_WHC_APINFO_RATE = 34,
    MESH_WHC_APINFO_BSSID = 35,
    MESH_WHC_APINFO_BEST_UPLINK_OTHERBAND_BSSID = 36,
    MESH_WHC_APINFO_CAP_BSSID = 37,
    MESH_WHC_APINFO_OTHERBAND_UPLINK_BSSID = 38,
    MESH_WHC_APINFO_SON = 39,
    MESH_WHC_APINFO_WDS = 40,
    MESH_WHC_CURRENT_CAP_RSSI = 41,
    MESH_WHC_APINFO_SFACTOR = 42,
    MESH_WHC_SKIP_HYST = 43,
    MESH_WHC_CAP_RSSI = 44,
    MESH_WHC_BACKHAUL_TYPE = 45,
    MESH_WHC_MIXEDBH_ULRATE = 46,
    MESH_WHC_APINFO_UPLINK_SNR = 47,
    MESH_VAP_SSID_CONFIG = 48,
    MESH_BEST_UL_HYST = 49,
    MESH_PARAM_MAP = 50,
    MESH_MAP_BSS_TYPE = 51,
    MESH_MAP2_BSTA_VLAN_ID = 52,
    MESH_SON_EVENT_BCAST = 53,
    MESH_MAP_VAP_BEACONING = 54,
    MESH_LOG_ENABLE_BSTEERING_RSSI = 55,
    MESH_SET_ALD = 56,
    MESH_GET_ALD_LINK_STATS =57,
    MESH_GET_VERSION_INFO = 58,
    MESH_VERSION_COMPATIBILITY_CHECK = 59,
    MESH_PARAM_SON = 60,      /* Mark/query AP as SON enabled */
    MESH_PARAM_VAP_COUNT = 61, /* Get the number of vaps on which SON is enabled*/
    MESH_PARAM_REPT_MULTI_SPECIAL = 62, /* Set ext feature caps */
    MESH_ALD_BUFF_THRESH = 63,
    MESH_SET_INNETWORK_2G = 64,
    MESH_PARAM_ACTIVE_VDEV_COUNT = 65,
    MESH_MAP_OPCLASS_FOR_CUR_HWMODE = 66,
    MESH_BSTEERING_EVENT_REPORTING_DELAY_INTERVAL = 67,
    MESH_BSTEERING_GET_BSTA_FLAG = 68, /*Get value of bSTA flag*/
    MESH_BSTEERING_SET_BSTA_FLAG = 69, /*Set value of bSTA flag*/
    MESH_SET_EZMESH_APP_INIT = 70, // Set the App Init
    MESH_GET_CAC_STATUS_DFS_CHANNEL_LIST = 71, // Get CAC status for all the dfs channels.
    MESH_GET_MLO_PRIMARY_LINK_INFO = 72, // Get mlo connection Primary link Flag
};

/**********************/
/* Versioning Support */
/**********************/

/* Macro to convert the macro into string literal */
#define STRINGIZE(x) #x
#define CONVERT_2_STRING(x) STRINGIZE(x)

/* ===============================================================
 * Macro definition of SON Version Information
 * Note: Do not change the values in below macro definition,
 * these values are set based on the backward compatibility
 * ==============================================================*/
#define SON_PKG_NAME "son"
#define SON_MAJOR_VERION    10
#define SON_MINOR_VERION    0
#define SON_COMPATIBILITY   0
#define SON_SU_BUILD        0

/* ===============================================================
 * Macro definition of MAP Version Information
 * Note: Do not change the values in below macro definition,
 * these values are set based on the backward compatibility
 * ==============================================================*/
#define MAP_PKG_NAME "easymesh"
#define MAP_MAJOR_VERION    12
#define MAP_MINOR_VERION    5
#define MAP_COMPATIBILITY   2
#define MAP_SU_BUILD        0

/* character length assumption :
 * 16+5+5+5+5 = 32 char + 4 sym = 36 + 1 nul character*/
#define MESH_VERSION_STR_MAX_LEN 36+1

/* =====================================================*/
/* Macro to generate SON Version string Ex. son-1.0.0.0 */
/* =====================================================*/
#define SON_VERSION SON_PKG_NAME "-" \
        CONVERT_2_STRING(SON_MAJOR_VERION) "." \
        CONVERT_2_STRING(SON_MINOR_VERION) "." \
        CONVERT_2_STRING(SON_COMPATIBILITY) "." \
        CONVERT_2_STRING(SON_SU_BUILD)

/* ==========================================================*/
/* Macro to generate MAP Version string Ex. easymesh-1.0.0.0 */
/* ==========================================================*/

#define MAP_VERSION MAP_PKG_NAME "-" \
        CONVERT_2_STRING(MAP_MAJOR_VERION) "." \
        CONVERT_2_STRING(MAP_MINOR_VERION) "." \
        CONVERT_2_STRING(MAP_COMPATIBILITY) "." \
        CONVERT_2_STRING(MAP_SU_BUILD)

/* ======================================
 * Intra-node Application Version
 * Note: Repacd uses hard-coded version
 * ======================================
 */
#define HYD_SON_VERSION SON_VERSION
#define WSPLCD_SON_VERSION SON_VERSION
#define LBD_SON_VERSION SON_VERSION
#define MAD_SON_VERSION SON_VERSION

#define EZMESH_MAP_VERSION MAP_VERSION
#define WSPLCD_MAP_VERSION MAP_VERSION
#define MAD_MAP_VERSION MAP_VERSION

/* ====================
 * Inter-node Version
 * ====================
 */
#define HYD_SON_INTER_NODE_MAJ_VER SON_MAJOR_VERION
#define HYD_SON_INTER_NODE_MIN_VER SON_MINOR_VERION
#define HYD_SON_INTER_NODE_COMP SON_MINOR_VERION

#define EZMESH_MAP_INTER_NODE_MAJ_VER MAP_MAJOR_VERION
#define EZMESH_MAP_INTER_NODE_MIN_VER MAP_MINOR_VERION
#define EZMESH_MAP_INTER_NODE_COMP MAP_COMPATIBILITY

/* ======================
 * Kernel Driver Version
 * ======================
 * Note: Hyfi-Bridge driver maintains same copy of version information
 */

/* UMAC SON Driver version */
#define UMAC_SON_DRIVER_VERSION SON_VERSION
#define UMAC_MAP_DRIVER_VERSION MAP_VERSION

enum version_compatibility_status {
    COMPATIBILITY_MATCH = 0,
    COMPATIBILITY_NOT_MATCH = 1,
    COMPATIBILITY_UNKNOWN = 2
};

#endif /* _SON_IOCTL_H_ */
