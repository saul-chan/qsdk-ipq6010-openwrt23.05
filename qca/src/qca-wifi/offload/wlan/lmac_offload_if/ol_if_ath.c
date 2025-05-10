/*
 * Copyright (c) 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2011, Atheros Communications Inc.
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
/*
 * LMAC offload interface functions for UMAC - for power and performance offload model
 */

#ifdef IPA_OFFLOAD
#include <wlan_ipa_ucfg_api.h>
#endif
#include <ol_if_athvar.h>
#include <ol_if_pdev.h>
#include <ol_if_fips.h>
#include <ol_if_athpriv.h>
#ifdef CONFIG_AFC_SUPPORT
#include <wlan_afc_ucfg_api.h>
#endif
#include <ol_if_utility.h>
#include <ol_txrx_api_internal.h>
#include <osdep.h>
#include <hif.h>
#include <bmi.h>
#include <target_type.h>
#ifdef WIFI_TARGET_TYPE_2_0
#include <sw_version.h>
#endif
#include <targaddrs.h>
#include <ol_helper.h>
#include <cdp_txrx_cmn.h>
#include <qdf_lock.h>
#include <qdf_types.h>
#include <qdf_util.h>
#include <qdf_mem.h>
#include <qdf_trace.h>
#include <wmi_unified_api.h>
#include <wmi_unified_vdev_api.h>
#include <cdp_txrx_cmn_reg.h>
#include <wlan_lmac_if_def.h>
#include <wlan_lmac_if_api.h>
#include <wlan_psoc_mlme_api.h>
#include <wlan_mgmt_txrx_utils_api.h>
#include <cdp_txrx_ctrl.h>
#include <qwrap_structure.h>
#include <wlan_global_lmac_if_api.h>
#include <target_if.h>
#include <service_ready_util.h>
#include <wlan_scan.h>
#include <ol_cfg.h>
#include <wlan_cfg.h>
#include <pld_common.h>
#include <ieee80211_api.h>
#include <qdf_types.h>
#include <qdf_parse.h>
#include <ieee80211_crypto_nlshim_api.h>
#if ATH_ACS_DEBUG_SUPPORT
#include "acs_debug.h"
#endif
#include <htc_debug.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_if.h>
#endif

/* FIX THIS: the HL vs. LL selection will need to be done
 * at runtime rather than compile time
 */

#if defined(CONFIG_HL_SUPPORT)
#include "wlan_tgt_def_config_hl.h"    /* TODO: check if we need a seperated config file */
#else
#include "wlan_tgt_def_config.h"
#endif

#if OBSS_PD
#include <ol_if_obss.h>
#endif
#include <fw_dbglog_api.h>
#include <ol_if_wow.h>
#include <a_debug.h>
#include <epping_test.h>
#include <pktlog_ac.h>
#include <ol_regdomain.h>

#if WLAN_SPECTRAL_ENABLE
#include "target_if_spectral.h"
#include <ol_if_spectral.h>
#endif

#include <ol_ath.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <ol_if_stats.h>
#include <wds_addr_api.h>
#include <qdf_atomic.h>
#include <ol_swap.h>
#include <ol_if_eeprom.h>

#if ATH_PERF_PWR_OFFLOAD

#include <ath_pci.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#ifndef __LINUX_POWERPC_ARCH__
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 24)
#include <asm/segment.h>
#endif
#endif
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <reg_struct.h>
#include <regtable.h>

#if UMAC_SUPPORT_ACFG || UMAC_SUPPORT_ACFG_RECOVERY
#include "ieee80211_acfg.h"
#include <acfg_event_types.h>   /* for ACFG_WDT_TARGET_ASSERT */
#endif

#if WIFI_MEM_MANAGER_SUPPORT
#include "mem_manager.h"
#endif

#include <osif_private.h>

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_vdev_if.h>
#endif
#include <dispatcher_init_deinit.h>
#if QCA_AIRTIME_FAIRNESS
#include <target_if_atf.h>
#endif
#if UNIFIED_SMARTANTENNA
#include <target_if_sa_api.h>
#endif /* UNIFIED_SMARTANTENNA */
#if WLAN_SPECTRAL_ENABLE
#include <wlan_spectral_ucfg_api.h>
#include <wlan_spectral_utils_api.h>
#endif
#include <init_event_handler.h>
#include <init_cmd_api.h>
#include <init_deinit_lmac.h>
#include <wlan_osif_priv.h>
#include <wlan_utility.h>

#include <reg_services_public_struct.h>
#include <wlan_reg_services_api.h>
#include <wlan_reg_channel_api.h>
#include <ieee80211_regdmn.h>
#include <ieee80211_ucfg.h>
#include <ol_regdomain_common.h>
#include <wlan_reg_ucfg_api.h>
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link.h>
#endif
#if WLAN_SUPPORT_MSCS && QCA_NSS_PLATFORM
#include <qca_mscs.h>
#endif

#if UMAC_SUPPORT_CBS
#include "ieee80211_cbs.h"
#endif

#if CONFIG_LEDS_IPQ
#include <drivers/leds/leds-ipq.h>
#endif
#ifdef WLAN_SUPPORT_FILS
#include <target_if_fd.h>
#endif

#include <target_if_dp.h>
#ifdef DIRECT_BUF_RX_ENABLE
#include <target_if_direct_buf_rx_main.h>
#endif
#include <cfg_ucfg_api.h>

#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif
#if WLAN_CFR_ENABLE
#include <wlan_cfr_utils_api.h>
#include <wlan_cfr_ucfg_api.h>
#endif

#if WLAN_WIFI_RADAR_ENABLE
#include <wlan_wifi_radar_utils_api.h>
#endif

#include <dp_rate_stats.h>

#include <wlan_vdev_mgr_ucfg_api.h>
#include <wlan_cfg80211_scan.h>

#if QLD
#include "qld_api.h"
#endif

#include <target_if_vdev_mgr_rx_ops.h>
#include <ol_if_twt.h>
#include <ol_if_led.h>
#include <wlan_gpio_tgt_api.h>
#include <dp_txrx.h>

#include <target_if_direct_buf_rx_api.h>
#ifdef WIFI_MONITOR_SUPPORT
#include <dp_mon_ol.h>
#include <target_if_dp_mon.h>
#include <cdp_txrx_mon.h>
#endif
#include "ieee80211_radiotap.h"

#include <target_if_mgmt_txrx_rx_reo.h>

#include <wlan_repeater_api.h>
#include <dp_extap.h>
#include <dp_lite_mon_pub.h>

#if ATH_SUPPORT_WRAP
#include "dp_wrap.h"
extern struct ieee80211vap *wlan_get_vap(struct wlan_objmgr_vdev *vdev);
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
#include <wlan_mlo_mgr_setup.h>
#include <wmi_unified_11be_param.h>
#include <wlan_mlo_mgr_peer.h>
#include <cdp_txrx_mlo.h>
extern void __qca_ol_update_mlo_capability(bool wsi_remap, bool wsi_recv_in_progress);
#endif
#include <wlan_lmac_dispatcher.h>

#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
#include <target_if_global_shmem.h>
#endif
#include <wlan_mlo_mgr_cmn.h>

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
#include <wmi_unified_crypto_api.h>
#endif

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#endif

#ifdef WLAN_FEATURE_11BE_MLO
#include <target_if_mlo_mgr.h>
#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
#include <wmi_unified_11be_api.h>
#include <wlan_mlme_dispatcher.h>
#include <wlan_mlo_mgr_ap.h>
#endif
#endif
#include <wlan_cp_stats_ucfg_api.h>
#include <wlan_cfg80211_ic_cp_stats.h>

#include <wlan_mlme_if.h>
#include <init_deinit_lmac.h>

#ifdef CONFIG_SAWF
#include <target_if_sawf.h>
#include <wlan_sawf.h>
#endif

#ifdef QCA_BLOCK_EVENT_TO_USERSPACE
extern struct filter_events g_acfg_ev_fl[WL_ACFG_MAX_EVENT];
extern struct filter_events g_iwevcustom_ev_fl[IEEE80211_EV_MAX_EVENT];
extern struct filter_events g_cfg_ev_fl[QCA_NL80211_VENDOR_SUBCMD_INDEX_LAST];
#endif

extern u_int32_t osif_get_num_active_vaps(wlan_dev_t comhandle);

extern ol_ath_soc_softc_t *ol_global_soc[GLOBAL_SOC_SIZE];
extern int ol_num_global_soc;
extern int ol_ath_target_start(struct ol_ath_soc_softc *soc);

static int ath_device_event(struct notifier_block *unused, unsigned long event, void *ptr);

#ifdef CONFIG_SAWF
static QDF_STATUS
ol_ath_subscribe_sawf_fse_wdi_event(struct ol_ath_softc_net80211 *scn);

static QDF_STATUS
ol_ath_unsubscribe_sawf_fse_wdi_event(struct ol_ath_softc_net80211 *scn);

#ifdef SAWF_ADMISSION_CONTROL
static QDF_STATUS
ol_ath_subscribe_sawf_peer_msduq_event(struct ol_ath_softc_net80211 *scn);

static QDF_STATUS
ol_ath_unsubscribe_sawf_peer_msduq_event(struct ol_ath_softc_net80211 *scn);
#endif
#endif

struct notifier_block ath_device_notifier = {
	        .notifier_call = ath_device_event
};

struct qca_notifier_block qca_device_notifier = {
                .notifier = &ath_device_notifier
};

#ifdef QCA_PARTNER_PLATFORM
extern struct ol_txrx_ast_entry_t *
ol_txrx_ast_find_hash_find(
        ol_txrx_pdev_handle pdev,
        u_int8_t *peer_mac_addr,
        int mac_addr_is_aligned);
extern struct ol_txrx_peer_t *
ol_txrx_peer_find_by_id(
    ol_txrx_pdev_handle pdev,
    u_int16_t peer_id);
#endif
extern void tpc_config_event_handler(ol_scn_t sc, u_int8_t *data, u_int32_t datalen);
extern void ol_ath_ifce_setup(struct ol_ath_softc_net80211 *scn,
                              ifce_status ifce_up);
extern int wlan_vap_omn_update(struct ieee80211com *ic);

#ifdef WLAN_FEATURE_11BE
static void ol_ath_populate_eht_caps(struct ieee80211com *ic,
        uint32_t *eht_cap_info, uint32_t *eht_supp_mcs, uint32_t eht_supp_mcs_size,
        uint32_t *eht_cap_phy_info, uint8_t eht_cap_phy_info_size,
        struct wlan_psoc_host_ppe_threshold *eht_ppet,
        uint32_t *eht_cap_info_internal);
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE_MLO
void osif_process_txrx_peer_create_event(void *pdev_hdl, enum WDI_EVENT event,
                                         void *data, u_int16_t peer_id,
                                         uint32_t status);
void osif_process_peer_unmap_event(void *pdev_hdl, enum WDI_EVENT event,
                                   void *data, u_int16_t peer_id,
                                   uint32_t status);
void osif_process_peer_delete_event(void *pdev_hdl, enum WDI_EVENT event,
                                    void *data, u_int16_t peer_id,
                                    uint32_t status);
void osif_process_primary_umac_update_event(void *pdev_hdl,
                                            enum WDI_EVENT event,
                                            void *data,
                                            u_int16_t peer_id,
                                            uint32_t status);
QDF_STATUS osifp_peer_primary_umac_update
                                 (struct osif_mldev *mldev,
                                  struct cdp_txrx_peer_params_update *param,
                                  bool is_bridge_peer);
#endif /* WLAN_FEATURE_11BE_MLO */

#ifdef QCA_RSSI_DB2DBM
static inline void
ol_ath_rssi_conv_evt_detach(ol_ath_soc_softc_t *soc);
#endif

#if UMAC_SUPPORT_CFG80211
#include <ieee80211_cfg80211.h>
#include "ol_ath_ucfg.h"
#endif

#include <wlan_vdev_mgr_utils_api.h>
#define    PDEV_STATS_DEFAULT_TIMER  1000
/* Beeliner Family specific routines */
extern int ol_target_init(ol_ath_soc_softc_t *soc, bool first);
extern struct g_wifi_info g_winfo;
int osif_recover_profile_alloc(struct ieee80211com *ic);
void ath_vdev_dump(struct wlan_objmgr_psoc *psoc, void *obj, void *args);
void osif_recover_vap_create(struct ieee80211com *ic);
#ifdef WLAN_FEATURE_11BE_MLO
void wlan_recover_bridge_vaps_init(struct ieee80211com *ic);
#endif
int ath_netdev_open(struct net_device *dev);
int ath_netdev_stop(struct net_device *dev);
void ic_reset_params(struct ieee80211com *ic);
static u_int8_t
ol_ath_get_emiwar_80p80_defval(ol_ath_soc_softc_t *soc);
void wmi_proc_create(wmi_unified_t wmi_handle, struct proc_dir_entry *par_entry, int id);
void wmi_proc_remove(wmi_unified_t wmi_handle, struct proc_dir_entry *par_entry, int id);
int osif_ol_ll_vap_hardstart(struct sk_buff *skb, struct net_device *dev);
int osif_ol_vap_hardstart_wifi3(struct sk_buff *skb, struct net_device *dev);
bool ol_ath_net80211_is_mode_offload(struct ieee80211com *ic);
QDF_STATUS ol_ath_get_ah_devid(struct wlan_objmgr_pdev *pdev, uint16_t *devid);
void *ol_hif_open(struct device *dev, void *bdev, void *bid,
        enum qdf_bus_type bus_type, bool reinit, qdf_device_t qdf_dev);
#ifdef QCA_SUPPORT_WDS_EXTENDED
int osif_wds_ext_peer_hardstart_wifi3(struct sk_buff *skb, struct net_device *dev);
void osif_deliver_wds_ext_data_ol(ol_osif_peer_handle osif, struct sk_buff *skb_list);
#if !(QCA_NSS_PLATFORM || QCA_PPE_VP)
void osif_deliver_wds_ext_data_wifi2(ol_osif_peer_handle osif, struct sk_buff *skb_list);
int osif_wds_ext_peer_hardstart_wifi2(struct sk_buff *skb, struct net_device *dev);
#endif
#endif
#if UMAC_SUPPORT_WEXT
void ol_ath_iw_detach(struct net_device *dev);
void ol_ath_iw_attach(struct net_device *dev);
#endif
#define DEFAULT_MGMT_RETRY_LIMIT (4)
#if QLD
void qld_print_table(struct qld_event *dump_table);
#endif
#if defined(EPPING_TEST) && !defined(HIF_USB)
unsigned int eppingtest = 1;
unsigned int bypasswmi = 1;
#else
unsigned int eppingtest = 0;
unsigned int bypasswmi = 0;
#endif

#define FILE_PATH_LEN 128

#define LOW_MEM_SYSTEM_RAM              (131072) /* 131072Kb */

#define DEBUG_SNIFFER_TEST_TX_METADATA "TXMETA"
#define DEBUG_SNIFFER_TEST_RX_METADATA "RXMETA"
#define DEBUG_SNIFFER_TEST_TX_DATA     "TXDATA"
#define DEBUG_SNIFFER_TEST_RX_DATA     "RXDATA"
#define DEBUG_SNIFFER_TEST_TX_MGMT     "TXMGMT"
#define DEBUG_SNIFFER_TEST_RX_MGMT     "RXMGMT"
#define DEBUG_SNIFFER_SIGNATURE_LEN 6
#define POWER_ON_TIMEOUT 2000

#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
void ol_peer_update_primary_info(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t *mac);
#endif
int ol_ath_get_dp_config_param(struct cdp_ctrl_objmgr_psoc *psoc, enum cdp_cfg_param_type param_num);
void ol_reset_params(struct ol_ath_softc_net80211 *scn);

int ol_ath_get_dp_config_param(struct cdp_ctrl_objmgr_psoc *psoc, enum cdp_cfg_param_type param_num);
uint8_t ol_ath_freq_to_channel(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint16_t freq);
uint8_t ol_ath_freq_to_band(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint16_t freq);
int ol_ath_send_delba(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id, uint8_t *peer_macaddr,
                      uint8_t tid, uint8_t reason_code, uint8_t cdp_rcode);
int ol_peer_ast_flowid_map(struct cdp_ctrl_objmgr_psoc *scn_handle, uint16_t peer_id,
                      uint8_t vdev_id, uint8_t *peer_mac_addr);
int ol_ath_get_soc_nss_cfg(struct cdp_ctrl_objmgr_psoc *scn_handle);
#ifdef WLAN_SUPPORT_PPEDS
QDF_STATUS ol_ath_get_ppeds_profile_info_for_vap(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id,
                      struct cdp_ds_vp_params *vp_params);
#endif
QDF_STATUS ol_ath_nss_stats_clr(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id);
#ifdef FEATURE_MEC_OFFLOAD
QDF_STATUS
ol_ath_set_mec_timer(struct cdp_ctrl_objmgr_psoc *psoc,
                     uint8_t vdev_id, uint16_t mec_timer_val);
#endif
#ifdef INTRA_BSS_FWD_OFFLOAD
void ol_ath_vdev_set_intra_bss_val(struct cdp_ctrl_objmgr_psoc *psoc,
                                   uint16_t vdev_id, bool enable);
#endif

#ifdef DP_UMAC_HW_RESET_SUPPORT
void ol_if_update_tx_hardstart(struct cdp_ctrl_objmgr_psoc *psoc,
                               uint8_t vdev_id, struct ol_txrx_hardtart_ctxt *ctxt);
#endif
enum QDF_GLOBAL_MODE ol_ath_get_conn_mode(void);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
uint8_t ol_ath_peer_get_num_mlo_links(struct cdp_ctrl_objmgr_psoc *psoc,
                                      uint8_t vdev_id,
                                      uint8_t *peer_macaddr,
                                      bool is_mld_peer);
osdev_t get_osdev_from_psoc(struct wlan_objmgr_psoc *psoc);
#endif

static struct ol_if_ops  dp_ol_if_ops = {
    .peer_set_default_routing = target_if_peer_set_default_routing,
    .peer_rx_reorder_queue_setup = target_if_peer_rx_reorder_queue_setup,
    .peer_rx_reorder_queue_remove = target_if_peer_rx_reorder_queue_remove,
    .peer_add_wds_entry = ol_ath_node_add_wds_entry,
    .peer_update_wds_entry = ol_ath_node_update_wds_entry,
    .peer_del_wds_entry = ol_ath_node_del_wds_entry,
#ifdef WLAN_FEATURE_MULTI_AST_DEL
    .peer_del_multi_wds_entry = ol_ath_node_del_multi_wds_entry,
#endif
    .peer_unref_delete = ol_ath_peer_unref_delete,
#ifdef RX_PEER_INVALID_ENH
    .rx_invalid_peer = ol_ath_rx_invalid_peer,
#endif
    .peer_map_event = ol_peer_map_event,
    .peer_unmap_event= ol_peer_unmap_event,
    .get_dp_cfg_param = ol_ath_get_dp_config_param,
    .lro_hash_config = target_if_lro_hash_config,
    .rx_mic_error= ol_ath_rx_mic_error,
#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
    .update_primary_link= ol_peer_update_primary_info,
#endif
    .freq_to_channel= ol_ath_freq_to_channel,
    .freq_to_band = ol_ath_freq_to_band,
#ifdef INTRA_BSS_FWD_OFFLOAD
    .vdev_set_intra_bss = ol_ath_vdev_set_intra_bss_val,
#endif
#ifdef FEATURE_MEC_OFFLOAD
    .set_mec_timer = ol_ath_set_mec_timer,
#endif
#if ATH_SUPPORT_NAC_RSSI
    .config_fw_for_nac_rssi = ol_ath_config_fw_for_nac_rssi,
    .config_bssid_in_fw_for_nac_rssi = ol_ath_config_bssid_in_fw_for_nac_rssi,
#endif
    .peer_sta_kickout = ol_ath_peer_sta_kickout,
    .send_delba = ol_ath_send_delba,
    .peer_delete_multiple_wds_entries = ol_ath_node_delete_multiple_wds_entries,
    .pdev_update_lmac_n_target_pdev_id = ol_ath_pdev_update_lmac_n_target_pdev_id,
    .peer_ast_flowid_map = ol_peer_ast_flowid_map,
    /* TODO: Add any other control path calls required to OL_IF/WMA layer */
    .get_soc_nss_cfg = ol_ath_get_soc_nss_cfg,
    .get_device_name = ol_ath_get_pdev_dev_name,
    .nss_stats_clr = ol_ath_nss_stats_clr,
#ifdef WLAN_SUPPORT_RX_FLOW_TAG
    .send_wakeup_trigger = ol_ath_send_wakeup_trigger,
#endif
    .notify_eapol_tx_compl_status = ol_ath_notify_eapol_tx_compl_status,
#ifdef QCA_SUPPORT_WDS_EXTENDED
    .rx_wds_ext_peer_learn = ol_ath_wds_ext_peer_learn,
#endif
#ifdef WLAN_SUPPORT_MESH_LATENCY
    .peer_update_mesh_latency_params = ol_ath_peer_update_mesh_latency_params,
#ifdef CONFIG_SAWF
    .peer_update_sawf_ul_params = ol_ath_peer_update_sawf_ul_params,
    .notify_deactivate_msduq = ol_ath_deactivate_peer_msduq,
#endif
#endif
#ifdef WLAN_SUPPORT_PPEDS
    .peer_set_ppeds_default_routing = target_if_peer_set_ppeds_default_routing,
    .get_ppeds_profile_info_for_vap = ol_ath_get_ppeds_profile_info_for_vap,
#endif
    .get_con_mode = ol_ath_get_conn_mode,
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    .peer_get_num_mlo_links = ol_ath_peer_get_num_mlo_links,
#endif
#ifdef QCA_SUPPORT_LITE_MONITOR
    .config_lite_mon_peer = target_if_config_lite_mon_peer,
    .config_lite_mon_tx_peer = target_if_config_lite_mon_tx_peer,
#endif
#ifdef DP_UMAC_HW_RESET_SUPPORT
    .dp_update_tx_hardstart = ol_if_update_tx_hardstart,
#endif
#ifdef WLAN_SUPPORT_SCS
    .peer_scs_rule_match = ol_ath_peer_scs_rule_match,
#endif
#if defined(IPA_WDS_EASYMESH_FEATURE) && defined(FEATURE_AST)
    .peer_send_wds_disconnect = ol_peer_send_wds_disconnect,
#endif
#ifdef CONFIG_SAWF_DEF_QUEUES
    .disable_sawf_svc = ol_ath_ucfg_disable_sawf,
#endif
#if defined(IPA_OFFLOAD) && defined(QCA_IPA_LL_TX_FLOW_CONTROL)
    .get_mlo_chip_id = ol_ath_get_mlo_chip_id,
#endif
#if defined(IPA_OFFLOAD) && defined(IPA_OFFLOAD_LOW_MEM)
   .pdev_get_num_buff = ol_ath_get_num_buff,
#endif
};

int ol_ath_pdev_tpc_config_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_gpio_input_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_nf_dbr_dbm_info_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_packet_power_info_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_generic_buffer_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_peer_mumimo_tx_count_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_peer_gid_userpos_list_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_pdev_caldata_version_check_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_pdev_ctl_failsafe_check_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_soc_hw_mode_change_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_cp_fwstats_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_halphy_fwstats_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_vdev_tsf_report_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_pdev_set_halphy_cal_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
int ol_ath_soc_rf_path_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
#ifdef CONFIG_DP_TRACE
static int ol_ath_dptrace_setparam(int cmd, int val1, int val2);
#endif


#if  QCN_ESP_IE
int ol_ath_esp_estimate_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen);
#endif

extern int whal_mcs_to_kbps(int, int, int, int);
#if ALL_POSSIBLE_RATES_SUPPORTED
    extern int whal_kbps_to_mcs(int, int, int);
    extern int whal_get_supported_rates(int, int, int **);
#else
    extern int whal_kbps_to_mcs(int, int, int, int, int);
    extern int whal_get_supported_rates(int, int, int, int, int **);
#endif
extern int whal_ratecode_to_kbps(uint8_t, uint8_t, uint8_t);
extern void ol_if_mgmt_drain(struct ieee80211_node *ni, int force,
                             enum ieee80211_mgmt_drain_mode drain_mode);
extern int32_t ol_ath_thermal_mitigation_detach(struct ol_ath_softc_net80211 *scn,
                                              struct net_device *dev);
extern int32_t ol_ath_thermal_mitigation_attach(struct ol_ath_softc_net80211 *scn,
                                              struct net_device *dev);
#ifdef IPA_OFFLOAD
extern void ol_ath_ipa_wds_peer_disconnect(struct ieee80211vap *vap, uint8_t *mac);
#endif

#if ATH_SUPPORT_HYFI_ENHANCEMENTS
extern int ath_net80211_add_hmmc(struct ieee80211vap *vap, u_int32_t ip, u_int32_t mask);
extern int ath_net80211_del_hmmc(struct ieee80211vap *vap, u_int32_t ip, u_int32_t mask);
extern void ath_net80211_buffull_handler(struct ieee80211com *ic);
#endif

extern struct global_ic_list ic_list;

extern unsigned int testmode;

#ifdef ATH_AHB
extern int wifi_exit_in_progress;
#endif

int ol_ath_target_stop(struct wlan_objmgr_psoc *psoc, bool flush_wq);
extern int ath_get_radio_index(struct net_device *netdev);
extern int asf_adf_attach(void);

/* Global recovery work queue structure, for maintaining
   recovery across all socs */
struct ol_tgt_recovery_wq *g_recov_wq;

static QDF_STATUS ol_ath_register_crypto_ops_handler(
                        struct wlan_lmac_if_crypto_tx_ops *crypto_tx_ops);
static QDF_STATUS ol_ath_register_crypto_rxpn_ops(
                        struct wlan_lmac_if_crypto_rx_ops *crypto_rx_ops);

/*
 * ol_ath_pri20_cfg_blockchanlist_parse:
 * Block channels listed in CFG from auto channel selection (ACS, ICM, ...)
 */
static void ol_ath_pri20_cfg_blockchanlist_parse(struct ieee80211com *ic,
                                                 const char *cfg_str);

/*
 * ol_ath_parse_wideband_support:
 * Set the support for wideband (5GHz-7GHz) along with CSA-specific flags.
 */
static void ol_ath_parse_wideband_support(struct ieee80211com *ic,
                                          const uint32_t cfg_wideband_csa_val);
enum QDF_GLOBAL_MODE ol_ath_get_conn_mode ()
{
        if (testmode == PLD_FTM) {
          return QDF_GLOBAL_FTM_MODE;
        } else if (testmode == PLD_COLDBOOT_CALIBRATION) {
          return QDF_GLOBAL_COLDBOOT_CALIB_MODE;
        } else if (testmode == PLD_FTM_COLDBOOT_CALIBRATION) {
          return QDF_GLOBAL_FTM_COLDBOOT_CALIB_MODE;
        } else {
          return QDF_GLOBAL_MISSION_MODE;
        }

    return QDF_GLOBAL_MISSION_MODE;
}

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
uint8_t ol_ath_peer_get_num_mlo_links(struct cdp_ctrl_objmgr_psoc *psoc,
                                      uint8_t vdev_id,
                                      uint8_t *peer_macaddr,
                                      bool is_mld_peer)
{
    struct wlan_objmgr_vdev *vdev_obj = NULL;
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;

    vdev_obj = wlan_objmgr_get_vdev_by_id_from_psoc(
                                      (struct wlan_objmgr_psoc *)psoc,
                                      vdev_id, WLAN_OSIF_ID);
    if (!vdev_obj)
        return 0;

    if (!vdev_obj->mlo_dev_ctx) {
        wlan_objmgr_vdev_release_ref(vdev_obj, WLAN_OSIF_ID);
        return 0;
    }

    if (is_mld_peer) {
        mlo_peer_ctx = wlan_mlo_get_mlpeer_by_mld_mac(
                                          vdev_obj->mlo_dev_ctx,
                                          (struct qdf_mac_addr *)peer_macaddr);
    } else {
        mlo_peer_ctx = wlan_mlo_get_mlpeer_by_linkmac(
                                          vdev_obj->mlo_dev_ctx,
                                          (struct qdf_mac_addr *)peer_macaddr);
    }
    wlan_objmgr_vdev_release_ref(vdev_obj, WLAN_OSIF_ID);

    if (!mlo_peer_ctx)
        return 0;

    return mlo_peer_ctx->max_links;
}
#endif

static
int diag_fw_event_handler(ol_soc_t sc, uint8_t *data, uint32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wlan_objmgr_psoc *psoc;
    void *dbglog_handle;
    int ret;
    struct target_psoc_info *tgt_psoc_info;

    psoc = soc->psoc_obj;
    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return -EINVAL;
    }

    if (!(dbglog_handle = target_psoc_get_dbglog_hdl(tgt_psoc_info))) {
        qdf_err("dbglog_handle is null");
        return -EINVAL;
    }

    ret = fwdbg_fw_handler(dbglog_handle, soc, data, datalen);

    return ret;
}

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
struct pld_mlo_param mlo_param;
struct pld_mlo_group_info mlo_arg[PLD_MAX_MLO_GROUPS];

uint8_t ch_idx;
uint8_t grp_idx;
uint8_t chip_bmask;
uint8_t dup_chip_bmask;
uint8_t soc_chip_ids[PLD_MAX_MLO_CHIPS];
uint8_t wsi_order[PLD_MAX_MLO_CHIPS];
uint8_t skip_soc_chip_ids[PLD_MAX_MLO_CHIPS];

enum mlo_global_param_e {
	MAX_NUM_GROUPS,
	MAX_NUM_CHIPS,
	GLOBAL_PARAM_MAX,
};

enum mlo_group_param_e {
	MAX_NUM_PEERS,
	NUM_CHIPS,
	NUM_WSI_CHIPS,
	WSI_ORDER,
	SOC_CHIP_IDS,
	SKIP_SOC_CHIP_IDS,
	GROUP_PARAM_MAX,
};

enum mlo_chip_param_e {
	CHIP_IDX,
	ADJ_CHIP,
	ADJ_CHIP_IDX,
	CHIP_PARAM_MAX,
};

char pld_mlo_param[GLOBAL_PARAM_MAX][PLD_PARAM_STR_LENGTH] = {
	"mlo_max_num_groups",
	"mlo_max_num_chips",
};

char pld_mlo_group_param[GROUP_PARAM_MAX][PLD_PARAM_STR_LENGTH] = {
	"mlo_max_num_peers",
	"mlo_num_chips",
	"mlo_num_wsi_chips",
	"mlo_wsi_order",
	"mlo_soc_chip_ids",
	"mlo_skip_soc_chip_ids",
};

char pld_mlo_chip_param[CHIP_PARAM_MAX][PLD_PARAM_STR_LENGTH] = {
	"mlo_chip_idx",
	"mlo_num_adj_chip",
	"mlo_adj_chip_idx",
};

QDF_STATUS status_g;

/**
 * validate_dynamic_group_info() - MLO group validation function
 * @group_info: MLO group information
 * @max_mlo_chips: maximum number of chips in a group
 *
 * Return: QDF_STATUS of operation
 */
static
QDF_STATUS validate_dynamic_group_info(struct pld_mlo_group_info *group_info,
                                       int max_mlo_chips)
{
    if (group_info->num_chips > max_mlo_chips) {
        qdf_err("ERROR: Invalid no. of chips %u and supported %d\n",
                 group_info->num_chips, max_mlo_chips);
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

/**
 * validate_dynamic_chip_info() - MLO chip validation function
 * @chip_info: MLO chip information
 * @max_mlo_chips: maximum number of chips supported
 *
 * Return: QDF_STATUS of operation
 */
static
QDF_STATUS validate_dynamic_chip_info(struct pld_mlo_chip_info *chip_info,
                                      int max_mlo_chips)
{
    int i = 0;

    if (chip_info->chip_id > max_mlo_chips) {
        qdf_err("ERROR: Invalid chip index %u and supported %d\n",
                chip_info->chip_id, max_mlo_chips);
        return QDF_STATUS_E_FAILURE;
    }

    for (i = 0; i < chip_info->num_adj_chips; i++) {
        if (chip_info->adj_chip_ids[i] > max_mlo_chips) {
            qdf_err("ERROR: Invalid chip index specified in adjacent list %u and supported %d\n",
                     chip_info->adj_chip_ids[i], max_mlo_chips);
            return QDF_STATUS_E_FAILURE;
        }
    }

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS validate_group_info(struct pld_mlo_group_info *group_info,
                                      bool is_dynamic_ini)
{
    int i = 0;

    if (group_info->max_num_peers != PLD_MAX_NUM_PEERS) {
        qdf_err("ERROR: Invalid maximum no. of peers %u\n",
                 group_info->max_num_peers);
        return QDF_STATUS_E_FAILURE;
    }
    if ((group_info->num_chips > PLD_MAX_MLO_CHIPS) ||
        (group_info->num_chips <= 0)) {
        qdf_err("ERROR: Invalid no. of chips %u\n",
                 group_info->num_chips);
        return QDF_STATUS_E_FAILURE;
    }
    for (i = 0; i < group_info->num_chips; i++) {
        if (is_dynamic_ini)
            chip_bmask |= (1 << soc_chip_ids[i]);
        else
            chip_bmask |= (1 << i);

        if (soc_chip_ids[i] > PLD_MAX_MLO_CHIPS) {
            qdf_err("ERROR: Invalid chip index specified in soc list %u\n",
                     soc_chip_ids[i]);
            return QDF_STATUS_E_FAILURE;
        }
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_param_handler(void *context, const char *str_param,
                             const char *str_value)
{
    uint16_t param = 0;

    if (status_g)
        return status_g;

    while (param < GLOBAL_PARAM_MAX) {
        if (qdf_str_eq(pld_mlo_param[param], str_param)) {
            switch(param) {
            case MAX_NUM_GROUPS:
                status_g = qdf_uint8_parse(str_value,
                                            &mlo_param.mlo_max_groups);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get max. number of groups status: %d", status_g);
                    return status_g;
                }
                if ((mlo_param.mlo_max_groups > PLD_MAX_MLO_GROUPS) ||
                    (mlo_param.mlo_max_groups <= 0)) {
                    qdf_err("%s=%u - Out of range [%d, %d]", str_param, mlo_param.mlo_max_groups, PLD_MIN_MLO_GROUPS, PLD_MAX_MLO_GROUPS);
                    status_g = QDF_STATUS_E_FAILURE;
                    return status_g;
                }
                break;
            case MAX_NUM_CHIPS:
                status_g = qdf_uint8_parse(str_value,
                                            &mlo_param.mlo_max_chips);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get max. number of chips status: %d", status_g);
                    return status_g;
                }
                if ((mlo_param.mlo_max_chips > PLD_MAX_MLO_CHIPS) ||
                    (mlo_param.mlo_max_chips <= 0)) {
                    qdf_err("%s=%u - Out of range [%d, %d]", str_param, mlo_param.mlo_max_chips, PLD_MIN_MLO_CHIPS, PLD_MAX_MLO_CHIPS);
                    status_g = QDF_STATUS_E_FAILURE;
                    return status_g;
                }
                break;
            default:
                break;
            }
            if (QDF_IS_STATUS_SUCCESS(status_g)) {
                return QDF_STATUS_SUCCESS;
            }
        }
        param++;
    }
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_group_param_handler(void *context, const char *str_param,
                                   const char *str_value)
{
    uint16_t param=0;
    size_t out_size = 0;
    int i = 0;
    struct pld_mlo_group_info *group_info = &mlo_arg[grp_idx];

    if (status_g)
        return status_g;

    while (param < GROUP_PARAM_MAX) {
        if (qdf_str_eq(pld_mlo_group_param[param], str_param)) {
            switch(param) {
            case MAX_NUM_PEERS:
                status_g = qdf_uint16_parse(str_value,
                                          &group_info->max_num_peers);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get max. number of peers in group %u", grp_idx);
                    return status_g;
                }
                break;
            case NUM_CHIPS:
                status_g = qdf_uint8_parse(str_value, &group_info->num_chips);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get number of chips in group: %u", grp_idx);
                    return status_g;
                }
                if (group_info->num_chips <= 0) {
                    status_g = QDF_STATUS_E_FAILURE;
                    qdf_err("ERROR: Vaild number of chips should be specify %s in group: %u", str_param, grp_idx);
                    return status_g;
                }
                break;
            case SOC_CHIP_IDS:
                status_g = qdf_uint8_array_parse(str_value, soc_chip_ids,
                                                group_info->num_chips,
                                                &out_size);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get soc chip IDs in group %u", grp_idx);
                    return status_g;
                }
                if (group_info->num_chips != out_size) {
                    status_g = QDF_STATUS_E_FAILURE;
                    qdf_err("ERROR: Number of chips and list in %s should match in group %u ", str_param, grp_idx);
                    return status_g;
                }
                for (i = 0; i < group_info->num_chips; i++) {
                    if (group_info->soc_chip_bitmap & (1 << soc_chip_ids[i])) {
                        qdf_err("ERROR: Duplicate chip index specified in soc list %u\n",
                                 soc_chip_ids[i]);
                        status_g = QDF_STATUS_E_FAILURE;
                        return status_g;
                    }
                    group_info->soc_chip_bitmap |= (1 << soc_chip_ids[i]);
                }
                break;
            case NUM_WSI_CHIPS:
                status_g = qdf_uint8_parse(str_value, &group_info->num_wsi_chips);
                break;
            case WSI_ORDER:
                status_g = qdf_uint8_array_parse(str_value, wsi_order,
                                                group_info->num_wsi_chips,
                                                &out_size);
                for (i = 0; i < group_info->num_wsi_chips; i++) {
                    if (group_info->wsi_order_bitmap & (1 << wsi_order[i])) {
                        qdf_err("ERROR: Duplicate soc index specified in wsi list %u\n",
                                 wsi_order[i]);
                        status_g = QDF_STATUS_E_FAILURE;
                        return status_g;
                    }
                    group_info->wsi_order_bitmap |= (1 << wsi_order[i]);
                }
                break;
            case SKIP_SOC_CHIP_IDS:
                status_g = qdf_uint8_array_parse(str_value, skip_soc_chip_ids,
                                                  group_info->num_chips,
                                                  &out_size);
                for (i = 0; i < out_size; i++) {
                    if (group_info->skip_soc_chip_bitmap & (1 << skip_soc_chip_ids[i])) {
                        qdf_err("ERROR: Duplicate soc index specified in soc list %u\n",
                                 skip_soc_chip_ids[i]);
                        status_g = QDF_STATUS_E_FAILURE;
                        return status_g;
                    }
                    group_info->skip_soc_chip_bitmap |= (1 << skip_soc_chip_ids[i]);
                }
                break;
            default:
                break;
            }
            if (QDF_IS_STATUS_SUCCESS(status_g)) {
                return QDF_STATUS_SUCCESS;
            }
        }
        param++;
    }

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS validate_chip_info(struct pld_mlo_chip_info *chip_info,
                                     int chip_sec, uint8_t soc_chip_bitmap,
                                     bool is_dynamic_ini)
{
    int i = 0;
    uint8_t adj_chip_bmask = 0;

    if ((chip_info->chip_id > PLD_MAX_MLO_CHIPS) ||
        !((is_dynamic_ini && (soc_chip_bitmap & (1 << chip_info->chip_id))) ||
        (!is_dynamic_ini && (chip_bmask & (1 << chip_info->chip_id))))) {
        qdf_err("ERROR: Invalid chip index for chip section %u; %u\n",
                 chip_sec, chip_info->chip_id);
        return QDF_STATUS_E_FAILURE;
    }

    if (dup_chip_bmask & (1 << chip_info->chip_id)) {
        qdf_err("ERROR: Duplicate chip index specified for chip section %u; %u\n",
                 chip_sec, chip_info->chip_id);
        return QDF_STATUS_E_FAILURE;
    }
    dup_chip_bmask |= (1 << chip_info->chip_id);

    if ((chip_info->num_adj_chips > PLD_MAX_LINKS_PER_CHIP) ||
        (chip_info->num_adj_chips < 0)) {
        qdf_err("ERROR: Invalid adjacent num chips for chip section %u; %u\n",
                 chip_sec, chip_info->num_adj_chips);
        return QDF_STATUS_E_FAILURE;
    }

    for (i = 0; i < chip_info->num_adj_chips; i++) {
        if ((chip_info->adj_chip_ids[i] > PLD_MAX_MLO_CHIPS) ||
            (chip_info->adj_chip_ids[i] < 0)) {
            qdf_err("ERROR: Invalid chip index specified in adjacent list %u\n",
                     chip_info->adj_chip_ids[i]);
            return QDF_STATUS_E_FAILURE;
        }
        if (adj_chip_bmask & (1 << chip_info->adj_chip_ids[i])) {
            qdf_err("ERROR: Duplicate chip index specified in adjacent list %u\n",
                     chip_info->adj_chip_ids[i]);
            return QDF_STATUS_E_FAILURE;
        }
        adj_chip_bmask |= (1 << chip_info->adj_chip_ids[i]);

        if ((chip_info->chip_id == chip_info->adj_chip_ids[i]) ||
            !((is_dynamic_ini && (soc_chip_bitmap & (1 << chip_info->adj_chip_ids[i]))) ||
            (!is_dynamic_ini && (chip_bmask & (1 << chip_info->adj_chip_ids[i]))))) {
            qdf_err("ERROR: Invalid adjacent chip index for chip section %u; %u\n",
                    chip_sec, chip_info->chip_id);
            return QDF_STATUS_E_FAILURE;
        }
    }
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlo_chip_param_handler(void *context, const char *str_param,
                                  const char *str_value)
{
    uint16_t param = 0;
    size_t out_size = 0;
    struct pld_mlo_chip_info *chip_info =
                                     &mlo_arg[grp_idx].chip_info[ch_idx];

    if (status_g)
        return status_g;

    while (param < CHIP_PARAM_MAX) {
        if (qdf_str_eq(pld_mlo_chip_param[param], str_param)) {
            switch(param) {
            case CHIP_IDX:
                status_g = qdf_uint8_parse(str_value, &chip_info->chip_id);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get chip_id for chip section %u status:%d",
                            ch_idx, status_g);
                    return status_g;
                }
                break;
            case ADJ_CHIP:
                status_g = qdf_uint8_parse(str_value,
                                            &chip_info->num_adj_chips);
                if (chip_info->num_adj_chips == 0 &&
                    mlo_arg[grp_idx].num_chips == 1)
                    break;
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get num_adj_chips for chip section %u status:%d",
                            ch_idx, status_g);
                    return status_g;
                }
                if (chip_info->num_adj_chips < 0) {
                    status_g = QDF_STATUS_E_FAILURE;
                    qdf_err("ERROR: num_adj_chips entry is not found %u\n",
                             chip_info->num_adj_chips);
                    return status_g;
                }
                break;
            case ADJ_CHIP_IDX:
                if (chip_info->num_adj_chips == 0 &&
                    mlo_arg[grp_idx].num_chips == 1)
                    break;
	        status_g = qdf_uint8_array_parse(str_value,
                                                chip_info->adj_chip_ids,
                                                chip_info->num_adj_chips,
                                                &out_size);
                if (QDF_IS_STATUS_ERROR(status_g)) {
                    qdf_err("ERROR: Failed to get adj_chip_ids for chip section %u status:%d",
                            ch_idx, status_g);
                    return status_g;
                }
                if (chip_info->num_adj_chips != out_size) {
                    status_g = QDF_STATUS_E_FAILURE;
                    qdf_err("ERROR: Invalid no. of adjacent list %u",
                            status_g);
                    return status_g;
                }
                break;
            default:
                break;
            }
            if (QDF_IS_STATUS_SUCCESS(status_g)) {
                return QDF_STATUS_SUCCESS;
            }
        }
        param++;
    }

    return QDF_STATUS_SUCCESS;
}

/**
 * ol_ath_parse_mlo_params_section() - INI section parsing function
 * @group_id: group ID
 * @path: INI path
 *
 * Return: QDF_STATUS of operation
 */
static QDF_STATUS ol_ath_parse_mlo_params_section(uint8_t group_id, char *path,
                                                  bool is_dynamic_ini)
{
    QDF_STATUS status;
    char sec[32] = {0};

    memset(sec, 0, sizeof(sec));
    snprintf(sec, sizeof(sec), "MLO_GROUP_%u", group_id);
    memset(&mlo_arg[group_id], 0, sizeof(struct pld_mlo_group_info));
    status_g = 0;
    dup_chip_bmask = 0;
    chip_bmask = 0;
    memset(soc_chip_ids, 0, PLD_MAX_MLO_CHIPS);
    memset(wsi_order, 0, PLD_MAX_MLO_CHIPS);
    memset(skip_soc_chip_ids, 0, PLD_MAX_MLO_CHIPS);

    status = qdf_ini_section_parse(path, NULL, mlo_group_param_handler,
                                   sec);
    if (QDF_IS_STATUS_ERROR(status) || QDF_IS_STATUS_ERROR(status_g)) {
        qdf_err("ERROR: Failed to parse *.ini file @ %s; status:%d",
                path, status_g);
        return QDF_STATUS_E_FAILURE;
    }

    status = validate_group_info(&mlo_arg[group_id], is_dynamic_ini);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("ERROR: Invalid entries in group section %u status:%d",
                 group_id, status);
        return status;
    }

    for (ch_idx = 0; ch_idx < mlo_arg[group_id].num_chips; ch_idx++) {
        memset(sec, 0, sizeof(sec));
        snprintf(sec, sizeof(sec), "MLO_SOC_CHIP_%u", soc_chip_ids[ch_idx]);
        memset(&mlo_arg[group_id].chip_info[ch_idx].chip_id, 0xff,
               sizeof(u8));
        status_g = 0;

        status = qdf_ini_section_parse(path, NULL,
                                       mlo_chip_param_handler, sec);
        if (QDF_IS_STATUS_ERROR(status) || QDF_IS_STATUS_ERROR(status_g)) {
            qdf_err("ERROR: Failed to parse *.ini file @ %s; status:%d",
                    path, status_g);
            return QDF_STATUS_E_FAILURE;
        }

        if (!is_dynamic_ini &&
            mlo_arg[group_id].chip_info[ch_idx].chip_id != ch_idx) {
            qdf_err("ERROR: Invalid chip index for chip section %u; %u\n",
                    soc_chip_ids[ch_idx],
                    mlo_arg[group_id].chip_info[ch_idx].chip_id);
            return QDF_STATUS_E_FAILURE;
        }
        status = validate_chip_info(&mlo_arg[group_id].chip_info[ch_idx],
                                    soc_chip_ids[ch_idx],
                                    mlo_arg[group_id].soc_chip_bitmap,
                                    is_dynamic_ini);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("ERROR: Invalid entries in chip section %u status:%d",
                     ch_idx, status);
            return status;
        }
    }

    return QDF_STATUS_SUCCESS;
}

/**
 * ol_ath_parse_mlo_params_from_ini() - INI parsing function
 * @is_dynamic_ini: flag to differentiate two different INI files
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS ol_ath_parse_mlo_params_from_ini(bool is_dynamic_ini)
{
    QDF_STATUS status;
    int group_id;
    char *path = "mlo_config.ini";

    if (is_dynamic_ini)
        path = "dynamic_wsi.ini";

    grp_idx = 0;
    status_g = 0;
    memset(&mlo_param, 0, sizeof(struct pld_mlo_param));
    status = qdf_ini_parse(path, NULL, mlo_param_handler, NULL);
    if (QDF_IS_STATUS_ERROR(status) || QDF_IS_STATUS_ERROR(status_g)) {
        mlo_param.mlo_default_cfg = true;
        return QDF_STATUS_SUCCESS;
    }

    if (mlo_param.mlo_max_groups > PLD_MAX_MLO_GROUPS) {
        qdf_err("ERROR: Invalid max. number of groups %u\n",
                mlo_param.mlo_max_groups);
        return QDF_STATUS_E_FAILURE;
    }

    if (is_dynamic_ini && (mlo_param.mlo_max_groups >= PLD_MAX_MLO_GROUPS)) {
        qdf_err("ERROR: Invalid max. number of group %u in %s\n",
                mlo_param.mlo_max_groups, path);
        return QDF_STATUS_E_FAILURE;
    }

    for (group_id = 0; group_id < mlo_param.mlo_max_groups; group_id++)
        if (ol_ath_parse_mlo_params_section(group_id, path, is_dynamic_ini))
            return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}

void init_mlo_cfg(void)
{
    int ret = 0;

    status_g = 0;
    ret = ol_ath_parse_mlo_params_from_ini(false);
    if (!ret) {
        if (mlo_param.mlo_default_cfg)
            qdf_info("Set default MLO config");
        else
            qdf_info("Set MLO configuration from INI file");

        ret = pld_reset_mlo_config(mlo_param.mlo_max_groups);
        if (ret) {
            qdf_err("%s: MLO reset failed %d", __func__, ret);
            qdf_info("Try default MLO config");
            mlo_param.mlo_default_cfg = true;
        }

        ret = pld_set_mlo_config(mlo_arg, &mlo_param);
        if (ret) {
            qdf_err("%s: MLO set failed %d", __func__, ret);
            return;
        }
    } else {
        qdf_err("ERROR: Set MLO configuration from INI file");
        memset(&mlo_param, 0, sizeof(struct pld_mlo_param));
        memset(mlo_arg, 0, sizeof(struct pld_mlo_group_info) * PLD_MAX_MLO_GROUPS);
    }
}
qdf_export_symbol(init_mlo_cfg);

void deinit_mlo_cfg(void)
{
    mlo_info("Reset MLO configuration");
    memset(&mlo_param, 0, sizeof(struct pld_mlo_param));
    memset(soc_chip_ids, 0, PLD_MAX_MLO_CHIPS);
    memset(wsi_order, 0, PLD_MAX_MLO_CHIPS);
    memset(skip_soc_chip_ids, 0, PLD_MAX_MLO_CHIPS);
    memset(mlo_arg, 0, sizeof(struct pld_mlo_group_info) * PLD_MAX_MLO_GROUPS);
}
qdf_export_symbol(deinit_mlo_cfg);

/**
 * check_path_exists() - check file is present or not in a given path
 * @path: file path
 *
 * Return: true if file is present otherwise false
 */
static bool check_path_exists(const char* path)
{
    struct file* filp = NULL;

    if (!path)
        return false;

    filp = filp_open(path, O_RDONLY, 00644);
    if (IS_ERR(filp)) {
        qdf_warn("Path %s doesnt exist", path);
        return false;
    }

    filp_close(filp, NULL);
    return true;
}

/**
 * ol_ath_dynamic_wsi_supported() - check dynamic WSI is supported
 * @group_id: group ID
 * @soc: soc context
 *
 * Return: QDF_STATUS of operation
 */
static QDF_STATUS ol_ath_dynamic_wsi_supported(uint8_t group_id,
                                               ol_ath_soc_softc_t *soc)
{
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
    bool is_wsi_add = soc->psoc_obj->wsi_remap_add;
    char dynamic_ini[64] = "/ini/dynamic_wsi.ini";
    char static_ini[64] = "/ini/mlo_config.ini";
    struct device *dev = soc->qdf_dev->dev;
    struct mlo_setup_info *setup_info;
    int max_mlo_chips = 0;
    int ret, ch_idx = 0;

    status_g = 0;
    max_mlo_chips = pld_get_max_mlo_chips(dev);
    if (max_mlo_chips <= 0 || max_mlo_chips > PLD_MAX_MLO_CHIPS) {
        qdf_err("%s: Invalid Max MLO chip %d", __func__, max_mlo_chips);
        return QDF_STATUS_E_INVAL;
    }

    /*
     * check mlo_config.ini file is present or not. If not, the system
     * is booted with full scale by default configuration.
     */
    if (check_path_exists(static_ini)) {
        ret = ol_ath_parse_mlo_params_from_ini(false);
        if (ret || mlo_param.mlo_default_cfg) {
            qdf_err("%s: MLO config INI parse failed %d", __func__, ret);
            return QDF_STATUS_E_INVAL;
        }

        if (mlo_arg[group_id].num_chips != max_mlo_chips) {
            qdf_err("%s: System not booted with Max capability", __func__);
            return QDF_STATUS_E_INVAL;
        }

        if (validate_dynamic_group_info(&mlo_arg[group_id], max_mlo_chips))
            return QDF_STATUS_E_INVAL;

        for (ch_idx = 0; ch_idx < mlo_arg[group_id].num_chips; ch_idx++)
            if (validate_dynamic_chip_info(&mlo_arg[group_id].chip_info[ch_idx],
                                           max_mlo_chips))
                return QDF_STATUS_E_INVAL;
    }

    /*
     * check dynamic_wsi.ini file is present or not. If not, return error.
     */
    if (!check_path_exists(dynamic_ini)) {
        qdf_err("File not found %s", dynamic_ini);
        return QDF_STATUS_E_INVAL;
    }

    ret = ol_ath_parse_mlo_params_from_ini(true);
    if (ret || mlo_param.mlo_default_cfg) {
        qdf_err("%s: MLO config INI parse failed %d", __func__, ret);
        return QDF_STATUS_E_INVAL;
    }

    if (validate_dynamic_group_info(&mlo_arg[group_id], max_mlo_chips))
        return QDF_STATUS_E_INVAL;

    for (ch_idx = 0; ch_idx < mlo_arg[group_id].num_chips; ch_idx++) {
        if (validate_dynamic_chip_info(&mlo_arg[group_id].chip_info[ch_idx],
                                       max_mlo_chips))
            return QDF_STATUS_E_INVAL;

    }

    /*
     * check if given chip section is removed from the dynamic_wsi.ini
     * file before proceeding dynamic WSI command.
     */
    if ((!is_wsi_add) && (chip_bmask & (1 << soc->soc_idx))) {
        qdf_err("Error: Chip details are not removed from INI file");
        return QDF_STATUS_E_INVAL;
    }

    /*
     * check if given chip section is added in the dynamic_wsi.ini
     * file before proceeding dynamic WSI command.
     */
    if (is_wsi_add && !(chip_bmask & (1 << soc->soc_idx))) {
        qdf_err("Error: Chip details are not added in INI file");
        return QDF_STATUS_E_INVAL;
    }

    /*
     * check whether remapping more than one chip at a time
     * before proceeding dynamic WSI command.
     */
    if (!mlo_ctx || !mlo_ctx->setup_info) {
        qdf_err("Error: MLO context or setup is NULL");
        return QDF_STATUS_E_INVAL;
    }
    setup_info = &mlo_ctx->setup_info[group_id];
    if (!(abs(mlo_arg[group_id].num_chips - setup_info->tot_links) <= 1)) {
        qdf_err("Error: MLO remap supported only one chip at a time");
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
QDF_STATUS ol_ath_dynamic_wsi_remap(ol_ath_soc_softc_t *soc)
{
    int group_id = 0;
    int ret = 0;

    if (!soc) {
        qdf_err("SOC is Null");
        return QDF_STATUS_E_INVAL;
    }

    if (!pld_get_mlo_capable(soc->qdf_dev->dev)) {
        qdf_err("SOC is non MLO capable");
        return QDF_STATUS_E_FAILURE;
    }

    ret = ol_ath_dynamic_wsi_supported(group_id, soc);
    if (ret) {
        qdf_err("%s: Dynamic feature not supported %d", __func__, ret);
        return ret;
    }

    qdf_info("WSI Dynamic Remap started");
    /* Teardown the MLO in a group */
    ret = ol_ath_dynamic_wsi_remap_mlo_teardown(soc);
    if (ret) {
        qdf_err("%s: Teardown MLO config failed %d", __func__, ret);
        return ret;
    }

    if (soc->psoc_obj->wsi_remap_add)
        ol_ath_parse_mlo_params_from_ini(false);
    else
        ol_ath_parse_mlo_params_from_ini(true);

    /* clear the MLO flag and populate the data to platform data */
    ret = pld_reset_mlo_group_config(mlo_param.mlo_max_groups);
    if (ret) {
        qdf_err("%s: MLO reset failed %d", __func__, ret);
        return QDF_STATUS_E_INVAL;
    }
    ret = pld_set_mlo_config(mlo_arg, &mlo_param);
    if (ret) {
        qdf_err("%s: MLO set config failed %d", __func__, ret);
        return QDF_STATUS_E_INVAL;
    }

    /* Bringup up the FW during Dynamic WSI remap add phase */
    if (soc->psoc_obj->wsi_remap_add) {
        soc->target_ready = 1;
        soc->psoc_obj->wsi_remap_fw_up_in_progress = true;
        pld_set_wsi_remap_state(soc->qdf_dev->dev, true);
        pld_subsystem_put(soc->qdf_dev->dev);
        if (ol_ath_target_start(soc)) {
            qdf_err("Start MLD target failed for soc id %d",soc->soc_idx);
            soc->psoc_obj->wsi_remap_fw_up_in_progress = false;
            return QDF_STATUS_E_INVAL;
        }
        soc->psoc_obj->wsi_remap_fw_up_in_progress = false;

        ol_ath_parse_mlo_params_from_ini(true);

        ret = pld_reset_mlo_group_config(mlo_param.mlo_max_groups);
        if (ret) {
            qdf_err("%s: MLO reset failed %d", __func__, ret);
            return QDF_STATUS_E_INVAL;
        }
        ret = pld_set_mlo_config(mlo_arg, &mlo_param);
        if (ret) {
            qdf_err("%s: MLO set config failed %d", __func__, ret);
            return QDF_STATUS_E_INVAL;
        }
    } else {
        pld_set_wsi_remap_state(soc->qdf_dev->dev, true);
    }

    /* Trigger QMI message to FW */
    ret = pld_set_wsi_remap(soc->qdf_dev->dev);
    if (ret) {
        qdf_err("%s: MLO config QMI message is failed %d", __func__, ret);
        return ret;
    }

    /* Perform the MLO setup of the current active Chips in a group */
    ret = ol_ath_dynamic_wsi_remap_mlo_setup(soc);
    if (ret) {
        qdf_err("%s: Setup MLO config failed %d", __func__, ret);
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ol_ath_dynamic_wsi_remap);
#endif

#endif

#ifdef DP_UMAC_HW_RESET_SUPPORT
void ol_if_update_tx_hardstart(struct cdp_ctrl_objmgr_psoc *psoc,
                               uint8_t vdev_id, struct ol_txrx_hardtart_ctxt *ctxt)
{
    struct ieee80211vap *vap;
    osif_dev *osifp;
    struct wlan_objmgr_vdev *vdev;

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc((struct wlan_objmgr_psoc *)psoc,
                                                vdev_id,
                                                WLAN_UMAC_RESET_ID);

    if (!vdev)
        return;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);

    if (!vap) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_UMAC_RESET_ID);
        return;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    spin_lock_bh(&osifp->tx_lock);
    osifp->iv_vap_send = ctxt->tx;
    osifp->iv_vap_fast_send = ctxt->tx_fast;
    osifp->iv_vap_send_exc = ctxt->tx_exception;

    spin_unlock_bh(&osifp->tx_lock);

    wlan_objmgr_vdev_release_ref(vdev, WLAN_UMAC_RESET_ID);
}
#endif

#ifdef INTRA_BSS_FWD_OFFLOAD
void ol_ath_vdev_set_intra_bss_val(struct cdp_ctrl_objmgr_psoc *psoc,
                                   uint16_t vdev_id, bool enable)
{
    ol_ath_vdev_set_intra_bss((struct wlan_objmgr_psoc *)psoc, vdev_id, enable);
}
#endif
#ifdef FEATURE_MEC_OFFLOAD
QDF_STATUS
ol_ath_set_mec_timer(struct cdp_ctrl_objmgr_psoc *psoc,
                     uint8_t vdev_id, uint16_t mec_timer_val)
{
    void *pdev_wmi_handle;
    struct set_mec_timer_params param;
    QDF_STATUS status = QDF_STATUS_E_FAILURE;
    struct wlan_objmgr_vdev *vdev;

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc((struct wlan_objmgr_psoc *)psoc,
                                                vdev_id,
                                                WLAN_VDEV_TARGET_IF_ID);
    if (!vdev) {
        qdf_err("vdev is NULL");
        return status;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(wlan_vdev_get_pdev(vdev));
    if (!pdev_wmi_handle) {
        qdf_err("WMI handle is NULL");
        status = QDF_STATUS_E_FAILURE;
        goto end;
    }

    /* pdev_id is 0 because it is soc level wmi */
    param.pdev_id = 0;
    param.vdev_id = vdev_id;
    param.mec_aging_timer_threshold = mec_timer_val;
    wmi_unified_pdev_set_mec_timer(pdev_wmi_handle, &param);

end:
    wlan_objmgr_vdev_release_ref(vdev, WLAN_VDEV_TARGET_IF_ID);
    return QDF_STATUS_SUCCESS;
}
#endif

static inline void ol_ath_dbglog_detach(ol_ath_soc_softc_t *soc)
{
        struct target_psoc_info *tgt_psoc_info;
        void *dbglog_handle = NULL;
        struct wmi_unified *wmi_handle = NULL;

        tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
        dbglog_handle = target_psoc_get_dbglog_hdl(tgt_psoc_info);

        wmi_handle = (struct wmi_unified *)target_psoc_get_wmi_hdl(tgt_psoc_info);
        if (!wmi_handle) {
            qdf_err("null wmi_handle");
            return;
        }
        wmi_unified_unregister_event_handler(wmi_handle, wmi_diag_event_id);
        target_psoc_set_dbglog_hdl(tgt_psoc_info, NULL);

        if (dbglog_handle) {
#ifdef OL_ATH_SMART_LOGGING
                fwdbg_smartlog_deinit(dbglog_handle, soc);
#endif /* OL_ATH_SMART_LOGGING */
                fwdbg_free(dbglog_handle, soc);
                if (soc->ol_if_ops->dbglog_detach)
                        soc->ol_if_ops->dbglog_detach(dbglog_handle);
        } else {
               qdf_err("dbglog_handle is null ");
        }
}

int
ol_ath_get_dp_config_param(struct cdp_ctrl_objmgr_psoc *psoc,
		enum cdp_cfg_param_type param_num)
{
	switch(param_num)
	{
		case  CDP_CFG_MAX_PEER_ID:
			return ol_cfg_max_peer_id((ol_soc_handle)psoc);
		case CDP_CFG_CCE_DISABLE:
			return ol_cfg_is_cce_disable((ol_soc_handle)psoc);
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
		case CDP_CFG_MLD_NETDEV_MODE_AP:
			return ol_cfg_get_mld_netdev_mode_ap((ol_soc_handle)psoc);
#endif
		default:
			return -EINVAL;
	}
}

int
ol_ath_get_soc_nss_cfg(struct cdp_ctrl_objmgr_psoc *psoc)
{
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    ol_ath_soc_softc_t *soc;
    struct wlan_objmgr_psoc *psoc_obj = (struct wlan_objmgr_psoc *)psoc;
    soc = lmac_get_psoc_feature_ptr(psoc_obj);

    if (!soc)
        return 0;

    return soc->nss_soc.nss_scfg;
#else
    return 0;
#endif
}

#ifdef WLAN_SUPPORT_PPEDS
QDF_STATUS
ol_ath_get_ppeds_profile_info_for_vap(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id,
                              struct cdp_ds_vp_params *vp_params)
{
    osif_dev *osdev;
    struct wlan_objmgr_vdev *vdev;
    struct ieee80211vap *vap;

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc((struct wlan_objmgr_psoc *)psoc,
                vdev_id, WLAN_VDEV_TARGET_IF_ID);
    if (!vdev) {
        qdf_err("vdev is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    vap = wlan_vdev_get_vap(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_VDEV_TARGET_IF_ID);
        return QDF_STATUS_E_NULL_VALUE;
    }

    osdev = (osif_dev *)vap->iv_ifp;

    vp_params->ppe_vp_profile_idx = osdev->ppe_vp_profile_idx;
    vp_params->ppe_vp_type = osdev->ppe_vp_type;
    wlan_objmgr_vdev_release_ref(vdev, WLAN_VDEV_TARGET_IF_ID);
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
QDF_STATUS
ol_ath_nss_stats_clr(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_vdev *vdev;
    struct ieee80211vap *vap;
    QDF_STATUS ret = QDF_STATUS_E_INVAL;

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc((struct wlan_objmgr_psoc *)psoc,
                                               vdev_id, WLAN_VDEV_TARGET_IF_ID);
    if (!vdev) {
        qdf_err("vdev is null");
        return ret;
    }

    vap = wlan_vdev_get_vap(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_VDEV_TARGET_IF_ID);
        return ret;
    }

    ic = vap->iv_ic;
    scn = OL_ATH_SOFTC_NET80211(ic);
    ret = scn->sc_ic.nss_radio_ops->ic_nss_ol_nss_stats_clr(scn, vdev_id);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_VDEV_TARGET_IF_ID);
    return ret;
}
#else
QDF_STATUS
ol_ath_nss_stats_clr(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id)
{
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef CONFIG_WIFI_EMULATION_WIFI_3_0
static bool is_emulation(struct wlan_objmgr_psoc *psoc)
{
     bool emu = cfg_get(psoc, CFG_OL_EMULATION);

     return emu;
}
#else
static bool is_emulation(struct wlan_objmgr_psoc *psoc)
{
     return false;
}
#endif

void ol_ath_get_pdev_stats(void *arg)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)arg;
    struct wlan_objmgr_pdev *pdev = scn->sc_pdev;
    struct stats_request_params param;
    uint8_t addr[QDF_MAC_ADDR_SIZE];
    uint8_t *myaddr;
    wmi_unified_t pdev_wmi_handle;

#ifdef CONFIG_WIFI_EMULATION_WIFI_3_0
    if (is_emulation(scn->soc->psoc_obj)) {
        qdf_debug("Skip stats on emulation");
        return;
    }
#endif

    pdev_wmi_handle = lmac_get_pdev_wmi_unified_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        qdf_err("WMI handle is NULL");
        return;
    }
    memset(&param, 0, sizeof(param));
    wlan_pdev_obj_lock(pdev);
    myaddr = wlan_pdev_get_hw_macaddr(pdev);
    IEEE80211_ADDR_COPY(addr, myaddr);
    wlan_pdev_obj_unlock(pdev);

    param.pdev_id = lmac_get_pdev_idx(pdev);
    param.stats_id = WMI_HOST_REQUEST_PDEV_STAT;
    wmi_unified_stats_request_send(pdev_wmi_handle, addr, &param);
    if (scn->is_scn_stats_timer_init && scn->pdev_stats_timer) {
        qdf_timer_mod(&(scn->scn_stats_timer),scn->pdev_stats_timer);
    }
}
qdf_export_symbol(ol_ath_get_pdev_stats);

void ol_tbtt_sync_timer_cb(void *arg)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)arg;
    struct wlan_objmgr_pdev *pdev = scn->sc_pdev;
    wlan_dev_t ic;
    wmi_unified_t pdev_wmi_handle;
    struct rnr_tbtt_multisoc_sync_param tbtt_multisoc_sync_param;


    if (!pdev) {
        QDF_TRACE(QDF_MODULE_ID_6GHZ, QDF_TRACE_LEVEL_DEBUG,
                  "%s:%d: pdev is NULL!!", __func__, __LINE__);
        return;
    }
    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        QDF_TRACE(QDF_MODULE_ID_6GHZ, QDF_TRACE_LEVEL_DEBUG,
                  "%s:%d: ic is NULL!!", __func__, __LINE__);
        return;
    }
    if (ieee80211_get_num_ap_vaps_up(ic)) {
        pdev_wmi_handle = lmac_get_pdev_wmi_unified_handle(pdev);
        tbtt_multisoc_sync_param.cmd_type = WMI_HOST_PDEV_GET_TBTT_OFFSET;
        tbtt_multisoc_sync_param.pdev_id = WMI_HOST_PDEV_ID_SOC;
        tbtt_multisoc_sync_param.rnr_vap_count = 0;

        if (!pdev_wmi_handle) {
            QDF_TRACE(QDF_MODULE_ID_6GHZ, QDF_TRACE_LEVEL_DEBUG,
                  "WMI handle is NULL");
            return;
        }
        wmi_unified_multisoc_tbtt_sync_cmd(pdev_wmi_handle, &tbtt_multisoc_sync_param);
    }
    qdf_timer_mod(&(scn->soc->tbtt_offset_sync_timer), DEFAULT_TBTT_SYNC_TIMER);

}

void ol_ath_clear_auth_cnt(void *arg)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)arg;
    qdf_atomic_init(&(scn->auth_cnt));
    qdf_timer_mod(&(scn->auth_timer), DEFAULT_AUTH_CLEAR_TIMER);
}

/*
 * ol_ath_pri20_cfg_blockchanlist_parse:
 * Parse the CFG string param containing channel numbers to block from
 * channel selection (ACS, ICM, ...)
 *
 * Arguments:
 * ic     : ic handle (containing channel lists)
 * cfg_str: CFG string parameter
 *
 * Return:
 * None
 */
static void ol_ath_pri20_cfg_blockchanlist_parse(struct ieee80211com *ic,
                                                 const char *cfg_str)
{
    uint8_t ix = 0;
    int offset = 0;
    const char *str_ptr = cfg_str;

    if (cfg_str == NULL || cfg_str[0] == '\0') {
        qdf_info("cfg block channel list is empty");
        return;
    }

    while (*str_ptr != '\0') {
        switch(*str_ptr) {
            case ',':
                if (ix == (ACS_MAX_CHANNEL_COUNT-1)) {
                    qdf_info("Max channel count reached, discarding cfg param");
                    qdf_mem_zero(&ic->ic_pri20_cfg_blockchanlist,
                                 sizeof(struct ieee80211_user_freq_list));
                    return;
                }
                ix++;
                str_ptr++;
                break;
            case ' ' :
            case '\"':
            case '\'':
                /* Skipping spaces, single and double-inverted commas */
                str_ptr++;
                continue;
            default :
                if (*str_ptr >= '0' && *str_ptr <= '9') {
                    if(!sscanf(str_ptr, "%hu %n",
                               &ic->ic_pri20_cfg_blockchanlist.freq[ix],
                               &offset)) {
                        qdf_info("Could not parse channel frequency from cfg param, discarding cfg param");
                        qdf_mem_zero(&ic->ic_pri20_cfg_blockchanlist,
                                     sizeof(struct ieee80211_user_freq_list));
                        return;
                    }
                    str_ptr += offset;
                    ic->ic_pri20_cfg_blockchanlist.n_freq++;
                    QDF_TRACE(QDF_MODULE_ID_ACS,
                              QDF_TRACE_LEVEL_INFO,
                              "(%hu) channel frequency %hu blocked from cfg",
                              ic->ic_pri20_cfg_blockchanlist.n_freq,
                              ic->ic_pri20_cfg_blockchanlist.freq[ix]);
                    break;
                } else {
                    qdf_info("Invalid character in string, discarding cfg param");
                    qdf_mem_zero(&ic->ic_pri20_cfg_blockchanlist,
                                 sizeof(struct ieee80211_user_freq_list));
                    return;
                }
        }
    }

    return;
}

void ol_ath_cohost_bssid_ind_init(struct ol_ath_softc_net80211 *scn)
{
    struct ieee80211com *ic;
    struct wlan_objmgr_psoc *psoc;
    uint8_t cohosted_bss_enable_bitmap;
    uint8_t pdev_id;
    uint8_t max_vdevs;

    if (!scn) {
        qdf_err("scn is null");
        return;
    }
    ic = &scn->sc_ic;
    psoc = scn->soc->psoc_obj;

    cohosted_bss_enable_bitmap = cfg_get(psoc, CFG_OL_COHOSTED_BSS_IND_ENABLE);
    pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    if (cohosted_bss_enable_bitmap & (1 << pdev_id)) {
        if (wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                    WLAN_PDEV_F_MBSS_IE_ENABLE)) {
            QDF_TRACE(QDF_MODULE_ID_COHOSTED_BSS, QDF_TRACE_LEVEL_INFO, "MBSS_IE enabled for pdev[%d], disable COHOSTED BSSID IND",
                      pdev_id);
            ic->ic_cohosted_bss_support = false;
            return;
        }
        ic->ic_cohosted_bss_support = true;
    } else {
        ic->ic_cohosted_bss_support = false;
        QDF_TRACE(QDF_MODULE_ID_COHOSTED_BSS, QDF_TRACE_LEVEL_INFO, "COHOSTED BSSID IND disabled for pdev[%d]",
                  pdev_id);
        return;
    }
    max_vdevs = wlan_pdev_get_max_vdev_count(ic->ic_pdev_obj) -
                wlan_pdev_get_max_monitor_vdev_count(ic->ic_pdev_obj);

   /* Promote max_vdevs to next power of 2 if not
    * already so.
    */
    if (!QDF_IS_PWR2(max_vdevs))
        max_vdevs = roundup_pow_of_two(max_vdevs);

    ic->max_cohosted_bssid_ind = ilog2(max_vdevs);
    QDF_TRACE(QDF_MODULE_ID_COHOSTED_BSS, QDF_TRACE_LEVEL_INFO, "COHOSTED BSSID IND enabled for pdev:%d, max_cohosted_bssid_ind:%d"
              " max_vdevs:%d", pdev_id, ic->max_cohosted_bssid_ind, max_vdevs);
}

/*
 * ol_ath_parse_wideband_support:
 * Set the support for wideband (5GHz-7GHz) support and CSA-specific flags
 *
 * Arguments:
 * ic              : Pointer to the ic structure
 * cfg_wideband_val: CFG value
 */
static void ol_ath_parse_wideband_support(struct ieee80211com *ic,
                                          const uint32_t cfg_wideband_csa_val)
{
    bool is_5ghz_capable = false, is_6ghz_capable = false;

    if (!ic) {
        qdf_err("Invalid ic pointer");
        return;
    }

    is_5ghz_capable = wlan_reg_is_band_present(ic->ic_pdev_obj,
                                               REG_BAND_5G);
    is_6ghz_capable = wlan_reg_is_band_present(ic->ic_pdev_obj,
                                               REG_BAND_6G);

    ic->ic_wideband_capable = false;
    if (is_5ghz_capable && is_6ghz_capable) {
        ic->ic_wideband_capable = true;
    } else {
        qdf_debug("Radio does not support wideband. Disabling");
        ic->ic_wideband_csa_support = WIDEBAND_CSA_DISABLED;
        return;
    }

    /*
     * Support is defined as follows:
     * 0: Disable wideband CSA
     * 1: Enable  wideband CSA (compatibility mode)
     * 2: Enable  wideband CSA (forced mode)
     */

    switch(cfg_wideband_csa_val) {
        case WIDEBAND_CSA_DISABLED:
        case WIDEBAND_CSA_COMPATIBILITY:
        case WIDEBAND_CSA_FORCED:
            ic->ic_wideband_csa_support = cfg_wideband_csa_val;
        break;

        default:
            qdf_err("Invalid cfg_wideband_val. Keeping wideband CSA disabled");
            ic->ic_wideband_csa_support = WIDEBAND_CSA_DISABLED;
        break;
    }

    return;
}

bool
ol_ath_get_host_hybrid_mode(ol_ath_soc_softc_t *soc)
{
    return wlan_psoc_nif_fw_ext_cap_get(soc->psoc_obj,
                                   WLAN_SOC_CEXT_HYBRID_MODE);
}

/* host_interest_item_address required by only wifi2.0, but its been invoked
 * from shared code. Since this function using definitions from targaddr.h
 * firmware header which is same across 2.0 & 3.0 hence it is maintained here.
 */
__inline__
u_int32_t host_interest_item_address(u_int32_t target_type, u_int32_t item_offset)
{
    switch (target_type)
    {
        default:
            ASSERT(0);
        case TARGET_TYPE_AR6002:
            return (AR6002_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6003:
            return (AR6003_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6004:
            return (AR6004_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6006:
            return (AR6006_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR9888:
            return (AR9888_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR6320:
            return (AR6320_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_AR900B:
            return (AR900B_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_QCA9984:
            return (QCA9984_HOST_INTEREST_ADDRESS + item_offset);
        case TARGET_TYPE_QCA9888:
            return (QCA9888_HOST_INTEREST_ADDRESS + item_offset);
    }
}
qdf_export_symbol(host_interest_item_address);

#if UMAC_SUPPORT_ACFG
extern void acfg_offchan_cancel(struct ieee80211com *ic);
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void wlan_recovery_vdev_delete_peers(struct wlan_objmgr_psoc *psoc,
                                     void *obj, void *args)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)obj;
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t vdev_id;

    if (!vdev)
        return;

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    vdev_id = wlan_vdev_get_id(vdev);

    cdp_vdev_recovery_flush_peers(soc_txrx_handle, vdev_id, false);
}

qdf_export_symbol(wlan_recovery_vdev_delete_peers);

void wlan_recovery_vdev_flush_mlo_peers(struct wlan_objmgr_psoc *psoc,
                                        void *obj, void *args)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)obj;
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t vdev_id;

    if (!vdev)
        return;

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    vdev_id = wlan_vdev_get_id(vdev);

    cdp_vdev_recovery_flush_peers(soc_txrx_handle, vdev_id, true);
}

qdf_export_symbol(wlan_recovery_vdev_flush_mlo_peers);

void wlan_recovery_peer_release_handler(struct wlan_objmgr_psoc *psoc,
                                        void *obj, void *args)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
    struct ieee80211vap *vap = NULL;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);

    vap = wlan_vdev_get_mlme_ext_obj(vdev);

    if (!vap) {
        return;
    }

    if (qdf_atomic_read(&peer->peer_objmgr.ref_cnt))
         ol_ath_find_logical_del_peer_and_release_ref(vap, peer->macaddr);
}

qdf_export_symbol(wlan_recovery_peer_release_handler);
#endif

void ol_recovery_check_beacon_stop(struct ieee80211vap *vap)
{
   struct ol_ath_vap_net80211 *avn;

   if (vap != NULL) {
       avn = OL_ATH_VAP_NET80211(vap);

       if (!avn->av_wbuf)
           return;

       ol_ath_beacon_stop(avn);
       ol_ath_beacon_free(vap);
   }
}

void wlan_pdev_operation(struct wlan_objmgr_psoc *psoc,
                              void *obj, void *args)
{
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_pdev *pdev;
    ol_txrx_soc_handle soc_txrx_handle;
    struct pdev_op_args *arg = (struct pdev_op_args *)args;
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211com *ic;
    ol_ath_soc_softc_t *soc;
    uint8_t i, pdev_id;
    int waitcnt;
    void *dp_pdev_ext_hdl;
#if ATH_SUPPORT_LED_CONTROLLER
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#ifndef CONFIG_X86
    struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
    uint8_t pdev_idx;
#endif
#endif
#endif
    qdf_event_t wait_event;

    pdev = (struct wlan_objmgr_pdev *)obj;
    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    scn = (struct ol_ath_softc_net80211*)lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
        qdf_err("scn is NULL");
        return;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is NULL");
        return;
    }

    ic =  &scn->sc_ic;
    soc = scn->soc;

    switch (arg->type) {
       case PDEV_ITER_POWERUP:
       {
#if UMAC_SUPPORT_WEXT
           ol_ath_iw_attach(scn->netdev);
#endif
#if UMAC_SUPPORT_ACFG || UMAC_SUPPORT_ACFG_RECOVERY
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_REINIT_DONE);
#if QCMAP_SUPPORT
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_QCMAP_REINIT_DONE);
#endif
#endif
           ic->recovery_in_progress = 0;
#ifdef WLAN_FEATURE_11BE_MLO
           /* Bring up MLO bridge VAPs from driver */
           wlan_recover_bridge_vaps_init(ic);
#endif
           break;
       }
       case PDEV_ITER_FATAL_SHUTDOWN:
       {
           ic->recovery_in_progress = 1;
           /* stop WMI so that no more WMI commands are sent
            * to target
            */
           if (pdev_wmi_handle)
               wmi_stop((wmi_unified_t)pdev_wmi_handle);
           break;
       }
       case PDEV_ITER_TARGET_ASSERT:
       {
           ic->recovery_in_progress = 1;
#if UMAC_SUPPORT_WEXT
           ol_ath_iw_detach(scn->netdev);
#endif
#if (UMAC_SUPPORT_ACFG && ACFG_NETLINK_TX)
           acfg_offchan_cancel(ic);
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_TARGET_ASSERT);
#if QCMAP_SUPPORT
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_QCMAP_TARGET_ASSERT);
#endif
#endif
#ifdef QCA_SUPPORT_CP_STATS
           pdev_cp_stats_tgt_asserts_inc(ic->ic_pdev_obj, 1);
#endif
           break;
       }
       case PDEV_ITER_PCIE_ASSERT:
       {
#if UMAC_SUPPORT_ACFG
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_PCIE_ASSERT);
#endif
           break;
       }

       case PDEV_ITER_PDEV_ENTRY_ADD:
       {
           /* Max radios reached with earlier pdev attach */
           if (arg->ret_val == PDEV_ITER_STATUS_FAIL)
               break;

           i = g_winfo.num_radios;
           if (g_winfo.wifi_radios[i].sc == NULL) {
               g_winfo.wifi_radios[i].sc = (void *)scn;
               g_winfo.wifi_radios[i].wifi_radio_type = OFFLOAD;
               scn->wifi_num = i;
               arg->ret_val = PDEV_ITER_STATUS_OK;
               g_winfo.num_radios++;
               qdf_debug("num_radios=%d, wifi_radios[%d].sc = %pK"
                       "wifi_radio_type = %d", g_winfo.num_radios,
                       i, g_winfo.wifi_radios[i].sc,
                       g_winfo.wifi_radios[i].wifi_radio_type);
               if (g_winfo.num_radios > NUM_MAX_RADIOS) {
                  qdf_info("Need to increase the NUM_MAX_RADIOS");
                  arg->ret_val = PDEV_ITER_STATUS_FAIL;
               }

           }

           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_QCMAP_WIFI_UP);
#if (UMAC_SUPPORT_ACFG && QCMAP_SUPPORT)
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_QCMAP_WIFI_UP);
#endif
           break;
       }
       case PDEV_ITER_PDEV_ENTRY_DEL:
       {
           for(i=0;i < NUM_MAX_RADIOS;i++) {
                if (g_winfo.wifi_radios[i].sc == scn) {
                    g_winfo.wifi_radios[i].sc = NULL;
                    g_winfo.num_radios--;
                }
           }
#if (UMAC_SUPPORT_ACFG && QCMAP_SUPPORT)
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_QCMAP_WIFI_DOWN);
#endif
           break;
       }
       case PDEV_ITER_RECOVERY_AHB_REMOVE:
       {
           /* stop WMI so that no more WMI commands are sent
            * to target
            */
           wmi_stop((wmi_unified_t)pdev_wmi_handle);
           /* Please do not add break, both cases should be executed */
       }
       case PDEV_ITER_RECOVERY_REMOVE:
       {
           if (osif_recover_profile_alloc(ic) != 0) {
               QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
                       QDF_TRACE_LEVEL_INFO, "[pdev id %d]ERROR: FW Recovery"
                       "profile alloc failed\n",
                       pdev_id);
               return;
           }
           break;
       }
       case PDEV_ITER_RECOVERY_WAIT:
       {
            waitcnt = 0;
            qdf_mem_zero(&wait_event, sizeof(wait_event));

            qdf_event_create(&wait_event);
            qdf_event_reset(&wait_event);

            while(wlan_pdev_get_vdev_count(pdev) &&
                          waitcnt <= OSIF_MAX_DELETE_VAP_TIMEOUT) {
               qdf_wait_single_event(&wait_event, 1000);
               waitcnt++;
            }
            qdf_event_destroy(&wait_event);

            if(waitcnt > OSIF_MAX_DELETE_VAP_TIMEOUT) {
                wlan_objmgr_iterate_obj_list_all(psoc, WLAN_VDEV_OP,
                         ath_vdev_dump, NULL, 1,
                         WLAN_MLME_NB_ID);
                if(ol_target_lithium(soc->psoc_obj)) {
                    /* Signal delayed BUG_ON to get q6 dump before it */
                    soc->delay_bug_on = true;
                }
            }
            break;
       }
       case PDEV_ITER_RECOVERY_STOP:
       {
           dev_close(scn->netdev);
           ic_reset_params(ic);
           break;
       }
       case PDEV_ITER_RECOVERY_PROBE:
       {
           osif_recover_vap_create(ic);
           break;
       }
#if OL_ATH_SUPPORT_LED
       case PDEV_ITER_LED_GPIO_STATUS:
       {
           struct ol_ath_softc_net80211 *scn_temp;

           scn_temp = (struct ol_ath_softc_net80211 *)arg->pointer;
           if ((scn != scn_temp) && (scn->scn_led_gpio == scn_temp->scn_led_gpio))
                arg->ret_val = PDEV_ITER_STATUS_OK;
           break;
       }
#endif
       case PDEV_ITER_PDEV_NETDEV_STOP:
       {
          ath_netdev_stop(scn->netdev);
          break;
       }
       case PDEV_ITER_PDEV_NETDEV_OPEN:
       {
          ath_netdev_open(scn->netdev);
          break;
       }
#if UMAC_SUPPORT_ACFG
       case PDEV_ITER_TARGET_FWDUMP:
       {
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_FWDUMP_READY);
           break;
       }
#endif
       case PDEV_ITER_SEND_SUSPEND:
       {
           struct suspend_params param;

           qdf_mem_set(&param, sizeof(param), 0);
           param.disable_target_intr = *((int *)arg->pointer);
           qdf_info("disable_target_intr val is %d",
                    param.disable_target_intr);

           wmi_unified_suspend_send(pdev_wmi_handle, &param,
                                    lmac_get_pdev_idx(pdev));
           break;
       }
       case PDEV_ITER_SEND_RESUME:
       {
           arg->ret_val = wmi_unified_resume_send(pdev_wmi_handle,
                                         lmac_get_pdev_idx(pdev));
           break;
       }
       case PDEV_ITER_PDEV_DEINIT_BEFORE_SUSPEND:
       {
           bool is_ema_ap_enabled = wlan_pdev_nif_feat_ext_cap_get(pdev,
                                        WLAN_PDEV_FEXT_EMA_AP_ENABLE);

           soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
#if OL_ATH_SUPPORT_LED
           qdf_timer_sync_cancel(&scn->scn_led_blink_timer);
           qdf_timer_sync_cancel(&scn->scn_led_poll_timer);

           if (scn->scn_led_gpio) {
               if (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA8074V2)
                   qal_vbus_gpio_set_value_cansleep(scn->scn_led_gpio, OL_LED_OFF);
               else if (lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA9574 ||
                        lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA5018 ||
                        lmac_get_tgt_type(psoc) == TARGET_TYPE_QCA5332) {
                       ol_ath_set_led_on_off(scn, OL_LED_OFF);
                   }
           } else {
#if ATH_SUPPORT_LED_CONTROLLER
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#ifndef CONFIG_X86
               reg_cap = ucfg_reg_get_hal_reg_cap(psoc);
               pdev_idx = lmac_get_pdev_idx(scn->sc_pdev);
               if (reg_cap) {
                   if(reg_cap[pdev_idx].wireless_modes & WIRELESS_MODES_2G) {
                        ipq_led_set_blink(HAWKEYE_2G_LED, 0, 0);
                   } else {
                        ipq_led_set_blink(HAWKEYE_5G_LED, 0, 0);
                   }
               }
#endif
#endif
#endif
           }

#endif
           if (!is_emulation(psoc) && ol_target_lithium(psoc)) {
               qdf_timer_sync_cancel(&scn->scn_stats_timer);
           }
           qdf_timer_sync_cancel(&scn->auth_timer);
#ifdef QCA_SUPPORT_CP_STATS
           if (pdev_cp_stats_ap_stats_tx_cal_enable_get(pdev)) {
               scn->sc_ic.ic_ath_enable_ap_stats(ic, 0);
               pdev_cp_stats_ap_stats_tx_cal_enable_update(pdev, 1);
           }
#endif
           /* mcopy and tx capture are mutually exclusive mode */
           if (ol_ath_is_mcopy_enabled(ic)) {
               uint8_t sniffer_mode = ic->ic_debug_sniffer;

               ol_ath_set_debug_sniffer(scn, SNIFFER_DISABLE);
               if (ic->ic_monitor_version != MONITOR_VERSION_2) {
                   ol_ath_pdev_set_param(scn->sc_pdev,
                                         wmi_pdev_param_set_promisc_mode_cmdid,
                                         0);
               }
               /*To enable M_COPY after recovery if it was enabled before recovery*/
               if (scn->soc->recovery_in_progress)
                   ic->ic_debug_sniffer = sniffer_mode;
               } else if (ic->ic_tx_capture) {
                   uint8_t tx_capture = ic->ic_tx_capture;

                   ol_ath_set_debug_sniffer(scn, SNIFFER_DISABLE);
                  /*To enable tx capture after recovery if it was enabled before recovery*/
                   if (scn->soc->recovery_in_progress)
                       ic->ic_tx_capture = tx_capture;
               }

           if (ic->ic_capture_latency) {
               uint8_t tx_lat_capture = ic->ic_capture_latency;

               ol_ath_set_capture_latency(scn, 0);
               /*To enable tx latency capture after recovery if it was enabled before recovery */
               if (scn->soc->recovery_in_progress)
                   ic->ic_capture_latency = tx_lat_capture;
           }

	   if (scn->soc->recovery_in_progress) {
		   qdf_err("############# RESETING MONITOR MODE #############");
		   cdp_reset_monitor_mode(soc_txrx_handle,
				          wlan_objmgr_pdev_get_pdev_id(pdev),
					  0);
	   }

#ifdef QCA_SUPPORT_LITE_MONITOR
           if (ic->ic_monitor_version == MONITOR_VERSION_2 &&
               scn->soc->recovery_in_progress) {
               if (cdp_is_lite_mon_enabled(soc_txrx_handle,
                                           pdev_id,
                                           LITE_MON_DIRECTION_RX)) {
                   ic->rx_config = qdf_mem_malloc(sizeof(struct lite_mon_config));
                   if (ic->rx_config) {
                       ic->rx_config->direction = LITE_MON_DIRECTION_RX;
                       monitor_get_lite_monitor_config(scn, ic->rx_config);
                   }
               }
               if (cdp_is_lite_mon_enabled(soc_txrx_handle,
                                           pdev_id,
                                           LITE_MON_DIRECTION_TX)) {
                   ic->tx_config = qdf_mem_malloc(sizeof(struct lite_mon_config));
                   if (ic->tx_config) {
                       ic->tx_config->direction = LITE_MON_DIRECTION_TX;
                       monitor_get_lite_monitor_config(scn, ic->tx_config);
                       }
                   }
           }
#endif

#if ATH_DATA_TX_INFO_EN
           ol_ath_stats_detach(ic);
#endif
           ol_ath_thermal_mitigation_detach(scn, scn->netdev);

           /* reset current PP in case of ema ap mode */
           if (is_ema_ap_enabled && ic->ic_mbss_ctx) {
               uint8_t gp_id;

               for (gp_id = 0; gp_id < ic->ic_mbss_ctx->max_ngroups; gp_id++)
                   ic->ic_mbss_ctx->ic_mbss[gp_id].current_pp = 1;
           }

           /* Stop wmi sequence check */
           wmi_interface_sequence_stop(pdev_wmi_handle);

           break;
       }
       case PDEV_ITER_PDEV_DETACH_OP:
       {
           soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

           qdf_info("soc_txrx_handle %pK dp_pdev id %d", soc_txrx_handle, pdev_id);
           cdp_pdev_detach(soc_txrx_handle, pdev_id, 0);

           break;
       }
       case PDEV_ITER_PDEV_DEINIT_OP:
       {
           soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
           dp_pdev_ext_hdl = cdp_pdev_get_dp_txrx_handle(soc_txrx_handle,
                                                         pdev_id);
           if (ol_target_be(psoc)) {
               ol_ath_unsubscribe_peer_wdi_event(scn);
#ifdef CONFIG_SAWF
               if (cfg_get(psoc, CFG_OL_SAWF_UL_FSE_ENABLE)) {
                   ol_ath_unsubscribe_sawf_fse_wdi_event(scn);
               }
#ifdef SAWF_ADMISSION_CONTROL
               if (cfg_get(psoc, CFG_OL_SAWF_ADM_CTRL_ENABLE)) {
                   ol_ath_unsubscribe_sawf_peer_msduq_event(scn);
               }
#endif
#endif
           }

#ifdef IPA_OFFLOAD
           ucfg_ipa_uc_ol_deinit(pdev);
#endif
#if ATH_SUPPORT_WRAP
           if (dp_pdev_ext_hdl) {
               dp_wrap_detach(pdev);
           }
#endif
           if (dp_pdev_ext_hdl) {
               dp_extap_detach(pdev);
           }
           cdp_pdev_set_dp_txrx_handle(soc_txrx_handle, pdev_id, NULL);

           if (dp_pdev_ext_hdl)
               qdf_mem_free(dp_pdev_ext_hdl);

           cdp_pdev_deinit(soc_txrx_handle, pdev_id, 0);

           wmi_unified_remove_work((wmi_unified_t)pdev_wmi_handle);

           qdf_flush_work(&scn->sc_ic.dfs_cac_timer_start_work);

           qdf_flush_work(&scn->sc_ic.ic_cfg80211_update_channel_list_start_work);

           qdf_flush_work(&scn->sc_ic.assoc_sm_set_country_code);

           if (!is_emulation(scn->soc->psoc_obj) &&
               ol_target_lithium(scn->soc->psoc_obj)) {
               if (scn->is_scn_stats_timer_init)
                   qdf_timer_sync_cancel(&scn->scn_stats_timer);
           }


#if WLAN_SPECTRAL_ENABLE
           spectral_unregister_dbr(scn->sc_pdev);
#endif /* WLAN_SPECTRAL_ENABLE */

#if QCA_STANDALONE_SOUNDING_TRIGGER
           ol_ath_pdev_host_trig_txbf_sounding_deinit(scn->sc_pdev);
#endif /* QCA_STANDALONE_SOUNDING_TRIGGER */

#if WLAN_CFR_ENABLE
    /* pdev deinit to be done on Target Stop */
           cfr_deinitialize_pdev(scn->sc_pdev);
#endif
#if WLAN_WIFI_RADAR_ENABLE
           wifi_radar_deinitialize_pdev(scn->sc_pdev);
#endif

           ol_ath_mgmt_detach(ic);

           if (ic->recovery_in_progress)
                   *ic->recovery_dfschan = *ic->ic_curchan;
           else
                   *ic->recovery_dfschan = empty_channel;

           break;
       }
       case PDEV_ITER_PDEV_RESET_PARAMS:
       {
           ol_reset_params(scn);
           break;
       }
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
       case PDEV_ITER_ML_REINIT_DONE:
       {
#if UMAC_SUPPORT_ACFG || UMAC_SUPPORT_ACFG_RECOVERY
           OSIF_RADIO_DELIVER_EVENT_WATCHDOG(ic, ACFG_WDT_ML_REINIT_DONE);
#endif
           break;
       }
#endif
       default:
       break;
     }
}
qdf_export_symbol(wlan_pdev_operation);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
void wlan_mlo_read_tsf2_offset(struct wlan_objmgr_psoc *psoc,
			       uint8_t pdev_id,
			       uint8_t lmac_id)
{
	ol_txrx_soc_handle soc_txrx_handle;
	uint64_t value = 0;

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_err("DP soc handle is NULL");
		return;
	}

	cdp_get_tsf2_offset(soc_txrx_handle, lmac_id, &value);
	cdp_mlo_update_delta_tsf2(soc_txrx_handle, pdev_id, value);
}

void wlan_mlo_read_tqm_offset(struct wlan_objmgr_psoc *psoc)
{
	ol_txrx_soc_handle soc_txrx_handle;
	uint64_t value = 0;

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_err("DP soc handle is NULL");
		return;
	}

	cdp_get_tqm_offset(soc_txrx_handle, &value);
	cdp_mlo_update_delta_tqm(soc_txrx_handle, value);
}
#endif

/* wow_nack and reason_code are params used by MCL's callback */
void
ol_target_send_suspend_complete(void *ctx, bool wow_nack, uint16_t reason_code)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *)ctx;

    soc->is_target_paused = TRUE;
    __ol_target_paused_event(soc);
}

/*
 * Routin to lock vap transmit
 *
 */
void ol_vap_tx_lock( void *vosdev)
{
#if !QCA_OL_TX_PDEV_LOCK && QCA_NSS_PLATFORM || (defined QCA_PARTNER_PLATFORM && QCA_PARTNER_SUPPORT_FAST_TX)
    osif_dev  *osdev = (osif_dev  *)vosdev;
    VAP_TX_SPIN_LOCK(&osdev->tx_lock);
#endif
    return;
}

/*
 * Routin to unlock vap transmit
 *
 */
void ol_vap_tx_unlock( void *vosdev)
{
#if !QCA_OL_TX_PDEV_LOCK && QCA_NSS_PLATFORM || (defined QCA_PARTNER_PLATFORM && QCA_PARTNER_SUPPORT_FAST_TX)
    osif_dev  *osdev = (osif_dev  *)vosdev;
    VAP_TX_SPIN_UNLOCK(&osdev->tx_lock);
#endif
    return;
}

A_STATUS HIF_USB_connect_service(ol_ath_soc_softc_t *soc)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    struct htc_service_connect_req connect;
    struct htc_service_connect_resp response;
    HTC_HANDLE htc_handle;

    htc_handle = lmac_get_htc_hdl(soc->psoc_obj);
    if (!htc_handle)
        return -1;

    A_MEMZERO(&connect,sizeof(connect));

    connect.EpCallbacks.EpSendFull        = NULL;
    connect.EpCallbacks.EpRecv            = NULL;
    connect.LocalConnectionFlags |= HTC_LOCAL_CONN_FLAGS_ENABLE_SEND_BUNDLE_PADDING;
    connect.MaxSendMsgSize = 1664;
    connect.service_id = WMI_DATA_BE_SVC;
    if ((status = htc_connect_service(htc_handle, &connect, &response)) !=
        QDF_STATUS_SUCCESS) {
        qdf_err("Failed to connect to Endpoint Ping BE svc, status:%d", status);
        return -1;
    } else {
        qdf_info("eppingtest BE endpoint:%d", response.Endpoint);
    }
    connect.service_id= WMI_DATA_BK_SVC;
    if ((status = htc_connect_service(htc_handle, &connect, &response)) !=
        EOK) {
        qdf_err("Failed to connect to Endpoint Ping BK svc, status:%d", status);
        return -1;
    } else {
        qdf_info("eppingtest BK endpoint:%d", response.Endpoint);
    }
    connect.service_id = WMI_DATA_VI_SVC;
    if ((status = htc_connect_service(htc_handle, &connect, &response))
            != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to connect to Endpoint Ping VI svc, status:%d", status);
        return -1;
    } else {
        qdf_info("eppingtest VI endpoint:%d", response.Endpoint);
    }
    connect.service_id = WMI_DATA_VO_SVC;
    if ((status = htc_connect_service(htc_handle, &connect, &response))
            != QDF_STATUS_SUCCESS) {
        qdf_err("Failed to connect to Endpoint Ping VO svc, status:%d", status);
        return -1;
    } else {
        qdf_info("eppingtest VO endpoint:%d", response.Endpoint);
    }
    return EOK;
}

static void ol_ath_enable_pdev_map(ol_ath_soc_softc_t *soc)
{
    /* cmd_map is used to translate pdev id from host pdev id
     * to target pdev id.
     */
    uint32_t phyb_2g_pdev_cmd_map[WMI_HOST_MAX_PDEV] = {WMI_HOST_PDEV_ID_2,
                                       WMI_HOST_PDEV_ID_1, WMI_HOST_PDEV_ID_1};
    /* evt_map is used to translate pdev id from target pdev id
     * to host pdev id. Keep both 0th and 1st index as 0 since phyb mode only
     * has one pdev. So even if target sends 1 or 2 as tgt_pdev_id, it will
     * get translated to 0 host pdev id.
     */
    uint32_t phyb_2g_pdev_evt_map[WMI_HOST_MAX_PDEV] = {WMI_HOST_PDEV_ID_0,
                                       WMI_HOST_PDEV_ID_0, WMI_HOST_PDEV_ID_2};

    struct target_psoc_info *tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    uint32_t target_type = target_psoc_get_target_type(tgt_psoc_info);
    struct wmi_unified *wmi_handle = target_psoc_get_wmi_hdl(tgt_psoc_info);
    bool is_2g_phyb_enabled = cfg_get(soc->psoc_obj, CFG_OL_MODE_2G_PHYB);


    if (is_2g_phyb_enabled && target_type == TARGET_TYPE_QCA8074V2) {
        qdf_mem_copy(wmi_handle->cmd_pdev_id_map, phyb_2g_pdev_cmd_map,
            WMI_HOST_MAX_PDEV * sizeof(uint32_t));
        qdf_mem_copy(wmi_handle->evt_pdev_id_map, phyb_2g_pdev_evt_map,
            WMI_HOST_MAX_PDEV * sizeof(uint32_t));
        wmi_handle->soc->is_pdev_is_map_enable = true;
    }
}

static void wmi_change_htc_endpoint(ol_ath_soc_softc_t *soc)
{
    uint32_t target_type;
    struct wmi_unified *wmi_handle;
    struct target_psoc_info *tgt_psoc_info;
    uint32_t phyb_2g_svc_id_mapping[] = {WMI_CONTROL_SVC_WMAC1,
                                         WMI_CONTROL_SVC,
                                         WMI_CONTROL_SVC_WMAC2};
    bool is_2g_phyb_enabled;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    target_type = target_psoc_get_target_type(tgt_psoc_info);
    wmi_handle = target_psoc_get_wmi_hdl(tgt_psoc_info);

    is_2g_phyb_enabled = cfg_get(soc->psoc_obj, CFG_OL_MODE_2G_PHYB);

    if (is_2g_phyb_enabled && target_type == TARGET_TYPE_QCA8074V2)
            wmi_handle->soc->svc_ids = &phyb_2g_svc_id_mapping[0];
}

static bool ol_ath_tgt_supports_diag_and_dbr_over_seperate_ce(ol_ath_soc_softc_t *soc)
{
    if (ol_target_be(soc->psoc_obj))
        return true;

    return false;
}

int ol_ath_connect_htc(ol_ath_soc_softc_t *soc)
{
    int status = QDF_STATUS_E_INVAL;
    struct htc_service_connect_req connect;
    struct target_psoc_info *tgt_psoc_info;
    struct wmi_unified *wmi_handle;
    HTC_HANDLE htc_handle;
    struct hif_opaque_softc *hif_handle;
    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        goto conn_fail;
    }

    wmi_handle = target_psoc_get_wmi_hdl(tgt_psoc_info);
    htc_handle = target_psoc_get_htc_hdl(tgt_psoc_info);
    hif_handle = target_psoc_get_hif_hdl(tgt_psoc_info);

    OS_MEMZERO(&connect,sizeof(connect));

    /* meta data is unused for now */
    connect.pMetaData = NULL;
    connect.MetaDataLength = 0;
    /* these fields are the same for all service endpoints */
    connect.EpCallbacks.pContext = soc;
    connect.EpCallbacks.EpTxCompleteMultiple = NULL /* Control path completion ar6000_tx_complete */;
    connect.EpCallbacks.EpRecv = NULL /* Control path rx */;
    connect.EpCallbacks.EpRecvRefill = NULL /* ar6000_rx_refill */;
    connect.EpCallbacks.EpSendFull = NULL /* ar6000_tx_queue_full */;

    wmi_change_htc_endpoint(soc);
    if (!bypasswmi) {
        if ((status = wmi_unified_connect_htc_service((wmi_unified_t)wmi_handle,
                            htc_handle)) != QDF_STATUS_SUCCESS)
             goto conn_fail;
#ifdef WLAN_DIAG_AND_DBR_OVER_SEPARATE_CE
        if (ol_ath_tgt_supports_diag_and_dbr_over_seperate_ce(soc)) {
            if ((status = wmi_diag_connect_pdev_htc_service((wmi_unified_t)wmi_handle,
                            htc_handle)) != QDF_STATUS_SUCCESS)
                qdf_warn("Connect failed for Diag CE");

            if ((status = wmi_dbr_connect_pdev_htc_service((wmi_unified_t)wmi_handle,
                            htc_handle)) != QDF_STATUS_SUCCESS)
                qdf_warn("Connect failed for DBR CE");
        }
#endif
    }
#if defined(EPPING_TEST) && !defined(HIF_USB)
    if (eppingtest){
        extern A_STATUS epping_connect_service(ol_ath_soc_softc_t *soc);
        if ((status = epping_connect_service(soc)) != EOK)
             goto conn_fail;
    }
#endif
    if (target_psoc_get_target_ver(tgt_psoc_info) == AR6004_VERSION_REV1_3) {
      if ((status = HIF_USB_connect_service(soc)) != EOK)
               goto conn_fail;
    }

    /*
     * Since cookies are used for HTC transports, they should be
     * initialized prior to enabling HTC.
     */
    ol_cookie_init((void *)soc);

    /*
     * Start HTC
     */
    if ((status = htc_start(htc_handle)) != QDF_STATUS_SUCCESS) {
        goto conn_fail;
    }

    if (!bypasswmi) {
        /*
         * Wait for WMI event to be ready
         */
        if (target_psoc_get_target_ver(tgt_psoc_info) == AR6004_VERSION_REV1_3) {
            target_psoc_set_wmi_ready(tgt_psoc_info, TRUE);
            target_psoc_set_wlan_init_status(tgt_psoc_info,
                             TARGET_INIT_STATUS_SUCCESS);
        } else {
            if ((status = __ol_ath_check_wmi_ready(soc)) != EOK) {
                goto conn_fail1;
            }

            if (target_psoc_get_wlan_init_status(tgt_psoc_info) !=
                                        TARGET_INIT_STATUS_SUCCESS)
            {
              qdf_err("Target wmi init failed with status %d",
                      target_psoc_get_wlan_init_status(tgt_psoc_info));
              status = ENODEV;
              goto conn_fail1;
            }
        }
    }
    return EOK;

conn_fail1:
    hif_disable_isr((struct hif_opaque_softc *)hif_handle);
    htc_stop(htc_handle);
conn_fail:
    return status;
}

int
ol_ath_disconnect_htc(ol_ath_soc_softc_t *soc)
{
    HTC_HANDLE htc_handle;

    htc_handle = lmac_get_htc_hdl(soc->psoc_obj);
    if (htc_handle)
        htc_stop(htc_handle);
    return 0;
}

#ifdef CE_TASKLET_DEBUG_ENABLE
void ol_ath_enable_ce_latency_stats(struct ol_ath_soc_softc *soc, uint8_t val)
{
    struct target_psoc_info *tgt_psoc_info;
    struct hif_opaque_softc *hif_handle;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return;
    }

    hif_handle = target_psoc_get_hif_hdl(tgt_psoc_info);
    if (!hif_handle) {
        qdf_err("hif_handle is null");
        return;
    }

    hif_enable_ce_latency_stats(hif_handle, val);
}
#endif

static void
ol_ath_update_band_mapping(
                    struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap,
                    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    reg_cap->low_2ghz_chan  =  mac_phy_cap->reg_cap_ext.low_2ghz_chan;
    reg_cap->high_2ghz_chan =  mac_phy_cap->reg_cap_ext.high_2ghz_chan;
    reg_cap->low_5ghz_chan  =  mac_phy_cap->reg_cap_ext.low_5ghz_chan;
    reg_cap->high_5ghz_chan =  mac_phy_cap->reg_cap_ext.high_5ghz_chan;
}

static void
ol_ath_update_wireless_modes(struct ieee80211com *ic,
                             struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap,
                             struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    uint64_t wireless_2G_modes = WIRELESS_MODES_2G;
    uint64_t wireless_5G_modes = WIRELESS_MODES_5G;
    uint64_t wireless_modes_before_update;

    qdf_debug("wireless_modes = %llx before update",
                    reg_cap->wireless_modes);
    wireless_modes_before_update = reg_cap->wireless_modes;
    if (!(mac_phy_cap->supported_bands & WMI_HOST_WLAN_5G_CAPABILITY)) {
        /* mac_phy cap indicates no support for 5G. Make sure 5G specific
         * modes are not present in wireless mode
         */
        reg_cap->wireless_modes &= ~(wireless_5G_modes);
    }
    if (!(mac_phy_cap->supported_bands & WMI_HOST_WLAN_2G_CAPABILITY)) {
        /* mac_phy cap indicates no support for 2G. Make sure 2G specific
         * modes are not present in wireless mode
         */
        reg_cap->wireless_modes &= ~(wireless_2G_modes);
    }

    qdf_debug("Wireless_modes = %llx after update",
                    reg_cap->wireless_modes);
    if (wireless_modes_before_update != reg_cap->wireless_modes) {
        ol_regdmn_update_pdev_wireless_modes(ic, reg_cap->wireless_modes);
        qdf_info("Wireless modes are updated in the regulatory pdev priv object");
    }
}

/* called in case of dynamic mode switch */
static void
ol_ath_update_wireless_modes_ext(
                      struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap,
                      struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    uint64_t wireless_2G_modes       = WIRELESS_MODES_2G;
    uint64_t wireless_5G_modes       = WIRELESS_MODES_5G;

    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
              QDF_TRACE_LEVEL_DEBUG,
              "wireless_modes = %llx before update",
              reg_cap->wireless_modes);

    reg_cap->wireless_modes = mac_phy_cap->reg_cap_ext.wireless_modes;

    if (!(mac_phy_cap->supported_bands & WMI_HOST_WLAN_5G_CAPABILITY)) {
        /* mac_phy cap indicates no support for 5G. Make sure 5G specific
         * modes are not present in wireless mode
         */
        reg_cap->wireless_modes &= ~(wireless_5G_modes);
    }
    if (!(mac_phy_cap->supported_bands & WMI_HOST_WLAN_2G_CAPABILITY)) {
        /* mac_phy cap indicates no support for 2G. Make sure 2G specific
         * modes are not present in wireless mode
         */
        reg_cap->wireless_modes &= ~(wireless_2G_modes);
    }

    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
              QDF_TRACE_LEVEL_DEBUG,
              "Wireless_modes = %llx after update", reg_cap->wireless_modes);
}

static void
ol_ath_update_target_cap_from_mac_phy_cap(struct wlan_psoc_target_capability_info *cap,
                                struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    /* If Radio supports both 2G and 5G, By default it will be brought up in 5G.
     * Update to @2G caps during band switch.
     */
    if (mac_phy_cap->supported_bands & WMI_HOST_WLAN_5G_CAPABILITY) {
        cap->ht_cap_info = mac_phy_cap->ht_cap_info_5G;
        cap->vht_cap_info = mac_phy_cap->vht_cap_info_5G;
        cap->vht_supp_mcs = mac_phy_cap->vht_supp_mcs_5G;
    } else if (mac_phy_cap->supported_bands & WMI_HOST_WLAN_2G_CAPABILITY) {
        cap->ht_cap_info = mac_phy_cap->ht_cap_info_2G;
        cap->vht_cap_info = mac_phy_cap->vht_cap_info_2G;
        cap->vht_supp_mcs = mac_phy_cap->vht_supp_mcs_2G;
    } else {
        qdf_info("No 2G OR 5G support set in ext service config");
    }
}

static inline void ol_ath_populate_chainmask(struct ieee80211com *ic,
        uint32_t tx_chain_mask, uint32_t rx_chain_mask)
{
    ieee80211com_set_num_rx_chain(ic, num_chain_from_chain_mask(rx_chain_mask));
    ieee80211com_set_num_tx_chain(ic, num_chain_from_chain_mask(tx_chain_mask));
    ieee80211com_set_tx_chainmask(ic, tx_chain_mask);
    ol_ath_update_chainmask_caps(ic, TX_CHAINMASK);
    ieee80211com_set_rx_chainmask(ic, rx_chain_mask);
    ol_ath_update_chainmask_caps(ic, RX_CHAINMASK);
    /*
     * Note that in the offload architecture chain_masks
     * and spatial_streams are synonymous
     */
    ieee80211com_set_spatialstreams(ic,
                       num_chain_from_chain_mask(rx_chain_mask));
    /* Update FW aDFS support based on the current chainmask */
    ol_ath_update_fw_adfs_support(ic, rx_chain_mask);
}

/**
 * ol_ath_update_chainmask() - updates tx/rx chainmask values
 * @ic: ic pointer
 * @cap: target capabilities
 * @mac_phy_cap: targets mac phy capabilities
 *
 * Return: none
 */
static inline void ol_ath_update_chainmask(struct ieee80211com *ic,
        struct wlan_psoc_target_capability_info *cap,
        struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    struct target_psoc_info *tgt_psoc_info;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wmi_unified *wmi_handle;
    uint32_t chain_mask;
    int mode;

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return;
    }

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return;
    }

    mode = target_psoc_get_preferred_hw_mode(tgt_psoc_info);

    if (!(wmi_service_enabled(wmi_handle, wmi_service_ext_msg))) {

        chain_mask = ((1 << cap->num_rf_chains) - 1);
        ol_ath_populate_chainmask(ic, chain_mask, chain_mask);
    } else {
        /* 5G is used by default if support for both 5G and 2G is present */
        if ((mac_phy_cap) &&
                (mac_phy_cap->supported_bands &
                 WMI_HOST_WLAN_5G_CAPABILITY)) {
            ol_ath_populate_chainmask(ic, mac_phy_cap->tx_chain_mask_5G,
                    mac_phy_cap->rx_chain_mask_5G);
            cap->num_rf_chains = ic->ic_num_rx_chain;
        } else if ((mac_phy_cap) &&
                (mac_phy_cap->supported_bands &
                 WMI_HOST_WLAN_2G_CAPABILITY)) {
            ol_ath_populate_chainmask(ic, mac_phy_cap->tx_chain_mask_2G,
                mac_phy_cap->rx_chain_mask_2G);
            cap->num_rf_chains = ic->ic_num_rx_chain;
        } else {
            qdf_info("No 2G OR 5G support set in ext service config");
        }
    }

#ifdef ATH_SUPPORT_WAPI

    if (target_psoc_get_target_type(tgt_psoc_info) == TARGET_TYPE_AR9888) {
    /*WAPI HW engine support upto 300 Mbps (MCS15h),
      limiting the chains to 2*/
      ic->ic_num_wapi_rx_maxchains = 2;
      ic->ic_num_wapi_tx_maxchains = 2;
    } else {
      ic->ic_num_wapi_rx_maxchains = ic->ic_num_rx_chain;
      ic->ic_num_wapi_tx_maxchains = ic->ic_num_tx_chain;
    }
#endif

}

/**
 * ol_ath_update_ht_caps() - updates iee80211 ht capabilities info
 * @ic: ic pointer
 * @ht_cap_info: targets ht capabilities information
 *
 * Return: none
 */
static void ol_ath_update_ht_caps(struct ieee80211com *ic, uint32_t ht_cap_info)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    uint16_t caps  = 0;
    u_int8_t mpdu_density;
    struct target_psoc_info *tgt_hdl;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap = NULL;

    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (tgt_hdl)
        mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);

    if (ht_cap_info & WMI_HOST_HT_CAP_ENABLED) {
        ieee80211com_set_cap(ic, IEEE80211_C_HT);
        ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SHORTGI40
                        | IEEE80211_HTCAP_C_CHWIDTH40
                        | IEEE80211_HTCAP_C_DSSSCCK40);
        if (ht_cap_info & WMI_HOST_HT_CAP_HT20_SGI)  {
            ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SHORTGI20);
        }
        if (ht_cap_info & WMI_HOST_HT_CAP_DYNAMIC_SMPS) {
            ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SMPOWERSAVE_DYNAMIC);
        } else {
            ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_SMPOWERSAVE_DISABLED);
        }

        if (ht_cap_info & WMI_HOST_HT_CAP_IBF_BFER) {
            ic->ic_implicitbf = 1 ;
        }

        ieee80211com_set_htextcap(ic, IEEE80211_HTCAP_EXTC_TRANS_TIME_5000
                        | IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_NONE);
        ieee80211com_set_maxampdu(ic, IEEE80211_HTCAP_MAXRXAMPDU_65536);

        /*
         * Target sends ampdu_density as part of "mac_phy_caps" in the
         * service_ready event.Host should use the min of the values
         * available from ini & mac_phy_caps.The value is picked as
         * per the below table:
         *  ini = 0 && mac_phy_caps = 0   ==> 0
         *  ini = 0 && mac_phy_caps > 0   ==> mac_phy_caps
         *  ini > 0 && mac_phy_caps = 0   ==> ini
         *  ini > 0 && mac_phy_caps > ini ==> ini
         *  ini > 0 && mac_phy_caps < ini ==> mac_phy_caps
         */
        mpdu_density = cfg_get(psoc, CFG_TGT_MPDU_DENSITY);
        if (mac_phy_cap && (mac_phy_cap->ampdu_density > 0)) {
            if ((mpdu_density == 0) ||
                ((mpdu_density > 0) && (mac_phy_cap->ampdu_density <
                                        mpdu_density)))
                mpdu_density = mac_phy_cap->ampdu_density;
        }
        ieee80211com_set_mpdudensity(ic, mpdu_density);
        ic->ic_mpdudensityoverride = 0;

        IEEE80211_ENABLE_AMPDU(ic);

    }

    /* Tx STBC is a 2-bit mask. Convert to ieee definition. */
    caps = (ht_cap_info & WMI_HOST_HT_CAP_TX_STBC) >> WMI_HOST_HT_CAP_TX_STBC_MASK_SHIFT;
    ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_TXSTBC & (caps << IEEE80211_HTCAP_C_TXSTBC_S));


    /* Rx STBC is a 2-bit mask. Convert to ieee definition. */
    caps = (ht_cap_info & WMI_HOST_HT_CAP_RX_STBC) >> WMI_HOST_HT_CAP_RX_STBC_MASK_SHIFT;
    ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_RXSTBC & (caps << IEEE80211_HTCAP_C_RXSTBC_S));

    if (ht_cap_info & WMI_HOST_HT_CAP_LDPC) {
        ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_ADVCODING);
        if ((lmac_get_tgt_type(psoc) == TARGET_TYPE_AR900B) &&
             (lmac_get_tgt_revision(psoc) == AR900B_REV_1)) {
            /* disable LDPC capability for Beeliner 1.0 */
            ieee80211com_set_ldpccap(ic,IEEE80211_HTCAP_C_LDPC_NONE);
        } else {
            /* enable LDPC capability */
            ieee80211com_set_ldpccap(ic, IEEE80211_HTCAP_C_LDPC_TXRX);
        }
    } else {
        /* check if new method of LDPC flags are provided
         * RX capability should be announced by AP or STA
         * TX capability is based on what peer can receive and
         * what AP can transmit
         */
        if (ht_cap_info & (WMI_HOST_HT_CAP_RX_LDPC | WMI_HOST_HT_CAP_TX_LDPC)) {
            if (ht_cap_info & WMI_HOST_HT_CAP_RX_LDPC) {
                ieee80211com_set_htcap(ic, IEEE80211_HTCAP_C_ADVCODING);
                ieee80211com_set_ldpccap(ic, IEEE80211_HTCAP_C_LDPC_RX);
            }
            if (ht_cap_info & WMI_HOST_HT_CAP_TX_LDPC)
                ieee80211com_set_ldpccap(ic, IEEE80211_HTCAP_C_LDPC_TX);
        } else {
            ieee80211com_set_ldpccap(ic,IEEE80211_HTCAP_C_LDPC_NONE);
        }
    }
}

/**
 * ol_ath_update_vht_caps() - updates iee80211 vht capabilities info
 * @ic: ic pointer
 * @vht_cap_info: targets vht capabilities information
 * @vht_supp_mcs: mcs map supported in vht
 *
 * Return: none
 */
static void ol_ath_update_vht_caps(struct ieee80211com *ic,
                                   uint32_t vht_cap_info, uint32_t vht_supp_mcs)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    uint32_t ampdu_exp = 0;
    struct wmi_unified *wmi_handle;
    struct wmi_unified *pdev_wmi_handle;
    uint16_t basic_mcs = 0xfffc;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return;
    }

    if (wmi_service_enabled(wmi_handle, wmi_service_11ac)) {

        /* Copy the VHT capabilities information */
        ieee80211com_set_vhtcap(ic, vht_cap_info);

        /* Adjust HT AMSDU len based on VHT MPDU len */
        if ((vht_cap_info & IEEE80211_VHTCAP_MAX_MPDU_LEN_7935) ||
            (vht_cap_info & IEEE80211_VHTCAP_MAX_MPDU_LEN_11454)) {
            ieee80211com_set_htcap(ic,IEEE80211_HTCAP_C_MAXAMSDUSIZE);
        } else {
            ieee80211com_clear_htcap(ic,IEEE80211_HTCAP_C_MAXAMSDUSIZE);
        }

        /* Adjust HT AMPDU len Exp  based on VHT MPDU len */
        ampdu_exp = vht_cap_info >> IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S;
        switch (ampdu_exp) {
            case 0:
            case 1:
            case 2:
            case 3:
                ieee80211com_set_maxampdu(ic, ampdu_exp);
            break;

            default:
                ieee80211com_set_maxampdu(ic, IEEE80211_HTCAP_MAXRXAMPDU_65536);
            break;
        }

        /* Set the VHT rate information */
        /* 11ac spec states it is mandatory to support MCS 0-7 and NSS=1 */
        ol_ath_vht_rate_setup(ic, vht_supp_mcs, 0, basic_mcs);

        ic->ic_vht_ampdu = 64;
        /* The default max amsdu value in FW for Beeliner family is 4
         * and for Peregrine is 3. So init the values accordingly.
         */
        ic->ic_vht_amsdu = cfg_get(psoc, CFG_TGT_VHT_AMSDU);
        if (wmi_service_enabled(pdev_wmi_handle,
                                wmi_service_extended_nss_support)) {
            ic->ic_fw_ext_nss_capable = 1;
            ic->ic_ext_nss_capable = 1;
        }
    }
}

static void ol_ath_update_6g_band_caps(struct ieee80211com *ic)
{
    uint16_t *he_6g_bandcap = (uint16_t *)ic->ic_he.he6g_bandcap;
    uint16_t htcap          = ic->ic_htcap;
    uint32_t vhtcap         = ic->ic_vhtcap;

    *he_6g_bandcap |= (((vhtcap & IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP) >>
                                IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S) <<
                                IEEE80211_6G_BANDCAP_MAX_AMPDU_LEN_EXP_S);
    *he_6g_bandcap |= ((vhtcap & IEEE80211_VHTCAP_MAX_MPDU_LEN_MASK) <<
                                IEEE80211_6G_BANDCAP_MAX_MPDU_LEN_S);
    *he_6g_bandcap |= (((htcap & IEEE80211_HTCAP_C_SMPOWERSAVE_MASK) >>
                                IEEE80211_HTCAP_C_SMPOWERSAVE_S) <<
                                IEEE80211_6G_BANDCAP_SMPOWERSAVE_S);
    *he_6g_bandcap |= (((vhtcap & IEEE80211_VHTCAP_RX_ANTENNA_PATTERN) >>
                                IEEE80211_VHTCAP_RX_ANTENNA_PATTERN_S) <<
                                IEEE80211_6G_BANDCAP_RX_ANTENNA_PATTERN_S);
    *he_6g_bandcap |= (((vhtcap & IEEE80211_VHTCAP_TX_ANTENNA_PATTERN) >>
                                IEEE80211_VHTCAP_TX_ANTENNA_PATTERN_S) <<
                                IEEE80211_6G_BANDCAP_TX_ANTENNA_PATTERN_S);

}

/*
 * Return true if target has HW capability support
 * for smart monitor else return false.
 */
bool ol_tgt_lithium_hw_nac_supp(struct wlan_objmgr_psoc *psoc)
{
    switch (lmac_get_tgt_type(psoc)) {
        case TARGET_TYPE_QCA8074V2:
        case TARGET_TYPE_QCA9574:
        case TARGET_TYPE_QCA6018:
        case TARGET_TYPE_QCA5018:
        case TARGET_TYPE_QCA5332:
        case TARGET_TYPE_QCN9000:
        case TARGET_TYPE_QCN9224:
        case TARGET_TYPE_QCN6122:
        case TARGET_TYPE_QCN9160:
        case TARGET_TYPE_QCN6432:
            return true;
    }

    return false;
}

/**
 * ol_ath_get_monitor_version() - get monitor version
 * @psoc: soc context
 *
 * Return: monitor version
 */
uint8_t ol_ath_get_monitor_version(struct wlan_objmgr_psoc *psoc)
{
    uint8_t version = 0;

    switch (lmac_get_tgt_type(psoc)) {
        case TARGET_TYPE_QCA8074:
        case TARGET_TYPE_QCA8074V2:
        case TARGET_TYPE_QCA6018:
        case TARGET_TYPE_QCA5018:
        case TARGET_TYPE_QCN9000:
        case TARGET_TYPE_QCN6122:
        case TARGET_TYPE_QCN9160:
        case TARGET_TYPE_QCA9574:
            version = MONITOR_VERSION_1;
            break;

        case TARGET_TYPE_QCN9224:
        case TARGET_TYPE_QCA5332:
        case TARGET_TYPE_QCN6432:
            version = MONITOR_VERSION_2;
            break;

        default:
            break;
    }

    return version;
}

#define DEFAULT_BURST_DURATION 8160
int ol_ath_pdev_set_burst(struct ol_ath_softc_net80211 *scn, bool value)
{
    struct wmi_unified *wmi_handle;
    int retval = 0;

    wmi_handle = lmac_get_wmi_hdl(scn->soc->psoc_obj);
    if(!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_service_enabled(wmi_handle, wmi_service_burst)) {
        /* Target is capable of burst mode. Therefore, enable the burst mode
         * in host and send WMI_PDEV_PARAM_BURST_ENABLE command to target.
         */
        retval = ol_ath_pdev_set_param(scn->sc_pdev,
                wmi_pdev_param_burst_enable, value);
        if (retval == EOK)
            scn->burst_enable = value;

        if (!scn->burst_dur) {
            retval = ol_ath_pdev_set_param(scn->sc_pdev,
                    wmi_pdev_param_burst_dur, DEFAULT_BURST_DURATION);
            if (retval == EOK)
                scn->burst_dur = DEFAULT_BURST_DURATION;
        }
    } else {
        retval = -EINVAL;
    }

    return retval;
}

/**
 * ol_ath_update_caps() - updates iee80211 capabilities/flags
 * @ic: ic pointer
 * @ev: target capabilities
 *
 * Return: none
 */
static void ol_ath_update_caps(struct ieee80211com *ic,
                               struct wlan_psoc_target_capability_info *ev)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    uint16_t ciphercap = 0;
    uint32_t target_type;
    struct wmi_unified *pdev_wmi_handle;
    ol_txrx_soc_handle soc_txrx_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    target_type = lmac_get_tgt_type(psoc);

    /* setup ieee80211 flags */
    ieee80211com_clear_cap(ic, -1);
    ieee80211com_clear_athcap(ic, -1);
    ieee80211com_clear_athextcap(ic, -1);
    ieee80211com_clear_ciphercap(ic, -1);
    ieee80211com_set_phytype(ic, IEEE80211_T_OFDM);
    ieee80211com_set_cap(ic,IEEE80211_C_SHPREAMBLE);

    ieee80211com_set_cap(ic,
                     IEEE80211_C_IBSS           /* ibss, nee adhoc, mode */
                     | IEEE80211_C_HOSTAP       /* hostap mode */
                     | IEEE80211_C_MONITOR      /* monitor mode */
                     | IEEE80211_C_SHSLOT       /* short slot time supported */
                     | IEEE80211_C_PMGT         /* capable of power management*/
                     | IEEE80211_C_WPA          /* capable of WPA1+WPA2 */
                     | IEEE80211_C_BGSCAN       /* capable of bg scanning */
                  );

    /* Setting cipher cap */
    ciphercap = ( (1 << IEEE80211_CIPHER_WEP) |
               (1 << IEEE80211_CIPHER_TKIP) |
               (1 << IEEE80211_CIPHER_AES_OCB) |
               (1 << IEEE80211_CIPHER_AES_CCM) |
               (1 << IEEE80211_CIPHER_WAPI) |
               (1 << IEEE80211_CIPHER_CKIP) |
               (1 << IEEE80211_CIPHER_AES_CMAC) |
               (1 << IEEE80211_CIPHER_AES_CCM_256) |
               (1 << IEEE80211_CIPHER_AES_GCM) |
               (1 << IEEE80211_CIPHER_AES_GCM_256) |
               (1 << IEEE80211_CIPHER_AES_GMAC) |
               (1 << IEEE80211_CIPHER_AES_GMAC_256) |
               (1 << IEEE80211_CIPHER_NONE)
             );
    /*
     * AR9888 Family of Chips do not support GCMP and CCMP-256
     */
    if ( (target_type == TARGET_TYPE_AR9888) ||
         (target_type == TARGET_TYPE_AR9888V2) )
    {
        ciphercap &= (~((1 << IEEE80211_CIPHER_AES_CCM_256) |
                        (1 << IEEE80211_CIPHER_AES_GCM) |
                        (1 << IEEE80211_CIPHER_AES_GCM_256) |
                        (1 << IEEE80211_CIPHER_AES_GMAC) |
                        (1 << IEEE80211_CIPHER_AES_GMAC_256)) & 0xffff);
    }

    ieee80211com_set_ciphercap(ic, ciphercap);
    ieee80211com_set_cap(ic, ciphercap);
    /* WMM enable */
    ieee80211com_set_cap(ic, IEEE80211_C_WME);

    ol_ath_pdev_set_burst(scn, true);

    if (wmi_service_enabled(pdev_wmi_handle, wmi_service_ap_uapsd)) {
        ieee80211com_set_cap(ic, IEEE80211_C_UAPSD);
        IEEE80211_UAPSD_ENABLE(ic);
    }

    /* Default 11h to start enabled  */
    ieee80211_ic_doth_set(ic);
#if UMAC_SUPPORT_WNM
    /* Default WNM enabled   */
    ieee80211_ic_wnm_set(ic);
    /* WNM needs PS state of STA, so enable it in the FW */
    ol_ath_pdev_set_param(scn->sc_pdev,
                          wmi_pdev_peer_sta_ps_statechg_enable, 1);
    scn->ps_report = 1;
#endif

    /* 11n Capabilities */
    ieee80211com_set_num_tx_chain(ic,1);
    ieee80211com_set_num_rx_chain(ic,1);
    ieee80211com_clear_htcap(ic, -1);
    ieee80211com_clear_htextcap(ic, -1);
    ol_ath_update_ht_caps(ic, ev->ht_cap_info);

    /* 11n configuration */
    ieee80211com_clear_htflags(ic, -1);

    /*
     * Indicate we need the 802.11 header padded to a
     * 32-bit boundary for 4-address and QoS frames.
     */
    IEEE80211_ENABLE_DATAPAD(ic);

    /* Check whether the hardware is VHT capable */
    ieee80211com_clear_vhtcap(ic, -1);
    ol_ath_update_vht_caps(ic, ev->vht_cap_info, ev->vht_supp_mcs);

    if (cfg_get(psoc, CFG_OL_ENABLE_MESH_SUPPORT) &&
                wmi_service_enabled(pdev_wmi_handle, wmi_service_mesh)) {
        qdf_info("Mesh Supported");
        ic->ic_mesh_vap_support = 1;
    }

    /* this should be updated from service bit map.
     * This change is added temporarily untill firmware support is added*/
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    ic->ic_promisc_support = 1;
    ic->ic_tso_support =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_TSO);
    ic->ic_lro_support =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_LRO);
    ic->ic_sg_support  =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_SG);
    ic->ic_gro_support =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_GRO);
    ic->ic_offload_tx_csum_support =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_OL_TX_CSUM);
    ic->ic_offload_rx_csum_support =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_OL_RX_CSUM);
    ic->ic_rawmode_support =
                cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_RAWMODE);
    ic->ic_peer_flow_control_support =
            cdp_get_dp_capabilities(soc_txrx_handle, CDP_CFG_DP_PEER_FLOW_CTRL);
    ic->ic_dynamic_grouping_support =
                                    cfg_get(psoc, CFG_TGT_DYN_GROUPING_SUPPORT);
    ic->ic_dpd_support            = cfg_get(psoc, CFG_TGT_DPD_SUPPORT);
    ic->ic_aggr_burst_support     = cfg_get(psoc, CFG_TGT_AGGR_BURST_SUPPORT);
    ic->ic_sifs_frame_support     = cfg_get(psoc, CFG_TGT_SIFS_FRAME_SUPPORT);
    ic->ic_block_interbss_support = cfg_get(psoc, CFG_TGT_BLK_INTERBSS_SUPPORT);
    ic->ic_disable_reset_support  = cfg_get(psoc, CFG_TGT_DIS_RESET_SUPPORT);
    ic->ic_msdu_ttl_support       = cfg_get(psoc, CFG_TGT_MSDU_TTL_SUPPORT);
    ic->ic_ppdu_duration_support  = cfg_get(psoc, CFG_TGT_PPDU_DUR_SUPPORT);
    ic->ic_burst_mode_support     = cfg_get(psoc, CFG_TGT_BURST_MODE_SUPPORT);
    ic->ic_wds_support            = cfg_get(psoc, CFG_TGT_WDS_SUPPORT);
    ic->ic_def_num_clients        = cfg_get(psoc, CFG_TGT_NUM_CLIENT);
    ic->ic_nac_client             = cfg_get(psoc, CFG_TGT_NAC_MAX_CLIENT);
    ic->ic_nac_bssid              = cfg_get(psoc, CFG_TGT_NAC_MAX_BSSID);

    /* Set smart monitor HW capability */
    if (ol_tgt_lithium_hw_nac_supp(psoc))
            ic->ic_hw_nac_monitor_support = 1;
    else
        qdf_err("Hw capability for nac is not supported");

    /* Set no bfee limit for all lithium targets and above.
     * Legacy targets will enable bfee based on total rx
     * streams and will not enable bfee by default.
     */
    ic->ic_no_bfee_limit = cfg_get(psoc, CFG_TGT_NO_BFEE_LIMIT_SUPPORT);
    ic->ic_he_target     = cfg_get(psoc, CFG_TGT_HE_TGT_SUPPORT);
    ic->ic_awgn_event_toezmesh = cfg_get(psoc, CFG_TGT_AWGN_EVENT_TOEZMESH_SUPPORT);
    ic->ic_advertise_max_cap_in_ies = cfg_get(psoc, CFG_TGT_ADVERTISE_MAX_CAP_IN_IES_SUPPORT);

    if (target_type >= TARGET_TYPE_QCA8074) {
        /* Set flag for ppdu_duration and burst_dur
         * HW requirement for Lithium based targets to support ppdu_duration
         * and burst_dur values greater than or equal to zero.
         */
        ic->ic_ppdu_min = 0;
        ic->ic_burst_min = 0;
    } else {
        ic->ic_ppdu_min = 100;
        ic->ic_burst_min = 1;
    }

    /* BA Buf Size will be initialized to 256 for HKv1/2, qca6018
     * and initialized to 64 for all other targets.
     */
    if (ol_target_lithium(psoc)) {
        ic->ic_he_default_ba_buf_size = IEEE80211_MAX_BA_BUFFER_SIZE;
    } else {
        ic->ic_he_default_ba_buf_size = IEEE80211_MIN_BA_BUFFER_SIZE;
    }

    /* The FW default for the UL PPDU Duration is 2000 usecs. */
    ic->ic_he_ul_ppdu_dur = 2000;

    /* ToDo, check ev->sys_cap_info for  WMI_SYS_CAP_ENABLE and WMI_SYS_CAP_TXPOWER when it is available from FW */
    ieee80211com_set_cap(ic, IEEE80211_C_TXPMGT);

    ieee80211_wme_initglobalparams(ic);
    /* Initialize MU EDCA parameters with defaults for the ic */
    ieee80211_muedca_initglobalparams(ic);

    /* For targets such HKv1 and lower, this is set to 0 */
    if (target_type >= TARGET_TYPE_QCA8074V2)
        ic->ic_he_ul_muofdma = 1;

    if (target_type >= TARGET_TYPE_QCN9224 ||
        target_type >= TARGET_TYPE_QCN6432) {
        ic->ic_eht_ul_ofdma = 1;
    }

    /* set monitor version */
    ic->ic_monitor_version = ol_ath_get_monitor_version(psoc);

}

static void dbg_print_wmi_service_11ac(ol_ath_soc_softc_t *soc,
                                                struct wlan_psoc_target_capability_info  *ev)
{
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return;
    }

    if (wmi_service_enabled(wmi_handle, wmi_service_11ac)) {
	qdf_nofl_debug("num_rf_chain:0x%08x  ", ev->num_rf_chains);
	qdf_nofl_debug("ht_cap_info:0x%08x  ", ev->ht_cap_info);
	qdf_nofl_debug("vht_cap_info:0x%08x  ", ev->vht_cap_info);
	qdf_nofl_debug("vht_supp_mcs:0x%08x", ev->vht_supp_mcs);
    }
    else {
	qdf_nofl_debug("\n No WMI 11AC service event received");
    }
}

static void
ol_ath_update_ic_hecap_mcsnssmap_from_target_hecap_mcsnssmap(
                                    struct ieee80211_he_handle *ic_he,
                                    uint32_t target_hecap_mcsnssmap)
{
    /* Target sends HE MCS-NSS info for less than equal to 80MHz encoded
     * in the lower 16 bits */
    ic_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80]    =
                    HECAP_TXRX_MCS_NSS_GET_LT80_INFO(target_hecap_mcsnssmap);
    /* Target sends HE MCS-NSS info for greater than 80MHz encoded in the
     * upper 16 bits */
    ic_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160]   =
                    HECAP_TXRX_MCS_NSS_GET_GT80_INFO(target_hecap_mcsnssmap);
    /* Target sends HE MCS-NSS info for greater than 80MHz encoded in the
     * upper 16 bits */
    ic_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] =
                    HECAP_TXRX_MCS_NSS_GET_GT80_INFO(target_hecap_mcsnssmap);
    /* Target is sending only rxmcsnss values. For the time being we consider
     * txmcsnss values to be same as those or rxmcsnss
     */
    ic_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80]    =
                    HECAP_TXRX_MCS_NSS_GET_LT80_INFO(target_hecap_mcsnssmap);
    ic_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160]   =
                    HECAP_TXRX_MCS_NSS_GET_GT80_INFO(target_hecap_mcsnssmap);
    ic_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] =
                    HECAP_TXRX_MCS_NSS_GET_GT80_INFO(target_hecap_mcsnssmap);
}

static inline void ol_ath_populate_he_caps(struct ieee80211com *ic,
        uint32_t *he_cap_info, uint32_t *he_supp_mcs,
        uint32_t *he_cap_phy_info, uint8_t he_cap_phy_info_size,
        struct wlan_psoc_host_ppe_threshold *he_ppet,
        uint32_t *he_cap_info_internal)
{
    struct ieee80211_he_handle *ic_he;
    ic_he = &ic->ic_he;

    if (he_cap_phy_info[0] &
        IEEE80211_HECAP_LDPC_MASK) {
        ieee80211com_set_ldpccap(ic, IEEE80211_HECAP_C_LDPC_TXRX);
    }

    qdf_mem_copy(&ic_he->hecap_macinfo, he_cap_info,
            sizeof(ic_he->hecap_macinfo));
    qdf_mem_copy(ic_he->hecap_phyinfo, he_cap_phy_info,
            he_cap_phy_info_size);
    qdf_mem_copy(&ic_he->hecap_info_internal, he_cap_info_internal,
            sizeof(ic_he->hecap_info_internal));
    ol_ath_update_ic_hecap_mcsnssmap_from_target_hecap_mcsnssmap(
            ic_he, *he_supp_mcs);
    qdf_mem_copy(&ic_he->hecap_ppet, he_ppet,
            sizeof(*he_ppet));

}

#ifdef WLAN_FEATURE_11BE
static void
ol_ath_update_ic_ehtcap_mcsnssmap_from_target(
                                    struct ieee80211_eht_handle *ic_eht,
                                    uint32_t *target_ehtcap_mcsnssmap,
                                    uint32_t eht_supp_mcs_size)
{
    int bw_i = 0;
    uint32_t bytes = 0;

    while ((bw_i < EHTCAP_TXRX_MCS_NSS_IDX_MAX) && (bytes < eht_supp_mcs_size)) {
        ic_eht->ehtcap_rxmcsnssmap[bw_i] = convert_mcs_map_to_rx_mcs_map(target_ehtcap_mcsnssmap[bw_i], bw_i == EHTCAP_TXRX_MCS_NSS_IDX_ONLY20);
        ic_eht->ehtcap_txmcsnssmap[bw_i] = convert_mcs_map_to_tx_mcs_map(target_ehtcap_mcsnssmap[bw_i], bw_i == EHTCAP_TXRX_MCS_NSS_IDX_ONLY20);
        target_if_info("bwidx = %d mcsnssmap %x rxmap %x txmap %x", bw_i, target_ehtcap_mcsnssmap[bw_i], ic_eht->ehtcap_rxmcsnssmap[bw_i], ic_eht->ehtcap_txmcsnssmap[bw_i]);
        bw_i++;
        bytes += sizeof(uint32_t);
    }
}

static void ol_ath_populate_eht_caps(struct ieee80211com *ic,
        uint32_t *eht_cap_info, uint32_t *eht_supp_mcs, uint32_t eht_supp_mcs_size,
        uint32_t *eht_cap_phy_info, uint8_t eht_cap_phy_info_size,
        struct wlan_psoc_host_ppe_threshold *eht_ppet,
        uint32_t *eht_cap_info_internal)
{
    struct ieee80211_eht_handle *ic_eht;
    ic_eht = &ic->ic_eht;

    qdf_mem_copy(&ic_eht->ehtcap_macinfo, eht_cap_info,
            sizeof(ic_eht->ehtcap_macinfo));
    qdf_mem_copy(ic_eht->ehtcap_phyinfo, eht_cap_phy_info,
            eht_cap_phy_info_size);
    qdf_mem_copy(&ic_eht->ehtcap_info_internal, eht_cap_info_internal,
            sizeof(ic_eht->ehtcap_info_internal));
    ol_ath_update_ic_ehtcap_mcsnssmap_from_target(
            ic_eht, eht_supp_mcs, eht_supp_mcs_size);
    qdf_mem_copy(&ic_eht->ehtcap_ppet, eht_ppet,
            sizeof(*eht_ppet));
}
#endif /* WLAN_FEATURE_11BE */

#ifdef CONFIG_2G_5G_WIDEBAND_SUPPPORT
/**
 * ol_ath_populate_2g5g_hecaps(): Populate 2.4 GHz/5GHz capability to the
 * ic_hecap.
 * @mac_phy_cap: Pointer to wlan_psoc_host_mac_phy_caps
 * @ic: Pointer to struct ieee80211com
 */
static void
ol_ath_populate_2g5g_hecaps(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
                            struct ieee80211com *ic)
{
    uint8_t i;
    uint32_t he_cap_phy_info_2G_5G[PSOC_HOST_MAX_PHY_SIZE] = {0};

    for(i = 0; i < PSOC_HOST_MAX_PHY_SIZE; i++) {
        he_cap_phy_info_2G_5G[i] =  mac_phy_cap->he_cap_phy_info_5G[i] |
            mac_phy_cap->he_cap_phy_info_2G[i];
    }
    qdf_info("Both bands supported case %x, populate both \
             5G and 2G HE caps, he_cap_phy_info_2G_5G[0]: %x", mac_phy_cap->supported_bands,
             he_cap_phy_info_2G_5G[0]);
    ol_ath_populate_he_caps(ic, mac_phy_cap->he_cap_info_5G,
                            &mac_phy_cap->he_supp_mcs_5G,
                            he_cap_phy_info_2G_5G, sizeof(he_cap_phy_info_2G_5G),
                            &mac_phy_cap->he_ppet5G, &mac_phy_cap->he_cap_info_internal);
}
#else
static inline void
ol_ath_populate_2g5g_hecaps(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
                            struct ieee80211com *ic)
{

}
#endif

static void
ol_ath_update_ext_he_caps(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
                       struct ieee80211com *ic)
{

    if (!mac_phy_cap || !ic) {
        qdf_err("mac_phy_cap or ic is Null");
        return;
    }

    /* Populate 11ax caps to IC */
    if (mac_phy_cap->supports_11ax) {
        ieee80211com_set_cap_ext(ic, IEEE80211_CEXT_11AX);
        if (mac_phy_cap->supported_bands == WMI_HOST_WLAN_5G_CAPABILITY) {

            qdf_debug("11ax 5G supported case");
            ol_ath_populate_he_caps(ic, mac_phy_cap->he_cap_info_5G,
                    &mac_phy_cap->he_supp_mcs_5G,
                    mac_phy_cap->he_cap_phy_info_5G, sizeof(mac_phy_cap->he_cap_phy_info_5G),
                    &mac_phy_cap->he_ppet5G, &mac_phy_cap->he_cap_info_internal);

        } else if (mac_phy_cap->supported_bands == WMI_HOST_WLAN_2G_CAPABILITY) {

            qdf_debug("11ax 2G supported case");
            ol_ath_populate_he_caps(ic, mac_phy_cap->he_cap_info_2G,
                    &mac_phy_cap->he_supp_mcs_2G,
                    mac_phy_cap->he_cap_phy_info_2G, sizeof(mac_phy_cap->he_cap_phy_info_2G),
                    &mac_phy_cap->he_ppet2G, &mac_phy_cap->he_cap_info_internal);

        } else {
#ifdef CONFIG_2G_5G_WIDEBAND_SUPPPORT
            ol_ath_populate_2g5g_hecaps(mac_phy_cap, ic);
#else
            qdf_info("Both bands supported case %x, default to populate \
               5G HE caps", mac_phy_cap->supported_bands);

            ol_ath_populate_he_caps(ic, mac_phy_cap->he_cap_info_5G,
                    &mac_phy_cap->he_supp_mcs_5G,
                    mac_phy_cap->he_cap_phy_info_5G, sizeof(mac_phy_cap->he_cap_phy_info_5G),
                    &mac_phy_cap->he_ppet5G, &mac_phy_cap->he_cap_info_internal);
#endif
        }
    } else {
        ieee80211com_clear_cap_ext(ic, IEEE80211_CEXT_11AX);
        qdf_info("11AX not supported ");
    }
}

#if defined(CONFIG_2G_5G_WIDEBAND_SUPPPORT) && defined(WLAN_FEATURE_11BE)
/**
 * ol_ath_populate_2g5g_eht_caps(): Populate 2.4 GHz/5GHz capability to the
 * ic_ehtcap.
 * @mac_phy_caps_ext2: Pointer to struct wlan_psoc_host_mac_phy_caps_ext2
 * @ic: Pointer to struct ieee80211com
 * @mac_phy_cap: Pointer to wlan_psoc_host_mac_phy_caps
 */
static void
ol_ath_populate_2g5g_eht_caps(struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2,
                              struct ieee80211com *ic,
                              struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
    uint8_t i;
    uint32_t eht_cap_phy_info_2G_5G[PSOC_HOST_MAX_EHT_PHY_SIZE] = {0};

    for(i = 0; i < PSOC_HOST_MAX_EHT_PHY_SIZE; i++) {
        eht_cap_phy_info_2G_5G[i] = mac_phy_caps_ext2->eht_cap_phy_info_5G[i] | mac_phy_caps_ext2->eht_cap_phy_info_2G[i];
    }
    qdf_info("Both bands supported case %x, default to populate \
             5G and 2G eht caps, eht_cap_phy_info_2G_5G[0]: %x", mac_phy_cap->supported_bands, eht_cap_phy_info_2G_5G[0]);
    ol_ath_populate_eht_caps(ic, mac_phy_caps_ext2->eht_cap_info_5G,
                             mac_phy_caps_ext2->eht_supp_mcs_ext_5G, sizeof(mac_phy_caps_ext2->eht_supp_mcs_ext_5G),
                             eht_cap_phy_info_2G_5G, sizeof(eht_cap_phy_info_2G_5G),
                             &mac_phy_caps_ext2->eht_ppet5G, &mac_phy_caps_ext2->eht_cap_info_internal);
}
#else
static inline void
ol_ath_populate_2g5g_eht_caps(struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2,
                              struct ieee80211com *ic,
                              struct wlan_psoc_host_mac_phy_caps *mac_phy_cap)
{
}
#endif

#ifdef WLAN_FEATURE_11BE
static void
ol_ath_update_ext_eht_caps(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
                       struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2,
                       struct ieee80211com *ic)
{

    if (!mac_phy_cap || !mac_phy_caps_ext2 || !ic) {
        qdf_err("mac_phy_caps or ac_phy_caps_ext2 or ic is Null");
        return;
    }

    /* Populate 11be caps to IC */
    if (mac_phy_cap->supports_11be) {
        ieee80211com_set_cap_ext(ic, IEEE80211_CEXT_11BE);
        if (mac_phy_cap->supported_bands == WMI_HOST_WLAN_5G_CAPABILITY) {

            qdf_info("11be 5G supported case");
            ol_ath_populate_eht_caps(ic, mac_phy_caps_ext2->eht_cap_info_5G,
                    mac_phy_caps_ext2->eht_supp_mcs_ext_5G, sizeof(mac_phy_caps_ext2->eht_supp_mcs_ext_5G),
                    mac_phy_caps_ext2->eht_cap_phy_info_5G, sizeof(mac_phy_caps_ext2->eht_cap_phy_info_5G),
                    &mac_phy_caps_ext2->eht_ppet5G, &mac_phy_caps_ext2->eht_cap_info_internal);

        } else if (mac_phy_cap->supported_bands == WMI_HOST_WLAN_2G_CAPABILITY) {

            qdf_info("11be 2G supported case");
            ol_ath_populate_eht_caps(ic, mac_phy_caps_ext2->eht_cap_info_2G,
                    mac_phy_caps_ext2->eht_supp_mcs_ext_2G, sizeof(mac_phy_caps_ext2->eht_supp_mcs_ext_2G),
                    mac_phy_caps_ext2->eht_cap_phy_info_2G, sizeof(mac_phy_caps_ext2->eht_cap_phy_info_2G),
                    &mac_phy_caps_ext2->eht_ppet2G, &mac_phy_caps_ext2->eht_cap_info_internal);

        } else {
#ifdef CONFIG_2G_5G_WIDEBAND_SUPPPORT
            ol_ath_populate_2g5g_eht_caps(mac_phy_caps_ext2, ic, mac_phy_cap);
#else
            qdf_info("Both bands supported case %x, default to populate \
               5G eht caps", mac_phy_cap->supported_bands);

            ol_ath_populate_eht_caps(ic, mac_phy_caps_ext2->eht_cap_info_5G,
                    mac_phy_caps_ext2->eht_supp_mcs_ext_5G, sizeof(mac_phy_caps_ext2->eht_supp_mcs_ext_5G),
                    mac_phy_caps_ext2->eht_cap_phy_info_5G, sizeof(mac_phy_caps_ext2->eht_cap_phy_info_5G),
                    &mac_phy_caps_ext2->eht_ppet5G, &mac_phy_caps_ext2->eht_cap_info_internal);
#endif
        }
    } else {
        ieee80211com_clear_cap_ext(ic, IEEE80211_CEXT_11BE);
        qdf_info("11BE not supported ");
    }
}
#else
static void
ol_ath_update_ext_eht_caps(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
                       struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2,
                       struct ieee80211com *ic)
{
}
#endif /* WLAN_FEATURE_11BE */

static void
ol_ath_update_ext_caps(struct wlan_psoc_host_mac_phy_caps *mac_phy_cap,
                       struct ieee80211com *ic)
{
    ol_ath_update_ext_he_caps(mac_phy_cap, ic);
}


uint32_t
ol_ath_get_host_pltfrm_mode(ol_ath_soc_softc_t *soc)
{
	uint32_t target_type;

	target_type = lmac_get_tgt_type(soc->psoc_obj);

	switch(target_type)
	{
	case TARGET_TYPE_AR6002:
	case TARGET_TYPE_AR6003:
	case TARGET_TYPE_AR6004:
	case TARGET_TYPE_AR6006:
	case TARGET_TYPE_AR9888:
	case TARGET_TYPE_AR6320:
		return HOST_PLATFORM_LOW_PERF;
#if PEER_FLOW_CONTROL_FORCED_MODE0
	case TARGET_TYPE_AR900B:
	case TARGET_TYPE_QCA9984:
	case TARGET_TYPE_QCA9888:
		return HOST_PLATFORM_LOW_PERF_NO_FETCH;
#elif MIPS_LOW_PERF_SUPPORT
	case TARGET_TYPE_AR900B:
	case TARGET_TYPE_QCA9984:
	case TARGET_TYPE_QCA9888:
		return HOST_PLATFORM_LOW_PERF;
#else
	case TARGET_TYPE_AR900B:
	case TARGET_TYPE_QCA9984:
	case TARGET_TYPE_QCA9888:
		return HOST_PLATFORM_HIGH_PERF;
#endif
	default:
		qdf_err("!!! Invalid Target Type %d !!!", target_type);
		return -EINVAL;
	}
	return EOK;
}

int
ol_ath_service_ready_event(ol_scn_t sc, uint8_t *evt_buf, uint32_t len)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct target_psoc_info *tgt_hdl;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    struct ol_ath_target_cap target_cap;

    QDF_STATUS status = wlan_objmgr_psoc_try_get_ref(psoc, WLAN_MLME_SB_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("unable to get psoc reference");
        return -1;
    }
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("psoc target hdl is null in service ready ev");
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_SB_ID);
        return -EINVAL;
    }

    target_cap.wlan_resource_config = tgt_hdl->info.wlan_res_cfg;

    /* call back into  os shim with the services bitmap and resource config to let
     * the os shim layer modify it according to its needs and requirements */
    if (soc->cfg_cb) {
        soc->cfg_cb(soc, &target_cap);
        tgt_hdl->info.wlan_res_cfg = target_cap.wlan_resource_config;
    }

    /* Dump service ready event for debugging */
    dbg_print_wmi_service_11ac(soc, &tgt_hdl->info.target_caps);

    wlan_objmgr_psoc_release_ref(psoc, WLAN_MLME_SB_ID);

    return 0;
}

static void ol_ath_print_bdf_version_ext2(void *scn_handle, uint8_t *event_data,
                                          uint32_t length)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) scn_handle;
    wmi_unified_t wmi_handle;
    struct wmi_host_sw_cal_ver cal;

    wmi_handle = GET_WMI_HDL_FROM_PSOC(soc->psoc_obj);
    if (wmi_handle) {
        if (wmi_extract_sw_cal_ver_ext2(wmi_handle, event_data, &cal) ==
                                        QDF_STATUS_SUCCESS) {
                    qdf_nofl_info("BDF version: %d CAL version: %d Status: %d",
                                  cal.bdf_cal_ver, cal.ftm_cal_ver, cal.status);
        }
    }
}

#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
/**
 * ol_mlo_glb_h_shmem_arena_ctx_init() - Initialize MLO global shared memory
 * arena context
 * @soc: pointer to soc
 *
 * Return: QDF_STATUS of operation
 */
static QDF_STATUS ol_mlo_glb_h_shmem_arena_ctx_init(ol_ath_soc_softc_t *soc)
{
    struct wlan_lmac_if_global_shmem_local_ops *shmem_local_ops;
    void *arena_vaddr;
    int arena_len;
    uint8_t grp_id;

    if (!soc) {
        qdf_err("soc  is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* We need to initialize shared memory for MLO capable devices only */
    if (!pld_get_mlo_capable(soc->qdf_dev->dev))
        return QDF_STATUS_SUCCESS;

    if (pld_get_mlo_global_config_region_info(soc->qdf_dev->dev,
                                              &arena_vaddr,
                                              &arena_len) != 0) {
        qdf_err("Unable to get MLO global region config from platform");
        return QDF_STATUS_E_FAILURE;
    }

    grp_id = pld_get_mlo_group_id(soc->qdf_dev->dev);
    shmem_local_ops =
            target_if_get_global_shmem_local_ops(soc->psoc_obj);

    if (!shmem_local_ops) {
        qdf_err("Low level ops of MGMT Rx REO is null");
        return QDF_STATUS_E_FAILURE;
    }

    qdf_assert_always(shmem_local_ops->implemented);

    return shmem_local_ops->init_shmem_arena_ctx(arena_vaddr, arena_len, grp_id);
}

/**
 * ol_mlo_glb_h_shmem_arena_ctx_deinit() - De-initialize MLO global shared
 * memory arena context
 * @soc: pointer to soc
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS ol_mlo_glb_h_shmem_arena_ctx_deinit(ol_ath_soc_softc_t *soc)
{
    struct wlan_lmac_if_global_shmem_local_ops *shmem_local_ops;
    uint8_t grp_id;

    if (!soc) {
        qdf_err("soc  is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    /* We need to de-initialize shared memory for MLO capable devices only */
    if (!pld_get_mlo_capable(soc->qdf_dev->dev))
        return QDF_STATUS_SUCCESS;

    grp_id = pld_get_mlo_group_id(soc->qdf_dev->dev);
    shmem_local_ops =
            target_if_get_global_shmem_local_ops(soc->psoc_obj);

    if (!shmem_local_ops) {
        qdf_err("SHMEM OPS is null");
        return QDF_STATUS_E_FAILURE;
    }

    qdf_assert_always(shmem_local_ops->implemented);

    return shmem_local_ops->deinit_shmem_arena_ctx(grp_id);

}

/**
 * ol_mlo_glb_h_shmem_arena_dump() - Dump specified number of bytes
 * of MLO global shmem arena
 * @vscn: pointer to scn
 * @num_bytes: number of bytes to dump
 *
 * Return: QDF_STATUS of operation
 */
static QDF_STATUS
ol_mlo_glb_h_shmem_arena_dump(void *vscn, uint16_t num_bytes)
{
#define MLO_GLOBAL_H_SHMEM_ARENA_DUMP_MAX_BYTES       (512)
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211*)vscn;
    ol_ath_soc_softc_t *soc;
    void *arena_vaddr;
    int arena_len;

    if (!scn) {
        qdf_err("scn  is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    soc = scn->soc;
    if (!soc) {
        qdf_err("soc  is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!pld_get_mlo_capable(soc->qdf_dev->dev))
        return QDF_STATUS_SUCCESS;

    if (pld_get_mlo_global_config_region_info(soc->qdf_dev->dev,
                                              &arena_vaddr,
                                              &arena_len) != 0) {
        qdf_err("Unable to get MLO global region config from platform");
        return QDF_STATUS_E_FAILURE;
    }

    if (num_bytes > qdf_min(arena_len, (int)MLO_GLOBAL_H_SHMEM_ARENA_DUMP_MAX_BYTES)) {
        qdf_err("Number of bytes to dump %u > arena len %d", num_bytes,
                qdf_min(arena_len, (int)MLO_GLOBAL_H_SHMEM_ARENA_DUMP_MAX_BYTES));
        return QDF_STATUS_E_INVAL;
    }

    QDF_TRACE_HEX_DUMP(QDF_MODULE_ID_MGMT_RX_REO, QDF_TRACE_LEVEL_FATAL, arena_vaddr, num_bytes);

    return QDF_STATUS_SUCCESS;
}
#else
static QDF_STATUS ol_mlo_glb_h_shmem_arena_ctx_init(ol_ath_soc_softc_t *soc)
{
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_mlo_glb_h_shmem_arena_ctx_deinit(ol_ath_soc_softc_t *soc)
{
    return QDF_STATUS_SUCCESS;
}
static QDF_STATUS ol_mlo_glb_h_shmem_arena_dump(void *vscn, uint16_t num_bytes)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_MLO_GLOBAL_SHMEM_SUPPORT */

/**
 * ol_ath_unified_event_handler() - Global master wmi service handler
 * for both ext and ext service ready events
 * @event_id: event_id
 * @scn_handle: scn handle
 * @event_data: event data
 * @length: event length
 *
 * Return: 0 for success, negative error code for failure
 */
int
ol_ath_unified_event_handler(uint32_t event_id,
                             void *scn_handle,
                             uint8_t *event_data,
                             uint32_t length)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) scn_handle;
    int ret;
#ifdef IPA_OFFLOAD
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
#endif

    if (wmi_service_ready_event_id == event_id) {
        return ol_ath_service_ready_event(scn_handle, event_data, length);
    } else if (wmi_ready_event_id == event_id) {
        ret = ol_mlo_glb_h_shmem_arena_ctx_init(soc);
        if (QDF_IS_STATUS_SUCCESS(ret))
            soc->glb_shmem_init_complete = true;
        else {
            qdf_err("Failed to initialize MLO global shared memory arena");
            qdf_assert_always(0);
        }
#ifdef IPA_OFFLOAD
        ucfg_ipa_set_dp_handle(psoc, psoc->dp_handle);
        return 0;
#endif
    } else if (wmi_service_ready_ext2_event_id == event_id) {
        ol_ath_print_bdf_version_ext2(scn_handle, event_data, length);
    }

    return 0;
}

/**
 * register_legacy_wmi_service_ready_callback(void) - register wmi legacy callback
 *
 * Legacy wmi Callback registered globally to target_if ctx
 * for all offload based architectures
 *
 * Return: Success
 */
QDF_STATUS register_legacy_wmi_service_ready_callback(void)
{
    target_if_register_legacy_service_ready_cb(ol_ath_unified_event_handler);

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(register_legacy_wmi_service_ready_callback);

void ol_ath_self_recovery(void *psoc, enum qdf_hang_reason reason,
                          const char *func, const uint32_t line)
{
    struct wlan_objmgr_psoc *psoc_obj = psoc;
    ol_ath_soc_softc_t *soc = NULL;

    soc = lmac_get_psoc_feature_ptr(psoc_obj);
    if (soc && soc->target_status == OL_TRGET_STATUS_RESET) {
         qdf_info("Ignore as Target in asserted state: PSOC_%d: %s, reason:%d:%d", wlan_psoc_get_id(psoc_obj), func,
                   reason, line);
        return;
    }

    qdf_info("PSOC_%d: %s, reason:%d:%d", wlan_psoc_get_id(psoc_obj), func,
             reason, line);

    if (reason == QDF_WMI_EXCEED_MAX_PENDING_CMDS)
        QDF_DEBUG_PANIC("BUG ON due to QDF_WMI_EXCEED_MAX_PENDING_CMDS");

    if (ol_ath_set_fw_hang(psoc, 0, PANIC_TYPE_ASSERT)) {
        if (target_if_vdev_mgr_is_panic_on_bug())
            QDF_DEBUG_PANIC("Panic due to response timeout");
    }
}
qdf_export_symbol(ol_ath_self_recovery);

/*
 *  WMI API for setting fw hang.
 *  type parameter can be one of the 6 values defined in
 *  wmi_unified.h enumerated as RECOVERY_SIM_TYPE
 */
int
ol_ath_set_fw_hang(struct wlan_objmgr_psoc *psoc, u_int32_t delay_time_ms,
                   uint32_t assert_type)
{
    struct crash_inject param;
    wmi_unified_t wmi_handle = GET_WMI_HDL_FROM_PSOC(psoc);

    if (!wmi_handle) {
        qdf_err("Unable to trigger FW hang, as wmi handle is NULL");
        return -EINVAL;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.delay_time_ms = delay_time_ms;
    param.type = assert_type;

    wmi_tag_crash_inject(wmi_handle, true);

    return wmi_crash_inject(wmi_handle, &param);
}
qdf_export_symbol(ol_ath_set_fw_hang);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
int
ol_ath_send_fw_recovery(struct wlan_objmgr_psoc *psoc, uint32_t recovery_type)
{

    struct crash_inject param;
    wmi_unified_t wmi_handle = GET_WMI_HDL_FROM_PSOC(psoc);

    if (!wmi_handle) {
        qdf_err("Unable to trigger FW hang, as wmi handle is NULL");
        return -EINVAL;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.delay_time_ms = 0;
    param.type = recovery_type;

    return wmi_crash_inject(wmi_handle, &param);
}
qdf_export_symbol(ol_ath_send_fw_recovery);
#endif

static int
ol_ath_get_pn_event_handler(ol_scn_t sc, u_int8_t *evt_buf, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wmi_host_get_pn_event pn_ev;
    struct wlan_objmgr_vdev *vdev;
    uint8_t pdev_id;
    struct wlan_objmgr_peer *peer;
    struct wlan_crypto_key *key = NULL;
    /* PN is a 48-bit value represented as an array of 6 octets.
     * Hence size of keytsc value (keytsc_size) is 6 bytes.
     */
    uint8_t keytsc_size = SEQ_LEN_6;
    struct ieee80211_node *ni;

    psoc = soc->psoc_obj;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi handle is NULL");
        return -EINVAL;
    }

    if (wmi_unified_extract_pn(wmi_handle, evt_buf, &pn_ev) !=
                               QDF_STATUS_SUCCESS) {
        qdf_err("Unable to extract PN event");
        return -1;
    }

    if ((vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, pn_ev.vdev_id,
                                                     WLAN_CRYPTO_ID)) == NULL) {
        qdf_err("vdev object (id: %d) is NULL", pn_ev.vdev_id);
        return -1;
    }

    if ((pdev = wlan_vdev_get_pdev(vdev)) == NULL) {
        qdf_err("pdev object is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_CRYPTO_ID);
        return -1;
    }

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    if ((peer = wlan_objmgr_get_peer(psoc, pdev_id, pn_ev.mac_addr,
                                     WLAN_CRYPTO_ID)) == NULL) {
        qdf_err("peer object is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_CRYPTO_ID);
        return -1;
    }

    if (!qdf_mem_cmp(peer->macaddr, wlan_vdev_mlme_get_macaddr(vdev), QDF_MAC_ADDR_SIZE))
        key = wlan_crypto_vdev_getkey(vdev, pn_ev.key_ix);
    else
        key = wlan_crypto_peer_getkey(peer, pn_ev.key_ix);

    if (key)
        key->keytsc = 0;

    /*When hostapd requests for a PN fetch during M3,Firmware returns
     *the PN of next frame.So decrementing by 1 so IPN/BIPN added in M3
     *IGTK/BIGTK KDE is that of the previous frame.This is to prevent
     *flagging of BIP replay counter errors on the station side for the
     *immediate next frame.
     */
    if (key && key->cipher_type != WLAN_CRYPTO_CIPHER_WAPI_SMS4) {
        qdf_mem_copy((uint8_t *)(&key->keytsc), (uint8_t *)(pn_ev.pn),
                     keytsc_size);

	if (key->keytsc > 0x0)
		key->keytsc--;
    }

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni) {
        qdf_atomic_inc(&(ni->getpn));
        qdf_event_set(&ni->get_txrxpn_event);
    }

    wlan_objmgr_peer_release_ref(peer, WLAN_CRYPTO_ID);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_CRYPTO_ID);
    return 0;
}

static int
ol_ath_get_rxpn_event_handler(ol_scn_t sc, u_int8_t *evt_buf, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wmi_host_get_rxpn_event pn_ev;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_objmgr_peer *peer;
    struct wlan_crypto_key *key;
    struct ieee80211_node *ni;
    uint8_t pdev_id;

    psoc = soc->psoc_obj;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi handle is NULL");
        return -EINVAL;
    }

    if (wmi_unified_extract_rxpn(wmi_handle, evt_buf, &pn_ev) !=
                               QDF_STATUS_SUCCESS) {
        qdf_err("RxPN extrcation failed");
        return -EINVAL;
    }

    if ((vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, pn_ev.vdev_id,
                                                     WLAN_CRYPTO_ID)) == NULL) {
        qdf_err("vdev object (id: %d) is NULL", pn_ev.vdev_id);
        return -EINVAL;
    }

    if ((pdev = wlan_vdev_get_pdev(vdev)) == NULL) {
        qdf_err("pdev object is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_CRYPTO_ID);
        return -EINVAL;
    }

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    if ((peer = wlan_objmgr_get_peer(psoc, pdev_id, pn_ev.mac_addr,
                                     WLAN_CRYPTO_ID)) == NULL) {
        qdf_err("peer object is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_CRYPTO_ID);
        return -EINVAL;
    }

    key = wlan_crypto_peer_getkey(peer, pn_ev.keyix);

    if (key)
        qdf_mem_zero(key->keyrsc, sizeof(key->keyrsc));

    if (key && key->cipher_type != WLAN_CRYPTO_CIPHER_WAPI_SMS4)
        qdf_mem_copy((uint8_t *)(&key->keyrsc), (uint8_t *)(pn_ev.pn),
                     sizeof(pn_ev.pn));

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni) {
        qdf_atomic_inc(&(ni->get_rxpn));
        qdf_event_set(&ni->get_txrxpn_event);
    }

    wlan_objmgr_peer_release_ref(peer, WLAN_CRYPTO_ID);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_CRYPTO_ID);
    return 0;
}

struct ieee80211_ath_channel
ol_ath_find_full_channel(struct ieee80211com *ic, struct chan_params *chan_p)
{
   return wlan_find_full_channel(ic, chan_p);
}

/* Offload Interface functions for UMAC */
static int
ol_ath_init(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static int
ol_ath_reset_start(struct ieee80211com *ic, bool no_flush)
{
    /* TBD */
    return 0;
}

static int
ol_ath_reset_end(struct ieee80211com *ic, bool no_flush)
{
    /* TBD */
    return 0;
}

static void
ol_set_beacon_interval(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

static int
ol_ath_reset(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static void ol_ath_kbps_to_mcs(ol_ath_soc_softc_t *soc, struct ieee80211com *ic)
{
    if(soc->ol_if_ops->kbps_to_mcs)
        ic->ic_whal_kbps_to_mcs = soc->ol_if_ops->kbps_to_mcs;
}

static void
ol_ath_ratecode_to_kbps(ol_ath_soc_softc_t *soc, struct ieee80211com *ic)
{
    if(soc->ol_if_ops->ratecode_to_kbps)
        ic->ic_whal_ratecode_to_kbps = soc->ol_if_ops->ratecode_to_kbps;
}

static void
ol_ath_get_supported_rates(ol_ath_soc_softc_t *soc, struct ieee80211com *ic)
{
    if(soc->ol_if_ops->get_supported_rates)
        ic->ic_get_supported_rates = soc->ol_if_ops->get_supported_rates;
}

static int
ol_ath_wmm_update(struct ieee80211com *ic, struct ieee80211vap *vap,
        uint8_t muedca_enabled)
{
    struct wmm_update_params param;
    struct wmeParams *wmep = NULL;
    struct muedcaParams *muedcap = NULL;
    struct wmi_host_wme_vparams wmm_vparams[WME_NUM_AC] = {0};
    int ac = 0;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return -EINVAL;

    if (!wmi_service_enabled(pdev_wmi_handle, wmi_service_deprecated_replace)) {
        qdf_mem_set(&param, sizeof(param), 0);
        param.wmep_array = (struct wmi_host_wmeParams *)ic->ic_wme.wme_chanParams.cap_wmeParams;
        wmi_unified_wmm_update_cmd_send(pdev_wmi_handle, &param);
    } else {
        /* For lithium chips onwards */
        /* Fill values from ic_wme.wme_chanParams.cap_wmeParams to wmm_vparams */
        if(muedca_enabled) {
            for (ac = 0; ac < MUEDCA_NUM_AC; ac++) {
                muedcap = &(vap->iv_muedcastate.muedca_paramList[ac]);

                wmm_vparams[ac].acm = muedcap->muedca_acm;
                wmm_vparams[ac].aifs = muedcap->muedca_aifsn;
                wmm_vparams[ac].cwmin = ATH_EXPONENT_TO_VALUE(muedcap->muedca_ecwmin);
                wmm_vparams[ac].cwmax = ATH_EXPONENT_TO_VALUE(muedcap->muedca_ecwmax);
                wmm_vparams[ac].mu_edca_timer = muedcap->muedca_timer;
                wmm_vparams[ac].noackpolicy = 0;
            }

        }
        else {
            for (ac = 0; ac < WME_NUM_AC; ac++) {
                wmep = &(ic->ic_wme.wme_chanParams.cap_wmeParams[ac]);

                wmm_vparams[ac].acm = wmep->wmep_acm;
                wmm_vparams[ac].aifs = wmep->wmep_aifsn;
                wmm_vparams[ac].cwmin = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmin);
                wmm_vparams[ac].cwmax = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmax);
                wmm_vparams[ac].txoplimit = wmep->wmep_txopLimit;
                wmm_vparams[ac].noackpolicy = wmep->wmep_noackPolicy;
            }
        }
        wmi_unified_process_update_edca_param(pdev_wmi_handle,
                                              wlan_vdev_get_id(vap->vdev_obj),
                                              muedca_enabled, wmm_vparams);
    }

    return 0;
}

static int
ol_ath_epcs_edca_update(struct ieee80211com *ic, struct ieee80211vap *vap,
        uint8_t muedca_enabled)
{
    struct wmeParams *wmep = NULL;
    struct muedcaParams *muedcap = NULL;
    struct wmi_host_wme_vparams wmm_vparams[WME_NUM_AC] = {0};
    int ac = 0;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return -EINVAL;

    if (muedca_enabled) {
        for (ac = 0; ac < MUEDCA_NUM_AC; ac++) {
            muedcap = &(vap->iv_epcs_muedcastate.muedca_paramList[ac]);

            wmm_vparams[ac].acm = muedcap->muedca_acm;
            wmm_vparams[ac].aifs = muedcap->muedca_aifsn;
            wmm_vparams[ac].cwmin = ATH_EXPONENT_TO_VALUE(muedcap->muedca_ecwmin);
            wmm_vparams[ac].cwmax = ATH_EXPONENT_TO_VALUE(muedcap->muedca_ecwmax);
            wmm_vparams[ac].mu_edca_timer = muedcap->muedca_timer;
            wmm_vparams[ac].noackpolicy = 0;
        }
    } else {
        for (ac = 0; ac < WME_NUM_AC; ac++) {
            wmep = &(ic->ic_epcs_edca.wme_ChanParams.cap_wmeParams[ac]);

            wmm_vparams[ac].acm = wmep->wmep_acm;
            wmm_vparams[ac].aifs = wmep->wmep_aifsn;
            wmm_vparams[ac].cwmin = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmin);
            wmm_vparams[ac].cwmax = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmax);
            wmm_vparams[ac].txoplimit = wmep->wmep_txopLimit;
            wmm_vparams[ac].noackpolicy = wmep->wmep_noackPolicy;
        }
    }
    wmi_unified_process_update_edca_param(pdev_wmi_handle,
                                          wlan_vdev_get_id(vap->vdev_obj),
                                          muedca_enabled, wmm_vparams);

    return 0;
}

static u_int32_t
ol_ath_txq_depth(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static u_int32_t
ol_ath_txq_depth_ac(struct ieee80211com *ic,int ac)
{
    /* TBD */
    return 0;
}

/**
 * ol_ath_ar900b_fw_test() - sends fw test cmds via wmi
 * @arg: cmd
 * @value: cmd value
 *
 * Return: none
 */
static void ol_ath_ar900b_fw_test(struct ieee80211com *ic,
                                  uint32_t arg, uint32_t value )
{
    struct set_fwtest_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.arg = arg;
    param.value = value;

#if !(QCA_TGT_FW_IRAM_DUMP_ENABLE)
#define QCA_WIFITOOL_IRAM_DUMP_COMMAND 164
    if (arg == QCA_WIFITOOL_IRAM_DUMP_COMMAND) {
        qdf_err("Command not supported");
        return;
    }
#endif

    wmi_unified_vdev_set_fwtest_param_cmd_send(pdev_wmi_handle, &param);

    return;
}

static void get_sta_mode_vap(void *arg, struct ieee80211vap *vap)
{
    ieee80211_vap_t *ppvap=(ieee80211_vap_t *)arg;

    if (IEEE80211_M_STA == vap->iv_opmode) {
        *ppvap = vap;
    }

    return;
}

static void
ol_ath_net80211_enable_radar(struct ieee80211com *ic, int no_cac)
{
}

static int ol_ath_set_channel(struct ieee80211com *ic)
{
    struct ieee80211_ath_channel *chan;
    uint32_t freq;
    ieee80211_vap_t vap = NULL;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    chan = ieee80211_get_current_channel(ic);

    freq = ieee80211_chan2freq(ic, chan);
    if (!freq) {
        qdf_err("ERROR : INVALID Freq");
        return -1;
    }

    /* update max channel power to max regpower of current channel */
    ieee80211com_set_curchanmaxpwr(ic, chan->ic_maxregpower);

    /* Update the channel for monitor mode path */
    cdp_set_curchan(soc_txrx_handle, pdev_id, freq);

    wlan_iterate_vap_list(ic, get_sta_mode_vap ,(void *)&vap );
    if (vap) {
        /* There is a active STA mode vap.
         * for STA mode channel change will happen through resmgr channnel change
         */
         return -1;
    }
    else {
        /* Call a new channel change WMI cmd for all VAPs in AP/monitor mode.  */
        /****************************************
        FIXME: better way is to restart all VAPs in osif layer resmgr_vap_start() path,
        with parameter restart=1, e.g. wlan_mlme_start_bss(vap,1)
        ****************************************/
        if(IEEE80211_IS_CHAN_VALID(ic->ic_prevchan) &&
	   !IEEE80211_ARE_CHANS_EQUAL(chan, ic->ic_prevchan)){
            /*If different channel, need to restart all VAPs.
              If channel numbers are same, but channel ic_flags are different,
              we still consider this as 'different' channel */

            qdf_info("Invoking channel change via MVR");
            wlan_pdev_mlme_vdev_sm_chan_change(ic->ic_pdev_obj, chan);
        } else {
            /* If DCS has detected a interference, it sets the
             * channel_switch_state and gives control to ACS to choose a
             * random channel. If the channel chosen by ACS is same as
             * the current channel, channel change is blocked here. Hence
             * reset the channel_switch_state flag.
             */
            if (ic->cw_inter_found) {
                TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
                    if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                        (wlan_vdev_mlme_is_active(vap->vdev_obj) ==
                         QDF_STATUS_SUCCESS)) {
                        vap->channel_switch_state = 0;
                    }
                }
            }
        }
    }

    /* Restore dcs state to previous state so that interference detection will work*/
    if (ic->ic_dcs_restore)
        ic->ic_dcs_restore(ic);

    if (ic->ic_cbs && ic->ic_cbs->cbs_enable)
        wlan_bk_scan(ic);

    return 0;
}

static void ol_ath_log_text(struct ieee80211com *ic, char *text)
{
    /* This function needs to be called from interrupt context. Temporarily
     * disabling this as this is getting called from different contexts
     */
    return;
}

static void
ol_ath_log_text_bh(struct ieee80211com *ic, char *text)
{
    /* This function needs to be called only from bottom half context */
#ifndef REMOVE_PKT_LOG
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    if(scn->pl_dev)
        scn->pl_dev->pl_funcs->pktlog_text(scn,text);
#endif
    return;
}

static void
ol_ath_pwrsave_set_state(struct ieee80211com *ic, IEEE80211_PWRSAVE_STATE newstate)
{
    /* The host does not manage the HW power state with offload FW. This function
     * exists solely for completeness.
     */
}

#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
#ifdef WLAN_FEATURE_11BE_MLO
void
ol_peer_update_primary_info(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t *mac)
{
    struct wlan_objmgr_peer *pr_peer = NULL;
    struct wlan_objmgr_psoc *soc = (struct wlan_objmgr_psoc *)psoc;
    struct wlan_objmgr_vdev *vdev = NULL;
#ifdef QCA_SUPPORT_WDS_EXTENDED
    osif_peer_dev *osif_peer = NULL;
    struct ieee80211vap *vap = NULL;
#endif

    pr_peer = wlan_objmgr_get_peer_by_mac(soc, mac, WLAN_MLME_SB_ID);
    if (!pr_peer)
        return;

    vdev = wlan_peer_get_vdev(pr_peer);
    if (!vdev)
        goto fail;

    wlan_objmgr_mlo_update_primary_info(pr_peer);
    osif_peer_update_mlo_sta_dev(pr_peer, pr_peer->mlo_peer_ctx);

#ifdef QCA_SUPPORT_WDS_EXTENDED
    cdp_wds_ext_get_peer_osif_handle(wlan_psoc_get_dp_handle(soc),
                                     wlan_vdev_get_id(vdev), mac,
                                     (ol_osif_peer_handle *)&osif_peer);
    if (!osif_peer)
        goto fail;

    vap = wlan_vdev_get_vap(vdev);
    if (!vap)
       goto fail;

    wlan_cfg80211_wds_ext_peer_learn(vap, mac);
#endif
fail:
    wlan_objmgr_peer_release_ref(pr_peer, WLAN_MLME_SB_ID);
    return;
}
#else
void
ol_peer_update_primary_info(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t *mac)
{
}
#endif
#endif

uint8_t ol_ath_freq_to_channel(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint16_t freq)
{
    uint8_t channel_number = 0;
    struct wlan_objmgr_pdev *pdev_obj = wlan_objmgr_get_pdev_by_id(
                                          (struct wlan_objmgr_psoc *)psoc,
                                          pdev_id,
                                          WLAN_MLME_SB_ID);

    if (!pdev_obj)
        return channel_number;

    channel_number = wlan_reg_freq_to_chan(pdev_obj, freq);
    wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);

    return channel_number;
}

uint8_t ol_ath_freq_to_band(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t pdev_id, uint16_t freq)
{
    return wlan_reg_freq_to_band(freq);
}

static int16_t ol_ath_get_noisefloor (struct ieee80211com *ic, struct ieee80211_ath_channel *chan,
                                        int wait_time)
{
    /* TBD */
    return 0;
}

static int16_t ol_ath_net80211_get_cur_chan_noisefloor(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    return scn->chan_nf;
}

static int16_t ol_ath_net80211_get_cur_hw_nf(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    return scn->cur_hw_nf;
}

static void
ol_ath_get_chainnoisefloor(struct ieee80211com *ic, struct ieee80211_ath_channel *chan, int16_t *nfBuf)
{
    /* TBD */
    return;
}

#if UNIFIED_SMARTANTENNA
static void ol_ath_set_ant_switch_tbl(struct ieee80211com *ic,
                                      uint32_t ant_ctrl_common1,
                                      uint32_t ant_ctrl_common2)
{
    struct ant_switch_tbl_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.ant_ctrl_common1 = ant_ctrl_common1;
    param.ant_ctrl_common2 = ant_ctrl_common2;

    (void)wmi_unified_set_ant_switch_tbl_cmd_send(pdev_wmi_handle, &param);
}
#endif

/*
 * Override the rate power table in EEPROM
 */
static void ol_ath_set_ratepwr_table(struct ieee80211com *ic,
                                     uint8_t *ratepwr_tbl, uint16_t ratepwr_len)
{
    struct ratepwr_table_params param;
    struct wmi_unified *pdev_wmi_handle;

    if (!ratepwr_tbl)
        return;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.ratepwr_tbl = ratepwr_tbl;
    param.ratepwr_len = ratepwr_len;

    (void)wmi_unified_set_ratepwr_table_cmd_send(pdev_wmi_handle, &param);
}

/*
 * Get the rate power table in EEPROM
 */
static void ol_ath_get_ratepwr_table(struct ieee80211com *ic)
{
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    (void)wmi_unified_get_ratepwr_table_cmd_send(pdev_wmi_handle);
}

/*
 * EEPROM rate power table operations
 */
static void ol_ath_ratepwr_table_ops(struct ieee80211com *ic, uint8_t ops,
                                     uint8_t *ratepwr_tbl, uint16_t ratepwr_len)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    if (lmac_get_tgt_type(psoc) != TARGET_TYPE_AR9888) {
        qdf_info("rate power table override is only supported for AR98XX");
        return;
    }

    if (ops == WMI_HOST_RATEPWR_TABLE_OPS_SET) {
        QC98XX_EEPROM_RATETBL eep_tbl;

        qdf_mem_set((void *)&eep_tbl, sizeof(QC98XX_EEPROM_RATETBL), 0);
        /* convert user format to eeprom format */
        ol_if_ratepwr_usr_to_eeprom((uint8_t *)&eep_tbl,
                                    sizeof(QC98XX_EEPROM_RATETBL),
                                    ratepwr_tbl, ratepwr_len);

        ol_ath_set_ratepwr_table(ic, (uint8_t*)&eep_tbl,
                                 sizeof(QC98XX_EEPROM_RATETBL));
    } else if (ops == WMI_HOST_RATEPWR_TABLE_OPS_GET) {
        ol_ath_get_ratepwr_table(ic);
    } else {
        qdf_err("Unknown ratepwr table ops %d", ops);
    }
    return;
}

static void ol_ath_set_ctl_table(struct ieee80211com *ic,
                                 uint8_t *ctl_array, uint16_t ctl_len)
{
    struct ctl_table_params param;
    struct wmi_unified *pdev_wmi_handle;
    struct wlan_objmgr_psoc *psoc = NULL;
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    if (!ctl_array)
        return;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);

    qdf_mem_copy(&param.ctl_band, ctl_array, sizeof(uint32_t));
    param.pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    param.ctl_array = ctl_array + sizeof(uint32_t);
    param.ctl_cmd_len = ctl_len;
    param.target_type = lmac_get_tgt_type(psoc);

    if (IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan))
        param.is_2g = TRUE;
    else
        param.is_2g = FALSE;

    (void)wmi_unified_set_ctl_table_cmd_send(pdev_wmi_handle, &param);
}

static void ol_ath_send_tdma_schedule(struct ieee80211com *ic,
                                      struct tdma_sched_info *tdma_config)
{
    int id = tdma_config->sched_id;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_tdma_sched_cmd_param params;
    struct wmi_unified *pdev_wmi_handle;

    if ((id < 0) && (id >= IEEE80211_MAX_TDMA_SCHEDULES)) {
        qdf_err("Schedule id is not in range\n");
        return;
    }

    pdev = ic->ic_pdev_obj;
    if (!pdev)
        return;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_zero(&params, sizeof(struct wlan_tdma_sched_cmd_param));

    params.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
    params.schedule_type = ic->tdma_info[id].sched_type;
    params.schedule_handle_id = ic->tdma_info[id].sched_id ;
    qdf_mem_copy(params.owner_bssid, ic->tdma_info[id].bssid, QDF_MAC_ADDR_SIZE);
    params.start_time_tsf_low = ic->tdma_info[id].start_time_tsf_low;
    params.start_time_tsf_high = ic->tdma_info[id].start_time_tsf_high;
    params.num_busy_slots = ic->tdma_info[id].num_busy_slots;
    params.busy_slot_dur_ms = ic->tdma_info[id].busy_slot_dur_ms;
    params.busy_slot_intvl_ms = ic->tdma_info[id].busy_slot_intvl_ms;
    params.edca_params_valid = ic->tdma_info[id].edca_params_valid;
    qdf_mem_copy(params.aifsn, ic->tdma_info[id].aifsn,
                 WLAN_MAX_AC * sizeof(uint16_t));
    qdf_mem_copy(params.cwmin, ic->tdma_info[id].cwmin,
                 WLAN_MAX_AC * sizeof(uint16_t));
    qdf_mem_copy(params.cwmax, ic->tdma_info[id].cwmax,
                 WLAN_MAX_AC * sizeof(uint16_t));

    wmi_unified_tdma_schedule_send(pdev_wmi_handle, &params);
}

static void
ol_ath_set_mimogain_table(struct ieee80211com *ic, uint8_t *array_gain,
                          uint16_t tbl_len, bool multichain_gain_bypass)
{
    struct mimogain_table_params param;
    struct wmi_unified *pdev_wmi_handle;

    if (!array_gain)
        return;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.array_gain = array_gain;
    param.tbl_len = tbl_len;
    param.multichain_gain_bypass = multichain_gain_bypass;

    (void)wmi_unified_set_mimogain_table_cmd_send(pdev_wmi_handle, &param);
}

#ifdef WLAN_SUPPORT_MESH_LATENCY
static QDF_STATUS
ol_ath_set_peer_latency_param_config(struct ieee80211com *ic, uint8_t *peer_mac,
       uint32_t tid, uint8_t dl_ul_enable, uint32_t service_interval,
       uint32_t burst_size, uint32_t add_or_sub)
{
    struct wmi_peer_latency_config_params param = {0};
    struct wmi_unified *pdev_wmi_handle;
    struct wlan_objmgr_pdev *pdev;
    uint8_t ac = WMI_HOST_AC_BE;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn)
        return QDF_STATUS_E_INVAL;

    pdev = scn->sc_pdev;
    if (!pdev)
        return QDF_STATUS_E_INVAL;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return QDF_STATUS_E_INVAL;

    param.pdev_id = lmac_get_pdev_idx(pdev);
    param.num_peer = 1;
    qdf_mem_copy(param.latency_info[0].peer_mac, peer_mac,
                      QDF_MAC_ADDR_SIZE);
    param.latency_info[0].latency_tid = tid;

    param.latency_info[0].dl_enable = 1;
    param.latency_info[0].ul_enable = 1;

    param.latency_info[0].service_interval = service_interval;
    param.latency_info[0].burst_size = burst_size;
    param.latency_info[0].add_or_sub = add_or_sub;
    ac = TID_TO_WME_AC(tid);
    param.latency_info[0].ac = ac;
    param.latency_info[0].sawf_ul_param = 0;

    return wmi_unified_config_peer_latency_info_cmd_send(pdev_wmi_handle, &param);
}
#endif

#ifdef QCA_STANDALONE_SOUNDING_TRIGGER
static QDF_STATUS ol_ath_trigger_txbf_sounding(struct ieee80211com *ic, struct ieee80211req_athdbg *req)
{
    struct wmi_txbf_sounding_trig_param txbf_sounding_params;
    struct wmi_unified *pdev_wmi_handle;
    struct wlan_objmgr_pdev *pdev;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    uint8_t i = 0;
    QDF_STATUS status;

    if (!scn) {
        qdf_err("scn is NULL!");
        return QDF_STATUS_E_INVAL;
    }

    pdev = scn->sc_pdev;
    if (!pdev) {
        qdf_err("pdev is NULL!");
        return QDF_STATUS_E_INVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is NULL!");
        return QDF_STATUS_E_INVAL;
    }

    qdf_mem_set(&txbf_sounding_params, sizeof(txbf_sounding_params), 0);

    txbf_sounding_params.feedback_type = req->data.txbf_sounding_trig_info.feedback_type;
    txbf_sounding_params.ng            = req->data.txbf_sounding_trig_info.ng;
    txbf_sounding_params.codebook      = req->data.txbf_sounding_trig_info.codebook;
    txbf_sounding_params.bw            = req->data.txbf_sounding_trig_info.bw;
    txbf_sounding_params.sounding_repeats = req->data.txbf_sounding_trig_info.sounding_repeats;
    txbf_sounding_params.num_sounding_peers = req->data.txbf_sounding_trig_info.num_sounding_peers;
    txbf_sounding_params.vdev_id            = req->data.txbf_sounding_trig_info.vdev_id;
    txbf_sounding_params.cqi_triggered_type = req->data.txbf_sounding_trig_info.cqi_triggered_type;

    QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG," [%s:%d] Vdev id:%d"
              "feedback_type:%d ng:%d Cb:%d Bw:%d cqi_triggered_type:%d "
              "sounding_repeats:%d num_sounding_peers:%d",__func__,__LINE__,
              txbf_sounding_params.vdev_id,
              txbf_sounding_params.feedback_type,
              txbf_sounding_params.ng,
              txbf_sounding_params.codebook,
              txbf_sounding_params.bw,
              txbf_sounding_params.cqi_triggered_type,
              txbf_sounding_params.sounding_repeats,
              txbf_sounding_params.num_sounding_peers);

    for (i = 0; i < txbf_sounding_params.num_sounding_peers; i++) {
         qdf_mem_copy(&txbf_sounding_params.macaddr[i][0],
                      &req->data.txbf_sounding_trig_info.macaddr[i][0],
                      IEEE80211_ADDR_LEN);
         QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,"[%s:%d] peer mac:%s",
                   __func__,__LINE__,ether_sprintf(txbf_sounding_params.macaddr[i]));
    }

    status = wmi_unified_txbf_sounding_trig_info_cmd_send(pdev_wmi_handle, &txbf_sounding_params);

    /*
     * Setting this flag to avoid  user to give sounding cmd to peer
     * if response is pending for an already ongoing cmd
     */
    if (status== QDF_STATUS_SUCCESS) {
        scn->sc_sounding_inprogress = true;
        scn->sc_txbf_sounding_last_cmd_time = qdf_system_ticks();
    }

    return status;
}
#endif

#if QCA_MANUAL_TRIGGERED_ULOFDMA
static QDF_STATUS
ol_ath_trigger_ul_ofdma_for_peer(struct ieee80211vap *vap,
                                 struct ul_ofdma_trig_info *data)
{
    struct wmi_trigger_ul_ofdma_su_params su_param = {0};
    struct wmi_trigger_ul_ofdma_mu_params mu_param = {0};
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211com *ic;
    struct wlan_objmgr_vdev *vdev;
    struct ol_ath_softc_net80211 *scn;
    int i = 0;
    QDF_STATUS status;

    ic = vap->iv_ic;
    if (!ic)
        return QDF_STATUS_E_INVAL;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn)
        return QDF_STATUS_E_INVAL;

    vdev = vap->vdev_obj;
    if (!vdev)
        return QDF_STATUS_E_INVAL;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return QDF_STATUS_E_INVAL;

    if (data->ul_ofdma_trig_num_peers == 1) {
        su_param.vdev_id = wlan_vdev_get_id(vdev);
        su_param.num_trigger = data->ul_ofdma_num_triggers;
        su_param.length = data->ul_ofdma_trig_length;
        su_param.preferred_ac = data->ul_ofdma_trig_preferred_ac;
        su_param.mcs = data->ul_ofdma_trig_mcs;
        su_param.nss = data->ul_ofdma_trig_nss;
        su_param.rssi = data->ul_ofdma_trig_target_rssi;

        qdf_mem_copy(su_param.macaddr, data->macaddr[0], QDF_MAC_ADDR_SIZE);

        status = wmi_unified_config_trigger_ulofdma_su_cmd_send(pdev_wmi_handle,
                                                                &su_param);
    }
    else {
        mu_param.vdev_id = wlan_vdev_get_id(vdev);
        mu_param.num_peer = data->ul_ofdma_trig_num_peers;
        mu_param.preferred_ac = data->ul_ofdma_trig_preferred_ac;
        for (i = 0; i < data->ul_ofdma_trig_num_peers; ++i)
        {
            qdf_mem_copy(mu_param.macaddr[i], data->macaddr[i],
                         QDF_MAC_ADDR_SIZE);
        }

        status = wmi_unified_config_trigger_ulofdma_mu_cmd_send(pdev_wmi_handle,
                                                                &mu_param);
    }
    return status;
}
#endif

static u_int8_t
ol_ath_get_common_power(struct ieee80211com *ic, struct ieee80211_ath_channel *chan)
{
    /* TBD */
    return 0;
}

static u_int32_t
ol_ath_getTSF32(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static int
ol_ath_getrmcounters(struct ieee80211com *ic, struct ieee80211_mib_cycle_cnts *pCnts)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    /* "ic->ic_mib_cycle_cnts" gets updated during a periodic stats event */
    pCnts->tx_frame_count = scn->mib_cycle_cnts.tx_frame_count;
    pCnts->rx_frame_count = scn->mib_cycle_cnts.rx_frame_count;
    pCnts->rx_clear_count = scn->mib_cycle_cnts.rx_clear_count;
    pCnts->rx_clear_ext_count = scn->mib_cycle_cnts.rx_clear_ext_count;
    pCnts->rx_clear_ext40_count = scn->mib_cycle_cnts.rx_clear_ext40_count;
    pCnts->rx_clear_ext80_count = scn->mib_cycle_cnts.rx_clear_ext80_count;
    pCnts->cycle_count = scn->mib_cycle_cnts.cycle_count;

    /* "is_rx_active" and "is_tx_active" not being used, but for safety, set it to 0 */
    pCnts->is_rx_active = 0;
    pCnts->is_tx_active = 0;

    return 0;
}

static u_int32_t
ol_ath_wpsPushButton(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

static void
ol_ath_clear_phystats(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

static int ol_ath_set_macaddr(struct ieee80211com *ic, uint8_t *macaddr)
{
    struct macaddr_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return -EINVAL;

    qdf_mem_set(&param, sizeof(param), 0);
    param.macaddr = macaddr;

    if (wmi_unified_set_macaddr_cmd_send(pdev_wmi_handle, &param)) {
        return -1;
    }
    return 0;
}

static int
ol_ath_set_chain_mask(struct ieee80211com *ic, ieee80211_device_param type, u_int32_t mask)
{
    /* TBD */
    return 0;
}

static u_int32_t
ol_ath_getmfpsupport(struct ieee80211com *ic)
{
    return IEEE80211_MFP_HW_CRYPTO;
}

static uint64_t ol_ath_get_tx_hw_retries(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_txrx_soc_handle soc_txrx_handle;
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    return cdp_ath_get_total_per(soc_txrx_handle,
                                 wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj));
}

static u_int64_t
ol_ath_get_tx_hw_success(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

/**
 * ol_ath_rate_node_update() - Update peer rate table
 * @ni: pointer to node
 * @isnew: peer association type
 *
 * Return: none
 */
static void ol_ath_rate_node_update(struct ieee80211_node *ni, int isnew)
{
    /* reuse the ASSOC_CMDID to perform the dynamic rate setting */
    ol_ath_send_peer_assoc(ni, isnew);
}

#ifdef BIG_ENDIAN_HOST
void swap_bytes(void *pv, size_t n)
{
       int noWords;
       int i;
       A_UINT32 *wordPtr;

       noWords =   n/sizeof(u_int32_t);
       wordPtr = (u_int32_t *)pv;
       for (i=0;i<noWords;i++)
       {
               *(wordPtr + i) = __cpu_to_le32(*(wordPtr + i));
       }
}
#define SWAPME(x, len) swap_bytes(&x, len);
#endif

#ifdef WLAN_SUPPORT_RF_CHARACTERIZATION
static int
ol_ath_chan_rf_characterization_info_event_handler(ol_soc_t  sc,
                                                   u_int8_t  *data,
                                                   u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_unified *wmi_handle = NULL;
    uint32_t num_rf_characterization_entries = 0;

    if (!soc) {
        qdf_err("Invalid soc");
        return -EINVAL;
    }

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("Invalid WMI handle");
        return -EINVAL;
    }

    /*
     * Retrieving the total number of TLVs (if any). If there are no TLVs
     * then we return without disrupting functionality.
     */
    wmi_extract_num_rf_characterization_entries(wmi_handle, data,
                                        &num_rf_characterization_entries);
    if (!num_rf_characterization_entries) {
        return 0;
    }

    /*
     * If there are already metrics present before handling this event,
     * they will be discarded and replaced by the metrics sent in this.
     */
    if (soc->rf_characterization_entries) {
        qdf_mem_free(soc->rf_characterization_entries);
        soc->num_rf_characterization_entries = 0;
        soc->rf_characterization_entries = NULL;
    }

    soc->num_rf_characterization_entries = num_rf_characterization_entries;
    soc->rf_characterization_entries = qdf_mem_malloc(
                                    sizeof(*soc->rf_characterization_entries) *
                                    soc->num_rf_characterization_entries);
    if (!soc->rf_characterization_entries) {
        soc->num_rf_characterization_entries = 0;
        return -EINVAL;
    }

    if (wmi_extract_rf_characterization_entries(wmi_handle, data,
                                         soc->num_rf_characterization_entries,
                                         soc->rf_characterization_entries)) {
        qdf_err("Failed to extract RF characterization metrics");
        qdf_mem_free(soc->rf_characterization_entries);
        soc->rf_characterization_entries = NULL;
        soc->num_rf_characterization_entries = 0;
        return -EINVAL;
    }

    return 0;
}
#endif

static int
ol_ath_debug_print_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
#ifdef BIG_ENDIAN_HOST
       char dbgbuf[500] = {0};
       memcpy(dbgbuf, data, datalen);
       SWAPME(dbgbuf, datalen);
       qdf_info("FIRMWARE:%s", dbgbuf);
       return 0;
#else
       qdf_info("FIRMWARE:%s", data);
       return 0;
#endif
}

static bool
ol_ath_set_config_enable(struct ieee80211vap* vap)
{
       struct wlan_objmgr_psoc *psoc;

       psoc = wlan_vdev_get_psoc(vap->vdev_obj);
       return ol_target_lithium(psoc);
}

static void
ol_ath_set_config(struct ieee80211vap* vap)
{
      /* Currently Not used for Offload */
}

/**
 * ol_ath_set_peer_tid_noack_config(): function to set noack policy based on tid
 *                                    map in the peer
 * @pdev: pdev handle
 * @vdev_id: id of vdev handle
 * @macaddr: peer mac address
 * @noack_map: Tid map based on which noack policy will be set
 * Return: 0 for success or error code
 */
static int
ol_ath_set_peer_tid_noack_config(struct wlan_objmgr_pdev *pdev,
				 uint32_t vdev_id,
				 uint8_t *macaddr,
				 uint16_t noack_map)
{
	struct peer_tid_config_params params = {0};
	wmi_unified_t wmi_handle;
	QDF_STATUS status;
	uint32_t tid_num = 0;

	wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!wmi_handle) {
		qdf_err("wmi handle NULL");
		return -EINVAL;
	}

	for (tid_num = 0; tid_num < CDP_MAX_DATA_TIDS - 1; tid_num++) {
		params.vdev_id = vdev_id;
		params.tid_num = tid_num;

		if ((noack_map) & (BIT(tid_num)))
			params.ack_policy = PEER_TID_CONFIG_NOACK;
		else
			/*Reset noack policy for TID*/
			params.ack_policy = PEER_TID_CONFIG_ACK;

		status = wmi_unified_peer_tid_config_send(wmi_handle, macaddr,
							  &params);
		if (QDF_IS_STATUS_ERROR(status)) {
			qdf_err("failed to apply noack policy for vdev_id : %d & tid_num : %d", vdev_id, tid_num);
			return qdf_status_to_os_return(status);
		}
	}

	return 0;
}

static void
ol_ath_set_safemode(struct ieee80211vap* vap, int val)
{
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_vdev *vdev_obj = vap->vdev_obj;
    cdp_config_param_type value = {0};

    if (vdev_obj) {
        psoc = wlan_vdev_get_psoc(vdev_obj);
        value.cdp_vdev_param_safe_mode = val;
        cdp_txrx_set_vdev_param(wlan_psoc_get_dp_handle(psoc),
                         wlan_vdev_get_id(vdev_obj), CDP_SAFEMODE, value);
    }
    return;
}

static void
ol_ath_set_privacy_filters(struct ieee80211vap* vap)
{
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_vdev *vdev_obj = vap->vdev_obj;

    if (vdev_obj) {
        psoc = wlan_vdev_get_psoc(vdev_obj);
        cdp_set_privacy_filters(wlan_psoc_get_dp_handle(psoc),
                wlan_vdev_get_id(vdev_obj), vap->iv_privacy_filters, vap->iv_num_privacy_filters);
    }
    return;
}

#if ATH_SUPPORT_IQUE
void ol_ath_set_acparams(struct ieee80211com *ic, uint8_t ac, uint8_t use_rts,
                         uint8_t aggrsize_scaling, uint32_t min_kbps)
{
    struct acparams_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.ac = ac;
    param.aggrsize_scaling = aggrsize_scaling;

    wmi_unified_set_acparams_cmd_send(pdev_wmi_handle, &param);
    return;
}

void
ol_ath_set_rtparams(struct ieee80211com *ic, u_int8_t ac, u_int8_t perThresh,
                          u_int8_t probeInterval)
{
    /* TBD */
    return;
}

void
ol_ath_get_iqueconfig(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

void
ol_ath_set_hbrparams(struct ieee80211vap *iv, u_int8_t ac, u_int8_t enable, u_int8_t per)
{
    /* TBD */
    return;
}
#endif /*ATH_SUPPORT_IQUE*/

#ifdef ATH_SUPPORT_TxBF
static int
ol_ath_net80211_txbf_alloc_key(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return 0;
}

static void
ol_ath_net80211_txbf_set_key(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return;
}

static int
ol_ath_net80211_txbf_del_key(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return 0;
}

static void
ol_ath_net80211_init_sw_cv_timeout(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return;
}

static int
ol_ath_set_txbfcapability(struct ieee80211com *ic)
{
    return 0;
}

#ifdef TXBF_DEBUG
static void
ol_ath_net80211_txbf_check_cvcache(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    return 0;
}
#endif

static void
ol_ath_net80211_v_cv_send(struct ieee80211_node *ni,
                       u_int8_t *data_buf,
                       u_int16_t buf_len)
{
    return;
}
static void
ol_ath_net80211_txbf_stats_rpt_inc(struct ieee80211com *ic,
                                struct ieee80211_node *ni)
{
    return;
}
static void
ol_ath_net80211_txbf_set_rpt_received(struct ieee80211com *ic,
                                struct ieee80211_node *ni)
{
    return;
}
#endif

QDF_STATUS
ol_is_mode_offload(struct wlan_objmgr_pdev *pdev, bool *is_offload)
{
    *is_offload = TRUE;
    return QDF_STATUS_SUCCESS;
}

bool
ol_ath_net80211_is_mode_offload(struct ieee80211com *ic)
{
    /*
     * If this function executes, it is offload mode
     */
    return TRUE;
}

QDF_STATUS ol_ath_get_ah_devid(struct wlan_objmgr_pdev *pdev,
        uint16_t *devid)
{
    /* devid is needed only for DA */
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_get_phymode_info(struct wlan_objmgr_pdev *pdev,
        uint32_t chan_mode, uint32_t *mode_info, bool is_2gvht_en)
{
    struct ol_ath_softc_net80211 *scn;
    struct pdev_osif_priv *osif_priv;

    osif_priv = wlan_pdev_get_ospriv(pdev);

    if (!osif_priv) {
        qdf_err("osif_priv is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    scn = (struct ol_ath_softc_net80211 *)osif_priv->legacy_osif_priv;

    *mode_info = ol_get_phymode_info_from_wlan_phymode(scn, chan_mode, is_2gvht_en);
    return QDF_STATUS_SUCCESS;
}

bool ol_ath_get_persistent_nol_info(struct wlan_objmgr_pdev *pdev)
{
    struct wlan_objmgr_psoc *psoc;

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        qdf_err("psoc is NULL");
        return false;
    }

    return cfg_get(psoc, CFG_OL_RETAIN_NOL_ACROSS_DRIVER_RELOAD);
}

static bool
ol_ath_net80211_is_macreq_enabled(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    return ((scn->macreq_enabled == 1) ? TRUE : FALSE);
}

static bool
ol_ath_net80211_is_macreq_ext_active(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    return ((scn->macreq_enabled && scn->macreq_ext_enabled) ? TRUE : FALSE);
}

static u_int32_t
ol_ath_net80211_get_mac_prealloc_idmask(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    return scn->sc_prealloc_idmask;
}

static void
ol_ath_net80211_set_mac_prealloc_id(struct ieee80211com *ic, u_int8_t id,
                                                            u_int8_t set)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    if(set) {
        scn->sc_prealloc_idmask |= (1 << id);
    }
    else {
        scn->sc_prealloc_idmask &= ~(1 << id);
    }
}

static uint8_t
ol_ath_net80211_get_bsta_fixed_idmask(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    return scn->sc_bsta_fixed_idmask;
}

static int
ol_ath_net80211_tr69_get_vdev_extd_stats(struct ieee80211vap *vap,
                                         uint32_t cmd,
                                         uint32_t *arg1)
{
    struct wlan_objmgr_psoc *psoc;
    ol_txrx_soc_handle soc_handle;
    wmi_host_vdev_extd_stats extd_stats;

    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);

    if (!psoc)
        return -ENOENT;

    soc_handle = wlan_psoc_get_dp_handle(psoc);

    cdp_get_vdev_extd_stats(soc_handle,
                            wlan_vdev_get_id(vap->vdev_obj),
                            &extd_stats);

    switch(cmd) {
        case IEEE80211_TR069_GET_FAIL_RETRANS_CNT:
             *arg1 = extd_stats.mpdu_fail_retry;
        break;
        case IEEE80211_TR069_GET_RETRY_CNT:
             *arg1 =  extd_stats.mpdu_suc_retry;
        break;
        case IEEE80211_TR069_GET_MUL_RETRY_CNT:
             *arg1 = extd_stats.mpdu_suc_multitry;
        break;
        case IEEE80211_TR069_GET_ACK_FAIL_CNT:
             *arg1 = extd_stats.ppdu_noack;
        break;
        case IEEE80211_TR069_GET_AGGR_PKT_CNT:
             *arg1 = extd_stats.ppdu_aggr_cnt;
        break;
        case IEEE80211_TR069_GET_CHAN_UTIL:
             *arg1 = 0;
        break;
        case IEEE80211_TR069_GET_RETRANS_CNT:
             *arg1 = extd_stats.mpdu_sw_requed;
        break;
    }

    return 0;
}

static int
ol_ath_net80211_tr69_get_sta_bytes_sent(struct ieee80211vap *vap, u_int32_t *bytessent, u_int8_t *dstmac)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni = NULL;
    struct wlan_objmgr_psoc *psoc;
    cdp_peer_stats_param_t buf = {0};
    QDF_STATUS status;

    ni = ieee80211_find_node(ic, dstmac, WLAN_TR69_ID);
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!ni || !ni->peer_obj || !psoc) {
        status = QDF_STATUS_E_NOENT;
    } else {
        status = cdp_txrx_get_peer_stats_param(wlan_psoc_get_dp_handle(psoc),
                                 wlan_vdev_get_id(ni->peer_obj->peer_objmgr.vdev),
                                 ni->peer_obj->macaddr, cdp_peer_tx_ucast,
                                 &buf);
        *bytessent = buf.tx_ucast.bytes;
    }

    if (ni)
        ieee80211_free_node(ni, WLAN_TR69_ID);

    return qdf_status_to_os_return(status);
}

static int
ol_ath_net80211_tr69_get_sta_bytes_rcvd(struct ieee80211vap *vap, u_int32_t *bytesrcvd, u_int8_t *dstmac)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni = NULL;
    struct wlan_objmgr_psoc *psoc;
    cdp_peer_stats_param_t buf  = {0};
    QDF_STATUS status;

    ni = ieee80211_find_node(ic, dstmac, WLAN_TR69_ID);
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!ni || !ni->peer_obj || !psoc) {
        status = QDF_STATUS_E_NOENT;
    } else {
        status = cdp_txrx_get_peer_stats_param(wlan_psoc_get_dp_handle(psoc),
                                 wlan_vdev_get_id(ni->peer_obj->peer_objmgr.vdev),
                                 ni->peer_obj->macaddr, cdp_peer_rx_ucast,
                                 &buf);
        *bytesrcvd = buf.rx_ucast.bytes;
    }

    if (ni)
        ieee80211_free_node(ni, WLAN_TR69_ID);

    return qdf_status_to_os_return(status);
}

static int
ol_ath_net80211_tr69_process_request(struct ieee80211vap *vap, int cmdid, void * arg1, void *arg2)
{
    //	struct ieee80211com *ic = vap->iv_ic;

	switch(cmdid){
#if 0
        case IEEE80211_TR069_GET_PLCP_ERR_CNT:
			ol_ath_net80211_tr69_get_plcp_err_cnt(dev, arg1);
            break;
        case IEEE80211_TR069_GET_FCS_ERR_CNT:
			ol_ath_net80211_tr69_get_fcs_err_cnt(dev, arg1);
            break;
        case IEEE80211_TR069_GET_PKTS_OTHER_RCVD:
			ol_ath_net80211_tr69_get_pkts_other_rcvd(dev, req);
            break;
#endif
        case IEEE80211_TR069_GET_FAIL_RETRANS_CNT:
        case IEEE80211_TR069_GET_RETRY_CNT:
        case IEEE80211_TR069_GET_MUL_RETRY_CNT:
        case IEEE80211_TR069_GET_ACK_FAIL_CNT:
        case IEEE80211_TR069_GET_AGGR_PKT_CNT:
        case IEEE80211_TR069_GET_CHAN_UTIL:
        case IEEE80211_TR069_GET_RETRANS_CNT:
             ol_ath_net80211_tr69_get_vdev_extd_stats(vap, cmdid, arg1);
        break;
        case IEEE80211_TR069_GET_STA_BYTES_SENT:
             ol_ath_net80211_tr69_get_sta_bytes_sent(vap, arg1, arg2);
            break;
        case IEEE80211_TR069_GET_STA_BYTES_RCVD:
             ol_ath_net80211_tr69_get_sta_bytes_rcvd(vap, arg1, arg2);
            break;
#if 0
        case IEEE80211_TR069_GET_DATA_SENT_ACK:
			ol_ath_net80211_tr69_get_data_sent_ack(dev, arg1);
            break;
        case IEEE80211_TR069_GET_DATA_SENT_NOACK:
			ol_ath_net80211_tr69_get_data_sent_noack(dev, req);
            break;
#endif
        default:
			break;
    }

    return 0;
}

/*default rate limit period - 60 sec*/
#define PEER_REF_PRINT_RATE_LIMIT_PERIOD (60*HZ)
/*default burst for rate limit */
#define PEER_REF_PRINT_RATE_LIMIT_BURST_DEFAULT   1
DEFINE_RATELIMIT_STATE(peer_refs_ratelimit,
        PEER_REF_PRINT_RATE_LIMIT_PERIOD,
        PEER_REF_PRINT_RATE_LIMIT_BURST_DEFAULT);

int print_peer_refs_ratelimit(void)
{
    return __ratelimit(&peer_refs_ratelimit);
}

bool ol_ath_wide_band_scan(struct ieee80211com *ic)
{
    bool ret_val;
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    /* return true if host and FW both support wide band scan.
     * false otherwise.
     */
    ret_val =(ic->ic_widebw_scan &&
              wlan_psoc_nif_fw_ext_cap_get(psoc,
                                           WLAN_SOC_CEXT_WIDEBAND_SCAN));

    return ret_val;
}

int
ol_ath_net80211_get_vap_stats(struct ieee80211vap *vap)
{
    struct wlan_objmgr_psoc *psoc;
    ol_txrx_soc_handle soc_handle;
    wmi_host_vdev_extd_stats vdev_extd_stats;

    psoc = wlan_pdev_get_psoc(vap->iv_ic->ic_pdev_obj);

    if(!psoc)
        return -ENOENT;

    soc_handle = wlan_psoc_get_dp_handle(psoc);
    cdp_get_vdev_extd_stats(soc_handle,
                            wlan_vdev_get_id(vap->vdev_obj),
                            &vdev_extd_stats);
    qdf_info("-----VAP Stats------\n");
    qdf_info("\n");
    qdf_info("ppdu_aggr         = %d\n", vdev_extd_stats.ppdu_aggr_cnt);
    qdf_info("ppdu_nonaggr      = %d\n", vdev_extd_stats.ppdu_nonaggr_cnt);
    qdf_info("noack             = %d\n", vdev_extd_stats.ppdu_noack);
    qdf_info("mpdu_queued       = %d\n", vdev_extd_stats.mpdu_queued);
    qdf_info("mpdu_requeued     = %d\n", vdev_extd_stats.mpdu_sw_requed);
    qdf_info("mpdu_suc_retry    = %d\n", vdev_extd_stats.mpdu_suc_retry);
    qdf_info("mpdu_suc_multitry = %d\n", vdev_extd_stats.mpdu_suc_multitry);
    qdf_info("mpdu_fail_retry   = %d\n", vdev_extd_stats.mpdu_fail_retry);
    qdf_info("-----VAP Stats------\n");
    qdf_info("\n");

    return 0;
}

/* The below mapping is according the doc, which is as follows,

DSCP        TID     AC
000000      0       WME_AC_BE
001000      1       WME_AC_BK
010000      1       WME_AC_BK
011000      0       WME_AC_BE
100000      5       WME_AC_VI
101000      5       WME_AC_VI
110000      6       WME_AC_VO
111000      6       WME_AC_VO

*/

#if ATH_SUPPORT_DSCP_OVERRIDE
void
ol_ath_set_pdev_dscp_tid_map(struct ieee80211vap *vap, uint32_t tos)
{

    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint8_t map_id = vap->iv_dscp_map_id;
    uint8_t tid = vap->iv_ic->ic_dscp_tid_map[map_id][(tos >> IP_DSCP_SHIFT) & IP_DSCP_MASK];
    ol_txrx_soc_handle soc_txrx_handle;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);

    psoc = wlan_pdev_get_psoc(pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    cdp_set_pdev_dscp_tid_map(soc_txrx_handle,
                              wlan_objmgr_pdev_get_pdev_id(pdev), map_id,
                              tos, tid);
}
void
ol_ath_set_hmmc_tid(struct ieee80211com *ic , u_int32_t tid)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_txrx_soc_handle soc_txrx_handle;
    cdp_config_param_type value = {0};

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle) {
        QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
                "%s : %d : soc_txrx_handle is NULL", __func__, __LINE__);
        return;
    }

    /* If Override is disabled: send default TID  else passed the intended tid value */
    if(!ic->ic_override_hmmc_dscp) {
        tid = 0xFF;                 //TBD::Replace it with proper macro
    }

    value.cdp_pdev_param_hmmc_tid = tid;
    cdp_txrx_set_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev), CDP_CONFIG_HMMC_TID_VALUE, value);

    ic->ic_dscp_hmmc_tid = tid;

    /* Send a message to NSS */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_radio_ops && ic->nss_radio_ops->ic_nss_ol_set_hmmc_dscp_tid) {
        ic->nss_radio_ops->ic_nss_ol_set_hmmc_dscp_tid(scn, tid);
    }
#endif
    return;
}

u_int32_t
ol_ath_get_hmmc_tid(struct ieee80211com *ic)
{
    return ic->ic_dscp_hmmc_tid;
}

void
ol_ath_set_hmmc_dscp_override(struct ieee80211com *ic , u_int32_t val)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_txrx_soc_handle soc_txrx_handle;
    cdp_config_param_type value = {0};

    /* If nothing to do just return */
    if(ic->ic_override_hmmc_dscp == val) {
        return;
    }
    ic->ic_override_hmmc_dscp = !!val;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle) {
        QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
                "%s : %d : soc_txrx_handle is NULL", __func__, __LINE__);
        return;
    }

    value.cdp_pdev_param_hmmc_tid_ovrd = !!val;
    cdp_txrx_set_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                            CDP_CONFIG_HMMC_TID_OVERRIDE, value);

    /* Send a message to NSS */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_radio_ops && ic->nss_radio_ops->ic_nss_ol_set_hmmc_dscp_override) {
        ic->nss_radio_ops->ic_nss_ol_set_hmmc_dscp_override(scn, val);
    }
#endif
    return;
}

u_int32_t
ol_ath_get_hmmc_dscp_override(struct ieee80211com *ic)
{
    return ic->ic_override_hmmc_dscp;
}

#endif

void ol_wlan_txpow_mgmt(struct ieee80211vap *vap, uint8_t subtype)
{
    struct wlan_objmgr_psoc *psoc;
    int retval = EOK;

    if (subtype == IEEE80211_FC0_SUBTYPE_BEACON) {
        retval = ol_ath_set_vap_beacon_tx_power(vap->vdev_obj,
            vap->iv_txpow_mgt_frm[(subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)]);
    }
    if (retval == EOK){
        psoc = wlan_vdev_get_psoc(vap->vdev_obj);
        cdp_set_mgmt_tx_power(wlan_psoc_get_dp_handle(psoc),
            wlan_vdev_get_id(vap->vdev_obj), subtype,
            vap->iv_txpow_mgt_frm[(subtype >> IEEE80211_FC0_SUBTYPE_SHIFT)]);
    }
return;
}

#if ATH_SUPPORT_WRAP && DBDC_REPEATER_SUPPORT
void ol_ioctl_disassoc_clients(struct ol_ath_softc_net80211 *scn)
{
	struct ieee80211com *ic = &scn->sc_ic;
	struct ieee80211vap           *tmp_vap;
	struct ieee80211com           *tmp_ic;
	int i=0;

	for (i=0; i < MAX_RADIO_CNT; i++) {
		tmp_ic = ic->ic_global_list->global_ic[i];
		if (tmp_ic) {
			TAILQ_FOREACH(tmp_vap, &tmp_ic->ic_vaps, iv_next) {
				if ((tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) &&
				    (wlan_vdev_is_up(tmp_vap->vdev_obj) == QDF_STATUS_SUCCESS)) {
					wlan_iterate_station_list(tmp_vap, sta_disassoc, NULL);
				}
			}
		}
	}
}
#endif


#if  ATH_SUPPORT_WRAP && DBDC_REPEATER_SUPPORT
int32_t ol_ioctl_get_primary_radio(struct ol_ath_softc_net80211 *scn, caddr_t param)
{
    int primary_radio;
    struct ieee80211com *ic = &scn->sc_ic;
    struct qwrap_config_t *qwrap_config = (struct qwrap_config_t *)param;

    if(ic->ic_primary_radio) {
        primary_radio = 1;
    } else {
        primary_radio = 0;
    }
    qwrap_config->primary_radio = primary_radio;
    return 0;
}

int32_t ol_ioctl_get_mpsta_mac_addr(struct ol_ath_softc_net80211 *scn, caddr_t param)
{
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211vap *mpsta_vap = NULL;
    struct qwrap_config_t *qwrap_config = (struct qwrap_config_t *)param;

    mpsta_vap = wlan_get_vap(dp_wrap_get_mpsta_vdev(ic->ic_pdev_obj));
    if (mpsta_vap == NULL)
    {
        return -EFAULT;
    }
    qdf_mem_copy(qwrap_config->addr, mpsta_vap->iv_myaddr, QDF_MAC_ADDR_SIZE);
    return 0;
}

int32_t ol_ioctl_get_force_client_mcast(struct ol_ath_softc_net80211 *scn, caddr_t param)
{
    int force_client_mcast;
    struct ieee80211com *ic = &scn->sc_ic;
    struct qwrap_config_t *qwrap_config = (struct qwrap_config_t *)param;

    if(ic->ic_global_list->force_client_mcast_traffic) {
        force_client_mcast = 1;
    } else {
        force_client_mcast = 0;
    }

    qwrap_config->force_client_mcast = force_client_mcast;
    return 0;
}

int32_t ol_ioctl_get_max_priority_radio(struct ol_ath_softc_net80211 *scn, caddr_t param)
{
    char wifi_iface[IFNAMSIZ];
    struct ieee80211com *ic = &scn->sc_ic;
    struct global_ic_list   *glist = ic->ic_global_list;
    struct ieee80211vap           *max_priority_stavap_up;
    struct ieee80211com *max_priority_ic;
    struct net_device *n_dev;
    struct qwrap_config_t *qwrap_config = (struct qwrap_config_t *)param;

    memset(&wifi_iface[0], 0, sizeof(wifi_iface));
    GLOBAL_IC_LOCK_BH(glist);
    max_priority_stavap_up = glist->max_priority_stavap_up;
    GLOBAL_IC_UNLOCK_BH(glist);
    if (max_priority_stavap_up &&
        (wlan_vdev_is_up(max_priority_stavap_up->vdev_obj) == QDF_STATUS_SUCCESS)) {
	max_priority_ic = max_priority_stavap_up->iv_ic;
	n_dev = max_priority_ic->ic_netdev;
	OS_MEMCPY(wifi_iface, n_dev->name, sizeof(wifi_iface));
	qdf_info("max priority radio:%s",wifi_iface);
    }
    memcpy(&qwrap_config->max_priority_radio, wifi_iface, IFNAMSIZ);
    return 0;
}
#endif

int32_t ol_ioctl_get_chan_vendorsurvey_info(struct ol_ath_softc_net80211 *scn,
        caddr_t param)
{
    int i;
    struct ieee80211com *ic = &scn->sc_ic;
    struct scan_chan_info *src_chan_info;
    struct extacs_chan_info *dest_chan_info = (struct extacs_chan_info *)param;
    struct external_acs_obj *extacs_handle = &ic->ic_extacs_obj;

    qdf_spin_lock(&extacs_handle->chan_info_lock);
    for (i = 0; i < NUM_MAX_CHANNELS; i++) {
        src_chan_info = &ic->ic_extacs_obj.chan_info[i];
        dest_chan_info->cycle_count = 0;
        dest_chan_info->channel_load = src_chan_info->chan_load;
        dest_chan_info->period = 0;
        dest_chan_info->noisefloor = src_chan_info->noise_floor;
        dest_chan_info->comp_usablity = 0;
        dest_chan_info->comp_usablity_sec80 = 0;
        dest_chan_info->max_regulatory_power = 0;
        dest_chan_info->max_regulatory_power_sec80 = 0;
        dest_chan_info->tx_power_tput = src_chan_info->tx_power_tput;
        dest_chan_info->tx_power_range = src_chan_info->tx_power_range;
        dest_chan_info++;
    }
    qdf_spin_unlock(&extacs_handle->chan_info_lock);

    return 0;
}

#if defined(WLAN_DISP_CHAN_INFO)
QDF_STATUS ol_ioctl_get_chan_info(struct ieee80211com *ic,
                                  struct ieee80211req_chaninfo_full *req_chan)
{
    struct ieee80211req_chaninfo *req_chan_info = &req_chan->req_chan_info;
    enum channel_dfs_state *dfs_ch_s =
        (enum channel_dfs_state *) req_chan->dfs_chan_state_arr;
    wlan_radio_get_chan_info(ic, true, req_chan_info->ic_chans, NULL,
                             &req_chan_info->ic_nchans);

    return utils_dfs_get_chan_dfs_state(ic->ic_pdev_obj, dfs_ch_s);
}
#endif /* WLAN_DISP_CHAN_INFO */

#if MESH_MODE_SUPPORT
extern void ieee80211_check_timeout_mesh_peer(void *arg, wlan_if_t vaphandle);
#endif

static void
ieee80211_reset_auth_fail_cnt(void *arg, wlan_if_t vaphandle)
{
    vaphandle->cont_auth_fail = 0;
}

/*
 * Per-ieee80211com timer callback.
 * used for checking any timeouts in the
 * COM device.
 */
static OS_TIMER_FUNC(ol_ath_timeout)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn = NULL;
    struct ol_ath_soc_softc *soc;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    if (ic == NULL)
        return;

    /* Don't process timeout when recovery in progress */
    if (ic->recovery_in_progress) {
       return ;
    }

    ieee80211_session_timeout(ic);
    ieee80211_noassoc_sta_timeout(ic);
    ieee80211_vap_mlme_inact_erp_timeout(ic);
    scn =  OL_ATH_SOFTC_NET80211(ic);

    if (!scn || !scn->soc) {
        return;
    }
    soc = scn->soc;

    wlan_iterate_vap_list(ic, ieee80211_reset_auth_fail_cnt, NULL);
#if MESH_MODE_SUPPORT
    /* check & delete timed out mesh peers */
    wlan_iterate_vap_list(ic, ieee80211_check_timeout_mesh_peer, NULL);
#endif

    OS_SET_TIMER(&ic->ic_inact_timer, IEEE80211_SESSION_WAIT*1000);
    if (scn->scn_wmi_dis_dump && soc->soc_attached) {
        if (qdf_ktime_to_ms(qdf_ktime_get()) - scn->last_sent_time >
                                 (scn->scn_wmi_hang_wait_time*1000)) {
            ol_ath_set_fw_hang(soc->psoc_obj, 0, PANIC_TYPE_ASSERT);
            scn->last_sent_time = qdf_ktime_to_ms(qdf_ktime_get());
        }
    }
}

static bool
ol_ath_support_phy_mode(struct ieee80211com *ic, enum ieee80211_phymode mode)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    u_int8_t cur_rx_mask = ieee80211com_get_rx_chainmask(ic);
    u_int8_t cur_tx_mask = ieee80211com_get_tx_chainmask(ic);
    bool rx_mask_is_valid = false;
    bool tx_mask_is_valid = false;
    uint32_t target_type;

    target_type = lmac_get_tgt_type(scn->soc->psoc_obj);

    /* For Lithium onwards */
    if (ol_target_lithium(scn->soc->psoc_obj)) {

        /* Validate TX & RX Chain mask for a given PHY mode */
        rx_mask_is_valid = ol_ath_validate_chainmask(scn, cur_rx_mask,
                VALIDATE_RX_CHAINMASK, mode);
        tx_mask_is_valid = ol_ath_validate_chainmask(scn, cur_tx_mask,
                VALIDATE_TX_CHAINMASK, mode);

        if (!rx_mask_is_valid || !tx_mask_is_valid)
            return false;
    } else {
        if (ieee80211_is_phymode_160_or_8080(mode)) {
            if (target_type == TARGET_TYPE_QCA9984) {
                /* Note: In case
                 * IEEE80211_MODE_11AXA_HE160/IEEE80211_MODE_11AXA_HE80_80 are used
                 * with TARGET_TYPE_QCA9984, then there is a separate code block
                 * which checks whether these modes are available for
                 * TARGET_TYPE_QCA9984 (via stubbing).
                 */

                /* 160 and 80+80 modes are allowed only with the following chainmasks
                 * in Cascade */
                switch (cur_rx_mask) {
                    case 0x5:
                    case 0x6:
                        /*
                         * We permit 0x7 for special case of 3x3 80MHz and 1x1 160MHz in
                         * QCA9984
                         */
                    case 0x7:
                    case 0x9:
                    case 0xa:
                    case 0xc:
                    case 0xf:
                        rx_mask_is_valid = true;
                        break;
                    default:
                        return false;
                }

                switch (cur_tx_mask) {
                    case 0x5:
                    case 0x6:
                        /*
                         * We permit 0x7 for special case of 3x3 80MHz and 1x1 160MHz in
                         * QCA9984
                         */
                    case 0x7:
                    case 0x9:
                    case 0xa:
                    case 0xc:
                    case 0xf:
                        if(rx_mask_is_valid)
                            return true;
                        else
                            return false;
                    default:
                        return false;
                }
            } else if (target_type == TARGET_TYPE_QCA9888 ) {
                switch (cur_rx_mask) {
                    /* 160 and 80+80 modes are allowed only with the following
                     * chainmasks in QCA9888 */
                    case 0x3:
                        return true;
                    default:
                        return false;
                }
            }
        }
    }
    /* Currently this function is used to validate 160/80+80 MHz vs. chainmask
     * compatibility for 160/80+80 MHz capable chipsets. Return true for other
     * cases.
     */
    return true;
}

/* Provides info on whether AP supports mode 160 or 80p80.
 * Modecaps can indicate whether AP or STA can come up in that particular mode.
 */
static bool
ieee80211_is_radio_max_160_or_8080(struct ieee80211com *ic)
{
   if ((ic->ic_current_modecaps & (1<<IEEE80211_MODE_11AC_VHT160)) ||
        (ic->ic_current_modecaps & (1<<IEEE80211_MODE_11AC_VHT80_80)) ||
        (ic->ic_current_modecaps & (1<<IEEE80211_MODE_11AXA_HE160)) ||
        (ic->ic_current_modecaps & (1ULL<<IEEE80211_MODE_11AXA_HE80_80))
#ifdef WLAN_FEATURE_11BE
        || (ic->ic_current_modecaps & (1ULL<<IEEE80211_MODE_11BEA_EHT160))
        || (ic->ic_current_modecaps & (1ULL<<IEEE80211_MODE_11BEA_EHT320))
#endif
        )
      return true;
   return false;
}

static int
ol_ath_get_bw_nss_mapping(struct ieee80211vap *vap,
                          struct ieee80211_bwnss_map *nssmap, uint8_t chainmask)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    int error = 0;
    uint32_t target_type;
    enum ieee80211_phymode cur_mode;
    uint32_t iv_nss;

    target_type =  lmac_get_tgt_type(psoc);

    /* Enabling STA max caps (if enabled through the flag) */
    if (vap->iv_sta_max_ch_cap) {
        cur_mode = vap->iv_des_mode;
    } else {
        cur_mode = vap->iv_cur_mode;
    }

    if (ieee80211_is_radio_max_160_or_8080(ic)
#ifdef WLAN_FEATURE_11BE
        || ieee80211_is_phymode_320(cur_mode)
#endif
       ) {
        if (target_type == TARGET_TYPE_QCA9984) {
            /* 160 and 80+80 modes are allowed only with the following chainmasks
             * in QCA9984. */
            switch (chainmask) {
                case 0x5:
                case 0x6:
                /*
                 * We permit 0x7 for special case of 3x3 80MHz and 1x1 160MHz in
                 * QCA9984
                 */
                case 0x7:
                case 0x9:
                case 0xa:
                case 0xc:
                    nssmap->bw_nss_160 = IEEE80211_BW_NSS_MAP_1x1;
                    break;
                case 0xf:
                    nssmap->bw_nss_160 = IEEE80211_BW_NSS_MAP_2x2;
                    break;
                default:
                    error = -EINVAL;
            }
        } else if (target_type == TARGET_TYPE_QCA9888) {
            /* 160 and 80+80 modes are allowed only with the following chainmasks
             * in QCA9888.
             */
                switch (chainmask) {
                    case 0x3:
                        nssmap->bw_nss_160 = IEEE80211_BW_NSS_MAP_1x1;
                        break;
                    default:
                        error = -EINVAL;
                }
        } else if ((target_type == TARGET_TYPE_QCA8074) ||
			(target_type == TARGET_TYPE_QCA8074V2) ||
			(target_type == TARGET_TYPE_QCA9574) ||
			(target_type == TARGET_TYPE_QCN9000) ||
			(target_type == TARGET_TYPE_QCN9224) ||
			(target_type == TARGET_TYPE_QCN6122) ||
			(target_type == TARGET_TYPE_QCN9160) ||
			(target_type == TARGET_TYPE_QCA5018) ||
			(target_type == TARGET_TYPE_QCA5332) ||
			(target_type == TARGET_TYPE_QCA6018) ||
			(target_type == TARGET_TYPE_QCN6432)) {
            ieee80211_compute_nss(ic, chainmask, nssmap);
        } else {
            /* Currently this function is used to find valid 160MHz NSS map for
             * 160/80+80 MHz capable chipsets based on the rx_chainmask
             * configured. Return error for other cases */
            error = -EINVAL;
        }
    } else {
        /* Currently this function is used to find valid 160MHz NSS map for
         * 160/80+80 MHz capable chipsets based on the rx_chainmask configured.
         * Return error for other cases */
        error = -EINVAL;
    }

    wlan_util_vdev_mlme_get_param(vap->vdev_mlme,
            WLAN_MLME_CFG_NSS, &iv_nss);
    if ((error != -EINVAL) && (iv_nss != 0)) {

#define IS_FACTOR_1_2_OF_MAX_NSS(x, y) ((8 * x) <= (5 * y))

        nssmap->bw_nss_160 = QDF_MIN(iv_nss, nssmap->bw_nss_160);
        if (nssmap->bw_nss_160 == iv_nss) {
            /* Same for all bandwidth */
            nssmap->flag = IEEE80211_NSSMAP_SAME_NSS_FOR_ALL_BW;
        } else if(IS_FACTOR_1_2_OF_MAX_NSS(nssmap->bw_nss_160, iv_nss)) {
            /* 1/2 case */
            nssmap->bw_nss_160 = iv_nss >> 1;
            nssmap->flag = IEEE80211_NSSMAP_1_2_FOR_160_AND_80_80;
        } else {
            /* 3/4 case */
            nssmap->flag = IEEE80211_NSSMAP_3_4_FOR_160_AND_80_80;
        }
    }

    return error;
}

static int
ol_ath_net80211_addba_responsesetup(struct ieee80211_node *ni, uint8_t tidno,
                                    uint8_t *dialogtoken, uint16_t *statuscode,
                                    struct ieee80211_ba_parameterset *baparamset,
                                    uint16_t *batimeout, uint8_t *extbufsz)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    uint16_t buffersize;

    if (!(soc_txrx_handle = wlan_psoc_get_dp_handle(psoc)))
        return QDF_STATUS_E_CANCELED;

    baparamset->amsdusupported = IEEE80211_BA_AMSDU_SUPPORTED;

    if (ieee80211com_get_rx_amsdu(ic, tidno) == 0) {
        baparamset->amsdusupported = !IEEE80211_BA_AMSDU_SUPPORTED;
    }

    baparamset->bapolicy = IEEE80211_BA_POLICY_IMMEDIATE;
    baparamset->tid = tidno;
    if (cdp_addba_responsesetup(soc_txrx_handle, peer->macaddr, wlan_vdev_get_id(vdev),
              tidno, dialogtoken, statuscode, &buffersize, batimeout) != QDF_STATUS_SUCCESS)
        return QDF_STATUS_E_FAILURE;

    baparamset->buffersize = (buffersize % MAX_BA_BUFF_SIZE);
    *extbufsz = buffersize / MAX_BA_BUFF_SIZE ;

    return 0;
}

static int ol_ath_net80211_addba_resp_tx_completion(struct ieee80211_node *ni,
                                                    uint8_t tidno, int status)
{
    struct ieee80211com *ic = ni->ni_ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    return cdp_addba_resp_tx_completion(soc_txrx_handle, peer->macaddr,
             wlan_vdev_get_id(vdev), tidno, status);
}

static int
ol_ath_net80211_addba_requestprocess(struct ieee80211_node *ni,
                                     uint8_t dialogtoken,
                                     struct ieee80211_ba_parameterset *baparamset,
                                     uint16_t batimeout,
                                     struct ieee80211_ba_seqctrl basequencectrl,
                                     uint16_t tot_buf_sz)
{
    struct ieee80211com *ic = ni->ni_ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    return cdp_addba_requestprocess(soc_txrx_handle, peer->macaddr,
                                    wlan_vdev_get_id(vdev), dialogtoken,
                                    baparamset->tid, batimeout,
                                    tot_buf_sz,
                                    basequencectrl.startseqnum);
}

static void
ol_ath_net80211_delba_process(struct ieee80211_node *ni,
                              struct ieee80211_delba_parameterset *delbaparamset,
                              uint16_t reasoncode)
{
    struct ieee80211com *ic = ni->ni_ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    cdp_delba_process(soc_txrx_handle, peer->macaddr,  wlan_vdev_get_id(vdev),
                        delbaparamset->tid, reasoncode);
}

static int ol_ath_net80211_delba_tx_completion(struct ieee80211_node *ni,
                                               uint8_t tidno, int status)
{

    struct ieee80211com *ic = ni->ni_ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    struct wlan_objmgr_vdev *vdev = wlan_peer_get_vdev(peer);
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    return cdp_delba_tx_completion(soc_txrx_handle, peer->macaddr,
                                   wlan_vdev_get_id(vdev), tidno, status);
}

static int ol_ath_get_target_phymode(struct ieee80211com *ic, uint32_t phymode,
                                     bool is_2gvht_en)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    return ol_get_phymode_info(scn, phymode, is_2gvht_en);
}

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
int ol_ath_set_mac_addr_rx_filter(struct wlan_objmgr_pdev *pdev,
                                  struct set_rx_mac_filter *param)
{
    struct wmi_unified *wmi_handle = NULL;
    QDF_STATUS status;

    wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!wmi_handle)
        return -1;

    status = wmi_unified_set_mac_addr_rx_filter(wmi_handle, param);
    return qdf_status_to_os_return(status);
}
#else
int ol_ath_set_mac_addr_rx_filter(struct wlan_objmgr_pdev *pdev,
                                  struct set_rx_mac_filter *param)
{
    return 0;
}
#endif

#ifdef WLAN_SUPPORT_RF_CHARACTERIZATION
#if ATH_ACS_DEBUG_SUPPORT
static int
ol_ath_set_chan_grade_info(struct ieee80211com *ic,
                           void* channel_events, uint8_t nchan)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_ath_soc_softc_t *soc = scn->soc;
    struct acs_debug_chan_event_container *chan = (struct acs_debug_chan_event_container *)channel_events;
    uint8_t ix;

    if (!soc) {
        qdf_err("Invalid soc");
        return -EINVAL;
    }

    if (soc->rf_characterization_entries) {
        qdf_mem_free(soc->rf_characterization_entries);
        soc->num_rf_characterization_entries = 0;
    }

    /*
     * NOTE: Since the rf_characterization_entries is maintained outside the scope
     *       of the ACS debug framework, the responsibility to free it is left
     *       as is.
     */
    soc->num_rf_characterization_entries = nchan;
    soc->rf_characterization_entries = qdf_mem_malloc(soc->num_rf_characterization_entries *
                                       sizeof(*soc->rf_characterization_entries));

    if (!soc->rf_characterization_entries) {
        soc->num_rf_characterization_entries = 0;
        qdf_info("Could not allocate space for rf_characterization_entries");
        return -EINVAL;
    }

    qdf_mem_zero(soc->rf_characterization_entries,
                 soc->num_rf_characterization_entries * sizeof(*soc->rf_characterization_entries));

    for (ix = 0; ix < soc->num_rf_characterization_entries; ix++) {
        soc->rf_characterization_entries[ix].freq = chan->event[ix].channel_freq;
        /* Bandwidth is currently being set to zero by default*/
        soc->rf_characterization_entries[ix].bw = 0;
        soc->rf_characterization_entries[ix].chan_metric = chan->event[ix].channel_rf_characterization;
    }

    return 0;
}
#endif

static void
ol_ath_get_chan_grade_info(struct ieee80211com *ic, uint32_t *hw_chan_grade_list)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_ath_soc_softc_t *soc = scn->soc;
    uint16_t ieee_chan;
    int i;

    if (!soc) {
        qdf_err("Invalid soc");
        return;
    }

    if (!soc->rf_characterization_entries) {
        qdf_debug("RF characterization entries is empty");
        return;
    }

    for (i = 0; i < soc->num_rf_characterization_entries; i++) {
        ieee_chan = ieee80211_acs_get_chan_idx(ic->ic_acs, soc->rf_characterization_entries[i].freq);

        if (soc->rf_characterization_entries[i].bw != WMI_HOST_CHAN_WIDTH_20) {
            /*
             * Channel metrics for bandwidths other than 20MHz are not
             * supported.
             */
            qdf_info("Skipping non-20MHz RF characterization entries");
            continue;
        }

        if (ieee_chan < IEEE80211_CHAN_MAX) {
            /* hw_chan_grade_list is an array of size IEE80211_CHAN_MAX */
            hw_chan_grade_list[ieee_chan] = soc->rf_characterization_entries[i].chan_metric;
        }
    }

    return;
}
#endif

static int ol_ath_set_stats_update_period(struct ieee80211vap *vap,
                                          uint32_t val)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_ath_soc_softc_t *soc = scn->soc;
    uint32_t target_version = lmac_get_tgt_version(soc->psoc_obj);

    if ((target_version == AR9887_REV1_VERSION) ||
        (target_version == AR9888_REV1_VERSION) ||
        (target_version == AR9888_REV2_VERSION) ||
        (target_version == AR9888_DEV_VERSION)  ||
        (target_version == QCA9984_DEV_VERSION)) {

        if ((val < PDEV_MIN_STATS_UPDATE_PERIOD && val != 0)
            || val > PDEV_DEFAULT_STATS_UPDATE_PERIOD) {
            qdf_err("Invalid update period, should be in range[%d,%d]",
                     PDEV_MIN_STATS_UPDATE_PERIOD,
                     PDEV_DEFAULT_STATS_UPDATE_PERIOD);
            return -EINVAL;
        }
        val = (val == 0)? PDEV_DEFAULT_STATS_UPDATE_PERIOD : val;
        /* set the pdev stats update period*/
        ol_ath_pdev_set_param(scn->sc_pdev,
                              wmi_pdev_param_pdev_stats_update_period, val);
    } else {
        qdf_err("Target not supported for this cmd");
        return -EINVAL;
    }

    return 0;
}

#ifdef CONFIG_AFC_SUPPORT
/**
 * ol_ath_afc_send_response_to_afcmem(): Write AFC response buffer in memory.
 * @psoc: pointer to psco object.
 * @pdev: pointer to pdev object.
 * @data: pointer to AFC response data.
 * @len:  Length of AFC response data.
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_afc_send_response_to_afcmem(struct wlan_objmgr_psoc *psoc,
                                              struct wlan_objmgr_pdev *pdev,
                                              struct wlan_afc_host_resp *data,
                                              uint32_t len)
{
    qdf_device_t qdf_dev;
    int pdev_id;
    char *afcdb = (char *)data;

    qdf_dev = wlan_psoc_get_qdf_dev(psoc);
    if (!qdf_dev) {
        qdf_err("Invalid qdf dev");
        return -EINVAL;
    }

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    return pld_send_buffer_to_afcmem(qdf_dev->dev,
                                     afcdb,
                                     len,
                                     pdev_id);
}

/**
 * ol_ath_register_afc_write_to_afcmem(): Registers to write AFC response in
 * memory.
 * @pdev: pointer to pdev object.
 *
 * return: Nothing
 */
static void ol_ath_register_afc_write_to_afcmem(struct wlan_objmgr_pdev *pdev)
{
    struct wlan_objmgr_psoc *psoc;

    psoc = wlan_pdev_get_psoc(pdev);
    ucfg_afc_register_data_send_cb(psoc, ol_ath_afc_send_response_to_afcmem);
}
#endif

int
ol_ath_dev_attach(struct ol_ath_softc_net80211 *scn,
                   IEEE80211_REG_PARAMETERS *ieee80211_conf_parm)
{
    struct ieee80211com *ic = &scn->sc_ic;
    ol_ath_soc_softc_t *soc = scn->soc;
    int error = 0;
    uint32_t target_type;
    spin_lock_init(&ic->ic_main_sta_lock);
    spin_lock_init(&ic->ic_chan_change_lock);
    spin_lock_init(&ic->ic_addba_lock);
    IEEE80211_STATE_LOCK_INIT(ic);
    spin_lock_init(&ic->ic_beacon_alloc_lock);

    spin_lock_init(&ic->ic_state_check_lock);
    spin_lock_init(&ic->ic_radar_found_lock);
    spin_lock_init(&ic->ic_radar_mode_switch_lock);
#ifdef WLAN_FEATURE_11BE_MLO
    spin_lock_init(&ic->ic_cu_vdev_map_lock);
#endif
    /* attach channel width management */
    error = ol_ath_cwm_attach(scn);
    if (error) {
        qdf_err("ol_ath_cwm_attach failed");
        return error;
    }

    target_type = lmac_get_tgt_type(soc->psoc_obj);
    /* XXX not right but it's not used anywhere important */
    ieee80211com_set_phytype(ic, IEEE80211_T_OFDM);

    /*
     * Set the Atheros Advanced Capabilities from station config before
     * starting 802.11 state machine.
     */
     /* TBD */

    /* this matches the FW default value */
    scn->arp_override = WME_AC_VO;
    scn->igmpmld_override = 0;
    scn->igmpmld_tid = 0;

    /* set default mgmt retry limit */
    scn->scn_mgmt_retry_limit = DEFAULT_MGMT_RETRY_LIMIT;

    scn->macreq_ext_enabled = cfg_get(soc->psoc_obj, CFG_OL_MACREQ_EXT_ENABLE);
    if (scn->macreq_ext_enabled) {
        if (!(wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                         WLAN_PDEV_F_MBSS_IE_ENABLE) ||
              ic->ic_cohosted_bss_support)) {
            qdf_err("MBSSID or co-hosted mode is required for Macreq Ext");
            scn->macreq_ext_enabled = false;
        }
    }
    qdf_info("Macreq Ext enabled=%d", scn->macreq_ext_enabled);

    /* Set the mac address */

    /* Setup Min frame size */
    ic->ic_minframesize = sizeof(struct ieee80211_frame_min);

    ic->id_mask_vap_downed[VDEV_BITMAP_SLOT0] = 0;
    ic->id_mask_vap_downed[VDEV_BITMAP_SLOT1] = 0;

    /*
     * Setup some device specific ieee80211com methods
     */
    ic->ic_init = ol_ath_init;
    ic->ic_reset_start = ol_ath_reset_start;
    ic->ic_reset = ol_ath_reset;
    ic->ic_reset_end = ol_ath_reset_end;
    ic->ic_wme.wme_update = ol_ath_wmm_update;
    ic->ic_epcs_edca.epcs_edca_update = ol_ath_epcs_edca_update;
    ic->ic_txq_depth = ol_ath_txq_depth;
    ic->ic_txq_depth_ac = ol_ath_txq_depth_ac;
    ic->ic_chwidth_change = ol_net80211_chwidth_change;
    ic->ic_nss_change = ol_net80211_nss_change;
    ic->ic_ext_nss_change = ol_net80211_ext_nss_change;
    ic->ic_frame_injector_config = ol_ath_frame_injector_config;
    ic->ic_ar900b_fw_test = ol_ath_ar900b_fw_test;
    ic->ic_fw_unit_test = ol_ath_fw_unit_test;
    ic->ic_coex_cfg = ol_ath_coex_cfg;
#if UNIFIED_SMARTANTENNA
    ic->ic_set_ant_switch = ol_ath_set_ant_switch_tbl;
#endif
    ic->ic_set_ctrl_table = ol_ath_set_ctl_table;
    ic->ic_send_tdma_sched = ol_ath_send_tdma_schedule;
    ic->ic_set_beacon_interval = ol_set_beacon_interval;
    ic->ic_set_sta_fixed_rate = ol_net80211_set_sta_fixed_rate;
    ic->ic_support_phy_mode = ol_ath_support_phy_mode;
    ic->ic_update_chainmask_caps = ol_ath_update_chainmask_caps;
    ic->ic_get_bw_nss_mapping = ol_ath_get_bw_nss_mapping;

    ic->ic_trigger_dcs = ol_ath_trigger_dcs;
	/* host side umac compiles with this flag, so we have no
	   option than writing this with flag, otherwise the other
	   drivers in oher OSes would fail*/
    ic->ic_disable_dcsim = ol_ath_disable_dcsim;
    ic->ic_enable_dcsim = ol_ath_enable_dcsim;
    ic->ic_disable_dcscw = ol_ath_disable_dcscw;
#ifdef QCA_STANDALONE_SOUNDING_TRIGGER
    ic->ic_trigger_trigger_txbf_sounding = ol_ath_trigger_txbf_sounding;
#endif
    ic->ic_dcs_restore = ol_ath_dcs_restore;
    ic->ic_fips_test = ol_ath_fips_test;
#if QCA_MANUAL_TRIGGERED_ULOFDMA
    ic->ic_trigger_ul_ofdma_for_peer = ol_ath_trigger_ul_ofdma_for_peer;
#endif
#ifdef WLAN_FEATURE_FIPS_BER_CCMGCM
    ic->ic_fips_extend_test = ol_ath_fips_extend_test;
    ic->ic_fips_mode_set = ol_ath_fips_mode_set;
#endif
/* Handles UTF commands from FTM daemon */
#if UMAC_SUPPORT_CFG80211
    ic->ic_ucfg_testmode_cmd = ol_ath_ucfg_utf_unified_cmd;
#endif
#ifdef WLAN_SUPPORT_MESH_LATENCY
    ic->ic_set_peer_latency_param_config =
                      ol_ath_set_peer_latency_param_config;
#endif
    /* Attach twt */
    ol_ath_twt_attach(ic);
    ol_ath_kbps_to_mcs(soc, ic);
    ol_ath_ratecode_to_kbps(soc, ic);
    ol_ath_get_supported_rates(soc, ic);
    /* Attach resmgr module */
    ol_ath_resmgr_attach(ic);

    ol_ath_power_attach(ic);
    ic->ic_is_target_lithium = ol_target_lithium;
    ic->ic_is_target_be = ol_target_be;

    /*
     * Attach ieee80211com object to net80211 protocal stack.
     */

    if (cfg_get(soc->psoc_obj, CFG_OL_NAWDS_LEARN_DEFER))
        ic->nawds_learn_defer = 1;
    else
        ic->nawds_learn_defer = 0;

    if (cfg_get(soc->psoc_obj, CFG_OL_ENABLE_MONITOR_PEER_DECAP))
        ic->enable_monitor_peer_decap = 1;
    else
        ic->enable_monitor_peer_decap = 0;

    error = ieee80211_ifattach(ic, ieee80211_conf_parm);
    if (error) {
        qdf_err("ieee80211_ifattach failed error : %d", error);
        return error;
    }

#ifdef CONFIG_AFC_SUPPORT
    /* Register callback to send AFC response */
    ol_ath_register_afc_write_to_afcmem(ic->ic_pdev_obj);
#endif
    /* Do not request partially random ref_bssid at init
     */
    ol_ath_assign_mbssid_ref_bssid(scn, false);

    ol_ath_assign_cohosted_ref_bssid(scn, false);

    qdf_spinlock_create(&ic->ic_channel_stats.lock);
     /*
     * Complete device specific ieee80211com methods init
     */
    ic->ic_set_channel = ol_ath_set_channel;
    ic->ic_enable_radar = ol_ath_net80211_enable_radar;

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
    ic->ic_enable_sta_radar = NULL;
#endif
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    ic->ic_mlo_capable = ol_ath_mlo_capable;
#endif
    ic->ic_pwrsave_set_state = ol_ath_pwrsave_set_state;
    ic->ic_get_noisefloor = ol_ath_get_noisefloor;
    ic->ic_get_chainnoisefloor = ol_ath_get_chainnoisefloor;
    ic->ic_set_txPowerLimit = ol_ath_setTxPowerLimit;
    ic->ic_get_common_power = ol_ath_get_common_power;
    ic->ic_get_TSF32        = ol_ath_getTSF32;
    ic->ic_rmgetcounters = ol_ath_getrmcounters;
    ic->ic_get_wpsPushButton = ol_ath_wpsPushButton;
    ic->ic_clear_phystats = ol_ath_clear_phystats;
    ic->ic_set_macaddr = ol_ath_set_macaddr;
    ic->ic_log_text = ol_ath_log_text;
    ic->ic_log_text_bh = ol_ath_log_text_bh;
    ic->ic_set_chain_mask = ol_ath_set_chain_mask;
    ic->ic_get_mfpsupport = ol_ath_getmfpsupport;
    ic->ic_set_hwmfpQos   = ol_ath_setmfpQos;
    ic->ic_get_tx_hw_retries  = ol_ath_get_tx_hw_retries;
    ic->ic_get_tx_hw_success  = ol_ath_get_tx_hw_success;
    ic->ic_rate_node_update = ol_ath_rate_node_update;
#if ATH_SUPPORT_IQUE
    ic->ic_set_acparams = ol_ath_set_acparams;
    ic->ic_set_rtparams = ol_ath_set_rtparams;
    ic->ic_get_iqueconfig = ol_ath_get_iqueconfig;
    ic->ic_set_hbrparams = ol_ath_set_hbrparams;
#endif
    ic->ic_set_config = ol_ath_set_config;
    ic->ic_set_config_enable = ol_ath_set_config_enable;
    ic->ic_set_peer_tid_noack_config = ol_ath_set_peer_tid_noack_config;
    ic->ic_set_fw_hang = ol_ath_set_fw_hang;
    ic->ic_set_safemode = ol_ath_set_safemode;
    ic->ic_set_privacy_filters = ol_ath_set_privacy_filters;
    ic->ic_addba_responsesetup = ol_ath_net80211_addba_responsesetup;
    ic->ic_addba_requestprocess = ol_ath_net80211_addba_requestprocess;
    ic->ic_addba_resp_tx_completion = ol_ath_net80211_addba_resp_tx_completion;
    ic->ic_delba_process = ol_ath_net80211_delba_process;
    ic->ic_delba_tx_completion = ol_ath_net80211_delba_tx_completion;
#ifdef ATH_SUPPORT_TxBF // For TxBF RC

#if WLAN_OBJMGR_REF_ID_TRACE
    ic->ic_ieee80211_find_node_debug = ieee80211_find_node_debug;
#else
    ic->ic_ieee80211_find_node = ieee80211_find_node;
#endif //WLAN_OBJMGR_REF_ID_TRACE
    ic->ic_v_cv_send = ol_ath_net80211_v_cv_send;
    ic->ic_txbf_alloc_key = ol_ath_net80211_txbf_alloc_key;
    ic->ic_txbf_set_key = ol_ath_net80211_txbf_set_key;
    ic->ic_txbf_del_key = ol_ath_net80211_txbf_del_key;
    ic->ic_init_sw_cv_timeout = ol_ath_net80211_init_sw_cv_timeout;
    ic->ic_set_txbf_caps = ol_ath_set_txbfcapability;
#ifdef TXBF_DEBUG
	ic->ic_txbf_check_cvcache = ol_ath_net80211_txbf_check_cvcache;
#endif
    ic->ic_txbf_stats_rpt_inc = ol_ath_net80211_txbf_stats_rpt_inc;
    ic->ic_txbf_set_rpt_received = ol_ath_net80211_txbf_set_rpt_received;
#endif
    ic->ic_get_cur_chan_nf = ol_ath_net80211_get_cur_chan_noisefloor;
    ic->ic_get_cur_hw_nf = ol_ath_net80211_get_cur_hw_nf;
    ic->ic_set_rxfilter = ol_ath_set_rxfilter;
    ic->ic_set_ctl_table = ol_ath_set_ctl_table;
    ic->ic_set_mimogain_table = ol_ath_set_mimogain_table;
    ic->ic_ratepwr_table_ops = ol_ath_ratepwr_table_ops;
    ic->ic_set_mgmt_retry_limit = ol_ath_set_mgmt_retry_limit;
    ic->ic_tr69_request_process = ol_ath_net80211_tr69_process_request;
    ic->wide_band_scan_enabled = ol_ath_wide_band_scan;
    ic->ic_get_target_phymode = ol_ath_get_target_phymode;
    ic->ic_set_mac_addr_rx_filter = ol_ath_set_mac_addr_rx_filter;
    if (ol_target_lithium(soc->psoc_obj))
        ic->ic_print_peer_refs = ol_ath_print_peer_refs;
    else
        ic->ic_print_peer_refs = NULL;
#if ATH_SUPPORT_DFS && QCA_DFS_NOL_VAP_RESTART
    ic->ic_pause_stavap_scan = 0;
#endif

    /*
     * In Offload case 'inactivity' is handled in the firmware.
     * So override the default function with a different one to handle other timeouts.
     */
    OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_inact_timer), ol_ath_timeout,
            (void *) (ic), QDF_TIMER_TYPE_WAKE_APPS);

    ic->ic_nl_handle = NULL;
#if QCA_LTEU_SUPPORT
    ol_ath_nl_attach(ic);
#endif

    /*
     * pktlog scn initialization
     */
#ifndef REMOVE_PKT_LOG
    if(enable_pktlog_support)
        ol_pktlog_attach(scn);
#endif

#ifdef QCA_PARTNER_PLATFORM
    ic->partner_com_params.ipc_ol_txrx_ast_find_hash_find = ol_txrx_ast_find_hash_find;
    ic->partner_com_params.ipc_ol_txrx_peer_find_by_id = ol_txrx_peer_find_by_id;
    ic->partner_com_params.ipc_ol_ath_getvap = ol_ath_getvap;
#endif
#ifdef CONFIG_DP_TRACE
    ic->ic_dptrace_set_param = ol_ath_dptrace_setparam;
#endif

    ol_if_eeprom_attach(ic);

    ol_ath_wow_attach(ic);
#if  ATH_DATA_TX_INFO_EN
    ol_ath_stats_attach(ic);
#endif
#if ATH_OL_FAST_CHANNEL_RESET_WAR
    ol_ath_fast_chan_change(scn->sc_pdev);
#endif
#ifdef QCA_BLOCK_EVENT_TO_USERSPACE
#if UMAC_SUPPORT_ACFG
    ic->ic_acfg_ev_fl = g_acfg_ev_fl;
#endif
    ic->ic_iwevcustom_ev_fl = g_iwevcustom_ev_fl;
    ic->ic_cfg_ev_fl = g_cfg_ev_fl;
#endif /* QCA_BLOCK_EVENT_TO_USERSPACE */

    ic->ic_no_vlan = 0;
    ic->ic_atf_logging = 0;
    ic->ic_set_pcp_tid_map = ol_ath_set_vap_pcp_tid_map;
    ic->ic_set_tidmap_tbl_id = ol_ath_set_vap_tidmap_tbl_id;
    ic->ic_set_tidmap_prty = ol_ath_set_vap_tidmap_prty;
#ifdef WLAN_SUPPORT_RF_CHARACTERIZATION
#if ATH_ACS_DEBUG_SUPPORT
    ic->ic_set_chan_grade_info = ol_ath_set_chan_grade_info;
#endif
    ic->ic_get_chan_grade_info = ol_ath_get_chan_grade_info;
#endif
    ic->ic_set_stats_update_period = ol_ath_set_stats_update_period;
    ic->ic_assemble_ratecode = ol_ath_assemble_ratecode;
#ifdef WLAN_FEATURE_11BE
    ic->ic_send_mlo_peer_tid_to_link_map = ol_ath_send_mlo_peer_tid_to_link_map;
    ic->ic_send_mlo_peer_established_mapping = ol_ath_send_mlo_peer_established_mapping;
    ic->ic_send_mu_param = ol_ath_send_mu_param;
#endif /* WLAN_FEATURE_11BE */
#if (defined(WLAN_SUPPORT_SCS) && defined(WLAN_FEATURE_11BE)) || defined(CONFIG_SAWF)
    ic->ic_send_ul_scs_qos_params = ol_ath_send_ul_scs_qos_params;
#endif
    ic->ic_get_q6_time = pld_get_q6_time;
#ifdef QCA_R2P_UPDATE_ENABLED
    ic->ic_read_set_home_chan_r2p_table = ol_ath_set_home_chan_r2p_table_async;
#endif
#ifdef WLAN_DBS_POWER_REDUCTION
    ic->ic_update_active_count = ol_ath_update_active_pdev_count;
#endif
    ic->ic_send_hwcts2self_command = ol_ath_send_hwcts2self_command;
    ic->ic_get_tgt_res_cfg = ol_ath_get_tgt_res_cfg;

    return EOK;
}

int
ol_asf_adf_attach(ol_ath_soc_softc_t *soc)
{
    asf_adf_attach();
    return EOK;
}

int
ol_asf_adf_detach(ol_ath_soc_softc_t *soc)
{
    return EOK;
}

#ifdef MU_CAP_WAR_ENABLED
static OS_TIMER_FUNC(ieee80211_mu_cap_mode_switch)
{
    struct     ieee80211com *ic;
    struct     ieee80211vap *vap = NULL;
    DEDICATED_CLIENT_MAC *dedicated, *temp;
    u_int8_t   i = 0;
    u_int16_t  associd;
    u_int16_t  total_mu_clients;

    MU_CAP_CLIENT_TYPE mu_type;
    ATH_LIST_HEAD(,DEDICATED_CLIENT_MAC)deauth_list;
    MU_CAP_WAR *war;
    LIST_INIT(&deauth_list);

    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

    /*mu_cap timer enrty*/
    ic = vap->iv_ic;
    war = &vap->iv_mu_cap_war;
    qdf_spin_lock_bh(&war->iv_mu_cap_lock);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,"IN Dedicated client TIMER\n");
    if (!war->mu_cap_war) {
        /*
         * Kicking out the dedicated SU Clients is
         * the only valid timer action
         * when the WAR is disabled.
         * You would never want to kick out
         * Dedicated-MU enabled client when WAR is
         * disabled!!
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                "In timer while MUCAP-WAR is disabled\n");
        war->mu_timer_cmd = MU_CAP_TIMER_CMD_KICKOUT_SU_CLIENTS;
    }


    total_mu_clients = war->mu_cap_client_num[MU_CAP_CLIENT_NORMAL] +
        war->mu_cap_client_num[MU_CAP_DEDICATED_MU_CLIENT] +
        war->mu_cap_client_num[MU_CAP_DEDICATED_SU_CLIENT];

    if ((war->mu_timer_cmd == MU_CAP_TIMER_CMD_KICKOUT_DEDICATED) &&
            ((total_mu_clients != 1) ||
             (war->mu_cap_client_num[MU_CAP_DEDICATED_MU_CLIENT] != 1))) {
        /*
         * Even thought the timer was started with the
         * purpose of kicking out MU-1X1 clients,
         * now, the conditions are not correct for kicking
         * out this MU-1X1 client,
         * possibly because other MU-Capable client(s) have joined
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                "Kick-out to SU-Capable 2x2 not possible now\n");
        goto end_of_timer;
    }

    switch(war->mu_timer_cmd)
    {
        case MU_CAP_TIMER_CMD_KICKOUT_DEDICATED:
            {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                        "KICKOUT-DEDICATED-MU-CLIENT\n");
                mu_type = MU_CAP_DEDICATED_MU_CLIENT;
                break;
            }
        case MU_CAP_TIMER_CMD_KICKOUT_SU_CLIENTS:
            {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                        "KICKOUT-ALL-DEDICATED-SU-CLIENTS\n");
                mu_type = MU_CAP_DEDICATED_SU_CLIENT;
                break;
            }
        default:
            {
                goto end_of_timer;
            }
    }


    /*
     * Copy the clients which meet the kick-out conditions
     * to a local table, then deauth them
     * outside of the iv_mu_cap_lock
     * This way the function ieee80211_mu_cap_client_leave can be
     * put inside iv_mu_cap_lock again
     */
    for(i=0;i<total_mu_clients;i++)
    {
        if (war->mu_cap_client_flag[i] == mu_type)
        {
            dedicated =
            OS_MALLOC(ic->ic_osdev, sizeof(struct DEDICATED_CLIENT_MAC), 0);
            if (dedicated == NULL) {
                ieee80211_note(vap, IEEE80211_MSG_ANY, "ERROR!! Memory "
                    "allocation failed in %s\n", __func__);
                continue;
            }
            OS_MEMCPY(dedicated->macaddr, war->mu_cap_client_addr[i],
                      QDF_MAC_ADDR_SIZE);
            LIST_INSERT_HEAD(&deauth_list, dedicated, list);
        }

    }

end_of_timer:
    war->iv_mu_timer_state = MU_TIMER_STOP;
    qdf_spin_unlock_bh(&war->iv_mu_cap_lock);

    /*
     * We now release the lock and call the
     * deauth function. The lock will be acquired later on
     * when the deauth function results in ieee80211_mu_cap_client_leave
     */
    /*De-assoc each dedicated clients*/
    LIST_FOREACH_SAFE(dedicated, &deauth_list, list, temp) {
        struct ieee80211_node *ni = ieee80211_find_node(ic, dedicated->macaddr, WLAN_MLME_SB_ID);
        LIST_REMOVE(dedicated, list);
        OS_FREE(dedicated);
        if (!ni) {
            ieee80211_note(vap, IEEE80211_MSG_ANY,
                    "NI IS NULL AFTER MU-CAP-WAR TIMER LOCK!\n");
            continue;
        }
        if (ni->ni_associd != 0 )
        {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                    "MUCAPWAR TIMER: Kicking out %s\n",
                            ether_sprintf(ni->ni_macaddr));
            associd = ni->ni_associd;
            wlan_mlme_deauth_request(ni->ni_vap,ni->ni_macaddr,
                    IEEE80211_REASON_UNSPECIFIED);
#ifdef WLAN_FEATURE_11BE_MLO
            IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(ni->ni_vap,
                    ni->ni_macaddr, associd, IEEE80211_REASON_UNSPECIFIED,
                    &ni->peer_obj->mldaddr[0]);
#else
            IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(ni->ni_vap,
                    ni->ni_macaddr, associd, IEEE80211_REASON_UNSPECIFIED,
                    NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
        }
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
    }
}

void ieee80211_mucap_vattach(struct ieee80211vap *vap) {
   struct ieee80211com *ic = vap->iv_ic;
   int cnt;
   MU_CAP_WAR *war = &vap->iv_mu_cap_war;
   osdev_t os_handle = ic->ic_osdev;
   war->iv_mu_timer_state = MU_TIMER_STOP;

   OS_INIT_TIMER(os_handle, &war->iv_mu_cap_timer,
           ieee80211_mu_cap_mode_switch, vap, QDF_TIMER_TYPE_WAKE_APPS);

   qdf_spinlock_create(&war->iv_mu_cap_lock);
   war->mu_cap_timer_period = MU_TIMER_PERIOD;

   for(cnt=0;cnt<ARRAY_SIZE(war->dedicated_client_list);cnt++) {
       LIST_INIT(&war->dedicated_client_list[cnt]);
   }
   war->dedicated_client_number = 0;

   /* Disable MU-CAP-WAR by default */
   war->modify_probe_resp_for_dedicated =
   war->mu_cap_war = 0;
}

void ieee80211_mucap_vdetach(struct ieee80211vap *vap) {

   struct DEDICATED_CLIENT_MAC *dedicated_mac=NULL, *temp=NULL;
   int cnt;
   MU_CAP_WAR *war = &vap->iv_mu_cap_war;
   qdf_spin_lock_bh(&war->iv_mu_cap_lock);
   if (war->iv_mu_timer_state == MU_TIMER_PENDING)
      war->iv_mu_timer_state = MU_TIMER_STOP;
   OS_FREE_TIMER(&war->iv_mu_cap_timer);
   qdf_spin_unlock_bh(&war->iv_mu_cap_lock);
   for(cnt=0;cnt<ARRAY_SIZE(war->dedicated_client_list);cnt++) {
       LIST_FOREACH_SAFE(dedicated_mac, &war->dedicated_client_list[cnt],
                         list, temp) {

           war->dedicated_client_number--;
           LIST_REMOVE(dedicated_mac,list);
           OS_FREE(dedicated_mac);
       }
   }
   qdf_spinlock_destroy(&war->iv_mu_cap_lock);
}
#endif

static void ol_swap_seg_free(ol_ath_soc_softc_t *soc, struct swap_seg_info *seg_info, u_int64_t *cpuaddr, int type)
{
    if(cpuaddr) {
#if WIFI_MEM_MANAGER_SUPPORT
        wifi_cmem_free(soc->soc_idx, 0, (CM_CODESWAP + type), seg_info->seg_cpuaddr[0]);
#else
        dma_free_coherent(soc->sc_osdev->device, seg_info->seg_size, seg_info->seg_cpuaddr[0] , (dma_addr_t)seg_info->seg_busaddr[0]);
#endif
        qdf_dev_release_mem((struct qdf_dev *)soc->sc_osdev->device,
                            (struct qdf_devm *)seg_info);
    }
}
void
ol_codeswap_detach (ol_ath_soc_softc_t *soc) {
    ol_swap_seg_free(soc, soc->target_otp_codeswap_seginfo, soc->target_otp_codeswap_cpuaddr, ATH_TARGET_OTP_CODE_SWAP);
    ol_swap_seg_free(soc, soc->target_otp_dataswap_seginfo, soc->target_otp_dataswap_cpuaddr, ATH_TARGET_OTP_DATA_SWAP);
    ol_swap_seg_free(soc, soc->target_bin_codeswap_seginfo, soc->target_bin_codeswap_cpuaddr, ATH_TARGET_BIN_CODE_SWAP);
    ol_swap_seg_free(soc, soc->target_bin_dataswap_seginfo, soc->target_bin_dataswap_cpuaddr, ATH_TARGET_BIN_DATA_SWAP);
    ol_swap_seg_free(soc, soc->target_bin_utf_codeswap_seginfo, soc->target_bin_utf_codeswap_cpuaddr,ATH_TARGET_BIN_UTF_CODE_SWAP);
    ol_swap_seg_free(soc, soc->target_bin_utf_dataswap_seginfo, soc->target_bin_utf_dataswap_cpuaddr,ATH_TARGET_BIN_UTF_DATA_SWAP);
}

int
ol_mempools_attach(ol_ath_soc_softc_t *soc)
{
    if(qdf_mempool_init(soc->qdf_dev, &soc->mempool_ol_ath_vap,
         soc->max_vaps, sizeof(struct ol_ath_vap_net80211), 0)) {
         soc->mempool_ol_ath_vap = NULL;
         qdf_err("ol_ath_vap memory pool init failed");
         goto fail1;
     }

    if(qdf_mempool_init(soc->qdf_dev, &soc->mempool_ol_ath_node,
        (CFG_MAX_TMPNODES + soc->max_vaps + soc->max_clients), sizeof(struct ol_ath_node_net80211), 0)) {
        soc->mempool_ol_ath_node = NULL;
        qdf_err("ol_ath_node memory pool init failed");
        goto fail2;
    }

    return 0;

fail2:
    qdf_mempool_destroy(soc->qdf_dev, soc->mempool_ol_ath_vap);
fail1:
    return -ENOMEM;
}

void
ol_mempools_detach(ol_ath_soc_softc_t *soc)
{
    qdf_mempool_destroy(soc->qdf_dev, soc->mempool_ol_ath_peer);
    qdf_mempool_destroy(soc->qdf_dev, soc->mempool_ol_ath_node);
    qdf_mempool_destroy(soc->qdf_dev, soc->mempool_ol_ath_vap);
}

/* Get default value of 80+80 EMI WAR enabled status based on chipset ID */
static u_int8_t
ol_ath_get_emiwar_80p80_defval(ol_ath_soc_softc_t *soc)
{
    u_int8_t defval=0;
    u_int32_t target_type = lmac_get_tgt_type(soc->psoc_obj);

    if (!IS_EMIWAR_80P80_APPLICABLE(target_type)) {
        return 0;
    }

    switch(target_type)
    {
        case TARGET_TYPE_QCA9984:
            defval = EMIWAR_80P80_DEFVAL_QCA9984;
            break;
        case TARGET_TYPE_QCA9888:
            defval = EMIWAR_80P80_DEFVAL_QCA9888;
            break;
        default:
            defval = 0;
            break;
    }

    return defval;
}

QDF_STATUS
ol_ath_get_wmi_target_type(ol_ath_soc_softc_t *soc, enum wmi_target_type *target)
{
    uint32_t target_type;

    target_type = lmac_get_tgt_type(soc->psoc_obj);

	switch(target_type)
	{
		case TARGET_TYPE_AR6002:
		case TARGET_TYPE_AR6003:
		case TARGET_TYPE_AR6004:
		case TARGET_TYPE_AR6006:
		case TARGET_TYPE_AR9888:
		case TARGET_TYPE_AR6320:
		case TARGET_TYPE_AR900B:
		case TARGET_TYPE_QCA9984:
		case TARGET_TYPE_QCA9888:
			*target = WMI_NON_TLV_TARGET;
			break;
#ifdef QCA_WIFI_QCA8074
		case TARGET_TYPE_QCA8074:
		case TARGET_TYPE_QCA8074V2:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCA6018
		case TARGET_TYPE_QCA6018:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCA9574
		case TARGET_TYPE_QCA9574:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCN9000
		case TARGET_TYPE_QCN9000:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCN6122
		case TARGET_TYPE_QCN6122:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCN9160
		case TARGET_TYPE_QCN9160:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCN6432
		case TARGET_TYPE_QCN6432:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCA5018
		case TARGET_TYPE_QCA5018:
			*target = WMI_TLV_TARGET;
			break;
#endif
#ifdef QCA_WIFI_QCA5332
                case TARGET_TYPE_QCA5332:
                        *target = WMI_TLV_TARGET;
                        break;
#endif
#ifdef QCA_WIFI_QCN9224
		case TARGET_TYPE_QCN9224:
			*target = WMI_TLV_TARGET;
			break;
#endif
		default:
			qdf_err("!!! Invalid Target Type %d !!!", target_type);
			return QDF_STATUS_E_INVAL;
	}
	return QDF_STATUS_SUCCESS;
}

static inline void ol_ath_configure_wmi_services(ol_ath_soc_softc_t *soc)
{
    uint8_t wmi_ep_count = 1;
    uint32_t target_type;
    struct target_psoc_info *tgt_psoc_info;
    void *hif_hdl;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return;
    }
    target_type = target_psoc_get_target_type(tgt_psoc_info);

    hif_hdl = lmac_get_ol_hif_hdl(soc->psoc_obj);
    /* qca8074 supports upto 3 macs and hence one ep per wmi.
     * But is preferred mode is DBS, then there is no need to connect to
     * all three WMI eps
     */
    if ((target_type == TARGET_TYPE_QCA8074) ||
        (target_type == TARGET_TYPE_QCA8074V2) ||
        (target_type == TARGET_TYPE_QCA9574) ||
        (target_type == TARGET_TYPE_QCA6018) ||
        (target_type == TARGET_TYPE_QCA5018) ||
        (target_type == TARGET_TYPE_QCA5332) ||
        (target_type == TARGET_TYPE_QCN6122) ||
        (target_type == TARGET_TYPE_QCN9160) ||
        (target_type == TARGET_TYPE_QCN9224) ||
        (target_type == TARGET_TYPE_QCN9000) ||
        (target_type == TARGET_TYPE_QCN6432)) {
        switch (target_psoc_get_preferred_hw_mode(tgt_psoc_info)) {
            case WMI_HOST_HW_MODE_SINGLE:
                wmi_ep_count = 1;
                break;
            case WMI_HOST_HW_MODE_DBS:
                wmi_ep_count = 2;
                break;
            case WMI_HOST_HW_MODE_DBS_OR_SBS:
                wmi_ep_count = 2;
                break;
            case WMI_HOST_HW_MODE_DBS_SBS:
                wmi_ep_count = 3;
                break;
            default:
                wmi_ep_count = hif_get_max_wmi_ep(hif_hdl);
                break;
        }
    }

    htc_set_wmi_endpoint_count(target_psoc_get_htc_hdl(tgt_psoc_info),
                            wmi_ep_count);
}

int htc_wmi_init(ol_ath_soc_softc_t *soc)
{
    struct htc_init_info htcInfo;
    struct target_psoc_info *tgt_psoc_info;
    HTC_HANDLE htc_handle;
    void *hif_hdl;
    /*
     * 5. Create HTC
     */
    OS_MEMZERO(&htcInfo,sizeof(htcInfo));
    htcInfo.pContext = soc;
    htcInfo.target_psoc = soc;
    if (soc->ol_if_ops->target_failure)
        htcInfo.TargetFailure = soc->ol_if_ops->target_failure;
    htcInfo.TargetSendSuspendComplete = ol_target_send_suspend_complete;
    /* Fetch the HTC Pipe Ready Timeout from INI (in msecs)*/
    htcInfo.htc_ready_timeout_ms = cfg_get(soc->psoc_obj,
                                          CFG_HTC_READY_TIMEOUT_MSEC);
    qdf_debug("HTC Pipe Ready Timeout %ums", htcInfo.htc_ready_timeout_ms);
    htc_global_credit_flow_disable();

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return -EIO;
    }

    hif_hdl = lmac_get_ol_hif_hdl(soc->psoc_obj);
    if ((htc_handle = htc_create(hif_hdl, &htcInfo, soc->qdf_dev,0)) == NULL)
    {
        return -EIO;
    }
    HTC_INFO("HT Create . %pK", htc_handle);

    target_psoc_set_htc_hdl(tgt_psoc_info, htc_handle);

    ol_ath_configure_wmi_services(soc);

    if (!hif_needs_bmi(hif_hdl)) {
        qdf_debug("Skipping BMI Done");
    } else {
#ifdef WLAN_FEATURE_BMI
        /*
         * 6. Complete BMI phase
         */
        if (soc->bmi_handle) {
            if (soc->bmi_handle->bmi_done) {
                if (soc->bmi_handle->bmi_done(hif_hdl, soc->bmi_handle,soc->sc_osdev)
                                              != A_OK)
                {
                    return -EIO;
                }
            }
        }
#endif
    }

    if (!bypasswmi) {
        /*
         * 7. Initialize WMI
         */

        /* initialize default target config */
        if (lmac_is_target_ar900b(soc->psoc_obj)) {
            /* Beeliner supports HW IP header alignment,
             * so native WIFI support is disabled and host receives 802.3 packets
             * with IP header aligned to 4-byte boundary.
             */
            wlan_psoc_nif_feat_cap_clear(soc->psoc_obj,
                                    WLAN_SOC_F_HOST_80211_ENABLE);
        }

        else if (target_psoc_get_target_type(tgt_psoc_info) == TARGET_TYPE_AR9888) {
            /* Peregrine does not support HW IP header alignment,
             * so native WIFI support should be enabled as IP header is always aligned
             * to 4-byte boundary in this mode.
             */
#if PERE_IP_HDR_ALIGNMENT_WAR
            wlan_psoc_nif_feat_cap_set(soc->psoc_obj,
                                    WLAN_SOC_F_HOST_80211_ENABLE);
#else
            wlan_psoc_nif_feat_cap_clear(soc->psoc_obj,
                                    WLAN_SOC_F_HOST_80211_ENABLE);
#endif
        }
    }

    HTC_INFO("host_enable %d",
              wlan_psoc_nif_feat_cap_get(soc->psoc_obj, WLAN_SOC_F_HOST_80211_ENABLE));
    target_if_set_default_config(soc->psoc_obj, tgt_psoc_info);

    return 0;
}

int ol_target_init_complete(ol_ath_soc_softc_t *soc)
{
    int status = 0;
    struct target_psoc_info *tgt_psoc_info;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    HTC_HANDLE htc_handle;
    void *hif_hdl;
    void *dbglog_handle = NULL;
    struct wmi_unified *wmi_handle = NULL;
    uint8_t num_radios;
    cdp_config_param_type val = {0};
#ifdef QCA_VDEV_STATS_HW_OFFLOAD_SUPPORT
    uint8_t qwrap_enable = 0;
#endif


    if (htc_wmi_init(soc))
        goto attach_failed;

    htc_handle = lmac_get_htc_hdl(soc->psoc_obj);
    if (!htc_handle)
        goto attach_failed;

    hif_hdl = lmac_get_ol_hif_hdl(soc->psoc_obj);
    if (!hif_hdl)
        goto attach_failed;

    soc_txrx_handle = wlan_psoc_get_dp_handle(soc->psoc_obj);
#ifdef WLAN_FEATURE_FASTPATH
    hif_enable_fastpath(hif_hdl);
#endif

    if (htc_wait_target(htc_handle) != QDF_STATUS_SUCCESS) {
        status = -EIO;
        goto attach_failed;
    }
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
        soc->nss_soc.ops->nss_soc_ce_post_recv_buffer(soc);
    }
#endif

    if (soc->ol_if_ops->cdp_soc_init) {
        soc_txrx_handle =
            soc->ol_if_ops->cdp_soc_init(wlan_psoc_get_dp_handle(soc->psoc_obj),
                                         soc->device_id, hif_hdl,
                                         (void *)soc->psoc_obj, htc_handle,
                                           (void *)soc->qdf_dev, &dp_ol_if_ops);
        if (!soc_txrx_handle) {
            qdf_err("soc attach failed");
            status = -EIO;
            goto attach_failed;
        }
        wlan_psoc_set_dp_handle(soc->psoc_obj, soc_txrx_handle);
    }

    /* max_ast_ageout_count is half of the maximum wmi queue size */
    cdp_soc_set_param(wlan_psoc_get_dp_handle(soc->psoc_obj),
                      DP_SOC_PARAM_MAX_AST_AGEOUT,
                      cfg_get(soc->psoc_obj, CFG_OL_MAX_WMI_CMDS) / 2);

#ifdef QCA_SUPPORT_EAPOL_OVER_CONTROL_PORT
    cdp_soc_set_param(wlan_psoc_get_dp_handle(soc->psoc_obj),
                      DP_SOC_PARAM_EAPOL_OVER_CONTROL_PORT,
                      cfg_get(soc->psoc_obj, CFG_OL_EAPOL_OVER_NL_ENABLE));
#endif

    ol_ath_soc_rate_stats_attach(soc);

    qdf_info("CDP soc attach success");

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
        soc->nss_soc.ops->nss_soc_wifi_set_cfg(soc, soc->nss_soc.nss_scfg);
    }
    qdf_info("Soc attach success NSS config %d ",
                cfg_get(soc->psoc_obj, CFG_NSS_WIFILI_OL));
#endif

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        goto attach_failed;
    }

    val.cdp_psoc_param_preferred_hw_mode =
            target_psoc_get_preferred_hw_mode(tgt_psoc_info);
    cdp_txrx_set_psoc_param(soc_txrx_handle, CDP_SET_PREFERRED_HW_MODE,
                            val);

    val.cdp_ast_indication_disable = soc->fw_ast_indication_disable;
    cdp_txrx_set_psoc_param(soc_txrx_handle, CDP_CFG_AST_INDICATION_DISABLE,
			    val);

#ifdef CONFIG_SAWF
    val.cdp_sawf_enabled = wlan_ucfg_get_config_param(soc->psoc_obj, SAWF);
    cdp_txrx_set_psoc_param(soc_txrx_handle, CDP_SAWF_ENABLE, val);
#endif

    if (!bypasswmi) {
        if (soc->ol_if_ops->dbglog_attach) {
            dbglog_handle = soc->ol_if_ops->dbglog_attach();
        }

        if (dbglog_handle) {
            target_psoc_set_dbglog_hdl(tgt_psoc_info, dbglog_handle);
            fwdbg_init(dbglog_handle, soc);
            soc->dbg_log_init = 1;
            wmi_handle = (struct wmi_unified *)target_psoc_get_wmi_hdl(tgt_psoc_info);
            if (wmi_handle) {
                wmi_unified_register_event_handler(wmi_handle, wmi_diag_event_id,
                        diag_fw_event_handler, WMI_RX_DIAG_WORK_CTX);
            } else {
                qdf_err("null wmi_handle");
            }

        } else {
            qdf_err("dbglog attach Failed");
        }
    }

    if ((status = ol_ath_connect_htc(soc)) != A_OK)
    {
        qdf_info("connect_htc failed");
        goto attach_failed;
    }

    num_radios = target_psoc_get_num_radios(tgt_psoc_info);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
        if ((status = soc->nss_soc.ops->nss_soc_wifi_attach(soc))) {
            qdf_err("nss_soc_wifi_attach failed");
            goto attach_failed;
        }
        if (soc->nss_soc.nss_nxthop && (status = soc->nss_soc.ops->nss_soc_wifi_set_default_nexthop(soc))) {
            qdf_err("nss_nxthop configuration failed");
            goto attach_failed;
        }
    }
#endif

#ifdef QCA_VDEV_STATS_HW_OFFLOAD_SUPPORT
    /**
      * If target is operating in QWRAP or WDS Extended mode, then
      * do not offload vdev stats collection to HW. Therefore set the value of
      * vdev_stats_hw_offload in dp_soc to 0 after successful dp_soc attach.
      */
#ifdef ATH_SUPPORT_WRAP
    qwrap_enable = wlan_rptr_psoc_is_max_qwrap(soc->psoc_obj);
#endif

    if (qwrap_enable) {
        val.cdp_psoc_param_vdev_stats_hw_offload = false;
        cdp_txrx_set_psoc_param(soc_txrx_handle,
                                CDP_CFG_VDEV_STATS_HW_OFFLOAD, val);
    }
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (!cfg_get(soc->psoc_obj, CFG_NSS_WIFI_OL)) {
#else
    if (target_psoc_get_target_type(tgt_psoc_info) != TARGET_TYPE_AR9888) {
#endif
        soc->num_tx_desc = cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_0);

	if (soc->num_tx_desc < cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_1))
            soc->num_tx_desc = cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_1);
	if (soc->num_tx_desc < cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_2))
            soc->num_tx_desc = cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_2);
	if (soc->num_tx_desc < cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_3))
            soc->num_tx_desc = cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_POOL_3);
    }
#if defined (QCA_LOWMEM_CONFIG) || defined (QCA_512M_CONFIG)
    if (ol_target_be(soc->psoc_obj))
        soc->num_tx_desc = cfg_get(soc->psoc_obj, CFG_DP_TX_DESC_LIMIT_0);
#endif

    /* initialize rf_path_ctx */
    ol_ath_init_deinit_rf_path_ctx(soc, true);

    if (lmac_get_tgt_version(soc->psoc_obj) != AR6004_VERSION_REV1_3) {
        if ((status = cdp_soc_attach_target(soc_txrx_handle)) != A_OK) {
            qdf_err("txrx soc attach failed");
            goto attach_failed;
        }
    }

    return 0;
attach_failed:
    return status;
}

/* This function is to be used by components to populate
 * the OL function pointers (tx_ops) required by the component
 * for UMAC-LMAC interaction, with the appropriate handler */
QDF_STATUS olif_register_umac_tx_ops(struct wlan_lmac_if_tx_ops *tx_ops)
{
    ol_ath_register_crypto_ops_handler(&tx_ops->crypto_tx_ops);
#ifdef WLAN_SUPPORT_FILS
    tx_ops->mgmt_txrx_tx_ops.fd_action_frame_send = target_if_fd_send;
#endif
    tx_ops->mgmt_txrx_tx_ops.mgmt_tx_send = ol_if_mgmt_send;
    tx_ops->mgmt_txrx_tx_ops.beacon_send = ol_ath_mgmt_beacon_send;
    tx_ops->mgmt_txrx_tx_ops.rx_frame_legacy_handler =
					ol_ath_mgmt_rx_frame_handler;
    tx_ops->scan.set_chan_list = ol_scan_set_chan_list;
    /* DFS function pointers */
#if ATH_SUPPORT_DFS
    tx_ops->dfs_tx_ops.dfs_enable = ol_if_dfs_enable;
    tx_ops->dfs_tx_ops.dfs_get_caps = ol_dfs_get_caps;
    tx_ops->dfs_tx_ops.dfs_gettsf64 = ol_if_get_tsf64;
    tx_ops->dfs_tx_ops.dfs_get_thresholds = ol_if_dfs_get_thresholds;
    tx_ops->dfs_tx_ops.dfs_disable = ol_if_dfs_disable;
    tx_ops->dfs_tx_ops.dfs_get_ext_busy = ol_if_dfs_get_ext_busy;
    tx_ops->dfs_tx_ops.dfs_get_target_type = lmac_get_pdev_target_type;
    tx_ops->dfs_tx_ops.dfs_get_ah_devid = ol_ath_get_ah_devid;
    tx_ops->dfs_tx_ops.dfs_get_phymode_info = ol_ath_get_phymode_info;
    tx_ops->dfs_tx_ops.dfs_get_persistent_nol_status = ol_ath_get_persistent_nol_info;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    tx_ops->dfs_tx_ops.dfs_host_dfs_check_support =
        ol_if_is_host_dfs_check_support_enabled;
#endif /* HOST_DFS_SPOOF_TEST */
    tx_ops->dfs_tx_ops.dfs_check_mode_switch_state =
        ol_if_hw_mode_switch_state;
#endif /* ATH_SUPPORT_DFS */

    /* Regulatory tx ops */
    tx_ops->afc_ops.extract_netdev = ol_ath_extract_netdev;
    tx_ops->reg_ops.set_wait_for_init_cc_response_event = ol_ath_set_wait_for_init_cc_response_event;
    tx_ops->reg_ops.fill_umac_legacy_chanlist = ol_ath_fill_umac_legacy_chanlist;
    tx_ops->mops.get_wifi_iface_id = ol_ath_get_interface_id;
    tx_ops->reg_ops.get_opclass_tbl_idx = ol_ath_get_opclass_tbl_idx;
    tx_ops->reg_ops.is_80p80_supported = ol_ath_pdev_is_80p80_supported;
    tx_ops->reg_ops.is_freq_80p80_supported = ol_ath_is_freq_80p80_supported;
#if ATH_SUPPORT_DFS
    tx_ops->reg_ops.init_dfs_nol = ol_ath_init_dfs_nol;
#endif
#ifdef CONFIG_AFC_SUPPORT
    tx_ops->reg_ops.trigger_acs_for_afc = ol_ath_trigger_acs_for_afc;
    tx_ops->reg_ops.trigger_update_channel_list = ol_ath_trigger_update_channel_list;
#if defined(CONFIG_AFC_SUPPORT) && defined(WLAN_FEATURE_11BE)
    tx_ops->reg_ops.reg_get_min_psd = ol_ath_fill_afc_min_psd;
#endif
#endif
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    tx_ops->mops.get_psoc_mlo_group_id = ol_ath_mlo_get_group_id;
    tx_ops->mops.get_psoc_mlo_chip_id = ol_ath_mlo_get_chip_id;
    tx_ops->mops.get_hw_link_id = target_if_pdev_get_hw_link_id;
    tx_ops->mops.target_if_mlo_setup_req = target_if_mlo_setup_req;
    tx_ops->mops.target_if_mlo_ready = target_if_mlo_ready;
    tx_ops->mops.target_if_mlo_teardown_req = target_if_mlo_teardown_req;
#endif
    tx_ops->reg_ops.end_r2p_table_update_wait = ol_ath_end_r2p_table_update_wait;
#ifdef CONFIG_SAWF
    tx_ops->sawf_tx_ops.sawf_svc_create_send = target_if_sawf_svc_create_send;
    tx_ops->sawf_tx_ops.sawf_svc_disable_send = target_if_sawf_svc_disable_send;
    tx_ops->sawf_tx_ops.sawf_ul_svc_update_send = target_if_sawf_ul_svc_update_params;
    tx_ops->sawf_tx_ops.sawf_update_ul_params = target_if_sawf_update_ul_params;
#endif

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(olif_register_umac_tx_ops);

/* This function is to be used by components to populate
 * the OL function pointer (crypto_rx_ops->get_rxpn)
 */
QDF_STATUS olif_register_umac_crypto_rxpn_ops(struct wlan_lmac_if_rx_ops *rx_ops)
{
    return ol_ath_register_crypto_rxpn_ops(&rx_ops->crypto_rx_ops);
}
qdf_export_symbol(olif_register_umac_crypto_rxpn_ops);

static QDF_STATUS ol_defaultkey(struct wlan_objmgr_vdev *vdev,
                                uint8_t keyidx, uint8_t *macaddr)
{
    if (!vdev) {
        qdf_err("vdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    ol_ath_wmi_send_vdev_param(vdev, wmi_vdev_param_def_keyid, keyidx);

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS ol_deletekey(struct wlan_objmgr_vdev *vdev,
                               struct wlan_crypto_key *key, uint8_t *macaddr,
                               uint32_t keytype)
{
    struct wlan_objmgr_pdev *pdev;
    struct pdev_osif_priv *osif_priv;
    struct ieee80211vap *vap;
    struct wlan_objmgr_psoc *psoc;
    uint8_t bssid_mac[IEEE80211_ADDR_LEN] = {0,0,0,0,0,0};

    if (!vdev) {
        qdf_err("vdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    pdev = wlan_vdev_get_pdev(vdev);

    if (!pdev) {
        qdf_err("pdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    osif_priv = wlan_pdev_get_ospriv(pdev);

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    psoc = wlan_pdev_get_psoc(pdev);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO, "Delkey");
    if (psoc && ol_target_lithium(psoc)) {
        if (IEEE80211_IS_MULTICAST(macaddr)) {
            qdf_mem_copy(bssid_mac, wlan_vdev_mlme_get_macaddr(vdev),
                         IEEE80211_ADDR_LEN);
            ol_ath_vdev_install_key_send(vap, key, bssid_mac, 0, 1, IEEE80211_CIPHER_NONE);
        } else {
            ol_ath_vdev_install_key_send(vap, key, macaddr, 0, 0, IEEE80211_CIPHER_NONE);
        }
    } else {
        ol_ath_vdev_install_key_send(vap, key, macaddr, 0, 1, IEEE80211_CIPHER_NONE);
    }
    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS ol_setkey(struct wlan_objmgr_vdev *vdev,
                            struct wlan_crypto_key *key,
                            uint8_t *macaddr,  uint32_t keytype){

    struct wlan_objmgr_pdev *pdev;
    struct pdev_osif_priv *osif_priv;
    struct ieee80211vap *vap;

    if (!vdev) {
        qdf_err("vdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    pdev = wlan_vdev_get_pdev(vdev);

    if (!pdev) {
        qdf_err("pdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    osif_priv = wlan_pdev_get_ospriv(pdev);

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }
#if ATH_SUPPORT_WRAP
    if (!wlan_rptr_vdev_is_key_set_allowed(vdev, key->flags)) {
        return QDF_STATUS_SUCCESS;
    }
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_CRYPTO, "setkey");

    return ol_ath_vdev_install_key_send(vap, key, macaddr,
                                 (key->flags & WLAN_CRYPTO_KEY_DEFAULT), 0,
                                  keytype);
}
static QDF_STATUS ol_allockey(struct wlan_objmgr_vdev *vdev,
                              struct wlan_crypto_key *key,
                              uint8_t *macaddr,  uint32_t keytype){
    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS ol_getpn(struct wlan_objmgr_vdev *vdev,
                            uint8_t *macaddr,  uint8_t keyix, uint32_t keytype){

    struct wlan_objmgr_pdev *pdev;
    struct pdev_osif_priv *osif_priv;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211vap *vap;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;

    if (!vdev) {
        qdf_err("vdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    pdev = wlan_vdev_get_pdev(vdev);

    if (!pdev) {
        qdf_err("pdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    osif_priv = wlan_pdev_get_ospriv(pdev);

    scn = (struct ol_ath_softc_net80211 *)(osif_priv->legacy_osif_priv);

    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);

    ret = ol_ath_vdev_getpn(vap, scn, wlan_vdev_get_id(vdev), macaddr, keyix, keytype);
    return ret;
}

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
static QDF_STATUS ol_set_ltf_keyseed(struct wlan_objmgr_psoc *psoc,
                                     struct wlan_crypto_ltf_keyseed_data *data)
{
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_vdev *vdev;
    wmi_unified_t wmi;
    QDF_STATUS status;

    if (!psoc) {
        target_if_err("psoc is null");
        return QDF_STATUS_E_INVAL;
    }
    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, data->vdev_id,
                                                WLAN_WIFI_POS_OSIF_ID);
    if (!vdev) {
        target_if_err("vdev is NULL");
        return QDF_STATUS_E_INVAL;
    }
    pdev = wlan_vdev_get_pdev(vdev);

    if (!pdev) {
        target_if_err("pdev is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
        return QDF_STATUS_E_FAILURE;
    }
    wmi = get_wmi_unified_hdl_from_pdev(pdev);

    if (!wmi) {
        target_if_err("wmi is null");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
        return QDF_STATUS_E_INVAL;
    }
    status = wmi_send_vdev_set_ltf_key_seed_cmd(wmi, data);
    if (QDF_IS_STATUS_ERROR(status))
        target_if_err("set LTF keyseed failed");

    wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
    return status;
}
#else
static QDF_STATUS ol_set_ltf_keyseed(struct wlan_objmgr_psoc *psoc,
                                     struct wlan_crypto_ltf_keyseed_data *data)
{
    return QDF_STATUS_SUCCESS;
}
#endif

static QDF_STATUS ol_get_rxpn(struct wlan_objmgr_vdev *vdev, uint8_t *macaddr,
                              uint16_t keyix){

    struct wlan_objmgr_pdev *pdev;
    struct pdev_osif_priv *osif_priv;
    struct ol_ath_softc_net80211 *scn;

    if (!vdev) {
        qdf_err("vdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        qdf_err("pdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    osif_priv = wlan_pdev_get_ospriv(pdev);

    scn = (struct ol_ath_softc_net80211 *)(osif_priv->legacy_osif_priv);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return ol_ath_vdev_get_rxpn(scn, wlan_vdev_get_id(vdev), macaddr, keyix);
}


/* OL callback to register crypto_ops handlers*/
static QDF_STATUS ol_ath_register_crypto_ops_handler(
                           struct wlan_lmac_if_crypto_tx_ops *crypto_tx_ops){

    crypto_tx_ops->setkey = ol_setkey;
    crypto_tx_ops->delkey = ol_deletekey;
    crypto_tx_ops->defaultkey = ol_defaultkey;
    crypto_tx_ops->allockey = ol_allockey;
    crypto_tx_ops->getpn = ol_getpn;
    crypto_tx_ops->set_ltf_keyseed = ol_set_ltf_keyseed;

    return QDF_STATUS_SUCCESS;
}

/* OL callback to register crypto rx_ops rxpn handler */
static QDF_STATUS ol_ath_register_crypto_rxpn_ops(
                struct wlan_lmac_if_crypto_rx_ops *crypto_rx_ops)
{
    crypto_rx_ops->get_rxpn = ol_get_rxpn;

    return QDF_STATUS_SUCCESS;
}

static
int unit_test_handler(ol_soc_t sc, uint8_t *data, uint32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    wmi_unit_test_event *unit_test;

    /*
     * Maximum buffer size from WMI is 2K
     */
    const uint32_t MAX_COPY_SIZE = 2048;
    const uint32_t MAX_BUFFER_SIZE = MAX_COPY_SIZE + sizeof(*unit_test);

    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_vdev *vdev;
    wlan_if_t vap;
    struct ieee80211req_athdbg_event *dbg_event;
    osif_dev *osifp;
    struct wmi_unified *wmi_handle;

    psoc = soc->psoc_obj;

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return 0;
    }

    unit_test = qdf_mem_malloc(sizeof(*unit_test)+MAX_BUFFER_SIZE);
    if (!unit_test) {
        QDF_TRACE(QDF_MODULE_ID_DEBUG, QDF_TRACE_LEVEL_ERROR,
                "%s: Memory allocation failed for wmi_unit_test_event\n",
                __func__);
        return 0;
    }
    wmi_extract_unit_test(wmi_handle, data, unit_test, MAX_COPY_SIZE);
    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, unit_test->vdev_id,
            WLAN_DEBUG_ID);
    if (!vdev) {
        QDF_TRACE(QDF_MODULE_ID_DEBUG, QDF_TRACE_LEVEL_ERROR,
                FL("vdev object NULL"));
        goto cleanup1;
    }
    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        QDF_TRACE(QDF_MODULE_ID_DEBUG, QDF_TRACE_LEVEL_ERROR,
                FL("vap object NULL"));
        goto cleanup2;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    dbg_event =
        qdf_mem_malloc(sizeof(*dbg_event) +
                unit_test->buffer_len);
    if (dbg_event == NULL) {
        QDF_TRACE(QDF_MODULE_ID_DEBUG, QDF_TRACE_LEVEL_ERROR,
                "%s: Memory allocation failed for ieee80211req_athdbg_event\n",
                __func__);
        goto cleanup2;
    }
    dbg_event->cmd = IEEE80211_DBGREQ_FW_UNIT_TEST;
    dbg_event->fw_unit_test.hdr.module_id = unit_test->module_id;
    dbg_event->fw_unit_test.hdr.flag = unit_test->flag;
    dbg_event->fw_unit_test.hdr.diag_token = unit_test->diag_token;
    dbg_event->fw_unit_test.hdr.payload_len = unit_test->payload_len;
    dbg_event->fw_unit_test.hdr.buffer_len = unit_test->buffer_len;
    qdf_mem_copy(dbg_event->fw_unit_test.buffer, unit_test->buffer,
            unit_test->buffer_len);
#if UMAC_SUPPORT_CFG80211
    wlan_cfg80211_generic_event(vap->iv_ic,
            QCA_NL80211_VENDOR_SUBCMD_DBGREQ, dbg_event,
            sizeof(*dbg_event) + (unit_test->buffer_len-1),
            osifp->netdev, GFP_KERNEL);
#endif
    qdf_mem_free(dbg_event);

cleanup2:
    wlan_objmgr_vdev_release_ref(vdev, WLAN_DEBUG_ID);
cleanup1:
    qdf_mem_free(unit_test);
    return 0;
}

static inline
bool tso_vdev_bit_status(struct ol_ath_soc_softc *soc)
{
    return !qdf_bitmap_empty(soc->tso_vdev_bitmap,
                             QDF_CHAR_BIT * sizeof(soc->tso_vdev_bitmap));
}

static void ath_device_cleanup_vap(osif_dev *osdev)
{
    wlan_if_t vap = osdev->os_if;
    struct net_device *parent = NULL;
    uint8_t num_vaps = 0;

    if (wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP &&
        !IEEE80211_IS_SPECIAL_VAP(vap)) {
        /* partner link's netdev will be same as current link in case of
         * single wiphy. hence dev_close on partner links not needed
         */
#if defined(WLAN_FEATURE_11BE_MLO) && !defined(ENABLE_CFG80211_BACKPORTS_MLO)
        osif_vap_netdev_close_mlo(vap->vdev_obj);
#endif /* WLAN_FEATURE_11BE_MLO && !ENABLE_CFG80211_BACKPORTS_MLO */
        osif_vap_netdev_close_mbssid(osdev);
#ifdef WLAN_FEATURE_11BE_MLO
        vap->ml_ap_removal_in_progress = false;
#endif
    }

    parent = osdev->os_comdev;
    num_vaps = osif_get_num_active_vaps(vap->iv_ic);
    if ((num_vaps == 0) && parent && (parent->flags & IFF_RUNNING)) {
        bool is_rtnl_locked = false;

        if (!rtnl_is_locked()) {
            rtnl_lock();
            is_rtnl_locked = true;
        }
        dev_close(parent);
        if (is_rtnl_locked) {
            rtnl_unlock();
        }
    }
}

/*
 * Handle changes in state of network device.
 *
 */
static int ath_device_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
    struct net_device *dev = netdev_notifier_info_to_dev(ptr);
#else
    struct net_device *dev = ptr;
#endif
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    osif_dev *osdev = NULL;
    wlan_if_t vap = NULL;
    uint8_t vdev_id;
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
    uint8_t i = 0;
    uint8_t num_vaps = 0;
#endif

    switch (event) {
        case NETDEV_FEAT_CHANGE:

            /* This api loops through all the vaps.
               Hence to avoid unnecessary handling of unrelated events
               this is called inside the event case
            */
            if (!(scn = osif_validate_and_get_scn_from_dev(dev))) {
                return NOTIFY_DONE;
            }

            osdev = ath_netdev_priv(dev);
            vap = osdev->os_if;
            vdev_id = wlan_vdev_get_id(vap->vdev_obj);
            soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

            if (!soc_txrx_handle)
                return 0;

            if (dev->features & NETIF_F_TSO) {
                if (!tso_vdev_bit_status(scn->soc))
                    cdp_tso_soc_attach(soc_txrx_handle);
                qdf_set_bit(vdev_id, scn->soc->tso_vdev_bitmap);
            } else if (!(dev->features & NETIF_F_TSO)) {
                qdf_clear_bit(vdev_id, scn->soc->tso_vdev_bitmap);
                if (!tso_vdev_bit_status(scn->soc))
                    cdp_tso_soc_detach(soc_txrx_handle);
            }
            break;
        case NETDEV_DOWN:
            /* Handle NETDEV_DOWN only for non-radio wdev's */
            if (!dev->ieee80211_ptr || (strncmp(dev->name, "wifi", 4) == 0))
                return NOTIFY_DONE;

#if defined(WLAN_FEATURE_11BE_MLO) && !defined(ENABLE_CFG80211_BACKPORTS_MLO)
            if ((strncmp(dev->name, "mld", 3) == 0))
                return NOTIFY_DONE;
#endif

#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            if (!(strncmp(dev->name, "mld", 3) == 0)) {
#endif
#if UMAC_SUPPORT_CFG80211
            osdev = get_vap_device_context(dev->ieee80211_ptr->wiphy, dev, INVALID_LINK_ID);
#else
            osdev = get_vap_cfg80211_context(NULL, dev, INVALID_LINK_ID);
#endif

            if (!osdev) {
                qdf_debug("NULL osdev");
                return NOTIFY_DONE;
            }

#ifdef QCA_SUPPORT_WDS_EXTENDED
            /* WDS EXT netdevice type needs to be returned */
            if (osdev->dev_type ==  OSIF_NETDEV_TYPE_WDS_EXT) {
                return NOTIFY_DONE;
            }
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            }
#endif
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            /**
             * In case of single wiphy, when mld netdev down comes,
             * links vifs are already stopped and deleted. So there is no
             * trace/deails of link vaps in the mld context. Hence check for
             * participating radios and check for num vaps and stop the
             * radio netdev. mbssid_stop to be checked later
             */
            if ((strncmp(dev->name, "mld", 3) == 0)) {
                struct cfg80211_context *cfg_ctx =
                         (struct cfg80211_context *)wiphy_priv(dev->ieee80211_ptr->wiphy);

                for (i = 0; i < NUM_MAX_RADIOS; i++) {
                     struct ieee80211com *ic = cfg_ctx->ic_list[i];

                     if (!ic)
                         continue;
                     num_vaps = osif_get_num_active_vaps(ic);
                     if ((num_vaps == 0) && (ic->ic_wdev.netdev) &&
                         (ic->ic_wdev.netdev->flags & IFF_RUNNING)) {
                         bool is_rtnl_locked = false;

                         if (!rtnl_is_locked()) {
                             rtnl_lock();
                             is_rtnl_locked = true;
                         }
                         dev_close(ic->ic_wdev.netdev);
                         if (is_rtnl_locked) {
                             rtnl_unlock();
                         }
                     }
                }
            } else {
#endif
                ath_device_cleanup_vap(osdev);
#ifdef ENABLE_CFG80211_BACKPORTS_MLO
            }
#endif
            break;
        default:
            break;

    }
    return NOTIFY_DONE;
}

/**
 * ol_ath_hw_mode_check_pdev_map(): Check if new pdev mapping is
 * possible or not for dynamic mode switch between DBS_SBS and DBS
 * @soc: soc handle
 *
 * return: QDF_STATUS_SUCCESS in case of success and
 * QDF_STATUS_E_INVAL otherwise
 */
static QDF_STATUS ol_ath_hw_mode_check_pdev_map(ol_ath_soc_softc_t *soc)
{
    struct wlan_objmgr_psoc *psoc        = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t    *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info *tgt_hdl;
    struct wlan_objmgr_pdev *pdev;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
    bool mac_phy_pdev_mapped[WMI_HOST_MAX_PDEV] = { 0 };
    int num_pdev_map = 0;
    uint8_t num_radios;
    int status = QDF_STATUS_E_INVAL;
    int i;
    int j;

    tgt_hdl = (struct target_psoc_info *)wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target handle is null");
        goto exit;
    }

    mac_phy_cap = target_psoc_get_mac_phy_cap_for_mode(tgt_hdl, hw_mode_ctx->target_mode);
    if (!mac_phy_cap) {
        qdf_err("mac_phy_cap is null");
        goto exit;
    }

    num_radios = target_psoc_get_num_radios_for_mode(tgt_hdl, hw_mode_ctx->target_mode);

    for (i = 0; i < WMI_HOST_MAX_PDEV; i++) {
        pdev = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_MLME_NB_ID);
        if (!pdev) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR,
                        FL("pdev object (id: %d) is NULL"), i);
            continue;
        }

        scn = (struct ol_ath_softc_net80211 *)
                                lmac_get_pdev_feature_ptr(pdev);
        if (!scn) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR, FL("scn is NULL"));

            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
            continue;
        }

        ic = &(scn->sc_ic);
        for (j = 0; j < num_radios; j++) {
            if (mac_phy_pdev_mapped[j])
                continue;

            if (hw_mode_ctx->target_mode == WMI_HOST_HW_MODE_DBS) {
                /* Map based on primary vs secondary interface and band capability */
                if ((pdev == hw_mode_ctx->primary_pdev) &&
                    (ic->ic_supported_bands & WMI_HOST_WLAN_5G_CAPABILITY) &&
                    (mac_phy_cap[j].supported_bands & WMI_HOST_WLAN_5G_CAPABILITY)) {
                    hw_mode_ctx->next_pdev_map[i] = mac_phy_cap[j].tgt_pdev_id;
                    mac_phy_pdev_mapped[j] = true;
                    break;
                } else if ((ic->ic_supported_bands & WMI_HOST_WLAN_2G_CAPABILITY) &&
                           (mac_phy_cap[j].supported_bands & WMI_HOST_WLAN_2G_CAPABILITY)) {
                    hw_mode_ctx->next_pdev_map[i] = mac_phy_cap[j].tgt_pdev_id;
                    mac_phy_pdev_mapped[j] = true;
                    break;
                }
            } else if (hw_mode_ctx->target_mode == WMI_HOST_HW_MODE_DBS_SBS) {
                /* Map based on operating frequency */
                if ((ic->ic_curchan->ic_freq >= mac_phy_cap[j].reg_cap_ext.low_5ghz_chan) &&
                    (ic->ic_curchan->ic_freq <= mac_phy_cap[j].reg_cap_ext.high_5ghz_chan) &&
                    (mac_phy_cap[j].supported_bands & WMI_HOST_WLAN_5G_CAPABILITY)) {
                    hw_mode_ctx->next_pdev_map[i] = mac_phy_cap[j].tgt_pdev_id;
                    mac_phy_pdev_mapped[j] = true;
                    break;
                } else if ((ic->ic_supported_bands & WMI_HOST_WLAN_2G_CAPABILITY) &&
                           (mac_phy_cap[j].supported_bands & WMI_HOST_WLAN_2G_CAPABILITY)) {
                    hw_mode_ctx->next_pdev_map[i] = mac_phy_cap[j].tgt_pdev_id;
                    mac_phy_pdev_mapped[j] = true;
                    break;
                }
            }
        }
        if (j == num_radios) {
            hw_mode_ctx->next_pdev_map[i] = WMI_HOST_PDEV_ID_INVALID;
            /* Map to a remaining pdev_id to avoid any crash due to invalid pdev_id */
            if (hw_mode_ctx->target_mode == WMI_HOST_HW_MODE_DBS) {
                hw_mode_ctx->next_pdev_map[i] = num_radios + 1;
            }
        }

        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
    }

    for (i = 0; i < WMI_HOST_MAX_PDEV; i++) {
        if (hw_mode_ctx->next_pdev_map[i] != WMI_HOST_PDEV_ID_INVALID)
            num_pdev_map++;
    }
    if (num_pdev_map < num_radios) {
        qdf_err("Couldn't map pdevs successfully num_pdev_map=%d num_radios=%d",
                num_pdev_map, num_radios);
        goto exit;

    }
    status = QDF_STATUS_SUCCESS;

exit:
    return status;
}

/**
 * ol_ath_hw_mode_apply_pdev_map(): Apply new pdev mapping
 * for dynamic mode switch between DBS_SBS and DBS
 * @soc: soc handle
 *
 * return: QDF_STATUS_SUCCESS in case of success and
 * QDF_STATUS_E_INVAL otherwise
 */
static QDF_STATUS ol_ath_hw_mode_apply_pdev_map (ol_ath_soc_softc_t *soc)
{
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info *tgt_hdl;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
    struct wmi_unified *wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    struct wlan_objmgr_pdev *pdev;
    uint8_t num_radios;
    int phy_idx;
    int lmac_id;
    int status = QDF_STATUS_E_INVAL;
    int i;

    tgt_hdl = (struct target_psoc_info *)wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target handle is null");
        goto exit;
    }

    mac_phy_cap = target_psoc_get_mac_phy_cap_for_mode(tgt_hdl, hw_mode_ctx->target_mode);
    if (!mac_phy_cap) {
        qdf_err("mac_phy_cap is null");
        goto exit;
    }

    num_radios  = target_psoc_get_num_radios_for_mode(tgt_hdl, hw_mode_ctx->target_mode);
    for (i = 0; i < WMI_HOST_MAX_PDEV; i++) {

        /* Update pdev_id mapping for WMI */
        hw_mode_ctx->pdev_map[i] = hw_mode_ctx->next_pdev_map[i];

        pdev = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_MLME_NB_ID);
        if (!pdev) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR,
                    FL("pdev object (id: %d) is NULL"), i);
            continue;
        }

        if (hw_mode_ctx->pdev_map[i] != WMI_HOST_PDEV_ID_INVALID) {
            /* Derive target phy_idx from mapped target pdev_id */
            phy_idx = hw_mode_ctx->pdev_map[i] - 1;
            /* Update wmi_handle in target_pdev_info */
            target_pdev_set_wmi_handle(
                wlan_pdev_get_tgt_if_handle(pdev),
                wmi_handle->soc->wmi_pdev[phy_idx]);
            /* Update phy_idx in target_pdev_info */
            target_pdev_set_phy_idx(
                wlan_pdev_get_tgt_if_handle(pdev),
                phy_idx);
            /* Derive target lmac_id from mapped target phy_idx */
            lmac_id = (phy_idx < num_radios)? mac_phy_cap[phy_idx].lmac_id : num_radios - 1;
            /* Update pdev_id mapping for DP */
            if (wlan_psoc_get_dp_handle(psoc)) {
		cdp_soc_handle_mode_change(wlan_psoc_get_dp_handle(psoc), i, lmac_id);
            }

            target_if_dbr_update_pdev_for_hw_mode_change(pdev, phy_idx);
        }

        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
    }

    if (wmi_handle) {
    /* Update pdev_id conversion table */
        wmi_pdev_id_conversion_enable((wmi_unified_t)wmi_handle, hw_mode_ctx->pdev_map, WMI_HOST_MAX_PDEV);
        status = QDF_STATUS_SUCCESS;
    }
exit:
    return status;
}

/**
  * ol_ath_allow_wmi_cmds(): Block WMI commands during a
  * HW mode change or allow WMI commands after the mode change.
  * @wmi_hdl: WMI handle of pdev
  * @allow: boolean to specify allow/block
  **/
static inline void ol_ath_allow_wmi_cmds(wmi_unified_t wmi_hdl, bool allow)
{
    /* stop or start based on 'allow' */
    if (wmi_hdl) {
        if (allow) {
	    wmi_start(wmi_hdl);
	} else {
	    wmi_stop(wmi_hdl);
	}
    }
}

int
ol_ath_soc_hw_mode_change_event_handler (ol_scn_t sc,
                                         u_int8_t *data,
                                         u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc           = (ol_ath_soc_softc_t *) sc;
    ol_ath_hw_mode_ctx_t *hw_mode_ctx = &soc->hw_mode_ctx;
    struct wmi_unified *wmi_handle;
    uint32_t fw_resp_status;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);

    if (wmi_handle) {
        /* Process return value from FW by extracting
         * response event params from "data". If SUCCESS,
         * set event. If FAILURE, don't set event and
         * command will time out.
         */
        if (wmi_unified_extract_hw_mode_resp(wmi_handle,
                    data, &fw_resp_status)
                        == QDF_STATUS_SUCCESS) {
            /*
             * FW status represents following values:
             *  0 - OK; command successful
             *  1 - EINVAL; Requested invalid hw_mode
             *  2 - ECANCELED; HW mode change canceled
             *  3 - ENOTSUP; HW mode not supported
             *  4 - EHARDWARE; HW mode change prevented by hardware
             *  5 - EPENDING; HW mode change is pending
             *  6 - ECOEX; HW mode change conflict with Coex
             *
             *  But host will have boolean interpretation
             *  of above. 0 means failure. 1 means success.
             *
             *  Note: In case FW sends EPENDING we will not
             *  release the event below and hw_mode_change
             *  handler will remain waiting till timeout.
             */
            hw_mode_ctx->is_fw_resp_success = !fw_resp_status;
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_INFO,
                      FL("Received response from FW with"""
                         """ status %s"),
                      hw_mode_ctx->is_fw_resp_success ? "PASS": "FAIL");
            if (fw_resp_status)
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                          QDF_TRACE_LEVEL_ERROR,
                          FL("fw_resp_status: %d"), fw_resp_status);

            if (hw_mode_ctx->is_fw_resp_success &&
                (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST)) {
                /* Apply pdev mapping */
                ol_ath_hw_mode_apply_pdev_map(soc);
                /* Re-start WMI cmd tx on primary pdev */
                ol_ath_allow_wmi_cmds(hw_mode_ctx->prev_wmi_hdl, true);
            }

            qdf_event_set(&hw_mode_ctx->event);
        }
    }

    return 0;
}

static void ol_ath_set_pdevid_to_phyid_map(ol_ath_soc_softc_t *soc)
{
    bool is_2g_phyb_enabled;
    struct target_psoc_info *tgt_psoc_info;
    uint32_t target_type;
    uint8_t phy_id_map[PSOC_MAX_PHY_REG_CAP] = {1, 0, 0};

    is_2g_phyb_enabled = cfg_get(soc->psoc_obj, CFG_OL_MODE_2G_PHYB);
    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    target_type = target_psoc_get_target_type(tgt_psoc_info);

    if (is_2g_phyb_enabled && target_type == TARGET_TYPE_QCA8074V2) {
	    target_psoc_set_pdev_id_to_phy_id_map(tgt_psoc_info, phy_id_map);
    }
}

/* INI rnr_6ghz_driver_override mask
 * bit 0 - continue to support the default behavior
 * bit 1 – override ap_rnr_field_size_limit when max_pp =1
 */
#define RNR_DRIVER_OVERRIDE_MASK     (0x01)
#define RNR_SIZE_LIMIT_OVERRIDE_MASK (0x02)
#define LOWERBAND_MBSSIE_ENABLE_MASK (0xffff)
#define DEFAULT_RESERVED_6GHZ_RNR (MLO_TBTT_INFO_FIELD_SIZE + 2*IE_LEN_ID_LEN)

void ol_ath_ema_init_ctx(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    ol_ath_soc_softc_t *soc;
    ol_ath_ema_pdev_ctx_t *ema_ctx;
    ol_ath_ema_ctx_t *soc_ema_ctx;

#if (!defined(QCA_LOWMEM_CONFIG) && !defined(QCA_512M_CONFIG))
    struct target_psoc_info *tgt_psoc_info;
    uint32_t target_type;
#endif
    uint32_t lowerband_mbss_ie_enable;

    scn = (struct ol_ath_softc_net80211 *)
            lmac_get_pdev_feature_ptr(pdev);

    if (!scn) {
         qdf_err("scn is NULL");
         return;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    soc = scn->soc;
    ema_ctx = &scn->ema_ctx;
    soc_ema_ctx = &soc->ema_ctx;

    /* Read configurations relevant to ema ap. Following params
     * will be required for calculating ema ap related resource
     * config in INIT cmd and need to be read prior to
     * ol_target_init_complete() call
     */
    soc_ema_ctx->max_mgmt_frm_sz = cfg_get(soc->psoc_obj,
                                CFG_OL_EMA_AP_MAX_MGMT_FRM_SZ);
    /* Limit ema_ctx->max_mgmt_frm_sz to 1.5K for
     * a. all low-mem platforms
     * b. HK, CYP and maple radios for which large beacon
     *    support is not enabled.
     */
    if (soc_ema_ctx->max_mgmt_frm_sz > mgmt_frm_size_1_5k) {
#if defined(QCA_LOWMEM_CONFIG) || defined(QCA_512M_CONFIG)
        soc_ema_ctx->max_mgmt_frm_sz = mgmt_frm_size_1_5k;
        target_if_info("Limit max_mgmt_frm_sz to 1.5K for low mem platforms");
#else
        tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
        target_type = target_psoc_get_target_type(tgt_psoc_info);
        if (target_type == TARGET_TYPE_QCA8074V2 ||
                target_type == TARGET_TYPE_QCA8074 ||
                target_type == TARGET_TYPE_QCA6018 ||
                target_type == TARGET_TYPE_QCA5018) {
            soc_ema_ctx->max_mgmt_frm_sz = mgmt_frm_size_1_5k;
            target_if_info("Limit max_mgmt_frm_sz to 1.5K for target_type:%d",target_type);
        }
#endif
    }

    if (soc_ema_ctx->max_mgmt_frm_sz == mgmt_frm_size_1_5k) {
        soc_ema_ctx->mbssie_headroom =
                                IEEE80211_EMA_MAX_HEADROOM_SIZE_FOR_1_5K_FRM;
    } else {
        soc_ema_ctx->mbssie_headroom =
                                IEEE80211_EMA_MAX_HEADROOM_SIZE_FOR_3_5K_FRM;
    }

    ema_ctx->beacon_common_part_size = cfg_get(soc->psoc_obj,
                                        CFG_OL_EMA_AP_BEACON_COMMON_PART_SIZE);

    ema_ctx->rnr_field_size_limit = cfg_get(soc->psoc_obj,
                                        CFG_OL_EMA_AP_RNR_FIELD_SIZE_LIMIT);

    ema_ctx->rnr_field_size_limit_override = cfg_get(soc->psoc_obj, CFG_OL_6GHZ_RNR_ADV_OVERRIDE);
    mbss_info("rnr_field_size_limit_override:%d", ema_ctx->rnr_field_size_limit_override);

    ema_ctx->rnr_field_size_limit_override = cfg_get(soc->psoc_obj,
                                        CFG_OL_6GHZ_RNR_ADV_OVERRIDE) & RNR_SIZE_LIMIT_OVERRIDE_MASK;
    mbss_info("rnr_field_size_limit_override &"
            " RNR_SIZE_LIMIT_OVERRIDE_MASK:>%d", ema_ctx->rnr_field_size_limit_override);

    ema_ctx->num_max_vaps = cfg_get(soc->psoc_obj,
                                        CFG_OL_EMA_AP_NUM_MAX_VAPS);

    ema_ctx->disable_6ghz_mbssid = false;

    soc_ema_ctx->feature_config = cfg_get(soc->psoc_obj,
                                         CFG_OL_MBSS_IE_ENABLE);

    lowerband_mbss_ie_enable = soc_ema_ctx->feature_config & LOWERBAND_MBSSIE_ENABLE_MASK;
    mbss_info("mbss_ie_enable :0x%x ",lowerband_mbss_ie_enable);

    ema_ctx->vendor_ie_config_low  = cfg_get(soc->psoc_obj,
                                        CFG_OL_EMA_AP_VENDOR_IE_SIZE_LOW);
    ema_ctx->vendor_ie_config_high = cfg_get(soc->psoc_obj,
                                        CFG_OL_EMA_AP_VENDOR_IE_SIZE_HIGH);

    ema_ctx->optional_ie_size =
        cfg_get(soc->psoc_obj, CFG_OL_EMA_AP_OPTIONAL_IE_SIZE);

    /* For WPS support in EMA in 6Ghz */
    ema_ctx->support_wps_6ghz = cfg_get(soc->psoc_obj,
                                        CFG_OL_EMA_AP_SUPPORT_WPS_IN_6GHZ);

    ema_ctx->max_non_tx_profile_sz_with_rsn =
        IEEE80211_MAX_NON_TX_PROFILE_SIZE_WITH_RSN;

#if QCA_SUPPORT_EMA_EXT
    /* Get ema-ext feature support bit from INI */
    ema_ctx->ext_enabled = cfg_get(soc->psoc_obj,
                                    CFG_OL_ENABLE_EMA_EXT);
    ema_ctx->mbss_split_profile_enabled = cfg_get(soc->psoc_obj,
                                    CFG_OL_SPLIT_NON_TX_PROFILE_ENABLED);
    /* common part of beacon can include the headroom bytes,
     * so set headroom to 0 */
    if (ema_ctx->ext_enabled) {
        soc_ema_ctx->mbssie_headroom = 0;
        ema_ctx->max_non_tx_profile_sz_with_rsn = 0;
    }
#else
    ema_ctx->ext_enabled = 0;
    ema_ctx->mbss_split_profile_enabled = 0;
#endif

    /* Promote ema_ctx->num_max_vaps to next power of 2 if not
     * already so. The rotation factor logic used in relation to
     * change of Tx-vap is dependent on the assumption that this
     * ema-param is power of 2
     */
    if (!QDF_IS_PWR2(ema_ctx->num_max_vaps))
        ema_ctx->num_max_vaps = roundup_pow_of_two(ema_ctx->num_max_vaps);

    /* Initialize max-ngroups to 1 */
    ema_ctx->max_ngroups = 1;

    /* Read INI value for max-pp */
    soc_ema_ctx->max_pp = cfg_get(soc->psoc_obj, CFG_OL_EMA_AP_MAX_PP);
    if ((soc_ema_ctx->max_pp > IEEE80211_ALLOWED_MAX_ODD_MAX_PP) &&
            (soc_ema_ctx->max_pp & 1)) {
         /* ema_max_pp algorithm requires max pp to be even
          * value so that intermediate current PP values can
          * be determined based on factors of max pp
          */
        soc_ema_ctx->max_pp++;
    }

    mbss_info("max_pp: %d", soc_ema_ctx->max_pp);

    qdf_assert(soc_ema_ctx->max_mgmt_frm_sz >
                  (ema_ctx->beacon_common_part_size +
                          soc_ema_ctx->mbssie_headroom));

    /* Max space available for a non-tx vap in a single beacon or
     * probe response frame
     */
    if (soc_ema_ctx->max_mgmt_frm_sz >
             (ema_ctx->beacon_common_part_size +
                          soc_ema_ctx->mbssie_headroom)) {
        ema_ctx->max_non_tx_sz = soc_ema_ctx->max_mgmt_frm_sz -
                                 (ema_ctx->beacon_common_part_size +
                                          soc_ema_ctx->mbssie_headroom);
    } else {
        ema_ctx->max_non_tx_sz = 0;
    }

    if (soc_ema_ctx->max_pp == 1 && !ema_ctx->rnr_field_size_limit_override && !lowerband_mbss_ie_enable) {
        /* If the mbssid-group is non-EMA then there is no
         * RNR requirement
         */
        ema_ctx->rnr_field_size_limit = 0;
    }

    if (ema_ctx->rnr_field_size_limit_override &&
            (ema_ctx->rnr_field_size_limit < DEFAULT_RESERVED_6GHZ_RNR)) {
        ema_ctx->rnr_field_size_limit = DEFAULT_RESERVED_6GHZ_RNR;
    }

    qdf_assert(soc_ema_ctx->max_mgmt_frm_sz >
                  (ema_ctx->beacon_common_part_size +
                      ema_ctx->rnr_field_size_limit +
                          soc_ema_ctx->mbssie_headroom));

    if (ema_ctx->max_non_tx_sz > ema_ctx->rnr_field_size_limit) {
        ema_ctx->max_non_tx_sz -= (ema_ctx->rnr_field_size_limit);
    } else {
        ema_ctx->max_non_tx_sz = 0;
    }

    /* Sanitize user configured vendor-ie config with respect
     * to number of vaps and allowed limit
     */
    ol_ath_ema_sanitize_vendor_ie_config(scn);

    /* Sanitize user configured optional-ie config with
     * respect to number of vaps and size remaining
     * after sanitizing vendor ies
     */
    ol_ath_ema_sanitize_optional_ie_config(scn);

    /* set max_non_tx_sz_limit using mandatory and user
     * configured limits
     */
    ema_ctx->max_non_tx_sz_limit =
        ema_ctx->max_non_tx_profile_sz_with_rsn +
        ema_ctx->vendor_ie_size_limit +
        ema_ctx->optional_ie_size_limit;

    mbss_info(" max_pp:%d  max_ngroups:%d ",soc_ema_ctx->max_pp, ema_ctx->max_ngroups);
    if (soc_ema_ctx->max_pp > EMA_AP_MAX_PP) {
        if (ema_ctx->max_ngroups < ema_ctx->num_max_vaps/2)
            ol_ath_ema_determine_max_pp(scn);
        /* else max_pp is 1 */
    } else {
        ol_ath_ema_determine_max_ngroup(scn);
    }

    /* Read configuration for overriding default RNR advertisement
     * behavior (default is selective advertisement only)
     */
    ema_ctx->rnr_6ghz_adv_override = cfg_get(soc->psoc_obj,
                                 CFG_OL_6GHZ_RNR_ADV_OVERRIDE) & RNR_DRIVER_OVERRIDE_MASK;
    mbss_info("rnr_6ghz_adv_override:%d",ema_ctx->rnr_6ghz_adv_override);

    ema_ctx->allow_mbss_cmn_all_vap = cfg_get(soc->psoc_obj,
                                  CFG_OL_ALLOW_MBSS_CMN_PARAM_ALL_VAP);

    soc->max_rnr_ie_allowed = cfg_get(soc->psoc_obj, CFG_OL_MAX_RNR_IE_ALLOWED);
}

int
ol_ath_pdev_get_dpd_status_event_handler(ol_soc_t sc,
                                         u_int8_t *data,
                                         u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_host_pdev_get_dpd_status_event event = {0};
    struct wmi_unified *wmi_handle;
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_pdev *pdev;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_extract_dpd_status_ev_param(wmi_handle, data, &event)) {
        qdf_err("Failed to extract dpd status event");
        return -1;
    }

    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj,
                                      event.pdev_id,
                                      WLAN_MLME_NB_ID);
    if (!pdev) {
        qdf_err("pdev object (id: %d) is NULL", event.pdev_id);
        return -1;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
        qdf_err("scn obj is NULL");
        return -1;
    }

    if (event.dpd_status)
        scn->dpdenable = CLI_DPD_STATUS_PASS;
    else
        scn->dpdenable = CLI_DPD_STATUS_FAIL;

    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
    return 0;
}

int
ol_ath_pdev_get_halphy_cal_status_event_handler(ol_soc_t sc,
                                         u_int8_t *data,
                                         u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_host_pdev_get_halphy_cal_status_event event = {0};
    struct wmi_unified *wmi_handle;
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_pdev *pdev;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_extract_halphy_cal_status_ev_param(wmi_handle, data, &event)) {
        qdf_err("Failed to extract halphy cal status event");
        return -1;
    }

    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj,
                                      event.pdev_id,
                                      WLAN_MLME_NB_ID);
    if (!pdev) {
        qdf_err("pdev object (id: %d) is NULL", event.pdev_id);
        return -1;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
        qdf_err("scn obj is NULL");
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
        return -1;
    }

    if (event.halphy_cal_ani_status)
        scn->is_ani_enable = TRUE;
    else
        scn->is_ani_enable = FALSE;

    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
    return 0;
}

#if QCA_MANUAL_TRIGGERED_ULOFDMA
/**
 * ol_ath_manual_ul_ofdma_trig_feedback_event_handler() - event handler
 * @soc: soc ctx
 * @data: event data
 * @datalen: data len
 *
 * Return : 0 on success or negative on failure
 */
static int
ol_ath_manual_ul_ofdma_trig_feedback_event_handler(ol_soc_t sc, u_int8_t *data,
                                                   u_int32_t datalen)
{
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;
    wmi_host_manual_ul_ofdma_trig_feedback_evt info = {0};
    struct wmi_unified *wmi_handle = NULL;
    wlan_if_t vap;
    int i = 0;
    uint8_t max_vdev_cnt;
    bool sc_mulofdma_support;

    soc = (ol_ath_soc_softc_t *)sc;
    psoc = soc->psoc_obj;
    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    sc_mulofdma_support = wmi_service_enabled(wmi_handle,
                                wmi_service_manual_ulofdma_trigger_support);

    if (!sc_mulofdma_support) {
        qdf_err("wmi_service_manual_ulofdma_trigger_support:%d",
                sc_mulofdma_support);
        return -EINVAL;
    }

    if (wmi_extract_ulofdma_trigger_feedback_event(wmi_handle, data, &info))
    {
        qdf_err("Failed to extract wmi_manual_ul_ofdma_trig_feedback_eventid");
        return -EINVAL;
    }

    max_vdev_cnt = wlan_psoc_get_max_vdev_count(psoc);
    if (info.vdev_id >= max_vdev_cnt) {
        qdf_err("invalid vdev_id:%u",info.vdev_id);
        return -EINVAL;
    }

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, info.vdev_id,
                                                WLAN_MLME_NB_ID);
    if (!vdev) {
        qdf_err("vdev is NULL");
        return -EINVAL;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    if (info.trigger_type == WMI_HOST_ULOFDMA_SU_TRIGGER) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                          "\n%s:curr_su_manual_trig=%u"
                          "remaining_su_manual_trig=%u\n", __func__,
                          info.u.su.curr_su_manual_trig,
                          info.u.su.remaining_su_manual_trig);

        if (info.u.su.curr_su_manual_trig != 0)
            scn->sc_mulofdma_ev_pending = false;
    }

    if (info.trigger_type == WMI_HOST_ULOFDMA_MU_TRIGGER) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                          "\n%s:remaining_mu_trig_peers=%u\n", __func__,
                          info.u.mu.remaining_mu_trig_peers);

        if (info.num_peer != 0)
            scn->sc_mulofdma_ev_pending = false;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                      "\n%s:vdev_id=%u, trigger_type=%u, manual_trig_status=%u"
                      "num_peer=%d\n",__func__, info.vdev_id, info.trigger_type,
                      info.manual_trig_status, info.num_peer);

    for (i = 0; i < info.num_peer; ++i) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                          "Peer_mac:%s\n", ether_sprintf(info.macaddr[i]));
    }

    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);
    return EOK;
}

/**
 * ol_ath_ul_ofdma_trig_rx_peer_userinfo_event_handler() - event handler
 * ul ofdma trig rx peer userinfo event handler
 * @soc: soc ctx
 * @data: event data
 * @datalen: data len
 *
 * Return : 0 on success or negative on failure
 */
int
ol_ath_ul_ofdma_trig_rx_peer_userinfo_event_handler(ol_soc_t sc, u_int8_t *data,
                                                    u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_unified *wmi_handle;
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_vdev *vdev = NULL;
    wlan_if_t vap;
    struct wmi_host_rx_peer_userinfo_evt_data resp = {0};
    struct wmi_host_rx_peer_userinfo *peer_resp;
    ol_man_trig_ulofdma_ctx *ulofdma_ctx;
    int i;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_extract_ul_ofdma_trig_rx_peer_userinfo(wmi_handle, data, &resp)) {
        qdf_err("Failed to extract rx userinfo event");
        return -EINVAL;
    }

    if (resp.comm_info.vdev_id >= wlan_psoc_get_max_vdev_count(soc->psoc_obj)) {
        qdf_err("invalid vdev_id:%u",resp.comm_info.vdev_id);
        return -EINVAL;
    }

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(soc->psoc_obj,
                               resp.comm_info.vdev_id, WLAN_MLME_NB_ID);

    if (!vdev) {
        qdf_err("vdev is NULL");
        return -EINVAL;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    if (!scn) {
        qdf_err("scn is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);
        return -EINVAL;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                      "\n%s:vdev_id:%d rx_ppdu_resp_type:%d num_peers:%d BW:%d"
                      "combined_rssi:%d\n", __func__,resp.comm_info.vdev_id,
                      resp.comm_info.rx_ppdu_resp_type,resp.comm_info.num_peers,
                      resp.comm_info.rx_resp_bw, resp.comm_info.combined_rssi);

    for (i = 0; i < resp.comm_info.num_peers; i++) {
        peer_resp = &resp.peer_info[i];
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                          "peer_mac:  " QDF_MAC_ADDR_FMT,
                          QDF_MAC_ADDR_REF(peer_resp->peer_mac.bytes));
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MANUAL_ULOFDMA_TRIG,
                          "resp_type:%d mcs:%d nss:%d gi_ltf_type:%d",
                          peer_resp->resp_type, peer_resp->mcs, peer_resp->nss,
                          peer_resp->gi_ltf_type);
    }

    ulofdma_ctx = &scn->ulofdma_ctx;
    if (ulofdma_ctx) {
        if (QDF_IS_STATUS_ERROR(ol_manual_trig_ulofdma_streamfs_send(
                                ulofdma_ctx,
                                (void *)&resp.comm_info,
                                sizeof(struct wmi_host_rx_peer_common_info),
                                (void *)&resp.peer_info,
                                (sizeof(struct wmi_host_rx_peer_userinfo) *
                                 resp.comm_info.num_peers)))) {
            qdf_err("manual trig ulofdma response relay failed");
        }
    } else {
        qdf_err("ulofdma_ctx is NULL");
    }
    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);

    return 0;
}
#endif /* QCA_MANUAL_TRIGGERED_ULOFDMA */

#ifdef QCA_RSSI_DB2DBM
int
ol_ath_pdev_rssi_dbm_conv_event_handler(ol_soc_t sc, u_int8_t *data,
                                        u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct rssi_db2dbm_param  event= {0};
    struct wmi_unified *wmi_handle;
    struct cdp_rssi_db2dbm_param_dp params = {0};
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(soc->psoc_obj);

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_extract_pdev_rssi_dbm_conv_ev_param(wmi_handle, data, &event)) {
        qdf_err("Failed to extract rssi_dbm event");
        return -EINVAL;
    }

    cp_stats_debug("RSSI_DB2DBM: pdev_id=%u, tmp_offset_present=%u, "
                   "rssi_param_present=%u, tmp_offset=%d, cur_bw=%u, "
                   "rx_chain=%u, xbar_config=%u, xlna_offset=%d, "
                   "xlna_threshold=%d", event.pdev_id,
                   event.rssi_temp_off_present, event.rssi_dbm_info_present,
                   event.temp_off_param.rssi_temp_offset,
                   event.rssi_dbm_param.curr_bw,
                   event.rssi_dbm_param.curr_rx_chainmask,
                   event.rssi_dbm_param.xbar_config,
                   event.rssi_dbm_param.xlna_bypass_offset,
                   event.rssi_dbm_param.xlna_bypass_threshold);
    qdf_mem_copy(&params, &event, sizeof(struct cdp_rssi_db2dbm_param_dp));
    cdp_set_params_rssi_dbm_conversion(soc_txrx_handle, &params);
    return EOK;
}
#endif

#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
QDF_STATUS ol_ath_read_shmem_addresses(ol_ath_soc_softc_t *soc)
{
    struct wlan_lmac_if_global_shmem_local_ops *shmem_local_ops;
    uint8_t mlo_chip_id;
    uint8_t grp_id;

    if (!soc) {
        qdf_err("soc  is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!pld_get_mlo_capable(soc->qdf_dev->dev))
        return QDF_STATUS_SUCCESS;

    shmem_local_ops = target_if_get_global_shmem_local_ops(soc->psoc_obj);

    if (!shmem_local_ops) {
        qdf_err("Low level ops of MGMT Rx REO is null");
        soc->crash_info_address = NULL;
        return QDF_STATUS_E_FAILURE;
    }

    grp_id = pld_get_mlo_group_id(soc->qdf_dev->dev);
    mlo_chip_id = pld_get_mlo_chip_id(soc->qdf_dev->dev);

    soc->crash_info_address =
            (uint32_t *)shmem_local_ops->get_crash_reason_address(grp_id, mlo_chip_id);
    soc->recovery_mode_address =
            (uint32_t *)shmem_local_ops->get_recovery_mode_address(grp_id, mlo_chip_id);

    return QDF_STATUS_SUCCESS;
}
#else
QDF_STATUS ol_ath_read_shmem_addresses(ol_ath_soc_softc_t *soc)
{
    return QDF_STATUS_SUCCESS;
}
#endif

static QDF_STATUS
ol_ath_register_custom_callbacks(ol_ath_soc_softc_t *soc)
{
    struct wlan_objmgr_psoc *psoc;
    HTC_HANDLE htc_handle;
    struct wmi_unified *wmi_handle;
    HTC_ENDPOINT_ID wmi_endpoint_id;

    if (!soc) {
        qdf_err("soc is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    psoc = soc->psoc_obj;
    if (!psoc) {
        qdf_err("psoc is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    htc_handle = lmac_get_htc_hdl(psoc);
    if (!htc_handle) {
        qdf_err("HTC handle is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
    if (!wmi_handle) {
        qdf_err("WMI handle is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    wmi_endpoint_id = wmi_get_endpoint(wmi_handle);

#ifdef WLAN_MGMT_RX_REO_SUPPORT
    if (wlan_mgmt_rx_reo_is_scheduler_enabled_at_psoc(psoc)) {
        QDF_STATUS status;

        status = htc_register_custom_cb(htc_handle, wmi_endpoint_id,
                                        target_if_mgmt_rx_reo_release_frames,
                                        soc);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("Failed to register mgmt rx reo custom cb");
            return status;
        }
    }
#endif /* WLAN_MGMT_RX_REO_SUPPORT */

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ol_ath_unregister_custom_callbacks(struct wlan_objmgr_psoc *psoc)
{
    HTC_HANDLE htc_handle;
    struct wmi_unified *wmi_handle;
    HTC_ENDPOINT_ID wmi_endpoint_id;

    if (!psoc) {
        qdf_err("psoc is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    htc_handle = lmac_get_htc_hdl(psoc);
    if (!htc_handle) {
        qdf_err("HTC handle is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    wmi_handle = get_wmi_unified_hdl_from_psoc(psoc);
    if (!wmi_handle) {
        qdf_err("WMI handle is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    wmi_endpoint_id = wmi_get_endpoint(wmi_handle);

#ifdef WLAN_MGMT_RX_REO_SUPPORT
    if (wlan_mgmt_rx_reo_is_scheduler_enabled_at_psoc(psoc)) {
        QDF_STATUS status;

        status = htc_unregister_custom_cb(htc_handle, wmi_endpoint_id);
        if (QDF_IS_STATUS_ERROR(status)) {
            qdf_err("Failed to unregister mgmt rx reo custom cb");
            return status;
        }
    }
#endif /* WLAN_MGMT_RX_REO_SUPPORT */

    return QDF_STATUS_SUCCESS;
}

#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
static int
ol_ath_ptqm_migration_event_handler(ol_scn_t sc, uint8_t *data,
                                    uint32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wmi_unified *wmi_handle;
    struct wlan_mlo_dev_context *ml_dev = NULL;
    struct peer_ptqm_migrate_event_params param = {0};
    struct peer_entry_ptqm_migrate_event_params entry = {0};
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_objmgr_peer *assoc_peer = NULL;
    struct ieee80211_node *assoc_ni = NULL;
    uint16_t num_peers = 0;
    uint8_t i = 0;
    int ret = 0;

    wlan_mlme_debug("PTQM migrate event received");
    if (!soc) {
        wlan_mlme_err("soc is NULL");
        return -EINVAL;
    }

    if (!data || datalen == 0) {
        wlan_mlme_err("Invalid data");
        return -EINVAL;
    }

    psoc = soc->psoc_obj;
    if (!psoc) {
        wlan_mlme_err("Psoc is NULL");
        return -EINVAL;
    }

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        wlan_mlme_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_peer_ptqm_migrate_event(wmi_handle, data, &param) !=
              QDF_STATUS_SUCCESS) {
        wlan_mlme_err("Failed to extract WMI params for PTQM Migrate event");
        return -EINVAL;
    }

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, param.vdev_id, WLAN_VDEV_TARGET_IF_ID);
    if (!vdev) {
        wlan_mlme_err("Failed to find vdev for id:%d", param.vdev_id);
        return -EINVAL;
    }

    if (!wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        wlan_mlme_debug("ptqm migrate event received for non mlo vdev");
        ret = -EINVAL;
        goto exit;
    }

    ml_dev = vdev->mlo_dev_ctx;
    if (!ml_dev) {
        wlan_mlme_err("MLD context is NULL");
        ret = -EINVAL;
        goto exit;
    }

    num_peers = param.num_peers;
    wlan_mlme_debug("Num peer entries in ptqm migrate event: %d", num_peers);
    if (num_peers == 0) {
        wlan_mlme_err("num peers is 0");
        ret = -EINVAL;
        goto exit;
    }

    for (i = 0; i < num_peers; i++) {
         if (QDF_IS_STATUS_SUCCESS(wmi_extract_peer_ptqm_entry_param(wmi_handle, data, i, &entry))) {
             wlan_mlme_debug("PTQM migrate completion received for ml peer id: %d status:%d",
                             entry.ml_peer_id, entry.status);
             ml_peer = wlan_mlo_get_mlpeer_by_ml_peerid(ml_dev, entry.ml_peer_id);
             if (!ml_peer || !ml_peer->primary_umac_migration_in_progress) {
                 QDF_ASSERT(0);
                 continue;
             }

             ml_peer->primary_umac_migration_in_progress = false;
             ml_peer->migrate_primary_umac_psoc_id = ML_PRIMARY_UMAC_ID_INVAL;
             qdf_clear_bit(entry.ml_peer_id, ml_dev->mlo_peer_id_bmap);

             wlan_ptqm_peer_migrate_completion(ml_dev, ml_peer, entry.status);

             if (entry.status == PRIMARY_LINK_PEER_MIGRATION_SUCCESS) {
                 assoc_peer = wlan_mlo_peer_get_assoc_peer(ml_peer);
                 if (assoc_peer) {
                     assoc_ni = wlan_peer_get_mlme_ext_obj(assoc_peer);
                     if (assoc_ni)
                         wlan_cfg80211_prim_netdev_notification(assoc_ni->ni_vap, assoc_peer);
                 }
             } else {
                 wlan_mlme_err("PTQM migrate failed for ml peer id: %d status:%d",
                               entry.ml_peer_id, entry.status);
                 wlan_mlo_peer_deauth_init(ml_peer, NULL, 0);
             }
          }
    }

    if (qdf_bitmap_empty(ml_dev->mlo_peer_id_bmap, MAX_MLO_PEER_ID)) {
        wlan_mlme_debug("PTQM migrate complete for all peers");
        qdf_timer_stop(&ml_dev->ptqm_migrate_timer);
    }
exit:
    if (vdev)
        wlan_objmgr_vdev_release_ref(vdev, WLAN_VDEV_TARGET_IF_ID);

    return ret;
}

static void ol_ath_ptqm_migration_attach(ol_ath_soc_softc_t *soc)
{
    wmi_unified_t wmi_handle;

    wmi_handle = lmac_get_wmi_unified_hdl(soc->psoc_obj);
    /* Register WMI event handlers */
    wmi_unified_register_event_handler(
             wmi_handle,
             wmi_peer_ptqm_migration_response_eventid,
             ol_ath_ptqm_migration_event_handler,
             WMI_RX_UMAC_CTX);
}
#else
static inline void ol_ath_ptqm_migration_attach(ol_ath_soc_softc_t *soc)
{ }
#endif

#if defined(IPA_OFFLOAD) && defined(IPA_OFFLOAD_LOW_MEM)
static bool
get_pumac_skip_flag(struct wlan_objmgr_psoc *psoc_obj)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    uint8_t num_radios, iter;
    qdf_freq_t low_2g, high_2g;

    num_radios = lmac_get_num_radios(psoc_obj);
    for (iter = 0; iter < num_radios; iter++) {
        pdev = wlan_objmgr_get_pdev_by_id(psoc_obj, iter, WLAN_IPA_ID);
        if (!pdev) {
            qdf_err("pdev object (id: %d) is NULL", iter);
            continue;
        }

        wlan_reg_get_freq_range(pdev, &low_2g, &high_2g, NULL, NULL);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_IPA_ID);
        if (wlan_reg_is_range_overlap_2g(low_2g, high_2g))
            return true;
    }

    return cfg_get(psoc_obj, CFG_OL_ML_PUMAC_SKIP);
}
#else
static bool
get_pumac_skip_flag(struct wlan_objmgr_psoc *psoc_obj)
{
    return cfg_get(psoc_obj, CFG_OL_ML_PUMAC_SKIP);
}
#endif

#ifdef CONFIG_SAWF
/**
 * ol_ath_is_sawf_msduq_reclaim_enabled() - Checks if msduq reclaim support is
 * present or not.
 *
 * @soc: soc ctx
 * @wmi_handle: WMI handle
 *
 * Return: False on failure / True on success
 */
static bool ol_ath_is_sawf_msduq_reclaim_enabled(ol_ath_soc_softc_t *soc,
                                                 wmi_unified_t wmi_handle)
{
    if (soc == NULL) {
        qdf_err("soc is NULL");
        return false;
    }

    if (wmi_handle == NULL) {
        qdf_err("wmi handle is NULL");
        return false;
    }

    return (cfg_get(soc->psoc_obj, CFG_DP_SAWF_MSDUQ_RECLAIM_ENABLE) &&
            wmi_service_enabled(wmi_handle, wmi_service_msduq_recfg));
}
#endif

int
ol_ath_soc_attach(ol_ath_soc_softc_t *soc,
                  IEEE80211_REG_PARAMETERS *ieee80211_conf_parm,
                  ol_ath_update_fw_config_cb cfg_cb)
{
    int status = 0;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;
    struct mgmt_txrx_mgmt_frame_cb_info rx_cb_info;
    struct target_psoc_info *tgt_psoc_info = NULL;
    void *wmi_handle = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
    struct wmi_unified_attach_params wmi_params = {0};
    HTC_HANDLE htc_handle;
    osdev_t osdev = NULL;
    uint32_t target_type;
    uint32_t recovery_type = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    bool eht_capab = false;
#endif
#if WLAN_SPECTRAL_ENABLE
    struct spectral_wmi_ops wmi_ops;
    struct spectral_tgt_ops tgt_ops;
#endif /* WLAN_SPECTRAL_ENABLE */
    uint8_t tgt_type = 0;
#ifdef DIRECT_BUF_RX_ENABLE
    enum wmi_rx_exec_ctx dbr_handler_ctx;
#endif
    bool mlo_pumac_skip = false;
#if defined(IPA_OFFLOAD) || defined(CONFIG_SAWF)
    cdp_config_param_type val = {0};
#endif
    wmi_params.osdev = soc->sc_osdev;
    wmi_params.psoc = soc->psoc_obj;
    wmi_params.use_cookie = 0;
    wmi_params.max_commands = cfg_get(soc->psoc_obj, CFG_OL_MAX_WMI_CMDS);
    wmi_params.soc_id = soc->soc_idx;
    wmi_params.is_async_ep = 1;
    soc->glb_shmem_init_complete = false;

    soc->cfg_cb = cfg_cb;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    recovery_type = soc->recovery_type;
#endif

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        goto attach_failed;
    }

    if (soc->ol_if_ops->target_init) {
        status = soc->ol_if_ops->target_init(soc, true);
	if (status != EOK) {
            goto attach_failed;
        }
    }

    /* Get the WMI target type - TLV or Non-TLV based on target */
    if (ol_ath_get_wmi_target_type(soc, &wmi_params.target_type)) {
                status = -1;
                goto attach_failed;
    }

    /* Event handlers and extract APIs and init cmd use soc->wmi_handle
     * and other commands uses pdev_scn->wmi_handle
     * basically, events are recieved on single CE and hence it is all mapped to
     * soc's wmi_handle...this is same as first pdev's wmi_handle.
     */
    if ((wmi_handle = wmi_unified_attach(soc, &wmi_params)) == NULL) {
        qdf_err("Failed to initialize WMI");
        status = -1;
        goto attach_failed;
    }

    target_psoc_set_wmi_hdl(tgt_psoc_info, wmi_handle);

    soc->wmi_diag_version = 0;
    HTC_INFO("WMI attached. wmi_handle %pK ", wmi_handle);
    /* Enable pdev_id conversion in WMI */
    wmi_pdev_id_conversion_enable((wmi_unified_t)wmi_handle, NULL, 0);

#if WMI_RECORDING
    wmi_proc_create(wmi_handle, soc->wmi_proc_entry, soc->soc_idx);
#endif

    if (dispatcher_psoc_open(soc->psoc_obj)) {
        qdf_err("Dispatcher open failed");
        status = -1;
        goto attach_failed;
    }

    target_type = lmac_get_tgt_type(soc->psoc_obj);

    osdev = get_osdev_from_psoc(soc->psoc_obj);

    if (target_type == TARGET_TYPE_QCN9224)
       if (osdev)
          pld_set_pci_link_speed_width(osdev->device, 3, 2);

    /* set wlan_psoc_mlme_set_11be_capab 1 for every 11be and further type target */
    if ((target_type == TARGET_TYPE_QCN9224) ||
            (target_type == TARGET_TYPE_QCA5332) ||
            (target_type == TARGET_TYPE_QCN6432)) {
        wlan_psoc_mlme_set_11be_capab(soc->psoc_obj, 1);
    } else {
        wlan_psoc_mlme_set_11be_capab(soc->psoc_obj, 0);
    }

#if WLAN_SPECTRAL_ENABLE
    wmi_ops.wmi_spectral_configure_cmd_send  = wmi_unified_vdev_spectral_configure_cmd_send;
    wmi_ops.wmi_spectral_enable_cmd_send     = wmi_unified_vdev_spectral_enable_cmd_send;
    wmi_ops.wmi_spectral_crash_inject        = wmi_crash_inject;
    wmi_ops.wmi_extract_pdev_sscan_fw_cmd_fixed_param   = wmi_extract_pdev_sscan_fw_cmd_fixed_param;
    wmi_ops.wmi_extract_pdev_sscan_fft_bin_index = wmi_extract_pdev_sscan_fft_bin_index;
    wmi_ops.wmi_unified_register_event_handler   = wmi_unified_register_event_handler;
    wmi_ops.wmi_unified_unregister_event_handler   = wmi_unified_unregister_event_handler;
    wmi_ops.wmi_service_enabled = wmi_service_enabled;
    wmi_ops.extract_pdev_spectral_session_chan_info = wmi_extract_pdev_spectral_session_chan_info;
    wmi_ops.extract_pdev_spectral_session_detector_info = wmi_extract_pdev_spectral_session_detector_info;
    wmi_ops.extract_spectral_caps_fixed_param = wmi_extract_spectral_caps_fixed_param;
    wmi_ops.extract_spectral_scan_bw_caps = wmi_extract_spectral_scan_bw_caps;
    wmi_ops.extract_spectral_fft_size_caps = wmi_extract_spectral_fft_size_caps;
    wlan_register_spectral_wmi_ops(soc->psoc_obj, &wmi_ops);

    tgt_ops.tgt_get_psoc_from_scn_hdl   = target_if_get_psoc_from_scn_hdl;
    wlan_register_spectral_tgt_ops(soc->psoc_obj, &tgt_ops);
#endif /* WLAN_SPECTRAL_ENABLE */

    init_deinit_register_tgt_psoc_ev_handlers(soc->psoc_obj);
    ol_ath_enable_pdev_map(soc);

#ifdef WLAN_SUPPORT_RF_CHARACTERIZATION
    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle,
                            wmi_chan_rf_characterization_info_event_id,
                            ol_ath_chan_rf_characterization_info_event_handler,
                            WMI_RX_UMAC_CTX);
#endif

    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_debug_print_event_id,
                                            ol_ath_debug_print_event_handler, WMI_RX_UMAC_CTX);
    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_peer_tx_pn_response_event_id,
                                           ol_ath_get_pn_event_handler, WMI_RX_UMAC_CTX);
    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_peer_rx_pn_response_event_id,
                                           ol_ath_get_rxpn_event_handler, WMI_RX_UMAC_CTX);

    /* Initialize FIPS WMI Event */
    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_pdev_fips_event_id,
                                           ol_ath_fips_event_handler, WMI_RX_UMAC_CTX);

#ifdef WLAN_FEATURE_FIPS_BER_CCMGCM
    /* Initialize FIPS WMI Event */
    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_pdev_fips_extend_event_id,
                                           ol_ath_fips_extend_event_handler, WMI_RX_UMAC_CTX);
#endif

    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_vdev_encrypt_decrypt_data_rsp_event_id,
                                           ol_ath_encrypt_decrypt_data_rsp_event_handler, WMI_RX_UMAC_CTX);

    wmi_unified_register_event_handler((wmi_unified_t)wmi_handle, wmi_tt_stats_event_id,
                          ol_ath_thermal_stats_event_handler, WMI_RX_UMAC_CTX);

    if (ol_target_lithium(soc->psoc_obj)) {
        osif_register_dev_ops_xmit(osif_ol_vap_hardstart_wifi3, OSIF_NETDEV_TYPE_OL_WIFI3);
#ifdef QCA_SUPPORT_WDS_EXTENDED
        if ((lmac_get_tgt_type(soc->psoc_obj) != TARGET_TYPE_QCA8074) &&
            cfg_get(soc->psoc_obj, CFG_OL_WDS_EXTENDED)) {
#if ATH_SUPPORT_WRAP
            if (!wlan_rptr_psoc_is_max_qwrap(soc->psoc_obj))
#endif /* ATH_SUPPORT_WRAP */
                {
                wlan_psoc_nif_feat_cap_set(soc->psoc_obj,
                                           WLAN_SOC_F_WDS_EXTENDED);
                /* set TX handle */
                osif_register_wds_ext_dev_ops_xmit(osif_wds_ext_peer_hardstart_wifi3, OSIF_NETDEV_TYPE_OL_WIFI3);
                /* set RX handle */
                soc->wds_ext_osif_rx = (ol_txrx_rx_fp)osif_deliver_wds_ext_data_ol;
                qdf_info("WDS Extended is enabled");
                }
#if ATH_SUPPORT_WRAP
            else
                qdf_err("WDS Ext can't be enabled along with qwrap");
#endif
            }
#endif /* QCA_SUPPORT_WDS_EXTENDED */
    } else {
        osif_register_dev_ops_xmit(osif_ol_ll_vap_hardstart, OSIF_NETDEV_TYPE_OL);
#if QCA_SUPPORT_WDS_EXTENDED && !(QCA_NSS_PLATFORM || QCA_PPE_VP)
        if (cfg_get(soc->psoc_obj, CFG_OL_WDS_EXTENDED)) {
#if ATH_SUPPORT_WRAP
            if (!wlan_rptr_psoc_is_max_qwrap(soc->psoc_obj))
#endif /* ATH_SUPPORT_WRAP */
            {
                wlan_psoc_nif_feat_cap_set(soc->psoc_obj,
                                           WLAN_SOC_F_WDS_EXTENDED);
                /* set TX handle */
                osif_register_wds_ext_dev_ops_xmit(osif_wds_ext_peer_hardstart_wifi2, OSIF_NETDEV_TYPE_OL);
                /* set RX handle */
                soc->wds_ext_osif_rx = (ol_txrx_rx_fp)osif_deliver_wds_ext_data_wifi2;
                qdf_info("WDS Extended is enabled");
            }
#if ATH_SUPPORT_WRAP
            else
                qdf_err("WDS Ext can't be enabled along with qwrap");
#endif
        }
#endif /* QCA_SUPPORT_WDS_EXTENDED */
    }
#ifdef WLAN_FEATURE_11BE_MLO
    wlan_psoc_mlme_get_11be_capab(soc->psoc_obj, &eht_capab);
    if (eht_capab) {
        osif_mldev_register_ops(true, cfg_get(soc->psoc_obj, CFG_OL_MLDEV_AP_MODE));
        osif_mldev_register_ops(false, cfg_get(soc->psoc_obj, CFG_OL_MLDEV_STA_MODE));
    }
#endif
    if (soc->ol_if_ops->cdp_soc_attach) {
        struct cdp_soc_attach_params params = {0};

        params.device_id = soc->device_id;
        params.hif_handle = lmac_get_ol_hif_hdl(soc->psoc_obj);
        params.htc_handle = lmac_get_htc_hdl(soc->psoc_obj);
        params.qdf_osdev = (void *)soc->qdf_dev;
        params.ol_ops = &dp_ol_if_ops;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
        if (pld_get_mlo_capable(soc->qdf_dev->dev)) {
            uint8_t grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);
            params.mlo_chip_id = pld_get_mlo_chip_id(soc->qdf_dev->dev);
            params.ml_context = (struct cdp_mlo_ctxt *)wlan_objmgr_get_dp_mlo_ctx(grp_id);
            params.mlo_enabled = 1;
        }
#endif
        soc_txrx_handle = soc->ol_if_ops->cdp_soc_attach
                          ((struct cdp_ctrl_objmgr_psoc *)soc->psoc_obj,
                           &params);
        if (!soc_txrx_handle) {
            qdf_err("soc attach failed");
            status = -EIO;
            goto attach_failed;
        }
#ifdef IPA_OFFLOAD
        val.cdp_ipa_enabled = ucfg_ipa_is_enabled();
        cdp_txrx_set_psoc_param(soc_txrx_handle, CDP_IPA_ENABLE, val);
#endif
        wlan_psoc_set_dp_handle(soc->psoc_obj, soc_txrx_handle);
    }

    ol_ath_set_pdevid_to_phyid_map(soc);

    if (ol_target_init_complete(soc)) {
        status = -1;
        goto target_init_failed;
    }

    ret = ol_ath_register_custom_callbacks(soc);
    if (QDF_IS_STATUS_ERROR(ret)) {
        qdf_err("Failed to register custom call backs");
        status = -1;
        goto target_init_failed;
    }

    ret = ol_ath_read_shmem_addresses(soc);
    if (QDF_IS_STATUS_ERROR(ret))
        qdf_err("Failed to read the crash reason address");

    if (lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCA8074 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCN6122 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCN9160 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCA8074V2 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCA9574 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCA5332 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCA5018 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCN9000 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCN9224 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCA6018 ||
        lmac_get_tgt_type(soc->psoc_obj) == TARGET_TYPE_QCN6432) {
        if (qca_register_netdevice_notifier(&qca_device_notifier))
            goto target_init_failed;
    }

#ifdef DIRECT_BUF_RX_ENABLE
    /* Adding handler context here */
    dbr_handler_ctx = cfg_get(soc->psoc_obj, CFG_DIRECT_DMA_HANDLER_CTX);
    ret = target_if_dbr_set_event_handler_ctx(soc->psoc_obj,
                          dbr_handler_ctx);
    if (QDF_IS_STATUS_ERROR(ret)) {
        qdf_err("Setting DBR event handler ctx failed");
    }
#endif

    /* UMAC Disatcher enable/start all components under PSOC */
    ret = dispatcher_psoc_enable(soc->psoc_obj);
    if (QDF_STATUS_SUCCESS != ret) {
        goto target_init_failed;
    }

#ifdef DIRECT_BUF_RX_ENABLE
    if (direct_buf_rx_target_attach(soc->psoc_obj,
                          hif_get_hal_handle(lmac_get_hif_hdl(soc->psoc_obj)),
                          soc->qdf_dev) != QDF_STATUS_SUCCESS) {
        qdf_err("Direct Buffer Rx Target Attach Failed");
    }
#endif

    /*
     * 8. Connect Services to HTC
     */
    qdf_debug("connect HTC");
    qdf_debug("bypasswmi : %d",bypasswmi);

#ifdef CONFIG_SAWF
    if (ol_ath_is_sawf_msduq_reclaim_enabled(soc, wmi_handle)) {
        /*
         * cdp_sawf_msduq_reclaim_enabled = True :
         * Initiate the SAWF MSDUQ Reclaim Timer
         */
        val.cdp_sawf_msduq_reclaim_enabled = true;
        cdp_txrx_set_psoc_param(soc_txrx_handle, CDP_SAWF_MSDUQ_RECLAIM_SUPPORT,
                                val);
    }
#endif

    if (!bypasswmi) {

        soc->max_clients = cfg_get(soc->psoc_obj, CFG_OL_MAX_CLIENTS);
        soc->max_vaps    = cfg_get(soc->psoc_obj, CFG_OL_MAX_VAPS);

#ifdef QVIT
        ol_ath_qvit_attach(soc);
#endif

        if(soc->ol_if_ops->wds_addr_init) {
            soc->ol_if_ops->wds_addr_init((wmi_unified_t)wmi_handle);
        }

        wmi_unified_register_event_handler(wmi_handle, wmi_unit_test_event_id,
                unit_test_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_wlan_profile_data_event_id,
                                                ol_ath_wlan_profile_data_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_tpc_config_event_id,
                                                ol_ath_pdev_tpc_config_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_gpio_input_event_id,
                                                ol_ath_gpio_input_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_nfcal_power_all_channels_event_id,
                                                ol_ath_nf_dbr_dbm_info_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_tpc_event_id,
                                                ol_ath_packet_power_info_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_generic_buffer_event_id,
                                                ol_ath_generic_buffer_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_peer_tx_mu_txmit_count_event_id,
                                                ol_ath_peer_mumimo_tx_count_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_peer_gid_userpos_list_event_id,
                                                ol_ath_peer_gid_userpos_list_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_check_cal_version_event_id,
                                                ol_ath_pdev_caldata_version_check_event_handler, WMI_RX_UMAC_CTX);
#if QCN_ESP_IE
        wmi_unified_register_event_handler(wmi_handle, wmi_esp_estimate_event_id,
                                                ol_ath_esp_estimate_event_handler, WMI_RX_UMAC_CTX);
#endif
        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_ctl_failsafe_check_event_id,
                                                ol_ath_pdev_ctl_failsafe_check_event_handler, WMI_RX_UMAC_CTX);

        if (ol_ath_is_dynamic_hw_mode_enabled(soc)) {
            wmi_unified_register_event_handler(wmi_handle, wmi_pdev_set_hw_mode_rsp_event_id,
                                                ol_ath_soc_hw_mode_change_event_handler, WMI_RX_UMAC_CTX);
        }

        wmi_unified_register_raw_event_handler(wmi_handle, wmi_pdev_cp_fwstats_eventid,
                                               ol_ath_cp_fwstats_event_handler, WMI_RX_UMAC_CTX);
        wmi_unified_register_raw_event_handler(wmi_handle, wmi_pdev_halphy_fwstats_eventid,
                                               ol_ath_halphy_fwstats_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_vdev_tsf_report_event_id,
                                           ol_ath_vdev_tsf_report_event_handler, WMI_RX_UMAC_CTX);
        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_get_dpd_status_event_id,
                                           ol_ath_pdev_get_dpd_status_event_handler, WMI_RX_UMAC_CTX);

        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_set_halphy_cal_event_id,
                                           ol_ath_pdev_set_halphy_cal_event_handler, WMI_RX_UMAC_CTX);
        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_get_halphy_cal_status_event_id,
                                           ol_ath_pdev_get_halphy_cal_status_event_handler, WMI_RX_UMAC_CTX);
#if QCA_STANDALONE_SOUNDING_TRIGGER
        wmi_unified_register_event_handler(wmi_handle, wmi_vdev_standalone_sound_complete_eventid,
                                           ol_ath_txbf_standalone_sounding_complete_event_handler,
                                           WMI_RX_UMAC_CTX);
#endif

#if QCA_MANUAL_TRIGGERED_ULOFDMA
        wmi_unified_register_event_handler(wmi_handle,
                                           wmi_manual_ul_ofdma_trig_feedback_eventid,
                                           ol_ath_manual_ul_ofdma_trig_feedback_event_handler,
                                           WMI_RX_UMAC_CTX);
        wmi_unified_register_event_handler(wmi_handle,
                                           wmi_manual_ul_ofdma_trig_rx_peer_userinfo_eventid,
                                           ol_ath_ul_ofdma_trig_rx_peer_userinfo_event_handler,
                                           WMI_RX_UMAC_CTX);
#endif
#ifdef QCA_RSSI_DB2DBM
        if (wmi_service_enabled(wmi_handle,
                                wmi_service_pdev_rssi_dbm_conv_event_support))
            wmi_unified_register_event_handler(wmi_handle,
                                               wmi_pdev_rssi_dbm_conversion_params_info_eventid,
                                               ol_ath_pdev_rssi_dbm_conv_event_handler,
                                               WMI_RX_UMAC_CTX);
#endif
        wmi_unified_register_event_handler(wmi_handle, wmi_pdev_set_rf_path_resp_eventid,
                                           ol_ath_soc_rf_path_event_handler, WMI_RX_UMAC_CTX);

#if !defined(WLAN_TWT_CONV_SUPPORTED)
        ol_ath_soc_twt_attach(soc);
#endif
        ol_ath_soc_dcs_attach(soc);

        ol_ath_node_soc_attach(soc);
        ol_ath_phyerr_attach(soc);
        ol_ath_stats_soc_attach(soc);
        ol_ath_soc_chan_info_attach(soc);
#ifdef WLAN_FEATURE_11BE
        ol_ath_sched_probe_resp_attach(soc);
#endif
        ol_ath_vap_soc_attach(soc);
        ol_ath_wow_soc_attach(soc);
        ol_ath_beacon_soc_attach(soc);
        ol_ath_mgmt_soc_attach(soc);
        ol_ath_ptqm_migration_attach(soc);
    }

   tgt_type = lmac_get_tgt_type(soc->psoc_obj);

#if WLAN_SUPPORT_MSCS && QCA_NSS_PLATFORM
  /*
   * Initialize MSCS module
   */
    if (tgt_type == TARGET_TYPE_QCA8074 ||
            tgt_type == TARGET_TYPE_QCA8074V2 ||
            tgt_type == TARGET_TYPE_QCA9574 ||
            tgt_type == TARGET_TYPE_QCA5018 ||
	    tgt_type == TARGET_TYPE_QCA5332 ||
	    tgt_type == TARGET_TYPE_QCN6432 ||
            tgt_type == TARGET_TYPE_QCN9000 ||
            tgt_type == TARGET_TYPE_QCN9224 ||
            tgt_type == TARGET_TYPE_QCA6018) {
        qca_mscs_module_init(soc);
    }
#endif

    /* MLO PUMAC-SETTING */
    wlan_psoc_set_pumac_skip(soc->psoc_obj, false );
    mlo_pumac_skip = get_pumac_skip_flag(soc->psoc_obj);

    if (mlo_pumac_skip) {
        wlan_psoc_set_pumac_skip(soc->psoc_obj, true);
        qdf_info("PUMAC skip");
    }

    qdf_debug("UMAC attach");

    /*
     * Register Rx callback with mgmt_txrx layer for receiving
     * mgmt frames.
     */
    rx_cb_info.frm_type = MGMT_FRAME_TYPE_ALL;
    rx_cb_info.mgmt_rx_cb = ieee80211_mgmt_input;
    wlan_mgmt_txrx_register_rx_cb(soc->psoc_obj, WLAN_UMAC_COMP_MLME,
                                       &rx_cb_info, 1);

#if QLD
    qdf_event_create(&soc->qld_wait);
#endif
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    qdf_event_create(&soc->fallback_event);
#endif
#if QCA_11AX_STUB_SUPPORT
    if (1 == OL_ATH_IS_11AX_STUB_ENABLED(soc)) {
        qdf_info("%s: 802.11ax stubbing enabled",
                soc->sc_osdev->netdev->name);
    }
#endif /* QCA_11AX_STUB_SUPPORT */

    soc->pn_rxfilter = 0;

    return EOK;

target_init_failed:
    dispatcher_psoc_close(soc->psoc_obj);
    if (soc->glb_shmem_init_complete)
        ol_mlo_glb_h_shmem_arena_ctx_deinit(soc);

attach_failed:
    qdf_err("error status %d", status);

#if WLAN_SUPPORT_MSCS && QCA_NSS_PLATFORM
  /*
   * De-Init MSCS module
   */
    if (tgt_type == TARGET_TYPE_QCA8074 ||
            tgt_type == TARGET_TYPE_QCA8074V2 ||
            tgt_type == TARGET_TYPE_QCA9574 ||
            tgt_type == TARGET_TYPE_QCA5018 ||
	    tgt_type == TARGET_TYPE_QCA5332 ||
	    tgt_type == TARGET_TYPE_QCN6432 ||
            tgt_type == TARGET_TYPE_QCN9000 ||
            tgt_type == TARGET_TYPE_QCN9224 ||
            tgt_type == TARGET_TYPE_QCA6018) {
        qca_mscs_module_deinit(soc);
    }
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
        qdf_info("nss-wifi: detach -zero delay ");
        soc->nss_soc.ops->nss_soc_wifi_detach(soc, 0);
    }
#endif
    init_deinit_free_num_units(soc->psoc_obj, tgt_psoc_info);

    htc_handle=target_psoc_get_htc_hdl(tgt_psoc_info);
    if (htc_handle) {
        htc_destroy(htc_handle);
        target_psoc_set_htc_hdl(tgt_psoc_info, NULL);
    }

#ifdef WLAN_FEATURE_BMI
    if (soc->bmi_handle) {
        if (soc->bmi_handle->bmi_cleanup)
            soc->bmi_handle->bmi_cleanup(soc->bmi_handle,soc->sc_osdev);
        if (soc->bmi_handle->bmi_free)
            soc->bmi_handle->bmi_free(soc->bmi_handle);
    }
#endif
    if (!bypasswmi) {
        if ((wmi_unified_t)wmi_handle) {
            wmi_unified_detach((wmi_unified_t)wmi_handle);
#if WMI_RECORDING
            wmi_proc_remove((wmi_unified_t)wmi_handle, soc->wmi_proc_entry, soc->soc_idx);
#endif
            wmi_handle = NULL;
            target_psoc_set_wmi_hdl(tgt_psoc_info, NULL);

        }
        soc_txrx_handle = wlan_psoc_get_dp_handle(soc->psoc_obj);
        if (soc_txrx_handle) {
            cdp_txrx_umac_reset_deinit(soc_txrx_handle, recovery_type);
            cdp_txrx_intr_detach(soc_txrx_handle);

            wlan_psoc_set_dp_handle(soc->psoc_obj, NULL);
            cdp_soc_detach(soc_txrx_handle);
#ifdef WLAN_FEATURE_FASTPATH
            soc->htt_handle = NULL;
#endif
        }

	if (soc->dbg_log_init) {
	    ol_ath_dbglog_detach(soc);
	    soc->dbg_log_init = 0;
	}
    }
#ifdef AH_CAL_IN_FLASH_PCI
#ifndef ATH_CAL_NAND_FLASH
    if (soc->cal_mem) {
        A_IOUNMAP(soc->cal_mem);
        soc->cal_mem = 0;
    }
#endif
    soc->cal_in_flash = 0;
#endif
    soc->cal_in_file = 0;
    soc->soc_lp_iot_vaps_mask = 0;

    return status;
}

QDF_STATUS ol_ath_notify_asserted_soc(struct ol_ath_soc_softc *soc)
{
    ol_txrx_soc_handle soc_txrx_handle;

    if (!soc) {
        qdf_err("Soc is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(soc->psoc_obj);

    if (!soc_txrx_handle) {
        qdf_err("Soc txrx handle is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    return cdp_notify_asserted_soc(soc_txrx_handle);
}

QDF_STATUS wlan_scan_register_callbacks(struct wlan_objmgr_psoc *psoc)
{
    /* scan entry update callback */
    ucfg_scan_register_bcn_cb(psoc,
        wlan_scan_cache_update_callback, SCAN_CB_TYPE_UPDATE_BCN);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_scan_unregister_callbacks(struct wlan_objmgr_psoc *psoc)
{
    /* scan entry update callback */
    ucfg_scan_register_bcn_cb(psoc, NULL, SCAN_CB_TYPE_UPDATE_BCN);

    return QDF_STATUS_SUCCESS;
}

static int ol_ath_pdev_regdmn_init(struct ol_ath_softc_net80211 *scn,
                                IEEE80211_REG_PARAMETERS *ieee80211_conf_param)
{
    ol_ath_soc_softc_t *soc = scn->soc;
    uint8_t pdev_idx;
    struct ieee80211com *ic;
    int status = 0;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    struct wlan_objmgr_pdev *pdev = scn->sc_pdev;
    struct target_pdev_info *tgt_pdev;
    struct target_psoc_info *tgt_hdl;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
    struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2;
    struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
    struct wlan_psoc_target_capability_info *target_cap;
    struct wmi_unified *wmi_handle;
    uint8_t reg_cap_phy_idx;
    bool eht_capab = false;

    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        return -EINVAL;
    }

    tgt_pdev = (struct target_pdev_info *)wlan_pdev_get_tgt_if_handle(pdev);
    if (!tgt_pdev) {
        target_if_err("target_pdev_info is null");
        return -EINVAL;
    }

    pdev_idx = target_pdev_get_pdev_idx(tgt_pdev);
    if (pdev_idx < 0) {
        qdf_err("pdev idx is invalid");
        return -EINVAL;
    }

    reg_cap_phy_idx = target_pdev_get_phy_idx(tgt_pdev);
    if (reg_cap_phy_idx < 0) {
        qdf_err("reg_cap_phy_idx is invalid");
        return -EINVAL;
    }

    target_cap  = target_psoc_get_target_caps(tgt_hdl);
    mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);
    mac_phy_caps_ext2 = target_psoc_get_mac_phy_cap_ext2(tgt_hdl);
    reg_cap     = ucfg_reg_get_hal_reg_cap(psoc);

    if (!reg_cap)
        return -EINVAL;

    if (!mac_phy_cap)
        qdf_err("mac_phy_cap is NULL");

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle)
        return -EINVAL;

    ic = &scn->sc_ic;

    /* If extended service ready is supported by firmware, Use mac_phy
     * capabilities provided by extended service ready event instead of
     * service ready event. Extended service ready will override caps
     * recieved in service ready
     */
    if (wmi_service_enabled(wmi_handle, wmi_service_ext_msg) && mac_phy_cap) {
        target_if_info(" phy id = %d Modes supported", pdev_idx);
        target_if_info(" 11b = %d 11g = %d 11a = %d 11n = %d 11ac = %d 11ax = %d",
                mac_phy_cap[pdev_idx].supports_11b,
                mac_phy_cap[pdev_idx].supports_11g,
                mac_phy_cap[pdev_idx].supports_11a,
                mac_phy_cap[pdev_idx].supports_11n,
                mac_phy_cap[pdev_idx].supports_11ac,
                mac_phy_cap[pdev_idx].supports_11ax);
        wlan_psoc_mlme_get_11be_capab(psoc, &eht_capab);
#ifdef WLAN_FEATURE_11BE
        if (eht_capab) {
            target_if_info(" 11be = %d",
                    mac_phy_cap[pdev_idx].supports_11be);
        }
#endif /* WLAN_FEATURE_11BE */

        target_if_info(" Reg cap - phy_id = %d supp_bnd = %x, modes = %llx, "
                "lo_2ghz = %d, hi_2ghz = %d lo_5ghz = %d, hi_5ghz = %d", pdev_idx,
                mac_phy_cap[pdev_idx].supported_bands,
                reg_cap[reg_cap_phy_idx].wireless_modes,
                reg_cap[reg_cap_phy_idx].low_2ghz_chan,
                reg_cap[reg_cap_phy_idx].high_2ghz_chan,
                reg_cap[reg_cap_phy_idx].low_5ghz_chan,
                reg_cap[reg_cap_phy_idx].high_5ghz_chan);

        ol_ath_update_wireless_modes(ic, &reg_cap[reg_cap_phy_idx], &mac_phy_cap[pdev_idx]);

        ol_ath_update_target_cap_from_mac_phy_cap(target_cap,
                                      &mac_phy_cap[pdev_idx]);
        ol_ath_update_ext_caps(&mac_phy_cap[pdev_idx], ic);
        if (mac_phy_caps_ext2 && eht_capab)
            ol_ath_update_ext_eht_caps(&mac_phy_cap[pdev_idx], &mac_phy_caps_ext2[pdev_idx], ic);
        /* update supprted bands from mac_phy_cap */
        ic->ic_supported_bands = mac_phy_cap[pdev_idx].supported_bands;
        ic->ic_tgt_defined_nss_ratio_enabled = mac_phy_cap[pdev_idx].nss_ratio_enabled;
        ic->ic_tgt_defined_nss_ratio = mac_phy_cap[pdev_idx].nss_ratio_info;
    }

    ol_ath_update_caps(ic, target_cap);

    ol_ath_update_6g_band_caps(ic);

    if (mac_phy_cap) {
        if (mac_phy_cap[pdev_idx].supported_bands &
            WMI_HOST_WLAN_5G_CAPABILITY) {
           if ((mac_phy_cap[pdev_idx].he_cap_phy_info_5G[0]) &
               IEEE80211_HECAP_LDPC_MASK)
               ieee80211com_set_ldpccap(ic, IEEE80211_HECAP_C_LDPC_TXRX);
           else
               qdf_debug("LDPC not supported in 5G HE CAP");
        }

        if (mac_phy_cap[pdev_idx].supported_bands &
            WMI_HOST_WLAN_2G_CAPABILITY) {
           if ((mac_phy_cap[pdev_idx].he_cap_phy_info_2G[0]) &
               IEEE80211_HECAP_LDPC_MASK)
               ieee80211com_set_ldpccap(ic, IEEE80211_HECAP_C_LDPC_TXRX);
           else
               qdf_info("LDPC not supported in 2G HE CAP");
        }

        if (!(mac_phy_cap[pdev_idx].supported_bands &
             (WMI_HOST_WLAN_5G_CAPABILITY | WMI_HOST_WLAN_2G_CAPABILITY)))
           qdf_info("Band 2G or 5G check failed. LDPC is not updated. Supported bands = %x",
                         mac_phy_cap[pdev_idx].supported_bands);
    }

    if (mac_phy_cap)
        ol_ath_update_chainmask(ic, target_cap, &mac_phy_cap[pdev_idx]);
    else
        ol_ath_update_chainmask(ic, target_cap, NULL);

    scn->max_tx_power = target_cap->hw_max_tx_power;
    scn->min_tx_power = target_cap->hw_min_tx_power;

    scn->txpowlimit2G = scn->max_tx_power;
    scn->txpowlimit5G = scn->max_tx_power;
    scn->txpower_scale = WMI_HOST_TP_SCALE_MAX;

    ieee80211com_set_txpowerlimit(ic, scn->max_tx_power);

    ol_regdmn_attach(scn);

    ol_regdmn_set_regdomain(scn->ol_regdmn_handle, reg_cap[reg_cap_phy_idx].eeprom_reg_domain);
    ol_regdmn_set_regdomain_ext(scn->ol_regdmn_handle, reg_cap[reg_cap_phy_idx].eeprom_reg_domain_ext);
#if QCA_11AX_STUB_SUPPORT
    if (1 == OL_ATH_IS_11AX_STUB_ENABLED(soc)) {
        ol_regdmn_stub_add_11ax_modes(scn->ol_regdmn_handle);
    }
#endif /* QCA_11AX_STUB_SUPPORT */
    if (target_psoc_get_target_ver(tgt_hdl) == AR6004_VERSION_REV1_3) {
        /*
           It's Hard code for HAL capability and We don't use this branch for McKinley.
           Because McKinley don't support WMI UNIFIED SERVICE READY,
           */
        reg_cap[reg_cap_phy_idx].eeprom_reg_domain = 0;
        reg_cap[reg_cap_phy_idx].eeprom_reg_domain_ext = 0x1f;
        reg_cap[reg_cap_phy_idx].high_2ghz_chan = 0xaac;
        reg_cap[reg_cap_phy_idx].high_5ghz_chan = 0x17d4;
        reg_cap[reg_cap_phy_idx].low_2ghz_chan = 0x908;
        reg_cap[reg_cap_phy_idx].low_5ghz_chan = 0x1338;
        reg_cap[reg_cap_phy_idx].regcap1 = 7;
        reg_cap[reg_cap_phy_idx].regcap2 = 0xbc0;
        reg_cap[reg_cap_phy_idx].wireless_modes = 0x1f9001;
        ol_regdmn_attach(scn);
        ol_regdmn_set_regdomain(scn->ol_regdmn_handle, reg_cap[reg_cap_phy_idx].eeprom_reg_domain);
        ol_regdmn_set_regdomain_ext(scn->ol_regdmn_handle, reg_cap[reg_cap_phy_idx].eeprom_reg_domain_ext);
    }
    if (wmi_service_enabled(wmi_handle, wmi_service_restrt_chnl_support)) {
        ic->ic_rch_params.restrict_channel_support = 1;
        ic->ic_rch_params.low_5ghz_chan  = reg_cap[reg_cap_phy_idx].low_5ghz_chan;
        ic->ic_rch_params.high_5ghz_chan = reg_cap[reg_cap_phy_idx].high_5ghz_chan;

        qdf_info("low_5ghz: %d  high_5gh: %d",
                 reg_cap[reg_cap_phy_idx].low_5ghz_chan,
                 reg_cap[reg_cap_phy_idx].high_5ghz_chan);
    } else {
        ic->ic_rch_params.restrict_channel_support = 0;
    }

    ic->ic_emiwar_80p80 = ol_ath_get_emiwar_80p80_defval(soc);

    ic->ic_reg_parm = *ieee80211_conf_param;
#ifdef WLAN_REG_PARTIAL_OFFLOAD
    if (!(wmi_service_enabled(wmi_handle, wmi_service_regulatory_db)))
        ol_regdmn_start(scn->ol_regdmn_handle, ieee80211_conf_param);
#endif
    return status;
}

static bool ol_ath_is_master_radio(struct ol_ath_softc_net80211 *scn)
{
    struct  wlan_objmgr_psoc *psoc    = scn->soc->psoc_obj;
    struct  wlan_objmgr_pdev *pdev    = scn->sc_pdev;
    struct  ieee80211com     *ic      = &(scn->sc_ic);
    ol_ath_hw_mode_ctx_t *hw_mode_ctx = &scn->soc->hw_mode_ctx;
    struct  tgt_info         *info    = NULL;
    bool    is_master_radio           = false;
    struct  wlan_psoc_host_mac_phy_caps *mac_phy_cap;
    struct  target_pdev_info *tgt_pdev;
    struct  target_psoc_info *tgt_hdl;
    uint8_t idx, num_hw_modes, num_radios, num_tx_chain;
    int32_t pdev_idx = -1;
    uint32_t supported_bands;

    /* not a master radio if feature not supported */
    if (!ol_ath_is_dynamic_hw_mode_enabled(scn->soc))
        goto exit;

    /* not a master radios if the radio does not support
     * target-band
     */
    if (!(hw_mode_ctx->target_band & ic->ic_supported_bands))
        goto exit;

    /* Master 5G radio has the capability to switch
     * between 8x8 and 4x4
     */
    /* identify the radio with IEEE80211_MAX_TX_CHAINS */

    tgt_hdl = (struct target_psoc_info *)wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        goto exit;
    }

    info = &(tgt_hdl->info);

    tgt_pdev = (struct target_pdev_info *)wlan_pdev_get_tgt_if_handle(pdev);
    if (!tgt_pdev) {
        target_if_err("target_pdev_info is null");
        goto exit;
    }

    pdev_idx = target_pdev_get_pdev_idx(tgt_pdev);
    if (pdev_idx < 0) {
        target_if_err("pdev idx is invalid");
        goto exit;
    }

    num_hw_modes = info->hw_modes.num_modes;

    /* check if the pdev corresponding to 'pdev_idx'
     * has max tx chain capacity for any of the modes
     */
    for (idx = 0; idx < num_hw_modes; idx++) {
        num_radios  = target_psoc_get_num_radios_for_mode
                        (tgt_hdl, info->hw_modes.hw_mode_ids[idx]);
        mac_phy_cap = target_psoc_get_mac_phy_cap_for_mode
                         (tgt_hdl, info->hw_modes.hw_mode_ids[idx]);

        if (pdev_idx < num_radios && mac_phy_cap) {
            supported_bands = mac_phy_cap[pdev_idx].supported_bands;

            if (supported_bands & WMI_HOST_WLAN_5G_CAPABILITY) {
                num_tx_chain = num_chain_from_chain_mask(
                                    mac_phy_cap[pdev_idx].tx_chain_mask_5G);

            } else if (supported_bands & WMI_HOST_WLAN_2G_CAPABILITY) {
                num_tx_chain =  num_chain_from_chain_mask(
                                    mac_phy_cap[pdev_idx].tx_chain_mask_2G);
            } else {
                /* Code required if band support extended in
                 * future. For now initialize num_tx_chain to
                 * 0.
                 */
                num_tx_chain = 0;
            }

            if (num_tx_chain == IEEE80211_MAX_TX_CHAINS) {
                is_master_radio = true;
                break;
            }
        } /* end if (pdev_idx < num_radios) */
    } /* end for */

exit:
    if (pdev_idx >= 0)
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                  QDF_TRACE_LEVEL_INFO,
                  FL("pdev_idx: %d is_master_radio: %s"),
                  pdev_idx, is_master_radio ? "YES" : "NO");
    return is_master_radio;
}

uint8_t ol_ath_get_max_supported_radios(ol_ath_soc_softc_t *soc)
{
    struct  wlan_objmgr_psoc *psoc    = soc->psoc_obj;
    struct  tgt_info         *info    = NULL;
    uint8_t max_num_radios            = 0;
    struct  target_psoc_info *tgt_hdl;
    uint8_t idx, num_hw_modes, num_radios;

    /* return default if feature not supported */
    if (!ol_ath_is_dynamic_hw_mode_enabled(soc))
        goto exit;

    tgt_hdl = (struct target_psoc_info *)wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        goto exit;
    }

    info = &(tgt_hdl->info);

    num_hw_modes = info->hw_modes.num_modes;

    for (idx = 0; idx < num_hw_modes; idx++) {
        num_radios = target_psoc_get_num_radios_for_mode
                        (tgt_hdl, info->hw_modes.hw_mode_ids[idx]);

        if (num_radios > max_num_radios)
            max_num_radios = num_radios;
    }

    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
              QDF_TRACE_LEVEL_INFO,
              FL("max_num_radios: %d"), max_num_radios);

exit:
    return max_num_radios;
}

static int ol_ath_reconfig_pdev_post_hw_mode_switch(
        struct ol_ath_softc_net80211 *scn)
{
    ol_ath_soc_softc_t *soc           = scn->soc;
    uint8_t pdev_idx;
    struct ieee80211com *ic;
    struct wlan_objmgr_psoc *psoc     = soc->psoc_obj;
    struct wlan_objmgr_pdev *pdev     = scn->sc_pdev;
    struct target_pdev_info *tgt_pdev;
    struct target_psoc_info *tgt_hdl;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
    struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2;
    struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap;
    struct wlan_psoc_target_capability_info *target_cap;
    struct wmi_unified *wmi_handle;
    ol_ath_hw_mode_ctx_t *hw_mode_ctx = &soc->hw_mode_ctx;
    int status = 0;
    bool eht_capab = false;

    tgt_hdl = (struct target_psoc_info *)wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target handle is null");
        status = -EINVAL;
        goto exit;
    }

    tgt_pdev = (struct target_pdev_info *)wlan_pdev_get_tgt_if_handle(pdev);
    if (!tgt_pdev) {
        qdf_err("target_pdev_info is null");
        status = -EINVAL;
        goto exit;
    }

    pdev_idx = target_pdev_get_phy_idx(tgt_pdev);
    if (pdev_idx < 0) {
        qdf_err("pdev idx is invalid");
        status = -EINVAL;
        goto exit;
    }

    target_cap = target_psoc_get_target_caps(tgt_hdl);
    mac_phy_cap = target_psoc_get_mac_phy_cap_for_mode(tgt_hdl, hw_mode_ctx->target_mode);

    if (!mac_phy_cap)
        qdf_err("mac phy cap is NULL");

    mac_phy_caps_ext2 = target_psoc_get_mac_phy_cap_ext2_for_mode(tgt_hdl, hw_mode_ctx->target_mode);

    reg_cap = ucfg_reg_get_hal_reg_cap(psoc);
    if (!reg_cap) {
        qdf_err("invalid reg_cap");
        status = -EINVAL;
        goto exit;
    }

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("invalid wmi_handle");
        status = -EINVAL;
        goto exit;
    }

    ic = &scn->sc_ic;

    /* If extended service ready is supported by firmware, Use mac_phy
     * capabilities provided by extended service ready event instead of
     * service ready event. Extended service ready will override caps
     * received in service ready
     */
    if (wmi_service_enabled(wmi_handle, wmi_service_ext_msg) && mac_phy_cap) {
        ol_ath_update_band_mapping(&reg_cap[pdev_idx], &mac_phy_cap[pdev_idx]);

        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                  QDF_TRACE_LEVEL_DEBUG,
                  "phy id = %d Modes supported", pdev_idx);

        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                  QDF_TRACE_LEVEL_DEBUG,
                  " 11b = %d 11g = %d 11a = %d 11n = %d 11ac = %d 11ax = %d",
                  mac_phy_cap[pdev_idx].supports_11b,
                  mac_phy_cap[pdev_idx].supports_11g,
                  mac_phy_cap[pdev_idx].supports_11a,
                  mac_phy_cap[pdev_idx].supports_11n,
                  mac_phy_cap[pdev_idx].supports_11ac,
                  mac_phy_cap[pdev_idx].supports_11ax);

        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                  QDF_TRACE_LEVEL_DEBUG,
                  " Reg cap - phy_id = %d supp_bnd = %x, modes = %llx, "
                  "lo_2ghz = %d, hi_2ghz = %d lo_5ghz = %d, hi_5ghz = %d",
                  pdev_idx,
                  mac_phy_cap[pdev_idx].supported_bands,
                  reg_cap[pdev_idx].wireless_modes,
                  reg_cap[pdev_idx].low_2ghz_chan,
                  reg_cap[pdev_idx].high_2ghz_chan,
                  reg_cap[pdev_idx].low_5ghz_chan,
                  reg_cap[pdev_idx].high_5ghz_chan);

        ol_ath_update_wireless_modes_ext(&reg_cap[pdev_idx], &mac_phy_cap[pdev_idx]);
        ol_ath_update_target_cap_from_mac_phy_cap(target_cap,
                                  &mac_phy_cap[pdev_idx]);
        ol_ath_update_ext_caps(&mac_phy_cap[pdev_idx], ic);
        wlan_psoc_mlme_get_11be_capab(psoc, &eht_capab);
        if (mac_phy_caps_ext2 && eht_capab)
            ol_ath_update_ext_eht_caps(&mac_phy_cap[pdev_idx], &mac_phy_caps_ext2[pdev_idx], ic);
        /* update supprted bands from mac_phy_cap */
        ic->ic_supported_bands = mac_phy_cap[pdev_idx].supported_bands;
    }

    ol_ath_update_caps(ic, target_cap);

    if (mac_phy_cap)
        ol_ath_update_chainmask(ic, target_cap, &mac_phy_cap[pdev_idx]);
    else
        ol_ath_update_chainmask(ic, target_cap, NULL);

    scn->max_tx_power  = target_cap->hw_max_tx_power;
    scn->min_tx_power  = target_cap->hw_min_tx_power;

    scn->txpowlimit2G  = scn->max_tx_power;
    scn->txpowlimit5G  = scn->max_tx_power;
    scn->txpower_scale = WMI_HOST_TP_SCALE_MAX;

    ieee80211com_set_txpowerlimit(ic, scn->max_tx_power);

exit:
    return status;
}

static void ol_ath_update_phymode_caps(struct ieee80211com *ic, enum ieee80211_phymode mode)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    ol_ath_soc_softc_t *soc = scn->soc;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    struct wlan_objmgr_pdev *pdev = scn->sc_pdev;
    struct target_pdev_info *tgt_pdev;
    struct target_psoc_info *tgt_hdl;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap_arr;
    int32_t pdev_idx;
    struct wmi_unified *wmi_handle;

    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target handle is null");
        return;
    }

    tgt_pdev = wlan_pdev_get_tgt_if_handle(pdev);
    if (!tgt_pdev) {
        target_if_err("target_pdev_info is null");
        return;
    }

    pdev_idx = target_pdev_get_pdev_idx(tgt_pdev);
    if (pdev_idx < 0) {
        qdf_info("pdev idx is invalid ");
        return;
    }
    QDF_TRACE(QDF_MODULE_ID_DEBUG, QDF_TRACE_LEVEL_DEBUG,
            "pdev id is %d \n",pdev_idx);

    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);

    if (wmi_service_enabled(wmi_handle,
                wmi_service_ext_msg)) {

        struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;

        mac_phy_cap_arr = target_psoc_get_mac_phy_cap(tgt_hdl);
        if (!mac_phy_cap_arr) {
            qdf_info("mac phy cap is NULL ");
            return;
        }

        mac_phy_cap = &mac_phy_cap_arr[pdev_idx];

        if (ieee80211_is_phymode_2g(mode)) {
            ieee80211com_clear_htcap(ic, -1);
            ieee80211com_clear_vhtcap(ic, -1);
            ol_ath_update_ht_caps(ic, mac_phy_cap->ht_cap_info_2G);
            ol_ath_update_vht_caps(ic, mac_phy_cap->vht_cap_info_2G,
                    mac_phy_cap->vht_supp_mcs_2G);
            ol_ath_populate_he_caps(ic, mac_phy_cap->he_cap_info_2G,
                    &mac_phy_cap->he_supp_mcs_2G,
                    mac_phy_cap->he_cap_phy_info_2G, sizeof(mac_phy_cap->he_cap_phy_info_2G),
                    &mac_phy_cap->he_ppet2G, &mac_phy_cap->he_cap_info_internal);
        } else if (ieee80211_is_phymode_5g_or_6g(mode)) {
            /*
             * 6GHz: no separate 6G handling required as 6G phy will not
             * report ht_cap_info_5G, vht_cap_info_5G.
             */
            ieee80211com_clear_htcap(ic, -1);
            ieee80211com_clear_vhtcap(ic, -1);
            ol_ath_update_ht_caps(ic, mac_phy_cap->ht_cap_info_5G);
            ol_ath_update_vht_caps(ic, mac_phy_cap->vht_cap_info_5G,
                    mac_phy_cap->vht_supp_mcs_5G);
            ol_ath_populate_he_caps(ic, mac_phy_cap->he_cap_info_5G,
                    &mac_phy_cap->he_supp_mcs_5G,
                    mac_phy_cap->he_cap_phy_info_5G, sizeof(mac_phy_cap->he_cap_phy_info_5G),
                    &mac_phy_cap->he_ppet5G, &mac_phy_cap->he_cap_info_internal);
        } else {
            qdf_debug("Error Mode %d not 2G or 5G", mode);
        }
    }
}

/**
 * ol_ath_is_dynamic_hw_mode_enabled(): Checks if FW supports
 * dynamic_hw_mode feature and if it is enabled in host
 * @soc: soc handle
 *
 * return: true if supported, false otherwise
 */
bool ol_ath_is_dynamic_hw_mode_enabled(ol_ath_soc_softc_t *soc)
{
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    bool fw_support               = wlan_psoc_nif_fw_ext_cap_get(psoc,
                                    WLAN_SOC_CEXT_DYNAMIC_HW_MODE);
    bool feature_support          = wlan_psoc_nif_feat_cap_get(psoc,
                                    WLAN_SOC_F_DYNAMIC_HW_MODE);
    bool is_enabled               = fw_support & feature_support;

    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
            FL("is_enabled: %d fw_support: %d feature_support: %d"),
            is_enabled, fw_support, feature_support);

    return is_enabled;
}

/**
 * ol_ath_handle_dbs_sbs_to_dbs_switch_in_dp(): Handles DP-
 * impact of switching from DBS_SBS to DBS mode
 * @ic: ic pointer
 *
 * Return: none
 */
static void ol_ath_handle_dbs_sbs_to_dbs_switch_in_dp(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    ol_txrx_soc_handle soc_txrx_handle;

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    cdp_txrx_set_pdev_status_down(soc_txrx_handle,
                        wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj), true);
}

/**
 * ol_ath_handle_dbs_to_dbs_sbs_switch_in_dp(): Handles DP-
 * impact of switching from DBS to DBS_SBS mode
 * @ic: ic pointer
 *
 * Return: none
 */
static void ol_ath_handle_dbs_to_dbs_sbs_switch_in_dp(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    ol_txrx_soc_handle soc_txrx_handle;

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    cdp_txrx_set_pdev_status_down(soc_txrx_handle,
                        wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj), false);
}

static void ol_if_update_beacon_for_modeswitch(struct ieee80211com *ic)
{
    struct ieee80211vap *tmp_vap;

    TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
        if (tmp_vap->iv_opmode == IEEE80211_M_HOSTAP) {
            tmp_vap->iv_mode_switch_update = 1;
            wlan_vdev_beacon_update(tmp_vap);
        }
    }
}

/**
 * ol_ath_hw_mode_config_primary_pdev(): Config primary pdev
 * during dynamic mode switch between DBS_SBS and DBS
 * @soc : soc handle
 * @up  : bool to indicate up or down
 *
 * return: QDF_STATUS_SUCCESS in case of success and
 * QDF_STATUS_E_INVAL otherwise
 */
static QDF_STATUS ol_ath_hw_mode_config_primary_pdev(ol_ath_soc_softc_t *soc,
                                                     bool up)
{
    struct wlan_objmgr_psoc *psoc        = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t *hw_mode_ctx = &soc->hw_mode_ctx;
    struct wlan_objmgr_pdev *pdev = hw_mode_ctx->primary_pdev;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;
    struct ieee80211vap *vap;
    bool pdev_reconfig_success;
    int status = QDF_STATUS_SUCCESS;
    ol_txrx_soc_handle soc_txrx_handle;

    if (!pdev) {
        status = QDF_STATUS_E_ABORTED;
        goto exit;
    }

    scn = (struct ol_ath_softc_net80211 *)lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                QDF_TRACE_LEVEL_ERROR, FL("scn is NULL"));
        status = QDF_STATUS_E_ABORTED;
        goto exit;
    }
    ic = &(scn->sc_ic);

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (up) {
        if (hw_mode_ctx->is_fw_resp_success) {
            /* copy curchan params */
            ol_ath_copy_curchan_params(ic, hw_mode_ctx);

            pdev_reconfig_success =
                !ol_ath_reconfig_pdev_post_hw_mode_switch(scn);

            /* regulatory re-init */
            ol_regdmn_reinit_post_mode_switch(ic);

            /* Reinit ic and vap channel structures */
            status = ol_ath_reinit_channel_params(ic, hw_mode_ctx);
            if (status != QDF_STATUS_SUCCESS) {
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                          QDF_TRACE_LEVEL_ERROR,
                          FL("Cannot reinit channel structures for pri.pdev!"));
                goto exit;
            }

            /* DFS Reinit. */
            status = ol_if_reinit_dfs_for_mode_switch_fast(scn,
                             hw_mode_ctx->target_mode);
            if (status != QDF_STATUS_SUCCESS) {
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                          QDF_TRACE_LEVEL_ERROR,
                          FL("Cannot reinit DFS after mode switch!"));
                goto exit;
            }

            /* set up rates */
            ol_ath_setup_rates(ic);

            TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
                /* reconfig vap params as-per new ic-caps */
                ol_ath_update_vap_caps(vap, ic);
            }
            wlan_vap_omn_update(ic);

            /* Mode switch is complete, if a monitor VAP exists,
               bring it back up. */
            if(ic->ic_mon_vap &&
               (wlan_vdev_mlme_is_active(ic->ic_mon_vap->vdev_obj) ==
                QDF_STATUS_SUCCESS)) {

                cdp_set_monitor_mode(soc_txrx_handle,
                                     wlan_vdev_get_id(ic->ic_mon_vap->vdev_obj), 0);
            }

            ol_if_update_beacon_for_modeswitch(ic);
        } else {
            /* If modeswitch fails copy back the NOL from psoc to pdevs. */
            ol_if_dfs_reinit_nol(scn, hw_mode_ctx->target_mode);
        }
    } else {
#if WLAN_SPECTRAL_ENABLE
        enum spectral_scan_mode smode;
        struct spectral_cp_request sscan_req;

        /* Stop spectral scan if it's on-going */
        for (smode = SPECTRAL_SCAN_MODE_NORMAL; smode < SPECTRAL_SCAN_MODE_MAX; smode++) {
            sscan_req.ss_mode = smode;
            sscan_req.req_id = SPECTRAL_STOP_SCAN;

            status = ucfg_spectral_control(pdev, &sscan_req);
            if (QDF_IS_STATUS_ERROR(status)) {
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR,
                        FL("Cannot stop SPECTRAL scan for primary pdev!"));
                goto exit;
            }

        } /* for */
#endif
        /* stop active scan */
        if (wlan_pdev_scan_in_progress(pdev)) {
            wlan_abort_scan(pdev,
                            wlan_objmgr_pdev_get_pdev_id(pdev),
                            INVAL_VDEV_ID, INVAL_SCAN_ID, true);
            if (QDF_IS_STATUS_ERROR(status)) {
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR,
                        FL("aborting the existing scan is unsuccessful"));
                goto exit;
            }
        }

        /* DFS Deinit. */
        status = ol_if_deinit_dfs_for_mode_switch_fast(scn, hw_mode_ctx);
        if (status != QDF_STATUS_SUCCESS) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                      QDF_TRACE_LEVEL_ERROR,
                      FL("Cannot deinit DFS before mode switch!"));
            goto exit;
        }

        /* Mode switch is starting, bring down monitor VAP as FW
           will not retain the filter settings. After mode switch
           completion, bring up monitor VAP if needed */
        if(ic->ic_mon_vap &&
           (wlan_vdev_mlme_is_active(ic->ic_mon_vap->vdev_obj) ==
            QDF_STATUS_SUCCESS)) {

            cdp_reset_monitor_mode(soc_txrx_handle,
                                   wlan_objmgr_pdev_get_pdev_id(pdev), 0);
        }
    }
exit:
    return status;
}

/**
 * ol_ath_hw_mode_config_secondary_pdev(): Config secondary pdev
 * during dynamic mode switch between DBS_SBS and DBS
 * @soc : soc handle
 * @up  : bool to indicate up or down
 *
 * return: QDF_STATUS_SUCCESS in case of success and
 * QDF_STATUS_E_INVAL otherwise
 */
static QDF_STATUS ol_ath_hw_mode_config_secondary_pdev(ol_ath_soc_softc_t *soc, bool up)
{
    struct wlan_objmgr_psoc *psoc        = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t    *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info *tgt_hdl;
    struct wlan_objmgr_pdev      *pdev;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com           *ic;
    uint8_t num_radios;
    struct ieee80211vap *vap;
    struct wlan_objmgr_vdev *vdev;
    int waitcnt = 0;
    int status = QDF_STATUS_SUCCESS;
    uint8_t i;

    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);
    num_radios = target_psoc_get_num_radios(tgt_hdl);

    for (i = 0; i < num_radios; i++) {
        pdev = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_MLME_NB_ID);
        if (!pdev) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR,
                    FL("pdev object (id: %d) is NULL"), i);
            continue;
        }

        scn = (struct ol_ath_softc_net80211 *) lmac_get_pdev_feature_ptr(pdev);
        if (!scn) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR, FL("scn is NULL"));
            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
            continue;
        }

        ic = &(scn->sc_ic);
        if ((pdev == hw_mode_ctx->primary_pdev) ||
            ((hw_mode_ctx->target_band & ic->ic_supported_bands) == 0)) {
            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
            continue;
        }

        switch(hw_mode_ctx->target_mode) {
        case WMI_HOST_HW_MODE_DBS:
            if (up) {
                if (!hw_mode_ctx->is_fw_resp_success) {
                    /* Bring up (back) secondary interface */
                    ol_ath_ifce_setup(scn, UP);
                }
            } else {
                /* Bring down secondary interface */
                ol_ath_ifce_setup(scn, DOWN);
                TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
                    vdev = vap->vdev_obj;
                    while ((wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_INIT) &&
                                      waitcnt < OSIF_MAX_STOP_VAP_TIMEOUT_CNT) {
                        schedule_timeout_interruptible(CONVERT_SEC_TO_SYSTEM_TIME(1)/100);
                        waitcnt++;
                    }

                    if (wlan_vdev_mlme_get_state(vdev) != WLAN_VDEV_S_INIT) {
                        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
                        status = QDF_STATUS_E_ABORTED;
                        goto exit;
                    }
                }

                waitcnt = 0;
                while (qdf_atomic_read(&scn->mgmt_ctx.mgmt_pending_completions) &&
                       waitcnt < OSIF_MAX_STOP_VAP_TIMEOUT_CNT) {
                    schedule_timeout_interruptible(CONVERT_SEC_TO_SYSTEM_TIME(1)/100);
                    waitcnt++;
                }
            }
            break;
        case WMI_HOST_HW_MODE_DBS_SBS:
            if (up) {
                /* Bring up secondary interface */
                ol_ath_ifce_setup(scn, UP);
            }
            break;
        }

        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
    }

exit:
    return status;
}

/**
 * ol_ath_init_hw_mode_switch(): Reset hw_mode_ctx and perform
 * sanity check before proceeding with hw-mode change. Following
 * aret the sanity checks:
 * 	1. Is dynamic hw-mode switch supported by fw and if the
 * 	feature is enabled in host?
 * 	2. Is recovery in progress?
 * 	3. Is tgt_hdl retrieval success?
 * 	4. Is wmi_handle retrieval success?
 * 	5. Is current_mode equal to requested mode?
 *  6. Is the request issued in a 2G wifi interface?
 * @scn: scn handle
 * @mode: requested target mode
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_init_hw_mode_switch(struct ol_ath_softc_net80211 *scn,
                                      uint32_t mode)
{
    ol_ath_soc_softc_t       *soc         = scn->soc;
    struct wlan_objmgr_psoc  *psoc        = soc->psoc_obj;
    struct ieee80211com      *ic          = &scn->sc_ic;
    struct ieee80211com      *primary_ic  = NULL;
    ol_ath_hw_mode_ctx_t     *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info  *tgt_hdl     = NULL;
    struct wmi_unified       *wmi_handle  = NULL;
    struct ieee80211vap      *vap;
    int status                            = -EINVAL;
    int i;
#if WLAN_CFR_ENABLE
    enum cfr_capt_status cfr_status = PEER_CFR_CAPTURE_DISABLE;
#endif

    if (!ol_ath_is_dynamic_hw_mode_enabled(soc)) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Feature not supported"));
        goto exit;
    }

    if (soc->recovery_in_progress) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Recovery in-progress. Try again "
                                      "after recovery-complete"));
        goto exit;
    }

    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("target hdl is null"));
        goto exit;
    }

    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
    if (!wmi_handle) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Invalid WMI handle"));
        goto exit;
    }

    /* reset relevant fields in hw_mode_ctx */

    if (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        struct ol_ath_softc_net80211 *primary_scn =
                (struct ol_ath_softc_net80211 *)lmac_get_pdev_feature_ptr(
                 hw_mode_ctx->primary_pdev);
        if (primary_scn)
            primary_ic = &primary_scn->sc_ic;
    }
    if (primary_ic) {
        IEEE80211_RADAR_MODE_SWITCH_LOCK(primary_ic);
    }
    /* set mode_switch flag */
    hw_mode_ctx->is_switch_in_progress      = true;
    if (primary_ic) {
        IEEE80211_RADAR_MODE_SWITCH_UNLOCK(primary_ic);
    }
    /* assign target_mode as the requested mode */
    hw_mode_ctx->target_mode                = mode;
    /* reset fw_resp_status */
    hw_mode_ctx->is_fw_resp_success         = false;
    /* bw reduction during dms flag */
    hw_mode_ctx->is_bw_reduced_during_dms   = false;
    /* reset event */
    qdf_event_reset(&hw_mode_ctx->event);
    for (i = 0; i < WMI_HOST_MAX_PDEV; i++) {
        hw_mode_ctx->next_pdev_map[i] = WMI_HOST_PDEV_ID_INVALID;
    }

    if (hw_mode_ctx->current_mode == mode) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Current mode == Requested mode"));
        goto exit;
    }

    if (!(hw_mode_ctx->target_band & ic->ic_supported_bands)) {
        /* The space in the following print will be filled
         * in future if 'target_band' becomes configurable
         * supporting the feature in bands other than 5G
         */
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Feature not supported in this radio."
            "<Does not support target_band> %s"),
            (hw_mode_ctx->target_band == WMI_HOST_WLAN_5G_CAPABILITY) ?
            "5GHz" : " ");
        goto exit;
    }

    if (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        /* Make sure none of ACS/CAC/CSA is in progress */
        scn = (struct ol_ath_softc_net80211 *) lmac_get_pdev_feature_ptr(
            hw_mode_ctx->primary_pdev);
        if (scn) {
            ic = &scn->sc_ic;
            if ((ic->ic_flags & IEEE80211_F_CHANSWITCH) ||
                wlan_is_ap_cac_timer_running(ic)) {
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR,
                    FL("Can't switch mode - either CSA or CAC Timer is in progress"));
                goto exit;
            }
            TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
                if (wlan_get_param(vap,IEEE80211_GET_ACS_STATE) ||
                    wlan_get_param(vap,IEEE80211_GET_CAC_STATE)) {
                    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR,
                        FL("Can't switch mode - either ACS or CAC is in progress"));
                    goto exit;
                }
                if ((vap->iv_opmode == IEEE80211_M_STA) || (vap->iv_opmode ==  IEEE80211_M_WDS)) {
                    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR,
                        FL("Can't switch mode - %s is not in AP mode!"), vap->iv_netdev_name);
                    goto exit;
                }
            }
        }

        /* return if CFR capture was started */
#if WLAN_CFR_ENABLE
        ucfg_cfr_get_capture_status(hw_mode_ctx->primary_pdev, &cfr_status);
        if (cfr_status == PEER_CFR_CAPTURE_ENABLE) {
            qdf_err("CFR capture in progress, abort HW mode change!!");
            goto exit;
        }
#endif
        if (ol_ath_hw_mode_check_pdev_map(soc)) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                QDF_TRACE_LEVEL_ERROR,
                FL("Can't switch mode - pdev mapping infeasible"));
            goto exit;
        }

        hw_mode_ctx->ts_start = qdf_ktime_to_ns(qdf_ktime_get());
        hw_mode_ctx->cnt_attempt++;

        /* Clear recover_mode now as it's tried */
        if (hw_mode_ctx->recover_mode) {
            hw_mode_ctx->recover_mode = 0;
        }

        status = ol_ath_hw_mode_config_secondary_pdev(soc, false);
        if (status != EOK)
            goto exit;
        status = ol_ath_hw_mode_config_primary_pdev(soc, false);
        if (status != EOK)
            goto exit;
    }
    status = EOK;

exit:
    return status;
}

/**
 * ol_ath_prepare_hw_mode_switch(): Prepare host for mode switch
 * before sending hw_mode_cmd to FW
 * @soc: soc handle
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_prepare_hw_mode_switch(ol_ath_soc_softc_t *soc)
{
    struct wlan_objmgr_psoc  *psoc          = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t       *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info  *tgt_hdl       = NULL;
    int status                              = -EINVAL;
    uint8_t num_radios, pdev_id;

    /* retrieve tgt_hdl */
    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    /* derive number of pdevs (radios) for current hw-mode
     */
    num_radios = target_psoc_get_num_radios(tgt_hdl);

    /*------------------Start: prepare mode switch--------------------*/
    for (pdev_id = 0; pdev_id < num_radios; pdev_id++) {
        struct wlan_objmgr_pdev *pdev  = NULL;
        struct ol_ath_softc_net80211     *scn;
        struct ieee80211com               *ic;

        pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);
        if (!pdev) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR,
                    FL("pdev object (id: %d) is NULL"), pdev_id);
            continue;
        }

        scn = (struct ol_ath_softc_net80211 *) lmac_get_pdev_feature_ptr(pdev);
        if (!scn) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR, FL("scn is NULL"));

            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
            continue;
        }

        ic = &(scn->sc_ic);

        /* Currently, FW advertises mac_phy_cap[pdev_x].supported_bands as
         * either 5G or 2G capable. Host code handles the possibility of
         * each phy being either 5G or 2G capble. Following logic of checking
         * if the target band is supported by this ic/phy or not is based on
         * the above fact. In future, phys may be capable of supporting mulitple
         * bands. In which case mac_phy_cap[pdev_x].supported_bands will reflect
         * the same and host code (including cmn_dev) needs to scale to support
         * that. Following condition will need change in similar lines.
         */
        if (hw_mode_ctx->target_band & ic->ic_supported_bands) {
            /* Unregister wext and cfg handlers for the radio
             * and vaps (in that radio) ifces. It will also
             * bring down the radio ifce
             */
            ol_ath_ifce_setup(scn, DOWN);

            /* as a serialization requirement for the hw_mode_switch_cmd
             * to fw we must wait for all vdevs to reach init state which
             * will be brought down in above step
             */
            if (wlan_pdev_wait_to_bringdown_vdevs(ic, ALL_VDEVS)) {
                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                    QDF_TRACE_LEVEL_ERROR,
                    FL("Failed to bring down all vaps for pdev id: %d"),
                    pdev_id);
                if (pdev)
                    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
                goto exit;
            }

            /* call dfs de-init */
            ol_if_dfs_pdev_deinit_pre_mode_switch(scn);
        }

        if (pdev)
            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);

    } /* for loop */
    ol_if_dfs_psoc_deinit_pre_mode_switch(psoc);
    /*------------------End: prepare mode switch--------------------*/

    status = EOK;

exit:
    return status;
}

/**
 * ol_ath_accomplish_hw_mode_switch(): Accomplish hw-mode switch
 * in host post receiving FW response for hw_mode_switch command
 * @scn: scn handle
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_accomplish_hw_mode_switch(struct ol_ath_softc_net80211 *scn)
{
    ol_ath_soc_softc_t       *soc         = scn->soc;
    struct wlan_objmgr_psoc  *psoc        = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t     *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info  *tgt_hdl     = NULL;
    int status                            = -EINVAL;
    uint8_t num_radios, pdev_id;

    /* retrieve tgt_hdl */
    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    /* derive max number of pdevs (radios) supported in the soc */
    num_radios = ol_ath_get_max_supported_radios(soc);

    /* Accomplish mode-switch step happens irrespective of
     * success/fail FW response. Following steps need
     * to execute for all radios as failure recovery is
     * handled here as well
     */
    /*------------------Start: accomplish mode switch---------------*/
    for (pdev_id = 0; pdev_id < num_radios; pdev_id++) {
        struct wlan_objmgr_pdev *pdev    = NULL;
        target_resource_config *tgt_cfg  = NULL;
        struct ol_ath_softc_net80211       *scn;
        struct ieee80211com                 *ic;
        struct ieee80211vap                *vap;
        bool pdev_reconfig_success       = false;

        pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, WLAN_MLME_NB_ID);
        if (!pdev) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR,
                        FL("pdev object (id: %d) is NULL"), pdev_id);
            continue;
        }

        scn = (struct ol_ath_softc_net80211 *)
                                lmac_get_pdev_feature_ptr(pdev);
        if (!scn) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_ERROR, FL("scn is NULL"));

            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
            continue;
        }

        ic      = &(scn->sc_ic);
        psoc    = scn->soc->psoc_obj;
        tgt_cfg = target_psoc_get_wlan_res_cfg(tgt_hdl);

        if (hw_mode_ctx->target_band & ic->ic_supported_bands) {
            if (hw_mode_ctx->is_fw_resp_success) {
                /* Reconfig mac-phy-cap, reg-cap etc. as per
                 * corresponding pdev in target-mode. The following
                 * function returns 0 for success and non-0 value
                 * for failure. Hence, a '!' of the return value
                 * is assigned to 'pdev_reconfig_success' status
                 * variable
                 */
                pdev_reconfig_success =
                    !ol_ath_reconfig_pdev_post_hw_mode_switch(scn);
            }

            switch(hw_mode_ctx->target_mode) {
                case WMI_HOST_HW_MODE_DBS:
                    /* pdev-level regulatory/dfs re-init is required for
                     * non-master radios only if pdev_reconfig failed
                     * as this particular case is handling an attempt
                     * to switch from dbs_sbs to dbs mode and a pdev_
                     * reconfig failure needs regulatory/dfs recovery
                     * to previous state (before attempting the switch)
                     * for non-master radios along with the master radio
                     */
                    if (ic->ic_master_radio || !pdev_reconfig_success) {
                        /* regulatory re-init */
                        ol_regdmn_reinit_post_mode_switch(ic);
                        /* dfs re-init */
                        ol_if_dfs_pdev_reinit_post_mode_switch(ic);
                    }
                    if (hw_mode_ctx->is_fw_resp_success) {
                        if (!ic->ic_master_radio) {
                            ol_ath_handle_dbs_sbs_to_dbs_switch_in_dp(ic);
                        } else {
                            ol_ath_ifce_setup(scn, UP);
                        }
                    } else {
                        ol_ath_ifce_setup(scn, UP);
                    }
                break;
                case WMI_HOST_HW_MODE_DBS_SBS:
                    /* pdev-level regulatory/dfs re-init required for
                     * non-master radios only if pdev_reconfig succeeded
                     * as this particular case is handling an attempt
                     * to switch from dbs to dbs_sbs mode and a pdev_
                     * reconfig success needs regulatory/dfs re-init
                     * as per the regulatory capabilities in the attempted
                     * hw-mode. In case of a pdev_reconfig failure,
                     * regulatory/dfs re-init will only happen for master
                     * radio to previous state (before attempting the
                     * switch) as only the master radio was relevant in
                     * dbs mode before this hw-mode switch-attempt
                     */
                    if (ic->ic_master_radio || pdev_reconfig_success) {
                        /* regulatory re-init */
                        ol_regdmn_reinit_post_mode_switch(ic);
                        /* dfs re-init */
                        ol_if_dfs_pdev_reinit_post_mode_switch(ic);
                    }
                    if (hw_mode_ctx->is_fw_resp_success) {
                        if (!ic->ic_master_radio) {
                            ol_ath_handle_dbs_to_dbs_sbs_switch_in_dp(ic);
                        }

                        ol_ath_ifce_setup(scn, UP);
                    } else {
                        if (ic->ic_master_radio) {
                            ol_ath_ifce_setup(scn, UP);
                        }
                    }
                break;
                default:
                    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                            QDF_TRACE_LEVEL_ERROR, FL("Unhandled mode"));
                    if (pdev)
                        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
                    goto exit;
            }

            if (hw_mode_ctx->is_fw_resp_success) {
                if (ic->ic_master_radio) {
                    /* Set max peers supported for each PDEV */
                    wlan_pdev_set_max_peer_count(scn->sc_pdev,
                        wlan_psoc_get_max_peer_count(psoc)/
                        target_psoc_get_num_radios(tgt_hdl));

                    /* set up rates */
                    ol_ath_setup_rates(ic);

                    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
                        /* reset vap params as-per new ic-caps
                         */
                        ieee80211_dynamic_vap_setup(ic, vap, vap->vdev_mlme);
                    }
                }
            }
        } /* band satisfiability */

        if (pdev)
            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);

    } /* for loop */
    /*------------------End: accomplish mode switch---------------*/
    status = EOK;

exit:
    return status;
}

/**
 * ol_ath_deinit_hw_mode_switch(): Perform reset and de-init steps
 * post hw-mode switch. This function performs following de-init steps:
 * 1. Sets current_hw_mode to requested mode if the mode switch succeeded.
 * It also sets the preferred_hw_mode in case of success.
 * 2. Sends success/fail acfg event
 * 3. Resets is_switch_in_progress
 * @scn: scn handle
 * @status: status reflecting hw-mode swithc attempt
 *
 * return: status - value zero in case of success and non-zero otherwise
 */
static int ol_ath_deinit_hw_mode_switch(struct ol_ath_softc_net80211 *scn,
                                        int status)
{
    ol_ath_soc_softc_t       *soc           = scn->soc;
    struct wlan_objmgr_psoc  *psoc          = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t       *hw_mode_ctx = &soc->hw_mode_ctx;
    struct target_psoc_info  *tgt_hdl       = NULL;
    uint32_t avg_len;
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event           = NULL;
#endif

    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    if (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        ol_ath_hw_mode_config_primary_pdev(soc, true);
    }

    if ((status == EOK && hw_mode_ctx->is_fw_resp_success) &&
            !soc->recovery_in_progress) {
        hw_mode_ctx->current_mode = hw_mode_ctx->target_mode;
        target_psoc_set_preferred_hw_mode(tgt_hdl, hw_mode_ctx->current_mode);
    }

    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
              QDF_TRACE_LEVEL_DEBUG, FL("current_mode: %s"),
              (hw_mode_ctx->current_mode == 1) ? "DBS":
              (hw_mode_ctx->current_mode == 4) ? "DBS_SBS": "Invalid mode");

#if UMAC_SUPPORT_ACFG
    if (!hw_mode_ctx->is_boot_in_progress) {
        if (osif_scn_event_filter_status_to_app(&scn->sc_ic, EVENT_ACFG,
                    WL_EVENT_TYPE_HW_MODE_CHANGE_STATUS)) {
            acfg_event = (acfg_event_data_t *)
                qdf_mem_malloc( sizeof(acfg_event_data_t));
            if (acfg_event != NULL) {
                if (soc->recovery_in_progress) {
                    /* if recover in-progress then always claim fialure */
                    acfg_event->result = false;
                } else {
                    acfg_event->result = hw_mode_ctx->is_fw_resp_success;
                }

                QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                        QDF_TRACE_LEVEL_DEBUG, FL("Send ACFG event"));
                acfg_send_event(scn->sc_osdev->netdev,
                        scn->sc_osdev, WL_EVENT_TYPE_HW_MODE_CHANGE_STATUS, acfg_event);
                qdf_mem_free(acfg_event);
            }
        }
    }
#endif

    hw_mode_ctx->is_switch_in_progress = false;
    if (soc->hw_mode_ctx.dynamic_hw_mode != WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        /* release wait on vap ifce_up */
        qdf_event_set(&hw_mode_ctx->event);
    } else {
        if ((status == QDF_STATUS_SUCCESS) || (status == QDF_STATUS_E_ABORTED))
            ol_ath_hw_mode_config_secondary_pdev(soc, true);

        /* Update stats only if operation is attempted */
        if (hw_mode_ctx->cnt_attempt >
            (hw_mode_ctx->cnt_success + hw_mode_ctx->cnt_failure)) {
            if ((status == QDF_STATUS_SUCCESS) && hw_mode_ctx->is_fw_resp_success)
                hw_mode_ctx->cnt_success++;
            else
                hw_mode_ctx->cnt_failure++;

            hw_mode_ctx->ts_end = qdf_ktime_to_ns(qdf_ktime_get());
            if (hw_mode_ctx->ts_end > hw_mode_ctx->ts_start) {
                hw_mode_ctx->time_last = qdf_do_div(
                    hw_mode_ctx->ts_end - hw_mode_ctx->ts_start,
                    QDF_NSEC_PER_MSEC);
            } else {
                hw_mode_ctx->time_last = 0;
            }
            if (hw_mode_ctx->time_last) {
                avg_len = (hw_mode_ctx->cnt_success < MOVING_AVG_LENGTH) ?
                    hw_mode_ctx->cnt_success : MOVING_AVG_LENGTH;
                hw_mode_ctx->time_avg = qdf_do_div(
                    ((uint64_t)hw_mode_ctx->time_avg) * (avg_len - 1) + hw_mode_ctx->time_last,
                    avg_len);
            }

            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
                      FL("-----------------------"));
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
                      FL("cnt_attempt : %d"), hw_mode_ctx->cnt_attempt);
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
                      FL("cnt_success : %d"), hw_mode_ctx->cnt_success);
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
                      FL("cnt_failure : %d"), hw_mode_ctx->cnt_failure);
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
                      FL("time_last   : %d (ms)"), hw_mode_ctx->time_last);
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_DEBUG,
                      FL("time_avg    : %d (ms)"), hw_mode_ctx->time_avg);
        }
    }

    return status;
}

/**
 * ol_ath_deinit_rf_path_switch(): Perform reset and de-init steps
 * post rf-path switch. This function performs following de-init steps:
 * 1. Sets current_hw_mode to requested mode if the mode switch succeeded.
 * It also sets the preferred_hw_mode in case of success.
 * 2. Sends success/fail acfg event
 * 3. Resets is_switch_in_progress
 * @scn: scn handle
 * @status: status reflecting hw-mode swithc attempt
 *
 * return: status - value zero in case of success and non-zero otherwise
 */
static int ol_ath_deinit_rf_path_switch(struct ol_ath_softc_net80211 *scn,
                                        int status)
{
    ol_ath_soc_softc_t       *soc           = scn->soc;
    struct wlan_objmgr_psoc  *psoc          = soc->psoc_obj;
    ol_ath_rf_path_ctx_t       *rf_path_ctx = &soc->rf_path_ctx;
    struct target_psoc_info  *tgt_hdl       = NULL;
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event           = NULL;
#endif

    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    if ((status == EOK && rf_path_ctx->is_fw_resp_success) &&
            !soc->recovery_in_progress) {
        rf_path_ctx->current_index = rf_path_ctx->target_index;
    }

    QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
              QDF_TRACE_LEVEL_DEBUG, FL("current_index: %d") ,
              rf_path_ctx->current_index);

#if UMAC_SUPPORT_ACFG
    if (osif_scn_event_filter_status_to_app(&scn->sc_ic, EVENT_ACFG,
                WL_EVENT_TYPE_RF_PATH_CHANGE_STATUS)) {
        acfg_event = (acfg_event_data_t *)
            qdf_mem_malloc( sizeof(acfg_event_data_t));
        if (acfg_event != NULL) {
            if (soc->recovery_in_progress) {
                /* if recover in-progress then always claim failure */
                acfg_event->result = false;
            } else {
                acfg_event->result = rf_path_ctx->is_fw_resp_success;
            }

            QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
                    QDF_TRACE_LEVEL_DEBUG, FL("Send ACFG event"));
            acfg_send_event(scn->sc_osdev->netdev,
                    scn->sc_osdev, WL_EVENT_TYPE_RF_PATH_CHANGE_STATUS,
                    acfg_event);
            qdf_mem_free(acfg_event);
        }
    }
#endif

    rf_path_ctx->is_switch_in_progress = false;

    rf_path_ctx->ts_end = qdf_ktime_to_ns(qdf_ktime_get());
            QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH, QDF_TRACE_LEVEL_DEBUG,
                      FL("-----------------------"));
            QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH, QDF_TRACE_LEVEL_DEBUG,
                      FL("time_start   : %lld (ms)"), rf_path_ctx->ts_start);
            QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH, QDF_TRACE_LEVEL_DEBUG,
                      FL("time_end    : %lld (ms)"), rf_path_ctx->ts_end);

    return status;
}

int
ol_ath_soc_rf_path_event_handler(ol_scn_t sc,
                                 u_int8_t *data,
                                 u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc           = (ol_ath_soc_softc_t *) sc;
    ol_ath_rf_path_ctx_t *rf_path_ctx = &soc->rf_path_ctx;
    struct wmi_unified *wmi_handle;
    uint32_t fw_resp_status;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);

    if (wmi_handle) {
        /* Process return value from FW by extracting
         * response event params from "data". If SUCCESS,
         * set event. If FAILURE, don't set event and
         * command will time out.
         */
        if (wmi_unified_extract_rf_path_resp(wmi_handle,
                    data, &fw_resp_status)
                        == QDF_STATUS_SUCCESS) {
            rf_path_ctx->is_fw_resp_success = !fw_resp_status;
            QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH, QDF_TRACE_LEVEL_INFO,
                      FL("Received response from FW with"""
                         """ status %s"),
                      rf_path_ctx->is_fw_resp_success ? "PASS": "FAIL");
            if (fw_resp_status)
                QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
                          QDF_TRACE_LEVEL_ERROR,
                          FL("fw_resp_status: %d"), fw_resp_status);

            qdf_event_set(&rf_path_ctx->rf_switch_done);
        }
    }

    return 0;
}

/**
 * ol_ath_set_hw_mode_omn_timer(): Set the OMN (Op-Mode Notification) timer
 *                                 for hw-mode change
 * @scn: scn handle
 * @omn_timeout: How long in milliseconds,
 *               will the new OMN IE included in beacon after hw-mode change
 *
 * return: EOK
 * otherwise
 */
int ol_ath_set_hw_mode_omn_timer(struct ol_ath_softc_net80211 *scn,
        uint32_t omn_timeout)
{
    struct ieee80211com     *ic = &scn->sc_ic;

    if (ic->ic_omn_cxt.omn_timeout == omn_timeout) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_WARN,
                "Error!! OMN-TIMER is already set to %d\n", omn_timeout);
        return -EINVAL;
    }
    ic->ic_omn_cxt.omn_timeout = omn_timeout;
    return EOK;
}

/**
 * ol_ath_set_hw_mode_omn_enable(): Enable Opmode notification after
 *                                  hw-mode change
 * @scn: scn handle
 * @enable: Opmode notification enable(1) or disable(0)
 *
 * return: EOK
 * otherwise
 */
int ol_ath_set_hw_mode_omn_enable(struct ol_ath_softc_net80211 *scn,
				  int enable)
{
    struct ieee80211com     *ic = &scn->sc_ic;

    if ((!ic->ic_omn_cxt.omn_enable) == (!enable)) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_WARN,
                "Error!! OMN-TIMER is already in  %d state\n", (!(!enable)));
        return -EINVAL;
    }
    ic->ic_omn_cxt.omn_enable = (!(!enable));
    return EOK;
}

/**
 * @scn: scn handle
 * @is_primary: indicate that interface is primary (only '1' is allowed)
 */
QDF_STATUS ol_ath_set_hw_mode_primary_if(struct ol_ath_softc_net80211 *scn,
                                         unsigned int is_primary)
{
    ol_ath_soc_softc_t *soc = scn->soc;
    ol_ath_hw_mode_ctx_t *hw_mode_ctx = &soc->hw_mode_ctx;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211com *ic;
    QDF_STATUS status = QDF_STATUS_E_INVAL;

    if (!ol_ath_is_dynamic_hw_mode_enabled(soc)) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Feature not supported"));
        return status;
    }

    if (hw_mode_ctx->is_switch_in_progress) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_ERROR,
                  "ERROR! Cannot change primary interface when "
                  "hw-mode switch is in progress\n");
        return status;
    }

    if (is_primary != 1) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_ERROR,
                "ERROR! Only 1 is a valid parameter\n"
                "The command for ol_ath_set_hw_mode_primary_if will make "
                "the handler interface as the primary and only '1' is a "
                "valid parameter for this\n");
        return status;
    }

    pdev = scn->sc_pdev;
    if (hw_mode_ctx->primary_pdev == pdev) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_INFO,
                 "INFO! Interface is already primary pdev\n"
                 "Not doing anything\n");
        return status;
    }

    ic = &(scn->sc_ic);
    if (!(ic->ic_supported_bands & WMI_HOST_WLAN_5G_CAPABILITY)) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                QDF_TRACE_LEVEL_ERROR, "ERROR!! Interface is not 5G capable\n");
        return status;
    }

    hw_mode_ctx->primary_pdev = pdev;
    return QDF_STATUS_SUCCESS;
}

/**
 * ol_ath_handle_tqm_reset(): Handle tqm reset enable/disable request
 * @scn: scn handle
 * @enable: enable/disable flag
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
int ol_ath_handle_tqm_reset(struct ol_ath_softc_net80211 *scn, uint32_t enable)
{
    ol_ath_soc_softc_t       *soc           = scn->soc;
    struct wlan_objmgr_psoc  *psoc          = soc->psoc_obj;
    struct target_psoc_info  *tgt_hdl       = NULL;
    struct wmi_unified       *wmi_handle    = NULL;

    /* retrieve tgt_hdl */
    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    if (!tgt_hdl) {
        qdf_err("NULL tgt_hdl");
        return -EINVAL;
    }

    /* retrieve wmi_handle */
    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);

    if (!wmi_handle) {
        qdf_err("NULL wmi_handle");
        return -EINVAL;
    }

    /* make a call to target_if to send tqm_reset_enable_disable wmi */
    if (wmi_unified_soc_tqm_reset_enable_disable_cmd(wmi_handle, enable) ==
                    QDF_STATUS_SUCCESS) {
        soc->tqm_reset_flag = !!enable;
        QDF_TRACE(QDF_MODULE_ID_WMI,
                  QDF_TRACE_LEVEL_DEBUG,
                  FL("tqm reset enable/disable command sent to FW"));
        return 0;
    } else {
        QDF_TRACE(QDF_MODULE_ID_WMI,
                  QDF_TRACE_LEVEL_DEBUG,
                  FL("failed to send tqm reset enable/disable command to FW"));
    }

    return -1;
}

/**
 * ol_ath_handle_hw_mode_switch(): Handle hw-mode swtich request
 * @scn: scn handle
 * @mode: requested target mode
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
int ol_ath_handle_hw_mode_switch(struct ol_ath_softc_net80211 *scn,
                                 uint32_t mode)
{
    ol_ath_soc_softc_t       *soc           = scn->soc;
    struct wlan_objmgr_psoc  *psoc          = soc->psoc_obj;
    ol_ath_hw_mode_ctx_t     *hw_mode_ctx   = &soc->hw_mode_ctx;
    struct target_psoc_info  *tgt_hdl       = NULL;
    struct wmi_unified       *wmi_handle    = NULL;
    int status;

    /* init and sanitize */
    status = ol_ath_init_hw_mode_switch(scn, mode);
    if (status != EOK)
        goto exit;

    /* retrieve tgt_hdl */
    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    /* retrieve wmi_handle */
    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
    if (!wmi_handle) {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("Invalid WMI handle"));
        goto exit;
    }

    if (soc->hw_mode_ctx.dynamic_hw_mode != WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        /* prepare mode-switch */
        status = ol_ath_prepare_hw_mode_switch(soc);
        if (status != EOK)
            goto exit;
    }

    QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG, QDF_TRACE_LEVEL_INFO,
              "Switching to HW mode %d..", mode);

    /* make a call to target_if to send hw_mode_switch wmi
     * as blocking call */
    if (wmi_unified_soc_set_hw_mode_cmd(wmi_handle, mode)
            == QDF_STATUS_SUCCESS) {
        if (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) {
            /*  Block WMI cmds on primary pdev during mode switch */
           hw_mode_ctx->prev_wmi_hdl =
                   lmac_get_pdev_wmi_handle(hw_mode_ctx->primary_pdev);
            ol_ath_allow_wmi_cmds(hw_mode_ctx->prev_wmi_hdl, false);
        }

        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_DEBUG, FL("hw_mode_cmd sent to FW"));

        /* wait for response event from FW */
        if (qdf_wait_for_event_completion(&hw_mode_ctx->event,
                MAX_HW_MODE_FW_RESP_TIME)) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                QDF_TRACE_LEVEL_ERROR, FL("Timed out waiting on FW response"));
            hw_mode_ctx->is_fw_resp_success = false;
        }

        if (!hw_mode_ctx->is_fw_resp_success &&
            (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST)) {
            /*  Allow WMI cmds on primary pdev */
            ol_ath_allow_wmi_cmds(hw_mode_ctx->prev_wmi_hdl, true);
        }
    } else {
        QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
            QDF_TRACE_LEVEL_ERROR, FL("WMI send failed for hw_mode_cmd"));
    }

    if (soc->hw_mode_ctx.dynamic_hw_mode != WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        /* accomplish mode-switch */
        if (ol_ath_accomplish_hw_mode_switch(scn))
            goto exit;
    }

    status = EOK;
exit:
    return ol_ath_deinit_hw_mode_switch(scn, status);
}

void ol_ath_assign_cohosted_ref_bssid(void *i_scn, bool partially_random)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *) i_scn;
    uint8_t *myaddr;
    uint32_t random_bytes;
    struct ieee80211com *ic = &scn->sc_ic;
    uint8_t *ref_bssid;

    if (ic->ic_cohosted_bss_support) {
        ref_bssid = (uint8_t *) &ic->cohosted_ref_bssid;

        wlan_pdev_obj_lock(scn->sc_pdev);
        myaddr = wlan_pdev_get_hw_macaddr(scn->sc_pdev);
        IEEE80211_ADDR_COPY(ref_bssid, myaddr);
        wlan_pdev_obj_unlock(scn->sc_pdev);

        /* 1. Make ref_bssid a locally administered bssid
         * as 48-n MSBs of all the 2^n bssids in a COHOSTED
         * BSS must be same
         * 2. If caller requests partially_random ref_bssid
         * then randomize the last octet of the same. This
         * may be relevant in the following scenario. If
         * COHOSTED BSS is enabled in multiple radios,
         * randomizing the last octet will enable to have
         * unique set of bssids across all the radios irr-
         * espctive of whether thse radios are across diff-
         * erent SOCs or not
         */
        if (partially_random) {
            OS_GET_RANDOM_BYTES(&random_bytes, sizeof(random_bytes));
            ref_bssid[ATH_BSSID_INDEX] =
                        (random_bytes + OS_GET_TICKS()) % ((1 << OCTET) - 1);
        }
        ref_bssid[0] |= IEEE802_MAC_LOCAL_ADMBIT;

        qdf_info("COHOSTED reference bssid is %s soc: %s pdev: %s",
                ether_sprintf(ref_bssid), scn->soc->sc_osdev->netdev->name,
                scn->netdev->name);
    }
}

/**
 * ol_ath_init_rf_path_switch(): Reset rf_path_ctx and perform
 * sanity check before proceeding with hw-mode change. Following
 * aret the sanity checks:
 * 	1. Is dynamic rf path switch supported by fw and if the
 * 	feature is enabled in host?
 * 	2. Is recovery in progress?
 * 	3. Is tgt_hdl retrieval success?
 * 	4. Is wmi_handle retrieval success?
 * 	5. Is current_index equal to requested index?
 * @scn: scn handle
 * @rd_path_index: requested target index
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_init_rf_path_switch(struct ol_ath_softc_net80211 *scn,
                                      uint32_t rf_path_index)
{
    ol_ath_soc_softc_t       *soc         = NULL;
    struct wlan_objmgr_psoc  *psoc        = NULL;
    ol_ath_rf_path_ctx_t     *rf_path_ctx = NULL;
    struct target_psoc_info  *tgt_hdl     = NULL;
    struct wmi_unified       *wmi_handle  = NULL;
    int status                            = -EINVAL;

    if (!scn)
        goto exit;

    soc = scn->soc;
    if (!soc)
        goto exit;

    rf_path_ctx = &soc->rf_path_ctx;
    psoc = soc->psoc_obj;
    if (!psoc)
        goto exit;

    if (wlan_pdev_nif_feat_ext_cap_get(scn->sc_ic.ic_pdev_obj,
                                       WLAN_PDEV_FEXT_OVERLAPPING_FREQ) == 0) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
                  QDF_TRACE_LEVEL_ERROR, FL("Feature not supported"));
        goto exit;
    }

    if (soc->recovery_in_progress) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_ERROR, FL("Recovery in-progress. Try again "
                                      "after recovery-complete"));
        goto exit;
    }

    if (rf_path_ctx->current_index == rf_path_index) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_ERROR, FL("Target index same as current"));
        goto exit;
    }

    if (wlan_pdev_wait_to_bringdown_vdevs(&scn->sc_ic, ALL_VDEVS)) {
                QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
                    QDF_TRACE_LEVEL_ERROR,
                    FL("Failed to bring down all vaps for pdev"));
                goto exit;
    }

    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_ERROR, FL("target hdl is null"));
        goto exit;
    }

    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
    if (!wmi_handle) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_ERROR, FL("Invalid WMI handle"));
        goto exit;
    }

    /* set mode_switch flag */
    rf_path_ctx->is_switch_in_progress      = true;

    /* assign target_mode as the requested mode */
    rf_path_ctx->target_index                = rf_path_index;

    /* reset event */
    qdf_event_reset(&rf_path_ctx->rf_switch_done);

    rf_path_ctx->ts_start = qdf_ktime_to_ns(qdf_ktime_get());
    status = EOK;

exit:
    return status;
}

/**
 * ol_ath_prepare_rf_path_switch(): Prepare host for mode switch
 * before sending rf_path_cmd to FW
 * @scn: scn handle
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_prepare_rf_path_switch(struct ol_ath_softc_net80211 *scn)
{
    ol_ath_soc_softc_t       *soc         = scn->soc;
    struct wlan_objmgr_psoc  *psoc        = soc->psoc_obj;
    struct target_psoc_info  *tgt_hdl       = NULL;
    int status                              = -EINVAL;

    /* retrieve tgt_hdl */
    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    ol_ath_rf_if_setup(scn, DOWN);

    /* dfs de-init */
    status = ol_if_dfs_pdev_deinit_pre_mode_switch(scn);
    if (status != QDF_STATUS_SUCCESS) {
        ol_ath_rf_if_setup(scn, UP);
        return status;
    }

    ol_if_dfs_psoc_deinit_pre_mode_switch(psoc);
    status = EOK;

    return status;
}

/**
 * ol_ath_post_rf_path_switch(): Post RF path selection handling
 * after receiving FW response for rf path switch command
 * @scn: scn handle
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
static int ol_ath_post_rf_path_switch(struct ol_ath_softc_net80211 *scn)
{
    ol_ath_soc_softc_t *soc = scn->soc;
    ol_ath_rf_path_ctx_t *rf_path_ctx = &soc->rf_path_ctx;
    struct wlan_psoc_host_service_ext_param *svc_param;
    struct wlan_psoc_host_service_ext2_param *svc_ext2_param;
    struct target_psoc_info *tgt_psoc;
    struct wlan_objmgr_pdev *pdev = scn->sc_pdev;
    QDF_STATUS status;

    tgt_psoc = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);

    if (rf_path_ctx->is_fw_resp_success) {

        /* copy freq. range of the selected target rf path index */
        if (rf_path_ctx->target_index == SECONDARY_RF_BAND) {

            svc_ext2_param = target_psoc_get_service_ext2_param(tgt_psoc);
            if (!svc_ext2_param) {
                qdf_err("svc_ext2_param is NULL");
                return -EINVAL;
            }

            /* copy values from ext2 event */
            status = ucfg_reg_update_hal_reg_range_caps(soc->psoc_obj,
                                                 svc_ext2_param->low_2ghz_chan_ext,
                                                 svc_ext2_param->high_2ghz_chan_ext,
                                                 svc_ext2_param->low_5ghz_chan_ext,
                                                 svc_ext2_param->high_5ghz_chan_ext,
                                                 wlan_objmgr_pdev_get_pdev_id(pdev));
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("Failed to update hal reg cap");
                return -EINVAL;
            }
        } else {
            svc_param = target_psoc_get_service_ext_param(tgt_psoc);
            if (!svc_param) {
                qdf_err("svc_param is NULL");
                return -EINVAL;
            }

            /* copy values from ext event */
            status = ucfg_reg_update_hal_reg_range_caps(soc->psoc_obj,
                                                 svc_param->low_2ghz_chan,
                                                 svc_param->high_2ghz_chan,
                                                 svc_param->low_5ghz_chan,
                                                 svc_param->high_5ghz_chan,
                                                 wlan_objmgr_pdev_get_pdev_id(pdev));
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("Failed to update hal reg cap");
                return -EINVAL;
            }
        }

        /* update channel list */
        ol_regdmn_reinit_post_mode_switch(&scn->sc_ic);

    } /* resp_success */

    ol_if_dfs_pdev_reinit_post_mode_switch(&scn->sc_ic);
    ol_ath_rf_if_setup(scn, UP);

    return EOK;
}

/**
 * ol_ath_handle_rf_path_switch(): Handle OF- band swtich request
 * @scn: scn handle
 * @mode: requested target mode
 *
 * return: status - value zero in case of success and non-zero
 * otherwise
 */
int ol_ath_handle_rf_path_switch(struct ol_ath_softc_net80211 *scn,
                                 uint32_t mode)
{
    ol_ath_soc_softc_t       *soc           = scn->soc;
    struct wlan_objmgr_psoc  *psoc          = soc->psoc_obj;
    ol_ath_rf_path_ctx_t     *rf_path_ctx   = &soc->rf_path_ctx;
    struct target_psoc_info  *tgt_hdl       = NULL;
    struct wmi_unified       *wmi_handle    = NULL;
    int status;

    /* init and sanitize */
    status = ol_ath_init_rf_path_switch(scn, mode);
    if (status != EOK)
        goto exit;

    /* retrieve tgt_hdl */
    tgt_hdl = (struct target_psoc_info *) wlan_psoc_get_tgt_if_handle(psoc);

    /* retrieve wmi_handle */
    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
    if (!wmi_handle) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_ERROR, FL("Invalid WMI handle"));
        goto exit;
    }
    QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH, QDF_TRACE_LEVEL_INFO,
              "Switching to OF band  %d..", mode);

    status = ol_ath_prepare_rf_path_switch(scn);
    if (status != EOK)
        goto exit;

    /* make a call to target_if to send hw_mode_switch wmi
     * as blocking call */
    if (wmi_unified_soc_set_rf_path_cmd(wmi_handle, mode,
                                        wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev))
                                        == QDF_STATUS_SUCCESS) {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_DEBUG, FL("rf_path_cmd sent to FW"));

        /* wait for response event from FW */
        if (qdf_wait_for_event_completion(&rf_path_ctx->rf_switch_done,
                MAX_RF_PATH_FW_RESP_TIME)) {
            QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
                QDF_TRACE_LEVEL_ERROR, FL("Timed out waiting on FW response"));
            rf_path_ctx->is_fw_resp_success = false;
        }
   } else {
        QDF_TRACE(QDF_MODULE_ID_RF_PATH_SWITCH,
            QDF_TRACE_LEVEL_ERROR, FL("WMI send failed for hw_mode_cmd"));
    }

    /* post rf switch handling */
    ol_ath_post_rf_path_switch(scn);

exit:
    return ol_ath_deinit_rf_path_switch(scn, status);
}

void ol_ath_assign_mbssid_ref_bssid(void *i_scn, bool partially_random)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *) i_scn;
    uint8_t *myaddr;
    uint32_t random_bytes;
    struct ieee80211com *ic = &scn->sc_ic;
    uint8_t *ref_bssid;

    if (ic->ic_mbss_ctx) {
        ref_bssid = (uint8_t *) &ic->ic_mbss_ctx->ref_bssid;

        wlan_pdev_obj_lock(scn->sc_pdev);
        myaddr = wlan_pdev_get_hw_macaddr(scn->sc_pdev);
        IEEE80211_ADDR_COPY(ref_bssid, myaddr);
        wlan_pdev_obj_unlock(scn->sc_pdev);

        /* 1. Make ref_bssid a locally administered bssid
         * as 48-n MSBs of all the 2^n bssids in a MBSSID
         * group must be same
         * 2. If caller requests partially_random ref_bssid
         * then randomize the last octet of the same. This
         * may be relevant in the following scenario. If
         * MBSSID (or EMA) is enabled in multiple radios,
         * randomizing the last octet will enable to have
         * unique set of bssids across all the radios irr-
         * espctive of whether thse radios are across diff-
         * erent SOCs or not
         *
         * Currently, one octet should suffice as n is
         * expected to be less than or equal to 8 for all
         * practical purposes for a forseeable future
         */
        if (partially_random) {
            OS_GET_RANDOM_BYTES(&random_bytes, sizeof(random_bytes));
            ref_bssid[ATH_BSSID_INDEX] =
                        (random_bytes + OS_GET_TICKS()) % ((1 << OCTET) - 1);
        }
        ref_bssid[0] |= IEEE802_MAC_LOCAL_ADMBIT;

        mbss_info("MBSSID reference bssid is %s soc: %s pdev: %s",
                ether_sprintf(ref_bssid), scn->soc->sc_osdev->netdev->name,
                scn->netdev->name);
    }
}

QDF_STATUS ol_ath_get_ulmumimo_support(struct ieee80211vap *vap,
                                       bool *ulmumimo_support)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;
    struct wlan_objmgr_psoc *psoc;
    struct target_psoc_info *tgt_hdl;
    struct ieee80211_ath_channel *ic_curchan;
    struct wlan_psoc_host_service_ext2_param *service_param;
    enum ieee80211_opmode opmode;

    if (!vap || !ulmumimo_support || !vap->iv_ic || !vap->iv_ic->ic_curchan)
        return QDF_STATUS_E_NULL_VALUE;

    *ulmumimo_support = false;
    ic_curchan = vap->iv_ic->ic_curchan;

    opmode = ieee80211_vap_get_opmode(vap);
    if ((opmode != IEEE80211_M_HOSTAP) &&
        (opmode != IEEE80211_M_STA))
        return QDF_STATUS_E_NOSUPPORT;

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    soc = scn->soc;
    psoc = soc->psoc_obj;
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        return -EINVAL;
    }

    service_param = &tgt_hdl->info.service_ext2_param;
    if (opmode == IEEE80211_M_HOSTAP) {
        if (IEEE80211_IS_CHAN_2GHZ(ic_curchan))
            *ulmumimo_support = service_param->ul_mumimo_rx_2g;
        else if (IEEE80211_IS_CHAN_5GHZ(ic_curchan))
            *ulmumimo_support = service_param->ul_mumimo_rx_5g;
        else if (IEEE80211_IS_CHAN_6GHZ(ic_curchan))
            *ulmumimo_support = service_param->ul_mumimo_rx_6g;
        else
            return QDF_STATUS_E_NOSUPPORT;
    } else {
        if (IEEE80211_IS_CHAN_2GHZ(ic_curchan))
            *ulmumimo_support = service_param->ul_mumimo_tx_2g;
        else if (IEEE80211_IS_CHAN_5GHZ(ic_curchan))
            *ulmumimo_support = service_param->ul_mumimo_tx_5g;
        else if (IEEE80211_IS_CHAN_6GHZ(ic_curchan))
            *ulmumimo_support = service_param->ul_mumimo_tx_6g;
        else
            return QDF_STATUS_E_NOSUPPORT;
    }
    return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_11BE_MLO
bool ol_ath_target_check_mlo_recovery(struct wlan_objmgr_psoc *psoc)
{
   ol_ath_soc_softc_t *soc = NULL;

   if (!psoc)
       return 0;

   soc = lmac_get_psoc_feature_ptr(psoc);
   if (!soc)
       return 0;

   if (soc->recovery_in_progress == 1 &&
       soc->recovery_type != MLO_RECOVERY_MODE_0)
         return 1;

   return 0;
}

bool ol_ath_check_mlo_grp_recovery(struct wlan_objmgr_psoc *psoc, bool check_all_soc)
{
    struct wlan_objmgr_psoc *grp_soc_list[MAX_MLO_CHIPS];
    ol_ath_soc_softc_t *temp_soc;
    uint8_t soc_idx;
    uint8_t grp_id;
    uint8_t tot_grp_socs;

    grp_id = wlan_mlo_get_psoc_group_id(psoc);

    tot_grp_socs = mlo_setup_get_total_socs(grp_id);
    if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
         return false;

    mlo_get_soc_list(grp_soc_list, grp_id, tot_grp_socs, WLAN_MLO_GROUP_DEFAULT_SOC_LIST);

    for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
         if (!grp_soc_list[soc_idx])
             continue;
         if (!check_all_soc && grp_soc_list[soc_idx] == psoc)
             continue;
         temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
         if (temp_soc && temp_soc->recovery_in_progress && !temp_soc->retain_mlo_peer) {
              qdf_info("Target Recovery is in progress for soc Id %d ",
                                 grp_soc_list[soc_idx]->soc_objmgr.psoc_id);
              return true;
         }
    }

    return false;
}

bool ol_ath_check_mlo_assoc_on_recovery(struct wlan_objmgr_psoc *psoc, bool check_all_soc)
{

    ol_ath_soc_softc_t *temp_soc = NULL;

    temp_soc = lmac_get_psoc_feature_ptr(psoc);
    if (!temp_soc)
         return false;

    if (temp_soc->recovery_in_progress &&
        temp_soc->retain_mlo_peer)
           return true;
    else
           return ol_ath_check_mlo_grp_recovery(psoc, check_all_soc);
}
qdf_export_symbol(ol_ath_check_mlo_assoc_on_recovery);

QDF_STATUS ol_ath_get_eml_phy_cap(struct ieee80211com *ic,
                                  struct wlan_mlo_eml_cap *phy_emlcap)
{

    struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2;
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;
    struct wlan_objmgr_psoc *psoc;
    struct target_psoc_info *tgt_hdl;

    if (!ic || !phy_emlcap)
        return QDF_STATUS_E_NULL_VALUE;

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc = scn->soc;
    psoc = soc->psoc_obj;
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        return -EINVAL;
    }

    mac_phy_caps_ext2 = target_psoc_get_mac_phy_cap_ext2(tgt_hdl);
    if (!mac_phy_caps_ext2)
        return QDF_STATUS_E_NULL_VALUE;

    qdf_mem_zero(phy_emlcap, sizeof(struct wlan_mlo_eml_cap));
    qdf_mem_copy(phy_emlcap, &mac_phy_caps_ext2->emlcap, sizeof(struct wlan_mlo_eml_cap));

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_get_mld_phy_cap(struct ieee80211com *ic,
                                  struct wlan_mlo_mld_cap *phy_mldcap)
{

    struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2;
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;
    struct wlan_objmgr_psoc *psoc;
    struct target_psoc_info *tgt_hdl;

    if (!ic || !phy_mldcap)
        return QDF_STATUS_E_NULL_VALUE;

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc = scn->soc;
    psoc = soc->psoc_obj;
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        return -EINVAL;
    }

    mac_phy_caps_ext2 = target_psoc_get_mac_phy_cap_ext2(tgt_hdl);
    if (!mac_phy_caps_ext2)
        return QDF_STATUS_E_NULL_VALUE;

    qdf_mem_zero(phy_mldcap, sizeof(struct wlan_mlo_mld_cap));
    qdf_mem_copy(phy_mldcap, &mac_phy_caps_ext2->mldcap, sizeof(struct wlan_mlo_mld_cap));

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_get_msd_phy_cap(struct ieee80211com *ic,
                                  struct wlan_mlo_msd_cap *phy_msdcap)
{

    struct wlan_psoc_host_mac_phy_caps_ext2 *mac_phy_caps_ext2;
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;
    struct wlan_objmgr_psoc *psoc;
    struct target_psoc_info *tgt_hdl;

    if (!ic || !phy_msdcap)
        return QDF_STATUS_E_NULL_VALUE;

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc = scn->soc;
    psoc = soc->psoc_obj;
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        target_if_err("target hdl is null");
        return -EINVAL;
    }

    mac_phy_caps_ext2 = target_psoc_get_mac_phy_cap_ext2(tgt_hdl);
    if (!mac_phy_caps_ext2)
        return QDF_STATUS_E_NULL_VALUE;

    qdf_mem_zero(phy_msdcap, sizeof(struct wlan_mlo_msd_cap));
    qdf_mem_copy(phy_msdcap, &mac_phy_caps_ext2->msdcap, sizeof(struct wlan_mlo_msd_cap));

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_get_msd_info_override_enable(struct ieee80211com *ic,
                                     bool *msd_info_override_en)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_INVAL;
    }

    soc = scn->soc;
    if (!soc) {
        qdf_err("soc is NULL");
        return QDF_STATUS_E_INVAL;
    }
    *msd_info_override_en = soc->msd_info_override_enable;

    return QDF_STATUS_SUCCESS;

}

QDF_STATUS ol_ath_get_msd_info_present_override(struct ieee80211com *ic,
                                     bool *msd_info_prsnt_override)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_INVAL;
    }

    soc = scn->soc;
    if (!soc) {
        qdf_err("soc is NULL");
        return QDF_STATUS_E_INVAL;
    }
    *msd_info_prsnt_override = soc->msd_info_present_override;

    return QDF_STATUS_SUCCESS;

}

QDF_STATUS ol_ath_get_msd_info_value_override(struct ieee80211com *ic,
                                     uint16_t *msd_info_val_override)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_INVAL;
    }

    soc = scn->soc;
    if (!soc) {
        qdf_err("soc is NULL");
        return QDF_STATUS_E_INVAL;
    }
    *msd_info_val_override = soc->msd_info_value_override;

    return QDF_STATUS_SUCCESS;

}
#endif

#ifdef WLAN_FEATURE_11BE
QDF_STATUS ol_ath_get_t2lm_enabled(struct ieee80211com *ic,
                                   uint8_t *t2lm_enable)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        t2lm_err("scn is NULL");
        return QDF_STATUS_E_INVAL;
    }

    soc = scn->soc;
    if (!soc) {
        t2lm_err("soc is NULL");
        return QDF_STATUS_E_INVAL;
    }

    *t2lm_enable = (int8_t)soc->t2lm_enable;

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_get_default_t2lm_response(struct ieee80211com *ic,
                                            uint8_t *default_t2lm_resp)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        t2lm_err("scn is NULL");
        return QDF_STATUS_E_INVAL;
    }

    soc = scn->soc;
    if (!soc) {
        t2lm_err("soc is NULL");
        return QDF_STATUS_E_INVAL;
    }

    *default_t2lm_resp = (int8_t)soc->default_t2lm_resp;

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_get_enable_eml_cap(struct ieee80211com *ic,
                                     bool *enable_eml_cap)
{
    struct ol_ath_softc_net80211 *scn;
    ol_ath_soc_softc_t *soc;

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return QDF_STATUS_E_INVAL;
    }

    soc = scn->soc;
    if (!soc) {
        qdf_err("soc is NULL");
        return QDF_STATUS_E_INVAL;
    }
    *enable_eml_cap = soc->enable_eml_cap;

    return QDF_STATUS_SUCCESS;

}
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
static void ol_ath_enable_telemetry_stats(struct wlan_objmgr_pdev *pdev)
{
    QDF_STATUS status;
    struct infra_cp_stats_cmd_info info = {0};

    info.stats_id = TYPE_REQ_CTRL_PATH_PMLO_STAT;
    info.action = ACTION_REQ_CTRL_PATH_STAT_PERIODIC_PUBLISH;
    info.request_id = info.stats_id;
    info.stat_periodicity = 1000;
    info.num_pdev_ids = 1;
    info.pdev_id[0] = wlan_objmgr_pdev_get_pdev_id(pdev);
    status = ucfg_send_telemetry_cp_stats_request(pdev, &info);
}
#else
static void ol_ath_enable_telemetry_stats(struct wlan_objmgr_pdev *pdev)
{ }
#endif

#ifdef CONFIG_SAWF
#ifdef SAWF_ADMISSION_CONTROL
static void
ol_ath_process_sawf_peer_msduq_event(void *pdev_hdl, enum WDI_EVENT event,
                                     void *data, u_int16_t peer_id,
                                     uint32_t status)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct cdp_sawf_peer_msduq_event_intf *params = NULL;

    params = (struct cdp_sawf_peer_msduq_event_intf *)data;

    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, params->vdev_id,
                                                WLAN_SAWF_ID);
    if (!vdev)
        return;

    ol_ath_send_sawf_peer_msduq_event(vdev, params);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_SAWF_ID);
}

static QDF_STATUS
ol_ath_subscribe_sawf_peer_msduq_event(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle)
        return QDF_STATUS_E_FAILURE;

    scn->sawf_peer_msduq_event_subscriber.callback = ol_ath_process_sawf_peer_msduq_event;
    scn->sawf_peer_msduq_event_subscriber.context = scn->sc_pdev;

    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->sawf_peer_msduq_event_subscriber,
                          WDI_EVENT_PEER_MSDUQ_EVENT))
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ol_ath_unsubscribe_sawf_peer_msduq_event(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle)
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->sawf_peer_msduq_event_subscriber,
                            WDI_EVENT_PEER_MSDUQ_EVENT))
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}
#endif

static inline
enum sawf_ul_op ieee80211_ucfg_fse_to_sawf_event(uint32_t fse_event)
{
    switch (fse_event) {
    case dp_rx_fse_event_add:
        return SAWF_UL_OP_ADD;
    case dp_rx_fse_event_mismatch:
        return SAWF_UL_OP_UPDATE;
    case dp_rx_fse_event_del:
        return SAWF_UL_OP_DELETE;
    default:
        return SAWF_UL_OP_INVALID;
    }
}

static void
ol_ath_sawf_fse_indication(void *pdev_hdl, enum WDI_EVENT event,
                           void *data, u_int16_t peer_id,
                           uint32_t status)
{
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct fse_info_cookie *fse_info = (struct fse_info_cookie *)data;

    qdf_debug("FSE event pdev_id %u status %u, svc_id %u, tid %u, mac %pM",
              wlan_objmgr_pdev_get_pdev_id(pdev), status,
              fse_info->svc_id, fse_info->tid, fse_info->dest_mac);

    ieee80211_ucfg_send_ul_sawf_params(pdev, fse_info->dest_mac,
                                       fse_info->svc_id, fse_info->tid,
                                       ieee80211_ucfg_fse_to_sawf_event(status));
}

static QDF_STATUS
ol_ath_subscribe_sawf_fse_wdi_event(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle)
        return QDF_STATUS_E_FAILURE;

    scn->sawf_config_ul_subscriber.callback = ol_ath_sawf_fse_indication;
    scn->sawf_config_ul_subscriber.context = scn->sc_pdev;

    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->sawf_config_ul_subscriber,
                          WDI_EVENT_FSE_UPDATE))
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
ol_ath_unsubscribe_sawf_fse_wdi_event(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (!soc_txrx_handle)
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->sawf_config_ul_subscriber,
                            WDI_EVENT_FSE_UPDATE))
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}
#endif

int ol_ath_pdev_attach(struct ol_ath_softc_net80211 *scn,
		IEEE80211_REG_PARAMETERS *ieee80211_conf_parm,
		uint8_t phy_id)
{
    ol_ath_soc_softc_t *soc = scn->soc;
    struct ieee80211com     *ic = &scn->sc_ic;
    int status = 0;
    osdev_t osdev = scn->sc_osdev;
    uint32_t host_num_peers_psoc = 0;
    uint32_t host_num_peers_pdev = 0;
    uint32_t max_peers = 0;
#if DBDC_REPEATER_SUPPORT
    int i,j,k;
    struct ieee80211com     *tmp_ic;
#endif
#if ATH_SUPPORT_DSCP_OVERRIDE
    int map_id;
#endif
    struct wlan_objmgr_psoc *psoc = NULL;
    struct target_psoc_info *tgt_hdl;
    target_resource_config *tgt_cfg;
    uint32_t target_type;
    uint32_t max_target_connected_peers;
    uint32_t target_version;
    uint8_t *myaddr;
    struct wmi_unified *wmi_handle;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    struct wlan_lmac_if_dfs_rx_ops *dfs_rx_ops;
#endif /* HOST_DFS_SPOOF_TEST */
    uint8_t num_radios;
    ol_txrx_soc_handle soc_txrx_handle;
#if OL_ATH_SUPPORT_LED
    bool led_gpio_enable_8074 = cfg_get(soc->psoc_obj, CFG_OL_LED_GPIO_ENABLE_8074);
    struct wlan_psoc_host_hal_reg_capabilities_ext *reg_cap = ucfg_reg_get_hal_reg_cap(scn->soc->psoc_obj);
    uint8_t pdev_idx = lmac_get_pdev_idx(scn->sc_pdev);
    int led_gpio;
    struct platform_device *plat_dev;
    struct device_node *dev_node;
    unsigned int active_low;
#endif
    bool is_prealloc_disabled;

    psoc = soc->psoc_obj;
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target handle is null");
        return -EINVAL;
    }
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

#if defined(QCA_OL_TX_MULTIQ_SUPPORT) || defined(QCA_OL_RX_MULTIQ_SUPPORT)
    /* Enable multiq support for lithium */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    ic->multiq_support_enabled = cfg_get(psoc, CFG_TGT_MULTIQ_SUPPORT);
#endif
#endif

    tgt_cfg = target_psoc_get_wlan_res_cfg(tgt_hdl);
    target_type = target_psoc_get_target_type(tgt_hdl);
    target_version = target_psoc_get_target_ver(tgt_hdl);
    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
    num_radios = target_psoc_get_num_radios(tgt_hdl);

    /**
     * Update edge channel deprioritize value based on the target type.
     * Currently, all lithium targets except TARGET_TYPE_QCN9000 and TARGET_TYPE_QCN6122
     * will be undergoing edge channel deprioritization.
     * More target types can be added to whitelist as needed.
     */
    if (ol_target_lithium(psoc) &&
       (target_type != TARGET_TYPE_QCN9000 && target_type != TARGET_TYPE_QCN9224 &&
        target_type != TARGET_TYPE_QCN9160 && target_type != TARGET_TYPE_QCN6432)) {
        ic->ic_edge_ch_dep_applicable = true;
    } else {
        ic->ic_edge_ch_dep_applicable = false;
    }

    ic->interface_id = scn->radio_id;
    qdf_debug("interface_id %d", ic->interface_id);

    ic->ic_auth_tx_xretry = 0;
#if ATH_SUPPORT_DSCP_OVERRIDE
    /* Initializing tid-map parameters to default vaules. */
    for (map_id = 0; map_id < NUM_DSCP_MAP; map_id++) {
        OS_MEMCPY(ic->ic_dscp_tid_map[map_id],dscp_tid_map, sizeof(uint32_t) * WMI_HOST_DSCP_MAP_MAX);
    }
    ic->ic_override_dscp = 0x00;
    ic->ic_dscp_hmmc_tid = 0x00;
    ic->ic_override_hmmc_dscp = 0x00;
#endif
    ic->ic_consider_obss_long_slot = CONSIDER_OBSS_LONG_SLOT_DEFAULT;

    /* init IEEE80211_DPRINTF_IC control object */
    ieee80211_dprintf_ic_init(ic);

    /* init for cohosted bss mode */
    ol_ath_cohost_bssid_ind_init(scn);

    /* initializing non-ap bit map */
    qdf_mem_zero((void *) &ic->ic_non_ap_bssid_index_bmap,
                 sizeof(ic->ic_non_ap_bssid_index_bmap));

    if (!bypasswmi) {
        // Use attach_failed1 for failures beyond this
        /*
         * 9. WLAN/UMAC initialization
         */
        ic->ic_get_num_clients = ol_ath_get_num_clients;
        ic->ic_if_mgmt_drain = ol_if_mgmt_drain;
        ic->ic_is_macreq_enabled = ol_ath_net80211_is_macreq_enabled;
        ic->ic_is_macreq_ext_active = ol_ath_net80211_is_macreq_ext_active;
        ic->ic_get_mac_prealloc_idmask = ol_ath_net80211_get_mac_prealloc_idmask;
        ic->ic_set_mac_prealloc_id = ol_ath_net80211_set_mac_prealloc_id;
        ic->ic_get_bsta_fixed_idmask = ol_ath_net80211_get_bsta_fixed_idmask;
        ic->ic_update_target_caps = ol_ath_update_phymode_caps;
        ic->ic_config_update = ol_ath_pdev_config_update;
        ic->ic_osdev = osdev;
        ic->ic_qdf_dev = soc->qdf_dev;
        ic->ic_netdev = scn->netdev;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
        ic->ic_is_host_dfs_check_enabled = ol_if_is_host_dfs_check_support_enabled;
#endif /* HOST_DFS_SPOOF_TEST */

        /* register bcn_tx_status_event_handler */
        ic->ic_register_beacon_tx_status_event_handler =
                        ol_ath_mgmt_register_offload_beacon_tx_status_event;
        ic->ic_config_bsscolor_offload                 =
                                ol_ath_config_bss_color_offload;
        ic->ic_is_ifce_allowed_in_dynamic_mode         =
                                ol_ath_is_ifce_allowed_in_dynamic_hw_mode;
        ic->ic_assign_mbssid_ref_bssid                 =
                                ol_ath_assign_mbssid_ref_bssid;
        ic->ic_assign_cohosted_ref_bssid                 =
                                ol_ath_assign_cohosted_ref_bssid;

#if defined(IPA_OFFLOAD)
        ic->ic_ipa_wds_peer_disconnect = ol_ath_ipa_wds_peer_disconnect;
#endif
        /* Update the peer count variable
         * The peer count configured is expected to be the max number
         * of client peers connected excluding the number of self & bss peers
         */
        max_target_connected_peers = tgt_cfg->num_peers - tgt_cfg->num_vdevs;
        max_peers = cfg_get(soc->psoc_obj, CFG_OL_MAX_PEERS);
        if(max_peers) {
            if (max_peers < num_radios) {
                qdf_info("Very low max peers configured. Setting max_peers to %d",
                       num_radios);
                host_num_peers_psoc = num_radios;
            }
            else {
                 host_num_peers_psoc = QDF_MIN(max_peers, max_target_connected_peers);
            }
        }
        else {
            host_num_peers_psoc = max_target_connected_peers;
        }
        qdf_atomic_set(&scn->peer_count, 0);
        qdf_atomic_inc(&scn->peer_count);

        /* This is required to make sure object manager psoc
         * peer count has room for connected peers as
         * well as bss peers*2 for vdevs.
         * We need 2 times the number of bss_peers
         * since during bss reset, a new peer is created
         * and then old peer is removed.
        */
#define MAX_BSS_PER_PER_VDEV 2
        if(max_peers) {
            wlan_psoc_set_max_peer_count(psoc,
            host_num_peers_psoc + (tgt_cfg->num_vdevs * MAX_BSS_PER_PER_VDEV));
        }
#if ATH_SUPPORT_WRAP
        /* This adjustment is made so that the number of peers
         * in host and FW become equal. Host is not sending a peer
         * create event when creating a sta which leads to mismatch
         * in peer count between host and FW */
        if (wlan_rptr_psoc_is_max_qwrap(psoc)) {
            host_num_peers_psoc = host_num_peers_psoc - 1;
        }
#endif
        host_num_peers_pdev = (host_num_peers_psoc/num_radios);

        qdf_atomic_add(host_num_peers_pdev, &scn->peer_count);

        ol_ath_pdev_regdmn_init(scn, ieee80211_conf_parm);
        wlan_pdev_obj_lock(scn->sc_pdev);
        myaddr = wlan_pdev_get_hw_macaddr(scn->sc_pdev);
        IEEE80211_ADDR_COPY(ic->ic_myaddr, myaddr);
        IEEE80211_ADDR_COPY(ic->ic_my_hwaddr, myaddr);
        wlan_pdev_obj_unlock(scn->sc_pdev);

        ol_ath_setup_rates(ic);
        ieee80211com_set_cap_ext(ic, IEEE80211_CEXT_PERF_PWR_OFLD);

        ic->ic_dprintf_ic = IEEE80211_DPRINTF_IC;
        ic->ru26_tolerant = true;

        status = ol_ath_dev_attach(scn, ieee80211_conf_parm);
        if (status) {
            qdf_err("ol_ath_dev_attach failed status %d", status);
            return status;
        }

#if UMAC_SUPPORT_CFG80211
        ic->ic_cfg80211_radio_attach = ieee80211_cfg80211_radio_attach;
        ic->ic_cfg80211_radio_detach = ieee80211_cfg80211_radio_detach;
        ic->ic_cfg80211_update_channel_list = wlan_cfg80211_update_channel_list;

        ic->ic_cfg80211_radio_handler.setcountry = ol_ath_ucfg_set_country ;
        ic->ic_cfg80211_radio_handler.getcountry = ol_ath_ucfg_get_country ;
        ic->ic_cfg80211_radio_handler.sethwaddr = ol_ath_ucfg_set_mac_address ;
        ic->ic_cfg80211_radio_handler.gethwaddr = NULL ;
        ic->ic_cfg80211_radio_handler.setparam = ol_ath_ucfg_setparam ;
        ic->ic_cfg80211_radio_handler.getparam = ol_ath_ucfg_getparam ;
        ic->ic_cfg80211_radio_handler.extended_commands = ol_ath_extended_commands;

#if UNIFIED_SMARTANTENNA
        ic->ic_cfg80211_radio_handler.set_saparam = ol_ath_ucfg_set_smart_antenna_param;
        ic->ic_cfg80211_radio_handler.get_saparam = ol_ath_ucfg_get_smart_antenna_param;
#endif

#if ATH_SUPPORT_DSCP_OVERRIDE
        ic->ic_cfg80211_radio_handler.setdscp_tid_map = NULL ;
        ic->ic_cfg80211_radio_handler.getdscp_tid_map = NULL ;
#endif
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
        ic->ic_cfg80211_radio_handler.ald_getStatistics = NULL ;
#endif

#if PEER_FLOW_CONTROL
        ic->ic_cfg80211_radio_handler.txrx_peer_stats = ol_ath_ucfg_txrx_peer_stats;
#endif

#if QCA_AIRTIME_FAIRNESS
        ic->ic_cfg80211_radio_handler.set_atf_sched_dur = ol_ath_ucfg_set_atf_sched_dur ;
#endif

        ic->ic_cfg80211_radio_handler.get_aggr_burst = ol_ath_iw_get_aggr_burst;
        ic->ic_cfg80211_radio_handler.set_aggr_burst = ol_ath_ucfg_set_aggr_burst;
        ic->ic_cfg80211_radio_handler.ucfg_phyerr = ol_ath_ucfg_phyerr;
        ic->ic_cfg80211_radio_handler.get_ath_stats = ol_ath_ucfg_get_ath_stats;
        ic->ic_cfg80211_radio_handler.get_dp_fw_peer_stats = ol_ath_get_dp_fw_peer_stats;
        ic->ic_cfg80211_radio_handler.get_dp_htt_stats = ol_ath_get_dp_htt_stats;
        ic->ic_cfg80211_radio_handler.get_cp_wmi_stats = ol_ath_get_cp_wmi_stats;
        ic->ic_cfg80211_radio_handler.get_target_pdev_id = ol_ath_get_target_pdev_id;
        ic->ic_cfg80211_radio_handler.get_cp_halphy_stats = ol_ath_get_cp_halphy_stats;
        ic->ic_cfg80211_radio_handler.set_ba_timeout = ol_ath_set_ba_timeout;
        ic->ic_cfg80211_radio_handler.get_ba_timeout = ol_ath_get_ba_timeout;
        ic->ic_cfg80211_radio_handler.set_pcp_tid_map = ol_ath_ucfg_set_pcp_tid_map;
        ic->ic_cfg80211_radio_handler.get_pcp_tid_map = ol_ath_ucfg_get_pcp_tid_map;
        ic->ic_cfg80211_radio_handler.set_tidmap_prty = ol_ath_ucfg_set_tidmap_prty;
        ic->ic_cfg80211_radio_handler.get_tidmap_prty = ol_ath_ucfg_get_tidmap_prty;
        ic->ic_cfg80211_radio_handler.set_btcoex_duty_cycle = ol_ath_ucfg_btcoex_duty_cycle;
        ic->ic_cfg80211_radio_handler.set_muedca_mode = ol_ath_ucfg_set_muedca_mode;
        ic->ic_cfg80211_radio_handler.get_muedca_mode = ol_ath_ucfg_get_muedca_mode;
        ic->ic_cfg80211_radio_handler.set_non_ht_dup = ol_ath_ucfg_set_non_ht_dup;
        ic->ic_cfg80211_radio_handler.get_non_ht_dup = ol_ath_ucfg_get_non_ht_dup;
        ic->ic_cfg80211_radio_handler.set_col_6ghz_rnr = ol_ath_ucfg_set_col_6ghz_rnr;
        ic->ic_cfg80211_radio_handler.get_col_6ghz_rnr = ol_ath_ucfg_get_col_6ghz_rnr;
        ic->ic_cfg80211_radio_handler.get_peer_extd_rate_link_stats =
                                               ol_ath_ucfg_get_peer_extd_rate_link_stats;
        ic->ic_cfg80211_radio_handler.send_egid_info = ol_ath_ucfg_send_egid_info;
#if DBG_LVL_MAC_FILTERING
        ic->ic_cfg80211_radio_handler.set_dbglvlmac = ol_ath_ucfg_set_dbglvlmac;
        ic->ic_cfg80211_radio_handler.get_dbglvlmac = ol_ath_ucfg_get_dbglvlmac;
#endif /* DBG_LVL_MAC_FILTERING */
#endif
        ic->ic_num_mcast_tbl_elements = ol_ath_num_mcast_tbl_elements;
        ic->ic_num_mcast_grps = ol_ath_num_mcast_grps;
        ic->ic_is_target_ar900b = ol_ath_is_target_ar900b;
        ic->ic_get_tgt_type = ol_ath_get_tgt_type;
#ifdef WLAN_FEATURE_11BE_MLO
        ic->ic_get_eml_phy_cap = ol_ath_get_eml_phy_cap;
        ic->ic_get_mld_phy_cap = ol_ath_get_mld_phy_cap;
        ic->ic_get_msd_phy_cap = ol_ath_get_msd_phy_cap;
        ic->ic_get_msd_info_override_enable = ol_ath_get_msd_info_override_enable;
        ic->ic_get_msd_info_present_override = ol_ath_get_msd_info_present_override;
        ic->ic_get_msd_info_value_override = ol_ath_get_msd_info_value_override;
        ic->ic_send_mlo_link_remove_cmd = target_if_mlo_send_link_removal_cmd;
#endif
#ifdef WLAN_FEATURE_11BE
        ic->ic_get_t2lm_enabled = ol_ath_get_t2lm_enabled;
        ic->ic_get_default_t2lm_response = ol_ath_get_default_t2lm_response;
        ic->ic_get_enable_eml_cap = ol_ath_get_enable_eml_cap;
#endif /* WLAN_FEATURE_11BE */
	ic->ic_get_ul_mumimo_support = ol_ath_get_ulmumimo_support;
#if UMAC_SUPPORT_CFG80211
        ic->ic_cfg80211_radio_handler.set_he_mesh_config = ol_ath_ucfg_set_he_mesh_config;
        ic->ic_cfg80211_radio_handler.get_he_mesh_config = ol_ath_ucfg_get_he_mesh_config;
        ic->ic_cfg80211_radio_handler.set_he_bss_color = ol_ath_ucfg_set_he_bss_color;
        ic->ic_cfg80211_radio_handler.get_he_bss_color = ol_ath_ucfg_get_he_bss_color;
        ic->ic_cfg80211_radio_handler.set_halphy_cal = ol_ath_ucfg_set_halphy_cal;
#ifdef WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG
        ic->ic_cfg80211_radio_handler.ic_set_rx_pkt_protocol_tagging = ol_ath_ucfg_set_rx_pkt_protocol_tagging;
#ifdef WLAN_SUPPORT_RX_TAG_STATISTICS
        ic->ic_cfg80211_radio_handler.ic_dump_rx_pkt_protocol_tag_stats = ol_ath_ucfg_dump_rx_pkt_protocol_tag_stats;
#endif /* WLAN_SUPPORT_RX_TAG_STATISTICS */
#endif /* WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG */
#ifdef WLAN_SUPPORT_RX_FLOW_TAG
        ic->ic_cfg80211_radio_handler.ic_rx_flow_tag_op = ol_ath_ucfg_rx_flow_tag_op;
#endif /* WLAN_SUPPORT_RX_FLOW_TAG */
        ic->ic_cfg80211_radio_handler.set_nav_override_config = ol_ath_ucfg_set_nav_override_config;
        ic->ic_cfg80211_radio_handler.get_nav_override_config = ol_ath_ucfg_get_nav_override_config;
#endif /* UMAC_SUPPORT_CFG80211 */
#if OBSS_PD
        ic->ic_cfg80211_radio_handler.set_he_sr_config = ol_ath_ucfg_set_he_sr_config;
        ic->ic_cfg80211_radio_handler.get_he_sr_config = ol_ath_ucfg_get_he_sr_config;
        ic->ic_cfg80211_radio_handler.set_he_srg_bitmap = ol_ath_ucfg_set_he_srg_bitmap;
        ic->ic_cfg80211_radio_handler.get_he_srg_bitmap = ol_ath_ucfg_get_he_srg_bitmap;
        ic->ic_cfg80211_radio_handler.set_sr_self_config = ol_ath_ucfg_set_sr_self_config;
        ic->ic_cfg80211_radio_handler.get_sr_self_config = ol_ath_ucfg_get_sr_self_config;
#endif /* OBSS PD */
#if CONFIG_SAWF_DEF_QUEUES
        ic->ic_cfg80211_radio_handler.dp_sawf_peer_svcid_map = ol_ath_dp_sawf_peer_svcid_map;
        ic->ic_cfg80211_radio_handler.dp_sawf_peer_svcid_unmap = ol_ath_dp_sawf_peer_unmap;
        ic->ic_cfg80211_radio_handler.dp_sawf_peer_map_report = ol_ath_dp_sawf_peer_map_report;
        ic->ic_cfg80211_radio_handler.set_rate_upper_cap = ol_ath_ucfg_set_rate_upper_cap;
        ic->ic_cfg80211_radio_handler.set_rate_retry_mcs_drop = ol_ath_ucfg_set_rate_retry_mcs_drop;
        ic->ic_cfg80211_radio_handler.set_mcs_probe_intvl = ol_ath_ucfg_set_mcs_probe_intvl;
        ic->ic_cfg80211_radio_handler.set_nss_probe_intvl = ol_ath_ucfg_set_nss_probe_intvl;
        ic->ic_cfg80211_radio_handler.set_sawf_params = ol_ath_ucfg_set_sawf_params;
        ic->ic_cfg80211_radio_handler.disable_sawf_svc = ol_ath_ucfg_disable_sawf;
#endif
	ic->ic_cfg80211_radio_handler.set_disable_mode =
					ol_ath_ucfg_set_disable_mode;
#ifdef CONFIG_SAWF
        ic->ic_cfg80211_radio_handler.dp_sawf_peer_msduq_report = ol_ath_dp_sawf_peer_msduq_report;
#endif
	ic->ic_cfg80211_radio_handler.set_peer_disable_mode =
					ol_ath_ucfg_set_peer_disable_mode;
#ifdef WLAN_MLO_GLOBAL_SHMEM_SUPPORT
        ic->ic_cfg80211_radio_handler.dump_mlo_glb_h_shmem_arena = ol_mlo_glb_h_shmem_arena_dump;
#endif /* WLAN_MLO_GLOBAL_SHMEM_SUPPORT */

#ifdef DP_UMAC_HW_RESET_SUPPORT
        ic->ic_cfg80211_radio_handler.set_umac_reset_debug =
            ol_ath_ucfg_set_umac_reset_debug;
        ic->ic_cfg80211_radio_handler.get_umac_reset_debug =
            ol_ath_ucfg_get_umac_reset_debug;
#endif
        ic->ic_cfg80211_radio_handler.set_op_support_rates =
            ol_ath_ucfg_set_op_support_rates;

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
        ic->ic_primary_chanlist = NULL;
        ic->ic_primary_allowed_enable = false;
#endif

        ol_ath_vap_attach(ic);

        if ((status = ol_ath_node_attach(scn, ic))) {
            qdf_err("ol_ath_node_attach failed");
            return status;
        };

        ol_ath_beacon_attach(ic);
        ol_ath_mgmt_attach(ic);

        ol_ath_chan_info_attach(ic);
        /* attach the dcs functionality */
        ol_ath_dcs_attach(ic);
        /* As of now setting ic with all ciphers assuming
         * hardware will support, eventually to query
         * the hardware to figure out h/w crypto support.
         */
        ieee80211com_set_cap(ic, IEEE80211_C_WEP);
        ieee80211com_set_cap(ic, IEEE80211_C_AES);
        ieee80211com_set_cap(ic, IEEE80211_C_AES_CCM);
        ieee80211com_set_cap(ic, IEEE80211_C_CKIP);
        ieee80211com_set_cap(ic, IEEE80211_C_TKIP);
        ieee80211com_set_cap(ic, IEEE80211_C_TKIPMIC);
        if (wmi_service_enabled(wmi_handle, wmi_service_11ac)) {
            ieee80211com_set_cap_ext(ic, IEEE80211_CEXT_11AC);
        }

        if (ieee80211_conf_parm->wmeEnabled) {
            ieee80211com_set_cap(ic, IEEE80211_C_WME);
        }

#if ATH_SUPPORT_WAPI
        ieee80211com_set_cap(ic, IEEE80211_C_WAPI);
#endif
        wlan_scan_update_channel_list(ic);

#if OL_ATH_SUPPORT_LED
        if (target_version == AR9888_REV2_VERSION || target_version == AR9887_REV1_VERSION) {
            scn->scn_led_gpio = PEREGRINE_LED_GPIO ;
        } else if (target_version ==  AR900B_DEV_VERSION) {
            scn->scn_led_gpio = BEELINER_LED_GPIO ;
        } else if (target_version ==  QCA9984_DEV_VERSION) {
            scn->scn_led_gpio = CASCADE_LED_GPIO ;
        } else if (target_version ==  QCA9888_DEV_VERSION) {
            scn->scn_led_gpio = BESRA_LED_GPIO ;
        } else if (led_gpio_enable_8074 && target_type == TARGET_TYPE_QCA8074V2 && reg_cap) {
            if (reg_cap[pdev_idx].wireless_modes & WIRELESS_MODES_2G) {
                scn->scn_led_gpio = IPQ8074_2G_LED_GPIO ;
            } else {
                scn->scn_led_gpio = IPQ8074_5G_LED_GPIO ;
            }
        } else if (target_type == TARGET_TYPE_QCA9574 ||
                   target_type == TARGET_TYPE_QCA5018 ||
                   target_type == TARGET_TYPE_QCA5332) {
              /* Fetch the GPIO number and active_low flag from DTS during init */
              plat_dev = (struct platform_device *)(scn->sc_osdev->bdev);
              dev_node = plat_dev->dev.of_node;
              led_gpio = qal_vbus_of_get_named_gpio_flags((struct qdf_device_node *)dev_node,
							  "led-gpio", 0,
                                                          &active_low);
              if (led_gpio < 0) {
                  qdf_err("Error: LED GPIO is not configured!!");
              } else {
                  scn->scn_led_gpio = led_gpio;
                  scn->scn_led_gpio_flags = active_low;
              }
        }

#if QCA_LTEU_SUPPORT
        if (wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_LTEU_SUPPORT)) {
            tgt_gpio_config(psoc, scn->scn_led_gpio, 1, WMI_HOST_GPIO_PULL_DOWN, WMI_HOST_GPIO_INTMODE_RISING_EDGE, 0, 0, 0);
        } else {
#endif
            ol_ath_led_init(soc, scn, target_type);
#if QCA_LTEU_SUPPORT
        }
#endif
#endif /* OL_ATH_SUPPORT_LED */
    }

    /*
     * Single vdev restart feature is used for run time configuration change
     * of some of the vdev parameters like hide_ssid etc..
     * Else, full vdev stop - start sequence is used for such configuration
     * changes.
     */
    ic->ic_is_vdev_restart_sup = true;

    is_prealloc_disabled = qdf_prealloc_disabled_config_get();
    if(is_prealloc_disabled) {
        ic->ic_scan_entry_max_count = ATH_SCANENTRY_MAX;
    } else {
        ic->ic_scan_entry_max_count = ATH_MAX_SCAN_ENTRIES;
    }
    ic->ic_scan_entry_timeout = ATH_SCANENTRY_TIMEOUT;
    ic->ic_rx_amsdu = 0xff;
    /* unless user prefers not to allow the frames between different vaps, let frames route through*/
    scn->scn_block_interbss = 0;

    /* Mask Value to set fixed mac addr for Backhaul STA. Default 255 */
    scn->sc_bsta_fixed_idmask = 0xff;

#if ATH_DATA_TX_INFO_EN
    /*Alloc buffer for data TX info*/
    scn->tx_status_buf = qdf_mem_malloc(sizeof(struct ieee80211_tx_status));
#endif

    /* Add ic to global list */
    GLOBAL_IC_LOCK_BH(&ic_list);
    ic_list.global_ic[ic_list.num_global_ic++] = ic;
    qdf_debug("Set global_ic[%d], ptr:%pK", ic_list.num_global_ic,&ic_list);
    ic->ic_global_list = &ic_list;
#if DBDC_REPEATER_SUPPORT
    ic_list.dbdc_process_enable = 1;
    ic_list.force_client_mcast_traffic = 0;

    qca_multi_link_set_dbdc_enable(true);
    qca_multi_link_set_force_client_mcast(false);

    if (ol_target_lithium(psoc)) {
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (ic->nss_radio_ops) {
            ic->nss_radio_ops->ic_nss_ol_enable_dbdc_process(ic, 1);
        }
#endif
    }
    ic_list.delay_stavap_connection = cfg_get(psoc, CFG_TGT_DBDC_TIME_DELAY);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (ic->nss_radio_ops)
        ic->nss_radio_ops->ic_nss_ol_set_force_client_mcast_traffic(ic, ic_list.force_client_mcast_traffic);
#endif
    ic_list.num_stavaps_up = 0;
    qca_multi_link_set_always_primary(false);
    ic_list.num_fast_lane_ic = 0;
    ic_list.max_priority_stavap_up = NULL;
    qca_multi_link_set_drop_sec_mcast(false);
    ic_list.num_l2uf_retries = 0;
    wlan_rptr_reset_flags(ic->ic_pdev_obj);
#endif
    GLOBAL_IC_UNLOCK_BH(&ic_list);

    if (soc->hw_mode_ctx.dynamic_hw_mode != WMI_HOST_DYNAMIC_HW_MODE_FAST)
        ic->ic_master_radio = ol_ath_is_master_radio(scn);

#if DBDC_REPEATER_SUPPORT
    spin_lock(&ic->ic_lock);
    if (ic_list.num_global_ic) {
        /* In case of DBDC repeater configuration, pass Multicast/Broadcast and
         ethernet client traffic through this radio */
        ic->ic_radio_priority = ic_list.num_global_ic;
        ic->ic_primary_radio = 0;
    }
    ic->fast_lane = 0;
    ic->fast_lane_ic = NULL;
    spin_unlock(&ic->ic_lock);

    k = 0;
    /*update other_ic list on each radio*/
    for (i=0; i < MAX_RADIO_CNT; i++) {
    GLOBAL_IC_LOCK_BH(&ic_list);
    tmp_ic = ic_list.global_ic[i];
    GLOBAL_IC_UNLOCK_BH(&ic_list);
    if (tmp_ic && (tmp_ic != ic) && (k < MAX_RADIO_CNT-1)) {
        spin_lock(&ic->ic_lock);
        ic->other_ic[k++] = tmp_ic;
        spin_unlock(&ic->ic_lock);
        for (j=0; j < MAX_RADIO_CNT-1 ; j++) {
        if (tmp_ic->other_ic[j] == NULL) {
            spin_lock(&tmp_ic->ic_lock);
            tmp_ic->other_ic[j] = ic;
            spin_unlock(&tmp_ic->ic_lock);
            break;
        }
        }
    }
    }

#endif
#if QCN_IE
    ic->ic_rept_clients = 0;
#endif
    /*Set default monitor VAP filter to enable all input*/
    ic->mon_filter_osif_mac = 0;
    ic->ic_non_doth_sta_cnt = 0;

    ic->ic_stop = ol_ath_target_stop;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    ic->ic_check_mlo_recovery = ol_ath_target_check_mlo_recovery;
    ic->ic_check_mlo_assoc_on_recovery = ol_ath_check_mlo_assoc_on_recovery;
#endif

    if (wmi_service_enabled(wmi_handle,
                wmi_service_multiple_vdev_restart)) {
        wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
                                   WLAN_PDEV_F_MULTIVDEV_RESTART);
    } else {
        wlan_pdev_nif_feat_cap_clear(ic->ic_pdev_obj,
                                     WLAN_PDEV_F_MULTIVDEV_RESTART);
    }

    if (wmi_service_enabled(wmi_handle,
                wmi_service_multiple_vdev_restart_bmap)) {
        wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
                                   WLAN_PDEV_F_MULTIVDEV_RESTART_BMAP);
    } else {
        wlan_pdev_nif_feat_cap_clear(ic->ic_pdev_obj,
                                     WLAN_PDEV_F_MULTIVDEV_RESTART_BMAP);
    }

    if (wmi_service_enabled(wmi_handle,
                            wmi_service_vdev_delete_all_peer)) {
        wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
                                   WLAN_PDEV_F_DELETE_ALL_PEER);
    } else {
        wlan_pdev_nif_feat_cap_clear(ic->ic_pdev_obj,
                                     WLAN_PDEV_F_DELETE_ALL_PEER);
    }

    if (wmi_service_enabled(wmi_handle,
                            wmi_beacon_protection_support)) {
        wlan_pdev_nif_feat_cap_set(ic->ic_pdev_obj,
                                   WLAN_PDEV_F_BEACON_PROTECTION);
    } else {
        wlan_pdev_nif_feat_cap_clear(ic->ic_pdev_obj,
                                     WLAN_PDEV_F_BEACON_PROTECTION);
    }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (scn->nss_radio.nss_rctx)
        ic->nss_vops->ic_osif_nss_wifi_monitor_set_filter(ic, MON_FILTER_ALL_DISABLE);
#endif

    ol_ath_twt_enable_command(scn);
#ifdef QVIT
    ol_ath_pdev_qvit_attach(scn);
#endif


    if (dispatcher_pdev_open(scn->sc_pdev)) {
        qdf_err("Dispatcher pdev open failed");
        return -EINVAL;
    }

#if QCA_STANDALONE_SOUNDING_TRIGGER
    ol_ath_pdev_host_trig_txbf_sounding_init(scn->sc_pdev);
#endif

#if QCA_MANUAL_TRIGGERED_ULOFDMA
    ol_ath_pdev_man_trig_ulofdma_init(scn->sc_pdev);
#endif
    ol_ath_pdev_dfs_phyerr_offload_en(scn->sc_pdev);
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    ic->ic_rebuilt_chanlist = 0;
    dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
    if (dfs_rx_ops) {
        if(dfs_rx_ops->dfs_reset_spoof_test)
            dfs_rx_ops->dfs_reset_spoof_test(scn->sc_pdev);
    }
#endif /* HOST_DFS_SPOOF_TEST */
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(WLAN_DFS_SYNTHETIC_RADAR)
    dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
    if (dfs_rx_ops && dfs_rx_ops->dfs_allow_hw_pulses)
        dfs_rx_ops->dfs_allow_hw_pulses(scn->sc_pdev, true);
#endif /* WLAN_DFS_PARTIAL_OFFLOAD && WLAN_DFS_SYNTHETIC_RADAR*/
#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
    /* Resetting the scan violation status on wifi. */
    ic->ic_is_dfs_scan_violation = false;
#endif
    /* Reset/Init the AP power type of the wifi */
    ucfg_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &ic->ic_ucfg_6g_ap_pwr_type);

    if (!is_emulation(psoc) && ol_target_lithium(psoc)) {
        scn->is_scn_stats_timer_init = 1;
	scn->pdev_stats_timer = PDEV_STATS_DEFAULT_TIMER;
        qdf_timer_init(soc->qdf_dev, &(scn->scn_stats_timer), ol_ath_get_pdev_stats, (void *)scn, QDF_TIMER_TYPE_SW);
    }

    /* register for peer state events */
    if (ol_target_be(psoc)) {
        if (ol_ath_subscribe_peer_wdi_event(scn) != QDF_STATUS_SUCCESS) {
            qdf_err("failure in registering the peer wdi events");
            qdf_assert_always(0);
        }
        qdf_info("Registered peer wdi events");
#ifdef CONFIG_SAWF
        if (cfg_get(psoc, CFG_OL_SAWF_UL_FSE_ENABLE)) {
            if (QDF_IS_STATUS_ERROR(ol_ath_subscribe_sawf_fse_wdi_event(scn)))
                qdf_err("unable to subscribe FSE wdi event");
        }
#ifdef SAWF_ADMISSION_CONTROL
        if (cfg_get(psoc, CFG_OL_SAWF_ADM_CTRL_ENABLE)) {
            if (QDF_IS_STATUS_ERROR(ol_ath_subscribe_sawf_peer_msduq_event(scn)))
                qdf_err("unable to subscribe Peer MSDUQ wdievent");
        }
#endif
#endif
    }

    if (ol_target_be(psoc) && (ol_ath_get_conn_mode() == QDF_GLOBAL_MISSION_MODE))
        ol_ath_enable_telemetry_stats(scn->sc_pdev);
#ifdef WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG
    ic->rx_pkt_protocol_tag_mask = 0;
    ol_ath_set_protocol_tagging(ic->ic_pdev_obj, RX_PKT_TAG_OPCODE_ADD,
		                RECV_PKT_TYPE_EAP, 3232);

#ifdef WLAN_FEATURE_11BE_MLO
    if (MLD_AP_HYBRID_NONBOND_MODE ==
        cfg_get(soc->psoc_obj, CFG_OL_MLDEV_AP_MODE)) {
        if (target_type == TARGET_TYPE_QCN9224 ||
            target_type == TARGET_TYPE_QCA5332) {
            ol_ath_set_protocol_tagging(ic->ic_pdev_obj,
                                        RX_PKT_TAG_OPCODE_ADD,
                                        RECV_PKT_TYPE_IGMP_V4,
                                        1234);
            ol_ath_set_protocol_tagging(ic->ic_pdev_obj,
                                        RX_PKT_TAG_OPCODE_ADD,
                                        RECV_PKT_TYPE_MLD_V6,
                                        4321);
        }
    }
#endif

#ifdef IPA_WDI3_VLAN_SUPPORT
    if (ucfg_ipa_is_vlan_enabled()) {
        ol_ath_set_protocol_tagging(ic->ic_pdev_obj,
                                    RX_PKT_TAG_OPCODE_ADD,
                                    RECV_PKT_TYPE_VLAN, 1122);
    }
#endif
#endif //WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG

#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
    if (scn->tx_pkt_cap_proto)
        ol_ath_set_tx_pkt_cap_custom_classify(ic->ic_pdev_obj,
                                              scn->tx_pkt_cap_proto);
#endif /* WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY */

#if OBSS_PD
    ol_ath_pdev_sr_init(scn);
#endif

    /* FW dynamic selection of Muedca param by default */
    ic->ic_muedca_mode_state = HEMUEDCA_FW_DYNAMIC_MODE;
    qdf_atomic_init(&(scn->auth_cnt));
    qdf_timer_init(soc->qdf_dev, &(scn->auth_timer), ol_ath_clear_auth_cnt, (void *)scn, QDF_TIMER_TYPE_SW);


    scn->max_auth = MAX_AUTH_REQUEST;
    scn->tx_ack_timeout = DEFAULT_TX_ACK_TIMEOUT;
    scn->sc_presp_retry_limit = 0;
    scn->sc_cts_timeout = 0;
    scn->sc_slot_time = 0;
    scn->is_ani_enable = DEFAULT_ANI_ENABLE_STATUS;
    scn->soft_chain = 0;

    /* Check for channels to be blocked for automatic channel selection (ACS, ICM, ...)*/
    ol_ath_pri20_cfg_blockchanlist_parse(ic, cfg_get(soc->psoc_obj, CFG_OL_PRI20_CFG_BLOCKCHANLIST));

    /* Enable support for wideband (5GHz-7GHz) support and CSA-specific flags */
    ol_ath_parse_wideband_support(ic, cfg_get(soc->psoc_obj, CFG_OL_WIDEBAND_CSA));

    ic->ic_offchan_dwell_time = cfg_get(soc->psoc_obj, CFG_OL_OFFCHAN_SCAN_DWELL_TIME);

#ifdef WLAN_FEATURE_11BE_MLO
    /* Bound ML probe response timer by offchan dwell time:
     * ML Probe response update time =
     *     min(ic_offchan_dwell_time, ic_ml_prbresp_update_time) +
     *     IEEE80211_ML_PRBRESP_UPDATE_DELTA
     */
    ic->ic_ml_prbresp_update_time = QDF_MIN(cfg_get(soc->psoc_obj, CFG_OL_ML_PRBRESP_UPDATE_TIME),
                                            ic->ic_offchan_dwell_time) +
                                    IEEE80211_ML_PRBRESP_UPDATE_DELTA;
#endif

    ic->ic_omn_cxt.omn_timeout = OMN_DEFAULT_UPDATE_TIME;
    qdf_timer_init(NULL, &ic->ic_omn_cxt.notify_timer, wlan_omn_timer_callback, (void *)ic, QDF_TIMER_TYPE_WAKE_APPS);

    if (ol_target_lithium(psoc)) {
        /* By default,Set driver mode for RNR advertisement */
        WLAN_6GHZ_RNR_DRIVER_MODE_SET(ic->ic_6ghz_rnr_enable);
    }
    if (wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_6G)) {
        /* As per 11be draft4.0,  RNR signaling will not use 320MHz Operating
         * class, as legacy STAs will not support decoding 320MHz Opclass.
         * This flag is enabled to avoid signaling to use of 320MHz opclass
         * in RNR and 6G discovery frame.
         */
        ic->ic_downgrade_320mhz_opclass = true;
    }

    if (!soc->tbtt_offset_sync_timer_init) {
        qdf_timer_init(soc->qdf_dev, &(soc->tbtt_offset_sync_timer),
                       ol_tbtt_sync_timer_cb, (void *)scn,
                       QDF_TIMER_TYPE_WAKE_APPS);
	soc->tbtt_offset_sync_timer_init = 1;
    }

    if (cfg_get(psoc, CFG_OL_EXTERNALACS_ENABLE))
        ieee80211_ic_externalacs_enable_set(ic);
    else
        ieee80211_ic_externalacs_enable_clear(ic);

    /* Initialize a QDF wait event for VDEV bringdown. */
    qdf_mem_zero(&ic->wait_event_for_down, sizeof(ic->wait_event_for_down));
    qdf_event_create(&ic->wait_event_for_down);

    return EOK;
}

void ol_ath_target_deinit(ol_ath_soc_softc_t *soc, int force)
{
    ol_txrx_soc_handle soc_txrx_handle;
    struct target_psoc_info *tgt_psoc_info;
    uint32_t recovery_type = 0;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    recovery_type = soc->recovery_type;
#endif

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return;
    }

    soc_txrx_handle = wlan_psoc_get_dp_handle(soc->psoc_obj);

    if (!bypasswmi) {
        if (soc_txrx_handle) {
            cdp_txrx_umac_reset_deinit(soc_txrx_handle, recovery_type);
            cdp_txrx_intr_detach(soc_txrx_handle);

            ol_ath_soc_rate_stats_detach(soc);

            cdp_soc_deinit(soc_txrx_handle);
#ifdef WLAN_FEATURE_FASTPATH
            soc->htt_handle = NULL;
#endif
        }

        if (soc->dbg_log_init) {
            ol_ath_dbglog_detach(soc);
            soc->dbg_log_init = 0;
        }
    }
}

void ol_ath_target_deinit_complete(ol_ath_soc_softc_t *soc)
{
   HTC_HANDLE htc_handle;
   void *hif_hdl;
   struct target_psoc_info *tgt_hdl;

   tgt_hdl = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
   htc_handle = lmac_get_htc_hdl(soc->psoc_obj);
   hif_hdl = lmac_get_ol_hif_hdl(soc->psoc_obj);

   if (htc_handle) {
        htc_destroy(htc_handle);
        lmac_set_htc_hdl(soc->psoc_obj, NULL);
    }
    /* de-initialize rf_path_ctx */
    ol_ath_init_deinit_rf_path_ctx(soc, false);

    /* Cleanup BMI */
#ifdef WLAN_FEATURE_BMI
    if (soc->bmi_handle != NULL) {
        if (soc->bmi_handle->bmi_cleanup)
            soc->bmi_handle->bmi_cleanup(soc->bmi_handle,soc->sc_osdev);
        if (soc->bmi_handle->bmi_free)
            soc->bmi_handle->bmi_free(soc->bmi_handle);
    }
#endif
    if (hif_hdl != NULL) {
        ol_hif_close(hif_hdl);
	target_psoc_set_hif_hdl(tgt_hdl, NULL);
	hif_hdl = NULL;
    }
}

#ifdef QCA_RSSI_DB2DBM
/**
 * ol_ath_rssi_conv_evt_detach() - Deregister rssi_conv event
 * @soc: soc handle
 *
 * Deregister wmi_pdev_rssi_dbm_conversion_params_info_eventid.
 *
 * Return: None
 */
static inline void
ol_ath_rssi_conv_evt_detach(ol_ath_soc_softc_t *soc)
{
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    wmi_unified_unregister_event_handler((void *)wmi_handle,
                                         wmi_pdev_rssi_dbm_conversion_params_info_eventid);
}
#else
static inline void ol_ath_rssi_conv_evt_detach(ol_ath_soc_softc_t *soc)
{}
#endif

int
ol_ath_soc_detach(ol_ath_soc_softc_t *soc, int force)
{
    int status = 0;
    struct mgmt_txrx_mgmt_frame_cb_info rx_cb_info;
    struct target_psoc_info *tgt_psoc_info;
    struct wmi_unified *wmi_handle;
    struct hif_opaque_softc *hif_hdl;
    struct pdev_op_args oper;
    ol_txrx_soc_handle soc_txrx_handle = NULL;
#ifdef CONFIG_SAWF
    cdp_config_param_type val = {0};
#endif
    uint8_t tgt_type = 0;

    if (ol_target_lithium(soc->psoc_obj)) {
        qca_unregister_netdevice_notifier(&qca_device_notifier);
    }

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return -EINVAL;
    }

    wmi_handle = target_psoc_get_wmi_hdl(tgt_psoc_info);
    if (!wmi_handle)
        return -EINVAL;

#if OL_ATH_SUPPORT_LED
    if (
#if QCA_LTEU_SUPPORT
        !(wlan_psoc_nif_feat_cap_get(soc->psoc_obj, WLAN_SOC_F_LTEU_SUPPORT)) &&
#endif
        !bypasswmi) {
            soc->led_blink_rate_table = NULL;
    }
#endif /* OL_ATH_SUPPORT_LED */

    if(soc->ol_if_ops->wds_addr_detach)
    	soc->ol_if_ops->wds_addr_detach((wmi_unified_t)wmi_handle);

    ol_mempools_detach(soc);

    ol_ath_phyerr_detach(soc);
    ol_ath_soc_chan_info_detach(soc);
#ifdef WLAN_FEATURE_11BE
    ol_ath_sched_probe_resp_detach(soc);
#endif
#if  ATH_DATA_TX_INFO_EN
    ol_ath_stats_soc_detach(soc);
#endif
    ol_ath_wow_soc_detach(soc);
    ol_ath_rssi_conv_evt_detach(soc);

   tgt_type = lmac_get_tgt_type(soc->psoc_obj);

#if WLAN_SUPPORT_MSCS && QCA_NSS_PLATFORM
  /*
   * De-Init MSCS module
   */
    if (tgt_type == TARGET_TYPE_QCA8074 ||
            tgt_type == TARGET_TYPE_QCA8074V2 ||
            tgt_type == TARGET_TYPE_QCA9574 ||
            tgt_type == TARGET_TYPE_QCA5018 ||
	    tgt_type == TARGET_TYPE_QCA5332 ||
	    tgt_type == TARGET_TYPE_QCN6432 ||
            tgt_type == TARGET_TYPE_QCN9000 ||
            tgt_type == TARGET_TYPE_QCN9224 ||
            tgt_type == TARGET_TYPE_QCA6018) {
        qca_mscs_module_deinit(soc);
    }
#endif

    /* UMAC Disatcher disable/stop all components under PSOC */
    dispatcher_psoc_disable(soc->psoc_obj);
    ol_mlo_glb_h_shmem_arena_ctx_deinit(soc);

    soc_txrx_handle = wlan_psoc_get_dp_handle(soc->psoc_obj);

#ifdef CONFIG_SAWF
    if (ol_ath_is_sawf_msduq_reclaim_enabled(soc, wmi_handle)) {
        /*
         * cdp_sawf_msduq_reclaim_enabled = False :
         * Deinit the SAWF MSDUQ Reclaim Timer
         */
        val.cdp_sawf_msduq_reclaim_enabled = false;
        cdp_txrx_set_psoc_param(soc_txrx_handle, CDP_SAWF_MSDUQ_RECLAIM_SUPPORT,
                                val);
    }
#endif

    if (!(soc->down_complete)) {
        QDF_STATUS ret;

        ret = ol_ath_unregister_custom_callbacks(soc->psoc_obj);
        qdf_assert_always(QDF_IS_STATUS_SUCCESS(ret));

        ol_ath_disconnect_htc(soc);
        ol_ath_target_deinit(soc, force);
    } else if (ol_target_lithium(soc->psoc_obj)) {
        oper.type = PDEV_ITER_PDEV_DETACH_OP;
        wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
    }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
        soc->nss_soc.ops->nss_soc_desc_pool_free(soc);
    }
#endif

    wlan_psoc_set_dp_handle(soc->psoc_obj, NULL);
    cdp_soc_detach(soc_txrx_handle);

#ifdef QVIT
    ol_ath_qvit_detach(soc);
#endif

    if (!bypasswmi) {
        if (soc->dbg_log_init) {
            ol_ath_dbglog_detach(soc);
            soc->dbg_log_init = 0;
        }
    }

    if (!(soc->down_complete)) {
    	hif_hdl = target_psoc_get_hif_hdl(tgt_psoc_info);
        hif_disable_isr((struct hif_opaque_softc *)hif_hdl);
        ol_ath_target_deinit_complete(soc);
    }
    dispatcher_psoc_close(soc->psoc_obj);

    if (!bypasswmi) {
        if (wmi_handle) {
            wmi_unified_detach((wmi_unified_t)wmi_handle);
#if WMI_RECORDING
            wmi_proc_remove((wmi_unified_t)wmi_handle, soc->wmi_proc_entry, soc->soc_idx);
#endif
            wmi_handle =NULL;
        }
    }

    ol_codeswap_detach(soc);

#ifdef AH_CAL_IN_FLASH_PCI
#ifndef ATH_CAL_NAND_FLASH
    if (soc->cal_mem) {
        A_IOUNMAP(soc->cal_mem);
        soc->cal_mem = 0;
    }
#endif
    soc->cal_in_flash = 0;
#endif
#if QLD
    qdf_event_destroy(&soc->qld_wait);
#endif
    soc->cal_in_file = 0;

    /* Deregister MgMt TxRx handler */
    rx_cb_info.frm_type = MGMT_FRAME_TYPE_ALL;
    rx_cb_info.mgmt_rx_cb = ieee80211_mgmt_input;
    wlan_mgmt_txrx_deregister_rx_cb(soc->psoc_obj, WLAN_UMAC_COMP_MLME,
                                       &rx_cb_info, 1);

    if (!(soc->down_complete)) {
         init_deinit_free_num_units(soc->psoc_obj, tgt_psoc_info);
    }

#ifdef WLAN_SUPPORT_RF_CHARACTERIZATION
    if (soc->rf_characterization_entries) {
        qdf_mem_free(soc->rf_characterization_entries);
        soc->num_rf_characterization_entries = 0;
        soc->rf_characterization_entries = NULL;
    }
#endif

    /* No Target accesses of any kind after this point */
    return status;
}

int ol_ath_pdev_detach(struct ol_ath_softc_net80211 *scn, int force)
{
    struct ieee80211com     *ic = &scn->sc_ic;
    ol_ath_soc_softc_t *soc = scn->soc;
    int status = 0;
    uint32_t target_type;
    struct ieee80211com     *tmp_ic;
#if DBDC_REPEATER_SUPPORT
    int j;
#endif
    int i = 0;
#ifdef WLAN_MLO_MULTI_CHIP
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
    struct mlo_setup_info *setup_mlo;
    bool is_mlo_capable;
    uint8_t grp_id;
#endif

#if ATH_DATA_TX_INFO_EN
    /*Free buffer for data TX info*/
    qdf_mem_free(scn->tx_status_buf);
    scn->tx_status_buf = NULL;
#endif

    if (!is_emulation(scn->soc->psoc_obj) &&
        ol_target_lithium(scn->soc->psoc_obj)) {
            if (scn->is_scn_stats_timer_init)
                qdf_timer_sync_cancel(&scn->scn_stats_timer);
    }

#if  ATH_DATA_TX_INFO_EN
    if (!(scn->soc->down_complete)) {
        ol_ath_stats_detach(ic);
    }
#endif
    if (!is_emulation(scn->soc->psoc_obj) &&
        ol_target_lithium(soc->psoc_obj))
            qdf_timer_sync_cancel(&scn->scn_stats_timer);

    qdf_timer_sync_cancel(&(scn->auth_timer));

    target_type = lmac_get_tgt_type(soc->psoc_obj);
    ol_ath_wow_detach(ic);

    /* deregister IEEE80211_DPRINTF_IC control object */
    ieee80211_dprintf_ic_deregister(ic);


    if (!bypasswmi) {
        ieee80211_stop_running(ic);

        /*
         * NB: the order of these is important:
         * o call the 802.11 layer before detaching the hal to
         *   insure callbacks into the driver to delete global
         *   key cache entries can be handled
         * o reclaim the tx queue data structures after calling
         *   the 802.11 layer as we'll get called back to reclaim
         *   node state and potentially want to use them
         * o to cleanup the tx queues the hal is called, so detach
         *   it last
         * Other than that, it's straightforward...
         */
        ieee80211_ifdetach(ic);
    }

    ol_regdmn_detach(scn->ol_regdmn_handle);
    ol_ath_mgmt_detach(ic);

#if WLAN_SPECTRAL_ENABLE
    /* Unregister spectral event handlers with DDMA */
    spectral_unregister_dbr(ic->ic_pdev_obj);
#endif /* WLAN_SPECTRAL_ENABLE */
#if QCA_MANUAL_TRIGGERED_ULOFDMA
    ol_ath_pdev_man_trig_ulofdma_deinit(scn->sc_pdev);
#endif
#if WLAN_CFR_ENABLE
    /* pdev deinit to be done on Target Stop */
    cfr_deinitialize_pdev(ic->ic_pdev_obj);
#endif
#if WLAN_WIFI_RADAR_ENABLE
    wifi_radar_deinitialize_pdev(ic->ic_pdev_obj);
#endif
#if QCA_STANDALONE_SOUNDING_TRIGGER
    ol_ath_pdev_host_trig_txbf_sounding_deinit(scn->sc_pdev);
#endif /* QCA_STANDALONE_SOUNDING_TRIGGER */

    qdf_event_destroy(&ic->ic_wait_for_init_cc_response);
    qdf_event_destroy(&ic->ic_wait_for_r2p_table_set_response);
#ifdef QCA_R2P_UPDATE_ENABLED
    qdf_event_destroy(&ic->ic_wait_for_off_chan_entry);
#endif

#if QCA_LTEU_SUPPORT
    ol_ath_nl_detach(ic);
#endif

#ifndef REMOVE_PKT_LOG
    if (scn->pl_dev){
        ol_pktlog_detach(scn);
    ol_pl_freehandle(scn->pl_dev);
        scn->pl_dev = NULL;
    }
#endif

#ifdef IPA_OFFLOAD
    ucfg_ipa_uc_ol_deinit(ic->ic_pdev_obj);
#endif
    /* Remove ic from global list */
    for (i=0; i < MAX_RADIO_CNT; i++) {
        tmp_ic = ic_list.global_ic[i];
        if (tmp_ic && (ic == tmp_ic)) {
            GLOBAL_IC_LOCK_BH(&ic_list);
            ic_list.global_ic[i] = NULL;
            ic_list.num_global_ic--;
            qdf_info("remove global_ic[%d]..gloabl_ic ptr:%pK",
                     ic_list.num_global_ic, &ic_list);
            GLOBAL_IC_UNLOCK_BH(&ic_list);
        }
    }
#if DBDC_REPEATER_SUPPORT
    for (i=0; i < MAX_RADIO_CNT; i++) {
        GLOBAL_IC_LOCK_BH(&ic_list);
        tmp_ic = ic_list.global_ic[i];
        GLOBAL_IC_UNLOCK_BH(&ic_list);
        if (tmp_ic && (tmp_ic != ic)) {
            for (j=0; j < MAX_RADIO_CNT-1 ; j++) {
                if (tmp_ic->other_ic[j] == ic) {
                    spin_lock(&tmp_ic->ic_lock);
                    tmp_ic->other_ic[j] = NULL;
                    spin_unlock(&tmp_ic->ic_lock);
                    break;
                }
            }
        }
    }

    /*
     * The removal of radio is done here as wiphy structure
     * is freed inside ic_cfg80211_radio_detach function.
     */
    if (ic->ic_wiphy) {
        qca_multi_link_remove_radio(ic->ic_wiphy);
    }

#endif

#if UMAC_SUPPORT_CFG80211
#ifdef WLAN_MLO_MULTI_CHIP
    grp_id = wlan_mlo_get_psoc_group_id(scn->soc->psoc_obj);

    if (ic->ic_cfg80211_config && ic->ic_cfg80211_radio_detach) {
        is_mlo_capable = ic->ic_mlo_capable(scn->soc->psoc_obj);

        if (is_mlo_capable) {
            qdf_debug("The psoc is mlo capable and group_id is:%d", grp_id);
            setup_mlo = &mlo_ctx->setup_info[grp_id];
        } else {
            setup_mlo = NULL;
        }
        ic->ic_cfg80211_radio_detach(ic, setup_mlo);
    }
#else
    if (ic->ic_cfg80211_config) {
        if(ic->ic_cfg80211_radio_detach) {
            ic->ic_cfg80211_radio_detach(ic);
      }
   }
#endif
#endif

#ifdef QVIT
    ol_ath_pdev_qvit_detach(scn);
#endif

#if OL_ATH_SUPPORT_LED
    if (
#if QCA_LTEU_SUPPORT
        !(wlan_psoc_nif_feat_cap_get(scn->soc->psoc_obj, WLAN_SOC_F_LTEU_SUPPORT)) &&
#endif
        !bypasswmi) {
            qdf_timer_sync_cancel(&scn->scn_led_blink_timer);
            qdf_timer_sync_cancel(&scn->scn_led_poll_timer);
    }
#endif /* OL_ATH_SUPPORT_LED */

#if OS_SUPPORT_ASYNC_Q
    OS_MESGQ_DRAIN(&scn->sc_osdev->async_q,NULL);
    OS_MESGQ_DESTROY(&scn->sc_osdev->async_q);
#endif
#if !defined(BUILD_X86) && !defined(CONFIG_X86)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 24)
    qcom_pcie_deregister_event(&soc->pcie_event);
#endif
#endif

    qdf_timer_free(&ic->ic_omn_cxt.notify_timer);
    if (soc->tbtt_offset_sync_timer_init) {
        qdf_timer_free(&(soc->tbtt_offset_sync_timer));
        soc->tbtt_offset_sync_timer_init = 0;
    }

    /* Destroy the wait event when pdev detaches */
    qdf_event_destroy(&ic->wait_event_for_down);

    return status;
}

void ol_reset_params(struct ol_ath_softc_net80211 *scn)
{
    struct ieee80211com *ic = &scn->sc_ic;

    /* should always be equal to define DEFAULT_LOWEST_RATE_IN_5GHZ 0x03  6 Mbps  in firmware */
    scn->ol_rts_cts_rate                    = 0x03;
#if WDI_EVENT_ENABLE
    scn->scn_user_peer_invalid_cnt          = 0;/* By default we will send one deauth in 10 msec in response to rx_peer_invalid */
#endif
    scn->dyngroup                           = 0;
    scn->dpdenable                          = CLI_DPD_NA_STATE;
    scn->scn_amsdu_mask                     = 0xffff;
    scn->scn_amsdu_mask                     = 0xffff;
    scn->txpower_scale                      = 0;
    scn->powerscale                         = 0;

    ol_ath_reset_dcs_params(scn);

    scn->burst_dur                          = 0;
    scn->burst_enable                       = 0;
    scn->is_ani_enable                      = DEFAULT_ANI_ENABLE_STATUS;
    scn->dtcs                               = 0; /* Dynamic Tx Chainmask Selection enabled/disabled */
    scn->vow_extstats                       = 0;
    scn->sc_arp_dbg_conf                    = 0;
#ifdef QCA_SUPPORT_CP_STATS
    pdev_cp_stats_ap_stats_tx_cal_enable_update(ic->ic_pdev_obj, 0);
#endif
    scn->macreq_enabled                     = 0; /* user mac request feature enable/disable */
    scn->sc_bsta_fixed_idmask               = 0xff; /* Mask Value to set fixed mac addr for Backhaul STA. Default 255 */
    scn->arp_override                       = WME_AC_VO;
    scn->igmpmld_override                   = 0;
    scn->igmpmld_tid                        = 0;
#if ATH_RX_LOOPLIMIT_TIMER
    scn->rx_looplimit_timeout               = 1;           /* 1 ms by default */
    scn->rx_looplimit_valid                 = true;          /* set to valid after initilization */
    scn->rx_looplimit                       = false;
#endif
    scn->sc_prealloc_idmask                 = 0;
    scn->sc_is_blockdfs_set                 = false;
    scn->scn_sifs_frmtype                   = 0;
    scn->scn_sifs_uapsd                     = 0;
    scn->scn_block_interbss                 = 0;
    scn->txbf_sound_period                  = 100;
    scn->scn_promisc                        = 0;
    scn->enable_statsv2                     = 0;
#if ATH_SUPPORT_WRAP
    scn->mcast_bcast_echo                   = 0;
#endif
#if ATH_DATA_TX_INFO_EN
    scn->enable_perpkt_txstats              = 0;
#endif
    scn->tx_ack_timeout                     = DEFAULT_TX_ACK_TIMEOUT;
    scn->sc_presp_retry_limit               = 0;
    scn->sc_cts_timeout                     = 0;
    scn->sc_slot_time                       = 0;
#ifdef QCA_SUPPORT_CP_STATS
    pdev_cp_stats_wmi_tx_mgmt_update(ic->ic_pdev_obj, 0);
    pdev_cp_stats_wmi_tx_mgmt_completions_update(ic->ic_pdev_obj, 0);
    pdev_cp_stats_wmi_tx_mgmt_completion_err_update(ic->ic_pdev_obj, 0);
#endif

    scn->soft_chain                         = 0;
    /*
     * CCK is enabled by default in the firmware for all the chipsets.
     */
    ic->cck_tx_enable = true;

    return;
}

int ol_ath_soc_stop(struct ol_ath_soc_softc *soc, bool flush_wq)
{
    struct net_device *dev;
    struct pci_dev *pcidev;
    int target_paused = TRUE;
    uint32_t target_type;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct target_psoc_info *tgt_hdl;
    struct hif_opaque_softc *hif_hdl;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wmi_unified *wmi_handle;
    struct pdev_op_args oper;
    QDF_STATUS status;

    psoc = soc->psoc_obj;
    target_type = lmac_get_tgt_type(psoc);
    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target handle is null");
        return -EINVAL;
    }

    hif_hdl = target_psoc_get_hif_hdl(tgt_hdl);
    wmi_handle = lmac_get_wmi_hdl(psoc);

    dev = soc->sc_osdev->netdev;
    pcidev = (struct pci_dev *)soc->sc_osdev->bdev;

    qdf_spin_lock_bh(&soc->soc_tgt_lock);
    if (qdf_atomic_test_and_set_bit(SOC_RESET_IN_PROGRESS_BIT,
                                    &soc->reset_in_progress)) {
        qdf_info("Soc %d Reset in progress, return", wlan_psoc_get_id(soc->psoc_obj));
        qdf_spin_unlock_bh(&soc->soc_tgt_lock);
        return -EBUSY;
    }

    if (soc->down_complete) {
        /*
         * Target has already been stopped, return success.
         */
        qdf_atomic_clear_bit(SOC_RESET_IN_PROGRESS_BIT, &soc->reset_in_progress);
        qdf_spin_unlock_bh(&soc->soc_tgt_lock);
        return 0;
    }
    qdf_spin_unlock_bh(&soc->soc_tgt_lock);

     /* To avoid PCI register access after pci unmap,
      * flush any pending pci_reconnect_work */
    if (flush_wq) {
        qdf_flush_work(&soc->pci_reconnect_work);
    }

    soc->soc_attached = 0;
#if OL_ATH_SUPPORT_LED
    soc->led_blink_rate_table = NULL;
#endif

    if (ol_ath_is_dynamic_hw_mode_enabled(soc) &&
        (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) &&
        (soc->hw_mode_ctx.current_mode == WMI_HOST_HW_MODE_DBS)) {
        struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
        int i;

        /* Save current_mode for SSR recovery and switch to DBS_SBS mode
         * for clean up
         */
        soc->hw_mode_ctx.recover_mode = soc->hw_mode_ctx.current_mode;
        soc->hw_mode_ctx.target_mode = WMI_HOST_HW_MODE_DBS_SBS;

        mac_phy_cap = target_psoc_get_mac_phy_cap_for_mode(
                          tgt_hdl, WMI_HOST_HW_MODE_DBS_SBS);
        if (mac_phy_cap) {
            for (i = 0; i < WMI_HOST_MAX_PDEV; i++) {
                soc->hw_mode_ctx.next_pdev_map[i] = mac_phy_cap[i].tgt_pdev_id;
            }

            ol_ath_hw_mode_config_primary_pdev(soc, false);
            ol_ath_hw_mode_apply_pdev_map(soc);
            ol_ath_hw_mode_config_primary_pdev(soc, true);
            ol_ath_hw_mode_config_secondary_pdev(soc, true);
        }
    }

    oper.type = PDEV_ITER_PDEV_DEINIT_BEFORE_SUSPEND;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
              wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    /* Suspend Target if not recovering */
    if (soc->recovery_in_progress == 0) {
        qdf_info("Suspending Target  soc=%pK", soc);
        if (!ol_ath_suspend_target(soc, 1)) {
            u_int32_t  timeleft;
            qdf_info("waiting for target paused event from target");
            /* wait for the event from Target*/
            timeleft = wait_event_interruptible_timeout(soc->sc_osdev->event_queue,
                (soc->is_target_paused == TRUE), PDEV_SUSPEND_TIMEOUT);
            if (!timeleft || signal_pending(current)) {
            qdf_info("ERROR: Failed to receive target paused event soc=%pK ", soc);
            target_paused = FALSE;
            }
            /*
             * reset is_target_paused and host can check that in next time,
             * or it will always be TRUE and host just skip the waiting
             * condition, it causes target assert due to host already suspend
             */
            soc->is_target_paused = FALSE;
        }
    }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
            qdf_info("nss-wifi: detach soc->soc_idx: %d - 100 delay ", soc->soc_idx);
            soc->nss_soc.ops->nss_soc_wifi_detach(soc, 100);
    }
#endif

    ol_ath_allow_wmi_cmds(wmi_handle, false);
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    qdf_info("disable dp interrupts:: soc: %pK", soc_txrx_handle);
    hif_disable_isr((struct hif_opaque_softc *)hif_hdl);
    cdp_txrx_intr_detach(soc_txrx_handle);
    cdp_txrx_ppeds_stop(soc_txrx_handle);
    status = ol_ath_unregister_custom_callbacks(psoc);
    qdf_assert_always(QDF_IS_STATUS_SUCCESS(status));

    ol_ath_disconnect_htc(soc);
    oper.type = PDEV_ITER_PDEV_DEINIT_OP;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
            wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (ol_ath_mlo_capable(psoc)) {
        uint8_t grp_id = ol_ath_mlo_get_group_id(psoc);
        mlo_setup_update_soc_down(psoc, grp_id);
    }
#endif
    ol_mlo_glb_h_shmem_arena_ctx_deinit(soc);
    ol_ath_target_deinit(soc, 1);

    ol_ath_target_deinit_complete(soc);

    target_if_psoc_tgt_info_mem_free(tgt_hdl);

    if((target_type !=  TARGET_TYPE_QCN6122) &&
       (target_type !=  TARGET_TYPE_QCN9160) &&
       (target_type !=  TARGET_TYPE_QCN6432) &&
       (target_type !=  TARGET_TYPE_QCN9000) &&
       (target_type !=  TARGET_TYPE_QCN9224) &&
       (target_type !=  TARGET_TYPE_QCA8074) &&
       (target_type != TARGET_TYPE_QCA8074V2) &&
       (target_type != TARGET_TYPE_QCA9574) &&
       (target_type !=  TARGET_TYPE_QCA5018) &&
       (target_type !=  TARGET_TYPE_QCA5332) &&
       (target_type !=  TARGET_TYPE_QCN6432) &&
       (target_type != TARGET_TYPE_QCA6018)) {
        pci_set_drvdata(pcidev, dev);
    }

    init_deinit_free_num_units(soc->psoc_obj, tgt_hdl);
    target_psoc_set_wmi_ready(tgt_hdl, FALSE);
    target_psoc_set_wmi_service_ready(tgt_hdl, FALSE);
    wlan_psoc_nif_op_flag_clear(psoc, WLAN_SOC_OP_VHT_INVALID_CAP);
    soc->dbg.print_rate_limit = DEFAULT_PRINT_RATE_LIMIT_VALUE;

    oper.type = PDEV_ITER_PDEV_RESET_PARAMS;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
                 wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    ol_ath_diag_user_agent_fini(soc);

    /* re-init hw_mode_ctx->current_mode */
    soc->hw_mode_ctx.current_mode = WMI_HOST_HW_MODE_MAX;

    soc->down_complete = true;

    qdf_atomic_clear_bit(SOC_RESET_IN_PROGRESS_BIT, &soc->reset_in_progress);
    return 0;
}

/* Suspend and unload the firmware during the last vap removal */
int ol_ath_target_stop(struct wlan_objmgr_psoc *psoc, bool flush_wq)
{
    struct ol_ath_soc_softc *soc;
    struct platform_device *pdev;
#ifdef CONFIG_CNSS2_SUPPORT
    struct pci_dev *pcidev = NULL;
#endif
    int tgt_type = 0;
    struct target_psoc_info *tgt_psoc_info;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    uint8_t grp_id;
    enum wmi_mlo_teardown_reason teardown_reason;
    bool wsi_remap_in_progress = false;
    int ret = 0;
    osdev_t osdev = NULL;
    struct mlo_mgr_context *mlo_ctx = wlan_objmgr_get_mlo_ctx();
#endif

    if (!psoc)
        return -EINVAL;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(psoc);
    if(!tgt_psoc_info) {
        qdf_info("target_psoc_info is null");
        return -EINVAL;
    }
    soc = (struct ol_ath_soc_softc *) target_psoc_get_feature_ptr(tgt_psoc_info);
    pdev = (struct platform_device *) soc->sc_osdev->bdev;
    tgt_type = lmac_get_tgt_type(soc->psoc_obj);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    /* find the grp id of the soc*/
    if (ol_ath_mlo_capable(soc->psoc_obj)) {
        grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);
        wsi_remap_in_progress = wlan_mlo_is_wsi_remap_in_progress(grp_id);
    }
    if (ol_ath_mlo_capable(soc->psoc_obj) && (!wsi_remap_in_progress)) {
        if (soc->psoc_obj->wsi_remap_remove) {
            /* reset the flag */
            soc->psoc_obj->wsi_remap_remove = false;
            soc->psoc_obj->wsi_remap_add = false;
            /* reset the PCIE power to max */
            osdev = get_osdev_from_psoc(soc->psoc_obj);
            if (osdev)
                pld_set_pci_link_speed_width(osdev->device, 3, 2);
            pld_set_wsi_remap_state(soc->qdf_dev->dev, false);
            ol_ath_parse_mlo_params_from_ini(false);

            /* clear the MLO flag and populate the data to platform data */
            ret = pld_reset_mlo_group_config(mlo_param.mlo_max_groups);
            if (ret) {
                qdf_err("%s: MLO reset failed %d", __func__, ret);
                return QDF_STATUS_E_INVAL;
            }
            ret = pld_set_mlo_config(mlo_arg, &mlo_param);
            if (ret) {
                qdf_err("%s: MLO set config failed %d", __func__, ret);
                return QDF_STATUS_E_INVAL;
            }
            /* update the MLO capabilities */
            __qca_ol_update_mlo_capability(true, soc->psoc_obj->wsi_remap_recovery_in_progress);
        }
        if (soc->psoc_obj->wsi_remap_add) {
            /* reset the flag */
            soc->psoc_obj->wsi_remap_remove = false;
            soc->psoc_obj->wsi_remap_add = false;
        }
    }
    /* If the system has been through wsi remap align the SOC during wifi down */
    if (mlo_ctx->dynamic_wsi_bypassed)
        mlo_set_soc_list(grp_id, psoc);

    if (ol_ath_mlo_capable(soc->psoc_obj) && !(soc->psoc_obj->wsi_remap_remove)) {

        if (soc->standby_mode == WLAN_STANDBY_MODE_ACTIVE_ENTRY)
            teardown_reason = WMI_HOST_MLO_TEARDOWN_REASON_STANDBY;
        else
            teardown_reason = WMI_HOST_MLO_TEARDOWN_REASON_DOWN;

        if (mlo_link_teardown_link(soc->psoc_obj, grp_id,
            teardown_reason) != QDF_STATUS_SUCCESS) {
            /* Teardown failed. Proceed to bringdown forcefully */
            qdf_debug("Failed to complete teardown");
        }
    }
#endif
    ol_ath_soc_stop(soc, flush_wq);

#ifdef CONFIG_CNSS2_SUPPORT
    /* pld_wlan_disable is called inside hif_wlan_disable */
    if ((tgt_type == TARGET_TYPE_QCA8074) ||
        (tgt_type == TARGET_TYPE_QCA8074V2) ||
        (tgt_type == TARGET_TYPE_QCA9574) ||
        (tgt_type == TARGET_TYPE_QCA5018) ||
        (tgt_type == TARGET_TYPE_QCA5332) ||
        (tgt_type == TARGET_TYPE_QCN6122) ||
        (tgt_type == TARGET_TYPE_QCN9160) ||
        (tgt_type == TARGET_TYPE_QCA6018) ||
        (tgt_type == TARGET_TYPE_QCN6432)) {
        if (!soc->psoc_obj->wsi_remap_remove)
            pld_subsystem_put(&pdev->dev);
    }
    if ((tgt_type == TARGET_TYPE_QCN9000) ||
        (tgt_type == TARGET_TYPE_QCN9224)) {
        pcidev = (struct pci_dev *)pdev;
        if (!soc->psoc_obj->wsi_remap_remove)
            pld_subsystem_put(&pcidev->dev);
    }
#endif
    return 0;
}
qdf_export_symbol(ol_ath_target_stop);

uint16_t ol_ath_get_num_clients(struct wlan_objmgr_pdev *pdev)
{
    uint16_t num_peers = wlan_pdev_get_max_peer_count(pdev);
    uint8_t num_vdevs = wlan_pdev_get_max_vdev_count(pdev);

    /* If num_peers is less than num_vdevs then use num_peers as it is without
     * removing bss peers.
     */
    if (num_vdevs > num_peers)
        return num_peers;
    else
        return (num_peers - num_vdevs);
}

#ifdef WLAN_FEATURE_11BE_MLO
static void
ol_ath_process_peer_primary_umac_update_event(void *pdev_hdl,
                                              enum WDI_EVENT event,
                                              void *data,
                                              u_int16_t peer_id,
                                              uint32_t status)
{
    uint8_t vdev_id;
    osif_dev  *osifp = NULL;
    struct cdp_txrx_peer_params_update *param = NULL;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    int  mode = 0;

    param = (struct cdp_txrx_peer_params_update *)data;
    if (!pdev || !param)
        return;

    vdev_id = param->vdev_id;
    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, vdev_id, WLAN_OSIF_ID);
    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap)
        goto release_ref;

    osifp = (osif_dev *)vap->iv_ifp;;
    if (!osifp || !osifp->mldev)
        goto release_ref;

    if (osifp_peer_primary_umac_update(osifp->mldev,
                                       param,
                                       osifp->is_bridge_link) !=
                                            QDF_STATUS_SUCCESS) {
        qdf_err("failure in updating primary umac\n");
        goto release_ref;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc)
        goto release_ref;

    /* In non bonding mode only mldX interface will be exposed to accelerators,
     * during the PTQM migration there won't be any changes required for
     * accelerators learning.
     * Where as in bonding mode link dev are exposed, accelerators need to
     * relearn the new link dev. This required the flush.
     */
    mode = ol_cfg_get_mld_netdev_mode_ap((ol_soc_handle)psoc);
    if (mode != MLD_AP_UNIFIED_BOND_MODE)
        goto release_ref;

    /* send flush event to ecm */
    ol_ath_send_ecm_event((struct cdp_ctrl_objmgr_psoc *)psoc,
                          vdev_id, param->peer_mac);
release_ref:
    if (vdev)
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
}

void
ol_ath_process_peer_flush_event(void *pdev_hdl, enum WDI_EVENT event,
                                void *data, u_int16_t peer_id,
                                uint32_t status)
{
    struct cdp_txrx_peer_params_update *param = NULL;
    struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)pdev_hdl;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    uint8_t vdev_id;

    param = (struct cdp_txrx_peer_params_update *)data;
    if (!pdev || !param)
        return;

    vdev_id = param->vdev_id;
    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, vdev_id, WLAN_OSIF_ID);
    if (!vdev)
        goto release_ref;

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc)
        goto release_ref;

    /* send flush event to ecm */
    ol_ath_send_ecm_event((struct cdp_ctrl_objmgr_psoc *)psoc,
                          vdev_id, param->peer_mac);
release_ref:
    if (vdev)
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OSIF_ID);
}

QDF_STATUS ol_ath_subscribe_peer_wdi_event(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    scn->txrx_peer_create_subscriber.callback = osif_process_txrx_peer_create_event;
    scn->txrx_peer_create_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->txrx_peer_create_subscriber,
                          WDI_EVENT_TXRX_PEER_CREATE))
        return QDF_STATUS_E_FAILURE;

    scn->peer_unmap_subscriber.callback = osif_process_peer_unmap_event;
    scn->peer_unmap_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->peer_unmap_subscriber,
                          WDI_EVENT_PEER_UNMAP))
        return QDF_STATUS_E_FAILURE;

    scn->peer_flush_subscriber.callback = ol_ath_process_peer_flush_event;
    scn->peer_flush_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->peer_flush_subscriber,
                          WDI_EVENT_PEER_UNMAP))
        return QDF_STATUS_E_FAILURE;

    scn->peer_delete_subscriber.callback = osif_process_peer_delete_event;
    scn->peer_delete_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->peer_delete_subscriber,
                          WDI_EVENT_PEER_DELETE))
        return QDF_STATUS_E_FAILURE;

    scn->peer_primary_umac_update_subscriber.callback =
    ol_ath_process_peer_primary_umac_update_event;
    scn->peer_primary_umac_update_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->peer_primary_umac_update_subscriber,
                          WDI_EVENT_PEER_PRIMARY_UMAC_UPDATE))
        return QDF_STATUS_E_FAILURE;

    scn->primary_mcast_update_subscriber.callback =
                                    osif_process_primary_umac_update_event;
    scn->primary_mcast_update_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->primary_mcast_update_subscriber,
                          WDI_EVENT_MCAST_PRIMARY_UPDATE))
        return QDF_STATUS_E_FAILURE;

    scn->primary_sta_umac_update_subscriber.callback =
                                    osif_process_primary_umac_update_event;
    scn->primary_sta_umac_update_subscriber.context = scn->sc_pdev;
    if (cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                          &scn->primary_sta_umac_update_subscriber,
                          WDI_EVENT_STA_PRIMARY_UMAC_UPDATE))
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_unsubscribe_peer_wdi_event(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->txrx_peer_create_subscriber,
                            WDI_EVENT_TXRX_PEER_CREATE))
        return QDF_STATUS_E_FAILURE;

    qdf_info("soc_txrx_handle %pK dp_pdev id %d", soc_txrx_handle, pdev_id);

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->peer_unmap_subscriber,
                            WDI_EVENT_PEER_UNMAP))
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->peer_flush_subscriber,
                            WDI_EVENT_PEER_UNMAP))
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->peer_delete_subscriber,
                            WDI_EVENT_PEER_DELETE))
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->peer_primary_umac_update_subscriber,
                            WDI_EVENT_PEER_PRIMARY_UMAC_UPDATE))
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->primary_mcast_update_subscriber,
                            WDI_EVENT_MCAST_PRIMARY_UPDATE))
        return QDF_STATUS_E_FAILURE;

    if (cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                            &scn->primary_sta_umac_update_subscriber,
                            WDI_EVENT_STA_PRIMARY_UMAC_UPDATE))
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}
#else
QDF_STATUS ol_ath_subscribe_peer_wdi_event(struct ol_ath_softc_net80211 *scn)
{
    return QDF_STATUS_SUCCESS;
}
QDF_STATUS ol_ath_unsubscribe_peer_wdi_event(struct ol_ath_softc_net80211 *scn)
{
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef WLAN_REG_PARTIAL_OFFLOAD
static void ol_ath_set_regdmn(struct wlan_objmgr_psoc *psoc,
                              struct wlan_objmgr_pdev *pdev_obj,
                              struct ol_ath_softc_net80211 *scn,
                              struct target_psoc_info *tgt_hdl)
{
    struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;
    struct wmi_unified *wmi_handle;

    wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
    if (!wmi_handle) {
        qdf_err("%s : wmi_handle is NULL", __func__);
        return;
    }

    if (!(wmi_service_enabled(wmi_handle, wmi_service_regulatory_db))) {
        struct cur_regdmn_info cur_regdmn = {};

        reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
        if (!(reg_rx_ops && reg_rx_ops->reg_get_current_regdomain)) {
            qdf_err("%s : reg_rx_ops is NULL", __func__);
            return;
        }

        if (wlan_objmgr_pdev_try_get_ref(pdev_obj, WLAN_REGULATORY_SB_ID) !=
                                         QDF_STATUS_SUCCESS) {
            return;
        }

        if (reg_rx_ops->reg_get_current_regdomain(pdev_obj, &cur_regdmn))
            return;

        ol_ath_pdev_set_regdomain(scn, &cur_regdmn);
        wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_REGULATORY_SB_ID);

    }
}
#endif

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
static int recovery_wsi_remap(ol_ath_soc_softc_t *soc)
{
    int ret;

    ol_ath_parse_mlo_params_from_ini(true);

    ret = pld_reset_mlo_group_config(mlo_param.mlo_max_groups);
    if (ret) {
        qdf_err("%s: MLO reset failed %d", __func__, ret);
        return QDF_STATUS_E_INVAL;
    }
    ret = pld_set_mlo_config(mlo_arg, &mlo_param);
    if (ret) {
        qdf_err("%s: MLO set config failed %d", __func__, ret);
        return QDF_STATUS_E_INVAL;
    }

    /* Send config QMI message to FW */
    ret = pld_set_wsi_remap(soc->qdf_dev->dev);
    if (ret) {
        qdf_err("%s: MLO config QMI message is failed %d", __func__, ret);
        return ret;
    }

    /* Perform the MLO setup of the current active Chips in a group */
    ret = ol_ath_dynamic_wsi_remap_mlo_setup(soc);
    if (ret) {
        qdf_err("%s: Recovery Setup MLO config failed %d", __func__, ret);
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

static int recovery_wsi_static_init(void)
{
    int ret = 0;

    ret = ol_ath_parse_mlo_params_from_ini(false);
    if (!ret) {
        if (mlo_param.mlo_default_cfg)
            qdf_info("Set default MLO config");
        else
            qdf_info("Set MLO configuration from INI file");

        ret = pld_reset_mlo_config(mlo_param.mlo_max_groups);
        if (ret) {
            qdf_err("%s: MLO reset failed %d", __func__, ret);
            qdf_info("Try default MLO config");
            mlo_param.mlo_default_cfg = true;
        }

        ret = pld_set_mlo_config(mlo_arg, &mlo_param);
        if (ret) {
            qdf_err("%s: MLO set failed %d", __func__, ret);
        }
    }

    return ret;
}
#endif
#endif

int ol_ath_soc_start(struct ol_ath_soc_softc *soc)
{
    struct net_device *netdev;
    struct platform_device *pdev;
    uint32_t pdev_idx;
    struct pci_dev *pcidev;
    struct _NIC_DEV *drv;
    int i, retval = 0, sniffer_mode;
#ifdef CONFIG_CNSS2_SUPPORT
    struct device *dev = NULL;
#endif
    struct target_psoc_info *tgt_hdl;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    struct wlan_psoc_host_mac_phy_caps *mac_phy_cap;
    uint32_t target_type = 0;
    uint32_t target_version;
    struct wlan_psoc_target_capability_info *target_cap;
    ol_txrx_soc_handle soc_txrx_handle;
    void *hif_hdl = NULL;
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
    struct wlan_lmac_if_dfs_rx_ops *dfs_rx_ops;
#endif /* HOST_DFS_SPOOF_TEST */
    uint8_t num_radios;
    struct ol_ath_softc_net80211 *scn;
    QDF_STATUS ret;

    tgt_hdl = (struct target_psoc_info *)wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target handle is null");
        goto fail;
    }
    target_cap = target_psoc_get_target_caps(tgt_hdl);
    mac_phy_cap = target_psoc_get_mac_phy_cap(tgt_hdl);
    target_type = target_psoc_get_target_type(tgt_hdl);
    target_version = target_psoc_get_target_ver(tgt_hdl);

    pdev = (struct platform_device *)soc->sc_osdev->bdev;
    pcidev = (struct pci_dev *)soc->sc_osdev->bdev;
    netdev = soc->sc_osdev->netdev;
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    soc->glb_shmem_init_complete = false;

#ifdef CONFIG_CNSS2_SUPPORT
    if ((target_type == TARGET_TYPE_QCA8074) ||
        (target_type == TARGET_TYPE_QCA8074V2) ||
        (target_type == TARGET_TYPE_QCA9574) ||
        (target_type == TARGET_TYPE_QCA5018) ||
        (target_type == TARGET_TYPE_QCA5332) ||
        (target_type == TARGET_TYPE_QCN6122) ||
        (target_type == TARGET_TYPE_QCN9160) ||
        (target_type == TARGET_TYPE_QCA6018) ||
        (target_type == TARGET_TYPE_QCN6432)) {
        dev = &pdev->dev;
    } else {
        dev = &pcidev->dev;
    }

    pld_wait_for_fw_ready(dev);
#endif

    drv = soc->qdf_dev->drv;

    OS_MEMSET(soc->qdf_dev, 0, sizeof(*(soc->qdf_dev)));

    soc->qdf_dev->drv = drv;
    soc->qdf_dev->cnss_pdev = pld_get_plat_dev_by_bus_dev(&pcidev->dev);

    if((target_type == TARGET_TYPE_QCA8074) ||
       (target_type == TARGET_TYPE_QCA8074V2) ||
       (target_type == TARGET_TYPE_QCA9574) ||
       (target_type == TARGET_TYPE_QCN6122) ||
       (target_type == TARGET_TYPE_QCA5332) ||
       (target_type == TARGET_TYPE_QCN9160) ||
       (target_type == TARGET_TYPE_QCA5018) ||
       (target_type == TARGET_TYPE_QCA6018) ||
       (target_type == TARGET_TYPE_QCN6432)) {
        soc->qdf_dev->dev = &pdev->dev; /* device */
        soc->qdf_dev->drv_hdl = NULL; /* bus handle */
        hif_hdl = ol_hif_open(&pdev->dev, pdev, (void *)soc->platform_devid, QDF_BUS_TYPE_AHB, 0,soc->qdf_dev);
    } else {
        soc->qdf_dev->dev = &pcidev->dev; /* device */
        soc->qdf_dev->drv_hdl = pcidev; /* bus handle */
        if (target_type != TARGET_TYPE_QCN9000 && target_type != TARGET_TYPE_QCN9224) {
            pci_set_drvdata(pcidev, NULL);
        }
        hif_hdl = ol_hif_open(&pcidev->dev, pcidev, (void *)soc->platform_devid, QDF_BUS_TYPE_PCI, 0,soc->qdf_dev);
        if (target_type != TARGET_TYPE_QCN9000 && target_type != TARGET_TYPE_QCN9224) {
            pci_set_drvdata(pcidev, netdev);
        }
    }

#ifdef IPA_OFFLOAD
    ol_ath_smmu_mem_map_setup(soc->qdf_dev, ucfg_ipa_is_ready());
#endif

    wlan_psoc_set_qdf_dev(psoc, soc->qdf_dev);
    if (!hif_hdl) {
        goto fail;
    }

    target_psoc_set_hif_hdl(tgt_hdl, hif_hdl);
    soc->targetdef = hif_get_targetdef(hif_hdl);

    /* set up dynamic_hw_mode switch if supported */
    if (cfg_get(psoc, CFG_OL_DYNAMIC_HW_MODE)) {
        soc->hw_mode_ctx.target_mode =
                        target_psoc_get_preferred_hw_mode(tgt_hdl);
        target_psoc_set_preferred_hw_mode(tgt_hdl,
                                          WMI_HOST_HW_MODE_DETECT);
    }

    target_psoc_set_total_mac_phy_cnt(tgt_hdl, 0);
    target_psoc_set_num_radios(tgt_hdl, 0);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (soc->nss_soc.ops) {
            if (soc->nss_soc.ops->nss_soc_wifi_init(soc)) {
                    goto fail;
            }
    }
#endif

    if (soc->ol_if_ops->target_init) {
        if (soc->ol_if_ops->target_init(soc, true))
            goto fail;
    }

    if (ol_target_init_complete(soc))
        goto fail;

    ret = ol_ath_register_custom_callbacks(soc);
    if (QDF_IS_STATUS_ERROR(ret)) {
        qdf_err("Failed to register custom call backs");
        goto fail;
    }

    ret = ol_ath_read_shmem_addresses(soc);
    if (QDF_IS_STATUS_ERROR(ret))
        qdf_err("Failed to read the crash reason address");

    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    if (!soc_txrx_handle) {
        qdf_err("psoc dp handle is null");
        goto fail;
    }

#ifdef DIRECT_BUF_RX_ENABLE
    if (direct_buf_rx_target_attach(soc->psoc_obj,
                          hif_get_hal_handle(lmac_get_hif_hdl(soc->psoc_obj)),
                          soc->qdf_dev) != QDF_STATUS_SUCCESS) {
        qdf_err("Direct Buffer Rx Target Attach Failed");
    }
#endif

    num_radios = target_psoc_get_num_radios(tgt_hdl);

    for (i = 0; i < num_radios; i++) {
        struct net_device *pdev_netdev;
        struct ieee80211com *ic;
        struct wlan_objmgr_pdev *pdev_obj;
        uint8_t pdev_id;
        uint32_t scan_radio_tx_on_dfs_channels;

        pdev_obj = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_MLME_NB_ID);
        if (!pdev_obj) {
             qdf_err("pdev object (id: %d) is NULL", i);
             goto fail;
        }

        scn = lmac_get_pdev_feature_ptr(pdev_obj);
        ic = &scn->sc_ic;
        pdev_idx = lmac_get_pdev_idx(scn->sc_pdev);
        pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev_obj);

        pdev_netdev = scn->netdev;
        if (!bypasswmi) {
            if ((target_type != TARGET_TYPE_QCA8074) &&
                (target_type != TARGET_TYPE_QCA8074V2) &&
                (target_type != TARGET_TYPE_QCA9574) &&
                (target_type != TARGET_TYPE_QCA6018) &&
                (target_type != TARGET_TYPE_QCA5018) &&
                (target_type != TARGET_TYPE_QCA5332) &&
                (target_type != TARGET_TYPE_QCN6122) &&
                (target_type != TARGET_TYPE_QCN9160) &&
                (target_type != TARGET_TYPE_QCN9224) &&
                (target_type != TARGET_TYPE_QCN9000) &&
                (target_type != TARGET_TYPE_QCN6432)) {
                if (target_version != AR6004_VERSION_REV1_3) {
                    if (cdp_pdev_attach_target(soc_txrx_handle,
                                               pdev_id) != A_OK) {
                        qdf_err("txrx pdev attach target failed");
                        wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_NB_ID);
                        goto fail;
                    }
                }
            } else {
                qdf_info("Skip txrx_pdev_attach_target");
            }
        }

        if ((target_type != TARGET_TYPE_QCA8074) &&
            (target_type != TARGET_TYPE_QCA8074V2) &&
            (target_type != TARGET_TYPE_QCN9000) &&
            (target_type != TARGET_TYPE_QCN9224) &&
            (target_type != TARGET_TYPE_QCA5332) &&
            (target_type != TARGET_TYPE_QCN6122) &&
            (target_type != TARGET_TYPE_QCN9160) &&
            (target_type != TARGET_TYPE_QCA5018) &&
            (target_type != TARGET_TYPE_QCA6018) &&
            (target_type != TARGET_TYPE_QCN6432)) {
            cdp_config_param_type val = {0};

            cdp_txrx_set_pdev_param(soc_txrx_handle,
                                    pdev_id, CDP_FILTER_UCAST_DATA, val);
            cdp_txrx_set_pdev_param(soc_txrx_handle,
                                    pdev_id, CDP_FILTER_MCAST_DATA, val);
            cdp_txrx_set_pdev_param(soc_txrx_handle,
                                    pdev_id,CDP_FILTER_NO_DATA, val);
        }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if ((target_type != TARGET_TYPE_QCA8074) &&
            (target_type != TARGET_TYPE_QCA8074V2) &&
            (target_type != TARGET_TYPE_QCA9574) &&
            (target_type != TARGET_TYPE_QCA9574) &&
            (target_type != TARGET_TYPE_QCN9000) &&
            (target_type != TARGET_TYPE_QCN9224) &&
            (target_type != TARGET_TYPE_QCN6122) &&
            (target_type != TARGET_TYPE_QCA5332) &&
            (target_type != TARGET_TYPE_QCN9160) &&
            (target_type != TARGET_TYPE_QCA5018) &&
            (target_type != TARGET_TYPE_QCA6018) &&
            (target_type != TARGET_TYPE_QCN6432)) {
            qdf_info("nss-wifi: update radio nss context with soc context for legay radio");
            scn->nss_radio.nss_rctx = soc->nss_soc.nss_sctx;
            scn->nss_radio.nss_rifnum = soc->nss_soc.nss_sifnum;
            scn->nss_radio.nss_idx = soc->nss_soc.nss_sidx;
        }
#endif
#ifdef IPA_OFFLOAD
        if(soc_txrx_handle->ol_ops->get_con_mode &&
                        soc_txrx_handle->ol_ops->get_con_mode() ==
                        QDF_GLOBAL_FTM_MODE) {
                ipa_disable_register_cb();
        } else {
                ipa_register_is_ipa_ready(pdev_obj);
        }
        ucfg_ipa_reg_sap_xmit_cb(pdev_obj,
                        osif_ol_vap_hardstart_wifi3_ipa_offload);
        ucfg_ipa_reg_send_to_nw_cb(pdev_obj,
                        osif_deliver_data_ol_ipa_offload);
#endif

        /*
         * Enable Bursting by default for pre-ES2 release only. Tobe removed Later
         */
        {
            int ac, duration, retval;
            ac = 0, retval = 0;
            duration = AGGR_BURST_DURATION;
            qdf_debug("BURSTING enabled by default");
            for (ac = 0; ac <= 3; ac++) {
                retval = ol_ath_pdev_set_param(scn->sc_pdev,
                            wmi_pdev_param_aggr_burst,
                            (ac & AGGR_BURST_AC_MASK) << AGGR_BURST_AC_OFFSET |
                             (AGGR_BURST_DURATION_MASK & duration));
            }
            if (EOK == retval)
                 scn->aggr_burst_dur[0] = duration;

            ol_ath_pdev_set_param(scn->sc_pdev,
                                  wmi_pdev_param_set_ppdu_duration_cmdid,
                                  AGGR_PPDU_DURATION);
        }

        ol_ath_twt_enable_command(scn);
        /* Enable beacon bursting by default */
        retval = ol_ath_pdev_set_param(scn->sc_pdev,
                                       wmi_pdev_param_beacon_tx_mode,
                                       scn->bcn_mode);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
        SET_MODULE_OWNER(pdev_netdev);
#endif

        /* Don't leave arp type as ARPHRD_ETHER as this is no eth device */
        pdev_netdev->type = ARPHRD_IEEE80211;

#if PERF_FIND_WDS_NODE
        wds_table_init(&scn->scn_wds_table);
#endif

        ic->ic_num_clients = ol_ath_get_num_clients(ic->ic_pdev_obj);

#if WLAN_SPECTRAL_ENABLE
        spectral_register_dbr(ic->ic_pdev_obj);
#endif /* WLAN_SPECTRAL_ENABLE */

#if WLAN_CFR_ENABLE
    /* pdev init to be done on Target Start */
        cfr_initialize_pdev(ic->ic_pdev_obj);
#endif
#if WLAN_WIFI_RADAR_ENABLE
        wifi_radar_initialize_pdev(ic->ic_pdev_obj);
#endif
#if QCA_STANDALONE_SOUNDING_TRIGGER
        ol_ath_pdev_host_trig_txbf_sounding_init(scn->sc_pdev);
#endif
        ol_ath_mgmt_attach(ic);
#if  ATH_DATA_TX_INFO_EN
        ol_ath_stats_attach(ic);
#endif

#if OBSS_PD
        /* Init Spatial Reuse */
        ol_ath_pdev_sr_init(scn);
#endif

#ifdef QCA_SUPPORT_CP_STATS
        if (pdev_cp_stats_ap_stats_tx_cal_enable_get(ic->ic_pdev_obj)) {
            scn->sc_ic.ic_ath_enable_ap_stats(&scn->sc_ic, 1);
            pdev_cp_stats_ap_stats_tx_cal_enable_update(ic->ic_pdev_obj, 0);
        }
#endif
        /* register for peer state events */
        if (ol_target_be(psoc)) {
            if (ol_ath_subscribe_peer_wdi_event(scn) != QDF_STATUS_SUCCESS) {
                qdf_err("failure in registering the peer wdi events");
                qdf_assert_always(0);
            }
            qdf_info("Registered peer wdi events");
#ifdef CONFIG_SAWF
            if (cfg_get(psoc, CFG_OL_SAWF_UL_FSE_ENABLE)) {
                ret = ol_ath_subscribe_sawf_fse_wdi_event(scn);
                if (QDF_IS_STATUS_ERROR(ret))
                    qdf_err("unable to subscribe FSE wdi event");
            }
#ifdef SAWF_ADMISSION_CONTROL
            if (cfg_get(psoc, CFG_OL_SAWF_ADM_CTRL_ENABLE)) {
                ret = ol_ath_subscribe_sawf_peer_msduq_event(scn);
                if (QDF_IS_STATUS_ERROR(ret))
                    qdf_err("Unable to subscribe Peer MSDUQ wdi event");
            }
#endif
#endif
        }

	ol_ath_set_protocol_tagging(ic->ic_pdev_obj, RX_PKT_TAG_OPCODE_ADD,
			            RECV_PKT_TYPE_EAP, 3232);

#ifdef WLAN_FEATURE_11BE_MLO
        if (MLD_AP_HYBRID_NONBOND_MODE ==
            cfg_get(soc->psoc_obj, CFG_OL_MLDEV_AP_MODE)) {
            if (target_type == TARGET_TYPE_QCN9224 ||
                target_type == TARGET_TYPE_QCA5332) {
                    ol_ath_set_protocol_tagging(ic->ic_pdev_obj,
                                                RX_PKT_TAG_OPCODE_ADD,
                                                RECV_PKT_TYPE_IGMP_V4, 1234);
                    ol_ath_set_protocol_tagging(ic->ic_pdev_obj,
                                                RX_PKT_TAG_OPCODE_ADD,
                                                RECV_PKT_TYPE_MLD_V6, 4321);
            }
        }
#endif

#ifdef IPA_WDI3_VLAN_SUPPORT
    if (ucfg_ipa_is_vlan_enabled()) {
        ol_ath_set_protocol_tagging(ic->ic_pdev_obj,
                                    RX_PKT_TAG_OPCODE_ADD,
                                    RECV_PKT_TYPE_VLAN, 1122);
    }
#endif

        /* as a part of pdev deinit wdi callback are removed and set to NULL */
        qdf_atomic_init(&scn->tx_data_frame_ref);

        if (soc->recovery_in_progress) {
            /*Enable M_COPY after recovery if it was enabled before recovery*/
            if (ol_ath_is_mcopy_enabled(ic)) {
                sniffer_mode = ic->ic_debug_sniffer;
                ic->ic_debug_sniffer = 0;
                ol_ath_set_debug_sniffer(scn, sniffer_mode);
                if (ic->ic_monitor_version != MONITOR_VERSION_2){
                    ol_ath_pdev_set_param(scn->sc_pdev,
                                          wmi_pdev_param_set_promisc_mode_cmdid,
                                          1);
                }
            } else if (ic->ic_tx_capture) {
                    ol_ath_set_debug_sniffer(scn, ic->ic_tx_capture);
            }

            /*Enable tx latency capture after recovery if it was enabled before recovery*/
            if (ic->ic_capture_latency) {
                uint32_t tmp_mode = ic->ic_capture_latency;

                ic->ic_capture_latency = 0;
                ol_ath_set_capture_latency(scn, tmp_mode);
            }
        } else {
            /* Disable M_COPY if it was enabled after wifi down */
            if ((ol_ath_is_mcopy_enabled(ic) || ic->ic_tx_capture))
                ol_ath_set_debug_sniffer(scn, 0);

            /* Disable tx latency capture if it was enabled after wifi down */
            if (ic->ic_capture_latency)
                ol_ath_set_capture_latency(scn, 0);
        }

#ifdef QCA_SUPPORT_LITE_MONITOR
        if (ic->ic_monitor_version == MONITOR_VERSION_2 &&
            soc->recovery_in_progress) {
            if (ic->rx_config) {
                monitor_set_lite_monitor_config(scn, ic->rx_config);
                qdf_mem_free(ic->rx_config);
                ic->rx_config = NULL;
            }
            if (ic->tx_config) {
                monitor_set_lite_monitor_config(scn, ic->tx_config);
                qdf_mem_free(ic->tx_config);
                ic->tx_config = NULL;
            }
        }
#endif

        ieee80211com_set_cap(ic, IEEE80211_C_WEP);
        ieee80211com_set_cap(ic, IEEE80211_C_AES);
        ieee80211com_set_cap(ic, IEEE80211_C_AES_CCM);
        ieee80211com_set_cap(ic, IEEE80211_C_CKIP);
        ieee80211com_set_cap(ic, IEEE80211_C_TKIP);
        ieee80211com_set_cap(ic, IEEE80211_C_TKIPMIC);

#if ATH_SUPPORT_WAPI
        ieee80211com_set_cap(ic, IEEE80211_C_WAPI);
#endif

        wlan_scan_update_channel_list(ic);

        /* Initialize tx packet capture feature flag */
        ic->ic_tx_pkt_capture = 0;
#ifdef ATH_SUPPORT_WAPI
        if ((target_type == TARGET_TYPE_QCA9984)
            || (target_type == TARGET_TYPE_QCA9888)) {
            A_UINT8 bit_loc;
            ATH_VAP_ID_BIT_LOC(bit_loc);
            qdf_info("WAPI MBSSID %d", bit_loc);
            ol_ath_pdev_set_param(scn->sc_pdev,
                                  wmi_pdev_param_wapi_mbssid_offset, bit_loc);
        }
#endif
        if (ol_ath_thermal_mitigation_attach(scn, scn->netdev)) {
            wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_NB_ID);
            goto fail;
        }

#if OL_ATH_SUPPORT_LED
        if (!bypasswmi) {
#if QCA_LTEU_SUPPORT
          if (wlan_psoc_nif_feat_cap_get(soc->psoc_obj, WLAN_SOC_F_LTEU_SUPPORT)) {
              tgt_gpio_config(psoc, scn->scn_led_gpio, 1, WMI_HOST_GPIO_PULL_DOWN, WMI_HOST_GPIO_INTMODE_RISING_EDGE, 0, 0, 0);
          } else {
#endif /* QCA_LTEU_SUPPORT */

              ol_ath_led_init(soc, scn, target_type);
#if QCA_LTEU_SUPPORT
          }
#endif /* QCA_LTEU_SUPPORT */
        }
#endif /* OL_ATH_SUPPORT_LED */
        wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_NB_ID);

        ol_ath_update_chainmask(ic, target_cap, &mac_phy_cap[pdev_idx]);
        ol_ath_pdev_dfs_phyerr_offload_en(scn->sc_pdev);
#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(HOST_DFS_SPOOF_TEST)
        ic->ic_rebuilt_chanlist = 0;
        dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
        if (dfs_rx_ops) {
            if(dfs_rx_ops->dfs_reset_spoof_test)
                dfs_rx_ops->dfs_reset_spoof_test(scn->sc_pdev);
        }
#endif /* HOST_DFS_SPOOF_TEST */

#if defined(WLAN_DFS_PARTIAL_OFFLOAD) && defined(WLAN_DFS_SYNTHETIC_RADAR)
        dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
        if (dfs_rx_ops && dfs_rx_ops->dfs_allow_hw_pulses)
                dfs_rx_ops->dfs_allow_hw_pulses(scn->sc_pdev, true);
#endif /* WLAN_DFS_PARTIAL_OFFLOAD && WLAN_DFS_SYNTHETIC_RADAR*/

#if defined(WLAN_DFS_FULL_OFFLOAD) && defined(QCA_DFS_NOL_OFFLOAD)
        /* Resetting the scan violation status on FW restart. */
        ic->ic_is_dfs_scan_violation = false;
#endif

        /* Reset/Init the AP power type of the wifi */
        ucfg_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &ic->ic_ucfg_6g_ap_pwr_type);

#ifdef OL_ATH_SMART_LOGGING
        {
            struct wmi_unified *wmi_handle;

            wmi_handle = target_psoc_get_wmi_hdl(tgt_hdl);
            if (wmi_service_enabled(wmi_handle,
                                    wmi_service_smart_logging_support)) {
                void *dbglog_handle = target_psoc_get_dbglog_hdl(tgt_hdl);

                if (dbglog_handle && !ic->smart_logging)
                    fwdbg_smartlog_init(dbglog_handle, ic);
            }
	}
#endif /* OL_ATH_SMART_LOGGING */

        if (lmac_is_target_ar900b(soc->psoc_obj)) {
           /* HIF subscribe */
#ifndef REMOVE_PKT_LOG
           if (soc->ol_if_ops->hif_pktlog_subscribe) {
		if (soc->ol_if_ops->hif_pktlog_subscribe(scn) != QDF_STATUS_SUCCESS) {
		    QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
				QDF_TRACE_LEVEL_INFO,
				"Unable to subscribe to the HIF %s\n",__FUNCTION__);
		}
           }
#endif
        }
        ol_ath_configure_cong_ctrl_max_msdus(scn);

        ol_ath_pdev_set_burst(scn, true);

        ol_tbr_rsta_config_rsid(psoc, pdev_obj);

        scan_radio_tx_on_dfs_channels =
            (wlan_pdev_nif_feat_ext_cap_get(pdev_obj, WLAN_PDEV_FEXT_SCAN_RADIO) &&
             cfg_get(psoc, CFG_OL_SCAN_RADIO_TX_ON_DFS_CHANNELS)) ? 1 : 0;
        ol_ath_pdev_set_param(pdev_obj, wmi_pdev_param_scan_radio_tx_on_dfs,
                              scan_radio_tx_on_dfs_channels);

        if (wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,WLAN_PDEV_F_MBSS_IE_ENABLE)
                && !ic->is_mbssid_enabled) {
            ic->is_mbssid_enabled = true;
        }
        if (ol_target_be(psoc) && (ol_ath_get_conn_mode() == QDF_GLOBAL_MISSION_MODE))
            ol_ath_enable_telemetry_stats(pdev_obj);

#ifdef WLAN_REG_PARTIAL_OFFLOAD
        ol_ath_set_regdmn(psoc, pdev_obj, scn, tgt_hdl);
#endif

        ic->wfa_test_rnr = 0;
#ifdef WLAN_FEATURE_11BE_MLO_3_LINK_TX
        ol_ath_pdev_set_param(pdev_obj, wmi_pdev_param_tid_mapping_3link_mlo,
                              cfg_get(psoc, CFG_TGT_MLO_3_LINK_TX));
#endif
    } /* for loop */

    ol_ath_diag_user_agent_init(soc);

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (ol_ath_mlo_capable(psoc)) {
        uint32_t recov = ol_ath_get_fw_recov_mode(soc, soc->recovery_enable);
        ol_ath_send_fw_recovery(psoc, recov);
    }
#endif

    if (soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        /* Setup hw mode switch context */
        ol_ath_hw_mode_setup_ctx(soc);
    } else {
        /* check and reconfigure HW mode to target mode */
        soc->hw_mode_ctx.is_boot_in_progress = true;
        ol_ath_check_and_reconfig_hw_mode(soc);
        soc->hw_mode_ctx.is_boot_in_progress = false;
    }

    soc->down_complete = false;
    soc->soc_attached = 1;
    return 0;

fail:
   qdf_assert_always(0);
   hif_disable_isr(hif_hdl);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
   if (soc->nss_soc.ops) {
       qdf_info("nss-wifi: detach - zero delay");
       soc->nss_soc.ops->nss_soc_wifi_detach(soc, 0);
   }
#endif

    /* Unlikely case.
     * Flush pending pci_reconnect_work
     */
    qdf_flush_work(&soc->pci_reconnect_work);
    ol_ath_disconnect_htc(soc);
    if (soc->glb_shmem_init_complete)
        ol_mlo_glb_h_shmem_arena_ctx_deinit(soc);
    ol_ath_target_deinit(soc, 1);
    ol_ath_target_deinit_complete(soc);
    if((target_type != TARGET_TYPE_QCA8074) &&
       (target_type != TARGET_TYPE_QCA8074V2) &&
       (target_type != TARGET_TYPE_QCA9574) &&
       (target_type != TARGET_TYPE_QCN9000) &&
       (target_type != TARGET_TYPE_QCN9224) &&
       (target_type != TARGET_TYPE_QCN6122) &&
       (target_type != TARGET_TYPE_QCA5332) &&
       (target_type != TARGET_TYPE_QCN9160) &&
       (target_type != TARGET_TYPE_QCA5018) &&
       (target_type != TARGET_TYPE_QCA6018) &&
       (target_type != TARGET_TYPE_QCN6432)) {
        pci_set_drvdata(pcidev, soc->sc_osdev->netdev);
    }

    init_deinit_free_num_units(soc->psoc_obj, tgt_hdl);
    target_psoc_set_wmi_ready(tgt_hdl, FALSE);
    target_psoc_set_wmi_service_ready(tgt_hdl, FALSE);
    wlan_psoc_nif_op_flag_clear(psoc, WLAN_SOC_OP_VHT_INVALID_CAP);
    soc->down_complete = true;
    return -1;
}

/* Load the firmware and enable interrupts during the first vap creation
 * or before sending any WMI command to the target.
 */
extern const struct of_device_id ath_wifi_of_match[];
extern int wifi3_ko_exit_in_progress;

int _ol_ath_recover_target_start(struct ol_ath_soc_softc *soc)
{
    struct platform_device *pdev;
#ifdef CONFIG_CNSS2_SUPPORT
    void *subhandle = NULL;
    struct pci_dev *pcidev = NULL;
    int target_type = lmac_get_tgt_type(soc->psoc_obj);
#endif

    pdev = (struct platform_device *)soc->sc_osdev->bdev;

    /*
     * Target has already been started, return success.
     */
    if (!(soc->down_complete))
        return 0;

#ifdef ATH_AHB
    if (wifi_exit_in_progress || wifi3_ko_exit_in_progress) {
        qdf_info("Exit in progress! return");
        return -EBUSY;
    }
#endif

#ifdef CONFIG_CNSS2_SUPPORT
    if ((target_type == TARGET_TYPE_QCA8074) ||
        (target_type == TARGET_TYPE_QCA8074V2) ||
        (target_type == TARGET_TYPE_QCA9574) ||
        (target_type == TARGET_TYPE_QCN6122) ||
        (target_type == TARGET_TYPE_QCN9160) ||
        (target_type == TARGET_TYPE_QCA5332) ||
        (target_type == TARGET_TYPE_QCA5018) ||
        (target_type == TARGET_TYPE_QCA6018) ||
        (target_type == TARGET_TYPE_QCN6432)) {
        subhandle = pld_subsystem_get(&pdev->dev, soc->device_id);
        if (IS_ERR_OR_NULL(subhandle))
          goto fail;
    }

    if((target_type == TARGET_TYPE_QCN9000) ||
       (target_type == TARGET_TYPE_QCN9224)) {
        /* this pcidev would have got removed during subsystem put */
        pcidev = (struct pci_dev *) pdev;
        subhandle = pld_subsystem_get(&pcidev->dev, soc->device_id);
        if (IS_ERR_OR_NULL(subhandle))
          goto fail;
    }

    qdf_info("subsystem_get success");
    return 0;

fail:
    qdf_err("subsystem get error %d",IS_ERR_OR_NULL(subhandle));
    return -EINVAL;
#else
    return -EINVAL;
#endif //CONFIG_CNSS2_SUPPORT


}

static int _ol_ath_target_start(struct ol_ath_soc_softc *soc)
{
    struct platform_device *pdev;
    int ret;
#ifdef CONFIG_CNSS2_SUPPORT
    void *subhandle = NULL;
    struct pci_dev *pcidev = NULL;
    int target_type = lmac_get_tgt_type(soc->psoc_obj);
#endif

    pdev = (struct platform_device *)soc->sc_osdev->bdev;

    if (!(soc->down_complete)) {
        /*
         * Target has already been started, return success.
         */
        return 0;
    }

#ifdef ATH_AHB
    if (wifi_exit_in_progress || wifi3_ko_exit_in_progress) {
        qdf_info("Exit in progress! return");
        return -EBUSY;
    }
#endif

    qdf_spin_lock_bh(&soc->soc_tgt_lock);
    if (qdf_atomic_test_and_set_bit(SOC_RESET_IN_PROGRESS_BIT,
                                    &soc->reset_in_progress)) {
        qdf_info("Soc %d Reset in progress, return", wlan_psoc_get_id(soc->psoc_obj));
        qdf_spin_unlock_bh(&soc->soc_tgt_lock);
        return -EBUSY;
    }
    qdf_spin_unlock_bh(&soc->soc_tgt_lock);

#ifdef CONFIG_CNSS2_SUPPORT
    if ((target_type == TARGET_TYPE_QCA8074) ||
        (target_type == TARGET_TYPE_QCA8074V2) ||
        (target_type == TARGET_TYPE_QCA9574) ||
        (target_type == TARGET_TYPE_QCA6018) ||
        (target_type == TARGET_TYPE_QCA5018) ||
        (target_type == TARGET_TYPE_QCA5332) ||
        (target_type == TARGET_TYPE_QCN6122) ||
        (target_type == TARGET_TYPE_QCN9160) ||
        (target_type == TARGET_TYPE_QCN9224) ||
        (target_type == TARGET_TYPE_QCN9000) ||
        (target_type == TARGET_TYPE_QCN6432)) {
        if((target_type == TARGET_TYPE_QCN9000) ||
           (target_type == TARGET_TYPE_QCN9224)) {
            /* this pcidev would have got removed during subsystem put */
            pcidev = (struct pci_dev *) pdev;
            subhandle = pld_subsystem_get(&pcidev->dev, soc->device_id);
            if (IS_ERR_OR_NULL(subhandle))
                goto fail;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
            pld_set_recovery_mode(&pcidev->dev, MLO_RECOVERY_MODE_0);
#endif
        } else {
            subhandle = pld_subsystem_get(&pdev->dev, soc->device_id);
            if (IS_ERR_OR_NULL(subhandle))
                goto fail;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
            pld_set_recovery_mode(&pdev->dev, MLO_RECOVERY_MODE_0);
#endif
        }

        qdf_info("subsystem_get success");
    }
#endif //CONFIG_CNSS2_SUPPORT

    ret = ol_ath_soc_start(soc);
    goto out;

fail:
    qdf_err("subsystem get error %d",IS_ERR_OR_NULL(subhandle));
    subhandle = NULL;
    ret = -EINVAL;

out:
    qdf_atomic_clear_bit(SOC_RESET_IN_PROGRESS_BIT, &soc->reset_in_progress);
    return ret;
}

int ol_ath_recover_target_start(struct ol_ath_soc_softc *soc)
{
   int ret = 0;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
   ol_ath_soc_softc_t *temp_soc;
   struct wlan_objmgr_psoc *grp_soc_list[MAX_MLO_CHIPS];
   uint8_t soc_idx;
   uint8_t grp_id;
   uint8_t tot_grp_socs;

   if ((ol_ath_mlo_capable(soc->psoc_obj)) && (soc->recovery_type == MLO_RECOVERY_MODE_0)) {
         grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);

         tot_grp_socs = mlo_setup_get_total_socs(grp_id);
         if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
              return -EINVAL;

         mlo_get_soc_list(grp_soc_list, grp_id, tot_grp_socs, WLAN_MLO_GROUP_DEFAULT_SOC_LIST);

         /* In case of MLD, check all the other soc targets are created
            if not start them too and in order of soc 0, 1 and 2 */
         for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
              temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
              if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj) && !temp_soc->target_ready) {
                   return 0;
              }
         }
#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
         /* Initially config max links in recovery if remap is done */
         if (soc->psoc_obj->wsi_remap_recovery_in_progress)
              recovery_wsi_static_init();
#endif
         for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
              temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
              if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj)) {
                  ret = _ol_ath_recover_target_start(temp_soc);
              }
              if (ret)
                  qdf_info("Start MLD target failed for psoc id %d",soc_idx);
         }

   } else
#endif
       ret = _ol_ath_recover_target_start(soc);

   return ret;
}

void ol_ath_recovery_queue(void *context)
{
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    uint8_t soc_idx = 0;
    ol_ath_soc_softc_t *tmp_soc = NULL;
    uint8_t tot_grp_socs = 0;
    uint8_t total_grp = 0;
    uint8_t grp_id = 0;
    osdev_t osdev;

    total_grp = pld_get_num_mlo_groups();
    if (!total_grp || total_grp > WLAN_MAX_MLO_GROUPS)
        return;

    for (grp_id = 0; grp_id < total_grp ; grp_id++) {
         tot_grp_socs = mlo_setup_get_total_socs(grp_id);
         if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
             return;

         if (g_recov_wq->num_ready_socs[grp_id] == tot_grp_socs) {
             for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
                  tmp_soc = g_recov_wq->soc_list[grp_id][soc_idx];
                  if (tmp_soc) {
                     osdev = get_osdev_from_psoc(tmp_soc->psoc_obj);
                     if (osdev)
                         pld_set_recovery_mode(osdev->device, MLO_RECOVERY_MODE_0);

                      ol_ath_recover_target_start(tmp_soc);
                  }
             }
         }
    }
#endif
}

int ol_ath_recover_work_queue_init()
{
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)

    if (!g_recov_wq) {
          g_recov_wq =  qdf_mem_malloc(sizeof(struct ol_tgt_recovery_wq));

          if (!g_recov_wq) {
               qdf_err(" Unable to mem allocate recovery workqueue, FW Recovery failed");
               return -EINVAL;
          }

           qdf_create_work(0, &g_recov_wq->work, ol_ath_recovery_queue, NULL);
           g_recov_wq->work_queue =
                   qdf_alloc_unbound_workqueue("ol_ath_recover_work_queue");

           if (!g_recov_wq->work_queue) {
               qdf_err(" Unable to allocate recovery workqueue, FW Recovery failed");
               qdf_flush_work(&g_recov_wq->work);
               qdf_destroy_work(0, &g_recov_wq->work);
               qdf_mem_free(g_recov_wq);
               return -EINVAL;
            }

            qdf_spinlock_create(&g_recov_wq->list_lock);

    } else {
        qdf_debug(" Recovery Work queue is already initialized");
    }
#endif
    return 0;
}

int ol_ath_recover_work_queue_reset()
{
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    uint8_t i = 0;
    uint8_t j = 0;

    if (!g_recov_wq) {
        qdf_err("Recovery Work queue structure empty!");
        return -EINVAL;
    }

    if (!g_recov_wq->work_queue) {
       qdf_err("Recovery Work queue not found!");
       return -EINVAL;
    }

    for (i = 0; i < WLAN_MAX_MLO_GROUPS; i++) {
         g_recov_wq->num_ready_socs[i] = 0;
         for (j = 0; j < MAX_MLO_CHIPS; j++) {
              g_recov_wq->soc_list[i][j] = NULL;
         }
    }
#endif
    return 0;
}

int ol_ath_recover_work_queue_deinit()
{
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (!g_recov_wq) {
        qdf_debug("Recovery Workqueue is already deinitialized");
        return 0;
    }

    if (!g_recov_wq->work_queue)
         goto out;

    qdf_flush_workqueue(0, g_recov_wq->work_queue);
    qdf_destroy_workqueue(0, g_recov_wq->work_queue);
    qdf_flush_work(&g_recov_wq->work);
    qdf_disable_work(&g_recov_wq->work);
    qdf_spinlock_destroy(&g_recov_wq->list_lock);
out:
    qdf_mem_free(g_recov_wq);
    g_recov_wq = NULL;
#endif
    return 0;
}

int ol_ath_recover_deferred_start(struct ol_ath_soc_softc *soc)
{
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
   uint8_t grp_id;
   uint8_t index;

   if (!g_recov_wq) {
       qdf_err("Recovery Work queue uninitialized");
       return -EINVAL;
   }

   grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);

   if (grp_id < 0 || grp_id >= WLAN_MAX_MLO_GROUPS) {
       qdf_err("Recovery start failed as mlo group is invalid");
       return -EINVAL;
   }

   qdf_spin_lock(&g_recov_wq->list_lock);
   g_recov_wq->num_ready_socs[grp_id]++;
   qdf_spin_unlock(&g_recov_wq->list_lock);

   for (index = 0; index < MAX_MLO_CHIPS; index++)
        if (!g_recov_wq->soc_list[grp_id][index])
            break;

   if (index >= MAX_MLO_CHIPS)
       return -EINVAL;

   g_recov_wq->soc_list[grp_id][index] = soc;
   qdf_queue_work(0, g_recov_wq->work_queue, &g_recov_wq->work);
#endif
   return 0;
}

int ol_ath_recover_start(struct ol_ath_soc_softc *soc)
{
    if (!soc)
        return -EINVAL;

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    if (soc->recovery_type == MLO_RECOVERY_MODE_0)
        ol_ath_recover_deferred_start(soc);
    else
#endif
        ol_ath_recover_target_start(soc);

    return 0;
}

int ol_ath_target_start(struct ol_ath_soc_softc *soc)
{
   int ret = 0;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
   ol_ath_soc_softc_t *temp_soc = NULL;
   struct wlan_objmgr_psoc *grp_soc_list[MAX_MLO_CHIPS];
   uint8_t soc_idx;
   uint32_t grp_id = 0;
   uint32_t tot_grp_socs = 0;

   if (wifi_exit_in_progress || wifi3_ko_exit_in_progress) {
       qdf_info("Exit in progress! return");
       return -EBUSY;
   }

   /* Block the command for the chip that is bypassed */
   if (soc->psoc_obj->wsi_remap_remove) {
       qdf_info("Chip is down using WSI remap! return");
       return -EINVAL;
   }

   if (ol_ath_mlo_capable(soc->psoc_obj) && !(soc->psoc_obj->wsi_remap_add)) {
       grp_id = ol_ath_mlo_get_group_id(soc->psoc_obj);

       tot_grp_socs = mlo_setup_get_total_socs(grp_id);
       if (!tot_grp_socs || tot_grp_socs > MAX_MLO_CHIPS)
             return -EINVAL;

       mlo_get_soc_list(grp_soc_list, grp_id, tot_grp_socs, WLAN_MLO_GROUP_DEFAULT_SOC_LIST);

         /* In case of MLD, check all the other soc targets same mld group
            are created, if not start them too and in probe order */

       for (soc_idx = 0; soc_idx < tot_grp_socs; soc_idx++) {
            temp_soc = lmac_get_psoc_feature_ptr(grp_soc_list[soc_idx]);
            if (temp_soc && ol_ath_mlo_capable(temp_soc->psoc_obj))
                ret = _ol_ath_target_start(temp_soc);
            if (ret)
                qdf_info("Start MLD target failed for psoc id %d",soc_idx);
       }
#ifdef WLAN_FEATURE_DYNAMIC_WSI_REMAP
       if (soc->psoc_obj->wsi_remap_recovery_in_progress) {
            qdf_info("Recovery while WSI bypass done on system, psoc id %d",soc_idx);
            recovery_wsi_remap(soc);
       }
#endif
   } else
#endif
        ret = _ol_ath_target_start(soc);

   return ret;

}
qdf_export_symbol(ol_ath_target_start);

int
ol_ath_resume(struct ol_ath_softc_net80211 *scn)
{
    struct ieee80211com     *ic;

    ic = &scn->sc_ic;
    /*
     * ignore if already resumed.
     */
    if (OS_ATOMIC_CMPXCHG(&(scn->sc_dev_enabled), 0, 1) == 1) {
    return 0;
    }
    ieee80211_stop_running(ic);

    ieee80211_start_running(ic);

    return 0;
}

int
ol_ath_suspend(struct ol_ath_softc_net80211 *scn)
{
    /*
     * ignore if already suspended;
     */
    if (OS_ATOMIC_CMPXCHG(&(scn->sc_dev_enabled), 1, 0) == 0) {
        return 0;
    }

    ieee80211_stop_running(&scn->sc_ic);

    return 0;
}

void
ol_ath_target_status_update(ol_ath_soc_softc_t *soc, ol_target_status status)
{
    /* target lost, host needs to recover/reattach */
    soc->target_status = status;
}

void
ol_ath_suspend_resume_attach(struct ol_ath_softc_net80211 *scn)
{
}


int
ol_ath_suspend_target(ol_ath_soc_softc_t *soc, int disable_target_intr)
{
    struct pdev_op_args oper;

    oper.pointer = (void *)&disable_target_intr;
    oper.type = PDEV_ITER_SEND_SUSPEND;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
             wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    return 0;
}

int
ol_ath_resume_target(ol_ath_soc_softc_t *soc)
{
    struct pdev_op_args oper;

    oper.type = PDEV_ITER_SEND_RESUME;
    wlan_objmgr_iterate_obj_list(soc->psoc_obj, WLAN_PDEV_OP,
             wlan_pdev_operation, &oper, 0, WLAN_MLME_NB_ID);

    return oper.ret_val;
}

/*
 * reg_xxx - All the variables are contents of the corresponding
 *  		register contents
 * xxx_delta - computed difference between previous cycle and c
 * 		    current cycle
 * reg_xxx_cu	- Computed channel utillization in %,
 *  		computed through register statistics
 *
 * FIXME ideally OL and non-OL layers can re-use the same code.
 * But this is done differently between OL and non-OL paths.
 * We may need to rework this completely for both to work with single
 * piece of code. The non-OL path also need lot of rework. All
 * stats need to be taken to UMAC layer. That seems to be too
 * much of work to do at this point of time. Right now host code
 * is limited to one below function, and this function alone
 * need to go UMAC. Given that function is small, tend to keep
 * only here here. If this gets any bigger we shall try doing it
 * in umac, and merge entire code ( ol and non-ol to umac ).
 */
void
ol_ath_wlan_interference_handler(struct ol_ath_softc_net80211 * scn,
                                 wmi_host_dcs_im_tgt_stats_t *curr_stats,
                                 uint32_t interference_type)
{
	wmi_host_dcs_im_tgt_stats_t *prev_stats;

	u_int32_t reg_tsf_delta = 0;                /* prev-tsf32  - curr-tsf32 */
	u_int32_t rxclr_delta = 0;                  /* prev-RXCLR - curr-RXCLR */
	u_int32_t rxclr_ext_delta = 0;              /* prev-RXEXTCLR - curreent RXEXTCLR, most of the time this is zero, chip issue ?? */
	u_int32_t cycle_count_delta = 0;            /* prev CCYCLE - curr CCYCLE */
	u_int32_t tx_frame_delta = 0;               /* prev TFCT - curr TFCNT */
	u_int32_t rx_frame_delta = 0;               /* prev RFCNT - curr RFCNT */
	u_int32_t reg_total_cu = 0; 				/* total channel utilization in %*/
	u_int32_t reg_tx_cu = 0;					/* transmit channel utilization in %*/
    u_int32_t reg_rx_cu = 0;					/* receive channel utilization in %*/
	u_int32_t reg_unused_cu = 0;                /* unused channel utillization */
	u_int32_t rx_time_cu=0;						/* computed rx time*/
	u_int32_t reg_ofdm_phyerr_delta = 0;		/* delta ofdm errors */
	u_int32_t reg_cck_phyerr_delta = 0;			/* delta cck errors*/
	u_int32_t reg_ofdm_phyerr_cu = 0;			/* amount utilization by ofdm errors*/
	u_int32_t ofdm_phy_err_rate = 0;			/* rate at which ofdm errors are seen*/
	u_int32_t cck_phy_err_rate=0;				/* rate at which cck errors are seen*/
	u_int32_t max_phy_err_rate = 0;
    u_int32_t max_phy_err_count = 0;
	u_int32_t total_wasted_cu = 0;
	u_int32_t wasted_tx_cu = 0;
	u_int32_t tx_err = 0;
	int too_many_phy_errors = 0;

	if (!scn || !curr_stats) {
		qdf_info("DCS: scn is NULL");
		return;
	}
    prev_stats =  &scn->scn_dcs.scn_dcs_im_stats.prev_dcs_im_stats;

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        wlan_dcs_im_print_stats(prev_stats, curr_stats);
    }

    /* counters would have wrapped. Ideally we should be able to figure this
     * out, but we never know how many times counters wrapped. just ignore
     */
	if ((curr_stats->mib_stats.listen_time <= 0) ||
        (curr_stats->reg_tsf32 <= prev_stats->reg_tsf32)) {

		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
            qdf_info("DCS: ignoring due to negative TSF value");
        }
        /* copy the current stats to previous stats for next run */
		wlan_dcs_im_copy_stats(prev_stats, curr_stats);
		return;
	}

	reg_tsf_delta = curr_stats->reg_tsf32 - prev_stats->reg_tsf32;

	/* do nothing if current stats are not seeming good, probably
	 * a reset happened on chip, force cleared
	 */
	if (prev_stats->mib_stats.reg_rxclr_cnt > curr_stats->mib_stats.reg_rxclr_cnt) {
		if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
            qdf_info("DCS: ignoring due to negative rxclr count");
        }

        /* copy the current stats to previous stats for next run */
		wlan_dcs_im_copy_stats(prev_stats, curr_stats);
		return;
	}

	rxclr_delta = curr_stats->mib_stats.reg_rxclr_cnt - prev_stats->mib_stats.reg_rxclr_cnt;
	rxclr_ext_delta = curr_stats->mib_stats.reg_rxclr_ext_cnt -
								prev_stats->mib_stats.reg_rxclr_ext_cnt;
	tx_frame_delta = curr_stats->mib_stats.reg_tx_frame_cnt -
								prev_stats->mib_stats.reg_tx_frame_cnt;

	rx_frame_delta = curr_stats->mib_stats.reg_rx_frame_cnt -
								prev_stats->mib_stats.reg_rx_frame_cnt;

	cycle_count_delta = curr_stats->mib_stats.reg_cycle_cnt -
								prev_stats->mib_stats.reg_cycle_cnt;

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        qdf_info(",rxclr_delta,%u,rxclr_ext_delta,%u,tx_frame_delta,%u,rx_frame_delta,%u,cycle_count_delta,%u",
                rxclr_delta , rxclr_ext_delta , tx_frame_delta,
                rx_frame_delta , cycle_count_delta );
    }

    if(0 == (cycle_count_delta >>8)) {
        wlan_dcs_im_copy_stats(prev_stats, curr_stats);
        if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE))
            qdf_info(" cycle count NULL---Investigate--");
        return;
    }

	/* total channel utiliztaion is the amount of time RXCLR is
	 * counted. RXCLR is counted, when 'RX is NOT clear', please
	 * refer to mac documentation. It means either TX or RX is ON
     *
     * Why shift by 8 ? after multiplication it could overflow. At one
     * second rate, neither cycle_count_celta, nor the tsf_delta would be
     * zero after shift by 8 bits
	 */
	reg_total_cu = ((rxclr_delta >> 8) * 100) / (cycle_count_delta >>8);
	reg_tx_cu = ((tx_frame_delta >> 8 ) * 100) / (cycle_count_delta >> 8);
	reg_rx_cu = ((rx_frame_delta >> 8 ) * 100) / (cycle_count_delta >> 8);
	rx_time_cu = ((curr_stats->rx_time >> 8) * 100 ) / (reg_tsf_delta >> 8);

    /**
     * Amount of the time AP received cannot go higher than the receive
     * cycle count delta. If at all it is, there should have been a
     * compution error, ceil it to receive_cycle_count_diff
     */
	if (rx_time_cu > reg_rx_cu) {
		rx_time_cu = reg_rx_cu;
	}

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        qdf_info(",reg_total_cu,%u,reg_tx_cu,%u,reg_rx_cu,%u,rx_time_cu,%u",
                    reg_total_cu, reg_tx_cu, reg_rx_cu, rx_time_cu);
    }

	/* Unusable channel utilization is amount of time that we
	 * spent in backoff or waiting for other transmit/receive to
	 * complete. If there is interference it is more likely that
	 * we overshoot the limit. In case of multiple stations, we
	 * still see increased channel utilization.  This assumption may
	 * not be true for the VOW scenario where either multicast or
	 * unicast-UDP is used ( mixed traffic would still cause high
	 * channel utilization).
     */
	wasted_tx_cu = ((curr_stats->tx_waste_time >> 8) * 100 ) / (reg_tsf_delta >> 8);

    /**
     * transmit channel utilization cannot go higher than the amount of time
     * wasted, if so cap the wastage to transmit channel utillzation. This
     * could happen to compution error.
     */
	if (reg_tx_cu < wasted_tx_cu) {
		wasted_tx_cu = reg_tx_cu;
	}

	tx_err = (reg_tx_cu  && wasted_tx_cu) ? (wasted_tx_cu * 100 )/reg_tx_cu : 0;

    /**
     * The below actually gives amount of time we are not using, or the
     * interferer is active.
     * rx_time_cu is what computed receive time *NOT* rx_cycle_count
     * rx_cycle_count is our receive+interferer's transmit
     * un-used is really total_cycle_counts -
     *      (our_rx_time(rx_time_cu)+ our_receive_time)
     */
	reg_unused_cu = (reg_total_cu >= (reg_tx_cu + rx_time_cu)) ?
							(reg_total_cu - (reg_tx_cu + rx_time_cu)) : 0;

    /* if any retransmissions are there, count them as wastage
     */
	total_wasted_cu = reg_unused_cu + wasted_tx_cu;

	/* check ofdm and cck errors */
    if (unlikely(curr_stats->mib_stats.reg_ofdm_phyerr_cnt  <
            prev_stats->mib_stats.reg_ofdm_phyerr_cnt)) {
        reg_ofdm_phyerr_delta = curr_stats->mib_stats.reg_ofdm_phyerr_cnt ;
    } else {
        reg_ofdm_phyerr_delta = curr_stats->mib_stats.reg_ofdm_phyerr_cnt -
                                    prev_stats->mib_stats.reg_ofdm_phyerr_cnt;
    }

	if (unlikely(curr_stats->mib_stats.reg_cck_phyerr_cnt  <
            prev_stats->mib_stats.reg_cck_phyerr_cnt)) {
        reg_cck_phyerr_delta = curr_stats->mib_stats.reg_cck_phyerr_cnt;
    } else {
        reg_cck_phyerr_delta = curr_stats->mib_stats.reg_cck_phyerr_cnt -
                                    prev_stats->mib_stats.reg_cck_phyerr_cnt;
    }

	/* add the influence of ofdm phy errors to the wasted channel
	 * utillization, this computed through time wasted in errors,
	 */
	reg_ofdm_phyerr_cu = reg_ofdm_phyerr_delta * scn->scn_dcs.phy_err_penalty ;
	total_wasted_cu += (reg_ofdm_phyerr_cu > 0) ?  (((reg_ofdm_phyerr_cu >> 8) * 100) / (reg_tsf_delta >> 8)) : 0;

	ofdm_phy_err_rate = (curr_stats->mib_stats.reg_ofdm_phyerr_cnt * 1000) /
                                curr_stats->mib_stats.listen_time;
	cck_phy_err_rate = (curr_stats->mib_stats.reg_cck_phyerr_cnt * 1000) /
                                curr_stats->mib_stats.listen_time;

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        qdf_info(",reg_unused_cu,%u,reg_ofdm_phyerr_delta,%u,reg_cck_phyerr_delta,%u,reg_ofdm_phyerr_cu,%u",
                    reg_unused_cu , reg_ofdm_phyerr_delta , reg_cck_phyerr_delta , reg_ofdm_phyerr_cu);
        qdf_info(",total_wasted_cu,%u,ofdm_phy_err_rate,%u,cck_phy_err_rate,%u",
                    total_wasted_cu , ofdm_phy_err_rate , cck_phy_err_rate );
        qdf_info(",new_unused_cu,%u,reg_ofdm_phy_error_cu,%u",
                reg_unused_cu, (curr_stats->mib_stats.reg_ofdm_phyerr_cnt*100)/
                                        curr_stats->mib_stats.listen_time);
    }

	/* check if the error rates are higher than the thresholds*/
	max_phy_err_rate = MAX(ofdm_phy_err_rate, cck_phy_err_rate);

	max_phy_err_count = MAX(curr_stats->mib_stats.reg_ofdm_phyerr_cnt,
                                curr_stats->mib_stats.reg_cck_phyerr_cnt);

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        qdf_info(",max_phy_err_rate,%u, max_phy_err_count,%u",max_phy_err_rate , max_phy_err_count);
    }

	if (((max_phy_err_rate >= scn->scn_dcs.phy_err_threshold) &&
				(max_phy_err_count > scn->scn_dcs.phy_err_threshold)) ||
         (curr_stats->phyerr_cnt > scn->scn_dcs.radar_err_threshold)) {
		too_many_phy_errors = 1;
	}

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_CRITICAL)) {
		qdf_info("\nDCS: total_cu %u ,tx_cu %u ,rx_cu %u ,rx_time_cu %u, unused cu %u ",reg_total_cu, reg_tx_cu, reg_rx_cu, rx_time_cu, reg_unused_cu);
                qdf_info("\nDCS: phyerr %u ,total_wasted_cu %u ,phyerror_cu %u ,wasted_cu %u, reg_tx_cu %u ,reg_rx_cu %u",too_many_phy_errors, total_wasted_cu, reg_ofdm_phyerr_cu, wasted_tx_cu, reg_tx_cu, reg_rx_cu);
                qdf_info("\nDCS: tx_err %u",tx_err);
	}

	if (reg_unused_cu >= scn->scn_dcs.coch_intr_thresh) {
		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt+=2; /* quickly reach to decision*/
	} else if (too_many_phy_errors &&
			   (((total_wasted_cu > (scn->scn_dcs.coch_intr_thresh+10)) &&
					((reg_tx_cu + reg_rx_cu) > scn->scn_dcs.user_max_cu)) ||
                            ((reg_tx_cu > DCS_TX_MAX_CU) && (tx_err >= scn->scn_dcs.tx_err_thresh)) )){
		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt++;
	}

	if (scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt >= scn->scn_dcs.intr_detection_threshold) {

        if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_CRITICAL)) {
            qdf_info("interference threshould exceeded");
            qdf_info(",unused_cu,%u,too_any_phy_errors,%u,total_wasted_cu,%u,reg_tx_cu,%u,reg_rx_cu,%u\n",
                    reg_unused_cu, too_many_phy_errors, total_wasted_cu,reg_tx_cu, reg_rx_cu);
        }

		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt = 0;
		scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt = 0;
        /* once the interference is detected, change the channel, as on
         * today this is common routine for wirelesslan and non-wirelesslan
         * interference. Name as such kept the same because of the DA code,
         * which is using the same function.
         */
		ol_ath_dcs_generic_interference_handler(scn, NULL, interference_type);
	} else if (!scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt ||
				scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt >= scn->scn_dcs.intr_detection_window) {
		scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt = 0;
		scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt = 0;
	}

	/* count the current run too*/
	scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt++;

    /* copy the stats for next cycle */
	wlan_dcs_im_copy_stats(prev_stats, curr_stats);

    if (unlikely(scn->scn_dcs.dcs_debug >= DCS_DEBUG_VERBOSE)) {
        qdf_info(",intr_count,%u,sample_count,%d",
                 scn->scn_dcs.scn_dcs_im_stats.im_intr_cnt,
                 scn->scn_dcs.scn_dcs_im_stats.im_samp_cnt);
    }
}

int
ol_ath_periodic_chan_stats_config (struct ol_ath_softc_net80211 * scn,
                                        bool enable,
                                        u_int32_t stats_period)
{
	struct periodic_chan_stats_params param;
	struct wmi_unified *pdev_wmi_handle;

	if (!scn || !(scn->periodic_chan_stats)) {
		return -EINVAL;
	}

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
	if (!pdev_wmi_handle)
		return -EINVAL;

	qdf_mem_set(&param, sizeof(param), 0);
  	param.stats_period = stats_period;
  	param.enable = enable;
	param.pdev_id = lmac_get_pdev_idx(scn->sc_pdev);
  	return wmi_unified_send_periodic_chan_stats_config_cmd(pdev_wmi_handle,&param);
}

int
ol_ath_pdev_ctl_failsafe_check_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_host_pdev_ctl_failsafe_event ctl_failsafe = {0};
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_extract_ctl_failsafe_check_ev_param(wmi_handle, data, &ctl_failsafe)) {
        return -1;
    }

    if (ctl_failsafe.ctl_failsafe_status != 0)
        QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY, QDF_TRACE_LEVEL_ERROR,
        "%s: CTL FAILSAFE Algorithm invoked when computing CTL table",__func__);

    return 0;
}

int
ol_ath_pdev_caldata_version_check_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    wmi_host_pdev_check_cal_version_event cal, *calInfo = &cal;
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_pdev_caldata_version_check_ev_param(wmi_handle, data, calInfo)) {
        return -1;
    }

    qdf_info("****************** CAl DATA Version Details ********************");
    if (calInfo->cal_ok != WMI_HOST_NO_FEATURE){
        qdf_info("Current Meta Cal Version               : 0x%x ",
                                              calInfo->software_cal_version);

        qdf_info("Board (MCN : %s) Cal Version           : 0x%x ",
                       calInfo->board_mcn_detail, calInfo->board_cal_version);

        if (calInfo->cal_ok == WMI_HOST_CALIBRATION_OK){
            qdf_info("Calibration Status: OK ");
        } else { /* calInfo->cal_ok  CALIBRATION_NOT_OK */
            qdf_info("Calibration Status: NOT OK ");
        }
        qdf_info("Note: Please cross check board's MCN ");
    } else {
        qdf_info("Board was not calibrated with this feature ");
    }

    return 0;
}

#if defined(WLAN_CONFIG_TELEMETRY_AGENT) && defined(QCA_SUPPORT_CP_STATS)
int
ol_ath_cp_fwstats_event_handler_telemetry(ol_soc_t sc,
                                           u_int8_t *evt_buf,
                                           u_int32_t datalen,
                                           wmi_unified_t wmi_handle,
                                           struct wmi_raw_event_buffer *raw_buf)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct infra_cp_stats_event params = {0};
    struct pdev_ic_cp_stats *pdev_cps = NULL;
    struct ctrl_path_pmlo_telemetry_stats_struct telemetry_stats = {0};

    params.telemetry_stats = &telemetry_stats;
    if (wmi_unified_extract_infra_cp_stats(wmi_handle, raw_buf->evt_processed_buf, datalen, &params) !=
                                    QDF_STATUS_SUCCESS) {
            qdf_err("Unable to extract cp_fwstats event");
            return -1;
    }
    if (params.request_id == TYPE_REQ_CTRL_PATH_PMLO_STAT) {
        pdev = wlan_objmgr_get_pdev_by_id(psoc, params.telemetry_stats->pdev_id, WLAN_MLME_SB_ID);
        if (pdev == NULL) {
            qdf_err("pdev object (id: %d) is NULL ", params.telemetry_stats->pdev_id);
            return -1;
        }

        pdev_cps = wlan_get_pdev_cp_stats_ref(pdev);
        if (!pdev_cps) {
            qdf_err("Failed to get pdev cp stats reference");
            wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
            return -1;
        }

        pdev_cps->stats.telemetry_stats = telemetry_stats;
        cdp_update_pdev_mon_telemetry_airtime_stats(wlan_psoc_get_dp_handle(psoc),
                                                    wlan_objmgr_pdev_get_pdev_id(pdev));
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
        return 0;
    }
    return 1;
}
#else
int
ol_ath_cp_fwstats_event_handler_telemetry(ol_soc_t sc,
                                           u_int8_t *evt_buf,
                                           u_int32_t datalen,
                                           wmi_unified_t wmi_handle,
                                           struct wmi_raw_event_buffer *raw_buf)
{
    return 1;
}
#endif

int
ol_ath_cp_fwstats_event_handler (ol_soc_t sc, u_int8_t *evt_buf, u_int32_t datalen)
{
    int ret;
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    uint32_t more_flag = 0;
    struct wmi_unified *wmi_handle;
    struct ieee80211com *ic = NULL;
    struct wmi_raw_event_buffer *raw_buf;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    raw_buf = (struct wmi_raw_event_buffer *)evt_buf;
    if (wmi_unified_extract_cp_stats_more_pending(wmi_handle, raw_buf->evt_processed_buf, &more_flag) !=
                                    QDF_STATUS_SUCCESS) {
            qdf_err("Unable to extract cp_fwstats event");
            return -1;
    }

    ret = ol_ath_cp_fwstats_event_handler_telemetry(sc,
                                                    evt_buf,
                                                    datalen,
                                                    wmi_handle,
                                                    raw_buf);
    if(ret != 1) {
        return ret;
    }

    ic = soc->cp_stats_ic;
    if (!ic) {
        qdf_err("ic is NULL \n");
        return -1;
    }
#if UMAC_SUPPORT_CFG80211
    wlan_cfg80211_wifi_fwstats_event(ic, raw_buf->evt_raw_buf, datalen);
#endif
    if (!more_flag)
        soc->cp_stats_ic = NULL;
    return 0;
}

#ifdef WLAN_FEATURE_11BE_MLO
static QDF_STATUS
ol_ath_vdev_update_fw_tsf_params(wlan_if_t vap,  struct vap_tsf *tsf_params)
{
    struct vap_tsf_offset_calc_params *tsf_offset_calc_params;
    struct wlan_mlo_dev_context *mlo_dev_ctx;
    struct wlan_objmgr_vdev *vdev;

    if (!vap) {
        qdf_err("vap is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!tsf_params) {
        qdf_err("TSF params is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        qdf_err("vdev is null");
        return QDF_STATUS_E_NULL_VALUE;
    }

    mlo_dev_ctx = vdev->mlo_dev_ctx;
    if (!mlo_dev_ctx)
        return QDF_STATUS_SUCCESS;

    tsf_recalculation_lock_acquire(mlo_dev_ctx);

    tsf_offset_calc_params = &vap->iv_tsf_offset_calc_params;
    tsf_offset_calc_params->tsf_snapshot_fw =
            tsf_params->tsf_low | ((uint64_t)tsf_params->tsf_high << 32);
    tsf_offset_calc_params->qtimer_snapshot_fw =
            tsf_params->qtimer_low | ((uint64_t)tsf_params->qtimer_high << 32);
    tsf_offset_calc_params->fw_snapshots_valid = true;
    tsf_offset_calc_params->tsf_recalculation_required = true;

    tsf_recalculation_lock_release(mlo_dev_ctx);

    return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_11BE_MLO */

int
ol_ath_halphy_fwstats_event_handler (ol_soc_t sc, u_int8_t *evt_buf, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    uint32_t end_of_event_flag = 0;
    uint32_t event_count = 0;
    struct wmi_unified *wmi_handle;
    struct ieee80211com *ic = NULL;
    struct wmi_raw_event_buffer *raw_buf;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    raw_buf = (struct wmi_raw_event_buffer *)evt_buf;
    if (wmi_unified_extract_halphy_stats_end_of_event(wmi_handle, raw_buf->evt_processed_buf, &end_of_event_flag) !=
                                    QDF_STATUS_SUCCESS) {
            qdf_err("Unable to extract halphy_fwstats end of event");
            return -1;
    }
    if (wmi_unified_extract_halphy_stats_event_count(wmi_handle, raw_buf->evt_processed_buf, &event_count) !=
                                    QDF_STATUS_SUCCESS) {
            qdf_err("Unable to extract halphy_fwstats event count");
            return -1;
    }
    ic = soc->cp_stats_ic;
    if (!ic) {
        qdf_err("ic is NULL \n");
        return -1;
    }
#if UMAC_SUPPORT_CFG80211
    wlan_cfg80211_wifi_fwstats_event(ic, raw_buf->evt_raw_buf, datalen);
#endif
    if (end_of_event_flag)
        soc->cp_stats_ic = NULL;
    return 0;
}

int
ol_ath_vdev_tsf_report_event_handler(ol_soc_t sc, u_int8_t *data,
                                     u_int32_t datalen)
{
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_vdev *vdev;
    struct wmi_host_tsf_event param;
    wlan_if_t vap;
    uint32_t delta_tsf;
#ifdef WLAN_FEATURE_11BE_MLO
    QDF_STATUS status;
#endif /* WLAN_FEATURE_11BE_MLO */

    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    psoc = soc->psoc_obj;

    wmi_handle = lmac_get_wmi_hdl(psoc);

    if (!wmi_handle) {
        qdf_err("wmi_handle is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (wmi_extract_vdev_tsf_report_event(wmi_handle, data, &param)) {
        qdf_err("Failed to extract vdev_tsf_report_event");
        return QDF_STATUS_E_FAILURE;
    }

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, param.vdev_id,
            WLAN_DEBUG_ID);
    if (!vdev) {
        qdf_err("vdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_DEBUG_ID);
        return QDF_STATUS_E_FAILURE;
    }

    vap->iv_tsf.iv_tsf = param.tsf;
    vap->iv_tsf.tsf_low = param.tsf_low;
    vap->iv_tsf.tsf_high = param.tsf_high;
    vap->iv_tsf.qtimer_low = param.qtimer_low;
    vap->iv_tsf.qtimer_high = param.qtimer_high;
    vap->iv_tsf.tsf_id = param.tsf_id;
    vap->iv_tsf.tsf_id_valid = param.tsf_id_valid;
    vap->iv_tsf.mac_id = param.mac_id;
    vap->iv_tsf.mac_id_valid = param.mac_id_valid;
    vap->iv_tsf.wlan_global_tsf_low = param.wlan_global_tsf_low;
    vap->iv_tsf.wlan_global_tsf_high = param.wlan_global_tsf_high;
    vap->iv_tsf.tqm_timer_low = param.tqm_timer_low;
    vap->iv_tsf.tqm_timer_high = param.tqm_timer_high;
    vap->iv_tsf.use_tqm_timer = param.use_tqm_timer;

    if (vap->iv_tsf.use_tqm_timer)
        delta_tsf = vap->iv_tsf.tsf_low - vap->iv_tsf.tqm_timer_low;
    else
        delta_tsf = vap->iv_tsf.tsf_low - vap->iv_tsf.qtimer_low;

#ifdef WLAN_CONFIG_TX_DELAY
    cdp_set_delta_tsf(wlan_psoc_get_dp_handle(psoc), wlan_vdev_get_id(vdev),
                      delta_tsf);
#endif
    qdf_atomic_inc(&(vap->vap_tsf_event));

#ifdef WLAN_FEATURE_11BE_MLO
    status = ol_ath_vdev_update_fw_tsf_params(vap, &vap->iv_tsf);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Failed to update FW TSF params");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_DEBUG_ID);
        return QDF_STATUS_E_FAILURE;
    }
#endif /* WLAN_FEATURE_11BE_MLO */

    wlan_objmgr_vdev_release_ref(vdev, WLAN_DEBUG_ID);

    return QDF_STATUS_SUCCESS;
}

int
ol_ath_pdev_tpc_config_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    if (soc->ol_if_ops->tpc_config_handler)  {
        soc->ol_if_ops->tpc_config_handler(sc, data, datalen);
    }

    return 0;
}

int ol_ath_packet_power_info_get(struct wlan_objmgr_pdev *pdev,
                                 struct packet_power_info_params *arg)
{
    struct packet_power_info_params param;
    struct wmi_unified *pdev_wmi_handle;
    QDF_STATUS status;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle)
        return -1;

    qdf_mem_set(&param, sizeof(param), 0);
    param.chainmask = arg->chainmask;
    param.chan_width = arg->chan_width;
    param.rate_flags = arg->rate_flags;
    param.su_mu_ofdma = arg->su_mu_ofdma;
    param.nss = arg->nss;
    param.preamble = arg->preamble;
    param.hw_rate = arg->hw_rate;

    status = wmi_unified_packet_power_info_get_cmd_send(pdev_wmi_handle,
                                                        &param);
    return qdf_status_to_os_return(status);
}

int ol_ath_halphy_cal_status_get(struct wlan_objmgr_pdev *pdev)
{
    struct halphy_cal_status_params param;
    struct wmi_unified *pdev_wmi_handle;
    QDF_STATUS status;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle)
        return -1;

    qdf_mem_set(&param, sizeof(param), 0);
    param.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    status = wmi_unified_halphy_cal_status_get_cmd_send(pdev_wmi_handle, &param);

    return qdf_status_to_os_return(status);
}

int
ol_ath_btcoex_duty_cycle(ol_ath_soc_softc_t *soc, u_int32_t period, u_int32_t duration)
{
    struct btcoex_cfg_params param;
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.period = period;
    param.wlan_duration = duration ;
    param.btcoex_param_flags = WMI_HOST_BTCOEX_PARAM_FLAGS_DUTY_CYCLE_BIT;
    return wmi_unified_send_btcoex_duty_cycle_cmd(wmi_handle, &param);
}

int
ol_ath_btcoex_wlan_priority(ol_ath_soc_softc_t *soc, u_int32_t wlan_priority)
{
    struct btcoex_cfg_params param;
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.btcoex_wlan_priority_bitmap = wlan_priority;
    param.btcoex_param_flags = WMI_HOST_BTCOEX_PARAM_FLAGS_WLAN_PRIORITY_BITMAP_BIT;
    return wmi_unified_send_btcoex_wlan_priority_cmd(wmi_handle, &param);
}

int
ol_ath_send_egid_info_cmd(ol_ath_soc_softc_t *soc, u_int32_t *egid_info)
{
    struct esl_egid_params param;
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);

    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    qdf_mem_zero(&param, 0);
    qdf_mem_copy(param.egid_info, egid_info, sizeof(u_int32_t) * ESL_EGID_INFO_MAX_SIZE);

    return wmi_unified_send_egid_info_cmd(wmi_handle, &param);
}

int
ol_ath_gpio_input_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    uint32_t gpio_num;
#if UMAC_SUPPORT_ACFG
    acfg_event_data_t *acfg_event = NULL;
    struct net_device *dev = soc->netdev ;
    struct ol_ath_softc_net80211 *scn;
#endif
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_gpio_input_ev_param(wmi_handle, data, &gpio_num)) {
        return -1;
    }
    qdf_info("GPIO Input Event on Num %d", gpio_num);

#if UMAC_SUPPORT_ACFG
    if (!dev) {
       qdf_err("dev is null!!!");
       return -EINVAL;
    }

    scn = get_radio_device_context(dev->ieee80211_ptr->wiphy, dev);
    if (!scn) {
        qdf_err("scn is null!!!");
        return -EINVAL;
    }
    if (!osif_scn_event_filter_status_to_app(&scn->sc_ic, EVENT_ACFG,
                WL_EVENT_TYPE_GPIO_INPUT)) {
        return 0;
    }

    acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
    if (!acfg_event)
        return 0;

    acfg_event->gpio_num = gpio_num;
    /*MULTI-RADIO TODO - is it SoC or we need pdev here?*/
    acfg_send_event(soc->sc_osdev->netdev, soc->sc_osdev, WL_EVENT_TYPE_GPIO_INPUT, acfg_event);
    qdf_mem_free(acfg_event);
#endif
    return 0;
}

#define MAX_NUM_CHAINS 8
int
ol_ath_nf_dbr_dbm_info_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    u_int32_t i, j;
    wmi_host_pdev_nfcal_power_all_channels_event event, *ev = &event;
#if UMAC_SUPPORT_ACFG
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_pdev *pdev;
    acfg_event_data_t *acfg_event = NULL;
#endif
    struct wmi_unified *wmi_handle;
    u_int8_t num_per_nfdbr_dbm;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_nfcal_power_ev_param(wmi_handle, data, ev)) {
        return -1;
    }

    if ((ev->num_freq == 0) || (ev->num_freq > WMI_HOST_RXG_CAL_CHAN_MAX) ||
        (ev->num_nfdbr_dbm > WMI_HOST_MAX_NUM_CHAINS * WMI_HOST_RXG_CAL_CHAN_MAX)) {
        return -1;
    }

    num_per_nfdbr_dbm = ev->num_nfdbr_dbm / ev->num_freq;
    qdf_info("nfdBr\tnfdBm num_freq %d num_nfdbr_dbm %d\n", ev->num_freq, num_per_nfdbr_dbm);
    for(j = 0; j < ev->num_freq; j++)
    {
        qdf_info("Freq=%d \n", ev->freqnum[j]);
        for(i = 0; i < num_per_nfdbr_dbm; i++) {
            qdf_info("%d\t%d\n",
                    (int8_t)ev->nfdbr[j*num_per_nfdbr_dbm+i],
                    (int8_t)ev->nfdbm[j*num_per_nfdbr_dbm+i]);
	}
    }
#if UMAC_SUPPORT_ACFG
    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, PDEV_UNIT(ev->pdev_id),
                                   WLAN_MLME_SB_ID);
    if (!pdev) {
         qdf_err("pdev object (id: %d) is NULL", PDEV_UNIT(ev->pdev_id));
         return -1;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
         qdf_info("scn(id: %d) is NULL", PDEV_UNIT(ev->pdev_id));
         wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
         return -1;
    }

    if (!osif_scn_event_filter_status_to_app(&scn->sc_ic, EVENT_ACFG,
                WL_EVENT_TYPE_NF_DBR_DBM_INFO)) {
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
        return 0;
    }
    acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
    if (!acfg_event) {
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
        return 0;
    }

    memcpy(acfg_event->nf_dbr_dbm.nfdbr, ev->nfdbr, sizeof((acfg_event->nf_dbr_dbm.nfdbr)));
    memcpy(acfg_event->nf_dbr_dbm.nfdbm, ev->nfdbm, sizeof(acfg_event->nf_dbr_dbm.nfdbm));
    memcpy(acfg_event->nf_dbr_dbm.freqNum, ev->freqnum, sizeof(acfg_event->nf_dbr_dbm.freqNum));
    acfg_send_event(scn->netdev, scn->sc_osdev, WL_EVENT_TYPE_NF_DBR_DBM_INFO, acfg_event);
    qdf_mem_free(acfg_event);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
#endif

    return 0;
}

#undef MAX_NUM_CHAINS

int
ol_ath_packet_power_info_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    wmi_host_pdev_tpc_event event, *ev = &event;
#if UMAC_SUPPORT_ACFG
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_pdev *pdev;
    acfg_event_data_t *acfg_event = NULL;
#endif
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_pdev_tpc_ev_param(wmi_handle, data, ev)) {
        return -1;
    }
    qdf_info("Max packet power = %d, Min pkt power: %d",
            ev->tpc[WMI_HOST_TX_POWER_MAX], ev->tpc[WMI_HOST_TX_POWER_MIN]);
#if UMAC_SUPPORT_ACFG
    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, PDEV_UNIT(ev->pdev_id),
                                   WLAN_MLME_SB_ID);
    if (!pdev) {
         qdf_err("pdev object (id: %d) is NULL", PDEV_UNIT(ev->pdev_id));
         return -1;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
         qdf_err("scn(id: %d) is NULL", PDEV_UNIT(ev->pdev_id));
         wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
         return -1;
    }

    if (!osif_scn_event_filter_status_to_app(&scn->sc_ic, EVENT_ACFG,
                WL_EVENT_TYPE_PACKET_POWER_INFO)) {
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
        return 0;
    }
    acfg_event = (acfg_event_data_t *)qdf_mem_malloc_atomic(sizeof(acfg_event_data_t));
    if (!acfg_event) {
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
        return 0;
    }

    memcpy(&(acfg_event->packet_power), ev->tpc, sizeof(ev->tpc));
    acfg_send_event(scn->netdev, scn->sc_osdev, WL_EVENT_TYPE_PACKET_POWER_INFO, acfg_event);
    qdf_mem_free(acfg_event);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
#endif
    return 0;
}

int
ol_ath_generic_buffer_event_handler (ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    wmi_host_pdev_generic_buffer_event event, *ev = &event;
#ifdef BIG_ENDIAN_HOST
    u_int8_t i;
#endif
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_pdev_generic_buffer_ev_param(wmi_handle, data, ev)) {
        return -1;
    }
    qdf_info("Generic buffer received: type=%d len=%d frag_id=%d, more_frag=%d",
            ev->buf_type, ev->buf_len, ev->frag_id, ev->more_frag);

#ifdef BIG_ENDIAN_HOST
    {
        u_int32_t *destp, *srcp;
        u_int32_t len;

        srcp = &ev->buf_info[0];
        destp = &ev->buf_info[0];
        len = ev->buf_len;
        for(i=0; i < (roundup(len, sizeof(u_int32_t))/4); i++) {
            *destp = le32_to_cpu(*srcp);
            destp++; srcp++;
        }
    }
#endif

    if (ev->buf_type == WMI_HOST_BUFFER_TYPE_RATEPWR_TABLE) {
        ol_if_ratepwr_recv_buf((u_int8_t *)(&ev->buf_info[0]), ev->buf_len,
                                ev->frag_id, ev->more_frag);
    } else if (ev->buf_type == WMI_HOST_BUFFER_TYPE_CTL_TABLE) {
        /* TODO */
    }
    return 0;
}

int ol_ath_peer_mumimo_tx_count_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    wmi_host_peer_txmu_cnt_event event, *ev = &event;
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_mumimo_tx_count_ev_param(wmi_handle, data, ev)) {
        return -1;
    }
    qdf_info("MUMIMO tx for this peer %u",ev->tx_mu_transmitted);
    return 0;
}

int ol_ath_peer_gid_userpos_list_event_handler(ol_soc_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    wmi_host_peer_gid_userpos_list_event event, *ev = &event;
    struct wmi_unified *wmi_handle;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    if (wmi_extract_peer_gid_userpos_list_ev_param(wmi_handle, data, ev)) {
        return -1;
    }
   qdf_info("User poistion list for GID 1->15:[%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u]",
       ev->usr_list[0], ev->usr_list[1], ev->usr_list[2], ev->usr_list[3], ev->usr_list[4], ev->usr_list[5],
       ev->usr_list[6], ev->usr_list[7], ev->usr_list[8], ev->usr_list[9], ev->usr_list[10],ev->usr_list[11],
       ev->usr_list[12], ev->usr_list[13], ev->usr_list[14]);
   return 0;
}

u_int8_t ol_scn_vow_extstats(ol_pdev_handle pdev)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *) pdev;
    return scn->vow_extstats;
}
qdf_export_symbol(ol_scn_vow_extstats);

u_int32_t ol_scn_vow_get_rxstats(ol_pdev_handle pdev, u_int32_t *phy_err_count, u_int32_t *rx_clear_count, u_int32_t *rx_cycle_count)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)pdev;
    *phy_err_count = scn->chan_stats.phy_err_cnt;
    *rx_clear_count = scn->mib_cycle_cnts.rx_clear_count;
    *rx_cycle_count = scn->mib_cycle_cnts.cycle_count;
    return 0;
}
qdf_export_symbol(ol_scn_vow_get_rxstats);

uint32_t
ol_get_phymode_info(struct ol_ath_softc_net80211 *scn, uint32_t chan_mode,
                    bool is_2gvht_en) {
    /* We define a non-const modeflags array in order to enable stubbing based
     * simulation. Such simulation would involve an override of the host mode to
     * FW mode mapping.
     */
    static uint32_t modeflags[] = {
        0,                            /* IEEE80211_MODE_AUTO            */
        WMI_HOST_MODE_11A,            /* IEEE80211_MODE_11A             */
        WMI_HOST_MODE_11B,            /* IEEE80211_MODE_11B             */
        WMI_HOST_MODE_11G,            /* IEEE80211_MODE_11G             */
        0,                            /* IEEE80211_MODE_FH              */
        0,                            /* IEEE80211_MODE_TURBO_A         */
        0,                            /* IEEE80211_MODE_TURBO_G         */
        WMI_HOST_MODE_11NA_HT20,      /* IEEE80211_MODE_11NA_HT20       */
        WMI_HOST_MODE_11NG_HT20,      /* IEEE80211_MODE_11NG_HT20       */
        WMI_HOST_MODE_11NA_HT40,      /* IEEE80211_MODE_11NA_HT40PLUS   */
        WMI_HOST_MODE_11NA_HT40,      /* IEEE80211_MODE_11NA_HT40MINUS  */
        WMI_HOST_MODE_11NG_HT40,      /* IEEE80211_MODE_11NG_HT40PLUS   */
        WMI_HOST_MODE_11NG_HT40,      /* IEEE80211_MODE_11NG_HT40MINUS  */
        WMI_HOST_MODE_11NG_HT40,      /* IEEE80211_MODE_11NG_HT40       */
        WMI_HOST_MODE_11NA_HT40,      /* IEEE80211_MODE_11NA_HT40       */
        WMI_HOST_MODE_11AC_VHT20,     /* IEEE80211_MODE_11AC_VHT20      */
        WMI_HOST_MODE_11AC_VHT40,     /* IEEE80211_MODE_11AC_VHT40PLUS  */
        WMI_HOST_MODE_11AC_VHT40,     /* IEEE80211_MODE_11AC_VHT40MINUS */
        WMI_HOST_MODE_11AC_VHT40,     /* IEEE80211_MODE_11AC_VHT40      */
        WMI_HOST_MODE_11AC_VHT80,     /* IEEE80211_MODE_11AC_VHT80      */
        WMI_HOST_MODE_11AC_VHT160,    /* IEEE80211_MODE_11AC_VHT160     */
        WMI_HOST_MODE_11AC_VHT80_80,  /* IEEE80211_MODE_11AC_VHT80_80   */
        WMI_HOST_MODE_11AX_HE20,      /* IEEE80211_MODE_11AXA_HE20      */
        WMI_HOST_MODE_11AX_HE20_2G,   /* IEEE80211_MODE_11AXG_HE20      */
        WMI_HOST_MODE_11AX_HE40,      /* IEEE80211_MODE_11AXA_HE40PLUS  */
        WMI_HOST_MODE_11AX_HE40,      /* IEEE80211_MODE_11AXA_HE40MINUS */
        WMI_HOST_MODE_11AX_HE40_2G,   /* IEEE80211_MODE_11AXG_HE40PLUS  */
        WMI_HOST_MODE_11AX_HE40_2G,   /* IEEE80211_MODE_11AXG_HE40MINUS */
        WMI_HOST_MODE_11AX_HE40,      /* IEEE80211_MODE_11AXA_HE40      */
        WMI_HOST_MODE_11AX_HE40_2G,   /* IEEE80211_MODE_11AXG_HE40      */
        WMI_HOST_MODE_11AX_HE80,      /* IEEE80211_MODE_11AXA_HE80      */
        WMI_HOST_MODE_11AX_HE160,     /* IEEE80211_MODE_11AXA_HE160     */
        WMI_HOST_MODE_11AX_HE80_80,   /* IEEE80211_MODE_11AXA_HE80_80   */
#ifdef WLAN_FEATURE_11BE
        WMI_HOST_MODE_11BE_EHT20,      /* IEEE80211_MODE_11BEA_EHT20      */
        WMI_HOST_MODE_11BE_EHT20_2G,   /* IEEE80211_MODE_11BEG_EHT20      */
        WMI_HOST_MODE_11BE_EHT40,      /* IEEE80211_MODE_11BEA_EHT40PLUS  */
        WMI_HOST_MODE_11BE_EHT40,      /* IEEE80211_MODE_11BEA_EHT40MINUS */
        WMI_HOST_MODE_11BE_EHT40_2G,   /* IEEE80211_MODE_11BEG_EHT40PLUS  */
        WMI_HOST_MODE_11BE_EHT40_2G,   /* IEEE80211_MODE_11BEG_EHT40MINUS */
        WMI_HOST_MODE_11BE_EHT40,      /* IEEE80211_MODE_11BEA_EHT40      */
        WMI_HOST_MODE_11BE_EHT40_2G,   /* IEEE80211_MODE_11BEG_EHT40      */
        WMI_HOST_MODE_11BE_EHT80,      /* IEEE80211_MODE_11BEA_EHT80      */
        WMI_HOST_MODE_11BE_EHT160,     /* IEEE80211_MODE_11BEA_EHT160     */
        WMI_HOST_MODE_11BE_EHT320,     /* IEEE80211_MODE_11BEA_EHT320     */
#endif /* WLAN_FEATURE_11BE */
    };
    uint32_t temp_phymode;
    enum wmi_target_type wmi_tgt_type;

    if (!scn) {
        qdf_err("scn is null");
        return WMI_HOST_MODE_UNKNOWN;
    }

#if QCA_11AX_STUB_SUPPORT
    if (1 == OL_ATH_IS_11AX_STUB_ENABLED(scn->soc)) {
        /* We re-map 802.11ax modes to equivalent 802.11n/ac modes. */
        modeflags[IEEE80211_MODE_11AXA_HE20] = WMI_HOST_MODE_11AC_VHT20;
        modeflags[IEEE80211_MODE_11AXG_HE20] = WMI_HOST_MODE_11NG_HT20;
        modeflags[IEEE80211_MODE_11AXA_HE40PLUS] = WMI_HOST_MODE_11AC_VHT40;
        modeflags[IEEE80211_MODE_11AXA_HE40MINUS] = WMI_HOST_MODE_11AC_VHT40;
        modeflags[IEEE80211_MODE_11AXG_HE40PLUS] = WMI_HOST_MODE_11NG_HT40;
        modeflags[IEEE80211_MODE_11AXG_HE40MINUS] = WMI_HOST_MODE_11NG_HT40;
        modeflags[IEEE80211_MODE_11AXA_HE40] = WMI_HOST_MODE_11AC_VHT40;
        modeflags[IEEE80211_MODE_11AXG_HE40] = WMI_HOST_MODE_11NG_HT40;
        modeflags[IEEE80211_MODE_11AXA_HE80] = WMI_HOST_MODE_11AC_VHT80;
        modeflags[IEEE80211_MODE_11AXA_HE160] = WMI_HOST_MODE_11AC_VHT160;
        modeflags[IEEE80211_MODE_11AXA_HE80_80] = WMI_HOST_MODE_11AC_VHT80_80;
    }
#endif /* QCA_11AX_STUB_SUPPORT */

    /* Assert if the chan mode is invalid */
    qdf_assert_always(chan_mode < IEEE80211_MODE_MAX);

    temp_phymode = modeflags[chan_mode];
    if (ol_ath_get_wmi_target_type(scn->soc, &wmi_tgt_type) != QDF_STATUS_SUCCESS) {
        qdf_info("Not able to get wmi target type");
        return temp_phymode;
    }

    if ((wmi_tgt_type == WMI_TLV_TARGET) && (is_2gvht_en == true)) {
        switch(chan_mode) {
            case IEEE80211_MODE_11NG_HT20:
                temp_phymode = WMI_HOST_MODE_11AC_VHT20_2G;
                break;

            case IEEE80211_MODE_11NG_HT40PLUS:
            case IEEE80211_MODE_11NG_HT40MINUS:
            case IEEE80211_MODE_11NG_HT40:
                temp_phymode = WMI_HOST_MODE_11AC_VHT40_2G;
                break;
        }
    }
    return temp_phymode;
}

uint32_t
ol_get_phymode_info_from_wlan_phymode(struct ol_ath_softc_net80211 *scn,
        uint32_t chan_mode,
        bool is_2gvht_en) {

    static uint32_t conv_phy_mode_to_wmi_phy_mode[] = {
        WMI_HOST_MODE_UNKNOWN,        /* WLAN_PHYMODE_AUTO            */
        WMI_HOST_MODE_11A,            /* WLAN_PHYMODE_11A             */
        WMI_HOST_MODE_11B,            /* WLAN_PHYMODE_11B             */
        WMI_HOST_MODE_11G,            /* WLAN_PHYMODE_11G             */
        WMI_HOST_MODE_11GONLY,        /* WLAN_PHYMODE_11G_ONLY        */
        WMI_HOST_MODE_11NA_HT20,      /* WLAN_PHYMODE_11NA_HT20       */
        WMI_HOST_MODE_11NG_HT20,      /* WLAN_PHYMODE_11NG_HT20       */
        WMI_HOST_MODE_11NA_HT40,      /* WLAN_PHYMODE_11NA_HT40       */
        WMI_HOST_MODE_11NG_HT40,      /* WLAN_PHYMODE_11NG_HT40PLUS   */
        WMI_HOST_MODE_11NG_HT40,      /* WLAN_PHYMODE_11NG_HT40MINUS  */
        WMI_HOST_MODE_11NG_HT40,      /* WLAN_PHYMODE_11NG_HT40       */
        WMI_HOST_MODE_11AC_VHT20,     /* WLAN_PHYMODE_11AC_VHT20      */
        WMI_HOST_MODE_11AC_VHT20_2G,  /* WLAN_PHYMODE_11AC_VHT20_2G   */
        WMI_HOST_MODE_11AC_VHT40,     /* WLAN_PHYMODE_11AC_VHT40      */
        WMI_HOST_MODE_11AC_VHT40_2G,  /* WLAN_PHYMODE_11AC_VHT40PLUS_2G  */
        WMI_HOST_MODE_11AC_VHT40_2G,  /* WLAN_PHYMODE_11AC_VHT40MINUS_2G */
        WMI_HOST_MODE_11AC_VHT40_2G,  /* WLAN_PHYMODE_11AC_VHT40_2G      */
        WMI_HOST_MODE_11AC_VHT80,     /* WLAN_PHYMODE_11AC_VHT80      */
        WMI_HOST_MODE_11AC_VHT80_2G,  /* WLAN_PHYMODE_11AC_VHT80_2G   */
        WMI_HOST_MODE_11AC_VHT160,    /* WLAN_PHYMODE_11AC_VHT160     */
        WMI_HOST_MODE_11AC_VHT80_80,  /* WLAN_PHYMODE_11AC_VHT80_80   */
        WMI_HOST_MODE_11AX_HE20,      /* WLAN_PHYMODE_11AXA_HE20      */
        WMI_HOST_MODE_11AX_HE20_2G,   /* WLAN_PHYMODE_11AXG_HE20      */
        WMI_HOST_MODE_11AX_HE40,      /* WLAN_PHYMODE_11AXA_HE40      */
        WMI_HOST_MODE_11AX_HE40_2G,   /* WLAN_PHYMODE_11AXG_HE40PLUS  */
        WMI_HOST_MODE_11AX_HE40_2G,   /* WLAN_PHYMODE_11AXG_HE40MINUS */
        WMI_HOST_MODE_11AX_HE40_2G,   /* WLAN_PHYMODE_11AXG_HE40      */
        WMI_HOST_MODE_11AX_HE80,      /* WLAN_PHYMODE_11AXA_HE80      */
        WMI_HOST_MODE_11AX_HE80,      /* WLAN_PHYMODE_11AXG_HE80      */
        WMI_HOST_MODE_11AX_HE160,     /* WLAN_PHYMODE_11AXA_HE160     */
        WMI_HOST_MODE_11AX_HE80_80,   /* WLAN_PHYMODE_11AXA_HE80_80   */
#ifdef WLAN_FEATURE_11BE
        WMI_HOST_MODE_11BE_EHT20,     /* WLAN_PHYMODE_11BEA_EHT20      */
        WMI_HOST_MODE_11BE_EHT20_2G,  /* WLAN_PHYMODE_11BEG_EHT20      */
        WMI_HOST_MODE_11BE_EHT40,     /* WLAN_PHYMODE_11BEA_EHT40      */
        WMI_HOST_MODE_11BE_EHT40_2G,  /* WLAN_PHYMODE_11BEG_EHT40PLUS  */
        WMI_HOST_MODE_11BE_EHT40_2G,  /* WLAN_PHYMODE_11BEG_EHT40MINUS */
        WMI_HOST_MODE_11BE_EHT40_2G,  /* WLAN_PHYMODE_11BEG_EHT40      */
        WMI_HOST_MODE_11BE_EHT80,     /* WLAN_PHYMODE_11BEA_EHT80      */
        WMI_HOST_MODE_11BE_EHT80,     /* WLAN_PHYMODE_11BEG_EHT80      */
        WMI_HOST_MODE_11BE_EHT160,    /* WLAN_PHYMODE_11BEA_EHT160     */
        WMI_HOST_MODE_11BE_EHT320,    /* WLAN_PHYMODE_11BEA_EHT320     */
#endif /* WLAN_FEATURE_11BE */
    };
    uint32_t temp_phymode;
    enum wmi_target_type wmi_tgt_type;

    if (scn == NULL)
        return WMI_HOST_MODE_UNKNOWN;

#if QCA_11AX_STUB_SUPPORT
    if (1 == OL_ATH_IS_11AX_STUB_ENABLED(scn->soc)) {
        /* We re-map 802.11ax modes to equivalent 802.11n/ac modes. */
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXA_HE20] = WMI_HOST_MODE_11AC_VHT20;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXG_HE20] = WMI_HOST_MODE_11NG_HT20;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXG_HE40PLUS] = WMI_HOST_MODE_11NG_HT40;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXG_HE40MINUS] = WMI_HOST_MODE_11NG_HT40;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXA_HE40] = WMI_HOST_MODE_11AC_VHT40;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXG_HE40] = WMI_HOST_MODE_11NG_HT40;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXA_HE80] = WMI_HOST_MODE_11AC_VHT80;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXA_HE160] = WMI_HOST_MODE_11AC_VHT160;
        conv_phy_mode_to_wmi_phy_mode[WLAN_PHYMODE_11AXA_HE80_80] = WMI_HOST_MODE_11AC_VHT80_80;
    }
#endif /* QCA_11AX_STUB_SUPPORT */

    if(chan_mode < WLAN_PHYMODE_11A || chan_mode >= WLAN_PHYMODE_MAX) {
        temp_phymode = WMI_HOST_MODE_UNKNOWN;
        return temp_phymode;
    }

    if (ol_ath_get_wmi_target_type(scn->soc, &wmi_tgt_type) != QDF_STATUS_SUCCESS) {
        qdf_print("Not able to get wmi target type");
        temp_phymode = WMI_HOST_MODE_UNKNOWN;
        return temp_phymode;
    }
    temp_phymode = conv_phy_mode_to_wmi_phy_mode[chan_mode];

    if ((wmi_tgt_type == WMI_TLV_TARGET) && (is_2gvht_en == true)) {
        switch(chan_mode) {
            case WLAN_PHYMODE_11NG_HT20:
                temp_phymode = WMI_HOST_MODE_11AC_VHT20_2G;
                break;

            case WLAN_PHYMODE_11NG_HT40PLUS:
            case WLAN_PHYMODE_11NG_HT40MINUS:
            case WLAN_PHYMODE_11NG_HT40:
                temp_phymode = WMI_HOST_MODE_11AC_VHT40_2G;
                break;
        }
    }
    return temp_phymode;
}

/**
 * ol_ath_process_tx_data_frames() - Callback registered for WDI_EVENT_TX_DATA
 * @pdev_hdl: pdev pointer
 * @event:   WDi event
 * @data:    skb received
 * @peer_id: peer_id
 * @status:  status from event
 *
 * The MSDU payload is appended with a tx capture identifier containing ppdu_id,
 * tx and rx address and is sent to this function.
 *
 * @Return: None
 */
static void ol_ath_process_tx_data_frames(void *pdev_hdl, enum WDI_EVENT event, void *data,
                                u_int16_t peer_id, uint32_t status)
{
    qdf_nbuf_t skb = (qdf_nbuf_t)data;
    struct wlan_objmgr_pdev *pdev_obj = (struct wlan_objmgr_pdev *) pdev_hdl;
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);
    struct ieee80211vap *vap;
    osif_dev  *osifp;
    char *buf = NULL;

    if (!ic) {
      qdf_nbuf_free(skb);
      qdf_debug("ic is NULL");
      return;
    }

    vap = ic->ic_mon_vap;
    if (!vap) {
      qdf_nbuf_free(skb);
      QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,"%s: No monitor vap created, to dump skb\n",__func__);
      return;
    }

    /**
     * We are currently forwarding the buffer to user space or freeing the
     * buffer based on the feature.
     * Have callback here to extract the data if required.
     */
    if (!vap->iv_lite_monitor && (ol_ath_is_mcopy_enabled(ic))) {
            qdf_nbuf_free(skb);
            return;
    }

    /* Free buffer if only tx_lat_capture is enabled */
    if (qdf_likely(((ic->ic_debug_sniffer != SNIFFER_M_COPY_MODE) &&
                   (ic->ic_debug_sniffer != SNIFFER_TX_CAPTURE_MODE))
                   && ic->ic_capture_latency)) {
        qdf_nbuf_free(skb);
        return;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    if ((buf = (char *)qdf_nbuf_put_tail(skb, DEBUG_SNIFFER_SIGNATURE_LEN))) {
        qdf_mem_copy(buf, DEBUG_SNIFFER_TEST_TX_DATA, DEBUG_SNIFFER_SIGNATURE_LEN);
    }

    /* Ensure to free the skb here if not being sent to upper layer */
    osif_deliver_tx_capture_data(osifp, skb);
}

/**
 * ol_ath_process_rx_data_frames() - Callback registered for WDI_EVENT_RX_DATA
 * @pdev_hdl: pdev pointer
 * @event:   WDi event
 * @data:    skb received
 * @peer_id: peer_id
 * @status:  status from event
 *
 * The MSDU payload is appended with a rx capture identifier containing 16bits ppdu_id,
 * and reserved field and is sent to this function.
 *
 * @Return: None
 */
static void ol_ath_process_rx_data_frames(void *pdev_hdl, enum WDI_EVENT event, void *data,
                                u_int16_t peer_id, uint32_t status)
{
    qdf_nbuf_t skb = (qdf_nbuf_t)data;
    struct wlan_objmgr_pdev *pdev_obj = (struct wlan_objmgr_pdev *) pdev_hdl;
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);
    struct ieee80211vap *vap;
    osif_dev  *osifp;
    char *buf = NULL;

    if (!ic) {
      qdf_nbuf_free(skb);
      qdf_debug("ic is NULL");
      return;
    }

    vap = ic->ic_mon_vap;

    if (!vap) {
      qdf_nbuf_free(skb);
      QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,"%s: No monitor vap created, to dump skb\n",__func__);
      return;
    }

    /**
     * We are currently forwarding the buffer to user space or freeing the
     * buffer based on the feature.
     * Have callback here to extract the data if required.
     */
    if (!vap->iv_lite_monitor && (ol_ath_is_mcopy_enabled(ic))) {
            qdf_nbuf_free(skb);
            return;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    if ((buf = (char *)qdf_nbuf_put_tail(skb, DEBUG_SNIFFER_SIGNATURE_LEN))) {
        qdf_mem_copy(buf, DEBUG_SNIFFER_TEST_RX_DATA, DEBUG_SNIFFER_SIGNATURE_LEN);
    }

    /* Ensure to free the skb here if not being sent to upper layer */
    osif_deliver_tx_capture_data(osifp, skb);
}

/**
 * ol_ath_process_tx_metadata() - Callback registered for WDI_EVENT_TX_PPDU_DESC
 * @pdev_hdl: pdev pointer
 * @event:   WDi event
 * @data:    skb received
 * @peer_id: peer_id
 * @status:  status from event
 *
 * @Return: None
 */
void ol_ath_process_tx_metadata(struct ieee80211com *ic, void *data)
{
    qdf_nbuf_t skb = (qdf_nbuf_t)data;
    struct ieee80211vap *vap = ic->ic_mon_vap;
    osif_dev  *osifp = NULL;
    char *buf = NULL;

    if (!vap) {
      qdf_nbuf_free(skb);
      QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,"%s: No monitor vap created, to dump skb\n",__func__);
      return;
    }

    /**
     * We are currently forwarding the buffer to user space or freeing the
     * buffer based on the feature.
     * Have callback here to extract the data if required.
     */
    if (!vap->iv_lite_monitor && (ol_ath_is_mcopy_enabled(ic))) {
            qdf_nbuf_free(skb);
            return;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    if ((buf = (char *)qdf_nbuf_put_tail(skb, DEBUG_SNIFFER_SIGNATURE_LEN))) {
        qdf_mem_copy(buf, DEBUG_SNIFFER_TEST_TX_METADATA, DEBUG_SNIFFER_SIGNATURE_LEN);
    }

    /* Ensure to free the skb here if not being sent to upper layer */
    osif_deliver_tx_capture_data(osifp, skb);
}

/**
 * ol_ath_process_rx_metadata() - Callback registered for WDI_EVENT_RX_PPDU_DESC
 * @pdev_hdl: pdev pointer
 * @event:   WDi event
 * @data:    skb received
 * @peer_id: peer_id
 * @status:  status from event
 *
 * @Return: None
 */
void ol_ath_process_rx_metadata(struct ieee80211com *ic, void *data)
{
    qdf_nbuf_t skb = (qdf_nbuf_t)data;
    struct ieee80211vap *vap = ic->ic_mon_vap;
    osif_dev  *osifp = NULL;
    char *buf = NULL;

    if (!vap) {
      qdf_nbuf_free(skb);
      QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,"%s: No monitor vap created, to dump skb\n",__func__);
      return;
    }

    /**
     * We are currently forwarding the buffer to user space or freeing the
     * buffer based on the feature.
     * Have callback here to extract the data if required.
     */
    if (!vap->iv_lite_monitor && (ol_ath_is_mcopy_enabled(ic))) {
            qdf_nbuf_free(skb);
            return;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    if ((buf = (char *)qdf_nbuf_put_tail(skb, DEBUG_SNIFFER_SIGNATURE_LEN))) {
        qdf_mem_copy(buf, DEBUG_SNIFFER_TEST_RX_METADATA, DEBUG_SNIFFER_SIGNATURE_LEN);
    }

    /* Ensure to free the skb here if not being sent to upper layer */
    osif_deliver_tx_capture_data(osifp, skb);
}

/**
 * ol_ath_process_rx_nondata_frames() - Callback registered for recvd. mgmt. pkt processing
 * @pdev_hdl: pdev pointer
 * @event:   WDI event
 * @data:    skb received
 * @peer_id: peer_id
 * @status:  status from event
 *
 * @Return: None
 */
static void ol_ath_process_rx_nondata_frames(void *pdev_hdl, enum WDI_EVENT event, void *data,
                                u_int16_t peer_id, uint32_t status)
{
    qdf_nbuf_t skb = (qdf_nbuf_t)data;
    struct wlan_objmgr_pdev *pdev_obj = (struct wlan_objmgr_pdev *) pdev_hdl;
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);
    struct ieee80211vap *vap;
    osif_dev  *osifp = NULL;
#ifdef DP_RX_MON_MEM_FRAG
    uint32_t num_frags;
    unsigned int frag_size;
    qdf_nbuf_t last_nbuf, ext_list;
#endif
    qdf_frag_t buf_addr;

    if (!ic) {
      qdf_nbuf_free(skb);
      qdf_debug("ic is NULL");
      return;
    }

    vap = ic->ic_mon_vap;

    if (!vap) {
      qdf_nbuf_free(skb);
      QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,"%s: No monitor vap created, to dump skb\n",__func__);
      return;
    }

    /**
     * We are currently forwarding the buffer to user space or freeing the
     * buffer based on the feature.
     * Have callback here to extract the data if required.
     */
#ifndef DP_RX_MON_MEM_FRAG
    if (qdf_nbuf_is_nonlinear(skb)) {
        qdf_nbuf_free(skb);
        return;
    }
#endif

    if ((!vap->iv_lite_monitor && (ol_ath_is_mcopy_enabled(ic)))) {
            qdf_nbuf_free(skb);
            return;
    }

    osifp = (osif_dev *)vap->iv_ifp;

    /**
     * IF feature is enabled then add string to the last frag of last skb.
     * Last DEBUG_SNIFFER_SIGNATURE_LEN bytes data from last frag of last
     * skb will be overwritten.
     */
#ifdef DP_RX_MON_MEM_FRAG
    ext_list = qdf_nbuf_get_ext_list(skb);
    last_nbuf = skb;
    while (ext_list) {
        last_nbuf = ext_list;
        ext_list = qdf_nbuf_queue_next(ext_list);
    }

    num_frags = qdf_nbuf_get_nr_frags(last_nbuf);
    if (!num_frags) {
        qdf_nbuf_free(skb);
        return;
    }
    buf_addr = qdf_nbuf_get_frag_addr(last_nbuf, num_frags - 1 );
    if (!buf_addr) {
        qdf_nbuf_free(skb);
        QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,
               "%s: SKB FRAG with address NULL is not expected \n",
               __func__);
        return;
    }
    frag_size = qdf_nbuf_get_frag_size_by_idx(last_nbuf, num_frags - 1);

    /**
     * Overwrite last 6 bytes of data with signature string.
     * If length of string is < 6, then write singature from initial
     * boundry.
     */
    if (frag_size >= DEBUG_SNIFFER_SIGNATURE_LEN)
        buf_addr = (char *)buf_addr + frag_size - DEBUG_SNIFFER_SIGNATURE_LEN;
#else
    /**
     * IF feature is disabled then add string to the tail room of skb.
     * In this case, we will have space available in tailroom and hence
     * last DEBUG_SNIFFER_SIGNATURE_LEN bytes string signature will be
     * appended and not overwritten.
     */
    buf_addr = (char *)qdf_nbuf_put_tail(skb, DEBUG_SNIFFER_SIGNATURE_LEN);
#endif

    if (buf_addr)
        qdf_mem_copy(buf_addr, DEBUG_SNIFFER_TEST_RX_MGMT, DEBUG_SNIFFER_SIGNATURE_LEN);

    /* Ensure to free the skb here if not being sent to upper layer */
    osif_deliver_lite_mon_data(osifp, skb);
}

/**
 * ol_ath_process_tx_nondata_frames() - Callback registered for WDI_EVENT_TX_MGMT_CTRL
 * @pdev_hdl: pdev pointer
 * @event:   WDI event
 * @data:    skb received
 * @peer_id: peer_id
 * @status:  status from event
 *
 * @Return: None
 */
static void ol_ath_process_tx_nondata_frames(void *pdev_hdl, enum WDI_EVENT event, void *data,
                                u_int16_t peer_id, uint32_t status)
{
    qdf_nbuf_t skb = (qdf_nbuf_t)data;
    struct wlan_objmgr_pdev *pdev_obj = (struct wlan_objmgr_pdev *) pdev_hdl;
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);
    struct ieee80211vap *vap;
    osif_dev  *osifp = NULL;
    char *buf = NULL;

    if (!ic) {
      qdf_nbuf_free(skb);
      qdf_debug("ic is NULL");
      return;
    }

    vap = ic->ic_mon_vap;
    if (!vap) {
      qdf_nbuf_free(skb);
      QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_DEBUG,"%s: No monitor vap created, to dump skb\n",__func__);
      return;
    }

    /**
     * We are currently forwarding the buffer to user space or freeing the
     * buffer based on the feature.
     * Have callback here to extract the data if required.
     */
    if (!vap->iv_lite_monitor && (ol_ath_is_mcopy_enabled(ic))) {
            qdf_nbuf_free(skb);
            return;
    }
    osifp = (osif_dev *)vap->iv_ifp;

    if ((buf = (char *)qdf_nbuf_put_tail(skb, DEBUG_SNIFFER_SIGNATURE_LEN))) {
        qdf_mem_copy(buf, DEBUG_SNIFFER_TEST_TX_MGMT, DEBUG_SNIFFER_SIGNATURE_LEN);
    }

    /* Ensure to free the skb here if not being sent to upper layer */
    osif_deliver_tx_capture_data(osifp, skb);
}

static void ol_ath_process_tx_nondata_frames_wrapper(struct ieee80211com *ic, qdf_nbuf_t payload)
{
    struct wlan_objmgr_pdev *pdev_obj = (struct wlan_objmgr_pdev *) ic->ic_pdev_obj;
    ol_ath_process_tx_nondata_frames(pdev_obj, 0, (void *)payload, 0, 0);
}

void ol_ath_subscribe_ppdu_desc_info(struct ol_ath_softc_net80211 *scn, uint8_t context)
{
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    struct ieee80211com *ic = &scn->sc_ic;

    if ((context == PPDU_DESC_DEBUG_SNIFFER) ||
            (context == PPDU_DESC_SMART_ANTENNA) ||
            (context == PPDU_DESC_ATF_STATS) ||
            ((context == PPDU_DESC_ENHANCED_STATS))) {
        if (qdf_atomic_read(&scn->tx_metadata_ref) == 0) {
            scn->stats_tx_subscriber.callback = ol_ath_process_ppdu_stats;
            scn->stats_tx_subscriber.context = scn->sc_pdev;
            if (cdp_wdi_event_sub(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                    &scn->stats_tx_subscriber, WDI_EVENT_TX_PPDU_DESC)) {
                return;
            }
        }
        qdf_atomic_inc(&scn->tx_metadata_ref);
    }

    if ((context == PPDU_DESC_ENHANCED_STATS) ||
        (context == PPDU_DESC_SMART_ANTENNA) ||
        (context == PPDU_DESC_ATF_STATS) ||
        (context == PPDU_DESC_CFR_RCC) ||
        (ol_ath_is_mcopy_enabled(ic))) {
        if (qdf_atomic_read(&scn->rx_metadata_ref) == 0) {
            scn->stats_rx_subscriber.callback = ol_ath_process_ppdu_stats;
            scn->stats_rx_subscriber.context = scn->sc_pdev;
            if (cdp_wdi_event_sub(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                    &scn->stats_rx_subscriber, WDI_EVENT_RX_PPDU_DESC)) {
                return;
            }
        }
        qdf_atomic_inc(&scn->rx_metadata_ref);
    }
}

void ol_ath_unsubscribe_ppdu_desc_info(struct ol_ath_softc_net80211 *scn, uint8_t context)
{
    struct ieee80211com *ic = &scn->sc_ic;
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    if ((context == PPDU_DESC_DEBUG_SNIFFER) ||
         (context == PPDU_DESC_SMART_ANTENNA) ||
         (context == PPDU_DESC_ATF_STATS) ||
         ((context == PPDU_DESC_ENHANCED_STATS) )) {
        if (qdf_atomic_dec_and_test(&scn->tx_metadata_ref)) {
            if (cdp_wdi_event_unsub(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                                &scn->stats_tx_subscriber, WDI_EVENT_TX_PPDU_DESC))
                return;
        }
    }

    if ((context == PPDU_DESC_ENHANCED_STATS) ||
        (context == PPDU_DESC_SMART_ANTENNA) ||
        (context == PPDU_DESC_ATF_STATS) ||
        (context == PPDU_DESC_CFR_RCC) ||
        (ol_ath_is_mcopy_enabled(ic))) {
        if (qdf_atomic_dec_and_test(&scn->rx_metadata_ref)) {
                if (cdp_wdi_event_unsub(soc_txrx_handle,
                        wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                        &scn->stats_rx_subscriber,
                        WDI_EVENT_RX_PPDU_DESC))
                    return;
        }
    }
}

int ol_ath_subscribe_tx_data_frames(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    scn->stats_tx_data_subscriber.callback = ol_ath_process_tx_data_frames;
    scn->stats_tx_data_subscriber.context = scn->sc_pdev;

    if (qdf_atomic_read(&scn->tx_data_frame_ref) == 0) {
        if (cdp_wdi_event_sub(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
            &scn->stats_tx_data_subscriber, WDI_EVENT_TX_DATA)) {
            return A_ERROR;
        }
    }
    qdf_atomic_inc(&scn->tx_data_frame_ref);
    return A_OK;
}

int ol_ath_unsubscribe_tx_data_frames(struct ol_ath_softc_net80211 *scn)
{
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    if (!qdf_atomic_read(&scn->tx_data_frame_ref))
        return A_OK;

    if (qdf_atomic_dec_and_test(&scn->tx_data_frame_ref)) {
        if (cdp_wdi_event_unsub(soc_txrx_handle,
                                wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                                &scn->stats_tx_data_subscriber, WDI_EVENT_TX_DATA))
            return A_ERROR;
    }
    return A_OK;
}

int ol_ath_set_capture_latency(struct ol_ath_softc_net80211 *scn, int val)
{
    struct ieee80211com *ic = &scn->sc_ic;
    ol_txrx_soc_handle soc_txrx_handle;
    cdp_config_param_type value = {0};

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    if (val) {
        if (ic->ic_capture_latency) {
            qdf_info("Capture_latency already enabled. Disable it first");
            return A_ERROR;
        }

        /* register subscriber */
        if (ol_ath_subscribe_tx_data_frames(scn)) {
            qdf_err("Subscription for tx_lat_capture failed.");
            return A_ERROR;
        }

        value.cdp_pdev_param_cptr_latcy = val;
        cdp_txrx_set_pdev_param(soc_txrx_handle,
                                wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                                CDP_CONFIG_CAPTURE_LATENCY, value);
        ic->ic_capture_latency = val;
    } else {
        if (!ic->ic_capture_latency) {
            qdf_info("Capture_latency already disabled. Enable it first");
            return A_ERROR;
        }

        ic->ic_capture_latency = val;

        /* Unregister Subscriber */
        if (ol_ath_unsubscribe_tx_data_frames(scn)) {
            qdf_err("Unsubscription for tx_lat_capture failed.");
            /* Resetting tx_lat_capture as unsubscription failed */
            ic->ic_capture_latency = 1;
            return A_ERROR;
        }
        value.cdp_pdev_param_cptr_latcy = 0;
        cdp_txrx_set_pdev_param(soc_txrx_handle,
                                wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                                CDP_CONFIG_CAPTURE_LATENCY, value);
    }

    return 0;
}
#ifdef QCA_SUPPORT_LITE_MONITOR
int ol_ath_set_debug_sniffer_rx_mon_2_0(struct ol_ath_softc_net80211 *scn,  int val){
    ol_txrx_soc_handle soc_txrx_handle;
    struct lite_mon_config *mon_config = NULL;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    mon_config = qdf_mem_malloc(sizeof(struct lite_mon_config));
    if (!mon_config){
        qdf_err("Memory allocation failed");
        return A_NO_MEMORY;
    }

    if (cdp_is_lite_mon_enabled(
                    soc_txrx_handle,
                    pdev_id,
                    LITE_MON_DIRECTION_RX)) {
        qdf_err("Lite monitor filter is already enabled, disable it first");
        qdf_mem_free(mon_config);
        mon_config = NULL;
        return A_ERROR;
    }

    mon_config->direction = LITE_MON_DIRECTION_RX;
    mon_config->cmdtype = LITE_MON_SET_FILTER;
    mon_config->debug = LITE_MON_TRACE_INFO;

    mon_config->data.filter_config.metadata = 0x0001;

    mon_config->data.filter_config.mgmt_filter[LITE_MON_MODE_FILTER_FP] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.data_filter[LITE_MON_MODE_FILTER_FP] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.ctrl_filter[LITE_MON_MODE_FILTER_FP] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.mgmt_filter[LITE_MON_MODE_FILTER_MO] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.data_filter[LITE_MON_MODE_FILTER_MO] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.ctrl_filter[LITE_MON_MODE_FILTER_MO] = LITE_MON_FILTER_ALL;

    mon_config->data.filter_config.len[LITE_MON_TYPE_DATA] = LITE_MON_LEN_2;
    mon_config->data.filter_config.len[LITE_MON_TYPE_MGMT] = LITE_MON_LEN_ALL;
    mon_config->data.filter_config.len[LITE_MON_TYPE_CTRL] = LITE_MON_LEN_ALL;

    mon_config->data.filter_config.legacy_filter_enabled = LEGACY_FILTER_MCOPY;

    if (val == SNIFFER_M_COPY_MODE)
        mon_config->data.filter_config.level = LITE_MON_LEVEL_PPDU;
    else if (val == SNIFFER_EXT_M_COPY_MODE)
        mon_config->data.filter_config.level = LITE_MON_LEVEL_MPDU;

    monitor_set_lite_monitor_config(scn, mon_config);

    qdf_mem_free(mon_config);
    mon_config = NULL;

    return A_OK;
}

int ol_ath_set_debug_sniffer_tx_mon_2_0(struct ol_ath_softc_net80211 *scn,  int val){
    ol_txrx_soc_handle soc_txrx_handle;
    struct lite_mon_config *mon_config = NULL;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    mon_config = qdf_mem_malloc(sizeof(struct lite_mon_config));
    if (!mon_config){
        qdf_err("Memory allocation failed");
        return A_NO_MEMORY;
    }

    if (cdp_is_lite_mon_enabled(
                    soc_txrx_handle,
                    pdev_id,
                    LITE_MON_DIRECTION_TX)) {
        qdf_err("Lite monitor filter is already enabled, disable it first");
        qdf_mem_free(mon_config);
        mon_config = NULL;
        return A_ERROR;
    }

    mon_config->direction = LITE_MON_DIRECTION_TX;
    mon_config->cmdtype = LITE_MON_SET_FILTER;
    mon_config->debug = LITE_MON_TRACE_INFO;

    mon_config->data.filter_config.mgmt_filter[LITE_MON_MODE_FILTER_FP] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.data_filter[LITE_MON_MODE_FILTER_FP] = LITE_MON_FILTER_ALL;
    mon_config->data.filter_config.ctrl_filter[LITE_MON_MODE_FILTER_FP] = LITE_MON_FILTER_ALL;

    mon_config->data.filter_config.len[LITE_MON_TYPE_DATA] = LITE_MON_LEN_2;
    mon_config->data.filter_config.len[LITE_MON_TYPE_MGMT] = LITE_MON_LEN_ALL;
    mon_config->data.filter_config.len[LITE_MON_TYPE_CTRL] = LITE_MON_LEN_ALL;

    if (val == SNIFFER_TX_CAPTURE_MODE)
        mon_config->data.filter_config.legacy_filter_enabled = LEGACY_FILTER_TX_CAPTURE;
    else
        mon_config->data.filter_config.legacy_filter_enabled = LEGACY_FILTER_MCOPY;

    if (val == SNIFFER_M_COPY_MODE)
        mon_config->data.filter_config.level = LITE_MON_LEVEL_PPDU;
    else if (val == SNIFFER_EXT_M_COPY_MODE)
        mon_config->data.filter_config.level = LITE_MON_LEVEL_MPDU;
    else{
        mon_config->data.filter_config.level = LITE_MON_LEVEL_MSDU;
        mon_config->data.filter_config.len[LITE_MON_TYPE_DATA] = LITE_MON_LEN_ALL;
    }

    monitor_set_lite_monitor_config(scn, mon_config);

    qdf_mem_free(mon_config);
    mon_config = NULL;

    return A_OK;
}

int ol_ath_disable_debug_sniffer_mon_2_0 (struct ol_ath_softc_net80211 *scn,
                                          uint8_t direction)
{
    struct lite_mon_config *mon_config = NULL;
    mon_config = qdf_mem_malloc(sizeof(struct lite_mon_config));

    if (!mon_config){
        qdf_err("Memory allocation failed");
        return A_NO_MEMORY;
    }

    mon_config->cmdtype = LITE_MON_SET_FILTER;
    mon_config->data.filter_config.disable = 1;
    mon_config->direction = direction;

    monitor_set_lite_monitor_config(scn, mon_config);

    qdf_mem_free(mon_config);
    mon_config = NULL;

    return A_OK;
}

int ol_ath_disable_debug_sniffer_rx_mon_2_0 (struct ol_ath_softc_net80211 *scn)
{
    return ol_ath_disable_debug_sniffer_mon_2_0(scn, LITE_MON_DIRECTION_RX);
}

int ol_ath_disable_debug_sniffer_tx_mon_2_0 (struct ol_ath_softc_net80211 *scn)
{
    return ol_ath_disable_debug_sniffer_mon_2_0(scn, LITE_MON_DIRECTION_TX);
}
#else
int ol_ath_set_debug_sniffer_rx_mon_2_0(struct ol_ath_softc_net80211 *scn,  int val){
    qdf_err("Couldn't send command. Lite mon is not supported");
    return A_ERROR;
}
int ol_ath_set_debug_sniffer_tx_mon_2_0(struct ol_ath_softc_net80211 *scn,  int val){
    qdf_err("Couldn't send command. Lite mon is not supported");
    return A_ERROR;
}
int ol_ath_disable_debug_sniffer_rx_mon_2_0 (struct ol_ath_softc_net80211 *scn){
    return A_ERROR;
}
int ol_ath_disable_debug_sniffer_tx_mon_2_0 (struct ol_ath_softc_net80211 *scn){
    return A_ERROR;
}
#endif
int ol_ath_set_debug_sniffer(struct ol_ath_softc_net80211 *scn,  int val)
{
    ol_txrx_soc_handle soc_txrx_handle;
    struct ieee80211com *ic = &scn->sc_ic;
    uint8_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
    cdp_config_param_type value = {0};
    QDF_STATUS status;

    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    if ((val == SNIFFER_TX_CAPTURE_MODE) || (val == SNIFFER_M_COPY_MODE) ||
        (val == SNIFFER_EXT_M_COPY_MODE)) {
	if (!ic->ic_debug_sniffer) {
            ic->ic_debug_sniffer = val;
	} else {
            qdf_info("Sniffer/AM copy mode already enabled. Disable it first");
            return A_ERROR;
	}

        ic->ic_process_nondata_tx_frames = ol_ath_process_tx_nondata_frames_wrapper;

        if (val == SNIFFER_M_COPY_MODE || val == SNIFFER_EXT_M_COPY_MODE) {
            if (ic->ic_monitor_version == MONITOR_VERSION_2){
                if (ol_ath_set_debug_sniffer_rx_mon_2_0(scn,val))
                    return A_ERROR;
                else{
                     scn->stats_rx_data_subscriber.callback = ol_ath_process_rx_data_frames;
                     scn->stats_rx_data_subscriber.context = scn->sc_pdev;

                     if (cdp_wdi_event_sub(soc_txrx_handle,
                                pdev_id,
                                &scn->stats_rx_data_subscriber,
                                WDI_EVENT_RX_DATA))
                         return A_ERROR;

                     scn->stats_rx_nondata_subscriber.callback = ol_ath_process_rx_nondata_frames;
                     scn->stats_rx_nondata_subscriber.context = scn->sc_pdev;
                     if (cdp_wdi_event_sub(soc_txrx_handle,
                                pdev_id,
                                &scn->stats_rx_nondata_subscriber,
                                WDI_EVENT_RX_MGMT_CTRL))
                         return A_ERROR;
                }
            }
	}

        ic->ic_tx_capture = 1;

        if (ic->ic_monitor_version == MONITOR_VERSION_2) {
            if(ol_ath_set_debug_sniffer_tx_mon_2_0(scn,val))
               return A_ERROR;
        }
        else {
             if (ol_ath_subscribe_tx_data_frames(scn))
                 return A_ERROR;

             scn->stats_nondata_subscriber.callback = ol_ath_process_tx_nondata_frames;
             scn->stats_nondata_subscriber.context = scn->sc_pdev;

             if (cdp_wdi_event_sub(soc_txrx_handle,
                              pdev_id,
                              &scn->stats_nondata_subscriber,
                              WDI_EVENT_TX_MGMT_CTRL))
                 return A_ERROR;

             ol_ath_subscribe_ppdu_desc_info(scn, PPDU_DESC_DEBUG_SNIFFER);

             value.cdp_pdev_param_dbg_snf = val;
             status = cdp_txrx_set_pdev_param(soc_txrx_handle,
                                         pdev_id,
                                         CDP_CONFIG_DEBUG_SNIFFER, value);
             if (status != QDF_STATUS_SUCCESS) {
                 ic->ic_debug_sniffer = SNIFFER_DISABLE;
                 ic->ic_tx_capture = 0;
                 if (val == SNIFFER_M_COPY_MODE || val == SNIFFER_EXT_M_COPY_MODE) {
                     cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                                         &scn->stats_rx_data_subscriber,
                                         WDI_EVENT_RX_DATA);
                     cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                                         &scn->stats_rx_nondata_subscriber,
                                         WDI_EVENT_RX_MGMT_CTRL);
                 }
                 ol_ath_unsubscribe_tx_data_frames(scn);
                 cdp_wdi_event_unsub(soc_txrx_handle, pdev_id,
                                     &scn->stats_nondata_subscriber,
                                     WDI_EVENT_TX_MGMT_CTRL);
                 ol_ath_unsubscribe_ppdu_desc_info(scn, PPDU_DESC_DEBUG_SNIFFER);

                 QDF_TRACE(QDF_MODULE_ID_DP, QDF_TRACE_LEVEL_ERROR,
                      "%s : %d : config debug sniffer failed", __func__, __LINE__);
                 return A_ERROR;
             }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
             if (scn->sc_ic.nss_radio_ops)
                 scn->sc_ic.nss_radio_ops->ic_nss_ol_wifi_tx_capture_set(scn, val);
#endif
        }
    } else if (val == SNIFFER_TX_MONITOR_MODE) {
	if (monitor_ol_ath_set_tx_sniffer_mode(scn, pdev_id, soc_txrx_handle) == A_ERROR)
                return A_ERROR;
    } else {
        /* Unregister Subscriber */
        ic->ic_tx_capture = 0;

        if (ic->ic_monitor_version == MONITOR_VERSION_2){
            if (ol_ath_disable_debug_sniffer_tx_mon_2_0(scn))
                return A_ERROR;
        }
        else{
             value.cdp_pdev_param_tx_capture = SNIFFER_DISABLE;
             cdp_txrx_set_pdev_param(soc_txrx_handle,
                                     pdev_id, CDP_CONFIG_TX_CAPTURE, value);

             if (ol_ath_unsubscribe_tx_data_frames(scn))
                 return A_ERROR;
        }

        if(ic->ic_debug_sniffer == 0) {
            qdf_err("Sniffer mode is disabled. Enable it first");
            return A_ERROR;
        }

        ic->ic_debug_sniffer = 0;
        ic->ic_process_nondata_tx_frames = NULL;

        if (ic->ic_monitor_version == MONITOR_VERSION_2){
            if (ol_ath_disable_debug_sniffer_rx_mon_2_0(scn))
                return A_ERROR;
        }
        else {
              value.cdp_pdev_param_dbg_snf = SNIFFER_DISABLE;
              status = cdp_txrx_set_pdev_param(soc_txrx_handle,
                                               pdev_id,
                                               CDP_CONFIG_DEBUG_SNIFFER, value);
              if (status != QDF_STATUS_SUCCESS)
                  return A_ERROR;

              if (cdp_wdi_event_unsub(soc_txrx_handle,
                               pdev_id,
                               &scn->stats_rx_data_subscriber,
                               WDI_EVENT_RX_DATA))
                  return A_ERROR;

              if (cdp_wdi_event_unsub(soc_txrx_handle,
                               pdev_id,
                               &scn->stats_nondata_subscriber,
                               WDI_EVENT_TX_MGMT_CTRL))
                  return A_ERROR;

              if (cdp_wdi_event_unsub(soc_txrx_handle,
                               pdev_id,
                               &scn->stats_rx_nondata_subscriber,
                               WDI_EVENT_RX_MGMT_CTRL))
                  return A_ERROR;

              ol_ath_unsubscribe_ppdu_desc_info(scn, PPDU_DESC_DEBUG_SNIFFER);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
              if (scn->sc_ic.nss_radio_ops)
                  scn->sc_ic.nss_radio_ops->ic_nss_ol_wifi_tx_capture_set(scn, SNIFFER_DISABLE);
#endif
        }
    }
    return 0;
}

int ol_ath_set_tx_capture (struct ol_ath_softc_net80211 *scn, int val) {
#ifndef REMOVE_PKT_LOG
    u_int32_t tx_capture;
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    uint32_t pdev_id = wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev);
    cdp_config_param_type value = {0};
    tx_capture = scn->pl_dev->pl_info->tx_capture_enabled;

#ifdef OL_ATH_SMART_LOGGING
    /* We use scn->pl_dev->tgt_pktlog_enabled inline with existing design for
     * this call. However, if access semantics for
     * scn->pl_dev->tgt_pktlog_enabled are changed for this call, then change
     * below accordingly.
     */
    if ((val == 1) && (!tx_capture)) {
        /* We intend to enable Tx capture. But if
         * scn->pl_dev->tgt_pktlog_enabled is true due to smart log FW initiated
         * packetlog, we have to get it out of the way.
         */
        if (scn->pl_dev->tgt_pktlog_enabled) {
            /* If smart log FW initiated packetlog is responsible for
             * scn->pl_dev->tgt_pktlog_enabled being true, then it will stop the
             * packetlog and block itself resulting in
             * scn->pl_dev->tgt_pktlog_enabled becoming false. These call
             * semantics are necessary for preserving locking requirements
             * within the smart log FW initiated packetlog module.
             *
             * We set the block_only_if_started argument to true so that if
             * scn->pl_dev->tgt_pktlog_enabled is true but smart log FW
             * initiated packetlog is not responsible, then the previous host
             * module caller's decision to not block FW initiated packetlog
             * remains in effect.
             */
            if (scn->pl_dev->pl_sl_funcs.smart_log_fw_pktlog_stop_and_block &&
                   (scn->pl_dev->pl_sl_funcs.smart_log_fw_pktlog_stop_and_block(
                        scn, ATH_PKTLOG_TX_CAPTURE_ENABLE, true) !=
                     QDF_STATUS_SUCCESS)) {
                return -1;
            }
        } else {
             /* Since we are the first to block smart log FW initiated
              * packetlog, we block it under all circumstances.
              */
             if (scn->pl_dev->pl_sl_funcs.smart_log_fw_pktlog_stop_and_block &&
                   (scn->pl_dev->pl_sl_funcs.smart_log_fw_pktlog_stop_and_block(
                        scn, ATH_PKTLOG_TX_CAPTURE_ENABLE, false) !=
                     QDF_STATUS_SUCCESS)) {
                return -1;
            }
        }
    }
#endif /* OL_ATH_SMART_LOGGING */

    if ((val == 1) && ((!scn->pl_dev->tgt_pktlog_enabled) && (!tx_capture))) {
           if (scn->pl_dev->pl_funcs->pktlog_enable(scn, ATH_PKTLOG_TX_CAPTURE_ENABLE) == 0) {
               qdf_info("Enabled Tx Capture");
               scn->pl_dev->pl_info->tx_capture_enabled = 1;
               value.cdp_pdev_param_tx_capture = val;
               cdp_txrx_set_pdev_param(soc_txrx_handle,
                                       pdev_id, CDP_CONFIG_TX_CAPTURE, value);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
            if (scn->sc_ic.nss_radio_ops)
                scn->sc_ic.nss_radio_ops->ic_nss_ol_wifi_tx_capture_set(scn, val);
#endif
            return 0;
        }
    } else if ((val == 0) && (tx_capture)) {
        if (scn->pl_dev->pl_funcs->pktlog_enable(scn, 0) == 0) {
            qdf_info("Disabled Tx Capture");
            scn->pl_dev->pl_info->tx_capture_enabled = 0;
            value.cdp_pdev_param_tx_capture = val;
            cdp_txrx_set_pdev_param(soc_txrx_handle,
                                    pdev_id, CDP_CONFIG_TX_CAPTURE, value);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
            if (scn->sc_ic.nss_radio_ops)
                scn->sc_ic.nss_radio_ops->ic_nss_ol_wifi_tx_capture_set(scn, val);
#endif
#ifdef OL_ATH_SMART_LOGGING
            if (scn->pl_dev->pl_sl_funcs.smart_log_fw_pktlog_unblock)
                scn->pl_dev->pl_sl_funcs.smart_log_fw_pktlog_unblock(scn);
#endif /* OL_ATH_SMART_LOGGING */
            return 0;
        }
    }
#endif
    return -1;
}

/*
 * Return true if target is a lithium target. else return false
 */
bool ol_target_lithium(struct wlan_objmgr_psoc *psoc)
{
    switch (lmac_get_tgt_type(psoc)) {
        case TARGET_TYPE_QCA8074:
        case TARGET_TYPE_QCA8074V2:
        case TARGET_TYPE_QCA9574:
        case TARGET_TYPE_QCA6018:
        case TARGET_TYPE_QCA5018:
        case TARGET_TYPE_QCA5332:
        case TARGET_TYPE_QCN9000:
        case TARGET_TYPE_QCN6122:
        case TARGET_TYPE_QCN9160:
        case TARGET_TYPE_QCN9224:
        case TARGET_TYPE_QCN6432:
            return true;
    }

    return false;
}
qdf_export_symbol(ol_target_lithium);

/* Waikiki is supported for both li and be in the code*/
bool ol_target_be(struct wlan_objmgr_psoc *psoc)
{
    switch (lmac_get_tgt_type(psoc)) {
        case TARGET_TYPE_QCN9224:
        case TARGET_TYPE_QCA5332:
        case TARGET_TYPE_QCN6432:
            return true;
    }

    return false;

}
qdf_export_symbol(ol_target_be);

#ifdef CONFIG_DP_TRACE
static int
ol_ath_dptrace_setparam(int cmd, int val1, int val2)
{
    switch (cmd) {
        case IEEE80211_DBG_DPTRACE_PROTO_BITMAP:
            qdf_dp_set_proto_bitmap(val1);
            return 0;
        case IEEE80211_DBG_DPTRACE_VERBOSITY:
            qdf_dp_trace_set_verbosity(val1);
            return 0;
        case IEEE80211_DBG_DPTRACE_NO_OF_RECORD:
            qdf_dp_set_no_of_record(val1);
            return 0;
        case IEEE80211_DBG_DPTRACE_DUMPALL:
            qdf_dp_trace_dump_all(val1, val2);
            return 0;
        case IEEE80211_DBG_DPTRACE_CLEAR:
            qdf_dp_trace_clear_buffer();
            return 0;
        case IEEE80211_DBG_DPTRACE_LIVEMODE_ON:
            qdf_dp_trace_enable_live_mode();
            return 0;
        case IEEE80211_DBG_DPTRACE_LIVEMODE_OFF:
            qdf_dp_trace_disable_live_mode();
            return 0;
        default:
            return -1;
    }
}
#endif


inline u_int32_t ol_if_peer_get_rate(struct wlan_objmgr_peer *peer, u_int8_t type)
{
    struct wlan_objmgr_psoc *psoc;
    ol_txrx_soc_handle soc;
    QDF_STATUS status;
    cdp_peer_stats_param_t buf = {0};
    uint8_t vdev_id;

    if (!peer)
        return 0;

    psoc = wlan_peer_get_psoc(peer);
    if (!psoc)
        return 0;

    vdev_id = wlan_vdev_get_id(peer->peer_objmgr.vdev);
    soc = wlan_psoc_get_dp_handle(psoc);
    switch (type) {
        case IEEE80211_RATE_TX:
             status = cdp_txrx_get_peer_stats_param(soc, vdev_id, peer->macaddr,
                                                    cdp_peer_tx_rate, &buf);
             if (QDF_IS_STATUS_ERROR(status))
                 return 0;

             return buf.tx_rate;
        case IEEE80211_RATE_RX:
             status = cdp_txrx_get_peer_stats_param(soc, vdev_id, peer->macaddr,
                                                    cdp_peer_rx_rate, &buf);
             if (QDF_IS_STATUS_ERROR(status))
                 return 0;

             return buf.rx_rate;
        case IEEE80211_RATECODE_TX:
             status = cdp_txrx_get_peer_stats_param(soc, vdev_id, peer->macaddr,
                                                    cdp_peer_tx_ratecode, &buf);
             if (QDF_IS_STATUS_ERROR(status))
                 return 0;

             return buf.tx_ratecode;
        case IEEE80211_RATEFLAGS_TX:
             status = cdp_txrx_get_peer_stats_param(soc, vdev_id, peer->macaddr,
                                                    cdp_peer_tx_flags, &buf);
             if (QDF_IS_STATUS_ERROR(status))
                 return 0;

             return buf.tx_flags;
        case IEEE80211_AVG_RATE_TX:
             status = cdp_txrx_get_peer_stats_param(soc, vdev_id, peer->macaddr,
                                                    cdp_peer_tx_avg_rate, &buf);
             if (QDF_IS_STATUS_ERROR(status))
                 return 0;

             return buf.tx_rate_avg;
        case IEEE80211_AVG_RATE_RX:
             status = cdp_txrx_get_peer_stats_param(soc, vdev_id, peer->macaddr,
                                                    cdp_peer_rx_avg_rate, &buf);
             if (QDF_IS_STATUS_ERROR(status))
                 return 0;

             return buf.rx_rate_avg;
        default:
             return 0;
    }
}

uint32_t ol_ath_get_interface_id(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct pdev_osif_priv *osif_priv;

    osif_priv = wlan_pdev_get_ospriv(pdev);

    if (!osif_priv) {
        qdf_err("osif_priv is NULL");
        return (uint32_t)-1;
    }

    scn = (struct ol_ath_softc_net80211 *)osif_priv->legacy_osif_priv;

    return scn->radio_id;
}

uint8_t ol_ath_is_bcn_mode_burst(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct pdev_osif_priv *osif_priv;

    if (!pdev) {
        qdf_err("pdev is null!");
        return 0;
    }

    osif_priv = wlan_pdev_get_ospriv(pdev);
    if (!osif_priv) {
        qdf_err("osif_priv is NULL");
        return 0;
    }
    scn = (struct ol_ath_softc_net80211 *)osif_priv->legacy_osif_priv;
    if (!scn) {
        qdf_err("scn is NULL");
        return 0;
    }

    return ((scn->bcn_mode == BEACON_TX_MODE_BURST)? 1 : 0);
}

void ol_ath_init_and_enable_radar_table(struct ieee80211com *ic)
{
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_dfs_rx_ops *dfs_rx_ops;
    struct wmi_unified *wmi_handle;

    pdev = ic->ic_pdev_obj;
    if (!pdev) {
        qdf_err("pdev is null");
        return;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        qdf_err("psoc is null");
        return;
    }

    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return;
    }

    dfs_rx_ops = wlan_lmac_if_get_dfs_rx_ops(psoc);
    if (dfs_rx_ops) {
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                                         QDF_STATUS_SUCCESS) {
            return;
        }

        if (!wmi_service_enabled(wmi_handle, wmi_service_dfs_phyerr_offload) &&
                dfs_rx_ops->dfs_get_radars)
            dfs_rx_ops->dfs_get_radars(pdev);

        if (dfs_rx_ops->dfs_radar_enable)
            dfs_rx_ops->dfs_radar_enable(pdev, 0, ic->ic_opmode, true);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    }
}

uint32_t ol_ath_num_mcast_tbl_elements(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    target_resource_config *tgt_cfg;

    tgt_cfg = lmac_get_tgt_res_cfg(psoc);
    if (!tgt_cfg) {
        qdf_err("psoc target res cfg is null");
        return 0;
    }

    return tgt_cfg->num_mcast_table_elems;
}

uint32_t ol_ath_num_mcast_grps(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    target_resource_config *tgt_cfg;

    tgt_cfg = lmac_get_tgt_res_cfg(psoc);
    if (!tgt_cfg) {
        qdf_err("psoc target res cfg is null");
        return 0;
    }

    return tgt_cfg->num_mcast_groups;
}

uint8_t ol_ath_get_tgt_res_cfg(struct ieee80211com *ic, uint8_t *tx_chain_mask, uint8_t *rx_chain_mask)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    target_resource_config * tgt_cfg = NULL;

    tgt_cfg = lmac_get_tgt_res_cfg(psoc);
    if (!tgt_cfg) {
        qdf_err("psoc target res cfg is null");
        return -1;
    }

    *tx_chain_mask = tgt_cfg->tx_chain_mask;
    *rx_chain_mask = tgt_cfg->rx_chain_mask;

    qdf_debug("Tx chain mask = %d, Rx chain mask = %d", *tx_chain_mask, *rx_chain_mask);

    return 0;
}

bool ol_ath_is_target_ar900b(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    return lmac_is_target_ar900b(psoc);
}

uint32_t ol_ath_get_tgt_type(struct ieee80211com *ic)
{
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    return lmac_get_tgt_type(psoc);
}

#if QCN_ESP_IE
int32_t ol_ath_esp_estimate_event_handler(ol_soc_t sc, uint8_t *data, uint32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct esp_estimation_event event, *ev = &event;
    struct ieee80211com *ic;
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_pdev *pdev;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle || wmi_extract_esp_estimate_ev_param(wmi_handle, data, ev))
        return -1;

    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, PDEV_UNIT(ev->pdev_id),
                                      WLAN_MLME_SB_ID);
    if (!pdev) {
        qdf_err("pdev object (id: %d) is NULL", PDEV_UNIT(ev->pdev_id));
        return -1;
    }
    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (ic) {
        ic->ic_fw_esp_air_time = ev->ac_airtime_percentage;
        ic->ic_esp_flag = 1;
    }
    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);

    return 0;
}
#endif /* QCN_ESP_IE */

int ol_ath_send_delba(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id, uint8_t *peer_macaddr,
                      uint8_t tid, uint8_t reason_code, uint8_t cdp_rcode)
{
    struct ieee80211_node *ni;
    struct ieee80211_action_mgt_args actionargs;
    struct wlan_objmgr_peer *peer;
    struct wlan_objmgr_vdev *vdev =  wlan_objmgr_get_vdev_by_id_from_psoc(
                                     (struct wlan_objmgr_psoc *)psoc, vdev_id, WLAN_MLME_SB_ID);

    if (!vdev)
        return -1;

    peer = wlan_objmgr_vdev_find_peer_by_mac(vdev, peer_macaddr, WLAN_MLME_SB_ID);

    if (!peer) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
        return -1;
    }

    ni = wlan_peer_get_mlme_ext_obj(peer);

    if (!ni) {
        wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
        return -1;
    }

    IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_ACTION, ni,
                "[%s]: Sending delba- node:0x%pK(%s) Tid: %d\n",
                __func__, ni, ether_sprintf(ni->ni_macaddr), tid);

    actionargs.category     = IEEE80211_ACTION_CAT_BA;
    actionargs.action       = IEEE80211_ACTION_BA_DELBA;
    actionargs.arg1         = tid;
    actionargs.arg2         = 0;
    actionargs.arg3         = reason_code;
    ieee80211_send_action(ni, &actionargs, NULL);

    wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);

    return 0;
}

uint8_t ol_ath_fill_umac_radio_band_info(struct wlan_objmgr_pdev *pdev)
{
    enum channel_state state;
    struct regulatory_channel *curr_chan_list;
    uint8_t i, num_valid_chan = 0;
    uint32_t low_5g = 0, high_5g = 0;
    uint8_t radio_band = NO_BAND_INFORMATION_AVAILABLE;
    uint32_t fiveg_channel[NUM_CHANNELS];
    int status = 0;

    curr_chan_list = qdf_mem_malloc
        (NUM_CHANNELS*sizeof(struct regulatory_channel));

    if (!curr_chan_list) {
        QDF_TRACE(QDF_MODULE_ID_SON, QDF_TRACE_LEVEL_ERROR,"%s: fail to alloc",
                  __func__);
        return NO_BAND_INFORMATION_AVAILABLE;
    }

    status = ucfg_reg_get_current_chan_list (pdev, curr_chan_list);

    if (QDF_IS_STATUS_ERROR(status)) {
        QDF_TRACE(QDF_MODULE_ID_SON, QDF_TRACE_LEVEL_ERROR,
                  "Couldn't get current chan list");
        qdf_mem_free(curr_chan_list);
        return NO_BAND_INFORMATION_AVAILABLE;
    }

    for (i=0; i<NUM_CHANNELS; i++) {
        state = curr_chan_list[i].state;
        if (state!= CHANNEL_STATE_DISABLE && state!= CHANNEL_STATE_INVALID
            && curr_chan_list[i].center_freq > MAX_FREQ_IN_TWOG) {
            fiveg_channel[num_valid_chan++]  = curr_chan_list[i].center_freq;
        }
    }

    if (!num_valid_chan) {
        qdf_mem_free(curr_chan_list);
        return NON_5G_RADIO;
    }

    low_5g = fiveg_channel[0];
    high_5g = fiveg_channel[num_valid_chan-1];
    if (low_5g >= regdmn_get_min_6ghz_chan_freq() && high_5g <= LOW_BAND_MAX_SIXG_FREQ)
        radio_band = LOW_BAND_6G_RADIO;
    else if (low_5g >= HIGH_BAND_MIN_SIXG_FREQ && high_5g <= regdmn_get_max_6ghz_chan_freq())
        radio_band = HIGH_BAND_6G_RADIO;
    else if (low_5g >= regdmn_get_min_6ghz_chan_freq() &&
         high_5g <= regdmn_get_max_6ghz_chan_freq())
        radio_band = FULL_BAND_6G_RADIO;
    else if ( low_5g >= regdmn_get_min_5ghz_chan_freq() &&
              high_5g <= regdmn_get_max_6ghz_chan_freq() &&
              high_5g > regdmn_get_max_5ghz_chan_freq() )
        radio_band = BAND_5G_6G_RADIO;
    else if (low_5g >= LOW_BAND_MIN_FIVEG_FREQ && high_5g <= LOW_BAND_MAX_FIVEG_FREQ)
        radio_band = LOW_BAND_5G_RADIO;
    else if (low_5g > LOW_BAND_MAX_FIVEG_FREQ)
        radio_band = HIGH_BAND_5G_RADIO;
    else if (low_5g >= LOW_BAND_MIN_FIVEG_FREQ && high_5g > LOW_BAND_MAX_FIVEG_FREQ)
        radio_band = FULL_BAND_5G_RADIO;

    qdf_mem_free(curr_chan_list);
    return radio_band;
}

/**
 * ol_ath_punctured_band_setting_check() - User input validity check for
 *                                         Preamble Puncturing
 * @ic - ic pointer
 * @value - user input
 *
 * Return: EINVAL - Failure
 *         0      - Success
 */
int ol_ath_punctured_band_setting_check(struct ieee80211com *ic, uint32_t value)
{
/* Preamble Puncturing is currently supported only for 80MHz BW setting.
 * The command takes bitmap input where each bit corresponds to a 20MHz band.
 * As per above, only B0-B3 are valid bits for preamble puncturing config for 80MHz.
 */
#define PUNCTURED_BAND_INVALID_MASK             0xFFF0
/* Currently only 20MHz band puncturing is supported */
#define NUM_20MHZ_PUNCTURED_BAND_SUPPORTED      1
    uint8_t puncture_bw_count = 0, primary20mhz_idx = 0;
    uint32_t puncture_bw_setting = value;
    uint16_t starting_freq = 0;

    if (value & PUNCTURED_BAND_INVALID_MASK) {
        qdf_err("Punctured band only supported for 80MHz");
        return -EINVAL;
    }

    while (puncture_bw_setting) {
        /* Determine number of bands punctured by user.
         */
        puncture_bw_setting &= (puncture_bw_setting - 1);
        puncture_bw_count++;
    }

    if (puncture_bw_count > NUM_20MHZ_PUNCTURED_BAND_SUPPORTED) {
        qdf_err("Puncturing supported for 20MHz band only");
        return -EINVAL;
    }

    /* Check if user is puncturing primary 20MHz band.
     * Calculate the Primary 20MHz channel index based on center
     * frequency and starting frequency setting.
     */
    starting_freq = (ic->ic_curchan->ic_vhtop_freq_seg1 - BW_40_MHZ);
    primary20mhz_idx = (ic->ic_curchan->ic_freq - starting_freq)/BW_20_MHZ;

    if(BIT(primary20mhz_idx) & value) {
        qdf_err("Primary 20MHz band cannot be punctured\n"
                "Punctured band index: %d", primary20mhz_idx);
        return -EINVAL;
    }

    return 0;
}

int ol_ath_get_ofdma_max_users(struct ol_ath_soc_softc *soc)
{
    struct target_psoc_info *tgt_psoc_info;
    struct target_mu_caps mu_caps;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return 0;
    }

    target_psoc_get_mu_max_users(tgt_psoc_info, &mu_caps);

    return (mu_caps.ofdma_ul << 16) | mu_caps.ofdma_dl;
}

int ol_ath_get_mumimo_max_users(struct ol_ath_soc_softc *soc)
{
    struct target_psoc_info *tgt_psoc_info;
    struct target_mu_caps mu_caps;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(soc->psoc_obj);
    if (!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return 0;
    }

    target_psoc_get_mu_max_users(tgt_psoc_info, &mu_caps);

    return (mu_caps.mumimo_ul << 16) | mu_caps.mumimo_dl;
}

int
ol_ath_pdev_set_halphy_cal_event_handler(ol_soc_t sc,
                                         u_int8_t *data,
                                         u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_host_pdev_set_halphy_cal_event event = {0};
    struct wmi_unified *wmi_handle;
    struct ol_ath_softc_net80211 *scn;
    struct wlan_objmgr_pdev *pdev;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;

    }

    if (wmi_extract_halphy_cal_ev_param(wmi_handle, data, &event)) {
        qdf_err("Failed to extract halphy cal status event");
        return -1;
    }

    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj,
                                      event.pdev_id,
                                      WLAN_MLME_NB_ID);
    if (!pdev) {
        qdf_err("pdev object (id: %d) is NULL", event.pdev_id);
        return -1;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
        qdf_err("scn obj is NULL");
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
        return -1;
    }

    if (event.status)
    {
        qdf_err("SET_HALPHY_CAL Failure");
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
        return -1;
    }
    else
    {
        qdf_info("SET_HALPHY_CAL Success");
    }

    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
    return 0;
}

#if QLD
/**
 * qld_generate_table() - function to generate qld table
 * @req: request structure
 *
 * Return: 0 - OK -EINVAL - On failure
 */
int qld_generate_table(void *req, struct qld_entry *entry)
{
    struct qld_event  *q_event = NULL;
    struct ol_qld_entry *q_entry = NULL;
    struct ol_qld_entry *start = NULL;
    struct qldinforeq *qld_req = NULL;

    if (!req || !entry) {
        qdf_err("QLD: Request or Node is NULL");
        return -EINVAL;
    }
    qld_req = (struct qldinforeq *)req;
    q_event = qld_req->qld_event_buf;
    start   = &q_event->qld_buffer[0];

    /* Position to current entry to copy */
    q_entry = &start[qld_req->index];
    q_entry->addr = entry->addr;
    q_entry->size = entry->size;
    qdf_snprintf(q_entry->name, sizeof(q_entry->name), "%s", entry->name);
    qld_req->index++;
    return 0;
}

/**
 * qld_send_event() - function to send event to userspace
 * @ic          : ic structure
 * @dev         : netdevice
 * @q_header    : qld_event
 * @total_count : total list count
 * @len         : length of event with header
 * @last_event  : flag to indicate if its last or only event
 *
 * @Return: 0 on success
 */
int qld_send_event(struct ieee80211com *ic, struct net_device *dev, \
                   struct qld_event *q_header, uint32_t total_count,  \
                   uint32_t len,bool last_event)
{
    /*Added header meta data*/
    q_header->cmd = IEEE80211_DBGREQ_GET_QLD_DUMP_TABLE;
    q_header->total_list_count = total_count;
    q_header->current_event_size = len;

    /* modify the flag if its last event*/
    if (last_event)
        q_header->flags = q_header->flags|QLD_END_EVENT;
    /* Send Data to userspace */
    wlan_cfg80211_generic_event(ic,
            QCA_NL80211_VENDOR_SUBCMD_DBGREQ, q_header, len, dev, GFP_ATOMIC);
    return 0;
}

#define QLD_LIST_ALLOC_SIZE (3 * 1024)   /*max size for qld msg */

/**
 * qld_submit_table() - function to submit table
 * @ic   : ic structure
 * @req  : req structure
 *
 * Return: 0 - OK -EINVAL - On failure
 */
int qld_submit_table(struct ieee80211com *ic ,void * req)
{
    struct qld_event  *q_event = NULL;
    struct net_device *dev;
    struct qldinforeq *qld_req = NULL;
    struct qld_event *q_event_header;
    uint32_t entry_size;
    uint8_t *send_msg;
    uint32_t len = 0;
    uint32_t total_len = 0;
    uint32_t total_count = 0;

    if (!req) {
        qdf_err("QLD: Request is NULL");
        return -EINVAL;
    }

    if ((qld_get_list_count(&total_count) != 0) || (total_count == 0)) {
        qdf_err("QLD: Invalid list count");
        return -EINVAL;
    }

    qld_req = req;
    q_event = (struct qld_event *)qld_req->qld_event_buf;
    entry_size = sizeof(struct ol_qld_entry);
    dev = ic->ic_netdev;
    q_event_header = q_event;
    send_msg = (uint8_t*)&q_event->qld_buffer[0];

    /* Give offset initially for header compensation*/
    len = sizeof(struct qld_event);
    total_len = len;

    q_event_header->flags = QLD_START_EVENT;
    qdf_err("QLD: Total list count %d", total_count);
    while (total_len < qld_req->space) {

        if (((len + entry_size) < QLD_LIST_ALLOC_SIZE)) {
            /* Keep moving the till we reach size that buffer can't hold*/
            len +=  entry_size;
            total_len += entry_size;
            send_msg += entry_size;
        } else {
            qdf_err("QLD:cmdid %d:current_size %d: flags %x",
                    q_event_header->cmd, q_event_header->current_event_size,
                    q_event_header->flags);
            qld_send_event(ic, dev, q_event_header, total_count, len, false);
            /*Reset the header ,we use the previous  memory to add header
              info so that we can send continous memory to user space
            |q_event header | q_entry [0] | q_entry[1] ... |
            |sent data ..| q_event_header | q_entry[1] ... |
            */
            /*Header is piggybacked */
            q_event_header  = (struct qld_event*) (send_msg - sizeof(struct qld_event));
            q_event_header->flags = QLD_MORE_EVENT;
            len = sizeof(struct qld_event);
        }
    }

    /* Send Remaining data! */
    qdf_err("QLD: Sending final chunk %d to userspace", len);
    if (len != 0) {
        qld_send_event(ic, dev, q_event_header, total_count, len, true);
        qdf_err("QLD:cmdid %d:current_size %d: flags %x",q_event_header->cmd,
                q_event_header->current_event_size,q_event_header->flags);
    }
    return 0;
}

/**
 * qld_dump_wait() - wait till upper layer finish
 * @soc   : soc structure
 *
 * Return: 0 - OK -EINVAL - On failure
 */
int qld_dump_wait(struct ol_ath_soc_softc *soc)
{
    if (!soc)
        return -EINVAL;

    qdf_err("QLD:Start waiting from application to finish");
    qdf_wait_single_event(&soc->qld_wait, QLD_WAIT_TIMEOUT_COUNT);
    qdf_err("QLD:Finished waiting from application");
    return 0;
}
qdf_export_symbol(qld_dump_wait);

/**
 * qld_process_list() - processing qld list
 * @soc   : soc structure
 *
 * Return: 0 - OK -EINVAL -ENOMEM - On failure
 */
int qld_process_list(struct ol_ath_soc_softc *soc)
{
    struct wlan_objmgr_pdev *pdev_obj;
    struct ieee80211com *ic;
    struct qldinforeq req;
    struct qld_event *q_event = NULL;
    uint32_t total_count = 0;
    uint32_t total_size;

    if (!soc)
        return -EINVAL;

    if ((qld_get_list_count(&total_count) != 0) || (total_count == 0)) {
        qdf_err("QLD: Invalid list count");
        return -EINVAL;
    }

    pdev_obj = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, 0, WLAN_MLME_SB_ID);

    if (!pdev_obj) {
        qdf_err("pdev object is NULL in FATAL_SHUTDOWN");
        return -EINVAL;
    }

    ic = wlan_pdev_get_mlme_ext_obj(pdev_obj);

    if (!ic) {
      qdf_err("QLD:ic is NULL");
      wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);
      return -EINVAL;
    }

    /* Total size(bytes) including size of  qld_event header */
    total_size = sizeof(struct qld_event) + \
                       (total_count * sizeof(struct ol_qld_entry));
    /* Allocate total memory for qld table*/
    q_event = (struct qld_event *) qdf_mem_malloc_atomic(total_size);
    if (!q_event) {
        qdf_err("QLD: Memory allocation failed");
        wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);
        return -ENOMEM;
    }

    /*Zero out memory*/
    qdf_mem_set(q_event,total_size, 0);
    qdf_mem_set(&req,sizeof(req), 0);
    q_event->total_list_count = total_count;
    req.qld_event_buf = q_event;
    req.space = total_size;

    /*Iterate list to generate table*/
    qld_iterate_list(&qld_generate_table, &req);
    qdf_err("QLD: Printing table before transmit");
    qld_print_table(q_event);
    qdf_err("QLD: Calling submit command to transmit table");
    qld_submit_table(ic,&req);
    qdf_event_reset(&soc->qld_wait);
    qdf_mem_free(q_event);
    wlan_objmgr_pdev_release_ref(pdev_obj, WLAN_MLME_SB_ID);
    return 0;
}
qdf_export_symbol(qld_process_list);

/*
 * @qld_print_talbe () - Print qld table
 * @ dump_table        : qld_event pointer
 *
 * Return: None
 */
void qld_print_table(struct qld_event *dump_table)
{
    uint32_t index = 0;
    uint32_t total_list_count;
    struct ol_qld_entry *q_entry;
    struct ol_qld_entry *start;

    if (!is_qld_enable())
        return;

    if (!dump_table) {
        qdf_err("dump_table handle is NULL");
        return;
    }
    total_list_count = dump_table->total_list_count;
    start = &dump_table->qld_buffer[0];
    qdf_err("TOTAL entries are %d", total_list_count);
    while (index < total_list_count) {
        /* Position to current entry to copy */
        q_entry = &start[index];
        qdf_err("Current entry name=%s, addr=%llx, size=%d",
                q_entry->name, q_entry->addr, q_entry->size);
        index++;
    }
    qdf_err("ALL entries printed");
}
qdf_export_symbol(qld_print_table);
#endif /* QLD */

osdev_t get_osdev_from_psoc(struct wlan_objmgr_psoc *psoc)
{

    struct target_psoc_info *tgt_psoc_info;
    struct ol_ath_soc_softc *soc;
    osdev_t osdev = NULL;

    if (!psoc)
        return NULL;

    tgt_psoc_info = wlan_psoc_get_tgt_if_handle(psoc);
    if(!tgt_psoc_info) {
        qdf_err("target_psoc_info is null");
        return NULL;
    }

    soc = (struct ol_ath_soc_softc *) target_psoc_get_feature_ptr(tgt_psoc_info);
    if (!soc)
        return NULL;

    osdev = soc->sc_osdev;

    return osdev;
}

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
bool ol_ath_mlo_capable(struct wlan_objmgr_psoc *psoc)
{
    osdev_t osdev = get_osdev_from_psoc(psoc);

    if (!osdev)
        return false;

    return pld_get_mlo_capable(osdev->device);
}

uint8_t ol_ath_mlo_get_group_id(struct wlan_objmgr_psoc *psoc)
{
    osdev_t osdev = get_osdev_from_psoc(psoc);
    uint8_t grp_id = WLAN_MLO_GROUP_INVALID;

    if (!osdev)
        return grp_id;

    if (!ol_ath_mlo_capable(psoc))
        return grp_id;

    grp_id = pld_get_mlo_group_id(osdev->device);
    return grp_id;
}

uint8_t ol_ath_mlo_get_chip_id(struct wlan_objmgr_psoc *psoc)
{
    osdev_t osdev = get_osdev_from_psoc(psoc);
    uint8_t chip_id = WLAN_MLO_CHIP_ID_INVALID;

    if (!osdev)
        return chip_id;

    if (!ol_ath_mlo_capable(psoc))
        return chip_id;

    chip_id = pld_get_mlo_chip_id(osdev->device);
    return chip_id;
}
#endif /* 11BE_MLO && MULTI_CHIP */

void ol_ath_send_standby_entry_complete(struct ol_ath_softc_net80211 *scn)
{
     qdf_info("WIFI Standby entry complete - Notify Application");
     ol_ath_wifi_notify_standby_entry_cmp();
}

void ol_ath_send_standby_exit_complete(struct ol_ath_softc_net80211 *scn)
{
    qdf_info("WIFI Standby exit complete - Notify Application");
    ol_ath_wifi_notify_standby_exit_cmp();

    ol_ath_set_standby_mode(scn, WLAN_STANDBY_MODE_DISABLED);
}

void ol_ath_send_wakeup_trigger(struct cdp_ctrl_objmgr_psoc *psoc, uint8_t vdev_id)
{
    struct net_device *dev = NULL;
    struct wlan_objmgr_vdev *vdev_obj = NULL;
    struct ieee80211vap *vap = NULL;
    osif_dev *osdev;

    vdev_obj = wlan_objmgr_get_vdev_by_id_from_psoc(
                                      (struct wlan_objmgr_psoc *)psoc,
                                      vdev_id, WLAN_OSIF_ID);

    if (!vdev_obj) {
        return;
    }

    vap = wlan_vdev_get_mlme_ext_obj(vdev_obj);
    if (!vap) {
        wlan_objmgr_vdev_release_ref(vdev_obj, WLAN_OSIF_ID);
        return;
     }

    osdev = (osif_dev *)vap->iv_ifp;
    dev = osdev->netdev;

    qdf_info("Received wakeup trigger from psoc %d and vdev id %d - Notify Application",
              ((struct wlan_objmgr_psoc *)psoc)->soc_objmgr.psoc_id, vdev_id);
    wlan_objmgr_vdev_release_ref(vdev_obj, WLAN_OSIF_ID);
    ol_ath_wifi_standby_wakeup_trigger(dev);
}

#define COOKIE_LEN       8
#define COOKIE_OFFSET    8
void ol_ath_notify_eapol_tx_compl_status(struct cdp_ctrl_objmgr_psoc *soc,qdf_nbuf_t nbuf,
                                         uint8_t vdev_id, uint8_t tx_status)
{
    struct net_device *dev;
    struct wlan_objmgr_vdev *vdev_obj = NULL;
    struct ieee80211vap *vap = NULL;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_peer_context *mlo_peer_ctx;
    wlan_if_t assoc_vap;
    struct wlan_objmgr_vdev *assoc_vdev = NULL;
    struct wlan_objmgr_peer *assoc_peer = NULL;
    uint8_t *dst = NULL,*src = NULL;
#endif
    struct wireless_dev *wdev;
    osif_dev* assoc_osdev;
    u64 cookie;

    vdev_obj = wlan_objmgr_get_vdev_by_id_from_psoc(
                                      (struct wlan_objmgr_psoc *)soc,
                                      vdev_id, WLAN_OSIF_ID);
    if (!vdev_obj)
        return;

    vap = wlan_vdev_get_mlme_ext_obj(vdev_obj);
    if (!vap) {
        wlan_objmgr_vdev_release_ref(vdev_obj, WLAN_OSIF_ID);
        return;
    }
    assoc_osdev = (osif_dev *)vap->iv_ifp;
    dev = get_cfg80211_notification_ndev(assoc_osdev);
    nbuf->dev = dev;

#ifdef WLAN_FEATURE_11BE_MLO
    /*Fetch assoc vdev for MLO/Bridge vap case*/
    if (ieee80211_mlo_is_mld_ap(vdev_obj) || IEEE80211_IS_MLO_BRIDGE_VAP(vap) ||
        mlo_is_mld_sta(vdev_obj)) {
        dst = qdf_nbuf_data(nbuf);
	src = qdf_nbuf_data(nbuf) + QDF_MAC_ADDR_SIZE;
        mlo_peer_ctx = wlan_mlo_get_mlpeer_by_mld_mac(vdev_obj->mlo_dev_ctx,
                                                       (struct qdf_mac_addr *)dst);
        if (mlo_peer_ctx) {
            assoc_peer = wlan_mlo_peer_get_assoc_peer(mlo_peer_ctx);
            if (assoc_peer) {
                assoc_vdev = wlan_peer_get_vdev(assoc_peer);
                assoc_vap = wlan_vdev_get_mlme_ext_obj(assoc_vdev);

                if (assoc_vap) {
                    assoc_osdev = (osif_dev*)(assoc_vap->iv_ifp);
                    dev = get_cfg80211_notification_ndev(assoc_osdev);
                    nbuf->dev = dev;
                }

            }

        }
    }
#endif

    OS_MEMCPY(&cookie, qdf_nbuf_head(nbuf) + COOKIE_OFFSET, COOKIE_LEN);

    wdev = get_cfg80211_notification_wdev(assoc_osdev);
    wlan_objmgr_vdev_release_ref(vdev_obj, WLAN_OSIF_ID);
#ifdef WLAN_FEATURE_11BE_MLO
    if (qdf_unlikely(!wdev || !wdev->wiphy)) {
        qdf_err("null wiphy dereference can happen at kernel hence return");
        if (dst && src) {
            qdf_err(" destination address is %s \n",ether_sprintf(dst));
            qdf_err(" source address is %s \n",ether_sprintf(src));
	}
        return;
    }
#endif
    cfg80211_control_port_tx_status(wdev, cookie,
                                    qdf_nbuf_data(nbuf), nbuf->len,
                                    (tx_status == 0)? true:false, GFP_ATOMIC);
}
#endif

uint32_t ol_ath_fill_tx_rx_obss_chan_util(struct wlan_objmgr_pdev *pdev)
{
    struct pdev_ic_cp_stats *cp_stats;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;
    uint32_t stats = 0;

    if (!pdev) {
        qdf_err("Invalid pdev!");
        return stats;
    }
    cp_stats = qdf_mem_malloc(sizeof(struct pdev_ic_cp_stats));
    if (!cp_stats) {
        qdf_err("Allocation Failed!");
        return stats;
    }
    ret = wlan_cfg80211_get_pdev_cp_stats(pdev, cp_stats);
    if (QDF_STATUS_SUCCESS != ret) {
        qdf_err("Unbale to get pdev control stats!");
        qdf_mem_free(cp_stats);
	return stats;
    }

    stats |= cp_stats->stats.chan_stats.dcs_total_util;
    stats |= (cp_stats->stats.chan_stats.dcs_obss_rx_util << 8);
    stats |= (cp_stats->stats.chan_stats.dcs_ap_tx_util << 16);
    stats |= (cp_stats->stats.chan_stats.dcs_ap_rx_util << 24);

    qdf_debug("CU Rx(24-31) %d Tx(16-23) %d OBSS Rx(8-15) %d Total(0-7) %d",
              cp_stats->stats.chan_stats.dcs_ap_rx_util,
              cp_stats->stats.chan_stats.dcs_ap_tx_util,
              cp_stats->stats.chan_stats.dcs_obss_rx_util,
              cp_stats->stats.chan_stats.dcs_total_util);

    qdf_mem_free(cp_stats);

    return stats;
}
