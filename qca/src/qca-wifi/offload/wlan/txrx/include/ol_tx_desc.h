/*
 * Copyright (c) 2011, 2022 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_tx_desc.h
 * @brief API definitions for the tx descriptor module within the data SW.
 */
#ifndef _OL_TX_DESC__H_
#define _OL_TX_DESC__H_

#include <queue.h>         /* TAILQ_HEAD */
#include <qdf_nbuf.h>      /* qdf_nbuf_t */
#include <ol_txrx_types.h> /* ol_tx_desc_t */
#include <ol_txrx_internal.h> /* TXRX_STATS_ADD */
#include <cdp_txrx_ctrl_def.h>

/**
 * @brief Use a tx descriptor ID to find the corresponding desriptor object.
 *
 * @param pdev - the data physical device sending the data
 * @param tx_desc_id - the ID of the descriptor in question
 * @return the descriptor object that has the specified ID
 */
struct ol_tx_desc_t *
ol_tx_desc_find(struct ol_txrx_pdev_t *pdev, u_int16_t tx_desc_id);

/**
 * @brief Free a list of tx descriptors and the tx frames they refer to.
 * @details
 *  Free a batch of "standard" tx descriptors and their tx frames.
 *  Free each tx descriptor, by returning it to the freelist.
 *  Unmap each netbuf, and free the netbufs as a batch.
 *  Irregular tx frames like TSO or managment frames that require
 *  special handling are processed by the ol_tx_desc_frame_free_nonstd
 *  function rather than this function.
 *
 * @param pdev - the data physical device that sent the data
 * @param tx_descs - a list of SW tx descriptors for the tx frames
 * @param had_error - boolean indication of whether the transmission failed.
 *            This is provided to callback functions that get notified of
 *            the tx frame completion.
 */
void ol_tx_desc_frame_list_free(
    struct ol_txrx_pdev_t *pdev,
    ol_tx_desc_list *tx_descs,
    int had_error);

/**
 * @brief Free a non-standard tx frame and its tx descriptor.
 * @details
 *  Check the tx frame type (e.g. TSO vs. management) to determine what
 *  special steps, if any, need to be performed prior to freeing the
 *  tx frame and its tx descriptor.
 *  This function can also be used to free single standard tx frames.
 *  After performing any special steps based on tx frame type, free the
 *  tx descriptor, i.e. return it to the freelist, and unmap and
 *  free the netbuf referenced by the tx descriptor.
 *
 * @param pdev - the data physical device that sent the data
 * @param tx_desc - the SW tx descriptor for the tx frame that was sent
 * @param had_error - boolean indication of whether the transmission failed.
 *            This is provided to callback functions that get notified of
 *            the tx frame completion.
 */
void ol_tx_desc_frame_free_nonstd(
    struct ol_txrx_pdev_t *pdev,
    struct ol_tx_desc_t *tx_desc,
    int had_error);

/*
 * @brief Retrieves the beacon headr for the vdev
 * @param pdev - opaque pointe to scn
 * @param vdevid - vdev id
 * @return void pointer to the beacon header for the given vdev
 */

void *
ol_ath_get_bcn_header(ol_pdev_handle pdev, A_UINT32 vdev_id);

/*
 * @brief Retrieves the beacon buffer for the vdev
 * @param pdev - opaque pointe to scn
 * @param vdevid - vdev id
 * @return void pointer to the beacon buffer for the given vdev
 */

void *
ol_ath_get_bcn_buffer(ol_pdev_handle pdev, A_UINT32 vdev_id);

/*
 * @brief Dump the details associated with a tx descriptor.
 * @details
 *  This will dump the members of the tx_desc.
 * @param tx_desc - the descriptor whose details has to be dumped
 */
void ol_dump_txdesc(struct ol_tx_desc_t *tx_desc);

/*
 * @brief Determine the ID of a tx descriptor.
 *
 * @param pdev - the data physical device that is sending the data
 * @param tx_desc - the descriptor whose ID is being determined
 * @return numeric ID that uniquely identifies the tx descriptor
 */
static inline u_int16_t
ol_tx_desc_id(struct ol_txrx_pdev_t *pdev, struct ol_tx_desc_t *tx_desc)
{
    return (u_int16_t)
        ((union ol_tx_desc_list_elem_t *)tx_desc - pdev->tx_desc.array);
}

static inline void
ol_tx_desc_free(struct ol_txrx_pdev_t *pdev, struct ol_tx_desc_t *tx_desc)
{
    OL_TX_DESC_LOCK(&pdev->tx_mutex);
    tx_desc->allocated = 0;
    ((union ol_tx_desc_list_elem_t *) tx_desc)->next = pdev->tx_desc.freelist;
    pdev->tx_desc.freelist = (union ol_tx_desc_list_elem_t *) tx_desc;
    OL_TXRX_STATS_SUB(pdev, tx.desc_in_use, 1);
    /* Clear TSO Info part of htt_frag_desc,
     * to avoid unexpected usage of it for non-tso pkts in reallocation of the descriptor */
    HTT_TX_DESC_CLEAR_TSO_INFO((u_int32_t *)(tx_desc->htt_frag_desc));
#if PEER_FLOW_CONTROL
    pdev->pflow_ctl_desc_count--;
#endif
    OL_TX_DESC_UNLOCK(&pdev->tx_mutex);
}

#endif /* _OL_TX_DESC__H_ */
