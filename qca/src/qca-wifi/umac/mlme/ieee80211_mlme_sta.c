/*
 * Copyright (c) 2011-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
 * All Rights Reserved.
 */

#include "ieee80211_mlme_priv.h"    /* Private to MLME module */
#include <ieee80211_target.h>
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <ieee80211_regdmn.h>
#include <ieee80211_cfg80211.h>
#include <wlan_utility.h>
#include <wlan_lmac_if_api.h>
#include <ieee80211_ev.h>
#include <wlan_cm_public_struct.h>
#include <wlan_cm_blm.h>
#ifdef OL_ATH_SMART_LOGGING
#include <ol_if_athvar.h>
#endif /* OL_ATH_SMART_LOGGING */
#include <wlan_cm_api.h>
#include <wlan_mlme_if.h>
#include <ieee80211_crypto_nlshim_api.h>
#include <wlan_repeater_api.h>
#include <wlan_mbss.h>
#include <wlan_mlo_mgr_sta.h>
#include <utils_mlo.h>
#include <wlan_reg_ucfg_api.h>
#include <wlan_mlo_mgr_sta.h>
#include <utils_mlo.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_peer.h>
#include <utils_mlo.h>
#include <ieee80211_proto_mlo.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_setup.h>
#endif
#include "cfg_ol.h"
#include "cfg_ucfg_api.h"
#include <wlan_mlme_vdev_mgmt_ops.h>
#if ATH_SUPPORT_WRAP
extern struct ieee80211vap *wlan_get_vap(struct wlan_objmgr_vdev *vdev);
#include "dp_wrap.h"
#endif
#include <wlan_reg_ucfg_api.h>

static int mlme_assoc_reassoc_request(wlan_if_t vaphandle,
                                      int       reassoc,
                                      u_int8_t  *prev_bssid,
                                      u_int32_t timeout);
static int mlme_process_asresp_elements(struct ieee80211_node *ni,
                                        u_int8_t              *frm,
                                        u_int32_t             ie_len,
                                        bool                  *qcn_ie,
                                        uint8_t               **t2lmie);
static int mlme_process_authresp_ft_elements(struct ieee80211_node *ni,
                                        u_int8_t              *frm,
                                        u_int32_t             ie_len);

static int mlme_process_timeout_interval_elements(struct ieee80211_node *ni,
                                        u_int8_t              *frm,
                                        u_int32_t             ie_len);

#define MAX_TSFSYNC_TIME   1500     /* msec */

QDF_STATUS wlan_mlme_cm_ext_hdl_create(struct wlan_objmgr_vdev *vdev,
                                       cm_ext_t **ext_cm_ptr)
{
    *ext_cm_ptr = qdf_mem_malloc(sizeof(cm_ext_t));
     if (!(*ext_cm_ptr))
         return QDF_STATUS_E_NOMEM;

      qdf_timer_init(NULL, &(*ext_cm_ptr)->cm_defer_resp,
                     mlme_cm_resp_timer, (void *)(vdev),
                     QDF_TIMER_TYPE_WAKE_APPS);

     return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_cm_ext_hdl_destroy(struct wlan_objmgr_vdev *vdev,
                                        cm_ext_t *ext_cm_ptr)
{
     qdf_timer_free(&ext_cm_ptr->cm_defer_resp);
     qdf_mem_free(ext_cm_ptr);

     return QDF_STATUS_SUCCESS;
}

struct scan_cache_entry *mlme_cm_get_active_scan_entry(struct ieee80211vap *vap)
{
    struct wlan_cm_vdev_connect_req req = {0};
    struct wlan_cm_vdev_reassoc_req reassoc_req = {0};

    if (wlan_cm_get_active_req_type(vap->vdev_obj) == CM_ROAM_ACTIVE) {
        wlan_cm_get_active_reassoc_req(vap->vdev_obj, &reassoc_req);
        if (reassoc_req.bss && reassoc_req.bss->entry)
            return reassoc_req.bss->entry;
    } else {
        wlan_cm_get_active_connect_req(vap->vdev_obj, &req);
        if (req.bss && req.bss->entry)
            return req.bss->entry;
    }
    return NULL;
}

struct ieee80211_ath_channel mlme_cm_get_chosen_chan(struct ieee80211vap *vap)
{
    struct ieee80211_ath_channel scan_chan_obj = empty_channel;
    struct ieee80211_ath_channel *scan_chan = &scan_chan_obj;
    struct ieee80211_ath_channel chosen_chan_obj = empty_channel;
    struct ieee80211_ath_channel *chosen_chan = &chosen_chan_obj;
    struct ieee80211_ath_channel tmp_chan;
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;
    struct wlan_cm_vdev_connect_req req = {0};
    struct wlan_cm_vdev_reassoc_req reassoc_req = {0};
    struct chan_params chan_p = {0};
    bool is_input_phymode_radar = false;

    if (wlan_cm_get_active_req_type(vap->vdev_obj) == CM_ROAM_ACTIVE) {
        wlan_cm_get_active_reassoc_req(vap->vdev_obj, &reassoc_req);
        if (reassoc_req.bss && reassoc_req.bss->entry) {
            tmp_chan = wlan_util_scan_entry_channel(vap->iv_ic,
                                                    reassoc_req.bss->entry,
                                                    true);
            *scan_chan = tmp_chan;
            *chosen_chan = tmp_chan;
        }
    } else {
        wlan_cm_get_active_connect_req(vap->vdev_obj, &req);
        if (req.bss && req.bss->entry) {
            tmp_chan = wlan_util_scan_entry_channel(vap->iv_ic,
                                                    req.bss->entry,
                                                    true);
            *scan_chan = tmp_chan;
            *chosen_chan = tmp_chan;
        }
    }

    if (!IEEE80211_IS_CHAN_VALID(scan_chan))
        return empty_channel;

    *chosen_chan = ieee80211_find_connect_channel(vap,
                                                  chosen_chan,
                                                  &phymode,
                                                  &is_input_phymode_radar,
                                                  IEEE80211_MSG_MLME);

    /* Return Empty channel only if the channel is invalid due to radar.
     * For any other reasons, return scan channel.
     */
    if (is_input_phymode_radar && !IEEE80211_IS_CHAN_VALID(chosen_chan)) {
        ieee80211_note(vap, IEEE80211_MSG_MLME, "%s ,Cannot connect in radar channel %d."
                " Mode %d \n", __func__, chan_p.freq, phymode);
        return empty_channel;
    }

    if (!IEEE80211_IS_CHAN_VALID(chosen_chan)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s ,cannot find a valid channel \n", __func__);
        *chosen_chan = *scan_chan;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s , Chosen Channel %d."
            " Mode %d . Is radar %d \n", __func__, chosen_chan->ic_freq,
            ieee80211_chan2mode(chosen_chan), is_input_phymode_radar);

    return *chosen_chan;
}

enum cm_action_debug {
    CM_EV_UNKNOWN,
    CM_EV_BSS_PEER_CREATE_SUCCESS_RESP,
    CM_EV_BSS_PEER_CREATE_FAILURE_RESP,
    CM_EV_BSS_PEER_DELETE_SUCCESS_RESP,
    CM_EV_BSS_PEER_DELETE_FAILURE_RESP,
    CM_EV_CONNECT_RESP,
    CM_EV_DISCONNECT_RESP,
    CM_EV_BSS_PEER_DELETE_IND,
    CM_EV_BSS_PEER_SELECT_IND,
    CM_EV_BSS_PEER_SELECT_IND_FAILURE,
    CM_CMD_BSS_PEER_SELECT_IND,
    CM_CMD_BSS_PEER_CREATE,
    CM_CMD_JOIN_START,
    CM_CMD_DISCONNECT_ACTIVE,
    CM_CMD_BSS_PEER_DELETE,
    CM_TXCHANSWITCH_START,
    CM_TXCHANSWITCH_COMPLETE,
    CM_REPEATER_CAC_START,
    CM_REPEATER_CAC_COMPLETE,
    CM_EV_MAX,
};

const char *wlan_mlme_cm_action_name[CM_EV_MAX + 1] = {
    "UNKNOWN",
    "BSS_PEER_CREATE_SUCCESS",
    "BSS_PEER_CREATE_FAILURE",
    "BSS_PEER_DELETE_SUCCESS",
    "BSS_PEER_DELETE_FAILURE",
    "CONNECT_RESP",
    "DISCONNECT_RESP",
    "BSS_PEER_DELETE_IND",
    "BSS_PEER_SELECT_IND",
    "BSS_PEER_SELECT_IND_FAILURE",
    "BSS_PEER_SELECT_IND_REQ",
    "BSS_PEER_CREATE_REQ",
    "JOIN_START_REQ",
    "DISCONNECT_ACTIVE",
    "BSS_PEER_DELETE_REQ",
    "TXCHANSWITCH_START",
    "TXCHANSWITCH_COMPLETE",
    "REPEATER_CAC_START",
    "REPEATER_CAC_COMPLETE",
    "EV_MAX"

};

#if SM_ENG_HIST_ENABLE
void wlan_mlme_cm_action_history_init(struct ieee80211vap *vap)
{
    cm_ext_t *cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);

    if (!cm_ext_handle)
        return;

    qdf_mem_zero(&cm_ext_handle->cm_action_history, sizeof(struct wlan_cm_action_entry));
    qdf_spinlock_create(&cm_ext_handle->cm_action_history.cm_history_lock);
}

void wlan_mlme_cm_action_history_deinit(struct ieee80211vap *vap)
{
    cm_ext_t *cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);

    if (!cm_ext_handle)
        return;

    qdf_spinlock_destroy(&cm_ext_handle->cm_action_history.cm_history_lock);
}

static void wlan_mlme_cm_action_save_history(struct ieee80211vap *vap,
                                      uint8_t event, QDF_STATUS status)
{
    cm_ext_t *cm_ext_handle = NULL;
    struct wlan_cm_action_history *cm_action_history = NULL;
    struct wlan_cm_action_entry *data = NULL;

    cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);
    if (!cm_ext_handle)
        return;

    cm_action_history = &cm_ext_handle->cm_action_history;
    data = &cm_action_history->data[cm_action_history->index];

    qdf_spin_lock_bh(&cm_action_history->cm_history_lock);
    cm_action_history->index++;
    cm_action_history->index %= WLAN_CM_EVENT_HISTORY_SIZE;
    qdf_mem_zero(data, sizeof(struct wlan_cm_action_entry));

    data->event = event;
    data->status = status;
    data->time = qdf_get_log_timestamp();
    qdf_spin_unlock_bh(&cm_action_history->cm_history_lock);
}

static inline void
wlan_mlme_cm_print_history_entry(struct wlan_cm_action_entry *data, uint8_t idx)
{
    if (!data->event)
        return;

    wlan_mlme_nofl_err("| 0x%016llx |%6d |%28s |%6d |",
                       data->time, idx, wlan_mlme_cm_action_name[data->event],
                       data->status);
}

void wlan_mlme_cm_action_print_history(struct wlan_cm_action_history *cm_action_history)
{
    uint8_t i, idx;
    qdf_spin_lock_bh(&cm_action_history->cm_history_lock);

    wlan_mlme_nofl_err("|%19s |%6s |%28s |%6s |","Time", "Index", "Event", "Status");

    for (i = 0; i < WLAN_CM_EVENT_HISTORY_SIZE; i++) {
         idx = (cm_action_history->index + i) % WLAN_CM_EVENT_HISTORY_SIZE;
         wlan_mlme_cm_print_history_entry(&cm_action_history->data[idx], idx);
    }
    qdf_spin_unlock_bh(&cm_action_history->cm_history_lock);
}
#else
static inline void wlan_mlme_cm_action_save_history(struct ieee80211vap *vap, uint8_t event,
                                             QDF_STATUS status)
{}
#endif

#ifdef WLAN_FEATURE_11BE_MLO
#ifdef ENABLE_CFG80211_BACKPORTS
static inline
void mlme_cm_fill_chan_freq_for_partner(struct ieee80211vap *vap, cm_ext_t *cm_ext_handle)
{
    int i;
    struct mlo_partner_info rsp_partner_info;
    struct mlo_partner_info vdev_partner_info;

    rsp_partner_info = cm_ext_handle->cm_conn_rsp.ml_parnter_info;
    vdev_partner_info = vap->vdev_obj->mlo_dev_ctx->sta_ctx->ml_partner_info;
    for (i = 0; i < cm_ext_handle->cm_conn_rsp.ml_parnter_info.num_partner_links; i++) {
         rsp_partner_info.partner_link_info[i].chan_freq =
                              vdev_partner_info.partner_link_info[i].chan_freq;
    }
}
#endif

static inline
void mlme_cm_fill_ml_partner_info(struct ieee80211vap *vap, cm_ext_t *cm_ext_handle)
{
    struct element_info assoc_rsp = {0};
    struct wlan_objmgr_vdev *assoc_vdev = NULL;

    if (cm_ext_handle->cm_conn_rsp.connect_status == QDF_STATUS_SUCCESS &&
        mlo_is_mld_sta(vap->vdev_obj)) {
        if (vap->iv_bss->ml_info.num_partner_links) {
            qdf_mem_copy(&cm_ext_handle->cm_conn_rsp.ml_parnter_info,
                          &vap->iv_bss->ml_info, sizeof(struct mlo_partner_info));
            /* Exclude assoc link */
            cm_ext_handle->cm_conn_rsp.ml_parnter_info.num_partner_links -= 1;
        }

#ifdef ENABLE_CFG80211_BACKPORTS
        assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vap->vdev_obj);
        if (assoc_vdev == vap->vdev_obj) {
            mlme_cm_fill_chan_freq_for_partner(vap, cm_ext_handle);
        }
#endif
        /* Fill partner for Bridge vap */
        if (mlo_is_sta_bridge_vdev(vap->vdev_obj)) {
            assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vap->vdev_obj);
            if (!assoc_vdev) {
                qdf_err("assoc vdev not found");
                return;
            }
            qdf_mem_copy(&cm_ext_handle->cm_conn_rsp.ml_parnter_info,
                         &assoc_vdev->mlo_dev_ctx->bridge_sta_ctx->bridge_partners,
                         sizeof(struct mlo_partner_info));
            cm_ext_handle->cm_conn_rsp.ml_parnter_info.num_partner_links = 2;
        }

        if (wlan_vdev_mlme_is_mlo_link_vdev(vap->vdev_obj)) {
            mlo_get_assoc_rsp(vap->vdev_obj, &assoc_rsp);
            cm_ext_handle->cm_conn_rsp.connect_ies.assoc_rsp.ptr =
                            assoc_rsp.ptr;
            cm_ext_handle->cm_conn_rsp.connect_ies.assoc_rsp.len =
                            assoc_rsp.len;
        }
    }
}
#else
static inline
void mlme_cm_fill_ml_partner_info(struct ieee80211vap *vap, cm_ext_t *cm_ext_handle)
{ }
#endif

void mlme_cm_connect_rsp(struct ieee80211vap *vap)
{
     struct ieee80211_ath_channel chan_obj;
     struct ieee80211_ath_channel *chan = &chan_obj;
     enum wlan_cm_active_request_type active_req;
     struct wlan_cm_vdev_connect_req req = {0};
     struct wlan_cm_vdev_reassoc_req reassoc_req = {0};
     cm_ext_t *cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);

     if (!cm_ext_handle)
         return;

     wlan_mlme_err("Connect Resp: psoc: %d, vdev: %d, Status: %d, Reason: %d",
                   wlan_psoc_get_id(wlan_vdev_get_psoc(vap->vdev_obj)),
                   wlan_vdev_get_id(vap->vdev_obj),
                   cm_ext_handle->cm_conn_rsp.connect_status,
                   cm_ext_handle->cm_conn_rsp.reason);

     cm_ext_handle->cm_conn_rsp.vdev_id = wlan_vdev_get_id(vap->vdev_obj);
     cm_ext_handle->cm_conn_rsp.aid = vap->iv_bss->ni_associd;
     cm_ext_handle->cm_conn_rsp.connect_ies.assoc_req.ptr = NULL;
     cm_ext_handle->cm_conn_rsp.connect_ies.assoc_req.len = 0;
     cm_ext_handle->cm_conn_rsp.connect_ies.assoc_rsp.ptr = vap->iv_sta_assoc_resp_ie;
     cm_ext_handle->cm_conn_rsp.connect_ies.assoc_rsp.len = vap->iv_sta_assoc_resp_len;
     qdf_mem_copy(cm_ext_handle->cm_conn_rsp.ssid.ssid, vap->iv_des_ssid[0].ssid, vap->iv_des_ssid[0].len);
     cm_ext_handle->cm_conn_rsp.ssid.length = vap->iv_des_ssid[0].len;

     *chan = mlme_cm_get_chosen_chan(vap);
     if (IEEE80211_IS_CHAN_VALID(chan))
         cm_ext_handle->cm_conn_rsp.freq = chan->ic_freq;

     mlme_cm_fill_ml_partner_info(vap, cm_ext_handle);

     active_req = wlan_cm_get_active_req_type(vap->vdev_obj);
     if (active_req == CM_ROAM_ACTIVE) {
         wlan_cm_get_active_reassoc_req(vap->vdev_obj, &reassoc_req);
         if (reassoc_req.bss && reassoc_req.bss->entry) {
              qdf_mem_copy(cm_ext_handle->cm_conn_rsp.bssid.bytes,
                           util_scan_entry_macaddr(reassoc_req.bss->entry),
                           sizeof(struct qdf_mac_addr));
         }

         cm_ext_handle->cm_conn_rsp.is_reassoc = true;
         wlan_cm_reassoc_rsp(vap->vdev_obj, &cm_ext_handle->cm_conn_rsp);
     } else {
         wlan_cm_get_active_connect_req(vap->vdev_obj, &req);
         if (req.bss && req.bss->entry) {
             qdf_mem_copy(cm_ext_handle->cm_conn_rsp.bssid.bytes,
                          util_scan_entry_macaddr(req.bss->entry),
                          sizeof(struct qdf_mac_addr));
         }
         wlan_cm_connect_rsp(vap->vdev_obj, &cm_ext_handle->cm_conn_rsp);
     }
}

void mlme_cm_disconnect_rsp(struct ieee80211vap *vap)
{
     cm_ext_t *cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);
     if (!cm_ext_handle)
         return;

     wlan_mlme_err("Disconnect Resp: Psoc: %d, Vdev: %d, Source: %d, Reason: %d",
                   wlan_psoc_get_id(wlan_vdev_get_psoc(vap->vdev_obj)),
                   wlan_vdev_get_id(vap->vdev_obj),
                   cm_ext_handle->cm_discon_rsp.req.req.source,
                   cm_ext_handle->cm_discon_rsp.req.req.reason_code);
     cm_ext_handle->cm_discon_rsp.ap_discon_ie.ptr = NULL;
     cm_ext_handle->cm_discon_rsp.ap_discon_ie.len = 0;
     wlan_cm_disconnect_rsp(vap->vdev_obj, &cm_ext_handle->cm_discon_rsp);
}

void mlme_cm_resp_timer(void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)arg;
    struct ieee80211vap *vap = NULL;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    cm_ext_t *cm_ext_handle = NULL;

    if (!vdev)
        return;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return;

    cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
    if (!cm_ext_handle)
        return;

    qdf_debug("CM resp event %d, psoc_id %d, vdev_id %d",cm_ext_handle->cm_evt_id,
              wlan_psoc_get_id(wlan_vdev_get_psoc(vdev)),
              wlan_vdev_get_id(vdev));

    if (cm_ext_handle->cm_evt_id == CM_CONNECT_RESP)
        status = cm_ext_handle->cm_conn_rsp.connect_status;

    wlan_mlme_cm_action_save_history(vap, (cm_ext_handle->cm_evt_id + 1), status);

    switch (cm_ext_handle->cm_evt_id) {
    case CM_BSS_PEER_CREATE_SUCCESS_RESP:
         wlan_cm_bss_peer_create_rsp(vdev, QDF_STATUS_SUCCESS, NULL);
         break;
    case CM_BSS_PEER_CREATE_FAILURE_RESP:
         wlan_cm_bss_peer_create_rsp(vdev, QDF_STATUS_E_FAILURE, NULL);
         break;
    case CM_BSS_PEER_DELETE_SUCCESS_RESP:
         wlan_cm_bss_peer_delete_rsp(vdev, QDF_STATUS_SUCCESS);
         break;
    case CM_CONNECT_RESP:
         mlme_cm_connect_rsp(vap);
         break;
    case CM_DISCONNECT_RESP:
         mlme_cm_disconnect_rsp(vap);
         break;
    case CM_BSS_PEER_SELECT_IND:
         wlan_cm_bss_select_ind_rsp(vdev, QDF_STATUS_SUCCESS);
         break;
    case CM_BSS_PEER_SELECT_IND_FAILURE:
         wlan_cm_bss_select_ind_rsp(vdev, QDF_STATUS_E_FAILURE);
         break;
    case CM_BSS_PEER_DELETE_IND:
         wlan_cm_bss_peer_delete_ind(vdev,
                                     (struct qdf_mac_addr *)vap->iv_bss->ni_macaddr);
         break;
    default:
         break;
    }
}

QDF_STATUS wlan_mlme_cm_join_start(struct wlan_objmgr_vdev *vdev, struct wlan_cm_vdev_connect_req *req)
{
    struct ieee80211vap *vap;
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);

    if (!vap)
        return QDF_STATUS_E_FAILURE;

    if (mlo_is_sta_bridge_vdev(vap->vdev_obj))
        status = wlan_mlme_join_infra_continue(vap, NULL, MAX_TSFSYNC_TIME);
    else
        status = wlan_mlme_join_infra_continue(vap, req->bss->entry, MAX_TSFSYNC_TIME);

    if (status != 0) {
        IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: join_infra failed \n",__func__);
        /*
         * Since VAP bring up failed, delete the earlier created bss peer
         */
        ieee80211_reset_bss(vap);
        wlan_mlme_cancel(vap);
        status = QDF_STATUS_E_FAILURE;
    }
    wlan_mlme_cm_action_save_history(vap, CM_CMD_JOIN_START,
                              status);
    return status;
}

QDF_STATUS wlan_mlme_cm_reassoc_join_start(
                    struct wlan_objmgr_vdev *vdev,
                    struct wlan_cm_vdev_reassoc_req *req)
{
    struct ieee80211vap *vap = NULL;
    cm_ext_t *cm_ext_handle = NULL;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
    if (!cm_ext_handle)
        return QDF_STATUS_E_FAILURE;

    cm_ext_handle->cm_conn_rsp.cm_id = req->cm_id;

    if (wlan_mlme_join_infra_continue(vap, req->bss->entry,
                                      MAX_TSFSYNC_TIME) != 0 ) {
        IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,
                "%s: join_infra failed \n",__func__);
        ieee80211_reset_bss(vap);
        wlan_mlme_cancel(vap);
        return QDF_STATUS_E_FAILURE;
    }

    return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_11BE_MLO
static void wlan_mlme_setup_ml_peer(struct wlan_objmgr_vdev *vdev,
                                    struct ieee80211_node *ni)
{
    struct wlan_objmgr_vdev *assoc_vdev = NULL;
    struct wlan_objmgr_peer *peer = ni->peer_obj;

    if (mlo_is_mld_sta(vdev)) {
        /* Set peer to mlo peer if vdev is mld sta vdev */
        wlan_peer_set_mlo(peer);
        /* If vdev is assoc link vdev, create mlo peer else attach
         * peer to ml peer
         */
        if (wlan_vdev_mlme_is_assoc_sta_vdev(vdev)) {
            wlan_peer_mlme_set_assoc_peer(ni->peer_obj, true);
            wlan_mlo_peer_create(vdev, peer,
                                 &ni->ml_info, NULL, -1);
        } else {
            assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vdev);
            if (assoc_vdev) {
                qdf_info("mlo peer attach for peer with mac"
                         QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(ni->ni_macaddr));
                wlan_mlo_link_peer_attach(
                           assoc_vdev->vdev_objmgr.bss_peer->mlo_peer_ctx,
                           peer, NULL);
                /* Update dev flags if assoc and primary umac are different
                   and the current vdev is the central vdev which should be
                   primary for certain topologies */
                if (vdev->vdev_objmgr.mlo_central_vdev)
                    osif_peer_update_mlo_sta_dev(peer, peer->mlo_peer_ctx);
            }
        }
    }
}
#else
static void wlan_mlme_setup_ml_peer(struct wlan_objmgr_vdev *vdev,
                                    struct ieee80211_node *ni)
{ }
#endif

static void wlan_mlme_join_setting_country_handler(void *osif, IEEE80211_STATUS status)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)osif;

    if (status == IEEE80211_STATUS_SUCCESS) {
        wlan_mlme_setup_ml_peer(vap->vdev_obj, vap->iv_bss);
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_CREATE_SUCCESS_RESP);
    } else {
        wlan_mlme_cancel(vap);
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_CREATE_FAILURE_RESP);
    }
}

QDF_STATUS wlan_mlme_cm_ext_bss_peer_create_req(struct wlan_objmgr_vdev *vdev,
                                                struct qdf_mac_addr *peer_mac,
                                                struct qdf_mac_addr *mld_mac,
                                                bool is_assoc_link)
{
    struct ieee80211vap *vap;
    u_int8_t *se_macaddr;
    struct ieee80211_node *ni = NULL;
    struct wlan_objmgr_peer *peer = NULL;
    struct wlan_cm_vdev_connect_req req = {0};
    struct wlan_cm_vdev_reassoc_req reassoc_req = {0};
    struct scan_cache_entry *entry = NULL;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    uint8_t peer_del_req_send;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    if (mlo_is_sta_bridge_vdev(vdev)) {
        ieee80211_reset_bss(vap);
        if (wlan_mlme_join_infra(vap, NULL, MAX_TSFSYNC_TIME, mld_mac) != 0 ) {
            IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: join_infra failed \n",__func__);
            wlan_mlme_join_setting_country_handler((void*)vap, IEEE80211_STATUS_UNSPECIFIED);
        }
        goto exit;
    }

    if (wlan_cm_get_active_req_type(vdev) == CM_ROAM_ACTIVE) {
        wlan_cm_get_active_reassoc_req(vdev, &reassoc_req);
        if (!reassoc_req.bss || !reassoc_req.bss->entry){
            status = QDF_STATUS_E_FAILURE;
            goto exit;
        }
        entry = reassoc_req.bss->entry;
    } else {
        wlan_cm_get_active_connect_req(vap->vdev_obj, &req);

        if (!req.bss || !req.bss->entry) {
            status = QDF_STATUS_E_FAILURE;
            goto exit;
        }
        entry = req.bss->entry;
    }

    se_macaddr = util_scan_entry_macaddr(entry);
    if (qdf_mem_cmp(se_macaddr, peer_mac, QDF_MAC_ADDR_SIZE)) {
        status = QDF_STATUS_E_FAILURE;
        goto exit;
    }

    peer = ieee80211_lookup_peer_by_mac(vap->iv_ic, se_macaddr);
    if ((peer != NULL) && (WLAN_PEER_STA == wlan_peer_get_peer_type(peer))) {
        /*
         * Node is present on another VAP and possibly on different radio
         * peer with same mac address cannot be created across SoC for HKv1
         *
         */
        qdf_warn("%s:peer with same mac address cannot be created across SoC\n",__func__);
        wlan_objmgr_peer_release_ref(peer, WLAN_MGMT_HANDLER_ID);
        status = QDF_STATUS_E_FAILURE;
        goto exit;
    }

    /* release the refrence taken for peer !WLAN_PEER_STA */
    if (peer != NULL) {
        wlan_objmgr_peer_release_ref(peer, WLAN_MGMT_HANDLER_ID);
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (mld_mac && wlan_vdev_mlme_is_assoc_sta_vdev(vdev) &&
#if ATH_SUPPORT_WRAP
        wlan_rptr_vdev_is_mpsta(vdev) &&
#endif
        mlo_mgr_ml_peer_exist_on_diff_ml_ctx(mld_mac->bytes, NULL)) {
        mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " rejecting connection",
                QDF_MAC_ADDR_REF(mld_mac));
        status = QDF_STATUS_E_FAILURE;
        goto exit;
    }
#endif

    /* assoc sm RUN to JOIN state transition requires to wait
     * for previous peer being deleted and new connection
     * attempt being started once peer delete resonse is received.
     */
    ni = ieee80211_vap_find_node(vap, se_macaddr, WLAN_MLME_SB_ID);
    if (ni) {
        qdf_info("%s: vap:%d(0x%pK), previous node: 0x%pK:%s found."
                 " delete it first", __func__, vap->iv_unit, vap,
                 ni, ether_sprintf(se_macaddr));
        qdf_info("vap->iv_myaddr: %s", ether_sprintf(vap->iv_myaddr));
        if (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_RTT_PASN) {
            peer_del_req_send = MLME_INDIVIDUAL_PEER_CLEANUP;
            cleanup_sta_peer(&peer_del_req_send, ni);
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            wlan_mlme_cancel(vap);
            status = QDF_STATUS_E_FAILURE;
            goto exit;
        }
        /* Free previous peer */
        ieee80211_sta_leave(ni);
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
        /* Not expected to hit this scenario. Added assert for debug purpose */
        QDF_ASSERT(0);
    }

    ieee80211_reset_bss(vap);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_STATE,
                      "%s: vap:%d(0x%pK), continue to JOIN_COMPLETE\n",
                      __func__, vap->iv_unit, vap);
    if (wlan_mlme_join_infra(vap, entry, MAX_TSFSYNC_TIME, mld_mac) != 0 ) {
        IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: join_infra failed \n",__func__);
        wlan_mlme_join_setting_country_handler((void*)vap, IEEE80211_STATUS_UNSPECIFIED);
    }
exit:
    wlan_mlme_cm_action_save_history(vap, CM_CMD_BSS_PEER_CREATE,
                              status);
    return status;
}

static void wlan_mlme_repeater_cac_handler(void *osif, IEEE80211_STATUS status);

#define MAX_TXCSA_TIME     1000    /* msec */
static void wlan_mlme_txchanswitch_handler(void *osif, IEEE80211_STATUS status)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)osif;
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;

    *chan = mlme_cm_get_chosen_chan(vap);
    wlan_mlme_cancel_txchanswitch_timer(vap);
    wlan_mlme_cm_action_save_history(vap, CM_TXCHANSWITCH_COMPLETE,
                               status);
    if (!IEEE80211_IS_CHAN_VALID(chan)) {
        wlan_mlme_cancel(vap);
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
        return;
    }

#if ATH_SUPPORT_DFS
    if(wlan_mlme_is_repeater_cac_set(vap) &&
       wlan_mlme_is_scanentry_dfs(chan)) {
       if (wlan_mlme_start_repeater_cac(vap, chan) != 0 ) {
           IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: repeater cac failed \n",__func__);
           wlan_mlme_repeater_cac_handler((void *)vap, IEEE80211_STATUS_UNSPECIFIED);
           return;
       }
    } else
#endif
    {
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND);
    }
    return;
}

static void wlan_mlme_repeater_cac_handler(void *osif, IEEE80211_STATUS status)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)osif;
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;
    struct wlan_cm_vdev_connect_req req = {0};

    wlan_mlme_cancel_repeater_cac_timer(vap);
    wlan_mlme_cm_action_save_history(vap, CM_REPEATER_CAC_COMPLETE,
                               status);
    if (status == IEEE80211_STATUS_SUCCESS) {
        /* If AP VAP comes up first, let's say in channel 100, then AP VAP is
         * doing CAC. In the mean time if STA VAP found Root AP in channel 60
         * then STA thinks that AP is doing CAC in channel 60. Since AP VAP is
         * still in DFS_WAIT number of AP VAP running is zero and therefore
         * TXCSA does not happen. If TXCSA does not happen after CAC AP VAP
         * comes up in channel 100 and STA comes up in channel 60. And STA
         * does not detect a beacon miss.
         */
        *chan = mlme_cm_get_chosen_chan(vap);
        if (!IEEE80211_IS_CHAN_VALID(chan)) {
            wlan_mlme_cancel(vap);
            wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
            return;
        }
        /* if STA's target channel and AP channels are different after CAC then go to TXSwitch again */
        if(wlan_mlme_is_chosen_diff_from_curchan(vap, chan)) {
           if (wlan_mlme_start_txchanswitch(vap, chan, MAX_TXCSA_TIME) != 0 ) {
               IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: txchanswitch failed \n",__func__);
               wlan_mlme_txchanswitch_handler((void*)vap, IEEE80211_STATUS_NOT_ASSOCED);//IEEE80211_STATUS_UNSPECIFIED);
               return;
           }
       } else {
               wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND);
         }
    } else {
        wlan_cm_get_active_connect_req(vap->vdev_obj, &req);
        if (req.bss && req.bss->entry)
            wlan_util_scan_entry_mlme_set_assoc_state(req.bss->entry, AP_ASSOC_STATE_NONE);

        wlan_mlme_cancel(vap);
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
    }
    return;
}

QDF_STATUS wlan_mlme_cm_vdev_down(struct wlan_objmgr_vdev *vdev)
{
   wlan_vdev_mlme_sm_deliver_evt(
            vdev, WLAN_VDEV_SM_EV_MLME_DOWN_REQ,
            0, NULL);

    return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_11BE_MLO
static void wlan_mlme_cleanup_ml_peer(struct wlan_objmgr_peer *peer)
{
    if (wlan_peer_is_mlo(peer))
        wlan_mlo_partner_peer_disconnect_notify(peer);
}
#else
static void wlan_mlme_cleanup_ml_peer(struct wlan_objmgr_peer *peer)
{ }
#endif

QDF_STATUS wlan_mlme_cm_bss_peer_delete_req(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211vap *vap;
    struct vdev_mlme_obj *vdev_mlme;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    wlan_mlme_cm_action_save_history(vap, CM_CMD_BSS_PEER_DELETE,
                              QDF_STATUS_SUCCESS);
    vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vdev);
    if (!vdev_mlme) {
        qdf_err("Vdev mlme component object is NULL");
        return QDF_STATUS_E_FAILURE;
    }

    if ((vap->iv_bss) && (!vdev_mlme->ml_reconfig_started))
        wlan_mlme_cleanup_ml_peer(vap->iv_bss->peer_obj);

    ieee80211_reset_bss(vap);
    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_cm_connect_start(struct wlan_objmgr_vdev *vdev,
                                      struct wlan_cm_connect_req *cm_conn_req)
{
    struct ieee80211vap *vap;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    if (wlan_reg_is_6ghz_chan_freq(cm_conn_req->chan_freq)) {
        if (wlan_6ghz_security_check(vap, cm_conn_req->crypto.akm_suites,
                    cm_conn_req->crypto.rsn_caps) != EOK) {
            qdf_warn("Error in 6GHz security configured !!");
            return QDF_STATUS_E_FAILURE;
        }
    }

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_cm_connect_active(struct wlan_objmgr_vdev *vdev,
                                       struct wlan_cm_vdev_connect_req *req)
{
    struct ieee80211vap *vap;
    wlan_scan_entry_t scan_entry;
    qdf_freq_t scan_freq;
    struct ieee80211_ath_channel chosen_chan_obj;
    struct ieee80211_ath_channel *chosen_chan = &chosen_chan_obj;
    struct wlan_objmgr_pdev *pdev;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    cm_ext_t *cm_ext_handle = wlan_cm_get_ext_hdl(vdev);

    if (!cm_ext_handle)
        return QDF_STATUS_E_FAILURE;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap || !vap->iv_ic) {
        status = QDF_STATUS_E_FAILURE;
        goto fail;
    }
    /* For bridge vdev dispatch event and return success */
    if (mlo_is_sta_bridge_vdev(vap->vdev_obj)) {
        qdf_mem_zero(&cm_ext_handle->cm_conn_rsp, sizeof(struct wlan_cm_connect_resp));
        cm_ext_handle->cm_conn_rsp.cm_id = req->cm_id;
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND);
        status = QDF_STATUS_SUCCESS;
        goto end;
    }

    scan_entry = req->bss->entry;
    qdf_mem_zero(&cm_ext_handle->cm_conn_rsp, sizeof(struct wlan_cm_connect_resp));
    cm_ext_handle->cm_conn_rsp.cm_id = req->cm_id;

    scan_freq = scan_entry->channel.chan_freq;
    if (!scan_freq) {
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
        status = QDF_STATUS_SUCCESS;
        goto end;
    }
#if ATH_SUPPORT_WRAP
    status = wlan_rptr_psta_validate_chan(vdev, scan_freq);
    if (QDF_IS_STATUS_ERROR(status)) {
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
        status = QDF_STATUS_SUCCESS;
        goto end;
    }
#endif

    *chosen_chan = mlme_cm_get_chosen_chan(vap);
    if (!IEEE80211_IS_CHAN_VALID(chosen_chan)) {
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
        status = QDF_STATUS_SUCCESS;
        goto end;
    }

    pdev = vap->iv_ic->ic_pdev_obj;
    if (pdev == NULL) {
        qdf_err("%s : pdev is null", __func__);
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
        status = QDF_STATUS_SUCCESS;
        goto end;
    }

    /* Go for Tx CSA when all of the following are true:
     * 1)Tx CSA is set by the user.
     * 2)Enhanced Independent Repeater  is set by the user.
     * 3)The STA is the main STA (either regular STA or main proxy STA).
     * 4)There is at least on AP running/beaconing.
     * 5)When the chosen phy channel and current channel is different.
     */
    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
        QDF_STATUS_SUCCESS) {
        wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND_FAILURE);
        status = QDF_STATUS_SUCCESS;
        goto end;
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME,
               "%s : TXCSA = %d RepCAC = %d Enh_ind = %d Mainsta = %d apvap = %d chosen_diff = %d apcac = %d\n", __func__,
                wlan_mlme_is_txcsa_set(vap) ,
                wlan_mlme_is_repeater_cac_set(vap),
                wlan_rptr_pdev_is_eir(pdev) ,
                wlan_mlme_is_vap_main_sta(vap) ,
                wlan_mlme_num_apvap_running(vap),
                wlan_mlme_is_chosen_diff_from_curchan(vap,chosen_chan),
                mlme_dfs_is_ap_cac_timer_running(pdev)
                );
    wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    if(wlan_rptr_pdev_is_eir(pdev) &&
       wlan_mlme_is_vap_main_sta(vap) &&
       wlan_mlme_num_apvap_running(vap) &&
       wlan_mlme_is_chosen_diff_from_curchan(vap,chosen_chan)) {
       if (wlan_mlme_start_txchanswitch(vap, chosen_chan, MAX_TXCSA_TIME) != 0 ) {
           IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: txchanswitch failed \n",__func__);
           wlan_mlme_txchanswitch_handler((void*)vap, IEEE80211_STATUS_CAPINFO);//IEEE80211_STATUS_UNSPECIFIED);
           status = QDF_STATUS_SUCCESS;
           goto end;
       }
    } else {
        /* If it is coming here for second time that is after txcsa is over
         * we need to see if the CAC timer is running then wait for the CAC
         */
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
                  QDF_STATUS_SUCCESS) {
            status = QDF_STATUS_E_FAILURE;
            goto end;
        }
        if(wlan_mlme_is_repeater_cac_set(vap) &&
           wlan_mlme_is_scanentry_dfs(chosen_chan) &&
           mlme_dfs_is_ap_cac_timer_running(pdev)) {
           wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
           if (wlan_mlme_start_repeater_cac(vap, chosen_chan) != 0 ) {
               IEEE80211_DPRINTF(vap,IEEE80211_MSG_STATE,"%s: repeater cac failed \n",__func__);
               wlan_mlme_repeater_cac_handler((void*)vap, IEEE80211_STATUS_UNSPECIFIED);
               status = QDF_STATUS_SUCCESS;
               goto end;
           }
        } else {
            wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
            wlan_mlme_dispatch_cm_resp(vap, CM_BSS_PEER_SELECT_IND);
        }
    }
end:
    wlan_mlme_cm_action_save_history(vap, CM_CMD_BSS_PEER_SELECT_IND,
                              status);
fail:
    return status;
}

/**
 * wlan_mlme_smart_log_connection_fail_stop - Stop connection failure smart logging
 * @ic: legacy ic handle
 *
 * Return: void
 */
#ifdef OL_ATH_SMART_LOGGING
static void wlan_mlme_smart_log_connection_fail_stop(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = NULL;
    struct ol_if_offload_ops *ol_if_ops = NULL;

    if (ic != NULL) {
        scn = OL_ATH_SOFTC_NET80211(ic);
    }

    if (scn && scn->soc) {
        ol_if_ops = scn->soc->ol_if_ops;
    }

    if (ol_if_ops && ol_if_ops->smart_log_connection_fail_stop) {
        /*
         * Currently, we will not alter the rest of the state machine flow
         * based on whether we succeeded or failed in stopping connection
         * failure logging. Hence we do not check for failure. We rely on
         * the API to print failure messages if any. Engineering action
         * would have to be taken separately to analyze logging API
         * failures.
         * We also rely on the API to determine whether the logging is
         * required.
         */
	    ol_if_ops->smart_log_connection_fail_stop(scn);
    }
}
#else
static inline
void wlan_mlme_smart_log_connection_fail_stop(struct ieee80211com *ic)
{
}
#endif

QDF_STATUS wlan_mlme_cm_connect_complete(struct wlan_objmgr_vdev *vdev,
                                         struct wlan_cm_connect_resp *resp)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    struct mlme_info mlme_info = {0};
    struct bss_info bss_info;
    struct ieee80211vap *vap;
    struct ieee80211com *ic;
    enum reg_6g_ap_type reg_cur_6g_ap_pwr_type;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    ic = vap->iv_ic;

    qdf_copy_macaddr(&bss_info.bssid, &resp->bssid);
    bss_info.freq = resp->freq;
    bss_info.ssid.length = resp->ssid.length;
    qdf_mem_copy(&bss_info.ssid.ssid, resp->ssid.ssid,
                 bss_info.ssid.length);

     if (resp->connect_status == QDF_STATUS_SUCCESS) {
         mlme_info.status = AP_STATE_GOOD;
         wlan_mlme_smart_log_connection_fail_stop(ic);
         if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan) ) {
             wlan_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj,
                                             &reg_cur_6g_ap_pwr_type);
             if ((vap->iv_bss->ni_ap_power_type != reg_cur_6g_ap_pwr_type) ||
                 (wlan_reg_is_afc_power_event_received(ic->ic_pdev_obj))) {
                 mlme_check_curchan_and_set_ap_pwr_type(ic, ic->ic_curchan);
             }
         }

         /* DFS Puncturing SM may be running before STA vap is connected.
          * The RE has to follow the puncture pattern, if any, from
          * the root. As STA vap is connected, stop the puncture SM here.
          */
         if (IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan)) {
             mlme_dfs_stop_punc_sm(ic->ic_pdev_obj);
         }
     } else {
#if ATH_SUPPORT_WRAP
              if ((ic != NULL) &&
                  ((dp_wrap_get_mpsta_vdev(ic->ic_pdev_obj) == NULL) ||
                  (dp_wrap_get_mpsta_vdev(ic->ic_pdev_obj) &&
                   wlan_rptr_vdev_is_mpsta(vap->vdev_obj))))
#endif
              {
                  mlme_info.status = AP_STATE_BAD;
                  mlme_info.bad_ap_time = qdf_system_ticks();
              }
     }

     wlan_scan_update_mlme_by_bssinfo(wlan_vdev_get_pdev(vdev),
                                      &bss_info, &mlme_info);
     return status;
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
QDF_STATUS wlan_mlme_cm_disconnect_start(struct wlan_objmgr_vdev *vdev,
                                         struct wlan_cm_disconnect_req *req)
{
    struct ieee80211vap *vap;
    enum wlan_vdev_state state;
    enum wlan_vdev_state substate;

    /* Action on disconnect request to serialization.
     * If STA CAC is enabled and CM is in join_active state, then
     * active command is not flushed. Hence dispatch EV_DOWN to abort
     * connect request.
     */
    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    if ((wlan_cm_get_active_req_type(vdev) == CM_CONNECT_ACTIVE) &&
            mlme_is_stacac_needed(vap)) {
        wlan_mlme_stop_bss(vap, 0);
        state = wlan_vdev_mlme_get_state(vdev);
        substate = wlan_vdev_mlme_get_substate(vdev);
        if ((state == WLAN_VDEV_S_START) &&
                ((substate == WLAN_VDEV_SS_START_RESTART_PROGRESS) ||
                 (substate == WLAN_VDEV_SS_START_START_PROGRESS)))
            wlan_vdev_mlme_sm_deliver_evt(vdev, WLAN_VDEV_SM_EV_DOWN, 0, NULL);
    }

    return QDF_STATUS_SUCCESS;
}
#else
QDF_STATUS wlan_mlme_cm_disconnect_start(struct wlan_objmgr_vdev *vdev,
                                         struct wlan_cm_disconnect_req *req)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    /* Action on disconnect request to serialization */
    return status;
}
#endif

QDF_STATUS wlan_mlme_cm_disconnect_active(struct wlan_objmgr_vdev *vdev,
                                          struct wlan_cm_vdev_discon_req *req)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    struct ieee80211vap *vap;
    cm_ext_t *cm_ext_handle = NULL;
    enum wlan_vdev_state state;
    enum wlan_vdev_state substate;

    /* Action on disconnect request activation*/
    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    cm_ext_handle = wlan_cm_get_ext_hdl(vdev);
    if (!cm_ext_handle)
        return QDF_STATUS_E_FAILURE;

    wlan_mlme_cm_action_save_history(vap, CM_CMD_DISCONNECT_ACTIVE,
                              status);
    qdf_mem_zero(&cm_ext_handle->cm_discon_rsp, sizeof(struct wlan_cm_discon_rsp));
    qdf_mem_copy(&cm_ext_handle->cm_discon_rsp.req, req, sizeof(struct wlan_cm_vdev_discon_req));
    wlan_mlme_stop_bss(vap, 0);

    state = wlan_vdev_mlme_get_state(vdev);
    substate = wlan_vdev_mlme_get_substate(vdev);
    if ((state == WLAN_VDEV_S_START) &&
        ((substate == WLAN_VDEV_SS_START_RESTART_PROGRESS) ||
        (substate == WLAN_VDEV_SS_START_START_PROGRESS)))
        wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj, WLAN_VDEV_SM_EV_DOWN, 0, NULL);
    return status;
}

#ifdef CONFIG_AFC_SUPPORT
/**
 * wlan_mlme_vdev_afc_pwr_modes_and_pwr() - Sends TPC wmi command for all the
 * running vaps and updates beacon template for AP vdevs.
 */
static void wlan_mlme_vdev_afc_pwr_modes_and_pwr(struct wlan_objmgr_pdev *pdev,
		                                                 void *object, void *arg)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
	struct ieee80211vap *vap = NULL;
	struct ieee80211com *ic = NULL;

	ic = wlan_pdev_get_mlme_ext_obj(pdev);
	if (!ic)
		return;

	if (ic->ic_flags & IEEE80211_F_CHANSWITCH) {
		qdf_info("Channel change in progress. Deferring beacon and "
			 "TPC update to restart response path");
		return;
	}

	vap = wlan_vdev_get_mlme_ext_obj(vdev);
	if (!vap)
		return;

	if (ieee80211_is_vap_state_running(vap) || ic_is_sta_vap(ic)) {
		ieee80211_send_tpc_power_cmd(vap);
		/* For a STA, FILS update is not needed as STA VAPs do not
		 * beacon. FILS update inherently takes care of beacon update
		 * even if FILS is diabled.
		 */
		if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
			enum wlan_frm_update_reason update_reason = UPDATE_FILS_UBPRBR;
			wlan_tmpl_update_6ghz_frm(ic, &update_reason);
		}
	}
}

void mlme_update_chan_list_and_restart_vaps(struct ieee80211com *ic,
					    bool update_bcnpwrinfo_and_fwtxpwr)
{
	uint8_t ctry_iso[REG_ALPHA2_LEN + 1];

	if (!ic) {
		qdf_info("ic pointer is NULL");
		return;
	}

#if UMAC_SUPPORT_CFG80211
	 qdf_sched_work(NULL,&ic->ic_cfg80211_update_channel_list_start_work);
#endif
	/* update channel list */
	ieee80211_update_channellist(ic, 1, true);
	ieee80211_getCurrentCountryISO(ic, ctry_iso);
	ieee80211_build_countryie_all(ic, ctry_iso);
	if (update_bcnpwrinfo_and_fwtxpwr)
		wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
		    wlan_mlme_vdev_afc_pwr_modes_and_pwr,
		    NULL, 0, WLAN_MLME_SB_ID);

}
qdf_export_symbol(mlme_update_chan_list_and_restart_vaps);
#endif

void mlme_quick_update_chan_list_and_send_tpc(struct ieee80211com *ic,
                                              bool send_tpc)
{
	struct ieee80211vap *special_vap = TAILQ_FIRST(&ic->ic_vaps);

	ieee80211_update_channellist(ic, 0, true);
	if (!send_tpc)
		return;

	if (!special_vap)
		return;

	if (ieee80211_is_vap_state_running(special_vap))
		ieee80211_send_tpc_power_cmd(special_vap);
}

qdf_export_symbol(mlme_quick_update_chan_list_and_send_tpc);

/* mlme_move_6g_client_to_best_pwr_mode: When a STA gets disconnected
 * from root AP, change the power mode of the VAPs to the best 6GHz AP power
 * mode.
*/
static QDF_STATUS
mlme_move_6g_client_to_best_pwr_mode(struct wlan_objmgr_vdev *vdev)
{
    struct ieee80211vap *vap;
    struct ieee80211com *ic;
    struct wlan_objmgr_pdev *pdev;
    enum reg_6g_ap_type cur_6g_ap_pwr_type, best_6g_ap_pwr_type;

    if (!WLAN_REG_IS_6GHZ_CHAN_FREQ(vdev->vdev_mlme.des_chan->ch_freq))
        return QDF_STATUS_SUCCESS;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return QDF_STATUS_E_FAILURE;

    ic = vap->iv_ic;
    pdev = ic->ic_pdev_obj;
    ucfg_reg_get_cur_6g_ap_pwr_type(pdev, &cur_6g_ap_pwr_type);

    wlan_reg_set_6ghz_client_type_from_target(pdev);
    /* If the power after disconnect will be same as the user configured power
     * mode, then no need to set the 6G AP power mode.
     */
    best_6g_ap_pwr_type = ieee80211_get_6g_chan_best_power_mode(ic);
    if (cur_6g_ap_pwr_type == best_6g_ap_pwr_type)
        return QDF_STATUS_SUCCESS;

    /* If the power mode changes, the channel list will be updated and a scan is
     * required again. Therefore flush the old scan result in order to force a
     * new scan.
     */
    ucfg_scan_flush_results(pdev, NULL);
    wlan_reg_set_ap_pwr_and_update_chan_list(pdev, best_6g_ap_pwr_type);
    mlme_update_chan_list_and_restart_vaps(ic, true);

    return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_mlme_cm_disconnect_complete(struct wlan_objmgr_vdev *vdev,
                                            struct wlan_cm_discon_rsp *rsp)
{
    QDF_STATUS status = QDF_STATUS_SUCCESS;

    /* Action on disconnect complete(sucess/failure) */
    status = mlme_move_6g_client_to_best_pwr_mode(vdev);
    return status;
}

enum mlme_req mlme_get_active_req_type(struct ieee80211vap *vap)
{
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    return mlme_priv->im_request_type;
}

static void
wlan_mlme_deauth_indication_sta(void *osif, u_int8_t *macaddr,
                                u_int16_t associd, u_int16_t reason,
                                u_int8_t *mldaddr)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)osif;
    struct wlan_objmgr_vdev *vdev;
    struct vdev_mlme_obj *vdev_mlme;

    vdev = vap->vdev_obj;

    vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vdev);
    if (!vdev_mlme) {
        qdf_err("Vdev mlme component object is NULL");
        return;
    }

    /* Ignore deauth/disassoc for link where link delete is in progress */
    if (vdev_mlme->ml_reconfig_started)
        return;

    if (wlan_cm_is_vdev_connected(vdev)) {
        wlan_mlme_cm_stop(vap->vdev_obj, CM_SB_DISCONNECT,
                          REASON_DEAUTH_NETWORK_LEAVING, false);
    }
}

static void wlan_mlme_disassoc_indication_sta(void *osif,
                                              u_int8_t *macaddr,
                                              u_int16_t associd,
                                              u_int32_t reason,
                                              u_int8_t *mldaddr)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)osif;
    struct wlan_objmgr_vdev *vdev;
    struct vdev_mlme_obj *vdev_mlme;

    vdev = vap->vdev_obj;

    vdev_mlme = wlan_vdev_mlme_get_cmpt_obj(vdev);
    if (!vdev_mlme) {
        qdf_err("Vdev mlme component object is NULL");
        return;
    }

    /* Ignore deauth/disassoc for link where link delete is in progress */
    if (vdev_mlme->ml_reconfig_started)
        return;

    if (wlan_cm_is_vdev_connected(vdev)) {
        wlan_mlme_cm_stop(vap->vdev_obj, CM_SB_DISCONNECT,
                          REASON_DISASSOC_NETWORK_LEAVING, false);
    }
}

static void wlan_mlme_beacon_miss(void *osif)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)osif;
    struct wlan_objmgr_vdev *vdev;

    vdev = vap->vdev_obj;

    if (wlan_cm_is_vdev_connected(vdev)) {
        wlan_mlme_cm_stop(vap->vdev_obj, CM_SB_DISCONNECT,
                          REASON_BEACON_MISSED, false);
    }
}

wlan_mlme_event_handler_table sta_cm_mlme_evt_handler = {
    wlan_mlme_join_setting_country_handler,   /* mlme_join_complete_set_country */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    wlan_mlme_txchanswitch_handler,           /* mlme_txchanswitch_complete */
    wlan_mlme_repeater_cac_handler,           /* mlme_repeater_cac_complete */
    NULL,
    wlan_mlme_deauth_indication_sta,
    NULL,
    NULL,
    wlan_mlme_disassoc_indication_sta,
};

static wlan_misc_event_handler_table sta_cm_misc_evt_handler = {
    NULL,                                     /* wlan_channel_change */
    NULL,                                     /* wlan_country_changed */
    NULL,                                     /* wlan_linkspeed */
    NULL,                                     /* wlan_michael_failure_indication */
    NULL,                                     /* wlan_replay_failure_indication */
    wlan_mlme_beacon_miss,                    /* wlan_beacon_miss_indication */
};

static void wlan_mlme_cm_update_advance_filter(struct wlan_objmgr_vdev *vdev,
                                               struct scan_filter *filter)
{
    wlan_cm_update_advance_filter(vdev, filter);
}

static void wlan_mlme_cm_candidate_list_custom_sort(struct wlan_objmgr_vdev *vdev,
                                                    qdf_list_t *candidate_list)
{
    wlan_cm_candidate_list_custom_sort(vdev, candidate_list);
}

void wlan_mlme_cm_event_init(struct ieee80211vap *vap)
{
    wlan_vap_register_mlme_event_handlers(vap,(os_handle_t)vap,&sta_cm_mlme_evt_handler);
    wlan_vap_register_misc_event_handlers(vap,(os_handle_t)vap,&sta_cm_misc_evt_handler);
    wlan_cm_set_candidate_advance_filter_cb(vap->vdev_obj,
                                            wlan_mlme_cm_update_advance_filter);
    wlan_cm_set_candidate_custom_sort_cb(vap->vdev_obj,
                                         wlan_mlme_cm_candidate_list_custom_sort);
}

void wlan_mlme_cm_event_deinit(struct ieee80211vap *vap)
{
    wlan_vap_register_mlme_event_handlers(vap,(os_handle_t)vap,&sta_cm_mlme_evt_handler);
    wlan_vap_register_misc_event_handlers(vap,(os_handle_t)vap,&sta_cm_misc_evt_handler);
}

/*
 * wlan_mlme_assign_new_chan - Assign new_chan based on the scan_entry_chan
 *
 * @ic - Pointer to ic
 * @chan_p - Pointer to chan_p
 * @new_chan - Pointer to new_chan
 * @scan_entry_chan - Pointer to scan_entry_chan
 *
 * Return - void
 */
static void
wlan_mlme_assign_new_chan(struct ieee80211com *ic,
                          struct chan_params *chan_p,
                          struct ieee80211_ath_channel *new_chan,
                          struct ieee80211_ath_channel *scan_entry_chan)
{
    enum reg_6g_ap_type new_ap_pwr_type;
    enum ieee80211_cwm_width ch_width;
    uint16_t bw;
    enum phy_ch_width phy_chan_width;
    uint16_t in_punc_bitmap;

    chan_p->freq = ieee80211_chan2freq(ic, scan_entry_chan);
    chan_p->cfreq2 = scan_entry_chan->ic_vhtop_freq_seg2;
    ch_width = ic->ic_cwm_get_width(ic);
    phy_chan_width = ieee80211_convert_cwm_wd_to_phych_width(ch_width);
    bw = wlan_reg_get_bw_value(phy_chan_width);
    in_punc_bitmap = ieee80211_get_chan_param_punc_bitmap(chan_p);
    new_ap_pwr_type =
                wlan_reg_get_best_pwr_mode(ic->ic_pdev_obj, chan_p->freq,
                                           chan_p->cfreq2, bw,
                                           in_punc_bitmap);
    chan_p->input_6g_pwr_mode =
            wlan_reg_conv_6g_ap_type_to_supported_6g_pwr_types(new_ap_pwr_type);

    set_chan_params_puncture_bitmap(chan_p, scan_entry_chan->op_puncture_bitmap);
    *new_chan = ieee80211_find_channel(ic, chan_p, scan_entry_chan->ic_flags);
}

/*
 * wlan_mlme_assign_new_chan_and_apply_best_pwr_mode - Assign new_chan based
 * on the scan_entry_chan and apply the best 6G power mode
 *
 * @ic - Pointer to ic.
 * @new_chan - Pointer to new_chan
 * @scan_entry_chan - Pointer to scan_entry_chan
 *
 * Return - void
 */
static void
wlan_mlme_assign_new_chan_and_apply_best_pwr_mode(struct ieee80211com *ic,
                                                  struct ieee80211_ath_channel *new_chan,
                                                  struct ieee80211_ath_channel *scan_entry_chan)
{
    struct chan_params chan_p = {0};
    uint16_t in_punc_bitmap;

    wlan_mlme_assign_new_chan(ic, &chan_p, new_chan, scan_entry_chan);
    if (!IEEE80211_IS_CHAN_VALID(new_chan))
         return;

    if (!IEEE80211_ARE_CHANS_EQUAL(scan_entry_chan, new_chan))
         return;

    new_chan = scan_entry_chan;
    in_punc_bitmap = ieee80211_get_chan_punc_bitmap(new_chan);
    ieee80211_apply_best_6g_ap_pwr_mode(ic,
                                        new_chan->ic_freq,
                                        new_chan->ic_vhtop_freq_seg2,
                                        false,
                                        ieee80211_chan2mode(new_chan),
                                        in_punc_bitmap);
}

static void wlan_mlme_vdev_txcsa_start_notify(struct wlan_objmgr_pdev *pdev,
                                              void *object, void *arg)
{
    struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)object;
    struct ieee80211vap *vap = NULL;
    struct ieee80211com *ic = NULL;
    struct ieee80211_ath_channel *scan_entry_chan = arg;
    struct ieee80211_ath_channel new_chan_obj = *scan_entry_chan;
    struct ieee80211_ath_channel *new_chan = &new_chan_obj;

    ic = wlan_pdev_get_mlme_ext_obj(pdev);
    if (!ic)
        return;

    vap = wlan_vdev_get_mlme_ext_obj(vdev);
    if (!vap)
        return;

    if(vap->iv_opmode == IEEE80211_M_HOSTAP ||
       vap->iv_opmode == IEEE80211_M_MONITOR || wlan_rptr_is_psta_vdev(vdev)){
       vap->iv_no_cac = 1;

       /*
        * Configure MAX supported phy for rpt AP VAP.
        * For ind_rpt, AP VAP comes up first based on
        * scan_entry_chan. scan_entry_chan has phy mode
        * that root AP can support. Overwriter it with
        * rpt AP VAP's phy mode
        */
        if (wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {
            *new_chan = ieee80211_use_max_phy_for_rep_ap(vap, scan_entry_chan);
            if (!IEEE80211_IS_CHAN_VALID(new_chan))
                wlan_mlme_assign_new_chan_and_apply_best_pwr_mode(ic,
                                                                  new_chan,
                                                                  scan_entry_chan);
        }
        if (wlan_vdev_chan_config_valid(vdev) == QDF_STATUS_SUCCESS) {
            qdf_err("Seamless chan change triggered for vdev:%u",
                    wlan_vdev_get_id(vap->vdev_obj));
            wlan_pdev_mlme_vdev_sm_seamless_chan_change(ic->ic_pdev_obj,
                                                        vap->vdev_obj, new_chan);
        }
    }
}

int wlan_mlme_start_txchanswitch(wlan_if_t vaphandle, struct ieee80211_ath_channel *scan_entry_chan, u_int32_t timeout)
{

    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211com           *ic = vap->iv_ic;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    u_int32_t                     tx_csa_complete_timeout = 0;
    struct ieee80211_quiet_param  quiet_ic = {0};

    if(!(ic->ic_flags & IEEE80211_F_CHANSWITCH)) {
         wlan_mlme_cm_action_save_history(vap,
                                   CM_TXCHANSWITCH_START,
                                   QDF_STATUS_SUCCESS);
        mlme_priv->im_request_type = MLME_REQ_TXCSA;
        tx_csa_complete_timeout = timeout;
        OS_SET_TIMER(&mlme_priv->im_timeout_timer, tx_csa_complete_timeout);

        if(wlan_mlme_is_txcsa_set(vaphandle)) {
            *ic->ic_chanchange_channel = *scan_entry_chan;
            ic->ic_chanchange_secoffset =
                ieee80211_sec_chan_offset(ic, ic->ic_chanchange_channel);
            ic->ic_chanchange_chwidth =
                ieee80211_get_chan_width(ic->ic_chanchange_channel);
            ic->ic_chanchange_params.freq = scan_entry_chan->ic_freq;
            ic->ic_chanchange_params.cfreq2 = scan_entry_chan->ic_vhtop_freq_seg2;
            set_chan_params_puncture_bitmap(&ic->ic_chanchange_params, scan_entry_chan->op_puncture_bitmap);

            ic->ic_chanchange_tbtt = ic->ic_chan_switch_cnt;
            ic->ic_radio_set_bcn_offload_quiet_mode(ic, &quiet_ic);
            ieee80211com_set_flags(ic, IEEE80211_F_CHANSWITCH);
            wlan_pdev_beacon_update(ic);
        } else {
            /* Change the channel for the AP vaps when TXCSA is set to 0 by the user.
             * Set IEEE80211_F_CHANSWITCH flag for all the AP vaps and ic_chanchange_tbtt to 0.
             * So that AP vaps will not send CSA to its connected clients.
             */
            wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_VDEV_OP,
                                              wlan_mlme_vdev_txcsa_start_notify,
                                              scan_entry_chan, 0, WLAN_MLME_SB_ID);
        }

      return 0;
    }
    wlan_mlme_cm_action_save_history(vap, CM_TXCHANSWITCH_START,
                              QDF_STATUS_E_FAILURE);
    return EBUSY;
}

#if ATH_SUPPORT_DFS
int wlan_mlme_start_repeater_cac(wlan_if_t vaphandle, struct ieee80211_ath_channel *bss_chan)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211com           *ic = vap->iv_ic;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    u_int32_t                     repeater_cac_complete_timeout = 0;

    wlan_mlme_cm_action_save_history(vap, CM_REPEATER_CAC_START,
                              QDF_STATUS_SUCCESS);
    mlme_priv->im_request_type = MLME_REQ_REPEATER_CAC;

    /* Add extra 2000milliseconds so that timeout does not happen before actual CAC timeout */
#define EXTRA_TIME 2000
#define CAC_DELAY 15 /* sec */
    repeater_cac_complete_timeout = 1000*ieee80211_dfs_get_cac_timeout(
            ic, bss_chan)+ EXTRA_TIME;
    OS_SET_TIMER(&mlme_priv->im_timeout_timer, repeater_cac_complete_timeout);

    wlan_cfg80211_dfs_cac_start(vap, ieee80211_dfs_get_cac_timeout(ic, bss_chan)+CAC_DELAY);
#if ATH_SUPPORT_STA_DFS
    IEEE80211_DELIVER_EVENT_CAC_STARTED(vaphandle, bss_chan->ic_freq,
            (ieee80211_dfs_get_cac_timeout(ic, bss_chan)+CAC_DELAY));
#endif
    return 0;
}
#endif

int wlan_mlme_cancel_txchanswitch_timer(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    mlme_priv->im_request_type = MLME_REQ_NONE;
    OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);
    return 0;

}
int wlan_mlme_cancel_repeater_cac_timer(wlan_if_t vaphandle)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    mlme_priv->im_request_type = MLME_REQ_NONE;
    OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);
    return 0;

}

#ifdef WLAN_FEATURE_11BE_MLO
static bool wlan_mlme_mlo_check_partner_scan_entries(
                           struct wlan_objmgr_vdev *vdev,
                           ieee80211_scan_entry_t scan_entry)
{
    struct mlo_partner_info partner_info = {0};
    uint16_t vdev_count = 0;
    struct wlan_objmgr_vdev *wlan_vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct wlan_objmgr_vdev *co_mld_vdev = NULL;
    uint8_t idx = 0;
    uint8_t i = 0;
    uint16_t release_ref_start_idx = 0;
    uint8_t link_ix = 0;
    uint8_t link_id = 0;
    bool link_id_found = false;
    struct scan_cache_entry *link_scan_entry = NULL;

    mlo_get_ml_vdev_list(vdev, &vdev_count, wlan_vdev_list);
    if (!vdev_count) {
         qdf_err("Number of VDEVs under MLD is reported as 0");
         return false;
    }

    if (vdev_count > QDF_ARRAY_SIZE(wlan_vdev_list)) {
        qdf_err("Count of VDEVs %u under MLD exceeds expected limit %zu, investigate",
                 vdev_count, QDF_ARRAY_SIZE(wlan_vdev_list));
        goto err_release_refs;
    }

    /* If the connect is initiated for a single link, then we can return
     * as we already have the scan entry available
     */
    if (vdev_count == 1) {
        if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
             /* This should not occur. If this occurs, would indicate larger issues.
              */
             qdf_err("Error while trying to release VDEV references - investigate.");
        }
        return true;
    }

    if (QDF_IS_STATUS_ERROR(util_scan_get_ml_partner_info(scan_entry, &partner_info))) {
        qdf_err("Unable to fetch the partner info in scan db");
        goto err_release_refs;
    }

    if (!partner_info.num_partner_links) {
        qdf_err("No partner links found in scan entry");
        goto err_release_refs;
    }

    for (idx = 0; idx < vdev_count; idx++) {
        release_ref_start_idx = idx;
        co_mld_vdev = wlan_vdev_list[idx];
        if (co_mld_vdev == NULL) {
            qdf_err("VDEV in MLD VDEV list is NULL");
            goto err_release_refs;
        }

        if (co_mld_vdev == vdev) {
            mlo_release_vdev_ref(wlan_vdev_list[idx]);
            continue;
        }
        /* Skip scan entries for STA bridge vdev */
        if (mlo_is_sta_bridge_vdev(co_mld_vdev)) {
            mlo_release_vdev_ref(wlan_vdev_list[idx]);
            continue;
        }
        link_ix = wlan_vdev_get_link_id(co_mld_vdev);

        link_id_found = false;
        for (i = 0; i < partner_info.num_partner_links; i++) {
             link_id = partner_info.partner_link_info[i].link_id;

             if (link_id != link_ix)
                 continue;

             link_id_found = true;
             link_scan_entry = ieee80211_get_scan_entry_by_ssid_and_bssid(
                                          co_mld_vdev,
                                          scan_entry->ssid,
                                          &partner_info.partner_link_info[i].link_addr);
             if (!link_scan_entry) {
                 qdf_err("no link scan entry found");
                 goto err_release_refs;
             } else {
                 util_scan_free_cache_entry(link_scan_entry);
                 mlo_release_vdev_ref(wlan_vdev_list[idx]);
                 break;
             }
        }

        if (link_id_found == false) {
            qdf_err("Partner info for requested link id: %d not found in scan entry",
                     link_ix);
            goto err_release_refs;
        }
    }

    return true;

err_release_refs:
    if ((ieee80211_mlo_release_vdev_list_refs(wlan_vdev_list,
                    QDF_ARRAY_SIZE(wlan_vdev_list), vdev_count,
                    release_ref_start_idx)) != QDF_STATUS_SUCCESS) {
        /* This should not occur. If this occurs, would indicate larger issues.
         */
        qdf_err("Error while trying to release VDEV references - investigate.");
    }

    return false;
}
#endif

/*
 * Sets that hardware up for joining to the specified
 * infrastructure network. The hardware should
 * set itself up to synchronize to beacons from the specified
 * access point. Once the join has completed, the mlme_join_complete_infra
 * routine should be called.
 *
 * timeout - change from number of beacon intervals to milliseconds to meet win8 requirement
 *           xijin. 2011/05/31
 */
int wlan_mlme_join_infra(wlan_if_t vaphandle, wlan_scan_entry_t bss_entry,
                         u_int32_t timeout, struct qdf_mac_addr *mld_mac)
{
    struct ieee80211vap           *vap = vaphandle;
    ieee80211_scan_entry_t        scan_entry = bss_entry;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    int                           error = 0;
    u_int32_t                     join_timeout_ms;
    bool                          thread_started = false;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;


    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    /* Initialize join state variables */
    ASSERT(mlme_priv->im_request_type == MLME_REQ_NONE);
    mlme_priv->im_request_type = MLME_REQ_JOIN_INFRA;
    atomic_set(&(mlme_priv->im_join_wait_beacon_to_synchronize), 0);
    mlme_priv->im_connection_up = 0;

    if (scan_entry) {
        vdev_mlme->mgmt.mbss_11ax.profile_idx = scan_entry->mbssid_info.profile_num;
        vdev_mlme->mgmt.mbss_11ax.profile_num = ilog2(scan_entry->mbssid_info.profile_count);
        qdf_mem_copy(vdev_mlme->mgmt.mbss_11ax.trans_bssid, scan_entry->mbssid_info.trans_bssid, QDF_MAC_ADDR_SIZE);
    }

#ifdef WLAN_FEATURE_11BE_MLO
    /* If vdev is mlo enabled then add a guard check to see if the STA is
     * able to see the scan entries on all the links for which connect is
     * requested. If the scan entry is not seen, then reject the connection.
     * This check should be removed once ML Probe for Join phase is implemented
     * for STA vdev to get partner info.
     */
    if (mlo_is_mld_sta(vap->vdev_obj) && scan_entry) {
        if (wlan_mlme_mlo_check_partner_scan_entries(vap->vdev_obj, scan_entry) == false) {
            qdf_err("Partner entries not found for ML connection");
            return -1;
        }
    }
#endif
    error = ieee80211_sta_join(vap, scan_entry, &thread_started, mld_mac);
    if (error) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Error %d (0x%08X) in ieee80211_sta_join\n",
            __func__, error, error);
        goto err;
    }

    if (thread_started) {
        /* Set the timeout timer for Join Failure case. */
        join_timeout_ms = mlme_priv->im_timeout;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "Setting Join Timeout timer for %d ms\n", join_timeout_ms);
        OS_SET_TIMER(&mlme_priv->im_timeout_timer, join_timeout_ms);
    }

    return error;

err:
    mlme_priv->im_request_type = MLME_REQ_NONE;
    return error;
}

int wlan_mlme_join_infra_continue(wlan_if_t vaphandle, wlan_scan_entry_t bss_entry, u_int32_t timeout)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211com           *ic = vap->iv_ic;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    struct ieee80211_node         *ni;
    int                           error = 0;
    struct wlan_crypto_params vdev_crypto_params;
    bool restart;
    struct ieee80211_ath_channel ni_chan;
    struct chan_params chan_p = {0};

    ic->ic_flags_ext2 &= ~IEEE80211_FEXT2_RESET_PRECACLIST;
    error = ieee80211_set_ctry_code_continue(ic, false);
    if (error) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Error %d (0x%08X) in ieee80211_sta_join\n",
            __func__, error, error);
        mlme_priv->im_request_type = MLME_REQ_NONE;
        return error;
    }
    /* Skip for WDS bridge vdev */
    if (!mlo_is_sta_bridge_vdev(vap->vdev_obj) && bss_entry) {
        if (!wlan_scan_entry_rsnparams(vaphandle, bss_entry, &vdev_crypto_params)) {
            vaphandle->mixed_encryption_mode = 0;
            if(wlan_crypto_is_htallowed(vap->vdev_obj, NULL))
                vaphandle->mixed_encryption_mode = 1;
        }
    }

    /* iv_bss is valid only after ieee80211_sta_join */
    ni = vap->iv_bss;
    ni->ni_assocstarttime = OS_GET_TICKS();
    /*
     * issue a vap start request to resource manager.
     * if the function returns EOK (0) then its ok to change the channel synchronously
     * if the function returns EBUSY  then resource manager will
     * switch channel asynchronously and post an event event handler registred by vap and
     * vap handler will intern call the wlan_mlme_join_infra_continue .
     */
    mlme_priv->im_timeout = timeout;
    restart = ieee80211_vap_is_connected(vap);
    if (restart) {
        vap->iv_quick_reconnect = true;
    } else {
        vap->iv_quick_reconnect = false;
    }

     /* In station mode, if rejoin is ongoing, VDEV SM is in SUSPEND_RESTART substate
      * it expects DISCONNECT_COMPLETE to move to START state and sends restart
      * req
      */
    if (restart) {
          wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj,
                                         WLAN_VDEV_SM_EV_DISCONNECT_COMPLETE, 0, NULL);
    }
    else {
        chan_p.freq = ni->ni_ch_param.ni_freq;
        chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
        set_chan_params_puncture_bitmap(&chan_p, ni->ni_ch_param.puncture_bitmap);

        ni_chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                               ni->ni_ch_param.ni_bss_phymode);

        if (!IEEE80211_IS_CHAN_VALID(&ni_chan))
            return -EINVAL;

          ieee80211_update_vdev_chan(vap->vdev_obj->vdev_mlme.des_chan, &ni_chan);
          error = wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj,
                                                WLAN_VDEV_SM_EV_START, 0, NULL);
          if (error) {
              qdf_err("Psoc:%d, vdev:%u EV_START failed",
                      wlan_psoc_get_id(wlan_vdev_get_psoc(vap->vdev_obj)),
                      wlan_vdev_get_id(vap->vdev_obj));
              mlme_priv->im_request_type = MLME_REQ_NONE;
          }
    }

    return error;
}

void ieee80211_mlme_join_infra_continue(struct ieee80211vap *vap, int32_t status)
{
    struct ieee80211com           *ic = vap->iv_ic;
    struct ieee80211_node         *ni;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    u_int32_t                     join_timeout_ms;
    cm_ext_t *cm_ext_handle = wlan_cm_get_ext_hdl(vap->vdev_obj);
    struct wlan_vdev_state_event event;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;

    if (wlan_reg_is_6ghz_chan_freq(vap->iv_bsschan->ic_freq))
        ieee80211_send_tpc_power_cmd(vap);

    if (!cm_ext_handle)
        return;

    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }

    if (mlme_priv->im_request_type != MLME_REQ_JOIN_INFRA) {
        cm_ext_handle->cm_conn_rsp.connect_status = QDF_STATUS_E_FAILURE;
        cm_ext_handle->cm_conn_rsp.reason = CM_JOIN_FAILED;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s : im_request_type != JOIN_INFRA\n",
            __func__);
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, IEEE80211_STATUS_UNSPECIFIED);
        return;
    }

    if (status != EOK) {
        cm_ext_handle->cm_conn_rsp.connect_status = QDF_STATUS_E_FAILURE;
        cm_ext_handle->cm_conn_rsp.reason = CM_JOIN_FAILED;
        mlme_priv->im_request_type = MLME_REQ_NONE;
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, IEEE80211_STATUS_UNSPECIFIED);
        return;
     }

    /* iv_bss is valid only after ieee80211_sta_join */
    ni = vap->iv_bss;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s ni=%02X:%02X:%02X:%02X:%02X:%02X\n",
        __func__,
        ni->ni_macaddr[0], ni->ni_macaddr[1], ni->ni_macaddr[2],
        ni->ni_macaddr[3], ni->ni_macaddr[4], ni->ni_macaddr[5]);

    /* Update erp info */
    if (ni->ni_erp & IEEE80211_ERP_USE_PROTECTION)
        ieee80211com_set_flags(ic, IEEE80211_F_USEPROT);
    else
        ieee80211com_clear_flags(ic, IEEE80211_F_USEPROT);
    ieee80211_set_protmode(ic);

    if(ni->ni_erp & IEEE80211_ERP_LONG_PREAMBLE)
        ieee80211com_set_flags(ic, IEEE80211_F_USEBARKER);
    else
        ieee80211com_clear_flags(ic, IEEE80211_F_USEBARKER);

    /* 11AX TODO (Phase II) - Update this for 11ax if required */

    /* Update slot time info */
    ieee80211_set_shortslottime(ic,
                                IEEE80211_IS_CHAN_A(vap->iv_bsschan) ||
                                IEEE80211_IS_CHAN_11NA(vap->iv_bsschan) ||
                                (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));

    /* In station mode, set init_in_progress to 0 before moving to join state.
     * This is added in corner case where wifi down/detect is issues when
     * vap start is in progress. This is leading to init_in_progess not getting
     * reset and leading to host not sending vap_stop before vap_delete.
     */

    /* update the uplink bssid */
    event.state = VDEV_STATE_CONNECTED;
    qdf_mem_copy(event.bssid, ni->ni_bssid, IEEE80211_ADDR_LEN);

    if (rx_ops && rx_ops->son_rx_ops.deliver_event)
        rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_VDEV_STATE, &event);

#ifdef ATH_SUPPORT_TxBF
    ieee80211_init_txbf(ic, ni);
#endif

    /* When stict_passive scan is enabled, do not send probe request in passive channels.
       Send join success here and transit to authentication state.
       When iv_wps_mode is set, probe request has to be sent as it is required for
       WPS-Push button exchange*/

    if ((ic->ic_strict_pscan_enable && IEEE80211_IS_CHAN_PASSIVE(ic->ic_curchan))
                                                            && !vap->iv_wps_mode) {
        mlme_priv->im_request_type = MLME_REQ_NONE;
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, IEEE80211_STATUS_SUCCESS);
        return;
    }
    else {
        /* Send a direct probe to increase the odds of receiving a probe response */
        ieee80211_send_probereq(ni, vap->iv_myaddr, ni->ni_bssid,
                ni->ni_bssid, ni->ni_essid, ni->ni_esslen,
                vap->iv_opt_ie.ie, vap->iv_opt_ie.length);
    }
    mlme_priv->im_join_is_timeout = 0;

    if (wlan_vdev_mlme_is_link_sta_vdev(vap->vdev_obj)) {
        mlme_priv->im_request_type = MLME_REQ_NONE;
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, IEEE80211_STATUS_SUCCESS);
       return;
    }

    /* Set the timeout timer for Join Failure case. */
    join_timeout_ms = mlme_priv->im_timeout;//IEEE80211_TU_TO_MS(mlme_priv->im_timeout * ni->ni_intval);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "Setting Join Timeout timer for %d ms\n", join_timeout_ms);
    OS_SET_TIMER(&mlme_priv->im_timeout_timer, join_timeout_ms);
    /* Set the appropriate filtering function and wait for Join Beacon */
    MLME_WAIT_FOR_BSS_JOIN(mlme_priv);
}

int wlan_assoc_external_auth(wlan_if_t vap, u_int16_t action)
{
#if UMAC_SUPPORT_CFG80211
    struct net_device *dev;
    ieee80211_ssid ssid;
    osif_dev *osifp;

    qdf_debug("action %d", action);
    dev = OSIF_TO_NETDEV(vap->iv_ifp);
    osifp = (osif_dev *)((os_if_t)vap->iv_ifp);
    wlan_get_desired_ssidlist(vap, &ssid, 1);

    if (vap->iv_cfg80211_create) {
        struct cfg80211_external_auth_params params;

        if(ssid.len)
           qdf_mem_copy(params.ssid.ssid, ssid.ssid, ssid.len);

        params.ssid.ssid_len = ssid.len;
        if (action == IEEE80211_EXTERNAL_AUTH_START)
            params.action = NL80211_EXTERNAL_AUTH_START;
        else
            params.action = NL80211_EXTERNAL_AUTH_ABORT;

        wlan_vap_get_bssid(vap, params.bssid);

        params.key_mgmt_suite = qdf_cpu_to_be32(WLAN_AKM_SUITE_SAE);

#ifdef ENABLE_CFG80211_BACKPORTS
        qdf_mem_copy(params.mld_addr, vap->ap_mld_addr.bytes, QDF_MAC_ADDR_SIZE);
        if (osifp->mldev && osifp->mldev->mld_dev)
            cfg80211_external_auth_request(osifp->mldev->mld_dev, &params, GFP_ATOMIC);
        else
            cfg80211_external_auth_request(dev, &params, GFP_ATOMIC);
#else
        cfg80211_external_auth_request(dev, &params, GFP_ATOMIC);
#endif
    } else {
#ifndef MGMT_FRAM_TAG_SIZE
#define MGMT_FRAM_TAG_SIZE 30
#endif
        struct ev_sta_external_auth_params params;
        union iwreq_data wreq;
        char *buf = NULL, *tag = NULL;
        size_t bufsize = MGMT_FRAM_TAG_SIZE + sizeof(struct ev_sta_external_auth_params);

        buf = (char *)qdf_mem_malloc(bufsize);
        if(buf == NULL)
           return 0;

        qdf_mem_zero(&wreq, sizeof(wreq));
        qdf_mem_zero(&params, sizeof(params));
        wreq.data.length = bufsize;
        wreq.data.flags = 0;
        tag = "EXTERNAL_AUTH_INDICATION";

        snprintf(buf, MGMT_FRAM_TAG_SIZE, "%s", tag);

        if(ssid.len)
            qdf_mem_copy(params.ssid, ssid.ssid, ssid.len);
        params.ssid_len = ssid.len;
        params.action = action;
        wlan_vap_get_bssid(vap, params.bssid);
        params.keymgmt_suite = qdf_cpu_to_be32(WLAN_AKM_SUITE_SAE);
        qdf_mem_copy(buf+MGMT_FRAM_TAG_SIZE, &params, sizeof(struct ev_sta_external_auth_params));
        WIRELESS_SEND_EVENT(dev, IWEVCUSTOM, &wreq, buf);
    }

#else
    /* TODO: WEXT implementation needs to be done */
#endif
    return 0;
}

int wlan_mlme_auth_request(wlan_if_t vaphandle, u_int32_t timeout)
{

    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    struct ieee80211_node         *ni = vap->iv_bss; /* bss node */
    int                           error = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    /* wait for auth seq number 2 (open response or shared challenge) */
    ASSERT(mlme_priv->im_request_type == MLME_REQ_NONE);
    mlme_priv->im_request_type = MLME_REQ_AUTH;

    if (vap->iv_sta_external_auth_enabled) {

        wlan_assoc_external_auth(vaphandle, IEEE80211_EXTERNAL_AUTH_START);
        /* Set the timeout timer for authenticate failure case */
        qdf_timer_sync_cancel(&vap->iv_sta_external_auth_timer);
        wlan_assoc_sm_sae_max_auth_retry(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj), true);
        qdf_timer_start(&vap->iv_sta_external_auth_timer,
                        IEEE80211_MLME_EXTERNAL_AUTH_TIMEOUT);
    } else
    {
        mlme_priv->im_expected_auth_seq_number = IEEE80211_AUTH_OPEN_RESPONSE;

        wlan_assoc_sm_sae_max_auth_retry(wlan_mlme_get_assoc_sm_handle(vap->vdev_obj), true);
        /*  Send the authentication packet */
        error = ieee80211_send_auth(ni, IEEE80211_AUTH_SHARED_REQUEST, 0, NULL, 0, NULL, NULL);
        if (error) {
            goto err;
        }
        /* Set the timeout timer for authenticate failure case */
        OS_SET_TIMER(&mlme_priv->im_timeout_timer, timeout);
        wlan_node_set_peer_state(ni, WLAN_AUTH_STATE);
    }
    return error;

err:
    mlme_priv->im_request_type = MLME_REQ_NONE;
    OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);
    return error;
}

int wlan_mlme_assoc_request(wlan_if_t vaphandle, u_int32_t timeout)
{
    IEEE80211_DPRINTF(vaphandle, IEEE80211_MSG_MLME, "%s\n", __func__);
    return mlme_assoc_reassoc_request(vaphandle, 0, NULL, timeout);
}

int wlan_mlme_reassoc_request(wlan_if_t vaphandle, u_int8_t *prev_bssid, u_int32_t timeout)
{
    IEEE80211_DPRINTF(vaphandle, IEEE80211_MSG_MLME, "%s\n", __func__);
    return mlme_assoc_reassoc_request(vaphandle, 1, prev_bssid, timeout);
}

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * wlan_mlme_sta_rem_rej_link()- In case of link rejection, remove the links
 * rejected by ML-AP in the ML-STAs partner info.
 * @vaphandle: Assoc vap pointer
 * @pinfo: MLO partner info details
 * @reassoc: Indicate if assoc or reassoc
 * @assoc_frm: Pointer to assoc response frame
 * @frame_len: Assoc response frame length.
 *
 * Return: QDF_STATUS_SUCCESS is rejected link removed, else failure
 */
static
QDF_STATUS wlan_mlme_sta_rem_rej_link(wlan_if_t vaphandle,
                                      struct mlo_partner_info *pinfo,
                                      uint8_t reassoc, uint8_t *assoc_frm,
                                      uint16_t frame_len)
{
    uint8_t i;
    wbuf_t link_frame;
    struct ieee80211_frame *wh;
    u_int8_t *frm;
    struct element_info assoc_frame = {0};
    uint16_t link_len;
    u_int16_t capinfo, status, associd;

    if (!wlan_vdev_mlme_is_assoc_sta_vdev(vaphandle->vdev_obj)) {
        mlo_get_assoc_rsp(vaphandle->vdev_obj, &assoc_frame);
    } else {
        assoc_frame.ptr = assoc_frm;
        assoc_frame.len = frame_len;
    }

    if (!assoc_frame.ptr || !assoc_frame.len)
        return QDF_STATUS_E_INVAL;

    for (i = 0; i < pinfo->num_partner_links; i++) {
        link_frame = wbuf_alloc(vaphandle->iv_ic->ic_osdev, WBUF_TX_MGMT,
                MAX_TX_RX_PACKET_SIZE);
        if (!link_frame)
            return QDF_STATUS_E_NOMEM;

        if (QDF_IS_STATUS_ERROR(util_gen_link_assoc_rsp(
                        assoc_frame.ptr, assoc_frame.len,
                        reassoc, pinfo->partner_link_info[i].link_id,
                        pinfo->partner_link_info[i].link_addr,
                        (uint8_t *)wbuf_header(link_frame),
                        MAX_TX_RX_PACKET_SIZE, (size_t *)&link_len))) {
            wbuf_release(vaphandle->iv_ic->ic_osdev, link_frame);
            mlo_err("Unable to generate link specific assoc response");
            return QDF_STATUS_E_INVAL;
        }

        if (!link_len || link_len > MAX_TX_RX_PACKET_SIZE) {
            wbuf_release(vaphandle->iv_ic->ic_osdev, link_frame);
            mlo_err("Link_frame len check failed %u", link_len);
            return QDF_STATUS_E_INVAL;
        }

        wbuf_set_pktlen(link_frame, link_len);
        wh = (struct ieee80211_frame *) wbuf_header(link_frame);
        frm = (u_int8_t *)&wh[1];

        capinfo = le16toh(*(u_int16_t *)frm); frm += 2;
        status = le16toh(*(u_int16_t *)frm); frm += 2;
        associd = le16toh(*(u_int16_t *)frm); frm += 2;

        mlo_debug("Link info link_id:%u, num_links:%u mac:" QDF_MAC_ADDR_FMT,
                pinfo->partner_link_info[i].link_id, pinfo->num_partner_links,
                QDF_MAC_ADDR_REF(pinfo->partner_link_info[i].link_addr.bytes));

        if (QDF_IS_STATUS_ERROR(status)) {
            mlo_info("Link rejected freq:%u, sta:%u assoc:%u link_id:%u, mac: " QDF_MAC_ADDR_FMT,
                    pinfo->partner_link_info[i].chan_freq, status, associd,
                    pinfo->partner_link_info[i].link_id,
                    QDF_MAC_ADDR_REF(pinfo->partner_link_info[i].link_addr.bytes));

            /* Rearrange the partner link info */
            if (i < pinfo->num_partner_links-1) {
                qdf_mem_copy(&pinfo->partner_link_info[i],
                             &pinfo->partner_link_info[pinfo->num_partner_links-1],
                             sizeof(*pinfo->partner_link_info));
                i--;
            }
            qdf_mem_zero(&pinfo->partner_link_info[pinfo->num_partner_links-1],
                         sizeof(*pinfo->partner_link_info));
            pinfo->num_partner_links--;
        }

        wbuf_release(vaphandle->iv_ic->ic_osdev, link_frame);
    }

    return QDF_STATUS_SUCCESS;
}

#ifdef WLAN_MLO_MULTI_CHIP
QDF_STATUS wlan_mlme_sta_is_peer_adjacent(wlan_if_t vap,
                                          struct mlo_partner_info *pinfo)
{
    uint8_t i;
    uint8_t psoc_id[2];
    uint8_t adjacent = 0;
    struct wlan_objmgr_vdev *link_vdev;

    /*
     * If 2 links form MLD and are not adjacent, then reject that association
     */

    if (!wlan_vdev_mlme_is_assoc_sta_vdev(vap->vdev_obj))
        return QDF_STATUS_SUCCESS;

    /* Bridge peer required for 2 link MLO only */
    if (pinfo->num_partner_links != 2)
        return QDF_STATUS_SUCCESS;

    for (i = 0; i < pinfo->num_partner_links; i++) {
        link_vdev = mlo_get_vdev_by_link_id(
                        vap->vdev_obj,
                        pinfo->partner_link_info[i].link_id,
                        WLAN_MLO_MGR_ID);

        if (!link_vdev) {
            mlo_err("Unable to get VDEV for link ID %u",
                    pinfo->partner_link_info[i].link_id);
            return QDF_STATUS_E_INVAL;
        }
        psoc_id[i] = wlan_vdev_get_psoc_id(link_vdev);
        wlan_objmgr_vdev_release_ref(link_vdev, WLAN_MLO_MGR_ID);
        mlo_debug("partner i:%u, mac: " QDF_MAC_ADDR_FMT, i,
                  QDF_MAC_ADDR_REF(pinfo->partner_link_info[i].link_addr.bytes));
    }

    mlo_chip_adjacent(psoc_id[0], psoc_id[1], &adjacent);
    if (!adjacent)
        return QDF_STATUS_E_FAILURE;

    return QDF_STATUS_SUCCESS;
}
#else
QDF_STATUS wlan_mlme_sta_is_peer_adjacent(wlan_if_t vap,
                                          struct mlo_partner_info *pinfo)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_MLO_MULTI_CHIP */
#endif /* WLAN_FEATURE_11BE_MLO */

int wlan_mlme_link_assoc(wlan_if_t vaphandle, u_int32_t timeout, bool reassoc)
{
    struct element_info assoc_frame = {0};
    wbuf_t link_frame;
    uint16_t link_len;
    struct qdf_mac_addr link_addr;
    int status;
    struct ieee80211_mlme_priv *mlme_priv = vaphandle->iv_mlme_priv;
    uint8_t link_id;
    int subtype;

    IEEE80211_DPRINTF(vaphandle, IEEE80211_MSG_MLME, "%s\n", __func__);
    mlo_get_assoc_rsp(vaphandle->vdev_obj, &assoc_frame);
    if (!assoc_frame.len) {
        qdf_err("Assoc resp frame not found");
        return -EINVAL;
    }
    link_frame = wbuf_alloc(vaphandle->iv_ic->ic_osdev, WBUF_TX_MGMT,
                            MAX_TX_RX_PACKET_SIZE);
    if (!link_frame)
        return -EINVAL;

    qdf_mem_copy(link_addr.bytes, wlan_vdev_mlme_get_macaddr(vaphandle->vdev_obj),
                 QDF_MAC_ADDR_SIZE);
    link_id = wlan_vdev_get_link_id(vaphandle->vdev_obj);
    if (QDF_IS_STATUS_ERROR(util_gen_link_assoc_rsp(
                               assoc_frame.ptr, assoc_frame.len,
                               reassoc, link_id,
                               link_addr, (uint8_t *)wbuf_header(link_frame),
                               MAX_TX_RX_PACKET_SIZE, (size_t *)&link_len))) {
        qdf_err("Unable to generate link specific assoc response");
        return -EINVAL;
    }

    if (!link_len) {
        qdf_err("Link assoc frame generation failed");
        wbuf_release(vaphandle->iv_ic->ic_osdev, link_frame);
        return -EINVAL;
    }

    wbuf_set_pktlen(link_frame, link_len);
    mlme_priv->im_request_type = reassoc ? MLME_REQ_REASSOC : MLME_REQ_ASSOC;
    subtype = reassoc ? IEEE80211_FC0_SUBTYPE_REASSOC_RESP : IEEE80211_FC0_SUBTYPE_ASSOC_RESP;
    status =  ieee80211_recv_asresp(vaphandle->iv_bss, link_frame,
                                    subtype);

    wbuf_release(vaphandle->iv_ic->ic_osdev, link_frame);
    return status;
}

/*
 * Private APIs
 *      - private to mlme implementation
 *      - called by mgmt frame processing (ieee80211_mgmt_input.c)
 */

/* Confirmations */
void ieee80211_mlme_join_complete_infra(struct ieee80211_node *ni)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    if ((mlme_priv->im_request_type == MLME_REQ_JOIN_INFRA) && (MLME_STOP_WAITING_FOR_JOIN(mlme_priv) == 1))
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

        /* Request complete */
        mlme_priv->im_request_type = MLME_REQ_NONE;

        /*
         * We have received the beacon that synchronises us with the BSS.
         * We don't care whether the Timer got cancelled or not. The macro
         * HW_STOP_WAITING_FOR_JOIN synchronizes us with the cancel operation
         */
        OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);

        /* Call MLME confirmation handler */
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, IEEE80211_STATUS_SUCCESS);
    }
    else
    {
        if (mlme_priv->im_request_type != MLME_REQ_NONE) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Failed: im_request_type=%d\n",
                __func__,
                mlme_priv->im_request_type);
        }

    }

    /* start SW bmiss. will be here for every beacon received from our AP  */
    mlme_sta_swbmiss_timer_start(vap);
}

#ifdef WLAN_FEATURE_11BE_MLO
static QDF_STATUS
wlan_mlme_fill_ml_params(struct ieee80211_node *ni, wbuf_t wbuf, uint8_t *t2lmie)
{
    struct ieee80211_frame *wh;
    u_int8_t *frm, *efrm;
    struct ieee80211vap *vap = ni->ni_vap, *assoc_vap = NULL;
    struct ieee80211vap *link_vap = NULL;
    struct wlan_objmgr_vdev *assoc_vdev = NULL;
    struct wlan_mlo_dev_context *mlo_dev_ctx = NULL;
    struct wlan_mlo_bridge_sta *bridge_sta_ctx = NULL;
    struct element_info assoc_rsp = {0};
    uint8_t *ml_ie = NULL;
    qdf_size_t ml_ie_len = 0;
    QDF_STATUS status;
    uint8_t i = 0, idx = 0, ix = 0, j = 0;
    struct wlan_objmgr_vdev *link_vdev;
    struct wlan_objmgr_vdev *tmp_link_vdevs[WLAN_UMAC_MLO_MAX_VDEVS] = { NULL };
    int rptinglinkidx;
    enum wlan_ml_variant variant;
    bool mldcapfound = false;
    uint16_t mldcap = 0;
    bool emlcapfound = false;
    bool bridge_vap_exists = false;
    uint16_t emlcap = 0;
    uint8_t bridge_part_idx, bridge_link_idx;
#ifdef WLAN_FEATURE_11BE
    struct wlan_t2lm_onging_negotiation_info t2lm_negotiation = {0};
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_mlo_peer_t2lm_policy *t2lm_policy;
    struct wlan_t2lm_info *negotiated_t2lm;
    struct wlan_t2lm_info *ongoing_t2lm;
    uint8_t dir = 0;
    uint32_t frame_len = 0;
    struct ieee80211com *ic = NULL;
    struct wlan_objmgr_vdev *mpsta_vdev = NULL;
    struct qdf_mac_addr ap_mld_mac;
    struct wlan_mlo_peer_context *mpsta_ml_peer = NULL;
#endif /* WLAN_FEATURE_11BE */
    uint8_t primary_umac_set = 0;
    struct wlan_mlo_link_peer_entry *peer_entry;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    frm = (u_int8_t *)&wh[1];
    efrm = wbuf_header(wbuf) + wbuf_get_pktlen(wbuf);

    if(efrm <= frm)
        return QDF_STATUS_E_FAILURE;

#ifdef WLAN_FEATURE_11BE
    frame_len = efrm - frm;
#endif /* WLAN_FEATURE_11BE */

    if (wlan_vdev_mlme_is_assoc_sta_vdev(vap->vdev_obj)) {
        assoc_vdev = vap->vdev_obj;
        assoc_vap = vap;
        assoc_rsp.ptr = frm + WLAN_ASSOC_RSP_IES_OFFSET;
        assoc_rsp.len = efrm-frm;
        assoc_rsp.len -= WLAN_ASSOC_RSP_IES_OFFSET;
    } else {
        assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vap->vdev_obj);
        if (!assoc_vdev) {
            qdf_err("assoc vdev not found");
            return QDF_STATUS_E_FAILURE;
        }
        assoc_vap = wlan_vdev_mlme_get_ext_hdl(assoc_vdev);
        if (!assoc_vap) {
            qdf_err("assoc vap not found");
            return QDF_STATUS_E_FAILURE;
        }
        mlo_get_assoc_rsp(vap->vdev_obj, &assoc_rsp);
        assoc_rsp.ptr += WLAN_ASSOC_RSP_IES_OFFSET;
        assoc_rsp.len -= WLAN_ASSOC_RSP_IES_OFFSET;
    }

    status = util_find_mlie(assoc_rsp.ptr, assoc_rsp.len,
                            &ml_ie, &ml_ie_len);
    if (QDF_IS_STATUS_ERROR(status) || !ml_ie) {
        qdf_err("Error finding ML IE in assoc response");
        return QDF_STATUS_E_FAILURE;
    }

    status = util_get_mlie_variant(ml_ie, ml_ie_len,
                                   (int *)&variant);
    if (status != QDF_STATUS_SUCCESS) {
        qdf_err("Unable to get Multi-Link element variant");
        return status;
    }

    if (variant != WLAN_ML_VARIANT_BASIC) {
        qdf_err("Invalid Multi-Link element variant %u",
                 variant);
        return status;
    }

    status = util_get_bvmlie_eml_cap(ml_ie, ml_ie_len, &emlcapfound, &emlcap);
    if (status != QDF_STATUS_SUCCESS) {
        mloie_err_rl("Error when trying to get EML capabilities in Multi-Link element for ni with MAC address "
                QDF_MAC_ADDR_FMT ,QDF_MAC_ADDR_REF(ni->ni_macaddr));
        return status;
    }

    if (ni->peer_obj->mlo_peer_ctx)
        ml_peer = ni->peer_obj->mlo_peer_ctx;

    if (!ml_peer) {
        qdf_err("ML peer not found");
        return QDF_STATUS_E_FAILURE;
    }

    if (emlcapfound) {
        ieee80211_parse_peer_eml_cap(ni, emlcap);
        if (ml_peer)
            qdf_mem_copy(&ml_peer->mlpeer_emlcap, &ni->mlpeer_eml_cap,
                         sizeof(struct wlan_mlo_eml_cap));
    }

    status = util_get_bvmlie_mldcap(ml_ie, ml_ie_len, &mldcapfound, &mldcap);
    if (status != QDF_STATUS_SUCCESS) {
        mloie_err_rl("Error when trying to get MLD capabilities in Multi-Link element for ni with MAC address "
                QDF_MAC_ADDR_FMT ,QDF_MAC_ADDR_REF(ni->ni_macaddr));
        return status;
    }

    if (mldcapfound) {
        ieee80211_parse_peer_mld_cap(ni, mldcap);
        ieee80211_parse_t2lm_enable_val(ni, mldcap);

#ifdef WLAN_FEATURE_11BE
        if (ml_peer && wlan_vdev_mlme_is_assoc_sta_vdev(vap->vdev_obj)) {
            ml_peer->t2lm_policy.t2lm_enable_val = ni->ni_t2lm_enable_val;

            t2lm_policy = &ni->peer_obj->mlo_peer_ctx->t2lm_policy;
            /* Initialize the direction to invalid */
            for(dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
                t2lm_policy->t2lm_negotiated_info.t2lm_info[dir].direction =
                    WLAN_T2LM_INVALID_DIRECTION;
            }

            /*Set the default TID-to-link mapping for BIDI */
            negotiated_t2lm =
                &t2lm_policy->t2lm_negotiated_info.t2lm_info[WLAN_T2LM_BIDI_DIRECTION];
            negotiated_t2lm->direction = WLAN_T2LM_BIDI_DIRECTION;
            negotiated_t2lm->default_link_mapping = true;

            if (ni->ni_t2lm_enable_val == WLAN_T2LM_MAP_ALL_TIDS_TO_SAME_LINK_SET) {
                /* T2LM IE in the Assoc response frame indicates,
                 * 1) If expected duration is present in the T2LM IE, then STA
                 *    need to use this mapping utill a new TID-to-link mapping
                 *    advertised or negotiated.
                 * 2) Else, AP provided the preferred link mapping, and default
                 *    link mapping remain established utill a new TID-to-link
                 *    mapping advertised or negotiated.
                 */
                if (t2lmie) {
                    ieee80211_parse_t2lm_ie(ni, &t2lm_negotiation, t2lmie, frame_len);

                    if ((t2lm_negotiation.t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction != WLAN_T2LM_INVALID_DIRECTION) &&
                            t2lm_negotiation.t2lm_info[WLAN_T2LM_BIDI_DIRECTION].expected_duration_present) {
                        /* Use the established mapping */
                        qdf_mem_copy(&ml_peer->t2lm_policy.ongoing_tid_to_link_mapping, &t2lm_negotiation,
                                sizeof(struct wlan_t2lm_onging_negotiation_info));
                        ieee80211_copy_negotiated_t2lm_info_to_peer_ctx(ml_peer, &t2lm_negotiation, true);
                    } else {
                        /* Default T2M mapping will be used in this case */

                        /* Reset the onging t2lm to default value.*/
                        for(dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
                            t2lm_policy->ongoing_tid_to_link_mapping.t2lm_info[dir].direction =
                                WLAN_T2LM_INVALID_DIRECTION;
                        }

                        ongoing_t2lm =
                            &t2lm_policy->ongoing_tid_to_link_mapping.t2lm_info[WLAN_T2LM_BIDI_DIRECTION];
                        ongoing_t2lm->direction = WLAN_T2LM_BIDI_DIRECTION;
                        ongoing_t2lm->default_link_mapping = true;
                    }
                } else {
                    /* T2LM negotiation successfull */
                    ieee80211_copy_negotiated_t2lm_info_to_peer_ctx(ml_peer,
                            &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping, true);
                }
            }
        }
#endif /* WLAN_FEATURE_11BE */
    }

    status = util_get_bvmlie_persta_partner_info(ml_ie, ml_ie_len, &ni->ml_info);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Unable to find per-sta profile in ML IE");
        return status;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     * This function is called as part of assoc resp processing.
     * If a STA link is found to be rejected as part of link reject based
     * on STATUS code, then remove that link from ni->ml_info.
     * Thereby STA assoc will continue only with AP suggested links.
     */
    status = wlan_mlme_sta_rem_rej_link(vap, &ni->ml_info, 0, frm, frame_len);
    if (QDF_IS_STATUS_ERROR(status)) {
        qdf_err("Rejecting assoc due to failure in handling link rejection");
        return status;
    }
#endif

    mlo_dev_ctx = assoc_vdev->mlo_dev_ctx;
    if (!mlo_dev_ctx) {
        qdf_err("ML Dev Ctx is null");
        return QDF_STATUS_E_FAILURE;
    }

    if (mlo_dev_ctx && mlo_dev_ctx->bridge_sta_ctx){
        bridge_sta_ctx = mlo_dev_ctx->bridge_sta_ctx;
        /* If topology has bridge vap store the bridge partner info
           and bridge link info, it will be used during peer create/assoc */
        if (bridge_sta_ctx->bridge_vap_exists) {
            if (ni->ml_info.num_partner_links != 1) {
                qdf_err("WDS Bridge OTA link should have single partner link, current count %d ",
                        ni->ml_info.num_partner_links);
                return QDF_STATUS_E_FAILURE;
            }
            bridge_part_idx = bridge_sta_ctx->bridge_partners.num_partner_links;

            if (bridge_part_idx > 1) {
                qdf_err("WDS Bridge Peer can store total 2-OTA partner links, current %d ",
                        bridge_part_idx);
                return QDF_STATUS_E_FAILURE;
            }
            qdf_mem_copy(&bridge_sta_ctx->bridge_partners.partner_link_info[bridge_part_idx],
                         &ni->ml_info.partner_link_info[0], sizeof(struct mlo_link_info));
            bridge_sta_ctx->bridge_partners.num_partner_links++;

            /* Add the bridge partner info at next location */
            bridge_link_idx = ni->ml_info.num_partner_links;
            ni->ml_info.partner_link_info[bridge_link_idx].link_id = bridge_sta_ctx->bridge_link_id;
            qdf_mem_copy(ni->ml_info.partner_link_info[bridge_link_idx].link_addr.bytes,
                         &mlo_dev_ctx->mld_addr, QDF_MAC_ADDR_SIZE);
            /* Account for bridge partner here */
            ni->ml_info.num_partner_links++;
            bridge_vap_exists = true;
        }
    }
    /* Add self as partner it will act as reporting link*/
    ni->ml_info.num_partner_links++;
    ml_peer->max_links = ni->ml_info.num_partner_links;
    rptinglinkidx = ni->ml_info.num_partner_links - 1;

    ni->ml_info.partner_link_info[rptinglinkidx].link_id =
                                    wlan_vdev_get_link_id(assoc_vdev);
    qdf_mem_copy(ni->ml_info.partner_link_info[rptinglinkidx].link_addr.bytes,
                 assoc_vap->iv_bss->ni_macaddr,
                 QDF_MAC_ADDR_SIZE);

    if (QDF_IS_STATUS_ERROR(wlan_mlme_sta_is_peer_adjacent(vap, &ni->ml_info))) {
        qdf_err("No adjacent link found, reject assoc");
        return QDF_STATUS_E_FAILURE;
    }

    if (wlan_vdev_mlme_is_assoc_sta_vdev(vap->vdev_obj)) {
        for (i = 0; i < ni->ml_info.num_partner_links; i++) {
             link_vdev = mlo_get_vdev_by_link_id(
                            vap->vdev_obj,
                            ni->ml_info.partner_link_info[i].link_id,
                            WLAN_MLO_MGR_ID);

             if (!link_vdev) {
                 qdf_err("Unable to get VDEV for link ID %u",
                         ni->ml_info.partner_link_info[i].link_id);
                 goto err;
             }
             tmp_link_vdevs[j++] = link_vdev;
        }

        for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
             if (!tmp_link_vdevs[i] || (tmp_link_vdevs[i] == vap->vdev_obj))
                 continue;

             link_vap = wlan_vdev_get_mlme_ext_obj(tmp_link_vdevs[i]);
             if (!link_vap)
                 continue;
             ieee80211_update_vdev_chan(tmp_link_vdevs[i]->vdev_mlme.bss_chan, link_vap->iv_bsschan);
        }

        if (wlan_rptr_is_psta_vdev(vap->vdev_obj)) {
            ic = vap->iv_ic;

            if (ic && ic->ic_sta_vap && ic->ic_sta_vap->vdev_obj)
                mpsta_vdev = ic->ic_sta_vap->vdev_obj;
            else
                goto err;

            status = wlan_vdev_get_bss_peer_mld_mac(mpsta_vdev, &ap_mld_mac);
            if (QDF_IS_STATUS_ERROR(status)) {
                qdf_err("Peer MLD node fetch failed\n");
                goto err;
            }

            mpsta_ml_peer = wlan_mlo_get_mlpeer(mpsta_vdev->mlo_dev_ctx,
                                                &ap_mld_mac);
            if (mpsta_ml_peer) {
                peer_entry = &ml_peer->peer_list[0];
                ml_peer->primary_umac_psoc_id =
                                           mpsta_ml_peer->primary_umac_psoc_id;
                mlo_peer_assign_primary_umac(ml_peer, peer_entry);
            }

            qdf_info("PSTA Assoc peer " QDF_MAC_ADDR_FMT
                     " primary umac soc %d ",
                     QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                     ml_peer->primary_umac_psoc_id);
        }
        else {
            mlo_peer_allocate_primary_umac(mlo_dev_ctx, ml_peer,
                                           tmp_link_vdevs);
        }

        if ((ml_peer->max_links == 1) ||
            (mlo_vdevs_check_single_soc(tmp_link_vdevs, ml_peer->max_links))) {
            /* For splitphy single soc case, assoc link is selected as primary */
            ni->ml_info.primary_link_id = wlan_vdev_get_link_id(assoc_vdev);
            for (i = 0; i < j; i++)
                  wlan_objmgr_vdev_release_ref(tmp_link_vdevs[i], WLAN_MLO_MGR_ID);
        } else {
             peer_entry = &ml_peer->peer_list[0];
             if (peer_entry->is_primary == true) {
                 /* For multi-soc case, assoc link is selected as primary in case if primary soc is splitphy */
                 ni->ml_info.primary_link_id = wlan_vdev_get_link_id(assoc_vdev);
                 for (i = 0; i < j; i++)
                     wlan_objmgr_vdev_release_ref(tmp_link_vdevs[i], WLAN_MLO_MGR_ID);
             } else {
                 for (i = 0; i < j; i++) {
                     if (ml_peer->primary_umac_psoc_id == wlan_vdev_get_psoc_id(tmp_link_vdevs[i])) {
                         if (!primary_umac_set) {
                             primary_umac_set = 1;
                             ni->ml_info.primary_link_id = ni->ml_info.partner_link_info[i].link_id;
                         }
                     }
                     wlan_objmgr_vdev_release_ref(tmp_link_vdevs[i], WLAN_MLO_MGR_ID);
                 }
             }
        }
    }

    ni->ml_links.num_partner_links = 0;
    for (i = 0; i < ni->ml_info.num_partner_links; i++) {
        link_vdev = mlo_get_vdev_by_link_id(
                            vap->vdev_obj,
                            ni->ml_info.partner_link_info[i].link_id,
                            WLAN_MLO_MGR_ID);

        if (!link_vdev) {
         qdf_err("Unable to get VDEV for link ID %u",
                 ni->ml_info.partner_link_info[i].link_id);
         return QDF_STATUS_E_FAILURE;
        }
        idx++;
        if (vap->vdev_obj == link_vdev) {
          mlo_release_vdev_ref(link_vdev);
          continue;
        }
        ix = ni->ml_links.num_partner_links;
        ni->ml_links.link_info[ix].vdev_id = wlan_vdev_get_id(link_vdev);
        ni->ml_links.link_info[ix].hw_mld_link_id =
                      wlan_mlo_get_pdev_hw_link_id(wlan_vdev_get_pdev(link_vdev));
        ni->ml_links.link_info[ix].mlo_enabled = 1;
        ni->ml_links.link_info[ix].mlo_assoc_link = !wlan_vdev_mlme_is_link_sta_vdev(link_vdev);
        ni->ml_links.link_info[ix].ieee_link_id = wlan_vdev_get_link_id(link_vdev);
        /* Set primary umac based on primary umac soc_id for specific RDP's
           Use this logic in case when we have central vdev requirement.
           mlo_central_vdev flag is set to true for two Cases
           a. When we have WDS bridge peer in RDP 467
           b. When we have normal vap in 3 link Non-MLO STA and we need
           central link as primary umac */
        if (ml_peer && mlo_is_force_central_primary(link_vdev)) {
            if (ml_peer->primary_umac_psoc_id == wlan_vdev_get_psoc_id(link_vdev)) {
                ni->ml_links.link_info[ix].mlo_primary_umac = 1;
            } else {
                ni->ml_links.link_info[ix].mlo_primary_umac = 0;
            }
        } else {
            if (assoc_vap->iv_bss->ml_info.primary_link_id == ni->ml_info.partner_link_info[i].link_id)
                ni->ml_links.link_info[ix].mlo_primary_umac = 1;
            else
                ni->ml_links.link_info[ix].mlo_primary_umac = 0;
        }
        ni->ml_links.link_info[ix].mlo_bridge_peer = 0;
        ni->ml_links.link_info[ix].mlo_logical_link_index_valid = 1;
        ni->ml_links.link_info[ix].emlsr_support = 0;
        /* Once tqm migration is supported, this needs to be handled at that time.
        * Currently we call this API only during assoc. But in case of vdev restart we
        * send peer assoc again. At that time this might contain older info if tqm
        * migrtaion is done. So, during tqm migrtaion this filed needs to be updated.
        */
        if (!wlan_vdev_mlme_is_link_sta_vdev(link_vdev))
            ni->ml_links.link_info[ix].logical_link_index = 0;
        else
            ni->ml_links.link_info[ix].logical_link_index = idx;

        if (mlo_is_sta_bridge_vdev(link_vdev))
            ni->ml_links.link_info[ix].mlo_bridge_peer = 1;
        else {
            ni->ml_links.link_info[ix].mlo_bridge_peer = 0;
            /* Save the partner ml links that are send to target during bridge peer assoc*/
            if (bridge_vap_exists) {
                bridge_part_idx = bridge_sta_ctx->bridge_ml_links.num_partner_links;
                qdf_mem_copy(&bridge_sta_ctx->bridge_ml_links.link_info[bridge_part_idx],
                             &ni->ml_links.link_info[ix], sizeof(struct mlo_tgt_link_info));
                bridge_sta_ctx->bridge_ml_links.num_partner_links++;
            }
        }
        ni->ml_links.num_partner_links++;
        mlo_release_vdev_ref(link_vdev);
    }

    return QDF_STATUS_SUCCESS;

err:
    for (i = 0; i < j; i++) {
         wlan_objmgr_vdev_release_ref(tmp_link_vdevs[i], WLAN_MLO_MGR_ID);
    }
    return QDF_STATUS_E_FAILURE;
}

void wlan_mlo_peer_get_bridge_sta_links_info(struct wlan_objmgr_vdev *vdev,
                                      struct mlo_tgt_partner_info *ml_links)
{
    struct wlan_objmgr_vdev *assoc_vdev = NULL;
    struct wlan_mlo_dev_context *mlo_dev_ctx = NULL;
    struct wlan_mlo_bridge_sta *bridge_sta_ctx = NULL;
    assoc_vdev = ucfg_mlo_get_assoc_link_vdev(vdev);

    if (!assoc_vdev) {
        qdf_err("assoc vdev not found");
        return;
    }
    mlo_dev_ctx = assoc_vdev->mlo_dev_ctx;

    if (!mlo_dev_ctx || !mlo_dev_ctx->bridge_sta_ctx) {
        qdf_err("mlo_dev_ctx or bridge_sta_ctx NULL !!");
        return;
    }
    bridge_sta_ctx = mlo_dev_ctx->bridge_sta_ctx;

    if (bridge_sta_ctx->bridge_ml_links.num_partner_links != 2) {
        qdf_err("WDS Bridge should have exactly two partner links Investigate!!");
        return;
    }
    qdf_mem_copy(&ml_links->link_info[0],&bridge_sta_ctx->bridge_ml_links.link_info[0],
                 sizeof(struct mlo_tgt_link_info));

    qdf_mem_copy(&ml_links->link_info[1],&bridge_sta_ctx->bridge_ml_links.link_info[1],
                 sizeof(struct mlo_tgt_link_info));
    /* Bridge partner value */
    ml_links->num_partner_links = 2;
}

qdf_export_symbol(wlan_mlo_peer_get_bridge_sta_links_info);

void wlan_mlo_peer_get_sta_links_info(struct wlan_objmgr_peer *peer,
                                      struct mlo_tgt_partner_info *ml_links)
{
    struct ieee80211_node *ni = wlan_peer_get_mlme_ext_obj(peer);

    if (!ni)
        return;

    if (ni->ml_links.num_partner_links)
        qdf_mem_copy(ml_links, &ni->ml_links,
                     sizeof(struct mlo_tgt_partner_info));
}

qdf_export_symbol(wlan_mlo_peer_get_sta_links_info);
#else
static inline
QDF_STATUS wlan_mlme_fill_ml_params(struct ieee80211_node *ni, wbuf_t wbuf,
        uint8_t *t2lmie)
{
    return QDF_STATUS_SUCCESS;
}
#endif

/* Receive assoc/reassoc response
 * - the caller of this routine validates the frame and ensures that the opmode == STA
 */
int ieee80211_mlme_recv_assoc_response(struct ieee80211_node *ni,
                                       int                   subtype,
                                       u_int16_t             capability,
                                       u_int16_t             status_code,
                                       u_int16_t             aid,
                                       u_int8_t              *ie_data,
                                       u_int32_t             ie_length,
                                       wbuf_t                wbuf)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    int                           mlme_request_type = mlme_priv->im_request_type;
    int                           error;
    u_int32_t                     rxlinkspeed, txlinkspeed; /* bits/sec */
    struct wlan_vdev_state_event event;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
    QDF_STATUS status = QDF_STATUS_SUCCESS;
    uint8_t *t2lmie = NULL;
    bool qcn_ie = false;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s im_request_type=%d status=%d (0x%08X)\n",
        __func__,
        mlme_priv->im_request_type,
        status_code, status_code);

    /* Ignore if no request in progress */
    if ((mlme_priv->im_request_type != MLME_REQ_ASSOC) &&
        (mlme_priv->im_request_type != MLME_REQ_REASSOC))
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Incorrect request type %d\n",
            __func__, mlme_priv->im_request_type);
        return -EINVAL;
    }

    if (!wlan_vdev_mlme_is_link_sta_vdev(vap->vdev_obj) &&
        !wlan_mlme_assoc_sm_is_state_assoc(vap->vdev_obj)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: assoc sm is not in assoc state\n", __func__);
        return -EINVAL;
    }

    if (!wlan_vdev_mlme_is_link_sta_vdev(vap->vdev_obj) &&
        !OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Timed-out already\n", __func__);
        return -EINVAL;
    }

    if (status_code != IEEE80211_STATUS_SUCCESS) {
        goto complete;
    }

    /* Validate AID */
    aid  &= ~IEEE80211_FIELD_TYPE_AID;
    if ((aid > 2007) || (aid == 0))
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Association response contains invalid AID=%d\n", __func__, aid);
        status_code = IEEE80211_STATUS_UNSPECIFIED;
        goto complete;
    }

    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }

    if (rx_ops && rx_ops->son_rx_ops.process_mgmt_frame) {
        error = rx_ops->son_rx_ops.process_mgmt_frame(vap->vdev_obj,  ni->peer_obj, subtype, wbuf_header(wbuf), wbuf_get_pktlen(wbuf), NULL);
        if (error) {
            event.state = VDEV_STATE_DISCONNECTED;
            if (rx_ops && rx_ops->son_rx_ops.deliver_event)
                rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_VDEV_STATE, &event);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s:mlme_process_asresp_elements failed. Error: %d\n", __func__,error);
            status_code = IEEE80211_STATUS_UNSPECIFIED;
            goto complete;
        }
    }
    error = mlme_process_asresp_elements(ni, ie_data, ie_length, &qcn_ie, &t2lmie);
    if (error) {
        event.state = VDEV_STATE_DISCONNECTED;
        if (rx_ops && rx_ops->son_rx_ops.deliver_event)
            rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_VDEV_STATE, &event);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: mlme_process_asresp_elements failed\n", __func__);
        status_code = IEEE80211_STATUS_UNSPECIFIED;
        goto complete;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_peer_is_mlo(ni->peer_obj) && !(ni->ni_ext_flags & IEEE80211_NODE_EHT)) {
        event.state = VDEV_STATE_DISCONNECTED;
        if (rx_ops && rx_ops->son_rx_ops.deliver_event)
            rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_VDEV_STATE, &event);
        mlme_err("Peer marked as MLO capable but not EHT, rejecting assoc resp");
        status_code = IEEE80211_STATUS_UNSPECIFIED;
        goto complete;
    }
#endif
    /* Association successful */

complete:
    switch (mlme_priv->im_request_type) {
    case MLME_REQ_ASSOC:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: mlme_assoc_complete status %d\n", __func__, status_code);
        if (subtype != IEEE80211_FC0_SUBTYPE_ASSOC_RESP) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: mlme_assoc_complete status type mismatched %d\n", __func__, subtype);
             return -EINVAL;
        }
        break;

    case MLME_REQ_REASSOC:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: mlme_reassoc_complete status %d\n", __func__, status_code);
        if (subtype != IEEE80211_FC0_SUBTYPE_REASSOC_RESP) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: mlme_assoc_complete status type mismatched %d\n", __func__, subtype);
             return -EINVAL;
        }
        break;

    default:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: mlme_reassoc_complete status %d unexpected request type %d\n",
            __func__, status_code, mlme_priv->im_request_type);
        return -EINVAL;
    }

    /* Request complete */
    mlme_priv->im_request_type = MLME_REQ_NONE;

    if (status_code == IEEE80211_STATUS_SUCCESS) {
        ASSERT(aid != 0);
        ni->ni_associd = aid;
        ni->ni_assoctime = OS_GET_TICKS() - ni->ni_assocstarttime;

#ifdef WLAN_FEATURE_11BE_MLO
        if (wlan_peer_is_mlo(ni->peer_obj))
            status = wlan_mlme_fill_ml_params(ni, wbuf, t2lmie);
#endif
        if (QDF_IS_STATUS_SUCCESS(status)) {
            wlan_node_set_peer_state(ni, WLAN_WAITKEY_STATE);
            if(wlan_vap_is_pmf_enabled(vap)){
               wlan_crypto_set_hwmfpQos(vap, 1);
            }
        } else {
            status_code = IEEE80211_STATUS_UNSPECIFIED;
        }
    }
    if (status_code == IEEE80211_STATUS_REJECT_TEMP) {
        error = mlme_process_timeout_interval_elements(ni, ie_data, ie_length);
    }

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     * Handling MLO address translation issues in Be
     * Indicate controller through NL event to disable AMSDUs for 3rd party root-AP
     */
    if (vap->iv_tx_encap_type == htt_cmn_pkt_type_raw && qcn_ie == false &&
        wlan_peer_is_mlo(ni->peer_obj) && wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {
        if (vap->iv_bss->ml_info.num_partner_links > 1) {
            wlan_cfg80211_hw_status_notification(vap, ni->peer_obj, false);
        } else if ((vap->iv_bss->ml_info.num_partner_links == 1) &&
                 (!cfg_get(psoc, CFG_OL_ADV_RAW_MODE_WAR_ENABLE) ||
                 qdf_mem_cmp(ni->peer_obj->macaddr, ni->peer_obj->mldaddr, QDF_MAC_ADDR_SIZE) == 0)) {
            /*
             * For cases when ini enable_advance_rawmode_war is true, then send enable
             * event only when peer mld mac is same as peer link mac
             */
            wlan_cfg80211_hw_status_notification(vap, ni->peer_obj, true);
        }
    }
#endif

    /* indicate linkspeed */
     mlme_get_linkrate(ni, &rxlinkspeed, &txlinkspeed);
     IEEE80211_DELIVER_EVENT_LINK_SPEED(vap, rxlinkspeed, txlinkspeed);

     if (vap->iv_he_ul_muofdma) {
            vap->iv_update_muedca_params = 1;
     }

     /* Association complete (success or failure) */
    switch (mlme_request_type) {
    case MLME_REQ_ASSOC:
        IEEE80211_DELIVER_EVENT_MLME_ASSOC_COMPLETE(vap, status_code, aid, wbuf);
        break;

    case MLME_REQ_REASSOC:
        IEEE80211_DELIVER_EVENT_MLME_REASSOC_COMPLETE(vap, status_code, aid, wbuf);
        break;

    default:
        break;
    }

    return 0;
}

/* Send association or reassociation request */
static int mlme_assoc_reassoc_request(wlan_if_t vaphandle, int reassoc, u_int8_t *prev_bssid, u_int32_t timeout)
{
    struct ieee80211vap           *vap = vaphandle;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    struct ieee80211_node         *ni = vap->iv_bss; /* bss node */
    int                           error = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    ASSERT(mlme_priv->im_request_type == MLME_REQ_NONE);
    mlme_priv->im_request_type = reassoc ? MLME_REQ_REASSOC : MLME_REQ_ASSOC;

    /* Set the timeout timer for association failure case */
    OS_SET_TIMER(&mlme_priv->im_timeout_timer, timeout);

    /* Transmit frame */
    error = ieee80211_send_assoc(ni, reassoc, prev_bssid);
    if (error) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: ieee80211_send_assoc error %d (0x%08X)\n",
            __func__, error, error);
        goto err;
    }

    wlan_node_set_peer_state(ni, WLAN_ASSOC_STATE);
    return error;

err:
    mlme_priv->im_request_type = MLME_REQ_NONE;
    OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer);
    return error;
}

#ifdef WLAN_FEATURE_11BE
QDF_STATUS ieee80211_validate_assoc_info_11be(struct ieee80211_node *ni)
{

    if (!ni) {
       qdf_rl_err("Node structure is NULL");
       return QDF_STATUS_E_INVAL;
    }

    if (ieee80211_is_phymode_11be(ni->ni_phymode)) {

        if (!(ni->ni_ext_flags & IEEE80211_NODE_EHT)) {
            qdf_rl_err("EHT flag is not set for EHT phymode");
            return QDF_STATUS_E_INVAL;
        }

        if (!ieee80211_is_basic_eht_txrx_mcsnss_requirement_met(
                  ni, EHTCAP_TXRX_MCS_NSS_IDX_80)) {
              qdf_rl_err("Basic EHT_MCS_NSS requirement for 20,40,80MHz is not met");
              return QDF_STATUS_E_INVAL;
        }

        if (ieee80211_is_phymode_11bea_eht160(ni->ni_phymode) &&
                 !(ni->ni_eht.eht_basic_txrxmcsnss_req_met_160)) {
              qdf_rl_err("EHT_MCS_NSS requirement for 160MHz is not met");
              return QDF_STATUS_E_INVAL;
         }

         if (ieee80211_is_phymode_11bea_eht320(ni->ni_phymode) &&
                 !(ni->ni_eht.eht_basic_txrxmcsnss_req_met_320)) {
              qdf_rl_err("EHT_MCS_NSS requirement for 320MHz is not met");
              return QDF_STATUS_E_INVAL;
         }

    }

    return QDF_STATUS_SUCCESS;
}
#else
QDF_STATUS ieee80211_validate_assoc_info_11be(struct ieee80211_node *ni)
{
    return QDF_STATUS_SUCCESS;
}
#endif /* WLAN_FEATURE_11BE */

QDF_STATUS ieee80211_validate_assoc_info(struct ieee80211_node *ni)
{
    if (!ni) {
       qdf_rl_err("Node structure is NULL");
       return QDF_STATUS_E_INVAL;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (ni->peer_obj->mlo_bridge_peer)
        return QDF_STATUS_SUCCESS;
#endif

    /* Drop the assoc response frame only if the chwidth is greater than
     * phymode.
     */
    if (ni->ni_chwidth > get_chwidth_phymode(ni->ni_phymode)) {
        qdf_rl_err("Node Channel width (%d) is greater than the phymode (%d)",
                ni->ni_chwidth, ni->ni_phymode);
        return QDF_STATUS_E_INVAL;
    }

    if (ieee80211_is_phymode_11ng(ni->ni_phymode) ||
        ieee80211_is_phymode_11na(ni->ni_phymode)) {

        if (!(ni->ni_flags & IEEE80211_NODE_HT)) {
             qdf_rl_err("HT flag is not set for HT phymode");
             return QDF_STATUS_E_INVAL;
        }

    } else if (ieee80211_is_phymode_11ac(ni->ni_phymode)) {

        if (!(ni->ni_flags & IEEE80211_NODE_VHT)) {
             qdf_rl_err("VHT flag is not set for VHT phymode");
             return QDF_STATUS_E_INVAL;
        }

    } else if (ieee80211_is_phymode_11ax(ni->ni_phymode)) {

        if (!(ni->ni_ext_flags & IEEE80211_NODE_HE)) {
             qdf_rl_err("HE flag is not set for HE phymode");
             return QDF_STATUS_E_INVAL;
        }

        if ((ni->ni_chwidth > IEEE80211_CWM_WIDTH20) &&
                 (!(ni->ni_he.hecap_phyinfo[HECAP_PHYBYTE_IDX0] &
                                  IEEE80211_HECAP_LDPC_MASK))) {
              qdf_rl_err("LDPC is not set in HE CAP for bandwidth greater than 20MHz");
              return QDF_STATUS_E_INVAL;
        }

        if (!ieee80211_is_basic_txrx_mcsnss_requirement_met(
                  ni, HECAP_TXRX_MCS_NSS_IDX_80)) {
              qdf_rl_err("Basic HE_MCS_NSS requirement for 20,40,80MHz is not met");
              return QDF_STATUS_E_INVAL;
        }

        if (ieee80211_is_phymode_11axa_he160(ni->ni_phymode) &&
                 !(ni->ni_he.he_basic_txrxmcsnss_req_met_160)) {
              qdf_rl_err("HE_MCS_NSS requirement for 160MHz is not met");
              return QDF_STATUS_E_INVAL;
         }

         if (ieee80211_is_phymode_11axa_he80_80(ni->ni_phymode) &&
                 !(ni->ni_he.he_basic_txrxmcsnss_req_met_80_80)) {
              qdf_rl_err("HE_MCS_NSS requirement for 80_80MHz is not met");
              return QDF_STATUS_E_INVAL;
         }

    }

    if (ieee80211_validate_assoc_info_11be(ni) != QDF_STATUS_SUCCESS) {
        return QDF_STATUS_E_INVAL;
    }

    return QDF_STATUS_SUCCESS;
}
qdf_export_symbol(ieee80211_validate_assoc_info);

/*
 * Process information elements from association response frame.
 * This includes rate negociation, wmm parameter updating and etc.
 */
static int mlme_process_asresp_elements(struct ieee80211_node *ni,
                                        u_int8_t              *frm,
                                        u_int32_t             ie_len,
                                        bool                  *qcn_ie,
                                        uint8_t               **t2lmie)
{
    struct ieee80211vap          *vap = ni->ni_vap;
    struct ieee80211com          *ic = ni->ni_ic;
    u_int8_t                     *efrm = frm + ie_len;
    u_int8_t                     *rates, *xrates, *wme, *htcap, *tspecie, *athextcap, *extcap, *whc_apinfo, *vhtcap, *vhtop, *htinfo, *generic_cap;
    u_int8_t                     *hecap, *heop, *srpie, *hecap_6g;
#ifdef WLAN_FEATURE_11BE
    uint8_t *ehtcap = NULL, *ehtop = NULL;
#endif /* WLAN_FEATURE_11BE */
    u_int8_t                     *assoc_resp_ie;
    u_int8_t                     *qcn = NULL;
    u_int8_t                     qosinfo;
    int                          ht_rates_allowed;
    struct ieee80211_ie_vhtop    *ap_vhtop;
    struct ieee80211_ie_vhtcap    *ap_vhtcap = NULL;
    struct ieee80211_ie_htinfo_cmn    *ap_htinfo;
#if OBSS_PD
    uint8_t num_sta, num_ap;
#endif /* OBSS_PD */
    u_int8_t *extenderie = NULL;
    uint32_t heop_info;

    ASSERT((vap->iv_opmode != IEEE80211_M_STA) || (ni == vap->iv_bss));

    rates = xrates = wme = htcap = tspecie = athextcap = extcap = whc_apinfo = vhtcap = vhtop = htinfo = assoc_resp_ie = generic_cap = NULL;
    ap_vhtop = NULL;
    hecap = heop = srpie = hecap_6g = NULL;
    ap_htinfo = NULL;
    assoc_resp_ie = frm;

    while (frm < efrm) {
        switch (*frm) {
        case IEEE80211_ELEMID_RATES:
            rates = frm;
            break;
        case IEEE80211_ELEMID_XRATES:
            xrates = frm;
            break;
        case IEEE80211_ELEMID_XCAPS:
            extcap = frm;
            break;
        case IEEE80211_ELEMID_HTCAP_ANA:
            htcap = (u_int8_t *)&((struct ieee80211_ie_htcap *)frm)->hc_ie;
            break;
       case IEEE80211_ELEMID_VHTCAP:
            if(vhtcap == NULL) {
                vhtcap = (u_int8_t *)(struct ieee80211_ie_vhtcap *)frm;
            }
            break;
       case IEEE80211_ELEMID_VHTOP:
            if(vhtop == NULL) {
                vhtop = (u_int8_t *)(struct ieee80211_ie_vhtop *)frm;
            }
            break;

        case IEEE80211_ELEMID_OP_MODE_NOTIFY:
            /* We can ignore this because the info we need is available in VHTCAP and VHTOP */
        break;
        case IEEE80211_ELEMID_EXTN:
            if ((frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) < efrm) {
                if (*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                            IEEE80211_ELEMID_EXT_HECAP) {
                    hecap = frm;
                } else if (*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                            IEEE80211_ELEMID_EXT_HEOP) {
                    heop  = frm;
#ifdef WLAN_FEATURE_11BE
                } else if (*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                           WLAN_EXTN_ELEMID_EHTCAP) {
                    ehtcap  = frm;
                } else if (*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                           WLAN_EXTN_ELEMID_EHTOP) {
                    ehtop  = frm;
#endif /* WLAN_FEATURE_11BE */
                } else if (*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                            IEEE80211_ELEMID_EXT_SRP) {
		            srpie = frm;
		        } else if(*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                        IEEE80211_ELEMID_EXT_6G_HECAP) {
                    hecap_6g = frm;
#ifdef WLAN_FEATURE_11BE
                } else if (*(frm + IEEE80211_HE_IE_HDR_OFFSET_TO_ID_EXT) ==
                           WLAN_EXTN_ELEMID_T2LM) {
                    /* There can be two T2LM IEs in the Assoc request frame.
                     * Point to the first T2LM IE.
                     */
                    if (!*t2lmie) {
                        *t2lmie = frm;
                    }
#endif /* WLAN_FEATURE_11BE */
                }
            }
        break;
        case IEEE80211_ELEMID_VENDOR:
            if (iswmeoui(frm, WME_PARAM_OUI_SUBTYPE))
                wme = frm;
            else if (iswmeoui(frm, WME_INFO_OUI_SUBTYPE))
                wme = frm;
            else if (iswmeoui(frm, WME_TSPEC_OUI_SUBTYPE))
                tspecie = frm;
            else if(ishtcap(frm)) {
                if (htcap == NULL) {
                    htcap = (u_int8_t *)&((struct vendor_ie_htcap *)frm)->hc_ie;
                }
            }
            else if (isatheros_extcap_oui(frm))
                 athextcap = frm;
            else if (is_extender_oui((uint8_t *)frm)) {
                /* location where Extender IE Present */
                extenderie = frm;
            }
            else if (isinterop_vht(frm) && !(vhtcap) &&
                        ieee80211vap_11ng_vht_interopallowed(vap)) {
                 /* Vht cap & Op location in Vendor specific Vht IE */
                 vhtcap = (u_int8_t *)(struct ieee80211_ie_vhtcap *) (frm + 7);
                 vhtop = (u_int8_t *)(struct ieee80211_ie_vhtop *) (frm + 21);
                 ieee80211node_set_flag(ni, IEEE80211_NODE_11NG_VHT_INTEROP_AMSDU_DISABLE);
            }
            else if (isbwnss_oui(frm)) {
                 ni->ni_bw160_nss = IEEE80211_GET_BW_NSS_FWCONF_160(*(frm + 8)); /*BW_NSS has sub_type & version, hence read data after version*/
            }
            else if(isqcn_oui(frm)) {
                qcn = frm;
#ifdef WLAN_FEATURE_11BE_MLO
                if (wlan_peer_is_mlo(ni->peer_obj)) {
                    ni->peer_obj->peer_flags |= WLAN_PEER_QCN_NODE;
                }
#endif
            }
            else if(isgeneric_cap_oui(frm)) {
                generic_cap = frm;
            }
            break;
#if UMAC_SUPPORT_WNM
        case IEEE80211_ELEMID_BSSMAX_IDLE_PERIOD:
			ieee80211_wnm_parse_bssmax_ie(ni, frm);
            break;
        case IEEE80211_ELEMID_TIM_BCAST_RESPONSE:
            ieee80211_parse_timresp_ie(frm, frm + frm[1] + 2, ni);
            break;
#endif
       case IEEE80211_ELEMID_HTINFO_ANA:
            htinfo = (u_int8_t *)(&((struct ieee80211_ie_htinfo *)frm)->hi_ie);
            break;

        }

        /* elem id + len = 2 bytes */
        frm += frm[1] + 2;
    }

    if (!rates || (rates[1] > IEEE80211_RATE_MAXSIZE)) {
        /* XXX: msg + stats */
        return -EINVAL;
    }

    if (!ieee80211_setup_rates(ni, rates, xrates,
                                 IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE |
                                 IEEE80211_F_DOXSECT)) {
        qdf_nofl_info("%s: association failed (rate set mismatch)\n", __func__);
        return -EINVAL;
    }

    /*
     * U-APSD is enabled/disabled per-association.
     */
    if (wme != NULL) {
        /* Parse IE according to subtype */
        if (iswmeparam(wme)) {
            /* Association is QOS-enabled */
            ieee80211node_set_flag(ni, IEEE80211_NODE_QOS);

            if (vap->iv_opmode != IEEE80211_M_BTAMP &&
                ieee80211_parse_wmeparams(vap, wme, &qosinfo, 1) >= 0) {

                /* Check if association is UAPSD-enabled */
                if (qosinfo & WME_CAPINFO_UAPSD_EN) {
                    ieee80211node_set_flag(ni, IEEE80211_NODE_UAPSD);
                }
                else {
                    ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
                }

                ieee80211_wme_updateparams(vap);
            }
        }
        else {
            /*** QOS requires WME Param */
            ieee80211node_clear_flag(ni, IEEE80211_NODE_QOS);

            if (ieee80211_parse_wmeinfo(vap, wme, &qosinfo) >= 0) {
                /* Check if association is UAPSD-enabled */
                if (qosinfo & WME_CAPINFO_UAPSD_EN) {
                    ieee80211node_set_flag(ni, IEEE80211_NODE_UAPSD);
                }
                else {
                    ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
                }

                ieee80211_wme_updateinfo(vap);
            }
        }
    }
    else {
        ieee80211node_clear_flag(ni, IEEE80211_NODE_QOS);
        ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
    }

    if ((tspecie != NULL) &&
        (ieee80211_parse_tspecparams(vap, tspecie) >= 0)) {
        /* store the tspec */
    }

    if (athextcap != NULL)
        ieee80211_process_athextcap_ie(ni, athextcap);

#if REPEATER_SAME_SSID
    if (wlan_rptr_global_is_s_ssid() && (extenderie != NULL) && (vap == ic->ic_sta_vap)) {
        wlan_rptr_process_extender_ie(ni->peer_obj, extenderie, IEEE80211_FRAME_TYPE_ASSOCRESP);
    }
#endif
    /*
     * With WEP and TKIP encryption algorithms:
     * Disable aggregation if IEEE80211_NODE_WEPTKIPAGGR is not set.
     * Disable 11n if IEEE80211_FEXT_WEP_TKIP_HTRATE is not set.
     */
    ht_rates_allowed = (IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan) ||
                        IEEE80211_IS_CHAN_11AC(ic->ic_curchan) ||
                        IEEE80211_IS_CHAN_11N(ic->ic_curchan));

    if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) &&
	wlan_crypto_is_htallowed(vap->vdev_obj, NULL)) {
        ieee80211node_set_flag(ni, IEEE80211_NODE_WEPTKIP);
        if (ieee80211_ic_wep_tkip_htrate_is_set(ic)) {
            if (!ieee80211_has_weptkipaggr(ni))
                ieee80211node_set_flag(ni, IEEE80211_NODE_NOAMPDU);
        } else {
            ht_rates_allowed = 0;
        }
    }

    if (IEEE80211_IS_CHAN_5GHZ_6GHZ(ic->ic_curchan)) {
        ni->ni_phymode = IEEE80211_MODE_11A;
    } else if (( xrates != NULL)  &&
             IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11G)) {
        ni->ni_phymode = IEEE80211_MODE_11G;
    } else {
        ni->ni_phymode = IEEE80211_MODE_11B;
    }


    /*
     * Channel width and Nss will get adjusted with HT parse and VHT parse
     * if those modes are enabled
     */
    ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
    ni->ni_streams = 1;
    /* 11n - HT rates not allowed using WEP and TKIP */
    if ((htcap != NULL) && (ht_rates_allowed)) {
        /* record capabilities, mark node as capable of HT */
        if(!ieee80211_parse_htcap(ni, htcap, NULL)) {
            qdf_nofl_info("%s: association failed (Rx MCS  set mismatch)\n", __func__);
            return -EINVAL;
        }

        if (!ieee80211_setup_ht_rates(ni, htcap,
            IEEE80211_F_DOFRATE | IEEE80211_F_DOXSECT |
            IEEE80211_F_DOBRS)) {
            qdf_nofl_info("%s: association failed (rate set mismatch)\n", __func__);
            return -EINVAL;
        }
#ifdef ATH_SUPPORT_TxBF
        // set keycache for txbf after sta associated successfully.
        if ( ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf){
            struct ieee80211com     *ic = vap->iv_ic;

            ieee80211_set_TxBF_keycache(ic,ni);
            ni->ni_bf_update_cv = 1;
            ni->ni_allow_cv_update = 1;
        }
#endif
    } else {
        /*
         * Flush any state from a previous association.
         */
        ieee80211node_clear_flag(ni, IEEE80211_NODE_HT);
        IEEE80211_NODE_CLEAR_HTCAP(ni);
    }

      /* Add vht cap for 2.4G mode, if 256QAM is enabled */
    if ((IEEE80211_IS_CHAN_11AX_SUPPORTED(vap->iv_bsschan) ||
         IEEE80211_IS_CHAN_11AC(vap->iv_bsschan) ||
         IEEE80211_IS_CHAN_11NG(vap->iv_bsschan)) &&
                    ieee80211vap_vhtallowed(vap)) {

        if (vhtcap != NULL) {
            /* record capabilities, mark node as capable of VHT */
            ieee80211_parse_vhtcap(ni, vhtcap, NULL);

            if (ni->ni_ext_nss_support && !validate_extnss_vhtcap(&ni->ni_vhtcap)) {
                qdf_print("%s: Association failed invalid vhtcap",__func__);
                return -EINVAL;
            }

            if (vhtop && htinfo) {
                ieee80211_parse_vhtop(ni, vhtop, htinfo);
            }

            if (!ieee80211_setup_vht_rates(ni, vhtcap,
                IEEE80211_F_DOFRATE | IEEE80211_F_DOXSECT | IEEE80211_F_DOBRS)) {
                qdf_nofl_info("%s: association failed (vht rate set mismatch)\n", __func__);
                return -EINVAL;
            }


        } else {
            /*
             * Flush any state from a previous association.
             */
            ieee80211node_clear_flag(ni, IEEE80211_NODE_VHT);
            ni->ni_vhtcap = 0;
        }

    } else {
        /*
         * Flush any state from a previous association.
         */
        ieee80211node_clear_flag(ni, IEEE80211_NODE_VHT);
        ni->ni_vhtcap = 0;
    }

    if(vhtop != NULL) {
        ap_vhtop = (struct ieee80211_ie_vhtop *)vhtop;
    }

    if (hecap != NULL && (IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan)
                      && ieee80211vap_heallowed(vap))) {
        if(heop) {
            /* Update Basic MCS NSS requirement from AP */
            ieee80211_update_basic_bss_mcs_nss_req(ni, heop);
        }
        /* record HE capabilities */
        ieee80211_parse_hecap(ni, hecap, IEEE80211_FC0_SUBTYPE_ASSOC_RESP);

    } else {
         /*
          * Flush any state from a previous association.
          */
         ni->ni_ext_flags &= ~IEEE80211_NODE_HE;
         qdf_mem_zero(&ni->ni_he, sizeof(struct ieee80211_he_handle));
    }

    if (hecap_6g != NULL && IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan)
                        && ieee80211vap_heallowed(vap)) {
        ieee80211_parse_he_6g_bandcap(ni, hecap_6g, IEEE80211_FC0_SUBTYPE_ASSOC_RESP);
        ieee80211_get_nss_from_hecap_mcsnssset_6g(ni);

    }

    if (heop != NULL && IEEE80211_IS_CHAN_11AX_SUPPORTED(ic->ic_curchan)
                      && ieee80211vap_heallowed(vap) ) {
        ieee80211_parse_heop(ni, heop, IEEE80211_FC0_SUBTYPE_ASSOC_RESP, NULL);

        /* Update target with this new HE OP params.
         */
#if SUPPORT_11AX_D3
        heop_info = (ni->ni_he.heop_param |
                     (ni->ni_he.heop_bsscolor_info << HEOP_PARAM_S));
#else
        heop_info = ni->ni_he.heop_param;
#endif
        if(ic->ic_vap_set_param &&
            (ic->ic_vap_set_param(vap,
                      IEEE80211_CONFIG_HE_OP, heop_info) != EOK)) {

            qdf_print("%s WARNING!!! heop set to target failed", __func__);
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_HE,
                    "%s: HEOP update sent to FW. color: %d\n",
                    __func__, ni->ni_he.heop_param & 0x3f);
        }

    }

#ifdef WLAN_FEATURE_11BE
    if (ehtcap != NULL && (IEEE80211_IS_CHAN_11BE(ic->ic_curchan)
                      && ieee80211vap_ehtallowed(vap))) {
        /* record EHT capabilities */
        ieee80211_parse_ehtcap(ni, ehtcap, IEEE80211_FC0_SUBTYPE_ASSOC_RESP);

    } else {
         /*
          * Flush any state from a previous association.
          */
         ni->ni_ext_flags &= ~IEEE80211_NODE_EHT;
         qdf_mem_zero(&ni->ni_eht, sizeof(struct ieee80211_eht_handle));
    }

    if (ehtop != NULL && IEEE80211_IS_CHAN_11BE(ic->ic_curchan)
                      && ieee80211vap_ehtallowed(vap) ) {
        ieee80211_parse_ehtop(ni, ehtop, IEEE80211_FC0_SUBTYPE_ASSOC_RESP, NULL);
    }
#endif /* WLAN_FEATURE_11BE */

    if (htinfo != NULL) {
        ap_htinfo = (struct ieee80211_ie_htinfo_cmn *)htinfo;
    }

#if OBSS_PD
    if (srpie != NULL) {
        num_sta = wlan_mbss_num_sta(ic->ic_pdev_obj);
        num_ap = wlan_mbss_num_ap(ic->ic_pdev_obj);
        ieee80211_parse_srpie(ni, srpie);
        /* Update target with this new SRP IE params.
         */
        if (!num_ap && num_sta) {
            if(ic->ic_spatial_reuse &&
                (ic->ic_spatial_reuse(vap) != EOK)) {
                qdf_print("%s WARNING!!! SRP set to target failed\n", __func__);
            }
        }
    }
#endif

    if (qcn != NULL) {
#if QCN_IE
        ieee80211_parse_qcnie(qcn, NULL, ni, NULL, IEEE80211_FC0_SUBTYPE_ASSOC_RESP);
#endif
        *qcn_ie = true;
    }
    if (vhtcap != NULL) {
        ap_vhtcap = (struct ieee80211_ie_vhtcap *)vhtcap;
    }
    /*
     * ieee80211_parse_vhtop, ieee80211_parse_hecap would have set the channel
     * width based on APs operating mode/channel. If vap is forced to operate
     * in a different lower mode than what AP is operating in, then set the
     * channel width based on the forced channel/phy mode.
     *
     * Required to be revisited for HE case, as chwidth update and
     * ieee80211_update_ht_vht_he_phymode is taken care in the
     * ieee80211_parse_hecap routine.
     */
    ieee80211_readjust_chwidth(ni, ap_vhtop, ap_htinfo, ap_vhtcap);

    /* Update the PHY mode */
    ieee80211_update_phymode(ic, ni);

    ieee80211_update_vap_shortgi(ni);

    if (ieee80211_validate_assoc_info(ni)) {
        qdf_rl_err("Association failed as Assoc params validation failed");
        return -EINVAL;
    }

    /* Parse generic vendor capability IE */
    if (generic_cap != NULL) {
        ieee80211_parse_generic_vendor_capabilities(ni, generic_cap);
    }

    return EOK;
}

/*
 * Process Timeout Interval information elements from association response frame.
 * This includes an (re)association is failed with 802.11 reason code 30 .
 */
static int mlme_process_timeout_interval_elements(struct ieee80211_node *ni,
                                        u_int8_t              *frm,
                                        u_int32_t             ie_len)
{
    struct ieee80211vap          *vap = ni->ni_vap;
    u_int8_t                     *efrm = frm + ie_len;
    u_int8_t                     *comebacktiem;

    ASSERT((vap->iv_opmode != IEEE80211_M_STA) || (ni == vap->iv_bss));

    comebacktiem = NULL;

    while (frm < efrm) {
        if (IEEE80211_ELEMID_TIMEOUT_INTERVAL == *frm) {
            comebacktiem = frm;
            break;
        }
        frm += frm[1] + 2;
    }

    if ((comebacktiem != NULL) &&
        (ieee80211_parse_timeieparams(vap, comebacktiem) >= 0)) {
        /* store the tie info */
    }

    return EOK;
}

static u_int32_t get_max_phyrate(struct ieee80211com *ic)
{
    u_int32_t    linkspeed; /* bits/sec */
    u_int8_t     tx_streams = ieee80211_getstreams(ic, ic->ic_tx_chainmask);

    if (tx_streams > IEEE80211_MAX_11N_STREAMS)
    {
        tx_streams = IEEE80211_MAX_11N_STREAMS;
    }

    /* 11n card, report highest supported HT rate. */
    switch (tx_streams) {
    default:
        /* Default to single stream */
    case 1:
        linkspeed = 150000000;
        break;
    case 2:
        linkspeed = 300000000;
        break;
    case 3:
        linkspeed = 450000000;
        break;
    case 4:
        linkspeed = 600000000;
        break;
    }

    return linkspeed;
}
static void mlme_calculate_11n_connection_speed(struct ieee80211_node *ni, u_int32_t* rxlinkspeed, u_int32_t* txlinkspeed)
{
    struct ieee80211com    *ic = ni->ni_ic;
    u_int32_t              linkspeed; /* bits/sec */
    u_int8_t     tx_streams = ieee80211_getstreams(ic, ic->ic_tx_chainmask);
    u_int8_t     rx_streams = ieee80211_getstreams(ic, ic->ic_rx_chainmask);

    if (tx_streams > IEEE80211_MAX_11N_STREAMS)
    {
        tx_streams = IEEE80211_MAX_11N_STREAMS;
    }

    if (rx_streams > IEEE80211_MAX_11N_STREAMS)
    {
        rx_streams = IEEE80211_MAX_11N_STREAMS;
    }

    linkspeed = ic->ic_get_maxphyrate(ic, ni) * 1000;

    if (linkspeed == 0) {
        *txlinkspeed = get_max_phyrate(ic);
        *rxlinkspeed = *txlinkspeed;
    } else {
        *txlinkspeed = linkspeed;

        ASSERT(tx_streams <= rx_streams);

        /*
         * WAR for HB95 (1x1) low DownLink throughput issue. Report higher rx link speed to increase TCP RWIN size.
         * For 1x1 and 1x2, we report the rxlinkspeed as double of the txlinkspeed.
         */

        if((tx_streams == 1) && (rx_streams == 1)) {
            *rxlinkspeed = linkspeed * 2;
        } else {
            *rxlinkspeed = (linkspeed * rx_streams)/tx_streams;
            ASSERT(rx_streams <= 4);
        }
    }
}

void
mlme_get_linkrate(struct ieee80211_node *ni, u_int32_t* rxlinkspeed, u_int32_t* txlinkspeed)
{
    struct ieee80211vap    *vap = ni->ni_vap;
    struct ieee80211com    *ic = ni->ni_ic;
    int                    ht_rates_allowed;
    u_int8_t               rate;

    /* Check if connected to BSS */
    if ((ni->ni_htrates.rs_nrates == 0) && (ni->ni_rates.rs_nrates == 0)) {

        /* Not connected */

        if (ic->ic_caps & IEEE80211_C_HT) {
            *txlinkspeed = get_max_phyrate(ic);
            *rxlinkspeed = *txlinkspeed;
        } else {
            struct ieee80211_rateset *rs;

            /* Legacy card, report highest rate */
            rs = &vap->iv_op_rates[ieee80211_chan2mode(ic->ic_curchan)];
            rate = rs->rs_rates[rs->rs_nrates - 1];
            *txlinkspeed = mlme_dot11rate_to_bps(rate & IEEE80211_RATE_VAL);
            *rxlinkspeed = *txlinkspeed;
        }
    } else {

        /* Connected */
        /*
         * With WEP and TKIP encryption algorithms:
         * Disable 11n if IEEE80211_FEXT_WEP_TKIP_HTRATE is not set.
         */
        ht_rates_allowed = ((IEEE80211_IS_CHAN_11AC(ic->ic_curchan) ||
                            IEEE80211_IS_CHAN_11N(ic->ic_curchan)) &&
                            ieee80211vap_htallowed(vap));

        if (ni->ni_htrates.rs_nrates &&
            ht_rates_allowed) {
            mlme_calculate_11n_connection_speed(ni, rxlinkspeed, txlinkspeed);
            if (ic->ic_reg_parm.indicateRxLinkSpeed)    *txlinkspeed = *rxlinkspeed;
        } else {
            /* get maximum rate from node rate set */
            rate = ni->ni_rates.rs_rates[ni->ni_rates.rs_nrates - 1] & IEEE80211_RATE_VAL;
            *txlinkspeed = mlme_dot11rate_to_bps(rate);
            *rxlinkspeed = *txlinkspeed;

        }
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Rx Link speed Data rate: %d, Tx Link speed Data rate: %d\n",
                      __func__, *rxlinkspeed, *txlinkspeed);
}

int mlme_recv_auth_sta(struct ieee80211_node *ni,
                        u_int16_t             algo,
                        u_int16_t             seq,
                        u_int16_t             status_code,
                        u_int8_t              *challenge,
                        u_int8_t              challenge_length,
                        wbuf_t                wbuf)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    int32_t authmode = 0;
    struct ieee80211_frame *wh;
    u_int8_t *frm, *efrm;
    osif_dev * osifp;
    struct wireless_dev *wdev = NULL;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    frm = (u_int8_t *)&wh[1];
    efrm = wbuf_header(wbuf) + wbuf_get_pktlen(wbuf);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    if (mlme_priv->im_request_type != MLME_REQ_AUTH) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: invalid request type\n",
            __func__,
            mlme_priv->im_request_type);
        return -1;
    }

    /* Validate algo */
    if ((algo == IEEE80211_AUTH_ALG_SAE) && (vap->iv_sta_external_auth_enabled)) {
#if UMAC_SUPPORT_CFG80211
        osifp = (osif_dev *)vap->iv_ifp;
        wdev = get_cfg80211_notification_wdev(osifp);
        if (!wdev) {
            qdf_err("wdev is null");
            return -1;
        }

        if (vap->iv_cfg80211_create) {
#ifdef ENABLE_CFG80211_BACKPORTS
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                osif_mlo_rx_translate_addr(ni, wbuf, IEEE80211_FC0_SUBTYPE_AUTH);
	    } else if (!qdf_is_macaddr_zero(&vap->mld_mac_addr)) {
	         /* For non-MLO AP and MLO STA, the driver will have to give frame to stack on MLD addr as A1 */
                 qdf_mem_copy(wh->i_addr1, &vap->mld_mac_addr, QDF_MAC_ADDR_SIZE);
            }

            /* in non-AP STA mode the supplicant runs a single sm so link-id -1 is expected */
            if(wlan_cfg80211_rx_mgmt(wdev, vap->iv_ic->ic_curchan->ic_freq, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), 0, -1)) {
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
            if(cfg80211_rx_mgmt(wdev, vap->iv_ic->ic_curchan->ic_freq, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), 0)) {
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
            if(cfg80211_rx_mgmt(wdev, vap->iv_ic->ic_curchan->ic_freq, 0, qdf_nbuf_data(wbuf), qdf_nbuf_len(wbuf), 0, GFP_ATOMIC)) {
#endif
               return -1;
            }
        }
        return 0;
#endif
    }

    authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
    if ( authmode == -1 ) {
        qdf_err("crypto_err while getting authmode params\n");
        return -1;
    }

    /*  If STA is in wep+mixed and AP is in wep+open, Initially it starts with
        shared auth and then fall down to open mode if algo:shared,
        mode:mixed/auto and status code is not success */
    if ((algo == IEEE80211_AUTH_ALG_SHARED) &&
        ((authmode & (1<<WLAN_CRYPTO_AUTH_SHARED)) &&
         (authmode & (1<<WLAN_CRYPTO_AUTH_OPEN))) &&
        (status_code != IEEE80211_STATUS_SUCCESS))
    {
        wlan_crypto_set_vdev_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE,
                              1 << WLAN_CRYPTO_AUTH_OPEN);
        ni->ni_authalg = IEEE80211_AUTH_ALG_OPEN;
        return -1;
    }

    /* Validate seq */
    if (seq != mlme_priv->im_expected_auth_seq_number) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Invalid seq %d,%d\n",
            __func__,
            seq, mlme_priv->im_expected_auth_seq_number);
        return -1;
    }
    if ((authmode & (1 << WLAN_CRYPTO_AUTH_SHARED)) &&
        (seq == IEEE80211_AUTH_SHARED_CHALLENGE) &&
        (status_code == IEEE80211_STATUS_SUCCESS))
    {
        /* Send the challenge response authentication packet.
         * We don't care if the send fails. If it does, the timeout routine will do
         * the necessary cleanup.
         */
        ieee80211_send_auth(ni, seq + 1, 0, challenge, challenge_length, NULL, NULL);
        mlme_priv->im_expected_auth_seq_number = IEEE80211_AUTH_SHARED_PASS;
        return 0;
    }

    if (!OS_CANCEL_TIMER(&mlme_priv->im_timeout_timer)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Timed-out already\n", __func__);
        return -1;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: mlme_auth_complete\n", __func__);

    if (vap->iv_roam.iv_roaming) {
        if (vap->iv_roam.iv_ft_roam) {
            qdf_mem_copy(vap->iv_roam.iv_ft_params.target_ap, wh->i_addr2, QDF_MAC_ADDR_SIZE);
            if (EOK == mlme_process_authresp_ft_elements(ni, frm, efrm - frm)) {
                int retval = 0;
                retval = vap->iv_ic->ic_vap_set_param(vap, IEEE80211_CONFIG_INDICATE_FT_ROAM, 1);
                /* In case of FT roam in HK we see key install before peer assoc which makes
                 * fw discard key install cmd to avoid this we notify fw about FT roam using
                 * peer set param cmd. For legacy this param is not required and not supported
                 * so we consider it as success. */
                if (retval == QDF_STATUS_SUCCESS || retval == QDF_STATUS_E_NOSUPPORT)
                    vap->iv_roam.iv_wait_for_ftie_update = 1;
                else
                    return -1;
            }
            else
                return -1;
        }
    }

    /* Request complete */
    mlme_priv->im_request_type = MLME_REQ_NONE;

    /* Authentication complete (success or failure) */
    IEEE80211_DELIVER_EVENT_MLME_AUTH_COMPLETE(vap, wh->i_addr3, status_code);
    return 0;
}

static int mlme_process_authresp_ft_elements(struct ieee80211_node *ni,
                                        u_int8_t              *frm,
                                        u_int32_t             ie_len)
{
    struct ieee80211vap          *vap = ni->ni_vap;
    u_int8_t                     *efrm = frm + ie_len;
    u_int8_t                     *ftie, *mdie, *rsnie;
    u_int16_t                    ftie_len, mdie_len, rsnie_len;
    u_int8_t                     *ies;

    ASSERT((vap->iv_opmode != IEEE80211_M_STA) || (ni == vap->iv_bss));

    ftie = mdie = rsnie = NULL;

    /* free any existing ft_params */
    if (vap->iv_roam.iv_ft_params.fties) {
        qdf_mem_free(vap->iv_roam.iv_ft_params.fties);
    }

    vap->iv_roam.iv_ft_params.fties = NULL;
    vap->iv_roam.iv_ft_params.fties_len = 0;
    while (frm < efrm) {
        switch (*frm) {
       case IEEE80211_ELEMID_FT:
            ftie = frm;
            break;

       case IEEE80211_ELEMID_MOBILITY_DOMAIN:
            mdie = frm;
            break;

       case IEEE80211_ELEMID_RSN:
            rsnie = frm;
            break;
        }
        /* elem id + len = 2 bytes */
        frm += frm[1] + 2;
    }

    if (!ftie || !mdie || !rsnie)
        return -1;

    ftie_len = mdie_len = rsnie_len = 0;
    ies = NULL;

    mdie_len = mdie[1];
    if (mdie_len != (MD_IE_LEN - 2))
        return -1;
    mdie_len += 2; /* additional 2bytes for elem id and length field */

    ftie_len = ftie[1];
    if (ftie_len < (MIN_FT_IE_LEN - 2) || ftie_len > (MAX_FT_IE_LEN - 2))
        return -1;
    ftie_len += 2; /*additional 2bytes for elem id and length field*/

    rsnie_len = rsnie[1];
    if (rsnie_len < (MIN_RSN_IE_LEN - 2) || rsnie_len > (MAX_RSN_IE_LEN - 2))
        return -1;
    rsnie_len += 2; /*additional 2bytes for elem id and length field*/

    vap->iv_roam.iv_ft_params.fties_len = mdie_len + ftie_len + rsnie_len;
    vap->iv_roam.iv_ft_params.fties = (u_int8_t *)qdf_mem_malloc(vap->iv_roam.iv_ft_params.fties_len);
    if(!vap->iv_roam.iv_ft_params.fties) {
        vap->iv_roam.iv_ft_params.fties_len = 0;
        qdf_mem_zero(vap->iv_roam.iv_ft_params.target_ap, QDF_MAC_ADDR_SIZE);
        return -1;
    }

    ies = vap->iv_roam.iv_ft_params.fties;
    /* copy MDE */
    qdf_mem_copy(ies, mdie, mdie_len);
    ies += mdie_len;

    /* copy FTE */
    qdf_mem_copy(ies, ftie, ftie_len);
    ies += ftie_len;

    /* copy RSNE */
    qdf_mem_copy(ies, rsnie, rsnie_len);

    return EOK;
}

void mlme_sta_bmiss_ind(wlan_if_t vap)
{
    ieee80211_mlme_event   event;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    /* deliver MLME event */
    event.u.event_bmiss.cur_bmiss_count = vap->iv_bmiss_count;
    event.u.event_bmiss.max_bmiss_count = vap->iv_bmiss_count_for_reset;
    event.type = IEEE80211_MLME_EVENT_BEACON_MISS;
    ieee80211_mlme_deliver_event(mlme_priv, &event);

    /* indicate beacon miss */
    IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);
}

void
ieee80211_vap_iter_beacon_miss(void *arg, wlan_if_t vap)
{
    systime_t              tstamp;
    systime_t              last_link_time;
    systime_t              last_traffic_time;
#ifdef ATH_SWRETRY
    struct ieee80211com    *ic = vap->iv_ic;
#endif
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    ieee80211_mlme_event   event;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
                      "%s: %s iv_bmiss_count=%d reset=%d max=%d arg=%08p swbmiss=%d\n",
                      __func__,
                      (arg != NULL) ? "SW" : "HW",
                      vap->iv_bmiss_count,
                      vap->iv_bmiss_count_for_reset,
                      vap->iv_bmiss_count_max,
                      arg,
                      mlme_sta_swbmiss_active(vap));

    /*
     * Our handling is only meaningful for stations that are
     * associated; any other conditions else will be handled
     * through different means (e.g. the tx timeout on mgt frames).
     */
    if ((vap->iv_opmode != IEEE80211_M_STA) ||
        (wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        mlme_sta_swbmiss_timer_print_status(vap); /* print the status of sw bmiss */
        return;
    }

    /*
     * ignore HW beacon miss and completely rely on SW beacon miss
     * if 1. HW beacon processing is _not_ in use AND
     *    2. SW beacon miss is enabled.
     */
    if ((arg == NULL) && (!wlan_is_hwbeaconproc_active(vap) &&
            mlme_sta_swbmiss_active(vap))) {
        return;
    }

    /* When swbmiss comes, we need to ignore it when Hardware Beacon Processing is on */
    if ((arg != NULL) && wlan_is_hwbeaconproc_active(vap))
    {
        return ;
    }

    /*
     * WAR for excessive beacon miss problem on SoC.
     * Consider a beacon miss only when we have two consecutive
     * beacon misses and there are no rx activities in between.
     *
     * Count beacon misses only if we gave the AP a chance by sending a
     * directed Probe Request.
     *
     * Don't do anything if we are scanning a foreign channel.
     * Trying to transmit a frame (Probe Request) during a channel change
     * (which includes a channel reset) can cause a NMI due to invalid HW
     * addresses.
     * Trying to transmit a Probe Request while in a foreign channel
     * wouldn't do us any good either.
     *
     * Query current time only after retrieving LastLinkTime. This avoids
     * possible negative values if this routine is preempted by reception of
     * a beacon or directed frame which would update the fields used to
     * calculate LastLinkTime.
     */

    last_traffic_time = ieee80211_get_directed_frame_timestamp(vap);
    last_link_time = (vap->iv_last_beacon_time > last_traffic_time) ?
        vap->iv_last_beacon_time : last_traffic_time;
    tstamp = OS_GET_TIMESTAMP();

    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
            "%d.%03d | %s: count=%d probe=%d beacon:%lums directed:%lums data:%lums ap_frame:%lums traffic_ind:%lums\n",
            ((u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp)) / 1000,
            ((u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp)) % 1000,
            __func__, vap->iv_bmiss_count,
            wlan_scan_can_transmit(wlan_vdev_get_pdev(vap->vdev_obj)),
            (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp - vap->iv_last_beacon_time),
            (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp - last_traffic_time),
            (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp - vap->iv_lastdata),
            (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp - ieee80211_get_last_ap_frame_timestamp(vap)),
            (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(tstamp - vap->iv_last_traffic_indication));
    }

    /*
     * Do not count beacon misses received when we're off-channel, or
     * within IEEE80211_MINIMUM_BMISS_TIME ms of the last valid beacon.
     */
    if ((!wlan_scan_in_home_channel(wlan_vdev_get_pdev(vap->vdev_obj))) ||
        (CONVERT_SYSTEM_TIME_TO_MS(tstamp - last_link_time) < IEEE80211_MINIMUM_BMISS_TIME)) {
        mlme_sta_swbmiss_timer_start(vap); /* restart beacon miss timer */
        return;
    }

#if UMAC_SUPPORT_WNM
    /* Ignore bmiss in WNM-Sleep Mode */
    if (wlan_get_powersave(vap) == IEEE80211_PWRSAVE_WNM) {
        mlme_sta_swbmiss_timer_start(vap); /* restart beacon miss timer */
        return;
    }
#endif

    vap->iv_bmiss_count++;


    event.u.event_bmiss.cur_bmiss_count = vap->iv_bmiss_count;
    event.u.event_bmiss.max_bmiss_count = vap->iv_bmiss_count_for_reset;
    event.type = IEEE80211_MLME_EVENT_BEACON_MISS;

    ieee80211_mlme_deliver_event(mlme_priv,&event);

    if (vap->iv_bmiss_count < vap->iv_bmiss_count_for_reset) {
#ifdef ATH_SWRETRY
        /* Turn off the sw retry mechanism until we receive
         * any data frame or probe response for the BSS we are
         * associated to.
         */
        if (ic->ic_set_swretrystate)
            ic->ic_set_swretrystate(vap->iv_bss, FALSE);
#endif

        /*
         * It is possible that the hardware gets into
         * deaf mode. Reset the hardware to see if it can recover
         * from the condition.
         */

        /* indicate device error */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s: Beacon miss, do internal reset!!\n", __func__);
        IEEE80211_DELIVER_EVENT_DEVICE_ERROR(vap);

        mlme_sta_swbmiss_timer_start(vap); /* restart beacon miss timer */
        return;
    }

    /*  max bmiss count reached */

    vap->iv_bmiss_count = 0;    /* reset bmiss counter */

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: Beacon miss, will indicate to OS!!\n", __func__);
    /* indicate beacon miss */
    IEEE80211_DELIVER_EVENT_BEACON_MISS(vap);
}


void mlme_sta_reset_bmiss(struct ieee80211vap *vap)
{
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    ieee80211_mlme_event   event;

    vap->iv_bmiss_count = 0;    /* reset bmiss counter */

    event.type = IEEE80211_MLME_EVENT_BEACON_MISS_CLEAR;
    ieee80211_mlme_deliver_event(mlme_priv,&event);
}

void ieee80211_inact_timeout_sta(struct ieee80211vap *vap)
{

    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

    if ((mlme_priv==NULL) || !mlme_priv->im_connection_up ||
      (wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        return;
    }
    /*
     * if there was an activity in the last IEEE80211_INACT_WAIT period.
     * then reset the counter.
     */
    if (CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP() - vap->iv_last_directed_frame) < (IEEE80211_INACT_WAIT * 1000)) {
        vap->iv_inact_count = (vap->iv_keep_alive_timeout + IEEE80211_INACT_WAIT -1)/IEEE80211_INACT_WAIT;
    }
    if (vap->iv_inact_count) {
        --vap->iv_inact_count;
        if ((vap->iv_inact_count == 0) && (vap->iv_pwrsave_sta)) {
            ieee80211_send_nulldata(vap->iv_bss,false,false,NULL);
            if (vap->iv_keep_alive_timeout) {
                vap->iv_inact_count =  (vap->iv_keep_alive_timeout + IEEE80211_INACT_WAIT -1)/IEEE80211_INACT_WAIT;
            }
        }
    }
}

void mlme_sta_vattach(struct ieee80211vap *vap)
{
    mlme_sta_swbmiss_timer_attach(vap);
}

void mlme_sta_vdetach(struct ieee80211vap *vap)
{
    mlme_sta_swbmiss_timer_detach(vap);
}

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS

void mlme_cancel_stacac_timer(struct ieee80211vap *vap)
{
    struct ieee80211_mlme_priv *mlme_priv = vap->iv_mlme_priv;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "Canceling STA_CAC timer \n");

	OS_CANCEL_TIMER(&mlme_priv->im_stacac_timeout_timer);
}

bool mlme_is_stacac_running(struct ieee80211vap *vap)
{
    struct ieee80211_mlme_priv *mlme_priv = vap->iv_mlme_priv;

    return (mlme_priv->im_is_stacac_running);
}

void mlme_set_stacac_running(struct ieee80211vap *vap, u_int8_t set)
{
    struct ieee80211_mlme_priv *mlme_priv = vap->iv_mlme_priv;

    mlme_priv->im_is_stacac_running = set;
}

bool mlme_is_stacac_valid(struct ieee80211vap *vap)
{
    struct ieee80211com    *ic = vap->iv_ic;

    return IEEE80211_IS_CHAN_CAC_VALID(ic->ic_curchan);
}

void mlme_set_stacac_valid(struct ieee80211vap *vap, u_int8_t set)
{
    struct ieee80211com    *ic = vap->iv_ic;

    if(set) {
        IEEE80211_CHAN_SET_CAC_VALID(ic->ic_curchan);
    } else {
        IEEE80211_CHAN_CLR_CAC_VALID(ic->ic_curchan);
    }
}

void mlme_clear_mlme_req(struct ieee80211vap *vap)
{
    struct ieee80211_mlme_priv *mlme_priv = vap->iv_mlme_priv;

    mlme_priv->im_request_type = MLME_REQ_NONE;
}

void mlme_reset_mlme_req(struct ieee80211vap *vap)
{
    wlan_mlme_cancel(vap);
}

void mlme_stacac_restore_defaults(struct ieee80211vap *vap)
{
        mlme_set_stacac_valid(vap,0);
        mlme_cancel_stacac_timer(vap);
        mlme_reset_mlme_req(vap);
        mlme_set_stacac_running(vap,0);
}
static bool mlme_is_stacac_needed_by_stadfsen(struct ieee80211vap *vap)
{

    struct ieee80211com *ic = vap->iv_ic;
    uint32_t dfs_reg = 0;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    struct wlan_lmac_if_reg_rx_ops *reg_rx_ops;

    pdev = ic->ic_pdev_obj;
    if(!pdev) {
        qdf_err("null pdev");
        return false;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if (!psoc) {
        qdf_err("null psoc");
        return false;
    }

    reg_rx_ops = wlan_lmac_if_get_reg_rx_ops(psoc);
    if (!reg_rx_ops) {
        qdf_err("null reg_rx_ops");
        return false;
    }

    if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_REGULATORY_SB_ID) !=
            QDF_STATUS_SUCCESS) {
        return false;
    }
    reg_rx_ops->get_dfs_region(pdev, &dfs_reg);
    wlan_objmgr_pdev_release_ref(pdev, WLAN_REGULATORY_SB_ID);

    return ((DFS_ETSI_DOMAIN == dfs_reg) &&
            ieee80211com_has_cap_ext(ic,IEEE80211_CEXT_STADFS) &&
            IEEE80211_IS_CHAN_HISTORY_RADAR(ic, ic->ic_curchan)
           );
}
static bool mlme_is_stacac_needed_by_dependent_repeater(struct ieee80211vap *vap)
{
    struct ieee80211com    *ic = vap->iv_ic;

    return (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) &&
            IEEE80211_IS_CSH_CAC_APUP_BYSTA_ENABLED(ic)
           );
}
static bool mlme_is_stacac_needed_by_independent_repeater(struct ieee80211vap *vap)
{
    struct ieee80211com    *ic = vap->iv_ic;

    return (wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) &&
            !(IEEE80211_IS_CSH_CSA_APUP_BYSTA_ENABLED(ic)) &&
            IEEE80211_IS_CSH_CAC_APUP_BYSTA_ENABLED(ic)
           );
}
static bool mlme_is_chandfs_and_stacac_valid(struct ieee80211vap *vap)
{
    struct ieee80211com    *ic = vap->iv_ic;

    return (
            ((IEEE80211_IS_CHAN_DFS(ic->ic_curchan) ||
              ((IEEE80211_IS_CHAN_160MHZ(ic->ic_curchan) ||
                IEEE80211_IS_CHAN_80_80MHZ(ic->ic_curchan))
               && IEEE80211_IS_CHAN_DFS_CFREQ2(ic->ic_curchan)))
            ) &&
            (!mlme_is_stacac_valid(vap))
           );
}

bool mlme_is_stacac_needed(struct ieee80211vap *vap)
{
    /* Whether STA should do CAC or not
     * if ( (A || B || C) && D) {then do CAC }
     *
     * A:- If sta CAC is needed because of staDFSEn is set
     *     (mlme_is_stacac_needed_by_stadfsen)
     * B:- If sta CAC is needed becuase in Repeater Dependent mode
     *     sta should do CAC
     *     (mlme_is_stacac_needed_by_dependent_repeater)
     * C:- If TXCSA is not set and REPEATER_CAC is set
     *     (mlme_is_stacac_needed_by_independent_repeater)
     * D:- IF the channel is DFS and CAC is invalid
     *     (mlme_is_chandfs_and_stacac_valid)
     */

    return ((mlme_is_stacac_needed_by_stadfsen(vap) ||
             mlme_is_stacac_needed_by_dependent_repeater(vap) ||
             mlme_is_stacac_needed_by_independent_repeater(vap)
            )
            &&
            mlme_is_chandfs_and_stacac_valid(vap)
           );
}

#endif
void ieee80211_indicate_sta_radar_detect(struct ieee80211_node *ni)
{
     ieee80211_mlme_recv_csa(ni, IEEE80211_RADAR_DETECT_DEFAULT_DELAY,true);
}

void filter_ht20_dfs_channels(struct ieee80211com *ic,
                              struct ath_sub_chan_freq_list *oldchan_info,
                              struct ath_sub_chan_freq_list *newchan_info)
{
    int i, j = 0;

    for (i = 0; i < oldchan_info->cl_nchans; i++) {
        if (wlan_reg_is_dfs_for_freq(ic->ic_pdev_obj,
                                     oldchan_info->sub_20chan_freq[i])) {
            newchan_info->sub_20chan_freq[j++] = oldchan_info->sub_20chan_freq[i];
        }
    }
    newchan_info->cl_nchans = j;
}

bool is_subset_channel_for_cac(struct ieee80211com *ic, struct ieee80211_ath_channel *curchan,
        struct ieee80211_ath_channel *prevchan)
{
    struct ath_sub_chan_freq_list old_curchan_info = {0};
    struct ath_sub_chan_freq_list old_prevchan_info = {0};
    struct ath_sub_chan_freq_list cur_dfs_subchan_list = old_curchan_info;
    struct ath_sub_chan_freq_list prev_dfs_subchan_list = old_prevchan_info;
    uint8_t i, j;
    bool is_found = false;

    if (!IEEE80211_IS_CHAN_VALID(prevchan) || !IEEE80211_IS_CHAN_VALID(curchan)) {
        qdf_err("Channel pointer is NULL!");
        return is_found;
    } else if (!IEEE80211_IS_CHAN_5GHZ(curchan) ||
               !IEEE80211_IS_CHAN_5GHZ(prevchan)) {
        return true;
    }

    /* Get the list of dfs sub-channels in the previous channel */
    ieee80211_get_extchaninfo(ic, prevchan, &old_prevchan_info);
    filter_ht20_dfs_channels(ic, &old_prevchan_info, &prev_dfs_subchan_list);

    /* Get the list of dfs sub-channels in the current channel */
    ieee80211_get_extchaninfo(ic, curchan, &old_curchan_info);
    filter_ht20_dfs_channels(ic, &old_curchan_info, &cur_dfs_subchan_list);

    /* Repater AP does CAC if the number of sub-channels in current channel is greater
     * that of previous channel.
     */
    if ((curchan->ic_freq == prevchan->ic_freq) &&
            (cur_dfs_subchan_list.cl_nchans > prev_dfs_subchan_list.cl_nchans)) {
        return false;
    }

    for (i = 0; i < cur_dfs_subchan_list.cl_nchans; i++) {
        is_found = false;
        for (j = 0; j < prev_dfs_subchan_list.cl_nchans; j++) {
            if (cur_dfs_subchan_list.sub_20chan_freq[i] == prev_dfs_subchan_list.sub_20chan_freq[j]) {
                is_found = true;
                break;
            }
        }

        /* If RootAP channel is not found, skip checking the next channels */
        if (is_found == false) {
            qdf_debug("The sub-channels of RootAP's operating channel are not a subset of the RepeaterAP's operating channel \n");
            break;
        }
    }

    /* If is_found == 0, then Repeater AP has to do CAC */
    qdf_debug("is_found = %d \n", is_found);
    return is_found;
}
