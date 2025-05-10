/*
 * Copyright (c) 2011, 2020, 2022 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_tx.h
 * @brief Internal definitions for the high-level tx module.
 */
#ifndef _OL_TX__H_
#define _OL_TX__H_

#include <qdf_nbuf.h>    /* qdf_nbuf_t */
#include <qdf_lock.h>
#include <cdp_txrx_cmn_struct.h> /* ol_txrx_vdev_handle */

qdf_nbuf_t
ol_tx_ll(struct cdp_soc_t *soc, uint8_t vdev_id, qdf_nbuf_t msdu_list);
#endif /* _OL_TX__H_ */
