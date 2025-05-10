/*
 * Copyright (c) 2011-2022 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 */
/*
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <qdf_nbuf.h>         /* qdf_nbuf_t, etc. */
#include <qdf_time.h>         /* qdf_nbuf_t, etc. */
#include <ip_prot.h>

#ifdef WLAN_FEATURE_FASTPATH
#include <hif.h>              /* HIF_DEVICE */
#endif
#include <htt.h>       /* HTT_TX_EXT_TID_MGMT */
#ifdef WLAN_FEATURE_FASTPATH
#include <htt_internal.h>     /* */
#include <htt_types.h>        /* htc_endpoint */
#endif
#include <ol_htt_tx_api.h>    /* htt_tx_desc_tid */
#include <cdp_txrx_cmn_struct.h>      /* ol_txrx_vdev_handle */

#include <ol_txrx_internal.h> /* TXRX_ASSERT1 */
#include <ol_txrx_types.h>    /* pdev stats */
#include <ol_tx_desc.h>       /* ol_tx_desc */
#include <ol_tx_send.h>       /* ol_tx_send */
#include <ol_txrx_peer_find.h> /* peerf_hash_find */

#include <ieee80211_acfg.h>   /* acfg event */
#include <if_meta_hdr.h>

#if QCA_PARTNER_DIRECTLINK_TX
#define QCA_PARTNER_DIRECTLINK_OL_TX 1
#include "ath_carr_pltfrm.h"
#undef QCA_PARTNER_DIRECTLINK_OL_TX
#endif /* QCA_PARTNER_DIRECTLINK_TX */


#include <init_deinit_lmac.h>
#if ATH_DEBUG
#include "osif_private.h"
extern void set_rtscts_enable(osif_dev * osdev);
#define ATH_DEBUG_SET_RTSCTS_ENABLE(_osdev) set_rtscts_enable(_osdev)
#else
#define ATH_DEBUG_SET_RTSCTS_ENABLE(_osdev)
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include <osif_nss_wifiol_if.h>
#include <osif_nss_wifiol_vdev_if.h>
#include <osif_nss_wifiol_2_0_if.h>
#endif

#if QCA_SUPPORT_RAWMODE_PKT_SIMULATION
#include <rawsim_api_defs.h>
#endif /* QCA_SUPPORT_RAWMODE_PKT_SIMULATION */

#define MLD_REPORT      0x83
#define MLD_LEAVE       0x84
#define MLDv2_REPORT    0x8f

#undef IS_SNAP
#define IS_SNAP(_llc) ((_llc)->llc_dsap == LLC_SNAP_LSAP && \
                                (_llc)->llc_ssap == LLC_SNAP_LSAP && \
                                (_llc)->llc_control == LLC_UI)


#ifdef WLAN_FEATURE_FASTPATH
qdf_nbuf_t
ol_tx_ll(struct cdp_soc_t *soc, uint8_t vdev_id, qdf_nbuf_t msdu_list)
{
    qdf_nbuf_t msdu = msdu_list;
    /*
     * The msdu_list variable could be used instead of the msdu var,
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    msdu = qdf_nbuf_unshare(msdu);
    qdf_nbuf_free(msdu);

    return NULL;
}
#endif

QDF_STATUS
ol_txrx_mgmt_tx_cb_set(
    struct cdp_soc_t *soc, uint8_t pdev_id,
    u_int8_t type,
    ol_txrx_mgmt_tx_cb download_cb,
    ol_txrx_mgmt_tx_cb ota_ack_cb,
    void *ctxt)
{
    struct ol_txrx_pdev_t *pdev = (struct ol_txrx_pdev_t *) ((struct ol_txrx_psoc_t *)soc)->pdev[0];
    if (!pdev)
        return QDF_STATUS_E_FAILURE;

    TXRX_ASSERT1(type < OL_TXRX_MGMT_NUM_TYPES);
    qdf_print(">>>> CB Set %pK", download_cb);
    pdev->tx_mgmt.callbacks[type].download_cb = download_cb;
    pdev->tx_mgmt.callbacks[type].ota_ack_cb = ota_ack_cb;
    pdev->tx_mgmt.callbacks[type].ctxt = ctxt;

    return QDF_STATUS_SUCCESS;
}


int
ol_txrx_mgmt_send(
    struct cdp_soc_t *soc, uint8_t vdev_id,
    qdf_nbuf_t tx_mgmt_frm,
    u_int8_t type)
{
    struct ol_txrx_vdev_t *vdev;
    A_UINT8  frm_hdr[HTT_MGMT_FRM_HDR_DOWNLOAD_LEN];
    A_UINT8 *frm_buf;
    int subtype;
    A_UINT32 paddr;
    struct ieee80211_frame *wh = (struct ieee80211_frame *)wbuf_header(tx_mgmt_frm);
    struct ol_txrx_pdev_t *pdev = (struct ol_txrx_pdev_t *) ((struct ol_txrx_psoc_t *)soc)->pdev[0];

    if (!(vdev = ol_txrx_get_vdev_from_pdev(pdev, vdev_id)))
        return -1;

    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

    frm_buf = (A_UINT8 *)wbuf_header(tx_mgmt_frm);
    OS_MEMCPY(frm_hdr, frm_buf, HTT_MGMT_FRM_HDR_DOWNLOAD_LEN);

    qdf_nbuf_map_single(pdev->osdev, tx_mgmt_frm, QDF_DMA_TO_DEVICE);

    if (subtype == IEEE80211_FC0_SUBTYPE_ACTION) {
        struct ieee80211_action *ia;
        ia = (struct ieee80211_action *)&wh[1];

        if ((ia->ia_category == IEEE80211_ACTION_CAT_PUBLIC) ||
            (ia->ia_category == IEEE80211_ACTION_CAT_QOS) ||
            (ia->ia_category == IEEE80211_ACTION_CAT_PROT_DUAL) ||
            (ia->ia_category == IEEE80211_ACTION_CAT_FST) ||
            (ia->ia_category == IEEE80211_ACTION_CAT_WNM)) {
            paddr =  QDF_NBUF_CB_PADDR(tx_mgmt_frm);
        } else {
            paddr = qdf_nbuf_get_frag_paddr(tx_mgmt_frm, 0);
        }
    }
    else {
        if(subtype == IEEE80211_FC0_SUBTYPE_DISASSOC || subtype == IEEE80211_FC0_SUBTYPE_AUTH) {
            paddr = QDF_NBUF_CB_PADDR(tx_mgmt_frm);
        }
        else {
            paddr = qdf_nbuf_get_frag_paddr(tx_mgmt_frm, 0);
        }
    }
    {
         ieee80211_vap_complete_buf_handler handler;
         void *arg;
         wbuf_get_complete_handler(tx_mgmt_frm,(void **)&handler,&arg);
    }
    return htt_h2t_mgmt_tx(pdev,
                   paddr,
                   tx_mgmt_frm,
                   wbuf_get_pktlen(tx_mgmt_frm),
                   vdev->vdev_id,
                   frm_hdr,
                   (ol_txrx_vdev_handle)vdev);
}

int
ol_txrx_mgmt_send_ext(struct cdp_soc_t *soc, uint8_t vdev_id,
			 qdf_nbuf_t tx_mgmt_frm,
			 uint8_t type, uint8_t use_6mbps, uint16_t chanfreq){
    //ol_txrx_vdev_handle vdev = (ol_txrx_vdev_handle)vdev_handle; Enable when needed
    return 0;
}

void
ol_txrx_sync(struct ol_txrx_pdev_t *pdev, u_int8_t sync_cnt)
{
    htt_h2t_sync_msg(pdev->htt_pdev, sync_cnt);
}

char *pflow_ctrl_peer_stats_str[] = {
    "Enqueue_Cnt",
    "Dequeue_Cnt",
    "Dequeue_Bytecnt",
    "Enqueue_Bytecnt",
    "Dequeue_Req_bytecnt",
    "Dequeue_Req_pktcnt",
    "Peer_q_full",
    "Msdu_TTL_Expiry",
    "Hol_Cnt",
    "Peer_Tx_Desc_Fail",
};

void
pflow_ctl_display_tidq_stats(struct ol_txrx_peer_t *peer, u_int8_t tid)
{
    int i;
    for(i=0;i < PFLOW_CTRL_TIDQ_STATS_MAX;i++) {
        qdf_print("%s :  %d",pflow_ctrl_peer_stats_str[i], peer->tidq[tid].stats[i]);
        peer->tidq[tid].stats[i] = 0;
    }
    return;
}

void
ol_tx_flow_ctrl_tidq_stats(struct ol_txrx_pdev_t *pdev, u_int32_t peer_id, u_int8_t tid)
{
    struct ol_txrx_peer_t *peer;
    peer = ol_txrx_peer_find_by_id((ol_txrx_pdev_handle) pdev, peer_id);
    if (!peer) {
        qdf_print("%s peer=NULL %d", __func__, peer_id);
        return;
    }
    qdf_print("----------------------");
    qdf_print(" peer_id %d, tid %d", peer_id, tid);
    qdf_print("---------------+------");
    /*
    This byte count is actually an encoded byte count; Top 2 bits are to represent the quantum multiple that the remaining 6 bits hold.
     0 – 64 byte quantum, 1 – 512 byte quantum, 2 – 1024 byte quantum, 3 – 2048 byte quantum.
    This is sent in queue map to target firmware and hence this compression.
    */
#ifdef BIG_ENDIAN_HOST
    qdf_print("encoded_byte_cnt   | 0x%x", QUEUE_MAP_BYTE_CNT_GET(pdev,peer_id,tid));
#else
    qdf_print("encoded_byte_cnt   | 0x%x", pdev->pmap->byte_cnt[tid][peer_id]);
#endif
    qdf_print("bytes         | %u", peer->tidq[tid].byte_count);
    qdf_print("queue_len     | %d", qdf_nbuf_queue_len(&peer->tidq[tid].nbufq));
    qdf_print("max_queue_len | %d", pdev->pflow_ctl_queue_max_len[peer_id][tid]);
    qdf_print("high_watermark| %u", peer->tidq[tid].high_watermark);
    qdf_print("Pkts stay time| %llu", peer->tstampstat.sum[tid]);
    qdf_print("Msdus stayed  | %llu", peer->tstampstat.nummsdu[tid]);
    qdf_print("EWMA time diff| %llu", peer->tstampstat.avg[tid]);

    pflow_ctl_display_tidq_stats(peer, tid);
    return;
}

QDF_STATUS
ol_txrx_per_peer_stats(struct cdp_soc_t *soc, char *addr)
{
    struct ol_txrx_pdev_t *pdev = (struct ol_txrx_pdev_t *)((struct ol_txrx_psoc_t *)soc)->pdev[0];
    struct ol_txrx_ast_entry_t *ast_entry;
    u_int16_t peer_id=HTT_INVALID_PEER;
    u_int16_t tid;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    int status;
#endif
    struct ieee80211_node *ni = NULL;
    struct ol_ath_softc_net80211 *scn;
    struct ieee80211com *ic;

    if (!pdev)
        return QDF_STATUS_E_FAILURE;

    scn = (struct ol_ath_softc_net80211 *) pdev->scnctx;
    ic = &(scn->sc_ic);
    ast_entry = ol_txrx_ast_find_hash_find(pdev, addr, 1);
    if (!ast_entry) {
        return QDF_STATUS_E_FAILURE;
    }
    peer_id = ast_entry->peer_id;
    qdf_nofl_info("Found peer_id=%d\n", peer_id);

    if (peer_id == HTT_INVALID_PEER) {
        qdf_nofl_info("No peer found\n");
        return QDF_STATUS_E_FAILURE;
    }

    if (peer_id >= OL_TXRX_MAX_PEER_IDS) {
        qdf_nofl_info("Invalid Peer Id=%d\n", peer_id);
        return QDF_STATUS_E_FAILURE;
    }

    ni = ieee80211_find_node(ic, addr, WLAN_MLME_SB_ID);
    if (ni == NULL) {
        qdf_print("Node not found!");
        return QDF_STATUS_E_FAILURE;
    }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    status = osif_nss_peer_stats_frame_send((struct ol_txrx_pdev_t *) pdev, peer_id);
    if (status != 0) {
        qdf_print("NSS command send failed for peer stats ");
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
        return QDF_STATUS_E_FAILURE;
    }
#endif

    qdf_print("------- Assoc ID = %d ----------------", IEEE80211_AID(ni->ni_associd));
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    qdf_print("------- per-Peer/TIDQ Stats ----------------");
    tid = 0;
    while(tid < OL_TX_PFLOW_CTRL_MAX_TIDS) {

        ol_tx_flow_ctrl_tidq_stats(pdev, peer_id, tid);

        tid++;
    }

    qdf_print("---------------------------------");

    return QDF_STATUS_SUCCESS;
}

