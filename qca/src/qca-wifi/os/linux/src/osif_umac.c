/*
* Copyright (c) 2011-2014,2017-2024 Qualcomm Innovation Center, Inc.
* All Rights Reserved
* Confidential and Proprietary - Qualcomm Innovation Center, Inc.
*
* 2011-2014 Qualcomm Atheros, Inc.
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*
* Copyright (c) 2010-2011, Atheros Communications Inc.
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
*
*/
#ifdef IPA_OFFLOAD
#include "wlan_ipa_ucfg_api.h"
#endif
#include "osif_private.h"
#include <wlan_opts.h>
#include <ieee80211_var.h>
#include <dp_extap.h>
#include <ieee80211_acfg.h>
#include <acfg_drv_event.h>
#include <ieee80211_cfg80211.h>
#if MESH_MODE_SUPPORT
#include <if_meta_hdr.h>
#endif
#include <qdf_nbuf.h> /* qdf_nbuf_map_single */
#include <qdf_platform.h>
#include <net/osif_net.h>

#include "wlan_osif_priv.h"
#include <qdf_perf.h>

#include "ath_netlink.h"

#include "scheduler_api.h"
#include "ieee80211_ev.h"

#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/seq_file.h>
#include "ieee80211_ioctl_acfg.h"
#include <ieee80211_vi_dbg.h>

#include <qdf_types.h>
#include <qdf_parse.h>
#ifdef UMAC_SUPPORT_PROXY_ARP
int wlan_proxy_arp(wlan_if_t vap, wbuf_t wbuf);
#endif

#include <linux/ethtool.h>

#if UNIFIED_SMARTANTENNA
#include <wlan_sa_api_utils_api.h>
#include <wlan_sa_api_utils_defs.h>
#endif

#include <ieee80211_nl.h>

#if ATH_SUPPORT_WRAP
#include "ieee80211_api.h"
#endif
#if ATH_PERF_PWR_OFFLOAD
#include <ol_cfg_raw.h>
#include <osif_rawmode.h>
#include <ol_if_athvar.h>
#endif /* ATH_PERF_PWR_OFFLOAD */
#include <target_type.h>
#include "ol_ath.h"
#include <linux/ethtool.h>
#include <osif_ol.h>
#include "dispatcher_init_deinit.h"
#include <ieee80211_objmgr_priv.h>
#include <qdf_threads.h>
#include <wlan_offchan_txrx_api.h>
#include <ieee80211_radiotap.h>
#include <ieee80211_ucfg.h>
#include <wlan_mlme_dp_dispatcher.h>
#include "cfg_ucfg_api.h"
#include "cfg_dispatcher.h"
#if ATH_SUPPORT_ZERO_CAC_DFS
#include <wlan_lmac_if_api.h>
#endif
#ifdef QCA_OL_TX_MULTIQ_SUPPORT
    #define OSIF_DP_NETDEV_TX_QUEUE_NUM 4
#else
    #define OSIF_DP_NETDEV_TX_QUEUE_NUM 1
#endif

#ifdef QCA_OL_RX_MULTIQ_SUPPORT
    #define OSIF_DP_NETDEV_RX_QUEUE_NUM 4
#else
    #define OSIF_DP_NETDEV_RX_QUEUE_NUM 1
#endif

#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif /* QCA_SUPPORT_CP_STATS */

#include <wlan_dfs_mlme_api.h>
#include <ieee80211_regdmn_dispatcher.h>
#include <wlan_reg_services_api.h>
#include <wlan_vdev_mlme_ser_if.h>
#include <wlan_mlme_vdev_mgmt_ops.h>
#include <wlan_vdev_mgr_ucfg_api.h>
#include <qal_notifier.h>
#include <wlan_mlme_if.h>
#include <wlan_psoc_mlme.h>
#include <wlan_psoc_mlme_main.h>
#include <wlan_psoc_mlme_api.h>
#include "ol_ratectrl_11ac_types.h"
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link.h>
#include <qca_multi_link_tbl.h>
#endif

#ifdef QCA_SUPPORT_WDS_EXTENDED
#include <cdp_txrx_cmn.h>
#endif /* QCA_SUPPORT_WDS_EXTENDED */
#include <wlan_if_mgr_api.h>
#include <wlan_mbss.h>
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
#include <wlan_telemetry_agent.h>
#endif
#include <ieee80211_acs.h>
#include <ieee80211_acs_internal.h>
#include <wlan_repeater_api.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_sta.h>
#include "ieee80211_crypto_nlshim_api.h"
#ifdef CONFIG_SAWF_DEF_QUEUES
#include <wlan_sawf.h>
#endif
#if QCA_AIRTIME_FAIRNESS
#include <wlan_atf_utils_api.h>
#endif
#include <wlan_objmgr_global_obj_i.h>
#include <wlan_cfg80211.h>
#ifdef CONFIG_BOND_MOD_SUPPORT
#include <qal_bond.h>
#endif
#include <pld_common.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <cdp_txrx_mlo.h>
#include "wlan_mlo_mgr_peer.h"
#endif
#include <wlan_reg_channel_api.h>
#include <cdp_txrx_ctrl.h>
#include <qca_wifi_mlo_metadata_info.h>

#if QCA_PPE_VP
#include <ppe_vp_public.h>
#include <ppe_vp_tx.h>

static const char *osif_vp_type_str[PPE_VP_USER_TYPE_MAX] = { "none",
                                                              "passive",
                                                              "active",
                                                              "ds" };
#endif

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
#include "wlan_cfg80211_scan.h"
#endif

/* ahbskip usage  : Skip registering direct attach devices only when set to 1 */
unsigned int ahbskip = 0;
module_param(ahbskip, int, 0644);

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef CONFIG_BOND_MOD_SUPPORT
static struct mlo_bond_info qca_bond_cb;
#endif
QDF_STATUS osif_mlo_peer_find_hash_attach(struct osif_mldev *mldev, int hash_elems);
QDF_STATUS osif_mlo_peer_find_hash_detach(struct osif_mldev *mldev);
int osif_ap_mlo_xmit_netdev_bond(struct sk_buff *skb, struct net_device *bond_dev);
int osif_ap_mlo_xmit_netdev(struct sk_buff *skb, struct net_device *bond_dev);
struct net_device *osif_get_ap_mlo_tx_netdev(void *bond_mlo_ctx, void *dst);
int osif_sta_mlo_xmit_netdev_bond(struct sk_buff *skb, struct net_device *bond_dev);
int osif_sta_mlo_xmit_netdev(struct sk_buff *skb, struct net_device *bond_dev);
struct net_device *osif_get_sta_mlo_tx_netdev(void *bond_mlo_ctx, void *dst);
#endif

#if QCA_SUPPORT_SON
struct chan_change_evt_t {
    u_int8_t channel;
    u_int16_t freq;
    u_int16_t puncture_bitmap;
    u_int16_t reg_punc_bitmap;
};
#endif

/*
 * fw_rec_dbg_mask usage: Downgrades the QDF debug print priority during FW
 * recovery
 */
int fw_rec_dbg_mask = 0;
qdf_declare_param(fw_rec_dbg_mask, int);

uint8_t used_mac_addr_byte[3] = { 0 };
#if ATH_SUPPORT_WRAP
#include "dp_wrap.h"
#endif
#include <wlan_repeater_api.h>

extern int osif_delete_vap_wait_and_free(osif_dev *osnetdev, int recover);
extern int ol_ath_ucfg_get_vap_info(struct ol_ath_softc_net80211 *scn,
                                    struct ieee80211_profile *profile);
extern int
wlan_get_vap_info(struct ieee80211vap *vap,
                  struct ieee80211vap_profile *vap_profile,
                  void *handle);
extern int
wlan_get_vdev_dp_stats(struct ieee80211vap *vap,
                       struct ieee80211_stats *vap_stats_usr,
                       struct ieee80211_mac_stats *ucast_stats_usr,
                       struct ieee80211_mac_stats *mcast_stats_usr);

static inline void osif_dev_set_offload_tx_csum(struct net_device *dev);
static inline void osif_dev_set_tso(struct net_device *dev);
static inline void osif_dev_set_sg(struct net_device *dev);

#ifdef WLAN_FEATURE_11BE_MLO
extern struct cfg80211_mld_wiphy_context mld_wiphy_ctx[WLAN_MAX_MLO_GROUPS];
#endif
#ifdef QCA_SUPPORT_WDS_EXTENDED
extern int
wlan_get_peer_dp_stats(struct ieee80211com *ic,
                       struct wlan_objmgr_peer *peer,
                       struct ieee80211_nodestats *ni_stats_user,
                       const uint8_t *mld_mac, bool is_ml_peer);
extern int
wlan_update_peer_cp_stats(struct ieee80211_node *ni,
                          struct ieee80211_nodestats *ni_stats_user);
#endif /* QCA_SUPPORT_WDS_EXTENDED */

extern int wlan_vap_delete_complete(struct ieee80211vap *vap);
static void ic_cancel_obss_nb_ru_tolerance_timer(struct ieee80211com *ic);

#ifdef QCA_PPE_VP
extern bool osif_ppe_vp_set_mtu(struct net_device *dev,
                                int32_t ppe_vp_num,
                                int mtu);
extern bool osif_ppe_vp_set_mac_addr(struct net_device *dev,
                                     int32_t ppe_vp_num,
                                     uint8_t *mac_addr);
#ifdef QCA_SUPPORT_WDS_EXTENDED
extern bool osif_ppe_vp_ext_vap_set_mtu(osif_peer_dev *osifp, int mtu);
extern bool osif_ppe_vp_ext_vap_set_mac_addr(osif_peer_dev *osifp, uint8_t *mac_addr);
#endif
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include "osif_nss_wifiol_if.h"
#include "osif_nss_wifiol_vdev_if.h"
#endif

#if QCA_PARTNER_DIRECTLINK_TX
#define QCA_PARTNER_DIRECTLINK_OSIF_UMAC 1
#include "ath_carr_pltfrm.h"
#undef QCA_PARTNER_DIRECTLINK_OSIF_UMAC
#endif /* QCA_PARTNER_DIRECTLINK_TX */

#if ATH_PARAMETER_API
#include "ath_papi.h"
#endif

#if !PEER_FLOW_CONTROL_FORCED_MODE0
#define TXOVERFLOW_PRINT_LIMIT 100
#endif
#include <wlan_dfs_ucfg_api.h>
#include <wlan_utility.h>
#include <wlan_mlme_if.h>
#include <wlan_cm_api.h>
#include <osif_cm_util.h>

#ifdef WLAN_FEATURE_11BE_MLO
#define MAX_MLDEV MAX_OSIF_MLDEV
enum {
    OSIF_MLDEV_OP_AP,
    OSIF_MLDEV_OP_STA,
    OSIF_MLDEV_OP_MAX,
};
struct net_device *osif_find_mld_dev(uint8_t *mld_macaddr, uint8_t grp_id);
struct net_device * osif_find_mld_dev_for_all_groups(uint8_t *mld_macaddr);
static void osif_mldev_link_add(struct net_device *mldev, osif_dev *osifp, uint8_t link_id);
static void osif_mldev_link_delete(osif_dev *osifp);
struct net_device *osif_find_mld_dev_by_ifname(char *ifname);
#define DP_XMIT_MLD 1
#endif

int osif_vap_stop(struct net_device *dev);
void ieee80211_proc_vattach(struct ieee80211vap *vap);
void ieee80211_proc_vdetach(struct ieee80211vap *vap);
#if IEEE80211_DEBUG_NODELEAK
void wlan_debug_dump_nodes_tgt(void);
#endif
void acs_bk_scan_cmpl_handler(void *data);

#if MESH_MODE_SUPPORT
void (*external_tx_complete)(struct sk_buff *skb) = NULL;
qdf_export_symbol(external_tx_complete);

#endif

struct ath_softc_net80211 *global_scn[GLOBAL_SCN_SIZE] = {NULL};
int num_global_scn = 0;

ol_ath_soc_softc_t *ol_global_soc[GLOBAL_SOC_SIZE] = {NULL};
int ol_num_global_soc = 0;

/*
 * This version symbol is added just to make wmi consolidated report
 * generation tool happy, we are not worried about contents.
 */
#define WLAN_VERSION_SIZE 8
uint8_t g_wlan_driver_version[WLAN_VERSION_SIZE] = "11.x";
qdf_export_symbol(g_wlan_driver_version);

unsigned int enable_mesh_peer_cap_update = 0;
module_param(enable_mesh_peer_cap_update, uint, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
qdf_export_symbol(enable_mesh_peer_cap_update);

#if QLD
/*
 * 0: means feature is disabled
 * Non zero: list size defined by qld_max_list
 */
unsigned int qld_max_list  = 0;
module_param(qld_max_list, uint, 0644);
qdf_export_symbol(qld_max_list);
#endif

unsigned int g_unicast_deauth_on_stop = 0;
module_param(g_unicast_deauth_on_stop, int, 0644);
qdf_export_symbol(g_unicast_deauth_on_stop);

/* module param to control max wait time for receiveing higher
 * channel width data frames. After time timeout, host will
 * stop checking rx frame bw to upgrade STA chan width.
 * Default is 60 seconds.
 */
unsigned int g_csa_max_rx_wait_time = 60000;
module_param(g_csa_max_rx_wait_time, int, 0644);
qdf_export_symbol(g_csa_max_rx_wait_time);

#ifdef QCA_WIFI_MODULE_PARAMS_FROM_INI
#define UMAC_PARAM_STR_LENGTH 40

extern uint32_t atf_mode;
extern uint32_t netlink_acfg;
extern uint32_t netlink_ath_ssid_event;
extern uint32_t enable_pktlog_support;
extern uint32_t netlink_band_steering_event;
extern uint32_t sa_api_mode;
extern uint32_t netlink_son_ald;

enum umac_num_module_param {
    PARAM_AHBSKIP,
    PARAM_G_UNICAST_DEAUTH_ON_STOP,
    PARAM_ATF_MODE,
    PARAM_NETLINK_ACFG,
    PARAM_ENABLE_MESH_PEER_CAP_UPDATE,
    PARAM_NETLINK_ATH_SSID_EVENT,
    PARAM_ENABLE_PKTLOG_SUPPORT,
    PARAM_NETLINK_BAND_STEERING_EVENT,
    PARAM_FW_REC_DBG_MASK,
    PARAM_QLD_MAX_LIST,
    PARAM_G_CSA_MAX_RX_WAIT_TIME,
    PARAM_SA_API_MODE,
    PARAM_NETLINK_SON_ALD,
    UMAC_PARAM_MAX,
};

static char umac_module_param[UMAC_PARAM_MAX][UMAC_PARAM_STR_LENGTH] = {
    "ahbskip",
    "g_unicast_deauth_on_stop",
    "atf_mode",
    "netlink_acfg",
    "enable_mesh_peer_cap_update",
    "netlink_ath_ssid_event",
    "enable_pktlog_support",
    "netlink_band_steering_event",
    "fw_rec_dbg_mask",
    "qld_max_list",
    "g_csa_max_rx_wait_time",
    "sa_api_mode",
    "netlink_son_ald",
};

/**
 * umac_module_param_handler() - update umac module param
 *
 * This function receives any one of umac module param and its value as input
 *
 * Return : QDF_STATUS_SUCCESS on Success
 */
QDF_STATUS umac_module_param_handler(void *context, const char *str_param, const char *str_value)
{
    QDF_STATUS status;
    uint16_t param=0;

    while ( param < UMAC_PARAM_MAX ) {
        if (qdf_str_eq(umac_module_param[param], str_param)) {
            switch(param) {
                case PARAM_AHBSKIP:
                    status = qdf_int32_parse(str_value, &ahbskip);
                    break;
                case PARAM_G_UNICAST_DEAUTH_ON_STOP:
                    status = qdf_int32_parse(str_value,
                                             &g_unicast_deauth_on_stop);
                    break;
                case PARAM_ATF_MODE:
#if QCA_AIRTIME_FAIRNESS
                    status = qdf_uint32_parse(str_value, &atf_mode);
#endif
                    break;
                case PARAM_NETLINK_ACFG:
#if UMAC_SUPPORT_ACFG
                    status = qdf_uint32_parse(str_value, &netlink_acfg);
#endif
                    break;
                case PARAM_ENABLE_MESH_PEER_CAP_UPDATE:
                    status = qdf_uint32_parse(str_value,
                                              &enable_mesh_peer_cap_update);
                    break;
                case PARAM_NETLINK_ATH_SSID_EVENT:
#if QCA_SUPPORT_SSID_STEERING
                            status = qdf_uint32_parse(str_value,
                            &netlink_ath_ssid_event);
#endif
                    break;
                case PARAM_ENABLE_PKTLOG_SUPPORT:
                    status = qdf_uint32_parse(str_value,
                                              &enable_pktlog_support);
                    break;
                case PARAM_NETLINK_BAND_STEERING_EVENT:
#if QCA_SUPPORT_SON
                    status = qdf_uint32_parse(str_value,
                                              &netlink_band_steering_event);
#endif
                    break;
                case PARAM_FW_REC_DBG_MASK:
                    status = qdf_int32_parse(str_value, &fw_rec_dbg_mask);
                    break;
                case PARAM_QLD_MAX_LIST:
                    status = qdf_uint32_parse(str_value, &qld_max_list);
                    break;
                case PARAM_G_CSA_MAX_RX_WAIT_TIME:
                    status = qdf_int32_parse(str_value,
                                             &g_csa_max_rx_wait_time);
                    break;
                case PARAM_SA_API_MODE:
#if UNIFIED_SMARTANTENNA
                    status = qdf_uint32_parse(str_value, &sa_api_mode);
#endif
                    break;
                case PARAM_NETLINK_SON_ALD:
#if QCA_SUPPORT_SON
                    status = qdf_uint32_parse(str_value, &netlink_son_ald);
#endif
                    break;
                default:
                    break;
            }
            if ( QDF_IS_STATUS_SUCCESS(status)) {
                return QDF_STATUS_SUCCESS;
            }
        }
        param++;
    }
    return QDF_STATUS_SUCCESS;
}

/**
 * initialize_umac_module_param_from_ini() - Update umac module params
 *
 *
 * Read the file which has wifi module params, parse and update
 * umac module params.
 *
 * Return: void
 */
void initialize_umac_module_param_from_ini(void)
{
    QDF_STATUS status;
    char *path = QDF_WIFI_MODULE_PARAMS_FILE;

    status = qdf_ini_parse(path, NULL, umac_module_param_handler, NULL);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("ERROR: Failed to parse *.ini file @ %s; status:%d",
                path, status);
        return;
    }
}
#endif

unsigned long ath_ioctl_debug = 0;      /* for run-time debugging of ioctl commands */
/* For storing LCI and LCR info for this AP */
u_int32_t ap_lcr[RTT_LOC_CIVIC_REPORT_LEN]={0};
int num_ap_lcr=0;
u_int32_t ap_lci[RTT_LOC_CIVIC_INFO_LEN]={0};
int num_ap_lci=0;
qdf_export_symbol(num_ap_lci);
qdf_export_symbol(ap_lci);
qdf_export_symbol(num_ap_lcr);
qdf_export_symbol(ap_lcr);
#if ATH_DEBUG
unsigned long ath_rtscts_enable = 0;
#endif
	A_UINT32 dscp_tid_map[WMI_HOST_DSCP_MAP_MAX] = {
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 1, 1,
			2, 2, 2, 2, 2, 2, 2, 2,
			3, 3, 3, 3, 3, 3, 3, 3,
			4, 4, 4, 4, 4, 4, 4, 4,
			5, 5, 5, 5, 5, 5, 5, 5,
			6, 6, 6, 6, 6, 6, 6, 6,
			7, 7, 7, 7, 7, 7, 7, 7,
	};
qdf_export_symbol(dscp_tid_map);

struct g_wifi_info g_winfo;
qdf_export_symbol(g_winfo);

qdf_spinlock_t asf_print_lock;

#ifdef OSIF_DEBUG
#define IEEE80211_ADD_MEDIA(_media, _s, _o) \
    do { \
        qdf_print("adding media word 0x%x ",IFM_MAKEWORD(IFM_IEEE80211, (_s), (_o), 0)); \
        ifmedia_add(_media, IFM_MAKEWORD(IFM_IEEE80211, (_s), (_o), 0), 0, NULL); \
    } while(0);
#else
#define IEEE80211_ADD_MEDIA(_media, _s, _o) \
        ifmedia_add(_media, IFM_MAKEWORD(IFM_IEEE80211, (_s), (_o), 0), 0, NULL);
#endif
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef QCA_PARTNER_PLATFORM
#define PARTNER_ETHTOOL_SUPPORTED 0
#endif

struct global_ic_list ic_list = {
    .radio_list_lock = __SPIN_LOCK_UNLOCKED(ic_list.radio_list_lock),
};
qdf_export_symbol(ic_list);

extern QDF_STATUS wlan_mlme_vdev_notify_down_complete(
                                     struct wlan_objmgr_vdev *vdev);
void osif_mlme_notify_handler(wlan_if_t vap,
        enum wlan_serialization_cmd_type cmd_type);
#define OLE_HEADER_PADDING 2

#include <ieee80211.h>
#include <ieee80211_mlme_dfs_dispatcher.h>

#define IS_SNAP(_llc) ((_llc)->llc_dsap == LLC_SNAP_LSAP && \
                       (_llc)->llc_ssap == LLC_SNAP_LSAP && \
                       (_llc)->llc_control == LLC_UI)
void
transcap_nwifi_to_8023(qdf_nbuf_t msdu)
{
    struct ieee80211_frame_addr4 *wh;
    uint32_t hdrsize;
    struct llc *llchdr;
    struct ether_header *eth_hdr;
    uint16_t ether_type = 0;
    uint8_t a1[QDF_MAC_ADDR_SIZE], a2[QDF_MAC_ADDR_SIZE], a3[QDF_MAC_ADDR_SIZE], a4[QDF_MAC_ADDR_SIZE];
    uint8_t fc1;

    wh = (struct ieee80211_frame_addr4 *)qdf_nbuf_data(msdu);
    IEEE80211_ADDR_COPY(a1, wh->i_addr1);
    IEEE80211_ADDR_COPY(a2, wh->i_addr2);
    IEEE80211_ADDR_COPY(a3, wh->i_addr3);
    fc1 = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;

    /* Native Wifi header  give only 80211 non-Qos packets */
    if (fc1 == IEEE80211_FC1_DIR_DSTODS )
    {
        hdrsize = sizeof(struct ieee80211_frame_addr4);

        /* In case of WDS frames, , padding is enabled by default
         * in Native Wifi mode, to make ipheader 4 byte aligned
         */
        hdrsize = hdrsize + OLE_HEADER_PADDING;
    }
    else
    {
        hdrsize = sizeof(struct ieee80211_frame);
    }

    llchdr = (struct llc *)(((uint8_t *)qdf_nbuf_data(msdu)) + hdrsize);

    if (IS_SNAP(llchdr) &&
            (llchdr->llc_un.type_snap.org_code[0] == RFC1042_SNAP_ORGCODE_0) &&
            (llchdr->llc_un.type_snap.org_code[1] == RFC1042_SNAP_ORGCODE_1) &&
            (llchdr->llc_un.type_snap.org_code[2] == RFC1042_SNAP_ORGCODE_2)) {
        ether_type = llchdr->llc_un.type_snap.ether_type;
        /*
         * Now move the data pointer to the beginning of the mac header :
         * new-header = old-hdr + (wifhdrsize + llchdrsize - ethhdrsize)
         */
        qdf_nbuf_pull_head(
                msdu, (hdrsize + sizeof(struct llc) - sizeof(struct ether_header)));

    } else if ((llchdr->llc_control == (LLC_PDU_TYPE | LLC_PDU_CMD_XID)) &&
            (llchdr->llc_fid == LLC_XID_FMTID)) {
        /* For Xid pkts keep the llc data */
        qdf_nbuf_pull_head(msdu,(hdrsize - sizeof(struct ether_header)));
        ether_type = htons(qdf_nbuf_len(msdu) - sizeof(struct ether_header));

    } else {
        /* For non snap pkts keep the llc data */
        qdf_nbuf_pull_head(msdu,(hdrsize - sizeof(struct ether_header)));
        ether_type = htons(qdf_nbuf_len(msdu) - sizeof(struct ether_header));
    }

    eth_hdr = (struct ether_header *)(qdf_nbuf_data(msdu));

    switch (fc1) {
    case IEEE80211_FC1_DIR_NODS:
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a1);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a2);
        break;
    case IEEE80211_FC1_DIR_TODS:
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a3);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a2);
        break;
    case IEEE80211_FC1_DIR_FROMDS:
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a1);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a3);
        break;
    case IEEE80211_FC1_DIR_DSTODS:
        IEEE80211_ADDR_COPY(a4, wh->i_addr4);
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a3);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a4);
        break;
    }
    eth_hdr->ether_type = ether_type;
}
qdf_export_symbol(transcap_nwifi_to_8023);

#ifdef LIMIT_MTU_SIZE

static struct net_device __fake_net_device = {
    .hard_header_len    = ETH_HLEN
};

static struct rtable __fake_rtable = {
    .u = {
        .dst = {
            .__refcnt       = ATOMIC_INIT(1),
            .dev            = &__fake_net_device,
            .path           = &__fake_rtable.u.dst,
            .metrics        = {[RTAX_MTU - 1] = 1500},
        }
    },
    .rt_flags   = 0,
};
#else
#define __fake_rtable 0
#endif


#if UMAC_VOW_DEBUG
void update_vow_dbg_counters(osif_dev  *osifp, qdf_nbuf_t msdu, unsigned long *vow_counter, int rx, int peer);
#endif
void osif_check_pending_mlsta_ap_vaps(wlan_dev_t comhandle, struct wlan_objmgr_vdev *vdev);
void osif_bring_down_mlsta_ap_vaps(wlan_dev_t comhandle, struct wlan_objmgr_vdev *vdev);

extern void ieee80211_ioctl_vattach(struct net_device *dev);
extern void ieee80211_ioctl_vdetach(struct net_device *dev);
#if ATH_SUPPORT_WRAP
osif_dev *wlan_get_osdev(struct wlan_objmgr_vdev *vdev);
struct ieee80211vap *wlan_get_vap(struct wlan_objmgr_vdev *vdev);
#endif

#ifdef QCA_PARTNER_PLATFORM
extern void wlan_pltfrm_attach(struct net_device *dev);
extern void wlan_pltfrm_detach(struct net_device *dev);
extern void osif_pltfrm_receive (os_if_t osif, wbuf_t wbuf,
                        u_int16_t type, u_int16_t subtype,
                        ieee80211_recv_status *rs);
extern void osif_pltfrm_record_macinfor(unsigned char unit, unsigned char* mac);
extern bool osif_pltfrm_deliver_data(os_if_t osif, wbuf_t wbuf);
extern void osif_pltfrm_vap_init( struct net_device *dev );
extern void osif_pltfrm_vap_stop( osif_dev  *osifp );
#endif
#if QCA_NSS_PLATFORM
extern void osif_send_to_nss(os_if_t osif, struct sk_buff *skb, void *nss_redir_ctx,
                             struct net_device *dev);
#elif QCA_PPE_VP
extern void osif_send_to_ppe_vp(os_if_t osif, struct sk_buff *skb, int32_t ppe_vp_num,
                             struct net_device *dev);
extern void osif_pltfrm_alloc_ext_ppe_vp(osif_peer_dev *osifp, struct net_device *peer_ndev);
extern void osif_pltfrm_attach_ext_ppe_vp(osif_peer_dev *osifp);
#endif
#if defined (QCA_PARTNER_PLATFORM) || (QCA_NSS_PLATFORM) || (QCA_PPE_VP)
extern void osif_pltfrm_create_vap(osif_dev *osifp);
extern void osif_pltfrm_delete_vap(osif_dev *osifp);
#ifdef QCA_SUPPORT_WDS_EXTENDED
extern void osif_pltfrm_create_ext_vap(osif_peer_dev *osifp);
extern void osif_pltfrm_delete_ext_vap(osif_peer_dev *osifp);
#endif
#endif
#ifdef WLAN_SUPPORT_PPEDS
#ifdef QCA_PPE_VP
extern void osif_pltfrm_setup_ds_vap_fw_recovery(osif_dev *osifp, bool recover);
#endif
#endif

static void osif_bkscan_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg);
QDF_STATUS osif_cmd_schedule_req(struct wlan_objmgr_vdev *vdev, enum osif_cmd_type cmd_type);

/* The code below is used to register a parent file in sysfs */
static ssize_t ath_parent_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osifp = ath_netdev_priv(net);
    struct net_device *parent = osifp->os_comdev;

    return snprintf(buf, IFNAMSIZ,"%s\n", parent->name);
}

/*Handler for sysfs entry cfg80211 xmlfile
  returns xml file that is used for vendor commands */
static ssize_t wifi_cfg80211_xmlfile_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    strlcpy(buf, "qcacommands_vap.xml", sizeof("qcacommands_vap.xml"));
    return strlen(buf);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_htcaps_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osdev = ath_netdev_priv(net);
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    u_int16_t capinfo = ic->ic_htcap;
    uint32_t iv_ldpc;

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    if ( !capinfo )
        return strlen(buf);

    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_LDPC, &iv_ldpc);
    /* configurable through IEEE80211_SUPPORT_LDPC param */
    if ((capinfo & IEEE80211_HTCAP_C_ADVCODING) && iv_ldpc)
        strlcat(buf, "[LDPC]", strlen(buf) + sizeof("[LDPC]") + 1);
    /* IEEE80211_HTCAP_C_CHWIDTH40 is configured  ucisn UCI scipts
       based on configuration */
    if ((capinfo & IEEE80211_HTCAP_C_SM_MASK) ==
            IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC)
        strlcat(buf, "[SMPS-STATIC]", strlen(buf) + sizeof("[SMPS-STATIC]") + 1);
    if ((capinfo & IEEE80211_HTCAP_C_SM_MASK) ==
            IEEE80211_HTCAP_C_SMPOWERSAVE_DYNAMIC)
        strlcat(buf, "[SMPS-DYNAMIC]", strlen(buf) + sizeof("[SMPS-DYNAMIC]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_GREENFIELD)
        strlcat(buf, "[GF]", strlen(buf) + sizeof("[GF]") + 1);
    /* IEEE80211_HTCAP_C_SHORTGI20/40 configured using UCI scripts
       based on configuration
      */
    /* configurable through IEEE80211_SUPPORT_TX_STBC param */
    if ((capinfo & IEEE80211_HTCAP_C_TXSTBC) && vap->iv_tx_stbc)
        strlcat(buf, "[TX-STBC]", strlen(buf) + sizeof("[TX-STBC]") + 1);
    if ((capinfo & IEEE80211_HTCAP_C_RXSTBC)) {
        if (vap->iv_rx_stbc == 1)
            strlcat(buf, "[RX-STBC-1]", strlen(buf) + sizeof("[RX-STBC-1]") + 1);
        if (vap->iv_rx_stbc == 2)
            strlcat(buf, "[RX-STBC-12]", strlen(buf) + sizeof("[RX-STBC-12]") + 1);
        if (vap->iv_rx_stbc == 3)
            strlcat(buf, "[RX-STBC-123]", strlen(buf) + sizeof("[RX-STBC-123]") + 1);
        if (vap->iv_rx_stbc == 4)
            strlcat(buf, "[RX-STBC-1234]", strlen(buf) + sizeof("[RX-STBC-1234]") + 1);
    }
    if (capinfo & IEEE80211_HTCAP_C_DELAYEDBLKACK)
        strlcat(buf, "[DELAYED-BA]", strlen(buf) + sizeof("[DELAYED-BA]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_MAXAMSDUSIZE)
        strlcat(buf, "[MAX-AMSDU-7935]", strlen(buf) + sizeof("[MAX-AMSDU-7935]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_DSSSCCK40)
        strlcat(buf, "[DSSS_CCK-40]", strlen(buf) + sizeof("[DSSS_CCK-40]") + 1);
    if ((capinfo & IEEE80211_HTCAP_C_INTOLERANT40) && vap->iv_ht40_intolerant)
        strlcat(buf, "[40-INTOLERANT]", strlen(buf) + sizeof("[40-INTOLERANT]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_LSIGTXOPPROT)
        strlcat(buf, "[LSIG-TXOP-PROT]", strlen(buf) + sizeof("[LSIG-TXOP-PROT]") + 1);

    return strlen(buf);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_vhtcaps_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define AMPDU_STRING_LEN 22
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osdev = ath_netdev_priv(net);
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    u_int32_t vhtcap = ic->ic_vhtcap;
    u_int8_t ampdu_len_exponenet = 0,bf_antenna_max = 0;
    char ampdu_exponent_string[32];
    uint32_t txbf;

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    /*
        [VHT160] [VHT160-80PLUS80] - add by UCI configuration
    */
    if (vhtcap & IEEE80211_VHTCAP_MAX_MPDU_LEN_7935)
        strlcat(buf, "[MAX-MPDU-7991]", strlen(buf) + sizeof("[MAX-MPDU-7991]") + 1 );
    if (vhtcap & IEEE80211_VHTCAP_MAX_MPDU_LEN_11454)
        strlcat(buf, "[MAX-MPDU-11454]", strlen(buf) + sizeof("[MAX-MPDU-11454]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SUP_CHAN_WIDTH_160)
        strlcat(buf, "[VHT160]", strlen(buf) + sizeof("[VHT160]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SUP_CHAN_WIDTH_80_160)
        strlcat(buf, "[VHT160-80PLUS80]", strlen(buf) + sizeof("[VHT160-80PLUS80]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_RX_LDPC)
        strlcat(buf, "[RXLDPC]", strlen(buf) + sizeof("[RXLDPC]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SHORTGI_80)
        strlcat(buf, "[SHORT-GI-80]", strlen(buf) + sizeof("[SHORT-GI-80]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SHORTGI_160)
        strlcat(buf, "[SHORT-GI-160]", strlen(buf) + sizeof("[SHORT-GI-160]") + 1);
    if ((vhtcap & IEEE80211_VHTCAP_TX_STBC) && vap->iv_tx_stbc)
        strlcat(buf, "[TX-STBC-2BY1]", strlen(buf) + sizeof("[TX-STBC-2BY1]") + 1);
    /* configurable through IEEE80211_SUPPORT_RX_STBC param */
    if ((vhtcap & IEEE80211_HTCAP_C_RXSTBC)) {
        if (vap->iv_rx_stbc == 1)
            strlcat(buf, "[RX-STBC1]", strlen(buf) + sizeof("[RX-STBC1]") + 1);
        if (vap->iv_rx_stbc == 2)
            strlcat(buf, "[RX-STBC12]", strlen(buf) + sizeof("[RX-STBC12]") + 1);
        if (vap->iv_rx_stbc == 3)
            strlcat(buf, "[RX-STBC123]", strlen(buf) + sizeof("[RX-STBC123]") + 1);
    }

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_SUBFER, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_SU_BFORMER) && txbf) {
        strlcat(buf, "[SU-BEAMFORMER]", strlen(buf) + sizeof("[SU-BEAMFORMER]") + 1);
        ucfg_wlan_vdev_mgr_get_param(vdev,
                WLAN_MLME_CFG_SOUNDING_DIM, &txbf);
        /* IEEE80211_VHTCAP_SOUND_DIM */
        if ( txbf == 3)
            strlcat(buf, "[SOUNDING-DIMENSION-4]", strlen(buf) + sizeof("[SOUNDING-DIMENSION-4]") + 1);
        if ( txbf == 2)
            strlcat(buf, "[SOUNDING-DIMENSION-3]", strlen(buf) + sizeof("[SOUNDING-DIMENSION-3]") + 1);
        if ( txbf == 1)
            strlcat(buf, "[SOUNDING-DIMENSION-2]", strlen(buf) + sizeof("[SOUNDING-DIMENSION-2]") + 1);
    }

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_SUBFEE, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_SU_BFORMEE) && txbf) {
        strlcat(buf, "[SU-BEAMFORMEE]", strlen(buf) + sizeof("[SU-BEAMFORMEE]") + 1);
        /* IEEE80211_VHTCAP_BF_MAX_ANT */
        bf_antenna_max = (vhtcap & IEEE80211_VHTCAP_BF_MAX_ANT) >> IEEE80211_VHTCAP_BF_MAX_ANT_S;
        if ( bf_antenna_max == 3)
            strlcat(buf, "[BF-ANTENNA-4]", strlen(buf) + sizeof("[BF-ANTENNA-4]") + 1);
        if ( bf_antenna_max == 2)
            strlcat(buf, "[BF-ANTENNA-3]", strlen(buf) + sizeof("[BF-ANTENNA-3]") + 1);
        if ( bf_antenna_max == 1)
            strlcat(buf, "[BF-ANTENNA-2]", strlen(buf) + sizeof("[BF-ANTENNA-2]") + 1);
    }

    /* IEEE80211_VHTCAP_MAX_AMPDU_LEN_FACTOR  */
    ampdu_len_exponenet = (vhtcap & IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP) >> IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S;
    snprintf(ampdu_exponent_string, AMPDU_STRING_LEN,
            "[MAX-A-MPDU-LEN-EXP%d]", ampdu_len_exponenet);
    strlcat(buf, ampdu_exponent_string, strlen(buf) + AMPDU_STRING_LEN + 1);

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_MUBFER, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_MU_BFORMER) && txbf)
        strlcat(buf, "[MU-BEAMFORMER]", strlen(buf) + sizeof("[MU-BEAMFORMER]") + 1);

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_MUBFEE, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_MU_BFORMEE) && txbf) {
        /* Not yet supported by hostapd */
    }
    if (vhtcap & IEEE80211_VHTCAP_TXOP_PS)
        strlcat(buf, "[VHT-TXOP-PS]", strlen(buf) + sizeof("[VHT-TXOP-PS]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_PLUS_HTC_VHT)
        strlcat(buf, "[HTC-VHT]", strlen(buf) + sizeof("[HTC-VHT]") + 1);
    /* IEEE80211_VHTCAP_LINK_ADAPT */

    /* RX/TX antenna pattern */
    if (vhtcap & IEEE80211_VHTCAP_RX_ANTENNA_PATTERN)
        strlcat(buf, "[RX-ANTENNA-PATTERN]", strlen(buf) + sizeof("[RX-ANTENNA-PATTERN]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_TX_ANTENNA_PATTERN)
        strlcat(buf, "[TX-ANTENNA-PATTERN]", strlen(buf) + sizeof("[TX-ANTENNA-PATTERN]") + 1);

    return strlen(buf);
}

static DEVICE_ATTR(cfg80211_htcaps, S_IRUGO, wifi_cfg80211_htcaps_show, NULL);
static DEVICE_ATTR(cfg80211_vhtcaps, S_IRUGO, wifi_cfg80211_vhtcaps_show, NULL);
static DEVICE_ATTR(cfg80211_xmlfile, S_IRUGO, wifi_cfg80211_xmlfile_show, NULL);
static DEVICE_ATTR(parent, S_IRUGO, ath_parent_show, NULL);
static struct attribute *ath_device_attrs[] = {
    &dev_attr_parent.attr,
    &dev_attr_cfg80211_xmlfile.attr,
    &dev_attr_cfg80211_htcaps.attr,
    &dev_attr_cfg80211_vhtcaps.attr,
    NULL
};

static struct attribute_group ath_attr_group = {
    .attrs  = ath_device_attrs,
};

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
/* Handler for sysfs entry cfg80211 partnerinfo */
static ssize_t wifi_cfg80211_partnerinfo_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
#define MAC_ADDR_CHAR 18
#define LINK_ID_CHAR 3
#define FREQ_CHAR 5
    struct net_device *net = to_net_dev(dev);
    struct osif_mldev *osifp_mld;
    osif_dev *osifp;
    wlan_if_t vap;
    struct ieee80211_ath_channel *channel;
    struct cfg80211_mlo_info *cfg80211_mlo_info;
    int i;
    uint8_t mlo_link_id, link_mac_addr[QDF_MAC_ADDR_SIZE], link_channel;
    uint16_t link_freq;
    char link_id[LINK_ID_CHAR], mac_addr[MAC_ADDR_CHAR], chan[LINK_ID_CHAR];
    char freq[FREQ_CHAR], band[LINK_ID_CHAR], link_band[LINK_ID_CHAR];

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    osifp_mld = osif_mld_get_mldev(net);

    if (!osifp_mld) {
        qdf_err("null osifp_mld\n");
        return -EINVAL;
    }

    cfg80211_mlo_info = &osifp_mld->mlo_info;

    if (!cfg80211_mlo_info) {
        qdf_err("cfg80211_mlo_info null\n");
        return -EINVAL;
    }

    for (i = 0; i < cfg80211_mlo_info->num_mlo_links; i++) {
        mlo_link_id = cfg80211_mlo_info->mlo_link_ids[i];
        IEEE80211_ADDR_COPY(link_mac_addr, cfg80211_mlo_info->mlo_mac_addrs[i].addr);
        osifp = osifp_mld->link_vifs[mlo_link_id];
        if (!osifp) {
            qdf_err("null osifp\n");
            continue;
        }
        vap = osifp->os_if;
        if (!vap) {
            qdf_err("null vap\n");
            return -EINVAL;
        }
        channel = vap->iv_bsschan;
        if (!channel) {
            qdf_err("null channel\n");
            return -EINVAL;
        }
        link_channel = channel->ic_ieee;
        link_freq = channel->ic_freq;

        if (IEEE80211_IS_CHAN_2GHZ(channel)) {
            strlcpy(link_band, "2G", sizeof("2G"));
        } else if (IEEE80211_IS_CHAN_5GHZ(channel)) {
            strlcpy(link_band, "5G", sizeof("5G"));
        } else {
            strlcpy(link_band, "6G", sizeof("6G"));
        }

        strlcat(buf, "Link ID :", strlen(buf) + sizeof("Link ID :") + 1);
        snprintf(link_id, sizeof(link_id), "%d", mlo_link_id);
        strlcat(buf, link_id, strlen(buf) + sizeof(link_id));

        strlcat(buf, " - MAC : ", strlen(buf) + sizeof(" - MAC : ") + 1);

        snprintf(mac_addr, sizeof(mac_addr), "%02x:%02x:%02x:%02x:%02x:%02x", link_mac_addr[0], link_mac_addr[1], link_mac_addr[2], link_mac_addr[3], link_mac_addr[4], link_mac_addr[5]);
        strlcat(buf, mac_addr, strlen(buf) + sizeof(mac_addr));

        strlcat(buf, " - Channel :", strlen(buf) + sizeof(" - Channel :") + 1);
        snprintf(chan, sizeof(chan), "%d", link_channel);
        strlcat(buf, chan, strlen(buf) + sizeof(chan));

        strlcat(buf, " - Frequency :", strlen(buf) + sizeof(" - Frequency :") + 1);
        snprintf(freq, sizeof(freq), "%d", link_freq);
        strlcat(buf, freq, strlen(buf) + sizeof(freq));

        strlcat(buf, " - Band :", strlen(buf) + sizeof(" - Band :") + 1);
        snprintf(band, sizeof(band), "%s", link_band);
        strlcat(buf, band, strlen(buf) + sizeof(band));

        strlcat(buf, "\n", strlen(buf) + sizeof("\n") + 1);
    }

    return strlen(buf);
}

static DEVICE_ATTR(cfg80211_partnerinfo, S_IRUGO, wifi_cfg80211_partnerinfo_show, NULL);
static struct attribute *mld_device_attrs[] = {
    &dev_attr_cfg80211_xmlfile.attr,
    &dev_attr_cfg80211_partnerinfo.attr,
    NULL
};

static struct attribute_group mld_attr_group = {
    .attrs = mld_device_attrs,
};

static ssize_t wifi_cfg80211_mld_linkmac_show (struct device *dev,
        struct device_attribute *attr, char *buf, uint8_t link_id)
{
#define MAC_ADDR_CHAR 18
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osifp = get_vap_device_context(net->ieee80211_ptr->wiphy, net, link_id);
    wlan_if_t vap = osifp->os_if;
    uint8_t link_mac_addr[QDF_MAC_ADDR_SIZE];
    char mac_addr[MAC_ADDR_CHAR];

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    IEEE80211_ADDR_COPY(link_mac_addr, vap->iv_myaddr);
    snprintf(mac_addr, sizeof(mac_addr), "%02x:%02x:%02x:%02x:%02x:%02x", link_mac_addr[0], link_mac_addr[1], link_mac_addr[2], link_mac_addr[3], link_mac_addr[4], link_mac_addr[5]);
    strlcat(buf, mac_addr, strlen(buf) + sizeof(mac_addr));

    strlcat(buf, "\n", strlen(buf) + sizeof("\n") + 1);

    return strlen(buf);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_mld_htcaps_show(struct device *dev,
        struct device_attribute *attr, char *buf, uint8_t link_id)
{
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osdev = get_vap_device_context(net->ieee80211_ptr->wiphy, net, link_id);
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    u_int16_t capinfo = ic->ic_htcap;
    uint32_t iv_ldpc;

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    if ( !capinfo )
        return strlen(buf);

    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_LDPC, &iv_ldpc);
    /* configurable through IEEE80211_SUPPORT_LDPC param */
    if ((capinfo & IEEE80211_HTCAP_C_ADVCODING) && iv_ldpc)
        strlcat(buf, "[LDPC]", strlen(buf) + sizeof("[LDPC]") + 1);
    /* IEEE80211_HTCAP_C_CHWIDTH40 is configured  ucisn UCI scipts
       based on configuration */
    if ((capinfo & IEEE80211_HTCAP_C_SM_MASK) ==
            IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC)
        strlcat(buf, "[SMPS-STATIC]", strlen(buf) + sizeof("[SMPS-STATIC]") + 1);
    if ((capinfo & IEEE80211_HTCAP_C_SM_MASK) ==
            IEEE80211_HTCAP_C_SMPOWERSAVE_DYNAMIC)
        strlcat(buf, "[SMPS-DYNAMIC]", strlen(buf) + sizeof("[SMPS-DYNAMIC]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_GREENFIELD)
        strlcat(buf, "[GF]", strlen(buf) + sizeof("[GF]") + 1);
    /* IEEE80211_HTCAP_C_SHORTGI20/40 configured using UCI scripts
       based on configuration
      */
    /* configurable through IEEE80211_SUPPORT_TX_STBC param */
    if ((capinfo & IEEE80211_HTCAP_C_TXSTBC) && vap->iv_tx_stbc)
        strlcat(buf, "[TX-STBC]", strlen(buf) + sizeof("[TX-STBC]") + 1);
    if ((capinfo & IEEE80211_HTCAP_C_RXSTBC)) {
        if (vap->iv_rx_stbc == 1)
            strlcat(buf, "[RX-STBC-1]", strlen(buf) + sizeof("[RX-STBC-1]") + 1);
        if (vap->iv_rx_stbc == 2)
            strlcat(buf, "[RX-STBC-12]", strlen(buf) + sizeof("[RX-STBC-12]") + 1);
        if (vap->iv_rx_stbc == 3)
            strlcat(buf, "[RX-STBC-123]", strlen(buf) + sizeof("[RX-STBC-123]") + 1);
        if (vap->iv_rx_stbc == 4)
            strlcat(buf, "[RX-STBC-1234]", strlen(buf) + sizeof("[RX-STBC-1234]") + 1);
    }
    if (capinfo & IEEE80211_HTCAP_C_DELAYEDBLKACK)
        strlcat(buf, "[DELAYED-BA]", strlen(buf) + sizeof("[DELAYED-BA]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_MAXAMSDUSIZE)
        strlcat(buf, "[MAX-AMSDU-7935]", strlen(buf) + sizeof("[MAX-AMSDU-7935]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_DSSSCCK40)
        strlcat(buf, "[DSSS_CCK-40]", strlen(buf) + sizeof("[DSSS_CCK-40]") + 1);
    if ((capinfo & IEEE80211_HTCAP_C_INTOLERANT40) && vap->iv_ht40_intolerant)
        strlcat(buf, "[40-INTOLERANT]", strlen(buf) + sizeof("[40-INTOLERANT]") + 1);
    if (capinfo & IEEE80211_HTCAP_C_LSIGTXOPPROT)
        strlcat(buf, "[LSIG-TXOP-PROT]", strlen(buf) + sizeof("[LSIG-TXOP-PROT]") + 1);

    return strlen(buf);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_mld_vhtcaps_show(struct device *dev,
        struct device_attribute *attr, char *buf, uint8_t link_id)
{
#define AMPDU_STRING_LEN 22
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osdev = get_vap_device_context(net->ieee80211_ptr->wiphy, net, link_id);
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    u_int32_t vhtcap = ic->ic_vhtcap;
    u_int8_t ampdu_len_exponenet = 0,bf_antenna_max = 0;
    char ampdu_exponent_string[32];
    uint32_t txbf;

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    /*
        [VHT160] [VHT160-80PLUS80] - add by UCI configuration
    */
    if (vhtcap & IEEE80211_VHTCAP_MAX_MPDU_LEN_7935)
        strlcat(buf, "[MAX-MPDU-7991]", strlen(buf) + sizeof("[MAX-MPDU-7991]") + 1 );
    if (vhtcap & IEEE80211_VHTCAP_MAX_MPDU_LEN_11454)
        strlcat(buf, "[MAX-MPDU-11454]", strlen(buf) + sizeof("[MAX-MPDU-11454]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SUP_CHAN_WIDTH_160)
        strlcat(buf, "[VHT160]", strlen(buf) + sizeof("[VHT160]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SUP_CHAN_WIDTH_80_160)
        strlcat(buf, "[VHT160-80PLUS80]", strlen(buf) + sizeof("[VHT160-80PLUS80]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_RX_LDPC)
        strlcat(buf, "[RXLDPC]", strlen(buf) + sizeof("[RXLDPC]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SHORTGI_80)
        strlcat(buf, "[SHORT-GI-80]", strlen(buf) + sizeof("[SHORT-GI-80]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_SHORTGI_160)
        strlcat(buf, "[SHORT-GI-160]", strlen(buf) + sizeof("[SHORT-GI-160]") + 1);
    if ((vhtcap & IEEE80211_VHTCAP_TX_STBC) && vap->iv_tx_stbc)
        strlcat(buf, "[TX-STBC-2BY1]", strlen(buf) + sizeof("[TX-STBC-2BY1]") + 1);
    /* configurable through IEEE80211_SUPPORT_RX_STBC param */
    if ((vhtcap & IEEE80211_HTCAP_C_RXSTBC)) {
        if (vap->iv_rx_stbc == 1)
            strlcat(buf, "[RX-STBC1]", strlen(buf) + sizeof("[RX-STBC1]") + 1);
        if (vap->iv_rx_stbc == 2)
            strlcat(buf, "[RX-STBC12]", strlen(buf) + sizeof("[RX-STBC12]") + 1);
        if (vap->iv_rx_stbc == 3)
            strlcat(buf, "[RX-STBC123]", strlen(buf) + sizeof("[RX-STBC123]") + 1);
    }

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_SUBFER, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_SU_BFORMER) && txbf) {
        strlcat(buf, "[SU-BEAMFORMER]", strlen(buf) + sizeof("[SU-BEAMFORMER]") + 1);
        ucfg_wlan_vdev_mgr_get_param(vdev,
                WLAN_MLME_CFG_SOUNDING_DIM, &txbf);
        /* IEEE80211_VHTCAP_SOUND_DIM */
        if ( txbf == 3)
            strlcat(buf, "[SOUNDING-DIMENSION-4]", strlen(buf) + sizeof("[SOUNDING-DIMENSION-4]") + 1);
        if ( txbf == 2)
            strlcat(buf, "[SOUNDING-DIMENSION-3]", strlen(buf) + sizeof("[SOUNDING-DIMENSION-3]") + 1);
        if ( txbf == 1)
            strlcat(buf, "[SOUNDING-DIMENSION-2]", strlen(buf) + sizeof("[SOUNDING-DIMENSION-2]") + 1);
    }

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_SUBFEE, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_SU_BFORMEE) && txbf) {
        strlcat(buf, "[SU-BEAMFORMEE]", strlen(buf) + sizeof("[SU-BEAMFORMEE]") + 1);
        /* IEEE80211_VHTCAP_BF_MAX_ANT */
        bf_antenna_max = (vhtcap & IEEE80211_VHTCAP_BF_MAX_ANT) >> IEEE80211_VHTCAP_BF_MAX_ANT_S;
        if ( bf_antenna_max == 3)
            strlcat(buf, "[BF-ANTENNA-4]", strlen(buf) + sizeof("[BF-ANTENNA-4]") + 1);
        if ( bf_antenna_max == 2)
            strlcat(buf, "[BF-ANTENNA-3]", strlen(buf) + sizeof("[BF-ANTENNA-3]") + 1);
        if ( bf_antenna_max == 1)
            strlcat(buf, "[BF-ANTENNA-2]", strlen(buf) + sizeof("[BF-ANTENNA-2]") + 1);
    }

    /* IEEE80211_VHTCAP_MAX_AMPDU_LEN_FACTOR  */
    ampdu_len_exponenet = (vhtcap & IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP) >> IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S;
    snprintf(ampdu_exponent_string, AMPDU_STRING_LEN,
            "[MAX-A-MPDU-LEN-EXP%d]", ampdu_len_exponenet);
    strlcat(buf, ampdu_exponent_string, strlen(buf) + AMPDU_STRING_LEN + 1);

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_MUBFER, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_MU_BFORMER) && txbf)
        strlcat(buf, "[MU-BEAMFORMER]", strlen(buf) + sizeof("[MU-BEAMFORMER]") + 1);

    ucfg_wlan_vdev_mgr_get_param(vdev,
            WLAN_MLME_CFG_MUBFEE, &txbf);
    if ((vhtcap & IEEE80211_VHTCAP_MU_BFORMEE) && txbf) {
        /* Not yet supported by hostapd */
    }
    if (vhtcap & IEEE80211_VHTCAP_TXOP_PS)
        strlcat(buf, "[VHT-TXOP-PS]", strlen(buf) + sizeof("[VHT-TXOP-PS]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_PLUS_HTC_VHT)
        strlcat(buf, "[HTC-VHT]", strlen(buf) + sizeof("[HTC-VHT]") + 1);
    /* IEEE80211_VHTCAP_LINK_ADAPT */

    /* RX/TX antenna pattern */
    if (vhtcap & IEEE80211_VHTCAP_RX_ANTENNA_PATTERN)
        strlcat(buf, "[RX-ANTENNA-PATTERN]", strlen(buf) + sizeof("[RX-ANTENNA-PATTERN]") + 1);
    if (vhtcap & IEEE80211_VHTCAP_TX_ANTENNA_PATTERN)
        strlcat(buf, "[TX-ANTENNA-PATTERN]", strlen(buf) + sizeof("[TX-ANTENNA-PATTERN]") + 1);

    return strlen(buf);
}

static ssize_t wifi_cfg80211_parent_show (struct device *dev,
        struct device_attribute *attr, char *buf, uint8_t link_id)
{
#define PARENT_IFNAME_CHAR 16
    struct net_device *net = to_net_dev(dev);
    osif_dev  *osifp = get_vap_device_context(net->ieee80211_ptr->wiphy, net, link_id);
    char parent[PARENT_IFNAME_CHAR];

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    snprintf(parent, sizeof(parent), "%s", osifp->os_comdev->name);
    strlcat(buf, parent, strlen(buf) + sizeof(parent));

    return strlen(buf);
}

static ssize_t wifi_cfg80211_link_id_show (struct device *dev,
        struct device_attribute *attr, char *buf, uint8_t link_id)
{
#define LINK_ID_CHAR 3
    char linkid[LINK_ID_CHAR];

    if ( !buf )
        return 0 ;

    buf[0]='\0';

    snprintf(linkid, sizeof(linkid), "%d", link_id);
    strlcat(buf, linkid, strlen(buf) + sizeof(linkid));

    return strlen(buf);
}

static ssize_t wifi_cfg80211_linkmac_show_link0(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_linkmac_show(dev, attr, buf, 0);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_htcaps_show_link0(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_htcaps_show(dev, attr, buf, 0);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_vhtcaps_show_link0(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_vhtcaps_show(dev, attr, buf, 0);
}

static ssize_t wifi_cfg80211_parent_show_link0(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_parent_show(dev, attr, buf, 0);
}

static ssize_t wifi_cfg80211_link_id_show_link0(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_link_id_show(dev, attr, buf, 0);
}

static DEVICE_ATTR(address_link0, S_IRUGO, wifi_cfg80211_linkmac_show_link0, NULL);
static DEVICE_ATTR(cfg80211_htcaps_link0, S_IRUGO, wifi_cfg80211_htcaps_show_link0, NULL);
static DEVICE_ATTR(cfg80211_vhtcaps_link0, S_IRUGO, wifi_cfg80211_vhtcaps_show_link0, NULL);
static DEVICE_ATTR(parent_link0, S_IRUGO, wifi_cfg80211_parent_show_link0, NULL);
static DEVICE_ATTR(link_id_link0, S_IRUGO, wifi_cfg80211_link_id_show_link0, NULL);

static struct attribute *ath_link0_device_attrs[] = {
    &dev_attr_address_link0.attr,
    &dev_attr_cfg80211_htcaps_link0.attr,
    &dev_attr_cfg80211_vhtcaps_link0.attr,
    &dev_attr_parent_link0.attr,
    &dev_attr_link_id_link0.attr,
    NULL
};

static struct attribute_group ath_link0_attr_group = {
    .attrs  = ath_link0_device_attrs,
    .name = "link0",
};

static ssize_t wifi_cfg80211_linkmac_show_link1(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_linkmac_show(dev, attr, buf, 1);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_htcaps_show_link1(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_htcaps_show(dev, attr, buf, 1);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_vhtcaps_show_link1(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_vhtcaps_show(dev, attr, buf, 1);
}

static ssize_t wifi_cfg80211_parent_show_link1(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_parent_show(dev, attr, buf, 1);
}

static ssize_t wifi_cfg80211_link_id_show_link1(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_link_id_show(dev, attr, buf, 1);
}

static DEVICE_ATTR(address_link1, S_IRUGO, wifi_cfg80211_linkmac_show_link1, NULL);
static DEVICE_ATTR(cfg80211_htcaps_link1, S_IRUGO, wifi_cfg80211_htcaps_show_link1, NULL);
static DEVICE_ATTR(cfg80211_vhtcaps_link1, S_IRUGO, wifi_cfg80211_vhtcaps_show_link1, NULL);
static DEVICE_ATTR(parent_link1, S_IRUGO, wifi_cfg80211_parent_show_link1, NULL);
static DEVICE_ATTR(link_id_link1, S_IRUGO, wifi_cfg80211_link_id_show_link1, NULL);

static struct attribute *ath_link1_device_attrs[] = {
    &dev_attr_address_link1.attr,
    &dev_attr_cfg80211_htcaps_link1.attr,
    &dev_attr_cfg80211_vhtcaps_link1.attr,
    &dev_attr_parent_link1.attr,
    &dev_attr_link_id_link1.attr,
    NULL
};

static struct attribute_group ath_link1_attr_group = {
    .attrs  = ath_link1_device_attrs,
    .name = "link1",
};

static ssize_t wifi_cfg80211_linkmac_show_link2(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_linkmac_show(dev, attr, buf, 2);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_htcaps_show_link2(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_htcaps_show(dev, attr, buf, 2);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_vhtcaps_show_link2(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_vhtcaps_show(dev, attr, buf, 2);
}

static ssize_t wifi_cfg80211_parent_show_link2(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_parent_show(dev, attr, buf, 2);
}

static ssize_t wifi_cfg80211_link_id_show_link2(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_link_id_show(dev, attr, buf, 2);
}

static DEVICE_ATTR(address_link2, S_IRUGO, wifi_cfg80211_linkmac_show_link2, NULL);
static DEVICE_ATTR(cfg80211_htcaps_link2, S_IRUGO, wifi_cfg80211_htcaps_show_link2, NULL);
static DEVICE_ATTR(cfg80211_vhtcaps_link2, S_IRUGO, wifi_cfg80211_vhtcaps_show_link2, NULL);
static DEVICE_ATTR(parent_link2, S_IRUGO, wifi_cfg80211_parent_show_link2, NULL);
static DEVICE_ATTR(link_id_link2, S_IRUGO, wifi_cfg80211_link_id_show_link2, NULL);

static struct attribute *ath_link2_device_attrs[] = {
    &dev_attr_address_link2.attr,
    &dev_attr_cfg80211_htcaps_link2.attr,
    &dev_attr_cfg80211_vhtcaps_link2.attr,
    &dev_attr_parent_link2.attr,
    &dev_attr_link_id_link2.attr,
    NULL
};

static struct attribute_group ath_link2_attr_group = {
    .attrs  = ath_link2_device_attrs,
    .name = "link2",
};

static ssize_t wifi_cfg80211_linkmac_show_link3(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_linkmac_show(dev, attr, buf, 3);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_htcaps_show_link3(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_htcaps_show(dev, attr, buf, 3);
}

/* Handler for sysfs entry cfg80211 htcaps
   returns htcaps supported that is usable for hostapd */
static ssize_t wifi_cfg80211_vhtcaps_show_link3(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_mld_vhtcaps_show(dev, attr, buf, 3);
}

static ssize_t wifi_cfg80211_parent_show_link3(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_parent_show(dev, attr, buf, 3);
}

static ssize_t wifi_cfg80211_link_id_show_link3(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return wifi_cfg80211_link_id_show(dev, attr, buf, 3);
}

static DEVICE_ATTR(address_link3, S_IRUGO, wifi_cfg80211_linkmac_show_link3, NULL);
static DEVICE_ATTR(cfg80211_htcaps_link3, S_IRUGO, wifi_cfg80211_htcaps_show_link3, NULL);
static DEVICE_ATTR(cfg80211_vhtcaps_link3, S_IRUGO, wifi_cfg80211_vhtcaps_show_link3, NULL);
static DEVICE_ATTR(parent_link3, S_IRUGO, wifi_cfg80211_parent_show_link3, NULL);
static DEVICE_ATTR(link_id_link3, S_IRUGO, wifi_cfg80211_link_id_show_link3, NULL);

static struct attribute *ath_link3_device_attrs[] = {
    &dev_attr_address_link3.attr,
    &dev_attr_cfg80211_htcaps_link3.attr,
    &dev_attr_cfg80211_vhtcaps_link3.attr,
    &dev_attr_parent_link3.attr,
    &dev_attr_link_id_link3.attr,
    NULL
};

static struct attribute_group ath_link3_attr_group = {
    .attrs  = ath_link3_device_attrs,
    .name = "link3",
};
#endif

static void osif_acs_bk_scantimer_fn( void * arg );

#ifdef ATHEROS_LINUX_PERIODIC_SCAN
static void osif_periodic_scan_start(os_if_t osif)
{
    osif_dev  *osdev = (osif_dev *) osif;
    wlan_if_t vap;

    vap = osdev->os_if;

    if (osdev->os_periodic_scan_period){
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s: Periodic Scan Timer Start. %d msec \n",
                            __func__, osdev->os_periodic_scan_period);
        OS_SET_TIMER(&osdev->os_periodic_scan_timer, osdev->os_periodic_scan_period);
    }

    return;
}

static void osif_periodic_scan_stop(os_if_t osif)
{
    osif_dev  *osdev = (osif_dev *) osif;
    wlan_if_t vap;

    vap = osdev->os_if;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s: Periodic Scan Timer Stop. \n", __func__);
    OS_CANCEL_TIMER(&osdev->os_periodic_scan_timer);

    return;
}

static OS_TIMER_FUNC(periodic_scan_timer_handler)
{
    osif_dev *osifp;
    wlan_if_t vap;
    struct net_device *dev;
    struct scan_start_request *scan_params = NULL;
    uint32_t chan[3];
    struct ieee80211com *ic;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    struct wlan_objmgr_vdev *vdev = NULL;

    OS_GET_TIMER_ARG(osifp, osif_dev *);
    vap = osifp->os_if;
    ic  = vap->iv_ic;
    dev = get_cfg80211_notification_ndev(osifp);

    if (!(dev->flags & IFF_UP)) {
        return;
    }

    vdev = osifp->ctrl_vdev;
    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_OSIF_SCAN_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        scan_info("unable to get reference");
        return;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s: Periodic Scan Timer Go. \n", __func__);

    if ((wlan_vap_get_opmode(vap) == IEEE80211_M_STA) &&
         (vap->iv_bss && vap->iv_bss->ni_chwidth == IEEE80211_CWM_WIDTH40) &&
         !(vap->iv_ic->ic_flags & IEEE80211_F_COEXT_DISABLE))  { /*do not trigger scan, if it disable coext is set */
        if (wlan_cm_is_vdev_connected(vdev) &&
            !wlan_scan_in_progress(vap)){

            scan_params = (struct scan_start_request*) qdf_mem_malloc(sizeof(*scan_params));
            if (!scan_params) {
                wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
                return;
            }

            /* Fill scan parameter */
            status = wlan_update_scan_params(vap,scan_params,IEEE80211_M_STA,true,false,true,true,0,NULL,1);
            if (status) {
                wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
                qdf_mem_free(scan_params);
                scan_err("scan param init failed with status: %d", status);
                return;
            }

            scan_params->scan_req.dwell_time_active = 120;
            scan_params->scan_req.idle_time = 610;
            scan_params->scan_req.repeat_probe_time = 0;
            scan_params->scan_req.max_scan_time = OSIF_PERIODICSCAN_MIN_PERIOD;
            scan_params->scan_req.scan_f_2ghz = true;
            scan_params->scan_req.scan_f_5ghz = true;
            scan_params->scan_req.scan_f_passive = false;
            scan_params->scan_req.scan_f_bcast_probe = true;

            if (osifp->os_scan_band != OSIF_SCAN_BAND_ALL) {
                scan_params->scan_req.scan_f_2ghz = false;
                scan_params->scan_req.scan_f_5ghz = false;
                if (osifp->os_scan_band == OSIF_SCAN_BAND_2G_ONLY)
                    scan_params->scan_req.scan_f_2ghz = true;
                else if (osifp->os_scan_band == OSIF_SCAN_BAND_5G_ONLY)
                    scan_params->scan_req.scan_f_5ghz = true;
                else {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
                            "%s: unknow scan band, scan all bands.\n", __func__);
                    scan_params->scan_req.scan_f_2ghz = true;
                    scan_params->scan_req.scan_f_5ghz = true;
                }
            }
#define CHAN_EXTENT_40MHz (6 * 5)
#define MAX_2_4_CHAN 15
            /* 11AX TODO: Recheck future 802.11ax drafts (>D1.0) on coex rules
             */
            if(IEEE80211_IS_CHAN_VALID(vap->iv_bsschan) &&
                    (IEEE80211_IS_CHAN_11N_HT40(vap->iv_bsschan) ||
                     IEEE80211_IS_CHAN_11AX_HE40(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
                     || IEEE80211_IS_CHAN_11BE_EHT40(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
                    )) {
                int max_chan_freq, min_chan_freq, nchans;
                qdf_freq_t supp_chans[MAX_2_4_CHAN];

                /* Get the list of supported channel */
                nchans = wlan_get_channel_list(vap->iv_ic, BIT(REG_BAND_2G),
                                               supp_chans, MAX_2_4_CHAN, REG_CURRENT_PWR_MODE);
                /* This is list is sorted already,
                 * Get Max and min supported chan */
                min_chan_freq = supp_chans[0];
                max_chan_freq = supp_chans[nchans > 0 ? (nchans-1) : 0];

                /* Get to the end of 40MHz chan span in case of HT40+ and
                 * then derive centre and start of extension channel for scan
                 * for e.g.,
                 * channel 1 uses 1 - 7 block.
                 * channel 1 + 6 = 7. 7 - 2 = 5 (centre) 7 - 4 = 3
                 * similarly in HT40-, it goes back to the start of the block and
                 * then derive centre and end of extn channel.
                 */
                if (IEEE80211_IS_FLAG_40PLUS(vap->iv_bsschan->ic_flags)) {
                    /* MIN of maximum channel num supported in the reg domain and end of block.
                     * to sure we are not scanning on channles not supported in reg domain */
                    chan[0] = MIN((vap->iv_bsschan->ic_freq + CHAN_EXTENT_40MHz),max_chan_freq);
                    chan[1] = chan[0] - 10;
                    chan[2] = chan[0] - 20;
                } else if (IEEE80211_IS_FLAG_40MINUS(vap->iv_bsschan->ic_flags)) {
                    /* MAX of minmum channel num supported in the reg domain and start of block.
                     * to be sure we are not passing negative channel num for scan */
                    chan[0] = MAX((vap->iv_bsschan->ic_freq - CHAN_EXTENT_40MHz),min_chan_freq);
                    chan[1] = chan[0] + 10;
                    chan[2] = chan[0] + 20;
                }
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
                                  "%s: Curr chan freq = %d scan Chan freq= %d %d %d\n", __func__,
                                  vap->iv_bsschan->ic_freq, chan[0], chan[1], chan[2]);

                status = ucfg_scan_init_chanlist_params(scan_params, 3, chan, NULL);
                if (status) {
                    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
                    qdf_mem_free(scan_params);
                    scan_err("init chan list failed with status: %d", status);
                    return;
                }
            }

            status = wlan_ucfg_scan_start(vap, scan_params, osifp->scan_requestor,
                    SCAN_PRIORITY_LOW, &(osifp->scan_id), 0, NULL);
            if (status) {
                scan_err("scan_start failed with status: %d", status);
            }
        }

        /* restart the timer */
        osif_periodic_scan_start((os_if_t)osifp);
    }

    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
    return;
}
#endif

#if ATH_SUPPORT_WAPI
static void
osif_wapi_rekeytimer_start (os_if_t osif)
{
    osif_dev  *osdev = (osif_dev *) osif;
//  IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "%s: WAPI Rekeying Timer Start. \n", __func__);
    OS_SET_TIMER(&osdev->os_wapi_rekey_timer, osdev->os_wapi_rekey_period);

}

static void
osif_wapi_rekeytimer_stop (os_if_t osif)
{
    osif_dev  *osdev = (osif_dev *) osif;
//  IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "%s: WAPI Rekeying Timer Stop. \n", __func__);
    OS_CANCEL_TIMER(&osdev->os_wapi_rekey_timer);

}
static OS_TIMER_FUNC(osif_wapi_rekey_timeout )
{
    osif_dev *osifp;
    wlan_if_t vap;
    struct net_device *dev;

    OS_GET_TIMER_ARG(osifp, osif_dev *);
    vap = osifp->os_if;
    dev = get_cfg80211_notification_ndev(osifp);

    if (!(dev->flags & IFF_UP)) {
        return;
    }

    if ((wlan_vdev_mlme_is_active(vap->vdev_obj) == QDF_STATUS_SUCCESS) &&
		ieee80211_vap_wapi_is_set(vap))
    {
        if(vap->iv_wapi_urekey_pkts)
        {
            wlan_iterate_station_list(vap,
				(ieee80211_sta_iter_func)wlan_wapi_unicast_rekey, (void*)vap);
        }
        if(vap->iv_wapi_mrekey_pkts)
        {
            wlan_wapi_multicast_rekey(vap,vap->iv_bss);
         }
    }

    osif_wapi_rekeytimer_start((os_if_t)osifp);
    return;
}

#endif  /* ATH_SUPPORT_WAPI */

#ifdef UMAC_SUPPORT_PROXY_ARP
int
do_proxy_arp(wlan_if_t vap, qdf_nbuf_t netbuf)
{
#if UMAC_SUPPORT_DGAF_DISABLE
    struct ether_header *eh = (struct ether_header *)netbuf->data;
#endif

    if (!vap)
        goto drop;

    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        if (qdf_unlikely(ieee80211_vap_proxyarp_is_set(vap))) {
            /* IEEE 802.11v Proxy ARP */
            if (wlan_proxy_arp(vap, netbuf))
                goto drop;
        }
#if UMAC_SUPPORT_DGAF_DISABLE
        if (qdf_unlikely(ieee80211_vap_dgaf_disable_is_set(vap))) {
            /* IEEE 802.11u DGAF Disable */
            if (IEEE80211_IS_MULTICAST(eh->ether_dhost)) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_PROXYARP, "HS20 DGAF: "
                    "discard multicast packet from %s\n",
                    ether_sprintf(eh->ether_shost));
                goto drop;
            }
        }
#endif /* UMAC_SUPPORT_DGAF_DISABLE */
    }
    return 0;

drop:
    return 1;
}
#endif /* UMAC_SUPPORT_PROXY_ARP */

#if UMAC_VOW_DEBUG
void update_vow_dbg_counters(osif_dev  *osifp, qdf_nbuf_t msdu, unsigned long *vow_counter, int rx, int peer)
{
    u_int8_t *data, *l3_hdr, *bp;
    u_int16_t ethertype;
    u_int32_t len;
    int offset;
#define ETHERNET_ADDR_LEN 6 /* bytes */
    struct vow_dbg_ethernet_hdr_t {
        u_int8_t dest_addr[ETHERNET_ADDR_LEN];
        u_int8_t src_addr[ETHERNET_ADDR_LEN];
        u_int8_t ethertype[2];
    };

#ifndef ETHERNET_HDR_LEN
#define ETHERNET_HDR_LEN (sizeof(struct vow_dbg_ethernet_hdr_t))
#endif

#ifndef ETHERTYPE_IPV4
#define ETHERTYPE_IPV4  0x0800 /* Internet Protocol, Version 4 (IPv4) */
#endif
#define IPV4_ADDR_LEN 4 /* bytes */
    struct vow_dbg_ipv4_hdr_t {
        u_int8_t ver_hdrlen;       /* version and hdr length */
        u_int8_t tos;              /* type of service */
        u_int8_t len[2];           /* total length */
        u_int8_t id[2];
        u_int8_t flags_fragoff[2]; /* flags and fragment offset field */
        u_int8_t ttl;              /* time to live */
        u_int8_t protocol;
        u_int8_t hdr_checksum[2];
        u_int8_t src_addr[IPV4_ADDR_LEN];
        u_int8_t dst_addr[IPV4_ADDR_LEN];
    };

#define IP_PROTOCOL_UDP         0x11 /* User Datagram Protocol */
#define IPV4_HDR_OFFSET_PROTOCOL (offsetof(struct vow_dbg_ipv4_hdr_t, protocol))
#define IPV4_HDR_OFFSET_TOS (offsetof(struct vow_dbg_ipv4_hdr_t, tos))
#define EXT_HDR_OFFSET 54     /* Extension header offset in network buffer */
#define UDP_PDU_RTP_EXT  0x90   /* ((2 << 6) | (1 << 4)) RTP Version 2 + X bit */
#define IP_VER4_N_NO_EXTRA_HEADERS 0x45
#define RTP_HDR_OFFSET  42
    data = qdf_nbuf_data(msdu);

    len = qdf_nbuf_len(msdu);
    if ( len < (EXT_HDR_OFFSET + 5) )
        return;

    offset = ETHERNET_ADDR_LEN * 2;
    l3_hdr = data + ETHERNET_HDR_LEN - rx;
    ethertype = (data[offset] << 8) | data[offset+1];
    if (rx || ethertype == ETHERTYPE_IPV4) {
        if(osifp->vow_dbg_en == 2) {
            int tid;
#if ATH_SUPPORT_DSCP_OVERRIDE
            int tos = l3_hdr[IPV4_HDR_OFFSET_TOS];
            wlan_if_t vap = osifp->os_if;

            tid = vap->iv_ic->ic_dscp_tid_map[vap->iv_dscp_map_id][tos>>IP_DSCP_SHIFT] & 0x7;
#endif
            if ((tid == 4) || (tid == 5)) {
                *vow_counter = *vow_counter + 1;
            }
        } else {
            offset = IPV4_HDR_OFFSET_PROTOCOL;
            if ((l3_hdr[offset] == IP_PROTOCOL_UDP) && (l3_hdr[0] == IP_VER4_N_NO_EXTRA_HEADERS)) {
                bp = data+EXT_HDR_OFFSET - rx ;

                if ( (data[RTP_HDR_OFFSET - rx] == UDP_PDU_RTP_EXT) &&
                        (bp[0] == 0x12) &&
                        (bp[1] == 0x34) &&
                        (bp[2] == 0x00) &&
                        (bp[3] == 0x08)) {

                    *vow_counter = *vow_counter + 1;
                    if( !rx && (peer >=0 && peer < MAX_VOW_CLIENTS_DBG_MONITOR) )
                    {
                        unsigned long pkt_count;
                        pkt_count = ntohl(*((unsigned long*)(bp + 4)));
                        if( pkt_count & 0x80000000 )
                            pkt_count &= 0x7FFFFFFF;
                        if( osifp->tx_prev_pkt_count[peer] )
                        {
                            if ( pkt_count > (osifp->tx_prev_pkt_count[peer] + 1) )
                                ; //qdf_print("***TX-Gap identified for peer %d(%s) prev=%u curr=%u Gap = %d", peer, (peer < 3 ? "valid":"Unknown"),
                            //      tx_prev_pkt_count[peer], pkt_count, pkt_count - tx_prev_pkt_count[peer]);
                        }
                        osifp->tx_prev_pkt_count[peer] = pkt_count;
                    }
                }
            }
        }
    }
}
qdf_export_symbol(update_vow_dbg_counters);
#endif

#if ATH_SUPPORT_WRAP
/*
 * osif_ol_wrap_rx_process()
 *  wrap rx process
 */
int
osif_ol_wrap_rx_process(osif_dev **osifp, struct net_device **dev, wlan_if_t vap, struct sk_buff *skb)
{
    int rv = 0;
    if (vap == NULL) {
        qdf_nbuf_free(skb);
        return 1;
    }
    rv = dp_wrap_rx_process(dev,vap->vdev_obj, skb);
    if (qdf_likely(rv == 0)) {
        *osifp = ath_netdev_priv(*dev);
        return rv;
    } else if (rv < 0) {
        qdf_nbuf_free(skb);
        return 1;
    } else {
        return rv;
    }
}
qdf_export_symbol(osif_ol_wrap_rx_process);
#endif

/* accept a single sk_buff at a time */
void
osif_deliver_data(os_if_t osif, struct sk_buff *skb)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
#if ATH_SUPPORT_WRAP
    int rv = 0;
#endif
    osif_dev  *osifp = (osif_dev *) osif;
#if ATH_RXBUF_RECYCLE
    struct net_device *comdev;
    struct ath_softc_net80211 *scn;
    struct ath_softc *sc;
#endif /* ATH_RXBUF_RECYCLE */
#if ATH_SUPPORT_WRAP || defined(WLAN_FEATURE_11BE_MLO)
    osif_dev  *osdev = (osif_dev *)osif;
    wlan_if_t vap = osdev->os_if;
#endif
    skb->dev = dev;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)) && MIPS_TP_ENHC
    prefetch(skb->data);
    prefetch(&skb->protocol);
    prefetch(&skb->__pkt_type_offset);
#endif
#if ATH_SUPPORT_WRAP
    if (dev && !IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        rv = dp_wrap_rx_process(&dev,vap->vdev_obj, skb);
        if (qdf_likely(rv == 0)) {
            osif = (os_if_t )ath_netdev_priv(dev);
        } else if (rv < 0) {
            qdf_nbuf_free(skb);
            goto done;
        } else {
            goto done;
        }
    }
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    if (ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        dev = ((osif_dev *)vap->iv_ifp)->rx_dev;
        skb->dev = dev;
    }
#endif
    if (qdf_unlikely(!dev)) {
            qdf_nbuf_free(skb);
            return;
    }
#ifdef QCA_PARTNER_PLATFORM
    if ( osif_pltfrm_deliver_data(osif, skb ) )
        goto done;
#endif

#if !(QCA_NSS_PLATFORM || QCA_PPE_VP)
    skb->protocol = eth_type_trans(skb, dev);
#endif
#if ATH_RXBUF_RECYCLE
    comdev = ((osif_dev *)osif)->os_comdev;
#ifdef UMAC_SUPPORT_CFG80211
    scn = get_radio_device_context(dev->ieee80211_ptr->wiphy, comdev);
#else
    scn = get_radio_device_context(NULL, comdev);
#endif
    sc = ATH_DEV_TO_SC(scn->sc_dev);
	/*
	 * Do not recycle the received mcast frame becasue it will be cloned twice
	 */
    if (sc->sc_osdev->rbr_ops.osdev_wbuf_collect && !(wbuf_is_cloned(skb))) {
        sc->sc_osdev->rbr_ops.osdev_wbuf_collect((void *)sc, (void *)skb);
    }
#endif /* ATH_RXBUF_RECYCLE */

#if ATH_SUPPORT_VLAN
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
    if ( osifp->vlanID != 0)
    {
        /* attach vlan tag */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        __vlan_hwaccel_put_tag(skb, osifp->vlanID);
#else
        __vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), osifp->vlanID);
#endif
    }
#else
    if ( osifp->vlanID != 0 && osifp->vlgrp != NULL)
    {
        /* attach vlan tag */
        vlan_hwaccel_rx(skb, osifp->vlgrp, osifp->vlanID);
    }
    else
#endif
#endif

   skb_orphan(skb);
#if !QCA_NSS_PLATFORM
#if !QCA_PPE_VP
    nbuf_debug_del_record(skb);
    netif_rx(skb);
#else /*QCA_PPE_VP*/
    osifp = (osif_dev *)osif;
#ifdef UMAC_SUPPORT_PROXY_ARP
    /* when proxy arp is enabled deliver the frames
     * directly to network stack using netif_rx().
     */
    if (qdf_unlikely((osifp->os_if->iv_opmode == IEEE80211_M_HOSTAP) &&
                     ieee80211_vap_proxyarp_is_set(osifp->os_if))) {
        skb->protocol = eth_type_trans(skb, dev);
        nbuf_debug_del_record(skb);
        netif_rx(skb);
    } else
#endif
    osif_send_to_ppe_vp(osif, skb, osifp->ppe_vp_num, dev);
#endif /*QCA_PPE_VP*/
#else /*QCA_NSS_PLATFORM*/
    osifp = (osif_dev *)osif;
    osif_send_to_nss(osif, skb, osifp->nss_redir_ctx, dev);
#endif /*QCA_NSS_PLATFORM*/
#if (ATH_SUPPORT_WRAP)
done:
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0) && !defined BUILD_X86
    dev->last_rx = jiffies;
#else
    return;
#endif
}
qdf_export_symbol(osif_deliver_data);

#define LEN_FIELD_SIZE     2

void transcap_dot3_to_eth2(struct sk_buff *skb)
{
    struct vlan_ethhdr *veh, veth_hdr;
    veh = (struct vlan_ethhdr *)skb->data;

    if (ntohs(veh->h_vlan_encapsulated_proto) >= IEEE8023_MAX_LEN)
        return;

    qdf_mem_copy(&veth_hdr, veh, sizeof(veth_hdr));
    qdf_nbuf_pull_head(skb, LEN_FIELD_SIZE);

    veh = (struct vlan_ethhdr *)skb->data;

    qdf_mem_copy(veh, &veth_hdr, (sizeof(veth_hdr) - LEN_FIELD_SIZE));
}

#if QCA_OL_VLAN_WAR
void
_ol_rx_vlan_war(struct sk_buff *skb, struct ol_ath_softc_net80211 *scn) {
    uint32_t target_type = scn->sc_ic.ic_get_tgt_type(&scn->sc_ic);
    struct ether_header *eh;

    /*
     * For chipstes (AR900B, QCA9984, QCA9888, QCA8074), enable VLAN
     * WAR only for 802.3 + VLAN packet. This WAR is not required QCA8074V2
     * onwards because of hardware support.
     */
    if ((target_type == TARGET_TYPE_QCA8074V2) ||
       (target_type == TARGET_TYPE_QCA9574) ||
       (target_type == TARGET_TYPE_QCN9000) ||
       (target_type == TARGET_TYPE_QCN9224) ||
       (target_type == TARGET_TYPE_QCN6122) ||
       (target_type == TARGET_TYPE_QCA5332) ||
       (target_type == TARGET_TYPE_QCN9160) ||
       (target_type == TARGET_TYPE_QCA5018) ||
       (target_type == TARGET_TYPE_QCA6018) ||
       (target_type == TARGET_TYPE_QCN6432))
        return;

    eh = (struct ether_header *)(skb->data);
    if (htons(eh->ether_type) != ETH_P_8021Q)
        return;

    /*
     * For VLAN, remove the extra two bytes of length field inserted by OLE.
     * This is standard behavior for OLE RX any format that looks weird
     * it keeps it in 802.3 format as that preserves all the information.
     * Frame Format : {dst_addr[6], src_addr[6], 802.1Q header[4], EtherType[2], Payload}
     * Example :
     * Before Removal : xx xx xx xx xx xx xx xx xx xx xx xx 81 00 00 02 00 4e 08 00 45 00 00...
     * After Removal  : xx xx xx xx xx xx xx xx xx xx xx xx 81 00 00 02 08 00 45 00 00...
     */

     transcap_dot3_to_eth2(skb);
}
qdf_export_symbol(_ol_rx_vlan_war);

static int encap_eth2_to_dot3(qdf_nbuf_t msdu);

inline  int
_ol_tx_vlan_war(struct sk_buff **skb, struct ol_ath_softc_net80211 *scn){
    struct ether_header *eh = (struct ether_header *)((*skb)->data);
    struct ethervlan_header *evh = (struct ethervlan_header *) eh;
    if ((htons(eh->ether_type) == ETH_P_8021Q)) {
	if (scn->sc_ic.ic_get_tgt_type(&scn->sc_ic) == TARGET_TYPE_AR9888) {
	    /* For Peregrine, enable VLAN WAR irrespective of eth2 + VLAN packet or 802.3 + VLAN packet*/
	    *skb = qdf_nbuf_unshare(*skb);
	    if (*skb == NULL) {
		return 1;
	    }
	    if (encap_eth2_to_dot3(*skb)){
		return 1;
	    }
	} else {
	    /* For other OL chipstes (Beeliner, Cascade, Besra, Dakota and hawkeye v1), enable VLAN WAR only for 802.3 + VLAN packet*/
	    /* This WAR is not needed in future chipsets (hawkeye v2), if this HW issue got fixed*/
	    if (htons(evh->ether_type) < IEEE8023_MAX_LEN) {
		*skb = qdf_nbuf_unshare(*skb);
		if (*skb == NULL) {
		    return 1;
		}
		if (encap_eth2_to_dot3(*skb)){
		    return 1;
		}
	    }
	}
    }
    return 0;
}
qdf_export_symbol(_ol_tx_vlan_war);
#endif /* QCA_OL_VLAN_WAR */

#ifdef QCA_OL_DMS_WAR
inline bool
ol_check_valid_dms(struct sk_buff **skb, wlan_if_t vap, uint8_t *peer_addr)
{
    struct ether_header *eh = (struct ether_header *)((*skb)->data);
    struct dms_meta_hdr *dms_mhdr;
    struct sk_buff *skb_orig = NULL;

    if (IEEE80211_IS_MULTICAST(eh->ether_dhost) &&
        !IEEE80211_IS_BROADCAST(eh->ether_dhost)) {
        skb_orig = qdf_nbuf_unshare(*skb);
        if (skb_orig == NULL) {
            return false;
        }
        *skb = skb_orig;
        dms_mhdr = (struct dms_meta_hdr *)((*skb)->head);
        /* In case of invalid DMS ID, treat it as a normal mcast packet */
        if (dms_mhdr->dms_id == 0) {
            return false;
        }
        IEEE80211_ADDR_COPY(peer_addr, dms_mhdr->unicast_addr);
        return true;
    }
    return false;
}
qdf_export_symbol(ol_check_valid_dms);

#endif /* QCA_OL_DMS_WAR */

#if ATH_PERF_PWR_OFFLOAD

#if QCA_NSS_PLATFORM
#if UMAC_VOW_DEBUG || UMAC_SUPPORT_VI_DBG
extern int transcap_nwifi_hdrsize(qdf_nbuf_t msdu);
#endif

#endif /*QCA_NSS_PLATFORM*/

#if ATH_SUPPORT_WAPI
bool
osif_wai_check(os_if_t osif, struct sk_buff *skb_list_head, struct sk_buff *skb_list_tail)
{
#define TYPEORLEN(_skb) (*(u_int16_t *)((_skb)->data + QDF_MAC_ADDR_SIZE * 2))
#define WAI_TYPE_CHECK(_skb)  do { if(TYPEORLEN(_skb) == ETHERTYPE_WAI) return true; } while(0)

    struct sk_buff *skb;

    if(!skb_list_head) {
        return false;
    }

    do {
        skb = skb_list_head;
        skb_list_head = skb_list_head->next;
        WAI_TYPE_CHECK(skb);
    } while((skb != skb_list_tail) && !skb_list_head);

    return false;
}
qdf_export_symbol(osif_wai_check);
#endif /* ATH_SUPPORT_WAPI */

#endif /* ATH_PERF_PWR_OFFLOAD */


static void
osif_deliver_l2uf(os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap;
    struct sk_buff *skb;
    struct l2_update_frame *l2uf;
    struct ether_header *eh;

    vap = osdev->os_if;

    skb = qdf_nbuf_alloc(NULL, sizeof(*l2uf), 0, 0, 0);
    if (!skb)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_INPUT, "%s\n",
                            "ieee80211_deliver_l2uf: no buf available");
        return;
    }
    skb_put(skb, sizeof(*l2uf));
    l2uf = (struct l2_update_frame *)(skb->data);
    eh = &l2uf->eh;
    /* dst: Broadcast address */
    IEEE80211_ADDR_COPY(eh->ether_dhost, dev->broadcast);
    skb->pkt_type = PACKET_BROADCAST;
    /* src: associated STA */
    IEEE80211_ADDR_COPY(eh->ether_shost, macaddr);
    eh->ether_type = htons(skb->len - sizeof(*eh));

    l2uf->dsap = 0;
    l2uf->ssap = 0x01;    /* CR Bit: Response */
    l2uf->control = IEEE80211_L2UPDATE_CONTROL;
    l2uf->xid[0] = IEEE80211_L2UPDATE_XID_0;
    l2uf->xid[1] = IEEE80211_L2UPDATE_XID_1;
    l2uf->xid[2] = IEEE80211_L2UPDATE_XID_2;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    skb->mac.raw = skb->data;
#else
    skb_reset_mac_header(skb);
#endif

    __osif_deliver_data(osif, skb);
}

#if DBDC_REPEATER_SUPPORT || UMAC_SUPPORT_SON
void
osif_hardstart_l2uf(os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap;
    struct sk_buff *skb;
    struct l2_update_frame *l2uf;
    struct ether_header *eh;

    vap = osdev->os_if;

    skb = qdf_nbuf_alloc(NULL, sizeof(*l2uf), 0, 0, 0);
    if (!skb)
    {
        qdf_print( "%s",
                            "ieee80211_hardstart_l2uf: no buf available");
        return;
    }
    skb_put(skb, sizeof(*l2uf));
    l2uf = (struct l2_update_frame *)(skb->data);
    eh = &l2uf->eh;
    /* dst: Broadcast address */
    IEEE80211_ADDR_COPY(eh->ether_dhost, dev->broadcast);
    /* src: associated STA */
    IEEE80211_ADDR_COPY(eh->ether_shost, macaddr);
    eh->ether_type = htons(skb->len - sizeof(*eh));

    l2uf->dsap = 0;
    l2uf->ssap = 0x01;    /* CR Bit: Response */
    l2uf->control = IEEE80211_L2UPDATE_CONTROL;
    l2uf->xid[0] = IEEE80211_L2UPDATE_XID_0;
    l2uf->xid[1] = IEEE80211_L2UPDATE_XID_1;
    l2uf->xid[2] = IEEE80211_L2UPDATE_XID_2;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    skb->mac.raw = skb->data;
#else
    skb_reset_mac_header(skb);
#endif
    if (vap->iv_evtable) {
        vap->iv_evtable->wlan_vap_xmit_queue(vap->iv_ifp, skb);
    }
}
#endif

struct ol_ath_softc_net80211 *osif_validate_and_get_scn_from_dev(struct net_device *dev)
{
    struct ieee80211com *tmp_ic;
    struct ieee80211vap *tmp_vap;
    int i;

    if (!dev)
        return NULL;

    GLOBAL_IC_LOCK_BH(&ic_list);
    for (i = 0; i < MAX_RADIO_CNT; i++) {
        tmp_ic = ic_list.global_ic[i];
        if (tmp_ic) {
            IEEE80211_COMM_LOCK_BH(tmp_ic);
            TAILQ_FOREACH(tmp_vap, &tmp_ic->ic_vaps, iv_next) {
                osif_dev *osifp = (osif_dev *)wlan_vap_get_registered_handle(tmp_vap);
                if (osifp == NULL) {
                    IEEE80211_COMM_UNLOCK_BH(tmp_ic);
                    GLOBAL_IC_UNLOCK_BH(&ic_list);
                    return NULL;
                }
                if (osifp->netdev == dev) {
                    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(tmp_ic);
                    IEEE80211_COMM_UNLOCK_BH(tmp_ic);
                    GLOBAL_IC_UNLOCK_BH(&ic_list);
                    return scn;
                }
            }
            IEEE80211_COMM_UNLOCK_BH(tmp_ic);
        }
    }
    GLOBAL_IC_UNLOCK_BH(&ic_list);
    return NULL;
}

qdf_export_symbol(osif_validate_and_get_scn_from_dev);

static void osif_get_active_vap_iter_func(void *arg, wlan_if_t vap)
{
    osif_dev *osifp;
    u_int16_t *active_vaps = (u_int16_t *) arg;
    struct net_device *dev;

    osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    if(osifp == NULL) {
        return;
    }

    if (IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        if (vap->iv_is_mlo_bridge_up)
            ++(*active_vaps);
        return;
    }

    dev = get_cfg80211_notification_ndev(osifp);

    if (dev->flags & IFF_UP) {
        ++(*active_vaps);
    }
}

static void osif_get_running_vap_iter_func(void *arg, wlan_if_t vap)
{
    u_int16_t *running_vaps = (u_int16_t *) arg;

    if(!vap || !vap->vdev_obj) {
        return;
    }
    if (wlan_vdev_chan_config_valid(vap->vdev_obj) == QDF_STATUS_SUCCESS) {
        ++(*running_vaps);
    }
}

u_int32_t osif_get_num_active_vaps( wlan_dev_t  comhandle)
{
    u_int16_t num_active_vaps=0;
    wlan_iterate_vap_list(comhandle,osif_get_active_vap_iter_func,(void *)&num_active_vaps);
    return num_active_vaps;
}
qdf_export_symbol(osif_get_num_active_vaps);

u_int16_t osif_get_num_running_vaps( wlan_dev_t  comhandle)
{
    u_int16_t num_running_vaps=0;
    wlan_iterate_vap_list(comhandle,osif_get_running_vap_iter_func,(void *)&num_running_vaps);
    return num_running_vaps;
}

uint32_t promisc_is_active (struct ieee80211com *ic)
{
    struct ieee80211vap *vap = NULL;
    osif_dev  *osifp = NULL;
    struct net_device *netdev = NULL;
    uint32_t promisc_active = false;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap && (vap->iv_opmode == IEEE80211_M_MONITOR)) {
            osifp = (osif_dev *)vap->iv_ifp;
            netdev = osifp->netdev;
            promisc_active = ((netdev->flags & IFF_UP) ? true : false);
            if (promisc_active) {
                break;
            }
        }
    }
    return promisc_active;
}

#ifdef QCA_BLOCK_EVENT_TO_USERSPACE
uint8_t
osif_vap_event_filter_status_to_app(struct ieee80211vap *vap,
                                    uint8_t event_type, uint8_t event)
{
    uint8_t to_app = 1; /* Default event allowed to user application. */

    switch (event_type) {
        case EVENT_ACFG:
#if UMAC_SUPPORT_ACFG
            if (event > 0 && event < WL_ACFG_MAX_EVENT) {
                if (!vap->iv_acfg_ev_fl[event].allow_to_app)
                    vap->iv_acfg_ev_fl[event].count += 1;

                return vap->iv_acfg_ev_fl[event].allow_to_app;
            } else
                qdf_err("ACFG event: %d is out of range", event);
#endif
            break;
        case EVENT_IWEVCUSTOM:
            if (event >= 0 && event < IEEE80211_EV_MAX_EVENT) {
                if (!vap->iv_iwevcustom_ev_fl[event].allow_to_app)
                    vap->iv_iwevcustom_ev_fl[event].count += 1;

                return vap->iv_iwevcustom_ev_fl[event].allow_to_app;
            } else
                qdf_err("IWEVCUSTOM event: %d is out of range", event);
            break;
        case EVENT_CFG:
            if (event > 0 && event < QCA_NL80211_VENDOR_SUBCMD_INDEX_LAST) {
                if (!vap->iv_cfg_ev_fl[event].allow_to_app)
                    vap->iv_cfg_ev_fl[event].count += 1;

                return vap->iv_cfg_ev_fl[event].allow_to_app;
            } else
                qdf_err("CFG event: %d is out of range", event);
            break;
        default:
            qdf_err("Invalid event type:%d", event_type);
            break;
    }
    return to_app;
}
qdf_export_symbol(osif_vap_event_filter_status_to_app);

uint8_t
osif_scn_event_filter_status_to_app(struct ieee80211com *ic,
                                    uint8_t event_type,  uint8_t event)
{
    uint8_t to_app = 1; /* Default event allowed to user application. */

    switch (event_type)
    {
        case EVENT_ACFG:
#if UMAC_SUPPORT_ACFG
            if (event > 0 && event < WL_ACFG_MAX_EVENT) {
                if (!ic->ic_acfg_ev_fl[event].allow_to_app)
                    ic->ic_acfg_ev_fl[event].count += 1;

                return ic->ic_acfg_ev_fl[event].allow_to_app;
            } else {
                qdf_err("ACFG event: %d is out of range", event);
            }
#endif
            break;
        case EVENT_IWEVCUSTOM:
            if (event > 0 && event < IEEE80211_EV_MAX_EVENT) {
                if (!ic->ic_iwevcustom_ev_fl[event].allow_to_app)
                    ic->ic_iwevcustom_ev_fl[event].count += 1;

                return ic->ic_iwevcustom_ev_fl[event].allow_to_app;
            } else {
                qdf_err("IWEVCUSTOM event: %d is out of range", event);
            }
            break;
        case EVENT_CFG:
            if (event > 0 && event < QCA_NL80211_VENDOR_SUBCMD_INDEX_LAST) {
                if (!ic->ic_cfg_ev_fl[event].allow_to_app)
                    ic->ic_cfg_ev_fl[event].count += 1;

                return ic->ic_cfg_ev_fl[event].allow_to_app;
            } else {
                qdf_err("CFG event: %d is out of range", event);
            }
            break;
        default:
            qdf_err("Invalid event type:%d ", event_type);
            break;
    }
    return to_app;
}
qdf_export_symbol(osif_scn_event_filter_status_to_app);

#else
uint8_t osif_vap_event_filter_status_to_app(struct ieee80211vap *vap,
                                            uint8_t event_type, uint8_t event)
{
    /* Default event allowed to user application. */
    return 1;
}
qdf_export_symbol(osif_vap_event_filter_status_to_app);

uint8_t osif_scn_event_filter_status_to_app(struct ieee80211com *ic,
                                            uint8_t event_type,  uint8_t event)
{
    /* Default event allowed to user application. */
    return 1;
}
qdf_export_symbol(osif_scn_event_filter_status_to_app);
#endif /* QCA_BLOCK_EVENT_TO_USERSPACE */

#if QCA_LTEU_SUPPORT
static void
osif_nl_scan_evhandler(struct wlan_objmgr_vdev *vdev, struct scan_event *event, void *arg)
{
    struct ieee80211_nl_handle *nl_handle = (struct ieee80211_nl_handle *)arg;
    osif_dev *osdev;
    struct net_device *dev;
    union iwreq_data wreq;
    struct event_data_scan scan;
    u_int8_t scan_id;
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);

    if (!vap) {
        qdf_err("NULL vap");
        return;
    }

    if (!nl_handle) {
        qdf_print("%s: Scan completion (type=%d reason=%d id=%d) but on non NL "
               "radio", __func__, event->type, event->reason, event->scan_id);
        return;
    }

    if (event->type != SCAN_EVENT_TYPE_COMPLETED ||
        event->reason == SCAN_REASON_CANCELLED) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_NL, "%s: Skip type=%d, "
                 "reason=%d\n", __func__, event->type, event->reason);
        return;
    }

    spin_lock_bh(&nl_handle->mu_lock);
    if (atomic_read(&nl_handle->scan_in_progress) == 0) {
        spin_unlock_bh(&nl_handle->mu_lock);
        qdf_print("%s: Scan completion id=%d (%d), but scan not active",
               __func__, event->scan_id, nl_handle->scanid);
        return;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_NL, "%s: Scan completion id=%d, expected=%d, "
             "type=%d, reason=%d, userid=%d\n", __func__, event->scan_id, nl_handle->scanid,
             event->type, event->reason, nl_handle->scan_id);
    scan_id = nl_handle->scan_id;
    atomic_set(&nl_handle->scan_in_progress, 0);
    spin_unlock_bh(&nl_handle->mu_lock);

    if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_SCAN)) {
        osdev = (osif_dev *)wlan_vap_get_registered_handle(vap);
        dev = get_cfg80211_notification_ndev(osdev);
        if (!dev) {
            qdf_err("NULL netdev");
            return;
        }

        memset(&wreq, 0, sizeof(wreq));
        wreq.data.flags = IEEE80211_EV_SCAN;
        wreq.data.length = sizeof(struct event_data_scan);
        wreq.data.length = sizeof(struct event_data_scan);
        scan.scan_req_id = scan_id;
        scan.scan_status = event->reason == SCAN_REASON_RUN_FAILED ?
            SCAN_FAIL : SCAN_SUCCESS;
        if (event->reason == SCAN_REASON_RUN_FAILED)
            qdf_info("%s: notify scan failed, reason=%d, scanid=%d",
                    __func__, event->reason, nl_handle->scan_id);
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (void *)&scan);
    }
}
#endif

/**
 *   ieee80211_update_assoc_ie: store assoc response ie
 *   @osdev : pointer to osdev.
 *   @wbuf  : assoc response frame.
 *
 *   returns NONE.
 */
void ieee80211_update_assoc_ie(wlan_if_t vap, wbuf_t wbuf)
{
    struct ieee80211_frame *wh;
    u_int8_t *frm, *efrm;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    frm = (u_int8_t *)&wh[1];
    efrm = wbuf_header(wbuf) + wbuf_get_pktlen(wbuf);

    if(efrm <= frm)
        return;

    if(vap->iv_sta_assoc_resp_ie)
        qdf_mem_free(vap->iv_sta_assoc_resp_ie);

    vap->iv_sta_assoc_resp_ie = (u_int8_t *)qdf_mem_malloc(efrm - frm);

    if(!vap->iv_sta_assoc_resp_ie)
        return;

    vap->iv_sta_assoc_resp_len = efrm-frm;
    qdf_mem_copy(vap->iv_sta_assoc_resp_ie, frm, vap->iv_sta_assoc_resp_len);
}

void osif_send_assoc_resp_ies(os_if_t osdev)
{
    union iwreq_data wreq;
    wlan_if_t vap = ((osif_dev *)osdev)->os_if;
    char *buf;
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osdev);

    if (!dev) {
        qdf_err("netdev is NULL for this vap type");
        return;
    }

    buf = (char *)qdf_mem_malloc(vap->iv_sta_assoc_resp_len);
    if (!buf) {
        qdf_info("%s: failed to allocate memory of size (%d)",__func__, vap->iv_sta_assoc_resp_len);
        return;
    }

    qdf_mem_zero(buf, vap->iv_sta_assoc_resp_len);
    wreq.data.flags = 0;
    wreq.data.length = vap->iv_sta_assoc_resp_len;
    qdf_mem_copy(buf, vap->iv_sta_assoc_resp_ie, vap->iv_sta_assoc_resp_len);

    WIRELESS_SEND_EVENT(dev, IWEVASSOCRESPIE, &wreq, buf);
    qdf_mem_free(buf);
    qdf_mem_free(vap->iv_sta_assoc_resp_ie);
    vap->iv_sta_assoc_resp_ie = NULL;
    vap->iv_sta_assoc_resp_len = 0;
}

void
osif_notify_scan_done(osif_dev *osdev, enum scan_completion_reason reason)
{
    union iwreq_data wreq;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap;
#if DBDC_REPEATER_SUPPORT
    struct ieee80211com *ic;
    static int count = 1;
#endif

    vap = osdev->os_if;
#if DBDC_REPEATER_SUPPORT
    ic = vap->iv_ic;
    if (ic->ic_global_list->delay_stavap_connection && !(ic->ic_radio_priority ==1) && count) {
        count--;
        return;
    }
#endif
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s\n", "notify scan done");

    /* dispatch wireless event indicating scan completed */
    wreq.data.length = 0;
    wreq.data.flags = 0;
#if QCA_LTEU_SUPPORT
    if (vap->iv_ic->ic_nl_handle) {
        if (reason == SCAN_REASON_RUN_FAILED) {
            wreq.data.flags = SCAN_REASON_RUN_FAILED;
            qdf_print("%s: notify scan failed , reason=%d",__func__,reason);
        } else {
            wreq.data.flags = SCAN_REASON_COMPLETED;
        }
    }
#else
    reason = reason; /* in case it warns for unused variable */
#endif
    WIRELESS_SEND_EVENT(dev, SIOCGIWSCAN, &wreq, NULL);
#if UMAC_SUPPORT_CFG80211
    if (osdev->scan_req) {
        if (reason == SCAN_REASON_COMPLETED) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
            struct cfg80211_scan_info info = {
                        .aborted = false,
            };
            cfg80211_scan_done(osdev->scan_req, &info); /* aborted false*/
#else
            cfg80211_scan_done(osdev->scan_req, false); /* aborted false*/
#endif
        } else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
            struct cfg80211_scan_info info = {
                        .aborted = true,
            };
	    cfg80211_scan_done(osdev->scan_req, &info); /* aborted true */
#else
            cfg80211_scan_done(osdev->scan_req, true); /* aborted true */
#endif
        }
        osdev->scan_req = NULL;
    }
#endif
}

/*
 * Function to store Last completed Scan Parameters
 */
static void osif_store_scan_params(struct ieee80211com *ic,struct scan_event *event)
{

    struct last_scan_params *scan_params = &ic->scan_params;
    struct scan_req_params *comp_scan_req = &event->scan_start_req->scan_req;

    /* Store Last scan finished parameters to display in proc */
    scan_params->dwell_time_passive = comp_scan_req->dwell_time_passive;
    scan_params->scan_f_passive = comp_scan_req->scan_f_passive;
    scan_params->scan_f_2ghz = comp_scan_req->scan_f_2ghz;
    scan_params->scan_f_5ghz = comp_scan_req->scan_f_5ghz;
    scan_params->scan_f_wide_band = comp_scan_req->scan_f_wide_band;
    scan_params->scan_f_continue_on_err = comp_scan_req->scan_f_continue_on_err;
    scan_params->scan_f_forced = comp_scan_req->scan_f_forced;
    scan_params->scan_f_bcast_probe = comp_scan_req->scan_f_bcast_probe;
    scan_params->dwell_time_passive = comp_scan_req->dwell_time_passive;
    scan_params->dwell_time_active = comp_scan_req->dwell_time_active;
    scan_params->scan_req_id = comp_scan_req->scan_req_id;
    scan_params->scan_id = comp_scan_req->scan_id;
    scan_params->max_scan_time = comp_scan_req->max_scan_time;
    scan_params->max_rest_time = comp_scan_req->max_rest_time;
    scan_params->min_rest_time = comp_scan_req->min_rest_time;
    scan_params->idle_time = comp_scan_req->idle_time;
    scan_params->chan_list = comp_scan_req->chan_list;
}

/*
 * Bringup vaps if cac was cancelled for scan
 */
void osif_restore_vaps(struct ieee80211com *ic, wlan_if_t vap)
{
    wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_STA_VDEVS_START);
    wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_PDEV_START);
}

/*
 * Function: osif_start_agile_cac
 * Start Agile CAC (PreCAC/RCAC).
 */
static void osif_start_agile_cac(struct wlan_objmgr_vdev *vdev)
{
        struct wlan_objmgr_pdev *pdev = NULL;
        struct wlan_objmgr_psoc *psoc = NULL;
        struct wlan_lmac_if_dfs_rx_ops *dfs_rx_ops = NULL;

        psoc = wlan_vdev_get_psoc(vdev);
        if (psoc) {
            dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
        }
        pdev = wlan_vdev_get_pdev(vdev);

#ifdef QCA_SUPPORT_AGILE_DFS
        /*
         * Agile RCAC/PreCAC will be stopped when scan is initiated.
         * Start it here once scan completes.
         */
        if (pdev && dfs_rx_ops && dfs_rx_ops->dfs_agile_sm_deliver_evt &&
            (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) == 0)) {
            if (wlan_vdev_chan_config_valid(vdev) == QDF_STATUS_E_FAILURE) {
                qdf_debug("%s: No active vaps on the radio, not posting agile start\n", __func__);
                wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
                return;
            }
             dfs_rx_ops->dfs_agile_sm_deliver_evt(pdev,
                                                  DFS_AGILE_SM_EV_AGILE_START);
             wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
        }
#endif
}

/*
 * Function: osif_stop_agile_cac
 * Stop Agile CAC (PreCAC/RCAC).
 */
static void osif_stop_agile_cac(struct wlan_objmgr_vdev *vdev)
{
        struct wlan_objmgr_pdev *pdev = NULL;
        struct wlan_objmgr_psoc *psoc = NULL;
        struct wlan_lmac_if_dfs_rx_ops *dfs_rx_ops = NULL;

        psoc = wlan_vdev_get_psoc(vdev);
        if (psoc) {
            dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
        }
        pdev = wlan_vdev_get_pdev(vdev);

#ifdef QCA_SUPPORT_AGILE_DFS
        /*
         * Stop Agile PreCAC/RCAC when scan is started/restarted.
         */
        if (pdev && dfs_rx_ops && dfs_rx_ops->dfs_agile_sm_deliver_evt &&
            (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) == 0)) {
             dfs_rx_ops->dfs_agile_sm_deliver_evt(pdev,
                                                  DFS_AGILE_SM_EV_AGILE_STOP);
             wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
        }
#endif
}

/*
 * scan handler used by the ioctl.
 */
static void osif_scan_evhandler(struct wlan_objmgr_vdev *vdev, struct scan_event *event, void *arg)
{
    osif_dev  *osifp = (osif_dev *) arg;
    wlan_if_t scan_originator_vap = osifp->os_if;
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);
    struct ieee80211com *ic;
    struct wlan_objmgr_pdev *pdev = NULL;
#ifdef CONFIG_AFC_SUPPORT
    enum reg_afc_dev_deploy_type reg_afc_deploy_type;
#endif
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    uint32_t dfs_region;
#endif

    pdev = wlan_vdev_get_pdev(vdev);

    if (!vap) {
        qdf_err("NULL vap");
        return;
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s scan_id %08X event %d reason %d requestor %X\n",
                      __func__, event->scan_id, event->type, event->reason, event->requester);

    ic = vap->iv_ic;
#if ATH_SUPPORT_MULTIPLE_SCANS
    /*
     * Ignore notifications received due to scans requested by other modules
     * and handle new event SCAN_EVENT_TYPE_DEQUEUED.
     */
    ASSERT(0);

    if (osifp->scan_id != event->scan_id) {
        return;
    }
#endif    /* ATH_SUPPORT_MULTIPLE_SCANS */

    /* For offchan scan, event handling is not needed */
    if(ucfg_scan_from_offchan_requestor(vdev, event)) {
        return;
    }

    if (event->type == SCAN_EVENT_TYPE_COMPLETED) {
        osif_start_agile_cac(vdev);
        if (event->reason != SCAN_REASON_CANCELLED) {
            if (event->scan_start_req->scan_req.scan_id != ic->scan_params.scan_id)
               osif_store_scan_params(ic,event);


            if (event->requester == osifp->scan_requestor) {

                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s SCAN DONE  reason %d \n",
                        __func__, event->reason);

                /* Reset Probe Response override flag */
                wlan_set_device_param(ic,
                        IEEE80211_DEVICE_OVERRIDE_SCAN_PROBERESPONSE_IE, 1);

#if QCA_LTEU_SUPPORT
                if (ic->ic_nl_handle) {
                    spin_lock_bh(&ic->ic_nl_handle->mu_lock);
                    if (atomic_read(&ic->ic_nl_handle->scan_in_progress) == 0) {
                        spin_unlock_bh(&ic->ic_nl_handle->mu_lock);
                        qdf_print("%s: Scan completion id=%d (%d), but scan not active",
                               __func__, event->scan_id, osifp->scan_id);
                        return;
                    }

                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_NL, "%s: Scan completion id=%d, "
                         "expected=%d, type=%d, reason=%d\n", __func__, event->scan_id,
                          osifp->scan_id, event->type, event->reason);

                    atomic_set(&ic->ic_nl_handle->scan_in_progress, 0);
                    spin_unlock_bh(&ic->ic_nl_handle->mu_lock);
                }
#endif

                osif_notify_scan_done(osifp, event->reason);
            }
        }
#if ATH_SUPPORT_DFS
        if(ic->ic_scan_over_cac && ic->ic_is_cac_cancelled_by_scan) {
            osif_restore_vaps(ic, vap);
            ic->ic_is_cac_cancelled_by_scan = 0;
        }
#endif
    }

    /* Check if a vap bringup was delayed because of scan in progress */
    if (event->type == SCAN_EVENT_TYPE_COMPLETED) {
        if (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) &&
                ic->schedule_bringup_vaps) {
            if ((osifp->os_opmode == IEEE80211_M_HOSTAP) ||
                    /* sta-vap is ready */
                ((osifp->os_opmode == IEEE80211_M_STA) && wlan_is_connected(vap) &&
                ucfg_mlo_is_mld_connected(vap->vdev_obj))) {
                osif_check_pending_mlsta_ap_vaps(wlan_vap_get_devhandle(vap), vap->vdev_obj);
                ic->schedule_bringup_vaps = false;
            }
        }
    }

    if (event->type == SCAN_EVENT_TYPE_STARTED ||
        event->type == SCAN_EVENT_TYPE_RESTARTED) {
        osif_stop_agile_cac(vdev);
    }

    if (!wlan_is_connected(vap) &&
            (vap->iv_opmode == IEEE80211_M_STA) &&
            (vap == scan_originator_vap) &&
            (ieee80211_vap_reset_ap_vaps_is_set(vap)) &&
            (wlan_vdev_mlme_get_state(vap->vdev_obj) != WLAN_VDEV_S_DFS_CAC_WAIT) &&
            (event->type == SCAN_EVENT_TYPE_STARTED ||
            event->type == SCAN_EVENT_TYPE_RESTARTED) &&
            !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)  &&
            !(wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_IN_PROGRESS)) {
        if (ucfg_mlo_is_mld_disconnected(vap->vdev_obj))
            osif_bring_down_mlsta_ap_vaps(wlan_vap_get_devhandle(vap), vap->vdev_obj);
    }
#if REPEATER_SAME_SSID
    if (wlan_rptr_global_is_s_ssid()) {
        wlan_rptr_ss_parse_scan_entries(vap->vdev_obj, event);
    }
#endif

#ifdef CONFIG_AFC_SUPPORT
    wlan_reg_get_afc_dev_deploy_type(ic->ic_pdev_obj, &reg_afc_deploy_type);

    if (reg_afc_deploy_type == AFC_DEPLOYMENT_OUTDOOR) {
        wlan_rptr_afc_parse_scan_entries(vap->vdev_obj, event);
    }
#endif

    /* Consider a case where NOL channel added to scan channel list sent to FW
     * in FCC domain.In the subsequent scan start request from host, FW would
     * block the scan start request due to DFS violation. Following are the
     * action taken:
     * 1. Rebuild ic and master channel list to consist of only non-DFS channel.
     * 2. Notify cfg layer that scan has failed so that subsequent scan is
     * is triggered from the supplicant.
     */
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    ieee80211_regdmn_get_dfs_region(pdev, (enum dfs_reg *)&dfs_region);
    if (event->type == SCAN_EVENT_TYPE_START_FAILED &&
        event->reason == SCAN_REASON_DFS_VIOLATION) {
        QDF_TRACE(QDF_MODULE_ID_DFS, QDF_TRACE_LEVEL_DEBUG,
                  "vap:%s Handling scan failure\n", vap->iv_netdev_name);
        ic->ic_is_dfs_scan_violation = true;
        dfs_mlme_handle_dfs_scan_violation(pdev);
        osif_notify_scan_done(osifp, event->reason);
    }
#endif

}

static int osif_acs_start_bss(wlan_if_t vap, wlan_chan_t channel)
{
    int error = 0;

    if (!vap) {
        qdf_err("vap is NULL");
        return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_mlme_is_non_mlo_11be_ap(vap)) {
        if (!wlan_mlme_is_non_mlo_11be_ap_operation_allowed(vap) &&
            !IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
            mlo_err("Non MLO 11BE AP operation is not allowed as per 802.11 standard");
            return QDF_STATUS_E_INVAL;
        }

        mlo_warn("Allowing 11be non-MLO operation as per INI configuration");
    }

    if (wlan_mlme_mlo_validate_ap_phymode(vap) != QDF_STATUS_SUCCESS) {
        if (vap->vdev_obj && vap->iv_ic && vap->iv_ic->ic_pdev_obj) {
            mlo_err("psoc id %u, pdev id %u, vdev id %u: Failed validation of AP PHY mode for MLO",
                    wlan_vdev_get_psoc_id(vap->vdev_obj),
                    wlan_objmgr_pdev_get_pdev_id(vap->iv_ic->ic_pdev_obj),
                    wlan_vdev_get_id(vap->vdev_obj));
        } else {
            mlo_err("Failed validation of AP PHY mode for MLO (Also unable to get psoc/pdev/vdev id due to NULL pointers in object linkage)");
        }

        return -EINVAL;
    }

    if (wlan_mlme_mlo_update_partner_info(vap) != QDF_STATUS_SUCCESS) {
        mlo_err("Failed wlan_mlme_mlo_update_partner_info");
        return -EINVAL;
    }
#endif /* WLAN_FEATURE_11BE_MLO */

    if ((error = wlan_mlme_start_bss(vap,0))!=0) {
        if (error == EAGAIN) {
            /* Radio resource is busy on scanning, try later */
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :mlme busy mostly scanning \n", __func__);
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                              "%s : failed start bss with error code %d\n",
                              __func__, error);
        }
        wlan_mlme_vdev_notify_down_complete(vap->vdev_obj);
        return error;
    } else {
        wlan_mlme_connection_up(vap);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                        "%s :vap up \n", __func__);
    }

    if (wlan_coext_enabled(vap))
    {
        wlan_determine_cw(vap, channel);
    }

    return error;
}

void osif_ht40_event_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
    int error;
    osif_dev *osifp;
    wlan_if_t vap;
    wlan_chan_t channel = (wlan_chan_t)arg;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_info("vap is NULL");
        return;
    }

    osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);

    if ( vap == NULL ) {
        int i;
        qdf_err("\n%s osif: (%pK) ", __func__, osifp);
        qdf_err("\nis_deleted: (%d),delete_in_prog: (%d) ",
                   osifp->is_deleted,
                    osifp->is_delete_in_progress);
        for ( i = 0; i < 25; i++ ) {
            qdf_err("\nosif [%d] 0x%pK",i,((char *)(osifp + i)));
        }
        return;
    }
    /* Do not process, if vap is in deletion or command is removed from queue */
    if ((osifp->is_delete_in_progress) ||
        (wlan_serialization_get_vdev_active_cmd_type(vap->vdev_obj) !=
                                          WLAN_SER_CMD_VDEV_START_BSS))
        goto done;

    if ((!channel) || (channel == IEEE80211_CHAN_ANYC))
        goto done;

    if ((error = osif_acs_start_bss(vap, channel))!=0) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :mlme busy mostly scanning \n", __func__);
    } else {
        wlan_restore_vap_params(vap);
    }

done:
    return;
}

#if UMAC_SUPPORT_CFG80211
static void cfg80211_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
    wlan_if_t vap;
    int error = 0;
    struct ieee80211vap *tmp_vap;
    bool bridge_vap_present = false;
    struct wlan_objmgr_psoc *psoc = NULL;
    wlan_chan_t channel = (wlan_chan_t)arg;
    enum ieee80211_phymode orig_mode;
    vap = wlan_vdev_get_mlme_ext_obj(vdev);

   if (!vap) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS, "%s : Received NULL vap\n", __func__);
        return;
    }

    orig_mode = vap->iv_des_mode;

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        struct ieee80211vap *stavap = vap->iv_ic->ic_sta_vap;
        if (stavap && (QDF_STATUS_SUCCESS ==
                wlan_vdev_chan_config_valid(stavap->vdev_obj))) {
            struct ieee80211_node *ni = stavap->iv_bss;
            struct chan_params chan_p = {0};
            chan_p.freq = ni->ni_ch_param.ni_freq;
            chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
            set_chan_params_puncture_bitmap(&chan_p,
                           ni->ni_ch_param.puncture_bitmap);
            *channel = ieee80211_find_dot11_channel(stavap->iv_ic,
                           &chan_p, ni->ni_ch_param.ni_bss_phymode);
            if (!IEEE80211_IS_CHAN_VALID(channel)) {
                qdf_err("vap-%u(%s) ACS failed. "
                        "STAVAP on radio has no valid channel to use\n",
                        vap->iv_unit, vap->iv_netdev_name);
                error = -1;
                goto done;
            }
            qdf_err("vap-%u(%s) ACS failed. "
                   "Using valid channel from STAVAP on radio\n",
                   vap->iv_unit, vap->iv_netdev_name);
        }
    }

#ifdef CONFIG_AFC_SUPPORT
    if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(channel))) {
        error = ieee80211_update_mode_and_power_type(vap,
                                                     channel,
                                                     ieee80211_chan2mode(channel));
        if (error) {
            qdf_err("Failed to change phymode and regulatory AP power type");
            goto done;
        }
    }
#endif /* CONFIG_AFC_SUPPORT */
    if (vap->iv_ic->ic_pdev_obj) {
        psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);
        if (!psoc)
            qdf_err("null psoc");
    } else {
        qdf_err("null pdev");
    }

    if (psoc && !cfg_get(psoc, CFG_OL_BRIDGE_VAP_DISABLE)) {
        TAILQ_FOREACH(tmp_vap, &vap->iv_ic->ic_vaps, iv_next) {
            if (IEEE80211_IS_MLO_BRIDGE_VAP(tmp_vap))
                bridge_vap_present = true;
        }
    }

    /* avoid setting channel to other vaps if bridge vap is present
     * to prevent different channel being configured for vaps.
     * start_ap takes care of setting same channel to all vaps.
     */
    if (!bridge_vap_present) {
        error = wlan_set_ieee80211_channel(vap, channel);
        if (error !=0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                             "%s : failed to set channel with error code %d\n",
                             __func__, error);
            goto done;
        }
    }

done:
#ifdef CONFIG_AFC_SUPPORT
    if (error &&
        ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(vap->iv_ic->ic_curchan))) {
        ieee80211_update_mode_and_power_type(vap, vap->iv_ic->ic_curchan,
                                             orig_mode);
        vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
    }
#endif /* CONFIG_AFC_SUPPORT */

    wlan_cfg80211_acs_report_channel(vap, channel);
}

#if UMAC_SUPPORT_ACS
int wlan_cfg80211_start_acs_scan(wlan_if_t vap,
                        cfg80211_hostapd_acs_params *cfg_acs_params)
{
   struct wlan_mbss_ev_data mbss_ev = {0};

    mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_ACS;
    mbss_ev.ext_cb = &cfg80211_acs_event_handler;
    mbss_ev.source.acs_source = MBSS_ACS_SRC_CFG;
    mbss_ev.arg = cfg_acs_params;

    return wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
}
#endif /* UMAC_SUPPORT_ACS */
#endif /* UMAC_SUPPORT_CFG80211*/

/*
* Auto Channel Select handler used for interface up.
*/
void osif_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
    osif_dev *osifp;
    wlan_if_t vap;
    int error = 0;
    uint32_t bcn_rate;
    uint32_t mgmt_rate;
    struct wlan_vdev_mgr_cfg mlme_cfg;
    wlan_chan_t channel = (wlan_chan_t)arg;
    enum ieee80211_phymode orig_mode;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_info("vap is null");
        goto done;
    }

    orig_mode = vap->iv_des_mode;

    osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);

    if (osifp->is_delete_in_progress) {
        goto done;
    }

     /* In cases where ACS fails to derive the channel,
      * the acs event handler is called with IEEE80211_CHAN_ANYC,
      * we can start the monitor vap with the default channel.
      */
     if (!channel || channel == IEEE80211_CHAN_ANYC) {

        qdf_err("ACS event handler vdev[%d], channel: %pK",
                 wlan_vdev_get_id(vap->vdev_obj), channel);

        if (wlan_vdev_mlme_get_opmode(vap->vdev_obj) == QDF_MONITOR_MODE)
            wlan_mlme_start_monitor(vap);
        else {
            wlan_mlme_release_vdev_req(vap->vdev_obj,
                                       WLAN_SER_CMD_VDEV_START_BSS, EINVAL);
            error = -1;
        }
        goto done;
    }

    /*
     * Check security configuration for 6GHz only for beaconing AP VAPs
     * and STA VAPs
     */
    if (wlan_reg_is_6ghz_chan_freq(channel->ic_freq) &&
        ieee80211_mbss_is_beaconing_ap(vap)) {
        error = wlan_cfg80211_6ghz_security_check(vap);
        if (error != EOK) {
            qdf_err("Error %d in 6GHz security configured !!", error);
            wlan_mlme_release_vdev_req(vap->vdev_obj,
                                       WLAN_SER_CMD_VDEV_START_BSS, EINVAL);
            goto done;
        }
    }

#ifdef CONFIG_AFC_SUPPORT
    if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(channel))) {
        error = ieee80211_update_mode_and_power_type(vap, channel,
                                                     ieee80211_chan2mode(channel));
        if (error) {
            qdf_err("Failed to change mode and regulatory AP power type");
            wlan_mlme_release_vdev_req(vap->vdev_obj,
                                       WLAN_SER_CMD_VDEV_START_BSS, EINVAL);
            goto done;
        }
    }
#endif /* CONFIG_AFC_SUPPORT */

    error = wlan_set_ieee80211_channel(vap, channel);
    if (error !=0) {
        qdf_err("Failed to set channel with error code %d", error);
        wlan_mlme_release_vdev_req(vap->vdev_obj,
                                   WLAN_SER_CMD_VDEV_START_BSS, EINVAL);
        goto done;
    }

    if (wlan_vdev_mlme_get_opmode(vap->vdev_obj) == QDF_MONITOR_MODE) {
        wlan_mlme_start_monitor(vap);
        goto done;
    }
    osif_acs_start_bss(vap, channel);

    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_BCN_TX_RATE, &bcn_rate);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_TX_MGMT_RATE, &mgmt_rate);

    /*setting mcast rate is defered */
    if (vap->iv_mcast_rate_config_defered) {
        wlan_set_param(vap, IEEE80211_MCAST_RATE, vap->iv_mcast_rate);
        vap->iv_mcast_rate_config_defered = FALSE;
    }

    /*setting bcast rate is defered */
    if (vap->iv_bcast_rate_config_defered) {
        wlan_set_param(vap, IEEE80211_BCAST_RATE, vap->iv_bcast_fixedrate);
        vap->iv_bcast_rate_config_defered = FALSE;
    }

    /*setting mgmt rate is defered */
    if (vap->iv_mgt_rate_config_defered) {
        wlan_set_param(vap, IEEE80211_MGMT_RATE, mgmt_rate);
        vap->iv_mgt_rate_config_defered = FALSE;
    }

    /*Set default mgmt rate*/
    if (!ieee80211_rate_is_valid_basic(vap, mgmt_rate)){
        if(IEEE80211_IS_CHAN_5GHZ_6GHZ(channel)){
            mlme_cfg.value = DEFAULT_LOWEST_RATE_5G;
        } else if(IEEE80211_IS_CHAN_2GHZ(channel)){
            mlme_cfg.value = DEFAULT_LOWEST_RATE_2G;
        }
        ucfg_wlan_vdev_mgr_set_param(vap->vdev_obj,
                WLAN_MLME_CFG_TX_MGMT_RATE, mlme_cfg);
    }

    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_TX_MGMT_RATE, &mgmt_rate);

    /* if rates set, use previous rates */
    /* for offload radios, fw will reset rates to default everytime VAP bringup */
    if(mgmt_rate){
        wlan_set_param(vap, IEEE80211_MGMT_RATE, mgmt_rate);
    }
    if (vap->iv_disabled_legacy_rate_set) {
        wlan_set_param(vap, IEEE80211_RTSCTS_RATE, mgmt_rate);
    }
    if(vap->iv_bcast_fixedrate){
        wlan_set_param(vap, IEEE80211_BCAST_RATE, vap->iv_bcast_fixedrate);
    }
    if(vap->iv_mcast_fixedrate){
        wlan_set_param(vap, IEEE80211_MCAST_RATE, vap->iv_mcast_fixedrate);
    }

    if(bcn_rate) {
        wlan_set_param(vap, IEEE80211_BEACON_RATE_FOR_VAP, bcn_rate);
    }

done:
#ifdef CONFIG_AFC_SUPPORT
    if (error &&
        ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(vap->iv_ic->ic_curchan))) {
        ieee80211_update_mode_and_power_type(vap, vap->iv_ic->ic_curchan,
                                             orig_mode);
        vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
    }
#endif /* CONFIG_AFC_SUPPORT */

    return;
}

void preempt_scan(struct net_device *dev, int max_grace, int max_wait)
{
#define PREEMPT_SCAN_DELAY ((CONVERT_SEC_TO_SYSTEM_TIME(1)/1000) + 1) /* 1 msec */
    osif_dev  *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    int total_delay = 0;
    int canceled = 0, ready = 0;
    struct wlan_objmgr_vdev *vdev = NULL;
    QDF_STATUS status;

    vdev = osifp->ctrl_vdev;
    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_OSIF_SCAN_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        scan_info("unable to get reference");
        return;
    }

    while (!ready && total_delay < max_grace + max_wait) {
        if (!wlan_vdev_scan_in_progress(vap->vdev_obj)) {
            ready = 1;
        } else {
            if (!canceled && total_delay > max_grace) {
                /*
                   Cancel any existing active scan, so that any new parameters
                   in this scan ioctl (or the defaults) can be honored, then
                   wait around a while to see if the scan cancels properly.
                 */
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
                        "%s: cancel pending scan request\n", __func__);
                wlan_ucfg_scan_cancel(vap, osifp->scan_requestor, 0,
                        WLAN_SCAN_CANCEL_VDEV_ALL, false);
                canceled = 1;
            }

            schedule_timeout_interruptible(PREEMPT_SCAN_DELAY);
            total_delay += 1;
        }
    }
    if (!ready) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
                "%s: Timeout canceling current scan.\n",
                __func__);
    }
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
}

void delete_default_vap_keys(struct ieee80211vap *vap)
{
    int i;
    /*
     * delete any default keys, if the vap is to be deleted.
     */
     for (i=0;i<IEEE80211_WEP_NKID; ++i) {
        u_int8_t macaddr[] = {0xff,0xff,0xff,0xff,0xff,0xff };
        wlan_del_key(vap,i,macaddr);
     }
}

static void
osif_auth_complete_ap(os_handle_t osif, u_int8_t *macaddr, IEEE80211_STATUS status)
{

	struct net_device *dev = get_cfg80211_notification_ndev(((osif_dev *)osif));
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	struct ev_msg msg;
	union iwreq_data wrqu;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
		qdf_err(" VAP is null!!!");
		return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_AUTH_AP)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			acfg_event->downlink = 0;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_AUTH_AP, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif
	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_AUTH_COMPLETE_AP)) {
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		msg.status = status;
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_AUTH_COMPLETE_AP;
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_assoc_complete_ap(os_handle_t osif, IEEE80211_STATUS status,
						u_int16_t aid, wbuf_t wbuf)
{
	struct net_device *dev = get_cfg80211_notification_ndev(((osif_dev *)osif));
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	union iwreq_data wrqu;
	struct ev_msg msg;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_ASSOC_AP)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			acfg_event->downlink = 0;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_ASSOC_AP, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif
	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_ASSOC_COMPLETE_AP)) {
		msg.status = status;
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_ASSOC_COMPLETE_AP;
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_deauth_complete_ap(os_handle_t osif, u_int8_t *macaddr,
			IEEE80211_STATUS status, u_int16_t associd,
			u_int16_t reason, u_int8_t *mldaddr)
{
	struct net_device *dev;
	union iwreq_data wreq;
	u_int8_t *msg;
	union iwreq_data wrqu;
	struct ev_msg evmsg;
        osif_dev *osdev = (osif_dev *)osif;
	wlan_if_t vap = osdev->os_if;
	union iwreq_data staleave_wreq;
	struct ev_sta_leave staleave_ev;
#if ATH_SUPPORT_WAPI
	union iwreq_data wapi_wreq;
	u_int8_t *sta_msg;
	int msg_len;
#endif
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
#if QCA_SUPPORT_SON
	struct wlan_assoc_disassoc_event_info info;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#endif

	if (!vap) {
		qdf_err("vap is null");
		return;
	}

	if (!vap->vdev_obj) {
		qdf_err("vdev is null");
		return;
	}

        dev = get_cfg80211_notification_ndev(osdev);

        if (!dev) {
            qdf_err("netdev is NULL");
            return;
        }

	msg = NULL;
	/* fire off wireless event station leaving */
	memset(&wreq, 0, sizeof(wreq));
	IEEE80211_ADDR_COPY(wreq.addr.sa_data, macaddr);
	wreq.addr.sa_family = ARPHRD_ETHER;
	WIRELESS_SEND_EVENT(dev, IWEVEXPIRED, &wreq, NULL);
#if QCA_SUPPORT_SON
	qdf_mem_zero(&info, sizeof(info));
	qdf_mem_copy(info.macaddr, macaddr, QDF_MAC_ADDR_SIZE);
	info.flag = ACTION_DISASSOC;
	info.reason = reason;
	psoc = wlan_vdev_get_psoc(vap->vdev_obj);
	if (psoc) {
		rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
	}
	if (rx_ops && rx_ops->son_rx_ops.deliver_event)
		rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_ASSOC_DISASSOC, &info);
#endif
#if ATH_SUPPORT_WAPI
	sta_msg = wlan_wapi_callback_begin(vap, macaddr, &msg_len, WAPI_STA_AGING);
	if (sta_msg) {
		if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_WAPI)) {
			OS_MEMSET(&wapi_wreq, 0, sizeof(wapi_wreq));
			wapi_wreq.data.length = msg_len;
			wapi_wreq.data.flags = IEEE80211_EV_WAPI;
			WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wapi_wreq, sta_msg);
		}
		msg = sta_msg;
		wlan_wapi_callback_end(sta_msg);
	}
#endif
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG,
				WL_EVENT_TYPE_DEAUTH_AP)){
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			acfg_event->reason = reason;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 0;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_DEAUTH_AP, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if(msg==NULL) {
		if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
					IEEE80211_EV_DEAUTH_COMPLETE_AP)) {
			memset(&evmsg, 0, sizeof(evmsg));
			evmsg.status = status;
			IEEE80211_ADDR_COPY(evmsg.addr, macaddr);
			memset(&wrqu, 0, sizeof(wrqu));
			wrqu.data.flags = IEEE80211_EV_DEAUTH_COMPLETE_AP;
			wrqu.data.length = sizeof(evmsg);
#ifdef WLAN_FEATURE_11BE_MLO
			if (mldaddr && IEEE80211_ADDR_IS_VALID(mldaddr))
				IEEE80211_ADDR_COPY(evmsg.mldaddr, mldaddr);
#endif /*WLAN_FEATURE_11BE_MLO*/
			WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&evmsg);
		}
#if UMAC_SUPPORT_CFG80211
	if(vap->iv_cfg80211_create) {
	    cfg80211_del_sta(dev, macaddr, GFP_ATOMIC);
	}
#endif
	}

	/* check for smart mesh configuration and send event */
	if (vap->iv_smart_mesh_cfg & SMART_MESH_80211_EVENTS) {
		if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
					IEEE80211_EV_STA_LEAVE)) {
			OS_MEMZERO(&staleave_ev, sizeof(staleave_ev));

			/* populate required fileds */
			OS_MEMCPY(&(staleave_ev.mac_addr), macaddr, QDF_MAC_ADDR_SIZE);
			staleave_ev.channel_num = vap->iv_bsschan->ic_ieee;
			staleave_ev.assoc_id = IEEE80211_AID(associd);
			staleave_ev.reason = reason;

			/* prepare custom event */
			staleave_wreq.data.flags = IEEE80211_EV_STA_LEAVE;
			staleave_wreq.data.length = sizeof(staleave_ev);
			WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &staleave_wreq, (char *)&staleave_ev);
		}
	}

	return;
}

static void
osif_auth_indication_ap(os_handle_t osif, u_int8_t *macaddr,
							u_int16_t status)
{
	struct net_device *dev = get_cfg80211_notification_ndev(((osif_dev *)osif));
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	struct ev_msg msg;
	union iwreq_data wrqu;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_AUTH_AP)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 1;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_AUTH_AP, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif
	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_AUTH_IND_AP)) {
		msg.status = status;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_AUTH_IND_AP;
		wrqu.data.length = sizeof(msg);
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_assoc_indication_ap(os_handle_t osif, u_int8_t *macaddr,
							u_int16_t result, wbuf_t wbuf,
							wbuf_t resp_buf, bool reassoc,
							u_int8_t *mldaddr)
{
    struct net_device *dev;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    union iwreq_data wreq;
#if ATH_SUPPORT_WAPI
    union iwreq_data wapi_wreq;
    u_int8_t *sta_msg;
    int msg_len;
#endif
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
    struct ev_msg msg;

    dev = get_cfg80211_notification_ndev(osifp);

    if (!dev) {
        qdf_err("netdev is NULL");
        return;
    }

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }

    if (osifp->os_opmode != IEEE80211_M_P2P_GO) {
        if (!wlan_get_param(vap, IEEE80211_TRIGGER_MLME_RESP))
        {
        	OS_MEMSET(&wreq, 0, sizeof(wreq));
	        IEEE80211_ADDR_COPY(wreq.addr.sa_data, macaddr);
	        wreq.addr.sa_family = ARPHRD_ETHER;
        	WIRELESS_SEND_EVENT(dev, IWEVREGISTERED, &wreq, NULL);
        }
        if  (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_ASSOC_IND_AP) ||
                osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_REASSOC_IND_AP)) {
            OS_MEMSET(&msg, 0, sizeof(msg));
            IEEE80211_ADDR_COPY(msg.addr, macaddr);
#ifdef WLAN_FEATURE_11BE_MLO
            if (mldaddr && IEEE80211_ADDR_IS_VALID(mldaddr))
                IEEE80211_ADDR_COPY(msg.mldaddr, mldaddr);
#endif /*WLAN_FEATURE_11BE_MLO*/
            memset(&wreq, 0, sizeof(wreq));
            if(reassoc)
                wreq.data.flags = IEEE80211_EV_REASSOC_IND_AP;
            else
                wreq.data.flags = IEEE80211_EV_ASSOC_IND_AP;
            wreq.data.length = sizeof(msg);
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (char *)&msg);
        }

#if ATH_SUPPORT_WAPI
        sta_msg = wlan_wapi_callback_begin(vap, macaddr, &msg_len, WAPI_WAI_REQUEST);
        if (sta_msg) {
#if UMAC_SUPPORT_CFG80211
            if(vap->iv_cfg80211_create){
                u8 *ies = NULL;
                size_t ies_len = 0;
                struct station_info sinfo;
                struct ieee80211_node *ni = NULL;
                ni = ieee80211_find_node(vap->iv_ic, macaddr, WLAN_MLME_SB_ID);
                if(ni) {
                    ies = ni->ni_wpa_ie;
                    ies_len = ies[1] + 2;
                    memset(&sinfo, 0, sizeof(sinfo));
                    sinfo.assoc_req_ies = ies;
                    sinfo.assoc_req_ies_len = ies_len;
                    cfg80211_new_sta(dev, macaddr, &sinfo, GFP_KERNEL);
                    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                }
            }
            else
#endif
            {
                if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                            IEEE80211_EV_WAPI)) {
                    OS_MEMSET(&wapi_wreq, 0, sizeof(wapi_wreq));
                    wapi_wreq.data.length = msg_len;
                    wapi_wreq.data.flags = IEEE80211_EV_WAPI;
                    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wapi_wreq, sta_msg);
                }
                wlan_wapi_callback_end(sta_msg);
            }
        }
#endif
    }
    else { /* For P2P supplicant needs entire stuff */
        char *concat_buf;
        int wbuf_len =  wbuf_get_pktlen(wbuf);

        concat_buf = OS_MALLOC(osifp->os_handle, wbuf_len + QDF_MAC_ADDR_SIZE,
                                GFP_ATOMIC);
        if (!concat_buf) {
            return;
        }
        OS_MEMSET(&wreq, 0, sizeof(wreq));
        wreq.data.length = wbuf_len + QDF_MAC_ADDR_SIZE;
        IEEE80211_ADDR_COPY(concat_buf, macaddr);
        OS_MEMCPY(concat_buf+QDF_MAC_ADDR_SIZE, wbuf_header(wbuf), wbuf_len);

        WIRELESS_SEND_EVENT(dev, IWEVGENIE, &wreq, concat_buf);
        OS_FREE(concat_buf);
    }

#if UMAC_SUPPORT_ACFG
	if (!osif_vap_event_filter_status_to_app(vap, EVENT_ACFG,
				WL_EVENT_TYPE_ASSOC_AP)) {
		return;
	}
	acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
	if (acfg_event == NULL)
		return;

	acfg_event->result = result;
	IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
	acfg_event->downlink = 1;
	acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_ASSOC_AP, acfg_event);
	qdf_mem_free(acfg_event);
#endif
	return;
}

static void osif_deauth_indication_ap(os_handle_t osif, u_int8_t *macaddr,
                                      u_int16_t associd, u_int16_t reason,
                                      u_int8_t *mldaddr)
{
    struct net_device *dev;
    union iwreq_data wreq;
    u_int8_t *msg;
    union iwreq_data wrqu;
    struct ev_msg evmsg;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    union iwreq_data staleave_wreq;
    struct ev_sta_leave staleave_ev;
#if ATH_SUPPORT_WAPI
    union iwreq_data wapi_wreq;
    u_int8_t *sta_msg;
    int msg_len;
#endif
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event = NULL;
#endif
#if QCA_SUPPORT_SON
    struct wlan_assoc_disassoc_event_info info;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#endif
    uint16_t no_delsta = (IEEE80211_REASON_NO_DEL_STA & reason);

    reason &= (~IEEE80211_REASON_NO_DEL_STA);
    dev = get_cfg80211_notification_ndev(osifp);

    if (!dev) {
        qdf_err("netdev is NULL");
        return;
    }

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }

    msg = NULL;
    /* fire off wireless event station leaving */
    memset(&wreq, 0, sizeof(wreq));
    IEEE80211_ADDR_COPY(wreq.addr.sa_data, macaddr);
    wreq.addr.sa_family = ARPHRD_ETHER;
    WIRELESS_SEND_EVENT(dev, IWEVEXPIRED, &wreq, NULL);
#if QCA_SUPPORT_SON
    qdf_mem_zero(&info, sizeof(info));
    qdf_mem_copy(info.macaddr, macaddr, QDF_MAC_ADDR_SIZE);
    info.flag = ACTION_DISASSOC;
    info.reason = reason;
    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }
    if (rx_ops && rx_ops->son_rx_ops.deliver_event)
        rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_ASSOC_DISASSOC, &info);
#endif
#if ATH_SUPPORT_WAPI
    sta_msg = wlan_wapi_callback_begin(vap, macaddr, &msg_len, WAPI_STA_AGING);
    if (sta_msg) {
        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_WAPI)) {
            OS_MEMSET(&wapi_wreq, 0, sizeof(wapi_wreq));
            wapi_wreq.data.length = msg_len;
            wapi_wreq.data.flags = IEEE80211_EV_WAPI;
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wapi_wreq, sta_msg);
        }
        wlan_wapi_callback_end(sta_msg);
    }
    msg = sta_msg;
#endif
#if UMAC_SUPPORT_ACFG
    if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_DEAUTH_AP)) {
        acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
        if (acfg_event) {
            acfg_event->reason = reason;
            IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
            acfg_event->downlink = 1;
            acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_DEAUTH_AP, acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#endif
    if (msg == NULL) {
        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_DEAUTH_IND_AP)) {
            memset(&evmsg, 0, sizeof(evmsg));
            evmsg.reason = reason;
            memset(&wrqu, 0, sizeof(wrqu));
            wrqu.data.flags = IEEE80211_EV_DEAUTH_IND_AP;
            wrqu.data.length = sizeof(evmsg);
            IEEE80211_ADDR_COPY(evmsg.addr, macaddr);
#ifdef WLAN_FEATURE_11BE_MLO
            if (mldaddr && IEEE80211_ADDR_IS_VALID(mldaddr))
                IEEE80211_ADDR_COPY(evmsg.mldaddr, mldaddr);
#endif /*WLAN_FEATURE_11BE_MLO*/
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&evmsg);
        }
#if UMAC_SUPPORT_CFG80211
        if(vap->iv_cfg80211_create && !no_delsta) {
           cfg80211_del_sta(dev, macaddr, GFP_ATOMIC);
        }
#endif
    }

    /* check for smart mesh configuration and send event */
    if (vap->iv_smart_mesh_cfg & SMART_MESH_80211_EVENTS) {

        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_STA_LEAVE)) {
            OS_MEMZERO(&staleave_ev, sizeof(staleave_ev));

            /* populate required fileds */
            OS_MEMCPY(&(staleave_ev.mac_addr), macaddr, QDF_MAC_ADDR_SIZE);
            staleave_ev.channel_num = vap->iv_bsschan->ic_ieee;
            staleave_ev.assoc_id = IEEE80211_AID(associd);
            staleave_ev.reason = reason;

            /* prepare custom event */
            staleave_wreq.data.flags = IEEE80211_EV_STA_LEAVE;
            staleave_wreq.data.length = sizeof(staleave_ev);
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &staleave_wreq, (char *)&staleave_ev);
        }

    }

    return;
}


static void
osif_auth_complete_sta(os_handle_t osif, u_int8_t *macaddr, IEEE80211_STATUS status)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	union iwreq_data wrqu;
	struct ev_msg msg = {0};
	wlan_if_t vap = osifp->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}

	if (vap->iv_cfg80211_create && vap->iv_roam.iv_roaming) {
		qdf_sched_work(dev, &vap->cfg80211_ft_supp_workq);
	}

#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_AUTH_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			acfg_event->downlink = 0;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_AUTH_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_AUTH_COMPLETE_STA)) {
		msg.status = status;
		if (macaddr)
			IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_AUTH_COMPLETE_STA;
		wrqu.data.length = sizeof(msg);
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_assoc_complete_sta_common(os_handle_t osif, IEEE80211_STATUS status,
		u_int16_t aid, wbuf_t wbuf, u_int8_t frame_subtype)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_CFG80211
	wlan_if_t vap = osifp->os_if;
#endif
	union iwreq_data wrqu;
	struct ev_msg msg = {0};
	struct ieee80211_frame *wh;

	/* mlme_cancel will send NULL wbuf*/
	if (wbuf)
		ieee80211_update_assoc_ie(vap, wbuf);
	if(!vap->iv_cfg80211_create) {
#if UMAC_SUPPORT_ACFG
		acfg_event_data_t *acfg_event = NULL;
		if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_ASSOC_STA)) {
			acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
			if (acfg_event) {
				acfg_event->result = status;
				acfg_event->downlink = 0;
				acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_ASSOC_STA, acfg_event);
				qdf_mem_free(acfg_event);
			}
		}
#endif
	}

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_ASSOC_COMPLETE_STA)) {
		if (wbuf) {
			/* Copy BSSID */
			wh = (struct ieee80211_frame *) wbuf_header(wbuf);
			IEEE80211_ADDR_COPY(msg.addr, wh->i_addr3);
		}
		msg.frame_subtype = frame_subtype;

		msg.status = status;
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_ASSOC_COMPLETE_STA;
		wrqu.data.length = sizeof(msg);
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_assoc_complete_sta(os_handle_t osif, IEEE80211_STATUS status,
		u_int16_t aid, wbuf_t wbuf)
{
	osif_assoc_complete_sta_common(osif, status, aid, wbuf,
					IEEE80211_FC0_SUBTYPE_ASSOC_RESP);
}

static void
osif_reassoc_complete_sta(os_handle_t osif, IEEE80211_STATUS status,
		u_int16_t aid, wbuf_t wbuf)
{
	osif_assoc_complete_sta_common(osif, status, aid, wbuf,
					IEEE80211_FC0_SUBTYPE_REASSOC_RESP);
}

static void
osif_deauth_complete_sta(os_handle_t osif, u_int8_t *macaddr,
				IEEE80211_STATUS status, u_int16_t associd,
				u_int16_t reason, u_int8_t *mldaddr)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	union iwreq_data wrqu;
	struct ev_msg msg;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}

    memset(&wrqu, 0, sizeof(wrqu));
    IEEE80211_ADDR_COPY(wrqu.addr.sa_data, macaddr);
    wrqu.addr.sa_family = ARPHRD_ETHER;
    WIRELESS_SEND_EVENT(dev, IWEVEXPIRED, &wrqu, NULL);

#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG,
				WL_EVENT_TYPE_DEAUTH_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 0;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_DEAUTH_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_DEAUTH_COMPLETE_STA)) {
		msg.status = status;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_DEAUTH_COMPLETE_STA;
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_disassoc_complete_sta(os_handle_t osif, u_int8_t *macaddr,
							u_int32_t reason,
							IEEE80211_STATUS status,
							u_int8_t *mldaddr)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event;
#endif
	struct ev_msg msg;
	union iwreq_data wrqu;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}

#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_DISASSOC_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			acfg_event->reason = reason;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 0;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_DISASSOC_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_DISASSOC_COMPLETE_STA)) {
		msg.status = status;
		msg.reason = reason;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_DISASSOC_COMPLETE_STA;
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_auth_indication_sta(os_handle_t osif, u_int8_t *macaddr,
							u_int16_t status)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	struct ev_msg msg;
	union iwreq_data wrqu;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_AUTH_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = status;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 1;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_AUTH_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_AUTH_IND_STA)) {
		msg.status = status;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_AUTH_IND_STA;
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_deauth_indication_sta(os_handle_t osif, u_int8_t *macaddr,
							u_int16_t associd,
							u_int16_t reason,
							u_int8_t *mldaddr)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	union iwreq_data wrqu;
	struct ev_msg msg = {0};
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_DEAUTH_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->reason = reason;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 1;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_DEAUTH_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_DEAUTH_IND_STA)) {
		msg.reason = reason;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_DEAUTH_IND_STA;
		wrqu.data.length = sizeof(msg);
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void
osif_unprotected_deauth_indication_sta(os_handle_t osif, u_int8_t *macaddr,
                                       u_int16_t associd, u_int16_t reason,
                                       wbuf_t wbuf)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
        union iwreq_data wrqu;
        struct ev_msg msg;
#if UMAC_SUPPORT_CFG80211
        int wbuf_len =  wbuf_get_pktlen(wbuf);
#endif
        wlan_if_t vap = ((osif_dev *)osif)->os_if;

        if (!vap) {
            qdf_err(" VAP is null!!!");
            return;
        }

        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_UNPROTECTED_DEAUTH_IND_STA)) {
            msg.reason = reason;
            /* sa/ta addr / addr2 */
            IEEE80211_ADDR_COPY(msg.addr, macaddr);
            memset(&wrqu, 0, sizeof(wrqu));
            wrqu.data.flags = IEEE80211_EV_UNPROTECTED_DEAUTH_IND_STA;
            wrqu.data.length = sizeof(struct ev_msg);
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
        }
#if UMAC_SUPPORT_CFG80211
        cfg80211_rx_unprot_mlme_mgmt(dev, wbuf_header(wbuf), wbuf_len);
#endif
        return;

}

static void
osif_assoc_indication_sta(os_handle_t osif, u_int8_t *macaddr,
							u_int16_t result, wbuf_t wbuf,
							wbuf_t resp_buf, bool reassoc,
							u_int8_t *mldaddr)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	struct ev_msg msg;
	union iwreq_data wrqu;
	wlan_if_t vap = ((osif_dev *)osif)->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_ASSOC_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->result = result;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 1;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_ASSOC_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_ASSOC_IND_STA)) {
		msg.status = result;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_ASSOC_IND_STA;
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void osif_disassoc_indication_sta(os_handle_t osif,
									       u_int8_t *macaddr,
									       u_int16_t associd,
									       u_int32_t reason,
									       u_int8_t *mldaddr)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif
	struct ev_msg msg = {0};
	union iwreq_data wrqu;
	wlan_if_t vap = osifp->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}
#if UMAC_SUPPORT_ACFG
	if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_DISASSOC_STA)) {
		acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
		if (acfg_event) {
			acfg_event->reason = reason;
			IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
			acfg_event->downlink = 1;
			acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
					WL_EVENT_TYPE_DISASSOC_STA, acfg_event);
			qdf_mem_free(acfg_event);
		}
	}
#endif

	if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_DISASSOC_IND_STA)) {
		msg.reason = reason;
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.flags = IEEE80211_EV_DISASSOC_IND_STA;
		wrqu.data.length = sizeof(msg);
		WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	}
	return;
}

static void osif_authorized_indication_sta(os_handle_t osif,
		u_int8_t *macaddr)
{
	osif_dev *osifp = (osif_dev *)osif;
	struct net_device *dev = get_cfg80211_notification_ndev(osifp);
	struct ev_msg msg = {0};
	union iwreq_data wrqu;
	wlan_if_t vap = osifp->os_if;

	if (!vap) {
	    qdf_err(" VAP is null!!!");
	    return;
	}

	if (!osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
				IEEE80211_EV_AUTHORIZED_IND_STA)) {
		return;
	}
	if (macaddr)
		IEEE80211_ADDR_COPY(msg.addr, macaddr);
	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.flags = IEEE80211_EV_AUTHORIZED_IND_STA;
	wrqu.data.length = sizeof(msg);
	WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
	return;
}

#if UMAC_SUPPORT_RRM_MISC
static void  osif_nonerpcnt(os_handle_t osif, u_int8_t nonerpcnt)
{
    ath_netlink_event_t event;
    memset(&event, 0x0, sizeof(event));
    event.type = ATH_EVENT_NODE_NONERP_JOINED;
    ath_adhoc_netlink_send(&event, (char *)&nonerpcnt, sizeof(u_int8_t));
    return;
}

static void  osif_bgjoin(os_handle_t osif, u_int8_t value)
{
    ath_netlink_event_t event;
    memset(&event, 0x0, sizeof(event));
    event.type = ATH_EVENT_NODE_BG_JOINED;
    ath_adhoc_netlink_send(&event, (char *)&value, sizeof(u_int8_t));
    return;
}

static void  osif_cochannelap_cnt(os_handle_t osif, u_int8_t cnt)
{
    ath_netlink_event_t event;
    memset(&event, 0x0, sizeof(event));
    event.type = ATH_EVENT_NODE_COCHANNEL_AP_CNT;
    ath_adhoc_netlink_send(&event, (char *)&cnt, sizeof(u_int8_t));
    return;
}
static void  osif_chload(os_handle_t osif, u_int8_t chload)
{
    ath_netlink_event_t event;
    memset(&event, 0x0, sizeof(event));
    event.type = ATH_EVENT_NODE_CHLOAD;
    ath_adhoc_netlink_send(&event, (char *)&chload, sizeof(u_int8_t));
    return;
}
#endif
static void  osif_ch_hop_channel_change(os_handle_t osif, u_int8_t channel)
{
    ath_netlink_event_t event;
    memset(&event, 0x0, sizeof(event));
    event.type = ATH_EVENT_CH_HOP_CHANNEL_CHANGE;
    ath_adhoc_netlink_send(&event, (char *)&channel, sizeof(u_int8_t));
    return;
}

static void osif_recv_probereq(os_handle_t osif, u_int8_t *mac_addr ,  u_int8_t *ssid_element)
{
      osif_dev *osifp = (osif_dev *)osif;
      struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    wlan_if_t vap = osifp->os_if;
    union iwreq_data wreq;
    struct ieee80211_node *ni = NULL;
    struct ieee80211_node *ni_bss = NULL;
    u_int8_t ssid_len = 0;
    struct ev_recv_probereq probe_ev;

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }
    ssid_len = ssid_element[1];
    ni_bss = vap->iv_bss;

    if (vap->iv_smart_mesh_cfg & SMART_MESH_80211_EVENTS) {

        if ((ssid_len == 0) || ((ssid_len == ni_bss->ni_esslen)
                                && (OS_MEMCMP((ssid_element)+2, ni_bss->ni_essid, ssid_len) == 0))) {
            ni = ieee80211_find_node(vap->iv_ic, mac_addr, WLAN_MLME_SB_ID);
            if (ni) {
                if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                            IEEE80211_EV_RECV_PROBEREQ)) {
                    OS_MEMZERO(&probe_ev, sizeof(probe_ev));

                    /* populate required fileds */
                    OS_MEMCPY(&(probe_ev.mac_addr), ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
#ifdef QCA_SUPPORT_CP_STATS
                    probe_ev.snr = peer_cp_stats_rx_mgmt_snr_get(ni->peer_obj);
                    probe_ev.rate = peer_cp_stats_rx_mgmt_rate_get(ni->peer_obj);
#endif
                    probe_ev.channel_num =
                        wlan_reg_freq_to_chan(ni->ni_ic->ic_pdev_obj,
                                ni->ni_ch_param.ni_freq);

                    /* prepare custom event */
                    wreq.data.flags = IEEE80211_EV_RECV_PROBEREQ;
                    wreq.data.length = sizeof(probe_ev);
                    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (char *)&probe_ev);
                }
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            }

        }
    }
    return;
}

static void osif_recv_neighreq(os_handle_t osif, u_int8_t *macaddr, void *arg)
{
    ieee80211_rrm_nrreq_info_t *nrreq = (ieee80211_rrm_nrreq_info_t *)arg;
    struct ev_neigh_req_recv neighreq_ev;
    union iwreq_data wreq;
    osif_dev *osifp = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }

    if (!osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                IEEE80211_EV_NEIGH_REQ_RECV_AP)) {
        return;
    }
    IEEE80211_ADDR_COPY(neighreq_ev.mac_addr, macaddr);
    qdf_mem_copy(&neighreq_ev.nrreq, nrreq,
                 sizeof(ieee80211_rrm_nrreq_info_t));
    wreq.data.flags = IEEE80211_EV_NEIGH_REQ_RECV_AP;
    wreq.data.length = sizeof(neighreq_ev);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (char *)&neighreq_ev);
}

#if DBDC_REPEATER_SUPPORT
static void osif_dbdc_stavap_connection (os_handle_t osif)
{
    osif_dev *osdev = (osif_dev *)osif;
    wlan_if_t vap;
    u_int8_t *macaddr;

    vap = osdev->os_if;
    macaddr = wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);

    if (!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj) || qdf_is_macaddr_zero((struct qdf_mac_addr *)macaddr))
        macaddr = vap->iv_myaddr;

    qdf_info("Send layer2 update frame on vap:%s to detect loop",ether_sprintf(macaddr));
    osif_hardstart_l2uf(osif, macaddr);
}
#endif


static void osif_session_timeout(os_handle_t osif, u_int8_t *mac)
{
#if MESH_MODE_SUPPORT || UMAC_SUPPORT_ACFG
    osif_dev *osifp = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
#endif
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event = NULL;
#endif

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }
#if MESH_MODE_SUPPORT
    if (dev &&
        osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                IEEE80211_EV_MESH_PEER_TIMEOUT)) {
        union iwreq_data  wrqu;
        struct ev_msg evmsg;
        evmsg.reason = IEEE80211_REASON_KICK_OUT;
        memset(&wrqu, 0, sizeof(wrqu));
        wrqu.data.flags = IEEE80211_EV_MESH_PEER_TIMEOUT;
        IEEE80211_ADDR_COPY(evmsg.addr, mac);
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&evmsg);
    }
#endif

#if UMAC_SUPPORT_ACFG
    if (!dev ||
        !osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, ACFG_SESSION_TIMEOUT)) {
        return;
    }
    acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
    if (acfg_event == NULL)
        return;

    acfg_event->reason = ACFG_SESSION_TIMEOUT;
    IEEE80211_ADDR_COPY(acfg_event->addr, mac);
    acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_STA_SESSION, acfg_event);
    qdf_mem_free(acfg_event);
#endif
    return;
}
#if QCA_SUPPORT_SSID_STEERING
static void  osif_ssid_event(os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
#if ATH_DEBUG
    wlan_if_t vap = osifp->os_if;
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: macaddr(%02x:%02x:%02x:%02x:%02x:%02x) VAP VALUE(%d)\n",
            __func__, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

    ath_ssid_steering_send_event(macaddr);
} /* void (*ssid_event)(os_handle_t, u_int8_t *macaddr); */
#endif

static void  osif_assocdeny_event(os_handle_t osif, u_int8_t *macaddr)
{
    static const char *tag = "MLME-ASSOCDENIAL.indication";
    osif_dev *osdev        = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap          = osdev->os_if;
    union iwreq_data         wrqu;
    char *buf              = NULL;
#define BUF_SIZE           128

    if (!dev) {
        qdf_err("netdev is NULL for this vap type");
        return;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: macaddr(%pM) VAP(%d) device %s\n",__func__, macaddr, vap->iv_unit, dev->name);
    if(NULL != (buf = OS_MALLOC(osif, BUF_SIZE, GFP_KERNEL))){
        OS_MEMZERO(&wrqu, sizeof(wrqu));
        OS_MEMZERO(buf, BUF_SIZE);
        snprintf(buf, BUF_SIZE, "%s(macaddr=%pM, vap=%s)", tag, macaddr, dev->name);
        wrqu.data.length = strlen(buf);
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);
        OS_FREE(buf);
    } else
        qdf_print(KERN_ERR "%s: failed to allocate memory of size (%d)",__func__,BUF_SIZE);
#undef BUF_SIZE
} /* void (*assocdeny_event)(os_handle_t, u_int8_t *macaddr); */

static void osif_leave_indication_ap(os_handle_t osif, u_int8_t *macaddr, u_int16_t associd, u_int32_t reason, u_int8_t *mldaddr)
{
    struct net_device *dev;
    union iwreq_data wreq;
    u_int8_t *msg;
    union iwreq_data wrqu;
    struct ev_msg evmsg;
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    union iwreq_data staleave_wreq;
    struct ev_sta_leave staleave_ev;
#if ATH_SUPPORT_WAPI
    union iwreq_data wapi_wreq;
    u_int8_t *sta_msg;
    int msg_len;
#endif
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event = NULL;
#endif
#if QCA_SUPPORT_SON
    struct wlan_assoc_disassoc_event_info info;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#endif
    uint32_t no_delsta = (IEEE80211_REASON_NO_DEL_STA & reason);

    reason &= (~IEEE80211_REASON_NO_DEL_STA);
    dev = get_cfg80211_notification_ndev(osifp);

    if (!dev) {
        qdf_err("netdev is NULL");
        return;
    }

    msg = NULL;
    /* fire off wireless event station leaving */
    memset(&wreq, 0, sizeof(wreq));
    IEEE80211_ADDR_COPY(wreq.addr.sa_data, macaddr);
    wreq.addr.sa_family = ARPHRD_ETHER;
    WIRELESS_SEND_EVENT(dev, IWEVEXPIRED, &wreq, NULL);
#if QCA_SUPPORT_SON
    qdf_mem_zero(&info, sizeof(info));
    qdf_mem_copy(info.macaddr, macaddr, QDF_MAC_ADDR_SIZE);
    info.flag = ACTION_DISASSOC;
    info.reason = reason;
    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }
    if (rx_ops && rx_ops->son_rx_ops.deliver_event)
        rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_ASSOC_DISASSOC, &info);
#endif
#if ATH_SUPPORT_WAPI
    sta_msg = wlan_wapi_callback_begin(vap, macaddr, &msg_len, WAPI_STA_AGING);
    if (sta_msg) {
        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_WAPI)) {
            OS_MEMSET(&wapi_wreq, 0, sizeof(wapi_wreq));
            wapi_wreq.data.length = msg_len;
            qdf_info("%s size:%d *", __func__, wreq.data.length);
            wapi_wreq.data.flags = IEEE80211_EV_WAPI;
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wapi_wreq, sta_msg);
        }
        wlan_wapi_callback_end(sta_msg);
    }
    msg = sta_msg;
#endif
#if UMAC_SUPPORT_ACFG
    if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_DISASSOC_AP)) {
        acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
        if (acfg_event) {
            acfg_event->reason = reason;
            IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
            acfg_event->downlink = 1;
            acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
                    WL_EVENT_TYPE_DISASSOC_AP, acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#endif
    if (msg == NULL) {
        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_DISASSOC_IND_AP)) {
            memset(&evmsg, 0, sizeof(evmsg));
            evmsg.reason = reason;
            memset(&wrqu, 0, sizeof(wrqu));
            wrqu.data.flags = IEEE80211_EV_DISASSOC_IND_AP;
            wrqu.data.length = sizeof(evmsg);
            IEEE80211_ADDR_COPY(evmsg.addr, macaddr);
#ifdef WLAN_FEATURE_11BE_MLO
            if (mldaddr && IEEE80211_ADDR_IS_VALID(mldaddr))
                IEEE80211_ADDR_COPY(evmsg.mldaddr, mldaddr);
#endif /*WLAN_FEATURE_11BE_MLO*/
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&evmsg);
        }
#if UMAC_SUPPORT_CFG80211
        if(vap->iv_cfg80211_create && !no_delsta) {
            cfg80211_del_sta(dev, macaddr, GFP_ATOMIC);
        }
#endif
    }

    /* check for smart mesh configuration and send event */
    if (vap->iv_smart_mesh_cfg & SMART_MESH_80211_EVENTS) {

        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_STA_LEAVE)) {
            OS_MEMZERO(&staleave_ev, sizeof(staleave_ev));

            /* populate required fileds */
            OS_MEMCPY(&(staleave_ev.mac_addr), macaddr, QDF_MAC_ADDR_SIZE);
            staleave_ev.channel_num = vap->iv_bsschan->ic_ieee;
            staleave_ev.assoc_id = IEEE80211_AID(associd);
            staleave_ev.reason = reason;

            /* prepare custom event */
            staleave_wreq.data.flags = IEEE80211_EV_STA_LEAVE;
            staleave_wreq.data.length = sizeof(staleave_ev);
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &staleave_wreq, (char *)&staleave_ev);
        }

    }
    return;
}


static void  osif_disassoc_complete_ap(os_handle_t osif, u_int8_t *macaddr,
                                       u_int32_t reason,
                                       IEEE80211_STATUS status,
                                       u_int8_t *mldaddr)
{
    struct net_device *dev;
    union iwreq_data wreq;
#if UMAC_SUPPORT_CFG80211
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
#endif
    struct ev_msg msg;
#if UMAC_SUPPORT_ACFG
	acfg_event_data_t *acfg_event = NULL;
#endif

    dev = get_cfg80211_notification_ndev(osifp);

    if (!dev) {
        qdf_err("netdev is NULL");
        return;
    }
    /* fire off wireless event station leaving */
    memset(&wreq, 0, sizeof(wreq));
    IEEE80211_ADDR_COPY(wreq.addr.sa_data, macaddr);
    wreq.addr.sa_family = ARPHRD_ETHER;
    WIRELESS_SEND_EVENT(dev, IWEVEXPIRED, &wreq, NULL);

    if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                IEEE80211_EV_DISASSOC_COMPLETE_AP)) {
        OS_MEMSET(&msg, 0, sizeof(msg));
        IEEE80211_ADDR_COPY(msg.addr, macaddr);
#ifdef WLAN_FEATURE_11BE_MLO
        if (mldaddr && IEEE80211_ADDR_IS_VALID(mldaddr))
            IEEE80211_ADDR_COPY(msg.mldaddr, mldaddr);
#endif /*WLAN_FEATURE_11BE_MLO*/
        memset(&wreq, 0, sizeof(wreq));
        wreq.data.flags = IEEE80211_EV_DISASSOC_COMPLETE_AP;
        wreq.data.length = sizeof(msg);
        msg.reason=reason;
        msg.status=status;
        WIRELESS_SEND_EVENT(dev,IWEVCUSTOM,&wreq, (char *)&msg);
    }

#if UMAC_SUPPORT_ACFG
    if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_DISASSOC_AP)) {
        acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
        if (acfg_event) {
            acfg_event->result = status;
            acfg_event->reason = reason;
            IEEE80211_ADDR_COPY(acfg_event->addr, macaddr);
            acfg_event->downlink = 0;
            acfg_send_event(dev, ((osif_dev *)osif)->os_handle,
                    WL_EVENT_TYPE_DISASSOC_AP, acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#endif

#if UMAC_SUPPORT_CFG80211
        if(vap->iv_cfg80211_create) {
            cfg80211_del_sta(dev, macaddr, GFP_ATOMIC);
        }
#endif
	return;
}

static void osif_node_authorized_indication_ap(os_handle_t osif, u_int8_t *mac_addr)
{

    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    union iwreq_data wreq;
    struct ieee80211_node *ni = NULL;
    struct ev_node_authorized authorize_ev;

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }

    ni = ieee80211_find_node(vap->iv_ic, mac_addr, WLAN_MLME_SB_ID);
    if (ni) {

        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_STA_AUTHORIZED)) {
            OS_MEMZERO(&authorize_ev, sizeof(authorize_ev));

            /* populate required fileds */
            OS_MEMCPY(&(authorize_ev.mac_addr), ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
            authorize_ev.channel_num =
                wlan_reg_freq_to_chan(ni->ni_ic->ic_pdev_obj,
                        ni->ni_ch_param.ni_freq);
            authorize_ev.assoc_id = IEEE80211_AID(wlan_node_get_associd(ni));
            authorize_ev.phymode = wlan_node_get_mode(ni);
            authorize_ev.nss = wlan_node_get_nss(ni);
            authorize_ev.is_256qam   = wlan_node_get_256qam_support(ni);

            /* prepare custom event */
            wreq.data.flags = IEEE80211_EV_STA_AUTHORIZED;
            wreq.data.length = sizeof(authorize_ev);
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (char *)&authorize_ev);
        }

        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
    }

#if QCA_ENABLE_L2UF
    if (osifp->iv_wdev.iftype == NL80211_IFTYPE_AP) {
        /* Send TGf L2UF frame on behalf of newly associated station */
        osif_deliver_l2uf(osif, mac_addr);

#if DBDC_REPEATER_SUPPORT || UMAC_SUPPORT_SON
        /* To send L2UF on same VAP, to which client connects */
        osif_hardstart_l2uf(osif, mac_addr);
#endif
    }
#endif /* QCA_ENABLE_L2UF */

    return;
}

#if ATH_SUPPORT_WAPI
static void osif_rekey_indication_ap(os_handle_t osif, u_int8_t *macaddr)
{
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    wlan_if_t vap = ((osif_dev *)osif)->os_if;
    union iwreq_data wapi_wreq;
    u_int8_t *sta_msg;
    int msg_len;

    if (!dev ||
        !osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,IEEE80211_EV_WAPI)) {
        return;
    }
    if (IEEE80211_IS_MULTICAST(macaddr))
        sta_msg = wlan_wapi_callback_begin(vap, macaddr, &msg_len, WAPI_MULTI_REKEY);
    else
        sta_msg = wlan_wapi_callback_begin(vap, macaddr, &msg_len, WAPI_UNICAST_REKEY);
    if (sta_msg) {
        OS_MEMSET(&wapi_wreq, 0, sizeof(wapi_wreq));
        wapi_wreq.data.length = msg_len;
        wapi_wreq.data.flags = IEEE80211_EV_WAPI;
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wapi_wreq, sta_msg);
        wlan_wapi_callback_end(sta_msg);
    }
}
#endif

#if ATH_SUPPORT_MGMT_TX_STATUS
/*
 * Handler to provide TX status for MGMT frames
 */
void osif_mgmt_tx_status(os_handle_t osif, IEEE80211_STATUS status, wbuf_t wbuf)
{
    osif_dev *osifp = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    union iwreq_data wreq;
    int wbuf_len =  wbuf_get_pktlen(wbuf);
    char *concat_buf;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

    if (!vap) {
        qdf_err("VAP is null!!!");
        return;
    }

    if (!osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_TX_MGMT)) {
        return;
    }

    concat_buf = OS_MALLOC(osifp->os_handle, wbuf_len + sizeof(IEEE80211_STATUS),
                                GFP_ATOMIC);
    if (!concat_buf) {
        return;
    }

    OS_MEMSET(&wreq, 0, sizeof(wreq));
    wreq.data.flags = IEEE80211_EV_TX_MGMT;
    wreq.data.length = wbuf_len + sizeof(IEEE80211_STATUS);
    *((IEEE80211_STATUS*)concat_buf) = status;
    OS_MEMCPY(concat_buf+sizeof(IEEE80211_STATUS), wbuf_header(wbuf), wbuf_len);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, concat_buf);
    OS_FREE(concat_buf);
}
#endif

#if ATH_PARAMETER_API
static void  osif_papi_event(os_handle_t osif,
                             PAPI_REPORT_TYPE type,
                             uint32_t len, const char *data)
{
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    ath_netlink_papi_event_t  netlink_event;

    if (!dev) {
        qdf_err("netdev is NULL");
        return;
    }
    memset(&netlink_event, 0x0, sizeof(netlink_event));
    netlink_event.type = type;
    netlink_event.sys_index = dev->ifindex;
    if (len && data) {
        OS_MEMCPY(&(netlink_event.data), data, len);
    }

    ath_papi_netlink_send(&netlink_event);
    return;

}
#endif

static void osif_recv_beaconrpt(os_handle_t osif, void *arg, uint32_t len) {
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

#if UMAC_SUPPORT_CFG80211
    if(vap->iv_cfg80211_create) {
        wlan_cfg80211_generic_event(vap->iv_ic,
                                    QCA_NL80211_VENDOR_SUBCMD_FWD_RRM_RPT,
                                    arg,
                                    len,
                                    get_cfg80211_notification_ndev(osifp),
                                    GFP_ATOMIC);
    }
#endif /*UMAC_SUPPORT_CFG80211*/
}

static void osif_recv_bstm(os_handle_t osif, void *arg, uint32_t len) {
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

#if UMAC_SUPPORT_CFG80211
    if(vap->iv_cfg80211_create) {
        wlan_cfg80211_generic_event(vap->iv_ic,
                                    QCA_NL80211_VENDOR_SUBCMD_FWD_BTM_RPT,
                                    arg,
                                    len,
                                    get_cfg80211_notification_ndev(osifp),
                                    GFP_ATOMIC);
    }
#endif /*UMAC_SUPPORT_CFG80211*/
}

static void osif_recv_smpsupdate(os_handle_t osif, void *arg, uint32_t len) {
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

#if UMAC_SUPPORT_CFG80211
    if(vap->iv_cfg80211_create) {
        wlan_cfg80211_generic_event(vap->iv_ic,
                                    QCA_NL80211_VENDOR_SUBCMD_SMPS_UPDATE,
                                    arg,
                                    len,
                                    get_cfg80211_notification_ndev(osifp),
                                    GFP_ATOMIC);
    }
#endif /*UMAC_SUPPORT_CFG80211*/
}

static void osif_recv_opmodeupdate(os_handle_t osif, void *arg, uint32_t len) {
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

#if UMAC_SUPPORT_CFG80211
    if(vap->iv_cfg80211_create) {
        wlan_cfg80211_generic_event(vap->iv_ic,
                                    QCA_NL80211_VENDOR_SUBCMD_OPMODE_UPDATE,
                                    arg,
                                    len,
                                    get_cfg80211_notification_ndev(osifp),
                                    GFP_ATOMIC);
    }
#endif /*UMAC_SUPPORT_CFG80211*/
}
static void osif_recv_awgninterferenceupdate(os_handle_t osif, void *arg, uint32_t len) {
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

#if UMAC_SUPPORT_CFG80211
    if(vap->iv_cfg80211_create) {
        wlan_cfg80211_generic_event(vap->iv_ic,
                                    QCA_NL80211_VENDOR_SUBCMD_AWGN_INTERFERENCE_UPDATE,
                                    arg,
                                    len,
                                    get_cfg80211_notification_ndev(osifp), GFP_ATOMIC);
    }
#endif /*UMAC_SUPPORT_CFG80211*/
}

static QDF_STATUS
dump_channel_switch_time_stats(struct ieee80211vap *vap)
{
    struct channel_switch_stats *stats;

    if (!vap) {
        qdf_err("vap is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (vap->iv_opmode != IEEE80211_M_HOSTAP)
        return QDF_STATUS_SUCCESS;

    stats = &vap->iv_channel_switch_stats;

    mlme_debug("vdev ID: %u Name: %s psoc id %u", wlan_vdev_get_id(vap->vdev_obj),
               vap->iv_netdev_name,
               wlan_psoc_get_id(wlan_vdev_get_psoc(vap->vdev_obj)));
    mlme_debug("Channel switch time : %llu us",
               stats->vdev_up - stats->command_handler_entry);
    mlme_debug("Command handler entry : %llu(0) us",
               stats->command_handler_entry);
    if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap)) {
        mlme_debug("MVR command Tx : %llu(%llu) us",
                   stats->mvr_command_tx,
                   stats->mvr_command_tx - stats->command_handler_entry);
    } else {
        mlme_debug("Vdev Stop command Tx : %llu(%llu) us",
                   stats->stop_command_tx,
                   stats->stop_command_tx - stats->command_handler_entry);
        mlme_debug("Vdev Stop resp Rx : %llu(%llu) us",
                   stats->stop_response_rx,
                   stats->stop_response_rx - stats->command_handler_entry);
        mlme_debug("Block for vdev down: %llu(%llu) us",
                   stats->block_for_vdev_down,
                   stats->block_for_vdev_down - stats->command_handler_entry);
        mlme_debug("Vdev Start command Tx : %llu(%llu) us",
                   stats->start_command_tx,
                   stats->start_command_tx - stats->command_handler_entry);
    }
    if (stats->block_for_vdev_up)
        mlme_debug("Block for vdev up: %llu(%llu) us",
                   stats->block_for_vdev_up,
                   stats->block_for_vdev_up - stats->command_handler_entry);
    if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap)) {
        mlme_debug("MVR resp Rx : %llu(%llu) us",
                   stats->mvr_response_rx,
                   stats->mvr_response_rx - stats->command_handler_entry);
    } else {
        mlme_debug("Vdev Start resp Rx : %llu(%llu) us",
                    stats->start_response_rx,
                    stats->start_response_rx - stats->command_handler_entry);
    }
    mlme_debug("Vdev up : %llu(%llu) us",
               stats->vdev_up,
               stats->vdev_up - stats->command_handler_entry);
    if (stats->command_handler_exit)
        mlme_debug("Command handler exit : %llu(%llu) us",
                   stats->command_handler_exit,
                   stats->command_handler_exit - stats->command_handler_entry);

    return QDF_STATUS_SUCCESS;
}

static void osif_vdev_state_up_active_indication(os_handle_t osif)
{
    osif_dev *osdev = (osif_dev *) osif;
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    struct net_device *ic_netdev = ic->ic_netdev;
    osdev_t ic_osdev = ic->ic_osdev;
    QDF_STATUS status;
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event;
    wlan_chan_t chan = ic->ic_curchan;
    u_int16_t length = 0;
#endif /* UMAC_SUPPORT_ACFG */

    if (!ic_netdev || !ic_osdev) {
        return;
    }

    vap->iv_channel_switch_stats.vdev_up = qdf_get_log_timestamp();

    status = dump_channel_switch_time_stats(vap);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Failed to dump channel switch stats");
        return;
    }

    /* Send ACFG event for special vap mode only */
    if (!IEEE80211_IS_SPECIAL_VAP_ONLY(vap))
        return;

#if UMAC_SUPPORT_ACFG
    if (!osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_SCAN_RADIO_TUNING_DONE)) {
        return;
    }
    acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
    if (!acfg_event)
        return;

    memset(acfg_event, 0, sizeof(acfg_event_data_t));

    if (!IEEE80211_IS_CHAN_VALID(chan))
        acfg_event->freqs[0] = 0;
    else
        acfg_event->freqs[0] = wlan_channel_frequency(chan);

    ieee80211_ucfg_get_phymode(vap, acfg_event->cur_phymode_str, &length, CURR_MODE);

    /* Send event on a radio level */
    acfg_send_event(ic_netdev, ic_osdev, WL_EVENT_TYPE_SCAN_RADIO_TUNING_DONE,
                    acfg_event);

    qdf_mem_free(acfg_event);
#endif /* UMAC_SUPPORT_ACFG */
}

/*
 * handler called when the AP vap comes up asynchronously.
 * (happens only when resource  manager is present).
 */
static void  osif_create_infra_complete( os_handle_t osif, IEEE80211_STATUS status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;
    struct net_device *dev;

    dev = OSIF_TO_NETDEV(osifp);

	if(osifp->is_delete_in_progress)
		return;

    if(status == IEEE80211_STATUS_SUCCESS) {
        wlan_mlme_connection_up(vap);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :vap-%d up\n", __func__, vap->iv_unit);
        wlan_restore_vap_params(vap);
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :vap-%d down, status=%d\n", __func__, vap->iv_unit, status);
        dev->flags &= ~IFF_RUNNING;
    }

}

/*
 * osif_cancel_acs_and_start_ap():
 * Cancel ACS for the given AP vap and report the STA VAP's iv_bsschan
 * back to hostapd.
 */
void osif_cancel_acs_and_start_ap(void *arg, struct ieee80211vap *vap)
{
    osif_dev *osifp = NULL;
    struct ieee80211vap *sta_vap = (struct ieee80211vap *)arg;
    struct wlan_mbss_ev_data mbss_ev = {0};

    if ((sta_vap == NULL) || (wlan_vap_get_opmode(sta_vap) != IEEE80211_M_STA)) {
        return;
    }

    if (!IEEE80211_IS_CHAN_VALID(sta_vap->iv_bsschan)) {
        return;
    }

    osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    if (osifp == NULL) {
        return;
    }

    /* Check if cfg ACS was running */
    if (wlan_mbss_vdev_acs_in_progress(vap->vdev_obj, MBSS_ACS_SRC_CFG))
        wlan_cfg80211_acs_report_channel(vap, sta_vap->iv_bsschan);

    wlan_ucfg_scan_cancel(vap, osifp->scan_requestor, 0,
                          WLAN_SCAN_CANCEL_VDEV_ALL, false);

    mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_ACS;
    wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

    mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_HT40;
    wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

    return;
}

/*
 * osif_cmd_flush_cb(): cb function from scheduler context
 * @msg: scheduler msg data
 *
 * This cb will be called when a msg is being flushed from the scheduler queue
 *
 * Return: Success
 */
QDF_STATUS osif_cmd_flush_cb(struct scheduler_msg *msg)
{
    struct osif_cmd_sched_data *req = msg->bodyptr;

    wlan_objmgr_vdev_release_ref(req->vdev, WLAN_SCHEDULER_ID);
    qdf_mem_free(req);
    return QDF_STATUS_SUCCESS;
}

/*
 * osif_cmd_sched_cb(): cb function from scheduler context
 * @msg: scheduler msg data
 *
 * This is the cb from the scheduler context where MBSSID scenario
 * related vdev cmds are processed
 *
 * Return: Success if vdev cmd is processed else error.
 */
QDF_STATUS osif_cmd_sched_cb(struct scheduler_msg *msg)
{
    struct osif_cmd_sched_data *req = msg->bodyptr;
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(req->vdev);
    wlan_if_t tmpvap;
    wlan_dev_t comhandle;

    if (!vap) {
        qdf_err("NULL vap");
        wlan_objmgr_vdev_release_ref(req->vdev, WLAN_SCHEDULER_ID);
        qdf_mem_free(req);
        return QDF_STATUS_SUCCESS;
    }
    comhandle = vap->iv_ic;

    switch (req->cmd_type) {
    case OSIF_CMD_PDEV_START:
        wlan_mbss_start_restart_ap_monitor_vdevs(wlan_vdev_get_pdev(req->vdev),
                                                 NULL);
        break;
    case OSIF_CMD_PDEV_STOP:
        wlan_mbss_stop_ap_monitor_vdevs(wlan_vdev_get_pdev(req->vdev), NULL);
        break;
    case OSIF_CMD_PDEV_STOP_START:
        wlan_mbss_stop_start_ap_vdevs(wlan_vdev_get_pdev(req->vdev), NULL);
        break;
    case OSIF_CMD_OTH_VDEV_STOP_START:
        TAILQ_FOREACH(tmpvap, &comhandle->ic_vaps, iv_next) {
            struct net_device *tmpvap_netdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
            if ((tmpvap != vap) && IS_UP(tmpvap_netdev) &&
                    wlan_vdev_chan_config_valid(tmpvap->vdev_obj) == QDF_STATUS_SUCCESS) {
                osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpvap_netdev, RESCAN);
            }
        }
        break;
    case OSIF_CMD_VDEV_START:
        wlan_mlme_start_vdev(req->vdev, 0, WLAN_MLME_NOTIFY_NONE);
        break;
    case OSIF_CMD_VDEV_STOP:
        if (vap->iv_opmode == IEEE80211_M_STA)
            wlan_mlme_cm_stop(vap->vdev_obj,  CM_OSIF_DISCONNECT,
                          REASON_STA_LEAVING, false);
        else
            wlan_mlme_stop_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
        break;
    case OSIF_CMD_STA_VAP_STOP:
        wlan_mbss_stop_sta_vdevs(wlan_vdev_get_pdev(vap->vdev_obj), NULL);
        break;
    case OSIF_CMD_STA_VAP_STOP_START:
        wlan_mbss_start_sta_vdevs(wlan_vdev_get_pdev(vap->vdev_obj), NULL);
        break;

    default:
        qdf_err("Unknown cmd type");
    }

    wlan_objmgr_vdev_release_ref(req->vdev, WLAN_SCHEDULER_ID);
    qdf_mem_free(req);
    return QDF_STATUS_SUCCESS;
}

/*
 * osif_cmd_schedule_req(): Form a scheduler msg to be posted for
 * vdev operations in MBSSID scenario
 * @vdev: Object manager vdev
 * @cmd_type: Serialization command type
 *
 * Return: Success if the request is posted to the scheduler, else error
 */
QDF_STATUS osif_cmd_schedule_req(struct wlan_objmgr_vdev *vdev,
        enum osif_cmd_type cmd_type)
{
    struct scheduler_msg msg = {0};
    struct osif_cmd_sched_data *req;
    QDF_STATUS ret = QDF_STATUS_E_FAILURE;

    if (!vdev)
        return QDF_STATUS_E_INVAL;

    req = qdf_mem_malloc(sizeof(*req));
    if (!req) {
        qdf_err("req is NULL");
        return ret;
    }

    ret = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SCHEDULER_ID);
    if (QDF_IS_STATUS_ERROR(ret)) {
        qdf_err("unable to get reference");
        goto sched_error;
    }

    req->vdev = vdev;
    req->cmd_type = cmd_type;

    msg.bodyptr = req;
    msg.callback = osif_cmd_sched_cb;
    msg.flush_callback = osif_cmd_flush_cb;

    ret = scheduler_post_message(QDF_MODULE_ID_OS_IF,
            QDF_MODULE_ID_OS_IF, QDF_MODULE_ID_OS_IF, &msg);
    if (QDF_IS_STATUS_ERROR(ret)) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
        qdf_err("failed to post scheduler_msg");
        goto sched_error;
    }
    return ret;

sched_error:
    qdf_mem_free(req);
    return ret;
}

static void osif_bringup_pending_ap_vaps(struct wlan_objmgr_vdev *vdev,
                                         void *arg)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    pdev = wlan_vdev_get_pdev(vdev);

    /* Skip bring-up of AP vap which has STA bridging vap in Depedent Repeater */
    if (pdev && mlo_is_sta_bridge_vdev(vdev) && (!wlan_rptr_pdev_is_eir(pdev))) {
        wlan_mbss_sched_start_stop(vdev, MBSS_SCHED_BRIDGE_VDEVS_START);
        return;
    }
    wlan_mbss_sched_start_stop(vdev, MBSS_SCHED_PDEV_START);
}

#ifdef WLAN_FEATURE_11BE_MLO
static void bringup_standalone_ap_vdev(struct wlan_objmgr_vdev *vdev,
                                       void *arg)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_vdev *sta_vdev = NULL;

    pdev = wlan_vdev_get_pdev(vdev);
    sta_vdev = wlan_rptr_pdev_get_stavdev(pdev);
    if (!sta_vdev)
        wlan_mbss_standalone_sched_start_stop(vdev,
                                          MBSS_SCHED_STANDALONE_AP_VDEVS_START);
}

static inline void osif_bringup_ml_ap_vaps(struct wlan_objmgr_vdev *vdev)
{
    mlo_iterate_connected_vdev_list(vdev,
                                    osif_bringup_pending_ap_vaps,
                                    NULL);

    mlo_iterate_ml_standalone_vdev_list(vdev,
                                        bringup_standalone_ap_vdev, NULL);
}
#else
static inline void osif_bringup_ml_ap_vaps(struct wlan_objmgr_vdev *vdev)
{ }
#endif

void osif_check_pending_mlsta_ap_vaps(wlan_dev_t comhandle,
                                      struct wlan_objmgr_vdev *vdev)
{
    if (!vdev)
        return;

    if (wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        osif_bringup_ml_ap_vaps(vdev);
     } else {
        wlan_mbss_sched_start_stop(vdev, MBSS_SCHED_PDEV_START);
     }
}

static void
osif_bringdown_pending_ap_vaps(struct wlan_objmgr_vdev *vdev,
                               void *arg)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    pdev = wlan_vdev_get_pdev(vdev);

    /* Skip bring-down of AP vap which has STA bridging vap in Depedent Repeater */
    if (pdev && mlo_is_sta_bridge_vdev(vdev) && (!wlan_rptr_pdev_is_eir(pdev))) {
        return;
    }
    wlan_mbss_sched_start_stop(vdev, MBSS_SCHED_PDEV_STOP);
}

#ifdef WLAN_FEATURE_11BE_MLO
static void bringdown_standalone_ap_vdev(struct wlan_objmgr_vdev *vdev,
                                       void *arg)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_vdev *sta_vdev = NULL;

    pdev = wlan_vdev_get_pdev(vdev);
    sta_vdev = wlan_rptr_pdev_get_stavdev(pdev);
    if (!sta_vdev)
        wlan_mbss_standalone_sched_start_stop(vdev,
                                          MBSS_SCHED_STANDALONE_AP_VDEVS_STOP);
}

static void osif_bringdown_ml_ap_vaps(struct wlan_objmgr_vdev *vdev)
{
    mlo_iterate_ml_vdev_list(vdev,
                             osif_bringdown_pending_ap_vaps,
                             NULL, false);

    mlo_iterate_ml_standalone_vdev_list(vdev,
                                        bringdown_standalone_ap_vdev, NULL);
}
#else
static void osif_bringdown_ml_ap_vaps(struct wlan_objmgr_vdev *vdev)
{ }
#endif

void osif_bring_down_mlsta_ap_vaps(wlan_dev_t comhandle,
                                   struct wlan_objmgr_vdev *vdev)
{
    if (!vdev)
        return;

    if (wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        osif_bringdown_ml_ap_vaps(vdev);
     } else {
        wlan_mbss_sched_start_stop(vdev, MBSS_SCHED_PDEV_STOP);
     }

}

static void osif_conn_sm_up_ev_hdl(os_if_t osif)
{
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap = osdev->os_if;
    wlan_dev_t ic = vap->iv_ic;
    union iwreq_data wreq;
    u_int8_t bssid[QDF_MAC_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    enum ieee80211_phymode des_mode = wlan_get_desired_phymode(vap);
#if REPEATER_SAME_SSID
    int ret;
#endif

    qdf_mem_set(&wreq, sizeof(wreq), 0);
    wlan_vap_get_bssid(vap, bssid);

#if REPEATER_SAME_SSID
    if (wlan_rptr_global_is_s_ssid() && (vap == ic->ic_sta_vap)) {
        ret = wlan_rptr_validate_stavap_connection(vap->vdev_obj, bssid);
        if (ret != QDF_STATUS_SUCCESS) {
            osif_cmd_schedule_req(vap->vdev_obj, OSIF_CMD_VDEV_STOP);
            return;
        }
    }
#endif

    wlan_mlme_connection_up(vap);

    if (wlan_rptr_pdev_is_eir(ic->ic_pdev_obj))
        wlan_determine_cw(vap, ic->ic_curchan);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :vap up ,connection up"
            " to bssid=%s, ifname=%s\n",
            __func__, ether_sprintf(bssid), dev->name);
    _netif_carrier_on(dev);

    if (!vap->iv_cfg80211_create)
        osif_send_assoc_resp_ies(osif);

    IEEE80211_ADDR_COPY(wreq.addr.sa_data, bssid);
    wreq.addr.sa_family = ARPHRD_ETHER;
    WIRELESS_SEND_EVENT(dev, SIOCGIWAP, &wreq, NULL);
    /*
     * This flag is set to false during first association. This flag is used
     * in repeater mode and if this flag is set to flase, AP vaps are not brought
     * down from scan_ev_handler.
     */
    ieee80211_vap_reset_ap_vaps_clear(vap);

#if UMAC_SUPPORT_CFG80211
    if (vap->iv_cfg80211_create)
        ieee80211_cfg80211_send_connect_result(osdev, bssid, IEEE80211_STATUS_SUCCESS);
#endif
    /* Fix for evid 97581 - Enhancement to support independent VAP in multi mode*/
    /* No check required on pending AP VAPs for repeater move trigger by cloud */
    if ((!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) &&
            !(wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_IN_PROGRESS)) {

#if DBDC_REPEATER_SUPPORT
        if (ieee80211_ic_rpt_max_phy_is_set(ic)) {
            qdf_debug("[rpt_max_phy]: delay bringing ap vaps up till node authorize");
        } else if (!wlan_rptr_global_is_g_wds() &&
                   ucfg_mlo_is_mld_connected(vap->vdev_obj)) {
            osif_check_pending_mlsta_ap_vaps(ic, vap->vdev_obj);
        }
#endif

#ifdef ATHEROS_LINUX_PERIODIC_SCAN
        /* 11AX: Recheck future 802.11ax drafts (>D1.0) on coex rules
         */
        if ((wlan_vap_get_opmode(vap) == IEEE80211_M_STA ) && (vap->iv_bss->ni_chwidth == IEEE80211_CWM_WIDTH40) &&
            ieee80211_is_phymode_g40(des_mode)) {
            if (!(ic->ic_flags & IEEE80211_F_COEXT_DISABLE)) {
                if (!osdev->os_periodic_scan_period)
                    osdev->os_periodic_scan_period = OSIF_PERIODICSCAN_COEXT_PERIOD;
                osdev->os_scan_band = OSIF_SCAN_BAND_2G_ONLY;
                osif_periodic_scan_start(osif);
            } else {
                    osdev->os_periodic_scan_period = OSIF_PERIODICSCAN_DEF_PERIOD;
            }
        }
#endif
    }

    /* Update repeater move state to STOP once connection is up */
    if (wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_IN_PROGRESS)
        wlan_rptr_move_set_state(ic->ic_pdev_obj, WLAN_REPEATER_MOVE_STOP);
}

static void osif_conn_sm_down_ev_hdl(os_if_t osif,
        wlan_connection_sm_event *smevent)
{
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap = osdev->os_if;
    wlan_dev_t ic = vap->iv_ic;
    union iwreq_data wreq;
    u_int8_t bssid[QDF_MAC_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    uint8_t auto_assoc = 0;
    int is_ap_cac_timer_running = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    struct vdev_mlme_obj *vdev_mlme;
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :sta disconnected \n", __func__);

#ifdef WLAN_FEATURE_11BE_MLO
    vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vap->vdev_obj);
    if (!vdev_mlme) {
        qdf_err("Vdev mlme component object is NULL");
        return;
    }
    vdev_mlme->ml_reconfig_started = false;
#endif
#if UMAC_SUPPORT_CFG80211
    auto_assoc = (vap->auto_assoc && !(vap->iv_cfg80211_create));
#else
    auto_assoc = vap->auto_assoc;
#endif

    if (smevent->disconnect_reason == WLAN_CONNECTION_SM_ASSOC_REJECT) {
#if UMAC_SUPPORT_CFG80211
        if(vap->iv_cfg80211_create)
            ieee80211_cfg80211_send_connect_result(osdev, bssid, smevent->reasoncode);
#endif
    } else if (((smevent->disconnect_reason == WLAN_CONNECTION_SM_RECONNECT_TIMEDOUT) ||
                (smevent->disconnect_reason == WLAN_CONNECTION_SM_CONNECT_TIMEDOUT)) &&
            (auto_assoc || (osdev->authmode == IEEE80211_AUTH_AUTO))) {

        _netif_carrier_off(dev);
        /* Connection timed out - retry connection */
#if UMAC_SUPPORT_CFG80211
        if(vap->iv_cfg80211_create) {
            if (vap->iv_sta_external_auth_enabled) {
                ieee80211_cfg80211_send_connect_result(osdev, bssid, IEEE80211_STATUS_TIMEOUT);
                return;
            }
            ieee80211_cfg80211_disconnected(vap, dev, WLAN_REASON_UNSPECIFIED);
        }
#endif

        if (osdev->authmode == IEEE80211_AUTH_AUTO) {
            /* If the auth mode is set to AUTO
             * switch between SHARED and AUTO mode
             */
            if (wlan_crypto_vdev_has_auth_mode(vap->vdev_obj, ((1 << WLAN_CRYPTO_AUTH_SHARED)))) {
                /*
                 *fix for CR#586870; The roaming should be set to
                 *AUTO so that the state machine is restarted
                 *for the connection to happen in OPEN mode.
                 */
                ieee80211com_set_roaming(ic, IEEE80211_ROAMING_AUTO);
                wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE, 1 << WLAN_CRYPTO_AUTH_OPEN);
            } else {
                wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE, 1 << WLAN_CRYPTO_AUTH_SHARED);
            }
        }

#if DBDC_REPEATER_SUPPORT
        /* Fix for evid 97581 - Enhancement to support independent VAP in multi mode*/
        /* Do not bring down VAPs for repeater move trigger by cloud */
        if ((!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) &&
                !(wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_IN_PROGRESS)) {
            /* Note: This should be moved to resource manager later */
            if (!wlan_rptr_global_is_g_wds() &&
                ucfg_mlo_is_mld_disconnected(vap->vdev_obj))
                osif_bring_down_mlsta_ap_vaps(ic, vap->vdev_obj);
        }
#endif

        ucfg_dfs_is_ap_cac_timer_running(ic->ic_pdev_obj, &is_ap_cac_timer_running);
        if((ic->ic_roaming != IEEE80211_ROAMING_MANUAL) && (!is_ap_cac_timer_running))
            osif_cmd_schedule_req(vap->vdev_obj, OSIF_CMD_VDEV_START);
    } else {
        /* Fix for evid 97581 - Enhancement to support independent VAP in multi mode*/
        /* Do not bring down VAPs for repeater move trigger by cloud */
        if ( (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) &&
                !(wlan_rptr_move_get_state(ic->ic_pdev_obj) == WLAN_REPEATER_MOVE_IN_PROGRESS)) {
            /* Note: This should be moved to resource manager later */
#if DBDC_REPEATER_SUPPORT
            if (!wlan_rptr_global_is_g_wds() &&
                ucfg_mlo_is_mld_disconnected(vap->vdev_obj)) {
                osif_bring_down_mlsta_ap_vaps(ic, vap->vdev_obj);
            }
#endif
        }
#ifdef ATHEROS_LINUX_PERIODIC_SCAN
        else {
            osif_periodic_scan_stop(osif);
        }
#endif
        /* bring down the vap only if it is not a restart */
        if (!osdev->is_restart) {
            memset(wreq.ap_addr.sa_data, 0, QDF_MAC_ADDR_SIZE);
            wreq.ap_addr.sa_family = ARPHRD_ETHER;
            WIRELESS_SEND_EVENT(dev, SIOCGIWAP, &wreq, NULL);
#if UMAC_SUPPORT_CFG80211
                /* Adding !vap->iv_roam_inprogress check for roaming to not to forward the
                   disconnect event to supplicant so that supplicant will not clear the
                   PMKID and it will be re-used in the reassociation with different AP.
                 */
                if(vap->iv_cfg80211_create && !vap->iv_roam_inprogress) {
                    ucfg_scan_flush_results(vap->iv_ic->ic_pdev_obj, NULL);
                    ieee80211_cfg80211_disconnected(vap, dev, WLAN_REASON_UNSPECIFIED);
               }
#endif
                _netif_carrier_off(dev);
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
            wlan_mlme_stacac_restore_defaults(vap);
            wlan_mlme_clear_mlme_req(vap);
#endif
            if (osdev->is_delete_in_progress) {
                OS_DELAY(5000);
                delete_default_vap_keys(vap);
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : Keys Deleted \n",__func__);
            }
        } else {
            osdev->is_restart = 0;
#if UMAC_SUPPORT_CFG80211
            if(vap->iv_cfg80211_create)
                ieee80211_cfg80211_disconnected(vap, dev, WLAN_REASON_UNSPECIFIED);
#endif
            _netif_carrier_off(dev);
        }
    }
}

static void osif_conn_sm_lost_ev_hdlr(os_if_t osif)
{
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap = osdev->os_if;
    wlan_dev_t ic = vap->iv_ic;
    union iwreq_data wreq;

    qdf_mem_set(&wreq, sizeof(wreq), 0);
    /* Fix for evid 97581 - Enhancement to support independent VAP in multi mode*/
    if (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {
        /* Note: This should be moved to resource manager later */
        if (!wlan_rptr_global_is_g_wds() &&
            ucfg_mlo_is_mld_disconnected(vap->vdev_obj)) {
            osif_bring_down_mlsta_ap_vaps(ic, vap->vdev_obj);
        }
    }
#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    else {
        osif_periodic_scan_stop(osif);
    }
#endif

#ifdef __linux__
    if (dev &&
        (wlan_vdev_is_up(vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
        qdf_mem_set(wreq.ap_addr.sa_data, QDF_MAC_ADDR_SIZE, 0);
        wreq.ap_addr.sa_family = ARPHRD_ETHER;
        WIRELESS_SEND_EVENT(dev, SIOCGIWAP, &wreq, NULL);
    }
#endif
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :connection lost \n", __func__);

}

static wlan_connection_sm_event_disconnect_reason
osif_cm_conn_to_sm_reason(enum wlan_cm_connect_fail_reason reason)
{
    switch (reason) {
    case CM_ASSOC_FAILED:
        return WLAN_CONNECTION_SM_ASSOC_REJECT;
        break;
    default:
        return WLAN_CONNECTION_SM_CONNECT_TIMEDOUT;
        break;
    }
}

static QDF_STATUS osif_cm_connect_complete(struct wlan_objmgr_vdev *vdev,
                                           struct wlan_cm_connect_resp *rsp,
                                           enum osif_cb_type type)
{
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);
    os_if_t osif;
    wlan_connection_sm_event smevent;
#ifdef IPA_OFFLOAD
    enum wlan_ipa_wlan_event wlan_event;
    struct wlan_objmgr_psoc *psoc = NULL;
#endif
    osif_dev *osdev;
    struct net_device *dev;

    if (!vap || !vap->iv_ifp)
        return QDF_STATUS_E_FAILURE;
    osif = vap->iv_ifp;

#ifdef IPA_OFFLOAD
    if (ucfg_ipa_is_enabled() && (rsp->connect_status == QDF_STATUS_SUCCESS)) {
        if ((type == OSIF_PRE_USERSPACE_UPDATE) &&
                (vap->iv_bss->ni_authmode == IEEE80211_AUTH_OPEN)) {
                wlan_event = WLAN_IPA_STA_CONNECT;
                psoc = wlan_vdev_get_psoc(vap->vdev_obj);
                if (!psoc)
                        return QDF_STATUS_E_FAILURE;
                ucfg_ipa_event_wq(psoc, vap->iv_bss->ni_macaddr, vdev,
                                  wlan_event);
        }
    }
#endif

    if (type != OSIF_POST_USERSPACE_UPDATE)
        return QDF_STATUS_SUCCESS;

    if (QDF_IS_STATUS_SUCCESS(rsp->connect_status)) {
        /*
         * Connect success handler similar to connection_up
         * ASSOC_SUCCESS
         */
        osif_mlme_notify_handler(vap, WLAN_SER_CMD_VDEV_CONNECT);
        osif_conn_sm_up_ev_hdl(osif);

        osdev = (osif_dev *)osif;
        dev = get_cfg80211_notification_ndev(osdev);
        if (dev)
            dev->flags |= IFF_RUNNING;
    } else {
        /*
         * Connect failure handler similar to connection_lost
         * NO CANDIDATE
         * JOIN_FAILED
         * AUTH_FAILED
         * AUTH_TIMEOUT
         * ASSOC_TIMEOUT
         * ASSOC_FAILED
         */
        osif_mlme_notify_handler(vap, WLAN_SER_CMD_VDEV_DISCONNECT);
        osif_conn_sm_lost_ev_hdlr(osif);

        /* Connect failure will also post down ev */
        smevent.reasoncode = rsp->status_code;
        smevent.disconnect_reason = osif_cm_conn_to_sm_reason(rsp->reason);
        osif_conn_sm_down_ev_hdl(osif, &smevent);
    }

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS osif_cm_disconnect_complete(struct wlan_objmgr_vdev *vdev,
                                              struct wlan_cm_discon_rsp *rsp,
                                              enum osif_cb_type type)
{
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);
    os_if_t osif;
    wlan_connection_sm_event smevent;
#ifdef IPA_OFFLOAD
    enum wlan_ipa_wlan_event wlan_event;
    struct wlan_objmgr_psoc *psoc = NULL;
#endif

    if (!vap || !vap->iv_ifp)
        return QDF_STATUS_E_FAILURE;
    osif = vap->iv_ifp;

#ifdef IPA_OFFLOAD
    if (ucfg_ipa_is_enabled()) {
            if (type == OSIF_PRE_USERSPACE_UPDATE) {
                    wlan_event = WLAN_IPA_STA_DISCONNECT;
                    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
                    if (!psoc)
                            return QDF_STATUS_E_FAILURE;
                    ucfg_ipa_event_wq(psoc, vdev->vdev_nif.osdev->wdev->netdev->dev_addr,
                                      vdev, wlan_event);
            }
    }
#endif
    if (type != OSIF_POST_USERSPACE_UPDATE)
        return QDF_STATUS_SUCCESS;

    /*
     * Disconnect and connect failure handler similar to connection_down
     * OSIF_DISCONNECT
     * STA_KICKOUT
     * PEER_DISCONNECT
     */
    osif_mlme_notify_handler(vap, WLAN_SER_CMD_VDEV_DISCONNECT);

    smevent.reasoncode = rsp->req.req.reason_code;
    smevent.disconnect_reason = WLAN_CONNECTION_SM_DISCONNECT_REQUEST;
    osif_conn_sm_down_ev_hdl(osif, &smevent);

    return QDF_STATUS_SUCCESS;
}

struct osif_cm_ops osif_ops = {
    .connect_complete_cb = osif_cm_connect_complete,
    .disconnect_complete_cb = osif_cm_disconnect_complete,
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    .osif_get_mld_netdev_cb = osif_vdev_get_mld_netdev,
#endif
};

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
QDF_STATUS
osif_scan_get_status_on_link_vifs(struct net_device *netdev,
                                  struct cfg80211_scan_request **req,
                                  struct pdev_osif_priv *osif_priv)
{
    struct osif_mldev *osifp_mldev;
    uint8_t i;

    osifp_mldev = osif_mld_get_mldev(netdev);
    if (osifp_mldev->dev_type == OSIF_NETDEV_TYPE_MLO) {
        for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
             if (osifp_mldev->request_copy[i])
                 return QDF_STATUS_E_PENDING;
        }
        *req = osifp_mldev->mld_scan_req;
    }
    return QDF_STATUS_SUCCESS;
}

void osif_scan_update_scan_status(struct net_device *netdev,
                                  struct cfg80211_scan_request **req,
                                  struct pdev_osif_priv *osif_priv,
                                  bool suspend)
{
    struct osif_mldev *osifp_mldev;
    uint8_t i, j;

    if (!netdev) {
        return;
    }

    osifp_mldev = osif_mld_get_mldev(netdev);
    if (!osifp_mldev)
        return;

    if (osifp_mldev->dev_type == OSIF_NETDEV_TYPE_MLO) {
        for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
             if (osifp_mldev->request_copy[i] && (osifp_mldev->request_copy[i] == *req)) {
                 if (suspend)
                     qdf_debug("Scan suspended on link %d", i);

                 for (j = 0; j < (*req)->n_channels; j++) {
                      if ((*req)->channels[j]) {
                          qdf_mem_free((*req)->channels[j]);
                      }
                 }
                 qdf_mem_free(osifp_mldev->request_copy[i]);
                 osifp_mldev->request_copy[i] = *req = NULL;
             }
        }
    }
}

struct osif_scan_ops osif_scan_ops = {
    .get_scan_status = osif_scan_get_status_on_link_vifs,
    .update_scan_status = osif_scan_update_scan_status,
};

static void osif_scan_register_cb(void)
{
    osif_scan_set_ops(&osif_scan_ops);
}
#else
static void osif_scan_register_cb(void)
{
}
#endif

static QDF_STATUS osif_cm_register_legacy_cb(void)
{
    osif_cm_set_legacy_cb(&osif_ops);
    return osif_cm_register_cb();
}

void
osif_replay_failure_indication(os_handle_t osif, const u_int8_t *frm, u_int keyix)
{
    static const char * tag = "MLME-REPLAYFAILURE.indication";
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap = osdev->os_if;
    struct ieee80211_frame *wh = (struct ieee80211_frame *)frm;
    union iwreq_data wrqu;
    char buf[128];

    if (!dev) {
        qdf_err("netdev is NULL for this vap type");
        return;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO,
        "%s: replay detected <keyix %d>\n",
        ether_sprintf(wh->i_addr2), keyix);

    /* TODO: needed parameters: count, keyid, key type, src address, TSC */
    snprintf(buf, sizeof(buf), "%s(keyid=%d %scast addr=%s)", tag,
        keyix, IEEE80211_IS_MULTICAST(wh->i_addr1) ?  "broad" : "uni",
        ether_sprintf(wh->i_addr1));
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = strlen(buf);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);
}
/*Event trigger when receive one packet with MIC error*/
void
osif_michael_failure_indication(os_handle_t osif, const uint8_t *ta_mac_addr,
        u_int keyix)
{
    static const char *tag = "MLME-MICHAELMICFAILURE.indication";
    struct net_device *dev;
    osif_dev *osdev = (osif_dev *)osif;
    wlan_if_t vap;
    union iwreq_data wrqu;
    char buf[128];
    struct ev_msg msg;

    vap = osdev->os_if;

    if (!vap) {
        qdf_err("vap is NULL");
        return;
    }

    dev = get_cfg80211_notification_ndev(osdev);

    if (!dev) {
        qdf_err("netdev is NULL");
        return;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO,
        "%s: Michael MIC verification failed <keyix %d>",
        ether_sprintf(ta_mac_addr), keyix);

#if UMAC_SUPPORT_CFG80211
    if(vap && vap->iv_cfg80211_create) {
       cfg80211_michael_mic_failure(dev, ta_mac_addr, NL80211_KEYTYPE_PAIRWISE,
                                    keyix, NULL, GFP_ATOMIC);
    }
    else
#endif
    {
       /* TODO: needed parameters: count, keyid, key type, src address, TSC */
       snprintf(buf, sizeof(buf), "%s(keyid=%d addr=%s)", tag,
                keyix, ether_sprintf(ta_mac_addr));
       memset(&wrqu, 0, sizeof(wrqu));
       wrqu.data.length = strlen(buf);
       wrqu.data.flags = 0;            /*hostapd use this event and expects this flags to be 0.*/
       WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);

       if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                   IEEE80211_EV_MIC_ERR_IND_AP)) {
           IEEE80211_ADDR_COPY(msg.addr, ta_mac_addr);
           memset(&wrqu, 0, sizeof(wrqu));
           wrqu.data.flags = IEEE80211_EV_MIC_ERR_IND_AP;    /*Sends two event for backward support.*/
           wrqu.data.length = sizeof(msg);
           WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu,(char*)&msg);
       }
    }
}
/*Event trigger when WPA unicast key setting*/
void
osif_keyset_done_indication(os_handle_t osif, u_int8_t *macaddr, u_int8_t status)
{
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    union iwreq_data wrqu;
    struct ev_msg msg;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

    if (!vap) {
        qdf_err("vap is null!!!");
        return;
    }

    if (!dev ||
        !osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                IEEE80211_EV_KEYSET_DONE_IND_AP)) {
        return;
    }

    IEEE80211_ADDR_COPY(msg.addr, macaddr);
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.flags = IEEE80211_EV_KEYSET_DONE_IND_AP;
    wrqu.data.length = sizeof(msg);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
}
/*Event trigger when received AUTH request from one STA and this STA is in the blacklist */
void
osif_blklst_sta_auth_indication(os_handle_t osif, u_int8_t *macaddr, u_int8_t status)
{
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    union iwreq_data wrqu;
    struct ev_msg msg;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;

    if (!vap) {
        qdf_err("vap is null!!!");
        return;
    }

    if (!dev ||
        !osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                IEEE80211_EV_BLKLST_STA_AUTH_IND_AP)) {
        return;
    }
    IEEE80211_ADDR_COPY(msg.addr, macaddr);
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.flags = IEEE80211_EV_BLKLST_STA_AUTH_IND_AP;
    wrqu.data.length = sizeof(msg);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, (char *)&msg);
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
static void osif_sta_cac_started (os_handle_t osif, u_int32_t frequency, u_int32_t timeout)
{
    static const char *tag = "CAC.started";
    struct net_device *dev = get_cfg80211_notification_ndev((osif_dev *)osif);
    union iwreq_data wrqu;
    char buf[128];

    if (!dev)
        return;

    /* TODO: needed parameters: count, keyid, key type, src address, TSC */
    snprintf(buf, sizeof(buf), "%s freq=%u timeout=%d", tag,
        frequency, timeout);
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = strlen(buf);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);
}
#endif

void
osif_notify_push_button(osdev_t osdev, u_int32_t push_time)
{
    struct net_device *dev = osdev->netdev;
    static char *tag = "PUSH-BUTTON.indication";
    union iwreq_data wrqu;
    char buf[128];
#if UMAC_SUPPORT_ACFG
#ifdef UMAC_SUPPORT_CFG80211
    osif_dev *osifp = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, INVALID_LINK_ID);
#else
    osif_dev *osifp = get_vap_device_context(NULL, dev, INVALID_LINK_ID);
#endif
    wlan_if_t vap = osifp->os_if;
    acfg_event_data_t *acfg_event = NULL;

    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }

    if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_PUSH_BUTTON)) {
        acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
        if (acfg_event) {
            acfg_send_event(dev, osdev, WL_EVENT_TYPE_PUSH_BUTTON, acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#endif

    snprintf(buf, sizeof(buf), "%s Push dur=%d", tag, push_time);
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = strlen(buf);
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);
}

int
ieee80211_load_module(const char *modname)
{
    request_module(modname);
    return 0;
}

static void osif_channel_change (os_handle_t osif, wlan_chan_t chan)
{
#if UMAC_SUPPORT_ACFG || QCA_SUPPORT_SON || UMAC_SUPPORT_CFG80211
    osif_dev  *osdev = (osif_dev *) osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap = osdev->os_if;
#endif

#if UMAC_SUPPORT_ACFG || UMAC_SUPPORT_CFG80211
    struct ieee80211com *ic = vap->iv_ic;
#endif

#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event = NULL;
    osdev_t  os_handle = ((osif_dev *)osif)->os_handle;
#endif
#if QCA_SUPPORT_SON
    union iwreq_data wreq;
    struct chan_change_evt_t chan_det = {0};
#endif
#if UMAC_SUPPORT_CFG80211
    struct ieee80211_channel *nl_chan;
    uint16_t chan_freq = 0;
#endif

#if UMAC_SUPPORT_ACFG
    if (!os_handle) {
        return;
    }
    if (osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_CHAN)) {
        acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
        if (acfg_event) {
            memset(acfg_event, 0, sizeof(acfg_event_data_t));

            if (!IEEE80211_IS_CHAN_VALID(chan))
                acfg_event->freqs[0] = 0;
            else
                acfg_event->freqs[0] = wlan_channel_frequency(chan);

            if(ic->ic_flags & IEEE80211_F_DFS_CHANSWITCH_PENDING){
                acfg_event->reason = ACFG_CHAN_CHANGE_DFS;
            }else{
                acfg_event->reason = ACFG_CHAN_CHANGE_NORMAL;
            }

            acfg_send_event(dev, os_handle, WL_EVENT_TYPE_CHAN, acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#endif

#if QCA_SUPPORT_SON
    // Band steering needs to be notified of channel change events so that
    // it can stop it and then re-enable once any ACS/CAC logic has completed.
    // Although this could have been done with a band steering event, a generic
    // link event seemed more consistent as a user-initiated channel change
    // already generates a link event (of type SIOCSIWFREQ).

    if(wlan_scan_in_progress(vap)) {
        return;
    }

    if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                IEEE80211_EV_CHAN_CHANGE)) {
        OS_MEMSET(&wreq, 0, sizeof(wreq));
        wreq.data.flags = IEEE80211_EV_CHAN_CHANGE;
        wreq.data.length = sizeof(chan_det);
        if (chan && (chan != IEEE80211_CHAN_ANYC)) {
            chan_det.channel = wlan_channel_ieee(chan);
            chan_det.freq = wlan_channel_frequency(chan);
#ifdef WLAN_FEATURE_11BE
            chan_det.puncture_bitmap = wlan_channel_puncture_bitmap(chan);
            chan_det.reg_punc_bitmap = wlan_channel_reg_punc_bitmap(chan);
#endif
        }
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (char *)&chan_det);
    }
#endif

#if UMAC_SUPPORT_CFG80211
    if((vap->iv_cfg80211_create) &&
       ((vap->iv_opmode == IEEE80211_M_HOSTAP)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 213)) && \
    (LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0))
        || ((vap->iv_opmode == IEEE80211_M_STA) &&
            (wlan_vdev_is_up_active_state(vap->vdev_obj) == QDF_STATUS_SUCCESS))
#endif
       )) {
        if(wlan_scan_in_progress(vap)) {
            return;
        }
        if (chan) {
            chan_freq = wlan_channel_frequency(chan);
        }
        nl_chan = ieee80211_get_channel(ic->ic_wiphy, chan_freq);
        if (!nl_chan) {
            qdf_err("%s: ieee80211_get_channel Failed .....", __func__);
            return;
        }
        ieee80211_cfg80211_construct_chandef(&vap->chan_notify.chandef_notify, nl_chan, chan);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && \
        defined(WLAN_FEATURE_11BE)
        vap->chan_notify.punct_bitmap = chan->op_puncture_bitmap;
#endif
        qdf_sched_work(dev, &vap->cfg80211_channel_notify_workq);
    }
#endif
}

#if UMAC_SUPPORT_CFG80211
static void osif_channel_switch_started(os_handle_t osif, wlan_chan_t chan)
{
    osif_dev  *osdev = (osif_dev *) osif;
    struct net_device *dev = osdev->netdev;
    wlan_if_t vap = osdev->os_if;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_channel *nl_chan = NULL;
    uint16_t chan_freq = 0;

    if(vap->iv_cfg80211_create && (vap->iv_opmode == IEEE80211_M_STA)) {
        if(wlan_scan_in_progress(vap)) {
            return;
        }

        if (chan) {
            chan_freq = wlan_channel_frequency(chan);
        }

        nl_chan = ieee80211_get_channel(ic->ic_wiphy, chan_freq);
        if (!nl_chan) {
            qdf_err("%s: ieee80211_get_channel Failed .....", __func__);
            return;
        }

        ieee80211_cfg80211_construct_chandef(&vap->chan_notify.chandef_notify, nl_chan, chan);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && \
        defined(WLAN_FEATURE_11BE)
        vap->chan_notify.punct_bitmap = chan->op_puncture_bitmap;
#endif
        qdf_sched_work(dev, &vap->cfg80211_channel_started_notify_workq);
    }
}
#else
static void osif_channel_switch_started(os_handle_t osif, wlan_chan_t chan)
{
}
#endif

static void osif_country_changed (os_handle_t osif, char *country)
{

}

static void osif_receive (os_if_t osif, wbuf_t wbuf,
                        u_int16_t type, u_int16_t subtype,
                        ieee80211_recv_status *rs)
{
    struct sk_buff *skb = (struct sk_buff *)wbuf;

    if (type != IEEE80211_FC0_TYPE_DATA) {
        if(wbuf_next(wbuf) != NULL) {
            wbuf_t wbx = wbuf;
            while (wbx) {
                wbuf = wbuf_next(wbx);
                wbuf_free(wbx);
                wbx = wbuf;
            }
        } else {
            wbuf_free(wbuf);
        }
        return;
    }
    /* deliver the data to network interface */
    __osif_deliver_data(osif, skb);
}

#if ATH_RX_INFO_EN
static void osif_send_mgmt_rx_info_event(os_handle_t osif, wbuf_t wbuf, ieee80211_recv_status *rs)
{
    osif_dev  *osdev = (osif_dev *) osif;
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event;
    wlan_if_t vap = ((osif_dev *)osif)->os_if;
#endif
    struct sk_buff *skb = (struct sk_buff *)wbuf;
    struct net_device *dev = osdev->netdev;
    acfg_mgmt_rx_info_t *mgmt_ri = NULL;
    int len = 0;
    static u_int32_t acfg_limit_print = 0;

    /*Drop event eariler here if acfg_event_list is full*/
    qdf_spin_lock_bh(&(osdev->os_handle->acfg_event_queue_lock));
    if(qdf_nbuf_queue_len(&(osdev->os_handle->acfg_event_list)) >= ACFG_EVENT_LIST_MAX_LEN){
        qdf_spin_unlock_bh(&(osdev->os_handle->acfg_event_queue_lock));
        if(!(acfg_limit_print % ACFG_EVENT_PRINT_LIMIT)){
        qdf_print("Work queue full, drop acfg event!");
        }
        ++acfg_limit_print;
        return;
    }
    qdf_spin_unlock_bh(&(osdev->os_handle->acfg_event_queue_lock));

#if UMAC_SUPPORT_ACFG
    if (!vap) {
        qdf_err(" VAP is null!!!");
        return;
    }
    if (!osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_MGMT_RX_INFO)) {
        return;
    }
    acfg_event = OS_MALLOC(osdev->os_handle, sizeof(acfg_event_data_t), GFP_KERNEL);
    if (acfg_event == NULL){
        return;
    }
    OS_MEMZERO(acfg_event, sizeof(acfg_event_data_t));

    mgmt_ri = &(acfg_event->mgmt_ri);
    mgmt_ri->ri_channel = rs->rs_channel;
    mgmt_ri->ri_rssi = rs->rs_snr;
    mgmt_ri->ri_datarate = rs->rs_datarate;
    mgmt_ri->ri_flags = rs->rs_flags;
    len = ((skb->len)>MGMT_FRAME_MAX_SIZE) ? MGMT_FRAME_MAX_SIZE: skb->len;
    OS_MEMCPY(mgmt_ri->raw_mgmt_frame, skb->data, len);

    acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_MGMT_RX_INFO, acfg_event);

    OS_FREE(acfg_event);
#endif
}
#endif

void
osif_update_dfs_info_to_app(struct ieee80211com *ic, char *buf)
{
    struct ieee80211vap *vap;
    union iwreq_data wrqu;
    struct net_device *dev = NULL;

    IEEE80211_COMM_LOCK(ic);
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
            continue;
        }
        if (vap->channel_switch_state) {
            continue;
        }

        dev = get_cfg80211_notification_ndev((osif_dev *)vap->iv_ifp);
        if (!dev)
            continue;

        memset(&wrqu, 0, sizeof(wrqu));
        wrqu.data.length = strlen(buf);
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);
    }
    IEEE80211_COMM_UNLOCK(ic);
}

/* From Atheros version:
* //depot/sw/releases/linuxsrc/src/802_11/madwifi/madwifi/net80211/ieee80211_input.c
*
* This is an entirely different approach which uses "wireless events"...
*/
#define GET_BE16(a) ((u16) (((a)[0] << 8) | (a)[1]))
#define ELEMENT_TYPE 0x1012

int push_method(const u_int8_t *frm)
{
     int len=6;
     int rc = 0;

     while ((len+4) < frm[1] ) {
        if((GET_BE16(frm+len) == ELEMENT_TYPE) && (frm[len+3] ==2)&& (GET_BE16(frm+len+4)== 0x0004))
        {
            rc = 1;
            qdf_print("wps pb detected");
            return rc;
        } else {
          len += frm[len+3]+4;
        }
     }
     return rc;
}

void osif_forward_mgmt_to_app(os_if_t osif, wbuf_t wbuf,
                                        u_int16_t type, u_int16_t subtype, ieee80211_recv_status *rs)
{
    struct net_device *dev;
    struct sk_buff *skb = (struct sk_buff *)wbuf;
    osif_dev  *osifp = (osif_dev *) osif;
    wlan_if_t vap = osifp->os_if;

    int filter_flag =0;
#ifndef MGMT_FRAM_TAG_SIZE
#define MGMT_FRAM_TAG_SIZE 30 /* hardcoded in atheros_wireless_event_wireless_custom */
#endif
    char *tag=NULL;
    char *wps_tag = NULL;
    #if 0       /* should be, but IW_CUSTOM_MAX is only 256 */
    const int bufmax = IW_CUSTOM_MAX;
    #else       /* HACK use size for IWEVASSOCREQIE instead */
    const int bufmax = IW_GENERIC_IE_MAX;
    #endif

    dev = get_cfg80211_notification_ndev(osifp);

    /* forward only if app filter is enabled */
	/* ifindex should be valid for forwarding frames to user space */
    if (!osifp->app_filter || (dev->ifindex == 0)){
        return;
    }

    switch (subtype)
    {
    case IEEE80211_FC0_SUBTYPE_BEACON:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_BEACON;
        tag = "Manage.beacon";
        break;
    case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
        /* HACK: Currently only WPS needs Probe reqs,
         * so forward WPS frames only */
        if (!wlan_frm_haswscie(wbuf) &&
            !wlan_get_param(vap, IEEE80211_TRIGGER_MLME_RESP)) {
            break;
        }
        if (!wlan_get_param(vap, IEEE80211_WPS_MODE) &&
            (subtype == IEEE80211_FC0_SUBTYPE_PROBE_REQ)) {
            break;
        }

        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_PROBE_REQ;
        tag = "Manage.prob_req";
        if(IEEE80211_VAP_IS_BACKHAUL_ENABLED(vap))// && IEEE80211_VAP_IS_HIDESSID_ENABLED(vap) /*orbi_ie*/)
        {
            struct ieee80211_frame *wh;
            u_int8_t *frm, *efrm;

            wh = (struct ieee80211_frame *) wbuf_header(wbuf);
            frm = (u_int8_t *)&wh[1];
            efrm = wbuf_header(wbuf) + wbuf_get_pktlen(wbuf);

            while (((frm+1) < efrm) && (frm + frm[1] + 1 < efrm)) {
                if (iswpsoui(frm) && push_method(frm))
                {
                    wps_tag = "Manage.prob_req_wps";
                    break;
                }
                else
                    wps_tag = NULL;
                frm += frm[1] + 2;
            }
        }
        break;
    case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_PROBE_RESP;
        tag = "Manage.prob_resp";
        break;
    case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
    case IEEE80211_FC0_SUBTYPE_REASSOC_REQ:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_ASSOC_REQ;
        tag = "Manage.assoc_req";
        break;
    case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
    case IEEE80211_FC0_SUBTYPE_REASSOC_RESP:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_ASSOC_RESP;
        tag = "Manage.assoc_resp";
        break;
    case IEEE80211_FC0_SUBTYPE_AUTH:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_AUTH;
        tag = "Manage.auth";
        break;
    case IEEE80211_FC0_SUBTYPE_DEAUTH:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_DEAUTH;
        tag = "Manage.deauth";
        break;
    case IEEE80211_FC0_SUBTYPE_DISASSOC:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_DISASSOC;
        tag = "Manage.disassoc";
        break;
    case IEEE80211_FC0_SUBTYPE_ACTION:
        filter_flag = osifp->app_filter & IEEE80211_FILTER_TYPE_ACTION;
        tag = "Manage.action";
        break;
    default:
        break;
    }
    if (filter_flag)
    {
        union iwreq_data wrqu;
        char *buf;
        size_t bufsize = MGMT_FRAM_TAG_SIZE + skb->len;

        if(bufsize > bufmax) {
            qdf_print("FIXME:%s: Event length more than expected..dropping", __FUNCTION__);
            return;
        }
        buf = OS_MALLOC(osifp->os_handle, bufsize, GFP_KERNEL);
        if (buf == NULL) return;
        OS_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = bufsize;
        OS_MEMZERO(buf, bufsize);
        snprintf(buf, MGMT_FRAM_TAG_SIZE, "%s %d", tag, skb->len);
        OS_MEMCPY(buf+MGMT_FRAM_TAG_SIZE, skb->data, skb->len);
        #if 0   /* the way it should? be */
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wrqu, buf);
        #else   /* HACK to get around 256 byte limit of IWEVCUSTOM */
        /* Note: application should treat IWEVASSOCREQIE same as IWEVCUSTOM
        * and should check for both.
        * IWEVASSOCREQIE is not used for anything (else) at least
        * not for Atheros chip driver.
        */

//        IEEE80211_DPRINTF(NULL, IEEE80211_MSG_IOCTL, "%s : filter_flag=%x, subtype=%x\n",
//                                      __func__,osifp->app_filter,subtype);
        WIRELESS_SEND_EVENT(dev, IWEVASSOCREQIE, &wrqu, buf);
        #endif
        OS_FREE(buf);

#if ATH_RX_INFO_EN
        if (rs)
            osif_send_mgmt_rx_info_event(osif, skb, rs);
#endif
    }

    if(wps_tag != NULL)
    {
        union iwreq_data wrqu;
        char *buf;
        size_t bufsize = MGMT_FRAM_TAG_SIZE + skb->len;

        if(bufsize > bufmax) {
            qdf_print("FIXME:%s: Event length more than expected..dropping", __FUNCTION__);
            return;
        }
        buf = OS_MALLOC(osifp->os_handle, bufsize, GFP_KERNEL);
        if (buf == NULL) return;
        OS_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = bufsize;
        OS_MEMZERO(buf, bufsize);
        snprintf(buf, MGMT_FRAM_TAG_SIZE, "%s %d", wps_tag, skb->len);
        OS_MEMCPY(buf+MGMT_FRAM_TAG_SIZE, skb->data, skb->len);
        WIRELESS_SEND_EVENT(dev, IWEVASSOCREQIE, &wrqu, buf);
        OS_FREE(buf);

#if ATH_RX_INFO_EN
        if (rs)
            osif_send_mgmt_rx_info_event(osif, skb, rs);
#endif
    }

#undef  MGMT_FRAM_TAG_SIZE
}


static int  osif_receive_filter_80211 (os_if_t osif, wbuf_t wbuf,
                                        u_int16_t type, u_int16_t subtype,
                                        ieee80211_recv_status *rs)
{
#if UMAC_SUPPORT_CFG80211
    osif_dev *osifp = (osif_dev *)osif;
    struct sk_buff *skb = (struct sk_buff *)wbuf;
    bool result = true ;
    struct ieee80211_frame *wh;
    wlan_if_t vap = osifp->os_if;
    struct wireless_dev *wdev = get_cfg80211_notification_wdev(osifp);

#ifdef ENABLE_CFG80211_BACKPORTS
    struct wlan_objmgr_vdev *vdev = NULL;
    int link_id = -1;
    vdev = vap->vdev_obj;
    if (wlan_vdev_mlme_is_mlo_vdev(vdev))
        link_id = wlan_vdev_get_link_id(vdev);
#endif
#endif

    if (type == IEEE80211_FC0_TYPE_MGT && wbuf) {
        osif_forward_mgmt_to_app(osif, wbuf, type, subtype, rs);
#if UMAC_SUPPORT_CFG80211
        if(vap->iv_cfg80211_create) {
            wlan_vendor_fwd_mgmt_frame(vap, skb, subtype);
            wh = (struct ieee80211_frame *) wbuf_header(wbuf);
            switch(subtype ) {
                case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
                    /* HACK: Currently only WPS needs Probe reqs,
                     * so forward WPS frames only */

                    if (!wlan_get_param(vap, IEEE80211_WPS_MODE) ){
                        break;
                    }
                case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
                case IEEE80211_FC0_SUBTYPE_REASSOC_REQ:
                case IEEE80211_FC0_SUBTYPE_AUTH:
                case IEEE80211_FC0_SUBTYPE_ACTION:
                case IEEE80211_FC0_SUBTYPE_DISASSOC:
                case IEEE80211_FC0_SUBTYPE_DEAUTH:
                    /* Deliver to cfg80211 only if the netdevice is registered
                     * with Kernel */
                    if (ieee80211_vap_registered_is_set(vap)) {
#ifdef ENABLE_CFG80211_BACKPORTS
                        result = wlan_cfg80211_rx_mgmt(wdev,
                                rs?rs->rs_freq:vap->iv_ic->ic_curchan->ic_freq,
                                0, (u8 *)skb->data, skb->len, 0, link_id);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
                        result = cfg80211_rx_mgmt(wdev,
                                rs?rs->rs_freq:vap->iv_ic->ic_curchan->ic_freq,
                                0, (u8 *)skb->data, skb->len, 0);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
                        result = cfg80211_rx_mgmt(&(osifp->iv_wdev),
                                rs?rs->rs_freq:vap->iv_ic->ic_curchan->ic_freq,
                                0, (u8 *)skb->data, skb->len, 0, GFP_ATOMIC);
#endif /* ENABLE_CFG80211_BACKPORTS */
                    }
                    if (!result) {
                        IEEE80211_DPRINTF(vap, IEEE80211_MSG_CFG80211,
                             "cfg80211_rx_mgmt for subtype:%0x to %s failed\n",
                              subtype,ether_sprintf(wh->i_addr2));
                        return 1;
                    }
                    break;
                default :
                    break;
            }
        }
#endif
    }
    return 0;
}

/*
 * osif_monitor_get_80211_header(): Function to retrieve IEEE80211 header
 * from received buffer in monitor mode
 * @wbuf: Received nbuf
 *
 * This function is called to retrieve IEEE80211 header from the received nbuf
 * in the monitor mode. This function is to be called only for the Lithium
 * family
 *
 * Return: Pointer to IEEE80211 header
 */
static
struct ieee80211_qosframe_addr4 *osif_monitor_get_80211_header(wbuf_t wbuf)
{
    struct ieee80211_qosframe_addr4 *qwh;

#ifdef DP_RX_MON_MEM_FRAG
    /**
     * Frag at index 0 gives IEEE80211 header if wbuf has frags attached
     * in monitor frag approach
     */
    if (qdf_nbuf_get_nr_frags(wbuf)) {
        qwh = (struct ieee80211_qosframe_addr4 *)qdf_nbuf_get_frag_addr(wbuf,0);
    } else
#endif
    {
        /* Get IEEE80211 header from wbuf header */
        u_int16_t radiotap_header_len;
        struct ieee80211_radiotap_header *t_rd_hdr1;

        t_rd_hdr1 = (struct ieee80211_radiotap_header *)wbuf_header(wbuf);
        radiotap_header_len = qdf_le16_to_cpu(t_rd_hdr1->it_len);
        qwh = (struct ieee80211_qosframe_addr4 *)(wbuf_header(wbuf) +
                                                  radiotap_header_len);
    }
    return qwh;
}

static int
mgmt_crypto_decap(wbuf_t wbuf, struct ieee80211com *ic,struct ieee80211vap *vap, uint8_t *macaddr)
{
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_crypto_rx_ops *crypto_rx_ops;
    struct ieee80211_frame *wh;
    int status = 0;

    pdev = ic->ic_pdev_obj;
    psoc = wlan_pdev_get_psoc(pdev);
    crypto_rx_ops = wlan_crypto_get_crypto_rx_ops(psoc);
    if (crypto_rx_ops && WLAN_CRYPTO_RX_OPS_DECAP(crypto_rx_ops)) {
        status = WLAN_CRYPTO_RX_OPS_DECAP(crypto_rx_ops)(vap->vdev_obj,
               wbuf, macaddr, 0);
        if ( status  == QDF_STATUS_SUCCESS) {
               wh = (struct ieee80211_frame *)wbuf_header(wbuf);
               wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
        }
        else if (status == QDF_STATUS_CRYPTO_DECRYPT_FAILED) {
               /*Increment Rx Decrypt error stats*/
               vdev_mcast_cp_stats_rx_decryptcrc_inc(vap->vdev_obj, 1);
        }
    }

    return 0;
}

void osif_receive_monitor_80211_base (os_if_t osif, wbuf_t wbuf, void *rs)
{
    osif_dev *osifp = (osif_dev *)osif;
    struct ieee80211vap *vap = NULL;
    struct sk_buff *skb;
    int found = 0;
    struct net_device *dev = osifp->netdev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    struct net_device_stats *stats = &osifp->os_devstats;
#else
    struct rtnl_link_stats64 *stats = &osifp->os_devstats;
#endif
    struct ieee80211com *ic = NULL;
    u_int32_t target_type;
    u_int16_t radiotap_header_len = 0;
    struct ieee80211_radiotap_header *t_rd_hdr1 = NULL;
    struct ieee80211_qosframe_addr4 *qwh;

    vap = osifp->os_if;
    if (!vap) {
        qdf_err("VAP is NULL");
        qdf_nbuf_free(wbuf);
        return;
    }

    ic = wlan_vap_get_devhandle(vap);
    if (!ic) {
        qdf_err("ic is NULL");
        qdf_nbuf_free(wbuf);
        return;
    }

    target_type = ic->ic_get_tgt_type(ic);

    /*
     * For lithium targets, the radiotap header is present at the
     * wbuf_header. For non-lithium targets, it is added later below
     */
    if (target_type >= TARGET_TYPE_QCA8074) {
        t_rd_hdr1 = (struct ieee80211_radiotap_header *)wbuf_header(wbuf);
        radiotap_header_len = qdf_le16_to_cpu(t_rd_hdr1->it_len);
        qwh = osif_monitor_get_80211_header(wbuf);
    } else {
        qwh = (struct ieee80211_qosframe_addr4 *)wbuf_header(wbuf);
    }
    if (qdf_unlikely(qwh->i_fc[1] == (IEEE80211_FC1_DIR_DSTODS | IEEE80211_FC1_WEP))
        && (IEEE80211_ADDR_IS_VALID(vap->mcast_encrypt_addr) || ic->enable_monitor_peer_decap)) {

        uint8_t *rtap_buf;
        if (qdf_nbuf_is_nonlinear(wbuf) &&
            qdf_nbuf_get_nr_frags(wbuf)) {
            /* Make copy of wbuf to get nbuf with linear data */
            qdf_nbuf_t nbuf_p = qdf_nbuf_copy(wbuf);
            qdf_nbuf_free(wbuf);

            if (!nbuf_p) {
                qdf_err("NBUF copy failed");
                return;
            }
            wbuf = nbuf_p;
        }
        if (target_type >= TARGET_TYPE_QCA8074) {
           qwh = osif_monitor_get_80211_header(wbuf);
        } else {
           qwh = (struct ieee80211_qosframe_addr4 *)wbuf_header(wbuf);
        }

        rtap_buf = (uint8_t *)qdf_mem_malloc((sizeof(struct ieee80211_radiotap_header)) +
                                              radiotap_header_len);
        qdf_mem_copy(rtap_buf, qdf_nbuf_data(wbuf), radiotap_header_len);
        qdf_nbuf_pull_head(wbuf, radiotap_header_len);

            if (IEEE80211_ADDR_EQ(qwh->i_addr1,vap->mcast_encrypt_addr))
                mgmt_crypto_decap(wbuf,ic,vap,vap->mcast_encrypt_addr);

            else {

            struct ieee80211_node *ni;
            ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)qwh, WLAN_MLME_SB_ID);
            if (ni) {
                mgmt_crypto_decap(wbuf, ic, vap, ni->ni_macaddr);
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);

            }
        }
        qdf_nbuf_push_head(wbuf, radiotap_header_len);
        qdf_mem_copy(qdf_nbuf_data(wbuf), rtap_buf, radiotap_header_len);
        qdf_mem_free(rtap_buf);
    }
    skb = (struct sk_buff *)wbuf;

#if ATH_SUPPORT_NAC

    if (IEEE80211_IS_SMART_MON_VAP(vap)) {
        int i,j;
        char nullmac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        int max_addrlimit = NAC_MAX_CLIENT;
        struct ieee80211_nac_info  *nac_list = vap->iv_nac.client;
        struct ieee80211_qosframe_addr4 *qwh = NULL;

        /* For non-lithium targets, radiotap is filled after this point
         * and nbuf has wireless header.
         * But for lithium targets, radiotap is already populated and
         * skb has payload starting from radiotap hdr. Hence we move skb
         * pointer by radiotap length offset to get to wireless header.
         */
        if (target_type >= TARGET_TYPE_QCA8074) {
            qwh = osif_monitor_get_80211_header(wbuf);
        } else {
            qwh = (struct ieee80211_qosframe_addr4 *)wbuf_header(skb);
        }
        for(i=0,j=0; i < max_addrlimit && j < max_addrlimit; i++) {

            if (IEEE80211_IS_MULTICAST((char *)(nac_list +i)->macaddr) ||
                IEEE80211_ADDR_EQ((char *)(nac_list +i)->macaddr, nullmac)) {
                continue;
            }

            if(OS_MEMCMP(qwh->i_addr2 , (nac_list +i)->macaddr, QDF_MAC_ADDR_SIZE)==0) {
                if (target_type < TARGET_TYPE_QCA8074) {
                    (nac_list +i)->rssi = ((ieee80211_recv_status *)rs)->rs_snr;
                } else {
                    /* For lithium targets rs struct passed may be NULL or have different format
                     * therefore we retrieve rssi from DP's neighbour peer struct
                     */
                    vap->iv_neighbour_rx(vap , i, IEEE80211_NAC_PARAM_LIST, IEEE80211_NAC_MACTYPE_CLIENT, (nac_list +i)->macaddr);
                }
                // Calculating the moving average of RSSI samples
                if ((nac_list + i)->avg_rssi != 0) {
                    (nac_list + i)->avg_rssi =
                    ((nac_list + i)->avg_rssi -
                    (A_RSSI)(((A_UINT8)((nac_list + i)->avg_rssi)) >> 2)) +
                    (A_RSSI)(((A_UINT8)((nac_list + i)->rssi)) >> 2);
                } else {
                    // First sample
                    (nac_list + i)->avg_rssi = (nac_list + i)->rssi;
                }

                if ((nac_list + i)->avg_rssi != 0) {
                    (nac_list + i)->rssi_measured_time = OS_GET_TIMESTAMP();
                } else {
                    // Adding this check to prevent stale rssi time
                    // for invalid rssi value
                    (nac_list + i)->rssi_measured_time = 0;
                }
                break;
            }

            j++;
        }
    }
#endif

    IEEE80211_VAP_LOCK(vap);
    if ((ic->mon_filter_osif_mac == 1) && vap->mac_entries) {
        int hash;
        struct ieee80211_qosframe_addr4 *qwh = NULL;
        struct ieee80211_mac_filter_list *mac_en;

        if (target_type >= TARGET_TYPE_QCA8074) {
            qwh = osif_monitor_get_80211_header(wbuf);
        } else {
            qwh = (struct ieee80211_qosframe_addr4 *)wbuf_header(skb);
        }
        hash = MON_MAC_HASH(qwh->i_addr1);
        LIST_FOREACH(mac_en, &vap->mac_filter_hash[hash], mac_entry) {
            if (mac_en->mac_addr != NULL) {
                if (IEEE80211_ADDR_EQ(qwh->i_addr1, mac_en->mac_addr)){
                    found = 1;
                    break;
                }
            }
        }
    }
    IEEE80211_VAP_UNLOCK(vap);

    if (qdf_likely(ic->ic_mon_decoder_type == MON_DECODER_RADIOTAP)) {
        /* For non lithium targets radiotap hdr has to be added here */
        if (target_type < TARGET_TYPE_QCA8074)
            osif_mon_add_radiotap_header(osif, skb, ((ieee80211_recv_status *)rs));
        skb->protocol = __constant_htons(ETH_P_802_2);
    } else {
        /* For non lithium targets prism hdr has to be added here */
        if (target_type < TARGET_TYPE_QCA8074)
            osif_mon_add_prism_header(osif, skb, ((ieee80211_recv_status *)rs));
        skb->protocol = __constant_htons(ETH_P_80211_RAW);
    }

    skb->dev = dev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    skb->mac.raw = skb->data;
#else
    skb_reset_mac_header(skb);
#endif
    skb->ip_summed = CHECKSUM_NONE;
    skb->pkt_type = PACKET_OTHERHOST;

    skb_orphan(skb);
    if (ic->mon_filter_osif_mac == 1) {
        /*If osif MAC addr based filter enabled,*/
        /*only input pkts from specified MAC*/
        if (qdf_unlikely(found == 1)) {
            nbuf_debug_del_record(skb);
            stats->rx_packets++;
            stats->rx_bytes += skb->len;
            netif_rx(skb);
        } else {
            qdf_nbuf_free(skb);
        }
    }else{
        nbuf_debug_del_record(skb);
        stats->rx_packets++;
        stats->rx_bytes += skb->len;
        netif_rx(skb);
    }
}
qdf_export_symbol(osif_receive_monitor_80211_base);

void osif_receive_monitor_80211 (os_if_t osif, wbuf_t wbuf,
                                        ieee80211_recv_status *rs)
{
    /* This breaks for man reasons
     * 1. Doesnt handle the generic fraglist skb case
     * 2. Doesnt handled non FCS trailers like encryption
     * Change model to have lower layer call in with non-trailer added
     * packet length
     */
    ((struct sk_buff *) wbuf)->len -= 4;
    osif_receive_monitor_80211_base(osif, wbuf, rs);
}


#if ATH_SUPPORT_FLOWMAC_MODULE
int
dev_queue_status(struct net_device *dev, int margin)
{
	struct Qdisc *fifoqdisc;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,15))
    struct netdev_queue *txq;
#endif
    int qlen = 0;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,15))
    txq = netdev_get_tx_queue(dev, 0);
    ASSERT((txq != NULL));
    fifoqdisc = txq->qdisc;
#else
	fifoqdisc = rcu_dereference(dev->qdisc);
#endif

    qlen = skb_queue_len(&fifoqdisc->q);
    if ((dev->tx_queue_len > margin) &&
            (qlen > dev->tx_queue_len - margin)) {
        return -ENOBUFS;
    }
    return 0;
}
int
dev_queue_length(struct net_device *dev)
{
	struct Qdisc *fifoqdisc;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,15))
    struct netdev_queue *txq;
#endif
    int qlen = 0;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,15))
    txq = netdev_get_tx_queue(dev, 0);
    ASSERT((txq != NULL));
    fifoqdisc = txq->qdisc;
#else
	fifoqdisc = rcu_dereference(dev->qdisc);
#endif
    qlen = skb_queue_len(&fifoqdisc->q);
    return qlen;
}
#endif

/*
* hand over the wbuf to the com device (ath dev).
*/
static int
osif_dev_xmit_queue (os_if_t osif, wbuf_t wbuf)
{
    int err;
    struct net_device *comdev;
    struct sk_buff *skb = (struct sk_buff *)wbuf;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
    struct ieee80211_cb * cb = NULL;
#endif

    int rval=0;
    comdev = ((osif_dev *)osif)->os_comdev;

    skb->dev = comdev;
    /*
     * rval would be -ENOMEM if there are 1000-128 frames left in the backlog
     * queue. We return this value out to the caller. The assumption of
     * caller is to make sure that he stalls the queues just after this. If
     * we do not let this frame out, node_ref issues might show up.
     */
#if ATH_SUPPORT_FLOWMAC_MODULE
    rval = dev_queue_status(comdev, 128);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
    cb = (struct ieee80211_cb *) qdf_nbuf_get_ext_cb(skb);
    cb->_u.context = wbuf_get_peer(skb);
#endif

    nbuf_debug_del_record(skb);
    err = dev_queue_xmit(skb);
#if ATH_SUPPORT_FLOWMAC_MODULE
    if (err == NET_XMIT_DROP) {
        /* TODO FIXME to stop the queue here with flow control enabled
        */
        rval = -ENOBUFS;
    }
    /* if there is no space left in below queue, make sure that we pause the
     * vap queue also
     */
    if (err == -ENOMEM) rval = -ENOBUFS;
#endif
    return rval;
}

DECLARE_N_EXPORT_PERF_CNTR(vap_xmit);


/*
* hand over the wbuf to the vap if (queue back into the umac).
*/
static void osif_vap_xmit_queue (os_if_t osif, wbuf_t wbuf)
{
    int err;
    osif_dev *osdev = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    wlan_if_t vap = osdev->os_if;
    struct sk_buff *skb = (struct sk_buff *)wbuf;

    if (!dev) {
        qdf_err("netdev is NULL for this vap type");
        return;
    }

    START_PERF_CNTR(vap_xmit, vap_xmit);

    skb->dev = dev;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
    skb->mac.raw = skb->data;
    skb->nh.raw = skb->data + sizeof(struct ether_header);
#else
    skb_reset_mac_header(skb);
    skb_set_network_header(skb, sizeof(struct ether_header));
#endif
    skb->protocol = __constant_htons(ETH_P_802_2);
    /* XXX inser`t vlan tage before queue it? */
    nbuf_debug_del_record(skb);
    err = dev_queue_xmit(skb);

    if (err != 0) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: wbuf xmit failed \n", __func__);
    }
    END_PERF_CNTR(vap_xmit);
}

#if ATH_SUPPORT_WRAP
static void wlan_wrap_vap_xmit_queue (struct net_device *dev, wbuf_t wbuf)
{
    osif_dev *osdev = ath_netdev_priv(dev);
    wlan_if_t vap;
    vap = osdev->os_if;
    osif_vap_xmit_queue(vap->iv_ifp,wbuf);
    return;
}
#endif

#if ATH_SUPPORT_FLOWMAC_MODULE
static void
osif_pause_queue (os_if_t osif, int pause, unsigned int pctl)
{
    osif_dev *osifp = (osif_dev *)osif;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    struct ath_softc_net80211 *scn;
    /* if puase, call the pause other wise wake */
#ifdef UMAC_SUPPORT_CFG80211
    scn = get_radio_device_context(osifp->os_comdev->ieee80211_ptr->wiphy,
                                   osifp->os_comdev);
#else
    scn = get_radio_device_context(NULL, osifp->os_comdev);
#endif

    if (!dev || !scn) return ;

    if (pause) {
        netif_stop_queue(dev);
        /* Literally we should have stopped the ethernet just over 2/3 queue
         * is full. But for this to happen this should get called through
         * network stack first. As soon as we stopped the queue, we would
         * not be getting any calls to hard_start. That way we never able to
         * pause the Ethernet effectivly. The only simple way is to let us
         * pause the both. Otherwise kernel code need to change, but that
         * becomes more non-portable code.
         * Or else, the vap should have queue of frames and when reaches
         * limit pause the Ethernet.
         */

        if (scn->sc_ops->flowmac_pause && pctl ) {
            scn->sc_ops->flowmac_pause(scn->sc_dev, 1);

        }


    } else {
        netif_wake_queue(dev);

        if (scn->sc_ops->flowmac_pause && pctl) {
            scn->sc_ops->flowmac_pause(scn->sc_dev, 0);

        }

    }
}

#endif

static void
osif_vap_scan_cancel(os_if_t osif)
{
    osif_dev *osdev = (osif_dev *)osif;
    wlan_if_t vap;
    struct wlan_objmgr_vdev *vdev = NULL;
    QDF_STATUS status;

    vap = osdev->os_if;
    vdev = osdev->ctrl_vdev;
    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_OSIF_SCAN_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        scan_info("unable to get reference");
        return;
    }
    /* Cancel all vdev scans without waiting for completions */
    wlan_ucfg_scan_cancel(vap, osdev->scan_requestor, 0,
            WLAN_SCAN_CANCEL_VDEV_ALL, false);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
}

static void osif_bringup_ap_vaps(os_if_t osif)
{
    osif_dev  *osdev = (osif_dev *) osif;
    wlan_if_t tmpvap = NULL, vap = osdev->os_if;
    wlan_dev_t comhandle = wlan_vap_get_devhandle(vap);

    TAILQ_FOREACH(tmpvap, &comhandle->ic_vaps, iv_next) {
        if(tmpvap->iv_opmode == IEEE80211_M_HOSTAP ||
                tmpvap->iv_opmode == IEEE80211_M_MONITOR ) {
            *tmpvap->iv_des_chan[tmpvap->iv_des_mode] = *comhandle->ic_curchan;
        }
    }
    wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_PDEV_START);
}

#if ATH_SUPPORT_IWSPY

#define SNR_QUAL_MIN		1
#define SNR_QUAL_THRESH1	5
#define SNR_QUAL_THRESH2	30
#define SNR_QUAL_MAX		42
#define QUAL_VALUE_MIN		0
#define QUAL_VALUE_THRESH1	5
#define QUAL_VALUE_THRESH2	85
#define QUAL_VALUE_MAX	94

static void
osif_set_linkquality(struct iw_quality *iq, u_int snr)
{
    if(snr >= SNR_QUAL_MAX)
        iq->qual = QUAL_VALUE_MAX ;
    else if(snr >= SNR_QUAL_THRESH2)
        iq->qual = QUAL_VALUE_THRESH2 + ((QUAL_VALUE_MAX-QUAL_VALUE_THRESH2)*(snr-SNR_QUAL_THRESH2) + (SNR_QUAL_MAX-SNR_QUAL_THRESH2)/2) \
					/ (SNR_QUAL_MAX-SNR_QUAL_THRESH2) ;
    else if(snr >= SNR_QUAL_THRESH1)
        iq->qual = QUAL_VALUE_THRESH1 + ((QUAL_VALUE_THRESH2-QUAL_VALUE_THRESH1)*(snr-SNR_QUAL_THRESH1) + (SNR_QUAL_THRESH2-SNR_QUAL_THRESH1)/2) \
					/ (SNR_QUAL_THRESH2-SNR_QUAL_THRESH1) ;
    else if(snr >= SNR_QUAL_MIN)
        iq->qual = snr;
    else
        iq->qual = QUAL_VALUE_MIN;

    iq->noise = 161;        /* -95dBm */
    iq->level = iq->noise + snr ;
    iq->updated = 0xf;
}

/*
 * Call for the driver to update the spy data.
 * For now, the spy data is a simple array. As the size of the array is
 * small, this is good enough. If we wanted to support larger number of
 * spy addresses, we should use something more efficient...
 */
void osif_iwspy_update(os_if_t osif,
			 u_int8_t *address,
			 int8_t snr)
{
    osif_dev  *osifp = (osif_dev *) osif;
    struct iw_spy_data *spydata = &osifp->spy_data;
	int	                i;
	int                 match = -1;
    struct iw_quality   iq;

	/* Make sure driver is not buggy or using the old API */
	if(!spydata)
		return;

	/* Update all records that match */
	for(i = 0; i < spydata->spy_number; i++)
		if(IEEE80211_ADDR_EQ(address, spydata->spy_address[i])) {
            osif_set_linkquality(&iq, snr);
			memcpy(&(spydata->spy_stat[i]), &iq, sizeof(struct iw_quality));
			match = i;
		}

#if 0 /* for SIOCSIWTHRSPY/SIOCGIWTHRSPY, not to implement now */
	/* Generate an event if we cross the spy threshold.
	 * To avoid event storms, we have a simple hysteresis : we generate
	 * event only when we go under the low threshold or above the
	 * high threshold. */
	if(match >= 0) {
		if(spydata->spy_thr_under[match]) {
			if(wstats->level > spydata->spy_thr_high.level) {
				spydata->spy_thr_under[match] = 0;
				iw_send_thrspy_event(dev, spydata,
						     address, wstats);
			}
		} else {
			if(wstats->level < spydata->spy_thr_low.level) {
				spydata->spy_thr_under[match] = 1;
				iw_send_thrspy_event(dev, spydata,
						     address, wstats);
			}
		}
	}
#endif
}
/*
 * Call for reset the spy data by mac address
 */
void osif_iwspy_reset_data(os_if_t osif,
			 u_int8_t *address)
{
    osif_dev  *osifp = (osif_dev *) osif;
    struct iw_spy_data *spydata = &osifp->spy_data;
	int	                i;

	/* Make sure driver is not buggy or using the old API */
	if(!spydata)
		return;

	/* Reset all records that match */
	for(i = 0; i < spydata->spy_number; i++)
		if(!memcmp(address, spydata->spy_address[i], ETH_ALEN)) {
            memset(&(spydata->spy_stat[i]), 0x0, sizeof(struct iw_quality));
		}
}
#endif

static void osif_linkspeed(os_handle_t osif, u_int32_t rxlinkspeed, u_int32_t txlinkspeed)
{

}

static void osif_beacon_miss(os_handle_t osif)
{
    osif_dev *osdev = (osif_dev *)osif;
    wlan_if_t vap;

    vap = osdev->os_if;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: beacon miss \n", __func__);
}

static void osif_device_error(os_handle_t osif)
{
    osif_dev *osdev = (osif_dev *)osif;
    wlan_if_t vap;
    ieee80211_reset_request reset_req;

    reset_req.reset_mac=0;
    reset_req.type = IEEE80211_RESET_TYPE_INTERNAL;
    reset_req.no_flush=0;
    vap = osdev->os_if;
    wlan_reset_start(vap, &reset_req);
    wlan_reset(vap, &reset_req);
    wlan_reset_end(vap, &reset_req);

}


/*
 * caller gets first element of list. Caller must free the element.
 */
struct pending_rx_frames_list *osif_fetch_p2p_mgmt(struct net_device *dev)
{
    osif_dev  *osifp = ath_netdev_priv(dev);
    unsigned long flags=0;
    struct pending_rx_frames_list *pending = NULL;

    spin_lock_irqsave(&osifp->list_lock, flags);
    if (!list_empty(&osifp->pending_rx_frames)) {
        pending = (void *) osifp->pending_rx_frames.next;
        list_del(&pending->list);
    }
    spin_unlock_irqrestore(&osifp->list_lock, flags);

    return pending;
}

#ifdef HOST_OFFLOAD
/*
 * Is the list of pending frames empty or not
 */
int osif_is_pending_frame_list_empty(struct net_device *dev)
{
    osif_dev  *osifp = ath_netdev_priv(dev);
    unsigned long flags=0;
    int empty = 0;

    spin_lock_irqsave(&osifp->list_lock, flags);
    if (list_empty(&osifp->pending_rx_frames))
        empty = 1;
    spin_unlock_irqrestore(&osifp->list_lock, flags);

    return empty;
}
#endif

/*
 * queue a big event for later delivery to user space
 * Caller must provide event->u.rx_frame.frame_len and
 * event->u.rx_frame.frame_buf for the data to be queued for later delivery
 */
void osif_p2p_rx_frame_handler(osif_dev *osifp,
                               wlan_p2p_event *event,
                               int frame_type)
{
    struct pending_rx_frames_list *pending;
    unsigned long flags;

    pending = (struct pending_rx_frames_list *)OS_MALLOC(osifp->os_handle,
                                 sizeof(*pending) + event->u.rx_frame.frame_len,
                                 GFP_ATOMIC);
    if (pending) {
        pending->frame_type = frame_type;
        pending->rx_frame = event->u.rx_frame;

        memcpy(pending->extra, event->u.rx_frame.frame_buf,
                                event->u.rx_frame.frame_len);

        INIT_LIST_HEAD (&pending->list);
        spin_lock_irqsave(&osifp->list_lock, flags);
        list_add_tail(&pending->list, &osifp->pending_rx_frames);
        spin_unlock_irqrestore(&osifp->list_lock, flags);
    } else {
        qdf_print(KERN_CRIT "%s OS_MALLOC failed, frame lost", __func__);
    }
}

struct event_data_chan_start {
    u_int32_t freq;
    u_int32_t duration;
    u_int32_t req_id;
};

struct event_data_chan_end {
    u_int32_t freq;
    u_int32_t reason;
    u_int32_t duration;
    u_int32_t req_id;
};

static void osif_wlan_tx_overflow_event(os_handle_t osif)
{
#if UMAC_SUPPORT_ACFG
    osif_dev  *osdev = (osif_dev *) osif;
#if !PEER_FLOW_CONTROL_FORCED_MODE0
    wlan_if_t vap = osdev->os_if;
    static u_int32_t tx_overflow_limit_print = 0;
#endif
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    acfg_event_data_t *acfg_event = NULL;

    if (dev && osif_vap_event_filter_status_to_app(vap, EVENT_ACFG, WL_EVENT_TYPE_TX_OVERFLOW)) {
        acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
        if (acfg_event) {
            memset(acfg_event, 0, sizeof(acfg_event_data_t));
            acfg_send_event(dev, ((osif_dev *)osif)->os_handle, WL_EVENT_TYPE_TX_OVERFLOW, acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#if !PEER_FLOW_CONTROL_FORCED_MODE0
    if(!(tx_overflow_limit_print % TXOVERFLOW_PRINT_LIMIT))
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: TX overflow event sent. \n", __func__);
    }
    tx_overflow_limit_print++;
#endif
#endif
}

#if QCA_LTEU_SUPPORT
static void
osif_mu_report(wlan_if_t vap, struct event_data_mu_rpt *mu_rpt)
{
    osif_dev *osdev = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct net_device *dev = get_cfg80211_notification_ndev(osdev);
    union iwreq_data wreq;

    if (!dev)
        return;

    memset(&wreq, 0, sizeof(wreq));
    wreq.data.flags = IEEE80211_EV_MU_RPT;
    wreq.data.length = sizeof(struct event_data_mu_rpt);
    WIRELESS_SEND_EVENT(dev, IWEVGENIE, &wreq, (void *)mu_rpt);
}
#endif

static wlan_misc_event_handler_table sta_misc_evt_handler = {
    osif_channel_change,                   /* wlan_channel_change */
    osif_country_changed,                  /* wlan_country_changed */
    osif_linkspeed,                        /* wlan_linkspeed */
    osif_michael_failure_indication,       /* wlan_michael_failure_indication */
    osif_replay_failure_indication,        /* wlan_replay_failure_indication */
    osif_beacon_miss,                      /* wlan_beacon_miss_indication */
    NULL,                                  /* wlan_beacon_rssi_indication */
    osif_device_error,                     /* wlan_device_error_indication */
    NULL,                                  /* wlan_sta_clonemac_indication */
    NULL,                                  /* wlan_sta_scan_entry_update */
    NULL,                                  /* wlan_ap_stopped */
#if ATH_SUPPORT_WAPI
    NULL,                                  /*wlan_sta_rekey_indication*/
#endif
#if UMAC_SUPPORT_RRM_MISC
    NULL,				                   /* osif_chload */
    NULL,                                                  /* osif_nonerpcnt */
    NULL,				                   /* osif_bgjoin */
    NULL,				                   /* osif_cochannelap_cnt*/
#endif
#if QCA_SUPPORT_SON
    NULL,                                     /* wlan_buffull */
#endif
    NULL,                   /* wlan_session_timeout */
#if ATH_SUPPORT_MGMT_TX_STATUS
   NULL,
#endif
#if QCA_SUPPORT_SSID_STEERING
    NULL, /* ssid_event , void (*ssid_event)(os_handle_t, u_int8_t *macaddr); */
#endif
    NULL,                                 /* assocdeny_event */
    NULL,                                 /*wlan_tx_overflow */
    NULL,                                 /*wlan_ch_hop_channel_change*/
    NULL,                                 /*  wlan_recv_probereq */
#if DBDC_REPEATER_SUPPORT
    osif_dbdc_stavap_connection,                /*stavap_connection*/
#endif
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    osif_sta_cac_started,                 /* wlan_sta_cac_started */
#endif
    NULL,				/*chan_util_event*/
    NULL,				/*wlan_keyset_done_indication*/
    NULL,				/*wlan_blklst_sta_auth_indication*/
#if ATH_PARAMETER_API
    NULL, /* papi_event ,  void (*papi_event)(os_handle_t,enum,char eventlen,char *data); */
#endif
    NULL,                            /* bss node freed indication */
    NULL,                            /* wlan_recv_neighreq */
    NULL,                            /* wlan_recv_beaconrpt */
    NULL,                            /* wlan_recv_bstm */
    NULL,			     /* wlan_recv_smpsupdate */
    NULL,			     /* wlan_recv_opmodeupdate */
    NULL, /* wlan_vdev_state_up_active_indication */
    osif_recv_awgninterferenceupdate,
    osif_channel_switch_started,  /* wlan_channel_switch_started */
};

static wlan_event_handler_table common_evt_handler = {
    osif_receive,
    osif_receive_filter_80211,
    osif_receive_monitor_80211,
    osif_dev_xmit_queue,
    osif_vap_xmit_queue,
    NULL,
#if ATH_SUPPORT_IWSPY
	osif_iwspy_update,
#endif
#if ATH_SUPPORT_FLOWMAC_MODULE
    osif_pause_queue,
#endif
    osif_vap_scan_cancel,       /* wlan_vap_scan_cancel */
    osif_bringup_ap_vaps,       /* wlan_bringup_ap_vaps */
};
static void
osif_join_complete_infra_sta(os_handle_t osif, IEEE80211_STATUS status)
{
    osif_dev  *osdev = (osif_dev *) osif;
    wlan_if_t vap = osdev->os_if;
#if DBDC_REPEATER_SUPPORT
    struct ieee80211_vap_info vap_info;
#endif

    if (status != IEEE80211_STATUS_SUCCESS) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                "%s: status: %d, return..\n", __func__, status);
        return;
    }

    if (wlan_rptr_pdev_is_eir(vap->iv_ic->ic_pdev_obj)) {
#if DBDC_REPEATER_SUPPORT
        if (!wlan_rptr_global_is_g_wds()) {
            vap_info.chan = 0;
            vap_info.no_cac = 1;
            wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_PDEV_START);
            vap_info.no_cac = 0;
        }
#endif
     }
}
#define IEEE80211_MSG_CSL_STR         "[CSL] "
static void osif_join_complete_infra_csl (os_handle_t osif, IEEE80211_STATUS status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_JOIN_COMPLETE_INFRA_EVENT");
}

static void osif_join_complete_adhoc_csl (os_handle_t osif, IEEE80211_STATUS status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_JOIN_COMPLETE_ADHOC_EVENT");
}

static void osif_auth_complete_csl (os_handle_t osif, u_int8_t *macaddr, IEEE80211_STATUS status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_AUTH_COMPLETE_EVENT");
}

static void osif_assoc_req_csl (os_handle_t osif, wbuf_t wbuf)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_ASSOC_REQ_EVENT");
}

static void osif_assoc_complete_csl (os_handle_t osif, IEEE80211_STATUS status, u_int16_t aid, wbuf_t wbuf)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_ASSOC_COMPLETE_EVENT");
}

static void osif_reassoc_complete_csl (os_handle_t osif, IEEE80211_STATUS status, u_int16_t aid, wbuf_t wbuf)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_REASSOC_COMPLETE_EVENT");
}

static void osif_deauth_complete_csl (os_handle_t osif, u_int8_t *macaddr, IEEE80211_STATUS status, u_int16_t associd, u_int16_t reason, u_int8_t *mldaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_DEAUTH_COMPLETE_EVENT");
}

static void osif_disassoc_complete_csl (os_handle_t osif, u_int8_t *macaddr,
                                        u_int32_t reason,
                                        IEEE80211_STATUS status,
                                        u_int8_t *mldaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_DISASSOC_COMPLETE_EVENT");
}

static void osif_txchanswitch_complete_csl (os_handle_t osif, IEEE80211_STATUS status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_TXCHANSWITCH_COMPLETE_EVENT");
}

static void osif_repeater_cac_complete_csl (os_handle_t osif, IEEE80211_STATUS status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_REPEATER_CAC_COMPLETE_EVENT");
}

static void osif_auth_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int16_t result)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_AUTH_INDICATION_EVENT");
}

static void osif_deauth_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int16_t associd, u_int16_t reason_code, u_int8_t *mldaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_DEAUTH_INDICATION_EVENT");
}

static void osif_assoc_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int16_t result, wbuf_t wbuf, wbuf_t resp_wbuf, bool reassoc, u_int8_t *mldaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_ASSOC_INDICATION_EVENT");
}

static void osif_reassoc_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int16_t result, wbuf_t wbuf, wbuf_t resp_wbuf, bool reassoc, u_int8_t *mldaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_REASSOC_INDICATION_EVENT");
}

static void osif_disassoc_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int16_t associd, u_int32_t reason_code, u_int8_t *mldaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_DISASSOC_INDICATION_EVENT");
}

static void osif_ibss_merge_start_indication_csl (os_handle_t osif, u_int8_t *bssid)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_IBSS_MERGE_START_INDICATION_EVENT");
}

static void osif_ibss_merge_completion_indication_csl (os_handle_t osif, u_int8_t *bssid)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_IBSS_MERGE_COMPLETION_INDICATION_EVENT");
}

static void osif_radar_detected_csl (os_handle_t osif, u_int32_t csa_delay)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_RADAR_DETECTED_EVENT");
}

static void osif_node_authorized_indication_csl (os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_NODE_AUTHORIZED_INDICATION_EVENT");
}

static void osif_unprotected_deauth_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int16_t associd, u_int16_t reason_code, wbuf_t wbuf)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MLME_UNPROTECTED_DEAUTH_INDICATION_EVENT");
}

static void osif_channel_change_csl (os_handle_t osif, wlan_chan_t chan)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "CHANNEL_CHANGE_EVENT");
}

static void osif_country_changed_csl (os_handle_t osif, char *country)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "COUNTRY_CHANGED_EVENT");
}

static void osif_linkspeed_csl (os_handle_t osif, u_int32_t rxlinkspeed, u_int32_t txlinkspeed)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "LINKSPEED_EVENT");
}

static void osif_michael_failure_indication_csl (os_handle_t osif,
        const u_int8_t *ta_mac_addr, u_int keyix)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MICHAEL_FAILURE_INDICATION_EVENT");
}

static void osif_replay_failure_indication_csl (os_handle_t osif, const u_int8_t *frm, u_int keyix)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "REPLAY_FAILURE_INDICATION_EVENT");
}

static void osif_beacon_miss_indication_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "BEACON_MISS_INDICATION_EVENT");
}

static void osif_beacon_rssi_indication_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "BEACON_RSSI_INDICATION_EVENT");
}

static void osif_device_error_indication_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "DEVICE_ERROR_INDICATION_EVENT");
}

static void osif_sta_clonemac_indication_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "STA_CLONEMAC_INDICATION_EVENT");
}

static void osif_sta_scan_entry_update_csl (os_handle_t osif, wlan_scan_entry_t scan_entry, bool flag)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "STA_SCAN_ENTRY_UPDATE_EVENT");
}

static void osif_ap_stopped_csl (os_handle_t osif, ieee80211_ap_stopped_reason reason)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "AP_STOPPED_EVENT");
}

#if ATH_SUPPORT_WAPI

static void osif_sta_rekey_indication_csl (os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "STA_REKEY_INDICATION_EVENT");
}
#endif

#if UMAC_SUPPORT_RRM_MISC

static void osif_channel_load_csl (os_handle_t osif, u_int8_t chload)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "CHANNEL_LOAD_EVENT");
}

static void osif_nonerpcnt_csl (os_handle_t osif, u_int8_t erpcnt)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "NONERPCNT_EVENT");
}

static void osif_bgjoin_csl (os_handle_t osif, u_int8_t val)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "BGJOIN_EVENT");
}

static void osif_cochannelap_cnt_csl (os_handle_t osif, u_int8_t val)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "COCHANNELAP_CNT_EVENT");
}
#endif

#if QCA_SUPPORT_SON

static void osif_buffull_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "BUFFULL_EVENT");
}
#endif

static void osif_session_timeout_csl (os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "SESSION_TIMEOUT_EVENT");
}

#if ATH_SUPPORT_MGMT_TX_STATUS

static void osif_mgmt_tx_status_csl (os_handle_t osif, IEEE80211_STATUS status, wbuf_t wbuf)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "MGMT_TX_STATUS_EVENT");
}
#endif

#if ATH_BAND_STEERING

static void osif_bsteering_event_csl (os_handle_t osif, ATH_BSTEERING_EVENT type, uint32_t eventlen, const char *data)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "BSTEERING_EVENT");
}
#endif

#if QCA_SUPPORT_SSID_STEERING

static void osif_ssid_event_csl (os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "SSID_EVENT");
}
#endif


static void osif_assocdeny_event_csl (os_handle_t osif, u_int8_t *macaddr)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "ASSOCDENY_EVENT");
}


static void osif_tx_overflow_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "TX_OVERFLOW_EVENT");
}

static void osif_ch_hop_channel_change_csl (os_handle_t osif, u_int8_t channel)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "CH_HOP_CHANNEL_CHANGE_EVENT");
}

static void osif_recv_probereq_csl (os_handle_t osif, u_int8_t *macaddr, u_int8_t *ssid_element)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note_mac(vap, IEEE80211_MSG_MLME, macaddr, IEEE80211_MSG_CSL_STR "RECV_PROBEREQ_EVENT");
}

static void osif_recv_neighreq_csl(os_handle_t osif, u_int8_t *macaddr, void *req)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "NEIGH REQ RECV");
}

static void osif_recv_beaconrpt_csl(os_handle_t osif, void *arg, uint32_t len)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "RECV_BEACONRPT_EVENT");
}

static void osif_recv_bstm_csl (os_handle_t osif, void *arg, uint32_t len)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "RECV_BSTMRPT_EVENT");
}

static void osif_recv_awgninterferenceupdate_csl (os_handle_t osif, void *arg, uint32_t len)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "RECV_AWGNINTERFERENCEUPDATE_EVENT");
}

static void osif_recv_smpsupdate_csl (os_handle_t osif, void *arg, uint32_t len)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "RECV_SMPSUPDATE_EVENT");
}

static void osif_recv_opmodeupdate_csl (os_handle_t osif, void *arg, uint32_t len)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "RECV_OPMODEUPDATE_EVENT");
}

#if DBDC_REPEATER_SUPPORT

static void osif_stavap_connection_csl (os_handle_t osif)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "STAVAP_CONNECTION_EVENT");
}
#endif

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS

static void osif_sta_cac_started_csl (os_handle_t osif, u_int32_t frequency, u_int32_t timeout)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "STA_CAC_STARTED_EVENT");
}
#endif

static void osif_chan_util_event_csl (os_handle_t osif, u_int32_t self_util, u_int32_t obss_util)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "CHAN_UTIL_EVENT");
}

static void osif_keyset_done_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int8_t status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "KEYSET_DONE_INDICATION_EVENT");
}

static void osif_blklst_sta_auth_indication_csl (os_handle_t osif, u_int8_t *macaddr, u_int8_t status)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "BLKLST_STA_AUTH_INDICATION_EVENT");
}

#if ATH_PARAMETER_API

static void osif_papi_event_csl (os_handle_t osif, PAPI_REPORT_TYPE type, uint32_t eventlen, const char *data)
{
    osif_dev *osifp = (osif_dev *)osif;
    wlan_if_t vap = osifp->os_if;
    ieee80211_note(vap, IEEE80211_MSG_MLME, IEEE80211_MSG_CSL_STR "PARAMETER_API_EVENT");
}
#endif

static wlan_mlme_event_handler_table sta_mlme_evt_handler = {
    NULL,                           /* mlme_join_complete_set_country */
    osif_join_complete_infra_sta,
    NULL,
    osif_auth_complete_sta,
    NULL,
    osif_assoc_complete_sta,
    osif_reassoc_complete_sta,
    osif_deauth_complete_sta,
    osif_disassoc_complete_sta,
    NULL,                           /* mlme_txchanswitch_complete */
    NULL,                           /* mlme_repeater_cac_complete  */
    osif_auth_indication_sta,
    osif_deauth_indication_sta,
    osif_assoc_indication_sta,
    osif_assoc_indication_sta,
    osif_disassoc_indication_sta,
    NULL,
    NULL,
    NULL,                             /* wlan_radar_detected */
    osif_authorized_indication_sta,   /* wlan_node_authorized_indication */
    osif_unprotected_deauth_indication_sta,   /* unprotected deauth indication */
};

static wlan_mlme_event_handler_table ap_mlme_evt_handler = {
    NULL,                                  /* mlme_join_complete_set_country */
	osif_create_infra_complete,
    NULL,
    osif_auth_complete_ap,
    NULL,
    osif_assoc_complete_ap,
    osif_assoc_complete_ap,
    osif_deauth_complete_ap,
    osif_disassoc_complete_ap,
    NULL,                                 /* mlme_txchanswitch_complete */
    NULL,                                 /* mlme_repeater_cac_complete */
    osif_auth_indication_ap,
    osif_deauth_indication_ap,
    osif_assoc_indication_ap,
    osif_assoc_indication_ap,
    osif_leave_indication_ap,
    NULL,
    NULL,
    NULL,
    osif_node_authorized_indication_ap,   /* wlan_node_authorized_indication */
    NULL,                                 /* unprotected deauth indication */
} ;

static wlan_misc_event_handler_table ap_misc_evt_handler = {
    osif_channel_change,
    osif_country_changed,
    osif_linkspeed,
    osif_michael_failure_indication,
    osif_replay_failure_indication,
    NULL,
    NULL,                               /* wlan_beacon_rssi_indication */
    NULL,
    NULL,
    NULL,
    NULL,                               /* wlan_ap_stopped */
#if ATH_SUPPORT_WAPI
    osif_rekey_indication_ap,
#endif
#if UMAC_SUPPORT_RRM_MISC
    osif_chload,
    osif_nonerpcnt,
    osif_bgjoin,
    osif_cochannelap_cnt,
#endif
#if QCA_SUPPORT_SON
    NULL,                                     /* wlan_buffull */
#endif
    osif_session_timeout,                   /* wlan_session_timeout */
#if ATH_SUPPORT_MGMT_TX_STATUS
   osif_mgmt_tx_status,
#endif
#if QCA_SUPPORT_SSID_STEERING
    osif_ssid_event,   /* ssid_event , void (*ssid_event)(os_handle_t, u_int8_t *macaddr); */
#endif
    osif_assocdeny_event,   /* assocdeny_event */
    osif_wlan_tx_overflow_event,            /* wlan_tx_overflow */
    osif_ch_hop_channel_change , /*wlan_ch_hop_channel_change*/
    osif_recv_probereq,     /*  wlan_recv_probereq */
#if DBDC_REPEATER_SUPPORT
    NULL,                /*stavap_connection*/
#endif
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    NULL,                 /* wlan_sta_cac_started */
#endif
    NULL,
    osif_keyset_done_indication,/*keyset_done_indication*/
    osif_blklst_sta_auth_indication, /*blklst_sta_auth_indication*/
#if ATH_PARAMETER_API
    osif_papi_event, /* papi_event ,  void (*papi_event)(os_handle_t,enum,char eventlen,char *data); */
#endif
    NULL,
    osif_recv_neighreq, /* wlan_recv_neighreq */
    osif_recv_beaconrpt, /* wlan_recv_beaconrpt */
    osif_recv_bstm, /* wlan_recv_bstm */
    osif_recv_smpsupdate, /*wlan_recv_smpsupdate */
    osif_recv_opmodeupdate, /*wlan_recv_opmodeupdate */
    osif_vdev_state_up_active_indication, /* wlan_vdev_state_up_active_indication */
    osif_recv_awgninterferenceupdate,
    osif_channel_switch_started, /* wlan_channel_switch_started */
};

static void osif_tx_lock(osif_dev *osifp)
{
    spin_lock_bh(&osifp->tx_lock);
}

static void osif_tx_unlock(osif_dev *osifp)
{
    spin_unlock_bh(&osifp->tx_lock);
}

static wlan_mlme_event_handler_table csl_mlme_evt_handler = {
    NULL,                           /* mlme_join_complete_set_country */
    osif_join_complete_infra_csl,
    osif_join_complete_adhoc_csl,
    osif_auth_complete_csl,
    osif_assoc_req_csl,
    osif_assoc_complete_csl,
    osif_reassoc_complete_csl,
    osif_deauth_complete_csl,
    osif_disassoc_complete_csl,
    osif_txchanswitch_complete_csl,
    osif_repeater_cac_complete_csl,
    osif_auth_indication_csl,
    osif_deauth_indication_csl,
    osif_assoc_indication_csl,
    osif_reassoc_indication_csl,
    osif_disassoc_indication_csl,
    osif_ibss_merge_start_indication_csl,
    osif_ibss_merge_completion_indication_csl,
    osif_radar_detected_csl,
    osif_node_authorized_indication_csl,
    osif_unprotected_deauth_indication_csl,
};

static wlan_misc_event_handler_table csl_misc_evt_handler = {
    osif_channel_change_csl,
    osif_country_changed_csl,
    osif_linkspeed_csl,
    osif_michael_failure_indication_csl,
    osif_replay_failure_indication_csl,
    osif_beacon_miss_indication_csl,
    osif_beacon_rssi_indication_csl,
    osif_device_error_indication_csl,
    osif_sta_clonemac_indication_csl,
    osif_sta_scan_entry_update_csl,
    osif_ap_stopped_csl,
#if ATH_SUPPORT_WAPI
    osif_sta_rekey_indication_csl,
#endif
#if UMAC_SUPPORT_RRM_MISC
    osif_channel_load_csl,
    osif_nonerpcnt_csl,
    osif_bgjoin_csl,
    osif_cochannelap_cnt_csl,
#endif
#if QCA_SUPPORT_SON
    osif_buffull_csl,
#endif
    osif_session_timeout_csl,
#if ATH_SUPPORT_MGMT_TX_STATUS
    osif_mgmt_tx_status_csl,
#endif
#if ATH_BAND_STEERING
    osif_bsteering_event_csl,
#endif
#if QCA_SUPPORT_SSID_STEERING
    osif_ssid_event_csl,
#endif
    osif_assocdeny_event_csl,
    osif_tx_overflow_csl,
    osif_ch_hop_channel_change_csl,
    osif_recv_probereq_csl,
#if DBDC_REPEATER_SUPPORT
    osif_stavap_connection_csl,
#endif
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    osif_sta_cac_started_csl,
#endif
    osif_chan_util_event_csl,
    osif_keyset_done_indication_csl,
    osif_blklst_sta_auth_indication_csl,
#if ATH_PARAMETER_API
    osif_papi_event_csl,
#endif
    NULL,
    osif_recv_neighreq_csl,
    osif_recv_beaconrpt_csl,
    osif_recv_bstm_csl,
    osif_recv_smpsupdate_csl,
    osif_recv_opmodeupdate_csl,
    NULL, /* wlan_vdev_state_up_active_indication */
    osif_recv_awgninterferenceupdate_csl,
    NULL,
};

static wlan_misc_event_handler_table csl_probe_evt_handler = {0};

void wlan_csl_enable(struct ieee80211vap *vap, int csl_val)
{
    u_int64_t old = wlan_get_debug_flags(vap);
    csl_probe_evt_handler.wlan_recv_probereq = osif_recv_probereq_csl;

    if (csl_val & LOG_CSL_BASIC) {
        wlan_set_debug_flags (vap, old | (IEEE80211_MSG_CSL));
    } else {
        wlan_set_debug_flags (vap, old & ~(IEEE80211_MSG_CSL));
    }

    if (csl_val & LOG_CSL_MLME_EVENTS) {
        wlan_vap_register_mlme_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_mlme_evt_handler);
    } else {
        wlan_vap_unregister_mlme_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_mlme_evt_handler);
    }

    if (csl_val & LOG_CSL_MISC_EVENTS) {
        wlan_vap_unregister_misc_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_probe_evt_handler);
        wlan_vap_register_misc_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_misc_evt_handler);
    } else {
        wlan_vap_unregister_misc_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_misc_evt_handler);

        /* Requirement is to log probe requests (misc event) when CSL config MLME is enabled
         * register csl_probe_evt_handler for only probe requests
         * in case CSL MISC config is disabled and CSL MLME config is enabled*/
        if (csl_val & LOG_CSL_MLME_EVENTS) {
            wlan_vap_register_misc_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_probe_evt_handler);
        } else {
            wlan_vap_unregister_misc_event_handlers (vap, (os_handle_t)vap->iv_ifp, &csl_probe_evt_handler);
        }
    }

    vap->iv_csl_support = csl_val;
}

static void osif_vap_setup(wlan_if_t vap, osif_dev *osifp,
                           enum ieee80211_opmode opmode)
{
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    cm_ext_t *cm_ext_handle = NULL;

    if (!dev) {
        qdf_err("netdev NULL");
        return;
    }

    osifp->osif_is_mode_offload = 1;
    vap->iv_netdev_name = dev->name;

#if QCA_LTEU_SUPPORT
    if (vap->iv_ic->ic_nl_handle)
        ieee80211_nl_register_handler(vap, osif_mu_report, osif_nl_scan_evhandler);
#endif

    ieee80211_ic_doth_set(vap->iv_ic);
    switch(opmode) {
    case IEEE80211_M_STA:
    case IEEE80211_M_P2P_CLIENT:
        wlan_vap_set_registered_handle(vap,(os_if_t)osifp);
        wlan_vap_register_event_handlers(vap,&common_evt_handler);
        wlan_vap_register_mlme_event_handlers(vap,(os_handle_t)osifp,&sta_mlme_evt_handler);
        wlan_vap_register_misc_event_handlers(vap,(os_handle_t)osifp,&sta_misc_evt_handler);
        wlan_mlme_cm_event_init(vap);
#if SM_ENG_HIST_ENABLE
        wlan_mlme_cm_action_history_init(vap);
#endif
        cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);
        if (!cm_ext_handle)
            QDF_ASSERT(0);
        else {
            cm_ext_handle->assoc_sm_handle = wlan_assoc_sm_create(osifp->os_handle, vap);
            if (!cm_ext_handle->assoc_sm_handle) {
                QDF_ASSERT(0);
                return;
            }
        }

#ifdef ATH_SUPPORT_P2P
        if (osifp->p2p_client_handle ) {
            wlan_p2p_client_register_event_handlers(osifp->p2p_client_handle, (void *)osifp, osif_p2p_dev_event_handler);
        }
#endif
        wlan_cm_set_max_connect_timeout(vap->vdev_obj, 60000);
        ieee80211_ic_2g_csa_set(vap->iv_ic);
#if ATH_SUPPORT_WRAP
        dp_wrap_register_xmit_handler(osifp->ctrl_vdev, &wlan_wrap_vap_xmit_queue);
#endif
        break;

    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_P2P_GO:
        wlan_vap_set_registered_handle(vap,(os_if_t)osifp);
        wlan_vap_register_event_handlers(vap,&common_evt_handler);
        wlan_vap_register_mlme_event_handlers(vap,(os_handle_t)osifp,&ap_mlme_evt_handler);
        wlan_vap_register_misc_event_handlers(vap,(os_handle_t)osifp,&ap_misc_evt_handler);
#ifdef ATH_SUPPORT_P2P
        if (osifp->p2p_go_handle) {
            wlan_p2p_GO_register_event_handlers(osifp->p2p_go_handle, (void *)osifp, osif_p2p_dev_event_handler);
        }
#endif
#if ATH_SUPPORT_WRAP
        dp_wrap_register_xmit_handler(osifp->ctrl_vdev, &wlan_wrap_vap_xmit_queue);
#endif
        break;
#ifdef ATH_SUPPORT_P2P
    case IEEE80211_M_P2P_DEVICE:
        wlan_vap_set_registered_handle(vap,(os_if_t)osifp);
        wlan_vap_register_event_handlers(vap,&common_evt_handler);
        wlan_vap_register_mlme_event_handlers(vap,(os_handle_t)dev,&ap_mlme_evt_handler);
        wlan_vap_register_misc_event_handlers(vap,(os_handle_t)dev,&ap_misc_evt_handler);
        wlan_p2p_register_event_handlers(osifp->p2p_handle, osifp, osif_p2p_dev_event_handler);
        break;
#endif
    case IEEE80211_M_MONITOR:
        wlan_vap_set_registered_handle(vap,(os_if_t)osifp);
        wlan_vap_register_event_handlers(vap,&common_evt_handler);
        break;
    default:
        break;
    }

}

static void
osif_set_multicast_list(struct net_device *dev)
{
    osif_dev  *osifp = ath_netdev_priv(dev);
    struct net_device *parent = osifp->os_comdev;
    wlan_if_t vap = osifp->os_if;
    wlan_if_t tmpvap;
    struct ieee80211com *ic = vap->iv_ic;
    int flags = 0;

    if (ic->recovery_in_progress) {
        return;
    }

    if (dev->flags & IFF_PROMISC)
    {
        parent->flags |= IFF_PROMISC;
    } else {
        parent->flags &= ~IFF_PROMISC;
    }
    if (dev->flags & IFF_ALLMULTI)
    {
        parent->flags |= IFF_ALLMULTI;
	} else {
	    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            os_if_t handle = wlan_vap_get_registered_handle(tmpvap);
            if (handle) {
                struct net_device *tmpdev = ((osif_dev *)handle)->netdev;
                if (tmpdev) {
			        flags |= tmpdev->flags;
                }
            }
	}
		if(!(flags & IFF_ALLMULTI))
			parent->flags &= ~IFF_ALLMULTI;
	}

    /* XXX merge multicast list into parent device */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
    parent->netdev_ops->ndo_set_rx_mode(parent);
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
    parent->netdev_ops->ndo_set_multicast_list(parent);
#else
    parent->set_multicast_list(parent);
#endif
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
uint16_t
osif_select_queue(struct net_device *dev, struct sk_buff *skb,
                   struct net_device *sb_dev)
#else
static uint16_t
osif_select_queue(struct net_device *dev, struct sk_buff *skb,
                    void *accel_priv, select_queue_fallback_t fallback)
#endif
{
    uint16_t cpu = get_cpu();
    put_cpu();
    return cpu;
}

uint16_t osif_pick_tx_queue(struct net_device *dev, struct sk_buff *skb,
                             struct net_device *sb_dev)
{
    uint16_t queue_id;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
    queue_id = osif_select_queue(dev, skb, sb_dev);
#else
    queue_id = osif_select_queue(dev, skb, NULL, NULL);
#endif
    return queue_id;

}
qdf_export_symbol(osif_pick_tx_queue);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
/*
 * osif_nss_set_mtu()
 *	API for changing the MTU.
 */
static int32_t osif_nss_set_mtu(struct net_device *dev, int mtu)
{
    struct nss_ctx_instance *wifi_nss_ctx = NULL;
    nss_tx_status_t status;
    int32_t if_num;

    wifi_nss_ctx = nss_wifi_get_context();
    if (!wifi_nss_ctx) {
        qdf_print("NSS Context not found. Change MTU failed\n");
        return -ENOMEM;
    }

    if_num = nss_cmn_get_interface_number_by_dev(dev);
    if (if_num < 0) {
        qdf_print("%p: Invalid interface number\n", wifi_nss_ctx);
        return -ENODEV;
    }

   status = nss_if_change_mtu(wifi_nss_ctx, if_num, mtu);
   if (status != NSS_TX_SUCCESS) {
        qdf_print("%p: Unable to send the change MTU message to NSS: %u\n", wifi_nss_ctx, mtu);
        return -1;
    }

    return 0;
}

/*
 * osif_nss_set_mac_addr()
 *	API for changing the MAC address.
 */
static int32_t osif_nss_set_mac_addr(struct net_device *dev, uint8_t *mac_addr)
{
    struct nss_ctx_instance *wifi_nss_ctx = NULL;
    nss_tx_status_t status;
    uint32_t if_num;

    wifi_nss_ctx = nss_wifi_get_context();
    if (!wifi_nss_ctx) {
        qdf_print("NSS Context not found. PPE VP create failed\n");
        return -ENOMEM;
    }

    if_num = nss_cmn_get_interface_number_by_dev(dev);
    if (if_num < 0) {
        qdf_print("%p: Invalid interface number\n", wifi_nss_ctx);
        return -ENODEV;
    }

    status = nss_if_change_mac_addr(wifi_nss_ctx, if_num, mac_addr);
    if (status != NSS_TX_SUCCESS) {
        qdf_print("%p: Unable to send the change MAC address message to NSS\n", wifi_nss_ctx);
        return -1;
    }

    return 0;
}
#endif

static int
osif_change_mtu(struct net_device *dev, int mtu)
{
    osif_dev  *osdev = ath_netdev_priv(dev);
    wlan_if_t vap = osdev->os_if;

    if (!(IEEE80211_MTU_MIN < mtu && mtu < IEEE80211_MTU_MAX))
        return -EINVAL;
    dev->mtu = mtu;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    osif_nss_set_mtu(dev, mtu);
#endif

#if QCA_PPE_VP
    if(!osif_ppe_vp_set_mtu(dev, osdev->ppe_vp_num, mtu)) {
        return -EINVAL;
    }
#endif
    ieee80211_ucfg_setparam_util(vap, IEEE80211_PARAM_MAX_MTU_SIZE, mtu,
			             NULL, false, false);
    /* XXX coordinate with parent device */
    return 0;
}

static int
osif_change_mac_addr(struct net_device *dev, void *mac_addr)
{
    struct sockaddr *sock_addr = (struct sockaddr *)mac_addr;
#if QCA_PPE_VP
    osif_dev  *osdev = ath_netdev_priv(dev);
#endif

    netdev_dbg(dev, "AddrFamily: %d, %0x:%0x:%0x:%0x:%0x:%0x\n",
            sock_addr->sa_family, sock_addr->sa_data[0], sock_addr->sa_data[1],
            sock_addr->sa_data[2], sock_addr->sa_data[3], sock_addr->sa_data[4],
            sock_addr->sa_data[5]);

    memcpy(dev->dev_addr, sock_addr->sa_data, ETH_ALEN);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPOT
    osif_nss_set_mac_addr(dev, sock_addr->sa_data);
#endif

#if QCA_PPE_VP
    osif_ppe_vp_set_mac_addr(dev, osdev->ppe_vp_num, sock_addr->sa_data);
#endif

    /* XXX coordinate with parent device */
    return 0;
}

#ifdef QCA_SUPPORT_WDS_EXTENDED
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
static void
    osif_wds_ext_getstats(struct net_device *dev, struct rtnl_link_stats64* stats64)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static struct rtnl_link_stats64*
    osif_wds_ext_getstats(struct net_device *dev, struct rtnl_link_stats64* stats64)
#else
static struct net_device_stats *
    osif_wds_ext_getstats(struct net_device *dev)
#endif
{
    osif_peer_dev *osifp = ath_netdev_priv(dev);
    osif_dev  *osdev = NULL;
    wlan_if_t vap = NULL;
    struct ieee80211com    *ic = NULL;
    struct ieee80211req_sta_stats *peer_stats = NULL;
    struct ieee80211_nodestats *pstats;
    struct ieee80211_node *ni = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    struct net_device_stats *stats = &osifp->os_devstats;
    struct net_device_stats *stats64 = stats;
#else
    struct rtnl_link_stats64 *stats = &osifp->os_devstats;
#endif

    osdev = osifp->parent_osif;
    if (!osdev)
        goto getstats_fail;

    vap = osdev->os_if;
    if (!vap)
        goto getstats_fail;

    peer_stats = (struct ieee80211req_sta_stats *)qdf_mem_malloc(sizeof(struct ieee80211req_sta_stats));
    if (!peer_stats)
        goto getstats_fail;

    qdf_mem_zero(peer_stats, sizeof(struct ieee80211req_sta_stats));

    if (!vap || ((vap->iv_ic) && vap->iv_ic->recovery_in_progress)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
        memcpy(stats64, &osifp->os_devstats, sizeof(struct rtnl_link_stats64));
#endif
        goto getstats_fail;
    }

    ni = ieee80211_vap_find_node(vap, osifp->peer_mac_addr,
                                 WLAN_MLME_SB_ID);

    if (ni == NULL)
        goto getstats_fail;

    ic = vap->iv_ic;
#ifdef QCA_SUPPORT_CP_STATS
    if(wlan_update_peer_cp_stats(ni, &peer_stats->is_stats) != 0) {
       ieee80211_free_node(ni, WLAN_MLME_SB_ID);
       goto getstats_fail;
    }
#endif
    if (ic) {
        if (wlan_get_peer_dp_stats(ic,
                                   ni->peer_obj,
                                   &peer_stats->is_stats, NULL, false) != 0) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            goto getstats_fail;
        }
    }

    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    pstats = &peer_stats->is_stats;

    stats->rx_packets = pstats->ns_rx_data +
                        pstats->ns_rx_mgmt +
                        pstats->ns_rx_ctrl;
#if UMAC_SUPPORT_STA_STATS_ENHANCEMENT
    stats->rx_bytes = pstats->ns_rx_bytes;
#endif
    stats->rx_errors = pstats->ns_rx_decap +
                       pstats->ns_rx_decryptcrc +
                       pstats->ns_rx_tkipicv +
                       pstats->ns_rx_wepfail +
                       pstats->ns_rx_ccmpmic +
                       pstats->ns_rx_wpimic +
                       pstats->ns_rx_tkipmic;
    stats->rx_dropped = pstats->ns_rx_dup;
    stats->tx_packets = pstats->ns_tx_data;
#if UMAC_SUPPORT_STA_STATS_ENHANCEMENT
    stats->tx_bytes = pstats->ns_tx_bytes;
#endif
    stats->tx_errors = pstats->ns_is_tx_not_ok;
    stats->tx_dropped = pstats->ns_tx_discard;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    memcpy(stats64, stats, sizeof(struct rtnl_link_stats64));
#endif

getstats_fail:
    if (peer_stats)
        qdf_mem_free(peer_stats);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
    return;
#else
    return stats64;
#endif
}

static void
ieee80211_update_netdev_info(struct ieee80211vap *vap, struct net_device *dev)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *assoc_node = NULL;
    osif_peer_dev *osifp  = ath_netdev_priv(dev);
#ifdef WLAN_FEATURE_11BE_MLO
    bool flag = false;
#endif

    if (!ic)
        return;

    ni = ieee80211_find_node(ic, osifp->peer_mac_addr, WLAN_MLME_NB_ID);
    if (!ni)
        return;

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_peer_is_mlo(ni->peer_obj) && !wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {
        assoc_node = ieee80211_ucfg_get_mlo_assoc_link_node(ni->peer_obj->mlo_peer_ctx);
        if (!assoc_node)
            goto release_ref;

        flag = true;
    } else
#endif
    {
        assoc_node = ni;
    }

    assoc_node->is_wds_client = 1;
    qdf_mem_copy(assoc_node->netdev_mac_addr, dev->dev_addr, QDF_MAC_ADDR_SIZE);
    assoc_node->netdev_if_index = dev->ifindex;

#ifdef WLAN_FEATURE_11BE_MLO
    if (flag)
        wlan_objmgr_peer_release_ref(assoc_node->peer_obj, WLAN_MLME_NB_ID);
#endif

release_ref:
    ieee80211_free_node(ni, WLAN_MLME_NB_ID);
}

int osif_wds_ext_extended_dev_open(struct net_device *dev)
{
   osif_peer_dev *osifp  = ath_netdev_priv(dev);
   osif_dev *osif_parent = osifp->parent_osif;
   wlan_if_t vap = osif_parent->os_if;
   struct wlan_objmgr_psoc *psoc = NULL;
   ol_txrx_soc_handle soc_txrx_handle;
   struct net_device *comdev = osif_parent->os_comdev;
   struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211*)ath_netdev_priv(comdev);
   QDF_STATUS status;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   struct ieee80211com *ic = NULL;
#endif

   if (!vap) {
       qdf_err("vap is null");
       return -EINVAL;
   }

   psoc = wlan_vdev_get_psoc(vap->vdev_obj);
   if (!psoc) {
       qdf_err("psoc is null");
       return -EINVAL;
   }

   ieee80211_update_netdev_info(vap, dev);

#ifdef WLAN_SUPPORT_SCS
    ieee80211_update_scs_rules(vap, dev);
#endif

   soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
   status = cdp_wds_ext_set_peer_rx(
                           soc_txrx_handle,
                           wlan_vdev_get_id(vap->vdev_obj),
                           osifp->peer_mac_addr,
                           scn->soc->wds_ext_osif_rx,
                           (ol_osif_peer_handle )osifp);

   if (status == QDF_STATUS_E_ALREADY)
       qdf_err("WDS ext peer callback already set");

   dev->flags |= IFF_RUNNING;

   /*
    * Open the NSS interface.
    */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   ic = vap->iv_ic;
   if (ic && ic->nss_ext_vops && ic->nss_radio_ops->ic_osif_nss_wifili_peer_set_4addr_cfg) {
       status = ic->nss_radio_ops->ic_osif_nss_wifili_peer_set_4addr_cfg(scn, osifp->peer_mac_addr,
                                            wlan_vdev_get_id(vap->vdev_obj), osifp->nss_ifnum);
       if (status != QDF_STATUS_SUCCESS) {
           qdf_err("4address peer configuration failed");
           return 0;
       }
   }

   if (ic && ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_up) {
       ic->nss_ext_vops->ic_osif_nss_ext_vdev_up(osifp);
   }
#endif
   return 0;
}

static int
osif_wds_ext_dev_open(struct net_device *dev)
{
   return osif_wds_ext_extended_dev_open(dev);
}

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
int osif_mld_phy_wds_ext_dev_open(struct net_device *dev)
{
   osif_peer_dev *osifp  = ath_netdev_priv(dev);
   int ret = 0;

   if (!(dev->flags & IFF_RUNNING)) {
       /* stop the TX queue here
        * queue will be resumed after extended open */
       netif_tx_stop_all_queues(dev);
       dev->flags |= IFF_RUNNING;
   }

   /* parent_osif is not yet initilized */
   if (!osifp->parent_osif) {
       return 0;
   }

   ret = osif_wds_ext_extended_dev_open(dev);
   if (ret) {
       qdf_err("Error while extended init %s", dev->name);
       return ret;
   }
   /* start the TX queue */
   netif_tx_start_all_queues(dev);

   return 0;
}
#endif

static int
osif_wds_ext_dev_stop(struct net_device *dev)
{
   osif_peer_dev *osifp  = ath_netdev_priv(dev);
   osif_dev *osif_parent = NULL;
   wlan_if_t vap = NULL;
   struct wlan_objmgr_psoc *psoc = NULL;
   ol_txrx_soc_handle soc_txrx_handle;
   QDF_STATUS status;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   struct ieee80211com *ic = NULL;
#endif

   osif_parent = osifp->parent_osif;
   if (!osif_parent) {
       qdf_err("parent osif is null dev = %s", dev->name);
       return -EINVAL;
   }

   vap = osif_parent->os_if;
   if (!vap) {
       qdf_err("vap is null");
       return -EINVAL;
   }

   /*
    * Close the NSS interface.
    */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   ic = vap->iv_ic;
   if (ic && ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_down) {
       ic->nss_ext_vops->ic_osif_nss_ext_vdev_down(osifp);
   }
#endif

   psoc = wlan_vdev_get_psoc(vap->vdev_obj);
   if (!psoc) {
       qdf_err("psoc is null");
       return -EINVAL;
   }

   soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
   dev->flags &= ~IFF_RUNNING;
   status = cdp_wds_ext_set_peer_rx(
                           soc_txrx_handle,
                           wlan_vdev_get_id(vap->vdev_obj),
                           osifp->peer_mac_addr, NULL, NULL);
   if (status == QDF_STATUS_E_ALREADY)
       qdf_err("WDS ext peer callback already unset");

   return 0;
}

static int
osif_wds_ext_change_mtu(struct net_device *dev, int mtu)
{
#if QCA_PPE_VP
    osif_peer_dev *osdev = ath_netdev_priv(dev);
#endif

    if (!(IEEE80211_MTU_MIN < mtu && mtu < IEEE80211_MTU_MAX))
        return -EINVAL;
    dev->mtu = mtu;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    osif_nss_set_mtu(dev, mtu);
#endif

#if QCA_PPE_VP
    if(!osif_ppe_vp_ext_vap_set_mtu(osdev, mtu)) {
        return -EINVAL;
    }
#endif
    return 0;
}

static ssize_t
osif_wds_ext_peer_mac_addr_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
#define MAC_ADDR_CHAR 18
    struct net_device *net = to_net_dev(dev);
    osif_peer_dev *osifp  = ath_netdev_priv(net);
    uint8_t peer_mac_addr[QDF_MAC_ADDR_SIZE] = {0};
    char peer_mac_addr_str[MAC_ADDR_CHAR] = {0};
#ifdef WLAN_FEATURE_11BE_MLO
    wlan_if_t vap = NULL;
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;

    if (!buf)
        return 0;
    buf[0]='\0';

    vap = osifp->parent_osif->os_if;
    if (!vap) {
        qdf_err("vap is null");
        return 0;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is null");
        return 0;
    }

    ni = ieee80211_find_node(ic, osifp->peer_mac_addr, WLAN_MLME_SB_ID);
    if (!ni) {
        qdf_err("ni is null");
        return 0;
    }

    if (wlan_peer_is_mlo(ni->peer_obj)) {
        ml_peer = ni->peer_obj->mlo_peer_ctx;
        if (!ml_peer) {
            qdf_err("invalid ml peer for " QDF_MAC_ADDR_FMT,
                    QDF_MAC_ADDR_REF(osifp->peer_mac_addr));
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return 0;
        }
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);

        memcpy(peer_mac_addr, ml_peer->peer_mld_addr.bytes, QDF_MAC_ADDR_SIZE);
    }
    else
#endif
        memcpy(peer_mac_addr, osifp->peer_mac_addr, QDF_MAC_ADDR_SIZE);

    snprintf(peer_mac_addr_str, MAC_ADDR_CHAR, "%02x:%02x:%02x:%02x:%02x:%02x",
             peer_mac_addr[0], peer_mac_addr[1], peer_mac_addr[2],
             peer_mac_addr[3], peer_mac_addr[4], peer_mac_addr[5]);

    strlcpy(buf, peer_mac_addr_str, MAC_ADDR_CHAR);

    return strlen(buf);
}

static DEVICE_ATTR(peer_mac_addr, S_IRUGO, osif_wds_ext_peer_mac_addr_show, NULL);
static struct attribute *ext_device_attrs[] = {
    &dev_attr_peer_mac_addr.attr,
    NULL
};

static struct attribute_group ext_attr_group = {
    .attrs = ext_device_attrs,
};

static int osif_wds_ext_config_parent_param(struct net_device *dev,
                                            uint8_t *macaddr)
{
   osif_peer_dev *osifp  = ath_netdev_priv(dev);
   struct net_device *parent_dev = osifp->parent_netdev;
   osif_dev *osif_parent = osifp->parent_osif;
   wlan_if_t vap = osif_parent->os_if;
   struct wlan_objmgr_psoc *psoc = NULL;
   ol_txrx_soc_handle soc_txrx_handle;
   struct ieee80211com *ic = NULL;
   struct ieee80211_node *ni = NULL;
   struct wlan_objmgr_vdev *vdev = NULL;

   memcpy(osifp->peer_mac_addr, macaddr, QDF_MAC_ADDR_SIZE);

   if (!vap) {
       qdf_err("vap is null");
       return -EINVAL;
   }

   vdev = vap->vdev_obj;
   if (!vdev) {
       qdf_err("vdev is null");
       return -EINVAL;
   }

   if (wlan_vdev_mlme_is_mlo_vdev(vdev))
       memcpy(dev->dev_addr, vdev->vdev_mlme.mldaddr, QDF_MAC_ADDR_SIZE);
   else
       memcpy(dev->dev_addr, vap->iv_myaddr, QDF_MAC_ADDR_SIZE);

   ic = vap->iv_ic;
   if (!ic) {
       qdf_err("ic is null");
       return -EINVAL;
   }

   ni = ieee80211_find_node(ic, osifp->peer_mac_addr, WLAN_MLME_SB_ID);

   if (!ni) {
       qdf_err("ni is null");
       return -EINVAL;
   }

   if (!ni->peer_obj) {
       qdf_err("peer_obj is NULL... investigate!!");
       ieee80211_free_node(ni, WLAN_MLME_SB_ID);
       return -EINVAL;
   }

#ifdef WLAN_FEATURE_11BE_MLO
   if (wlan_peer_is_mlo(ni->peer_obj)) {
        struct wlan_mlo_peer_context *ml_peer = NULL;
        struct wlan_objmgr_peer *bridge_peer = NULL;
        struct wlan_objmgr_vdev *bridge_vdev = NULL;
        osif_dev *osif_bridge_vdev = NULL;
        struct ieee80211_node *bridge_ni = NULL;
        QDF_STATUS status;

        ml_peer = ni->peer_obj->mlo_peer_ctx;
        if (!ml_peer) {
            qdf_err("invalid ml peer for" QDF_MAC_ADDR_FMT,
                    QDF_MAC_ADDR_REF(osifp->peer_mac_addr));
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return -EINVAL;
        }

        bridge_peer = wlan_mlo_peer_get_bridge_peer(ml_peer);
        if (!bridge_peer)
             goto ext_dev_open;

        bridge_vdev = wlan_peer_get_vdev(bridge_peer);
        if (!bridge_vdev || !bridge_vdev->vdev_objmgr.mlo_bridge_vdev)
            goto ext_dev_open;

        bridge_ni = wlan_peer_get_mlme_ext_obj(bridge_peer);
        if (!bridge_ni) {
            qdf_err("bridge ni is null mac =" QDF_MAC_ADDR_FMT,
                    QDF_MAC_ADDR_REF(osifp->peer_mac_addr));
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return -EINVAL;
        }

        /* As bridge vap is unknown to the hostapd will be creating
         * Ext interface on partner link.
         * parent interface will be bridge vap link as it is a primary
         */
        vap = bridge_ni->ni_vap;
        ic = bridge_ni->ni_ic;

        /* release the reference of link peer */
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
        /* featch the ref to primary peer */
        ni = ieee80211_try_ref_node(bridge_ni, WLAN_MLME_SB_ID);
        if (!ni) {
            qdf_err("unable to get the peer ref mac =" QDF_MAC_ADDR_FMT,
                    QDF_MAC_ADDR_REF(osifp->peer_mac_addr));
            return -EINVAL;
        }

        memcpy(osifp->peer_mac_addr, bridge_ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
        osif_bridge_vdev = ((osif_dev *)vap->iv_ifp);
        osifp->parent_osif = osif_bridge_vdev;

        /* For a bridge vap if netdev is associated will hold netdev reference
         * else will be holding the vdev ref.
         */
        if (osif_bridge_vdev->netdev) {
            /* hold the reference of primary dev */
            osifp->parent_netdev = osif_bridge_vdev->netdev;
            qdf_net_if_hold_dev((struct qdf_net_if *)osifp->parent_netdev);
        } else {
            status = wlan_objmgr_vdev_try_get_ref(vap->vdev_obj, WLAN_OSIF_ID);
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("unable to get the vdev ref mac =" QDF_MAC_ADDR_FMT,
                        QDF_MAC_ADDR_REF(osifp->peer_mac_addr));
                ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                return -EINVAL;
            }

            osifp->parent_netdev = NULL;
            osifp->vdev = vap->vdev_obj;
        }
        /* release the ref to old parent dev held during the create */
        qdf_net_if_release_dev((struct qdf_net_if *)parent_dev);
    }
ext_dev_open:
#endif
   ic->ic_node_use_4addr(ni);

   if (ic->ic_node_wmi_send_wds_ext(ni)) {
       qdf_err("WMI send for node");
       ieee80211_free_node(ni, WLAN_MLME_SB_ID);
       return -EINVAL;
   }

#ifdef WLAN_FEATURE_11BE_MLO
    /* Set WDS_EXT MLO identifier , for MLO peers*/
    if (wlan_peer_is_mlo(ni->peer_obj)) {
        osifp->is_mlo = 1;
    }
#endif

   ieee80211_free_node(ni, WLAN_MLME_SB_ID);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   if (ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_change_mac_addr) {
       if (ic->nss_ext_vops->ic_osif_nss_ext_vdev_change_mac_addr(osifp, osifp->peer_mac_addr)
              != QDF_STATUS_SUCCESS) {
           qdf_err("wds vap mac setup failed");
           return -EINVAL;
       }
   }
#endif

   psoc = wlan_vdev_get_psoc(vap->vdev_obj);
   if (!psoc) {
       qdf_err("psoc is null");
       return -EINVAL;
   }

   soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
   osifp->peer_id = cdp_get_peer_id(soc_txrx_handle,
                                    wlan_vdev_get_id(vap->vdev_obj),
                                    osifp->peer_mac_addr);
   /*
    * Send message to the extended vap to configure wds peer.
    */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   if (ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_cfg_wds_peer) {
       if (ic->nss_ext_vops->ic_osif_nss_ext_vdev_cfg_wds_peer(osifp, ic)
           != QDF_STATUS_SUCCESS) {
           qdf_err("Could not setup nss wds peer");
       }
   }
#endif
#if QCA_PPE_VP
    osif_ppe_vp_ext_vap_set_mac_addr(osifp, osifp->peer_mac_addr);
#endif

    /*
     * Create the NSS redirect interface / PPE VP instance.
     */
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
#if QCA_PPE_VP
    osif_pltfrm_attach_ext_ppe_vp(osifp);
#endif
#else
#if QCA_NSS_PLATFORM || QCA_PPE_VP
    osif_pltfrm_create_ext_vap(osifp);
#endif
#endif

    /* set MTU configured for VAP to WDS ext interface */
    osif_wds_ext_change_mtu(dev, parent_dev->mtu);

    /* set sysfs file attr */
    if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj,
                (struct qdf_vfs_attr *)&ext_attr_group)) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                "%s: sysfs create group failed\n", __FUNCTION__);
    }
    return 0;
}

static int osif_wds_ext_change_mac(struct net_device *dev, void *p)
{
   struct sockaddr *addr = p;

   return osif_wds_ext_config_parent_param(dev, addr->sa_data);
}

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
static int osif_mld_phy_wds_ext_change_mac(struct net_device *dev, void *p)
{
   struct sockaddr *addr = p;

   memcpy(dev->dev_addr, addr->sa_data, QDF_MAC_ADDR_SIZE);

   return 0;
}

static inline struct ieee80211_node *
osif_find_node_from_parent_dev(struct net_device *parent_dev,
                               uint8_t *peer_mac_addr,
                               wlan_objmgr_ref_dbgid id)
{
   osif_dev *osif_parent = ath_netdev_priv(parent_dev);
   struct wlan_objmgr_peer *peer = NULL;
   struct ieee80211_node *ni = NULL;

   /* legacy vap */
   if (osif_parent->dev_type == OSIF_NETDEV_TYPE_VAP) {
       peer = wlan_objmgr_vdev_find_peer_by_mac(osif_parent->ctrl_vdev,
                                                peer_mac_addr,
                                                id);
       if (!peer) {
           qdf_err("unable to find the ni on legacy vap" QDF_MAC_ADDR_FMT,
                   QDF_MAC_ADDR_REF(peer_mac_addr));
           return NULL;
       }

       ni = wlan_peer_get_mlme_ext_obj(peer);
       if (!ni) {
           qdf_err("ni is NULL for peer on legacy vap" QDF_MAC_ADDR_FMT,
                   QDF_MAC_ADDR_REF(peer_mac_addr));
           wlan_objmgr_peer_release_ref(peer, id);
           return NULL;
       }

       return ni;
   } else if (osif_parent->dev_type == OSIF_NETDEV_TYPE_MLO) {
       struct osif_mldev *mldev = (struct osif_mldev *)osif_parent;
       osif_dev *osif_link = NULL;
       struct wlan_objmgr_vdev *vdev_obj = NULL;
       struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
       int i = 0;

       /* try finding the ml_peer peer */
       for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
           osif_link = mldev->link_vifs[i];
           if (osif_link)
               break;
       }
       vdev_obj = osif_link->ctrl_vdev;
       if (vdev_obj->mlo_dev_ctx) {
           /* search based on mld mac */
           mlo_peer_ctx = wlan_mlo_get_mlpeer_by_mld_mac(
                                     vdev_obj->mlo_dev_ctx,
                                     (struct qdf_mac_addr *)peer_mac_addr);
           if (!mlo_peer_ctx) {
               /* search based of link mac */
               mlo_peer_ctx = wlan_mlo_get_mlpeer_by_linkmac(
                                     vdev_obj->mlo_dev_ctx,
                                     (struct qdf_mac_addr *)peer_mac_addr);
           }
           if (mlo_peer_ctx) {
               peer = wlan_mlo_peer_get_primary_peer(mlo_peer_ctx);
               if (!peer) {
                   qdf_err("unable to get the primary peer on mld vap"
                           QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(peer_mac_addr));
                   return NULL;
               }

               ni = wlan_peer_get_mlme_ext_obj(peer);
               if (!ni) {
                   qdf_err("ni is NULL for peer found on mld vap"
                           QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(peer_mac_addr));
                   return NULL;
               }

               /* featch the ref to primary peer */
               ni = ieee80211_try_ref_node(ni, id);
               if (!ni) {
                   qdf_err("unable to get the ref to ni found on mld vap"
                   QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(peer_mac_addr));
                   return NULL;
               }

               return ni;
           }
       }

       /* try finding the legacy peer */
       for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
           osif_link = mldev->link_vifs[i];
           if (!osif_link)
               continue;

           peer = wlan_objmgr_vdev_find_peer_by_mac(osif_link->ctrl_vdev,
                                                    peer_mac_addr,
                                                    id);
           if (!peer) {
               continue;
           }

           ni = wlan_peer_get_mlme_ext_obj(peer);
           if (!ni) {
               qdf_err("ni is NULL for peer on legacy vap" QDF_MAC_ADDR_FMT,
                       QDF_MAC_ADDR_REF(peer_mac_addr));
               wlan_objmgr_peer_release_ref(peer, id);
               continue;
           }

           return ni;

       }
       qdf_err("unable to find the peer on mld vap" QDF_MAC_ADDR_FMT,
               QDF_MAC_ADDR_REF(peer_mac_addr));
   }

   return NULL;
}

int osif_mld_phy_configure_parent_param(struct net_device *dev, uint8_t *mac)
{
   osif_peer_dev *osifp  = ath_netdev_priv(dev);
   osif_dev *osif_parent = NULL;
   struct ieee80211_node *ni = NULL;
   wlan_if_t vap = NULL;
   int ret = 0;

   if (osifp->parent_osif) {
       qdf_err("parent osif is already configured");
       return -EINVAL;
   }

   /* find the node from parent netdev */
   ni = osif_find_node_from_parent_dev(osifp->parent_netdev,
                                       mac, WLAN_MLME_SB_ID);
   if (!ni) {
       qdf_err("unable to find the ni" QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(mac));
       return -EINVAL;
   }

   vap = ni->ni_vap;
   if (!vap ) {
       qdf_err("vap is null ni = %px" QDF_MAC_ADDR_FMT,
               ni, QDF_MAC_ADDR_REF(mac));
       ieee80211_free_node(ni, WLAN_MLME_SB_ID);
       return -EINVAL;
   }

   osif_parent = (osif_dev *)vap->iv_ifp;
   if (!osif_parent) {
       qdf_err("osif_parent is NULL" QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(mac));
       ieee80211_free_node(ni, WLAN_MLME_SB_ID);
       return -EINVAL;
   }

   /* update parent osif here till this point parent osif will be NULL */
   osifp->parent_osif = osif_parent;

   /* need to update the flags from comdev to wds_ext interface */

   /* send primary link mac address */
   ret = osif_wds_ext_config_parent_param(dev, ni->ni_macaddr);
   if (ret) {
       /* release the reference held during global search */
       ieee80211_free_node(ni, WLAN_MLME_SB_ID);
       qdf_err("error during the parent param config %s", dev->name);
       return ret;
   }
   /* release the reference held during global search */
   ieee80211_free_node(ni, WLAN_MLME_SB_ID);

   return osif_mld_phy_wds_ext_dev_open(dev);
}
#endif /* ENABLE_CFG80211_BACKPORTS_MLO */
#endif

wlan_if_t osif_get_vap(osif_dev *osifp)
{
    return osifp->os_if;
}
qdf_export_symbol(osif_get_vap);

static int os_if_cm_init(osif_dev *osdev)
{
    wlan_if_t vap = osdev->os_if;
    return osif_cm_osif_priv_init(vap->vdev_obj);
}

static int os_if_cm_deinit(osif_dev *osdev)
{
    wlan_if_t vap = osdev->os_if;
    return osif_cm_osif_priv_deinit(vap->vdev_obj);
}

void osif_recover_peer_release_handler(struct wlan_objmgr_psoc *psoc,
                                           void *obj, void *args)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_info("peer_release_handler: Null vap, vdev: 0x%pK", vdev);
        return;
    }

    if (vap->iv_ic->ic_node_release_ref)
        vap->iv_ic->ic_node_release_ref(vap, peer->macaddr);
}

QDF_STATUS osif_vap_wait_for_vdev_up_state(struct ieee80211vap *vap)
{
    QDF_STATUS status;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;

    if (wlan_vdev_mlme_get_state(vdev) == WLAN_VDEV_S_UP)
        return QDF_STATUS_SUCCESS;

    /*
     * Wait for the VDEV up event completion i.e. until the VDEV up event is
     * set by another function.
     */
    status = qdf_wait_for_event_completion(&vap->wait_event_vdev_up_state,
                                           OSIF_VAP_UP_TIMEOUT);
    if (status == QDF_STATUS_E_TIMEOUT) {
        wlan_mlme_err("Timedout waiting for vdev to reach up state");
        return QDF_STATUS_E_TIMEOUT;
    }

    if (wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_UP) {
        wlan_mlme_err("vdev is not in up state");
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}

void vap_wait_for_vdev_init_state(struct wlan_objmgr_psoc *psoc,
                                  struct wlan_objmgr_vdev *vdev,
                                  int recover)
{
    int waitcnt = 0;
    uint8_t psoc_id;
    qdf_event_t wait_event;
    enum wlan_serialization_cmd_type cmd_type;
    struct ieee80211vap *vap = NULL;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
       qdf_err("vap is NULL");
       goto fail;
    }

    qdf_mem_zero(&wait_event, sizeof(wait_event));
    qdf_event_create(&wait_event);
    qdf_event_reset(&wait_event);

    cmd_type = wlan_serialization_get_vdev_active_cmd_type(vdev);
    while (((wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_INIT) ||
            (cmd_type == WLAN_SER_CMD_VDEV_START_BSS)) &&
        waitcnt < OSIF_MAX_DELETE_VAP_INIT_TIMEOUT &&
        (recover || !vap->iv_ic->recovery_in_progress)) {
        qdf_wait_single_event(&wait_event, 1000);
        waitcnt++;
        cmd_type = wlan_serialization_get_vdev_active_cmd_type(vdev);
    }
    qdf_event_destroy(&wait_event);

    /* Handle case where before delete response received, target is asserted
       and recovery is enabled, in such case, recovery will hold rtnl lock is freed,
       and above wait will run till response received, hence to avoid the same, wait
       only if recovery is not in progress, and handle the recovery case below */
    if (!recover && vap->iv_ic->recovery_in_progress) {
        wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_STOP_RESP,
                                       0, NULL);
        qdf_mem_zero(&wait_event, sizeof(wait_event));
        qdf_event_create(&wait_event);
        qdf_event_reset(&wait_event);
        cmd_type = wlan_serialization_get_vdev_active_cmd_type(vdev);

        while (((wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_INIT) ||
                (cmd_type == WLAN_SER_CMD_VDEV_START_BSS)) &&
                 waitcnt < OSIF_MAX_DELETE_VAP_INIT_TIMEOUT) {
            qdf_wait_single_event(&wait_event, 1000);
            waitcnt++;
            cmd_type = wlan_serialization_get_vdev_active_cmd_type(vdev);
        }
        qdf_event_destroy(&wait_event);
    }

fail:
    if (wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_INIT) {
        wlan_mlme_err("Timeout: vdev:%u SM state %d, subsate:%d",
                      wlan_vdev_get_id(vdev), wlan_vdev_mlme_get_state(vdev),
                      wlan_vdev_mlme_get_substate(vdev));
        psoc_id = wlan_psoc_get_id(psoc);
        wlan_mlme_print_vdev_sm_history(psoc, vdev, &psoc_id);
        wlan_ser_print_history(vdev, WLAN_SER_CMD_NONSCAN, WLAN_SER_CMD_NONSCAN);
        wlan_objmgr_psoc_check_for_peer_leaks(psoc);
        QDF_BUG(0);
    }
}

void vap_wait_for_peer_delete(struct wlan_objmgr_psoc *psoc,
                              struct wlan_objmgr_vdev *vdev,
                              int recover)
{
    int waitcnt = 0;
    qdf_event_t wait_event;
    struct ieee80211vap *vap = NULL;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
       qdf_err("vap is NULL");
       goto fail;
    }

    qdf_mem_zero(&wait_event, sizeof(wait_event));
    qdf_event_create(&wait_event);
    qdf_event_reset(&wait_event);

    /* Wait for Peers to be deleted */
    while (wlan_vdev_get_peer_count(vdev) &&
           waitcnt < OSIF_MAX_DELETE_VAP_TIMEOUT &&
           (recover || !vap->iv_ic->recovery_in_progress)) {
        qdf_wait_single_event(&wait_event, 1000);
        waitcnt++;
    }
    qdf_event_destroy(&wait_event);

    if (!recover && vap->iv_ic->recovery_in_progress) {
        /* release references held for pending peer delete response first */
        wlan_objmgr_iterate_obj_list_all(psoc, WLAN_PEER_OP,
              osif_recover_peer_release_handler, NULL, 1,
              WLAN_MLME_NB_ID);

        qdf_mem_zero(&wait_event, sizeof(wait_event));
        qdf_event_create(&wait_event);
        qdf_event_reset(&wait_event);

        while (wlan_vdev_get_peer_count(vdev) &&
               waitcnt < OSIF_MAX_DELETE_VAP_TIMEOUT) {
               qdf_wait_single_event(&wait_event, 1000);
               waitcnt++;
        }
        qdf_event_destroy(&wait_event);
    }

fail:
    if (wlan_vdev_get_peer_count(vdev)) {
        wlan_mlme_err("Peers(cnt %d) are not freed for vdev:%d",
                      wlan_vdev_get_peer_count(vdev), wlan_vdev_get_id(vdev));
        wlan_objmgr_psoc_check_for_peer_leaks(psoc);
        QDF_BUG(0);
    }
}

void vdev_wait_for_mld_disconnection(struct wlan_objmgr_vdev *vdev)
{
    int waitcnt = 0;
    qdf_event_t wait_event;

    qdf_mem_zero(&wait_event, sizeof(wait_event));
    qdf_event_create(&wait_event);
    qdf_event_reset(&wait_event);

    while (!ucfg_mlo_is_mld_disconnected(vdev) &&
           waitcnt < OSIF_MAX_DELETE_VAP_INIT_TIMEOUT) {
        qdf_wait_single_event(&wait_event, 1000);
        waitcnt++;
    }

    qdf_event_destroy(&wait_event);
    if (!ucfg_mlo_is_mld_disconnected(vdev)) {
        wlan_mlme_err("MLD disconnection failed for vdev:%d",
                      wlan_vdev_get_id(vdev));
        QDF_BUG(0);
    }
}

static void osif_wlan_pre_soc_stop(struct wlan_objmgr_psoc *psoc, struct ieee80211com* ic)
{
    /*cancel ru_tolerance_timer (per pdev), if we are about to stop target */
    ic_cancel_obss_nb_ru_tolerance_timer(ic);
    ieee80211_regulatory_reset(ic);
#if ATH_SUPPORT_DFS
    ieee80211_dfs_reset(ic);
#endif
#if QCA_AIRTIME_FAIRNESS
    wlan_atf_pdev_reset(ic->ic_pdev_obj);
#endif /* QCA_AIRTIME_FAIRNESS */
}

static void osif_wlan_post_soc_stop(struct wlan_objmgr_psoc *psoc,struct ieee80211com* ic)
{
    struct wlan_objmgr_pdev *pdev = ic->ic_pdev_obj;

    ic_reset_params(ic);
    wlan_rptr_reset_flags(pdev);
    global_ic_list_reset_params(ic);
    if (ic->ic_is_target_lithium(psoc)) {
        ieee80211_user_rnr_free(ic);
    }
}

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
static void osif_wlan_stop_pdev_iterate(struct wlan_objmgr_psoc *psoc,
                                        void *object,
                                        void *arg)
{
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)object;
    struct ieee80211com* ic;
    bool pre_soc_stop = *(bool *)arg;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("Pdev in inconsistent state");
        return;
    }

    if (pre_soc_stop) {
        osif_wlan_pre_soc_stop(psoc, ic);
    } else {
        osif_wlan_post_soc_stop(psoc, ic);
    }
}

static void
osif_wlan_stop_psoc_iterate_list(struct ieee80211com* ic)
{
    bool pre_stop = true;
    uint8_t index = 0;
    struct wlan_objmgr_psoc *psoc;

    while (index < WLAN_OBJMGR_MAX_DEVICES) {
        if (g_umac_glb_obj->psoc[index]) {
            psoc = g_umac_glb_obj->psoc[index];
            if (ic->ic_mlo_capable(psoc)) {
                pre_stop = true;
                wlan_objmgr_iterate_obj_list(psoc, WLAN_PDEV_OP,
                        osif_wlan_stop_pdev_iterate,
                        &pre_stop,
                        false, WLAN_MLME_NB_ID);

#ifndef QCA_SINGLE_WIFI_3_0
                ic->ic_stop(psoc, 1);
#endif

                pre_stop = false;
                wlan_objmgr_iterate_obj_list(psoc, WLAN_PDEV_OP,
                        osif_wlan_stop_pdev_iterate,
                        &pre_stop,
                        false, WLAN_MLME_NB_ID);

            }
        }
        index++;
    }
}
#endif

static void wlan_soc_vap_count(struct wlan_objmgr_psoc *psoc, void *obj, void *args)
{
    int *vap_count = NULL;

    vap_count = (int *) args;
    (*vap_count)++;
}

static void osif_wlan_stop(struct ieee80211com* ic)
{
    int vap_count = 0;
    struct wlan_objmgr_psoc *psoc = NULL;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    struct global_ic_list *glist = ic->ic_global_list;
    int i;
    struct ieee80211com* tmpic = NULL;
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_ath_soc_softc_t *soc = NULL;
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
#endif

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc)
        return;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
   scn = OL_ATH_SOFTC_NET80211(ic);
   soc = scn->soc;

   if (ic->ic_mlo_capable(psoc) &&
       (soc->standby_mode != WLAN_STANDBY_MODE_ACTIVE_ENTRY) &&
       (!soc->psoc_obj->wsi_remap_remove)) {
        for (i = 0; i < glist->num_global_ic; i++) {
              tmpic = glist->global_ic[i];
              if (!tmpic)
                  continue;
              psoc = wlan_pdev_get_psoc(tmpic->ic_pdev_obj);

              if (tmpic->ic_mlo_capable(psoc) && !TAILQ_EMPTY(&tmpic->ic_vaps))
                  return;
        }

        /* MLD socs needs to be stopped together,
           Hence, above check ensures all vaps are down in all the socs
           and then bring down the socs */
        osif_wlan_stop_psoc_iterate_list(ic);

        /* If the other chips are put to down state clear the flag post soc stop */
        if (mlo_ctx && mlo_ctx->dynamic_wsi_bypassed)
            mlo_ctx->dynamic_wsi_bypassed = false;

    } else
#endif
    {
        if (TAILQ_EMPTY(&ic->ic_vaps)) {
            wlan_objmgr_iterate_obj_list(psoc, WLAN_VDEV_OP,
                    wlan_soc_vap_count, &vap_count, 1,
                    WLAN_MLME_NB_ID);
            osif_wlan_pre_soc_stop(psoc, ic);
            /* Do soc stop only if no VAPs on SoC*/
            if(vap_count == 0) {
#ifndef QCA_SINGLE_WIFI_3_0
                ic->ic_stop(psoc, 1);
#endif
            }
            osif_wlan_post_soc_stop(psoc, ic);
        }
    }
}

int
wlan_delete_vap_wait_and_free(wlan_if_t vap, int recover)
{
    osif_dev *osnetdev = (osif_dev *)vap->iv_ifp;
    struct ieee80211com *ic;
    osif_dev  *osifp;
    int waitcnt = 0;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_objmgr_psoc *psoc;
    struct psoc_mlme_obj *psoc_mlme;
    struct vdev_response_timer *vdev_rsp;
    uint8_t vdev_id;
    qdf_event_t wait_event;
    wlan_assoc_sm_t assoc_sm_hdl;
#if QCN_IE
    qdf_hrtimer_data_t *t_bpr_timer = &vap->bpr_timer;
#endif

    qdf_mem_zero(&wait_event, sizeof(wait_event));

    osifp = (osif_dev *)vap->iv_ifp;
    vdev = osifp->ctrl_vdev;
    vdev_id = wlan_vdev_get_id(vdev);
    psoc = wlan_vdev_get_psoc(vdev);

    ic = wlan_vap_get_devhandle(vap);
    vap_wait_for_vdev_init_state(psoc, vdev, recover);
#if QCN_IE
   if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
       qdf_hrtimer_kill(t_bpr_timer);
    }
#endif
   wlan_vdev_mlme_ser_cancel_request(vdev,
                                     WLAN_SER_CMD_NONSCAN,
                                     WLAN_SER_CANCEL_VDEV_NON_SCAN_NB_CMD);

    /*
     * Wait until all non-scan active commands are also removed from the
     * serialization queue.
     */
    wlan_mlme_wait_for_cmd_completion(vdev);

    /*
     * Wait until all active and pending scan commands are also removed from
     * the serialization queue.
     */
    wlan_mlme_wait_for_scan_cmd_completion(vdev);

   if (mlo_is_mld_sta(vdev)) {
       vdev_wait_for_mld_disconnection(vdev);
   }

    wlan_vap_delete_complete(vap);
    if (vap->iv_bss) {
        /* always send individual peer delete for BSS peer */
        qdf_atomic_set(&(vap->iv_bss->ni_peer_del_req_enable), 1);
        ieee80211_sta_leave(vap->iv_bss);
    }

#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    wlan_hmwds_reset_table(vap);
#endif
    ieee80211_node_vdetach(vap);
    vap_wait_for_peer_delete(psoc, vdev, recover);

    if (vap->iv_opmode == IEEE80211_M_STA) {
        wlan_mlme_cm_event_deinit(vap);
#if SM_ENG_HIST_ENABLE
        wlan_mlme_cm_action_history_deinit(vap);
#endif
        assoc_sm_hdl = wlan_mlme_get_assoc_sm_handle(vdev);
        if (assoc_sm_hdl)
            wlan_assoc_sm_delete(assoc_sm_hdl);
    }

    os_if_cm_deinit(osifp);
    wlan_objmgr_delete_vap(vap);

    if (recover)
        return 0;

    waitcnt = 0;
    qdf_event_create(&wait_event);
    qdf_event_reset(&wait_event);

    while(!osifp->is_deleted && waitcnt < OSIF_MAX_DELETE_VAP_TIMEOUT) {
        qdf_wait_single_event(&wait_event, 1000);
        waitcnt++;
    }

    psoc_mlme = mlme_psoc_get_priv(psoc);
    vdev_rsp =  &psoc_mlme->psoc_vdev_rt[vdev_id];

    waitcnt = 0;
    while(qdf_atomic_test_bit(DELETE_RESPONSE_BIT, &vdev_rsp->rsp_status) &&
                    waitcnt < OSIF_MAX_DELETE_VAP_TIMEOUT &&
                    !ic->recovery_in_progress) {
        qdf_wait_single_event(&wait_event, 500);
        waitcnt++;

    }
    qdf_event_destroy(&wait_event);

    wlan_vap_set_registered_handle(osnetdev->os_if, NULL);
    osnetdev->os_if = NULL;

    /* If not set after waiting deinit vap from here*/
    if (!osifp->is_deleted) {
        msleep(2000);
        if (!osifp->is_deleted) {
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                    "WARNING: Waiting for VAP delete timedout!!! vdev id: %d\n", vap->iv_unit);
            if (vap->iv_ic->ic_print_peer_refs)
                vap->iv_ic->ic_print_peer_refs(vap->vdev_obj, false);
        }
    } else {
        ic->ic_vap_free(vap);
    }
    osnetdev->is_delete_in_progress = 0;
    osif_wlan_stop(ic);

    /* Clearing user set start aid value and flag which stored in IC when all vaps are deleted */
    if (TAILQ_EMPTY(&ic->ic_vaps) && ic->ic_is_start_aid_set) {
        ic->ic_is_start_aid_set = false;
        ic->ic_start_aid = -1;
        qdf_info("ic->ic_start_aid: %d, ic->ic_is_start_aid_set: %d", ic->ic_start_aid, ic->ic_is_start_aid_set);
    }
    return 0;
}
qdf_export_symbol(wlan_delete_vap_wait_and_free);

int
osif_delete_vap_wait_and_free(osif_dev *osnetdev, int recover)
{
    wlan_if_t vap = osnetdev->os_if;

    return wlan_delete_vap_wait_and_free(vap, recover);
}

int
osif_vap_roam(struct net_device *dev, wlan_if_t vap)
{
    osif_dev  *osifp = (osif_dev *)vap->iv_ifp;
    int waitcnt;
    int is_ap_cac_timer_running = 0;
    struct net_device *parent = osifp->os_comdev;
    enum ieee80211_opmode opmode;
    struct wlan_objmgr_vdev *vdev;
    QDF_STATUS status;
    ieee80211_ssid ssid;
    int desired_bssid = 0;

    opmode = wlan_vap_get_opmode(vap);
    if (opmode != IEEE80211_M_STA)
        return 0;

    vdev = osifp->ctrl_vdev;
    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_OSIF_SCAN_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        scan_info("unable to get reference");
        return -EBUSY;
    }

    if ((dev->flags & IFF_RUNNING) == 0) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
        return -EINVAL;
    }

    if (IS_UP(parent)) {
        osifp->os_if->iv_mlmeconnect=0;
        ssid.len=0;
        wlan_get_desired_ssidlist(vap,&ssid,1);
        desired_bssid = wlan_aplist_get_desired_bssid_count(vap);
        /* check if there is only one desired bssid and it is broadcast , ignore the setting if it is */
        if (desired_bssid == 1) {
            u_int8_t des_bssid[QDF_MAC_ADDR_SIZE];
            wlan_aplist_get_desired_bssidlist(vap, &des_bssid);
            if (IEEE80211_IS_BROADCAST(des_bssid)) {
                desired_bssid=0;
            }
        }
        if ((desired_bssid || ssid.len)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: desired bssid %d ssid len %d \n",__func__,
                    desired_bssid, ssid.len);

            /*If in TXCHANSWITCH then wait before roam */
            if (mlme_get_active_req_type(vap) == MLME_REQ_TXCSA) {
                schedule_timeout_interruptible(OSIF_DISCONNECT_TIMEOUT); /* atleast wait for one iteration */
                waitcnt = 0;
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                        "%s: Tx ChanSwitch is happening\n", __func__);
                while(waitcnt < OSIF_TXCHANSWITCH_LOOPCOUNT) {
                    schedule_timeout_interruptible(OSIF_DISCONNECT_TIMEOUT);
                    waitcnt++;
                }
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
                        "%s: Tx ChanSwitch is Completed\n", __func__);
            }

            ucfg_dfs_is_ap_cac_timer_running(vap->iv_ic->ic_pdev_obj, &is_ap_cac_timer_running);
            if(is_ap_cac_timer_running) {
                wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
                return -EINVAL;
            }
            if (wlan_vdev_is_up(vdev) == QDF_STATUS_SUCCESS) {
                vap->iv_roam_inprogress = TRUE;
            }
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                    "Did not roam : num desired_bssid %d ssid len %d \n",
                    desired_bssid, ssid.len);
            wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
            return -EINVAL;
        }
    }

    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);

    return 0;
}

int osif_vap_pre_init_ap(wlan_if_t vap, int forcescan)
{
#if ATH_SUPPORT_WAPI
    osif_dev* osifp = (osif_dev *)(vap->iv_ifp);
#endif
    struct ieee80211com *ic = vap->iv_ic;
    ieee80211_ssid ssid;

    /* do not start AP if no ssid is set */
    ssid.len = 0;
    wlan_get_desired_ssidlist(vap, &ssid, 1);
    if (!ssid.len)
        return 0;

    if (ic->recovery_in_progress) {
        qdf_err("FW Crash on vap %d ...returning", vap->iv_unit);
        return -EINVAL;
    }

#if QCN_IE
    vap->iv_next_beacon_tstamp =
        qdf_ktime_add_ns(qdf_ktime_get(), ic->ic_intval * QDF_NSEC_PER_MSEC);
#endif

#if ATH_SUPPORT_WAPI
    osif_wapi_rekeytimer_start((os_if_t)osifp);
#endif

    return 0;
}

int osif_vap_pre_init_sta(wlan_if_t vap, int forcescan)
{
    osif_dev* osifp = (osif_dev *)(vap->iv_ifp);
    ieee80211_ssid ssid;
    int desired_bssid = 0;
    u_int8_t des_bssid[QDF_MAC_ADDR_SIZE];

    ssid.len=0;
    wlan_get_desired_ssidlist(vap,&ssid,1);
    desired_bssid = wlan_aplist_get_desired_bssid_count(vap);
    if (desired_bssid == 1) {
        wlan_aplist_get_desired_bssidlist(vap, &des_bssid);
        if (IEEE80211_IS_BROADCAST(des_bssid))
            desired_bssid = 0;
    }

    if ((!desired_bssid && !ssid.len)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                "Did not start connection SM : num desired_bssid %d ssid len %d \n",
                desired_bssid, ssid.len);
        return -EAGAIN;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s: desired bssid %d ssid len %d \n",
            __func__, desired_bssid, ssid.len);

    if (forcescan & RESCAN)
        osifp->is_restart = 1;

    vap->iv_roam_inprogress = FALSE;

    return 0;
}

QDF_STATUS osif_vap_validate_link_sta(struct vdev_mlme_obj *vdev_mlme, void *ext_data)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;
    int is_ap_cac_timer_running = 0;

    vdev = vdev_mlme->vdev;
    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
       qdf_err("vap is NULL");
       return QDF_STATUS_E_FAILURE;
    }

    if (wlan_vap_get_opmode(vap) != IEEE80211_M_STA)
        return QDF_STATUS_E_FAILURE;

    ic = vap->iv_ic;
#if ATH_SUPPORT_WRAP
    if (wlan_rptr_vdev_is_no_event(vdev) && ic->ic_sta_vap) {
        if (!wlan_is_connected(ic->ic_sta_vap)) {
            return QDF_STATUS_E_FAILURE;
        }
    }
#endif

    ucfg_dfs_is_ap_cac_timer_running(ic->ic_pdev_obj, &is_ap_cac_timer_running);
    if (is_ap_cac_timer_running) {
        return QDF_STATUS_E_FAILURE;
    }

    vap->iv_roam_inprogress = FALSE;
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS osif_mlo_sta_clone_crypto_param(struct ieee80211vap *vap,
                                           struct wlan_cm_connect_req *req)
{

    wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE,
                               req->crypto.auth_type);
    wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_KEY_MGMT,
                               req->crypto.akm_suites);
    wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER,
                               req->crypto.ciphers_pairwise);
    wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_MCAST_CIPHER,
                               req->crypto.group_cipher);
    wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_MGMT_CIPHER,
                               req->crypto.mgmt_ciphers);
    wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_RSN_CAP,
                               req->crypto.rsn_caps);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS osif_mlo_sta_clone_security_param(struct vdev_mlme_obj *vdev_mlme,
                                             struct wlan_cm_connect_req *req)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *assoc_vdev = NULL;
    struct ieee80211vap *assoc_vap = NULL;
    int len;
    u_int8_t *buf = qdf_mem_malloc_atomic(IEEE80211_MAX_IE_LEN);

    if (!buf) {
        qdf_err("IE buf allocation failed");
        return QDF_STATUS_E_FAILURE;
    }

    vdev = vdev_mlme->vdev;
    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);

    if(vap == NULL) {
        qdf_err("vap is NULL");
        goto clone_fail;
    }

    if (wlan_vap_get_opmode(vap) != IEEE80211_M_STA)
        goto clone_fail;

    assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vdev);

    if (!assoc_vdev) {
        qdf_err("Assoc Vdev is NULL");
        goto clone_fail;
    }

    assoc_vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(assoc_vdev);

    if (!assoc_vap) {
        qdf_err("Assoc vap is NULL");
        goto clone_fail;
    }

    /* Copy sae_pwe value from assoc-vap to non-assoc-vap */
    vap->iv_sae_pwe = assoc_vap->iv_sae_pwe;

    /* Copy RSN-IE from assoc-vap to non-assoc-vap */
    len = ieee80211_add_or_retrieve_ie_from_app_opt_ies(assoc_vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
             IEEE80211_ELEMID_RSN, -1, &buf, TYPE_ALL_BUF, NULL, true) ;

    if (len) {
	buf -= len;
        wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
	     (u_int8_t *)buf, len, HOSTAPD_IE);
    }

    /* Copy RSNX-IE from assoc-vap to non-assoc-vap */
    len = ieee80211_add_or_retrieve_ie_from_app_opt_ies(assoc_vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
             IEEE80211_ELEMID_RSNX, -1, &buf, TYPE_ALL_BUF, NULL, true) ;

    if (len) {
        buf -= len;
        wlan_mlme_app_ie_set_check(vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
	     (u_int8_t *)buf, len, HOSTAPD_IE);
    }

    /* Copy crypto params from assoc-vap to non-assoc vap */
    if (osif_mlo_sta_clone_crypto_param(vap, req) != QDF_STATUS_SUCCESS)
        goto clone_fail;

    /* Free temp_ie buf */
    if (buf)
        qdf_mem_free(buf);

    return QDF_STATUS_SUCCESS;

clone_fail:
    if (buf)
        qdf_mem_free(buf);
    return QDF_STATUS_E_FAILURE;

}

#ifdef WLAN_FEATURE_11BE_MLO
#define COMMON_INFO_LEN_POS (1 + 1 + WLAN_ML_CTRL_SIZE + 1)
#define STA_INFO_LEN_POS (1 + WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_SIZE +1)
QDF_STATUS osif_update_op_class_from_mlie(struct vdev_mlme_obj *vdev_mlme, uint8_t *mlie)
{
    u_int8_t *per_sta = NULL;
    u_int8_t *per_sta_control = NULL;
    u_int8_t *per_sta_profile = NULL;
    u_int8_t *next_ie = NULL;
    u_int8_t link_id;

    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *link_vap = NULL;
    struct wlan_objmgr_vdev *link_vdev = NULL;

    uint16_t stacontrol;
    u_int8_t *op_class = NULL;


    op_class = qdf_mem_malloc_atomic(IEEE80211_MAX_IE_LEN + 2);

    if (!op_class) {
        qdf_err("Op-class alloc failed");
        goto fail;
    }

    if (!mlie) {
        qdf_err("No ML-IE found");
        goto fail;
    }


    vdev = vdev_mlme->vdev;

    mloie_debug("Hexdump of Multi-Link element in Connect-Req:");
    qdf_trace_hex_dump(QDF_MODULE_ID_MLOIE, QDF_TRACE_LEVEL_DEBUG, mlie, mlie[1]+2);

    /* Based on IEEE802.11BE Draft-2 spec consider ML-IE */
    per_sta = mlie + 1 + 1 + WLAN_ML_CTRL_SIZE + 1 + mlie[COMMON_INFO_LEN_POS];

    next_ie = mlie + mlie[1]+2;


    while (true) {

        if (per_sta >= next_ie)
            break;

        per_sta_control = per_sta + 2;

        if (per_sta_control >= next_ie)
            break;

        if (per_sta_control + WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_SIZE >= next_ie)
            break;

        qdf_mem_copy(&stacontrol, per_sta_control, WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_SIZE);

        link_id = QDF_GET_BITS(stacontrol, WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_LINKID_IDX,
                      WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_LINKID_BITS);

        link_vdev = mlo_get_vdev_by_link_id(vdev, link_id, WLAN_MLO_MGR_ID);

        if (!link_vdev) {
            qdf_err("No MLO Link-Vdev found");
            goto fail;
        }

        link_vap = wlan_vdev_get_mlme_ext_obj(link_vdev);

        if (!link_vap) {
            qdf_err("No MLO Link-vap found");
            goto fail;
        }

        per_sta_profile = per_sta + 1 + WLAN_ML_BV_LINFO_PERSTAPROF_STACTRL_SIZE
                          + WLAN_ML_BV_LINFO_PERSTAPROF_STAINFO_LENGTH_SIZE
                          + per_sta[STA_INFO_LEN_POS];

        if (per_sta_profile >= next_ie)
            break;

        /* Now only one IE OP-CLASS IE in per-sta profile of ML-IE */
        if (per_sta_profile[0] == IEEE80211_ELEMID_SUPP_OP_CLASS) {

            if ((per_sta_profile[1] + 2) > (IEEE80211_MAX_IE_LEN + 2))
                break;

            /* Check whether Op Class IE in per-STA profile is within ML IE boundary */
            if ((per_sta_profile + per_sta_profile[1]+2) > next_ie)
                break;

            qdf_mem_copy(op_class, per_sta_profile, per_sta_profile[1]+2);

            mloie_debug("Hexdump of OP-Class IE in ML-IE Link Info:");
            qdf_trace_hex_dump(QDF_MODULE_ID_MLOIE, QDF_TRACE_LEVEL_DEBUG, op_class, op_class[1]+2);

            /* Save IE in VAP APP-IE buffer */
            wlan_mlme_app_ie_set_check(link_vap, IEEE80211_FRAME_TYPE_ASSOCREQ,
                                       (u_int8_t *)op_class,
                                       op_class[1]+2, DEFAULT_IDENTIFIER);

            qdf_info(" Link Op-class update in app-ie buffer");
         }

         /* Goto next link-info */
         per_sta += per_sta[1] + 2;

         if (link_vdev) {
            mlo_release_vdev_ref(link_vdev);
            link_vdev = NULL;
         }
    }

    if (link_vdev) {
        mlo_release_vdev_ref(link_vdev);
        link_vdev = NULL;
    }

    if (op_class)
        qdf_mem_free(op_class);

    return QDF_STATUS_SUCCESS;

fail:

    if (link_vdev) {
        mlo_release_vdev_ref(link_vdev);
        link_vdev = NULL;
    }

    if (op_class)
        qdf_mem_free(op_class);

    return QDF_STATUS_E_FAILURE;
}
#endif

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
struct net_device *osif_vdev_get_mld_netdev(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211vap *vap;
    struct osif_mldev *osifp_mldev;
    osif_dev *osifp;

    if(wlan_vdev_mlme_is_mlo_vdev(vdev)) {
       vap = wlan_vdev_mlme_get_ext_hdl(vdev);
       if (!vap) {
           qdf_err("vap is null");
           return NULL;
       }

       osifp = (osif_dev *)((os_if_t)vap->iv_ifp);
       osifp_mldev = osifp->mldev;
       if (!osifp_mldev) {
           qdf_err("osifp_mldev is null");
           return NULL;
       } else
           return osifp_mldev->mld_dev;
    }
    return NULL;
}
#else
struct net_device *osif_vdev_get_mld_netdev(struct wlan_objmgr_vdev *vdev)
{
    return NULL;
}
#endif

int wlan_vap_pre_init(wlan_if_t vap, int forcescan)
{
    osif_dev *osifp = (osif_dev *)vap->iv_ifp;
#ifndef ENABLE_CFG80211_BACKPORTS_MLO
    struct net_device *parent = osifp->os_comdev;
#endif
    enum ieee80211_opmode opmode;
    wlan_dev_t comhandle;
    int ret_val = 0;
#if UMAC_VOW_DEBUG
    struct wlan_objmgr_psoc *psoc = NULL;
#endif

    opmode = wlan_vap_get_opmode(vap);
    comhandle = wlan_vap_get_devhandle(vap);

    if (comhandle->no_chans_available == 1) {
        qdf_err("%s: vap-%d(%s) channel is not available",
                __func__, vap->iv_unit, vap->iv_netdev_name);
        return -EINVAL;
    }

#ifndef ENABLE_CFG80211_BACKPORTS_MLO
    if (osif_get_num_active_vaps(comhandle) == 0 &&
            (parent->flags & IFF_RUNNING) == 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
        ret_val = dev_open(parent, NULL);
#else
        ret_val = dev_open(parent);
#endif
        if (ret_val)
            return ret_val;
    }

    /*
     * If the parent is up and running, then kick the 802.11 state machine
     * as appropriate. parent should always be up+running
     */
    if (!IS_UP(parent)) {
        QDF_ASSERT(0);
        return -EINVAL;
    }
#endif

    osifp->os_last_siwscan = 0;
    osifp->os_if->iv_mlmeconnect=0;
#if UMAC_VOW_DEBUG
    psoc = wlan_pdev_get_psoc(comhandle->ic_pdev_obj);
    osifp->carrier_vow_config = cfg_get(psoc, CFG_OL_CARRIER_VOW_CONFIG);
#endif

    switch (opmode) {
    case IEEE80211_M_STA:
        ret_val = osif_vap_pre_init_sta(vap, forcescan);
        break;
    case IEEE80211_M_MONITOR:
        /* No init config for monitor mode */
        break;
    default:
        ret_val = osif_vap_pre_init_ap(vap, forcescan);
        break;
    }

    return ret_val;
}

int osif_vap_pre_init(osif_dev *osifp, struct net_device *dev, int forcescan)
{
    wlan_if_t vap = osifp->os_if;
    int ret_val = 0;

    if (!vap)
        return -EINVAL;

    ret_val = wlan_vap_pre_init(vap, forcescan);
    if (!ret_val)
        dev->flags |= IFF_RUNNING;

    return ret_val;
}

int wlan_vap_init(wlan_if_t vap)
{
    enum ieee80211_opmode opmode;

    opmode = wlan_vap_get_opmode(vap);
    switch (opmode) {
    case IEEE80211_M_STA:
        if (wlan_cm_is_vdev_connected(vap->vdev_obj))
            mlo_disconnect(vap->vdev_obj, CM_OSIF_CFG_DISCONNECT,
                           REASON_UNSPEC_FAILURE, NULL);
        break;
    case IEEE80211_M_MONITOR:
        wlan_mlme_start_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
        break;
    default:
        wlan_mlme_stop_start_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_MLME);
        break;
    }

    return 0;
}

int osif_vap_init(osif_dev *osifp, struct net_device *dev, int forcescan)
{
    wlan_if_t vap = osifp->os_if;
    int ret_val;

    ret_val = osif_vap_pre_init(osifp, dev, forcescan);
    if (ret_val)
        return ret_val;

    return wlan_vap_init(vap);
}

/* osif_vap_open_preprocess() - check for sanity and invoke platfrom vap init
 * @dev - net_device pointer
 *
 * Return: Zero on success
 *         Erroe code on failure
 */
static int osif_vap_open_preprocess(struct net_device *dev)
{
    osif_dev  *osifp = ath_netdev_priv(dev);
#ifdef ATH_BUS_PM
    struct net_device *comdev = osifp->os_comdev;
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211*)ath_netdev_priv(comdev);
#endif
    wlan_if_t vap = osifp->os_if;
    enum ieee80211_opmode opmode;
    struct ieee80211com *ic = NULL;

    if (!vap)
        return -EINVAL;

    ic = vap->iv_ic;

#ifdef ATH_BUS_PM
    if (scn->sc_osdev->isDeviceAsleep)
	return -EPERM;
#endif /* ATH_BUS_PM */

    opmode = wlan_vap_get_opmode(vap);
#if UNIFIED_SMARTANTENNA
    if (opmode == IEEE80211_M_STA) {
        /* Initialise smart antenna with default param to help scanning */
        (vap->iv_ic)->sta_not_connected_cfg = TRUE;
    }
#endif
#ifdef QCA_PARTNER_PLATFORM
    osif_pltfrm_vap_init( dev );
#endif
    return 0;
}

/* osif_vap_open_finish() - check for sanity and invoke vap init
 * @dev - net_device pointer
 *
 * Return: Zero on success
 *         Error code on failure
 */
static int osif_vap_open_main(struct net_device *dev)
{
    int err = 0;
    osif_dev  *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    enum ieee80211_opmode opmode;
    struct ieee80211com *ic = NULL;
    uint8_t grp_id;
    struct mbssid_context *mbss_ctx;
    struct mbssid_info *ic_mbss = NULL;

    if (!vap)
        return -EINVAL;

    ic = vap->iv_ic;
    if (!ic)
        return -EINVAL;

    grp_id = vap->iv_mbss.group_id;

    if (ic->is_mbssid_enabled) {
        if (!ic->ic_mbss_ctx) {
            qdf_err("mbss_ctx is null!!!");
            return -EINVAL;
        }
        mbss_ctx = ic->ic_mbss_ctx;
        ic_mbss  = &mbss_ctx->ic_mbss[grp_id];
    }

    /* if first vap is a nawds vap, bring it up as a non-tx vap */
    if (ic->is_mbssid_enabled &&
        (ieee80211_mbssid_get_num_beaconing_ap_vaps_up(ic, grp_id) == 1) &&
        ieee80211_mbss_is_beaconing_ap(vap)) {
        if (ieee80211_ucfg_reset_nawds_txvap(vap))
            return -EINVAL;
    }
    ieee80211_ucfg_set_txvap(vap);

    if (ic->is_mbssid_enabled) {
        if (ic_mbss && !ic_mbss->transmit_vap &&
           (ieee80211_mbss_is_beaconing_ap(vap))) {
          mbss_err("vap%d:Tx vap should be configured before bringup of first VAP",
                  vap->iv_unit);
          return -EOPNOTSUPP;
        }
    }

    if (ic->is_mbssid_enabled &&
        !ieee80211_mbssid_get_num_beaconing_ap_vaps_up(ic, grp_id)) {
        if (ieee80211_mbss_is_beaconing_ap(vap)) {
            ic->ic_ema_config_init(vap);
        } else {
            mbss_debug("Skip calling ic_ema_config_init as vap%d is not an AP that is part of an MBSSID group",
                       vap->iv_unit);
        }
    }

    /*
     * Beacon interval is got as a part of cfg80211_ap_settings.
     * Include this block before calling osif_vap_init from start_ap.
     */

    opmode = wlan_vap_get_opmode(vap);

    wlan_vap_up_check_beacon_interval(vap,opmode);

    if (ic->ic_need_vap_reinit) {
        osif_pdev_restart_vaps(ic);
        ic->ic_need_vap_reinit = 0;
    }

    err = osif_vap_init(osifp, dev, 0);
    return err;
}

#if UMAC_SUPPORT_CFG80211
/* is_defer_vap_init_required: function decides to defer vap init
 * till hostpad call start_ap or bringup vap during dev_open.
 * @vap: vap pointer
 * Returns TRUE if vap_init need to be defered
           FALSE to bringup vap immediately
 */

bool is_defer_vap_init_required(wlan_if_t vap)
{
    /*
     * vap_init need to be defered if VAP is cfg80211 enabled and
     * vap mode is: AP/STA/WDS.
     * proprtiery VAP modes (iv_special_vap_mode, iv_mesh_vap_mode)
     * does not require hostpad to bringup vap so defer of vap_init is
     * not required.
     */
    if(vap->iv_cfg80211_create && ((vap->iv_opmode == IEEE80211_M_HOSTAP) ||
                (vap->iv_opmode == IEEE80211_M_STA) ||
                (vap->iv_opmode == IEEE80211_M_WDS)) &&
                !IEEE80211_IS_SPECIAL_VAP(vap)
#if MESH_MODE_SUPPORT
                && (vap->iv_mesh_vap_mode == 0)
#endif
      )
    {
        return true;
    } else {
        return false;
    }
}
#endif

/* osif_vap_open() - Vap open
 * @dev - net_device pointer
 *
 * The functionality of vap open is split into 3 chunks
 * 1. osif_vap_open_preprocess() - Check for sanity and do platform vap init
 * 2. Check for cfg80211 configuration and return, for vap init will be taken care by hostapd
 * 3. osif_vap_open_main() - Check for sanity and invoke osif_vap_init()
 *
 * Return: Zero on success
 *         Error code on failure
 */
int
osif_vap_open(struct net_device *dev)
{
    osif_dev  *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    struct net_device *parent = osifp->os_comdev;
    wlan_dev_t comhandle = NULL;
    int status = 0;

    if (vap->iv_ic->recovery_in_progress) {
       return -EINVAL;
    }

    status = osif_vap_open_preprocess(dev);

    if (status != 0)
	    return status;

    comhandle = wlan_vap_get_devhandle(vap);
    if ((dev->flags & IFF_RUNNING) == 0) {
        if (osif_get_num_active_vaps(comhandle) == 0 &&
                (parent->flags & IFF_RUNNING) == 0){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
            status = dev_open(parent, NULL);
#else
            status = dev_open(parent);
#endif
            if (status != 0)
                return status;
        }
    }

    /*
     * if cfg80211 created vap and mode is AP
     * postpone osif_vap_init till start_ap
     */
#if UMAC_SUPPORT_CFG80211
    if (is_defer_vap_init_required(vap)) {

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_CFG80211,
        "%s : Vap is created using cfg80211, Hostapd/supplicant needs to call startap/connect \n",
        __func__);
        return 0;
    }
#endif
    return osif_vap_open_main(dev);
}

/*
 * osif_num_ap_up_vaps() - Set frequency to the radio
 * @ic: ic object handle
 *
 * This function gets the number of AP VAPs brought up by the user space
 *
 * Return: Number of AP VAPs brought up by the user space
 */
int osif_num_ap_up_vaps(struct ieee80211com *ic)
{
    wlan_if_t tmpvap;
    struct net_device *tmpdev;
    int cnt = 0;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
        if (IS_UP(tmpdev) && (tmpvap->iv_opmode == IEEE80211_M_HOSTAP)) {
            cnt++;
        }
    }
    return cnt;
}

int
osif_restart_for_config(struct ieee80211com *ic,
                        int (*config_callback)(struct ieee80211com *ic,
                                               struct ieee80211_ath_channel *new_chan),
                        struct ieee80211_ath_channel *new_chan)
{
    QDF_STATUS retval;
    int32_t err = 0;

    qdf_assert_always(ic != NULL);

    wlan_mbss_stop_vdevs(ic->ic_pdev_obj, NULL);
    retval = wlan_pdev_wait_to_bringdown_vdevs(ic, ALL_VDEVS);
    if (retval == QDF_STATUS_E_INVAL)
        return -EINVAL;

    if (config_callback) {
         err = config_callback(ic, new_chan);
         if (err)
             return -EINVAL;
    }

    wlan_mbss_start_vdevs(ic->ic_pdev_obj, NULL);

    return 0;
}

/*
* Return netdevice statistics.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
void
osif_getstats(struct net_device *dev, struct rtnl_link_stats64* stats64)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static struct rtnl_link_stats64*
    osif_getstats(struct net_device *dev, struct rtnl_link_stats64* stats64)
#else
static struct net_device_stats *
    osif_getstats(struct net_device *dev)
#endif
{
    osif_dev  *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    struct ieee80211_mac_stats unimac_stats;
    struct ieee80211_mac_stats multimac_stats;
    const struct ieee80211_mac_stats *unimacstats;
    const struct ieee80211_mac_stats *multimacstats;
    struct ieee80211com    *ic = NULL;
    struct wlan_objmgr_psoc *psoc;
    ol_txrx_soc_handle soc_txrx_handle;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    struct net_device_stats *stats = &osifp->os_devstats;
    struct net_device_stats *stats64 = stats;
#else
    struct rtnl_link_stats64 *stats = &osifp->os_devstats;
#endif
    struct cdp_vdev_stats *vdev_stats = NULL;
    cdp_config_param_type value = {0};

    qdf_mem_zero(&unimac_stats, sizeof(unimac_stats));
    qdf_mem_zero(&multimac_stats, sizeof(multimac_stats));
    unimacstats = &unimac_stats;
    multimacstats = &multimac_stats;

    if (!vap || osifp->is_delete_in_progress || (vap->iv_opmode == IEEE80211_M_MONITOR)
        || ((vap->iv_ic) && vap->iv_ic->recovery_in_progress)
        || ((dev->flags & IFF_RUNNING) != IFF_RUNNING)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
        memcpy(stats64, &osifp->os_devstats, sizeof(struct rtnl_link_stats64));
#endif
        goto getstats_fail;
    }

#if QCA_PARTNER_DIRECTLINK_RX
    /* get netdevice statistics through partner API */
    if (qdf_unlikely(osifp->is_directlink)) {
	    return osif_getstats_partner(dev);
    }
#endif /* QCA_PARTNER_DIRECTLINK_RX */

    ic = vap->iv_ic;
#ifdef QCA_SUPPORT_CP_STATS
    if (!wlan_mac_stats_from_cp_stats(vap, 0, &unimac_stats))
        goto getstats_fail;

    if (!wlan_mac_stats_from_cp_stats(vap, 1, &multimac_stats))
        goto getstats_fail;
#endif
    if (!ic)
        goto getstats_fail;

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc)
        goto getstats_fail;

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_txrx_handle)
        goto getstats_fail;

    vdev_stats = qdf_mem_malloc_atomic(sizeof(struct cdp_vdev_stats));
    if (!vdev_stats) {
         qdf_err("vdev stats Allocation Failed!");
         return;
    }

    cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_CFG_VDEV_STATS_HW_OFFLOAD,
                            &value);

    if (cdp_host_get_interface_stats(soc_txrx_handle,
                                     wlan_vdev_get_id(vap->vdev_obj),
                                     vdev_stats) != QDF_STATUS_SUCCESS)
        goto getstats_fail;

    stats->tx_packets = vdev_stats->tx.comp_pkt.num;
    stats->tx_bytes = vdev_stats->tx.comp_pkt.bytes;
    stats->tx_errors = vdev_stats->tx.tx_failed;
    stats->tx_dropped = vdev_stats->tx_i.dropped.dropped_pkt.num +
                        vdev_stats->tx_i.sg.dropped_host.num +
                        vdev_stats->tx_i.mcast_en.dropped_map_error +
                        vdev_stats->tx_i.mcast_en.dropped_self_mac +
                        vdev_stats->tx_i.mcast_en.dropped_send_fail +
                        vdev_stats->tx.nawds_mcast_drop;

    stats->rx_errors = vdev_stats->rx.err.mic_err +
                       vdev_stats->rx.err.decrypt_err +
                       vdev_stats->rx.err.fcserr +
                       vdev_stats->rx.err.pn_err +
                       vdev_stats->rx.err.oor_err +
                       vdev_stats->rx.err.jump_2k_err +
                       vdev_stats->rx.err.rxdma_wifi_parse_err;
    stats->rx_dropped = vdev_stats->rx.mec_drop.num +
                        vdev_stats->rx.multipass_rx_pkt_drop +
                        vdev_stats->rx.peer_unauth_rx_pkt_drop +
                        vdev_stats->rx.policy_check_drop +
                        vdev_stats->rx.nawds_mcast_drop +
                        vdev_stats->rx.mcast_3addr_drop;
    stats->rx_crc_errors = 0;

    if (value.cdp_psoc_param_vdev_stats_hw_offload) {
        stats->rx_packets = vdev_stats->rx_i.reo_rcvd_pkt.num +
                            vdev_stats->rx_i.null_q_desc_pkt.num +
                            vdev_stats->rx_i.routed_eapol_pkt.num;
        stats->rx_bytes = vdev_stats->rx_i.reo_rcvd_pkt.bytes +
                          vdev_stats->rx_i.null_q_desc_pkt.bytes +
                          vdev_stats->rx_i.routed_eapol_pkt.bytes;
    } else {
        stats->rx_packets = vdev_stats->rx.to_stack.num;
        stats->rx_bytes = vdev_stats->rx.to_stack.bytes;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    memcpy(stats64, stats, sizeof(struct rtnl_link_stats64));
#endif

getstats_fail:
    if (vdev_stats)
         qdf_mem_free(vdev_stats);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
    return;
#else
    return stats64;
#endif
}
qdf_export_symbol(osif_getstats);

static void osif_com_vap_event_handler(void *event_arg, wlan_dev_t devhandle, os_if_t osif, ieee80211_dev_vap_event event)
{
    osif_dev  *osifp = (osif_dev *) osif;
#if ACFG_NETLINK_TX
#if UMAC_SUPPORT_ACFG
    acfg_netlink_pvt_t *acfg_nl = NULL;
    wlan_if_t vap = osifp->os_if;
#endif
#endif

    switch(event) {
    case IEEE80211_VAP_CREATED:
        break;
    case IEEE80211_VAP_DELETED:
        osifp->is_deleted=1;
#if ACFG_NETLINK_TX
#if UMAC_SUPPORT_ACFG
        acfg_nl = (acfg_netlink_pvt_t *)vap->iv_ic->ic_acfg_handle;
        if (vap == acfg_nl->vap) {
            acfg_nl->vap = NULL;
        }
#endif
#endif
        break;
    case IEEE80211_VAP_STOPPED:
       break;
    case IEEE80211_VAP_STOP_ERROR:
        /* The requested STOP returned error.
         * Tasks waiting for is_stop_event_pending may not receive the Stop event.
         */
        /* TODO: handle this case in conjuction with is_stop_event_pending */
       break;
    default:
        break;
    }

}

static wlan_dev_event_handler_table com_evtable = {
    osif_com_vap_event_handler,
#if WLAN_SPECTRAL_ENABLE
    NULL,
#endif
#if UMAC_SUPPORT_ACFG
    osif_radio_evt_radar_detected_ap,           /*wlan_radar_event*/
#else
    NULL,
#endif
#if UMAC_SUPPORT_ACFG || UMAC_SUPPORT_ACFG_RECOVERY
     osif_radio_evt_watch_dog,                   /*wlan_wdt_event*/
#else
    NULL,
#endif
#if UMAC_SUPPORT_ACFG
    osif_radio_evt_dfs_cac_start,               /*wlan_cac_start_event*/
    osif_radio_evt_dfs_up_after_cac,            /*wlan_up_after_cac_event*/
    osif_radio_evt_chan_util,                   /*wlan_chan_util_event*/
#endif
};

void wlan_register_scantimer_handler(void * devhandle)
{
    wlan_autoselect_register_scantimer_handler(devhandle, osif_acs_bk_scantimer_fn , devhandle);
}

extern void ieee80211_dfs_non_dfs_chan_config(void *arg);

void osif_attach(struct net_device *comdev)
{
#ifdef UMAC_SUPPORT_CFG80211
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
#else
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(NULL, comdev);
#endif
    wlan_device_register_event_handlers(devhandle,comdev,&com_evtable);
    wlan_register_scantimer_handler(devhandle);

#ifdef QCA_PARTNER_PLATFORM
    wlan_pltfrm_attach(comdev);
#endif /* QCA_PARTNER_PLATFORM */
}
qdf_export_symbol(osif_attach);

void osif_detach(struct net_device *comdev)
{
#ifdef UMAC_SUPPORT_CFG80211
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
#else
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(NULL, comdev);
#endif
    wlan_device_unregister_event_handlers(devhandle,(void *)comdev,&com_evtable);

#ifdef QCA_PARTNER_PLATFORM
    wlan_pltfrm_detach(comdev);
#endif

#ifdef ATH_SUPPORT_DFS
    qdf_flush_work(&devhandle->dfs_cac_timer_start_work);
    qdf_disable_work(&devhandle->dfs_cac_timer_start_work);
#endif

    qdf_flush_work(&devhandle->ic_cfg80211_update_channel_list_start_work);
    qdf_disable_work(&devhandle->ic_cfg80211_update_channel_list_start_work);

    qdf_mem_free(devhandle->set_country);
    qdf_flush_work(&devhandle->assoc_sm_set_country_code);
    qdf_disable_work(&devhandle->assoc_sm_set_country_code);

    devhandle->set_country = NULL;
}
qdf_export_symbol(osif_detach);

#if QCA_OL_VLAN_WAR
#define MAC_ADDR_CPY(a,b)       \
    do {     \
        *(uint32_t *)&a[0] = *(uint32_t *)&b[0]; \
        a[4]=b[4]; \
        a[5]=b[5]; \
    } while (0);

static int encap_eth2_to_dot3(qdf_nbuf_t msdu)
{
    u_int16_t typeorlen;
    struct ether_header eth_hdr, *eh;
    struct llc *llcHdr;
    qdf_assert(msdu != NULL);
    if (qdf_nbuf_headroom(msdu) < sizeof(*llcHdr))
    {
        qdf_print("Encap: Don't have enough headroom");
        return 1;
    }

    eh = (struct ether_header *) qdf_nbuf_data(msdu);

    /*
     * Save addresses to be inserted later
     */
    MAC_ADDR_CPY(eth_hdr.ether_dhost, eh->ether_dhost);
    MAC_ADDR_CPY(eth_hdr.ether_shost, eh->ether_shost);
     /*** NOTE: REQUIRES NTOHS IF REFERENCED ***/
    typeorlen = eh->ether_type;

    /*
     * Make room for LLC + SNAP headers
     */
    if (qdf_nbuf_push_head(msdu, sizeof(*llcHdr)) == NULL){
        qdf_print("Encap: Failed to push LLC header");
        return 1;
    }

    eh = (struct ether_header *) qdf_nbuf_data(msdu);

    MAC_ADDR_CPY(eh->ether_dhost, eth_hdr.ether_dhost);
    MAC_ADDR_CPY(eh->ether_shost, eth_hdr.ether_shost);
    eh->ether_type = htons((uint16_t) (msdu->len - sizeof(eth_hdr)));

    llcHdr = (struct llc *)((u_int8_t *)eh + sizeof(eth_hdr));
    llcHdr->llc_dsap                     = LLC_SNAP_LSAP;
    llcHdr->llc_ssap                     = LLC_SNAP_LSAP;
    llcHdr->llc_un.type_snap.control     = LLC_UI;
    llcHdr->llc_un.type_snap.org_code[0] = RFC1042_SNAP_ORGCODE_0;
    llcHdr->llc_un.type_snap.org_code[1] = RFC1042_SNAP_ORGCODE_1;
    llcHdr->llc_un.type_snap.org_code[2] = RFC1042_SNAP_ORGCODE_2;
    llcHdr->llc_un.type_snap.ether_type  = typeorlen;
    return 0;
}
#endif

#if MESH_MODE_SUPPORT
void
os_if_tx_free_batch_ext(struct sk_buff *bufs, int tx_err)
{
    while (bufs) {
        struct sk_buff *next = bufs->next;
        if (external_tx_complete != NULL) {
            external_tx_complete(bufs);
        } else {
            qdf_nbuf_free(bufs);
        }
        bufs = next;
    }
}

void
os_if_tx_free_ext(struct sk_buff *skb)
{
    if (external_tx_complete != NULL) {
        nbuf_debug_del_record(skb);
        external_tx_complete(skb);
    } else {
        qdf_nbuf_free(skb);
    }
}
qdf_export_symbol(os_if_tx_free_ext);
#endif

/*
* Convert a media specification to an 802.11 phy mode.
*/
static int
media2mode(const struct ifmedia_entry *ime, enum ieee80211_phymode *mode)
{

    /* 11AX TODO (Phase II) - Add 11ax processing here if required based on
     * latest code status.
     */

    switch (IFM_MODE(ime->ifm_media)) {
    case IFM_IEEE80211_11A:
        *mode = IEEE80211_MODE_11A;
        break;
    case IFM_IEEE80211_11B:
        *mode = IEEE80211_MODE_11B;
        break;
    case IFM_IEEE80211_11G:
        *mode = IEEE80211_MODE_11G;
        break;
    case IFM_IEEE80211_FH:
        *mode = IEEE80211_MODE_FH;
        break;
        case IFM_IEEE80211_11NA:
                *mode = IEEE80211_MODE_11NA_HT20;
                break;
        case IFM_IEEE80211_11NG:
                *mode = IEEE80211_MODE_11NG_HT20;
                break;
    case IFM_AUTO:
        *mode = IEEE80211_MODE_AUTO;
        break;
    default:
        return 0;
    }

    if (ime->ifm_media & IFM_IEEE80211_HT40PLUS) {
            if (ieee80211_is_phymode_11na_ht20(*mode))
                    *mode = IEEE80211_MODE_11NA_HT40PLUS;
            else if (ieee80211_is_phymode_11ng_ht20(*mode))
                    *mode = IEEE80211_MODE_11NG_HT40PLUS;
            else
                    return 0;
    }

    if (ime->ifm_media & IFM_IEEE80211_HT40MINUS) {
            if (ieee80211_is_phymode_11na_ht20(*mode))
                    *mode = IEEE80211_MODE_11NA_HT40MINUS;
            else if (ieee80211_is_phymode_11ng_ht20(*mode))
                    *mode = IEEE80211_MODE_11NG_HT40MINUS;
            else
                    return 0;
    }

    return 1;
}

/*
 * convert IEEE80211 ratecode to ifmedia subtype.
 */
static int
rate2media(struct ieee80211vap *vap,int rate,enum ieee80211_phymode mode)
{
#define N(a)    (sizeof(a) / sizeof(a[0]))
    struct ieee80211com    *ic = wlan_vap_get_devhandle(vap);
    static const struct
    {
        u_int   m;      /* rate + mode */
        u_int   r;      /* if_media rate */
    } rates[] = {
                {   0x1b | IFM_IEEE80211_FH, IFM_IEEE80211_FH1 },
                {   0x1a | IFM_IEEE80211_FH, IFM_IEEE80211_FH2 },
                {   0x1b | IFM_IEEE80211_11B, IFM_IEEE80211_DS1 },
                {   0x1a | IFM_IEEE80211_11B, IFM_IEEE80211_DS2 },
                {   0x19 | IFM_IEEE80211_11B, IFM_IEEE80211_DS5 },
                {   0x18 | IFM_IEEE80211_11B, IFM_IEEE80211_DS11 },
                {   0x1a | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM2_25 },
                {   0x0b | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM6 },
                {   0x0f | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM9 },
                {   0x0a | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM12 },
                {   0x0e | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM18 },
                {   0x09 | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM24 },
                {   0x0d | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM36 },
                {   0x08 | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM48 },
                {   0x0c | IFM_IEEE80211_11A, IFM_IEEE80211_OFDM54 },
                {   0x1b | IFM_IEEE80211_11G, IFM_IEEE80211_DS1 },
                {   0x1a | IFM_IEEE80211_11G, IFM_IEEE80211_DS2 },
                {   0x19 | IFM_IEEE80211_11G, IFM_IEEE80211_DS5 },
                {   0x18 | IFM_IEEE80211_11G, IFM_IEEE80211_DS11 },
                {   0x0b | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM6 },
                {   0x0f | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM9 },
                {   0x0a | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM12 },
                {   0x0e | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM18 },
                {   0x09 | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM24 },
                {   0x0d | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM36 },
                {   0x08 | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM48 },
                {   0x0c | IFM_IEEE80211_11G, IFM_IEEE80211_OFDM54 },
                /* NB: OFDM72 doesn't realy exist so we don't handle it */
                /* 11AX TODO (Phase I) - Need to check if and what changes are
                 * required here.
                 */
        };
    u_int mask, i, phytype;

    mask = rate & IEEE80211_RATE_VAL;
    switch (mode)
    {
    case IEEE80211_MODE_11A:
    case IEEE80211_MODE_TURBO_A:
    case IEEE80211_MODE_11NA_HT20:
    case IEEE80211_MODE_11NA_HT40MINUS:
    case IEEE80211_MODE_11NA_HT40PLUS:
    case IEEE80211_MODE_11NA_HT40:
    case IEEE80211_MODE_11AC_VHT20:
    case IEEE80211_MODE_11AC_VHT40PLUS:
    case IEEE80211_MODE_11AC_VHT40MINUS:
    case IEEE80211_MODE_11AC_VHT40:
    case IEEE80211_MODE_11AC_VHT80:
    case IEEE80211_MODE_11AC_VHT160:
    case IEEE80211_MODE_11AC_VHT80_80:
    case IEEE80211_MODE_11AXA_HE20:
    case IEEE80211_MODE_11AXA_HE40PLUS:
    case IEEE80211_MODE_11AXA_HE40MINUS:
    case IEEE80211_MODE_11AXA_HE40:
    case IEEE80211_MODE_11AXA_HE80:
    case IEEE80211_MODE_11AXA_HE160:
    case IEEE80211_MODE_11AXA_HE80_80:
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEA_EHT20:
    case IEEE80211_MODE_11BEA_EHT40PLUS:
    case IEEE80211_MODE_11BEA_EHT40MINUS:
    case IEEE80211_MODE_11BEA_EHT40:
    case IEEE80211_MODE_11BEA_EHT80:
    case IEEE80211_MODE_11BEA_EHT160:
    case IEEE80211_MODE_11BEA_EHT320:
#endif /* WLAN_FEATURE_11BE */
        mask |= IFM_IEEE80211_11A;
        break;
    case IEEE80211_MODE_11B:
        mask |= IFM_IEEE80211_11B;
        break;
    case IEEE80211_MODE_FH:
        mask |= IFM_IEEE80211_FH;
        break;
    case IEEE80211_MODE_AUTO:
        phytype = wlan_get_current_phytype(ic);

        if (phytype == IEEE80211_T_FH)
        {
            mask |= IFM_IEEE80211_FH;
            break;
        }
        /* NB: hack, 11g matches both 11b+11a rates */
        /* fall thru... */
    case IEEE80211_MODE_11G:
    case IEEE80211_MODE_TURBO_G:
    case IEEE80211_MODE_11NG_HT20:
    case IEEE80211_MODE_11NG_HT40MINUS:
    case IEEE80211_MODE_11NG_HT40PLUS:
    case IEEE80211_MODE_11NG_HT40:
    case IEEE80211_MODE_11AXG_HE20:
    case IEEE80211_MODE_11AXG_HE40PLUS:
    case IEEE80211_MODE_11AXG_HE40MINUS:
    case IEEE80211_MODE_11AXG_HE40:
#ifdef WLAN_FEATURE_11BE
    case IEEE80211_MODE_11BEG_EHT20:
    case IEEE80211_MODE_11BEG_EHT40PLUS:
    case IEEE80211_MODE_11BEG_EHT40MINUS:
    case IEEE80211_MODE_11BEG_EHT40:
#endif /* WLAN_FEATURE_11BE */
        mask |= IFM_IEEE80211_11G;
        break;
    default:
        break;
    }
    for (i = 0; i < N(rates); i++) {
        if (rates[i].m == mask) {
            return rates[i].r;
     }
    }
    return IFM_AUTO;
#undef N
}


static int
osif_media_change(struct net_device *dev)
{
    osif_dev  *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    enum ieee80211_phymode newphymode;
    struct ifmedia_entry *ime = osifp->os_media.ifm_cur;

    /*
    * First, identify the phy mode.
    */
    if (!media2mode(ime, &newphymode))
        return EINVAL;
    if (newphymode != wlan_get_desired_phymode(vap)) {
        return wlan_set_desired_phymode(vap,newphymode);
    }
    return 0;
}

static const u_int mopts[] = {
        IFM_AUTO,
        IFM_IEEE80211_11A,
        IFM_IEEE80211_11B,
        IFM_IEEE80211_11G,
        IFM_IEEE80211_FH,
        IFM_IEEE80211_11A | IFM_IEEE80211_TURBO,
        IFM_IEEE80211_11G | IFM_IEEE80211_TURBO,
        IFM_IEEE80211_11NA,
        IFM_IEEE80211_11NG,
        IFM_IEEE80211_11NA | IFM_IEEE80211_HT40PLUS,
        IFM_IEEE80211_11NA | IFM_IEEE80211_HT40MINUS,
        IFM_IEEE80211_11NG | IFM_IEEE80211_HT40PLUS,
        IFM_IEEE80211_11NG | IFM_IEEE80211_HT40MINUS,
        0,
        0,
        IFM_IEEE80211_11AC,
        IFM_IEEE80211_11AC | IFM_IEEE80211_W40PLUS,
        IFM_IEEE80211_11AC | IFM_IEEE80211_W40MINUS,
        0,
        IFM_IEEE80211_11AC | IFM_IEEE80211_W80,
        IFM_IEEE80211_11AC | IFM_IEEE80211_W160,
        IFM_IEEE80211_11AC | IFM_IEEE80211_W80_80,
        IFM_IEEE80211_11AXA,
        IFM_IEEE80211_11AXG,
        IFM_IEEE80211_11AXA | IFM_IEEE80211_W40PLUS,
        IFM_IEEE80211_11AXA | IFM_IEEE80211_W40MINUS,
        IFM_IEEE80211_11AXG | IFM_IEEE80211_W40PLUS,
        IFM_IEEE80211_11AXG | IFM_IEEE80211_W40MINUS,
        0,
        0,
        IFM_IEEE80211_11AXA | IFM_IEEE80211_W80,
        IFM_IEEE80211_11AXA | IFM_IEEE80211_W160,
        IFM_IEEE80211_11AXA | IFM_IEEE80211_W80_80,
#ifdef WLAN_FEATURE_11BE
        IFM_IEEE80211_11BEA,
        IFM_IEEE80211_11BEG,
        IFM_IEEE80211_11BEA | IFM_IEEE80211_W40PLUS,
        IFM_IEEE80211_11BEA | IFM_IEEE80211_W40MINUS,
        IFM_IEEE80211_11BEG | IFM_IEEE80211_W40PLUS,
        IFM_IEEE80211_11BEG | IFM_IEEE80211_W40MINUS,
        0,
        0,
        IFM_IEEE80211_11BEA | IFM_IEEE80211_W80,
        IFM_IEEE80211_11BEA | IFM_IEEE80211_W160,
        IFM_IEEE80211_11BEA | IFM_IEEE80211_W320,
#endif
};

/*
* Common code to calculate the media status word
* from the operating mode and channel state.
*/
static int
media_status(enum ieee80211_opmode opmode, wlan_chan_t chan)
{
    int status;
    enum ieee80211_phymode mode = IEEE80211_MODE_AUTO;    /* autoselect */

    status = IFM_IEEE80211;
    switch (opmode) {
    case IEEE80211_M_STA:
        break;
    case IEEE80211_M_IBSS:
        status |= IFM_IEEE80211_ADHOC;
        break;
    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_P2P_GO:
        status |= IFM_IEEE80211_HOSTAP;
        break;
    case IEEE80211_M_MONITOR:
        status |= IFM_IEEE80211_MONITOR;
        break;
    case IEEE80211_M_AHDEMO:
    case IEEE80211_M_WDS:
        /* should not come here */
        break;
    default:
        break;
    }
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        mode = wlan_channel_phymode(chan);
    }
    if (mode >= 0 && mode < (sizeof(mopts)/sizeof(mopts[0])) ) {
        status |= mopts[mode];
    }

    /* XXX else complain? */

    return status;
}

static void
osif_media_status(struct net_device *dev, struct ifmediareq *imr)
{

    int rate;
    osif_dev  *osdev = ath_netdev_priv(dev);
    wlan_if_t vap = osdev->os_if;
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;
    enum ieee80211_phymode mode;

    imr->ifm_status = IFM_AVALID;
    if ((dev->flags & IFF_UP))
        imr->ifm_status |= IFM_ACTIVE;
    *chan = wlan_get_current_channel(vap,true);
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        imr->ifm_active = media_status(osdev->os_opmode, chan);
    }

    if(wlan_is_connected(vap)) {
         mode = wlan_get_bss_phymode(vap);
    }
    else {
         mode = IEEE80211_MODE_AUTO;
    }

    rate = wlan_get_param(vap, IEEE80211_FIXED_RATE);
    if (rate != IEEE80211_FIXED_RATE_NONE)
    {
        /*
        * A fixed rate is set, report that.
        */
        imr->ifm_active &= ~IFM_TMASK;
        if ( rate & 0x80) {
            imr->ifm_active |= IFM_IEEE80211_HT_MCS;
        }
        else {
            imr->ifm_active |= rate2media(vap,rate,mode);
        }
    }
    else {
        imr->ifm_active |= IFM_AUTO;
    }
    imr->ifm_current = imr->ifm_active;
}


static int os_if_media_init(osif_dev *osdev)
{
    wlan_if_t vap;
    int i,mopt;
    u_int16_t nmodes;
    enum ieee80211_phymode modes[IEEE80211_MODE_MAX];
    struct ifmedia *media = &osdev->os_media;
    struct ifmediareq imr;
    ifmedia_init(media, 0, osif_media_change, osif_media_status);

    vap = osdev->os_if;
    if (wlan_get_supported_phymodes(osdev->os_devhandle,modes,
                                    &nmodes,IEEE80211_MODE_MAX) != 0 ) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : get_supported_phymodes failed \n", __func__);
        return -EINVAL;
    }
    for (i=0;i<nmodes;++i) {
        mopt = mopts[(u_int32_t)modes[i]];
        IEEE80211_ADD_MEDIA(media, IFM_AUTO, mopt); /* e.g. 11a auto */
        IEEE80211_ADD_MEDIA(media, IFM_AUTO, mopt | IFM_IEEE80211_ADHOC);
        IEEE80211_ADD_MEDIA(media, IFM_AUTO, mopt | IFM_IEEE80211_HOSTAP);
        IEEE80211_ADD_MEDIA(media, IFM_AUTO, mopt | IFM_IEEE80211_ADHOC | IFM_FLAG0);
        IEEE80211_ADD_MEDIA(media, IFM_AUTO, mopt | IFM_IEEE80211_MONITOR);
    }

    osif_media_status(osdev->netdev, &imr);
    ifmedia_set(media, imr.ifm_active);
    return 0;
}

void osif_mlme_notify_handler(wlan_if_t vap,
        enum wlan_serialization_cmd_type cmd_type)
{
    osif_dev *osdev = (osif_dev *)vap->iv_ifp;
    struct ieee80211com *ic = vap->iv_ic;
    union iwreq_data wreq;
    enum ieee80211_opmode opmode;
    struct net_device *dev;
    wlan_if_t tmp_vap;

    opmode = wlan_vap_get_opmode(vap);
    dev = get_cfg80211_notification_ndev(osdev);

    if (!dev)
        return;

    switch (cmd_type) {
    case WLAN_SER_CMD_VDEV_START_BSS:
    case WLAN_SER_CMD_VDEV_CONNECT:
        break;
    case WLAN_SER_CMD_VDEV_STOP_BSS:
    case WLAN_SER_CMD_VDEV_DISCONNECT:
#if UMAC_SUPPORT_CFG80211
        if (opmode == IEEE80211_M_HOSTAP && vap->iv_cfg80211_create) {
            if (osdev->scan_req) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
                struct cfg80211_scan_info info = {
                        .aborted = true,
                };
                cfg80211_scan_done(osdev->scan_req, &info); /* aborted*/
#else
                cfg80211_scan_done(osdev->scan_req, true); /* aborted*/
#endif
                osdev->scan_req = NULL;
            }
        }
#endif
        if (osdev->is_delete_in_progress) {
            OS_DELAY(5000);
            delete_default_vap_keys(vap);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : Keys Deleted \n",__func__);
        }

        if (opmode == IEEE80211_M_HOSTAP) {
#if ATH_SUPPORT_WAPI
            osif_wapi_rekeytimer_stop((os_if_t)osdev);
#endif
        }
#if UNIFIED_SMARTANTENNA
        if (opmode == IEEE80211_M_STA) {
            QDF_STATUS status = wlan_objmgr_vdev_try_get_ref(vap->vdev_obj, WLAN_SA_API_ID);
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("unable to get reference");
            } else {
                wlan_sa_api_stop(vap->iv_ic->ic_pdev_obj, vap->vdev_obj, SMART_ANT_RECONFIGURE);
                wlan_sa_api_start(vap->iv_ic->ic_pdev_obj, vap->vdev_obj, SMART_ANT_STA_NOT_CONNECTED | SMART_ANT_RECONFIGURE);
                (vap->iv_ic)->sta_not_connected_cfg = TRUE;
                wlan_objmgr_vdev_release_ref(vap->vdev_obj, WLAN_SA_API_ID);
            }
        }
#endif
        ic->vap_down_in_progress = FALSE;

        wlan_vap_down_check_beacon_interval(vap, opmode);
        if (ic->ic_need_vap_reinit) {
            osif_cmd_schedule_req(vap->vdev_obj, OSIF_CMD_OTH_VDEV_STOP_START);
            ic->ic_need_vap_reinit = 0;
        }

        /* If there are only non-beaconing VAPs on this ic, then deliver CSA_COMPLETE,
         * as it might be stuck on CSA_RESTART state */
        if (!ieee80211_get_num_beaconing_vaps(ic)
            && vap->iv_opmode == IEEE80211_M_HOSTAP) {
            TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                if (tmp_vap != vap && (tmp_vap->iv_opmode == IEEE80211_M_MONITOR
                    || wlan_is_non_beaconing_ap_vap(tmp_vap))) {
                    IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_DOTH,
                            "%s:%d tmp_vap:%s vdev:%d pdev:%d psoc:%d state:%d substate:%d opmode:%d iv_special_vap:%d iv_smart_monitor:%d",
                            __func__, __LINE__,
                            tmp_vap->iv_netdev_name,
                            wlan_vdev_get_id(tmp_vap->vdev_obj),
                            wlan_objmgr_pdev_get_pdev_id(tmp_vap->iv_ic->ic_pdev_obj),
                            wlan_psoc_get_id(wlan_pdev_get_psoc(tmp_vap->iv_ic->ic_pdev_obj)),
                            wlan_vdev_mlme_get_state(tmp_vap->vdev_obj),
                            wlan_vdev_mlme_get_substate(tmp_vap->vdev_obj),
                            tmp_vap->iv_opmode, tmp_vap->iv_special_vap_mode,
                            tmp_vap->iv_smart_monitor_vap);
                    mlme_vdev_sm_deliver_csa_complete(tmp_vap->vdev_obj);
                }
            }
        }

        if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM,
                    IEEE80211_EV_IF_NOT_RUNNING)) {
            memset(&wreq, 0, sizeof(wreq));
            wreq.data.flags = IEEE80211_EV_IF_NOT_RUNNING;
            WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, NULL);
        }
        break;
    default:
        break;
    }
}

#ifdef WLAN_FEATURE_11BE_MLO
void osif_vap_check_blocking_cmd_on_mlo(struct wlan_objmgr_vdev *vdev)
{
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    uint16_t num_links = 0;
    uint8_t i;
    qdf_event_t wait_event;
    int waitcnt = 0;
    bool blocking_cmd;

    if (!(vdev->mlo_dev_ctx && wlan_vdev_mlme_is_mlo_ap(vdev))) {
        mlo_debug("Not an MLO vdev, returning");
        return;
    }

    qdf_mem_zero(&wait_event, sizeof(wait_event));
    qdf_event_create(&wait_event);
    qdf_event_reset(&wait_event);

    mlo_ap_get_vdev_list(vdev, &num_links, vdev_list);
    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
         mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                 num_links, QDF_ARRAY_SIZE(vdev_list));
         for (i = 0; i < QDF_ARRAY_SIZE(vdev_list); i++)
             mlo_release_vdev_ref(vdev_list[i]);
         return;
    }

    while (waitcnt < OSIF_MAX_STOP_ML_VAP_TIMEOUT_CNT) {
        blocking_cmd = false;
        for (i = 0; i < num_links; i++) {
             if (wlan_serialization_is_blocking_non_scan_cmd_waiting(
                         wlan_vdev_get_pdev(vdev_list[i]))) {
                 blocking_cmd = true;
                 qdf_wait_single_event(&wait_event, 1000);
                 waitcnt++;
                 break;
             }
        }
        if (blocking_cmd == false)
            break;
    }
    qdf_event_destroy(&wait_event);

    if (waitcnt == OSIF_MAX_STOP_ML_VAP_TIMEOUT_CNT) {
        mlo_err("Blocking command present for MLO vdev in Down path, check..!");
        /*
         * On timeout dump sm history across the all pdevs.
         */
        for (i = 0; i < num_links; i++) {
            mlo_err("Serialization history of PSOC:%u, PDEV:%u",
                    wlan_vdev_get_psoc_id(vdev_list[i]),
                    wlan_objmgr_pdev_get_pdev_id(wlan_vdev_get_pdev(vdev_list[i])));
            wlan_ser_print_history(vdev_list[i], WLAN_SER_CMD_NONSCAN, 0);
        }
    }

    for (i = 0; i < num_links; i++)
        mlo_release_vdev_ref(vdev_list[i]);
}

void osif_vap_bringdown_mlo(struct wlan_objmgr_vdev *vdev)
{
    uint8_t i;
    uint16_t num_links = 0;
    struct ieee80211vap *tmp_vap = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic;
    struct net_device *tmp_dev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap)
        return;

    if (!(vdev->mlo_dev_ctx && wlan_vdev_mlme_is_mlo_ap(vdev))) {
        mlo_debug("Not an MLO vdev, returning");
        return;
    }

    psoc = wlan_pdev_get_psoc(wlan_vdev_get_pdev(vdev));

    if (!psoc) {
        return;
    }

    ic = vap->iv_ic;
    if (!ic)
        return;

    /* Link removal, skip partner link bringdown sequence */
    if (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
        ic->ic_check_mlo_recovery(psoc)) {
        mlo_debug("ML AP removal in progress");
        return;
    }

    mlo_ap_get_active_vdev_list(vdev, &num_links, vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
         mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                 num_links, QDF_ARRAY_SIZE(vdev_list));
         for (i = 0; i < QDF_ARRAY_SIZE(vdev_list); i++)
             mlo_release_vdev_ref(vdev_list[i]);
         return;
    }

    for (i = 0; i < num_links; i++) {
        tmp_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
        if ((vdev_list[i] == vdev) || !tmp_vap) {
            mlo_release_vdev_ref(vdev_list[i]);
            continue;
        }
        tmp_dev = get_cfg80211_notification_ndev(((osif_dev *)tmp_vap->iv_ifp));
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        if (tmp_dev && (tmp_dev->flags & IFF_RUNNING) &&
            !((osif_dev *)tmp_vap->iv_ifp)->stop_initiated) {
#else
        if (tmp_dev && (tmp_dev->flags & IFF_RUNNING)) {
#endif
            if (osif_vap_stop_wrapper(((osif_dev *)tmp_vap->iv_ifp)))
                mlo_err("PSOC%d-VDEV%d Partner mlo vdev stop failed",
                        wlan_psoc_get_id(wlan_vdev_get_psoc(vdev_list[i])),
                        wlan_vdev_get_id(vdev_list[i]));
        }
        mlo_release_vdev_ref(vdev_list[i]);
    }
}

/*
 * Call dev_close() for partner-vdev's when current MLO-vdev is down
 *
 * NOTE: this function should be called only after osif_vap_bringdown_mlo()
 * and from netdev_notifier call chain to properly bring-down vdev's and
 * update netdev flags
 */
void osif_vap_netdev_close_mlo(struct wlan_objmgr_vdev *vdev)
{
    uint8_t i;
    uint16_t num_links = 0;
    struct ieee80211vap *tmp_vap = NULL;
    struct net_device *tmp_dev = NULL;
    struct ieee80211com *ic;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    bool rtnl_lock_acquired = false;

    tmp_vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!tmp_vap)
        return;

    if (!vdev->mlo_dev_ctx) {
        mlo_debug("Not an MLO vdev, returning");
        return;
     }

    ic = tmp_vap->iv_ic;
    if (ic && ic->recovery_in_progress) {
        mlo_debug("FW recovery is in progress");
        return;
    }

    if (tmp_vap->ml_ap_removal_in_progress) {
        mlo_debug("ML AP removal is in progress");
        return;
    }

    mlo_ap_get_partner_vdev_list_from_mld(vdev, &num_links, vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
         mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu, investigate",
                 num_links, QDF_ARRAY_SIZE(vdev_list));
         for (i = 0; i < QDF_ARRAY_SIZE(vdev_list); i++)
             mlo_release_vdev_ref(vdev_list[i]);
         return;
    }

    for (i = 0; i < num_links; i++) {
        tmp_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
        if ((vdev_list[i] == vdev) || !tmp_vap) {
            mlo_release_vdev_ref(vdev_list[i]);
            continue;
        }
        tmp_dev = get_cfg80211_notification_ndev((osif_dev *)tmp_vap->iv_ifp);
        /*
         * In scenarios where hostapd is killed or abrupt system reboot is
         * issued, RTNL lock is not held leading to assert.
         * Hence acquire lock if not held earlier.
         */
        if (tmp_dev && (tmp_dev->flags & IFF_UP)) {
            if (!rtnl_is_locked()) {
                rtnl_lock();
                rtnl_lock_acquired = true;
            }
            dev_close(tmp_dev);
            if (rtnl_lock_acquired) {
                rtnl_unlock();
            }
        }
        mlo_release_vdev_ref(vdev_list[i]);
    }
}
qdf_export_symbol(osif_vap_netdev_close_mlo);

void osif_peer_update_mlo_sta_dev(struct wlan_objmgr_peer *peer,
                                struct wlan_mlo_peer_context *ml_peer)
{
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_vdev *tmp_vdev = NULL;
    struct ieee80211vap *tmp_vap = NULL;
    osif_dev *osdev;
    struct net_device *dev;
    uint8_t i = 0;

    if (!ml_peer)
        return;

    vdev = wlan_peer_get_vdev(peer);
    if (!vdev)
        return;

    if (wlan_vdev_mlme_get_opmode(vdev) != QDF_STA_MODE)
        return;

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
         peer_entry = &ml_peer->peer_list[i];
         if (!peer_entry->link_peer)
             continue;

         tmp_vdev = wlan_peer_get_vdev(peer_entry->link_peer);
         if (!tmp_vdev)
             continue;

         tmp_vap = wlan_vdev_get_mlme_ext_obj(tmp_vdev);
         if (!tmp_vap)
             continue;

         osdev = (osif_dev *)tmp_vap->iv_ifp;
         dev = osdev->netdev;

         if (peer_entry->link_peer == peer) {
             dev->flags |= IFF_RUNNING;
             continue;
         }
         dev->flags &= ~IFF_RUNNING;
    }
}

qdf_export_symbol(osif_peer_update_mlo_sta_dev);
#endif

#ifdef WLAN_FEATURE_11BE_MLO
static int
osif_mlo_dev_open(struct net_device *dev)
{
    dev->flags |= IFF_RUNNING;
    return 0;
}

static int
osif_mlo_dev_stop(struct net_device *dev)
{
    dev->flags &= ~IFF_RUNNING;
    return 0;
}

static void
osif_mld_getstats(struct net_device *mld_dev, struct rtnl_link_stats64* stats64)
{
    struct osif_mldev *osifp_mldev = ath_netdev_priv(mld_dev);
    struct net_device *dev;
    osif_dev *link_dev;
    int i, j;
    struct cdp_vdev_stats *vdev_stats = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint8_t stats_collected = 0;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;
    ol_txrx_soc_handle soc_txrx_handle;
    cdp_config_param_type value = {0};

    vdev_stats = qdf_mem_malloc_atomic(sizeof(struct cdp_vdev_stats));
    if (!vdev_stats) {
         qdf_err("Allocation Failed!");
         return;
    }

    for (i = 0; i < MAX_MLO_CHIPS; i++) {
        for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
            link_dev = osifp_mldev->link_dev[i][j];
            if (!link_dev)
                continue;

            dev = link_dev->netdev;
            if (!dev)
                continue;
            /*
             * In case of fw recovery, VAPs will go down but VP's will
             * still be there. Hence this callback will keep on coming even if VAP object
             * is down in recovery mode. So we check the recovery mode using the netdev
             * queue and return if netdev queue is stopped.
             */

            if (netif_queue_stopped(dev))
                continue;

            vdev = link_dev->ctrl_vdev;
            if (!vdev) {
                qdf_err("Invalid vdev");
                goto fail;
            }

            psoc = wlan_vdev_get_psoc(vdev);
            if (!psoc) {
                qdf_err("Invalid Psoc");
                goto fail;
            }

             soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
             if (!soc_txrx_handle)
                 goto fail;

            cdp_txrx_get_psoc_param(soc_txrx_handle, CDP_CFG_VDEV_STATS_HW_OFFLOAD,
                                    &value);

            ret = cdp_mlo_get_mld_vdev_stats(soc_txrx_handle,
                                             wlan_vdev_get_id(vdev),
                                             vdev_stats, false);

            if (ret != QDF_STATUS_SUCCESS) {
                 qdf_debug("Unable to get Vdev Stats!");
                 goto fail;
            }

            stats64->rx_errors = vdev_stats->rx.err.mic_err +
                                 vdev_stats->rx.err.decrypt_err +
                                 vdev_stats->rx.err.fcserr +
                                 vdev_stats->rx.err.pn_err +
                                 vdev_stats->rx.err.oor_err +
                                 vdev_stats->rx.err.jump_2k_err +
                                 vdev_stats->rx.err.rxdma_wifi_parse_err;
            stats64->rx_dropped = vdev_stats->rx.mec_drop.num +
                                  vdev_stats->rx.multipass_rx_pkt_drop +
                                  vdev_stats->rx.peer_unauth_rx_pkt_drop +
                                  vdev_stats->rx.policy_check_drop +
                                  vdev_stats->rx.nawds_mcast_drop +
                                  vdev_stats->rx.mcast_3addr_drop;

            stats64->tx_packets = vdev_stats->tx.comp_pkt.num;
            stats64->tx_bytes = vdev_stats->tx.comp_pkt.bytes;
            stats64->tx_errors = vdev_stats->tx.tx_failed;
            stats64->tx_dropped = vdev_stats->tx_i.dropped.dropped_pkt.num +
                                  vdev_stats->tx_i.sg.dropped_host.num +
                                  vdev_stats->tx_i.mcast_en.dropped_map_error +
                                  vdev_stats->tx_i.mcast_en.dropped_self_mac +
                                  vdev_stats->tx_i.mcast_en.dropped_send_fail +
                                  vdev_stats->tx.nawds_mcast_drop;
            if (value.cdp_psoc_param_vdev_stats_hw_offload) {
                stats64->rx_packets = vdev_stats->rx_i.reo_rcvd_pkt.num +
                                      vdev_stats->rx_i.null_q_desc_pkt.num +
                                      vdev_stats->rx_i.routed_eapol_pkt.num;
                stats64->rx_bytes = vdev_stats->rx_i.reo_rcvd_pkt.bytes +
                                    vdev_stats->rx_i.null_q_desc_pkt.bytes +
                                    vdev_stats->rx_i.routed_eapol_pkt.bytes;
            } else {
                stats64->rx_packets = vdev_stats->rx.to_stack.num;
                stats64->rx_bytes = vdev_stats->rx.to_stack.bytes;
            }
            stats_collected = 1;
            break;
        }
        if (stats_collected)
             break;
    }
fail:
    qdf_mem_free(vdev_stats);
}

static int
osif_mldev_vlan_add_vid(struct net_device *dev, __be16 proto, unsigned short vid)
{
    return 0;
}

static int
osif_mldev_vlan_kill_vid(struct net_device *dev, __be16 proto, unsigned short vid)
{
    return 0;
}

static int
osif_mld_change_mtu(struct net_device *dev, int mtu)
{
#if QCA_PPE_VP
    struct osif_mldev *mldev = osif_mld_get_mldev(dev);

    if (!mldev)
        return -EINVAL;
#endif

    if (!(IEEE80211_MTU_MIN < mtu && mtu < IEEE80211_MTU_MAX))
        return -EINVAL;

    dev->mtu = mtu;

#if QCA_PPE_VP
    if(!osif_ppe_vp_set_mtu(dev, mldev->ppe_vp_num, mtu)) {
        return -EINVAL;
    }
#endif

    return 0;
}

static int
osif_mld_change_mac_addr(struct net_device *dev, void *mac_addr)
{
    struct sockaddr *sock_addr = (struct sockaddr *)mac_addr;
#if QCA_PPE_VP
    struct osif_mldev *mldev = osif_mld_get_mldev(dev);

    if (!mldev)
        return -EINVAL;
#endif

    netdev_dbg(dev, "AddrFamily: %d, %0x:%0x:%0x:%0x:%0x:%0x\n",
            sock_addr->sa_family, sock_addr->sa_data[0], sock_addr->sa_data[1],
            sock_addr->sa_data[2], sock_addr->sa_data[3], sock_addr->sa_data[4],
            sock_addr->sa_data[5]);

    memcpy(dev->dev_addr, sock_addr->sa_data, ETH_ALEN);

#if QCA_PPE_VP
    osif_ppe_vp_set_mac_addr(dev, mldev->ppe_vp_num, sock_addr->sa_data);
#endif

    /* XXX coordinate with parent device */
    return 0;
}

static struct net_device_ops osif_mld_dev_ap_ops = {
    .ndo_get_stats64 = osif_mld_getstats,
    .ndo_open = osif_mlo_dev_open,
    .ndo_stop = osif_mlo_dev_stop,
    .ndo_start_xmit = osif_ap_mlo_xmit_netdev,
    .ndo_select_queue = osif_select_queue,
    .ndo_vlan_rx_add_vid = osif_mldev_vlan_add_vid,
    .ndo_vlan_rx_kill_vid = osif_mldev_vlan_kill_vid,
    .ndo_change_mtu = osif_mld_change_mtu,
    .ndo_set_mac_address = osif_mld_change_mac_addr,
};

static struct net_device_ops osif_mld_dev_sta_ops = {
    .ndo_get_stats64 = osif_mld_getstats,
    .ndo_open = osif_mlo_dev_open,
    .ndo_stop = osif_mlo_dev_stop,
    .ndo_start_xmit = osif_sta_mlo_xmit_netdev,
    .ndo_select_queue = osif_select_queue,
    .ndo_vlan_rx_add_vid = osif_mldev_vlan_add_vid,
    .ndo_vlan_rx_kill_vid = osif_mldev_vlan_kill_vid,
    .ndo_change_mtu = osif_mld_change_mtu,
    .ndo_set_mac_address = osif_mld_change_mac_addr,
};

static QDF_STATUS osif_mldev_add_mld_osdev(struct osif_mldev *osifp_mld,
                                           uint8_t grp_id)
{
    struct cfg80211_mld_wiphy_context *mld_ctx;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_priv;
#else
    struct cfg80211_mld_wiphy_priv *mld_priv;
#endif
    int i;

    if (grp_id >= WLAN_MAX_MLO_GROUPS ) {
        qdf_err("Invalid grp_id");
        return QDF_STATUS_E_FAILURE;
    }

    mld_ctx = &mld_wiphy_ctx[grp_id];
    mld_priv = wiphy_priv(mld_ctx->mld_wiphy);

    for (i = 0; i < MAX_MLDEV; i++) {
        if (mld_priv->mldev[i])
            continue;
        mld_priv->mldev[i] = osifp_mld;
        return QDF_STATUS_SUCCESS;
    }

    return QDF_STATUS_E_FAILURE;
}

static QDF_STATUS osif_mldev_remove_mld_osdev(struct osif_mldev *osifp_mld)
{
    uint8_t grp_id;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_priv = wiphy_priv(osifp_mld->wdev.wiphy);
#else
    struct cfg80211_mld_wiphy_priv *mld_priv = wiphy_priv(osifp_mld->wdev.wiphy);
#endif
    struct net_device *mld_dev = osifp_mld->mld_dev;
    int i;

    qdf_debug("The wiphy is pointing to:%pK", osifp_mld->wdev.wiphy);

    grp_id = mld_priv->ml_grp_id;
    qdf_info("%s: The mld grp_id is: %d", mld_dev->name, grp_id);

    if (grp_id >= WLAN_MAX_MLO_GROUPS ) {
        qdf_err("Invalid grp_id");
        return QDF_STATUS_E_FAILURE;
    }

    /* in error cases multiple mld's can have the same mac address,
     * comparison of mac address will result in stale entries in the list.
     * therefore migrating the comparison to the mld priv pointer
     */

    for (i = 0; i < MAX_MLDEV; i++) {
        if (mld_priv->mldev[i]) {
            if (osifp_mld == mld_priv->mldev[i]) {
                mld_priv->mldev[i] = NULL;
                return QDF_STATUS_SUCCESS;
            }
        }
    }

    return QDF_STATUS_E_FAILURE;
}

static uint8_t osif_mldev_get_num_osdev(struct osif_mldev *osifp_mld)
{
    uint8_t count = 0;
    uint8_t psoc_id = 0;
    uint8_t pdev_id = 0;

    for (psoc_id = 0; psoc_id < MAX_MLO_CHIPS; psoc_id++) {
        for (pdev_id = 0; pdev_id < MAX_PDEV_PER_SOC; pdev_id++) {
            if (osifp_mld->link_dev[psoc_id][pdev_id])
                count ++;
        }
    }
    return count;
}

/* 4 bits to store regular mlo hw link id
 * 5th bit will be for bridge VAP
 */
#define HW_LINK_ID_FOR_BRIDGE_VAP_SHIFT 4
#define DERIVE_HW_LINK_ID_FOR_BRIDGE_VAP(id) \
        (id | 1 << HW_LINK_ID_FOR_BRIDGE_VAP_SHIFT)

static void osif_mld_map_link_dev(osif_dev *osdev)
{
    struct osif_mldev *osifp_mldev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_vdev *vdev;
    uint8_t psoc_id, pdev_id, chip_id;
    uint16_t hw_link_id = INVALID_HW_LINK_ID;
    struct ieee80211com *ic;
    struct ieee80211vap *vap;

    vap = osdev->os_if;
    if (!vap)
        return;

    ic = vap->iv_ic;
    osifp_mldev = osdev->mldev;

    vdev = osdev->ctrl_vdev;
    psoc = wlan_vdev_get_psoc(vdev);
    pdev = wlan_vdev_get_pdev(vdev);

    psoc_id = wlan_psoc_get_id(psoc);
    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
    chip_id = wlan_mlo_get_psoc_mlo_chip_id(psoc);

    if (chip_id == WLAN_MLO_CHIP_ID_INVALID) {
        qdf_err("MLD config invalid (or) chip_id (%u) wrongly populated for "
                "psoc_id %u of target_type %u",
                 chip_id, psoc_id, ic->ic_get_tgt_type(ic));
        qdf_assert_always(0);
    }

    qdf_assert_always(chip_id < MAX_MLO_CHIPS && pdev_id < MAX_PDEV_PER_SOC);

    hw_link_id = wlan_mlo_get_pdev_hw_link_id(pdev);
    if (hw_link_id == INVALID_HW_LINK_ID || hw_link_id > MAX_POSSIBLE_LINK_VAPS_PER_MLD) {
        qdf_err("hw_link_id in chip_id (%u) wrongly populated for "
                "psoc_id %u of target_type %u",
                 chip_id, psoc_id, ic->ic_get_tgt_type(ic));
        qdf_assert_always(0);
    }

    if (osdev->is_bridge_link) {
        osifp_mldev->bridge_dev[chip_id][pdev_id] = osdev;
        hw_link_id = DERIVE_HW_LINK_ID_FOR_BRIDGE_VAP(hw_link_id);
        osifp_mldev->link_vaps[hw_link_id] = osdev;
    } else {
        osifp_mldev->link_dev[chip_id][pdev_id] = osdev;
        osifp_mldev->link_vaps[hw_link_id] = osdev;
    }

    osdev->mlo_link_id = hw_link_id;

    /* In STA mode, while adding the first slave update the primary umac
     * this is to handle downgrade scenarios.
     */
     if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) &&
         (osif_mldev_get_num_osdev(osifp_mldev) == 1)) {
         osifp_mldev->primary_chip_id = chip_id;
         osifp_mldev->primary_pdev_id = pdev_id;
     }
    qdf_info("%s: psoc_id %u pdev_id %u chip_id %u is_bridge_link = %d"
             "mlo_link_id= %u", osifp_mldev->mld_dev->name, psoc_id, pdev_id,
             chip_id, osdev->is_bridge_link, osdev->mlo_link_id);
}

static void osif_mld_unmap_link_dev(osif_dev *osdev)
{
    struct osif_mldev *osifp_mldev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_vdev *vdev;
    uint8_t psoc_id, pdev_id, chip_id;

    osifp_mldev = osdev->mldev;

    vdev = osdev->ctrl_vdev;
    psoc = wlan_vdev_get_psoc(vdev);
    pdev = wlan_vdev_get_pdev(vdev);

    psoc_id = wlan_psoc_get_id(psoc);
    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
    chip_id = wlan_mlo_get_psoc_mlo_chip_id(psoc);

    qdf_assert_always(chip_id < MAX_MLO_CHIPS && pdev_id < MAX_PDEV_PER_SOC);

    if (osdev->is_bridge_link) {
        osifp_mldev->bridge_dev[chip_id][pdev_id] = NULL;
	osifp_mldev->link_vaps[osdev->mlo_link_id] = NULL;
    } else {
        osifp_mldev->link_dev[chip_id][pdev_id] = NULL;
	osifp_mldev->link_vaps[osdev->mlo_link_id] = NULL;
    }

    qdf_info("%s: psoc_id %u pdev_id %u chip_id %u is_bridge_link = %d"
             "mlo_link_id= %u", osifp_mldev->mld_dev->name, psoc_id, pdev_id,
             chip_id, osdev->is_bridge_link, osdev->mlo_link_id);
}

/*
 * osif_mldev_bridge_link_add: add bridge link to the mldev
 * @osifp_br : pointer to bridge vap osdev
 * @mldev : pointer to mldev context.
 *
 * returns true/false on success/fail
 */
bool osif_mldev_bridge_link_add(osif_dev *osifp_br, struct osif_mldev *mldev)
{
    if (!mldev || !mldev->mld_dev || !osifp_br)
        return false;

    osifp_br->mldev = mldev;
    osifp_br->rx_dev = osif_mld_get_rx_ndev(mldev->mld_dev, osifp_br->netdev);
    /* hold the reference of rx_dev */
    qdf_net_if_hold_dev((struct qdf_net_if *)osifp_br->rx_dev);

    /* update the bridge dev */
    osifp_br->is_bridge_link = 1;
    osif_mld_map_link_dev(osifp_br);

    return true;
}

/*
 * osif_mldev_bridge_link_delete: delete bridge link from mld.
 * @osdev : pointer to bridge vap osdev
 *
 * Return: None
 */
void osif_mldev_bridge_link_delete(osif_dev *osifp_br)
{
    if (!osifp_br || !osifp_br->rx_dev || !osifp_br->mldev)
        return;

    if (!osifp_br->is_bridge_link)
        return;

    /* release the bridge dev */
    osif_mld_unmap_link_dev(osifp_br);

    /* release the reference of rx_dev */
    qdf_net_if_release_dev((struct qdf_net_if *)osifp_br->rx_dev);
    osifp_br->mldev = NULL;
    osifp_br->rx_dev =  NULL;
}

#ifdef WLAN_FEATURE_11BE_MLO
void wlan_set_ssid_mlo_bridge_vap(wlan_if_t vap)
{
    ieee80211_ssid tmpssid;

    /* Set ESSID for bridge VAP */
    tmpssid.len = 7;
    qdf_mem_copy(tmpssid.ssid, "ssid_br", 7);
    tmpssid.ssid[tmpssid.len] = '\0';
    ieee80211_ucfg_set_essid(vap, &tmpssid, 0);
}

/*
 * wlan_delete_mlo_bridge_vap: delete bridge link.
 * @vap : pointer to bridge vap
 * @recover : recovery delete flag
 *
 * Return: 0 success; otherwise failure
 */
int wlan_delete_mlo_bridge_vap(wlan_if_t vap, int recover)
{
    osif_dev *osifp = (osif_dev *)vap->iv_ifp;
    struct wlan_objmgr_pdev *pdev;
    struct ieee80211com *ic;
    int ret;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    ic = vap->iv_ic;

    /* Return if VAP is already being deleted */
    IEEE80211_COMM_LOCK_BH(ic);
    if (osifp->is_delete_in_progress) {
        IEEE80211_COMM_UNLOCK_BH(ic);
        return 0;
    }
    osifp->is_delete_in_progress = 1;
    IEEE80211_COMM_UNLOCK_BH(ic);

    osifp->is_deleted = 0;
    vap->iv_is_mlo_bridge_up = false;

    ic->ic_vap_free_macaddr(ic, vap->iv_myaddr);

    if (recover) {
        qdf_info("Recovering MLO bridge VAP %s", ether_sprintf(vap->iv_myaddr));
        ieee80211_flush_vap_mgmt_queue(vap, true);
        wlan_vap_recover(vap);
        ret = wlan_delete_vap_wait_and_free(vap, 1);
        osifp->is_delete_in_progress = 0;
    } else {
        qdf_info("Deleting MLO bridge VAP %s", ether_sprintf(vap->iv_myaddr));
        osif_mldev_bridge_link_delete(osifp);

        if (osif_brvap_is_bonding_mode(wlan_pdev_get_psoc(pdev)))
            ret = osif_ioctl_delete_vap(get_cfg80211_notification_ndev(osifp),
                                        vap->link_id);
        else {
            wlan_vap_delete(vap);
            ret = wlan_delete_vap_wait_and_free(vap, 0);
            qdf_mem_free(osifp);
        }
    }
    /* Decrement the pdev bridge vap count */
    wlan_pdev_dec_mlo_bridge_vdev_count(pdev);

    return ret;
}
qdf_export_symbol(wlan_delete_mlo_bridge_vap);

#define MLO_MAX_BRIDGE_ACROSS_CHIPS 32
QDF_STATUS osif_mlo_delete_bridge_vaps(void)
{

    struct wlan_objmgr_vdev *br_vdev_list[MLO_MAX_BRIDGE_LINKS_PER_MLD] = {NULL};
    wlan_if_t br_vap[MLO_MAX_BRIDGE_ACROSS_CHIPS]= {NULL};
    struct wlan_mlo_dev_context *mld_cur;
    struct wlan_mlo_dev_context *mld_next;
    qdf_list_t *ml_list;
    uint8_t j = 0;
    uint8_t i;
    struct mlo_mgr_context *mlo_mgr_ctx = wlan_objmgr_get_mlo_ctx();

    if (!mlo_mgr_ctx)
        return QDF_STATUS_E_NULL_VALUE;

    ml_link_lock_acquire(mlo_mgr_ctx);
    ml_list = &mlo_mgr_ctx->ml_dev_list;
    /* Get first mld context */
    mld_cur = wlan_mlo_list_peek_head(ml_list);
    /**
     * Iterate through ml list, till ml mldaddr matches with
     * entry of list
     */
    while (mld_cur) {
        if (j > (MLO_MAX_BRIDGE_ACROSS_CHIPS - MLO_MAX_BRIDGE_LINKS_PER_MLD)) {
            qdf_err("Max number of bridge across device reached!!");
            break;
        }
        for (i = 0; i < MLO_MAX_BRIDGE_LINKS_PER_MLD; i++) {
            mlo_dev_lock_acquire(mld_cur);
            br_vdev_list[i] = mld_cur->wlan_bridge_vdev_list[i];
            mlo_dev_lock_release(mld_cur);
            br_vap[j++]= wlan_vdev_get_mlme_ext_obj(br_vdev_list[i]);
        }
        /* get next mld node */
        mld_next = wlan_mlo_get_next_mld_ctx(ml_list, mld_cur);
        mld_cur = mld_next;
    }
    ml_link_lock_release(mlo_mgr_ctx);

    for (j = 0; j < MLO_MAX_BRIDGE_ACROSS_CHIPS;j++) {
        if (br_vap[j])
            wlan_delete_mlo_bridge_vap(br_vap[j], 0);
    }

return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(osif_mlo_delete_bridge_vaps);
/*
 * wlan_init_mlo_bridge_vap: Init and bringup bridge link.
 * @vap : pointer to bridge vap
 *
 * Return: 0 success; otherwise fail
 */
int wlan_init_mlo_bridge_vap(wlan_if_t vap)
{
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211com *ic = vap->iv_ic;
    wlan_chan_t chan = &chan_obj;
    struct chan_params chan_p = {0};

    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap))
        return -1;

    /*
     * Set current channel from pdev if available otherwise
     * pick the first channel supported.
     */
    *chan = wlan_get_current_channel(vap, false);
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        chan_p.freq = wlan_channel_frequency(chan);
        chan_p.cfreq2 = chan->ic_vhtop_freq_seg2;
        vap->iv_des_mode = ieee80211_get_phymode_from_chan_flag(chan,chan->ic_flags);
        vap->iv_des_hw_mode = vap->iv_des_mode;
        qdf_info("Using current channel for bridge VAP %d", chan_p.freq);
    } else {
        if (wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_2G)) {
            vap->iv_des_mode = IEEE80211_MODE_11BEG_EHT20;
            vap->iv_des_hw_mode = vap->iv_des_mode;
        } else if (wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_5G) ||
                 wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_6G)) {
            vap->iv_des_mode = IEEE80211_MODE_11BEA_EHT20;
            vap->iv_des_hw_mode = vap->iv_des_mode;
        } else {
            qdf_err("invalid band derived from channel for bridge VAP [%u, %u]",
                    wlan_vdev_get_id(vap->vdev_obj),
                    wlan_vdev_get_psoc_id(vap->vdev_obj));
            return -1;
        }

        chan_p.freq = ieee80211_reg_find_first_valid_freq(vap->iv_ic,
                                                          vap->iv_des_mode,
                                                          REG_CURRENT_PWR_MODE);
        *chan = ieee80211_find_dot11_channel(ic, &chan_p, vap->iv_des_mode);
        qdf_info("Picking first valid channel for bridge VAP %d", chan_p.freq);
    }

    *(vap->iv_des_chan[vap->iv_des_mode]) = *chan;

    vap->iv_is_mlo_bridge_up = true;

    return wlan_vap_init(vap);
}

static void bridge_vap_init(void *arg, wlan_if_t vap)
{
    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap))
        return;

    wlan_init_mlo_bridge_vap(vap);
}

void wlan_recover_bridge_vaps_init(struct ieee80211com *ic)
{
    wlan_iterate_vap_list(ic, bridge_vap_init, NULL);
}
qdf_export_symbol(wlan_recover_bridge_vaps_init);
#endif

#ifdef CONFIG_BOND_MOD_SUPPORT
void osif_mldev_link_add_bond(struct net_device *mld_dev,
                              osif_dev *osifp,
                              uint8_t link_id)
{
    struct net_device *link_dev = osifp->netdev;
    struct osif_mldev *osifp_mldev = osifp->mldev;
    int r;

    if (!osifp_mldev)
        return;

    if (!link_dev) {
        qdf_err("link netdev is NULL in bonding mode link_id = %d\n", link_id);
        return;
    }

    osif_mld_map_link_dev(osifp);

    r = qal_bond_create_slave(mld_dev, link_dev);
    if (r)
        qdf_err("Failed to add link dev %s - err %d", link_dev->name, r);
}

void osif_mldev_link_delete_bond(osif_dev *osifp)
{
    struct net_device *link_dev = osifp->netdev;
    struct osif_mldev *osifp_mldev;
    struct net_device *mld_dev;
    int r;

    osifp_mldev = osifp->mldev;
    if (!osifp_mldev)
        return;

    mld_dev = osifp_mldev->mld_dev;

    osif_mld_unmap_link_dev(osifp);

    if (!link_dev) {
        qdf_err("link netdev is NULL in bonding mode");
        return;
    }

    r = qal_bond_slave_release(mld_dev, link_dev);
    if (r)
        qdf_err("Failed to delete link dev %s - err %d", link_dev->name, r);
}

struct osif_mldev *osif_mldev_get_mldev_bond(struct net_device *dev)
{
    return (struct osif_mldev *)qal_bond_get_mlo_ctx(dev);
}

static inline
struct net_device *osif_mldev_get_rx_ndev_bond(struct net_device *mldev,
                                               struct net_device *dev)
{
    return dev;
}

static inline
struct net_device *osif_mld_get_mld_peer_rx_netdev_bond(
                                               struct net_device *mldev,
                                               const uint8_t *mac_addr)
{
    struct osif_mldev *mldev_ctx = osif_mld_get_mldev(mldev);

    if (!mldev_ctx)
        return NULL;

    return osifp_peer_find_hash_find_netdev(mldev_ctx, (uint8_t *)mac_addr);
}
#else
void osif_mldev_link_add_bond(struct net_device *mld_dev,
                              osif_dev *osifp,
                              uint8_t link_id)
{
}

void osif_mldev_link_delete_bond(osif_dev *osifp)
{
}

struct osif_mldev *osif_mldev_get_mldev_bond(struct net_device *dev)
{
    return NULL;
}

static inline
struct net_device *osif_mldev_get_rx_ndev_bond(struct net_device *mldev,
                                               struct net_device *dev)
{
    return NULL;
}

static inline
struct net_device *osif_mld_get_mld_peer_rx_netdev_bond(
                                               struct net_device *mldev,
                                               const uint8_t *mac_addr)
{
    return NULL;
}
#endif

/*
 * Fetch the value of ndev_name_ctrl ini param
 */
static bool osif_psoc_get_ini_ndev_name(void)
{
    uint8_t index = 0;
    struct wlan_objmgr_psoc *psoc;

    while (index < WLAN_OBJMGR_MAX_DEVICES) {
        if (g_umac_glb_obj->psoc[index]) {
            psoc = g_umac_glb_obj->psoc[index];
            if (cfg_get(psoc, CFG_OL_WILDCARD_NDEV_NAME))
                return true;
        }
        index++;
    }
    return false;
}

struct net_device *
osif_mldev_create_mld(struct wiphy* mld_wiphy, void *mld_params,
                      const char *iname, enum nl80211_iftype type)
{
    struct osif_mldev *osifp_mld;
    struct net_device *dev = NULL;
    int unit = -1, r;
    char name[IFNAMSIZ];
    int multiq_support_enabled = 1;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_cfg_ctx = NULL;
#else
    struct cfg80211_mld_wiphy_priv *mld_cfg_ctx = NULL;
#endif
    uint8_t grp_id;
#if QCA_PPE_VP
    struct ppe_vp_ai vpai;
    int32_t vpnum = 0;
#endif

     /*   If the ini param is set, then :
      *   - the netdev-name doesnt need to have the number attached
      *   - there is no need to look for the number in the string provided
      *     by the user
      *   - do not allow reuse of an existing netdev-name
      */
    if (!osif_psoc_get_ini_ndev_name()) {

        r = qdf_net_ifc_name2unit(iname, &unit);
        if (r < 0) {
            qdf_err("Failed to extract unit number");
            return NULL;
        }

        if (unit == -1) {
            unit = qdf_net_new_mldunit();
            if (unit == -1) {
                qdf_err("Failed to allocate unit number");
                return NULL;
            }
            r = snprintf(name, sizeof(name), "%s%d", iname, unit);
            if (r >= sizeof(name)) {
                qdf_err("source too long");
                return NULL;
            }
        } else {
            int dev_exist = qdf_net_dev_exist_by_name(iname);
            if (dev_exist) {
                qdf_err("%s net dev exists already", iname);
                return NULL;
            }

            qdf_net_alloc_mldunit(unit);
            if (strlcpy(name, iname, sizeof(name)) >=  sizeof(name)) {
                qdf_err("source too long");
                return NULL;
            }
            name[IFNAMSIZ - 1] = '\0';
        }
    } else {
            if (qdf_net_dev_exist_by_name(iname)) {
                qdf_err("%s net dev exists already", iname);
                return NULL;
            }

            if (strlcpy(name, iname, sizeof(name)) >=  sizeof(name)) {
                qdf_err("source too long");
                return NULL;
            }
            name[IFNAMSIZ - 1] = '\0';

            unit = qdf_net_new_mldunit();
    }


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
    if (multiq_support_enabled)
        dev = alloc_netdev_mqs(sizeof(struct osif_mldev), name, 0, ether_setup,
                OSIF_DP_NETDEV_TX_QUEUE_NUM, OSIF_DP_NETDEV_RX_QUEUE_NUM);
    else
        dev = alloc_netdev(sizeof(struct osif_mldev), name, 0, ether_setup);
#else
    if (multiq_support_enabled)
        dev = alloc_netdev_mqs(sizeof(struct osif_mldev), name, ether_setup,
                OSIF_DP_NETDEV_TX_QUEUE_NUM, OSIF_DP_NETDEV_RX_QUEUE_NUM);
    else
        dev = alloc_netdev(sizeof(struct osif_mldev), name, ether_setup);
#endif
    if (!dev) {
        qdf_err("Failed to create netdevice for %s", name);
        goto fail;
    }

    strlcpy(dev->name, name, sizeof(dev->name));
    dev->needed_headroom = sizeof (struct ieee80211_qosframe) + sizeof(struct llc) + QDF_MAC_ADDR_SIZE +
#if MESH_MODE_SUPPORT
    sizeof(struct meta_hdr_s) +
#endif
    IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
    qdf_info("%s: needed_headroom reservation %d", dev->name, dev->needed_headroom);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
    dev->max_mtu = ATH_MAX_MTU;
#endif
    qdf_net_update_net_device_dev_addr(dev, ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr, IEEE80211_ADDR_LEN);

    osifp_mld = ath_netdev_priv(dev);
    memset(osifp_mld, 0, sizeof(struct osif_mldev));

    osifp_mld->wdev.wiphy = mld_wiphy;
    osifp_mld->wdev.iftype = type;
    osifp_mld->dev_type = OSIF_NETDEV_TYPE_MLO;

    qdf_mem_copy(osifp_mld->mld_mac_addr, ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr, QDF_MAC_ADDR_SIZE);
    osifp_mld->mld_dev = dev;
    osifp_mld->pkt_fail_cnt = 0;
    osifp_mld->is_raw_mld = 0;
    osifp_mld->is_mesh_mld = 0;

    dev->ieee80211_ptr = &osifp_mld->wdev;

    if (osif_mlo_peer_find_hash_attach(osifp_mld, 1000))
        qdf_err("osif_peer hash attch failed");

    qdf_info("MLD iftype %u", type);
    if (type == NL80211_IFTYPE_AP) {
        dev->netdev_ops = &osif_mld_dev_ap_ops;
    } else if (type == NL80211_IFTYPE_STATION) {
        osifp_mld->wdev.use_4addr = 1;
        dev->netdev_ops = &osif_mld_dev_sta_ops;
    }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    /* tx_csum, SG and TSO will be supported on mld */
    osif_dev_set_offload_tx_csum(dev);
    osif_dev_set_sg(dev);
    osif_dev_set_tso(dev);

    mld_cfg_ctx = (struct cfg80211_context*)wiphy_priv(mld_wiphy);
#else
    mld_cfg_ctx = (struct cfg80211_mld_wiphy_priv*)wiphy_priv(mld_wiphy);
#endif
    grp_id = mld_cfg_ctx->ml_grp_id;
    qdf_debug("The mld grp_id is:%d", grp_id);

    if (grp_id >= WLAN_MAX_MLO_GROUPS ) {
        qdf_err("Invalid grp_id");
        goto fail;
    }

    if (osif_mldev_add_mld_osdev(osifp_mld, grp_id)) {
        qdf_err("Failed to add mldev %s", name);
        goto fail;
    }

    if (wlan_cfg80211_register_netdevice(dev)) {
        qdf_err("%s: unable to register device", name);
        goto fail;
    }

#if QCA_PPE_VP
    memset(&vpai, 0, sizeof(struct ppe_vp_ai));

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    /* create DS VP by default */
    vpai.usr_type = PPE_VP_USER_TYPE_DS;

    if (osifp_mld->is_valid_cp) {
        /* skip vp allocation */
        if (osifp_mld->cp.icp_flags & IEEE80211_PPE_VP_NO_ACCEL)
            goto set_file_attributes;

        if (osifp_mld->cp.icp_flags & IEEE80211_PPE_VP_DS_ACCEL)
            vpai.usr_type = PPE_VP_USER_TYPE_DS;
        else if (osifp_mld->cp.icp_flags & IEEE80211_PPE_VP_PASSIVE_ACCEL)
            vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
        else if (osifp_mld->cp.icp_flags & IEEE80211_PPE_VP_ACTIVE_ACCEL)
            vpai.usr_type = PPE_VP_USER_TYPE_ACTIVE;
    }
#else
    /* As of now we are supporting passive vp in non bonding mode */
    vpai.usr_type = PPE_VP_USER_TYPE_PASSIVE;
#endif
    vpai.type = PPE_VP_TYPE_SW_L2;
    vpai.net_dev_type = PPE_VP_NET_DEV_TYPE_WIFI;
    vpai.core_mask = cfg_get(g_umac_glb_obj->psoc[0], CFG_DP_MLO_VP_CORE_MASK);

    /*
     * Allocate ppe vp for mld interface
     */
    vpnum = ppe_vp_alloc(dev, &vpai);
    if (vpnum <= 0) {
        /* In some cases we might go out of available VP's,
         * continue without creating a VP in those cases
         */
        osifp_mld->ppe_vp_num = 0;
        qdf_info("\ndev:%p, dev->name:%s, VP allocation failed", dev, dev->name);
    } else {
        osifp_mld->ppe_vp_num = vpnum;
        osifp_mld->ppe_vp_type = vpai.usr_type;
        qdf_info("vp user type: %s vpai.core_mask = %u vp_num = %d allocated VP for dev = %s",
                 osif_vp_type_str[vpai.usr_type], vpai.core_mask, vpnum, dev->name);
    }
#endif

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
set_file_attributes:
    if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj,
                                        (struct qdf_vfs_attr *)&mld_attr_group)) {
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                           "%s: sysfs create group failed\n", __FUNCTION__);
    }
#endif

    return dev;
fail:
    if (unit != -1)
        qdf_net_delete_wlanunit(unit);
    if (dev)
        free_netdev(dev);

    return NULL;
}

int
osif_mldev_destroy_mld(struct net_device *mldev)
{
    struct osif_mldev *osifp_mld;

    osifp_mld = ath_netdev_priv(mldev);

    osif_mldev_remove_mld_osdev(osifp_mld);

    /* make sure that nobody else in the group active */
    if (osif_mldev_get_num_osdev(osifp_mld))
        return 0;

#if QCA_PPE_VP
    /* free the mld vp */
    if (osifp_mld->ppe_vp_num > 0)
        ppe_vp_free(osifp_mld->ppe_vp_num);
#endif
    osif_mlo_peer_find_hash_detach(osifp_mld);
    wlan_cfg80211_unregister_netdevice(mldev);
    return 0;
}

void osif_mldev_link_add(struct net_device *mldev, osif_dev *osifp, uint8_t link_id)
{
    struct osif_mldev *osifp_mldev = osifp->mldev;

    if (!osifp_mldev)
        return;

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    osifp_mldev->link_vifs[link_id] = osifp;
#endif

    osif_mld_map_link_dev(osifp);
}

void osif_mldev_link_delete(osif_dev *osifp)
{
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct osif_mldev *osifp_mldev = osifp->mldev;
    uint8_t link_id;

    if (!osifp_mldev)
        return;

    link_id = ((wlan_if_t)osifp->os_if)->link_id;
    if (link_id >= IEEE80211_MLD_MAX_NUM_LINKS) {
        qdf_err("invalid link_id return");
        return;
    }

    osifp_mldev->link_vifs[link_id] = NULL;
#endif

    osif_mld_unmap_link_dev(osifp);
}

static inline
struct net_device *osif_mld_get_mld_peer_rx_netdev(
                                              struct net_device *mldev,
                                              const uint8_t *mac_addr)
{
    struct osif_mldev *mldev_ctx = osif_mld_get_mldev(mldev);
    osif_dev  *osifp = NULL;

    if (!mldev_ctx)
        return NULL;

    osifp = osifp_peer_find_hash_find_osdev(mldev_ctx, (uint8_t *)mac_addr);
    if (!osifp)
        return NULL;

    return mldev;
}

struct osif_mldev *osif_mldev_get_mldev(struct net_device *dev)
{
    return ath_netdev_priv(dev);
}

static inline
struct net_device *osif_mldev_get_rx_ndev(struct net_device *mldev,
                                          struct net_device *dev)
{
    return mldev;
}

static inline
void osif_mldev_attach_vlan(struct net_device *mld_dev)
{
    mld_dev->features |= NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX |
                         NETIF_F_HW_VLAN_CTAG_FILTER |
                         NETIF_F_HW_VLAN_STAG_TX | NETIF_F_HW_VLAN_STAG_RX |
                         NETIF_F_HW_VLAN_STAG_FILTER;
}

void osif_mldev_reconfig_link_add(struct net_device *dev, uint8_t *mld_mac, uint8_t link_id)
{
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_vdev *vdev;
    struct osif_mldev *mld_osdev;
    struct net_device *mld_dev, *rx_dev;
    osif_dev *osdev;
    uint8_t grp_id;
    wlan_if_t vap;

#if UMAC_SUPPORT_CFG80211
    osdev = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, link_id);
#else
    osdev = get_vap_device_context(NULL, dev, link_id);
#endif

    if (!osdev) {
        qdf_err("null osifp");
        return;
    }

    vap = osdev->os_if;
    if (!vap) {
        qdf_err("NULL vap");
        return;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        qdf_err("Invalid vdev");
        return;
    }

    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        qdf_err("Invalid psoc");
        return;
    }

    grp_id = wlan_mlo_get_psoc_group_id(psoc);

    mld_dev = osif_find_mld_dev(mld_mac, grp_id);
    if (!mld_dev) {
        qdf_err("Invaild mld dev for mac %s", ether_sprintf(mld_mac));
        return;
    }

    mld_osdev = osif_mld_get_mldev(mld_dev);
    if (!mld_osdev) {
        qdf_err("Invalid mld dev");
        return;
    }

    osdev->mldev = mld_osdev;

    if (osdev->rx_dev) {
        rx_dev = osdev->rx_dev;
        osdev->rx_dev = NULL;
        qdf_net_if_release_dev((struct qdf_net_if *)rx_dev);
    }
    rx_dev = osif_mld_get_rx_ndev(mld_dev, dev);
    qdf_net_if_hold_dev((struct qdf_net_if *)rx_dev);
    osdev->rx_dev = rx_dev;

    osif_mld_link_add(mld_dev, osdev, vap->link_id);
}

void osif_mldev_reconfig_link_delete(struct net_device *dev, uint8_t link_id)
{
    osif_dev *osdev;

#if UMAC_SUPPORT_CFG80211
    osdev = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, link_id);
#else
    osdev = get_vap_device_context(NULL, dev, link_id);
#endif
    if (!osdev) {
        qdf_err("NULL osdev");
        return;
    }

    osif_mld_link_delete(osdev);

    osdev->mldev = NULL;

    if (osdev->rx_dev) {
        /* reset the dev pointer to self */
        qdf_net_if_hold_dev((struct qdf_net_if *)dev);

        qdf_net_if_release_dev((struct qdf_net_if *)osdev->rx_dev);

        /* reset rx_dev to self dev */
        osdev->rx_dev = dev;
    }
}

static struct osif_mldev_ops {
    struct net_device *(*osif_mldev_op_create_mld)(struct wiphy* mld_wiphy,
                                                    void *mld_params,
                                                    const char *iname,
                                                    enum nl80211_iftype type);
    int (*osif_mldev_op_destroy_mld)(struct net_device *dev);
    void (*osif_mldev_op_link_add)(struct net_device *mld_dev,
                                   osif_dev *osifp,
                                   uint8_t link_id);
    void (*osif_mldev_op_link_delete)(osif_dev *osifp);
    struct osif_mldev *(*osif_mldev_op_get_mldev)(struct net_device *mld_dev);
    struct net_device *(*osif_mldev_op_get_rx_ndev)(struct net_device *mldev,
                                                    struct net_device *dev);
    struct net_device *(*osif_mldev_op_get_mld_peer_rx_netdev)(
                                                   struct net_device *mldev,
                                                   const uint8_t *mac_addr);
    void (*osif_mldev_op_vlan_attach)(struct net_device *mld_dev);
    void (*osif_mldev_op_reconfig_link_add)(struct net_device *dev,
                                            uint8_t *mld_mac, uint8_t link_id);
    void (*osif_mldev_op_reconfig_link_delete)(struct net_device *dev, uint8_t link_id);
} osif_mldev_ops[OSIF_MLDEV_OP_MAX] = {
    /*Initialize both AP and STA ops to bonding mode*/
    {
        .osif_mldev_op_create_mld = osif_create_mld_bond,
        .osif_mldev_op_destroy_mld = osif_destroy_mld_bond,
        .osif_mldev_op_link_add = osif_mldev_link_add_bond,
        .osif_mldev_op_link_delete = osif_mldev_link_delete_bond,
        .osif_mldev_op_get_mldev = osif_mldev_get_mldev_bond,
        .osif_mldev_op_get_rx_ndev = osif_mldev_get_rx_ndev_bond,
        .osif_mldev_op_get_mld_peer_rx_netdev =
                            osif_mld_get_mld_peer_rx_netdev_bond,
        .osif_mldev_op_vlan_attach = NULL,
        .osif_mldev_op_reconfig_link_add = NULL,
        .osif_mldev_op_reconfig_link_delete = NULL,
    },
    {
        .osif_mldev_op_create_mld = osif_create_mld_bond,
        .osif_mldev_op_destroy_mld = osif_destroy_mld_bond,
        .osif_mldev_op_link_add = osif_mldev_link_add_bond,
        .osif_mldev_op_link_delete = osif_mldev_link_delete_bond,
        .osif_mldev_op_get_mldev = osif_mldev_get_mldev_bond,
        .osif_mldev_op_get_rx_ndev = osif_mldev_get_rx_ndev_bond,
        .osif_mldev_op_get_mld_peer_rx_netdev =
                               osif_mld_get_mld_peer_rx_netdev_bond,
        .osif_mldev_op_vlan_attach = NULL,
        .osif_mldev_op_reconfig_link_add = NULL,
        .osif_mldev_op_reconfig_link_delete = NULL,
    }
};

struct osif_mldev_ops *osif_mldev_find_ops_by_type(uint8_t type)
{
    switch (type) {
    case NL80211_IFTYPE_STATION:
        return &osif_mldev_ops[OSIF_MLDEV_OP_STA];
    case NL80211_IFTYPE_AP:
        return &osif_mldev_ops[OSIF_MLDEV_OP_AP];
    default:
        qdf_err("Unsupported type %u", type);
    }

    return NULL;
}

static uint8_t osif_mldev_get_iftype(struct net_device *mldev)
{
    struct wireless_dev *wdev;

    if (!(wdev = mldev->ieee80211_ptr)) {
        qdf_err("Invalid wdev for mldev %s", mldev->name);
        return NL80211_IFTYPE_MAX;
    }
    return wdev->iftype;
}

static inline
struct osif_mldev_ops *osif_mldev_find_ops_by_mld_ndev(struct net_device *mldev)
{
    return osif_mldev_find_ops_by_type(osif_mldev_get_iftype(mldev));
}

static inline
struct osif_mldev_ops *osif_mldev_find_ops_by_link_ndev(struct net_device *dev)
{
    osif_dev *osifp = ath_netdev_priv(dev);

    if (!osifp->mldev)
        return NULL;

    return osif_mldev_find_ops_by_type(osif_mldev_get_iftype(osifp->mldev->mld_dev));
}

inline
struct net_device *osif_mld_create(struct wiphy* mld_wiphy,
                                   void *mld_params,
                                   const char *ifname,
                                   enum nl80211_iftype type)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_type(type);

    if (ops && ops->osif_mldev_op_create_mld)
        return ops->osif_mldev_op_create_mld(mld_wiphy, mld_params, ifname, type);
    return NULL;
}

inline
int osif_mld_destroy(struct net_device *dev)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_mld_ndev(dev);

    if (ops && ops->osif_mldev_op_destroy_mld)
        return ops->osif_mldev_op_destroy_mld(dev);
    return 0;
}

inline
void osif_mld_link_add(struct net_device *mldev, osif_dev *osifp, uint8_t link_id)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_mld_ndev(mldev);

    if (ops && ops->osif_mldev_op_link_add)
        return ops->osif_mldev_op_link_add(mldev, osifp, link_id);
}

inline
void osif_mld_link_delete(osif_dev *osifp)
{
    struct osif_mldev_ops *ops = NULL;
    struct net_device *mldev = NULL;

    if (!osifp->mldev || !osifp->mldev->mld_dev)
        return;

    mldev = osifp->mldev->mld_dev;
    ops = osif_mldev_find_ops_by_mld_ndev(mldev);

    if (ops && ops->osif_mldev_op_link_delete)
        return ops->osif_mldev_op_link_delete(osifp);
}

inline
struct osif_mldev *osif_mld_get_mldev(struct net_device *mldev)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_mld_ndev(mldev);

    if (ops && ops->osif_mldev_op_get_mldev)
        return ops->osif_mldev_op_get_mldev(mldev);
    return NULL;
}

qdf_export_symbol(osif_mld_get_mldev);

inline
struct net_device *osif_mld_get_rx_ndev(struct net_device *mldev,
                                        struct net_device *dev)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_mld_ndev(mldev);

    if (ops && ops->osif_mldev_op_get_rx_ndev)
        return ops->osif_mldev_op_get_rx_ndev(mldev, dev);
    return NULL;
}

struct net_device *osif_get_mld_peer_rx_netdev(struct net_device *mldev,
                                               const uint8_t *mac_addr)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_mld_ndev(mldev);

    if (ops && ops->osif_mldev_op_get_mld_peer_rx_netdev)
        return ops->osif_mldev_op_get_mld_peer_rx_netdev(mldev, mac_addr);
    return NULL;
}

qdf_export_symbol(osif_get_mld_peer_rx_netdev);

void osif_mldev_vlan_attach(struct net_device *mldev)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_mld_ndev(mldev);

    if (ops && ops->osif_mldev_op_vlan_attach)
        return ops->osif_mldev_op_vlan_attach(mldev);
}

void osif_mld_reconfig_link_add_start_ap(struct net_device *dev,
                                         uint8_t *mld_mac, uint8_t link_id)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_type(NL80211_IFTYPE_AP);

    if (ops && ops->osif_mldev_op_reconfig_link_add)
        return ops->osif_mldev_op_reconfig_link_add(dev, mld_mac, link_id);
}

void osif_mld_reconfig_link_delete_stop_ap(struct net_device *dev, uint8_t link_id)
{
    struct osif_mldev_ops *ops = osif_mldev_find_ops_by_type(NL80211_IFTYPE_AP);

    if (ops && ops->osif_mldev_op_reconfig_link_delete)
            return ops->osif_mldev_op_reconfig_link_delete(dev, link_id);
}

static void osif_mldev_set_ops(struct osif_mldev_ops *ops, bool bond_enable)
{
    if (!ops)
        return;

    if (bond_enable) {
        /* Register bonding ops */
        ops->osif_mldev_op_create_mld = osif_create_mld_bond;
        ops->osif_mldev_op_destroy_mld = osif_destroy_mld_bond;
        ops->osif_mldev_op_link_add = osif_mldev_link_add_bond;
        ops->osif_mldev_op_link_delete = osif_mldev_link_delete_bond;
        ops->osif_mldev_op_get_mldev = osif_mldev_get_mldev_bond;
        ops->osif_mldev_op_get_rx_ndev = osif_mldev_get_rx_ndev_bond;
        ops->osif_mldev_op_get_mld_peer_rx_netdev =
                                   osif_mld_get_mld_peer_rx_netdev_bond,
        ops->osif_mldev_op_vlan_attach = NULL;
        ops->osif_mldev_op_reconfig_link_add = NULL;
        ops->osif_mldev_op_reconfig_link_delete = osif_mldev_reconfig_link_delete;
    } else {
        /* Register non-bonding ops */
        ops->osif_mldev_op_create_mld = osif_mldev_create_mld;
        ops->osif_mldev_op_destroy_mld = osif_mldev_destroy_mld;
        ops->osif_mldev_op_link_add = osif_mldev_link_add;
        ops->osif_mldev_op_link_delete = osif_mldev_link_delete;
        ops->osif_mldev_op_get_mldev = osif_mldev_get_mldev;
        ops->osif_mldev_op_get_rx_ndev = osif_mldev_get_rx_ndev;
        ops->osif_mldev_op_get_mld_peer_rx_netdev = osif_mld_get_mld_peer_rx_netdev,
        ops->osif_mldev_op_vlan_attach = osif_mldev_attach_vlan;
        ops->osif_mldev_op_reconfig_link_add = osif_mldev_reconfig_link_add;
        ops->osif_mldev_op_reconfig_link_delete = osif_mldev_reconfig_link_delete;
    }

}
void osif_mldev_register_ops(bool is_ap, uint32_t mode)
{
    struct osif_mldev_ops *ops;
    bool bond_enable = false;

    if (is_ap) {
        if (MLD_AP_UNIFIED_BOND_MODE == mode)
            bond_enable = true;
        else
            bond_enable = false;

        qdf_info("MLD netdev AP mode %x", mode);
        ops = osif_mldev_find_ops_by_type(NL80211_IFTYPE_AP);
        osif_mldev_set_ops(ops, bond_enable);
    } else {
        if (MLD_STA_UNIFIED_BOND_MODE == mode)
            bond_enable = true;
        else
            bond_enable = false;

        qdf_info("MLD netdev STA mode %x", mode);
        ops = osif_mldev_find_ops_by_type(NL80211_IFTYPE_STATION);
        osif_mldev_set_ops(ops, bond_enable);
    }
}
qdf_export_symbol(osif_mldev_register_ops);

void osif_mldev_unregister_ops(void)
{
    struct osif_mldev_ops *ops;

    ops = osif_mldev_find_ops_by_type(NL80211_IFTYPE_AP);
    if (!ops)
        return;

    qdf_mem_zero(ops, sizeof(struct osif_mldev_ops));
}
qdf_export_symbol(osif_mldev_unregister_ops);
#endif

/*
 * Call dev_close() for nonTx VAPs when Tx VAP is down
 */
void osif_vap_netdev_close_mbssid(osif_dev *osifp)
{
    struct ieee80211com *ic;
    struct ieee80211vap *vap, *tmpvap;
    uint8_t grp_id;

    vap = osifp->os_if;
    if (!vap) {
        mbss_err("VAP is NULL");
        return;
    }

    ic = vap->iv_ic;
    if (!ic->is_mbssid_enabled || !ic->ic_mbss_ctx) {
        mbss_debug("MBSSID is not enabled");
        return;
    }

    if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap))
        return;

#ifdef WLAN_FEATURE_11BE_MLO
    /**
     * If Tx VAP is going through ML AP removal, avoid explicit dev_close on
     * Non-Tx VAPs. This is because dev_close would trigger an internal
     * wlan_cfg80211_stop_ap() to be sent by the kernel without the reconfig
     * flag set. So, we would end up clearing the previously set reconfig flag,
     * which would bringdown the entire MLD of Non-Tx VAP.
     */
    if (vap->ml_ap_removal_in_progress) {
        mlo_err("%s: ML AP removal is in progress on Tx vap", vap->iv_netdev_name);
        if (ieee80211_ucfg_reset_txvap(vap, 1))
            mbss_debug("Reset Tx VAP (%d) failed", vap->iv_unit);

        return;
    }
#endif

    grp_id = vap->iv_mbss.group_id;

    /* Update netdev flags for NonTx VAP only if MBSS auto mode is clear.
     * If auto mode is set, MBSS algorithm will choose the next Tx VAP
     * and bring it up
     */
    if (ieee80211_ic_mbss_automode_is_clear(ic)) {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            struct net_device *tmpdev = get_cfg80211_notification_ndev((osif_dev *)tmpvap->iv_ifp);
            bool rtnl_lock_acquired = false;

            if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP) &&
                    (tmpvap->iv_mbss.group_id == grp_id) &&
                    IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(tmpvap) &&
                    (tmpdev->flags & IFF_UP)) {
                if (!rtnl_is_locked()) {
                    rtnl_lock();
                    rtnl_lock_acquired = true;
                }
                /* do not close the netdev if the tmpvap is link vap of
                 * another mld for now.
                 * to be revisited as part of impacted feature effort
                 */
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
                if (!(((osif_dev *)tmpvap->iv_ifp)->mldev))
#endif
                dev_close(tmpdev);
                if (rtnl_lock_acquired) {
                    rtnl_unlock();
                }
            }
        }
    }

    if (ieee80211_ucfg_reset_txvap(vap, 1))
        mbss_debug("Reset Tx VAP (%d) failed", vap->iv_unit);

    return;
}
qdf_export_symbol(osif_vap_netdev_close_mbssid);

int osif_vap_stop_ap(wlan_if_t vap)
{
    enum ieee80211_opmode opmode;
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
#if WLAN_SPECTRAL_ENABLE
    bool is_spectral_active = false;
    enum spectral_scan_mode smode;
    struct wlan_lmac_if_tx_ops *tx_ops;
#endif /* WLAN_SPECTRAL_ENABLE */

    if (!ic)
        return -EINVAL;

    pdev = ic->ic_pdev_obj;
    if (!pdev)
        return -EINVAL;

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc)
        return -EINVAL;

    if (IEEE80211_IS_SPECIAL_VAP(vap) &&
        cfg_get(psoc, CFG_OL_GET_SPL_VAP_WO_WIFI)) {
        qdf_debug("Skip special VAP stop during ifconfig down");
        return EOK;
    }

    opmode = wlan_vap_get_opmode(vap);
    if (ieee80211_ucfg_bringdown_txvap(vap)) {
        /* If ucfg_bringdown fails, invoke bringdown sequence */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : stopping %s vap \n",
                __func__, opmode == IEEE80211_M_HOSTAP ? "AP" : "MONITOR");

#ifdef WLAN_FEATURE_11BE_MLO
        /* MLO reconfig: While link removal is in progress for this vap,
         * if CSA is in progress for partner vap then this vap's stop command
         * will be deferred till CSA completes. Due to this, AP will beacon
         * even though ML reconfig timer expired till partner vdev CSA
         * completes.
         * To fix this, allow the vdev to send stop command if ML reconfig timer
         * expired.
         */
        if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) &&
            !wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_STOP_LINK_DEL)) {
            osif_vap_check_blocking_cmd_on_mlo(vap->vdev_obj);
        }
#endif

        wlan_mlme_stop_vdev(vap->vdev_obj, WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE |
                WLAN_MLME_STOP_BSS_F_NO_RESET, WLAN_MLME_NOTIFY_OSIF);
#ifdef WLAN_FEATURE_11BE_MLO
        if (!wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_STOP_LINK_DEL))
            osif_vap_bringdown_mlo(vap->vdev_obj);
#endif
    }

#if WLAN_SPECTRAL_ENABLE
    /* If all the vaps are going down, stop spectral scan */

    tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
    if (!tx_ops) {
        qdf_err("tx_ops is NULL");
        return -EINVAL;
    }

    smode = SPECTRAL_SCAN_MODE_NORMAL;
    for (; smode < SPECTRAL_SCAN_MODE_MAX; smode++)
        is_spectral_active |= tx_ops->sptrl_tx_ops.
            sptrlto_is_spectral_active(pdev, smode);

    if (is_spectral_active) {
        size_t active_vdev_count = 0;

        active_vdev_count = osif_get_num_active_vaps(ic);
        if (active_vdev_count == 1) {
            enum spectral_cp_error_code err;

            smode = SPECTRAL_SCAN_MODE_NORMAL;
            for (; smode < SPECTRAL_SCAN_MODE_MAX; smode++)
                tx_ops->sptrl_tx_ops.
                    sptrlto_stop_spectral_scan(pdev, smode, &err);
        }
    }
#endif /* WLAN_SPECTRAL_ENABLE */
    return 0;
}

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
static void osif_vap_delete_pasn_peer(struct wlan_objmgr_vdev *vdev,
                                      void *object, void *arg)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
    struct ieee80211_node *ni = NULL;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        qdf_err("ni is null");
        return;
    }

    if (wlan_peer_get_peer_type(peer) == WLAN_PEER_RTT_PASN)
        cleanup_sta_peer(arg, ni);
}

static void osif_delete_all_pasn_peers(struct ieee80211vap *vap)
{
    uint8_t peer_del_req_send = MLME_INDIVIDUAL_PEER_CLEANUP;

    /* Send peer delete all only when fw capability is set */
    if (wlan_pdev_nif_feat_cap_get(
                wlan_vdev_get_pdev(vap->vdev_obj),
                WLAN_PDEV_F_DELETE_ALL_PEER))
        peer_del_req_send = MLME_MULTIPLE_PEER_CLEANUP;

    /* Delete the PASN peers */
    wlan_objmgr_iterate_peerobj_list(vap->vdev_obj,
            osif_vap_delete_pasn_peer, &peer_del_req_send,
            WLAN_MLME_SB_ID);
}
#else
static void osif_delete_all_pasn_peers(struct ieee80211vap *vap)
{
    return;
}
#endif /* defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT) */

int osif_vap_stop_sta(wlan_if_t vap)
{
    osif_delete_all_pasn_peers(vap);

    return wlan_mlme_cm_stop(vap->vdev_obj, CM_OSIF_DISCONNECT,
                             REASON_STA_LEAVING, true);
}

int osif_vap_pre_stop_ap(osif_dev *osifp)
{
    /* Do nothing */
    return 0;
}

int osif_vap_pre_stop_sta(osif_dev *osifp)
{
    wlan_if_t vap = osifp->os_if;

    qdf_info("Stopping STA vap");
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    if (osifp->os_opmode == IEEE80211_M_STA)
        wlan_mlme_stacac_restore_defaults(vap);
#endif

    vap->iv_roam_inprogress = FALSE;

    return 0;
}

int osif_vap_pre_stop(osif_dev *osifp)
{
    wlan_if_t vap = osifp->os_if;
    struct ieee80211com *ic;
    enum ieee80211_opmode opmode;
    int ret = 0;
    struct net_device *dev;

    if (!vap)
       return -EINVAL;

    dev = get_cfg80211_notification_ndev(osifp);

    ic = vap->iv_ic;

#ifdef QCA_PARTNER_PLATFORM
    osif_pltfrm_vap_stop(osifp);
#endif

    wlan_ucfg_scan_cancel(vap, osifp->scan_requestor, INVAL_SCAN_ID,
            WLAN_SCAN_CANCEL_VDEV_ALL, false);

#if UMAC_SUPPORT_CFG80211
    if (ic->ic_cfg80211_config)
        wlan_cfg80211_cleanup_scan_queue(ic->ic_pdev_obj, dev);
#endif

    if (dev->flags & IFF_RUNNING) {
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        if (!osifp->mldev)
#endif
        dev->flags &= ~IFF_RUNNING;
    }

    opmode = wlan_vap_get_opmode(vap);
    switch(opmode) {
    case IEEE80211_M_STA:
        ret = osif_vap_pre_stop_sta(osifp);
        break;
    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_MONITOR:
        ret = osif_vap_pre_stop_ap(osifp);
    default:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : mode not suported \n",
                __func__);
        break;
    }

    return ret;
}

int osif_vap_stop_wrapper(osif_dev *osifp)
{
    wlan_if_t vap = osifp->os_if;
    enum ieee80211_opmode opmode;
    int ret = 0;

    ret = osif_vap_pre_stop(osifp);
    if (ret)
        return ret;

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    osifp->stop_initiated = true;
#endif

    opmode = wlan_vap_get_opmode(vap);
    switch(opmode) {
        case IEEE80211_M_STA:
            ret = osif_vap_stop_sta(vap);
            break;
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_MONITOR:
            ret = osif_vap_stop_ap(vap);
            break;
        default:
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                              "%s : mode not suported \n",
                              __func__);
            break;
    }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    osifp->stop_initiated = false;
#endif

    return ret;
}

int osif_vap_stop(struct net_device *dev)
{
    osif_dev *osifp;

#if UMAC_SUPPORT_CFG80211
    osifp = get_vap_device_context(dev->ieee80211_ptr->wiphy,
                                   dev,
                                   INVALID_LINK_ID);
#else
    osifp = get_vap_device_context(NULL, dev, INAVLID_LINK_ID);
#endif

    return osif_vap_stop_wrapper(osifp);
}

int osif_pdev_restart(struct ieee80211vap *vap)
{
    return wlan_mlme_dev_restart(vap->vdev_obj, WLAN_SER_CMD_PDEV_RESTART);
}

int osif_vdev_restart(struct ieee80211vap *vap)
{
    return wlan_mlme_dev_restart(vap->vdev_obj, WLAN_SER_CMD_VDEV_RESTART);
}

int osif_pdev_stop_start_vaps(struct ieee80211com *ic)
{
    wlan_if_t tmpvap;
    struct net_device *tmpdev;

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
        tmpdev = ((osif_dev *)tmpvap->iv_ifp)->netdev;
        if (IS_UP(tmpdev) &&
            wlan_vdev_chan_config_valid(tmpvap->vdev_obj) == QDF_STATUS_SUCCESS)
            osif_vap_init((osif_dev *)tmpvap->iv_ifp, tmpdev, RESCAN);
    }
    return 0;
}

/*
 * osif_pdev_restart_vaps() - Multi-vdev restart
 * @ic: radio parameter
 *
 * Restart all vdevs belonging to this pdev.
 * Only one pdev restart command is added to the serialization queue of the
 * first vdev. On cmd activation, restart is triggered for all the vdev's of
 * this pdev
 *
 * Return: Success on adding cmd to vdev queue
 * */
int osif_pdev_restart_vaps(struct ieee80211com *ic)
{
    int retv = 0;
    wlan_if_t tmpvap;
    uint8_t num_sta, num_vdev;
    int is_ap_cac_timer_running = 0;

    if (!ic) {
        qdf_err("ic is null");
        return -EINVAL;
    }

    /*
     * If there are STA vaps belonging to this ic then use individual vdev
     * stop-start. Else use optimized pdev restart.
     */
    num_sta = wlan_mbss_num_sta(ic->ic_pdev_obj);
    num_vdev = wlan_mbss_num_vdev(ic->ic_pdev_obj);

    /*
     * If cac timer is running, instead of using blocking pdev restart,
     * use individual vdev stop-start.
     */
    ucfg_dfs_is_ap_cac_timer_running(ic->ic_pdev_obj, &is_ap_cac_timer_running);
    if ((num_sta) || !num_vdev || is_ap_cac_timer_running ||
        !ieee80211_vaps_up(ic, IEEE80211_M_HOSTAP)) {
        if (ic->ic_config_update)
            ic->ic_config_update(ic);

        retv = osif_pdev_stop_start_vaps(ic);

        return retv;
    }

    tmpvap = TAILQ_FIRST(&ic->ic_vaps);
    if (tmpvap)
        retv = osif_pdev_restart(tmpvap);
    else
        qdf_err("Invalid VAP");

    return retv;
}

static int osif_setup_vap( osif_dev *osifp, enum ieee80211_opmode opmode,
                            u_int32_t flags, char *bssid)
{
    wlan_dev_t devhandle = osifp->os_devhandle;
    int error;
    wlan_if_t vap;
    ieee80211_privacy_exemption privacy_filter;
    int scan_priority_mapping_base;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct vdev_osif_priv *vdev_osifp = NULL;
    struct wlan_vdev_create_params params;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint16_t waitcnt;
    QDF_STATUS status;

    psoc = wlan_pdev_get_psoc(devhandle->ic_pdev_obj);
    switch (opmode) {
#ifdef  ATH_SUPPORT_P2P
    case IEEE80211_M_P2P_GO:
        qdf_print(" %s: Creating NEW P2P GO interface", __func__);
        osifp->p2p_go_handle = wlan_p2p_GO_create((wlan_p2p_t) devhandle, NULL);
        if (osifp->p2p_go_handle == NULL) {
            return EIO;
        }
        vap=wlan_p2p_GO_get_vap_handle(osifp->p2p_go_handle);
        break;

    case IEEE80211_M_P2P_CLIENT:
        qdf_print(" %s: Creating NEW P2P Client interface", __func__);
        osifp->p2p_client_handle = wlan_p2p_client_create((wlan_p2p_t) devhandle, NULL);
        if (osifp->p2p_client_handle == NULL) {
            return EIO;
        }
        vap=wlan_p2p_client_get_vap_handle(osifp->p2p_client_handle);
        break;

    case IEEE80211_M_P2P_DEVICE:
        qdf_print(" %s: Creating NEW P2P Device interface", __func__);
        osifp->p2p_handle = wlan_p2p_create(devhandle, NULL);
        if (osifp->p2p_handle == NULL) {
            return EIO;
        }
        vap=wlan_p2p_get_vap_handle(osifp->p2p_handle);
        break;
#endif
    default:
        qdf_print(" %s: Creating NEW DEFAULT interface", __func__);
        if (opmode == IEEE80211_M_STA) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE;
        }
        else if (opmode == IEEE80211_M_IBSS) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_IBSS_BASE;
        }
        else if (opmode == IEEE80211_M_HOSTAP) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_AP_BASE;
        }
        else if (opmode == IEEE80211_M_MONITOR) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE;
        }
        else {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE;
        }

        OS_MEMZERO(&params,sizeof(struct wlan_vdev_create_params));
        params.opmode = ieee80211_opmode2qdf_opmode(opmode);
        if(params.opmode >= QDF_MAX_NO_OF_MODE) {
           qdf_err("opmode(%d) is not supported", opmode);
           return EINVAL;
        }
        IEEE80211_ADDR_COPY(params.macaddr,bssid);
        params.flags = flags;
        params.size_vdev_priv = sizeof(struct vdev_osif_priv);
        params.legacy_osif = osifp;

        vdev = wlan_objmgr_vdev_obj_create(devhandle->ic_pdev_obj, &params);

        if(vdev != NULL) {
            vdev_osifp = wlan_vdev_get_ospriv(vdev);
            vdev_osifp->wdev = NULL;
            /* In sta mode when old bss node is freed, it is posted as an event to
             * connection state machine, which initiates creation of new bss peer.
             * At this point, unless the peer's destroy handler has been executed and
             * max peers per vap are decremented already, the creation of new bss peer
             * will fail. To handle this syncronization the max peers per vap are
             * increased from 2 to 3
             */
            if (opmode == IEEE80211_M_STA) {
                wlan_vdev_set_max_peer_count(vdev, 3);
            }
            wlan_pdev_set_max_beacon_count(wlan_vdev_get_pdev(vdev),100);
            waitcnt = 0;
            while((!((vdev->obj_state == WLAN_OBJ_STATE_CREATED) ||
                   (vdev->obj_state == WLAN_OBJ_STATE_CREATION_FAILED)) &&
                     (waitcnt < WLAN_VDEV_CREATE_TIMEOUT_CNT))) {
                qdf_sleep_us(WLAN_VDEV_CREATE_TIMEOUT*1000);
                waitcnt++;
            }
            if(vdev->obj_state == WLAN_OBJ_STATE_CREATION_FAILED) {
                qdf_print("VDEV (partial creation) failed ");
                vap = wlan_vdev_get_mlme_ext_obj(vdev);
                if (vap != NULL) {
                   wlan_vap_delete(vap);
                }
                else {
                   wlan_objmgr_vdev_obj_delete(vdev);
                }
                vdev = NULL;
            }
        }
        else {
            qdf_print("VDEV (creation) failed ");
        }

        break;
    }
    if (!vdev) {
        return ENOMEM;
    }
    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if (vap) {
        /*
         * Update vdev chainmasks.
         *
         * Rather than obtain a lock for the vdev again at another place in this
         * function, we set the vdev's chainmasks right here while we hold the
         * lock.
         * But we need to do so only if vap is non-NULL.
         */
        wlan_vdev_mlme_set_txchainmask(vdev, devhandle->ic_tx_chainmask);
        wlan_vdev_mlme_set_rxchainmask(vdev, devhandle->ic_rx_chainmask);
    }

    if (!vap) {
        return ENOMEM;
    }

     /*TODO it is supposed to be done as part of creation, but this can be updated seperately */
    vap->iv_scan_priority_base = scan_priority_mapping_base;
    osifp->os_opmode = opmode;
    osifp->os_if = vap;            /* back pointer */
    osifp->ctrl_vdev = vdev;      /* storing vdev object pointer */
    osif_vap_setup(vap, osifp, opmode);

    if (opmode == IEEE80211_M_P2P_DEVICE)
       wlan_set_desired_phymode(vap,IEEE80211_MODE_11G);
    else
       wlan_set_desired_phymode(vap, IEEE80211_MODE_AUTO);

    /* always setup a privacy filter to allow receiving unencrypted EAPOL frame */
    privacy_filter.ether_type = ETHERTYPE_PAE;
    privacy_filter.packet_type = IEEE80211_PRIVACY_FILTER_PACKET_BOTH;
    privacy_filter.filter_type = IEEE80211_PRIVACY_FILTER_KEY_UNAVAILABLE;
    wlan_set_privacy_filters(vap,&privacy_filter,1);
    do {
            ieee80211_cipher_type ctypes[1];
    error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE, 1 << WLAN_CRYPTO_AUTH_OPEN);

            ctypes[0] = IEEE80211_CIPHER_NONE;
            osifp->uciphers[0] = osifp->mciphers[0] = IEEE80211_CIPHER_NONE;
            osifp->u_count = osifp->m_count = 1;
            error = wlan_set_ucast_ciphers(vap,ctypes,1);
            error = wlan_set_mcast_ciphers(vap,ctypes,1);
    } while(0);

    /* register scan event handler */
    /* Allocate requester ID without attaching callback function */
    osifp->scan_requestor = ucfg_scan_register_requester(wlan_vdev_get_psoc(vdev),
            (uint8_t*)"osif_umac", NULL, NULL);
    if (!osifp->scan_requestor) {
        scan_err("unable to allocate requester");
    }
    /* Register scan event callback with scan module */
    status = ucfg_scan_register_event_handler(wlan_vdev_get_pdev(vdev),
            &osif_scan_evhandler, (void*)osifp);
    if (status) {
        scan_err("unable to register scan event handler vap: 0x%pK status: %d", vap, status);
    }

    return 0;
}

static void osif_delete_vap(osif_dev *osifp, int recover)
{
    struct wlan_objmgr_vdev *vdev = osifp->ctrl_vdev;
    struct ieee80211com *ic;
    wlan_if_t vap = NULL;
    struct wlan_mbss_ev_data mbss_ev = {0};

    wlan_objmgr_vdev_get_ref(vdev, WLAN_MLME_NB_ID);
    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);

    if(vap == NULL) {
        qdf_err("vap is NULL, vdev deletion failed ");
        return;
    }
#ifdef WLAN_OBJMGR_REF_ID_TRACE
    if (vap->iv_ref_leak_test_flag) {
        wlan_objmgr_vdev_get_ref(vdev, WLAN_MLME_NB_ID);
    }
#endif
    ic = wlan_vdev_get_ic(vdev);
    if(ic == NULL) {
        qdf_err("ic is NULL, vdev deletion failed ");
        return;
    }
#if QCA_LTEU_SUPPORT
    if (ic->ic_nl_handle)
        ieee80211_nl_unregister_handler(vap, osif_mu_report, osif_nl_scan_evhandler);
#endif
#if DBG_LVL_MAC_FILTERING
    dbgmac_peer_list_free(vap->dbgmac_peer_list);
#endif
    STA_VAP_DOWNUP_LOCK(ic);
    if(vap == ic->ic_sta_vap)
        ic->ic_sta_vap = NULL;
    if(vap == ic->ic_mon_vap)
    {
        ic->ic_mon_vap = NULL;
        wlan_pdev_set_mon_vdev(ic->ic_pdev_obj, NULL);
    }
    STA_VAP_DOWNUP_UNLOCK(ic);

#if DBDC_REPEATER_SUPPORT
    GLOBAL_IC_LOCK_BH(ic->ic_global_list);
    if (ic->ic_global_list->dbdc_process_enable &&
        vap == ic->ic_global_list->max_priority_stavap_up) {
        ic->ic_global_list->max_priority_stavap_up = NULL;
    }
    GLOBAL_IC_UNLOCK_BH(ic->ic_global_list);
#endif

    /* Disable new scans on this vdev */
    ucfg_scan_vdev_set_disable(vdev, REASON_VDEV_DOWN);

    /* unregister scan event handler */
    ucfg_scan_unregister_event_handler(wlan_vap_get_pdev(vap), &osif_scan_evhandler, (void *)osifp);
    ucfg_scan_unregister_requester(wlan_vap_get_psoc(vap), osifp->scan_requestor);

    wlan_ucfg_scan_cancel(vap, osifp->scan_requestor, INVAL_SCAN_ID,
            WLAN_SCAN_CANCEL_VDEV_ALL, true);

    mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_ACS;
    wlan_mbss_if_mgr_send_event(vdev, &mbss_ev);

    mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_HT40;
    wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

    if ((osifp->os_opmode == IEEE80211_M_STA) && wlan_rptr_vdev_is_extap(vdev)) {
        dp_extap_disable(vdev);
        dp_extap_mitbl_purge(dp_get_extap_handle(vdev));
    }

    osifp->is_deleted = 0;
    /*
    * flush the frames belonging to this vap from osdep queues.
    */
    // ath_flush_txqueue(osifp->os_devhandle, vap);
    switch( osifp->os_opmode) {
#ifdef  ATH_SUPPORT_P2P
    case IEEE80211_M_P2P_GO:
        qdf_print(" Deleting  P2PGO vap ");
        wlan_p2p_GO_delete(osifp->p2p_go_handle);
        osifp->p2p_go_handle = NULL;
        break;

    case IEEE80211_M_P2P_CLIENT:
        qdf_print(" Deleting  P2P Client  ");
        wlan_mlme_cancel(vap);
        wlan_p2p_client_delete(osifp->p2p_client_handle);
        osifp->p2p_client_handle = NULL;
        break;

    case IEEE80211_M_P2P_DEVICE:
        qdf_print(" Deleting  P2P device  ");
        wlan_p2p_delete(osifp->p2p_handle);
        osifp->p2p_handle = NULL;
        break;
#endif
    case IEEE80211_M_MONITOR:
        if (vap->mac_entries)
            wlan_mon_flush_maclist(vap);
    default:
        if (recover) {
            ieee80211_flush_vap_mgmt_queue(vap, true);
            wlan_vap_recover(vap);
        } else {
            wlan_vap_delete(vap);
        }

        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);

        break;
    }
}

#if WLAN_SPECTRAL_ENABLE
int
osif_ioctl_eacs(struct net_device *dev, struct ifreq *ifr, osdev_t os_handle)
{

    osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);
    struct scan_start_request *scan_params = NULL;
    ieee80211_ssid    ssid_list[IEEE80211_SCAN_MAX_SSID];
    int               n_ssid;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    struct wlan_objmgr_vdev *vdev = NULL;

    if (!(dev->flags & IFF_UP)) {
        return -EINVAL;     /* XXX */
    }

    vdev = osifp->ctrl_vdev;
    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_OSIF_SCAN_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        scan_info("unable to get reference");
        return -EINVAL;
    }

    scan_params = (struct scan_start_request*) qdf_mem_malloc_atomic(
						sizeof(*scan_params));
    if (!scan_params) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
        scan_err("unable to allocate scan request");
        return -ENOMEM;
    }

    if (wlan_vdev_scan_in_progress(vap->vdev_obj)) {
        wlan_ucfg_scan_cancel(vap, osifp->scan_requestor, 0,
                WLAN_SCAN_CANCEL_VDEV_ALL, false);
        OS_DELAY(1000);
    }

    /* Fill scan parameter */
    n_ssid = wlan_get_desired_ssidlist(vap, ssid_list, IEEE80211_SCAN_MAX_SSID);

    status = wlan_update_scan_params(vap,scan_params,IEEE80211_M_HOSTAP,
            false,true,false,true,0,NULL,0);
    if (status) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
        qdf_mem_free(scan_params);
        scan_err("scan param init failed with status: %d", status);
        return -EINVAL;
    }

    switch (opmode)
    {
        case IEEE80211_M_HOSTAP:
            scan_params->scan_req.scan_flags = 0;
            scan_params->scan_req.scan_f_passive = true;
            scan_params->scan_req.scan_f_2ghz = true;
            scan_params->scan_req.scan_f_5ghz = true;

            /* XXX tunables */
            scan_params->scan_req.dwell_time_passive = 300;
            break;

        default:break;
    }

    status = wlan_ucfg_scan_start(vap, scan_params, osifp->scan_requestor,
            SCAN_PRIORITY_LOW, &(osifp->scan_id), 0, NULL);
    if (status) {
        scan_err("scan_start failed with status: %d", status);
    }
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_SCAN_ID);
    return 0;
}
qdf_export_symbol(osif_ioctl_eacs);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
extern int ieee80211_ioctl_wandev(struct net_device *dev, struct if_settings *ifs);
extern int ieee80211_ioctl_devprivate(struct net_device *dev, struct ifreq *ifr, void __user *data, int cmd);
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
extern int ieee80211_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
extern int ieee80211_ioctl_internal(struct net_device *dev, struct ifreq *ifr, void __user *data, int cmd);

/*
 * ndo_ops structure for 11AC fast path
 */


#ifdef WLAN_FEATURE_FASTPATH
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static struct net_device_ops osif_11ac_fp_dev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_getstats,
#else
    .ndo_get_stats = osif_getstats,
#endif
    .ndo_open = osif_vap_open,
    .ndo_stop = osif_vap_stop,
    .ndo_start_xmit = NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
    .ndo_set_multicast_list = osif_set_multicast_list,
#else
    .ndo_set_rx_mode = osif_set_multicast_list,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    .ndo_siocwandev = ieee80211_ioctl_wandev,
    .ndo_siocdevprivate = ieee80211_ioctl_devprivate,
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_do_ioctl = ieee80211_ioctl,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_change_mtu = osif_change_mtu,
    .ndo_set_mac_address = osif_change_mac_addr,
};

static struct net_device_ops osif_11ac_wifi3_fp_dev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_getstats,
#else
    .ndo_get_stats = osif_getstats,
#endif
    .ndo_open = osif_vap_open,
    .ndo_stop = osif_vap_stop,
    .ndo_start_xmit = NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
    .ndo_set_multicast_list = osif_set_multicast_list,
#else
    .ndo_set_rx_mode = osif_set_multicast_list,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    .ndo_siocwandev = ieee80211_ioctl_wandev,
    .ndo_siocdevprivate = ieee80211_ioctl_devprivate,
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_do_ioctl = ieee80211_ioctl,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_change_mtu = osif_change_mtu,
    .ndo_set_mac_address = osif_change_mac_addr,
};
#endif
#endif /* WLAN_FEATURE_FASTPATH */

#ifdef QCA_SUPPORT_WDS_EXTENDED

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
/* Will be overloading only change mac functionality
 * all other remains same as that of legacy wds_ext
 */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static struct net_device_ops osif_mld_phy_wds_ext_dev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_wds_ext_getstats,
#else
    .ndo_get_stats = osif_wds_ext_getstats,
#endif
    .ndo_open = osif_mld_phy_wds_ext_dev_open,
    .ndo_stop = osif_wds_ext_dev_stop,
    .ndo_start_xmit = NULL,
    .ndo_set_mac_address = osif_mld_phy_wds_ext_change_mac,
    .ndo_change_mtu = osif_wds_ext_change_mtu,
};
#endif
#endif /*ENABLE_CFG80211_BACKPORTS_MLO */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static struct net_device_ops osif_wds_ext_dev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_wds_ext_getstats,
#else
    .ndo_get_stats = osif_wds_ext_getstats,
#endif
    .ndo_open = osif_wds_ext_dev_open,
    .ndo_stop = osif_wds_ext_dev_stop,
    .ndo_start_xmit = NULL,
    .ndo_set_mac_address = osif_wds_ext_change_mac,
    .ndo_change_mtu = osif_wds_ext_change_mtu,
};
#endif
#endif /* QCA_SUPPORT_WDS_EXTENDED */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static struct net_device_ops osif_dev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_getstats,
#else
    .ndo_get_stats = osif_getstats,
#endif
    .ndo_open = osif_vap_open,
    .ndo_stop = osif_vap_stop,
    .ndo_start_xmit = NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
    .ndo_set_multicast_list = osif_set_multicast_list,
#else
    .ndo_set_rx_mode = osif_set_multicast_list,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    .ndo_siocwandev = ieee80211_ioctl_wandev,
    .ndo_siocdevprivate = ieee80211_ioctl_devprivate,
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_do_ioctl = ieee80211_ioctl,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_change_mtu = osif_change_mtu,
    .ndo_set_mac_address = osif_change_mac_addr,
};
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,2,0)) || PARTNER_ETHTOOL_SUPPORTED
static struct ethtool_ops osif_ethtool_ops;
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(3,2,0) */

static inline
void osif_dev_set_offload_tx_csum(struct net_device *dev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,2,0)
    dev->hw_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_HW_CSUM;
    dev->vlan_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_HW_CSUM;
    dev->ethtool_ops = &osif_ethtool_ops;
#else
    dev->features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_HW_CSUM;
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    /* ethtool interface changed for kernel versions > 3.3.0,
     * Corresponding changes as follows */
    dev->wanted_features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_HW_CSUM;
#endif
}


static inline
void osif_dev_set_tso(struct net_device *dev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,2,0) || PARTNER_ETHTOOL_SUPPORTED
    dev->hw_features |= NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
    /*  dev->gso_max_size  not setting explicitly since our HW capable handling any size packet,
     *  No limitation for MAX Jumbo Pkt size */
    /* Folowing is required to have TSO support for VLAN over VAP interfaces */
    dev->vlan_features |= NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
    dev->ethtool_ops = &osif_ethtool_ops;
#else
    /* For Lower kernel versions TSO is enabled always,
     * when enabled in build using the option HOST_SW_TSO_SG_ENABLE */
    dev->features |= NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0) || PARTNER_ETHTOOL_SUPPORTED
    /* ethtool interface changed for kernel versions > 3.3.0,
     * Corresponding changes as follows */
    dev->wanted_features |= NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif
}

static inline
void osif_dev_set_sg(struct net_device *dev)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,2,0)
    dev->hw_features |= NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
    /*  dev->gso_max_size  not setting explicitly since our HW capable handling any size packet,
     *  No limitation for MAX Jumbo Pkt size */
    /* Folowing is required to have TSO support for VLAN over VAP interfaces */
    dev->vlan_features |= NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
    dev->ethtool_ops = &osif_ethtool_ops;
#else
    /* For Lower kernel versions TSO is enabled always,
     * when enabled in build using the option HOST_SW_TSO_SG_ENABLE */
    dev->features |= NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0)
    /* ethtool interface changed for kernel versions > 3.3.0,
     * Corresponding changes as follows */
    dev->wanted_features |= NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
#endif
}

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static struct net_device_ops osif_nss_wifiol_dev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_getstats,
#else
    .ndo_get_stats = osif_getstats,
#endif
    .ndo_open = osif_vap_open,
    .ndo_stop = osif_vap_stop,
    .ndo_start_xmit = NULL,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
    .ndo_set_multicast_list = osif_set_multicast_list,
#else
    .ndo_set_rx_mode = osif_set_multicast_list,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    .ndo_siocwandev = ieee80211_ioctl_wandev,
    .ndo_siocdevprivate = ieee80211_ioctl_devprivate,
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_do_ioctl = ieee80211_ioctl,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    .ndo_change_mtu = osif_change_mtu,
    .ndo_set_mac_address = osif_change_mac_addr,
};
#endif

#ifdef QCA_SUPPORT_WDS_EXTENDED
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static struct net_device_ops osif_nss_wds_ext_vdev_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .ndo_get_stats64 = osif_wds_ext_getstats,
#else
    .ndo_get_stats = osif_wds_ext_getstats,
#endif
    .ndo_open = osif_wds_ext_dev_open,
    .ndo_stop = osif_wds_ext_dev_stop,
    .ndo_start_xmit = NULL,
    .ndo_set_mac_address = osif_wds_ext_change_mac,
    .ndo_change_mtu = osif_wds_ext_change_mtu,
};
#endif
#endif /* QCA_SUPPORT_WDS_EXTENDED */
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

#ifdef QCA_SUPPORT_WDS_EXTENDED
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
int osif_register_wds_ext_dev_ops_xmit(int (*vap_hardstart)(struct sk_buff *skb, struct net_device *dev), int netdev_type) {
    if (vap_hardstart == NULL)
        return -EINVAL;

    switch (netdev_type) {
    case OSIF_NETDEV_TYPE_OL_WIFI3:
    case OSIF_NETDEV_TYPE_OL:
        osif_wds_ext_dev_ops.ndo_start_xmit = vap_hardstart;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        osif_mld_phy_wds_ext_dev_ops.ndo_start_xmit = vap_hardstart;
#endif
        break;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    case OSIF_NETDEV_TYPE_NSS_WIFIOL:
        osif_nss_wds_ext_vdev_ops.ndo_start_xmit = vap_hardstart;
        break;
#endif
    default:
        return -EINVAL;
    }

    return 0;
}
#else
int osif_register_wds_ext_dev_ops_xmit(int (*vap_hardstart)(struct sk_buff *skb, struct net_device *dev), int netdev_type) {
    return 0;
}
#endif
qdf_export_symbol(osif_register_wds_ext_dev_ops_xmit);
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
int osif_register_dev_ops_xmit(int (*vap_hardstart)(struct sk_buff *skb, struct net_device *dev), int netdev_type) {
    if (vap_hardstart == NULL)
        return -EINVAL;

    switch (netdev_type) {
    case OSIF_NETDEV_TYPE_DA:
        osif_dev_ops.ndo_start_xmit = vap_hardstart;
        break;
#ifdef WLAN_FEATURE_FASTPATH
    case OSIF_NETDEV_TYPE_OL:
        osif_11ac_fp_dev_ops.ndo_start_xmit = vap_hardstart;
        break;
    case OSIF_NETDEV_TYPE_OL_WIFI3:
        osif_11ac_wifi3_fp_dev_ops.ndo_start_xmit = vap_hardstart;
        break;
#endif
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    case OSIF_NETDEV_TYPE_NSS_WIFIOL:
        osif_nss_wifiol_dev_ops.ndo_start_xmit = vap_hardstart;
        break;
#endif
    default:
        return -EINVAL;
        break;
    }
    return 0;
}
#else
int osif_register_dev_ops_xmit(int (*vap_hardstart)(struct sk_buff *skb, struct net_device *dev), int netdev_type) {
    return 0;
}
#endif
qdf_export_symbol(osif_register_dev_ops_xmit);

int
osif_create_vap_check(struct net_device *comdev,
        struct ieee80211_clone_params *cp,
        void *mld_params,
        enum virtual_interface_type vif_type)
{
    osdev_t os_handle = NULL;
    enum ieee80211_opmode opmode = cp->icp_opmode;
#ifdef UMAC_SUPPORT_CFG80211
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211*)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
#else
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211*)get_radio_device_context(NULL, comdev);
#endif
    struct ieee80211com *ic = &scn->sc_ic;
    bool scan_radio_support;
#ifdef WLAN_FEATURE_11BE_MLO
    struct qdf_mac_addr mld_addr;
    struct net_device *mld_dev = NULL;
    struct wlan_mlo_dev_context *ml_dev = NULL;
    struct osif_mldev *osifp_mld;
#endif
    bool macreq_enabled, is_vapid_set;

    os_handle = ic->ic_osdev;
    if (!capable(CAP_NET_ADMIN))
        return -EINVAL;

    cp->icp_opmode = ieee80211_opmode2qdf_opmode(opmode);
    if(cp->icp_opmode >= QDF_MAX_NO_OF_MODE) {
        qdf_print("opmode(%d) is not supported", opmode);
        return -EINVAL;
    }

    scan_radio_support =
            wlan_pdev_nif_feat_ext_cap_get(ic->ic_pdev_obj,
                                           WLAN_PDEV_FEXT_SCAN_RADIO);
    if (scan_radio_support && !((cp->icp_flags & IEEE80211_SPECIAL_VAP) &&
        !(cp->icp_flags & IEEE80211_SMART_MONITOR_VAP))) {
        qdf_err("only special vap type is allowed for scan radio pdev");
        return -EINVAL;
    }

#if QCA_LTEU_SUPPORT
    if (ic->ic_nl_handle && cp->icp_opmode != QDF_SAP_MODE) {
        qdf_print("Only AP mode %d is supported for NL, not %d", QDF_SAP_MODE, cp->icp_opmode);
        return -EINVAL;
    }
    if(QDF_IBSS_MODE == cp->icp_opmode)
    {
        /* IBSS is not supported in partial offload architecture in linux
           As wlanconfig does not know about arhictecure i.e.e DA or partial offload
           so it sets the mode which may result a crash of firmware
           so refining it here and making sure if mode is IBSS and architecture
           is peregrine then we should not create VAP */
        qdf_print("ADHOC Not enabled in partial offload ");
        return -EINVAL;
    }
#endif

    macreq_enabled = ic->ic_is_macreq_enabled(ic);
    /* Check whether user-space indicates that valid vapid is passed since some
     * tools do not initialize vapid to -1 when not used and we cannot differentiate
     * id=0 as a legit vapid=0 from defualt zero initializer.
     */
    is_vapid_set = !!(cp->icp_flags & IEEE80211_VAPID_SET);

    if ((macreq_enabled && !is_vapid_set) || (!macreq_enabled && is_vapid_set)) {
        if (macreq_enabled)
            qdf_err("VAP ID should be provided when MAC request is enabled");
        else
            qdf_err("VAP ID should not be provided when MAC request is not enabled");

        return -EINVAL;
    }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    /*
     * For Legacy VAPs mld params check is not required
     * mld_params check etc.. requiired for VIF_TYPE_MLD_LINK_VAP only
     */
    if (vif_type == VIF_TYPE_LEGACY)
        return 0;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    /* VAP check for mld config */
    if (mld_params && !(cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP)) {
        IEEE80211_ADDR_COPY(&mld_addr,
                 ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr);
        if (!qdf_is_macaddr_zero(&mld_addr)) {
            ml_dev = wlan_mlo_get_mld_ctx_by_mldaddr(&mld_addr);
            if (ml_dev) {
                if (wlan_mlo_check_valid_config(ml_dev, ic->ic_pdev_obj, cp->icp_opmode) != QDF_STATUS_SUCCESS) {
                    qdf_err("Vdev: %s create failed due to Invalid user config for vdev!", cp->icp_name);
                    return -EINVAL;
                }
            }
        } else if (((struct ieee80211_mld_create_params *)mld_params)->mld_ifname_valid) {
            mld_dev = osif_find_mld_dev_by_ifname(
                    ((struct ieee80211_mld_create_params *)
                     mld_params)->mld_ifname);
            if (mld_dev) {
                osifp_mld = osif_mld_get_mldev(mld_dev);
                if (osifp_mld && osifp_mld->derived_mld_addr)
                    IEEE80211_ADDR_COPY(&mld_addr, cp->icp_bssid);
                else
                    IEEE80211_ADDR_COPY(&mld_addr, mld_dev->dev_addr);

                if (!qdf_is_macaddr_zero(&mld_addr)) {
                    ml_dev = wlan_mlo_get_mld_ctx_by_mldaddr(&mld_addr);
                    if (ml_dev) {
                        if (wlan_mlo_check_valid_config(ml_dev, ic->ic_pdev_obj,
                                    cp->icp_opmode) != QDF_STATUS_SUCCESS) {
                            qdf_err("Vdev: %s create failed due to Invalid user config for vdev!",
                                    cp->icp_name);
                            return -EINVAL;
                        }
                    }
                }
            }
        }
    }
#endif
    return 0;
}

osif_dev *
osif_recover_vap_netdev(struct ieee80211vap *vap)
{
    struct net_device *dev = NULL;
    osif_dev  *osifp = NULL;
    int unit;
#if UMAC_SUPPORT_CFG80211
    struct wireless_dev iv_wdev;
#endif
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    nss_if_num_t nss_ifnum;
    uint32_t recovery_state;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    struct osif_mldev *mldev;
#endif
#if QCA_PPE_VP
#ifndef QCA_SINGLE_WIFI_3_0
    uint32_t vp_num;
    uint32_t ppe_vp_type;
    uint32_t ppe_vp_profile_idx;
#ifdef WLAN_SUPPORT_PPEDS
    uint32_t ppeds_node_id = PPE_VP_DS_INVALID_NODE_ID;
#endif
#endif
#endif
    uint8_t dev_type;
    void * ctrl_vdev;
    bool is_mlo_bridge = (vap->cp.icp_flags & IEEE80211_MLO_BRIDGE_VAP) ? true : false;

    /* copy few things to restore down */
    osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);
    dev = osifp->netdev;
    unit = osifp->os_unit;
    ctrl_vdev = (void *)osifp->ctrl_vdev;

#if UMAC_SUPPORT_CFG80211
    if (!is_mlo_bridge || (is_mlo_bridge && osifp->netdev)) {
        memcpy(&iv_wdev, &osifp->iv_wdev, sizeof(iv_wdev));
    }
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    nss_ifnum = osifp->nss_recov_if;
    recovery_state = osifp->nss_recov_mode;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    mldev = osifp->mldev;
#endif
#if QCA_PPE_VP
#ifndef QCA_SINGLE_WIFI_3_0
    vp_num = osifp->ppe_vp_num;
    ppe_vp_type = osifp->ppe_vp_type;
    ppe_vp_profile_idx = osifp->ppe_vp_profile_idx;
#ifdef WLAN_SUPPORT_PPEDS
    ppeds_node_id = osifp->ppeds_node_id;
#endif
#endif
#endif
    dev_type = osifp->dev_type;
    ieee80211_vap_deleted_clear(vap); /* clear the deleted */

    memset(osifp,0,sizeof(osif_dev));

    spin_lock_init(&osifp->list_lock);

    spin_lock_init(&osifp->tx_lock);

    osifp->os_unit = unit;
    osifp->os_if = vap;
    osifp->netdev = dev;
    osifp->rx_dev = dev;
    osifp->ctrl_vdev = ctrl_vdev;

    osifp->is_bridge_link = is_mlo_bridge;

#if UMAC_SUPPORT_CFG80211
    if (!is_mlo_bridge || (is_mlo_bridge && osifp->netdev)) {
        memcpy(&osifp->iv_wdev, &iv_wdev, sizeof(osifp->iv_wdev));
    }
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    osifp->nss_recov_if = nss_ifnum;
    osifp->nss_recov_mode = recovery_state;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    osifp->mldev = mldev;
    if (mldev)
        osifp->rx_dev = osif_mld_get_rx_ndev(mldev->mld_dev, dev);
#endif
#if QCA_PPE_VP
#ifndef QCA_SINGLE_WIFI_3_0
    osifp->ppe_vp_num = vp_num;
    osifp->ppe_vp_type = ppe_vp_type;
    osifp->ppe_vp_profile_idx = ppe_vp_profile_idx;
#ifdef WLAN_SUPPORT_PPEDS
    osifp->ppeds_node_id = ppeds_node_id;
#endif
#endif
#endif
    osifp->dev_type = dev_type;
    return osifp;
}

int
osif_create_vap_netdev_free(struct net_device *dev, int unit)
{
    if (!dev)
        return 0;

    qdf_net_delete_wlanunit(unit);
    free_netdev(dev);

    return 0;
}

int
osif_create_vap_netdev_unregister(struct net_device *dev, int unit)
{
    qdf_net_delete_wlanunit(unit);
    wlan_cfg80211_unregister_netdevice(dev);

    return 0;
}

static osif_dev *
osif_create_link_vap_netdev(struct ieee80211_clone_params *cp,
                            struct ieee80211com *ic, void *mld_params)
{
    struct net_device *dev = NULL;
    char name[IFNAMSIZ];
    char unum[QCA_INTF_IDXSIZE];
    int error, unit;
    osif_dev  *osifp = NULL;
    uint8_t multiq_support_enabled = ic->multiq_support_enabled;
    bool is_mlo_bridge = (cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP) ? true : false;
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    /* Allocation is tricky here, so let's give a few explanation.
     * If the ini param for name-strings is not set:
     * We are basically trying to handle two cases:
     * - if the number isn't specified by the user, we have to allocate one,
     *   in which case we need to make sure it's not already been allocated
     *   already. User says "ath" and we create "athN" with N being a new unit#
     * - if the number is specified, we just need to make sure it's not been
     *   allocated already, which we check using dev_get_by_name()
     *
     *   If the ini param is set, then :
     *   - the netdev-name doesnt need to have the number attached
     *   - there is no need to look for the number in the string provided
     *     by the user
     *   - do not allow reuse of an existing netdev-name
     */
    if (!(cfg_get(psoc, CFG_OL_WILDCARD_NDEV_NAME))) {
        error = qdf_net_ifc_name2unit(cp->icp_name, &unit);
        if (error) {
            qdf_err("Failed to extract unit number");
            return NULL;
        }

        if (unit == -1) {
            if (!is_mlo_bridge)
                unit = qdf_net_new_wlanunit();
            else
                unit = qdf_net_new_mlobrunit();
            if (unit == -1) {
                qdf_err("Failed to allocate unit number");
                return NULL;
            }
            snprintf(name, sizeof(name), "%s", cp->icp_name);
            snprintf(unum, sizeof(unum), "%d", unit);
            if (strlen(name) < (IFNAMSIZ - QCA_INTF_IDXSIZE)) {
               if (strlcat(name, unum, sizeof(name))  >=  sizeof(name)) {
                  qdf_err("source too long");
                  return NULL;
               }
            }
        } else {
            int dev_exist = qdf_net_dev_exist_by_name(cp->icp_name);
            if (dev_exist) {
                qdf_err("%s net dev exists already", cp->icp_name);
                return NULL;
            }
            if (!is_mlo_bridge)
                qdf_net_alloc_wlanunit(unit);
            else
                qdf_net_alloc_mlobrunit(unit);
            if (strlcpy(name, cp->icp_name, sizeof(name)) >=  sizeof(name)) {
                qdf_err("source too long");
                return NULL;
            }
            name[IFNAMSIZ - 1] = '\0';
        }
    } else {
        if (qdf_net_dev_exist_by_name(cp->icp_name)) {
            qdf_err("%s net dev exists already", cp->icp_name);
            return NULL;
        }
        if (!is_mlo_bridge)
            unit = qdf_net_new_wlanunit();
        else
            unit = qdf_net_new_mlobrunit();
        if (unit == -1) {
            qdf_err("Failed to allocate unit number");
            return NULL;
        }
        if (strlcpy(name, cp->icp_name, sizeof(name)) >=  sizeof(name)) {
            qdf_err("source too long");
            return NULL;
        }
        name[IFNAMSIZ - 1] = '\0';
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)

    if (multiq_support_enabled)
        dev = alloc_netdev_mqs(sizeof(osif_dev), "wifi%d", 0, ether_setup,
                OSIF_DP_NETDEV_TX_QUEUE_NUM, OSIF_DP_NETDEV_RX_QUEUE_NUM);
    else
        dev = alloc_netdev(sizeof(osif_dev), "wifi%d", 0, ether_setup);
#else
    if (multiq_support_enabled)
        dev = alloc_netdev_mqs(sizeof(osif_dev), name, ether_setup,
                OSIF_DP_NETDEV_TX_QUEUE_NUM, OSIF_DP_NETDEV_RX_QUEUE_NUM);
    else
        dev = alloc_netdev(sizeof(osif_dev), name, ether_setup);
#endif
    if (dev == NULL) {
        if (!is_mlo_bridge)
            qdf_net_delete_wlanunit(unit);
        else
            qdf_net_delete_mlobrunit(unit);
        return NULL;
    }
    if (name != NULL)   /* XXX */
        if (strlcpy(dev->name, name, sizeof(dev->name)) >= sizeof(dev->name)) {
            qdf_err("source too long");
            return NULL;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
    dev->max_mtu = ATH_MAX_MTU;
#endif

    /* For VIF_TYPE_LEGACY osifp is allocated by netdev priv */
    osifp = ath_netdev_priv(dev);

    /*
    ** Clear and initialize the osif structure
    */
    memset(osifp,0,sizeof(osif_dev));
    osifp->netdev = dev;
    osifp->rx_dev = dev;

    osifp->os_unit  = unit;

    return osifp;
}

osif_dev *
osif_create_vap_netdev_alloc(struct ieee80211_clone_params *cp,
        struct ieee80211com *ic, void *mld_params,
        enum virtual_interface_type vif_type)
{
    osif_dev  *osifp = NULL;
    bool is_mlo_bridge = (cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP) ? true : false;
#ifdef WLAN_FEATURE_11BE_MLO
    struct qdf_mac_addr mld_addr;
    struct net_device *mld_dev;
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    uint8_t grp_id = wlan_mlo_get_psoc_group_id(psoc);
    uint8_t ml_dev_allocated = 0;
#endif

    if (vif_type == VIF_TYPE_INVALID) {
        qdf_err("invalid vif_type");
        return NULL;
    }

    if (vif_type == VIF_TYPE_MLD_LINK_VAP) {
        /* for mld link vaps netdev won't be created
         * only osifp will be allocated
         */
        osifp = qdf_mem_malloc(sizeof(osif_dev));
        if (osifp == NULL) {
            qdf_err("osifp create failed for MLO link vap");
            return NULL;
        }
        memset(osifp,0,sizeof(osif_dev));
    } else {
        osifp = osif_create_link_vap_netdev(cp, ic, mld_params);
        if (osifp == NULL) {
            qdf_err("osifp create failed for vif type = %d name = %s", vif_type, cp->icp_name);
            return NULL;
        }
    }

    spin_lock_init(&osifp->list_lock);

    spin_lock_init(&osifp->tx_lock);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    /*
     * Initialize NSS recovery interface number for VAP.
     */
    osifp->nss_recov_if = -1;
    osifp->nss_recov_mode = false;
#endif
    if (is_mlo_bridge)
        return osifp;

#ifdef WLAN_FEATURE_11BE_MLO
    if (mld_params
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
	&& (vif_type != VIF_TYPE_LEGACY)
#endif
       ) {
        IEEE80211_ADDR_COPY(&mld_addr,
                ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr);

        if (!qdf_is_macaddr_zero(&mld_addr)) {
            mld_dev = osif_find_mld_dev(((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr, grp_id);
            if (mld_dev) {
                osifp->mldev = osif_mld_get_mldev(mld_dev);
                osifp->rx_dev = osif_mld_get_rx_ndev(mld_dev, osifp->netdev);
                ml_dev_allocated = 1;
            }
        }
        /*
        * if mld params had dummy mac addr, mld_dev would not have
        * been found searching by dummy mac address.
        */
        if (!ml_dev_allocated) {
            if (((struct ieee80211_mld_create_params *)
                        mld_params)->mld_ifname_valid) {
                mld_dev = osif_find_mld_dev_by_ifname(
                        ((struct ieee80211_mld_create_params *)
                         mld_params)->mld_ifname);
                if (mld_dev) {
                    osifp->mldev = osif_mld_get_mldev(mld_dev);
                    osifp->rx_dev = osif_mld_get_rx_ndev(mld_dev, osifp->netdev);
                    ml_dev_allocated = 1;
                } else {
                    qdf_debug("Cannot find mld netdev by mac address nor by ifname %s",
                              ((struct ieee80211_mld_create_params *)mld_params)->mld_ifname);
                    return NULL;
                }
            }
        }
    }
#endif
    /* hold the reference of rx_dev */
    qdf_net_if_hold_dev((struct qdf_net_if *)osifp->rx_dev);
    return osifp;
}

int
osif_create_vap_bssid_free(struct ieee80211com *ic,
                   uint8_t *req_mac,
                   struct ieee80211_clone_params *cp)
{
    bool macreq_enabled = FALSE;

    macreq_enabled = ic->ic_is_macreq_enabled(ic);
    /* remove the bssid from vapid */
    if( (cp->icp_vapid != VAP_ID_AUTO) &&
        ((cp->icp_flags & IEEE80211_CLONE_BSSID) == 0) &&
        (macreq_enabled == TRUE))
        {
            wlan_vap_free_mac_addr(ic, req_mac, cp);
        }

    return 0;
}

static bool
mesh_mbssid_mac_sanity_check(struct ieee80211com *ic,
                             u_int8_t *mesh_bssid,
                             uint8_t *ref_bssid)
{
    u_int8_t temp_mesh[IEEE80211_ADDR_LEN];
    u_int8_t temp_ref[IEEE80211_ADDR_LEN];

    qdf_mem_copy(temp_mesh, mesh_bssid, IEEE80211_ADDR_LEN);
    qdf_mem_copy(temp_ref, ref_bssid, IEEE80211_ADDR_LEN);

    /* This logic is with the assumption of n <= 8 always */
    temp_mesh[ATH_BSSID_INDEX] &= ~(MBSSID_SUPER_GROUP_SIZE(ic) - 1);
    temp_ref[ATH_BSSID_INDEX] &= ~(MBSSID_SUPER_GROUP_SIZE(ic) - 1);

    if (!qdf_mem_cmp(temp_mesh, temp_ref, IEEE80211_ADDR_LEN)) {
        mbss_err("48-n bits of mesh vap bssid can not be same as ref_bssid %s",
                 ether_sprintf(ref_bssid));
        return false;
    }

    return true;
}

static bool
osif_mbssid_sanity_check(struct ol_ath_softc_net80211 *scn,
                         struct ieee80211_clone_params *cp)
{
    struct ieee80211com *ic = &scn->sc_ic;
    bool is_mbssid_enabled  = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                                WLAN_PDEV_F_MBSS_IE_ENABLE);
    bool sanity_ok          = true;
    uint8_t i;
    /* Number of vaps in mbss-cache */
    uint8_t nvaps_in_cache;
    ol_ath_ema_pdev_ctx_t *ema_ctx = &scn->ema_ctx;
    struct mbssid_context *mbss_ctx;

    nvaps_in_cache = (is_mbssid_enabled) ?
                      ieee80211_mbssid_get_num_vaps_across_ap_mbss_caches(ic) :
                              ieee80211_get_num_beacon_ap_vaps(ic);

    if (is_mbssid_enabled || ic->ic_wideband_capable) {
        if (cp->icp_opmode != QDF_SAP_MODE ||
            cp->icp_flags & IEEE80211_SMART_MONITOR_VAP ||
            cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP ||
            cp->icp_flags & IEEE80211_SPECIAL_VAP) {
            mbss_err("mbssid sanity required for AP VAPs only");
            goto exit;
        }

        mbss_ctx = ic->ic_mbss_ctx;
        if (!mbss_ctx) {
            mbss_err("mbss_ctx is null!!");
            sanity_ok = false;
            goto exit;
        }

        if (cp->icp_flags & IEEE80211_MESH_VAP) {
            if ((cp->icp_flags & IEEE80211_CLONE_BSSID) == 0) {
                sanity_ok = mesh_mbssid_mac_sanity_check(ic, cp->icp_bssid, mbss_ctx->ref_bssid);
            }
            goto exit;
        }

        /* Fail sanity check if num_max_vaps no
         * of vaps are already created
         */
        if (nvaps_in_cache >= ema_ctx->num_max_vaps) {
            mbss_err("mbssid_sanity failed - num_max_vaps"
                     " = %d vaps already created",
                     ema_ctx->num_max_vaps);
            sanity_ok = false;
            goto exit;
        }

        if ((cp->icp_flags & IEEE80211_CLONE_BSSID) == 0 &&
            !qdf_is_macaddr_zero((struct qdf_mac_addr *)cp->icp_bssid)) {
            bool is_first_ap_vap = !nvaps_in_cache;
            if (is_first_ap_vap) {
                /* Overwrite the ref_bssid */
                IEEE80211_ADDR_COPY(mbss_ctx->ref_bssid, cp->icp_bssid);
            } else {
                /* Sanity check on 48-n MSB bits required */
                for (i = 0; i < IEEE80211_ADDR_LEN - 1; i++) {
                    if (cp->icp_bssid[i] != mbss_ctx->ref_bssid[i]) {
                        mbss_err("48-n MSB of bssid does not match"
                                 " with that of ref_bssid");
                        sanity_ok = false;
                        goto exit;
                    }
                }

                if ((cp->icp_bssid[ATH_BSSID_INDEX] &
                        ~(MBSSID_SUPER_GROUP_SIZE(ic) - 1)) !=
                    (mbss_ctx->ref_bssid[ATH_BSSID_INDEX] &
                        ~(MBSSID_SUPER_GROUP_SIZE(ic) - 1))) {
                    mbss_err("48-n MSB of bssid does not match"
                             " with that of ref_bssid");
                    sanity_ok = false;
                    goto exit;
                }
            } /* end check for first vap */
        } /* end check for clone_bssid */
    } /* check for mbssid feature enable */

exit:
    mbss_info("mbssid_sanity_ok: %s", sanity_ok ? "YES" : "NO");
    return sanity_ok;
}

static QDF_STATUS
osifp_create_wlan_vap_bss_status(struct ieee80211com *ic,
                                 struct ieee80211vap *vap)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    uint16_t waitcnt;
    qdf_event_t wait_event;
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    qdf_mem_zero(&wait_event, sizeof(wait_event));
    qdf_event_create(&wait_event);
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (wlan_psoc_nif_feat_cap_get(psoc,
                                   WLAN_SOC_F_PEER_CREATE_RESP)) {
        waitcnt = 0;
        qdf_event_reset(&wait_event);

        while(!wlan_vap_get_bss_rsp_evt_status(vap) &&
              waitcnt < WLAN_VDEV_CREATE_TIMEOUT_CNT) {
              qdf_wait_single_event(&wait_event, (WLAN_VDEV_CREATE_TIMEOUT/2));
              waitcnt++;
        }

        if (!wlan_vap_get_bss_rsp_evt_status(vap) ||
            !wlan_vap_is_bss_created(vap)) {
            status = QDF_STATUS_E_FAILURE;
        }
    } else {
        wlan_vap_set_bss_status(vap, PEER_CREATED);
    }

    qdf_event_destroy(&wait_event);
    return status;
}

#ifdef WLAN_FEATURE_11BE_MLO
int osif_get_mldev_params(struct wiphy *wiphy, struct osif_mldev *mld_osdev)
{
    struct wireless_dev *wdev;
    osif_dev *osdev;
    int i, j, ret, flen, nb, bsize;
    char buffer[256];

    if (!mld_osdev)
        return -ENODEV;

    bsize = sizeof(buffer);
    qdf_mem_zero(buffer, bsize);
    qdf_debug("mld dev : %s", mld_osdev->mld_dev->name);

    wdev = &mld_osdev->wdev;

    qdf_debug("type : %u", wdev->iftype);
    qdf_debug("link devs :");
    nb = snprintf(buffer, bsize, "mld dev: %s\ntype: %u\nlink devs : \n",
                  mld_osdev->mld_dev->name, wdev->iftype);
    flen = nb;
    if (nb < 0 || nb >= bsize)
    {
        qdf_err("Fail to update cfgcmd reply buffer, Exp: %d, Act: %d", (bsize - 1), flen);
        goto out;
    }

    for (i = 0; i < MAX_MLO_CHIPS; i++) {
        for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
            if (!(osdev = mld_osdev->link_dev[i][j]))
                continue;
            qdf_debug("chip id %u pdev %u dev %s", i, j, osdev->netdev->name);
            nb = snprintf((buffer + flen),
                          (bsize - flen),
                          "chip id %u pdev %u dev %s\n", i, j, osdev->netdev->name);
            flen += nb;
            if (nb < 0 || nb >= (bsize - flen))
            {
                qdf_err("Fail to update cfgcmd reply buffer, Exp: %d, Act: %d", (bsize - 1), flen);
                goto out;
            }
        }
    }

    qdf_debug("primary_chip_id : %u", mld_osdev->primary_chip_id);
    qdf_debug("primary_pdev_id : %u", mld_osdev->primary_pdev_id);
    nb = snprintf((buffer + flen), (bsize - flen),
                  "primary_chip_id : %u\nprimary_pdev_id : %u",
                  mld_osdev->primary_chip_id, mld_osdev->primary_pdev_id);
    flen += nb;
    if (nb < 0 || nb >= (bsize - flen))
    {
        qdf_err("Fail to update cfgcmd reply buffer, Exp: %d, Act: %d", (bsize - 1), flen);
        goto out;
    }

out:
    ret = cfg80211_reply_command(wiphy, bsize, buffer, 0);

    return ret;
}

#ifdef CONFIG_BOND_MOD_SUPPORT
/*
 * This will be triggered by kernel before freeing netdev
 */
void osif_mld_bond_priv_destructor(struct net_device *dev)
{
    struct osif_mldev *mldev = osif_mld_get_mldev(dev);

    qdf_mem_free(mldev);
}

struct net_device*
osif_create_mld_bond(struct wiphy* mld_wiphy, void *mld_params,
                     const char *iname, enum nl80211_iftype type)
{
    char name[IFNAMSIZ];
    char unum[QCA_INTF_IDXSIZE];
    int error, unit;
    struct net_device *mldev = NULL;
    struct osif_mldev *osifp_mld = NULL;
    struct sockaddr sa;
    uint8_t grp_id;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_ctx = wiphy_priv(mld_wiphy);
#else
    struct cfg80211_mld_wiphy_priv *mld_ctx = wiphy_priv(mld_wiphy);
#endif

     /*   If the ini param is set, then :
      *   - the netdev-name doesnt need to have the number attached
      *   - there is no need to look for the number in the string provided
      *     by the user
      *   - do not allow reuse of an existing netdev-name
      */
    if (!osif_psoc_get_ini_ndev_name()) {
        error = qdf_net_ifc_name2unit(iname, &unit);
        if (error) {
            qdf_err("Failed to extract unit number");
            return NULL;
        }

        if (unit == -1) {
            unit = qdf_net_new_mldunit();
            if (unit == -1) {
                qdf_err("Failed to allocate unit number");
                return NULL;
            }
            snprintf(name, sizeof(name), "%s", iname);
            snprintf(unum, sizeof(unum), "%d", unit);
            if (strlen(name) < (IFNAMSIZ - QCA_INTF_IDXSIZE)) {
                if (strlcat(name, unum, sizeof(name))  >=  sizeof(name)) {
                    qdf_err("source too long");
                    return NULL;
                }
            }
        } else {
            int dev_exist = qdf_net_dev_exist_by_name(iname);
            if (dev_exist) {
                qdf_debug("%s net dev exists already", iname);
                return NULL;
            }

            qdf_net_alloc_mldunit(unit);
            if (strlcpy(name, iname, sizeof(name)) >=  sizeof(name)) {
                qdf_err("source too long");
                return NULL;
            }
            name[IFNAMSIZ - 1] = '\0';
        }
    } else {
            if (qdf_net_dev_exist_by_name(iname)) {
                qdf_debug("%s net dev exists already", iname);
                return NULL;
            }

            unit = qdf_net_new_mldunit();
            if (strlcpy(name, iname, sizeof(name)) >=  sizeof(name)) {
                qdf_err("source too long");
                return NULL;
            }
            name[IFNAMSIZ - 1] = '\0';
    }

    osifp_mld = qdf_mem_malloc(sizeof(struct osif_mldev));
    if (!osifp_mld) {
        qdf_err("Failed to allocate MLD priv");
        return NULL;
    }

    if (osif_mlo_peer_find_hash_attach(osifp_mld, 1000))
        qdf_err("osif_peer hash attch failed");

    osifp_mld->wdev.wiphy = mld_wiphy;
    osifp_mld->wdev.iftype = type;
    osifp_mld->dev_type = OSIF_NETDEV_TYPE_MLO;
    if (type == NL80211_IFTYPE_STATION) {
        osifp_mld->wdev.use_4addr = 1;
        qca_bond_cb.bond_mlo_xmit_netdev = osif_sta_mlo_xmit_netdev_bond;
        qca_bond_cb.bond_get_mlo_tx_netdev = osif_get_sta_mlo_tx_netdev;
    } else {
        qca_bond_cb.bond_mlo_xmit_netdev = osif_ap_mlo_xmit_netdev_bond;
        qca_bond_cb.bond_get_mlo_tx_netdev = osif_get_ap_mlo_tx_netdev;
    }
    qca_bond_cb.bond_mlo_netdev_priv_destructor = osif_mld_bond_priv_destructor;
    qca_bond_cb.bond_mlo_ctx = (void *)osifp_mld;
    qca_bond_cb.wdev = &osifp_mld->wdev;

    qdf_info("qca_bond_cb.wdev: %p", qca_bond_cb.wdev);
    mldev = qal_bond_create(&init_net, name, &qca_bond_cb);
    if (!mldev) {
        qdf_err("Failed to create MLD netdevice");
        osif_mlo_peer_find_hash_detach(osifp_mld);
        qdf_net_delete_mldunit(unit);
        qdf_mem_free(osifp_mld);
        return NULL;
    }

    sa.sa_family = mldev->type;
    memcpy(&sa.sa_data, ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr, IEEE80211_ADDR_LEN);
    dev_set_mac_address(mldev, &sa, NULL);

    qdf_mem_copy(osifp_mld->mld_mac_addr, ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr, QDF_MAC_ADDR_SIZE);

    osifp_mld->mld_dev = mldev;
    osifp_mld->pkt_fail_cnt = 0;
    osifp_mld->is_raw_mld = 0;
    osifp_mld->is_mesh_mld = 0;

    if (name != NULL) {
        if (strlcpy(mldev->name, name, sizeof(mldev->name)) >= sizeof(mldev->name)) {
            qdf_err("source too long");
            goto fail;
        }
    }

    mldev->needed_headroom = sizeof (struct ieee80211_qosframe) + sizeof(struct llc) + QDF_MAC_ADDR_SIZE +
#if MESH_MODE_SUPPORT
    sizeof(struct meta_hdr_s) +
#endif
    IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
    qdf_info("%s: needed_headroom reservation %d", mldev->name, mldev->needed_headroom);

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    /* Initilize all data elements of osifp_mld */
    osifp_mld->num_link_vaps = 0;
    osifp_mld->wiphy = mld_wiphy; 
    qdf_mem_zero(osifp_mld->link_vifs[0], (sizeof(osif_dev *) * IEEE80211_MLD_MAX_NUM_LINKS));
#endif

    grp_id = mld_ctx->ml_grp_id;
    if (osif_mldev_add_mld_osdev(osifp_mld, grp_id)) {
        qdf_err("failure in updating the MLD net device");
        goto fail;
    }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&mldev->dev.kobj,
                                        (struct qdf_vfs_attr *)&mld_attr_group)) {
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                           "%s: sysfs create group failed\n", __FUNCTION__);
    }
#endif

    return mldev;

fail:
    qdf_err("failure in initializing MLD net device");
    osif_mlo_peer_find_hash_detach(osifp_mld);
    qal_bond_destroy(mldev);
    return NULL;
}
#else
struct net_device*
osif_create_mld_bond(struct wiphy* mld_wiphy, void *mld_params,
                     const char *iname, enum nl80211_iftype type)
{
    return NULL;
}
#endif

struct net_device *
osif_find_mld_dev(uint8_t *mld_macaddr, uint8_t grp_id)
{
    int i;
    struct cfg80211_mld_wiphy_context *mld_ctx = NULL;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_priv = NULL;
#else
    struct cfg80211_mld_wiphy_priv *mld_priv = NULL;
#endif

    if (grp_id >= WLAN_MAX_MLO_GROUPS ) {
        qdf_err ("The mld grp_id is Invalid");
        qdf_debug("grp_id:%d, mld_macaddr:%s\n", grp_id, ether_sprintf(mld_macaddr));
        return NULL;
    }

    qdf_debug("The mld grp_id is:%d", grp_id);

    mld_ctx = &mld_wiphy_ctx[grp_id];

    if (mld_ctx->mld_wiphy == NULL) {
        qdf_err("mld phy not created");
        return NULL;
    }
    mld_priv = wiphy_priv(mld_ctx->mld_wiphy);

    for (i=0; i<MAX_MLDEV; i++) {
        if (mld_priv->mldev[i]) {
            struct osif_mldev *mldev = mld_priv->mldev[i];
            if (OS_MEMCMP(mldev->mld_mac_addr, mld_macaddr, QDF_MAC_ADDR_SIZE) == 0) {
                if (mldev->mld_dev)
                return mldev->mld_dev;
            }
        }
    }

    qdf_err("net_dev not found");
    return NULL;
}
struct net_device *
osif_find_mld_dev_for_all_groups(uint8_t *mld_macaddr) {
    struct net_device *temp = NULL;
    int j;

    for (j=0; j<WLAN_MAX_MLO_GROUPS; j++) {
        temp = osif_find_mld_dev(mld_macaddr, j);
        if (temp)
            return temp;
    }
    return temp;
}

struct net_device *
osif_find_mld_dev_by_ifname(char *ifname)
{
    struct net_device *temp = NULL;
    int i, j, ifname_len;
    struct cfg80211_mld_wiphy_context *mld_ctx;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_priv = NULL;
#else
    struct cfg80211_mld_wiphy_priv *mld_priv = NULL;
#endif

    if (!ifname) {
        qdf_err("net_dev is NULL");
        return NULL;
    }

    ifname_len = strlen(ifname) + 1;
    for (j=0; j<WLAN_MAX_MLO_GROUPS; j++) {
        mld_ctx = &mld_wiphy_ctx[j];

        if (!mld_ctx) {
            qdf_debug("MLD context not created at idx %d", j);
            continue;
        }
        if (mld_ctx->mld_wiphy == NULL) {
            qdf_err("MLD phy not created at idx %d", j);
            continue;
        }

        mld_priv = wiphy_priv(mld_ctx->mld_wiphy);
        if (!mld_priv) {
            qdf_err("MLD wiphy private pointer at idx %d is NULL ", j);
            continue;
        }
        for (i=0; i<MAX_MLDEV; i++) {
            if (mld_priv->mldev[i]) {
                struct osif_mldev *mldev = mld_priv->mldev[i];
                temp = mldev->mld_dev;
                if (!temp) {
                    qdf_debug("MLD dev at idx %d of MLO group at idx %d is NULL", i, j);
                    continue;
                }
                if (!temp->name) {
                    qdf_debug("MLD dev name at idx %d of MLO group at idx %d is NULL", i, j);
                    continue;
                }
                if(temp && temp->name &&
                        (strncmp(temp->name, ifname, ifname_len) == 0)) {
                    return temp;
                }
            }
        }
    }
    qdf_err("net_dev '%s' not found by name", ifname);
    return NULL;
}

#ifdef CONFIG_BOND_MOD_SUPPORT
int
osif_destroy_mld_bond(struct net_device *mldev)
{
    int ret = -1, i, j, status;
    uint8_t grp_id;
    struct osif_mldev *osifp_mld;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_priv;
#else
    struct cfg80211_mld_wiphy_priv *mld_priv;
#endif

    osifp_mld = osif_mld_get_mldev(mldev);
    if (!osifp_mld) {
        qdf_debug("mldev context is NULL");
        return ret;
    }

    mld_priv = wiphy_priv(osifp_mld->wdev.wiphy);

    grp_id = mld_priv->ml_grp_id;

    if (grp_id >= WLAN_MAX_MLO_GROUPS ) {
        qdf_err("Invalid grp_id");
        return ret;
    }

    if (osif_mldev_remove_mld_osdev(osifp_mld)) {
        qdf_err("net_dev %s not found", mldev->name);
        return -EINVAL;
    }

    osif_mlo_peer_find_hash_detach(osifp_mld);

    status = qal_bond_destroy(mldev);

    if (status == 1)
        return 0;

    qdf_err("%s destroy failed", mldev->name);

    for (i = 0; i < MAX_MLO_CHIPS; i++) {
        for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
            osif_dev *osifp;

            osifp = osifp_mld->link_dev[i][j];

            if (!osifp)
                continue;

            qdf_err("releasing slave %s", osifp->netdev->name);
            osif_mld_link_delete(osifp);
            osifp->mldev = NULL;
        }
    }

    status = qal_bond_destroy(mldev);
    if (status == 1) {
        return 0;
    }

    return ret;
}
#else
int
osif_destroy_mld_bond(struct net_device *mldev)
{
    return 0;
}
#endif
#endif

static bool ol_ath_is_multiband_radio(struct ieee80211com *ic)
{
    bool is_2ghz_capable, is_5ghz_capable, is_6ghz_capable;

    is_2ghz_capable = wlan_reg_is_band_present(ic->ic_pdev_obj,
                                               REG_BAND_2G);
    is_5ghz_capable = wlan_reg_is_band_present(ic->ic_pdev_obj,
                                               REG_BAND_5G);
    is_6ghz_capable = wlan_reg_is_band_present(ic->ic_pdev_obj,
                                               REG_BAND_6G);

    return ((is_2ghz_capable && is_5ghz_capable) ||
            (is_5ghz_capable && is_6ghz_capable) ||
            (is_6ghz_capable && is_2ghz_capable));
}

wlan_if_t
osifp_create_wlan_vap(struct ieee80211_clone_params *cp,
                     osif_dev *osifp,
                     struct net_device *comdev,
                     int cfg80211_create,
                     void *mld_params,
                     enum virtual_interface_type vif_type)
{
#ifdef UMAC_SUPPORT_CFG80211
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
#else
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(NULL, comdev);
#endif
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211 *)devhandle;
    struct ieee80211com *ic = &scn->sc_ic;
    bool macreq_enabled = FALSE;
    uint8_t bsta_fixed_idmask = 0xff;
    uint32_t prealloc_idmask;
    uint8_t req_mac[QDF_MAC_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    uint8_t num_vdevs;
    int id = -1;
    uint16_t waitcnt;
    uint8_t id_used = 0, non_ap_idx = 0;
    uint8_t vap_destroy;
    struct wlan_vdev_create_params params = {0};
    struct vdev_osif_priv *vdev_osifp = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    int scan_priority_mapping_base;
    wlan_if_t vap;
    wlan_if_t tempvap;
#if UMAC_SUPPORT_CFG80211
    enum nl80211_iftype nl_type;
#endif
    uint8_t vap_addr[QDF_MAC_ADDR_SIZE] = {0, 0, 0, 0, 0, 0};
    struct net_device *dev;
    struct wlan_objmgr_psoc *psoc = NULL;
    int current_bssid_ix = 0;
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                    WLAN_PDEV_F_MBSS_IE_ENABLE);
    bool is_mbssid_macaddr_exception;
#ifdef WLAN_FEATURE_11BE_MLO
    struct qdf_mac_addr *mld_addr;
    struct net_device *mld_dev = NULL;
    bool is_mlo_bridge_vap = (cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP) ? true : false;
    struct osif_mldev *osifp_mld;
#endif
    bool eht_capab = false;

    dev = osifp->netdev;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (osifp->mldev && (vif_type == VIF_TYPE_MLD_LINK_VAP)) {
        dev = osifp->mldev->mld_dev;
    }
    if (!dev && !is_mlo_bridge_vap) {
        qdf_err("netdev NULL");
        return NULL;
    }
#endif

    is_mbssid_macaddr_exception =((cp->icp_flags & IEEE80211_SMART_MONITOR_VAP) ||
                                (cp->icp_flags & IEEE80211_SPECIAL_VAP)
#ifdef WLAN_FEATURE_11BE_MLO
                                || is_mlo_bridge_vap
#endif
                                );

    macreq_enabled = ic->ic_is_macreq_enabled(ic);
    prealloc_idmask = ic->ic_get_mac_prealloc_idmask(ic);
    bsta_fixed_idmask = ic->ic_get_bsta_fixed_idmask(ic);

    if (!osif_mbssid_sanity_check(OL_ATH_SOFTC_NET80211(ic), cp)) {
        mbss_err("mbssid sanity failed - vap create failed");
        return NULL;
    }

    num_vdevs = ATH_BCBUF;
    if (macreq_enabled == TRUE) {
       if (cp->icp_vapid == VAP_ID_AUTO) {
           for(id = 0; id < ATH_MAX_PREALLOC_ID; id++)
           {
               if((prealloc_idmask & (1 << id)) == 0)
                   break;
           }
       }

       /* generate the bssid from vapid */
       if((cp->icp_vapid != VAP_ID_AUTO) &&
          (cp->icp_flags & IEEE80211_CLONE_BSSID) == 0) {
            if((cp->icp_vapid >= ATH_MAX_PREALLOC_ID)) {
                /* Support for Higher vapids (8-15) logic is added. */
                cp->icp_vapid = (cp->icp_vapid - ATH_MAX_PREALLOC_ID);
            } else {
                /* Support for Lower vapids (0-7) logic is unchanged. */
                cp->icp_vapid = (cp->icp_vapid + ATH_MAX_PREALLOC_ID);
            }

            IEEE80211_ADDR_COPY(req_mac, ic->ic_my_hwaddr);

            if (is_mbssid_enabled && !is_mbssid_macaddr_exception) {
                IEEE80211_ADDR_COPY(req_mac, ic->ic_mbss_ctx->ref_bssid);
                MBSSID_ATH_SET_VAP_BSSID(req_mac,
                          ic->ic_my_hwaddr, cp->icp_vapid, cp->icp_opmode);
            } else {
                if (ic->ic_wideband_capable && !is_mbssid_macaddr_exception &&
                    !ic->ic_cohosted_bss_support) {
                    IEEE80211_ADDR_COPY(req_mac, ic->ic_mbss_ctx->ref_bssid);
                }
                if (ic->ic_cohosted_bss_support && !is_mbssid_macaddr_exception) {
                    IEEE80211_ADDR_COPY(req_mac, ic->cohosted_ref_bssid);
                    CO_HOST_ATH_SET_VAP_BSSID(req_mac, ic->ic_myaddr, cp->icp_vapid, cp->icp_opmode);
                } else {
                    ATH_SET_VAP_BSSID(req_mac, ic->ic_my_hwaddr, cp->icp_vapid);
                }
            }

            qdf_info("Requested VAP id %d and MAC %s",
                    cp->icp_vapid, ether_sprintf(req_mac));

            if (!is_mbssid_enabled && !ic->ic_wideband_capable &&
                  !ic->ic_cohosted_bss_support) {
                if(wlan_vap_allocate_mac_addr(ic, req_mac) != -1)
                {
                   IEEE80211_ADDR_COPY(cp->icp_bssid, req_mac);
                } else {
                   qdf_info("%s: Invalid MAC requested", __func__);
                   return NULL;
                }
            } else if (is_mbssid_enabled || ic->ic_cohosted_bss_support) {
               IEEE80211_ADDR_COPY(cp->icp_bssid, req_mac);
            }
       }
    } else {
        if ((cp->icp_flags & IEEE80211_CLONE_BSSID) &&
            cp->icp_opmode != QDF_WDS_MODE) {
#ifdef WLAN_FEATURE_11BE_MLO
            /* For MLO bridge VAPs use the ids beyond ATH_BCBUF */
            if (is_mlo_bridge_vap) {
                num_vdevs = ATH_BCBUF + MLO_MAX_BRIDGE_LINKS_PER_RADIO;
                for (id = 0; id < MLO_MAX_BRIDGE_LINKS_PER_RADIO; id++) {
                    if((prealloc_idmask & (1 << (id + ATH_BCBUF))) == 0) {
                        id += ATH_BCBUF;
                        break;
                    }
                }
            } else
#endif
            {
                for(id = 0; id < ATH_BCBUF; id++) {
                    if((prealloc_idmask & (1 << id)) == 0)
                        break;
                }
            }
        }

#if ATH_SUPPORT_WRAP
       if (cp->icp_flags & IEEE80211_CLONE_MACADDR) {
            /* set the flag as non-MAIN sta, since MAC address need not be
               derived */
           if ((cp->icp_opmode == QDF_STA_MODE) &&
                    (IEEE80211_ADDR_IS_VALID(cp->icp_bssid))) {
              cp->icp_flags |= IEEE80211_WRAP_NON_MAIN_STA;
           }
           else {
              num_vdevs = ic->ic_get_qwrap_num_vdevs(ic);
              for (id = 0; id < num_vdevs; id++) {
                  /* get the first available slot */
                  if ((prealloc_idmask & (1 << id)) == 0)
                      break;
              }
           }
       } else
#endif
       if ((cp->icp_flags & IEEE80211_CLONE_BSSID) == 0) {
           /* do not clone use the one passed in */
           qdf_info("No cloning");
           /*
            * Added proper logic to get vap id and bssid value.
            * when user pass BSSID MAC value through commandline using -bssid option, The
            * exsiting logic won't work to get proper vap id to create VAP
            */
       }
    }

    /* With MBSS/wideband enabled, BSSID is assigned from ref BSSID
     * of Tx VAP and id of sc_prealloc_idmask is not linked
     * to BSSID anymore, hence skip checking slot id
     * for MBSS/WB case. If user tries to create more than supported
     * VAP for MBSS/WB, obj mgr pdev max vap check will fail
     * and prevent that extra VAP creation attempt
     */
    if (!is_mbssid_enabled && !ic->ic_wideband_capable &&
        !ic->ic_cohosted_bss_support && (id == num_vdevs)) {
        qdf_info("%s: no dynamic vaps free", __func__);
        return NULL;
    }

    if (IEEE80211_IS_MULTICAST(ic->ic_myaddr))
    {
        qdf_info("Multicast/Broadcast- Invalid MAC requested ");
        return NULL;
    }
    /* Allocate BSSID for valid vdev id */
    if(id != -1) {
       IEEE80211_ADDR_COPY(vap_addr, ic->ic_myaddr);

       if (is_mbssid_enabled && !is_mbssid_macaddr_exception && (cp->icp_opmode == QDF_SAP_MODE)) {
           IEEE80211_ADDR_COPY(vap_addr, ic->ic_mbss_ctx->ref_bssid);
           MBSSID_ATH_SET_VAP_BSSID(vap_addr, ic->ic_myaddr, id, cp->icp_opmode);
       }
       else if ((bsta_fixed_idmask >= 0) && (bsta_fixed_idmask < num_vdevs) &&
                (cp->icp_opmode == QDF_STA_MODE)) {
           /* Multi AP requires STA mode to have consistent mac address for reboots or wifi load/unload */
           ATH_SET_VAP_BSSID(vap_addr, ic->ic_myaddr, bsta_fixed_idmask);
       }
       else {
           if (ic->ic_wideband_capable && !is_mbssid_macaddr_exception &&
               (cp->icp_opmode == QDF_SAP_MODE) && !ic->ic_cohosted_bss_support) {
               IEEE80211_ADDR_COPY(vap_addr, ic->ic_mbss_ctx->ref_bssid);
           }

	   if ((is_mbssid_macaddr_exception || (cp->icp_opmode != QDF_SAP_MODE))
               && IS_NON_AP_MAC_BMAP_REQ_CHECKS_SET(ic)) {
                   non_ap_idx = qdf_ffz(ic->ic_non_ap_bssid_index_bmap);
                   id = ATH_BCBUF + non_ap_idx;
           }

           if (ic->ic_cohosted_bss_support && !is_mbssid_macaddr_exception && (cp->icp_opmode == QDF_SAP_MODE)) {
               IEEE80211_ADDR_COPY(vap_addr, ic->cohosted_ref_bssid);
               CO_HOST_ATH_SET_VAP_BSSID(vap_addr, ic->ic_myaddr, id, cp->icp_opmode);
           } else {
               ATH_SET_VAP_BSSID(vap_addr, ic->ic_myaddr, id);
           }
       }
       IEEE80211_ADDR_COPY(cp->icp_bssid, vap_addr);

       if ((cp->icp_opmode != QDF_SAP_MODE) ||
           is_mbssid_macaddr_exception ||
           (!is_mbssid_enabled && !ic->ic_wideband_capable &&
            !ic->ic_cohosted_bss_support)) {
           ic->ic_set_mac_prealloc_id(ic, id, 1);
           id_used = 1;
       }

       if ((is_mbssid_macaddr_exception || (cp->icp_opmode != QDF_SAP_MODE))
	   && IS_NON_AP_MAC_BMAP_REQ_CHECKS_SET(ic)) {
           id = ATH_BCBUF + non_ap_idx;
       }

    }

    psoc = wlan_pdev_get_psoc(devhandle->ic_pdev_obj);
    switch (cp->icp_opmode) {
#ifdef ATH_SUPPORT_P2P
    case QDF_P2P_GO_MODE:
        qdf_print(" IEEE80211_M_P2P_GO created ");
        osifp->p2p_go_handle = wlan_p2p_GO_create((wlan_p2p_t) devhandle, NULL);
        if (osifp->p2p_go_handle == NULL) {
            return NULL;
        }
        vap=wlan_p2p_GO_get_vap_handle(osifp->p2p_go_handle);
        /* enable UAPSD by default for GO Vap */
        /*  wlan_set_param(vap, IEEE80211_FEATURE_UAPSD, 0x1); */
        break;

    case QDF_P2P_CLIENT_MODE:
        qdf_print(" IEEE80211_M_P2P_CLIENT created ");
        osifp->p2p_client_handle = wlan_p2p_client_create((wlan_p2p_t) devhandle, NULL);
        if (osifp->p2p_client_handle == NULL) {
            return NULL;
        }
        vap=wlan_p2p_client_get_vap_handle(osifp->p2p_client_handle);
        break;

    case QDF_P2P_DEVICE_MODE:
        qdf_print(" IEEE80211_M_P2P_DEVICE created ");
        osifp->p2p_handle = wlan_p2p_create(devhandle, NULL);
        if (osifp->p2p_handle == NULL) {
            return NULL;
        }
        vap=wlan_p2p_get_vap_handle(osifp->p2p_handle);
        break;
#endif
    default:
        if (cp->icp_opmode == QDF_STA_MODE) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE;
#if UMAC_SUPPORT_CFG80211
            nl_type = NL80211_IFTYPE_STATION;
#endif
        }
        else if (cp->icp_opmode == QDF_IBSS_MODE) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_IBSS_BASE;
#if UMAC_SUPPORT_CFG80211
            nl_type = NL80211_IFTYPE_ADHOC;
#endif
        }
        else if (cp->icp_opmode == QDF_SAP_MODE) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_AP_BASE;
            if (cp->icp_flags & IEEE80211_SPECIAL_VAP){
                dev->type = ARPHRD_IEEE80211_RADIOTAP;
            }
#if UMAC_SUPPORT_CFG80211
            nl_type = NL80211_IFTYPE_AP;
#endif
        }
        else if (cp->icp_opmode == QDF_MONITOR_MODE) {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE;
            dev->type = ARPHRD_IEEE80211_RADIOTAP;
#if UMAC_SUPPORT_CFG80211
            nl_type = NL80211_IFTYPE_MONITOR;
#endif
        }
        else {
            scan_priority_mapping_base = DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_BASE;
#if UMAC_SUPPORT_CFG80211
            nl_type = NL80211_IFTYPE_STATION;
#endif
        }

        params.opmode = cp->icp_opmode;
        IEEE80211_ADDR_COPY(params.macaddr, cp->icp_bssid);
        params.flags = cp->icp_flags;
        IEEE80211_ADDR_COPY(params.mataddr, cp->icp_mataddr);
        params.size_vdev_priv = sizeof(struct vdev_osif_priv);
        params.legacy_osif = osifp;

#ifdef WLAN_FEATURE_11BE_MLO
        if (mld_params && (vif_type != VIF_TYPE_LEGACY)) {
            if (((struct ieee80211_mld_create_params *)mld_params)->mld_ifname_valid) {
                mld_dev = osif_find_mld_dev_by_ifname(
                        ((struct ieee80211_mld_create_params *)mld_params)->mld_ifname);
                if (mld_dev) {
                    osifp_mld = osif_mld_get_mldev(mld_dev);
                    if (osifp_mld->derived_mld_addr) {
                        /* Update ml netdev mac addr to Link MAC address */
                        IEEE80211_ADDR_COPY(mld_dev->dev_addr, cp->icp_bssid);
                        /* Initilize mldev mld_mac_addr to Link MAC address */
                        IEEE80211_ADDR_COPY(osifp->mldev->mld_mac_addr, cp->icp_bssid);
                        osifp_mld->derived_mld_addr = 0;
                        mlo_info ("mld address copied from link id");
                    }
                    if (IEEE80211_ADDR_IS_VALID(mld_dev->dev_addr)) {
                        IEEE80211_ADDR_COPY(params.mldaddr, mld_dev->dev_addr);
                    } else {
                        mlo_err("Failed to copy mld addr from link vap: %s",
                            cp->icp_name);
                        return NULL;
                    }
                } else {
                    mlo_err("For vap: %s, mld_dev not found by name %s",
                            cp->icp_name,
                            ((struct ieee80211_mld_create_params *)mld_params)->mld_ifname);
                    return NULL;
                }
            } else {
                IEEE80211_ADDR_COPY(params.mldaddr,
                    ((struct ieee80211_mld_create_params *)mld_params)->mld_macaddr);
            }
        }
#endif

        TAILQ_FOREACH(tempvap, &ic->ic_vaps, iv_next) {
            if(!qdf_mem_cmp(tempvap->iv_myaddr, cp->icp_bssid, QDF_MAC_ADDR_SIZE)) {
                qdf_print("VAP Create Error. Vap macaddr %pM matches with already existing vap %s with ID %d", cp->icp_bssid, tempvap->iv_netdev_name, tempvap->iv_unit);
                WARN_ON(1);
                return NULL;
            }
        }
        qdf_info("VDEV Create %pM", cp->icp_bssid);
        psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
        if (psoc) {
            wlan_psoc_mlme_get_11be_capab(psoc, &eht_capab);
        }
        /* Handling enable/disable mlo_mbssid in lower band */
        if (!ic->ic_enable_mlo_mbssid_lb && eht_capab) {
            qdf_info("mlo_mbssid disabled in lower band radios\n");
#ifdef WLAN_FEATURE_11BE_MLO
        if (IEEE80211_ADDR_IS_VALID(params.mldaddr)) {
            if (!ol_ath_is_multiband_radio(ic)) {
                qdf_info("Not a Multiband radio. Continue with MBSSID check\n");
                if (!IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)
                    && is_mbssid_enabled) {
                    qdf_err("VAP Create Error. MBSSID enabled in lower band with MLD mld_macaddr: %pM",
                            params.mldaddr);
                    return NULL;
                }
            }
        }
#endif
        }

        vdev = wlan_objmgr_vdev_obj_create(devhandle->ic_pdev_obj, &params);
        if(vdev != NULL) {

            vdev_osifp = wlan_vdev_get_ospriv(vdev);

#if UMAC_SUPPORT_CFG80211
           if (cfg80211_create) {
               vdev_osifp->wdev = &(osifp->iv_wdev);
           }
           else {
              vdev_osifp->wdev = NULL;
           }
#endif

            waitcnt = 0;
            /* In sta mode when old bss node is freed, it is posted as an event to
             * connection state machine, which initiates creation of new bss peer.
             * At this point, unless the peer's destroy handler has been executed and
             * max peers per vap are decremented already, the creation of new bss peer
             * will fail. To handle this syncronization the max peers per vap are
             * increased from 2 to 3
             */
            if (cp->icp_opmode == QDF_STA_MODE) {
                wlan_vdev_set_max_peer_count(vdev, 3);
            }
            while((!((vdev->obj_state == WLAN_OBJ_STATE_CREATED) ||
                   (vdev->obj_state == WLAN_OBJ_STATE_CREATION_FAILED)) &&
                     (waitcnt < WLAN_VDEV_CREATE_TIMEOUT_CNT))) {
                qdf_sleep_us(WLAN_VDEV_CREATE_TIMEOUT*1000);
                waitcnt++;
            }
            if(vdev->obj_state == WLAN_OBJ_STATE_CREATION_FAILED) {
                qdf_print("VDEV (partial creation) failed ");
                /* If peer count is non-zero, means MLME has created peer,
                 * MLME should free the vap after reseeting the state
                 */
                vap = wlan_vdev_get_mlme_ext_obj(vdev);
                if (vap != NULL) {
                   wlan_vap_delete(vap);
                }
                else {
                   wlan_objmgr_vdev_obj_delete(vdev);
                }
                vdev = NULL;
            }
        }
        else {
            qdf_print("VDEV (creation) failed ");
        }

        break;
    }


    vap_destroy = 0;
    if (vdev == NULL) {
       vap_destroy = 1;
    }
    else {
       vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);

       if (vap) {
           if (cp->icp_opmode == QDF_SAP_MODE &&
               (QDF_IS_STATUS_ERROR(osifp_create_wlan_vap_bss_status(ic, vap)))) {
               goto fail;
           }
           /*
            * Update vdev chainmasks.
            *
            * Rather than obtain a lock for the vdev again at another place in
            * this function, we set the vdev's chainmasks right here while we
            * hold the lock.
            * But we need to do so only if vap is non-NULL.
            */
           wlan_vdev_mlme_set_txchainmask(vdev, devhandle->ic_tx_chainmask);
           wlan_vdev_mlme_set_rxchainmask(vdev, devhandle->ic_rx_chainmask);
#if UNIFIED_SMARTANTENNA
           wlan_sa_api_start(vap->iv_ic->ic_pdev_obj, vdev, SMART_ANT_STA_NOT_CONNECTED | SMART_ANT_NEW_CONFIGURATION);
#endif
#ifdef WLAN_FEATURE_11BE_MLO
           wlan_vdev_mlme_aid_mgr_max_aid_set(vap->vdev_obj, vap->iv_max_aid);
#endif

       }
       if(vap == NULL) {
           vap_destroy = 1;
       }
    }
    if(vap_destroy) {
      if(id_used == 1) {
        /* Reset the id slot */
        ic->ic_set_mac_prealloc_id(ic, id, 0);
      }
      return NULL;
    }

#if UMAC_SUPPORT_CFG80211
    /*
       1) Each netdevice have ieee80211_ptr which is of type wdev.
       2) Driver internal data struct ic and vap (for each  netdev) should contain one wdev
            a. net_dev->ieee80211_ptr = &ic->wdev;
       3) Internal wdev need to initilize with
           a. ic/vap->wdev.wiphy = registered cfg80211dev.
           b. ic/vap->wdev.netdev = our netdev.

    */
    if (cfg80211_create) {
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        /* For Link VAP do not attach netdev to wireless stack */
        if (vif_type != VIF_TYPE_MLD_LINK_VAP) {
#endif
            osifp->iv_wdev.wiphy = ic->ic_wdev.wiphy;
            osifp->iv_wdev.netdev = dev;
            osifp->iv_wdev.iftype = nl_type;
#ifdef WLAN_FEATURE_11BE_MLO
            mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
            /* The MLD address is available in vdev since it is part of an MLD,
               copy the same into osifp  */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)
            if (!qdf_is_macaddr_zero(mld_addr)) {
                IEEE80211_ADDR_COPY(&osifp->iv_wdev.mld_address, mld_addr->bytes);
            }
#endif
#endif
            dev->ieee80211_ptr = &osifp->iv_wdev;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        } else {
                qdf_err("Do not attach netdev for link vap..........");
        }
#endif

        /* Other below things are required for link vaps as well */
        /* Create wokrQ cfg80211 events which nneed to be called in process context */
        qdf_create_work(0, &vap->cfg80211_channel_notify_workq,
                ieee80211_cfg80211_schedule_channel_notify, vap);
        qdf_create_work(0, &vap->cfg80211_channel_started_notify_workq,
                ieee80211_cfg80211_schedule_channel_started_notify, vap);

        if (cp->icp_opmode == QDF_STA_MODE)
                qdf_create_work(0, &vap->cfg80211_ft_supp_workq,
                        ieee80211_cfg80211_post_ft_workq, vap);

        qdf_create_work(0, &vap->acs_bk_cmpl_work,
                acs_bk_scan_cmpl_handler, vap);
    }
    /*TODO it is supposed to be done as part of creation, but this can be updated seperately */
    vap->iv_scan_priority_base = scan_priority_mapping_base;
    vap->iv_cfg80211_create = cfg80211_create;

#endif

    /*
     * For wideband channel operation, VAP creation in co-hosted mode
     * requires a profile index assigned to the VAP in order to account
     * for the BSSIDs during VAP delete if EMA operation is not enabled.
     * If EMA operation is enabled, this profile index is reset during
     * the initial MBSSID setup routine.
     */
    if (ic->ic_wideband_capable && !is_mbssid_enabled && current_bssid_ix && !is_mbssid_macaddr_exception)
        vap->vdev_mlme->mgmt.mbss_11ax.profile_idx = current_bssid_ix;

    osifp->os_if = vap;
    osifp->ctrl_vdev = vdev;            /* back pointer to vdev object*/
#ifdef WLAN_FEATURE_11BE_MLO
    if (!is_mlo_bridge_vap || (is_mlo_bridge_vap && dev))
#endif
        vap->iv_max_mtu_size = dev->mtu;

    /* Store valid VAP ID if one passed from the user-sapce */
    if (macreq_enabled) {
        vap->iv_user_vapid = (cp->icp_vapid) ? (cp->icp_vapid != VAP_ID_AUTO) : VAP_USER_VAPID_INVALID;
    } else {
       if ((is_mbssid_macaddr_exception || (cp->icp_opmode != QDF_SAP_MODE))
            && IS_NON_AP_MAC_BMAP_REQ_CHECKS_SET(ic)) {
            if (qdf_unlikely(non_ap_idx == IEEE80211_INVALID_NON_AP_BSSIDX)) {
                qdf_info("Non AP index bitmap full VAP Id:%d",vap->iv_user_vapid);
                return NULL;
       }
       vap->iv_user_vapid = non_ap_idx;
       qdf_set_bit(vap->iv_user_vapid,
                   (unsigned long *) &ic->ic_non_ap_bssid_index_bmap);
       }
    }

#if defined(WLAN_FEATURE_11BE_MLO) && !defined(ENABLE_CFG80211_BACKPORTS_MLO)
    wlan_mlme_mlo_link_rej_init(vdev, MLO_LINK_REJ_RANGE);
#endif

    return vap;

fail:
    osifp->os_if = vap;
    osifp->ctrl_vdev = vdev;
    osifp->netdev = dev;
    wlan_vap_set_registered_handle(vap,(os_if_t)osifp);
    osif_delete_vap(osifp, 0);
    osif_delete_vap_wait_and_free(osifp, 0);
    return NULL;
}

void osif_netdev_extended_init(struct net_device *comdev,
                               osif_dev *osifp,
                               bool recover)
{
#ifdef UMAC_SUPPORT_CFG80211
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
#else
    wlan_dev_t devhandle = (wlan_dev_t)get_radio_device_context(NULL, comdev);
#endif
    wlan_if_t vap = osifp->os_if;
    u_int8_t *vap_macaddr;
    u_int32_t target_type;
    struct net_device *dev = osifp->netdev;
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211 *)devhandle;
    struct ieee80211com *ic = &scn->sc_ic;
    osdev_t os_handle = NULL;

    os_handle = ic->ic_osdev;
    vap_macaddr = wlan_vap_get_macaddr(vap);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
    qdf_net_update_net_device_dev_addr(dev, (const void *)vap_macaddr,
                                       IEEE80211_ADDR_LEN);
#else
    IEEE80211_ADDR_COPY(dev->dev_addr, vap_macaddr);
#endif

#ifdef QCA_PARTNER_PLATFORM
    osif_pltfrm_record_macinfor(osifp->os_unit,dev->dev_addr);
#endif

    /* Note: Set the dev features flag to comdev features
     * before qdf_net_vlan_attach - otherwise
     * VLAN flag settings in qdf_net_vlan_attach gets
     * overwritten
     */
    dev->features = comdev->features;

    /* enable TSO/LRO only of HW Supports */
    if(osifp->osif_is_mode_offload) {
        if (ic->ic_offload_tx_csum_support) {
            osif_dev_set_offload_tx_csum(dev);
        }
        if (ic->ic_sg_support) {
            osif_dev_set_sg(dev);
        }
        if (ic->ic_sg_support) {
            osif_dev_set_sg(dev);
        }
        if (ic->ic_tso_support) {
            osif_dev_set_tso(dev);
        }
        QDF_TRACE(QDF_MODULE_ID_DP_INIT, QDF_TRACE_LEVEL_INFO_HIGH,
                  "TX Checksum:%d|SG:%d|TSO:%d|LRO:%d",
                   ic->ic_offload_tx_csum_support, ic->ic_sg_support,
                   ic->ic_tso_support, ic->ic_lro_support);

    }

// fixme where is this done in the new scheme?    dev->priv = vap;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#ifdef WLAN_FEATURE_FASTPATH
    if (osifp->osif_is_mode_offload) {
        target_type = ic->ic_get_tgt_type(ic);
        if ((target_type == TARGET_TYPE_QCA8074) ||
            (target_type == TARGET_TYPE_QCA8074V2) ||
            (target_type == TARGET_TYPE_QCA9574) ||
            (target_type == TARGET_TYPE_QCA6018) ||
            (target_type == TARGET_TYPE_QCA5332) ||
            (target_type == TARGET_TYPE_QCA5018) ||
            (target_type == TARGET_TYPE_QCN6122) ||
            (target_type == TARGET_TYPE_QCN9160) ||
            (target_type == TARGET_TYPE_QCN9224) ||
            (target_type == TARGET_TYPE_QCN9000) ||
            (target_type == TARGET_TYPE_QCN6432)) {
            dev->netdev_ops = &osif_11ac_wifi3_fp_dev_ops;
        } else {
            dev->netdev_ops = &osif_11ac_fp_dev_ops;
        }
    } else
#endif /* WLAN_FEATURE_FASTPATH*/
    {
        osif_dev_ops.ndo_start_xmit = os_handle->vap_hardstart;
        dev->netdev_ops = &osif_dev_ops;
    }

    /* Enable ndo_select_queue only when multiq is supported */
#ifdef WLAN_FEATURE_FASTPATH
    if (ic->multiq_support_enabled) {
        osif_11ac_fp_dev_ops.ndo_select_queue = osif_select_queue;
        osif_11ac_wifi3_fp_dev_ops.ndo_select_queue = osif_select_queue;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        osif_nss_wifiol_dev_ops.ndo_select_queue = osif_select_queue;
#endif
#ifdef QCA_SUPPORT_WDS_EXTENDED
	osif_wds_ext_dev_ops.ndo_select_queue = osif_select_queue;
#endif
    }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    ((struct net_device_ops *) (dev->netdev_ops))->ndo_siocwandev = ieee80211_ioctl_wandev;
    ((struct net_device_ops *) (dev->netdev_ops))->ndo_siocdevprivate = ieee80211_ioctl_devprivate;
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */
    ((struct net_device_ops *) (dev->netdev_ops))->ndo_do_ioctl = ieee80211_ioctl;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0) */

#else /* LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30) */
    dev->get_stats = osif_getstats;
    dev->open = osif_vap_open;
    dev->stop = osif_vap_stop;

    dev->hard_start_xmit = os_handle->vap_hardstart;
    dev->set_multicast_list = osif_set_multicast_list;
#if 0
    dev->set_mac_address = ieee80211_set_mac_address;
#endif
    dev->change_mtu = osif_change_mtu;
    dev->set_mac_address = osif_change_mac_addr;
#endif
    dev->tx_queue_len = 0;          /* NB: bypass queueing */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
    dev->priv_flags |= IFF_NO_QUEUE;
#endif
    dev->needed_headroom = comdev->needed_headroom;
    dev->hard_header_len = comdev->hard_header_len;

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if ( osifp->osif_is_mode_offload && ic->nss_vops && osifp->nss_wifiol_ctx ) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
        dev->netdev_ops = &osif_nss_wifiol_dev_ops;
#else
        if(os_handle->vap_hardstart)
            dev->hard_start_xmit = os_handle->vap_hardstart;
#endif
        QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "WLAN-NSS: VAP NSS ops initialized ");
    } else
#endif
    {
#if QCA_NSS_PLATFORM
#ifndef QCA_SINGLE_WIFI_3_0
        if (!(of_machine_is_compatible("qcom,ipq40xx"))) {
            osif_pltfrm_create_vap(osifp);
        }
#endif
#elif QCA_PPE_VP
        if (!recover)
            osif_pltfrm_create_vap(osifp);
#ifdef WLAN_SUPPORT_PPEDS
        /*
         * In case of FW recovery in DS mode, need not create a new
         * VP and setup the DS profile again, instead setup the DS VP
         * with the exsisting profile.
         */
        osif_pltfrm_setup_ds_vap_fw_recovery(osifp, recover);
#endif
#elif defined(QCA_PARTNER_PLATFORM )
        osif_pltfrm_create_vap(osifp);
#endif
    }

    /*
     * qdf_net_vlan_attach() sets the vlan as a active feature in dev->features
     * and assigns vlan functions in dev->netdev_ops
     *
     * Should be called after final dev->netdev_ops is assigned to keep
     * dev->features flag and func pointers in dev->netdev_ops in a sync
     *
     */
#if ATH_SUPPORT_VLAN
    if (!ic->ic_no_vlan) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
    	qdf_net_vlan_attach(dev,(struct net_device_ops *) dev->netdev_ops);
#else
    	qdf_net_vlan_attach(dev);
#endif
    }
#endif

    /*
     * The caller is assumed to allocate the device with
     * alloc_etherdev or similar so we arrange for the
     * space to be reclaimed accordingly.
     */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    /* in 2.4 things are done differently... */
    dev->features |= NETIF_F_DYNALLOC;
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,9))
    dev->needs_free_netdev = true;
#else
    dev->destructor = free_netdev;
#endif
#endif

    os_if_media_init(osifp);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
    dev->do_ioctl = ieee80211_ioctl;
#endif

#if UMAC_SUPPORT_WEXT
    ieee80211_ioctl_vattach(dev);
#endif
}

int
osif_create_vap_complete(struct net_device *comdev,
                         struct ieee80211_clone_params *cp,
                         osif_dev *osifp,
                         bool recover)
{
    wlan_dev_t devhandle = ath_netdev_priv(comdev);
    wlan_if_t vap = osifp->os_if;
#if UMAC_PER_PACKET_DEBUG || ATH_SUPPORT_WRAP || QCA_PARTNER_DIRECTLINK_TX
    struct net_device *dev = osifp->netdev;
#endif
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211 *)devhandle;
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211vap *parent_vap;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    struct wlan_objmgr_psoc *psoc = NULL;
#endif /* QCA_SUPPORT_WDS_EXTENDED */
#if ATH_SUPPORT_WAPI
    /* for WAPI support, ETHERTYPE_WAPI should be added to the privacy filter */
    ieee80211_privacy_exemption privacy_filter[2];
#else
    ieee80211_privacy_exemption privacy_filter;
#endif
#if WLAN_SUPPORT_GREEN_AP
    struct wlan_objmgr_pdev *pdev = wlan_vap_get_pdev(vap);
#endif
    int error;
    QDF_STATUS status;
    osdev_t os_handle = NULL;

    os_handle = ic->ic_osdev;
    memcpy(&vap->cp, cp, sizeof(vap->cp));

    INIT_LIST_HEAD(&osifp->pending_rx_frames);
    spin_lock_init(&osifp->list_lock);

    osifp->os_handle = os_handle;
    osifp->os_devhandle = devhandle;
    osifp->os_comdev = comdev;
    osifp->os_opmode = wlan_vap_get_opmode(vap);

#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    osifp->os_periodic_scan_period = OSIF_PERIODICSCAN_DEF_PERIOD;
    OS_INIT_TIMER(os_handle, &(osifp->os_periodic_scan_timer), periodic_scan_timer_handler,
            (void *)osifp, QDF_TIMER_TYPE_WAKE_APPS);
#endif
    osifp->wlan_vendor_fwd_mgmt_mask = OSIF_VENDOR_FWD_MGMT_MASK_DEFAULT;
#ifdef ATH_SUPPORT_WAPI
    osifp->os_wapi_rekey_period = OSIF_WAPI_REKEY_TIMEOUT;
    OS_INIT_TIMER(os_handle, &(osifp->os_wapi_rekey_timer), osif_wapi_rekey_timeout,
            (void *)osifp, QDF_TIMER_TYPE_WAKE_APPS);
#endif
    osif_vap_setup(vap, osifp, wlan_vap_get_opmode(vap));
#if ATH_NON_BEACON_AP
    if( (wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) &&
            (cp->icp_flags & IEEE80211_NON_BEACON_AP) ){
        IEEE80211_VAP_NON_BEACON_ENABLE(vap);
    }
#endif

    if (vap->vif_type == VIF_TYPE_MLD_LINK_VAP) {
#if QCA_PPE_VP
        if (!recover)
            osif_pltfrm_create_vap(osifp);
#ifdef WLAN_SUPPORT_PPEDS
        /*
         * In case of FW recovery in DS mode, need not create a new
         * VP and setup the DS profile again, instead setup the DS VP
         * with the exsisting profile.
         */
        osif_pltfrm_setup_ds_vap_fw_recovery(osifp, recover);
#endif
#endif
    } else {
        osif_netdev_extended_init(comdev, osifp, recover);
    }

    /* set the vlan as a active feature for mld netdev */
#if ATH_SUPPORT_VLAN
    if (!ic->ic_no_vlan) {
#ifdef WLAN_FEATURE_11BE_MLO
        if (osifp->mldev && osifp->mldev->mld_dev)
            osif_mldev_vlan_attach(osifp->mldev->mld_dev);
#endif
    }
#endif

    ic_sta_vap_update(devhandle, vap);
    ic_mon_vap_update(devhandle, vap);

   if (ic->ic_sta_vap) {
        /*OBSS Interference Event is disabled for Repeater mode*/
        OL_ATH_DCS_DISABLE(scn->scn_dcs.dcs_enable, CAP_DCS_OBSSIM);
    }

#ifdef WLAN_FEATURE_FASTPATH
    if (osifp->osif_is_mode_offload) {
        spin_lock_init(&osifp->nbuf_arr_lock);
        osifp->nbuf_arr = (qdf_nbuf_t *)qdf_mem_malloc(MSDUS_ARRAY_SIZE * sizeof(qdf_nbuf_t));
        if (!osifp->nbuf_arr) {
            return -EINVAL;
        }
    }
#endif

    if (cp->icp_opmode == QDF_P2P_DEVICE_MODE)
        wlan_set_desired_phymode(vap, IEEE80211_MODE_11G);
    else
        wlan_set_desired_phymode(vap, IEEE80211_MODE_AUTO);

    /*
     * If a parent VAP exists, then copy the desired mode and if present, copy
     * the desired channel as well.
     */
    parent_vap = TAILQ_FIRST(&ic->ic_vaps);
    if (parent_vap != NULL && parent_vap != vap) {
        wlan_set_desired_phymode(vap, parent_vap->iv_des_mode);

        if (!IEEE80211_ARE_CHANS_EQUAL(vap->iv_des_chan[vap->iv_des_mode],
                     parent_vap->iv_des_chan[parent_vap->iv_des_mode])) {
            qdf_debug("Updating VAP%d channel for mode %d as per parent VAP%d",
                     vap->iv_unit, vap->iv_des_mode, parent_vap->iv_unit);
            *vap->iv_des_chan[vap->iv_des_mode] =
                       *parent_vap->iv_des_chan[parent_vap->iv_des_mode];
        }
    }

#if ATH_SUPPORT_WAPI
    /* always setup a privacy filter to allow receiving unencrypted EAPOL frame and/or WAI frame for WAPI*/
    privacy_filter[0].ether_type = ETHERTYPE_PAE;
    privacy_filter[0].packet_type = IEEE80211_PRIVACY_FILTER_PACKET_BOTH;
    privacy_filter[0].filter_type = IEEE80211_PRIVACY_FILTER_KEY_UNAVAILABLE;
    privacy_filter[1].ether_type = ETHERTYPE_WAI;
    privacy_filter[1].packet_type = IEEE80211_PRIVACY_FILTER_PACKET_BOTH;
    privacy_filter[1].filter_type =IEEE80211_PRIVACY_FILTER_ALLWAYS;
    wlan_set_privacy_filters(vap,privacy_filter,2);
#else
    /* always setup a privacy filter to allow receiving unencrypted EAPOL frame */
    privacy_filter.ether_type = ETHERTYPE_PAE;
    privacy_filter.packet_type = IEEE80211_PRIVACY_FILTER_PACKET_BOTH;
    privacy_filter.filter_type = IEEE80211_PRIVACY_FILTER_KEY_UNAVAILABLE;
    wlan_set_privacy_filters(vap,&privacy_filter,1);
#endif /* ATH_SUPPORT_WAPI */

#if UMAC_SUPPORT_WEXT
    if (!(vap->cp.icp_flags & IEEE80211_MLO_BRIDGE_VAP))
        ieee80211_proc_vattach(vap);
#endif

    error = wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE,1 << WLAN_CRYPTO_AUTH_OPEN);

#if UMAC_PER_PACKET_DEBUG
    if (strlcpy(vap->iv_proc_fname,"pppdata",PROC_FNAME_SIZE) >= PROC_FNAME_SIZE) {
        qdf_print("source too long");
        return NULL;
    }
    strcat(vap->iv_proc_fname, dev->name);
    vap->iv_proc_entry = create_proc_entry(vap->iv_proc_fname, 0644, vap->iv_proc_root);
    if(vap->iv_proc_entry == NULL) {
        qdf_print(" iv null ");
    } else {
        vap->iv_proc_entry->data = (void *)vap;
        vap->iv_proc_entry->write_proc = osif_proc_pppdata_write;
    }
#endif
    /* register scan event handler */
    osifp->scan_requestor = ucfg_scan_register_requester(wlan_vap_get_psoc(vap),
            (u_int8_t*)"osif_umac", NULL, NULL);
    if (!osifp->scan_requestor) {
        scan_err("unable to allocate requester");
    }
    status = ucfg_scan_register_event_handler(wlan_vap_get_pdev(vap),
            &osif_scan_evhandler, (void *)osifp);
    if (status) {
        scan_err("unable to register scan event handler vap: 0x%pK status: %d", vap, status);
    }
#if ATH_SUPPORT_WRAP
    wlan_rptr_vdev_create_complete(vap->vdev_obj,dev);
#endif

#if WLAN_SUPPORT_GREEN_AP
    if (vap->iv_opmode == IEEE80211_M_STA) {
        /* If it is a STAVAP, stop the Green AP feature */
         wlan_green_ap_stop(pdev);
     }
#endif

#if QCA_PARTNER_DIRECTLINK_TX
    osifp->is_directlink = 0;
    if (osifp->osif_is_mode_offload) {
        if (ol_is_directlink(wlan_vdev_get_id(osifp->ctrl_vdev))) {
            osifp->is_directlink = 1;
            if (wlan_pltfrm_directlink_enable(osifp)) {
                qdf_print(KERN_ERR "%s: direct link unable to register device", dev->name);
            }
        }
    }
#endif /* QCA_PARTNER_DIRECTLINK_TX */

#if UMAC_SUPPORT_ACS

    /* If user has blocked channels from being selected from ACS,
     * the said channels are loaded into an ic structure as part
     * ol_ath_pdev_attach(). To ensure that the channels remain
     * blocked even through the SM, we are sending in the
     * blocked channels during VAP up.
     */
    if (ic->ic_pri20_cfg_blockchanlist.n_freq) {
        wlan_acs_block_channel_list(vap,
                            ic->ic_pri20_cfg_blockchanlist.freq,
                            ic->ic_pri20_cfg_blockchanlist.n_freq);
    }
#endif /* UMAC_SUPPORT_ACS */

#ifdef QCA_SUPPORT_WDS_EXTENDED
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (psoc && wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_WDS_EXTENDED)) {
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_vdev_set_cfg((osif_dev *)vap->iv_ifp,
                                                   OSIF_NSS_WIFI_VDEV_WDS_BACKHAUL_CFG);
        }
#endif
    }
#endif /* QCA_SUPPORT_WDS_EXTENDED */

    os_if_cm_init(osifp);
    return 0;
}

int
osif_create_vap_netdev_register(osif_dev *osifp)
{
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);
    wlan_if_t vap = osifp->os_if;

    if (!dev)
        return -EINVAL;

    /* NB: rtnl is held on entry so don't use register_netdev */
    if (wlan_cfg80211_register_netdevice(dev)) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: unable to register device\n", dev->name);

        osif_vap_stop_wrapper(osifp);

        return -EINVAL;
    }

    /* Mark the VAP as registered with Kernel */
    ieee80211_vap_registered_set(vap);

    return 0;
}

int
osif_create_vap_recover(struct net_device *comdev,
                          struct ieee80211vap *vap,
                          osdev_t os_handle,
#ifdef WLAN_FEATURE_11BE_MLO
                          struct ieee80211_mld_create_params *params,
#endif
                          uint8_t is_mbss_tx_vdev)
{
    osif_dev  *osifp;
    int ret;
    struct ieee80211_clone_params cp;
#ifdef UMAC_SUPPORT_CFG80211
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211*)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
    u_int8_t cfg80211_create;
#else
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)get_radio_device_context(NULL, comdev);
#endif
    struct ieee80211com *ic = &scn->sc_ic;
    struct net_device *dev;
    struct qdf_vfs_attr *attr_grp;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
    struct wlan_vdev_state_event event;
    QDF_TRACE_LEVEL level;
    memcpy(&cp, &vap->cp, sizeof(cp));
#if UMAC_SUPPORT_CFG80211
    cfg80211_create = vap->iv_cfg80211_create;
#endif

    osifp = osif_recover_vap_netdev(vap);
    if (osifp == NULL) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: netdev failed\n", __FUNCTION__);
        goto fail0;
    }
    dev = osifp->netdev;
#if UMAC_SUPPORT_CFG80211
#ifdef WLAN_FEATURE_11BE_MLO
    vap = osifp_create_wlan_vap(&cp, osifp, comdev, cfg80211_create, params, vap->vif_type);
#else
    vap = osifp_create_wlan_vap(&cp, osifp, comdev, cfg80211_create, NULL, vap->vif_type);
#endif
    if (vap == NULL) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: wlan vap failed\n", __FUNCTION__);
        goto fail2;
    }
#endif

    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap) || (IEEE80211_IS_MLO_BRIDGE_VAP(vap) && osifp->netdev)) {
        ret = osif_create_vap_complete(comdev, &cp, osifp, true);
        if (ret != 0) {
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: wlan vap compl failed\n", __FUNCTION__);
            goto fail3;
        }

        if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            switch (vap->link_id) {
                case 0:
                    attr_grp = (struct qdf_vfs_attr *)&ath_link0_attr_group;
                    break;
                case 1:
                    attr_grp = (struct qdf_vfs_attr *)&ath_link1_attr_group;
                    break;
                case 2:
                    attr_grp = (struct qdf_vfs_attr *)&ath_link2_attr_group;
                    break;
                case 3:
                    attr_grp = (struct qdf_vfs_attr *)&ath_link3_attr_group;
                    break;
                default:
                    attr_grp = (struct qdf_vfs_attr *)&ath_attr_group;
                    break;
            }
#else
            attr_grp = (struct qdf_vfs_attr *)&ath_attr_group;
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            if (osifp->mldev) {
                if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&osifp->mldev->mld_dev->dev.kobj, attr_grp)) {
                    QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: sysfs create group failed\n", __FUNCTION__);
                }
            } else {
                if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj, attr_grp)) {
                    QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: sysfs create group failed\n", __FUNCTION__);
                }
            }
#else
            if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj, attr_grp)) {
                QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: sysfs create group failed\n", __FUNCTION__);
            }
#endif
        }

        event.state = VDEV_STATE_RECOVER_CREATE;
        psoc = wlan_vdev_get_psoc(vap->vdev_obj);
        if (psoc) {
            rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
        }
        if (rx_ops && rx_ops->son_rx_ops.deliver_event)
            rx_ops->son_rx_ops.deliver_event(osifp->ctrl_vdev, NULL, MLME_EVENT_VDEV_STATE, &event);
    } else {
        memcpy(&vap->cp, &cp, sizeof(vap->cp));
        osifp->os_handle = ic->ic_osdev;
        osifp->os_devhandle = ic;
        osifp->os_comdev = comdev;
        osifp->os_opmode = wlan_vap_get_opmode(vap);
        osifp->osif_is_mode_offload = 1;
        wlan_vap_set_registered_handle(vap,(os_if_t)osifp);
    }
    /* Returning QDF prints to regular level */
    if (fw_rec_dbg_mask) {
        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_ANY,
                                       QDF_TRACE_LEVEL_NONE, true);
        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_DFS,
                                       QDF_TRACE_LEVEL_NONE, true);
        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_TARGET_IF,
                                       QDF_TRACE_LEVEL_NONE, true);
        for (level = QDF_TRACE_LEVEL_FATAL; level <= QDF_TRACE_LEVEL_DEBUG; level++) {
            if (level <= ic->ic_fw_rec_dbg[QDF_MODULE_ID_ANY]) {
                qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_ANY,
                                               level, true);
            }
            if (level <= ic->ic_fw_rec_dbg[QDF_MODULE_ID_DFS]) {
                qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_DFS,
                                               level, true);
            }

            if (level <= ic->ic_fw_rec_dbg[QDF_MODULE_ID_TARGET_IF]) {
                qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_TARGET_IF,
                                               level, true);
            }
        }

        ic->ic_fw_rec_dbg[QDF_MODULE_ID_ANY] = 0;
        ic->ic_fw_rec_dbg[QDF_MODULE_ID_TARGET_IF] = 0;
        ic->ic_fw_rec_dbg[QDF_MODULE_ID_DFS] = 0;
    }

    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap) || (IEEE80211_IS_MLO_BRIDGE_VAP(vap) && osifp->netdev)) {
        /* Restore netdev register flag after recovery */
        ieee80211_vap_registered_set(vap);

        if ((wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                        WLAN_PDEV_F_MBSS_IE_ENABLE)) && is_mbss_tx_vdev) {
            ieee80211_ucfg_set_txvap(vap);
        }

        /* Start tx queue of the vdev */
        netif_start_queue(dev);
    }

    return 0;

 fail3:
    osif_delete_vap(osifp, 0);
    osif_delete_vap_wait_and_free(osifp, 0);
#if UMAC_SUPPORT_CFG80211
 fail2:
#endif
    if (cp.icp_flags & IEEE80211_MLO_BRIDGE_VAP)
        qdf_mem_free(osifp);
    else {
        osif_create_vap_bssid_free(ic, cp.icp_bssid, &cp);
        osif_create_vap_netdev_unregister(osifp->netdev, osifp->os_unit);
    }
 fail0:
    return -EINVAL;
}

static void wlan_util_get_vdev_ifname_cb(struct wlan_objmgr_psoc *psoc,
                                         void *obj, void *arg)
{
        char *name = NULL;
        osif_dev *osifp = NULL;
        struct wlan_objmgr_vdev *vdev = obj;
        struct wlan_find_vdev_filter *filter = arg;

        if (!vdev || !filter || filter->found_vdev)
            return;

        wlan_vdev_obj_lock(vdev);
        if (vdev->vdev_nif.osdev) {
#if UMAC_SUPPORT_CFG80211
            if (vdev->vdev_nif.osdev->wdev) {
                name = vdev->vdev_nif.osdev->wdev->netdev->name;
            } else {
#else
            {
#endif
                osifp = vdev->vdev_nif.osdev->legacy_osif_priv;
                if (osifp)
                    name = osifp->netdev->name;
            }
        }

        if (!name) {
             wlan_vdev_obj_unlock(vdev);
             return;
        }

        if (!qdf_str_cmp(name, filter->ifname))
            filter->found_vdev = vdev;

        wlan_vdev_obj_unlock(vdev);
}

struct wlan_objmgr_vdev *osif_get_vdev_by_ifname(
                struct wlan_objmgr_psoc *psoc, char *ifname,
                wlan_objmgr_ref_dbgid ref_id)
{
        QDF_STATUS status;
        struct wlan_find_vdev_filter filter = {0};

        filter.ifname = ifname;
        wlan_objmgr_iterate_obj_list(psoc, WLAN_VDEV_OP,
                                     wlan_util_get_vdev_ifname_cb,
                                     &filter, 0, ref_id);

        if (!filter.found_vdev)
            return NULL;

        status = wlan_objmgr_vdev_try_get_ref(filter.found_vdev, ref_id);
        if (QDF_IS_STATUS_ERROR(status))
            return NULL;

        return filter.found_vdev;
}


/*
* Create a virtual ap.  This is public as it must be implemented
* outside our control (e.g. in the driver).
*/
int
osif_ioctl_create_vap(struct net_device *comdev, struct ifreq *ifr,
                        struct ieee80211_clone_params *cp,
                        osdev_t os_handle)
{
#ifdef UMAC_SUPPORT_CFG80211
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211*)get_radio_device_context(comdev->ieee80211_ptr->wiphy, comdev);
#else
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)get_radio_device_context(NULL, comdev);
#endif
    struct ieee80211com *ic = &scn->sc_ic;
    struct net_device *dev = NULL;
    dev = (struct net_device *)osif_create_vap(ic, comdev, cp, 0, NULL, VIF_TYPE_LEGACY);
    if (dev !=NULL) {
        /* return name */
        if (strlcpy(ifr->ifr_name, cp->icp_name, IFNAMSIZ) >= IFNAMSIZ) {
            qdf_print("Source too long !");
            return -EINVAL;
        }
        qdf_print(KERN_ERR "VAP device %s created!",
                  cp->icp_name);
        return 0;
    } else  {
        return -EIO;
    }
}

bool vif_netdev_setup_register_required(struct ieee80211com *ic ,
                            enum virtual_interface_type vif_type)
{
    return true;
}

bool
osif_brvap_is_bonding_mode(struct wlan_objmgr_psoc *psoc)
{
#ifdef WLAN_FEATURE_11BE_MLO
    uint32_t mode;

    mode = cfg_get(psoc, CFG_OL_MLDEV_AP_MODE);

    if (MLD_AP_UNIFIED_BOND_MODE == mode)
        return true;
#endif
    return false;
}

void*
osif_create_vap(struct ieee80211com *ic, struct net_device *comdev,
                struct ieee80211_clone_params *cp, int cfg80211_create,
                void *mld_params, enum virtual_interface_type vif_type)
{
    osif_dev  *osifp;
    int ret;
    struct ieee80211vap *vap;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct qdf_mac_addr *mld_addr;
#endif
    struct qdf_vfs_attr *attr_grp;

    ret = osif_create_vap_check(comdev, cp, mld_params, vif_type);
    if (ret) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: check failed\n", __FUNCTION__);
        return NULL;
    }

    if (cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP &&
        !osif_brvap_is_bonding_mode(wlan_pdev_get_psoc(ic->ic_pdev_obj))) {
        osifp = qdf_mem_malloc(sizeof(osif_dev));
        if (osifp == NULL) {
            qdf_err("osifp create failed for MLO bridge");
            goto fail0;
        }
        memset(osifp,0,sizeof(osif_dev));
        osifp->netdev = NULL;
        osifp->rx_dev = NULL;
    } else {
        osifp = osif_create_vap_netdev_alloc(cp, ic, mld_params, vif_type);
        if (osifp == NULL) {
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
                           QDF_TRACE_LEVEL_INFO, "%s:create netdev failed\n",
                           __FUNCTION__);
            goto fail0;
        }
    }

    vap = osifp_create_wlan_vap(cp, osifp, comdev, cfg80211_create, mld_params,
                                vif_type);
    if (vap == NULL) {
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: wlan vap failed\n", __FUNCTION__);
        goto fail2;
    }

    vap->vif_type = vif_type;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (mld_params && (vif_type == VIF_TYPE_MLD_LINK_VAP))
        vap->link_id = ((struct ieee80211_mld_create_params *)mld_params)->link_id;
    else
        vap->link_id = INVALID_LINK_ID;
#endif

    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap) ||
        (IEEE80211_IS_MLO_BRIDGE_VAP(vap) &&
         osif_brvap_is_bonding_mode(wlan_pdev_get_psoc(ic->ic_pdev_obj))))
    {
        /*
         * For virtual_interface_type VIF_TYPE_MLD_LEGACY_VAP &
	 * VIF_TYPE_LEGACY_VAP we require to setup & register net devices.
         * For Link VAPs netdev not required. For now create netdev till
	 * required changes from NSS & DP are ready.
	 *
         */
        if (vif_netdev_setup_register_required(ic, vif_type)) {
            ret = osif_create_vap_complete(comdev, cp, osifp, false);
            if (ret != 0) {
                QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s: wlan vap compl failed\n", __FUNCTION__);
                goto fail3;
            }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            if (vif_type != VIF_TYPE_MLD_LINK_VAP) {
                ret = osif_create_vap_netdev_register(osifp);
		/* Mark the VAP as registered with Kernel */
		ieee80211_vap_registered_set(vap);
            } else {
                if (ic->ic_wdev.iftype == NL80211_IFTYPE_STATION) {
                    osifp->iv_wdev.wiphy = ic->ic_wdev.wiphy;
                    osifp->iv_wdev.netdev = osifp->mldev->mld_dev;
                    osifp->iv_wdev.iftype = ic->ic_wdev.iftype;
#ifdef WLAN_FEATURE_11BE_MLO
                    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
                    /* The MLD address is available in vdev since it is part of an MLD,
                       copy the same into osifp  */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,1,0)
                    if (!qdf_is_macaddr_zero(mld_addr)) {
                        IEEE80211_ADDR_COPY(&osifp->iv_wdev.mld_address, mld_addr->bytes);
                    }
#endif
#endif
                }
		/* Mark the VAP as registered with Kernel */
		ieee80211_vap_registered_set(vap);
            }
#else
            ret = osif_create_vap_netdev_register(osifp);
#endif

            if (ret != 0) {
                QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "%s:register netdev failed\n", __FUNCTION__);
                goto fail3;
            }
        }
#ifdef WLAN_FEATURE_11BE_MLO
        if (osifp->mldev
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
	    && (vif_type == VIF_TYPE_MLD_LINK_VAP)
#endif
	   ) {
		osif_mld_link_add(osifp->mldev->mld_dev, osifp, vap->link_id);
	     }
#endif

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        switch (vap->link_id) {
            case 0:
                attr_grp = (struct qdf_vfs_attr *)&ath_link0_attr_group;
                break;
            case 1:
                attr_grp = (struct qdf_vfs_attr *)&ath_link1_attr_group;
                break;
            case 2:
                attr_grp = (struct qdf_vfs_attr *)&ath_link2_attr_group;
                break;
            case 3:
                attr_grp = (struct qdf_vfs_attr *)&ath_link3_attr_group;
                break;
            default:
                attr_grp = (struct qdf_vfs_attr *)&ath_attr_group;
                break;
        }
#else
        attr_grp = (struct qdf_vfs_attr *)&ath_attr_group;
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
        if (osifp->mldev) {
            if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&osifp->mldev->mld_dev->dev.kobj,
                                        attr_grp)) {
                      QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                           "%s: sysfs create group failed\n", __FUNCTION__);
            }

            QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO_HIGH, "VAP device %s created osifp: (%pK) os_if: (%pK)",
                      osifp->mldev->mld_dev->name, osifp, osifp->os_if);
        } else {
            if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&osifp->netdev->dev.kobj,
                                        attr_grp)) {
                      QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                           "%s: sysfs create group failed\n", __FUNCTION__);
            }

            QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO_HIGH, "VAP device  created osifp: (%pK) os_if: (%pK)",
                      osifp, osifp->os_if);
        }
#else
            if (qdf_vfs_set_file_attributes((struct qdf_dev_obj *)&osifp->netdev->dev.kobj,
                                        attr_grp)) {
                      QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                           "%s: sysfs create group failed\n", __FUNCTION__);
            }

            QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO_HIGH, "VAP device created osifp: (%pK) os_if: (%pK)",
                      osifp, osifp->os_if);
#endif

        return get_cfg80211_notification_ndev(osifp);
    } else {
        memcpy(&vap->cp, cp, sizeof(vap->cp));
        osifp->os_handle = ic->ic_osdev;
        osifp->os_devhandle = ic;
        osifp->os_comdev = comdev;
        osifp->os_opmode = wlan_vap_get_opmode(vap);
        osifp->osif_is_mode_offload = 1;
        wlan_vap_set_registered_handle(vap,(os_if_t)osifp);

        QDF_TRACE(QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO, "Bridge VAP created osifp: (%pK) os_if: (%pK)",
                  osifp, osifp->os_if);

        return osifp;
    }

 fail3:
    osif_delete_vap(osifp, 0);
    osif_delete_vap_wait_and_free(osifp, 0);
 fail2:
    if (cp->icp_flags & IEEE80211_MLO_BRIDGE_VAP) {
        if (osif_brvap_is_bonding_mode(wlan_pdev_get_psoc(ic->ic_pdev_obj))) {
            qdf_net_delete_mlobrunit(osifp->os_unit);
            free_netdev(osifp->netdev);
        } else
            qdf_mem_free(osifp);
    }
    else {
        qdf_net_if_release_dev((struct qdf_net_if *)osifp->rx_dev);
        osif_create_vap_bssid_free(ic, cp->icp_bssid, cp);
        osif_create_vap_netdev_free(osifp->netdev, osifp->os_unit);
    }
 fail0:
    return NULL;
}
qdf_export_symbol(osif_ioctl_create_vap);

void
osif_delete_vap_unregister_nss(osif_dev *osnetdev, int recover)
{
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct net_device *dev = get_cfg80211_notification_ndev(osnetdev);

    if (!dev)
        return;

    if (osnetdev->nss_wifiol_ctx) {
        /*
         * When wifi offload is enabled, we could have NSS qdiscs configured
         * on the wifi interface. These need to be destroyed before we
         * can destroy the interface.
         */
        wlan_if_t vap = osnetdev->os_if;
        struct ieee80211com *ic = wlan_vap_get_devhandle(vap);
        if (recover) {
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            if (!osnetdev->mldev)
#endif
                dev_shutdown(dev);
        }
        if (ic->nss_vops) {
            ic->nss_vops->ic_osif_nss_vap_delete(osnetdev, recover);
        }
        osnetdev->nss_ifnum =  -1;
        return;
    } else
#endif
    {
#if QCA_NSS_PLATFORM
    if (!(of_machine_is_compatible("qcom,ipq40xx"))) {
        osif_pltfrm_delete_vap(osnetdev);
    }
#elif QCA_PPE_VP
    if (!recover)
        osif_pltfrm_delete_vap(osnetdev);
#elif defined(QCA_PARTNER_PLATFORM )
    osif_pltfrm_delete_vap(osnetdev);
#endif
    }
}

int
osif_delete_vap_start(osif_dev *osnetdev, int recover)
{
    struct net_device *dev = get_cfg80211_notification_ndev(osnetdev);
    wlan_if_t vap = osnetdev->os_if;
    struct ieee80211com *ic;
    struct wlan_vdev_state_event event;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
    struct qdf_vfs_attr *attr_grp;

    if (!vap || !dev) {
        qdf_err("vap or netdev is NULL");
        return -EINVAL;
    }

    ic = wlan_vap_get_devhandle(vap);
    osnetdev->is_delete_in_progress=1;
    ieee80211_proc_vdetach(vap);

    if (ic->ic_cbs)
        wlan_bk_scan_stop(ic);

    event.state = VDEV_STATE_DELETED;
    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }
    if (rx_ops && rx_ops->son_rx_ops.deliver_event)
        rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_VDEV_STATE, &event);

#if QCA_PARTNER_DIRECTLINK_TX
    if (qdf_unlikely(osnetdev->is_directlink)) {
        if (wlan_pltfrm_directlink_disable(osnetdev)) {
            qdf_print(KERN_ERR "%s: direct link unable to deregister device", dev->name);
        }
    }
#endif /* QCA_PARTNER_DIRECTLINK_TX */



#ifdef ATHEROS_LINUX_PERIODIC_SCAN
    OS_FREE_TIMER(&(osnetdev->os_periodic_scan_timer));
#endif
#ifdef ATH_SUPPORT_WAPI
    OS_FREE_TIMER(&(osnetdev->os_wapi_rekey_timer));
#endif
    ifmedia_removeall(&osnetdev->os_media);
    /* unregister scan event handler */
        ucfg_scan_unregister_event_handler(wlan_vap_get_pdev(vap), &osif_scan_evhandler, (void *)osnetdev);
        ucfg_scan_unregister_requester(wlan_vap_get_psoc(vap), osnetdev->scan_requestor);
#if UMAC_PER_PACKET_DEBUG
    remove_proc_entry(vap->iv_proc_fname, vap->iv_proc_root);
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    switch (vap->link_id) {
        case 0:
                attr_grp = (struct qdf_vfs_attr *)&ath_link0_attr_group;
                break;
        case 1:
                attr_grp = (struct qdf_vfs_attr *)&ath_link1_attr_group;
                break;
        case 2:
                attr_grp = (struct qdf_vfs_attr *)&ath_link2_attr_group;
                break;
        case 3:
                attr_grp = (struct qdf_vfs_attr *)&ath_link3_attr_group;
                break;
        default:
                attr_grp = (struct qdf_vfs_attr *)&ath_attr_group;
                break;
        }
#else
        attr_grp = (struct qdf_vfs_attr *)&ath_attr_group;
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (osnetdev->mldev) {
        qdf_vfs_clear_file_attributes((struct qdf_dev_obj *)&osnetdev->mldev->mld_dev->dev.kobj,
                                  attr_grp);
    } else {
        qdf_vfs_clear_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj,
                                  attr_grp);
    }
#else
    qdf_vfs_clear_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj,
                                  attr_grp);
#endif
#if ATH_SUPPORT_WRAP
    wlan_rptr_vdev_delete_start(vap->vdev_obj);
#endif

    if (recover) {
        wlan_assoc_sm_msgq_drain(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj));

        /* Drain mgmt pkts before dev_change_flags, else node references are
         * not freed in osif_vap_stop called from dev_changes_flags
         */
        ieee80211_flush_vap_mgmt_queue(vap, true);
    }
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (!osnetdev->mldev) {
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
        dev_change_flags(dev, dev->flags & (~IFF_UP), NULL);
#else
        dev_change_flags(dev, dev->flags & (~IFF_UP));
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    }
#endif

#if ATH_SUPPORT_WRAP
    /* ProxySTA VAP has its own mac address and doesn't use mBSSID */
    if (!wlan_rptr_vdev_is_psta(vap->vdev_obj))
        wlan_vap_free_mac_addr(ic, vap->iv_myaddr, &vap->cp);
#else
    /* free the MAC address */
    wlan_vap_free_mac_addr(ic, vap->iv_myaddr, &vap->cp);
#endif

    return 0;
}

int
osif_delete_vap_complete(osif_dev *osnetdev)
{
    struct net_device *dev;

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (osnetdev->mldev)
        dev = osnetdev->mldev->mld_dev;
    else
        dev = osnetdev->netdev;
#else
    dev = osnetdev->netdev;
#endif

    osnetdev->is_delete_in_progress = 0;

    spin_lock_destroy(&osnetdev->list_lock);
    spin_lock_destroy(&osnetdev->tx_lock);

#ifdef WLAN_FEATURE_FASTPATH
        if (osnetdev->osif_is_mode_offload) {
            ASSERT(osnetdev->nbuf_arr);
            /* Free up the accumulated SKB's also */
            qdf_mem_free(osnetdev->nbuf_arr);
            osnetdev->nbuf_arr = NULL;
            spin_lock_destroy(&osnetdev->nbuf_arr_lock);
        }
#endif

#if UMAC_SUPPORT_WEXT
    ieee80211_ioctl_vdetach(dev);
#endif
    return 0;
}

void
osif_delete_vap_netdev_unregister(osif_dev *osifp)
{
    wlan_if_t vap = osifp->os_if;
    struct net_device *dev = get_cfg80211_notification_ndev(osifp);

    /* Mark the VAP as unregistered with Kernel */
    ieee80211_vap_registered_clear(vap);
    /* release the reference of rx_dev */
    qdf_net_if_release_dev((struct qdf_net_if *)osifp->rx_dev);

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (!osifp->mldev)
        wlan_cfg80211_unregister_netdevice(dev);
#else
    wlan_cfg80211_unregister_netdevice(dev);
#endif
}

int osif_delete_vap_netdev_unregister_complete(osif_dev *osnetdev)
{
    qdf_net_delete_wlanunit(osnetdev->os_unit);

    return 0;
}

int
osif_delete_vap_recover(wlan_if_t vap)
{
    osif_dev *osnetdev = (osif_dev *)wlan_vap_get_registered_handle(vap);
    struct ieee80211com *ic;
    struct ieee80211_profile *profile;
    struct ieee80211vap_profile *vap_profile = NULL;
    struct wlan_vdev_state_event event;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
    QDF_TRACE_LEVEL level;

    event.state = VDEV_STATE_RECOVER_DELETE;
    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }
    if (rx_ops && rx_ops->son_rx_ops.deliver_event)
        rx_ops->son_rx_ops.deliver_event(osnetdev->ctrl_vdev, NULL, MLME_EVENT_VDEV_STATE, &event);

    /* save the VAP's profile before deleting */
    ic = wlan_vap_get_devhandle(vap);
    profile = ic->profile;

    vap_profile = &profile->vap_profile[profile->num_vaps];
    wlan_get_vap_info(vap, vap_profile, NULL);
    profile->num_vaps++;

    /*
     * Disables excessive prints from showing up during firmware recovery
     * Only the following modules are disabled, but more can be added based
     * as needed - ANY, TIF, DFS
     * This downgrades the prints to value set in module param "fw_rec_dbg_mask"
     */
    if (fw_rec_dbg_mask) {
        for (level = QDF_TRACE_LEVEL_DEBUG; level >= QDF_TRACE_LEVEL_FATAL; level--) {
            if (!(ic->ic_fw_rec_dbg[QDF_MODULE_ID_ANY]) &&
                     qdf_print_is_verbose_enabled(qdf_get_pidx(), QDF_MODULE_ID_ANY,
                                                  level)) {
                ic->ic_fw_rec_dbg[QDF_MODULE_ID_ANY] = level;
            }

            if (!(ic->ic_fw_rec_dbg[QDF_MODULE_ID_DFS]) &&
                     qdf_print_is_verbose_enabled(qdf_get_pidx(), QDF_MODULE_ID_DFS,
                                                  level)) {
                ic->ic_fw_rec_dbg[QDF_MODULE_ID_DFS] = level;
            }

            if (!(ic->ic_fw_rec_dbg[QDF_MODULE_ID_TARGET_IF]) &&
                     qdf_print_is_verbose_enabled(qdf_get_pidx(), QDF_MODULE_ID_TARGET_IF,
                                                  level)) {
                ic->ic_fw_rec_dbg[QDF_MODULE_ID_TARGET_IF] = level;
            }
        }

        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_ANY,
                                       QDF_TRACE_LEVEL_NONE, true);
        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_ANY,
                                       fw_rec_dbg_mask, true);

        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_DFS,
                                       QDF_TRACE_LEVEL_NONE, true);
        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_DFS,
                                       fw_rec_dbg_mask, true);

        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_TARGET_IF,
                                       QDF_TRACE_LEVEL_NONE, true);
        qdf_print_set_category_verbose(qdf_get_pidx(), QDF_MODULE_ID_TARGET_IF,
                                       fw_rec_dbg_mask, true);
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        wlan_mlme_stop_vdev(vap->vdev_obj, WLAN_MLME_STOP_BSS_F_WAIT_RX_DONE |
                WLAN_MLME_STOP_BSS_F_NO_RESET, WLAN_MLME_NOTIFY_OSIF);
        wlan_delete_mlo_bridge_vap(vap, 1);
    } else
#endif
    {
        osif_delete_vap_start(osnetdev, 1);

#ifndef QCA_SINGLE_WIFI_3_0
        osif_delete_vap_unregister_nss(osnetdev, 1);
#endif

        osif_delete_vap(osnetdev, 1);

        osif_delete_vap_wait_and_free(osnetdev, 1);
        osif_delete_vap_complete(osnetdev);
    }
    return 0;
}
qdf_export_symbol(osif_delete_vap_recover);

int
osif_ioctl_delete_vap(struct net_device *dev, uint8_t link_id)
{
#if UMAC_SUPPORT_CFG80211
    osif_dev *osnetdev = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, link_id);
    wlan_if_t vap;
    struct ieee80211com *ic;
    struct wlan_objmgr_psoc *psoc;
    bool is_mlo_bridge;

    if (!osnetdev)
        return 0;

    vap = osnetdev->os_if;
    if (!vap)
        return 0;

    ic = vap->iv_ic;
    is_mlo_bridge = IEEE80211_IS_MLO_BRIDGE_VAP(vap);

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (psoc == NULL) {
        qdf_err("psoc is null");
        return 0;
    }

    if (ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP) {
        if (!wlan_psoc_nif_fw_ext_cap_get(psoc,
                     WLAN_SOC_CEXT_MBSS_PARAM_IN_START) &&
            (wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj, WLAN_PDEV_F_MBSS_IE_ENABLE) &&
            !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) &&
            !ic->ic_wifi_down_ind)) {
            qdf_warn("[MBSSID] Trying to delete a Transmitting VAP without"
                     "deleting non-transmitting VAPs first, Aborting VAP delete..\n");
            return 0;
        }
    }
#else
    osif_dev *osnetdev = get_vap_device_context(NULL, dev, INVALID_LINK_ID);

    if (!osnetdev)
        return 0;
#endif

    /*
     * unregister_netdev is done first and other clean up is done later by holding dev_hold()
     * Please maintain the current order
     * First - unregister_netdev()
     * next - nss vdev dealloc done osif_delete_vap_unregister_nss()
     */
    qdf_net_if_hold_dev((struct qdf_net_if *)dev);
    osif_delete_vap_start(osnetdev, 0);
#ifdef WLAN_FEATURE_11BE_MLO
    if (osnetdev->mldev)
        osif_mld_link_delete(osnetdev);
#endif
    osif_delete_vap_netdev_unregister(osnetdev);
#ifndef QCA_SINGLE_WIFI_3_0
    osif_delete_vap_unregister_nss(osnetdev, 0);
#endif
#if UMAC_SUPPORT_CFG80211
    if (vap->iv_cfg80211_create) {
        qdf_flush_work(&vap->cfg80211_channel_notify_workq);
        qdf_flush_work(&vap->cfg80211_channel_started_notify_workq);
        qdf_destroy_work(NULL, &vap->cfg80211_channel_notify_workq);
        qdf_destroy_work(NULL, &vap->cfg80211_channel_started_notify_workq);

        if (ieee80211vap_get_opmode(vap) == IEEE80211_M_STA) {
            qdf_flush_work(&vap->cfg80211_ft_supp_workq);
            qdf_destroy_work(NULL, &vap->cfg80211_ft_supp_workq);
        }
        qdf_flush_work(&vap->acs_bk_cmpl_work);
        qdf_destroy_work(NULL, &vap->acs_bk_cmpl_work);
    }
#endif
    osif_delete_vap(osnetdev, 0);

    osif_delete_vap_wait_and_free(osnetdev, 0);

    osif_delete_vap_complete(osnetdev);

    if (is_mlo_bridge)
        qdf_net_delete_mlobrunit(osnetdev->os_unit);
    else
        osif_delete_vap_netdev_unregister_complete(osnetdev);

    qdf_net_if_release_dev((struct qdf_net_if *)dev);

    return 0;
}
qdf_export_symbol(osif_ioctl_delete_vap);

int
osif_ioctl_switch_vap(struct net_device *dev, enum ieee80211_opmode opmode)
{
#ifdef UMAC_SUPPORT_CFG80211
        osif_dev *osifp = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, INVALID_LINK_ID);
#else
        osif_dev *osifp = get_vap_device_context(NULL, dev, INVALID_LINK_ID);
#endif
        wlan_if_t vap;
        u_int8_t *macaddr;
        u_int64_t msg_flags;

        if (!osifp) {
            qdf_err("osifp NULL");
            return -EINVAL;
        }

        vap = osifp->os_if;

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : from %d to %d  \n",
                        __func__, osifp->os_opmode, opmode);


        if (osifp->os_opmode == opmode) {
            return 0;
        }

        macaddr = wlan_vap_get_macaddr(vap);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : old mac addr %s   \n", __func__, ether_sprintf(macaddr));

        _netif_carrier_off(dev);
        msg_flags = wlan_get_debug_flags(vap);
        osifp->is_delete_in_progress = 1;
        osif_vap_stop_wrapper(osifp);
        osif_delete_vap(osifp, 0);
        osif_delete_vap_wait_and_free(osifp, 0);
        osifp->os_if = NULL;
        osif_setup_vap(osifp,opmode,IEEE80211_CLONE_BSSID,NULL);
        vap = osifp->os_if;
        if (!vap) {
           return -EINVAL;
        }
        wlan_set_debug_flags(vap, msg_flags);
        _netif_carrier_on(dev);
        macaddr = wlan_vap_get_macaddr(vap);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s : new mac addr %s   \n", __func__, ether_sprintf(macaddr));

        return 0;
}

int osif_recover_profile_alloc(struct ieee80211com *ic)
{
    struct ieee80211_profile *profile;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct net_device *dev = scn->sc_osdev->netdev;
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;

    if(ic->profile) {
        qdf_mem_free(ic->profile);
        ic->profile = NULL;
    }

    profile = (struct ieee80211_profile *)qdf_mem_malloc(
                  sizeof (struct ieee80211_profile));
    if (profile == NULL) {
        return -ENOMEM;
    }
    OS_MEMSET(profile, 0, sizeof (struct ieee80211_profile));

    if (strlcpy(profile->radio_name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
        qdf_print("source too long");
        return -ENOMEM;
    }
    wlan_get_device_mac_addr(ic, profile->radio_mac);
    profile->cc = (u_int16_t)wlan_get_device_param(ic,
                                IEEE80211_DEVICE_COUNTRYCODE);
    *chan = wlan_get_dev_current_channel(ic);
    if (IEEE80211_IS_CHAN_VALID(chan)) {
        profile->channel = chan->ic_ieee;
        profile->freq = chan->ic_freq;
    }

    ic->profile = profile;

    return 0;
}
qdf_export_symbol(osif_recover_profile_alloc);

int osif_recover_profile_free(struct ieee80211com *ic)
{
    if(ic->profile)
        qdf_mem_free(ic->profile);
    ic->profile = NULL;

    return 0;
}

void
osif_recover_vap_create(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ieee80211_profile *profile = ic->profile;
    struct ieee80211vap_profile *vap_profile = NULL;
    int i, ret = 0;

    if (!profile)
        return;

    for(i = 0; i < profile->num_vaps; i++) {
        vap_profile = &profile->vap_profile[i];
        ret = osif_create_vap_recover(scn->netdev,
                                vap_profile->vap,
                                (osdev_t) scn->sc_osdev,
#ifdef WLAN_FEATURE_11BE_MLO
                                &vap_profile->mld_params,
#endif
                                vap_profile->txvap);

        vap_profile->vap->iv_des_mode = vap_profile->phymode;
        if (ret != 0) {
            QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_INFO,
                           "Warning: Vap %pK recovery failed, trying other vaps...\n",
                   vap_profile->vap);
        }
#ifdef WLAN_FEATURE_11BE_MLO
        else if (IEEE80211_IS_MLO_BRIDGE_VAP(vap_profile->vap)) {
            /* Increment the pdev bridge vap count */
            wlan_pdev_inc_mlo_bridge_vdev_count(ic->ic_pdev_obj);

            /* Set ESSID for bridge VAP */
            wlan_set_ssid_mlo_bridge_vap(vap_profile->vap);
        }
#endif
    }

    osif_recover_profile_free(ic);
}
qdf_export_symbol(osif_recover_vap_create);

int
wlan_get_vap_info(struct ieee80211vap *vap,
                    struct ieee80211vap_profile *vap_profile,
                    void *handle)
{
    ieee80211_keyval k;
    int kid;
    u_int8_t *macaddr;
    osif_dev *osif;
    int32_t cipher;
    int32_t authmode;
    wlan_crypto_auth_mode mode;
#ifdef WLAN_FEATURE_11BE_MLO
    struct qdf_mac_addr *mld_addr;
#endif

    osif = (osif_dev *)wlan_vap_get_registered_handle(vap);
    vap_profile->vap = vap;

    if (IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
        if (strlcpy(vap_profile->name, "ath-br", IFNAMSIZ) >= IFNAMSIZ) {
            qdf_err("source too long");
            return -1;
        }
    } else {
        if (strlcpy(vap_profile->name, osif->netdev->name, IFNAMSIZ) >= IFNAMSIZ) {
            qdf_err("source too long");
            return -1;
        }
    }

    if (wlan_pdev_nif_feat_cap_get(vap->iv_ic->ic_pdev_obj,
                                    WLAN_PDEV_F_MBSS_IE_ENABLE)) {
        uint8_t grp_id = vap->iv_mbss.group_id;
        struct mbssid_info *ic_mbss = &vap->iv_ic->ic_mbss_ctx->ic_mbss[grp_id];

        if (!IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) &&
            (vap == ic_mbss->transmit_vap))
            vap_profile->txvap = 1;
    }
    wlan_get_vap_addr(vap, vap_profile->vap_mac);
    vap_profile->phymode = wlan_get_desired_phymode(vap);

#ifdef WLAN_FEATURE_11BE_MLO
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);

    if (!qdf_is_macaddr_zero(mld_addr)) {
        IEEE80211_ADDR_COPY(&vap_profile->mld_params.mld_macaddr, mld_addr->bytes);
    }
#endif

    authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
    if ( authmode == -1 ) {
        qdf_err("crypto_err while getting authmode params\n");
        return -1;
    }

    if (authmode & (uint32_t)((1 << WLAN_CRYPTO_AUTH_OPEN) | (1 << WLAN_CRYPTO_AUTH_SHARED)))
        vap_profile->sec_method = (uint32_t)((1 << WLAN_CRYPTO_AUTH_AUTO));
    else {
	for (mode = WLAN_CRYPTO_AUTH_NONE; mode <= WLAN_CRYPTO_AUTH_MAX; mode++) {
           if (authmode & (uint32_t)((1 << mode)))
              vap_profile->sec_method = mode;
        }
    }

    cipher = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER);
    if ( cipher == -1 ) {
        qdf_err("crypto_error while getting cipher params\n");
        return -1;
    }
    vap_profile->cipher = cipher;
    if (wlan_get_param(vap, IEEE80211_FEATURE_PRIVACY))
    {
        vap_profile->cipher |= 1<<IEEE80211_CIPHER_WEP;
        kid = wlan_get_default_keyid(vap);
        if (kid == IEEE80211_KEYIX_NONE) {
            kid = 0;
        }
        if ((0 <= kid) && (kid < IEEE80211_WEP_NKID)) {
            macaddr = (u_int8_t *)ieee80211broadcastaddr;
            k.keydata = vap_profile->wep_key[kid];
            if (wlan_get_key(vap, kid, macaddr, &k, 64, GET_PN_ENABLE) == 0) {
                vap_profile->wep_key_len[kid] = k.keylen;
            }
        }
    } else {
        vap_profile->cipher &= ~(1<<IEEE80211_CIPHER_WEP);
    }
    return 0;
}
qdf_export_symbol(wlan_get_vap_info);

#if UMAC_SUPPORT_ACFG
int
osif_set_vap_vendor_param(struct net_device *dev, acfg_vendor_param_req_t *req, uint8_t link_id)
{
    int status = 0, reinit = 0;
#ifdef UMAC_SUPPORT_CFG80211
    osif_dev *osifp = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, link_id);
#else
    osif_dev *osifp = get_vap_device_context(NULL, dev, link_id);
#endif
    if (!osifp) {
        qdf_err("NULL osifp");
        return -1;
    }

    /* vendors can add their commands here */
    switch(req->param)
    {
        case ACFG_VENDOR_PARAM_CMD_PRINT:
            qdf_print("Received user print data %s", (char *)&req->data);
            break;
        case ACFG_VENDOR_PARAM_CMD_INT:
            qdf_print("Received user int data %d", *(uint32_t *)&req->data);
            break;
        case ACFG_VENDOR_PARAM_CMD_MAC:
            qdf_print("Received user mac data %02x:%02x:%02x:%02x:%02x:%02x", req->data.data[0], \
                                                                             req->data.data[1], \
                                                                             req->data.data[2], \
                                                                             req->data.data[3], \
                                                                             req->data.data[4], \
                                                                             req->data.data[5]);
            break;
        default:
            qdf_print("ACFG driver unknown vendor param");
            status = -1;
    }
    /* If driver restart required, please set the "ENETRESET" flag in reinit variable */
    if(reinit == ENETRESET)
    {
        /* Reinit the wlan driver */
        return IS_UP(dev) ? -osif_vap_init(osifp, dev, RESCAN) : 0;
    }
    return status;
}

int
osif_get_vap_vendor_param(struct net_device *dev, acfg_vendor_param_req_t *req)
{
    int status = 0;

    /* vendors can add their commands here */
    switch(req->param)
    {
        case ACFG_VENDOR_PARAM_CMD_PRINT:
            strlcpy((char *)&req->data, "QCA", sizeof(req->data));
            req->type = ACFG_TYPE_STR;
            qdf_print("Sending driver data %s type %d", (char *)&req->data, req->type);
            break;
        case ACFG_VENDOR_PARAM_CMD_INT:
            req->type = ACFG_TYPE_INT;
            *(uint32_t *)&req->data = 12345678;
            qdf_print("Sending driver data %d type %d", *(uint32_t *)&req->data, req->type);
            break;
        case ACFG_VENDOR_PARAM_CMD_MAC:
        {
            int i;
            req->type = ACFG_TYPE_MACADDR;
            for(i = 0; i < MAC_ADDR_LEN; i++)
                req->data.data[i] = i;
            qdf_print("Sending driver mac data %02x:%02x:%02x:%02x:%02x:%02x type %d", req->data.data[0], \
                                                                                      req->data.data[1], \
                                                                                      req->data.data[2], \
                                                                                      req->data.data[3], \
                                                                                      req->data.data[4], \
                                                                                      req->data.data[5], req->type);
            break;
        }
        default:
            qdf_print("ACFG driver invalid vendor param");
            status = -1;
    }
    return status;
}
#endif //UMAC_SUPPORT_ACFG

#if UMAC_PER_PACKET_DEBUG

int  osif_atoi_proc(char *buf) {
    int value=0;
    while(*buf) {
        value=(value << 3) + (value << 1) + (*buf - '0');
        buf++;
    }
    return value;
}


ssize_t osif_proc_pppdata_write(struct file *filp, const char __user *buff,
                        unsigned long len, void *data )
{
	struct ieee80211vap *vap;
    struct ieee80211com    *ic;
	char cmd[20], buf1[10], buf2[10];
	int filter,selevm;

	vap = (struct ieee80211vap *) data;
    ic = wlan_vap_get_devhandle(vap);
    sscanf(buff,"%s %s %s",cmd ,buf1 ,buf2);
    if (strcmp("rate_retry", cmd) == 0) {
	    vap->iv_userrate = osif_atoi_proc(buf1);
	    wlan_vdev_set_userrate(osif_atoi_proc(buf1));
      vap->iv_userretries = osif_atoi_proc(buf2);
	    wlan_vdev_set_userretries(osif_atoi_proc(buf2));
	} else if(strcmp("txchainmask", cmd) == 0) {
	    vap->iv_usertxchainmask = osif_atoi_proc(buf1);
	    wlan_vdev_set_usertxchainmask(osif_atoi_proc(buf1));
	} else if(strcmp("txpower", cmd) == 0) {
	    vap->iv_usertxpower = osif_atoi_proc(buf1);
	    wlan_vdev_set_usertxpower(osif_atoi_proc(buf1));
	} else if(strcmp("rxfilter", cmd) == 0) {
	    filter = osif_atoi_proc(buf1);
        ic->ic_set_rxfilter(ic->ic_pdev_obj, filter);
    } else if(strcmp("plcpheader", cmd) == 0) {
        selevm = osif_atoi_proc(buf1);
        ic->ic_set_rx_sel_plcp_header(ic, !selevm, 0);
    }
    return len;
}

#endif



static INLINE void
ieee80211_vap_resetdeschan(void *arg, struct ieee80211vap *vap, bool is_last_vap)
{
    *vap->iv_des_chan[vap->iv_des_mode] = empty_channel;

}
extern void osif_ht40_event_handler(struct wlan_objmgr_vdev *vdev, void *arg);

static void
is_sta_connected(void *arg, wlan_node_t node)
{
    u_int32_t *is_valid = (u_int32_t *)arg;
    *is_valid =1;

}


static void osif_staconnected_check(void *arg, wlan_if_t vap)
{
    u_int32_t  *staconnected = (u_int32_t *) arg;
    u_int32_t is_valid = 0 ;

    wlan_iterate_station_list(vap, is_sta_connected, &is_valid);
    if(is_valid == 1){
        *staconnected= 1;
    }

}


/*
* Auto Channel Select handler used for interface up.
*/
static void osif_bkscan_acs_event_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{

    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);
    int error = 0;
    wlan_chan_t channel = (wlan_chan_t)arg;
    osif_dev *osifp;
    enum ieee80211_phymode orig_mode;

    if (!vap ) {
        qdf_info("vap is NULL");
        goto done;
    }
    osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
    orig_mode = vap->iv_des_mode;

    if (!IEEE80211_IS_CHAN_VALID(channel)) {
        error = -1;
        goto done;
    }

#ifdef CONFIG_AFC_SUPPORT
    if (ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(channel))) {
        error = ieee80211_update_mode_and_power_type(vap, channel,
                                                     ieee80211_chan2mode(channel));
        if (error) {
            qdf_err("Failed to change regulatory AP power type");
            goto done;
        }
    }
#endif /* CONFIG_AFC_SUPPORT */

    error = wlan_set_ieee80211_channel(vap, channel);
    if (error !=0) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                        "%s : failed to set channel with error code %d\n",
                        __func__, error);
        goto done;
    }

    qdf_sched_work(0, &vap->acs_bk_cmpl_work);

done:
#ifdef CONFIG_AFC_SUPPORT
    if (error &&
        ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(vap->iv_ic->ic_curchan))) {
        ieee80211_update_mode_and_power_type(vap, vap->iv_ic->ic_curchan,
                                             orig_mode);
        vap->iv_afc_orig_mode = IEEE80211_MODE_AUTO;
    }
#endif /* CONFIG_AFC_SUPPORT */

    return;
}

void acs_bk_scan_cmpl_handler(void *data)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)data;
    struct ieee80211_vap_info vap_info;

    vap_info.chan = vap->iv_des_chan[vap->iv_des_mode];
    vap_info.no_cac = 0;

    wlan_mbss_start_ap_vdevs(wlan_vdev_get_pdev(vap->vdev_obj), &vap_info);

    if (wlan_coext_enabled(vap)) {
        wlan_determine_cw(vap, vap->iv_des_chan[vap->iv_des_mode]);
    }

    return;
}

static void osif_acs_bk_scantimer_fn( void *arg )
{
    struct ieee80211com *ic = (struct ieee80211com *)arg ;
    struct ieee80211vap *vap;
    u_int32_t  staconnected = 0, num_vaps;
    int acs_ctrlflags = ic->ic_acs_ctrlflags;
    struct wlan_mbss_ev_data mbss_ev = {0};

    if(!osif_get_num_active_vaps(ic)){
        return;
    }

    /* Check if CW Interference is already been found and being handled */
    if (ic->cw_inter_found) {
        return;
    }
    if (acs_ctrlflags & (ACS_PEREODIC_OBSS_CHK | ACS_PEIODIC_SCAN_CHK))  {
        wlan_iterate_vap_list(ic,osif_staconnected_check,(void *)&staconnected);
        if(staconnected == 1)
            return;
    }

    /* Loop through and figure the first VAP on this radio */
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
#ifdef QCA_PARTNER_PLATFORM
            if (!IEEE80211_IS_CHAN_VALID(vap->iv_des_chan[vap->iv_des_mode]))
            	return;
            if (!vap->iv_list_scanning)
#endif
            goto vapfound;
        }
    }

vapfound :
    if(vap == NULL)
        return;

    /* Don't interfere with CAC in progress */
    if (wlan_is_ap_cac_timer_running(ic))
        return;

    if(acs_ctrlflags & ACS_PEIODIC_SCAN_CHK){
        ieee80211_iterate_vap_list_internal(ic, ieee80211_vap_resetdeschan,vap,num_vaps);
        wlan_mbss_stop_ap_monitor_vdevs(wlan_vdev_get_pdev(vap->vdev_obj), NULL);
        *vap->iv_des_chan[vap->iv_des_mode] = empty_channel;
        wlan_pdev_wait_to_bringdown_vdevs(ic, ALL_VDEVS);

        mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_ACS;
        mbss_ev.ext_cb = &osif_bkscan_acs_event_handler;
        mbss_ev.source.acs_source = MBSS_ACS_SRC_BG_SCAN;
        mbss_ev.arg = NULL;
        wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
    } else if(acs_ctrlflags & ACS_PEREODIC_OBSS_CHK){
        enum ieee80211_phymode des_mode = wlan_get_desired_phymode(vap);

        /* 11AX TODO: Recheck future 802.11ax drafts (>D1.0) on coex rules */

        /* Check only for HE40/HT40 or HE20/HT20  do not change channel */
        if (ieee80211_is_phymode_g40(des_mode) && wlan_coext_enabled(vap)) {
#ifdef QCA_PARTNER_PLATFORM
        vap->iv_list_scanning = 1;
#endif
            ic->ic_obss_done_freq = 0;
            wlan_mbss_sched_start_stop(vap->vdev_obj, MBSS_SCHED_PDEV_STOP_START);
        }
    } else if(acs_ctrlflags & ACS_PEIODIC_BG_SCAN) {
        mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_ACS;
        mbss_ev.source.acs_source = MBSS_ACS_SRC_BG_SCAN;
        mbss_ev.arg = NULL;
        wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
    }
}

static int
proc_clients_keys_show(struct seq_file *m, void *v)
{
    return 0;
}

#ifdef QCA_SUPPORT_WDS_EXTENDED
int
osif_wds_ext_delete_netdev(struct net_device *dev)
{
   osif_peer_dev *osifp = ath_netdev_priv(dev);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   wlan_if_t vap = NULL;
   struct ieee80211com *ic = NULL;
   osif_dev  *osdev = NULL;
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

   qdf_net_if_hold_dev((struct qdf_net_if *)dev);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   osdev = osifp->parent_osif;
   vap = osdev->os_if;
   ic = vap->iv_ic;
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

   qdf_vfs_clear_file_attributes((struct qdf_dev_obj *)&dev->dev.kobj,
                                 (struct qdf_vfs_attr *)&ext_attr_group);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 24)
    dev_change_flags(dev, dev->flags & (~IFF_UP), NULL);
#else
    dev_change_flags(dev, dev->flags & (~IFF_UP));
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    /*
     * Make the NSS interface down.
     */
    if (ic && ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_down)
        ic->nss_ext_vops->ic_osif_nss_ext_vdev_down(osifp);

    /*
     * Delete the NSS interface.
     */
    if (ic && ic->nss_ext_vops && ic->nss_ext_vops->ic_osif_nss_ext_vdev_delete)
        ic->nss_ext_vops->ic_osif_nss_ext_vdev_delete(osifp);
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

   /*
    * Release the reference on the parent interface.
    */
   if (osifp->parent_netdev)
       qdf_net_if_release_dev((struct qdf_net_if *)osifp->parent_netdev);

   /* Release the reference to vdev */
   if (osifp->vdev)
       wlan_objmgr_vdev_release_ref(osifp->vdev, WLAN_OSIF_ID);

   /* stop the tx queue just before unregistering the netdev */
   netif_stop_queue(dev);

   wlan_cfg80211_unregister_netdevice(dev);
#if QCA_NSS_PLATFORM || QCA_PPE_VP
   osif_pltfrm_delete_ext_vap(osifp);
#endif
   qdf_net_if_release_dev((struct qdf_net_if *)dev);

   return 0;
}
qdf_export_symbol(osif_wds_ext_delete_netdev);

static osif_peer_dev *
osif_create_wds_ext_netdev_alloc(struct ieee80211_clone_params *cp, uint8_t multiq_support_enabled)
{
    struct net_device *dev = NULL;
    struct net_device *parent_dev = NULL;
    char name[IFNAMSIZ];
    char parent_dev_name[IFNAMSIZ] = {0};
    int count = 0, dev_exist;
    osif_peer_dev  *osifp = NULL;

    dev_exist = qdf_net_dev_exist_by_name(cp->icp_name);
    if (dev_exist) {
	    qdf_err("%s net dev exists already", cp->icp_name);
	    return NULL;
    }

    if (strlcpy(name, cp->icp_name, sizeof(name)) >=  sizeof(name)) {
	    qdf_err("source too long");
	    return NULL;
    }
    name[IFNAMSIZ - 1] = '\0';

    /* get parent interface */
    while(name[count] != '.') {
          parent_dev_name[count] = name[count];
          count++;
    }
    parent_dev = dev_get_by_name(&init_net, parent_dev_name);
    if (!parent_dev) {
        qdf_err("device %s not Found", parent_dev_name);
        return NULL;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
    if (multiq_support_enabled)
        dev = alloc_netdev_mqs(sizeof(osif_peer_dev), name, 0, ether_setup,
                OSIF_DP_NETDEV_TX_QUEUE_NUM, OSIF_DP_NETDEV_RX_QUEUE_NUM);
    else
        dev = alloc_netdev(sizeof(osif_peer_dev), name, 0, ether_setup);
#else
    if (multiq_support_enabled)
        dev = alloc_netdev_mqs(sizeof(osif_peer_dev), name, ether_setup,
                OSIF_DP_NETDEV_TX_QUEUE_NUM, OSIF_DP_NETDEV_RX_QUEUE_NUM);
    else
        dev = alloc_netdev(sizeof(osif_peer_dev), name, ether_setup);
#endif
    if(dev == NULL) {
        qdf_err("Failed to create netdevice for %s:%s", parent_dev_name, name);
        goto fail;
    }

    if (name != NULL)
        if (strlcpy(dev->name, name, sizeof(dev->name)) >= sizeof(dev->name)) {
            qdf_err("source too long");
            goto fail;
        }

    /*
    ** Clear and initialize the osif structure
    */
    osifp = ath_netdev_priv(dev);
    memset(osifp,0,sizeof(osif_peer_dev));
    osifp->netdev = dev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
    dev->max_mtu = ATH_MAX_MTU;
#endif

    /* holding a ref on parent since pointer is stored
     * and will be released when child dev is deleted
     */
    osifp->parent_netdev = parent_dev;
    osifp->parent_osif = ath_netdev_priv(parent_dev);

    return osifp;
fail:
    if (parent_dev)
        qdf_net_if_release_dev((struct qdf_net_if *)parent_dev);

    if (dev)
        free_netdev(dev);

    return NULL;
}

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
static struct wiphy *
osif_get_parent_wiphy_context(osif_dev *osdev)
{
#ifdef WLAN_FEATURE_11BE_MLO
    struct osif_mldev *mldev = NULL;

    if (osdev->dev_type == OSIF_NETDEV_TYPE_MLO) {
        mldev = (struct osif_mldev *)osdev;
        return mldev->wdev.wiphy;
    }
#endif

    return osdev->iv_wdev.wiphy;
}


struct net_device *osif_mld_phy_create_wds_ext_netdev(struct ieee80211_clone_params *cp)
{
    struct net_device *peer_ndev;
    osif_peer_dev  *osifp;
    int multiq_support_enabled = 1;
    osif_dev *parent_osif;

    osifp = osif_create_wds_ext_netdev_alloc(cp, multiq_support_enabled);
    if (osifp == NULL) {
        qdf_err("wds ext net dev allocation failed");
        return NULL;
    }
    osifp->dev_type = OSIF_NETDEV_TYPE_WDS_EXT;
    peer_ndev = osifp->netdev;
    /* In case if single wiphy parent osif will be
     * initilized during the change station
     */
    osifp->parent_osif = NULL;

    parent_osif = ath_netdev_priv(osifp->parent_netdev);

#if UMAC_SUPPORT_CFG80211
    osifp->wdev.wiphy = osif_get_parent_wiphy_context(parent_osif);
    osifp->wdev.netdev = peer_ndev;
    osifp->wdev.iftype = NL80211_IFTYPE_AP_VLAN;
    peer_ndev->ieee80211_ptr = &osifp->wdev;
#endif

#ifdef WLAN_FEATURE_FASTPATH
    osif_mld_phy_wds_ext_dev_ops.ndo_select_queue = osif_select_queue;
#endif

    peer_ndev->netdev_ops = &osif_mld_phy_wds_ext_dev_ops;
    peer_ndev->tx_queue_len = 0;          /* NB: bypass queueing */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
    peer_ndev->priv_flags |= IFF_NO_QUEUE;
#endif

    /* tx_csum, SG and TSO will be supported on wds ext interfaces */
    osif_dev_set_offload_tx_csum(peer_ndev);
    osif_dev_set_sg(peer_ndev);
    osif_dev_set_tso(peer_ndev);

    /*
     * Register the netdevice. All previous failures need to free the device.
     */
    if (wlan_cfg80211_register_netdevice(peer_ndev)) {
        qdf_err("%s: unable to register device\n", osifp->netdev->name);
        goto free_dev;
    }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
#if QCA_PPE_VP
    osif_pltfrm_alloc_ext_ppe_vp(osifp, peer_ndev);
#endif
#endif

    return osifp->netdev;
free_dev:
    if (osifp->parent_netdev)
        qdf_net_if_release_dev((struct qdf_net_if *)osifp->parent_netdev);

    free_netdev(peer_ndev);
    return NULL;
}

qdf_export_symbol(osif_mld_phy_create_wds_ext_netdev);
#endif

struct net_device *osif_create_wds_ext_netdev(struct ieee80211com *ic, struct ieee80211_clone_params *cp)
{
    struct net_device *peer_ndev;
    osif_peer_dev  *osifp;
    struct net_device *comdev = ic->ic_netdev;
    osif_dev *parent_osif;

    osifp = osif_create_wds_ext_netdev_alloc(cp, ic->multiq_support_enabled);
    if (osifp == NULL) {
        qdf_err("wds ext net dev allocation failed");
        return NULL;
    }
    osifp->dev_type = OSIF_NETDEV_TYPE_WDS_EXT;
    peer_ndev = osifp->netdev;

    /* Note: Set the dev features flag to comdev features
     * before qdf_net_vlan_attach - otherwise
     * VLAN flag settings in qdf_net_vlan_attach gets
     * overwritten
     */
    peer_ndev->features = comdev->features;

    parent_osif = ath_netdev_priv(osifp->parent_netdev);
    /* enable TSO/LRO only of HW Supports */
    if(parent_osif->osif_is_mode_offload) {
        if (ic->ic_offload_tx_csum_support) {
            osif_dev_set_offload_tx_csum(peer_ndev);
        }
        if (ic->ic_sg_support) {
            osif_dev_set_sg(peer_ndev);
        }
        if (ic->ic_tso_support) {
            osif_dev_set_tso(peer_ndev);
        }
    }

#if UMAC_SUPPORT_CFG80211
    osifp->wdev.wiphy = ic->ic_wdev.wiphy;
    osifp->wdev.netdev = peer_ndev;
    osifp->wdev.iftype = NL80211_IFTYPE_AP_VLAN;
    peer_ndev->ieee80211_ptr = &osifp->wdev;
#endif
    peer_ndev->netdev_ops = &osif_wds_ext_dev_ops;
    peer_ndev->tx_queue_len = 0;          /* NB: bypass queueing */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
    peer_ndev->priv_flags |= IFF_NO_QUEUE;
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    /*
     * Replace the netdev ops with the offlaod ops
     */
    if (!ic->nss_ext_vops) {
        goto register_netdev;
    }

    peer_ndev->netdev_ops = &osif_nss_wds_ext_vdev_ops;

    /*
     * Allocate the NSS interface.
     */
    osifp->nss_ifnum = ic->nss_ext_vops->ic_osif_nss_ext_vdev_alloc(osifp, ic);
    if (!NSS_IF_IS_TYPE_DYNAMIC(osifp->nss_ifnum)) {
	qdf_err("received invalid nss_ifnum(%d)", osifp->nss_ifnum);
        goto free_dev;
    }

    /*
     * Register the NSS interface. NSS dealloaction is taken care by the
     * register cb in case of failures. No need of external deallocation.
     */
    if (ic->nss_ext_vops->ic_osif_nss_ext_vdev_register(osifp, ic)) {
        qdf_err("NSS registration failed if_num(%d)", osifp->nss_ifnum);
        goto free_dev;
    }

register_netdev:
#endif /* QCA_NSS_WIFI_OFFLOAD_SUPPORT */

    /*
     * Register the netdevice. All previous failures need to free the device.
     */
    if (wlan_cfg80211_register_netdevice(peer_ndev)) {
        qdf_err("%s: unable to register device\n", osifp->netdev->name);
        goto free_dev;
    }

    return osifp->netdev;
free_dev:
    if (osifp->parent_netdev)
        qdf_net_if_release_dev((struct qdf_net_if *)osifp->parent_netdev);

    free_netdev(peer_ndev);
    return NULL;
}

qdf_export_symbol(osif_create_wds_ext_netdev);
#endif

static int
proc_iv_config_show(struct seq_file *m, void *v)
{
    struct ieee80211vap *vap = (struct ieee80211vap *) m->private;
    struct ieee80211com *ic = vap->iv_ic;
    uint32_t ldpc;
    uint32_t dtim_period;
    uint32_t rts_threshold;
    uint32_t frag_threshold;
    uint32_t inact_run;
    uint32_t nss;
#if QCA_LTEU_SUPPORT
    uint32_t scan_repeat_probe_time;
    uint32_t scan_probe_delay;
#endif
    uint32_t vhtbfsoundingdim;
    uint32_t vhtbfeestscap;
    uint32_t vhtsubfer;
    uint32_t vhtsubfee;
    uint32_t vhtmubfer;
    uint32_t vhtmubfee;
    uint32_t implicitbf;
#if WLAN_SUPPORT_MBSSSID
    uint32_t bssid_idx;
#endif
    uint32_t bcn_rate;
    uint32_t mgmt_rate;
    uint32_t amsdu_size;
    uint32_t ampdu_size;

    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_LDPC, &ldpc);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_DTIM_PERIOD, &dtim_period);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_NSS, &nss);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_SUBFER, &vhtsubfer);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_SUBFEE, &vhtsubfee);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_MUBFER, &vhtmubfer);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_MUBFEE, &vhtmubfee);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_IMLICIT_BF, &implicitbf);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_SOUNDING_DIM, &vhtbfsoundingdim);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_BFEE_STS_CAP, &vhtbfeestscap);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_RTS_THRESHOLD, &rts_threshold);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_FRAG_THRESHOLD, &frag_threshold);
#if QCA_LTEU_SUPPORT
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_PROBE_DELAY, &scan_probe_delay);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_REPEAT_PROBE_TIME, &scan_repeat_probe_time);
#endif
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_MAX_UNRESPONSIVE_INACTIVE_TIME, &inact_run);
#if WLAN_SUPPORT_MBSSSID
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_PROFILE_IDX, &bssid_idx);
#endif
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_BCN_TX_RATE, &bcn_rate);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_TX_MGMT_RATE, &mgmt_rate);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_AMPDU, &ampdu_size);
    ucfg_wlan_vdev_mgr_get_param(vap->vdev_obj,
            WLAN_MLME_CFG_AMSDU, &amsdu_size);

    seq_printf(m, "iv_myaddr: %s\n", ether_sprintf(vap->iv_myaddr));
    seq_printf(m, "iv_hw_addr: %s\n", ether_sprintf(wlan_vap_get_hw_macaddr(vap)));
    {
        int id = 0;
        wlan_objmgr_vdev_get_ref(vap->vdev_obj, WLAN_MLME_NB_ID);
        id = wlan_vdev_get_id(vap->vdev_obj);
        wlan_objmgr_vdev_release_ref(vap->vdev_obj, WLAN_MLME_NB_ID);
        seq_printf(m, "vap_id: %d\n", id);
    }
    seq_printf(m, "ic_my_hwaddr: %s\n", ether_sprintf(ic->ic_my_hwaddr));
    seq_printf(m, "ic_myaddr: %s\n", ether_sprintf(ic->ic_myaddr));

#define VAP_UINT_FIELD(x)   seq_printf(m, #x ": %u\n", vap->x)
#define VAP_UINT64_FIELD(x) seq_printf(m, #x ": %llu\n", vap->x)
#define VAP_ULONG_FIELD(x)  seq_printf(m, #x ": %lu\n", vap->x)
#define VAP_INT_FIELD(x)    seq_printf(m, #x ": %d\n", vap->x)

    VAP_UINT_FIELD(iv_unit);
    VAP_UINT_FIELD(iv_debug);
    VAP_UINT_FIELD(iv_opmode);
    VAP_UINT_FIELD(iv_scan_priority_base);
    VAP_UINT_FIELD(iv_create_flags);
    VAP_UINT_FIELD(iv_flags);
    VAP_UINT_FIELD(iv_flags_ext);
    VAP_UINT_FIELD(iv_flags_ext2);
    VAP_UINT_FIELD(iv_deleted);
    VAP_UINT_FIELD(iv_scanning);
    VAP_UINT_FIELD(iv_smps);
    VAP_UINT_FIELD(iv_standby);
    VAP_UINT_FIELD(iv_cansleep);
    VAP_UINT_FIELD(iv_sw_bmiss);
    VAP_UINT_FIELD(iv_copy_beacon);
    VAP_UINT_FIELD(iv_wapi);
    VAP_UINT_FIELD(iv_sta_fwd);
    VAP_UINT_FIELD(iv_dynamic_mimo_ps);
    VAP_UINT_FIELD(iv_doth);
    VAP_UINT_FIELD(iv_country_ie);
    VAP_UINT_FIELD(iv_wme);
    VAP_UINT_FIELD(iv_dfswait);
#if 0
    VAP_UINT_FIELD(iv_enable_when_dfs_clear);
#endif
    VAP_UINT_FIELD(iv_erpupdate);
    VAP_UINT_FIELD(iv_needs_scheduler);
    VAP_UINT_FIELD(iv_forced_sleep);
    VAP_UINT_FIELD(iv_qbssload);
    VAP_UINT_FIELD(iv_qbssload_update);
    VAP_UINT_FIELD(iv_rrm);
    VAP_UINT_FIELD(iv_wnm);
    VAP_UINT_FIELD(iv_proxyarp);
    VAP_UINT_FIELD(iv_dgaf_disable);
    VAP_UINT_FIELD(iv_ap_reject_dfs_chan);
    VAP_UINT_FIELD(iv_smartnet_enable);
    VAP_UINT_FIELD(iv_trigger_mlme_resp);
    VAP_UINT_FIELD(iv_mfp_test);
    VAP_UINT_FIELD(iv_sgi);
    VAP_UINT_FIELD(iv_he_sgi);
    VAP_UINT_FIELD(iv_data_sgi);
    VAP_UINT_FIELD(iv_he_data_sgi);
    seq_printf(m, "ldpc: %u\n", ldpc);
    VAP_UINT_FIELD(iv_ratemask_default);
    VAP_UINT_FIELD(iv_key_flag);
    VAP_UINT_FIELD(iv_list_scanning);
#if ATH_SUPPORT_ZERO_CAC_DFS
    VAP_UINT_FIELD(iv_pre_cac_timeout_channel_change);
#endif
    VAP_UINT_FIELD(iv_quick_reconnect);
    VAP_UINT_FIELD(iv_256qam);
    VAP_UINT_FIELD(iv_11ng_vht_interop);
    VAP_UINT_FIELD(iv_mbo);
    VAP_UINT_FIELD(iv_ext_acs_inprogress);
    VAP_UINT_FIELD(iv_send_additional_ies);
    VAP_UINT_FIELD(vlan_set_flags);
    VAP_UINT_FIELD(iv_rawmode_pkt_sim);


#if 0
    VAP_UINT_FIELD(iv_one_mb_enabled);
    VAP_UINT_FIELD(iv_two_mb_enabled);
    VAP_UINT_FIELD(iv_five_mb_enabled);
#endif


    VAP_UINT_FIELD(iv_caps);
    VAP_UINT_FIELD(iv_ath_cap);
    VAP_UINT_FIELD(iv_chanchange_count);
    VAP_UINT_FIELD(iv_mcast_rate);
    VAP_UINT_FIELD(iv_mcast_fixedrate);
    VAP_UINT_FIELD(iv_mcast_rate_config_defered);

    seq_printf(m, "iv_node_count: %d\n", wlan_vdev_get_peer_count(vap->vdev_obj));
    VAP_UINT_FIELD(iv_unicast_counterm.mic_count_in_60s);
    VAP_UINT_FIELD(iv_unicast_counterm.timestamp);
    VAP_UINT_FIELD(iv_multicast_counterm.mic_count_in_60s);
    VAP_UINT_FIELD(iv_multicast_counterm.timestamp);
    VAP_UINT_FIELD(iv_max_aid);
    VAP_UINT_FIELD(iv_sta_assoc);
    VAP_UINT_FIELD(iv_ps_sta);
    VAP_UINT_FIELD(iv_ps_pending);
#if 0
    VAP_UINT_FIELD(iv_bad_count_ps_pending_underflow);
    VAP_UINT_FIELD(iv_bad_count_tim_bitmap_empty);
#endif

    seq_printf(m, "dtim_period: %u\n", dtim_period);
    VAP_UINT_FIELD(iv_dtim_count);
    VAP_UINT_FIELD(iv_atim_window);
    VAP_UINT_FIELD(iv_tim_len);
    VAP_UINT_FIELD(iv_rescan);
    VAP_UINT_FIELD(iv_fixed_rateset);
    VAP_UINT_FIELD(iv_fixed_retryset);
    VAP_UINT_FIELD(iv_legacy_ratemasklower32);
    VAP_UINT_FIELD(iv_ht_ratemasklower32);
    VAP_UINT_FIELD(iv_vht_ratemasklower32);
    VAP_UINT_FIELD(iv_vht_ratemaskhigher32);
#if ATH_SUPPORT_AP_WDS_COMBO
    VAP_UINT_FIELD(iv_no_beacon);
#endif
    VAP_ULONG_FIELD(iv_assoc_comeback_time);
    VAP_UINT_FIELD(iv_skip_pmf_reassoc_to_hostap);
    VAP_UINT_FIELD(iv_rsn_override);

    seq_printf(m, "rtsthreshold: %u\n", rts_threshold);
    seq_printf(m, "fragthreshold: %u\n", frag_threshold);
    VAP_UINT_FIELD(iv_def_txkey);
    VAP_UINT_FIELD(iv_num_privacy_filters);
    VAP_UINT_FIELD(iv_pmkid_count);
    VAP_UINT_FIELD(iv_wps_mode);
    VAP_UINT_FIELD(iv_wep_mbssid);

    VAP_UINT_FIELD(iv_des_mode);
    VAP_UINT_FIELD(iv_des_hw_mode);
    VAP_UINT_FIELD(iv_des_modecaps);
    VAP_UINT_FIELD(iv_cur_mode);
    VAP_UINT_FIELD(iv_des_ibss_chan_freq);
    VAP_UINT_FIELD(iv_des_cfreq2_80_80);
    VAP_UINT_FIELD(iv_rateCtrlEnable);
    VAP_UINT_FIELD(iv_rc_txrate_fast_drop_en);
    VAP_UINT_FIELD(iv_des_nssid);
    seq_printf(m, "iv_conf_des_bssid: %s\n", ether_sprintf(vap->iv_conf_des_bssid));

    VAP_UINT_FIELD(iv_bmiss_count);
    VAP_UINT_FIELD(iv_bmiss_count_for_reset);
    VAP_UINT_FIELD(iv_bmiss_count_max);
    VAP_ULONG_FIELD(iv_last_beacon_time);
    VAP_ULONG_FIELD(iv_last_directed_frame);
    VAP_ULONG_FIELD(iv_last_ap_frame);
    VAP_ULONG_FIELD(iv_last_traffic_indication);
    VAP_ULONG_FIELD(iv_lastdata);
    VAP_UINT64_FIELD(iv_txrxbytes);
    VAP_UINT_FIELD(iv_lastbcn_phymode_mismatch);
    VAP_UINT_FIELD(iv_beacon_copy_len);
    VAP_UINT_FIELD(iv_mlmeconnect);
    seq_printf(m, "ampdu_size: %u\n", ampdu_size);
    seq_printf(m, "amsdu_size: %u\n", amsdu_size);
    VAP_UINT_FIELD(iv_ratedrop);
    VAP_UINT_FIELD(iv_assoc_denial_notify);
    VAP_UINT_FIELD(iv_protmode);
    VAP_UINT_FIELD(dyn_bw_rts);
#if ATH_SUPPORT_WRAP
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    VAP_UINT_FIELD(iv_nss_qwrap_en);
#endif
#endif
    VAP_UINT_FIELD(iv_beacon_info_count);
    VAP_UINT_FIELD(iv_esslen);
    seq_printf(m, "iv_essid: %s\n", ether_sprintf(vap->iv_essid));
    VAP_UINT_FIELD(iv_ibss_peer_count);
    VAP_UINT_FIELD(channel_change_done);
    VAP_UINT_FIELD(iebuf_updated);
    VAP_UINT_FIELD(iv_doth_updated);
    VAP_UINT_FIELD(mixed_encryption_mode);
    VAP_UINT_FIELD(iv_ena_vendor_ie);
    VAP_UINT_FIELD(iv_update_vendor_ie);
    VAP_UINT_FIELD(iv_mgt_rate_config_defered);
#if WDS_VENDOR_EXTENSION
    VAP_UINT_FIELD(iv_wds_rx_policy);
#endif
#ifdef ATH_SUPPORT_QUICK_KICKOUT
    VAP_UINT_FIELD(iv_sko_th);
#endif
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    VAP_UINT_FIELD(iv_chanutil_enab);
#endif
    VAP_UINT_FIELD(auto_assoc);
    VAP_UINT_FIELD(iv_inact_init);
    VAP_UINT_FIELD(iv_inact_auth);
    seq_printf(m, "inact_run: %u\n", inact_run);
    VAP_UINT_FIELD(iv_inact_probe);
    VAP_UINT_FIELD(iv_session);
    VAP_UINT_FIELD(iv_keep_alive_timeout);
    VAP_UINT_FIELD(iv_inact_count);
    VAP_UINT_FIELD(iv_smps_snrthresh);
    VAP_UINT_FIELD(iv_smps_datathresh);
#if ATH_SW_WOW
    VAP_UINT_FIELD(iv_sw_wow);
#endif
#ifdef ATH_SUPPORT_TxBF
    VAP_UINT_FIELD(iv_autocvupdate);
    VAP_UINT_FIELD(iv_cvupdateper);
#endif
    VAP_UINT_FIELD(iv_cswitch_rxd);
#if ATH_SUPPORT_WAPI
    VAP_UINT_FIELD(iv_wapi_urekey_pkts);
    VAP_UINT_FIELD(iv_wapi_mrekey_pkts);
#endif

#if ATH_SUPPORT_HS20
    seq_printf(m, "iv_hessid: %s\n", ether_sprintf(vap->iv_hessid));
    VAP_UINT_FIELD(iv_access_network_type);
    VAP_UINT_FIELD(iv_hotspot_xcaps);
    VAP_UINT_FIELD(iv_hotspot_xcaps2);
    VAP_UINT_FIELD(iv_hc_bssload);
    VAP_UINT_FIELD(iv_osen);
#endif
    VAP_UINT_FIELD(iv_wep_keycache);

#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
    VAP_UINT_FIELD(iv_rejoint_attemp_time);
#endif

    VAP_UINT_FIELD(iv_send_deauth);
#if UMAC_SUPPORT_WNM
    VAP_UINT_FIELD(iv_wnmsleep_intval);
    VAP_UINT_FIELD(iv_wnmsleep_force);
#endif
    VAP_UINT_FIELD(iv_is_started);
    VAP_UINT_FIELD(iv_is_up);

#if ATH_SUPPORT_HYFI_ENHANCEMENTS
    VAP_UINT_FIELD(iv_nopbn);
#endif

#if UMAC_PER_PACKET_DEBUG
    VAP_UINT_FIELD(iv_userrate);
    VAP_UINT_FIELD(iv_userretries);
    VAP_UINT_FIELD(iv_usertxpower);
    VAP_UINT_FIELD(iv_usertxchainmask);
#endif

    VAP_UINT_FIELD(iv_aponly);
    VAP_UINT_FIELD(iv_force_onetxchain);
    VAP_UINT_FIELD(iv_vht_fixed_mcs);
    VAP_UINT_FIELD(iv_he_fixed_mcs);
    seq_printf(m, "nss: %u\n", nss);
    VAP_UINT_FIELD(iv_tx_stbc);
    VAP_UINT_FIELD(iv_rx_stbc);
    VAP_UINT_FIELD(iv_opmode_notify);
    VAP_UINT_FIELD(iv_rtscts_enabled);
    VAP_UINT_FIELD(iv_rc_num_retries);
    VAP_UINT_FIELD(iv_bcast_fixedrate);
    VAP_UINT_FIELD(iv_bcast_rate_config_defered);
    VAP_UINT_FIELD(iv_vht_tx_mcsmap);
    VAP_UINT_FIELD(iv_vht_rx_mcsmap);
    VAP_UINT_FIELD(iv_disabled_legacy_rate_set);
    VAP_UINT_FIELD(iv_disable_htmcs);
    VAP_UINT_FIELD(iv_configured_vht_mcsmap);
    VAP_UINT_FIELD(iv_set_vht_mcsmap);
    VAP_UINT_FIELD(min_dwell_time_passive);
    VAP_UINT_FIELD(max_dwell_time_passive);

#if QCA_LTEU_SUPPORT
    seq_printf(m, "scan_repeat_probe_time: %u\n", scan_repeat_probe_time);
    VAP_UINT_FIELD(scan_rest_time);
    VAP_UINT_FIELD(scan_idle_time);
    seq_printf(m, "scan_probe_delay: %u\n", scan_probe_delay);
    VAP_UINT_FIELD(mu_start_delay);
    VAP_UINT_FIELD(wifi_tx_power);
#endif

    VAP_UINT_FIELD(iv_vht_sgimask);
    VAP_UINT_FIELD(iv_vht80_ratemask);

#if ATH_SUPPORT_DSCP_OVERRIDE
    VAP_UINT_FIELD(iv_dscp_map_id);
    VAP_UINT_FIELD(iv_vap_dscp_priority);
#endif
    VAP_UINT_FIELD(iv_pause_scan);

#if ATH_PERF_PWR_OFFLOAD
    VAP_UINT_FIELD(iv_tx_encap_type);
    VAP_UINT_FIELD(iv_rx_decap_type);
#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    VAP_UINT_FIELD(iv_rawmodesim_txaggr);
    VAP_UINT_FIELD(iv_rawmodesim_debug_level);
    VAP_UINT_FIELD(iv_fixed_frm_cnt_flag);
    VAP_INT_FIELD(iv_num_encap_frames);
    VAP_INT_FIELD(iv_num_decap_frames);
#endif
#endif
    VAP_UINT_FIELD(mhdr_len);

#if MESH_MODE_SUPPORT
    VAP_UINT_FIELD(iv_mesh_vap_mode);
#endif
    VAP_UINT_FIELD(iv_mesh_mgmt_txsend_config);
    VAP_UINT_FIELD(iv_special_vap_mode);
    VAP_UINT_FIELD(iv_special_vap_is_monitor);
    VAP_UINT_FIELD(iv_smart_monitor_vap);
    seq_printf(m, "vhtbfeestscap: %u\n", vhtbfeestscap);
    seq_printf(m, "vhtbfsoundingdim: %u\n", vhtbfsoundingdim);

    VAP_UINT_FIELD(iv_novap_reset);
    seq_printf(m, "mcast_encrypt_addr: %s\n", ether_sprintf(vap->mcast_encrypt_addr));
    VAP_UINT_FIELD(mac_entries);
    VAP_UINT_FIELD(rtt_enable);
    VAP_UINT_FIELD(lci_enable);
    VAP_UINT_FIELD(lcr_enable);
    VAP_UINT_FIELD(iv_smart_mesh_cfg);

#if MESH_MODE_SUPPORT
    VAP_UINT_FIELD(mhdr);
    VAP_UINT_FIELD(mdbg);
    seq_printf(m, "bssid_mesh: %s\n", ether_sprintf(vap->bssid_mesh));
    VAP_UINT_FIELD(iv_mesh_cap);
#endif

#if ATH_DATA_RX_INFO_EN
    VAP_UINT_FIELD(rxinfo_perpkt);
#endif

    VAP_UINT_FIELD(iv_cfg_assoc_war_160w);
    VAP_UINT_FIELD(iv_rev_sig_160w);
    VAP_UINT_FIELD(channel_switch_state);
    VAP_UINT_FIELD(iv_disable_ht_tx_amsdu);
    VAP_UINT_FIELD(iv_cts2self_prot_dtim_bcn);
    VAP_UINT_FIELD(iv_enable_vsp);
    VAP_UINT_FIELD(iv_cfg_raw_dwep_ind);

#if UMAC_SUPPORT_ACFG
    VAP_UINT_FIELD(iv_diag_warn_threshold);
    VAP_UINT_FIELD(iv_diag_err_threshold);
#endif

#if QCA_LTEU_SUPPORT
    VAP_UINT_FIELD(scan_probe_spacing_interval);
#endif

    VAP_UINT_FIELD(watermark_threshold);
    VAP_UINT_FIELD(watermark_threshold_reached);
    VAP_UINT_FIELD(watermark_threshold_flag);
    VAP_UINT_FIELD(assoc_high_watermark);
    VAP_UINT_FIELD(assoc_high_watermark_time);
    VAP_UINT_FIELD(iv_cfg_nstscap_war);
    seq_printf(m, "beacon rate: %d\n", bcn_rate);
    VAP_UINT_FIELD(iv_csmode);
    VAP_UINT_FIELD(iv_enable_ecsaie);
    VAP_UINT_FIELD(iv_ecsa_opclass);

#if DYNAMIC_BEACON_SUPPORT
    VAP_UINT_FIELD(iv_dbeacon_snr_thr);
    VAP_UINT_FIELD(iv_dbeacon_timeout);
#endif

#if UMAC_SUPPORT_CFG80211
    VAP_UINT_FIELD(iv_cfg80211_create);
#endif
    VAP_UINT_FIELD(iv_scan_band);

    /* country ie data */
    VAP_UINT64_FIELD(iv_country_ie_chanflags);

    /* U-APSD Settings */
    VAP_UINT_FIELD(iv_uapsd);
    VAP_UINT_FIELD(iv_wmm_enable);
    VAP_UINT_FIELD(iv_wmm_power_save);

    VAP_UINT_FIELD(iv_disable_HTProtection);
    VAP_UINT_FIELD(iv_ht40_intolerant);
    VAP_UINT_FIELD(iv_chwidth);
    VAP_UINT_FIELD(iv_chextoffset);
    VAP_UINT_FIELD(iv_chscaninit);

    VAP_UINT_FIELD(iv_ccmpsw_seldec);
    seq_printf(m, "mgmt rate: %u\n", mgmt_rate);
#ifdef ATH_SUPPORT_TxBF
    VAP_UINT_FIELD(iv_txbfmode);
#endif

    seq_printf(m, "vhtsubfer: %u\n", vhtsubfer);
    seq_printf(m, "vhtsubfee: %u\n", vhtsubfee);
    seq_printf(m, "vhtmubfer: %u\n", vhtmubfer);
    seq_printf(m, "vhtmubfee: %u\n", vhtmubfee);
    seq_printf(m, "implicitbf: %u\n", implicitbf);
    /* HE Parameters */
    VAP_UINT_FIELD(iv_he_ctrl);
    VAP_UINT_FIELD(iv_he_twtreq);
    VAP_UINT_FIELD(iv_he_twtres);
    VAP_UINT_FIELD(iv_he_frag);
    VAP_UINT_FIELD(iv_he_max_frag_msdu);
    VAP_UINT_FIELD(iv_he_min_frag_size);
    VAP_UINT_FIELD(iv_he_multi_tid_aggr);
    VAP_UINT_FIELD(iv_he_link_adapt);
    VAP_UINT_FIELD(iv_he_all_ack);
    VAP_UINT_FIELD(iv_he_ul_mu_sched);
    VAP_UINT_FIELD(iv_he_actrl_bsr);
    VAP_UINT_FIELD(iv_he_32bit_ba);
    VAP_UINT_FIELD(iv_he_mu_cascade);
    VAP_UINT_FIELD(iv_he_omi);
    VAP_UINT_FIELD(iv_he_ofdma_ra);
    VAP_UINT_FIELD(iv_he_amsdu_frag);
    VAP_UINT_FIELD(iv_he_flex_twt);
    VAP_UINT_FIELD(iv_he_relaxed_edca);
    VAP_UINT_FIELD(iv_he_spatial_reuse);
    VAP_UINT_FIELD(iv_he_multi_bss);
    VAP_UINT_FIELD(iv_he_ul_mumimo);
    VAP_UINT_FIELD(iv_he_ul_muofdma);
    VAP_UINT_FIELD(iv_he_dl_muofdma);
    VAP_UINT_FIELD(iv_he_dl_muofdma_bfer);
    VAP_UINT_FIELD(iv_he_twt_required);
    VAP_UINT_FIELD(iv_he_rts_threshold);
    VAP_UINT_FIELD(iv_he_extended_range);
    VAP_UINT_FIELD(iv_he_dcm);
    VAP_UINT_FIELD(iv_bcn_offload_enable);
    VAP_UINT_FIELD(iv_oce);
    VAP_UINT_FIELD(iv_he_su_bfee);
    VAP_UINT_FIELD(iv_he_su_bfer);
    VAP_UINT_FIELD(iv_he_mu_bfee);
    VAP_UINT_FIELD(iv_he_mu_bfer);
    VAP_UINT_FIELD(iv_he_tx_mcsnssmap);
    VAP_UINT_FIELD(iv_he_rx_mcsnssmap);
    VAP_UINT_FIELD(iv_he_default_pe_durn);
    VAP_UINT_FIELD(iv_he_ltf);
    VAP_UINT_FIELD(iv_he_ar_gi_ltf);
    VAP_UINT_FIELD(iv_ext_nss_support);
    VAP_UINT_FIELD(iv_csl_support);
    VAP_UINT_FIELD(iv_lite_monitor);
    VAP_UINT_FIELD(iv_remove_csa_ie);
    VAP_UINT_FIELD(iv_refuse_all_addbas);
    VAP_UINT_FIELD(iv_ba_buffer_size);
#if defined(WLAN_SUPPORT_TWT)
    VAP_UINT_FIELD(iv_twt_rsp);
#endif

#if ATH_ACL_SOFTBLOCKING
    VAP_UINT_FIELD(iv_softblock_wait_time);
    VAP_UINT_FIELD(iv_softblock_allow_time);
#endif
#if QCN_IE
    VAP_UINT_FIELD(iv_bpr_enable);
    VAP_UINT_FIELD(iv_bpr_delay);
    VAP_UINT_FIELD(iv_bpr_timer_start_count);
    VAP_UINT_FIELD(iv_bpr_timer_resize_count);
    VAP_UINT_FIELD(iv_bpr_callback_count);
    VAP_UINT_FIELD(iv_bpr_timer_cancel_count);
#endif

    VAP_UINT_FIELD(iv_htflags);
#ifdef QCA_OL_DMS_WAR
    VAP_UINT_FIELD(dms_amsdu_war);
#endif
    VAP_UINT64_FIELD(wmi_tx_mgmt);
    VAP_UINT64_FIELD(wmi_tx_mgmt_completions);
    VAP_UINT64_FIELD(wmi_tx_mgmt_sta);
    VAP_UINT64_FIELD(wmi_tx_mgmt_completions_sta);
    VAP_UINT_FIELD(iv_read_rxprehdr);
#if DYNAMIC_BEACON_SUPPORT
    VAP_UINT_FIELD(iv_dbeacon);
    VAP_UINT_FIELD(iv_dbeacon_runtime);
#endif
    VAP_UINT_FIELD(iv_sm_gap_ps);
    VAP_UINT_FIELD(iv_static_mimo_ps);

#if WLAN_SUPPORT_MBSSSID
    seq_printf(m, "bssid_idx: %u\n", bssid_idx);
#endif
#ifdef WLAN_FEATURE_11BE
    VAP_UINT_FIELD(iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0]);
    VAP_UINT_FIELD(iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX1]);
    VAP_UINT_FIELD(iv_eht_tx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX2]);
    VAP_UINT_FIELD(iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX0]);
    VAP_UINT_FIELD(iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX1]);
    VAP_UINT_FIELD(iv_eht_rx_mcsnssmap[EHTCAP_TXRX_MCS_NSS_IDX2]);
    VAP_UINT_FIELD(iv_eht_ltf);
    seq_printf(m, "eht_su_bfme: %u\n", vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfme);
    seq_printf(m, "eht_su_bfmr: %u\n", vap->vdev_mlme->proto.eht_cap_info.eht_phycap_subfmr);
    VAP_UINT_FIELD(iv_eht_mu_bfee);
    seq_printf(m, "eht_mu_bfmr: %u\n", vap->vdev_mlme->proto.eht_cap_info.eht_phycap_mubfmrlt80mhz);
#endif /* WLAN_FEATURE_11BE */
    VAP_UINT_FIELD(iv_diff_mode);
    VAP_UINT_FIELD(iv_spl_vap_scan);
    VAP_UINT_FIELD(iv_repurpose);

    return 0;
}

int osif_print_scan_config(wlan_if_t vaphandle, struct seq_file *m)
{
    const char*sep;
    struct ieee80211com *ic = vaphandle->iv_ic;
    struct chan_list *chan_list;
    uint8_t ch;
    uint8_t band;
    int i;
    char s = ieee80211_get_vap_mode(vaphandle);

    seq_printf(m, "scanning: %d\n", ic->scan_params.scan_ev_completed ? 1 : 0);
    seq_printf(m, "channels: ");
    sep = "";

    chan_list = qdf_mem_malloc(sizeof(*chan_list));

    if (!chan_list) {
        qdf_err("chan_list is NULL");
        return -ENOMEM;
    }

    qdf_mem_copy(chan_list, &ic->scan_params.chan_list, sizeof(*chan_list));

    for (i = 0; i < chan_list->num_chan; i++) {
        ch = wlan_reg_freq_to_chan(ic->ic_pdev_obj, chan_list->chan[i].freq);
        band = wlan_reg_freq_to_band(chan_list->chan[i].freq);
        seq_printf(m, "%s%u:%u %c", sep, ch, band, s);
        sep = " ";
    }
    seq_printf(m, "\n");
    seq_printf(m, "flags: %s%s%s%s%s%s%s\n",
               (ic->scan_params.scan_f_passive) ? "passive " : "",
               (ic->scan_params.scan_f_2ghz) ? "2GHz " : "",
               (ic->scan_params.scan_f_5ghz) ? "5GHz " : "",
               (ic->scan_params.scan_f_wide_band) ? "allbands " : "",
               (ic->scan_params.scan_f_continue_on_err) ? "continuous " : "",
               (ic->scan_params.scan_f_forced) ? "forced " : "",
               (ic->scan_params.scan_f_bcast_probe) ? "bcast_probe " : "");

    seq_printf(m,
               "max rest time: %u\n"
               "min rest time: %u\n"
               "idle time: %u\n"
               "max scan time: %u\n"
               "dwell on active: %u\n"
               "dwell on passive: %u\n",
               ic->scan_params.max_rest_time,
               ic->scan_params.min_rest_time,
               ic->scan_params.idle_time,
               ic->scan_params.max_scan_time,
               ic->scan_params.dwell_time_active,
               ic->scan_params.dwell_time_passive);

    qdf_mem_free(chan_list);
    return 0;
}



static int
proc_scan_config_show(struct seq_file *m, void *v)
{
    struct ieee80211vap *vap = (struct ieee80211vap *) m->private;

    osif_print_scan_config(vap,m);
    seq_printf(m, "bg_scan_enabled: %d\n", (vap->iv_flags & IEEE80211_F_BGSCAN) ? 1 : 0);
    return 0;
}

static
void proc_clients_rates_show_iter_cb(void *arg, struct ieee80211_node *ni)
{
    struct seq_file *m = (struct seq_file *)arg;
    struct ieee80211_rateset *rs= &(ni->ni_rates);
    int i;

    seq_printf(m, "entry mac=%s\n", ether_sprintf(ni->ni_macaddr));
    seq_printf(m, "rate_info:\n");

    for (i = 0; i < (rs->rs_nrates); i++)
        seq_printf(m, " RateIndex[%d], %d\n", i,
                   rs->rs_rates[i] & IEEE80211_RATE_VAL);
}

static int
proc_clients_rates_show(struct seq_file *m, void *v)
{
    struct ieee80211vap *vap = (struct ieee80211vap *) (((uintptr_t) m->private) & ~0x3);

    IEEE80211_VAP_LOCK(vap);
    wlan_iterate_all_sta_list(vap, proc_clients_rates_show_iter_cb, (void *)m);
    IEEE80211_VAP_UNLOCK(vap);

    return 0;
}

struct osif_proc_info {
    const char *name;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
    struct proc_ops *ops;
#else
    struct file_operations *ops;
#endif
    int extra;
};

#define PROC_FOO(func_base, extra) { #func_base, &proc_ieee80211_##func_base##_ops, extra }

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define GENERATE_PROC_STRUCTS(func_base)				\
	static int proc_##func_base##_open(struct inode *inode, \
					   struct file *file)		\
	{								\
		struct proc_dir_entry *dp = PDE(inode);			\
		return single_open(file, proc_##func_base##_show, dp->data); \
	}								\
									\
	static struct file_operations proc_ieee80211_##func_base##_ops = { \
		.open		= proc_##func_base##_open,	\
		.read		= seq_read,				\
		.llseek		= seq_lseek,				\
		.release	= single_release,			\
	};
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
#define GENERATE_PROC_STRUCTS(func_base)				\
	static int proc_##func_base##_open(struct inode *inode, \
					   struct file *file)		\
	{								\
        return single_open(file, proc_##func_base##_show, pde_data(inode)); \
    } \
									\
									\
	static struct proc_ops proc_ieee80211_##func_base##_ops = { \
		.proc_open		= proc_##func_base##_open,	\
		.proc_read		= seq_read,			\
		.proc_lseek		= seq_lseek,			\
		.proc_release		= single_release,		\
	};
#else
#define GENERATE_PROC_STRUCTS(func_base)                                \
        static int proc_##func_base##_open(struct inode *inode, \
                                           struct file *file)           \
        {                                                               \
        return single_open(file, proc_##func_base##_show, PDE_DATA(inode)); \
    } \
                                                                        \
                                                                        \
        static struct file_operations proc_ieee80211_##func_base##_ops = { \
                .open           = proc_##func_base##_open,      \
                .read           = seq_read,                             \
                .llseek         = seq_lseek,                            \
                .release        = single_release,                       \
        };
#endif
#endif

GENERATE_PROC_STRUCTS(clients_keys);
GENERATE_PROC_STRUCTS(clients_rates);
GENERATE_PROC_STRUCTS(iv_config);
GENERATE_PROC_STRUCTS(scan_config);

const struct osif_proc_info osif_proc_infos[] = {
    PROC_FOO(clients_keys, 0),
    PROC_FOO(clients_rates, 0),
    PROC_FOO(iv_config, 0),
    PROC_FOO(scan_config, 0),
};

#define NUM_PROC_INFOS (sizeof(osif_proc_infos) / sizeof(osif_proc_infos[0]))

void
ieee80211_proc_vattach(struct ieee80211vap *vap)
{
    char *devname = NULL;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    char link_id[3];
    char nlink[5] = "link";
    struct proc_dir_entry *parent_proc = NULL;
#endif

    /*
     * Reserve space for the device name outside the net_device structure
     * so that if the name changes we know what it used to be.
     */
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if(((osif_dev *)vap->iv_ifp)->mldev) {
        /*
         * if mld_proc itself failed to create, avoid creating link proc
         */
        if (!((osif_dev *)vap->iv_ifp)->mldev->mld_proc)
            return;

        snprintf(link_id, 3, "%d", vap->link_id);
        devname = qdf_mem_malloc((strlen(nlink) + strlen(link_id) + 1) * sizeof(char));

        if (devname == NULL) { 
            qdf_warn(KERN_ERR "%s: no memory for VAP name!", __func__);
            return;
        }
        if (strlcpy(devname, nlink, strlen(nlink) + 1) >= strlen(nlink) + 1) {
            qdf_err("source too long");
            qdf_mem_free(devname);
            return ;
        }
        if (strlcat(devname, link_id, ((strlen(nlink) + strlen(link_id) + 1) * sizeof(char))) >= ((strlen(nlink) + strlen(link_id) + 1) * sizeof(char))) {
            qdf_err("source to long");
            qdf_mem_free(devname);
            return ;
        }

    } else if (((osif_dev *)vap->iv_ifp)->netdev)
#endif
    {
        devname = qdf_mem_malloc((strlen(((osif_dev *)vap->iv_ifp)->netdev->name) + 1) * sizeof(char));

        if (devname == NULL) {
            qdf_warn(KERN_ERR "%s: no memory for VAP name!", __func__);
            return;
        }

        if (strlcpy(devname, ((osif_dev *)vap->iv_ifp)->netdev->name,
            strlen(((osif_dev *)vap->iv_ifp)->netdev->name) + 1) >= strlen(((osif_dev *)vap->iv_ifp)->netdev->name) + 1) {
            qdf_err("source too long");
            qdf_mem_free(devname);
            return ;
        }
    }   

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if(((osif_dev *)vap->iv_ifp)->mldev) {
        /* create procfs for link vap */
        parent_proc = ((osif_dev *)vap->iv_ifp)->mldev->mld_proc;
        vap->iv_proc = proc_mkdir(devname, parent_proc);
    } else if (((osif_dev *)vap->iv_ifp)->netdev)
#endif
    {
        /* create procfs for legacy vap */
        vap->iv_proc = proc_mkdir(devname, NULL);
    }

    if (vap->iv_proc) {
      	int i;
        for (i = 0; i < NUM_PROC_INFOS; ++i) {
            struct proc_dir_entry *entry;
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)))
            entry = proc_create_data(osif_proc_infos[i].name, 0644, vap->iv_proc,
                        (const struct file_operations *) osif_proc_infos[i].ops, (void *)vap);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0))
            entry = proc_create_data(osif_proc_infos[i].name, 0644, vap->iv_proc,
                                     osif_proc_infos[i].ops, (void *)vap);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
            entry = proc_create_data(osif_proc_infos[i].name, 0644, vap->iv_proc,
                                      (const struct proc_ops *) osif_proc_infos[i].ops, (void *)vap);
#else
            entry = create_proc_entry(osif_proc_infos[i].name,
                                       0644, vap->iv_proc);
#endif
            if (entry == NULL) {
                qdf_print(KERN_ERR "%s: Proc Entry creation failed!", __func__);
                goto entry_creation_failed;
            }

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
            entry->data = vap;
            entry->proc_fops = osif_proc_infos[i].ops;
#endif
        }
    }
entry_creation_failed:
    qdf_mem_free(devname);

}

void
ieee80211_proc_vdetach(struct ieee80211vap *vap)
{
    if (vap->iv_proc) {
        int i;
        for (i = 0; i < NUM_PROC_INFOS; ++i)
            remove_proc_entry(osif_proc_infos[i].name, vap->iv_proc);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
        remove_proc_entry(vap->iv_proc->name, vap->iv_proc->parent);
#else
        proc_remove(vap->iv_proc);
#endif
        vap->iv_proc = NULL;

    }
}

int
ath_get_radio_index(struct net_device *netdev)
{
    if (!qdf_mem_cmp(netdev->name, "wifi0", 5)) {
        return 0;
    } else if (!qdf_mem_cmp(netdev->name, "wifi1", 5)) {
        return 1;
    } else if (!qdf_mem_cmp(netdev->name, "wifi2", 5)) {
        return 2;
    } else {
        qdf_print("Error: Could not match the device name : %s", netdev->name);
        return -1;
    }
}
qdf_export_symbol(ath_get_radio_index);
int
asf_adf_attach(void)
{
    static int first = 1;

    /*
     * Configure the shared asf_print instances with ADF
     * function pointers for print.
     * (Do this initialization just once.)
     */
    if (first) {
        first = 0;

        qdf_spinlock_create(&asf_print_lock);
        asf_print_setup(
            (asf_vprint_fp)qdf_vprint,
            (asf_print_lock_fp)qdf_spin_lock_bh,
            (asf_print_unlock_fp)qdf_spin_unlock_bh,
            &asf_print_lock);
    }

    return EOK;
}
qdf_export_symbol(asf_adf_attach);

#ifndef REMOVE_PKT_LOG
struct ath_pktlog_funcs *g_pktlog_funcs = NULL;
struct ath_pktlog_rcfuncs *g_pktlog_rcfuncs = NULL;
struct ol_pl_os_dep_funcs *g_ol_pl_os_dep_funcs = NULL;

qdf_export_symbol(g_pktlog_funcs);
qdf_export_symbol(g_pktlog_rcfuncs);
qdf_export_symbol(g_ol_pl_os_dep_funcs);

void (*g_osif_hal_ani)(void *) = NULL;
/*
 * ath_pktlog module passes the ani func here.
 * This func is passed to HAL using the callback registered by qca_da in the below func.
 */
void osif_pktlog_log_ani_callback_register(void *pktlog_ani)
{
    if (g_osif_hal_ani) {
        g_osif_hal_ani(pktlog_ani);
    }
}
qdf_export_symbol(osif_pktlog_log_ani_callback_register);

/* qca_da module registers the HAL function here */
void osif_hal_log_ani_callback_register(void *hal_ani)
{
    g_osif_hal_ani = hal_ani;
}
qdf_export_symbol(osif_hal_log_ani_callback_register);
#endif

bool ieee80211_is_6g_valid_rate(struct ieee80211vap *vap, u_int32_t rate)
{
    if(IEEE80211_IS_CHAN_6GHZ(vap->iv_ic->ic_curchan) &&
            ((rate >= IEEE80211_HE_6GHZ_RATE_LOW_THRES) &&
             (rate <= IEEE80211_HE_6GHZ_RATE_HIGH_THRES))) {
        /* Allow HE rate to be set when AP is operating in 6GHz band.
         */
        return 1;
    }

    return 0;
}
qdf_export_symbol(ieee80211_is_6g_valid_rate);

int
ieee80211_rate_is_valid_basic(struct ieee80211vap *vap, u_int32_t rate)
{
    enum ieee80211_phymode mode = wlan_get_desired_phymode(vap);
    struct ieee80211_rateset *op_rs = &(vap->iv_op_rates[mode]);
    int i;
    int rs_rate,basic_rate;

    if(ieee80211_is_6g_valid_rate(vap, rate)) {
        return 1;
    } else {
        /* Find all basic rates */
        for (i=0; i < op_rs->rs_nrates; i++) {
            rs_rate = op_rs->rs_rates[i];
            /* Added this check for 11ng. Since 6,12 and 24 Mbps rates are not showing as basic rate as per the current code*/
            if (vap->iv_disabled_legacy_rate_set) {
                basic_rate = rs_rate & IEEE80211_RATE_VAL;
            } else {
                basic_rate = rs_rate & IEEE80211_RATE_BASIC;
            }
            if (basic_rate) {
                if (rate == ((rs_rate & IEEE80211_RATE_VAL) * 1000)/2) {
                    return 1;
                }
            }
        }
    }

    return 0;
}
qdf_export_symbol(ieee80211_rate_is_valid_basic);

#if ATH_SUPPORT_WRAP && DBDC_REPEATER_SUPPORT
void osif_set_primary_radio_event (struct ieee80211com *ic)
{
    osif_dev *osdev;
    struct net_device *dev;
    union iwreq_data wreq;
    wlan_if_t apvap;
    if (!wlan_rptr_pdev_is_qwrap(ic->ic_pdev_obj)) {
       return;
    }
    apvap = wlan_get_vap(dp_wrap_get_vdev(ic->ic_pdev_obj));
    if (!apvap) {
       return;
    }
    if (!osif_scn_event_filter_status_to_app(ic, EVENT_IWEVCUSTOM,
                IEEE80211_EV_PRIMARY_RADIO_CHANGED)) {
        return;
    }
    osdev = (osif_dev *)wlan_vap_get_registered_handle(apvap);
    dev = OSIF_TO_NETDEV(osdev);
    memset(&wreq, 0, sizeof(wreq));
    wreq.data.flags = IEEE80211_EV_PRIMARY_RADIO_CHANGED;
    WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, NULL);
}
#endif

int wlan_is_ap_cac_timer_running (struct ieee80211com *ic)
{
    struct wlan_objmgr_pdev *pdev;
    pdev = ic->ic_pdev_obj;
    return mlme_dfs_is_ap_cac_timer_running(pdev);
}

#if UMAC_SUPPORT_CFG80211
static enum nl80211_chan_width ieee80211_csa_get_nl_width(
        int width_mhz,
        struct ieee80211_ie_sec_chan_offset *secchanoffsetie)
{
    switch (width_mhz) {
    case 40:
        return NL80211_CHAN_WIDTH_40;
    case 80:
        return NL80211_CHAN_WIDTH_80;
    case 160:
    case 320:
        return NL80211_CHAN_WIDTH_160;
    default:
        if (secchanoffsetie) {
            switch (secchanoffsetie->sec_chan_offset) {
            case IEEE80211_SEC_CHAN_OFFSET_SCN:
                return NL80211_CHAN_WIDTH_20;
            case IEEE80211_SEC_CHAN_OFFSET_SCA:
            case IEEE80211_SEC_CHAN_OFFSET_SCB:
                return NL80211_CHAN_WIDTH_40;
            default:
                break;
            }
        }
    }

    return NL80211_CHAN_WIDTH_20_NOHT;
}

static enum nl80211_band ieee80211_csa_get_nl_band(uint16_t freq)
{
    enum reg_wifi_band band;

    band = wlan_reg_freq_to_band(freq);

    switch (band) {
    case REG_BAND_2G:
        return IEEE80211_BAND_2GHZ;
    case REG_BAND_5G:
        return IEEE80211_BAND_5GHZ;
    case REG_BAND_6G:
        return IEEE80211_BAND_6GHZ;
    default:
        return IEEE80211_BAND_2GHZ;
    }
}

static int ieee80211_csa_get_nl_center_freq(
        enum nl80211_band band, int freq_mhz,
        struct ieee80211_ie_wide_bw_switch *widebwie,
        struct ieee80211_ie_sec_chan_offset *secchanoffsetie)
{
    if (widebwie) {
        switch (band) {
        case IEEE80211_BAND_2GHZ:
            return 2407 + (widebwie->new_ch_freq_seg1 * 5);
        case IEEE80211_BAND_5GHZ:
            return 5000 + (widebwie->new_ch_freq_seg1 * 5);
        case IEEE80211_BAND_6GHZ:
            return 5950 + (widebwie->new_ch_freq_seg1 * 5);
        default:
            break;
        }
    }

    if (secchanoffsetie) {
        if (!freq_mhz)
            return 0;

        switch (secchanoffsetie->sec_chan_offset) {
        case IEEE80211_SEC_CHAN_OFFSET_SCN:
            return freq_mhz;
        case IEEE80211_SEC_CHAN_OFFSET_SCA:
            return freq_mhz + 10;
        case IEEE80211_SEC_CHAN_OFFSET_SCB:
            return freq_mhz - 10;
        default:
            break;
        }
    }

    return freq_mhz;
}

void ieee80211_update_width_and_puncture_bitmap(
        struct ieee80211vap *vap, enum ieee80211_cwm_width dest_chwidth,
        uint32_t *width, struct chan_params *chan_p)
{
    switch(dest_chwidth) {
        case IEEE80211_CWM_WIDTH20:
            *width = 20;
            break;
        case IEEE80211_CWM_WIDTH40:
            *width = 40;
            break;
        case IEEE80211_CWM_WIDTH80:
            *width = 80;
            break;
        case IEEE80211_CWM_WIDTH160:
            *width = 160;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_CWM_WIDTH320:
            *width = 320;
            break;
#endif
        default:
            *width = 20;
            break;
    }

#ifdef WLAN_FEATURE_11BE
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
    vap->chan_notify.chandef_notify.puncture_bitmap = chan_p->puncture_bitmap;
#else
    vap->chan_notify.punct_bitmap = chan_p->puncture_bitmap;
#endif
#endif
}

void ieee80211_notify_channel_switch_started(
        struct ieee80211_node *ni,
        struct ieee80211_ath_channelswitch_ie *chanie,
        struct ieee80211_extendedchannelswitch_ie *echanie,
        struct ieee80211_ie_sec_chan_offset *secchanoffsetie,
        struct ieee80211_ie_wide_bw_switch *widebwie,
        uint32_t width, u_int8_t *cswrp)
{
    struct net_device *dev;
    struct ieee80211com *ic;
    struct ieee80211_channel *nl_chan;
    struct ieee80211vap *vap = ni->ni_vap;
    uint8_t chan_num = 0;
    uint8_t secondary_chan = 0;
    struct chan_params chan_p = {0};
    enum ieee80211_cwm_width dest_chwidth = 0;
    QDF_STATUS status;
    uint8_t ccfs0 = 0;
    enum ieee80211_mode mode = IEEE80211_MODE_INVALID;

    if (!vap) {
        qdf_err("null vap");
        return;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("null ic");
        return;
    }

    nl_chan = &vap->channel_started_notify;
    qdf_mem_zero(nl_chan, sizeof(*nl_chan));
    qdf_mem_zero(&vap->chan_notify, sizeof(struct channel_notify));

    if (echanie) {
        chan_num = echanie->newchannel;
        vap->cfg80211_channel_started_count = echanie->tbttcount;
    } else if (chanie) {
        chan_num = chanie->newchannel;
        vap->cfg80211_channel_started_count = chanie->tbttcount;
    }

    if (echanie &&
        wlan_reg_is_6ghz_op_class(ic->ic_pdev_obj, echanie->newClass)) {
        nl_chan->center_freq = wlan_reg_chan_opclass_to_freq_auto(chan_num,
                                                                  echanie->newClass,
                                                                  false);
    } else {
        nl_chan->center_freq = wlan_reg_legacy_chan_to_freq(ic->ic_pdev_obj,
                                                            chan_num);
    }

    nl_chan->band = ieee80211_csa_get_nl_band(nl_chan->center_freq);
    vap->chan_notify.chandef_notify.chan = nl_chan;

    mode = ieee80211_csa_get_mode(ni, echanie, chan_num);
    if (mode == IEEE80211_MODE_INVALID) {
        /* No valid mode found. */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_DOTH,
                          "%s : Invalid current mode\n", __func__);
        return;
    }

    chan_p.freq = nl_chan->center_freq;
    status = ieee80211_parse_csa_bw_ind_ie(ic, cswrp, echanie, &chan_p,
                                           &secondary_chan, &dest_chwidth,
                                           &ccfs0, mode);
    if (QDF_IS_STATUS_SUCCESS(status)) {
        /* CSA channel is punctured, derive the chwidth and cfreq1 from BW
         * indication element.
         */
        ieee80211_update_width_and_puncture_bitmap(vap, dest_chwidth, &width,
                                                   &chan_p);
        switch (nl_chan->band) {
        case IEEE80211_BAND_2GHZ:
            vap->chan_notify.chandef_notify.center_freq1 = 2407 + (ccfs0 * 5);
            break;
        case IEEE80211_BAND_5GHZ:
            vap->chan_notify.chandef_notify.center_freq1 = 5000 + (ccfs0 * 5);
            break;
        case IEEE80211_BAND_6GHZ:
            vap->chan_notify.chandef_notify.center_freq1 = 5950 + (ccfs0 * 5);
            break;
        default:
            vap->chan_notify.chandef_notify.center_freq1 = 0;
            break;
        }
    } else {
        /* CSA channel is not punctured, derive the cfreq1 from wideband IE or
         * secondary channel offset IE.
         */
        vap->chan_notify.chandef_notify.center_freq1 =
            ieee80211_csa_get_nl_center_freq(nl_chan->band,
                                             nl_chan->center_freq, widebwie,
                                             secchanoffsetie);
    }

    vap->chan_notify.chandef_notify.width = ieee80211_csa_get_nl_width(width,
                                                                       secchanoffsetie);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_DOTH,
            "%s: chan:%d center_freq:%d band:%d width:%d cfreq1:%d tbtt:%d",
            __func__,
            chan_num, nl_chan->center_freq, nl_chan->band,
            vap->chan_notify.chandef_notify.width,
            vap->chan_notify.chandef_notify.center_freq1,
            vap->cfg80211_channel_started_count);

    qdf_sched_work(dev, &vap->cfg80211_channel_started_notify_workq);
}
#else
void ieee80211_notify_channel_switch_started(
        struct ieee80211_node *ni,
        struct ieee80211_ath_channelswitch_ie *chanie,
        struct ieee80211_extendedchannelswitch_ie *echanie,
        struct ieee80211_ie_sec_chan_offset *secchanoffsetie,
        struct ieee80211_ie_wide_bw_switch *widebwie,
        uint32_t width, u_int8_t *cswrp)
{
}
#endif

void ieee80211_mgmt_sta_send_csa_rx_nl_msg(
        struct ieee80211com                         *ic,
        struct ieee80211_node                       *ni,
        struct ieee80211_ath_channel                *chan,
        struct ieee80211_ath_channelswitch_ie       *chanie,
        struct ieee80211_extendedchannelswitch_ie   *echanie,
        struct ieee80211_ie_sec_chan_offset         *secchanoffsetie,
        struct ieee80211_ie_wide_bw_switch          *widebwie,
        u_int8_t *cswrp)
{
    struct ieee80211_csa_rx_ev csa_rx_ev = {0};
    union iwreq_data wreq = {{0}};
    struct net_device *dev;
    uint8_t tbttcount = 0;
    wlan_if_t vap = ni->ni_vap;
    dev = ((osif_dev *)vap->iv_ifp)->netdev;

    memcpy(csa_rx_ev.bssid, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
    csa_rx_ev.valid = !!(IEEE80211_IS_CHAN_VALID(chan));

    if (chanie) {
        csa_rx_ev.chan = chanie->newchannel;
        tbttcount = chanie->tbttcount;
    } else if (echanie) {
        csa_rx_ev.chan = echanie->newchannel;
        tbttcount = echanie->tbttcount;
    }

    if (widebwie) {
        csa_rx_ev.cfreq2_mhz = widebwie->new_ch_freq_seg2;
        if (widebwie->new_ch_width == IEEE80211_VHTOP_CHWIDTH_2040) {
            csa_rx_ev.width_mhz = 40;
            if (widebwie->new_ch_freq_seg1 > csa_rx_ev.chan)
                csa_rx_ev.secondary = 1;
            else
                csa_rx_ev.secondary = -1;
        } else if ((widebwie->new_ch_width == IEEE80211_VHTOP_CHWIDTH_80) ||
                   (widebwie->new_ch_width == IEEE80211_VHTOP_CHWIDTH_REVSIG_160) ||
                   (widebwie->new_ch_width == IEEE80211_VHTOP_CHWIDTH_REVSIG_80_80)) {
            if (widebwie->new_ch_freq_seg2 == 0) {
                csa_rx_ev.width_mhz = 80;
            } else if ((widebwie->new_ch_freq_seg2 > 0) &&
                    abs(widebwie->new_ch_freq_seg2 - widebwie->new_ch_freq_seg1) > 16) {
                csa_rx_ev.width_mhz = 160;
            } else if ((widebwie->new_ch_freq_seg2 > 0) &&
                    abs(widebwie->new_ch_freq_seg2 - widebwie->new_ch_freq_seg1) == 8) {
                csa_rx_ev.width_mhz = 160;
            }
        } else if ((widebwie->new_ch_width == IEEE80211_VHTOP_CHWIDTH_160) ||
                   (widebwie->new_ch_width == IEEE80211_VHTOP_CHWIDTH_80_80)) {
            csa_rx_ev.width_mhz = 160;
#ifdef WLAN_FEATURE_11BE
        } else if (widebwie->new_ch_width == IEEE80211_CHWIDTH_320) {
            csa_rx_ev.width_mhz = 320;
#endif
        } else {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_SCANENTRY,
                    "%s : Invalid wideband channel width %d specified\n",
                    __func__, widebwie->new_ch_width);
            return;
        }
    }

    if (secchanoffsetie) {
        switch (secchanoffsetie->sec_chan_offset) {
            case IEEE80211_SEC_CHAN_OFFSET_SCN:
                csa_rx_ev.secondary = 0;
                break;
            case IEEE80211_SEC_CHAN_OFFSET_SCA:
                csa_rx_ev.secondary = 1;
                break;
            case IEEE80211_SEC_CHAN_OFFSET_SCB:
                csa_rx_ev.secondary = -1;
                break;
        }
    }

    if (osif_vap_event_filter_status_to_app(vap, EVENT_IWEVCUSTOM, IEEE80211_EV_CSA_RX)) {
        wreq.data.flags = IEEE80211_EV_CSA_RX;
        wreq.data.length = sizeof(csa_rx_ev);

        if (!dev){
            qdf_err("dev is null");
            return;
        }
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, (void *)&csa_rx_ev);
    }

    qdf_info("ifname=%s valid=%d chan=%d width=%d sec=%d cfreq2=%d tbtt=%u\n",
            dev->name,
            csa_rx_ev.valid,
            csa_rx_ev.chan,
            csa_rx_ev.width_mhz,
            csa_rx_ev.secondary,
            csa_rx_ev.cfreq2_mhz, tbttcount);

    ieee80211_notify_channel_switch_started(ni, chanie, echanie,
                                            secchanoffsetie, widebwie,
                                            csa_rx_ev.width_mhz, cswrp);
}

static QDF_STATUS umac_debug_panic_handler(void *data)
{
	wlan_mlme_print_all_sm_history();
	return QDF_STATUS_SUCCESS;
}

static void ic_cancel_obss_nb_ru_tolerance_timer(struct ieee80211com *ic)
{
    qdf_timer_sync_cancel(&ic->ic_obss_nb_ru_tolerance_timer);

    return;
}

#if QCA_MANUAL_TRIGGERED_ULOFDMA
/* sizeof struct wmi_host_rx_peer_userinfo_evt_data + 40 bytes reserved */
#define MAX_M_ULOFDMA_SUBBUF_SIZE 400
/* max 50 records of manual ulofdma trigger */
#define MAX_M_ULOFDMA_N_SUBBUFS 50
/**
 * ol_manual_trig_ulofdma_get_dev_name() - Get net device name from pdev
 * @pdev: objmgr pdev
 *
 * Return: netdev name
 */
static char*
ol_manual_trig_ulofdma_get_dev_name(struct wlan_objmgr_pdev *pdev)
{
    struct pdev_osif_priv *pdev_ospriv;
    struct qdf_net_if *nif;

    pdev_ospriv = wlan_pdev_get_ospriv(pdev);
    if (!pdev_ospriv) {
        qdf_err("pdev_ospriv is NULL");
        return NULL;
    }

    nif = pdev_ospriv->nif;
    if (!nif) {
        qdf_err("pdev nif is NULL");
        return NULL;
    }

    return  qdf_net_if_get_devname(nif);
}

/**
 * ol_manual_trig_ulofdma_streamfs_init() - manual ulofdma streamfs init
 * @pdev: the physical device object.
 *
 * Return : QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS
ol_manual_trig_ulofdma_streamfs_init(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    ol_man_trig_ulofdma_ctx *ulofdma_ctx = NULL;
    char *devname;
    char folder[32];

    if (!pdev) {
        qdf_err("PDEV is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (!scn->sc_mulofdma_support) {
        qdf_info("mulofdma is not supported:%d",scn->sc_mulofdma_support);
        return QDF_STATUS_E_FAILURE;
    }

    ulofdma_ctx = &scn->ulofdma_ctx;

    devname = ol_manual_trig_ulofdma_get_dev_name(pdev);
    if (!devname) {
        qdf_err("devname is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    snprintf(folder, sizeof(folder), "mulofdma_%s", devname);

    ulofdma_ctx->dir_ptr = qdf_streamfs_create_dir((const char *)folder, NULL);

    if (!ulofdma_ctx->dir_ptr) {
        qdf_err("Directory create failed");
        return QDF_STATUS_E_FAILURE;
    }

    ulofdma_ctx->subbuf_size = MAX_M_ULOFDMA_SUBBUF_SIZE;
    ulofdma_ctx->num_subbufs = MAX_M_ULOFDMA_N_SUBBUFS;

    ulofdma_ctx->chan_ptr = qdf_streamfs_open("data", ulofdma_ctx->dir_ptr,
                     ulofdma_ctx->subbuf_size,
                     ulofdma_ctx->num_subbufs, NULL);

   if (!ulofdma_ctx->chan_ptr) {
        qdf_err("Chan create failed");
        qdf_streamfs_remove_dir_recursive(ulofdma_ctx->dir_ptr);
        ulofdma_ctx->dir_ptr = NULL;
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ol_manual_trig_ulofdma_streamfs_init);

/**
 * ol_manual_trig_ulofdma_streamfs_remove() - manual ulofdma streamfs remove
 * @pdev: the physical device object.
 *
 * Return : QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS
ol_manual_trig_ulofdma_streamfs_remove(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    ol_man_trig_ulofdma_ctx *ulofdma_ctx = NULL;

    if (!pdev) {
        qdf_err("PDEV is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (!scn->sc_mulofdma_support) {
        qdf_info("mulofdma is not supported:%d",scn->sc_mulofdma_support);
        return QDF_STATUS_E_FAILURE;
    }

    ulofdma_ctx  = &scn->ulofdma_ctx;

    if (ulofdma_ctx) {
        if (ulofdma_ctx->chan_ptr) {
            qdf_streamfs_close(ulofdma_ctx->chan_ptr);
            ulofdma_ctx->chan_ptr = NULL;
        }

        if (ulofdma_ctx->dir_ptr) {
            qdf_streamfs_remove_dir_recursive(ulofdma_ctx->dir_ptr);
            ulofdma_ctx->dir_ptr = NULL;
        }

    } else {
        qdf_err("ulofdma_ctx is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ol_manual_trig_ulofdma_streamfs_remove);

/**
 * ol_manual_trig_ulofdma_streamfs_write() - manual ulofdma streamfs write
 * @ulofdma_ctx: ulofdma ctx
 * @write_data: ulofdma data
 * @write_len: ulofdma data len
 *
 * Return : QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS
ol_manual_trig_ulofdma_streamfs_write(ol_man_trig_ulofdma_ctx *ulofdma_ctx,
                                   const void *write_data, size_t write_len)
{
    if (ulofdma_ctx->chan_ptr) {
    /* write to channel buffer */
    qdf_streamfs_write(ulofdma_ctx->chan_ptr, (const void *)write_data,
                       write_len);
    } else
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ol_manual_trig_ulofdma_streamfs_write);

/**
 * ol_manual_trig_ulofdma_streamfs_flush() - manual ulofdma streamfs flush
 * @ulofdma_ctx: ulofdma ctx
 *
 * Return : QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS
ol_manual_trig_ulofdma_streamfs_flush(ol_man_trig_ulofdma_ctx *ulofdma_ctx)
{
    if (ulofdma_ctx->chan_ptr) {

    /* Flush the data write to channel buffer */
        qdf_streamfs_flush(ulofdma_ctx->chan_ptr);
    } else
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ol_manual_trig_ulofdma_streamfs_flush);

/**
 * ol_manual_trig_ulofdma_streamfs_send() - manual ulofdma streamfs send
 * @ulofdma_ctx: ulofdma ctx
 * @comm_info: Pointer to common info
 * @comm_info_len: Common info length
 * @peer_info: Pointer to peer info
 * @comm_info_len: Peer info length
 *
 * Return : QDF_STATUS_SUCCESS or QDF_STATUS_E_FAILURE
 */
QDF_STATUS
ol_manual_trig_ulofdma_streamfs_send(ol_man_trig_ulofdma_ctx *ulofdma_ctx,
        const void *comm_info, size_t comm_info_len,
        const void *peer_info, size_t peer_info_len)
{
    if (comm_info && peer_info) {
        if (QDF_IS_STATUS_ERROR(ol_manual_trig_ulofdma_streamfs_write(ulofdma_ctx,
                                comm_info, comm_info_len))) {
            qdf_err("comm info relay write failed");
            return QDF_STATUS_E_FAILURE;
        }
        if (QDF_IS_STATUS_ERROR(ol_manual_trig_ulofdma_streamfs_write(ulofdma_ctx,
                                peer_info, peer_info_len))) {
            qdf_err("peer info relay write failed");
            return QDF_STATUS_E_FAILURE;
        }
        if (QDF_IS_STATUS_ERROR(ol_manual_trig_ulofdma_streamfs_flush(ulofdma_ctx))) {
            qdf_err("relay flush failed");
            return QDF_STATUS_E_FAILURE;
        }
    } else {
            qdf_err("comm_info or peer_info is NULL");
            return QDF_STATUS_E_FAILURE;
    }
    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ol_manual_trig_ulofdma_streamfs_send);
#endif /* QCA_MANUAL_TRIGGERED_ULOFDMA */

#if ATH_SUPPORT_WRAP
osif_dev *
wlan_get_osdev(struct wlan_objmgr_vdev *vdev) {
    struct vdev_osif_priv *vdev_osifp = NULL;
    osif_dev *osifp = NULL;

    if (qdf_unlikely(!vdev))
        return NULL;

    vdev_osifp = wlan_vdev_get_ospriv(vdev);
    if (qdf_unlikely(!vdev_osifp))
        return NULL;

    osifp = (osif_dev *)vdev_osifp->legacy_osif_priv;
    return osifp;
}
qdf_export_symbol(wlan_get_osdev);

struct ieee80211vap *
wlan_get_vap(struct wlan_objmgr_vdev *vdev) {
    struct vdev_osif_priv *vdev_osifp = NULL;
    osif_dev *osifp = NULL;

    if (qdf_unlikely(!vdev))
        return NULL;
    vdev_osifp = wlan_vdev_get_ospriv(vdev);
    if (qdf_unlikely(!vdev_osifp))
        return NULL;

    osifp = (osif_dev *)vdev_osifp->legacy_osif_priv;
    return osifp->os_if;
}
qdf_export_symbol(wlan_get_vap);
#endif

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef QCA_SUPPORT_WDS_EXTENDED
static struct wlan_objmgr_peer *
osif_mlo_find_non_assoc_link_peer(struct wlan_mlo_peer_context *ml_peer,
                                  struct wlan_objmgr_peer *assoc_peer)
{
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_peer *non_assoc_peer = NULL;
    struct wlan_objmgr_peer *link_peer = NULL;
    int i = 0;

    if (!ml_peer)
        return NULL;

    mlo_peer_lock_acquire(ml_peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry)
            continue;

        link_peer = peer_entry->link_peer;
        if (!link_peer)
            continue;

        if (assoc_peer == link_peer)
            continue;

        if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE)
            continue;

        non_assoc_peer = link_peer;
        break;
    }
    mlo_peer_lock_release(ml_peer);

    return non_assoc_peer;
}

static void osif_mlo_bridge_peer_migrate(struct wlan_objmgr_vdev *vdev,
                                         void *object, void *arg)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_objmgr_peer *assoc_peer = NULL;
    struct wlan_objmgr_peer *bridge_peer = NULL;
    struct wlan_objmgr_peer *non_assoc_peer = NULL;
    struct wlan_objmgr_vdev *new_primary_vdev = NULL;
    struct ieee80211vap *primary_vap = NULL;
    osif_peer_dev *osif_peer = NULL;

    if (!wlan_peer_is_mlo(peer) || !peer->mlo_peer_ctx)
        return;

    ml_peer = peer->mlo_peer_ctx;
    if (ml_peer->link_peer_cnt == 1)
        return;

    if (ml_peer->primary_umac_migration_in_progress) {
        qdf_err("peer " QDF_MAC_ADDR_FMT " primary umac migration already in progress",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes));
        return;
    }

    bridge_peer = wlan_mlo_peer_get_bridge_peer(ml_peer);
    if (!bridge_peer)
        return;

    if (!wlan_vdev_mlme_is_mlo_bridge_vdev(wlan_peer_get_vdev(bridge_peer)))
        return;

    /* check whether peer is WDS capable or not */
    cdp_wds_ext_get_peer_osif_handle(wlan_psoc_get_dp_handle(wlan_vdev_get_psoc(vdev)),
                                     wlan_vdev_get_id(vdev), peer->macaddr,
                                     (ol_osif_peer_handle *)&osif_peer);
    if (!osif_peer)
        return;

    assoc_peer = wlan_mlo_peer_get_assoc_peer(ml_peer);
    if (!assoc_peer)
        return;

    /* link removal is on non assoc link */
    if (assoc_peer != peer)
        return;

    non_assoc_peer = osif_mlo_find_non_assoc_link_peer(ml_peer, assoc_peer);
    if (!non_assoc_peer) {
        qdf_err("Unable the find the next avaliable link");
        return;
    }

    new_primary_vdev = wlan_peer_get_vdev(non_assoc_peer);
    if (!new_primary_vdev) {
        qdf_err("vdev is NULL in selected link");
        return;
    }

    primary_vap = wlan_vdev_get_vap(new_primary_vdev);
    if (!primary_vap) {
        qdf_err("vap is NULL in selected link");
        return;
    }

    /* switch wds ext interface to non assoc link*/
    wlan_cfg80211_wds_ext_peer_learn(primary_vap, ml_peer->peer_mld_addr.bytes);
}

QDF_STATUS
osif_mlo_bridge_peer_wds_ext_migrate(struct ieee80211vap *vap)
{
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;

    if (!vdev) {
        qdf_err("Vdev is NULL");
        return QDF_STATUS_E_INVAL;
    }

    wlan_objmgr_iterate_peerobj_list(vdev,
                                     osif_mlo_bridge_peer_migrate,
                                     NULL, WLAN_MLME_NB_ID);
    return QDF_STATUS_SUCCESS;
}
#endif

static inline int osif_log2_ceil(unsigned int value)
{
    unsigned int count = value;
    int log2 = -1;

    while (count) {
        log2++;
        count >>= 1;
    }
    if (1 << log2 != value)
        log2++;

    return log2;
}

QDF_STATUS osif_mlo_peer_find_hash_attach(struct osif_mldev *mldev, int hash_elems)
{
    int i, log2;

    if (!mldev)
        return QDF_STATUS_E_INVAL;

    log2 = osif_log2_ceil(hash_elems);
    hash_elems = 1 << log2;

    mldev->peer_hash.mask = hash_elems - 1;
    mldev->peer_hash.idx_bits = log2;

    /* allocate an array of TAILQ peer object lists */
    mldev->peer_hash.bins = qdf_mem_malloc(
    hash_elems * sizeof(TAILQ_HEAD(anonymous_tail_q, osifp_peer)));
    if (!mldev->peer_hash.bins)
        return QDF_STATUS_E_NOMEM;

    for (i = 0; i < hash_elems; i++)
         TAILQ_INIT(&mldev->peer_hash.bins[i]);

    qdf_spinlock_create(&mldev->peer_hash_lock);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS osif_mlo_peer_find_hash_detach(struct osif_mldev *mldev)
{
    uint32_t index = 0;
    struct osifp_peer *peer = NULL;
    struct osifp_peer *peer_next = NULL;

    if (!mldev)
        return QDF_STATUS_E_INVAL;

    if (!mldev->peer_hash.bins)
        return QDF_STATUS_E_INVAL;

    for (index = 0; index <= mldev->peer_hash.mask; index++) {
        if (!TAILQ_EMPTY(&mldev->peer_hash.bins[index])) {
            TAILQ_FOREACH_SAFE(peer, &mldev->peer_hash.bins[index],
                               hash_list_elem, peer_next) {
                if (!peer)
                    continue;

                TAILQ_REMOVE(&mldev->peer_hash.bins[index], peer, hash_list_elem);
                qdf_mem_free(peer);
            }
        }
    }

    qdf_mem_free(mldev->peer_hash.bins);
    mldev->peer_hash.bins = NULL;
    qdf_spinlock_destroy(&mldev->peer_hash_lock);

    return QDF_STATUS_SUCCESS;
}

static inline unsigned
osif_peer_find_hash_index(struct osif_mldev *mldev,
                          union osif_align_mac_addr *mac_addr)
{
    uint32_t index;

    index = mac_addr->align2.bytes_ab ^
            mac_addr->align2.bytes_cd ^
            mac_addr->align2.bytes_ef;

    index ^= index >> mldev->peer_hash.idx_bits;
    index &= mldev->peer_hash.mask;
    return index;
}

static QDF_STATUS
osifp_peer_find_hash_add(struct osif_mldev *mldev,
                         struct osifp_peer *peer)
{
    uint32_t index;
    union osif_align_mac_addr mac_addr_aligned;

    if (!mldev->peer_hash.bins)
        return QDF_STATUS_E_FAILURE;

    qdf_mem_copy(&mac_addr_aligned.raw[0],
                 &peer->mac_addr, QDF_MAC_ADDR_SIZE);

    index = osif_peer_find_hash_index(mldev, &mac_addr_aligned);
    qdf_spin_lock_bh(&mldev->peer_hash_lock);
    TAILQ_INSERT_HEAD(&mldev->peer_hash.bins[index], peer,
                      hash_list_elem);
    qdf_spin_unlock_bh(&mldev->peer_hash_lock);
    return QDF_STATUS_SUCCESS;
}

static void
osifp_peer_find_hash_remove(struct osif_mldev *mldev,
                            uint8_t *mac_addr)
{
    uint32_t index;
    int found = 0;
    struct osifp_peer *peer = NULL;
    struct osifp_peer *found_peer = NULL;
    union osif_align_mac_addr mac_addr_aligned;

    if (!mldev->peer_hash.bins)
        return;

    qdf_mem_copy(&mac_addr_aligned.raw[0],
                 mac_addr, QDF_MAC_ADDR_SIZE);

    index = osif_peer_find_hash_index(mldev, &mac_addr_aligned);

    QDF_ASSERT(!TAILQ_EMPTY(&mldev->peer_hash.bins[index]));
    qdf_spin_lock_bh(&mldev->peer_hash_lock);

    /*
     * As new peer is added at the HEAD of the list at delete peer need
     * to be deleted from tail
     *
     * find the last peer in the list with matching mac address and
     * delete
     */
    TAILQ_FOREACH(peer, &mldev->peer_hash.bins[index], hash_list_elem) {
        if(qdf_mem_cmp(mac_addr, peer->mac_addr, QDF_MAC_ADDR_SIZE) == 0) {
            found = 1;
            found_peer = peer;
            continue;
        }
    }
    if (found && found_peer) {
        TAILQ_REMOVE(&mldev->peer_hash.bins[index], found_peer, hash_list_elem);
        qdf_mem_free(found_peer);
    } else {
        QDF_ASSERT(1);
    }

    qdf_spin_unlock_bh(&mldev->peer_hash_lock);
}

QDF_STATUS
osifp_peer_primary_umac_update(struct osif_mldev *mldev,
                               struct cdp_txrx_peer_params_update *params,
                               bool is_bridge_peer)
{
    struct osifp_peer *peer = NULL;
    union osif_align_mac_addr mac_addr_aligned;
    uint32_t index;

    if (!mldev->peer_hash.bins)
        return QDF_STATUS_E_FAILURE;

    qdf_mem_copy(&mac_addr_aligned.raw[0], params->peer_mac, QDF_MAC_ADDR_SIZE);
    index = osif_peer_find_hash_index(mldev, &mac_addr_aligned);
    qdf_spin_lock_bh(&mldev->peer_hash_lock);
    TAILQ_FOREACH(peer, &mldev->peer_hash.bins[index], hash_list_elem) {
        if ((qdf_mem_cmp(params->peer_mac, peer->mac_addr, QDF_MAC_ADDR_SIZE) == 0) &&
             (peer->primary_chip_id == params->old_chip_id) &&
             (peer->pdev_id == params->old_pdev_id) &&
             (peer->vdev_id == params->old_vdev_id)) {

            peer->primary_chip_id = params->chip_id;
            peer->pdev_id = params->pdev_id;
            peer->vdev_id = params->vdev_id;
            peer->is_bridge_peer = is_bridge_peer;

            qdf_spin_unlock_bh(&mldev->peer_hash_lock);
            return QDF_STATUS_SUCCESS;
        }
    }
    qdf_warn("peer not found for peer mac" QDF_MAC_ADDR_FMT,
             QDF_MAC_ADDR_REF(params->peer_mac));
    qdf_spin_unlock_bh(&mldev->peer_hash_lock);
    return QDF_STATUS_E_FAILURE;
}

qdf_export_symbol(osifp_peer_primary_umac_update);

static inline
osif_dev *osif_get_osdev_from_vdev(struct wlan_objmgr_vdev *vdev)
{
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);

    if (!vap)
        return NULL;

     return (osif_dev *)vap->iv_ifp;
}

void
osif_process_txrx_peer_create_event(void *pdev_hdl, enum WDI_EVENT event,
                                    void *data, u_int16_t peer_id,
                                    uint32_t status)
{
    osif_dev  *osifp = NULL;
    struct osifp_peer *peer = NULL;
    struct cdp_txrx_peer_params_update *param = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct wlan_objmgr_vdev *vdev = NULL;

    param = (struct cdp_txrx_peer_params_update *)data;
    if (!pdev || !param)
        return;

    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, param->vdev_id, WLAN_OSIF_ID);
    if (!vdev)
        return;

    osifp = osif_get_osdev_from_vdev(vdev);
    if (!osifp || !osifp->mldev) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
        return;
    }

    peer = (struct osifp_peer *) qdf_mem_malloc(sizeof(struct osifp_peer));
    qdf_mem_copy(&peer->mac_addr, param->peer_mac, QDF_MAC_ADDR_SIZE);
    peer->primary_chip_id = param->chip_id;
    peer->pdev_id = param->pdev_id;
    peer->logically_deleted = false;
    peer->is_bridge_peer = osifp->is_bridge_link;
    peer->vdev_id = param->vdev_id;

    if (osifp_peer_find_hash_add(osifp->mldev, peer) != QDF_STATUS_SUCCESS)
        qdf_mem_free(peer);

    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
}

qdf_export_symbol(osif_process_txrx_peer_create_event);

void
osif_process_peer_delete_event(void *pdev_hdl, enum WDI_EVENT event,
                               void *data, u_int16_t peer_id,
                               uint32_t status)
{
    osif_dev  *osifp = NULL;
    struct cdp_txrx_peer_params_update *param = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct wlan_objmgr_vdev *vdev = NULL;

    param = (struct cdp_txrx_peer_params_update *)data;
    if (!pdev || !param)
        return;

    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, param->vdev_id, WLAN_OSIF_ID);
    if (!vdev)
        return;

    osifp = osif_get_osdev_from_vdev(vdev);
    if (!osifp || !osifp->mldev) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
        return;
    }

    osifp_peer_find_hash_remove(osifp->mldev, param->peer_mac);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
}

qdf_export_symbol(osif_process_peer_delete_event);

void
osif_process_peer_unmap_event(void *pdev_hdl, enum WDI_EVENT event,
                                void *data, u_int16_t peer_id,
                                uint32_t status)
{
    osif_dev  *osifp = NULL;
    struct cdp_txrx_peer_params_update *param = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct osif_mldev *mldev = NULL;
    union osif_align_mac_addr mac_addr_aligned;
    uint32_t index;
    struct osifp_peer *peer = NULL;

    param = (struct cdp_txrx_peer_params_update *)data;
    if (!pdev || !param)
        return;

    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, param->vdev_id, WLAN_OSIF_ID);
    if (!vdev)
        return;

    osifp = osif_get_osdev_from_vdev(vdev);
    if (!osifp || !osifp->mldev) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
        return;
    }

    if (!mldev || !mldev->peer_hash.bins) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
        return;
    }

    qdf_mem_copy(&mac_addr_aligned.raw[0], param->peer_mac, QDF_MAC_ADDR_SIZE);
    index = osif_peer_find_hash_index(mldev, &mac_addr_aligned);
    qdf_spin_lock_bh(&mldev->peer_hash_lock);
    TAILQ_FOREACH(peer, &mldev->peer_hash.bins[index], hash_list_elem) {
        if ((qdf_mem_cmp(param->peer_mac, peer->mac_addr, QDF_MAC_ADDR_SIZE) == 0) &&
            (param->chip_id == peer->primary_chip_id) &&
            (param->pdev_id == peer->pdev_id) &&
            osifp->is_bridge_link == peer->is_bridge_peer) {

            peer->logically_deleted = true;
            qdf_spin_unlock_bh(&mldev->peer_hash_lock);
            wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
            return;
        }
    }
    qdf_warn("stale unmap even for peer" QDF_MAC_ADDR_FMT,
             QDF_MAC_ADDR_REF(param->peer_mac));

    qdf_spin_unlock_bh(&mldev->peer_hash_lock);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
}

qdf_export_symbol(osif_process_peer_unmap_event);

void osif_update_raw_mld_param(osif_dev *osdev)
{
    int i = 0;
    int j = 0;

    if (osdev->mldev) {
        for (i = 0; i < MAX_MLO_CHIPS; i++) {
            for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
                osif_dev  *osifp = NULL;
                wlan_if_t vap = NULL;

                osifp = osdev->mldev->link_dev[i][j];
                if (!osifp)
                    continue;

                vap = osifp->os_if;
                if (!vap)
                    continue;

                if ((vap->iv_tx_encap_type == osif_pkt_type_raw) ||
                    (vap->iv_tx_encap_type == osif_pkt_type_native_wifi)) {
                    osdev->mldev->is_raw_mld = 1;
                    return;
                }
            }
        }
    }
}

qdf_export_symbol(osif_update_raw_mld_param);

#if MESH_MODE_SUPPORT
void osif_update_mesh_mld_param(osif_dev *osdev)
{
    int i = 0;
    int j = 0;

    if (osdev->mldev) {
        for (i = 0; i < MAX_MLO_CHIPS; i++) {
            for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
                osif_dev  *osifp = NULL;
                wlan_if_t vap = NULL;

                osifp = osdev->mldev->link_dev[i][j];
                if (!osifp)
                    continue;

                vap = osifp->os_if;
                if (!vap)
                    continue;
                if (vap->iv_mesh_vap_mode) {
		    osdev->mldev->is_mesh_mld = 1;
                    return;
		}
           }
        }
    }
}
#endif

void
osif_process_primary_umac_update_event(void *pdev_hdl,
                                       enum WDI_EVENT event,
                                       void *data,
                                       u_int16_t peer_id,
                                       uint32_t status)
{
    osif_dev  *osifp = NULL;
    struct cdp_txrx_peer_params_update *param = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;

    param = (struct cdp_txrx_peer_params_update *)data;
    if (!pdev || !param)
        return;

    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, param->vdev_id, WLAN_OSIF_ID);
    if (!vdev)
        return;

    osifp = osif_get_osdev_from_vdev(vdev);
    if (!osifp || !osifp->mldev) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
        return;
    }

    osifp->mldev->primary_chip_id = param->chip_id;
    osifp->mldev->primary_pdev_id = param->pdev_id;
    osif_update_raw_mld_param(osifp);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
#if MESH_MODE_SUPPORT
    osif_update_mesh_mld_param(osifp);
#endif
}

qdf_export_symbol(osif_process_primary_umac_update_event);

osif_dev *
osifp_peer_find_hash_find_osdev(struct osif_mldev *mldev,
                                uint8_t *mac_addr)
{
    struct osifp_peer *peer = NULL;
    uint32_t index;
    osif_dev  *osifp = NULL;
    union osif_align_mac_addr mac_addr_aligned;

    if (!mldev)
        return NULL;

    if (!mldev->peer_hash.bins)
        return NULL;

    qdf_mem_copy(&mac_addr_aligned.raw[0], mac_addr, QDF_MAC_ADDR_SIZE);
    index = osif_peer_find_hash_index(mldev, &mac_addr_aligned);
    qdf_spin_lock_bh(&mldev->peer_hash_lock);
    TAILQ_FOREACH(peer, &mldev->peer_hash.bins[index], hash_list_elem) {
        if (qdf_mem_cmp(mac_addr, peer->mac_addr, QDF_MAC_ADDR_SIZE) == 0) {
            if (peer->logically_deleted)
                break;

            if (peer->is_bridge_peer)
                osifp = mldev->bridge_dev[peer->primary_chip_id][peer->pdev_id];
            else
                osifp = mldev->link_dev[peer->primary_chip_id][peer->pdev_id];

            qdf_spin_unlock_bh(&mldev->peer_hash_lock);
            if (!osifp)
                return NULL;

            return osifp;
        }
    }
    qdf_spin_unlock_bh(&mldev->peer_hash_lock);
    return NULL;
}

qdf_export_symbol(osifp_peer_find_hash_find_osdev);

/*
 * osifp_peer_find_hash_find_netdev:
 * This function tries to find peer specific link netdev by searching in mld hash
 * @mldev : pointer to mldev context.
 * @mac_addr : mac address of the peer
 *
 * returns link netdev/NULL on success/fail
 */
struct net_device *
osifp_peer_find_hash_find_netdev(struct osif_mldev *mldev,
                                 uint8_t *mac_addr)
{
    osif_dev  *osifp = NULL;

    osifp = osifp_peer_find_hash_find_osdev(mldev, mac_addr);
    if (!osifp)
        return NULL;

    return osifp->netdev;
}

/*
 * Handles multicast frames in mesh and raw mode, In MLO case  multicast
 * packet will send to all the links
 */
static int osif_ap_mlo_special_mcast_handler(struct sk_buff *skb, struct osif_mldev *mldev)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < MAX_MLO_CHIPS; i++) {
        for (j = 0; j < MAX_PDEV_PER_SOC; j++) {
            osif_dev  *osdev = NULL;
            wlan_if_t vap = NULL;
            struct sk_buff *nbuf_clone = NULL;

            osdev = mldev->link_dev[i][j];
            if (!osdev)
                continue;

            vap = osdev->os_if;
            if (!vap)
                continue;

            nbuf_clone = qdf_nbuf_clone(skb);
            if (qdf_unlikely(!nbuf_clone))
                continue;

            qdf_nbuf_set_tx_fctx_type(nbuf_clone, (void *)0, CB_FTYPE_MLO_MCAST);
            nbuf_debug_del_record(nbuf_clone);

            if (osdev->netdev) {
                nbuf_clone->dev = osdev->netdev;
                dev_queue_xmit(nbuf_clone);
            } else {
              if (qdf_likely(osdev->mld_xmit))
                  osdev->mld_xmit(nbuf_clone, osdev);
            }
        }
    }
    /* cloned buffers will be sent, free the original buffer here */
    qdf_nbuf_free(skb);
    return 0;
}

#ifndef QCA_SUPPORT_RAWMODE_PKT_SIMULATION
static int osif_ap_mlo_raw_xmit(struct sk_buff *skb, struct osif_mldev *mldev)
{
    struct net_device *link_dev = NULL;
    struct ieee80211_frame *wh = NULL;
    osif_dev *link_osif = NULL;

    wh = (struct ieee80211_frame *)skb->data;
    if (qdf_unlikely(IEEE80211_IS_MULTICAST(wh->i_addr1))) {
        return osif_ap_mlo_special_mcast_handler(skb, mldev);
    } else {
        link_osif = osifp_peer_find_hash_find_osdev(mldev, wh->i_addr1);
        if (qdf_unlikely(!link_osif))
            goto fail;

        link_dev = link_osif->netdev;
        if (qdf_unlikely(link_dev && netif_queue_stopped(link_dev)))
            goto fail;

#ifndef ENABLE_CFG80211_BACKPORTS_MLO
        if (qdf_unlikely(!link_dev && !link_osif->is_bridge_link))
            goto fail;
#endif

        if (qdf_likely(link_osif->mld_xmit)) {
            nbuf_debug_del_record(skb);
            return link_osif->mld_xmit(skb, link_osif);
        }
    }

fail:
    if (mldev)
        mldev->pkt_fail_cnt++;

    qdf_nbuf_free(skb);
    return 0;
}
#endif

#if MESH_MODE_SUPPORT
int osif_ap_mlo_mesh_xmit(struct sk_buff *skb, struct osif_mldev *mldev)
{
    struct net_device *link_dev = NULL;
    struct ether_header *eh = NULL;
    osif_dev *link_osif = NULL;

    eh = (struct ether_header *)skb->data;
    if (qdf_unlikely(IEEE80211_IS_MULTICAST(eh->ether_dhost))) {
        return osif_ap_mlo_special_mcast_handler(skb, mldev);
     } else {
        link_osif = osifp_peer_find_hash_find_osdev(mldev, eh->ether_dhost);
        if (qdf_unlikely(!link_osif))
            goto fail;

        link_dev = link_osif->netdev;
        if (qdf_unlikely(link_dev && netif_queue_stopped(link_dev)))
            goto fail;

#ifndef ENABLE_CFG80211_BACKPORTS_MLO
        if (qdf_unlikely(!link_dev && !link_osif->is_bridge_link))
            goto fail;
#endif

        if (qdf_likely(link_osif->mld_xmit)) {
            nbuf_debug_del_record(skb);
            return link_osif->mld_xmit(skb, link_osif);
        }
    }

fail:
    if (mldev)
        mldev->pkt_fail_cnt++;

    qdf_nbuf_free(skb);
    return 0;
}
#endif

static inline osif_dev *
osif_ap_mlo_find_mcast_netdev(struct osif_mldev *mldev, struct sk_buff *skb)
{
    osif_dev  *mcast_osdev = NULL;
    uint8_t chip_id = mldev->primary_chip_id;
    uint8_t pdev_id = mldev->primary_pdev_id;

    mcast_osdev = mldev->link_dev[chip_id][pdev_id];
    if (qdf_unlikely(mcast_osdev)) {
        qdf_nbuf_set_fast_xmit(skb, 0);
        qdf_nbuf_set_tx_fctx_type(skb, (void *)0, CB_FTYPE_MLO_MCAST);
        return mcast_osdev;
    }

    return NULL;
}

int osif_ap_mlo_xmit_netdev(struct sk_buff *skb, struct net_device *bond_dev)
{
    struct ether_header *eh = NULL;
    struct net_device *link_dev = NULL;
    struct osif_mldev *mldev = osif_mld_get_mldev(bond_dev);
    osif_dev *link_osif = NULL;

    nbuf_debug_add_record(skb);
    if (qdf_unlikely(!mldev)) {
        qdf_err("mldev context is NULL");
        goto fail;
    }

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    if (qdf_likely(MLO_METADATA_TAG_IS_VALID(qdf_nbuf_get_mark(skb)) &&
                   qdf_nbuf_fast_xmit(skb) &&
                   !qdf_nbuf_pkt_type_is_mcast(skb))) {
        uint8_t link_id = MLO_METADATA_LINK_ID_GET(qdf_nbuf_get_mark(skb));

        link_osif = mldev->link_vaps[link_id];
        if (qdf_likely(link_osif))
            goto fast_xmit;
    }
#endif

#ifndef QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    if (qdf_unlikely(mldev->is_raw_mld))
        return osif_ap_mlo_raw_xmit(skb, mldev);
#endif

#if MESH_MODE_SUPPORT
    if (qdf_unlikely(mldev->is_mesh_mld))
        return osif_ap_mlo_mesh_xmit(skb, mldev);
#endif

    eh = (struct ether_header *)skb->data;
    if (qdf_unlikely(IEEE80211_IS_MULTICAST(eh->ether_dhost))) {
        if (qdf_unlikely(mldev->is_raw_mld))
            return osif_ap_mlo_special_mcast_handler(skb, mldev);

        link_osif = osif_ap_mlo_find_mcast_netdev(mldev, skb);
    } else
        link_osif = osifp_peer_find_hash_find_osdev(mldev, eh->ether_dhost);


    if (qdf_unlikely(!link_osif))
        goto fail;

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
fast_xmit:
#endif
    link_dev = link_osif->netdev;
    if (qdf_unlikely(link_dev && netif_queue_stopped(link_dev)))
        goto fail;

#ifndef ENABLE_CFG80211_BACKPORTS_MLO
    if (qdf_unlikely(!link_dev && !link_osif->is_bridge_link))
        goto fail;
#endif

    qdf_nbuf_set_vdev_xmit_type(skb, DP_XMIT_MLD);

    if (qdf_likely(link_osif->mld_xmit)) {
        nbuf_debug_del_record(skb);
        return link_osif->mld_xmit(skb, link_osif);
    }

fail:
    if (mldev)
        mldev->pkt_fail_cnt++;

    qdf_nbuf_free(skb);
    return 0;
}

int osif_ap_mlo_xmit_netdev_bond(struct sk_buff *skb, struct net_device *bond_dev)
{
    struct net_device *link_dev = NULL;
    struct osif_mldev *mldev = osif_mld_get_mldev(bond_dev);
    struct ether_header *eh = NULL;
    osif_dev *link_osif = NULL;

    nbuf_debug_add_record(skb);
    if (qdf_unlikely(!mldev)) {
        qdf_err("mldev context is NULL");
        goto fail;
    }

#ifndef QCA_SUPPORT_RAWMODE_PKT_SIMULATION
    if (qdf_unlikely(mldev->is_raw_mld))
        return osif_ap_mlo_raw_xmit(skb, mldev);
#endif

#if MESH_MODE_SUPPORT
    if (qdf_unlikely(mldev->is_mesh_mld))
        return osif_ap_mlo_mesh_xmit(skb, mldev);
#endif

    eh = (struct ether_header *)skb->data;
    if (qdf_unlikely(IEEE80211_IS_MULTICAST(eh->ether_dhost))) {
        if (qdf_unlikely(mldev->is_raw_mld))
            return osif_ap_mlo_special_mcast_handler(skb, mldev);

        link_osif = osif_ap_mlo_find_mcast_netdev(mldev, skb);

        if (qdf_likely(link_osif))
            link_dev =  link_osif->netdev;
    } else
        link_dev = osifp_peer_find_hash_find_netdev(mldev, eh->ether_dhost);

    if (qdf_likely(link_dev && !netif_queue_stopped(link_dev))) {
        nbuf_debug_del_record(skb);
        skb->dev = link_dev;
        dev_queue_xmit(skb);
        return 0;
    }

fail:
    if (mldev)
        mldev->pkt_fail_cnt++;

    qdf_nbuf_free(skb);
    return 0;
}

struct net_device *
osif_get_ap_mlo_tx_netdev(void *bond_mlo_ctx, void *dst)
{
    struct osif_mldev *mldev = (struct osif_mldev *)bond_mlo_ctx;
    char *da_addr = (char *)dst;

    if (!mldev)
        return NULL;

    if (qdf_unlikely(IEEE80211_IS_MULTICAST(da_addr)))
        return NULL;

    return osifp_peer_find_hash_find_netdev(mldev, dst);
}

osif_dev *osif_sta_mlo_find_osdev(struct osif_mldev *mldev)
{
    uint8_t chip_id;
    uint8_t pdev_id;

    chip_id = mldev->primary_chip_id;
    pdev_id = mldev->primary_pdev_id;

    return mldev->link_dev[chip_id][pdev_id];
}

qdf_export_symbol(osif_sta_mlo_find_osdev);

static inline struct net_device *
osif_sta_mlo_find_netdev(struct osif_mldev *mldev)
{
    osif_dev  *osdev = NULL;

    osdev = osif_sta_mlo_find_osdev(mldev);
    if (qdf_unlikely(!osdev))
        return NULL;

    return osdev->netdev;
}

int osif_sta_mlo_xmit_netdev(struct sk_buff *skb, struct net_device *bond_dev)
{
    struct osif_mldev *mldev = osif_mld_get_mldev(bond_dev);
    osif_dev *link_osif = NULL;
    struct net_device *link_dev = NULL;

    nbuf_debug_add_record(skb);
    if (qdf_unlikely(!mldev)) {
        qdf_err("mldev context is NULL");
        goto fail;
    }

    link_osif = osif_sta_mlo_find_osdev(mldev);

    if (qdf_unlikely(!link_osif))
        goto fail;

    link_dev = link_osif->netdev;
    if (link_dev && netif_queue_stopped(link_dev))
        goto fail;

    if (qdf_likely(link_osif->mld_xmit)) {
        nbuf_debug_del_record(skb);
        return link_osif->mld_xmit(skb, link_osif);
    }

fail:
    if (mldev)
        mldev->pkt_fail_cnt++;

    qdf_nbuf_free(skb);
    return 0;
}

int osif_sta_mlo_xmit_netdev_bond(struct sk_buff *skb, struct net_device *bond_dev)
{
    struct osif_mldev *mldev = osif_mld_get_mldev(bond_dev);
    struct net_device *link_dev = NULL;

    nbuf_debug_add_record(skb);
    if (qdf_unlikely(!mldev)) {
        qdf_err("mldev context is NULL");
        goto fail;
    }

    link_dev = osif_sta_mlo_find_netdev(mldev);
    if (qdf_likely(link_dev && !netif_queue_stopped(link_dev))) {
        nbuf_debug_del_record(skb);
        skb->dev = link_dev;
        dev_queue_xmit(skb);
        return 0;
    }

fail:
    if (mldev)
        mldev->pkt_fail_cnt++;

    qdf_nbuf_free(skb);
    return 0;
}

struct net_device *
osif_get_sta_mlo_tx_netdev(void *bond_mlo_ctx, void *dst)
{
    uint8_t chip_id;
    uint8_t pdev_id;
    osif_dev  *osdev = NULL;
    struct osif_mldev *mldev = (struct osif_mldev *)bond_mlo_ctx;

    if (qdf_unlikely(!mldev))
        return NULL;

    chip_id = mldev->primary_chip_id;
    pdev_id = mldev->primary_pdev_id;
    osdev = mldev->link_dev[chip_id][pdev_id];
    if (qdf_unlikely(!osdev))
        return NULL;

    return osdev->netdev;
}

struct net_device *
osif_get_mlo_link_netdev(struct net_device *dev, void *dst)
{
    struct osif_mldev *mlo_ctx = NULL;
    struct osif_mldev *tmp_mlo_ctx = NULL;
    struct wiphy *mld_wiphy = NULL;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    struct cfg80211_context *mld_wiphy_priv;
#else
    struct cfg80211_mld_wiphy_priv *mld_wiphy_priv;
#endif
    int i = 0;

    /* Not a wifi interface */
    if (!dev->ieee80211_ptr)
        return NULL;

    mld_wiphy = dev->ieee80211_ptr->wiphy;
    mld_wiphy_priv = wiphy_priv(mld_wiphy);
    for (i = 0; i < MAX_OSIF_MLDEV; i++) {
         tmp_mlo_ctx = mld_wiphy_priv->mldev[i];
         if (tmp_mlo_ctx && tmp_mlo_ctx->mld_dev == dev) {
             mlo_ctx = tmp_mlo_ctx;
             break;
         }
    }

    if (!mlo_ctx)
        return NULL;

    if (mlo_ctx->wdev.iftype == NL80211_IFTYPE_AP)
        return osif_get_ap_mlo_tx_netdev(mlo_ctx, dst);
    else
        return osif_get_sta_mlo_tx_netdev(mlo_ctx, dst);
}

#else
struct net_device *
osif_get_mlo_link_netdev(struct net_device *dev, void *dst)
{
    return NULL;
}
#endif

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
osif_dev *get_sta_osif_dev_by_freq(struct osif_mldev *osifp_mldev,
                                   unsigned int freq)
{
    struct ieee80211com *ic = NULL;
    osif_dev *osifp;
    wlan_if_t vap;
    uint16_t i;
    QDF_STATUS status = QDF_STATUS_E_FAILURE;

    for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
         qdf_freq_t low_2g, high_2g, low_5g, high_5g;

         osifp =  osifp_mldev->link_vifs[i];
         if (!osifp)
             continue;

         vap = osifp->os_if;
         if (!vap) {
             qdf_err("vap is null for link %d", i);
             return NULL;
         }

         ic = vap->iv_ic;
         status = wlan_reg_get_freq_range(ic->ic_pdev_obj, &low_2g, &high_2g,
                                          &low_5g, &high_5g);
         if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("failed to get freq range");
            return NULL;
         }

         osif_debug("low_2g = %d high_2g = %d, low_5g = %d high_5g = %d",
                    low_2g, high_2g, low_5g, high_5g);
         if ((low_2g && high_2g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_2_4_GHZ) &&
             (freq >= low_2g) && (freq <= high_2g)) {
             return osifp;

         } else if ((low_5g && high_5g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_5_GHZ) &&
             ((freq >= low_5g) && (freq <= high_5g))) {
             return osifp;
         } else {
             qdf_err("freq %d not belong to this vap %p", freq, vap);
         }
    }
    return NULL;
}

struct ieee80211vap *get_sta_link_vap_by_freq(struct osif_mldev *osifp_mldev,
                                              unsigned int freq)
{
    struct ieee80211com *ic = NULL;
    osif_dev *osifp;
    wlan_if_t vap;
    uint16_t i;
    QDF_STATUS status = QDF_STATUS_E_FAILURE;

    for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
         qdf_freq_t low_2g, high_2g, low_5g, high_5g;

         osifp =  osifp_mldev->link_vifs[i];
         if (!osifp)
             continue;

         vap = osifp->os_if;
         if (!vap) {
             qdf_err("vap is null for link %d", i);
             return NULL;
         }

         ic = vap->iv_ic;
         status = wlan_reg_get_freq_range(ic->ic_pdev_obj, &low_2g, &high_2g,
                                          &low_5g, &high_5g);
         if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("failed to get freq range");
            return NULL;
         }

         osif_debug("low_2g = %d high_2g = %d, low_5g = %d high_5g = %d",
                    low_2g, high_2g, low_5g, high_5g);
         if ((low_2g && high_2g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_2_4_GHZ) &&
             (freq >= low_2g) && (freq <= high_2g)) {
             return vap;

         } else if ((low_5g && high_5g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_5_GHZ) &&
             ((freq >= low_5g) && (freq <= high_5g))) {
             return vap;
         } else {
             qdf_err("freq %d not belong to this vap %p", freq, vap);
         }
    }
    return NULL;
}

QDF_STATUS get_sta_link_vdev_link_addr_by_freq(struct osif_mldev *osifp_mldev,
                                               unsigned int freq,
                                               struct qdf_mac_addr *link_addr)
{
    struct ieee80211com *ic = NULL;
    osif_dev *osifp;
    wlan_if_t vap;
    uint16_t i;
    QDF_STATUS status = QDF_STATUS_E_FAILURE;

    for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
         qdf_freq_t low_2g, high_2g, low_5g, high_5g;

         osifp =  osifp_mldev->link_vifs[i];
         if (!osifp)
             continue;

         vap = osifp->os_if;
         if (!vap) {
             qdf_err("vap is null for link %d", i);
             return QDF_STATUS_E_FAILURE;
         }

         ic = vap->iv_ic;
         status = wlan_reg_get_freq_range(ic->ic_pdev_obj, &low_2g, &high_2g,
                                          &low_5g, &high_5g);
         if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("failed to get freq range");
            return status;
         }

         qdf_err("link_freq = %d", freq);
         qdf_err("low_2g = %d high_2g = %d, low_5g = %d high_5g = %d",
                    low_2g, high_2g, low_5g, high_5g);
         if ((low_2g && high_2g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_2_4_GHZ) &&
             (freq >= low_2g) && (freq <= high_2g)) {
             qdf_mem_copy(link_addr->bytes, vap->iv_myaddr, QDF_MAC_ADDR_SIZE);
             return QDF_STATUS_SUCCESS;

         } else if ((low_5g && high_5g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_5_GHZ) &&
             ((freq >= low_5g) && (freq <= high_5g))) {
             qdf_mem_copy(link_addr->bytes, vap->iv_myaddr, QDF_MAC_ADDR_SIZE);
             return QDF_STATUS_SUCCESS;

         } else {
             qdf_err("freq %d not belong to this vap %p", freq, vap);
         }
    }

    return status;
}

void fill_partner_chan_freq(struct wlan_objmgr_vdev *vdev,
                            const struct cfg80211_connect_params *req,
                            int cur_link_id)
{
    int i, partner_count = 0;
    struct mlo_partner_info ml_partner_info;

    for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
         if (i == cur_link_id)
             continue;

         if (req->links[i].freq == 0)
             continue;

         ml_partner_info = vdev->mlo_dev_ctx->sta_ctx->ml_partner_info;
         ml_partner_info.partner_link_info[partner_count].chan_freq = req->links[i].freq;
         partner_count++;
    }
}

QDF_STATUS osif_get_partner_info(struct net_device *dev,
                                 struct wlan_objmgr_vdev *vdev,
                                 const struct cfg80211_connect_params *req,
                                 struct mlo_partner_info *partner_info)
{
    struct wiphy *wiphy = dev->ieee80211_ptr->wiphy;
    struct ieee80211com *ic = NULL;
    osif_dev *osifp;
    wlan_if_t vap;
    QDF_STATUS status;
    struct osif_mldev *osifp_mldev;
    uint16_t i;
    struct qdf_mac_addr macaddr = {0};
    uint8_t num_partner_links = 0;

    osifp_mldev = osif_mld_get_mldev(dev);
    if (!osifp_mldev) {
        qdf_err("osifp_mldev is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (osifp_mldev->dev_type != OSIF_NETDEV_TYPE_MLO) {
        qdf_err("osifp_mldev dev_type is not of type OSIF_NETDEV_TYPE_MLO");
        return QDF_STATUS_E_FAILURE;
    }

    osifp = get_vap_device_context(wiphy, dev, req->link_id);
    if (!osifp) {
        qdf_err("osifp is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    vap = osifp->os_if;
    if (!vap) {
        qdf_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    ic = vap->iv_ic;
    if (!req->ap_mld_addr) {
        /* Here if AP MLD addr is not present then its a non-ML vap's dev
         * received as parameter. Return FAILURE since it is a non-ML
         * association.
         */
        qdf_err("ap_mld_addr is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    qdf_err("ap_mld_addr "QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(req->ap_mld_addr));

    for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
         if ((!req->links[i].bssid) || (req->links[i].bssid &&
             IS_NULL_ADDR(req->links[i].bssid))) {
             continue;
         }

         qdf_err("Link id:%d Freq:%d", i, req->links[i].freq);
         qdf_err("BSSID "QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(req->links[i].bssid));

         /* Check for assoc link */
         if (i == req->link_id) {
             qdf_mem_copy(vap->ap_mld_addr.bytes, req->ap_mld_addr,
                          QDF_MAC_ADDR_SIZE);
             fill_partner_chan_freq(vdev, req, i);
             continue;
         }

         if (partner_info->num_partner_links >=
             QDF_ARRAY_SIZE(partner_info->partner_link_info)) {
             qdf_err("Insufficient size %zu of array for partner link info",
                     QDF_ARRAY_SIZE(partner_info->partner_link_info));
             return QDF_STATUS_E_NOMEM;
         }

         /* Get the link addr of link vdevs from the link frequency */
         status = get_sta_link_vdev_link_addr_by_freq(osifp_mldev,
                                                      req->links[i].freq,
                                                      &macaddr);
         if (status == QDF_STATUS_SUCCESS) {
             qdf_mem_copy(partner_info->partner_link_info[num_partner_links].link_addr.bytes,
                          macaddr.bytes, QDF_MAC_ADDR_SIZE);
             partner_info->partner_link_info[num_partner_links].link_id = i;
             qdf_mem_copy(vap->ap_mld_addr.bytes, req->ap_mld_addr,
                          QDF_MAC_ADDR_SIZE);
             num_partner_links++;
         }
    }

    partner_info->num_partner_links = num_partner_links;
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
osif_mlo_sta_connect_get_partner_info(struct wlan_objmgr_vdev *vdev,
                                      const struct cfg80211_connect_params *req,
                                      struct mlo_partner_info *partner_info)

{
    struct ieee80211vap *vap;
    struct net_device *dev;
    osif_dev *osifp;
    QDF_STATUS ret;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    osifp = (osif_dev *)((os_if_t)vap->iv_ifp);
    dev = osifp->mldev->mld_dev;
    ret = osif_get_partner_info(dev, vdev, req, partner_info);

    return ret;
}

QDF_STATUS get_mlsta_osif_dev_by_freq(struct osif_mldev *osifp_mldev,
                                      unsigned int freq, osif_dev **osifp_new)
{
    struct ieee80211com *ic = NULL;
    osif_dev *osifp;
    wlan_if_t vap;
    uint16_t i;
    QDF_STATUS status = QDF_STATUS_E_FAILURE;

    for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
         qdf_freq_t low_2g, high_2g, low_5g, high_5g;

         osifp =  osifp_mldev->link_vifs[i];
         if (!osifp)
             continue;

         vap = osifp->os_if;
         if (!vap) {
             qdf_err("VAP NULL on link %d", i);
             return QDF_STATUS_E_FAILURE;
         }

         ic = vap->iv_ic;
         status = wlan_reg_get_freq_range(ic->ic_pdev_obj, &low_2g, &high_2g,
                                          &low_5g, &high_5g);
         if (QDF_IS_STATUS_ERROR(status)) {
             qdf_err("Failed to get freq range");
             return status;
         }

         qdf_err("low_2g = %d high_2g = %d, low_5g = %d high_5g = %d",
                    low_2g, high_2g, low_5g, high_5g);
         if ((low_2g && high_2g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_2_4_GHZ) &&
             (freq >= low_2g) && (freq <= high_2g)) {
             *osifp_new = osifp;
             return QDF_STATUS_SUCCESS;
         }

         if ((low_5g && high_5g) &&
             (util_scan_scm_freq_to_band(freq) == WLAN_BAND_5_GHZ) &&
             ((freq >= low_5g) && (freq <= high_5g))) {
             *osifp_new = osifp;
             return QDF_STATUS_SUCCESS;
         } else {
             qdf_err("freq %d not belong to this vap %p", freq, vap);
             status = QDF_STATUS_E_INVAL;
         }
    }
    return status;
}

osif_dev *osif_get_mlsta_osif_dev(struct net_device *dev,
                                  struct cfg80211_connect_params *req)
{
    QDF_STATUS status;
    struct osif_mldev *osifp_mldev;
    uint16_t i;
    osif_dev *osifp = NULL;
    wlan_if_t vap;
    struct wiphy *wiphy;
    uint32_t freq;

    osifp_mldev = osif_mld_get_mldev(dev);
    if (!osifp_mldev) {
        qdf_err("osifp_mldev is NULL");
        return NULL;
    }

    if (osifp_mldev->dev_type == OSIF_NETDEV_TYPE_VAP) {
        qdf_err("osifp_mldev dev_type is OSIF_NETDEV_TYPE_VAP");
        wiphy = dev->ieee80211_ptr->wiphy;
        if (!wiphy) {
            qdf_err("wiphy is null");
            return NULL;
        }
        return get_vap_device_context(wiphy, dev, 0);
    } else if (osifp_mldev->dev_type != OSIF_NETDEV_TYPE_MLO) {
        qdf_err("osifp_mldev dev_type is not type OSIF_NETDEV_TYPE_MLO");
        return NULL;
    }

    if (!req->ap_mld_addr) {
        /* Here if AP MLD addr is not present then its a non-ML vap's dev
         * received as parameter. In this case, the osif_dev to be returned
         * is the ath_netdev_priv(dev).
         */
        qdf_err("ap_mld_addr is NULL. MLO fallback to Non-ML is not yet supported!");
        return NULL;

        wiphy = dev->ieee80211_ptr->wiphy;
        if (!wiphy) {
            osif_err("wiphy is null");
            return NULL;
        }

       /* cfg80211_connect_params has two field: channel_hint and channel.
        * Channel is populated directly from frequency in the kernel and
        * channel_hint is populated from frequency_hint in the kernel.
        *
        * If frequency is present, the channel field is updated. Else
        * channel_hint is updated based on frequency_hint.
        *
        * In 6.6 kernel, frequency is moved to per link_vifs basis and
        * thus is not present. So in this case, only frequency_hint is
        * populated due to which only channel_hint field would be accessible
        * and not channel field.
        */

       if (req->channel)
           freq = req->channel->center_freq;
       else if (req->channel_hint)
           freq = req->channel_hint->center_freq;

        return get_sta_osif_dev_by_freq(osifp_mldev, freq);
    }

    for (i = 0; i < IEEE80211_MLD_MAX_NUM_LINKS; i++) {
        if ((!req->links[i].bssid) ||
            (req->links[i].bssid && IS_NULL_ADDR(req->links[i].bssid))) {
            osif_debug("req->links[%d].bssid is null", i);
            continue;
        }

        qdf_err("Link id:%d freq:%d", i, req->links[i].freq);
        qdf_err("BSSID "QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(req->links[i].bssid));

        if (i == req->link_id) {
            req->bssid = req->links[i].bssid;
            status = get_mlsta_osif_dev_by_freq(osifp_mldev, req->links[i].freq, &osifp);
            if (!osifp) {
                qdf_err("osifp is null");
                return NULL;
            }

            if (status == QDF_STATUS_SUCCESS) {
                vap = osifp->os_if;
                if (!vap) {
                    qdf_err("VAP NULL on link");
                    return NULL;
                }

                vap->sta_mld_assoc_linkid = req->link_id;
                vap->sta_mld_assoc_bssid = req->bssid;
                return osifp;
            }
            break;
        }
    }

    return NULL;
}
#else
QDF_STATUS get_sta_link_vdev_link_addr_by_freq(struct osif_mldev *osifp_mldev,
                                               unsigned int freq,
                                               struct qdf_mac_addr *link_addr)
{
    return QDF_STATUS_E_FAILURE;
}

QDF_STATUS osif_get_partner_info(struct net_device *dev,
                                 struct wlan_objmgr_vdev *vdev,
                                 const struct cfg80211_connect_params *req,
                                 struct mlo_partner_info *partner_info)
{
    return QDF_STATUS_E_FAILURE;
}

QDF_STATUS
osif_mlo_sta_connect_get_partner_info(struct wlan_objmgr_vdev *vdev,
                                      const struct cfg80211_connect_params *req,
                                      struct mlo_partner_info *partner_info)
{
    return QDF_STATUS_E_FAILURE;
}

QDF_STATUS get_mlsta_osif_dev_by_freq(struct osif_mldev *osifp_mldev,
                                      unsigned int freq, osif_dev **osifp_new)
{
    return QDF_STATUS_E_FAILURE;
}

osif_dev *osif_get_mlsta_osif_dev(struct net_device *dev,
                                  struct cfg80211_connect_params *req)
{
    struct wiphy *wiphy = dev->ieee80211_ptr->wiphy;

    if (!wiphy) {
        qdf_err("wiphy is null");
        return NULL;
    }

    return get_vap_device_context(wiphy, dev, 0);
}
#endif /* ENABLE_CFG80211_BACKPORTS_MLO */

void * osif_get_mld_netdev(osif_dev *osifp)
{
#if defined (WLAN_FEATURE_11BE_MLO)
   wlan_if_t vap = osifp->os_if;
   struct wlan_objmgr_vdev *vdev = vap->vdev_obj;

   if (osifp->mldev && wlan_vdev_mlme_is_mlo_vdev(vdev))
       return osifp->mldev->mld_dev;
#endif
   return NULL;
}
qdf_export_symbol(osif_get_mld_netdev);

uint16_t osif_get_hard_header_len_from_osdev(osif_dev *osdev)
{
    struct net_device *dev = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;

    if (!osdev)
        return 0;

    if (osdev->netdev) {
        dev = osdev->netdev;
    } else {
        vap = osdev->os_if;
        if (!vap)
            return 0;

        ic = vap->iv_ic;
        if (!ic)
            return 0;

        dev = ic->ic_wdev.netdev;
        if (!dev)
            return 0;
    }
    return dev->hard_header_len;
}

qdf_export_symbol(osif_get_hard_header_len_from_osdev);

uint16_t osif_get_needed_headroom_from_osdev(osif_dev *osdev)
{
    struct net_device *dev = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;

    if (!osdev)
        return 0;

    if (osdev->netdev) {
        dev = osdev->netdev;
    } else {
        vap = osdev->os_if;
        if (!vap)
            return 0;

        ic = vap->iv_ic;
        if (!ic)
            return 0;

        dev = ic->ic_wdev.netdev;
        if (!dev)
            return 0;
    }
    return dev->needed_headroom;
}

qdf_export_symbol(osif_get_needed_headroom_from_osdev);

#ifdef WLAN_SUPPORT_DPDK
static int osif_cfgmgr_msg_handler(struct cfgmgr_cmn_msg *cm, void *cb_data)
{
    uint32_t msg_type = 0;
    uint16_t msg_len = 0;

    if (!cm) {
        qdf_err("cmn msg is NULL");
        return -EINVAL;
    }

    msg_len = cm->msg_len;
    if (!msg_len || msg_len <= (sizeof(struct cfgmgr_cmn_msg))) {
        qdf_err("msg_len is invalid %d", msg_len);
        return -EINVAL;
    }

    msg_type = cm->msg_type;
    if (msg_type <= DPDK_WLAN_CMD_TYPE_INVALID && msg_type >= DPDK_WLAN_CMD_TYPE_MAX) {
        qdf_err("Invalid msg type %d", msg_type);
        return -EINVAL;
    }

    switch (msg_type) {
        case DPDK_WLAN_CMD_TYPE_SOC_INFO:
            if (cm->msg_len != sizeof(struct dpdk_wlan_soc_info_cmd)) {
                qdf_err("Invalid msg len %d for msg type %d", cm->msg_len, msg_type);
                return -EINVAL;
            }
            return ieee80211_ucfg_cfgmgr_get_soc_info((struct dpdk_wlan_soc_info_cmd *)cm);
        case DPDK_WLAN_CMD_TYPE_VDEV_INFO:
            if (cm->msg_len != sizeof(struct dpdk_wlan_vdev_info_cmd)) {
                qdf_err("Invalid msg len %d for msg type %d", cm->msg_len, msg_type);
                return -EINVAL;
            }
            return ieee80211_ucfg_cfgmgr_get_vdev_info((struct dpdk_wlan_vdev_info_cmd *)cm);
        case DPDK_WLAN_CMD_TYPE_PEER_INFO:
            if (cm->msg_len != sizeof(struct dpdk_wlan_peer_info_cmd)) {
                qdf_err("Invalid msg len %d for msg type %d", cm->msg_len, msg_type);
                return -EINVAL;
            }
            return ieee80211_ucfg_cfgmgr_get_peer_info((struct dpdk_wlan_peer_info_cmd *)cm);
        default:
            break;
    }

    return 0;
}
#endif

int init_umac(void)
{
#ifdef QCA_WIFI_MODULE_PARAMS_FROM_INI
	initialize_umac_module_param_from_ini();
#endif
	mlme_set_ops_cb();

	/* Connection manager callback */
	osif_cm_register_legacy_cb();

	/* scan call back registration */
	osif_scan_register_cb();

	/* Global dispatcher init*/
	if(dispatcher_init() != 0) {
		goto fail;
	}

	/* MBSS component init */
	if(wlan_mbss_init()) {
		goto fail_mbss;
	}

	if (wlan_mbss_register_ext_ops())
		goto fail_mbss;

	/* Global dispatcher enable*/
	if(dispatcher_enable() != 0) {
		goto fail_mbss;
	}

	/* init UMAC */
	if(wlan_umac_init() != 0) {
		goto fail_umac;
	}

	/* MLO mlme callback registration */
	mlo_set_ops_cb();

	QDF_BUG(!cfg_parse("global.ini"));
	QDF_BUG(!cfg_parse_to_global_store("internal/global_i.ini"));

	/* Set the print ratelimting thresholds */
	qdf_rl_print_count_set(200);
	qdf_rl_print_time_set(2);

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
	/* Telemtry agent component init */
	if(wlan_telemetry_agent_init()) {
		goto fail_telemetry;
	}
#endif

#ifdef WLAN_SUPPORT_DPDK
	/* DPDK Cfgmgr ops regisrtation */
	cfgmgr_wlan_register_msg_handler(osif_cfgmgr_msg_handler, NULL);
#endif
	/*
	 * Registering different module ID's
	 * for different message queue of the scheduler.
	 *
	 * When message are posted to this queues, thier
	 * handlers are executed in the scheduler context.
	 *
	 * The priotiry of the message queues
	 * is based on the order in whcih they
	 * are registered.
	 */
	scheduler_register_module(QDF_MODULE_ID_SYS,
			&scheduler_timer_q_mq_handler);
	scheduler_register_module(QDF_MODULE_ID_MLME,
			&scheduler_mlme_mq_handler);
	scheduler_register_module(QDF_MODULE_ID_OS_IF,
			&scheduler_os_if_mq_handler);
	scheduler_register_module(QDF_MODULE_ID_SCAN,
			&scheduler_scan_mq_handler);

	/* Set the threshold for scheduler timeout to 20 secs */
	scheduler_set_watchdog_timeout(20000);

#ifdef CONFIG_SAWF_DEF_QUEUES
	/*Initialize the SAWF*/
	if (!QDF_IS_STATUS_SUCCESS(wlan_sawf_init())) {
		return -ENOMEM;
	}
#endif
	goto exit;

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
fail_telemetry:
	wlan_umac_deinit();
#endif
fail_umac:
	dispatcher_disable();
	wlan_mbss_deinit();
fail_mbss:
	dispatcher_deinit();
fail:
	return -ENOMEM;
exit:
    return 0;
}

void exit_umac(void)
{

#ifdef WLAN_SUPPORT_DPDK
     /* DPDK Cfgmgr ops regisrtation */
     cfgmgr_wlan_unregister_msg_handler();
#endif

     /* de init UMAC */
    wlan_umac_deinit();

    /* Global dispatcher disable*/
    dispatcher_disable();

    cfg_release();

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
    wlan_telemetry_agent_deinit();
#endif
    /* MBSS component deinit */
    wlan_mbss_deinit();

#ifdef CONFIG_SAWF_DEF_QUEUES
    /* Deinit SAWF ctx*/
    wlan_sawf_deinit();
#endif

   /* Global Deinit after PSOC removed */
    dispatcher_deinit();

}

#ifndef QCA_SINGLE_WIFI_3_0
module_init(init_umac);
module_exit(exit_umac);
#else
qdf_export_symbol(init_umac);
qdf_export_symbol(exit_umac);
#endif

MODULE_DESCRIPTION("Support for Atheros 802.11 wireless LAN cards.");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif
