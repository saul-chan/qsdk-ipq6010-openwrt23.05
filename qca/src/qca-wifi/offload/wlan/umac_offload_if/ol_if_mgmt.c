/*
 * Copyright (c) 2015, 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2015 Qualcomm Atheros, Inc.
 */

/*
 * Copyright (c) 2011 Atheros Communications Inc.
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


/*
 * UMAC management specific offload interface functions - for power and performance offload model
 */
#include "target_type.h"
#include "ol_if_athvar.h"
#include "ol_if_ath_api.h"
#include "qdf_mem.h"
#include <init_deinit_lmac.h>
#include <cdp_txrx_ctrl.h>
#include <wdi_event_api.h>
#include <enet.h>
#include "ol_helper.h"
#include "ol_if_txrx_handles.h"
#include "a_debug.h"
#include "cdp_txrx_cmn.h"
#include <ieee80211_mlme_priv.h>

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_private.h>
#include <osif_nss_wifiol_if.h>
#include <osif_nss_wifiol_vdev_if.h>
#endif
#include <wlan_lmac_if_api.h>
#include <wlan_osif_priv.h>
#include <ieee80211_cfg80211.h>

#include "wlan_crypto_global_def.h"
#include "wlan_crypto_global_api.h"
#include <wlan_offchan_txrx_api.h>
#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif

#ifdef WLAN_CFR_ENABLE
#include <wlan_cfr_utils_api.h>
#endif

#include <wlan_vdev_mgr_ucfg_api.h>
#include <wlan_vdev_mgr_utils_api.h>
#include <wlan_mlme_vdev_mgmt_ops.h>
#include <wlan_utility.h>
#include <target_if_mgmt_txrx_rx_reo.h>
#include <wlan_mgmt_txrx_rx_reo_utils_api.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_cmn.h>
#include <wlan_mlo_mgr_peer.h>
#endif
#include "pld_common.h"
#ifdef WLAN_FEATURE_11BE
#include <wmi_unified_11be_api.h>
#endif /* WLAN_FEATURE_11BE */
#include<target_if_mlo_mgr.h>

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#include <wlan_mlo_mgr_sta.h>
#endif

#if ATH_PERF_PWR_OFFLOAD
#define NDIS_SOFTAP_SSID  "NDISTEST_SOFTAP"
#define NDIS_SOFTAP_SSID_LEN  15
#define OFFCHAN_EXT_TID_NONPAUSE    19
#define MGMT_TARGET_SUPPORTED_MAX_WMI   64
#define MGMT_TARGET_SUPPORTED_MAX_HTT   32
#define WMI_MGMT_DESC_POOL_SIZE 512
#define OL_TXRX_MGMT_TYPE_BASE htt_cmn_pkt_num_types
#define OL_TXRX_MGMT_NUM_TYPES 8
#define BCN_MAX_PROCESS_LIMIT  100

#define LINK_CONTROL_FLAG_TX_LINK_TUPLE_BIT     0
#define LINK_CONTROL_FLAG_PREFFERED_LINK_BIT    1
#define WMI_TX_SEND_FLAG_SET_PS_BUFFERING_FOR_SA_QUERY 0x00000002
#define WMI_TX_SEND_FLAG_SET_IS_SA_QUERY  0x00000004

static u_int32_t
ol_ath_net80211_rate_node_update(struct ieee80211com *ic,
                                 struct ieee80211_node *ni,
                                 int isnew);
int wmi_mgmt_desc_pool_init(struct ieee80211com *ic, uint32_t pool_size);
void wmi_mgmt_desc_pool_deinit(struct ieee80211com *ic);
struct wmi_mgmt_desc_t *wmi_mgmt_desc_get(struct ieee80211com *ic);
void wmi_mgmt_desc_put(struct ieee80211com *ic, struct wmi_mgmt_desc_t *wmi_mgmt_desc);
void ol_ath_update_dp_stats(void *soc, enum WDI_EVENT event, void *stats, uint16_t id, uint32_t type);
void ieee80211_csa_interop_update(void *pdev, enum WDI_EVENT event, void *stats, uint16_t id, uint32_t type);

/*
 *  WMI API for 802.11 management frame processing
 */
static uint32_t
ol_map_phymode_to_wmimode(struct ieee80211_node *ni)
{
    uint32_t phymode;
    struct ol_ath_softc_net80211 *scn = NULL;
    bool is_2gvht_en;
    enum wmi_target_type wmi_tgt_type;


    is_2gvht_en = ieee80211_vap_256qam_is_set(ni->ni_vap) &&
                  (ni->ni_flags & IEEE80211_NODE_VHT);
    phymode = ni->ni_phymode;

    if (!ni->ni_ic) {
        qdf_err("ni_ic is null");
        return WMI_HOST_MODE_UNKNOWN;
    }
    scn = OL_ATH_SOFTC_NET80211(ni->ni_ic);
    if (!scn) {
        qdf_err("scn is null");
        return WMI_HOST_MODE_UNKNOWN;
    }

    if (ol_ath_get_wmi_target_type(scn->soc, &wmi_tgt_type) != QDF_STATUS_SUCCESS) {
        qdf_info("Not able to get wmi target type");
        return WMI_HOST_MODE_UNKNOWN;
    }

    /* For non tlv chipsets there is a difference in behaviour b/w
     * phymode send on vdev start and peer assoc. Following table
     * captures this info.
     *  ____________________________________________________________
     * |AP(vht_11ng) |STA (vht_11ng) | Mode to FW(peer assoc)       |
     * |_____________|_______________|______________________________|
     * |1            |   1           | WMI_HOST_MODE_11AC_VHT20     |
     * |1            |   0           | WMI_HOST_MODE_11NG_HT20      |
     * |0            |   0           | WMI_HOST_MODE_11NG_HT20      |
     * |0            |   1           | WMI_HOST_MODE_11NG_HT20      |
     * |_____________|_______________|______________________________|
     *
     *  ____________________________________________________________
     * |AP(vht_11ng) |STA (vht_11ng) | Mode to FW(vdev start)       |
     * |_____________|_______________|______________________________|
     * |1            |   1           | WMI_HOST_MODE_11NG_HT20      |
     * |1            |   0           | WMI_HOST_MODE_11NG_HT20      |
     * |0            |   0           | WMI_HOST_MODE_11NG_HT20      |
     * |0            |   1           | WMI_HOST_MODE_11NG_HT20      |
     * |_____________|_______________|______________________________|
     */
     if (wmi_tgt_type == WMI_NON_TLV_TARGET && is_2gvht_en) {
        switch(phymode) {
            case IEEE80211_MODE_11NG_HT20:
                return WMI_HOST_MODE_11AC_VHT20;
            break;
            case IEEE80211_MODE_11NG_HT40PLUS:
            case IEEE80211_MODE_11NG_HT40MINUS:
            case IEEE80211_MODE_11NG_HT40:
                return WMI_HOST_MODE_11AC_VHT40;
            break;
            default:
            break;
        }
    }

    return(ol_get_phymode_info(scn, phymode, is_2gvht_en));
}

#ifdef WLAN_FEATURE_11BE
void fill_assoc_11be_params(struct ieee80211_node *ni,
                            struct peer_assoc_params *param)
{
    struct ieee80211vap *vap = NULL;

    if (!ni) {
        qdf_err("ni is null");
        return;
    }

    if (!param) {
        qdf_err("param is null");
        return;
    }

    vap = ni->ni_vap;
    if (!vap) {
        qdf_err("vap is null");
        return;
    }

    if (!(ni->ni_ext_flags & IEEE80211_NODE_EHT)) {
        return;
    }

    param->eht_flag = TRUE;
    if (ieee80211_vap_wme_is_set(vap)) {
        param->qos_flag = TRUE;
    }
    param->peer_eht_ops = ni->ni_eht.ehtop_param;
#ifdef WLAN_FEATURE_11BE
    /**
     * Flip the puncture bitmap sent to FW here as target expects the puncture
     * bitmap to be in old format where 0 represents subchannel punctured and
     * 1 represents subchannel not punctured. This can be removed once the FW is
     * updated.
     */

    param->puncture_bitmap = ni->ni_ch_param.puncture_bitmap;
#endif

    OL_IF_MSG_COPY_CHAR_ARRAY(&param->peer_eht_cap_macinfo,
                              &(ni->ni_eht.ehtcap_macinfo),
                              qdf_min(sizeof(param->peer_eht_cap_macinfo),
                              sizeof(ni->ni_eht.ehtcap_macinfo)));
    qdf_mem_copy(&param->peer_eht_cap_phyinfo, &(ni->ni_eht.ehtcap_phyinfo),
                             sizeof(param->peer_eht_cap_phyinfo));

    if (IEEE80211_IS_EHTCAP_MACINFO(ni->ni_eht.ehtcap_macinfo)) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s EHT MAC Capabilities info: All bits are 0", __func__);
    }

    if (ieee80211_is_phymode_11be(ni->ni_phymode)) {
        uint32_t *peer_eht_rx_mcs_set = param->peer_eht_rx_mcs_set;
        uint32_t *peer_eht_tx_mcs_set = param->peer_eht_tx_mcs_set;
        uint32_t *ni_eht_rx_mcs_set   = ni->ni_eht.ehtcap_rxmcsnssmap;
        uint32_t *ni_eht_tx_mcs_set   = ni->ni_eht.ehtcap_txmcsnssmap;


        param->peer_eht_mcs_count = 0;
        qdf_mem_copy(&param->peer_eht_ppet, &(ni->ni_eht.ehtcap_ppet),
                             sizeof(param->peer_eht_ppet));

       peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX2] =
               ni_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_320];
       peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX2] =
               ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_320];

       param->peer_eht_mcs_count++;
       /* fall through */

       peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX1] =
               ni_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_160];
       peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX1] =
               ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_160];

       param->peer_eht_mcs_count++;
       /* fall through */

       if ((ni->ni_phymode != IEEE80211_MODE_11BEA_EHT20) &&
               (ni->ni_phymode != IEEE80211_MODE_11BEG_EHT20)) {
           peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX0] =
               ni_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_80];
           peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX0] =
               ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_80];
       } else {
           peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX0] =
               ni_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_ONLY20];
           peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX0] =
               ni_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX_ONLY20];
       }

       param->peer_eht_mcs_count++;

       IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s eht mcs nss set:  320MHz rx 0x%x tx 0x%x"
            "160MHz rx 0x%x tx 0x%x 80/20 rx 0x%x tx 0x%x",
             __func__,
             peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX2],
             peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX2],
             peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX1],
             peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX1],
             peer_eht_rx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX0],
             peer_eht_tx_mcs_set[EHTCAP_TXRX_MCS_NSS_IDX0]);
    }
}
#else
void fill_assoc_11be_params(struct ieee80211_node *ni,
                            struct peer_assoc_params *param)
{
}
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE_MLO
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_0 0
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_1 (1U << 4)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_2 (1U << 5)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_3 (1U << 6)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_4 (1U << 7)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_5 (1U << 8)

/**
 * get_emlsr_tran_delay() - API to get eMLSR Transition delay in us
 * @emlsr_trans_delay: eMLSR Transition delay value advertised in Basic ML IE
 * @emlsr_trans_delay_us: Pointer to save the eMLSR Transition delay in us
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS get_emlsr_tran_delay(uint8_t emlsr_trans_delay,
                                       uint16_t *emlsr_trans_delay_us)
{
    uint16_t delay_us[WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_INVALIDSTART] = {
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_0,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_1,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_2,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_3,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_4,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_5};

    if (!emlsr_trans_delay_us)
        return QDF_STATUS_E_NULL_VALUE;

    if (emlsr_trans_delay >= WLAN_ML_BV_CINFO_EMLCAP_EMLSRTRANSDELAY_INVALIDSTART)
        return QDF_STATUS_E_INVAL;

    *emlsr_trans_delay_us = delay_us[emlsr_trans_delay];
    return QDF_STATUS_SUCCESS;
}

#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_0 0
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_1 (1U << 5)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_2 (1U << 6)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_3 (1U << 7)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_4 (1U << 8)
/**
 * get_emlsr_pad_delay() - API to get eMLSR Padding delay in us
 * @emlsr_pad_delay: eMLSR Padding delay value advertised in Basic ML IE
 * @emlsr_pad_delay_us: Pointer to save the eMLSR Padding delay in us
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS get_emlsr_pad_delay(uint8_t emlsr_pad_delay,
                                      uint16_t *emlsr_pad_delay_us)
{
    uint16_t delay_us[WLAN_ML_BV_CINFO_EMLCAP_EMLSRDELAY_INVALIDSTART] = {
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_0,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_1,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_2,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_3,
        WLAN_ML_BV_CINFO_EMLCAP_EMLSRPADDELAY_4};

    if (!emlsr_pad_delay_us)
        return QDF_STATUS_E_NULL_VALUE;

    if (emlsr_pad_delay >= WLAN_ML_BV_CINFO_EMLCAP_EMLSRDELAY_INVALIDSTART)
        return QDF_STATUS_E_INVAL;

    *emlsr_pad_delay_us =  delay_us[emlsr_pad_delay];
    return QDF_STATUS_SUCCESS;
}

#define WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_0 0
#define WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_1 (1U << 5)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_2 (1U << 6)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_3 (1U << 7)
#define WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_4 (1U << 8)
/**
 * get_emlmr_delay() - API to get eMLMR delay in us
 * @emlsr_delay: eMLMR delay value advertised in Basic ML IE
 * @emlsr_delay_us: Pointer to save the eMLMR delay in us
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS get_emlmr_delay(uint8_t emlmr_delay,
                                  uint16_t *emlmr_delay_us)
{
    uint16_t delay_us[WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_INVALIDSTART] = {
        WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_0,
        WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_1,
        WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_2,
        WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_3,
        WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_4};

    if (!emlmr_delay_us)
        return QDF_STATUS_E_NULL_VALUE;

    if (emlmr_delay >= WLAN_ML_BV_CINFO_EMLCAP_EMLMRDELAY_INVALIDSTART)
        return QDF_STATUS_E_INVAL;

    *emlmr_delay_us =  delay_us[emlmr_delay];
    return QDF_STATUS_SUCCESS;
}

#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_0  0
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_1  (1U << 7)
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_2  (1U << 8)
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_3  (1U << 9)
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_4  IEEE80211_TU_TO_USEC(1)
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_5  IEEE80211_TU_TO_USEC((1U << 1))
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_6  IEEE80211_TU_TO_USEC((1U << 2))
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_7  IEEE80211_TU_TO_USEC((1U << 3))
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_8  IEEE80211_TU_TO_USEC((1U << 4))
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_9  IEEE80211_TU_TO_USEC((1U << 5))
#define WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_10 IEEE80211_TU_TO_USEC((1U << 6))
/**
 * get_eml_trans_timeout() - API to get EML transition timeout in us
 * @eml_trans_timeout: Transition Timeout value advertised in Basic ML IE
 * @eml_trans_timeout_us: Pointer to save the Transition Timeout in us
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS get_eml_trans_timeout(uint8_t eml_trans_timeout,
                                        uint32_t *eml_trans_timeout_us)
{
    uint32_t timeout_us[WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_INVALIDSTART] = {
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_0,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_1,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_2,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_3,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_4,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_5,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_6,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_7,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_8,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_9,
        WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_10};

    if (!eml_trans_timeout_us)
        return QDF_STATUS_E_NULL_VALUE;

    if (eml_trans_timeout >= WLAN_ML_BV_CINFO_EMLCAP_TRANSTIMEOUT_INVALIDSTART)
        return QDF_STATUS_E_INVAL;

    *eml_trans_timeout_us =  timeout_us[eml_trans_timeout];

    return QDF_STATUS_SUCCESS;
}

void fill_assoc_mlo_params(struct ieee80211_node *ni,
                           struct peer_assoc_params *param)
{
    struct ieee80211vap *vap;
    struct ieee80211com *ic;
    struct wlan_objmgr_peer *peer;
    struct wlan_mlo_peer_context *ml_peer;
    struct wlan_mlo_link_peer_entry *peer_entry;
    uint8_t i, idx = 0;
    bool is_eml_enabled = false;
    uint8_t link_id;
    struct mlo_tgt_partner_info ml_links = {0};
    struct wlan_mlo_eml_cap *ml_emlcap;
    struct mlo_mgr_context *mlo_mgr_ctx;

    if (!ni) {
        qdf_err("ni is null");
        return;
    }

    if (!param) {
        qdf_err("param is null");
        return;
    }

    vap = ni->ni_vap;
    if (!vap) {
        qdf_err("vap is null");
        return;
    }

    ic = vap->iv_ic;
    if (!ic) {
        qdf_err("ic is null");
        return;
    }

    if (!(ni->ni_ext_flags & IEEE80211_NODE_EHT)) {
        return;
    }
    peer = ni->peer_obj;

    if (!wlan_peer_is_mlo(peer)) {
        mlo_debug("Peer is Not MLO capable");
        return;
    }
    ml_peer = peer->mlo_peer_ctx;
    param->mlo_params.mlo_enabled = 1;

    param->ml_links.num_links = 0;
    for (i=0; i<MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry->link_peer)
             continue;
        idx++;
        if (peer_entry->link_peer == peer) {
            param->mlo_params.mlo_assoc_link = wlan_peer_mlme_is_assoc_peer(peer);
            param->mlo_params.mlo_primary_umac = peer_entry->is_primary;
            param->mlo_params.mlo_logical_link_index_valid = 1;
            param->mlo_params.logical_link_index = idx -1;
            param->mlo_params.mlo_peer_id_valid = 1;
            param->mlo_params.ml_peer_id = ml_peer->mlo_peer_id;
            param->mlo_params.mlo_bridge_peer = peer->mlo_bridge_peer;
            qdf_copy_macaddr((struct qdf_mac_addr *)&param->mlo_params.mld_mac,
                             &ml_peer->peer_mld_addr);

            break;
        }
    }

    param->ml_links.num_links = 0;

    if (vap->iv_opmode == IEEE80211_M_STA) {
        mlo_info("Fill assoc mlo params for vdev:%d psoc:%d",
                 wlan_vdev_get_id(vap->vdev_obj),
                 wlan_vdev_get_psoc_id(vap->vdev_obj));
        if (mlo_is_sta_bridge_vdev(vap->vdev_obj)) {
            wlan_mlo_peer_get_bridge_sta_links_info(vap->vdev_obj, &ml_links);
        } else {
            wlan_mlo_peer_get_sta_links_info(peer, &ml_links);
        }

    }
    else
        wlan_mlo_peer_get_links_info(peer, &ml_links);

    if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
        (!ic->ic_get_enable_eml_cap ||
         (QDF_STATUS_SUCCESS != ic->ic_get_enable_eml_cap(ic, &is_eml_enabled)))) {
        mlo_err("Failed to get EML configuration");
    }

    /* Handle disable EML requested by CLI commad */
    mlo_mgr_ctx = wlan_objmgr_get_mlo_ctx();
    if (mlo_mgr_ctx && mlo_mgr_ctx->disable_eml && is_eml_enabled)
        is_eml_enabled = false;

    if (is_eml_enabled) {
        ml_emlcap = &ml_peer->mlpeer_emlcap;
        param->mlo_params.emlsr_support = ml_emlcap->emlsr_supp;
        param->mlo_params.emlmr_support = ml_emlcap->emlmr_supp;
        if (get_emlsr_pad_delay(ml_emlcap->emlsr_pad_delay,
                                &param->mlo_params.emlsr_pad_delay_us) != QDF_STATUS_SUCCESS)
            mlo_err("Invalid emlsr_pad_delay=%u for %s peer",
                    ml_emlcap->emlsr_pad_delay,
                    ether_sprintf(ni->ni_macaddr));
        if (get_emlsr_tran_delay(ml_emlcap->emlsr_trans_delay,
                                 &param->mlo_params.emlsr_trans_delay_us) != QDF_STATUS_SUCCESS)
            mlo_err("Invalid emlsr_trans_delay=%u for %s peer",
                    ml_emlcap->emlsr_trans_delay,
                    ether_sprintf(ni->ni_macaddr));
        if (get_emlmr_delay(ml_emlcap->emlmr_delay,
                            &param->mlo_params.emlmr_delay_us) != QDF_STATUS_SUCCESS)
            mlo_err("Invalid emlmr_delay=%u for %s peer",
                    ml_emlcap->emlmr_delay,
                    ether_sprintf(ni->ni_macaddr));
        if (get_eml_trans_timeout(ml_emlcap->trans_timeout,
                                  &param->mlo_params.trans_timeout_us) != QDF_STATUS_SUCCESS)
            mlo_err("Invalid emlsr_pad_delay=%u for %s peer",
                    ml_emlcap->emlsr_pad_delay,
                    ether_sprintf(ni->ni_macaddr));
    }

    param->mlo_params.max_num_simultaneous_links = ml_peer->mlpeer_mldcap.max_simult_link;

    link_id = wlan_vdev_get_link_id(vap->vdev_obj);
    param->mlo_params.ieee_link_id = link_id;

    for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
        if (link_id == ml_peer->mlpeer_nstrinfo[i].link_id) {
            param->mlo_params.nstr_bitmap_present = ml_peer->mlpeer_nstrinfo[i].nstr_lp_present;
            if (param->mlo_params.nstr_bitmap_present) {
                param->mlo_params.nstr_bitmap_size = ml_peer->mlpeer_nstrinfo[i].nstr_bmp_size;
                param->mlo_params.nstr_indication_bitmap = ml_peer->mlpeer_nstrinfo[i].nstr_lp_bitmap;
            }
            break;
        }
    }

    for (i = 0; i < ml_links.num_partner_links; i++) {
        param->ml_links.num_links++;
        param->ml_links.partner_info[i].vdev_id = ml_links.link_info[i].vdev_id;
        param->ml_links.partner_info[i].hw_mld_link_id = ml_links.link_info[i].hw_mld_link_id;
        param->ml_links.partner_info[i].mlo_enabled = ml_links.link_info[i].mlo_enabled;
        param->ml_links.partner_info[i].mlo_assoc_link = ml_links.link_info[i].mlo_assoc_link;
        param->ml_links.partner_info[i].mlo_primary_umac = ml_links.link_info[i].mlo_primary_umac;
        param->ml_links.partner_info[i].mlo_logical_link_index_valid = ml_links.link_info[i].mlo_logical_link_index_valid;
        param->ml_links.partner_info[i].emlsr_support = ml_links.link_info[i].emlsr_support;
        param->ml_links.partner_info[i].logical_link_index = ml_links.link_info[i].logical_link_index;
        param->ml_links.partner_info[i].mlo_bridge_peer = ml_links.link_info[i].mlo_bridge_peer;
        param->ml_links.partner_info[i].link_id = ml_links.link_info[i].ieee_link_id;

        if (vap->iv_opmode == IEEE80211_M_STA)
            mlo_info("partner link info %d: num_partner links %d vdev id %d, hw_mld_link_id %d is_primary %d logical_link_index %d",
                     i, ml_links.num_partner_links,
                     param->ml_links.partner_info[i].vdev_id,
                     param->ml_links.partner_info[i].hw_mld_link_id,
                     param->ml_links.partner_info[i].mlo_primary_umac,
                     param->ml_links.partner_info[i].logical_link_index);
        else
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                           "%s partner link info %d: num_partner links %d vdev id %d, hw_mld_link_id %d",
                           __func__, i, ml_links.num_partner_links,
                           param->ml_links.partner_info[i].vdev_id,
                           param->ml_links.partner_info[i].hw_mld_link_id);
    }
}

QDF_STATUS send_dp_peer_setup(struct ieee80211_node *ni)
{
    struct ieee80211com *ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct cdp_peer_setup_info peer_setup_info = {0};
    struct wlan_mlo_peer_context *ml_peer;
    struct ieee80211vap *vap;
    uint8_t vdev_id;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_objmgr_psoc *primary_psoc;
    struct wlan_mlo_link_peer_entry *peer_entry;
    struct wlan_objmgr_peer *peer;
    uint8_t i;
    QDF_STATUS status;

    if (!ni) {
        qdf_err("ni is null");
        return QDF_STATUS_E_FAILURE;
    }

    ic = ni->ni_ic;
    vap = ni->ni_vap;
    if (!vap) {
        qdf_err("VAP is null, peer setup failed for " QDF_MAC_ADDR_FMT,
                &ni->ni_macaddr);
        return QDF_STATUS_E_FAILURE;
    }

    /*
     * for non MLO vap peer setup is called in create
     * for MLO bridge VAP call peer setup from here
     */
    if (!wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj) &&
        !IEEE80211_IS_MLO_BRIDGE_VAP(vap))
        return QDF_STATUS_SUCCESS;

    vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    peer = ni->peer_obj;

    if (!wlan_peer_is_mlo(peer)) {
        status = cdp_peer_setup(soc_txrx_handle, vdev_id, ni->ni_macaddr,
                                NULL);
        if (status != QDF_STATUS_SUCCESS)
            qdf_err("[%s] CDP peer setup failed for legacy Peer "
                    QDF_MAC_ADDR_FMT "peer psoc_id %u", vap->iv_netdev_name,
                    &ni->ni_macaddr, wlan_peer_get_psoc_id(peer));
        return status;
    }
    ml_peer = peer->mlo_peer_ctx;

    peer_setup_info.mld_peer_mac = (uint8_t *)&ml_peer->peer_mld_addr;

    for (i=0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry->link_peer)
             continue;

        if (peer_entry->link_peer == peer) {
            if (peer_entry->is_primary)
                peer_setup_info.is_primary_link = 1;
            break;
        }
    }

    primary_psoc = wlan_objmgr_get_psoc_by_id(ml_peer->primary_umac_psoc_id,
                                              WLAN_MLME_SB_ID);
    if (!primary_psoc) {
        qdf_err("Unable to get primary psoc skipping peer setup for peer "
                QDF_MAC_ADDR_FMT " MLD mac addr" QDF_MAC_ADDR_FMT
                "peer psoc_id %u primary psoc_id %u", &ni->ni_macaddr,
                &ml_peer->peer_mld_addr, wlan_peer_get_psoc_id(peer),
                ml_peer->primary_umac_psoc_id);
        return QDF_STATUS_E_FAILURE;
    }

    peer_setup_info.primary_umac_id =
        pld_get_mlo_chip_id(primary_psoc->soc_objmgr.qdf_dev->dev);

    wlan_objmgr_psoc_release_ref(primary_psoc, WLAN_MLME_SB_ID);

    peer_setup_info.is_first_link = wlan_peer_mlme_is_assoc_peer(peer);
    peer_setup_info.num_links = ml_peer->link_peer_cnt;
    if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE)
        peer_setup_info.is_bridge_peer = 1;

    status = cdp_peer_setup(soc_txrx_handle, vdev_id, ni->ni_macaddr,
                            &peer_setup_info);

    if (status != QDF_STATUS_SUCCESS)
        qdf_err("CDP peer setup failed for peer " QDF_MAC_ADDR_FMT
                "MLD mac addr" QDF_MAC_ADDR_FMT "peer psoc_id %u"
                " primary psoc_id %u is_assoc_link: %u is_primary_link %u"
                " num_links %u is_bridge_peer %u", &ni->ni_macaddr,
                &ml_peer->peer_mld_addr, wlan_peer_get_psoc_id(peer),
                ml_peer->primary_umac_psoc_id, peer_setup_info.is_first_link,
                peer_setup_info.is_primary_link, peer_setup_info.num_links,
                peer_setup_info.is_bridge_peer);
    return status;
}
#else
void fill_assoc_mlo_params(struct ieee80211_node *ni,
                           struct peer_assoc_params *param)
{
}

QDF_STATUS send_dp_peer_setup(struct ieee80211_node *ni)
{
    return QDF_STATUS_SUCCESS;
}
#endif

#ifdef WLAN_FEATURE_11BE
void ol_ath_fill_t2lm_params(struct wlan_mlo_link_peer_entry *peer_entry,
        struct wlan_prev_t2lm_negotiated_info *t2lm_negotiated_info,
        struct wmi_host_tid_to_link_map_params *params)
{
    struct wlan_t2lm_info *t2lm = NULL;
    uint8_t dir = 0;
    uint8_t tid = 0;

    params->pdev_id = peer_entry->link_peer->pdev_id;
    qdf_mem_copy(params->peer_macaddr, peer_entry->link_peer->macaddr,
            QDF_MAC_ADDR_SIZE);

    t2lm_debug("Fill T2LM WMI info for peer: " QDF_MAC_ADDR_FMT " pdev_id:%d",
            QDF_MAC_ADDR_REF(params->peer_macaddr), params->pdev_id);

    for (dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
        t2lm = &t2lm_negotiated_info->t2lm_info[dir];

        if (t2lm->direction == WLAN_T2LM_INVALID_DIRECTION)
            continue;

        params->t2lm_info[params->num_dir].direction = t2lm->direction;
        params->t2lm_info[params->num_dir].default_link_mapping =
            t2lm->default_link_mapping;


        if (params->t2lm_info[params->num_dir].default_link_mapping) {
            t2lm_debug("dir:%d default_link_map = 1", t2lm->direction);
            params->num_dir++;
            continue;
        }

        qdf_mem_copy(&params->t2lm_info[params->num_dir].t2lm_provisioned_links,
                &t2lm->hw_link_map_tid,
                sizeof(uint16_t) * T2LM_MAX_NUM_TIDS);
        t2lm_debug("num_dir:%d direction:%d default_link_mapping:%d",
                params->num_dir, params->t2lm_info[params->num_dir].direction,
                params->t2lm_info[params->num_dir].default_link_mapping);

        for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
            t2lm_debug("tid:%d hw_link_map:%x ieee_link_map:%x", tid,
                    params->t2lm_info[params->num_dir].t2lm_provisioned_links[tid],
                    t2lm->ieee_link_map_tid[tid]);
        }

        params->num_dir++;
    }

}

void ol_ath_fill_link_control_params(
        struct wlan_mlo_link_peer_entry *peer_entry,
        struct wlan_prev_t2lm_negotiated_info *t2lm_negotiated_info,
        struct wmi_host_tid_to_link_map_params *params)
{
    struct wlan_link_preference *link_preference = NULL;
    uint8_t pref_link = 0;
    uint8_t latency = 0;
    uint8_t links = 0;

    params->pdev_id = peer_entry->link_peer->pdev_id;
    qdf_mem_copy(params->peer_macaddr, peer_entry->link_peer->macaddr,
                 QDF_MAC_ADDR_SIZE);

    link_preference = &t2lm_negotiated_info->link_preference;
    params->preferred_links.num_pref_links = link_preference->num_pref_links;
    links = link_preference->num_pref_links;
    t2lm_debug("num_pref_links: %d", links);

    for (pref_link = 0; pref_link < links; pref_link++) {
        params->preferred_links.preffered_link_order[pref_link] =
                                link_preference->pref_order[pref_link];
        t2lm_debug("preffered_link_order[%d]: %d", pref_link,
                link_preference->pref_order[pref_link]);
    }

    for (latency = 0; latency < WIFI_AC_MAX; latency++) {
        params->preferred_links.timeout[latency] =
                                link_preference->timeout[latency];
        t2lm_debug("timeout[%d]: %d", latency,
                link_preference->timeout[latency]);
    }

    /* Currently setting the Preference Link enable bit for Link control params.
     * This will be modified to be updated using config
     *
     * Bit 0: Tx Link Tuple enable/disable
     * Bit 1: Preferred Link enable/disable
     */
    params->preferred_links.link_control_flags =
                        BIT(LINK_CONTROL_FLAG_PREFFERED_LINK_BIT);
    params->preferred_links.tlt_characterization_params =
                                link_preference->tlt_characterization_params;

}

QDF_STATUS ol_ath_send_mlo_peer_tid_to_link_map(
        struct wlan_mlo_peer_context *ml_peer, bool include_t2lm_params)
{
    struct wmi_unified *wmi_handle = NULL;
    struct wmi_host_tid_to_link_map_params params = {0};
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    uint8_t peer_cnt = 0;
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    for (peer_cnt = 0; peer_cnt < MAX_MLO_LINK_PEERS; peer_cnt++) {
        qdf_mem_zero(&params, sizeof(params));

        peer_entry = &ml_peer->peer_list[peer_cnt];

        if (peer_entry->link_peer) {
            vdev = wlan_peer_get_vdev(peer_entry->link_peer);
            if (!vdev) {
                t2lm_err("null vdev");
                return QDF_STATUS_E_NULL_VALUE;
            }

            wmi_handle = lmac_get_pdev_wmi_handle(wlan_vdev_get_pdev(vdev));
            if (!wmi_handle) {
                t2lm_err("null wmi handle");
                return QDF_STATUS_E_NULL_VALUE;
            }

            if (include_t2lm_params) {
                ol_ath_fill_t2lm_params(peer_entry,
                        &ml_peer->t2lm_policy.t2lm_negotiated_info, &params);
            }

            ol_ath_fill_link_control_params(peer_entry,
                    &ml_peer->t2lm_policy.t2lm_negotiated_info, &params);

            status = wmi_send_mlo_peer_tid_to_link_map_cmd(wmi_handle,
                                                           &params,
                                                           include_t2lm_params);
            if (QDF_IS_STATUS_ERROR(status)) {
                t2lm_err("Failed to send T2LM WMI command for pdev_id:%d peer_mac: " QDF_MAC_ADDR_FMT,
                        params.pdev_id,
                        QDF_MAC_ADDR_REF(params.peer_macaddr));
                return status;
            }
        }
    }

    return status;
}

void ol_ath_fill_established_t2lm_params(
        struct wlan_mlo_link_peer_entry *peer_entry,
        struct wlan_t2lm_info *t2lm,
        struct wmi_host_tid_to_link_map_params *params)
{
    uint8_t tid = 0;

    params->pdev_id = peer_entry->link_peer->pdev_id;
    qdf_mem_copy(params->peer_macaddr, peer_entry->link_peer->macaddr,
                 QDF_MAC_ADDR_SIZE);

    t2lm_debug("Fill T2LM WMI info for peer: " QDF_MAC_ADDR_FMT " pdev_id:%d",
               QDF_MAC_ADDR_REF(params->peer_macaddr), params->pdev_id);

    params->t2lm_info[params->num_dir].direction = t2lm->direction;
    params->t2lm_info[params->num_dir].default_link_mapping =
        t2lm->default_link_mapping;

    if (params->t2lm_info[params->num_dir].default_link_mapping) {
        t2lm_debug("dir:%d default_link_map = 1", t2lm->direction);
    } else {
        qdf_mem_copy(&params->t2lm_info[params->num_dir].t2lm_provisioned_links,
                     &t2lm->hw_link_map_tid,
                     sizeof(uint16_t) * T2LM_MAX_NUM_TIDS);

        t2lm_debug("num_dir:%d direction:%d default_link_mapping:%d",
                   params->num_dir,
                   params->t2lm_info[params->num_dir].direction,
                   params->t2lm_info[params->num_dir].default_link_mapping);

        for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
            t2lm_debug("tid:%d hw_link_map:%x ieee_link_map:%x", tid,
                       params->t2lm_info[params->num_dir].t2lm_provisioned_links[tid],
                       t2lm->ieee_link_map_tid[tid]);
        }
    }

    params->num_dir++;
}

void fill_assoc_t2lm_params(struct ieee80211_node *ni,
        struct peer_assoc_params *param)
{
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_peer *peer = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    uint8_t t2lm_supported = 0;
    uint8_t i =0;

    peer = ni->peer_obj;
    if(!peer)
        return;

    if (!wlan_peer_is_mlo(peer)) {
        return;
    }

    ml_peer = peer->mlo_peer_ctx;
    if (!ml_peer) {
        t2lm_err("ML Peer is NULL");
        return;
    }

    ol_ath_get_t2lm_enabled(ni->ni_ic, &t2lm_supported);

    if (!t2lm_supported ||
        ((ml_peer->t2lm_policy.t2lm_enable_val != WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) &&
         (ml_peer->t2lm_policy.t2lm_enable_val != WLAN_T2LM_MAP_EACH_TID_TO_SAME_OR_DIFFERENET_LINK_SET))) {
        t2lm_info("peer_mac " QDF_MAC_ADDR_FMT " does not support T2LM",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes));
        return;
    }

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        if (!peer_entry->link_peer)
            continue;

        if (peer_entry->link_peer == peer) {
            ol_ath_fill_t2lm_params(peer_entry,
                    &ml_peer->t2lm_policy.t2lm_negotiated_info,
                    &param->t2lm_params);
            ol_ath_fill_link_control_params(peer_entry,
                    &ml_peer->t2lm_policy.t2lm_negotiated_info,
                    &param->t2lm_params);
            break;
        }
    }
}

QDF_STATUS ol_ath_send_mlo_peer_established_mapping(
        struct wlan_objmgr_vdev *vdev, struct wlan_mlo_peer_context *ml_peer)
{
    struct wlan_t2lm_info *t2lm;
    struct wmi_unified *wmi_handle = NULL;
    struct wmi_host_tid_to_link_map_params params = {0};
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_vdev *tmp_vdev = NULL;
    uint8_t peer_cnt = 0;
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    t2lm = &vdev->mlo_dev_ctx->t2lm_ctx.established_t2lm.t2lm;

    mlo_peer_lock_acquire(ml_peer);
    for (peer_cnt = 0; peer_cnt < MAX_MLO_LINK_PEERS; peer_cnt++) {
        qdf_mem_zero(&params, sizeof(params));

        peer_entry = &ml_peer->peer_list[peer_cnt];
        if (!peer_entry->link_peer) {
            continue;
        }

        tmp_vdev = wlan_peer_get_vdev(peer_entry->link_peer);
        if (!tmp_vdev) {
            t2lm_err("null tmp_vdev");
            status = QDF_STATUS_E_NULL_VALUE;
            goto exit;
        }

        wmi_handle = lmac_get_pdev_wmi_handle(wlan_vdev_get_pdev(tmp_vdev));
        if (!wmi_handle) {
            t2lm_err("null wmi handle");
            status = QDF_STATUS_E_NULL_VALUE;
            goto exit;
        }

        ol_ath_fill_established_t2lm_params(peer_entry, t2lm, &params);

        status = wmi_send_mlo_peer_tid_to_link_map_cmd(wmi_handle, &params, true);
        if (QDF_IS_STATUS_ERROR(status)) {
            t2lm_err("Failed to send T2LM WMI command for pdev_id:%d peer_mac: " QDF_MAC_ADDR_FMT,
                     params.pdev_id,
                     QDF_MAC_ADDR_REF(params.peer_macaddr));
            goto exit;
        }
    }

exit:
    mlo_peer_lock_release(ml_peer);
    return status;
}

#else
void fill_assoc_t2lm_params(struct ieee80211_node *ni,
        struct peer_assoc_params *param)
{
    return;
}

void ol_ath_send_mlo_peer_established_mapping(
        struct wlan_objmgr_vdev *vdev, struct wlan_mlo_peer_context *ml_peer)
{
    return;
}

#endif /* WLAN_FEATURE_11BE */

static uint32_t ol_get_wpa_wmi_auth_type(int32_t akm)
{
    /* Try the more preferred ones first. */
    if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X))
        return WMI_HOST_WPA;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK))
        return WMI_HOST_WPA_PSK;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_CCKM))
        return WMI_HOST_CCKM_WPA;
    else
        return WMI_HOST_NO_AUTH;
}

static uint32_t ol_get_wapi_wmi_auth_type(int32_t akm)
{
    /* Try the more preferred ones first. */
    if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_WAPI_CERT))
        return WMI_HOST_WAPI;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_WAPI_PSK))
        return WMI_HOST_WAPI_PSK;
    else
        return WMI_HOST_NO_AUTH;
}

uint16_t ol_get_rsn_wmi_auth_type(uint32_t akm)
{
    if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA384))
        return WMI_HOST_FT_RSNA_FILS_SHA384;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_FILS_SHA256))
        return WMI_HOST_FT_RSNA_FILS_SHA256;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FILS_SHA384))
        return WMI_HOST_RSNA_FILS_SHA384;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FILS_SHA256))
        return WMI_HOST_RSNA_FILS_SHA256;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_SAE_EXT_KEY))
        return WMI_HOST_FT_RSNA_SAE_SHA384;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_SAE_EXT_KEY))
        return WMI_HOST_WPA3_SAE_SHA384;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_SAE))
        return WMI_HOST_FT_RSNA_SAE;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_SAE))
        return WMI_HOST_WPA3_SAE;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_OWE))
        return WMI_HOST_WPA3_OWE;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X))
        return WMI_HOST_FT_RSNA;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_PSK))
        return WMI_HOST_FT_RSNA_PSK;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X))
        return WMI_HOST_RSNA;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK))
        return WMI_HOST_RSNA_PSK;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_CCKM))
        return WMI_HOST_CCKM_RSNA;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_PSK_SHA256))
        return WMI_HOST_RSNA_PSK_SHA256;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SHA256))
        return WMI_HOST_RSNA_8021X_SHA256;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B))
        return WMI_HOST_RSNA_SUITE_B_8021X_SHA256;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_IEEE8021X_SUITE_B_192))
        return WMI_HOST_RSNA_SUITE_B_8021X_SHA384;
    else if (QDF_HAS_PARAM(akm, WLAN_CRYPTO_KEY_MGMT_FT_IEEE8021X_SHA384))
        return WMI_HOST_FT_RSNA_SUITE_B_8021X_SHA384;
    else
        return WMI_HOST_NO_AUTH;
}

/*
 * APT to convert WLAN_CRYPTO_KEY_MGMT* to WMI_AUTH*
 */
uint32_t ol_crypto_authmode_to_wmi_authmode(int32_t authmodeset,
                                            int32_t akm, int32_t ucastcipherset)
{
    if (!authmodeset || authmodeset < 0)
        return WMI_HOST_OPEN;

    if (QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_OPEN))
        return WMI_HOST_OPEN;

    if (QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_AUTO) ||
            QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_NONE)) {
                if ((QDF_HAS_PARAM(ucastcipherset, WLAN_CRYPTO_CIPHER_WEP) ||
                     QDF_HAS_PARAM(ucastcipherset, WLAN_CRYPTO_CIPHER_WEP_40) ||
                     QDF_HAS_PARAM(ucastcipherset,
                                   WLAN_CRYPTO_CIPHER_WEP_104) ||
                     QDF_HAS_PARAM(ucastcipherset, WLAN_CRYPTO_CIPHER_TKIP) ||
                     QDF_HAS_PARAM(ucastcipherset,
                                   WLAN_CRYPTO_CIPHER_AES_GCM) ||
                     QDF_HAS_PARAM(ucastcipherset,
                                   WLAN_CRYPTO_CIPHER_AES_GCM_256) ||
                     QDF_HAS_PARAM(ucastcipherset,
                                   WLAN_CRYPTO_CIPHER_AES_CCM) ||
                                   QDF_HAS_PARAM(ucastcipherset,
                                   WLAN_CRYPTO_CIPHER_AES_OCB) ||
                     QDF_HAS_PARAM(ucastcipherset,
                                   WLAN_CRYPTO_CIPHER_AES_CCM_256)))
                         return WMI_HOST_OPEN;
                else
                         return WMI_HOST_NO_AUTH;
    }

    if (QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_SHARED))
        return WMI_HOST_SHARED;

    if (QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_8021X) ||
        QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_RSNA) ||
        QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_CCKM) ||
        QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_SAE) ||
        QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_FILS_SK))
            return ol_get_rsn_wmi_auth_type(akm);

     if (QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_WPA))
         return ol_get_wpa_wmi_auth_type(akm);

     if (QDF_HAS_PARAM(authmodeset, WLAN_CRYPTO_AUTH_WAPI))
         return ol_get_wapi_wmi_auth_type(akm);

     return WMI_HOST_OPEN;
}
/*
 *  WMI API for 802.11 management frame processing
 */
int ol_ath_send_peer_assoc(struct ieee80211_node *ni, int isnew)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct peer_assoc_params param;
    uint8_t rx_nss160 = 0, rx_nss80p80 = 0;
    bool same_nss_for_all_bw = false;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wmi_unified *pdev_wmi_handle;
    int32_t authmode, ucastcipher, akm;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    QDF_STATUS status;

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_peer_is_assoc_rejected(peer)) {
        qdf_err("Skip assoc cmd as peer is rejected " QDF_MAC_ADDR_FMT,
                QDF_MAC_ADDR_REF(ni->ni_macaddr));
        return 0;
    }
#endif

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return -EINVAL;

    qdf_mem_set(&param, sizeof(param), 0);

    soc_txrx_handle =
                wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(ic->ic_pdev_obj));

    IEEE80211_ADDR_COPY(param.peer_mac, ni->ni_macaddr);
    param.vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    param.peer_new_assoc = isnew;
    param.peer_associd = IEEE80211_AID(ni->ni_associd);
    param.peer_bw_rxnss_override = 0;
    vap->iv_sta_negotiated_ch_width = ni->ni_chwidth;

    if(ieee80211_is_pmf_enabled(vap, ni)) {
        param.is_pmf_enabled = TRUE;
    }

    /*
     * Do not enable HT/VHT if WMM/wme is disabled for vap.
     */
    if (ieee80211_vap_wme_is_set(vap)) {
        param.is_wme_set = TRUE;

        if ((ni->ni_flags & IEEE80211_NODE_QOS) || (ni->ni_flags & IEEE80211_NODE_HT) ||
                (ni->ni_flags & IEEE80211_NODE_VHT) || (ni->ni_ext_flags & IEEE80211_NODE_HE)) {
            param.qos_flag = TRUE;
        }
        if (ni->ni_flags & IEEE80211_NODE_UAPSD ) {
            param.apsd_flag = TRUE;
        }
        if (ni->ni_flags & IEEE80211_NODE_HT) {
            param.ht_flag = TRUE;
        }
        if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40) || (ni->ni_chwidth == IEEE80211_CWM_WIDTH80) ||
                (ni->ni_chwidth == IEEE80211_CWM_WIDTH160)){
            param.bw_40 = TRUE;
        }
        if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH80) || (ni->ni_chwidth == IEEE80211_CWM_WIDTH160)) {
            param.bw_80 = TRUE;
        }
        if (ni->ni_chwidth == IEEE80211_CWM_WIDTH160) {
            param.bw_160 = TRUE;
        }
#ifdef WLAN_FEATURE_11BE
        if (ni->ni_chwidth == IEEE80211_CWM_WIDTH320) {
            param.bw_40 = TRUE;
            param.bw_80 = TRUE;
            param.bw_160 = TRUE;
            param.bw_320 = TRUE;
        }
#endif
        /* Typically if STBC is enabled for VHT it should be enabled for HT as well */
        if ((ni->ni_htcap & IEEE80211_HTCAP_C_RXSTBC) && (ni->ni_vhtcap & IEEE80211_VHTCAP_RX_STBC)) {
            param.stbc_flag = TRUE;
        }

        /* Typically if LDPC is enabled for VHT it should be enabled for HT as well */
        if ((ni->ni_htcap & IEEE80211_HTCAP_C_ADVCODING) && (ni->ni_vhtcap & IEEE80211_VHTCAP_RX_LDPC)) {
            param.ldpc_flag = TRUE;
        }

        if ((ni->ni_htcap & IEEE80211_HTCAP_C_SM_MASK) == IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC) {
            param.static_mimops_flag = TRUE;
        }
        if ((ni->ni_htcap & IEEE80211_HTCAP_C_SM_MASK) == IEEE80211_HTCAP_C_SMPOWERSAVE_DYNAMIC) {
            param.dynamic_mimops_flag = TRUE;
        }
        if ((ni->ni_htcap & IEEE80211_HTCAP_C_SM_MASK) == IEEE80211_HTCAP_C_SMPOWERSAVE_DISABLED) {
            param.spatial_mux_flag = TRUE;
        }
        if (ni->ni_flags & IEEE80211_NODE_VHT) {
            param.vht_flag = TRUE;
        }
        if ((ni->ni_flags & IEEE80211_NODE_VHT) &&
             (ieee80211_vap_256qam_is_set(ni->ni_vap)) &&
              IEEE80211_IS_CHAN_11NG(ic->ic_curchan)) {
            param.vht_ng_flag = TRUE;
        }
        if (ni->ni_ext_flags & IEEE80211_NODE_HE) {
            param.he_flag = TRUE;
        }
    }
    if (ni->ni_ext_flags & IEEE80211_NODE_TWT_REQUESTER)
        param.twt_requester = true;

    if (ni->ni_ext_flags & IEEE80211_NODE_TWT_RESPONDER)
        param.twt_responder = true;

    /*
     * Suppress authorization for all AUTH modes that need 4-way handshake (during re-association).
     * Authorization will be done for these modes on key installation.
     */
    if(!isnew && ieee80211_node_is_authorized(ni) ) {
        param.auth_flag = TRUE;
    }

    if ((akm = wlan_crypto_get_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_KEY_MGMT)) == -1) {
       qdf_err("crypto_err while getting akm\n");
       return -1;
    }

    authmode = wlan_crypto_get_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
    if ( authmode == -1 ) {
        qdf_err("crypto_err while getting authmode params\n");
        return -1;
    }
    if ((ucastcipher = wlan_crypto_get_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER)) == -1 ) {
        qdf_err("crypto_err while getting ucastcipher\n");
        return -1;
    }
    param.akm = ol_crypto_authmode_to_wmi_authmode(authmode, akm, ucastcipher);

    if (authmode & ((uint32_t)((1 << WLAN_CRYPTO_AUTH_WPA)
                             | (1 << WLAN_CRYPTO_AUTH_RSNA)
                             | (1 << WLAN_CRYPTO_AUTH_WAPI)))) {
        /*
         *  In WHCK NDIS Test, 4-way handshake is not mandatory in WPA TKIP/CCMP mode.
         *  Check the SSID, if the SSID is NDIS softAP ssid, the function will unset WMI_PEER_NEED_PTK_4_WAY flag.
         *  This will bypass the check and set the ALLOW_DATA in fw to let the data packet sent out.
         */
        if (OS_MEMCMP(ni->ni_essid, NDIS_SOFTAP_SSID, NDIS_SOFTAP_SSID_LEN) == 0 || ni->is_ft_reauth)
            param.need_ptk_4_way = FALSE;
        else
            param.need_ptk_4_way = TRUE;
    }

    if(authmode & ((uint32_t)(1 << WLAN_CRYPTO_AUTH_WPA))){
        param.need_gtk_2_way = TRUE;
    }
    /* safe mode bypass the 4-way handshake */
    if (IEEE80211_VAP_IS_SAFEMODE_ENABLED(ni->ni_vap)) {
        param.safe_mode_enabled = TRUE;
    }
      /* Disable AMSDU for station transmit, if user configures it */
    if ((vap->iv_opmode == IEEE80211_M_STA) && (ic->ic_sta_vap_amsdu_disable) &&
        !(ni->ni_flags & IEEE80211_NODE_VHT)) {
        param.amsdu_disable = TRUE;
    }
      /* Disable AMSDU for AP transmit to 11n Stations, if user configures it */
    if ((vap->iv_opmode == IEEE80211_M_HOSTAP) && (vap->iv_disable_ht_tx_amsdu) &&
        (ni->ni_flags & IEEE80211_NODE_HT) && (!(IEEE80211_NODE_USE_VHT(ni))) ) {
        param.amsdu_disable = TRUE;
    }
     /* enable INTER BSS PEER  flag for nawds peer */
    if (ni->ni_flags & IEEE80211_NODE_NAWDS) {
        param.inter_bss_peer = TRUE;
    }

    if(IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
        qdf_mem_copy(&param.peer_he_caps_6ghz, &(ni->ni_he.he6g_bandcap),
                                            sizeof(ni->ni_he.he6g_bandcap));
    }

    param.peer_caps = ni->ni_capinfo;
    param.peer_listen_intval = ni->ni_lintval;
    param.peer_ht_caps = ni->ni_htcap;
    if (!param.ht_flag && ni->ni_htcap) {
        qdf_err("Node %s does not support HT but htcaps are populated\n",
                ether_sprintf(ni->ni_macaddr));
        qdf_err("Node Flags: 0x%x , ht_caps: 0x%x \n", ni->ni_flags, ni->ni_htcap);
        return -EINVAL;
    }

    param.peer_max_mpdu = ni->ni_maxampdu;
    param.peer_mpdu_density = ni->ni_mpdudensity;
    param.peer_vht_caps = ni->ni_vhtcap;
    if (!param.vht_flag && ni->ni_vhtcap) {
        qdf_err("Node %s does not support VHT but vhtcaps are populated\n",
                ether_sprintf(ni->ni_macaddr));
        qdf_err("Node Flags: 0x%x , vht_caps: 0x%x \n", ni->ni_flags, ni->ni_vhtcap);
        return -EINVAL;
    }
    param.min_data_rate = ni->ni_minimumrate;
#if WAR_DISABLE_MU_2x2_STA
    /* WAR to disable MU-MIMO for 2x2 STAs */
    if (((ni->ni_vhtcap & IEEE80211_VHTCAP_SOUND_DIM) >> IEEE80211_VHTCAP_SOUND_DIM_S) == 1) {
        param.peer_vht_caps &= ~IEEE80211_VHTCAP_SOUND_DIM;
        param.peer_vht_caps &= ~IEEE80211_VHTCAP_MU_BFORMEE;
    }
#endif

    /* Update peer rate information */
    param.peer_rate_caps = ol_ath_net80211_rate_node_update(ic, ni, isnew);
    param.peer_legacy_rates.num_rates = ni->ni_rates.rs_nrates;
    /* NOTE: cmd->peer_legacy_rates.rates is of type A_UINT32 */
    /* ni->ni_rates.rs_rates is of type u_int8_t */
    /**
     * for cmd->peer_legacy_rates.rates:
     * rates (each 8bit value) packed into a 32 bit word.
     * the rates are filled from least significant byte to most
     * significant byte.
     */
    OS_MEMCPY( param.peer_legacy_rates.rates, ni->ni_rates.rs_rates, ni->ni_rates.rs_nrates);

    param.peer_ht_rates.num_rates = ni->ni_htrates.rs_nrates;
    OS_MEMCPY( param.peer_ht_rates.rates, ni->ni_htrates.rs_rates, ni->ni_htrates.rs_nrates);

    param.peer_nss = (ni->ni_streams==0)?1:ni->ni_streams;

    /* set the default vht max rate info */
    if (ni->ni_vhtcap) {
        if((ni->ni_maxrate_vht != 0xff) && (!isnew)) {
	        u_int8_t user_max_nss = (ni->ni_maxrate_vht >> VHT_MAXRATE_IDX_SHIFT) & 0XF;
            param.peer_nss = (param.peer_nss > user_max_nss) ? user_max_nss : param.peer_nss;
        } else {
            ni->ni_maxrate_vht = 0xff;
        }
    } else {
        ni->ni_maxrate_vht = 0;
    }

    if (ni->ni_vhtcap) {
        param.vht_capable = TRUE;
        param.rx_max_rate = ni->ni_rx_max_rate;
        param.rx_mcs_set = ni->ni_rx_vhtrates;
        param.tx_max_rate = ni->ni_tx_max_rate;
        param.tx_mcs_set = ni->ni_tx_vhtrates;
        /* Update Peer support for VHT MCS10/11 */
        if(ic->ic_he_target) {
            param.tx_mcs_set |= IEEE80211_VHT_MCS10_11_SUPP;
            if(ni->ni_higher_vhtmcs_supp) {
                param.tx_mcs_set |= ((ni->ni_higher_vhtmcs_supp <<
                                        IEEE80211_VHT_HIGHER_MCS_S) &
                                        IEEE80211_VHT_HIGHER_MCS_MAP);
            }
            else if(ni->ni_vap->iv_vht_mcs10_11_nq2q_peer_supp &&
                                ni->ni_vap->iv_vht_mcs10_11_supp) {
                param.tx_mcs_set |= ((ic->ic_vhtcap_max_mcs.tx_mcs_set.higher_mcs_supp <<
                                        IEEE80211_VHT_HIGHER_MCS_S) &
                                        IEEE80211_VHT_HIGHER_MCS_MAP);
            }
        }
        param.tx_max_mcs_nss = ni->ni_maxrate_vht;
    }

    /* In very exceptional  conditions it is observed  that
     * firmware was receiving phymode as 0 for peer from host, and resulting in Target Assert
     * Changing the phymode to desired mode
     */
    if(ni->ni_phymode == 0) {
        ni->ni_phymode = vap->iv_des_mode;
    }
#if SUPPORT_11AX_D3
    param.peer_he_ops = (ni->ni_he.heop_param |
             (ni->ni_he.heop_bsscolor_info << HEOP_PARAM_S));
#else
    param.peer_he_ops = ni->ni_he.heop_param;
#endif
    OL_IF_MSG_COPY_CHAR_ARRAY(&param.peer_he_cap_macinfo,
                              &(ni->ni_he.hecap_macinfo),
                              qdf_min(sizeof(param.peer_he_cap_macinfo),
                              sizeof(ni->ni_he.hecap_macinfo)));
    qdf_mem_copy(&param.peer_he_cap_phyinfo, &(ni->ni_he.hecap_phyinfo),
                             sizeof(param.peer_he_cap_phyinfo));
    qdf_mem_copy(&param.peer_ppet, &(ni->ni_he.hecap_ppet),
                             sizeof(param.peer_ppet));

    /* WAR to disable dl mu for NAWDS client */
    if (ni->ni_flags & IEEE80211_NODE_NAWDS) {
        ni->ni_he.hecap_info_internal |=
        IEEE80211_HE_DL_MU_SUPPORT_DISABLE << IEEE80211_HE_DL_OFDMA_SUPP_S;
        ni->ni_he.hecap_info_internal |=
        IEEE80211_HE_DL_MU_SUPPORT_DISABLE << IEEE80211_HE_DL_MUMIMO_SUPP_S;
    }
    param.peer_he_cap_info_internal = ni->ni_he.hecap_info_internal;

    if (IEEE80211_IS_HECAP_MACINFO(ni->ni_he.hecap_macinfo)) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s HE MAC Capabilities info: All bits are 0", __func__);
    }

    /*11AX TODO (Phase II) - Add further checks for population NSS & MCS */
    if (ieee80211_is_phymode_11ax(ni->ni_phymode)
#ifdef WLAN_FEATURE_11BE
            || ieee80211_is_phymode_11be(ni->ni_phymode)
#endif
            ) {
        uint32_t *peer_he_rx_mcs_set = param.peer_he_rx_mcs_set;
        uint32_t *peer_he_tx_mcs_set = param.peer_he_tx_mcs_set;
        uint16_t *ni_he_rx_mcs_set = ni->ni_he.hecap_rxmcsnssmap;
        uint16_t *ni_he_tx_mcs_set = ni->ni_he.hecap_txmcsnssmap;
        int mode_idx;

        param.peer_he_mcs_count = 0;

        if (ni->ni_he_width_set_org & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE80_80) {
            mode_idx = HECAP_TXRX_MCS_NSS_IDX_80_80;
            peer_he_rx_mcs_set[mode_idx] = ni_he_rx_mcs_set[mode_idx];
            peer_he_tx_mcs_set[mode_idx] = ni_he_tx_mcs_set[mode_idx];

            if(ni->ni_tx_higher_hemcs_supp)
                peer_he_tx_mcs_set[mode_idx] |=
                    ((ni->ni_tx_higher_hemcs_supp << IEEE80211_HE_HIGHER_MCS_G80) &
                     IEEE80211_HE_HIGHER_MCS_MAP);

            if(ni->ni_rx_higher_hemcs_supp)
                peer_he_rx_mcs_set[mode_idx] |=
                    ((ni->ni_rx_higher_hemcs_supp << IEEE80211_HE_HIGHER_MCS_G80) &
                     IEEE80211_HE_HIGHER_MCS_MAP);

            param.peer_he_mcs_count++;
        }

        if ((ni->ni_he_width_set_org & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE160)
            || (ni->ni_he_width_set_org & IEEE80211_HECAP_PHY_CHWIDTH_11AXA_HE80_80)) {
            mode_idx = HECAP_TXRX_MCS_NSS_IDX_160;
            peer_he_rx_mcs_set[mode_idx] = ni_he_rx_mcs_set[mode_idx];
            peer_he_tx_mcs_set[mode_idx] = ni_he_tx_mcs_set[mode_idx];

            if(ni->ni_tx_higher_hemcs_supp)
                peer_he_tx_mcs_set[mode_idx] |=
                    ((ni->ni_tx_higher_hemcs_supp << IEEE80211_HE_HIGHER_MCS_G80) &
                     IEEE80211_HE_HIGHER_MCS_MAP);

            if(ni->ni_rx_higher_hemcs_supp)
                peer_he_rx_mcs_set[mode_idx] |=
                    ((ni->ni_rx_higher_hemcs_supp << IEEE80211_HE_HIGHER_MCS_G80) &
                     IEEE80211_HE_HIGHER_MCS_MAP);

            param.peer_he_mcs_count++;
       }

       /* -------- Populate Rx/Tx MCS values for 20/40/80 BWs  ------------- */

       mode_idx = HECAP_TXRX_MCS_NSS_IDX_80;
       peer_he_rx_mcs_set[mode_idx] = ni_he_rx_mcs_set[mode_idx];
       peer_he_tx_mcs_set[mode_idx] = ni_he_tx_mcs_set[mode_idx];

       if(ni->ni_tx_higher_hemcs_supp)
           peer_he_tx_mcs_set[mode_idx] |=
               ((ni->ni_tx_higher_hemcs_supp << IEEE80211_HE_HIGHER_MCS_L80) &
                IEEE80211_HE_HIGHER_MCS_MAP);

       if(ni->ni_rx_higher_hemcs_supp)
           peer_he_rx_mcs_set[mode_idx] |=
               ((ni->ni_rx_higher_hemcs_supp << IEEE80211_HE_HIGHER_MCS_L80) &
                IEEE80211_HE_HIGHER_MCS_MAP);

       param.peer_he_mcs_count++;
    }
    param.peer_bss_max_idle_option = ni->wnm_bss_idle_option;

    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s: Client %02x:%02x:%02x:%02x:%02x:%02x, vdev %d "
            "VHT Caps=%x HT Caps=%x RX MCS=%x TX MCS=%x \n"
            "stbc:%d, ldpc:%d, ht_flag:%d, vht_flag:%d, vht_ng_flag:%d, he_flag:%d\n"
            "bw_40:%d, bw_80:%d, bw_160:%d\n"
            "peer_nss: %d",
            __func__, param.peer_mac[0], param.peer_mac[1],
            param.peer_mac[2], param.peer_mac[3], param.peer_mac[4],
            param.peer_mac[5], param.vdev_id,
            param.peer_vht_caps, param.peer_ht_caps,
            param.rx_mcs_set ,param.tx_mcs_set,
            param.stbc_flag, param.ldpc_flag, param.ht_flag, param.vht_flag,
            param.vht_ng_flag, param.he_flag, param.bw_40, param.bw_80, param.bw_160, param.peer_nss);

    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s 11ax Param OPS=%x  MACInfo[0]=%x"
            " MACInfo[1]=%x MCS_cnt=%x"
            " TX_MCS[80]=%x TX_MCS[160]=%x TX_MCS[80_80]=%x"
            " RX_MCS[80]=%x RX_MCS[160]=%x RX_MCS[80_80]=%x"
            " Phyinfo[0]=%x Phyinfo[1]=%x Phyinfo[2]=%x",
            __func__, param.peer_he_ops,
            param.peer_he_cap_macinfo[0],
            param.peer_he_cap_macinfo[1], param.peer_he_mcs_count,
            param.peer_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80],
            param.peer_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_160],
            param.peer_he_tx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80_80],
            param.peer_he_rx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80],
            param.peer_he_rx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_160],
            param.peer_he_rx_mcs_set[HECAP_TXRX_MCS_NSS_IDX_80_80],
            param.peer_he_cap_phyinfo[IC_HECAP_PHYDWORD_IDX0],
            param.peer_he_cap_phyinfo[IC_HECAP_PHYDWORD_IDX1],
            param.peer_he_cap_phyinfo[IC_HECAP_PHYDWORD_IDX2]);

    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s 6GHz Caps: %x Q2Q PHY Feature Support=%x"
            " Peer NSS = %d\n Max BSS Idle Option: Protected flag= %d",
            __func__, param.peer_he_caps_6ghz, param.peer_he_cap_info_internal,
            param.peer_nss, param.peer_bss_max_idle_option);

    param.peer_phymode = ol_map_phymode_to_wmimode(ni);
#if QCN_IE
    param.peer_bsscolor_rept_info = ni->ni_bsscolor_rept_info;
#endif

    fill_assoc_11be_params(ni, &param);

    fill_assoc_mlo_params(ni, &param);

    if (isnew) {
        if (send_dp_peer_setup(ni) != QDF_STATUS_SUCCESS) {
            qdf_err("vap:%u  [%s] Error, DP setup failed ni " QDF_MAC_ADDR_FMT,
                    vap->iv_unit, vap->iv_netdev_name, &ni->ni_macaddr);
            return -1;
        }
    }

    if (peer->obj_state == WLAN_OBJ_STATE_LOGICALLY_DELETED) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                "%s: Error, STA is in logically deleted state, return",
                __func__);
        return -1;
    }

    fill_assoc_t2lm_params(ni, &param);

    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s ni phymode=%d peer phymode=%d",
            __func__, ni->ni_phymode, param.peer_phymode);

    /* Send bandwidth-NSS mapping to FW if we are 160/80+80 MHz capable.
     *
     * Note: At this point, the exact advertisement mechanisms and the degree
     * of flexibility applicable for 802.11ax BW-NSS mapping have not settled.
     * For now, we provision for HE160/80+80 capable chipsets to be able to
     * use host-FW exchange for BW-NSS mapping if required.
     * 11AX TODO: Modify below provision and above comment based on updates in
     * the standard.
     */

    if (ic->ic_modecaps &
            ((1 << IEEE80211_MODE_11AC_VHT160) |
             (1 << IEEE80211_MODE_11AC_VHT80_80) |
             (1 << IEEE80211_MODE_11AXA_HE160) |
             (1ULL << IEEE80211_MODE_11AXA_HE80_80)
#ifdef WLAN_FEATURE_11BE
             | (1ULL << IEEE80211_MODE_11BEA_EHT160)
             | (1ULL << IEEE80211_MODE_11BEA_EHT320)
#endif /* WLAN_FEATURE_11BE */
            )) {
        if (ni->ni_prop_ie_used || ni->ni_ext_nss_support) {
            rx_nss160 = ni->ni_bw160_nss;
            rx_nss80p80 = ni->ni_bw80p80_nss;
        } else if ((param.peer_phymode == WMI_HOST_MODE_11AC_VHT160) || (param.peer_phymode == WMI_HOST_MODE_11AC_VHT80_80)) {
            if (param.peer_phymode == WMI_HOST_MODE_11AC_VHT80_80) {
                rx_nss80p80 = param.peer_nss;
            }
            rx_nss160 = param.peer_nss;
            same_nss_for_all_bw = true;
        }

        if (!ic->ic_fw_ext_nss_capable) {
            if (rx_nss160) {
                param.peer_bw_rxnss_override = IEEE80211_BW_NSS_FWCONF_160(rx_nss160);
            }
        } else {
            /* Irrespective of whether vap supports EXT NSS or not populate into FW in same manner
             * since rx_nss values are to be populated appropriately into thge local variables by
             * this point
             */
            if (rx_nss160) {
                param.peer_bw_rxnss_override = IEEE80211_BW_NSS_FWCONF_160(rx_nss160);
            }
            if (rx_nss80p80) {
                param.peer_bw_rxnss_override |= IEEE80211_BW_NSS_FWCONF_80_80(rx_nss80p80);
            }
        }

        /* In very exceptional  conditions it is observed  that
         * firmware was receiving bw_rxnss_override as 0 for peer from host, and resulting in Target Assert.
         * Changing the rxnss_override to minimum nss. This is a temporary WAR. Needs to be fixed
         * properly.
         */
        if (((param.peer_phymode == WMI_HOST_MODE_11AC_VHT160) || (param.peer_phymode == WMI_HOST_MODE_11AC_VHT80_80))
               && (!param.peer_bw_rxnss_override) && !(same_nss_for_all_bw)) {
            param.peer_bw_rxnss_override = IEEE80211_BW_NSS_FWCONF_160(1);
        }
    }

    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s sending wmi peer assoc, isnew:%d",
            __func__, isnew);

#ifdef WLAN_FEATURE_11BE_MLO
    param.qcn_node_flag = (ni->peer_obj->peer_flags & WLAN_PEER_QCN_NODE) ? true : false;
    param.mesh_node_flag = (ni->peer_obj->peer_flags & WLAN_PEER_MESH_NODE) ? true : false;
    /*
     * wlan_peer_mesh_override_flag overrides peer flags, w.r.t the value
     * assigned to the flag:
     * 1. (ENABLE - war) Always set the QCN & MESH flags to true
     * 2. (DISABLE - war) Always set the QCN & MESH flags to 0
     */
    if (wlan_peer_is_mlo(ni->peer_obj)) {
        if (vap->wlan_peer_mesh_override_flag == WLAN_PEER_MESH_NODE_OVERRIDE_ENABLE) {
            param.qcn_node_flag = param.mesh_node_flag = true;
        } else if (vap->wlan_peer_mesh_override_flag ==
                        WLAN_PEER_MESH_NODE_OVERRIDE_DISABLE) {
            param.qcn_node_flag = param.mesh_node_flag = false;
        }
    }
    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
           "%s peer assoc send wlan_peer_mesh_override_flag %d params: qcn %d mesh %d",
           __func__, vap->wlan_peer_mesh_override_flag, param.qcn_node_flag,
           param.mesh_node_flag);
    if (wlan_peer_is_mlo(peer) &&
        (wlan_mlo_peer_is_disconnect_progress(
                      peer->mlo_peer_ctx) == QDF_STATUS_SUCCESS)) {
        qdf_err("vap:%u  [%s] ML peer delete is initiated, skip peer assoc " QDF_MAC_ADDR_FMT,
                vap->iv_unit, vap->iv_netdev_name, &ni->ni_macaddr);
        return -1;
    }
#endif
    if (ni->peer_fw_create_fail) {
        qdf_err("vap:%u  [%s] peer create response failed, skip peer assoc " QDF_MAC_ADDR_FMT,
                vap->iv_unit, vap->iv_netdev_name, &ni->ni_macaddr);
        return -1;
    }
    status = wmi_unified_peer_assoc_send(pdev_wmi_handle, &param);
#ifdef WLAN_FEATURE_11BE_MLO
    if (QDF_IS_STATUS_SUCCESS(status)) {
        /* Set flag to indicate peer assoc is sent */
        wlan_mlo_link_peer_assoc_set(peer, true);
    }
#endif
    return qdf_status_to_os_return(status);
}

/**
 * ol_ath_net80211_addba_clearresponse() - Send addba clearresponse cmd
 * @ni: node information
 *
 * Return: none
 */
static void ol_ath_net80211_addba_clearresponse(struct ieee80211_node *ni)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211vap *vap = ni->ni_vap;
    struct addba_clearresponse_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.vdev_id = wlan_vdev_get_id(vap->vdev_obj);

    wmi_unified_addba_clearresponse_cmd_send(pdev_wmi_handle,
                                             ni->ni_macaddr, &param);
}

/**
 * ol_ath_net80211_addba_send() - Send addba command request
 * @ni: node information
 * @tidno: tid no
 * @buffer_size: buffer size
 *
 * Return: 0 on success, other value on failure
 */
static int ol_ath_net80211_addba_send(struct ieee80211_node *ni, uint8_t tidno,
                                      uint16_t buffersize)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211vap *vap = ni->ni_vap;
    struct addba_send_params param;
    struct wmi_unified *pdev_wmi_handle;
    QDF_STATUS status;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle)
        return -EINVAL;

    qdf_mem_set(&param, sizeof(param), 0);
    param.vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    param.tidno = tidno;
    param.buffersize = buffersize;

    /* Send the management frame buffer to the target */
    status = wmi_unified_addba_send_cmd_send(pdev_wmi_handle, ni->ni_macaddr,
                                             &param);
    return qdf_status_to_os_return(status);
}

/**
 * ol_ath_net80211_delba_send() - Send delba command request
 * @ni: node information
 * @tidno: tid no
 * @initiator: initiator
 * @reasoncode: reason code
 *
 * Return: none
 */
static void ol_ath_net80211_delba_send(struct ieee80211_node *ni, uint8_t tidno,
                                       uint8_t initiator, uint16_t reasoncode)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct delba_send_params param;
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211_action_mgt_args actionargs;
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_delba_parameterset delbaparams;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(ic->ic_pdev_obj);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    param.tidno = tidno;
    param.initiator = initiator;
    param.reasoncode = reasoncode;

    if (ic->ic_he_target && initiator == 0) {
        /* In case the BA Session was established by
         * remote,
         *
         * 1. send a delba frame to the remote
         * 2. clear the data structures updated
         *    during the receipt of  add_ba_req
         */
        delbaparams.tid       = tidno;
        delbaparams.initiator = 0;
        delbaparams.reserved0 = 0;

        spin_lock(&ic->ic_addba_lock);
        if (ic->ic_delba_process)
                ic->ic_delba_process(ni, &delbaparams, reasoncode);
        spin_unlock(&ic->ic_addba_lock);

        /* send DELBA */
        actionargs.category     = IEEE80211_ACTION_CAT_BA;
        actionargs.action       = IEEE80211_ACTION_BA_DELBA;
        actionargs.arg1         = tidno;
        actionargs.arg2         = initiator;
        actionargs.arg3         = reasoncode;

        ieee80211_send_action(ni, &actionargs, NULL);

    } else {
        /* send the management frame buffer to the target */
        wmi_unified_delba_send_cmd_send(pdev_wmi_handle, ni->ni_macaddr,
                                        &param);
    }
}

/**
 * ol_ath_net80211_addba_setresponse() - Send addba setresponse command
 * @ni: node information
 * @tidno: tid
 * @statuscode: status code in response
 *
 * Return: none
 */
static void ol_ath_net80211_addba_setresponse(struct ieee80211_node *ni,
                                              uint8_t tidno,
                                              uint16_t statuscode)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211vap *vap = ni->ni_vap;
    struct addba_setresponse_params param;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wmi_unified *pdev_wmi_handle;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    soc_txrx_handle =
                wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(pdev));

    param.vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    param.tidno = tidno;
    param.statuscode = statuscode;

    cdp_set_addbaresponse(soc_txrx_handle, ni->ni_macaddr,
                          wlan_vdev_get_id(vap->vdev_obj), tidno, statuscode);

    wmi_unified_addba_setresponse_cmd_send(pdev_wmi_handle, ni->ni_macaddr,
                                           &param);
}

/**
 * ol_ath_net80211_send_singleamsdu() - Send single VHT MPDU AMSDUs
 * @ni: node information
 * @tidno: tid
 *
 * Return: none
 */
static void ol_ath_net80211_send_singleamsdu(struct ieee80211_node *ni,
                                             uint8_t tidno)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211vap *vap = ni->ni_vap;
    struct singleamsdu_params param;
    struct wmi_unified *pdev_wmi_handle;

    pdev = wlan_vdev_get_pdev(vap->vdev_obj);
    pdev_wmi_handle = lmac_get_pdev_wmi_handle(pdev);
    if (!pdev_wmi_handle)
        return;

    qdf_mem_set(&param, sizeof(param), 0);
    param.vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    param.tidno = tidno;

    /* send the management frame buffer to the target */
    wmi_unified_singleamsdu_cmd_send(pdev_wmi_handle, ni->ni_macaddr, &param);
}

/**
 * ol_ath_set_ratecap() - Determine the capabilities of the peer
 * for use by the rate control module residing in the target
 * @ni: node information
 *
 * Return: Returns rate capabilities
 */
static uint32_t ol_ath_set_ratecap(struct ieee80211_node *ni)
{
    uint32_t ratecap = 0;

    /* peer can support 3 streams */
    if (ni->ni_streams == 3)
        ratecap |= WMI_HOST_RC_TS_FLAG;

    /* peer can support 2 streams */
    if (ni->ni_streams >= 2)
        ratecap |= WMI_HOST_RC_DS_FLAG;

    /*
     * With SM power save, only singe stream rates can be used for static MIMOPS
     * In dynamic SM power save mode, a STA enables its multiple receive chains
     * when it receives the start of a frame sequence addressed to it.
     * The receiver switches to the multiple receive chain mode when it receives
     * RTS addressed to it and switches back immediately when frame seq ends.
     */
    if ((ni->ni_htcap & IEEE80211_HTCAP_C_SM_MASK) ==
        IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC)
        ratecap &= ~(WMI_HOST_RC_TS_FLAG|WMI_HOST_RC_DS_FLAG);

    return ratecap;
}

static uint32_t
ol_ath_net80211_rate_node_update(struct ieee80211com *ic,
                                 struct ieee80211_node *ni, int isnew)
{
    enum ieee80211_cwm_width ic_cw_width = ic->ic_cwm_get_width(ic);
    uint32_t capflag = 0;

    if (ni->ni_flags & IEEE80211_NODE_HT) {
        capflag |=  WMI_HOST_RC_HT_FLAG;

        if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40) &&
            (ic_cw_width == IEEE80211_CWM_WIDTH40)) {
            capflag |=  WMI_HOST_RC_CW40_FLAG;
        }

        if (((ni->ni_htcap & IEEE80211_HTCAP_C_SHORTGI40) &&
             (ic_cw_width == IEEE80211_CWM_WIDTH40)) ||
            ((ni->ni_htcap & IEEE80211_HTCAP_C_SHORTGI20) &&
             (ic_cw_width == IEEE80211_CWM_WIDTH20))) {
            capflag |= WMI_HOST_RC_SGI_FLAG;
        }

        /* Rx STBC is a 2-bit mask. Needs to convert from ieee definition to ath definition. */
        capflag |= (((ni->ni_htcap & IEEE80211_HTCAP_C_RXSTBC) >> IEEE80211_HTCAP_C_RXSTBC_S)
                    << WMI_HOST_RC_RX_STBC_FLAG_S);
        capflag |= ol_ath_set_ratecap(ni);
    }

    if (ni->ni_flags & IEEE80211_NODE_UAPSD)
        capflag |= WMI_HOST_RC_UAPSD_FLAG;

    return capflag;
}


/**
 * ol_ath_send_peer_update() - Update peer information
 * @ic: ic pointer
 * @ni: node information
 * @val: Peer use 4 addr value
 *
 * Return: none
 */
static void ol_ath_send_peer_update(struct ieee80211com *ic,
                                    struct ieee80211_node *ni, uint32_t val)
{
    struct ieee80211vap *vap = ni->ni_vap;
    const uint32_t min_idle_inactive_time_secs = 256;
    const uint32_t max_idle_inactive_time_secs = 256 * 2;
    const uint32_t max_unresponsive_time_secs  = (256 * 2) + 5;
    struct wlan_vdev_mgr_cfg mlme_cfg;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;

    if (ol_ath_node_set_param(ic->ic_pdev_obj, ni->ni_macaddr,
                              WMI_HOST_PEER_USE_4ADDR, val,
                              wlan_vdev_get_id(vap->vdev_obj))) {
        qdf_nofl_info("%s:Unable to change peer Next Hop setting\n", __func__);
    }

    if (vap->iv_peer_idle_time) {
         qdf_debug("Peer inactive / unresponsive time is updated already");
         return;
    } else {
       vap->iv_peer_idle_time = 1;
    }

    if (ol_ath_wmi_send_vdev_param(vap->vdev_obj,
                                   wmi_vdev_param_ap_enable_nawds,
                                   min_idle_inactive_time_secs)) {
        qdf_nofl_info("%s: Enable NAWDS Failed\n", __func__);
        vap->iv_peer_idle_time = 0;
    }

    mlme_cfg.value = min_idle_inactive_time_secs;
    if (vdev_mlme_set_param(vdev_mlme, WLAN_MLME_CFG_MIN_IDLE_INACTIVE_TIME,
                            mlme_cfg)) {
        qdf_err("setting MIN inactive time failed");
        vap->iv_peer_idle_time = 0;
    }

    mlme_cfg.value = max_idle_inactive_time_secs;
    if (vdev_mlme_set_param(vdev_mlme, WLAN_MLME_CFG_MAX_IDLE_INACTIVE_TIME,
                            mlme_cfg)) {
        qdf_err("setting MAX inactive time failed");
        vap->iv_peer_idle_time = 0;
    }

    mlme_cfg.value = max_unresponsive_time_secs;
    if (vdev_mlme_set_param(vdev_mlme,
            WLAN_MLME_CFG_MAX_UNRESPONSIVE_INACTIVE_TIME,
            mlme_cfg)) {
        qdf_err("setting MAX unresponsive time failed");
        vap->iv_peer_idle_time = 0;
    }

}

int
ol_ath_net80211_newassoc(struct ieee80211_node *ni, int isnew)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    A_UINT32 uapsd, max_sp, trigger_tid = 0;
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_psoc *psoc;
    ol_ath_soc_softc_t *soc;
    uint8_t vdev_id = wlan_vdev_get_id(vap->vdev_obj);
    cdp_config_param_type val = {0};
    int retval = 0;
    osif_dev *osdev;

    struct wlan_objmgr_pdev *pdev = (vap->iv_ic->ic_pdev_obj);
    uint16_t noack_map;
    uint8_t use_4addr = 1;
    osdev = (osif_dev *)vap->iv_ifp;

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    soc = (ol_ath_soc_softc_t *)lmac_get_psoc_feature_ptr(psoc);
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    /* 0 - 3 addr for all the packets including eapol
     * 1 - 4 addr for all the packets except eapol
     * 2 - 4 addr for all packets including eapol
     */
    if (vap->iv_4addr_eapol_en) {
	    qdf_err("RUPESH sending WMI");
	    use_4addr = 2;

            /* turn on 4-addr framing for this node */
            if (ol_ath_node_set_param(scn->sc_pdev, ni->ni_macaddr,
                                      WMI_HOST_PEER_USE_4ADDR, use_4addr,
                                      wlan_vdev_get_id(vap->vdev_obj)))
                qdf_err("node set 4-addr framing including eapol failed");
    }

#if WDS_VENDOR_EXTENSION
    if ((ni->ni_flags & IEEE80211_NODE_NAWDS) && (ni->ni_flags & IEEE80211_NODE_WDS)) {
	    int wds_tx_policy_ucast = 0, wds_tx_policy_mcast = 0;

	    if (ni->ni_wds_tx_policy) {
		    wds_tx_policy_ucast = (ni->ni_wds_tx_policy & WDS_POLICY_TX_UCAST_4ADDR) ? 1: 0;
		    wds_tx_policy_mcast = (ni->ni_wds_tx_policy & WDS_POLICY_TX_MCAST_4ADDR) ? 1: 0;
		    cdp_set_wds_tx_policy_update(soc_txrx_handle, vdev_id, ni->ni_macaddr,
				    wds_tx_policy_ucast, wds_tx_policy_mcast);
	    }
	    else {
		    /* if tx_policy is not set, and node is WDS, ucast/mcast frames will be sent as 4ADDR */
		    wds_tx_policy_ucast = wds_tx_policy_mcast = 1;
		    cdp_set_wds_tx_policy_update(soc_txrx_handle, vdev_id, ni->ni_macaddr,
				    wds_tx_policy_ucast, wds_tx_policy_mcast);
	    }
            if (ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP) {
                if (wds_tx_policy_mcast) {
                    /* turn on MCAST_INDICATE so that multicast/broadcast frames
                     * can be cloned and sent as 4-addr directed frames to clients
                     * that want them in 4-addr format
                     */
                    ol_ath_wmi_send_vdev_param(vap->vdev_obj,
                                               wmi_vdev_param_mcast_indicate, 1);
                    /* turn on UNKNOWN_DEST_INDICATE so that unciast frames to
                     * unknown destinations are indicated to host which can then
                     * send to all connected WDS clients
                     */
                    ol_ath_wmi_send_vdev_param(vap->vdev_obj,
                                    wmi_vdev_param_unknown_dest_indicate, 1);

                }
                if (wds_tx_policy_ucast || wds_tx_policy_mcast) {
                    /* turn on 4-addr framing for this node */
                    if (ol_ath_node_set_param(scn->sc_pdev, ni->ni_macaddr,
                                              WMI_HOST_PEER_USE_4ADDR, use_4addr,
                                              wlan_vdev_get_id(vap->vdev_obj)))
                        qdf_err("node set 4-addr framing failed");
                 }
        }
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (scn->nss_radio.nss_rctx) {
            /*
             * Sets WDS Vendor Extension flag and pushes configuration to NSS FW
             */
            if (ic->nss_vops) {
                ic->nss_vops->ic_osif_nss_vdev_set_cfg(vap->iv_ifp, OSIF_NSS_WIFI_VDEV_WDS_EXT_ENABLE);
            }
            if (ic->nss_radio_ops) {
                ic->nss_radio_ops->ic_nss_ol_wds_extn_peer_cfg_send(scn, ni->ni_macaddr, vdev_id);
            }
        }
#endif //QCA_NSS_WIFI_OFFLOAD_SUPPORT
    }
#endif //WDS_VENDOR_EXTENSION

    /*
     * 1. Check NAWDS or not
     * 2. Do not Pass PHY MODE 0 as association not allowed in NAWDS.
     *     rc_mask will be NULL and Tgt will assert
     */

    if (ni->ni_flags & IEEE80211_NODE_NAWDS ) {
        IEEE80211_DPRINTF( vap, IEEE80211_MSG_ASSOC, "\n NODE %pK is NAWDS ENABLED\n",ni);

        if(ni->ni_phymode == 0){
            ni->ni_phymode = vap->iv_des_mode;
        }

        /* Update Host Peer Table */
        val.cdp_peer_param_nawds = 1;
        cdp_txrx_set_peer_param(soc_txrx_handle, vdev_id, ni->ni_macaddr, CDP_CONFIG_NAWDS, val);

            /* Vdev nawds enabling is required only
             * to differentiate Normal/Nawds path
             * But NAWDS is per peer not per vdev
             */
        val.cdp_vdev_param_nawds = 1;
        cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id, CDP_ENABLE_NAWDS, val);
        osif_vap_tx_activity_update(vap);

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (ic->nss_vops)
            ic->nss_vops->ic_osif_nss_vdev_set_cfg(vap->iv_ifp, OSIF_NSS_WIFI_VDEV_NAWDS_MODE);

        /*
         * Send nawds enable on this peer.
         */
        if (soc && soc->nss_soc.ops) {
            soc->nss_soc.ops->nss_soc_nawds_enable(soc, ni->ni_macaddr, vdev_id, 1);
        }
#endif
        ol_ath_send_peer_update(ic, ni, use_4addr);
    }

    val.cdp_peer_param_dms = (ni->ni_flags & IEEE80211_NODE_DISABLE_DMS) ? 0 : 1;
    cdp_txrx_set_peer_param(soc_txrx_handle, vdev_id, ni->ni_macaddr, CDP_CONFIG_PEER_DMS, val);
    /* TODO: Fill in security params */

    if (ieee80211_validate_assoc_info(ni)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                          "%s Peer Association failed as Assoc params validation failed", __func__);
        return -EINVAL;
    }

    /* Notify target of the association/reassociation */
    retval = ol_ath_send_peer_assoc(ni, isnew);
    if (retval) {
        qdf_err("Failed to send peer assoc to FW");
#ifdef WLAN_FEATURE_11BE_MLO
        /* Set flag to indicate peer assoc is not sent */
        wlan_mlo_link_peer_assoc_set(ni->peer_obj, false);
#endif
        return retval;
    }

    if (ni->ni_flags & IEEE80211_NODE_NAWDS ) {
        /* Update Host Peer Table */
        val.cdp_peer_param_nawds = 1;
        cdp_txrx_set_peer_param(soc_txrx_handle, vdev_id, ni->ni_macaddr, CDP_CONFIG_NAWDS, val);
    }

    if (vap->iv_is_noack_policy_set) {
        /*Apply NoAck policy on newly assoc peer*/
        noack_map = vap->noack_map;
        retval = ic->ic_set_peer_tid_noack_config(pdev, vdev_id, ni->ni_macaddr, noack_map);
        if (retval < 0)
            qdf_err("Could not set the noack policy");
    }

    /* XXX must be sent _after_ new assoc */
    switch (vap->iv_opmode) {
    case IEEE80211_M_HOSTAP:
        if (ni->ni_flags & IEEE80211_NODE_UAPSD) {
            if(isnew){
                uapsd = 0;
                if (WME_UAPSD_AC_ENABLED(0, ni->ni_uapsd)) {
                    uapsd |= WMI_HOST_AP_PS_UAPSD_AC0_DELIVERY_EN |
                        WMI_HOST_AP_PS_UAPSD_AC0_TRIGGER_EN;
                }
                if (WME_UAPSD_AC_ENABLED(1, ni->ni_uapsd)) {
                    uapsd |= WMI_HOST_AP_PS_UAPSD_AC1_DELIVERY_EN |
                        WMI_HOST_AP_PS_UAPSD_AC1_TRIGGER_EN;
                }
                if (WME_UAPSD_AC_ENABLED(2, ni->ni_uapsd)) {
                    uapsd |= WMI_HOST_AP_PS_UAPSD_AC2_DELIVERY_EN |
                        WMI_HOST_AP_PS_UAPSD_AC2_TRIGGER_EN;
                }
                if (WME_UAPSD_AC_ENABLED(3, ni->ni_uapsd)) {
                    uapsd |= WMI_HOST_AP_PS_UAPSD_AC3_DELIVERY_EN |
                        WMI_HOST_AP_PS_UAPSD_AC3_TRIGGER_EN;
                }
                (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                        WMI_HOST_AP_PS_PEER_PARAM_UAPSD, uapsd);
            } else {
#if UMAC_SUPPORT_ADMCTL
                ic->ic_node_update_dyn_uapsd(ni,0,WME_UAPSD_AC_INVAL,WME_UAPSD_AC_INVAL);
#endif
            }
            switch (ni->ni_uapsd_maxsp) {
            case 2:
                max_sp = WMI_HOST_AP_PS_PEER_PARAM_MAX_SP_2;
                break;
            case 4:
                max_sp = WMI_HOST_AP_PS_PEER_PARAM_MAX_SP_4;
                break;
            case 6:
                max_sp = WMI_HOST_AP_PS_PEER_PARAM_MAX_SP_6;
                break;
            default:
                max_sp = WMI_HOST_AP_PS_PEER_PARAM_MAX_SP_UNLIMITED;
                break;
            }
        } else {
            uapsd = 0;
            max_sp = 0;
            (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                        WMI_HOST_AP_PS_PEER_PARAM_UAPSD, uapsd);
        }
        (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                    WMI_HOST_AP_PS_PEER_PARAM_MAX_SP, max_sp);

        if (scn->scn_sifs_frmtype) {
            (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                    WMI_HOST_AP_PS_PEER_PARAM_SIFS_RESP_FRMTYPE,
                                    scn->scn_sifs_frmtype);

#define BE_AC_MASK 0x1
#define BK_AC_MASK 0x2
#define VI_AC_MASK 0x4
#define VO_AC_MASK 0x8

#define TRIGGER_BE_TIDS 0x9
#define TRIGGER_BK_TIDS 0x6
#define TRIGGER_VI_TIDS 0x30
#define TRIGGER_VO_TIDS 0xC0
            if(scn->scn_sifs_uapsd & BE_AC_MASK)
                trigger_tid = TRIGGER_BE_TIDS;
            if(scn->scn_sifs_uapsd & BK_AC_MASK)
                trigger_tid |= TRIGGER_BK_TIDS;
            if(scn->scn_sifs_uapsd & VI_AC_MASK)
                trigger_tid |= TRIGGER_VI_TIDS;
            if(scn->scn_sifs_uapsd & VO_AC_MASK)
                trigger_tid |= TRIGGER_VO_TIDS;

            (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                    WMI_HOST_AP_PS_PEER_PARAM_SIFS_RESP_UAPSD,
                                    trigger_tid);
        } else {
            /*
             * No meaning to enable trigger too. Disable this too
             */
            AR_DEBUG_PRINTF(ATH_DEBUG_INFO,("\n Disabling Both SIFS RESP and UAPSD TRIGGER\n"));
#define DISABLE_SIFS_RESP_TRIGGER 0x0
            (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                    WMI_HOST_AP_PS_PEER_PARAM_SIFS_RESP_FRMTYPE,
                                    DISABLE_SIFS_RESP_TRIGGER);
            (void)ol_power_set_ap_ps_param(vap, OL_ATH_NODE_NET80211(ni),
                                    WMI_HOST_AP_PS_PEER_PARAM_SIFS_RESP_UAPSD,
                                    DISABLE_SIFS_RESP_TRIGGER);
        }
        break;
    default:
        break;
    }

    return 0;
}

static INLINE void
ol_ath_rxstat2ieee(struct ieee80211com *ic,
                struct mgmt_rx_event_params *rx_event,
                struct ieee80211_rx_status *rs)
{
    uint32_t phy_mode = (uint32_t) rx_event->phy_mode;
    struct chan_params chan_p = {0};
    /* TBD: More fields to be updated later */
    rs->rs_snr      = rx_event->snr;
    rs->rs_rssi     = rx_event->rssi;
    rs->rs_datarate = rx_event->rate;
    rs->rs_channel  = rx_event->channel;
    rs->rs_flags  = 0;
    if (rx_event->status & WMI_HOST_RXERR_CRC)
        rs->rs_flags  |= IEEE80211_RX_FCS_ERROR;
    if (rx_event->status & WMI_HOST_RXERR_DECRYPT)
        rs->rs_flags  |= IEEE80211_RX_DECRYPT_ERROR;
    if (rx_event->status & WMI_HOST_RXERR_MIC)
        rs->rs_flags  |= IEEE80211_RX_MIC_ERROR;
    if (rx_event->status & WMI_HOST_RXERR_KEY_CACHE_MISS)
        rs->rs_flags  |= IEEE80211_RX_KEYMISS;
    if (rx_event->status & WMI_HOST_RXERR_PN)
        rs->rs_flags  |= IEEE80211_RX_PN_ERROR;

    /* TBD: whalGetNf in firmware is fixed to-96. Maybe firmware should calculate it based on whalGetChanNf? */
    rs->rs_abs_rssi = rx_event->snr + ATH_DEFAULT_NOISEFLOOR;
    switch (phy_mode)
    {
    case WMI_HOST_MODE_11A:
        rs->rs_phymode = IEEE80211_MODE_11A;
        break;
    case WMI_HOST_MODE_11B:
        rs->rs_phymode = IEEE80211_MODE_11B;
        break;
    case WMI_HOST_MODE_11G:
        rs->rs_phymode = IEEE80211_MODE_11G;
        break;
    case WMI_HOST_MODE_11NA_HT20:
        rs->rs_phymode = IEEE80211_MODE_11NA_HT20;
        break;
    case WMI_HOST_MODE_11NA_HT40:
        if (ic->ic_cwm_get_extoffset(ic) == EXT_CHAN_OFFSET_ABOVE)
            rs->rs_phymode = IEEE80211_MODE_11NA_HT40PLUS;
        else
            rs->rs_phymode = IEEE80211_MODE_11NA_HT40MINUS;
        break;
    case WMI_HOST_MODE_11NG_HT20:
        rs->rs_phymode = IEEE80211_MODE_11NG_HT20;
        break;
    case WMI_HOST_MODE_11NG_HT40:
        if (ic->ic_cwm_get_extoffset(ic) == EXT_CHAN_OFFSET_ABOVE)
            rs->rs_phymode = IEEE80211_MODE_11NG_HT40PLUS;
        else
            rs->rs_phymode = IEEE80211_MODE_11NG_HT40MINUS;
        break;
    case WMI_HOST_MODE_11AC_VHT20:
        rs->rs_phymode = IEEE80211_MODE_11AC_VHT20;
        break;
    case WMI_HOST_MODE_11AC_VHT40:
        if (ic->ic_cwm_get_extoffset(ic) == EXT_CHAN_OFFSET_ABOVE)
            rs->rs_phymode = IEEE80211_MODE_11AC_VHT40PLUS;
        else
            rs->rs_phymode = IEEE80211_MODE_11AC_VHT40MINUS;
        break;
    case WMI_HOST_MODE_11AC_VHT80:
        rs->rs_phymode = IEEE80211_MODE_11AC_VHT80;
        break;
    case WMI_HOST_MODE_11AC_VHT160:
        rs->rs_phymode = IEEE80211_MODE_11AC_VHT160;
        break;
    case WMI_HOST_MODE_11AC_VHT80_80:
        rs->rs_phymode = IEEE80211_MODE_11AC_VHT80_80;
        break;
    /* 11AX TODO (Phase II) - Add HE mode related entries here after confirming
     * applicability.
     */
    /* 11BE modes not added as HE modes are not processed */
    default:
        break;
    }
    rs->rs_freq = rx_event->chan_freq;
#ifdef HOST_SUPPORT_BEELINER_MPHYR
#define DEFAULT_MPHYR_FREQ 5200
        rs->rs_freq = DEFAULT_MPHYR_FREQ;
#endif
    if (!rs->rs_freq) {
        /*
         * If rx_status was created for an invalid peer, then band
         * and channel number need to be set as invalid.
         */
        rs->rs_band    = WLAN_BAND_UNSPECIFIED;
        rs->rs_channum = 0;
    } else {
        rs->rs_band = reg_wifi_band_to_wlan_band_id(wlan_reg_freq_to_band(rs->rs_freq));
        rs->rs_channum = wlan_reg_freq_to_chan(ic->ic_pdev_obj, rs->rs_freq);
    }
    rs->rs_full_chan = &rs->rs_full_chan_obj;
    *rs->rs_full_chan = ol_ath_find_full_channel(ic, &chan_p);
    rs->rs_isvalidsnr = 0;
}

#ifndef REMOVE_PKT_LOG
void
chk_vht_groupid_action_frame(struct ol_ath_softc_net80211 * scn,struct ieee80211_node *ni, wbuf_t wbuf)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_frame *wh;
    struct ieee80211_action_vht_gid_mgmt  *gid_mgmt_frm;
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    int type, subtype;
    u_int8_t action_category;
    u_int8_t vht_action;
    u_int8_t *mem_status;
    u_int8_t *user_position;

    wlan_wbuf_set_peer_node(wbuf, ni);

    if (wbuf_get_pktlen(wbuf) < ic->ic_minframesize) {
        goto endtortn;
    }
    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    if ((wh->i_fc[0] & IEEE80211_FC0_VERSION_MASK) != IEEE80211_FC0_VERSION_0) {
        goto endtortn;
    }
    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    if((type == IEEE80211_FC0_TYPE_MGT)&&(subtype == IEEE80211_FC0_SUBTYPE_ACTION))
    {
        action_category = ((struct ieee80211_action *)(&wh[1]))->ia_category;
        vht_action = ((struct ieee80211_action *)(&wh[1]))->ia_action;
        if((action_category == IEEE80211_ACTION_CAT_VHT)&&(vht_action == IEEE80211_ACTION_VHT_GROUP_ID))
        {
           /* Save the group ID managemnt information so dump themn into log file in rx_info_remote of pktlog*/
           gid_mgmt_frm = (struct ieee80211_action_vht_gid_mgmt *)(&wh[1]);
	   mem_status = &((gid_mgmt_frm->member_status[0]));
	   user_position = &((gid_mgmt_frm->user_position[0]));

	   cdp_set_gid_flag(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj),
			   mem_status, user_position);

        }
    }

endtortn:
    return;

}
#endif

static bool ol_ath_is_bcn_rate_limit_needed(struct ieee80211com *ic,
                                            struct ieee80211_frame *wh)
{
    struct ieee80211vap *stavap = ic->ic_sta_vap;

    if (stavap) {
        if (ieee80211_is_vap_down(stavap)) {
              return true;
	}

        if (!ieee80211_vap_is_connected(stavap) ||
            IEEE80211_ADDR_EQ(wh->i_addr3, stavap->iv_bss->ni_bssid)) {
              return false;
        }
    }

    return true;
}

/*
 * WMI RX event handler for management frames
 */
static int
ol_ath_mgmt_rx_event_handler(ol_scn_t sc, u_int8_t *data, u_int32_t datalen)
{
    ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
    struct ieee80211com *ic;
    struct ieee80211_frame *wh;
    uint8_t *bufp;
    uint32_t len;
    wbuf_t wbuf;
    struct mgmt_rx_event_params *rx_event;
    struct ol_ath_softc_net80211 *scn;
    struct wmi_unified *wmi_handle;
    struct wlan_objmgr_pdev *pdev;
    enum wmi_target_type wmi_tgt_type;
    QDF_STATUS status;
    int ret;
    struct wlan_lmac_if_mgmt_txrx_rx_ops *mgmt_rx_ops;

    wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
    if (!wmi_handle) {
        qdf_err("wmi_handle is null");
        return -EINVAL;
    }

    rx_event = alloc_mgmt_rx_event_params();
    if (!rx_event) {
        qdf_err("Unable to allocate MGMT Rx event params");
        return -ENOMEM;
    }

    if(wmi_extract_mgmt_rx_params(wmi_handle, data, rx_event, &bufp)) {
        qdf_err("Failed to extract mgmt frame");
        ret = 0;
        goto free_rx_params;
    }

    /* Extract MGMT Rx REO parameters */
    status  = target_if_mgmt_rx_reo_extract_reo_params(wmi_handle,
                                                       data, rx_event);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Failed to extract MGMT Rx REO params");
        ret = -1;
        goto free_rx_params;
    }

    if (wmi_service_enabled(wmi_handle,
                            wmi_service_pn_replay_check_support) &&
            wmi_extract_frame_pn_params(wmi_handle, data,
                                        &rx_event->pn_params)) {
        qdf_err("Failed to extract Frame PN Params");
        ret = -1;
    }

    if(wmi_extract_mgmt_rx_ext_params(wmi_handle, data, rx_event->ext_params)) {
        qdf_debug("Unable to extract mgmt rx ext params, setting it to zero");
        rx_event->ext_params->u.addba.ba_win_size = 0;
        rx_event->ext_params->u.addba.reo_win_size = 0;
    }

    if (wmi_service_enabled(wmi_handle,
                            wmi_service_is_my_mgmt_frame) &&
            wmi_extract_is_conn_ap_frame(wmi_handle, data, &rx_event->is_conn_ap)) {
            qdf_info("Failed to extract is_con_ap param");
            ret = -1;
    }

    status = target_if_extract_mlo_link_removal_info_mgmt_rx(
		wmi_handle, data, rx_event);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Failed to extract MLO link removal information");
        ret = qdf_status_to_os_return(status);
        goto free_rx_params;
    }

    pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, PDEV_UNIT(
                                        rx_event->pdev_id), WLAN_MLME_SB_ID);
    if (pdev == NULL) {
        qdf_err("%s: pdev object (id: %d) is NULL", __func__,
                  PDEV_UNIT(rx_event->pdev_id));
        ret = -1;
        goto free_rx_params;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (scn == NULL) {
        qdf_err("scn object is NULL");
        ret = -1;
        goto release_pdev_ref;
    }

    ic = &scn->sc_ic;
    if (ic == NULL) {
        qdf_err("ic object is NULL");
        ret = -1;
        goto release_pdev_ref;
    }

    if (!rx_event->chan_freq) {
        /**
          * If the channel frequency is not filled by F/W, 6G is not supported.
          * Hence the channel is a 2G/5G channel
          */
        rx_event->chan_freq = wlan_chan_to_freq(rx_event->channel);
    }

    if (!rx_event->channel) {
        rx_event->channel = wlan_reg_freq_to_chan(pdev, rx_event->chan_freq);
    }

#if ATH_ACS_DEBUG_SUPPORT
    /*
     * Ignoring external beacons when the ACS debug framework
     * is enabled so as to display only the custom beacons sent from the
     * ACS debug tool
     */
    if (ic->ic_acs_debug_support) {
        ret = 0;
        goto release_pdev_ref;
    }
#endif

    /* Calculate the RSSI for WMI NON_TLV targets in the host as FW does not
     * have the support to send the RSSI value to the host through
     * WMI_MGMT_RX_EVENTID.
     */

    status = ol_ath_get_wmi_target_type(soc, &wmi_tgt_type);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Failed to get wmi target type");
        ret = -1;
        goto release_pdev_ref;
    }

    if (wmi_tgt_type == WMI_NON_TLV_TARGET)
        rx_event->rssi = rx_event->snr + scn->chan_nf;

    len = roundup(rx_event->buf_len, sizeof(u_int32_t));
    wbuf =  wbuf_alloc(ic->ic_osdev, WBUF_RX_INTERNAL,
                       len);

    if (wbuf == NULL) {
        qdf_err("wbuf alloc failed");
        ret = 0;
        goto release_pdev_ref;
    }

#ifdef DEBUG_RX_FRAME
   {
    int i;
    qdf_nofl_info("%s wbuf 0x%x frame length %d  \n ",
                 __func__,(unsigned int) wbuf, rx_event->buf_len);
    for (i=0;i<rx_event->buf_len; ++i ) {
      qdf_nofl_info("%x ", bufp[i]);
      if (i%16 == 0) qdf_nofl_info("\n");
    }
   }
   qdf_nofl_info("%s rx frame type 0x%x frame length %d  \n ",
                 __func__,bufp[0], rx_event->buf_len);
#endif

    wbuf_init(wbuf, rx_event->buf_len);
    wh = (struct ieee80211_frame *)wbuf_header(wbuf);
#ifdef BIG_ENDIAN_HOST
    {
        /* for big endian host, copy engine byte_swap is enabled
         * But the rx mgmt frame buffer content is in network byte order
         * Need to byte swap the mgmt frame buffer content - so when copy engine
         * does byte_swap - host gets buffer content in the correct byte order
         */
        int i;
        u_int32_t *destp, *srcp;
        destp = (u_int32_t *)wh;
        srcp =  (u_int32_t *)bufp;
        for(i=0; i < (len/4); i++) {
            *destp = cpu_to_le32(*srcp);
            destp++; srcp++;
        }
    }
#else
    OS_MEMCPY(wh, bufp, rx_event->buf_len);
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    if (IEEE80211_IS_PROBEREQ(wh) || IEEE80211_IS_ASSOCREQ(wh) ||
            IEEE80211_IS_REASSOCREQ(wh)) {
        if (wmi_extract_mgmt_rx_ml_cu_params(wmi_handle, data,
                    &rx_event->cu_params)) {
            qdf_debug("Failed to extract CU Params");
        }
    }

    if (IEEE80211_IS_PROBEREQ(wh) && ol_ath_mlo_capable(soc->psoc_obj)) {
        if (wmi_extract_mlo_vdev_bcast_tid_to_link_map_event(wmi_handle, data,
                    &rx_event->t2lm_params)) {
            qdf_err("Failed to extract T2LM info");
        }
    }
#endif

    if (IEEE80211_IS_BEACON(wh) && ic->ic_enable_bcn_ratelimit &&
        ol_ath_is_bcn_rate_limit_needed(ic, wh)) {
	if (ic->ic_bcn_process_cnt >= 100) {
           ic->ic_bcn_process_cnt = 0;
        }
        ic->ic_bcn_process_cnt++;
        if (ic->ic_bcn_process_cnt > ic->ic_bcn_ratelimit_count) {
            wbuf_free(wbuf);
            ic->ic_bcn_dropped_cnt++;
            ret = 0;
            goto release_pdev_ref;
        }
    }
    mgmt_rx_ops = wlan_lmac_if_get_mgmt_txrx_rx_ops(soc->psoc_obj);

    if (mgmt_rx_ops && mgmt_rx_ops->mgmt_rx_frame_entry) {
        status = mgmt_rx_ops->mgmt_rx_frame_entry(pdev, wbuf, rx_event);

        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
        if (QDF_IS_STATUS_ERROR(status)) {
#ifdef WLAN_MGMT_RX_REO_ERROR_HANDLING
            qdf_rl_debug("Failed to process mgmt rx frame");
#else
            qdf_rl_err("Failed to process mgmt rx frame");
#endif /* WLAN_MGMT_RX_REO_ERROR_HANDLING */
            return -1;
        }
        return 0;
    }

    wbuf_free(wbuf);
    ret = -1;

release_pdev_ref:
    status = target_if_mgmt_rx_reo_host_drop_handler(pdev, rx_event);
    if (QDF_IS_STATUS_ERROR(status))
        qdf_rl_err("Failed to handle dropped frame");

    wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);

free_rx_params:
    free_mgmt_rx_event_params(rx_event);

    return ret;
}

void
ol_ath_mgmt_handler(struct wlan_objmgr_pdev *pdev,
        struct ol_ath_softc_net80211 *scn,
        wbuf_t wbuf,
        struct ieee80211_frame * wh,
        struct mgmt_rx_event_params *rx_event,
        bool null_data_handler)
{

    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211_rx_status rs;
    struct ieee80211_node *ni;
    struct wlan_objmgr_psoc *psoc;

#if ATH_SUPPORT_IWSPY
    wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    ieee80211_input_iwspy_update_snr(ic, wh->i_addr2, rx_event->snr);
#endif
    /*
     * From this point on we assume the frame is at least
     * as large as ieee80211_frame_min; verify that.
     */
    if (wbuf_get_pktlen(wbuf) < ic->ic_minframesize) {
        qdf_nofl_info("%s: short packet %d\n", __func__, wbuf_get_pktlen(wbuf));
        wbuf_free(wbuf);
        return;
    }

    /* Drop mgmt frame while mode switch is in progress */
    if (scn->soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) {
        struct wmi_unified *wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
        int subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

        if (!wmi_handle || wmi_is_blocked(wmi_handle)) {
            QDF_TRACE(QDF_MODULE_ID_DYNAMIC_MODE_CHG,
                      QDF_TRACE_LEVEL_INFO_HIGH,
                      "Mode switch in progress, dropping mgmt frame subtype 0x%x (wmi_handle %pK)\n",
                      subtype, wmi_handle);
            wbuf_free(wbuf);
            return;
        }
    }

    qdf_mem_zero(&rs, sizeof(rs));

    /*
     * Locate the node for sender, track state, and then
     * pass the (referenced) node up to the 802.11 layer
     * for its use.  If the sender is unknown spam the
     * frame; it'll be dropped where it's not wanted.
     */
    ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)
                               wbuf_header(wbuf), WLAN_MGMT_RX_ID);
    psoc = wlan_pdev_get_psoc(scn->sc_pdev);
    rs.rs_ic = ic;
    rs.rs_ni = ni;
    rs.rs_snr = rx_event->snr;
    rs.ba_win_size = rx_event->ext_params->u.addba.ba_win_size;
    rs.reo_win_size = rx_event->ext_params->u.addba.reo_win_size;

    if (rx_event->ext_params->meta_id == MGMT_RX_PARAMS_EXT_META_TWT) {
        rs.twt_ie_len = rx_event->ext_params->u.twt.ie_len;

	if (rs.twt_ie_len)
            rs.twt_ie_data = rx_event->ext_params->u.twt.ie_data;
	else
	    rs.twt_ie_data = NULL;

	rs.twt_ie_meta = true;
    } else {
        rs.twt_ie_data = NULL;
	rs.twt_ie_meta = false;
        rs.twt_ie_len = 0;
    }

    /*
     * rs to be made available in the placeholder
     * in rx_event structure used by mgmt_txrx
     */
    rx_event->rx_params = (void *)&rs;

    /* If we receive a probereq with broadcast bssid which is usually
     * sent by sender to discover new networks, all the vaps should send reply
     */
    if (ni == NULL || (IEEE80211_IS_PROBEREQ(wh) && IEEE80211_IS_BROADCAST(wh->i_addr3))) {
        ol_ath_rxstat2ieee(ic, rx_event, &rs);

#ifdef WLAN_FEATURE_11BE_MLO
        if (IEEE80211_IS_PROBEREQ(wh) || IEEE80211_IS_ASSOCREQ(wh) ||
                IEEE80211_IS_REASSOCREQ(wh)) {
            rs.rs_cu = rx_event->cu_params;
            ic->ic_cu = rx_event->cu_params;
        }

        if (IEEE80211_IS_PROBEREQ(wh)) {
            rs.rs_t2lm = rx_event->t2lm_params;
        }

        rs.link_removal_info = rx_event->link_removal_info;
        rs.num_link_removal_info = rx_event->num_link_removal_info;
#endif
        mgmt_txrx_rx_handler(psoc, wbuf, rx_event);
        if (ni) {
            ieee80211_free_node(ni, WLAN_MGMT_RX_ID);
        }
    } else {
        ol_ath_rxstat2ieee(ni->ni_ic, rx_event, &rs);

#ifdef WLAN_FEATURE_11BE_MLO
        if (IEEE80211_IS_PROBEREQ(wh) || IEEE80211_IS_ASSOCREQ(wh) ||
                IEEE80211_IS_REASSOCREQ(wh)) {
            rs.rs_cu = rx_event->cu_params;
        }

        if (IEEE80211_IS_PROBEREQ(wh)) {
            rs.rs_t2lm = rx_event->t2lm_params;
        }

        rs.link_removal_info = rx_event->link_removal_info;
        rs.num_link_removal_info = rx_event->num_link_removal_info;
#endif
#ifdef QCA_SUPPORT_CP_STATS
        WLAN_PEER_CP_STAT_SET(ni, rx_mgmt_rate, rx_event->rate);
        WLAN_PEER_CP_STAT_SET(ni, rx_mgmt_snr, rx_event->snr);
#endif

#ifndef REMOVE_PKT_LOG
        if(scn->pl_dev && !null_data_handler)
            chk_vht_groupid_action_frame(scn, ni, wbuf);
#endif
        /*
         * hand over the wbuf to the mgmt_txrx layer
         * for further processing.
         */
        mgmt_txrx_rx_handler(psoc, wbuf, rx_event);
        ieee80211_free_node(ni, WLAN_MGMT_RX_ID);
    }
    return ;
}
qdf_export_symbol(ol_ath_mgmt_handler);

QDF_STATUS ol_ath_mgmt_rx_frame_handler(
			struct wlan_objmgr_pdev *pdev,
			qdf_nbuf_t wbuf,
			struct mgmt_rx_event_params *mgmt_rx_params)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211_frame * wh;

    if (!wbuf) {
        qdf_err("wbuf is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!pdev) {
        qdf_err("pdev is NULL");
        wbuf_free(wbuf);
        return QDF_STATUS_E_NULL_VALUE;
    }

    if (!mgmt_rx_params) {
        qdf_err("MGMT Rx Params is NULL");
        wbuf_free(wbuf);
        return QDF_STATUS_E_NULL_VALUE;
    }

    scn = lmac_get_pdev_feature_ptr(pdev);
    if (!scn) {
        qdf_err("scn object is NULL");
        wbuf_free(wbuf);
        return QDF_STATUS_E_NULL_VALUE;
    }

    wh = (struct ieee80211_frame *)wbuf_header(wbuf);

    ol_ath_mgmt_handler(pdev, scn, wbuf, wh, mgmt_rx_params, false);

    return QDF_STATUS_SUCCESS;
}

static int
mgmt_crypto_encap(wbuf_t wbuf, struct ieee80211_node *ni)
{
    struct ieee80211_frame *wh;
    wh = (struct ieee80211_frame *)wbuf_header(wbuf);

    /* encap only incase of WEP bit set */
    if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
        struct wlan_objmgr_pdev *pdev;
        struct wlan_objmgr_psoc *psoc;
        struct wlan_lmac_if_crypto_rx_ops *crypto_rx_ops;
        struct ieee80211com *ic = ni->ni_ic;
        struct ieee80211vap *vap = ni->ni_vap;
        struct ieee80211vap *pvap = NULL;
        int tid;

        pdev = ic->ic_pdev_obj;

        if(pdev == NULL) {
            qdf_print("%s[%d]pdev is NULL", __func__, __LINE__);
            return -1;
        }
        psoc = wlan_pdev_get_psoc(pdev);

        if(psoc == NULL) {
            qdf_print("%s[%d]psoc is NULL", __func__, __LINE__);
            return -1;
        }

        crypto_rx_ops = wlan_crypto_get_crypto_rx_ops(psoc);
        tid = wbuf_get_tid(wbuf);
        if (tid == OFFCHAN_EXT_TID_NONPAUSE) {
            if (!TAILQ_EMPTY(&ic->ic_vaps)) {
                struct ieee80211vap *tvap = NULL;
                TAILQ_FOREACH(tvap, &ic->ic_vaps, iv_next) {
                if ((tvap->iv_opmode == IEEE80211_M_MONITOR) ||
                    (IEEE80211_IS_SPECIAL_VAP_ONLY(tvap))) {
                        pvap = tvap;
                        break;
                    }
                }
            }
        }

        if((pvap != NULL) && (IEEE80211_ADDR_EQ(wh->i_addr1, pvap->mcast_encrypt_addr))) {
            if (crypto_rx_ops && WLAN_CRYPTO_RX_OPS_ENCAP(crypto_rx_ops)) {
                if (WLAN_CRYPTO_RX_OPS_ENCAP(crypto_rx_ops)(pvap->vdev_obj,
                        wbuf, pvap->mcast_encrypt_addr, 0) != QDF_STATUS_SUCCESS) {
                    qdf_info("Mgmt encap Failed \n");
                    return -1;
                }
            } else {
                qdf_info("Mgmt encap Failed \n");
                return -1;
            }
        } else {
            if (crypto_rx_ops && WLAN_CRYPTO_RX_OPS_ENCAP(crypto_rx_ops)) {
                if (WLAN_CRYPTO_RX_OPS_ENCAP(crypto_rx_ops)(vap->vdev_obj,
                            wbuf, ni->ni_macaddr, 0) != QDF_STATUS_SUCCESS) {
                    struct ieee80211_frame *wh;
                    wh = (struct ieee80211_frame *)wbuf_header(wbuf);
                    wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
                }
            } else {
                qdf_info("Mgmt encap Failed \n");
                return -1;
            }
        }
    }

    return 0;
}

#if WLAN_CFR_ENABLE
static int client_is_in_cfr_unassoc_pool(struct wlan_objmgr_pdev *pdev,
                                         uint8_t dest_mac[QDF_MAC_ADDR_SIZE])
{
    int retv = 0, idx = 0;
    struct pdev_cfr *pdev_cfr = NULL;
    struct unassoc_pool_entry *unassoc_entry;

    retv = wlan_objmgr_pdev_try_get_ref(pdev, WLAN_CFR_ID);
    if (retv != 0) {
        cfr_err("Unable to get pdev reference");
        return -EINVAL;
    }

    pdev_cfr = wlan_objmgr_pdev_get_comp_private_obj(pdev, WLAN_UMAC_COMP_CFR);
    if (pdev_cfr == NULL) {
        cfr_err("pdev_cfr is NULL");
        wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
        return -EINVAL;
    }

    /* Loop through table to find mac */
    for (idx = 0; idx < MAX_CFR_ENABLED_CLIENTS; idx++) {
        unassoc_entry = &(pdev_cfr->unassoc_pool[idx]);
        if (unassoc_entry->is_valid) {
            /* Compare only if entry is valid */
            if (qdf_mem_cmp(&(unassoc_entry->mac.bytes[0]), dest_mac,
                            QDF_MAC_ADDR_SIZE) == 0) {
                cfr_debug("Entry found on idx = %d", idx);
                if (unassoc_entry->cfr_params.period == 0) {
                    /* Remove entry if it is single shot */
                    qdf_mem_zero(&pdev_cfr->unassoc_pool[idx],
                                 sizeof(struct unassoc_pool_entry));
                    pdev_cfr->cfr_current_sta_count--;
                }
                wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
                return 0;
            }
        }
    }

    wlan_objmgr_pdev_release_ref(pdev, WLAN_CFR_ID);
    return -EINVAL;
}
#endif /* WLAN_CFR_ENABLE */

static inline bool ol_mgmt_fill_tx_params(struct ieee80211com *ic,
        uint32_t vdev_id, wbuf_t wbuf, struct tx_send_params *params,
        bool offchan)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    bool data_frame = false;
    A_UINT8 retry, power, subtype_index, tid;
    A_UINT8 nss, preamble;
    uint32_t mcs = 0, rate;
    uint8_t tx_beamforming = 0;
    struct ieee80211vap *vap;
    bool fill_tx_params = false;


    vap = ol_ath_vap_get(scn, vdev_id);
    if (vap == NULL) {
        qdf_print("vap NULL");
        return false;
    }

    qdf_mem_zero(params, sizeof(*params));
    subtype_index = ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) >> IEEE80211_FC0_SUBTYPE_SHIFT);

    if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_DATA)
        data_frame = true;

    params->frame_type = data_frame;

    wbuf_get_tx_beamforming(wbuf, &tx_beamforming);
    params->en_beamforming = tx_beamforming;
    wbuf_get_tx_ctrl(wbuf, &retry, &power, NULL);
    wbuf_get_tx_rate(wbuf, &rate);
    tid = wbuf_get_tid(wbuf);

    if (offchan || (tid == EXT_TID_NONPAUSE)) {
        {
            if(ic->ic_he_target) {
                mcs = rate & RATECODE_V1_RIX_MASK;
                preamble = (rate >> PREAMBLE_OFFSET_IN_V1_RC) &
                            RATECODE_V1_PREAMBLE_MASK;
                nss = (rate >> RATECODE_V1_NSS_OFFSET) & RATECODE_V1_NSS_MASK;
            }
            else {
                mcs = rate & RATECODE_LEGACY_RIX_MASK;
                preamble = (rate >> PREAMBLE_OFFSET_IN_LEGACY_RC) &
                            RATECODE_LEGACY_PREAMBLE_MASK;
                nss = (rate >> RATECODE_LEGACY_NSS_OFFSET) & NSS_MASK_IN_LEGACY_RC;
            }

            if ((preamble == WMI_HOST_RATE_PREAMBLE_OFDM) ||
                (preamble == WMI_HOST_RATE_PREAMBLE_CCK)) {
                nss = 1;
            }

#define MGMT_MAX_CCK_RATES 4
#define MGMT_MAX_OFDM_RATES 8
            if (preamble == WMI_HOST_RATE_PREAMBLE_CCK) {
                uint8_t cck_bit_pos[MGMT_MAX_CCK_RATES] = {3, 2, 1, 0};

                if (mcs < MGMT_MAX_CCK_RATES)
                    mcs = (0x1 << cck_bit_pos[mcs]);
                else
                    mcs = 0;
            }

            if (preamble == WMI_HOST_RATE_PREAMBLE_OFDM) {
                uint8_t ofdm_bit_pos[MGMT_MAX_OFDM_RATES] =
                                            {10, 8, 6, 4, 11, 9, 7, 5};

                if (mcs < MGMT_MAX_OFDM_RATES)
                    mcs = (0x1 << ofdm_bit_pos[mcs]);
                else
                    mcs = 0;
            }

            params->mcs_mask = (mcs & 0xFFF);
            if (nss)
                params->nss_mask = (0x1 << (nss - 1));
            params->preamble_type = (0x1 << preamble);
        }

        if (power) {
            params->pwr = power & 0xFF;
        }

        if (retry == 0)
            retry = 1;

        params->retry_limit = retry & 0xF;
        params->retry_limit_ext = (retry & 0x70) >> 4;
        fill_tx_params = true;
        offchan_debug("power - %d mcs - %d nss - %d retry - %d preamble - 0x%x frame - %d chain_mask - 0x%x bw_mask - 0x%x",
            params->pwr, params->mcs_mask, params->nss_mask,
            params->retry_limit, params->preamble_type, params->frame_type,
            params->chain_mask, params->bw_mask);
    } else {
        /* Only power populated */
        if (subtype_index <= (IEEE80211_FC0_SUBTYPE_DEAUTH >> IEEE80211_FC0_SUBTYPE_SHIFT)) {
            if(vap->iv_txpow_mgt_frm[subtype_index] != 0xff) {
                params->pwr = vap->iv_txpow_mgt_frm[subtype_index];
                fill_tx_params = true;
            }
        }
    }

    /* Only CFR enable populated */
#if WLAN_CFR_ENABLE
    if (!wlan_cfr_is_feature_disabled(ic->ic_pdev_obj) && ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PROBE_RESP)) {
        if (client_is_in_cfr_unassoc_pool(ic->ic_pdev_obj, wh->i_addr1) == 0) {
            params->cfr_enable = 1;
            fill_tx_params = true;
            cfr_debug("cfr_enable = %d | fill_tx_params = %s",
                      params->cfr_enable, fill_tx_params?"true":"false");
        }
    }
#endif

    if (((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) ==
         IEEE80211_FC0_SUBTYPE_PROBE_RESP) && (scn->sc_presp_retry_limit)) {
      /* max retry limit which can be set is 0x7f */
        params->retry_limit = scn->sc_presp_retry_limit & RETRY_LIMIT_MASK;
        params->retry_limit_ext = (scn->sc_presp_retry_limit >>
                                   RETRY_LIMIT_EXT_SHIFT) &
                                  RETRY_LIMIT_EXT_MASK;
        fill_tx_params = true;
        qdf_debug("\nProberesp retry limit set. user=%x retry=%x retry_ext=%x",
                  scn->sc_presp_retry_limit, params->retry_limit,
                  params->retry_limit_ext);
        qdf_debug("fill_tx_params=%d\n",fill_tx_params);
    }

    params->chain_mask = 0;
    params->bw_mask = 0;

    ol_ath_release_vap(vap);

    return fill_tx_params;
}

int32_t ol_mgmt_offchan_tx(struct ieee80211com *ic, uint32_t vdev_id, wbuf_t wbuf)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wmi_offchan_data_tx_params param;
    struct wmi_unified *pdev_wmi_handle;
    struct ieee80211vap *vap;

    pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
    if (!pdev_wmi_handle) {
        offchan_err("null pdev_wmi_handle");
        return -1;
    }
    vdev = wlan_objmgr_get_vdev_by_id_from_pdev(ic->ic_pdev_obj, vdev_id, WLAN_OFFCHAN_TXRX_ID);
    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vdev || !vap) {
        if (vdev)
            wlan_objmgr_vdev_release_ref(vdev, WLAN_OFFCHAN_TXRX_ID);
        return -1;
    }

    qdf_mem_zero(&param, sizeof(param));

    param.tx_params_valid = ol_mgmt_fill_tx_params(ic, vdev_id, wbuf, &param.tx_param, true);

    param.qdf_ctx = scn->soc->qdf_dev;
    param.vdev_id = vdev_id;
    param.desc_id = wbuf_get_txrx_desc_id(wbuf);
#ifdef WLAN_FEATURE_11BE_MLO
    param.chanfreq = vap->iv_ml_prbreq_info.scan_freq ? vap->iv_ml_prbreq_info.scan_freq : 0;
#else
    param.chanfreq = 0;
#endif
    param.tx_frame = wbuf;
    param.frm_len = qdf_nbuf_len(wbuf);
    param.pdata = qdf_nbuf_data(wbuf);

    offchan_debug("Send offchan frame");
    if (wmi_offchan_data_tx_cmd_send(pdev_wmi_handle, &param)) {
        offchan_err("Failed to send offchan Tx");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_OFFCHAN_TXRX_ID);
        return -1;
    }
#ifdef QCA_SUPPORT_CP_STATS
    pdev_cp_stats_wmi_tx_mgmt_inc(ic->ic_pdev_obj, 1);
#endif

    wlan_objmgr_vdev_release_ref(vdev, WLAN_OFFCHAN_TXRX_ID);
    return 0;
}

int32_t ol_mgmt_send(struct ieee80211com *ic, uint32_t vdev_id, wbuf_t wbuf)
{
	struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
	struct wmi_mgmt_params param;
	struct wmi_unified *pdev_wmi_handle;
	struct ieee80211vap *vap;

	if (!qdf_nbuf_len(wbuf)) {
		qdf_err("%s: frame len is zero", __func__);
		return -1;
	}

	pdev_wmi_handle = lmac_get_pdev_wmi_handle(scn->sc_pdev);
	if (!pdev_wmi_handle ||
            (wmi_is_blocked(pdev_wmi_handle) &&
			(scn->soc->hw_mode_ctx.dynamic_hw_mode ==
			 WMI_HOST_DYNAMIC_HW_MODE_FAST))) {
		return -1;
	}

	qdf_mem_zero(&param, sizeof(param));

	param.tx_params_valid = ol_mgmt_fill_tx_params(ic, vdev_id, wbuf, &param.tx_param, false);

	param.qdf_ctx = scn->soc->qdf_dev;
	param.vdev_id = vdev_id;
	param.desc_id = wbuf_get_txrx_desc_id(wbuf);
	param.chanfreq = 0;
	param.tx_frame = wbuf;
	param.frm_len = qdf_nbuf_len(wbuf);
	param.pdata = qdf_nbuf_data(wbuf);
	param.tx_flags = wbuf_is_incorrect_pmf_key(wbuf);
	if (wbuf_is_psbuffer(wbuf)) {
	        vap = ol_ath_vap_get(scn, vdev_id);
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACTION,"setting WMI_TX_SEND_FLAG_SET_PS_BUFFERING_FOR_SA_QUERY");
		param.tx_flags |= WMI_TX_SEND_FLAG_SET_PS_BUFFERING_FOR_SA_QUERY;
		ol_ath_release_vap(vap);
	}

	if(wbuf_is_saquery(wbuf)) {
		param.tx_flags |= WMI_TX_SEND_FLAG_SET_IS_SA_QUERY;
	}

	if (wbuf_is_link_agno_tx(wbuf))
		param.mlo_link_agnostic = 1;

	if(wbuf_is_incorrect_pmf_key(wbuf)) {
		qdf_info("%s: Sending incorrect key frame", __func__);
	}

	if (wmi_mgmt_unified_cmd_send(pdev_wmi_handle, &param)) {
		qdf_print("%s: Failed to send mgmt Tx", __func__);
		return -1;
	}
#ifdef QCA_SUPPORT_CP_STATS
        pdev_cp_stats_wmi_tx_mgmt_inc(ic->ic_pdev_obj, 1);
#endif
	return 0;
}

static bool mgmt_send_check_probe_rsp_for_throttling(struct ieee80211com *ic,
        wbuf_t wbuf, struct ieee80211_node *ni)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    struct ieee80211vap *vap = ni->ni_vap;

    switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) {
    case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
        {
            u_int64_t adjusted_tsf_le;
            uint64_t tsf_adj;

            if(qdf_atomic_read(&scn->mgmt_ctx.mgmt_pending_completions) >
                        scn->mgmt_ctx.mgmt_pending_probe_resp_threshold) {
                return true;
            }
            ucfg_wlan_vdev_mgr_get_tsf_adjust(vap->vdev_obj,
                    &tsf_adj);
            /*
             * Make the TSF offset negative to match TSF in beacons
             */
            adjusted_tsf_le = cpu_to_le64(0ULL -tsf_adj);

            OS_MEMCPY(&wh[1], &adjusted_tsf_le, sizeof(adjusted_tsf_le));
        }
        break;
    }

    return false;
}
/*
 * Send Mgmt frames via WMI
 */
int
ol_ath_tx_mgmt_wmi_send(struct ieee80211com *ic, wbuf_t wbuf,
                        struct ieee80211_node *ni, void *mgmt_tx_params)
{

    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    int ret = 0;
    uint32_t offchan_tx = 0;
    int subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

    if (mgmt_tx_params)
        offchan_tx = *((uint32_t*)mgmt_tx_params);


    if (IEEE80211_ADDR_IS_VALID(vap->mcast_encrypt_addr)){
        if (IEEE80211_ADDR_EQ(wh->i_addr1,vap->mcast_encrypt_addr)) {
            wh->i_fc[1] |= IEEE80211_FC1_WEP;
        }
    }


    if ((qdf_atomic_read(&scn->mgmt_ctx.mgmt_pending_completions) >=
                scn->mgmt_ctx.mgmt_pending_max) ||
            mgmt_send_check_probe_rsp_for_throttling(ic, wbuf, ni)) {

        if (offchan_tx)
            return -ENOMEM;

        qdf_spin_lock_bh(&scn->mgmt_ctx.mgmt_backlog_queue_lock);
        /* Limit probe response not to fill more than 3/4 of mgmt
         * backlog queue. This will help prioritize other mgmt
         * in max multi vap scenario.
         */
        if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP ) {
            if (qdf_nbuf_queue_len(&scn->mgmt_ctx.mgmt_backlog_queue) >
               ((MGMT_DESC_POOL_MAX * 3) / 4)) {
                qdf_spin_unlock_bh(&scn->mgmt_ctx.mgmt_backlog_queue_lock);
                return -ENOMEM;
            }
        }
        else if (qdf_nbuf_queue_len(&scn->mgmt_ctx.mgmt_backlog_queue) >
                MGMT_DESC_POOL_MAX) {
            qdf_spin_unlock_bh(&scn->mgmt_ctx.mgmt_backlog_queue_lock);
            return -ENOMEM;
        }
        qdf_nbuf_queue_add(&scn->mgmt_ctx.mgmt_backlog_queue, wbuf);
        qdf_spin_unlock_bh(&scn->mgmt_ctx.mgmt_backlog_queue_lock);

        return 0;
    }

    /*Encap crypto header and trailer
     *Needed in case of shared WEP authentication
     */
    if (mgmt_crypto_encap(wbuf, ni) < 0)
        return -1;

    mgmt_txrx_debug("frame subtype: %d, ni: 0x%pK, mac: %s, refcnt: %d\n",
            wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK,
            ni, ether_sprintf(ni->ni_macaddr), wlan_objmgr_node_refcnt(ni));

    if (!wlan_psoc_nif_fw_ext_cap_get(scn->soc->psoc_obj,
                                  WLAN_SOC_CEXT_WMI_MGMT_REF)) {
          ret = cdp_mgmt_send(wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(ic->ic_pdev_obj)),
                                wlan_vdev_get_id(vap->vdev_obj),
                 wbuf, OL_TXRX_MGMT_TYPE_BASE);
    } else {
        if (offchan_tx) {
            if(qdf_atomic_read(&scn->mgmt_ctx.mgmt_pending_completions) <
                    scn->mgmt_ctx.mgmt_pending_max)
                ret = ol_mgmt_offchan_tx(ic, wlan_vdev_get_id(vap->vdev_obj), wbuf);
            else
                return -ENOMEM;
        } else
            ret = ol_mgmt_send(ic, wlan_vdev_get_id(vap->vdev_obj), wbuf);

        if (!ret) {
            vap->wmi_tx_mgmt++;
            if (ni != vap->iv_bss)
                vap->wmi_tx_mgmt_sta++;
        }

    }

    if (!ret)
        qdf_atomic_inc(&scn->mgmt_ctx.mgmt_pending_completions);

    return ret;
}

/*
 * This API gets invoked from the mgmt_txrx layer
 * to hand over the Tx frames to lower layer
 */
QDF_STATUS ol_if_mgmt_send (struct wlan_objmgr_vdev *vdev,
                            qdf_nbuf_t nbuf, u_int32_t desc_id,
                            void *mgmt_tx_params)
{
    struct wlan_objmgr_pdev *pdev;
    struct pdev_osif_priv *osif_priv;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;
    struct ieee80211_node *ni;
    QDF_STATUS status;

    if (vdev == NULL) {
        return QDF_STATUS_E_FAILURE;
    }

    pdev = wlan_vdev_get_pdev(vdev);

    if (pdev == NULL) {
        return QDF_STATUS_E_FAILURE;
    }

    osif_priv = wlan_pdev_get_ospriv(pdev);

    scn = (struct ol_ath_softc_net80211 *)(osif_priv->legacy_osif_priv);
    ic = &scn->sc_ic;
    ni = wlan_wbuf_get_peer_node(nbuf);
    if (ni == NULL) {
        return QDF_STATUS_E_FAILURE;
    }
    /* It is to check, whether ni is in logically deleted state */
    if (!ieee80211_try_ref_node(ni, WLAN_MLME_OBJMGR_ID)) {
        qdf_debug("peer is in log-del state Drop mgmt");
        return QDF_STATUS_E_FAILURE;
    }

    /*
     * Store mgmt_txrx_desc_id in cb
     */
    wbuf_set_txrx_desc_id(nbuf, desc_id);
    status = ol_ath_tx_mgmt_wmi_send(ic, nbuf, ni, mgmt_tx_params);
    if (status) {
        /* restore back peer in cb->peer_desc.peer before
         * returning back to umac layer.
         */
        wlan_wbuf_set_peer_node(nbuf, ni);
    }

    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
    return status;
}

static inline void ol_if_process_rx_invalid(struct ol_ath_softc_net80211 *scn, enum WDI_EVENT event,
        void *data, u_int16_t peer_id)
{
    #define MIN_DEAUTH_INTERVAL 10 /* in msec */
    struct ieee80211com *ic = &scn->sc_ic;
    struct wdi_event_rx_peer_invalid_msg *msg = (struct wdi_event_rx_peer_invalid_msg *)data;
    qdf_nbuf_t msdu = msg->msdu;
    struct ieee80211_frame *wh = msg->wh;
    u_int8_t vdev_id = msg->vdev_id;
    struct ieee80211vap *vap;
    struct mgmt_rx_event_params rx_event;
    struct ieee80211_rx_status rs = { 0 };
    wbuf_t wbuf;
    int wbuf_len;
    qdf_time_t now = 0,elasped_time = 0,max;
    struct ieee80211_node *ni;
    osif_dev *osdev;

    vap = ol_ath_vap_get(scn, vdev_id);
    if (vap == NULL) {
        /* No active vap */
        return;
    }

    osdev = (osif_dev *)vap->iv_ifp;
    if (osdev == NULL) {
        ol_ath_release_vap(vap);
        return;
    }

    if (osdev->is_delete_in_progress) {
        ol_ath_release_vap(vap);
        qdf_rl_err("%s: vap %pK delete in progress", __func__, vap);
        return;
    }

    max = (qdf_time_t)(-1); /*Max value 0xffffffffffffffff*/

    now = qdf_system_ticks_to_msecs(qdf_system_ticks());
    /* Wrap around condition */
    if(now < scn->scn_last_peer_invalid_time) {
        elasped_time = qdf_system_ticks_to_msecs(max) - scn->scn_last_peer_invalid_time + now + 1;
    } else
        elasped_time = now - scn->scn_last_peer_invalid_time;

    if(((elasped_time < MIN_DEAUTH_INTERVAL) &&
                scn->scn_last_peer_invalid_time) &&
            scn->scn_peer_invalid_cnt >= scn->scn_user_peer_invalid_cnt) {
        ol_ath_release_vap(vap);
        return;
    }
    if(elasped_time >= MIN_DEAUTH_INTERVAL)
        scn->scn_peer_invalid_cnt = 0;

    scn->scn_last_peer_invalid_time = qdf_system_ticks_to_msecs(qdf_system_ticks());

    scn->scn_peer_invalid_cnt++;

    /* Some times host gets peer_invalid frames
	 * for already associated node
	 * Not sending such frames to UMAC if the node
	 * is already associated
	 * to prevent UMAC sending Deauth to such associated
	 * nodes.
	 */
    ni = ieee80211_find_rxnode(ic, (struct ieee80211_frame_min *)wh, WLAN_MGMT_RX_ID);
    if (ni != NULL) {
       ieee80211_free_node(ni, WLAN_MGMT_RX_ID);
       ol_ath_release_vap(vap);
       return;
    }

    ni = ieee80211_try_ref_bss_node(vap, WLAN_MGMT_RX_ID);
    if (ni == NULL) {
       /* BSS node is already got deleted */
       ol_ath_release_vap(vap);
       return;
    }

    if (qdf_nbuf_len(msdu) < sizeof(struct ethernet_hdr_t)) {
        qdf_info("Invalid msdu len");
        goto done;
    }

    /* the msdu is already encapped with eth hdr */
    wbuf_len = qdf_nbuf_len(msdu) + sizeof(struct ieee80211_frame) - sizeof(struct ethernet_hdr_t);

    wbuf =  wbuf_alloc(ic->ic_osdev, WBUF_RX_INTERNAL, wbuf_len);
    if (wbuf == NULL) {
        qdf_err("%s: wbuf alloc failed", __func__);
        goto done; /* to free bss node ref */
    }
    wbuf_init(wbuf, wbuf_len);
    OS_MEMCPY(wbuf_header(wbuf), wh, sizeof(struct ieee80211_frame));
    OS_MEMCPY(wbuf_header(wbuf) + sizeof(struct ieee80211_frame),
              qdf_nbuf_data(msdu) + sizeof(struct ethernet_hdr_t),
              wbuf_len - sizeof(struct ieee80211_frame));
    OS_MEMZERO(&rx_event, sizeof(rx_event));
    /* we received this message because there is no entry for the peer in the key table */
    rx_event.status |= WMI_HOST_RXERR_KEY_CACHE_MISS;
    ol_ath_rxstat2ieee(ic, &rx_event, &rs);
    if (qdf_unlikely(wh->i_fc[1] &  IEEE80211_FC1_WEP)) {
        if (IEEE80211_ADDR_IS_VALID(vap->mcast_encrypt_addr)) {
            if (IEEE80211_ADDR_EQ(wh->i_addr1,vap->mcast_encrypt_addr)) {
                struct wlan_objmgr_pdev *pdev;
                struct wlan_objmgr_psoc *psoc;
                struct wlan_lmac_if_crypto_rx_ops *crypto_rx_ops;
                int status = 0;
                pdev = ic->ic_pdev_obj;
                psoc = wlan_pdev_get_psoc(pdev);
                crypto_rx_ops = wlan_crypto_get_crypto_rx_ops(psoc);
                if (crypto_rx_ops && WLAN_CRYPTO_RX_OPS_DECAP(crypto_rx_ops)) {
                    status = WLAN_CRYPTO_RX_OPS_DECAP(crypto_rx_ops)(vap->vdev_obj,
                                   wbuf, vap->mcast_encrypt_addr, 0);
                    if (status  == QDF_STATUS_SUCCESS) {
                        wh = (struct ieee80211_frame *)wbuf_header(wbuf);
                        wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
                    }
                }

            }
        }
    }
    ieee80211_input(ni, wbuf, &rs, NULL);

done:
    ieee80211_free_node(ni, WLAN_MGMT_RX_ID);
    ol_ath_release_vap(vap);
#undef MIN_DEAUTH_INTERVAL
}

#if WDI_EVENT_ENABLE
void rx_peer_invalid(void *pdev, enum WDI_EVENT event, void *data, u_int16_t peer_id, enum htt_cmn_rx_status status)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)pdev;

    ol_if_process_rx_invalid(scn, event, data, peer_id);
}

void htt_stats_callback(void *pdev, enum WDI_EVENT event, void *data, u_int16_t data_len, enum htt_cmn_rx_status status)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;

    scn = (struct ol_ath_softc_net80211 *) pdev;

    ic = &scn->sc_ic;

    wlan_cfg80211_wifi_fwstats_event(ic, data, data_len);
}

void hmwds_ast_add_status_cb(void *pdev, enum WDI_EVENT event,
                            void *data, u_int16_t data_len,
                            uint32_t status)
{
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;

    scn = (struct ol_ath_softc_net80211 *) pdev;

    ic = &scn->sc_ic;

    wlan_cfg80211_hmwds_ast_add_status_event(ic, data, data_len);
}
#endif

void rx_dp_peer_invalid(void *scn_handle, enum WDI_EVENT event, void *data, uint16_t peer_id)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)scn_handle;

    ol_if_process_rx_invalid(scn, event, data, peer_id);
}

void mgmt_send_queued_frames(struct ol_ath_softc_net80211 *scn)
{
    qdf_nbuf_t wbuf = NULL;
    struct ieee80211_node *ni;
    struct ieee80211com *ic = &scn->sc_ic;
    u_int32_t mgmt_txrx_desc_id = 0;
    struct wlan_objmgr_peer *peer;
    struct ieee80211vap *vap;

retry:

    qdf_spin_lock_bh(&scn->mgmt_ctx.mgmt_backlog_queue_lock);
    if (!qdf_nbuf_is_queue_empty(&scn->mgmt_ctx.mgmt_backlog_queue)) {
        wbuf = qdf_nbuf_queue_remove(&scn->mgmt_ctx.mgmt_backlog_queue);
    }
    qdf_spin_unlock_bh(&scn->mgmt_ctx.mgmt_backlog_queue_lock);

    if (wbuf == NULL)
        return;

    mgmt_txrx_desc_id = wbuf_get_txrx_desc_id(wbuf);
    peer = mgmt_txrx_get_peer(scn->sc_pdev, mgmt_txrx_desc_id);
    if (peer == NULL) {
        qdf_err("peer is null");
        return;
    }

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (ni == NULL) {
        qdf_err("ni is NULL. Drop mgmt");
        goto drop;
    }

    /* It is to check, whether ni is in logically deleted state */
    if (!ieee80211_try_ref_node(ni, WLAN_MLME_OBJMGR_ID)) {
        qdf_debug("peer is in log-del state Drop mgmt");
        goto drop;
    }
    vap = ni->ni_vap;

    /* It is to check, whether ni is in logically deleted state,
       no harm in releasing here, peer should be protected with other MGMT refs,
       to avoid handling in all negative scnearios, releasing ref here
    */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

    if (mgmt_send_check_probe_rsp_for_throttling(ic, wbuf, ni)) {
        /* This is probe response and still needs throttling */
        if(qdf_nbuf_queue_len(&scn->mgmt_ctx.mgmt_backlog_queue) > MGMT_DESC_POOL_MAX/2) {
            goto drop;
        }

    }
    if (mgmt_crypto_encap(wbuf, ni) < 0)
        goto drop;

    if (wlan_psoc_nif_fw_ext_cap_get(scn->soc->psoc_obj,WLAN_SOC_CEXT_WMI_MGMT_REF)) {
        if (ol_mgmt_send(ic, wlan_vdev_get_id(vap->vdev_obj), wbuf) == 0) {
            qdf_atomic_inc(&scn->mgmt_ctx.mgmt_pending_completions);
            vap->wmi_tx_mgmt++;
            if (ni != vap->iv_bss)
                vap->wmi_tx_mgmt_sta++;
            return;
        }
    } else {
        if (cdp_mgmt_send(wlan_psoc_get_dp_handle(wlan_pdev_get_psoc(ic->ic_pdev_obj)),
                                                  wlan_vdev_get_id(vap->vdev_obj),
                                                  wbuf, OL_TXRX_MGMT_TYPE_BASE) == 0) {
            qdf_atomic_inc(&scn->mgmt_ctx.mgmt_pending_completions);
            return;
        }
    }

drop:
    {
        struct ieee80211_tx_status ts;

        wbuf_set_peer(wbuf, peer);
        ts.ts_flags = IEEE80211_TX_ERROR;
        ts.ts_retries = 0;
        mgmt_txrx_tx_completion_handler(scn->sc_pdev,
                mgmt_txrx_desc_id,
                IEEE80211_TX_ERROR,
                (void *)&ts);
    }
    wbuf = NULL;
    goto retry;

}

static void
ol_mgmt_tx_completion_update_vdev_mgmt_stats(struct ol_ath_softc_net80211 *scn, wbuf_t wbuf)
{
    struct wlan_objmgr_peer *peer;
    struct ieee80211_node *ni;
    uint32_t mgmt_txrx_desc_id = 0;
    struct ieee80211vap *vap;

    mgmt_txrx_desc_id = wbuf_get_txrx_desc_id(wbuf);
    peer = mgmt_txrx_get_peer(scn->sc_pdev, mgmt_txrx_desc_id);
    if (!peer) {
        qdf_print("%s: null peer for desc id %d", __func__, mgmt_txrx_desc_id);
        qdf_assert_always(0);
        return;
    }
    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        qdf_print("%s: null ni for peer: 0x%pK", __func__, peer);
        qdf_assert_always(0);
        return;
    }

    vap = ni->ni_vap;
    if (vap) {
        if (ni != vap->iv_bss)
            vap->wmi_tx_mgmt_completions_sta++;
    } else {
        qdf_print("%s: null vap for ni: 0x%pK peer: 0x%pK mac:%s",
                  __func__, ni, peer, ether_sprintf(ni->ni_macaddr));
    }
}

/*
 * Management related attach functions for offload solutions
 */
void
ol_ath_mgmt_tx_complete(void *ctxt, wbuf_t wbuf, int err)
{
    struct wlan_objmgr_pdev *pdev;
    struct ol_ath_softc_net80211 *scn = NULL;
    struct wlan_objmgr_peer *peer;
    struct ieee80211_tx_status ts;
    u_int32_t mgmt_txrx_desc_id = 0;
    uint32_t cleanup_flag = 0;
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    int subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    struct ieee80211_node *ni;

    pdev = ctxt;
    scn = lmac_get_pdev_feature_ptr(pdev);
    if (scn == NULL) {
        wbuf_complete_any(wbuf);
        return;
    }

    mgmt_txrx_desc_id = wbuf_get_txrx_desc_id(wbuf);

    if (err & IEEE80211_SKB_FREE_ONLY) {
        wbuf_complete_any(wbuf);
        return;
    } else if (err & IEEE80211_TX_ERROR_NO_SKB_FREE) {
        cleanup_flag = IEEE80211_TX_ERROR_NO_SKB_FREE;
    }

    if (!err) {
        ts.ts_flags = 0;
    } else {
        if (err == 3) { /* 1 = drop due to wal resoure, 3 = retry  */
            ts.ts_flags = IEEE80211_TX_XRETRY;
        } else {
            ts.ts_flags = IEEE80211_TX_ERROR;
        }
    }
    ts.ts_flags |= cleanup_flag;
    ts.ts_retries=0;

    peer = mgmt_txrx_get_peer(pdev, mgmt_txrx_desc_id);
    QDF_ASSERT(peer != NULL);
    wbuf_set_peer(wbuf, peer);

    KASSERT((wlan_wbuf_get_peer_node(wbuf) != NULL),("ni can not be null"));

    ni = wlan_wbuf_get_peer_node(wbuf);
    if (!ni) {
        qdf_print("%s: ni can not be null", __func__);
        qdf_assert_always(ni);
        return;
    }
    KASSERT((ni->ni_vap != NULL),("vap can not be null"));

    if ((subtype == IEEE80211_FC0_SUBTYPE_ASSOC_RESP) ||
        (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)) {
        ni->ni_last_assoc_rx_time = 0;
    }

    ni->ni_vap->wmi_tx_mgmt_completions++;
    mgmt_txrx_tx_completion_handler(pdev, mgmt_txrx_desc_id, err, (void *)&ts);
    /* Check for frames in backlog queue and send it */
    mgmt_send_queued_frames(scn);

}

static int
ol_ath_mgmt_tx_completion_event_handler(ol_scn_t sc, uint8_t *data,
				uint32_t datalen)
{
	ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
	wmi_host_mgmt_tx_compl_event cmpl_params;
	struct ol_ath_softc_net80211 *scn;
	qdf_nbuf_t nbuf;
	struct ieee80211com *ic;
	uint32_t *skb_ptr;
	struct wmi_unified *wmi_handle;
	struct wlan_objmgr_pdev *pdev;
        struct cdp_tx_mgmt_comp_info *tx_cap_mgmt;

	wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
	if (!wmi_handle) {
		qdf_err("wmi_handle is null");
		return -EINVAL;
	}

	if (wmi_extract_mgmt_tx_compl_param(wmi_handle, data,
				&cmpl_params) < 0) {
		qdf_print("Failed wmi extract tx comp");
		return -1;
	}
        pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj, PDEV_UNIT(
                                     cmpl_params.pdev_id), WLAN_MLME_SB_ID);
        if (pdev == NULL) {
             qdf_print("%s: pdev object (id: %d) is NULL", __func__,
                       PDEV_UNIT(cmpl_params.pdev_id));
             return -1;
        }

        scn = lmac_get_pdev_feature_ptr(pdev);
        if (scn == NULL) {
             qdf_print("%s: scn(id: %d) is NULL", __func__,
                       PDEV_UNIT(cmpl_params.pdev_id));
             wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
             return -1;
        }

	nbuf = mgmt_txrx_get_nbuf(scn->sc_pdev, cmpl_params.desc_id);
	ic = &scn->sc_ic;
    /* When dynamic mode switch is enabled, mgmt tx completion event might
     * arrive late with previous pdev_id right after mode switch. So if nbuf
     * is not found from the given pdev, let's try to search it from other
     * pdevs
     */
    if ((soc->hw_mode_ctx.dynamic_hw_mode == WMI_HOST_DYNAMIC_HW_MODE_FAST) &&
        !nbuf) {
        struct wlan_objmgr_psoc *psoc = soc->psoc_obj;
        struct wlan_objmgr_pdev *temp_pdev;
        qdf_nbuf_t temp_nbuf;
        int pdev_idx = lmac_get_pdev_idx(pdev);
        int i;

        for (i = 0; i < WMI_HOST_MAX_PDEV; i++) {
            temp_pdev = wlan_objmgr_get_pdev_by_id(psoc, i, WLAN_MLME_SB_ID);
            if (temp_pdev == NULL)
                continue;
            pdev_idx = lmac_get_pdev_idx(temp_pdev);
            temp_nbuf = mgmt_txrx_get_nbuf(temp_pdev, cmpl_params.desc_id);
            if (temp_nbuf) {
                wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
                pdev = temp_pdev;
                scn = lmac_get_pdev_feature_ptr(pdev);
                ic = &scn->sc_ic;
                nbuf = temp_nbuf;
                break;
            }
            wlan_objmgr_pdev_release_ref(temp_pdev, WLAN_MLME_SB_ID);
        }
    }
	if (!nbuf) {
		qdf_atomic_dec(&scn->mgmt_ctx.mgmt_pending_completions);
#ifdef QCA_SUPPORT_CP_STATS
                pdev_cp_stats_wmi_tx_mgmt_completion_err_inc(ic->ic_pdev_obj, 1);
#endif
		qdf_print("Wbuf not found for Desc_id");
                wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
		return -1;
	}

	qdf_nbuf_unmap_single(scn->soc->qdf_dev, nbuf,
					QDF_DMA_TO_DEVICE);

        /* Clone mgmt packet to provide to upper layer */
        if ((ic->ic_debug_sniffer || ic->ic_tx_pkt_capture) &&
	    ic->ic_monitor_version == MONITOR_VERSION_1) {
            ol_txrx_soc_handle soc_txrx_handle =
	            wlan_psoc_get_dp_handle(soc->psoc_obj);
            qdf_nbuf_t mgmt_frm_cpy = NULL;

            if (ic->ic_tx_pkt_capture) {
                mgmt_frm_cpy = qdf_nbuf_copy_expand(nbuf,
                                    sizeof(struct cdp_tx_mgmt_comp_info), 0);
	    } else {
                mgmt_frm_cpy = qdf_nbuf_copy_expand(nbuf,
                                    sizeof(cmpl_params.ppdu_id), 0);
            }

            if (mgmt_frm_cpy) {
                if (ic->ic_tx_pkt_capture) {
		    skb_ptr = (uint32_t *)qdf_nbuf_push_head(mgmt_frm_cpy,
                                        sizeof(struct cdp_tx_mgmt_comp_info));
                    tx_cap_mgmt = (struct cdp_tx_mgmt_comp_info *)skb_ptr;
                    tx_cap_mgmt->ppdu_id = cmpl_params.ppdu_id;
                    tx_cap_mgmt->is_sgen_pkt = false;
                    tx_cap_mgmt->retries_count = cmpl_params.retries_count;
                    tx_cap_mgmt->tx_tsf = cmpl_params.tx_tsf;
                } else {
		    skb_ptr = (uint32_t *)qdf_nbuf_push_head(mgmt_frm_cpy,
                                        sizeof(cmpl_params.ppdu_id));
                    *skb_ptr = cmpl_params.ppdu_id;
                }
                cdp_deliver_tx_mgmt(soc_txrx_handle,
                    wlan_objmgr_pdev_get_pdev_id(pdev), mgmt_frm_cpy);
            }
        }

	mgmt_txrx_debug("desc_id: %d, wbuf: 0x%pK\n",
		cmpl_params.desc_id, nbuf);

#ifdef QCA_SUPPORT_CP_STATS
        pdev_cp_stats_wmi_tx_mgmt_completions_inc(ic->ic_pdev_obj, 1);
#endif
	ol_mgmt_tx_completion_update_vdev_mgmt_stats(scn, nbuf);

        qdf_atomic_dec(&scn->mgmt_ctx.mgmt_pending_completions);
	ol_ath_mgmt_tx_complete(scn->sc_pdev, nbuf,
			cmpl_params.status);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);

	return 0;
}

static int
ol_ath_mgmt_offchan_tx_completion_event_handler(ol_scn_t sc, uint8_t *data,
				uint32_t datalen)
{
	ol_ath_soc_softc_t *soc = (ol_ath_soc_softc_t *) sc;
	struct wmi_host_offchan_data_tx_compl_event cmpl_params;
	struct ol_ath_softc_net80211 *scn;
	qdf_nbuf_t nbuf;
	struct wmi_unified *wmi_handle;
	struct ieee80211com *ic;
	struct wlan_objmgr_pdev *pdev;

	wmi_handle = lmac_get_wmi_hdl(soc->psoc_obj);
	if (!wmi_handle) {
		qdf_err("wmi_handle is null");
		return -EINVAL;
	}

	if (wmi_extract_offchan_data_tx_compl_param(wmi_handle, data,
				&cmpl_params) < 0) {
		offchan_err("Failed tp extract tx comp");
		return -1;
	}
	offchan_debug("Desc_id = %d pdev_id = %d",
				cmpl_params.desc_id, cmpl_params.pdev_id);
        pdev = wlan_objmgr_get_pdev_by_id(soc->psoc_obj,
                         PDEV_UNIT(cmpl_params.pdev_id), WLAN_MLME_SB_ID);
        if (pdev == NULL) {
             qdf_print("%s: pdev object (id: %d) is NULL ", __func__, PDEV_UNIT(cmpl_params.pdev_id));
             return -1;
        }

        scn = lmac_get_pdev_feature_ptr(pdev);
        if (scn == NULL) {
             qdf_print("%s: scn (id: %d) is NULL ", __func__, PDEV_UNIT(cmpl_params.pdev_id));
             wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
             return -1;
        }

	nbuf = mgmt_txrx_get_nbuf(scn->sc_pdev, cmpl_params.desc_id);
	ic = &scn->sc_ic;
	if (!nbuf) {
		qdf_atomic_dec(&scn->mgmt_ctx.mgmt_pending_completions);
#ifdef QCA_SUPPORT_CP_STATS
                pdev_cp_stats_wmi_tx_mgmt_completion_err_inc(ic->ic_pdev_obj, 1);
#endif
		qdf_print("Wbuf not found for Desc_id");
                wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);
		return -1;
	}
	qdf_nbuf_unmap_single(scn->soc->qdf_dev, nbuf,
				QDF_DMA_TO_DEVICE);

#ifdef QCA_SUPPORT_CP_STATS
        pdev_cp_stats_wmi_tx_mgmt_completions_inc(ic->ic_pdev_obj, 1);
#endif
	ol_mgmt_tx_completion_update_vdev_mgmt_stats(scn, nbuf);

        qdf_atomic_dec(&scn->mgmt_ctx.mgmt_pending_completions);
	ol_ath_mgmt_tx_complete(scn->sc_pdev, nbuf,
			cmpl_params.status);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_MLME_SB_ID);

	return 0;
}

void ol_ath_wmi_over_htt_comp_handler(void *ctxt, wbuf_t wbuf, int err)
{
    struct wlan_objmgr_pdev *pdev;
    struct ol_ath_softc_net80211 *scn = NULL;

    pdev = ctxt;
    scn = lmac_get_pdev_feature_ptr(pdev);
    if (scn == NULL) {
        wbuf_complete_any(wbuf);
        return;
    }
    qdf_atomic_dec(&scn->mgmt_ctx.mgmt_pending_completions);
    ol_ath_mgmt_tx_complete(ctxt, wbuf, err);
}

void
ol_ath_mgmt_register_offload_beacon_tx_status_event(struct ieee80211com *ic,
                                            bool unregister) {
    struct wlan_objmgr_psoc *psoc;
    wmi_unified_t wmi_handle;

    QDF_TRACE(QDF_MODULE_ID_BSSCOLOR, QDF_TRACE_LEVEL_DEBUG,
                                            "%s>>", __func__);

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    wmi_handle = lmac_get_wmi_unified_hdl(psoc);

    if (wmi_handle) {
        if (!unregister) {
            wmi_unified_register_event_handler(wmi_handle,
                    wmi_offload_bcn_tx_status_event_id,
                    ol_ath_offload_bcn_tx_status_event_handler,
                    WMI_RX_UMAC_CTX);
        } else {
            wmi_unified_unregister_event_handler(wmi_handle,
                    wmi_offload_bcn_tx_status_event_id);
        }
    } else {
        QDF_TRACE(QDF_MODULE_ID_BSSCOLOR, QDF_TRACE_LEVEL_ERROR,
                            "%s: wmi_handle is null.", __func__);
    }

    QDF_TRACE(QDF_MODULE_ID_BSSCOLOR, QDF_TRACE_LEVEL_DEBUG,
                                            "%s<<", __func__);
}

void
ol_ath_mgmt_soc_attach(ol_ath_soc_softc_t *soc)
{
    wmi_unified_t wmi_handle;

    wmi_handle = lmac_get_wmi_unified_hdl(soc->psoc_obj);
    /* Register WMI event handlers */
    wmi_unified_register_event_handler(wmi_handle, wmi_mgmt_rx_event_id,
            ol_ath_mgmt_rx_event_handler, WMI_RX_UMAC_CTX);

    if (wlan_psoc_nif_fw_ext_cap_get(soc->psoc_obj,
                                  WLAN_SOC_CEXT_WMI_MGMT_REF)) {
        wmi_unified_register_event_handler(wmi_handle,
                wmi_mgmt_tx_completion_event_id,
                ol_ath_mgmt_tx_completion_event_handler, WMI_RX_UMAC_CTX);
        wmi_unified_register_event_handler(wmi_handle,
                wmi_offchan_data_tx_completion_event,
                ol_ath_mgmt_offchan_tx_completion_event_handler, WMI_RX_UMAC_CTX);
    }
}

static int ol_ath_subscribe_csa_interop_phy(struct ieee80211com *ic, bool subscribe)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    ol_txrx_soc_handle soc_txrx_handle = NULL;

    if (!ic) {
        return -1;
    }

    scn = OL_ATH_SOFTC_NET80211(ic);
    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);

    scn->csa_phy_update_subscriber.callback = ieee80211_csa_interop_update;
    scn->csa_phy_update_subscriber.context = ic->ic_pdev_obj;

#if WDI_EVENT_ENABLE
    if (subscribe) {
        return cdp_wdi_event_sub(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj),
                &scn->csa_phy_update_subscriber, WDI_EVENT_RX_PPDU_DESC);
    } else {
        return cdp_wdi_event_unsub(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj),
                &scn->csa_phy_update_subscriber, WDI_EVENT_RX_PPDU_DESC);
    }
#endif
    return -1;
}

void
ol_ath_mgmt_attach(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    int pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    ol_txrx_soc_handle soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    QDF_STATUS status;

    /* TODO:
     * Disable this WMI xmit logic once we have the transport ready
     * for management frames
     */
    ic->ic_newassoc = ol_ath_net80211_newassoc;
    ic->ic_addba_clearresponse = ol_ath_net80211_addba_clearresponse;
    ic->ic_addba_send = ol_ath_net80211_addba_send;
    ic->ic_delba_send = ol_ath_net80211_delba_send;
    ic->ic_addba_setresponse = ol_ath_net80211_addba_setresponse;
    ic->ic_send_singleamsdu = ol_ath_net80211_send_singleamsdu;
    ic->ic_subscribe_csa_interop_phy = ol_ath_subscribe_csa_interop_phy;


#if WDI_EVENT_ENABLE
    scn->soc->scn_rx_peer_invalid_subscriber.callback      = rx_peer_invalid;
    scn->soc->scn_rx_peer_invalid_subscriber.context       = scn;
    cdp_wdi_event_sub(soc_txrx_handle, pdev_id, &scn->soc->scn_rx_peer_invalid_subscriber, WDI_EVENT_RX_PEER_INVALID);
    scn->scn_last_peer_invalid_time = 0;
    scn->scn_peer_invalid_cnt = 0;
    scn->scn_user_peer_invalid_cnt = 1 ;/* By default we will send one deauth in 10 msec in response to rx_peer_invalid */

    scn->htt_stats_subscriber.callback = htt_stats_callback;
    scn->htt_stats_subscriber.context = scn;
    cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                      &scn->htt_stats_subscriber,
                      WDI_EVENT_HTT_STATS);

    scn->dp_stats_subscriber.callback = ol_ath_update_dp_stats;
    scn->dp_stats_subscriber.context = ic->ic_pdev_obj;
    cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
        &scn->dp_stats_subscriber, WDI_EVENT_UPDATE_DP_STATS);

    scn->hmwds_ast_add_status_subscriber.callback = hmwds_ast_add_status_cb;
    scn->hmwds_ast_add_status_subscriber.context = scn;
    cdp_wdi_event_sub(soc_txrx_handle, pdev_id,
                      &scn->hmwds_ast_add_status_subscriber,
                      WDI_EVENT_HMWDS_AST_ADD_STATUS);
#endif
    /* should always be equal to define DEFAULT_LOWEST_RATE_IN_5GHZ 0x03  6 Mbps  in firmware */
    scn->ol_rts_cts_rate = 0x03;
    qdf_nbuf_queue_init(&scn->mgmt_ctx.mgmt_backlog_queue);
    qdf_spinlock_create(&scn->mgmt_ctx.mgmt_backlog_queue_lock);
    scn->mgmt_ctx.mgmt_backlog_queue_lock_initialised = 1;
    qdf_atomic_init(&scn->mgmt_ctx.mgmt_pending_completions);
   /* register txmgmt completion call back */
    if (!wlan_psoc_nif_fw_ext_cap_get(scn->soc->psoc_obj,
                                  WLAN_SOC_CEXT_WMI_MGMT_REF)) {
        cdp_mgmt_tx_cb_set(soc_txrx_handle, pdev_id,
			    (OL_TXRX_MGMT_NUM_TYPES-1),
			    NULL,
			    ol_ath_wmi_over_htt_comp_handler,
			    scn->sc_pdev);
        scn->mgmt_ctx.mgmt_pending_max = MGMT_TARGET_SUPPORTED_MAX_HTT;
	scn->mgmt_ctx.mgmt_pending_probe_resp_threshold = (MGMT_TARGET_SUPPORTED_MAX_HTT * 3) / 4;
    } else {
	scn->mgmt_ctx.mgmt_pending_max = MGMT_TARGET_SUPPORTED_MAX_WMI;
        scn->mgmt_ctx.mgmt_pending_probe_resp_threshold = (MGMT_TARGET_SUPPORTED_MAX_WMI * 3) / 4;
    }

    /* For Dynamic WSI Remap, REO pdev attach will happen post MLO setup */
    if (!scn->soc->psoc_obj->wsi_remap_add) {
        status = wlan_mgmt_rx_reo_pdev_attach(ic->ic_pdev_obj);
        qdf_assert_always(QDF_IS_STATUS_SUCCESS(status));
    }
}

void ol_ath_mgmt_detach(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    QDF_STATUS status;

    if (wlan_psoc_nif_fw_ext_cap_get(psoc, WLAN_SOC_CEXT_WMI_MGMT_REF)) {
        if (!qdf_nbuf_is_queue_empty(&scn->mgmt_ctx.mgmt_backlog_queue)) {
            qdf_print("Mgmt frames still pending in queue");
            QDF_BUG(0);
        }
        if (scn->mgmt_ctx.mgmt_backlog_queue_lock_initialised) {
            qdf_spinlock_destroy(&scn->mgmt_ctx.mgmt_backlog_queue_lock);
            scn->mgmt_ctx.mgmt_backlog_queue_lock_initialised = 0;
        }
    }

    /* For Dynamic WSI Remap REO pdev detach will happen during teardown */
    if (!scn->soc->psoc_obj->wsi_remap_remove) {
        status = wlan_mgmt_rx_reo_pdev_detach(ic->ic_pdev_obj);
        qdf_assert_always(QDF_IS_STATUS_SUCCESS(status));
    }
}
#endif
