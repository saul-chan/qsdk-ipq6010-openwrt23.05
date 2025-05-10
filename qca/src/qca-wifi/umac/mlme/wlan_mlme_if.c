/*
 * Copyright (c) 2011-2014, 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2014 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#include <cdp_txrx_ctrl.h>
#include <wlan_mlo_mgr_setup.h>
#endif
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
#include <telemetry_agent.h>
#endif
#include <scheduler_api.h>
#include <wlan_dfs_ucfg_api.h>
#include <dfs.h>
#include <wlan_dfs_mlme_api.h>
#include <wlan_vdev_mlme_ser_if.h>
#include <wlan_mlme_if.h>
#include <wlan_vdev_mgr_utils_api.h>
#include <wlan_mlme_vdev_mgmt_ops.h>
#include <wlan_pdev_mlme.h>
#include <wlan_utility.h>
#include <ieee80211_acs_internal.h>
#include <wlan_cm_api.h>
#include <wlan_if_mgr_public_struct.h>
#include <wlan_mbss.h>
#include <wlan_mlme_dispatcher.h>
#include <qdf_threads.h>

#if UMAC_SUPPORT_CFG80211
#include <ieee80211_cfg80211.h>
#endif
#include <wlan_mlo_mgr_sta.h>
#include <wlan_mlo_mgr_peer.h>
#ifdef WLAN_CONFIG_TELEMETRY_AGENT
#include <wlan_telemetry_agent.h>
#include <cfg80211_external.h>
#include <telemetry_agent_wifi_driver_if.h>
#include <telemetry_agent_app_if.h>
#include <wlan_reg_services_api.h>
#include <cdp_txrx_host_stats.h>
#endif
#include <ieee80211_cfg80211.h>
#include <ol_if_athvar.h>
#ifdef CONFIG_SAWF
#include <cdp_txrx_sawf.h>
#endif
#include <cfg_ucfg_api.h>
#include <ieee80211_ucfg.h>

extern ol_ath_soc_softc_t *ol_global_soc[GLOBAL_SOC_SIZE];

QDF_STATUS mlme_vdev_add_cmd_to_pq(struct wlan_objmgr_vdev *vdev,
                                   enum wlan_serialization_cmd_type type,
                                   uint8_t high_prio);

#ifdef WLAN_FEATURE_11BE
int wlan_mlme_send_sta_cap_change(struct wlan_objmgr_peer *, enum qca_wlan_vendor_attr_app_generic_category category);
int wlan_mlme_send_t2lm_event(struct wlan_objmgr_peer *, struct wlan_t2lm_onging_negotiation_info *, enum qca_wlan_vendor_t2lm_category category, bool);
#endif

static int8_t wlan_mlme_calculate_link_rssi(struct ieee80211_node *ni, struct wlan_objmgr_peer *active_peer);
#define PSOC_VDEV_ID "PSOC:%u VDEV:%u "

#if SM_ENG_HIST_ENABLE
#include <wlan_sm_engine_dbg.h>

#define VDEV_MLME_LINE "-----------------------------------"\
		       "-----------------------------------"\
		       "-----------------------------------"

void wlan_mlme_print_vdev_sm_history(struct wlan_objmgr_psoc *psoc,
					    void *obj,
					    void *arg)
{
	struct vdev_mlme_obj *vdev_mlme = NULL;
	struct wlan_sm *vdev_sm = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	uint8_t psoc_index;

	psoc_index = *((uint8_t *)arg);

	vdev = (struct wlan_objmgr_vdev *)obj;
	if (!vdev) {
		mlme_err("VDEV is null");
		goto error;
	}
	vdev_mlme = wlan_objmgr_vdev_get_comp_private_obj(
			vdev,
			WLAN_UMAC_COMP_MLME);
	if (!vdev_mlme) {
		mlme_err("VDEV MLME is null");
		goto error;
	}

	vdev_sm = vdev_mlme->sm_hdl;

	if (!vdev_sm) {
		mlme_err("VDEV SM is null");
		goto error;
	}

	mlme_nofl_err(VDEV_MLME_LINE);
	mlme_nofl_err("SM History: "PSOC_VDEV_ID,
		      psoc_index,
		      wlan_vdev_get_id(vdev));
	mlme_nofl_err(VDEV_MLME_LINE);
	wlan_sm_print_history(vdev_sm);

error:
	return;
}
qdf_export_symbol(wlan_mlme_print_vdev_sm_history);

void wlan_mlme_psoc_iterate_handler(
		struct wlan_objmgr_psoc *psoc,
		void *arg, uint8_t index)
{
	uint8_t psoc_index = index;
	wlan_objmgr_iterate_obj_list(psoc, WLAN_VDEV_OP,
				     wlan_mlme_print_vdev_sm_history,
				     &psoc_index, false, WLAN_MLME_NB_ID);

	return;
}

#if SM_HIST_DEBUGFS_SUPPORT
#define VDEV_MLME_LINE_PROC "-----------------------------------"\
		       "-----------------------------------"\
		       "-----------------------------------\n"
struct sm_psoc {
	struct seq_file *m;
	uint8_t psoc_index;
};


void wlan_mlme_print_vdev_sm_fs_history(struct wlan_objmgr_psoc *psoc,
					    void *obj,
					    void *arg)
{
	struct vdev_mlme_obj *vdev_mlme = NULL;
	struct wlan_sm *vdev_sm = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	uint8_t psoc_index;
	struct sm_psoc *psoc_dbg;

	psoc_dbg = ((struct sm_psoc *)arg);

	psoc_index = psoc_dbg->psoc_index;

	vdev = (struct wlan_objmgr_vdev *)obj;
	if (!vdev) {
		mlme_err("VDEV is null");
		goto error;
	}
	vdev_mlme = wlan_objmgr_vdev_get_comp_private_obj(
			vdev,
			WLAN_UMAC_COMP_MLME);
	if (!vdev_mlme) {
		mlme_err("VDEV MLME is null");
		goto error;
	}

	vdev_sm = vdev_mlme->sm_hdl;

	if (!vdev_sm) {
		mlme_err("VDEV SM is null");
		goto error;
	}

	seq_printf(psoc_dbg->m, VDEV_MLME_LINE_PROC);

	seq_printf(psoc_dbg->m, "SM History: "PSOC_VDEV_ID,
		      psoc_index,
		      wlan_vdev_get_id(vdev));
	seq_printf(psoc_dbg->m, VDEV_MLME_LINE_PROC);
	wlan_sm_print_fs_history(vdev_sm, psoc_dbg->m);

error:
	return;
}

void wlan_mlme_psoc_fs_iterate_handler(
		struct wlan_objmgr_psoc *psoc,
		void *arg, uint8_t index)
{
	struct sm_psoc psoc_dbg;

	psoc_dbg.psoc_index = index;
	psoc_dbg.m = (struct seq_file *)arg;
	wlan_objmgr_iterate_obj_list(psoc, WLAN_VDEV_OP,
				     wlan_mlme_print_vdev_sm_fs_history,
				     &psoc_dbg, false, WLAN_MLME_NB_ID);

	return;
}

int
wlan_mlme_sm_debugfs_history(qdf_debugfs_file_t m, void *arg)
{
	wlan_objmgr_iterate_psoc_list(
			wlan_mlme_psoc_fs_iterate_handler,
			(void *)m,
			WLAN_MLME_NB_ID);

	return 0;
}
#endif

void wlan_mlme_print_all_sm_history(void)
{
	wlan_objmgr_iterate_psoc_list(
			wlan_mlme_psoc_iterate_handler,
			NULL,
			WLAN_MLME_NB_ID);
}
#endif

QDF_STATUS mlme_is_active_cmd_in_sched_ctx(void *umac_cmd)
{
    struct wlan_mlme_ser_data *data = (struct wlan_mlme_ser_data *)umac_cmd;

    if (!data) {
        wlan_mlme_err("Null data");
        return QDF_STATUS_E_INVAL;
    }

    if (data->activation_ctx == MLME_CTX_SCHED)
        return QDF_STATUS_SUCCESS;
    else
        return QDF_STATUS_E_FAILURE;
}


#ifdef WLAN_FEATURE_11BE_MLO
QDF_STATUS mlme_mlo_validate_partnerinfo(struct wlan_mlme_mlo_info *ml_info,
        wlan_if_t vap, uint8_t *link_id)
{
    struct wlan_mlo_dev_context *mlo_dev_ctx = NULL;
    struct wlan_objmgr_vdev *tmp_vdev = NULL;
    struct qdf_mac_addr tmp_mlo_link_mac;
    int i;
    uint8_t tmp_link_id = 0;
    bool linkid_found = false;

    if ((vap == NULL) || (ml_info == NULL) || (link_id == NULL))
        return QDF_STATUS_E_INVAL;

    mlo_dev_ctx = vap->vdev_obj->mlo_dev_ctx;
    mlo_info("Num MLO Links: %d", ml_info->num_mlo_links);

    /* Validate number of links */

    if (mlo_dev_ctx->wlan_vdev_count < ml_info->num_mlo_links) {
        mlo_err("Num links indicated by user space/hostapd %d vs. Max supported links %d",
                ml_info->num_mlo_links, mlo_dev_ctx->wlan_vdev_count);
        return QDF_STATUS_E_INVAL;
    }

    for (i = 0; i < ml_info->num_mlo_links; i++) {

        mlo_info("LinkID: %d LinkMAC: %s", ml_info->mlo_link_ids[i],
                ether_sprintf(ml_info->mlo_mac_addrs[i].bytes));

        if (IEEE80211_ADDR_EQ(ml_info->mlo_mac_addrs[i].bytes, vap->iv_myaddr)) {
            linkid_found = true;
            tmp_link_id = ml_info->mlo_link_ids[i];
        }

        /* Check MLO link MAC addresses from user space present in mlo_dev_ctx */
        IEEE80211_ADDR_COPY(&tmp_mlo_link_mac, ml_info->mlo_mac_addrs[i].bytes);
        tmp_vdev = mlo_get_ml_vdev_by_mac(vap->vdev_obj, &tmp_mlo_link_mac);
        if (!tmp_vdev) {
            mlo_err("ml_mac %s from user space is not present in mlo_dev_ctx",
                    ether_sprintf(ml_info->mlo_mac_addrs[i].bytes));
            return QDF_STATUS_E_INVAL;
        }
        /* For Link add case, mcast vdev is set tp false, since mcast vdev should have
           configured already on initial MLD bring up
         */
        if (!(wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_ADD))) {
            if (i == 0)
                wlan_mlme_mlo_set_mcast_vdev(tmp_vdev, true);
        }
    }

    if (!linkid_found) {
        mlo_err("linkid not found for vap-%d (%s)", vap->iv_unit, vap->iv_netdev_name);
        mlo_err("dumping user/hostpad config");
        for (i = 0; i < ml_info->num_mlo_links; i++) {
            mlo_err("LinkID: %d LinkMAC: %s", ml_info->mlo_link_ids[i],
                    ether_sprintf(ml_info->mlo_mac_addrs[i].bytes));
        }
        return QDF_STATUS_E_INVAL;
    }

    *(link_id) = tmp_link_id;

    return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_MCAST_MLO
void wlan_mlme_mlo_set_mcast_vdev(struct wlan_objmgr_vdev *vdev, bool mcast_vdev)
{
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_psoc *psoc;
    uint8_t vdev_id;
    cdp_config_param_type val = {0};

    vdev_id = wlan_vdev_get_id(vdev);
    psoc = wlan_vdev_get_psoc(vdev);
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

    if (mcast_vdev)
        wlan_vdev_mlme_feat_ext2_cap_set(vdev, WLAN_VDEV_FEXT2_MLO_MCAST);
    else
        wlan_vdev_mlme_feat_ext2_cap_clear(vdev, WLAN_VDEV_FEXT2_MLO_MCAST);

    val.cdp_vdev_param_mcast_vdev = mcast_vdev;
    cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id,
                            CDP_SET_MCAST_VDEV, val);
}
#endif

QDF_STATUS wlan_mlme_mlo_remove_vap_partner_mlinfo(wlan_if_t vap)
{
    int i, j;
    struct wlan_mlme_mlo_info *ml_info = NULL;
    struct wlan_mlme_mlo_info new_ml_info;
    struct qdf_mac_addr *mld_addr;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    wlan_if_t link_vap;
    uint16_t num_links = 0;

    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
    if (qdf_is_macaddr_zero(mld_addr)) {
        return QDF_STATUS_SUCCESS;
    }

    mlo_ap_get_vdev_list(vap->vdev_obj, &num_links, vdev_list);

    for (j = 0; j < num_links; j++) {
         if (!vdev_list[j])
             continue;

        link_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[j]);
        if (!link_vap || (link_vap == vap)) {
            continue;
        }

        qdf_mem_zero(&new_ml_info, sizeof(struct wlan_mlme_mlo_info));
        ml_info = &link_vap->mlo_info;
        for (i = 0; i < ml_info->num_mlo_links; i++) {
            /* Skip removal vap details */
            if (IEEE80211_ADDR_EQ(ml_info->mlo_mac_addrs[i].bytes,
                                  vap->iv_myaddr)) {
                continue;
            }
            new_ml_info.mlo_link_ids[new_ml_info.num_mlo_links] =
                ml_info->mlo_link_ids[i];

            IEEE80211_ADDR_COPY(
                    &new_ml_info.mlo_mac_addrs[new_ml_info.num_mlo_links].bytes,
                    ml_info->mlo_mac_addrs[i].bytes);

            new_ml_info.num_mlo_links++;
        }

        qdf_mem_zero(ml_info, sizeof(struct wlan_mlme_mlo_info));
        qdf_mem_copy(ml_info, &new_ml_info, sizeof(struct wlan_mlme_mlo_info));
    }

    for (i = 0; i < num_links; i++)
         mlo_release_vdev_ref(vdev_list[i]);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_mlo_update_partner_info(wlan_if_t vap)
{
    uint8_t linkid = 0;
    int i;
    struct wlan_mlme_mlo_info *ml_info = NULL;
    struct qdf_mac_addr tmp_mlo_link_mac;
    struct wlan_objmgr_vdev *tmp_vdev, *vdev;
    struct qdf_mac_addr *mld_addr;
    enum ieee80211_phymode mode;
    struct wlan_objmgr_psoc *psoc = NULL;

    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
    if (qdf_is_macaddr_zero(mld_addr)) {
        return QDF_STATUS_SUCCESS;
    }

    vdev = vap->vdev_obj;
    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        mlo_err("Failed to get psoc");
	return QDF_STATUS_E_FAILURE;
    }
    ml_info = &vap->mlo_info;
    mode = wlan_get_desired_phymode(vap);
    if (!ml_info->num_mlo_links && (mode >= IEEE80211_MODE_11NA_HT20) &&
        (mode < IEEE80211_MODE_11BEA_EHT20))
        return QDF_STATUS_SUCCESS;

    if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap) && (mlme_mlo_validate_partnerinfo(
                    ml_info, vap, &linkid) != QDF_STATUS_SUCCESS)) {
        return QDF_STATUS_E_INVAL;
    }

    mlo_info("mlo_ap_vdev_attach with LinkID: %d num_links: %d",
             linkid, ml_info->num_mlo_links);

    /*
     * In ML reconfig AP removal case, we are detaching VDEV from DP MLD.
     *
     * If the same VDEV gets added to the CP MLD via AP addition without
     * going through VDEV create path, attach that VDEV to the DP MLD
     */
    if (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_ADD)) {
        if (cdp_mlo_dev_ctxt_attach(wlan_psoc_get_dp_handle(psoc),
                                    wlan_vdev_get_id(vdev),
				    (uint8_t *)mld_addr)
                                    != QDF_STATUS_SUCCESS) {
            mlo_err("Failed to attach DP vdev  (" QDF_MAC_ADDR_FMT ") to"
                    " MLO Dev ctxt (" QDF_MAC_ADDR_FMT ")",
		    QDF_MAC_ADDR_REF(wlan_vdev_mlme_get_macaddr(vdev)),
		    QDF_MAC_ADDR_REF(mld_addr->bytes));
            }
    }

    mlo_ap_vdev_attach(vdev, linkid, ml_info->num_mlo_links);

    for (i = 0; i < ml_info->num_mlo_links; i++) {
        if (!IEEE80211_ADDR_EQ(ml_info->mlo_mac_addrs[i].bytes, vap->iv_myaddr)) {
            IEEE80211_ADDR_COPY(&tmp_mlo_link_mac, ml_info->mlo_mac_addrs[i].bytes);
            tmp_vdev = mlo_get_ml_vdev_by_mac(vdev, &tmp_mlo_link_mac);
            mlo_debug("Set WLAN_VDEV_FEXT2_MLO for vdev linkmac: %s",
                      ether_sprintf(ml_info->mlo_mac_addrs[i].bytes));
            wlan_vdev_mlme_set_mlo_vdev(tmp_vdev);
        }
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_mlo_validate_ap_phymode(wlan_if_t vap)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_mlme_mlo_info *ml_info = NULL;
    struct qdf_mac_addr *mld_addr = NULL;
    enum ieee80211_phymode mode;

    if (!vap) {
        mlo_err("vap is NULL");
        return QDF_STATUS_E_INVAL;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        mlo_err("vdev is NULL");
        return QDF_STATUS_E_INVAL;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        mlo_err("pdev is NULL");
        return QDF_STATUS_E_INVAL;
    }

    /* Acquire lock as required by wlan_vdev_mlme_get_mldaddr() */
    wlan_vdev_obj_lock(vdev);
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
    if (qdf_is_macaddr_zero(mld_addr)) {
        wlan_vdev_obj_unlock(vdev);
        return QDF_STATUS_SUCCESS;
    }
    wlan_vdev_obj_unlock(vdev);

    mode = wlan_get_desired_phymode(vap);

    ml_info = &vap->mlo_info;

    /* Userspace/hostapd requests for MLO by setting the number of MLO links to
     * 1 or more. (The number of MLO links can be 1 for single link MLO). The
     * request would be invalid if the number of MLO links is 1 or more but the
     * PHY mode is non AUTO and is lower than IEEE80211_MODE_11BEA_EHT20.
     */
    if ((ml_info->num_mlo_links > 0) && (mode > IEEE80211_MODE_AUTO) &&
        (mode < IEEE80211_MODE_11BEA_EHT20)) {
         mlo_err("psoc id %u, pdev id %u, vdev id %u: Invalid PHY mode %d for MLO. Num MLO links requested by userspace/hostapd=%u",
                 wlan_vdev_get_psoc_id(vdev),
                 wlan_objmgr_pdev_get_pdev_id(pdev), wlan_vdev_get_id(vdev),
                 mode, ml_info->num_mlo_links);
         return QDF_STATUS_E_INVAL;
    }

    mlo_debug("psoc id %u, pdev id %u, vdev id %u: Valid PHY mode %d for num MLO links requested by userspace/hostapd=%u",
              wlan_vdev_get_psoc_id(vdev), wlan_objmgr_pdev_get_pdev_id(pdev),
              wlan_vdev_get_id(vdev), mode, ml_info->num_mlo_links);

    return QDF_STATUS_SUCCESS;
}

bool wlan_mlme_is_non_mlo_11be_ap(wlan_if_t vap)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct qdf_mac_addr *mld_addr = NULL;
    enum ieee80211_phymode mode;

    if (!vap) {
        mlo_err("vap is NULL");
        return true;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        mlo_err("vdev is NULL");
        return true;
    }

    /* Acquire lock as required by wlan_vdev_mlme_get_mldaddr() */
    wlan_vdev_obj_lock(vdev);
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
    wlan_vdev_obj_unlock(vdev);

    mode = wlan_get_desired_phymode(vap);

    return (qdf_is_macaddr_zero(mld_addr) || !vap->mlo_info.num_mlo_links) &&
           ieee80211_is_phymode_11be(mode) && wlan_vdev_mlme_is_ap(vdev) &&
           !IEEE80211_IS_SPECIAL_VAP(vap);
}

bool wlan_mlme_is_non_mlo_11be_ap_operation_allowed(wlan_if_t vap)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;

    if (!vap) {
        mlo_err("vap is NULL");
        return false;
    }

    vdev = vap->vdev_obj;
    if (!vdev) {
        mlo_err("vdev is NULL");
        return false;
    }

    psoc = wlan_vdev_get_psoc(vdev);
    if (!psoc) {
        mlo_err("psoc is NULL");
        return false;
    }

    return cfg_get(psoc, CFG_MLME_NON_MLO_11BE_AP_OPERATION_ENABLE);
}

QDF_STATUS
mlme_mlo_link_add_get_current_batch(wlan_if_t vap,
                                    uint16_t *batch_size,
                                    struct wlan_objmgr_vdev **batch_vdev_list)
{
    struct wlan_mlo_dev_context *mlo_dev_ctx;
    struct wlan_objmgr_vdev *tmp_vdev;
    struct qdf_mac_addr tmp_mlo_link_mac;
    int i;
    struct wlan_mlme_mlo_info *ml_info;

    *batch_size = 0;

    if (!vap) {
        mlo_err("vap is NULL");
        return QDF_STATUS_E_NULL_VALUE;
    }

    mlo_dev_ctx = vap->vdev_obj->mlo_dev_ctx;
    ml_info = &vap->mlo_info;

    if (mlo_dev_ctx->wlan_vdev_count < ml_info->num_mlo_links) {
        mlo_err("Num links indicated by user space %d vs. Current links in MLD %d",
                ml_info->num_mlo_links, mlo_dev_ctx->wlan_vdev_count);
        return QDF_STATUS_E_INVAL;
    }

    for (i = 0; i < ml_info->num_mlo_links; i++) {
        IEEE80211_ADDR_COPY(&tmp_mlo_link_mac, ml_info->mlo_mac_addrs[i].bytes);

        /* VDEV should have been added to MLD as part of VDEV create sequence */
        tmp_vdev = mlo_get_ml_vdev_by_mac(vap->vdev_obj, &tmp_mlo_link_mac);
        if (!tmp_vdev) {
            mlo_err("ml_mac %s from user space is not present in mlo_dev_ctx",
                    ether_sprintf(ml_info->mlo_mac_addrs[i].bytes));
            return QDF_STATUS_E_INVAL;
        }

         if (wlan_vdev_mlme_is_mlo_vdev(tmp_vdev)) {
             if (QDF_IS_STATUS_ERROR(wlan_vdev_chan_config_valid(tmp_vdev))) {
                mlo_err("An existing partner VAP(Link MAC: %s) received in"
                        "the link add request is not active",
                        ether_sprintf(ml_info->mlo_mac_addrs[i].bytes));
                return QDF_STATUS_E_INVAL;
             }
         } else {
             /**
              * If this link is not yet marked as ML vdev,
              * then it is getting added as part of this link addition batch
              */
             batch_vdev_list[*batch_size] = tmp_vdev;
             (*batch_size) += 1;
         }
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
mlme_mlo_link_add_get_current_batch_size(wlan_if_t vap, uint16_t *batch_size)
{
    struct wlan_objmgr_vdev *batch_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};

    return mlme_mlo_link_add_get_current_batch(vap,
                                               batch_size,
                                               batch_vdev_list);
}
#endif /* WLAN_FEATURE_11BE_MLO */

/**
 * wlan_mlme_start_ap_vdev() - API to dispatch EV_START to VDEV SM
 * @cmd: Serialization command structure
 *
 * This function will be called from serialzation module for non-STA opmode.
 * Will be executed in caller context if activated directly from active queue.
 * Else it will be in scheduler context (activated as part of movement from
 * pending to active queue)
 *
 * Return: QDF_STATUS
 */
static QDF_STATUS
wlan_mlme_start_ap_vdev(struct wlan_serialization_command *cmd)
{
    wlan_dev_t comhandle;
    struct ieee80211_ath_channel chan_obj;
    wlan_chan_t chan = &chan_obj;
    int error;
    struct wlan_objmgr_vdev *vdev;
    wlan_if_t vap;
    osif_dev *osifp;
    struct wlan_mlme_ser_data *data;
    int forcescan;
    enum ieee80211_phymode des_mode;
    ieee80211_acs_t acs;
    struct wlan_mbss_ev_data mbss_ev = {0};
#ifdef CONFIG_AFC_SUPPORT
    enum reg_6g_ap_type ap_pwr_type;
#endif /* CONFIG_AFC_SUPPORT */

    if (!cmd) {
        wlan_mlme_err("cmd is null, investigate");
        return QDF_STATUS_E_FAILURE;
    }

    if (!cmd->umac_cmd) {
        wlan_mlme_err("umac_cmd is null, investigate");
        return QDF_STATUS_E_FAILURE;
    }

    vdev = cmd->vdev;
    data = cmd->umac_cmd;
    forcescan = data->flags;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        wlan_mlme_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if (!vap->iv_ic) {
        wlan_mlme_err("IC is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    osifp = (osif_dev *)vap->iv_ifp;
    des_mode = wlan_get_desired_phymode(vap);

    if (vap->iv_ic->recovery_in_progress) {
        wlan_mlme_err("recovery_in_progress on vap %d", vap->iv_unit);
        return QDF_STATUS_E_FAILURE;
    }

    if ((wlan_vdev_mlme_get_opmode(vdev) == QDF_MONITOR_MODE) &&
        (!wlan_autoselect_in_progress(vap))) {
        wlan_mlme_start_monitor(vap);
        return QDF_STATUS_SUCCESS;
    }

    comhandle = wlan_vap_get_devhandle(vap);
    if(comhandle->ic_sta_vap && wlan_is_connected(comhandle->ic_sta_vap)) {
        *chan = wlan_get_current_channel(vap, true);
    } else {
        *chan = wlan_get_current_channel(vap, false);
    }

#if WLAN_SUPPORT_PRIMARY_ALLOWED_CHAN
    if(IEEE80211_IS_CHAN_VALID(chan) &&
            !ieee80211_ic_externalacs_enable_is_set(vap->iv_ic) &&
            vap->iv_ic->ic_primary_allowed_enable &&
            !(ieee80211_check_allowed_prim_freqlist(
                    vap->iv_ic,chan->ic_freq))) {
        wlan_mlme_err("channel %d freq %d is not a primary allowed channel",
                      chan->ic_ieee, chan->ic_freq);
        *chan = empty_channel;
    }
#endif

#ifdef CONFIG_AFC_SUPPORT
    /*
     * AFC bring-up behavior:
     * - For auto channel selected channel, if in LPI and the radio is SP
     *   capable then re-run the ACS algorithm.
     * - For fixed channel, if in LPI radio is SP capable, then
     *   change the channel power mode to SP. Otherwise, remain in LPI
     *
     * Attempt move to SP only if there are no STA modes. If a STA VAP comes up
     * at a later point, power mode switches should be done through the
     * STA connect/disconnect path due to LPI dependency for scanning.
     */
    wlan_reg_get_cur_6g_ap_pwr_type(vap->iv_ic->ic_pdev_obj, &ap_pwr_type);
    if (!vap->iv_ic->ic_sta_vap &&
        IEEE80211_IS_CHAN_VALID(chan) &&
        wlan_reg_is_6ghz_chan_freq(chan->ic_freq) &&
        ieee80211_is_afc_capable(vap->iv_ic, ieee80211_chan2mode(chan)) &&
        (ap_pwr_type != REG_STANDARD_POWER_AP)) {
        /* Setting AP power type for given channel */
        ieee80211_change_ap_power_type(vap, chan);
    }
#endif /* CONFIG_AFC_SUPPORT */

    if (!IEEE80211_IS_CHAN_VALID(chan)) {
        /* Prevent ACS for special vap, user is supposed to fix channel */
        if (IEEE80211_IS_SPECIAL_VAP_ONLY(vap)) {
            wlan_mlme_err("ACS is not supported with special vap");
            return QDF_STATUS_E_FAILURE;
        }

        if (ieee80211_ic_externalacs_enable_is_set(vap->iv_ic)) {
            /*
             * If any VAP is active, the channel is already selected
             * so go ahead and init the VAP, the same channel will be used
             */
            if (ieee80211_vaps_active(vap->iv_ic)) {
                /* Set the curent channel to the VAP */
                struct ieee80211_ath_channel sel_chan_obj;
                struct chan_params chan_p = {0};
                wlan_chan_t sel_chan = &sel_chan_obj;

                *sel_chan = wlan_get_current_channel(vap, true);
                chan_p.freq = sel_chan->ic_freq;
                chan_p.cfreq2 = sel_chan->ic_vhtop_freq_seg2;
                set_chan_params_puncture_bitmap(&chan_p, sel_chan->op_puncture_bitmap);
                error = wlan_set_channel(vap, &chan_p);
                wlan_mlme_info("wlan_set_channel error code: %d", error);
                if (error !=0) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                            "%s : failed to set channel with error code %d\n",
                            __func__, error);
                    return QDF_STATUS_E_FAILURE;
                }
            } else {
                /*
                 * No VAPs active. An external entity is responsible for
                 * auto channel selection at VAP init time
                 */
                wlan_mlme_info("No active vaps");
                return QDF_STATUS_E_FAILURE;
            }
        } else {
            /* start ACS module to get channel */
            mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_ACS;
            mbss_ev.ext_cb = &osif_acs_event_handler;
            mbss_ev.source.acs_source = MBSS_ACS_SRC_MLME;
            mbss_ev.arg = NULL;
            wlan_mlme_debug("started wlan_autoselect_find_infra_bss_channel");
            return wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
        }
    }

    /*
     * 11AX: Recheck future 802.11ax drafts (>D1.0) on coex rules
     */
    if (ieee80211_is_phymode_g40(des_mode) &&
            (wlan_coext_enabled(vap) &&
            !vap->iv_ic->ic_obss_done_freq &&
            (osif_get_num_running_vaps(comhandle) == 0))) {

        mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_HT40;
        mbss_ev.ext_cb = &osif_ht40_event_handler;
        mbss_ev.source.ht40_source = MBSS_HT40_SRC_MLME;
        mbss_ev.arg = NULL;
        wlan_mlme_debug("started wlan_attempt_ht40_bss");
        return wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
    }

    if (forcescan)
        vap->iv_rescan = 1;

    acs = vap->iv_ic->ic_acs;
    IEEE80211_VAP_LOCK(vap);
    if ((wlan_mbss_vdev_acs_in_progress(vap->vdev_obj, MBSS_ACS_SRC_MAX) ||
         wlan_mbss_vdev_ht40_in_progress(vap->vdev_obj, MBSS_HT40_SRC_MAX)) &&
         !acs->acs_bkscantimer_en) {
        /* do not start AP if there is already a pending ACS */
        IEEE80211_VAP_UNLOCK(vap);
        wlan_mlme_debug("ACS Scan in progress for vdev:%d",
                      wlan_vdev_get_id(vap->vdev_obj));
        return QDF_STATUS_E_FAILURE;
    }
    IEEE80211_VAP_UNLOCK(vap);

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_mlme_is_non_mlo_11be_ap(vap)) {
        if (!wlan_mlme_is_non_mlo_11be_ap_operation_allowed(vap) &&
            !IEEE80211_IS_MLO_BRIDGE_VAP(vap)) {
            mlo_err("Non MLO 11BE AP operation is not allowed as per 802.11 standard");
            return QDF_STATUS_E_INVAL;
        }

        mlo_warn("Allowing 11be non-MLO operation as per INI configuration");
    }

    if (wlan_mlme_mlo_validate_ap_phymode(vap) != QDF_STATUS_SUCCESS) {
        if (vdev && vap->iv_ic->ic_pdev_obj) {
            mlo_err("psoc id %u, pdev id %u, vap id %u: Failed validation of AP PHY mode for MLO",
                    wlan_vdev_get_psoc_id(vdev),
                    wlan_objmgr_pdev_get_pdev_id(vap->iv_ic->ic_pdev_obj),
                    wlan_vdev_get_id(vdev));
        } else {
            mlo_err("Failed validation of AP PHY mode for MLO (Also unable to get psoc/pdev/vdev id due to NULL pointers in object linkage)");
        }

        return QDF_STATUS_E_FAILURE;
    }

    if (wlan_mlme_check_mbss_mixed_modes_config(vap) != QDF_STATUS_SUCCESS) {
        wlan_mlme_err("PHY mode mismatch for MBSSID vap (%s)", vap->iv_netdev_name);
        return QDF_STATUS_E_FAILURE;
    }

    if((wlan_mlme_mlo_update_partner_info(vap) != QDF_STATUS_SUCCESS)) {
        mlo_err("Failed wlan_mlme_mlo_update_partner_infop");
        return QDF_STATUS_E_FAILURE;
    }
#endif

    error = wlan_mlme_start_bss(vap, forcescan);
    if (error) {
        /* Radio resource is busy on scanning, try later */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL,
                "%s :mlme busy mostly scanning \n", __func__);
        return QDF_STATUS_E_FAILURE;
    } else {
        wlan_mlme_connection_up(vap);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_IOCTL, "%s :vap up \n", __func__);
        if (wlan_coext_enabled(vap))
            wlan_determine_cw(vap, chan);
    }

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
wlan_mlme_stop_ap_vdev(struct wlan_serialization_command *cmd)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    wlan_if_t vap;
    osif_dev *osifp;
    struct net_device *dev;
    struct ieee80211com *ic;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;
    enum QDF_OPMODE opmode = wlan_vdev_mlme_get_opmode(vdev);
    int flags = 0;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        wlan_mlme_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    osifp = (osif_dev *)vap->iv_ifp;
    dev = osifp->netdev;
    ic = vap->iv_ic;

    if (opmode == QDF_MONITOR_MODE) {
        flags =  WLAN_MLME_STOP_BSS_F_NO_RESET |
                 WLAN_MLME_STOP_BSS_F_FORCE_STOP_RESET;
        wlan_mlme_stop_bss(vap, flags);
        return QDF_STATUS_SUCCESS;
    }
    /* Cancel CBS */
    wlan_bk_scan_stop_async(ic);
    if (wlan_vdev_scan_in_progress(vap->vdev_obj)) {
        struct wlan_mbss_ev_data mbss_ev = {0};
        wlan_mlme_debug("Scan in progress.. Cancelling it. vap: 0x%pK", vap);
        wlan_ucfg_scan_cancel(vap, osifp->scan_requestor, 0,
                WLAN_SCAN_CANCEL_VDEV_ALL, false);

        mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_ACS;
        wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

        mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_HT40;
        wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

#if UMAC_SUPPORT_CFG80211
        if (ic->ic_cfg80211_config)
            wlan_cfg80211_cleanup_scan_queue(ic->ic_pdev_obj, dev);
#endif
        /* we know a scan was interrupted because we're stopping
         * the VAP. This may lead to an invalid channel pointer.
         * Thus, initialise it with the default channel information
         * from the ic to prevent a crash in
         * ieee80211_init_node_rates() during bss reset.
         */
        if (!IEEE80211_IS_CHAN_VALID(vap->iv_bsschan)) {
            wlan_mlme_err("Info: overwriting invalid BSS channel"
                    "info by defaults (%pK)", vap->iv_ic->ic_curchan);
            *vap->iv_bsschan = *(vap->iv_ic->ic_curchan);
        }
    }

    flags = WLAN_MLME_STOP_BSS_F_NO_RESET;
    if (data)
        flags |= data->flags;

    wlan_mlo_ap_vdev_quiet_set(vdev);
    wlan_mlme_stop_bss(vap, flags);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_add_stop_start_to_pq_head(struct wlan_objmgr_vdev *vdev)
{
    wlan_mlme_debug("Adding STOP-START Sequence for vdev:%u",
                    wlan_vdev_get_id(vdev));
    mlme_vdev_add_cmd_to_pq(vdev, WLAN_SER_CMD_VDEV_START_BSS, true);
    mlme_vdev_add_cmd_to_pq(vdev, WLAN_SER_CMD_VDEV_STOP_BSS, true);

    return QDF_STATUS_SUCCESS;
}

void wlan_mlme_get_num_valid_vdevs(struct wlan_objmgr_pdev *pdev,
                                   void *object, void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    uint8_t *num_vdevs = (uint8_t *)arg;

    /*
     * Get number of VDEVs not in INIT
     */
    if (QDF_IS_STATUS_ERROR(wlan_vdev_mlme_is_init_state(vdev)))
        ++(*num_vdevs);
}

void wlan_mlme_check_if_mvr_sup(struct wlan_objmgr_pdev *pdev,
                                void *object, void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    bool *restart = (bool *)arg;

    /*
     * Get number of VDEVs not in INIT or UP state
     */
    if (QDF_IS_STATUS_ERROR(wlan_vdev_mlme_is_init_state(vdev)) &&
            QDF_IS_STATUS_ERROR(wlan_vdev_is_up_active_state(vdev))) {
        *restart = false;
    }
}

void wlan_mlme_restart_bss_iter_cb(struct wlan_objmgr_pdev *pdev,
                                   void *object, void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    bool *restart = (bool*)arg;

    if (wlan_vdev_mlme_get_state(vdev) == WLAN_VDEV_S_INIT)
        return;

    if (*restart) {
        wlan_mlme_restart_bss(vdev);
    } else {
        mlme_vdev_add_stop_start_to_pq_head(vdev);
    }
}

static QDF_STATUS
mlme_activate_vdev_start(struct wlan_serialization_command *cmd)
{
    enum QDF_OPMODE opmode;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_mlme_ser_data *data;

    if (!cmd || !cmd->vdev || !cmd->umac_cmd) {
        wlan_mlme_err("Null input cmd:%pK", cmd);
        return QDF_STATUS_E_INVAL;
    }

    vdev = cmd->vdev;
    data = cmd->umac_cmd;
    opmode = wlan_vdev_mlme_get_opmode(vdev);

    data->cmd_in_sched = 0;
    if (opmode == QDF_SAP_MODE || opmode == QDF_MONITOR_MODE)
        return wlan_mlme_start_ap_vdev(cmd);

    return QDF_STATUS_E_INVAL;
}

static QDF_STATUS
mlme_activate_vdev_stop(struct wlan_serialization_command *cmd)
{
    enum QDF_OPMODE opmode;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_mlme_ser_data *data;

    if (!cmd || !cmd->vdev || !cmd->umac_cmd) {
        wlan_mlme_err("Null input cmd:%pK", cmd);
        return QDF_STATUS_E_INVAL;
    }

    vdev = cmd->vdev;
    data = cmd->umac_cmd;
    opmode = wlan_vdev_mlme_get_opmode(vdev);

    data->cmd_in_sched = 0;

    if (opmode == QDF_SAP_MODE || opmode == QDF_MONITOR_MODE)
        return wlan_mlme_stop_ap_vdev(cmd);

    return QDF_STATUS_E_INVAL;
}

static QDF_STATUS
mlme_activate_pdev_restart(struct wlan_serialization_command *cmd)
{
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_mlme_ser_data *data;
    struct pdev_mlme_obj *pdev_mlme;
    bool do_restart = true;
    uint8_t num_vdevs = 0;

    if (!cmd || !cmd->vdev || !cmd->umac_cmd) {
        wlan_mlme_err("Null input cmd:%pK", cmd);
        return QDF_STATUS_E_INVAL;
    }

    vdev = cmd->vdev;
    data = cmd->umac_cmd;
    data->cmd_in_sched = 0;

    pdev = wlan_vdev_get_pdev(vdev);
    pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
    if (!pdev_mlme) {
        wlan_mlme_err("Null input");
        return QDF_STATUS_E_INVAL;
    }

    qdf_mem_zero(&pdev_mlme->pdev_restart.restart_bmap,
                sizeof(pdev_mlme->pdev_restart.restart_bmap));

    pdev_mlme->pdev_restart.vdev = vdev;

    /*
     * If all the AP VDEVs are in INIT state then remove PDEV_RESTART
     * cmd from serialization and return
     */
    wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
                                      wlan_mlme_get_num_valid_vdevs,
                                      &num_vdevs, 0,
                                      WLAN_MLME_SER_IF_ID);

    if (!num_vdevs) {
        wlan_mlme_debug("Pdev:%d No valid vdevs found",
                        wlan_objmgr_pdev_get_pdev_id(pdev));
        wlan_mlme_release_vdev_req(vdev, WLAN_SER_CMD_PDEV_RESTART, EINVAL);
        return QDF_STATUS_SUCCESS;
    }

    /*
     * Multivdev restart only if all the VDEVs are in UP state, else
     * individual stop-start is dispatched
     */
    wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
                                      wlan_mlme_check_if_mvr_sup,
                                      &do_restart, 0,
                                      WLAN_MLME_SER_IF_ID);

    if (do_restart) {
        wlan_mlme_err("Starting MVR Vdev %d Pdev %d Psoc %d",
                      wlan_vdev_get_id(vdev),
                      wlan_objmgr_pdev_get_pdev_id(pdev),
                      wlan_psoc_get_id(wlan_pdev_get_psoc(pdev)));
        wlan_pdev_mlme_op_set(pdev, WLAN_PDEV_OP_MBSSID_RESTART);
    }

    wlan_mlme_debug("MVR Restart:%u", do_restart);
    wlan_objmgr_pdev_iterate_obj_list(pdev, WLAN_VDEV_OP,
                                      wlan_mlme_restart_bss_iter_cb,
                                      &do_restart, 0, WLAN_MLME_SER_IF_ID);

    /*
     * Remove the pdev restart request from serialization queue if
     * stop-start sequence is dispatched
     */
    if (!do_restart)
        wlan_mlme_release_vdev_req(vdev,
                                   WLAN_SER_CMD_PDEV_RESTART, EINVAL);

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlme_activate_vdev_restart(struct wlan_serialization_command *cmd)
{
    struct wlan_objmgr_vdev *vdev;
    struct wlan_mlme_ser_data *data;

    if (!cmd || !cmd->vdev || !cmd->umac_cmd) {
        wlan_mlme_err("Null input cmd:%pK", cmd);
        return QDF_STATUS_E_INVAL;
    }

    vdev = cmd->vdev;
    data = cmd->umac_cmd;

    data->cmd_in_sched = 0;

    if (wlan_vdev_mlme_get_state(vdev) == WLAN_VDEV_S_INIT)
        return QDF_STATUS_E_FAILURE;

    if (QDF_IS_STATUS_SUCCESS(wlan_vdev_is_up_active_state(vdev))) {
        return wlan_mlme_restart_bss(vdev);
    } else {
        /*
         * On restart cmd activation, if the VDEV SM is not in UP state, then
         * follow the default stop-start mechanism to restart the vdev.
         * and remove the existing restart cmd from active queue
         */
        mlme_vdev_add_stop_start_to_pq_head(vdev);
    }

    return QDF_STATUS_E_FAILURE;
}

static QDF_STATUS
mlme_activate_pdev_csa_restart(struct wlan_serialization_command *cmd)
{
    struct ieee80211com *ic;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_mlme_ser_data *data;
    struct pdev_mlme_obj *pdev_mlme;

    if (!cmd || !cmd->vdev || !cmd->umac_cmd) {
        wlan_mlme_err("Null input cmd:%pK", cmd);
        return QDF_STATUS_E_INVAL;
    }

    vdev = cmd->vdev;
    data = cmd->umac_cmd;
    data->cmd_in_sched = 0;

    pdev = wlan_vdev_get_pdev(vdev);
    pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!pdev_mlme || !ic) {
        wlan_mlme_err("Null input");
        return QDF_STATUS_E_INVAL;
    }

    if (IEEE80211_CHANCHANGE_STARTED_IS_SET(ic)) {
        wlan_mlme_err("Chan change already in progress return "PSOC_VDEV_ID,
                      wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
        return QDF_STATUS_E_BUSY;
    }

    pdev_mlme->pdev_restart.vdev = vdev;

    qdf_mem_zero(&pdev_mlme->pdev_restart.restart_bmap,
                sizeof(pdev_mlme->pdev_restart.restart_bmap));

    qdf_info("Dispatch CSA event to all vdevs from serialization");
    wlan_pdev_mlme_vdev_sm_csa_restart(pdev, ic->ic_chanchange_channel);

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlme_vdev_req_flush_cb(struct scheduler_msg *msg)
{
    struct wlan_serialization_command *cmd = msg->bodyptr;

    if (!cmd || !cmd->vdev) {
        wlan_mlme_err("Null input cmd:%pK", cmd);
        return QDF_STATUS_E_INVAL;
    }

    wlan_objmgr_vdev_release_ref(cmd->vdev, WLAN_SCHEDULER_ID);
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS mlme_vdev_req_sched_cb(struct scheduler_msg *msg)
{
    struct wlan_serialization_command *cmd = msg->bodyptr;
    struct wlan_objmgr_vdev *vdev;
    QDF_STATUS ret = QDF_STATUS_E_FAILURE;

    if (!cmd) {
        wlan_mlme_err("cmd is null");
        return QDF_STATUS_E_INVAL;
    }

    vdev = cmd->vdev;
    if (!vdev) {
        wlan_mlme_err("vdev is null");
        return QDF_STATUS_E_INVAL;
    }

    switch (cmd->cmd_type) {
    case WLAN_SER_CMD_VDEV_START_BSS:
        wlan_mlme_nofl_debug("START_SCHED V:%u", wlan_vdev_get_id(vdev));
        ret = mlme_activate_vdev_start(cmd);
        break;
    case WLAN_SER_CMD_VDEV_STOP_BSS:
        wlan_mlme_nofl_debug("STOP_SCHED V:%u", wlan_vdev_get_id(vdev));
        ret = mlme_activate_vdev_stop(cmd);
        break;
    case WLAN_SER_CMD_PDEV_RESTART:
        wlan_mlme_nofl_debug("PDEV_RESTART_SCH V:%u", wlan_vdev_get_id(vdev));
        ret = mlme_activate_pdev_restart(cmd);
        break;
    case WLAN_SER_CMD_VDEV_RESTART:
        wlan_mlme_nofl_debug("VDEV_RESTART_SCH V:%u", wlan_vdev_get_id(vdev));
        ret = mlme_activate_vdev_restart(cmd);
        break;
    case WLAN_SER_CMD_PDEV_CSA_RESTART:
        wlan_mlme_nofl_debug("VDEV_CSA_RESTART_SCH V:%u", wlan_vdev_get_id(vdev));
        ret = mlme_activate_pdev_csa_restart(cmd);
        break;
    default:
        break;
    }
    /*
     * Called from scheduler context hence release the command on error
     */
    if (QDF_IS_STATUS_ERROR(ret)) {
        wlan_mlme_nofl_debug("Activation failed for cmd:%d", cmd->cmd_type);
        wlan_mlme_release_vdev_req(vdev, cmd->cmd_type, EINVAL);
    }

    wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
    return ret;
}

static QDF_STATUS
mlme_activate_cmd_in_sched_ctx(struct wlan_serialization_command *cmd)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;
    struct scheduler_msg msg = {0};
    QDF_STATUS ret;

    msg.bodyptr = cmd;
    msg.callback = mlme_vdev_req_sched_cb;
    msg.flush_callback = mlme_vdev_req_flush_cb;

    ret = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SCHEDULER_ID);
    if (QDF_IS_STATUS_ERROR(ret))
        return ret;

    data->cmd_in_sched = 1;
    data->activation_ctx = MLME_CTX_SCHED;
    ret = scheduler_post_message(QDF_MODULE_ID_MLME,
            QDF_MODULE_ID_MLME, QDF_MODULE_ID_MLME, &msg);
    if (QDF_IS_STATUS_ERROR(ret)) {
        wlan_mlme_err("failed to post scheduler_msg");
        wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
        return ret;
    }
    wlan_mlme_debug("Cmd act in sched vdev:%d cmd type:%d",
            wlan_vdev_get_id(vdev), cmd->cmd_type);

    return ret;
}

/*
 * mlme_reset_cmd_info() - Reset any flags/data associated with the cmd type
 * @cmd: Serialization command
 *
 * In timeout scenario, reset any flags/data associated with the timed out
 * command.
 *
 * Return: void
 */
void mlme_reset_cmd_info(struct wlan_serialization_command *cmd)
{
    struct wlan_objmgr_vdev *vdev;
    struct wlan_objmgr_pdev *pdev;
    struct pdev_mlme_obj *pdev_mlme;

    if (!cmd) {
        wlan_mlme_err("Ser command is null, investigate!");
        return;
    }

    if (!cmd->vdev) {
        wlan_mlme_err("vdev is null, investigate!");
        return;
    }

    vdev = cmd->vdev;
    pdev = wlan_vdev_get_pdev(vdev);
    pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
    if (!pdev_mlme) {
        wlan_mlme_err("Pdev mlme info is null cmd_type:%u vdev_id:%u",
                      cmd->cmd_type, wlan_vdev_get_id(vdev));
        return;
    }

    switch(cmd->cmd_type) {
    case WLAN_SER_CMD_PDEV_RESTART:
        /* fall through */
    case WLAN_SER_CMD_PDEV_CSA_RESTART:
        if (pdev_mlme->pdev_restart.vdev) {
            wlan_mlme_err("Clearing restart vdev cmd_type:%u restart_vdev:%u",
                          cmd->cmd_type,
                          wlan_vdev_get_id(pdev_mlme->pdev_restart.vdev));
            pdev_mlme->pdev_restart.vdev = NULL;
        }
        break;
    default:
        break;
    }
}

/*
 * mlme_proc_vdev_cmd_error_handler() - Default case cmd activation handler
 * @cmd: Serialization command
 * @reason: Cmd processing reason from serialization module
 *
 * Vdev cmd activation error handels the default cases such as
 * cancel, timeout and release
 *
 * Return: Success on handling the error.
 */
QDF_STATUS
mlme_proc_vdev_cmd_error_handler(struct wlan_serialization_command *cmd,
                                 enum wlan_serialization_cb_reason reason)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_objmgr_psoc *psoc = wlan_vdev_get_psoc(vdev);
    uint8_t psoc_id = wlan_psoc_get_id(psoc);

    switch (reason) {
    case WLAN_SER_CB_CANCEL_CMD:
        break;
    case WLAN_SER_CB_RELEASE_MEM_CMD:
        wlan_mlme_if_mgr_ev_complete_send(vdev, cmd->cmd_type);
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);
        qdf_mem_free(cmd->umac_cmd);
        cmd->umac_cmd = NULL;
        break;
    case WLAN_SER_CB_ACTIVE_CMD_TIMEOUT:
        wlan_mlme_if_mgr_ev_complete_send(vdev, cmd->cmd_type);
        wlan_mlme_err("Timeout for vdev:%d cmdtype:%d",
                      wlan_vdev_get_id(vdev), cmd->cmd_type);
        wlan_mlme_print_vdev_sm_history(psoc, vdev, &psoc_id);
        wlan_ser_print_history(vdev, WLAN_SER_CMD_NONSCAN, WLAN_SER_CMD_NONSCAN);
        mlme_reset_cmd_info(cmd);
        break;
    default:
        QDF_ASSERT(0);
        return QDF_STATUS_E_INVAL;
        break;
    }
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
mlme_ser_proc_vdev_start(struct wlan_serialization_command *cmd,
                         enum wlan_serialization_cb_reason reason)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;
    QDF_STATUS status;

    wlan_mlme_nofl_debug("START "PSOC_VDEV_ID"rs:%d act:%d",
                         wlan_vdev_get_psoc_id(vdev),
                         wlan_vdev_get_id(vdev),
                         reason, cmd->activation_reason);

    switch (reason) {
    case WLAN_SER_CB_ACTIVATE_CMD:
        wlan_mlme_if_mgr_ev_send(vdev, cmd->cmd_type);
        if (cmd->activation_reason == SER_PENDING_TO_ACTIVE) {
            status = mlme_activate_cmd_in_sched_ctx(cmd);
        } else {
            data->activation_ctx = MLME_CTX_DIRECT;
            status = mlme_activate_vdev_start(cmd);
        }
        break;
    default:
        status = mlme_proc_vdev_cmd_error_handler(cmd, reason);
        break;
    }
    return status;
}

QDF_STATUS
mlme_ser_proc_vdev_stop(struct wlan_serialization_command *cmd,
                        enum wlan_serialization_cb_reason reason)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;
    QDF_STATUS status;

    wlan_mlme_nofl_debug("STOP "PSOC_VDEV_ID"rs:%d act:%d",
                         wlan_vdev_get_psoc_id(vdev),
                         wlan_vdev_get_id(vdev),
                         reason, cmd->activation_reason);

    switch (reason) {
    case WLAN_SER_CB_ACTIVATE_CMD:
        wlan_mlme_if_mgr_ev_send(vdev, cmd->cmd_type);
        if (cmd->activation_reason == SER_PENDING_TO_ACTIVE) {
            status = mlme_activate_cmd_in_sched_ctx(cmd);
        } else {
            data->activation_ctx = MLME_CTX_DIRECT;
            status = mlme_activate_vdev_stop(cmd);
        }
        break;
    default:
        status = mlme_proc_vdev_cmd_error_handler(cmd, reason);
        break;
    }
    return status;
}

QDF_STATUS
mlme_ser_proc_pdev_restart(struct wlan_serialization_command *cmd,
                           enum wlan_serialization_cb_reason reason)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;

    wlan_mlme_nofl_debug("PDEV restart V:%u rs:%d act:%d",
                         wlan_vdev_get_id(vdev),
                         reason, cmd->activation_reason);

    switch (reason) {
    case WLAN_SER_CB_ACTIVATE_CMD:
        if (cmd->activation_reason == SER_PENDING_TO_ACTIVE) {
            return mlme_activate_cmd_in_sched_ctx(cmd);
        } else {
            data->activation_ctx = MLME_CTX_DIRECT;
            return mlme_activate_pdev_restart(cmd);
        }
        break;
    default:
        return mlme_proc_vdev_cmd_error_handler(cmd, reason);
        break;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
mlme_ser_proc_vdev_restart(struct wlan_serialization_command *cmd,
                           enum wlan_serialization_cb_reason reason)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;

    wlan_mlme_nofl_debug("VDEV restart V:%u rs:%d act:%d",
                         wlan_vdev_get_id(vdev),
                         reason, cmd->activation_reason);

    switch (reason) {
    case WLAN_SER_CB_ACTIVATE_CMD:
        if (cmd->activation_reason == SER_PENDING_TO_ACTIVE) {
            return mlme_activate_cmd_in_sched_ctx(cmd);
        } else {
            data->activation_ctx = MLME_CTX_DIRECT;
            return mlme_activate_vdev_restart(cmd);
        }
        break;
    default:
        return mlme_proc_vdev_cmd_error_handler(cmd, reason);
        break;
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS
mlme_ser_proc_pdev_csa_restart(struct wlan_serialization_command *cmd,
                         enum wlan_serialization_cb_reason reason)
{
    struct wlan_objmgr_vdev *vdev = cmd->vdev;
    struct wlan_mlme_ser_data *data = cmd->umac_cmd;

    wlan_mlme_nofl_debug("CSA_RESTART V:%u rs:%d act:%d",
                         wlan_vdev_get_id(vdev),
                         reason, cmd->activation_reason);

    switch (reason) {
    case WLAN_SER_CB_ACTIVATE_CMD:
        if (cmd->activation_reason == SER_PENDING_TO_ACTIVE) {
            return mlme_activate_cmd_in_sched_ctx(cmd);
        } else {
            data->activation_ctx = MLME_CTX_DIRECT;
            return mlme_activate_pdev_csa_restart(cmd);
        }
        break;
    default:
        return mlme_proc_vdev_cmd_error_handler(cmd, reason);
        break;
    }

    return QDF_STATUS_SUCCESS;
}

void wlan_mlme_vdev_cmd_ap_handler(struct wlan_objmgr_vdev *vdev,
                                   enum wlan_serialization_cmd_type cmd_type)
{
    wlan_if_t vap = wlan_vdev_get_mlme_ext_obj(vdev);

    if (!vap) {
        wlan_mlme_err("vap is NULL");
        return;
    }

    switch (cmd_type) {
    case WLAN_SER_CMD_VDEV_START_BSS:
    case WLAN_SER_CMD_VDEV_RESTART:
        wlan_mlme_debug("START complete notification");
        wlan_restore_vap_params(vap);
        break;
    default:
        break;
    }
}

void wlan_mlme_vdev_cmd_sta_handler(struct wlan_objmgr_vdev *vdev,
                                    enum wlan_serialization_cmd_type cmd_type)
{
    /*No-op for now*/
}

void wlan_mlme_vdev_cmd_handler(struct wlan_objmgr_vdev *vdev,
                                enum wlan_serialization_cmd_type cmd_type)
{
    enum QDF_OPMODE opmode = wlan_vdev_mlme_get_opmode(vdev);

    if (opmode == QDF_SAP_MODE)
        wlan_mlme_vdev_cmd_ap_handler(vdev, cmd_type);
    else
        wlan_mlme_vdev_cmd_sta_handler(vdev, cmd_type);
}

QDF_STATUS mlme_check_and_rel_pdev_restart(struct wlan_objmgr_vdev *vdev,
                                           struct pdev_mlme_obj *pdev_mlme)
{
    struct wlan_objmgr_vdev *restart_vdev;
    enum wlan_serialization_cmd_type active_cmd_type;
    /*
     * If pdev restart is in progress, then don't remove the active pdev
     * restart cmd until all the vdevs are up. Reset the bitmap of the
     * corresponding vdev when the vdev sm notifies it is in UP state.
     */
    wlan_util_change_map_index(pdev_mlme->pdev_restart.restart_bmap,
                               wlan_vdev_get_id(vdev), 0);
    if (wlan_util_map_is_any_index_set(pdev_mlme->pdev_restart.restart_bmap,
                sizeof(pdev_mlme->pdev_restart.restart_bmap))) {
        wlan_mlme_debug("Vdev:%u wait for others vdev to move to UP",
                        wlan_vdev_get_id(vdev));
        return QDF_STATUS_E_PENDING;
    }

    /*
     * Remove pdev restart from active queue since all the vdevs are UP
     */
    restart_vdev = pdev_mlme->pdev_restart.vdev;
    pdev_mlme->pdev_restart.vdev = NULL;

    active_cmd_type = wlan_serialization_get_vdev_active_cmd_type(restart_vdev);

    if (active_cmd_type == WLAN_SER_CMD_PDEV_CSA_RESTART)
        wlan_vdev_mlme_ser_remove_request(restart_vdev,
                                      wlan_vdev_get_id(restart_vdev),
                                      WLAN_SER_CMD_PDEV_CSA_RESTART);
    else
        wlan_vdev_mlme_ser_remove_request(restart_vdev,
                                      wlan_vdev_get_id(restart_vdev),
                                      WLAN_SER_CMD_PDEV_RESTART);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_cancel_scan_on_acs_vap(wlan_if_t vap)
{
    struct scan_cancel_request *cancel_req = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    osif_dev *osifp;

    if (!vap || !vap->vdev_obj) {
        return QDF_STATUS_E_INVAL;
    }

    osifp = (osif_dev *)vap->iv_ifp;
    vdev = vap->vdev_obj;
    pdev = wlan_vdev_get_pdev(vdev);

    cancel_req = qdf_mem_malloc(sizeof(*cancel_req));
    if (!cancel_req)
        return QDF_STATUS_E_NOMEM;

    cancel_req->vdev = vdev;
    cancel_req->cancel_req.requester = osifp->scan_requestor;
    cancel_req->cancel_req.scan_id = 0;
    cancel_req->cancel_req.req_type = WLAN_SCAN_CANCEL_VDEV_ALL;
    cancel_req->cancel_req.vdev_id = wlan_vdev_get_id(vdev);
    cancel_req->cancel_req.pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);

    return ucfg_scan_cancel(cancel_req);
}

QDF_STATUS
wlan_mlme_release_vdev_req(struct wlan_objmgr_vdev *vdev,
                           enum wlan_serialization_cmd_type cmd_type,
                           int32_t notify_status)
{
    wlan_if_t vap;
    uint8_t vdev_id;
    uint8_t notify_osif;
    struct wlan_objmgr_psoc *psoc;
    enum wlan_serialization_cmd_type active_cmd_type;
    struct wlan_mlme_ser_data *data;
    struct wlan_objmgr_pdev *pdev;
    struct pdev_mlme_obj *pdev_mlme;
    struct wlan_mbss_ev_data mbss_ev = {0};

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    vdev_id = wlan_vdev_get_id(vdev);
    psoc = wlan_vdev_get_psoc(vdev);
    if (!vap || !psoc) {
        wlan_mlme_err("%s is null", vap ? "psoc" : "vap");
        return QDF_STATUS_E_INVAL;
    }

    active_cmd_type = wlan_serialization_get_vdev_active_cmd_type(vdev);
    /* If requested release command is STOP, remove command though active
     * command type is not STOP, otherwise release only if active command and
     * requested command types match
     */
    if ((cmd_type != active_cmd_type) &&
        (active_cmd_type == WLAN_SER_CMD_VDEV_STOP_BSS)) {
        wlan_mlme_nofl_debug("cmd mismatch, v:%u ct:%d rt:%d", vdev_id,
                             cmd_type, active_cmd_type);
        return QDF_STATUS_E_FAILURE;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    pdev_mlme = wlan_pdev_mlme_get_cmpt_obj(pdev);
    if (!pdev_mlme) {
        wlan_mlme_err("Null input");
        return QDF_STATUS_E_INVAL;
    }

    /*
     * VDEV has come up, clear the quiet bit that was set
     * when vdev was going down.
     */
    if ((active_cmd_type == WLAN_SER_CMD_VDEV_START_BSS ||
            active_cmd_type == WLAN_SER_CMD_VDEV_RESTART ||
            active_cmd_type == WLAN_SER_CMD_PDEV_RESTART) &&
            (notify_status == EOK))
        wlan_mlo_ap_vdev_quiet_clear(vdev);

    if (pdev_mlme->pdev_restart.vdev)
        return mlme_check_and_rel_pdev_restart(vdev, pdev_mlme);

    /*
     * If the cmd is activated in scheduler context, then wait until cmd
     * completes its execution i.e do not free the cmd now.
     */
    data = wlan_serialization_get_active_cmd(psoc, vdev_id, active_cmd_type);
    if (data && data->cmd_in_sched) {
        wlan_mlme_nofl_err("IF_REL "PSOC_VDEV_ID "exec in sched type:%d",
                           wlan_psoc_get_id(psoc), vdev_id, active_cmd_type);
        return QDF_STATUS_E_PENDING;
    }

    /*
     * If vdev restart is complete, then up complete cb requests for
     * removal for start bss instead of vdev restart. If the active cmd type
     * is vdev restart then replace the cmd type with the active cmd type.
     */
    if (cmd_type == WLAN_SER_CMD_VDEV_START_BSS &&
            active_cmd_type == WLAN_SER_CMD_VDEV_RESTART)
        cmd_type = WLAN_SER_CMD_VDEV_RESTART;

    /*
     * If the requested cmd type and the active cmd type is same,
     * only then call mlme and osif callback.
     */
    if (cmd_type == active_cmd_type && notify_status == EOK) {
        data = wlan_serialization_get_active_cmd(psoc, vdev_id, cmd_type);
        if (data) {
            if (cmd_type == WLAN_SER_CMD_VDEV_STOP_BSS) {
                wlan_mlme_debug(PSOC_VDEV_ID"down, call osif_start_acs_on_other_vaps",
                        wlan_vdev_get_psoc_id(vdev), wlan_vdev_get_id(vdev));
                wlan_cancel_scan_on_acs_vap(vap);
                /* Send stop ACS for this VDEV and MBSS frmework will
                 * trigger ACS on another VDEV as required */
                mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_ACS;
                wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);

                mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_HT40;
                wlan_mbss_if_mgr_send_event(vap->vdev_obj, &mbss_ev);
            }

            notify_osif = data->notify_osif;
            if ((notify_osif == WLAN_MLME_NOTIFY_MLME ||
                 notify_osif == WLAN_MLME_NOTIFY_OSIF)) {
                /* MLME Post processing handler */
                wlan_mlme_vdev_cmd_handler(vdev, cmd_type);

                /* OSIF Post processing handler */
                if (notify_osif == WLAN_MLME_NOTIFY_OSIF)
                    osif_mlme_notify_handler(vap, cmd_type);
            }
        }
    }
    wlan_mlme_nofl_debug("IF_REL "PSOC_VDEV_ID"notif:%d ct:%d rt:%d",
                         wlan_vdev_get_psoc_id(vdev), vdev_id,
                         notify_status, cmd_type, active_cmd_type);
    if (active_cmd_type < WLAN_SER_CMD_MAX)
        wlan_vdev_mlme_ser_remove_request(vdev, vdev_id, active_cmd_type);

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlme_cmd_abort_flush_cb(struct scheduler_msg *msg)
{
    struct wlan_objmgr_vdev *vdev = msg->bodyptr;
    wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
    return QDF_STATUS_SUCCESS;

}

static QDF_STATUS mlme_cmd_abort_start_sched_cb(struct scheduler_msg *msg)
{
    struct wlan_objmgr_vdev *vdev = msg->bodyptr;

    wlan_vdev_mlme_ser_remove_request(vdev, wlan_vdev_get_id(vdev),
                                      WLAN_SER_CMD_VDEV_START_BSS);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlme_cmd_abort_restart_sched_cb(struct scheduler_msg *msg)
{
    struct wlan_objmgr_vdev *vdev = msg->bodyptr;

    wlan_vdev_mlme_ser_remove_request(vdev, wlan_vdev_get_id(vdev),
                                      WLAN_SER_CMD_VDEV_RESTART);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlme_cmd_abort(struct wlan_objmgr_vdev *vdev)
{
    enum wlan_serialization_cmd_type active_cmd;
    struct scheduler_msg msg = {0};
    QDF_STATUS ret;

    active_cmd = wlan_serialization_get_vdev_active_cmd_type(vdev);

    /*
     * Serialization command abort is not applicable for
     * 1. VDEV STOP cmd
     * 2. PDEV Restart cmd
     * 3. PDEV CSA Restart cmd
     */
    if (active_cmd == WLAN_SER_CMD_VDEV_STOP_BSS ||
        active_cmd == WLAN_SER_CMD_PDEV_RESTART ||
        active_cmd == WLAN_SER_CMD_PDEV_CSA_RESTART) {
        wlan_mlme_err("Skip cmd abort for vdev:%d cmd:%d",
                      wlan_vdev_get_id(vdev), active_cmd);
        return QDF_STATUS_E_INVAL;
    }

    /* In case, START is activated and before we post to scheduler and we
     * set the activation_ctx flag we receive the stop command,
     * we trigger abort, and the function mlme_is_active_cmd_in_sched_ctx
     * would return failure and we remove the START cmd.
     * So we wait for 5 msec for making sure the activation cb is
     * completed and the scheduler flas is set correctly and the function
     * mlme_is_active_cmd_in_sched_ctx returns appropriate value.
     */
    while (QDF_IS_STATUS_SUCCESS(wlan_ser_get_cmd_activation_status(vdev))) {
        qdf_sleep_us(5000);
    }

    ret = wlan_ser_validate_umac_cmd(vdev,
                                     active_cmd,
                                     mlme_is_active_cmd_in_sched_ctx);

    if (ret == QDF_STATUS_SUCCESS) {
        if (active_cmd == WLAN_SER_CMD_VDEV_START_BSS) {
            msg.callback = mlme_cmd_abort_start_sched_cb;
        } else if (active_cmd == WLAN_SER_CMD_VDEV_RESTART) {
            msg.callback = mlme_cmd_abort_restart_sched_cb;
        } else {
            /*
             * When a new command is added to be part of abort sequence then add
             * a corresponding scheduler callback here
             */
            wlan_mlme_err("Unknown active_cmd_type %u", active_cmd);
            return QDF_STATUS_E_FAILURE;
        }

        ret = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_SCHEDULER_ID);
        if (QDF_IS_STATUS_ERROR(ret))
            return ret;

        msg.bodyptr = vdev;
        msg.flush_callback = mlme_cmd_abort_flush_cb;
        ret = scheduler_post_message(QDF_MODULE_ID_MLME, QDF_MODULE_ID_MLME,
                                     QDF_MODULE_ID_MLME, &msg);
        if (QDF_IS_STATUS_ERROR(ret)) {
            wlan_objmgr_vdev_release_ref(vdev, WLAN_SCHEDULER_ID);
            wlan_mlme_err("failed to post scheduler_msg active cmd:%u",
                          active_cmd);
            return ret;
        }
    } else if (ret == QDF_STATUS_E_FAILURE) {
        wlan_vdev_mlme_ser_remove_request(vdev, wlan_vdev_get_id(vdev),
                                          active_cmd);
    } else {
        wlan_mlme_err("Active cmd not found type:%u", active_cmd);
        return ret;
    }

    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlme_start_vdev(struct wlan_objmgr_vdev *vdev,
                                  uint32_t f_scan, uint8_t notify_osif)
{
    struct wlan_serialization_command cmd = {0};
    struct wlan_mlme_ser_data *data;
    QDF_STATUS status;
    enum wlan_serialization_status ret;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);

    wlan_mlme_debug(PSOC_VDEV_ID"notify:%d", wlan_vdev_get_psoc_id(vdev),
                      vdev_id, notify_osif);
    data = qdf_mem_malloc(sizeof(*data));
    if (!data)
        return QDF_STATUS_E_NOMEM;

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_mem_free(data);
        return QDF_STATUS_E_BUSY;
    }

    data->vdev = vdev;
    data->flags = f_scan;
    data->notify_osif = notify_osif;

    cmd.source = WLAN_UMAC_COMP_MLME;
    cmd.vdev = vdev;
    cmd.cmd_type = WLAN_SER_CMD_VDEV_START_BSS;
    cmd.cmd_cb = mlme_ser_proc_vdev_start;
    cmd.cmd_id = vdev_id;
    cmd.umac_cmd = data;
    cmd.cmd_timeout_duration = WLAN_MLME_SER_CMD_TIMEOUT_MS;

    ret = wlan_vdev_mlme_ser_start_bss(&cmd);
    if (ret != WLAN_SER_CMD_ACTIVE && ret != WLAN_SER_CMD_PENDING) {
        wlan_mlme_nofl_err("IF_START "PSOC_VDEV_ID"err:%d",
                          wlan_vdev_get_psoc_id(vdev), vdev_id, ret);
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);
        qdf_mem_free(data);
        return QDF_STATUS_E_FAILURE;
    }

    wlan_mlme_nofl_debug("IF_START "PSOC_VDEV_ID"notif:%d ret:%d %s",
                          wlan_vdev_get_psoc_id(vdev), vdev_id, 
                           notify_osif, ret, ret ? "AQ" : "PQ");
    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS mlme_stop_vdev(struct wlan_objmgr_vdev *vdev,
                                 uint32_t flags, uint8_t notify_osif)
{
    struct wlan_mlme_ser_data *data;
    QDF_STATUS status;
    enum wlan_serialization_status ret;
    struct wlan_serialization_command cmd = {0};
    wlan_if_t vap;
    osif_dev *osifp;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);

    wlan_mlme_debug(PSOC_VDEV_ID"notify:%d",wlan_vdev_get_psoc_id(vdev),
                     vdev_id, notify_osif);
    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        wlan_mlme_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    osifp = (osif_dev *)vap->iv_ifp;

    data = qdf_mem_malloc(sizeof(*data));
    if (!data)
        return QDF_STATUS_E_NOMEM;

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_mem_free(data);
        return QDF_STATUS_E_BUSY;
    }

    data->vdev = vdev;
    data->flags = flags;
    data->notify_osif = notify_osif;

    cmd.source = WLAN_UMAC_COMP_MLME;
    cmd.vdev = vdev;
    cmd.cmd_type = WLAN_SER_CMD_VDEV_STOP_BSS;
    cmd.cmd_cb = mlme_ser_proc_vdev_stop;
    cmd.cmd_id = vdev_id;
    cmd.umac_cmd = data;
    cmd.cmd_timeout_duration = WLAN_MLME_SER_CMD_TIMEOUT_MS;

    /* Mark to disable queue if vdev delete has triggered this stop cmd */
    if (osifp && osifp->is_delete_in_progress) {
        cmd.queue_disable = 1;
        wlan_mlme_nofl_info("IF_STOP: V:%u Serialization queue disable",
                            vdev_id);
    }

    ret = wlan_vdev_mlme_ser_stop_bss(&cmd);
    if (ret != WLAN_SER_CMD_ACTIVE && ret != WLAN_SER_CMD_PENDING &&
        ret != WLAN_SER_CMD_ALREADY_EXISTS) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);
        qdf_mem_free(data);
        wlan_mlme_nofl_debug("IF_STOP "PSOC_VDEV_ID"err_ret:%d",wlan_vdev_get_psoc_id(vdev),
                              vdev_id, ret);
        return QDF_STATUS_E_FAILURE;
    }

    wlan_mlme_nofl_debug("IF_STOP "PSOC_VDEV_ID"notif:%d ret:%d %s",
                         wlan_vdev_get_psoc_id(vdev), vdev_id,
                         notify_osif, ret, ret ? "AQ" : "PQ");
    if (ret == WLAN_SER_CMD_PENDING) {
        /*
         * To avoid the stop cmd waiting in pending queue, we are
         * triggering active cmd abort, where remove request for the cmd in the
         * active queue is sent.
         */
        wlan_mlme_nofl_debug("IF_STOP "PSOC_VDEV_ID"act cmd abort",
                            wlan_vdev_get_psoc_id(vdev), vdev_id);
        mlme_cmd_abort(vdev);
    }
    return QDF_STATUS_SUCCESS;
}

static QDF_STATUS
mlme_pdev_or_vdev_restart(struct wlan_objmgr_vdev *vdev,
                          enum wlan_serialization_cmd_type type)
{
    struct wlan_serialization_command cmd = {0};
    struct wlan_mlme_ser_data *data;
    QDF_STATUS status;
    enum wlan_serialization_status ret;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);

    wlan_mlme_debug("vdev:%d type:%d", vdev_id, type);
    data = qdf_mem_malloc(sizeof(*data));
    if (!data)
        return QDF_STATUS_E_NOMEM;

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_mem_free(data);
        return QDF_STATUS_E_BUSY;
    }

    cmd.source = WLAN_UMAC_COMP_MLME;
    cmd.vdev = vdev;
    cmd.cmd_id = vdev_id;
    cmd.umac_cmd = data;
    cmd.cmd_type = type;
    cmd.cmd_timeout_duration = MLME_SER_RESTART_CMD_TIMEOUT_MS;

    if (type == WLAN_SER_CMD_VDEV_RESTART) {
        data->notify_osif = WLAN_MLME_NOTIFY_MLME;
        cmd.cmd_cb = mlme_ser_proc_vdev_restart;
        ret = wlan_vdev_mlme_ser_vdev_restart(&cmd);
    } else {
        cmd.cmd_cb = mlme_ser_proc_pdev_restart;
        cmd.is_blocking = true;
        ret = wlan_vdev_mlme_ser_pdev_restart(&cmd);
    }

    if (ret != WLAN_SER_CMD_ACTIVE && ret != WLAN_SER_CMD_PENDING) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);
        qdf_mem_free(data);
        wlan_mlme_debug("RESTART %u "PSOC_VDEV_ID"err:%d", type,
                        wlan_vdev_get_psoc_id(vdev), vdev_id, ret);
        if (ret == WLAN_SER_CMD_ALREADY_EXISTS) {
            wlan_mlme_debug("Restart cmd already exists, cfg will be applied later");
            return QDF_STATUS_SUCCESS;
        }
        return QDF_STATUS_E_FAILURE;
    }

    wlan_mlme_nofl_debug("IF_RESTART "PSOC_VDEV_ID"ret:%d %s",
                         wlan_vdev_get_psoc_id(vdev),
                         vdev_id, ret, ret ? "AQ" : "PQ");
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_start_vdev(struct wlan_objmgr_vdev *vdev,
                                uint32_t f_scan, uint8_t notify_osif)
{
	QDF_STATUS status;
	QDF_STATUS ret_val;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wlan_mlme_info("unable to get reference");
		return QDF_STATUS_E_BUSY;
	}

	wlan_vdev_mlme_cmd_lock(vdev);
	ret_val = mlme_start_vdev(vdev, f_scan, notify_osif);
	wlan_vdev_mlme_cmd_unlock(vdev);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);

	return ret_val;
}

QDF_STATUS wlan_mlme_stop_vdev(struct wlan_objmgr_vdev *vdev,
                               uint32_t flags, uint8_t notify_osif)
{
	QDF_STATUS status;
	QDF_STATUS ret_val;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wlan_mlme_info("unable to get reference");
		return QDF_STATUS_E_BUSY;
	}

	wlan_vdev_mlme_cmd_lock(vdev);
	ret_val = mlme_stop_vdev(vdev, flags, notify_osif);
	wlan_vdev_mlme_cmd_unlock(vdev);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);

	return ret_val;

}

QDF_STATUS wlan_mlme_stop_start_vdev(struct wlan_objmgr_vdev *vdev,
                                     uint32_t flags, uint8_t notify_osif)
{
	QDF_STATUS status;
	QDF_STATUS ret_val;

	status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wlan_mlme_info("unable to get reference");
		return QDF_STATUS_E_BUSY;
	}

	wlan_vdev_mlme_cmd_lock(vdev);
	ret_val = mlme_stop_vdev(vdev, flags, notify_osif);
	ret_val = mlme_start_vdev(vdev, flags, notify_osif);

	wlan_vdev_mlme_cmd_unlock(vdev);

	wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);

	return ret_val;

}

void wlan_mlme_inc_act_cmd_timeout(struct wlan_objmgr_vdev *vdev,
                                   enum wlan_serialization_cmd_type cmd_type)
{
    struct wlan_serialization_command cmd = {0};
    uint32_t dfs_cac_timeout;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_channel *chan;
    cm_ext_t *cm_ext_handle = NULL;

    pdev = wlan_vdev_get_pdev(vdev);
    if (!pdev) {
        wlan_mlme_err("pdev is NULL");
        return;
    }

    if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {
        cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
        if (!cm_ext_handle)
            return;
    }

    chan = wlan_vdev_mlme_get_des_chan(vdev);
    dfs_cac_timeout = dfs_mlme_get_cac_timeout_for_freq(pdev,
                                                        chan->ch_freq,
                                                        chan->ch_cfreq2,
                                                        chan->ch_flags);

    /* Seconds to milliseconds with added delta of ten seconds*/
    dfs_cac_timeout = (dfs_cac_timeout + 10) * 1000;
    if (dfs_cac_timeout < WLAN_MLME_SER_CMD_TIMEOUT_MS)
        dfs_cac_timeout = WLAN_MLME_SER_CMD_TIMEOUT_MS;

    wlan_mlme_debug("DFS timeout:%u", dfs_cac_timeout);
    cmd.vdev = vdev;
    if (cmd_type == WLAN_SER_CMD_VDEV_CONNECT) {
        if (!cm_ext_handle)
            return;
        cmd.cmd_id = cm_ext_handle->cm_conn_rsp.cm_id;
    } else {
        cmd.cmd_id = wlan_vdev_get_id(vdev);
    }

    cmd.cmd_timeout_duration = dfs_cac_timeout;
    cmd.cmd_type = cmd_type;
    cmd.source = WLAN_UMAC_COMP_MLME;
    mlme_ser_inc_act_cmd_timeout(&cmd);
}

void wlan_mlme_wait_for_cmd_completion(struct wlan_objmgr_vdev *vdev)
{
    qdf_event_t vdev_wait_for_active_cmds;
    uint32_t max_wait_iterations =
                   WLAN_SERIALZATION_CANCEL_WAIT_ITERATIONS;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);

    memset(&vdev_wait_for_active_cmds, 0, sizeof(vdev_wait_for_active_cmds));

    qdf_event_create(&vdev_wait_for_active_cmds);
    qdf_event_reset(&vdev_wait_for_active_cmds);

    while(wlan_serialization_get_vdev_active_cmd_type(vdev)
                    != WLAN_SER_CMD_MAX && max_wait_iterations) {
        qdf_wait_single_event(&vdev_wait_for_active_cmds,
                              WLAN_SERIALZATION_CANCEL_WAIT_TIME);
        max_wait_iterations--;
    }

    if (!max_wait_iterations)
            wlan_mlme_err("Waiting for active cmds removal timed out. vdev id: %d\n",
                     vdev_id);

    qdf_event_destroy(&vdev_wait_for_active_cmds);
}

void wlan_mlme_wait_for_scan_cmd_completion(struct wlan_objmgr_vdev *vdev)
{
    qdf_event_t vdev_wait_for_active_cmds;
    uint32_t max_wait_iterations = SCM_CANCEL_SCAN_WAIT_ITERATION;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);
    struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);

    /* Cancel the scan commands from Pending and Active queue */
    wlan_serialization_purge_all_scan_cmd_by_vdev_id(pdev, vdev_id);

    /* Wait for already activated scan commands to complete */
    memset(&vdev_wait_for_active_cmds, 0, sizeof(vdev_wait_for_active_cmds));

    qdf_event_create(&vdev_wait_for_active_cmds);
    qdf_event_reset(&vdev_wait_for_active_cmds);

    while(wlan_vdev_scan_in_progress(vdev) && max_wait_iterations) {
        qdf_wait_single_event(&vdev_wait_for_active_cmds,
                              SCM_CANCEL_SCAN_WAIT_TIME);
        max_wait_iterations--;
    }

    if (!max_wait_iterations)
        wlan_mlme_err("Failed to wait for scans to get complete: %d\n",
                vdev_id);

    qdf_event_destroy(&vdev_wait_for_active_cmds);
}

QDF_STATUS wlan_mlme_dev_restart(struct wlan_objmgr_vdev *vdev,
                                 enum wlan_serialization_cmd_type type)
{
    QDF_STATUS status;
    QDF_STATUS ret_val;

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        wlan_mlme_info("unable to get reference");
        return QDF_STATUS_E_BUSY;
    }

    wlan_vdev_mlme_cmd_lock(vdev);
    ret_val = mlme_pdev_or_vdev_restart(vdev, type);
    wlan_vdev_mlme_cmd_unlock(vdev);

    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);

    return ret_val;
}

QDF_STATUS mlme_vdev_add_cmd_to_pq(struct wlan_objmgr_vdev *vdev,
                                   enum wlan_serialization_cmd_type type,
                                   uint8_t high_prio)
{
    struct wlan_serialization_command cmd = {0};
    struct wlan_mlme_ser_data *data;
    QDF_STATUS status;
    enum wlan_serialization_status ret;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);

    wlan_mlme_info("vdev:%d cmd_type:%d", vdev_id, type);

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status))
        return QDF_STATUS_E_BUSY;

    data = qdf_mem_malloc(sizeof(*data));
    if (!data) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);
        return QDF_STATUS_E_NOMEM;
    }

    data->vdev = vdev;
    cmd.source = WLAN_UMAC_COMP_MLME;
    cmd.vdev = vdev;
    cmd.cmd_type = type;
    cmd.cmd_id = vdev_id;
    cmd.umac_cmd = data;
    cmd.is_high_priority = high_prio;
    cmd.cmd_timeout_duration = WLAN_MLME_SER_CMD_TIMEOUT_MS;

    if (type == WLAN_SER_CMD_VDEV_START_BSS)
        cmd.cmd_cb = mlme_ser_proc_vdev_start;
    else if (type == WLAN_SER_CMD_VDEV_STOP_BSS)
        cmd.cmd_cb = mlme_ser_proc_vdev_stop;
    else {
        wlan_mlme_err(PSOC_VDEV_ID"Unknown cmd type:%d",
                          wlan_vdev_get_psoc_id(vdev),vdev_id, type);
        goto add_cmd_fail;
    }

    ret = wlan_serialization_request(&cmd);
    if (ret != WLAN_SER_CMD_ACTIVE && ret != WLAN_SER_CMD_PENDING) {
        wlan_mlme_err(PSOC_VDEV_ID"type:%d Enqueue failed ret:%d",
                      wlan_vdev_get_psoc_id(vdev), vdev_id, type, ret);
        goto add_cmd_fail;
    }

    wlan_mlme_nofl_debug("IF_ADD "PSOC_VDEV_ID"type:%d ret:%d",
                         wlan_vdev_get_psoc_id(vdev), vdev_id, type, ret);
    return QDF_STATUS_SUCCESS;

add_cmd_fail:
    qdf_mem_free(data);
    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);

    return QDF_STATUS_E_FAILURE;
}

static QDF_STATUS mlme_pdev_csa_restart(struct wlan_objmgr_vdev *vdev,
                                 uint32_t flags, uint8_t notify_osif)
{
    struct wlan_mlme_ser_data *data;
    QDF_STATUS status;
    enum wlan_serialization_status ret;
    struct wlan_serialization_command cmd = {0};
    wlan_if_t vap;
    wlan_dev_t ic;
    wlan_if_t tmp_vap;
    osif_dev *osifp;
    uint8_t vdev_id = wlan_vdev_get_id(vdev);

    wlan_mlme_debug("vdev:%d notify:%d", vdev_id, notify_osif);
    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap) {
        wlan_mlme_err("vap is NULL");
        return QDF_STATUS_E_FAILURE;
    }
    osifp = (osif_dev *)vap->iv_ifp;
    ic = vap->iv_ic;
    data = qdf_mem_malloc(sizeof(*data));
    if (!data)
        return QDF_STATUS_E_NOMEM;

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_mem_free(data);
        return QDF_STATUS_E_BUSY;
    }

    data->vdev = vdev;
    data->flags = flags;
    data->notify_osif = notify_osif;

    cmd.source = WLAN_UMAC_COMP_MLME;
    cmd.vdev = vdev;
    cmd.cmd_type = WLAN_SER_CMD_PDEV_CSA_RESTART;
    cmd.cmd_cb = mlme_ser_proc_pdev_csa_restart;
    cmd.cmd_id = vdev_id;
    cmd.umac_cmd = data;
    cmd.is_blocking = true;
    cmd.cmd_timeout_duration = MLME_SER_RESTART_CMD_TIMEOUT_MS;

    TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
        if (tmp_vap->iv_opmode == IEEE80211_M_STA) {
            wlan_mlme_cm_stop(tmp_vap->vdev_obj, CM_OSIF_CFG_DISCONNECT,
                              REASON_DISASSOC_NETWORK_LEAVING, false);
        }
    }

    ret = wlan_vdev_mlme_ser_pdev_csa_restart(&cmd);
    if (ret != WLAN_SER_CMD_ACTIVE && ret != WLAN_SER_CMD_PENDING &&
        ret != WLAN_SER_CMD_ALREADY_EXISTS) {
        wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);
        qdf_mem_free(data);
        wlan_mlme_nofl_debug("IF_CSA_RESTART "PSOC_VDEV_ID"err_ret:%d",
                             wlan_vdev_get_psoc_id(vdev), vdev_id, ret);
        return QDF_STATUS_E_FAILURE;
    }

    wlan_mlme_nofl_debug("IF_CSA_RESTART "PSOC_VDEV_ID"notif:%d ret:%d %s",
                           wlan_vdev_get_psoc_id(vdev), vdev_id, 
                           notify_osif, ret, ret ? "AQ" : "PQ");

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_pdev_csa_restart(struct wlan_objmgr_vdev *vdev,
        uint32_t flags, uint8_t notify_osif)
{
    QDF_STATUS status;
    QDF_STATUS ret_val;

    status = wlan_objmgr_vdev_try_get_ref(vdev, WLAN_MLME_SER_IF_ID);
    if (QDF_IS_STATUS_ERROR(status)) {
        wlan_mlme_info("unable to get reference");
        return QDF_STATUS_E_BUSY;
    }

    wlan_vdev_mlme_cmd_lock(vdev);
    ret_val = mlme_pdev_csa_restart(vdev, flags, notify_osif);
    wlan_vdev_mlme_cmd_unlock(vdev);

    wlan_objmgr_vdev_release_ref(vdev, WLAN_MLME_SER_IF_ID);

    return ret_val;
}

QDF_STATUS wlan_mlme_cm_start(struct wlan_objmgr_vdev *vdev,
                              enum wlan_cm_source source)
{
    struct wlan_cm_connect_req req = {0};
    QDF_STATUS ret;
    enum reg_6g_ap_type reg_cur_6g_ap_pwr_type;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_vdev *assoc_vdev = NULL;

    req.source = source;

    if (mlo_is_mld_sta(vdev)) {
        assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vdev);
        if (!assoc_vdev) {
            QDF_ASSERT(0);
            return QDF_STATUS_E_FAILURE;
        }
    } else {
        assoc_vdev = vdev;
    }

    ret = wlan_vdev_mlme_get_ssid(assoc_vdev, req.ssid.ssid,
                                  &req.ssid.length);
    if (QDF_IS_STATUS_ERROR(ret)) {
        wlan_mlme_err("SSID not found");
        return ret;
    }

    ret = wlan_vdev_get_bss_peer_mac(assoc_vdev, &req.bssid);
    if (QDF_IS_STATUS_ERROR(ret)) {
        wlan_mlme_err("BSSID not found");
        return ret;
    }

    pdev = wlan_vdev_get_pdev(vdev);
    wlan_reg_get_cur_6g_ap_pwr_type(pdev, &reg_cur_6g_ap_pwr_type);
    /*
     * Make sure that the sta is in LPI power mode, so that it can scan
     * all the 6G LPI channels and connect to root AP. It can move to SP
     * after connection complete.
     */
    if (reg_cur_6g_ap_pwr_type == REG_STANDARD_POWER_AP) {
        wlan_reg_set_ap_pwr_and_update_chan_list(pdev, REG_INDOOR_AP);
        mlme_update_chan_list_and_restart_vaps(wlan_vdev_get_ic(vdev), true);
    }

    req.crypto.auth_type =
        wlan_crypto_get_param(assoc_vdev, WLAN_CRYPTO_PARAM_AUTH_MODE);

    req.crypto.ciphers_pairwise =
        wlan_crypto_get_param(assoc_vdev, WLAN_CRYPTO_PARAM_UCAST_CIPHER);

    req.crypto.group_cipher =
        wlan_crypto_get_param(assoc_vdev, WLAN_CRYPTO_PARAM_MCAST_CIPHER);

    req.crypto.akm_suites =
        wlan_crypto_get_param(assoc_vdev, WLAN_CRYPTO_PARAM_KEY_MGMT);

    req.crypto.rsn_caps =
        wlan_crypto_get_param(assoc_vdev, WLAN_CRYPTO_PARAM_RSN_CAP);

    return mlo_connect(assoc_vdev, &req);
}

#ifdef WLAN_FEATURE_11BE_MLO
static inline
void wlan_reset_ml_reconfig_flag(struct wlan_objmgr_vdev *vdev,
                                 void *arg)
{
    struct vdev_mlme_obj * vdev_mlme;

    if (!vdev)
        return;

    vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vdev);
    if (!vdev_mlme) {
        qdf_err("Vdev mlme component object is NULL");
        return;
    }
    vdev_mlme->ml_reconfig_started = false;
}
#endif
QDF_STATUS wlan_mlme_cm_stop(struct wlan_objmgr_vdev *vdev,
                             enum wlan_cm_source source,
                             enum wlan_reason_code reason, bool sync)
{
#ifdef WLAN_FEATURE_11BE_MLO

    if (wlan_vdev_mlme_is_mlo_vdev(vdev)) {
        mlo_iterate_connected_vdev_list(vdev,
                                        wlan_reset_ml_reconfig_flag,
                                        NULL);
    }
#endif

    if (sync && ((!wlan_vdev_mlme_is_mlo_vdev(vdev)) || (wlan_mlo_get_sta_mld_ctx_count() == 1)))
        return mlo_sync_disconnect(vdev, source, reason, NULL);
    else
        return mlo_disconnect(vdev, source, reason, NULL);
}

QDF_STATUS wlan_mbss_start_acs(struct wlan_objmgr_vdev *vdev, void *arg)
{
	QDF_STATUS status;
	wlan_if_t vap;

	vap = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!vap) {
		qdf_info("vap is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_info("ACS Arg: %pK",arg);
	status = wlan_autoselect_find_infra_bss_channel(
			vap, arg, &wlan_mbss_done_acs);

	return status;
}

void wlan_mbss_done_acs(struct wlan_objmgr_vdev *vdev, void *arg,
			QDF_STATUS status)
{
	struct wlan_mbss_ev_data mbss_ev = {0};

	if (QDF_IS_STATUS_SUCCESS(status)){
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_DONE_ACS;
	} else {
		mbss_debug("ACS cancel done for vdev: %d",
			   wlan_vdev_get_id(vdev));
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_CANCEL_ACS;
	}

	mbss_ev.ext_cb = NULL;
	mbss_ev.source.acs_source = MBSS_ACS_SRC_MAX;
	mbss_ev.arg = arg;
	mbss_ev.status = status;

	qdf_info("Channel: %pK", arg);
	wlan_mbss_if_mgr_send_event(vdev, &mbss_ev);

	return;
}

QDF_STATUS wlan_mbss_cancel_acs(struct wlan_objmgr_vdev *vdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	wlan_if_t vap;

	vap = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!vap) {
		qdf_info("vap is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	mbss_debug("Cancelling ACS for vdev: %d", wlan_vdev_get_id(vdev));
	wlan_autoselect_cancel_selection(vap);

	return status;
}

QDF_STATUS wlan_mbss_start_ht40(struct wlan_objmgr_vdev *vdev, void *arg)
{
	QDF_STATUS status;
	wlan_if_t vap;

	vap = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!vap) {
		qdf_info("vap is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_info("HT40 Arg: %pK",arg);
	status = wlan_attempt_ht40_bss(vap, &wlan_mbss_done_ht40);

	return status;
}

void wlan_mbss_done_ht40(struct wlan_objmgr_vdev *vdev, void *arg,
			 QDF_STATUS status)
{
	struct wlan_mbss_ev_data mbss_ev = {0};

	if (QDF_IS_STATUS_SUCCESS(status)){
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_DONE_HT40;
	} else {
		mbss_debug("HT40 cancel done for vdev: %d",
			   wlan_vdev_get_id(vdev));
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_CANCEL_HT40;
	}

	mbss_ev.ext_cb = NULL;
	mbss_ev.source.acs_source = MBSS_ACS_SRC_MAX;
	mbss_ev.arg = arg;
	mbss_ev.status = status;

	qdf_info("Channel: %pK", arg);

	wlan_mbss_if_mgr_send_event(vdev, &mbss_ev);

	return;
}

QDF_STATUS wlan_mbss_cancel_ht40(struct wlan_objmgr_vdev *vdev, void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	wlan_if_t vap;

	vap = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!vap) {
		qdf_info("vap is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	mbss_debug("Cancelling HT40 for vdev: %d", wlan_vdev_get_id(vdev));
	wlan_autoselect_cancel_selection(vap);

	return status;
}

QDF_STATUS wlan_mbss_register_ext_ops(void)
{
	QDF_STATUS status;
	struct wlan_mbss_ext_cb ops;

	ops.mbss_start_acs = &wlan_mbss_start_acs;
	ops.mbss_cancel_acs = &wlan_mbss_cancel_acs;
	ops.mbss_start_ht40 = &wlan_mbss_start_ht40;
	ops.mbss_cancel_ht40 = &wlan_mbss_cancel_ht40;

	ops.mbss_start_ap_vdevs_cb = &wlan_mlme_start_ap_vdevs;
	ops.mbss_stop_ap_vdevs_cb = NULL;
	ops.mbss_stop_start_ap_vdevs_cb = &wlan_mlme_stop_start_ap_vdevs;
	ops.mbss_stop_start_ap_monitor_vdevs_cb = &wlan_mlme_stop_start_ap_monitor_vdevs;
	ops.mbss_stop_ap_monitor_vdevs_cb = &wlan_mlme_stop_ap_monitor_vdevs;
	ops.mbss_stop_sta_vdevs_cb = &wlan_mlme_stop_sta_vdevs;
	ops.mbss_start_vdevs_cb = &wlan_mlme_start_vdevs;
	ops.mbss_start_ap_monitor_vdevs_cb = &wlan_mlme_start_ap_monitor_vdevs;
	ops.mbss_stop_vdevs_cb = &wlan_mlme_stop_vdevs;
	ops.mbss_start_sta_vdevs_cb = &wlan_mlme_start_sta_vdevs;
	ops.mbss_start_restart_ap_monitor_vdevs_cb = &wlan_mlme_start_restart_ap_monitor_vdevs;
#ifdef WLAN_FEATURE_11BE_MLO
	ops.mbss_start_standalone_ap_vdevs_cb = &wlan_mlme_start_ap_vdevs;
	ops.mbss_stop_standalone_ap_vdevs_cb = &wlan_mlme_stop_vdevs;
	ops.mbss_start_bridge_vdevs_cb = &wlan_mlme_start_bridge_vdevs;
#endif

	status = wlan_mbss_set_ext_ops(&ops);
	return status;
}

void wlan_mlme_start_restart_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
					     void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	if ((wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) ||
	    (wlan_vap_get_opmode(vap) == IEEE80211_M_MONITOR)) {
		if(wlan_vdev_mlme_get_state(vap->vdev_obj) == WLAN_VDEV_S_INIT) {
			if (!IEEE80211_IS_MLO_BRIDGE_VAP(vap))
				wlan_mlme_start_ap_vdevs(pdev, object, arg);
			else
				wlan_mlme_start_bridge_vdevs(pdev, object, arg);
#if MESH_MODE_SUPPORT
		} else if (vap->iv_mesh_vap_mode &&
			(vap->iv_bsschan != vap->iv_ic->ic_curchan)) {
		/*
		* Since mesh mode operates independently, it will not
		* be in INIT state. But, mesh AP restart needs to be done
		* to ensure it moves to the new operating channel.
		*/
			wlan_mlme_stop_start_ap_vdevs(pdev, object, arg);
#endif
		}
	}
}

void wlan_mlme_start_sta_vdevs(struct wlan_objmgr_pdev *pdev,
			       void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	if (!((wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) ||
		(wlan_vap_get_opmode(vap) == IEEE80211_M_MONITOR))) {
		wlan_mlme_cm_start(vap->vdev_obj, CM_OSIF_CONNECT);
	}
}

void wlan_mlme_start_bridge_vdevs(struct wlan_objmgr_pdev *pdev,
			      void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	if ((wlan_vap_get_opmode(vap) != IEEE80211_M_HOSTAP) ||
	    !IEEE80211_IS_MLO_BRIDGE_VAP(vap))
		return;

	if (arg) {
		if(((struct ieee80211_vap_info *)arg)->chan) {
			*vap->iv_des_chan[vap->iv_des_mode] = *((struct ieee80211_vap_info *)arg)->chan;
		}
		vap->iv_no_cac = ((struct ieee80211_vap_info *)arg)->no_cac;
	} else {
		vap->iv_no_cac = 0;
	}

	wlan_mlme_start_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
}

void wlan_mlme_start_ap_vdevs(struct wlan_objmgr_pdev *pdev,
			      void *object, void *arg)
{
	osif_dev  *osifp = NULL;
	struct net_device *dev;
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	if (wlan_vap_get_opmode(vap) != IEEE80211_M_HOSTAP)
		return;

	osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
	if(osifp == NULL)
		return;

	dev = get_cfg80211_notification_ndev(osifp);
	if (dev == NULL)
		return;

	if (!(dev->flags & IFF_UP))
		return;

	if (arg) {
		if(((struct ieee80211_vap_info *)arg)->chan) {
			*vap->iv_des_chan[vap->iv_des_mode] = *((struct ieee80211_vap_info *)arg)->chan;
		}
		vap->iv_no_cac = ((struct ieee80211_vap_info *)arg)->no_cac;
	} else {
		vap->iv_no_cac = 0;
	}

	wlan_mlme_start_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
}

void wlan_mlme_stop_start_ap_vdevs(struct wlan_objmgr_pdev *pdev,
				   void *object, void *arg)
{
	osif_dev  *osifp = NULL;
	struct net_device *dev;
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
	if(osifp == NULL)
		return;

	/* recovery will bring down all VAPs.
	* reset here will modify the VAP list
	* which recovery is already using */
	if(vap->iv_ic->recovery_in_progress)
		return;

	if((wlan_vap_get_opmode(vap) != IEEE80211_M_HOSTAP))
		return;

	dev = OSIF_TO_NETDEV(osifp);
	if (dev == NULL)
		return;

	if (!(dev->flags & IFF_UP))
		return;

	if(arg) {
		 if(((struct ieee80211_vap_info *)arg)->chan) {
		*vap->iv_des_chan[vap->iv_des_mode] = *((struct ieee80211_vap_info *)arg)->chan;
		}
		vap->iv_no_cac = ((struct ieee80211_vap_info *)arg)->no_cac;
	} else {
		vap->iv_no_cac = 0;
	}

	if (!osifp->is_delete_in_progress)
		wlan_mlme_stop_start_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_MLME);
}

void wlan_mlme_stop_start_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
					   void *object, void *arg)
{
	osif_dev  *osifp = NULL;
	struct net_device *dev;
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
	if(osifp == NULL)
		return;

	/* recovery will bring down all VAPs.
	* reset here will modify the VAP list
	* which recovery is already using */
	if(vap->iv_ic->recovery_in_progress)
		return;

	dev = OSIF_TO_NETDEV(osifp);
	if (dev == NULL)
		return;

	if (!(dev->flags & IFF_UP))
		return;

	if(arg) {
		if(((struct ieee80211_vap_info *)arg)->chan) {
			*vap->iv_des_chan[vap->iv_des_mode] = *((struct ieee80211_vap_info *)arg)->chan;
		}
		vap->iv_no_cac = ((struct ieee80211_vap_info *)arg)->no_cac;
	} else {
		vap->iv_no_cac = 0;
	}

	if (!osifp->is_delete_in_progress) {
		vap->iv_roam_inprogress = FALSE;
		wlan_mlme_stop_start_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_MLME);
	}
}

void wlan_mlme_stop_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
				     void *object, void *arg)
{
	osif_dev  *osifp = NULL;
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
	if(osifp == NULL)
		return;

	/* recovery will bring down all VAPs.
	* bringing down here will modify the VAP list
	* which recovery is already using */
	if(vap->iv_ic->recovery_in_progress)
		return;

	if (!((wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) ||
		(wlan_vap_get_opmode(vap) == IEEE80211_M_MONITOR)))
		return;

#if MESH_MODE_SUPPORT
	if (vap->iv_mesh_vap_mode)
		return;
#endif
	wlan_mlme_stop_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
}

void wlan_mlme_stop_sta_vdevs(struct wlan_objmgr_pdev *pdev,
			      void *object, void *arg)
{
	osif_dev  *osifp = NULL;
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
	if(osifp == NULL)
		return;

	/* recovery will bring down all VAPs.
	* bringing down here will modify the VAP list
	* which recovery is already using */
	if(vap->iv_ic->recovery_in_progress) {
		return;
	}

	if ((wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) ||
		(wlan_vap_get_opmode(vap) == IEEE80211_M_MONITOR))
		return;

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
	wlan_mlme_stacac_restore_defaults(vap);
	wlan_mlme_clear_mlme_req(vap);
#endif

	wlan_mlme_cm_stop(vap->vdev_obj, CM_OSIF_CFG_DISCONNECT,
			  REASON_STA_LEAVING, false);
}

void wlan_mlme_start_vdevs(struct wlan_objmgr_pdev *pdev,
			   void *object, void *arg)
{
	osif_dev  *osifp = NULL;
	struct net_device *dev;
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	osifp  = (osif_dev *)wlan_vap_get_registered_handle(vap);
	if(osifp == NULL)
		return;

	/* recovery will bring down all VAPs.
	* reset here will modify the VAP list
	* which recovery is already using */
	if(vap->iv_ic->recovery_in_progress)
		return;

	if (IEEE80211_IS_MLO_BRIDGE_VAP(vap))
		return;

	dev = OSIF_TO_NETDEV(osifp);
	if (dev == NULL)
		return;

	if (!(dev->flags & IFF_UP))
		return;

	if(arg)
		*vap->iv_des_chan[vap->iv_des_mode] = *(struct ieee80211_ath_channel *)arg;

	if (vap->iv_opmode != IEEE80211_M_STA)
		wlan_mlme_start_vdev(vap->vdev_obj, 1, WLAN_MLME_NOTIFY_NONE);
}

void wlan_mlme_start_ap_monitor_vdevs(struct wlan_objmgr_pdev *pdev,
				      void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	if (!((wlan_vap_get_opmode(vap) == IEEE80211_M_HOSTAP) ||
		(wlan_vap_get_opmode(vap) == IEEE80211_M_MONITOR))) {
		return;
	}

	if(arg)
		*vap->iv_des_chan[vap->iv_des_mode] = *(struct ieee80211_ath_channel *)arg;

	wlan_mlme_start_vdev(vap->vdev_obj, 1, WLAN_MLME_NOTIFY_NONE);
}

void wlan_mlme_stop_vdevs(struct wlan_objmgr_pdev *pdev,
			  void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev;
	wlan_if_t vap;

	vdev = (struct wlan_objmgr_vdev *)object;
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap) {
		qdf_info("vap is null");
		return;
	}

	if (IEEE80211_IS_MLO_BRIDGE_VAP(vap))
		return;

	vap->iv_roam_inprogress = FALSE;
	if (vap->iv_opmode == IEEE80211_M_STA)
		wlan_mlme_cm_stop(vap->vdev_obj,  CM_OSIF_CFG_DISCONNECT,
				  REASON_STA_LEAVING, false);
	else
		wlan_mlme_stop_vdev(vap->vdev_obj, 0, WLAN_MLME_NOTIFY_NONE);
}

void wlan_mlme_if_mgr_ev_send(
		struct wlan_objmgr_vdev *vdev,
		enum wlan_serialization_cmd_type cmd_type)
{
	struct wlan_mbss_ev_data mbss_ev = {0};

	if (cmd_type == WLAN_SER_CMD_VDEV_START_BSS){
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_BSS;
	}
	else if (cmd_type == WLAN_SER_CMD_VDEV_STOP_BSS) {
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_BSS;
	} else {
		return;
	}

	wlan_mbss_if_mgr_send_event(vdev, &mbss_ev);
}

void wlan_mlme_if_mgr_ev_complete_send(
		struct wlan_objmgr_vdev *vdev,
		enum wlan_serialization_cmd_type cmd_type)
{
	struct wlan_mbss_ev_data mbss_ev = {0};

	if (cmd_type == WLAN_SER_CMD_VDEV_START_BSS){
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_START_BSS_COMPLETE;
	}
	else if (cmd_type == WLAN_SER_CMD_VDEV_STOP_BSS) {
		mbss_ev.if_mgr_event = WLAN_IF_MGR_EV_AP_STOP_BSS_COMPLETE;
	} else {
		return;
	}

	wlan_mbss_if_mgr_send_event(vdev, &mbss_ev);
}

#ifdef WLAN_CONFIG_TELEMETRY_AGENT
#define WLAN_LOW5G_BAND_HIGHIEST_FREQ 5490
#define WLAN_HIGH5G_BAND_LOWEST_FREQ 5490
#define CH_FREQ_5150 5150
#define CH_FREQ_5250 5250

int wifi_driver_get_psoc_info(void *obj, struct agent_psoc_iface_init_obj *stats)
{
	struct wlan_objmgr_psoc *psoc = (struct wlan_objmgr_psoc *)obj;
	uint8_t ml_grp_id = 0;

	if (!psoc)
		return -1;

	if (!mlo_psoc_get_grp_id(psoc, &ml_grp_id)) {
		telemetry_agent_err("Invalid MLO Group id");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct agent_psoc_iface_init_obj));
	stats->soc_id = wlan_psoc_get_id(psoc);
	stats->max_peers = wlan_psoc_get_max_peer_count(psoc);
	stats->num_peers = wlan_psoc_get_peer_count(psoc);
	telemetry_agent_debug("Psoc info: id:%d max_peers:%d num_peer:%d",
			      stats->soc_id, stats->max_peers, stats->num_peers);

	return 0;
}
qdf_export_symbol(wifi_driver_get_psoc_info);

int wifi_driver_get_pdev_info(void *obj, struct agent_pdev_iface_init_obj *stats)
{
	struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)obj;
	struct wlan_objmgr_psoc *psoc = NULL;
	qdf_freq_t low_2g, high_2g, low_5g, high_5g;
	uint8_t ml_grp_id = 0;

	if (!pdev)
		return -1;

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return -1;

	psoc = wlan_pdev_get_psoc(pdev);
	if (!mlo_psoc_get_grp_id(psoc, &ml_grp_id)) {
		telemetry_agent_err("Invalid MLO Group id");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct agent_pdev_iface_init_obj));
	stats->link_id = wlan_mlo_get_pdev_hw_link_id(pdev);
	stats->link_id = (ml_grp_id << 8) | stats->link_id;
	stats->soc_id = wlan_psoc_get_id(psoc);

	wlan_reg_get_freq_range(pdev, &low_2g, &high_2g, &low_5g, &high_5g);

	if (wlan_reg_is_range_overlap_2g(low_2g, high_2g)) {
		stats->band = T2LM_BAND_2GHz;
	} else {
#ifdef CONFIG_BAND_6GHZ
		if (wlan_reg_is_range_overlap_6g(low_5g, high_5g)) {
			stats->band = T2LM_BAND_6Ghz;
		} else
#endif
		{
			if (high_5g < WLAN_LOW5G_BAND_HIGHIEST_FREQ)
				stats->band = T2LM_BAND_5GHz_LOW;
			else if (low_5g >= WLAN_HIGH5G_BAND_LOWEST_FREQ)
				stats->band = T2LM_BAND_5GHz_HIGH;
			else
				stats->band = T2LM_BAND_5GHz;
		}
	}

	telemetry_agent_debug("Pdev Info: link_id:%d soc_id:%d band:%d",
			      stats->link_id, stats->soc_id, stats->band);

	return 0;
}
qdf_export_symbol(wifi_driver_get_pdev_info);

int wifi_driver_get_peer_info(void *obj, struct agent_peer_iface_init_obj *stats)
{
	uint8_t is_assoc_link = 1;
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct ieee80211_node *ni = NULL;
#ifdef WLAN_FEATURE_11BE_MLO
	struct wlan_mlo_peer_context *ml_peer = NULL;
	struct qdf_mac_addr *ap_mld_addr = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	uint8_t i = 0;
	uint8_t idx = 0;
	struct wlan_prev_t2lm_negotiated_info t2lm_negotiated_info;
	uint8_t tid = 0;
#endif
#ifdef CONFIG_SAWF
	uint8_t index;
	struct sawf_msduq_svc_params msduq_params[SAWF_MAX_QUEUES] = {0};
#endif

	if (!peer)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_RTT_PASN)
		return -1;

	psoc = wlan_peer_get_psoc(peer);
	if (!psoc)
		return -1;

	pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer));
	if (!pdev)
		return -1;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -1;

	qdf_mem_zero(stats, sizeof(struct agent_peer_iface_init_obj));
	qdf_mem_copy(stats->peer_link_mac, peer->macaddr, 6);
	stats->bw = (uint8_t)ni->ni_chwidth + 1;
	stats->freq = ni->ni_ch_param.ni_freq;
#ifdef WLAN_FEATURE_11BE_MLO
	stats->ieee_link_id = wlan_vdev_get_link_id(wlan_peer_get_vdev(peer));
	if (wlan_peer_is_mlo(peer)) {
		ml_peer = peer->mlo_peer_ctx;
		if (!ml_peer)
			return -1;
		vdev = wlan_peer_get_vdev(peer);
		if (!vdev) {
			 wlan_mlme_debug("VDEV is NULL while getting vdev_id");
			return -1;
		}

		if (!wlan_peer_mlme_is_assoc_peer(peer))
			is_assoc_link = 0;

		stats->vdev_id = wlan_vdev_get_id(vdev);
		ap_mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
		qdf_mem_copy(stats->ap_mld_addr, ap_mld_addr, QDF_MAC_ADDR_SIZE);

		qdf_mem_copy(stats->peer_mld_mac, peer->mldaddr, 6);

		stats->disabled_link_bitmap = ml_peer->ml_dev->t2lm_ctx.established_t2lm.disabled_link_bitmap;

		if (ml_peer->t2lm_policy.t2lm_enable_val != WLAN_T2LM_NOT_SUPPORTED) {
			t2lm_negotiated_info = ml_peer->t2lm_policy.t2lm_negotiated_info;

			for (i = 0; i < QCA_WLAN_VENDOR_T2LM_MAX_VALID_DIRECTION; i++) {
				if (t2lm_negotiated_info.t2lm_info[i].direction == WLAN_T2LM_INVALID_DIRECTION)
					continue;
				stats->t2lm_info[idx].direction = t2lm_negotiated_info.t2lm_info[i].direction + 1;
				stats->t2lm_info[idx].default_link_mapping = t2lm_negotiated_info.t2lm_info[i].default_link_mapping;
				qdf_mem_copy(stats->t2lm_info[idx].t2lm_provisioned_links,
					t2lm_negotiated_info.t2lm_info[i].ieee_link_map_tid,
					sizeof(stats->t2lm_info[i].t2lm_provisioned_links));
				telemetry_agent_debug("idx:%d direction:%d default_link_mapping:%d",
						idx, stats->t2lm_info[idx].direction,
						stats->t2lm_info[idx].default_link_mapping);
				for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
					telemetry_agent_debug("tid:%d t2lm_provisioned_links:%x",
							tid, stats->t2lm_info[idx].t2lm_provisioned_links[tid]);
				}

				idx++;
			}
		}
	} else { /* Provides ap_mld_addr(if_any) for legacy clients via init stats */
		vdev = wlan_peer_get_vdev(peer);
		if (!vdev) {
			wlan_mlme_debug("VDEV is NULL while getting vdev_id");
			return -1;
		}

		ap_mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);
		qdf_mem_copy(stats->ap_mld_addr, ap_mld_addr, QDF_MAC_ADDR_SIZE);
	}
	qdf_mem_copy(stats->caps.tx_mcs_nss_map, ni->ni_eht.ehtcap_txmcsnssmap, sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
	qdf_mem_copy(stats->caps.rx_mcs_nss_map, ni->ni_eht.ehtcap_rxmcsnssmap, sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);

	telemetry_agent_debug("Peer Info: mac " QDF_MAC_ADDR_FMT " mld mac " QDF_MAC_ADDR_FMT " bw:%d freq:%d",
			      QDF_MAC_ADDR_REF(stats->peer_link_mac), QDF_MAC_ADDR_REF(stats->peer_mld_mac),
			      stats->bw, stats->freq);
#endif

	stats->is_assoc_link = is_assoc_link;
	if (ni->ext_caps.ni_ext_capabilities &
			IEEE80211_EXTCAPIE_BSSTRANSITION) {
		SET_PEER_FLAG(stats->peer_flags, BTM_CAP);
	}

#ifdef CONFIG_SAWF
	cdp_get_peer_sawf_msduq_svc_params(psoc->dp_handle,
					   peer->macaddr, msduq_params);

	for (index = 0; index < SAWF_MAX_QUEUES; index++) {
		stats->msduq_info[index].is_used = msduq_params[index].is_used;
		stats->msduq_info[index].svc_id = msduq_params[index].svc_id;
		stats->msduq_info[index].svc_type = msduq_params[index].svc_type;
		stats->msduq_info[index].svc_tid = msduq_params[index].svc_tid;
		stats->msduq_info[index].svc_ac = msduq_params[index].svc_ac;
		stats->msduq_info[index].priority = msduq_params[index].priority;
		stats->msduq_info[index].service_interval = msduq_params[index].service_interval;
		stats->msduq_info[index].burst_size = msduq_params[index].burst_size;
		stats->msduq_info[index].min_throughput = msduq_params[index].min_throughput;
		stats->msduq_info[index].delay_bound = msduq_params[index].delay_bound;
		stats->msduq_info[index].mark_metadata = msduq_params[index].mark_metadata;
	}
#endif

	qdf_trace_hex_dump(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
			   stats->caps.tx_mcs_nss_map,
			   sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
	qdf_trace_hex_dump(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
			   stats->caps.rx_mcs_nss_map,
			   sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
	return 0;
}

qdf_export_symbol(wifi_driver_get_peer_info);

int wifi_driver_get_psoc_stats(void *obj)
{
	/* Currently used to check whether psoc is mlo enabled or not
	 * Can be enhanced to fill more  psoc stats if required
	 */
	struct wlan_objmgr_psoc *psoc = (struct wlan_objmgr_psoc *)obj;
	uint8_t ml_grp_id = 0;

	if (!psoc)
		return -1;

	if (!mlo_psoc_get_grp_id(psoc, &ml_grp_id)) {
		telemetry_agent_debug("Invalid MLO Group id");
		return -1;
	}

	return 0;
}

qdf_export_symbol(wifi_driver_get_psoc_stats);

int wifi_driver_get_pdev_stats(void *obj, struct agent_link_iface_stats_obj *stats)
{
	struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)obj;
	ol_txrx_soc_handle soc_txrx_handle;
	struct ieee80211com *ic = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
#ifdef QCA_SUPPORT_CP_STATS
	struct pdev_ic_cp_stats *pdev_cps = NULL;
#endif
	struct cdp_pdev_telemetry_stats dpstats = {0};
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	uint8_t i = 0;
	uint8_t ml_grp_id = 0;
	uint8_t pdev_id;

	if (!pdev)
		return -1;

#ifdef QCA_SUPPORT_CP_STATS
	pdev_cps = wlan_get_pdev_cp_stats_ref(pdev);
	if (!pdev_cps) {
		telemetry_agent_err("Failed to get pdev cp stats reference");
		return -1;
	}
#endif
	psoc = wlan_pdev_get_psoc(pdev);
	if (!mlo_psoc_get_grp_id(psoc, &ml_grp_id)) {
		telemetry_agent_debug("Invalid MLO Group id");
		return -1;
	}

	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic)
		return -1;

	if (ic->ic_curchan == NULL) {
		telemetry_agent_err("%s: ic->ic_curchan is NULL!", __func__);
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct agent_link_iface_stats_obj));
	if (ic->ic_mon_vap ||
			cdp_is_lite_mon_enabled(soc_txrx_handle, pdev_id,
				LITE_MON_DIRECTION_RX) ||
			cdp_is_lite_mon_enabled(soc_txrx_handle, pdev_id,
				LITE_MON_DIRECTION_TX)) {
		stats->is_mon_enabled = true;
	} else {
		stats->is_mon_enabled = false;
	}

	stats->link_id = wlan_mlo_get_pdev_hw_link_id(pdev);
	stats->link_id = (ml_grp_id << 8) | stats->link_id;
	stats->soc_id = wlan_psoc_get_id(psoc);
	stats->freq = ic->ic_curchan->ic_freq;

	stats->available_airtime[0] = pdev_cps->stats.telemetry_stats.estimated_air_time_ac_be;
	stats->available_airtime[1] = pdev_cps->stats.telemetry_stats.estimated_air_time_ac_bk;
	stats->available_airtime[2] = pdev_cps->stats.telemetry_stats.estimated_air_time_ac_vi;
	stats->available_airtime[3] = pdev_cps->stats.telemetry_stats.estimated_air_time_ac_vo;

	status = cdp_get_pdev_telemetry_stats(psoc->dp_handle,
					      wlan_objmgr_pdev_get_pdev_id(pdev),
					      &dpstats);
	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_debug("Failed to get dp stats");
		return -1;
	}

	stats->congestion[WME_AC_BE] =
		(pdev_cps->stats.avg_chan_lat_per_ac[WIFI_AC_BE] *100)/1000000;
	stats->congestion[WME_AC_BK] =
		(pdev_cps->stats.avg_chan_lat_per_ac[WIFI_AC_BK] *100)/1000000;
	stats->congestion[WME_AC_VI] =
		(pdev_cps->stats.avg_chan_lat_per_ac[WIFI_AC_VI] *100)/1000000;
	stats->congestion[WME_AC_VO] =
		(pdev_cps->stats.avg_chan_lat_per_ac[WIFI_AC_VO] *100)/1000000;

	for (i = 0; i < WME_AC_MAX; i++) {
		stats->tx_mpdu_failed[i] = dpstats.tx_mpdu_failed[i];
		stats->tx_mpdu_total[i] = dpstats.tx_mpdu_total[i];
		stats->traffic_condition[i] =
			pdev_cps->stats.telemetry_stats.traffic_condition_used_per_ac[i];
		stats->error_margin[i] =
			pdev_cps->stats.telemetry_stats.error_margin_per_ac[i];
		stats->num_dl_asymmetric_clients[i] =
			pdev_cps->stats.telemetry_stats.num_of_dl_asymmetric_clients_per_ac[i];
		stats->num_ul_asymmetric_clients[i] =
			pdev_cps->stats.telemetry_stats.num_of_ul_asymmetric_clients_per_ac[i];
		stats->avg_chan_latency[i] =
			pdev_cps->stats.telemetry_stats.avg_chan_lat_per_ac[i];
	}

	stats->freetime = pdev_cps->stats.pdev_freetime_per_sec;
	stats->obss_airtime = pdev_cps->stats.telemetry_stats.link_obss_airtime;

	stats->dl_payload_ratio[WME_AC_BE] = pdev_cps->stats.telemetry_stats.payload_ratio_dl_ac_be;
	stats->dl_payload_ratio[WME_AC_BK] = pdev_cps->stats.telemetry_stats.payload_ratio_dl_ac_bk;
	stats->dl_payload_ratio[WME_AC_VI] = pdev_cps->stats.telemetry_stats.payload_ratio_dl_ac_vi;
	stats->dl_payload_ratio[WME_AC_VO] = pdev_cps->stats.telemetry_stats.payload_ratio_dl_ac_vo;

	stats->ul_payload_ratio[WME_AC_BE] = pdev_cps->stats.telemetry_stats.payload_ratio_ul_ac_be;
	stats->ul_payload_ratio[WME_AC_BK] = pdev_cps->stats.telemetry_stats.payload_ratio_ul_ac_bk;
	stats->ul_payload_ratio[WME_AC_VI] = pdev_cps->stats.telemetry_stats.payload_ratio_ul_ac_vi;
	stats->ul_payload_ratio[WME_AC_VO] = pdev_cps->stats.telemetry_stats.payload_ratio_ul_ac_vo;

	for (i = 0; i < WME_AC_MAX; i++)
		stats->link_airtime[i] = dpstats.link_airtime[i];
	telemetry_agent_debug("Pdev Stats: link_id:%d soc_id:%d freetime:%d obss_airtime:%u",
			      stats->link_id, stats->soc_id, stats->freetime, stats->obss_airtime);
	for (i = 0; i < WME_AC_MAX; i++) {
		telemetry_agent_debug("AC:%d Available Airtime:%d: Link_airtime:%d Congestion:%d "
				      "tx_mpdu_failed:%d tx_mpdu_total:%d",
				      i, stats->available_airtime[i], stats->link_airtime[i],
				      stats->congestion[i], stats->tx_mpdu_failed[i],
				      stats->tx_mpdu_total[i]);
		telemetry_agent_debug("AC:%d traffic_condition:%u error_margin:%u "
				      "num_dl_asymmetric_clients:%u num_ul_asymmetric_clients:%u "
				      "dl_payload_ratio:%u ul_payload_ratio:%u "
				      "avg_chan_latency:%u",
				      i, stats->traffic_condition[i], stats->error_margin[i],
				      stats->num_dl_asymmetric_clients[i],
				      stats->num_ul_asymmetric_clients[i],
				      stats->dl_payload_ratio[i], stats->ul_payload_ratio[i],
				      stats->avg_chan_latency[i]);
	}
	return 0;
}

qdf_export_symbol(wifi_driver_get_pdev_stats);

int wlan_mlme_get_effective_channel_bandwidth(struct ieee80211_node *ni)
{
	uint16_t cwm_width;
	uint16_t eff_chan_bandwidth;
	uint8_t puncture_chan_count = 0;
	uint16_t puncture_bmap;

	switch(ni->ni_chwidth) {
		case IEEE80211_CWM_WIDTH20:
			cwm_width = 20;
			break;
		case IEEE80211_CWM_WIDTH40:
			cwm_width = 40;
			break;
		case IEEE80211_CWM_WIDTH80:
			cwm_width = 80;
			break;
		case IEEE80211_CWM_WIDTH160:
		case IEEE80211_CWM_WIDTH80_80:
			cwm_width = 160;
			break;
#ifdef WLAN_FEATURE_11BE
		case IEEE80211_CWM_WIDTH320:
			cwm_width = 320;
			break;
#endif
		 default:
			return -1;
	}
	/* Calculating effective channel bandwidth using punctured bandwidth */
	puncture_bmap = ni->ni_ch_param.puncture_bitmap;

	while (puncture_bmap && puncture_bmap != 0xffffffff) {
		puncture_bmap &= (puncture_bmap - 1);
		puncture_chan_count++;
	}
	eff_chan_bandwidth = cwm_width - (puncture_chan_count * 20);

	return eff_chan_bandwidth;
}

int wifi_driver_get_peer_stats(void *obj, struct agent_peer_iface_stats_obj *stats)
{
	struct agent_peer_db *peer_db = (struct agent_peer_db *)obj;
	struct wlan_objmgr_peer *peer = peer_db->peer_obj_ptr;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct ieee80211_node *ni = NULL;
#ifdef WLAN_FEATURE_11BE_MLO
	struct wlan_mlo_peer_context *ml_peer = NULL;
	struct wlan_objmgr_peer *active_peer = NULL;
	struct wlan_t2lm_info *ml_peer_t2lm_info = NULL;
	uint8_t dir = 0;
	uint8_t i;
#endif
	uint16_t eff_chan_bandwidth;
	bool is_assoc = false;
	struct cdp_peer_telemetry_stats dpstats = {0};
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	uint8_t ac = 0;

	if (!peer)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_RTT_PASN)
		return -1;

	psoc = wlan_peer_get_psoc(peer);
	if (!psoc)
		return -1;

	pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer));
	if (!pdev)
		return -1;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -1;

	qdf_mem_zero(stats, sizeof(struct agent_peer_iface_stats_obj));

#ifdef WLAN_FEATURE_11BE_MLO
	if (wlan_peer_is_mlo(peer)) {
		ml_peer = peer->mlo_peer_ctx;
		if (!ml_peer)
			return -1;

		qdf_mem_copy(stats->peer_mld_mac, peer->mldaddr, QDF_MAC_ADDR_SIZE);

		if (ml_peer->t2lm_policy.t2lm_enable_val == WLAN_T2LM_NOT_SUPPORTED) {
			active_peer = peer;
		} else {

			for (dir = 0 ; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
				if ((ml_peer->t2lm_policy.t2lm_negotiated_info.t2lm_info[dir].direction) !=
						WLAN_T2LM_INVALID_DIRECTION) {
					ml_peer_t2lm_info =
						&ml_peer->t2lm_policy.t2lm_negotiated_info.t2lm_info[dir];

					if (ml_peer_t2lm_info->default_link_mapping) {
						active_peer = peer;
						break;
					} else {
						for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {

							if ((ml_peer_t2lm_info->hw_link_map_tid[0])
									& (1 << ml_peer->peer_list[i].hw_link_id) ) {
								active_peer = ml_peer->peer_list[i].link_peer;
								if (active_peer != NULL) {
									break;
								}
							}
						}
						if (active_peer != NULL) {
							break;
						}
					}
				}
			}

			if (!active_peer) {
				telemetry_agent_err("Error!, active_peer is NULL");
				return -1;
			}
		}
	} else {
		active_peer = peer;
	}
#endif
	qdf_mem_copy(stats->peer_link_mac, peer->macaddr, QDF_MAC_ADDR_SIZE);
	status = cdp_get_peer_telemetry_stats(psoc->dp_handle,
					      peer->macaddr,
					      &dpstats);
	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_debug("Failed to get dp stats");
		return -1;
	}

	for (ac = 0; ac < WME_AC_MAX; ac++) {
		stats->airtime_consumption[ac] = (uint8_t)(dpstats.tx_airtime_consumption[ac] +
				dpstats.rx_airtime_consumption[ac]);
	}
	stats->tx_mpdu_retried = dpstats.tx_mpdu_retried;
	stats->tx_mpdu_total = dpstats.tx_mpdu_total;
	stats->rx_mpdu_retried = dpstats.rx_mpdu_retried;
	stats->rx_mpdu_total = dpstats.rx_mpdu_total;
	stats->rssi = wlan_mlme_calculate_link_rssi(ni, active_peer);
	eff_chan_bandwidth = wlan_mlme_get_effective_channel_bandwidth(ni);
	if (eff_chan_bandwidth == -1) {
		telemetry_agent_err("Failed to get effective channel bandwidth");
		return -1;
	}
	/* Detect changes in BW and NSS parameters */
	if ((peer_db->rx_mcs_nss_map[0] !=  ni->ni_eht.ehtcap_rxmcsnssmap[0]) ||
			(peer_db->tx_mcs_nss_map[0] !=  ni->ni_eht.ehtcap_txmcsnssmap[0]) ||
			(peer_db->chan_bw !=  ni->ni_chwidth) ||
			(peer_db->eff_chan_bw != eff_chan_bandwidth)) {
		telemetry_agent_debug("Changes detected in STA BW/NSS values:\n"
				      "prev_chwidth = %d curr_chwidth = %d\n"
				      "prev_rxstreams = %x curr_rxstreams = %x\n"
				      "prev_ni_txstreams = %x curr_txstreams = %x\n",
				      peer_db->chan_bw, ni->ni_chwidth,
				      peer_db->rx_mcs_nss_map[0], ni->ni_eht.ehtcap_rxmcsnssmap[0],
				      peer_db->tx_mcs_nss_map[0], ni->ni_eht.ehtcap_txmcsnssmap[0]);

		/* Marks it assoc/re-assoc case if tx nss value in data base is zero */
		if (peer_db->tx_mcs_nss_map[0] == 0) {
			is_assoc = true;
		}
		/* Updating prev values to current values */
		peer_db->chan_bw = ni->ni_chwidth;
		peer_db->eff_chan_bw = eff_chan_bandwidth;
		peer_db->rx_mcs_nss_map[0] = ni->ni_eht.ehtcap_rxmcsnssmap[0];
		peer_db->tx_mcs_nss_map[0] = ni->ni_eht.ehtcap_txmcsnssmap[0];

		/* On detecting change in BW/NSS, triggers RM Generic msg to RM, provided its not assoc/reassoc case*/
		if(!is_assoc) {
			telemetry_agent_debug("Initiating RM Generic event for notifying RM about STA BW/NSS changes\n");
			wlan_mlme_send_sta_cap_change(peer, QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO);
		}
	}

#ifdef CONFIG_SAWF
	cdp_swaf_peer_sla_configuration(psoc->dp_handle, peer->macaddr, &stats->sla_mask);
#endif

	telemetry_agent_debug("Peer Stats: mac " QDF_MAC_ADDR_FMT " mld_mac " QDF_MAC_ADDR_FMT " tx_mpdu_retried:%d "
			      "tx_mpdu_total:%d rx_mpdu_retried:%d rx_mpdu_total:%d rssi:%d sla_mask:0x%04X",
			      QDF_MAC_ADDR_REF(stats->peer_link_mac), QDF_MAC_ADDR_REF(stats->peer_mld_mac),
			      stats->tx_mpdu_retried,
			      stats->tx_mpdu_total, stats->rx_mpdu_retried, stats->rx_mpdu_total,
			      stats->rssi, stats->sla_mask);
	for (ac = 0; ac < WME_AC_MAX; ac++)
		telemetry_agent_debug("Airtime for AC %d: %d", ac, stats->airtime_consumption[ac]);
	return 0;
}

qdf_export_symbol(wifi_driver_get_peer_stats);

#ifdef QCA_SUPPORT_CP_STATS
int wifi_driver_get_emesh_pdev_stats(void *obj, struct emesh_link_iface_stats_obj *stats)
{
	struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)obj;
	struct pdev_ic_cp_stats *pdev_cps = NULL;

	if (!pdev)
		return -1;

	pdev_cps = wlan_get_pdev_cp_stats_ref(pdev);
	if (!pdev_cps) {
		telemetry_agent_err("Failed to get pdev cp stats reference");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct emesh_link_iface_stats_obj));
	qdf_mem_copy(stats->link_mac,
		     wlan_pdev_get_hw_macaddr(pdev),
		     QDF_MAC_ADDR_SIZE);
	stats->link_idle_airtime = pdev_cps->stats.telemetry_stats.link_idle_airtime;
	telemetry_agent_debug("link mac: " QDF_MAC_ADDR_FMT " link_idle_airtime: %d",
			      QDF_MAC_ADDR_REF(stats->link_mac), stats->link_idle_airtime);
	return 0;
}

qdf_export_symbol(wifi_driver_get_emesh_pdev_stats);
#endif

int wifi_driver_get_emesh_peer_stats(void *obj, struct emesh_peer_iface_stats_obj *stats)
{
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct cdp_peer_telemetry_stats dpstats = {0};
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	uint8_t ac = 0;

	if (!peer)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_RTT_PASN)
		return -1;

	psoc = wlan_peer_get_psoc(peer);
	if (!psoc)
		return -1;

	status = wlan_objmgr_peer_try_get_ref(peer, WLAN_MLME_HANDLER_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_err("Peer get ref failed");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct emesh_peer_iface_stats_obj));
	qdf_mem_copy(stats->peer_link_mac, peer->macaddr, QDF_MAC_ADDR_SIZE);
	status = cdp_get_peer_telemetry_stats(psoc->dp_handle,
					      peer->macaddr,
					      &dpstats);
	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_err("Failed to get dp stats");
		wlan_objmgr_peer_release_ref(peer, WLAN_MLME_HANDLER_ID);
		return -1;
	}

	telemetry_agent_debug("Peer mac: " QDF_MAC_ADDR_FMT " ",
			QDF_MAC_ADDR_REF(stats->peer_link_mac));
	for (ac = 0; ac < WME_AC_MAX; ac++) {
		stats->tx_airtime_consumption[ac] = dpstats.tx_airtime_consumption[ac];
		telemetry_agent_debug("AC: %d, tx_airtime_consumption: %d",
				ac, stats->tx_airtime_consumption[ac]);
	}

	wlan_objmgr_peer_release_ref(peer, WLAN_MLME_HANDLER_ID);
	return 0;
}

qdf_export_symbol(wifi_driver_get_emesh_peer_stats);

int wifi_driver_get_deter_pdev_stats(void *obj, struct deter_link_iface_stats_obj *stats)
{
	struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)obj;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct cdp_pdev_deter_stats *deter_stats = NULL;
	uint8_t ml_grp_id = 0;
	if (!pdev)
		return -1;

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return -1;

	if (!mlo_psoc_get_grp_id(psoc, &ml_grp_id)) {
		telemetry_agent_debug("Invalid MLO Group id");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct deter_link_iface_stats_obj));

	deter_stats = cdp_get_pdev_stats_deter(psoc->dp_handle,
					  wlan_objmgr_pdev_get_pdev_id(pdev));

	if (!deter_stats) {
		wlan_mlme_err("Failed to get deterministic stats");
		return -1;
	}

	stats->hw_link_id = wlan_mlo_get_pdev_hw_link_id(pdev);
	stats->hw_link_id = (ml_grp_id << 8) | stats->hw_link_id;
	qdf_mem_copy(stats->link_mac,
		     wlan_pdev_get_hw_macaddr(pdev),
		     QDF_MAC_ADDR_SIZE);

	qdf_mem_copy(stats->dl_ofdma_usr,
		     deter_stats->dl_ofdma_usr,
		     sizeof(stats->dl_ofdma_usr));
	qdf_mem_copy(stats->ul_ofdma_usr,
		     deter_stats->ul_ofdma_usr,
		     sizeof(stats->ul_ofdma_usr));
	qdf_mem_copy(stats->dl_mimo_usr,
		     deter_stats->dl_mimo_usr,
		     sizeof(stats->dl_mimo_usr));
	qdf_mem_copy(stats->ul_mimo_usr,
		     deter_stats->ul_mimo_usr,
		     sizeof(stats->ul_mimo_usr));

	qdf_mem_copy(stats->ul_mode_cnt,
		     deter_stats->ul_mode_cnt,
		     sizeof(stats->ul_mode_cnt));
	qdf_mem_copy(stats->dl_mode_cnt,
		     deter_stats->dl_mode_cnt,
		     sizeof(stats->dl_mode_cnt));
	qdf_mem_copy(stats->ch_access_delay,
		     deter_stats->ch_access_delay,
		     sizeof(stats->ch_access_delay));

	qdf_mem_copy(stats->ts,
		     deter_stats->ts,
		     sizeof(stats->ts));

	stats->ch_util.tx_util = deter_stats->ch_util.ap_tx_util;
	stats->ch_util.rx_util = deter_stats->ch_util.ap_rx_util;
	stats->ch_util.chan_util = deter_stats->ch_util.ap_chan_util;

	stats->rx_su_cnt = deter_stats->rx_su_cnt;

	return 0;
}

qdf_export_symbol(wifi_driver_get_deter_pdev_stats);

int wifi_driver_get_deter_peer_stats(void *obj, struct deter_peer_iface_stats_obj *stats)
{
#if 0
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct cdp_peer_deter_stats *deter_stats = NULL;
	struct wlan_objmgr_vdev *vdev = {0};
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	uint8_t inx = 0;

	if (!peer)
		return -1;

	psoc = wlan_peer_get_psoc(peer);
	if (!psoc)
		return -1;

	status = wlan_objmgr_peer_try_get_ref(peer, WLAN_MLME_HANDLER_ID);
	if (QDF_IS_STATUS_ERROR(status)) {
		wlan_mlme_err("Peer get ref failed");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct deter_peer_iface_stats_obj));
	qdf_mem_copy(stats->peer_link_mac, peer->macaddr, QDF_MAC_ADDR_SIZE);
	deter_stats = cdp_get_peer_stats_deter(psoc->dp_handle,
					  CDP_VDEV_ALL,
					  peer->macaddr);

	if (!deter_stats){
		wlan_mlme_err("Failed to get deterministic stats");
		wlan_objmgr_peer_release_ref(peer, WLAN_MLME_HANDLER_ID);
		return -1;
	}

	for (inx = 0; inx < CDP_DATA_TID_MAX; inx++) {
		qdf_mem_copy(stats->deter[inx].dl_det, deter_stats[inx].dl_det,
			     sizeof(stats->deter[inx].dl_det));
		qdf_mem_copy(stats->deter[inx].ul_det, deter_stats[inx].ul_det,
			     sizeof(stats->deter[inx].ul_det));
		qdf_mem_copy(&stats->deter[inx].rx_det, &deter_stats[inx].rx_det,
			     sizeof(stats->deter[inx].rx_det));
	}

	vdev = wlan_peer_get_vdev(peer);
	stats->vdev_id = vdev->vdev_objmgr.vdev_id;

	wlan_objmgr_peer_release_ref(peer, WLAN_MLME_HANDLER_ID);

#endif
	return 0;
}

qdf_export_symbol(wifi_driver_get_deter_peer_stats);
int wifi_driver_get_admctrl_pdev_stats(void *obj, struct admctrl_link_iface_stats_obj *stats)
{
	uint8_t i = 0;
	uint8_t ml_grp_id = 0;
	struct wlan_objmgr_psoc *psoc = NULL;
#ifdef QCA_SUPPORT_CP_STATS
	struct pdev_ic_cp_stats *pdev_cps = NULL;
#endif
	struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)obj;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	struct cdp_pdev_telemetry_stats dpstats = {0};

	if (!pdev)
		return -1;

#ifdef QCA_SUPPORT_CP_STATS
	pdev_cps = wlan_get_pdev_cp_stats_ref(pdev);
	if (!pdev_cps) {
		telemetry_agent_err("Failed to get pdev cp stats reference");
		return -1;
	}
#endif

	psoc = wlan_pdev_get_psoc(pdev);
	if (!mlo_psoc_get_grp_id(psoc, &ml_grp_id)) {
		telemetry_agent_debug("Invalid MLO Group id");
		return -1;
	}

	qdf_mem_zero(stats, sizeof(struct admctrl_link_iface_stats_obj));
	stats->link_id = wlan_mlo_get_pdev_hw_link_id(pdev);
	stats->link_id = (ml_grp_id << 8) | stats->link_id;

	status = cdp_get_pdev_telemetry_stats(psoc->dp_handle,
					      wlan_objmgr_pdev_get_pdev_id(pdev),
					      &dpstats);
	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_debug("Failed to get dp stats");
		return -1;
	}

	for (i = 0; i < WME_AC_MAX; i++) {
		stats->tx_link_airtime[i] = dpstats.tx_link_airtime[i];
		telemetry_agent_debug("AC: %d Link_airtime: %d",
				      i, stats->tx_link_airtime[i]);
	}

	stats->freetime = pdev_cps->stats.pdev_freetime_per_sec;
	telemetry_agent_debug("Pdev Stats: link_id: %d freetime: %d",
			      stats->link_id, stats->freetime);

	return 0;
}

qdf_export_symbol(wifi_driver_get_admctrl_pdev_stats);

int wifi_driver_get_admctrl_peer_stats(void *obj, struct admctrl_peer_iface_stats_obj *stats)
{
	uint8_t is_assoc_link = 1;
	uint8_t vdev_id;
	enum wlan_peer_type peer_type;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)obj;
#ifdef CONFIG_SAWF
	uint8_t q_idx, tid, index, ac;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;
	struct sawf_admctrl_peer_stats sawf_stats = {0};
	struct sawf_admctrl_msduq_stats *msduq_stats = NULL;
#endif

	if (!peer)
		return -1;

	peer_type = wlan_peer_get_peer_type(peer);
	if ((peer_type == WLAN_PEER_MLO_BRIDGE) ||
	    (peer_type == WLAN_PEER_RTT_PASN))
		return -1;

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev)
		return -1;

	psoc = wlan_peer_get_psoc(peer);
	if (!psoc)
		return -1;

	vdev_id = wlan_vdev_get_id(vdev);

	qdf_mem_zero(stats, sizeof(struct admctrl_peer_iface_stats_obj));
	qdf_mem_copy(stats->peer_link_mac, peer->macaddr, QDF_MAC_ADDR_SIZE);

	telemetry_agent_debug("Peer mac: " QDF_MAC_ADDR_FMT,
			QDF_MAC_ADDR_REF(stats->peer_link_mac));

#ifdef WLAN_FEATURE_11BE_MLO
	if (wlan_peer_is_mlo(peer)) {
		struct wlan_mlo_peer_context *ml_peer = peer->mlo_peer_ctx;
		if (!ml_peer)
			return -1;

		qdf_mem_copy(stats->peer_mld_mac, peer->mldaddr, QDF_MAC_ADDR_SIZE);
		telemetry_agent_debug("Peer mld mac: " QDF_MAC_ADDR_FMT,
				QDF_MAC_ADDR_REF(stats->peer_mld_mac));

		if (!wlan_peer_mlme_is_assoc_peer(peer))
			is_assoc_link = 0;
	}
#endif

	stats->is_assoc_link = is_assoc_link;

#ifdef CONFIG_SAWF
	status = cdp_get_peer_sawf_admctrl_stats(psoc->dp_handle, peer->macaddr,
						 &sawf_stats,
						 CDP_LINK_PEER_TYPE);

	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_debug("Failed to get dp stats");
		return -1;
	}

	for (ac = 0; ac < WME_AC_MAX; ac++) {
		stats->tx_airtime_consumption[ac] = sawf_stats.tx_airtime_consumption[ac];
		telemetry_agent_debug("AC: %d, tx_airtime_consumption: %u",
				      ac, stats->tx_airtime_consumption[ac]);
	}

	msduq_stats = sawf_stats.msduq_stats;

	for (tid = 0; tid < DP_SAWF_MAX_TIDS; tid++) {
		for (q_idx = 0; q_idx < DP_SAWF_MAX_QUEUES; q_idx++) {
			index = tid + q_idx * DP_SAWF_MAX_TIDS;
			stats->msduq_stats[index].tx_success_num = msduq_stats->tx_success_pkt;
			telemetry_agent_debug("index: %d, tx_success_pkt: %llu",
					      index, stats->msduq_stats[index].tx_success_num);
			msduq_stats++;
		}
	}

	stats->tx_success_num = sawf_stats.tx_success_pkt;
	stats->avg_tx_rate = sawf_stats.avg_tx_rate;
	telemetry_agent_debug("tx_success_num: %llu, tx_rate: %llu",
			      stats->tx_success_num, stats->avg_tx_rate);

#ifdef WLAN_FEATURE_11BE_MLO
	if (wlan_peer_is_mlo(peer)) {
		qdf_mem_zero(&sawf_stats, sizeof(struct sawf_admctrl_peer_stats));
		status = cdp_get_peer_sawf_admctrl_stats(psoc->dp_handle,
							 peer->mldaddr,
							 &sawf_stats,
							 CDP_MLD_PEER_TYPE);

		if (QDF_IS_STATUS_ERROR(status)) {
			telemetry_agent_err("Failed to get mld peer stats");
			return -1;
		}

		stats->mld_tx_success_num = sawf_stats.tx_success_pkt;
		telemetry_agent_debug("mld_tx_success_num: %llu",
				      stats->mld_tx_success_num);
	}
#endif
#endif

	return 0;
}

qdf_export_symbol(wifi_driver_get_admctrl_peer_stats);

#ifdef WLAN_FEATURE_11BE
/*
 * wlan_vdev_derive_link_rssi() - API to derive link rssi using path loss method
 * @vdev: The vdev for which rssi needs to be derived
 * @assoc_vdev: Assoc vdev pointer
 * @rssi: client rssi on assoc vdev
 * @tx_pow: client tx power
 *
 * Return: derived rssi
 */
static int8_t wlan_vdev_derive_link_rssi(struct wlan_objmgr_vdev *vdev,
					 struct wlan_objmgr_vdev *assoc_vdev,
					 int8_t rssi, uint8_t tx_pow)
{
	struct wlan_channel *channel, *assoc_channel;
	uint16_t ch_freq, assoc_freq;
	uint8_t assoc_tx_pow;
	int8_t diff_txpow;
	struct wlan_objmgr_pdev *pdev, *assoc_pdev;
	uint8_t log10_freq;
	int8_t derived_rssi;
	int16_t ten_derived_rssi;
	int8_t ten_diff_pl = 0;

	pdev = wlan_vdev_get_pdev(vdev);
	assoc_pdev = wlan_vdev_get_pdev(assoc_vdev);

	channel = wlan_vdev_get_active_channel(vdev);
	if (channel)
		ch_freq = channel->ch_freq;
	else
		ch_freq = 1;

	assoc_channel = wlan_vdev_get_active_channel(assoc_vdev);
	if (assoc_channel)
		assoc_freq = assoc_channel->ch_freq;
	else
		assoc_freq = 1;

	/*
	 *  diff of path loss (of two links) = log10(freq1) - log10(freq2)
	 *                       (since distance is constant)
	 *  since log10 is not available, we cameup with approximate ranges
	 */
	log10_freq = (ch_freq * 10) / assoc_freq;
	if ((log10_freq >= 20) && (log10_freq < 30))
		ten_diff_pl = 4;  /* 0.4 *10 */
	else if ((log10_freq >= 11) && (log10_freq < 20))
		ten_diff_pl = 1;  /* 0.1 *10 */
	else if ((log10_freq >= 8) && (log10_freq < 11))
		ten_diff_pl = 0; /* 0 *10 */
	else if ((log10_freq >= 4) && (log10_freq < 8))
		ten_diff_pl = -1; /* -0.1 * 10 */
	else if ((log10_freq >= 1) && (log10_freq < 4))
		ten_diff_pl = -4;  /* -0.4 * 10 */

	assoc_tx_pow = wlan_reg_get_channel_reg_power_for_freq(assoc_pdev,
							       assoc_freq);

	diff_txpow = tx_pow -  assoc_tx_pow;

	ten_derived_rssi = (diff_txpow * 10) - ten_diff_pl + (rssi * 10);
	derived_rssi = ten_derived_rssi / 10;

	return derived_rssi;
}

/*
 * wlan_mlme_calculate_link_rssi() - API to derive link rssi using path loss method
 * @ni: the peer for which rssi needs to be derived
 * @active_peer: active association peer
 *
 * Return: derived rssi
 */
static int8_t wlan_mlme_calculate_link_rssi(struct ieee80211_node *ni,
					    struct wlan_objmgr_peer *active_peer)
{
	struct wlan_objmgr_peer *peer;
	struct wlan_objmgr_vdev *vdev;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct ieee80211vap *vap;
	struct wlan_mlo_peer_context *ml_peer;
	struct wlan_objmgr_vdev *active_vdev;
	uint16_t tx_pow, tx_pow_psd;
	uint8_t client_tx_pow;
	bool is_psd_power;
	struct wlan_channel *channel;
	uint16_t ch_freq;
	uint16_t ctry_code;

	if (!ni)
		return 0;

	peer = ni->peer_obj;
	if (peer == active_peer)
		return ni->ni_snr;

	ml_peer = peer->mlo_peer_ctx;
	if (!ml_peer)
		return 0;

	vdev = wlan_peer_get_vdev(peer);
	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap)
		return 0;

	active_vdev = wlan_peer_get_vdev(active_peer);

	/* if chan is non 6GHZ, then use AP tx power as client tx power
	 * else get client tx power for 6GHZ client using ap power type
	 */
	if (IEEE80211_IS_CHAN_6GHZ(vap->iv_bsschan)) {
		pdev = wlan_vdev_get_pdev(vdev);
		is_psd_power = wlan_reg_is_6g_psd_power(pdev);

		if (is_psd_power) {
			wlan_reg_get_client_power_for_6ghz_ap(
					pdev, REG_SUBORDINATE_CLIENT,
					vap->iv_bsschan->ic_freq, &is_psd_power, &tx_pow,
					&tx_pow_psd);
			wlan_reg_psd_2_eirp(
				pdev,
				tx_pow_psd,
				ieee80211_get_chan_width_from_mode(ni->ni_phymode),
				&tx_pow);
		} else {
			wlan_reg_get_client_power_for_6ghz_ap(
					pdev, REG_DEFAULT_CLIENT,
					vap->iv_bsschan->ic_freq, &is_psd_power, &tx_pow,
					&tx_pow_psd);
		}
		client_tx_pow = tx_pow;
	} else {
			channel = wlan_vdev_get_active_channel(vdev);
			if (channel)
				ch_freq = channel->ch_freq;
			else
				ch_freq = 1;
			pdev = wlan_vdev_get_pdev(vdev);
			client_tx_pow = wlan_reg_get_channel_reg_power_for_freq(
						pdev, ch_freq);
		/* If country is US and freq is between 5150-5250, client tx_pow is 6dBm
		 * less than AP tx power
		 */
		ctry_code = ieee80211_getCurrentCountry(vap->iv_ic);
		if (ctry_code == 840 && ch_freq >= CH_FREQ_5150 && ch_freq <= CH_FREQ_5250)
			client_tx_pow -= 6;
	}
	return (wlan_vdev_derive_link_rssi(vdev, active_vdev,
					   wlan_peer_get_rssi(active_peer),
					   client_tx_pow) - CP_STATS_TGT_NOISE_FLOOR_DBM);
}

void wlan_mlme_fill_hw_link_map_tid(struct ieee80211vap *vap, struct wlan_t2lm_info *t2lm)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
	uint16_t vdev_count = 0;
	uint16_t valid_link_mask = 0;
	int i;

	/* Get the valid hw_link_id map from ML vdev list */
	mlo_get_ml_vdev_list(vap->vdev_obj, &vdev_count, wlan_vdev_list);
	if (!vdev_count) {
		t2lm_err("Number of VDEVs under MLD is reported as 0");
		return;
	}

	for (i = 0; i < vdev_count; i++) {
		vdev = wlan_vdev_list[i];
		if (!vdev) {
			t2lm_err("vdev is null");
			continue;
		}

		pdev = wlan_vdev_get_pdev(vdev);
		if (!pdev) {
			t2lm_err("pdev is null");
			continue;
		}

		valid_link_mask |= BIT(ieee80211_mlo_get_pdev_hw_link_id(pdev));
	}

	if (vdev_count) {
		if (ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
					QDF_ARRAY_SIZE(wlan_vdev_list),
					vdev_count, 0) != QDF_STATUS_SUCCESS) {
			t2lm_err("failed to release mlo vdev ref");
			return;
		}
	}

	for (i = 0; i < T2LM_MAX_NUM_TIDS; i++) {
		t2lm->hw_link_map_tid[i] = valid_link_mask;
	}
}

int wlan_mlme_populate_preferred_link_for_teardown(
		struct wlan_objmgr_peer *peer,
		struct wlan_preferred_links *preference_link)
{
	struct ieee80211_node *ni = NULL;
	struct wlan_t2lm_info t2lm = {0};
	struct wlan_mlo_peer_context *ml_peer;
	struct wlan_link_preference *link_pref;
	uint8_t dir, pref_link, latency = 0;
	uint8_t ml_grp_id = 0;
	uint8_t tid = 0;
	uint16_t hw_link_id = 0;
	enum wlan_link_band_caps link_type = 0;
	enum wlan_link_band_caps link_band_mask = 0;
	struct wlan_objmgr_psoc *psoc;
	struct wlan_objmgr_pdev *pdev;
	uint8_t link_id;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -EINVAL;

	psoc = wlan_vdev_get_psoc(ni->ni_vap->vdev_obj);
	if (!psoc) {
		wlan_mlme_err("psoc is NULL");
		return -EINVAL;
	}

	/* Set Preferred link params */
	qdf_mem_copy(preference_link->peer_mld_mac_addr, peer->mldaddr,
			QDF_MAC_ADDR_SIZE);

	/* Set num_t2lm_of_tid = 1 to indicate T2LM info is present in the Preferred link attribute */
	preference_link->num_t2lm_of_tids = 1;

	/* Homogeneous mapping is true for default link mapping */
	preference_link->homogeneous_mapping = true;

	/* Set default mapping only for BIDI direction */
	for(dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
		preference_link->t2lm[dir].direction =
			WLAN_T2LM_INVALID_DIRECTION;
	}

	dir = WLAN_T2LM_BIDI_DIRECTION;
	preference_link->t2lm[dir].direction = WLAN_T2LM_BIDI_DIRECTION;

	/* Fill the hw_link_map_tid for default mapping. */
	wlan_mlme_fill_hw_link_map_tid(ni->ni_vap, &t2lm);

	ml_grp_id = wlan_mlo_get_psoc_group_id(psoc);

	for(tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
		link_band_mask = 0;
		hw_link_id = 0;

		for (link_id = 0; link_id < WLAN_T2LM_MAX_NUM_LINKS; link_id++) {
			hw_link_id = (t2lm.hw_link_map_tid[tid] & BIT(link_id));
			if (!hw_link_id)
				continue;

			pdev = wlan_mlo_get_pdev_by_hw_link_id(link_id, ml_grp_id,
					WLAN_MLO_MGR_ID);
			if (!pdev) {
				continue;
			}

			link_type = wlan_cfg80211_link_band_info(pdev);
			link_band_mask |= BIT((int)link_type);

			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLO_MGR_ID);
			pdev = NULL;
		}
		preference_link->t2lm[dir].t2lm_provisioned_links[tid] =
			link_band_mask;
	}

	/* Fill preferred links and timeout values */
	ml_peer = ni->peer_obj->mlo_peer_ctx;
	if (ml_peer) {
		link_pref = &ml_peer->t2lm_policy.t2lm_negotiated_info.link_preference;

		preference_link->num_pref_links = link_pref->num_pref_links;

		for (pref_link = 0; pref_link < link_pref->num_pref_links; pref_link++) {
			pdev = wlan_mlo_get_pdev_by_hw_link_id(
					link_pref->pref_order[pref_link], ml_grp_id,
					WLAN_MLO_MGR_ID);
			if (!pdev) {
				continue;
			}

			preference_link->preffered_link_order[pref_link] =
				wlan_cfg80211_link_band_info(pdev);

			wlan_objmgr_pdev_release_ref(pdev, WLAN_MLO_MGR_ID);
			pdev = NULL;
		}

		for (latency = 0; latency < WIFI_AC_MAX; latency++) {
			preference_link->timeout[latency] = link_pref->timeout[latency];
		}
	}

	return 0;
}

void wlan_mlme_fill_t2lm_info(struct ieee80211_node *ni,
			      struct wlan_t2lm_onging_negotiation_info *t2lm_info,
			      struct wlan_t2lm_info *ev_t2lm_info,
			      bool is_assoc_frame)
{
	uint8_t i = 0, j = 0;
	uint8_t tid = 0;

	for (i = 0; i < WLAN_T2LM_MAX_DIRECTION; i++)
		ev_t2lm_info[i].direction = WLAN_T2LM_INVALID_DIRECTION;

	for (i = 0; i < QCA_WLAN_VENDOR_T2LM_MAX_VALID_DIRECTION; i++) {
		if (t2lm_info->t2lm_info[i].direction == WLAN_T2LM_INVALID_DIRECTION)
			continue;
		ev_t2lm_info[j].direction = t2lm_info->t2lm_info[i].direction;
		if (is_assoc_frame && ni->ni_use_default_t2lm_map)
			ev_t2lm_info[j].default_link_mapping = 1;
		else
			ev_t2lm_info[j].default_link_mapping =
			t2lm_info->t2lm_info[i].default_link_mapping;
		qdf_mem_copy(ev_t2lm_info[j].ieee_link_map_tid,
				t2lm_info->t2lm_info[i].ieee_link_map_tid,
				sizeof(ev_t2lm_info[i].ieee_link_map_tid));
		t2lm_debug("idx:%d direction:%d default_link_mapping:%d",
			j, ev_t2lm_info[j].direction,
				ev_t2lm_info[j].default_link_mapping);
		for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
			t2lm_debug("tid:%d t2lm_provisioned_links:%x",
					tid, ev_t2lm_info[j].ieee_link_map_tid[tid]);
		}
		j++;
	}
}

int wlan_mlme_fill_mlo_t2lm_info(struct wlan_objmgr_peer *peer,
				 struct qdf_mac_addr *ap_mld_addr,
				 struct mld_peer_link_entry *link_entry,
				 uint8_t ml_grp_id,
				 bool is_assoc)
{
	struct wlan_objmgr_peer *link_peer = NULL;
	struct wlan_objmgr_vdev *link_vdev = NULL;
	struct wlan_mlo_link_peer_entry *mlo_peer = NULL;
	struct wlan_objmgr_pdev *link_pdev = NULL;
	struct ieee80211_node *ni = NULL;
	struct wlan_t2lm_info *ml_peer_t2lm_info = NULL;
#ifdef QCA_SUPPORT_CP_STATS
	struct pdev_ic_cp_stats *pdev_cps = NULL;
#endif
	uint8_t i = 0, j = 0, dir = 0;
	bool bridge_peer_present = false;
	struct wlan_mlo_peer_context *ml_peer = NULL;
	struct wlan_objmgr_peer *active_peer = NULL;
	uint16_t eff_chan_bandwidth;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -1;

	ml_peer = ni->peer_obj->mlo_peer_ctx;
	if (!ml_peer) {
		return -1;
	}

	for (i = 0; i < MAX_MLO_LINK_PEERS; i++)
	{
		mlo_peer = &peer->mlo_peer_ctx->peer_list[i];
		if (!mlo_peer)
			continue;

		link_peer = mlo_peer->link_peer;

		if (!link_peer)
			continue;

		if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE) {
			bridge_peer_present = true;
			continue;
		}

		link_pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(link_peer));
		if (!link_pdev)
			return -1;

#ifdef QCA_SUPPORT_CP_STATS
		pdev_cps = wlan_get_pdev_cp_stats_ref(link_pdev);
		if (!pdev_cps) {
			t2lm_err("Failed to get pdev cp stats reference");
			return -1;
		}
#endif

		ni = wlan_peer_get_mlme_ext_obj(link_peer);
		if (!ni)
			return -1;

		if (ml_peer->t2lm_policy.t2lm_enable_val == WLAN_T2LM_NOT_SUPPORTED)
			return -1;

		if (is_assoc)
			active_peer = ni->peer_obj->mlo_peer_ctx->peer_list[0].link_peer;
		else {
			for (dir = 0 ; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
				if ((ml_peer->t2lm_policy.t2lm_negotiated_info.t2lm_info[dir].direction) !=
						WLAN_T2LM_INVALID_DIRECTION) {
					ml_peer_t2lm_info =
						&ml_peer->t2lm_policy.t2lm_negotiated_info.t2lm_info[dir];

					if (ml_peer_t2lm_info->default_link_mapping) {
						active_peer = link_peer;
						break;
					} else {
						for (j = 0; j < MAX_MLO_LINK_PEERS; j++) {

							if ((ml_peer_t2lm_info->hw_link_map_tid[0]) &
									(1 << ml_peer->peer_list[j].hw_link_id) ) {
								active_peer = ml_peer->peer_list[j].link_peer;
								if (active_peer != NULL) {
									break;
								}
							}
						}
						if (active_peer != NULL) {
							break;
						}
					}
				}
			}
		}

		if (!active_peer) {
			t2lm_err("Error!, active_peer is NULL");
			return -1;
		}

		link_vdev = wlan_peer_get_vdev(link_peer);
		if (!link_vdev) {
			t2lm_err("Error: Link VDEV found NULL while getting vdev_id");
			return -1;
		}

		link_entry->hw_link_id = wlan_mlo_get_pdev_hw_link_id(link_pdev);
		link_entry->hw_link_id = (ml_grp_id << 8 )| link_entry->hw_link_id;
		link_entry->vdev_id = wlan_vdev_get_id(link_vdev);
		qdf_mem_copy(link_entry->ap_mld_addr, ap_mld_addr, QDF_MAC_ADDR_SIZE);
		qdf_mem_copy(link_entry->link_addr, link_peer->macaddr, QDF_MAC_ADDR_SIZE);
		link_entry->ieee_link_id = mlo_peer->link_ix;
		link_entry->is_assoc_link = mlo_peer_is_assoc_peer(ml_peer, link_peer);
		link_entry->chan_freq = ni->ni_ch_param.ni_freq;
		link_entry->link_rssi = wlan_mlme_calculate_link_rssi(ni, active_peer);
		link_entry->chan_bw = ni->ni_chwidth + 1;
		qdf_mem_copy(link_entry->caps.tx_mcs_nss_map, ni->ni_eht.ehtcap_txmcsnssmap, sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		qdf_mem_copy(link_entry->caps.rx_mcs_nss_map, ni->ni_eht.ehtcap_rxmcsnssmap, sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		link_entry->soc_id = wlan_psoc_get_id(wlan_peer_get_psoc(peer));
		eff_chan_bandwidth = wlan_mlme_get_effective_channel_bandwidth(ni);
		if (eff_chan_bandwidth == -1) {
			t2lm_err("Failed to get effective channel bandwidth");
			return -1;
		}
		link_entry->eff_chan_bw = eff_chan_bandwidth;
		if (ni->ext_caps.ni_ext_capabilities &
			IEEE80211_EXTCAPIE_BSSTRANSITION) {
			SET_PEER_FLAG(link_entry->peer_flags, BTM_CAP);
		}

		t2lm_debug("idx:%d hw_link_id:%d link_addr " QDF_MAC_ADDR_FMT " ieee_link_id:%d "
			    "is_assoc_link:%d chan_bw:%d chan_freq:%d link_rssi:%d peer_flags:0x%x",
			    i, link_entry->hw_link_id, QDF_MAC_ADDR_REF(link_entry->link_addr),
			    link_entry->ieee_link_id, link_entry->is_assoc_link, link_entry->chan_bw,
			    link_entry->chan_freq, link_entry->link_rssi,
				link_entry->peer_flags);
		qdf_trace_hex_dump(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
				   link_entry->caps.tx_mcs_nss_map,
				   sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		qdf_trace_hex_dump(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
				   link_entry->caps.rx_mcs_nss_map,
				   sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		link_entry++;
	}

	return 0;
}

int wlan_mlme_send_sta_cap_change(struct wlan_objmgr_peer *peer,
			      enum qca_wlan_vendor_attr_app_generic_category category)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	struct qdf_mac_addr *ap_mld_addr;
	struct ieee80211_node *ni = NULL;
	struct peer_generic_assoc_event ev = {0};
	struct wlan_mlo_peer_context *ml_peer;
	uint8_t ml_grp_id = 0;
	uint8_t vdev_id = 0;

	if (!peer)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE)
		return -1;

	if (!wlan_peer_is_mlo(peer))
		return -1;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -1;

	if (!mlo_psoc_get_grp_id(wlan_peer_get_psoc(peer), &ml_grp_id)) {
		t2lm_err("Invalid MLO Group id");
		return -1;
	}

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev) {
		t2lm_err("Error: VDEV found NULL while getting vdev_id");
		return -1;
	}
	vdev_id = wlan_vdev_get_id(vdev);

	ap_mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);

	/* MLO peer handling */
	qdf_mem_copy(ev.mld_mac_addr, peer->mldaddr, QDF_MAC_ADDR_SIZE);

	ev.category = category;

	ml_peer = ni->peer_obj->mlo_peer_ctx;
	if (!ml_peer) {
		return -1;
	}

	if (wlan_mlme_fill_mlo_t2lm_info(peer, ap_mld_addr, ev.link_entry, ml_grp_id, false))
		return -1;

	ev.num_links = peer->mlo_peer_ctx->link_peer_cnt;
	ev.t2lm_mapping = peer->mlo_peer_ctx->t2lm_policy.t2lm_enable_val;

	/* Call NL API to fill attributes using ev and send event to APP */
	t2lm_debug("MLD MAC" QDF_MAC_ADDR_FMT " category:%d "
		 "num_links:%d t2lm_mapping:%d", QDF_MAC_ADDR_REF(ev.mld_mac_addr),
		 ev.category, ev.num_links, ev.t2lm_mapping);

	wlan_cfg80211_t2lm_app_reply_generic_response(&ev, (uint8_t)QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_OMI_NO_T2LM_INFO, 0);

	return 0;
}

int wlan_mlme_send_t2lm_event(struct wlan_objmgr_peer *peer,
			      struct wlan_t2lm_onging_negotiation_info *t2lm_info,
			      enum qca_wlan_vendor_t2lm_category category,
			      bool is_assoc_frame)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct qdf_mac_addr *ap_mld_addr;
	struct ieee80211_node *ni = NULL;
	struct peer_t2lm_policy_event ev = {0};
	uint8_t i = 0;
	uint8_t tid = 0;
	uint8_t dir, pref_link, latency = 0;
	uint8_t link_id, links = 0;
	uint16_t hw_link_id = 0;
	enum wlan_link_band_caps link_type = 0;
	enum wlan_link_band_caps link_band_mask = 0;
	struct wlan_mlo_peer_context *ml_peer;
	int ret = 0;
	bool bridge_peer_present = false;
	struct wlan_t2lm_onging_negotiation_info *ongoing_t2lm = NULL;
	uint8_t ml_grp_id = 0;
	uint8_t vdev_id = 0;

	if (!peer)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE)
		return -1;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -1;

	if (!mlo_psoc_get_grp_id(wlan_peer_get_psoc(peer), &ml_grp_id)) {
		t2lm_err("Invalid MLO Group id");
		return -1;
	}

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev) {
		t2lm_err("Error: VDEV found NULL while getting vdev_id");
		return -1;
	}
	vdev_id = wlan_vdev_get_id(vdev);

	ap_mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);

	if ((category == QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST) &&
	    t2lm_info == NULL) {
		t2lm_err("Expected t2lm info for T2LM request event");
		return -1;
	}

	ev.category = category;

	/* MLO peer handling */
	qdf_mem_copy(ev.mld_mac_addr, peer->mldaddr, QDF_MAC_ADDR_SIZE);

	if (t2lm_info) {
		wlan_mlme_fill_t2lm_info(ni, t2lm_info, ev.t2lm_info, is_assoc_frame);
		ev.dialog_token = t2lm_info->dialog_token;
	}

		ev.t2lm_mapping = peer->mlo_peer_ctx->t2lm_policy.t2lm_enable_val;
		if (t2lm_info) {
			ev.t2lm_resp_status = t2lm_info->t2lm_resp_type;
			ev.t2lm_tx_status = t2lm_info->t2lm_tx_status;
		}

		t2lm_debug("t2lm_info:%p category:%d t2lm_resp_type:%d",
				t2lm_info, category, t2lm_info?t2lm_info->t2lm_resp_type: 0);

	ml_peer = ni->peer_obj->mlo_peer_ctx;
	if (!ml_peer) {
		return -1;
	}

	ongoing_t2lm = &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping;

	t2lm_debug("ongoing_tid_to_link_mapping.category:%d",
			ongoing_t2lm->category);

	qdf_mem_copy(ev.preference_link.peer_mld_mac_addr, peer->mldaddr,
			QDF_MAC_ADDR_SIZE);

	/* Below if condition will be called when,
	 * 1. When AP reecives T2LM response frame from STA with status code as
	 *    success. (Category is set as T2LM response)
	 * 2. When AP sends the T2LM response frame for the STA as part of receiving
	 *    T2LM request frame. (Category is set as NONE)
	 */
	if (t2lm_info && ml_peer &&
		((category == QCA_WLAN_VENDOR_T2LM_CATEGORY_LINK_QUALIFIER) ||
		 ((category == QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE) &&
		  (t2lm_info->t2lm_resp_type == WLAN_T2LM_RESP_TYPE_SUCCESS)) ||
		 ((category == QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE) &&
		  ((ongoing_t2lm->category == WLAN_T2LM_CATEGORY_RESPONSE) &&
		   (ongoing_t2lm->t2lm_resp_type == WLAN_T2LM_RESP_TYPE_SUCCESS))))) {

		t2lm_info->t2lm_info_present = ongoing_t2lm->t2lm_info_present;
		t2lm_info->pref_link_present = ongoing_t2lm->pref_link_present;

		if (t2lm_info->pref_link_present) {
			qdf_mem_copy(&t2lm_info->link_preference,
					&ongoing_t2lm->link_preference,
					sizeof(struct wlan_link_preference));
		}

		qdf_mem_copy(t2lm_info->t2lm_info, ongoing_t2lm->t2lm_info,
				sizeof(struct wlan_t2lm_info) * WLAN_T2LM_MAX_DIRECTION);


		for (i = 0; i < WLAN_T2LM_MAX_DIRECTION; i++) {
			if (t2lm_info->t2lm_info[i].direction == WLAN_T2LM_INVALID_DIRECTION)
				continue;

			if (t2lm_info->t2lm_info[i].default_link_mapping)
				wlan_mlme_fill_hw_link_map_tid(ni->ni_vap,
						&t2lm_info->t2lm_info[i]);
		}

		t2lm_debug("t2lm_info_present:%d", t2lm_info->t2lm_info_present);
		if (t2lm_info->t2lm_info_present) {
			ev.preference_link.homogeneous_mapping =
				ieee80211_ucfg_is_homogeneous_t2lm(t2lm_info);

			for(dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
				ev.preference_link.t2lm[dir].direction =
					t2lm_info->t2lm_info[dir].direction;

				if (t2lm_info->t2lm_info[dir].direction ==
						WLAN_T2LM_INVALID_DIRECTION)
					continue;

				ev.preference_link.num_t2lm_of_tids++;

				for(tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
					link_id = 0;
					link_band_mask = 0;
					hw_link_id = 0;

					for (link_id = 0; link_id < WLAN_T2LM_MAX_NUM_LINKS; link_id++) {
						hw_link_id =
							(t2lm_info->t2lm_info[dir].hw_link_map_tid[tid] &
							 BIT(link_id));

						if (!hw_link_id)
							continue;

						pdev = wlan_mlo_get_pdev_by_hw_link_id(
								link_id,
								ml_grp_id, WLAN_MLO_MGR_ID);
						if (!pdev) {
							continue;
						}

						link_type = wlan_cfg80211_link_band_info(pdev);
						link_band_mask |= BIT((int)link_type);

						wlan_objmgr_pdev_release_ref(pdev, WLAN_MLO_MGR_ID);
						pdev = NULL;
					}
					ev.preference_link.t2lm[dir].t2lm_provisioned_links[tid]=
						link_band_mask;
				}
			}
		}

		t2lm_debug("pref_link_present:%d", t2lm_info->pref_link_present);
		if (t2lm_info->pref_link_present) {
			ev.preference_link.num_pref_links =
				t2lm_info->link_preference.num_pref_links;
			links = t2lm_info->link_preference.num_pref_links;

			for (pref_link = 0; pref_link < links; pref_link++) {
				pdev = wlan_mlo_get_pdev_by_hw_link_id(
						t2lm_info->link_preference.pref_order[pref_link],
						ml_grp_id, WLAN_MLO_MGR_ID);

				if (!pdev) {
					continue;
				}

				ev.preference_link.preffered_link_order[pref_link] =
					wlan_cfg80211_link_band_info(pdev);

				wlan_objmgr_pdev_release_ref(pdev, WLAN_MLO_MGR_ID);
				pdev = NULL;
			}

			for (latency = 0; latency < WIFI_AC_MAX; latency++) {
				ev.preference_link.timeout[latency] =
					t2lm_info->link_preference.timeout[latency];
			}
		}
	} else if ((category == QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE) &&
			(ongoing_t2lm->category == WLAN_T2LM_CATEGORY_TEARDOWN))  {
		ret = wlan_mlme_populate_preferred_link_for_teardown(peer, &ev.preference_link);
		if (ret)
			return -1;
	}

	if (wlan_mlme_fill_mlo_t2lm_info(peer, ap_mld_addr, ev.link_entry, ml_grp_id, false))
		return -1;

	/* exclude bridge peer */
	if (bridge_peer_present && peer->mlo_peer_ctx->link_peer_cnt)
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt - 1;
	else
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt;

	/* Call NL API to fill attributes using ev and send event to APP */
	t2lm_debug("MLD MAC" QDF_MAC_ADDR_FMT " category:%d "
		   "num_links:%d t2lm_tx_status:%d t2lm_mapping:%d t2lm_resp_status:%d",
		   QDF_MAC_ADDR_REF(ev.mld_mac_addr), ev.category,
		   ev.num_links, ev.t2lm_tx_status, ev.t2lm_mapping,
		   ev.t2lm_resp_status);

	wlan_cfg80211_send_peer_t2lm_event(&ev, peer);
	return 0;
}

int wlan_mlme_send_assoc_event(struct wlan_objmgr_peer *peer,
				bool is_assoc_frame, 
				struct wlan_t2lm_onging_negotiation_info *t2lm_info,
				enum qca_wlan_vendor_attr_app_generic_category category)
{
	struct mld_peer_link_entry *link_entry;
	struct wlan_objmgr_pdev *link_pdev = NULL;
	struct wlan_objmgr_vdev *vdev = NULL;
	struct qdf_mac_addr *ap_mld_addr;
	struct ieee80211_node *ni = NULL;
	struct peer_generic_assoc_event ev = {0};
	bool bridge_peer_present = false;
	uint8_t ml_grp_id = 0;
	uint8_t vdev_id = 0;
	uint16_t eff_chan_bandwidth;

	if (!peer)
		return -1;

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE)
		return -1;

	ni = wlan_peer_get_mlme_ext_obj(peer);
	if (!ni)
		return -1;

	if (!mlo_psoc_get_grp_id(wlan_peer_get_psoc(peer), &ml_grp_id)) {
		t2lm_debug("Invalid MLO Group id");
		return -1;
	}

	vdev = wlan_peer_get_vdev(peer);
	if (!vdev) {
		t2lm_err("Error: VDEV found NULL while getting vdev_id");
		return -1;
	}
	vdev_id = wlan_vdev_get_id(vdev);

	ap_mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vdev);

	if (category == QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_WITH_T2LM_INFO &&
	    t2lm_info == NULL) {
		t2lm_err("Expected t2lm info for T2LM request event");
		return -1;
	}

	ev.category = category;

	if (!wlan_peer_is_mlo(peer)) {
		/* Legacy peer assoc event */
		if (category != QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO) {
			t2lm_err("Invalid Category for Legacy Assoc");
			return -1;
		}

		link_pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer));
		if (!link_pdev)
			return -1;

		link_entry = &ev.link_entry[0];
		link_entry->hw_link_id = wlan_mlo_get_pdev_hw_link_id(link_pdev);
		link_entry->hw_link_id = (ml_grp_id << 8 )| link_entry->hw_link_id;
		link_entry->vdev_id = vdev_id;
		qdf_mem_copy(link_entry->ap_mld_addr, ap_mld_addr, QDF_MAC_ADDR_SIZE);
		qdf_mem_copy(link_entry->link_addr, peer->macaddr, QDF_MAC_ADDR_SIZE);
		link_entry->is_assoc_link = 1;
		link_entry->chan_bw = ni->ni_chwidth + 1;
		link_entry->chan_freq = ni->ni_ch_param.ni_freq;
		link_entry->link_rssi = ni->ni_snr;
		eff_chan_bandwidth = wlan_mlme_get_effective_channel_bandwidth(ni);
		if (eff_chan_bandwidth == -1) {
			t2lm_err("Failed to get effective channel bandwidth");
			return -1;
		}
		link_entry->eff_chan_bw = eff_chan_bandwidth;

		qdf_mem_copy(link_entry->caps.tx_mcs_nss_map, ni->ni_eht.ehtcap_txmcsnssmap,
			     sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		qdf_mem_copy(link_entry->caps.rx_mcs_nss_map, ni->ni_eht.ehtcap_rxmcsnssmap,
			     sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		link_entry->soc_id = wlan_psoc_get_id(wlan_peer_get_psoc(peer));
		t2lm_debug("hw_link_id:%d link_addr " QDF_MAC_ADDR_FMT " ieee_link_id:%d "
			   "is_assoc_link:%d chan_bw:%d chan_freq:%d link_rssi:%d",
			   link_entry->hw_link_id, QDF_MAC_ADDR_REF(link_entry->link_addr),
			   link_entry->ieee_link_id, link_entry->is_assoc_link, link_entry->chan_bw,
			   link_entry->chan_freq, link_entry->link_rssi);
		qdf_trace_hex_dump(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
				   link_entry->caps.tx_mcs_nss_map,
				   sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		qdf_trace_hex_dump(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
				   link_entry->caps.rx_mcs_nss_map,
				   sizeof(uint32_t)*EHTHANDLE_CAP_TXRX_MCS_NSS_SIZE);
		ev.num_links = 1;

		/* Call NL API to fill attributes using ev and send event to APP */
		t2lm_debug("Legacy Peer MAC" QDF_MAC_ADDR_FMT " category:%d ",
			   QDF_MAC_ADDR_REF(peer->macaddr), ev.category);
		wlan_cfg80211_t2lm_app_reply_generic_response(&ev, (uint8_t)QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO, 0);
		return 0;
	}

	/* MLO peer handling */
	qdf_mem_copy(ev.mld_mac_addr, peer->mldaddr, QDF_MAC_ADDR_SIZE);
	if (t2lm_info) {
		wlan_mlme_fill_t2lm_info(ni, t2lm_info, ev.t2lm_info, is_assoc_frame);
	}

	ev.t2lm_mapping = peer->mlo_peer_ctx->t2lm_policy.t2lm_enable_val;


	if (wlan_mlme_fill_mlo_t2lm_info(peer, ap_mld_addr,
					 ev.link_entry, ml_grp_id, is_assoc_frame))
		return -1;

	/* exclude bridge peer */
	if (bridge_peer_present && peer->mlo_peer_ctx->link_peer_cnt)
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt - 1;
	else
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt;

	/* Call NL API to fill attributes using ev and send event to APP */
	t2lm_debug("MLD MAC" QDF_MAC_ADDR_FMT " category:%d "
		   "num_links:%d t2lm_mapping:%d ",
		   QDF_MAC_ADDR_REF(ev.mld_mac_addr), ev.category,
		   ev.num_links, ev.t2lm_mapping);

	wlan_cfg80211_t2lm_app_reply_generic_response(&ev, (uint8_t)category, 0);
	return 0;
}

void wlan_mlme_send_disassoc_event(struct wlan_objmgr_peer *peer,
				   enum qca_wlan_vendor_attr_app_generic_category category)
{
	struct peer_generic_disassoc_event ev = {0};
	struct mld_peer_link_entry *link_entry;
	struct wlan_objmgr_pdev *link_pdev = NULL;
	struct wlan_objmgr_peer *link_peer = NULL;
	struct wlan_mlo_link_peer_entry *mlo_peer = NULL;
	uint8_t i = 0;
	uint8_t idx = 0;
	bool bridge_peer_present = false;
	uint8_t ml_grp_id = 0;

	if (!peer)
		return;

	if (!mlo_psoc_get_grp_id(wlan_peer_get_psoc(peer), &ml_grp_id)) {
		t2lm_debug("Invalid MLO Group id");
		return;
	}

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE) {
		t2lm_debug("Peer( " QDF_MAC_ADDR_FMT ") is MLO Bridge. Skip disassoc event",
			   QDF_MAC_ADDR_REF(peer->macaddr));
		return;
	}

	if (!wlan_peer_is_mlo(peer)) {
		/* Legacy peer disassoc event */
		ev.category = category;
		ev.num_links = 1;
		link_pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer));
		if (!link_pdev)
			return;
		t2lm_debug("Peer( " QDF_MAC_ADDR_FMT ") is Legacy. Send disassoc",
			   QDF_MAC_ADDR_REF(peer->macaddr));
		link_entry = &ev.peer_entry[idx];
		link_entry->hw_link_id = wlan_mlo_get_pdev_hw_link_id(link_pdev);
		link_entry->hw_link_id = (ml_grp_id << 8 )| link_entry->hw_link_id;
		qdf_mem_copy(link_entry->link_addr, &peer->macaddr[0], QDF_MAC_ADDR_SIZE);
		link_entry->is_assoc_link = 1;

		wlan_cfg80211_t2lm_app_reply_generic_response(&ev, (uint8_t)category, 0);
		return;
	}

	/* MLO peer disassoc event */
	if (!peer->mlo_peer_ctx) {
		t2lm_debug("Peer( " QDF_MAC_ADDR_FMT ") is not an MLO",
			   QDF_MAC_ADDR_REF(peer->macaddr));
		return;
	}

	qdf_mem_copy(ev.mld_mac_addr, peer->mldaddr, QDF_MAC_ADDR_SIZE);
	ev.category = category;

	for (i = 0; i < MAX_MLO_LINK_PEERS; i++)
	{
		mlo_peer = &peer->mlo_peer_ctx->peer_list[i];
		if (!mlo_peer)
			continue;

		link_peer = mlo_peer->link_peer;

		if (!link_peer)
			continue;

		if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE) {
			bridge_peer_present = true;
			continue;
		}

		link_pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(link_peer));
		if (!link_pdev)
			return;

		link_entry = &ev.peer_entry[idx++];
		link_entry->hw_link_id = wlan_mlo_get_pdev_hw_link_id(link_pdev);
		link_entry->hw_link_id = (ml_grp_id << 8 )| link_entry->hw_link_id;
		qdf_mem_copy(link_entry->link_addr, link_peer->macaddr, QDF_MAC_ADDR_SIZE);
		link_entry->is_assoc_link = !i;
	}

	/* exclude bridge peer */
	if (bridge_peer_present && peer->mlo_peer_ctx->link_peer_cnt)
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt - 1;
	else
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt;
	wlan_cfg80211_t2lm_app_reply_generic_response(&ev, (uint8_t)category, 0);
	return;
}

void wlan_mlme_send_teardown_event(struct wlan_objmgr_peer *peer,
				   enum qca_wlan_vendor_t2lm_category category)
{
	struct peer_t2lm_tear_down ev = {0};
	struct mld_peer_link_entry *link_entry;
	struct wlan_objmgr_pdev *link_pdev = NULL;
	struct wlan_objmgr_peer *link_peer = NULL;
	struct wlan_mlo_link_peer_entry *mlo_peer = NULL;
	uint8_t i = 0;
	uint8_t idx = 0;
	bool bridge_peer_present = false;
	int ret = 0;
	uint8_t ml_grp_id = 0;

	if (!wlan_peer_is_mlo(peer) || !peer->mlo_peer_ctx) {
		t2lm_debug("Peer( " QDF_MAC_ADDR_FMT ") is not an MLO. Skip teardown event",
			   QDF_MAC_ADDR_REF(peer->macaddr));
		return;
	}

	if (wlan_peer_get_peer_type(peer) == WLAN_PEER_MLO_BRIDGE) {
		t2lm_debug("Peer( " QDF_MAC_ADDR_FMT ") is MLO Bridge. Skip teardown event",
			   QDF_MAC_ADDR_REF(peer->macaddr));
		return;
	}

	if (!mlo_psoc_get_grp_id(wlan_peer_get_psoc(peer), &ml_grp_id)) {
		t2lm_err("Invalid MLO Group id");
		return;
	}

	qdf_mem_copy(ev.mld_mac_addr, peer->mldaddr, QDF_MAC_ADDR_SIZE);
	ev.category = category;

	for (i = 0; i < MAX_MLO_LINK_PEERS; i++)
	{
		mlo_peer = &peer->mlo_peer_ctx->peer_list[i];
		if (!mlo_peer)
			continue;

		link_peer = mlo_peer->link_peer;

		if (!link_peer)
			continue;

		if (wlan_peer_get_peer_type(link_peer) == WLAN_PEER_MLO_BRIDGE) {
			bridge_peer_present = true;
			continue;
		}

		link_pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(link_peer));
		if (!link_pdev)
			return;

		link_entry = &ev.peer_entry[idx++];
		link_entry->hw_link_id = wlan_mlo_get_pdev_hw_link_id(link_pdev);
		link_entry->hw_link_id = (ml_grp_id << 8) | link_entry->hw_link_id;
		qdf_mem_copy(link_entry->link_addr, link_peer->macaddr, QDF_MAC_ADDR_SIZE);
		link_entry->is_assoc_link = !i;
	}

	/* exclude bridge peer */
	if (bridge_peer_present && peer->mlo_peer_ctx->link_peer_cnt)
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt - 1;
	else
		ev.num_links = peer->mlo_peer_ctx->link_peer_cnt;

	ret = wlan_mlme_populate_preferred_link_for_teardown(peer,
		&ev.preference_link);
	if (ret)
		return;

	wlan_cfg80211_send_t2lm_teardown_event(&ev, peer);
}

int wlan_mlme_assoc_t2lm_params(struct wlan_objmgr_peer *peer,
				bool is_assoc_frame,
				struct wlan_t2lm_onging_negotiation_info *t2lm_info,
				enum qca_wlan_vendor_t2lm_category category)
{
	if (!wlan_mlme_is_rm_app_enabled())
		return -1;

	t2lm_debug("Sending event to RM APP for peer " QDF_MAC_ADDR_FMT " category:%d",
			QDF_MAC_ADDR_REF(peer->macaddr), category);

	switch (category) {
	case QCA_WLAN_VENDOR_T2LM_CATEGORY_REQUEST:
		/* Fallthrough */
	case QCA_WLAN_VENDOR_T2LM_CATEGORY_RESPONSE:
		/* Fallthrough */
	case QCA_WLAN_VENDOR_T2LM_CATEGORY_LINK_QUALIFIER:
		/* Fallthrough */
	case QCA_WLAN_VENDOR_T2LM_CATEGORY_NONE:
		wlan_mlme_send_t2lm_event(peer, t2lm_info,
					  category, is_assoc_frame);
		break;
	case QCA_WLAN_VENDOR_T2LM_CATEGORY_TEARDOWN:
		wlan_mlme_send_teardown_event(peer, category);
		break;
	default:
		break;
	}

	return 0;
}

bool wlan_mlme_is_rm_app_enabled(void)
{
	if (ol_global_soc[0] && ol_global_soc[0]->rm_app_init_done)
		return true;

	return false;
}

bool wlan_mlme_is_rm_admctrl_service_enabled(void)
{
       if (ol_global_soc[0] && ol_global_soc[0]->rm_app_init_done &&
           ol_global_soc[0]->rm_admctrl_service_enabled)
               return true;

       return false;
}

bool wlan_mlme_is_rm_energy_service_enabled(void) {
       if (ol_global_soc[0] && ol_global_soc[0]->rm_app_init_done &&
           ol_global_soc[0]->rm_energy_service_enabled)
               return true;

       return false;
}
#endif /* WLAN_FEATURE_11BE */

int wifi_driver_get_erp_pdev_stats(void *obj, struct erp_link_iface_stats_obj *stats)
{
	struct wlan_objmgr_pdev *pdev = (struct wlan_objmgr_pdev *)obj;
	struct ieee80211com *ic = NULL;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct cdp_pdev_erp_stats erp_dp_stats = {0};
	uint64_t current_ts = 0;
	QDF_STATUS status = QDF_STATUS_E_FAILURE;

	if (!pdev)
		return -1;

	/* if in stand by mode return */
	if (pdev->standby_active)
		return -1;

	psoc = wlan_pdev_get_psoc(pdev);
	if (!psoc)
		return -1;

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic)
		return -1;

	qdf_mem_zero(stats, sizeof(struct erp_link_iface_stats_obj));
	status = cdp_get_pdev_erp_stats(psoc->dp_handle,
					wlan_objmgr_pdev_get_pdev_id(pdev),
					&erp_dp_stats);
	if (QDF_IS_STATUS_ERROR(status)) {
		telemetry_agent_err("Failed to get erp dp stats");
		return -1;
	}

	if (ic->ic_latest_assoc_ts) {
		current_ts = qdf_get_log_timestamp_usecs();
		stats->time_since_last_assoc = current_ts -
						ic->ic_latest_assoc_ts;
	}
	stats->sta_vap_exist = (ic->ic_sta_vap) ? true : false;
	stats->tx_data_msdu_cnt = erp_dp_stats.tx_data_msdu_cnt;
	stats->rx_data_msdu_cnt = erp_dp_stats.rx_data_msdu_cnt;
	stats->total_tx_data_bytes = erp_dp_stats.total_tx_data_bytes;
	stats->total_rx_data_bytes = erp_dp_stats.total_rx_data_bytes;

	return 0;
}

qdf_export_symbol(wifi_driver_get_erp_pdev_stats);
#endif /* WLAN_CONFIG_TELEMETRY_AGENT */

QDF_STATUS wlan_mlme_check_mbss_mixed_modes_config(struct ieee80211vap *vap)
{
    if (!vap || !vap->iv_ic) {
        qdf_err("VAP or IC is NULL");
        return QDF_STATUS_E_INVAL;
    }

    if (vap->iv_ic->is_mbssid_enabled && ieee80211_mbss_is_beaconing_ap(vap) &&
        ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
        if (wlan_do_iter_fun_mbss_group(vap->iv_ic, vap->iv_mbss.group_id,
                                        MBSS_ITER_FUNC_PURPOSE_REPURPOSE_PHYMODE_CHECK, &vap->iv_des_mode)) {
            qdf_err("Error: PHY mode mismatch in repurpose vap-%d (%s) des mode: %d of mbss group: %d",
                    vap->iv_unit, vap->iv_netdev_name,
                    vap->iv_des_mode, vap->iv_mbss.group_id);
            return QDF_STATUS_E_INVAL;
        }
    }

    return QDF_STATUS_SUCCESS;
}
