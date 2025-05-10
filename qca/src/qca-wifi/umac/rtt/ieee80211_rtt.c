/*
* Copyright (c) 2016, 2018-2019, 2022 Qualcomm Innovation Center, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Innovation Center, Inc.
*
*/

/*
 * 2016 Qualcomm Atheros, Inc..
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <ieee80211_var.h>
#include <qdf_types.h> /* qdf_print */
#include "ieee80211_rtt.h"
#include "ol_if_athvar.h"

#if ATH_SUPPORT_LOWI
#include "ath_lowi_if.h"
#include "wifi_pos_api.h"
#endif /* ATH_SUPPORT_LOWI */

#include <os_if_wifi_pos_utils.h>
#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
#include <wifi_pos_pasn_api.h>
#include <cfg_ucfg_api.h>
#endif /* defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT) */

#define LOWI_MESSAGE_SUBIE_AND_LEN_OCTETS  2

#if ATH_SUPPORT_LOWI
/* Send Where are you action frame */
/* Function     : ieee80211_lowi_send_wru_frame
 * Arguments    : Pointer to data for WRU frame
 * Functionality: Creates and sends Where are you action frame
 * Return       : Void
 */
void ieee80211_lowi_send_wru_frame (struct wlan_objmgr_psoc *psoc, u_int8_t *data)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap * vap = NULL;
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211com *ic = NULL;
    struct wru_lci_request *wru = (struct wru_lci_request *)data;
    struct ieee80211_ftmrrreq *actionbuf;
    struct wlan_objmgr_peer *peer;

    peer = wlan_objmgr_get_peer_by_mac(psoc, &wru->sta_mac[0], WLAN_RTT_ID);
    if(peer == NULL) {
        qdf_err("%s: Could not find node[%s] in associated nodes table.", __func__, ether_sprintf(&wru->sta_mac[0]));
        return;
    }
    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)
    {
        qdf_err("Unable to get ni");
        wlan_objmgr_peer_release_ref(peer, WLAN_RTT_ID);
        return;
    }
    ic = ni->ni_ic;
    /* Get VAP where this node is associated */
    vap = ni->ni_vap;
    /* Make sure this VAP is active and in AP mode */
    if ((wlan_vdev_chan_config_valid(vap->vdev_obj) != QDF_STATUS_SUCCESS) ||
        (IEEE80211_M_HOSTAP != vap->iv_opmode)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_WIFIPOS,
             "%s: ERROR: VAP is either not active or not in AP mode. Not sending WRU frame\n", __func__);
        wlan_objmgr_peer_release_ref(peer, WLAN_RTT_ID);
        return;
    }
    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        ieee80211_free_node(ni, WLAN_RTT_ID);
        return;
    }
    actionbuf = (struct ieee80211_ftmrrreq *)frm;
    actionbuf->header.ia_category = IEEE80211_ACTION_CAT_RADIO;
    actionbuf->header.ia_action = IEEE80211_ACTION_MEAS_REQUEST;
    actionbuf->dialogtoken = wru->dialogtoken;
    actionbuf->num_repetitions = htole16(wru->num_repetitions);
    frm = &actionbuf->elem[0];

    OS_MEMCPY(frm, &(wru->id), wru->len + LOWI_MESSAGE_SUBIE_AND_LEN_OCTETS); //include id and len fields while copying
    frm += wru->len + LOWI_MESSAGE_SUBIE_AND_LEN_OCTETS;
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    /* If Managment Frame protection is enabled (PMF), set Privacy bit */
    if (ieee80211_vap_mfp_test_is_set(vap) ||
        (wlan_crypto_is_pmf_enabled(vap->vdev_obj, ni->peer_obj) &&
         ieee80211_node_is_authorized(ni))) {
        /* MFP is enabled, so we need to set Privacy bit */
        struct ieee80211_frame *wh;
        wh = (struct ieee80211_frame*)wbuf_header(wbuf);
        wh->i_fc[1] |= IEEE80211_FC1_WEP;
    }
    ieee80211_send_mgmt(vap, ni, wbuf, false);
    qdf_print("Where Are you frame successfully sent for %s", ether_sprintf(&wru->sta_mac[0]));
    ieee80211_free_node(ni, WLAN_RTT_ID);    /* reclaim node */
    return;
}
#endif /* ATH_SUPPORT_LOWI */

/* Send FTMRR action frame */
/* Function     : ieee80211_lowi_send_ftmrr_frame
 * Arguments    : Pointer to data for FTMRR frame
 * Functionality: Creates and sends FTMRR action frame
 * Return       : 0 on success, -1 on failure.
 */
int ieee80211_lowi_send_ftmrr_frame(struct wlan_objmgr_psoc *psoc, u_int8_t *data)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap * vap = NULL;
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211com *ic = NULL;
    struct ftmrr_request *ftmrr = (struct ftmrr_request *)data;
    struct ieee80211_ftmrrreq *actionbuf;
    struct wlan_objmgr_peer *peer;

    peer = wlan_objmgr_get_peer_by_mac(psoc, &ftmrr->sta_mac[0], WLAN_RTT_ID);
    if(peer == NULL) {
        qdf_err("%s: Could not find node[%s] in associated nodes table.", __func__, ether_sprintf(&ftmrr->sta_mac[0]));
        return -1;
    }
    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)
    {
        qdf_err("Unable to get ni");
        wlan_objmgr_peer_release_ref(peer, WLAN_RTT_ID);
        return -1;
    }
    ic = ni->ni_ic;
    /* Get VAP where this node is associated */
    vap = ni->ni_vap;
    /* Make sure vap is active and  in AP mode, else do not FTMRR frame */
    if ((wlan_vdev_chan_config_valid(vap->vdev_obj) != QDF_STATUS_SUCCESS) ||
        (IEEE80211_M_HOSTAP != vap->iv_opmode)) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_NORMAL, IEEE80211_MSG_WIFIPOS,
            "%s: ERROR: Vap is either not active or not in AP mode. Not sending FTMRR frame\n", __func__);
        wlan_objmgr_peer_release_ref(peer, WLAN_RTT_ID);
        return -1;
    }
    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        ieee80211_free_node(ni, WLAN_RTT_ID);
        return -1;
    }
    actionbuf = (struct ieee80211_ftmrrreq *)frm;
    actionbuf->header.ia_category = IEEE80211_ACTION_CAT_RADIO;
    actionbuf->header.ia_action = IEEE80211_ACTION_MEAS_REQUEST;
    actionbuf->dialogtoken = ftmrr->dialogtoken;
    actionbuf->num_repetitions = htole16(ftmrr->num_repetitions);
    frm = &actionbuf->elem[0];

    OS_MEMCPY(frm, &(ftmrr->id), ftmrr->len + LOWI_MESSAGE_SUBIE_AND_LEN_OCTETS); //include id and len fields while copying
    frm += ftmrr->len + LOWI_MESSAGE_SUBIE_AND_LEN_OCTETS;
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    /* If Managment Frame protection is enabled (PMF), set Privacy bit */
    if (ieee80211_vap_mfp_test_is_set(vap) ||
        (wlan_crypto_is_pmf_enabled(vap->vdev_obj, ni->peer_obj) &&
         ieee80211_node_is_authorized(ni))) {
        /* MFP is enabled, so we need to set Privacy bit */
        struct ieee80211_frame *wh;
        wh = (struct ieee80211_frame*)wbuf_header(wbuf);
        wh->i_fc[1] |= IEEE80211_FC1_WEP;
    }
    ieee80211_send_mgmt(vap, ni, wbuf, false);
    qdf_print("FTMRR frame successfully sent for %s", ether_sprintf(&ftmrr->sta_mac[0]));
    ieee80211_free_node(ni, WLAN_RTT_ID);    /* reclaim node */
    return 0;
}

int ieee80211_send_ftmrr_frame(struct wlan_objmgr_pdev *pdev,
                               struct ieee80211_wlanconfig_ftmrr *ftmrr_config)
{
    struct ieee80211_ftmrr ftmrr;
    struct neighbor_report_element_arr *nr;
    struct ieee80211com *ic;
    struct wlan_objmgr_psoc *psoc;
    uint8_t sub_element_len;
    uint8_t wbc_len;
    int i;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("null ic");
        return -1;
    }

    psoc = wlan_pdev_get_psoc(pdev);

    qdf_mem_zero(&ftmrr, sizeof(struct ieee80211_ftmrr));

    qdf_mem_copy(&ftmrr.sta_mac, ftmrr_config->sta_mac, IEEE80211_ADDR_LEN);
    ftmrr.dialogtoken = ++ic->ic_ftmrr_dialogtoken;
    ftmrr.element_id = RM_MEAS_REQ_ELEM_ID;

    ftmrr.meas_token = ++ic->ic_ftmrr_meas_token;
    ftmrr.meas_type = LOWI_WLAN_FTM_RANGE_REQ_TYPE;
    ftmrr.rand_inter = ftmrr_config->random_interval;
    ftmrr.min_ap_count = ftmrr_config->num_elements;

    ftmrr.len = sizeof(ftmrr.meas_token) + sizeof(ftmrr.meas_req_mode) +
                sizeof(ftmrr.meas_type) + sizeof(ftmrr.rand_inter) +
                sizeof(ftmrr.min_ap_count) +
                ftmrr.min_ap_count * sizeof(struct neighbor_report_element_arr);

    nr = (struct neighbor_report_element_arr *)&ftmrr.elem[0];
    sub_element_len = sizeof(struct neighbor_report_element_arr) -
                      sizeof(nr->sub_element_id) - sizeof(nr->sub_element_len);
    wbc_len = sizeof(nr->wbc_ch_width) + sizeof(nr->wbc_center_ch0) +
              sizeof(nr->wbc_center_ch1);

    for(i = 0; i < ftmrr.min_ap_count; i++) {
        nr = (struct neighbor_report_element_arr *)&ftmrr.elem[i];
        nr->sub_element_id = RM_NEIGHBOR_RPT_ELEM_ID;
        nr->sub_element_len = sub_element_len;
        qdf_mem_copy(nr->bssid, &ftmrr_config->elem[i].bssid,
                     IEEE80211_ADDR_LEN);
        nr->bssid_info = ftmrr_config->elem[i].bssid_info;
        nr->opclass = ftmrr_config->elem[i].opclass;
        nr->channel_num = ftmrr_config->elem[i].chan;
        nr->phytype = ftmrr_config->elem[i].phytype;

        nr->wbc_element_id = RM_WIDE_BW_CHANNEL_ELEM_ID;
        nr->wbc_len = wbc_len;
        nr->wbc_ch_width = ftmrr_config->elem[i].chwidth;
        nr->wbc_center_ch0 = ftmrr_config->elem[i].center_ch1;
        nr->wbc_center_ch1 =ftmrr_config->elem[i].center_ch2;
    }

    return ieee80211_lowi_send_ftmrr_frame(psoc, (uint8_t *)&ftmrr);
}

#if ATH_SUPPORT_LOWI
void wifi_pos_send_action_cb(struct wlan_objmgr_psoc *psoc,
                             uint32_t oem_sub_type, uint8_t *req, uint32_t len)
{
    int req_id;

    switch (oem_sub_type) {
    case TARGET_OEM_CONFIGURE_WRU:
        qdf_info("%s: Received Where Are You frame request from LOWI ", __func__);
        ieee80211_lowi_send_wru_frame(psoc, req + LOWI_MESSAGE_WRU_POSITION); //remove subtype(4),req_id(4)
        req_id = *((u_int8_t *)req+LOWI_MESSAGE_REQ_ID_POSITION);
        wifi_pos_send_report_resp(psoc, req_id, req+LOWI_MESSAGE_WRU_POSITION, LOWI_LCI_REQ_WRU_OK);
        return;

    case TARGET_OEM_CONFIGURE_FTMRR:
        qdf_info("%s: Received FTMRR frame request from LOWI ", __func__);
        ieee80211_lowi_send_ftmrr_frame(psoc, req + LOWI_MESSAGE_FTMRR_POSITION); //remove subtype(4),req_id(4)
        req_id = *((u_int8_t *)req+LOWI_MESSAGE_REQ_ID_POSITION);
        wifi_pos_send_report_resp(psoc, req_id, req+LOWI_MESSAGE_FTMRR_POSITION, LOWI_FTMRR_OK);
        return;
    }
}

QDF_STATUS wifi_pos_get_pdev_id_by_dev_name(char *dev_name, uint8_t *pdev_id,
                                            struct wlan_objmgr_psoc **psoc)
{
    struct net_device *dev = NULL;
    wlan_dev_t devhandle = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;

    dev = dev_get_by_name(&init_net, dev_name);
    if (!dev) {
        qdf_err("device %s not Found", dev_name);
        return QDF_STATUS_E_NULL_VALUE;
    }

    devhandle = ath_netdev_priv(dev);
    if (!devhandle) {
        qdf_err("null devhandle");
        goto out;
    }

    pdev = devhandle->ic_pdev_obj;
    if (!pdev) {
        qdf_err("null pdev");
        goto out;
    }

    *pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
    *psoc = wlan_pdev_get_psoc(pdev);
    if (!*psoc) {
        qdf_err("null psoc");
        goto out;
    }

    qdf_net_if_release_dev((struct qdf_net_if *)dev);
    return QDF_STATUS_SUCCESS;

out:
    qdf_net_if_release_dev((struct qdf_net_if *)dev);
    return QDF_STATUS_E_NULL_VALUE;
}

#ifdef WLAN_RTT_MEASUREMENT_NOTIFICATION
/* Based on the channel info this function can be used to terminate the RTT
 * request received from the LOWI application.
 */
QDF_STATUS wifi_pos_measurement_request_notification(
        struct wlan_objmgr_pdev *pdev, struct rtt_channel_info *chinfo)
{
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef WLAN_FEATURE_11BE
QDF_STATUS wifi_pos_get_max_fw_phymode_for_channels(struct wlan_objmgr_pdev *pdev,
        struct wifi_pos_channel_power *wifi_pos_ch, uint16_t wifi_pos_num_chans)
{
    struct ieee80211req_chaninfo *chinfo = NULL;
    struct ieee80211_channel_list_info *chlist = NULL;
    struct ieee80211com *ic = NULL;
    enum ieee80211_phymode phymode;
    int i = 0, j = 0;
    uint32_t fw_phymode = 0;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("NULL ic");
        return QDF_STATUS_E_NULL_VALUE;
    }

    chinfo = (struct ieee80211req_chaninfo *)qdf_mem_malloc(sizeof(*chinfo));
    if (!chinfo) {
        qdf_err("NULL chinfo");
        return QDF_STATUS_E_NOMEM;
    }

    chlist = (struct ieee80211_channel_list_info *)qdf_mem_malloc(sizeof(*chlist));
    if (!chlist) {
        qdf_err("NULL chlist");
        qdf_mem_free(chinfo);
        return QDF_STATUS_E_NOMEM;
    }

    ieee80211_get_radio_channel_list(ic, chinfo->ic_chans, chlist->chans,
                                     &chlist->nchans);

    /* Get the max supported FW phymode for the given channel */
    for (i = 0; i < chlist->nchans; i++) {
        /* Get the host phymode for the given channel frequency */
        phymode = ieee80211_get_phymode_for_channel_flags(
                chlist->chans[i].flags);

        /* Get the corresponding FW phymode */
        fw_phymode = wlan_get_target_phymode(ic, phymode, false);


        /* Populate the FW phymode in channel array*/
        for (j = 0; j < wifi_pos_num_chans; j++)
            if (chlist->chans[i].freq == wifi_pos_ch[j].ch_power.center_freq) {
                wifi_pos_ch[j].phy_mode = fw_phymode;
                break;
            }
    }

    qdf_mem_free(chlist);
    qdf_mem_free(chinfo);

    return QDF_STATUS_SUCCESS;
}
#else
QDF_STATUS wifi_pos_get_max_fw_phymode_for_channels(struct wlan_objmgr_pdev *pdev,
        struct wifi_pos_channel_power *wifi_pos_ch, uint16_t wifi_pos_num_chans)
{
    struct ieee80211req_chaninfo *chinfo = NULL;
    struct ieee80211req_channel_list *chlist = NULL;
    struct ieee80211com *ic = NULL;
    struct ieee80211vap *vap = NULL;
    enum ieee80211_phymode phymode;
    int i = 0, j = 0;
    uint32_t fw_phymode = 0;
    bool is_2gvht_en = 0;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic) {
        qdf_err("NULL ic");
        return QDF_STATUS_E_NULL_VALUE;
    }

    chinfo = (struct ieee80211req_chaninfo *)qdf_mem_malloc(sizeof(*chinfo));
    if (!chinfo) {
        qdf_err("NULL chinfo");
        return QDF_STATUS_E_NOMEM;
    }

    chlist = (struct ieee80211req_channel_list *)qdf_mem_malloc(sizeof(*chlist));
    if (!chlist) {
        qdf_err("NULL chlist");
        qdf_mem_free(chinfo);
        return QDF_STATUS_E_NOMEM;
    }

    ieee80211_get_channel_list(ic, chinfo->ic_chans, chlist->chans,
                               &chlist->nchans, false);

    /* RTT is triggered on the first vap, hence take the first vap to check if
     * 256 QAM is enabled.
     */
    if (ic->ic_pdev_is_2ghz_supported(ic)) {
        vap = TAILQ_FIRST(&ic->ic_vaps);
        if (vap) {
            is_2gvht_en = ieee80211_vap_256qam_is_set(vap);
        }
    }

    /* Get the max supported FW phymode for the given channel */
    for (i = 0; i < chlist->nchans; i++) {
        /* Get the host phymode for the given channel frequency */
        phymode = ieee80211_get_phymode_for_ch_flags(chlist->chans[i].flags);

        /* Get the corresponding FW phymode */
        fw_phymode = wlan_get_target_phymode(ic, phymode, is_2gvht_en);


        /* Populate the FW phymode in channel array*/
        for (j = 0; j < wifi_pos_num_chans; j++)
            if (chlist->chans[i].freq == wifi_pos_ch[j].ch_power.center_freq) {
                wifi_pos_ch[j].phy_mode = fw_phymode;
                break;
            }
    }

    qdf_mem_free(chlist);
    qdf_mem_free(chinfo);

    return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_11BE */

#if defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT)
static QDF_STATUS wlan_wifi_pos_pasn_peer_create(struct wlan_objmgr_psoc *psoc,
        struct qdf_mac_addr *peer_addr, uint8_t vdev_id)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;
    struct ieee80211_node *ni = NULL;

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id,
                                                WLAN_WIFI_POS_OSIF_ID);
    if (!vdev) {
        qdf_err("null vdev");
        wifi_pos_handle_ranging_peer_create_rsp(psoc, vdev_id, peer_addr,
                                                QDF_STATUS_E_FAILURE);
        return QDF_STATUS_E_FAILURE;
    }

    vap = wlan_vdev_get_vap(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        goto err;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is NULL");
        goto err;
    }

    if (qdf_is_macaddr_group(peer_addr) || qdf_is_macaddr_zero(peer_addr)) {
        qdf_err("Invalid peer address received reject it");
        goto err;
    }

    ni = wlan_objmgr_alloc_node(vap, WLAN_PEER_RTT_PASN, peer_addr->bytes);
    if (!ni) {
        qdf_err("ni is null");
        goto err;
    }

    IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_bss->ni_bssid);
    ni->ni_bss_node = ieee80211_try_ref_bss_node(vap, WLAN_MLME_OBJMGR_ID);
    if (!ni->ni_bss_node) {
        IEEE80211_NODE_LEAVE(ni);
        ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
        goto err;
    }

    /* Set the ni_bssid for PASN peer as it is required to send and receive the
     * PASN auth frames.
     */

    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        qdf_mem_copy(ni->ni_bssid, vap->iv_myaddr, QDF_MAC_ADDR_SIZE);
    } else {
        qdf_mem_copy(ni->ni_bssid, peer_addr->bytes, QDF_MAC_ADDR_SIZE);
    }

    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

    qdf_info("Created ranging peer peer_addr " QDF_MAC_ADDR_FMT " vdev_id %d",
            QDF_MAC_ADDR_REF(peer_addr->bytes), vdev_id);

    wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
    return QDF_STATUS_SUCCESS;

err:
    /* Post peer create failure to wifi_pos component */
    wifi_pos_handle_ranging_peer_create_rsp(psoc, vdev_id, peer_addr,
                                            QDF_STATUS_E_FAILURE);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
    return QDF_STATUS_E_FAILURE;
}

/**
 * wlan_wifi_is_ntb_supported() - Check 11az NTB support using the below
 * conditions:
 * 1) If 11az NTB responder role is enabled or
 * 2) 11az NTB initiator mode is enabled using cfg80211tool command and
 *    FW supports 11az NTB feature.
 * @psoc: Pointer to PSOC
 * @vap: Vap handle
 *
 * Return: true or false
 */
bool wlan_wifi_is_ntb_supported(struct wlan_objmgr_psoc *psoc,
                                struct ieee80211vap *vap)
{
    if (vap->rtt_responder_role & RTT_11AZ_NTB_RESPONDER_ROLE)
        return true;

    if ((vap->rtt_enable & RTT_INITIATOR_MODE) &&
        (wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_RTT_11AZ_NTB_SUPPORT)))
        return true;

    return false;
}

/**
 * wlan_wifi_is_tb_supported() - Check the 11az TB support using the below
 * conditions:
 * 1) If 11az TB responder role is enabled or
 * 2) 11az TB initiator mode is enabled using cfg80211tool command and
 *    FW supports 11az TB feature.
 * @psoc: Pointer to PSOC
 * @vap: Vap handle
 *
 * Return: true or false
 */
bool wlan_wifi_is_tb_supported(struct wlan_objmgr_psoc *psoc,
                               struct ieee80211vap *vap)
{
    if (vap->rtt_responder_role & RTT_11AZ_TB_RESPONDER_ROLE)
        return true;

    if ((vap->rtt_enable & RTT_INITIATOR_MODE) &&
        (wlan_psoc_nif_fw_ext2_cap_get(psoc, WLAN_RTT_11AZ_TB_SUPPORT)))
        return true;

    return false;
}

/**
 * wlan_wifi_pos_pasn_peer_delete() - Callback to delete ranging peer
 * @psoc: Pointer to PSOC
 * @peer_addr: Address of the peer for which PASN peer needs to be deleted
 * @vdev_id: Vdev id
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS wlan_wifi_pos_pasn_peer_delete(struct wlan_objmgr_psoc *psoc,
                                                 struct qdf_mac_addr *peer_addr,
                                                 uint8_t vdev_id,
                                                 bool no_fw_peer_delete)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211vap *vap = NULL;
    struct ieee80211_node *ni = NULL;

    vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc, vdev_id,
            WLAN_WIFI_POS_OSIF_ID);
    if (!vdev) {
        qdf_err("null vdev");
        return QDF_STATUS_E_NULL_VALUE;
    }

    vap = wlan_vdev_get_vap(vdev);
    if (!vap) {
        qdf_err("vap is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
        return QDF_STATUS_E_NULL_VALUE;
    }

    ni = ieee80211_vap_find_node(vap, peer_addr->bytes, WLAN_MLME_HANDLER_ID);
    if (!ni) {
        qdf_err("ni is NULL");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
        return QDF_STATUS_E_NULL_VALUE;
    }

    IEEE80211_NODE_LEAVE(ni);
    ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);

    wlan_objmgr_vdev_release_ref(vdev, WLAN_WIFI_POS_OSIF_ID);
    return QDF_STATUS_SUCCESS;
}

/**
 * wlan_wifi_pos_vdev_delete_resume() - Resume vdev delete operation
 * after deleting all pasn peers
 * @vdev: Pointer to objmgr vdev
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
wlan_wifi_pos_vdev_delete_resume(struct wlan_objmgr_vdev *vdev)
{
    return QDF_STATUS_SUCCESS;
}

static uint32_t
wlan_wifi_pos_get_rsta_11az_ranging_cap(struct wlan_objmgr_psoc *psoc)
{
    return cfg_get(psoc, CFG_RESPONDER_11AZ_SUPPORT);
}

static bool wlan_wifi_pos_get_rsta_sec_ltf_cap(struct wlan_objmgr_psoc *psoc)
{
    return cfg_get(psoc, CFG_RESPONDER_SECURE_LTF_SUPPORT);
}

static struct wifi_pos_legacy_ops wifi_pos_ops = {
    .pasn_peer_create_cb = wlan_wifi_pos_pasn_peer_create,
    .pasn_peer_delete_cb = wlan_wifi_pos_pasn_peer_delete,
    .pasn_vdev_delete_resume_cb = wlan_wifi_pos_vdev_delete_resume,
    .get_rsta_11az_ranging_cap = wlan_wifi_pos_get_rsta_11az_ranging_cap,
    .get_rsta_sec_ltf_cap = wlan_wifi_pos_get_rsta_sec_ltf_cap,
};

QDF_STATUS
wifi_pos_register_legacy_ops(struct wlan_objmgr_psoc *psoc)
{
    return wifi_pos_set_legacy_ops(psoc, &wifi_pos_ops);
}
#else
QDF_STATUS
wifi_pos_register_legacy_ops(struct wlan_objmgr_psoc *psoc)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* defined(WIFI_POS_CONVERGED) && defined(WLAN_FEATURE_RTT_11AZ_SUPPORT) */

void wifi_pos_register_cbs(struct wlan_objmgr_psoc *psoc)
{
    wifi_pos_register_send_action(psoc, wifi_pos_send_action_cb);
    wifi_pos_register_get_pdev_id_by_dev_name(psoc, wifi_pos_get_pdev_id_by_dev_name);
#ifdef WLAN_RTT_MEASUREMENT_NOTIFICATION
    wifi_pos_register_measurement_request_notification(
            psoc, wifi_pos_measurement_request_notification);
#endif /* WLAN_RTT_MEASUREMENT_NOTIFICATION  */
    wifi_pos_register_get_max_fw_phymode_for_channels(psoc,
            wifi_pos_get_max_fw_phymode_for_channels);
    wifi_pos_register_legacy_ops(psoc);
    osif_wifi_pos_register_ops(psoc);
}
qdf_export_symbol(wifi_pos_register_cbs);
#endif /* ATH_SUPPORT_LOWI */
