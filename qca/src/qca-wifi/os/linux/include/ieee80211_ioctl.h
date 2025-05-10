/*
 * Copyright (c) 2013-2014,2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2013-2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
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

#ifndef _NET80211_IEEE80211_IOCTL_H_
#define _NET80211_IEEE80211_IOCTL_H_

/*
 * IEEE 802.11 ioctls.
 */
#ifndef EXTERNAL_USE_ONLY
#include <_ieee80211.h>
#include "ieee80211.h"
#include "ieee80211_defines.h"
/* duplicate defination - to avoid including ieee80211_var.h */
#ifndef __ubicom32__
#define IEEE80211_ADDR_COPY(dst,src)    OS_MEMCPY(dst, src, IEEE80211_ADDR_LEN)
#else
#define IEEE80211_ADDR_COPY(dst,src)    OS_MACCPY(dst, src)
#endif
#define IEEE80211_KEY_XMIT      0x01    /* key used for xmit */
#define IEEE80211_KEY_RECV      0x02    /* key used for recv */
#ifndef __ubicom32__
#define IEEE80211_ADDR_EQ(a1,a2)        (OS_MEMCMP(a1, a2, IEEE80211_ADDR_LEN) == 0)
#else
#define IEEE80211_ADDR_EQ(a1,a2)        (OS_MACCMP(a1, a2) == 0)
#endif
#define IEEE80211_APPIE_MAX                  1024 /* max appie buffer size */
#define IEEE80211_KEY_GROUP     0x04    /* key used for WPA group operation */
#define IEEE80211_SCAN_MAX_SSID     10

/* Modify below definitions if changes are made to ieee80211_var.h
 * Duplicate definition to avoid including ieee80211_var.h
 * */
#if ATH_SUPPORT_AP_WDS_COMBO
#define IEEE80211_MAX_VAPS 16
#elif ATH_SUPPORT_WRAP
#define IEEE80211_MAX_VAPS 32
#elif ATH_PERF_PWR_OFFLOAD
#define IEEE80211_MAX_VAPS 17
#else
#define IEEE80211_MAX_VAPS 16
#endif

#include <umac_lmac_common.h>
#endif /* EXTERNAL_USE_ONLY */

/**
 * Considering a scan radio where channels of all bands are included on the
 * same pdev.
 * 2G max num channels : 14
 * 5G max num channels : 30 (including 5.9G)
 * 6G max num channels : 59
 * Total count: 103
 * IEEE80211_CHANINFO_MAX =
 * Total num of channels * sizeof(struct ieee80211_channel_info) / sizeof(__u32)
 * (103 * 56)/ 4 = 1442. Hence allocating 1500 as the max chan buffer size.
 */
#define IEEE80211_CHANINFO_MAX           1500 /* max Chaninfo buffer size */

#if QCA_AIRTIME_FAIRNESS
#include <wlan_atf_utils_defs.h>
#endif
#include <ext_ioctl_drv_if.h>
#ifdef WLAN_CFR_ENABLE
#include <wlan_cfr_public_structs.h>
#endif
#include <cfg80211_ven_cmd.h>

#if QCA_SUPPORT_SON
#include <wlan_son_ioctl.h>
#endif

#ifdef WLAN_WIFI_RADAR_ENABLE
#include <wlan_wifi_radar_public_structs.h>
#endif

/*
 * Macros used for Tr069 objects
 */
#define TR069MAXPOWERRANGE 30
#define TR69MINTXPOWER 1
#define TR69MAX_RATE_POWER 63
#define TR69SCANSTATEVARIABLESIZE 20
#define TR69_MAX_BUF_LEN    800

/* Macro used for tdma */
#define WLAN_MAX_AC   4

#if 0
/*
 * Per/node (station) statistics available when operating as an AP.
 */
struct ieee80211_nodestats {
	u_int32_t	ns_rx_data;		/* rx data frames */
	u_int32_t	ns_rx_mgmt;		/* rx management frames */
	u_int32_t	ns_rx_ctrl;		/* rx control frames */
	u_int32_t	ns_rx_ucast;		/* rx unicast frames */
	u_int32_t	ns_rx_mcast;		/* rx multi/broadcast frames */
	u_int64_t	ns_rx_bytes;		/* rx data count (bytes) */
	u_int64_t	ns_rx_beacons;		/* rx beacon frames */
	u_int32_t	ns_rx_proberesp;	/* rx probe response frames */

	u_int32_t	ns_rx_dup;		/* rx discard 'cuz dup */
	u_int32_t	ns_rx_noprivacy;	/* rx w/ wep but privacy off */
	u_int32_t	ns_rx_wepfail;		/* rx wep processing failed */
	u_int32_t	ns_rx_demicfail;	/* rx demic failed */
	u_int32_t	ns_rx_decap;		/* rx decapsulation failed */
	u_int32_t	ns_rx_defrag;		/* rx defragmentation failed */
	u_int32_t	ns_rx_disassoc;		/* rx disassociation */
	u_int32_t	ns_rx_deauth;		/* rx deauthentication */
    u_int32_t   ns_rx_action;       /* rx action */
	u_int32_t	ns_rx_decryptcrc;	/* rx decrypt failed on crc */
	u_int32_t	ns_rx_unauth;		/* rx on unauthorized port */
	u_int32_t	ns_rx_unencrypted;	/* rx unecrypted w/ privacy */

	u_int32_t	ns_tx_data;		/* tx data frames */
	u_int32_t	ns_tx_mgmt;		/* tx management frames */
	u_int32_t	ns_tx_ucast;		/* tx unicast frames */
	u_int32_t	ns_tx_mcast;		/* tx multi/broadcast frames */
	u_int64_t	ns_tx_bytes;		/* tx data count (bytes) */
	u_int32_t	ns_tx_probereq;		/* tx probe request frames */
	u_int32_t	ns_tx_uapsd;		/* tx on uapsd queue */

	u_int32_t	ns_tx_novlantag;	/* tx discard 'cuz no tag */
	u_int32_t	ns_tx_vlanmismatch;	/* tx discard 'cuz bad tag */
#ifdef ATH_SUPPORT_IQUE
	u_int32_t	ns_tx_dropblock;	/* tx discard 'cuz headline block */
#endif

	u_int32_t	ns_tx_eosplost;		/* uapsd EOSP retried out */

	u_int32_t	ns_ps_discard;		/* ps discard 'cuz of age */

	u_int32_t	ns_uapsd_triggers;	     /* uapsd triggers */
	u_int32_t	ns_uapsd_duptriggers;	 /* uapsd duplicate triggers */
	u_int32_t	ns_uapsd_ignoretriggers; /* uapsd duplicate triggers */
	u_int32_t	ns_uapsd_active;         /* uapsd duplicate triggers */
	u_int32_t	ns_uapsd_triggerenabled; /* uapsd duplicate triggers */

	/* MIB-related state */
	u_int32_t	ns_tx_assoc;		/* [re]associations */
	u_int32_t	ns_tx_assoc_fail;	/* [re]association failures */
	u_int32_t	ns_tx_auth;		/* [re]authentications */
	u_int32_t	ns_tx_auth_fail;	/* [re]authentication failures*/
	u_int32_t	ns_tx_deauth;		/* deauthentications */
	u_int32_t	ns_tx_deauth_code;	/* last deauth reason */
	u_int32_t	ns_tx_disassoc;		/* disassociations */
	u_int32_t	ns_tx_disassoc_code;	/* last disassociation reason */
	u_int32_t	ns_psq_drops;		/* power save queue drops */
};

/*
 * Summary statistics.
 */
struct ieee80211_stats {
	u_int32_t	is_rx_badversion;	/* rx frame with bad version */
	u_int32_t	is_rx_tooshort;		/* rx frame too short */
	u_int32_t	is_rx_wrongbss;		/* rx from wrong bssid */
	u_int32_t	is_rx_dup;		/* rx discard 'cuz dup */
	u_int32_t	is_rx_wrongdir;		/* rx w/ wrong direction */
	u_int32_t	is_rx_mcastecho;	/* rx discard 'cuz mcast echo */
	u_int32_t	is_rx_notassoc;		/* rx discard 'cuz sta !assoc */
	u_int32_t	is_rx_noprivacy;	/* rx w/ wep but privacy off */
	u_int32_t	is_rx_unencrypted;	/* rx w/o wep and privacy on */
	u_int32_t	is_rx_wepfail;		/* rx wep processing failed */
	u_int32_t	is_rx_decap;		/* rx decapsulation failed */
	u_int32_t	is_rx_mgtdiscard;	/* rx discard mgt frames */
	u_int32_t	is_rx_ctl;		/* rx discard ctrl frames */
	u_int32_t	is_rx_beacon;		/* rx beacon frames */
	u_int32_t	is_rx_rstoobig;		/* rx rate set truncated */
	u_int32_t	is_rx_elem_missing;	/* rx required element missing*/
	u_int32_t	is_rx_elem_toobig;	/* rx element too big */
	u_int32_t	is_rx_elem_toosmall;	/* rx element too small */
	u_int32_t	is_rx_elem_unknown;	/* rx element unknown */
	u_int32_t	is_rx_badchan;		/* rx frame w/ invalid chan */
	u_int32_t	is_rx_chanmismatch;	/* rx frame chan mismatch */
	u_int32_t	is_rx_nodealloc;	/* rx frame dropped */
	u_int32_t	is_rx_ssidmismatch;	/* rx frame ssid mismatch  */
	u_int32_t	is_rx_auth_unsupported;	/* rx w/ unsupported auth alg */
	u_int32_t	is_rx_auth_fail;	/* rx sta auth failure */
	u_int32_t	is_rx_auth_countermeasures;/* rx auth discard 'cuz CM */
	u_int32_t	is_rx_assoc_bss;	/* rx assoc from wrong bssid */
	u_int32_t	is_rx_assoc_notauth;	/* rx assoc w/o auth */
	u_int32_t	is_rx_assoc_capmismatch;/* rx assoc w/ cap mismatch */
	u_int32_t	is_rx_assoc_norate;	/* rx assoc w/ no rate match */
	u_int32_t	is_rx_assoc_badwpaie;	/* rx assoc w/ bad WPA IE */
	u_int32_t	is_rx_deauth;		/* rx deauthentication */
	u_int32_t	is_rx_disassoc;		/* rx disassociation */
    u_int32_t   is_rx_action;       /* rx action mgt */
	u_int32_t	is_rx_badsubtype;	/* rx frame w/ unknown subtype*/
	u_int32_t	is_rx_nobuf;		/* rx failed for lack of buf */
	u_int32_t	is_rx_decryptcrc;	/* rx decrypt failed on crc */
	u_int32_t	is_rx_ahdemo_mgt;	/* rx discard ahdemo mgt frame*/
	u_int32_t	is_rx_bad_auth;		/* rx bad auth request */
	u_int32_t	is_rx_unauth;		/* rx on unauthorized port */
	u_int32_t	is_rx_badkeyid;		/* rx w/ incorrect keyid */
	u_int32_t	is_rx_ccmpreplay;	/* rx seq# violation (CCMP) */
	u_int32_t	is_rx_ccmpformat;	/* rx format bad (CCMP) */
	u_int32_t	is_rx_ccmpmic;		/* rx MIC check failed (CCMP) */
	u_int32_t	is_rx_tkipreplay;	/* rx seq# violation (TKIP) */
	u_int32_t	is_rx_tkipformat;	/* rx format bad (TKIP) */
	u_int32_t	is_rx_tkipmic;		/* rx MIC check failed (TKIP) */
	u_int32_t	is_rx_tkipicv;		/* rx ICV check failed (TKIP) */
	u_int32_t	is_rx_badcipher;	/* rx failed 'cuz key type */
	u_int32_t	is_rx_nocipherctx;	/* rx failed 'cuz key !setup */
	u_int32_t	is_rx_acl;		/* rx discard 'cuz acl policy */
	u_int32_t	is_rx_ffcnt;		/* rx fast frames */
	u_int32_t	is_rx_badathtnl;   	/* driver key alloc failed */
	u_int32_t	is_tx_nobuf;		/* tx failed for lack of buf */
	u_int32_t	is_tx_nonode;		/* tx failed for no node */
	u_int32_t	is_tx_unknownmgt;	/* tx of unknown mgt frame */
	u_int32_t	is_tx_badcipher;	/* tx failed 'cuz key type */
	u_int32_t	is_tx_nodefkey;		/* tx failed 'cuz no defkey */
	u_int32_t	is_tx_noheadroom;	/* tx failed 'cuz no space */
	u_int32_t	is_tx_ffokcnt;		/* tx fast frames sent success */
	u_int32_t	is_tx_fferrcnt;		/* tx fast frames sent success */
	u_int32_t	is_scan_active;		/* active scans started */
	u_int32_t	is_scan_passive;	/* passive scans started */
	u_int32_t	is_node_timeout;	/* nodes timed out inactivity */
	u_int32_t	is_crypto_nomem;	/* no memory for crypto ctx */
	u_int32_t	is_crypto_tkip;		/* tkip crypto done in s/w */
	u_int32_t	is_crypto_tkipenmic;	/* tkip en-MIC done in s/w */
	u_int32_t	is_crypto_tkipdemic;	/* tkip de-MIC done in s/w */
	u_int32_t	is_crypto_tkipcm;	/* tkip counter measures */
	u_int32_t	is_crypto_ccmp;		/* ccmp crypto done in s/w */
	u_int32_t	is_crypto_wep;		/* wep crypto done in s/w */
	u_int32_t	is_crypto_setkey_cipher;/* cipher rejected key */
	u_int32_t	is_crypto_setkey_nokey;	/* no key index for setkey */
	u_int32_t	is_crypto_delkey;	/* driver key delete failed */
	u_int32_t	is_crypto_badcipher;	/* unknown cipher */
	u_int32_t	is_crypto_nocipher;	/* cipher not available */
	u_int32_t	is_crypto_attachfail;	/* cipher attach failed */
	u_int32_t	is_crypto_swfallback;	/* cipher fallback to s/w */
	u_int32_t	is_crypto_keyfail;	/* driver key alloc failed */
	u_int32_t	is_crypto_enmicfail;	/* en-MIC failed */
	u_int32_t	is_ibss_capmismatch;	/* merge failed-cap mismatch */
	u_int32_t	is_ibss_norate;		/* merge failed-rate mismatch */
	u_int32_t	is_ps_unassoc;		/* ps-poll for unassoc. sta */
	u_int32_t	is_ps_badaid;		/* ps-poll w/ incorrect aid */
	u_int32_t	is_ps_qempty;		/* ps-poll w/ nothing to send */
};
#endif

/*
 * Max size of optional information elements.  We artificially
 * constrain this; it's limited only by the max frame size (and
 * the max parameter size of the wireless extensions).
 */
#define	IEEE80211_MAX_OPT_IE	512
#define	IEEE80211_MAX_WSC_IE	256

/*
 * WPA/RSN get/set key request.  Specify the key/cipher
 * type and whether the key is to be used for sending and/or
 * receiving.  The key index should be set only when working
 * with global keys (use IEEE80211_KEYIX_NONE for ``no index'').
 * Otherwise a unicast/pairwise key is specified by the bssid
 * (on a station) or mac address (on an ap).  They key length
 * must include any MIC key data; otherwise it should be no
 more than IEEE80211_KEYBUF_SIZE.
 */
struct ieee80211req_key {
	u_int8_t	ik_type;	/* key/cipher type */
	u_int8_t	ik_pad;
	u_int16_t	ik_keyix;	/* key index */
	u_int8_t	ik_keylen;	/* key length in bytes */
	u_int16_t	ik_flags;
/* NB: IEEE80211_KEY_XMIT and IEEE80211_KEY_RECV defined elsewhere */
#define	IEEE80211_KEY_DEFAULT	0x80	/* default xmit key */
	u_int8_t	ik_macaddr[IEEE80211_ADDR_LEN];
	u_int64_t	ik_keyrsc;	/* key receive sequence counter */
	u_int64_t	ik_keytsc;	/* key transmit sequence counter */
	u_int8_t	ik_keydata[IEEE80211_KEYBUF_SIZE+IEEE80211_MICBUF_SIZE];
        u_int8_t        ik_txiv[IEEE80211_WAPI_IV_SIZE];      /* WAPI key tx iv */
        u_int8_t        ik_recviv[IEEE80211_WAPI_IV_SIZE];    /* WAPI key rx iv */
};

/*
 * Delete a key either by index or address.  Set the index
 * to IEEE80211_KEYIX_NONE when deleting a unicast key.
 */
struct ieee80211req_del_key {
	u_int8_t	idk_keyix;	/* key index */
	u_int8_t	idk_macaddr[IEEE80211_ADDR_LEN];
};

/*
 * MLME state manipulation request.  IEEE80211_MLME_ASSOC
 * only makes sense when operating as a station.  The other
 * requests can be used when operating as a station or an
 * ap (to effect a station).
 */
struct ieee80211req_mlme {
	u_int8_t	im_op;		/* operation to perform */
#define	IEEE80211_MLME_ASSOC		1	/* associate station */
#define	IEEE80211_MLME_DISASSOC		2	/* disassociate station */
#define	IEEE80211_MLME_DEAUTH		3	/* deauthenticate station */
#define	IEEE80211_MLME_AUTHORIZE	4	/* authorize station */
#define	IEEE80211_MLME_UNAUTHORIZE	5	/* unauthorize station */
#define	IEEE80211_MLME_STOP_BSS		6	/* stop bss */
#define IEEE80211_MLME_CLEAR_STATS	7	/* clear station statistic */
#define IEEE80211_MLME_AUTH	        8	/* auth resp to station */
#define IEEE80211_MLME_REASSOC	        9	/* reassoc to station */
#define	IEEE80211_MLME_AUTH_FILS        10	/* AUTH - when FILS enabled */
	u_int8_t	im_ssid_len;	/* length of optional ssid */
	u_int16_t	im_reason;	/* 802.11 reason code */
	u_int16_t	im_seq;	        /* seq for auth */
	u_int8_t	im_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	im_ssid[IEEE80211_NWID_LEN];
	u_int8_t        im_optie[IEEE80211_MAX_OPT_IE];
	u_int16_t       im_optie_len;
	struct          ieee80211req_fils_aad  fils_aad;
};

/*
 * request to add traffic stream for an associated station.
 */
struct ieee80211req_ts {
	u_int8_t    macaddr[IEEE80211_ADDR_LEN];
	u_int8_t    tspec_ie[IEEE80211_MAX_OPT_IE];
	u_int8_t    tspec_ielen;
	u_int8_t    res;
};

/*
 * Net802.11 scan request
 *
 */
enum {
    IEEE80211_SCANREQ_BG        = 1,    /*start the bg scan if vap is connected else fg scan */
    IEEE80211_SCANREQ_FORCE    = 2,    /*start the fg scan */
    IEEE80211_SCANREQ_STOP        = 3,    /*cancel any ongoing scanning*/
    IEEE80211_SCANREQ_PAUSE      = 4,    /*pause any ongoing scanning*/
    IEEE80211_SCANREQ_RESUME     = 5,    /*resume any ongoing scanning*/
};

/*
 * Set the active channel list.  Note this list is
 * intersected with the available channel list in
 * calculating the set of channels actually used in
 * scanning.
 */
struct ieee80211req_chanlist {
	u_int8_t	ic_channels[IEEE80211_CHAN_BYTES];
};

/*
 * Get the active channel list info.
 */
struct ieee80211req_chaninfo {
        uint32_t ic_nchans;
        struct ieee80211_ath_channel ic_chans[IEEE80211_CHAN_MAX];
};

/*
 * Regulatory EIRP/PSD powers for valid 6GHz AP power types.
 */
struct reg_power_val {
    uint8_t psd_flag;
    uint16_t psd_eirp;
    uint32_t tx_power;
};

#define MAX_AP_PWR_TYPE 3

struct ieee80211_channel_info {
    uint8_t ieee;
    uint16_t freq;
    uint64_t flags;
    uint32_t flags_ext;
    uint8_t vhtop_ch_num_seg1;
    uint8_t vhtop_ch_num_seg2;
    struct reg_power_val reg_powers[MAX_AP_PWR_TYPE];
    uint8_t supp_power_modes;
};

#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_6GHZ (1 << 4)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_A (1 << 5)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_B (1 << 6)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_G (1 << 7)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_PUREG (1 << 8)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_FHSS (1 << 9)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EXT_PSC (1 << 10)
#ifdef WLAN_FEATURE_11BE
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT20 (1 << 11)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40PLUS (1 << 12)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT40MINUS (1 << 13)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT80 (1 << 14)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT160 (1 << 15)
#define QCA_WLAN_VENDOR_CHANNEL_PROP_FLAG_EHT320 (1 << 16)
#endif /* WLAN_FEATURE_11BE*/

#define VENDOR_CHAN_FLAG2(_flag)  \
        ((uint64_t)(_flag) << 32)

struct ieee80211req_channel_list {
    uint32_t nchans;
    struct ieee80211_channel_info chans[IEEE80211_CHAN_MAX];
};

#define MAX_NUM_SEG2_CENTERS 3

#define INDEX_160_SEG2_CENTER 0
#define INDEX_320_SEG2_CENTER1 1
#define INDEX_320_SEG2_CENTER2 2
#define INVALID_SEG2_CENTER_INDEX 0xF

struct ieee80211_channel_params {
    uint8_t ieee;
    uint16_t freq;
    uint64_t flags;
    uint32_t flags_ext;
    uint8_t vhtop_ch_num_seg1;
    uint8_t vhtop_ch_num_seg2[MAX_NUM_SEG2_CENTERS];
    struct reg_power_val reg_powers[MAX_AP_PWR_TYPE];
    uint8_t supp_power_modes;
};

struct ieee80211_channel_list_info {
    uint32_t nchans;
    struct ieee80211_channel_params chans[IEEE80211_CHAN_MAX];
};

/*
 * Get the active channel list info along with dfs channel states.
 */
struct ieee80211req_chaninfo_full {
	struct ieee80211req_chaninfo req_chan_info;
	enum wlan_channel_dfs_state dfs_chan_state_arr[NUM_DFS_CHANS];
};

typedef struct beacon_rssi_info {
    struct beacon_rssi_stats {
        u_int32_t   ni_last_bcn_snr;
        u_int32_t   ni_last_bcn_age;
        u_int32_t   ni_last_bcn_cnt;
        u_int64_t   ni_last_bcn_jiffies;
    } stats;
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
} beacon_rssi_info;

typedef struct ack_rssi_info {
    struct ack_rssi_stats {
        u_int32_t   ni_last_ack_rssi;
        u_int32_t   ni_last_ack_age;
        u_int32_t   ni_last_ack_cnt;
        u_int64_t   ni_last_ack_jiffies;
    } stats;
    u_int8_t macaddr[IEEE80211_ADDR_LEN];
} ack_rssi_info;

#if QCA_MANUAL_TRIGGERED_ULOFDMA
#define MAX_UL_OFDMA_USER 8
#define MAX_UL_OFDMA_TRIGGER 8
#define MAX_UL_OFDMA_PREF_AC 3
#define MIN_UL_OFDMA_LEN 120
#define MAX_UL_OFDMA_LEN 3098003
#define MAX_UL_OFDMA_EV_TIMEOUT 7000    /* timeout in ms */

/**
 * struct ul_ofdma_trig_info - manual trigger ul_ofdma info
 * @ul_ofdma_trig_length: trigger length
 * @ul_ofdma_trig_nss: trigger nss value
 * @ul_ofdma_trig_mcs: trigger mcs value
 * @ul_ofdma_trig_gi_ltf: trigger gi_ltf value
 * @reserved: reserved bits
 * @ul_ofdma_trig_target_rssi: trigger target rssi value
 * @ul_ofdma_trig_num_peers: num peer
 * @ul_ofdma_num_triggers: num of triggers
 * @ul_ofdma_trig_preferred_ac: Preferred AC
 * @macaddr: Peer macaddr
 */
typedef struct ul_ofdma_trig_info {
    uint32_t ul_ofdma_trig_length:12,
             ul_ofdma_trig_nss:4,
             ul_ofdma_trig_mcs:4,
             ul_ofdma_trig_gi_ltf:2,
             reserved:10;
    int8_t ul_ofdma_trig_target_rssi;
    uint8_t ul_ofdma_trig_num_peers;
    uint8_t ul_ofdma_num_triggers;
    uint8_t ul_ofdma_trig_preferred_ac;
    uint8_t macaddr[MAX_UL_OFDMA_USER][IEEE80211_ADDR_LEN];
} ul_ofdma_trig_info;
#endif

/*
* Ressource request type from app
*/
enum {
    IEEE80211_RESREQ_ADDTS = 0,
    IEEE80211_RESREQ_ADDNODE,
};
/*
 * Resource request for adding Traffic stream
 */
struct ieee80211req_res_addts {
	u_int8_t	tspecie[IEEE80211_MAX_OPT_IE];
	u_int8_t	status;
};
/*
 * Resource request for adding station node
 */
struct ieee80211req_res_addnode {
	u_int8_t	auth_alg;
};
/*
 * Resource request from app
 */
struct ieee80211req_res {
	u_int8_t	macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	type;
        union {
            struct ieee80211req_res_addts addts;
            struct ieee80211req_res_addnode addnode;
        } u;
};

/*
 * CSA deauth mode types
 */
enum csa_deauth_modes {
    CSA_DEAUTH_MODE_DISABLE = 0,
    CSA_DEAUTH_MODE_UNICAST = 1,
    CSA_DEAUTH_MODE_BROADCAST = 2,
};

/*
 * Retrieve the WPA/RSN information element for an associated station.
 */
struct ieee80211req_wpaie {
	u_int8_t	wpa_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	wpa_ie[IEEE80211_MAX_OPT_IE];
	u_int8_t    rsn_ie[IEEE80211_MAX_OPT_IE];
	u_int8_t    wps_ie[IEEE80211_MAX_OPT_IE];
};

/*
 * Retrieve the WSC information element for an associated station.
 */
struct ieee80211req_wscie {
	u_int8_t	wsc_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	wsc_ie[IEEE80211_MAX_WSC_IE];
};


/*
 * Retrieve per-node statistics.
 */
struct ieee80211req_sta_stats {
	union {
		/* NB: explicitly force 64-bit alignment */
		u_int8_t	macaddr[IEEE80211_ADDR_LEN];
		u_int64_t	pad;
	} is_u;
	struct ieee80211_nodestats is_stats;
};

enum {
	IEEE80211_STA_OPMODE_NORMAL,
	IEEE80211_STA_OPMODE_XR
};

/*
 * Retrieve per-station information; to retrieve all
 * specify a mac address of ff:ff:ff:ff:ff:ff.
 */
struct ieee80211req_sta_req {
	union {
		/* NB: explicitly force 64-bit alignment */
		u_int8_t	macaddr[IEEE80211_ADDR_LEN];
		u_int64_t	pad;
	} is_u;
	struct ieee80211req_sta_info info[1];	/* variable length */
};

/*
 * Get/set per-station tx power cap.
 */
struct ieee80211req_sta_txpow {
	u_int8_t	it_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	it_txpow;
};

/*
 * Wlan Latency Parameters
 */
typedef struct wlan_latency_info {
    uint32_t tid;
    uint32_t dl_ul_enable;;
    uint32_t service_interval;
    uint32_t burst_size;
    uint32_t burst_size_add_or_sub;
    uint8_t  peer_mac[IEEE80211_ADDR_LEN];
} wlan_latency_info_t;

/*
 * WME parameters are set and return using i_val and i_len.
 * i_val holds the value itself.  i_len specifies the AC
 * and, as appropriate, then high bit specifies whether the
 * operation is to be applied to the BSS or ourself.
 */
#define	IEEE80211_WMEPARAM_SELF	0x0000		/* parameter applies to self */
#define	IEEE80211_WMEPARAM_BSS	0x8000		/* parameter applies to BSS */
#define	IEEE80211_WMEPARAM_VAL	0x7fff		/* parameter value */

/*
 * Scan result data returned for IEEE80211_IOC_SCAN_RESULTS.
 */
struct ieee80211req_scan_result {
	u_int16_t	isr_len;		/* length (mult of 4) */
	u_int16_t	isr_freq;		/* MHz */
	u_int32_t	isr_flags;		/* channel flags */
	u_int8_t	isr_noise;
	u_int8_t	isr_rssi;
	u_int8_t	isr_intval;		/* beacon interval */
	u_int16_t	isr_capinfo;		/* capabilities */
	u_int8_t	isr_erp;		/* ERP element */
	u_int8_t	isr_bssid[IEEE80211_ADDR_LEN];
	u_int8_t	isr_nrates;
	u_int8_t	isr_rates[IEEE80211_RATE_MAXSIZE];
	u_int8_t	isr_ssid_len;		/* SSID length */
	u_int16_t	isr_ie_len;		/* IE length */
	u_int8_t	isr_pad[4];
	/* variable length SSID followed by IE data */
};

/* Options for Mcast Enhancement */
enum {
		IEEE80211_ME_DISABLE =	0,
		IEEE80211_ME_TUNNELING =	1,
		IEEE80211_ME_TRANSLATE =	2
};

/* Options for requesting nl reply */
enum {
		DBGREQ_REPLY_IS_NOT_REQUIRED =	0,
		DBGREQ_REPLY_IS_REQUIRED     =	1,
};


/*
 * athdbg request
 */
enum {
    IEEE80211_DBGREQ_SENDADDBA     =	0,
    IEEE80211_DBGREQ_SENDDELBA     =	1,
    IEEE80211_DBGREQ_SETADDBARESP  =	2,
    IEEE80211_DBGREQ_GETADDBASTATS =	3,
    IEEE80211_DBGREQ_SENDBCNRPT    =	4, /* beacon report request */
    IEEE80211_DBGREQ_SENDTSMRPT    =	5, /* traffic stream measurement report */
    IEEE80211_DBGREQ_SENDNEIGRPT   =	6, /* neigbor report */
    IEEE80211_DBGREQ_SENDLMREQ     =	7, /* link measurement request */
    IEEE80211_DBGREQ_SENDBSTMREQ   =	8, /* bss transition management request */
    IEEE80211_DBGREQ_SENDCHLOADREQ =    9, /* bss channel load  request */
    IEEE80211_DBGREQ_SENDSTASTATSREQ =  10, /* sta stats request */
    IEEE80211_DBGREQ_SENDNHIST     =    11, /* Noise histogram request */
    IEEE80211_DBGREQ_SENDDELTS     =	12, /* delete TSPEC */
    IEEE80211_DBGREQ_SENDADDTSREQ  =	13, /* add TSPEC */
    IEEE80211_DBGREQ_SENDLCIREQ    =    14, /* Location config info request */
    IEEE80211_DBGREQ_GETRRMSTATS   =    15, /* RRM stats */
    IEEE80211_DBGREQ_SENDFRMREQ    =    16, /* RRM Frame request */
    IEEE80211_DBGREQ_GETBCNRPT     =    17, /* GET BCN RPT */
    IEEE80211_DBGREQ_SENDSINGLEAMSDU=   18, /* Sends single VHT MPDU AMSDUs */
    IEEE80211_DBGREQ_GETRRSSI	   =	19, /* GET the Inst RSSI */
    IEEE80211_DBGREQ_GETACSREPORT  =	20, /* GET the ACS report with SNR in neighbor report*/
    IEEE80211_DBGREQ_SETACSUSERCHANLIST  =    21, /* SET ch list for acs reporting  */
    IEEE80211_DBGREQ_GETACSUSERCHANLIST  =    22, /* GET ch list used in acs reporting */
    IEEE80211_DBGREQ_BLOCK_ACS_CHANNEL	 =    23, /* Block acs for these channels */
    IEEE80211_DBGREQ_TR069  	         =    24, /* to be used for tr069 */
    IEEE80211_DBGREQ_CHMASKPERSTA        =    25, /* to be used for chainmask per sta */
    IEEE80211_DBGREQ_FIPS		   = 26, /* to be used for setting fips*/
    IEEE80211_DBGREQ_FW_TEST	   = 27, /* to be used for firmware testing*/
    IEEE80211_DBGREQ_SETQOSMAPCONF       =    28, /* set QoS map configuration */
    IEEE80211_DBGREQ_INITRTT3       = 37, /* to test RTT3 feature*/
    IEEE80211_DBGREQ_SET_ANTENNA_SWITCH       = 38, /* Dynamic Antenna Selection */
    IEEE80211_DBGREQ_SETSUSERCTRLTBL          = 39, /* set User defined control table*/
    IEEE80211_DBGREQ_OFFCHAN_TX               = 40, /* Offchan tx*/
    IEEE80211_DBGREQ_GET_RRM_STA_LIST             = 43, /* to get list of connected rrm capable station */
    /* bss transition management request, targetted to a particular AP (or set of APs) */
    IEEE80211_DBGREQ_SENDBSTMREQ_TARGET           = 44,
    IEEE80211_DBGREQ_SENDBSTMREQ_DISASSOC_TARGET           = 45,
#if QCA_LTEU_SUPPORT
    IEEE80211_DBGREQ_MU_SCAN                      = 47, /* do a MU scan */
    IEEE80211_DBGREQ_LTEU_CFG                     = 48, /* LTEu specific configuration */
    IEEE80211_DBGREQ_AP_SCAN                      = 49, /* do a AP scan */
#endif
    IEEE80211_DBGREQ_ATF_DEBUG_SIZE               = 50, /* Set the ATF history size */
    IEEE80211_DBGREQ_ATF_DUMP_DEBUG               = 51, /* Dump the ATF history */
#if QCA_LTEU_SUPPORT
    IEEE80211_DBGREQ_SCAN_REPEAT_PROBE_TIME       = 52, /* scan probe time, part of scan params */
    IEEE80211_DBGREQ_SCAN_REST_TIME               = 53, /* scan rest time, part of scan params */
    IEEE80211_DBGREQ_SCAN_IDLE_TIME               = 54, /* scan idle time, part of scan params */
    IEEE80211_DBGREQ_SCAN_PROBE_DELAY             = 55, /* scan probe delay, part of scan params */
    IEEE80211_DBGREQ_MU_DELAY                     = 56, /* delay between channel change and MU start (for non-gpio) */
    IEEE80211_DBGREQ_WIFI_TX_POWER                = 57, /* assumed tx power of wifi sta */
#endif
    IEEE80211_DBGREQ_CHAN_LIST                    =60,
    IEEE80211_DBGREQ_MBO_BSSIDPREF                = 61,
#if UMAC_SUPPORT_VI_DBG
    IEEE80211_DBGREQ_VOW_DEBUG_PARAM        	  = 62,
    IEEE80211_DBGREQ_VOW_DEBUG_PARAM_PERSTREAM	  = 63,
#endif
#if QCA_LTEU_SUPPORT
    IEEE80211_DBGREQ_SCAN_PROBE_SPACE_INTERVAL     = 64,
#endif
    IEEE80211_DBGREQ_ASSOC_WATERMARK_TIME         = 65,  /* Get the date when the max number of devices has been associated crossing the threshold */
    IEEE80211_DBGREQ_DISPLAY_TRAFFIC_STATISTICS   = 66, /* Display the traffic statistics of each connected STA */
    IEEE80211_DBGREQ_ATF_DUMP_NODESTATE           = 67,
    IEEE80211_DBGREQ_FW_UNIT_TEST		  = 71, /* Used by Fw Unit test from Lithium family */
    IEEE80211_DBGREQ_DPTRACE                      = 72,	/* set dp_trace parameters */
    IEEE80211_DBGREQ_COEX_CFG                     = 73, /* coex configuration */
    IEEE80211_DBGREQ_SET_SOFTBLOCKING             = 74, /* set softblocking flag of a STA */
    IEEE80211_DBGREQ_GET_SOFTBLOCKING             = 75, /* get softblocking flag of a STA */
    IEEE80211_DBGREQ_REFUSEALLADDBAS              = 76, /* refuse all incoming ADDBA REQs */
    IEEE80211_DBGREQ_GET_BTM_STA_LIST             = 80, /* to get list of connected btm capable stat */
    IEEE80211_DBGREQ_TWT_ADD_DIALOG               = 81,
    IEEE80211_DBGREQ_TWT_DEL_DIALOG               = 82,
    IEEE80211_DBGREQ_TWT_PAUSE_DIALOG             = 83,
    IEEE80211_DBGREQ_TWT_RESUME_DIALOG            = 84,
#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
    IEEE80211_DBGREQ_SETPRIMARY_ALLOWED_CHANLIST  = 86, /* set primary allowed channel list */
    IEEE80211_DBGREQ_GETPRIMARY_ALLOWED_CHANLIST  = 87, /* get primary allowed channel list */
#endif
    IEEE80211_DBGREQ_ATF_GET_GROUP_SUBGROUP       = 89, /* Get group and subgroup list */
    IEEE80211_DBGREQ_BSTEERING_SETINNETWORK_2G    = 90, /* set 2.4G innetwork inforamtion in SON module */
    IEEE80211_DBGREQ_BSTEERING_GETINNETWORK_2G    = 91, /* get 2.4G innetwork inforamtion from SON module */
    IEEE80211_DBGREQ_BSSCOLOR_DOT11_COLOR_COLLISION_AP_PERIOD = 92, /* override BSS Color Collision AP period */
    IEEE80211_DBGREQ_BSSCOLOR_DOT11_COLOR_COLLISION_CHG_COUNT = 93, /* override BSS Color Change Announcement count */
    IEEE80211_DBGREQ_SENDCCA_REQ                  = 95, /* Clear Channel Assesment (CCA) request */
    IEEE80211_DBGREQ_SENDRPIHIST                  = 96, /* Received Power Indicator histogram request */
    IEEE80211_DBGREQ_PEERNSS                      = 97, /* Set peer NSS */
    IEEE80211_DBGREQ_GET_SURVEY_STATS             = 98, /* Get channel survey stats */
    IEEE80211_DBGREQ_RESET_SURVEY_STATS           = 99, /* Reset channel survey stats */
    IEEE80211_DBGREQ_OFFCHAN_RX                   = 100, /* Offchan rx */
    IEEE80211_DBGREQ_LAST_BEACON_RSSI             = 102, /* Get last beacon RSSI */
    IEEE80211_DBGREQ_LAST_ACK_RSSI                = 103, /* Get last beacon RSSI */
    IEEE80211_DBGREQ_ACL_SET_CLI_PARAMS           = 106, /* Set band steering per-client parameters */
    IEEE80211_DBGREQ_ACL_GET_CLI_PARAMS           = 107, /* Get band steering per-client parameters */
#if QLD
    IEEE80211_DBGREQ_GET_QLD_DUMP_TABLE           = 109, /* Request QLD table from driver */
#endif
    IEEE80211_DBGREQ_SEND_CUSTOM_NEIGRPT          = 111, /* Send custom neighbor report response */
    IEEE80211_DBGREQ_FAKE_MGMT_RX                 = 112,
    IEEE80211_DBGREQ_FRAME_INJECTOR               = 113, /* Enable Injector frame */
    IEEE80211_DBGREQ_HMWDS_AST_ADD_STATUS         = 115, /*  HMWDS ast add stats */
    IEEE80211_DBGREQ_SENDMSCSRESP                 = 116, /* Send MSCS Response */
    IEEE80211_DBGREQ_MESH_SET_GET_CONFIG          = 117, /*  Mesh config set and get */
    IEEE80211_DBGREQ_ADD_TPE                      = 118, /* Add TPE IE */
    IEEE80211_DBGREQ_DEL_TPE                      = 119, /* Delete TPE IE */
#ifdef WLAN_SUPPORT_BCAST_TWT
    IEEE80211_DBGREQ_TWT_BTWT_INVITE_STA          = 120, /* Invite STA to Broadcast TWT session */
    IEEE80211_DBGREQ_TWT_BTWT_REMOVE_STA          = 121, /* Remove STA from Broadcast TWT session */
#endif
    IEEE80211_DBGREQ_PEER_LATENCY_PARAM_CONFIG    = 122, /* Config peer latency parameters */
    IEEE80211_DBGREQ_GET_SCAN_CHAN_UTIL           = 123,
    IEEE80211_DBGREQ_ADD_DSCPACTION_POLICY        = 124, /* Add DSCP Action policy */
    IEEE80211_DBGREQ_DEL_DSCPACTION_POLICY        = 125, /* Deletes DSCP Action policy */
    IEEE80211_DBGREQ_SEND_DSCPACTION_POLICY       = 126, /* Send DSCP Action request */
#ifdef QCA_SUPPORT_SCAN_SPCL_VAP_STATS
    IEEE80211_DBGREQ_GET_SCAN_SPCL_VAP_STATS      = 127, /* Get spcl vap stats */
#endif
    IEEE80211_DBGREQ_GETACSBLOCKCHANLIST          = 128, /* GET Block ch list used in acs reporting */
    IEEE80211_DBGREQ_RETAINACSBLOCKCHANLIST       = 129, /* Retain acs blocked channels across wifi down/up */
    IEEE80211_DBGREQ_UNBLOCK_ACS_CHANNEL          = 130, /* Unblock ACS channel from block list */
    IEEE80211_DBGREQ_WFA_TEST_CMD                 = 131, /* WFA test cmd */
    IEEE80211_DBGREQ_SENDSCSRESP                 = 132, /* Send SCS Response */
    IEEE80211_DBGREQ_FIPS_EXTEND                  = 133,
    IEEE80211_DBGREQ_FIPS_MODE_SET                = 134,
    IEEE80211_DBGREQ_ACS_CHANWEIGHTAGE            = 135,
    IEEE80211_DBGREQ_RM_CMD                       = 136, /* RRM command*/
    IEEE80211_DBGREQ_BSTM_CMD                     = 137, /* BSTM command*/
    IEEE80211_DBGREQ_SEND_CUSTBCNRPT              = 138, /* custom beacon report request */
    IEEE80211_DBGREQ_DISALLOW_MSCS                = 139, /* Disallow MSCS feature for selected STA/s */
    IEEE80211_DBGREQ_DISABLE_OPCLASS_CHAN         = 140, /* Disable opclass channels */
#if QCA_MANUAL_TRIGGERED_ULOFDMA
    IEEE80211_DBGREQ_PEER_TRIGGER_UL_OFDMA        = 141, /* manual trigger ulofdma for given peer */
#endif
#ifdef QCA_STANDALONE_SOUNDING_TRIGGER
    IEEE80211_DBGREQ_TRIGGER_TXBF_SOUNDING        = 142,
#endif
    IEEE80211_DBGREQ_GET_FIPS_IFINDEX             = 143,
    IEEE80211_DBGREQ_TWT_ENABLE                   = 144, /* Enable/Disable TWT on Radio/Pdev */
#ifdef QCA_STANDALONE_SOUNDING_TRIGGER
    IEEE80211_DBGREQ_GET_TRIGGERED_FB_TYPE        = 145,
#endif
    IEEE80211_DBGREQ_GETACSREPORT_WITH_RSSI_IN_NR = 146, /* GET the ACS report with RSSI in neighbor report */
    IEEE80211_DBGREQ_MAX
};

enum {
    IEEE80211_DBGREQ_WFA_MODULE_MBSSID                = 1, /* Module ID Mbssid for wfa test cmd */
    IEEE80211_DBGREQ_WFA_MODULE_11AX                  = 2, /* Module ID 11ax for wfa test cmd */
    IEEE80211_DBGREQ_WFA_MODULE_6GHZ                  = 3, /* Module ID 6GHz for wfa test cmd */
};

#ifdef QCA_STANDALONE_SOUNDING_TRIGGER
/*TXBF Sounding Macros */
#define TXBF_SOUNDING_FEEDBACK_TYPE_SU  		0
#define TXBF_SOUNDING_FEEDBACK_TYPE_MU  		1
#define TXBF_SOUNDING_FEEDBACK_TYPE_CQI 		2
#define TXBF_SOUNDING_NG_MIN            		0
#define TXBF_SOUNDING_NG_MAX            		1
#define TXBF_SOUNDING_CODEBOOK_MIN      		0
#define TXBF_SOUNDING_CODEBOOK_MAX      		1
#define TXBF_SOUNDING_PEERS_MAX 			3
#define TXBF_SOUNDING_PEERS_MIN 			1
#define TXBF_SOUNDING_GET_TRIG_PEERS_MAX		5
#define TXBF_SOUNDING_REPEATS_SU_MAX 			5
#define TXBF_SOUNDING_REPEATS_MU_MAX 			3
#define TXBF_SOUNDING_REPEATS_MIN 			1
#define TXBF_SOUNDING_REPEATS_CQI_NON_TRIGERRED_MAX 	5
#define TXBF_SOUNDING_REPEATS_CQI_TRIGERRED_MAX 	3
#define TXBF_SOUNDING_CQI_TRIGGERED_TYPE 		1
#define TXBF_SOUNDING_CQI_NON_TRIGGERED_TYPE 		0
#define MAX_SOUNDING_EV_TIMEOUT 			10000/* timout in ms */

/**
 * struct wmi_txbf_sounding_trig_info - TXBF souding parameters
 * @pdev_id: pdev id
 * @vde_id: vdev id
 * @feedback_type: single user/multi user feedback type,[0]Range[0-SU, 1-MU]
 * @ng: [2:1] Ng -Range[0-1]
 * @codebook: [3] Codebook -Range[0-1]
 * @bw: [6:4] Bandwidth -Range[0-4]
 * @reserved: [31:7] reserved
 * @sounding_repeats: sounding repetations,Range[[0-3]-MU, [0-5]-SU]
 * @num_sounding_peers: number of peers, Range[1- SU, [1-3]-MU]
 * @macaddr: mac address of peers
 */
typedef struct txbf_sounding_trig_info {
	u_int32_t  pdev_id;
	u_int32_t  vdev_id;
	u_int32_t  feedback_type      :2,
		   ng                 :2,
		   codebook           :1,
		   bw                 :3,
		   cqi_triggered_type :1,
		   reserved           :23;
	u_int32_t  sounding_repeats;
	u_int8_t   num_sounding_peers;
	u_int8_t   macaddr[TXBF_SOUNDING_PEERS_MAX][IEEE80211_ADDR_LEN];
} txbf_sounding_trig_info_t;

struct peer_trig_fb_info {
    u_int8_t   macaddr[IEEE80211_ADDR_LEN];
    uint16_t   he_trig_su        :1,
               he_trig_mu        :1,
               he_trig_cqi       :1,
               he_non_trig_cqi   :1,
               eht_trig_su       :1,
               eht_trig_mu       :1,
               eht_trig_cqi      :1,
               eht_non_trig_cqi  :1,
               reserved          :8;
};

typedef struct peer_trig_fb_type_info {
    u_int16_t  n_peers;
    struct peer_trig_fb_info peers[TXBF_SOUNDING_GET_TRIG_PEERS_MAX];
} peer_trig_fb_type_info_t;
#endif

typedef struct ieee80211req_acs_r{
    u_int32_t index;
    u_int32_t data_size;
    void *data_addr;
}ieee80211req_acs_t;

#define ACS_MAX_CHANNEL_COUNT 255
typedef struct ieee80211_user_chanlist_r {
    u_int16_t n_chan;
    struct ieee80211_chan_def chans[ACS_MAX_CHANNEL_COUNT];
} ieee80211_user_chanlist_t;

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
typedef struct ieee80211_primary_allowed_chanlist {
        u_int8_t n_chan;
        struct ieee80211_chan_def chans[ACS_MAX_CHANNEL_COUNT];
} ieee80211_primary_allowed_chanlist_t;

typedef struct ieee80211_primary_allowed_freqlist {
        u_int8_t n_chan;
        uint16_t *freq;
} ieee80211_primary_allowed_freqlist_t;
#endif

typedef struct ieee80211_opclass_chan_list {
        uint8_t is_disable;
        uint8_t n_chan;
        uint8_t opclass;
        uint8_t chans[MAX_CHANNELS_PER_OP_CLASS];
} ieee80211_opclass_chan_list_t;

enum ieee80211_offchan_rx_sec_chan_offset {
    OFFCHAN_RX_SCN = 0,
    OFFCHAN_RX_SCA = 1,
    OFFCHAN_RX_SCB = 3,
};

struct acs_chan_weightage {
    uint8_t  band;
    uint16_t chnum;
    uint8_t  weightage;
};

struct acs_chan_weightage_list {
    uint8_t  get;
    uint8_t  append;
    uint16_t last_idx;
    struct acs_chan_weightage chan[IEEE80211_CHAN_MAX];
};

enum ieee80211_offchan_rx_bandwidth {
    OFFCHAN_RX_BANDWIDTH_20MHZ,
    OFFCHAN_RX_BANDWIDTH_40MHZ,
    OFFCHAN_RX_BANDWIDTH_80MHZ,
    OFFCHAN_RX_BANDWIDTH_160MHZ,
#ifdef WLAN_FEATURE_11BE
    OFFCHAN_RX_BANDWIDTH_320MHZ,
#endif /* WLAN_FEATURE_11BE */
};

typedef struct ieee80211_wide_band_scan {
    u_int8_t bw_mode;
    u_int8_t sec_chan_offset;
} ieee80211_wide_band_scan_t;

/**
 * ieee80211_offchan_tx_test: off channel tx parameters
 * band: band in which off chan tx has to happen
 * ieee_chan: primary 20 offchan tx chan
 * dwell_time: Dwell time
 * wide_scan: indicated wide bandwidth scan
 * ieee_cfreq2: center frequency of continuous bandwidth channel of center freq
 *               of secondary 80 of 80+80 channel
 */
typedef struct ieee80211_offchan_tx_test {
    u_int8_t band;
    u_int8_t ieee_chan;
    u_int16_t dwell_time;
    ieee80211_wide_band_scan_t wide_scan;
    u_int8_t ieee_cfreq2;
} ieee80211_offchan_tx_test_t;

#if UMAC_SUPPORT_VI_DBG
typedef struct ieee80211_vow_dbg_stream_param {
	u_int8_t  stream_num;         /* the stream number whose markers are being set */
	u_int8_t  marker_num;         /* the marker number whose parameters (offset, size & match) are being set */
	u_int32_t marker_offset;      /* byte offset from skb start (upper 16 bits) & size in bytes(lower 16 bits) */
	u_int32_t marker_match;       /* marker pattern match used in filtering */
} ieee80211_vow_dbg_stream_param_t;

typedef struct ieee80211_vow_dbg_param {
	u_int8_t  num_stream;        /* Number of streams */
	u_int8_t  num_marker;       /* total number of markers used to filter pkts */
	u_int32_t rxq_offset;      /* Rx Seq num offset skb start (upper 16 bits) & size in bytes(lower 16 bits) */
	u_int32_t rxq_shift;         /* right-shift value in case field is not word aligned */
	u_int32_t rxq_max;           /* Max Rx seq number */
	u_int32_t time_offset;       /* Time offset for the packet*/
} ieee80211_vow_dbg_param_t;
#endif

typedef struct ieee80211_sta_info {
    u_int16_t count; /* In application layer this variable is used to store the STA count and in the driver it is used as an index */
    u_int16_t max_sta_cnt;
    u_int8_t *dest_addr;
}ieee80211_sta_info_t;

typedef struct ieee80211_noise_stats{
    u_int8_t noise_value;
    u_int8_t min_value;
    u_int8_t max_value;
    u_int8_t median_value;
}ieee80211_noise_stats_t;

typedef struct ieee80211_node_info {
    u_int16_t count;
    u_int16_t bin_number;
    u_int32_t traf_rate;
}ieee80211_node_info_t;
/* User defined control table for calibrated data */
#define MAX_USER_CTRL_TABLE_LEN     2048
typedef struct ieee80211_user_ctrl_tbl_r {
    u_int16_t ctrl_len;
    u_int8_t *ctrl_table_buff;
} ieee80211_user_ctrl_tbl_t;

struct ieee80211req_fake_mgmt {
    u_int32_t buflen;  /*application supplied buffer length */
    u_int8_t  *buf;
};

/*
 * command id's for use in tr069 request
 */
typedef enum _ieee80211_tr069_cmd_ {
    TR069_CHANHIST           = 1,
    TR069_TXPOWER            = 2,
    TR069_GETTXPOWER         = 3,
    TR069_GUARDINTV          = 4,
    TR069_GET_GUARDINTV      = 5,
    TR069_GETASSOCSTA_CNT    = 6,
    TR069_GETTIMESTAMP       = 7,
    TR069_GETDIAGNOSTICSTATE = 8,
    TR069_GETNUMBEROFENTRIES = 9,
    TR069_GET11HSUPPORTED    = 10,
    TR069_GETPOWERRANGE      = 11,
    TR069_SET_OPER_RATE      = 12,
    TR069_GET_OPER_RATE      = 13,
    TR069_GET_POSIBLRATE     = 14,
    TR069_SET_BSRATE         = 15,
    TR069_GET_BSRATE         = 16,
    TR069_GETSUPPORTEDFREQUENCY  = 17,
    TR069_GET_PLCP_ERR_CNT   = 18,
    TR069_GET_FCS_ERR_CNT    = 19,
    TR069_GET_PKTS_OTHER_RCVD = 20,
    TR069_GET_FAIL_RETRANS_CNT = 21,
    TR069_GET_RETRY_CNT      = 22,
    TR069_GET_MUL_RETRY_CNT  = 23,
    TR069_GET_ACK_FAIL_CNT   = 24,
    TR069_GET_AGGR_PKT_CNT   = 25,
    TR069_GET_STA_BYTES_SENT = 26,
    TR069_GET_STA_BYTES_RCVD = 27,
    TR069_GET_DATA_SENT_ACK  = 28,
    TR069_GET_DATA_SENT_NOACK = 29,
    TR069_GET_CHAN_UTIL      = 30,
    TR069_GET_RETRANS_CNT    = 31,
    TR069_GET_RRM_UTIL       = 32,
    TR069_GET_CSA_DEAUTH     = 33,
    TR069_SET_CSA_DEAUTH     = 34,
}ieee80211_tr069_cmd;

typedef struct {
	u_int32_t value;
	int value_array[TR069MAXPOWERRANGE];
}ieee80211_tr069_txpower_range;

typedef struct{
    u_int8_t         chanid;
    u_int8_t         chanband;
    timespec_t chan_time;
}ieee80211_chanlhist_t;

typedef struct{
    u_int8_t act_index;
    ieee80211_chanlhist_t chanlhist[IEEE80211_CHAN_MAXHIST+1];
}ieee80211_channelhist_t;

typedef struct{
    u_int32_t channel;
    u_int32_t chann_util;
    u_int32_t obss_util;
    int16_t  noise_floor;
    u_int8_t  radar_detect;
} ieee80211_rrmutil_t;

/*
 * common structure to handle tr069 commands;
 * the cmdid and data pointer has to be appropriately
 * filled in
 */
typedef struct{
    u_int32_t data_size;
    ieee80211_tr069_cmd cmdid;
    u_int8_t data_buff[TR69_MAX_BUF_LEN];
}ieee80211req_tr069_t;

typedef struct ieee80211req_fips {
    u_int32_t data_size;
    void *data_addr;
}ieee80211req_fips_t;

typedef struct ieee80211req_fips_mode {
    u_int32_t fips_mode_set;
}ieee80211req_fips_mode_t;

#define MAX_SCAN_CHANS       32

#if QCA_LTEU_SUPPORT

typedef enum {
    MU_ALGO_1 = 0x1, /* Basic binning algo */
    MU_ALGO_2 = 0x2, /* Enhanced binning algo */
    MU_ALGO_3 = 0x4, /* Enhanced binning including accounting for hidden nodes */
    MU_ALGO_4 = 0x8, /* TA based MU calculation */
} mu_algo_t;

typedef struct {
    u_int8_t     mu_req_id;             /* MU request id */
    u_int8_t     mu_channel;            /* IEEE channel number on which to do MU scan */
    mu_algo_t    mu_type;               /* which MU algo to use */
    u_int32_t    mu_duration;           /* duration of the scan in ms */
    u_int32_t    lteu_tx_power;         /* LTEu Tx power */
    u_int32_t    mu_rssi_thr_bssid;     /* RSSI threshold to account for active APs */
    u_int32_t    mu_rssi_thr_sta;       /* RSSI threshold to account for active STAs */
    u_int32_t    mu_rssi_thr_sc;        /* RSSI threshold to account for active small cells */
    u_int32_t    home_plmnid;           /* to be compared with PLMN ID to distinguish same and different operator WCUBS */
    u_int32_t    alpha_num_bssid;       /* alpha for num active bssid calculation,kept for backward compatibility */
} ieee80211req_mu_scan_t;

#define LTEU_MAX_BINS        10

typedef struct {
    u_int8_t     lteu_gpio_start;        /* start MU/AP scan after GPIO toggle */
    u_int8_t     lteu_num_bins;          /* no. of elements in the following arrays */
    u_int8_t     use_actual_nf;          /* whether to use the actual NF obtained or a hardcoded one */
    u_int32_t    lteu_weight[LTEU_MAX_BINS];  /* weights for MU algo */
    u_int32_t    lteu_thresh[LTEU_MAX_BINS];  /* thresholds for MU algo */
    u_int32_t    lteu_gamma[LTEU_MAX_BINS];   /* gamma's for MU algo */
    u_int32_t    lteu_scan_timeout;      /* timeout in ms to gpio toggle */
    u_int32_t    alpha_num_bssid;      /* alpha for num active bssid calculation */
    u_int32_t    lteu_cfg_reserved_1;    /* used to indicate to fw whether or not packets with phy error are to
                                            be included in MU calculation or not */

} ieee80211req_lteu_cfg_t;

typedef enum {
    SCAN_PASSIVE,
    SCAN_ACTIVE,
} scan_type_t;

typedef struct {
    u_int8_t     scan_req_id;          /* AP scan request id */
    u_int8_t     scan_num_chan;        /* Number of channels to scan, 0 for all channels */
    u_int8_t     scan_channel_list[MAX_SCAN_CHANS]; /* IEEE channel number of channels to scan */
    scan_type_t  scan_type;            /* Scan type - active or passive */
    u_int32_t    scan_duration;        /* Duration in ms for which a channel is scanned, 0 for default */
    u_int32_t    scan_repeat_probe_time;   /* Time before sending second probe request, (u32)(-1) for default */
    u_int32_t    scan_rest_time;       /* Time in ms on the BSS channel, (u32)(-1) for default */
    u_int32_t    scan_idle_time;       /* Time in msec on BSS channel before switching channel, (u32)(-1) for default */
    u_int32_t    scan_probe_delay;     /* Delay in msec before sending probe request, (u32)(-1) for default */
} ieee80211req_ap_scan_t;



#endif /* QCA_LTEU_SUPPORT */

#define MAX_CUSTOM_CHANS     101

typedef struct {
    u_int8_t     scan_numchan_associated;        /* Number of channels to scan, 0 for all channels */
    u_int8_t     scan_numchan_nonassociated;
    struct ieee80211_chan_def scan_channel_list_associated[MAX_CUSTOM_CHANS];
    struct ieee80211_chan_def scan_channel_list_nonassociated[MAX_CUSTOM_CHANS];
}ieee80211req_custom_chan_t;

#define MAX_FW_UNIT_TEST_NUM_ARGS 100
/**
 * struct ieee80211_fw_unit_test_cmd - unit test command parameters
 * @module_id: module id
 * @num_args: number of arguments
 * @diag_token: Token representing the transaction ID (between host and fw)
 * @args: arguments
 */
struct ieee80211_fw_unit_test_cmd {
        u_int32_t module_id;
        u_int32_t num_args;
        u_int32_t diag_token;
        u_int32_t args[MAX_FW_UNIT_TEST_NUM_ARGS];
} __attribute__ ((packed));

/**
 * struct ieee80211_fw_unit_test_event - unit test event structure
 * @module_id: module id (typically the same module-id as send in the command)
 * @diag_token: This token identifies the command token to
 *              which fw is responding
 * @flag: Informational flags to be used by the application (wifitool)
 * @payload_len: Length of meaningful bytes inside buffer[]
 * @buffer_len: Actual length of the buffer[]
 * @buffer[]: buffer containing event data
 */
struct ieee80211_fw_unit_test_event {
    struct {
        u_int32_t module_id;
        u_int32_t diag_token;
        u_int32_t flag;
        u_int32_t payload_len;
        u_int32_t buffer_len;
    } hdr;
    u_int8_t buffer[1];
} __attribute__ ((packed));

struct ieee80211req_athdbg_event {
    u_int8_t cmd;
    union {
        struct ieee80211_fw_unit_test_event fw_unit_test;
    };
} __attribute__ ((packed));

enum {
    IEEE80211_DBG_DPTRACE_PROTO_BITMAP = 1,
    IEEE80211_DBG_DPTRACE_VERBOSITY,
    IEEE80211_DBG_DPTRACE_NO_OF_RECORD,
    IEEE80211_DBG_DPTRACE_DUMPALL,
    IEEE80211_DBG_DPTRACE_CLEAR,
    IEEE80211_DBG_DPTRACE_LIVEMODE_ON,
    IEEE80211_DBG_DPTRACE_LIVEMODE_OFF,
};

typedef struct __ieee80211req_dbptrace {
    u_int32_t dp_trace_subcmd;
    u_int32_t val1;
    u_int32_t val2;
} ieee80211req_dptrace;

/**
 * coex_cfg_t - coex configuration command parameters
 * @type  : config type (wmi_coex_config_type enum)
 * @arg[] : arguments based on config type
 */
#define COEX_CFG_MAX_ARGS 6
typedef struct {
    u_int32_t type;
    u_int32_t arg[COEX_CFG_MAX_ARGS];
} coex_cfg_t;

#define MAX_CHANNELS_PER_OPERATING_CLASS  24
#define IEEE80211_MAX_OPERATING_CLASS 32

#ifdef WLAN_SUPPORT_TWT
#define TWT_MAX_DIALOG_ID 31
#define TWT_MIN_DIALOG_ID 0
#define IEEE80211_TWT_FLAG_BCAST 0x1
#define IEEE80211_TWT_FLAG_TRIGGER 0x2
#define IEEE80211_TWT_FLAG_FLOW_TYPE 0x4
#define IEEE80211_TWT_FLAG_PROTECTION 0x8
#ifdef WLAN_SUPPORT_BCAST_TWT
#define IEEE80211_TWT_FLAG_BTWT_ID0 0x10
#define IEEE80211_TWT_FLAG_BTWT_PERSISTENCE_IDX 0x10
#define IEEE80211_TWT_FLAG_BTWT_PERSISTENCE_BITS 0x8
#define IEEE80211_GET_BTWT_PERSISTENCE(flags)           \
        HE_GET_BITS(flags,                       \
        IEEE80211_TWT_FLAG_BTWT_PERSISTENCE_IDX,        \
        IEEE80211_TWT_FLAG_BTWT_PERSISTENCE_BITS)
#define IEEE80211_SET_BTWT_PERSISTENCE(flags, val)      \
        HE_SET_BITS(flags,                       \
        IEEE80211_TWT_FLAG_BTWT_PERSISTENCE_IDX,        \
        IEEE80211_TWT_FLAG_BTWT_PERSISTENCE_BITS, val)

#define IEEE80211_TWT_FLAG_BTWT_RECOMMENDATION_IDX 0x18
#define IEEE80211_TWT_FLAG_BTWT_RECOMMENDATION_BITS 0x3
#define IEEE80211_GET_BTWT_RECOMMENDATION(flags)        \
        HE_GET_BITS(flags,                       \
        IEEE80211_TWT_FLAG_BTWT_RECOMMENDATION_IDX,     \
        IEEE80211_TWT_FLAG_BTWT_RECOMMENDATION_BITS)
#define IEEE80211_SET_BTWT_RECOMMENDATION(flags, val)   \
        HE_SET_BITS(flags,                       \
        IEEE80211_TWT_FLAG_BTWT_RECOMMENDATION_IDX,     \
        IEEE80211_TWT_FLAG_BTWT_RECOMMENDATION_BITS, val)
#endif


struct ieee80211_twt_add_dialog {
    uint32_t dialog_id;
    uint32_t wake_intvl_us;
    uint32_t wake_intvl_mantis;
    uint32_t wake_dura_us;
    uint32_t sp_offset_us;
    uint32_t twt_cmd;
    uint32_t flags;
    uint32_t link_id_bitmap;
    uint32_t r_twt_dl_tid_bitmap;
    uint32_t r_twt_ul_tid_bitmap;
};

struct ieee80211_twt_del_pause_dialog {
    uint32_t dialog_id;
#ifdef WLAN_SUPPORT_BCAST_TWT
    uint32_t b_twt_persistence;
#endif
};

struct ieee80211_twt_resume_dialog {
    uint32_t dialog_id;
    uint32_t sp_offset_us;
    uint32_t next_twt_size;
};

#ifdef WLAN_SUPPORT_BCAST_TWT
struct ieee80211_twt_btwt_sta_inv_remove {
    uint32_t dialog_id;
    uint32_t dl_tid_bitmap;
    uint32_t ul_tid_bitmap;
};
#endif
#endif

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

#define MAP_MAX_CAC_MODES 1

#define MAP_DEFAULT_PPDU_DURATION 100
#define MAP_PPDU_DURATION_UNITS 50

/**
 * @brief Data Format Subfield Encoding
 */
typedef enum wlan_data_format {
    data_no_aggregation_enabled,
    data_amsdu_aggregation_enabled,
    data_ampdu_aggregation_enabled,
    data_amsdu_ampdu_aggregation_enabled,
    data_aggregation_max,  // always last
} wlan_data_format;

/**
 * @brief BA Window Size Subfield Encoding
 */
typedef enum wlan_ba_window_size {
    ba_window_not_used = 0,
    ba_window_size_2 = 2,
    ba_window_size_4 = 4,
    ba_window_size_6 = 6,
    ba_window_size_8 = 8,
    ba_window_size_16 = 16,
    ba_window_size_32 = 32,
    ba_window_size_64 = 64,
} wlan_ba_window_size;

/**
 * @brief BA Window Size Subfield Value
 */
typedef enum wlan_ba_window_value {
    ba_window_value_0,
    ba_window_value_1,
    ba_window_value_2,
    ba_window_value_3,
    ba_window_value_4,
    ba_window_value_5,
    ba_window_value_6,
    ba_window_value_7,
} wlan_ba_window_value;

/**
 * @brief Enable/Disable flags for Tx Packet capture
 */
typedef enum {
  PKT_CAPTURE_ENH_DISABLE = 0,
  PKT_CAPTURE_ENH_ENABLE,
} PKT_CAPTURE_ENH_STATUS;

/**
 * @brief List of parameters for configuring rx and tx pkt capture per peer
 */
struct ieee80211_pkt_capture_enh {
  //Enable or disable Rx Pkt Capture enhancements
  PKT_CAPTURE_ENH_STATUS rx_pkt_cap_enable;
  //Enable or disable Tx Pkt Capture enhancements
  PKT_CAPTURE_ENH_STATUS tx_pkt_cap_enable;
  //peer's MAC address
  u_int8_t peer_mac[IEEE80211_ADDR_LEN];
  //Skip node check when adding peer for filtering
  u_int8_t skip_node_check	:1;
};

/**
 * @brief Opcodes to add or delete or print stats for a flow tag
 */
typedef enum {
    RX_FLOW_TAG_OPCODE_ADD,
    RX_FLOW_TAG_OPCODE_DEL,
    RX_FLOW_TAG_OPCODE_DUMP_STATS,
} RX_FLOW_TAG_OPCODE_TYPE;

/**
 * @brief IP protocol version type
 */
typedef enum {
    IP_VER_4,
    IP_VER_6
} IP_VER_TYPE;

/**
 * @brief Layer 4 Protocol types supported for flow tagging
 */
typedef enum {
    L4_PROTOCOL_TYPE_TCP,
    L4_PROTOCOL_TYPE_UDP,
} L4_PROTOCOL_TYPE;


/**
 * @brief 5-tuple for RX flow
 */
struct ieee80211_rx_flow_tuple {
    uint32_t                source_ip[4];   //L3 Source IP address (v4 and v6)
    uint32_t                dest_ip[4];     //L3 Destination IP address (v4 and v6)
    uint16_t                source_port;    //L4 Source port
    uint16_t                dest_port;      //L4 Destination port
    L4_PROTOCOL_TYPE        protocol;       //L4 Protocol type (TCP or UDP)
};

struct ieee80211_rx_flow_info {
    uint32_t svc_id;
    uint8_t tid;
    uint8_t drop_flow;
    uint8_t ring_id;
    u_int8_t dstmac[IEEE80211_ADDR_LEN];
};

/**
 * @brief List of parameters for configuring RX flow tag
 */
typedef struct ieee80211_rx_flow_tag {
    RX_FLOW_TAG_OPCODE_TYPE         op_code;        //operation to be performed
    IP_VER_TYPE                     ip_ver;         //IPv4 or IPv6 version
    struct ieee80211_rx_flow_tuple  flow_tuple;     //5-tuple info per flow
    struct ieee80211_rx_flow_info   flow_info;      // flow information
    uint16_t                        flow_metadata;  //flow meta data for above 5 tuple
} ieee80211_wlanconfig_rx_flow_tag;

/**
 * @brief Opcodes to add or delete a protocol or flow tag
 */
typedef enum {
  RX_PKT_TAG_OPCODE_ADD,
  RX_PKT_TAG_OPCODE_DEL,
} RX_PKT_TAG_OPCODE_TYPE;

/**
 * @brief List of protocols supported for protocol tag
 */
typedef enum {
  RECV_PKT_TYPE_ARP,
  RECV_PKT_TYPE_NS,
  RECV_PKT_TYPE_IGMP_V4,
  RECV_PKT_TYPE_MLD_V6,
  RECV_PKT_TYPE_DHCP_V4,
  RECV_PKT_TYPE_DHCP_V6,
  RECV_PKT_TYPE_DNS_TCP_V4,
  RECV_PKT_TYPE_DNS_TCP_V6,
  RECV_PKT_TYPE_DNS_UDP_V4,
  RECV_PKT_TYPE_DNS_UDP_V6,
  RECV_PKT_TYPE_ICMP_V4,
  RECV_PKT_TYPE_ICMP_V6,
  RECV_PKT_TYPE_TCP_V4,
  RECV_PKT_TYPE_TCP_V6,
  RECV_PKT_TYPE_UDP_V4,
  RECV_PKT_TYPE_UDP_V6,
  RECV_PKT_TYPE_IPV4,
  RECV_PKT_TYPE_IPV6,
  RECV_PKT_TYPE_EAP,
  RECV_PKT_TYPE_VLAN,
  RECV_PKT_TYPE_MAX,
} RX_PKT_TAG_RECV_PKT_TYPE;

/**
 * @brief List of parameter for configuring protocol tag
 */
struct ieee80211_rx_pkt_protocol_tag {
  RX_PKT_TAG_OPCODE_TYPE    op_code;  //ADD or DEL tag
  RX_PKT_TAG_RECV_PKT_TYPE  pkt_type; //Packet type ARP, NS, EAP,…
  u_int32_t                 pkt_type_metadata; //Metadata to be used to tag the given packet type
} ;

/**
 * @brief List of protocols supported for TX packet Capture classification
 */
typedef enum {
  TX_PKT_TYPE_ARP = 1,
  TX_PKT_TYPE_EAPOL,
  TX_PKT_TYPE_DHCP,
  TX_PKT_TYPE_DNS,
  TX_PKT_TYPE_ICMP,
  TX_PKT_TYPE_MAX,
} TX_PKT_CAP_PKT_TYPE;

/**
 * @brief List of parameter for configuring TX packet Capture classification
 */
struct ieee80211_tx_pkt_cap_params {
  u_int32_t pkt_type_bitmap; //Packet type ARP, NS, EAP,…
};

#define MU_SNIF_MAX_AID 4
/**
 * @brief mode on MU sniffer
 */
typedef enum {
    IEEE80211_MU_SNIF_DIS,
    IEEE80211_MU_SNIF_USER,
    IEEE80211_MU_SNIF_WILD,
} IEEE80211_MU_SNIF_MODE;

/**
 * @brief List of parameters for configuring RX flow tag
 */
typedef struct ieee80211_mu_snif {
    IEEE80211_MU_SNIF_MODE mode;     // MU sniffer mode
    u_int32_t num_user;     //max number of user to decode
    u_int32_t num_aid;     //number of user aid
    u_int32_t aid[MU_SNIF_MAX_AID]; //flow meta data for above 5 tuple
} ieee80211_wlanconfig_mu_snif;

/**
 * Parameters that can be configured by userspace on a per client
 * basis
 */
typedef struct ieee80211_acl_cli_param_t {
    u_int8_t  probe_rssi_hwm;
    u_int8_t  probe_rssi_lwm;
    u_int8_t  inact_snr_xing;
    u_int8_t  low_snr_xing;
    u_int8_t  low_rate_snr_xing;
    u_int8_t  high_rate_snr_xing;
    u_int8_t  auth_block;
    u_int8_t  auth_rssi_hwm;
    u_int8_t  auth_rssi_lwm;
    u_int8_t  auth_reject_reason;
} ieee80211_acl_cli_param_t;

#ifdef QCA_SUPPORT_SCAN_SPCL_VAP_STATS
typedef struct ieee80211_scan_spcl_vap_stats {
    uint64_t tx_ok_pkts; /* raw frame home chan tx success cnt */
    uint64_t tx_ok_bytes; /* raw frame home chan tx success bytes */
    uint64_t tx_retry_pkts; /* raw frame home chan tx retry cnt */
    uint64_t tx_retry_bytes; /* raw frame home chan tx retry bytes */
    uint64_t tx_err_pkts; /* raw frame home chan tx err cnt */
    uint64_t tx_err_bytes; /* raw frame home chan tx err bytes */
    uint64_t rx_ok_pkts; /* rx fcs ok cnt */
    uint64_t rx_ok_bytes; /* rx fcs ok bytes */
    uint64_t rx_err_pkts; /* rx fcs err cnt */
    uint64_t rx_err_bytes; /* rx fcs err bytes */
    uint64_t rx_mgmt_pkts; /* rx mgmt cnt */
    uint64_t rx_ctrl_pkts; /* rx ctrl cnt */
    uint64_t rx_data_pkts; /* rx data cnt */
} ieee80211_scan_spcl_vap_stats_t;
#endif

typedef enum {
    IEEE80211_WFA_TEST_6GHZ_AFC_SUBCMD_RESET = 1,
    IEEE80211_WFA_TEST_6GHZ_AFC_SUBCMD_MAX,
} ieee80211_wfa_test_6ghz_afc_subcmds;

struct ieee80211_wfa_test_cmd {
    uint8_t module_id;
    uint8_t cmd_id;
    uint8_t num_args;
    uint32_t args[100];
} __attribute__ ((packed));

struct ieee80211req_athdbg {
    u_int8_t cmd;
    u_int8_t needs_reply;
    u_int8_t dstmac[IEEE80211_ADDR_LEN];
    union {
        u_long param[5];
        ieee80211_rrm_beaconreq_info_t bcnrpt;
        ieee80211_rrm_tsmreq_info_t    tsmrpt;
        ieee80211_rrm_nrreq_info_t     neigrpt;
        struct ieee80211_bstm_reqinfo   bstmreq;
        struct ieee80211_bstm_reqinfo_target   bstmreq_target;
        struct ieee80211_mscs_resp      mscsresp;
        struct ieee80211_mscs_disallow_db disallow_mscs_list;
        struct ieee80211_qos_mgmt_ie    qos_data;
        struct ieee80211_qos_req        dscp_req;
#ifdef WLAN_SUPPORT_SCS
        struct ieee80211_scs_resp       scsresp;
#endif
        struct ieee80211_user_bssid_pref bssidpref;
        ieee80211_tspec_info     tsinfo;
        ieee80211_rrm_cca_info_t   cca;
        ieee80211_rrm_rpihist_info_t   rpihist;
        ieee80211_rrm_chloadreq_info_t chloadrpt;
        ieee80211_rrm_stastats_info_t  stastats;
        ieee80211_rrm_nhist_info_t     nhist;
        ieee80211_rrm_frame_req_info_t frm_req;
        ieee80211_rrm_lcireq_info_t    lci_req;
        ieee80211req_rrmstats_t        rrmstats_req;
        ieee80211req_acs_t             acs_rep;
        ieee80211req_tr069_t           tr069_req;
        timespec_t t_spec;
        ieee80211req_fips_t fips_req;
        ieee80211req_fips_mode_t fips_mode_req;
        struct ieee80211_qos_map       qos_map;
        ieee80211_acl_cli_param_t      acl_cli_param;
        ieee80211_offchan_tx_test_t offchan_req;
#if UMAC_SUPPORT_VI_DBG
	ieee80211_vow_dbg_stream_param_t   vow_dbg_stream_param;
	ieee80211_vow_dbg_param_t	   vow_dbg_param;
#endif

#if QCA_LTEU_SUPPORT
        ieee80211req_mu_scan_t         mu_scan_req;
        ieee80211req_lteu_cfg_t        lteu_cfg;
        ieee80211req_ap_scan_t         ap_scan_req;
#endif
        ieee80211req_custom_chan_t     custom_chan_req;
#if QCA_AIRTIME_FAIRNESS
        atf_debug_req_t                atf_dbg_req;
#endif
	struct ieee80211_fw_unit_test_cmd	fw_unit_test_cmd;
        ieee80211_user_ctrl_tbl_t      *user_ctrl_tbl;
        ieee80211_user_chanlist_t      user_chanlist;
        ieee80211req_dptrace           dptrace;
        coex_cfg_t                     coex_cfg_req;
#if ATH_ACL_SOFTBLOCKING
        u_int8_t                       acl_softblocking;
#endif
        u_int8_t twt_enable;
#ifdef WLAN_SUPPORT_TWT
        struct ieee80211_twt_add_dialog twt_add;
        struct ieee80211_twt_del_pause_dialog twt_del_pause;
        struct ieee80211_twt_resume_dialog twt_resume;
#ifdef WLAN_SUPPORT_BCAST_TWT
        struct ieee80211_twt_btwt_sta_inv_remove twt_btwt_sta_inv_remove;
#endif
#endif
        ieee80211_node_info_t          node_info;
        beacon_rssi_info               beacon_rssi_info;
        ack_rssi_info                  ack_rssi_info;
#ifdef WLAN_SUPPORT_RX_FLOW_TAG
        ieee80211_wlanconfig_rx_flow_tag rx_flow_tag_info;
#endif /* WLAN_SUPPORT_RX_FLOW_TAG */
        ieee80211_user_nrresp_info_t   *neighrpt_custom;
#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
        ieee80211_primary_allowed_chanlist_t primary_allowed_chanlist;
#endif
        struct ieee80211req_fake_mgmt mgmt_frm;
        struct ieee80211_tpe_ie_config tpe_conf;
        wlan_latency_info_t wlan_latency_info;
#ifdef QCA_SUPPORT_SCAN_SPCL_VAP_STATS
        ieee80211_scan_spcl_vap_stats_t scan_spcl_vap_stats;
#endif
        struct ieee80211_wfa_test_cmd wfa_test_cmd;
        struct acs_chan_weightage_list acs_chanweightage_chlist;
        ieee80211_rrm_cmd_t rm_cmd;
        ieee80211_bstm_cmd_t bstm_cmd;
        ieee80211_opclass_chan_list_t opclass_disabled_chans;
#ifdef QCA_STANDALONE_SOUNDING_TRIGGER
        txbf_sounding_trig_info_t txbf_sounding_trig_info;
        peer_trig_fb_type_info_t peer_trig_fb_type_info;
#endif
#if QCA_MANUAL_TRIGGERED_ULOFDMA
        struct ul_ofdma_trig_info trig_ul_ofdma;
#endif
    } data;
};

#ifdef __linux__
/*
 * Wireless Extensions API, private ioctl interfaces.
 *
 * NB: Even-numbered ioctl numbers have set semantics and are privileged!
 *	(regardless of the incorrect comment in wireless.h!)
 *
 *	Note we can only use 32 private ioctls, and yes they are all claimed.
 */
#ifndef _NET_IF_H
#include <linux/if.h>
#endif
#define	IEEE80211_IOCTL_SETPARAM	(SIOCIWFIRSTPRIV+0)
#define	IEEE80211_IOCTL_GETPARAM	(SIOCIWFIRSTPRIV+1)
#define	IEEE80211_IOCTL_SETKEY		(SIOCIWFIRSTPRIV+2)
#define	IEEE80211_IOCTL_SETWMMPARAMS	(SIOCIWFIRSTPRIV+3)
#define	IEEE80211_IOCTL_DELKEY		(SIOCIWFIRSTPRIV+4)
#define	IEEE80211_IOCTL_GETWMMPARAMS	(SIOCIWFIRSTPRIV+5)
#define	IEEE80211_IOCTL_SETMLME		(SIOCIWFIRSTPRIV+6)
#define	IEEE80211_IOCTL_GETCHANINFO	(SIOCIWFIRSTPRIV+7)
#define	IEEE80211_IOCTL_SETOPTIE	(SIOCIWFIRSTPRIV+8)
#define	IEEE80211_IOCTL_GETOPTIE	(SIOCIWFIRSTPRIV+9)
#define	IEEE80211_IOCTL_ADDMAC		(SIOCIWFIRSTPRIV+10)        /* Add ACL MAC Address */
#define	IEEE80211_IOCTL_DELMAC		(SIOCIWFIRSTPRIV+12)        /* Del ACL MAC Address */
#define	IEEE80211_IOCTL_GETCHANLIST	(SIOCIWFIRSTPRIV+13)
#define	IEEE80211_IOCTL_SETCHANLIST	(SIOCIWFIRSTPRIV+14)
#define IEEE80211_IOCTL_KICKMAC		(SIOCIWFIRSTPRIV+15)
#define	IEEE80211_IOCTL_CHANSWITCH	(SIOCIWFIRSTPRIV+16)
#define	IEEE80211_IOCTL_GETMODE		(SIOCIWFIRSTPRIV+17)
#define	IEEE80211_IOCTL_SETMODE		(SIOCIWFIRSTPRIV+18)
#define IEEE80211_IOCTL_GET_APPIEBUF	(SIOCIWFIRSTPRIV+19)
#define IEEE80211_IOCTL_SET_APPIEBUF	(SIOCIWFIRSTPRIV+20)
#define IEEE80211_IOCTL_SET_ACPARAMS	(SIOCIWFIRSTPRIV+21)
#define IEEE80211_IOCTL_FILTERFRAME	(SIOCIWFIRSTPRIV+22)
#define IEEE80211_IOCTL_SET_RTPARAMS	(SIOCIWFIRSTPRIV+23)
#define IEEE80211_IOCTL_DBGREQ	        (SIOCIWFIRSTPRIV+24)
#define IEEE80211_IOCTL_SEND_MGMT	(SIOCIWFIRSTPRIV+26)
#define IEEE80211_IOCTL_SET_MEDENYENTRY (SIOCIWFIRSTPRIV+27)
#define IEEE80211_IOCTL_CHN_WIDTHSWITCH (SIOCIWFIRSTPRIV+28)
#define IEEE80211_IOCTL_GET_MACADDR	(SIOCIWFIRSTPRIV+29)        /* Get ACL List */
#define IEEE80211_IOCTL_SET_HBRPARAMS	(SIOCIWFIRSTPRIV+30)
#define IEEE80211_IOCTL_SET_RXTIMEOUT	(SIOCIWFIRSTPRIV+31)
/*
 * MCAST_GROUP is used for testing, not for regular operation.
 * It is defined unconditionally (overlapping with SET_RXTIMEOUT),
 * but only used for debugging (after disabling SET_RXTIMEOUT).
 */
#define IEEE80211_IOCTL_MCAST_GROUP     (SIOCIWFIRSTPRIV+31)

#define CURR_MODE 0 /* used to get the curret mode of operation*/
#define PHY_MODE 1  /* used to get the desired phymode */

enum {
	IEEE80211_WMMPARAMS_CWMIN	= 1,
	IEEE80211_WMMPARAMS_CWMAX	= 2,
	IEEE80211_WMMPARAMS_AIFS	= 3,
	IEEE80211_WMMPARAMS_TXOPLIMIT	= 4,
	IEEE80211_WMMPARAMS_ACM		= 5,
	IEEE80211_WMMPARAMS_NOACKPOLICY	= 6,
#if UMAC_VOW_DEBUG
    IEEE80211_PARAM_VOW_DBG_CFG     = 7,  /*Configure VoW debug MACs*/
#endif
};

enum {
    IEEE80211_LEGACY_PREAMBLE   = 0,
    IEEE80211_HT_PREAMBLE       = 1,
    IEEE80211_VHT_PREAMBLE      = 2,
    IEEE80211_HE_PREAMBLE       = 3,
    IEEE80211_EHT_PREAMBLE      = 4,
};

#if QCA_SUPPORT_GPR
enum {
    IEEE80211_GPR_DISABLE       = 0,
    IEEE80211_GPR_ENABLE        = 1,
    IEEE80211_GPR_PRINT_STATS   = 2,
    IEEE80211_GPR_CLEAR_STATS   = 3,
};
#endif

enum {
	IEEE80211_IOCTL_RCPARAMS_RTPARAM	= 1,
	IEEE80211_IOCTL_RCPARAMS_RTMASK		= 2,
};

enum {
    IEEE80211_MUEDCAPARAMS_ECWMIN = 1,
    IEEE80211_MUEDCAPARAMS_ECWMAX = 2,
    IEEE80211_MUEDCAPARAMS_AIFSN = 3,
    IEEE80211_MUEDCAPARAMS_ACM = 4,
    IEEE80211_MUEDCAPARAMS_TIMER = 5,
};

#define WOW_CUSTOM_PKT_LEN 102
#define WOW_SYNC_PATTERN 0xFF
#define WOW_SYNC_LEN 6
#define WOW_MAC_ADDR_COUNT 16
#define ETH_TYPE_WOW 0x0842

/*
 * New get/set params for p2p.
 * The first 16 set/get priv ioctls know the direction of the xfer
 * These sub-ioctls, don't care, any number in 16 bits is ok
 * The param numbers need not be contiguous, but must be unique
 */
#define IEEE80211_IOC_P2P_GO_OPPPS        621    /* IOCTL to turn on/off oppPS for P2P GO */
#define IEEE80211_IOC_P2P_GO_CTWINDOW     622    /* IOCTL to set CT WINDOW size for P2P GO*/
#define IEEE80211_IOC_P2P_GO_NOA          623    /* IOCTL to set NOA for P2P GO*/

//#define IEEE80211_IOC_P2P_FLUSH           616    /* IOCTL to flush P2P state */
#define IEEE80211_IOC_SCAN_REQ            624    /* IOCTL to request a scan */
//needed, below
#define IEEE80211_IOC_SCAN_RESULTS        IEEE80211_IOCTL_SCAN_RESULTS

#define IEEE80211_IOC_SSID                626    /* set ssid */
#define IEEE80211_IOC_MLME                IEEE80211_IOCTL_SETMLME
#define IEEE80211_IOC_CHANNEL             628    /* set channel */

#define IEEE80211_IOC_WPA                 IEEE80211_PARAM_WPA    /* WPA mode (0,1,2) */
#define IEEE80211_IOC_AUTHMODE            IEEE80211_PARAM_AUTHMODE
#define IEEE80211_IOC_KEYMGTALGS          IEEE80211_PARAM_KEYMGTALGS    /* key management algorithms */
#define IEEE80211_IOC_WPS_MODE            632    /* Wireless Protected Setup mode  */

#define IEEE80211_IOC_UCASTCIPHERS        IEEE80211_PARAM_UCASTCIPHERS    /* unicast cipher suites */
#define IEEE80211_IOC_UCASTCIPHER         IEEE80211_PARAM_UCASTCIPHER    /* unicast cipher */
#define IEEE80211_IOC_MCASTCIPHER         IEEE80211_PARAM_MCASTCIPHER    /* multicast/default cipher */
//unused below
#define IEEE80211_IOC_START_HOSTAP        636    /* Start hostap mode BSS */

#define IEEE80211_IOC_DROPUNENCRYPTED     637    /* discard unencrypted frames */
#define IEEE80211_IOC_PRIVACY             638    /* privacy invoked */
#define IEEE80211_IOC_OPTIE               IEEE80211_IOCTL_SETOPTIE    /* optional info. element */
#define IEEE80211_IOC_BSSID               640    /* GET bssid */
//unused below 3
#define IEEE80211_IOC_P2P_SET_CHANNEL     641    /* Set Channel */
#define IEEE80211_IOC_P2P_CANCEL_CHANNEL  642    /* Cancel current set-channel operation */
#define IEEE80211_IOC_P2P_SEND_ACTION     643    /* Send Action frame */

#define IEEE80211_IOC_P2P_OPMODE          644    /* set/get the opmode(STA,AP,P2P GO,P2P CLI) */
#define IEEE80211_IOC_P2P_FETCH_FRAME     645    /* get rx_frame mgmt data, too large for an event */

#define IEEE80211_IOC_SCAN_FLUSH          646
#define IEEE80211_IOC_CONNECTION_STATE    647 	/* connection state of the iface */
#define IEEE80211_IOC_P2P_NOA_INFO        648   /*  To get NOA sub element info from p2p client */
#define IEEE80211_IOC_CANCEL_SCAN           650   /* To cancel scan request */
#define IEEE80211_IOC_P2P_RADIO_IDX         651   /* Get radio index */
#ifdef HOST_OFFLOAD
#endif

struct ieee80211_p2p_go_neg {
    u_int8_t peer_addr[IEEE80211_ADDR_LEN];
    u_int8_t own_interface_addr[IEEE80211_ADDR_LEN];
    u_int16_t force_freq;
    u_int8_t go_intent;
    char pin[9];
} __attribute__ ((packed));

struct ieee80211_p2p_prov_disc {
    u_int8_t peer_addr[IEEE80211_ADDR_LEN];
    u_int16_t config_methods;
} __attribute__ ((packed));

struct ieee80211_p2p_serv_disc_resp {
    u_int16_t freq;
    u_int8_t dst[IEEE80211_ADDR_LEN];
    u_int8_t dialog_token;
    /* followed by response TLVs */
} __attribute__ ((packed));

struct ieee80211_p2p_go_noa {
    u_int8_t  num_iterations;   /* Number of iterations (equal 1 if one shot)
                                   and 1-254 if periodic) and 255 for continuous */
    u_int16_t offset_next_tbtt; /* offset in msec from next tbtt */
    u_int16_t duration;         /* duration in msec */
} __attribute__ ((packed));

struct ieee80211_p2p_set_channel {
    u_int32_t freq;
    u_int32_t req_id;
    u_int32_t channel_time;
} __attribute__ ((packed));

struct ieee80211_p2p_send_action {
    u_int32_t freq;
    u_int32_t scan_time;
    u_int32_t cancel_current_wait;
    u_int8_t dst_addr[IEEE80211_ADDR_LEN];
    u_int8_t src_addr[IEEE80211_ADDR_LEN];
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    /* Followed by Action frame payload */
} __attribute__ ((packed));

struct ieee80211_send_action_cb {
    u_int8_t dst_addr[IEEE80211_ADDR_LEN];
    u_int8_t src_addr[IEEE80211_ADDR_LEN];
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    u_int8_t ack;
    /* followed by frame body */
} __attribute__ ((packed));

/* Optional parameters for IEEE80211_IOC_SCAN_REQ */
struct ieee80211_scan_req {
#define MAX_SCANREQ_FREQ 16
    u_int32_t freq[MAX_SCANREQ_FREQ];
    u_int8_t num_freq;
    u_int8_t num_ssid;
    u_int16_t ie_len;
#define MAX_SCANREQ_SSID 10
    u_int8_t ssid[MAX_SCANREQ_SSID][32];
    u_int8_t ssid_len[MAX_SCANREQ_SSID];
    /* followed by ie_len octets of IEs to add to Probe Request frames */
} __attribute__ ((packed));


struct ieee80211_ioc_channel {
    u_int32_t phymode; /* enum ieee80211_phymode */
    u_int32_t channel; /* IEEE channel number */
} __attribute__ ((packed));

#define LINUX_PVT_SET_VENDORPARAM       (SIOCDEVPRIVATE+0)
#define LINUX_PVT_GET_VENDORPARAM       (SIOCDEVPRIVATE+1)
#define	SIOCG80211STATS		(SIOCDEVPRIVATE+2)
/* NB: require in+out parameters so cannot use wireless extensions, yech */
#define	IEEE80211_IOCTL_GETKEY		(SIOCDEVPRIVATE+3)
#define	IEEE80211_IOCTL_GETWPAIE	(SIOCDEVPRIVATE+4)
#define	IEEE80211_IOCTL_STA_STATS	(SIOCDEVPRIVATE+5)
#define	IEEE80211_IOCTL_STA_INFO	(SIOCDEVPRIVATE+6)
#define	SIOC80211IFCREATE		(SIOCDEVPRIVATE+7)
#define	SIOC80211IFDESTROY	 	(SIOCDEVPRIVATE+8)
#define	IEEE80211_IOCTL_SCAN_RESULTS	(SIOCDEVPRIVATE+9)
#define IEEE80211_IOCTL_RES_REQ         (SIOCDEVPRIVATE+10)
#define IEEE80211_IOCTL_GETMAC          (SIOCDEVPRIVATE+11)
#define IEEE80211_IOCTL_CONFIG_GENERIC  (SIOCDEVPRIVATE+12)
#define SIOCIOCTLTX99                   (SIOCDEVPRIVATE+13)
#define IEEE80211_IOCTL_P2P_BIG_PARAM   (SIOCDEVPRIVATE+14)
#define SIOCDEVVENDOR                   (SIOCDEVPRIVATE+15)    /* Used for ATH_SUPPORT_LINUX_VENDOR */
#define	IEEE80211_IOCTL_GET_SCAN_SPACE  (SIOCDEVPRIVATE+16)

#define IEEE80211_IOCTL_ATF_ADDSSID     0xFF01
#define IEEE80211_IOCTL_ATF_DELSSID     0xFF02
#define IEEE80211_IOCTL_ATF_ADDSTA      0xFF03
#define IEEE80211_IOCTL_ATF_DELSTA      0xFF04
#define IEEE80211_IOCTL_ATF_SHOWATFTBL  0xFF05
#define IEEE80211_IOCTL_ATF_SHOWAIRTIME 0xFF06
#define IEEE80211_IOCTL_ATF_FLUSHTABLE  0xFF07                 /* Used to Flush the ATF table entries */

#define IEEE80211_IOCTL_ATF_ADDGROUP    0xFF08
#define IEEE80211_IOCTL_ATF_CONFIGGROUP 0xFF09
#define IEEE80211_IOCTL_ATF_DELGROUP    0xFF0a
#define IEEE80211_IOCTL_ATF_SHOWGROUP   0xFF0b

#define IEEE80211_IOCTL_ATF_ADDSTA_TPUT     0xFF0C
#define IEEE80211_IOCTL_ATF_DELSTA_TPUT     0xFF0D
#define IEEE80211_IOCTL_ATF_SHOW_TPUT       0xFF0E

#define IEEE80211_IOCTL_ATF_GROUPSCHED      0XFF0F
#define IEEE80211_IOCTL_ATF_ADDAC           0xFF10
#define IEEE80211_IOCTL_ATF_DELAC           0xFF11
#define IEEE80211_IOCTL_ATF_SHOWSUBGROUP    0xFF12
#define IEEE80211_IOCTL_ATF_GET_STATS       0xFF13
#define IEEE80211_IOCTL_ATF_GET_AC_STATS    0xFF14

#if 0
struct ieee80211_clone_params {
    char	icp_name[IFNAMSIZ];	/* device name */
    u_int16_t	icp_opmode;		/* operating mode */
    u_int32_t	icp_flags;		/* see IEEE80211_CLONE_BSSID for e.g */
    u_int8_t icp_bssid[IEEE80211_ADDR_LEN];    /* optional mac/bssid address */
    int32_t         icp_vapid;             /* vap id for MAC addr req */
    u_int8_t icp_mataddr[IEEE80211_ADDR_LEN];    /* optional MAT address */
};
#define	    IEEE80211_CLONE_BSSID       0x0001		/* allocate unique mac/bssid */
#define	    IEEE80211_NO_STABEACONS	    0x0002		/* Do not setup the station beacon timers */
#define    IEEE80211_CLONE_WDS          0x0004      /* enable WDS processing */
#define    IEEE80211_CLONE_WDSLEGACY    0x0008      /* legacy WDS operation */
#endif
/* added APPIEBUF related definations */
#define    IEEE80211_APPIE_FRAME_BEACON      0
#define    IEEE80211_APPIE_FRAME_PROBE_REQ   1
#define    IEEE80211_APPIE_FRAME_PROBE_RESP  2
#define    IEEE80211_APPIE_FRAME_ASSOC_REQ   3
#define    IEEE80211_APPIE_FRAME_ASSOC_RESP  4
#define    IEEE80211_APPIE_FRAME_TDLS_FTIE   5   /* TDLS SMK_FTIEs */
#define    IEEE80211_APPIE_FRAME_AUTH        6
#define    IEEE80211_APPIE_NUM_OF_FRAME      7
#define    IEEE80211_APPIE_FRAME_WNM         8

#define    DEFAULT_IDENTIFIER 0
#define    HOSTAPD_IE 1
#define    HOSTAPD_WPS_IE 2
#define    TWT_IE 3

struct ieee80211req_getset_appiebuf {
    u_int32_t app_frmtype; /*management frame type for which buffer is added*/
    u_int32_t app_buflen;  /*application supplied buffer length */
    u_int8_t  identifier;
    u_int8_t  app_buf[];
};

struct ieee80211req_mgmtbuf {
    u_int8_t  macaddr[IEEE80211_ADDR_LEN]; /* mac address to be sent */
    u_int32_t buflen;  /*application supplied buffer length */
    u_int8_t  buf[];
};

/* the following definations are used by application to set filter
 * for receiving management frames */
enum {
     IEEE80211_FILTER_TYPE_BEACON      =   0x1,
     IEEE80211_FILTER_TYPE_PROBE_REQ   =   0x2,
     IEEE80211_FILTER_TYPE_PROBE_RESP  =   0x4,
     IEEE80211_FILTER_TYPE_ASSOC_REQ   =   0x8,
     IEEE80211_FILTER_TYPE_ASSOC_RESP  =   0x10,
     IEEE80211_FILTER_TYPE_AUTH        =   0x20,
     IEEE80211_FILTER_TYPE_DEAUTH      =   0x40,
     IEEE80211_FILTER_TYPE_DISASSOC    =   0x80,
     IEEE80211_FILTER_TYPE_ACTION      =   0x100,
     IEEE80211_FILTER_TYPE_ALL         =   0xFFF  /* used to check the valid filter bits */
};

struct ieee80211req_set_filter {
      u_int32_t app_filterype; /* management frame filter type */
};

struct ieee80211_wlanconfig_atf {
    u_int8_t     macaddr[IEEE80211_ADDR_LEN];    /* MAC address (input) */
    u_int32_t    short_avg;                      /* AirtimeShortAvg (output) */
    u_int64_t    total_used_tokens;              /* AirtimeTotal    (output) */
};

struct ieee80211_wlanconfig_nawds {
    u_int8_t num;
    u_int8_t mode;
    u_int64_t defcaps;
    u_int8_t override;
    u_int8_t mac[IEEE80211_ADDR_LEN];
    u_int64_t caps;
    u_int8_t  psk[32];
#ifdef WLAN_FEATURE_11BE
    u_int16_t puncture_bitmap;
    u_int16_t def_puncture_bitmap;
#endif
#ifdef UMAC_SUPPORT_MLNAWDS
    u_int8_t mld_mac[IEEE80211_ADDR_LEN];
    u_int8_t peer_mld_mac[IEEE80211_ADDR_LEN];
    u_int8_t peer_link_mac[IEEE80211_ADDR_LEN];
    u_int8_t link_id;
#endif
};

#define MAX_PARTNER_LINKS 3
struct ieee80211_vap_peer_info {
    u_int8_t vap_mac[IEEE80211_ADDR_LEN];
    u_int8_t peer_link_mac[IEEE80211_ADDR_LEN];
    u_int8_t link_id;
    u_int64_t caps;
    u_int16_t puncture_bitmap;
};

struct ieee80211_wlanconfig_mesh {
#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
    u_int16_t num_links;
    u_int8_t peer_mld_mac[IEEE80211_ADDR_LEN];
    struct ieee80211_vap_peer_info peers[MAX_PARTNER_LINKS];    /* Index 0 always represents that vap on which command was given */
#endif
#endif
};

struct ieee80211_wlanconfig_hmwds {
    u_int8_t  wds_ni_macaddr[IEEE80211_ADDR_LEN];
    u_int16_t wds_macaddr_cnt;
    u_int8_t  wds_macaddr[0];
};

/**
 * ieee80211_wlanconfig_ipa_wds - struct for deleting wds type ast entry
 * @wds_ni_macaddr  : wds ni macaddress
 * @wds_macaddr_cnt : wds node mac address cnt
 * @wds_macaddr : wds node mac address
 * @peer_macaddr : direct associated peer mac address
 */
struct ieee80211_wlanconfig_ipa_wds {
    u_int8_t  wds_ni_macaddr[IEEE80211_ADDR_LEN];
    u_int16_t wds_macaddr_cnt;
    u_int8_t  wds_macaddr[IEEE80211_ADDR_LEN];
    u_int8_t  peer_macaddr[IEEE80211_ADDR_LEN];
};

struct ieee80211_wlanconfig_ald_sta {
    u_int8_t  macaddr[IEEE80211_ADDR_LEN];
    u_int32_t enable;
};

struct ieee80211_wlanconfig_ald {
    union {
        struct ieee80211_wlanconfig_ald_sta ald_sta;
    } data;
};

struct ieee80211_wlanconfig_wnm_bssmax {
    u_int16_t idleperiod;
    u_int8_t idleoption;
};

struct ieee80211_wlanconfig_wds {
    u_int8_t destmac[IEEE80211_ADDR_LEN];
    u_int8_t peermac[IEEE80211_ADDR_LEN];
    u_int32_t flags;
};

struct ieee80211_wlanconfig_wds_table {
    u_int16_t wds_entry_cnt;
    struct ieee80211_wlanconfig_wds wds_entries[0];
};

typedef enum {
    IEEE80211_HMMC_LIST = 0,
    IEEE80211_DENY_LIST = 1,
    IEEE80211_HMMC_LIST_V6 = 2,
    IEEE80211_DENY_LIST_V6 = 3,
} IEEE80211_ME_LIST;

struct ieee80211_wlanconfig_me_list {
    union {
        u_int32_t ip;
        u_int32_t ipv6[4];
    };
    u_int32_t mask; // or prefix for ipv6
    IEEE80211_ME_LIST me_list_type;
};

struct ieee80211_wlanconfig_setmaxrate {
    u_int8_t mac[IEEE80211_ADDR_LEN];
    u_int8_t maxrate;
};

#define TFS_MAX_FILTER_LEN 50
#define TFS_MAX_TCLAS_ELEMENTS 2
#define TFS_MAX_SUBELEMENTS 2
#define TFS_MAX_REQUEST 2
#define TFS_MAX_RESPONSE 600

#define FMS_MAX_SUBELEMENTS    2
#define FMS_MAX_TCLAS_ELEMENTS 2
#define FMS_MAX_REQUEST        2
#define FMS_MAX_RESPONSE       2

typedef enum {
    IEEE80211_WNM_TFS_AC_DELETE_AFTER_MATCH = 0,
    IEEE80211_WNM_TFS_AC_NOTIFY = 1,
} IEEE80211_WNM_TFS_ACTIONCODE;

typedef enum {
    IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE0 = 0,
    IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE1 = 1,
    IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE2 = 2,
    IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE3 = 3,
    IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE4 = 4,
    IEEE80211_WNM_TCLAS_CLASSIFIER_TYPE10 = 10,
} IEEE80211_WNM_TCLAS_CLASSIFIER;

typedef enum {
    IEEE80211_WNM_TCLAS_CLAS14_VERSION_4 = 4,
    IEEE80211_WNM_TCLAS_CLAS14_VERSION_6 = 6,
} IEEE80211_WNM_TCLAS_VERSION;

#ifndef IEEE80211_IPV4_LEN
#define IEEE80211_IPV4_LEN 4
#endif

#ifndef IEEE80211_IPV6_LEN
#define IEEE80211_IPV6_LEN 16
#endif

/*
 * TCLAS Classifier Type 1 and Type 4 are exactly the same for IPv4.
 * For IPv6, Type 4 has two more fields (dscp, next header) than
 * Type 1. So we use the same structure for both Type 1 and 4 here.
 */
struct clas14_v4 {
    u_int8_t     version;
    u_int8_t     source_ip[IEEE80211_IPV4_LEN];
    u_int8_t     reserved1[IEEE80211_IPV6_LEN - IEEE80211_IPV4_LEN];
    u_int8_t     dest_ip[IEEE80211_IPV4_LEN];
    u_int8_t     reserved2[IEEE80211_IPV6_LEN - IEEE80211_IPV4_LEN];
    u_int16_t    source_port;
    u_int16_t    dest_port;
    u_int8_t     dscp;
    u_int8_t     protocol;
    u_int8_t     reserved;
    u_int8_t     reserved3[2];
}__packed;

struct clas14_v6 {
    u_int8_t     version;
    u_int8_t     source_ip[IEEE80211_IPV6_LEN];
    u_int8_t     dest_ip[IEEE80211_IPV6_LEN];
    u_int16_t    source_port;
    u_int16_t    dest_port;
    u_int8_t     clas4_dscp;
    u_int8_t     clas4_next_header;
    u_int8_t     flow_label[3];
}__packed;

struct clas3 {
    u_int16_t filter_offset;
    u_int32_t filter_len;
    u_int8_t  filter_value[TFS_MAX_FILTER_LEN];
    u_int8_t  filter_mask[TFS_MAX_FILTER_LEN];
}__packed;

struct tfsreq_tclas_element {
    u_int8_t classifier_type;
    u_int8_t classifier_mask;
    u_int8_t priority;
    union {
        union {
            struct clas14_v4 clas14_v4;
            struct clas14_v6 clas14_v6;
        } clas14;
        struct clas3 clas3;
    } clas;
}__packed;

struct tfsreq_subelement {
    u_int32_t num_tclas_elements;
    u_int8_t tclas_processing;
    struct tfsreq_tclas_element tclas[TFS_MAX_TCLAS_ELEMENTS];
};

struct ieee80211_wlanconfig_wnm_tfs_req {
    u_int8_t tfsid;
    u_int8_t actioncode;
    u_int8_t num_subelements;
    struct tfsreq_subelement subelement[TFS_MAX_SUBELEMENTS];
};

/* All array size allocation is based on higher range i.e.lithium */
#define NAC_MAX_CLIENT		48
/* MAX allowed clients for beeliner family */
#define NAC_MAX_CLIENT_V0	8
#define NAC_MAX_BSSID		8
/* MAX allowed bssids for beeliner family */
#define NAC_MAX_BSSID_V0	3

typedef enum ieee80211_nac_mactype {
    IEEE80211_NAC_MACTYPE_BSSID  = 1,
    IEEE80211_NAC_MACTYPE_CLIENT = 2,
} IEEE80211_NAC_MACTYPE;

struct ieee80211_wlanconfig_nac {
    u_int8_t    mac_type;
    u_int8_t    mac_list[NAC_MAX_CLIENT][IEEE80211_ADDR_LEN]; /* client has max limit */
    u_int8_t    rssi[NAC_MAX_CLIENT];
    timespec_t      ageSecs[NAC_MAX_CLIENT];
};

struct ieee80211_wlanconfig_nac_rssi {
    u_int8_t    mac_bssid[IEEE80211_ADDR_LEN];
    u_int8_t    mac_client[IEEE80211_ADDR_LEN];
    u_int8_t    chan_num;
    u_int8_t    client_rssi_valid;
    u_int8_t    client_rssi;
};

/* Civic information size in bytes */
#define CIVIC_INFO_LEN 256
#define COUNTRY_CODE_LEN 2

/**
 * ieee80211_wlanconfig_lcr - wlanconfig LCR structure
 * @req_id: Request id
 * @country_code: Country code
 * @civic_len: Civic info length
 * @civic_info: Civic info
 */
struct ieee80211_wlanconfig_lcr {
    uint16_t req_id;
    uint8_t country_code[COUNTRY_CODE_LEN];
    uint8_t civic_len;
    uint8_t civic_info[CIVIC_INFO_LEN];
};

/**
 * lci_motion_pattern - LCI motion pattern
 * @LCI_MOTION_NOT_EXPECTED: Not expected to change location
 * @LCI_MOTION_EXPECTED: Expected to change location
 * @LCI_MOTION_UNKNOWN: Movement pattern unknown
 */
enum lci_motion_pattern {
    LCI_MOTION_NOT_EXPECTED = 0,
    LCI_MOTION_EXPECTED     = 1,
    LCI_MOTION_UNKNOWN      = 2
};

/**
 * ieee80211_wlanconfig_lci - wlanconfig LCI structure
 * @req_id: Request id
 * @latitude: Latitude
 * @longitude: Longitude
 * @altitude: Altitude
 * @latitude_unc: Latitude uncertainty
 * @longitude_unc: Longitude uncertainty
 * @altitude_unc: Altitude uncertainty
 * @motion_pattern: Motion pattern
 * @floor: Floor number
 * @height_above_floor: Height above the floor
 * @height_unc: Height uncertainty
 */
struct ieee80211_wlanconfig_lci {
    uint16_t req_id;
    uint64_t latitude;
    uint64_t longitude;
    uint32_t altitude;
    uint8_t latitude_unc;
    uint8_t longitude_unc;
    uint8_t altitude_unc;
    enum lci_motion_pattern motion_pattern;
    uint32_t floor;
    uint32_t height_above_floor;
    uint32_t height_unc;
};

/* Number of variables present in ieee80211_wlanconfig_ftmrr_elems structure */
#define FTMRR_ELEMS 8

#define MAX_NEIGHBOR_NUM 15
/**
 * ieee80211_wlanconfig_ftmrr_elems - wlanconfig FTMRR structure
 * @bssid: STA mac address
 * @bssid_info: BSSID info
 * @chan: Channel number
 * @center_ch1: Center channel number1
 * @center_ch2: Center channel number2
 * @chwidth: Channel bandwidth
 * @opclass: Operating class
 * @phytype: Phytype
 */
struct ieee80211_wlanconfig_ftmrr_elems {
	uint8_t bssid[IEEE80211_ADDR_LEN];
	uint32_t bssid_info;
	uint8_t chan;
	uint8_t center_ch1;
	uint8_t center_ch2;
	uint8_t chwidth;
	uint8_t opclass;
	uint8_t phytype;
};

struct ieee80211_wlanconfig_ftmrr {
	uint8_t sta_mac[IEEE80211_ADDR_LEN];
	uint8_t random_interval;
	uint8_t num_elements;
	struct ieee80211_wlanconfig_ftmrr_elems elem[MAX_NEIGHBOR_NUM];
};

/* Max supported TIDs */
#define NUM_MAX_TIDS 8

/**
 * enum t2lm_dir - TID-to-link mapping direction
 * @T2LM_DIRECTION_DL: Downlink
 * @T2LM_DIRECTION_UL: Uplink
 * @T2LM_DIRECTION_BIDI: Bidirectional
 * @T2LM_DIRECTION_MAX: Max direction
 * @T2LM_DIRECTION_INVALID: Invalid direction
 */
enum t2lm_dir {
    T2LM_DIRECTION_DL,
    T2LM_DIRECTION_UL,
    T2LM_DIRECTION_BIDI,
    T2LM_DIRECTION_MAX,
    T2LM_DIRECTION_INVALID
};

#ifdef WLAN_FEATURE_11BE

/* Number of T2LM params expected per TID (params are listed in struct
 * tid_to_link_map).
 */
#define T2LM_MAP_PARAMS 2

/* T2LM Request rejected and preferred TID-to-link mapping is suggested */
#define T2LM_STATUS_PREFERRED_TID_TO_LINK_MAPPING 134

struct epcs_wmeParams {
    uint8_t     aci;                /* acess category index */
    u_int8_t    wmep_aifsn;         /* AIFSN parameters */
    u_int8_t    wmep_logcwmin;      /* cwmin in exponential form */
    u_int8_t    wmep_logcwmax;      /* cwmax in exponential form */
    u_int16_t   wmep_txopLimit;     /* txopLimit */
};

struct epcs_muedcaParams {
    uint8_t     aci;                /* acess category index */
    uint8_t     muedca_aifsn;       /* AIFSN parameter */
    uint8_t     muedca_ecwmin;      /* CWmin in exponential form */
    uint8_t     muedca_ecwmax;      /* CWmax in exponential form */
    uint8_t     muedca_timer;       /* MU EDCA timer value */
};

/**
 * struct ieee80211_wlanconfig_epcs - User configured epcs params
 * @enable: 1 - epcs_enable
 *          0 - epcs_disable
 * @authorize: epcs authrize request for mac_addr
 * @deauthorize: epcs deauthrize request for mac_addr
 * @set_edca: epcs edca parameter is set
 * @set_muedca: epcs muedca parameter is set
 * @get_edca: get edca parameter
 * @get_muedca: get muedca parameter
 * @peer_mld_mac: peer mld mac address
 * @edca_param: epcs edca param
 * @muedca_param: epcs muedca param
 */
struct ieee80211_wlanconfig_epcs {
    uint8_t enable;
    uint8_t authorize;
    uint8_t deauthorize;
    uint8_t set_edca;
    uint8_t set_muedca;
    uint8_t get_edca;
    uint8_t get_muedca;
    uint8_t peer_mld_mac[IEEE80211_ADDR_LEN];
    struct epcs_wmeParams edca_param;
    struct epcs_muedcaParams muedca_param;
};
#endif /* WLAN_FEATURE_11BE */

/**
 * struct tid_to_link_map - TID-to-link mapping params
 * @tid_num: TID number
 * @link_map: T2LM provisioned links for the given TID
 */
struct tid_to_link_map {
    uint8_t tid_num;
    uint16_t link_map;
};

/**
 * struct t2lm_of_direction - TID-to-link mapping for a given direction
 * @num_tids: Total number of TIDs for which mapping is given
 * @direction: direction, DL/UL/BIDI
 * @default_mapping: default tid-to-link mapping value
 * @map_tid_to_links: link mappings of the TIDs
 * @link_mapping_size: link map size for TIDn where value 0:2 bytes 1:1 byte
 */
struct t2lm_of_direction {
    uint8_t num_tids;
    uint8_t direction;
    uint8_t default_mapping;
    struct tid_to_link_map map_tid_to_links[NUM_MAX_TIDS];
    uint8_t link_mapping_size;
};

/**
  * struct ieee80211_wlanconfig_t2lm - User configured T2LM params
  * @assoc_frm: 1 - include the T2LM frame during assoc.
  *         0 - send T2LM action frame
  * @peer_mld_mac: MLO peer macaddress (STA macaddr)
  * @status: T2LM response status value
  * @unsolicited_t2lm_resp: Unsolicited T2LM response
  * @t2lm_direction: TID-to-link mapping params for DL/UL/BIDI
 */
struct ieee80211_wlanconfig_t2lm {
    uint8_t assoc_frm;
    uint8_t peer_mld_mac[IEEE80211_ADDR_LEN];
    uint8_t status;
    uint8_t unsolicited_t2lm_resp;
    struct t2lm_of_direction t2lm_direction[T2LM_DIRECTION_MAX];
};

struct ieee80211_t2lm_link_map_value {
    uint16_t link_map_value;
    u_int16_t map_switch_time;
    uint32_t expected_dur;
    uint8_t link_mapping_size;
};

struct ieee80211_wlanconfig_tdma_schedule {
    uint32_t pdev_id;
    uint8_t sched_type;
    uint8_t sched_id;
    uint8_t bssid[IEEE80211_ADDR_LEN];
    uint32_t start_time_tsf_low;
    uint32_t start_time_tsf_high;
    uint16_t num_busy_slots;
    uint16_t busy_slot_dur_ms;
    uint16_t busy_slot_intvl_ms;
    uint8_t edca_params_valid;
    uint16_t aifsn[WLAN_MAX_AC];
    uint16_t cwmin[WLAN_MAX_AC];
    uint16_t cwmax[WLAN_MAX_AC];
};

struct ieee80211_wlanconfig_enable_ol_stats {
   uint8_t value;
};

#if QCA_SUPPORT_PEER_ISOLATION
struct ieee80211_wlanconfig_isolation {
    u_int8_t    mac[IEEE80211_ADDR_LEN]; /* client has max limit */
};

struct ieee80211_wlanconfig_isolation_list {
    u_int32_t    mac_cnt; /* MAC list count */
    u_int8_t     buf[1]; /* MAC list buffer */
};
#endif

struct ieee80211_wlanconfig_wnm_tfs {
    u_int8_t num_tfsreq;
    struct ieee80211_wlanconfig_wnm_tfs_req tfs_req[TFS_MAX_REQUEST];
};

struct tfsresp_element {
	u_int8_t tfsid;
    u_int8_t status;
} __packed;

struct ieee80211_wnm_tfsresp {
    u_int8_t num_tfsresp;
    struct tfsresp_element  tfs_resq[TFS_MAX_RESPONSE];
} __packed;

typedef struct  ieee80211_wnm_rate_identifier_s {
    u_int8_t mask;
    u_int8_t mcs_idx;
    u_int16_t rate;
}__packed ieee80211_wnm_rate_identifier_t;

struct fmsresp_fms_subele_status {
    u_int8_t status;
    u_int8_t del_itvl;
    u_int8_t max_del_itvl;
    u_int8_t fmsid;
    u_int8_t fms_counter;
    ieee80211_wnm_rate_identifier_t rate_id;
    u_int8_t mcast_addr[6];
};

struct fmsresp_tclas_subele_status {
    u_int8_t fmsid;
    u_int8_t ismcast;
    u_int32_t mcast_ipaddr;
    ieee80211_tclas_processing tclasprocess;
    u_int32_t num_tclas_elements;
    struct tfsreq_tclas_element tclas[TFS_MAX_TCLAS_ELEMENTS];
};

struct fmsresp_element {
    u_int8_t fms_token;
    u_int8_t num_subelements;
    u_int8_t subelement_type;
    union {
        struct fmsresp_fms_subele_status fms_subele_status[FMS_MAX_TCLAS_ELEMENTS];
        struct fmsresp_tclas_subele_status tclas_subele_status[FMS_MAX_SUBELEMENTS];
    }status;
};

struct ieee80211_wnm_fmsresp {
    u_int8_t num_fmsresp;
    struct fmsresp_element  fms_resp[FMS_MAX_RESPONSE];
};

struct fmsreq_subelement {
    u_int8_t del_itvl;
    u_int8_t max_del_itvl;
    u_int8_t tclas_processing;
    u_int32_t num_tclas_elements;
    ieee80211_wnm_rate_identifier_t rate_id;
    struct tfsreq_tclas_element tclas[FMS_MAX_TCLAS_ELEMENTS];
} __packed;

struct ieee80211_wlanconfig_wnm_fms_req {
    u_int8_t fms_token;
    u_int8_t num_subelements;
    struct fmsreq_subelement subelement[FMS_MAX_SUBELEMENTS];
};

struct ieee80211_wlanconfig_wnm_fms {
    u_int8_t num_fmsreq;
    struct ieee80211_wlanconfig_wnm_fms_req  fms_req[FMS_MAX_REQUEST];
};

enum {
    IEEE80211_WNM_TIM_HIGHRATE_ENABLE = 0x1,
    IEEE80211_WNM_TIM_LOWRATE_ENABLE = 0x2,
};

struct ieee80211_wlanconfig_wnm_tim {
    u_int8_t interval;
    u_int8_t enable_highrate;
    u_int8_t enable_lowrate;
};

struct ieee80211_wlanconfig_wnm_bssterm {
    u_int16_t delay;    /* in TBTT */
    u_int16_t duration; /* in minutes */
};

struct ieee80211_wlanconfig_wnm {
    union {
        struct ieee80211_wlanconfig_wnm_bssmax bssmax;
        struct ieee80211_wlanconfig_wnm_tfs tfs;
        struct ieee80211_wlanconfig_wnm_fms fms;
        struct ieee80211_wlanconfig_wnm_tim tim;
        struct ieee80211_wlanconfig_wnm_bssterm bssterm;
    } data;
};

/* generic structure to support sub-ioctl due to limited ioctl */
typedef enum {
    IEEE80211_WLANCONFIG_NOP                           = 0,
    IEEE80211_WLANCONFIG_NAWDS_SET_MODE                = 1,
    IEEE80211_WLANCONFIG_NAWDS_SET_DEFCAPS             = 2,
    IEEE80211_WLANCONFIG_NAWDS_SET_OVERRIDE            = 3,
    IEEE80211_WLANCONFIG_NAWDS_SET_ADDR                = 4,
    IEEE80211_WLANCONFIG_NAWDS_CLR_ADDR                = 5,
    IEEE80211_WLANCONFIG_NAWDS_GET                     = 6,
    IEEE80211_WLANCONFIG_WNM_SET_BSSMAX                = 7,
    IEEE80211_WLANCONFIG_WNM_GET_BSSMAX                = 8,
    IEEE80211_WLANCONFIG_WNM_TFS_ADD                   = 9,
    IEEE80211_WLANCONFIG_WNM_TFS_DELETE                = 10,
    IEEE80211_WLANCONFIG_WNM_FMS_ADD_MODIFY            = 11,
    IEEE80211_WLANCONFIG_WNM_SET_TIMBCAST              = 12,
    IEEE80211_WLANCONFIG_WNM_GET_TIMBCAST              = 13,
    IEEE80211_WLANCONFIG_WDS_ADD_ADDR                  = 14,
    IEEE80211_WLANCONFIG_ME_LIST_ADD                   = 15,
    IEEE80211_WLANCONFIG_ME_LIST_DEL                   = 16,
    IEEE80211_WLANCONFIG_ME_LIST_DUMP                  = 17,
    IEEE80211_WLANCONFIG_HMWDS_ADD_ADDR                = 18,
    IEEE80211_WLANCONFIG_HMWDS_RESET_ADDR              = 19,
    IEEE80211_WLANCONFIG_HMWDS_RESET_TABLE             = 20,
    IEEE80211_WLANCONFIG_HMWDS_READ_ADDR               = 21,
    IEEE80211_WLANCONFIG_HMWDS_READ_TABLE              = 22,
    IEEE80211_WLANCONFIG_HMWDS_SET_BRIDGE_ADDR         = 23,
    IEEE80211_WLANCONFIG_SET_MAX_RATE                  = 24,
    IEEE80211_WLANCONFIG_WDS_SET_ENTRY                 = 25,
    IEEE80211_WLANCONFIG_WDS_DEL_ENTRY                 = 26,
    IEEE80211_WLANCONFIG_ALD_STA_ENABLE                = 27,
    IEEE80211_WLANCONFIG_WNM_BSS_TERMINATION           = 28,
    IEEE80211_WLANCONFIG_GETCHANINFO_160               = 29,
    IEEE80211_WLANCONFIG_VENDOR_IE_ADD                 = 30,
    IEEE80211_WLANCONFIG_VENDOR_IE_UPDATE              = 31,
    IEEE80211_WLANCONFIG_VENDOR_IE_REMOVE              = 32,
    IEEE80211_WLANCONFIG_VENDOR_IE_LIST                = 33,
    IEEE80211_WLANCONFIG_NAC_ADDR_ADD                  = 34,
    IEEE80211_WLANCONFIG_NAC_ADDR_DEL                  = 35,
    IEEE80211_WLANCONFIG_NAC_ADDR_LIST                 = 36,
    IEEE80211_PARAM_STA_ATF_STAT                       = 37,
    IEEE80211_WLANCONFIG_HMWDS_REMOVE_ADDR             = 38,
    IEEE80211_WLANCONFIG_HMWDS_DUMP_WDS_ADDR           = 39,
    IEEE80211_WLANCONFIG_NAC_RSSI_ADDR_ADD             = 40,
    IEEE80211_WLANCONFIG_NAC_RSSI_ADDR_DEL             = 41,
    IEEE80211_WLANCONFIG_NAC_RSSI_ADDR_LIST            = 42,
    IEEE80211_WLANCONFIG_ADD_IE                        = 43,
    IEEE80211_WLANCONFIG_NAWDS_KEY                     = 44,
    IEEE80211_WLANCONFIG_CFR_START                     = 45,
    IEEE80211_WLANCONFIG_CFR_STOP                      = 46,
    IEEE80211_WLANCONFIG_CFR_LIST_PEERS                = 47,
    IEEE80211_WLANCONFIG_RX_FLOW_TAG_OP                = 48,
    IEEE80211_WLANCONFIG_MU_SNIF                       = 49,
    IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_ADD       = 50,
    IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_DEL       = 51,
    IEEE80211_WLANCONFIG_PEER_ISOLATION_ADDR_LIST      = 52,
    IEEE80211_WLANCONFIG_PEER_ISOLATION_FLUSH_LIST     = 53,
    IEEE80211_WLANCONFIG_PEER_ISOLATION_NUM_CLIENT     = 54,
    IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_FTM            = 55,
    IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_NDPA_NDP       = 56,
    IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_FLITER          = 57,
    IEEE80211_WLANCONFIG_CFR_RCC_ALL_FTM_ACK           = 58,
    IEEE80211_WLANCONFIG_CFR_RCC_NDPA_NDP_ALL          = 59,
    IEEE80211_WLANCONFIG_CFR_RCC_ALL_PKT               = 60,
    IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_ADDR            = 61,
    IEEE80211_WLANCONFIG_CFR_RCC_BW_NSS                = 62,
    IEEE80211_WLANCONFIG_CFR_RCC_SUBTYPE               = 63,
    IEEE80211_WLANCONFIG_CFR_RCC_CAPT_DUR              = 64,
    IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL           = 65,
    IEEE80211_WLANCONFIG_CFR_RCC_UL_MU_USER_MASK       = 66,
    IEEE80211_WLANCONFIG_CFR_RCC_FREEZE_TLV_DELAY_CNT  = 67,
    IEEE80211_WLANCONFIG_CFR_EN_CFG                    = 68,
    IEEE80211_WLANCONFIG_CFR_RESET_CFG                 = 69,
    IEEE80211_WLANCONFIG_CFR_GET_CFG                   = 70,
    IEEE80211_WLANCONFIG_CFR_RCC_DBG_COUNTERS          = 71,
    IEEE80211_WLANCONFIG_CFR_RCC_CLR_COUNTERS          = 72,
    IEEE80211_WLANCONFIG_CFR_RCC_DUMP_LUT              = 73,
    IEEE80211_WLANCONFIG_CFR_RCC_DISABLE_ALL           = 74,
    IEEE80211_WLANCONFIG_CFR_RCC_CAPT_COUNT            = 75,
    IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL_MODE_SEL  = 76,
    IEEE80211_WLANCONFIG_CFR_RCC_TARA_FILTER_AS_FP     = 77,
    IEEE80211_WLANCONFIG_CFR_RCC_COMMIT                = 78,
    IEEE80211_WLANCONFIG_LCR                           = 79,
    IEEE80211_WLANCONFIG_LCI                           = 80,
    IEEE80211_WLANCONFIG_FTMRR                         = 81,
#ifdef UMAC_SUPPORT_MLNAWDS
    IEEE80211_WLANCONFIG_MLNAWDS_SET_MODE              = 82,
    IEEE80211_WLANCONFIG_MLNAWDS_SET_OVERRIDE          = 83,
    IEEE80211_WLANCONFIG_MLNAWDS_SET_ADDR              = 84,
    IEEE80211_WLANCONFIG_MLNAWDS_CLR_ADDR              = 85,
    IEEE80211_WLANCONFIG_MLNAWDS_GET                   = 86,
    IEEE80211_WLANCONFIG_MLNAWDS_KEY                   = 87,
    IEEE80211_WLANCONFIG_MLNAWDS_START                 = 88,
#endif
    IEEE80211_WLANCONFIG_SVC_CLASS_CREATE              = 89,
    IEEE80211_WLANCONFIG_SVC_CLASS_DISABLE             = 90,
    IEEE80211_WLANCONFIG_SVC_CLASS_VIEW                = 91,
#ifdef CONFIG_SAWF
    IEEE80211_WLANCONFIG_SAWF_TELEMETRY                = 92,
    IEEE80211_WLANCONFIG_SAWF_SLA                      = 93,
    IEEE80211_WLANCONFIG_SAWF_SLA_DETECT               = 94,
#endif
    IEEE80211_WLANCONFIG_T2LM_REQUEST                  = 95,
    IEEE80211_WLANCONFIG_T2LM_RESPONSE                 = 96,
    IEEE80211_WLANCONFIG_T2LM_TEARDOWN                 = 97,
    IEEE80211_WLANCONFIG_T2LM_LINK_MAP_CONFIG          = 98,
#ifdef WLAN_FEATURE_11BE
    IEEE80211_WLANCONFIG_EPCS                          = 99,
#endif
    IEEE80211_WLANCONFIG_TDMA_SCHEDULE                 = 100,
    IEEE80211_WLANCONFIG_IPA_WDS_REMOVE_ADDR           = 101,
#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
    IEEE80211_WLANCONFIG_MESH_ADDLOCALPEER             = 102,
    IEEE80211_WLANCONFIG_MESH_ALLOWDATA                = 103,
    IEEE80211_WLANCONFIG_MESH_DELLOCALPEER             = 104,
#endif
#endif
    IEEE80211_WLANCONFIG_ENABLE_OL_STATS               = 105,
    IEEE80211_WLANCONFIG_SVC_CREATE                    = 106,
    IEEE80211_WLANCONFIG_SVC_DELETE                    = 107,
    IEEE80211_WLANCONFIG_SVC_UPDATE                    = 108,
#ifdef WLAN_WIFI_RADAR_ENABLE
    IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE            = 109,
    IEEE80211_WLANCONFIG_WIFI_RADAR_CAPTURE_STOP       = 110,
    IEEE80211_WLANCONFIG_WIFI_RADAR_TX_CAL             = 111,
    IEEE80211_WLANCONFIG_WIFI_RADAR_RX_CAL             = 112,
    IEEE80211_WLANCONFIG_WIFI_RADAR_GET_RX_CAL_STATUS  = 113,
#endif
} IEEE80211_WLANCONFIG_CMDTYPE;
/* Note: Do not place any of the above ioctls within compile flags,
   The above ioctls are also being used by external apps.
   External apps do not define the compile flags as driver does.
   Having ioctls within compile flags leave the apps and drivers to use
   a different values.
*/

typedef enum {
    IEEE80211_WLANCONFIG_OK          = 0,
    IEEE80211_WLANCONFIG_FAIL        = 1,
} IEEE80211_WLANCONFIG_STATUS;

struct sawf_wlanconfig_disable_param {
	uint8_t svc_id;
	uint8_t svc_type;
};

enum {
	WLAN_SVC_TYPE_SAWF,
	WLAN_SVC_TYPE_SCS,
	WLAN_SVC_TYPE_STC,
	WLAN_SVC_TYPE_MAX,
};

enum {
	WLAN_SVC_DIR_UL,
	WLAN_SVC_DIR_DL,
	WLAN_SVC_DIR_UL_DL,
	WLAN_SVC_DIR_MAX,
};

enum {
	WLAN_SVC_PARAM_MIN_TPUT,
	WLAN_SVC_PARAM_MAX_TPUT,
	WLAN_SVC_PARAM_BURST_SIZE,
	WLAN_SVC_PARAM_SERVICE_INTERVAL,
	WLAN_SVC_PARAM_DELAY_BOUND,
	WLAN_SVC_PARAM_MSDU_TTL,
	WLAN_SVC_PARAM_PRIORITY,
	WLAN_SVC_PARAM_TID,
	WLAN_SVC_PARAM_MSDU_RATE_LOSS,
	WLAN_SVC_PARAM_UL_BURST_SIZE,
	WLAN_SVC_PARAM_UL_SERVICE_INTERVAL,
	WLAN_SVC_PARAM_DL_SCHED_DISABLE_MODES,
	WLAN_SVC_PARAM_UL_SCHED_DISABLE_MODES,
	WLAN_SVC_PARAM_UL_MIN_TPUT,
	WLAN_SVC_PARAM_UL_MAX_LATENCY,
};

struct sawf_wlanconfig_create_param {
	uint32_t svc_id;
	uint8_t svc_type;
	char app_name[64];
	uint32_t param_set_mask;

	uint32_t min_thruput_rate;
	uint32_t max_thruput_rate;
	uint32_t burst_size;
	uint32_t service_interval;
	uint32_t delay_bound;
	uint32_t msdu_ttl;
	uint32_t priority;
	uint32_t tid;
	uint32_t msdu_rate_loss;
	uint32_t ul_burst_size;
	uint32_t ul_service_interval;
	char dl_disabled_modes[128];
	char ul_disabled_modes[128];
	uint32_t ul_min_tput;
	uint32_t ul_max_latency;
};

struct sawf_wlanconfig_svc_create {
	char app_name[64];
	uint32_t min_thruput_rate;
	uint32_t max_thruput_rate;
	uint32_t burst_size;
	uint32_t service_interval;
	uint32_t delay_bound;
	uint32_t msdu_ttl;
	uint32_t priority;
	uint32_t tid;
	uint32_t msdu_rate_loss;
	uint32_t ul_burst_size;
	uint32_t ul_service_interval;
	char dl_disabled_modes[128];
	char ul_disabled_modes[128];
	uint32_t ul_min_tput;
	uint32_t ul_max_latency;

	uint8_t type;
	uint8_t direction;
	uint32_t mask;
	uint8_t peer_mac_addr[IEEE80211_ADDR_LEN];
};

struct sawf_wlanconfig_svc_delete {
	uint32_t svc_id;
	uint8_t type;
	uint8_t peer_mac_addr[IEEE80211_ADDR_LEN];
};

struct sawf_wlanconfig_svc_update {
	uint32_t svc_id;
	char app_name[64];
	uint32_t min_thruput_rate;
	uint32_t max_thruput_rate;
	uint32_t burst_size;
	uint32_t service_interval;
	uint32_t delay_bound;
	uint32_t msdu_ttl;
	uint32_t priority;
	uint32_t tid;
	uint32_t msdu_rate_loss;
	uint32_t ul_burst_size;
	uint32_t ul_service_interval;
	char dl_disabled_modes[128];
	char ul_disabled_modes[128];
	uint32_t ul_min_tput;
	uint32_t ul_max_latency;

	uint8_t type;
	uint8_t direction;
	uint32_t mask;
	uint8_t peer_mac_addr[IEEE80211_ADDR_LEN];
};

struct sawf_wlanconfig_svc_rsp {
	uint32_t svc_id;
};

#ifdef CONFIG_SAWF
struct sawf_wlanconfig_telemetry {
	uint32_t mv_avg_pkt;
	uint32_t mv_avg_win;
	uint32_t sla_num_pkt;
	uint32_t sla_time_sec;
};

struct sawf_wlanconfig_sla{
	uint8_t svc_id;
	uint32_t min_thruput_rate;
	uint32_t max_thruput_rate;
	uint32_t burst_size;
	uint32_t service_interval;
	uint32_t delay_bound;
	uint32_t msdu_ttl;
	uint32_t msdu_rate_loss;
};

enum sawf_sla_detect {
	SAWF_SLA_DETECT_NUM_PACKET,
	SAWF_SLA_DETECT_PER_SECOND,
	SAWF_SLA_DETECT_MOV_AVG,
	SAWF_SLA_DETECT_NUM_SECOND,
	SAWF_SLA_DETECT_MAX,
};

struct sawf_wlanconfig_sla_detect{
	enum sawf_sla_detect sla_detect;
	uint32_t min_thruput_rate;
	uint32_t max_thruput_rate;
	uint32_t burst_size;
	uint32_t service_interval;
	uint32_t delay_bound;
	uint32_t msdu_ttl;
	uint32_t msdu_rate_loss;
};

#endif

#ifdef WLAN_WIFI_RADAR_ENABLE
struct ieee80211_wifi_radar_cal_info {
	uint64_t timestamp;
	uint32_t wifi_radar_pkt_bw;
	uint32_t channel_bw;
	uint32_t band_center_freq;
	uint32_t cal_num_ltf_tx;
	uint32_t cal_num_skip_ltf_rx;
	uint32_t cal_num_ltf_accumulation;
	uint8_t per_tx_chain_cal_status[HOST_MAX_CHAINS][HOST_MAX_CHAINS];
};
#endif

struct ieee80211_wlanconfig {
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype;  /* sub-command */
    IEEE80211_WLANCONFIG_STATUS status;     /* status code */
    union {
        struct ieee80211_wlanconfig_nawds nawds;
        struct ieee80211_wlanconfig_mesh mesh;
        struct ieee80211_wlanconfig_hmwds hmwds;
        struct ieee80211_wlanconfig_wnm wnm;
        struct ieee80211_wlanconfig_me_list me_list;
        struct ieee80211_wlanconfig_wds_table wds_table;
        struct ieee80211_wlanconfig_ald ald;
        struct ieee80211_wlanconfig_nac nac;
        struct ieee80211_wlanconfig_atf atf;
        struct ieee80211_wlanconfig_nac_rssi nac_rssi;
#ifdef WLAN_CFR_ENABLE
        struct cfr_wlanconfig_param cfr_config;
#endif
        struct sawf_wlanconfig_create_param sawf_create_config;
        struct sawf_wlanconfig_disable_param sawf_disable_config;
        struct sawf_wlanconfig_svc_create sawf_svc_create;
        struct sawf_wlanconfig_svc_delete sawf_svc_delete;
        struct sawf_wlanconfig_svc_update sawf_svc_update;
#ifdef CONFIG_SAWF
        struct sawf_wlanconfig_telemetry sawf_telemetry;
        struct sawf_wlanconfig_sla sawf_sla;
	struct sawf_wlanconfig_sla_detect sawf_sla_detect;
#endif
        ieee80211_wlanconfig_rx_flow_tag rx_flow_tag_info;
	ieee80211_wlanconfig_mu_snif mu_snif;
#if QCA_SUPPORT_PEER_ISOLATION
        struct ieee80211_wlanconfig_isolation isolation;
#endif
        struct ieee80211_wlanconfig_lcr lcr_config;
        struct ieee80211_wlanconfig_lci lci_config;
        struct ieee80211_wlanconfig_ftmrr ftmrr_config;
        struct ieee80211_wlanconfig_t2lm t2lm;
        struct ieee80211_t2lm_link_map_value t2lm_link_map_config;
#ifdef WLAN_FEATURE_11BE
        struct ieee80211_wlanconfig_epcs epcs;
#endif /* WLAN_FEATURE_11BE */
        struct ieee80211_wlanconfig_ipa_wds ipa_wds;
        struct ieee80211_wlanconfig_tdma_schedule tdma_schedule_config;
        struct ieee80211_wlanconfig_enable_ol_stats enable_ol_val;
#ifdef WLAN_WIFI_RADAR_ENABLE
        struct wifi_radar_wlanconfig_params wifi_radar_config;
        struct ieee80211_wifi_radar_cal_info wifi_radar_cal_info;
#endif
    } data;

    struct ieee80211_wlanconfig_setmaxrate smr;
};

#define VENDORIE_OUI_LEN 3
#define MAX_VENDOR_IE_LEN 128
#define MAX_VENDOR_BUF_LEN 2048

struct ieee80211_wlanconfig_ie {
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype;  /* sub-command */
    u_int8_t    ftype;      /* Frame type in which this IE is included */
    struct {
        u_int8_t elem_id;
        u_int8_t len;
        u_int8_t app_buf[];
    }ie;
};

struct ieee80211_wlanconfig_vendorie {

    IEEE80211_WLANCONFIG_CMDTYPE cmdtype;  /* sub-command */
    u_int8_t    ftype_map; /* map which frames , thesse IE are included */
    u_int16_t    tot_len;   /* total vie struct length */
struct  {
    u_int8_t    id;
    u_int8_t    len;    /* len of oui + cap_info */
    u_int8_t    oui[VENDORIE_OUI_LEN];
    u_int8_t    cap_info[];
} ie;
};

struct ieee80211_csa_rx_ev {
    u_int32_t valid;
    u_int32_t chan;
    u_int32_t width_mhz;
    int secondary; /* -1: below, 1:above */
    u_int32_t cfreq2_mhz;
    char bssid[IEEE80211_ADDR_LEN];
};


/**
 * struct ieee80211_ev_assoc_reject - Data for IEEE80211_EV_ASSOC_REJECT events
 * This structure is defined from the reference of assoc_reject structure which
 * is defined in hostapd
 */
struct ieee80211_ev_assoc_reject {
    const u_int8_t *bssid;           /* BSSID of the AP that rejected association */
    const u_int8_t *resp_ies;        /* (Re)Association Response IEs */
    size_t resp_ies_len;             /* Length of resp_ies in bytes */
    u_int16_t status_code;           /* Status Code from (Re)association Response */
    int timed_out;                   /* Whether failure is due to timeout (etc.) rather than explicit rejection response from the AP. */
    const char *timeout_reason;      /* Reason for the timeout */
    u_int16_t fils_erp_next_seq_num; /* The next sequence number to use in FILS ERP messages */
};

/* kev event_code value for Atheros IEEE80211 events */
enum {
    IEEE80211_EV_SCAN_DONE,
    IEEE80211_EV_CHAN_START,
    IEEE80211_EV_CHAN_END,
    IEEE80211_EV_RX_MGMT,
    IEEE80211_EV_P2P_SEND_ACTION_CB,
    IEEE80211_EV_IF_RUNNING,
    IEEE80211_EV_IF_NOT_RUNNING,
    IEEE80211_EV_AUTH_COMPLETE_AP,
    IEEE80211_EV_ASSOC_COMPLETE_AP,
    IEEE80211_EV_DEAUTH_COMPLETE_AP,
    IEEE80211_EV_AUTH_IND_AP,
    IEEE80211_EV_AUTH_COMPLETE_STA,
    IEEE80211_EV_ASSOC_COMPLETE_STA,
    IEEE80211_EV_DEAUTH_COMPLETE_STA,
    IEEE80211_EV_DISASSOC_COMPLETE_STA,
    IEEE80211_EV_AUTH_IND_STA,
    IEEE80211_EV_DEAUTH_IND_STA,
    IEEE80211_EV_ASSOC_IND_STA,
    IEEE80211_EV_DISASSOC_IND_STA,
    IEEE80211_EV_DEAUTH_IND_AP,
    IEEE80211_EV_DISASSOC_IND_AP,
    IEEE80211_EV_ASSOC_IND_AP,
    IEEE80211_EV_REASSOC_IND_AP,
    IEEE80211_EV_MIC_ERR_IND_AP,
    IEEE80211_EV_KEYSET_DONE_IND_AP,
    IEEE80211_EV_BLKLST_STA_AUTH_IND_AP,
    IEEE80211_EV_WAPI,
    IEEE80211_EV_TX_MGMT,
    IEEE80211_EV_CHAN_CHANGE,
    IEEE80211_EV_RECV_PROBEREQ,
    IEEE80211_EV_STA_AUTHORIZED,
    IEEE80211_EV_STA_LEAVE,
    IEEE80211_EV_ASSOC_FAILURE,
    IEEE80211_EV_PRIMARY_RADIO_CHANGED,
    IEEE80211_EV_PREFERRED_BSSID,
    IEEE80211_EV_CAC_EXPIRED,
#if QCA_LTEU_SUPPORT
    IEEE80211_EV_MU_RPT,
    IEEE80211_EV_SCAN,
#endif
#if QCA_AIRTIME_FAIRNESS
    IEEE80211_EV_ATF_CONFIG,
#endif
#if MESH_MODE_SUPPORT
    IEEE80211_EV_MESH_PEER_TIMEOUT,
#endif
    IEEE80211_EV_UNPROTECTED_DEAUTH_IND_STA,
    IEEE80211_EV_DISASSOC_COMPLETE_AP,
    IEEE80211_EV_ASSOC_REJECT,
    IEEE80211_EV_CSA_RX, // Reported when STA vap receives beacon/action frame with CSA IE
    IEEE80211_EV_RADAR_DETECTED,
    IEEE80211_EV_CAC_STARTED,
    IEEE80211_EV_CAC_COMPLETED,
    IEEE80211_EV_NOL_STARTED,
    IEEE80211_EV_NOL_FINISHED,
    IEEE80211_EV_AUTHORIZED_IND_STA,
    IEEE80211_EV_NEIGH_REQ_RECV_AP,
    IEEE80211_EV_PRECAC_STARTED,
    IEEE80211_EV_PRECAC_COMPLETED,
    IEEE80211_EV_PREFERRED_MLO_BSSID,
    /*
     * Any new addition of event should be added before max event and under
     * g_iwevcustom_ev_fl[] global event filter array, defined to filter configured events
     * and categorize to respective subevent and flag to deliver event to userspace.
     */
    IEEE80211_EV_MAX_EVENT, /* Max event is added to have a count of iwevcustom events */
};

#endif /* __linux__ */

#ifndef EXTERNAL_USE_ONLY
#define IEEE80211_VAP_PROFILE_NUM_ACL 64

struct rssi_info {
    u_int8_t avg_rssi;
    u_int8_t valid_mask;
    int8_t   rssi_ctrl[MAX_CHAINS];
    int8_t   rssi_ext[MAX_CHAINS];
};

struct ieee80211vap_profile  {
    struct ieee80211vap *vap;
    char name[IFNAMSIZ];
    u_int32_t opmode;
    u_int32_t phymode;
    char  ssid[IEEE80211_NWID_LEN];
    u_int32_t bitrate;
    u_int32_t beacon_interval;
    u_int32_t txpower;
    u_int32_t txpower_flags;
    struct rssi_info bcn_rssi;
    struct rssi_info rx_rssi;
    u_int8_t  vap_mac[IEEE80211_ADDR_LEN];
    u_int32_t  rts_thresh;
    u_int8_t  rts_disabled;
    u_int8_t  rts_fixed;
    u_int32_t frag_thresh;
    u_int8_t frag_disabled;
    u_int8_t frag_fixed;
    u_int32_t   sec_method;
    u_int32_t   cipher;
    u_int8_t wep_key[4][256];
    u_int8_t wep_key_len[4];
    u_int8_t  maclist[IEEE80211_VAP_PROFILE_NUM_ACL][IEEE80211_ADDR_LEN];
   	u_int8_t  node_acl;
    int  num_node;
    u_int8_t wds_enabled;
    u_int8_t wds_addr[IEEE80211_ADDR_LEN];
    u_int32_t wds_flags;
    u_int8_t txvap;
#ifdef WLAN_FEATURE_11BE_MLO
    struct ieee80211_mld_create_params mld_params;
#endif
};

struct ieee80211_profile {
    u_int8_t radio_name[IFNAMSIZ];
    u_int8_t channel;
    u_int32_t freq;
    u_int16_t cc;
    u_int8_t  radio_mac[IEEE80211_ADDR_LEN];
    struct ieee80211vap_profile vap_profile[IEEE80211_MAX_VAPS];
    int num_vaps;
};
#endif

/* FIPS Structures to be used by application */

#define FIPS_ENCRYPT 0
#define FIPS_DECRYPT 1

enum fips_mode {FIPS_MODE_ECB_AES=1, FIPS_MODE_CCM_GCM=2, FIPS_MODE_CCM_GCM_NIST=3};

struct ath_ioctl_fips {
    u_int32_t fips_cmd;/* 1 - Encrypt, 2 - Decrypt*/
    enum fips_mode mode;/*1 for AES_CTR and 2 for AES_CCM(DISA)*/
    u_int32_t key_idx;
    u_int32_t key_cipher;
    u_int32_t key_len;
#define MAX_KEY_LEN_FIPS 32
    u_int8_t  key[MAX_KEY_LEN_FIPS];
#define MAX_IV_LEN_FIPS  16
    u_int8_t iv[MAX_IV_LEN_FIPS];
    u_int32_t pn_len;
#define MAC_PN_LENGTH 8
    u_int32_t pn[MAC_PN_LENGTH];
    u_int32_t header_len;
#define MAX_HDR_LEN 40
    u_int8_t header[MAX_HDR_LEN];
    u_int32_t data_len;
    u_int32_t data[1];
};

struct ath_fips_output {
    u_int32_t error_status;
    u_int32_t data_len;
    u_int32_t data[1]; /* output from Fips Register*/
};

struct ath_ioctl_fips_extend_cmd_params {
    u_int32_t fips_cmd;     /* 0 - Encrypt, 1 - Decrypt*/
    u_int32_t key_cipher;   /* 0 - CCM , 1 - GCM */
    u_int32_t key_len;
#define MAX_KEY_LEN_FIPS_EXTEND 64
    u_int8_t  key[MAX_KEY_LEN_FIPS_EXTEND];
    u_int32_t nonce_iv_len;
#define MAX_NONCEIV_LEN_FIPS_EXTEND 16
    u_int8_t  nonce_iv[MAX_NONCEIV_LEN_FIPS_EXTEND];
    u_int32_t tag_len;        /* 8 or 16  */
    u_int32_t aad_len;
    u_int32_t payload_len;
};

struct ath_ioctl_fips_extend {
    u_int32_t recv;
    u_int32_t get;
    u_int32_t cookie;
    u_int32_t frag_idx;
    u_int32_t more_bit;
    u_int32_t data_len;

    struct ath_ioctl_fips_extend_cmd_params cmd_params;   /* first fragment */

    u_int32_t data[1]; /* (aad, payload in order) */
};


struct ath_fips_extend_output {
    u_int32_t recv;
    u_int32_t fips_cookie;
    u_int32_t cmd_frag_idx;   /* Fragment Number to refer to cmd frag idx */
    u_int32_t more_bit;       /* More bit is 0 for last fragment */
    u_int32_t error_status;
    u_int32_t data_len;
    u_int32_t data[1]; /* output from Fips Register */
};

#define FIPS_MODE_ENABLE 1
#define FIPS_MODE_DISABLE 0
struct ath_ioctl_fips_mode_set {
    u_int32_t fips_mode_set;   /* FIPS_MODE_ENABLE / FIPS_MODE_DISABLE */
};

#define IS_UP_AUTO(_vap) \
    (IS_UP((_vap)->iv_dev) && \
    (_vap)->iv_ic->ic_roaming == IEEE80211_ROAMING_AUTO)

#if QCA_LTEU_SUPPORT

#define MU_MAX_ALGO          4
#define MU_DATABASE_MAX_LEN  32

typedef enum {
    MU_STATUS_SUCCESS,
    /* errors encountered in initiating MU scan are as below */
    MU_STATUS_BUSY_PREV_REQ_IN_PROG,      /* returned if previous request for MU scan is currently being processed */
    MU_STATUS_INVALID_INPUT,              /* returned if MU scan parameter passed has an invalid value */
    MU_STATUS_FAIL_BB_WD_TRIGGER,         /* returned if hardware baseband hangs */
    MU_STATUS_FAIL_DEV_RESET,             /* returned if hardware hangs and driver needs to perform a reset to recover */
    MU_STATUS_FAIL_GPIO_TIMEOUT,          /* returned if GPIO trigger has timed out*/
} mu_status_t;

typedef enum {
    DEVICE_TYPE_AP,
    DEVICE_TYPE_STA,
    DEVICE_TYPE_SC_SAME_OPERATOR,
    DEVICE_TYPE_SC_DIFF_OPERATOR,
} mu_device_t;

typedef struct{
    /* specifying device type(AP/STA/SameOPClass/DiffOPClass)for each entry of the MU database*/
    mu_device_t mu_device_type;
    /* specifying BSSID of each entry */
    u_int8_t mu_device_bssid[IEEE80211_ADDR_LEN];
    /* Mac address of each entry */
    u_int8_t mu_device_macaddr[IEEE80211_ADDR_LEN];
    /* average packet duration for each device in micro secs to avoid decimals */
    u_int32_t mu_avg_duration;
    /* average rssi recorded for the device */
    u_int32_t mu_avg_rssi;
    /* percentage of medium utilized by the device */
    u_int32_t mu_percentage;
}mu_database;

struct event_data_mu_rpt {
    u_int8_t        mu_req_id;                                  /* MU request id, copied from the request */
    u_int8_t        mu_channel;                                 /* IEEE channel number on which MU was done */
    mu_status_t     mu_status;                                  /* whether the MU scan was successful or not */
    u_int32_t       mu_total_val[MU_MAX_ALGO-1];                /* the aggregate MU computed by the 3 algos */
    u_int32_t       mu_num_bssid;                               /* number of active BSSIDs */
    u_int32_t       mu_actual_duration;                         /* time in ms for which the MU scan was done */
    u_int32_t       mu_hidden_node_algo[LTEU_MAX_BINS];         /* The MU computed by the hidden node algo, reported on a per bin basis */
    u_int32_t       mu_num_ta_entries;                          /* number of active TA entries in the database */
    mu_database     mu_database_entries[MU_DATABASE_MAX_LEN];   /* the MU report for each TA */
};

typedef enum {
    SCAN_SUCCESS,
    SCAN_FAIL,
} scan_status_t;

struct event_data_scan {
    u_int8_t        scan_req_id;               /* AP scan request id, copied from the request */
    scan_status_t   scan_status;               /* whether the AP scan was successful or not */
};

#endif /* QCA_LTEU_SUPPORT */

struct ieee80211_smps_update_data {
   uint8_t is_static;
   uint8_t macaddr[IEEE80211_ADDR_LEN];
};

struct ieee80211_opmode_update_data{
   uint8_t max_chwidth;
   uint8_t num_streams;
   uint8_t macaddr[IEEE80211_ADDR_LEN];
};

struct channel_stats {
    uint8_t ieee;            /* Channel Number */
    uint32_t freq;           /* Channel frequency */
    int16_t chan_nf;         /* Channel noise floor */
    uint64_t cycle_cnt;      /* Cumulative sum of cycle cnt delta */
    uint64_t tx_frm_cnt;     /* Cumulative sum of tx frame cnt delta */
    uint64_t rx_frm_cnt;     /* Cumulative sum of rx frame cnt delta */
    uint64_t clear_cnt;      /* Cumulative sum of clear cnt delta */
    uint64_t ext_busy_cnt;   /* Cumulative sum of ext busy cnt delta */
    uint64_t bss_rx_cnt;     /* Cumulative sum of own bss rx cnt delta */
    uint32_t timestamp;      /* cycle_cnt is converted to ms */
};

struct spl_vap_chan_util {
    uint32_t freq;
    uint32_t chan_nf;
    int16_t noise_floor;
    uint8_t chan_util;
};


/**
 * struct ieee80211_hmwds_ast_add_status - hmwds ast add status
 * @cmd: dbg req cmd id
 * @peer_mac: peer mac address
 * @ast_mac: ast mac address
 * @status: ast add status
 * @args: arguments
 */
struct ieee80211_hmwds_ast_add_status {
    u_int8_t cmd;
    u_int8_t peer_mac[IEEE80211_ADDR_LEN];
    u_int8_t ast_mac[IEEE80211_ADDR_LEN];
    int      status;
} __attribute__ ((packed));

struct ieee80211_awgn_update_data{
    uint32_t primary_freq;
    uint32_t center_freq;
    uint32_t bandwidth;
    uint32_t bitmap;
};

#endif /* _NET80211_IEEE80211_IOCTL_H_ */
