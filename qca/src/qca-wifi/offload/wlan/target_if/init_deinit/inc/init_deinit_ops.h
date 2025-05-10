/*
 * Copyright (c) 2017-2018 Qualcomm Innovation Center, Inc.
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
/**
 * DOC: init_deinit_ops.h
 *
 * Public APIs to WIN ops
 */

#ifndef _INIT_DEINIT_OPS_H_
#define _INIT_DEINIT_OPS_H_

#define WLAN_LOW_6G_BAND_HIGHIEST_FREQ 6425
#define WLAN_HIGH_6G_BAND_LOWEST_FREQ 6425
#define WLAN_LOW_5G_BAND_HIGHIEST_FREQ 5490
#define WLAN_HIGH_5G_BAND_LOWEST_FREQ 5490

enum wlan_pdev_band_info {
    WLAN_PDEV_BAND_UNKNOWN,
    WLAN_2GHZ_PDEV,
    WLAN_5GHZ_LOW_BAND_PDEV,
    WLAN_5GHZ_HIGH_BAND_PDEV,
    WLAN_5GHZ_PDEV,
    WLAN_6GHZ_PDEV,
    WLAN_6GHZ_LOW_BAND_PDEV,
    WLAN_6GHZ_HIGH_BAND_PDEV,
};

#define WLAN_SOC_SINGLE_PHY_2G		(1 << WLAN_2GHZ_PDEV)
#define WLAN_SOC_SINGLE_PHY_5G		(1 << WLAN_5GHZ_PDEV)
#define WLAN_SOC_SINGLE_PHY_6G		(1 << WLAN_6GHZ_PDEV)
#define WLAN_SOC_SPLIT_PHY_2G_5G_LOW	((1 << WLAN_2GHZ_PDEV) | \
					 (1 << WLAN_5GHZ_LOW_BAND_PDEV))
#define WLAN_SOC_SPLIT_PHY_6G_5G_HIGH	((1 << WLAN_6GHZ_PDEV) | \
					 (1 << WLAN_5GHZ_HIGH_BAND_PDEV))

/**
 * init_deinit_register_featurs_ops()- To Register feature ops
 * @psoc: PSOC object
 *
 * API to register feature ops
 *
 *Return: SUCCESS on succesful registration of feature ops or FAILURE
 */
QDF_STATUS init_deinit_register_featurs_ops(struct wlan_objmgr_psoc *psoc);

/**
 * init_deinit_get_total_peers_for_pdev_id() - Get total number of peers from
 * cfg private context of psoc for given pdev id.
 * @psoc: PSOC object
 * @pdev_id : Pdev id
 *
 * This API can be called only before WMI_INIT_CMDID sent from host to FW.
 * After WMI_READY_EVENTID, call either wlan_pdev_get_max_peer_count()/
 * wlan_psoc_get_max_peer_count().
 *
 * Return: Sum of number of vdevs, montior vaps and peers for given pdev id.
 */
uint16_t init_deinit_get_total_peers_for_pdev_id(struct wlan_objmgr_psoc *psoc,
		uint16_t pdev_id);

/**
 * init_deinit_get_total_vdevs_for_pdev_id() - Get total number of vdevs from
 * cfg private context of psoc for given pdev id.
 * @psoc: PSOC object
 * @pdev_id : Pdev id
 *
 * This API can be called only before WMI_INIT_CMDID sent from host to FW.
 * After WMI_READY_EVENTID, call either wlan_pdev_get_max_vdev_count()/
 * wlan_psoc_get_max_vdev_count().
 *
 * Return: Sum of number of vdevs, montior vaps for given pdev id.
 */
uint8_t init_deinit_get_total_vdevs_for_pdev_id(struct wlan_objmgr_psoc *psoc,
		uint16_t pdev_id);

#endif /* _INIT_DEINIT_OPS_H_ */
