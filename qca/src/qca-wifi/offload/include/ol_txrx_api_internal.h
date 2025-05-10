/*
 * Copyright (c) 2016-2017,2020-2022 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary . Qualcomm Innovation Center, Inc.
 *
 * 2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/**
 * @file ol_txrx_api.h
 * @brief Definitions used in multiple external interfaces to the txrx SW.
 */

#ifndef _OL_TXRX_API_INTERNAL_H_
#define _OL_TXRX_API_INTERNAL_H_

#include "cdp_txrx_handle.h"
#include "cdp_txrx_extd_struct.h"
#include "wdi_event_api.h"

/******************************************************************************
 *
 * Control Interface (A Interface)
 *
 *****************************************************************************/

int
ol_txrx_pdev_attach_target(ol_txrx_soc_handle soc, uint8_t pdev_id);

QDF_STATUS
ol_txrx_vdev_detach(ol_txrx_soc_handle soc, uint8_t vdev_id,
		    ol_txrx_vdev_delete_cb callback, void *cb_context);

QDF_STATUS
ol_txrx_pdev_attach(ol_txrx_soc_handle soc,
                    struct cdp_pdev_attach_params *params);

QDF_STATUS
ol_txrx_pdev_detach(ol_txrx_soc_handle soc, uint8_t pdev_id, int force);

QDF_STATUS
ol_txrx_peer_detach(struct cdp_soc_t *soc, uint8_t vdev_id,
                    uint8_t *peer_mac, uint32_t flags,
                    enum cdp_peer_type peer_type);

QDF_STATUS
ol_txrx_set_monitor_mode(struct cdp_soc_t *soc, uint8_t vdev_id, uint8_t smart_monitor);

QDF_STATUS
ol_txrx_get_peer_mac_from_peer_id(
		ol_txrx_soc_handle soc,
		uint32_t peer_id, uint8_t *peer_mac);

void
ol_txrx_vdev_tx_lock(struct cdp_soc_t *soc, uint8_t vdev_id);

void
ol_txrx_vdev_tx_unlock(struct cdp_soc_t *soc, uint8_t vdev_id);

QDF_STATUS
ol_txrx_ath_getstats(struct cdp_soc_t *soc, uint8_t id, struct cdp_dev_stats *stats, uint8_t type);

QDF_STATUS
ol_txrx_set_gid_flag(struct cdp_soc_t *soc, uint8_t pdev_id, u_int8_t *mem_status,
		u_int8_t *user_position);

uint32_t
ol_txrx_fw_supported_enh_stats_version(struct cdp_soc_t *soc, uint8_t pdev_id);

QDF_STATUS
ol_txrx_if_mgmt_drain(struct cdp_soc_t *soc, uint8_t vdev_id, int force);

QDF_STATUS
ol_txrx_set_curchan(
	struct cdp_soc_t *soc, uint8_t pdev_id,
	uint32_t chan_mhz);

QDF_STATUS
ol_txrx_set_privacy_filters(struct cdp_soc_t *soc, uint8_t vdev_id,
			 void *filter, uint32_t num);

/******************************************************************************
 * Data Interface (B Interface)
 *****************************************************************************/
QDF_STATUS
ol_txrx_vdev_register(struct cdp_soc_t *soc, uint8_t vdev_id,
			 ol_osif_vdev_handle osif_vdev,
			 struct ol_txrx_ops *txrx_ops);

int
ol_txrx_mgmt_send(
	struct cdp_soc_t *soc, uint8_t vdev_id,
	qdf_nbuf_t tx_mgmt_frm,
	uint8_t type);

int
ol_txrx_mgmt_send_ext(struct cdp_soc_t *soc, uint8_t vdev_id,
			 qdf_nbuf_t tx_mgmt_frm,
			 uint8_t type, uint8_t use_6mbps, uint16_t chanfreq);

QDF_STATUS
ol_txrx_mgmt_tx_cb_set(struct cdp_soc_t *soc, uint8_t pdev_id,
			 uint8_t type,
			 ol_txrx_mgmt_tx_cb download_cb,
			 ol_txrx_mgmt_tx_cb ota_ack_cb, void *ctxt);

QDF_STATUS
ol_txrx_data_tx_cb_set(struct cdp_soc_t *soc, uint8_t vdev_id,
		 ol_txrx_data_tx_cb callback, void *ctxt);

/******************************************************************************
 * Statistics and Debugging Interface (C Inteface)
 *****************************************************************************/

int
ol_txrx_aggr_cfg(struct cdp_soc_t *soc, uint8_t vdev_id,
			 int max_subfrms_ampdu,
			 int max_subfrms_amsdu);

int
ol_txrx_fw_stats_get(
	 struct cdp_soc_t *soc, uint8_t vdev_id,
	 struct ol_txrx_stats_req *req,
	 bool per_vdev,
	 bool response_expected);

int
ol_txrx_debug(struct cdp_soc_t *soc, uint8_t vdev_id, int debug_specs);

QDF_STATUS
ol_txrx_fw_stats_cfg(
	 struct cdp_soc_t *soc, uint8_t vdev_id,
	 uint8_t cfg_stats_type,
	 uint32_t cfg_val);

void ol_txrx_print_level_set(unsigned level);

void *
ol_txrx_get_vdev_from_vdev_id(uint8_t vdev_id);

int
ol_txrx_mempools_attach(ol_txrx_soc_handle dp_soc);
int
ol_txrx_set_filter_neighbour_peers(
	struct cdp_pdev *pdev,
	u_int32_t val);
/**
 * @brief set the safemode of the device
 * @details
 *  This flag is used to bypass the encrypt and decrypt processes when send and
 *  receive packets. It works like open AUTH mode, HW will treate all packets
 *  as non-encrypt frames because no key installed. For rx fragmented frames,
 *  it bypasses all the rx defragmentaion.
 *
 * @param vdev - the data virtual device object
 * @param val - the safemode state
 * @return - void
 */

void
ol_txrx_set_safemode(
	struct cdp_vdev *vdev,
	u_int32_t val);
/**
 * @brief configure the drop unencrypted frame flag
 * @details
 *  Rx related. When set this flag, all the unencrypted frames
 *  received over a secure connection will be discarded
 *
 * @param vdev - the data virtual device object
 * @param val - flag
 * @return - void
 */
void
ol_txrx_set_drop_unenc(
	struct cdp_vdev *vdev,
	u_int32_t val);


/**
 * @brief set the Tx encapsulation type of the VDEV
 * @details
 *  This will be used to populate the HTT desc packet type field during Tx
 *
 * @param vdev - the data virtual device object
 * @param val - the Tx encap type
 * @return - void
 */
void
ol_txrx_set_tx_encap_type(
	struct cdp_vdev *vdev,
	u_int32_t val);

/**
 * @brief set the Rx decapsulation type of the VDEV
 * @details
 *  This will be used to configure into firmware and hardware which format to
 *  decap all Rx packets into, for all peers under the VDEV.
 *
 * @param vdev - the data virtual device object
 * @param val - the Rx decap mode
 * @return - void
 */
void
ol_txrx_set_vdev_rx_decap_type(
	struct cdp_vdev *vdev,
	u_int32_t val);

/**
 * @brief get the Rx decapsulation type of the VDEV
 *
 * @param vdev - the data virtual device object
 * @return - the Rx decap type
 */
enum htt_cmn_pkt_type
ol_txrx_get_vdev_rx_decap_type(struct cdp_vdev *vdev);

/* Is this similar to ol_txrx_peer_state_update() in MCL */
/**
 * @brief Update the authorize peer object at association time
 * @details
 *  For the host-based implementation of rate-control, it
 *  updates the peer/node-related parameters within rate-control
 *  context of the peer at association.
 *
 * @param cdp_soc - pointer to dp soc
 * @param vdev_id - id of vdev handle
 * @param peer_mac - peer mac address
 * @authorize - either to authorize or unauthorize peer
 *
 * @return QDF_STATUS
 */
QDF_STATUS
ol_txrx_peer_authorize(struct cdp_soc_t *cdp_soc, uint8_t vdev_id,
                       uint8_t *peer_mac, u_int32_t authorize);

/* Should be ol_txrx_ctrl_api.h */
void ol_txrx_set_mesh_mode(struct cdp_vdev *vdev, u_int32_t val);

void ol_tx_flush_buffers(struct cdp_soc_t *soc, uint8_t vdev_id);

A_STATUS
wdi_event_sub(struct cdp_soc_t *soc, uint8_t pdev_id,
    wdi_event_subscribe *event_cb_sub_handle,
    uint32_t event);

A_STATUS
wdi_event_unsub(struct cdp_soc_t *soc, uint8_t pdev_id,
    wdi_event_subscribe *event_cb_sub_handle,
    uint32_t event);

uint16_t ol_get_peer_mac_list(ol_txrx_soc_handle soc, uint8_t vdev_id,
                              u_int8_t newmac[][QDF_MAC_ADDR_SIZE],
                              uint16_t mac_cnt, bool limit);
/* Should be a function pointer in ol_txrx_osif_ops{} */
#if ATH_MCAST_HOST_INSPECT
/**
 * @brief notify mcast frame indication from FW.
 * @details
 *      This notification will be used to convert
 *      multicast frame to unicast.
 *
 * @param pdev - handle to the ctrl SW's physical device object
 * @param vdev_id - ID of the virtual device received the special data
 * @param msdu - the multicast msdu returned by FW for host inspect
 */

int ol_mcast_notify(struct cdp_pdev *pdev,
	u_int8_t vdev_id, qdf_nbuf_t msdu);
#endif


/* Need to remove the "req" parameter */
/* Need to rename the function to reflect the functionality "show" / "display"
 * WIN -- to figure out whether to change OSIF to converge (not an immediate AI)
 * */
#ifdef WLAN_FEATURE_FASTPATH
int ol_txrx_host_stats_get(
	struct cdp_soc_t *soc, uint8_t vdev_id,
	struct ol_txrx_stats_req *req);

QDF_STATUS
ol_txrx_host_ce_stats(struct cdp_soc_t *soc, uint8_t vdev_id);

int
ol_txrx_stats_publish(struct cdp_soc_t *soc, uint8_t pdev_id, struct cdp_stats_extd *buf);
QDF_STATUS
ol_txrx_get_vdev_stats(struct cdp_soc_t *soc, uint8_t vdev_id, void *buf, bool is_aggr);
/**
 * @brief Enable enhanced stats functionality.
 *
 * @param soc - the soc object
 * @param pdev_id - id of the physical device object
 * @return - QDF_STATUS
 */
QDF_STATUS
ol_txrx_enable_enhanced_stats(struct cdp_soc_t *soc, uint8_t pdev_id);

/**
 * @brief Disable enhanced stats functionality.
 *
 * @param soc - the soc object
 * @param pdev_id - id of the physical device object
 * @return - QDF_STATUS
 */
QDF_STATUS
ol_txrx_disable_enhanced_stats(struct cdp_soc_t *soc, uint8_t pdev_id);

#endif /* WLAN_FEATURE_FASTPATH*/

#if PEER_FLOW_CONTROL
QDF_STATUS
ol_txrx_per_peer_stats(struct cdp_soc_t *soc, uint8_t *addr);
#endif

QDF_STATUS ol_txrx_reset_monitor_mode(struct cdp_soc_t *soc,
                                      uint8_t pdev_id,  u_int8_t val);

#endif /* _OL_TXRX_API_INTERNAL_H_ */
