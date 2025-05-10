/*
 * Copyright (c) 2016-2021,2023-2024  Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#if UMAC_SUPPORT_CFG80211
#ifndef _IEEE80211_CFG80211_H___
#define _IEEE80211_CFG80211_H___
#include <wlan_cfg80211_api.h>
#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include <osif_private.h>
#include <wlan_cfg80211_scan.h>
#include <cdp_txrx_stats_struct.h>
#include <cfg80211_external.h>
#include <ieee80211_objmgr_priv.h>
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
#include <telemetry_agent_app_if.h>
#endif

#include <wlan_mlo_t2lm.h>

#ifdef WLAN_SUPPORT_TELEMETRY
#include <wlan_stats.h>
#endif /* WLAN_SUPPORT_TELEMETRY */

#define RATETAB(rate, rate_id, flag) \
{ \
    .bitrate = rate, \
    .hw_value = rate_id, \
    .flags = flag, \
}

#define IS_NULL_ADDR(_a)            \
    ((_a)[0] == 0x00 &&             \
     (_a)[1] == 0x00 &&             \
     (_a)[2] == 0x00 &&             \
     (_a)[3] == 0x00 &&             \
     (_a)[4] == 0x00 &&             \
     (_a)[5] == 0x00)

#define MAX_BUFFER_LEN 1180

#define REG_RULE_2412_2462    REG_RULE(2412-10, 2462+10, 40, 0, 20, 0)
#define REG_RULE_2467_2472    REG_RULE(2467-10, 2472+10, 40, 0, 20, 0)
#define REG_RULE_2484         REG_RULE(2484-10, 2484+10, 40, 0, 20,0)
#define REG_RULE_5180_5320    REG_RULE(5180-10, 5320+10, 160, 0, 20,0)
#define REG_RULE_5500_5640    REG_RULE(5500-10, 5640+10, 160, 0, 20,0)
#define REG_RULE_5660_5720    REG_RULE(5660-10, 5720+10, 80, 0, 20,0)
#define REG_RULE_5745_5925    REG_RULE(5745-10, 5925+10, 160, 0, 20, 0)
#define REG_RULE_5955_7115    REG_RULE(5955-10, 7115+10, 160, 0, 20, 0)

#define CFG80211_MAX_CHANNELS_2G 14 /* all possible channels for 2.4G */
#define CFG80211_MAX_CHANNELS_5G 47 /* all possible channels for 5G */
#define CFG80211_MAX_CHANNELS_6G 64 /* all possible channels for 6G ((253 + 3)/4) */
#define CFG80211_RTS_THRESHOLD_DISABLE  0xffffffff /*cfg80211 value for disabling RTS threshold */
#define CFG80211_FRAG_THRESHOLD_DISABLE 0xffffffff /*cfg80211 value for disabling FRAG threshold */
#define CFG80211_VAP_CREATE 1
#ifndef ENABLE_CFG80211_BACKPORTS_MLO
#define MAX_OSIF_MLDEV 24
#else
#define MAX_OSIF_MLDEV 48
#endif
#define MLD_PHY_NAME 10
#define ESL_EGID_INFO_MAX_SIZE 4
#define CFG80211_BSS_PARAM_NOT_CHANGED -1
#define CFG80211_BSS_PARAM_SET 1
#define CFG80211_BSS_PARAM_CLEAR 0

#define CFG80211_SUPPORT_BSS_PARAMS 0x0001
#define CFG80211_SUPPORT_MGMT_TX_CANCEL_WAIT 0x0002
#define CFG80211_SUPPORT_SET_HT_CAP 0x0004
#define CFG80211_SUPPORT_SET_VHT_CAP 0x0008
#define CFG80211_SUPPORT_SET_HE_CAP 0x0010
#define CFG80211_SUPPORT_CHANGE_BCN 0x0020
#define CFG80211_SUPPORT_TXQ_PARAM 0x0040
#define CFG80211_SUPPORT_DEFAULT_BEACON_KEY 0x0000
#define CFG80211_SUPPORT_BSS_PARAMS_SHORT_PREAMBLE 0x0000
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
/*
 * The below enum has been deprecated in 5.4 kernel.
 * For backward-compatibility, it is added here.
 */
/**
 * enum ieee80211_band - supported frequency bands
 * The bands are assigned this way because the supported
 * bitrates differ in these bands.
 *
 * @IEEE80211_BAND_2GHZ: 2.4GHz ISM band
 * @IEEE80211_BAND_5GHZ: around 5GHz band (4.9 - 5.7 GHz)
 * @IEEE80211_BAND_60GHZ: around 60 GHz band (58.32 - 64.80 GHz)
 * @IEEE80211_BAND_6GHZ: around 6 GHz band (5.9 - 6.4 GHz)
 * @IEEE80211_NUM_BANDS: number of defined bands
 */
enum ieee80211_band {
        IEEE80211_BAND_2GHZ = NL80211_BAND_2GHZ,
        IEEE80211_BAND_5GHZ = NL80211_BAND_5GHZ,
        IEEE80211_BAND_60GHZ = NL80211_BAND_60GHZ,
        IEEE80211_BAND_6GHZ = NL80211_BAND_6GHZ,

        /* keep last */
        IEEE80211_NUM_BANDS
};
#endif

/**
 * enum command_execution_type - Command execution type
 * @COMMAND_EXECUTION_TYPE_NON_BLOCKING: Control is
 * returned to user space immediately after mode and/or channel
 * change request is queued to FW
 * @COMMAND_EXECUTION_TYPE_BLOCKING: Control is returned
 * to user space only after the mode and/or channel change is
 * completed and the vdev on which mode and/or channel change is
 * requested is moved to "WLAN_VDEV_SS_UP_ACTIVE" state
 * @COMMAND_EXECUTION_TYPE_MAX: Maximum value of command
 * type
 */
enum command_execution_type {
    COMMAND_EXECUTION_TYPE_NON_BLOCKING,
    COMMAND_EXECUTION_TYPE_BLOCKING,
    COMMAND_EXECUTION_TYPE_MAX,
};

/**
  * enum amsdu_enable_flag - flag to indicate to enable/disable A-MSDUs
  * @FLAG_INVALID: amsdu flag is invalid
  * @DISABLE_AMSDU: disable AMSDUs
  * @ENABLE_AMSDU: enable AMSDUs
  */
enum amsdu_enable_flag {
    FLAG_INVALID = 0,
    DISABLE_AMSDU = 1,
    ENABLE_AMSDU = 2,
};

/*
 * We provision space for a max of 14 currently available channels from 2.4 GHz,
 * 41 channels (currently available and potential) from 5 GHz, approx. 60
 * potential channels from 6 GHz, each channel requiring approx 92 bytes, for a
 * total of (14 + 41 + 60) x 92 = 10580 bytes.
 *
 * We also provision for some buffer space, taking the buffered total to a
 * rounded figure of 1024 * 14 = 14336 bytes. We thus finally request for an
 * event message buffer size of 14336 bytes.
 */
#define EXTSCAN_EVENT_BUF_SIZE (14336)

#define RROP_REPLY_BUF_SIZE (4096)

#define MAX_SCAN_SSID                    (10)
#define SIR_MAC_MAX_SSID_LENGTH          (32)
#define MAX_CHANNEL (NUM_24GHZ_CHANNELS + NUM_5GHZ_CHANNELS + NUM_6GHZ_CHANNELS)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0))
#define NUM_NL80211_BANDS ((enum nl80211_band)IEEE80211_NUM_BANDS)
#endif
#define RATE_MASK         (0x7f)

/* Enums for MBSSID non-Tx profile get API */
enum {
    IEEE80211_PARAM_VENDOR_IE   = 0,
    IEEE80211_PARAM_OPTIONAL_IE = 1,
};

/* Enums for radio level non-Tx profile API cases */
enum {
    IEEE80211_PARAM_NTX_PFL_SIZE = 0,
    IEEE80211_PARAM_MAX_PP       = 1,
};

/* Enums for MBSSID specific non-Tx profile API handler */
enum {
    IEEE80211_EMA_VAP_CONFIG = 1,
    IEEE80211_EMA_IC_CONFIG  = 2,
};

enum {
    IEEE80211_RCPARAMS_RTPARAM	= 1,
    IEEE80211_RCPARAMS_RTMASK	= 2,
};

#if CFG80211_SUPPORT_11AX_HOSTAPD
enum hostpad_wireless_mode {
    MODE_ABG,
    MODE_HT,
    MODE_VHT,
    MODE_HE,
#if WLAN_FEATURE_11BE
    MODE_EHT
#endif
};
#endif /* CFG80211_SUPPORT_11AX_HOSTAPD */

/**
 * struct hdd_vendor_acs_chan_params - vendor acs channel parameters
 * @channel_count: channel count
 * @channel_list: pointer to channel list
 * @vendor_pcl_list: pointer to pcl list
 * @vendor_weight_list: pointer to pcl weight list
 */
struct hdd_vendor_acs_chan_params {
    uint32_t channel_count;
    uint8_t *channel_list;
    uint8_t *vendor_pcl_list;
    uint8_t *vendor_weight_list;
};

/**
 * struct vendor_chan_info - vendor channel info
 * @pri_ch_freq: primary channel frequency
 * @sec_ch_freq: secondary channel frequency
 * @seg0_center_ch_freq: segment 0 frequency
 * @seg1_center_ch_freq: segment 1 frequency
 * @puncture_bitmap: puncture bitmap
 * @chan_width: channel width
 */
struct vendor_chan_info {
    qdf_freq_t pri_ch_freq;
    qdf_freq_t sec_ch_freq;
    qdf_freq_t seg0_center_ch_freq;
    qdf_freq_t seg1_center_ch_freq;
    uint16_t puncture_bitmap;
    uint8_t chan_width;
};

/* TODO: qca_wlan_genric_data, qca_wlan_set_params,
 * qca_wlan_get_params
 * These should be align with qca_wlan_vendor_attr_config
 * in qca-vendor.h
 * QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_COMMAND
 * QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_DATA
 *
 * It requires qca_nl80211_lib changes also in reading
 * responses
 */
enum qca_wlan_genric_data {
    QCA_WLAN_VENDOR_ATTR_GENERIC_PARAM_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_PARAM_DATA,
    QCA_WLAN_VENDOR_ATTR_PARAM_LENGTH,
    QCA_WLAN_VENDOR_ATTR_PARAM_FLAGS,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GENERIC_PARAM_LAST,
    QCA_WLAN_VENDOR_ATTR_GENERIC_PARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_GENERIC_PARAM_LAST - 1
};

enum qca_wlan_set_params {
    QCA_WLAN_VENDOR_ATTR_SETPARAM_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_SETPARAM_COMMAND,
    QCA_WLAN_VENDOR_ATTR_SETPARAM_VALUE,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_SETPARAM_LAST,
    QCA_WLAN_VENDOR_ATTR_SETPARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_SETPARAM_LAST - 1
};

enum qca_wlan_get_params {
    QCA_WLAN_VENDOR_ATTR_GETPARAM_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_GETPARAM_VALUE,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GETPARAM_LAST,
    QCA_WLAN_VENDOR_ATTR_GETPARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_GETPARAM_LAST - 1
};

enum qca_wlan_mac_commands {
    MAC_COMMND_ADD_WDS_ADDR = 1,
    MAC_COMMND_SEND_WOWPKT = 2,
    MAC_COMMND_GET_ACLMAC = 3,
    MAC_COMMND_SET_PEER_KICKOUT = 4,
};

#define INVALID_LINK_ID 0xFF
#define is_valid_link_id(X) !(X >= INVALID_LINK_ID)

struct cfg80211_context {
#ifndef ENABLE_CFG80211_BACKPORTS_MLO
    struct ieee80211com *ic;
    struct net_device *mld_netdev; /* for mld-wifi */
#else
    uint8_t is_mld_phy;  /* set this for MLO chips */
    struct ieee80211com *ic_list[NUM_MAX_RADIOS]; /* Max Radios*/
    struct wiphy *mld_wiphy; /* pointer to SingleWiphy */
    uint8_t ml_grp_id;
    /* HOLD MLD netdev list */
    struct osif_mldev *mldev[MAX_OSIF_MLDEV]; /* List of MLDs and context */
    qdf_atomic_t mld_count;
    qdf_atomic_t netdev_count;
#endif
};

#ifdef WLAN_FEATURE_11BE_MLO
/*
 * SingelWiphy private context
 */

struct cfg80211_mld_wiphy_priv {
    struct wiphy *mld_wiphy;
    struct net_device *mld_netdev;
    uint8_t ml_grp_id;
    /* HOLD MLD netdev list */
    struct osif_mldev *mldev[MAX_OSIF_MLDEV];
    qdf_atomic_t mld_count;
};

/*
 * Internal data struct
 */
struct cfg80211_mld_wiphy_context {
    struct wiphy *mld_wiphy;
    struct ieee80211_supported_band wlan_band_2g; /* 2g band information that is required for wiphy */
    struct ieee80211_supported_band wlan_band_5g; /* 5g band information that is required for wiphy */
    struct ieee80211_supported_band wlan_band_6g; /* 6g band information that is required for wiphy */
    struct ieee80211_sband_iftype_data iftype_data_2g[NUM_NL80211_IFTYPES]; /* HE, EHT capabilties for 2G */
    struct ieee80211_sband_iftype_data iftype_data_5g[NUM_NL80211_IFTYPES]; /* HE, EHT capabilties for 5G */
    struct ieee80211_sband_iftype_data iftype_data_6g[NUM_NL80211_IFTYPES]; /* HE, EHT capabilties for 6G */
    struct wlan_cfg80211_band_chans    channel_list_g; /* Variable channel list */
    struct wlan_cfg80211_band_chans    channel_list_a; /* Variable channel list */
    struct wlan_cfg80211_band_chans    channel_list_6g; /* Variable channel list */
    struct ieee80211com *ic[NUM_MAX_RADIOS]; /* IC list for mld group */
    uint8_t radio_cnt; /* Radio Count */
    bool is_created;
};

#ifndef ENABLE_CFG80211_BACKPORTS_MLO
/**
 * struct singlewiphy_netdev_ctx - singlewiphy netdev context
 * @mld_dev: netdev mld device
 * @wiphy: wiphy
 * @wdev: wireless device
 */
struct singlewiphy_netdev_ctx {
    struct net_device *mld_dev;
    struct wiphy *wiphy;
    struct wireless_dev wdev;
};
#endif
#endif

enum qca_wlan_vendor_channel_width wlan_cfg80211_get_qca_vendor_channel_width(struct ieee80211com *ic);

#ifdef WLAN_FEATURE_11BE

/* cfg80211_mlo_link_update_evt_params - mlo_link enable/disable evt params
 * @hw_link_id_map: Bitmap of all the disabled hw_link_ids
 * @ap_mld_addr: AP MLD Address of the currently disabled links
 */

#define SET_PEER_FLAG(_val, _attr) do {             \
    (_val) |= (1 << (PEER_FLAGS_##_attr##_BIT));    \
    } while (0)
#define CLEAR_PEER_FLAG(_val, _attr) do {           \
    (_val) &= ~(1 << (PEER_FLAGS_##_attr##_BIT));   \
    } while (0)
#define IS_PEER_FLAG_SET(_val, _attr) \
    ((_val >> PEER_FLAGS_##_attr##_BIT) & 0x1)

/* Bit definitions for peer_flags */
#define PEER_FLAGS_BTM_CAP_BIT 0        /* Peer BTM capability */

struct cfg80211_mlo_link_update_evt_params
{
    uint16_t hw_link_id_map;
    uint8_t  ap_mld_addr[QDF_MAC_ADDR_SIZE];
};

enum link_band_caps {
	LINK_BAND_2GHz,
	LINK_BAND_5GHz,
	LINK_BAND_5GHz_LOW,
	LINK_BAND_5GHz_High,
	LINK_BAND_6Ghz,
	LINK_BAND_6Ghz_Low,
	LINK_BAND_6GHz_High,
	LINK_BAND_INVALID,
};

/* struct link_info - link info structure
 * @hw_link_id: HW link id of the pdev
 * @link_mac_addr: link MAC
 * @chan_bw: Channel bandwidth of the AP
 * @chan_freq: Channel frequency of AP
 * @band_cap: Channel band of the AP
 */
struct link_info {
	uint16_t hw_link_id;
	uint8_t link_mac_addr[6];
	uint8_t chan_bw;
	uint16_t chan_freq;
	enum link_band_caps band_cap;
};

struct soc_device_info {
	uint8_t soc_id;
	uint8_t num_links;
	struct link_info link_info[MAX_T2LM_LINKS];
};

struct wlan_eht_peer_caps {
    uint32_t tx_mcs_nss_map[QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_MAX];
    uint32_t rx_mcs_nss_map[QCA_WLAN_VENDOR_EHTCAP_TXRX_MCS_NSS_IDX_MAX];
};

enum t2lm_tx_status {
    T2LM_TX_STATUS_NONE= 0,
    T2LM_TX_STATUS_SUCCESS = 1,
    T2LM_TX_STATUS_FAILURE = 2,
    T2LM_TX_STATUS_RX_TIMEOUT = 3,
    T2LM_TX_STATUS_INVALID,
};

enum t2lm_resp_frm_type {
    T2LM_RESP_TYPE_SUCCESS = 0,
    T2LM_RESP_TYPE_DENIED_TID_TO_LINK_MAPPING = 133,
    T2LM_RESP_TYPE_PREFERRED_TID_TO_LINK_MAPPING = 134,
    T2LM_RESP_TYPE_INVALID,
};

enum t2lm_resp_frm_status {
    T2LM_RESP_FRM_SUCCESS = 0,
};

/* struct mld_peer_link_entry - link entry structure
 * @hw_link_id:  HW link id of the pdev
 * @vdev_id: vdev id corresponding to peer
 * @ap_mld_addr: mld addr corresponding to AP
 * @link_addr: MAC address of link peer
 * @ieee_link_id: IEEE link ID of the link peer
 * @soc_id: soc id
 * @is_assoc_link: assoc request received link
 * @chan_bw: negotiated channel bandwidth
 * @chan_freq: Channel frequency
 * @available_airtime: Available airtime of this link
 * @link_rssi: link rssi.will be set only for assoc link
 * @caps: Peer MCS/NSS capabilities
 * @band_cap: Channel band of this link
 * @eff_chan_bw: channel bandwidth after puncturing
 * @peer_flags: Peer capabilities
 */
struct mld_peer_link_entry {
    uint16_t hw_link_id;
    uint8_t vdev_id;
    uint8_t ap_mld_addr[QDF_MAC_ADDR_SIZE];
    uint8_t link_addr[QDF_MAC_ADDR_SIZE];
    uint8_t ieee_link_id;
    uint8_t soc_id;
    uint8_t is_assoc_link;
    uint8_t chan_bw;
    uint16_t chan_freq;
    uint16_t available_airtime;
    int8_t link_rssi;
    struct wlan_eht_peer_caps caps;
    enum link_band_caps band_cap;
    uint16_t eff_chan_bw;
    uint16_t peer_flags;
};

/* struct peer_t2lm_policy_event - peer T2LM policy event structure
 * @mld_mac_addr: non-AP MLD macaddr (peer MLD mac)
 * @category: T2LM mapping frame category value
 * @num_links: Number of links present in the MLD
 * @dialog_token: dialog token used in T2LM request and response frame
 * @link_entry: MLD peer info
 * @t2lm_info: T2LM mapping from T2LM req and resp
 * @t2lm_tx_status: Status code applicable for the T2LM frames
 * @t2lm_mapping: T2LM request mapping
 * @t2lm_resp_status: T2LM response status
 */
struct peer_t2lm_policy_event {
    uint8_t mld_mac_addr[QDF_MAC_ADDR_SIZE];
    enum qca_wlan_vendor_t2lm_category category;
    uint8_t num_links;
    uint8_t dialog_token;
    struct mld_peer_link_entry link_entry[MLD_MAX_LINKS];
    struct wlan_t2lm_info t2lm_info[WLAN_T2LM_MAX_DIRECTION];
    enum t2lm_tx_status t2lm_tx_status;
    enum wlan_t2lm_enable t2lm_mapping;
    enum t2lm_resp_frm_status t2lm_resp_status;
    struct wlan_preferred_links preference_link;
};

struct peer_generic_assoc_event {
	uint8_t mld_mac_addr[QDF_MAC_ADDR_SIZE];
	uint8_t num_links;
	enum wlan_t2lm_enable t2lm_mapping;
	enum qca_wlan_vendor_t2lm_category category;
	struct mld_peer_link_entry link_entry[MLD_MAX_LINKS];
	struct wlan_t2lm_info t2lm_info[WLAN_T2LM_MAX_DIRECTION];
};

struct soc_level_info {
	int8_t t2lm_app_version;
	int8_t t2lm_driver_version;
	int8_t num_soc_devices;
	struct soc_device_info soc_info[WLAN_OBJMGR_MAX_DEVICES];
};

/* struct peer_t2lm_policy_cmd - peer T2LM policy cmd structure
 * @mld_mac_addr: MLD macaddr
 * @category: T2LM mapping frame category value
 * @num_links: Number of links present in the MLD
 * @link_entry: MLD peer info
 * @t2lm_info: T2LM mapping from T2LM req and resp
 * @t2lm_resp_type: T2LM response type
 */
struct peer_t2lm_policy_cmd {
    uint8_t mld_mac_addr[QDF_MAC_ADDR_SIZE];
    enum qca_wlan_vendor_t2lm_category category;
    uint8_t num_links;
    struct mld_peer_link_entry link_entry[MLD_MAX_LINKS];
    struct wlan_t2lm_info t2lm_info[T2LM_MAX_NUM_TIDS];
    enum t2lm_resp_frm_type t2lm_resp_type;
};

/* struct peer_t2lm_tear_down - peer T2LM tear down structure
 * @mld_mac_addr: MLD macaddr
 * @category: T2LM mapping frame category value
 * @num_links: Number of links present in the MLD
 * @peer_entry: MLD peer info
 * populate only link link_addr, hw_link_id, link_index and is_assoc_link
 * @preference_link: Preferred link info
 */
struct peer_t2lm_tear_down {
    uint8_t mld_mac_addr[QDF_MAC_ADDR_SIZE];
    enum qca_wlan_vendor_t2lm_category category;
    uint8_t num_links;
    struct mld_peer_link_entry peer_entry[MLD_MAX_LINKS];
    struct wlan_preferred_links preference_link;
};

/* struct peer_generic_disassoc_event - peer disassoc event structure
 * @mld_mac_addr: MLD macaddr
 * @category: T2LM mapping frame category value
 * @num_links: Number of links present in the MLD
 * @peer_entry: MLD peer info
 * populate only link link_addr, hw_link_id, link_index and is_assoc_link
 */
struct peer_generic_disassoc_event {
    uint8_t mld_mac_addr[QDF_MAC_ADDR_SIZE];
    enum qca_wlan_vendor_attr_app_generic_category category;
    uint8_t num_links;
    struct mld_peer_link_entry peer_entry[MLD_MAX_LINKS];
};

int wlan_cfg80211_send_peer_t2lm_event(struct peer_t2lm_policy_event *t2lm_policy,
        struct wlan_objmgr_peer *peer);
int wlan_cfg80211_send_t2lm_teardown_event(struct peer_t2lm_tear_down *tear_down_event,
        struct wlan_objmgr_peer *peer);
enum wlan_link_band_caps wlan_cfg80211_link_band_info(struct wlan_objmgr_pdev *pdev);

#endif /* WLAN_FEATURE_11BE */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0))
static inline int
wlan_nla_put_u64(struct sk_buff *skb, int attrtype, u64 value)
{
    return nla_put_u64(skb, attrtype, value);
}
#else
static inline int
wlan_nla_put_u64(struct sk_buff *skb, int attrtype, u64 value)
{
    return nla_put_u64_64bit(skb, attrtype, value, NL80211_ATTR_PAD);
}
#endif

#ifdef WLAN_SUPPORT_TELEMETRY
/**
 * Function to setup respective object's
 * stats buffer to send over netlink socket
 */
QDF_STATUS telemetric_reply_setup(struct wlan_objmgr_pdev *pdev,
                                  struct wlan_objmgr_vdev *vdev,
                                  struct stats_config *cfg,
                                  uint8_t *mac);
#endif /* WLAN_SUPPORT_TELEMETRY */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
int ieee80211_cfg80211_dump_chandef(struct cfg80211_chan_def *chan_def);
#else
int ieee80211_cfg80211_dump_chandef(struct cfg80211_chan_def *chan_def, uint32_t punct_bitmap);
#endif
void ieee80211_cfg80211_schedule_channel_notify(void *context);
void ieee80211_cfg80211_schedule_channel_started_notify(void *context);
int wlan_cfg80211_set_freq(struct net_device *dev, struct ieee80211_channel *cfgchan);
int wlan_cfg80211_chan_to_phymode(struct ieee80211vap *vap,
        struct cfg80211_ap_settings *params,
        struct cfg80211_chan_def chandef);
int wlan_cfg80211_set_phymode(struct wiphy *wiphy,
        struct net_device *dev,
        struct cfg80211_ap_settings *params,
        struct cfg80211_chan_def chandef);
int wlan_cfg80211_set_channel(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params,
        struct wlan_cfg8011_vendor_params *vparams,
        bool send_acs_report);
/**
 * wlan_cfg80211_get_channel_config() - Get chan_num and band.
 * @wiphy: wiphy handle.
 * @wdev:  wireless device handle.
 * @params: CFG param.
 * Return: -EINVAL on failure, 0 on success.
 */
int wlan_cfg80211_get_channel_config(struct wiphy *wiphy,
                                     struct wireless_dev *wdev,
                                     struct wlan_cfg8011_genric_params *params);

int wlan_cfg80211_set_beacon_interval(wlan_if_t vap, struct ieee80211com *ic, int value);
int wlan_cfg80211_getopmode(enum nl80211_iftype type, u_int32_t *flags);
int wlan_cfg80211_probe_client(struct wiphy *wiphy, struct net_device *dev,const u8 *peer, u64 *cookie);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
struct wireless_dev *wlan_cfg80211_add_virtual_intf(struct wiphy *wiphy,
        const char *name,
        unsigned char name_assign_type,
        enum nl80211_iftype type,
        struct vif_params *params);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
struct wireless_dev *wlan_cfg80211_add_virtual_intf(struct wiphy *wiphy,
        const char *name,
        unsigned char name_assign_type,
        enum nl80211_iftype type,
        u_int32_t *flags,
        struct vif_params *params);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
struct wireless_dev *wlan_cfg80211_add_virtual_intf(struct wiphy *wiphy,
        const char *name,
        enum nl80211_iftype type,
        u_int32_t *flags,
        struct vif_params *params);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
struct wireless_dev *wlan_cfg80211_add_virtual_intf(struct wiphy *wiphy, char *name,
        enum nl80211_iftype type,
        u_int32_t *flags,
        struct vif_params *params);
#else
struct net_device *wlan_cfg80211_add_virtual_intf(struct wiphy *wiphy, char *name,
        enum nl80211_iftype type,
        u_int32_t *flags,
        struct vif_params *params);
#endif
struct wireless_dev *wlan_cfg80211_mld_add_virtual_intf(struct wiphy *wiphy,
        const char *name,
        unsigned char name_assign_type,
        enum nl80211_iftype type,
        struct vif_params *params);

int wlan_cfg80211_mld_del_virtual_intf(struct wiphy *wiphy, struct wireless_dev *wdev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
int wlan_cfg80211_del_virtual_intf(struct wiphy *wiphy, struct wireless_dev *wdev);
#else
int wlan_cfg80211_del_virtual_intf(struct wiphy *wiphy, struct net_device *dev);
#endif
int wlan_cfg80211_set_beacon(struct wiphy *wiphy,struct net_device *dev, struct cfg80211_beacon_data *params);
int wlan_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_ap_settings *params);
int wlan_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_beacon_data *params);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
int wlan_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev, unsigned int link_id);
#else
int wlan_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_ap_settings *params);
#endif
int wlan_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *dev, struct bss_parameters *params);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
int wlan_scan(struct wiphy *wiphy, struct net_device *dev,struct cfg80211_scan_request *request);
#else
int wlan_scan(struct wiphy *wiphy,struct cfg80211_scan_request *request);
#endif
int wlan_connect(struct wiphy *wiphy, struct net_device *ndev, struct cfg80211_connect_params *req);
int wlan_disconnect(struct wiphy *wiphy, struct net_device *dev, u16 reason);
int wlan_join_ibss(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_ibss_params *params);
int wlan_leave_ibss(struct wiphy *wiphy, struct net_device *dev);
int wlan_cfg80211_set_wiphy_paramss(struct wiphy *wiphy, u32 changed);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
int wlan_cfg80211_set_txpower(struct wiphy *wiphy, struct wireless_dev *wdev, enum nl80211_tx_power_setting type, int dbm);
#else
int wlan_cfg80211_set_txpower(struct wiphy *wiphy, enum nl80211_tx_power_setting type, int dbm);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0) || defined(WITH_BACKPORTS)
int wlan_cfg80211_get_txpower(struct wiphy *wiphy, struct wireless_dev *wdev,int *dbm);
#else
int wlan_cfg80211_get_txpower(struct wiphy *wiphy, int *dbm);
#endif
#ifdef CFG80211_MLO_SINGLE_NETDEV_API
int wlan_cfg80211_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *netdev, int link_id, u8 key_index);
#else
int wlan_cfg80211_set_default_mgmt_key(struct wiphy *wiphy, struct net_device *netdev, u8 key_index);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
int wlan_cfg80211_get_station(struct wiphy *wiphy, struct net_device *dev, const uint8_t *mac, struct station_info *sinfo);
#else
int wlan_cfg80211_get_station(struct wiphy *wiphy, struct net_device *dev, uint8_t *mac, struct station_info *sinfo);
#endif
int wlan_cfg80211_set_power_mgmt(struct wiphy *wiphy, struct net_device *dev, bool mode, int timeout);
int wlan_cfg80211_set_pmksa(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_pmksa *pmksa);
int wlan_cfg80211_del_pmksa(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_pmksa *pmksa);
int wlan_cfg80211_flush_pmksa(struct wiphy *wiphy, struct net_device *dev);
int wlan_cfg80211_update_ft_ies(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_update_ft_ies_params *ftie);
int wlan_cfg80211_set_rekey_dataata(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_gtk_rekey_data *data);
int wlan_cfg80211_sched_scan_start(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_sched_scan_request *request);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
int wlan_cfg80211_sched_scan_stop(struct wiphy *wiphy, struct net_device *dev, u64 reqid);
#else
int wlan_cfg80211_sched_scan_stop(struct wiphy *wiphy,  struct net_device *dev);
#endif
void wlan_regulatory_notifier(struct wiphy *wiphy,struct regulatory_request *request);
int wlan_cfg80211_set_mac_acl(struct wiphy *wiphy, struct net_device *dev, const struct cfg80211_acl_data *params);
/* FTM */
int wlan_cfg80211_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev, struct ieee80211_channel *chan, unsigned int duration, u64 *cookie);
int wlan_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie);
int wlan_cfg80211_testmode(struct wiphy *wiphy, struct wireless_dev *wdev, void *data, int len); /* Receives UTF commands from FTM daemon */
int wlan_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev, struct cfg80211_mgmt_tx_params *params, u64 *cookie);
int wlan_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie);
int wlan_get_num_channels(struct ieee80211com *ic, enum ieee80211_phymode phymode);
int wlan_cfg80211_update_channel_list(struct ieee80211com *ic);
void wlan_cfg80211_register_vendor_cmd_event_handlers(struct ieee80211com *ic);
void wlan_cfg80211_unregister_vendor_cmd_event_handlers(struct ieee80211com *ic);

void cfg80211_populate_band_2g(struct ieee80211com *ic, struct ieee80211_supported_band *band, struct ieee80211_channel *channel_list, int num_channels);
void cfg80211_populate_band_5g(struct ieee80211com *ic, struct ieee80211_supported_band *band, struct ieee80211_channel *channel_list, int num_channels);
int ieee80211_cfg80211_init(struct net_device *dev, struct wiphy *wiphy, struct ieee80211com *ic);
#ifdef WLAN_MLO_MULTI_CHIP
int ieee80211_cfg80211_radio_attach(struct device *dev, struct net_device *net_dev, struct ieee80211com *ic, struct mlo_setup_info *setup_mlo);
int ieee80211_cfg80211_radio_detach(struct ieee80211com *ic, struct mlo_setup_info *setup_mlo);
#else
int ieee80211_cfg80211_radio_attach(struct device *dev, struct net_device *net_dev, struct ieee80211com *ic);
int ieee80211_cfg80211_radio_detach(struct ieee80211com *ic);
#endif
#ifdef WLAN_MLO_MULTI_CHIP
int ieee80211_cfg80211_mld_wiphy_attach(struct device *dev,
        struct net_device *net_dev,
        struct ieee80211com *ic, struct mlo_setup_info *setup_mlo);
int ieee80211_cfg80211_mld_wiphy_detach(struct wlan_objmgr_psoc *psoc,
        struct mlo_setup_info *setup_mlo, struct ieee80211com *ic);
#endif
void ieee80211_cfg80211_send_connect_result(osif_dev  *osdev, u_int8_t bssid[QDF_MAC_ADDR_SIZE], uint8_t status);
void ieee80211_cfg80211_disconnected(wlan_if_t vap, struct net_device *dev, u_int8_t reasoncode);
int ieee80211_ucfg_handle_dbgreq (struct net_device *dev, osif_dev *osifp, struct ieee80211req_athdbg *req, void *buff, unsigned int data_size);
int ieee80211_ucfg_radio_level_handle_dbgreq (struct net_device *dev, struct ieee80211req_athdbg *req);
int wlan_cfg80211_start_acs_scan(wlan_if_t vap, cfg80211_hostapd_acs_params *cfg_acs_params);
void wlan_cfg80211_acs_report_channel(wlan_if_t vap,
                                  struct ieee80211_ath_channel *channel);
int wlan_cfg80211_do_dcs_trigger(wlan_if_t vap,
                                 uint32_t interference_type);
void wlan_cfg80211_send_wiphy_regd_sync_event(struct ieee80211com *ic);
void wlan_cfg80211_generic_event(struct ieee80211com *ic,
                            int cmdid,
                            void *buffer,
                            uint32_t buffer_len,
                            struct net_device *vap_dev,
                            gfp_t gfp);
void wlan_cfg80211_dfs_cac_start(wlan_if_t vap, u_int32_t timeout);
uint64_t wlan_get_channel_flags(uint64_t flags);
uint32_t wlan_get_channel_flags_ext(uint32_t flags);
int ieee80211_sendwowpkt(struct net_device *dev, u_int8_t macaddr[QDF_MAC_ADDR_SIZE]);

int ieee80211_add_wdsaddr(wlan_if_t vap, union iwreq_data *u);
#if WLAN_DFS_CHAN_HIDDEN_SSID
int ieee80211_set_conf_bssid(wlan_if_t vap, u_int8_t macaddr[QDF_MAC_ADDR_SIZE]);
#endif
int wlan_cfg80211_phyerr(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int data_len);
/* free the clone params list on radio detach */
void wlan_cfg80211_free_clone_params_list(struct ieee80211com *ic);
int wlan_cfg80211_set_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
                struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_nawds_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#ifdef UMAC_SUPPORT_MLNAWDS
int wlan_cfg80211_mlnawds_params(struct wiphy *wiphy,
                                 struct wireless_dev *wdev,
                                 struct wlan_cfg8011_genric_params *params);
#endif
#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
int wlan_cfg80211_mesh_params(struct wiphy *wiphy,
                              struct wireless_dev *wdev,
                              struct wlan_cfg8011_genric_params *params);
#endif
#endif
int wlan_cfg80211_get_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_hmwds_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#ifdef IPA_OFFLOAD
int wlan_cfg80211_ipa_wds_delete_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#endif
int wlan_cfg80211_list_cap(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_list_chan(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#ifdef WLAN_FEATURE_11BE
int wlan_cfg80211_list_channels(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#endif
int wlan_cfg80211_active_chan_list(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_list_chan160(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_wnm_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_vendorie_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_set_maxrate(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_nac_rssi_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_nac_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_peer_isolation_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#if QCA_AIRTIME_FAIRNESS
int wlan_cfg80211_atf(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#endif
int wlan_cfg80211_list_sta(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params,
        bool query_for_sta);
int wlan_cfg80211_list_scan(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#ifdef WLAN_FEATURE_11BE_MLO
int wlan_cfg80211_list_link_info(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#endif
#if QLD
int wlan_cfg80211_get_qld_dump(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#endif
int wlan_cfg80211_me_list_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_sta_stats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_vap_stats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params,
        bool sta_vap);
int wlan_cfg80211_radio_stats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_80211stats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_cloneparams(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_extendedstats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_athstats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_phystats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_ald_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_vendor_scan(struct wiphy *wiphy,
                              struct wireless_dev *wdev,
                              const void *data,
                              int data_len);
int wlan_cfg80211_update_vendor_channel(struct wiphy *wiphy,
						struct wireless_dev *wdev,
						const void *data, int data_len);
QDF_STATUS update_vendor_acs_channel(wlan_if_t vap, uint8_t reason,
                  uint8_t channel_cnt,
                  struct vendor_chan_info *channel_list);
int wlan_hdd_cfg80211_dump_survey(struct wiphy *wiphy,
                                  struct net_device *dev,
                                  int idx,
                                  struct survey_info *survey);
int ieee80211_cfg80211_construct_chandef(struct cfg80211_chan_def *chan_def,
                                         struct ieee80211_channel *nl_chan,
                                         struct ieee80211_ath_channel *ath_chan);
int wlan_cfg80211_httstats(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);

void wlan_cfg80211_wifi_fwstats_event(struct ieee80211com *ic, void *buffer,
                                  uint32_t buffer_len);

int wlan_cfg80211_vendor_get_rropinfo(struct wiphy *wiphy,
                              struct wireless_dev *wdev,
                              const void *data,
                              int data_len);
void wlan_vendor_fwd_mgmt_frame(wlan_if_t vap, struct sk_buff *skb, u_int16_t subtype);
void ieee80211_cfg80211_post_ft_event(osif_dev *osdev);
void ieee80211_cfg80211_post_ft_workq(void *arg);

int wlan_cfg80211_cfr_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);

#ifdef QCA_BLOCK_EVENT_TO_USERSPACE
int wlan_cfg80211_set_event_filter(struct wiphy *wiphy,
                                   struct wireless_dev *wdev,
                                   struct wlan_cfg8011_genric_params *params);
int wlan_cfg80211_get_event_filter(struct wiphy *wiphy,
                                   struct wireless_dev *wdev,
                                   struct wlan_cfg8011_genric_params *params);
#endif /* QCA_BLOCK_EVENT_TO_USERSPACE */

void ieee80211_ucfg_setparam_on_partner_vap(wlan_if_t vap, int param,
                                            int value, char *extra);

#ifdef WLAN_WIFI_RADAR_ENABLE
int wlan_cfg80211_wifi_radar_params(struct wiphy *wiphy,
                                    struct wireless_dev *wdev,
                                    struct wlan_cfg8011_genric_params *params);
#endif

#ifdef CONFIG_SAWF_DEF_QUEUES
int wlan_cfg80211_sawf_params(struct wiphy *wiphy,
        struct wireless_dev *wdev,
        struct wlan_cfg8011_genric_params *params);
#endif

int wlan_cfg80211_rtt_params(struct wiphy *wiphy, struct wireless_dev *wdev,
                             struct wlan_cfg8011_genric_params *params);

int wlan_cfg80211_get_channel_survey_stats(struct ieee80211com *ic);
int wlan_cfg80211_get_cur_chan_util(struct ieee80211com *ic);
int wlan_cfg80211_reset_channel_survey_stats(struct ieee80211com *ic);
void wlan_cfg80211_hmwds_ast_add_status_event(struct ieee80211com *ic,
                                              void *buffer,
                                              uint32_t buffer_len);
void wlan_cfg80211_mbssid_multigroup_tx_vdev_notification(wlan_if_t vap, bool is_up_event);
#ifdef QCA_SUPPORT_WDS_EXTENDED
void wlan_cfg80211_wds_ext_peer_learn(wlan_if_t vap,
                                      uint8_t peer_mac_addr[QDF_MAC_ADDR_SIZE]);
#endif /* QCA_SUPPORT_WDS_EXTENDED */
#ifndef WIFI_MONITOR_SUPPORT
static inline
int monitor_cfg80211_set_peer_pkt_capture_params(struct wiphy *wiphy,
						 struct wireless_dev *wdev,
						 struct wlan_cfg8011_genric_params *params)
{
	return EOK;
}
#endif
int cfg80211_reply_command(struct wiphy *wiphy, int length,
                           void *data, u_int32_t flag);

/**
 * wlan_cfg80211_sched_mode_probe_resp_handler - Handler for managed MU event
 * @vap: Pointer to vap object
 * @evt_params: parameters corresponding to this event
 *
 * Return: -EINVAL on failure, 0 on success.
 */
int wlan_cfg80211_sched_mode_probe_resp_handler(
		wlan_if_t vap,
		struct wlan_host_sched_mode_probe_resp_event *evt_params);

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * struct cfg80211_mlo_link_removal_evt_params - Link removal event parameters.
 * This information will be in correspondence with an outgoing beacon instance.
 * @tbtt_count: AP removal timer TBTT count in the reported beacon
 * @qtimer_reading: Q-timer reading when the reported beacon is sent out
 * @tsf: TSF of the reported beacon
 * @beacon_interval: Current beacon interval on this VAP
 * @hw_link_id: hwlink id of the link going for removal
 * @mld_addr: mld addr of the mld that link is part of
 */
struct cfg80211_mlo_link_removal_evt_params
{
    uint32_t tbtt_count;
    uint64_t qtimer_reading;
    uint64_t tsf;
    uint16_t beacon_interval;
    uint16_t hw_link_id;
    uint8_t  mld_addr[QDF_MAC_ADDR_SIZE];
};

/**
 * wlan_get_ifidx_by_link_id - Search through vdevs and find the ifindex of
 * vdev with matching link id.
 * @vap: Pointer to the vap object
 * @link_id: link_id of the link
 *
 * Return: Corresponding ifindex
 */
uint32_t wlan_get_ifidx_by_link_id(wlan_if_t vap, uint8_t link_id);

/**
 * wlan_cfg80211_mlo_link_removal_evt_handler() - Handler for MLO link removal
 * event.
 * @vap: VAP corresponding to the link under going removal
 * @evt_params: parameters corresponding to this event
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS wlan_cfg80211_mlo_link_removal_evt_handler(
                wlan_if_t vap,
                struct cfg80211_mlo_link_removal_evt_params *evt_params);
void wlan_cfg80211_prim_netdev_notification(wlan_if_t vap,
                                            struct wlan_objmgr_peer *peer);
/**
 * wlan_cfg80211_hw_status_notification -
 * Send notification for HW status through cfg80211 events
 * @vap: Pointer to vap object
 * @peer: pointer to peer object
 * @enable: enable AMSDUs
 * Return: None
 */
void wlan_cfg80211_hw_status_notification(wlan_if_t vap,
                                          struct wlan_objmgr_peer *peer,
                                          bool enable);

/**
 * wlan_cfg80211_send_link_unavailable_update_event -
 * Send notification for link disable/enable event
 * @vdev: Pointer to vdev object
 * @t2lm: pointer to t2lm info  object
 * Return: QDF_STATUS of operation
 */
QDF_STATUS wlan_cfg80211_send_link_unavailable_update_event(struct wlan_objmgr_vdev *vdev,
                                                   struct wlan_t2lm_info *t2lm);

/**
 * is_mld_phy - Return true if the given wiphy is mld
 * @wiphy: Pointer to wiphy
 * Return: bool
 */
bool is_mld_phy(struct wiphy *wiphy);
#else
static inline
void wlan_cfg80211_hw_status_notification(wlan_if_t vap,
                                          struct wlan_objmgr_peer *peer,
                                          bool enable)
{
}

static inline QDF_STATUS wlan_cfg80211_send_link_unavailable_update_event(struct wlan_objmgr_vdev *vdev,
                                                   struct wlan_t2lm_info *t2lm)
{
	qdf_err("link unavailable event cannot be sent in non-MLO");
	return QDF_STATUS_E_INVAL;
}

static inline
bool is_mld_phy(struct wiphy *wiphy)
{
    return false;
}
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
static inline bool is_allowed_mld_command(enum qca_nl80211_vendor_subcmds_internal cmd)
{
    bool result = false;

    switch (cmd) {
        case QCA_NL80211_VENDOR_SUBCMD_LINK_INFO:
	case QCA_NL80211_VENDOR_SUBCMD_MCTBL_UPDATE:
            result = true;
            break;
        default:
            result = false;
            break;
    }
    return result;
}

static inline struct ieee80211com *wlan_cfg80211_find_ic_by_netdev_name(struct wiphy *wiphy,
        char *dev_name)
{
    uint8_t idx = 0;
    struct cfg80211_context *cfg_ctx = NULL;
    struct ieee80211com *ic = NULL , *tmp_ic = NULL;
    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

    for (idx = 0; idx < NUM_MAX_RADIOS; idx++) {
        tmp_ic = cfg_ctx->ic_list[idx];
        if (tmp_ic) {
            /* Radio Interface */
            if (strncmp(tmp_ic->ic_wdev.netdev->name ,
                        dev_name, strlen(dev_name)) == 0) {
                ic = tmp_ic;
                break;
            }
        }
    }

    return ic;
}

static inline bool is_mld_phy_radio_command(struct wiphy *wiphy,
        struct net_device *netdev)
{
    struct cfg80211_context *cfg_ctx_mld = NULL;
    uint8_t idx = 0;
    bool is_radio_cmd = false;
    struct ieee80211com *ic;

    cfg_ctx_mld = (struct cfg80211_context *)wiphy_priv(wiphy);

    for (idx = 0; idx < NUM_MAX_RADIOS; idx++) {
        ic = cfg_ctx_mld->ic_list[idx];
        if (ic) {
            /* Radio Interface */
            if (ic->ic_wdev.netdev == netdev) {
                   is_radio_cmd = true;
                   break;
            }
        }
    }
    qdf_debug("netdev name %s returning %d", netdev->name, is_radio_cmd);
    return is_radio_cmd;
}
#endif

#ifdef WLAN_FEATURE_11BE_MLO
static inline
bool is_mld_netdev(struct net_device *netdev)
{
    struct osif_mldev *mldev = NULL;

    mldev = (struct osif_mldev *)ath_netdev_priv(netdev);
    if (mldev->dev_type == OSIF_NETDEV_TYPE_MLO)
        return true;

    return false;
}
#endif

int wlan_cfg80211_fill_preferred_link_attr(struct sk_buff *t2lm_event,
					   struct wlan_preferred_links *pref_link_entry);

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
static inline
struct ol_ath_softc_net80211 *get_radio_device_context(struct wiphy *wiphy,
                                                       struct net_device *netdev)
{
    return ath_netdev_priv(netdev);
}

static inline
osif_dev *get_assoc_vap_device_context(struct wiphy *wiphy,
                                       struct net_device *netdev)
{
    struct cfg80211_context *cfg_ctx = NULL;
    struct osif_mldev *mldev = NULL;
    osif_dev *temp_osifp = NULL;
    struct ieee80211vap *temp_vap = NULL;
    int i = 0;

    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

    if (!cfg_ctx->is_mld_phy) {
        return ath_netdev_priv(netdev);
    } else {
        mldev = (struct osif_mldev *)ath_netdev_priv(netdev);
        if (mldev->dev_type != OSIF_NETDEV_TYPE_MLO) {
            qdf_debug("Not mld dev, return legacy vap osifp");
            return (osif_dev *)mldev;
        } else {
            for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
                 temp_osifp = mldev->link_vifs[i];
                 if (!temp_osifp)
                     continue;
                 temp_vap = temp_osifp->os_if;
                 if (wlan_vdev_mlme_is_assoc_sta_vdev(temp_vap->vdev_obj))
                     return temp_osifp;
            }
            return NULL;
        }
    }
}

/* Legacy VAP or Link VAP */
static inline
osif_dev *get_vap_device_context(struct wiphy *wiphy,
                                 struct net_device *netdev,
                                 uint8_t link_id)
{
    struct cfg80211_context *cfg_ctx = NULL;
    struct osif_mldev *mldev = NULL;

    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

    if (!cfg_ctx->is_mld_phy) {
        return ath_netdev_priv(netdev);
    } else {
	/* osif_mld_get_mldev - use this one bonded model is enabled */
        mldev = (struct osif_mldev *)ath_netdev_priv(netdev);
        if (mldev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
            qdf_debug("received command on wds_ext interface");
            return NULL;
        } else if (mldev->dev_type != OSIF_NETDEV_TYPE_MLO) {
            qdf_debug("Not mld dev, return legacy vap osifp");
            return (osif_dev *)mldev;
        } else {
            if (link_id < IEEE80211_MLD_MAX_NUM_LINKS)
                return mldev->link_vifs[link_id];
            else
                return NULL;
        }
    }
}

static inline
osif_dev *get_vap_device_context_no_linkid(struct wiphy *wiphy,
                                           struct net_device *netdev,
                                           const u8 *mac_addr)
{
    struct cfg80211_context *cfg_ctx = NULL;
    struct osif_mldev *mldev = NULL;

    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

    if (!cfg_ctx->is_mld_phy) {
        return ath_netdev_priv(netdev);
    } else {
        mldev = (struct osif_mldev *)ath_netdev_priv(netdev);
        if (mldev->dev_type != OSIF_NETDEV_TYPE_MLO) {
            qdf_debug("Not mld dev, return legacy vap osifp");
            return (osif_dev *)mldev;
        } else {
            osif_dev *temp_osifp = NULL;
            struct ieee80211vap *temp_vap = NULL;
            struct ieee80211_node *ni;
            int i = 0;

            if (!mac_addr) {
                qdf_debug("Mac is Null");
                return NULL;
            }

            for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
                 temp_osifp = mldev->link_vifs[i];
                 if (!temp_osifp)
                     continue;
                 temp_vap = temp_osifp->os_if;
                 ni = ieee80211_find_node_by_mld_mac(temp_vap, (u8 *)mac_addr, WLAN_MLME_SB_ID);
                 if (ni) {
                     ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                     return temp_osifp;
                 }
            }

            /* check for legacy clients */
            for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
                 temp_osifp = mldev->link_vifs[i];
                 if (!temp_osifp)
                     continue;
                 temp_vap = temp_osifp->os_if;
                 ni = ieee80211_find_node(temp_vap->iv_ic, mac_addr, WLAN_MLME_SB_ID);
                 if (ni) {
                     ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                     return temp_osifp;
                 }
            }
            return NULL;
        }
    }
}

static inline
struct ieee80211com *wlan_cfg80211_get_ic_ptr(struct wiphy *wiphy,
                                              struct wireless_dev *wdev,
                                              uint8_t link_id)
{
    struct cfg80211_context *cfg_ctx = NULL;
    struct ieee80211com *ic = NULL;
    osif_dev *osifp;
    wlan_if_t vap;

    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

    if (!cfg_ctx->is_mld_phy)  {
        ic = cfg_ctx->ic_list[0];
    } else {
        if (!wdev)
            return NULL;

        if (is_mld_phy_radio_command(wiphy, wdev->netdev)) {
            ic = wlan_cfg80211_find_ic_by_netdev_name(wiphy, wdev->netdev->name);
        } else {
            osifp = get_vap_device_context(wiphy, wdev->netdev, link_id);
            if (!osifp) {
                qdf_debug("osifp is NULL\n");
                return NULL;
            }
            vap = osifp->os_if;
            if (!vap) {
                qdf_err("NULL vap");
                return NULL;
            }
            ic = vap->iv_ic;
        }
    }
    return ic;
}

static inline
struct wireless_dev *get_cfg80211_notification_wdev(osif_dev *osifp)
{
    struct ieee80211vap *vap;
    struct net_device *mld_dev;

    if (!osifp)
        return NULL;

    vap = osifp->os_if;
    if (!vap)
        return NULL;

    if (vap->vif_type == VIF_TYPE_LEGACY ||
             vap->vif_type == VIF_TYPE_MLD_LEGACY_VAP) {
        return &osifp->iv_wdev;
    } else if (vap->vif_type == VIF_TYPE_MLD_LINK_VAP) {
        mld_dev = osif_get_mld_netdev(osifp);
        if (!mld_dev)
            return NULL;

        return mld_dev->ieee80211_ptr;
    } else {
        qdf_err("Invalid VAP type");
        return NULL;
    }
}

static inline
struct net_device *get_cfg80211_notification_ndev(osif_dev *osifp)
{
    struct ieee80211vap *vap;

    if (!osifp)
        return NULL;

    vap = osifp->os_if;
    if (!vap)
        return NULL;

    if (vap->vif_type == VIF_TYPE_LEGACY ||
             vap->vif_type == VIF_TYPE_MLD_LEGACY_VAP) {
        return osifp->netdev;
    } else if (vap->vif_type == VIF_TYPE_MLD_LINK_VAP) {
        if (!osifp->mldev)
            return NULL;

        return osifp->mldev->mld_dev;
    } else {
        qdf_err("Invalid vap type %d for notification_ndev", vap->vif_type);
        return NULL;
    }
}

static inline
osif_dev *get_vap_device_context_from_bssid(struct wiphy *wiphy,
                                            struct net_device *netdev,
                                            const u8 *bssid)
{
    struct cfg80211_context *cfg_ctx = NULL;
    struct osif_mldev *mldev = NULL;
    osif_dev *temp_osifp = NULL;
    struct ieee80211_node *ni;
    int i = 0;
    struct ieee80211vap *temp_vap = NULL;

    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);

    qdf_debug("External Auth param->bssid: %pM", bssid);
    if (!cfg_ctx->is_mld_phy) {
        return ath_netdev_priv(netdev);
    } else {
        mldev = (struct osif_mldev *)ath_netdev_priv(netdev);
        if (mldev->dev_type != OSIF_NETDEV_TYPE_MLO) {
            qdf_err("Not mld dev, return legacy vap osifp");
            return (osif_dev *)mldev;
        } else {
            if (!bssid) {
                qdf_err("BSSID is Null");
                return NULL;
            }

            for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
                 temp_osifp = mldev->link_vifs[i];
                 if (!temp_osifp) {
                     continue;
                 }

                 temp_vap = temp_osifp->os_if;
                 if (!temp_vap) {
                     continue;
                 }

                 ni = ieee80211_find_node(temp_vap->iv_ic, (u8 *)bssid, WLAN_MLME_SB_ID);
                 if (ni) {
                     ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                     return temp_osifp;
                 }
            }
            return NULL;
        }
    }
}
#else
static inline
struct ol_ath_softc_net80211 *get_radio_device_context(struct wiphy *wiphy,
                                                       struct net_device *netdev)
{
    return ath_netdev_priv(netdev);
}

static inline
osif_dev *get_assoc_vap_device_context(struct wiphy *wiphy,
                                       struct net_device *netdev)
{
    return ath_netdev_priv(netdev);
}

/* Legacy VAP or Link VAP */
static inline
osif_dev *get_vap_device_context(struct wiphy *wiphy,
                                 struct net_device *netdev,
                                 uint8_t link_id)
{
    return ath_netdev_priv(netdev);
}

static inline
osif_dev *get_vap_device_context_no_linkid(struct wiphy *wiphy,
                                           struct net_device *netdev,
                                           const u8 *mac_addr)
{
    return ath_netdev_priv(netdev);
}

static inline
osif_dev *get_vap_device_context_from_bssid(struct wiphy *wiphy,
                                            struct net_device *netdev,
                                            const u8 *bssid)
{
    return ath_netdev_priv(netdev);
}

static inline
struct ieee80211com *wlan_cfg80211_get_ic_ptr(struct wiphy *wiphy,
                                              struct wireless_dev *wdev,
                                              uint8_t link_id)
{
    struct cfg80211_context *cfg_ctx = NULL;
    struct ieee80211com *ic = NULL;

    cfg_ctx = (struct cfg80211_context *)wiphy_priv(wiphy);
    ic = cfg_ctx->ic;

    return ic;
}

static inline
struct wireless_dev *get_cfg80211_notification_wdev(osif_dev *osifp)
{
    return &osifp->iv_wdev;
}

static inline
struct net_device *get_cfg80211_notification_ndev(osif_dev *osifp)
{
    return osifp->netdev;
}
#endif

#ifdef CONFIG_SAWF
/* Structure to hold parsed information received from RM */
struct wlan_cfg80211_sawf_flow_deprioritize_params {
    bool is_mlo_peer;
    uint8_t peer_mac[QDF_MAC_ADDR_SIZE];
    uint8_t mld_mac[QDF_MAC_ADDR_SIZE];
    uint8_t msduq_id;
    uint8_t svc_id;
    uint8_t svc_type;
    uint16_t hw_link_id;
    uint32_t mark_metadata;
};

/* Structure to pass relevant information to connection manager */
struct qca_sawf_flow_deprioritize_params {
    uint8_t peer_mac[QDF_MAC_ADDR_SIZE];
    uint32_t mark_metadata;
    bool netdev_info_valid;
    uint32_t netdev_ifindex;
    uint8_t netdev_mac[QDF_MAC_ADDR_SIZE];
};
#endif

#if defined(WLAN_SUPPORT_SCS) && defined(SAWF_ADMISSION_CONTROL)
struct ieee80211_scs_adm_ctrl_info {
    uint8_t scsid;
    uint8_t status;
    uint8_t tid;
    uint8_t ac;
    uint32_t service_interval;
    uint32_t burst_size;
    uint32_t delay_bound;
    uint32_t min_data_rate;
};

struct ieee80211_scs_adm_ctrl_intf {
    bool is_mlo_peer;
    uint16_t hw_link_id;
    uint8_t peer_mac_addr[IEEE80211_ADDR_LEN];
    uint8_t peer_mld_mac_addr[IEEE80211_ADDR_LEN];
    uint8_t dialog_token;
    uint8_t num_scs_reqs;
    struct ieee80211_scs_adm_ctrl_info scs_info[IEEE80211_SCS_MAX_SIZE];
};

void ieee80211_process_scs_adm_ctrl_resp(struct ieee80211com *ic,
                                         struct ieee80211_scs_adm_ctrl_intf *intf);

void wlan_cfg80211_send_scs_adm_ctrl_req(struct ieee80211vap *vap,
                     struct ieee80211_scs_adm_ctrl_intf *scs_adm_ctrl_intf);
#endif
#endif /* _IEEE80211_CFG80211_H___ */
#endif /* UMAC_SUPPORT_CFG80211 */
