/*
 * Copyright (c) 2012, 2022 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _OL_TXRX__H_
#define _OL_TXRX__H_

#include <qdf_nbuf.h> /* qdf_nbuf_t */
#include <cdp_txrx_cmn_struct.h> /* ol_txrx_vdev_t, etc. */
#include <ol_txrx_types.h> /* ol_txrx_vdev_t, etc. */
#include <ol_tx_desc.h>


void
ol_txrx_peer_unref_delete(ol_txrx_peer_handle peer);

void
ol_txrx_aggregate_vdev_stats(struct ol_txrx_vdev_t *vdev, struct cdp_vdev_stats *stats);

#if PEER_FLOW_CONTROL
#if QCA_SUPPORT_WDS_EXTENDED && !QCA_NSS_PLATFORM
void
ol_txrx_classify(void *vdev, qdf_nbuf_t nbuf, enum txrx_direction dir,
                 struct ol_txrx_nbuf_classify *nbuf_class, uint8_t override_peer);
#else
void
ol_txrx_classify(void *vdev, qdf_nbuf_t nbuf,
        enum txrx_direction dir, struct ol_txrx_nbuf_classify *nbuf_class);
#endif
#endif
void ol_iterate_update_peer_list(struct cdp_pdev *pdev_hdl);

#endif /* _OL_TXRX__H_ */
