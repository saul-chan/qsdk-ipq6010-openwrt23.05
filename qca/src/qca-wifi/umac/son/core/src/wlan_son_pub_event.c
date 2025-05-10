/*
 * Copyright (c) 2017, 2021 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

/* This file implements all public function api for WIFI SON */

#include <osif_private.h>
#include "wlan_son_internal.h"
#include "wlan_son_pub.h"
#include <ath_band_steering.h>
#include <wlan_osif_priv.h>

#if QCA_SUPPORT_SON
/**
 * @brief  Attach module to open socket to user space.
 * @param [in]
 *
 * @param [inout] void .
 */

int8_t son_netlink_attach(void)
{
	int8_t retv = EOK;

	if ( EOK != ath_band_steering_netlink_init()) {
		SON_LOGF("SON Socket init failed");
		retv = -EINVAL;
	}

	return retv;
}

/**
 * @brief To close socket to user layer.
 * @param [in] void
 *
 * @param [inout] void .
 */

int8_t son_netlink_destroy(void)
{
	int8_t retv = EOK;

	if ( EOK != ath_band_steering_netlink_delete()) {
		SON_LOGF("SON Socket init failed");
		retv = -EINVAL;
	}

	return retv;
}

/**
 * @brief
 * @param [in]
 *
 * @param [inout] void .
 */

void son_notify_user(struct wlan_objmgr_vdev *vdev,
			    ATH_BSTEERING_EVENT type,
			    uint32_t len, const char *data)
{
	struct net_device *dev = NULL;
	ath_netlink_bsteering_event_t  netlink_event = {0};
	ath_netlink_bsteering_event_t  *p_netlink_event;
	uint32_t event_size = sizeof(netlink_event);
	struct vdev_osif_priv *osif_priv = NULL;
	struct son_vdev_priv *vd_priv = NULL;

	vd_priv = wlan_son_get_vdev_priv(vdev);
	if (!vd_priv)
		return;

	osif_priv = wlan_vdev_get_ospriv(vdev);

	if (!osif_priv) {
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		return;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
            SON_LOGE("%s: wireless dev is NULL ", __func__);
            return;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		return;
	}

	if (len > sizeof(netlink_event.data)) {
		event_size = event_size + len - sizeof(netlink_event.data);
		p_netlink_event = (ath_netlink_bsteering_event_t *) qdf_mem_malloc(event_size);
	} else {
		p_netlink_event = &netlink_event;
	}

	if (p_netlink_event) {
		memset(p_netlink_event, 0x0, event_size);
		p_netlink_event->type = type;
		p_netlink_event->sys_index = dev->ifindex;

		if (len && data) {
			qdf_mem_copy(&(p_netlink_event->data), data, len);
		}

		ath_band_steering_netlink_send(p_netlink_event, vd_priv->lbd_pid, wlan_son_is_vdev_event_bcast_enabled(vdev), event_size);

		if (p_netlink_event != &netlink_event) {
			OS_FREE(p_netlink_event);
		}
	}
	return;
}

/**
 * @brief Send STA stats to userspace via netlink message
 *
 * @param [in] vdev  the first vdev on the radio for which STA
 *                  stats were updated
 * @param [in] sta_stats  the STA stats to send
 */
PUBLIC void son_send_sta_stats_event(struct wlan_objmgr_vdev *vdev,
				     struct bs_sta_stats_ind *sta_stats)
{

	if (!sta_stats || !sta_stats->peer_count || !vdev) {
		/* No stats collected, nothing to send */
		return;
	}

	/* No need to do any checks here - should not have any updated stats unless
	   band steering and interference detection are enabled */

	son_notify_user(vdev, ATH_EVENT_BSTEERING_STA_STATS,
			sizeof(struct bs_sta_stats_ind),
			(const char *)sta_stats);
	return;
}
/**
 * @brief send activity change event to user space.
 * @param [in] vdev, vdev to which station is connected.
 * @param [in] macaddr, sta mac address.
 * @param [in] boolean flag indicating activity of client.
 * @param [inout] void .
 */

void son_notify_activity_change(struct wlan_objmgr_vdev *vdev,
				       char *macaddr,
				       bool activity)
{
	struct bs_activity_change_ind ind = {0};
	struct wlan_objmgr_pdev *pdev = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	if(!wlan_son_is_pdev_enabled(pdev)) {
		return;
	}

	if(!wlan_son_is_vdev_enabled(vdev))
		return;

	WLAN_ADDR_COPY(ind.client_addr, macaddr);
	ind.activity = activity ? 1 : 0;
	son_notify_user(vdev,
			ATH_EVENT_BSTEERING_CLIENT_ACTIVITY_CHANGE,
			sizeof(struct bs_activity_change_ind),
			(const char *) &ind);
	return;
}

/**
 * @brief Generate an event with the provided channel utilization measurement.
 *
 * @pre vap has already been checked and confirmed to be valid and band
 *      steering has been confirmed to be enabled
 *
 * @param [in] vap  the VAP that was used for the sampling
 * @param [in] chan_utilization  the utilization in percent
 * @param [in] is_debug  whether the log generated should be a debug event
 *                       or a regular event; debug events represent an
 *                       instantaneous measurement whereas normal (non-debug)
 *                       represent a filtered/averaged measurement
 */

PUBLIC void son_send_utilization_event(struct wlan_objmgr_vdev *vdev,
				       u_int8_t chan_utilization,
				       bool is_debug)
{
	struct bs_chan_utilization_ind ind;
	ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_CHAN_UTIL;

	ind.utilization = chan_utilization;

	if (is_debug) {
		event = ATH_EVENT_BSTEERING_DBG_CHAN_UTIL;
	}

	son_notify_user(vdev, event, sizeof(ind),
			(const char *) &ind);

	return;
}

/**
 * @brief Send an event to user space when requested RSSI measurement is available
 *
 * @pre vap has already been checked and confirmed to be valid and band
 *      steering has already been confirmed to be enabled
 *
 * @param [in] vap  the VAP that the client whose RSSI is measured associated to
 * @param [in] mac_addr  the MAC address of the client
 * @param [in] rssi  the measured RSSI
 * @param [in] is_debug  whether the log generated should be a debug event
 *                       or a regular event; debug events represent an
 *                       instantaneous (but averaged by firmware already) RSSI
 *                       measurement whereas normal (non-debug) represent the
 *                       RSSI measured by sending NDPs
 */
void son_send_rssi_measurement_event(struct wlan_objmgr_vdev *vdev,
				     const u_int8_t *mac_addr,
				     u_int8_t rssi,
				     bool is_debug)
{
	struct bs_rssi_measurement_ind ind;
	ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_CLIENT_RSSI_MEASUREMENT;

	qdf_mem_copy(ind.client_addr, mac_addr, QDF_MAC_ADDR_SIZE);
	ind.rssi = rssi;

	if (is_debug) {
		event = ATH_EVENT_BSTEERING_DBG_RSSI;
	}

	son_notify_user(vdev, event, sizeof(ind),
			(const char *) &ind);
}

/**
 * @brief Generate an event when Tx power change on a VAP
 *
 * @param [in] vap  the VAP on which Tx power changes
 * @param [in] tx_power  the new Tx power
 */
void son_send_txpower_change_event(struct wlan_objmgr_vdev *vdev,
				   u_int16_t tx_power)
{
	struct bs_tx_power_change_ind event = {0};
	struct wlan_objmgr_pdev *pdev = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!wlan_son_is_vdev_enabled(vdev) ||
	    !wlan_son_is_pdev_valid(pdev)) {
		return;
	}

	if (!wlan_son_is_pdev_enabled(pdev))
		return;


	/* Tx power is stored in half dBm */
	event.tx_power = tx_power / 2;
	son_notify_user(vdev, ATH_EVENT_BSTEERING_TX_POWER_CHANGE,
			sizeof(struct bs_tx_power_change_ind),
			(const char *)&event);

	return;
}
/**
 * @brief Send an event to user space on RSSI measurement crossed threshold
 *
 * @pre vap has already been checked and confirmed to be valid and band
 *      steering has confirmed to be enabled
 *
 * @param [in] vap  the VAP that the client whose RSSI is measured associated to
 * @param [in] mac_addr  the MAC address of the client
 * @param [in] rssi  the measured RSSI
 * @param [in] inact_xing  flag indicating if the RSSI crossed inactivity RSSI threshold.
 * @param [in] low_xing  flag indicating if the RSSI crossed low RSSI threshold
 * @param [in] rate_xing  flag indicating if the RSSI crossed
 *                        the rate RSSI threshold
 * @param [in] ap_xing  flag indicating if the RSSI crossed the AP steering RSSI threshold
 */
void son_send_rssi_xing_event(struct wlan_objmgr_vdev *vdev,
			      const u_int8_t *mac_addr,
			      u_int8_t rssi,
			      BSTEERING_XING_DIRECTION inact_xing,
			      BSTEERING_XING_DIRECTION low_xing,
			      BSTEERING_XING_DIRECTION rate_xing,
			      BSTEERING_XING_DIRECTION ap_xing)
{
	struct bs_rssi_xing_threshold_ind ind;
	ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_CLIENT_RSSI_CROSSING;

	qdf_mem_copy(ind.client_addr, mac_addr, QDF_MAC_ADDR_SIZE);
	ind.rssi = rssi;
	ind.inact_rssi_xing = inact_xing;
	ind.low_rssi_xing = low_xing;
	ind.rate_rssi_xing = rate_xing;
	ind.ap_rssi_xing = ap_xing;

	son_notify_user(vdev, event, sizeof(ind),
			(const char *) &ind);
}

/**
 * @brief Send an event to user space on RSSI measurement crossing threshold for MAP
 *
 * @pre vap has already been checked and confirmed to be valid and band
 *      steering has confirmed to be enabled
 *
 * @param [in] vdev that the client whose RSSI is measured associated to
 * @param [in] mac_addr  the MAC address of the client
 * @param [in] rssi  the measured RSSI
 */
void son_send_rssi_xing_map_event(struct wlan_objmgr_vdev *vdev,
				  const u_int8_t *mac_addr,
				  u_int8_t rssi,
				  BSTEERING_XING_DIRECTION map_xing)
{
	struct bs_rssi_xing_threshold_ind_map ind;
	ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_MAP_CLIENT_RSSI_CROSSING;

	qdf_mem_copy(ind.client_addr, mac_addr, QDF_MAC_ADDR_SIZE);
	ind.rssi = rssi;
	ind.map_rssi_xing = map_xing;

	son_notify_user(vdev, event, sizeof(ind),
			(const char *) &ind);
}

/**
 * @brief Send an event to user space if the Tx rate crossed a
 *        threshold
 *
 * @pre vdev has already been checked and confirmed to be valid
 *      and band steering has confirmed to be enabled
 *
 * @param [in] vdev  the VAP that the client whose Tx rate is
 *                  measured associated to
 * @param [in] mac_addr  the MAC address of the client
 * @param [in] tx_rate  the Tx rate
 * @param [in] xing  flag indicating the direction of the Tx
 *                   rate crossing.
 */
void son_send_tx_rate_xing_event(
	struct wlan_objmgr_vdev *vdev,
	const u_int8_t *mac_addr,
	u_int32_t tx_rate,
	BSTEERING_XING_DIRECTION xing)
{
	struct bs_tx_rate_xing_threshold_ind ind;
	ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_CLIENT_TX_RATE_CROSSING;

	qdf_mem_copy(ind.client_addr, mac_addr, QDF_MAC_ADDR_SIZE);
	ind.tx_rate = tx_rate;
	ind.xing = xing;

	son_notify_user(vdev, event, sizeof(ind),
			(const char *) &ind);
	return;
}

/**
 * @brief Send an event to user space when the Tx rate changes.
 *        Note this is a debug only message.
 *
 * @pre vap has already been checked and confirmed to be valid and band
 *      steering has already been confirmed to be enabled
 *
 * @param [in] vap  the VAP that the client whose RSSI is measured associated to
 * @param [in] mac_addr  the MAC address of the client
 * @param [in] tx_rate  the latest Tx rate
 */
void son_send_tx_rate_measurement_event(struct wlan_objmgr_vdev *vdev,
					const u_int8_t *mac_addr,
					u_int32_t tx_rate)
{
	struct bs_tx_rate_measurement_ind ind;
	ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_DBG_TX_RATE;

	qdf_mem_copy(ind.client_addr, mac_addr, QDF_MAC_ADDR_SIZE);
	ind.tx_rate = tx_rate;

	son_notify_user(vdev, event, sizeof(ind),
			(const char *) &ind);

	return;
}

/**
 * @brief Called when a VAP is stopped (this is only seen on a
 *        RE when the uplink STA interface is disassociated)
 *
 * @param [in] vap  VAP that has stopped
 */

void son_send_vap_stop_event(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct son_pdev_priv *pd_priv = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	if(!wlan_son_is_vdev_enabled(vdev) ||
	   !wlan_son_is_pdev_valid(pdev) ) {
		return;
	}

	pd_priv = wlan_son_get_pdev_priv(pdev);

	SON_LOCK(&pd_priv->son_lock);

	if (wlan_son_is_pdev_enabled(pdev)) {
		ATH_BSTEERING_EVENT event = ATH_EVENT_BSTEERING_VAP_STOP;

		/* No data to send with this event */
		son_notify_user(vdev, event, 0, NULL);
	}

	SON_UNLOCK(&pd_priv->son_lock);
	return;
}

 /**
 * @brief Called when ALD stats data is sent to user space
 * @param [in] vdev reference to fetch pid
 * @param [in] type ALD_event type
 * @param [in] len  buffer length
 * @param [in] data buffer pointer
 */

void ald_notify_user(struct wlan_objmgr_vdev *vdev,
        ATH_BSTEERING_EVENT type,
        uint32_t len, void *data)
{
    struct son_vdev_priv *vd_priv = NULL;
    u_int32_t pid = WLAN_DEFAULT_NETLINK_PID;

    if ( vdev == NULL)
        return;

    vd_priv = wlan_son_get_vdev_priv(vdev);
    if (!vd_priv)
        return;

    pid = vd_priv->lbd_pid;

    if (pid == WLAN_DEFAULT_NETLINK_PID)
        return;

    ald_netlink_send(type, pid,wlan_son_is_vdev_event_bcast_enabled(vdev),len, data);
}

#if QCA_SUPPORT_SSID_STEERING
/**
 * @brief  Attach module to open socket to user space.
 * @param [in] void.
 *
 * @return EOK for success and -EINVAL for failure.
 */
int8_t son_ssid_steering_netlink_attach(void)
{
	int8_t retv = EOK;

	if ( EOK != ath_ssid_steering_netlink_init()) {
		SON_LOGF("SON Socket init failed");
		retv = -EINVAL;
	}

	return retv;
}

/**
 * @brief To close socket to user layer.
 * @param [in] void.
 *
 * @return EOK for success and -EINVAL for failure.
 */
int8_t son_ssid_steering_netlink_destroy(void)
{
	int8_t retv = EOK;

	if ( EOK != ath_ssid_steering_netlink_delete()) {
		SON_LOGF("SON Socket init failed");
		retv = -EINVAL;
	}

	return retv;
}

/**
 * @brief Event to send macaddress of client to user layer.
 *
 * @param [in] macaddress of client.
 *
 * @return void.
 */
void son_send_ssid_steering_event(u_int8_t *macaddr)
{
	ssidsteering_event_t    ssidsteering;

	qdf_mem_zero(&ssidsteering, sizeof(ssidsteering));
	ssidsteering.type = ATH_EVENT_SSID_NODE_ASSOCIATED;
	qdf_mem_copy(ssidsteering.mac, macaddr, QDF_MAC_ADDR_SIZE);

	ath_ssid_steering_netlink_send(&ssidsteering);
}
#endif
#endif
