/*
 * Copyright (c) 2013,2017,2023 Qualcomm Innovation Center, Inc..
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 * 2013 Qualcomm Atheros, Inc.
 */

#include "sa_api_defs.h"
#include "sa_api_cmn_api.h"
#include <qdf_mem.h>
#include <qdf_types.h>
#include <osif_private.h>
#include <wlan_mlme_dispatcher.h>
#include "sa_api_ol.h"
#include "cfg_sa.h"
#include "cfg_ucfg_api.h"

struct smartantenna_ops *g_sa_ops = NULL;
qdf_atomic_t g_sa_init;

uint32_t rate_table_24[MAX_OFDM_CCK_RATES] = {0x1b,0x1a,0x19,0x18,0x0b,0x0f,0x0a,0x0e,0x09,0x0d, 0x08,0x0c};
uint32_t rate_table_5[MAX_OFDM_CCK_RATES] = {0x43,0x42,0x41,0x40,0x03,0x07,0x02,0x06,0x01,0x05,0x00,0x04};

enum sa_band_id reg_wifi_band_to_sa_band_id(enum reg_wifi_band reg_wifi_band)
{
	enum sa_band_id band;
	static uint32_t reg_wifi_band_to_sa_band_id_map[] =
	{
		[REG_BAND_2G] = SA_BAND_2GHZ,
		[REG_BAND_5G] = SA_BAND_5GHZ,
		[REG_BAND_6G] = SA_BAND_6GHZ,
		[REG_BAND_UNKNOWN] = SA_BAND_MAX,
	};

	if ((reg_wifi_band < REG_BAND_2G) ||
	    (reg_wifi_band >= REG_BAND_UNKNOWN)) {
		qdf_warn("Invalid reg_wifi_band %d", reg_wifi_band);
		return SA_BAND_MAX;
	}

	band = reg_wifi_band_to_sa_band_id_map[reg_wifi_band];
	if (band == SA_BAND_MAX) {
		qdf_warn("Invalid sa_band_id %d, reg_wifi_band: %d",
			 band, reg_wifi_band);
		return SA_BAND_MAX;
	}

	return band;
}

uint32_t sa_api_convert_rate_2g(uint32_t rate_code)
{
	uint32_t rate_code_converted = 0;
	uint8_t preamble = (rate_code & VHT_PREAMBLE_MASK) >> VHT_MCS_SHIFT;
	uint8_t nss = 0, mcs = 0;
	int i = 0;

	if (preamble == AR600P_PREAMBLE_HT) {
		nss = (rate_code & VHT_NSS_MASK) >> VHT_NSS_SHIFT;
		mcs = (rate_code & VHT_MCS_MASK);

		rate_code_converted =  (HT_RATE_FLAG | nss << HT_NSS_SHIFT | mcs);
	} else {
		for (i = 0; i < MAX_OFDM_CCK_RATES; i++) {
			if (rate_table_5[i] == rate_code)
				break;
		}
		if (i < MAX_OFDM_CCK_RATES)
			rate_code_converted = rate_table_24[i];
	}
	return rate_code_converted;
}

uint32_t sa_api_convert_rate_5g(uint32_t rate_code)
{
	uint32_t rate_code_converted = 0;
	uint8_t mcs = 0, nss = 0, i = 0;

	if (IS_HT_RATE(rate_code)) {
		rate_code &= HT_RATE_MASK;
		nss = (rate_code & HT_NSS_MASK) >> HT_NSS_SHIFT;
		mcs = (rate_code & HT_MCS_MASK);
		rate_code_converted = AR600P_ASSEMBLE_HW_RATECODE(mcs, nss, AR600P_PREAMBLE_HT);
	} else {
		for (i = 0; i < MAX_OFDM_CCK_RATES; i++) {
			if (rate_table_24[i] == rate_code) {
				break;
			}
		}
		if (i < MAX_OFDM_CCK_RATES) {
			rate_code_converted = rate_table_5[i];
		}
	}
	return rate_code_converted;
}

/**
 * convert_chan_width_to_sa_chan_width() - Function to convert Host supported
 * Channel BW to Smart Antenna supported channel BW.
 * @ch_width: Host supported BW.
 *
 * Return: Smart Antenna Supported BW
 */
static uint8_t convert_chan_width_to_sa_chan_width(uint8_t ch_width)
{
	switch (ch_width) {
	case IEEE80211_CWM_WIDTH20:
		return SA_CH_WIDTH20;
	case IEEE80211_CWM_WIDTH40:
		return SA_CH_WIDTH40;
	case IEEE80211_CWM_WIDTH80:
		return SA_CH_WIDTH80;
	case IEEE80211_CWM_WIDTH160:
	case IEEE80211_CWM_WIDTH80_80:
		return SA_CH_WIDTH160;
#ifdef WLAN_FEATURE_11BE
	case IEEE80211_CWM_WIDTH320:
		return SA_CH_WIDTH320;
#endif /* WLAN_FEATURE_11BE */
	};

	return IEEE80211_CWM_WIDTHINVALID;
}

int sa_api_update_tx_feedback(struct wlan_objmgr_pdev *pdev,
			      struct wlan_objmgr_peer *peer, void *tx_feedback)
{
	uint32_t rx_antenna, tx_antenna_array[SMART_ANT_MAX_SA_CHAINS];
	uint8_t status = 0;
	uint32_t train_rate_array[SMART_ANTENNA_MAX_RATE_SERIES];
	uint32_t train_antenna_array[SMART_ANT_MAX_SA_CHAINS], num_train_pkts;
	uint32_t tx_feedback_config = 0;
	uint16_t tx_minpkt = SA_INVALD_PARAM_VAL;
	uint16_t tx_per_th = SA_INVALD_PARAM_VAL;
	struct wlan_objmgr_psoc *psoc;
	struct pdev_sa_api *pa;
	struct peer_sa_api *pe;

	/* intentionally avoiding locks in data path code */
	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		sa_api_err("psoc null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	/* intentionally avoiding locks in data path code */
	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!pa) {
		sa_api_err("pa null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	/* intentionally avoiding locks in data path code */
	pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
	if (!pe) {
		sa_api_err("pe null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	if (pe->ccp == NULL)
		return SMART_ANT_STATUS_FAILURE;

	if (!SMART_ANTENNA_TX_FEEDBACK_ENABLED(pa))
		return SMART_ANT_STATUS_FAILURE;

	if (g_sa_ops->sa_update_txfeedback) {
		if (SMART_ANT_STATUS_SUCCESS == g_sa_ops->sa_update_txfeedback(pe->ccp, (struct  sa_tx_feedback *)tx_feedback, &status)) {
			if (status & SMART_ANT_TX_FEEDBACK_CONFIG_REQUIRED) {
				if (SMART_ANT_STATUS_SUCCESS == sa_api_get_node_config(pe, SA_TX_FEEDBACK_CONFIG, &tx_feedback_config))
					/* configure target to send 1 tx feedback to host for N ppdus sent */
					tgt_sa_api_set_node_config_ops(psoc, peer, SA_TX_FEEDBACK_CONFIG, 1, &tx_feedback_config);
			}
			if (status & SMART_ANT_RX_CONFIG_REQUIRED) {
				if (SMART_ANT_STATUS_SUCCESS == sa_api_get_rxantenna(pa, &rx_antenna))
					/* set RX antenna */
					tgt_sa_api_set_rx_antenna(psoc, pdev, rx_antenna);
			}
			if (status & SMART_ANT_TX_CONFIG_REQUIRED) {
				if (SMART_ANT_STATUS_SUCCESS == sa_api_get_txantenna(pe, &tx_antenna_array[0]))
					/* set TX antenna array */
					tgt_sa_api_set_tx_antenna(psoc, peer, &tx_antenna_array[0]);
			}
			if (status & SMART_ANT_TRAINING_REQUIRED) {
				/* Get train parameters and program train info to lower modules */
				if (SMART_ANT_STATUS_SUCCESS == g_sa_ops->sa_get_traininginfo(pe->ccp, &train_rate_array[0], &train_antenna_array[0], &num_train_pkts)) {
					if (g_sa_ops->sa_get_ext_traininginfo)
						g_sa_ops->sa_get_ext_traininginfo(pe->ccp, &tx_minpkt, &tx_per_th);
					tgt_sa_api_set_training_info(psoc, peer, &train_rate_array[0], &train_antenna_array[0], num_train_pkts, tx_minpkt, tx_per_th);
				}
			}

			return SMART_ANT_STATUS_SUCCESS;
		}
	}

	return SMART_ANT_STATUS_FAILURE;
}

int sa_api_update_rx_feedback(struct wlan_objmgr_pdev *pdev,
			      struct wlan_objmgr_peer *peer, void *rx_feedback)
{
	uint32_t rx_antenna;
	uint8_t status = 0;
	uint32_t train_rate_array[SMART_ANTENNA_MAX_RATE_SERIES];
	uint32_t train_antenna_array[SMART_ANT_MAX_SA_CHAINS], num_train_pkts;
	uint16_t tx_minpkt = SA_INVALD_PARAM_VAL;
	uint16_t tx_per_th = SA_INVALD_PARAM_VAL;
	struct wlan_objmgr_psoc *psoc;
	struct pdev_sa_api *pa;
	struct peer_sa_api *pe;

	/* intentionally avoiding locks in data path code */
	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc) {
		sa_api_err("psoc null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	/* intentionally avoiding locks in data path code */
	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!pa) {
		sa_api_err("pa null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	if (!SMART_ANTENNA_RX_FEEDBACK_ENABLED(pa))
		return SMART_ANT_STATUS_FAILURE;

	/* intentionally avoiding locks in data path code */
	pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
	if (!pe) {
		sa_api_err("pe null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	if (!pe->ccp)
		return SMART_ANT_STATUS_FAILURE;

	if (g_sa_ops->sa_update_rxfeedback) {
		if (SMART_ANT_STATUS_SUCCESS == g_sa_ops->sa_update_rxfeedback(pe->ccp, (struct sa_rx_feedback *)rx_feedback, &status)) {
			if (status & SMART_ANT_RX_CONFIG_REQUIRED) {
				if (SMART_ANT_STATUS_SUCCESS == sa_api_get_rxantenna(pa, &rx_antenna)) {
					/* set RX antenna */
					tgt_sa_api_set_rx_antenna(psoc, pdev, rx_antenna);
				}
			}
			if (status & SMART_ANT_TRAINING_REQUIRED) {
				/* Get train parameters and program train info to lower modules */
				if (SMART_ANT_STATUS_SUCCESS == g_sa_ops->sa_get_traininginfo(pe->ccp, &train_rate_array[0], &train_antenna_array[0], &num_train_pkts)) {
					if (g_sa_ops->sa_get_ext_traininginfo)
						g_sa_ops->sa_get_ext_traininginfo(pe->ccp, &tx_minpkt, &tx_per_th);
					tgt_sa_api_set_training_info(psoc, peer, &train_rate_array[0], &train_antenna_array[0], num_train_pkts, tx_minpkt, tx_per_th);
				}
			}

			return SMART_ANT_STATUS_SUCCESS;
		}
	}

	return SMART_ANT_STATUS_FAILURE;
}

void sa_api_peer_connect(struct wlan_objmgr_pdev *pdev,
			 struct wlan_objmgr_peer *peer, struct sa_rate_cap *rate_cap)
{
	struct sa_node_info node_info;
	struct sa_rate_info rate_info;
	uint8_t i = 0;
	uint32_t htindex = 0, legacyindex = 0;
	struct peer_phy_info {
		uint8_t rxstreams;
		uint8_t streams;
		uint8_t cap;
		uint32_t mode;
		uint32_t ext_mode;
	} phy_info;
	struct peer_sa_api *pe;
	struct pdev_sa_api *pa;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_vdev *vdev;

	psoc = wlan_pdev_get_psoc(pdev);
	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!psoc || !pa) {
		sa_api_err("psoc %pK pa %pK !!! \n", psoc, pa);
		return;
	}
	sa_api_debug("+");

	if (!SMART_ANTENNA_ENABLED(pa))
		return;

	pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
	if (!pe) {
		sa_api_err("pe null !!! \n");
		return;
	}

	qdf_mem_zero(&node_info, sizeof(struct sa_node_info));
	qdf_mem_zero(&phy_info, sizeof(struct peer_phy_info));

	wlan_node_get_peer_phy_info(peer, &phy_info.rxstreams, &phy_info.streams, &phy_info.cap, &phy_info.mode, &phy_info.ext_mode);
	pe->ch_width = wlan_node_get_peer_chwidth(peer);

	if (tgt_sa_api_prepare_rateset(psoc, pdev, peer, &rate_info) == QDF_STATUS_SUCCESS) {
		/* form rate set to sa module */
		for (i = 0; i < rate_info.num_of_rates; i++) {
			if (IS_HT_RATE(rate_info.rates[i].ratecode)) { /* ht rate */
				if (pe->ch_width == IEEE80211_CWM_WIDTH20) {
					node_info.rate_cap.ratecode_20[htindex] = sa_api_convert_rate_5g(rate_info.rates[i].ratecode);
					htindex++;
					node_info.rate_cap.ratecount[RATE_INDEX_BW20]++;
				} else {
					node_info.rate_cap.ratecode_40[htindex] = sa_api_convert_rate_5g(rate_info.rates[i].ratecode);
					htindex++;
					node_info.rate_cap.ratecount[RATE_INDEX_BW40]++;
				}
			} else { /* legacy rate */
				node_info.rate_cap.ratecode_legacy[legacyindex] = sa_api_convert_rate_5g(rate_info.rates[i].ratecode);
				legacyindex++;
				node_info.rate_cap.ratecount[RATE_INDEX_CCK_OFDM]++;
			}
		}
	} else {
		/* rates are already set */
		if (rate_cap)
			qdf_mem_copy(&node_info.rate_cap, rate_cap, sizeof(struct sa_rate_cap));
	}

	qdf_mem_copy(node_info.mac_addr, wlan_peer_get_macaddr(peer), 6);
	node_info.radio_id = pa->interface_id;

	node_info.max_ch_bw = convert_chan_width_to_sa_chan_width(pe->ch_width);
	node_info.chainmask = ((phy_info.streams) | (phy_info.rxstreams << 4));

	node_info.ni_cap = phy_info.cap;

	vdev = wlan_peer_get_vdev(peer);
	node_info.channel_num =  wlan_reg_freq_to_chan(pdev, wlan_vdev_get_chan_freq(vdev));
	node_info.band =  reg_wifi_band_to_sa_band_id(wlan_reg_freq_to_band(wlan_vdev_get_chan_freq(vdev)));

	/* Fill the Operating Mode appropriately */
	sa_api_info("Peer %s mode 0x%x  EXT mode 0x%x\n", ether_sprintf(node_info.mac_addr),
		    phy_info.mode, phy_info.ext_mode);
#ifdef WLAN_FEATURE_11BE
	node_info.opmode = !!(phy_info.ext_mode & IEEE80211_NODE_EHT) ? OPMODE_EHT : OPMODE_CCK_OFDM;
#endif /* WLAN_FEATURE_11BE */
	if (!node_info.opmode && (phy_info.ext_mode & IEEE80211_NODE_HE)) {
		node_info.opmode = OPMODE_HE;
	} else if (!node_info.opmode && (phy_info.mode & IEEE80211_NODE_VHT)) {
		node_info.opmode = OPMODE_VHT;
	} else if (!node_info.opmode && (phy_info.mode & IEEE80211_NODE_HT)) {
		node_info.opmode = OPMODE_HT;
	}

	if ((node_info.band == SA_BAND_6GHZ) &&
	    !((node_info.opmode == OPMODE_HE) || (node_info.opmode == OPMODE_EHT))) {
		sa_api_err("Invalid opmode %d for 6GHz peer!\n", node_info.opmode);
		return;
	}

	if (g_sa_ops->sa_peer_connect)
		g_sa_ops->sa_peer_connect(&pe->ccp, &node_info);
}

void sa_api_peer_disconnect(struct wlan_objmgr_peer *peer)
{
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_pdev *pdev;
	struct peer_sa_api *pe;
	struct pdev_sa_api *pa;
	enum QDF_OPMODE opmode;

	if (!peer) {
		sa_api_err("peer is null\n");
		return;
	}

	vdev = wlan_peer_get_vdev(peer);
	pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
	if (!vdev || !pe) {
		sa_api_err("vdev %pK pe %pK\n", vdev, pe);
		return;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	opmode = wlan_vdev_mlme_get_opmode(vdev);
	if (!pdev) {
		sa_api_err("pdev is NULL\n");
		return;
	}

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!pa) {
		sa_api_err("pa null !!! \n");
		return;
	}

	if (SMART_ANTENNA_ENABLED(pa)) {
		/*  Do we need to check removing own vap/node ??? */
		if ((opmode == QDF_SAP_MODE) && (peer != wlan_vdev_get_bsspeer(vdev))) {
			if (g_sa_ops->sa_peer_disconnect) {
				if (pe->ccp) {
					g_sa_ops->sa_peer_disconnect(pe->ccp);
					pe->ccp = NULL;
				}
			}
		} else if ((opmode == QDF_STA_MODE) &&
				(peer != wlan_vdev_get_bsspeer(vdev))) {
			if (g_sa_ops->sa_peer_disconnect) {
				if (pe->ccp) {
					g_sa_ops->sa_peer_disconnect(pe->ccp);
					pe->ccp = NULL;
				}
			}
		}
	}
}

static inline
void sa_api_peer_connect_iterate(struct wlan_objmgr_pdev *pdev,
		void *object, void *arg)
{
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)arg;
	struct wlan_objmgr_peer *bsspeer;
	uint8_t authorized = 0;

	bsspeer = wlan_vdev_get_bsspeer(vdev);

	authorized = wlan_peer_mlme_flag_get(peer, WLAN_PEER_F_AUTH);

	if ((peer != bsspeer) && authorized)
		sa_api_peer_connect(pdev, peer, NULL);
}

static inline
void sa_api_cwm_iterate(struct wlan_objmgr_pdev *pdev,
		void *object, void *arg)
{
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)arg;
	struct wlan_objmgr_peer *bsspeer;
	struct peer_sa_api *pe;
	uint8_t ch_width = 0; /* wlan_vdev_get_ch_width(vdev); */

	bsspeer = wlan_vdev_get_bsspeer(vdev);

	pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);

	if ((peer != bsspeer) && pe->ch_width != ch_width) {
		pe->ch_width = ch_width;
		sa_api_peer_connect(pdev, peer, NULL);
	}
}

int sa_api_channel_change(struct wlan_objmgr_pdev *pdev)
{
	struct pdev_sa_api *pa;
	struct wlan_objmgr_vdev *vdev;

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!pa) {
		sa_api_err("pa null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}

	if (!SMART_ANTENNA_ENABLED(pa))
		return SMART_ANT_STATUS_FAILURE;

	if (wlan_pdev_scan_in_progress(pdev))
		return 0;

	if ((pa->state != SMART_ANT_STATE_INIT_DONE))
		return SMART_ANT_STATUS_FAILURE;

	vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, 0, WLAN_SA_API_ID);
	if (NULL == vdev) {
		sa_api_err("vdev null !!! \n");
		return SMART_ANT_STATUS_FAILURE;
	}

	/* call smart antenna deint and init with re configuration */
	sa_api_deinit(pdev, vdev, pa, SMART_ANT_RECONFIGURE);
	sa_api_init(pdev, vdev, pa, SMART_ANT_RECONFIGURE);

	/* call peer_connect to each node in the list */
	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_PEER_OP,
			sa_api_peer_connect_iterate,
			vdev, 0, WLAN_SA_API_ID);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SA_API_ID);

	return SMART_ANT_STATUS_SUCCESS;
}

int sa_api_cwm_action(struct wlan_objmgr_pdev *pdev)
{
	struct pdev_sa_api *pa;
	struct wlan_objmgr_vdev *vdev;

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!pa) {
		sa_api_err("pa null !!! \n");
		return QDF_STATUS_E_FAILURE;
	}
	/* get current cwm mode */
	if (!SMART_ANTENNA_ENABLED(pa))
		return SMART_ANT_STATUS_FAILURE;

	vdev = wlan_objmgr_get_vdev_by_id_from_pdev(pdev, 0, WLAN_SA_API_ID);
	if (NULL == vdev) {
		sa_api_err("vdev null !!! \n");
		return SMART_ANT_STATUS_FAILURE;
	}

	/* call peer_connect to each node in the list */
	wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_PEER_OP,
			sa_api_cwm_iterate, vdev, 0, WLAN_SA_API_ID);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_SA_API_ID);

	return SMART_ANT_STATUS_SUCCESS;
}

int sa_api_get_bcn_txantenna(struct wlan_objmgr_pdev *pdev, uint32_t *ant)
{
	struct pdev_sa_api *pa;

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (!pa) {
		sa_api_err("smart antenna private object null\n");
		return QDF_STATUS_E_FAILURE;
	}

	if (!SMART_ANTENNA_ENABLED(pa))
		return SMART_ANT_STATUS_FAILURE;

	if (g_sa_ops->sa_get_bcn_txantenna) {
		return g_sa_ops->sa_get_bcn_txantenna(pa->interface_id, ant);
	} else {
		QDF_ASSERT(FALSE); /* This condition should not happen */
	}

	return SMART_ANT_STATUS_FAILURE;
}

int sa_api_set_param(struct wlan_objmgr_pdev *pdev, char *params)
{
	struct sa_config_params sa_params;
	uint32_t tx_antenna_array[SMART_ANT_MAX_SA_CHAINS];
	uint8_t  mac_addr[6] = {'\0'};
	uint32_t *config = (uint32_t *) params;
	uint32_t radio_mac1 = config[0];
	uint32_t mac2 = config[1];
	struct wlan_objmgr_peer *peer = NULL;
	struct wlan_objmgr_psoc *psoc;
	int retval = 0;
	int config_required = 0;
	uint32_t rx_antenna = 0;
	uint32_t default_tx_antenna = 0;
	uint32_t tx_feedback_config = 0;
	uint32_t train_rate_array[SMART_ANTENNA_MAX_RATE_SERIES];
	uint32_t train_antenna_array[SMART_ANT_MAX_SA_CHAINS], num_train_pkts;
	uint16_t tx_minpkt = SA_INVALD_PARAM_VAL;
	uint16_t tx_per_th = SA_INVALD_PARAM_VAL;
	struct pdev_sa_api *pa;
	struct peer_sa_api *pe;

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	psoc = wlan_pdev_get_psoc(pdev);
	if (!pa || !psoc) {
		sa_api_err("null pa %pK, psoc %pK !!! \n", pa, psoc);
		return QDF_STATUS_E_FAILURE;
	}

	sa_params.radio_id = pa->interface_id;
	sa_params.param_type = (config[0] & SA_PARM_TYPE_MASK) >> SA_PARM_TYPE_SHIFT;
	sa_params.param_id  = config[2];
	sa_params.param_value  = config[3];
	sa_params.ccp = NULL;

	if (sa_params.param_type) {
		/* look for node */
		mac_addr[0] = (uint8_t)((radio_mac1 & 0x0000ff00) >> 8);
		mac_addr[1] = (uint8_t)((radio_mac1 & 0x000000ff) >> 0);
		mac_addr[2] = (uint8_t)((mac2 & 0xff000000) >> 24);
		mac_addr[3] = (uint8_t)((mac2 & 0x00ff0000) >> 16);
		mac_addr[4] = (uint8_t)((mac2 & 0x0000ff00) >> 8);
		mac_addr[5] = (uint8_t)((mac2 & 0x000000ff) >> 0);

		peer = wlan_objmgr_get_peer(psoc,
					    wlan_objmgr_pdev_get_pdev_id(pdev),
					    &mac_addr[0], WLAN_SA_API_ID);
		if (!peer) {
			sa_api_err("peer %s null!\n", ether_sprintf(mac_addr));
			return SMART_ANT_STATUS_FAILURE;
		}

		pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
		if (!pe) {
			sa_api_err("pe null !!! \n");
			wlan_objmgr_peer_release_ref(peer, WLAN_SA_API_ID);
			return QDF_STATUS_E_FAILURE;
		}

		if (!pe->ccp) {
			wlan_objmgr_peer_release_ref(peer, WLAN_SA_API_ID);
			return SMART_ANT_STATUS_FAILURE;
		}
		sa_params.ccp  = pe->ccp;
		retval = g_sa_ops->sa_set_param(&sa_params);
	} else {
		retval = g_sa_ops->sa_set_param(&sa_params);
	}

	config_required = (retval == SMART_ANT_STATUS_FAILURE) ? 0 : retval;
	if (config_required) {
		if (config_required & SMART_ANT_RX_CONFIG_REQUIRED) {
			if (SMART_ANT_STATUS_SUCCESS == sa_api_get_rxantenna(pa, &rx_antenna)) {
				/* set RX antenna */
				tgt_sa_api_set_rx_antenna(psoc, pdev, rx_antenna);
			}
		} else if ((config_required & SMART_ANT_TX_FEEDBACK_CONFIG_REQUIRED) && (peer)) {
			if (SMART_ANT_STATUS_SUCCESS == sa_api_get_node_config(pe, SA_TX_FEEDBACK_CONFIG, &tx_feedback_config)) {
				/* configure target to send 1 tx feedback to host for N ppdus sent */
				tgt_sa_api_set_node_config_ops(psoc, peer, SA_TX_FEEDBACK_CONFIG, 1, &tx_feedback_config);
			}
		} else if ((config_required & SMART_ANT_TX_CONFIG_REQUIRED) && (peer)) {
			if (SMART_ANT_STATUS_SUCCESS == sa_api_get_txantenna(pe, &tx_antenna_array[0])) {
				/* set TX antenna array */
				tgt_sa_api_set_tx_antenna(psoc, peer, &tx_antenna_array[0]);
			}
		} else if ((config_required & SMART_ANT_TRAINING_REQUIRED) && (peer)) {
			if (SMART_ANT_STATUS_SUCCESS == g_sa_ops->sa_get_traininginfo(pe->ccp, &train_rate_array[0], &train_antenna_array[0], &num_train_pkts)) {
				if (g_sa_ops->sa_get_ext_traininginfo)
					g_sa_ops->sa_get_ext_traininginfo(pe->ccp, &tx_minpkt, &tx_per_th);
				tgt_sa_api_set_training_info(psoc, peer, &train_rate_array[0], &train_antenna_array[0], num_train_pkts, tx_minpkt, tx_per_th);
			}
		} else if (config_required & SMART_ANT_DEFAULT_TX_CONFIG_REQUIRED) {
			if (SMART_ANT_STATUS_SUCCESS == sa_api_get_default_txantenna(pa, &default_tx_antenna))
				tgt_sa_api_set_default_tx_antenna(psoc, pdev, default_tx_antenna);
		}
	}

	if (peer)
		wlan_objmgr_peer_release_ref(peer, WLAN_SA_API_ID);

	return retval;
}

int sa_api_get_param(struct wlan_objmgr_pdev *pdev, char *params)
{
	struct sa_config_params sa_params;
	uint8_t  mac_addr[6] = {'\0'};
	uint32_t *config = (uint32_t *) params;
	uint32_t radio_mac1 = config[0];
	uint32_t mac2 = config[1];
	int retval = 0;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_peer *peer = NULL;
	struct pdev_sa_api *pa;
	struct peer_sa_api *pe;

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	psoc = wlan_pdev_get_psoc(pdev);
	if (!pa) {
		sa_api_err("null pa %pK, psoc %pK !!! \n", pa, psoc);
		return QDF_STATUS_E_FAILURE;
	}

	if (!SMART_ANTENNA_ENABLED(pa))
		return SMART_ANT_STATUS_FAILURE;

	sa_params.radio_id = pa->interface_id;
	sa_params.param_type = (config[0] & SA_PARM_TYPE_MASK) >> SA_PARM_TYPE_SHIFT;
	sa_params.param_id  = config[2];
	if (sa_params.param_type) {
		/* look for node */
		mac_addr[0] = (uint8_t)((radio_mac1 & 0x0000ff00) >> 8);
		mac_addr[1] = (uint8_t)((radio_mac1 & 0x000000ff) >> 0);
		mac_addr[2] = (uint8_t)((mac2 & 0xff000000) >> 24);
		mac_addr[3] = (uint8_t)((mac2 & 0x00ff0000) >> 16);
		mac_addr[4] = (uint8_t)((mac2 & 0x0000ff00) >> 8);
		mac_addr[5] = (uint8_t)((mac2 & 0x000000ff) >> 0);

		peer = wlan_objmgr_get_peer(psoc,
					    wlan_objmgr_pdev_get_pdev_id(pdev),
					    mac_addr,
					    WLAN_SA_API_ID);
		if (!peer) {
			sa_api_err("Invalid peer %s", ether_sprintf(mac_addr));
			return QDF_STATUS_E_FAILURE;
		}

		pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
		if (!pe) {
			sa_api_err("pe null !!! \n");
			wlan_objmgr_peer_release_ref(peer, WLAN_SA_API_ID);
			return QDF_STATUS_E_FAILURE;
		}

		if (!pe->ccp) {
			wlan_objmgr_peer_release_ref(peer, WLAN_SA_API_ID);
			return SMART_ANT_STATUS_FAILURE;
		}
		sa_params.ccp  = pe->ccp;
		retval = g_sa_ops->sa_get_param(&sa_params);
	} else {
		retval = g_sa_ops->sa_get_param(&sa_params);
	}

	config[0] = sa_params.param_value;
	if (peer)
		wlan_objmgr_peer_release_ref(peer, WLAN_SA_API_ID);

	return retval;
}

QDF_STATUS
wlan_sa_api_psoc_obj_create_handler(struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct sa_api_context *sc = NULL;

	if (NULL == psoc) {
		sa_api_err("PSOC is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	sc = (struct sa_api_context *)qdf_mem_malloc(sizeof(struct sa_api_context));
	if (NULL == sc) {
		sa_api_err("Failed to allocate sa_api_ctx object\n");
		return QDF_STATUS_E_NOMEM;
	}

	sc->psoc_obj = psoc;
	sc->sa_validate_sw = cfg_get(psoc, CFG_SA_VALIDATE_SW);

	sc->sa_suppoted = cfg_get(psoc, CFG_SA_ENABLE);

	if (WLAN_DEV_OL == wlan_objmgr_psoc_get_dev_type(psoc))
		sa_api_ctx_init_ol(sc);

	sa_api_info("soc %pK sa_suppoted %d, sa_validate_sw %d, enable %d",
			psoc, sc->sa_suppoted, sc->sa_validate_sw, sc->enable);
	wlan_objmgr_psoc_component_obj_attach(psoc, WLAN_UMAC_COMP_SA_API,
			(void *)sc, QDF_STATUS_SUCCESS);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_psoc_obj_destroy_handler(struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct sa_api_context *sc = NULL;

	if (NULL == psoc) {
		sa_api_err("PSOC is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	sa_api_debug("#");
	sc = wlan_objmgr_psoc_get_comp_private_obj(psoc, WLAN_UMAC_COMP_SA_API);
	if (NULL != sc) {
		wlan_objmgr_psoc_component_obj_detach(psoc, WLAN_UMAC_COMP_SA_API,
				(void *)sc);
		qdf_mem_free(sc);
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_pdev_obj_create_handler(struct wlan_objmgr_pdev *pdev, void *arg)
{
	struct wlan_objmgr_psoc *psoc;
	struct sa_api_context *sc = NULL;
	struct pdev_sa_api *pa = NULL;

	if (NULL == pdev) {
		sa_api_err("PDEV is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pa = (struct pdev_sa_api *)qdf_mem_malloc(sizeof(struct pdev_sa_api));
	if (NULL == pa) {
		sa_api_err("Failed to allocate pdev_sa_api object\n");
		return QDF_STATUS_E_NOMEM;
	}

	qdf_mem_zero(pa, sizeof(struct pdev_sa_api));

	psoc = wlan_pdev_get_psoc(pdev);
	if (NULL == psoc) {
		sa_api_err("PSOC is NULL\n");
		qdf_mem_free(pa);
		return QDF_STATUS_E_FAILURE;
	}

	sc = wlan_objmgr_psoc_get_comp_private_obj(psoc, WLAN_UMAC_COMP_SA_API);
	if (NULL == sc) {
		sa_api_err("SA API context is NULL\n");
		qdf_mem_free(pa);
		return QDF_STATUS_E_FAILURE;
	}

	if (sc->sa_api_pdev_init)
		sc->sa_api_pdev_init(psoc, pdev, sc, pa);

	sa_api_info("pdev %pK state %d, mode %d, max_fallback_rates %d, "
			"radio_id %d, interface_id %d, enable %d", pdev,
			pa->state, pa->mode, pa->max_fallback_rates,
			pa->radio_id, pa->interface_id, pa->enable);
	wlan_objmgr_pdev_component_obj_attach(pdev, WLAN_UMAC_COMP_SA_API,
			(void *)pa, QDF_STATUS_SUCCESS);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_pdev_obj_destroy_handler(struct wlan_objmgr_pdev *pdev, void *arg)
{
	struct pdev_sa_api *pa = NULL;

	if (NULL == pdev) {
		sa_api_err("PDEV is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (NULL != pa) {
		wlan_objmgr_pdev_component_obj_detach(pdev, WLAN_UMAC_COMP_SA_API,
				(void *)pa);
		qdf_mem_free(pa);
	}

	sa_api_debug("#");
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_vdev_obj_create_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
	struct wlan_objmgr_pdev *pdev;
	struct pdev_sa_api *pa = NULL;

	if (NULL == vdev) {
		sa_api_err("VDEV is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);

	if (NULL == pdev) {
		sa_api_err("PDEV is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (NULL == pa) {
		sa_api_err("aP is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_vdev_obj_destroy_handler(struct wlan_objmgr_vdev *vdev, void *arg)
{
	struct wlan_objmgr_pdev *pdev;
	struct pdev_sa_api *pa = NULL;

	if (NULL == vdev) {
		sa_api_err("VDEV is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pdev = wlan_vdev_get_pdev(vdev);
	if (NULL == pdev) {
		sa_api_err("PDEV is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pa = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_SA_API);
	if (NULL == pa) {
		sa_api_err("aP is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	sa_api_deinit(pdev, vdev, pa, SMART_ANT_NEW_CONFIGURATION);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_peer_obj_create_handler(struct wlan_objmgr_peer *peer, void *arg)
{
	struct peer_sa_api *pe = NULL;

	if (NULL == peer) {
		sa_api_err("PEER is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	pe = (struct peer_sa_api *)qdf_mem_malloc(sizeof(struct peer_sa_api));
	if (NULL == pe) {
		sa_api_err("Failed to allocate peer_sa_api object\n");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_mem_zero(pe, sizeof(struct peer_sa_api));
	pe->peer_obj = peer;
	pe->ch_width = wlan_node_get_peer_chwidth(peer);

	wlan_objmgr_peer_component_obj_attach(peer, WLAN_UMAC_COMP_SA_API,
			(void *)pe, QDF_STATUS_SUCCESS);

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_sa_api_peer_obj_destroy_handler(struct wlan_objmgr_peer *peer, void *arg)
{
	struct peer_sa_api *pe = NULL;

	if (NULL == peer) {
		sa_api_err("PEER is NULL\n");
		return QDF_STATUS_E_FAILURE;
	}

	sa_api_peer_disconnect(peer);

	pe = wlan_objmgr_peer_get_comp_private_obj(peer, WLAN_UMAC_COMP_SA_API);
	if (pe != NULL) {
		wlan_objmgr_peer_component_obj_detach(peer, WLAN_UMAC_COMP_SA_API,
				(void *)pe);
		qdf_mem_free(pe);
	}

	return QDF_STATUS_SUCCESS;
}

int register_smart_ant_ops(struct smartantenna_ops *sa_ops)
{
	g_sa_ops = sa_ops;
	qdf_atomic_init(&g_sa_init);

	return SMART_ANT_STATUS_SUCCESS;
}

int deregister_smart_ant_ops(char *dev_name)
{
	if (qdf_atomic_read(&g_sa_init) == 0)
		g_sa_ops = NULL;

	return SMART_ANT_STATUS_SUCCESS;
}
