/*
 * Copyright (c) 2013,2017,2020,2023 Qualcomm Innovation Center, Inc..
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 * 2013 Qualcomm Atheros, Inc.
 */

#if UNIFIED_SMARTANTENNA
#include "sa_api_defs.h"
#include "wlan_sa_api_tgt_api.h"
#include <wlan_mlme_dispatcher.h>
#include <wlan_utility.h>

extern struct smartantenna_ops *g_sa_ops;

enum sa_band_id reg_wifi_band_to_sa_band_id(enum reg_wifi_band reg_wifi_band);

void sa_api_peer_connect(struct wlan_objmgr_pdev *pdev,
			 struct wlan_objmgr_peer *peer, struct sa_rate_cap *rate_cap);

void sa_api_peer_disconnect(struct wlan_objmgr_peer *peer);

int sa_api_init(struct wlan_objmgr_pdev *pdev,
		struct wlan_objmgr_vdev *vdev, struct pdev_sa_api *pa, int new_init);

int sa_api_deinit(struct wlan_objmgr_pdev *pdev,
		  struct wlan_objmgr_vdev *vdev, struct pdev_sa_api *pa, int notify);

int sa_api_update_tx_feedback(struct wlan_objmgr_pdev *pdev,
			      struct wlan_objmgr_peer *peer, void *tx_feedback);

int sa_api_update_rx_feedback(struct wlan_objmgr_pdev *pdev,
			      struct wlan_objmgr_peer *peer, void *rx_feedback);

int sa_api_set_param(struct wlan_objmgr_pdev *pdev, char *params);

int sa_api_get_param(struct wlan_objmgr_pdev *pdev, char *params);

uint32_t sa_api_convert_rate_5g(uint32_t rate_code);

uint32_t sa_api_convert_rate_2g(uint32_t rate_code);

int sa_api_get_bcn_txantenna(struct wlan_objmgr_pdev *pdev, uint32_t *ant);

int sa_api_channel_change(struct wlan_objmgr_pdev *pdev);

int sa_api_cwm_action(struct wlan_objmgr_pdev *pdev);

static inline
int sa_api_get_rxantenna(struct pdev_sa_api *pa, uint32_t *rx_ant)
{
	if (g_sa_ops->sa_get_rxantenna) {
		return g_sa_ops->sa_get_rxantenna(pa->interface_id, rx_ant);
	} else {
		ASSERT(FALSE); /* This condition should not happen */
	}
	return SMART_ANT_STATUS_FAILURE;
}

static inline
int sa_api_get_default_txantenna(struct pdev_sa_api *pa, uint32_t *default_txant)
{
	if (g_sa_ops->sa_get_txdefaultantenna) {
		return g_sa_ops->sa_get_txdefaultantenna(pa->interface_id, default_txant);
	} else {
		QDF_ASSERT(FALSE); /* This condition should not happen */
	}
	return SMART_ANT_STATUS_FAILURE;
}

static inline
int sa_api_get_txantenna(struct peer_sa_api *pe, uint32_t *tx_ant_array)
{
	if (g_sa_ops->sa_get_txantenna) {
		return g_sa_ops->sa_get_txantenna(pe->ccp, tx_ant_array);
	} else {
		ASSERT(FALSE); /* This condition should not happen */
	}
	return SMART_ANT_STATUS_FAILURE;
}

static inline
int sa_api_get_node_config(struct peer_sa_api *pe, uint32_t id, uint32_t *config)
{
	if (g_sa_ops->sa_get_node_config) {
		return g_sa_ops->sa_get_node_config(pe->ccp, id, config);
	} else {
		ASSERT(FALSE); /* This condition should not happen */
	}
	return SMART_ANT_STATUS_FAILURE;
}

static inline
int sa_api_get_traininfo(struct peer_sa_api *peer, uint32_t *train_rate_array, uint32_t *train_ant_array, uint32_t *num_train_pkts)
{
	if (g_sa_ops->sa_get_traininginfo) {
		return g_sa_ops->sa_get_traininginfo(peer->ccp, train_rate_array, train_ant_array, num_train_pkts);
	} else {
		ASSERT(FALSE); /* This condition should not happen */
	}
	return SMART_ANT_STATUS_FAILURE;
}
#endif
