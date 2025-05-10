/*
 * Copyright (c) 2017-2021, 2023-2024 Qualcomm Innovation Center, Inc.
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

#ifndef OL_IF_PDEV_H
#define OL_IF_PDEV_H

#include <hif.h>
#include <qdf_trace.h>
#include <qdf_types.h>
#include <wmi_unified_api.h>
#include <wlan_lmac_if_def.h>
#include <wlan_lmac_if_api.h>
#include <cdp_txrx_ctrl.h>
#include <wlan_global_lmac_if_api.h>
#include <init_deinit_lmac.h>
#include <target_if.h>
#ifdef CONFIG_SAWF_DEF_QUEUES
#include "wlan_sawf.h"
#endif

#if UMAC_SUPPORT_CFG80211
#include <ieee80211_cfg80211.h>
#endif /* UMAC_SUPPORT_CFG80211 */

#define MAX_IE_SIZE 512
#define MAX_HT_IE_LEN 32
#define MAX_VHT_IE_LEN 32

/**
 * ol_ath_pdev_set_param() - Sends pdev parameters to firmware via wmi layer
 * @pdev: pointer to pdev object
 * @param_id: pdev param id
 * @param_value: parameter value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_set_param(struct wlan_objmgr_pdev *pdev,
			  wmi_conv_pdev_params_id param_id,
			  uint32_t param_value);

/**
 * ol_ath_pdev_dfs_phyerr_offload_en() - Send dfs phyerr offload enable
 * command to Firmware
 * @pdev: pointer to pdev object
 *
 * In full-offload mode, after wifi is brought down and brought up,
 * the phyerror offload enable command (phyerr_offload_en_cmd) should
 * be resent since the firmware gets reloaded.
 * The command is to process the dfs pulses in the firmware and to
 * send the radar-found event to the host
 *
 * Return: None
 */
void ol_ath_pdev_dfs_phyerr_offload_en(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_pdev_fips() - Send pdev fips config parameters to firmware
 * @pdev: pdev object
 * @key: Pointer to fips key
 * @key_len: length of key
 * @data: Pointer to data buf
 * @data_len: length of data buf
 * @mode: mode
 * @op: operation
 * @pdev_id: pdev_id for identifying the MAC
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_fips(struct wlan_objmgr_pdev *pdev, uint8_t *key,
		     uint32_t key_len, uint8_t *data, uint32_t data_len,
		     uint32_t mode, uint32_t op, uint32_t pdev_id);

int ol_ath_set_protocol_tagging(struct wlan_objmgr_pdev *pdev,
                                RX_PKT_TAG_OPCODE_TYPE opcode,
                                RX_PKT_TAG_RECV_PKT_TYPE pkt_type,
                                uint32_t metadata);

#if QCA_MANUAL_TRIGGERED_ULOFDMA
/**
 * ol_ath_pdev_man_trig_ulofdma_init() - manual ulofdma trig pdev init
 * @pdev: pdev object
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_man_trig_ulofdma_init(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_pdev_man_trig_ulofdma_deinit() - manual ulofdma trig pdev deinit
 * @pdev: pdev object
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_man_trig_ulofdma_deinit(struct wlan_objmgr_pdev *pdev);
#endif

/**
 * ol_ath_pdev_fips_extend() - Send pdev fips extend config parameters to firmware
 * @pdev_id: pdev_id for identifying the MAC
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_fips_extend(struct wlan_objmgr_pdev *pdev,
			    struct ath_fips_extend_cmd *fips_buf,
			    uint32_t pdev_id);

/**
 * ol_ath_fips_mode_set() - Send pdev fips mode config parameters to firmware
 * @pdev_id: pdev_id for identifying the MAC
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_fips_mode_set(struct wlan_objmgr_pdev *pdev,
			      uint32_t pdev_id, uint32_t mode);

/**
 * ol_ath_set_ht_vht_ies() - Set ht vht ie information
 * @ni: Node information
 *
 * Return: none
 */
void ol_ath_set_ht_vht_ies(struct ieee80211_node *ni);

/**
 * ol_ath_set_rxfilter() - Set Rx Filter value
 * @pdev: pdev object
 * @filter: Rx filter value
 *
 * Return: none
 */
void ol_ath_set_rxfilter(struct wlan_objmgr_pdev *pdev, uint32_t filter);

/**
 * ol_ath_setTxPowerLimit() - Set Tx Power limit value
 * @pdev: pdev object
 * @limit: tx power limit
 * @tpcInDb: tx power in db
 * @is2GHz: if 2GHz band
 *
 * Return: none
 */
void ol_ath_setTxPowerLimit(struct wlan_objmgr_pdev *pdev, uint32_t limit,
			    uint16_t tpcInDb, uint32_t is2GHz);

/**
 * ol_ath_setmfpqos() - set hw mfp qos feature
 * @pdev: pdev object
 * @dot11w: the feature to set
 *
 * Return: none
 */
void ol_ath_setmfpQos(struct wlan_objmgr_pdev *pdev, uint32_t dot11w);

#if ATH_OL_FAST_CHANNEL_RESET_WAR
#define DISABLE_FAST_CHANNEL_RESET 1
/**
 * ol_ath_fast_chan_change() - Disables fast channel reset
 * @pdev: pdev object
 *
 * Return: none
 */
void ol_ath_fast_chan_change(struct wlan_objmgr_pdev *pdev);
#endif

/**
 * ol_ath_set_mgmt_retry_limit() - Set mgmt frame retry limit
 * @pdev: pdev object
 * @limit: retry limit value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_mgmt_retry_limit(struct wlan_objmgr_pdev *pdev, uint8_t limit);

/**
 * ol_ath_set_default_pcp_tid_map() -Set pcp to tid mapping
 * @pdev: pdev object
 * @pcp: pcp value
 * @tid: tid value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_default_pcp_tid_map(struct wlan_objmgr_pdev *pdev, uint32_t pcp,
				   uint32_t tid);

/**
 * ol_ath_set_default_tidmap_prty() -Set tid map priority
 * @pdev: pdev object
 * @val: tid map priority value
 *
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_default_tidmap_prty(struct wlan_objmgr_pdev *pdev, uint32_t val);

/**
 * ol_scan_set_chan_list() -Set channel list
 * @pdev: pdev object
 * @arg: scan list parameters
 *
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_scan_set_chan_list(struct wlan_objmgr_pdev *pdev, void *arg);

/**
 * ol_ath_fill_umac_legacy_chanlist() -Set legacy channel list
 * @pdev: pdev object
 * @curr_chan_list: Channel list
 *
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_ath_fill_umac_legacy_chanlist(struct wlan_objmgr_pdev *pdev,
                                            struct regulatory_channel *curr_chan_list);


/**
 * ol_ath_set_wait_for_init_cc_response_event() - Set the event waiting for
 * init_cc response thereby waking it up.
 * @pdev: pdev object
 * @status: status of the init_cc handler.
 *
 * Return: void
 */
void ol_ath_set_wait_for_init_cc_response_event(struct wlan_objmgr_pdev *pdev,
                                                QDF_STATUS status);

/**
 * ol_ath_extract_netdev() - Extract netdev from pdev
 * @pdev: pdev object
 *
 * Return: netdev
 */
qdf_netdev_t ol_ath_extract_netdev(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_pdev_is_80p80_supported() - Check if the device supports 80p80 channels.
 * @pdev: pdev object
 *
 * Return: true if device supports, false if it doesn't.
 */
bool ol_ath_pdev_is_80p80_supported(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_is_freq_80p80_supported() - Check if the given freq
 * supports 80p80 channel mode.
 * @pdev: pdev object
 * @primary_freq: Primary frequency in MHz
 *
 * Return: true if freq supports 80p80, false if it doesn't.
 */
bool ol_ath_is_freq_80p80_supported(struct wlan_objmgr_pdev *pdev,
                                    qdf_freq_t primary_freq);
/**
 * ol_ath_fill_afc_min_psd() - Fill min_psd from afc response for the given
 * primary_freq, cen320, bw and puncturing pattern
 *
 * @pdev: Pointer to struct wlan_objmgr_pdev
 * @primary_freq: Primary frequency in MHz
 * @cen320: 320 MHz center frequency in MHz
 * @punc_pattern: Puncturing pattern
 * @bw: Channel BW in MHz
 * @min_psd: Pointer to min_psd which must be filled
 *
 * Return : QDF_STATUS
 */
#if defined(CONFIG_AFC_SUPPORT) && defined(WLAN_FEATURE_11BE)
QDF_STATUS ol_ath_fill_afc_min_psd(struct wlan_objmgr_pdev *pdev,
                                   qdf_freq_t primary_freq,
                                   qdf_freq_t cen320,
                                   uint16_t punc_pattern,
                                   uint16_t bw,
                                   int16_t *min_psd);
#else
static inline
QDF_STATUS ol_ath_fill_afc_min_psd(struct wlan_objmgr_pdev *pdev,
                                   qdf_freq_t primary_freq,
                                   qdf_freq_t cen320,
                                   uint16_t punc_pattern,
                                   uint16_t bw,
                                   int16_t *min_psd)
{
    return QDF_STATUS_SUCCESS;
}
#endif

/**
 * ol_ath_trigger_acs_for_afc() - Call ACS to choose a channel for AFC
 * @pdev: pdev object
 *
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_ath_trigger_acs_for_afc(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_trigger_update_channel_list() - Set power mode and update channel list
 * @pdev: pdev object
 *
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_ath_trigger_update_channel_list(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_set_duration_based_tx_mode_select() -Set Tx Mode Selection
 * @pdev: pdev object
 * @tx_mode_select_enable: Flag to set tx mode select
 * Return: void
 */
void ol_ath_set_duration_based_tx_mode_select(struct wlan_objmgr_pdev *pdev,
                                              uint32_t tx_mode_select_enable);

/**
 * ol_ath_enable_low_latency_mode() - Enable low latency mode
 * @pdev: pdev object
 * @enable_low_latency_mode: Flag to enable low latency mode
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_ath_enable_low_latency_mode(struct wlan_objmgr_pdev *pdev,
                                    uint32_t enable_low_latency_mode);

#if CONFIG_SAWF_DEF_QUEUES
/*
 * ol_ath_pdev_set_rate_upper_cap() - Enable/Disable rate upper cap params -
 * NSS and MCS
 * @pdev: pdev object
 * @nss: Max NSS
 * @mcs: Max MCS
 * @en_nss_cap: Enable/disable NSS upper cap
 * @en_mcs_cap: Enable/disable MCS upper cap
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_set_rate_upper_cap(struct wlan_objmgr_pdev *pdev,
                                   uint8_t nss, uint8_t mcs,
                                   uint8_t en_nss_cap, uint8_t en_mcs_cap);

/*
 * ol_ath_pdev_get_rate_upper_cap() - Get max NSS and MCS
 * @pdev: pdev object
 * @nss: Max NSS
 * @mcs: Max MCS
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_get_rate_upper_cap(struct wlan_objmgr_pdev *pdev,
                                   uint8_t *nss, uint8_t *mcs);

/*
 * ol_ath_pdev_set_rate_retry_mcs_drop() - Enable/Disable rate retry threshold
 * and number of mcs to drop
 * @pdev: pdev object
 * @retry_thresh: retry threshold
 * @mcs_drop: number of mcs to drop
 * @en_retry_thresh: enable retry threshold
 * @en_mcs_drop: enable mcs drop value
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_set_rate_retry_mcs_drop(struct wlan_objmgr_pdev *pdev,
                                        uint8_t retry_thresh, uint8_t mcs_drop,
                                        uint8_t en_retry_thresh,
                                        uint8_t en_mcs_drop);

/*
 * ol_ath_pdev_get_rate_retry_mcs_drop() - Get retry threhold and mcs drop
 * @pdev: pdev object
 * @retry_thresh: retry threshold
 * @mc_drop: mcs drop value
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_get_rate_retry_mcs_drop(struct wlan_objmgr_pdev *pdev,
                                        uint8_t *retry_thresh,
                                        uint8_t *mcs_drop);
/*
 * ol_ath_pdev_set_mcs_probe_intvl() - Set MCS probe interval
 * @pdev: pdev object
 * @min: min probe interval
 * @max: max probe interval
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_set_mcs_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t min, uint16_t max);

/*
 * ol_ath_pdev_get_mcs_probe_intvl() - Get MCS probe interval
 * @pdev: pdev object
 * @min: min probe interval
 * @max: max probe interval
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_get_mcs_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t *min, uint16_t *max);

/*
 * ol_ath_pdev_set_nss_probe_intvl() - Set NSS probe interval
 * @pdev: pdev object
 * @min: min probe interval
 * @max: max probe interval
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_set_nss_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t min, uint16_t max);

/*
 * ol_ath_pdev_get_nss_probe_intvl() - Get NSS probe interval
 * @pdev: pdev object
 * @min: min probe interval
 * @max: max probe interval
 * Return: 0 if success, other value if failure
 */
int ol_ath_pdev_get_nss_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t *min, uint16_t *max);

/*
 * ol_ath_set_sawf_params() - Configure service class parameters
 * @pdev: pdev object
 * @param: service class parameters
 * Return: 0 if success, other value if failure
 */
int ol_ath_set_sawf_params(struct wlan_objmgr_pdev *pdev,
                           struct wlan_sawf_svc_class_params *param);

/*
 * @svc_id: service class ID
 * Return: 0 if success, other value if failure
 */

int ol_ath_disable_sawf(uint8_t svc_id);
#endif

#if QCA_STANDALONE_SOUNDING_TRIGGER
/**
 * ol_ath_pdev_host_trig_txbf_sounding_init() - TxBF standalone sounding host
 *                                              trigger pdev init
 * @pdev: pointer to pdev object
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS
ol_ath_pdev_host_trig_txbf_sounding_init(struct wlan_objmgr_pdev *pdev);

/**
 * ol_ath_pdev_host_trig_txbf_sounding_deinit() - TxBF standalone sounding host
 *                                              trigger pdev deinit
 * @pdev: pointer to pdev object
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS
ol_ath_pdev_host_trig_txbf_sounding_deinit(struct wlan_objmgr_pdev *pdev);
#endif /* QCA_STANDALONE_SOUNDING_TRIGGER */

/**
 * ol_ath_get_opclass_tbl_idx() - Get opclass table index value
 * @pdev: pdev object
 * @opclass_tbl_idx: opclass table index value
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_ath_get_opclass_tbl_idx(struct wlan_objmgr_pdev *pdev,
                                      uint8_t *opclass_tbl_idx);

/**
 * ol_ath_end_r2p_table_update_wait() - End the wait triggered for the response
 * event of rate to power table update.
 * @psoc: psoc object
 * @pdev_id: pdev_id received from target
 *
 * Return: QDF_STATUS_SUCCESS if success, other qdf status if failure
 */
QDF_STATUS ol_ath_end_r2p_table_update_wait(struct wlan_objmgr_psoc *psoc,
                                            uint32_t pdev_id);

/*
 * ol_ath_pdev_set_disable_mode() - Disable schd modes
 * @pdev: pdev object
 * @disabled_modes : modes to be disable
 * Return: 0 if success other value if it is failure
 */
int ol_ath_pdev_set_disable_mode(struct wlan_objmgr_pdev *pdev,
				 uint32_t disabled_modes);

/*
 * ol_ath_pdev_set_peer_disable_mode() - Disable peer schd mode
 * @pdev_wmi_handle: pdev wmi handle
 * @peer_mac: peer mac address
 * @pdev_id: pdev id
 * @disabled_modes : modes to be disable
 *
 * Return: 0 if success other value if it is failure
 */
int ol_ath_pdev_set_peer_disable_mode(struct wmi_unified *pdev_wmi_handle,
				      uint8_t *peer_mac,
				      uint8_t pdev_id, uint32_t disabled_modes);

/*
 * ol_ath_set_tx_pkt_cap_custom_classify() - Configure TX Packet Cap
 * @pdev: pdev object
 * @pkt_type_bitmap: Protocols to be configured
 *
 * Return: 0 if success other value if it is failure
 */
int ol_ath_set_tx_pkt_cap_custom_classify(struct wlan_objmgr_pdev *pdev,
                                          uint32_t pkt_type_bitmap);
#endif /* OL_IF_PDEV_H */
