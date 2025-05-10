/*
 * Copyright (c) 2017-2020,2023-2024 Qualcomm Innovation Center, Inc.
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

#include <ol_if_pdev.h>
#include <ol_regdomain.h>
#ifdef DIRECT_BUF_RX_ENABLE
#include <target_if_direct_buf_rx_api.h>
#endif

#if QCA_STANDALONE_SOUNDING_TRIGGER
#define DBR_EVENT_TIMEOUT_IN_MS_TXBF_CBF 1
#define DBR_NUM_RESP_PER_EVENT_TXBF_CBF  6
#endif

int ol_ath_pdev_set_param(struct wlan_objmgr_pdev *pdev,
			  wmi_conv_pdev_params_id param_id,
                          uint32_t param_value)
{
	struct pdev_params pparam;
	int32_t pdev_idx;
	struct wmi_unified *pdev_wmi_handle;
	QDF_STATUS status;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle)
		return -1;

	pdev_idx = lmac_get_pdev_idx(pdev);
	if (pdev_idx < 0)
		return -1;

	qdf_mem_set(&pparam, sizeof(pparam), 0);
	pparam.param_id = param_id;
	pparam.param_value = param_value;

	status = wmi_unified_pdev_param_send(pdev_wmi_handle,
					     &pparam, pdev_idx);

	return qdf_status_to_os_return(status);
}

void ol_ath_pdev_dfs_phyerr_offload_en(struct wlan_objmgr_pdev *pdev)
{
	struct wmi_unified *wmi_handle;
	struct wmi_unified *pdev_wmi_handle;
	struct wlan_objmgr_psoc *psoc;

	psoc = wlan_pdev_get_psoc(pdev);
	wmi_handle = lmac_get_wmi_hdl(psoc);
	if (!wmi_handle) {
		qdf_err("wmi_handle is null");
		return;
	}

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev wmi handle is null");
		return;
	}

	if (wmi_service_enabled(wmi_handle, wmi_service_dfs_phyerr_offload))
		wmi_unified_dfs_phyerr_offload_en_cmd(pdev_wmi_handle,
						      WMI_HOST_PDEV_ID_SOC);
}

int ol_ath_pdev_fips(struct wlan_objmgr_pdev *pdev, uint8_t *key,
		     uint32_t key_len, uint8_t *data, uint32_t data_len,
		     uint32_t mode, uint32_t op, uint32_t pdev_id)
{
	struct fips_params param;
	struct wmi_unified *pdev_wmi_handle;
	QDF_STATUS status;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev wmi handle is null");
		return -EINVAL;
	}

	qdf_mem_set(&param, sizeof(param), 0);
	param.key_len = key_len;
	param.data_len = data_len;
	param.op = op;
	param.key = key;
	param.data = data;
	param.mode = mode;
	param.pdev_id = pdev_id;

	status = wmi_unified_pdev_fips_cmd_send(pdev_wmi_handle, &param);
	return qdf_status_to_os_return(status);
}

#ifdef WLAN_FEATURE_FIPS_BER_CCMGCM
int ol_ath_pdev_fips_extend(struct wlan_objmgr_pdev *pdev,
			    struct ath_fips_extend_cmd *fips_buf,
			    uint32_t pdev_id)
{
	struct fips_extend_params param;
	struct wmi_unified *pdev_wmi_handle;
	QDF_STATUS status;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev wmi handle is null");
		return -EINVAL;
	}

	qdf_mem_set(&param, sizeof(param), 0);
	param.pdev_id = pdev_id;

	param.cookie = fips_buf->cookie;
	param.frag_idx = fips_buf->frag_idx;
	param.more_bit = fips_buf->more_bit;

	param.cmd_params.fips_cmd = fips_buf->cmd_params.fips_cmd;
        param.cmd_params.key_cipher = fips_buf->cmd_params.key_cipher;
	param.cmd_params.key_len = fips_buf->cmd_params.key_len;
	memcpy(param.cmd_params.key, fips_buf->cmd_params.key, param.cmd_params.key_len);

	param.cmd_params.nonce_iv_len = fips_buf->cmd_params.nonce_iv_len;
	memcpy(param.cmd_params.nonce_iv, fips_buf->cmd_params.nonce_iv, param.cmd_params.nonce_iv_len);

	param.cmd_params.tag_len = fips_buf->cmd_params.tag_len;

	param.cmd_params.aad_len = fips_buf->cmd_params.aad_len;
	param.cmd_params.payload_len = fips_buf->cmd_params.payload_len;
	param.data_len = fips_buf->data_len;
	param.data = fips_buf->data;

	status = wmi_unified_pdev_fips_extend_cmd_send(pdev_wmi_handle, &param);
	return qdf_status_to_os_return(status);
}

int ol_ath_pdev_fips_mode_set(struct wlan_objmgr_pdev *pdev,
			      uint32_t pdev_id, uint32_t mode)
{
	struct fips_mode_set_params param;
	struct wmi_unified *pdev_wmi_handle;
	QDF_STATUS status;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev wmi handle is null");
		return -EINVAL;
	}

	qdf_mem_set(&param, sizeof(param), 0);
	param.pdev_id = pdev_id;
	param.mode = mode;

	status = wmi_unified_pdev_fips_mode_set_cmd(pdev_wmi_handle, &param);
	return qdf_status_to_os_return(status);
}
#endif

/**
 * ol_ath_pdev_set_ht_ie() - Set ht ie data
 * @pdev: pdev object
 * @ie_len: ie length
 * @ie_data: ie data
 *
 * Return: 0 if success, other value if failure
 */
static int ol_ath_pdev_set_ht_ie(struct wlan_objmgr_pdev *pdev,
				 uint32_t ie_len, uint8_t *ie_data)
{
	struct ht_ie_params param;
	struct wmi_unified *pdev_wmi_handle;
	QDF_STATUS status;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev wmi handle is null");
		return -EINVAL;
	}

	qdf_mem_set(&param, sizeof(param), 0);
	param.ie_len = ie_len;
	param.ie_data = ie_data;

	status = wmi_unified_set_ht_ie_cmd_send(pdev_wmi_handle, &param);
	return qdf_status_to_os_return(status);
}

/**
 * ol_ath_pdev_set_vht_ie() - Set vht ie data
 * @pdev:  pdev object
 * @ie_len: ie length
 * @ie_data: ie data
 *
 * Return: 0 if success, other value if failure
 */
static int ol_ath_pdev_set_vht_ie(struct wlan_objmgr_pdev *pdev,
				  uint32_t ie_len, uint8_t *ie_data)
{
	struct vht_ie_params param;
	struct wmi_unified *pdev_wmi_handle;
	QDF_STATUS status;

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
	if (!pdev_wmi_handle) {
		qdf_err("pdev wmi handle is null");
		return -EINVAL;
	}

	qdf_mem_set(&param, sizeof(param), 0);
	param.ie_len = ie_len;
	param.ie_data = ie_data;

	status = wmi_unified_set_vht_ie_cmd_send(pdev_wmi_handle, &param);
	return qdf_status_to_os_return(status);
}

void ol_ath_set_ht_vht_ies(struct ieee80211_node *ni)
{
	struct ieee80211com *ic = NULL;
	struct ieee80211vap *vap = NULL;
	struct ol_ath_softc_net80211 *scn = NULL;
	uint8_t *buf = NULL;
	uint8_t *buf_end = NULL;

	if (!ni) {
		QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
			       QDF_TRACE_LEVEL_ERROR,
			       "%s: Node is NULL\n", __func__);
		return;
	}

	ic = ni->ni_ic;
	vap = ni->ni_vap;
	if (!ic || !vap) {
		QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
			       QDF_TRACE_LEVEL_ERROR,
			       "%s: IC or VAP is NULL\n", __func__);
		return;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		QDF_PRINT_INFO(QDF_PRINT_IDX_SHARED, QDF_MODULE_ID_ANY,
			       QDF_TRACE_LEVEL_ERROR,
			       "%s: SCN is NULL\n", __func__);
		return;
	}

	if (scn->set_ht_vht_ies)
		return;

	buf = qdf_mem_malloc(MAX_IE_SIZE);

	if (buf) {
		buf_end = ieee80211_add_htcap(buf, vap->iv_bss,
					      IEEE80211_FC0_SUBTYPE_PROBE_REQ);
		if ((buf_end - buf) <= MAX_HT_IE_LEN)
			ol_ath_pdev_set_ht_ie(scn->sc_pdev, buf_end - buf, buf);
		else
			qdf_err("HT IE len %d is more than expected",
				(int)(buf_end - buf));

		buf_end = ieee80211_add_vhtcap(buf, vap->iv_bss, ic,
					       IEEE80211_FC0_SUBTYPE_PROBE_REQ,
					       NULL, NULL);

		if ((buf_end - buf) <= MAX_VHT_IE_LEN)
			ol_ath_pdev_set_vht_ie(scn->sc_pdev, buf_end-buf, buf);
		else
			qdf_err("VHT IE len %d is more than expected",
				(int)(buf_end - buf));

		scn->set_ht_vht_ies = 1;
		qdf_mem_free(buf);
	}
}

void ol_ath_set_rxfilter(struct wlan_objmgr_pdev *pdev, uint32_t filter)
{
	if (ol_ath_pdev_set_param(pdev, wmi_pdev_param_rx_filter, filter))
		qdf_err("Error setting rxfilter 0x%08x", filter);
}

void ol_ath_setTxPowerLimit(struct wlan_objmgr_pdev *pdev, uint32_t limit,
			    uint16_t tpcInDb, uint32_t is2GHz)
{
	int ret = 0;
	uint16_t cur_tx_power;
	struct ol_ath_softc_net80211 *scn;
	struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);

	if (!ic) {
		qdf_err("ic is NULL!");
		return;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("scn is NULL!");
		return;
	}

	cur_tx_power = ieee80211com_get_txpowerlimit(ic);

	if (cur_tx_power == limit)
		return;

	/* Update max tx power only if the curr max tx power is diff */
	if (limit > scn->max_tx_power) {
		qdf_info("Tx power value is greater than supported ");
		qdf_info("max tx power %d, Limiting to default Max",
			  scn->max_tx_power);
		limit = scn->max_tx_power;
	}

	if (is2GHz)
		ret = ol_ath_pdev_set_param(pdev,
					    wmi_pdev_param_txpower_limit2g,
					    limit);
	else
		ret = ol_ath_pdev_set_param(pdev,
					    wmi_pdev_param_txpower_limit5g,
					    limit);
	if (ret)
		return;

	/* Update the ic_txpowlimit */
	if (is2GHz)
		scn->txpowlimit2G = limit;
	else
		scn->txpowlimit5G = limit;

	if ((is2GHz && IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan)) ||
	    (!is2GHz && !IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan)))
		ieee80211com_set_txpowerlimit(ic, (uint16_t)limit);
}

void ol_ath_setmfpQos(struct wlan_objmgr_pdev *pdev, uint32_t dot11w)
{
	ol_ath_pdev_set_param(pdev, wmi_pdev_param_pmf_qos, dot11w);
}

#if QCA_MANUAL_TRIGGERED_ULOFDMA
/**
 * ol_manual_trig_ulofdma_is_ini_enabled() - manual ulofdma is ini enabled
 * @pdev: the physical device object.
 *
 * Return : true if enabled, else false.
 */
static bool
ol_manual_trig_ulofdma_is_ini_enabled(struct wlan_objmgr_pdev *pdev)
{
    struct wlan_objmgr_psoc *psoc;
    uint8_t mulofdma_enable_bitmap;

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        qdf_err("psoc is null");
        return false;
    }

    mulofdma_enable_bitmap = cfg_get(psoc, CFG_OL_MANUAL_ULOFDMA);

    if (mulofdma_enable_bitmap &
        (1 << wlan_objmgr_pdev_get_pdev_id(pdev))) {
        qdf_info("mulofdma ini is enabled for pdev[%d]",
                 wlan_objmgr_pdev_get_pdev_id(pdev));
        return true;
    }

    return false;
}

/**
 * ol_man_trig_ulofdma_is_service_enabled() - manual ulofdma is service enabled
 * @pdev: the physical device object.
 *
 * Return : true if enabled, else false.
 */
static bool
ol_man_trig_ulofdma_is_service_enabled(struct wlan_objmgr_pdev *pdev)
{
    struct wmi_unified *wmi_handle;
    struct wmi_unified *pdev_wmi_handle;
    struct wlan_objmgr_psoc *psoc;

    psoc = wlan_pdev_get_psoc(pdev);
    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return false;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return false;
    }

    if (wmi_service_enabled(wmi_handle,
                            wmi_service_manual_ulofdma_trigger_support)) {
        qdf_info("mulofdma service is enabled for pdev[%d]",
                 wlan_objmgr_pdev_get_pdev_id(pdev));
        return true;
    }

    return false;
}

int ol_ath_pdev_man_trig_ulofdma_init(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    bool m_ulofdma_ini, m_ulofdma_service;
    uint8_t pdev_id;

    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    m_ulofdma_ini = ol_manual_trig_ulofdma_is_ini_enabled(pdev);
    if (!m_ulofdma_ini) {
        qdf_debug("mulofdma ini bit is disabled for pdev%d",pdev_id);
        return 0;
    }

    m_ulofdma_service = ol_man_trig_ulofdma_is_service_enabled(pdev);
    if (!m_ulofdma_service) {
        qdf_debug("mulofdma service bit is disabled for pdev%d",pdev_id);
        return 0;
    }

    scn->sc_mulofdma_support = m_ulofdma_ini && m_ulofdma_service;
    qdf_info("mulofdma support=%d for pdev=%d",
             scn->sc_mulofdma_support, pdev_id);

    /* init relayfs */
    ol_manual_trig_ulofdma_streamfs_init(scn->sc_pdev);

    return 0;
}

int ol_ath_pdev_man_trig_ulofdma_deinit(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);

    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    /* deinit relayfs */
    ol_manual_trig_ulofdma_streamfs_remove(scn->sc_pdev);

    return 0;
}
#endif /* QCA_MANUAL_TRIGGERED_ULOFDMA */

#ifdef WLAN_SUPPORT_RX_PROTOCOL_TYPE_TAG
int ol_ath_set_protocol_tagging(struct wlan_objmgr_pdev *pdev,
                                RX_PKT_TAG_OPCODE_TYPE opcode,
		                RX_PKT_TAG_RECV_PKT_TYPE pkt_type,
		                uint32_t metadata)
{
	struct ol_ath_softc_net80211 *scn;
	struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);
	struct ieee80211_rx_pkt_protocol_tag tag_info;

	if (!ic) {
		qdf_err("ic is NULL!");
		return -EINVAL;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("scn is NULL!");
		return -EINVAL;
	}

	tag_info.op_code = opcode;
	tag_info.pkt_type = pkt_type;
	tag_info.pkt_type_metadata = metadata;
	ol_ath_ucfg_set_rx_pkt_protocol_tagging(scn, &tag_info);

	return 0;
}
#else
int ol_ath_set_protocol_tagging(struct wlan_objmgr_pdev *pdev,
				RX_PKT_TAG_OPCODE_TYPE opcode,
				RX_PKT_TAG_RECV_PKT_TYPE pkt_type,
				uint32_t metadata)
{
	return 0;
}
#endif

#ifdef WLAN_SUPPORT_TX_PKT_CAP_CUSTOM_CLASSIFY
int ol_ath_set_tx_pkt_cap_custom_classify(struct wlan_objmgr_pdev *pdev,
					  uint32_t pkt_type_bitmap)
{
	struct ol_ath_softc_net80211 *scn;
	struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);
	struct ieee80211_tx_pkt_cap_params param_info;

	if (!ic) {
		qdf_err("ic is NULL!");
		return -EINVAL;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("scn is NULL!");
		return -EINVAL;
	}

	param_info.pkt_type_bitmap = pkt_type_bitmap;
	ol_ath_ucfg_set_tx_pkt_cap_custom_classify(scn, &param_info);
	return 0;
}
#else
int ol_ath_set_tx_pkt_cap_custom_classify(struct wlan_objmgr_pdev *pdev,
					  uint32_t pkt_type_bitmap)
{
	return 0;
}
#endif

#if ATH_OL_FAST_CHANNEL_RESET_WAR
/* WAR for EV#117307, MSDU_DONE is not set for data packet,
 * to fix this issue, fast channel change is disabled for x86 platform
 */
void ol_ath_fast_chan_change(struct wlan_objmgr_pdev *pdev)
{
	qdf_info("Disabling fast channel reset");

	if (ol_ath_pdev_set_param(pdev, wmi_pdev_param_fast_channel_reset,
				  DISABLE_FAST_CHANNEL_RESET))
		qdf_err("Failed to disable fast channel reset");
}
#endif

int ol_ath_set_mgmt_retry_limit(struct wlan_objmgr_pdev *pdev, uint8_t limit)
{
	int ret = 0;
	struct ol_ath_softc_net80211 *scn;
	struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);

	if (!ic) {
		qdf_err("ic is NULL!");
		return -EINVAL;
	}

	scn = OL_ATH_SOFTC_NET80211(ic);
	if (!scn) {
		qdf_err("scn is NULL!");
		return -EINVAL;
	}

	qdf_info("Set mgmt retry limit to %d", limit);

	ret = ol_ath_pdev_set_param(pdev, wmi_pdev_param_mgmt_retry_limit,
				    limit);
	if (ret) {
		qdf_err("Set mgmt retry limit failed!");
		return ret;
	}

	scn->scn_mgmt_retry_limit = limit;
	return 0;
}

int ol_ath_set_default_pcp_tid_map(struct wlan_objmgr_pdev *pdev, uint32_t pcp,
				   uint32_t tid)
{
	ol_txrx_soc_handle soc_txrx_handle;
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;

	psoc = wlan_pdev_get_psoc(pdev);
	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_err("dp handle is null");
		return -EINVAL;
	}

	status = cdp_set_pdev_pcp_tid_map(soc_txrx_handle,
					  wlan_objmgr_pdev_get_pdev_id(pdev),
					  pcp, tid);
	return qdf_status_to_os_return(status);
}

int ol_ath_set_default_tidmap_prty(struct wlan_objmgr_pdev *pdev, uint32_t val)
{
	ol_txrx_soc_handle soc_txrx_handle;
	cdp_config_param_type value = {0};
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;

	psoc = wlan_pdev_get_psoc(pdev);

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	if (!soc_txrx_handle) {
		qdf_err("dp handle is null");
		return -EINVAL;
	}

	value.cdp_pdev_param_tidmap_prty = val;

	status = cdp_txrx_set_pdev_param(soc_txrx_handle,
					 wlan_objmgr_pdev_get_pdev_id(pdev),
					 CDP_TIDMAP_PRTY, value);
	return qdf_status_to_os_return(status);
}

QDF_STATUS ol_scan_set_chan_list(struct wlan_objmgr_pdev *pdev, void *arg)
{
    struct ieee80211com *ic = NULL;
    struct scan_chan_list_params *param = arg;
    struct wmi_unified *pdev_wmi_handle;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev_wmi_handle is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return wmi_unified_scan_chan_list_cmd_send(pdev_wmi_handle, param);
}

#ifdef CONFIG_AFC_SUPPORT
QDF_STATUS ol_ath_trigger_acs_for_afc(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic;
    QDF_STATUS status = QDF_STATUS_E_INVAL;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (ieee80211_is_6g_txable_chan_available(ic, REG_BEST_PWR_MODE))
        status = ieee80211_dcs_afc_trigger(ic);

    if ((status == QDF_STATUS_E_INVAL) || (status == QDF_STATUS_E_NOSUPPORT) ||
            (status == QDF_STATUS_E_FAILURE)) {
        return mlme_check_curchan_and_set_ap_pwr_type(ic, ic->ic_curchan);
    }

    return status;
}

QDF_STATUS ol_ath_trigger_update_channel_list(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return mlme_check_curchan_and_set_ap_pwr_type(ic, ic->ic_curchan);
}

#endif

QDF_STATUS ol_ath_fill_umac_legacy_chanlist(struct wlan_objmgr_pdev *pdev,
                                    struct regulatory_channel *curr_chan_list)
{
    struct ieee80211com *ic;
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_psoc *psoc;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return QDF_STATUS_E_FAILURE;
    }

    if (wmi_service_enabled(wmi_handle, wmi_service_regulatory_db))
        ieee80211_reg_get_current_chan_list(ic, curr_chan_list);

    return QDF_STATUS_SUCCESS;
}

void ol_ath_set_wait_for_init_cc_response_event(struct wlan_objmgr_pdev *pdev,
                                                QDF_STATUS status)
{
    struct ieee80211com *ic;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return;
    }

    ic->ic_set_country_failed = QDF_IS_STATUS_ERROR(status);
    qdf_event_set(&ic->ic_wait_for_init_cc_response);
}

qdf_netdev_t ol_ath_extract_netdev(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return NULL;
    }

    return ic->ic_netdev;
}

bool ol_ath_pdev_is_80p80_supported(struct wlan_objmgr_pdev *pdev)
{
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);

    if (!ic)
        return false;

    return ieee80211_is_any_80p80_supported(ic, false);
}

bool ol_ath_is_freq_80p80_supported(struct wlan_objmgr_pdev *pdev,
                                    qdf_freq_t primary_freq)
{
    struct ieee80211com *ic = wlan_pdev_get_mlme_ext_obj(pdev);
    qdf_freq_t sec_cfreq;
    uint8_t sec_ieee;

    if (!ic)
        return false;

    return ieee80211_is_80p80mhz_supported(ic, primary_freq, &sec_cfreq,
                                           &sec_ieee, false, REG_CURRENT_PWR_MODE);
}

#if defined(CONFIG_AFC_SUPPORT) && defined(WLAN_FEATURE_11BE)
QDF_STATUS ol_ath_fill_afc_min_psd(struct wlan_objmgr_pdev *pdev,
                                   qdf_freq_t primary_freq,
                                   qdf_freq_t cen320,
                                   uint16_t punc_pattern,
                                   uint16_t bw,
                                   int16_t *min_psd)
{
    QDF_STATUS status;
    struct ieee80211com *ic;
    qdf_freq_t chan_center;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    /* The API to find channel center assumes BEST POWER MODE and there is no
     * need to use puncturing bitmap to find channel center.
     */
    chan_center = ieee80211_find_ch_center(ic, primary_freq, cen320,
                                           bw, NO_PUNCTURE_BITMASK);
    status = reg_get_6g_min_psd(pdev, primary_freq, chan_center, punc_pattern, bw,
                             min_psd);
    /* The value given by reg_get_6g_min_psd API is min_psd * 10. Dividing it by 10 here */
    *min_psd = *min_psd / 10;

    return status;
}
#endif

/*
 * Enable duration based Tx mode selection per radio
 */
void ol_ath_set_duration_based_tx_mode_select(struct wlan_objmgr_pdev *pdev,
                                       uint32_t tx_mode_select_enable)
{
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211com *ic;
    struct wmi_pdev_enable_tx_mode_selection tx_mode_select_param;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&tx_mode_select_param, sizeof(tx_mode_select_param), 0);
    tx_mode_select_param.pdev_id = lmac_get_pdev_idx(pdev);
    tx_mode_select_param.enable_tx_mode_selection = tx_mode_select_enable;

    (void)wmi_unified_set_radio_tx_mode_select_cmd_send(pdev_wmi_handle,
                    &tx_mode_select_param);
}

/*
 * Enable low latency mode
 */
QDF_STATUS ol_ath_enable_low_latency_mode(struct wlan_objmgr_pdev *pdev,
                                       uint32_t enable_low_latency_mode)
{
    if (!pdev) {
        qdf_err("pdev is NULL");
        return -EINVAL;
    }

    return ol_ath_pdev_set_param(pdev, wmi_pdev_param_low_latency_mode,
                                  enable_low_latency_mode);
}
int ol_ath_pdev_set_peer_disable_mode(struct wmi_unified *pdev_wmi_handle,
				      uint8_t *peer_mac,
				      uint8_t pdev_id, uint32_t disabled_modes)
{
	return wmi_unified_set_peer_disable_mode(pdev_wmi_handle, peer_mac,
						 pdev_id,
						 disabled_modes);
}

#ifdef CONFIG_SAWF_DEF_QUEUES
int ol_ath_pdev_set_rate_upper_cap(struct wlan_objmgr_pdev *pdev,
                                   uint8_t nss, uint8_t mcs,
                                   uint8_t en_nss_cap, uint8_t en_mcs_cap)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;
    struct wmi_rc_params param;
    uint8_t pdev_id;
    QDF_STATUS status;

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.upper_cap_nss = nss;
    param.upper_cap_mcs = mcs;
    param.en_nss_cap = en_nss_cap;
    param.en_mcs_cap = en_mcs_cap;

    status = wmi_unified_set_rate_upper_cap_cmd_send(pdev_wmi_handle, pdev_id,
                                                     &param);
    if (status != QDF_STATUS_SUCCESS)
        return qdf_status_to_os_return(status);

    scn->rc_upper_cap_nss = nss;
    scn->rc_upper_cap_mcs = mcs;

    return 0;
}

/*
 * Get Max NSS and MCS
 */
int ol_ath_pdev_get_rate_upper_cap(struct wlan_objmgr_pdev *pdev,
                                   uint8_t *nss, uint8_t *mcs)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    if (nss)
        *nss = scn->rc_upper_cap_nss;
    if (mcs)
        *mcs = scn->rc_upper_cap_mcs;

    return 0;
}

int ol_ath_pdev_set_rate_retry_mcs_drop(struct wlan_objmgr_pdev *pdev,
                                        uint8_t retry_thresh, uint8_t mcs_drop,
                                        uint8_t en_retry_thresh,
                                        uint8_t en_mcs_drop)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;
    struct wmi_rc_params param;
    uint8_t pdev_id;
    QDF_STATUS status;

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.retry_thresh = retry_thresh;
    param.mcs_drop = mcs_drop;
    param.en_retry_thresh = en_retry_thresh;
    param.en_mcs_drop = en_mcs_drop;

    status = wmi_unified_set_rate_retry_mcs_drop_cmd_send(pdev_wmi_handle,
                                                          pdev_id, &param);

    if (status != QDF_STATUS_SUCCESS)
        return qdf_status_to_os_return(status);

    scn->rc_retry_thresh = retry_thresh;
    scn->rc_mcs_drop = mcs_drop;

    return 0;
}

int ol_ath_pdev_get_rate_retry_mcs_drop(struct wlan_objmgr_pdev *pdev,
                                        uint8_t *retry_thresh,
                                        uint8_t *mcs_drop)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    if (retry_thresh)
        *retry_thresh = scn->rc_retry_thresh;

    if (mcs_drop)
        *mcs_drop = scn->rc_mcs_drop;

    return 0;
}

int ol_ath_pdev_set_mcs_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t min, uint16_t max)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;
    struct wmi_rc_params param;
    uint8_t pdev_id;
    QDF_STATUS status;

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.min_mcs_probe_intvl = min;
    param.max_mcs_probe_intvl = max;

    status = wmi_unified_set_mcs_probe_intvl_cmd_send(pdev_wmi_handle,
                                                      pdev_id, &param);

    if (status != QDF_STATUS_SUCCESS)
        return qdf_status_to_os_return(status);

    scn->rc_mcs_probe_intvl_min = min;
    scn->rc_mcs_probe_intvl_max = max;

    return 0;
}

int ol_ath_pdev_get_mcs_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t *min, uint16_t *max)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    if (min)
        *min = scn->rc_mcs_probe_intvl_min;
    if (max)
        *max = scn->rc_mcs_probe_intvl_max;

    return 0;
}

int ol_ath_pdev_set_nss_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t min, uint16_t max)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;
    struct wmi_rc_params param;
    uint8_t pdev_id;
    QDF_STATUS status;

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    qdf_mem_set(&param, sizeof(param), 0);
    param.min_nss_probe_intvl = min;
    param.max_nss_probe_intvl = max;

    status = wmi_unified_set_nss_probe_intvl_cmd_send(pdev_wmi_handle,
                                                      pdev_id, &param);

    if (status != QDF_STATUS_SUCCESS)
        return qdf_status_to_os_return(status);

    scn->rc_nss_probe_intvl_min = min;
    scn->rc_nss_probe_intvl_max = max;

    return 0;
}

int ol_ath_pdev_get_nss_probe_intvl(struct wlan_objmgr_pdev *pdev,
                                    uint16_t *min, uint16_t *max)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    if (!wmi_service_enabled(pdev_wmi_handle,
                             wmi_service_pdev_rate_config_support)) {
        qdf_err("wmi_service_pdev_rate_config not supported");
        return -EOPNOTSUPP;
    }

    if (min)
        *min = scn->rc_nss_probe_intvl_min;
    if (max)
        *max = scn->rc_nss_probe_intvl_max;

    return 0;
}

#define IS_SAWF_PARAM_SET(param) (param != DEF_SAWF_CONFIG_VALUE)
static uint16_t ol_ath_get_enabled_sawf_params_mask(struct wlan_sawf_svc_class_params *svc_class_param) {
	uint16_t enabled_params = 0x0000;

	if (IS_SAWF_PARAM_SET(svc_class_param->min_thruput_rate))
		enabled_params |= (1 << SAWF_PARAM_MIN_THROUGHPUT);
	if (IS_SAWF_PARAM_SET(svc_class_param->max_thruput_rate))
		enabled_params |= (1 << SAWF_PARAM_MAX_THROUGHPUT);
	if (IS_SAWF_PARAM_SET(svc_class_param->burst_size))
		enabled_params |= (1 << SAWF_PARAM_BURST_SIZE);
	if (IS_SAWF_PARAM_SET(svc_class_param->service_interval))
		enabled_params |= (1 << SAWF_PARAM_SERVICE_INTERVAL);
	if (IS_SAWF_PARAM_SET(svc_class_param->delay_bound))
		enabled_params |= (1 << SAWF_PARAM_DELAY_BOUND);
	if (IS_SAWF_PARAM_SET(svc_class_param->msdu_ttl))
		enabled_params |= (1 << SAWF_PARAM_MSDU_TTL);
	if (IS_SAWF_PARAM_SET(svc_class_param->msdu_rate_loss))
		enabled_params |= (1 << SAWF_PARAM_MSDU_LOSS);

	return enabled_params;
}

static void ol_ath_enable_sawf_param(struct wlan_objmgr_psoc *psoc,
                            void *msg, uint8_t index) {

    struct wmi_sawf_params *param;
    struct wmi_unified *pdev_wmi_handle = NULL;
    struct wlan_objmgr_pdev *pdev;
    struct target_psoc_info *tgt_hdl;
    ol_ath_soc_softc_t *soc;

    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target_psoc_info is null");
        return;
    }

    soc = (ol_ath_soc_softc_t *)target_psoc_get_feature_ptr(tgt_hdl);

    if (!soc || !soc->soc_attached)
        return;

    pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_MLME_NB_ID);

    pdev_wmi_handle = lmac_get_pdev_wmi_unified_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return;
    }

    param = (struct wmi_sawf_params *)msg;
    wmi_sawf_create_send(pdev_wmi_handle, param);

    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
}

int ol_ath_set_sawf_params(struct wlan_objmgr_pdev *pdev,
                           struct wlan_sawf_svc_class_params *svc_class_param)
{
    struct wmi_sawf_params param;
    struct wlan_sawf_svc_class_params new_param;

    param.svc_id = svc_class_param->svc_id;
    param.min_thruput_rate = svc_class_param->min_thruput_rate;
    param.max_thruput_rate = svc_class_param->max_thruput_rate;
    param.burst_size = svc_class_param->burst_size;
    param.service_interval = svc_class_param->service_interval;
    param.delay_bound = svc_class_param->delay_bound;
    param.msdu_ttl = svc_class_param->msdu_ttl;
    param.priority = svc_class_param->priority;
    param.tid = svc_class_param->tid;
    param.msdu_rate_loss = svc_class_param->msdu_rate_loss;
    param.disabled_modes = svc_class_param->disabled_modes;
    wlan_objmgr_iterate_psoc_list(ol_ath_enable_sawf_param, &param,
                                  WLAN_MLME_NB_ID);

    /* After WMI call, the params are updated with
     * the FW provided details, so we need to update
     * the same information in our database.
     */
    new_param.svc_id = param.svc_id;
    new_param.min_thruput_rate = param.min_thruput_rate;
    new_param.max_thruput_rate = param.max_thruput_rate;
    new_param.burst_size = param.burst_size;
    new_param.service_interval = param.service_interval;
    new_param.delay_bound = param.delay_bound;
    new_param.msdu_ttl = param.msdu_ttl;
    new_param.priority = param.priority;
    new_param.tid = param.tid;
    new_param.msdu_rate_loss = param.msdu_rate_loss;

    new_param.svc_type = svc_class_param->svc_type;
    new_param.ul_burst_size = svc_class_param->ul_burst_size;
    new_param.ul_service_interval = svc_class_param->ul_service_interval;
    new_param.disabled_modes = svc_class_param->disabled_modes;
    new_param.ul_min_tput = svc_class_param->ul_min_tput;
    new_param.ul_max_latency = svc_class_param->ul_max_latency;
    new_param.enabled_param_mask =
        ol_ath_get_enabled_sawf_params_mask(svc_class_param);

    wlan_update_sawf_params_nolock(&new_param);

    return 0;
}

static void ol_ath_disable_sawf_param(struct wlan_objmgr_psoc *psoc,
                            void *msg, uint8_t index) {

    uint8_t *svc_id;
    struct wmi_unified *pdev_wmi_handle = NULL;
    struct wlan_objmgr_pdev *pdev;
    struct target_psoc_info *tgt_hdl;
    ol_ath_soc_softc_t *soc;

    tgt_hdl = wlan_psoc_get_tgt_if_handle(psoc);
    if (!tgt_hdl) {
        qdf_err("target_psoc_info is null");
        return;
    }

    soc = (ol_ath_soc_softc_t *)target_psoc_get_feature_ptr(tgt_hdl);

    if (!soc || !soc->soc_attached)
        return;

    pdev = wlan_objmgr_get_pdev_by_id(psoc, 0, WLAN_MLME_NB_ID);

    pdev_wmi_handle = lmac_get_pdev_wmi_unified_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return;
    }

    svc_id = (uint8_t *)msg;
    wmi_sawf_disable_send(pdev_wmi_handle, *svc_id);

    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_NB_ID);
}


int ol_ath_disable_sawf(uint8_t svc_id)
{

    wlan_objmgr_iterate_psoc_list(ol_ath_disable_sawf_param, &svc_id,
                                  WLAN_MLME_NB_ID);

    return 0;
}
#endif

int ol_ath_pdev_set_disable_mode(struct wlan_objmgr_pdev *pdev,
				 uint32_t disabled_modes)
{
    struct ieee80211com *ic;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *pdev_wmi_handle;
    uint8_t pdev_id;

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return -EINVAL;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return -EINVAL;
    }

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle) {
        qdf_err("pdev wmi handle is null");
        return -EINVAL;
    }

    return ol_ath_pdev_set_param(pdev, wmi_pdev_param_set_disabled_sched_modes,
                                 disabled_modes);
}

QDF_STATUS ol_ath_get_opclass_tbl_idx(struct wlan_objmgr_pdev *pdev,
                                      uint8_t *opclass_tbl_idx)
{
    struct ieee80211com *ic;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    *opclass_tbl_idx = ic->ic_opclass_tbl_idx;

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS ol_ath_end_r2p_table_update_wait(struct wlan_objmgr_psoc *psoc,
                                            uint32_t pdev_id)
{
    struct ieee80211com *ic;
    struct wlan_objmgr_pdev *pdev;
    struct wmi_unified *wmi_handle;
    wlan_objmgr_ref_dbgid dbg_id = WLAN_REGULATORY_NB_ID;

    wmi_handle = lmac_get_wmi_hdl(psoc);

    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return QDF_STATUS_E_FAILURE;
    }

    pdev_id = wmi_handle->ops->convert_target_pdev_id_to_host(wmi_handle, pdev_id);
    pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, dbg_id);
    if(!pdev) {
        qdf_err("pdev is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("ic is NULL");
        wlan_objmgr_pdev_release_ref(pdev, dbg_id);
        return QDF_STATUS_E_FAILURE;
    }

    wlan_objmgr_pdev_release_ref(pdev, dbg_id);
    qdf_event_set(&ic->ic_wait_for_r2p_table_set_response);

    return QDF_STATUS_SUCCESS;
}

#if QCA_STANDALONE_SOUNDING_TRIGGER
/**
 * ol_ath_txbf_cbf_dbr_event_handler() - TxBF cbf cv/cqi data dbr event handler
 * API to handle Txbf cbf cv data dbr event
 * @pdev:  pointer to pdev object
 * @payload: dbr event buffer
 *
 * Return: true to release buf
 */
static bool
ol_ath_txbf_cbf_dbr_event_handler(struct wlan_objmgr_pdev *pdev,
                                  struct direct_buf_rx_data *payload)
{

    struct wlan_objmgr_psoc *psoc;
    struct txbf_cv_report report = {0};
    struct txbf_cv_hdr *hdr;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = NULL;
    int idx =0;

    if (!pdev || !payload) {
        qdf_err("PDEV or payload is NULL\n");
        return true;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        qdf_err("psoc is null");
        return true;
    }
    ic  =  wlan_pdev_mlme_get_ext_hdl(pdev);
    if (!ic) {
        qdf_err("ic is NULL!");
        return true;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return true;
    }

    if (!scn->sc_host_trig_sounding_support) {
        qdf_err("TxBF Standalone Sounding is not enabled");
        return true;
    }

    if (payload->cv_meta_data_valid) {
        hdr = &report.cv_hdr;
        report.data = (uint8_t *)payload->vaddr;
        report.data_len = payload->dbr_len;
        report.is_valid = payload->cv_meta_data.is_valid;

        hdr->asnr_len = payload->cv_meta_data.asnr_len;
        report.asnr_data =  report.data + payload->cv_meta_data.asnr_offset;
        hdr->dsnr_len = payload->cv_meta_data.dsnr_len;
        report.dsnr_data =  report.data + payload->cv_meta_data.dsnr_offset;

        qdf_mem_copy(hdr->peer_mac, &payload->cv_meta_data.peer_mac,
                sizeof(hdr->peer_mac));
        hdr->fb_type = payload->cv_meta_data.fb_type;
        hdr->fb_params = payload->cv_meta_data.fb_params;

        QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,
                  "peer_mac:%s", ether_sprintf(hdr->peer_mac));

        QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,
                  "fb_type:%s fb_params:0x%x ",
                  "asnr_offset:%d asnr_len:%d dsnr_offset:%d dsnr_len:%d",
                  payload->cv_meta_data.fb_type, payload->cv_meta_data.fb_params,
                  payload->cv_meta_data.asnr_offset, hdr->asnr_len,
                  payload->cv_meta_data.dsnr_offset, hdr->dsnr_len);

        /* send report to relayfs */
        if (QDF_IS_STATUS_ERROR(txbf_cbf_send_cv_data(ic, &report))) {
            qdf_err("cbf cv data to relayfs failed");
        }
    } else if (payload->cqi_meta_data_valid) {
        /* Loop through number of users in cqi metadata
         * and push respective user data to relayfs.
         */
        for (idx = 0; idx < payload->cqi_meta_data.num_users; idx++) {
            qdf_mem_zero(&report, sizeof(report));
            hdr = &report.cv_hdr;
            report.data = (uint8_t *)payload->vaddr;
            report.data_len = payload->dbr_len;

            report.is_valid = payload->cqi_meta_data.is_valid;
            qdf_mem_copy(hdr->peer_mac,
                    &payload->cqi_meta_data.user_info[idx].peer_mac,
                    sizeof(hdr->peer_mac));
            hdr->fb_type = TXBF_SOUNDING_FEEDBACK_TYPE_CQI;
            hdr->fb_params = payload->cqi_meta_data.user_info[idx].fb_params;
            hdr->asnr_len = payload->cqi_meta_data.user_info[idx].asnr_len;
            report.asnr_data =  report.data +
                payload->cqi_meta_data.user_info[idx].asnr_offset;

            QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,
                    "peer_mac:%s", ether_sprintf(hdr->peer_mac));

            QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,
                    "fb_type:%d fb_params:0x%x "
                    "asnr_offset:%d asnr_len:%d ",
                    hdr->fb_type, hdr->fb_params,
                    payload->cqi_meta_data.user_info[idx].asnr_offset,
                    hdr->asnr_len);
            /* send report to relayfs */
            if (QDF_IS_STATUS_ERROR(txbf_cbf_send_cv_data(ic, &report))) {
                qdf_err("cbf cqi data to relayfs failed");
            }
        }
    } else {
        qdf_err("Invalid cv/cqi meta_data");
    }

    return true;
}

#ifdef DIRECT_BUF_RX_ENABLE
QDF_STATUS
ol_ath_tgt_txbf_cbf_register_to_dbr(struct wlan_objmgr_pdev *pdev)
{
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_direct_buf_rx_tx_ops *dbr_tx_ops;
    struct dbr_module_config dbr_config;
    struct wlan_lmac_if_tx_ops *tx_ops;

    psoc = wlan_pdev_get_psoc(pdev);
    tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
    if (!tx_ops) {
        qdf_err("tx_ops is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    dbr_tx_ops = &tx_ops->dbr_tx_ops;
    dbr_config.num_resp_per_event = DBR_NUM_RESP_PER_EVENT_TXBF_CBF;
    dbr_config.event_timeout_in_ms = DBR_EVENT_TIMEOUT_IN_MS_TXBF_CBF;
    if (dbr_tx_ops->direct_buf_rx_module_register) {
        return dbr_tx_ops->direct_buf_rx_module_register
            (pdev, DBR_MODULE_CBF, &dbr_config,
             ol_ath_txbf_cbf_dbr_event_handler);
    } else {
        qdf_err("direct_buf_rx_module_register is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
ol_ath_tgt_txbf_cbf_unregister_to_dbr(struct wlan_objmgr_pdev *pdev)
{
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_direct_buf_rx_tx_ops *dbr_tx_ops = NULL;
    struct wlan_lmac_if_tx_ops *tx_ops;

    psoc = wlan_pdev_get_psoc(pdev);
    tx_ops = wlan_psoc_get_lmac_if_txops(psoc);
    if (!tx_ops) {
        qdf_err("tx_ops is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    dbr_tx_ops = &tx_ops->dbr_tx_ops;
    if (dbr_tx_ops->direct_buf_rx_module_unregister) {
        return dbr_tx_ops->direct_buf_rx_module_unregister(pdev, DBR_MODULE_CBF);
    } else {
        qdf_err("direct_buf_rx_module_unregister is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

#else
QDF_STATUS
ol_ath_tgt_txbf_cbf_register_to_dbr(struct wlan_objmgr_pdev *pdev)
{
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
ol_ath_tgt_txbf_cbf_unregister_to_dbr(struct wlan_objmgr_pdev *pdev)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* DIRECT_BUF_RX_ENABLE */

QDF_STATUS
ol_ath_pdev_host_trig_txbf_sounding_init(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_psoc *psoc;

    psoc = wlan_pdev_get_psoc(pdev);
    wmi_handle = lmac_get_wmi_hdl(psoc);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return QDF_STATUS_E_FAILURE;
    }

    if (!ic) {
        qdf_err("ic is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    if (wmi_service_enabled(wmi_handle, wmi_service_standalone_sound) &&
            wlan_pdev_get_config_params(scn->sc_pdev,
                WLAN_CONFIG_HOST_TRIGGER_TXBF_SOUNDING)) {

        /* Register DBR framework for TxBF  CBF module */
        if (QDF_IS_STATUS_ERROR(ol_ath_tgt_txbf_cbf_register_to_dbr(scn->sc_pdev))) {
            qdf_err("Failed to register dbr for TxBF cbf module");
            return QDF_STATUS_E_FAILURE;
        }

        /* Init relafs */
        if (QDF_IS_STATUS_ERROR(txbf_cbf_streamfs_init(scn->sc_pdev))) {
            /* Unregister DBR framework for TxBF  CBF module */
            if (ol_ath_tgt_txbf_cbf_unregister_to_dbr(scn->sc_pdev)) {
                qdf_err("Failed to unregister dbr for TxBF cbf module");
            }
            qdf_err("Failed to init relayfs for TxBF cbf");
            return QDF_STATUS_E_FAILURE;
        }
        scn->sc_host_trig_sounding_support = true;
        qdf_info("Enable TxBF Standalone sounding support");
    } else {
       qdf_info("TxBF Standalone Sounding is not enabled");
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
ol_ath_pdev_host_trig_txbf_sounding_deinit(struct wlan_objmgr_pdev *pdev)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic = wlan_pdev_mlme_get_ext_hdl(pdev);

    if (!ic) {
        qdf_err("ic is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    if (!scn) {
        qdf_err("scn is NULL!");
        return QDF_STATUS_E_FAILURE;
    }

    if (!scn->sc_host_trig_sounding_support) {
        QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,
                  "TxBF Standalone Sounding is not enabled");
        return QDF_STATUS_SUCCESS;
    }
    qdf_info(" deinit txbf standalone sounding");
    /* Deinit relayfs */
    if (txbf_cbf_streamfs_deinit(scn->sc_pdev)) {
        qdf_err("Failed to deinit relayfs for TxBF cbf");
    }

    /* Unregister DBR framework for TxBF  CBF module */
    if (ol_ath_tgt_txbf_cbf_unregister_to_dbr(scn->sc_pdev)) {
        qdf_err("Failed to unregister dbr for TxBF cbf module");
    }

    scn->sc_sounding_inprogress = false;
    scn->sc_host_trig_sounding_support = false;
    return QDF_STATUS_SUCCESS;
}

int
ol_ath_txbf_standalone_sounding_complete_event_handler(ol_soc_t sc, u_int8_t *data,
                                                       u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct wmi_host_standalone_sounding_evt_params ss_params = {0};
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ol_ath_softc_net80211 *scn;
    wlan_if_t vap;
    struct txbf_cv_status cv_status = {0};

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("null wmi_handle");
        return -EINVAL;
    }

    if (wmi_extract_standalone_sounding_evt_params(wmi_handle, data, &ss_params)) {
        qdf_err("Failed to standalone sounding event params");
        return -EINVAL;
    }

    QDF_TRACE(QDF_MODULE_ID_SOUNDING, QDF_TRACE_LEVEL_DEBUG,"[%s:%d] vdev_id:%d status:%d "
              "buffer_uploaded:%d num_sounding_repeats:%d",__func__,__LINE__,
              ss_params.vdev_id, ss_params.status,
              ss_params.buffer_uploaded, ss_params.num_sounding_repeats);

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(soc->psoc_obj,
            ss_params.vdev_id, WLAN_MLME_NB_ID);
    if (!vdev) {
        qdf_err("vdev is NULL");
        return -EINVAL;
    }
    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        return -EINVAL;
    }
    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_NB_ID);

    scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);
    if (!scn) {
        qdf_err("scn is NULL");
        return -EINVAL;
    }
    /* send error status to relayfs */
    if (!ss_params.buffer_uploaded) {
        cv_status.status = ss_params.status;
        if (QDF_IS_STATUS_ERROR(txbf_cbf_send_cv_status(vap->iv_ic, &cv_status))) {
            qdf_err("cbf cv status relayfs failed");
        }
    }

    if (scn->sc_sounding_inprogress) {
        scn->sc_sounding_inprogress = false;
    } else {
        qdf_err("Investigate sc_sounding_inprogress is reset");
    }
    return EOK;
}
#endif /* QCA_STANDALONE_SOUNDING_TRIGGER */
