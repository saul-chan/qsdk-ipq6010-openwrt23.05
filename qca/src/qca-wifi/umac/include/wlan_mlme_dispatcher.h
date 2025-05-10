/*
 * Copyright (c) 2017-2018,2020-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */
#include <_ieee80211.h>
#include "ieee80211_ioctl.h"
#ifndef _WLAN_MLME_DISPATCHER_H
#define _WLAN_MLME_DISPATCHER_H
#include <wlan_reg_services_api.h>
#include <wlan_repeater_api.h>

int16_t wlan_vdev_get_chan_freq(struct wlan_objmgr_vdev *vdev);
int16_t wlan_vdev_get_chan_freq_seg2(struct wlan_objmgr_vdev *vdev);

enum phy_ch_width wlan_vdev_get_ch_width(struct wlan_objmgr_vdev *vdev);
enum wlan_phymode wlan_vdev_get_phymode(struct wlan_objmgr_vdev *vdev);

int wlan_vdev_acl_probe(struct wlan_objmgr_vdev *vdev, bool set, struct ieee80211req_athdbg *req_t);

int wlan_vdev_local_disassoc(struct wlan_objmgr_vdev *vdev, struct wlan_objmgr_peer *peer);


int wlan_vdev_get_node_info(struct wlan_objmgr_vdev *vdev, u_int8_t *mac,
			    wlan_node_info *info);

int wlan_peer_get_node_max_MCS(struct wlan_objmgr_peer *peer);

QDF_STATUS wlan_peer_send_null(struct wlan_objmgr_peer *peer);

bool wlan_peer_update_sta_stats(struct wlan_objmgr_peer *peer,
				struct wlan_sta_stats_ind *sta_stats,
                                void *stats);

u_int8_t wlan_peer_get_snr(struct wlan_objmgr_peer *peer);

u_int32_t wlan_peer_get_rate(struct wlan_objmgr_peer *peer);

struct ieee80211_ath_channel wlan_vdev_get_channel(struct wlan_objmgr_vdev *vdev);

int wlan_vdev_acs_set_user_chanlist(struct wlan_objmgr_vdev *vdev, bool append,
        qdf_freq_t *chanlist);

u_int8_t wlan_vdev_get_rx_streams(struct wlan_objmgr_vdev *vdev);
u_int8_t wlan_vdev_get_160_nss(struct wlan_objmgr_vdev *vdev);
u_int8_t wlan_vdev_get_80p80_nss(struct wlan_objmgr_vdev *vdev);
u_int8_t wlan_vdev_get_le80_nss(struct wlan_objmgr_vdev *vdev);
u_int8_t wlan_vdev_get_nss(struct wlan_objmgr_vdev *vdev);

u_int8_t wlan_vdev_is_sta(struct wlan_objmgr_vdev *vdev);

int wlan_vdev_get_apcap(struct wlan_objmgr_vdev *vdev, wlan_ap_cap *apcap);

int wlan_vdev_add_acl_validity_timer(struct wlan_objmgr_vdev *vdev,
				     const u_int8_t *mac_addr,
				     u_int16_t validity_timer);

int wlan_vdev_acs_start_scan_report(struct wlan_objmgr_vdev *vdev , int val);

u_int8_t  wlan_vdev_get_chwidth(struct wlan_objmgr_vdev *vdev);

bool wlan_vdev_is_deleted_set(struct wlan_objmgr_vdev *vdev);
bool wlan_vdev_acl_is_probe_wh_set(struct wlan_objmgr_vdev *vdev,
				   const u_int8_t *mac_addr,
				   u_int8_t probe_rssi);
void wlan_vdev_acl_override_rssi_thresholds(struct wlan_objmgr_vdev *vdev,
					    const u_int8_t *mac_addr,
					    u_int32_t *inact_rssi_low,
					    u_int32_t *inact_rssi_high,
					    u_int32_t *low_rssi,
					    u_int32_t *low_rate_rssi,
					    u_int32_t *high_rate_rssi);
#if QCA_SUPPORT_SON
bool wlan_vdev_acl_is_drop_mgmt_set(struct wlan_objmgr_vdev *vdev,
                                    const u_int8_t *mac_addr);
struct mlme_external_tx_ops * wlan_mlme_register_tx_ops(void);
#endif
struct wlan_objmgr_vdev * wlan_get_vdev_from_netdev(struct net_device *dev);

int wlan_node_get_capability(struct wlan_objmgr_peer *peer,
			     struct wlan_peer_cap *cap);

int wlan_vdev_get_sec20chan_freq_mhz(struct wlan_objmgr_vdev *vdev,
        uint16_t *sec20chan_freq);

uint32_t wlan_pdev_in_gmode(struct wlan_objmgr_pdev *pdev);

uint32_t wlan_pdev_in_amode(struct wlan_objmgr_pdev *pdev);

int wlan_pdev_get_esp_info(struct wlan_objmgr_pdev *pdev,
			   wlan_esp_data *esp_info);

int wlan_pdev_get_multi_ap_opclass(struct wlan_objmgr_pdev *pdev, struct wlan_radio_basic_capabilities *apcap,
                                   struct wlan_op_chan *op_chan,
                                   struct wlan_op_class *op_class,
                                   bool dfs_required);

void wlan_pdev_get_supp_opclass_list(struct wlan_objmgr_pdev *pdev,
                                     uint8_t *opclass_list,
                                     uint8_t *num_supp_op_class,
                                     bool global_tbl_lookup);

void wlan_vdev_get_curr_chan_and_opclass(struct wlan_objmgr_vdev *vdev,
                                         uint8_t *chan_num,
                                         uint8_t *opclass);
/**
 * wlan_get_bw_and_behav_limit() - get channel width and behav limit
 * Needed in regulatory hence make non static
 * @chan: ieee channel struct
 * @chan_width: channel width
 * @behav_limit: behavior limit
 * Return: Void.
 */
void wlan_get_bw_and_behav_limit(struct ieee80211_ath_channel *chan,
                                 uint16_t *chan_width,
                                 uint16_t *behav_limit);

int wlan_pdev_get_current_chan_txpower(struct wlan_objmgr_pdev *pdev);

uint32_t wlan_node_get_peer_chwidth(struct wlan_objmgr_peer *peer);

void wlan_node_get_peer_phy_info(struct wlan_objmgr_peer *peer, uint8_t *rxstreams, uint8_t *streams, uint8_t *cap, uint32_t *mode, uint32_t *ext_mode);
#define RXMIC_OFFSET 8

void wlan_set_peer_rate_legacy(struct wlan_objmgr_peer *peer,
                                struct ieee80211_rateset *rates);
void wlan_set_peer_rate_ht(struct wlan_objmgr_peer *peer,
           struct ieee80211_rateset *rates);
void wlan_set_peer_rate_vht(struct wlan_objmgr_peer *peer,
           u_int16_t vhtrate_map);
void wlan_peer_dump_rates(struct wlan_objmgr_peer *peer);
void wlan_deliver_mlme_evt_disassoc(struct wlan_objmgr_vdev *vdev, u_int8_t *stamac,
                                    struct wlan_objmgr_peer *peer, uint8_t is_ml_link_peer_del);

int wlan_add_client(struct wlan_objmgr_vdev *vdev,
                    struct wlan_objmgr_peer *peer,
                    struct wlan_addclient_peer_caps *peer_caps,
                    struct wlan_addclient_ml_info *ml_info);
u_int16_t wlan_get_aid(struct wlan_objmgr_peer *peer);
int wlan_is_aid_set(struct wlan_objmgr_vdev *vdev, u_int16_t associd);
void wlan_set_aid(struct wlan_objmgr_vdev *vdev, u_int16_t associd,
                  struct wlan_objmgr_peer *peer);
void wlan_clear_aid(struct wlan_objmgr_vdev *vdev, u_int16_t associd,
                    struct wlan_objmgr_peer *peer);
void wlan_set_peer_aid(struct wlan_objmgr_peer *peer, u_int16_t associd);
void wlan_set_peer_flag(struct wlan_objmgr_peer *peer, int flag);
void wlan_clear_peer_flag(struct wlan_objmgr_peer *peer, int flag);
int wlan_peer_is_flag_set(struct wlan_objmgr_peer *peer, int flag);
int wlan_get_peer_num_streams(struct wlan_objmgr_peer *peer);
void wlan_peer_auth(struct wlan_objmgr_vdev *vdev, u_int8_t *stamac,
                u_int32_t authorize);
void wlan_peer_mlme_recv_assoc_request(struct wlan_objmgr_peer *peer, int newassoc,
                                       struct wlan_addclient_ml_info *ml_info);
int wlan_vdev_del_key(struct wlan_objmgr_vdev *vdev, u_int16_t keyix,
                u_int8_t *macaddr);
int wlan_vdev_set_key(struct wlan_objmgr_vdev *vdev, u_int8_t *macaddr,
            u_int8_t cipher, u_int16_t keyix, u_int32_t keylen,
            u_int8_t *keydata);
u_int32_t wlan_ucfg_get_maxphyrate(struct wlan_objmgr_vdev *vdev);
int wlan_vdev_get_curmode(struct wlan_objmgr_vdev *vdev);
uint8_t wlan_get_opclass(struct wlan_objmgr_vdev *vdev);
uint8_t wlan_get_prim_chan(struct wlan_objmgr_vdev *vdev);
uint8_t wlan_get_seg1(struct wlan_objmgr_vdev *vdev);
uint16_t wlan_peer_get_beacon_interval(struct wlan_objmgr_peer *peer);

QDF_STATUS wlan_vdev_get_elemid(struct wlan_objmgr_vdev *vdev,
				ieee80211_frame_type ftype, uint8_t *iebuf,
				uint32_t *ielen, uint32_t elem_id);

extern uint8_t phymode2convphymode[IEEE80211_MODE_MAX];
int wlan_peer_is_extflag_set(struct wlan_objmgr_peer *peer, int flag);
void wlan_set_peer_extflag(struct wlan_objmgr_peer *peer, int flag);
void wlan_clear_peer_extflag(struct wlan_objmgr_peer *peer, int flag);

uint16_t wlan_get_wlan_band_id_chan_to_freq(
        struct wlan_objmgr_pdev *pdev,
        uint32_t chan, enum wlan_band_id band_id);
uint8_t wlan_get_opclass_from_channel(struct wlan_objmgr_vdev *vdev,
                                      struct ieee80211_ath_channel *channel);
void wlan_update_hifitbl(struct wlan_objmgr_vdev *vdev, void *msg, u_int32_t len, u_int32_t flag);
void wlan_set_node_peer_flag(struct wlan_objmgr_peer *peer, u_int32_t flag);
void wlan_clear_node_peer_flag(struct wlan_objmgr_peer *peer, u_int32_t flag);
bool wlan_vdev_is_delete_in_progress(struct wlan_objmgr_vdev *vdev);
struct ieee80211_ath_channel wlan_vdev_get_current_channel(
        struct wlan_objmgr_vdev *vdev,
        bool hwChan);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
void wlan_vdev_nss_ol_psta_add(struct wlan_objmgr_vdev *vdev);
void wlan_vdev_nss_ol_psta_delete(struct wlan_objmgr_vdev *vdev);
void wlan_vdev_nss_ol_set_cfg(struct wlan_objmgr_vdev *vdev, int cmd);
void wlan_nss_prep_mac_db_store_stavap(struct wlan_objmgr_vdev *vdev, uint8_t num_sta);
void wlan_nss_dbdc_process_mac_db_down(struct wlan_objmgr_vdev *vdev);
#endif
void wlan_vdev_set_powersave(struct wlan_objmgr_vdev *vdev, int val);
void wlan_vdev_set_param(struct wlan_objmgr_vdev *vdev, ieee80211_param param, u_int32_t val);
void wlan_vdev_pwrsave_force_sleep(struct wlan_objmgr_vdev *vdev, bool val);
int wlan_send_wds_cmd(struct wlan_objmgr_vdev *vdev, uint8_t val);
#if DBDC_REPEATER_SUPPORT
void wlan_legacy_dbdc_flags_get(struct wlan_objmgr_pdev *pdev, struct dbdc_flags *flags);
void wlan_max_pri_stavap_process_up(struct wlan_objmgr_vdev *vdev);
void wlan_delay_stavap_conn_process_up(struct wlan_objmgr_vdev *vdev);
void wlan_dbdc_process_mac_db_up(struct wlan_objmgr_vdev *vdev, uint8_t num_sta);
void wlan_act_update_force_cli_mcast_process_up(struct wlan_objmgr_vdev *vdev);
void wlan_max_pri_stavap_process_down(struct wlan_objmgr_vdev *vdev, bool *max_priority_stavap_disconnected);
void wlan_delay_stavap_conn_process_down(struct wlan_objmgr_vdev *vdev);
void wlan_force_cli_mcast_process_down(struct wlan_objmgr_vdev *vdev, bool *max_priority_stavap_disconnected);
#endif
void wlan_rptr_send_event(struct wlan_objmgr_vdev *vdev, uint8_t *ev_data, uint32_t ev_len, uint8_t ev_flags);
struct wlan_objmgr_vdev *
wlan_get_stavap(struct wlan_objmgr_pdev *pdev);
void
wlan_peer_disassoc(struct wlan_objmgr_peer *peer);
void
wlan_pdev_update_beacon(struct wlan_objmgr_pdev *pdev);
bool wlan_target_lithium(struct wlan_objmgr_pdev *pdev);
bool wlan_dessired_ssid_found(struct wlan_objmgr_vdev *vdev, u_int8_t *ssid, u_int8_t ssid_len);

/**
 * wlan_pdev_get_opclass_for_cur_hwmode() - Get opclass for the current
 * hw mode.
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @apcap: Pointer to wlan_radio_basic_capabilities
 *
 * Return - None
 */
void wlan_pdev_get_opclass_for_cur_hwmode(struct wlan_objmgr_pdev *pdev,
                                          wlan_radio_basic_capabilities *apcap);
int wlan_override_client_params(struct wlan_objmgr_vdev *vdev,
                                struct wlan_objmgr_peer *peer,
                                struct wlan_addclient_peer_caps *peer_caps);

/**
 * wlan_check_rpt_max_phy() - Check if the repeater_max_phy mode enabled.
 * @@pdev: Pointer to struct wlan_objmgr_pdev
 *
 * Return - Boolean value.
 */
bool wlan_check_rpt_max_phy(struct wlan_objmgr_pdev *pdev);

/**
 * wlan_pdev_get_5ghz_chan_info() - Get the 5 GHz channel information for
 * the given pdev.
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @cac_tlv: Pointer to wlan_radio_chan_list_info
 *
 * Return - None
 */
QDF_STATUS wlan_pdev_get_5ghz_chan_info(struct wlan_objmgr_pdev *pdev,
                                        wlan_radio_chan_list_info *cac_tlv);
/**
 * wlan_pdev_get_5ghz_chan_info() - Check if there are any 6 GHz channels for
 * Tx in the given power mode.
 * @vdev: Pointer to struct wlan_objmgr_vdev
 * @pwr_mode: Input power mode
 *
 * Return - True if there are channels available, else false.
 */
bool wlan_is_6g_txable_chan_available(struct wlan_objmgr_vdev *vdev,
                                      uint8_t pwr_mode);
#endif
