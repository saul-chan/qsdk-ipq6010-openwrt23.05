/*
 * Copyright (c) 2017-2018, 2020-2022 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#ifndef _DP_TXRX_H
#define _DP_TXRX_H

#include "dp_extap_mitbl.h"
#include "dp_me.h"
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <cdp_txrx_cmn.h>
#if ATH_SUPPORT_WRAP
#include "dp_wrap_struct.h"
#endif

/* Opaque type for rawmode packet simulation */
struct rawmode_sim_ctxt;
typedef rwlock_t extap_devt_lock_t;

typedef struct dp_pdev_extap {
	mi_node_t *miroot;    /* EXTAP MAC - IP table Root */
	extap_devt_lock_t        mi_lock;   /*lock for dev table*/
} dp_pdev_extap_t;

typedef struct dp_txrx_pdev_handle {
	dp_pdev_extap_t extap_hdl; /* Extap handler */
	dp_pdev_me_t pdev_me_hdl; /* Pdev ME Handle */
#if ATH_SUPPORT_WRAP
	dp_pdev_wrap_t wrap_pdev_hdl; /*Pdev wrap handle*/
#endif
} dp_txrx_pdev_handle_t;

typedef struct dp_vdev_txrx_handle {
	dp_vdev_me_t vdev_me;
	struct rawmode_sim_ctxt *rsim_ctxt;
	dp_vdev_igmp_me_t vdev_igmp_me;
#if ATH_SUPPORT_WRAP
	dp_vdev_wrap_t wrap_vdev_hdl; /*vdev wrap handle*/
#endif
} dp_vdev_txrx_handle_t;

static inline QDF_STATUS dp_vdev_ext_attach(ol_txrx_soc_handle soc, uint8_t vdev_id, uint8_t *macaddr)
{
    return dp_me_attach(soc, vdev_id, macaddr);
}

/**
 *dp_get_vdev_me_handle() - get ME handle from vdev
 *@soc: Datapath soc handle
 *@vdev_id: vdev id
 *
 *Return: ME handle
 */
static inline dp_vdev_me_t *dp_get_vdev_me_handle(ol_txrx_soc_handle soc,
                                                  uint8_t vdev_id)
{
    dp_vdev_txrx_handle_t *dp_hdl;

    if (!soc)
        return NULL;

    dp_hdl = cdp_vdev_get_dp_ext_txrx_handle(soc, vdev_id);
    if (!dp_hdl)
        return NULL;

    return &dp_hdl->vdev_me;
}

/**
 *dp_get_vdev_rawmode_sim_ctxt() - get rawmode packet simulation ctxt from vdev
 *@soc: Datapath soc handle
 *@vdev_id: vdev id
 *
 *Return: rawmode_sim_ctxt
 */
static inline struct rawmode_sim_ctxt *
dp_get_vdev_rawmode_sim_ctxt(ol_txrx_soc_handle soc, uint8_t vdev_id)
{
    dp_vdev_txrx_handle_t *dp_hdl;

    if (!soc)
        return NULL;

    dp_hdl = cdp_vdev_get_dp_ext_txrx_handle(soc, vdev_id);
    if (!dp_hdl)
        return NULL;

    return dp_hdl->rsim_ctxt;
}

/**
 *dp_get_vdev_igmp_me_handle() - get IGMP ME handle from vdev
 *@soc: Datapath soc handle
 *@vdev_id: vdev id
 *
 *Return: IGMP ME handle
 */
static inline dp_vdev_igmp_me_t *
dp_get_vdev_igmp_me_handle(ol_txrx_soc_handle soc,
                           uint8_t vdev_id)
{
    dp_vdev_txrx_handle_t *dp_hdl;

    if (!soc)
        return NULL;

    dp_hdl = cdp_vdev_get_dp_ext_txrx_handle(soc, vdev_id);
    if (!dp_hdl)
        return NULL;

    return &dp_hdl->vdev_igmp_me;
}

/**
 * dp_get_pdev_me_handle() - get pdev ME handle
 * @soc: soc txrx handle
 * @pdev_id: pdev id
 *
 * Return: pdev ME handle
 */
static inline dp_pdev_me_t *dp_get_pdev_me_handle(ol_txrx_soc_handle soc,
                                                  uint8_t pdev_id)
{
    dp_txrx_pdev_handle_t *dp_hdl;

    if (!soc)
        return NULL;

    dp_hdl = cdp_pdev_get_dp_txrx_handle(soc, pdev_id);

    if (!dp_hdl)
        return NULL;

    return &dp_hdl->pdev_me_hdl;
}

/**
 * dp_set_igmp_me_mode() - set if IGMP ME mode
 * @soc: soc txrx handle
 * @vdev_id: vdev_id
 * @mode: IGMP ME mode
 * @mac: vdev mac address
 * Return: void
 */
static inline void
dp_set_igmp_me_mode(ol_txrx_soc_handle soc, uint8_t vdev_id, uint8_t mode,
                    uint8_t *mac)
{
    dp_vdev_me_t *vdev_me;
    dp_vdev_igmp_me_t *vdev_igmp_me;

    vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

    if (!vdev_me)
        return;

    vdev_igmp_me = dp_get_vdev_igmp_me_handle(soc, vdev_id);

    if (!vdev_igmp_me)
        return;

    vdev_me->me_igmp_allow = vdev_igmp_me->igmp_me_enabled = mode;

    if (mac)
        qdf_mem_copy(vdev_igmp_me->macaddr, mac, QDF_MAC_ADDR_SIZE);
}

/**
 * dp_get_igmp_me_mode() - get if IGMP ME mode
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 *
 * Return: IGMP ME mode
 */
static inline uint8_t
dp_get_igmp_me_mode(ol_txrx_soc_handle soc, uint8_t vdev_id)
{
    dp_vdev_igmp_me_t *vdev_igmp_me;

    vdev_igmp_me = dp_get_vdev_igmp_me_handle(soc, vdev_id);

    if (!vdev_igmp_me) {
        return 0;
    }

    return vdev_igmp_me->igmp_me_enabled;
}

/**
 * dp_get_me_mcast_table() - get ME mcast_table
 * @soc: soc txrx handle
 * @vdev_id: vdev id
 * @flag : flag to select mcast table
 *
 * Return: ME mcast table handle
 */
static inline
struct dp_me_mcast_table *dp_get_me_mcast_table(ol_txrx_soc_handle soc,
                                                uint8_t vdev_id, uint32_t flag)
{
    dp_vdev_txrx_handle_t *dp_hdl;
    struct dp_me_mcast_table *table = NULL;

    dp_hdl = cdp_vdev_get_dp_ext_txrx_handle(soc, vdev_id);

    if (!dp_hdl)
        return table;

#ifdef WLAN_FEATURE_11BE_MLO
    if (flag & DP_ME_MCTBL_MLD_VDEV)
        table = &(dp_hdl->vdev_me.mld_me_snoop_table.mcast_table);
    else
#endif
    {
	    (void)flag;
	    table = &(dp_hdl->vdev_me.me_snoop_table.mcast_table);
    }

    return table;
}

/**
 * dp_set_me_mode() - set if ME mode
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 * @mode : ME mode
 *
 * Return: void
 */

static inline void dp_set_me_mode(ol_txrx_soc_handle soc, uint8_t vdev_id, uint8_t mode)
{
   dp_vdev_me_t *vdev_me;

   vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

   if (!vdev_me)
       return;

   vdev_me->me_mcast_mode = mode;
}

/**
 * dp_get_me_mode() - get if ME mode
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 *
 * Return: ME mode
 */

static inline uint8_t dp_get_me_mode(ol_txrx_soc_handle soc, uint8_t vdev_id)
{
   dp_vdev_me_t *vdev_me;

   vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

   if (!vdev_me)
       return MC_ME_DISABLE;

   return vdev_me->me_mcast_mode;
}

/**
 * dp_set_me_dms_support_needed_in_sw() - set if dms_support_needed or not
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 * @dms_amsdu_war : dms_support
 *
 * Return: void
 */

static inline void dp_set_me_dms_support_needed_in_sw(ol_txrx_soc_handle soc,
						      uint8_t vdev_id,
						      uint8_t dms_support)
{
	dp_vdev_me_t *vdev_me;

	vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

	if (!vdev_me)
		return;

	vdev_me->me_dms_support = dms_support;
}

/**
 * dp_get_me_dms_support_needed_in_sw() - get if dms_support_needed or not
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 *
 * Return: dms_support
 */

static inline uint8_t dp_get_me_dms_support_needed_in_sw(ol_txrx_soc_handle soc,
							 uint8_t vdev_id)
{
	dp_vdev_me_t *vdev_me;

	vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

	if (!vdev_me)
		return MC_ME_DISABLE;

	return vdev_me->me_dms_support;
}

/**
 * dp_get_me_mcast_lock() - get ME lock
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 * @flag : flag to select mcast table
 *
 * Return: me lock pointer
 */
static inline
rwlock_t *dp_get_me_mcast_lock(ol_txrx_soc_handle soc, uint8_t vdev_id, uint32_t flag)
{
    dp_vdev_me_t *vdev_me;
    rwlock_t *lock;

    vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

    if (!vdev_me)
       return NULL;

#ifdef WLAN_FEATURE_11BE_MLO
    if (flag & DP_ME_MCTBL_MLD_VDEV)
        lock = &(vdev_me->mld_me_snoop_table.lock);
    else
#endif
    {
       (void)flag;
       lock = &(vdev_me->me_snoop_table.lock);
    }

    return lock;
}

#endif /* _DP_TXRX_H */
