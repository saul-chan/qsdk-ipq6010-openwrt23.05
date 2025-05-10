/*
 * Copyright (c) 2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */


#ifndef __VEN_TOOL__
#define __VEN_TOOL__

//Current NL message types are COMMAND, RESPONSE and EVENT
#define NUM_NLMSG_TYPES 3
#define NLMSG_TYPE_COMMAND 0
#define NLMSG_TYPE_RESPONSE 1
#define NLMSG_TYPE_EVENT 2

#define FILE_NAME_LEN 64
#define FILE_PATH_LEN 256
#ifdef __IPQ__
#define NO_OF_ELEMENTS_IN_COMMON_DATA 4
#else
#define NO_OF_ELEMENTS_IN_COMMON_DATA 2
#endif
#define QCA_NL80211_VENDOR_SUBCMD_GET_BUS_SIZE 84
#define QCA_NL80211_VENDOR_SUBCMD_GET_RADAR_HISTORY 199
#define QCA_NL80211_VENDOR_SUBCMD_DFS_CAPABILITY 11
#define QCA_NL80211_VENDOR_SUBCMD_STATS_EXT 13
#define QCA_NL80211_VENDOR_SUBCMD_GET_SUPPORTED_FEATURES 38
#define QCA_NL80211_VENDOR_SUBCMD_SCANNING_MAC_OUI 39
#define QCA_NL80211_VENDOR_SUBCMD_GET_FEATURES 55
#define QCA_NL80211_VENDOR_SUBCMD_KEY_MGMT_SET_KEY 50
#define QCA_NL80211_VENDOR_SUBCMD_TDLS_GET_CAPABILITIES 78
#define QCA_NL80211_VENDOR_SUBCMD_AVOID_FREQUENCY 10
#define QCA_NL80211_VENDOR_SUBCMD_GET_WAKE_REASON_STATS 85
#define QCA_NL80211_VENDOR_SUBCMD_GETBAND 192
#define QCA_NL80211_VENDOR_SUBCMD_NUD_STATS_GET 150
#define QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION 74
#define QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION 75

enum qca_wlan_vendor_attr_config {
        QCA_WLAN_VENDOR_ATTR_CONFIG_INVALID = 0,
        /* 8-bit unsigned value. Used for representing MLO link ID of a
         * Single wiphy MLD netdev.
         */
        QCA_WLAN_VENDOR_ATTR_CONFIG_MLO_LINK_ID = 99,
        QCA_WLAN_VENDOR_ATTR_CONFIG_AFTER_LAST,
        QCA_WLAN_VENDOR_ATTR_CONFIG_MAX =
	QCA_WLAN_VENDOR_ATTR_CONFIG_AFTER_LAST - 1,
};

typedef struct {
    char iface[IFACE_LEN];
    char config_file[FILE_NAME_LEN];
    uint8_t is_vap_command;
    uint8_t skip_cmd_table;
#ifdef __IPQ__
    uint8_t link_id;
#endif
} common_data;

#endif
