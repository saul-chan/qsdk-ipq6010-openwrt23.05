/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */


#ifndef __WLANIF_CMN_H__
#define __WLANIF_CMN_H__

#include <sys/socket.h>
#include <netinet/in.h>


#define ETH_LEN 6




#define SP_RULE_FLAG_MATCH_ALWAYS_TRUE          0x01            /* Rule match always true. (skip field matching) mask */
#define SP_RULE_FLAG_MATCH_PRECEDENCE           0x04            /* Match precedence mask*/
#define SP_RULE_FLAG_MATCH_RULE_OUTPUT          0x08            /* Match rule output mask*/
#define SP_RULE_FLAG_MATCH_USER_PRIORITY        0x10            /* Match user priority mask*/
#define SP_RULE_FLAG_MATCH_SRC_MAC              0x20            /* Match source mac address mask*/
#define SP_RULE_FLAG_MATCH_DST_MAC              0x40            /* Match Destination mac address mask*/
#define SP_RULE_FLAG_MATCH_SRC_IPV4             0x80            /* Match source ipv4 address match mask */
#define SP_RULE_FLAG_MATCH_DST_IPV4             0x100           /* Match destination ipv4 address  mask */
#define SP_RULE_FLAG_MATCH_SRC_IPV6             0x200           /* Match source ipv6 address match mask */
#define SP_RULE_FLAG_MATCH_DST_IPV6             0x400           /* Match Destination ipv6 address match mask */
#define SP_RULE_FLAG_MATCH_SRC_PORT             0x800           /* Match source port mask */
#define SP_RULE_FLAG_MATCH_DST_PORT             0x1000          /* Match destination port mask */
#define SP_RULE_FLAG_MATCH_PROTO_NUM            0x2000          /* Match protocol number mask */
#define SP_RULE_FLAG_MATCH_VLAN_ID              0x4000          /* Match VLAN ID mask */
#define SP_RULE_FLAG_MATCH_DSCP                 0x8000          /* Match DSCP mask */
#define SP_RULE_FLAG_MATCH_VLAN_PCP             0x10000         /* Match VLAN pcp mask */
#define SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID     0x20000         /* Match Service class ID */
#define SP_RULE_FLAG_MATCH_SRC_IPV4_MASK	0x40000		/* Match Source ipv4 address mask */
#define SP_RULE_FLAG_MATCH_DST_IPV4_MASK	0x80000		/* Match Destination ipv4 address mask */
#define SP_RULE_FLAG_MATCH_SRC_IPV6_MASK	0x100000	/* Match Source ipv6 address mask */
#define SP_RULE_FLAG_MATCH_DST_IPV6_MASK	0x200000	/* Match Destination ipv6 address mask	*/
#define SP_RULE_FLAG_MATCH_DSCP_REMARK          0x400000        /* Match DSCP remark mask */
#define SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK      0x800000        /* Match VLAN pcp remark mask */
#define SP_RULE_FLAG_MATCH_IP_VERSION_TYPE	0x1000000	/* Match IP Version type */
#define SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE	0x2000000	/* Match Classifier type */
#define SP_RULE_FLAG_MATCH_SPI			0x4000000	/* Match SPI for TCLASS10 */
#define SP_RULE_FLAG_MATCH_IFINDEX		0x8000000	/* Match Interface Index */
#define SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_START	0x10000000	/* Match Source port range start */
#define SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_END	0x20000000	/* Match Source port range end */
#define SP_RULE_FLAG_MATCH_DST_PORT_RANGE_START	0x40000000	/* Match Destination port range start */
#define SP_RULE_FLAG_MATCH_DST_PORT_RANGE_END	0x80000000	/* Match Destination port range end */

#define	SP_RULE_FLAG_MATCH_UP			0x02		/* Match up in 802.11 qos control mask */
#define	SP_RULE_FLAG_MATCH_UP_SENSE		0x04 		/* UP in 802.11 qos control match sense mask */
#define	SP_RULE_FLAG_MATCH_SOURCE_MAC_SENSE	0x10		/* Match source mac address mask */
#define	SP_RULE_FLAG_MATCH_DST_MAC_SENSE	0x40		/* Destination mac address match sense mask */
#define	SP_RULE_FLAG_MATCH_SRC_IPV4_SENSE	0x100		/* Source ipv4 address sense mask */
#define	SP_RULE_FLAG_MATCH_SRC_IPV6_SENSE	0x400		/* Source ipv6 address sense mask */
#define	SP_RULE_FLAG_MATCH_DST_IPV4_SENSE	0x1000		/* Destination ipv4 address sense mask */
#define	SP_RULE_FLAG_MATCH_DST_IPV6_SENSE	0x4000		/* Destination ipv6 address sense mask */
#define	SP_RULE_FLAG_MATCH_SRC_PORT_SENSE	0x10000		/* Source port sense mask */
#define	SP_RULE_FLAG_MATCH_DST_PORT_SENSE	0x40000		/* Destination port sense mask */
#define	SP_RULE_FLAG_MATCH_PROTOCOL_SENSE	0x100000	/* Protocol sense mask */
#define	SP_RULE_FLAG_MATCH_VLAN_ID_SENSE	0x400000	/* VLAN id sense mask */
#define	SP_RULE_FLAG_MATCH_DSCP_SENSE		0x1000000	/* DSCP sense mask */


/*
 * Attributes added for EMESH classifier.
 */
#define SP_RULE_FLAG_EXT_MATCH_AE_TYPE          0x01            /* Match ae type extended flags */
#define SP_RULE_FLAG_EXT_MATCH_SRC_IFACE        0x02            /* Match src interface extended flags */
#define SP_RULE_FLAG_EXT_MATCH_DST_IFACE        0x04            /* Match dst interface extended flags */
#define SP_RULE_FLAG_EXT_BURST_SIZE_DL		0x08		/* Match Burst Size in DL Direction */
#define SP_RULE_FLAG_EXT_BURST_SIZE_UL		0x10		/* Match Burst Size in UL Direction */
#define SP_RULE_FLAG_EXT_MSCS_TID_BITMAP	0x20		/* Match MSCS TID Bitmap Direction */
#define SP_RULE_FLAG_EXT_MESH_FLAGS_IN		0x40		/* Match MESH Classifier Type flags */
#define SP_RULE_FLAG_EXT_SVC_INTERVAL_DL	0x80		/* Match Service interval in DL Direction */
#define SP_RULE_FLAG_EXT_SVC_INTERVAL_UL	0x100		/* Match Service interval in UL Direction */

/*
 * sp_rule_classifier_type
 * 	This enum defines rule classifier type
 */
enum sp_rule_classifier_type {
	SP_RULE_TYPE_SAWF_INVALID,	/* For invalid type */
	SP_RULE_TYPE_MESH,	/* For non-sawf rule */
	SP_RULE_TYPE_SAWF,	/* For SAWF rule */
	SP_RULE_TYPE_SCS,	/* For SCS rule */
	SP_RULE_TYPE_MSCS,	/* For MSCS rule */
	SP_RULE_TYPE_SAWF_SCS,	/* For SAWF-SCS rule type */
	SP_RULE_TYPE_SAWF_IFLI,	/* For IFLI rule type */
	SP_RULE_TYPE_SAWF_MAX,	/* Max SP rule type */
};

enum {
    SPM_CMD_RULE_UNSPEC,
    /* Must NOT use element 0 */
    SPM_CMD_RULE_ACTION,
    SPM_CMD_RULE_QUERY,
    SPM_CMD_RULE_FLUSH,
    SPM_CMD_MAX,
};

enum sp_gnl_attr {
    SP_GNL_ATTR_UNSPEC,
	SP_GNL_ATTR_ID,
	SP_GNL_ATTR_ADD_DELETE_RULE,
	SP_GNL_ATTR_RULE_PRECEDENCE,
	SP_GNL_ATTR_RULE_OUTPUT,
	SP_GNL_ATTR_USER_PRIORITY,
	SP_GNL_ATTR_SRC_MAC,
	SP_GNL_ATTR_DST_MAC,
	SP_GNL_ATTR_SRC_IPV4_ADDR,
	SP_GNL_ATTR_SRC_IPV4_ADDR_MASK,
	SP_GNL_ATTR_DST_IPV4_ADDR,
	SP_GNL_ATTR_DST_IPV4_ADDR_MASK,
	SP_GNL_ATTR_SRC_IPV6_ADDR,
	SP_GNL_ATTR_SRC_IPV6_ADDR_MASK,
	SP_GNL_ATTR_DST_IPV6_ADDR,
	SP_GNL_ATTR_DST_IPV6_ADDR_MASK,
	SP_GNL_ATTR_SRC_PORT,
	SP_GNL_ATTR_DST_PORT,
	SP_GNL_ATTR_PROTOCOL_NUMBER,
	SP_GNL_ATTR_VLAN_ID,
	SP_GNL_ATTR_DSCP,
	SP_GNL_ATTR_DSCP_REMARK,
	SP_GNL_ATTR_VLAN_PCP,
	SP_GNL_ATTR_VLAN_PCP_REMARK,
	SP_GNL_ATTR_SERVICE_CLASS_ID,
	SP_GNL_ATTR_IP_VERSION_TYPE,
	SP_GNL_ATTR_CLASSIFIER_TYPE,
	SP_GNL_ATTR_FILTER_VALUE,
	SP_GNL_ATTR_FILTER_MASK,
	SP_GNL_ATTR_TID_BITMAP,
	SP_GNL_ATTR_PRIORITY_LIMIT,
	SP_GNL_ATTR_IFINDEX,
	SP_GNL_ATTR_SRC_PORT_RANGE_START,
	SP_GNL_ATTR_SRC_PORT_RANGE_END,
	SP_GNL_ATTR_DST_PORT_RANGE_START,
	SP_GNL_ATTR_DST_PORT_RANGE_END,
	SP_GNL_ATTR_AE_TYPE,
	SP_GNL_ATTR_SRC_IFACE,
	SP_GNL_ATTR_DST_IFACE,
	SP_GNL_ATTR_SVC_INTERVAL_DL,
	SP_GNL_ATTR_SVC_INTERVAL_UL,
	SP_GNL_ATTR_BURST_SIZE_DL,
	SP_GNL_ATTR_BURST_SIZE_UL,
	SP_GNL_ATTR_SENSE_MESH_FLAG_IN,
	SP_GNL_ATTR_MAX,
};

typedef struct sp_rule {
    /*
     *flag to specify which rule parameters are present
     */
    uint32_t flags;

    /*
     *flags extended to specify which rule parameters are present
     */
    uint32_t flags_ext;
    uint32_t attr_id;
    /*
     *Rule operation to be performed
     */
    uint8_t add_del_rule;
    uint8_t precedence;
    /*
     * The value of rule_output determines how to set the pcp value
     * to be marked in the matched packet.
     */
    uint8_t rule_output;
    uint8_t user_priority;
    /*
     * Source mac address
     */
    u_int8_t src_mac[ETH_LEN];
    /*
     * Destination mac address
     */
    u_int8_t dst_mac[ETH_LEN];
    /*
     * Source ipv4 address
     */
    uint32_t src_ipv4_addr;
    /*
     * Destination ipv4 address
     */
    uint32_t dst_ipv4_addr;
    /*
     * Destination ipv6 address
     */
    struct in6_addr src_ipv6_addr;
    /*
     * Destination ipv6 address
     */
    struct in6_addr dst_ipv6_addr;
    /*
     * Source port number
     */
    uint16_t src_port;
    /*
     * Destination port number
     */
    uint16_t dst_port;
    /*
     * Protocol number
     */
    uint8_t proto_num;
    /*
     * VLAN id
     */
    uint16_t vlan_id;
    /*
     * DSCP value
     */
    uint8_t dscp;
    /*
     * DSCP remark
     */
    uint8_t dscp_remark;
    /*
     * Vlan Priority
     */
    uint8_t vlan_pcp;
    /*
     * Vlan Priority remark
     */
    uint8_t vlan_pcp_remark;
    /*
     * Service class id
     */
    uint8_t service_class_id;
    /*
     * Source ipv4 address mask
     */
    uint32_t src_ipv4_addr_mask;
    /*
     * Destination ipv4 address mask
     */
    uint32_t dst_ipv4_addr_mask;
    /*
     * Filter value
     */
    uint32_t filter_value;
    /*
     * Filter mask
     */
    uint32_t filter_mask;
    /*
     * Source ipv6 address mask
     */
    struct in6_addr src_ipv6_addr_mask;
    /*
     * Destination ipv6 address mask
     */
    struct in6_addr dst_ipv6_addr_mask;
    /*
     * IP Version type
     */
    uint8_t ip_version_type;
    /*
     * Classifier type
     */
    uint8_t classifier_type;

    /* internal field to keep track if qsp rule is valid or not */
    u_int8_t valid_qsp;
    /*
     * Service interval
     * Specified msec
     * This is min latency expectation and is used
     * by Wi-Fi FW for peer tid queue scheduling
     */
    u_int8_t serviceIntervalDl;
    u_int8_t serviceIntervalUl;
    /*
    * Burst size
    * Specified in bytes
    * This is used by Wi-Fi FW for peer tid queue
    * scheduling
    */
    uint32_t burstSizeDl;
    uint32_t burstSizeUl;
    /*
     * MSCS tid  Bitmap
     */
    u_int8_t mscs_tid_bitmap;
    /*
     * Flags set by the MESH Classifier Type.
     */
    uint32_t mesh_flags_in;
}
sp_rule;

/*
 * sal_rule_socket
 * Structure containing necessary information to communicate with sal rule.
 */
struct sal_rule_socket {
    struct nl_sock * sk;
    struct sp_rule rule;
    int family_id;
};


struct wlanif_config {
    void *ctx;
    uint32_t IsCfg80211;
    int pvt_cmd_sock_id;
    int pvt_event_sock_id;
};

#if !SONLIB_SUPPORT_ENABLED
/// The number of bytes in a MAC address
#define IEEE80211_ADDR_LEN 6

/// The max SSID length in bytes
#define IEEE80211_NWID_LEN  32

#ifndef ACS_RANK_DESC_DBG_LEN
/// The max length in bytes for the ACS channel ranking description
#define ACS_RANK_DESC_DBG_LEN 80
#endif  // ACS_RANK_DESC_DBG_LEN

#ifndef HOST_MAX_CHAINS
#define HOST_MAX_CHAINS 8
#endif

// This struct has to be a duplicate definition of ieee80211_neighbor_info
typedef struct ieee80211_neighbor_info_t {
    u_int32_t   phymode; /* ap channel width*/
    int32_t     rssi; /* ap singal strength */
    u_int8_t    bssid[IEEE80211_ADDR_LEN]; /* BSSID information */
    u_int8_t    ssid_len; /* length of the ssid */
    u_int8_t    ssid[IEEE80211_NWID_LEN + 1]; /* SSID details */
    u_int8_t    qbssload_ie_valid; /* Flag to indicate if qbss load ie is present */
    u_int8_t    station_count; /* number of station associated */
    u_int8_t    channel_utilization; /* channel busy time in 0-255 scale */
    int8_t      power_info;                   /* Indoor or Standard power info */
} ieee80211_neighbor_info_t;

// This struct has to be a duplicate definition of ACS_LIST_TYPE
typedef enum {
    acsChanStats,
    acsNeighGetListCount,
    acsNeighGetList,
    acsChanNfStats,
    acsChanWeightStats,
    acsPwrStats
} acs_list_type_e;

// This struct has to be a duplicate definition of wlan_band_id
enum wlan_band_id_e {
    wlanBand_UnSpecified = 0,
    wlanBand_2GHZ = 1,
    wlanBand_5GHZ = 2,
    wlanBand_6GHZ = 3,
    /* Add new band definitions here */
    wlanBand_MAX,
};

typedef struct acs_rank_t {
    u_int32_t rank;
    char desc[ACS_RANK_DESC_DBG_LEN];
} acs_rank_t;


// This struct has to be a duplicate definition of ieee80211_acs_dbg
typedef struct ieee80211_acs_report_t {
    u_int8_t  nchans;
    u_int8_t  entry_id;
    u_int16_t chan_freq;
    enum wlan_band_id_e chan_band;
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
    ieee80211_neighbor_info_t *neighbor_list;
    u_int32_t chan_80211_b_duration;
    acs_rank_t acs_rank;
    u_int8_t acs_status;
    acs_list_type_e acs_type;
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
    acs_rank_t acs_rank_overlap;     /* ACS Rank and channel reject code for overlap channel */
    int32_t power_overlap;
#endif
} ieee80211_acs_report_t;

#define IEEE80211_RRM_CHAN_MAX 255
#define IEEE80211_MAX_REQ_IE 255

#define IEEE80211_BCNREQUEST_VALIDSSID_REQUESTED 0x01
#define IEEE80211_BCNREQUEST_NULLSSID_REQUESTED 0x02

#define IEEE80211_RRM_NUM_CHANREQ_MAX 16
#define IEEE80211_RRM_NUM_CHANREP_MAX 2

typedef struct wlanifBSteerEventsPriv_t *wlanifBSteerEventsHandle_t;

#endif /*SONLIB_SUPPORT_ENABLED*/

typedef struct afc_freq_obj_t {
    u_int16_t low_freq;
    u_int16_t high_freq;
    u_int32_t max_psd;
} afc_freq_obj_t;

typedef struct chan_eirp_obj_t {
    u_int8_t cfi;
    u_int32_t eirp;
} chan_eirp_obj_t;

typedef struct afc_chan_obj_t {
    u_int8_t global_opclass;
    u_int8_t num_chans;
    chan_eirp_obj_t *chan_eirp_info;
} afc_chan_obj_t;

typedef struct ieee80211_afc_response_t {
    u_int32_t resp_id;
    u_int8_t fw_status_code;
    u_int32_t server_resp_code;
    u_int32_t afc_wfa_version;
    u_int32_t afc_expiry_date;
    u_int32_t afc_expiry_time;
    u_int8_t num_freq_objs;
    u_int8_t num_chan_objs;
} ieee80211_afc_response_t;

typedef struct ieee80211_afc_expiry_t {
    uint64_t req_id;
} ieee80211_afc_expiry_t;

typedef void (*atf_stats_cb)(void *buffer, int length);

/* Netlink socket ports for different applications to bind to
 * These ports are reserved for these specific applications and
 * should take care not to reuse it.
 */
#define LBD_NL80211_CMD_SOCK      899
#define LBD_NL80211_EVENT_SOCK    900
#define WSPLCD_NL80211_CMD_SOCK   901
#define WSPLCD_NL80211_EVENT_SOCK 902
#define HYD_NL80211_CMD_SOCK      903
#define HYD_NL80211_EVENT_SOCK    904
#define IFACEMGR_NL80211_CMD_SOCK      950
#define IFACEMGR_NL80211_EVENT_SOCK    951
#define LIBSTORAGE_NL80211_CMD_SOCK    952
#define LIBSTORAGE_NL80211_EVENT_SOCK  953


int wlanif_cfg80211_init(struct wlanif_config *cfg80211_conf);
void wlanif_cfg80211_deinit(struct wlanif_config *cfg80211_conf);
int getName_cfg80211(void *,const char *ifname, char *name );
int getMLDAddr_cfg80211(void *ctx, const char * ifname, struct ether_addr *mldAddr );
int isAP_cfg80211(void *, const char * ifname, uint32_t *result);
int getBSSID_cfg80211(void *, const char *ifname, struct ether_addr *BSSID );
int getESSID_cfg80211(void *ctx, const char * ifname, void *buf, uint32_t *len );
int getFreq_cfg80211(void *, const char * ifname, int32_t * freq);
int getChannelWidth_cfg80211(void *, const char *ifname, int * chwidth);
int getChannelExtOffset_cfg80211(void *, const char *ifname, int * choffset);
int getChannelBandwidth_cfg80211(void *, const char *ifname, int * bandwidth);
int getAcsState_cfg80211(void *, const char *ifname, int * acsstate);
int getCacState_cfg80211(void *, const char *ifname, int * cacstate);
int getParentIfindex_cfg80211(void *, const char *ifname, int * cacstate);
int getSmartMonitor_cfg80211(void *, const char *ifname, int * smartmonitor);
int getGenericInfoAtf_cfg80211(void *, const char *ifname, int cmd, void * chanInfo, int chanInfoSize);
int getGenericInfoAld_cfg80211(void *, const char *ifname, void * chanInfo, int chanInfoSize);
int getGenericInfoHmwds_cfg80211(void *, const char *ifname, void * chanInfo, int chanInfoSize);
int getGenericInfoAstRemove_cfg80211(void *, const char *ifname, void * chanInfo, int chanInfoSize);
int getGenericNac_cfg80211(void *, const char *ifname, void * config, int configSize);
int getCfreq2_cfg80211(void *, const char * ifname, int32_t * cfreq2);
int getChUtil_cfg80211(void *ctx, const char * ifname, int32_t * chUtil);
int getChannelInfo_cfg80211(void *, const char *ifname, void * chanInfo, int chanInfoSize);
int getChannelInfo160_cfg80211(void *, const char *ifname, void * chanInfo, int chanInfoSize);
int getStationInfo_cfg80211(void *, const char *ifname, void **buf, int* len);
int getDbgreq_cfg80211(void * , const char *ifname, void *data , uint32_t data_len);
int getMeshreq_cfg80211(void * , const char *ifname, void *data , uint32_t data_len);
int getExtended_cfg80211(void * , const char *ifname, void *data , uint32_t data_len);
int addDelKickMAC_cfg80211(void *, const char *ifname, int operation, void *data, uint32_t len);
int setFilter_cfg80211(void *, const char *ifname, void *data, uint32_t len);
int getWirelessMode_cfg80211(void * ctx , const char *ifname, void *data, uint32_t len);
int sendMgmt_cfg80211(void *, const char *ifname, void *data, uint32_t len);
int setParamMaccmd_cfg80211(void *, const char *ifname, void *data, uint32_t len);
int setMapVapBeacon_cfg80211(void *ctx, const char *ifname, void *data, uint32_t len);
int setParam_cfg80211(void *, const char *ifname,int cmd, void *data, uint32_t len);
int getStaStats_cfg80211(void *, const char *ifname, void *data, uint32_t len);
int getRange_cfg80211(void *ctx, const char *ifname, int *we_version);
int getStaCount_cfg80211(void *ctx, const char *ifname, int32_t *result);
int setIntfMode_cfg80211(void *ctx, const char *ifname, const char *mode, u_int8_t len);
int setParamVapInd_cfg80211(void *ctx, const char *ifname, void *data, uint32_t len);
int setFreq_cfg80211(void *ctx, const char *ifname, int freq, int band, int pp, int cntrChanFreq);
int setCSA_cfg80211(void *ctx, const char * ifname, int freq, int band, int chwidth, uint8_t cntrChan, int pp);

int setRRM_cfg80211(void *ctx, const char * ifname, u_int8_t value);
int setWds_cfg80211(void *ctx, const char * ifname, int value);
int set6GsecComp_cfg80211(void *ctx, const char * ifname, int value);
int getCountryCode_cfg80211(void *ctx, const char *ifname, size_t size, char *countryCode);
int setCACTimeout_cfg80211(void *ctx, const char * ifname, int cac_timeout);
int setCFreq2_cfg80211(void *ctx, const char * ifname, int chan_num);
int getACSReport_cfg80211(void *ctx, const char *ifName, u_int8_t* numChans,
                          ieee80211_acs_report_t *chanData, u_int8_t* numNeighbors,
                          ieee80211_neighbor_info_t *neighborData, u_int8_t neighborChans[]);
int setAcsChanList_cfg80211(void *ctx, const char *ifName, u_int8_t numChans, u_int8_t *channels, int band);
int getMapBssRole_cfg80211(void *ctx, const char *ifName, u_int8_t *mapBssRole);
int getFallbackFreq_cfg80211(void *ctx, const char * ifname, int * fallbackFreq);
int setFallbackFreq_cfg80211(void *ctx, const char * ifname, int fallbackFreq);
int setNOLChannel_cfg80211(void *ctx, const char * radioName, int32_t freq_offset, int32_t bang_radar_type);
int getNOLChannel_cfg80211(void *ctx, const char * radioName, void* nolinfo);
int setRRMFilter_cfg80211(void *ctx, const char* ifname, u_int8_t rrm_filter);
int setWNMFilter_cfg80211(void *ctx, const char* ifname, u_int8_t wnm_filter);
int getNoiseFloor_cfg80211(void *ctx, const char *ifname, int16_t *noise);
int getFullRadioMetrics_cfg80211(void *ctx, const char * ifname, u_int32_t *allMetrics);
int getTxChanUtil_cfg80211(void *ctx, const char *ifname, u_int8_t *txUtil);
int getRxSelfChanUtil_cfg80211(void *ctx, const char *ifname, u_int8_t *rxSelf);
int getRxObssChanUtil_cfg80211(void *ctx, const char *ifname, u_int8_t *rxObss);
int getApPwrType_cfg80211(void *ctx, const char *ifname, int *apPwrType);
int setApPwrType_cfg80211(void *ctx, const char * ifname, int apPwrType);
int getIfaceStats_cfg80211(void * ctx , const char *ifname, void *data , uint32_t data_len);
int getChanListAFC_cfg80211(void *ctx, const char* ifname, void *afcChanList);
int setAfcTimerReset_cfg80211(void * ctx , const char *ifname);

int setGenericSAWF_cfg80211(void *ctx, const char * ifname,void * config, int configSize);

int setHlosTidOvrd_cfg80211(void *ctx, const char * ifname, int enable);
int setDscpOvrd_cfg80211(void *ctx, const char * ifname, int enable);
int setDscpTidMap_cfg80211(void *ctx, const char * ifname, int tos, int tid);
int setDscpPolicy_cfg80211(void *ctx, const char * ifname, u_int8_t attrId, u_int8_t policyId,
                           u_int8_t reqType, u_int8_t dscp, u_int8_t srcIp[], u_int8_t dstIp[],
                           u_int16_t srcPort, u_int16_t dstPort, u_int8_t protocolNum);
int sendDscpPolicy_cfg80211(void *ctx, const char * ifname, struct ether_addr *staMac,
                            u_int8_t policyId, u_int8_t reset);
int addDelInterfaceToMLDLink_cfg80211(void *ctx, const char *ifname, const struct ether_addr *mld_addr, int addDelValue);
int setT2lmMapConfig_cfg80211(void *ctx, const char * ifname,void * config, int configSize, int linkId);
int setT2lmMapConfigPerPeer_cfg80211(void *ctx, const char * ifname,void * config, int configSize, int linkId);

int setBSSColor_cfg80211(void *ctx, const char *ifname, uint8_t color, int override);
int setEnableHeSigaSR15_cfg80211(void *ctx, const char * ifname, int enable);
int setSRGValid_cfg80211(void *ctx, const char * ifname, int valid, int minOffset, int maxOffset);
int setPSREnabled_cfg80211(void *ctx, const char * ifname, int enable);
int setNonSRGValid_cfg80211(void *ctx, const char * ifname, int valid, int maxOffset);
int setBSSColorBmp_cfg80211(void *ctx, const char * ifname, u_int32_t bitmap0, u_int32_t bitmap1);
int setPartialBSSIDBmp_cfg80211(void *ctx, const char * ifname, u_int32_t bitmap0, u_int32_t bitmap1);
int setColorCollisionApPeriod_cfg80211(void *ctx, const char * ifname, int period);

int getBSSColor_cfg80211(void *ctx, const char *ifname, char* bssColor);
int getEnableHeSigaSR15_cfg80211(void *ctx, const char * ifname, uint32_t * enable);
int getSRGValid_cfg80211(void *ctx, const char * ifname, char* srgInfoValid);
int getPSREnabled_cfg80211(void *ctx, const char * ifname, uint32_t *enable);
int getNonSRGValid_cfg80211(void *ctx, const char * ifname, char* nonSrgInfo);
int getBSSColorBmp_cfg80211(void *ctx, const char * ifname, char* bitmap);
int getPartialBSSIDBmp_cfg80211(void *ctx, const char * ifname, char* bitmap);
int getNeighborColorBmp_cfg80211(void *ctx, const char * ifname, char* bitmap);

/**
 * @brief Get the BandInfo from the Wi-Fi driver for the given radio interface.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: radio interface name
 * @param     [out] band_info: BandInfo for given radio interface name.
 *                  band_info details are
 *                       1 indicates the RADIO_IN_HIGH_BAND
 *                       2 indicates the RADIO_IN_FULL_BAND
 *                       3 indicates the RADIO_IN_LOW_BAND
 *                       4 indicates the RADIO_IS_NON_5G ie 2G BAND
 * @return    Success:0, Failure: -1
 */
int getBandInfo_cfg80211(void *ctx, const char * ifname, uint8_t * band_info);

/**
 * @brief Get the uplink rate from the Wi-Fi driver for the given VAP interface.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [out] ul_rate: uplink rate for given vap interface name.
 * @return    Success:0, Failure: -1
 */
int getUplinkRate_cfg80211(void *ctx, const char * ifname, uint16_t * ul_rate);

/**
 * @brief Get the num Active vdev count on a Radio from the Wi-Fi driver for
 *        the given VAP interface.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [out] vdev_count : num of Active vdev on a radio/band
 * @return    Success:0, Failure: -1
 */
int getActiveVdevCount_cfg80211(void *ctx, const char *ifname, int *vdev_count);

/**
 * @brief Set the uplink rate to the Wi-Fi driver for the given VAP interface.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [in] ul_rate: set uplink rate
 * @return    Success:0, Failure: -1
 */
int setUplinkRate_cfg80211(void * ctx, const char *ifname, uint16_t ul_rate);

/**
 * @brief Set the Map BSS Type to the Wi-Fi driver for the given VAP interface.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [in] mapBssType: bss type indicating its FH, BH, STA, SM or Teardown
 * @return    Success:0, Failure: -1
**/
int setMapBssType_cfg80211(void * ctx, const char *ifname, uint8_t mapBssType);

/**
 * @brief Set the Map version to the Wi-Fi driver.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [in] map: map version number
 * @return    Success:0, Failure: -1
**/
int setMapVersion_cfg80211(void * ctx, const char *ifname, uint8_t map);

/**
 * @brief Set the backhaul type whenever there is a change
 * in the backhaul connection between WiFi and Ethernet vice versa.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [in] bh_type: backhaul type.
 *                 backhaul type : WiFiBh is 1 and EthBh is 2
 * @return    Success:0, Failure: -1
 */
int setSonBhType_cfg80211(void * ctx, const char *ifname, uint8_t bh_type);

int setParam_mesh_cfg80211(void *ctx, const char *ifname, int cmd, void *data, uint32_t len);
/**
 * @brief Set Param SON is set on a VAP
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [in] len length of the data
 * @return    Success:0, Failure: -1
 */
int setParamSon_cfg80211(void * ctx, const char *ifname, uint8_t paramSon);

/**
 * @brief Function to get phystats of radio
 *
 * @param [in] ctx   opaque pointer to private vendor struct
 * @param [in] ifname   interface name
 * @param [in] data pointer to ol_stats structure
 * @param [in] len length of the data
 *
 * @return Success: 0, Failure: -1
 */
int getPhyStats_cfg80211(void * ctx , const char *ifname, void *data, uint32_t len);
struct iw_priv_args *getPrivArgs_cfg80211(void *ctxt, const char *ifname, size_t *len);

/**
 * @brief: get_nl80211_event_msg: Function to parse the cfg event data
 * @msg: Pointer to netlink event message
 * @outbuf: out put, pointer on whcih parsed data is stored
 * @return Success: 0
 */
int get_nl80211_event_msg(u_int8_t *msg, void * ctx, void *outbuf);

/**
 * @brief: get_nl80211_event_msg: Function to parse the AFC event data
 * @msg: Pointer to netlink event message
 * @outbuf: out put, pointer on whcih parsed data is stored
 * @return Success: 0
 */
int parse_afc_cfg80211_event_msg(u_int8_t *msg, void * ctx, void *outbuf);
int get_cfg80211_event_sock(void * ctx);

/**
 * @brief Function to Get the version compatibility from the Wi-Fi driver for the given VAP interface.
 *
 * @param     [in] context: opaque pointer.
 * @param     [in] ifname: vap interface name
 * @param     [in] version_info: pass the version version information to be sent to Wi-Fi driver
 * @param     [in] len length of the data
 * @return    Pass: 0, Fail: 1, Error: -1
 */
int getVersionCompatibility_cfg80211(void *ctx, const char * ifname, void *version_info, int data_len);

/*enum to handle MAC operations*/
enum wlanif_ioops_t
{
    IO_OPERATION_ADDMAC=0,
    IO_OPERATION_DELMAC,
    IO_OPERATION_KICKMAC,
    IO_OPERATION_LOCAL_DISASSOC,
    IO_OPERATION_ADDMAC_VALIDITY_TIMER,
    IO_OPERATION_DEAUTH,
};


/**
 * @brief Get whether the Radio is tuned for low, high, full band or 2g.
 * the equivalent enum declared in ol_if_athvar.h and one-to-one mapping.
*/
typedef enum wlanifBandInfo_e {
    bandInfo_Unknown, /* unable to retrieve band info due to some error */
    bandInfo_High, /* supports channel starts from 100 to 165 */
    bandInfo_Full, /* supports channel starts from 36 to 165 */
    bandInfo_Low, /* supports channel starts from 36 to 64 */
    bandInfo_Non5G, /* supports 2g channel 1 to 14 */
    bandInfo_6G,
    bandInfo_5G_6G, /* RADIO_IS_EITHER_5G_6G */
    bandInfo_6G_Low, /* supports channel start from 1 to 113 */
    bandInfo_6G_High /* supports channel start from 117 to 233 */
} wlanifBandInfo_e;

/**
 * SON WiFi backhaul type is defined 1 and
 * Eth backhaul type is defined 2, PLC is defined as 3 respectively.
 */
typedef enum backhaulType_e {
    backhaulType_Unknown = 0,
    backhaulType_Wifi = 1,
    backhaulType_Ether = 2,
    bacckhaulType_Plc = 3,
} backhaulType_e;

extern struct wlanif_config * wlanif_config_init(int pvt_cmd_sock_id,
                                                 int pvt_event_sock_id);
extern void wlanif_config_deinit(struct wlanif_config *);

extern struct sal_rule_socket * sal_rule_init(void);
int sal_rule_config(struct sal_rule_socket *rs);
/*
* sal_rule_flush()
* API used to flush rules from spm database based on classifier_type
* only classifier type is required
*/
int sal_rule_flush(struct sal_rule_socket *rs, uint8_t classifier_type);
int sal_rule_deinit(struct sal_rule_socket *rs);

int atfGetGroupValue(void *ctx, const char *iface, uint8_t *group_val);
int getMLDLinkID_cfg80211(void *ctx, const char *iface, uint8_t *linkID);
int getPP_cfg80211(void *ctx, const char *ifname, int *puncture_pattern);
int setDfsPuncture_cfg80211(void *ctx, const char * ifname, uint8_t dfs_punc_enable);
int getStrictPunc_cfg80211(void *ctx, const char *ifname, int *strict_punc);
int setEirpThreshold_cfg80211(void *ctx, const char * ifname, uint8_t eirp_th);
int setAWGNChannel_cfg80211(void *ctx, const char * ifname, uint8_t channel);
int getAcsMinDwellTime_cfg80211(void *ctx, const char * ifname, int32_t *mindwelltime);
int setWideBandScan_cfg80211(void *ctx, const char *ifname, int value);
int getStaSignal_cfg80211(void *ctx, const char * ifname, int *rssi);
#endif /* #define __WLANIF_CMN_H__ */
