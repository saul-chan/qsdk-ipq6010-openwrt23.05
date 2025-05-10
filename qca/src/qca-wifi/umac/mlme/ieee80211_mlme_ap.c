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
#include <target_if.h>
#if UNIFIED_SMARTANTENNA
#include <wlan_sa_api_utils_api.h>
#endif
#include <ieee80211_mlme_dfs_dispatcher.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <ieee80211_regdmn.h>
#if WLAN_SUPPORT_SPLITMAC
#include <wlan_splitmac.h>
#include "wlan_mlme_dispatcher.h"
#endif
#include <acfg_event.h>
#include <acfg_drv_event.h>

#include <wlan_cmn.h>
#include <wlan_objmgr_cmn.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_objmgr_peer_obj.h>

#include <target_type.h>
#include "wlan_crypto_global_def.h"
#include "wlan_crypto_global_api.h"
#include "wlan_mlme_dp_dispatcher.h"
#include "wlan_utility.h"
#if ATH_POWERSAVE_WAR
#define IEEE80211_PSPOLL_KICKOUT_THR 30000
#endif
#define RX_DECAP_TYPE_RAW            0
#include <target_type.h>
#include <wlan_utility.h>
#include <wlan_mlme_if.h>
#include <ieee80211_node_priv.h>
#include <wlan_repeater_api.h>
#include <wlan_repeater_internal.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <utils_mlo.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#endif
#include <wlan_mbss.h>
#include <ieee80211_wds.h>
#include <ieee80211_ucfg.h>
#include <ieee80211_cfg80211.h>
#include "cfg_ol.h"
#include "cfg_ucfg_api.h"

#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#endif
#ifdef WLAN_FEATURE_11BE_MLO
#define IS_SPLIT_PHY(no_of_radios) (no_of_radios > 1)
#endif

/* ieee80211_generic_linear_plot - Retreive output given slope and input coordinate
 *
 * uint8_t lower_bound_y: Min output value
 * uint8_t upper_bound_y: Max output value
 * uint16_t input1: Value to get input ratio
 * uint16_t input2: Value to get input ratio
 *
 */
static inline uint8_t ieee80211_linear_plot(uint8_t lower_bound_y,
                                            uint8_t upper_bound_y,
                                            uint16_t input1,
                                            uint16_t input2)
{

    return (lower_bound_y + ((input1 * (upper_bound_y - lower_bound_y)) / input2));

}

void ieee80211_estimate_dynamic_muedca_params(ieee80211_vap_t vap)
{
    struct ieee80211_muedca_state *muedca = &vap->iv_muedcastate;
    int iter;
    struct ieee80211com *ic = vap->iv_ic;
    enum ieee80211_phymode mode;
    const wmeParamType *pBssPhyParam;

    if (IEEE80211_IS_CHAN_VALID(vap->iv_bsschan))
        mode = ieee80211_chan2mode(vap->iv_bsschan);
    else
        mode = IEEE80211_MODE_AUTO;

/*
 * MU-EDCA param value (#ax_STAs, #legacy_STAs) = lower_bound + (1-#legacy_STAs/#total_STAs) * (upper_bound – lower_bound)
 */
    if (!vap->iv_sta_assoc) {
        vap->iv_muedcastate.mu_edca_dynamic_state |= MUEDCA_DYNAMIC_ALGO_UPDATE_STATE_MASK;
        qdf_debug("No STA connected");
        return;
    }
    /* Do not update param if only 11ax sta connected/disconnected,
     * resulting in all STAs being 11ax only.
     */
    if (!(vap->iv_muedcastate.mu_edca_dynamic_state & MUEDCA_DYNAMIC_ALGO_UPDATE_STATE_MASK)) {
        qdf_debug("Dynamic muedca state: %d - Change in 11ax client count only",
                  vap->iv_muedcastate.mu_edca_dynamic_state);
        vap->iv_muedcastate.mu_edca_dynamic_state |= MUEDCA_DYNAMIC_ALGO_UPDATE_STATE_MASK;
        return;
    }
    for(iter = 0; iter < MUEDCA_NUM_AC; iter++) {

        switch (iter) {
            case WME_AC_BK:
                pBssPhyParam = &ic->phyParamForAC[WME_AC_BK][mode];
                break;
            case WME_AC_VI:
                pBssPhyParam = &ic->bssPhyParamForAC[WME_AC_VI][mode];
                break;
            case WME_AC_VO:
                pBssPhyParam = &ic->bssPhyParamForAC[WME_AC_VO][mode];
                break;
            case WME_AC_BE:
            default:
                pBssPhyParam = &ic->bssPhyParamForAC[WME_AC_BE][mode];
                break;
    }

    vap->iv_muedcastate.muedca_paramList[iter].muedca_ecwmin =
    ieee80211_linear_plot(pBssPhyParam->logcwmin,
                          ic->ic_muedca_defaultParams[iter].muedca_ecwmin,
                          vap->iv_ax_sta_assoc,
                          vap->iv_sta_assoc);
    vap->iv_muedcastate.muedca_paramList[iter].muedca_aifsn =
    ieee80211_linear_plot(pBssPhyParam->aifsn,
                          ic->ic_muedca_defaultParams[iter].muedca_aifsn,
                          vap->iv_ax_sta_assoc,
                          vap->iv_sta_assoc);
    vap->iv_muedcastate.muedca_paramList[iter].muedca_timer =
    ieee80211_linear_plot(MUEDCA_TIMER_MIN,
                          ic->ic_muedca_defaultParams[iter].muedca_timer,
                          vap->iv_ax_sta_assoc,
                          vap->iv_sta_assoc);
    }

       muedca->muedca_param_update_count =
       ((muedca->muedca_param_update_count + 1) & MUEDCA_MAX_UPDATE_CNT);
       if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
           IEEE80211_DPRINTF(vap, IEEE80211_MSG_CU,
                   "%s: set cu cat2 flag for vdev id:%d ",__func__, vap->iv_unit);
           wlan_vdev_mlme_op_flags_set(vap->vdev_obj, WLAN_VDEV_OP_CU_CAT2);
       }
       wlan_vdev_beacon_update(vap);

}

void ieee80211_mlme_event_callback(ieee80211_vap_t vap,
                                   ieee80211_mlme_event *event,
                                   void *arg)
{
    switch (event->type) {
        case IEEE80211_MLME_EVENT_STA_JOIN:
        case IEEE80211_MLME_EVENT_STA_LEAVE:
            if (vap->iv_he_muedca == IEEE80211_MUEDCA_STATE_ENABLE &&
                vap->iv_ic->ic_muedca_mode_state == HEMUEDCA_HOST_DYNAMIC_MODE &&
                vap->iv_muedcastate.mu_edca_dynamic_state & MUEDCA_DYNAMIC_ALGO_ENABLE_STATE_MASK) {
                ieee80211_estimate_dynamic_muedca_params(vap);
            }
            break;
        default:
            break;
    }
}

#ifdef WLAN_FEATURE_11BE_MLO
static uint8_t wlan_get_num_rejected_partner_links(struct ieee80211_node *ni)
{
    struct wlan_objmgr_peer *link_peer;
    struct ieee80211_node *link_ni;
    struct wlan_mlo_link_peer_entry *peer_entry;
    struct wlan_mlo_peer_context *ml_peer = ni->peer_obj->mlo_peer_ctx;
    uint8_t cnt = 0;
    uint8_t i;

    if (!wlan_peer_is_mlo(ni->peer_obj))
        return 0;

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer->peer_list[i];
        link_peer = peer_entry->link_peer;

        if (!link_peer)
            continue;

        if (mlo_peer_is_assoc_peer(ml_peer, link_peer))
            continue;

        link_ni = wlan_peer_get_mlme_ext_obj(link_peer);
        if (!link_ni)
            continue;

        if (link_ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS)
            continue;

        cnt++;
    }

    return cnt;
}

static
enum wlan_link_band_caps wlan_peer_get_link_band(struct wlan_objmgr_peer *peer)
{
    struct wlan_objmgr_pdev *pdev = NULL;

    if (!peer) {
        qdf_err("Invalid peer");
        return WLAN_LINK_BAND_INVALID;
    }

    pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer));
    if (!pdev) {
        qdf_err("Invalid pdev");
        return WLAN_LINK_BAND_INVALID;
    }

    return wlan_cfg80211_link_band_info(pdev);
}

static
enum mlo_link_rej_cfg wlan_mlo_ap_get_link_reject_cfg(
                            struct wlan_objmgr_vdev *vdev)
{
    struct wlan_mlo_ap *ap_ctx;

    if (!vdev || !vdev->mlo_dev_ctx || !vdev->mlo_dev_ctx->ap_ctx)
        return MLO_LINK_REJ_DISABLED;

    ap_ctx = vdev->mlo_dev_ctx->ap_ctx;

    return ap_ctx->mlo_link_reject;
}

static QDF_STATUS wlan_check_and_reject_partner_links(struct ieee80211_node *ni)
{
    struct wlan_objmgr_peer *link_peer;
    struct ieee80211_node *link_ni;
    struct ieee80211_node *reject_ni;
    struct wlan_mlo_link_peer_entry *peer_entry;
    struct wlan_mlo_peer_context *ml_peer = ni->peer_obj->mlo_peer_ctx;
    uint8_t num_links_to_reject = 0;
    uint8_t num_links_rejected;
    uint8_t i;
    enum mlo_link_rej_cfg link_rej_config = 0;
    struct wlan_objmgr_vdev *vdev = NULL;
    enum wlan_link_band_caps link_band = 0;

    if (!wlan_peer_is_mlo(ni->peer_obj))
        return QDF_STATUS_SUCCESS;

    if (!ni->ni_vap->lre_test_num_assoc_link)
        ni->ni_vap->lre_test_num_assoc_link = WLAN_UMAC_MLO_ASSOC_MAX_SUPPORTED_LINKS;

    if (ml_peer->link_peer_cnt > ni->ni_vap->lre_test_num_assoc_link)
        num_links_to_reject = ml_peer->link_peer_cnt - ni->ni_vap->lre_test_num_assoc_link;

    /* Get the number of partner links already rejected */
    num_links_rejected = wlan_get_num_rejected_partner_links(ni);

    if (num_links_rejected >= num_links_to_reject)
        return QDF_STATUS_SUCCESS;

    vdev = wlan_peer_get_vdev(ni->peer_obj);
    link_rej_config = wlan_mlo_ap_get_link_reject_cfg(vdev);
    mlo_info("LRE num_assoc_link_sup:%u num_links_already_rej:%u",
             ni->ni_vap->lre_test_num_assoc_link, num_links_rejected);

    /* Iterate over the partner links and reject excess links based on least BW */
    while (num_links_rejected < num_links_to_reject) {
        reject_ni = NULL;

        for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
            peer_entry = &ml_peer->peer_list[i];
            link_peer = peer_entry->link_peer;

            if (!link_peer)
                continue;

            if (mlo_peer_is_assoc_peer(ml_peer, link_peer))
                continue;

            link_ni = wlan_peer_get_mlme_ext_obj(link_peer);
            if (!link_ni)
                continue;

            if (link_ni->ni_assocstatus != IEEE80211_STATUS_SUCCESS)
                continue;

            /*
             * 1. Avoid rejecting 2.4GHz band
             * 2. Make sure to select links across different bands instead of
             * selecting link on same band. i.e. 5G High vs 5G Low.
             */
            if (link_rej_config == MLO_LINK_REJ_RANGE) {
                link_band = wlan_peer_get_link_band(link_ni->peer_obj);
                if (link_band == WLAN_LINK_BAND_2GHZ)
                    continue;

                if (link_band == WLAN_LINK_BAND_5GHZ_LOW ||
                        link_band == WLAN_LINK_BAND_5GHZ_HIGH) {
                    /* Find least BW b/w 5G_LOW and 5G_HIGH */
                    if (!reject_ni || (link_ni->ni_chwidth < reject_ni->ni_chwidth))
                        reject_ni = link_ni;
                } else if (ni->ni_vap->lre_test_num_assoc_link <
                            WLAN_UMAC_MLO_ASSOC_MAX_SUPPORTED_LINKS) {
                    /* Link rejection test scenario only */
                    if (!reject_ni || (link_ni->ni_chwidth < reject_ni->ni_chwidth))
                        reject_ni = link_ni;
                }
            } else if (link_rej_config == MLO_LINK_REJ_THROUGHPUT) {
                if (!reject_ni || (link_ni->ni_chwidth < reject_ni->ni_chwidth))
                    reject_ni = link_ni;
            }
        }

        if (reject_ni) {
            qdf_info("MLD " QDF_MAC_ADDR_FMT " Exceeds max assoc links, reject " QDF_MAC_ADDR_FMT " chwidth %d",
                     QDF_MAC_ADDR_REF(&reject_ni->peer_obj->mldaddr[0]),
                     QDF_MAC_ADDR_REF(&reject_ni->ni_macaddr[0]), reject_ni->ni_chwidth);
            reject_ni->ni_assocstatus = IEEE80211_STATUS_UNSPECIFIED;
            num_links_rejected++;
        } else {
            qdf_err("MLD " QDF_MAC_ADDR_FMT " Exceeds max assoc links, No links available to reject, reject assoc",
                    QDF_MAC_ADDR_REF(&ni->peer_obj->mldaddr[0]));
            return QDF_STATUS_E_FAILURE;
        }
    }
    return QDF_STATUS_SUCCESS;
}
#endif

int ieee80211_mlme_recv_assoc_request(struct ieee80211_node *ni,
                                      u_int8_t reassoc,u_int8_t *vendor_ie, wbuf_t wbuf, struct ieee80211_rx_status *rs)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211com           *ic = ni->ni_ic;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    struct wlan_objmgr_vdev *vdev;
    u_int8_t                      newassoc = (ni->ni_associd == 0);
    wbuf_t                        resp_wbuf = NULL;
    u_int16_t                     assocstatus;
    ieee80211_mlme_event          event;
    u_int8_t                      flag=0;
    u_int8_t                      node_leave = 0, isvht, ishe;
    int32_t authmode;
    int32_t peer_authmode;
    int                           status = 0;
    int                           join_ret = 0;
    int                           subtype;
    uint16_t aid;
    int is_splitmac_enable = 0;
#if QCA_SUPPORT_SON
    struct wlan_assoc_disassoc_event_info info;
#endif
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
    int val = 0;
    uint8_t is_assoc_link = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    struct mlo_partner_info *ml_info = NULL;
    uint8_t i;
    u_int8_t no_of_radios = 0;
    struct ieee80211_node *temp_ni;
    bool mlo_link_reject = false;
#endif

    vdev = vap->vdev_obj;
    aid = -1;

#if WLAN_SUPPORT_SPLITMAC
    is_splitmac_enable = splitmac_is_enabled(vdev);
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    if (vdev->mlo_dev_ctx &&
        vdev->mlo_dev_ctx->ap_ctx &&
        vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject)
        mlo_link_reject = true;

    if (!ieee80211_mlo_is_mld_ap(vdev) ||
        !wlan_peer_is_mlo(ni->peer_obj) ||
        (ni->ni_ml_rptingrole == IEEE80211_MLO_RPTINGROLE_REPORTINGSTA)) {
        is_assoc_link = 1;
    }
#else
    is_assoc_link = 1;
#endif /* WLAN_FEATURE_11BE_MLO */


    /* AP  must be up and running */
     if (!mlme_priv->im_connection_up ||
        (wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        return 0;
    }

    /* Reject if vap is in legacy mode and 11ax or higher mode client tries to connect */
    if (vap->iv_diff_mode && ((ni->ni_phymode >= IEEE80211_MODE_11AXA_HE20) ||
                              (get_chwidth_phymode(ni->ni_phymode) > get_chwidth_phymode(vap->iv_cur_mode)))) {
        return 0;
    }

    IEEE80211_NOTE(vap, IEEE80211_MSG_MLME, ni, "%s", __func__);
    wlan_node_set_peer_state(ni, WLAN_ASSOC_STATE);

#ifdef WLAN_FEATURE_11BE_MLO
    if (ieee80211_mlo_is_mld_ap(vdev) &&
        wlan_peer_is_mlo(ni->peer_obj) &&
        mlo_link_reject &&
        ni->ni_assocstatus && !is_assoc_link) {
        assocstatus = ni->ni_assocstatus;
    } else
#endif
    {
        if (is_splitmac_enable && reassoc) {
            /* skip node join */
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                           "%s", "Skip node join, reassoc=%d \n",reassoc);
        } else {
            join_ret = ieee80211_node_join(ni, 0);
        }
        if (join_ret) {
            /* Association Failure */
            assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
        } else {
            assocstatus = IEEE80211_STATUS_SUCCESS;
            if (!reassoc) {
                ieee80211_admctl_node_leave(vap, ni);
            }

            /* Indicate that a new node has associated */
            event.type = IEEE80211_MLME_EVENT_STA_JOIN;
            event.u.event_sta.sta_count= vap->iv_sta_assoc;
            event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
            event.u.event_sta.ni = ni;
            ieee80211_mlme_deliver_event(mlme_priv,&event);
        }
    }

    if (reassoc)
        subtype = IEEE80211_FC0_SUBTYPE_REASSOC_REQ;
    else
        subtype = IEEE80211_FC0_SUBTYPE_ASSOC_REQ;

    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }

    if (rx_ops && rx_ops->son_rx_ops.process_mgmt_frame && wbuf && is_assoc_link) {
         val = rx_ops->son_rx_ops.process_mgmt_frame(vap->vdev_obj, ni->peer_obj, subtype, wbuf_header(wbuf),
                                                     wbuf_get_pktlen(wbuf), &assocstatus);
#ifdef WLAN_FEATURE_11BE_MLO
        /*
         * wlan_peer_mesh_override_flag overrides peer flags, w.r.t the value
         * assigned to the flag:
         * 1. (ENABLE - war) Always disable the AMSDUs on the vap
         * 2. (DISABLE - war) Do not set AMSDU levels based on peer flags
         */

        if (ni->ni_table && ni->ni_table->psoc) {
            no_of_radios =
                target_psoc_get_num_radios(ni->ni_table->psoc->tgt_if_handle);
            if (wlan_peer_is_mlo(ni->peer_obj) && IS_SPLIT_PHY(no_of_radios) &&
                (vap->wlan_peer_mesh_override_flag != WLAN_PEER_MESH_NODE_OVERRIDE_DISABLE)) {
                if ((vap->wlan_peer_mesh_override_flag ==
                     WLAN_PEER_MESH_NODE_OVERRIDE_ENABLE) ||
                    ((ni->peer_obj->peer_flags & WLAN_PEER_MESH_NODE) &&
                     !(ni->peer_obj->peer_flags & WLAN_PEER_QCN_NODE))) {
                    ic->ic_rx_amsdu = 0;
                }

            }
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "%s amsdu status: 0x%X, radio_count %d wlan_peer_mesh_override_flag %d: ",
            __func__, ic->ic_rx_amsdu, no_of_radios, vap->wlan_peer_mesh_override_flag);
        }
#endif // WLAN_FEATURE_11BE_MLO
    }
    if ((val > 0) && (assocstatus == IEEE80211_STATUS_SUCCESS)) {
        ieee80211_try_mark_node_for_delayed_cleanup(ni);
    }
    if (val > 0 || join_ret) {
        if (is_splitmac_enable) {
        /* splitmac has aid assigned from controller
         * clear aid if join failed */
            IEEE80211_AID_CLR(vap, ni->ni_associd);
            ni->ni_associd = 0;
        }
    }

    /* clear the last auth seq number */
    ni->ni_last_rxauth_seq = 0xfff;
    ni->ni_last_auth_rx_time = 0;

#ifdef WLAN_FEATURE_11BE_MLO
    /* Perform MLO peer create only for assoc link and new assoc case */
    if (is_assoc_link) {
       if (assocstatus == IEEE80211_STATUS_SUCCESS) {
          if (wlan_peer_is_mlo(ni->peer_obj)) {
                /* If AP receives Assoc req from station which already has MLO peer ctxt
                   drop Assoc req.
                   Station should start with Auth for new connection
                 */
               if (ni->peer_obj->mlo_peer_ctx) {
                    status = wlan_mlo_peer_asreq(vdev, ni->peer_obj, &ni->ml_info,
                                                 wbuf);
                    if (status == QDF_STATUS_E_INVAL) {
                        assocstatus = IEEE80211_REASON_ASSOC_EXPIRE;
                        mlo_err("vdev%d: link peers of MLO peer are not matching for MAC addr "QDF_MAC_ADDR_FMT,
                                    wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(ni->ni_macaddr));
                       return -EBUSY;
                    } else if (status == QDF_STATUS_E_FAILURE) {
                       mlo_err("vdev%d: Dropping duplicate Assoc req from MAC addr "QDF_MAC_ADDR_FMT,
                                    wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(ni->ni_macaddr));
                       return -EBUSY;
                    }
                    ni->ni_ml_link_peer_created = 1;
               } else {
                   if (!wbuf) {
                       mlo_err("psoc:%d vdev%d: Dropping assoc req processing, buf is NULL from MAC addr "QDF_MAC_ADDR_FMT,
                               wlan_psoc_get_id(wlan_vdev_get_psoc(vdev)),
                               wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(ni->ni_macaddr));
                       return -EBUSY;
                   }
                  /* Please check if there is ML Peer with any of partner link MAC address,
                     then reject assocation
                   */
                   ml_info = &ni->ml_info;
                   for (i = 0; i < ml_info->num_partner_links; i++) {
                        if (qdf_is_macaddr_equal((struct qdf_mac_addr *)ni->ni_macaddr,
                                                 &ml_info->partner_link_info[i].link_addr))
                            continue;

                        if(mlo_mgr_ml_peer_exist_on_diff_ml_ctx(
                             &ml_info->partner_link_info[i].link_addr.bytes[0],
                              NULL)) {
                            mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " partner peer create failed",
                                    QDF_MAC_ADDR_REF(&ml_info->partner_link_info[i].link_addr.bytes[0]));
                            assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
                            goto skip_mlo_peer_create;

                        }
                   }

#ifdef WLAN_FEATURE_11BE
                   ni->ml_info.t2lm_enable_val = ni->ni_t2lm_enable_val;
#endif /* WLAN_FEATURE_11BE */
#if WLAN_SUPPORT_SPLITMAC
                   if (splitmac_is_enabled(vdev)) {
                       aid = ni->ni_associd;
                   }
#endif
                   wlan_peer_mlme_set_assoc_peer(ni->peer_obj, true);

                   if(mlo_mgr_ml_peer_exist_on_diff_ml_ctx(
                        &ni->peer_obj->mldaddr[0],
                         NULL)) {
                       mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " skip ML peer create, forward assoc req to Hostapd",
                               QDF_MAC_ADDR_REF(&ni->peer_obj->mldaddr[0]));
                       wlan_peer_clear_mlo(ni->peer_obj);
                       assocstatus = IEEE80211_STATUS_SUCCESS;
                       goto skip_mlo_peer_create;

                   }
                   /*
                    * Before creating a new mlo peer, check whether another
                    * peer already exists with link mac addr that is same as
                    * the mld addr of current peer.
                    */
                   temp_ni = ieee80211_find_node(ic, &ni->peer_obj->mldaddr[0],
                                                 WLAN_MLO_MGR_ID);
                   if (temp_ni) {
                       if (temp_ni != ni) {
                           mlo_err("ML peer exists with this mld MAC " QDF_MAC_ADDR_FMT " Rejecting assoc",
                                   &ni->peer_obj->mldaddr[0]);
                           wlan_peer_clear_mlo(ni->peer_obj);
                           assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
                           /* release the ref taken by find_node() */
                           ieee80211_free_node(temp_ni, WLAN_MLO_MGR_ID);
                           goto skip_mlo_peer_create;
                       }
                       /* release the ref taken by find_node() */
                       ieee80211_free_node(temp_ni, WLAN_MLO_MGR_ID);
                   }
                   if (wlan_mlo_peer_create(vdev, ni->peer_obj, &ni->ml_info, wbuf, aid) !=
                                            QDF_STATUS_SUCCESS) {
                       assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
                       wlan_peer_clear_mlo(ni->peer_obj);

                       mlo_err("psoc:%d vdev%d: MLO peer create failed for MAC addr "QDF_MAC_ADDR_FMT,
                               wlan_psoc_get_id(wlan_vdev_get_psoc(vdev)),
                               wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(ni->ni_macaddr));
                   } else {
                       ni->ni_associd = ni->peer_obj->mlo_peer_ctx->assoc_id;
                       ni->ni_ml_peer_created = 1;
#ifdef WLAN_FEATURE_11BE_MLO
                       if (mlo_link_reject) {
                           /* Check the number of partner links requested by the MLO STA and
                            * reject links based on BW */
                           if (QDF_STATUS_SUCCESS != wlan_check_and_reject_partner_links(ni))
                               assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
                       }
#endif
                   }
               }
          }
          else {
              if ((!is_splitmac_enable) && (ni->ni_associd == 0)) {
                  if (wlan_vdev_mlme_is_mlo_ap(vdev) || (ic->is_mbssid_enabled)) {
                      aid = mlme_get_aid(vap->vdev_obj);
                      if (aid == (uint16_t)-1) {
                          /* status needs to be updated on local and ni member, local status is
                             used to prepare assoc resp
                           */
                          assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
                      }
                      else {
                          ni->ni_associd = aid;
                      }
                  }
                  else {
                      /*
                       * It would be good to search the bitmap
                       * more efficiently, but this will do for now.
                       */
                      aid = ieee80211_get_vap_start_aid(ic, vap);

                      for (; aid < vap->iv_max_aid; aid++)
                      {
                          if (!IEEE80211_AID_ISSET(vap, aid))
                              break;
                      }

                      if (aid >= vap->iv_max_aid) {
                          /*
                           * Keep stats on this situation.
                           */
                          IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni, "aid (%d)"
                                  " greater than max aid (%d)", aid,
                                  vap->iv_max_aid);
                          /* status needs to be updated on local and ni member, local status is
                             used to prepare assoc resp
                           */
                          assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
                      }
                      else {
                          ni->ni_associd = aid | 0xc000;
                          IEEE80211_AID_SET(vap, ni->ni_associd);
                      }
                  }
              }
          }
       }
    }
#endif

#ifdef WLAN_FEATURE_11BE_MLO
skip_mlo_peer_create:
#endif
    /* Clear any previously cached status */
    ni->ni_assocstatus = assocstatus;

    /* Association rejection from above */
    if (ni->ni_assocstatus != IEEE80211_STATUS_SUCCESS) {
        if (is_assoc_link) {
           /* Update already formed association response and send it out */
           resp_wbuf =  ieee80211_setup_assocresp(ni, NULL, reassoc, ni->ni_assocstatus, NULL, NULL, NULL, NULL);
           if (resp_wbuf)
               ieee80211_send_mgmt(vap,ni, resp_wbuf,false);

        }
#ifdef WLAN_FEATURE_11BE_MLO
        if (!is_assoc_link && mlo_link_reject)
            qdf_info("Partner link [%s] assoc failed with status %d", ether_sprintf(ni->ni_macaddr), ni->ni_assocstatus);
        else
#endif
        {
            /* Flag to remove the node from node table */
            node_leave = 1;
            status = -EBUSY;
        }
    } else {
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_authorize_attempt_inc(vap->vdev_obj, 1);
#endif
        /* 11AX : Recheck future 802.11ax drafts (>D1.0) on coex rules */
        if ((ni->ni_flags & IEEE80211_NODE_HT || IEEE80211_NODE_USE_HE(ni)
#ifdef WLAN_FEATURE_11BE
             || IEEE80211_NODE_USE_EHT(ni)
#endif
            ) && (ni->ni_flags & IEEE80211_NODE_40MHZ_INTOLERANT)) {
            /* If RSSI greater than/equal threshold then only do CW change */
            if (ni->ni_snr >= ic->obss_rx_snr_threshold) {
                ni->ni_set_40mhz_intol_bw = 1;
                ieee80211_change_cw(ic);
            } else {
                ni->ni_set_40mhz_intol_bw = 0;
            }
        }

        if (!ieee80211_vap_trigger_mlme_resp_is_set(vap) && is_assoc_link) {
            /* Setup association response frame before indication */
            resp_wbuf = ieee80211_setup_assocresp(ni, NULL, reassoc, assocstatus, NULL, NULL, rs, NULL);
            if (!resp_wbuf)
                assocstatus = IEEE80211_REASON_UNSPECIFIED;

            /* Move this down after sending the Assoc resp, so that the EAPOL
             * frame that is sent as consequence of this event, doesn't go OTA
             * before the Assoc Resp frame on some partial offload platforms. */
        
            /* Windows Platform have to call indication first to update ni->assocstatus
            * value, Keep original alg for Windows Platform.
            */
            /* Memory allocation failure, no point continuing */
            if (!resp_wbuf)
                return 0;

            flag = TRUE;
        }

        ni->ni_assocuptime = OS_GET_TICKS();
#if UMAC_SUPPORT_WNM
        ni->ni_wnm->last_rcvpkt_tstamp =  ni->ni_assocuptime;
#endif

        /* If null/dummy key plumbing for WEP is configured, set the appropriate
         * flag in key structure and call handler to plumb keys for all
         * 4 key indexes
         */
        if ((vap->iv_cfg_raw_dwep_ind) && (vap->iv_rx_decap_type ==
            RX_DECAP_TYPE_RAW)) {
        }

        isvht = (ni->ni_flags & IEEE80211_NODE_VHT);
        ishe = (ni->ni_ext_flags & IEEE80211_NODE_HE);
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC | IEEE80211_MSG_DEBUG, ni,
            "station %sassociated at aid %d: %s preamble, %s slot time"
            "%s%s%s%s cap 0x%x\n"
            , newassoc ? "" : "re"
            , IEEE80211_NODE_AID(ni)
            , ic->ic_flags & IEEE80211_F_SHPREAMBLE ? "short" : "long"
            , ic->ic_flags & IEEE80211_F_SHSLOT ? "short" : "long"
            , ic->ic_flags & IEEE80211_F_USEPROT ? ", protection" : ""
            , ni->ni_flags & IEEE80211_NODE_QOS ? ", QoS" : ""
            , ishe ? "HE" : (isvht ? "VHT" : (ni->ni_flags & IEEE80211_NODE_HT ? ", HT" : ""))
            , (ni->ni_flags & IEEE80211_NODE_HT)  ?
                       (ni->ni_htcap & IEEE80211_HTCAP_C_CHWIDTH40 ? "40" : "20") : ""
            , ni->ni_capinfo
        );
        wlan_node_set_peer_state(ni, WLAN_WAITKEY_STATE);
        if (is_assoc_link) {
            /* give driver a chance to setup state like ni_txrate */
#ifdef WLAN_FEATURE_11BE_MLO
            if ((!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ni->ni_flags & IEEE80211_NODE_AUTH)) &&
                 (is_splitmac_enable && !wlan_peer_is_mlo(ni->peer_obj))) {
#else
            if ((!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ni->ni_flags & IEEE80211_NODE_AUTH)) &&
                 (is_splitmac_enable)) {
#endif
                 newassoc = !ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                 if ((ic->ic_newassoc != NULL) && !(ni->is_ft_reassoc)) {
                     int retval = ic->ic_newassoc(ni, newassoc);
                     if (retval) {
                         qdf_err("node %s, failed to send peer assoc command, disconnect the STA", ether_sprintf(ni->ni_macaddr));
                         ieee80211_try_mark_node_for_delayed_cleanup(ni);
                         wlan_mlme_disassoc_request(vap,ni->ni_macaddr,IEEE80211_REASON_ASSOC_LEAVE);
                         return -1;
                     }
                     /* In case of OMN in Asoc req, phymode is not updated
                      * and therefore we explicitly send chwidth in OMN
                      * to target. This is confirmed by checking if chan width
                      * obtained from phymode and peer ch width differ.
                      */
                      if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
                          ic->ic_chwidth_change(ni);

                      ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                 }
            }

            if(ieee80211_is_pmf_enabled(vap, ni) &&
                vap->iv_opmode == IEEE80211_M_HOSTAP &&
                (vap->iv_skip_pmf_reassoc_to_hostap > 0) &&
                (ni->ni_flags & IEEE80211_NODE_AUTH))
            {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                            "[%s] drop assoc resp for pmf client from hostapd\n",
                            __func__);
                wlan_mlme_assoc_resp(vap,ni->ni_macaddr, IEEE80211_STATUS_REJECT_TEMP, 0, NULL);
            }

            if(flag) {
                ieee80211_send_mgmt(vap,ni,resp_wbuf,false);
#ifdef WLAN_FEATURE_11BE_MLO
                if (ni->ni_assocstatus == IEEE80211_STATUS_SUCCESS) {
                    if(!(ieee80211_is_pmf_enabled(vap, ni)) ||  !(ieee80211node_has_flag(ni,IEEE80211_NODE_AUTH))){
                        newassoc = !ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
                        if (((ic->ic_newassoc != NULL) && !(ni->is_ft_reassoc)) && !is_splitmac_enable) {
                            int retval = ic->ic_newassoc(ni, newassoc);
                            if (retval) {
                                qdf_err("node %s, failed to send peer assoc command, disconnect the STA", ether_sprintf(ni->ni_macaddr));
                                ieee80211_try_mark_node_for_delayed_cleanup(ni);
                                wlan_mlme_disassoc_request(vap,ni->ni_macaddr,IEEE80211_REASON_ASSOC_LEAVE);
                                return -1;
                            }
                        }
                    }
                }
#endif /* WLAN_FEATURE_11BE_MLO */
            }
        }
        if((ni->ni_capinfo & IEEE80211_CAPINFO_RADIOMEAS)
                && ieee80211_vap_rrm_is_set(vap))
        {
            ieee80211_set_node_rrm(ni,TRUE);
        }
        else {
            ieee80211_set_node_rrm(ni,FALSE);
        }

        /* To take care of shared wep condition,
           after challenge text success we are setting node to authorized
           so checking it here should be fine.*/
        if (ieee80211_node_is_authorized(ni)) {
#if QCA_SUPPORT_SON
	        wlan_acl_apply_node_snr_thresholds(vap, ni->ni_macaddr);
            qdf_mem_zero(&info, sizeof(info));
            qdf_mem_copy(info.macaddr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
            info.flag = ACTION_ASSOC;
            info.reason = ni->ni_assocstatus;
            if (rx_ops && rx_ops->son_rx_ops.deliver_event)
                rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_ASSOC_DISASSOC, &info);
#endif

#if ATH_PARAMETER_API
            ieee80211_papi_send_assoc_event(vap, ni, PAPI_STA_ASSOCIATION);
#endif
            }
        /*
         * Authorize the node when configured in open mode.
         * Node authorizations for other modes are initiated by hostapd.
         * Security modes configured at node & vap should match.
         */
        authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
        if ( authmode == -1 ) {
            qdf_err("crypto_err while getting authmode params\n");
            return -1;
        }

        peer_authmode = wlan_crypto_get_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
        if ( peer_authmode == -1 ) {
            qdf_err("crypto_err while getting peer_authmode params\n");
            return -1;
        }

        if((ni->ni_authmode != IEEE80211_AUTH_8021X) &&
           (!(peer_authmode & ((uint32_t)((1 << WLAN_CRYPTO_AUTH_WPA) | (1 << WLAN_CRYPTO_AUTH_RSNA)
                                     | (1 << WLAN_CRYPTO_AUTH_8021X) | (1 << WLAN_CRYPTO_AUTH_WAPI)))) &&
           (((!(authmode & (1 << WLAN_CRYPTO_AUTH_SHARED))) || (ni->ni_authalg != IEEE80211_AUTH_SHARED)))))
        {
#if QCA_SUPPORT_SON
	        wlan_acl_apply_node_snr_thresholds(vap, ni->ni_macaddr);
            qdf_mem_zero(&info, sizeof(info));
            qdf_mem_copy(info.macaddr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
            info.flag = ACTION_ASSOC;
            info.reason = ni->ni_assocstatus;
            if (rx_ops && rx_ops->son_rx_ops.deliver_event)
                rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, NULL, MLME_EVENT_ASSOC_DISASSOC, &info);
#endif
        }

        /* Update MIMO powersave flags and node rates */
        if ( !(ni->ni_flags & IEEE80211_NODE_AUTH) && is_splitmac_enable) {
            ieee80211_update_noderates(ni);
        }

        /* need to add a station join notification */

        if (!(ni->ni_capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT)) {
            /* Spectrum managemnt is not supported by this node */
            if (!(ni->ni_ext_flags & IEEE80211_NODE_NON_DOTH_STA)) {
                /* Non spectrum managemnt node associated for forst time */
                ic->ic_non_doth_sta_cnt++;
                ieee80211node_set_extflag(ni, IEEE80211_NODE_NON_DOTH_STA);
            }
        } else {
            /* Spectrum managemnt supported by this node */
            if ((ni->ni_ext_flags & IEEE80211_NODE_NON_DOTH_STA)) {
                /* Already associated sta (without spectrum management
                 * capability) reassociated with spectrum management
                 * capability.
                 * Mark this STA now supports sepectrum managemnet.
                 */
                ieee80211node_clear_extflag(ni, IEEE80211_NODE_NON_DOTH_STA);
                ic->ic_non_doth_sta_cnt--;
            }
        }
    }

    if ((!node_leave) && (is_assoc_link)) {
        /* Now send the notification and remove the node if needed */
        if (reassoc) {
#ifdef WLAN_FEATURE_11BE_MLO
            IEEE80211_DELIVER_EVENT_MLME_REASSOC_INDICATION(vap, ni->ni_macaddr,
                assocstatus, wbuf, resp_wbuf, &ni->peer_obj->mldaddr[0]);
        } else {
            IEEE80211_DELIVER_EVENT_MLME_ASSOC_INDICATION(vap, ni->ni_macaddr,
                assocstatus, wbuf, resp_wbuf, &ni->peer_obj->mldaddr[0]);
#else
            IEEE80211_DELIVER_EVENT_MLME_REASSOC_INDICATION(vap, ni->ni_macaddr,
                assocstatus, wbuf, resp_wbuf, NULL);
        } else {
            IEEE80211_DELIVER_EVENT_MLME_ASSOC_INDICATION(vap, ni->ni_macaddr,
                assocstatus, wbuf, resp_wbuf, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
#if QCA_SUPPORT_SSID_STEERING
            if (ieee80211_is_ssid_steering_vdev_pvt(vap)){
                IEEE80211_DELIVER_SSID_EVENT(vap, ni->ni_macaddr);
            }
#endif
        }
    }

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     * Handling TxOLE Address translation issue in RAW mode
     * Indicate controller through NL event to disable AMSDUs for 3rd party clients
     * Applicable for MLD devices
     */
    if ((!node_leave) && (vap->iv_tx_encap_type == htt_cmn_pkt_type_raw) &&
        (ni->ni_qcn_ie == NULL) && wlan_peer_is_mlo(ni->peer_obj) &&
        wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {
        if (ni->peer_obj->mlo_peer_ctx && (ni->peer_obj->mlo_peer_ctx->max_links > 1)) {
           wlan_cfg80211_hw_status_notification(vap, ni->peer_obj, false);
        } else if ((ni->peer_obj->mlo_peer_ctx && (ni->peer_obj->mlo_peer_ctx->max_links == 1)) &&
                   (!cfg_get(psoc, CFG_OL_ADV_RAW_MODE_WAR_ENABLE) ||
                   qdf_mem_cmp(vdev->vdev_mlme.macaddr, vdev->vdev_mlme.mldaddr, QDF_MAC_ADDR_SIZE) == 0)) {
           /*
            * For cases when ini enable_advance_rawmode_war is true, then send enable
            * event only when peer mld mac is same as peer link mac
            */
           wlan_cfg80211_hw_status_notification(vap, ni->peer_obj, true);
        }
    }
#endif /* WLAN_FEATURE_11BE_MLO */

#if DYNAMIC_BEACON_SUPPORT
    /*
     * STA assoc successfully,
     * resume beacon and trigger suspend beacon timer for iv_dbeacon_timeout.
     */
    if (vap->iv_dbeacon && vap->iv_dbeacon_runtime && assocstatus == IEEE80211_STATUS_SUCCESS) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "STA assoc successfully. Resume beacon \n");
        qdf_spin_lock_bh(&vap->iv_dbeacon_lock);
        if (ieee80211_mlme_beacon_suspend_state(vap)) {
            ieee80211_mlme_set_beacon_suspend_state(vap, false);
        }
        OS_SET_TIMER(&vap->iv_dbeacon_suspend_beacon, vap->iv_dbeacon_timeout*1000);
        qdf_spin_unlock_bh(&vap->iv_dbeacon_lock);
    }
#endif

    if (node_leave) {
	IEEE80211_NODE_LEAVE(ni);
    } else {
        ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_REQ);
    }

    return status;
}

/*
 *  create a insfra structure network (Host AP mode).
 */

int ieee80211_mlme_create_infra_continue(struct ieee80211vap *vap)
{
    struct ieee80211com         *ic = vap->iv_ic;
    bool skip_dfs_cac = false;
    uint8_t num_sta;
    struct wlan_objmgr_pdev *pdev;
    int status = 0;
    bool is_target_scan_radio;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_print("%s : pdev is null", __func__);
        return EINVAL;
    }

    /* Update channel and rates of the node */
    ieee80211_node_set_chan(vap->iv_bss);
    vap->iv_cur_mode = vap->iv_bss->ni_ch_param.ni_bss_phymode;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s \n", __func__);

#ifdef ATH_SUPPORT_DFS
    /*
     * Cancel CAC timer and reset valid bit if prevchan is valid and
     * is not same as current chan or has different flag than current chan
     */
    if (IEEE80211_IS_CHAN_VALID(ic->ic_prevchan)) {
        if (wlan_objmgr_pdev_try_get_ref(pdev, WLAN_DFS_ID) !=
            QDF_STATUS_SUCCESS) {
            return EINVAL;
        }

        mlme_dfs_cac_valid_reset_for_freq(pdev,
                                 ic->ic_prevchan->ic_freq,
                                 ic->ic_prevchan->ic_flags);
        wlan_objmgr_pdev_release_ref(pdev, WLAN_DFS_ID);
    }
#endif

    /*
     * Get total number of VAPs of each type supported by the IC.
     */
    num_sta = wlan_mbss_num_sta(ic->ic_pdev_obj);
    /* if station vap is present and in connected state, then skip DFS CAC timer */
    if (ic->ic_sta_vap &&
        (wlan_is_connected(ic->ic_sta_vap) ||
         wlan_pdev_mlme_op_get(pdev, WLAN_PDEV_OP_MBSSID_RESTART))
        && mlme_dfs_is_spoof_done(ic->ic_pdev_obj) &&
        !ic->ic_rpt_ap_needs_dfs) {
        skip_dfs_cac = true;
    }

#if ATH_SUPPORT_ZERO_CAC_DFS
    /*
     * If precac required timer expires it does a vdev_restart of all the active vaps
     * so that precac algorithm can pick the next channel. However vdev_restart response
     * also calls this function and tries to start the CAC. We do not want CAC for
     * vdev_restart by precac.
     */
    if(vap->iv_pre_cac_timeout_channel_change == 1) {
        vap->iv_pre_cac_timeout_channel_change = 0;
        IEEE80211_DPRINTF(vap,IEEE80211_MSG_DFS, "%s pre_cac channel change so skip cac\n",__func__);
        skip_dfs_cac = true;
    }
#endif

    if (vap->iv_no_cac) {
        skip_dfs_cac = true;
        vap->iv_no_cac = 0;
    }

    if(IEEE80211_IS_CHAN_VALID(ic->recovery_dfschan) &&
       IEEE80211_ARE_CHANS_EQUAL(ic->recovery_dfschan,ic->ic_curchan)) {
        qdf_print("%s: same DFS channel during recovery, skipping CAC", __func__);
        *ic->recovery_dfschan = empty_channel;
        skip_dfs_cac = true;
    }

    if (IEEE80211_IS_CSH_OPT_AVOID_DUAL_CAC_ENABLED(ic)) {
        if ((num_sta >= 1) && IEEE80211_IS_CHAN_VALID(ic->ic_prevchan)
	    && IEEE80211_IS_CHAN_VALID(ic->ic_curchan) &&
                IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(ic->ic_curchan)) {
            skip_dfs_cac = is_subset_channel_for_cac(ic, ic->ic_curchan, ic->ic_prevchan);
        }
    }

    is_target_scan_radio =
        wlan_pdev_nif_feat_ext_cap_get(pdev,
                                       WLAN_PDEV_FEXT_SCAN_RADIO);
    if (is_target_scan_radio) {
        bool is_dfs_disabled_for_scan_radio =
            wlan_pdev_nif_feat_ext_cap_get(pdev,
                                           WLAN_PDEV_FEXT_SCAN_RADIO_DFS_DIS);
        if (is_dfs_disabled_for_scan_radio)
            skip_dfs_cac = true;
    }

    if (IEEE80211_IS_CHAN_VALID(ic->ic_curchan) && wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq))
        ieee80211_send_tpc_power_cmd(vap);

    if (wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) && ic->ic_rpt_ap_needs_dfs) {
        qdf_debug("don't skip CAC for independent rpt mode with rpt_max_phy");
        skip_dfs_cac = false;
        ic->ic_rpt_ap_needs_dfs = false;
    }

    if (vap->restart_txn && ic->ic_is_restart_on_same_chan)
        skip_dfs_cac = true;

    if (IEEE80211_IS_SMART_MON_VAP(vap)) {
        qdf_info("Skipping CAC for smart monitor Vap");
        skip_dfs_cac = true;
    }

    if (skip_dfs_cac || ieee80211_dfs_cac_start(vap)) {

        /* In case skip_dfs_cac is set, reset CSA from chanswitch ioctl flag */
        if (ic->ic_flags_ext2 & IEEE80211_FEXT2_CSA_WAIT)
                ic->ic_flags_ext2 &= ~IEEE80211_FEXT2_CSA_WAIT;

        if (!ic->ic_nl_handle) {
            /* NON DFS channel, Start host ap */
            skip_dfs_cac = true;
            wlan_vdev_mlme_sm_deliver_evt_sync(vap->vdev_obj,
                                      WLAN_VDEV_SM_EV_START_SUCCESS, 0, NULL);
        }
    } else {
        wlan_mlme_inc_act_cmd_timeout(vap->vdev_obj,
                        WLAN_SER_CMD_VDEV_START_BSS);
        wlan_vdev_mlme_sm_deliver_evt_sync(vap->vdev_obj,
                                  WLAN_VDEV_SM_EV_DFS_CAC_WAIT, 0, NULL);
    }
    /* On every vap start/restart, set the CAC states of the dfs_curchan
     * and dfs_prevchan appropriately.
     */
    mlme_dfs_deliver_cac_state_events_for_prevchan(ic->ic_pdev_obj);
    if (skip_dfs_cac && !IEEE80211_IS_SMART_MON_VAP(vap))
        ieee80211_dfs_deliver_events(ic, ic->ic_curchan, WLAN_EV_CAC_COMPLETED);

    return status;
}

enum ieee80211_phymode
ieee80211_derive_max_phy(enum ieee80211_phymode des_mode,
                         struct ieee80211_ath_channel *chan)
{
    enum ieee80211_mode des_phy;
    enum ieee80211_phymode chan_mode;
    enum phy_ch_width chwidth;
    int8_t sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCN;

    chwidth = ieee80211_get_phy_chan_width(chan);
    des_phy = get_mode_from_phymode(des_mode);
    chan_mode = ieee80211_chan2mode(chan);

    if (ieee80211_is_phymode_40plus(chan_mode))
        sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCA;
    else if (ieee80211_is_phymode_40minus(chan_mode))
        sec_offset = IEEE80211_SEC_CHAN_OFFSET_SCB;

    return ieee80211_get_composite_phymode(des_phy,
                                           chwidth,
                                           sec_offset);
}

struct ieee80211_ath_channel
ieee80211_find_desired_chan(struct ieee80211vap *vap,
                      struct ieee80211_ath_channel *new_chan,
                      enum ieee80211_phymode mode)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;
    enum ieee80211_phymode final_phymode;
    struct chan_params chan_p = {0};

    if (!IEEE80211_IS_CHAN_VALID(new_chan) ||
        (vap->iv_des_mode ==  IEEE80211_MODE_AUTO))
        return empty_channel;

    chan_p.freq = new_chan->ic_freq;
    chan_p.cfreq2 = new_chan->ic_vhtop_freq_seg2;
    set_chan_params_puncture_bitmap(&chan_p, new_chan->op_puncture_bitmap);

    *chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                         mode);
    if (!IEEE80211_IS_CHAN_VALID(chan)) {
        final_phymode = ieee80211_derive_max_phy(mode, new_chan);
        *chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                             final_phymode);
    }
    return *chan;
}

struct ieee80211_ath_channel
ieee80211_use_max_phy_for_rep_ap(struct ieee80211vap *vap,
                                 struct ieee80211_ath_channel *new_chan)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;
    enum phy_ch_width des_chwidth;
    enum phy_ch_width cur_chwidth;
    enum ieee80211_phymode final_phymode;
    enum ieee80211_phymode des_phymode;
    struct chan_params chan_p = {0};

    if (!ieee80211_ic_rpt_max_phy_is_set(ic))
        return empty_channel;

    ic->ic_rpt_ap_needs_dfs = 0;

    /*Not a repeatar*/
    if (!ic->ic_sta_vap)
        return empty_channel;

    /*Change only AP phy mode. STA follows root AP's Phy*/
    if (vap->iv_opmode != IEEE80211_M_HOSTAP)
        return empty_channel;

    /*
     * Apply mode change only for first 2 VAPs(first AP and STA)
     * Rest of the VAPs can follow cur_chan for mode
     */
    if (!wlan_rptr_pdev_is_eir(ic->ic_pdev_obj) &&
        ieee80211_get_num_active_vaps(ic) > 2)
        return empty_channel;

    /* Not supported for QWRAP */
    if (wlan_rptr_pdev_is_eir(ic->ic_pdev_obj))
        if (wlan_rptr_pdev_is_qwrap(ic->ic_pdev_obj))
            return empty_channel;

    des_phymode = vap->iv_des_mode;
    if (!IEEE80211_IS_CHAN_VALID(new_chan))
        return empty_channel;

    if (vap->iv_des_mode == IEEE80211_MODE_AUTO) {
        if (vap->iv_des_hw_mode == IEEE80211_MODE_AUTO)
            return empty_channel;
        des_phymode = vap->iv_des_hw_mode;
    }

    des_chwidth = ieee80211_get_chan_width_from_phymode(des_phymode);
    cur_chwidth = ieee80211_get_phy_chan_width(new_chan);

    qdf_debug("rpt AP VAP's des_chwidth=%d, rpt STA VAP's des_chwidth=%d",
              des_chwidth, cur_chwidth);

    chan_p.freq = new_chan->ic_freq;
    chan_p.cfreq2 = new_chan->ic_vhtop_freq_seg2;
    set_chan_params_puncture_bitmap(&chan_p, new_chan->op_puncture_bitmap);

    *chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                         des_phymode);
    if (IEEE80211_IS_CHAN_VALID(chan) &&
        (des_chwidth > cur_chwidth)) {
        if (IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(chan)) {
            /*
             * Max phy chan is DFS chan here. Rpt AP VAP should perform CAC
             * before coming up in this chan.
             * E.g. Rpt AP VAP's des_chan=100HT160 and rpt STA VAP's
             * des_chan=100HT80. Mark CAC is required here before
             * coming up in 100HT160.
             */
            qdf_debug("CAC required for rpt_max_phy chan");
            ic->ic_rpt_ap_needs_dfs = 1;
        } else {
            /*
             * Configure chan corresponds to max phy of rpt AP if chan
             * is non-dfs.
             * E.g. Rpt AP VAP's des_chan=36HT80 and Rpt STA VAP's
             * des_chan=36HT20. Configure 36HT80 since it is non-dfs.
             */
            qdf_debug("CAC not required for rpt_max_phy chan");
        }
    }
    if (!IEEE80211_IS_CHAN_VALID(chan) || IEEE80211_IS_CHAN_RADAR(ic, chan)) {
        /*
         * For some cases, it is not possible to set max phymode.
         * E.g:
         * 1. Rpt AP VAP's phymode=HT160, and root is operating in
         *    channel 149VHT20. In this case chan will be NULL. Try to
         *    configure max phy with Root AP's chanwidth.
         *    For above example, if rpt supported phy is 11AX, derive 149HE20.
         * 2. Lets say second segment of 36HT160 is RADAR affected in Rpt.
         *    Rpt AP VAP's phymode=HT160, and root is operating in
         *    channel 36HT80. Above logic derives channel as 36HT160.
         *    since it is RADAR affected, Rpt can't come-up in this channel.
         * To address these cases, try to configure max phy with
         * Root AP's chanwidth.
         * Since Root and Rpt BW are same, CAC is not required in this case.
         */
        ic->ic_rpt_ap_needs_dfs = 0;
        final_phymode = ieee80211_derive_max_phy(des_phymode, new_chan);
        qdf_debug("Max chwidth for rpt ap not possible. Configure phymode %d",
                  final_phymode);
        *chan = ieee80211_find_dot11_channel(ic, &chan_p,
                                                 final_phymode);
    }
    return *chan;
}

struct ieee80211_ath_channel ieee80211_derive_chan(struct ieee80211vap *vap)
{
    struct ieee80211_ath_channel chan_obj = *vap->iv_des_chan[vap->iv_des_mode];
    struct ieee80211_ath_channel *chan = &chan_obj;
    struct ieee80211com *ic = vap->iv_ic;
    int coex_enabled;
    int num_bridge_vap = 0;
    wlan_if_t tmpvap;
    struct wlan_channel *iter_vdev_chan = NULL;
    struct ieee80211_ath_channel max_phy_chan_obj = empty_channel;
    struct ieee80211_ath_channel *max_phy_chan = &max_phy_chan_obj;
    uint8_t numvaps_up = 0, diff_mode = 0;
    struct chan_params chan_p = {0};

    /* if num active vaps is not 0
     * make the vap to follow the current channel instead of desired
     * channel
     */
    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
       if (!tmpvap)
           continue;
       if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP) &&
           IEEE80211_IS_MLO_BRIDGE_VAP(tmpvap)) {
           continue;
       }
       iter_vdev_chan = wlan_vdev_get_active_channel(tmpvap->vdev_obj);
       if (iter_vdev_chan) {
           *max_phy_chan = ieee80211_use_max_phy_for_rep_ap(vap,
                                                            ic->ic_curchan);
           break;
       }
    }

    TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
       if (!tmpvap)
           continue;
       if ((tmpvap->iv_opmode == IEEE80211_M_HOSTAP) &&
           IEEE80211_IS_MLO_BRIDGE_VAP(tmpvap) &&
           (QDF_STATUS_SUCCESS == wlan_vdev_mlme_is_active(tmpvap->vdev_obj))) {
               num_bridge_vap++;
       }
    }

    /* Check if max phy mode can be used for AP VAP of rpt.
     * If so derive chan using desired chan's flags */
    if (iter_vdev_chan && !IEEE80211_IS_CHAN_VALID(max_phy_chan) &&
        (!ic->ic_diff_mode || ic_is_sta_vap(ic))) {
        chan_p.freq = iter_vdev_chan->ch_freq;
        chan_p.cfreq2 = iter_vdev_chan->ch_cfreq2;
        set_chan_params_puncture_bitmap(&chan_p, iter_vdev_chan->puncture_bitmap);

        *chan = ieee80211_find_channel(ic, &chan_p,
                                      iter_vdev_chan->ch_flags);
        if (!IEEE80211_IS_CHAN_VALID(chan))
           *chan = *vap->iv_des_chan[vap->iv_des_mode];
    } else if (IEEE80211_IS_CHAN_VALID(max_phy_chan)) {
        *chan = *max_phy_chan;
    } else if (ic->ic_diff_mode) {
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (!tmpvap->iv_diff_mode && ieee80211_is_vap_state_running(tmpvap)) {
                diff_mode = 1;
                break;
            }
        }
        if (diff_mode) {
            *chan = *tmpvap->iv_des_chan[tmpvap->iv_des_mode];
            if (!IEEE80211_IS_CHAN_VALID(chan))
                *chan = *vap->iv_des_chan[vap->iv_des_mode];
        }
    }

    if (!IEEE80211_IS_CHAN_VALID(chan))
        return empty_channel;

    numvaps_up = ieee80211_get_num_ap_vaps_up(ic);
    coex_enabled = !(ic->ic_flags & IEEE80211_F_COEXT_DISABLE);

    numvaps_up -= num_bridge_vap;

    /* Use the current operating channel if numvaps_up is non-zero */
    if (coex_enabled && !numvaps_up) {
        if (IEEE80211_IS_CHAN_BW_40MHZ(chan) &&
                IEEE80211_IS_CHAN_2GHZ(chan)) {
            enum ieee80211_phymode mode;
            /*
             *  If we are in 40+ or 40- mode and if channel intolerant bit
             *  is set then its better to use 20 mode, because 40 PhyMode
             *  would be cause a performance drop as PHY has to monitor Ext
             *  channel too.
             */
#ifdef WLAN_FEATURE_11BE
            if (IEEE80211_IS_CHAN_EHT(chan)) {
               mode = IEEE80211_MODE_11BEG_EHT20;
            } else
#endif /* WLAN_FEATURE_11BE */
            if (IEEE80211_IS_CHAN_HE(chan)) {
               mode = IEEE80211_MODE_11AXG_HE20;
            } else {
               mode = IEEE80211_MODE_11NG_HT20;
            }

            if (IEEE80211_IS_CHAN_40INTOL(ic, chan)) {
                struct ieee80211_ath_channel    chan20_obj;
                struct ieee80211_ath_channel    *chan20 = &chan20_obj;
                struct chan_params chan_p = {0};

                chan_p.freq = chan->ic_freq;
                chan_p.cfreq2 = 0;
                /* Find the HT20 channel info */
                *chan20 = ieee80211_find_dot11_channel(ic, &chan_p,
                                                       mode);
                if (IEEE80211_IS_CHAN_VALID(chan20)) {
                    qdf_debug("Overriding HT40 channel with HT20 channel");
                    *chan = *chan20;
                }
                else {
                    qdf_debug("Unable to find HT20 channel in mode %d",mode);
                }
            }
        }
    }

    if (IEEE80211_IS_CHAN_DISALLOW_HOSTAP(chan)) {
        *chan = empty_channel;
        return *chan;
    }

    /* Enable vap_doth bit for 5 GHz and 6 GHz channel when the current pdev
     * supports 2 GHz and user has not disabled vap_doth.
     */
    if (!vap->iv_user_disabled_vap_doth &&
        ic->ic_pdev_is_2ghz_supported(ic) &&
        (IEEE80211_IS_CHAN_5GHZ(chan) ||
         IEEE80211_IS_CHAN_6GHZ(chan))) {
        ieee80211_vap_doth_set(vap);
    }

    /* Setup BSS color  */
    ieee80211_setup_bsscolor(vap, chan);

    return *chan;
}

int
mlme_create_infra_bss(struct ieee80211vap *vap, u_int8_t restart)
{
    struct ieee80211com         *ic = vap->iv_ic;
    ieee80211_ssid              *ssid = NULL;
    int                         n_ssid;
    int                         error = 0;
    u_int8_t acs_report_scan_active = 0;
    bool scan_active = false;
    struct ieee80211_ath_channel chan_obj;
    struct ieee80211_ath_channel *chan = &chan_obj;
    QDF_STATUS sm_ret;
#ifdef CONFIG_AFC_SUPPORT
    enum reg_afc_dev_deploy_type reg_afc_deploy_type;
    enum reg_6g_ap_type reg_cur_6g_ap_pwr_type;
#endif
#ifdef WLAN_FEATURE_11BE
    struct wlan_rptr_global_priv *g_priv = NULL;
    struct rptr_ext_cbacks *ext_cb = NULL;
    struct wlan_objmgr_vdev *sta_vdev = NULL;
    struct wlan_objmgr_vdev *vdev_obj = NULL;
    struct wlan_objmgr_pdev *pdev;
    int i;
#endif

    n_ssid = ieee80211_get_desired_ssid(vap, 0,&ssid);

    if (ssid == NULL)
        return EINVAL;

#ifdef WLAN_FEATURE_11BE
    /* If repeater is configured as dependent WDS, bring down all standalone
     * ML AP vaps in repeater until the other AP vaps in the AP ML context
     * are up.
     */
    g_priv = wlan_rptr_get_global_ctx();
    ext_cb = &g_priv->ext_cbacks;

    if ((ic->ic_sta_vap == 0) && wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) {
         for (i = 0; i < WLAN_UMAC_MLO_MAX_VDEVS; i++) {
              // Get all ML AP vaps in AP ML context
              vdev_obj = vap->vdev_obj->mlo_dev_ctx->wlan_vdev_list[i];
              if (!vdev_obj)
                  continue;

              if (vdev_obj != vap->vdev_obj) {
                  pdev = wlan_vdev_get_pdev(vdev_obj);
                  sta_vdev = ext_cb->get_stavap(pdev);
                  if (!sta_vdev)
                      break;

                  if ((wlan_vdev_is_up(sta_vdev) != QDF_STATUS_SUCCESS) &&
                      !wlan_rptr_pdev_is_eir(pdev)) {
                      mlme_nofl_err("ML STAs are not connected, return");
                      return EAGAIN;
                  }
              }
         }
    }
#endif

    /*
     * if there is a scan in progress.
     * then there is a vap currently scanning and the chip
     * is off on a different channel. we can not bring up
     * vap at this point.  Scan can be in progress for
     * independant repeater vaps, since they do not change channels.
     *
     * When the resmgr is active, do not fail vap creation even if a scan is in
     * progress.
     * In repeater dependent mode, if STA vap is not associated do not bring up
     * AP vaps unless the it is a mesh VAP.
     */
    acs_report_scan_active = wlan_get_param(vap, IEEE80211_START_ACS_REPORT);

    if (!ieee80211_resmgr_active(ic) &&
           ((scan_active = wlan_scan_in_progress(vap) &&
             !(ieee80211_ic_offchanscan_is_set(ic) || acs_report_scan_active)) ||
             ieee80211_sta_assoc_in_progress(ic))
#if MESH_MODE_SUPPORT
             && !vap->iv_mesh_vap_mode
#endif
        && !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {

        if (scan_active)
            ic->schedule_bringup_vaps = true;

        mlme_nofl_err("psoc:%d vdev:%d: Scan is in progress, return",
                       wlan_psoc_get_id(wlan_vdev_get_psoc(vap->vdev_obj)),
                       wlan_vdev_get_id(vap->vdev_obj));
        return EAGAIN;
    }

#ifdef CONFIG_AFC_SUPPORT
    wlan_reg_get_afc_dev_deploy_type(ic->ic_pdev_obj, &reg_afc_deploy_type);
    wlan_reg_get_cur_6g_ap_pwr_type(ic->ic_pdev_obj, &reg_cur_6g_ap_pwr_type);

    if (wlan_reg_is_6ghz_chan_freq(ic->ic_curchan->ic_freq) &&
        !vap->iv_special_vap_mode && ic->ic_sta_vap) {
        if (reg_afc_deploy_type == AFC_DEPLOYMENT_OUTDOOR &&
            reg_cur_6g_ap_pwr_type == REG_STANDARD_POWER_AP) {
                if (!wlan_reg_is_afc_power_event_received(ic->ic_pdev_obj) ||
                    !wlan_reg_is_afc_done(ic->ic_pdev_obj, ic->ic_curchan->ic_freq)) {
                        mlme_nofl_err("vdev:%d: AFC not done, dont bring up vap, return",
                                       wlan_vdev_get_id(vap->vdev_obj));
                        return EAGAIN;
                 }
         }
    }
#endif

    /* create BSS node for infra network */
    error = ieee80211_create_infra_bss(vap,ssid->ssid, ssid->len);

    if (error) {
        goto err;
    }
    if (ieee80211_mixed_modes_config(vap, vap->iv_des_mode)) {
        *chan = *vap->iv_des_chan[vap->iv_des_mode];
    } else {
        *chan = ieee80211_derive_chan(vap);
    }
    if (!IEEE80211_IS_CHAN_VALID(chan)) {
        qdf_err("Channel is NULL");
        return EINVAL;
    }

    ieee80211_update_vdev_chan(vap->vdev_obj->vdev_mlme.des_chan, chan);

    sm_ret = wlan_vdev_mlme_sm_deliver_evt(vap->vdev_obj,
                                      WLAN_VDEV_SM_EV_START, 0, NULL);
    if (sm_ret != QDF_STATUS_SUCCESS) {
        qdf_err("SM event delivery failed");
        return EINVAL;
    }

err:
    return error;
}
void ieee80211_mlme_create_infra_continue_async(struct ieee80211vap *vap, int32_t status)
{
    int error = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: status %d\n", __func__,status);
    if (status == EOK) {
        error = ieee80211_mlme_create_infra_continue(vap);
    }

    if (!error) {
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, (status == EOK) ? IEEE80211_STATUS_SUCCESS : IEEE80211_STATUS_REFUSED);
    } else {
        IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, IEEE80211_STATUS_REFUSED);
    }
}

/*
 * function to handle shared auth in HOST AP mode.
 */

u_int16_t
mlme_auth_shared(struct ieee80211_node *ni, u_int16_t seq, u_int16_t status,
                 u_int8_t *challenge,u_int16_t challenge_len)
{
    struct ieee80211vap    *vap = ni->ni_vap;
    struct ieee80211com    *ic = ni->ni_ic;
    u_int16_t              estatus = IEEE80211_STATUS_SUCCESS;

    /*
     * NB: this can happen as we allow pre-shared key
     * authentication to be enabled w/o wep being turned
     * on so that configuration of these can be done
     * in any order.  It may be better to enforce the
     * ordering in which case this check would just be
     * for sanity/consistency.
     */
    estatus = 0;            /* NB: silence compiler */
    if (!IEEE80211_VAP_IS_PRIVACY_ENABLED(vap)) {
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                           ni->ni_macaddr, "shared key auth",
                           "%s", " PRIVACY is disabled");
        estatus = IEEE80211_STATUS_ALG;
    }

    if (estatus == IEEE80211_STATUS_SUCCESS) {
        switch (seq) {
        case IEEE80211_AUTH_SHARED_CHALLENGE:
        case IEEE80211_AUTH_SHARED_RESPONSE:
            if (challenge == NULL) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                                   ni->ni_macaddr, "%s\n", "shared key auth no challenge");
#ifdef QCA_SUPPORT_CP_STATS
                vdev_cp_stats_rx_auth_err_inc(vap->vdev_obj, 1);
#endif
                estatus = IEEE80211_STATUS_CHALLENGE;
            } else if (challenge_len != IEEE80211_CHALLENGE_LEN) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                                   "shared key auth bad challenge len %d", challenge_len);
#ifdef QCA_SUPPORT_CP_STATS
                vdev_cp_stats_rx_auth_err_inc(vap->vdev_obj, 1);
#endif
                estatus = IEEE80211_STATUS_CHALLENGE;
            }
        default:
            break;
        }
    }

    if (estatus == IEEE80211_STATUS_SUCCESS) {
        switch (seq) {
        case IEEE80211_AUTH_SHARED_REQUEST:
            if (ni->ni_challenge == NULL)
                ni->ni_challenge = (u_int32_t *)OS_MALLOC(ic->ic_osdev ,IEEE80211_CHALLENGE_LEN,0);
            if (ni->ni_challenge == NULL) {
                IEEE80211_NOTE(ni->ni_vap,
                               IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                               "%s", "shared key challenge alloc failed");
                /* XXX statistic */
                estatus = IEEE80211_STATUS_UNSPECIFIED;
            } else {
                /*
                 * get random bytes for challenge text.
                 */

                OS_GET_RANDOM_BYTES(ni->ni_challenge,
                                    IEEE80211_CHALLENGE_LEN);
                IEEE80211_NOTE(vap,
                               IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                               "%s", "shared key auth request \n");
                ieee80211_send_auth(ni,(seq + 1),0,(u_int8_t *)ni->ni_challenge,IEEE80211_CHALLENGE_LEN,NULL,NULL);
            }
            break;
        case IEEE80211_AUTH_SHARED_RESPONSE:
            if (ni->ni_challenge == NULL) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                                   ni->ni_macaddr, "shared key response",
                                   "%s", "no challenge recorded");
#ifdef QCA_SUPPORT_CP_STATS
                vdev_cp_stats_rx_auth_err_inc(vap->vdev_obj, 1);
#endif
                estatus = IEEE80211_STATUS_CHALLENGE;
            } else if (memcmp(ni->ni_challenge, challenge,
                              challenge_len) != 0) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                                   ni->ni_macaddr, "shared key response",
                                   "%s", "challenge mismatch");
#ifdef QCA_SUPPORT_CP_STATS
                vdev_cp_stats_rx_auth_fail_inc(vap->vdev_obj, 1);
#endif
                estatus = IEEE80211_STATUS_CHALLENGE;
            } else {
                IEEE80211_NOTE(vap, IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                               "station authenticated (%s)\n", "shared key");
                ieee80211_node_authorize(ni);
                /*
                 * shared auth success.
                 */
                ieee80211_send_auth(ni,(seq + 1),0, NULL,0,NULL,NULL);
            }
            break;
        default:
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                               ni->ni_macaddr, "shared key auth ",
                               "bad seq %d \n", seq);
#ifdef QCA_SUPPORT_CP_STATS
            vdev_cp_stats_rx_auth_err_inc(vap->vdev_obj, 1);
#endif
            estatus = IEEE80211_STATUS_SEQUENCE;
            break;
        }
    }

    /*
     * Send an error response.
     */
    if (estatus != IEEE80211_STATUS_SUCCESS) {
        ieee80211_send_auth(ni,(seq + 1),estatus, NULL,0,NULL,NULL);
    }

    return estatus;
}

static int mlme_recv_auth_ap_handle_duplicate(struct ieee80211_node *ni,
                                            u_int16_t algo, u_int16_t seq, u_int16_t frame_seq,
                                            bool* create_new_node)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    u_int16_t associd = 0;
    /*
     * Check is frame is duplicate frame or not.
     * check the received sequence number and compare previous seq number of auth frame.
     */
    if((ni->ni_last_rxauth_seq == frame_seq) && (algo != IEEE80211_AUTH_ALG_SAE)){
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                    "recv duplicate auth frame seq num %d\n",frame_seq);
        return -1;
    } else if (ni->ni_last_auth_rx_time != 0
        && seq != IEEE80211_AUTH_SHARED_RESPONSE && (algo != IEEE80211_AUTH_ALG_SAE)){
        /*
         * Check is auth frame is recevied before a valid assoc request is given
         * ni_last_auth_rx_time is made as '0' on receving assoc request to node.
         * if ni_last_auth_rx_time is not '0' then already auth is in progress.
         * if frame is recevied within nominal auth + assoc req time,
         * then drop this auth. As already one more frame in progress.
         */
        systime_t now;
        now = OS_GET_TIMESTAMP();
        if(CONVERT_SYSTEM_TIME_TO_MS(now - ni->ni_last_auth_rx_time) < 100){
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                "recv another auth frame when previous is in progress\n");
            return -1;
        }
    }

    ni->ni_last_auth_rx_time = OS_GET_TIMESTAMP();
    ni->ni_last_rxauth_seq = frame_seq;

    if ((seq == IEEE80211_AUTH_OPEN_REQUEST ||
         seq == IEEE80211_AUTH_SHARED_REQUEST) &&
         (algo != IEEE80211_AUTH_ALG_FT) && (algo != IEEE80211_AUTH_ALG_SAE)) {
        /* if receive the re-auth frame without any disassoc check if node is at power save mode
            let the sta leave the power save state. */
        if ((ni->ni_flags & IEEE80211_NODE_PWR_MGT) == IEEE80211_NODE_PWR_MGT)
        {
            ieee80211_mlme_node_pwrsave_ap(ni,0);
        }
        /* Leave the node only if PMF not enabled */
        if (!(ieee80211_is_pmf_enabled(vap, ni) && ieee80211_node_is_authorized(ni)) && ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_REQ)) {
            if(!ieee80211_vap_trigger_mlme_resp_is_set(vap)) {
                if (!ieee80211_try_ref_node(ni, WLAN_MLME_HANDLER_ID))
                    return -1;

                associd = ni->ni_associd;
#ifdef WLAN_FEATURE_11BE_MLO
                if (wlan_peer_is_mlo(ni->peer_obj)) {
                    wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                }
#endif
                if(IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                    IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                            ni->ni_macaddr, associd, IEEE80211_REASON_ASSOC_LEAVE,
                            &ni->peer_obj->mldaddr[0]);
#else
                    IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                            ni->ni_macaddr, associd, IEEE80211_REASON_ASSOC_LEAVE,
                            NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                }
                ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);
            } else {
                if (qdf_atomic_read(&(ni->ni_auth_tx_completion_pending))) {
                    IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                            "recv auth when AUTH TX completion is pending...\n");
                    return -1;

                } else {
                    ieee80211_try_mark_node_for_delayed_cleanup(ni);
                    wlan_mlme_disassoc_request(vap,ni->ni_macaddr,IEEE80211_REASON_ASSOC_LEAVE);
                    IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                            "recv auth when valid node is present ignore auth and disconnect sta\n");
                    return -1;
                }
            }
        } else {
            /* PMF is enabled so don't create new node */
            *create_new_node = FALSE;
        }
    } else if (seq == IEEE80211_AUTH_SHARED_RESPONSE || algo == IEEE80211_AUTH_ALG_FT
                || algo == IEEE80211_AUTH_ALG_SAE){
        /*
         * Second auth in AUTH_ALG_SHARED or
         * Auth request with FT as algo for roaming
         */
        *create_new_node = FALSE;
#ifdef WLAN_FEATURE_11BE_MLO
        /* Leave the node only if node is MLO peer and not authorized */
        if (ni->peer_obj->mlo_peer_ctx && !ieee80211_node_is_authorized(ni)) {
             ieee80211_try_mark_node_for_delayed_cleanup(ni);
             wlan_mlme_disassoc_request(vap,ni->ni_macaddr,IEEE80211_REASON_ASSOC_LEAVE);
             IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                     "recv auth when valid node is present and not authorized ignore auth and disconnect sta\n");
             return -1;
        }
#endif
    } else {
        /*
         * Invalid auth frame in AP mode.
         */
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr,
                   "Invalid auth frame with algo %d AuthSeq %d \n",algo,seq);
        return -1;
    }

  return 0;
}

struct ieee80211_node* find_logically_deleted_node_on_soc(struct wlan_objmgr_psoc *psoc,
        const uint8_t *macaddr, const uint8_t *bssid, wlan_objmgr_ref_dbgid id)
{
    uint8_t pdev_id;
    struct ieee80211_node* ni_temp = NULL;
    struct wlan_objmgr_pdev *pdev;
    struct ieee80211com    *ic;

    if(psoc == NULL)
        return NULL;

    for (pdev_id = 0; pdev_id < WLAN_UMAC_MAX_PDEVS; pdev_id++) {
        pdev = wlan_objmgr_get_pdev_by_id(psoc, pdev_id, id);
        if (pdev != NULL) {
            ic = wlan_pdev_get_mlme_ext_obj(pdev);
            if (ic != NULL)
                ni_temp = _ieee80211_find_logically_deleted_node(ic,
                        macaddr, bssid, id);

            wlan_objmgr_pdev_release_ref(pdev, id);

            if (ni_temp != NULL)
                break;
        }
    }

    return ni_temp;
}

#ifdef AST_HKV1_WORKAROUND
int mlme_find_and_delete_wds_before_auth(struct ieee80211vap *vap, uint8_t *mac,
                                         struct recv_auth_params_defer *auth_params)
{
    struct ieee80211com *ic = vap->iv_ic;
    wbuf_t cwbuf = NULL;

    if (ic->ic_get_tgt_type(ic) != TARGET_TYPE_QCA8074)
        return -1;

    if (ic->ic_node_lookup_wds_and_del) {
        struct recv_auth_params_defer *auth_cookie = NULL;

        if (auth_params) {
            auth_cookie = qdf_mem_malloc(sizeof(struct recv_auth_params_defer));
            if (!auth_cookie) {
                return -1;
            }
            qdf_mem_copy(auth_cookie, auth_params, sizeof(struct recv_auth_params_defer));
            cwbuf  =  wbuf_clone(vap->iv_ic->ic_osdev, auth_params->wbuf);
            if (!cwbuf) {
                qdf_mem_free(auth_cookie);
                return -1;
            }
            auth_cookie->wbuf = cwbuf;
        }

        if (ic->ic_node_lookup_wds_and_del(vap->iv_ifp, mac, auth_cookie) < 0) {
            if (cwbuf)
                wbuf_free(cwbuf);
            if (auth_cookie)
                qdf_mem_free(auth_cookie);
            return -1;
        }
        /* auth_cookie will be freed in WDS del completion handler */
        return 0;
    }

    return -1;
}
#endif

/*
 * get_auth_frame_tlv_start: This API is called to get the start of the
 *                           TLVs (i.e., Information Elements) in the auth
 *                           frame
 * @auth_frame: Begining of the Auth frame including the 802.11 header
 * @algo: Auth algorithm
 * @auth_tlv_start_ptr: OUT pointer to set to the beginning of the
 *                      TLVs (or Information Elements) within the Auth frame
 * auth_frame_len: Length of the Auth frame
 *
 * Return: QDF_STATUS_SUCCESS in the case of success, QDF_STATUS value giving
 * the reason for error in the case of failure
 */
static QDF_STATUS get_auth_frame_tlv_start(uint8_t *auth_frame,
        uint16_t algo,
        uint8_t **auth_tlv_start_ptr,
        int auth_frame_len)
{
    uint16_t sae_group_id = 0;
    struct ieee80211_mgmt *mgmt;
    uint8_t *auth_body_ptr;
    int auth_body_len;
    static const int WLAN_SAE_STATUS_HASH_TO_ELEMENT = 126;
    static const int WLAN_SAE_STATUS_PK = 127;
    static const int SAE_GROUP_19_FIXED_FIELDS_LEN = 96;
    static const int SAE_GROUP_20_FIXED_FIELDS_LEN = 144;
    static const int SAE_GROUP_21_FIXED_FIELDS_LEN = 198;
    static const int SAE_MSG_TYPE_COMMIT = 1;

    /*
     * Assume that Auth frame has enough length to hold
     * initial common fields for all types of auth frames
     */
    mgmt = (struct ieee80211_mgmt *)auth_frame;
    auth_body_ptr = mgmt->u.auth.variable;
    auth_body_len = auth_frame_len - (mgmt->u.auth.variable - auth_frame);

    /*
     * Non-SAE Auth frame handling
     */
    if ((algo == IEEE80211_AUTH_ALG_OPEN) ||
            (algo == IEEE80211_AUTH_ALG_SHARED) ||
            (algo == IEEE80211_AUTH_ALG_FT)) {
        *auth_tlv_start_ptr = auth_body_ptr;
        return QDF_STATUS_SUCCESS;
    } else if (algo != IEEE80211_AUTH_ALG_SAE) {
        return QDF_STATUS_E_NOSUPPORT;
    }

    /*
     * SAE Auth frame requirements
     */
    if (mgmt->u.auth.auth_transaction != SAE_MSG_TYPE_COMMIT)
        return QDF_STATUS_E_NOSUPPORT;
    if (mgmt->u.auth.status_code != WLAN_SAE_STATUS_HASH_TO_ELEMENT &&
            mgmt->u.auth.status_code != WLAN_SAE_STATUS_PK)
        return QDF_STATUS_E_NOSUPPORT;

    /*
     * First field after the common auth fields is the
     * SAE Group-ID
     */
    if (auth_body_len < sizeof(uint16_t)) {
        qdf_err("ERROR! not enough bytes for SAE Auth frame\n");
        return QDF_STATUS_E_NULL_VALUE;
    }
    sae_group_id = *(uint16_t *)(auth_body_ptr);
    auth_body_ptr += sizeof(uint16_t);
    auth_body_len -= sizeof(uint16_t);

    switch (sae_group_id) {
        /*
         * Only SAE groups 19,20 and 21 are supported
         */
        case 19:
            if (auth_body_len < SAE_GROUP_19_FIXED_FIELDS_LEN)
                return QDF_STATUS_E_NOSUPPORT;

            auth_body_ptr += SAE_GROUP_19_FIXED_FIELDS_LEN;
            break;
        case 20:
            if (auth_body_len < SAE_GROUP_20_FIXED_FIELDS_LEN)
                return QDF_STATUS_E_NOSUPPORT;

            auth_body_ptr += SAE_GROUP_20_FIXED_FIELDS_LEN;
            break;
        case 21:
            if (auth_body_len < SAE_GROUP_21_FIXED_FIELDS_LEN)
                return QDF_STATUS_E_NOSUPPORT;

            auth_body_ptr += SAE_GROUP_21_FIXED_FIELDS_LEN;
            break;
        default:
            return QDF_STATUS_E_NOSUPPORT;
    }

    *auth_tlv_start_ptr = auth_body_ptr;
    return QDF_STATUS_SUCCESS;
}

int mlme_recv_auth_ap(struct ieee80211_node *ni,
                       u_int16_t algo, u_int16_t seq, u_int16_t status_code,
                       u_int8_t *challenge, u_int8_t challenge_length, wbuf_t wbuf,
                       const struct ieee80211_rx_status *rs)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    struct ieee80211_frame        *wh;
    u_int16_t                     frame_seq;
    bool create_new_node = TRUE;
    struct recv_auth_params_defer auth_params;
    struct ieee80211_node *ni_temp = NULL; /*to hold logically deleted node */
    struct wlan_objmgr_peer *peer = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    systime_t now;
    uint32_t delta;
    int ret = 0;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_dev_context *ml_dev;
    struct wlan_mlo_peer_context *ml_peer;
#ifdef UMAC_MLO_AUTH_DEFER
    struct mlpeer_auth_params *mlo_auth_params = NULL;
    struct ieee80211_rx_status *mlo_auth_rs;
    qdf_nbuf_t cwbuf;
#endif
    QDF_STATUS status = QDF_STATUS_E_FAILURE;
    bool auth_mlie_parse_supported = true;
    uint8_t *efrm, *frm = NULL;
    uint8_t *ml_ie = NULL;
    qdf_size_t ml_ie_len = 0;
    struct qdf_mac_addr mldmacaddr;
    bool mldmac_found = false;
    enum wlan_ml_variant variant;
#endif

    peer = ni->peer_obj;
    if (peer)
        vdev = wlan_peer_get_vdev(peer);

    if (!vdev) {
        qdf_print("%s: null vdev", __func__);
        return -1;
    }
    wh = (struct ieee80211_frame *) wbuf_header(wbuf);

    /* AP must be up and running */
    if (!mlme_priv->im_connection_up || 
        (wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                        "VAP is not up? im_connection_up=%d, vdev up=%d\n",
                        mlme_priv->im_connection_up, wlan_vdev_is_up(vap->vdev_obj));
        return -1;
    }

    IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                       "recv auth frame with algorithm %d seq %d \n", algo, seq);
    frame_seq = ((le16toh(*(u_int16_t *)wh->i_seq)) & IEEE80211_SEQ_SEQ_MASK) >> IEEE80211_SEQ_SEQ_SHIFT;

        /* Always remove the old client node. Otherwise, station count can be wrong */
    if (ni != vap->iv_bss)  {
#if WLAN_SUPPORT_SPLITMAC
        if (splitmac_is_enabled(vdev)) {
            if(splitmac_api_get_state(peer) == SPLITMAC_ADDCLIENT_START) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                   "recv auth frame when assoc is in progress %d\n",frame_seq);
                return -1;
            }
        }
#endif
        if(mlme_recv_auth_ap_handle_duplicate(ni, algo, seq, frame_seq, &create_new_node))
            return -1;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    else if (wlan_vdev_mlme_is_mlo_ap(vdev)) {
       ml_dev = vdev->mlo_dev_ctx;
       if (!ml_dev) {
           IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                            "ML DEV is NUL\n");
           return -1;
       }

       /* Derive MLD MAC address */
       efrm = wbuf_header(wbuf) + wbuf_get_pktlen(wbuf);
       if (get_auth_frame_tlv_start(((uint8_t *)wh), algo, &frm, efrm - ((uint8_t *)wh)))
           auth_mlie_parse_supported = false;

       if (auth_mlie_parse_supported)
	       status = util_find_mlie(frm, (efrm - frm), &ml_ie, &ml_ie_len);
       if (QDF_IS_STATUS_SUCCESS(status) && ml_ie) {
           status = util_get_mlie_variant(ml_ie, ml_ie_len, (int *)&variant);
           if (status != QDF_STATUS_SUCCESS) {
               mlo_err("Get MLIE variant failed");
           }
           else {
               if (variant != WLAN_ML_VARIANT_BASIC) {
                   mlo_err("MLIE variant is not basic");
               }
               else {
                   status = util_get_bvmlie_mldmacaddr(ml_ie, ml_ie_len, &mldmacaddr);
                   if (status != QDF_STATUS_SUCCESS) {
                       mlo_err("Get MLIE MLD MAC failed");
                   }
                   else {
                       mldmac_found = true;
                   }
               }
           }
       }

       ml_peer = wlan_mlo_get_mlpeer_by_linkmac(ml_dev, (struct qdf_mac_addr *)wh->i_addr2);
       if ((!ml_peer) && (mldmac_found)) {
           ml_peer = wlan_mlo_get_mlpeer(ml_dev, &mldmacaddr);
       }
       if (!ml_peer) {
           ml_peer = wlan_mlo_get_mlpeer(ml_dev, (struct qdf_mac_addr *)wh->i_addr2);
           if (!ml_peer) {
               if (mlo_mgr_ml_peer_exist_on_diff_ml_ctx(wh->i_addr2, NULL)) {
                   if (wlan_vap_is_pmf_enabled(vap)) {
                       mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " on different MLD ctx, send auth req to hapd",
                               QDF_MAC_ADDR_REF(wh->i_addr2));
                       create_new_node = 0;
                       goto skip_del;
                   } else {
                       mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " on different MLD ctx, rejecting auth req",
                               QDF_MAC_ADDR_REF(wh->i_addr2));
                       return -1;
                   }
               }
           } else {
               if (wlan_vap_is_pmf_enabled(vap)) {
                   mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " send auth req to hapd",
                           QDF_MAC_ADDR_REF(wh->i_addr2));
                   create_new_node = 0;
                   goto skip_del;
               } else {
                   mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " rejecting auth req",
                           QDF_MAC_ADDR_REF(wh->i_addr2));
                   return -1;
               }
           }
       }
       if (ml_peer) {
          if (wlan_mlo_peer_is_disconnect_progress(ml_peer) == QDF_STATUS_SUCCESS) {
             IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                          "recv auth frame from station, its MLO connection is in disconnection %d\n",frame_seq);
#ifdef UMAC_MLO_AUTH_DEFER
               /* Need to hold AUTH request till ML PEER is freed */
              mlo_auth_params = qdf_mem_malloc(sizeof(struct mlpeer_auth_params));
              if (!mlo_auth_params) {
                  IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                           "recv auth frame from station, its MLO connection, but auth param alloc failed %d\n",frame_seq);
                  return -1;
              }
              mlo_auth_rs = qdf_mem_malloc(sizeof(struct ieee80211_rx_status));
              if (!mlo_auth_rs) {
                  IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                           "recv auth frame from station, its MLO connection, but auth param rs alloc failed %d\n",frame_seq);
                  qdf_mem_free(mlo_auth_params);
                  return -1;
              }

              cwbuf = wbuf_clone(vap->iv_ic->ic_osdev, wbuf);
              if (!cwbuf) {
                  IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                           "recv auth frame from station, its MLO connection, but wbuf clone failed %d\n",frame_seq);
                  qdf_mem_free(mlo_auth_rs);
                  qdf_mem_free(mlo_auth_params);
                  return -1;
              }
              mlo_auth_params->psoc_id = wlan_vdev_get_psoc_id(vdev);
              mlo_auth_params->vdev_id = wlan_vdev_get_id(vdev);
              qdf_copy_macaddr(&mlo_auth_params->link_addr, (struct qdf_mac_addr *)wh->i_addr2);
              if (mldmac_found) {
                  qdf_copy_macaddr(&mlo_auth_params->mldaddr, &mldmacaddr);
              }
              else {
                  qdf_zero_macaddr(&mlo_auth_params->mldaddr);
              }
              mlo_auth_params->algo = algo;
              mlo_auth_params->seq = seq;
              mlo_auth_params->status_code = status_code;
              mlo_auth_params->challenge = challenge;
              mlo_auth_params->challenge_length = challenge_length;
              mlo_auth_params->wbuf = cwbuf;
              qdf_mem_copy(mlo_auth_rs, rs, sizeof(struct ieee80211_rx_status));
              mlo_auth_params->rs = mlo_auth_rs;

              status = mlo_peer_link_auth_defer(ml_peer, (struct qdf_mac_addr *)wh->i_addr2, mlo_auth_params);
              if (status != QDF_STATUS_SUCCESS) {
                  IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                           "recv auth frame from station, its MLO connection, but auth defer is failed %d\n",frame_seq);
                  mlo_peer_free_auth_param(mlo_auth_params);
                  return -1;
              }
#endif
          }
          else {
             if (wlan_vap_is_pmf_enabled(vap)) {
                 mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " send auth req to hapd",
                         QDF_MAC_ADDR_REF(wh->i_addr2));
                 create_new_node = 0;
                 goto skip_del;
             }
             IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                           "recv auth frame from station, which is having active MLO connection, intiating delete on active connection %d\n",frame_seq);
             wlan_mlo_peer_delete(ml_peer);
          }
          return -1;
       }
    }
    else {
        /* if ML peer exists with the link MAC address, reject auth request */
       if(mlo_mgr_ml_peer_exist_on_diff_ml_ctx(wh->i_addr2, NULL)) {
           mlo_err("ML Peer exists with this MAC " QDF_MAC_ADDR_FMT " rejecting auth req",
                   QDF_MAC_ADDR_REF(wh->i_addr2));
           return -1;
       }
    }
skip_del:
#endif
    /*
     * 1. If node delete in progress take a refrence for that node.
     * 2. else call defer function immediatly.
     *
     */
    auth_params.algo = algo;
    auth_params.seq = seq;
    auth_params.status_code = status_code;
#ifdef WLAN_FEATURE_11BE_MLO
    if (mldmac_found) {
        qdf_copy_macaddr(&auth_params.mldaddr, &mldmacaddr);
    }
    else {
        qdf_zero_macaddr(&auth_params.mldaddr);
    }
#endif
    auth_params.challenge = challenge;
    auth_params.challenge_length = challenge_length;
    auth_params.wbuf = wbuf;
    auth_params.vdev_id = wlan_vdev_get_id(vdev);
    qdf_mem_copy(&auth_params.rs, rs, sizeof(struct ieee80211_rx_status));

    if (create_new_node) {
        ni_temp = find_logically_deleted_node_pdev_psoc(vap->iv_ic,wh->i_addr2,
                                                        WLAN_MLME_SB_ID);
        if (ni_temp != NULL) {
            now = OS_GET_TIMESTAMP();
            delta = CONVERT_SYSTEM_TIME_TO_MS(now - ni->ss_last_data_time);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_PEER_DELETE,
                    "%s: node 0x%pK, mac:%s logically deleted, delta:%d\n",
                    __func__, ni_temp, ether_sprintf(ni_temp->ni_macaddr), delta);
            if (!qdf_atomic_read(&ni_temp->ni_fw_peer_delete_rsp_pending)) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_PEER_DELETE,
                        "delete response pending is ZERO continue AUTH \n");
                ieee80211_free_node(ni_temp, WLAN_MLME_SB_ID);
                ret = mlme_recv_auth_ap_defer(ni, create_new_node, &auth_params, 0);
            } else {
                qdf_nbuf_t cwbuf;

                cwbuf  =  wbuf_clone(vap->iv_ic->ic_osdev,
                                      auth_params.wbuf);
                if (!cwbuf) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_PEER_DELETE,
                                       "wbuf clone failed: %s\n",
                                       ether_sprintf(ni_temp->ni_macaddr));
                    ieee80211_free_node(ni_temp, WLAN_MLME_SB_ID);
                    return -1;
                }

                IEEE80211_NODE_STATE_LOCK(ni_temp);
                /* If auth in progress and wbuf is NULL means, previous pending
                 * auth is in processing, so return
                 */
                if ((ni->auth_inprogress) && !(ni_temp->auth_params.wbuf)) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_PEER_DELETE,
                                   "pending auth is in process, so return: %s\n",
                                    ether_sprintf(ni_temp->ni_macaddr));
                    ieee80211_free_node(ni_temp, WLAN_MLME_SB_ID);
                    IEEE80211_NODE_STATE_UNLOCK(ni_temp);
                    wbuf_free(cwbuf);
                    return -1;

                }
                /* If another AUTH frame while waiting for peer del response,
                 * free the old one and use new one
                 */
                if (ni_temp->auth_params.wbuf) {
                    wbuf_free(ni_temp->auth_params.wbuf);
                    ni_temp->auth_params.wbuf = NULL;
                }

                IEEE80211_DPRINTF(vap, IEEE80211_MSG_PEER_DELETE,
                                   "defer AUTH for: %s\n",
                                    ether_sprintf(ni_temp->ni_macaddr));
                /* copy AUTH params */
                ni_temp->auth_params.algo = auth_params.algo;
                ni_temp->auth_params.seq = auth_params.seq;
                ni_temp->auth_params.status_code = auth_params.status_code;
                ni_temp->auth_params.challenge = auth_params.challenge;
                ni_temp->auth_params.challenge_length =
                                               auth_params.challenge_length;
                ni_temp->auth_params.vdev_id = auth_params.vdev_id;
                ni_temp->auth_params.wbuf  =  cwbuf;
                qdf_mem_copy(&ni_temp->auth_params.rs,
                              &auth_params.rs,
                              sizeof(struct ieee80211_rx_status));
                ni_temp->auth_inprogress = 1;
                IEEE80211_NODE_STATE_UNLOCK(ni_temp);
                /* mlme_recv_auth_ap_defer will be called
                 * from mlme_auth_peer_delete_handler
                 */
                ieee80211_free_node(ni_temp, WLAN_MLME_SB_ID);
                ret = -1;
            }
        } else {
#ifdef AST_HKV1_WORKAROUND
            ret = mlme_find_and_delete_wds_before_auth(vap, wh->i_addr2, &auth_params);
            if (ret != 0)
#endif
	    ret = mlme_recv_auth_ap_defer(ni, create_new_node, &auth_params, 0);
        }
    } else {
        ret = mlme_recv_auth_ap_defer(ni, false, &auth_params, 0);
    }

    return ret;
}

#ifdef AST_HKV1_WORKAROUND
int mlme_auth_wds_delete_resp_handler(struct wlan_objmgr_psoc *psoc,
                                      struct recv_auth_params_defer *auth_params)
{
    struct ieee80211_node *tmpni = NULL;
    struct wlan_objmgr_vdev *frm_recv_vdev = NULL;
    struct ieee80211vap *frm_recv_vap = NULL;
    qdf_nbuf_t wbuf = NULL;

    if (auth_params) {
        wbuf = auth_params->wbuf;

        frm_recv_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
                auth_params->vdev_id, WLAN_MLME_NB_ID);
        if (!frm_recv_vdev)
            goto exit;
        frm_recv_vap = wlan_vdev_get_mlme_ext_obj(frm_recv_vdev);
        if (!frm_recv_vap)
            goto exit;

        tmpni = ieee80211_ref_node(frm_recv_vap->iv_bss, WLAN_MLME_HANDLER_ID);
        if (!tmpni)
            goto exit;
        mlme_recv_auth_ap_defer(tmpni, 1, auth_params, 1);
        /* Free reference taken during find_node above */
        ieee80211_free_node(tmpni, WLAN_MLME_HANDLER_ID);
        /* Free reference taken during find_vdev above */
        wlan_objmgr_vdev_release_ref(frm_recv_vdev, WLAN_MLME_NB_ID);
        qdf_mem_free(auth_params);
    }
    return 0;

exit:
    /* Couldn't process this request. Drop frame */
    if (frm_recv_vdev)
        wlan_objmgr_vdev_release_ref(frm_recv_vdev, WLAN_MLME_NB_ID);
    if (wbuf)
        wbuf_free(wbuf);
    if (auth_params)
        qdf_mem_free(auth_params);

    return -1;
}
#endif

int mlme_auth_peer_delete_handler(struct ieee80211vap *vap, struct ieee80211_node *ni)
{
  /* copy back AUTH params
   * Remove refrence for that node ideally it should be freed
   * call mlme_recv_auth_ap_defer with cached params
   */
    struct recv_auth_params_defer auth_params;
    struct ieee80211_node *tmpni = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *frm_recv_vdev = NULL;
    struct ieee80211vap *frm_recv_vap = NULL;
    qdf_nbuf_t wbuf = NULL;

    if (ni->auth_inprogress) {
        IEEE80211_NODE_STATE_LOCK(ni);
        auth_params.algo = ni->auth_params.algo;
        auth_params.seq = ni->auth_params.seq;
        auth_params.status_code = ni->auth_params.status_code;
#ifdef WLAN_FEATURE_11BE_MLO
        if (qdf_is_macaddr_zero(&ni->auth_params.mldaddr)) {
            qdf_zero_macaddr(&auth_params.mldaddr);
        }
        else {
	    qdf_copy_macaddr(&auth_params.mldaddr, &ni->auth_params.mldaddr);
        }
#endif
        auth_params.challenge = ni->auth_params.challenge;
        auth_params.challenge_length = ni->auth_params.challenge_length;
        auth_params.vdev_id = ni->auth_params.vdev_id;
        auth_params.wbuf = ni->auth_params.wbuf;
        ni->auth_params.wbuf = NULL;
        IEEE80211_NODE_STATE_UNLOCK(ni);
        wbuf = auth_params.wbuf;
        qdf_mem_copy(&auth_params.rs, &ni->auth_params.rs, sizeof(struct ieee80211_rx_status));
        /* vap in argument is the VAP on which peer delete happend. But peer can send
         * AUTH on a different vap. Find vap from stored vdev ID.
         */
        psoc = wlan_vdev_get_psoc(vap->vdev_obj);
        if (!psoc)
            goto exit;
        frm_recv_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
                auth_params.vdev_id, WLAN_MLME_NB_ID);
        if (!frm_recv_vdev)
            goto exit;
        frm_recv_vap = wlan_vdev_get_mlme_ext_obj(frm_recv_vdev);
        if (!frm_recv_vap)
            goto exit;

        tmpni = ieee80211_ref_node(frm_recv_vap->iv_bss, WLAN_MLME_HANDLER_ID);
        if (!tmpni)
            goto exit;
        mlme_recv_auth_ap_defer(tmpni, 1, &auth_params, 1);
        /* Free reference taken during find_node above */
        ieee80211_free_node(tmpni, WLAN_MLME_HANDLER_ID);
        /* Free reference taken during find_vdev above */
        wlan_objmgr_vdev_release_ref(frm_recv_vdev, WLAN_MLME_NB_ID);
    }
    return 0;

exit:
    /* Couldn't process this request. Drop frame */
    if (frm_recv_vdev)
        wlan_objmgr_vdev_release_ref(frm_recv_vdev, WLAN_MLME_NB_ID);
    if (wbuf)
        wbuf_free(wbuf);
    return -1;
}

static void ieee80211_notify_deferred_auth(struct ieee80211vap *vap,
                                           struct ieee80211_node *ni,
                                           wbuf_t wbuf,
                                           struct ieee80211_rx_status *rs)
{
    struct ieee80211_frame *wh;
    int subtype, ret;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

    if (ieee80211_vap_registered_is_set(vap) &&
        vap->iv_evtable && vap->iv_evtable->wlan_receive_filter_80211) {
        ret = vap->iv_evtable->wlan_receive_filter_80211(vap->iv_ifp, wbuf,
                                                         IEEE80211_FC0_TYPE_MGT,
                                                         subtype, rs);
        /* If indication to OS fails, remove the node */
        if (ret) {
            if(ni && (ni != vap->iv_bss)) {
#ifdef WLAN_FEATURE_11BE_MLO
                if (wlan_peer_is_mlo(ni->peer_obj)) {
                    wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                }
#endif
                IEEE80211_NODE_LEAVE(ni);
            }
        }
    }
}

bool wlan_rx_auth_sanity_check(struct ieee80211vap *vap, u_int16_t algo, u_int16_t seq)
{
    int32_t authmode;

    if ((algo != IEEE80211_AUTH_ALG_OPEN) &&
        (algo != IEEE80211_AUTH_ALG_SHARED) &&
        (algo != IEEE80211_AUTH_ALG_FT) &&
        (algo != IEEE80211_AUTH_ALG_FILS_SK) &&
        (algo != IEEE80211_AUTH_ALG_FILS_SK_PFS) &&
        (algo != IEEE80211_AUTH_ALG_FILS_PK) &&
        (algo != IEEE80211_AUTH_ALG_PASN) &&
        (algo != IEEE80211_AUTH_ALG_SAE) &&
        (algo != IEEE80211_AUTH_ALG_LEAP)) {
        return FALSE;
    }

    authmode = wlan_crypto_get_param(vap->vdev_obj,
                             WLAN_CRYPTO_PARAM_AUTH_MODE);
    if (authmode == -1) {
        return FALSE;
    }

    /* Validate algo */
    if (algo == IEEE80211_AUTH_ALG_SHARED &&
        !(authmode & (1 << WLAN_CRYPTO_AUTH_SHARED))) {
        return FALSE;
    }

    if (algo == IEEE80211_AUTH_ALG_OPEN &&
        (authmode & (1 << WLAN_CRYPTO_AUTH_SHARED)) &&
        !(authmode & (1 << WLAN_CRYPTO_AUTH_OPEN))) {
        return FALSE;
    }

    if (algo == IEEE80211_AUTH_ALG_OPEN) {
        if (seq != IEEE80211_AUTH_OPEN_REQUEST) {
            return FALSE;
        }
    }

    return TRUE;
}

int mlme_recv_auth_ap_defer(struct ieee80211_node *ni, bool create_new_node,
        struct recv_auth_params_defer *auth_params, int peer_delete_inprogress)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211_frame        *wh;
    u_int16_t                     frame_seq = 0;
    u_int16_t                     indication_status = IEEE80211_STATUS_SUCCESS,response_status = IEEE80211_STATUS_SUCCESS ;
    bool                          send_auth_response=true,indicate=true;
    u_int16_t algo, seq, status_code;
    u_int8_t *challenge;
    u_int8_t challenge_length;
    wbuf_t wbuf;
    struct ieee80211_rx_status *rs = &auth_params->rs;
    struct ieee80211_mlme_priv  *mlme_priv = vap->iv_mlme_priv;
    int32_t authmode;
#if QCA_SUPPORT_SON
    struct wlan_auth_reject_ind auth_reject_event_data = {0};
    u_int8_t bs_rej_reason;
    bool bs_blocked;
    bool bs_rejected;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_lmac_if_rx_ops *rx_ops = NULL;
#endif
    uint16_t vap_assoc_limit;
    uint8_t *mldmac = NULL;

#if QCA_SUPPORT_SON
    psoc = wlan_vdev_get_psoc(vap->vdev_obj);
    if (psoc) {
        rx_ops = wlan_psoc_get_lmac_if_rxops(psoc);
    }
#endif
    /*
     * iv_max_aid represents the size of the AID bitmap. For MBSSID/EMA this
     * does not always imply the actual number of clients allowed due to
     * reserved AIDs. Additionally, for MBSSID/EMA, use per-VAP AID limit
     * instead of the global AID limit.
     */
    vap_assoc_limit =  ieee80211_get_vap_assoc_limit(vap->iv_ic, vap);

    algo = auth_params->algo;
    seq = auth_params->seq;
    status_code = auth_params->status_code;
#ifdef WLAN_FEATURE_11BE_MLO
    if (!qdf_is_macaddr_zero(&auth_params->mldaddr)) {
        mldmac = &auth_params->mldaddr.bytes[0];
    }
#endif
    challenge = auth_params->challenge;
    challenge_length = auth_params->challenge_length;
    wbuf = auth_params->wbuf;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);

    /* Max auth fail received check auth sanity, drop instantly if fails */
    if (vap->cont_auth_fail >= vap->max_cont_auth_fail) {
        if (!wlan_rx_auth_sanity_check(vap, algo, seq)) {
            if (peer_delete_inprogress) {
                wbuf_free(auth_params->wbuf);
            }
            return -1;
        }
    }

    /* AP must be up and running */
    if (!mlme_priv->im_connection_up || 
        (wlan_vdev_is_up(vap->vdev_obj) != QDF_STATUS_SUCCESS)) {
        if (peer_delete_inprogress) {
            wbuf_free(auth_params->wbuf);
        }
        qdf_print("%s: vap%d is going down, Drop auth request from %s",
                __func__, vap->iv_unit, ether_sprintf(wh->i_addr2));
        return -1;
    }

#if QCA_SUPPORT_SON
    bs_rej_reason = 0;
    bs_blocked = ieee80211_acl_is_auth_blocked(vap,
                                               wh->i_addr2,
                                               rs->rs_snr,
                                               &bs_rej_reason);
    bs_rejected = (bs_blocked && (bs_rej_reason > 0));

    if (bs_blocked && !bs_rejected) {
        /*
         * This condition will never be true when using Wi-Fi SON applications
         */
        /* Ignore auth frame */
        QDF_TRACE(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
                  "[%s] auth: ignored by band steering\n",
                  ether_sprintf(wh->i_addr2));

        qdf_mem_copy(auth_reject_event_data.client_addr, wh->i_addr2, QDF_MAC_ADDR_SIZE);
        auth_reject_event_data.rssi = rs->rs_rssi;
        auth_reject_event_data.reason = 0;
        auth_reject_event_data.bs_blocked = bs_blocked;
        auth_reject_event_data.bs_rejected = bs_rejected;
        if (rx_ops && rx_ops->son_rx_ops.deliver_event)
            rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, ni->peer_obj, MLME_EVENT_TX_AUTH_FAIL, &auth_reject_event_data);
        return -1;
    }
#endif /* QCA_SUPPORT_SON */


    do {
        if (create_new_node) {
            /* create a node for the station */

            ni = ieee80211_dup_bss(vap, wh->i_addr2);
            if (ni == NULL) {
                indication_status = IEEE80211_STATUS_OTHER;
                /* free cloned wbuffer */
                if (peer_delete_inprogress) {
                    wbuf_free(auth_params->wbuf);
                }
                return -1;
            }

            ni->auth_inprogress = 0;
            wlan_node_set_peer_state(ni, WLAN_AUTH_STATE);

            /* update the last auth frame sequence number */
            ni->ni_last_rxauth_seq = frame_seq;
            ni->ni_last_auth_rx_time = OS_GET_TIMESTAMP();

            /* override bss authmode for shared auth request algorithm*/
            if (algo  == IEEE80211_AUTH_ALG_SHARED)
                ni->ni_authmode = IEEE80211_AUTH_SHARED;
        } else {
            if (!ieee80211_try_ref_node(ni, WLAN_MLME_HANDLER_ID)) {
                indication_status = IEEE80211_STATUS_OTHER;
                /* free cloned wbuffer */
                if (peer_delete_inprogress) {
                    wbuf_free(auth_params->wbuf);
                }
                return -1;
            }
        }

        authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
        if ( authmode == -1 ) {
            qdf_err("crypto_err while getting authmode params\n");
            return -1;
        }

        /* Validate algo */
        if (algo == IEEE80211_AUTH_ALG_SHARED && !(authmode & (1 << WLAN_CRYPTO_AUTH_SHARED))) {
            response_status = IEEE80211_STATUS_ALG;
            indication_status = IEEE80211_STATUS_ALG;
            break;
        }

        if (algo == IEEE80211_AUTH_ALG_OPEN && (authmode & (1 << WLAN_CRYPTO_AUTH_SHARED)) &&
            !(authmode & (1 << WLAN_CRYPTO_AUTH_OPEN))) {
            response_status = IEEE80211_STATUS_ALG;
            indication_status = IEEE80211_STATUS_ALG;
            break;
        }

        /*
         * Consult the ACL policy module if setup.
         */
        if (!ieee80211_acl_check(vap, wh->i_addr2, mldmac)) {
#if QCA_SUPPORT_SON
        if(!(ieee80211_acl_flag_check(vap, wh->i_addr2, IEEE80211_ACL_FLAG_AUTH_ALLOW))) {
#endif /* QCA_SUPPORT_SON */
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL,
                      "[%s] auth: disallowed by ACL \n",ether_sprintf(wh->i_addr2));
            response_status = IEEE80211_STATUS_REFUSED;
            indication_status = IEEE80211_STATUS_REFUSED;
            IEEE80211_DELIVER_EVENT_BLKLST_STA_AUTH_INDICATION(vap, wh->i_addr2, indication_status);
#ifdef QCA_SUPPORT_CP_STATS
            vdev_cp_stats_rx_acl_inc(vap->vdev_obj, 1);
#endif
            break;
#if QCA_SUPPORT_SON
        } else {
        qdf_mem_copy(auth_reject_event_data.client_addr, wh->i_addr2, QDF_MAC_ADDR_SIZE);
        auth_reject_event_data.rssi = rs->rs_rssi;
        if (rx_ops && rx_ops->son_rx_ops.deliver_event)
            rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, ni->peer_obj, MLME_EVENT_TX_AUTH_ALLOW, &auth_reject_event_data);
        }
#endif /* QCA_SUPPORT_SON */
        }

#if QCA_SUPPORT_SON
        if (bs_rejected) {
            QDF_TRACE(QDF_MODULE_ID_MLME, QDF_TRACE_LEVEL_DEBUG,
                      "[%s] auth: rejected by band steering, reason=%u\n",
                      ether_sprintf(wh->i_addr2), bs_rej_reason);
            response_status = bs_rej_reason;
            indication_status = IEEE80211_STATUS_REFUSED;
            break;
        }
#endif /* QCA_SUPPORT_SON */
        if (IEEE80211_VAP_IS_COUNTERM_ENABLED(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH | IEEE80211_MSG_CRYPTO,
              "[%s] auth: TKIP countermeasures enabled \n",ether_sprintf(wh->i_addr2));
#ifdef QCA_SUPPORT_CP_STATS
            vdev_cp_stats_rx_auth_countermeasures_inc(vap->vdev_obj, 1);
#endif
	    response_status = IEEE80211_REASON_MIC_FAILURE;
	    indication_status = IEEE80211_STATUS_REFUSED;
	    break;
        }
        /*
         * reject auth if there are too many STAs already associated.
         */
        if (vap->iv_sta_assoc >= vap_assoc_limit) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH,
                    "[%s] num auth'd STAs is %d, max is %d, rejecting "
                    "new auth\n", ether_sprintf(wh->i_addr2),
                    vap->iv_sta_assoc, vap->iv_max_aid);

            response_status = IEEE80211_STATUS_TOOMANY;
            indication_status = IEEE80211_STATUS_TOOMANY;
            break;
        }
        if (algo == IEEE80211_AUTH_ALG_OPEN) {
            if (seq != IEEE80211_AUTH_OPEN_REQUEST) {
                response_status = IEEE80211_STATUS_SEQUENCE;
                indication_status = IEEE80211_STATUS_SEQUENCE;
                break;
            }
        } else if (algo == IEEE80211_AUTH_ALG_SHARED) {
#if UMAC_SUPPORT_CFG80211
          if ( !vap->iv_cfg80211_create ) {
#endif
                response_status = indication_status = mlme_auth_shared(ni,seq,status_code,challenge,challenge_length);
                send_auth_response=false;
                if (seq == IEEE80211_AUTH_SHARED_REQUEST && response_status == IEEE80211_STATUS_SUCCESS)
                    indicate=false;
#if UMAC_SUPPORT_CFG80211
            }
#endif
            break;
        } else if(algo == IEEE80211_AUTH_ALG_FT) {
            ni->is_ft_reauth = 1;
            /* TODO: decide what all need to be done for FT frame */
            /*response_status =;
              indication_status = ;
              send_auth_response=;
              indicate=;
              vap->iv_stats.xxxx++*/
            break;
        }
#if WLAN_SUPPORT_FILS
        else if(algo == IEEE80211_AUTH_ALG_FILS_SK ||
                algo == IEEE80211_AUTH_ALG_FILS_SK_PFS ||
                algo == IEEE80211_AUTH_ALG_FILS_PK) {
            /* Any specific handling required for FILS Auth */
            break;
        }
#endif
        else if(algo == IEEE80211_AUTH_ALG_SAE){
                break;
        } else if (algo == IEEE80211_AUTH_ALG_PASN) {
            break;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH | IEEE80211_MSG_CRYPTO,
                    "[%s] auth: unsupported algorithm %d \n",ether_sprintf(wh->i_addr2),algo);
#ifdef QCA_SUPPORT_CP_STATS
            vdev_cp_stats_rx_auth_unsupported_inc(vap->vdev_obj, 1);
#endif
            response_status = IEEE80211_STATUS_ALG;
            indication_status = IEEE80211_STATUS_ALG;
            break;
        }
    } while (FALSE);

    if (indicate ) {
        IEEE80211_DELIVER_EVENT_MLME_AUTH_INDICATION(vap, ni->ni_macaddr,
                indication_status);
    }

#if QCA_SUPPORT_SON
    if (response_status != IEEE80211_STATUS_SUCCESS) {
        qdf_mem_copy(auth_reject_event_data.client_addr, wh->i_addr2, QDF_MAC_ADDR_SIZE);
        auth_reject_event_data.rssi = rs->rs_rssi;
        auth_reject_event_data.reason = response_status;
        auth_reject_event_data.bs_blocked = bs_blocked;
        auth_reject_event_data.bs_rejected = bs_rejected;
        if (rx_ops && rx_ops->son_rx_ops.deliver_event)
            rx_ops->son_rx_ops.deliver_event(vap->vdev_obj, ni->peer_obj, MLME_EVENT_TX_AUTH_FAIL, &auth_reject_event_data);
    }
#else
    // To silence compiler warning about unused variable.
    (void) rs;
#endif

	if (ieee80211_vap_oce_check(vap)) {
		ni->ni_abs_rssi = rs->rs_abs_rssi;
	}

    ni->ni_authalg = algo;
    ni->ni_authstatus = response_status;
#if UMAC_SUPPORT_CFG80211
    if (ieee80211_vap_trigger_mlme_resp_is_set(vap) || vap->iv_cfg80211_create ) {
#else
    if (ieee80211_vap_trigger_mlme_resp_is_set(vap)) {
#endif
        /* Wait for application to trigger mlme response for auth */
        /*
         * In case of WEXT hostapd calls wlan_mlme_auth and driver handles failures
         * like ACL check, MIC & IEEE80211_STATUS_TOOMANY failures and node will be
         * deauthed.
         * In case of cfg80211 hostpad does not call back driver to handle these
         * failures, so send auth with erro status incase of failures in driver itself.
         *
         */
#if UMAC_SUPPORT_CFG80211
        if ((vap->iv_cfg80211_create) && response_status != IEEE80211_STATUS_SUCCESS)
#else
        if (response_status != IEEE80211_STATUS_SUCCESS)
#endif
        {
            int32_t key_mgmt;

            key_mgmt = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_KEY_MGMT);
            if ( key_mgmt == -1 ) {
                qdf_err("crypto_err while getting key_mgmt params\n");
                return -1;
            }


            if (key_mgmt & (1 << WLAN_CRYPTO_KEY_MGMT_SAE))
                ieee80211_send_auth(ni, seq , response_status, NULL, 0, NULL, NULL);
            else
                ieee80211_send_auth(ni, seq + 1, response_status, NULL, 0, NULL, NULL);
            /* auth is not success, remove the node from node table */
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s auth failed with status %d \n", __func__, ni->ni_authstatus);
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj))
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
#endif
            IEEE80211_NODE_LEAVE(ni);
        }
    } else {

        IEEE80211_DELIVER_EVENT_MLME_AUTH_COMPLETE(vap, ni->ni_macaddr, response_status);

        if (send_auth_response) {
            ieee80211_send_auth(ni, seq + 1, response_status, NULL, 0, NULL, NULL);
        }

        if (indication_status != IEEE80211_STATUS_SUCCESS ){
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj))
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
#endif
            /* auth is not success, remove the node from node table*/
            IEEE80211_NODE_LEAVE(ni);
        }
    }

    if ((response_status == IEEE80211_STATUS_REFUSED) && (vap->iv_assoc_denial_notify)){
        IEEE80211_DELIVER_ASSOC_DENIAL_EVENT(vap, ni->ni_macaddr);
    }


    if (peer_delete_inprogress) {
        /* Indidate this frame to OS/upper layer */
        ieee80211_notify_deferred_auth(vap, ni, wbuf, rs);
        /* free cloned wbuffer */
        wbuf_free(auth_params->wbuf);
    }

    /*
     * release the reference created at the begining of the case above
     * either by alloc_node or ref_node.
     */
    if(create_new_node)
        ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
    else
        ieee80211_free_node(ni, WLAN_MLME_HANDLER_ID);

    if (response_status != IEEE80211_STATUS_SUCCESS) {
        vap->cont_auth_fail++;
        return -1;
    } else {
        /* Reset continuous auth_fail counter */
        vap->cont_auth_fail = 0;
        return 0;
    }
}

void
ieee80211_mlme_node_leave_ap(struct ieee80211_node *ni)
{
    struct ieee80211vap         *vap = ni->ni_vap;
    struct ieee80211_mlme_priv  *mlme_priv;
    ieee80211_mlme_event          event;

    ASSERT(vap != NULL);
    ASSERT(vap->iv_opmode != IEEE80211_M_STA);

    mlme_priv = vap->iv_mlme_priv;
    event.u.event_sta.sta_count= vap->iv_sta_assoc;
    event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
    event.u.event_sta.ni = ni;

    event.type = IEEE80211_MLME_EVENT_STA_LEAVE;
    ieee80211_mlme_deliver_event(mlme_priv,&event);

    /* NB: preserve ni_table */
    if (ieee80211node_has_flag(ni, IEEE80211_NODE_PWR_MGT)) {

        vap->iv_ps_sta--;
        ieee80211node_clear_flag(ni, IEEE80211_NODE_PWR_MGT);

        event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
        event.type = IEEE80211_MLME_EVENT_STA_EXIT_PS;
        ieee80211_mlme_deliver_event(mlme_priv,&event);
    }


}

void
ieee80211_mlme_node_pwrsave_ap(struct ieee80211_node *ni, int enable)
{
    struct ieee80211vap *vap = ni->ni_vap;

    struct ieee80211com *ic = ni->ni_ic;

    ieee80211_mlme_event          event;

    if  ( ((ni->ni_flags & IEEE80211_NODE_PWR_MGT) != 0) ^ enable) {
        struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

        if (enable) {
            vap->iv_ps_sta++;
            ieee80211node_set_flag(ni, IEEE80211_NODE_PWR_MGT);
#ifdef ATH_SWRETRY
            if (ic->ic_node_psupdate) {
                ic->ic_node_psupdate(ni, 1, 1);
                IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni, "%s", "pause LMAC node\n");
            }
#endif
            IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni,
                           "power save mode on, %u sta's in ps mode\n", vap->iv_ps_sta);
            ieee80211node_pause(ni);
            event.type = IEEE80211_MLME_EVENT_STA_ENTER_PS;
        } else {

            vap->iv_ps_sta--;
            ieee80211node_clear_flag(ni, IEEE80211_NODE_PWR_MGT);
#ifdef ATH_SWRETRY
            if (ic->ic_node_psupdate) {
                ic->ic_node_psupdate(ni, 0, 1);
                IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni, "%s", "unpause LMAC node\n");
            }
#endif
            ieee80211node_unpause(ni);
            IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni,
                           "power save mode off, %u sta's in ps mode\n", vap->iv_ps_sta);
            event.type = IEEE80211_MLME_EVENT_STA_EXIT_PS;

			/*
			 * Enable aggregation back after the client exit from power-save
			 */
			if(ni && ni->ni_pspoll) {
#if ATH_POWERSAVE_WAR
#ifdef ATH_SUPPORT_QUICK_KICKOUT
			    systime_t current_time = OS_GET_TIMESTAMP();
			    if (CONVERT_SYSTEM_TIME_TO_MS(current_time - ni->ni_pspoll_time) > (IEEE80211_PSPOLL_KICKOUT_THR)) {
				ni->ni_kickout = true;
			    }
#endif
			    /* Revisit later, this reset of ni_pspoll_time to be moved to inactivity phase */
			    ni->ni_pspoll_time = 0;
#endif
                            ic->ic_node_pspoll(ni, 0);
			}


        }

        event.u.event_sta.sta_count= vap->iv_sta_assoc;
        event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
        event.u.event_sta.ni = ni;
        ieee80211_mlme_deliver_event(mlme_priv,&event);
    }
}

#ifdef WLAN_FEATURE_11BE
uint8_t *ieee80211_add_default_t2lm_ie(struct ieee80211_node *ni,
        uint8_t *frm)
{
    struct wlan_t2lm_onging_negotiation_info t2lm = {0};

    t2lm.t2lm_info[WLAN_T2LM_DL_DIRECTION].direction =
        WLAN_T2LM_INVALID_DIRECTION;
    t2lm.t2lm_info[WLAN_T2LM_UL_DIRECTION].direction =
        WLAN_T2LM_INVALID_DIRECTION;

    t2lm.t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction =
        WLAN_T2LM_BIDI_DIRECTION;
    t2lm.t2lm_info[WLAN_T2LM_BIDI_DIRECTION].default_link_mapping = true;

    frm = ieee80211_add_t2lm_ie(frm, ni, &t2lm);
    if (!frm) {
        t2lm_err("Failed to add T2LM IE");
        return NULL;
    }

    wlan_copy_configured_t2lm_to_ml_peer_ctx(ni->peer_obj->mlo_peer_ctx,
            &t2lm, 0);

    return frm;
}

void ieee80211_copy_received_assoc_t2lm_ie_to_ml_peer_ctx(
        struct wlan_mlo_peer_context *ml_peer_ctx)
{
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct ieee80211_node *temp_ni = NULL;
    QDF_STATUS status;
    uint8_t i = 0;

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &ml_peer_ctx->peer_list[i];

        if (!peer_entry->link_peer) {
            continue;
        }

        status = wlan_objmgr_peer_try_get_ref(peer_entry->link_peer,
                WLAN_MLME_SB_ID);
        if (QDF_IS_STATUS_ERROR(status)) {
            t2lm_err("failed to get peer refcount");
            continue;
        }

        temp_ni = wlan_peer_get_mlme_ext_obj(peer_entry->link_peer);
        if (!temp_ni) {
            wlan_objmgr_peer_release_ref(peer_entry->link_peer,
                    WLAN_MLME_SB_ID);
            continue;
        }

        if (temp_ni->ni_ml_rptingrole !=
                IEEE80211_MLO_RPTINGROLE_REPORTINGSTA) {
            wlan_objmgr_peer_release_ref(peer_entry->link_peer,
                    WLAN_MLME_SB_ID);
            continue;
        }

        wlan_copy_configured_t2lm_to_ml_peer_ctx(ml_peer_ctx,
                &temp_ni->asreq_t2lm_negotiation, 0);
        wlan_objmgr_peer_release_ref(peer_entry->link_peer,
                WLAN_MLME_SB_ID);
        break;
    }
}

void ieee80211_asresp_t2lm_ie(struct ieee80211_node *ni,
        uint8_t *frm, struct wlan_mlo_peer_context *ml_peer_ctx,
        uint8_t *t2lm_ie_len)
{
    struct wlan_objmgr_peer *peer = NULL;
    struct ieee80211vap *vap = NULL;
    uint8_t *efrm = frm;

    *t2lm_ie_len = 0;

    /* If RM/T2LM APP is present, then add the T2LM IE based on the RM/T2LM
     * response and skip the below code.
     */

    if (ni->ni_use_established_t2lm) {
        frm = ieee80211_add_t2lm_ie(frm, ni, &ni->asreq_t2lm_negotiation);
        if (!frm) {
            t2lm_err("Failed to add T2LM IE");
        } else {
            *t2lm_ie_len = frm-efrm;
            wlan_copy_configured_t2lm_to_ml_peer_ctx(ml_peer_ctx,
                    &ni->asreq_t2lm_negotiation, 0);
        }
        return;
    }

    vap = ieee80211_get_t2lm_test_cmd_vap(ni, ml_peer_ctx, &peer);

    /* Add the default mapping in T2LM IE if T2LM test command is not issued and
     * use default T2LM is set.
     */
    if (!(vap && vap->t2lm_test_cmd) && ni->ni_use_default_t2lm_map) {
        frm = ieee80211_add_default_t2lm_ie(ni, frm);
        if (peer) {
            wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
        }
        if (frm)
            *t2lm_ie_len = frm-efrm;
        return;
    }

    /* T2LM test command issued case */
    if (vap && vap->t2lm_test_cmd &&
        !qdf_mem_cmp(vap->t2lm_peer_mld_mac, ml_peer_ctx->peer_mld_addr.bytes,
            QDF_MAC_ADDR_SIZE)) {
        /* If no T2LM IE added in the Assoc response indicates that AP
         * MLD accepted the T2LM mapping provided by the peer MLD. Else,
         * the preffered mapping will be added to the Assoc response
         * frame.
         */
        if (vap->t2lm_negotiation.category == WLAN_T2LM_CATEGORY_RESPONSE) {
            if (vap->t2lm_negotiation.t2lm_resp_type ==
                    WLAN_T2LM_RESP_TYPE_SUCCESS) {
                if (ni->ni_use_default_t2lm_map) {
                    /* Heterogeneous T2LM received from STA */
                    frm = ieee80211_add_default_t2lm_ie(ni, frm);
                    if (peer) {
                        wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
                    }
                    if (frm)
                        *t2lm_ie_len = frm-efrm;
                    return;
                } else {
                    /* Do not include the T2LM IE in the Assoc response
                     * frame if the received T2LM is accepted by the AP MLD.
                     */
                    ieee80211_copy_received_assoc_t2lm_ie_to_ml_peer_ctx(
                            ml_peer_ctx);
                    if (peer) {
                        wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
                    }
                    return;
                }
            } else {
                /* Add the T2LM mapping provided by the T2LM test command */
                if (vap->t2lm_negotiation.t2lm_resp_type ==
                        WLAN_T2LM_RESP_TYPE_DENIED_TID_TO_LINK_MAPPING) {
                    vap->t2lm_negotiation.t2lm_info[WLAN_T2LM_BIDI_DIRECTION].direction =
                        WLAN_T2LM_BIDI_DIRECTION;
                    vap->t2lm_negotiation.t2lm_info[WLAN_T2LM_BIDI_DIRECTION].default_link_mapping =
                        true;
                }
                frm = ieee80211_add_t2lm_ie(frm, ni, &vap->t2lm_negotiation);
                if (!frm) {
                    t2lm_err("Failed to add T2LM IE");
                    if (peer) {
                        wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
                    }
                    return;
                }

                *t2lm_ie_len = frm-efrm;
                wlan_copy_configured_t2lm_to_ml_peer_ctx(ml_peer_ctx,
                        &vap->t2lm_negotiation, 0);
            }
        }
    } else {
        /* Unit test command is not issued. Hence, copy the received T2LM from
         * assoc request to MLO peer context.
         */
        ieee80211_copy_received_assoc_t2lm_ie_to_ml_peer_ctx(ml_peer_ctx);
    }

    if (peer) {
        wlan_objmgr_peer_release_ref(peer, WLAN_MLME_SB_ID);
    }

    return;
}

void wlan_initialize_default_t2lm_values(struct wlan_mlo_peer_context *ml_peer)
{
    struct wlan_mlo_peer_t2lm_policy *t2lm_policy = NULL;
    struct wlan_t2lm_info *negotiated_t2lm = NULL;
    struct wlan_t2lm_info *ongoing_t2lm = NULL;
    uint8_t dir = 0;
    struct ieee80211_node *ni = NULL;

    ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
    if (!ni)
        return;

    /* Retuen from here if Assoc response T2LM IE is already prepared by the
     * partner vdevs.
     */
    if (ni->ni_asresp_t2lmie_len || ni->ni_asresp_t2lmie) {
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return;
    }

    ml_peer->t2lm_policy.t2lm_enable_val =
        ni->ni_t2lm_enable_val;

    t2lm_policy = &ml_peer->t2lm_policy;
    /* Initialize the direction to invalid */
    for(dir = 0; dir < WLAN_T2LM_MAX_DIRECTION; dir++) {
        t2lm_policy->t2lm_negotiated_info.t2lm_info[dir].direction =
            WLAN_T2LM_INVALID_DIRECTION;
        t2lm_policy->ongoing_tid_to_link_mapping.t2lm_info[dir].direction =
            WLAN_T2LM_INVALID_DIRECTION;
    }

    /*Set the default TID-to-link mapping for BIDI */
    negotiated_t2lm =
        &t2lm_policy->t2lm_negotiated_info.t2lm_info[WLAN_T2LM_BIDI_DIRECTION];
    ongoing_t2lm =
        &t2lm_policy->ongoing_tid_to_link_mapping.t2lm_info[WLAN_T2LM_BIDI_DIRECTION];

    negotiated_t2lm->direction = WLAN_T2LM_BIDI_DIRECTION;
    negotiated_t2lm->default_link_mapping = true;
    ongoing_t2lm->direction = WLAN_T2LM_BIDI_DIRECTION;
    ongoing_t2lm->default_link_mapping = true;

    ni->ni_asresp_t2lmie =
        qdf_mem_malloc(sizeof(struct wlan_t2lm_onging_negotiation_info));
    if (!ni->ni_asresp_t2lmie) {
        t2lm_err("Failed to allocate ni_asresp_t2lmie");
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return;
    }

    ieee80211_asresp_t2lm_ie(ni, ni->ni_asresp_t2lmie, ml_peer,
            &ni->ni_asresp_t2lmie_len);

    if (!ni->ni_asresp_t2lmie_len) {
        qdf_mem_free(ni->ni_asresp_t2lmie);
        ni->ni_asresp_t2lmie = NULL;
        wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
        return;
    }

    wlan_objmgr_peer_release_ref(ni->peer_obj, WLAN_MLME_NB_ID);
}
#endif /* WLAN_FEATURE_11BE */

#ifdef WLAN_FEATURE_11BE_MLO
QDF_STATUS wlan_mlme_peer_create(struct wlan_objmgr_vdev *vdev,
                                 struct wlan_mlo_peer_context *ml_peer,
                                 struct qdf_mac_addr *addr,
                                 qdf_nbuf_t frm_buf)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_peer *peer;
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;
    struct ieee80211_frame *wh = NULL;
    uint8_t subtype = 0;
    uint8_t reassoc = 0;
    wbuf_t link_req_wbuf = NULL;
    wbuf_t link_resp_wbuf = NULL;
    struct qdf_mac_addr link_addr;
    qdf_size_t link_req_len = 0;
    bool nawds_peer = false, mesh_peer = false;
    int is_splitmac_enable = 0;
#if WLAN_SUPPORT_SPLITMAC
    uint8_t i;
    uint8_t override_done = 0;
    struct ieee80211_node *assoc_ni = NULL;
#endif
    uint8_t link_id;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " vap is NULL ",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));
        goto peer_create_fail;
    }

    ic = vap->iv_ic;
    if(ic == NULL) {
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " ic is NULL ",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));
        goto peer_create_fail;
    }

    if (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
        wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE)) {
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " link removal is in progress %d, %d",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]),
                wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_STOP_LINK_DEL),
                wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE));
        goto peer_create_fail;
    }

       /* If partner peer is disallowed by ACL, do not create link peer */
    if (!ieee80211_acl_check(vap, &addr->bytes[0], &ml_peer->peer_mld_addr.bytes[0])) {
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " is disallowed by ACL ",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));
        goto peer_create_fail;
    }

    /*
     * Before creating a new peer, check whether peer another already exists
     * with link mac addr that is same as the mld mac addr of current peer.
     */
    ni = ieee80211_find_node(ic, ml_peer->peer_mld_addr.bytes, WLAN_MLO_MGR_ID);
    if (ni) {
        mlo_err("ML Peer with mld " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " exists",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));

         /* release the ref taken by find_node() */
        ieee80211_free_node(ni, WLAN_MLO_MGR_ID);
        ni = NULL;
        goto peer_create_fail;
    }

    ni = ieee80211_find_node(ic, &addr->bytes[0], WLAN_MLO_MGR_ID);
    if (ni) {
        /**
         * Clean up the existing peer if it corresponds to a client node that
         * connected earlier in non-PMF mode.
         * PMF client needs to be cleaned up only with SA query which will be
         * handled in upcoming change.
         */
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " entry exists",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));
        if (!ieee80211_is_pmf_enabled(vap, ni) &&
            (ni != ni->ni_vap->iv_bss) &&
            (ieee80211_vap_get_opmode(ni->ni_vap) != IEEE80211_M_STA)) {
            mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " entry exists, deleting older entry",
                    QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                    QDF_MAC_ADDR_REF(&addr->bytes[0]));

            if (wlan_peer_is_mlo(ni->peer_obj))
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);

            IEEE80211_NODE_LEAVE(ni);
        }

         /* release the ref taken by find_node() */
        ieee80211_free_node(ni, WLAN_MLO_MGR_ID);
        ni = NULL;
        goto peer_create_fail;
    }

    /* Use the tmp node when peer limit is reached and mark the peer as rejected */
    pdev = wlan_vdev_get_pdev(vdev);
    psoc = wlan_pdev_get_psoc(pdev);
    if (vdev->mlo_dev_ctx && vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject &&
        (wlan_psoc_get_peer_count(psoc) >= wlan_psoc_get_max_peer_count(psoc) ||
         wlan_pdev_get_peer_count(pdev) >= wlan_pdev_get_max_peer_count(pdev) ||
         wlan_vdev_get_peer_count(vdev) >= wlan_vdev_get_max_peer_count(vdev))) {
        mlo_warn("Allocate tmp node for " QDF_MAC_ADDR_FMT
                 " [vdev: peers %d max %d] [pdev: peers %d max %d] [psoc: peers %d max %d]",
                 QDF_MAC_ADDR_REF(&addr->bytes[0]),
                 wlan_vdev_get_peer_count(vdev), wlan_vdev_get_max_peer_count(vdev),
                 wlan_pdev_get_peer_count(pdev), wlan_pdev_get_max_peer_count(pdev),
                 wlan_psoc_get_peer_count(psoc), wlan_psoc_get_max_peer_count(psoc));
        ni = ieee80211_tmp_node(vap, &addr->bytes[0]);
        if (ni)
            wlan_peer_set_assoc_rejected(ni->peer_obj);
    } else
        ni = ieee80211_dup_bss(vap, &addr->bytes[0]);
    if (!ni) {
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " peer create failed",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));
        goto peer_create_fail;
    }

    if (ni->ni_ml_rptingrole >= IEEE80211_MLO_RPTINGROLE_INVALID) {
        qdf_rl_err("Invalid MLO reporting role value %d for ni with MAC address %s",
                ni->ni_ml_rptingrole, ether_sprintf(ni->ni_macaddr));
        goto peer_create_fail;
    }

    peer = ni->peer_obj;
    wlan_peer_set_mlo(peer);
#if WLAN_SUPPORT_SPLITMAC
    is_splitmac_enable = splitmac_is_enabled(vdev);
    if (is_splitmac_enable) {
        splitmac_api_set_state(peer, SPLITMAC_ADDCLIENT_START);
    }
#endif
    nawds_peer = wlan_mlo_peer_is_nawds(ml_peer);
    mesh_peer = wlan_mlo_peer_is_mesh(ml_peer);
    if ((ni->ni_ml_rptingrole != IEEE80211_MLO_RPTINGROLE_REPORTINGSTA) &&
        ((!nawds_peer) && (!mesh_peer))) {
        ni->ni_ml_rptingrole = IEEE80211_MLO_RPTINGROLE_REPORTEDSTA;

        wh = (struct ieee80211_frame *) wbuf_header(frm_buf);
        subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

        if ((subtype != IEEE80211_FC0_SUBTYPE_ASSOC_REQ) &&
                (subtype != IEEE80211_FC0_SUBTYPE_REASSOC_REQ)) {
            mloie_err_rl("Unsupported or invalid subtype %u", subtype);
            goto peer_create_fail;
        }

        reassoc = (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ? 1 : 0;

        qdf_mem_copy(link_addr.bytes, vap->iv_bss->ni_bssid,
                sizeof(link_addr.bytes));

        link_req_wbuf =
            wbuf_alloc(ic->ic_osdev, WBUF_TX_MGMT, MAX_TX_RX_PACKET_SIZE);

        if (!link_req_wbuf) {
            mloie_err_rl("Unable to allocate wbuf for link specific assoc request");
            goto peer_create_fail;
        }

        mloie_debug("Generating link specific assoc request for reported STA with link MAC address %s",
                ether_sprintf(addr->bytes));
        link_id = wlan_vdev_get_link_id(vdev);
        if (util_gen_link_assoc_req(
                    wbuf_header(frm_buf) + sizeof(struct ieee80211_frame),
                    wbuf_get_pktlen(frm_buf) - sizeof(struct ieee80211_frame),
                    reassoc, link_id, link_addr,
                    wbuf_header(link_req_wbuf), MAX_TX_RX_PACKET_SIZE,
                    &link_req_len) != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Unable to generate link specific assoc request");
            goto peer_create_fail;
        }

        wbuf_set_pktlen(link_req_wbuf, link_req_len);

        mloie_debug("Processing link specific assoc request for reported STA with link MAC address %s",
                ether_sprintf(addr->bytes));
        if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP) && ieee80211_recv_asreq(ni, link_req_wbuf, subtype, NULL, 0) != 0) {
            /* Error handling for reported STAs will be added in a future
             * change after the peer management and hostapd/supplicant
             * dependencies are met.
             */
            mloie_err_rl("Unable to carry out receive processing for link specific assoc request");
            goto peer_create_fail;
        }

        if (!is_splitmac_enable) {
           if (vdev->mlo_dev_ctx && vdev->mlo_dev_ctx->ap_ctx->mlo_link_reject) {
               /* By default mark all partner link assoc rejected, it will be
                * cleared after hostapd also accepts this partner link */
               wlan_peer_set_assoc_rejected(peer);

               /* Tmp node is used when peer limit is reached */
               if (ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)) {
                   /* Mark EHT node and assoc status */
                   ni->ni_ext_flags |= IEEE80211_NODE_EHT;
                   ni->ni_assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
               }
           }

           mloie_debug("Generating link specific assoc response for reported STA with link MAC address %s status %d",
                   ether_sprintf(addr->bytes), ni->ni_assocstatus);
           link_resp_wbuf = ieee80211_setup_assocresp(ni, NULL, reassoc,
                   ni->ni_assocstatus, NULL, (uint8_t *)ml_peer, NULL, NULL);

           if (link_resp_wbuf == NULL) {
               mloie_err_rl("Unable to generate link specific assoc response");
               goto peer_create_fail;
           }
        }
    } else if (nawds_peer || mesh_peer) {
        if (ni->ni_ml_rptingrole != IEEE80211_MLO_RPTINGROLE_REPORTINGSTA)
            ni->ni_ml_rptingrole = IEEE80211_MLO_RPTINGROLE_REPORTEDSTA;
    }

    if (nawds_peer)
        wlan_mlo_nawds_config(vap, ni, ml_peer);
    if (mesh_peer)
        wlan_mlo_mesh_config(vap, ni, ml_peer);

#if WLAN_SUPPORT_SPLITMAC
    if (is_splitmac_enable) {
        /* Get the Assoc link to send T2LM action frame */
        assoc_ni = ieee80211_ucfg_get_mlo_assoc_link_node(ml_peer);
        if (!assoc_ni) {
            mlo_err("assoc peer is not present, so declaring peer create failure");
            goto peer_create_fail;
        }

        for (i = 0; i < assoc_ni->addclient_ml_info.num_partner_links; i++) {
            if (WLAN_ADDR_EQ(&addr->bytes[0],
                  assoc_ni->addclient_ml_info.partner_caps[i].stamac) ==
                                                 QDF_STATUS_SUCCESS) {
               assoc_ni->addclient_ml_info.partner_caps[i].mlo = 1;
               assoc_ni->addclient_ml_info.partner_caps[i].associd = assoc_ni->ni_associd;;
               wlan_override_client_params(vdev, peer,
                            &assoc_ni->addclient_ml_info.partner_caps[i]);
               override_done = 1;
            }
        }
        if (assoc_ni->peer_obj) {
            wlan_objmgr_peer_release_ref(assoc_ni->peer_obj, WLAN_MLME_NB_ID);
        }
        if (!override_done) {
            mlo_err("params are not overriden for mac"QDF_MAC_ADDR_FMT,
                    QDF_MAC_ADDR_REF(&addr->bytes[0]));
            goto peer_create_fail;
        }

        if (ieee80211_mlme_recv_assoc_request(ni, reassoc, NULL, link_req_wbuf, NULL) == -EBUSY) {
            mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " mlme assoc processing failed",
                    QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                    QDF_MAC_ADDR_REF(&addr->bytes[0]));
            goto peer_create_fail;
        }
    }
#endif

    if (!(ni->ni_ext_flags & IEEE80211_NODE_EHT)) {
        qdf_rl_err("EHT flag is not set for ni with MAC address %s",
                   ether_sprintf(ni->ni_macaddr));
        goto peer_create_fail;
    }

    ni->ni_associd = ml_peer->assoc_id;
    qdf_copy_macaddr((struct qdf_mac_addr *)&peer->mldaddr,
                     &ml_peer->peer_mld_addr);

    ret = wlan_mlo_link_peer_attach(ml_peer, peer, link_resp_wbuf);
    if (QDF_IS_STATUS_ERROR(ret)) {
        mlo_err("ML Peer " QDF_MAC_ADDR_FMT " partner peer " QDF_MAC_ADDR_FMT " link peer attach failed",
                QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes),
                QDF_MAC_ADDR_REF(&addr->bytes[0]));
        goto peer_create_fail;
    }

    ni->ni_inact_reload = ni->ni_vap->iv_inact_auth;
    ni->ni_inact = ni->ni_inact_reload;

    if ((!nawds_peer) && (!mesh_peer)) {
        if (!is_splitmac_enable &&
            wlan_mlo_peer_is_assoc_done(ml_peer) == QDF_STATUS_SUCCESS) {
            int retval = ic->ic_newassoc(ni, 1);
            if (retval)
                goto peer_create_fail;
        }
    }
#if WLAN_SUPPORT_SPLITMAC
    if (is_splitmac_enable) {
        splitmac_api_set_state(peer, SPLITMAC_ADDCLIENT_END);
    }
#endif

    /* free the extra refcount got from dup_bss() */
    if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP))
        ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

    if (link_req_wbuf)
        wbuf_free(link_req_wbuf);

    return QDF_STATUS_SUCCESS;

peer_create_fail:
    wlan_mlo_partner_peer_create_failed_notify(ml_peer);

    if (link_req_wbuf)
        wbuf_free(link_req_wbuf);

    if (ni) {
         /* Freeing the node, as the peer is not used */
        IEEE80211_NODE_LEAVE(ni);

        /* free the extra refcount got from dup_bss() */
        if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP))
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
    }

    return QDF_STATUS_E_FAILURE;
}

/**
 * ieee80211_bridge_peer_set_security: Set RSN caps for bridge peer
                                       (created on AP side) from assoc link
                                       peer.
 * @br_ni: pointer to real link handle
 * @ml_peer: pointer to ml peer context
 *
 * returns: QDF_STATUS_SUCCESS in case of success, QDF_STATUS_E_* otherwise.
 */
static inline QDF_STATUS
ieee80211_bridge_peer_set_security(struct ieee80211_node *br_ni,
                                   struct wlan_mlo_peer_context *ml_peer)
{
    int32_t rsn_caps;
    struct wlan_objmgr_peer *assoc_peer;

    if (!br_ni)
        return QDF_STATUS_E_FAILURE;

    if (br_ni->ni_vap->iv_opmode != IEEE80211_M_HOSTAP)
        return QDF_STATUS_SUCCESS;

    if (!ml_peer)
        return QDF_STATUS_E_NULL_VALUE;

    assoc_peer = wlan_mlo_peer_get_assoc_peer(ml_peer);
    if (!assoc_peer)
        return QDF_STATUS_E_NULL_VALUE;

    rsn_caps = wlan_crypto_get_peer_param(assoc_peer,
                                          WLAN_CRYPTO_PARAM_RSN_CAP);
    if (rsn_caps < 0)
        return QDF_STATUS_E_INVAL;

    return wlan_crypto_set_peer_param(br_ni->peer_obj,
                                      WLAN_CRYPTO_PARAM_RSN_CAP, rsn_caps);
}

void ieee80211_inherit_bss_self_peer(struct ieee80211_node *ni,
                                     struct wlan_mlo_peer_context *ml_peer)
{
    struct ieee80211_node *iv_bss = ni->ni_vap->iv_bss;
    struct ieee80211vap *vap = iv_bss->ni_vap;
    struct ieee80211com *ic = vap->iv_ic;
    uint16_t capinfo;

    ni->ni_chwidth = ic->ic_cwm_get_width(ic);
    ni->ni_flags = iv_bss->ni_flags;

    ni->ni_htcap = iv_bss->ni_htcap;
    if (ni->ni_htcap)
        ni->ni_flags |= IEEE80211_NODE_HT;

    ni->ni_vhtcap = iv_bss->ni_vhtcap;
    if (ni->ni_vhtcap)
        ni->ni_flags |= IEEE80211_NODE_VHT;

    ni->ni_ext_flags |= iv_bss->ni_ext_flags;

    if (IEEE80211_IS_CHAN_11AX_SUPPORTED(vap->iv_bsschan) && ieee80211vap_heallowed(vap))
        ni->ni_ext_flags |= IEEE80211_NODE_HE;

    /* Mark EHT node */
    ni->ni_ext_flags |= IEEE80211_NODE_EHT;
    qdf_mem_copy(&ni->ni_he, &ni->ni_ic->ic_he, sizeof(struct ieee80211_he_handle));

    capinfo = IEEE80211_CAPINFO_ESS;
    if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap))
        capinfo |= IEEE80211_CAPINFO_PRIVACY;

    if ((ic->ic_flags & IEEE80211_F_SHPREAMBLE) &&
            IEEE80211_IS_CHAN_2GHZ(vap->iv_bsschan))
        capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;

    if (ic->ic_flags & IEEE80211_F_SHSLOT)
        capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;

    if (ieee80211_ic_doth_is_set(ic) &&
            ieee80211_vap_doth_is_set(vap))
        capinfo |= IEEE80211_CAPINFO_SPECTRUM_MGMT;

    if (IEEE80211_VAP_IS_PUREB_ENABLED(vap))
        capinfo &= ~IEEE80211_CAPINFO_SHORT_SLOTTIME;

    /* set rrm capbabilities, if supported */
    if (ieee80211_vap_rrm_is_set(vap))
        capinfo |= IEEE80211_CAPINFO_RADIOMEAS;
    ni->ni_capinfo = capinfo;

    ni->ni_lintval = iv_bss->ni_lintval;
    ni->ni_maxampdu = iv_bss->ni_maxampdu;
    ni->ni_mpdudensity = iv_bss->ni_mpdudensity;
    ni->ni_minimumrate = iv_bss->ni_minimumrate;

    qdf_mem_copy(&ni->ni_rates, &iv_bss->ni_rates, sizeof(struct ieee80211_rateset));
    qdf_mem_copy(&ni->ni_htrates, &iv_bss->ni_htrates, sizeof(struct ieee80211_rateset));
    ni->ni_streams = ieee80211_get_txstreams(ic, vap);
    ni->ni_maxrate_vht = 0xff;
    ni->ni_rx_max_rate = vap->iv_vhtcap_max_mcs.rx_mcs_set.data_rate;
    OS_MEMCPY(&ni->ni_tx_vhtrates,
              &vap->iv_vhtcap_max_mcs.tx_mcs_set.mcs_map, sizeof(ni->ni_tx_vhtrates));
    OS_MEMCPY(&ni->ni_rx_vhtrates,
              &vap->iv_vhtcap_max_mcs.rx_mcs_set.mcs_map, sizeof(ni->ni_rx_vhtrates));
    ni->ni_tx_max_rate = vap->iv_vhtcap_max_mcs.tx_mcs_set.data_rate;
    ni->ni_higher_vhtmcs_supp = iv_bss->ni_higher_vhtmcs_supp;
    ni->ni_he_width_set_org = iv_bss->ni_he_width_set_org;
    ni->ni_tx_higher_hemcs_supp = iv_bss->ni_tx_higher_hemcs_supp;
    ni->ni_rx_higher_hemcs_supp = iv_bss->ni_rx_higher_hemcs_supp;
    ni->wnm_bss_idle_option = iv_bss->wnm_bss_idle_option;
#if QCN_IE
    ni->ni_bsscolor_rept_info = iv_bss->ni_bsscolor_rept_info;
#endif

    qdf_mem_copy(&ni->ni_eht, &ic->ic_eht, sizeof(struct ieee80211_eht_handle));
    qdf_mem_copy(&ni->ni_ch_param, &iv_bss->ni_ch_param, sizeof(struct ni_chan_param));
    ni->ni_prop_ie_used = iv_bss->ni_prop_ie_used;
    ni->ni_ext_nss_support = iv_bss->ni_ext_nss_support;
    ni->ni_bw160_nss = iv_bss->ni_bw160_nss;
    ni->ni_bw80p80_nss = iv_bss->ni_bw80p80_nss;
    ni->ni_phymode = ni->ni_vap->iv_cur_mode;
    ieee80211_validate_eht_txrx_mcsnssmap(ic, ni);
    if (QDF_STATUS_SUCCESS != ieee80211_bridge_peer_set_security(ni,
                                                                 ml_peer)) {
        qdf_err("Failed to set security param for bridge peer");
        return;
    }
}

QDF_STATUS wlan_mlme_bridge_peer_create(struct wlan_objmgr_vdev *vdev,
					struct wlan_mlo_peer_context *ml_peer,
					struct qdf_mac_addr *addr,
					qdf_nbuf_t frm_buf)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_peer *peer;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        goto peer_create_fail;
    }

    ic = vap->iv_ic;
    if(ic == NULL) {
        goto peer_create_fail;
    }

    if (wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
        wlan_vdev_mlme_op_flags_get(vdev, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE))
        goto peer_create_fail;

    ni = ieee80211_mlo_bridge_node(vap, &addr->bytes[0]);

    if (!ni) {
        goto peer_create_fail;
    }

    peer = ni->peer_obj;
    wlan_peer_set_mlo(peer);

    ni->ni_associd = ml_peer->assoc_id;
    qdf_copy_macaddr((struct qdf_mac_addr *)&peer->mldaddr,
                     &ml_peer->peer_mld_addr);

    /* free the extra refcount got from ieee80211_mlo_bridge_node() */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

    peer->mlo_bridge_peer = true;
    ieee80211_inherit_bss_self_peer(ni, ml_peer);

    /* Update bridge peer count of VAP and RADIO */
    IEEE80211_VAP_LOCK(vap);
    vap->iv_bridge_cnt++;
    IEEE80211_VAP_UNLOCK(vap);

    IEEE80211_COMM_LOCK(ic);
    ic->ic_bridge_cnt++;
    IEEE80211_COMM_UNLOCK(ic);

    ret = wlan_mlo_link_peer_attach(ml_peer, peer, NULL);
    if (QDF_IS_STATUS_ERROR(ret)) {
        goto peer_create_fail;
    }

    return QDF_STATUS_SUCCESS;

peer_create_fail:
    qdf_info("Bridge peer create failed: %s", ether_sprintf(addr->bytes));
    wlan_mlo_partner_peer_create_failed_notify(ml_peer);
    if (ni) {
        /* Freeing the node, as the peer is not used */
        IEEE80211_NODE_LEAVE(ni);
    }

    return QDF_STATUS_E_FAILURE;
}


void wlan_mlme_peer_assoc(struct wlan_objmgr_peer *peer)
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic;
    wlan_if_t vap = NULL;
#ifdef WLAN_FEATURE_11BE
    enum wlan_t2lm_resp_frm_type t2lm_resp_type = WLAN_T2LM_RESP_TYPE_INVALID;
#endif /* WLAN_FEATURE_11BE */
    int retval;

    if (wlan_objmgr_peer_try_get_ref(peer, WLAN_MLO_MGR_ID)!=
                                                  QDF_STATUS_SUCCESS)
       return;

    vdev = wlan_peer_get_vdev(peer);
    if (!vdev) {
        goto peer_assoc_fail;
    }

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        goto peer_assoc_fail;
    }

    ic = vap->iv_ic;
    if(ic == NULL) {
        goto peer_assoc_fail;
    }

    if (!wlan_peer_is_mlo(peer))
        goto peer_assoc_fail;
    else
        ml_peer = peer->mlo_peer_ctx;

    if (!ml_peer)
        goto peer_assoc_fail;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        goto peer_assoc_fail;
    }
    if (wlan_mlo_peer_is_assoc_done(ml_peer) == QDF_STATUS_SUCCESS) {
        if (!wlan_mlo_peer_is_nawds(ml_peer) && !wlan_mlo_peer_is_mesh(ml_peer)) {
#ifdef WLAN_FEATURE_11BE
            t2lm_resp_type = ml_peer->t2lm_policy.ongoing_tid_to_link_mapping.t2lm_resp_type;

            if (wlan_peer_mlme_is_assoc_peer(ni->peer_obj) &&
                    (t2lm_resp_type == WLAN_T2LM_RESP_TYPE_SUCCESS)) {
                /* Copy the ongoing T2LM info to successfully negotiated info
                 * structure.
                 */
                ieee80211_copy_negotiated_t2lm_info_to_peer_ctx(ml_peer,
                        &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping, true);
            }
#endif /* WLAN_FEATURE_11BE */
            retval = ic->ic_newassoc(ni, 1);
            if (retval)
                goto peer_assoc_fail;

            /* In case of OMN in Asoc req, phymode is not updated
             * and therefore we explicitly send chwidth in OMN
             * to target. This is confirmed by checking if chan width
             * obtained from phymode and peer ch width differ.
             */
            if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
                ic->ic_chwidth_change(ni);
            ieee80211node_set_extflag(ni, IEEE80211_NODE_ASSOC_RESP);
            ieee80211_update_noderates(ni);
#if QCA_SUPPORT_SSID_STEERING
            /* Send assoc notify for link address */
            if (ieee80211_is_ssid_steering_vdev_pvt(vap)){
                IEEE80211_DELIVER_SSID_EVENT(vap, ni->ni_macaddr);
            }
#endif
            /* Reset last ASSOC Rx time for partner links,
               since assoc resp is sent assoc link */
            ni->ni_last_assoc_rx_time = 0;
            ni->ni_last_authorize_rst_time = OS_GET_TIMESTAMP();
        } else if (wlan_mlo_peer_is_nawds(ml_peer)) {
            wlan_mlnawds_assoc_peer((struct ieee80211vap *)vap,
                                    ni,
                                    wlan_peer_get_macaddr(peer));
        } else {
            wlan_mlmesh_assoc_peer((struct ieee80211vap *)vap,
                                   ni,
                                   wlan_peer_get_macaddr(peer));
        }
    }
    else {
        goto peer_assoc_fail;
    }

    wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
    return;

peer_assoc_fail:

     /* if peer_fw_create_fail is 1, peer delete is already initiated,
        so skip peer create failure */
    if ((ml_peer) && (ni) && (!ni->peer_fw_create_fail)) {
        wlan_mlo_partner_peer_create_failed_notify(ml_peer);
    }

    wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
}

void wlan_mlme_peer_delete(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni = NULL;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_vdev *vdev;
    struct wlan_mlo_peer_context *ml_peer;

    if (wlan_objmgr_peer_try_get_ref(peer, WLAN_MLO_MGR_ID)!=
                                                  QDF_STATUS_SUCCESS)
       return;

    vdev = wlan_peer_get_vdev(peer);
    if (!vdev) {
        wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
        return;
    }

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
        return;
    }

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
        return;
    }

    ml_peer = peer->mlo_peer_ctx;

    if (wlan_mlo_peer_is_nawds(ml_peer)) {
        wlan_mlo_mlnawds_clear_nawds_entry(vap, ni->ni_macaddr, peer->mldaddr);
    }

    IEEE80211_NODE_LEAVE(ni);

    wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);

    return;
}

void wlan_mlme_peer_assoc_fail(struct wlan_objmgr_peer *peer)
{
    struct ieee80211_node *ni = NULL;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_vdev *vdev;

    vdev = wlan_peer_get_vdev(peer);
    if (!vdev) {
        return;
    }

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        return;
    }

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        return;
    }

    if (!wlan_peer_mlme_is_assoc_peer(peer)) {
        mlo_err("vdev%d: This is not assoc peer, so returning for MAC addr "QDF_MAC_ADDR_FMT,
                 vap->iv_unit, QDF_MAC_ADDR_REF(peer->mldaddr));
        return;
    }

    if (wlan_objmgr_peer_try_get_ref(peer, WLAN_MLO_MGR_ID) ==
                                                  QDF_STATUS_SUCCESS) {
         ni->ni_assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
#if WLAN_SUPPORT_SPLITMAC
         if (splitmac_is_enabled(vdev)) {
               /* Free assoc response buffer, since internl processing of
                  add client is failed */
             if ((ni->ni_hostapd_asresp_trig) && (ni->ni_assoc_resp_wbuf)) {
                 wbuf_free(ni->ni_assoc_resp_wbuf);
                 ni->ni_hostapd_asresp_trig = 0;
                 ni->ni_assoc_resp_wbuf = NULL;
#if UMAC_SUPPORT_ACFG
                 acfg_assoc_failure_indication_event(vap, ni->ni_macaddr,
                                                     IEEE80211_STATUS_TOOMANY);
#endif

             }
         } else
#endif
         {
             ieee80211_send_assocresp(ni, 0, IEEE80211_REASON_ASSOC_TOOMANY, NULL);
             IEEE80211_NODE_LEAVE(ni);
         }
         wlan_objmgr_peer_release_ref(peer, WLAN_MLO_MGR_ID);
    }

    return;
}

void wlan_mlme_peer_deauth(struct wlan_objmgr_peer *peer, uint8_t is_disassoc)
{
    struct ieee80211_node *ni = NULL;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_vdev *vdev;

    vdev = wlan_peer_get_vdev(peer);
    if (!vdev) {
        return;
    }

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        return;
    }

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni) {
        return;
    }

    ieee80211_try_mark_node_for_delayed_cleanup(ni);
    if (is_disassoc) {
        ieee80211_send_disassoc_with_callback(ni,
                IEEE80211_REASON_ASSOC_LEAVE,
                ieee80211_mlme_frame_complete_handler, ni);
    } else {
        ieee80211_send_deauth(ni, IEEE80211_REASON_ASSOC_LEAVE);
    }

    return;
}

#ifdef UMAC_MLO_AUTH_DEFER
void wlan_mlme_peer_process_auth(struct mlpeer_auth_params *pending_auth_param)
{
  /* copy back AUTH params
   * Remove refrence for that node ideally it should be freed
   * call mlme_recv_auth_ap_defer with cached params
   */
    struct recv_auth_params_defer auth_params;
    struct ieee80211_node *tmpni = NULL;
    struct wlan_objmgr_psoc *psoc = NULL;
    struct wlan_objmgr_vdev *frm_recv_vdev = NULL;
    struct ieee80211vap *frm_recv_vap = NULL;
    qdf_nbuf_t wbuf = NULL;

    psoc = wlan_objmgr_get_psoc_by_id(pending_auth_param->psoc_id, WLAN_MLO_MGR_ID);
    if (!psoc)
        goto exit;

    frm_recv_vdev = wlan_objmgr_get_vdev_by_id_from_psoc(psoc,
                pending_auth_param->vdev_id, WLAN_MLO_MGR_ID);
    if (!frm_recv_vdev)
         goto exit;

    auth_params.algo = pending_auth_param->algo;
    auth_params.seq = pending_auth_param->seq;
    auth_params.status_code = pending_auth_param->status_code;
    if (qdf_is_macaddr_zero(&pending_auth_param->mldaddr)) {
        qdf_zero_macaddr(&auth_params.mldaddr);
    }
    else {
        qdf_copy_macaddr(&auth_params.mldaddr, &pending_auth_param->mldaddr);
    }

    auth_params.challenge = pending_auth_param->challenge;
    auth_params.challenge_length = pending_auth_param->challenge_length;
    auth_params.vdev_id = wlan_vdev_get_id(frm_recv_vdev);
    auth_params.wbuf = pending_auth_param->wbuf;
    wbuf = auth_params.wbuf;
    qdf_mem_copy(&auth_params.rs, pending_auth_param->rs, sizeof(struct ieee80211_rx_status));
     /* Free auth param and radio status struct */
    pending_auth_param->wbuf = NULL;
    qdf_mem_free(pending_auth_param->rs);
    qdf_mem_free(pending_auth_param);

    frm_recv_vap = wlan_vdev_get_mlme_ext_obj(frm_recv_vdev);
    if (!frm_recv_vap)
        goto exit;

    tmpni = ieee80211_ref_node(frm_recv_vap->iv_bss, WLAN_MLME_HANDLER_ID);
    if (!tmpni)
        goto exit;

    mlme_recv_auth_ap_defer(tmpni, 1, &auth_params, 1);
    /* Free reference taken during find_node above */
    ieee80211_free_node(tmpni, WLAN_MLME_HANDLER_ID);
    if (frm_recv_vdev)
        wlan_objmgr_vdev_release_ref(frm_recv_vdev, WLAN_MLO_MGR_ID);
    if (psoc)
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLO_MGR_ID);

    return;

exit:

    if (frm_recv_vdev)
        wlan_objmgr_vdev_release_ref(frm_recv_vdev, WLAN_MLO_MGR_ID);
    /* Couldn't process this request. Drop frame */
    if (wbuf)
        wbuf_free(wbuf);
    if (psoc)
        wlan_objmgr_psoc_release_ref(psoc, WLAN_MLO_MGR_ID);

    return;

}
#endif

QDF_STATUS wlan_mlme_peer_reassoc(struct wlan_objmgr_vdev *vdev,
                                  struct wlan_mlo_peer_context *ml_peer,
                                  struct qdf_mac_addr *addr,
                                  qdf_nbuf_t frm_buf)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic;
    wlan_if_t vap = NULL;
    struct wlan_objmgr_peer *peer = NULL;
    QDF_STATUS ret = QDF_STATUS_SUCCESS;
    struct ieee80211_frame *wh = NULL;
    uint8_t subtype = 0;
    uint8_t reassoc = 0;
    wbuf_t link_req_wbuf = NULL;
    wbuf_t link_resp_wbuf = NULL;
    struct qdf_mac_addr link_addr;
    qdf_size_t link_req_len = 0;
    uint8_t link_id;
    bool nawds_peer = false, mesh_peer = false;
    int is_splitmac_enable = 0;

    vap = (wlan_if_t)wlan_vdev_get_mlme_ext_obj(vdev);
    if(vap == NULL) {
        qdf_err("vap is NULL");
        goto peer_reassoc_fail;
    }

    ic = vap->iv_ic;
    if(ic == NULL) {
        qdf_err("ic is NULL");
        goto peer_reassoc_fail;
    }

    mlo_debug("vdev%d: Partner peer reassoc frame from "QDF_MAC_ADDR_FMT,
               wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(addr->bytes));
    ni = ieee80211_find_node(ic, &addr->bytes[0], WLAN_MLO_MGR_ID);
    if (!ni) {
        mlo_err("vdev%d: Partner peer is NULL, dropping reassoc frame from "QDF_MAC_ADDR_FMT,
                wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(addr->bytes));
        goto peer_reassoc_fail;
    } else {
        peer = ni->peer_obj;
        if (!wlan_peer_is_mlo(peer)) {
            mlo_err("vdev%d: Partner peer is non-MLO, dropping reassoc frame from "QDF_MAC_ADDR_FMT,
                    wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(addr->bytes));
            goto peer_reassoc_fail;
        }

        if (!wlan_mlo_peer_is_link_peer(ml_peer, peer)) {
            mlo_err("vdev%d: Partner peer is not a link peer, dropping reassoc frame from "QDF_MAC_ADDR_FMT,
                    wlan_vdev_get_id(vdev), QDF_MAC_ADDR_REF(addr->bytes));
            goto peer_reassoc_fail;
        }
    }


    if (ni->ni_ml_rptingrole >= IEEE80211_MLO_RPTINGROLE_INVALID) {
        qdf_rl_err("Invalid MLO reporting role value %d for ni with MAC address %s",
                ni->ni_ml_rptingrole, ether_sprintf(ni->ni_macaddr));
        goto peer_reassoc_fail;
    }

#if WLAN_SUPPORT_SPLITMAC
    is_splitmac_enable = splitmac_is_enabled(vdev);
#endif

    nawds_peer = wlan_mlo_peer_is_nawds(ml_peer);
    mesh_peer = wlan_mlo_peer_is_mesh(ml_peer);
    if ((ni->ni_ml_rptingrole != IEEE80211_MLO_RPTINGROLE_REPORTINGSTA) &&
        ((!nawds_peer) && (!mesh_peer))) {

        ni->ni_ml_rptingrole = IEEE80211_MLO_RPTINGROLE_REPORTEDSTA;

        wh = (struct ieee80211_frame *) wbuf_header(frm_buf);
        subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

        if ((subtype != IEEE80211_FC0_SUBTYPE_ASSOC_REQ) &&
                (subtype != IEEE80211_FC0_SUBTYPE_REASSOC_REQ)) {
            mloie_err_rl("Unsupported or invalid subtype %u", subtype);
            goto peer_reassoc_fail;
        }

        reassoc = (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) ? 1 : 0;

        qdf_mem_copy(link_addr.bytes, vap->iv_bss->ni_bssid,
                sizeof(link_addr.bytes));

        link_req_wbuf =
            wbuf_alloc(ic->ic_osdev, WBUF_TX_MGMT, MAX_TX_RX_PACKET_SIZE);

        if (!link_req_wbuf) {
            mloie_err_rl("Unable to allocate wbuf for link specific assoc request");
            goto peer_reassoc_fail;
        }

        mloie_debug("Generating link specific assoc request for reported STA with link MAC address %s",
                ether_sprintf(addr->bytes));
        link_id = wlan_vdev_get_link_id(vdev);
        if (util_gen_link_assoc_req(
                    wbuf_header(frm_buf) + sizeof(struct ieee80211_frame),
                    wbuf_get_pktlen(frm_buf) - sizeof(struct ieee80211_frame),
                    reassoc, link_id, link_addr,
                    wbuf_header(link_req_wbuf), MAX_TX_RX_PACKET_SIZE,
                    &link_req_len) != QDF_STATUS_SUCCESS) {
            mloie_err_rl("Unable to generate link specific assoc request");
            goto peer_reassoc_fail;
        }

        wbuf_set_pktlen(link_req_wbuf, link_req_len);

        mloie_debug("Processing link specific assoc request for reported STA with link MAC address %s",
                ether_sprintf(addr->bytes));
        if (ieee80211_recv_asreq(ni, link_req_wbuf, subtype, NULL, 0) != 0) {
            /* Error handling for reported STAs will be added in a future
             * change after the peer management and hostapd/supplicant
             * dependencies are met.
             */
            mloie_err_rl("Unable to carry out receive processing for link specific assoc request");
            goto peer_reassoc_fail;
        }

        if (!is_splitmac_enable) {
            mloie_debug("Generating link specific assoc response for reported STA with link MAC address %s",
                    ether_sprintf(addr->bytes));
            link_resp_wbuf = ieee80211_setup_assocresp(ni, NULL, reassoc,
                    IEEE80211_STATUS_SUCCESS, NULL, (uint8_t *)ml_peer, NULL, NULL);

            if (link_resp_wbuf == NULL) {
                mloie_err_rl("Unable to generate link specific assoc response");
                goto peer_reassoc_fail;
            }
        }
    }
    if (!(ni->ni_ext_flags & IEEE80211_NODE_EHT)) {
        wbuf_free(link_resp_wbuf);
        qdf_rl_err("EHT flag is not set for ni with MAC address %s",
                   ether_sprintf(ni->ni_macaddr));
        goto peer_reassoc_fail;
    }

    ret = wlan_mlo_link_asresp_attach(ml_peer, peer, link_resp_wbuf);
    if (QDF_IS_STATUS_ERROR(ret)) {
        wbuf_free(link_resp_wbuf);
        goto peer_reassoc_fail;
    }

    if (link_req_wbuf)
        wbuf_free(link_req_wbuf);

    ieee80211_free_node(ni, WLAN_MLO_MGR_ID);

    return QDF_STATUS_SUCCESS;

peer_reassoc_fail:
    if (link_req_wbuf)
        wbuf_free(link_req_wbuf);

    if (ni) {
        wlan_mlo_link_asresp_attach(ml_peer, peer, NULL);
        ieee80211_free_node(ni, WLAN_MLO_MGR_ID);
    }

    return QDF_STATUS_E_FAILURE;
}
#endif
