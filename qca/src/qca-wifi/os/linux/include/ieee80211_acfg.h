/*
 * Copyright (c) 2019,2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */

#ifndef __IEEE80211_ACFG_H__
#define __IEEE80211_ACFG_H__
#include <acfg_api_types.h>

typedef enum {
	WL_EVENT_TYPE_DISASSOC_AP = 1,
	WL_EVENT_TYPE_DISASSOC_STA,
	WL_EVENT_TYPE_DEAUTH_AP,
	WL_EVENT_TYPE_DEAUTH_STA,
	WL_EVENT_TYPE_ASSOC_AP,
	WL_EVENT_TYPE_ASSOC_STA,
	WL_EVENT_TYPE_AUTH_AP,
	WL_EVENT_TYPE_AUTH_STA,
	WL_EVENT_TYPE_NODE_LEAVE,
	WL_EVENT_TYPE_PUSH_BUTTON,
    WL_EVENT_TYPE_CHAN,
    WL_EVENT_TYPE_RADAR,
	WL_EVENT_TYPE_STA_SESSION,
	WL_EVENT_TYPE_TX_OVERFLOW,
    WL_EVENT_TYPE_GPIO_INPUT,
    WL_EVENT_TYPE_MGMT_RX_INFO,
    WL_EVENT_TYPE_WDT_EVENT,
    WL_EVENT_TYPE_NF_DBR_DBM_INFO,
    WL_EVENT_TYPE_PACKET_POWER_INFO,
    WL_EVENT_TYPE_CAC_START,
    WL_EVENT_TYPE_UP_AFTER_CAC,
    WL_EVENT_TYPE_QUICK_KICKOUT,
    WL_EVENT_TYPE_ASSOC_FAILURE,
    WL_EVENT_TYPE_CHAN_STATS,
    WL_EVENT_TYPE_EXCEED_MAX_CLIENT,
#if defined(OL_ATH_SMART_LOGGING) && !defined(REMOVE_PKT_LOG)
    WL_EVENT_TYPE_SMART_LOG_FW_PKTLOG_STOP,
#endif /* defined(OL_ATH_SMART_LOGGING) && !defined(REMOVE_PKT_LOG) */
    WL_EVENT_TYPE_HW_MODE_CHANGE_STATUS,
    WL_EVENT_TYPE_SCAN_RADIO_TUNING_DONE,
    WL_EVENT_TYPE_SCAN_RADIO_CHAN_STATS,
    WL_EVENT_TYPE_RF_PATH_CHANGE_STATUS,
    WL_EVENT_TYPE_STANDBY_TRIGGER,
    /*
     * Any new addition of event should be added before max event and under
     * g_acfg_ev_fl[] global event filter array, defined to filter configured events
     * and categorize to respective subevent and flag to deliver event to userspace.
     */
    WL_ACFG_MAX_EVENT, /* Max event is added to have a count of acfg events */
} acfg_event_type_t;

typedef struct acfg_event_data {
	u_int16_t result;
	u_int16_t reason;
	u_int8_t addr[QDF_MAC_ADDR_SIZE];
	u_int8_t downlink;
    u_int8_t count;
    u_int16_t freqs[IEEE80211_CHAN_MAX];
    u_int8_t gpio_num;
    acfg_mgmt_rx_info_t mgmt_ri;
    acfg_nf_dbr_dbm_t nf_dbr_dbm;
    acfg_packet_power_t   packet_power;
    u_int8_t kick_node_mac[QDF_MAC_ADDR_SIZE];
    acfg_chan_stats_t chan_stats;
    u_int8_t dump_file[FW_DUMP_FILE_NAME_SIZE];
    char cur_phymode_str[ACFG_MAX_MODE_LEN];
    acfg_scan_blanking_params_t blanking_params;
    uint8_t trigger_type;
    char vapname[MAX_VAPNAME_LEN];
} acfg_event_data_t;

#if UMAC_SUPPORT_ACFG || UMAC_SUPPORT_ACFG_RECOVERY
void acfg_send_event(struct net_device *dev, osdev_t osdev, acfg_event_type_t type,
                    acfg_event_data_t *acfg_event);
uint32_t acfg_event_workqueue_init(osdev_t osdev);
void acfg_event_workqueue_delete(osdev_t osdev);
#else
#define acfg_send_event(dev,handle,type,acfg_event) {}
#endif

#endif /*__IEEE80211_ACFG_H__*/
