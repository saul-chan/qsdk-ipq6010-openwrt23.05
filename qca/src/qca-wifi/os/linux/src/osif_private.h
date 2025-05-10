/*
 * Copyright (c) 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * copyright (c) 2010, Atheros Communications Inc.
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


#ifndef _OSIF_PRIVATE_H_
#define _OSIF_PRIVATE_H_

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/sysctl.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/sysfs.h>
#include <net/iw_handler.h>
#include <linux/wireless.h>
#include <linux/if_arp.h>		/* XXX for ARPHRD_* */
#if ATH_SUPPORT_FLOWMAC_MODULE
#include <net/pkt_sched.h>
#endif
#include <asm/uaccess.h>

#include "if_media.h"
#include "if_upperproto.h"

#include "ieee80211.h"
#include "_ieee80211.h"
#include <ieee80211_api.h>
#include "ieee80211_defines.h"
#include <ieee80211_ioctl.h>
#include <ieee80211_sme_api.h>
#include <ieee80211P2P_api.h>
#include <ieee80211.h>
#if UMAC_SUPPORT_CFG80211
#include <net/cfg80211.h>
#endif
#if UMAC_SUPPORT_ACFG
#include <acfg_api_types.h>
#endif
#ifdef ATH_SUPPORT_LINUX_VENDOR
#include <ioctl_vendor.h>      /* Vendor Include */
#endif

#if ATH_PERF_PWR_OFFLOAD
#include <cdp_txrx_cmn_struct.h>
#endif
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <nss_api_if.h>
#endif
#include <wlan_cm_public_struct.h>

#define OS_SIWSCAN_TIMEOUT ((12000 * HZ) / 1000)
#define SIWSCAN_TIME_ENH_IND_RPT	 5
#define OSIF_MAX_CONNECTION_ATTEMPT      3
#define OSIF_MAX_CONNECTION_STOP_TIMEOUT 20
#define OSIF_MAX_CONNECTION_TIMEOUT      0xFFFFFFFF
#define OSIF_MAX_DELETE_VAP_TIMEOUT      60
#define OSIF_MAX_DELETE_VAP_INIT_TIMEOUT 150
#define OSIF_MAX_STOP_VAP_TIMEOUT_CNT    6000
#define OSIF_MAX_START_VAP_TIMEOUT_CNT   300
#define OSIF_MAX_STOP_ML_VAP_TIMEOUT_CNT 30
#define OSIF_MAX_CHAN_CHANGE_TIMEOUT_OFFSET 10

#define OSIF_ACS_SCAN_TIMEOUT            ((CONVERT_SEC_TO_SYSTEM_TIME(1)/20) + 1) /*50 msec */
#define OSIF_CONNECTION_TIMEOUT          ((CONVERT_SEC_TO_SYSTEM_TIME(1)/20) + 1) /*50 msec */
#define OSIF_DELETE_VAP_TIMEOUT          CONVERT_SEC_TO_SYSTEM_TIME(1)
#define OSIF_DISCONNECT_TIMEOUT        10 /* 10 msec */
#define OSIF_STOP_VAP_TIMEOUT          5  /* 5 msec */
#define OSIF_VAP_UP_TIMEOUT            (30000)  /* 30 sec */
#define OSIF_VAPUP_TICK                (HZ/50) /* 20 ms */
#define OSIF_VAPUP_TIMEOUT_COUNT       150 /* 3 seconds(20 ms *150) */
#define OSIF_START_VAP_TIMEOUT         ((CONVERT_SEC_TO_SYSTEM_TIME(1)/100) + 1) /* 10 msec */
#define OSIF_PEER_DELETE_TIMEOUT       20 /* 20 msec */

/* make sure the above constant does not return 0 */
#define OSIF_TXCHANSWITCH_LOOPCOUNT     100
#define OSIF_PEER_DELETE_LOOP_COUNT     50

#define OSIF_VAP_STOP_TIMEBUFF_US     (5000)
#define OSIF_VAP_OPEN_TIMEBUFF_US     (5000)

#ifdef QCA_SUPPORT_WDS_EXTENDED
struct net_device*
osif_create_wds_ext_netdev(struct ieee80211com *ic, struct ieee80211_clone_params *cp);
struct net_device *osif_mld_phy_create_wds_ext_netdev(struct ieee80211_clone_params *cp);
int osif_mld_phy_configure_parent_param(struct net_device *dev, uint8_t *mac);
#endif

int osif_ioctl_create_vap(struct net_device *dev, struct ifreq *ifr,
							 struct ieee80211_clone_params *cp,
							 osdev_t os_handle);
struct net_device*
osif_create_mld_bond(struct wiphy* mld_wiphy, void *mld_params, const char *iname, enum nl80211_iftype type);
int
osif_destroy_mld_bond(struct net_device *mldev);
void*
osif_create_vap(struct ieee80211com *ic, struct net_device *comdev,
                struct ieee80211_clone_params *cp, int cfg80211_create,
                void *mld_params, enum virtual_interface_type vif_type);
bool
osif_brvap_is_bonding_mode(struct wlan_objmgr_psoc *psoc);

#ifdef WLAN_FEATURE_11BE_MLO
void osif_vap_bringdown_mlo(struct wlan_objmgr_vdev *vdev);
void osif_vap_check_blocking_cmd_on_mlo(struct wlan_objmgr_vdev *vdev);
void osif_vap_netdev_close_mlo(struct wlan_objmgr_vdev *vdev);
void osif_peer_update_mlo_sta_dev(struct wlan_objmgr_peer *peer,
                                  struct wlan_mlo_peer_context *ml_peer);
void wlan_set_ssid_mlo_bridge_vap(wlan_if_t vap);
int wlan_delete_mlo_bridge_vap(wlan_if_t vap, int recover);
QDF_STATUS osif_mlo_delete_bridge_vaps(void);
int wlan_init_mlo_bridge_vap(wlan_if_t vap);
#endif
int osif_ioctl_delete_vap(struct net_device *dev, uint8_t link_id);
#ifdef QCA_SUPPORT_WDS_EXTENDED
int osif_wds_ext_delete_netdev(struct net_device *dev);
#endif
int osif_ioctl_switch_vap(struct net_device *dev, enum ieee80211_opmode opmode);
void osif_attach(struct net_device *dev);
void osif_detach(struct net_device *dev);
void osif_notify_push_button(osdev_t osdev, u_int32_t push_time);
int wlan_vap_init(wlan_if_t vap);
int wlan_delete_vap_wait_and_free(wlan_if_t vap, int recover);
void vap_wait_for_vdev_init_state(struct wlan_objmgr_psoc *psoc,
                                  struct wlan_objmgr_vdev *vdev,
                                  int recover);
int osif_pdev_restart(struct ieee80211vap *vap);
int osif_vdev_restart(struct ieee80211vap *vap);
int osif_pdev_restart_vaps(struct ieee80211com *ic);
int osif_vap_roam(struct net_device *dev, struct ieee80211vap *vap);
int osif_vap_stop(struct net_device *dev);
int osif_restart_for_config(struct ieee80211com *ic,
			    int (*config_callback)(struct ieee80211com *ic, struct ieee80211_ath_channel *chan),
			    struct ieee80211_ath_channel *chan);
QDF_STATUS osif_vap_validate_link_sta(struct vdev_mlme_obj *vdev_mlme, void *ext_data);
QDF_STATUS osif_mlo_sta_clone_crypto_param(struct ieee80211vap *vap, struct wlan_cm_connect_req *req);
QDF_STATUS osif_mlo_sta_clone_security_param(struct vdev_mlme_obj *vdev_mlme, struct wlan_cm_connect_req *req);
QDF_STATUS osif_update_op_class_from_mlie(struct vdev_mlme_obj *vdev_mlme, uint8_t *ml_ie);
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
struct net_device *osif_vdev_get_mld_netdev(struct wlan_objmgr_vdev *vdev);
QDF_STATUS
osif_mlo_sta_connect_get_partner_info(struct wlan_objmgr_vdev *vdev,
                                      const struct cfg80211_connect_params *req,
                                      struct mlo_partner_info *partner_info);
#endif

/*
 * osif_num_ap_up_vaps() - Set frequency to the radio
 * @ic: ic object handle
 *
 * This function gets the number of AP VAPs brought up by the user space
 *
 * Return: Number of AP VAPs brought up by the user space
 */
int osif_num_ap_up_vaps(struct ieee80211com *ic);
void osif_deliver_data(os_if_t osif, struct sk_buff *skb);
int osif_vap_hardstart(struct sk_buff *skb,struct net_device *dev);
void osif_forward_mgmt_to_app(os_if_t osif, wbuf_t wbuf,
                                        u_int16_t type, u_int16_t subtype, ieee80211_recv_status *rs);
void osif_update_dfs_info_to_app(struct ieee80211com *ic, char *buf);

int osif_ioctl_get_vap_info (struct net_device *dev,
                            struct ieee80211_profile *profile);
void osif_cancel_acs_and_start_ap(void *arg, struct ieee80211vap *vap);
void delete_default_vap_keys(struct ieee80211vap *vap);
void osif_send_assoc_resp_ies(os_if_t osdev);
void ieee80211_update_assoc_ie(wlan_if_t vap, wbuf_t wbuf);
struct ol_ath_softc_net80211 *osif_validate_and_get_scn_from_dev(struct net_device *dev);
struct net_device *osif_get_mlo_link_netdev(struct net_device *dev, void *dst);
#if UMAC_SUPPORT_ACFG
int osif_set_vap_vendor_param(struct net_device *dev, acfg_vendor_param_req_t *req, uint8_t link_id);
int osif_get_vap_vendor_param(struct net_device *dev, acfg_vendor_param_req_t *req);
#endif

/**
* osif_vap_event_filter_status_to_app () –  Get the vap specific events configured
* filter status of the given event to delivering to application.
* @vap: VAP pointer
* @event_type: Type of event
* @event: Event index
*
* Return: Status of the event configured to deliver to app or not
*/
uint8_t osif_vap_event_filter_status_to_app(struct ieee80211vap *vap,
                                           uint8_t event_type, uint8_t event);

/**
* osif_scn_event_filter_status_to_app () –  Get the radio specific events configured
* filter status of the given event to delivering to application.
* @ic: pointer to ic context
* @event_type: Type of event
* @event: Event index
*
* Return: Status of the event configured to deliver to app or not
*/
uint8_t osif_scn_event_filter_status_to_app(struct ieee80211com *ic,
                                           uint8_t event_type,  uint8_t event);

QDF_STATUS osif_vap_wait_for_vdev_up_state(struct ieee80211vap *vap);
uint16_t osif_pick_tx_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev);
#if UMAC_PER_PACKET_DEBUG
int  osif_atoi_proc(char *buf);
ssize_t osif_proc_pppdata_write(struct file *filp, const char __user *buff,
                unsigned long len, void *data );
#endif

#if WLAN_SPECTRAL_ENABLE
int osif_ioctl_eacs(struct net_device *dev, struct ifreq *ifr, osdev_t os_handle);
#endif
#if ATH_SUPPORT_VLAN
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
void			qdf_net_vlan_attach(struct net_device *dev,struct net_device_ops *osif_dev_ops);
#else
void			qdf_net_vlan_attach(struct net_device *dev);
#endif
void			qdf_net_vlan_detach(struct net_device *dev);
#endif
struct wlan_objmgr_vdev *osif_get_vdev_by_ifname(struct wlan_objmgr_psoc *psoc,
                                                 char *ifname,
                                                 wlan_objmgr_ref_dbgid ref_id);

/* TGf l2_update_frame  format */
struct l2_update_frame
{
    struct ether_header eh;
    u_int8_t dsap;
    u_int8_t ssap;
    u_int8_t control;
    u_int8_t xid[3];
}  __packed;

#define IEEE80211_L2UPDATE_CONTROL 0xaf
#define IEEE80211_L2UPDATE_XID_0 0x81
#define IEEE80211_L2UPDATE_XID_1 0x01
#define IEEE80211_L2UPDATE_XID_2 0x0

#ifdef WLAN_FEATURE_FASTPATH
#define MAX_MSDUS_ACC           1
#define MAX_MSDUS_ACC_MASK      (MAX_MSDUS_ACC - 1)
#define MSDUS_ARRAY_SIZE        1
#define MSDUS_ARRAY_SIZE_MASK   (MSDUS_ARRAY_SIZE - 1)
#endif /* WLAN_FEATURE_FASTPATH */

#define WIFI_3_0_ULTRA_FASTPATH 0x2
#define WIFI_3_0_FASTPATH	0x1

enum netdev_type {
    OSIF_NETDEV_TYPE_VAP = 0,
    OSIF_NETDEV_TYPE_WDS_EXT,
    OSIF_NETDEV_TYPE_MLO
};

typedef struct _osif_dev {
    /*
     * dev_type should be first field for both osif_dev and osif_peer_dev
     * any new fields should be added after dev_type
     */
    uint8_t dev_type;
    osdev_t  os_handle;
    struct net_device *netdev;

#ifdef WLAN_FEATURE_FASTPATH
    qdf_nbuf_t  *nbuf_arr;      /* array to accumulate packets from the OS */
    uint16_t    nbuf_arr_pi;    /* producer index for nbuf_arr */
    uint16_t    nbuf_arr_ci;    /* consumer index for nbuf_arr */
    spinlock_t nbuf_arr_lock;   /* lock to access nbuf_arr (first cut) */
#endif /* WLAN_FEATURE_FASTPATH */
#if QCA_NSS_PLATFORM
    void *nss_redir_ctx;              /* QCA NSS platform specific handler */
#elif QCA_PPE_VP
    int32_t ppe_vp_num;              /* QCA NSS platform specific handler */
    int32_t ppe_vp_profile_idx;          /* QCA NSS platform specific handler */
    unsigned long ppe_vp_type;		/* QCA NSS platform specific handler */
#ifdef WLAN_SUPPORT_PPEDS
    uint32_t ppeds_node_id;		/* will be valid only when ppe_vp_type is DS */
#endif
#endif
    bool is_ar900b;		/* is this a beeliner or peregrine */

    struct wlan_objmgr_vdev *ctrl_vdev; /* UMAC vdev object pointer */
    uint8_t wifi3_0_fast_path;		/* Master flag for fast path
					   & slow path dev_ndo_start transmit functions */
    uint8_t wifi3_0_rx_fast_path;	/* Master flag for fast path
					   & slow path dev_ndo_start receive functions */
    wlan_if_t os_if;
    bool    osif_is_mode_offload;   /* = 1 for offload, = 0 for direct attach */
    struct list_head pending_rx_frames; /* stuff too big for wext events, send by ioctl */
    spinlock_t list_lock;

    wlan_dev_t os_devhandle;
    struct net_device *os_comdev;
    struct ifmedia os_media;    /* interface media config */
    u_int8_t  os_unit;
#if ATH_SUPPORT_VLAN
	struct vlan_group	*vlgrp;
	unsigned short		vlanID;
#endif
    wlan_connection_sm_t sm_handle;
    enum ieee80211_opmode os_opmode;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    struct rtnl_link_stats64 os_devstats;
#else
    struct net_device_stats os_devstats;
#endif
    struct iw_statistics    os_iwstats; /* wireless statistics block */

    /* cache for the security config */
    ieee80211_cipher_type uciphers[IEEE80211_CIPHER_MAX];
    u_int16_t u_count;
    ieee80211_cipher_type mciphers[IEEE80211_CIPHER_MAX];
    u_int16_t m_count;
    wlan_scan_requester         scan_requestor;
    wlan_scan_id                scan_id;
    u_int32_t       is_deleted:1,
                    is_ibss_create:1,
                    is_delete_in_progress:1,
                    is_restart:1,
                    use_p2p_go:1,
                    use_p2p_client:1,
                    is_p2p_interface:1, /* use_p2p_go | use_p2p_client */
                    is_bss_started:1,
                    disable_ibss_create:1,
                    is_scan_chevent:1;  /* Is delivery of Scan Channel Events enabled during
                                           802.11 scans. Applicable only for 11ac offload.
                                           Currently takes effect only for IEEE80211_M_HOSTAP
                                           operating mode since the Scan Channel Events
                                           are of use only in that mode.
                                           XXX - Add for other operating modes if use
                                           cases arise.
                                           XXX - Currently, the Events are not delivered
                                           upwards to the application layer. They are
                                           used internally in the driver. If delivery
                                           to application layer is desired, it can be
                                           added using IWEVCUSTOM. */

    u_int32_t       no_stop_disassoc:1;
    u_int32_t       peer_protocol_cnt:1;
    u_int8_t        os_giwscan_count;
    unsigned long   os_last_siwscan;    /* time last set scan request */
#define OSIF_SCAN_BAND_ALL            (0)
#define OSIF_SCAN_BAND_2G_ONLY        (1)
#define OSIF_SCAN_BAND_5G_ONLY        (2)
    u_int8_t        os_scan_band;       /* only scan channels of requested band */

    u_int8_t authmode;
    u_int32_t app_filter;
#ifdef ATHEROS_LINUX_PERIODIC_SCAN
#define OSIF_PERIODICSCAN_DEF_PERIOD        (0)
#define OSIF_PERIODICSCAN_MIN_PERIOD        (30000)
#define OSIF_PERIODICSCAN_COEXT_PERIOD      (180000)

    u_int32_t   os_periodic_scan_period;    /* 0 means off Periodic Scan */
    os_timer_t  os_periodic_scan_timer;
#endif
    spinlock_t  tx_lock; /* lock for the tx path */
#ifdef ATH_SUPPORT_LINUX_VENDOR
#define OSIF_VENDOR_RAWDATA_SIZE        (24)
    u_int8_t os_vendor_specific[OSIF_VENDOR_RAWDATA_SIZE];    /* Some vendor ioctl() maybe cache something. */
#endif
#if ATH_SUPPORT_IWSPY
    struct 	iw_spy_data 		  spy_data;	/* iwspy support */
#endif
#if ATH_SUPPORT_WAPI
    u_int32_t    os_wapi_rekey_period;
    os_timer_t    os_wapi_rekey_timer;
#endif
#if ATH_PERF_PWR_OFFLOAD
    void *iv_vap_send;
    void *iv_vap_fast_send;
    void *iv_vap_send_exc;
#endif /* ATH_PERF_PWR_OFFLOAD */
#if UMAC_VOW_DEBUG
    #define MAX_VOW_CLIENTS_DBG_MONITOR 8
    u_int8_t vow_dbg_en;
    unsigned long umac_vow_counter;
    unsigned long tx_dbg_vow_counter[MAX_VOW_CLIENTS_DBG_MONITOR];
    u_int8_t tx_dbg_vow_peer[MAX_VOW_CLIENTS_DBG_MONITOR][2];
    unsigned long tx_prev_pkt_count[MAX_VOW_CLIENTS_DBG_MONITOR];
    bool carrier_vow_config;
#endif
#if UMAC_SUPPORT_VI_DBG
    u_int8_t vi_dbg;
#endif

#if QCA_PARTNER_DIRECTLINK_TX
    bool is_directlink;
#endif /* QCA_PARTNER_DIRECTLINK_TX */

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    void *nss_wifiol_ctx;
    nss_if_num_t nss_ifnum;   /* NSS interface number associated with vap */
    spinlock_t queue_lock;    /* Lock for the vdev tx queue */
    struct sk_buff_head wifiol_txqueue; /* qwrap and extap tx/rx queues for bursty traffic handling */
    uint32_t stale_pkts_timer_interval;
    os_timer_t wifiol_stale_pkts_timer;
    uint8_t inspect_mode;    /* NSS vap inspect mode */
    uint32_t nssol_rxprehdr_read;
                            /* Enable rx preheader read for exceptioned packets */
    uint32_t nss_recov_if;  /* Nss interface for vap during recovery */
    bool nss_recov_mode;    /* Specifies recovery process (in_progress/Done) */
#endif
    /* keep cfg80211 in netdev priv to avoid iv_wdev
     * to go NULL
     */
#if UMAC_SUPPORT_CFG80211
    struct wiphy          *iv_wiphy; /* cfg80211 device */
    struct wireless_dev   iv_wdev;  /* wiress dev */
    struct cfg80211_scan_request *scan_req; /* scan request */
#endif
#define OSIF_VENDOR_FWD_MGMT_MASK_DEFAULT 0x2000 /* Only action frames, bit-13 */
    uint16_t wlan_vendor_fwd_mgmt_mask; /* Subtype mask */
    uint8_t is_bridge_link:1,
            mlo_link_id:5,
            reserved:2;
    struct net_device *rx_dev;
    struct osif_mldev *mldev;
#ifdef WLAN_FEATURE_11BE_MLO
    int (* mld_xmit)(struct sk_buff *skb, void *os_dev);
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    bool stop_initiated;
#endif
} osif_dev;

osif_dev *osif_get_mlsta_osif_dev(struct net_device *dev,
                                  struct cfg80211_connect_params *req);

void osif_notify_scan_done(osif_dev *osdev, enum scan_completion_reason reason);

#ifdef QCA_SUPPORT_WDS_EXTENDED

typedef struct _osif_peer_dev {
    /*
     * dev_type should be first field for both osif_dev and osif_peer_dev
     * any new fields should be added after dev_type
     */
    uint8_t dev_type;
    struct net_device *netdev;
    struct net_device *parent_netdev;
    uint16_t peer_id;
    uint8_t peer_mac_addr[QDF_MAC_ADDR_SIZE];
    struct wiphy      *wiphy; /* cfg80211 device */
    struct wireless_dev  wdev;  /* wiress dev */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    struct rtnl_link_stats64 os_devstats;
#else
    struct net_device_stats os_devstats;
#endif
#if QCA_NSS_PLATFORM
    void *nss_redir_ctx;              /* QCA NSS platform specific handler */
    /* #endif */
    /* #elif QCA_PPE_VP */
#elif QCA_PPE_VP
    int32_t ppe_vp_num;              /* QCA NSS platform specific handler */
    int32_t ppe_vp_profile_idx;          /* QCA NSS platform specific handler */
    unsigned long ppe_vp_type;		/* QCA NSS platform specific handler */
#ifdef WLAN_SUPPORT_PPEDS
    uint32_t ppeds_node_id;	/* will be valid only when ppe_vp_type is DS */
#endif
#endif
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    void *nss_wifiol_ctx; /* NSS context */
    nss_if_num_t nss_ifnum;	/* NSS interface number associated with this interface. */
#endif
    osif_dev *parent_osif;
    struct wlan_objmgr_vdev *vdev;
    uint8_t is_mlo:1,         /* Identifier of interface created for MLO peer */
            reserved:7;
} osif_peer_dev;
#endif

enum osif_cmd_type {
    OSIF_CMD_PDEV_START,
    OSIF_CMD_PDEV_STOP,
    OSIF_CMD_PDEV_STOP_START,
    OSIF_CMD_OTH_VDEV_STOP_START,
    OSIF_CMD_VDEV_START,
    OSIF_CMD_VDEV_STOP,
    OSIF_CMD_STA_VAP_STOP,
    OSIF_CMD_STA_VAP_STOP_START,
};

struct osif_cmd_sched_data {
    struct wlan_objmgr_vdev *vdev;
    enum osif_cmd_type cmd_type;
};

#define OFFLOAD       2

struct wifi_radio_t  {
    u_int8_t wifi_radio_type;
    void *sc;
};

#define NUM_MAX_RADIOS 5

struct g_wifi_info {
    u_int8_t num_radios;
    struct wifi_radio_t wifi_radios[NUM_MAX_RADIOS];
};

struct wlan_find_vdev_filter
{
        char *ifname;
        struct wlan_objmgr_vdev *found_vdev;
};

#define OSIF_WAPI_REKEY_TIMEOUT 5000  /*ms*/

#ifndef ETH_P_80211_RAW
#define ETH_P_80211_RAW (ETH_P_ECONET + 1)
#endif

enum
{
        DIDmsg_lnxind_wlansniffrm               = 0x00000044,
        DIDmsg_lnxind_wlansniffrm_hosttime      = 0x00010044,
        DIDmsg_lnxind_wlansniffrm_mactime       = 0x00020044,
        DIDmsg_lnxind_wlansniffrm_channel       = 0x00030044,
        DIDmsg_lnxind_wlansniffrm_rssi          = 0x00040044,
        DIDmsg_lnxind_wlansniffrm_sq            = 0x00050044,
        DIDmsg_lnxind_wlansniffrm_signal        = 0x00060044,
        DIDmsg_lnxind_wlansniffrm_noise         = 0x00070044,
        DIDmsg_lnxind_wlansniffrm_rate          = 0x00080044,
        DIDmsg_lnxind_wlansniffrm_istx          = 0x00090044,
        DIDmsg_lnxind_wlansniffrm_frmlen        = 0x000A0044,
        DIDmsg_lnxind_wlansniffrm_rate_phy1     = 0x000B0044,
        DIDmsg_lnxind_wlansniffrm_rate_phy2     = 0x000C0044,
        DIDmsg_lnxind_wlansniffrm_rate_phy3     = 0x000D0044,
};

enum
{
    P80211ENUM_msgitem_status_no_value  = 0x00
};
enum
{
    P80211ENUM_truth_false              = 0x00
};

/*
 * Some data is too big to return in a wireless extension event.
 * So instead, the event signals the availability of data an IOCTL is used
 * to fetch data - which calls the fetch_p2p_mgmt list access routine.
 */
struct pending_rx_frames_list {
    struct list_head list;
    struct wlan_p2p_rx_frame rx_frame; /* data to be returned via ioctl */
    u_int32_t  freq;        /* pre-pend frequency to data on fx_frames */
    u_int32_t  frame_type;  /* pre-pend type to data */
    int    extra[1];        /* variable length extra data */
};
struct pending_rx_frames_list *osif_fetch_p2p_mgmt(struct net_device *dev);
#ifdef HOST_OFFLOAD
int osif_is_pending_frame_list_empty(struct net_device *dev);
#endif
void osif_p2p_rx_frame_handler(osif_dev *osifp, wlan_p2p_event *event,
                                int frame_type);

void *osif_get_mld_netdev(osif_dev *osifp);

int osif_vap_init(osif_dev *osifp, struct net_device *dev, int forcescan);
int osif_vap_pre_init(osif_dev *osifp, struct net_device *dev, int forcescan);
int osif_vap_pre_stop(osif_dev *osifp);
int osif_vap_stop_wrapper(osif_dev *osifp);
void osif_vap_netdev_close_mbssid(osif_dev *osifp);

#ifdef LIMIT_MTU_SIZE
#define LIMITED_MTU 1368
#include <net/icmp.h>
#include <linux/icmp.h>
#include <net/route.h>


#define LIMIT_PATH_MTU_TX(_skb,_Fake_rtable)                            \
        if (_skb->len >=  LIMITED_MTU + ETH_HLEN) {                     \
            _skb->h.raw = _skb->nh.raw = _skb->data +ETH_HLEN ;         \
            _skb->dst = (struct dst_entry *)&_Fake_rtable;              \
            _skb->pkt_type = PACKET_HOST;                               \
            dst_hold(_skb->dst);                                        \
            icmp_send(_skb, ICMP_DEST_UNREACH,                          \
                     ICMP_FRAG_NEEDED, htonl(LIMITED_MTU - 4 ));        \
            qdf_nbuf_free(_skb);                                    \
            return 0;                                                   \
        }

#define LIMIT_PATH_MTU_RX(_skb,_Fake_rtable)                            \
        if (_skb->len >=  LIMITED_MTU) {                                \
            _skb->h.raw = _skb->nh.raw = _skb->data  ;                  \
            _skb->dst = (struct dst_entry *)&_Fake_rtable;              \
            _skb->pkt_type = PACKET_HOST;                               \
            dst_hold(_skb->dst);                                        \
            icmp_send(_skb, ICMP_DEST_UNREACH,                          \
                     ICMP_FRAG_NEEDED, htonl(LIMITED_MTU - 4 ));        \
            qdf_nbuf_free(_skb);                                    \
            return;                                                     \
        }
#endif

#define IS_UP(_dev) \
    (_dev && (((_dev)->flags & (IFF_RUNNING|IFF_UP)) == (IFF_RUNNING|IFF_UP)))
#define RESCAN  0x01

#define NETDEV_TO_VAP(_dev) (((osif_dev *)netdev_priv(_dev))->os_if)
#define NETDEV_TO_VDEV(_dev) (((osif_dev *)netdev_priv(_dev))->ctrl_vdev)
#define OSIF_TO_NETDEV(_osif) (((osif_dev *)(_osif))->netdev)

#define nbuf_debug_add_record(_skb) \
    {\
        qdf_nbuf_frag_count_inc(_skb); \
        qdf_net_buf_debug_acquire_frag(_skb, __func__, __LINE__); \
        qdf_nbuf_count_inc(_skb); \
        qdf_net_buf_debug_acquire_skb(_skb, __func__, __LINE__); \
    }
#define nbuf_debug_del_record(_skb) \
    {\
        qdf_nbuf_frag_count_dec(_skb); \
        qdf_net_buf_debug_release_frag(_skb, __func__, __LINE__); \
        qdf_nbuf_count_dec(_skb); \
        qdf_net_buf_debug_release_skb(_skb); \
    }

#if ATH_PERF_PWR_OFFLOAD
/* Packet format configuration for a given interface or radio.
 * This must correspond to ordering in the enumeration htt_pkt_type.
 */
enum osif_pkt_type {
    osif_pkt_type_raw = 0,
    osif_pkt_type_native_wifi = 1,
    osif_pkt_type_ethernet = 2,
};
#endif /* ATH_PERF_PWR_OFFLOAD */

#define OSIF_NETDEV_TYPE_DA 0
#define OSIF_NETDEV_TYPE_OL 1
#define OSIF_NETDEV_TYPE_NSS_WIFIOL 2
#define OSIF_NETDEV_TYPE_OL_WIFI3 3
int osif_register_dev_ops_xmit(int (*vap_hardstart)(struct sk_buff *skb, struct net_device *dev), int netdev_type);

#ifdef QCA_SUPPORT_WDS_EXTENDED
int osif_register_wds_ext_dev_ops_xmit(int (*vap_hardstart)(struct sk_buff *skb, struct net_device *dev), int netdev_type);
static inline osif_dev *osif_wds_ext_get_parent_osif(osif_peer_dev *osifp)
{
    return osifp->parent_osif;
}

#endif

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef QCA_SUPPORT_WDS_EXTENDED
QDF_STATUS
osif_mlo_bridge_peer_wds_ext_migrate(struct ieee80211vap *vap);
#endif
bool osif_mldev_bridge_link_add(osif_dev *osifp_br, struct osif_mldev *mldev);
void osif_mldev_bridge_link_delete(osif_dev *osifp_br);
struct net_device *osif_mld_create(struct wiphy* mld_wiphy,
                                   void *mld_params,
                                   const char *ifname,
                                   enum nl80211_iftype type);
int osif_mld_destroy(struct net_device *dev);
void osif_mld_link_add(struct net_device *mldev, osif_dev *osifp, uint8_t link_id);
void osif_mld_link_delete(osif_dev *osifp);
struct osif_mldev *osif_mld_get_mldev(struct net_device *mldev);
struct net_device *osif_mld_get_rx_ndev(struct net_device *mldev,
                                        struct net_device *dev);
struct net_device *osif_get_mld_peer_rx_netdev(struct net_device *mldev,
                                               const uint8_t *mac_addr);
struct net_device *
osifp_peer_find_hash_find_netdev(struct osif_mldev *mldev,
                                 uint8_t *mac_addr);
osif_dev *osifp_peer_find_hash_find_osdev(struct osif_mldev *mldev,
                                          uint8_t *mac_addr);
osif_dev *osif_sta_mlo_find_osdev(struct osif_mldev *mldev);
void osif_mldev_register_ops(bool is_ap, uint32_t mode);
void osif_mldev_unregister_ops(void);
void osif_mldev_vlan_attach(struct net_device *mldev);
void osif_mldev_reconfig_link_add(struct net_device *dev, uint8_t *mld_mac, uint8_t link_id);
void osif_mldev_reconfig_link_delete(struct net_device *dev, uint8_t link_id);
void osif_mld_reconfig_link_add_start_ap(struct net_device *dev,
                                         uint8_t *mld_mac, uint8_t link_id);
void osif_mld_reconfig_link_delete_stop_ap(struct net_device *dev, uint8_t link_id);
int osif_ol_vap_hardstart_wifi3_wrapper(struct sk_buff *skb, void *os_dev);
struct net_device * osif_find_mld_dev_for_all_groups(uint8_t *mld_macaddr);
#if MESH_MODE_SUPPORT
void osif_update_mesh_mld_param(osif_dev *osdev);
#endif
#endif

int  osif_ol_wrap_tx_process(osif_dev **osifp, struct ieee80211vap *vap, struct sk_buff **skb);
int  osif_ol_wrap_rx_process(osif_dev **osifp, struct net_device **dev, wlan_if_t vap, struct sk_buff *skb);
void osif_mon_add_radiotap_header(os_if_t osif, struct sk_buff *skb, ieee80211_recv_status *rs);
void osif_mon_add_prism_header(os_if_t osif, struct sk_buff *skb, ieee80211_recv_status *rs);

#define PREEMPT_SCAN_MAX_GRACE (100) /* units in msec */
#define PREEMPT_SCAN_MAX_WAIT  (100) /* units in msec */
void preempt_scan(struct net_device *dev, int max_grace, int max_wait);

void osif_deliver_tx_capture_data(osif_dev *osifp, struct sk_buff *skb);

void osif_deliver_lite_mon_data(osif_dev *osifp, struct sk_buff *skb);

void osif_restore_vaps(struct ieee80211com *ic, wlan_if_t vap);

void osif_deliver_data_ol(os_if_t osif, struct sk_buff *skb_list);
#ifdef IPA_OFFLOAD
void osif_deliver_data_ol_ipa_offload(struct sk_buff *skb_list, struct net_device *dev);
#endif

#ifdef WLAN_FEATURE_11BE_MLO
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
/**
 * struct cfg80211_mlo_info - MLO settings
 *
 * Used to configure AP MLO Interface
 *
 * @num_mlo_links: number of MLO links.
 * @reconfig: whether reconfiguration or not
 * @mlo_link_ids: Array of link ids.
 * @mlo_mac_addrs: Array of MLO MAC address.
 */
#define MAX_NUM_MLO_LINKS 16
struct cfg80211_mlo_info {
        u8 num_mlo_links;
        bool reconfig;
        u32 mlo_link_ids[MAX_NUM_MLO_LINKS];
        struct mac_address mlo_mac_addrs[MAX_NUM_MLO_LINKS];
};
#endif

#define MAX_PDEV_PER_SOC 2
struct osifp_peer {
       uint8_t mac_addr[6];
       uint8_t primary_chip_id;
       uint8_t pdev_id;
       uint8_t vdev_id;
       bool logically_deleted;
       bool is_bridge_peer;
       TAILQ_ENTRY(osifp_peer) hash_list_elem;
};

#define MAX_POSSIBLE_BRIDGE_VAPS_PER_MLD 16
#define MAX_POSSIBLE_LINK_VAPS_PER_MLD 16

/* link vaps + bridge vaps */
#define MLD_MAX_NUM_VAPS (MAX_POSSIBLE_LINK_VAPS_PER_MLD + MAX_POSSIBLE_BRIDGE_VAPS_PER_MLD)

struct osif_mldev {
       uint8_t dev_type;
       struct net_device *mld_dev;
       struct wiphy *wiphy;
       struct wireless_dev wdev;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
       osif_dev *link_vifs[IEEE80211_MLD_MAX_NUM_LINKS]; /* Protocol Links */
       atomic_t num_link_vaps;
       struct cfg80211_mlo_info mlo_info;
       struct cfg80211_scan_request *mld_scan_req;
       struct cfg80211_scan_request *request_copy[IEEE80211_MLD_MAX_NUM_LINKS];
       struct ieee80211_clone_params cp;
       struct proc_dir_entry *mld_proc;
#endif
       osif_dev *link_vaps[MLD_MAX_NUM_VAPS]; /* Data path Links */
       osif_dev *link_dev[MAX_MLO_CHIPS][MAX_PDEV_PER_SOC];
       osif_dev *bridge_dev[MAX_MLO_CHIPS][MAX_PDEV_PER_SOC];
       struct {
               unsigned mask;
               unsigned idx_bits;
               TAILQ_HEAD(, osifp_peer) *bins;
       } peer_hash;
       qdf_spinlock_t peer_hash_lock;
       uint8_t primary_chip_id;
       uint8_t primary_pdev_id;
       uint64_t pkt_fail_cnt;
       uint8_t is_raw_mld:1,
               is_mesh_mld:1,
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
               is_valid_cp:1,
               reserved:5;
#else
               reserved:6;
#endif
       void *mld;
       uint8_t mld_mac_addr[QDF_MAC_ADDR_SIZE];
       uint8_t derived_mld_addr;
#if QCA_PPE_VP
       int32_t ppe_vp_num;
       unsigned long ppe_vp_type;
#endif
       uint8_t index;
};
#endif

union osif_align_mac_addr {
        uint8_t raw[QDF_MAC_ADDR_SIZE];
        struct {
                uint16_t bytes_ab;
                uint16_t bytes_cd;
                uint16_t bytes_ef;
        } align2;
        struct {
                uint32_t bytes_abcd;
                uint16_t bytes_ef;
        } align4;
        struct __attribute__((__packed__)) {
                uint16_t bytes_ab;
                uint32_t bytes_cdef;
        } align4_2;
};

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
struct ieee80211vap *get_sta_link_vap_by_freq(struct osif_mldev *osifp_mldev,
                                              unsigned int freq);
osif_dev *get_sta_osif_dev_by_freq(struct osif_mldev *osifp_mldev,
                                   unsigned int freq);
#endif

#if QCA_MANUAL_TRIGGERED_ULOFDMA
typedef struct {
    uint32_t num_subbufs;            /* num of subbuf of chan buffer */
    uint32_t subbuf_size;            /* size of each subbuf */
    qdf_streamfs_chan_t chan_ptr;    /* chan buf ptr */
    qdf_dentry_t dir_ptr;            /* chan buf dir ptr */
} ol_man_trig_ulofdma_ctx;

QDF_STATUS
ol_manual_trig_ulofdma_streamfs_init(struct wlan_objmgr_pdev *pdev);

QDF_STATUS
ol_manual_trig_ulofdma_streamfs_remove(struct wlan_objmgr_pdev *pdev);

QDF_STATUS
ol_manual_trig_ulofdma_streamfs_write(ol_man_trig_ulofdma_ctx *ulofdma_ctx,
                                      const void *write_data, size_t write_len);

QDF_STATUS
ol_manual_trig_ulofdma_streamfs_flush(ol_man_trig_ulofdma_ctx *ulofdma_ctx);
QDF_STATUS
ol_manual_trig_ulofdma_streamfs_send(ol_man_trig_ulofdma_ctx *ulofdma_ctx,
                                    const void *comm_info, size_t comm_info_len,
                                    const void *peer_info, size_t peer_info_len);
#endif
uint16_t osif_get_hard_header_len_from_osdev(osif_dev *osdev);
uint16_t osif_get_needed_headroom_from_osdev(osif_dev *osdev);
#endif /* _OSIF_PRIVATE_H_ */
