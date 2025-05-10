/*
 * Copyright (c) 2015, 2017, 2019-2021 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2015 Qualcomm Atheros, Inc.
 *
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/*
 *  Copyright (c) 2010, Atheros Communications Inc.
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
#include "linux/if.h"
#include "linux/socket.h"
#include <net/rtnetlink.h>
#include <net/sock.h>

#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cache.h>
#include <linux/proc_fs.h>

#if QCA_SUPPORT_SON
#include "ald_netlink.h"
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_vdev_if.h>
#endif

#include <wlan_son_pub.h>
#include "wlan_son_internal.h"
#include <wlan_osif_priv.h>
#include <wlan_utility.h>
#include <dp_txrx.h>

struct ald_netlink *ald_nl = NULL;

void son_ald_send_info_iter_func(struct wlan_objmgr_vdev *vdev)
{
	struct net_device *dev;
	struct ald_stat_info *info = NULL;
        struct vdev_osif_priv *osif_priv = NULL;
	struct son_vdev_priv *vd_priv = NULL;

	info = qdf_mem_malloc(sizeof(struct ald_stat_info));
	if (info == NULL)
		return;

	if ((vdev == NULL) ||
	    !(wlan_vdev_is_up(vdev) == QDF_STATUS_SUCCESS)) {
		goto err;
	}

	vd_priv = wlan_son_get_vdev_priv(vdev);
	if (!vd_priv)
		goto err;

	if ((wlan_vdev_is_up(vdev) == QDF_STATUS_SUCCESS)) {
		info->cmd = IEEE80211_ALD_ALL;

		osif_priv = wlan_vdev_get_ospriv(vdev);
		if (!osif_priv) {
			SON_LOGE("%s: osif_priv is NULL ", __func__);
			goto err;
		}

                /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
                if ( !osif_priv->wdev ) {
                    SON_LOGE("%s: wireless dev is NULL ", __func__);
                    goto err;
                }

		dev = osif_priv->wdev->netdev;
		if (!dev) {
			SON_LOGE("%s: dev is NULL ", __func__);
			goto err;
		}

		if (strlcpy(info->name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
			SON_LOGE("%s: source too long",__func__);
			goto err;
		}

		vd_priv->iv_ald->staticp = info;
		son_ald_get_statistics(vdev, NULL);
		vd_priv->iv_ald->staticp = NULL;
    ald_notify_user(vdev, IEEE80211_ALD_ALL, sizeof(struct ald_stat_info),(void*)info);
	}

err:
	qdf_mem_free(info);
}

int son_ald_assoc_notify(struct wlan_objmgr_vdev *vdev, u_int8_t *macaddr,
			 u_int8_t aflag, u_int16_t reasonCode)
{
	struct net_device *dev = NULL;
	wlan_chan chan = {0};
	wlan_chan *loc_chan = NULL;
	struct ald_assoc_info info;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;
        struct mlme_external_tx_ops *tx_ops = NULL;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
            SON_LOGE("%s: wireless dev is NULL ", __func__);
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info.cmd = IEEE80211_ALD_ASSOCIATE;
	if (strlcpy(info.name, dev->name , IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	tx_ops = wlan_son_get_tx_ops(vdev);
	if (!tx_ops || !tx_ops->vdev_ops) {
		SON_LOGE("%s: tx_ops/vdev_ops is null ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
        tx_ops->vdev_ops(vdev, VDEV_GET_CHAN, NULL, &chan);
        loc_chan = &chan;

	if (!IEEE80211_IS_CHANNEL_VALID(loc_chan)) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	if(loc_chan->ic_freq * 100000 < 500000)
		info.afreq = ALD_FREQ_24G;
	else
		info.afreq = ALD_FREQ_5G;
	info.aflag = aflag;
	qdf_mem_copy(info.macaddr, macaddr, QDF_MAC_ADDR_SIZE);
	info.reasonCode = reasonCode;
  ald_notify_user(vdev,IEEE80211_ALD_ASSOCIATE, sizeof(info),(void *)&info);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function for 75% buffers full warning */
int son_ald_buffull_notify(struct wlan_objmgr_vdev *vdev)
{
	struct net_device *dev = NULL;
	struct ald_buffull_info info;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            SON_LOGE("%s: wireless dev is NULL ", __func__);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info.cmd = IEEE80211_ALD_BUFFULL_WRN;
	if (strlcpy(info.name, dev->name , IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	info.resv = ATH_TXBUF;
  ald_notify_user(vdev,IEEE80211_ALD_BUFFULL_WRN, sizeof(info),(void *)&info);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to notify CBS event */
int son_ald_cbs_notify(struct wlan_objmgr_vdev *vdev, ald_cbs_event_type type)
{
	struct net_device *dev = NULL;
	struct ald_cbs_info info;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
            wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            SON_LOGE("%s: wireless dev is NULL ", __func__);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info.cmd = IEEE80211_ALD_CBS;
	if (strlcpy(info.name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long\n", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	info.type = type;
  ald_notify_user(vdev, IEEE80211_ALD_CBS, sizeof(info),(void*)&info);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to notify ACS Neighbour Scan complete */
int son_ald_acs_complete_notify(struct wlan_objmgr_vdev *vdev)
{
	struct net_device *dev = NULL;
	struct ald_cbs_info info;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		SON_LOGE("%s: osif_priv is NULL ", __func__);
                wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
            SON_LOGE("%s: wireless dev is NULL ", __func__);
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info.cmd = IEEE80211_ALD_ACS_COMPLETE;
	if (strlcpy(info.name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long\n", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	info.type = ALD_CBS_COMPLETE;
  ald_notify_user(vdev,IEEE80211_ALD_ACS_COMPLETE, sizeof(info),(void*)&info);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

        return 0;
}

/* function to notify CAC complete */
int son_ald_cac_complete_notify(struct wlan_objmgr_vdev *vdev, u_int8_t radar_detected)
{
	struct net_device *dev = NULL;
	struct ald_cac_complete_info info;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            SON_LOGE("%s: wireless dev is NULL ", __func__);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info.cmd = IEEE80211_ALD_CAC_COMPLETE;
	if (strlcpy(info.name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long\n", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	info.radar_detected = radar_detected;
  ald_notify_user(vdev,IEEE80211_ALD_CAC_COMPLETE,sizeof(info),(void*)&info);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to notify change in BSS assoc allowance status */
int son_ald_assoc_allowance_status_notify(struct wlan_objmgr_vdev *vdev,
					  u_int8_t *bssid, u_int8_t assoc_status)
{
	struct net_device *dev = NULL;
	struct ald_assoc_allowance_info info;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            SON_LOGE("%s: wireless dev is NULL ", __func__);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info.cmd = IEEE80211_ALD_ASSOC_ALLOWANCE_STATUS_CHANGE;
	if (strlcpy(info.name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long\n", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	qdf_mem_copy(info.bssid, bssid, IEEE80211_ADDR_LEN);
	info.assoc_status = assoc_status;
  ald_notify_user(vdev,IEEE80211_ALD_ASSOC_ALLOWANCE_STATUS_CHANGE, sizeof(info),(void*)&info);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to send WNM request frame to userspace */
int son_ald_wnm_frame_recvd_notify(struct wlan_objmgr_vdev *vdev, u_int8_t action,
				   u_int8_t *macaddr, u_int8_t *frame, uint16_t frame_len)
{
	struct net_device *dev = NULL;
	struct ald_wnm_frame_info *info = NULL;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	if (frame_len > ALD_MAX_FRAME_SZ) {
	    SON_LOGE("%s: Source frame too long", __func__);
	    return -1;
	}

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
            SON_LOGE("%s: wireless dev is NULL ", __func__);
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info = qdf_mem_malloc(sizeof(struct ald_wnm_frame_info));
	if (info == NULL) {
		SON_LOGF("Insufficient memory can not allocate ald_wnm_frame ");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	info->cmd = action;
	if (strlcpy(info->name, dev->name, IFNAMSIZ) >= IFNAMSIZ) {
		SON_LOGE("%s: source too long\n", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		qdf_mem_free(info);
		return -1;
	}
	qdf_mem_copy(info->macaddr, macaddr, IEEE80211_ADDR_LEN);
	qdf_mem_copy(info->frame, frame, frame_len);
	info->frameSize = frame_len;
  ald_notify_user(vdev, IEEE80211_ALD_WNM_FRAME_RECEIVED, sizeof(struct ald_wnm_frame_info),(void*)info);

	qdf_mem_free(info);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to send ANQP neighbor report request frame to userspace */
int son_ald_anqp_frame_recvd_notify(struct wlan_objmgr_vdev *vdev, u_int8_t *macaddr,
				    u_int8_t *frame, uint16_t frame_len)
{
	struct net_device *dev;
	struct ald_anqp_frame_info *info = NULL;
        struct vdev_osif_priv *osif_priv = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

	osif_priv = wlan_vdev_get_ospriv(vdev);
	if (!osif_priv) {
		SON_LOGE("%s: osif_priv is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

        /* wireless dev (wdev) would be NULL if cfg80211 is disabled */
        if ( !osif_priv->wdev ) {
            SON_LOGE("%s: wireless dev is NULL ", __func__);
	    wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
            return -1;
        }

	dev = osif_priv->wdev->netdev;
	if (!dev) {
		SON_LOGE("%s: dev is NULL ", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}

	info = qdf_mem_malloc(sizeof(struct ald_anqp_frame_info));
	if (info == NULL) {
		SON_LOGF("Insufficient memory can not allocate ald_anqp_frame ");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		return -1;
	}
	if ((frame_len > ALD_MAX_FRAME_SZ) ||
			(strlcpy(info->name, dev->name, IFNAMSIZ) >= IFNAMSIZ)) {
		SON_LOGE("%s: source too long\n", __func__);
		wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);
		qdf_mem_free(info);
		return -1;
	}
	qdf_mem_copy(info->macaddr, macaddr, IEEE80211_ADDR_LEN);
	qdf_mem_copy(info->frame, frame, frame_len);
	info->frameSize = frame_len;
  ald_notify_user(vdev, IEEE80211_ALD_ANQP_FRAME_RECEIVED, sizeof(struct ald_anqp_frame_info),(void*)info);
        qdf_mem_free(info);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to send MSCS frame to userspace */
int son_send_mscs_event(struct wlan_objmgr_vdev *vdev,
		struct ald_mscs_event_info *mscsInfo)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

  ald_notify_user(vdev, IEEE80211_ALD_MSCS, sizeof(struct ald_mscs_event_info),(void*)mscsInfo);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to send SCS frame to userspace */
int son_send_scs_event(struct wlan_objmgr_vdev *vdev,
		struct ald_scs_event_info *scsInfo)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

  ald_notify_user(vdev, IEEE80211_ALD_SCS, sizeof(struct ald_scs_event_info),(void*)scsInfo);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to send DSCP query frame to userspace */
int son_send_dscp_query_event(struct wlan_objmgr_vdev *vdev,
		struct ald_dscp_policy_query_info *dscpQuery)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}

  ald_notify_user(vdev, IEEE80211_ALD_DSCP_QUERY, sizeof(struct ald_dscp_policy_query_info),(void*)dscpQuery);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

/* function to send DSCP resp frame to userspace */
int son_send_dscp_resp_event(struct wlan_objmgr_vdev *vdev,
		struct ald_dscp_policy_resp_info *dscpResp)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SON_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return -1;
	}
  ald_notify_user(vdev, IEEE80211_ALD_DSCP_RESP, sizeof(struct ald_dscp_policy_resp_info),(void*)dscpResp);
	wlan_objmgr_vdev_release_ref(vdev, WLAN_SON_ID);

	return 0;
}

static void son_bufful_iter(struct wlan_objmgr_pdev *pdev,
			    void *obj, void *arg)
{
	struct wlan_objmgr_vdev *vdev = obj;

        son_ald_buffull_notify(vdev);
}

void son_buffull_handler(struct wlan_objmgr_pdev *pdev)
{
	if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_SON_ID) !=
	    QDF_STATUS_SUCCESS) {
		SON_LOGE("%s: Unable to get reference", __func__);
		return;
	}

	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
					  son_bufful_iter,
					  NULL, 0, WLAN_SON_ID);

	wlan_objmgr_pdev_release_ref(pdev, WLAN_SON_ID);
}

#endif /* QCA_SUPPORT_SON */
