/*
 * Copyright (c) 2017-2021 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

/* This file implements all public function api for WIFI SON */
#include <wlan_son_pub.h>
#include "wlan_son_internal.h"
#include <wlan_son_utils_api.h>
#include <ath_band_steering.h>
#include "ald_netlink.h"
#include <wlan_repeater_api.h>
#include "qdf_types.h"
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_peer.h>
#endif

u_int32_t g_son_sta_stats_event_timer = 0;
#if QCA_SUPPORT_SON
/**
 * @brief Increment repeater count per vap.
 *
 * This is used by assoication routine based on ie detection.
 *
 * @param [in] vdev VAP for incrementing repeater count.
 *
 * @param [inout] void .
 */

void son_repeater_cnt_inc(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;
	vdev_priv = wlan_son_get_vdev_priv(vdev);

	vdev_priv->iv_connected_REs++;
	return;
}

/**
 * @brief Decrement repeater count per vap.
 *
 * This is used by assoication routine based on ie detection.
 *
 * @param [in] vdev VAP for incrementing repeater count.
 *
 * @param [inout] void .
 */

void son_repeater_cnt_dec(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;
	vdev_priv = wlan_son_get_vdev_priv(vdev);

	vdev_priv->iv_connected_REs--;
	return;
}

/**
 * @brief Store uplink bssid for SON.
 *
 * This is used by association routine to store bssid upon association.
 *
 * @param [in] pdev
 * @param [in] bssid to store
 *
 * @param [inout] void.
 */

void son_update_uplink_bssid(struct wlan_objmgr_pdev *pdev, char *bssid)
{

	struct son_pdev_priv *pd_priv = NULL;

	pd_priv = wlan_son_get_pdev_priv(pdev);

	qdf_mem_copy(pd_priv->uplink_bssid, bssid, MAC_ADDR_LEN);
	return ;
}

/**
 * @brief To update backhaul rate.
 *
 * This is used by uplink node detection logic.
 *
 * @param [in] vdev participating in uplink node detection.
 * @param [in] rate to be updated.
 * @param [in] bool self , update self rate if true or  uplink rate.
 *
 * @param [inout] void
 */

void son_update_backhaul_rate(struct wlan_objmgr_vdev *vdev, u_int16_t rate,
			      bool self)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	pd_priv = wlan_son_get_pdev_priv(pdev);

	if (self)
		pd_priv->serving_ap_backhaul_rate = rate;
	else
		pd_priv->uplink_rate = rate;
	return;
}

/**
 * @brief To NSS.
 *
 * This is used by uplink node detection logic.
 *
 * @param [in] vdev participating in uplink node detection.
 * @param [in] nss to be updated.
 *
 * @param [inout] void
 */

void son_update_nss(struct wlan_objmgr_vdev *vdev, u_int8_t nss)
{
    struct son_vdev_priv *vdev_priv = NULL;
    vdev_priv = wlan_son_get_vdev_priv(vdev);

    if ( vdev_priv )
        vdev_priv->nss = nss;
}

/**
 * @brief To NSS.
 *
 * This is used by uplink node detection logic.
 *
 * @param [in] vdev participating in uplink node detection.
 * @param [in] bool self , update self rate if true or  uplink rate.
 *
 * @param [inout] nss
 */

u_int8_t son_get_nss(struct wlan_objmgr_vdev *vdev)
{
    struct son_vdev_priv *vdev_priv = NULL;
    struct mlme_external_tx_ops *tx_ops = NULL;
    u_int8_t nss = 0;

    vdev_priv = wlan_son_get_vdev_priv(vdev);

    if ( !vdev_priv )
        return 0;
    tx_ops = wlan_son_get_tx_ops(vdev);
    if (!tx_ops || !tx_ops->vdev_ops) {
	    SON_LOGE("%s: tx_ops/vdev_ops is NULL ", __func__);
	    return 0;
    }
    if (tx_ops->vdev_ops(vdev, VDEV_GET_NSS, NULL, &nss) != QDF_STATUS_SUCCESS)
	    return 0;

    return nss;
}

/**
 * @brief To update uplink (backhaul link) snr
 *
 * @param [in] vdev participating in uplink node detection.
 * @param [in] snr to be updated.
 *
 * @param [inout] void
 */

void son_update_uplink_snr(struct wlan_objmgr_vdev *vdev, u_int8_t snr)
{

	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	pd_priv = wlan_son_get_pdev_priv(pdev);

	pd_priv->uplink_snr = snr;
	return;
}

/**
 * @brief Get backhaul rate.
 *
 * This is used by uplink node detection logic.
 *
 * @param [in] vdev participating in detection logic.
 * @param [in] bool self  true means for serving ap rate
 *
 * @param [inout] return rate.
 */

u_int16_t son_get_backhaul_rate(struct wlan_objmgr_vdev *vdev , bool self)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	pd_priv = wlan_son_get_pdev_priv(pdev);

	if (self)
		return pd_priv->serving_ap_backhaul_rate;
	else
		return pd_priv->uplink_rate;
}

/**
 * @brief Get connected repeater count.
 *
 * This is used by uplink node detection
 *
 * @param [in] Vdev participating.
 *
 * @param [inout] count of connected repeater.
 */

u_int8_t son_repeater_cnt_get(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;

	vdev_priv = wlan_son_get_vdev_priv(vdev);

	return vdev_priv->iv_connected_REs;
}

/**
 * @brief Enable/disable inst rssi log for son
 *
 * @param [in] enable/disable log
 *
 * @param [inout] void.
 */

void son_record_inst_rssi_log_enable (struct wlan_objmgr_vdev *vdev, int enable)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	pd_priv = wlan_son_get_pdev_priv(pdev);
	if (wlan_son_is_vdev_enabled(vdev))
	{
		if(enable)
			pd_priv->son_inst_rssi_log = true;
		else
			pd_priv->son_inst_rssi_log = false;
	}
	else
		qdf_err("SON mode not enabled for bsteerrssi_log \n");
}

/**
  * @brief provides per-user rx_stats including frame_control and qos_control
  *
  * callback api to identify the peer and derive queue_size from frame_control
  * and qos_control
  *
  * @param [in] pointer to psoc object
  * @param [in] WDI event enum value
  * @param [in] data pointer having qos stats
  * @param [in] data length
  * @param [in] status
  *
  * @param [inout] void.
  */

void son_qos_stats_update_cb(void *psoc_obj, enum WDI_EVENT event,
			     void *data, uint16_t data_len,
			     uint32_t status)
{
	//struct cdp_interface_qos_stats *stats = (struct cdp_interface_qos_stats *)data;

	//check if stats->peer_id != INVALID_PEER
	//derive peer from peer mac addr
	//derive queue_size from fc and qc checking if fc and qc fields are valid
}

/**
 * @brief Record RSSI per peer.
 *
 * @param [in] peer to update RSSI.
 * @param [in] rssi value
 *
 * @param [inout] void.
 */

void son_record_inst_peer_rssi(struct wlan_objmgr_peer *peer,
			       u_int8_t rssi)
{
	struct son_pdev_priv *pd_priv = NULL;
	struct son_peer_priv *pe_priv = NULL;
	bool generate_event = false;
	u_int8_t report_rssi = BSTEERING_INVALID_RSSI;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct mlme_external_tx_ops *tx_ops = NULL;

	if (!peer) {
		return;
	}

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	psoc = wlan_pdev_get_psoc(pdev);

	pe_priv = wlan_son_get_peer_priv(peer);

	if (!pe_priv) {
		return;
	}

	if (!wlan_son_is_vdev_enabled(vdev) ||
		!wlan_son_is_pdev_valid(pdev)) {
		return;
	}

	pd_priv = wlan_son_get_pdev_priv(pdev);

	do {
		SON_LOCK(&pd_priv->son_lock);
		if (!wlan_son_is_pdev_enabled(pdev)) {
			break;
		}

		if (!pd_priv->son_inst_rssi_inprogress ||
		    WLAN_ADDR_EQ(pd_priv->son_inst_rssi_macaddr,
				 peer->macaddr)) {
			/* The RSSI measurement is not for the one requested */
			SON_LOGI("Inst RSSI measurement for [%x] [%x] [%x] [%x] [%x] [%x] is %d",
				 pd_priv->son_inst_rssi_macaddr[0],
				 pd_priv->son_inst_rssi_macaddr[1],
				 pd_priv->son_inst_rssi_macaddr[2],
				 pd_priv->son_inst_rssi_macaddr[3],
				 pd_priv->son_inst_rssi_macaddr[4],
				 pd_priv->son_inst_rssi_macaddr[5],
				 rssi);
			break;
		}

		if (BSTEERING_INVALID_RSSI != rssi) {
			pd_priv->son_avg_inst_rssi = ((pd_priv->son_avg_inst_rssi
					       * pd_priv->son_inst_rssi_count)+
						      rssi) /
				( pd_priv->son_inst_rssi_count + 1);
			++ pd_priv->son_inst_rssi_count;
			if(pd_priv->son_inst_rssi_log)
			{
				qdf_info("Inst RSSI measurement for [%x] [%x] [%x] [%x] [%x] [%x] is %d",
					pd_priv->son_inst_rssi_macaddr[0],
					pd_priv->son_inst_rssi_macaddr[1],
					pd_priv->son_inst_rssi_macaddr[2],
					pd_priv->son_inst_rssi_macaddr[3],
					pd_priv->son_inst_rssi_macaddr[4],
					pd_priv->son_inst_rssi_macaddr[5],
					rssi);
			}
			if ( pd_priv->son_inst_rssi_count >=  pd_priv->son_inst_rssi_num_samples) {
				generate_event = true;
				report_rssi =  pd_priv->son_avg_inst_rssi;
			}
			/* It is important to update the value here as otherwise
			   the threshold crossing logic will not work properly.
			   The example scenario is:
			   1. STA is active and average RSSI is
			   above the crossing threshold.
			   2. STA becomes inactive which triggers
			   an instantaneous RSSI measuremnt, and the triggered
			   measurement is below the threshold.
			   3. Further updates to the average RSSI are above
			   the threshold
			   In this scenario, we want the RSSI update in
			   step 3 to generate an RSSIcrossing event. */
			pe_priv->son_rssi = rssi;
		} else {
			++pd_priv->son_inst_rssi_err_count;
			/* If we get twice as many failed samples
			   as the number of samplesrequested.
			   just give up and indicate a failure to
			   measure the RSSI. */
			if (pd_priv->son_inst_rssi_err_count >= 2 *
			    pd_priv->son_inst_rssi_num_samples) {
				generate_event = true;
			}
		}

		if (generate_event) {
			son_send_rssi_measurement_event(vdev, peer->macaddr,
					report_rssi, false /* is_debug */);
			qdf_timer_stop(&pd_priv->son_inst_rssi_timer);
			pd_priv->son_inst_rssi_inprogress = false;
			break;
		}
		tx_ops = wlan_son_get_tx_ops(vdev);
		if (!tx_ops || !tx_ops->peer_ops) {
			SON_LOGE("%s: tx_ops/ peer_ops is NULL ", __func__);
			break;
		}


		/* More measurements are needed */
	        if (tx_ops->peer_ops(peer, PEER_REQ_INST_STAT, NULL, NULL) != QDF_STATUS_SUCCESS) {
		        break;
                }
	} while (0);

	SON_UNLOCK(&pd_priv->son_lock);
}

/**
 * @brief Send error to user space if instantanous rssi was failed.
 *
 * This is used by rssi threshold logic in steering.
 *
 * @param [in] peer
 *
 * @param [inout] void.
 */

void son_record_inst_peer_rssi_err(struct wlan_objmgr_peer *peer)
{
	son_record_inst_peer_rssi(peer, BSTEERING_INVALID_RSSI);
}


/**
 * @brief update user space if current rate changes from last reported rate.
 *
 * This is used by rate indication logic in steering.
 *
 * @param [in] peer
 * @param [in] curent rate.
 * @param [in] last tx rate.
 *
 * @param [inout] void.
 */

void son_update_peer_rate(struct wlan_objmgr_peer *peer, u_int32_t txrate,
			  u_int32_t last_txrate)
{
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_peer_priv *pe_priv = NULL;
	u_int32_t high_threshold, low_threshold;
	BSTEERING_XING_DIRECTION xing = BSTEERING_XING_UNCHANGED;

	if (!peer) {
		return;
	}

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	if (!wlan_son_is_vdev_enabled(vdev) ||
		!wlan_son_is_pdev_enabled(pdev)) {
		return;
	}

	pd_priv = wlan_son_get_pdev_priv(pdev);
	pe_priv = wlan_son_get_peer_priv(peer);

	if (!pe_priv) {
		return;
	}

	do {
		SON_LOCK(&pd_priv->son_lock);

		if (pd_priv->son_dbg_config_params.raw_tx_rate_log_enable) {
			son_send_tx_rate_measurement_event(vdev, peer->macaddr,
							   last_txrate);
		}

		low_threshold = pd_priv->son_config_params.low_tx_rate_crossing_threshold;
		high_threshold =
			pd_priv->son_config_params.high_tx_rate_crossing_threshold[pe_priv->son_peer_class_group];

		if (!last_txrate) {
			/* First Tx rate measurement.In this case,
			   generate an event ifthe rate is above / below
			   the threshold (since there is no history to check).*/
			if (txrate < low_threshold) {
				xing = BSTEERING_XING_DOWN;
			} else if (txrate > high_threshold) {
				xing = BSTEERING_XING_UP;
			}
			break;
		}

		/* Check thresold crossings */
		if (txrate < low_threshold &&
		    last_txrate >= low_threshold) {
			xing = BSTEERING_XING_DOWN;
		} else if (txrate > high_threshold &&
			   last_txrate <= high_threshold) {
			xing = BSTEERING_XING_UP;
		}
	} while (0);

	if (xing != BSTEERING_XING_UNCHANGED) {
		son_send_tx_rate_xing_event(vdev, peer->macaddr, last_txrate,
					    xing);
	}

	SON_UNLOCK(&pd_priv->son_lock);
}

/**
 * @brief To enable or disable SON timers.
 *
 * This is used by vap create or delete functionality to enable or disable SON timers.
 *
 * @param [in] vdev.
 * @param [in] enable - enable or disable SON timers.
 */

int son_enable_disable_steering(struct wlan_objmgr_vdev *vdev,
			        bool enable)
{
    return son_core_pdev_enable_disable_steering(vdev, enable);
}

/**
 * @brief Determine whether the ACK RSSI is enabled or not for steering
 *
 * @param [in] vdev
 *
 * @return non-zero if it is enabled; otherwise 0
 */
u_int8_t son_is_ackrssi_enabled(struct wlan_objmgr_vdev *vdev)
{
        struct son_vdev_priv *vdev_priv = NULL;

        vdev_priv = wlan_son_get_vdev_priv(vdev);

        if (vdev_priv)
                return atomic_read(&vdev_priv->v_ackrssi_enabled);

        return false;

}

/**
 * @brief To check if probe response are withheld during steering.
 *
 * This is used by frame input routine to check withheld condition.
 *
 * @param [in] vdev.
 * @param [in] address 2 from mac frame.
 * @param [in] rssi of frame received.
 * @param [inout] false if not enabled otherwise true.
 */

bool son_is_probe_resp_wh_2G(struct wlan_objmgr_vdev *vdev,
			     u_int8_t *mac_addr,
			     u_int8_t sta_rssi)
{
	struct wlan_objmgr_pdev *pdev = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	if( !wlan_son_is_pdev_enabled(pdev) ||
	    !wlan_son_is_vdev_enabled(vdev))
		return false;

	return son_core_is_probe_resp_wh_2g(vdev, mac_addr, sta_rssi);
}

/** unused function kept for future reference **/
#if 0
/**
 * @brief Query the band steering module for whether it is withholding
 *        probe responses for the given MAC address on this VAP.
 *
 * @param [in] vdev  the VAP on which the probe request was received
 * @param [in] mac_addr  the MAC address of the client that sent the probe
 *                       request
 *
 * @return true if the response should be withheld; otherwise false
 */

bool son_is_probe_resp_wh(struct wlan_objmgr_vdev *vdev,
			  const u_int8_t *mac_addr, u_int8_t probe_rssi)
{
	struct wlan_objmgr_pdev *pdev = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	if( !wlan_son_is_pdev_enabled(pdev) ||
	    !wlan_son_is_vdev_enabled(vdev))
		return false;

	return wlan_vdev_acl_is_probe_wh_set(vdev, mac_addr, probe_rssi);
}
#endif

/**
 * @brief Called when firmware stats are updated for a STA, with
 *        RSSI changed and a valid Tx rate
 *
 * @param [in] ni  the node for which the stats are updated
 * @param [in] current_vdev  vdev for which stats up to this point
 *                          have been collected.  If it does not
 *                          match the VAP the current node is
 *                          on, should start message over so
 *                          each message is only STAs on a
 *                          particular VAP
 * @param [inout] sta_stats  the structure to update with STA
 *                           stats from cdp_interface peer stats
 *
 * @return true if stats are updated, interference detection is
 *         enabled on the radio, and band steering is enabled on
 *         the VAP; false otherwise
 */
bool son_update_sta_stats(
	struct wlan_objmgr_peer *peer,
	struct wlan_objmgr_vdev *current_vdev,
	struct bs_sta_stats_ind *sta_stats,
	struct cdp_interface_peer_stats *stats)
{
	struct son_pdev_priv *pd_priv = NULL;
	bool updated_stats = false;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;

	if (!peer) {
		return false;
	}

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	if (!sta_stats || !wlan_son_is_vdev_enabled(vdev) ||
		!wlan_son_is_pdev_valid(pdev)) {
		return false;
	}

	pd_priv = wlan_son_get_pdev_priv(pdev);

	do {
		SON_LOCK(&pd_priv->son_lock);
		if (!wlan_son_is_pdev_enabled(pdev)) {
			SON_UNLOCK(&pd_priv->son_lock);
			break;
		}

		if ((current_vdev && current_vdev != vdev) ||
		    sta_stats->peer_count >= BSTEERING_MAX_PEERS_PER_EVENT) {
			/* Send the current message immediately, then start over */
			son_send_sta_stats_event(current_vdev, sta_stats);
			sta_stats->peer_count = 0;
		}

		qdf_mem_copy(sta_stats->peer_stats[sta_stats->peer_count].client_addr,
				wlan_peer_get_macaddr(peer), QDF_MAC_ADDR_SIZE);
		sta_stats->peer_stats[sta_stats->peer_count].tx_rate =
			stats->last_peer_tx_rate;
		sta_stats->peer_stats[sta_stats->peer_count].tx_packet_count =
			stats->tx_packet_count;
		sta_stats->peer_stats[sta_stats->peer_count].rx_packet_count =
			stats->rx_packet_count;
		sta_stats->peer_stats[sta_stats->peer_count].tx_byte_count =
			stats->tx_byte_count;
		sta_stats->peer_stats[sta_stats->peer_count].rx_byte_count =
			stats->rx_byte_count;
		sta_stats->peer_stats[sta_stats->peer_count].rssi = stats->peer_rssi;
		sta_stats->peer_stats[sta_stats->peer_count].per = stats->per;
		sta_stats->peer_stats[sta_stats->peer_count].ack_rssi = stats->ack_rssi;
		sta_stats->peer_count++;
		SON_UNLOCK(&pd_priv->son_lock);
		updated_stats = true;
	} while (0);

	return updated_stats;
}

/**
 * @brief check if rssi seq is matching with last received frame.
 * @param [in] peer .
 * @param [in] rssi sequnece number.
 *
 * @param [inout] True if matches .
 */

int32_t son_match_peer_rssi_seq(struct wlan_objmgr_peer *peer,
				u_int32_t rssi_seq)
{
	struct son_peer_priv *pe_priv = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;

	vdev = wlan_peer_get_vdev(peer);

	if (!wlan_son_is_vdev_enabled(vdev))
		return false;

	pe_priv = wlan_son_get_peer_priv(peer);

	if (!pe_priv) {
		return -EINVAL;
	}

	if ( rssi_seq != pe_priv->son_rssi_seq) {
		pe_priv->son_rssi_seq = rssi_seq;
		return true;
	} else
		return false;
}
/**
 * @brief Check if an RSSI measurement crossed the threshold
 *
 * @param [in] peer  the node for which the RSSI measurement occurred
 * @param [in] rssi  the measured RSSI
 * @param [in] low_threshold  the lower threshold to compare with
 * @param [in] high_threshold  the upper threshold to compare with
 *
 * @return the crossing direction as enumerated in BSTEERING_XING_DIRECTION
 */
static BSTEERING_XING_DIRECTION son_check_rssi_cross_threshold(
	struct wlan_objmgr_peer *peer,
	u_int8_t rssi, u_int32_t low_threshold,
	u_int32_t high_threshold)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	u_int8_t delta = 0;
	BSTEERING_XING_DIRECTION direction = BSTEERING_XING_UNCHANGED;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;
	struct son_peer_priv *pe_priv = NULL;

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	pd_priv = wlan_son_get_pdev_priv(pdev);

	delta = pd_priv->son_rssi_xing_report_delta;
	/* In offload the averaging of RSSI value is taken care of.
	   So delta value is 0 */
	/* The RSSI value at a particular position keeps
	   toggling and the approx window size is 6.So the delta value is 2 */

	pe_priv = wlan_son_get_peer_priv(peer);

	if (!pe_priv) {
		return BSTEERING_XING_UNCHANGED;
	}

	if ((rssi < low_threshold && pe_priv->son_rssi >= low_threshold) ||
	    (rssi < high_threshold - delta && pe_priv->son_rssi >= high_threshold - delta))
		direction = BSTEERING_XING_DOWN;
	else if ((rssi > high_threshold + delta && pe_priv->son_rssi <= high_threshold + delta) ||
		 (rssi > low_threshold + delta && pe_priv->son_rssi <= low_threshold + delta))
		direction =  BSTEERING_XING_UP;

	return direction;
}

/**
 * @brief to record rssi per peer.
 *
 * This is used by steering routine to record rssi per peer.
 *
 * @param [in] peer
 * @param [in] rssi to be stored.
 *
 * @param [inout] void.
 */

void son_record_peer_rssi(struct wlan_objmgr_peer *peer, u_int8_t rssi)
{
	BSTEERING_XING_DIRECTION inact_xing = BSTEERING_XING_UNCHANGED;
	BSTEERING_XING_DIRECTION low_xing = BSTEERING_XING_UNCHANGED;
	BSTEERING_XING_DIRECTION rate_xing = BSTEERING_XING_UNCHANGED;
	BSTEERING_XING_DIRECTION ap_xing = BSTEERING_XING_UNCHANGED;
	BSTEERING_XING_DIRECTION map_xing = BSTEERING_XING_UNCHANGED;
	u_int32_t low_rate_rssi_threshold_map = 0;
	u_int32_t high_rate_rssi_threshold_map = 0;
	u_int32_t ap_steer_rssi_xing_low_threshold = 0;
	struct wlan_rssi_threshold_data in_rssi_threshold = {0};
	struct wlan_rssi_threshold_data out_rssi_threshold_data = {0};
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_peer_priv *pe_priv = NULL, *pe_priv_check = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct mlme_external_tx_ops *tx_ops = NULL;
	ieee80211_bsteering_param_t *son_config_params = NULL;
	ieee80211_bsteering_map_param_t *map_config_params = NULL;
	u_int8_t client_class_group = 0;

	if (!peer) {
		return;
	}

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	psoc = wlan_pdev_get_psoc(pdev);

	if (!wlan_son_is_vdev_enabled(vdev) ||
		!wlan_son_is_pdev_valid(pdev)) {
		return;
	}

	pd_priv = wlan_son_get_pdev_priv(pdev);
	pe_priv = wlan_son_get_peer_priv(peer);
	son_config_params = &pd_priv->son_config_params;

	if (!pe_priv) {
		return;
	}

	do {
		SON_LOCK(&pd_priv->son_lock);

		if (!wlan_son_is_pdev_enabled(pdev)) {
			break;
		}

		if (pd_priv->son_dbg_config_params.raw_rssi_log_enable) {
			son_send_rssi_measurement_event(vdev,
						peer->macaddr,
						rssi, true /* is_debug */);
		}

		if (son_config_params->client_classification_enable) {
			client_class_group = pe_priv->son_peer_class_group;
		}


		ap_steer_rssi_xing_low_threshold =
				son_config_params->ap_steer_rssi_xing_low_threshold[client_class_group];

                in_rssi_threshold.low_rssi =
                                   son_config_params->low_rssi_crossing_threshold;
                in_rssi_threshold.inact_rssi_low =
                                   son_config_params->inactive_rssi_xing_low_threshold[client_class_group];
                in_rssi_threshold.inact_rssi_high =
                                   son_config_params->inactive_rssi_xing_high_threshold[client_class_group];
                in_rssi_threshold.low_rate_rssi =
                                   son_config_params->low_rate_rssi_crossing_threshold;
                in_rssi_threshold.high_rate_rssi =
                                   son_config_params->high_rate_rssi_crossing_threshold[client_class_group];
		qdf_mem_copy(&in_rssi_threshold.mac, wlan_peer_get_macaddr(peer), QDF_MAC_ADDR_SIZE);

		tx_ops = wlan_son_get_tx_ops(vdev);
		if (!tx_ops || !tx_ops->vdev_ops) {
			SON_LOGE("%s: tx_ops/vdev_ops is NULL ", __func__);
			SON_UNLOCK(&pd_priv->son_lock);
			return;
		}

		tx_ops->vdev_ops(vdev, VDEV_GET_ACL_RSSI_THRESHOLDS, &in_rssi_threshold, &out_rssi_threshold_data);

		if (!pe_priv->son_rssi) {
		/* First RSSI measurement */
		/* Check if the RSSI starts above or below the rate threshold
		   (STA will always be considered active at startup) */
			if (rssi < out_rssi_threshold_data.low_rate_rssi) {
				rate_xing = BSTEERING_XING_DOWN;
			} else if (rssi > out_rssi_threshold_data.high_rate_rssi) {
				rate_xing = BSTEERING_XING_UP;
			}
			break;
		}

		pe_priv_check = wlan_son_get_peer_priv(peer);
		if (!pe_priv_check ) {
			SON_UNLOCK(&pd_priv->son_lock);
			return;
		}

		if(son_core_is_peer_inact(peer)) {
			/* Check inactivity rssi threshold crossing */
			inact_xing = son_check_rssi_cross_threshold(
				peer, rssi,
                                out_rssi_threshold_data.inact_rssi_low,
                                out_rssi_threshold_data.inact_rssi_high);
		} else {
			/* Check rate rssi thresold crossing */
			rate_xing = son_check_rssi_cross_threshold(
                                peer, rssi, out_rssi_threshold_data.low_rate_rssi,
                                out_rssi_threshold_data.high_rate_rssi);
		}
		/* Check low rssi thresold crossing */
		low_xing = son_check_rssi_cross_threshold(
			peer, rssi,
			out_rssi_threshold_data.low_rssi,
			BSTEER_INVALID_RSSI_HIGH_THRESHOLD);
		/* Check AP rssi threshold crossing */
		ap_xing = son_check_rssi_cross_threshold(
			peer, rssi,
			ap_steer_rssi_xing_low_threshold,
			BSTEER_INVALID_RSSI_HIGH_THRESHOLD);
	} while (0);


	if (inact_xing != BSTEERING_XING_UNCHANGED ||
	    low_xing != BSTEERING_XING_UNCHANGED ||
	    rate_xing != BSTEERING_XING_UNCHANGED ||
	    ap_xing != BSTEERING_XING_UNCHANGED) {
		son_send_rssi_xing_event(
			vdev,
			peer->macaddr, rssi,
			inact_xing, low_xing, rate_xing, ap_xing);
	}

	/* Check if MAP is set */
	map_config_params = &pd_priv->map_config_params;
	if (son_vdev_map_capability_get(vdev, SON_MAP_CAPABILITY)) {
		if (map_config_params->rssi_threshold) {
			low_rate_rssi_threshold_map = map_config_params->rssi_threshold -
							map_config_params->rssi_hysteresis;
			high_rate_rssi_threshold_map = map_config_params->rssi_threshold +
							map_config_params->rssi_hysteresis;

			if (rssi < low_rate_rssi_threshold_map) {
				map_xing = BSTEERING_XING_DOWN;
			} else if (rssi > high_rate_rssi_threshold_map) {
				map_xing = BSTEERING_XING_UP;
			}

			if (pe_priv->ni_bs_rssi_prev_xing_map != map_xing) {
				pe_priv->ni_bs_rssi_prev_xing_map = map_xing;
				son_send_rssi_xing_map_event(vdev, peer->macaddr,
							     rssi, map_xing);
			}
		}
	}

	pe_priv->son_rssi = rssi;

	SON_UNLOCK(&pd_priv->son_lock);
	return;
}
/**
 * @brief reset inactivity count variables upon assoication .
 *
 * This is used by inactivity logic in steering module.
 *
 * @param [in] peer.
 * @param [inout] void.
 */

void son_peer_authorize(struct wlan_objmgr_peer *peer)
{
	struct son_peer_priv *pe_priv = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;
	struct mlme_external_tx_ops *tx_ops = NULL;
        bool kickout_allow = false;

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	pe_priv = wlan_son_get_peer_priv(peer);
	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->peer_ops) {
		SON_LOGE("%s: tx_ops/ peer_ops is NULL ", __func__);
		return;
	}
	if( wlan_son_is_pdev_enabled(pdev)&&
	    wlan_son_is_vdev_valid(vdev)) {
		if(pe_priv) {
			pd_priv = wlan_son_get_pdev_priv(pdev);
			pe_priv->peer = peer;
			pe_priv->son_inact_flag = false;
			pe_priv->son_inact_reload =
				pd_priv->son_inact[pe_priv->son_peer_class_group];
			pe_priv->son_inact = pe_priv->son_inact_reload;
			pe_priv->son_steering_flag = false;
                        kickout_allow = true;
                        tx_ops->peer_ops(peer, PEER_SET_KICKOUT_ALLOW, &kickout_allow, NULL);
		}
	}

	return;
}

/**
 * @brief store pid per vdev for multi instances of user space daemons.
 *
 * This is used by steering infrastructure.
 *
 * @param [in] vdev
 * @param [in] pid to be stored.
 *
 * @param [inout] void.
 */

void son_set_vdev_lbd_pid(struct wlan_objmgr_vdev *vdev, u_int32_t pid)
{
	struct son_vdev_priv *vd_priv = NULL;

	vd_priv = wlan_son_get_vdev_priv(vdev);

	if (!vd_priv) {
		SON_LOGE("FAILED to updated lbd PID\n");
		return;
	}

	if (vd_priv->lbd_pid != pid) {
		SON_LOGI("Lbd Pid %d",pid);
		vd_priv->lbd_pid = pid;
	}

	return;
}

/**
 * @brief Mark peer as inactive .
 *
 * This is used by Rx path to reset inactive flag.
 *
 * @param [in] peer
 * @param [in] bool inactive flag .
 *
 * @param [inout] void.
 */


void son_mark_node_inact(struct wlan_objmgr_peer *peer, bool inactive)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;

	vdev = wlan_peer_get_vdev(peer);

	pdev = wlan_vdev_get_pdev(vdev);

	if (!wlan_son_is_pdev_enabled(pdev)) {
		return;
	}

	if(!wlan_son_is_vdev_enabled(vdev)) {
		return ;
	}

	son_core_mark_peer_inact(peer, inactive);

	return;
}

/**
 * @brief update assoc_frame
 * @param [in] peer
 * @param [in] frame
 * @param [in] frame_len
 */

void son_update_assoc_frame(struct wlan_objmgr_peer *peer, uint8_t *frame, uint16_t frame_len)
{
#define VENDOR_OUI_BYTE0 0x00
#define VENDOR_OUI_BYTE1 0x17
#define VENDOR_OUI_BYTE2 0xF2
	uint8_t *frm, *efrm, oui[3];
	struct ieee80211_frame *wh;
	struct son_peer_priv *pe_priv = NULL;
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	int subtype;

	pe_priv = wlan_son_get_peer_priv(peer);

	if (!pe_priv || !frame) {
		SON_LOGI("%s pe_priv or frame pointer is NULL \n", __func__);
		return;
	}

	pe_priv->son_peer_class_group = 0;

	vdev = wlan_peer_get_vdev(peer);
	pdev = wlan_vdev_get_pdev(vdev);
	pd_priv = wlan_son_get_pdev_priv(pdev);

	if (!pd_priv)
		return;


	if (pe_priv->assoc_frame) {
		qdf_mem_free(pe_priv->assoc_frame);
	}

	pe_priv->assoc_frame = qdf_mem_malloc(frame_len);

	if (!pe_priv->assoc_frame) {
		SON_LOGI("%s Insufficient memory to allocate for assoc frame \n",__func__);
		return;
	}
	memcpy(pe_priv->assoc_frame, frame, frame_len);
	pe_priv->assoc_frame_length = frame_len;

	if (!(pd_priv->son_config_params.client_classification_enable)) {
		return;
	}

	wh = (struct ieee80211_frame *) frame;
	subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;;
	frm = (u_int8_t *)&wh[1];
	efrm = frame + frame_len;
	frm += 4; /* Ignore Capability Information and Listen Interval */

	if (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) {
		frm += 6; /* Ignore current AP info */
	}

	while (!(pe_priv->son_peer_class_group) &&
		   (((frm + 1) < efrm) && (frm + frm[1] + 1) < efrm)) {
		switch (*frm) {
			case IEEE80211_ELEMID_VENDOR:
				if (!iswpaoui(frm) && !iswmeinfo(frm) && !(pe_priv->son_peer_class_group)) {
					oui[0] = (uint8_t) (*(frm + 2));
					oui[1] = (uint8_t) (*(frm + 3));
					oui[2] = (uint8_t) (*(frm + 4));
					if ((oui[0] == VENDOR_OUI_BYTE0) &&
						(oui[1] == VENDOR_OUI_BYTE1) &&
						(oui[2] == VENDOR_OUI_BYTE2)) {
						pe_priv->son_peer_class_group = 1;
					}
				}
			break;
			default:
			break;
		}
		frm += frm[1] + 2;
	}
#undef VENDOR_OUI_BYTE2
#undef VENDOR_OUI_BYTE1
#undef VENDOR_OUI_BYTE0
}

/**
 * @Convert chanwidth.
 *
 * @param [in] chanwidth
 *
 * @param [out] wlan_chwidth_e.
 */
wlan_chwidth_e convert_chanwidth(u_int32_t chanwidth)
{
	if (chanwidth == IEEE80211_CWM_WIDTH20)
		return wlan_chwidth_20;
	else if (chanwidth == IEEE80211_CWM_WIDTH40)
		return wlan_chwidth_40;
	else if (chanwidth == IEEE80211_CWM_WIDTH80)
		return wlan_chwidth_80;
	else if (chanwidth == IEEE80211_CWM_WIDTH160)
		return wlan_chwidth_160;
#ifdef WLAN_FEATURE_11BE
	else if (chanwidth == IEEE80211_CWM_WIDTH320)
		return wlan_chwidth_320;
#endif
	else
		return wlan_chwidth_invalid;
}

/**
 * @brief get uplink rate based on SNR.
 *
 * This is used by uplink node detection logic.
 *
 * @param [in] vdev
 * @param [in] SNR value for computaion.
 *
 * @param [inout] rate corrosponding to SNR.
 */

u_int16_t son_get_uplinkrate(struct wlan_objmgr_vdev *vdev, u_int8_t snr)
{
	u_int8_t nss = 0;
	wlan_phymode_e phymode;
	wlan_chwidth_e chwidth;
	struct mlme_external_tx_ops *tx_ops = NULL;
	u_int32_t chanwidth = 0;

	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->vdev_ops) {
		SON_LOGE("%s: tx_ops/vdev_ops is NULL ", __func__);
		return 0;
	}

	nss = son_get_nss(vdev);
	if (!nss) {
		SON_LOGE("%s: nss is Invalid ", __func__);
		return 0;
	}


	phymode = convert_phymode(vdev);
	tx_ops->vdev_ops(vdev, VDEV_GET_CHAN_WIDTH, NULL, &chanwidth);

	chwidth = convert_chanwidth(chanwidth);

	return son_SNRToPhyRateTablePerformLookup(snr, nss, phymode, chwidth);

}
/**
 * @brief: Change feature capablities per vdev.
 * @param  vaphandle   : vap handle
 * @param action: SET, GET and clear.
 * @param cap: capabilites to change.
 * @return :in case of get it return positive value if cap
 *           is set, defualt is EOK and it can return -EINVAL
 *           if vdev is null.
 *           only valid for STA vaps;
 * Function work under vdev_lock.
 */

u_int8_t son_vdev_feat_capablity(struct wlan_objmgr_vdev *vdev,
				 son_capability_action action,
				 u_int32_t cap)
{
	u_int8_t retv = EOK;
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct mlme_external_tx_ops *tx_ops = NULL;
        u_int8_t set = 0;

	if (!vdev)
		return -EINVAL;
	pdev = wlan_vdev_get_pdev(vdev);
	pd_priv = wlan_son_get_pdev_priv(pdev);

	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->pdev_ops) {
		SON_LOGE("%s: tx_ops/pdev_ops is NULL ", __func__);
		return -EINVAL;
	}

	switch(action)
	{
	case  SON_CAP_GET:
		retv = wlan_vdev_mlme_feat_cap_get(vdev, cap);
		break;
	case  SON_CAP_SET:
		wlan_vdev_mlme_feat_cap_set(vdev, cap);
		pd_priv->son_num_vap++;
		if (pd_priv->son_num_vap) {
                    set = 1;
                    tx_ops->pdev_ops(pdev, PDEV_SET_SEC_HM_WDS, &set, NULL);
		}
		break;
	case  SON_CAP_CLEAR:
		wlan_vdev_mlme_feat_cap_clear(vdev, cap);
		pd_priv->son_num_vap--;
		if (!pd_priv->son_num_vap) {
                    set = 0;
                    tx_ops->pdev_ops(pdev, PDEV_SET_SEC_HM_WDS, &set, NULL);
		}
		break;
	default:
		SON_LOGW(" Invalid action ");
	}

	return retv;
}

/**
 * Function work under vdev_lock.
 * @brief: Get the count of son enabled vdev .
 * @param  vaphandle   : vap handle
 * @param action: GET
 * @return :It returns the number of vaps for which SON is enabled
 *            default is EOK and it can return -EINVAL
 *           if vdev is null.
 *
 */
u_int8_t son_vdev_get_count(struct wlan_objmgr_vdev *vdev,
        son_capability_action action)
{
	u_int8_t retv = EOK;
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;

	if (!vdev)
		return -EINVAL;
	pdev = wlan_vdev_get_pdev(vdev);
	pd_priv = wlan_son_get_pdev_priv(pdev);

	switch(action)
	{
	case SON_CAP_GET:
		retv =  pd_priv->son_num_vap;
		break;
	default:
		SON_LOGW(" Invalid action ");
	}


	return retv;
}


/**
 * @brief: Change feature extended cap per vdev.
 * @param  vaphandle   : vap handle
 * @param action: SET, GET and clear.
 * @param cap: capabilites to change.
 * @return :in case of get it return positive value if cap
 *           is set, defualt is EOK and it can return -EINVAL
 *           if vdev is null.
 *           only valid for STA vaps;
 * Function work under vdev_lock.
 */

u_int8_t son_vdev_fext_capablity(struct wlan_objmgr_vdev *vdev,
				 son_capability_action action,
				 u_int32_t cap)
{
	u_int8_t retv = EOK;

	if (!vdev)
		return -EINVAL;

	switch(action)
	{
	case  SON_CAP_SET:
		wlan_vdev_mlme_feat_ext_cap_set(vdev, cap);
		break;
	case  SON_CAP_GET:
		retv = wlan_vdev_mlme_feat_ext_cap_get(vdev, cap);
		break;
	case  SON_CAP_CLEAR:
		wlan_vdev_mlme_feat_ext_cap_clear(vdev, cap);
		break;
	default:
		SON_LOGW(" Invalid action ");
	}

	return retv;
}

/**
 * @brief Set or Clear MAP Capability Flags per vdev
 * @param vdev handle
 * @param cap MAP capability to be set/cleared
 * @param value To Set or Clear the capability flags
 *              MAP Capability SON_MAP_VAP_TYPE is used to set/clear following
 *              capability flags by making use of specific bits of the value :
 *                  Fronthaul BSS : Set/Cleared according to Bit 5 of value
 *                  Backhaul BSS : Set/Cleared according to Bit 6 of value
 *                  Backhaul STA : Set/Cleared according to Bit 7 of value
 * @return Default is EOK
 *         -EINVAL if vdev is invalid
 */

int son_vdev_map_capability_set(struct wlan_objmgr_vdev *vdev, son_map_capability cap, int value)
{
	int retv = EOK;
	u_int8_t mbo_ie_set = true;
	u_int8_t wnm_bss_pref = 0xFF;
	u_int8_t son_map_version = 0;
	struct son_vdev_priv *vdev_priv = NULL;
	struct mlme_external_tx_ops *tx_ops = NULL;
	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if (!vdev_priv)
		return -EINVAL;

	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->vdev_ops) {
		SON_LOGE("%s: tx_ops/vdev_ops is NULL ", __func__);
		return -EINVAL;
	}

	switch(cap)
	{
	case SON_MAP_CAPABILITY:
		vdev_priv->iv_map_version = value;
		if (vdev_priv->iv_map_version >= EASYMESH_VERSION_TWO) {
			tx_ops->vdev_ops(vdev, VDEV_SET_MBO_IE_BSTM, &mbo_ie_set, NULL);
			tx_ops->vdev_ops(vdev, VDEV_SET_WNM_BSS_PREF, &wnm_bss_pref, NULL);
			son_map_version = value;
		}
		tx_ops->vdev_ops(vdev, VDEV_SET_SON_MAP_VERSION, &son_map_version, NULL);
		break;
	case SON_MAP_CAPABILITY_VAP_TYPE:
		vdev_priv->iv_mapbh = (value & MAP_BACKHAUL_BSS)?1:0;
		vdev_priv->iv_mapfh = (value & MAP_FRONTHAUL_BSS)?1:0;
		vdev_priv->iv_mapbsta = (value & MAP_BACKHAUL_STA)?1:0;
		vdev_priv->iv_mapteardown = (value & MAP_BSS_TEARDOWN)?1:0;
		vdev_priv->iv_map_r1bsta_assoc_disallow = (value & MAP2_R1_BSTA_ASSOC_DISALLOW)?1:0;
		vdev_priv->iv_map_r2above_bsta_assoc_disallow = (value & MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW)?1:0;
		break;
	case SON_MAP_CAPABILITY_VAP_UP:
		vdev_priv->iv_mapvapup = value?1:0;
		break;
	case SON_MAP_CAPABILITY_ASSOC_STATUS:
		vdev_priv->iv_mapr2_assoc_status_notify = value?1:0;
		break;
	case SON_MAP_CAPABILITY_BSTA_VLAN_ID:
		vdev_priv->iv_mapr2_sta_primary_vlan = value;
		break;
	default:
		SON_LOGW(" Invalid MAP Capability ");
	}
	return retv;
}

/**
 * @brief Get MAP Capability Flags per vdev
 * @param vdev handle
 * @param cap MAP capability to get
 * @return Value of MAP Capability, defualt is EOK
 */

int son_vdev_map_capability_get(struct wlan_objmgr_vdev *vdev, son_map_capability cap)
{
	struct son_vdev_priv *vdev_priv = NULL;
	int retv = EOK;

	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if(vdev_priv)
	{
		switch(cap)
		{
		case SON_MAP_CAPABILITY:
			retv = vdev_priv->iv_map_version;
			break;
		case SON_MAP_CAPABILITY_VAP_TYPE:
			retv = (vdev_priv->iv_mapbh ? MAP_BACKHAUL_BSS : 0) |
			       (vdev_priv->iv_mapfh ? MAP_FRONTHAUL_BSS : 0) |
			       (vdev_priv->iv_mapbsta ? MAP_BACKHAUL_STA : 0) |
			       (vdev_priv->iv_mapteardown ? MAP_BSS_TEARDOWN : 0) |
			       (vdev_priv->iv_map_r1bsta_assoc_disallow ? MAP2_R1_BSTA_ASSOC_DISALLOW : 0) |
			       (vdev_priv->iv_map_r2above_bsta_assoc_disallow ? MAP2_R2_ABOVE_BSTA_ASSOC_DISALLOW : 0);
			break;
		case SON_MAP_CAPABILITY_VAP_UP:
			retv = vdev_priv->iv_mapvapup?1:0;
			break;
		case SON_MAP_CAPABILITY_ASSOC_STATUS:
			retv = vdev_priv->iv_mapr2_assoc_status_notify?1:0;
			break;
		case SON_MAP_CAPABILITY_BSTA_VLAN_ID:
			retv = vdev_priv->iv_mapr2_sta_primary_vlan;
			break;
		default:
			SON_LOGW(" Invalid MAP Capability ");
		}
	}
	return retv;
}

/**
 * @brief Set steering in progress flag per node.
 *
 * This is used by user space to set this flag.
 *
 * @param [in] peer to set steering in progress flag.
 *
 * @param [inout] retun trued if success.
 */

bool son_is_steer_in_prog(struct wlan_objmgr_peer *peer)
{
	struct son_peer_priv *pe_priv = NULL;
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev= NULL;
	bool steer_in_prog = false;

	vdev = wlan_peer_get_vdev(peer);
	pdev = wlan_vdev_get_pdev(vdev);

	pd_priv = wlan_son_get_pdev_priv(pdev);
	pe_priv = wlan_son_get_peer_priv(peer);

	if (!pe_priv)
		return -EINVAL;

	SON_LOCK(&pd_priv->son_lock);
	steer_in_prog = pe_priv->son_steering_flag;
	SON_UNLOCK(&pd_priv->son_lock);

	return steer_in_prog;
}

/**
 * @brief Verify that the son handle is valid within the
 *        struct psoc provided.
 *
 * @param [in] psoc  the handle to the radio where the band steering state
 *                 resides
 *
 * @return true if handle is valid; otherwise false
 */

bool wlan_son_is_pdev_valid(struct wlan_objmgr_pdev *pdev)
{
	struct son_pdev_priv *pdev_priv = NULL;

	if (pdev)
		pdev_priv = wlan_son_get_pdev_priv(pdev);

	return pdev && pdev_priv;
}

/**
 * @brief Determine whether the band steering module is enabled or not.
 *
 * @param [in] ic  the handle to the radio where the band steering state
 *                 resides
 *
 * @return non-zero if it is enabled; otherwise 0
 */

u_int8_t wlan_son_is_pdev_enabled(struct wlan_objmgr_pdev *pdev)
{
	struct son_pdev_priv *pdev_priv = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_vdev *vdev_next = NULL;
	struct wlan_objmgr_pdev_objmgr *objmgr;
	qdf_list_t *vdev_list;

	if (!pdev)
	{
		qdf_err("%s:pdev is NULL ", __func__);
		return false;
	}

	if (wlan_son_is_pdev_valid(pdev))
		pdev_priv = wlan_son_get_pdev_priv(pdev);

	if (pdev_priv){
		if (atomic_read(&pdev_priv->son_enabled))
			return true;
	}

	/* band steering can be disabled when complete band is teared down
	 * which is possible in MAP.
	 * Add check to see if VAP is up on any band and return true*/

	/* wlan object lock/unlock is needed here to ensure
	 * * proper synchronization and prevent kernel panic */
	wlan_pdev_obj_lock(pdev);

	objmgr = &pdev->pdev_objmgr;
	vdev_list = &objmgr->wlan_vdev_list;
	/* Get first vdev */
	vdev = wlan_pdev_vdev_list_peek_head(vdev_list);

	while (vdev != NULL) {
		if (son_vdev_map_capability_get(vdev, SON_MAP_CAPABILITY_VAP_UP))
		{
			wlan_pdev_obj_unlock(pdev);
			return true;
		}
		/* get next vdev */
		vdev_next = wlan_vdev_get_next_vdev_of_pdev(vdev_list, vdev);
		vdev = vdev_next;
	}

	wlan_pdev_obj_unlock(pdev);
	return false;

}

/**
 * @brief Determine whether band steering events are enabled on
 *        a vdev.
 *
 * @param [in] vdev to check
 *
 * @return non-zero if it is enabled; otherwise 0
 */

u_int8_t wlan_son_is_vdev_event_enabled(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;

	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if (vdev_priv)
		return atomic_read(&vdev_priv->v_son_enabled);

	return false;
}


/**
 * @brief Determine whether broadcasting of band steering events is enabled on
 *        a vdev.
 *
 * @param [in] vdev to check
 *
 * @return non-zero if it is enabled; otherwise 0
 */

u_int8_t wlan_son_is_vdev_event_bcast_enabled(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;

	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if (vdev_priv)
		return atomic_read(&vdev_priv->event_bcast_enabled);

	return false;
}

/**
 * @brief Enable/Disable broadcasting of band steering events.
 *
 * @param [in] vdev to check and flag to enable/disable
 *
 * @return non-zero if it is failure; otherwise 0
 */

int son_core_enable_disable_vdev_bcast_events(struct wlan_objmgr_vdev *vdev ,
					u_int8_t enable)
{
	struct son_vdev_priv *vpriv = NULL;

	vpriv = wlan_son_get_vdev_priv(vdev);

	if(vpriv) {
		qdf_atomic_set(&vpriv->event_bcast_enabled, enable);
		return EOK;
	}

	return -EINVAL;
}



/**
 * @brief Determine whether the vdev handle is valid, has a valid band
 *        steering handle, is operating in a mode where band steering
 *        is relevant, and is not in the process of being deleted.
 *
 * @return true if the vdev is valid; otherwise false
 */

bool wlan_son_is_vdev_valid(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;
	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_SAP_MODE) &&
			!(vdev_priv->vdev_is_deleted))
		return true;

	return false;
}

/**
 * @brief Determine whether the vdev has band steering enabled.
 *
 * Validate that the vdev has a valid band steering handle, that
 * it is operating in the right mode (AP mode), and that band steering has been
 * enabled on the vdev
 *
 * @param [in] vdev the VAP to check
 *
 * @return true if the vdev is valid and has band steering enabled; otherwise
 *         false
 */
bool wlan_son_is_vdev_enabled(struct wlan_objmgr_vdev *vdev)
{
	return ((wlan_son_is_vdev_valid(vdev) && wlan_son_is_vdev_event_enabled(vdev)) ||
		son_vdev_map_capability_get(vdev, SON_MAP_CAPABILITY_VAP_UP));
}
/**
 * @brief Set WHC flag in peer.
 * whc flag is set as per son ie in mangement frames.
 * @param [in] Peer.
 *
 * @return void.
 */

void son_set_whc_apinfo_flag(struct wlan_objmgr_peer *peer, u_int8_t flag)
{
	struct son_peer_priv *pe_priv = NULL;
	pe_priv = wlan_son_get_peer_priv(peer);

	if (pe_priv)
		pe_priv->ni_whc_apinfo_flags |= flag;

	return;
}

/**
 * @brief clear WHC apinfo flag in peer.
 * @param [in] Peer.
 *
 * @return void.
 */

void son_clear_whc_apinfo_flag(struct wlan_objmgr_peer *peer, u_int8_t flag)
{
	struct son_peer_priv *pe_priv = NULL;
	pe_priv = wlan_son_get_peer_priv(peer);

	if (pe_priv)
		pe_priv->ni_whc_apinfo_flags &= ~flag;

	return;
}
/**
 * @brief check if peer has whc apinfo flag set.
 * @param [in] Peer and flag to check.
 * @return true if flag is set otherwise false.
 */

int son_has_whc_apinfo_flag(struct wlan_objmgr_peer *peer, u_int8_t flag)
{
	struct son_peer_priv *pe_priv = NULL;
	pe_priv = wlan_son_get_peer_priv(peer);

	if(pe_priv)
		return ((pe_priv->ni_whc_apinfo_flags & flag) != 0);
	else
		return false;
}
/**
 * @brief Set peer repeater flag.Son ie is parsed for this.
 * @param [in] peer.
 * @return void.
 */
void son_set_whc_rept_info(struct wlan_objmgr_peer *peer)
{
	struct son_peer_priv *pe_priv = NULL;
	pe_priv = wlan_son_get_peer_priv(peer);

	if (pe_priv)
		pe_priv->ni_whc_rept_info = 1;

	return;
}
/**
 * @brief clear whc repeater infor flag.
 * @param [in] Peer.
 * @return void.
 */
void son_clear_whc_rept_info(struct wlan_objmgr_peer *peer)
{
	struct son_peer_priv *pe_priv = NULL;

	if (!peer)
		return;

	pe_priv = wlan_son_get_peer_priv(peer);

	if (pe_priv)
		pe_priv->ni_whc_rept_info = 0;

	return;
}
/**
 * @brief check if peer has whc repeater info flag.
 * @param [in] Peer.
 * @return true if flag is set otherwise false.
 */

int32_t son_get_whc_rept_info(struct wlan_objmgr_peer *peer)
{
	struct son_peer_priv *pe_priv = NULL;

	if (!peer)
		return -EINVAL;

	pe_priv = wlan_son_get_peer_priv(peer);

	if (pe_priv)
		return (pe_priv->ni_whc_rept_info);
	else
		return -EINVAL;
}
int8_t ucfg_son_set_root_dist(struct wlan_objmgr_vdev *vdev,
			      u_int8_t root_distance)
{
	if (vdev)
		return(son_core_set_root_dist(vdev, root_distance));
	else {
		SON_LOGI("SON on Pdev Needs to be enabled for root distance");
		return 0;
	}

	return 0;
}

u_int8_t ucfg_son_get_root_dist(struct wlan_objmgr_vdev *vdev)
{
	if (vdev)
		return(son_core_get_root_dist(vdev));
	else {
		SON_LOGI("SON on Pdev Needs to be enabled for root distance");
		return 0;
	}
}

/**
 * @brief Called to get the in network table for 2G
 *
 * @param [in] vap  the VAP on which the table is requested for
 * @param [in] channel  return all mac address for this channel
 * @param [inout] num_entries  if 0 return total entries in table,
 *                          else return number of entries
 * @param [inout] data  pointer to hold data to return
 */

int8_t son_get_innetwork_table(struct wlan_objmgr_vdev *vdev, void *data, int *num_entries,
                               int8_t channel) {
	struct son_pdev_priv *pd_priv = NULL;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
        struct mlme_external_tx_ops *tx_ops = NULL;
	struct in_network_table *tmpnode;
        wlan_chan c = {0};
        wlan_chan *loc_chan = NULL;
	int index = 0;

	pd_priv = wlan_son_get_pdev_priv(pdev);

	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->vdev_ops) {
		SON_LOGE("%s: tx_ops/vdev_ops is NULL ", __func__);
		return QDF_STATUS_E_INVAL;
	}

        tx_ops->vdev_ops(vdev, VDEV_GET_CHAN, NULL, &c);

        loc_chan = &c;

	if (!IEEE80211_IS_CHANNEL_VALID(loc_chan)) {
		SON_LOGI(" failed to resolve channel from vdev \r\n");
		return QDF_STATUS_E_INVAL;
	}
	if (IEEE80211_IS_CHAN_5GHZ(loc_chan)) {
		SON_LOGI("channel is not 2.4G return failed \r\n");
		return QDF_STATUS_E_INVAL;
	}

	if (!*num_entries) {
		num_entries = (int *)data;
		TAILQ_FOREACH(tmpnode, &pd_priv->in_network_table_2g, table_list) {
			index++;
		}
		*num_entries = index;
		return 0;
	} else {
		if (!channel) {
			struct ieee80211_bsteering_in_network_2G_table *in_network_2g_data =
				(struct ieee80211_bsteering_in_network_2G_table *)data;

			TAILQ_FOREACH(tmpnode, &pd_priv->in_network_table_2g, table_list) {
				if (index == *num_entries)
				    break;
				memcpy(in_network_2g_data[index].mac_addr, tmpnode->macaddr,QDF_MAC_ADDR_SIZE);
				in_network_2g_data[index].ch = tmpnode->channel;
				index++;
			}

			in_network_2g_data->total_index = index;
		} else {
			u_int8_t *mac_addr = (u_int8_t *)data;
			TAILQ_FOREACH(tmpnode, &pd_priv->in_network_table_2g, table_list) {
				if (channel == tmpnode->channel) {
					memcpy(mac_addr + (index * QDF_MAC_ADDR_SIZE), tmpnode->macaddr,QDF_MAC_ADDR_SIZE);
					index++;
				}
			}
		}
	}

	return 0;
}

/**
 * @brief To get uplink_rate for mixed backhaul.
 *
 * @param [in] vdev the VAP to get.
 *
 * @return uplink_rate.
 */
u_int16_t son_get_ul_mixedbh(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *son_pd = NULL;
	u_int16_t uplink_rate = 0;

	pdev = wlan_vdev_get_pdev(vdev);
	if(!pdev) {
		qdf_err("%s:pdev is NULL ", __func__);
		return uplink_rate;
	}

	son_pd = wlan_son_get_pdev_priv(pdev);
	if (son_pd)  {
		uplink_rate = son_pd->curr_ul_rate_mixedbh;
	}
	return uplink_rate;
}

/**
 * @brief To set uplink_rate for mixed backhaul.
 *
 * @param [in] vdev the VAP to set.
 * @param [in] u_int16_t ulrate.
 *
 * @return true if set ul rate succesfully, otherwise return false.
 */
bool son_set_ul_mixedbh(struct wlan_objmgr_vdev *vdev, u_int16_t ulrate)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *son_pdev = NULL;

	pdev = wlan_vdev_get_pdev(vdev);
	if(!pdev) {
		qdf_err("%s:pdev is NULL ", __func__);
		return false;
	}

	son_pdev = wlan_son_get_pdev_priv(pdev);
	if (son_pdev)  {
		son_pdev->recv_ul_rate_mixedbh = ulrate;
		return true;
	}
	return false;
}

/**
 * @brief To set sonmode and backhaul type for mixed backhaul.
 *
 * @param [in] vdev the VAP
 * @param [in] u_int16_t sonmode (CAP or RE) and backhaul type used.
 *
 * @return true if set ul rate succesfully, otherwise return false.
 */
bool son_set_backhaul_type_mixedbh(struct wlan_objmgr_vdev *vdev, u_int8_t backhaul_type)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *son_pdev = NULL;

	pdev = wlan_vdev_get_pdev(vdev);
	if(!pdev) {
		qdf_err("%s:pdev is NULL ", __func__);
		return false;
	}

	son_pdev = wlan_son_get_pdev_priv(pdev);
	if (son_pdev)  {
		son_pdev->son_backhaul_type = backhaul_type;
		return true;
	}
	return false;
}

#if QCA_SUPPORT_SSID_STEERING
/**
 * @brief To get vdev config type of ssid steering.
 *
 * @param [in] vdev the VAP.
 *
 * @return true if VAP is private.
 */
bool son_get_ssid_steering_vdev_is_pvt(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;
	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if (!vdev_priv) {
		qdf_err("%s:SON vdev_priv is NULL ", __func__);
		return false;
	}

	if (vdev_priv->ssid_steering_config ==
	    SON_SSID_STEERING_PRIVATE_VDEV)
		return true;
	else
		return false;

}

/**
 * @brief To get vdev config type of ssid steering.
 *
 * @param [in] vdev the VAP.
 *
 * @return true if VAP is public.
 */
bool son_get_ssid_steering_vdev_is_pub(struct wlan_objmgr_vdev *vdev)
{
	struct son_vdev_priv *vdev_priv = NULL;
	vdev_priv = wlan_son_get_vdev_priv(vdev);

	if (!vdev_priv) {
		qdf_err("%s:SON vdev_priv is NULL ", __func__);
		return false;
	}

	if (vdev_priv->ssid_steering_config ==
	    SON_SSID_STEERING_PUBLIC_VDEV)
		return true;
	else
		return false;
}
#endif


void son_bs_stats_update_cb(void *psoc_obj, enum WDI_EVENT event,
                            void *data, uint16_t data_len,
                            uint32_t status)
{
	struct cdp_interface_peer_stats *bs_stats = (struct cdp_interface_peer_stats *)data;
	struct wlan_objmgr_psoc *psoc = (struct wlan_objmgr_psoc *) psoc_obj;
	struct wlan_objmgr_peer *peer = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *current_vdev = NULL;
	struct wlan_objmgr_vdev *rssi_vdev = NULL;
	struct bs_sta_stats_ind sta_stats = {0};
	struct son_peer_priv *pe_priv = NULL;
	unsigned long current_jiffies;

	static uint32_t prev_ack_rssi = 0;
	uint8_t is_ack_rssi_enabled = 0;
	struct wlan_objmgr_vdev *vdev =  wlan_objmgr_get_vdev_by_id_from_psoc(
                                            psoc, bs_stats->vdev_id, WLAN_MLME_SB_ID);

	if (!vdev) {
		qdf_print("%s vdev is null",__func__);
		return;
	}
	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev) {
		qdf_print("%s pdev is null",__func__);
		return;
	}
	peer = wlan_objmgr_vdev_find_peer_by_mac(vdev, bs_stats->peer_mac, WLAN_SON_ID);
	if (!peer) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
		SON_LOGI("%s ctrl peer is null",__func__);
		return;
	}

	/* Discard stats of Self peer for AP mode */
	if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_SAP_MODE) &&
			 (wlan_peer_get_peer_type(peer) == WLAN_PEER_AP)) {
		wlan_objmgr_peer_release_ref(peer, WLAN_SON_ID);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
		return;
	}

	son_ald_record_set_free_descs(psoc_obj, bs_stats->free_buff);
	if (son_ald_record_get_buff_full_warn(psoc_obj) &&
			(son_ald_record_get_free_descs(psoc_obj) <= son_ald_record_get_buff_lvl(psoc_obj))) {
		son_buffull_handler(pdev);
		son_ald_record_set_buff_full_warn(psoc_obj, 0);
	}

	/* set rssi_changed flag */
	rssi_vdev = wlan_peer_get_vdev(peer);
	is_ack_rssi_enabled = son_is_ackrssi_enabled(rssi_vdev);
	if (is_ack_rssi_enabled) {
		bs_stats->rssi_changed = false;
		bs_stats->rssi_changed =
			prev_ack_rssi != bs_stats->ack_rssi ? true : false;
	}
	pe_priv = wlan_son_get_peer_priv(peer);
       if (!pe_priv) {
           qdf_print("SONUNEXPECTED: %s: peer priv is NULL",__func__);
           return;
       }

	/* New RSSI measurement */
	if (is_ack_rssi_enabled && bs_stats->rssi_changed) {
		son_record_peer_rssi(peer, bs_stats->ack_rssi);
	}
	else if (bs_stats->rssi_changed) {
		son_record_peer_rssi(peer, bs_stats->peer_rssi);
	}

	/* Tx rate has changed */
	if (bs_stats->peer_tx_rate &&
			(bs_stats->peer_tx_rate != bs_stats->last_peer_tx_rate)) {
		son_update_peer_rate(peer,
				     bs_stats->peer_tx_rate,
				     bs_stats->last_peer_tx_rate);
	}

	/* Only need to send a STA stats update for this peer if the
	 * RSSI changed and the Tx rate is valid
	*/
	current_jiffies = jiffies;
	if ((jiffies_to_msecs(current_jiffies - pe_priv->last_jiffies) >= g_son_sta_stats_event_timer)
			&& bs_stats->rssi_changed && bs_stats->peer_tx_rate) {
		pe_priv->last_jiffies = current_jiffies;
		if (son_update_sta_stats(peer,
					current_vdev,
					&sta_stats,
					bs_stats)) {
			current_vdev = wlan_peer_get_vdev(peer);
		}
		if (current_vdev) {
			son_send_sta_stats_event(current_vdev, &sta_stats);
		}
	}

	/* Save previous value */
	prev_ack_rssi = bs_stats->ack_rssi;

	/* Check the peer TX and RX packet count for inactivity check */
       if (bs_stats->tx_packet_count > pe_priv->tx_packet_count ||
               bs_stats->rx_packet_count > pe_priv->rx_packet_count) {
               son_mark_node_inact(peer, false /* inactive */);
               pe_priv->tx_packet_count = bs_stats->tx_packet_count;
               pe_priv->rx_packet_count = bs_stats->rx_packet_count;
        }

	if (peer)
		wlan_objmgr_peer_release_ref(peer, WLAN_SON_ID);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SB_ID);
}

void son_iterate_get_active_vdev_per_pdev(struct wlan_objmgr_pdev *pdev,
		            void *obj,
			    void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)obj;
	int *active_vdev = (int *)arg;

	if (vdev) {
		if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_SAP_MODE) ||
				(wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE)) {
			if (wlan_vdev_chan_config_valid(vdev) == QDF_STATUS_SUCCESS) {
				++(*active_vdev);
			}
		}
	}
}

int son_get_num_active_vdev_per_pdev(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	int num_active_vdev = 0;

	pdev = wlan_vdev_get_pdev(vdev);

	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
			son_iterate_get_active_vdev_per_pdev,
			&num_active_vdev, 0,
			WLAN_SON_ID);

	return num_active_vdev;
}

static void son_iterate_vdev_appie_update_ie(struct wlan_objmgr_pdev *pdev,
					     void *obj,
					     void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)obj;

	if (vdev) {
		if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
			son_update_appielist(vdev, IEEE80211_FRAME_TYPE_ASSOCREQ);
		} else if (wlan_vdev_mlme_get_opmode(vdev) == QDF_SAP_MODE) {
			// Avoid including SON IE in Fronthaul VAP
			if (!(son_vdev_map_capability_get(vdev, SON_MAP_CAPABILITY_VAP_TYPE) & MAP_FRONTHAUL_BSS))
			{
				son_update_appielist(vdev, IEEE80211_FRAME_TYPE_BEACON);
				son_update_appielist(vdev, IEEE80211_FRAME_TYPE_PROBERESP);
				son_update_appielist(vdev, IEEE80211_FRAME_TYPE_ASSOCRESP);
			}
		}
	}
	return;
}


void son_pdev_appie_update(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_pdev *pdev = NULL;;

	pdev = wlan_vdev_get_pdev(vdev);

	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
			                  son_iterate_vdev_appie_update_ie,
			                  NULL, 0,
			                  WLAN_SON_ID);
	return;
}

int son_update_appielist(struct wlan_objmgr_vdev *vdev, ieee80211_frame_type ftype)
{
        struct mlme_external_tx_ops *tx_ops = NULL;
        wlan_app_ie_data in_appie = {0};
        u_int8_t rptr_is_wds = 0;

#define AP_TOT_LEN 33
#define STA_TOT_LEN 8

	if(!vdev)
		return -EINVAL;

	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->vdev_ops) {
		SON_LOGE("%s: tx_ops/vdev_ops is NULL ", __func__);
		return -EINVAL;
	}
	if (tx_ops->vdev_ops(vdev, VDEV_GET_WDS, NULL, &rptr_is_wds) != QDF_STATUS_SUCCESS)
		return -EINVAL;

	if (rptr_is_wds) {
		u_int16_t whcCaps = QCA_OUI_WHC_AP_INFO_CAP_WDS;
		u_int8_t app_ie[AP_TOT_LEN] = {0};

		if (son_vdev_feat_capablity(vdev, SON_CAP_GET, WLAN_VDEV_F_SON) ||
				((wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) &&
				 (ftype == IEEE80211_FRAME_TYPE_ASSOCREQ))) {
			whcCaps |= QCA_OUI_WHC_AP_INFO_CAP_SON;
		}

		in_appie.ftype = ftype;
		in_appie.ie = app_ie;
		son_add_ap_appie(in_appie.ie, whcCaps, vdev);
		in_appie.size = in_appie.ie[1] + 2;
		in_appie.id = DEFAULT_IDENTIFIER;
		tx_ops->vdev_ops(vdev, VDEV_SET_IE , &in_appie, NULL);
	}

	if (rptr_is_wds &&
			son_vdev_fext_capablity(vdev,
				SON_CAP_GET,
				WLAN_VDEV_FEXT_SON_SPL_RPT)) {
		u_int8_t app_ie[STA_TOT_LEN] = {0};
		in_appie.ftype = ftype;
		in_appie.ie = app_ie;

		son_add_rept_appie(app_ie);
		in_appie.size = in_appie.ie[1] + 2;
		in_appie.id = DEFAULT_IDENTIFIER;
		tx_ops->vdev_ops(vdev, VDEV_SET_IE , &in_appie, NULL);
	}
#undef STA_TOT_LEN
#undef AP_TOT_LEN
	return 0;
}

int son_update_mgmt_frame(struct wlan_objmgr_vdev *vdev,
			  struct wlan_objmgr_peer *peer, int subtype,
			  u_int8_t *frame, u_int16_t frame_len,
			  void *meta_data)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	const u_int8_t *map_info = NULL;
	struct son_peer_priv *pe_priv = NULL;
	const struct ieee80211_ie_map *mapIE = NULL;

	if (!vdev || !frame || !frame_len)
		return -EINVAL;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev)
		return -EINVAL;

	switch (subtype) {
		case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
		case IEEE80211_FC0_SUBTYPE_REASSOC_REQ:
		{
			struct mlme_external_tx_ops *tx_ops = NULL;
			struct ieee80211_frame *wh;
			uint8_t *frm, *efrm;
			uint8_t *whc_rept_info = NULL;
			int reassoc;
			int join_ret = 0;
			uint16_t assoc_status = 0;
			u_int8_t rptr_is_wds = 0;

			if (!peer) {
				return EINVAL;
			}

                        if (!meta_data)
				return EINVAL;

			assoc_status = *((uint16_t *)meta_data);
			wh = (struct ieee80211_frame *) frame;
			frm = (u_int8_t *)&wh[1];
			efrm = frame + frame_len;

			if (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) {
				reassoc = 1;
			} else {
				reassoc = 0;
			}

			frm += 4;
			if (reassoc)
				frm += 6;    /* ignore current AP info */

			while (((frm + 1) < efrm) && (frm + frm[1] + 1) < efrm) {
				switch (*frm) {
					case IEEE80211_ELEMID_VENDOR:
						if (is_qca_son_rept_oui(frm, QCA_OUI_WHC_REPT_INFO_SUBTYPE))
							whc_rept_info = frm;
						else if (is_qca_son_oui(frm, QCA_OUI_WHC_AP_INFO_SUBTYPE)) {
							u_int16_t whcCaps;
							struct ieee80211_ie_whc_apinfo *whcAPInfoIE = (struct ieee80211_ie_whc_apinfo *)frm;

							whcCaps = LE_READ_2(&whcAPInfoIE->whc_apinfo_capabilities);
							if (whcCaps & QCA_OUI_WHC_AP_INFO_CAP_WDS) {
								son_set_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_WDS);
							}
							if (whcCaps & QCA_OUI_WHC_AP_INFO_CAP_SON) {
								son_set_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_SON);
								son_repeater_cnt_inc(vdev);
							}
						}
						else if ((LE_READ_4(frm + 2) == ((WFA_OUI_MAP_TYPE_IE << 24) | WFA_OUI_IE))) {
							map_info = frm;
							mapIE = (const struct ieee80211_ie_map *)map_info;
							if (mapIE->map_ext_subelement == MAP_PROFILE_EXTENSION_SUBELEMENT) {
								if(mapIE->map_ext_value & MAP_BACKHAUL_STA){
									pe_priv = wlan_son_get_peer_priv(peer);
									if (pe_priv) {
										pe_priv->son_bSTA_flag = true;
									}
							    }
#ifdef WLAN_FEATURE_11BE_MLO
								if (wlan_peer_is_mlo(peer))
									peer->peer_flags |=
									    WLAN_PEER_MESH_NODE;
#endif
						    }
						}
						break;
					default:
						break;
				}
				frm += frm[1] + 2;
			}
#ifdef WLAN_FEATURE_11BE_MLO
			if (wlan_peer_is_mlo(peer)) {
				pe_priv = wlan_son_get_peer_priv(peer);
				if (pe_priv) {
					pe_priv->son_is_primary_link_flag = true;
				}
			}
#endif
			son_clear_whc_rept_info(peer);
			if (whc_rept_info != NULL)
				son_process_whc_rept_info_ie(peer, whc_rept_info);

			if (assoc_status == IEEE80211_STATUS_SUCCESS) {
				tx_ops = wlan_son_get_tx_ops(vdev);
				if (!tx_ops || !tx_ops->vdev_ops) {
					SON_LOGE("%s: tx_ops/ vdev_ops is NULL ", __func__);
					return -EINVAL;
				}
				if (tx_ops->vdev_ops(vdev, VDEV_GET_WDS, NULL, &rptr_is_wds) != QDF_STATUS_SUCCESS)
					return -EINVAL;
				if (rptr_is_wds &&
						son_vdev_fext_capablity(vdev,
							SON_CAP_GET,
							WLAN_VDEV_FEXT_SON_SPL_RPT)) {
					if (!son_get_whc_rept_info(peer)) {
//						IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni, "WHC "
//								"AP Rept Special mode - association disallowed");
						join_ret = 1;
					}
				} else {
					if (son_get_whc_rept_info(peer)) {
//						IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni, "WHC "
//								"STA Rept Special mode - association disallowed");
						join_ret = 1;
					}
				}

				if (!join_ret) {
					if(son_has_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_SON)) {
						son_pdev_appie_update(vdev);
					}
					if (frame) {
						son_update_assoc_frame(peer, frame, frame_len);
					}
				} else {
					return 1;
				}
			} else {
				return 1;
			}
			break;
		}
		case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
		case IEEE80211_FC0_SUBTYPE_REASSOC_RESP:
		{
			struct ieee80211_frame *wh;
			uint8_t *frm, *efrm;
			u_int32_t ie_len;
			u_int8_t *whc_apinfo = NULL;
			u_int8_t *assoc_resp_ie = NULL;

			if (!peer) {
				return EINVAL;
			}
			wh = (struct ieee80211_frame *) frame;
			frm = (u_int8_t *)&wh[1];
			efrm = frame + frame_len;
			ie_len = efrm - frm;

			frm += 6;
			assoc_resp_ie = frm;

			while (frm < efrm) {
				switch (*frm) {
					case IEEE80211_ELEMID_VENDOR:
						if (is_qca_son_oui(frm,
								   QCA_OUI_WHC_AP_INFO_SUBTYPE))
							whc_apinfo = frm;
						else if ((LE_READ_4(frm + 2) ==
							 ((WFA_OUI_MAP_TYPE_IE << 24) |
							   WFA_OUI_IE))) {
							map_info = frm;
							mapIE =
							 (const struct ieee80211_ie_map *)map_info;
							if (mapIE->map_ext_subelement ==
								MAP_PROFILE_EXTENSION_SUBELEMENT) {
#ifdef WLAN_FEATURE_11BE_MLO
								if (wlan_peer_is_mlo(peer)) {
									peer->peer_flags |=
										WLAN_PEER_MESH_NODE;
								}
#endif
							}
						}
						break;
					default:
						break;
				}
				frm += frm[1] + 2;
			}

			if (whc_apinfo != NULL) {
				son_process_whc_apinfo_ie(peer, whc_apinfo);
				if (ucfg_son_get_root_dist(vdev) == SON_INVALID_ROOT_AP_DISTANCE) {
					/* If the AP is in WDS/SON mode and there is no Root connectivity
					 * donot associate to it */
					return -EINVAL;
				}
			} else {
				son_clear_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_WDS);
				son_clear_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_SON);
			}

			if (assoc_resp_ie != NULL) {
				son_process_assoc_resp_ie(peer, assoc_resp_ie, ie_len);
			}
			break;
		}
		case IEEE80211_FC0_SUBTYPE_BEACON:
		{
			uint8_t snr, nss;
			u_int8_t update_beacon = 0;
			u_int16_t se_apinfo_uplink_rate;
			struct ieee80211_ie_whc_apinfo *se_sonadv = NULL;
			u_int16_t uplink_rate = 0;
			struct wlan_beacon_frm_info *beacon_info = NULL;

			if (!meta_data)
				return -EINVAL;

			beacon_info = (struct wlan_beacon_frm_info *) meta_data;
			se_sonadv = (struct ieee80211_ie_whc_apinfo *) beacon_info->se_sonadv;
			snr = beacon_info->snr;
			son_update_uplink_snr(vdev, snr);
			nss = beacon_info->nss;
			son_update_nss(vdev, nss);
			/* Update the uplink rate */
			uplink_rate = son_get_uplinkrate(vdev, snr);

			if(son_get_backhaul_rate(vdev, false) != uplink_rate) {
				son_update_backhaul_rate(vdev, uplink_rate , false);
				update_beacon = 1;
			}


			if(se_sonadv != NULL && se_sonadv->whc_apinfo_root_ap_dist > 0) {
				/* Get the serving AP backhaul rate from scan entry */
				son_update_backhaul_rate(vdev,
						LE_READ_2(&se_sonadv->whc_apinfo_uplink_rate),
						true);
			}

			if(se_sonadv != NULL) {
				se_apinfo_uplink_rate = LE_READ_2(&se_sonadv->whc_apinfo_uplink_rate);

				if(ucfg_son_get_root_dist(vdev) > 0) {
					if (son_get_backhaul_rate(vdev, true) != se_apinfo_uplink_rate) {
						/* Get the serving AP backhaul rate from scan entry */
						son_update_backhaul_rate(vdev, se_apinfo_uplink_rate , true);
						update_beacon = 1;
					}

					if (ucfg_son_get_root_dist(vdev) != se_sonadv->whc_apinfo_root_ap_dist + 1) {

						if (se_sonadv->whc_apinfo_root_ap_dist != SON_INVALID_ROOT_AP_DISTANCE) {
							ucfg_son_set_root_dist(vdev, se_sonadv->whc_apinfo_root_ap_dist + 1);
						} else {
							ucfg_son_set_root_dist(vdev, SON_INVALID_ROOT_AP_DISTANCE);
						}
						update_beacon = 1;
					}
				}
			}

			if (update_beacon) {
				son_pdev_appie_update(vdev);
				return 1;
			}
			break;
		}
		case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
		{
                        struct wlan_probe_req_data *data = NULL;
			struct bs_probe_req_data prb_data = {0};

			if (!meta_data) {
				return EINVAL;
			}
			data = (struct wlan_probe_req_data * ) meta_data;

			qdf_mem_copy(&prb_data.probe_req.sender_addr, &data->probe_req.sender_addr,
					IEEE80211_ADDR_LEN);
			prb_data.probe_req.rssi = data->probe_req.rssi;
			prb_data.probe_req.blocked = data->probe_req.blocked;
			prb_data.probe_req.ssid_null = data->probe_req.ssid_null;
			prb_data.is_chan_2G = data->is_chan_2G;

			son_notify_user(vdev, ATH_EVENT_BSTEERING_PROBE_REQ,
					sizeof(struct bs_probe_req_ind),
					(const char *) &prb_data.probe_req);
			if (prb_data.is_chan_2G &&
					son_is_probe_resp_wh_2G(vdev, prb_data.probe_req.sender_addr,
						prb_data.probe_req.rssi)) {
				return 1;
			}
			break;
		}
		case IEEE80211_FC0_SUBTYPE_ACTION:
		case IEEE80211_FCO_SUBTYPE_ACTION_NO_ACK:
		{
                        struct wlan_act_frm_info *info = NULL;
			struct ieee80211_action *ia = NULL;
			struct bs_rrm_frame_report_ind *event = NULL;
			size_t event_len = 0;

			if (!meta_data) {
				return EINVAL;
			}
			SON_LOGI("%s: event wnm frame recvd", __func__);
			info = (struct wlan_act_frm_info *)meta_data;
			if (!info)
				return -EINVAL;

			ia = info->ia;
			if (!ia)
				return -EINVAL;
			switch (ia->ia_category) {
				case IEEE80211_ACTION_CAT_PROT_DUAL:
				case IEEE80211_ACTION_CAT_PUBLIC:
					switch (ia->ia_action) {
						case IEEE80211_ACTION_GAS_INITIAL_REQUEST:
							if (info->ald_info) {
								son_ald_anqp_frame_recvd_notify(vdev, info->data.macaddr,
												frame, frame_len);
							}
							break;
						default:
							SON_LOGE("%s: Invalid action", __func__);
							return -EINVAL;
					}
					break;
				case IEEE80211_ACTION_CAT_WNM:
					if (info->ald_info) {
						son_ald_wnm_frame_recvd_notify(vdev, ia->ia_action, info->data.macaddr,
									       frame, frame_len);
					}
					break;
				case IEEE80211_ACTION_CAT_RM:
					if (!wlan_son_is_vdev_enabled(vdev) ||
							!wlan_son_is_pdev_enabled(pdev) ) {
						return -EINVAL;
					}

					event_len = sizeof(struct bs_rrm_frame_report_ind) + frame_len;
					event = (struct bs_rrm_frame_report_ind*)qdf_mem_malloc(event_len);

					if (!event) {
						SON_LOGE("%s: Failed to create event buffer\n", __func__);
						return -EINVAL;
					}

					event->rrm_type = BSTEERING_RRM_TYPE_BCNRPT;
					event->measrpt_mode = IEEE80211_RRM_MEASRPT_MODE_SUCCESS;
					event->dialog_token = info->data.rrm_data.dialog_token;
					event->num_meas_rpts = info->data.rrm_data.num_meas_rpts;
					event->data_len = frame_len;
					WLAN_ADDR_COPY(event->macaddr, info->data.rrm_data.macaddr);
					qdf_mem_copy(&event->meas_rpt_data, frame, frame_len);
					son_notify_user(vdev, ATH_EVENT_BSTEERING_RRM_FRAME_REPORT,
							event_len, (const char *) event);
					qdf_mem_free(event);
					break;
				default:
					SON_LOGE("%s: Invalid action category", __func__);
					return -EINVAL;
			}
			break;
		}
		default:
			return -EINVAL;
	}
	return 0;
}

int son_update_mlme_event(struct wlan_objmgr_vdev *vdev,
		          struct wlan_objmgr_peer *peer,
			  enum ieee80211_event_type event, void *event_data)
{
	ald_cbs_event_type ald_cbs_type = {0};
	struct wlan_assoc_disassoc_event_info *assoc_info = NULL;
	struct wlan_assoc_allow_event_info *assoc_allow_info = NULL;
	wlan_cbs_event_type *type = NULL;
	struct bs_auth_reject_ind auth_reject = {0};
	struct wlan_auth_reject_ind *auth_reject_evdata = NULL;
	u_int8_t *radar_detected = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct bs_disconnect_ind disconnect_event_data = {0};
	struct ald_mscs_event_info *mscs_info = NULL;
	struct ald_scs_event_info *scs_info = NULL;
	struct ald_dscp_policy_query_info *dscp_query_info = NULL;
	struct ald_dscp_policy_resp_info *dscp_resp_info = NULL;

	if (!vdev)
		return -EINVAL;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev)
		return -EINVAL;

	switch(event) {
		case MLME_EVENT_ASSOC_DISASSOC:
			SON_LOGI("%s: event ald assoc", __func__);
			assoc_info = (struct wlan_assoc_disassoc_event_info *)event_data;
			if (!assoc_info)
				return -EINVAL;
			son_ald_assoc_notify(vdev, assoc_info->macaddr,
						assoc_info->flag, assoc_info->reason);
			if (assoc_info->flag == ALD_ACTION_DISASSOC) {
				disconnect_event_data.source = BSTEERING_SOURCE_LOCAL;
				disconnect_event_data.type = assoc_info->flag;
				disconnect_event_data.reason = assoc_info->reason;
				qdf_mem_copy(disconnect_event_data.client_addr, assoc_info->macaddr,
					 QDF_MAC_ADDR_SIZE);
				son_notify_user(vdev, ATH_EVENT_BSTEERING_CLIENT_DISCONNECTED,
						sizeof(struct bs_disconnect_ind),
						(const char *)&disconnect_event_data);
			}
			break;
		case MLME_EVENT_CBS_STATUS:
			SON_LOGI("%s: event ald cbs", __func__);
			type = (wlan_cbs_event_type *)event_data;

			if (!type)
				return -EINVAL;
                        ald_cbs_type = *type;
			son_ald_cbs_notify(vdev, ald_cbs_type);
			break;
		case MLME_EVENT_ACS_COMPLETE:
			SON_LOGI("%s: event ald acs complete", __func__);
			son_ald_acs_complete_notify(vdev);
			break;
		case MLME_EVENT_CAC_STATUS:
			SON_LOGI("%s: event ald cac complete", __func__);
			radar_detected = (u_int8_t *)event_data;
			if (!radar_detected)
				return -EINVAL;
			son_ald_cac_complete_notify(vdev, *radar_detected);
			break;
		case MLME_EVENT_ASSOC_ALLOWANCE_STATUS:
			if (!event_data)
				return -EINVAL;

			assoc_allow_info = (struct wlan_assoc_allow_event_info *)event_data;
			if (!assoc_allow_info)
				return -EINVAL;
			son_ald_assoc_allowance_status_notify(vdev, assoc_allow_info->bssid,
							      assoc_allow_info->status);
			break;
		case MLME_EVENT_TX_AUTH_FAIL:
			if(!wlan_son_is_vdev_enabled(vdev) ||
					!wlan_son_is_pdev_enabled(pdev) ) {
				return -EINVAL;
			}
                        if (!event_data)
				return -EINVAL;
			auth_reject_evdata = (struct wlan_auth_reject_ind *)event_data;

                        qdf_mem_copy(&auth_reject.client_addr, &auth_reject_evdata->client_addr,
                                     IEEE80211_ADDR_LEN);
                        auth_reject.rssi = auth_reject_evdata->rssi;
                        auth_reject.bs_blocked = auth_reject_evdata->bs_blocked;
                        auth_reject.bs_rejected = auth_reject_evdata->bs_rejected;
                        auth_reject.reason = auth_reject_evdata->reason;

			SON_LOGI("%s: event Bsteering tx auth fail", __func__);
			son_notify_user(vdev, ATH_EVENT_BSTEERING_TX_AUTH_FAIL,
					sizeof(struct bs_auth_reject_ind),
					(const char *)&auth_reject);
			break;
		case MLME_EVENT_TX_AUTH_ALLOW:
			if(!wlan_son_is_vdev_enabled(vdev) ||
					!wlan_son_is_pdev_enabled(pdev) ) {
				return -EINVAL;
			}

			if (!event_data)
				return -EINVAL;

			auth_reject_evdata = (struct wlan_auth_reject_ind *)event_data;

			qdf_mem_copy(&auth_reject.client_addr, &auth_reject_evdata->client_addr,
					IEEE80211_ADDR_LEN);
			auth_reject.rssi = auth_reject_evdata->rssi;
			auth_reject.bs_blocked = auth_reject_evdata->bs_blocked;
			auth_reject.bs_rejected = auth_reject_evdata->bs_rejected;
			auth_reject.reason = auth_reject_evdata->reason;

			SON_LOGI("%s: event Bsteering tx auth allow", __func__);
			son_notify_user(vdev, ATH_EVENT_BSTEERING_DBG_TX_AUTH_ALLOW,
					sizeof(struct bs_auth_reject_ind),
					(const char *)&auth_reject);
			break;
		case MLME_EVENT_TX_ASSOC_FAIL:
#if QCA_SUPPORT_DE
			if(!wlan_son_is_vdev_enabled(vdev) ||
					!wlan_son_is_pdev_enabled(pdev) ) {
				return -EINVAL;
			}
			if (!event_data)
				return -EINVAL;
			auth_reject_evdata = (struct wlan_auth_reject_ind *)event_data;

			qdf_mem_copy(&auth_reject.client_addr, &auth_reject_evdata->client_addr,
					IEEE80211_ADDR_LEN);
			auth_reject.rssi = auth_reject_evdata->rssi;
			auth_reject.bs_blocked = auth_reject_evdata->bs_blocked;
			auth_reject.bs_rejected = auth_reject_evdata->bs_rejected;
			auth_reject.reason = auth_reject_evdata->reason;

			SON_LOGI("%s: event Bsteering tx assoc fail", __func__);
			son_notify_user(vdev, ATH_EVENT_BSTEERING_TX_ASSOC_FAIL,
					sizeof(struct bs_assoc_reject_ind),
					(const char *)&auth_reject);
#endif
			break;
		case MLME_EVENT_CLIENT_ASSOCIATED:
			{
				struct bs_node_associated_ind assoc = {0};
				struct wlan_peer_cap peerAssocData = {0};
	                        struct mlme_external_tx_ops *tx_ops = NULL;
				int max_MCS = 0;
				u_int8_t i = 0;
				struct son_peer_priv *pe_priv = NULL;

				if (!peer)
					return -EINVAL;

				son_enable_disable_peer_ext_stats(peer, 1); // Enable Peer Ext Stats Collection
				son_peer_authorize(peer);
				if(!wlan_son_is_vdev_enabled(vdev) ||
						!wlan_son_is_pdev_enabled(pdev)) {
					return -EINVAL;
				}

				SON_LOGI("%s: event Bsteering node associated", __func__);

				tx_ops = wlan_son_get_tx_ops(vdev);
				if (!tx_ops || !tx_ops->peer_ops) {
					SON_LOGE("%s: tx_ops/ peer_ops is NULL ", __func__);
					return -EINVAL;
				}

				pe_priv = wlan_son_get_peer_priv(peer);
				qdf_mem_copy(&assoc.client_addr, wlan_peer_get_macaddr(peer), QDF_MAC_ADDR_SIZE);
				if (tx_ops->peer_ops(peer, PEER_GET_MAX_MCS, NULL, &max_MCS) != QDF_STATUS_SUCCESS)
					return -EINVAL;
				assoc.datarate_info.max_MCS = max_MCS < 0 ? 0 : max_MCS;


#ifdef WLAN_FEATURE_11BE_MLO
				assoc.isMLDSupported = wlan_peer_is_mlo(peer);

				if (assoc.isMLDSupported) {
					struct mlo_partner_info ml_links = {0};

					qdf_mem_copy(&assoc.mldInfo.mld_addr,
							wlan_peer_mlme_get_mldaddr(peer), QDF_MAC_ADDR_SIZE);
					wlan_mlo_peer_get_partner_links_info(peer, &ml_links);
					assoc.mldInfo.mldLinks = 0;
					if (ml_links.num_partner_links <= IEEE80211_MAX_MLD_LINKS) {
						assoc.mldInfo.mldLinks = ml_links.num_partner_links;
						for (i = 0; i < assoc.mldInfo.mldLinks; i++) {
							assoc.mldInfo.mldPartnerInfo[i].link_id =
								ml_links.partner_link_info[i].link_id;
							assoc.mldInfo.mldPartnerInfo[i].vdev_id =
								ml_links.partner_link_info[i].vdev_id;
							WLAN_ADDR_COPY(assoc.mldInfo.mldPartnerInfo[i].partnerLinkAddr,
								&ml_links.partner_link_info[i].link_addr.bytes[0]);
						}
					}
					assoc.isPrimary = pe_priv->son_is_primary_link_flag;
				}
#endif
				if ((tx_ops->peer_ops(peer, PEER_GET_CAPABILITY, NULL,
						   &peerAssocData) == QDF_STATUS_SUCCESS) && pe_priv) {
					if (!(peerAssocData.is_BTM_Supported) &&
							(!(peerAssocData.is_beacon_meas_supported))) {
						pe_priv->son_peer_class_group = 0;
					}
					assoc.client_class_group = pe_priv->son_peer_class_group;

					qdf_mem_copy(&assoc.client_bssid, &peerAssocData.bssid, IEEE80211_ADDR_LEN);
					assoc.isBTMSupported = peerAssocData.is_BTM_Supported;
					assoc.isRRMSupported = peerAssocData.is_RRM_Supported;
					assoc.isBeaconMeasurementSupported = peerAssocData.is_beacon_meas_supported;
					assoc.band_cap = peerAssocData.band_cap;
					assoc.client_htcap = peerAssocData.htcap;
					assoc.client_vhtcap = peerAssocData.vhtcap;
					for (i=0; i<RRM_CAPS_LEN; i++)
						assoc.rrm_caps[i] =  peerAssocData.rrmcaps[i];
					assoc.datarate_info.max_chwidth = peerAssocData.info.max_chwidth;
					assoc.datarate_info.num_streams = peerAssocData.info.num_streams;
					assoc.datarate_info.phymode = peerAssocData.info.phymode;
					assoc.datarate_info.max_txpower = peerAssocData.info.max_txpower;
					assoc.datarate_info.is_static_smps = peerAssocData.info.is_static_smps;
					assoc.datarate_info.is_mu_mimo_supported
						= peerAssocData.info.is_mu_mimo_supported;
					assoc.isbSTA = pe_priv->son_bSTA_flag;

					son_notify_user(vdev,
							ATH_EVENT_BSTEERING_NODE_ASSOCIATED,
							sizeof(assoc),
							(const char *) &assoc);
				}
				break;
			}
		case MLME_EVENT_CLIENT_DISCONNECTED:
			{
				struct bs_sta_stats_ind sta_stats_info = {0};
				struct bs_disconnect_ind disconnect_info = {0};
				struct wlan_client_disconnect_ind *client_data = NULL;
				struct wlan_sta_stats_ind disassoc = {0};
				struct wlan_disconnect_ind disconnect_event_data = {0};
				int i =0;

				if(!wlan_son_is_vdev_enabled(vdev) ||
						!wlan_son_is_pdev_enabled(pdev) )
					return -EINVAL;

				if (!event_data)
					return -EINVAL;

				client_data = (struct wlan_client_disconnect_ind *)event_data;
				disassoc = client_data->sta_stats_event_data;
				disconnect_event_data = client_data->disconnect_event_data;

				sta_stats_info.peer_count = disassoc.peer_count;
				for (i=0; i < sta_stats_info.peer_count; i++) {
					qdf_mem_copy(&sta_stats_info.peer_stats[i].client_addr,
							&disassoc.peer_stats[i].client_addr, IEEE80211_ADDR_LEN);
					qdf_mem_copy(&sta_stats_info.peer_stats[i].bssid,
							&disassoc.peer_stats[i].bssid, IEEE80211_ADDR_LEN);
					sta_stats_info.peer_stats[i].rssi = disassoc.peer_stats[i].rssi;
					sta_stats_info.peer_stats[i].per = disassoc.peer_stats[i].per;
					sta_stats_info.peer_stats[i].tx_byte_count = disassoc.peer_stats[i].tx_byte_count;
					sta_stats_info.peer_stats[i].rx_byte_count = disassoc.peer_stats[i].rx_byte_count;
					sta_stats_info.peer_stats[i].tx_packet_count = disassoc.peer_stats[i].tx_packet_count;
					sta_stats_info.peer_stats[i].rx_packet_count = disassoc.peer_stats[i].rx_packet_count;
					sta_stats_info.peer_stats[i].tx_error_packets = disassoc.peer_stats[i].tx_error_packets;
					sta_stats_info.peer_stats[i].tx_retrans = disassoc.peer_stats[i].tx_retrans;
					sta_stats_info.peer_stats[i].rx_error_packets = disassoc.peer_stats[i].rx_error_packets;
					sta_stats_info.peer_stats[i].tx_rate = disassoc.peer_stats[i].tx_rate;
					sta_stats_info.peer_stats[i].ack_rssi = disassoc.peer_stats[i].ack_rssi;
					sta_stats_info.peer_stats[i].is_disassoc_stats = disassoc.peer_stats[i].is_disassoc_stats;
				}

				qdf_mem_copy(&disconnect_info.client_addr,
						&disconnect_event_data.client_addr, IEEE80211_ADDR_LEN);
				disconnect_info.source = disconnect_event_data.source;
				disconnect_info.type =  disconnect_event_data.type;
				disconnect_info.reason = disconnect_event_data.reason;

				SON_LOGI("%s: event Bsteering client disconnected", __func__);
				son_notify_user(vdev, ATH_EVENT_BSTEERING_CLIENT_DISCONNECTED,
						sizeof(struct bs_disconnect_ind),
						(const char *) &disconnect_info);
				son_notify_user(vdev, ATH_EVENT_BSTEERING_STA_STATS,
						sizeof(struct bs_sta_stats_ind),
						(const char *) &sta_stats_info);

				if(son_has_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_SON)) {
					son_clear_whc_apinfo_flag(peer, IEEE80211_NODE_WHC_APINFO_SON);
					son_repeater_cnt_dec(vdev);
					son_pdev_appie_update(vdev);
				}
				break;
			}
		case MLME_EVENT_INST_RSSI:
			{
				struct wlan_peer_inst_rssi *ev = NULL;
				ev = (struct wlan_peer_inst_rssi *)event_data;

				if (!event_data)
					return -EINVAL;

				if (ev->valid)
					son_record_inst_peer_rssi(peer, ev->iRSSI);
				else
					son_record_inst_peer_rssi_err(peer);

				break;
			}

		case MLME_EVENT_TX_PWR_CHANGE:
			{
				u_int16_t *txpowlevel = NULL;

				if (!event_data)
					return -EINVAL;

				txpowlevel = (u_int16_t*)event_data;


				son_send_txpower_change_event(vdev, *txpowlevel);

				break;
			}
		case MLME_EVENT_VDEV_STATE:
			{
				struct son_vdev_priv *vdev_priv = NULL;
				struct wlan_vdev_state_event *ev = NULL;

				vdev_priv = wlan_son_get_vdev_priv(vdev);

				if (!event_data || !vdev_priv)
					return -EINVAL;

				ev = (struct wlan_vdev_state_event *)event_data;

				switch(ev->state)  {
					case VDEV_STATE_CREATED:
						if (vdev_priv->vdev_is_deleted)
							vdev_priv->vdev_is_deleted = 0;
						break;
					case VDEV_STATE_DELETED:
						vdev_priv->vdev_is_deleted = 1;
						break;
					case VDEV_STATE_STOPPED:
						son_send_vap_stop_event(vdev);
						break;
					case VDEV_STATE_RECOVER_CREATE:
						son_enable_disable_steering(vdev, true);
						break;
					case VDEV_STATE_RECOVER_DELETE:
						son_enable_disable_steering(vdev, false);
						break;
					case VDEV_STATE_CONNECTED:
						son_update_uplink_bssid(pdev, ev->bssid);
						break;
					case VDEV_STATE_DISCONNECTED:
						ucfg_son_set_root_dist(vdev,
								SON_INVALID_ROOT_AP_DISTANCE);
						break;
					case VDEV_STATE_WDS_ENABLED:
						break;
					case VDEV_STATE_WDS_DISABLED:
						break;
					default:
						return -EINVAL;
				}
				break;
			}
		case MLME_EVENT_MSCS:
			{
				SON_LOGI("%s: event ald mscs", __func__);

				mscs_info = (struct ald_mscs_event_info *)event_data;
				if (!mscs_info)
					return -EINVAL;

				son_send_mscs_event(vdev, mscs_info);
				break;
			}
		case MLME_EVENT_SCS:
			{
				SON_LOGI("%s: event ald scs", __func__);

				scs_info = (struct ald_scs_event_info *)event_data;
				if (!scs_info)
					return -EINVAL;

				son_send_scs_event(vdev, scs_info);
				break;
			}
		case MLME_EVENT_DSCP_POLICY_QUERY:
			{
				SON_LOGI("%s: event ald dscp query", __func__);

				dscp_query_info = (struct ald_dscp_policy_query_info *)event_data;
				if (!dscp_query_info)
					return -EINVAL;

				son_send_dscp_query_event(vdev, dscp_query_info);
				break;
			}
		case MLME_EVENT_DSCP_POLICY_RESPONSE:
			{
				SON_LOGI("%s: event ald dscp response", __func__);

				dscp_resp_info = (struct ald_dscp_policy_resp_info *)event_data;
				if (!dscp_resp_info)
					return -EINVAL;

				son_send_dscp_resp_event(vdev, dscp_resp_info);
				break;
			}
		default:
			return -EINVAL;
	}

	return 0;
}


/**
 * @brief To enable or disable peer ext stats.
 *
 * @param [in] peer.
 * @param [in] enable - enable or disable peer ext stats.
 *
 * @return 0 if success else error code
 */
int son_enable_disable_peer_ext_stats(struct wlan_objmgr_peer *peer,
				      uint32_t enable)
{
    return son_core_enable_disable_peer_ext_stats(peer, enable);
}

/**
 * @brief To set ald record free desc.
 *
 * @param [in] psoc.
 * @param [in] descs - free descs.
 *
 */
void son_ald_record_set_free_descs(struct wlan_objmgr_psoc *psoc,
			           u_int32_t descs)
{
	son_core_ald_record_set_free_descs(psoc, descs);
}

/**
 * @brief To get ald record free desc.
 *
 * @param [in] psoc.
 *
 */
u_int32_t son_ald_record_get_free_descs(struct wlan_objmgr_psoc *psoc)
{
	return son_core_ald_record_get_free_descs(psoc);
}

/**
 * @brief To set/reset ald record bufful warn.
 *
 * @param [in] psoc.
 * @param [in] enable - 1-enable, 0-disable
 *
 */
void son_ald_record_set_buff_full_warn(struct wlan_objmgr_psoc *psoc,
				       u_int8_t enable)
{
	son_core_ald_record_set_buff_full_warn(psoc, enable);
}

/**
 * @brief To get ald record bufful warn.
 *
 * @param [in] psoc.
 *
 */
u_int8_t son_ald_record_get_buff_full_warn(struct wlan_objmgr_psoc *psoc)
{
	return son_core_ald_record_get_buff_full_warn(psoc);
}

/**
 * @brief To set ald record free buff lvl.
 *
 * @param [in] psoc.
 * @param [in] thres - threshold.
 *
 */
void son_ald_record_set_buff_lvl(struct wlan_objmgr_psoc *psoc,
			         int thres)
{
	son_core_ald_record_set_buff_lvl(psoc, thres);
}

/**
 * @brief To get ald record free buff lvl.
 *
 * @param [in] psoc.
 *
 */
u_int16_t son_ald_record_get_buff_lvl(struct wlan_objmgr_psoc *psoc)
{
	return son_core_ald_record_get_buff_lvl(psoc);
}

/**
 * @brief To set ald record pool size.
 *
 * @param [in] psoc.
 * @param [in] sz - pool size.
 *
 */
void son_ald_record_set_pool_size(struct wlan_objmgr_psoc *psoc,
				  u_int32_t sz)
{
	son_core_ald_record_set_pool_size(psoc, sz);
}

/**
 * @brief To get ald record pool size.
 *
 * @param [in] psoc.
 *
 */
u_int32_t son_ald_record_get_pool_size(struct wlan_objmgr_psoc *psoc)
{
	return son_core_ald_record_get_pool_size(psoc);
}

#else

void son_update_uplink_snr(struct wlan_objmgr_vdev *vdev, u_int8_t snr)
{
	return;
}

void son_bs_stats_update_cb(void *pdev, enum WDI_EVENT event,
			    void *data, uint16_t data_len,
			    uint32_t status)
{
	return;
}

int8_t son_netlink_attach(void)
{
	return EOK;
}

int8_t son_netlink_destroy(void)
{
	return EOK;
}

void son_notify_activity_change(struct wlan_objmgr_vdev *vdev,
				char *macaddr, bool inactive)
{
	return;
}

void son_send_sta_stats_event(struct wlan_objmgr_vdev *vdev,
			      struct bs_sta_stats_ind *sta_stats)
{
	return;
}

void son_send_utilization_event(struct wlan_objmgr_vdev *vdev,
				u_int8_t chan_utilization,
				bool is_debug)
{
	return;
}

void son_send_rssi_measurement_event(struct wlan_objmgr_vdev *vdev,
			             const u_int8_t *mac_addr,
				     u_int8_t rssi,
				     bool is_debug)
{
	return;
}

void son_send_rssi_xing_event(struct wlan_objmgr_vdev *vdev,
			      const u_int8_t *mac_addr,
			      u_int8_t rssi,
			      BSTEERING_XING_DIRECTION inact_xing,
			      BSTEERING_XING_DIRECTION low_xing,
			      BSTEERING_XING_DIRECTION rate_xing,
			      BSTEERING_XING_DIRECTION ap_xing)
{
	return;
}

void son_send_rssi_xing_map_event(struct wlan_objmgr_vdev *vdev,
			          const u_int8_t *mac_addr,
				  u_int8_t rssi,
				  BSTEERING_XING_DIRECTION map_xing)
{
	return;
}

void son_send_tx_rate_xing_event(struct wlan_objmgr_vdev *vdev,
				 const u_int8_t *mac_addr,
				 u_int32_t tx_rate,
				 BSTEERING_XING_DIRECTION xing)
{
	return;
}

void son_send_tx_rate_measurement_event(struct wlan_objmgr_vdev *vdev,
				        const u_int8_t *mac_addr,
				        u_int32_t tx_rate)
{
	return;
}

void son_send_vap_stop_event(struct wlan_objmgr_vdev *vdev)
{
	return;
}

void son_send_txpower_change_event(struct wlan_objmgr_vdev *vdev,
				   u_int16_t tx_power)
{
	return;
}

/**
  * @brief provides per-user rx_stats including frame_control and qos_control
  *
  * callback api to identify the peer and derive queue_size from frame_control
  * and qos_control
  *
  * @param [in] pointer to psoc object
  * @param [in] WDI event enum value
  * @param [in] data pointer having qos stats
  * @param [in] data length
  * @param [in] status
  *
  * @param [inout] void.
  */
void son_qos_stats_update_cb(void *psoc_obj, enum WDI_EVENT event,
			     void *data, uint16_t data_len,
			     uint32_t status)
{
	return;
}

/**
 * @brief Verify that the son handle is valid within the
 *        struct psoc provided.
 *
 * @param [in] psoc  the handle to the radio where the band steering state
 *                 resides
 *
 * @return true if handle is valid; otherwise false
 */

bool wlan_son_is_pdev_valid(struct wlan_objmgr_pdev *pdev)
{
	return false;
}

/**
 * @brief Determine whether the band steering module is enabled or not.
 *
 * @param [in] pdev  the handle to the radio where the band steering state
 *                 resides
 *
 * @return non-zero if it is enabled; otherwise 0
 */

u_int8_t wlan_son_is_pdev_enabled(struct wlan_objmgr_pdev *pdev)
{
	return false;
}
/**
 * @brief Determine whether band steering events are enabled on
 *        a VAP.
 *
 * @param [in] vap  VAP to check
 *
 * @return non-zero if it is enabled; otherwise 0
 */

u_int8_t wlan_son_is_vdev_event_enabled(struct wlan_objmgr_vdev *vdev)
{
	return false;

}

/**
 * @brief Determine whether the VAP handle is valid, has a valid band
 *        steering handle, is operating in a mode where band steering
 *        is relevant, and is not in the process of being deleted.
 *
 * @return true if the VAP is valid; otherwise false
 */

bool wlan_son_is_vdev_valid(struct wlan_objmgr_vdev *vdev)
{
	return false;
}

/**
 * @brief Determine whether the VAP has band steering enabled.
 *
 * Validate that the VAP has a valid band steering handle, that
 * it is operating in the right mode (AP mode), and that band steering has been
 * enabled on the VAP.
 *
 * @param [in] vap  the VAP to check
 *
 * @return true if the VAP is valid and has band steering enabled; otherwise
 *         false
 */

bool wlan_son_is_vdev_enabled(struct wlan_objmgr_vdev *vdev)
{
	return false;
}

void son_peer_authorize(struct wlan_objmgr_peer *peer)
{
	return;
}

void son_mark_node_inact(struct wlan_objmgr_peer *peer, bool inactive)
{

	return;
}

void son_record_act_change(struct wlan_objmgr_pdev *pdev,
		           u_int8_t *mac_addr,
			   bool active)
{

	return;
}

void son_record_inst_peer_rssi_err(struct wlan_objmgr_peer *peer)
{

	return;

}

void son_record_inst_peer_rssi(struct wlan_objmgr_peer *peer,
			       u_int8_t rssi)
{
	return;

}

void son_record_peer_rssi(struct wlan_objmgr_peer *peer, u_int8_t rssi)
{
	return;
}


int32_t son_match_peer_rssi_seq(struct wlan_objmgr_peer *peer,
			        u_int32_t rssi_seq)
{
	return - EINVAL;

}


void son_update_peer_rate(struct wlan_objmgr_peer *peer,
			  u_int32_t rssi, u_int32_t last_rate)
{
	return;
}

bool son_update_sta_stats(struct wlan_objmgr_peer *peer,
		          struct wlan_objmgr_vdev *current_vdev,
			  struct bs_sta_stats_ind *sta_stats,
			  struct cdp_interface_peer_stats *stats)
{
	return false;
}

int son_enable_disable_steering(struct wlan_objmgr_vdev *vdev,
				 bool enable)
{
    return 0;
}

void son_set_vdev_lbd_pid(struct wlan_objmgr_vdev *vdev, u_int32_t pid)
{
	return;
}

bool son_is_probe_resp_wh_2G(struct wlan_objmgr_vdev *vdev,
			     u_int8_t *mac_addr,
			     u_int8_t sta_rssi)
{
	return false;
}


bool son_is_probe_resp_wh(struct wlan_objmgr_vdev *vdev,
		          const u_int8_t *mac_addr, u_int8_t probe_rssi)
{
	return false;


}

void son_repeater_cnt_inc(struct wlan_objmgr_vdev *vdev)
{
	return;
}


void son_repeater_cnt_dec(struct wlan_objmgr_vdev *vdev)
{
	return;
}

void son_update_nss(struct wlan_objmgr_vdev *vdev, u_int8_t nss)
{
        return;
}

u_int8_t son_repeater_cnt_get(struct wlan_objmgr_vdev *vdev)
{
	return 0;
}

void son_record_inst_rssi_log_enable (struct wlan_objmgr_vdev *vdev, int enable)
{
	return;
}


void son_update_backhaul_rate(struct wlan_objmgr_vdev *vdev, u_int16_t rate, bool self)
{

	return;
}


u_int16_t son_get_backhaul_rate(struct wlan_objmgr_vdev *vdev, bool self)
{
	return 0;

}


void son_update_uplink_bssid(struct wlan_objmgr_pdev *pdev , char *bssid)
{
	return;
}



/**
 * get uplink estimated rate
 * @param      vaphandle   : vap handle
 * @param      snr         : rssi value from scan entry
 *
 * @return : estimated rate.
 *           only valid for STA vaps;
 *
 */
u_int16_t son_get_uplinkrate(struct wlan_objmgr_vdev *vdev,
			     u_int8_t snr)

{
	return 0;
}


/**
 * Function work under vdev_lock.
 * @brief: Change feature extented capa per vdev.
 * @param  vaphandle   : vap handle
 * @param action: SET, GET and clear.
 * @param cap: capabilites to change.
 * @return :in case of get it return positive value if cap
 *           is set, defualt is EOK and it can return -EINVAL
 *           if vdev is null.
 *           only valid for STA vaps;
 */

u_int8_t son_vdev_fext_capablity(struct wlan_objmgr_vdev *vdev,
			         son_capability_action action,
				 u_int32_t cap)
{
	return EOK;
}

/**
 * Function work under vdev_lock.
 * @brief: Change feature capablities per vdev.
 * @param  vaphandle   : vap handle
 * @param action: SET, GET and clear.
 * @param cap: capabilites to change.
 * @return :in case of get it return positive value if cap
 *           is set, defualt is EOK and it can return -EINVAL
 *           if vdev is null.
 *           only valid for STA vaps;
 */

u_int8_t son_vdev_feat_capablity(struct wlan_objmgr_vdev *vdev,
			         son_capability_action action,
				 u_int32_t cap)
{
	return EOK;
}

/**
 * Function work under vdev_lock.
 * @brief: Get the count of son enabled vdev .
 * @param  vaphandle   : vap handle
 * @param action: GET
 * @return :It returns the number of vaps for which SON is enabled
 *            default is EOK and it can return -EINVAL
 *           if vdev is null.
 *
 */
u_int8_t son_vdev_get_count(struct wlan_objmgr_vdev *vdev,
                            son_capability_action action)
{
        return EOK;
}

int son_vdev_map_capability_set(struct wlan_objmgr_vdev *vdev, son_map_capability cap, int val)
{
	return EOK;
}

int son_vdev_map_capability_get(struct wlan_objmgr_vdev *vdev, son_map_capability cap)
{
	return EOK;
}

bool son_is_steer_in_prog(struct wlan_objmgr_peer *peer)
{
	return false;

}

void son_set_whc_apinfo_flag(struct wlan_objmgr_peer *peer, u_int8_t flag)
{
	return;
}

void son_clear_whc_apinfo_flag(struct wlan_objmgr_peer *peer, u_int8_t flag)
{
	return;
}

int son_has_whc_apinfo_flag(struct wlan_objmgr_peer *peer, u_int8_t flag)
{
	return -EINVAL;
}

void son_set_whc_rept_info(struct wlan_objmgr_peer *peer)
{
	return;
}

void son_clear_whc_rept_info(struct wlan_objmgr_peer *peer)
{
	return;
}

int32_t son_get_whc_rept_info(struct wlan_objmgr_peer *peer)
{
	return EOK;
}

int8_t ucfg_son_set_root_dist(struct wlan_objmgr_vdev *vdev,
			      u_int8_t root_distance)
{
	return EOK;
}
u_int8_t ucfg_son_get_root_dist(struct wlan_objmgr_vdev *vdev)
{
	return 0;

}

int8_t son_get_innetwork_table(struct wlan_objmgr_vdev *vdev, void *data, int *num_entries,
                               int8_t channel)
{
	return false;
}

bool son_set_backhaul_type_mixedbh(struct wlan_objmgr_vdev *vdev, u_int8_t backhaul_type)
{
	return false;
}

bool son_set_ul_mixedbh(struct wlan_objmgr_vdev *vdev, u_int16_t ulrate)
{
	return false;
}

u_int16_t son_get_ul_mixedbh(struct wlan_objmgr_vdev *vdev)
{
	return false;
}

int son_update_mgmt_frame(struct wlan_objmgr_vdev *vdev,
			  struct wlan_objmgr_peer *peer, int subtype,
			  u_int8_t *frame, u_int16_t frame_len,
			  void *meta_data)
{
		return 0;
}

int son_update_mlme_event(struct wlan_objmgr_vdev *vdev,
			  struct wlan_objmgr_peer *peer,
			  enum ieee80211_event_type event, void *event_data)
{
		return -EINVAL;
}

void son_pdev_appie_update(struct wlan_objmgr_vdev *vdev)
{
	return;
}

int son_update_appielist(struct wlan_objmgr_vdev *vdev, ieee80211_frame_type ftype)
{
	return 0;
}

int son_enable_disable_peer_ext_stats(struct wlan_objmgr_peer *peer,
			              uint32_t enable)
{
	return 0;
}

void son_ald_record_set_free_descs(struct wlan_objmgr_psoc *psoc,
				   u_int32_t descs)
{
	return;
}

u_int32_t son_ald_record_get_free_descs(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

void son_ald_record_set_buff_full_warn(struct wlan_objmgr_psoc *psoc,
				       u_int8_t enable)
{
	return;
}

u_int8_t son_ald_record_get_buff_full_warn(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

void son_ald_record_set_buff_lvl(struct wlan_objmgr_psoc *psoc,
				 int thres)
{
	return;
}

u_int16_t son_ald_record_get_buff_lvl(struct wlan_objmgr_psoc *psoc)
{
	return false;
}

void son_ald_record_set_pool_size(struct wlan_objmgr_psoc *psoc,
				  u_int32_t sz)
{
	return;
}

u_int32_t son_ald_record_get_pool_size(struct wlan_objmgr_psoc *psoc)
{
	return false;
}
#endif
