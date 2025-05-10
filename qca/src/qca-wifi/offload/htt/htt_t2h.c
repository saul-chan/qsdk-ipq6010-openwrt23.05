/*
 * Copyright (c) 2011, 2015-2018, 2020-2022, 2024 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011, 2015-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file htt_t2h.c
 * @brief Provide functions to process target->host HTT messages.
 * @details
 *  This file contains functions related to target->host HTT messages.
 *  There are two categories of functions:
 *  1.  A function that receives a HTT message from HTC, and dispatches it
 *      based on the HTT message type.
 *  2.  functions that provide the info elements from specific HTT messages.
 */

#include <htc_api.h>         /* HTC_PACKET */
#include <htt.h>             /* HTT_T2H_MSG_TYPE, etc. */
#include <qdf_nbuf.h>        /* qdf_nbuf_t */

#include <ol_htt_rx_api.h>
#include <ol_txrx_htt_api.h> /* htt_tx_status */
#include <ol_ratectrl_11ac_if.h>
#include <ol_txrx_types.h>
#include <ol_txrx_internal.h>
#include <ol_txrx_stats.h>

#ifdef WLAN_FEATURE_FASTPATH
#include <ol_if_athvar.h>
#include <hif.h>
#endif /* WLAN_FEATURE_FASTPATH */

#include <htt_internal.h>
#include <pktlog_ac_fmt.h>
#include <wdi_event.h>
#include <ol_htt_tx_api.h>
#include <ol_txrx_api.h>
#include <init_deinit_lmac.h>
#if QCA_PARTNER_DIRECTLINK_TX
#define QCA_PARTNER_DIRECTLINK_HTT_T2H 1
#include "ath_carr_pltfrm.h"
#undef QCA_PARTNER_DIRECTLINK_HTT_T2H
#endif /* QCA_PARTNER_DIRECTLINK_TX */
#include <wlan_lmac_if_api.h>
#include <qdf_types.h>

/*--- target->host HTT message dispatch function ----------------------------*/

static u_int8_t *
htt_t2h_mac_addr_deswizzle(u_int8_t *tgt_mac_addr, u_int8_t *buffer)
{
#if BIG_ENDIAN_HOST
    /*
     * The host endianness is opposite of the target endianness.
     * To make u_int32_t elements come out correctly, the target->host
     * upload has swizzled the bytes in each u_int32_t element of the
     * message.
     * For byte-array message fields like the MAC address, this
     * upload swizzling puts the bytes in the wrong order, and needs
     * to be undone.
     */
    buffer[0] = tgt_mac_addr[3];
    buffer[1] = tgt_mac_addr[2];
    buffer[2] = tgt_mac_addr[1];
    buffer[3] = tgt_mac_addr[0];
    buffer[4] = tgt_mac_addr[7];
    buffer[5] = tgt_mac_addr[6];
    return buffer;
#else
    /*
     * The host endianness matches the target endianness -
     * we can use the mac addr directly from the message buffer.
     */
    return tgt_mac_addr;
#endif
}

/* Target to host Msg/event  handler  for low priority messages*/
void
htt_t2h_lp_msg_handler(void *context, qdf_nbuf_t htt_t2h_msg )
{
    struct htt_pdev_t *pdev = (struct htt_pdev_t *) context;
    u_int32_t *msg_word;
    enum htt_t2h_msg_type msg_type;

    msg_word = (u_int32_t *) qdf_nbuf_data(htt_t2h_msg);
    msg_type = HTT_T2H_MSG_TYPE_GET(*msg_word);
    switch (msg_type) {
    case HTT_T2H_MSG_TYPE_VERSION_CONF:
        {
            pdev->tgt_ver.major = HTT_VER_CONF_MAJOR_GET(*msg_word);
            pdev->tgt_ver.minor = HTT_VER_CONF_MINOR_GET(*msg_word);
            qdf_print("target uses HTT version %d.%d; host uses %d.%d",
                pdev->tgt_ver.major, pdev->tgt_ver.minor,
                HTT_CURRENT_VERSION_MAJOR, HTT_CURRENT_VERSION_MINOR);
            if (pdev->tgt_ver.major != HTT_CURRENT_VERSION_MAJOR) {
                qdf_print("*** Incompatible host/target HTT versions!");
            }
            /* abort if the target is incompatible with the host */
            qdf_assert(pdev->tgt_ver.major == HTT_CURRENT_VERSION_MAJOR);
            if (pdev->tgt_ver.minor != HTT_CURRENT_VERSION_MINOR) {
                qdf_print(
                    "*** Warning: host/target HTT versions are different, "
                    "though compatible!");
            }
            break;
        }
    case HTT_T2H_MSG_TYPE_RX_FLUSH:
        {
            break;
        }
    case  HTT_T2H_MSG_TYPE_RX_FRAG_IND:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_RX_ADDBA:
        {
            qdf_warn("HTT_T2H_MSG_TYPE_RX_ADDBA not expected");
            break;
        }
    case HTT_T2H_MSG_TYPE_RX_DELBA:
        {
            qdf_warn("HTT_T2H_MSG_TYPE_RX_DELBA not expected");
            break;
        }
    case HTT_T2H_MSG_TYPE_PEER_MAP:
        {
            u_int8_t mac_addr_deswizzle_buf[QDF_MAC_ADDR_SIZE];
            u_int8_t *peer_mac_addr;
            u_int16_t peer_id;
            u_int8_t vdev_id;

            peer_id = HTT_RX_PEER_MAP_PEER_ID_GET(*msg_word);
            vdev_id = HTT_RX_PEER_MAP_VDEV_ID_GET(*msg_word);
            peer_mac_addr = htt_t2h_mac_addr_deswizzle(
                (u_int8_t *) (msg_word+1), &mac_addr_deswizzle_buf[0]);

            if (peer_id > ol_cfg_max_peer_id((ol_soc_handle)pdev->ctrl_psoc)) {
                qdf_print("%s: HTT_T2H_MSG_TYPE_PEER_UNMAP,"
                          "invalid peer_id, %u",
                           __FUNCTION__,
                           peer_id);
                break;
            }
            ol_rx_peer_map_handler(
                (struct ol_txrx_pdev_t *)pdev->txrx_pdev, peer_id, vdev_id, peer_mac_addr);
            break;
        }
    case HTT_T2H_MSG_TYPE_PEER_UNMAP:
        {
            u_int16_t peer_id;
            peer_id = HTT_RX_PEER_UNMAP_PEER_ID_GET(*msg_word);
            if (peer_id > ol_cfg_max_peer_id((ol_soc_handle)pdev->ctrl_psoc)) {
                qdf_print("%s: HTT_T2H_MSG_TYPE_PEER_UNMAP,"
                          "invalid peer_id, %u",
                           __FUNCTION__,
                           peer_id);
                break;
            }
            ol_rx_peer_unmap_handler((struct ol_txrx_pdev_t *)pdev->txrx_pdev, peer_id);
            break;
        }
    case HTT_T2H_MSG_TYPE_SEC_IND:
        {
            break;
        }
#if TXRX_STATS_LEVEL != TXRX_STATS_LEVEL_OFF
    case HTT_T2H_MSG_TYPE_STATS_CONF:
        {
            break;
        }
#endif
#ifndef REMOVE_PKT_LOG
    case HTT_T2H_MSG_TYPE_PKTLOG:
        {
            break;
        }
#endif

    default:
        break;
    };
    /* Free the indication buffer */
    qdf_nbuf_free(htt_t2h_msg);
}

/**
 * htt_t2h_is_num_msdus_valid() - Vaidate num_msdus
 * @num_msdus : no of msdu's
 * @msg_len: length of the t2h message
 *
 * Check if num_msdus can result in going beyond the length of t2h msg
 * Return: true if no overflow or false otherwise
 */
static inline bool
htt_t2h_is_num_msdus_valid(uint32_t num_msdus, uint32_t msg_len)
{
    /*
     * Each desc id occupies 2 bytes.
     * htt msg header is of 4 bytes.
     */
    if ((num_msdus * HTT_TX_COMPL_BYTES_PER_MSDU_ID +
        HTT_TX_COMPL_HDR_SZ) > msg_len) {
        qdf_err("num_msdus(%d) is invalid, msg_len = %d\n",
                 num_msdus, msg_len);
        return false;
    }
    return true;
}

/*
 * htt_t2h_is_mdpu_range_valid() - Vaidate num_mpdu_range
 * @msg_len : length of t2h message
 * @num_mpdu_ranges: no of mpdu_ranges
 * @num_msdu_bytes: no of msdu bytes presnt in the msg
 * @mpdu_range_offset: offset of mpdu_range inside the msg
 *
 * Validate that num_mpdu_range doesnt result in accessing
 * memory beyond the t2h message boundary
 * Return: true if num_mpdu_range is valid or false otherwise
 */
static inline bool
htt_t2h_is_mdpu_range_valid(uint32_t msg_len, uint32_t num_mpdu_ranges,
                            uint32_t num_msdu_bytes,
                            uint32_t mpdu_range_offset)
{
        uint32_t computed_msg_len;

        if (qdf_unlikely(mpdu_range_offset > msg_len)) {
            qdf_err("Invalid mpdu_range_offset words %d\n",
                    mpdu_range_offset);
            return false;
        }
        computed_msg_len = mpdu_range_offset +
                           (num_mpdu_ranges * (int)sizeof(uint32_t));
        /*
         * Check that the addition and multiplication
         * do not cause integer overflow
         */
        if (qdf_unlikely(computed_msg_len < mpdu_range_offset)) {
            qdf_err("Invalid num_mpdu_ranges %u\n", num_mpdu_ranges);
            return false;
        }
        if (qdf_unlikely(computed_msg_len > msg_len)) {
            qdf_err("Invalid offset_words + mpdu_ranges %u\n",
                    computed_msg_len);
            return false;
        }
    return true;
}

/* Generic Target to host Msg/event  handler  for low priority messages
  Low priority message are handler in a different handler called from
  this function . So that the most likely succes path like Rx and
  Tx comp   has little code   foot print
 */
void
htt_t2h_msg_handler(void *context, HTC_PACKET *pkt)
{
    struct htt_pdev_t *pdev = (struct htt_pdev_t *) context;
    qdf_nbuf_t htt_t2h_msg = (qdf_nbuf_t) pkt->pPktContext;
    u_int32_t *msg_word;
    enum htt_t2h_msg_type msg_type;

    /* check for successful message reception */
    if (pkt->Status != QDF_STATUS_SUCCESS) {
        if (pkt->Status != QDF_STATUS_E_CANCELED) {
            pdev->stats.htc_err_cnt++;
        }
        qdf_nbuf_free(htt_t2h_msg);
        return;
    }

    /* confirm alignment */
    if ((((unsigned long) qdf_nbuf_data(htt_t2h_msg)) & 0x3) != 0) {
        qdf_err("Incorrect alignment %d",
                (((unsigned long) qdf_nbuf_data(htt_t2h_msg)) & 0x3));
        return;
    }

    msg_word = (u_int32_t *) qdf_nbuf_data(htt_t2h_msg);
    msg_type = HTT_T2H_MSG_TYPE_GET(*msg_word);
    switch (msg_type) {
    case HTT_T2H_MSG_TYPE_RX_IND:
        {
            unsigned num_mpdu_ranges;
            unsigned num_msdu_bytes;
            u_int16_t peer_id;
            u_int8_t tid;
            /* Validate that the length is good before reading the msg */
            if (qdf_nbuf_len(htt_t2h_msg) < HTT_RX_IND_MIN_LEN) {
                qdf_err("t2h message length %zu is less than min length",
                        qdf_nbuf_len(htt_t2h_msg));
                qdf_nbuf_free(htt_t2h_msg);
                return;
            }
            peer_id = HTT_RX_IND_PEER_ID_GET(*msg_word);
            tid = HTT_RX_IND_EXT_TID_GET(*msg_word);

            num_msdu_bytes = HTT_RX_IND_FW_RX_DESC_BYTES_GET(
                *(msg_word + 2 + HTT_RX_PPDU_DESC_SIZE32));
            /*
             * 1 word for the message header,
             * HTT_RX_PPDU_DESC_SIZE32 words for the FW rx PPDU desc
             * 1 word to specify the number of MSDU bytes,
             * 1 word for every 4 MSDU bytes (round up),
             * 1 word for the MPDU range header
             */
            pdev->rx_mpdu_range_offset_words =
                (HTT_RX_IND_HDR_BYTES + num_msdu_bytes + 3) >> 2;
            num_mpdu_ranges = HTT_RX_IND_NUM_MPDU_RANGES_GET(*(msg_word + 1));
            pdev->rx_ind_msdu_byte_idx = 0;
            if (!htt_t2h_is_mdpu_range_valid(qdf_nbuf_len(htt_t2h_msg),
                                             num_mpdu_ranges,
                                             num_msdu_bytes,
                                             pdev->rx_mpdu_range_offset_words)) {
                struct ol_txrx_pdev_t *txrx_pdev = (struct ol_txrx_pdev_t *)
                                                   pdev->txrx_pdev;
                txrx_pdev->pdev_data_stats.mpdu_cnt_err++;
                qdf_nbuf_free(htt_t2h_msg);
                return;
            }
            if (pdev->cfg.is_high_latency) {
                /*
                 * TODO: remove copy after stopping reuse skb on HIF layer
                 * because SDIO HIF may reuse skb before upper layer release it
                 */
                ol_rx_indication_handler(
                        pdev->txrx_pdev, htt_t2h_msg, peer_id, tid, num_mpdu_ranges);

                return;
            } else {
                ol_rx_indication_handler(
                        pdev->txrx_pdev, htt_t2h_msg, peer_id, tid, num_mpdu_ranges);
            }
            break;
        }
    case HTT_T2H_MSG_TYPE_TX_COMPL_IND:
        {
            qdf_warn("HTT_T2H_MSG_TYPE_TX_COMPL_IND not expected");
            break;
        }
    case HTT_T2H_MSG_TYPE_TX_INSPECT_IND:
        {
            qdf_warn("HTT_T2H_MSG_TYPE_TX_INSPECT_IND not expected");
            break;
        }
    default:
        htt_t2h_lp_msg_handler(context, htt_t2h_msg);
        return ;

    };
}

#ifdef WLAN_FEATURE_FASTPATH

#define OL_TX_LL_FAST_UPDATE_HTT_COMPLS(_num_htt_cmpls, _num_msdus) \
    _num_htt_cmpls +=  _num_msdus ;

void
htt_t2h_msg_handler_misc(struct htt_pdev_t *pdev, qdf_nbuf_t htt_t2h_msg,
                                        uint32_t *num_htt_cmpls)
{
    u_int32_t *msg_word = 0;
    enum htt_t2h_msg_type msg_type;

    msg_word = (u_int32_t *) qdf_nbuf_data(htt_t2h_msg);
    msg_type = HTT_T2H_MSG_TYPE_GET(*msg_word);
    switch (msg_type) {
    case HTT_T2H_MSG_TYPE_VERSION_CONF:
        {
            pdev->tgt_ver.major = HTT_VER_CONF_MAJOR_GET(*msg_word);
            pdev->tgt_ver.minor = HTT_VER_CONF_MINOR_GET(*msg_word);
            qdf_print(KERN_INFO"target uses HTT version %d.%d; host uses %d.%d",
                pdev->tgt_ver.major, pdev->tgt_ver.minor,
                HTT_CURRENT_VERSION_MAJOR, HTT_CURRENT_VERSION_MINOR);
            if (pdev->tgt_ver.major != HTT_CURRENT_VERSION_MAJOR) {
                qdf_print("*** Incompatible host/target HTT versions!");
            }
            /* abort if the target is incompatible with the host */
            qdf_assert(pdev->tgt_ver.major == HTT_CURRENT_VERSION_MAJOR);
            if (pdev->tgt_ver.minor != HTT_CURRENT_VERSION_MINOR) {
                qdf_print(
                    "*** Warning: host/target HTT versions are different, "
                    "though compatible!");
            }
            break;
        }
    case HTT_T2H_MSG_TYPE_RX_FLUSH:
        {
            break;
        }
    case  HTT_T2H_MSG_TYPE_RX_FRAG_IND:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_RX_ADDBA:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_RX_DELBA:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_PEER_MAP:
        {
            u_int8_t mac_addr_deswizzle_buf[QDF_MAC_ADDR_SIZE];
            u_int8_t *peer_mac_addr;
            u_int16_t peer_id;
            u_int8_t vdev_id;

            peer_id = HTT_RX_PEER_MAP_PEER_ID_GET(*msg_word);
            vdev_id = HTT_RX_PEER_MAP_VDEV_ID_GET(*msg_word);
            peer_mac_addr = htt_t2h_mac_addr_deswizzle(
                (u_int8_t *) (msg_word+1), &mac_addr_deswizzle_buf[0]);

            ol_rx_peer_map_handler((struct ol_txrx_pdev_t *)pdev->txrx_pdev,
				    peer_id, vdev_id, peer_mac_addr);
            break;
        }
    case HTT_T2H_MSG_TYPE_PEER_UNMAP:
        {
            u_int16_t peer_id;
            peer_id = HTT_RX_PEER_UNMAP_PEER_ID_GET(*msg_word);

            ol_rx_peer_unmap_handler((struct ol_txrx_pdev_t *)pdev->txrx_pdev, peer_id);
            break;
        }
    case HTT_T2H_MSG_TYPE_SEC_IND:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_TX_INSPECT_IND:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_MGMT_TX_COMPL_IND:
        {
            struct htt_mgmt_tx_compl_ind * compl_msg =
                                 (struct htt_mgmt_tx_compl_ind *)(msg_word + 1);
            qdf_nbuf_t tx_mgmt_frm = NULL;
            qdf_nbuf_t mgmt_frm_cpy = NULL;
            A_UINT32 *ppdu_id_ptr = NULL;

            /* tx capture related processing */
            if (compl_msg->status != IEEE80211_TX_ERROR) {
                tx_mgmt_frm =
                      pdev->tx_mgmt_desc_ctxt.pool[compl_msg->desc_id].mgmt_frm;
                if (tx_mgmt_frm &&
                     ((struct ol_txrx_pdev_t *)pdev->txrx_pdev)->tx_capture &&
                     pdev->htt_process_nondata_tx_frames) {
                    mgmt_frm_cpy = qdf_nbuf_copy(tx_mgmt_frm);
                    if (mgmt_frm_cpy) {
                        ppdu_id_ptr = (A_UINT32 *)qdf_nbuf_push_head(
                                      mgmt_frm_cpy, sizeof(compl_msg->ppdu_id));
                        *(ppdu_id_ptr) = compl_msg->ppdu_id;
                        /*
                         * frame will be freed after processing in the below
                         * callback
                         */
                        pdev->htt_process_nondata_tx_frames(mgmt_frm_cpy);
                    }
                }
                tx_mgmt_frm = NULL;
            }

            htt_tx_mgmt_desc_free(pdev, compl_msg->desc_id, compl_msg->status);
            OL_TX_LL_FAST_UPDATE_HTT_COMPLS(*num_htt_cmpls, 1);
            break;
        }
#if TXRX_STATS_LEVEL != TXRX_STATS_LEVEL_OFF
    case HTT_T2H_MSG_TYPE_STATS_CONF:
        {
            break;
        }
#endif
#ifndef REMOVE_PKT_LOG
    case HTT_T2H_MSG_TYPE_PKTLOG:
        {
            break;
        }
#endif

    case HTT_T2H_MSG_TYPE_EN_STATS:
        {
            break;
        }
    case HTT_T2H_MSG_TYPE_CHAN_CHANGE:
        {
            pdev->rs_freq = *(msg_word + 1);
            ol_ath_chan_change_msg_handler((struct ol_txrx_pdev_t *)pdev->txrx_pdev, msg_word, qdf_nbuf_len(htt_t2h_msg));

            break;
        }
    case HTT_T2H_MSG_TYPE_AGGR_CONF:
        {
            msg_word++;
            htt_pkt_buf_list_del(pdev,(u_int64_t)*msg_word);
            (*num_htt_cmpls)++;
        }
        break;
    case HTT_T2H_MSG_TYPE_STATS_NOUPLOAD:
        {
            u_int64_t cookie;

            cookie = *(msg_word + 1);
            cookie |= ((u_int64_t) (*(msg_word + 2))) << 32;

            htt_pkt_buf_list_del(pdev,cookie);
            (*num_htt_cmpls)++;
        }
        break;
#if PEER_FLOW_CONTROL && !PEER_FLOW_CONTROL_HOST_SCHED
    case HTT_T2H_MSG_TYPE_TX_MODE_SWITCH_IND:
        {
        }
        break;
    case HTT_T2H_MSG_TYPE_TX_FETCH_IND:
        {
        }
        break;
    case HTT_T2H_MSG_TYPE_TX_FETCH_CONF:
        {
        }
        break;
#endif

    default:
        break;
    };
}

#define HTT_T2H_MSG_BUF_REINIT(_buf, _qdf_dev)                              \
	do {                                                                \
		uint32_t paddr_lo; unsigned char *buf_end;	\
		paddr_lo = qdf_nbuf_get_frag_paddr((_buf), 0);  \
		buf_end = skb_end_pointer(_buf);	\
		qdf_nbuf_init((_buf));                                              \
		OS_SYNC_SINGLE((osdev_t)_qdf_dev->drv, paddr_lo,                          \
				buf_end - (_buf)->data,                             \
				BUS_DMA_FROMDEVICE, (dma_context_t)&paddr_lo);                    \
	} while(0)

void
htt_t2h_msg_handler_fast(void *htt_pdev, qdf_nbuf_t *nbuf_cmpl_arr,
        uint32_t num_cmpls)
{
    struct htt_pdev_t *pdev = (struct htt_pdev_t *)htt_pdev;
    struct ol_txrx_pdev_t *txrx_pdev = (struct ol_txrx_pdev_t *)pdev->txrx_pdev;
    qdf_nbuf_t htt_t2h_msg;
    u_int32_t *msg_word;
    uint32_t i;
    enum htt_t2h_msg_type msg_type;
    qdf_device_t qdf_dev = wlan_psoc_get_qdf_dev((void *)pdev->ctrl_psoc);
    uint32_t num_htt_cmpls = 0;
    uint32_t msg_len;
    HTC_FRAME_HDR *htc_hdr;

    for (i = 0; i < num_cmpls; i++) {

        htt_t2h_msg = nbuf_cmpl_arr[i];
        htc_hdr = (HTC_FRAME_HDR *)htt_t2h_msg;

        msg_len = qdf_nbuf_len(htt_t2h_msg);
        /* Move the data pointer to point to HTT header
         * past the HTC header + HTC header alignment padding
         */
        qdf_nbuf_pull_head(htt_t2h_msg, HTC_HEADER_LEN + HTC_HDR_ALIGNMENT_PADDING);

        msg_word = (u_int32_t *) qdf_nbuf_data(htt_t2h_msg);
        msg_type = HTT_T2H_MSG_TYPE_GET(*msg_word);

        /*
         * Process the data path messages first
         */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (pdev->nss_wifiol_ctx) {
            if ((msg_type == HTT_T2H_MSG_TYPE_TX_INSPECT_IND) ||
                    (msg_type == HTT_T2H_MSG_TYPE_TX_FETCH_IND) ||
                    (msg_type == HTT_T2H_MSG_TYPE_TX_FETCH_CONF) ||
                    (msg_type == HTT_T2H_MSG_TYPE_RX_IND))
            {
                qdf_nofl_err("NSS Wifi Offload : Unhandled %x\n", msg_type);
                qdf_nofl_err("msg_len : %zu htc_len : %d", qdf_nbuf_len(htt_t2h_msg),
                                        htc_hdr->PayloadLen);
                qdf_nbuf_free(htt_t2h_msg);
                continue;
            }
        }
#endif

        switch (msg_type) {
            case HTT_T2H_MSG_TYPE_RX_IND:
                {
                    unsigned num_mpdu_ranges;
                    unsigned num_msdu_bytes;
                    u_int16_t peer_id;
                    u_int8_t tid;

                    /* Validate that the length is good before reading the msg */
                    if (qdf_nbuf_len(htt_t2h_msg) < HTT_RX_IND_MIN_LEN) {
                        qdf_err("t2h message length %zu is less than min length",
                                qdf_nbuf_len(htt_t2h_msg));
                        qdf_nbuf_free(htt_t2h_msg);
                        return;
                    }
                    peer_id = HTT_RX_IND_PEER_ID_GET(*msg_word);
                    tid = HTT_RX_IND_EXT_TID_GET(*msg_word);

                    num_msdu_bytes = HTT_RX_IND_FW_RX_DESC_BYTES_GET(
                            *(msg_word + 2 + HTT_RX_PPDU_DESC_SIZE32));
                    /*
                     * 1 word for the message header,
                     * HTT_RX_PPDU_DESC_SIZE32 words for the FW rx PPDU desc
                     * 1 word to specify the number of MSDU bytes,
                     * 1 word for every 4 MSDU bytes (round up),
                     * 1 word for the MPDU range header
                     */
                    pdev->rx_mpdu_range_offset_words =
                        (HTT_RX_IND_HDR_BYTES + num_msdu_bytes + 3) >> 2;
                    num_mpdu_ranges = HTT_RX_IND_NUM_MPDU_RANGES_GET(*(msg_word + 1));
                    pdev->rx_ind_msdu_byte_idx = 0;
                    if (!htt_t2h_is_mdpu_range_valid(qdf_nbuf_len(htt_t2h_msg),
                                                     num_mpdu_ranges,
                                                     num_msdu_bytes,
                                                     pdev->rx_mpdu_range_offset_words)) {
                        struct ol_txrx_pdev_t *txrx_pdev = (struct ol_txrx_pdev_t *)
                                                           pdev->txrx_pdev;
                        txrx_pdev->pdev_data_stats.mpdu_cnt_err++;
                        qdf_nbuf_free(htt_t2h_msg);
                        return;
                    }
                    ol_rx_indication_handler(pdev->txrx_pdev,
                            htt_t2h_msg, peer_id, tid, num_mpdu_ranges);
                    break;
                }
            case HTT_T2H_MSG_TYPE_TX_COMPL_IND:
                {
                   qdf_warn("HTT_T2H_MSG_TYPE_TX_COMPL_IND not expected");
                    break;
                }
            default:
                /*
                 * Handle less frequent HTT messages here
                 */
                htt_t2h_msg_handler_misc(pdev, htt_t2h_msg, &num_htt_cmpls);
                break;
        }
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
        if (pdev->nss_wifiol_ctx) {
            qdf_nbuf_free(htt_t2h_msg);
        } else
#endif
        {
            /* Re-initialize the indication buffer */
            HTT_T2H_MSG_BUF_REINIT(htt_t2h_msg, qdf_dev);
        }
    }

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
    if (pdev->nss_wifiol_ctx) {
        return;
    }
#endif

    if(num_htt_cmpls) {
        HTT_TX_PDEV_LOCK(&txrx_pdev->tx_lock);
#if QCA_OL_TX_PDEV_LOCK
        hif_update_tx_ring(pdev->osc, num_htt_cmpls);
#endif
        HTT_TX_PDEV_UNLOCK(&txrx_pdev->tx_lock);
    }
}
qdf_export_symbol(htt_t2h_msg_handler_fast);
#endif /* WLAN_FEATURE_FASTPATH */

/*--- target->host HTT message Info Element access methods ------------------*/

/*--- tx completion message ---*/

u_int16_t
htt_tx_compl_desc_id(void *iterator, int num)
{
    /*
     * The MSDU IDs are packed , 2 per 32-bit word.
     * Iterate on them as an array of 16-bit elements.
     * This will work fine if the host endianness matches
     * the target endianness.
     * If the host endianness is opposite of the target's,
     * this iterator will produce descriptor IDs in a different
     * order than the target inserted them into the message -
     * if the target puts in [0, 1, 2, 3, ...] the host will
     * put out [1, 0, 3, 2, ...].
     * This is fine, except for the last ID if there are an
     * odd number of IDs.  But the TX_COMPL_IND handling code
     * in the htt_t2h_msg_handler already added a duplicate
     * of the final ID, if there were an odd number of IDs,
     * so this function can safely treat the IDs as an array
     * of 16-bit elements.
     */
    return *(((u_int16_t *) iterator) + num);
}

/*--- rx indication message ---*/

int
htt_rx_ind_flush(qdf_nbuf_t rx_ind_msg)
{
    u_int32_t *msg_word;

    msg_word = (u_int32_t *) qdf_nbuf_data(rx_ind_msg);
    return HTT_RX_IND_FLUSH_VALID_GET(*msg_word);
}

void
htt_rx_ind_flush_seq_num_range(
    qdf_nbuf_t rx_ind_msg,
    int *seq_num_start,
    int *seq_num_end)
{
    u_int32_t *msg_word;

    msg_word = (u_int32_t *) qdf_nbuf_data(rx_ind_msg);
    msg_word++;
    *seq_num_start = HTT_RX_IND_FLUSH_SEQ_NUM_START_GET(*msg_word);
    *seq_num_end   = HTT_RX_IND_FLUSH_SEQ_NUM_END_GET(*msg_word);
}

int
htt_rx_ind_release(qdf_nbuf_t rx_ind_msg)
{
    u_int32_t *msg_word;

    msg_word = (u_int32_t *) qdf_nbuf_data(rx_ind_msg);
    return HTT_RX_IND_REL_VALID_GET(*msg_word);
}

void
htt_rx_ind_release_seq_num_range(
    qdf_nbuf_t rx_ind_msg,
    int *seq_num_start,
    int *seq_num_end)
{
    u_int32_t *msg_word;

    msg_word = (u_int32_t *) qdf_nbuf_data(rx_ind_msg);
    msg_word++;
    *seq_num_start = HTT_RX_IND_REL_SEQ_NUM_START_GET(*msg_word);
    *seq_num_end   = HTT_RX_IND_REL_SEQ_NUM_END_GET(*msg_word);
}

void
htt_rx_ind_mpdu_range_info(
    struct htt_pdev_t *pdev,
    qdf_nbuf_t rx_ind_msg,
    int mpdu_range_num,
    enum htt_rx_status *status,
    int *mpdu_count)
{
    u_int32_t *msg_word;

    msg_word = (u_int32_t *) qdf_nbuf_data(rx_ind_msg);
    msg_word += pdev->rx_mpdu_range_offset_words + mpdu_range_num;
    *status = HTT_RX_IND_MPDU_STATUS_GET(*msg_word);
    *mpdu_count = HTT_RX_IND_MPDU_COUNT_GET(*msg_word);
}


/*--- stats confirmation message ---*/

void
htt_t2h_dbg_stats_hdr_parse(
    u_int8_t *stats_info_list,
    enum htt_dbg_stats_type *type,
    enum htt_dbg_stats_status *status,
    int *length,
    u_int8_t **stats_data)
{
    u_int32_t *msg_word = (u_int32_t *) stats_info_list;
    *type = HTT_T2H_STATS_CONF_TLV_TYPE_GET(*msg_word);
    *status = HTT_T2H_STATS_CONF_TLV_STATUS_GET(*msg_word);
    *length = HTT_T2H_STATS_CONF_TLV_HDR_SIZE +       /* header length */
        HTT_T2H_STATS_CONF_TLV_LENGTH_GET(*msg_word); /* data length */
    *stats_data = stats_info_list + HTT_T2H_STATS_CONF_TLV_HDR_SIZE;
}

void
htt_rx_frag_ind_flush_seq_num_range(
    qdf_nbuf_t rx_frag_ind_msg,
    int *seq_num_start,
    int *seq_num_end)
{
    u_int32_t *msg_word;

    msg_word = (u_int32_t *) qdf_nbuf_data(rx_frag_ind_msg);
    msg_word++;
    *seq_num_start = HTT_RX_FRAG_IND_FLUSH_SEQ_NUM_START_GET(*msg_word);
    *seq_num_end   = HTT_RX_FRAG_IND_FLUSH_SEQ_NUM_END_GET(*msg_word);
}

void
htt_t2h_dbg_enh_stats_hdr_parse(
    u_int32_t *stats_base,
    enum htt_t2h_en_stats_type *type,
    enum htt_t2h_en_stats_status *status)
{
    u_int32_t *msg_word;

    msg_word = stats_base + 1;

    *type = HTT_T2H_EN_STATS_CONF_TLV_TYPE_GET(*msg_word);
    *status = HTT_T2H_EN_STATS_CONF_TLV_STATUS_GET(*msg_word);
}

