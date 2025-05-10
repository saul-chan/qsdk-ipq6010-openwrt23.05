/*
 * Copyright (c) 2011-2015,2017-2022 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2015 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <qdf_nbuf.h>          /* qdf_nbuf_t, etc. */
#include <ieee80211.h>         /* ieee80211_frame */
#include <ieee80211_var.h>     /* IEEE80211_ADDR_COPY */

/* external API header files */
#include <ol_ctrl_txrx_api.h>  /* ol_rx_notify */
#include <ol_htt_api.h>        /* htt_pdev_handle */
#include <ol_txrx_htt_api.h>   /* ol_rx_indication_handler */
#include <ol_htt_rx_api.h>     /* htt_rx_peer_id, etc. */

/* internal API header files */
#include <ol_txrx_types.h>     /* ol_txrx_vdev_t, etc. */
#include <ol_txrx_peer_find.h> /* ol_txrx_peer_find_by_id */
#include <ol_if_txrx_handles.h>
#include <wdi_event.h>

#include <htt_types.h>
#include <ol_if_athvar.h>
#include <net.h>
#include <ipv4.h>
#include <ip_prot.h>
#include <ol_vowext_dbg_defs.h>
#include <dp_ratetable.h>

#ifndef REMOVE_PKT_LOG
#include <pktlog_ac_fmt.h>
#endif

#if QCA_PARTNER_DIRECTLINK_RX
#define QCA_PARTNER_DIRECTLINK_OL_RX 1
#include "ath_carr_pltfrm.h"
#undef QCA_PARTNER_DIRECTLINK_OL_RX
#endif
#include <init_deinit_lmac.h>
#if UNIFIED_SMARTANTENNA
#include <wlan_sa_api_utils_defs.h>
#include <target_if_sa_api.h>
#endif
#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif
#include <dp_rate_stats_pub.h>
#define INVALID_PREAMBLE 0xff

#include <htt_internal.h> /* htt_rx_desc */

extern void transcap_nwifi_to_8023(qdf_nbuf_t msdu);

static int ol_rx_monitor_deliver(
    struct ol_txrx_pdev_t * pdev,
    qdf_nbuf_t head_msdu,
    u_int32_t no_clone_reqd, void *rx_desc, enum htt_rx_status status,
    struct ol_txrx_peer_t *peer)
{
    qdf_nbuf_t mon_mpdu = NULL;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    struct ieee80211_rx_status *rs = pdev->rx_mon_recv_status;
    u_int8_t is_mcast = 0;
    u_int32_t mgmt_type;
    u_int32_t ctrl_type;
    u_int32_t preamble;

    if (pdev->monitor_vdev == NULL) {
        return -1;
    }

    if(qdf_unlikely(rs->rs_snr > 127)){
        rs->rs_snr = 127;
    }

    /* When smart monitor vap is configured , Only deliver
     * valid neighbour peer packets that comes from nac bss
     * peer id and filter out all other other packets
     * including valid and invalid peers
     */
     if(pdev->filter_neighbour_peers && peer
#if ATH_SUPPORT_NAC
        && peer->nac
#endif
       ){
         /* Deliver all non associated packets to smart
          * monitor vap.htt status check not needed, as
          * we might receive varied htt status and all
          * packets are passed to interface & not dropped
          */

    /*Filter based on pkt types to monitor VAP*/
     } else {
         /*data pkts*/
         is_mcast = htt_rx_msdu_is_wlan_mcast(htt_pdev, rx_desc);
         preamble = htt_rx_ppdu_wlan_preamble(htt_pdev, rx_desc);
         mgmt_type = htt_pdev->ar_rx_ops->check_desc_mgmt_type(head_msdu);
         ctrl_type = htt_pdev->ar_rx_ops->check_desc_ctrl_type(head_msdu);

         if (preamble == INVALID_PREAMBLE)
             return -1;

         if (qdf_likely((mgmt_type && ctrl_type) != 0)) {
             is_mcast = htt_rx_msdu_is_wlan_mcast(htt_pdev, rx_desc);
             if (qdf_likely(is_mcast != 1)) {
                 /*ucast data pkts*/
                 if (pdev->mon_filter_ucast_data == 1) {
                     /*drop ucast pkts*/
                     return -1;
                 }
             } else {
                 /*mcast data pkts*/
                 if (pdev->mon_filter_mcast_data == 1) {
                     /*drop mcast pkts*/
                     return -1;
                 }
             }
         } else {
             /*mgmt/ctrl etc. pkts*/
             if (pdev->mon_filter_non_data == 1) {
                 return -1;
             }
         }
    }

     /* Cloned mon MPDU for delivery via monitor intf */
     mon_mpdu = htt_pdev->ar_rx_ops->restitch_mpdu_from_msdus(
        htt_pdev->arh, head_msdu, pdev->rx_mon_recv_status, no_clone_reqd);

    if (peer) {
        rs->rs_peer_valid = 1;
    } else {
        rs->rs_peer_valid = 0;
    }

    if (mon_mpdu) {
        pdev->monitor_vdev->osif_rx_mon(
            pdev->monitor_vdev->osif_vdev, mon_mpdu, pdev->rx_mon_recv_status);
    } else if (no_clone_reqd) {
       return -1;
    }

    return 0;
}

void ol_rx_process_inv_peer(
    ol_txrx_pdev_handle pdevh,
    void *rx_mpdu_desc,
    qdf_nbuf_t msdu
    )
{
    uint8_t a1[QDF_MAC_ADDR_SIZE];
    struct ol_txrx_pdev_t *pdev = (struct ol_txrx_pdev_t *)pdevh;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    struct ol_txrx_vdev_t *vdev = NULL;
    struct ieee80211_frame *wh;
    struct wdi_event_rx_peer_invalid_msg msg;

    wh = (struct ieee80211_frame *)htt_pdev->ar_rx_ops->wifi_hdr_retrieve(rx_mpdu_desc);
    if (!IEEE80211_IS_DATA(wh))
        return;

    /* ignore frames for non-existent bssids */
    IEEE80211_ADDR_COPY(a1, wh->i_addr1);
    TAILQ_FOREACH(vdev, &pdev->vdev_list, vdev_list_elem) {
        if (qdf_mem_cmp(a1, vdev->mac_addr.raw, QDF_MAC_ADDR_SIZE) == 0) {
            break;
        }
    }
    if (!vdev)
        return;

    if (qdf_nbuf_len(msdu) < sizeof(struct ieee80211_frame)) {
        return;
    }

    msg.wh = wh;
    msg.msdu = msdu;
    msg.vdev_id = vdev->vdev_id;
    wdi_event_handler(WDI_EVENT_RX_PEER_INVALID, pdev, &msg, HTT_INVALID_PEER, WDI_NO_VAL);
}
qdf_export_symbol(ol_rx_process_inv_peer);

#ifndef REMOVE_PKT_LOG
void
phy_data_type_chk(
    qdf_nbuf_t   head_msdu,
    u_int32_t    *cv_chk_status)
{
  struct bb_tlv_pkt_hdr *bb_hdr = NULL;
  qdf_nbuf_t   next_msdu = head_msdu;

  bb_hdr    = (struct bb_tlv_pkt_hdr *)qdf_nbuf_data(next_msdu);
  if ((bb_hdr->sig == PKTLOG_PHY_BB_SIGNATURE) &&
      (bb_hdr->tag == PKTLOG_RX_IMPLICIT_CV_TRANSFER || bb_hdr->tag == PKTLOG_BEAMFORM_DEBUG_H))
  {
    *cv_chk_status = 0;
  }else{
    *cv_chk_status = 1;
  }
}
qdf_export_symbol(phy_data_type_chk);

void
cbf_type_chk(
    qdf_nbuf_t   head_msdu,
    u_int32_t    *cv_chk_status)
{
  struct ieee80211_frame *wh = NULL;
  u_int8_t  type, subtype;
  qdf_nbuf_t   next_msdu = head_msdu;

  wh = (struct ieee80211_frame *)qdf_nbuf_data(next_msdu);
  type    = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
  subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
  if((type == IEEE80211_FC0_TYPE_MGT )&&(subtype == IEEE80211_FCO_SUBTYPE_ACTION_NO_ACK))
  {
    *cv_chk_status = 0;
  }else{
    *cv_chk_status = 1;
  }
}
qdf_export_symbol(cbf_type_chk);

#endif

#if UMAC_SUPPORT_WNM
int ol_ath_wnm_update_bssidle_time( struct ol_ath_softc_net80211 *scn ,
                                struct ol_txrx_peer_t *peer,
                                htt_pdev_handle htt_pdev ,
                                void *rx_mpdu_desc )
{
    struct ieee80211com *ic = &scn->sc_ic;
    struct ieee80211_node *ni = NULL;
    int secured = 0;

    ni = ieee80211_find_node(ic, peer->mac_addr.raw, WLAN_MLME_SB_ID);
    if (ni && ni != ni->ni_bss_node) {
        secured = htt_rx_mpdu_is_encrypted(htt_pdev, rx_mpdu_desc);
        ieee80211_wnm_bssmax_updaterx(ni, secured);
    }
    if (ni) {
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
    }

    return 1;
}
#endif

static inline void
ol_rx_aggregate_bukt_stats_update(struct ol_txrx_pdev_t *pdev,
        struct ol_txrx_peer_t *peer, u_int8_t tid, int num_mpdus)
{
    if (num_mpdus < 10) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_10, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_10, 1);
    } else if (num_mpdus < 20) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_20, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_20, 1);
    } else if (num_mpdus < 30) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_30, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_30, 1);
    } else if (num_mpdus < 40) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_40, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_40, 1);
    } else if (num_mpdus < 50) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_50, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_50, 1);
    } else if (num_mpdus < 60) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_60, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_60, 1);
    } else {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AGGREGATE_MORE, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AGGREGATE_MORE, 1);
    }
}

static inline void
ol_rx_amsdu_bukt_stats_update(struct ol_txrx_pdev_t *pdev,
        struct ol_txrx_peer_t *peer, u_int8_t tid, u_int32_t npackets)
{
    if (npackets == 1) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AMSDU_1, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AMSDU_1, 1);
    } else if (npackets == 2) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AMSDU_2, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AMSDU_2, 1);
    } else if (npackets == 3) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AMSDU_3, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AMSDU_3, 1);
    } else if (npackets == 4) {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AMSDU_4, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AMSDU_4, 1);
    } else {
        PFLOW_CTRL_PDEV_VIDEO_STATS_ADD(pdev, RX_VIDEO_AMSDU_MORE, 1);
        PFLOW_TXRX_TIDQ_STATS_ADD(peer, tid, RX_AMSDU_MORE, 1);
    }
}


static void
ol_txrx_update_peer_rx_rssi(struct ol_txrx_pdev_t *txrx_pdev,
                            struct ol_txrx_peer_t *peer,
                            void *rx_desc,
                            struct cdp_rx_indication_ppdu *cdp_rx_ppdu)
{
    uint32_t *ppdu_start;
    uint32_t snr;
    uint32_t co, cm, cs, chain, ht;

    ppdu_start = txrx_pdev->htt_pdev->ar_rx_ops->get_rssi(rx_desc, &co, &cm, &cs);

    snr = (ppdu_start[co] & cm) >> cs;
    cdp_rx_ppdu->rssi = snr;

    for (chain = 0; chain < WLANSTATS_MAX_CHAIN_LEGACY; chain++) {
         for (ht = 0; ht < WLANSTATS_MAX_BW_LEGACY; ht++) {
              snr = ppdu_start[chain];
              snr >>= ht * WLANSTATS_SNR_OFFSET;
              snr &= WLANSTATS_SNR_MASK;

              if (snr & WLANSTATS_SNR_MAX)
                  continue;
              cdp_rx_ppdu->rssi_chain[chain][ht] = snr;
         }
    }
}

static void
ol_txrx_dispatch_ppdu_indication(struct ol_txrx_pdev_t *txrx_pdev, uint16_t peer_id)
{
    qdf_nbuf_t ppdu_nbuf;
    struct cdp_rx_indication_ppdu *cdp_rx_ppdu;

    ppdu_nbuf = qdf_nbuf_alloc(NULL,
                      sizeof(struct cdp_rx_indication_ppdu), 0, 0, FALSE);
    if (!ppdu_nbuf) {
         qdf_warn("Buffer allocation failed in %s", __func__);
            return;
        }

    cdp_rx_ppdu = (struct cdp_rx_indication_ppdu *)qdf_nbuf_data(ppdu_nbuf);
    qdf_mem_copy(cdp_rx_ppdu, &txrx_pdev->cdp_rx_ppdu,
                 sizeof(struct cdp_rx_indication_ppdu));

    if (cdp_rx_ppdu->peer_id != HTT_INVALID_PEER) {
        wdi_event_handler(WDI_EVENT_RX_PPDU_DESC, txrx_pdev,
                          ppdu_nbuf, cdp_rx_ppdu->peer_id, HTT_RX_IND_MPDU_STATUS_OK);
    } else if (txrx_pdev->mcopy_mode) {
         wdi_event_handler(WDI_EVENT_RX_PPDU_DESC, txrx_pdev,
                           ppdu_nbuf, HTT_INVALID_PEER, HTT_RX_IND_MPDU_STATUS_OK);
    } else {
            qdf_nbuf_free(ppdu_nbuf);
    }
    qdf_mem_zero(&txrx_pdev->cdp_rx_ppdu, sizeof(*cdp_rx_ppdu));
    txrx_pdev->cdp_rx_ppdu.peer_id = HTT_INVALID_PEER;
}

static void
ol_rx_populate_ppdu(struct ol_txrx_pdev_t *txrx_pdev,
                    struct ol_txrx_peer_t *peer,
                    void *rx_desc)
{
    bool is_retry;
    struct cdp_rx_indication_ppdu *cdp_rx_ppdu;
    uint32_t *ppdu;
    uint32_t ratekbps;
    uint8_t rate_code = 0, nss=0, bw=0, sgi = 0, l_sig_rate_select, l_sig_rate;
    uint8_t preamble_type, preamble=0, rate_flags = 0;
    uint16_t mcs=0;
    uint32_t rix;
    struct ol_txrx_psoc_t *soc = (struct ol_txrx_psoc_t *)txrx_pdev->soc;
    uint16_t ratecode;
    enum cdp_punctured_modes punc_mode = NO_PUNCTURE;

    cdp_rx_ppdu = &txrx_pdev->cdp_rx_ppdu;
    if (qdf_unlikely(soc->peerstats_enabled
#if UNIFIED_SMARTANTENNA
        || target_if_sa_api_is_rx_feedback_enabled((struct wlan_objmgr_psoc *)soc->psoc_obj,
              ((struct ol_ath_softc_net80211 *)txrx_pdev->scnctx)->sc_pdev)
#endif
       ))
        ol_txrx_update_peer_rx_rssi(txrx_pdev, peer, rx_desc, cdp_rx_ppdu);

    qdf_mem_copy(cdp_rx_ppdu->mac_addr, peer->mac_addr.raw, 6);
    cdp_rx_ppdu->peer_id = peer->peer_ids[0];
    cdp_rx_ppdu->vdev_id = peer->vdev->vdev_id;

    is_retry = txrx_pdev->htt_pdev->ar_rx_ops->is_retry(rx_desc);
    if (is_retry)
        cdp_rx_ppdu->retries++;

    ppdu = txrx_pdev->htt_pdev->ar_rx_ops->get_ppdu_start(rx_desc);
    if (ppdu) {
	    preamble_type = (ppdu[RX_D_PREAMBLE_OFFSET] & RX_D_PREAMBLE_MASK) >> RX_D_PREAMBLE_SHIFT;
	    switch(preamble_type)
	    {
		    case 4:
			    l_sig_rate_select = (ppdu[RX_D_PREAMBLE_OFFSET] & RX_D_MASK_LSIG_SEL) ; /* Bit 4 */
			    l_sig_rate = (ppdu[RX_D_PREAMBLE_OFFSET]  & RX_D_MASK_LSIG_RATE); /* Bit[3:0] */
			    rate_flags = 0x00;    /* bw is HT20 and sgi 0 for legacy rates */
			    if(l_sig_rate_select) { /* CCK */
				    switch(l_sig_rate) {
					    case 0xb:
					    case 0x1:
						    rate_code=0x43;
						    break;
					    case 0xa:
					    case 0xe:
					    case 0x2:
					    case 0x5:
						    rate_code=0x42;
						    break;
					    case 0x9:
					    case 0xd:
					    case 0x3:
					    case 0x6:
						    rate_code=0x41;
						    break;
					    case 0x8:
					    case 0xc:
					    case 0x4:
					    case 0x7:
						    rate_code=0x40;
						    break;
				    }
			    } else { /* OFDM, l_sig_rate_select==0 */
				    switch(l_sig_rate) {
					    case 0x8:
						    rate_code=0x00;
						    break;
					    case 0x9:
						    rate_code=0x01;
						    break;
					    case 0xa:
						    rate_code=0x02;
						    break;
					    case 0xb:
						    rate_code=0x03;
						    break;
					    case 0xc:
						    rate_code=0x04;
						    break;
					    case 0xd:
						    rate_code=0x05;
						    break;
					    case 0xe:
						    rate_code=0x06;
						    break;
					    case 0xf:
						    rate_code=0x07;
						    break;
				    }
			    }
			    preamble = GET_HW_RATECODE_PREAM(rate_code);
			    mcs = GET_HW_RATECODE_RATE(rate_code);
			    nss = GET_HW_RATECODE_NSS(rate_code);
			    break;
			    /* HT */
		    case 8: /* HT w/o TxBF */
		    case 9:/* HT w/ TxBF */
			    mcs = (u_int8_t)(ppdu[RX_D_HT_RATE_OFFSET] & RX_D_HT_MCS_MASK);
			    nss = mcs>>3;
			    mcs %= 8;
			    bw  = (u_int8_t)((ppdu[RX_D_HT_RATE_OFFSET] >> 7) & 1);
			    sgi = (u_int8_t)((ppdu[RX_D_VHT_RATE_OFFSET] >> 7) & 1);
			    rate_flags = bw << 5 | sgi << 3;
			    preamble = HW_RATECODE_PREAM_HT;
			    rate_code = ASSEMBLE_HW_RATECODE(mcs, nss, preamble);
			    break;
			    /* VHT */
		    case 0x0c: /* VHT w/o TxBF */
		    case 0x0d: /* VHT w/ TxBF */
			    mcs = (u_int8_t)((ppdu[RX_D_VHT_RATE_OFFSET] >> 4) & RX_D_VHT_MCS_MASK);
			    nss = (u_int8_t)((ppdu[RX_D_HT_RATE_OFFSET] >> 10) & RX_D_VHT_NSS_MASK);
			    bw  = (u_int8_t)((ppdu[RX_D_HT_RATE_OFFSET] & RX_D_VHT_BW_MASK));
			    sgi = (u_int8_t)((ppdu[RX_D_VHT_RATE_OFFSET]) & RX_D_VHT_SGI_MASK);
			    if (bw == 0x3)
				    rate_flags = 0x80 | sgi << 3;
			    else
				    rate_flags = bw << 5 | sgi << 3;
			    preamble = HW_RATECODE_PREAM_VHT;
			    rate_code = ASSEMBLE_HW_RATECODE(mcs, nss, preamble);
			    break;
		    default:
			    break;
	    }
	    peer->stats.rx.rx_ratecode = rate_code;
	    peer->stats.rx.rx_flags = rate_flags;

	    ratekbps = dp_getrateindex(sgi, mcs, nss, preamble, bw, punc_mode, &rix, &ratecode);
	    if (ratekbps)
		    peer->stats.rx.avg_rx_rate = dp_ath_rate_lpf(peer->stats.rx.avg_rx_rate, ratekbps);
	    peer->stats.rx.rnd_avg_rx_rate = dp_ath_rate_out(peer->stats.rx.avg_rx_rate);
	    cdp_rx_ppdu->u.mcs = mcs;
	    cdp_rx_ppdu->u.nss = nss;
	    cdp_rx_ppdu->u.gi = sgi;
	    cdp_rx_ppdu->u.gi = sgi;
	    cdp_rx_ppdu->u.bw = bw;
	    cdp_rx_ppdu->u.preamble = preamble;
	    cdp_rx_ppdu->rx_ratekbps = ratekbps;
	    cdp_rx_ppdu->rx_ratecode = rate_code;
	    cdp_rx_ppdu->rix = rix;
    }
}

#if UNIFIED_SMARTANTENNA
static void
ol_rx_populate_evm_details(struct ol_txrx_pdev_t *txrx_pdev,
        void *rx_desc, struct cdp_rx_indication_ppdu *cdp_rx_ppdu)
{
    uint32_t *ppdu;
    uint8_t pilot;

    ppdu = txrx_pdev->htt_pdev->ar_rx_ops->get_ppdu_end(rx_desc);
    for (pilot = 0; pilot < MAX_EVM_SIZE_LEGACY; pilot++) {
        cdp_rx_ppdu->evm_info.pilot_evm[pilot] = ppdu[pilot];
    }
    cdp_rx_ppdu->evm_info.nss_count = 4;
    cdp_rx_ppdu->evm_info.pilot_count = 4;
    cdp_rx_ppdu->rx_antenna = (ppdu[RX_DESC_ANTENNA_OFFSET] & SMART_ANT_ANTENNA_MASK);
}
#endif

A_STATUS
ol_rx_update_peer_stats(void *pdev, qdf_nbuf_t msdu,
        uint16_t peer_id,
        enum htt_cmn_rx_status status)
{
    int is_mcast;
    void *rx_desc;
    struct ol_txrx_pdev_t *txrx_pdev =  (struct ol_txrx_pdev_t *)pdev;
    struct ieee80211vap *vap;
    struct ieee80211_node *ni;
    struct ol_txrx_vdev_t *vdev = NULL;
    struct ol_txrx_peer_t *peer;
    struct wlan_objmgr_pdev *pdev_obj;
    struct ol_ath_softc_net80211 *scn;
    uint32_t data_length = 0;
    char *hdr_des;
    struct ieee80211_frame *wh;
    uint32_t attn;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    struct ethhdr *eh = NULL;
#endif
    bool is_bcast = false;
    struct cdp_rx_indication_ppdu *cdp_rx_ppdu;
    if (!msdu) {
        qdf_err("Invalid data for pdev %pK", txrx_pdev);
        return A_ERROR;
    }

    scn = (struct ol_ath_softc_net80211 *)txrx_pdev->scnctx;
    if (!scn) {
        qdf_err("Invalid scn for pdev %pK", txrx_pdev);
        return A_ERROR;
    }

    pdev_obj = (struct wlan_objmgr_pdev *)scn->sc_pdev;
    peer = (peer_id == HTT_INVALID_PEER) ?
        NULL : txrx_pdev->peer_id_to_obj_map[peer_id];
    rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev,
            msdu);

    cdp_rx_ppdu = &txrx_pdev->cdp_rx_ppdu;
    switch (status) {
        case HTT_RX_IND_MPDU_STATUS_OK:
            if (peer) {
                if (!pdev) {
                    qdf_print("Invalid pdev in %s",
                            __func__);
                    goto handler_fail;
                }
                if (!msdu) {
                    qdf_print("Invalid data in %s",
                            __func__);
                    goto handler_fail;
                }
                if (txrx_pdev->htt_pdev->ar_rx_ops->is_first_mpdu(rx_desc)) {
                    ol_rx_populate_ppdu(txrx_pdev, peer, rx_desc);
                }

                while (msdu) {
                    rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev,
                            msdu);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                    if (txrx_pdev->nss_wifiol_ctx) {
                        /*  In NSS offload mode htt_rx_msdu_forward and htt_rx_msdu_discard
                            will be overwritten by the NSS metadata. Hence mcast detection
                            is from the ethernet header. */
                        eh = (struct ethhdr *)qdf_nbuf_data(msdu);
                        is_mcast = IEEE80211_IS_MULTICAST(eh->h_dest) ? 1 : 0;
                    }
                    else
#endif
                    {
                        /*  Here the mcast packets are decided on the basis that
                            the target sets "only" the forward bit for mcast packets.
                            If it is a normal packet then "both" the forward bit and
                            the discard bit is set. Forward bit indicates that the
                            packet needs to be forwarded and the discard bit
                            indicates that the packet should not be delivered to
                            the OS.*/
                        is_mcast = (htt_rx_msdu_forward(txrx_pdev->htt_pdev, rx_desc)
                                && !htt_rx_msdu_discard(txrx_pdev->htt_pdev, rx_desc));
                        is_bcast = qdf_nbuf_is_bcast_pkt(msdu);
                    }
                    data_length = RXDESC_GET_DATA_LEN(rx_desc);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
                if (txrx_pdev->nss_wifiol_ctx && qdf_nbuf_is_bcast_pkt(msdu)) {
                    is_bcast = true;
                }
#endif
                    peer->stats.rx.to_stack.num++;
                    peer->stats.rx.to_stack.bytes += data_length;
                    if (is_mcast) {
                        peer->stats.rx.multicast.num++;
                        peer->stats.rx.multicast.bytes += data_length;
                        if (is_bcast) {
                            peer->stats.rx.bcast.num++;
                            peer->stats.rx.bcast.bytes += data_length;
                        }
                    }
                    cdp_rx_ppdu->num_msdu++;
                    cdp_rx_ppdu->num_bytes += data_length;
                    msdu = qdf_nbuf_next(msdu);
                }
                cdp_rx_ppdu->num_mpdu++;
                if (txrx_pdev->htt_pdev->ar_rx_ops->is_last_mpdu(rx_desc)) {
#if UNIFIED_SMARTANTENNA
                    if (target_if_sa_api_is_rx_feedback_enabled(wlan_pdev_get_psoc(pdev_obj),
                                                        pdev_obj)) {
                        ol_rx_populate_evm_details(txrx_pdev, rx_desc, cdp_rx_ppdu);
                    }
#endif
                    ol_txrx_dispatch_ppdu_indication(txrx_pdev, peer->peer_ids[0]);
                }
            }
            break;
        case HTT_RX_IND_MPDU_STATUS_ERR_FCS:
            rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev,
                    msdu);
            data_length = RXDESC_GET_DATA_LEN(rx_desc);
            /*
             * Make data_length = 0, because an invalid FCS Error data length is received.
             * We ignore this data length as it updates the Rx bytes stats in wrong way.
             */
            /* In crcerr case, we may or may not have valid peer, so updating it at pdev */
            txrx_pdev->pdev_data_stats.rx.rx_crcerr++;
            data_length = 0;
            if (peer) {
                peer->stats.rx.to_stack.bytes +=data_length;
                peer->stats.rx.to_stack.num++;
            }
            break;
        case HTT_RX_IND_MPDU_STATUS_TKIP_MIC_ERR:
            if (peer) {
                rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev, msdu);
                data_length = RXDESC_GET_DATA_LEN(rx_desc);
                peer->stats.rx.err.mic_err++;
                peer->stats.rx.to_stack.bytes += data_length;
                peer->stats.rx.to_stack.num++;
            }
            break;
        case HTT_RX_IND_MPDU_STATUS_DECRYPT_ERR:
            if (peer) {
                rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev, msdu);
                data_length = RXDESC_GET_DATA_LEN(rx_desc);
                peer->stats.rx.err.decrypt_err++;
                peer->stats.rx.to_stack.num++;
                peer->stats.rx.to_stack.bytes += data_length;
            }
            break;
        case HTT_RX_IND_MPDU_STATUS_MGMT_CTRL:
            if (peer) {
                rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev, msdu);
                data_length = RXDESC_GET_DATA_LEN(rx_desc);
                hdr_des = txrx_pdev->htt_pdev->ar_rx_ops->wifi_hdr_retrieve(rx_desc);
                wh = (struct ieee80211_frame*)hdr_des;
            if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_CTL)
#ifdef QCA_SUPPORT_CP_STATS
                pdev_cp_stats_rx_num_ctl_inc(pdev_obj, 1);
#endif

            if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_MGT) {

#ifdef QCA_SUPPORT_CP_STATS
                pdev_cp_stats_rx_num_mgmt_inc(pdev_obj, 1);
#endif
                vdev = peer->vdev;
                vap = ol_ath_pdev_vap_get(pdev_obj, vdev->vdev_id);

                if (qdf_likely(vap)) {
#ifdef QCA_SUPPORT_CP_STATS
                    vdev_ucast_cp_stats_rx_mgmt_inc(vap->vdev_obj, 1);
#endif
                    ni = ieee80211_vap_find_node(vap, peer->mac_addr.raw, WLAN_MLME_SB_ID);
                    if (qdf_likely(ni)) {
#ifdef QCA_SUPPORT_CP_STATS
                        peer_cp_stats_rx_mgmt_inc(ni->peer_obj, 1);
#endif
                        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
                    }
                    ol_ath_release_vap(vap);
                }
            }
                attn = txrx_pdev->htt_pdev->ar_rx_ops->get_attn_word(rx_desc);
                if (!(attn & OL_RX_NULL_DATA_MASK)) {
                    peer->stats.rx.to_stack.num++;
                    peer->stats.rx.to_stack.bytes += data_length;
                    /* NULL data frame must be 'unicast' */
                }
            while (msdu) {
                rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev, msdu);
                attn = txrx_pdev->htt_pdev->ar_rx_ops->get_attn_word(rx_desc);
                if (attn & OL_RX_OVERFLOW_MASK) {
                    /*RX overflow*/
#ifdef QCA_SUPPORT_CP_STATS
                    pdev_cp_stats_rx_overrun_inc(pdev_obj, 1);
#endif
                }
                msdu = qdf_nbuf_next(msdu);
            }
            }
            break;
        default:
            rx_desc = htt_rx_msdu_desc_retrieve(txrx_pdev->htt_pdev,
                    msdu);
            data_length = RXDESC_GET_DATA_LEN(rx_desc);
            txrx_pdev->pdev_data_stats.rx.rx_packets++;
            txrx_pdev->pdev_data_stats.rx.rx_bytes += data_length;
            break;

    }
    return A_OK;

handler_fail:
    return A_ERROR;
}

void ol_rx_deliver( struct ol_txrx_vdev_t *vdev, struct ol_txrx_peer_t *peer, unsigned tid, qdf_nbuf_t msdu_list);

bool qdf_nbuf_is_nwifi_wapi_pkt(struct sk_buff *skb)
{
    uint16_t ether_type;
    u_int8_t *data;
    data = qdf_nbuf_data(skb);

    data += sizeof(struct ieee80211_frame);
    if (data) {
#define ETHER_TYPE_OFFSET 6
        ether_type = (data[ETHER_TYPE_OFFSET] << 8) | data[ETHER_TYPE_OFFSET + 1];
        if (ether_type == ETHERTYPE_WAI)
            return true;
    }
    return false;
}

void
ol_rx_indication_handler(
        ol_txrx_pdev_handle pdevh,
        qdf_nbuf_t rx_ind_msg,
        u_int16_t peer_id,
        u_int8_t tid,
        int num_mpdu_ranges)
{
    int mpdu_range;
    struct ol_txrx_pdev_t *pdev = (struct ol_txrx_pdev_t *)pdevh;
    int seq_num_start = 0, seq_num_end = 0, rx_ind_release = 0;
    struct ol_txrx_vdev_t *vdev = NULL;
    struct ol_txrx_peer_t *peer = NULL;
    htt_pdev_handle htt_pdev;
    int skip_monitor =0;
    struct ieee80211_frame *wh;
    struct ol_ath_softc_net80211 *scn = NULL;
    int type = 0, subtype = 0;
    struct mgmt_rx_event_params rx_event = {0};
    void *rx_desc = NULL;
    struct wlan_objmgr_pdev *spdev;
    struct wlan_objmgr_psoc *spsoc;
    int msdu_status = 0;
    struct cdp_vdev_stats *vdev_stats = NULL;

    scn = (struct ol_ath_softc_net80211 *)pdev->scnctx;
    if (qdf_unlikely(!scn)) {
        qdf_err("Invalid scn for pdev %pK", pdev);
        return;
    }

    qdf_spin_lock(&pdev->mon_mutex);
    if (pdev->monitor_vdev == NULL) {
        skip_monitor= 1;
        qdf_spin_unlock(&pdev->mon_mutex);
    }

    spdev = (struct wlan_objmgr_pdev *)scn->sc_pdev;
    spsoc = wlan_pdev_get_psoc(spdev);

    htt_pdev = pdev->htt_pdev;
    peer = ol_txrx_peer_find_by_id(pdevh, peer_id);
    if (peer) {
        vdev = peer->vdev;
	vdev_stats = &vdev->stats;
    }

    if (htt_rx_ind_release(rx_ind_msg)) {
        /* the ind info of release is saved here and do release at the end.
         * This is for the reason of in HL case, the qdf_nbuf_t for msg and
         * payload are the same buf. And the buf will be changed during
         * processing
         */
        rx_ind_release = 1;
        htt_rx_ind_release_seq_num_range(
            rx_ind_msg, &seq_num_start, &seq_num_end);
    }

#if RX_DEBUG
    htt_pdev->g_mpdu_ranges = num_mpdu_ranges;
#endif

    for (mpdu_range = 0; mpdu_range < num_mpdu_ranges; mpdu_range++) {
        enum htt_rx_status status;
        int i, num_mpdus;
        qdf_nbuf_t tail_msdu;
        qdf_nbuf_t msdu = NULL;
        void *rx_mpdu_desc;
        u_int32_t npackets = 0;
        int msdu_chaining = 0;
        u_int32_t    mgmt_type = 0;

        htt_rx_ind_mpdu_range_info(
            pdev->htt_pdev, rx_ind_msg, mpdu_range, &status, &num_mpdus);
#if RX_DEBUG
        htt_pdev->g_num_mpdus = num_mpdus;
        htt_pdev->g_htt_status = status;
#endif
        for (i = 0; i < num_mpdus; i++) {
            int discard = 1;
            npackets = 0;
            /* pull the MPDU's MSDUs off the buffer queue */
            msdu_chaining = htt_rx_amsdu_pop(htt_pdev,
#if QCA_OL_SUPPORT_RAWMODE_TXRX
                        vdev,
#endif
                        rx_ind_msg, &msdu, &tail_msdu, &npackets);
#if RX_DEBUG
            if (!npackets) {
                if ( skip_monitor == 0 ) {
                    qdf_spin_unlock(&pdev->mon_mutex);
                }
                return;
            }
#endif
            /* pull the MPDU desc off the desc queue */
            rx_mpdu_desc =
                    htt_rx_mpdu_desc_list_next(htt_pdev, rx_ind_msg);


            if (status == htt_rx_status_err_inv_peer) {
                 /* once per mpdu */
                 ol_rx_process_inv_peer((ol_txrx_pdev_handle) pdev, rx_mpdu_desc, msdu);
            }
            mgmt_type = htt_pdev->ar_rx_ops->check_desc_mgmt_type(msdu);

            msdu_status = htt_pdev->ar_rx_ops->update_msdu_info(
                                        htt_pdev->arh, msdu, tail_msdu);
            /* Consider only ampdu last subframe and last subframe known.
             * 10th and 11th bits of rs_flags indicates last subframe ampdu
             * known and last ampdu subframe respectively
	     */

            pdev->rx_mon_recv_status->rs_flags &= 0xFFFFF3FF;
            pdev->rx_mon_recv_status->rs_flags |= msdu_status;

            rx_desc = htt_rx_desc(msdu);
            if ((status == htt_rx_status_ctrl_mgmt_null) && (peer == NULL)) {
                wh = (struct ieee80211_frame *) msdu->data;

                type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
                subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
                if (msdu->len >= sizeof(struct ieee80211_frame)) {
                    if ((type == IEEE80211_FC0_TYPE_DATA) && (subtype == IEEE80211_FC0_SUBTYPE_NODATA)) {
                        /*
                         * Check for FCS error bit and Null data bit in Rx Attention word.
                         * Send frames only with no fcs error
                         */
                        if (!(htt_pdev->ar_rx_ops->is_fcs_err(rx_desc))) {
                            qdf_nbuf_t next = qdf_nbuf_next(msdu);
                            msdu->next = NULL;
                            ol_ath_mgmt_handler(spdev, scn, msdu, wh, &rx_event, true);

                            msdu = next ;
                            while (msdu) {
                                qdf_nbuf_t next = qdf_nbuf_next(msdu);
                                qdf_nbuf_free(msdu);
                                msdu = next;
                            }
                            discard = 0;
                        }
                    }
                }
            }

            if (!skip_monitor && discard) {
                /*
                 * TBDXXX - to deliver SDU with chaining in monitor mode.
                 * Though the stitch function is available, there is a chance
                 * that the skb->len can get bad values, due to which kernel
                 * panics are seen.
                 */

                 if (!msdu_chaining){
                     discard = ol_rx_monitor_deliver(pdev, msdu, 1,
                                                     rx_mpdu_desc, status, peer);
                 }

            }


            if (discard) {
                /* Free the nbuf iff monitor layer did not absorb */
                while (1) {
                    qdf_nbuf_t next;
                    next = qdf_nbuf_next(msdu);
                    htt_rx_desc_frame_free(htt_pdev, msdu);
                    if (msdu == tail_msdu) {
                        break;
                    }
                    msdu = next;
                }
            }
        }
    }

    /*
     * Now that a whole batch of MSDUs have been pulled out of HTT
     * and put into the rx reorder array, it is an appropriate time
     * to request HTT to provide new rx MSDU buffers for the target
     * to fill.
     * This could be done after the end of this function, but it's
     * better to do it now, rather than waiting until after the driver
     * and OS finish processing the batch of rx MSDUs.
     */
#if QCA_PARTNER_DIRECTLINK_RX
    /*
     * For Direct Link RX, replenish already done on partner side so skip it.
     */
    if (CE_is_directlink(((struct ol_txrx_pdev_t*)htt_pdev->txrx_pdev)->ce_tx_hdl)) {
        qdf_nofl_info("ERROR: %s called over Directlink\n",__func__);
        A_ASSERT(0);
    }
#endif /* QCA_PARTNER_DIRECTLINK_RX */

    htt_rx_msdu_buff_replenish(htt_pdev);

    if (skip_monitor == 0) {
        qdf_spin_unlock(&pdev->mon_mutex);
    }
}

#if 0
void
ol_rx_sec_ind_handler(
    ol_txrx_pdev_handle pdev,
    u_int16_t peer_id,
    enum htt_sec_type sec_type,
    int is_unicast,
    u_int32_t *michael_key,
    u_int32_t *rx_pn)
{
    struct ol_txrx_peer_t *peer;
    int sec_index, i;

    peer = ol_txrx_peer_find_by_id(pdev, peer_id);
    if (! peer) {
        TXRX_PRINT(TXRX_PRINT_LEVEL_ERR,
            "Couldn't find peer from ID %d - skipping security inits\n",
            peer_id);
        return;
    }
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
        "sec spec for peer %pK (%02x:%02x:%02x:%02x:%02x:%02x): "
        "%s key of type %d\n",
        peer,
        peer->mac_addr.raw[0], peer->mac_addr.raw[1], peer->mac_addr.raw[2],
        peer->mac_addr.raw[3], peer->mac_addr.raw[4], peer->mac_addr.raw[5],
        is_unicast ? "ucast" : "mcast",
        sec_type);
    sec_index = is_unicast ? txrx_sec_ucast : txrx_sec_mcast;
    peer->security[sec_index].sec_type = sec_type;
    /* michael key only valid for TKIP, but for simplicity, copy it anyway */
    qdf_mem_copy(
        &peer->security[sec_index].michael_key[0],
        michael_key,
        sizeof(peer->security[sec_index].michael_key));
#ifdef BIG_ENDIAN_HOST
    OL_IF_SWAPBO(peer->security[sec_index].michael_key[0],
                 sizeof(peer->security[sec_index].michael_key));
#endif /* BIG_ENDIAN_HOST */

    if (sec_type != htt_sec_type_wapi) {
        qdf_mem_set(peer->tids_last_pn_valid, OL_TXRX_NUM_EXT_TIDS, 0x00);
    } else {
        for (i = 0; i < OL_TXRX_NUM_EXT_TIDS; i++) {
            /*
             * Setting PN valid bit for WAPI sec_type,
             * since WAPI PN has to be started with predefined value
             */
            peer->tids_last_pn_valid[i] = 1;
            qdf_mem_copy(
                (u_int8_t *) &peer->tids_last_pn[i],
                (u_int8_t *) rx_pn, sizeof(union htt_rx_pn_t));
            peer->tids_last_pn[i].pn128[1] =
                qdf_cpu_to_le64(peer->tids_last_pn[i].pn128[1]);
            peer->tids_last_pn[i].pn128[0] =
                qdf_cpu_to_le64(peer->tids_last_pn[i].pn128[0]);
        }
    }

#if QCA_PARTNER_DIRECTLINK_RX
    /*
     * inform partner side for security information update.
     */
    if (CE_is_directlink(pdev->ce_tx_hdl)) {
        ol_rx_sec_ind_handler_partner(peer, peer_id, sec_type, rx_pn);
    }
#endif /* QCA_PARTNER_DIRECTLINK_RX */

}
#endif


#define OLE_HEADER_PADDING 2

#include <ieee80211.h>

uint16_t
transcap_nwifi_ether_type(qdf_nbuf_t msdu)
{
    struct ieee80211_frame_addr4 *wh;
    uint32_t hdrsize;
    uint8_t fc1;
    struct llc *llchdr;
    uint16_t ether_type = 0;
    wh = (struct ieee80211_frame_addr4 *)qdf_nbuf_data(msdu);
    fc1 = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;

    /* Native Wifi header  give only 80211 non-Qos packets */
    if (fc1 == IEEE80211_FC1_DIR_DSTODS )
    {
        hdrsize = sizeof(struct ieee80211_frame_addr4);

        /* In case of WDS frames, , padding is enabled by default
         * in Native Wifi mode, to make ipheader 4 byte aligned
         */
        hdrsize = hdrsize + OLE_HEADER_PADDING;
    } else {
        hdrsize = sizeof(struct ieee80211_frame);
    }

    /*
     *
     * header size (wifhdrsize + llchdrsize )
     */
    llchdr = (struct llc *)(((uint8_t *)qdf_nbuf_data(msdu)) + hdrsize);
    ether_type = llchdr->llc_un.type_snap.ether_type;
    return ntohs(ether_type);
}

#define SIZEOF_80211_HDR (sizeof(struct ieee80211_frame))
/**
 * @brief Check the first msdu to decide whether the a-msdu should be accepted.
 */
int
ol_rx_filter(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    qdf_nbuf_t msdu,
    void *rx_desc)
{
    #define FILTER_STATUS_REJECT 1
    #define FILTER_STATUS_ACCEPT 0
    u_int8_t *wh;
    u_int32_t offset = 0;
    u_int16_t ether_type = 0;
    u_int8_t is_encrypted = 0, is_mcast = 0, i;
    privacy_filter_packet_type packet_type = PRIVACY_FILTER_PACKET_UNICAST;
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;

    /* Safemode must avoid the PrivacyExemptionList and ExcludeUnencrypted checking */
    if (vdev->safemode) {
        return FILTER_STATUS_ACCEPT;
    }

    is_mcast = htt_rx_msdu_is_wlan_mcast(htt_pdev, rx_desc);

    if (vdev->filters_num > 0) {
        if (pdev->rx_decap_mode == htt_pkt_type_native_wifi) {
            offset = SIZEOF_80211_HDR + LLC_SNAP_HDR_OFFSET_ETHERTYPE;
        } else {
            offset = ETHERNET_ADDR_LEN * 2;
        }

        /* get header info from msdu */
        wh = qdf_nbuf_data(msdu);

        /* get ether type */
        ether_type = (wh[offset] << 8) | wh[offset+1];
        if (ether_type == ETHERTYPE_VLAN) {
            ether_type = (wh[offset + sizeof(struct vlan_hdr)] << 8) |
                                      wh[offset + sizeof(struct vlan_hdr) + 1];
        }

        /* get packet type */
        if (is_mcast) {
            packet_type = PRIVACY_FILTER_PACKET_MULTICAST;
        } else {
            packet_type = PRIVACY_FILTER_PACKET_UNICAST;
        }
    }
    /* get encrypt info */
    is_encrypted = htt_rx_mpdu_is_encrypted(htt_pdev, rx_desc);

    for (i=0; i < vdev->filters_num; i++) {
        /* skip if the ether type does not match */
        if (vdev->privacy_filters[i].ether_type != ether_type) {
            continue;
        }

        /* skip if the packet type does not match */
        if (vdev->privacy_filters[i].packet_type != packet_type &&
            vdev->privacy_filters[i].packet_type != PRIVACY_FILTER_PACKET_BOTH) {
            continue;
        }

        if (vdev->privacy_filters[i].filter_type == PRIVACY_FILTER_ALWAYS) {
            /*
             * In this case, we accept the frame if and only if it was originally
             * NOT encrypted.
             */
            if (is_encrypted) {
                return FILTER_STATUS_REJECT;
            } else {
                return FILTER_STATUS_ACCEPT;
            }
        } else if (vdev->privacy_filters[i].filter_type  == PRIVACY_FILTER_KEY_UNAVAILABLE) {
            /*
             * In this case, we reject the frame if it was originally NOT encrypted but
             * we have the key mapping key for this frame.
             */
            if (!is_encrypted && !is_mcast && (peer->security[txrx_sec_ucast].sec_type != htt_sec_type_none)) {
                 return FILTER_STATUS_REJECT;
            } else {
                return FILTER_STATUS_ACCEPT;
            }
        } else {
            /*
             * The privacy exemption does not apply to this frame.
             */
            break;
        }
    }

    /* Reject if the the node is not authorized */
    if (!peer->authorize) {
        return FILTER_STATUS_REJECT;
    }

    /*
     * If the privacy exemption list does not apply to the frame, check ExcludeUnencrypted.
     * if ExcludeUnencrypted is not set, or if this was oringially an encrypted frame,
     * it will be accepted.
     *
     */
    if (!vdev->drop_unenc || is_encrypted) {
        return FILTER_STATUS_ACCEPT;
    }

    /*
     *  If this is a open connection, it will be accepted.
     */
    if(peer->security[is_mcast ? txrx_sec_mcast : txrx_sec_ucast].sec_type == htt_sec_type_none) {
        return FILTER_STATUS_ACCEPT;
    }

    return FILTER_STATUS_REJECT;
}

#if QCA_SUPPORT_WDS_EXTENDED && !QCA_NSS_PLATFORM
static inline bool
ol_rx_deliver_to_stack_ext(struct ol_txrx_vdev_t *vdev,
                           struct ol_txrx_peer_t *peer,
                           qdf_nbuf_t nbuf_head)
{
    qdf_nbuf_t next_nbuf, deliver_list_tail = NULL;

    if (qdf_likely(!vdev->wds_ext_enabled)) {
        return false;
    }

    if (!peer->wds_ext.init) {
        return false;
    }

    if (peer->osif_rx) {
        peer->osif_rx(peer->wds_ext.osif_peer, nbuf_head);
    } else {
        while(nbuf_head) {
            next_nbuf = qdf_nbuf_next(nbuf_head);
            qdf_nbuf_free(nbuf_head);
            nbuf_head = next_nbuf;

            if (next_nbuf == NULL && nbuf_head)
                qdf_nbuf_set_next(deliver_list_tail, NULL); /* add NULL terminator */
        }
    }

    return true;
}
#endif

void
ol_rx_deliver(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    qdf_nbuf_t msdu_list)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    qdf_nbuf_t deliver_list_head = NULL;
    qdf_nbuf_t deliver_list_tail = NULL;
    qdf_nbuf_t msdu;
    uint8_t filter = 0;
#if OL_ATH_SUPPORT_LED
    uint32_t   byte_cnt = 0;
#endif
#if QCA_SUPPORT_WDS_EXTENDED && !QCA_NSS_PLATFORM
    bool status = false;
#endif

    if (!peer) {
        return;
    }

    msdu = msdu_list;
    /*
     * Check each MSDU to see whether it requires special handling,
     * and free each MSDU's rx descriptor
     */
    while (msdu) {
        void *rx_desc;
        int discard, inspect, dummy_fwd;
        qdf_nbuf_t next = qdf_nbuf_next(msdu);

        rx_desc = htt_rx_msdu_desc_retrieve(pdev->htt_pdev, msdu);

        /* Save to skb->priority */
        wbuf_set_priority(msdu,tid);

        // for HL, point to payload right now
        if (pdev->cfg.is_high_latency) {
            qdf_nbuf_pull_head(msdu,
                    htt_rx_msdu_rx_desc_size_hl(htt_pdev,
                        rx_desc));
        }

        htt_rx_msdu_actions(
            pdev->htt_pdev, rx_desc, &discard, &dummy_fwd, &inspect);

	if (pdev->host_80211_enable) {
		transcap_nwifi_to_8023(msdu);
	}

        /* Check the first msdu in mpdu, if it will be filter out, then discard all the mpdu */
        if (htt_rx_msdu_first_msdu_flag(htt_pdev, rx_desc)) {
            filter = ol_rx_filter(vdev, peer, msdu, rx_desc);
        }

        htt_rx_msdu_desc_free(htt_pdev, msdu);

        /*
         * In encryption mode, all data packets except
         * EAPOL frames should be dropped when peer is not
         * authenticated. Thie feature is enabled for all peers
         * under this vdev when peer_authorize flag is set.
         */
        if (!peer->authorize && !(qdf_nbuf_is_ipv4_eapol_pkt(msdu) || qdf_nbuf_is_ipv4_wapi_pkt(msdu))) {
             qdf_nbuf_free(msdu);
             pdev->pdev_stats.peer_unauth_rx_pkt_drop++;
             msdu = next;
             /* If discarding packet is last packet of the delivery list,NULL terminator should be added for delivery list. */
             if (next == NULL && deliver_list_head){
                 qdf_nbuf_set_next(deliver_list_tail, NULL); /* add NULL terminator */
             }

             continue;
        }

        if (discard || filter) {
#if HTT_DEBUG_DATA
            HTT_PRINT("-\n");
            if (tid == 0) {
                if (qdf_nbuf_data(msdu)[0x17] == 0x6) {
                    // TCP, dump seq
                    HTT_PRINT("0x%02x 0x%02x 0x%02x 0x%02x\n",
                            qdf_nbuf_data(msdu)[0x26],
                            qdf_nbuf_data(msdu)[0x27],
                            qdf_nbuf_data(msdu)[0x28],
                            qdf_nbuf_data(msdu)[0x29]);
                } else {
                    HTT_PRINT_BUF(qdf_nofl_info, qdf_nbuf_data(msdu),
                            0x41,__func__);
                }
            }

#endif
            qdf_nbuf_free(msdu);
            /* If discarding packet is last packet of the delivery list,NULL terminator should be added for delivery list. */
            if (next == NULL && deliver_list_head){
                qdf_nbuf_set_next(deliver_list_tail, NULL); /* add NULL terminator */
            }
        } else {
#if OL_ATH_SUPPORT_LED
            byte_cnt += qdf_nbuf_len(msdu);
#endif
            TXRX_STATS_MSDU_INCR(vdev->pdev, rx.to_stack, msdu);
            OL_TXRX_PROT_AN_LOG(vdev->pdev->prot_an_rx_sent, msdu);
            OL_TXRX_LIST_APPEND(deliver_list_head, deliver_list_tail, msdu);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
            qdf_nbuf_record_rx_queue(msdu, 0);
            skb_set_hash(msdu, 0xaa , PKT_HASH_TYPE_L4);
#endif
        }
        msdu = next;
    }
    /* sanity check - are there any frames left to give to the OS shim? */
    if (!deliver_list_head) {
        return;
    }


#if HTT_DEBUG_DATA
    /*
     * we need a place to dump rx data buf
     * to host stack, here's a good place
     */
    for (msdu = deliver_list_head; msdu; msdu = qdf_nbuf_next(msdu)) {
        if (tid == 0) {
            if (qdf_nbuf_data(msdu)[0x17] == 0x6) {
                // TCP, dump seq
                HTT_PRINT("0x%02x 0x%02x 0x%02x 0x%02x\n",
                        qdf_nbuf_data(msdu)[0x26],
                        qdf_nbuf_data(msdu)[0x27],
                        qdf_nbuf_data(msdu)[0x28],
                        qdf_nbuf_data(msdu)[0x29]);
            } else {
                HTT_PRINT_BUF(qdf_nofl_info, qdf_nbuf_data(msdu),
                        0x41,__func__);
            }
        }
    }
#endif

#if QCA_SUPPORT_WDS_EXTENDED && !QCA_NSS_PLATFORM
    status = ol_rx_deliver_to_stack_ext(vdev, peer,
                                        deliver_list_head);

    if (status == false) {
        vdev->osif_rx(vdev->osif_vdev, deliver_list_head);
    }
#else
    vdev->osif_rx(vdev->osif_vdev, deliver_list_head);
#endif
#if OL_ATH_SUPPORT_LED
    {
        struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)pdev->scnctx;
        if (scn) {
            scn->scn_led_byte_cnt += byte_cnt;
            ol_ath_led_event(scn, OL_ATH_LED_RX);
        }
    }
#endif

}

inline int
ol_get_rx_decap_mode_frmvdev(ol_txrx_vdev_handle vdev)
{
    return ((struct ol_txrx_vdev_t *)vdev)->rx_decap_type;
}

void
ol_rx_discard(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    qdf_nbuf_t msdu_list)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;

    while (msdu_list) {
        qdf_nbuf_t msdu = msdu_list;

        msdu_list = qdf_nbuf_next(msdu_list);
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO2,
            "discard rx %pK from partly-deleted peer %pK "
            "(%02x:%02x:%02x:%02x:%02x:%02x)\n",
            msdu, peer,
            peer->mac_addr.raw[0], peer->mac_addr.raw[1],
            peer->mac_addr.raw[2], peer->mac_addr.raw[3],
            peer->mac_addr.raw[4], peer->mac_addr.raw[5]);
        htt_rx_desc_frame_free(htt_pdev, msdu);
    }
}

void
ol_rx_peer_init(struct ol_txrx_pdev_t *pdev, struct ol_txrx_peer_t *peer)
{
    int tid;
    for (tid = 0; tid < OL_TXRX_NUM_EXT_TIDS; tid++) {

        /* invalid sequence number */
        peer->tids_last_seq[tid] = 0xffff;
	qdf_spinlock_create(&peer->tids_rx_reorder[tid].tid_lock);
    }
    /*
     * Set security defaults: no PN check, no security.
     * The target may send a HTT SEC_IND message to overwrite these defaults.
     */
    peer->security[txrx_sec_ucast].sec_type =
        peer->security[txrx_sec_mcast].sec_type = htt_sec_type_none;
}

void
ol_rx_peer_cleanup(struct ol_txrx_vdev_t *vdev, struct ol_txrx_peer_t *peer)
{
}

/*
 * Free frames including both rx descriptors and buffers
 */
void
ol_rx_frames_free(
    htt_pdev_handle htt_pdev,
    qdf_nbuf_t frames)
{
    qdf_nbuf_t next, frag = frames;

    while (frag) {
        next = qdf_nbuf_next(frag);
        htt_rx_desc_frame_free(htt_pdev, frag);
        frag = next;
    }
}

