/*
 * Copyright (c) 2011-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * Qualcomm Atheros, Inc. has chosen to take madwifi subject to the BSD license and terms.
 *
 * Copyright (c) 2008, Atheros Communications Inc.
 */

#ifndef _IEEE80211_DEFINES_H_
#define _IEEE80211_DEFINES_H_

#include <ieee80211.h>
#ifndef EXTERNAL_USE_ONLY
#include <_ieee80211.h>        /* IEEE80211_ADDR_LEN, iee80211_phymode */
#include <wlan_mgmt_txrx_utils_api.h>
#endif

#define HOST_MAX_CHAINS 8 /* Max Tx/Rx chains */
#define NO_TX_VAP 255 /* No Tx vap */
#define IFACE_MAX_LEN 16
/* Note this is dependent on firmware - currently can only have 3 per event */
#define BSTEERING_MAX_PEERS_PER_EVENT 3
#define RSSI_MAX_CHAINS 8
#define RSSI_INVALID -128
#define FIRST_DFS_CHAN_NUM        (52)
#define CHAN_NUM_SPACING           (4)
#define WLAN_5_GHZ_BASE_FREQ    (5000)
#define WLAN_CHAN_SPACING_5MHZ     (5)
#define INVALID_INDEX             (-1)

/*
 * Public defines for Atheros Upper MAC Layer
 */

/**
 * @brief Opaque handle of 802.11 protocal layer.
 */
struct ieee80211com;
typedef struct ieee80211com *wlan_dev_t;

/**
 * @brief Opaque handle to App IE module.
*/
struct wlan_mlme_app_ie;
typedef struct wlan_mlme_app_ie *wlan_mlme_app_ie_t;

/**
 * @brief Opaque handle of network instance (vap) in 802.11 protocal layer.
*/
struct ieee80211vap;
typedef struct ieee80211vap *wlan_if_t;

struct ieee80211vapprofile;
typedef struct ieee80211vapprofile *wlan_if_info_t;

/**
 * @brief Opaque handle of a node in the wifi network.
 */
struct ieee80211_node;
typedef struct ieee80211_node *wlan_node_t;

/**
 * @brief Opaque handle of OS interface (ifp in the case of unix ).
 */
struct _os_if_t;
typedef struct _os_if_t *os_if_t;

/**
 *
 * @brief Opaque handle.
 */
typedef void *os_handle_t;

/**
 * @brief Opaque handle of a channel.
 */
struct ieee80211_ath_channel;
typedef struct ieee80211_ath_channel *wlan_chan_t;

/**
 * @brief Opaque handle scan_entry.
 */
struct ieee80211_scan_entry;
typedef struct scan_cache_entry *wlan_scan_entry_t;

#define IEEE80211_NWID_LEN                  32
#define IEEE80211_ISO_COUNTRY_LENGTH        3       /* length of 11d ISO country string */
#define IEEE80211_MAX_SSID                  32
#define IEEE80211_MAX_NEIGHBOURS            IEEE80211_MAX_SSID

typedef struct _ieee80211_ssid {
    u_int32_t   len;
    u_int8_t    ssid[IEEE80211_NWID_LEN + 1];
} ieee80211_ssid;

typedef struct ieee80211_neighbor_info {
    u_int32_t   phymode;                      /* ap channel width*/
    int         rssi;                         /* ap singal strength */
    u_int8_t    bssid[IEEE80211_ADDR_LEN];    /* BSSID information */
    u_int8_t    ssid_len;                     /* length of the ssid */
    u_int8_t    ssid[IEEE80211_NWID_LEN + 1]; /* SSID details */
    u_int8_t    qbssload_ie_valid;            /* Flag to indicate if qbss load ie is present */
    u_int8_t    station_count;                /* number of station associated */
    u_int8_t    channel_utilization;          /* channel busy time in 0-255 scale */
    int8_t      power_info;                   /* Indoor or Standard power info */
} ieee80211_neighbor_info;

#if DBDC_REPEATER_SUPPORT
#define DBDC_REPEATER_LOOP_DETECT_TIME_DELAY 10
#endif

#define PPDU_STATS_VERSION_1 1
#define PPDU_STATS_VERSION_2 2
#define PPDU_STATS_VERSION_3 3

#define UNSIGNED_LONG_LONG_ONE (uint64_t)1
#define BITS_PER_BYTE 8

typedef struct ieee80211_tx_status {
    int         ts_flags;
#define IEEE80211_TX_ERROR          0x01
#define IEEE80211_TX_XRETRY         0x02
#define IEEE80211_TX_FLUSH          0x04
#if ATH_DATA_TX_INFO_EN
/*per-msdu tx flag*/
#define IEEE80211_TX_DROP           0x08
#define IEEE80211_TX_FIRST_MSDU     0x10
#define IEEE80211_TX_LAST_MSDU      0x20
#define IEEE80211_TX_SUCCESS        0x40
#endif
#define IEEE80211_TX_ERROR_NO_SKB_FREE      0x0100
#define IEEE80211_SKB_FREE_ONLY             0x0200
    int         ts_retries;     /* number of retries to successfully transmit this frame */
#ifdef ATH_SUPPORT_TxBF
    u_int8_t    ts_txbfstatus;
#define	AR_BW_Mismatch      0x1
#define	AR_Stream_Miss      0x2
#define	AR_CV_Missed        0x4
#define AR_Dest_Miss        0x8
#define AR_Expired          0x10
#define AR_TxBF_Valid_HW_Status    (AR_BW_Mismatch|AR_Stream_Miss|AR_CV_Missed|AR_Dest_Miss|AR_Expired)
#define TxBF_STATUS_Sounding_Complete   0x20
#define TxBF_STATUS_Sounding_Request    0x40
#define TxBF_Valid_SW_Status  (TxBF_STATUS_Sounding_Complete | TxBF_STATUS_Sounding_Request)
#define TxBF_Valid_Status  (AR_TxBF_Valid_HW_Status | TxBF_Valid_SW_Status)
    u_int32_t    ts_tstamp;     /* tx time stamp */
#endif
#if ATH_SUPPORT_FLOWMAC_MODULE
    u_int8_t    ts_flowmac_flags;
#define IEEE80211_TX_FLOWMAC_DONE           0x01
#endif
    u_int32_t    ts_rateKbps;
#if ATH_DATA_TX_INFO_EN
    /*per-msdu tx info,
      keep these heavily used members together on same cache line
    */
    u_int32_t    msdu_length;    /*msdu length*/
    u_int32_t    msdu_priority;  /*msdu priority*/
    u_int32_t    ch_width;       /*channel width*/
    u_int64_t    msdu_q_time;    /*time spent in tx queue: tx_complete_time-queue_time */

    /*per-ppdu tx info*/
    u_int32_t    ppdu_rate;     /*ppdu rate in ratecode*/
    u_int8_t     ppdu_num_mpdus_success;
    u_int8_t     ppdu_bw_idx;
    u_int8_t     ppdu_num_mpdus_fail;
    u_int16_t    ppdu_num_msdus_success;
    u_int32_t    ppdu_bytes_success;
    u_int32_t    ppdu_duration; /*ppdu estimated air time*/
    u_int8_t     ppdu_retries;
    u_int8_t     ppdu_is_aggregate;
    u_int16_t    start_seq_num;
    u_int16_t    version;
    u_int32_t    ppdu_ack_timestamp;
    u_int32_t    ppdu_bmap_enqueued_lo;  /* bitmap of enqueued MPDUs (LSBs)*/
    u_int32_t    ppdu_bmap_enqueued_hi;  /* bitmap of enqueued MPDUs (MSBs)*/
    u_int32_t    ppdu_bmap_tried_lo;     /* bitmap of MPDUs tried OTA (LSBs)*/
    u_int32_t    ppdu_bmap_tried_hi;     /* bitmap of MPDUs tried OTA (MSBs)*/
    u_int32_t    ppdu_bmap_failed_lo;    /* bitmap of unacked MPDUs (LSBs)*/
    u_int32_t    ppdu_bmap_failed_hi;    /* bitmap of unacked MPDUs (MSBs)*/
#endif
} ieee80211_xmit_status;

#define RX_PHY_DATA_RADAR 0x01

#ifndef EXTERNAL_USE_ONLY
typedef struct ieee80211_rx_status {
    int         rs_numchains;
    int         rs_flags;
#define IEEE80211_RX_FCS_ERROR      0x01
#define IEEE80211_RX_MIC_ERROR      0x02
#define IEEE80211_RX_DECRYPT_ERROR  0x04
/* holes in flags here between, ATH_RX_XXXX to IEEE80211_RX_XXX */
#define IEEE80211_RX_KEYMISS        0x200
#define IEEE80211_RX_PN_ERROR       0x400
    int         rs_rssi;       /* RSSI (noise floor ajusted) */
    int         rs_abs_rssi;   /* absolute RSSI */
    int         rs_datarate;   /* data rate received */
    int         rs_ratephy1;
    int         rs_ratephy2;
    int         rs_ratephy3;
    u_int32_t   rs_lsig_word;


#define IEEE80211_MAX_ANTENNA       3                /* Keep the same as ATH_MAX_ANTENNA */
    u_int8_t    rs_snrctl[IEEE80211_MAX_ANTENNA];   /* SNR */
    u_int8_t    rs_snrextn[IEEE80211_MAX_ANTENNA];  /* SNR */
    u_int8_t    rs_isvalidsnr;                      /* rs_snr is valid or not */

    enum ieee80211_phymode rs_phymode;
    int                    rs_freq;        /*in MHz e.g. 2412 MHz*/
    enum wlan_band_id      rs_band;
    uint8_t                rs_channum;

    union {
        u_int8_t            data[8];
        u_int64_t           tsf;
    } rs_tstamp;

    /*
     * Detail channel structure of recv frame.
     * It could be NULL if not available
     */
    struct ieee80211_ath_channel *rs_full_chan;
    struct ieee80211_ath_channel rs_full_chan_obj;

#if ATH_VOW_EXT_STATS
    u_int32_t vow_extstats_offset; /* Lower 16 bits holds the udp checksum offset in the data pkt */
                                   /* Higher 16 bits contains offset in the data pkt at which vow ext stats are embedded */
#endif
    u_int8_t rs_isaggr;
    u_int16_t  rs_channel;
#ifdef ATH_SUPPORT_TxBF
    u_int32_t   rs_rpttstamp;   /* txbf report time stamp*/
#endif

    /* The following counts are meant to assist in stats calculation.
       These variables are incremented only in specific situations, and
       should not be relied upon for any purpose other than the original
       stats related purpose they have been introduced for. */

    u_int16_t   rs_cryptodecapcount; /* Crypto bytes decapped/demic'ed. */
    u_int8_t    rs_qosdecapcount;    /* QoS/HTC bytes decapped. */

    /* End of stats calculation related counts. */

    uint8_t rs_lsig[IEEE80211_LSIG_LEN];
    uint8_t rs_htsig[IEEE80211_HTSIG_LEN];
    uint8_t rs_servicebytes[IEEE80211_SB_LEN];
    uint8_t rs_fcs_error;
    uint8_t rs_peer_valid;
    struct ieee80211com *rs_ic;    /* for use in umac in the rx path */
    struct ieee80211_node *rs_ni;    /* for use in umac in the rx path */
    uint8_t rs_snr; /* SNR */
    uint16_t ba_win_size;
    uint16_t reo_win_size;
#ifdef WLAN_FEATURE_11BE_MLO
    struct mlo_mgmt_ml_info rs_cu;
    struct mlo_bcast_t2lm_info rs_t2lm;
    struct mgmt_rx_mlo_link_removal_info *link_removal_info;
    int num_link_removal_info;
#endif
    uint8_t *twt_ie_data;
    uint16_t twt_ie_len;
    bool twt_ie_meta;
} ieee80211_recv_status;

#endif /* EXTERNAL_USE_ONLY */

#if ATH_DATA_RX_INFO_EN && MESH_MODE_SUPPORT
#error "can only have ATH_DATA_RX_INFO_EN or MESH_MODE_SUPPORT should be enabled but not both."
#endif

#define IEEE80211_RX_FIRST_MSDU     0x01
#define IEEE80211_RX_LAST_MSDU      0x02
#if ATH_DATA_RX_INFO_EN
typedef struct per_msdu_data_recv_status {
    int         rs_flags;
/* first_msdu=1, last_msdu=1  ==> non-Aggregation frame*/
/* first_msdu=1, last_msdu=0  ==> first msdu*/
/* first_msdu=0, last_msdu=0  ==> msdu frame between first and last msdu*/
/* first_msdu=0, last_msdu=1  ==> last msdu*/
    int         rs_rssi;       /* RSSI (noise floor ajusted) */
    int         rs_ratephy1;
    int         rs_ratephy2;
    int         rs_ratephy3;
    int16_t     rs_noisefloor;
    uint8_t     rs_snr;
} per_msdu_data_rx_status;
#endif



/*
 * flags to be passed to ieee80211_vap_create function .
 */
#define IEEE80211_CLONE_BSSID           0x0001  /* allocate unique mac/bssid */
#define IEEE80211_CLONE_NOBEACONS       0x0002  /* don't setup beacon timers */
#define IEEE80211_CLONE_WDS             0x0004  /* enable WDS processing */
#define IEEE80211_CLONE_WDSLEGACY       0x0008  /* legacy WDS operation */
#define IEEE80211_PRIMARY_VAP           0x0010  /* primary vap */
#define IEEE80211_P2PDEV_VAP            0x0020  /* p2pdev vap */
#define IEEE80211_P2PGO_VAP             0x0040  /* p2p-go vap */
#define IEEE80211_P2PCLI_VAP            0x0080  /* p2p-client vap */
#define IEEE80211_CLONE_MACADDR         0x0100  /* create vap w/ specified mac/bssid */
#define IEEE80211_CLONE_MATADDR         0x0200  /* create vap w/ specified MAT addr */
#define IEEE80211_WRAP_VAP              0x0400  /* wireless repeater ap vap */
#define IEEE80211_WRAP_WIRED_STA        0x0800  /* wired qwrap sta */
#define IEEE80211_SPECIAL_VAP           0x1000  /* special vap mode */
#define IEEE80211_MESH_VAP              0x2000  /* mesh vap mode */
#define IEEE80211_SMART_MONITOR_VAP     0x4000  /* smart monitor vap */
#if ATH_NON_BEACON_AP
#define IEEE80211_NON_BEACON_AP         0x8000  /* non-beaconing AP */
#endif
#define IEEE80211_LP_IOT_VAP            0x10000 /* AP mode for IOT clients */
#define IEEE80211_WRAP_NON_MAIN_STA     0x20000  /* Non-Main proxy sta*/
#define IEEE80211_MONITOR_LITE_VAP      0x40000  /* Monitor mode for lite sniffer */
#define IEEE80211_PPE_VP_NO_ACCEL	0x80000 /* PPE VP accel support */
#define IEEE80211_PPE_VP_PASSIVE_ACCEL	0x100000 /* PPE VP accel support */
#define IEEE80211_PPE_VP_ACTIVE_ACCEL	0x200000 /* PPE VP accel support */
#define IEEE80211_PPE_VP_DS_ACCEL	0x400000 /* PPE VP accel support */
#define IEEE80211_MLO_BRIDGE_VAP        0x800000 /* MLO bridge vap */
#define IEEE80211_VAPID_SET             0x1000000 /* vapid parameter passed from user-space */
#define IEEE80211_PPE_VP_ACCEL_CLEAR(_vap) ((_vap)->cp.icp_flags &= ~(IEEE80211_PPE_VP_NO_ACCEL |\
								      IEEE80211_PPE_VP_PASSIVE_ACCEL |\
								      IEEE80211_PPE_VP_ACTIVE_ACCEL |\
								      IEEE80211_PPE_VP_DS_ACCEL))

/*
 * For the new multi-vap scan feature, there is a set of default priority tables
 * for each OpMode.
 * The following are the default list of the VAP Scan Priority Mapping based on OpModes.
 * NOTE: the following are only used when "#if ATH_SUPPORT_MULTIPLE_SCANS" is true.
 */
/* For IBSS opmode */
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_IBSS_BASE               0
/* For STA opmode */
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE                0
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_P2P_CLIENT          1
/* For HostAp opmode */
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_AP_BASE                 0
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_AP_P2P_GO               1
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_AP_P2P_DEVICE           2
/* For BTAmp opmode */
#define DEF_VAP_SCAN_PRI_MAP_OPMODE_BTAMP_BASE              0

typedef enum _ieee80211_dev_vap_event {
    IEEE80211_VAP_CREATED = 1,
    IEEE80211_VAP_STOPPED,
    IEEE80211_VAP_DELETED,
    IEEE80211_VAP_STOP_ERROR
} ieee80211_dev_vap_event;

typedef struct _wlan_dev_event_handler_table {
    void (*wlan_dev_vap_event)  (void *event_arg, wlan_dev_t, os_if_t, ieee80211_dev_vap_event);  /* callback to receive vap events*/
#if WLAN_SPECTRAL_ENABLE
    void (*wlan_dev_spectral_indicate)(void*, void*, u_int32_t);
#endif
    /* Radar detected event */
    void (*wlan_radar_event)    (void *event_arg, struct ieee80211com *ic);
    /* Per radio watch dog event */
    void (*wlan_wdt_event)      (void *event_arg, struct ieee80211com *ic, u_int32_t reason);
    /* DFS CAC start event */
    void (*wlan_cac_start_event) (void *event_arg, struct ieee80211com *ic);
    /* VAP up after CAC event */
    void (*wlan_up_after_cac_event) (void *event_arg, struct ieee80211com *ic);
    /* channel utilisation event */
    void (*wlan_chan_util_event) (struct ieee80211com *ic, u_int8_t self_util, u_int8_t obss_util);
} wlan_dev_event_handler_table;

typedef enum _ieee80211_ap_stopped_reason {
    IEEE80211_AP_STOPPED_REASON_DUMMY = 0,          /* Dummy placeholder. Should not use */
    IEEE80211_AP_STOPPED_REASON_CHANNEL_DFS = 1,
} ieee80211_ap_stopped_reason;

typedef uint16_t IEEE80211_REASON_CODE;
typedef uint16_t IEEE80211_STATUS;

#ifndef EXTERNAL_USE_ONLY
#include <wbuf.h> /* wbuf_t */
typedef struct _wlan_event_handler_table {
    void (*wlan_receive) (os_if_t osif, wbuf_t wbuf,
                          u_int16_t type, u_int16_t subtype,
                          ieee80211_recv_status *rs);                       /* callback to receive all the packets */
    int  (*wlan_receive_filter_80211) (os_if_t osif, wbuf_t wbuf,
                                       u_int16_t type, u_int16_t subtype,
                                       ieee80211_recv_status *rs);          /* callback to receive and filter all the 802.11 frames*/
    void (*wlan_receive_monitor_80211) (os_if_t osif, wbuf_t wbuf,
                                        ieee80211_recv_status *rs);         /* recieve 802.11 frames for monitor mode only*/
    int  (*wlan_dev_xmit_queue) (os_if_t osif, wbuf_t wbuf);                /* queue packet to the device for transmit */
    void (*wlan_vap_xmit_queue) (os_if_t osif, wbuf_t wbuf);                /* queue packet to the interface for transmit */
    void (*wlan_xmit_update_status)(os_if_t osif, wbuf_t wbuf,
                                    ieee80211_xmit_status *ts);             /* callback to indicate tx completion.
                                                                             * NB: this is to allow IHV to update certain statistics,
                                                                             * it should NOT consume the wbuf */
#if ATH_SUPPORT_IWSPY
	void (*wlan_iwspy_update)(os_if_t osif, u_int8_t *address, u_int8_t snr); /* IWSPY support, update snr */
#endif
#if ATH_SUPPORT_FLOWMAC_MODULE
    /* pause: 1-pause, 0-wake */
    void (*wlan_pause_queue)(os_if_t osif, int pause, unsigned int pctl);
#endif
    void (*wlan_vap_scan_cancel) (os_if_t osif);                            /* Cancel STA vap scan */
    void (*wlan_bringup_ap_vaps) (os_if_t osif);       /* Bringup pending ap vaps */
} wlan_event_handler_table;

typedef struct _wlan_mlme_event_handler_table {
    /* MLME confirmation handler */
    void (*mlme_join_complete_set_country)(os_handle_t, IEEE80211_STATUS);
    void (*mlme_join_complete_infra)(os_handle_t, IEEE80211_STATUS);
    void (*mlme_join_complete_adhoc)(os_handle_t, IEEE80211_STATUS);
    void (*mlme_auth_complete)(os_handle_t, u_int8_t *macaddr,IEEE80211_STATUS);
    void (*mlme_assoc_req)(os_handle_t, wbuf_t wbuf);
    void (*mlme_assoc_complete)(os_handle_t, IEEE80211_STATUS, u_int16_t aid, wbuf_t wbuf);
    void (*mlme_reassoc_complete)(os_handle_t, IEEE80211_STATUS, u_int16_t aid, wbuf_t wbuf);
    void (*mlme_deauth_complete)(os_handle_t,u_int8_t *, IEEE80211_STATUS, u_int16_t associd, u_int16_t reason, u_int8_t *);
    void (*mlme_disassoc_complete)(os_handle_t, u_int8_t *, u_int32_t, IEEE80211_STATUS, u_int8_t *);
    void (*mlme_txchanswitch_complete)(os_handle_t,IEEE80211_STATUS);
    void (*mlme_repeater_cac_complete)(os_handle_t,IEEE80211_STATUS);

    /* MLME indication handler */
    void (*mlme_auth_indication)(os_handle_t, u_int8_t *macaddr, u_int16_t result);
    void (*mlme_deauth_indication)(os_handle_t, u_int8_t *macaddr, u_int16_t associd, u_int16_t reason_code, u_int8_t *mldaddr);
    void (*mlme_assoc_indication)(os_handle_t, u_int8_t *macaddr, u_int16_t result, wbuf_t wbuf, wbuf_t resp_wbuf, bool reassoc, u_int8_t *mldaddr);
    void (*mlme_reassoc_indication)(os_handle_t, u_int8_t *macaddr, u_int16_t result, wbuf_t wbuf, wbuf_t resp_wbuf, bool reassoc, u_int8_t *mldaddr);
    void (*mlme_disassoc_indication)(os_handle_t, u_int8_t *macaddr, u_int16_t associd, u_int32_t reason_code, u_int8_t *mldaddr);
    void (*mlme_ibss_merge_start_indication)(os_handle_t, u_int8_t *bssid);
    void (*mlme_ibss_merge_completion_indication)(os_handle_t, u_int8_t *bssid);
    void (*wlan_radar_detected) (os_handle_t, u_int32_t csa_delay);
    void (*wlan_node_authorized_indication) (os_handle_t, u_int8_t *macaddr);
    void (*mlme_unprotected_deauth_indication)(os_handle_t, u_int8_t *macaddr, u_int16_t associd, u_int16_t reason_code, wbuf_t wbuf);
} wlan_mlme_event_handler_table;

typedef struct _wlan_misc_event_handler_table {
    void (*wlan_channel_change) (os_handle_t, wlan_chan_t chan);
    void (*wlan_country_changed) (os_handle_t, char *country);
    void (*wlan_linkspeed)(os_handle_t, u_int32_t rxlinkspeed, u_int32_t txlinkspeed);
    void (*wlan_michael_failure_indication)(os_handle_t, const u_int8_t *ta_mac_addr,
                                            u_int keyix);
    void (*wlan_replay_failure_indication)(os_handle_t, const u_int8_t *frm, u_int keyix);
    void (*wlan_beacon_miss_indication)(os_handle_t);
    void (*wlan_beacon_rssi_indication)(os_handle_t);
    void (*wlan_device_error_indication)(os_handle_t);
    void (*wlan_sta_clonemac_indication)(os_handle_t);
    void (*wlan_sta_scan_entry_update)(os_handle_t, wlan_scan_entry_t, bool);
    void (*wlan_ap_stopped)(os_handle_t, ieee80211_ap_stopped_reason reason);
#if ATH_SUPPORT_WAPI
    void (*wlan_sta_rekey_indication)(os_handle_t, u_int8_t *macaddr);
#endif
#if UMAC_SUPPORT_RRM_MISC
    void (*wlan_channel_load) (os_handle_t, u_int8_t chload);
    void (*wlan_nonerpcnt) (os_handle_t, u_int8_t erpcnt);
    void (*wlan_bgjoin) (os_handle_t, u_int8_t val);
    void (*wlan_cochannelap_cnt) (os_handle_t, u_int8_t val);
#endif
#if QCA_SUPPORT_SON
    void (*wlan_buffull)(os_handle_t);
#endif
    void (*wlan_session_timeout)(os_handle_t, u_int8_t *macaddr);
#if ATH_SUPPORT_MGMT_TX_STATUS
    void (*wlan_mgmt_tx_status)(os_handle_t, IEEE80211_STATUS, wbuf_t wbuf);
#endif
#if QCA_SUPPORT_SSID_STEERING
    void (*ssid_event) (os_handle_t, u_int8_t *macaddr);
#endif
    void (*assocdeny_event) (os_handle_t, u_int8_t *macaddr);
    void (*wlan_tx_overflow) (os_handle_t);
    void (*wlan_ch_hop_channel_change) (os_handle_t, u_int8_t channel);
    void (*wlan_recv_probereq) (os_handle_t, u_int8_t *macaddr, u_int8_t *ssid_element);
#if DBDC_REPEATER_SUPPORT
    void (*stavap_connection) (os_handle_t);
#endif
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    void (*wlan_sta_cac_started) (os_handle_t, u_int32_t frequency, u_int32_t timeout);
#endif
    void (*chan_util_event) (os_handle_t, u_int32_t self_util, u_int32_t obss_util);
    void (*wlan_keyset_done_indication) (os_handle_t, u_int8_t *macaddr, u_int8_t status);
    void (*wlan_blklst_sta_auth_indication) (os_handle_t, u_int8_t *macaddr, u_int8_t status);
#if ATH_PARAMETER_API
    void (*papi_event) (os_handle_t, PAPI_REPORT_TYPE, uint32_t eventlen, const char *data);
#endif
    int (*wlan_bss_node_freed_indication)(struct ieee80211vap *vap);
    void (*wlan_recv_neighreq) (os_handle_t, u_int8_t *macaddr, void *nreq);
    void (*wlan_recv_beaconrpt) (os_handle_t, void *bcnrpt, uint32_t len);
    void (*wlan_recv_bstm) (os_handle_t, void *bstm_data, uint32_t len);
    void (*wlan_recv_smpsupdate) (os_handle_t, void *smps_data, uint32_t len);
    void (*wlan_recv_opmodeupdate) (os_handle_t, void *opmode_data, uint32_t len);
    void (*wlan_vdev_state_up_active_indication) (os_handle_t);
    void (*wlan_recv_awgninterferenceupdate) (os_handle_t, void *awgn_data, uint32_t len);
    void (*wlan_channel_switch_started) (os_handle_t, wlan_chan_t chan);
} wlan_misc_event_handler_table;

typedef struct _wlan_ccx_handler_table {
    void (*wlan_ccx_trigger_roam) (os_if_t,  u_int16_t reason);
    void (*wlan_ccx_set_vperf) (os_if_t, u_int8_t);
    void (*wlan_ccx_fill_tsrsie)(os_if_t, u_int8_t tid, u_int32_t rate, u_int8_t *frm, u_int8_t *len);
    bool (*wlan_ccx_validate_bss)(os_if_t, wlan_scan_entry_t, int);
    void (*wlan_ccx_process_qos)(os_if_t, u_int8_t, u_int16_t);
    bool (*wlan_ccx_check_msdu_life)(os_if_t, u_int16_t);
    void (*wlan_ccx_vperf_pause)(os_if_t, bool);
} wlan_ccx_handler_table;

/* action frame complete callback*/
typedef void (*wlan_action_frame_complete_handler)(wlan_if_t vaphandle, wbuf_t wbuf, void *arg, u_int8_t *dst_addr, u_int8_t *src_addr, u_int8_t *bssid, ieee80211_xmit_status *ts);

/*
 *
 *  wlan_dev_xmit      : the OS dependent layer needs to register a callback function.
 *                       umac layer uses this callback to xmit a packet. the implementaion
 *                       of this  call back function would implement OS specific handling of
 *                       the wbuf and at the end it if it decides to xmit the wbuf it should
 *                       call the ath_tx_send (the ath layer xmit funtion). the implementation
 *                       should return 0 on success and -ve value on failure.
 *
 *  wlan_vap_xmit      : the OS dependent layer needs to register a callback function.
 *                       umac layer uses this callback to xmit a packet. the implementaion
 *                       of this  call back function would implement send the packet on to the
 *                       transmit queue of the vap. if IEEE80211_DELIVER_80211 param is set the
 *                       frame is 802.11 frame else  it is ethernet (802.3) frame.
 *
 *  wlan_receive       : handler to receive all the packets icluding data,mgmt,control.
 *                       UMAC will deliver standard 802.11 frames (with qos control removed)
 *                       if IEEE80211_DELIVER_80211 param is set.
 *                       UMAC will deliver ethernet frames (with 802.11 header decapped)
 *                       if IEEE80211_DELIVER_80211 param is not set.
 *                       the handler should cosume the wbuf (release it eventually).
 *
 *  wlan_receive_filter_80211 : handler to receive all the frames with 802.11 header unstripped but the FCS stripped at the end.
 *                              implementation of the handler should make a copy of the wbuf and should not
 *                              release the wbuf passed via the handler. implementation should return non zero if the packet
 *                              should not be processed further by umac.
 *
 *  wlan_receive_monitor_80211 : handler to receive the  all the frames with the 802.11 header including the FCS.
 *                               the handler should cosume the wbuf (release it eventually).
 *                               this function is exclusively used for monitor mode.
 *
 *
 */
#endif /* EXTERNAL_USE_ONLY */
/*
 * scan API related structs.
 */
typedef enum _ieee80211_scan_type {
    IEEE80211_SCAN_BACKGROUND,
    IEEE80211_SCAN_FOREGROUND,
    IEEE80211_SCAN_SPECTRAL,
    IEEE80211_SCAN_REPEATER_BACKGROUND,
    IEEE80211_SCAN_REPEATER_EXT_BACKGROUND,
    IEEE80211_SCAN_RADIO_MEASUREMENTS,
} ieee80211_scan_type;

/*
 * Priority numbers must be sequential, starting with 0.
 */
typedef enum ieee80211_scan_priority_t {
    IEEE80211_SCAN_PRIORITY_VERY_LOW    = 0,
    IEEE80211_SCAN_PRIORITY_LOW,
    IEEE80211_SCAN_PRIORITY_MEDIUM,
    IEEE80211_SCAN_PRIORITY_HIGH,
    IEEE80211_SCAN_PRIORITY_VERY_HIGH,

    IEEE80211_SCAN_PRIORITY_COUNT   /* number of priorities supported */
} IEEE80211_SCAN_PRIORITY;

typedef u_int16_t    IEEE80211_SCAN_REQUESTOR;
typedef u_int32_t    IEEE80211_SCAN_ID;

#define IEEE80211_SCAN_ID_NONE                    0

/* All P2P scans currently use medium priority */
#define IEEE80211_P2P_DEFAULT_SCAN_PRIORITY       IEEE80211_SCAN_PRIORITY_MEDIUM
#define IEEE80211_P2P_SCAN_PRIORITY_HIGH          IEEE80211_SCAN_PRIORITY_HIGH

/* Masks identifying types/ID of scans */
#define IEEE80211_SPECIFIC_SCAN       0x00000000
#define IEEE80211_VAP_SCAN            0x01000000
#define IEEE80211_ALL_SCANS           0x04000000

/**
 * host scan bit. only relevant for host/target architecture.
 * do not reuse this bit definition. target uses this .
 *
 */
#define IEEE80211_HOST_SCAN           0x80000000
#define IEEE80211_SCAN_CLASS_MASK     0xFF000000

#define IEEE80211_SCAN_PASSIVE            0x0001 /* passively scan all the channels */
#define IEEE80211_SCAN_ACTIVE             0x0002 /* actively  scan all the channels (regdomain rules still apply) */
#define IEEE80211_SCAN_2GHZ               0x0004 /* scan 2GHz band */
#define IEEE80211_SCAN_5GHZ               0x0008 /* scan 5GHz band */
#define IEEE80211_SCAN_ALLBANDS           (IEEE80211_SCAN_5GHZ | IEEE80211_SCAN_2GHZ)
#define IEEE80211_SCAN_CONTINUOUS         0x0010 /* keep scanning until maxscantime expires */
#define IEEE80211_SCAN_FORCED             0x0020 /* forced scan (OS request) - should proceed even in the presence of data traffic */
#define IEEE80211_SCAN_NOW                0x0040 /* scan now (User request)  - should proceed even in the presence of data traffic */
#define IEEE80211_SCAN_ADD_BCAST_PROBE    0x0080 /* add wildcard ssid and broadcast probe request if there is none */
#define IEEE80211_SCAN_EXTERNAL           0x0100 /* scan requested by OS */
#define IEEE80211_SCAN_BURST              0x0200 /* scan multiple channels before returning to BSS channel */
#define IEEE80211_SCAN_CHAN_EVENT         0x0400 /* scan chan event for  offload architectures */
#define IEEE80211_SCAN_OFFCHAN_MGMT_TX    0x0800 /* allow mgmt transmission during off channel scan */
#define IEEE80211_SCAN_OFFCHAN_DATA_TX    0x1000 /* allow data transmission during off channel scan */
#define IEEE80211_SCAN_80MHZ              0x2000 /* support to scan VHT80/40 */
#define IEEE80211_SCAN_PROM_MODE          0x4000 /* force scan with promiscous mode */
#define IEEE80211_SCAN_PHY_ERR            0x8000 /* capture pkts with PHY errors */
#define IEEE80211_SCAN_HALF_RATE          0x10000 /* Enable Half rate scan */
#define IEEE80211_SCAN_QUARTER_RATE       0x20000 /* Enable Quarter rate scan */

#define IEEE80211_SCAN_PARAMS_MAX_SSID     10
#define IEEE80211_SCAN_PARAMS_MAX_BSSID    10


/* flag definitions passed to scan_cancel API */

#define IEEE80211_SCAN_CANCEL_ASYNC 0x0 /* asynchronouly wait for scan SM to complete cancel */
#define IEEE80211_SCAN_CANCEL_WAIT  0x1 /* wait for scan SM to complete cancel */
#define IEEE80211_SCAN_CANCEL_SYNC  0x2 /* synchronously execute cancel scan */
/* Scan Parameters for Enhanced-Independent-Repeater */
#define MIN_REST_TIME 500
#define MAX_REST_TIME 600
#define MIN_DWELL_TIME_ACTIVE 150
#define MAX_DWELL_TIME_ACTIVE 200
#define INIT_REST_TIME 14000
#define MAX_DWELL_TIME_PASSIVE 200
#define DWELL_TIME_PASSIVE_LESS_BCN_LOSS 105

#ifndef EXTERNAL_USE_ONLY
typedef bool (*ieee80211_scan_termination_check) (void *arg);

typedef struct _ieee80211_scan_params {
    ieee80211_scan_type                 type;
    int                                 min_dwell_time_active;  /* min time in msec on active channels */
    int                                 max_dwell_time_active;  /* max time in msec on active channels (if no response) */
    int                                 min_dwell_time_passive; /* min time in msec on passive channels */
    int                                 max_dwell_time_passive; /* max time in msec on passive channels (if no response) */
    int                                 min_rest_time;          /* min time in msec on the BSS channel, only valid for BG scan */
    int                                 init_rest_time;         /*  used by EIRPTR - min time in msec on the BSS channel, only valid for BG scan */
    int                                 scan_offset_time;       /* Offset time from the last beacon after which scan should start*/
    int                                 max_rest_time;          /* max time in msec on the BSS channel, only valid for BG scan */
    int                                 max_offchannel_time;    /* max time away from BSS channel, in ms */
    int                                 repeat_probe_time;      /* time before sending second probe request */
    int                                 idle_time;              /* time in msec on bss channel before switching channel */
    int                                 max_scan_time;          /* maximum time in msec allowed for scan  */
    int                                 probe_delay;            /* delay in msec before sending probe request */
    int                                 offchan_retry_delay;    /* delay in msec before retrying off-channel switch */
    int                                 min_beacon_count;       /* number of home AP beacons to receive before leaving the home channel */
    int                                 max_offchan_retries;    /* maximum number of times to retry off-channel switch */
    int                                 beacon_timeout;         /* maximum time to wait for beacons */
    int                                 flags;                  /* scan flags */
    int                                 num_channels;           /* number of channels to scan */
    bool                                multiple_ports_active;  /* driver has multiple ports active in the home channel */
    bool                                restricted_scan;        /* Perform restricted scan */
    bool                                chan_list_allocated;
    IEEE80211_SCAN_PRIORITY             p2p_scan_priority;      /* indicates the scan priority if this is a P2P-related scan */
    u_int32_t                           *chan_list;             /* array of ieee channels (or) frequencies to scan */
    int                                 num_ssid;               /* number of desired ssids */
    ieee80211_ssid                      ssid_list[IEEE80211_SCAN_PARAMS_MAX_SSID];
    int                                 num_bssid;              /* number of desired bssids */
    u_int8_t                            bssid_list[IEEE80211_SCAN_PARAMS_MAX_BSSID][IEEE80211_ADDR_LEN];
    struct ieee80211_node               *bss_node;              /* BSS node */
    int                                 ie_len;                 /* length of the ie data to be added to probe req */
    u_int8_t                            *ie_data;               /* pointer to ie data */
    ieee80211_scan_termination_check    check_termination_function;  /* function checking for termination condition */
    void                                *check_termination_context;  /* context passed to function above */
#if QCA_LTEU_SUPPORT
    u_int32_t                           probe_spacing_time;     /* time in msec between 2 consecutive probe requests */
#endif
} ieee80211_scan_params;

/* Data types used to specify scan priorities */
typedef u_int32_t IEEE80211_PRIORITY_MAPPING[IEEE80211_SCAN_PRIORITY_COUNT];

/**************************************
 * Called before attempting to roam.  Modifies the rssiAdder of a BSS
 * based on the preferred status of a BSS.
 *
 * According to CCX spec, AP in the neighbor list is not meant for giving extra
 * weightage in roaming. By doing so, roaming becomes sticky. See bug 21220.
 * Change the weightage to 0. Cisco may ask in future for a user control of
 * this weightage.
 */
#define PREFERRED_BSS_RANK                20
#define NEIGHBOR_BSS_RANK                  0    /* must be less than preferred BSS rank */

/*
 * The utility of the BSS is the metric used in the selection
 * of a BSS. The Utility of the BSS is reduced if we just left the BSS.
 * The Utility of the BSS is not reduced if we have left the
 * BSS for 8 seconds (8000ms) or more.
 * 2^13 milliseconds is a close approximation to avoid expensive division
 */
#define LAST_ASSOC_TIME_DELTA_REQUIREMENT (1 << 13) // 8192

#define QBSS_SCALE_MAX                   255  /* Qbss channel load Max value */
#define QBSS_SCALE_DOWN_FACTOR             2  /* scale factor to reduce Qbss channel load */
#define QBSS_HYST_ADJ                     60  /* Qbss Weightage factor for the current AP */

#ifdef CONFIG_WIFI_EMULATION_WIFI_3_0
#define BAD_AP_TIMEOUT   0
#else
#define BAD_AP_TIMEOUT   6000   // In milli seconds
#endif
/*
 * To disable BAD_AP status check on any scan entry
 */
#define BAD_AP_TIMEOUT_DISABLED             0

/*
 * BAD_AP timeout specified in seconds
 */
#define BAD_AP_TIMEOUT_IN_SECONDS           10

/*
 * State values used to represent our assoc_state with ap (discrete, not bitmasks)
 */
#define AP_ASSOC_STATE_NONE     0
#define AP_ASSOC_STATE_AUTH     1
#define AP_ASSOC_STATE_ASSOC    2

/*
 * Entries in the scan list are considered obsolete after 75 seconds.
 */
#define IEEE80211_SCAN_ENTRY_EXPIRE_TIME           75000

#define TIME_UNIT_TO_MICROSEC   1024    /* 1 TU equals 1024 microsecs */

#ifndef MILLISEC_TO_MICROSEC
#define MILLISEC_TO_MICROSEC    1000U
#endif /* MILLISEC_TO_MICROSEC */

/*
 * idle time is only valid for scan type IEEE80211_SCAN_BACKGROUND.
 * if idle time is set then the scanner would change channel from BSS
 * channel to foreign channel only if both resttime is expired and
 * the theres was not traffic for idletime msec on the bss channel.
 * value of 0 for idletime would cause the channel to switch from BSS
 * channel to foreign channel as soon  as the resttime is expired.
 *
 * if maxscantime is nonzero and if the scanner can not complete the
 * scan in maxscantime msec then the scanner will cancel the scan and
 * post IEEE80211_SCAN_COMPLETED event with reason SCAN_TIMEDOUT.
 *
 */

/*
 * chanlist can be either ieee channels (or) frequencies.
 * if a value is less than 1000 implementation assumes it
 * as ieee channel # otherwise implementation assumes it
 * as frequency in Mhz.
 */

typedef enum _ieee80211_scan_event_type {
    IEEE80211_SCAN_STARTED,
    IEEE80211_SCAN_COMPLETED,
    IEEE80211_SCAN_RADIO_MEASUREMENT_START,
    IEEE80211_SCAN_RADIO_MEASUREMENT_END,
    IEEE80211_SCAN_RESTARTED,
    IEEE80211_SCAN_HOME_CHANNEL,
    IEEE80211_SCAN_FOREIGN_CHANNEL,
    IEEE80211_SCAN_BSSID_MATCH,
    IEEE80211_SCAN_FOREIGN_CHANNEL_GET_NF,
    IEEE80211_SCAN_DEQUEUED,
    IEEE80211_SCAN_PREEMPTED,

    IEEE80211_SCAN_EVENT_COUNT
} ieee80211_scan_event_type;

#define IEEE80211_SCAN_CHAN_SHIFT       0
#define IEEE80211_SCAN_CHAN_MASK        0xffff
#define IEEE80211_SCAN_CHAN_MODE_SHIFT  16
#define IEEE80211_SCAN_CHAN_MODE_MASK   0xff

typedef enum _ieee80211_scan_chan_mode {
   IEEE80211_SCAN_CHAN_MODE_NONE,
   IEEE80211_SCAN_CHAN_MODE_11NA_HT20,
   IEEE80211_SCAN_CHAN_MODE_11NG_HT20,
   IEEE80211_SCAN_CHAN_MODE_11NA_HT40,
   IEEE80211_SCAN_CHAN_MODE_11NG_HT40,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT20,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT40,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT80,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT160,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT80_80,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT20_2G,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT40_2G,
   IEEE80211_SCAN_CHAN_MODE_11AC_VHT80_2G,
} ieee80211_scan_chan_mode_t;

typedef enum ieee80211_scan_completion_reason {
    IEEE80211_REASON_NONE,
    IEEE80211_REASON_COMPLETED,
    IEEE80211_REASON_CANCELLED,
    IEEE80211_REASON_TIMEDOUT,
    IEEE80211_REASON_TERMINATION_FUNCTION,
    IEEE80211_REASON_MAX_OFFCHAN_RETRIES,
    IEEE80211_REASON_PREEMPTED,
    IEEE80211_REASON_RUN_FAILED,
    IEEE80211_REASON_INTERNAL_STOP,

    IEEE80211_REASON_COUNT
} ieee80211_scan_completion_reason;

typedef struct _ieee80211_scan_event {
    ieee80211_scan_event_type           type;
    ieee80211_scan_completion_reason    reason;
    wlan_chan_t                         chan;
    IEEE80211_SCAN_REQUESTOR            requestor; /* Requestor ID passed to the scan_start function */
    IEEE80211_SCAN_ID                   scan_id;   /* Specific ID of the scan reporting the event */
} ieee80211_scan_event;

typedef enum _ieee80211_scan_request_status {
    IEEE80211_SCAN_STATUS_QUEUED,
    IEEE80211_SCAN_STATUS_RUNNING,
    IEEE80211_SCAN_STATUS_PREEMPTED,
    IEEE80211_SCAN_STATUS_COMPLETED
} ieee80211_scan_request_status;

/*
 * the sentry field of tht ieee80211_scan_event is only valid if the
 * event type is IEEE80211_SCAN_BSSID_MATCH.
 */

typedef void (*ieee80211_scan_event_handler) (wlan_if_t vaphandle, ieee80211_scan_event *event, void *arg);

typedef struct _ieee80211_scan_info {
    ieee80211_scan_type                type;
    IEEE80211_SCAN_REQUESTOR           requestor;                   /* Originator ID passed to the scan_start function */
    IEEE80211_SCAN_ID                  scan_id;                     /* Specific ID of the scan reporting the event */
    IEEE80211_SCAN_PRIORITY            priority;                    /* Requested priority level (low/medium/high) */
    ieee80211_scan_request_status      scheduling_status;           /* Queued/running/preempted/completed */
    int                                min_dwell_time_active;       /* min time in msec on active channels */
    int                                max_dwell_time_active;       /* max time in msec on active channel (if no response) */
    int                                min_dwell_time_passive;      /* min time in msec on passive channels */
    int                                max_dwell_time_passive;      /* max time in msec on passive channel*/
    int                                min_rest_time;               /* min time in msec on the BSS channel, only valid for BG scan */
    int                                max_rest_time;               /* max time in msec on the BSS channel, only valid for BG scan */
    int                                max_offchannel_time;         /* max time away from BSS channel, in ms */
    int                                repeat_probe_time;           /* time before sending second probe request */
    int                                min_beacon_count;            /* number of home AP beacons to receive before leaving the home channel */
    int                                flags;                       /* scan flags */
    systime_t                          scan_start_time;             /* system time when last scani started */
    int                                scanned_channels;            /* number of scanned channels */
    int                                default_channel_list_length; /* number of channels in the default channel list */
    int                                channel_list_length;         /* number of channels in the channel list used for the current scan */
    int                                in_progress            : 1,  /* if the scan is in progress */
                                       cancelled              : 1,  /* if the scan is cancelled */
                                       preempted              : 1,  /* if the scan is preempted */
                                       restricted             : 1;  /* if the scan is restricted */
} ieee80211_scan_info;

typedef struct _ieee80211_scan_request_info {
    wlan_if_t                        vaphandle;
    IEEE80211_SCAN_REQUESTOR         requestor;
    IEEE80211_SCAN_PRIORITY          requested_priority;
    IEEE80211_SCAN_PRIORITY          absolute_priority;
    IEEE80211_SCAN_ID                scan_id;
    ieee80211_scan_request_status    scheduling_status;
    ieee80211_scan_params            params;
    systime_t                        request_timestamp;
    u_int32_t                        maximum_duration;
} ieee80211_scan_request_info;

#endif /* EXTERNAL_USE_ONLY */

#ifndef EXTERNAL_USE_ONLY
struct wlan_objmgr_vdev;
typedef void (*ieee80211_acs_event_handler) (struct wlan_objmgr_vdev *vdev,
                                             void *arg, QDF_STATUS status);
typedef void (*ieee80211_acs_scantimer_handler) (void *arg);
#endif /* EXTERNAL_USE_ONLY */

#define MAX_CHAINS 4

typedef struct _wlan_snr_info {
    u_int8_t    avg_snr;     /* average snr */
    u_int8_t    valid_mask;   /* bitmap of valid elements in rssi_ctrl/ext array */
    int8_t      snr_ctrl[MAX_CHAINS];
    int8_t      snr_ext[MAX_CHAINS];
} wlan_snr_info;

typedef enum _wlan_snr_type {
    WLAN_SNR_TX,
    WLAN_SNR_RX,
    WLAN_SNR_BEACON,  /* snr of the beacon, only valid for STA/IBSS vap */
    WLAN_SNR_RX_DATA
} wlan_snr_type;

typedef enum _ieee80211_rate_type {
    IEEE80211_RATE_TYPE_LEGACY,
    IEEE80211_RATE_TYPE_MCS,
} ieee80211_rate_type;

typedef struct _ieee80211_rate_info {
    ieee80211_rate_type    type;
    u_int32_t              rate;     /* Instantaneous packet rate in kbps */
    u_int32_t              lastrate; /* last packet rate in kbps */
    u_int8_t               mcs;      /* mcs index . is valid if rate type is MCS20 or MCS40 */
    u_int32_t              maxrate_per_client;
    u_int8_t               flags;
    u_int32_t              avg_rate; /* average rate in kbps */
} ieee80211_rate_info;

typedef enum _ieee80211_node_param_type {
    IEEE80211_NODE_PARAM_TX_POWER,
    IEEE80211_NODE_PARAM_ASSOCID,
    IEEE80211_NODE_PARAM_INACT,     /* inactivity timer value */
    IEEE80211_NODE_PARAM_AUTH_MODE, /* auth mode */
    IEEE80211_NODE_PARAM_CAP_INFO,  /* auth mode */
} ieee80211_node_param_type;

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_RU_INDEX_MAX 16
#define IEEE80211_DOT11_MAX 6
#define IEEE80211_MAX_MCS (16 + 1)
#else
#define IEEE80211_RU_INDEX_MAX 6
#define IEEE80211_DOT11_MAX 5
#define IEEE80211_MAX_MCS (14 + 1)
#endif

/*
 * Per/node (station) statistics available when operating as an AP.
 */
struct ieee80211_nodestats {
    /* All below fields are moved to cp_stats component */
    int8_t       ns_rx_mgmt_rssi;         /* mgmt frame rssi */
    u_int32_t    ns_rx_mgmt;              /* rx management frames */
    u_int32_t    ns_rx_noprivacy;         /* rx w/ wep but privacy off */
    u_int32_t    ns_rx_wepfail;           /* rx wep processing failed */
    u_int32_t    ns_rx_ccmpmic;           /* rx CCMP MIC failure */
    u_int32_t    ns_rx_wpimic;            /* rx WAPI MIC failure */
    u_int32_t    ns_rx_tkipicv;           /* rx ICV check failed (TKIP) */
    u_int32_t    ns_rx_pnerr;             /* rx PN error */
    u_int32_t    ns_tx_mgmt;              /* tx management frames */
    u_int32_t    ns_is_tx_not_ok;         /* tx not ok */
    u_int32_t    ns_ps_discard;           /* ps discard 'cuz of age */
    u_int32_t    ns_last_rx_mgmt_rate;         /* last received mgmt frame rate */
    u_int32_t    ns_psq_drops;           /* power save queue drops */
#ifdef ATH_SUPPORT_IQUE
    u_int32_t    ns_tx_dropblock;
#endif
    u_int32_t    ns_tx_assoc;            /* [re]associations */
    u_int32_t    ns_tx_assoc_fail;       /* [re]association failures */
    u_int32_t    ns_is_tx_nobuf;
    u_int32_t    ns_rx_decryptcrc;        /* rx decrypt failed on crc */
    /* end of cp stats fields */
    u_int32_t    ns_rx_data;             /* rx data frames */

    u_int32_t    ns_rx_ctrl;             /* rx control frames */
    u_int32_t    ns_rx_ucast;            /* rx unicast frames */
    u_int32_t    ns_rx_mcast;            /* rx multi/broadcast frames */
    u_int64_t    ns_rx_bytes;            /* rx data count (bytes) */
    u_int64_t    ns_last_per;            /* last packet error rate */
    u_int64_t    ns_rx_beacons;          /* rx beacon frames */
    u_int32_t    ns_rx_proberesp;        /* rx probe response frames */

    u_int32_t    ns_rx_dup;              /* rx discard 'cuz dup */
    u_int32_t    ns_rx_demicfail;        /* rx demic failed */

    /* We log MIC and decryption failures against Transmitter STA stats.
       Though the frames may not actually be sent by STAs corresponding
       to TA, the stats are still valuable for some customers as a sort
       of rough indication.
       Also note that the mapping from TA to STA may fail sometimes. */
    u_int32_t    ns_rx_tkipmic;           /* rx TKIP MIC failure */
    u_int32_t    ns_rx_decap;            /* rx decapsulation failed */
    u_int32_t    ns_rx_defrag;           /* rx defragmentation failed */
    u_int32_t    ns_rx_disassoc;         /* rx disassociation */
    u_int32_t    ns_rx_deauth;           /* rx deauthentication */
    u_int32_t    ns_rx_action;           /* rx action */
    u_int32_t    ns_rx_unauth;           /* rx on unauthorized port */
    u_int32_t    ns_rx_unencrypted;      /* rx unecrypted w/ privacy */

    u_int32_t    ns_tx_data;             /* tx data frames */
    u_int32_t    ns_tx_data_success;     /* tx data frames successfully
                                            transmitted (unicast only) */
    u_int64_t    ns_tx_wme[4];           /* tx data per AC */
    u_int64_t    ns_rx_wme[4];           /* rx data per AC */
    u_int32_t    ns_tx_ucast;            /* tx unicast frames */
    u_int32_t    ns_tx_mcast;            /* tx multi/broadcast frames */
    u_int32_t    ns_tx_bcast;            /* tx broadcast frames */
    u_int64_t    ns_tx_bytes;            /* tx data count (bytes) */
    u_int64_t    ns_tx_bytes_success;    /* tx success data count - unicast only
                                            (bytes) */
    u_int32_t    ns_tx_probereq;         /* tx probe request frames */
    u_int32_t    ns_tx_uapsd;            /* tx on uapsd queue */
    u_int32_t    ns_tx_discard;          /* tx dropped by NIC */
    u_int32_t    ns_tx_last_ack_rssi;
    u_int32_t    ns_tx_novlantag;        /* tx discard 'cuz no tag */
    u_int32_t    ns_tx_vlanmismatch;     /* tx discard 'cuz bad tag */

    u_int32_t    ns_tx_eosplost;         /* uapsd EOSP retried out */

    u_int32_t    ns_uapsd_triggers;      /* uapsd triggers */
    u_int32_t    ns_uapsd_duptriggers;    /* uapsd duplicate triggers */
    u_int32_t    ns_uapsd_ignoretriggers; /* uapsd duplicate triggers */
    u_int32_t    ns_uapsd_active;         /* uapsd duplicate triggers */
    u_int32_t    ns_uapsd_triggerenabled; /* uapsd duplicate triggers */
    u_int32_t    ns_last_tx_rate;         /* last tx data rate */
    u_int32_t    ns_last_rx_rate;         /* last received data frame rate */

    /* MIB-related state */
    u_int32_t    ns_dot11_tx_bytes;
    u_int32_t    ns_dot11_rx_bytes;
#if ATH_SUPPORT_EXT_STAT
    u_int32_t    ns_tx_bytes_rate;
    u_int32_t    ns_tx_data_rate;
    u_int32_t    ns_rx_bytes_rate;
    u_int32_t    ns_rx_data_rate;
    u_int32_t    ns_tx_bytes_success_last;
    u_int32_t    ns_tx_data_success_last;
    u_int32_t    ns_rx_bytes_last;
    u_int32_t    ns_rx_data_last;
#endif
    u_int32_t    ns_tx_auth;             /* [re]authentications */
    u_int32_t    ns_tx_auth_fail;        /* [re]authentication failures*/
    u_int32_t    ns_tx_deauth;           /* deauthentications */
    u_int32_t    ns_tx_deauth_code;      /* last deauth reason */
    u_int32_t    ns_tx_disassoc;         /* disassociations */
    u_int32_t    ns_tx_disassoc_code;    /* last disassociation reason */
    int32_t    ns_rssi_chain[RSSI_MAX_CHAINS]; /* Ack RSSI per chain */
    u_int32_t   inactive_time;
    u_int32_t   ns_tx_mu_blacklisted_cnt;  /* number of time MU tx has been blacklisted */
    u_int64_t   ns_excretries[WME_NUM_AC]; /* excessive retries */
#if UMAC_SUPPORT_STA_STATS_ENHANCEMENT
    u_int64_t    ns_rx_ucast_bytes;      /* tx bytes of unicast frames */
    u_int64_t    ns_rx_mcast_bytes;      /* tx bytes of multi/broadcast frames */
    u_int64_t    ns_tx_ucast_bytes;      /* tx bytes of unicast frames */
    u_int64_t    ns_tx_mcast_bytes;      /* tx bytes of multi/broadcast frames */
    u_int64_t    ns_tx_bcast_bytes;      /* tx bytes of broadcast frames */
#endif
    u_int64_t    ns_rx_mpdus;            /* Number of rs mpdus */
    u_int64_t    ns_rx_ppdus;            /* Number of ppdus */
    u_int64_t    ns_rx_retries;          /* Number of rx retries */
    u_int64_t    ns_tx_mpdus_success;
    u_int64_t    ns_tx_ppdus;
    u_int64_t    ns_tx_mpdus_tried;
    u_int32_t    ns_mpdu_retries;
    u_int32_t    ns_total_mpdu_retries;
    u_int32_t    ns_ppdu_tx_rate;        /* Avg per ppdu tx rate in kbps */
    u_int32_t    ns_ppdu_rx_rate;        /* Avg per ppdu rx rate in kbps */
    u_int32_t    ns_rssi;                /* most recent received packet RSSI from connected sta used for MAP*/
    u_int32_t    ns_rx_avg_rssi;
    u_int32_t    ns_rssi_time_delta;     /* time delta in ms between rssi measurement and IOCTL call */
    /* No of packets not trans successfully due to no of retrans attempts exceeding 802.11 retry limit */
    u_int32_t ns_failed_retry_count;
    /* No of packets that were successfully transmitted after one or more retransmissions */
    u_int32_t ns_retry_count;
    /* No of packets that were successfully transmitted after more than one retransmission */
    u_int32_t ns_multiple_retry_count;
    u_int32_t ru_tx[IEEE80211_RU_INDEX_MAX];
    u_int32_t mcs_tx[IEEE80211_DOT11_MAX][IEEE80211_MAX_MCS];
    u_int32_t ru_rx[IEEE80211_RU_INDEX_MAX];
    u_int32_t mcs_rx[IEEE80211_DOT11_MAX][IEEE80211_MAX_MCS];

#ifdef VDEV_PEER_PROTOCOL_COUNT
    u_int16_t icmp_tx_ingress;
    u_int16_t icmp_tx_egress;
    u_int16_t icmp_rx_ingress;
    u_int16_t icmp_rx_egress;

    u_int16_t arp_tx_ingress;
    u_int16_t arp_tx_egress;
    u_int16_t arp_rx_ingress;
    u_int16_t arp_rx_egress;

    u_int16_t eap_tx_ingress;
    u_int16_t eap_tx_egress;
    u_int16_t eap_rx_ingress;
    u_int16_t eap_rx_egress;
#endif
    /* TWT stats */
#if WLAN_SUPPORT_TWT
    u_int32_t ns_tx_data_success_twt;     /* tx data frames successfully TX */
    u_int32_t ns_rx_data_twt;             /* rx data frames */
    u_int32_t ns_twt_event_type; /* TWT session type */
    u_int32_t ns_twt_flow_id:16, /* TWT flow id */
              ns_twt_bcast:1,    /* Broadcast TWT */
              ns_twt_trig:1,     /* TWT trigger */
              ns_twt_announ:1;   /* TWT announcement */
    u_int32_t ns_twt_dialog_id;  /* TWT diag ID */
    u_int32_t ns_twt_wake_dura_us; /* Wake time duration in us */
    u_int32_t ns_twt_wake_intvl_us; /* Interval between wake perions in us */
    u_int32_t ns_twt_sp_offset_us;  /* Time until first TWT SP occurs */
#endif
    u_int32_t ns_avg_rssi; /* avg rssi*/
    u_int32_t ns_avg_ack_rssi; /* avg rssi*/
    int32_t noise_floor;

    u_int32_t    ns_max_rate;         /* max rate supported */
};
#define IEEE80211_MBO_NUM_NONPREF_CHAN_ATTR     6     /* 2 bands and 3 values are available for preference */
#define IEEE80211_MBO_NUM_NONPREF_CHANLIST      16    /* Max number of channels mntioned in 802.11mc spec */

typedef struct {
    u_int8_t               operating_class;
    u_int8_t               num_channels;
    u_int8_t               channels[IEEE80211_MBO_NUM_NONPREF_CHANLIST];
    u_int8_t               channels_preference;
    u_int8_t               reason_preference;
} mbo_nonpref_channel;

struct ieee80211_mbo_attributes {
    u_int8_t               cellular_cap;
    mbo_nonpref_channel    channel[IEEE80211_MBO_NUM_NONPREF_CHAN_ATTR];
    u_int8_t               num_attr;
    u_int8_t               oce_sta;
    u_int8_t               oce_cap;
    u_int8_t               trans_reject_code;
};

/*
 * station power save mode.
 */
typedef enum ieee80211_psmode {
    IEEE80211_PWRSAVE_NONE = 0,          /* no power save */
    IEEE80211_PWRSAVE_LOW,
    IEEE80211_PWRSAVE_NORMAL,
    IEEE80211_PWRSAVE_MAXIMUM,
    IEEE80211_PWRSAVE_WNM                /* WNM-Sleep Mode */
} ieee80211_pwrsave_mode;

/* station power save pspoll handling */
typedef enum {
  IEEE80211_CONTINUE_PSPOLL_FOR_MORE_DATA,
  IEEE80211_WAKEUP_FOR_MORE_DATA,
} ieee80211_pspoll_moredata_handling;

/*
 * apps power save state.
 */
typedef enum {
     APPS_AWAKE = 0,
     APPS_PENDING_SLEEP,
     APPS_SLEEP,
     APPS_FAKE_SLEEP,           /* Pending blocking sleep */
     APPS_FAKING_SLEEP,         /* Blocking sleep */
     APPS_UNKNOWN_PWRSAVE,
} ieee80211_apps_pwrsave_state;

typedef enum _iee80211_mimo_powersave_mode {
    IEEE80211_MIMO_POWERSAVE_NONE,    /* no mimo power save */
    IEEE80211_MIMO_POWERSAVE_STATIC,  /* static mimo power save */
    IEEE80211_MIMO_POWERSAVE_DYNAMIC  /* dynamic mimo powersave */
} ieee80211_mimo_powersave_mode;

typedef enum _ieee80211_cap {
    IEEE80211_CAP_SHSLOT,                    /* CAPABILITY: short slot */
    IEEE80211_CAP_SHPREAMBLE,                /* CAPABILITY: short premable */
    IEEE80211_CAP_MULTI_DOMAIN,              /* CAPABILITY: multiple domain */
    IEEE80211_CAP_WMM,                       /* CAPABILITY: WMM */
    IEEE80211_CAP_HT,                        /* CAPABILITY: HT */
    IEEE80211_CAP_PERF_PWR_OFLD,             /* CAPABILITY: power performance offload support */
    IEEE80211_CAP_11AC,                      /* CAPABILITY: 11ac support */
} ieee80211_cap;

typedef enum _ieee80211_device_param {
	IEEE80211_DEVICE_RSSI_CTL,
    IEEE80211_DEVICE_NUM_TX_CHAIN,
    IEEE80211_DEVICE_NUM_RX_CHAIN,
    IEEE80211_DEVICE_TX_CHAIN_MASK,
    IEEE80211_DEVICE_RX_CHAIN_MASK,
    IEEE80211_DEVICE_TX_CHAIN_MASK_LEGACY,
    IEEE80211_DEVICE_RX_CHAIN_MASK_LEGACY,
    IEEE80211_DEVICE_BMISS_LIMIT,            /* # of beacon misses for HW to generate BMISS intr */
    IEEE80211_DEVICE_PROTECTION_MODE,        /* protection mode*/
    IEEE80211_DEVICE_CWM_EXTPROTMODE,
    IEEE80211_DEVICE_CWM_EXTPROTSPACING,
    IEEE80211_DEVICE_CWM_ENABLE,
    IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD,
    IEEE80211_DEVICE_DOTH,
    IEEE80211_DEVICE_ADDBA_MODE,
    IEEE80211_DEVICE_COUNTRYCODE,
    IEEE80211_DEVICE_MULTI_CHANNEL,           /* turn on/off off channel support */
    IEEE80211_DEVICE_MAX_AMSDU_SIZE,          /* Size of AMSDU to be sent on the air */
    IEEE80211_DEVICE_P2P,                     /* Enable or Disable P2P */
    IEEE80211_DEVICE_OVERRIDE_SCAN_PROBERESPONSE_IE, /* Override scan Probe response IE, 0: Don't over-ride */
    IEEE80211_DEVICE_2G_CSA,
    IEEE80211_DEVICE_PWRTARGET,
    IEEE80211_DEVICE_OFF_CHANNEL_SUPPORT,
    IEEE80211_DEVICE_GREEN_AP_ENABLE_PRINT,
} ieee80211_device_param;

typedef enum _ieee80211_param {
    IEEE80211_BEACON_INTVAL,                 /* in TUs */
#if ATH_SUPPORT_AP_WDS_COMBO
    IEEE80211_NO_BEACON,                     /* the vap does not tx beacon/probe resp. */
#endif
    IEEE80211_LISTEN_INTVAL,                 /* number of beacons */
    IEEE80211_DTIM_INTVAL,                   /* number of beacons */
    IEEE80211_BMISS_COUNT_RESET,             /* number of beacon miss intrs before reset */
    IEEE80211_BMISS_COUNT_MAX,               /* number of beacon miss intrs for bmiss notificationst */
    IEEE80211_ATIM_WINDOW,                   /* ATIM window */
    IEEE80211_SHORT_SLOT,                    /* short slot on/off */
    IEEE80211_SHORT_PREAMBLE,                /* short preamble on/off */
    IEEE80211_RTS_THRESHOLD,                 /* rts threshold, 0 means no rts threshold  */
    IEEE80211_FRAG_THRESHOLD,                /* fragmentation threshold, 0 means no rts threshold  */
    IEEE80211_FIXED_RATE,                    /*
                                              * rate code series(0: auto rate, 32 bit value:  rate
                                              * codes for 4 rate series. each byte for one rate series)
                                              */
    IEEE80211_MCAST_RATE,                    /* rate in Kbps */
    IEEE80211_TXPOWER,                       /* in 0.5db units */
    IEEE80211_AMPDU_DENCITY,                 /* AMPDU dencity*/
    IEEE80211_AMPDU_LIMIT,                   /* AMPDU limit*/
    IEEE80211_MAX_AMPDU,                     /* Max AMPDU Exp*/
    IEEE80211_VHT_MAX_AMPDU,                 /* VHT Max AMPDU Exp */
    IEEE80211_WPS_MODE,                      /* WPS mode*/
    IEEE80211_TSN_MODE,                      /* TSN mode*/
    IEEE80211_MULTI_DOMAIN,                  /* Multiple domain */
    IEEE80211_SAFE_MODE,                     /* Safe mode */
    IEEE80211_NOBRIDGE_MODE,                 /* No bridging done, all frames sent up the stack */
    IEEE80211_PERSTA_KEYTABLE_SIZE,          /* IBSS-only, read-only: persta key table size */
    IEEE80211_RECEIVE_80211,                 /* deliver std 802.11 frames 802.11 instead of ethernet frames on the rx */
    IEEE80211_SEND_80211,                    /* OS sends std 802.11 frames 802.11 instead of ethernet frames on tx side */
    IEEE80211_MIN_BEACON_COUNT,              /* minumum number beacons to tx/rx before vap can pause */
    IEEE80211_IDLE_TIME,                     /* minimun no activity time before vap can pause */
    IEEE80211_MIN_FRAMESIZE,                 /* smallest frame size we are allowed to receive */
                                             /* features. 0:feature is off. 1:feature is on. */
    IEEE80211_FEATURE_WMM,                   /* WMM */
    IEEE80211_FEATURE_WMM_PWRSAVE,           /* WMM Power Save */
    IEEE80211_FEATURE_UAPSD,                 /* UAPSD setting (BE/BK/VI/VO) */
    IEEE80211_FEATURE_PRIVACY,               /* encryption */
    IEEE80211_FEATURE_DROP_UNENC,            /* drop un encrypted frames */
    IEEE80211_FEATURE_COUNTER_MEASURES ,     /* turn on couter measures */
    IEEE80211_FEATURE_HIDE_SSID,             /* turn on hide ssid feature */
    IEEE80211_FEATURE_APBRIDGE,              /* turn on internal mcast traffic bridging for AP */
    IEEE80211_FEATURE_PUREB,                 /* turn on pure B mode for AP */
    IEEE80211_FEATURE_PUREG,                 /* turn on pure G mode for AP */
    IEEE80211_FEATURE_REGCLASS,              /* add regulatory class IE in AP */
    IEEE80211_FEATURE_COUNTRY_IE,            /* add country IE for vap in AP */
    IEEE80211_FEATURE_IC_COUNTRY_IE,         /* add country IE for ic in AP */
    IEEE80211_FEATURE_DOTH,                  /* enable 802.11h */
    IEEE80211_FEATURE_PURE11N,               /* enable pure 11n  mode */
    IEEE80211_FEATURE_PRIVATE_RSNIE,         /* enable OS shim to setup RSN IE*/
    IEEE80211_FEATURE_COPY_BEACON,           /* keep a copy of beacon */
    IEEE80211_FEATURE_PSPOLL,                /* enable/disable pspoll mode in power save SM */
    IEEE80211_FEATURE_CONTINUE_PSPOLL_FOR_MOREDATA, /* enable/disable option to contunue sending ps polls when there is more data */
    IEEE80211_FEATURE_AMPDU,                 /* Enable or Disable Aggregation */
    IEEE80211_FIXED_RETRIES,                 /* fixed retries  0-4 */
    IEEE80211_SHORT_GI,                      /* short gi on/off */
    IEEE80211_HT40_INTOLERANT,
    IEEE80211_CHWIDTH,
    IEEE80211_CHEXTOFFSET,
    IEEE80211_DISABLE_2040COEXIST,
    IEEE80211_DISABLE_HTPROTECTION,
    IEEE80211_STA_QUICKKICKOUT,
    IEEE80211_CHSCANINIT,
    IEEE80211_FEATURE_STAFWD,                /* dynamic AP Client  feature */
    IEEE80211_DRIVER_CAPS,
    IEEE80211_DRIVER_HW_CAPS,                /* hw caps */
    IEEE80211_BANDWIDTH,
    IEEE80211_FREQ_BAND,
    IEEE80211_EXTCHAN,
    IEEE80211_SECOND_CENTER_FREQ,
#if ATH_SUPPORT_IQUE
    IEEE80211_IQUE_CONFIG,
    IEEE80211_ME,                            /* mcast enhancement mode */
	IEEE80211_MEDEBUG,
	IEEE80211_MEDUMP,
	IEEE80211_ME_SNOOPLENGTH,
	IEEE80211_ME_TIMEOUT,
	IEEE80211_ME_DROPMCAST,
    IEEE80211_ME_SHOWDENY,
    IEEE80211_ME_CLEARDENY,
    IEEE80211_ME_ADDDENY,
    IEEE80211_HBR_TIMER,
#endif //ATH_SUPPORT_IQUE
    IEEE80211_UAPSD_MAXSP,                    /* UAPSD service period setting (0:unlimited, 2,4,6) */
    IEEE80211_WEP_MBSSID,
    IEEE80211_MGMT_RATE,                      /* ieee rate to be used for management*/
    IEEE80211_RTSCTS_RATE,                    /* basic supported rate to be used for rts and cts*/
    IEEE80211_NON_BASIC_RTSCTS_RATE,           /* non basic supported rate to be used for rts and cts*/
    IEEE80211_RESMGR_VAP_AIR_TIME_LIMIT,      /* When multi-channel enabled, restrict air-time allocated to a VAP */

    IEEE80211_AUTO_ASSOC,
    IEEE80211_PROTECTION_MODE,                /* per VAP protection mode*/
    IEEE80211_AUTH_INACT_TIMEOUT,             /* inactivity time while waiting for 802.11x auth to complete */
    IEEE80211_INIT_INACT_TIMEOUT,             /* inactivity time while waiting for 802.11 auth/assoc to complete */
    IEEE80211_RUN_INACT_TIMEOUT,              /* inactivity time when fully authed*/
    IEEE80211_PROBE_INACT_TIMEOUT,            /* inactivity counter value below which starts probing */
    IEEE80211_QBSS_LOAD,
    IEEE80211_XBSS_LOAD,
    IEEE80211_HC_BSSLOAD,
    IEEE80211_OSEN,
    IEEE80211_WNM_CAP,
    IEEE80211_WNM_BSS_CAP,
    IEEE80211_WNM_TFS_CAP,
    IEEE80211_WNM_TIM_CAP,
    IEEE80211_WNM_SLEEP_CAP,
    IEEE80211_WNM_FMS_CAP,
    IEEE80211_WNM_BSTM_CAP,                   /* Enable/Disable BSTM capablity */
    IEEE80211_AP_REJECT_DFS_CHAN,             /* AP to reject resuming on DFS Channel */
	IEEE80211_COMP,
	IEEE80211_FF,
	IEEE80211_TURBO,
	IEEE80211_BURST,
	IEEE80211_AR,
	IEEE80211_SLEEP,
	IEEE80211_EOSPDROP,
	IEEE80211_MARKDFS,
	IEEE80211_DFSDOMAIN,
	IEEE80211_WDS_AUTODETECT,
	IEEE80211_WEP_TKIP_HT,
	IEEE80211_ATH_RADIO,
	IEEE80211_IGNORE_11DBEACON,
    /* Video debug feature */
    IEEE80211_VI_DBG_CFG,                     /* Video debug configuration - Bit0- enable dbg, Bit1 - enable stats log */
    IEEE80211_VI_DBG_NUM_STREAMS,             /* Total number of receive streams */
    IEEE80211_VI_STREAM_NUM,                  /* the stream number whose marker parameters are being set */
    IEEE80211_VI_DBG_NUM_MARKERS,             /* total number of markers used to filter pkts */
    IEEE80211_VI_MARKER_NUM,                  /* the marker number whose parameters (offset, size & match) are being set */
    IEEE80211_VI_MARKER_OFFSET_SIZE,          /* byte offset from skb start (upper 16 bits) & size in bytes(lower 16 bits) */
    IEEE80211_VI_MARKER_MATCH,                /* marker pattern match used in filtering */
    IEEE80211_VI_RXSEQ_OFFSET_SIZE,           /* Rx Seq num offset skb start (upper 16 bits) & size in bytes(lower 16 bits) */
    IEEE80211_VI_RX_SEQ_RSHIFT,               /* right-shift value in case field is not word aligned */
    IEEE80211_VI_RX_SEQ_MAX,                  /* maximum Rx Seq number (to check wrap around) */
    IEEE80211_VI_RX_SEQ_DROP,                 /* Indicator to the debug app that a particular seq num has been dropped */
    IEEE80211_VI_TIME_OFFSET_SIZE,            /* Timestamp offset skb start (upper 16 bits) & size in bytes(lower 16 bits) */
    IEEE80211_VI_RESTART,                     /* If set to 1 resets all internal variables/counters & restarts debug tool*/
    IEEE80211_VI_RXDROP_STATUS,               /* Total RX drops in wireless */
    IEEE80211_TRIGGER_MLME_RESP,              /* Option for App to trigger mlme response */
#ifdef ATH_SUPPORT_TxBF
    IEEE80211_TXBF_AUTO_CVUPDATE,             /* auto CV update enable */
    IEEE80211_TXBF_CVUPDATE_PER,              /* per threshold to initial CV update*/
#endif
    IEEE80211_MAX_CLIENT_NUMBERS,
	IEEE80211_SMARTNET,
    IEEE80211_FEATURE_MFP_TEST,               /* MFP test */
	IEEE80211_WEATHER_RADAR, 				  /* weather radar channel skip */
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    IEEE80211_CHAN_UTIL_ENAB,
    IEEE80211_CHAN_UTIL,                      /* Get Channel Utilization value (scale: 0 - 255) */
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */
    IEEE80211_WEP_KEYCACHE,                   /* WEP KEYCACHE is enable */
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
    IEEE80211_REJOINT_ATTEMP_TIME,
#endif
    IEEE80211_SEND_DEAUTH,                  /* send deauth instead of disassoc while doing interface down  */
    IEEE80211_PROXYARP_CAP,
    IEEE80211_DGAF_DISABLE,
    IEEE80211_SET_TXPWRADJUST,
    IEEE80211_RRM_CAP,
    IEEE80211_RRM_DEBUG,
    IEEE80211_RRM_STATS,
    IEEE80211_RRM_SLWINDOW,
    IEEE80211_RRM_CAP_IE,
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    IEEE80211_NOPBN,                       /* no push button notification */
#endif
#if ATH_SUPPORT_DSCP_OVERRIDE
	IEEE80211_DSCP_MAP_ID,
#endif
#if HOST_SW_TSO_SG_ENABLE
    IEEE80211_TSO_STATS_RESET_GET,
    IEEE80211_TSO_STATS_GET,
#endif
    IEEE80211_FEATURE_OFF_CHANNEL_SUPPORT,
    IEEE80211_FIXED_VHT_MCS,                  /* VHT mcs index */
    IEEE80211_FIXED_NSS,                      /* Spatial Streams count */
    IEEE80211_SUPPORT_LDPC,                   /* LDPC Support */
    IEEE80211_SUPPORT_TX_STBC,                /* TX STBC enable/disable */
    IEEE80211_SUPPORT_RX_STBC,                /* RX STBC enable/disable */
    IEEE80211_DEFAULT_KEYID,                  /* XMIT default key */
    IEEE80211_OPMODE_NOTIFY_ENABLE,           /* Op mode notification enable/disable */
    IEEE80211_ENABLE_RTSCTS,                  /* Enable/Disable RTS-CTS */
    IEEE80211_RC_NUM_RETRIES,
    IEEE80211_BCAST_RATE,                     /* Bcast Rate*/
    IEEE80211_VHT_TX_MCSMAP,                  /* VHT TX MCS Map */
    IEEE80211_VHT_RX_MCSMAP,                  /* VHT RX MCS Map */
#if WDS_VENDOR_EXTENSION
    IEEE80211_WDS_RX_POLICY,                  /* Rx filtering policy based on frame formats for vendor-specific WDS */
#endif
    IEEE80211_ENABLE_OL_STATS,                /* ENABLE/DISABLE Stats */
    IEEE80211_GET_ACS_STATE,                  /* get acs state */
    IEEE80211_GET_CAC_STATE,                  /* get cac state */
    IEEE80211_VHT_SUBFEE,                     /* Enable/Disable VHT SU Beamformee */
    IEEE80211_VHT_MUBFEE,                     /* Enable/Disable VHT MU Beamformee */
    IEEE80211_VHT_SUBFER,                     /* Enable/Disable VHT SU Beamformer */
    IEEE80211_VHT_MUBFER,                     /* Enable/Disable VHT MU Beamformer */
    IEEE80211_SUPPORT_IMPLICITBF,             /* Enable/Disable Implicit Beamforming support */
    IEEE80211_EXT_ACS_IN_PROGRESS,            /* Whether external auto channel selection is
                                                 in progress */
    IEEE80211_DESIRED_CHANNEL,                /* Get desired channel
                                                 corresponding to desired PHY
                                                 mode */
    IEEE80211_DESIRED_PHYMODE,                /* Get desired PHY mode */
    IEEE80211_SEND_ADDITIONAL_IES,            /* Enable/Disable Sending of Extra IEs to host */
    IEEE80211_START_ACS_REPORT,               /* acs scan report */
    IEEE80211_MIN_DWELL_ACS_REPORT,           /* min dwell for scan in acs scan report */
    IEEE80211_MAX_DWELL_ACS_REPORT,           /* max dwell for scan in acs scan report */
    IEEE80211_MAX_SCAN_TIME_ACS_REPORT,       /* max scan time for scan in acs scan report */
    IEEE80211_SET_CABQ_MAXDUR,                /* max tx percentage for cabq */
    IEEE80211_256QAM,                         /* Enable/disabel 256QAM rate support in 2G mode*/
    IEEE80211_11NG_VHT_INTEROP,                /* Enable/disabel 11NG VHT Interop */
    IEEE80211_ACS_ENABLE_CH_HOP,              /* Enabling/disabling channel hopping in acs*/
    IEEE80211_ACS_CH_HOP_LONG_DUR,            /* long duration timer in sec  used in ch hopping */
    IEEE80211_ACS_CH_HOP_NO_HOP_DUR,          /* NO Hop timer in sec  used in ch hopping */
    IEEE80211_ACS_CH_HOP_CNT_WIN_DUR,         /* cnt window timer in sec  used in ch hopping */
    IEEE80211_ACS_CH_HOP_NOISE_TH,            /* Noise threshold to be used in detection algo */
    IEEE80211_ACS_CH_HOP_CNT_TH,              /* cnt threshold used in ch hop algo */
    IEEE80211_MAX_SCANENTRY,
    IEEE80211_SCANENTRY_TIMEOUT,
    IEEE80211_FEATURE_PURE11AC,               /* enable pure 11ac  mode */
    IEEE80211_SCAN_MIN_DWELL,                 /*Scan MIN Dwell time set by user */
    IEEE80211_SCAN_MAX_DWELL,                 /*Scan MAX Dwell time set by user */
    IEEE80211_SCAN_REST_TIME,                 /*Scan rest time set by user */
    IEEE80211_SCAN_MODE,                      /*Scan mode set by user */
    IEEE80211_VAP_DSCP_PRIORITY,               /* VAP based DSCP priority */
#if QCA_LTEU_SUPPORT
    IEEE80211_SCAN_REPEAT_PROBE_TIME,         /* scan probe time, part of scan params */
    IEEE80211_SCAN_IDLE_TIME,                 /* scan idle time, part of scan params */
    IEEE80211_SCAN_PROBE_DELAY,               /* scan probe delay, part of scan params */
    IEEE80211_MU_DELAY,                       /* delay between channel change and MU start (for non-gpio) */
#endif
    IEEE80211_VHT_SGIMASK,                    /* Set VHT SGI MASK */
    IEEE80211_VHT80_RATEMASK,                 /* Set VHT80 Auto Rate MASK */
    IEEE80211_VHT_BF_STS_CAP,
    IEEE80211_VHT_BF_SOUNDING_DIM,
    IEEE80211_SESSION_TIMEOUT,  /* STA's session time */
#if ATH_PERF_PWR_OFFLOAD
    IEEE80211_VAP_TX_ENCAP_TYPE,              /* Used to set the Tx encapsulation
                                                 type of the VAP -
                                                 RAW/NWIFI/Ethernet */
    IEEE80211_VAP_RX_DECAP_TYPE,              /* Used to set the Rx encapsulation
                                                 type of the VAP -
                                                 RAW/NWIFI/Ethernet */
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    IEEE80211_RAWMODE_SIM_TXAGGR,             /* Enable/disable raw mode simulation
                                                 Tx A-MSDU aggregation */
    IEEE80211_RAWMODE_PKT_SIM_STATS,          /* Get Raw mode packet simulation
                                                 stats. */
    IEEE80211_CLR_RAWMODE_PKT_SIM_STATS,      /* Clear Raw mode packet simulation
                                                 stats. */
    IEEE80211_RAWMODE_SIM_DEBUG_LEVEL,        /* Set raw mode simulation debug level */
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
#endif /* ATH_PERF_PWR_OFFLOAD*/
    IEEE80211_PROXY_STA,                      /* Enable/Disable ProxySTA support */
	IEEE80211_BW_NSS_RATEMASK,                /* Set ratemask with specific Bandwidth and NSS */
#if RX_CHECKSUM_OFFLOAD
    IEEE80211_RX_CKSUM_ERR_STATS_GET,
    IEEE80211_RX_CKSUM_ERR_RESET_GET,
#endif /* RX_CHECKSUM_OFFLOAD */
    IEEE80211_TXRX_DBG_SET,
    IEEE80211_TXRX_FW_STATS,
#if QCA_AIRTIME_FAIRNESS
    IEEE80211_ATF_OPT,                        /* Enable/Disable AirTime feature */
    IEEE80211_ATF_PER_UNIT,                   /* Set percentage unit either 100 or 1000 */
    IEEE80211_ATF_DYNAMIC_ENABLE,             /* Enable/Disable ATF */
    IEEE80211_ATF_GROUPING,
    IEEE80211_ATF_PEER_REQUEST,
    IEEE80211_BWF_PEER_REQUEST,               /* Bandwidth fairness request */
    IEEE80211_ATF_SSID_SCHED_POLICY,   /* per ssid atf sched policy */
#endif
    IEEE80211_RX_FILTER_MONITOR,
    IEEE80211_PEER_POSITION_SET,
    IEEE80211_VAP_TXRX_FW_STATS,
    IEEE80211_TXRX_FW_MSTATS,
    IEEE80211_VAP_TXRX_FW_STATS_RESET,
    IEEE80211_TXRX_FW_STATS_RESET,
    IEEE80211_CTSPROT_DTIM_BCN_SET,
#if HOST_SW_SG_ENABLE
    IEEE80211_SG_STATS_RESET_GET,
    IEEE80211_SG_STATS_GET,
#endif /* HOST_SW_SG_ENABLE */
    IEEE80211_TX_PPDU_LOG_CFG_SET,
    IEEE80211_AMSDU_SET,
    IEEE80211_AMSDU_GET,
    IEEE80211_AMPDU_SET,
    IEEE80211_AMPDU_GET,
    IEEE80211_RATE_DROPDOWN_SET,
    IEEE80211_PEER_MUMIMO_TX_COUNT_RESET_SET,
    IEEE80211_RX_FILTER_NEIGHBOUR_PEERS_MONITOR,
    IEEE80211_TXRX_VAP_STATS,
    IEEE80211_FEATURE_STRICT_BW,             /* enable strict bw restriction in pure 11ac  mode */
    IEEE80211_SMART_MESH_CONFIG,             /* smart MESH configuration */
#if QCA_LTEU_SUPPORT
    IEEE80211_WIFI_TX_POWER,                 /* assumed tx power of wifi sta */
#endif
    IEEE80211_CONFIG_ASSOC_WAR_160W,         /* Configure association WAR for 160 MHz width
                                                (i.e. 160/80+80 MHz modes)
                                                starting from LSB
                                                First bit set        = Default WAR behavior (VHT_OP modified)
                                                First+second bit set = (VHT_OP+ VHT_CAP modified)
                                                No bit set (default) = WAR disabled
					      */
    IEEE80211_RAWMODE_PKT_SIM,              /* Raw mode simulation */
    IEEE80211_CONFIG_RAW_DWEP_IND,           /* Enable/Disable indication that Dynamic WEp
                                                is being used in RAW mode */
    IEEE80211_CONFIG_PARAM_CUSTOM_CHAN_LIST,
#if UMAC_SUPPORT_ACFG
    IEEE80211_CONFIG_DIAG_WARN_THRESHOLD,    /* Diagnostics warning threshold */
    IEEE80211_CONFIG_DIAG_ERR_THRESHOLD,    /* Diagnostics error threshold */
#endif
    IEEE80211_MBO,                            /* Enable MBO */
    IEEE80211_MBOCAP,                         /* Enable MBO CAP */
    IEEE80211_MBO_ASSOC_DISALLOW,             /* MBO DISALLOW assoc for MBO */
    IEEE80211_MBO_CELLULAR_PREFERENCE,        /* MBO cellular preference */
    IEEE80211_MBO_TRANSITION_REASON,          /* MBO Tansition reason */
    IEEE80211_MBO_ASSOC_RETRY_DELAY,          /* MBO  assoc retry delay */
    IEEE80211_CONFIG_REV_SIG_160W,            /* enable/disable revised signalling for 160/80+80 MHz */
    IEEE80211_CONFIG_MU_CAP_TIMER,            /* mu cap timer period configuration */
    IEEE80211_CONFIG_MU_CAP_WAR,              /* enable/disable mup cap war function*/
    IEEE80211_CONFIG_DISABLE_SELECTIVE_HTMCS,           /* Enable/Disable selective HT-MCS for vap */
    IEEE80211_CONFIG_CONFIGURE_SELECTIVE_VHTMCS,           /* Enable/Disable selective VHT-MCS for vap */
    IEEE80211_PEER_TX_COUNT_SET,
    IEEE80211_CONFIG_RDG_ENABLE,
    IEEE80211_CONFIG_DFS_SUPPORT,
    IEEE80211_CONFIG_DFS_ENABLE,
    IEEE80211_CONFIG_ACS_SUPPORT,
    IEEE80211_CONFIG_SSID_STATUS,
    IEEE80211_CONFIG_DL_QUEUE_PRIORITY_SUPPORT,
    IEEE80211_CONFIG_CLEAR_MIN_MAX_SNR,
    IEEE80211_CONFIG_CLEAR_QOS,
    IEEE80211_CONFIG_TRAFFIC_STATS,  /* Enable/Disable the measurement of Traffic Statistics */
    IEEE80211_CONFIG_TRAFFIC_RATE,   /* Configuration parameter to Set the traffic rate,the rate at which the received signal statistics are be measured */
    IEEE80211_CONFIG_TRAFFIC_INTERVAL, /* Configuration parameter to set the traffic interval,the time till which the received signal statistics are to be measured */
#if MESH_MODE_SUPPORT
    IEEE80211_MESH_CAPABILITIES,              /* Mesh vap capabilities */
    IEEE80211_CONFIG_MGMT_TX_FOR_MESH,
    IEEE80211_CONFIG_RX_MESH_FILTER,
    IEEE80211_CONFIG_MESH_MCAST,
#endif
    IEEE80211_CONFIG_VDEV_PEER_PROTOCOL_COUNT,
    IEEE80211_CONFIG_VDEV_PEER_PROTOCOL_DROP_MASK,
    IEEE80211_CONFIG_ASSOC_DENIAL_NOTIFICATION, /*Enable/Disable configuration to send assoc denial notification of blacklisted MAC to user*/
    IEEE80211_CONFIG_WATERMARK_THRESHOLD,       /* get/set the watermark threshold value */
    IEEE80211_CONFIG_WATERMARK_REACHED,         /* get the number of times the watermark threshold reached value */
    IEEE80211_CONFIG_ASSOC_REACHED,             /* get the max number of devices associated crossing the threshold */
#if QCA_LTEU_SUPPORT
    IEEE80211_SCAN_PROBE_SPACE_INTERVAL,
#endif
    IEEE80211_CONFIG_DISABLE_SELECTIVE_LEGACY_RATE,           /* Enable/Disable selective Legacy Rates for vap */
    IEEE80211_CONFIG_MON_DECODER,               /* Monitor VAP RX decoder format */
    IEEE80211_DYN_BW_RTS,
    IEEE80211_CONFIG_NSTSCAP_WAR,
    IEEE80211_BEACON_RATE_FOR_VAP,            /* Modify the beacon rate */
    IEEE80211_CONFIG_CHANNEL_SWITCH_MODE,
    IEEE80211_CONFIG_ECSA_IE,
    IEEE80211_CONFIG_ECSA_OPCLASS,
    IEEE80211_CONFIG_VAP_TXPOW_MGMT,              /* Configure the tx power */
    IEEE80211_CONFIG_VAP_TXPOW,
    IEEE80211_CONFIG_TX_CAPTURE,              /* Configure TX capture capability for vap */
    IEEE80211_OCE,                            /* Enable OCE */
    IEEE80211_OCE_ASSOC_REJECT,               /* Enable OCE RSSI-based assoc reject */
    IEEE80211_OCE_ASSOC_MIN_RSSI,             /* Minimum RSSI for assoc accept */
    IEEE80211_OCE_ASSOC_RETRY_DELAY,          /* Retry delay for subsequent (re-)assoc */
    IEEE80211_OCE_WAN_METRICS,                /* Enable OCE reduced WAN metrics */
    IEEE80211_FEATURE_BACKHAUL,
    IEEE80211_FIXED_HE_MCS,                   /* HE MCS index */
    IEEE80211_CONFIG_HE_EXTENDED_RANGE,       /* HE Extended Range */
    IEEE80211_CONFIG_HE_DCM,                  /* HE DCM */
    IEEE80211_CONFIG_HE_BSS_COLOR,            /* HE BSS color */
    IEEE80211_CONFIG_HE_FRAGMENTATION,        /* HE Fragmentation */
    IEEE80211_CONFIG_HE_DL_MU_OFDMA,          /* HE DL MU OFDMA */
    IEEE80211_CONFIG_HE_UL_MU_OFDMA,          /* HE Ul MU OFDMA */
    IEEE80211_CONFIG_HE_UL_MU_MIMO,           /* HE Ul MU MIMO */
    IEEE80211_CONFIG_HE_SU_BFEE,              /* HE SU BFEE */
    IEEE80211_CONFIG_HE_SU_BFER,              /* HE SU BFER */
    IEEE80211_CONFIG_HE_MU_BFEE,              /* HE MU BFEE */
    IEEE80211_CONFIG_HE_MU_BFER,              /* HE MU BFER */
#if ATH_SUPPORT_DSCP_OVERRIDE
	IEEE80211_DP_DSCP_MAP,
#endif
    IEEE80211_TXRX_DP_STATS,
    IEEE80211_SON_CAP_RSSI,
    IEEE80211_CONFIG_EXT_NSS_SUPPORT,         /* Configure EXT NSS Support */
    IEEE80211_CONFIG_HE_LTF,                  /* HE LTF */
    IEEE80211_CONFIG_HE_RTSTHRSHLD,           /* HE RTS THRSHLD */
    IEEE80211_CONFIG_HE_OP,                   /* HE OP */
    IEEE80211_FEATURE_DISABLE_CABQ,           /* Disable multicast buffer when STA is PS */
    IEEE80211_CONFIG_CSL_SUPPORT,             /* Configure Connection state logging */
    IEEE80211_SUPPORT_TIMEOUTIE,              /* timeoutie assoc comeback value */
    IEEE80211_SUPPORT_PMF_ASSOC,              /* enable pmf timeoutie feature */
    IEEE80211_FEATURE_FILS,                   /* Enable FILS */
    IEEE80211_CONFIG_REFUSE_ALL_ADDBAS,       /* Refuse all incoming ADD BAs */
    IEEE80211_CONFIG_HE_TX_MCSMAP,            /* HE TX MCS Map */
    IEEE80211_CONFIG_HE_RX_MCSMAP,            /* HE RX MCS Map */
    IEEE80211_CONFIG_M_COPY,                  /* Configure mirror copy capability*/
    IEEE80211_CONFIG_ADDBA_MODE,              /* ADDBA Mode */
    IEEE80211_CONFIG_HE_AR_GI_LTF,            /* HE Auto Rate GI LTF combination */
    IEEE80211_CONFIG_BA_BUFFER_SIZE,          /* Configure Block buffer size */
    IEEE80211_CONFIG_READ_RXPREHDR,           /* configure rx preheader read capability for data received on vap */
    IEEE80211_CONFIG_HE_SOUNDING_MODE,        /* Select HE/VHT, SU/MU and Trig/Non-trig sounding */
    IEEE80211_UPDATE_DEV_STATS,
    IEEE80211_CONFIG_HE_MU_EDCA,              /* HE MU EDCA */
    IEEE80211_CONFIG_HE_DYNAMIC_MU_EDCA,      /* HE MU EDCA dynamic selection */
    IEEE80211_PRB_RATE,                       /* IEEE rate to be used for probe-response */
    IEEE80211_PRB_RETRY,                      /* IEEE retry count to be used for probe-response */
    IEEE80211_SUPPORT_RSN_OVERRIDE,           /* enable rsn_override feature */
    IEEE80211_CONFIG_HE_HT_CTRL,              /* HE HTC support */
#if QCN_IE
    IEEE80211_CONFIG_BCAST_PROBE_RESPONSE,    /* Enable bcast probe response */
#endif
    IEEE80211_OCE_HLP,                        /* Enable OCE FILS HLP */
    IEEE80211_CONFIG_WIFI_DOWN_IND,           /* Enable WIFI down indication */
    IEEE80211_CONFIG_RU26,                    /* RU 26 allocation configuration */
    IEEE80211_SIFS_TRIGGER_RATE,              /* configure sifs_trigger rate */
    IEEE80211_DISABLE_LPI_ANT_OPTIMIZATION,   /* Disable lpi_ant optimization */
    IEEE80211_CONFIG_SEC_STA_WDS,            /* Band steering events enabled */
    IEEE80211_CONFIG_FT_ENABLE,               /* Enable BSS Fast Transition */
    IEEE80211_CONFIG_HE_UL_SHORTGI,           /* UL Shortgi setting */
    IEEE80211_CONFIG_HE_UL_LTF,               /* UL LTF setting */
    IEEE80211_CONFIG_HE_UL_NSS,               /* UL NSS setting */
    IEEE80211_CONFIG_HE_UL_PPDU_BW,           /* UL channel bandwidth */
    IEEE80211_CONFIG_HE_UL_LDPC,              /* Enable/Disable UL LDPC */
    IEEE80211_CONFIG_HE_UL_STBC,              /* Enable/Disable UL STBC */
    IEEE80211_CONFIG_HE_UL_FIXED_RATE,        /* UL Fixed rate MCS setting */
    IEEE80211_CONFIG_RAWMODE_OPEN_WAR,        /* enable rawmode open mode vap war feature */
    IEEE80211_GET_OPMODE,                     /* get opmode of vap */
    IEEE80211_CONFIG_HE_BSR_SUPPORT,          /* HE BSR SUpport */
    IEEE80211_CONFIG_INDICATE_FT_ROAM,        /* indicate FT roaming */
    IEEE80211_CONFIG_CAPTURE_LATENCY_ENABLE,  /* capture latency */
    IEEE80211_CONFIG_VHT_MCS_10_11_SUPP,      /* Overall VHT MCS 10/11 Support */
    IEEE80211_CONFIG_VHT_MCS_10_11_NQ2Q_PEER_SUPP, /* Non- Q2Q Peer VHT MCS 10/11 Support */
    IEEE80211_CONFIG_SAE_PWID,                /* Enable SAE Password ID */
    IEEE80211_CONFIG_MCAST_RC_STALE_PERIOD,   /* Set Mcast RC Stale Time Period (in seconds) */
    IEEE80211_CONFIG_HE_MAX_AMPDU_LEN_EXP,    /* Set value of max AMPDU exponent extension */
    IEEE80211_CONFIG_HE_MULTI_TID_AGGR,       /* 'TIDs-1' QoS Data Frames STA can RX in multi tid AMPDU */
    IEEE80211_CONFIG_HE_MULTI_TID_AGGR_TX,    /* 'TIDs-1' QoS Data Frames STA can TX in multi tid AMPDU */
    IEEE80211_CONFIG_HE_SU_PPDU_1X_LTF_800NS_GI, /* Enable SU PPDU 1x HE LTF with 800ns GI */
    IEEE80211_CONFIG_HE_SU_MU_PPDU_4X_LTF_800NS_GI, /* Enable SU MU PPDU 4x HE LTF with 800ns GI */
    IEEE80211_CONFIG_HE_MAX_FRAG_MSDU,         /* Set max number of fragmented MSDUs STA can Rx concurrently */
    IEEE80211_CONFIG_HE_MIN_FRAG_SIZE,         /* Set min frag size for first octet in MSDU */
    IEEE80211_CONFIG_HE_OMI,                   /* Enable/Disable omi HE MAC CAP for STA */
    IEEE80211_CONFIG_HE_NDP_4X_LTF_3200NS_GI,  /* Enable/Disable NDP 4x HE-LTF with 3200ns GI*/
    IEEE80211_CONFIG_HE_ER_SU_PPDU_1X_LTF_800NS_GI, /* Enable/disable ER SU PPDU 1x HE-LTF with 800ns GI */
    IEEE80211_CONFIG_HE_ER_SU_PPDU_4X_LTF_800NS_GI, /* Enable/disable ER SU PPDU 4x HE-LTF with 800ns GI */
    IEEE80211_CONFIG_ENABLE_MULTI_GROUP_KEY,  /* Enable/disable multiple Group key for VLAN */
    IEEE80211_CONFIG_MAX_GROUP_KEYS,          /* Maximum number of Group key for VLAN */
    IEEE80211_RRM_FILTER,                     /* Enable filter to send RRM packet to user app */
    IEEE80211_WNM_FILTER,                     /* Enable filter to send WNM packet to user app */
    IEEE80211_CONFIG_MAX_MTU_SIZE,
    IEEE80211_CONFIG_VENDOR_FRAME_FWD_MASK,
    IEEE80211_CONFIG_HE_AR_LDPC,              /* Enable/Disable HE Auto-Rate LDPC */
    IEEE80211_CONFIG_ENABLE_MCAST_RC,         /* Enable Multicast RC */
    IEEE80211_CONFIG_6GHZ_BCAST_PROB_RSP,     /* Enable/Disable 20 TU Probe Resp */
    IEEE80211_CONFIG_HE_MCS_12_13_SUPP,       /* Overall HE MCS 12/13 Support */
    IEEE80211_CONFIG_CFR_RCC,                 /* Enable CFR RCC mode */
    IEEE80211_CONFIG_6GHZ_NON_HT_DUP,         /* Enable/Disable Non-HT duplicate setting*/
    IEEE80211_STATS_UPDATE_PERIOD,            /* Set stats update period in legacy devices */
    IEEE80211_CONFIG_HE_DL_MU_OFDMA_BFER,     /* HE DL MU OFDMA_BFER */
    IEEE80211_CONFIG_ASSOC_MIN_RSSI,          /* Set minimum required RSSI level in a assoc_req */
    IEEE80211_CONFIG_OCE_TX_POWER,            /* Enable tx power indication as OCE attribute in Beacon and Probe Response frames */
    IEEE80211_OCE_VERSION_OVERRIDE,           /* Override OCE release version */
#if ATH_PERF_PWR_OFFLOAD && QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    IEEE80211_RAWSIM_DEBUG_NUM_ENCAP_FRAMES,  /* Set the number of encapped frames to be dumped when debug is enabled */
    IEEE80211_RAWSIM_DEBUG_NUM_DECAP_FRAMES, /* Set the number of decapped frames to be dumped when debug is enabled */
#endif /* ATH_PERF_PWR_OFFLOAD && QCA_SUPPORT_RAWMODE_PKT_SIMULATION */
    IEEE80211_CONFIG_ENABLE_MSCS,             /* Enable MSCS Procedures*/
#if ATH_SUPPORT_IQUE
    IEEE80211_IGMP_ME,                            /* mcast enhancement mode */
#endif //ATH_SUPPORT_IQUE
    IEEE80211_CONFIG_6G_HE_OP_MIN_RATE,       /* Set 6G OP mininum rate */
    IEEE80211_CONFIG_6G_SECURITY_COMP,        /* 6G Security Compliance on/off */
    IEEE80211_CONFIG_6G_KEYMGMT_MASK,         /* 6G Key Mgmt Mask Config */
    IEEE80211_CONFIG_HE_ER_SU_DISABLE,
    IEEE80211_CONFIG_HE_1024QAM_LT242RU_RX_ENABLE,
    IEEE80211_CONFIG_HE_UL_MU_DATA_DIS_RX_SUPP,
    IEEE80211_CONFIG_HE_FULL_BW_UL_MUMIMO,
    IEEE80211_CONFIG_HE_DCM_MAX_CONSTELLATION_RX,
    IEEE80211_CONFIG_DISABLE_INACT_PROBING,   /* Disable FW inactivity probing behavior */
    IEEE80211_VDEV_TSF,
    IEEE80211_FWD_ACTION_FRAMES_TO_APP,
    IEEE80211_WNM_STATS,
    IEEE80211_CONFIG_VAP_TID_LATENCY_PARAM,
    IEEE80211_CONFIG_AP_MAX_AUTH_FAIL,
#ifdef WLAN_FEATURE_11BE
    IEEE80211_CONFIG_EHT_TX_MCSMAP, /* EHT TX MCS Map */
    IEEE80211_CONFIG_EHT_RX_MCSMAP, /* EHT RX MCS Map */
    IEEE80211_FIXED_EHT_MCS,        /* EHT MCS */
    IEEE80211_EHT_NSEP_PRIORITY_ACCESS,
    IEEE80211_EHT_OM_CONTROL,
    IEEE80211_EHT_TRIGGERED_TXOP_SHARING,
    IEEE80211_EHT_320MHZ_IN_6GHZ,
    IEEE80211_EHT_242TONE_RU_BW_GT_20MHZ,
    IEEE80211_EHT_NDP_4X_EHT_LTF_AND_320NSGI,
    IEEE80211_EHT_PARTIAL_BW_UL_MU,
    IEEE80211_EHT_SU_BFMR,
    IEEE80211_EHT_SU_BFME,
    IEEE80211_EHT_BFME_SS_LT_80MHZ,
    IEEE80211_EHT_BFME_SS_160MHZ,
    IEEE80211_EHT_BFME_SS_320MHZ,
    IEEE80211_EHT_NUM_SOUND_LT_80MHZ,
    IEEE80211_EHT_NUM_SOUND_160MHZ,
    IEEE80211_EHT_NUM_SOUND_320MHZ,
    IEEE80211_EHT_NG_16_SU_FB,
    IEEE80211_EHT_NG_16_MU_FB,
    IEEE80211_EHT_CODBK_42_SU_FB,
    IEEE80211_EHT_CODBK_75_MU_FB,
    IEEE80211_EHT_TRIG_SU_BF_FB,
    IEEE80211_EHT_TRIG_MU_BF_PART_BW_FB,
    IEEE80211_EHT_TRIG_CQI_FB,
    IEEE80211_EHT_PART_BW_DL_MU_MIMO,
    IEEE80211_EHT_PSR_SR,
    IEEE80211_EHT_PWR_BST_FACTOR,
    IEEE80211_EHT_4X_EHT_MU_LTF_AND_800NS_GI,
    IEEE80211_EHT_MAX_NC,
    IEEE80211_EHT_NON_TRIG_CQI_FB,
    IEEE80211_EHT_TX_1024_AND_4096_QAM_LT_242_TONE_RU,
    IEEE80211_EHT_RX_1024_AND_4096_QAM_LT_242_TONE_RU,
    IEEE80211_EHT_PPE_THRES_PRESENT,
    IEEE80211_EHT_CMN_NOM_PKT_PAD,
    IEEE80211_EHT_MAX_NUM_EHT_LTF,
    IEEE80211_EHT_SUP_MCS15,
    IEEE80211_EHT_EHT_DUP_IN_6GHZ,
    IEEE80211_EHT_20MHZ_OP_STA_RX_NDP_WIDER_BW,
    IEEE80211_EHT_NON_OFDMA_UL_MU_MIMO_LT_80MHZ,
    IEEE80211_EHT_NON_OFDMA_UL_MU_MIMO_160MHZ,
    IEEE80211_EHT_NON_OFDMA_UL_MU_MIMO_320MHZ,
    IEEE80211_EHT_MU_BFMR_LT_80MHZ,
    IEEE80211_EHT_MU_BFMR_160MHZ,
    IEEE80211_EHT_MU_BFMR_320MHZ,
    IEEE80211_CONFIG_EHT_MU_BFEE,
    IEEE80211_CONFIG_EHT_UL_OFDMA,
    IEEE80211_CONFIG_EHT_DL_OFDMA,
    IEEE80211_CONFIG_EHT_DL_OFDMA_TXBF,
    IEEE80211_CONFIG_EHT_UL_NSS,
    IEEE80211_CONFIG_EHT_UL_PPDU_BW,
    IEEE80211_CONFIG_EHT_UL_SHORTGI,
    IEEE80211_CONFIG_EHT_UL_LDPC,
    IEEE80211_CONFIG_EHT_UL_STBC,
    IEEE80211_CONFIG_EHT_UL_FIXED_RATE,
    IEEE80211_CONFIG_EHT_UL_LTF,
    IEEE80211_EHT_TB_SOUNDING_FB_RATE_LIMIT,
    IEEE80211_EHT_RX_1024QAM_IN_WB_DL_OFDMA_SUPPORT,
    IEEE80211_EHT_RX_4096QAM_IN_WB_DL_OFDMA_SUPPORT,
    IEEE80211_CONFIG_EHT_LTF,                  /* EHT LTF */
#endif /* WLAN_FEATURE_11BE */
    IEEE80211_CONFIG_ENABLE_SCS,              /* Enable SCS Procedures */
    IEEE80211_SUPPORT_VAP_DIFF_MODE, /* Support lower mode for vap */
    IEEE80211_CONFIG_DSCP_ACTION_POLICY,      /* Enable DAP protocol */
    IEEE80211_CONFIG_VAP_SSID_STEERING,
    IEEE80211_CONFIG_MCAST_STEER, /* Config MGMT Tx BF Support*/
    IEEE80211_CONFIG_STEALTHDOWN, /* flag to disble deauth/disassoc Tx when vap is brought down */
    IEEE80211_NEIGH_RPT, /* flag set/get neighbor report */
    IEEE80211_DRIVER_HW_CAPS_EXT, /* ext hw caps */
    IEEE80211_FEATURE_PURE11AX, /* Enable pure 11ax mode */
    IEEE80211_DISABLED_SCHED_MODES, /* Disable the schedular modes */
    IEEE80211_RTT_11AZ_TB_MAX_SESSION_EXPIRY, /* Config 11AZ TB Max Session Expiry*/
    IEEE80211_RTT_11AZ_NTB_MAX_TIME_BW_MEAS, /* Config 11AZ NTB Max Time Between Measurement */
    IEEE80211_RTT_11AZ_NTB_MIN_TIME_BW_MEAS, /* Config 11AZ NTB Min Time Between Measurement */
    IEEE80211_CONFIG_11AZ_SECURITY_CONFIG, /*11az related security params*/
#ifdef ASSOC_REJECT_SUPPORT_ENABLED
    IEEE80211_CONFIG_ASSOC_REJECT,
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    IEEE80211_CONFIG_MLO_MAX_RECOM_ACTIVE_LINKS, /* MLO Recommended Max simultaneous link config */
    IEEE80211_CONFIG_MLO_EXTMLDCAP_FLAG, /* MLO EXTMLDCAP Advertisement enable/disable flag */
#endif /* WLAN_FEATURE_11BE_MLO */
    IEEE80211_ME6_WAR,                            /* enable me6 war */
    IEEE80211_CONFIG_GTX_ENABLE, /* Enable/Disable green Tx */
    IEEE80211_SHORT_GI20,
    IEEE80211_SHORT_GI40,
    IEEE80211_4ADDR_EAPOL,
} ieee80211_param;

#ifdef WLAN_FEATURE_11BE
#define EHT_MU_BFMR_GET_LT_80MHZ_SUPPORT(val) QDF_GET_BITS(val, 0, 1);
#define EHT_MU_BFMR_GET_160MHZ_SUPPORT(val) QDF_GET_BITS(val, 1, 1);
#define EHT_MU_BFMR_GET_320MHZ_SUPPORT(val) QDF_GET_BITS(val, 2, 1);

#define EHT_MU_MIMO_GET_LT_80MHZ_SUPPORT(val) QDF_GET_BITS(val, 0, 1);
#define EHT_MU_MIMO_GET_160MHZ_SUPPORT(val) QDF_GET_BITS(val, 1, 1);
#define EHT_MU_MIMO_GET_320MHZ_SUPPORT(val) QDF_GET_BITS(val, 2, 1);
#endif

#define MIN_20TU_INTVAL    20 /* Minimum 20TU probe response interval */

/* Maximum 20tu probe response interval
 * 32ms is due to boundary timer limit in FW.
 * Target cannot guarantee anything beyond this value.
 */
#define MAX_BCAST_PRB_RSP_INTVAL 32

#define  IEEE80211_PROTECTION_NONE         0
#define  IEEE80211_PROTECTION_CTSTOSELF    1
#define  IEEE80211_PROTECTION_RTS_CTS      2

typedef enum _ieee80211_privacy_filter {
    IEEE80211_PRIVACY_FILTER_ALLWAYS,
    IEEE80211_PRIVACY_FILTER_KEY_UNAVAILABLE,
} ieee80211_privacy_filter ;

typedef enum _ieee80211_privacy_filter_packet_type {
    IEEE80211_PRIVACY_FILTER_PACKET_UNICAST,
    IEEE80211_PRIVACY_FILTER_PACKET_MULTICAST,
    IEEE80211_PRIVACY_FILTER_PACKET_BOTH
} ieee80211_privacy_filter_packet_type ;

typedef struct _ieee80211_privacy_excemption_filter {
    u_int16_t                               ether_type; /* type of ethernet to apply this filter, in host byte order*/
    ieee80211_privacy_filter                filter_type;
    ieee80211_privacy_filter_packet_type    packet_type;
} ieee80211_privacy_exemption;

/*
 * Authentication mode.
 * NB: the usage of auth modes NONE, AUTO are deprecated,
 * they are implemented through combinations of other auth modes
 * and cipher types. The deprecated values are preserved here to
 * maintain binary compatibility with applications like
 * wpa_supplicant and hostapd.
 */
typedef enum _ieee80211_auth_mode {
    IEEE80211_AUTH_NONE     = 0, /* deprecated */
    IEEE80211_AUTH_OPEN     = 1, /* open */
    IEEE80211_AUTH_SHARED   = 2, /* shared-key */
    IEEE80211_AUTH_8021X    = 3, /* 802.1x */
    IEEE80211_AUTH_AUTO     = 4, /* deprecated */
    IEEE80211_AUTH_WPA      = 5, /* WPA */
    IEEE80211_AUTH_RSNA     = 6, /* WPA2/RSNA */
    IEEE80211_AUTH_CCKM     = 7, /* CCK */
    IEEE80211_AUTH_WAPI     = 8, /* WAPI */
    IEEE80211_AUTH_SAE      = 9, /* SAE */
} ieee80211_auth_mode;

#define IEEE80211_AUTH_MAX      (IEEE80211_AUTH_WAPI+1)

/*
 * Cipher types.
 * NB: The values are preserved here to maintain binary compatibility
 * with applications like wpa_supplicant and hostapd.
 */
typedef enum _ieee80211_cipher_type {
    IEEE80211_CIPHER_WEP          = 0,
    IEEE80211_CIPHER_TKIP         = 1,
    IEEE80211_CIPHER_AES_OCB      = 2,
    IEEE80211_CIPHER_AES_CCM      = 3,
    IEEE80211_CIPHER_WAPI         = 4,
    IEEE80211_CIPHER_CKIP         = 5,
    IEEE80211_CIPHER_AES_CMAC     = 6,
    IEEE80211_CIPHER_AES_CCM_256  = 7,
    IEEE80211_CIPHER_AES_CMAC_256 = 8,
    IEEE80211_CIPHER_AES_GCM      = 9,
    IEEE80211_CIPHER_AES_GCM_256  = 10,
    IEEE80211_CIPHER_AES_GMAC     = 11,
    IEEE80211_CIPHER_AES_GMAC_256 = 12,
    IEEE80211_CIPHER_NONE         = 13,
} ieee80211_cipher_type;

#define IEEE80211_CIPHER_MAX    (IEEE80211_CIPHER_NONE+1)

/* key direction */
typedef enum _ieee80211_key_direction {
    IEEE80211_KEY_DIR_TX,
    IEEE80211_KEY_DIR_RX,
    IEEE80211_KEY_DIR_BOTH
} ieee80211_key_direction;

#define IEEE80211_KEYIX_NONE    ((u_int16_t) -1)
#define IEEE80211_WAPI_IV_SIZE  (16)

#define IEEE80211_MAX_WPA_KEK_LEN 64
#define IEEE80211_FILS_NONCE_LEN  16
struct ieee80211req_fils_aad {
    u_int8_t    ANonce[IEEE80211_FILS_NONCE_LEN];
    u_int8_t    SNonce[IEEE80211_FILS_NONCE_LEN];
    u_int8_t    kek[IEEE80211_MAX_WPA_KEK_LEN];
    u_int32_t   kek_len;
} __packed;

typedef struct _ieee80211_keyval {
    ieee80211_cipher_type   keytype;
    ieee80211_key_direction keydir;
    u_int                   persistent:1,   /* persistent key */
                            mfp:1;          /* management frame protection */
    u_int16_t               keylen;         /* length of the key data fields */
    u_int8_t                *macaddr;       /* mac address of length IEEE80211_ADDR_LEN . all bytes are 0xff for multicast key */
    u_int64_t               keyrsc;
    u_int64_t               keytsc;
    u_int16_t               txmic_offset;   /* TKIP/SMS4 only: offset to tx mic key */
    u_int16_t               rxmic_offset;   /* TKIP/SMS4 only: offset to rx mic key */
    u_int8_t                *keydata;
#if ATH_SUPPORT_WAPI
    u_int8_t                key_used;       /*index for WAPI rekey labeling*/
    u_int8_t                txiv[IEEE80211_WAPI_IV_SIZE];      /* WAPI tx iv */
    u_int8_t                recviv[IEEE80211_WAPI_IV_SIZE];    /* WAPI rx iv */
#endif
    u_int16_t                keyindex;       /*key index */
#if UMAC_SUPPORT_FILS
    struct ieee80211req_fils_aad  kval_filsaad;
#endif
     u_int8_t                 flags;         /*key flags*/
} ieee80211_keyval;

#define IEEE80211_AES_CMAC_LEN     128
typedef enum _ieee80211_rsn_param {
    IEEE80211_UCAST_CIPHER_LEN,
    IEEE80211_MCAST_CIPHER_LEN,
    IEEE80211_MCASTMGMT_CIPHER_LEN,
    IEEE80211_KEYMGT_ALGS,
    IEEE80211_RSN_CAPS
} ieee80211_rsn_param;

#define IEEE80211_PMKID_LEN     16

typedef struct _ieee80211_pmkid_entry {
    u_int8_t    bssid[IEEE80211_ADDR_LEN];
    u_int8_t    pmkid[IEEE80211_PMKID_LEN];
} ieee80211_pmkid_entry;

typedef enum _wlan_wme_param {
    WLAN_WME_CWMIN,
    WLAN_WME_CWMAX,
    WLAN_WME_AIFS,
    WLAN_WME_TXOPLIMIT,
    WLAN_WME_ACM,      /*bss only*/
    WLAN_WME_ACKPOLICY /*bss only*/
} wlan_wme_param;

typedef enum _wlan_muedca_param {
    WLAN_MUEDCA_ECWMIN,
    WLAN_MUEDCA_ECWMAX,
    WLAN_MUEDCA_AIFSN,
    WLAN_MUEDCA_ACM,
    WLAN_MUEDCA_TIMER
} wlan_muedca_param;

typedef enum _ieee80211_frame_type {
    IEEE80211_FRAME_TYPE_PROBEREQ,
    IEEE80211_FRAME_TYPE_BEACON,
    IEEE80211_FRAME_TYPE_PROBERESP,
    IEEE80211_FRAME_TYPE_ASSOCREQ,
    IEEE80211_FRAME_TYPE_ASSOCRESP,
    IEEE80211_FRAME_TYPE_AUTH
} ieee80211_frame_type;

#define IEEE80211_FRAME_TYPE_MAX    (IEEE80211_FRAME_TYPE_AUTH+1)

typedef enum _ieee80211_ampdu_mode {
    IEEE80211_AMPDU_MODE_OFF,   /* disable AMPDU */
    IEEE80211_AMPDU_MODE_ON,    /* enable AMPDU */
    IEEE80211_AMPDU_MODE_WDSVAR /* enable AMPDU with 4addr WAR */
} ieee80211_ampdu_mode;

typedef enum _ieee80211_reset_type {
    IEEE80211_RESET_TYPE_DEVICE = 0,    /* device reset on error: tx timeout and etc. */
    IEEE80211_RESET_TYPE_DOT11_INTF,    /* dot11 reset: only reset one network interface (vap) */
    IEEE80211_RESET_TYPE_INTERNAL,      /* internal reset */
} ieee80211_reset_type;

typedef struct _ieee80211_reset_request {
    ieee80211_reset_type    type;

    u_int                   reset_hw:1,         /* reset the actual H/W */
                            /*
                             * The following fields are only valid for DOT11 reset, i.e.,
                             * IEEE80211_RESET_TYPE_DOT11_INTF
                             */
                            reset_phy:1,        /* reset PHY */
                            reset_mac:1,        /* reset MAC */
                            set_default_mib:1,  /* set default MIB variables */
                            no_flush:1;
    u_int8_t                macaddr[IEEE80211_ADDR_LEN];
    enum ieee80211_phymode  phy_mode;
} ieee80211_reset_request;

#define IEEE80211_MSG_MAX 63
#define IEEE80211_MSG_SMARTANT 7            /* Bit 7 (0x80)for Smart Antenna debug */
enum {
    /* IEEE80211_PARAM_DBG_LVL */
    /* DBG LVL 0 (zero) is free */
    IEEE80211_MSG_ACS = 1,                  /* auto channel selection */
    IEEE80211_MSG_SCAN_SM,                  /* scan state machine */
    IEEE80211_MSG_SCANENTRY,                /* scan entry */
    IEEE80211_MSG_WDS,                      /* WDS handling */
    IEEE80211_MSG_ACTION,                   /* action management frames */
    IEEE80211_MSG_ROAM,                     /* sta-mode roaming */
    IEEE80211_MSG_INACT,                    /* inactivity handling */
    IEEE80211_MSG_DOTH      = 8,            /* 11.h */
    IEEE80211_MSG_IQUE,                     /* IQUE features */
    IEEE80211_MSG_WME,                      /* WME protocol */
    IEEE80211_MSG_ACL,                      /* ACL handling */
    IEEE80211_MSG_WPA,                      /* WPA/RSN protocol */
    IEEE80211_MSG_RADKEYS,                  /* dump 802.1x keys */
    IEEE80211_MSG_RADDUMP,                  /* dump 802.1x radius packets */
    IEEE80211_MSG_RADIUS,                   /* 802.1x radius client */
    IEEE80211_MSG_DOT1XSM   = 16,           /* 802.1x state machine */
    IEEE80211_MSG_DOT1X,                    /* 802.1x authenticator */
    IEEE80211_MSG_POWER,                    /* power save handling */
    IEEE80211_MSG_STATE,                    /* state machine */
    IEEE80211_MSG_OUTPUT,                   /* output handling */
    IEEE80211_MSG_SCAN,                     /* scanning */
    IEEE80211_MSG_AUTH,                     /* authentication handling */
    IEEE80211_MSG_ASSOC,                    /* association handling */
    IEEE80211_MSG_NODE      = 24,           /* node handling */
    IEEE80211_MSG_ELEMID,                   /* element id parsing */
    IEEE80211_MSG_XRATE,                    /* rate set handling */
    IEEE80211_MSG_INPUT,                    /* input handling */
    IEEE80211_MSG_CRYPTO,                   /* crypto work */
    IEEE80211_MSG_DUMPPKTS,                 /* IFF_LINK2 equivalant */
    IEEE80211_MSG_DEBUG,                    /* IFF_DEBUG equivalent */
    IEEE80211_MSG_MLME,                     /* MLME */
    /* IEEE80211_PARAM_DBG_LVL_HIGH */
    IEEE80211_MSG_RRM       = 32,           /* Radio resource measurement */
    IEEE80211_MSG_WNM,                      /* Wireless Network Management */
    IEEE80211_MSG_P2P_PROT,                 /* P2P Protocol driver */
    IEEE80211_MSG_PROXYARP,                 /* 11v Proxy ARP */
    IEEE80211_MSG_L2TIF,                    /* Hotspot 2.0 L2 TIF */
    IEEE80211_MSG_WIFIPOS,                  /* WifiPositioning Feature */
    IEEE80211_MSG_WRAP,                     /* WRAP or Wireless ProxySTA */
    IEEE80211_MSG_DFS,                      /* DFS debug mesg */
    IEEE80211_MSG_ATF       = 40,           /* ATF debug mesg */
    IEEE80211_MSG_SPLITMAC,                 /* splitmac debug */
    IEEE80211_MSG_IOCTL,                    /* ioctl */
    IEEE80211_MSG_NAC,                      /* nac debug */
    IEEE80211_MSG_MESH,                     /* mesh debug */
    IEEE80211_MSG_MBO,                      /* MBO debug mesg */
    IEEE80211_MSG_EXTIOCTL_CHANSWITCH  = 46,/* extended IOCTL chan switch debug */
    IEEE80211_MSG_EXTIOCTL_CHANSSCAN   = 47,/* extended IOCTL chan scan debug */
    IEEE80211_MSG_HE                   = 48,/* 11AX HE Debug */
    IEEE80211_MSG_CFG80211             = 49,/* cfg80211 debug */
    IEEE80211_MSG_PEER_DELETE          = 50,/* peer create/delete debug */
    IEEE80211_MSG_FILS,                     /* FILS debug */
    IEEE80211_MSG_CBS,                      /* CBS debug */
    IEEE80211_MSG_HE_6GHZ              = 53,/* 11AX 6GHz debug */
#ifdef WLAN_FEATURE_11BE
    IEEE80211_MSG_EHT                  = 54,/* 11be debug */
#endif /* WLAN_FEATURE_11BE */

    IEEE80211_MSG_OFFCHAN,                  /* off channel tx/rx debug - 55 */
    IEEE80211_MSG_CU,                       /* critical update debug - 56 */
#if QCA_MANUAL_TRIGGERED_ULOFDMA
    IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,      /* manual ulofdma trigger - 57 */
#endif
    IEEE80211_MSG_HW_STATUS,                /* HW status to userspace - 58 */
    IEEE80211_MSG_TPE_IE,                   /* TPE IE related debug - 59 */

    IEEE80211_MSG_CUST_BEGIN           = 60,/* Customer specifc enum begin  */
    IEEE80211_MSG_CUST_END             = 62,/* Customer specifc enum end  */

    IEEE80211_MSG_UNMASKABLE = IEEE80211_MSG_MAX,  /* anything */
    IEEE80211_MSG_ANY = IEEE80211_MSG_MAX,  /* anything */
};

static inline const char * msg_type_to_str (int msg_type) {

    switch (msg_type) {
        case IEEE80211_MSG_ACS:
            return "ACS";
        case IEEE80211_MSG_SCAN_SM:
            return "SCAN_SM";
        case IEEE80211_MSG_SCANENTRY:
            return "SCANENTRY";
        case IEEE80211_MSG_WDS:
            return "WDS";
        case IEEE80211_MSG_ACTION:
            return "ACTION";
        case IEEE80211_MSG_ROAM:
            return "ROAM";
        case IEEE80211_MSG_INACT:
            return "INACT";
        case IEEE80211_MSG_DOTH:
            return "DOTH";
        case IEEE80211_MSG_IQUE:
            return "IQUE";
        case IEEE80211_MSG_WME:
            return "WME";
        case IEEE80211_MSG_ACL:
            return "ACL";
        case IEEE80211_MSG_WPA:
            return "WPA";
        case IEEE80211_MSG_RADKEYS:
            return "RADKEYS";
        case IEEE80211_MSG_RADDUMP:
            return "RADDUMP";
        case IEEE80211_MSG_RADIUS:
            return "RADIUS";
        case IEEE80211_MSG_DOT1XSM:
            return "DOT1XSM";
        case IEEE80211_MSG_DOT1X:
            return "DOT1X";
        case IEEE80211_MSG_POWER:
            return "POWER";
        case IEEE80211_MSG_STATE:
            return "SM";
        case IEEE80211_MSG_OUTPUT:
            return "OUTPUT";
        case IEEE80211_MSG_SCAN:
            return "SCAN";
        case IEEE80211_MSG_AUTH:
            return "AUTH";
        case IEEE80211_MSG_ASSOC:
            return "ASSOC";
        case IEEE80211_MSG_NODE:
            return "NODE";
        case IEEE80211_MSG_ELEMID:
            return "ELEMID";
        case IEEE80211_MSG_XRATE:
            return "XRATE";
        case IEEE80211_MSG_INPUT:
            return "INPUT";
        case IEEE80211_MSG_CRYPTO:
            return "CRYPTO";
        case IEEE80211_MSG_DUMPPKTS:
            return "DUMPPKTS";
        case IEEE80211_MSG_DEBUG:
            return "DEBUG";
        case IEEE80211_MSG_MLME:
            return "MLME";
        case IEEE80211_MSG_RRM:
            return "RRM";
        case IEEE80211_MSG_WNM:
            return "WNM";
        case IEEE80211_MSG_P2P_PROT:
            return "P2P_PROT";
        case IEEE80211_MSG_PROXYARP:
            return "PROXYARP";
        case IEEE80211_MSG_L2TIF:
            return "L2TIF";
        case IEEE80211_MSG_WIFIPOS:
            return "WIFIPOS";
        case IEEE80211_MSG_WRAP:
            return "WRAP";
        case IEEE80211_MSG_DFS:
            return "DFS";
        case IEEE80211_MSG_ATF:
            return "ATF";
        case IEEE80211_MSG_SPLITMAC:
            return "SPLITMAC";
        case IEEE80211_MSG_IOCTL:
            return "IOCTL";
        case IEEE80211_MSG_NAC:
            return "NAC";
        case IEEE80211_MSG_MESH:
            return "MESH";
        case IEEE80211_MSG_MBO:
            return "MBO";
        case IEEE80211_MSG_EXTIOCTL_CHANSWITCH:
            return "CHANSWITCH";
        case IEEE80211_MSG_EXTIOCTL_CHANSSCAN:
            return "CHANSCAN";
        case IEEE80211_MSG_CBS:
            return "CBS";
        default:
            return "UNSPECIFIED";
    }
}
#define IEEE80211_MSG_CSL (1ULL << IEEE80211_MSG_ASSOC) |     \
                          (1ULL << IEEE80211_MSG_AUTH) |      \
                          (1ULL << IEEE80211_MSG_MLME) |      \
                          (1ULL << IEEE80211_MSG_INACT) |     \
                          (1ULL << IEEE80211_MSG_CRYPTO)

#define LOG_CSL_BASIC                            0x1
#define LOG_CSL_MLME_EVENTS                      0x2
#define LOG_CSL_MISC_EVENTS                      0x8

/* verbosity levels */
#define     IEEE80211_VERBOSE_OFF                  100
#define     IEEE80211_VERBOSE_FORCE               1
#define     IEEE80211_VERBOSE_SERIOUS             2
#define     IEEE80211_VERBOSE_NORMAL              3
#define     IEEE80211_VERBOSE_LOUD                4
#define     IEEE80211_VERBOSE_DETAILED            5
#define     IEEE80211_VERBOSE_COMPLEX             6
#define     IEEE80211_VERBOSE_FUNCTION            7
#define     IEEE80211_VERBOSE_TRACE               8

#define IEEE80211_DEBUG_DEFAULT IEEE80211_MSG_DEBUG

/*
 * the lower 4 bits of the msg flags are used for extending the
 * debug flags.
 */

/*
 * flag defintions for wlan_mlme_stop_bss(vap) API.
 */
#define WLAN_MLME_STOP_BSS_F_SEND_DEAUTH                0x01
#define WLAN_MLME_STOP_BSS_F_CLEAR_ASSOC_STATE          0x02
#define WLAN_MLME_STOP_BSS_F_FORCE_STOP_RESET           0x04
#define WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE               0x08
#define WLAN_MLME_STOP_BSS_F_NO_RESET                   0x10
#define WLAN_MLME_STOP_BSS_F_STANDBY                    0x20

/*
 * WAPI commands to authenticator
 */
#define WAPI_WAI_REQUEST            (u_int16_t)0x00F1
#define WAPI_UNICAST_REKEY          (u_int16_t)0x00F2
#define WAPI_STA_AGING              (u_int16_t)0x00F3
#define WAPI_MULTI_REKEY            (u_int16_t)0x00F4
#define WAPI_STA_STATS              (u_int16_t)0x00F5

/* Max supported ema ap groups */
#define EMA_AP_MAX_GROUPS 8
#define EMA_MAX_NGROUPS (EMA_AP_MAX_GROUPS + IEEE80211_EMA_MAX_N_SEC_GROUPS)

/*
 * IEEE80211 PHY Statistics.
 */
struct ieee80211_phy_stats {
    u_int64_t   ips_tx_packets;      /* frames successfully transmitted */
    u_int64_t   ips_tx_multicast;    /* multicast/broadcast frames successfully transmitted */
    u_int64_t   ips_tx_fragments;    /* fragments successfully transmitted */
    u_int64_t   ips_tx_xretries;     /* frames that are xretried. NB: not number of retries */
    u_int64_t   ips_tx_retries;      /* frames transmitted after retries. NB: not number of retries */
    u_int64_t   ips_tx_multiretries; /* frames transmitted after more than one retry. */
    u_int64_t   ips_tx_timeout;      /* frames that expire the dot11MaxTransmitMSDULifetime */
    u_int64_t   ips_rx_packets;      /* frames successfully received */
    u_int64_t   ips_rx_multicast;    /* multicast/broadcast frames successfully received */
    u_int64_t   ips_rx_fragments;    /* fragments successfully received */
    u_int64_t   ips_rx_timeout;      /* frmaes that expired the dot11MaxReceiveLifetime */
    u_int64_t   ips_rx_dup;          /* duplicated fragments */
    u_int64_t   ips_rx_mdup;         /* multiple duplicated fragments */
    u_int64_t   ips_rx_promiscuous;  /* frames that are received only because promiscuous filter is on */
    u_int64_t   ips_rx_promiscuous_fragments; /* fragments that are received only because promiscuous filter is on */
    u_int64_t   ips_tx_rts;          /* RTS success count */
    u_int64_t   ips_tx_shortretry;   /* tx on-chip retries (short). RTSFailCnt */
    u_int64_t   ips_tx_longretry;    /* tx on-chip retries (long). DataFailCnt */
    u_int64_t   ips_rx_crcerr;       /* rx failed 'cuz of bad CRC */
    u_int64_t   ips_rx_fifoerr;      /* rx failed 'cuz of FIFO overrun */
    u_int64_t   ips_rx_decrypterr;   /* rx decryption error */
};

struct ieee80211_chan_stats {
    u_int32_t   chan_clr_cnt;            /* Rx clear count */
    u_int32_t   cycle_cnt;               /* Cycle count */
    u_int32_t   phy_err_cnt;             /* Phy err count */
    u_int32_t   duration_11b_data;       /* 80211b data duration in the channel */
    u_int32_t   chan_tx_power_tput;      /* EEPROM Tx power value for Best throughput */
    u_int32_t   chan_tx_power_range;     /* EEPROM Tx power value for Maximum range*/
    u_int32_t   perchain_nf[HOST_MAX_CHAINS]; /* Per chain NF value in dBm */
};

typedef enum
{
    APSTATS_LEVEL_NO_VAL,
    APSTATS_LEVEL_AP,
    APSTATS_LEVEL_RADIO,
    APSTATS_LEVEL_VAP,
    APSTATS_LEVEL_NODE,
    APSTATS_LEVEL_NUM_ENTRIES,
} apstats_level_t;

typedef struct
{
    apstats_level_t level;
    void *firstchild;
    void *parent;                   /**< Pointer back to vaplevel_stats_t for
                                          VAP under which the node falls. */

    void *next; /**< Next in peer Linked List. */
    void *prev; /**< Previous in peer Linked List. */
    /*
     * Subtree root indicates the clipped portion of the apstats
     * tree that is to be shown in the apstats_display function
     * Gathering the stats is always going to be on the whole
     * topology. While displaying, based on the level requested,
     * a node will be marked as subtree root, so that nodes outside of the
     * subtree_root are not displayed (i.e., not returned via apstats_get_next
     */
    uint8_t subtree_root;
    uint8_t link_id; /* link_id for MLO objects */
} apstats_obj_t;

typedef struct _vaplevel_stats_t
{
    /* Statistics. */

    apstats_obj_t obj;
    u_int64_t tx_data_packets;       /**< No. of data frames transmitted. */
    u_int64_t tx_data_bytes;         /**< No. of data bytes transmitted. */
    u_int64_t tx_datapyld_bytes;     /**< No. of data frame payload bytes
                                          transmitted. */
    u_int64_t tx_data_wme[WME_NUM_AC]; /** No. of data frames transmitted
                                           per AC */
    u_int64_t rx_data_wme[WME_NUM_AC]; /** No. of data frames received
                                           per AC */
    u_int64_t rx_data_packets;       /**< No. of data frames received. */
    u_int64_t rx_data_bytes;         /**< No. of data bytes received. */
    u_int64_t rx_datapyld_bytes;     /**< No. of data frame payload bytes
                                          received. */

    u_int64_t tx_ucast_data_packets; /**< No. of unicast data frames
                                          transmitted. */
    u_int64_t rx_ucast_data_packets;  /**< No. of unicast data frames
                                           received. */
    u_int64_t tx_mbcast_data_packets;/**< No. of multicast/broadcast data frames
                                          transmitted. */
    u_int64_t rx_mbcast_data_packets;/**< No. of multicast/broadcast data frames
                                       transmitted. */

    u_int64_t tx_bcast_data_packets;
    u_int64_t tx_bcast_data_bytes;
    u_int64_t rx_bcast_data_packets;
    u_int32_t tx_rate;               /**< Average link rate of transmissions. */
    u_int32_t rx_rate;               /**< Average link rate at which frames
                                          were received. */

    u_int8_t  chan_util_enab;        /**< Whether Channel Utilization measurement
                                          is enabled. */
    u_int8_t  txrx_rate_available;   /**< Whether Average Tx/Rx rate information
                                          is available for this VAP (it is
                                          gathered from per-node stats, so
                                          at least one STA should be associated
                                          with the VAP for this to be
                                          available). */
    u_int16_t chan_util;             /**< Channel Utilization (0-255). */
    char ifname[IFACE_MAX_LEN];           /**< Interface name for VAP. */
    uint8_t  macaddr[IEEE80211_ADDR_LEN];       /**< MAC address of VAP. */

    u_int64_t rx_micerr;             /**< No. of MIC errors in frames received. */
    u_int64_t rx_decrypterr;         /**< No. of decryption errors in frames
                                          received. */
    u_int64_t rx_pnerr;              /**< No. of PN errors in frames received. */
    u_int64_t rx_err;                /**< No. of receive errors. */
    u_int64_t tx_err;                /**< No. of transmitted errors. */
    u_int64_t tx_discard;            /**< No. of discarded packets sent. */
    u_int64_t host_discard;

    u_int64_t rx_discard;            /**< No. of frames whose transmission
                                          failed. (Note: Not number of failed
                                          attempts). */
    u_int64_t u_last_tx_rate;
    u_int64_t m_last_tx_rate;
    u_int64_t u_last_tx_rate_mcs;
    u_int64_t m_last_tx_rate_mcs;
    u_int64_t tx_offer_packets;
    u_int64_t tx_offer_packets_bytes;
    u_int64_t mgmt_tx_fail;          /**< mgmt Tx failure count */

    u_int64_t last_per;              /**< Total cumulative PER of all nodes in this vap */
    /* Miscellaneous info required for application logic. */

    u_int32_t   total_num_offchan_tx_mgmt; /* total number of offchan TX mgmt frames */
    u_int32_t   total_num_offchan_tx_data; /* total number of offchan TX data frames */
    u_int64_t tx_beacon_swba_cnt;    /*Beacon intr SWBA counter*/
    u_int64_t retries;               /**< retries */
    u_int64_t tx_mgmt;               /**< No. of mgmt frames transmitted */
    u_int64_t rx_mgmt;               /**< No. of mgmt frames received */
    uint64_t excretries[WME_NUM_AC];  /**< excessive retries */
    u_int32_t   tx_bcn_succ_cnt;
    u_int32_t   tx_bcn_outage_cnt;
    u_int64_t sta_xceed_rlim;         /* no of connections per vap refused after radio limit */
    u_int64_t sta_xceed_vlim;         /* no of connections per vap refused after vap limit */
    u_int64_t mlme_auth_attempt;      /* no of 802.11 MLME Auth Attempt per vap */
    u_int64_t mlme_auth_success;      /* no of 802.11 MLME Auth success per vap */
    u_int64_t authorize_attempt;      /* no of Authorization attempt per vap */
    u_int64_t authorize_success;      /* no of Authorization success per vap */
    u_int64_t tx_eapol_packets;       /* no of EAPol data frames transmitted */
    u_int64_t peer_delete_req;        /* no of peer delete requests */
    u_int64_t peer_delete_resp;       /* no of peer delete response */
    u_int64_t peer_delete_all_req;    /* no of peer delete all requests */
    u_int64_t peer_delete_all_resp;   /* no of peer delete all response */
    u_int64_t prob_req_drops;         /* no of prob request drops */
    u_int64_t oob_probe_req_count;    /* no of out of band probe requests*/
    u_int64_t wc_probe_req_drops;     /* no of wildcard probe requests drops*/
    u_int32_t num_offchan_tx_failed;  /* number of offchan TX frames failed */
    u_int8_t  group_id;               /* ema group id */
    u_int32_t current_pp;             /* current Profile Periodicity */
    u_int32_t no_act_vaps;            /* no of active vaps */
    u_int32_t tx_vap;                 /* Transmit vap */
    u_int64_t fils_enable;            /* fils frames enable */
    u_int64_t fils_frames_sent;       /* fils frames sent successfully*/
    u_int64_t fils_frames_sent_fail;  /* fils frames sent fail*/
    u_int64_t tx_offload_prb_resp_succ_cnt; /* offload probe response success count */
    u_int64_t tx_offload_prb_resp_fail_cnt; /* offload probe response fail count */
    u_int64_t tx_20TU_prb_resp;       /* 20TU probe response */
    u_int64_t tx_20TU_prb_interval;   /* 20TU probe response interval */
    u_int64_t ntx_pfl_rollback_stats;   /* Non-Tx profile rollback count (ema_ext) */
    u_int64_t ie_overflow_stats;     /* Overflow stats for Tx/NonTx VAPs */
    u_int64_t total_num_offchan_tx_dpp_queued;
    u_int64_t total_num_offchan_tx_dpp_completion;
#ifdef QCA_SUPPORT_EAPOL_OVER_CONTROL_PORT
    /* TX/RX EAPOL STATS */
    u_int16_t m1_packet_cnt;
    u_int16_t m2_packet_cnt;
    u_int16_t m3_packet_cnt;
    u_int16_t m4_packet_cnt;
    u_int16_t g1_packet_cnt;
    u_int16_t g2_packet_cnt;
#endif
} vaplevel_stats_t;

/**
 * Radio level stats.
 */
typedef struct _radiolevel_stats_t
{
    /* Statistics. */
    apstats_obj_t obj;

    u_int64_t tx_data_packets;       /**< No. of data frames transmitted. */
    u_int64_t tx_data_bytes;         /**< No. of data bytes transmitted. */
    u_int64_t rx_data_packets;       /**< No. of data frames received. */
    u_int64_t rx_data_bytes;         /**< No. of data bytes received. */

    u_int64_t tx_ucast_data_packets; /**< No. of unicast data frames
                                          transmitted. */
    u_int64_t tx_mbcast_data_packets;/**< No. of multicast/broadcast data frames
                                          transmitted. */
    u_int64_t tx_data_wme[WME_NUM_AC]; /**< No. of data frames transmitted per AC */
    u_int64_t rx_data_wme[WME_NUM_AC]; /**< No. of data frames received per AC */
    u_int64_t tx_beacon_frames;      /**< No. of beacon frames transmitted. */
    u_int64_t tx_mgmt_frames;        /**< No. of management frames transmitted
                                          (including beacon frames). */
    u_int64_t rx_mgmt_frames;        /**< No. of management frames received. */

    u_int64_t rx_mgmt_frames_rssi_drop;  /**< No. of management frames dropped because of low RSSI */

    u_int32_t tx_rate;               /**< Average link rate of transmissions. */
    u_int32_t rx_rate;               /**< Average link rate at which frames
                                          were received. */

    u_int8_t  txrx_rate_available;   /**< Whether Average Tx/Rx rate information
                                          is available for this radio (depends
                                          on whether the info is available
                                          for at least one of the VAPs). */
    u_int8_t  rx_rssi;               /**< Rx RSSI of last frame received. */
    u_int8_t  thrput_enab;           /**< Whether in-driver throughput measurement
                                          is enabled. */

    u_int8_t  chan_util_enab;        /**< Whether Channel Utilization measurement
                                          is enabled (depends on whether it is
                                          enabled for at least one of the
                                          VAPs). */
    u_int16_t chan_util;             /**< Channel Utilization (0-255). This is the
                                          average of per-VAP values, considering
                                          those VAPs with the measurement
                                          enabled. */
    u_int8_t  total_per;             /**< PER measured from start of operation. */
    u_int8_t  prdic_per_enab;        /**< Whether periodic PER measurement is
                                          enabled. */

    u_int64_t rx_phyerr;             /**< No. of PHY errors in frames received. */
    u_int64_t rx_crcerr;             /**< No. of CRC errors in frames received. */
    u_int64_t rx_micerr;             /**< No. of MIC errors in frames received. */
    u_int64_t rx_decrypterr;         /**< No. of decryption errors in frames
                                          received. */
    u_int64_t rx_pnerr;              /**< No. of PN errors in frames received. */
    u_int64_t rx_err;                /**< No. of receive errors. */

    u_int64_t tx_discard;            /**< No. of frames whose transmission
                                          failed. (Note: Not number of failed
                                          attempts). */
    u_int64_t tx_err;                /**< No. of tx error  */
    u_int64_t rx_ctl_frames;            /**< Rxed control frames */
    u_int64_t tx_ctl_frames;            /**< Txed control frames */

    u_int64_t sta_xceed_rlim;         /* no of connections per radio refused after radio limit */
    u_int64_t sta_xceed_vlim;         /* no of connections per radio refused after vap limit */
    u_int64_t mlme_auth_attempt;      /* no of 802.11 MLME Auth Attempt per radio  */
    u_int64_t mlme_auth_success;      /* no of 802.11 MLME Auth success per radio  */
    u_int64_t authorize_attempt;      /* no of Authorization attempt per radio  */
    u_int64_t authorize_success;      /* no of Authorization success per radio  */
    u_int32_t thrput;                /**< Throughput in kbps */

    u_int8_t  prdic_per;             /**< Periodic PER measured in configured
                                          time window. */

    u_int8_t  self_bss_util;            /* self bss channel utilization per radio */
    u_int8_t  obss_util;               /* other bss and non wifi channel utilization per radio */
    /*
    Required to determine which wlan interface.
    */
    u_int8_t ol_enable;

    /* Miscellaneous info required for application logic. */

    char ifname[IFACE_MAX_LEN];           /**< Interface name for radio. */
    uint8_t macaddr[IEEE80211_ADDR_LEN];       /**< MAC address for radio. */

    int32_t chan_nf;
    u_int32_t tx_frame_count;
    u_int32_t rx_frame_count;
    u_int32_t rx_clear_count;
    u_int32_t cycle_count;
    u_int32_t phy_err_count;
    u_int32_t chan_tx_pwr;
    u_int8_t  is_mbssid_enabled;
    u_int8_t  n_groups;
    u_int32_t current_pp[EMA_MAX_NGROUPS];
    u_int32_t no_act_vaps[EMA_MAX_NGROUPS];
    u_int32_t tx_vap[EMA_MAX_NGROUPS];

    /* co-located rnr stats */
    u_int32_t created_vap;          /* no of created vap in 6ghz */
    u_int32_t active_vap;           /* no of active vap in 6ghz */
    u_int32_t rnr_count;            /* rnr count */
    u_int32_t soc_status_6ghz;      /* 6ghz soc status */
} radiolevel_stats_t;

struct ieee80211_mac_stats {
    u_int64_t   ims_rx_badkeyid;    /* rx w/ incorrect keyid */
    u_int64_t   ims_rx_decryptok;   /* rx decrypt okay */
    u_int64_t   ims_rx_pnerr;       /* rx PN failed */
    u_int64_t   ims_rx_wepfail;     /* rx wep processing failed */
    u_int64_t   ims_rx_tkipreplay;  /* rx seq# violation (TKIP) */
    u_int64_t   ims_rx_tkipformat;  /* rx format bad (TKIP) */
    u_int64_t   ims_rx_tkipicv;     /* rx ICV check failed (TKIP) */
    u_int64_t   ims_rx_ccmpreplay;  /* rx seq# violation (CCMP) */
    u_int64_t   ims_rx_ccmpformat;  /* rx format bad (CCMP) */
    u_int64_t   ims_rx_ccmpmic;     /* rx MIC check failed (CCMP) */
    u_int64_t   ims_rx_wpireplay;  /* rx seq# violation (WPI) */
    u_int64_t   ims_rx_wpimic;     /* rx MIC check failed (WPI) */
    u_int64_t   ims_rx_countermeasure; /* rx TKIP countermeasure activation count */
    u_int64_t   ims_tx_mgmt;       /* tx mgmt frames */
    u_int64_t   ims_rx_mgmt;       /* rx mgmt frames */
    /* end of stats moved to cp stats component */
    u_int64_t   ims_tx_packets; /* frames successfully transmitted */
    u_int64_t   ims_rx_packets; /* frames successfully received */
    u_int64_t   ims_tx_bytes;	/* bytes successfully transmitted */
    u_int64_t	ims_rx_bytes;   /* bytes successfully received */

    /* TODO: For the byte counts below, we need to handle some scenarios
       such as encryption related decaps, etc */
    u_int64_t   ims_tx_data_packets;/* data frames successfully transmitted */
    u_int64_t   ims_rx_data_packets;/* data frames successfully received */
    u_int64_t   ims_tx_bcast_data_packets;/* bcast frames successfully transmitted */
    u_int64_t   ims_tx_bcast_data_bytes;/* bcast bytes successfully transmitted */
    u_int64_t   ims_rx_bcast_data_packets;/* bcast frames successfully received */
    u_int64_t   ims_rx_bcast_data_bytes;/* bcast bytes successfully received */
    u_int64_t   ims_tx_data_bytes;  /* data bytes successfully transmitted,
                                       inclusive of FCS. */
    u_int64_t   ims_rx_data_bytes;  /* data bytes successfully received,
                                       inclusive of FCS. */

    u_int64_t   ims_tx_datapyld_bytes;  /* data payload bytes successfully
                                           transmitted */
    u_int64_t   ims_rx_datapyld_bytes;  /* data payload successfully
                                           received */
    u_int64_t   ims_tx_wme[4]; /* data frames transmitted per AC */
    u_int64_t   ims_rx_wme[4]; /* data frames received per AC */

    /* Decryption errors */
    u_int64_t   ims_rx_unencrypted; /* rx w/o wep and privacy on */
    u_int64_t   ims_rx_decryptcrc;  /* rx decrypt failed on crc */
    u_int64_t   ims_rx_tkipmic;     /* rx MIC check failed (TKIP) */
    u_int64_t   ims_rx_fcserr;     /* rx MIC check failed (CCMP) */
    /* Other Tx/Rx errors */
    u_int64_t   ims_tx_discard;     /* tx dropped by NIC */
    u_int64_t   ims_rx_discard;     /* rx dropped by NIC */

    u_int64_t   ims_last_tx_rate;
    u_int64_t   ims_last_tx_rate_mcs;
    u_int64_t   ims_retries;       /* retries */
    u_int64_t   ims_tx_eapol_packets; /* tx eapol frames */
};

#ifdef __KERNEL__
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
#define stats_t u_int64_t
#else
#define stats_t u_int32_t
#endif
#endif

/*
 * Summary statistics.
 * Note: This structure should be 64 bit alligned.
 * Please make sure it is 64 bit(multiples of 8) alligned after adding new variables.
 */

struct ieee80211_stats {
    u_int64_t   is_rx_wrongbss;            /* rx from wrong bssid */
    u_int64_t   is_rx_wrongdir;            /* rx w/ wrong direction */
    u_int64_t   is_rx_mcastecho;           /* rx discard 'cuz mcast echo */
    u_int64_t   is_rx_notassoc;            /* rx discard 'cuz sta !assoc */
    u_int64_t   is_rx_noprivacy;           /* rx w/ wep but privacy off */
    u_int64_t   is_rx_mgtdiscard;          /* rx discard mgt frames */
    u_int64_t   is_rx_ctl;                 /* rx discard ctrl frames */
    u_int64_t   is_rx_rstoobig;            /* rx rate set truncated */
    u_int64_t   is_rx_elem_missing;        /* rx required element missing*/
    u_int64_t   is_rx_elem_toobig;         /* rx element too big */
    u_int64_t   is_rx_badchan;             /* rx frame w/ invalid chan */
    u_int64_t   is_rx_nodealloc;           /* rx frame dropped */
    u_int64_t   is_rx_ssidmismatch;        /* rx frame ssid mismatch  */
    u_int64_t   is_rx_auth_unsupported;    /* rx w/ unsupported auth alg */
    u_int64_t   is_rx_auth_fail;           /* rx sta auth failure */
    u_int64_t   is_rx_auth_countermeasures;/* rx auth discard 'cuz CM */
    u_int64_t   is_rx_assoc_bss;           /* rx assoc from wrong bssid */
    u_int64_t   is_rx_assoc_notauth;       /* rx assoc w/o auth */
    u_int64_t   is_rx_assoc_capmismatch;   /* rx assoc w/ cap mismatch */
    u_int64_t   is_rx_assoc_norate;        /* rx assoc w/ no rate match */
    u_int64_t   is_rx_assoc_badwpaie;      /* rx assoc w/ bad WPA IE */
    u_int64_t   is_rx_action;              /* rx action mgt */
    u_int64_t   is_rx_bad_auth;            /* rx bad auth request */
    u_int64_t   is_tx_nodefkey;            /* tx failed 'cuz no defkey */
    u_int64_t   is_tx_noheadroom;          /* tx failed 'cuz no space */
    u_int64_t   is_rx_acl;                 /* rx discard 'cuz acl policy */
    u_int64_t   is_rx_nowds;               /* 4-addr packets received with no wds enabled */
    u_int64_t   is_tx_nobuf;               /* tx failed for lack of buf */
    u_int64_t   is_tx_nonode;              /* tx failed for no node */
    u_int64_t   is_tx_badcipher;           /* tx failed 'cuz key type */
    u_int64_t   is_tx_not_ok;            /* tx ok not set in desc */
    u_int64_t   tx_beacon_swba_cnt;        /* Beacon intr SWBA counter host gets*/
    u_int64_t   is_node_timeout;           /* nodes timed out inactivity */
    u_int64_t   is_crypto_nomem;           /* no memory for crypto ctx */
    u_int64_t   is_crypto_tkip;            /* tkip crypto done in s/w */
    u_int64_t   is_crypto_tkipenmic;       /* tkip en-MIC done in s/w */
    u_int64_t   is_crypto_tkipcm;          /* tkip counter measures */
    u_int64_t   is_crypto_ccmp;            /* ccmp crypto done in s/w */
    u_int64_t   is_crypto_wep;             /* wep crypto done in s/w */
    u_int64_t   is_crypto_setkey_cipher;   /* cipher rejected key */
    u_int64_t   is_crypto_setkey_nokey;    /* no key index for setkey */
    u_int64_t   is_crypto_delkey;          /* driver key delete failed */
    u_int64_t   is_crypto_badcipher;       /* unknown cipher */
    u_int64_t   is_crypto_attachfail;      /* cipher attach failed */
    u_int64_t   is_crypto_swfallback;      /* cipher fallback to s/w */
    u_int64_t   is_crypto_keyfail;         /* driver key alloc failed */
    u_int64_t   is_crypto_enmicfail;       /* en-MIC failed */
    u_int64_t   is_ibss_capmismatch;       /* merge failed-cap mismatch */
    u_int64_t   is_ps_unassoc;             /* ps-poll for unassoc. sta */
    u_int64_t   is_ps_badaid;              /* ps-poll w/ incorrect aid */
    u_int64_t   padding;                   /* make structure size to 64 bit
                                              alligned. multiples of 8.
                                            */
    u_int64_t   total_num_offchan_tx_mgmt; /* total number of offchan TX mgmt frames */
    u_int64_t   total_num_offchan_tx_data; /* total number of offchan TX data frames */
    u_int64_t   num_offchan_tx_failed;     /* number of offchan TX frames failed */
    u_int64_t   total_invalid_macaddr_nodealloc_failcnt; /* number of invalid mac addr node alloc fail count */
    u_int64_t   tx_bcn_succ_cnt;
    u_int64_t   tx_bcn_outage_cnt;
    u_int64_t   sta_xceed_rlim;            /* no of connections refused after radio limit */
    u_int64_t   sta_xceed_vlim;            /* no of connections refused after vap limit */
    u_int64_t   mlme_auth_attempt;         /* no of 802.11 MLME Auth Attempt */
    u_int64_t   mlme_auth_success;         /* no of 802.11 MLME Auth Success */
    u_int64_t   authorize_attempt;         /* no of Authorization Attempt */
    u_int64_t   authorize_success;         /* no of Authorization successful */
    /* end of cp stats */
    u_int64_t   is_scan_active;            /* active scans started */
    u_int64_t   is_rx_unauth;              /* rx on unauthorized port */
    u_int64_t   is_rx_tooshort;            /* rx frame too short */
    u_int64_t   peer_delete_req;           /* no of peer delete req sent to target */
    u_int64_t   peer_delete_resp;          /* no of peer delete resp rcvd from target */
    u_int64_t   peer_delete_all_req;       /* no of peer delete all req */
    u_int64_t   peer_delete_all_resp;      /* no of peer delete all resp */
    u_int64_t   prob_req_drops;            /* no of drop request drops */
    u_int64_t   oob_probe_req_count;       /* no of out of band probe requests */
    u_int64_t   wc_probe_req_drops;        /* no of wildcard probe req drops*/
    /* unused, can be cleaned up */
    u_int64_t   is_rx_badversion;          /* rx frame with bad version */
    u_int64_t   is_rx_decap;               /* rx decapsulation failed */
    u_int64_t   is_rx_beacon;              /* rx beacon frames */
    u_int64_t   is_rx_elem_toosmall;       /* rx element too small */
    u_int64_t   is_rx_elem_unknown;        /* rx element unknown */
    u_int64_t   is_rx_chanmismatch;        /* rx frame chan mismatch */
    u_int64_t   is_rx_deauth;              /* rx deauthentication */
    u_int64_t   is_rx_disassoc;            /* rx disassociation */
    u_int64_t   is_rx_badsubtype;          /* rx frame w/ unknown subtype*/
    u_int64_t   is_rx_nobuf;               /* rx failed for lack of buf */
    u_int64_t   is_rx_ahdemo_mgt;          /* rx discard ahdemo mgt frame*/
    u_int64_t   is_rx_badcipher;           /* rx failed 'cuz key type */
    u_int64_t   is_rx_nocipherctx;         /* rx failed 'cuz key !setup */
    u_int64_t   is_rx_ffcnt;               /* rx fast frames */
    u_int64_t   is_rx_badathtnl;           /* driver key alloc failed */
    u_int64_t   is_tx_unknownmgt;          /* tx of unknown mgt frame */
    u_int64_t   is_tx_ffokcnt;             /* tx fast frames sent success */
    u_int64_t   is_tx_fferrcnt;            /* tx fast frames sent success */
    u_int64_t   is_scan_passive;           /* passive scans started */
    u_int64_t   is_crypto_tkipdemic;       /* tkip de-MIC done in s/w */
    u_int64_t   is_crypto_nocipher;        /* cipher not available */
    u_int64_t   is_ibss_norate;            /* merge failed-rate mismatch */
    u_int64_t   is_ps_qempty;              /* ps-poll w/ nothing to send */
    u_int64_t   tx_offer_pkt_cnt;          /* no of pkts offered to umac for Tx */
    u_int64_t   tx_offer_pkt_bytes_cnt;    /* no of bytes offered to umac for Tx*/
    u_int64_t   mgmt_tx_fail;              /* mgmt Tx failure */
    u_int64_t   ims_tx_packets;            /* frames successfully transmitted */
    u_int64_t   ims_tx_bytes;              /* bytes successfully transmitted */
    u_int64_t   total_rx_packets;          /* pkt count for good frames rcvd */
    u_int64_t   total_rx_bytes;            /* byte count for good frames rcvd */
    u_int64_t   fils_frames_sent;          /* Fils frame sent successfully*/
    u_int64_t   fils_frames_sent_fail;     /* Fils frame sent failed*/
    u_int64_t   tx_offload_prb_resp_succ_cnt;/* tx unsolicited probe response success count */
    u_int64_t   tx_offload_prb_resp_fail_cnt;/* tx unsolicited probe response fail count */
    u_int64_t   tx_20TU_prb_resp;            /* tx 20 TU probe response active status */
    u_int64_t   tx_20TU_prb_interval;        /* tx 20 TU probe response interval */
};

typedef enum _ieee80211_send_frame_type {
    IEEE80211_SEND_NULL,
    IEEE80211_SEND_QOSNULL,
} ieee80211_send_frame_type;

typedef struct _ieee80211_tspec_info {
    u_int8_t    traffic_type;
    u_int8_t    direction;
    u_int8_t    dot1Dtag;
    u_int8_t    tid;
    u_int8_t    acc_policy_edca;
    u_int8_t    acc_policy_hcca;
    u_int8_t    aggregation;
    u_int8_t    psb;
    u_int8_t    ack_policy;
    u_int16_t   norminal_msdu_size;
    u_int16_t   max_msdu_size;
    u_int32_t   min_srv_interval;
    u_int32_t   max_srv_interval;
    u_int32_t   inactivity_interval;
    u_int32_t   suspension_interval;
    u_int32_t   srv_start_time;
    u_int32_t   min_data_rate;
    u_int32_t   mean_data_rate;
    u_int32_t   peak_data_rate;
    u_int32_t   max_burst_size;
    u_int32_t   delay_bound;
    u_int32_t   min_phy_rate;
    u_int16_t   surplus_bw;
    u_int16_t   medium_time;
} ieee80211_tspec_info;

#ifndef EXTERNAL_USE_ONLY
/*
 * Manual ADDBA support
 */
enum {
    ADDBA_SEND     = 0,
    ADDBA_STATUS   = 1,
    DELBA_SEND     = 2,
    ADDBA_RESP     = 3,
    ADDBA_CLR_RESP = 4,
    SINGLE_AMSDU   = 5,
};

enum {
    ADDBA_MODE_AUTO   = 0,
    ADDBA_MODE_MANUAL = 1,
};

struct ieee80211_addba_delba_request {
    wlan_dev_t             ic;
    u_int8_t               action;
    u_int8_t               tid;
    u_int16_t              status;
    u_int16_t              aid;
    u_int32_t              arg1;
    u_int32_t              arg2;
};
#endif /* EXTERNAL_USE_ONLY */

#ifdef ATH_BT_COEX
typedef enum _ieee80211_bt_coex_info_type {
    IEEE80211_BT_COEX_INFO_SCHEME        = 0,
    IEEE80211_BT_COEX_INFO_BTBUSY        = 1,
} ieee80211_bt_coex_info_type;
#endif

struct tkip_countermeasure {
    u_int16_t   mic_count_in_60s;
    u_int32_t   timestamp;
} ;

enum _ieee80211_qos_frame_direction {
    IEEE80211_RX_QOS_FRAME = 0,
    IEEE80211_TX_QOS_FRAME = 1,
    IEEE80211_TX_COMPLETE_QOS_FRAME = 2
};

#define IEEE80211_MAX_QOS_UP_RANGE       8
#define IEEE80211_MAX_QOS_DSCP_EXCEPT    21

struct ieee80211_dscp_range {
    u_int8_t low;
    u_int8_t high;
};

struct ieee80211_dscp_exception {
    u_int8_t dscp;
    u_int8_t up;
};

struct ieee80211_qos_map {
    struct ieee80211_dscp_range
        up[IEEE80211_MAX_QOS_UP_RANGE];                 /* user priority map fields */
    u_int16_t valid;
    u_int16_t num_dscp_except;                          /* count of dscp exception fields */
    struct ieee80211_dscp_exception
        dscp_exception[IEEE80211_MAX_QOS_DSCP_EXCEPT];  /* dscp exception fields */
};

#ifdef WLAN_SUPPORT_SCS
/*
 * SCS procedure based data structures -
 * Should be used if SCS feature is supported
 */

#define IEEE80211_SCS_MAX_SIZE        10
#define IEEE80211_SCS_TCLAS_MAX_SIZE  10

#define IEEE80211_FLOW_LABEL_SIZE     3

struct tclas_tuple_type4 {
    u_int8_t mask;
    u_int8_t version;
    union {
        u_int8_t  ipv4[IEEE80211_IPV4_LEN];
        u_int8_t  ipv6[IEEE80211_IPV6_LEN];
    } src_ip;
    union {
        u_int8_t  ipv4[IEEE80211_IPV4_LEN];
        u_int8_t  ipv6[IEEE80211_IPV6_LEN];
    } dst_ip;
    u_int16_t src_port;
    u_int16_t dst_port;
    u_int8_t  dscp;
    /* Protocol in case of IPv4 and Next_Header in case of IPv6 */
    u_int8_t  next_header;
    /* Flow_Label is applicable only in case of IPv6 */
    u_int8_t  flow_label[IEEE80211_FLOW_LABEL_SIZE];
}__packed;

struct tclas_tuple_type10 {
    u_int8_t protocol_instance;
    u_int8_t protocol_number;
    u_int8_t filter_len;
    u_int8_t *filter_mask;
    u_int8_t *filter_val;
} __packed;

struct tclas_tuple {
    uint8_t type;
    union {
        struct tclas_tuple_type4 type4;
        struct tclas_tuple_type10 type10;
    } tclas;
} __packed;

#ifdef WLAN_FEATURE_11BE
#define IEEE80211_MIN_QOS_ATTR_LEN 21

#define IEEE80211_QOS_ATTR_CTRL_INFO_LEN                   4
#define IEEE80211_QOS_ATTR_SERVICE_INTERVAL_LEN            4
#define IEEE80211_QOS_ATTR_MIN_DATA_RATE_LEN               3
#define IEEE80211_QOS_ATTR_DELAY_BOUND_LEN                 3
#define IEEE80211_QOS_ATTR_MAX_MSDU_SIZE_LEN               2
#define IEEE80211_QOS_ATTR_SERVICE_START_TIME_LEN          4
#define IEEE80211_QOS_ATTR_SERVICE_START_TIME_LINK_ID_LEN  1
#define IEEE80211_QOS_ATTR_MEAN_DATA_RATE_LEN              3
#define IEEE80211_QOS_ATTR_BURST_SIZE_LEN                  4
#define IEEE80211_QOS_ATTR_MSDU_LIFETIME_LEN               2
#define IEEE80211_QOS_ATTR_MSDU_DELIVERY_RATIO_LEN         1
#define IEEE80211_QOS_ATTR_MSDU_COUNT_EXPONENT_LEN         1
#define IEEE80211_QOS_ATTR_MEDIUM_TIME_LEN                 2

#define IEEE80211_QOS_ATTR_MAX_MSDU_SIZE_PRESENT               0x0001
#define IEEE80211_QOS_ATTR_SERVICE_START_TIME_PRESENT          0x0002
#define IEEE80211_QOS_ATTR_SERVICE_START_TIME_LINK_ID_PRESENT  0x0004
#define IEEE80211_QOS_ATTR_MEAN_DATA_RATE_PRESENT              0x0008
#define IEEE80211_QOS_ATTR_BURST_SIZE_PRESENT                  0x0010
#define IEEE80211_QOS_ATTR_MSDU_LIFETIME_PRESENT               0x0020
#define IEEE80211_QOS_ATTR_MSDU_DELIVERY_RATIO_PRESENT         0x0040
#define IEEE80211_QOS_ATTR_MSDU_COUNT_EXPONENT_PRESENT         0x0080
#define IEEE80211_QOS_ATTR_MEDIUM_TIME_PRESENT                 0x0100

#define IEEE80211_QOS_CTRL_INFO_DIRECTION_MASK         0x00000003
#define IEEE80211_QOS_CTRL_INFO_DIRECTION_SHIFT        0
#define IEEE80211_QOS_CTRL_INFO_TID_MASK               0x0000003C
#define IEEE80211_QOS_CTRL_INFO_TID_SHIFT              2
#define IEEE80211_QOS_CTRL_INFO_USER_PRIORITY_MASK     0x000001C0
#define IEEE80211_QOS_CTRL_INFO_USER_PRIORITY_SHIFT    6
#define IEEE80211_QOS_CTRL_INFO_BITMAP_MASK            0x01FFFE00
#define IEEE80211_QOS_CTRL_INFO_BITMAP_SHIFT           9
#define IEEE80211_QOS_CTRL_INFO_LINK_ID_MASK           0x1E000000
#define IEEE80211_QOS_CTRL_INFO_LINK_ID_SHIFT          25

#define IEEE80211_GET_FIELD_FROM_QOS_CTRL_INFO(ctrl_info, FIELD) \
        ((ctrl_info & IEEE80211_QOS_CTRL_INFO_##FIELD##_MASK) >> IEEE80211_QOS_CTRL_INFO_##FIELD##_SHIFT)

#define IEEE80211_CHECK_QOS_ATTR_PRESENT(bitmap, FIELD) \
        (bitmap & IEEE80211_QOS_ATTR_##FIELD##_PRESENT)

#define IEEE80211_QOS_ATTR_INVALID_SERVICE_CLASS_ID    0xFFFF

enum ieee80211_qos_attr_direction_enum {
    IEEE80211_QOS_ATTR_DIRECTION_UPLINK = 0,
    IEEE80211_QOS_ATTR_DIRECTION_DOWNLINK,
    IEEE80211_QOS_ATTR_DIRECTION_DIRECT_LINK,
};

uint8_t ieee80211_get_scs_qos_attr_direction(struct ieee80211_node *ni,
                                             uint8_t idx);
#endif /* WLAN_FEATURE_11BE */
#endif /* WLAN_SUPPORT_SCS */

#if (defined(WLAN_FEATURE_11BE) && defined(WLAN_SUPPORT_SCS)) || defined(CONFIG_SAWF)

#define SCS_SLA_DEFAULT_VALUE    90

struct ieee80211_qos_attr {
    uint32_t service_class_id;
    uint32_t direction:2,
             tid:4,
             user_priority:3,
             bitmap:16,
             link_id:4,
             reserved:3;
    uint32_t min_service_interval;
    uint32_t max_service_interval;
    uint32_t resultant_service_interval;
    uint32_t min_data_rate;
    uint32_t delay_bound;
    uint16_t max_msdu_size;
    uint32_t service_start_time;
    uint8_t service_start_time_link_id;
    uint32_t mean_data_rate;
    uint32_t burst_size;
    uint32_t resultant_burst_size;
    uint16_t msdu_lifetime;
    uint8_t msdu_delivery_ratio;
    uint8_t msdu_count_exponent;
    uint16_t medium_time;
    uint8_t access_category;
    uint8_t actual_rx_tid;
    TAILQ_ENTRY(ieee80211_qos_attr) qos_attr_elem;
} __packed;
#endif /* (WLAN_FEATURE_11BE && WLAN_SUPPORT_SCS) || CONFIG_SAWF */

#ifdef WLAN_SUPPORT_SCS
enum ieee80211_scs_state {
    SCS_STATE_INIT,
    SCS_STATE_APP_RESP_PENDING,
    SCS_STATE_ACTIVE,
};

struct ieee80211_scs_data {
    uint32_t rule_id_prefix;
    uint8_t scsid;
    u_int8_t request_type;
    uint8_t access_priority;
    uint8_t tclas_elements;
    struct tclas_tuple tclas_tuple[IEEE80211_SCS_TCLAS_MAX_SIZE];
    struct ieee80211_tclas_processing tclas_processing;
#ifdef WLAN_FEATURE_11BE
    struct ieee80211_qos_attr *qos_attr;
#endif
    enum ieee80211_scs_state scs_state;
};

struct ieee80211_scs_resp {
    uint8_t dialog_token;
    uint8_t num_scs_resp;
    uint8_t scsid[IEEE80211_SCS_MAX_SIZE];
    uint8_t status[IEEE80211_SCS_MAX_SIZE];
};

#define IEEE80211_SCS_TCLAS4_VERSION     0x01
#define IEEE80211_SCS_TCLAS4_SRC_IP      0x02
#define IEEE80211_SCS_TCLAS4_DST_IP      0x04
#define IEEE80211_SCS_TCLAS4_SRC_PORT    0x08
#define IEEE80211_SCS_TCLAS4_DST_PORT    0x10
#define IEEE80211_SCS_TCLAS4_DSCP        0x20
#define IEEE80211_SCS_TCLAS4_NEXT_HEADER 0x40
#define IEEE80211_SCS_TCLAS4_FLOW_LABEL  0x80

#define IEEE80211_SCS_RULE_SOC_ID_SHIFT      28
#define IEEE80211_SCS_RULE_SOC_ID_MASK       0xF0000000

#define IEEE80211_SCS_RULE_PEER_ID_SHIFT     12
#define IEEE80211_SCS_RULE_PEER_ID_MASK      0x0FFFF000

#define IEEE80211_SCS_RULE_SCS_ID_SHIFT      4
#define IEEE80211_SCS_RULE_SCS_ID_MASK       0x00000FF0

#define IEEE80211_SCS_RULE_TCLAS_INDEX_SHIFT 0
#define IEEE80211_SCS_RULE_TCLAS_INDEX_MASK  0x0000000F

#define IEEE80211_SCS_RULE_TCLAS_INDEX_MATCH_ALL 0xF

#define IEEE80211_FETCH_ID_FROM_SCS_RULE_ID(_rule_id, TYPE) ( \
         (_rule_id & IEEE80211_SCS_RULE_##TYPE##_MASK) >> IEEE80211_SCS_RULE_##TYPE##_SHIFT)

#define IEEE80211_SCS_GET_SCS_ID_FROM_RULEID(rule_id) \
        (((rule_id & IEEE80211_SCS_RULE_SCS_ID_MASK) >> IEEE80211_SCS_RULE_SCS_ID_SHIFT))
#define IEEE80211_SCS_GET_TCLAS_INDEX_FROM_RULEID(rule_id) \
        (((rule_id & IEEE80211_SCS_RULE_TCLAS_INDEX_MASK) >> IEEE80211_SCS_RULE_TCLAS_INDEX_SHIFT))

#define IEEE80211_SCS_OUTPUT_TID_MASK 0x7

struct ieee80211_scs_rule_config_intf {
    uint32_t rule_id;
    uint8_t req_type;
    uint8_t output_tid;
    uint8_t classifier_type;

    struct {
        uint8_t classifier_mask;
        uint8_t version;

        union {
             uint8_t ipv4[IEEE80211_IPV4_LEN];
             uint8_t ipv6[IEEE80211_IPV6_LEN];
        } src_ip_addr;

        union {
             uint8_t ipv4[IEEE80211_IPV4_LEN];
             uint8_t ipv6[IEEE80211_IPV6_LEN];
        } dst_ip_addr;

        uint16_t src_port;
        uint16_t dst_port;
        uint8_t dscp;
        uint8_t next_header;
        uint8_t flow_label[IEEE80211_FLOW_LABEL_SIZE];
    } tclas4_element;

    struct {
        uint8_t protocol_instance;
        uint8_t protocol_no;
        uint8_t filter_length;
        uint8_t *filter_mask;
        uint8_t *filter_value;
    } tclas10_element;

    uint16_t service_class_id;

#ifdef QCA_SUPPORT_WDS_EXTENDED
    uint8_t dst_mac_addr_valid:1,
            netdev_if_index_valid:1;
    uint8_t dst_mac_addr[IEEE80211_ADDR_LEN];
    uint32_t netdev_if_index;
#endif
};

void wlan_cfg80211_scs_rule_config(wlan_if_t vap,
                                   struct ieee80211_scs_rule_config_intf *scs_rule_intf);

uint8_t ieee80211_check_for_existing_scsid(struct ieee80211_node *ni,
                                           uint8_t scs_id);

uint8_t ieee80211_send_scs_rule_config_event(struct ieee80211_node *ni,
                                             uint8_t idx, uint8_t request_type);

#define scs_err(params...) QDF_TRACE_ERROR(QDF_MODULE_ID_SCS, params)
#define scs_info(params...) QDF_TRACE_INFO(QDF_MODULE_ID_SCS, params)
#define scs_debug(params...) QDF_TRACE_DEBUG(QDF_MODULE_ID_SCS, params)
#define scs_trace(params...) \
        __QDF_TRACE_FL(QDF_TRACE_LEVEL_TRACE, QDF_MODULE_ID_SCS, ## params)

#endif

#define IEEE80211_MSCS_MAX_SIZE       4

/* MSCS Procedure based data structures -
 * Should be used if MSCS feature is supported
 */

struct ieee80211_mscs_assoc_data {
   uint8_t is_present:1;
   uint16_t retval;
};

struct ieee80211_mscs_priority_stats {
    u_int8_t user_priority_bitmap;      /* Bitmap representing the supported User priority values */
    u_int8_t user_priority_limit;
    u_int32_t stream_timeout;
};

struct ieee80211_mscs_service{
    u_int8_t classifier_mask;
    u_int8_t classifier_type;
    u_int8_t  user_priority;
};


/* Tuple for every MSCS session */
struct ieee80211_mscs_data{
    u_int8_t mscs_active:1;
    u_int8_t tuple_ctr;
    struct ieee80211_mscs_service node_service[IEEE80211_MSCS_MAX_SIZE];
    struct ieee80211_mscs_priority_stats node_stats;
    struct ieee80211_mscs_assoc_data mscs_assoc;
};

#if WLAN_SUPPORT_DAP
struct ieee80211_dscp_action_table {
    uint8_t policy_id;
    uint8_t status_code;
};

struct ieee80211_dscp_data {
    struct ieee80211_dscp_action_table response_table[IEEE80211_DSCP_ACTION_MAX_POLICIES];
    uint8_t dialog_token;
};
#endif

typedef struct ieee80211_vap_opmode_count {
    int    total_vaps;
    int    ibss_count;
    int    sta_count;
    int    wds_count;
    int    ahdemo_count;
    int    ap_count;
    int    monitor_count;
    int    btamp_count;
    int    unknown_count;
} ieee80211_vap_opmode_count;

struct ieee80211_app_ie_t {
        u_int32_t       length;
        u_int8_t        *ie;
};

/* NOTE: This macro corresponds to macro ACS_RANK_DESC_LEN, Please change it
 * aswell if changing this.
 */
#define ACS_RANK_DESC_DBG_LEN 80
typedef enum {
        ACS_RUN_IDLE, /* acs is not yet started */
        ACS_RUN_INCOMPLETE, /* acs run is incomplete/cancelled due to ICM or some other reason */
        ACS_RUN_COMPLETED,
} ACS_RUN_STATUS;

typedef enum {
        ACS_DEFAULT,
        ACS_SUCCESS,
        ACS_FAILED_NBSS,
        ACS_FAILED_RNDM,
} ACS_STATUS;

/* ACS Channel Ranking structure
 * rank: Channel Rank
 * desc: Reason in case of no rank
 */
typedef struct acs_rank_dbg {
    u_int32_t rank;
    char desc[ACS_RANK_DESC_DBG_LEN];
}acs_rank_dbg_t;


typedef enum {
    ACS_CHAN_STATS,
    ACS_NEIGHBOUR_GET_LIST_COUNT,
    ACS_NEIGHBOUR_GET_LIST,
    ACS_CHAN_NF_STATS,
    ACS_CHAN_WEIGHT_STATS,
    ACS_CHAN_PWR_STATS,
} ACS_LIST_TYPE;

/* ACS debug structure to
 * export report to user tool
 */
struct ieee80211_acs_dbg {
    u_int8_t  nchans;
    u_int8_t  entry_id;
    u_int16_t chan_freq;
    enum wlan_band_id chan_band;
    u_int8_t  ieee_chan;
    u_int8_t  chan_nbss;
    int32_t   chan_maxsnr;
    int32_t   chan_minsnr;
    int32_t   chan_maxrssi;
    int32_t   chan_minrssi;
    int16_t   noisefloor;
    int16_t   perchain_nf[HOST_MAX_CHAINS];
    int16_t   channel_loading;
    u_int32_t chan_load;
    u_int8_t  sec_chan;
    int32_t   chan_nbss_srp;
    int32_t   chan_srp_load;
    u_int8_t  chan_in_pool;
    u_int8_t  chan_radar_noise;
    int32_t neighbor_size;
    ieee80211_neighbor_info *neighbor_list;
    u_int32_t chan_80211_b_duration;
    acs_rank_dbg_t acs_rank;     /* ACS Rank and channel reject code */
    u_int8_t  acs_status;
    ACS_LIST_TYPE acs_type;
    uint32_t chan_availability;
    uint32_t chan_efficiency;
    uint32_t chan_nbss_near;
    uint32_t chan_nbss_mid;
    uint32_t chan_nbss_far;
    uint32_t chan_nbss_eff;
    uint32_t chan_grade;
    u_int8_t op_class;
    u_int8_t chan_width;
    uint32_t chan_weightage;
    int32_t  power;
#ifdef WLAN_FEATURE_11BE
    uint8_t  ieee_ccfs;
    uint8_t  ieee_ccfs_overlap;
    uint16_t puncture_bitmap;
    uint16_t puncture_bitmap_overlap;
    uint32_t chan_efficiency_overlap;
    acs_rank_dbg_t acs_rank_overlap;     /* ACS Rank and channel reject code for overlap channel */
    int32_t power_overlap;
#endif
};

/*
 * MAC ACL operations.
 */
typedef enum {
    IEEE80211_MACCMD_POLICY_OPEN,  /* set policy: no ACL's */
    IEEE80211_MACCMD_POLICY_ALLOW,  /* set policy: allow traffic */
    IEEE80211_MACCMD_POLICY_DENY,  /* set policy: deny traffic */
    IEEE80211_MACCMD_FLUSH,  /* flush ACL database */
    IEEE80211_MACCMD_DETACH,  /* detach ACL policy */
    IEEE80211_MACCMD_POLICY_RADIUS,  /* set policy: RADIUS managed ACLs */
    IEEE80211_MACCMD_IMMEDIATE   /* ACL policies to take effect immediately */
} ieee80211_acl_cmd;

#if QCA_SUPPORT_ACL_IMMEDIATE
/*
 * MAC ACL actions for ACL immediate
 */
enum {
    IEEE80211_ACL_IMMEDIATE_ACTION_ADDMAC = 0,
    IEEE80211_ACL_IMMEDIATE_ACTION_DELMAC = 1,
    IEEE80211_ACL_IMMEDIATE_ACTION_POLICY = 2,
};
#endif /* QCA_SUPPORT_ACL_IMMEDIATE */

#define IEEE80211_CHAN_MAXHIST    32

#define IEEE80211_LP_IOT_BCN_INTVAL_DEFAULT 25 /* Low-power IOT device default beacon interval is 25msecs */
#define WLAN_DEFAULT_NETLINK_PID 0xffffffff
enum {
    IEEE80211_TR069_CHANHIST           = 1,
    IEEE80211_TR069_TXPOWER            = 2,
    IEEE80211_TR069_GETTXPOWER         = 3,
    IEEE80211_TR069_GUARDINTV          = 4,
    IEEE80211_TR069_GET_GUARDINTV      = 5,
    IEEE80211_TR069_GETASSOCSTA_CNT    = 6,
    IEEE80211_TR069_GETTIMESTAMP       = 7,
    IEEE80211_TR069_GETDIAGNOSTICSTATE = 8,
    IEEE80211_TR069_GETNUMBEROFENTRIES = 9,
    IEEE80211_TR069_GET11HSUPPORTED    = 10,
    IEEE80211_TR069_GETPOWERRANGE      = 11,
    IEEE80211_TR069_SET_OPER_RATE      = 12,
    IEEE80211_TR069_GET_OPER_RATE      = 13,
    IEEE80211_TR069_GET_POSIBLRATE     = 14,
    IEEE80211_TR069_SET_BSRATE         = 15,
    IEEE80211_TR069_GET_BSRATE         = 16,
    IEEE80211_TR069_GETSUPPORTEDFREQUENCY  = 17,
    IEEE80211_TR069_GET_PLCP_ERR_CNT   = 18,
    IEEE80211_TR069_GET_FCS_ERR_CNT    = 19,
    IEEE80211_TR069_GET_PKTS_OTHER_RCVD = 20,
    IEEE80211_TR069_GET_FAIL_RETRANS_CNT = 21,
    IEEE80211_TR069_GET_RETRY_CNT      = 22,
    IEEE80211_TR069_GET_MUL_RETRY_CNT  = 23,
    IEEE80211_TR069_GET_ACK_FAIL_CNT   = 24,
    IEEE80211_TR069_GET_AGGR_PKT_CNT   = 25,
    IEEE80211_TR069_GET_STA_BYTES_SENT = 26,
    IEEE80211_TR069_GET_STA_BYTES_RCVD = 27,
    IEEE80211_TR069_GET_DATA_SENT_ACK  = 28,
    IEEE80211_TR069_GET_DATA_SENT_NOACK = 29,
    IEEE80211_TR069_GET_CHAN_UTIL      = 30,
    IEEE80211_TR069_GET_RETRANS_CNT    = 31,
    IEEE80211_TR069_GET_RRM_UTIL       = 32,
};

#define IS_NULL_ADDR(_a)            \
    ((_a)[0] == 0x00 &&             \
     (_a)[1] == 0x00 &&             \
     (_a)[2] == 0x00 &&             \
     (_a)[3] == 0x00 &&             \
     (_a)[4] == 0x00 &&             \
     (_a)[5] == 0x00)

#define SNR_MIN  1
#define SNR_MAX  127

#define MAX_FT_IE_LEN 257
#define MIN_FT_IE_LEN 84
#define MAX_RSN_IE_LEN 256
#define MIN_RSN_IE_LEN 36
#define MD_IE_LEN 5
struct ieee80211_ft_params {
    u_int8_t    *fties;
    u_int32_t   fties_len;
    u_int8_t    target_ap[IEEE80211_ADDR_LEN];
};

struct ieee80211_roam_t {
    u_int32_t                         iv_roaming:1,
                                      iv_ft_enable:1,
                                      iv_ft_roam:1,
                                      iv_wait_for_ftie_update:1,
                                      iv_roam_disassoc:1;
    u_int8_t                          iv_mdie[MD_IE_LEN];
    struct ieee80211_app_ie_t         iv_ft_ie; /* holds fties provided by supplicant */
    struct ieee80211_ft_params        iv_ft_params; /* holds fties parsed from ft auth-response to be provided to supp */
};

/* MAX_FRAGS_PER_RAW_TX_MPDU: This value will be per chipset. Currently all
   suported chipsets have this value. If this is changed per chipset max has
   to be taken care of. */
#define MAX_FRAGS_PER_RAW_TX_MPDU 6

#if QLD
#define QLD_MAX_NAME 48
#define QLD_START_EVENT 0x01
#define QLD_MORE_EVENT  0x02
#define QLD_END_EVENT   0x04
#define QLD_WAIT_TIMEOUT_COUNT 30000  /* 30sec timeout */

/**
 * struct qld_enry - Individual entry in qld_event
 * @addr:          Start address of object to dump
 * @size:          Size of memory dump
 * @name:          Name of memory dump
 */
struct ol_qld_entry {
    uint64_t addr;
    uint32_t size;
    char name[QLD_MAX_NAME];
} __packed;

/**
 * struct qld_event - qld_event
 * @total_list_count:  total qld_list count
 * @current_event_size:current size of event buffer including header
 * @flags:             Flags used for event communication
 * @cmd:               command to send to userspace.
 * @qld_buffer:        buffer for continuous ol_qld_entries
 */
struct qld_event {
    uint32_t total_list_count;
    uint32_t current_event_size;
    uint8_t  flags;
    uint8_t  cmd;
    struct ol_qld_entry qld_buffer[0];
} __packed;

/**
 * struct qld_userentry - Userspace entry
 * @u_addr:        Userspace start addr of dump
 * @entry:         qld_entry structure
 */
struct qld_userentry {
    uint64_t u_addr;
    struct ol_qld_entry entry;
} __packed;

/**
 * struct qldinforeq - qldinfo request
 * @qld_event_buf:     qld event buffer
 * @entry:         qld entry
 * @index:             index of current entry
 * @space:             Space needed for sending all entries including header
 */
struct qldinforeq {
    struct  qld_event *qld_event_buf;
    struct  ol_qld_entry entry;
    size_t  index;
    size_t  space;
};
#endif /* QLD */

/*
 * Bitmask to indicate the config change causing pdev restart
 */
#define PDEV_CFG_TXCHAINMASK 0x1
#define PDEV_CFG_RXCHAINMASK 0x2

/**
 * get_wlan_band_id_to_string() - convert band id enum to printable string
 * @id: enum wlan_band_id
 *
 * Return: pointer to printable string
 */
static inline const char *
get_wlan_band_id_to_string(enum wlan_band_id id) {
    const char * const wlan_band_id_to_string_map [] = {
        [WLAN_BAND_UNSPECIFIED] = "Unknown",
        [WLAN_BAND_2GHZ] = "2.4GHz",
        [WLAN_BAND_5GHZ] = "5GHz",
        [WLAN_BAND_6GHZ] = "6GHz",
        [WLAN_BAND_MAX]  = "Unknown",
    };

    if ((id < WLAN_BAND_UNSPECIFIED) || (id >= WLAN_BAND_MAX)) {
        id = WLAN_BAND_MAX;
    }

    return wlan_band_id_to_string_map[id];
}

/* To specify the buffer where IEs will be checked while addition */
enum {
    TYPE_APP_IE_BUF = 0x1, /* LSB set (01) */
    TYPE_OPT_IE_BUF = 0x2, /* MSB set (10) */
    TYPE_ALL_BUF = 0x3 /* All bit set (11) */
};

/* Interface for External modules to interact with wlan driver */
typedef ieee80211_frame_type wlan_frame_type;

typedef enum ieee80211_cwm_width wlan_ch_width;

typedef ieee80211_ssid wlan_ssid;

typedef enum ieee80211_phymode wlan_phy_mode;

#ifdef CONFIG_BAND_6GHZ
#define MAX_OPERATING_CLASSES 22
#else
#define MAX_OPERATING_CLASSES 17
#endif

#ifdef CONFIG_BAND_6GHZ
#define MAX_CHANNELS_PER_OP_CLASS  70
#else
#define MAX_CHANNELS_PER_OP_CLASS  25
#endif

#define WME_AC_MAX 4

#define MAX_HE_MCS 6

#define MAX_EHT_MCS 8

#define IEEE1905_MAX_ROLES 2

typedef struct wlan_radio_basic_capabilities {
    // Maximum number of BSSes supported by this radio
    u_int8_t max_supp_bss;

    // Number of operating classes supported for this radio
    u_int8_t num_supp_op_classes;

    // Info for each supported operating class
    struct {
	// Operating class that this radio is capable of operating on
	// as defined in Table E-4.
	u_int8_t opclass;

	// Maximum transmit power EIRP the radio is capable of transmitting
	// in the current regulatory domain for the operating class.
	// The field is coded as 2's complement signed dBm.
	int8_t max_tx_pwr_dbm;

	// Number of statically non-operable channels in the operating class
	// Other channels from this operating class which are not listed here
	// are supported by this radio.
	u_int8_t num_non_oper_chan;

	// Channel number which is statically non-operable in the operating class
	// (i.e. the radio is never able to operate on this channel)
	u_int8_t non_oper_chan_num[MAX_CHANNELS_PER_OP_CLASS];
    } opclasses[MAX_OPERATING_CLASSES];

    // Basic Radio capabilities are valid
    u_int8_t wlan_radio_basic_capabilities_valid : 1;
} wlan_radio_basic_capabilities;

/*
 * Get the DFS channel parameters of a given input freq.
 */
struct ieee80211_dfs_ch_params {
    uint8_t op_class;
    uint8_t chan_num;
    enum wlan_channel_dfs_state chan_state;
    uint32_t remaining_cac_time;
    uint64_t cac_completion_time;
    uint32_t remaining_nol_time;
};

typedef struct ieee80211req_dfs_chan_list_info {
    uint8_t num_chans;
    struct ieee80211_dfs_ch_params dfs_chan_info[NUM_5GHZ_CHANS];
} wlan_radio_chan_list_info;

typedef struct wlan_op_chan {
    u_int8_t num_supp_op_classes;
    // Operating class that this radio is capable of operating on
    // as defined in Table E-4.
    u_int8_t opclass;

    // operating channel width
    wlan_ch_width ch_width;

    // Number of statically operable channels in the operating class
    u_int8_t num_oper_chan;

    // Channel number which is statically operable in the operating class
    u_int8_t oper_chan_num[MAX_CHANNELS_PER_OP_CLASS];

    u_int8_t dfs_required : 1;
} wlan_op_chan;

/**
 * @brief Parameters to store the channel set information for an op class
 */
typedef struct wlan_op_class {
    // Operating class that this radio is capable of operating on
    // as defined in Table E-4.
    u_int8_t opclass;

    // operating channel width
    wlan_ch_width ch_width;

    // secondary channel location w.r.t. primary channel
    // The location is defined by constants IEEE80211_SEC_CHAN_OFFSET_SC{N,A,B}
    u_int8_t sc_loc;

    // Number of channels in the operating class
    u_int8_t num_chan;

    // Channel numbers defined in the regulatory domain
    u_int8_t channels[MAX_CHANNELS_PER_OP_CLASS];
} wlan_op_class;

typedef struct wlan_esp_data {
    struct {
	u_int8_t ba_window_size;
	u_int8_t est_air_time_fraction;
	u_int16_t data_ppdu_dur_target;
    } per_ac[WME_AC_MAX];
} wlan_esp_data;

typedef enum wlan_mlme_pdev_param {
    PDEV_SET_SEC_HM_WDS,
    PDEV_GET_ESP_INFO,
    PDEV_GET_DESC_POOLSIZE,
    PDEV_GET_PHY_ERRCNT,
    PDEV_GET_CAPABILITY,
    PDEV_GET_OPERABLE_CHAN,
    PDEV_GET_OPERABLE_CLASS,
    PDEV_GET_OPCLASS_FOR_CUR_HWMODE,
    PDEV_SET_EMESH_RELAYFS_INIT,
    PDEV_SET_EMESH_RELAYFS_DEINIT,
    PDEV_GET_CAC_TLV,
} wlan_mlme_pdev_param;

union wlan_mlme_pdev_data {
    wlan_esp_data esp_info;
    int32_t desc_poolsize;
    u_int32_t phy_errcnt;
    wlan_radio_basic_capabilities cap;
    wlan_radio_chan_list_info cac_tlv;
    wlan_op_chan op_chan;
    wlan_op_class op_class;
    u_int8_t hmwds_en : 1;
};

typedef struct wlan_app_ie_data {
    wlan_frame_type ftype;
    u_int8_t *ie;
    u_int32_t size;
    u_int8_t id;
} wlan_app_ie_data;

typedef enum wlan_acl_flag {
    WLAN_ACL_FLAG_PROBE_RESP_WH,
    WLAN_ACL_FLAG_AUTH_ALLOW,
} wlan_acl_flag;

typedef struct wlan_acl_data {
    u_int8_t dstmac[IEEE80211_ADDR_LEN];
    wlan_acl_flag flag;
    u_int16_t time_sec;
} wlan_acl_data;

typedef struct wlan_rssi_threshold_data {
    u_int8_t mac[IEEE80211_ADDR_LEN];
    u_int32_t inact_rssi_low;
    u_int32_t inact_rssi_high;
    u_int32_t low_rssi;
    u_int32_t low_rate_rssi;
    u_int32_t high_rate_rssi;
} wlan_rssi_threshold_data;

typedef struct wlan_chan {
    u_int16_t ic_freq;
    u_int8_t  ic_ieee;
    u_int64_t ic_flags;
} wlan_chan;

#define IEEE80211_IS_CHANNEL_VALID(c) (c && c->ic_freq)

typedef struct wlan_ap_ht_capabilities {
    // Maximum number of supported Tx spatial streams
    u_int8_t max_tx_nss;
    // Maximum number of supported Rx spatial streams
    u_int8_t max_rx_nss;
    // HT caps, uses bit fields
    u_int16_t htcap;
} wlan_ap_ht_capabilities;

typedef struct wlan_ap_vht_capabilities {
    // Set to Tx VHT MCS Map field per Figure 9-562.
    // Supported VHT Tx MCS
    u_int16_t supp_tx_mcs;
    // Set to Rx VHT MCS Map field per Figure 9-562.
    // Supported VHT Rx MCS
    u_int16_t supp_rx_mcs;
    // Maximum number of supported Tx spatial streams
    u_int8_t max_tx_nss;
    // Maximum number of supported Rx spatial streams
    u_int8_t max_rx_nss;
    // VHT caps, uses bit fields
    u_int32_t vhtcap;
} wlan_ap_vht_capabilities;

typedef struct wlan_ap_he_capabilities {
    u_int8_t num_mcs_entries;
    u_int16_t supported_he_mcs[MAX_HE_MCS];
    u_int8_t max_tx_nss;
    u_int8_t max_rx_nss;
    u_int32_t phyinfo[HEHANDLE_CAP_PHYINFO_SIZE];
    u_int32_t he_relaxed_edca:1,      /* HE Signalls EDCA support Enable:1 Disable:0 */
              he_spatial_reuse:1,     /* HE Signals Spatial Reuse Enable:1 Disable:0 */
              he_multi_bss:1,         /* HE Multi Bss Enable:1 Disable:0 */
              he_su_ppdu_1x_ltf_800ns_gi:1, /* HE Signals 800ns GI for 1x ltf */
              he_su_mu_ppdu_4x_ltf_800ns_gi:1, /* HE Signals 800ns GI for 4x ltf */
              he_ndp_4x_ltf_3200ns_gi:1, /* HE Signals 3200ns GI for NDP with 4x HE-LTF */
              he_er_su_ppdu_1x_ltf_800ns_gi:1, /* HE Signals 800ns GI for ER SU with 4x HE-LTF */
              he_er_su_ppdu_4x_ltf_800ns_gi:1, /* HE Signals 800ns GI for ER SU with 1x HE-LTF */
              he_su_bfee:1,           /* HE SU BFEE */
              he_su_bfer:1,           /* HE SU BFER */
              he_mu_bfee:1,           /* HE MU BFEE */
              he_mu_bfer:1,           /* HE MU BFER */
              he_ul_muofdma:1,        /* HE UL MU OFDMA */
              he_dl_muofdma:1,        /* HE DL MU OFDMA */
              he_dl_muofdma_bfer:1,   /* HE DL MU OFDMA BFER*/
              he_ul_mumimo:1,         /* HE DL MU MIMO */
              he_sounding_mode:4,     /* HE/VHT, SU/MU and Trig/Non-Trig sounding */
              he_muedca:1;            /* HE MU EDCA */
} wlan_ap_he_capabilities;

typedef struct wlan_ap_wifi6_capabilities {
    u_int8_t numofroles;
    struct {
	u_int8_t role : 2;
	u_int8_t he160 : 1;
	u_int8_t he80plus80 : 1;
	u_int16_t supported_he_mcs[MAX_HE_MCS];
	u_int8_t subeamformer : 1;
	u_int8_t subeamformee : 1;
	u_int8_t mu_beam_former_status : 1;
	u_int8_t beam_formee_sts_less_than_80supported : 1;
	u_int8_t beam_formee_sts_more_than_80supported : 1;
	u_int8_t ulmumimosupported : 1;
	u_int8_t ulofdmasupported : 1;
	u_int8_t dlofdmasupported : 1;
	u_int8_t maxuser_per_dl_mumimotxap : 4;
	u_int8_t max_user_per_dl_mumimorxap : 4;
	u_int8_t maxuserdlofdmatxap;
	u_int8_t maxuserdlofdmarxap;
	u_int8_t rtssupported : 1;
	u_int8_t murtssupported : 1;
	u_int8_t multibssidsupported : 1;
	u_int8_t muedcasupported : 1;
	u_int8_t twtrequestersupprted : 1;
	u_int8_t twtrespondersupported : 1;
    } role_cap[IEEE1905_MAX_ROLES];
} wlan_ap_wifi6_capabilities;

typedef struct wlan_ap_eht_capabilities {
    u_int8_t num_mcs_entries;
    u_int32_t supported_eht_mcs[MAX_EHT_MCS];
    u_int8_t max_tx_nss;
    u_int8_t max_rx_nss;
} wlan_ap_eht_capabilities;

typedef struct wlan_ap_cap {
    wlan_ap_ht_capabilities htcap;
    wlan_ap_vht_capabilities vhtcap;
    wlan_ap_he_capabilities hecap;
    wlan_ap_wifi6_capabilities wifi6cap;
    wlan_ap_eht_capabilities ehtcap;
	     // HT capabilities are valid
    u_int8_t wlan_ap_ht_capabilities_valid : 1,

	     // VHT capabilities are valid
	     wlan_ap_vht_capabilities_valid : 1,

	     // HE capabilities are valid
	     wlan_ap_he_capabilities_valid : 1,

	     wlan_ap_wifi6_capabilites_valid : 1,

	     // EHT capabilities are valid
	     wlan_ap_eht_capabilities_valid : 1,

	     reserved : 2;
} wlan_ap_cap;

typedef struct wlan_node_info {
    wlan_ch_width max_chwidth;
    wlan_phy_mode phymode;
    u_int8_t num_streams;
    u_int8_t max_MCS;
    u_int8_t max_txpower;
    /* Set to 1 if this client is operating in Static SM Power Save mode */
    u_int8_t is_static_smps : 1,
    /* Set to 1 if this client supports MU-MIMO */
	     is_mu_mimo_supported : 1;
} wlan_node_info;

typedef struct wlan_mctbl_data {
    void *data;
    u_int32_t len;
} wlan_mctbl_data;

typedef enum wlan_mlme_vdev_param {
    VDEV_SET_IE,
    VDEV_CLR_IE,
    VDEV_SET_ACL,
    VDEV_CLR_ACL,
    VDEV_SET_ACL_TIMER,
    VDEV_SET_PEER_ACT_STATS,
    VDEV_SET_SEC_STA_WDS,
    VDEV_SET_MEC,
    VDEV_SET_MBO_IE_BSTM,
    VDEV_SET_WPS_ACL_ENABLE,
    VDEV_SET_WNM_BSS_PREF,
    VDEV_SET_SON_MAP_VERSION,
    VDEV_SET_MCTBL,
    VDEV_GET_NSS,
    VDEV_GET_CHAN,
    VDEV_GET_CHAN_WIDTH,
    VDEV_GET_CHAN_UTIL,
    VDEV_GET_APCAP,
    VDEV_GET_CONNECT_N_TX,
    VDEV_GET_SSID,
    VDEV_GET_MAX_PHYRATE,
    VDEV_GET_ACL,
    VDEV_GET_ACL_RSSI_THRESHOLDS,
    VDEV_GET_NODE_CAP,
    VDEV_GET_WDS,
} wlan_mlme_vdev_param;

union wlan_mlme_vdev_data {
    /* Setting App IE */
    wlan_app_ie_data appie;
    /* ACL configuration */
    wlan_acl_data acl;
    /* ACL RSSI thresholds */
    wlan_rssi_threshold_data acl_rssi;
    /* struct mapapcap_t apcap; */
    wlan_ap_cap apcap;
    /* struct ieee80211_bsteering_datarate_info_t rate_info; */
    wlan_node_info nodeinfo;
    /* wlan_channel */
    wlan_chan chan;
    /* channel width */
    wlan_ch_width chan_width;
    /* ssid and length */
    wlan_ssid ssid;
    /* For generic use, send and get bool flag */
    u_int8_t flag : 1;
    /* For generic use, send MAC address */
    u_int8_t mac[IEEE80211_ADDR_LEN];
    /* Number of spatial streams */
    u_int8_t nss;
    /* Rate in kbps */
    u_int32_t rate;
    /* Channel utilization 0-255 */
    u_int32_t chan_util;
    /* WNM BSS preference */
    u_int8_t bss_pref;
    /* iv_map_version  */
    u_int8_t iv_map_version;
    /* Multicast table */
    wlan_mctbl_data mctbl;
};

enum ieee80211_event_type {
    MLME_EVENT_NONE,
    MLME_EVENT_ASSOC_DISASSOC,
    MLME_EVENT_CBS_STATUS,
    MLME_EVENT_ACS_COMPLETE,
    MLME_EVENT_CAC_STATUS,
    MLME_EVENT_ASSOC_ALLOWANCE_STATUS,
    MLME_EVENT_ALD_WNM_FRAME_RECVD,
    MLME_EVENT_ALD_ANQP_FRAME_RECVD,
    MLME_EVENT_TX_AUTH_FAIL,
    MLME_EVENT_TX_AUTH_ALLOW,
    MLME_EVENT_TX_ASSOC_FAIL,
    MLME_EVENT_CLIENT_ASSOCIATED,
    MLME_EVENT_CLIENT_DISCONNECTED,
    MLME_EVENT_VDEV_STATE,
    MLME_EVENT_INST_RSSI,
    MLME_EVENT_TX_PWR_CHANGE,
    MLME_EVENT_MSCS,
    MLME_EVENT_SCS,
    MLME_EVENT_DSCP_POLICY_QUERY,
    MLME_EVENT_DSCP_POLICY_RESPONSE,
};

enum wlan_vdev_state_ext {
    VDEV_STATE_CREATED,
    VDEV_STATE_DELETED,
    VDEV_STATE_STOPPED,
    VDEV_STATE_RECOVER_CREATE,
    VDEV_STATE_RECOVER_DELETE,
    VDEV_STATE_CONNECTED,
    VDEV_STATE_DISCONNECTED,
    VDEV_STATE_WDS_ENABLED,
    VDEV_STATE_WDS_DISABLED,
};

struct wlan_vdev_state_event {
    enum wlan_vdev_state_ext state;
    uint8_t bssid[IEEE80211_ADDR_LEN];
};

struct wlan_peer_inst_rssi {
    u_int32_t iRSSI;
    u_int8_t valid : 1;
};

/**
 * Metadata about an authentication message that was sent with a failure
 * code due to the client being prohibited by the ACL.
 */
struct wlan_auth_reject_ind {
    u_int8_t client_addr[IEEE80211_ADDR_LEN];
    u_int8_t rssi;
    u_int8_t bs_blocked:1,
             bs_rejected:1;
    u_int32_t reason;
};

/**
 * Metadata about an association response message that was sent
 * with a failure code
 */
struct wlan_assoc_reject_ind {
    u_int8_t client_addr[IEEE80211_ADDR_LEN];
    u_int32_t reason; /* Reason code for authentication rejection */
};

/*
 * EasyMesh R1 SubElements
 */
struct ieee80211_mapie {
#define MAP_OUI_LENGTH 3
    /* IEEE 802.11 vendor specific information element */
    u_int8_t map_apinfo_id;
    /* Length of the following fields in the IE in octets */
    u_int8_t map_apinfo_len;
    /* Wi-Fi Alliance specific OUI */
    u_int8_t map_apinfo_oui[MAP_OUI_LENGTH];
    /* Wi-Fi Alliance specific OUI Type for MAP */
    u_int8_t map_apinfo_type;
    /* Multi-AP Profile extension subelement identifier */
    u_int8_t map_ext_subelement;
    /* Number of Bytes in the subelement value */
    u_int8_t map_ext_len;
    /* MapBSS Type */
    u_int8_t map_ext_value;
#undef MAP_OUI_LENGTH
} __packed;

/* Peer param and commands */
typedef struct wlan_client_he_capabilities {
    u_int8_t  he_cap_macinfo[HECAP_MACINFO_SIZE];
    u_int16_t he_cap_rxmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
    u_int16_t he_cap_txmcsnssmap[HEHANDLE_CAP_TXRX_MCS_NSS_SIZE];
    u_int32_t he_cap_phyinfo[HEHANDLE_CAP_PHYINFO_SIZE];
    u_int32_t he_cap_ppet_numnss_m1;
    u_int32_t he_cap_ppet_ru_mask;
    u_int32_t he_cap_ppet16_ppet8_ru3_ru0[HE_PPET16_PPET8_SIZE];
    u_int32_t he_cap_info_internal;
} wlan_client_he_capabilities;

typedef struct wlan_peer_cap {
    u_int8_t is_BTM_Supported : 1,
	     is_RRM_Supported : 1,
	     is_beacon_meas_supported : 1,
	     band_cap : 3;
    wlan_node_info info;
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    u_int16_t htcap;
    u_int32_t vhtcap;
    wlan_client_he_capabilities hecap;
    u_int8_t rrmcaps[RRM_CAPS_LEN];
} wlan_peer_cap;

typedef enum wlan_mlme_peer_param {
    PEER_SET_KICKOUT,
    PEER_SET_KICKOUT_ALLOW,
    PEER_SET_EXT_STATS,
    PEER_SET_VLAN_ID,
    PEER_REQ_INST_STAT,
    PEER_GET_CAPABILITY,
    PEER_GET_MAX_MCS,
} wlan_mlme_peer_param;

union wlan_mlme_peer_data {
    /* peer capabiliy */
    wlan_peer_cap peercap;
    /* peer max mcs */
    u_int32_t mcs;
    /* generic flag to enable/disable */
    u_int8_t enable : 1;
    /* map ie vlan id */
    u_int16_t vlan_id;
};

typedef enum {
    BSTEER_SOURCE_LOCAL = 0, /* Disconnect initiated by AP */
    BSTEER_SOURCE_REMOTE /* Disconnect initiated by STA */
} BSTEER_DISCONNECT_SOURCE;

typedef enum {
    BSTEER_DISASSOC = 0, /* Disconnect as a result of disassoc */
    BSTEER_DEAUTH /* Disconnect as a result of deauth */
} BSTEER_DISCONNECT_TYPE;

enum {
    ACTION_ASSOC = 0,
    ACTION_DISASSOC,
    ACTION_MAX,
};

/**
 * Metadata about a client disconnect event
 */
struct wlan_disconnect_ind {
    u_int8_t client_addr[IEEE80211_ADDR_LEN];
    BSTEER_DISCONNECT_SOURCE source; /* Who initiated the disconnect */
    BSTEER_DISCONNECT_TYPE type; /* Type - deauth or disassoc */
    u_int32_t reason; /* reason code */
};

/**
 * STA stats per peer
 */
struct wlan_sta_stats_per_peer {
    /* The MAC address of the client */
    u_int8_t client_addr[IEEE80211_ADDR_LEN];
    /* Uplink RSSI */
    u_int8_t rssi;
    /* PER */
    u_int8_t per;
    /* The Tx byte count */
    u_int64_t tx_byte_count;
    /* The Rx byte count */
    u_int64_t rx_byte_count;
    /* The Tx packet count */
    u_int32_t tx_packet_count;
    /* The Rx packet count */
    u_int32_t rx_packet_count;
    /* Number of packets that could be transmitted due to errors */
    u_int32_t tx_error_packets;
    /* Number of transmitted packets which were retransmissions */
    u_int32_t tx_retrans;
    /* Number of packets received from STA that were dropped due to errors */
    u_int32_t rx_error_packets;
    /* The last Tx rate (in Kbps) */
    u_int32_t tx_rate;
    /* ack rssi */
    u_int32_t ack_rssi;
    /* BSSID of the BSS that the STA was associated to */
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    /* Flag to indicate that above stats are for disconnected client */
    u_int8_t is_disassoc_stats;
};

/**
 * Metadata for STA stats
 */
struct wlan_sta_stats_ind {
    /* Number of peers for which stats are provided */
    u_int8_t peer_count;
    /* Stats per peer */
    struct wlan_sta_stats_per_peer peer_stats[BSTEERING_MAX_PEERS_PER_EVENT];
};

/**
 * Metadata for disconnect and STA stats events
 */
struct wlan_client_disconnect_ind {
    struct wlan_disconnect_ind disconnect_event_data;
    struct wlan_sta_stats_ind sta_stats_event_data;
};

struct wlan_assoc_disassoc_event_info {
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
    u_int8_t flag;
    u_int16_t reason;
};

/**
 * @brief The type of the CBS event
 */
typedef enum wlan_cbs_event_type {
    /// CBS has completed
    CBS_COMPLETE,
    /// CBS was cancelled before completion
    CBS_CANCELLED
} wlan_cbs_event_type;


struct wlan_cbs_info {
    u_int32_t cmd;
    u_int8_t name[IFACE_MAX_LEN];
    wlan_cbs_event_type type;
};

struct wlan_assoc_allow_event_info {
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    u_int8_t status;
};

struct wlan_beacon_frm_info {
    u_int8_t *se_sonadv;
    u_int8_t snr;
    u_int8_t nss;
};

/**
 * Metadata about a probe request received from a client that is useful
 * for making band steering decisions.
 */
struct wlan_probe_req_ind {
    /* The MAC address of the client that sent the probe request.*/
    u_int8_t sender_addr[IEEE80211_ADDR_LEN];
    /*  The RSSI of the received probe request.*/
    u_int8_t rssi;
    /* Whether the probe response was blocked or not */
    u_int8_t blocked   : 1,
    /* Whether SSID of incoming probe-request is NULL */
             ssid_null : 1;
};

struct wlan_probe_req_data {
    struct wlan_probe_req_ind probe_req;
    u_int8_t is_chan_2G;
};

struct wlan_rrm_frm_data {
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
    u_int8_t dialog_token;
    u_int8_t num_meas_rpts;
};

struct wlan_act_frm_info {
    struct ieee80211_action *ia;
    u_int32_t ald_info:1;
    union {
        u_int8_t macaddr[IEEE80211_ADDR_LEN];
        struct wlan_rrm_frm_data rrm_data;
    } data;
};

struct wlan_cfg8011_genric_params {
    unsigned int command; /* attribute 17*/
    unsigned int value;   /* attriute 18 */
    void *data;          /* attribute 19 */
    unsigned int data_len;
    unsigned int length; /* attribue 20 */
    unsigned int flags;  /* attribute 21 */
    u_int8_t link_id;
};

#if QCA_SUPPORT_SON
/**
 * Metadata and defines for QoS descriptors:
 * MSCS,SCS and DSCP policy
 * to send to SON application.
 */
#define QOS_DESCRIPTOR_LEN 1024
#define DSCP_POLICY_RESP_LEN 11
#define DSCP_POLICY_QUERY_LEN 7

#if WLAN_SUPPORT_MSCS
struct wlan_mscs_frm_info {
    u_int16_t qmid;
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    struct ieee80211_mscs_descriptor mscs_descriptor;
};
#endif /* WLAN_SUPPORT_MSCS */

#ifdef WLAN_SUPPORT_SCS
struct wlan_scs_frm_info {
    u_int16_t qmid;
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    u_int8_t ie_len;
    u_int8_t scs_descriptor[QOS_DESCRIPTOR_LEN];
};
#endif /* WLAN_SUPPORT_SCS */

#if WLAN_SUPPORT_DAP
struct wlan_policy_query_info {
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
    u_int8_t frame_len;
    u_int8_t dscp_policy_query_frame[QOS_DESCRIPTOR_LEN];
};

struct wlan_dscp_policy_resp_info {
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
    u_int8_t frame_len;
    u_int8_t dscp_policy_resp_frame[QOS_DESCRIPTOR_LEN];
};
#endif /* WLAN_SUPPORT_DAP */
#endif /* QCA_SUPPORT_SON */

struct wlan_cfg8011_vendor_params {
    /* attribute 62 */
    union {
        unsigned int ccfs;
        unsigned int mode_chan_command_type;
    };
    unsigned int puncture_bitmap; /* attribute 63 */
};

#if defined(WLAN_DISP_CHAN_INFO)
/**
 * show_dfs_state() - Convert a given state into a string.
 * @state: DFS channel state
 *
 * Return: An equivalent string
 */
static inline const char *show_dfs_state(enum wlan_channel_dfs_state state)
{
    switch (state) {
        case WLAN_CH_DFS_S_CAC_REQ:
            return "DFS_CAC_REQUIRED";
        case WLAN_CH_DFS_S_CAC_STARTED:
            return "DFS_CAC_STARTED";
        case WLAN_CH_DFS_S_CAC_COMPLETED:
            return "DFS_CAC_COMPLETED";
        case WLAN_CH_DFS_S_NOL:
            return "DFS_NOL";
        case WLAN_CH_DFS_S_PRECAC_STARTED:
            return "DFS_PRECAC_STARTED";
        case WLAN_CH_DFS_S_PRECAC_COMPLETED:
            return "DFS_PRECAC_COMPLETED";
        case WLAN_CH_DFS_S_NON_DFS:
            return "NON_DFS_CHANNEL";
        default:
            return "DFS_INVALID";
            break;
    }
}

/**
 * convert_freq_to_index() - Encode the input frequency to an index.
 * @freq: Frequency in MHz
 * @index: Pointer to the index to be filled
 *
 * Return: None
 */
static inline void
convert_freq_to_index(uint16_t freq, int8_t *index)
{
    uint8_t chan_num = (freq - WLAN_5_GHZ_BASE_FREQ) / WLAN_CHAN_SPACING_5MHZ;
    int8_t tmp_ind = (chan_num - FIRST_DFS_CHAN_NUM) / CHAN_NUM_SPACING;
    *index = ((tmp_ind >= 0) && (tmp_ind < NUM_DFS_CHANS)) ?
              tmp_ind : INVALID_INDEX;
}
#endif /*WLAN_DISP_CHAN_INFO*/

#endif

