/*
 * Copyright (c) 2016-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef IEEE80211_UCFG_H_
#define IEEE80211_UCFG_H_

#ifdef WLAN_SUPPORT_DPDK
#include <cfgmgr_api_if.h>
#include <dpdk_wlan_msg_types.h>
#endif

void ieee80211_vap_iter_set_des_channel(void *arg, wlan_if_t vap);
int ieee80211_ucfg_set_essid(wlan_if_t vap, ieee80211_ssid *data, bool is_vap_restart_required);
int ieee80211_ucfg_set_beacon_interval(wlan_if_t vap, struct ieee80211com *ic,
        int value, bool is_vap_restart_required);
int ieee80211_ucfg_get_essid(wlan_if_t vap, ieee80211_ssid *data, int *nssid);
int ieee80211_ucfg_get_freq(wlan_if_t vap);
QDF_STATUS ieee80211_get_csa_chwidth_from_cwm(
        enum ieee80211_cwm_width cwm_width, u_int16_t *pcsa_ch_width);

int ieee80211_ucfg_set_freq(wlan_if_t vap, struct chan_params *chan);
int ieee80211_ucfg_set_freq_internal(wlan_if_t vap, struct chan_params *chan);
bool ieee80211_is_dot11_channel_mode_valid(wlan_if_t vap,
                                           struct chan_params *chan_p,
                                           int mode);
int ieee80211_ucfg_set_chanswitch(wlan_if_t vaphandle, struct chan_params *chan_p, u_int8_t tbtt, u_int16_t ch_width);
struct ieee80211_ath_channel
    ieee80211_ucfg_get_current_channel(wlan_if_t vaphandle, bool hwChan);
struct ieee80211_ath_channel
    ieee80211_ucfg_get_bss_channel(wlan_if_t vaphandle);
int ieee80211_ucfg_delete_vap(wlan_if_t vap);
int ieee80211_ucfg_set_rts(wlan_if_t vap, u_int32_t val);
int ieee80211_ucfg_set_frag(wlan_if_t vap, u_int32_t val);
int ieee80211_ucfg_set_txpow(wlan_if_t vaphandle, int txpow);
int ieee80211_ucfg_get_txpow(wlan_if_t vaphandle, int *txpow, int *fixed);
int ieee80211_ucfg_get_txpow_fraction(wlan_if_t vaphandle, int *txpow, int *fixed);
int ieee80211_ucfg_set_ap(wlan_if_t vap, u_int8_t (*des_bssid)[QDF_MAC_ADDR_SIZE]);
int ieee80211_ucfg_get_ap(wlan_if_t vap, u_int8_t *addr);
#ifdef QCA_SUPPORT_MBSS_CMN_PARAM
bool ieee80211_ucfg_is_mbss_common_param(wlan_if_t vap, int param);
#endif
#ifdef WLAN_SUPPORT_GREEN_AP
extern QDF_STATUS ucfg_green_ap_config(struct wlan_objmgr_pdev *pdev, uint8_t val);
#endif
int ieee80211_disabled_sched_mode(struct ieee80211com *ic, wlan_if_t vap,
                                  uint32_t value);
int ieee80211_ucfg_set_peer_tid_noack_config(struct ieee80211com *ic,
                                             struct wlan_objmgr_pdev *pdev,
                                             uint32_t vdev_id,
                                             uint8_t *macaddr,
                                             uint16_t noack_map);
int ieee80211_ucfg_setparam_util(wlan_if_t vap, int param, int value, char *extra,
                                 bool is_cmn_param, bool is_tx_vap);
int ieee80211_ucfg_setparam(wlan_if_t vap, int param, int value, char *extra);
int ieee80211_ucfg_getparam(wlan_if_t vap, int param, int *value);
u_int32_t ieee80211_ucfg_get_maxphyrate(wlan_if_t vaphandle);
bool ieee80211_is_mode_lower(wlan_if_t vap, enum ieee80211_phymode mode);
bool ieee80211_is_mode_higher(wlan_if_t vap, enum ieee80211_phymode mode);
enum ieee80211_phymode ieee80211_get_derived_mode(wlan_if_t vap, enum ieee80211_phymode mode, enum ieee80211_mode change_mode, struct chan_params* chan_p);
enum ieee80211_mode ieee80211_mixed_modes_config(wlan_if_t vap, enum ieee80211_phymode mode);
int ieee80211_ucfg_set_phymode(wlan_if_t vap, char *modestr, int len, bool reset_vap);
int ieee80211_ucfg_set_wirelessmode(wlan_if_t vap, int mode);
int ieee80211_ucfg_set_encode(wlan_if_t vap, u_int16_t length, u_int16_t flags, void *keybuf);
int ieee80211_ucfg_set_rate(wlan_if_t vap, int value);
int ieee80211_ucfg_get_phymode(wlan_if_t vap, char *modestr, u_int16_t *length, int type);
void ieee80211_ucfg_get_opmode(wlan_if_t vap, char *modestr, u_int16_t *length);
int ieee80211_ucfg_splitmac_add_client(wlan_if_t vap, u_int8_t *stamac, u_int16_t associd,
        u_int8_t qos, struct ieee80211_rateset lrates,
        struct ieee80211_rateset htrates, u_int16_t vhtrates);
int ieee80211_ucfg_splitmac_del_client(wlan_if_t vap, u_int8_t *stamac);
int ieee80211_ucfg_splitmac_authorize_client(wlan_if_t vap, u_int8_t *stamac, u_int32_t authorize);
int ieee80211_ucfg_splitmac_set_key(wlan_if_t vap, u_int8_t *macaddr, u_int8_t cipher,
        u_int16_t keyix, u_int32_t keylen, u_int8_t *keydata);
int ieee80211_ucfg_splitmac_del_key(wlan_if_t vap, u_int8_t *macaddr, u_int16_t keyix);
int ieee80211_ucfg_getstainfo(wlan_if_t vap, struct ieee80211req_sta_info *si, uint32_t *len);
int ieee80211_ucfg_getstaspace(wlan_if_t vap);

int ieee80211_convert_mode(const char *mode);
#define MAX_PHYMODE_STRLEN 30
void ieee80211_convert_phymode_to_string(enum ieee80211_phymode  phymode,
                                           char *modestr, u_int16_t *length);

#if ATH_SUPPORT_IQUE
int ieee80211_ucfg_rcparams_setrtparams(wlan_if_t vap, uint8_t rt_index, uint8_t per, uint8_t probe_intvl);
int ieee80211_ucfg_rcparams_setratemask(wlan_if_t vap, uint8_t preamble,
        uint32_t mask_lower32, uint32_t mask_higher32, uint32_t mask_lower32_2);
#endif
int ieee80211_ucfg_rtt_params(struct ieee80211com *ic, wlan_if_t vap,
                              struct ieee80211_wlanconfig *config);
int ieee80211_ucfg_nawds(wlan_if_t vap, struct ieee80211_wlanconfig *config);
#ifdef UMAC_SUPPORT_MLNAWDS
int ieee80211_ucfg_mlnawds(wlan_if_t vap, struct ieee80211_wlanconfig *config);
#endif
#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
int ieee80211_ucfg_mesh(wlan_if_t vap, struct ieee80211_wlanconfig *config);
#endif
#endif
int ieee80211_ucfg_me_list(wlan_if_t vap, struct ieee80211_wlanconfig *config);
int ieee80211_ucfg_hmwds(wlan_if_t vap, struct ieee80211_wlanconfig *config, int buffer_len);
#ifdef IPA_OFFLOAD
int ieee80211_ucfg_ipa_wds(wlan_if_t vap, struct ieee80211_wlanconfig *config,
                           int buffer_len);
#endif
int ieee80211_ucfg_wnm(wlan_if_t vap, struct ieee80211_wlanconfig *config);
int ieee80211_ucfg_vendorie(wlan_if_t vap, struct ieee80211_wlanconfig_vendorie *vie);
int ieee80211_ucfg_addie(wlan_if_t vap, struct ieee80211_wlanconfig_ie *ie_buffer);
int ieee80211_ucfg_nac(wlan_if_t vap, struct ieee80211_wlanconfig *config);
int ieee80211_ucfg_nac_rssi(wlan_if_t vap, struct ieee80211_wlanconfig *config);
int ieee80211_ucfg_isolation(wlan_if_t vap, struct ieee80211_wlanconfig *config);
int ieee80211_ucgf_scanlist(wlan_if_t vap);
size_t scan_space(wlan_scan_entry_t se, u_int16_t *ielen);
QDF_STATUS get_scan_space(void *arg, wlan_scan_entry_t se);
QDF_STATUS get_scan_space_rep_move(void *arg, wlan_scan_entry_t se);
QDF_STATUS get_scan_result(void *arg, wlan_scan_entry_t se);
int ieee80211_ucfg_get_best_otherband_uplink_bssid(wlan_if_t vap, char *bssid);
int ieee80211_ucfg_get_otherband_uplink_bssid(wlan_if_t vap, char *bssid);
int ieee80211_ucfg_scanlist(wlan_if_t vap);
void ieee80211_ucfg_setmaxrate_per_client(void *arg, wlan_node_t node);
void get_sta_space(void *arg, wlan_node_t node);
uint8_t get_phymode_from_chwidth(struct ieee80211com *ic, struct ieee80211_node *ni);
void get_sta_info(void *arg, wlan_node_t node);
int ieee80211_ucfg_setwmmparams(void *osif, int wmmparam, int ac, int bss, int value);
int ieee80211_ucfg_getwmmparams(void *osif, int wmmparam, int ac, int bss);
int ieee80211_ucfg_set_peer_nexthop(void *osif, uint8_t *mac, int32_t if_num);
int ieee80211_ucfg_set_vlan_type( void *osif, uint8_t default_vlan, uint8_t port_vlan);
int ieee80211_ucfg_set_hlos_tid_override(void *osif, uint8_t val, bool is_mscs);
int ieee80211_ucfg_get_hlos_tid_override(void *osif);
int ieee80211_ucfg_set_drop_3addr_mcast(wlan_if_t vap, uint8_t val);
int ieee80211_ucfg_set_muedcaparams(void *osif, uint8_t muedcaparam,
        uint8_t ac, uint8_t value);
int ieee80211_ucfg_set_peer_tid_latency_enable(void *osif, uint8_t val);
int ieee80211_ucfg_get_peer_tid_latency_enable(void *osif);
int ieee80211_ucfg_set_vap_mesh_tid(void *osif, uint8_t val);
int ieee80211_ucfg_get_vap_mesh_tid(void *osif, uint8_t *tid, uint8_t *dl_ul_enable);;
struct ieee80211vap *ieee80211_ucfg_get_txvap(struct ieee80211com *ic, int group_id, mbss_mode_t current_mode);
#if QCA_SUPPORT_GPR
int ieee80211_ucfg_send_gprparams(wlan_if_t vap, uint8_t value);
#endif
int ieee80211_ucfg_get_muedcaparams(void *osif, uint8_t muedcaparam, uint8_t ac);
int ieee80211_ucfg_setmlme(struct ieee80211com *ic, void *osif, struct ieee80211req_mlme *mlme);
int ieee80211_ucfg_cfr_params(struct ieee80211com *ic, wlan_if_t vap, struct ieee80211_wlanconfig *config);
#ifdef WLAN_WIFI_RADAR_ENABLE
int ieee80211_ucfg_wifi_radar_params(struct ieee80211com *ic, struct ieee80211_wlanconfig *config);
#endif
extern unsigned int g_unicast_deauth_on_stop;
extern unsigned int g_csa_max_rx_wait_time;
int ieee80211_ucfg_get_quality(wlan_if_t vap, void *iq);
int ieee80211_get_chan_nf(struct ieee80211com *ic, int16_t *nf_val);
int ieee80211_ucfg_send_probereq(wlan_if_t vap, int val);
QDF_STATUS check_inter_band_switch_compatibility(struct ieee80211com *ic);
int ieee80211_ucfg_set_txvap(wlan_if_t vap);
int ieee80211_ucfg_reset_txvap(wlan_if_t vap, uint8_t force);
int ieee80211_ucfg_bringdown_txvap(wlan_if_t vap);
void ieee80211_ucfg_copy_txvap_param(struct ieee80211vap *vap,
                                     struct ieee80211vap *last_txvap);
int ieee80211_ucfg_reset_nawds_txvap(wlan_if_t vap);
int ieee80211_ucfg_get_partial_bssid_bitmap(struct ieee80211com *ic, uint32_t *value);
//int ieee80211_ucfg_reset_mesh_nawds_txvap(wlan_if_t vap);
int ieee80211_ucfg_mu_snif(struct ieee80211com *ic, wlan_if_t vap, struct ieee80211_wlanconfig *config);
#ifdef CONFIG_SAWF_DEF_QUEUES
int ieee80211_ucfg_sawf_view(bool is_sawf);

struct sawf_rsp_param {
    uint32_t svc_id;
};

int ieee80211_ucfg_sawf_params(struct ieee80211com *ic,
                               struct ieee80211_wlanconfig *config,
                               struct sawf_rsp_param *rsp_param);
#endif
#ifdef WLAN_FEATURE_11BE
bool ieee80211_is_tids_mapped_to_assoc_link(struct ieee80211vap *vap,
        struct wlan_t2lm_onging_negotiation_info *t2lm);
int ieee80211_ucfg_tid_to_link_mapping(struct ieee80211com *ic, wlan_if_t vap,
                                       struct ieee80211_wlanconfig *config);
int ieee80211_ucfg_link_map_config(struct ieee80211com *ic, wlan_if_t vap,
                                   struct ieee80211_wlanconfig *config);
bool ieee80211_ucfg_is_homogeneous_t2lm(
        struct wlan_t2lm_onging_negotiation_info *t2lm_info);
struct ieee80211_node *ieee80211_ucfg_get_mlo_assoc_link_node(
        struct wlan_mlo_peer_context *mlo_peer_ctx);
bool ieee80211_ucfg_is_valid_t2lm_link_map(struct ieee80211vap *vap);
int ieee80211_ucfg_epcs(struct ieee80211com *ic, wlan_if_t vap,
                                               struct ieee80211_wlanconfig *config);
#ifdef WLAN_FEATURE_11BE_MLO
int ieee80211_get_link_id_bpcc_val(wlan_if_t  vap, struct mlo_link_id_bpcc_count *get_bpcc_link_id);

/**
 * ieee80211_send_non_tx_vap_t2lm_link_map_config() - API to send the T2LM WMI
 * command for non-tx vaps.
 * @tx_vap: Pointer to Tx vap
 *
 * Return: Success or failure
 */
int ieee80211_send_non_tx_vap_t2lm_link_map_config(struct ieee80211vap *vap);

/**
 * ieee80211_send_t2lm_link_map_config() - API to send the configured T2LM
 * values to FW.
 * @vap: T2LM configured vap
 * @send_default_map: Send default T2LM to FW
 *
 * Return: success or failure
 */
int ieee80211_send_t2lm_link_map_config(struct ieee80211vap *vap,
                                        bool send_default_map);
#endif /* WLAN_FEATURE_11BE_MLO */
#endif /* WLAN_FEATURE_11BE */

/*
 * IEEE80211_SKIP_WIDEBAND_SWITCH:
 * Wideband channel sanity is not checked for the following:
 * (1) If there are no beaconing AP VAPs present
 *     i.e., the channel change request came from a non-AP VAP and therefore
 *     there is no need for a mode switch or security compliancy check.
 * (2) If the desired mode or channel is not set
 *     i.e., bring-up channel should not be restricted.
 *     Security compliancy will be checked during start_ap() and MBSS
 *     mode compliancy is not required since mode switch will not
 *     be performed to honor the bring-up MBSS mode.
 * (3) If the target frequency is zero
 *     i.e., ACS invokation from userspace need not have a wideband check
 *     since the channel is still not determined
 * (4) If the transition is not a wideband channel switch
 *     Don't restrict intraband channel changes.
 */

#define IEEE80211_SKIP_WIDEBAND_SWITCH(__vap, __target_chan)           \
    (!ieee80211_get_num_beacon_ap_vaps((__vap)->iv_ic) ||              \
     !(IEEE80211_IS_CHAN_11AX((__target_chan))) ||                     \
     !((__vap)->vdev_obj->vdev_mlme.des_chan)->ch_freq ||              \
     !(IEEE80211_IS_CHAN_VALID(__target_chan)) ||                      \
     !IEEE80211_ARE_CHANS_INTERWIDEBAND((__vap)->iv_ic->ic_curchan,    \
                                        (__target_chan)))              \


#ifdef WLAN_FEATURE_11BE_MLO
/**
 * ieee80211_ucfg_validate_mlo_link_removal_request() - Validate link removal
 * request.
 * @vap: VAP corresponding to the link to be removed
 * @tbtt_count: AP removal timer TBTT value
 *
 * Return: QDF_STATUS of validation
 */
QDF_STATUS
ieee80211_ucfg_validate_mlo_link_removal_request(wlan_if_t vap,
                                                 uint32_t tbtt_count);

/**
 * ieee80211_ucfg_process_mlo_link_removal_cmd() - Handle link removal command
 * @vap: VAP corresponding to the link to be removed
 * @tbtt_count: AP removal timer TBTT value
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS ieee80211_ucfg_process_mlo_link_removal_cmd(wlan_if_t vap,
                                                       uint32_t tbtt_count);

#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
/**
 * ieee80211_ucfg_set_peer_ptqm_migrate() - Migrate PTQM of a given peer
 * @vap: VAP pointer
 * @mac: mld mac of peer
 * @value: Bitmask of user inputs
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS
ieee80211_ucfg_set_peer_ptqm_migrate(struct ieee80211vap *vap,
                                     uint8_t *mac, uint32_t value);

/**
 * ieee80211_ucfg_set_link_ptqm_migrate() - Migrate peers from a given link
 * @vap: VAP pointer
 *
 * Return: QDF_STATUS of operation
 */
QDF_STATUS
ieee80211_ucfg_set_link_ptqm_migrate(struct ieee80211vap *vap);

/*
 * ieee80211_ucfg_mlreconfig_set_link_ptqm_layer_migrate()
 * Migrate PTQM link for ML Reconfig
 *
 * @vap: VAP pointer
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
ieee80211_ucfg_mlreconfig_set_link_ptqm_layer_migrate(struct ieee80211vap *vap);
#endif /* QCA_SUPPORT_PRIMARY_LINK_MIGRATE */
#endif
void ieee80211_parse_tdma_config(
                 struct ieee80211com *ic,
                 struct ieee80211_wlanconfig_tdma_schedule *tdma_config);

#ifdef WLAN_SUPPORT_DPDK
int ieee80211_ucfg_cfgmgr_get_soc_info(struct dpdk_wlan_soc_info_cmd *req);
int ieee80211_ucfg_cfgmgr_get_vdev_info(struct dpdk_wlan_vdev_info_cmd *req);
int ieee80211_ucfg_cfgmgr_get_peer_info(struct dpdk_wlan_peer_info_cmd *req);
#endif

#if defined(CONFIG_SAWF) || defined(WLAN_SUPPORT_SCS)
void
ieee80211_ucfg_set_scs_qos_attr(struct ieee80211_qos_attr *qos_attr,
                                uint32_t min_tput, uint32_t service_interval,
                                uint32_t burst_size, uint32_t delay_bound,
                                uint32_t tid);
#endif
#ifdef CONFIG_SAWF
enum sawf_ul_op {
    SAWF_UL_OP_ADD,
    SAWF_UL_OP_UPDATE,
    SAWF_UL_OP_DELETE,
    SAWF_UL_OP_INVALID,
};

QDF_STATUS
ieee80211_ucfg_send_ul_sawf_params(struct wlan_objmgr_pdev *pdev,
                                   uint8_t *peer_mac_addr,
                                   uint32_t svc_id, uint8_t rx_tid,
                                   enum sawf_ul_op ul_op);
#endif
#endif //IEEE80211_UCFG_H_
