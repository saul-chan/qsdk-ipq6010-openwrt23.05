/*
 * Copyright (c) 2011-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 *
 */

#include "ieee80211_node_priv.h"
#include "ieee80211_wds.h"
#include "ieee80211_var.h"
#include "ieee80211_api.h"
#include <osif_private.h>
#include <ieee80211_sme_api.h>
#include <ieee80211_cfg80211.h>
#if UNIFIED_SMARTANTENNA
#include <wlan_sa_api_utils_api.h>
#endif
#include <mlme/ieee80211_mlme_priv.h>
#if WLAN_SUPPORT_GREEN_AP
#include <wlan_green_ap_api.h>
#endif
#if WLAN_SUPPORT_SPLITMAC
#include <wlan_splitmac.h>
#endif

#if QCA_AIRTIME_FAIRNESS
#include <wlan_atf_utils_api.h>
#endif
#include "ol_if_athvar.h"

#ifdef WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#endif

#include "wlan_crypto_global_def.h"
#include "wlan_crypto_global_api.h"
#include <wlan_mlme_dp_dispatcher.h>
#include <wlan_vdev_mlme.h>
#include <wlan_reg_services_api.h>
#include <target_type.h>
#include <wlan_repeater_api.h>
#include <wlan_reg_channel_api.h>
#include <wlan_mlo_mgr_sta.h>
#include <cfg80211_external.h>
#include <wlan_mlme_if.h>

#include <wlan_mbss.h>
#ifdef WLAN_FEATURE_11BE
#include <wlan_mlo_t2lm.h>
#endif
#if defined(WLAN_SUPPORT_SCS) && defined(SAWF_ADMISSION_CONTROL)
#include <ieee80211_ucfg.h>
#endif

#if WLAN_OBJMGR_REF_ID_TRACE
#define node_reclaim(nt,ni)  _node_reclaim(nt,ni,__func__,__LINE__,__FILE__)
#endif

#if defined(CONFIG_SAWF_DEF_QUEUES) && defined(WLAN_SUPPORT_SCS)
QDF_STATUS ieee80211_disable_service_class(struct ieee80211com *ic,
                                           uint8_t svc_id);
#endif

static void
ieee80211_node_table_reset(struct ieee80211_node_table *nt, struct ieee80211vap *match);
static void
ieee80211_node_table_reset_nolock(struct ieee80211_node_table *nt, struct ieee80211vap *match);

struct ieee80211_iter_arg {
    int32_t count;
    wlan_if_t vap;
    u_int32_t flag;
    struct ieee80211_node *nodes[IEEE80211_512_AID];
};

static void ieee80211_node_iter(struct wlan_objmgr_vdev *vdev,
                                void *object, void *arg);
#ifdef UMAC_SUPPORT_PROXY_ARP
void
ieee80211_node_remove_ipv6_by_node(struct ieee80211_node_table *nt, struct ieee80211_node *ni);
#endif

static struct ieee80211_node *
node_alloc(struct ieee80211vap *vap, const u_int8_t *macaddr, bool tmpnode, void *peer)
{
     struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;

    /* create a node */
    ni = (struct ieee80211_node *)OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_node), GFP_KERNEL);
    if (ni == NULL) {
        qdf_nofl_info("Can't create an node\n");
        return NULL;
    }
    OS_MEMZERO(ni, sizeof(struct ieee80211_node));

    return ni;
}

#if defined(WLAN_SUPPORT_SCS) && defined(SAWF_ADMISSION_CONTROL)
static void ieee80211_scs_session_timeout_handler(void *arg)
{
    uint8_t index = 0, scs_id = 0, request_type = 0, response = 0, scs_index = 0;
    struct ieee80211_scs_data *scs_data = NULL;
    struct ieee80211_node *ni = (struct ieee80211_node *)arg;
    struct ieee80211_node *assoc_ni = ni;
#ifdef WLAN_FEATURE_11BE_MLO
    bool flag = false;
#endif

    qdf_spin_lock_bh(&ni->ni_scs_lock);

    /* RM response received before timer expiry */
    if (!ni->transaction_key)
       goto unlock;

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_peer_is_mlo(ni->peer_obj) && !wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {
        assoc_ni = ieee80211_ucfg_get_mlo_assoc_link_node(ni->peer_obj->mlo_peer_ctx);
        if (!assoc_ni)
            goto unlock;

        qdf_spin_lock_bh(&assoc_ni->ni_scs_lock);
        flag = true;
    }
#endif

    /* Traverse in reverse direction to avoid multiple shifting overhead */
    for (index = assoc_ni->ni_scs_sessions; index > 0; index--) {
        scs_index = index - 1;
        scs_data = assoc_ni->ni_scs[scs_index];

        if (scs_data->scs_state == SCS_STATE_APP_RESP_PENDING) {
            scs_id = scs_data->scsid;
            request_type = scs_data->request_type;

            ieee80211_scs_del_stream(assoc_ni, scs_index);

            if (request_type == IEEE80211_SCS_ADD_RULE)
                response = IEEE80211_SCS_REQUEST_DECLINED;
            else if (request_type == IEEE80211_SCS_CHANGE_RULE)
                response = IEEE80211_SCS_TCLAS_PROCESSING_TERMINATED;

            /* Prepare response in ni to handle multi SCS descriptor case */
            ieee80211_prepare_scs_resp(ni, scs_id, response);
        }
    }

    /* Update response_pending to false to process the new request when it comes */
    assoc_ni->request_processing = false;

    /* Update transaction_key to 0 to ignore the RM response if received after timer expiry */
    ni->transaction_key = 0;

#ifdef WLAN_FEATURE_11BE_MLO
    if (flag) {
        /* Release lock and reference on assoc_node */
        qdf_spin_unlock_bh(&assoc_ni->ni_scs_lock);
        wlan_objmgr_peer_release_ref(assoc_ni->peer_obj, WLAN_MLME_NB_ID);
    }
#endif

    ieee80211_send_scs_resp(ni);

unlock:
    qdf_spin_unlock_bh(&ni->ni_scs_lock);
}
#endif

/*
* allocates a node ,sets up the node and inserts the node into the node table.
* the allocated node will have 2 references one for adding it to the table and the
* the other for the caller to use.
*/

struct ieee80211_node *
ieee80211_alloc_node(struct ieee80211_node_table *nt,
                     struct ieee80211vap *vap,
                     struct wlan_objmgr_peer *peer)
{
    struct ieee80211com *ic = nt->nt_ic;
    struct ieee80211_node *ni;
    rwlock_state_t lock_state;
    uint8_t *macaddr;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);
#ifdef WLAN_SUPPORT_SCS
    uint8_t i = 0;
#ifdef WLAN_FEATURE_11BE
    uint8_t ac = 0;
#endif
#endif
     /* get MAC address from peer */
    macaddr = wlan_peer_get_macaddr(peer);

    if((!(IEEE80211_ADDR_IS_VALID(macaddr))) || (IEEE80211_IS_MULTICAST(macaddr))){
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE,
                       "%s : Invalid MAC Address:%s \n",__func__, ether_sprintf(macaddr));
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_invalid_macaddr_nodealloc_fail_inc(vap->vdev_obj, 1);
#endif
        return NULL;
    }
    if(IEEE80211_CHK_NODE_TARGET(ic))
        return NULL;
    ni = ic->ic_node_alloc(vap, macaddr, FALSE /* not temp node */, peer);
    if (ni == NULL) {
        /* XXX msg */
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_rx_node_alloc_inc(vap->vdev_obj, 1);
#endif
        return NULL;
    }
    ni->peer_obj = peer;

#if IEEE80211_DEBUG_NODELEAK
    OS_RWLOCK_WRITE_LOCK(&ic->ic_nodelock,&lock_state);
    TAILQ_INSERT_TAIL(&ic->ic_nodes, ni, ni_alloc_list);
    OS_RWLOCK_WRITE_UNLOCK(&ic->ic_nodelock,&lock_state);
#endif

    /* copy some default variables from parent */
    IEEE80211_ADDR_COPY(ni->ni_macaddr, macaddr);

    ni->ni_ic = ic;
    if (vap->iv_opmode == IEEE80211_M_IBSS) {
        /* underlined vap is configured for IBSS so allocate
         * persta key memory and initialize it.
         */
        ni->ni_persta = (struct ni_persta_key *)
                        OS_MALLOC(ic->ic_osdev,sizeof(struct ni_persta_key), GFP_ATOMIC);
        if (ni->ni_persta == NULL) {
            _ieee80211_free_node(ni);
            qdf_nofl_info ("%s: freeing node as unable to allocate memory for ni_persta", __func__);
            return NULL;
        }
        OS_MEMSET(ni->ni_persta, 0 , sizeof(struct ni_persta_key));
    } else {
        /* Explicitly set ni_persta as NULL */
        ni->ni_persta = NULL;
    }

#if UMAC_SUPPORT_WNM
    ni->ni_wnm = (struct ieee80211_wnm_node *) OS_MALLOC(ic->ic_osdev,
                      (sizeof(struct ieee80211_wnm_node)),0);

    if(ni->ni_wnm == NULL) {
        if (ni->ni_persta) {
            OS_FREE(ni->ni_persta);
            ni->ni_persta = NULL;
        }
        _ieee80211_free_node(ni);
        qdf_nofl_info ("%s: freeing node as unable to allocate memory for ni_wnm", __func__);
        return NULL;
    }
    OS_MEMSET(ni->ni_wnm, 0 , sizeof(struct ieee80211_wnm_node));
    /* ieee80211_wnm_nattach frees ni->ni_wnm if in case of failure */
    ieee80211_wnm_nattach(ni);
    if(ni->ni_wnm == NULL) {
        if (ni->ni_persta) {
            OS_FREE(ni->ni_persta);
            ni->ni_persta = NULL;
        }
        _ieee80211_free_node(ni);
        qdf_nofl_info ("%s: freeing node as unable to allocate memory for ni_wnm", __func__);
        return NULL;
    }

#endif
#if WLAN_SUPPORT_DAP
    if (vap->iv_dscp_action_policy) {
         ni->ni_dscp_action = (struct ieee80211_dscp_data *) OS_MALLOC(ic->ic_osdev,
                 (sizeof(struct ieee80211_dscp_data)),0);

         if (ni->ni_dscp_action == NULL)
             return NULL;

         OS_MEMSET(ni->ni_dscp_action, 0, sizeof(struct ieee80211_dscp_data));
    }
#endif

#ifdef WLAN_SUPPORT_SCS
    qdf_spinlock_create(&ni->ni_scs_lock);

    /* Initialize dl svc_id and refcount with 0 value */
    for (i = 0; i < MAX_SCS_REQUEST_PER_DIRECTION; i++) {
        ni->dl_info.dl_node[i].svc_id = 0;
        ni->dl_info.dl_node[i].ref_cnt = 0;
    }

#ifdef SAWF_ADMISSION_CONTROL
    /* SCS Timer Initializtion */
    qdf_timer_init(NULL, &ni->ni_scs_timer, ieee80211_scs_session_timeout_handler, ni, QDF_TIMER_TYPE_WAKE_APPS);
#endif

#if UMAC_SUPPORT_WNM
    OS_INIT_TIMER(NULL, &ni->ni_btm_disassoc_timer,
                  ieee80211_btm_disassoc_sta, ni,
                  QDF_TIMER_TYPE_WAKE_APPS);
#endif /* UMAC_SUPPORT_WNM */

#ifdef WLAN_FEATURE_11BE
    for (ac = 0; ac < WME_NUM_AC; ac++)
        TAILQ_INIT(&ni->ac_qos_list[ac]);

    qdf_mem_zero(&ni->ul_info, sizeof(struct ul_svc_info));
#endif
#endif

    IEEE80211_NODE_STATE_LOCK_INIT(ni);

    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    ni->ni_vap = vap;

    ni->ni_table = nt;
    ieee80211_ref_node(ni, WLAN_MLME_OBJMGR_ID);     /* mark referenced for adding it to  the node table*/

#if WLAN_OBJMGR_REF_ID_TRACE
    if (vap->iv_ref_leak_test_flag) {
        ieee80211_ref_node(ni, WLAN_MLME_OBJMGR_ID);     /* mark referenced for adding it to  the node table*/
    }
#endif
    ni->ni_bss_node = ni;
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    ieee80211_node_saveq_attach(ni);

    ieee80211_node_reset(ni);
    ieee80211_admctl_init(ni);
    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                   "%s: vap=0x%x, peercount=%d, ni=0x%x, ni_bss_node=0x%x bss_ref=%d \n",
                   __func__, vap, wlan_vdev_get_peer_count(vap->vdev_obj),
                   ni, ni->ni_bss_node, wlan_objmgr_node_refcnt(ni));

    ni->ni_snr_min = CDP_INVALID_SNR;

    return ni;
}

/*
 * Reclaim any resources in a node and reset any critical
 * state.  Typically nodes are free'd immediately after,
 * but in some cases the storage may be reused so we need
 * to insure consistent state (should probably fix that).
 */
static void
node_cleanup(struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct wlan_objmgr_peer *peer = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;

    peer = ni->peer_obj;
    if (peer)
        vdev = wlan_peer_get_vdev(peer);

#define       N(a)    (sizeof(a)/sizeof(a[0]))

    ASSERT(vap);

    /*
     * Tmp node didn't attach pwr save staff, so skip ps queue
     * cleanup
     */
    if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)) {
        ieee80211_node_saveq_cleanup(ni);
    }

    /*
     * Preserve SSID, WPA, and WME ie's so the bss node is
     * reusable during a re-auth/re-assoc state transition.
     * If we remove these data they will not be recreated
     * because they come from a probe-response or beacon frame
     * which cannot be expected prior to the association-response.
     * This should not be an issue when operating in other modes
     * as stations leaving always go through a full state transition
     * which will rebuild this state.
     *
     * XXX does this leave us open to inheriting old state?
     */
#ifdef WLAN_FEATURE_11BE_MLO
    if (ni->ni_associd && !(peer && wlan_peer_is_mlo(peer))) {
        if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) || (vap->iv_ic->is_mbssid_enabled)) {
            mlme_free_aid(vap->vdev_obj, ni->ni_associd);
        }
        else if (vap->iv_aid_bitmap != NULL) {
            IEEE80211_AID_CLR(vap, ni->ni_associd);
        }
    }
#else
    if (ni->ni_associd && vap->iv_aid_bitmap != NULL) {
        IEEE80211_AID_CLR(vap, ni->ni_associd);
    }
#endif
    ni->ni_associd = 0;
    ni->ni_assocuptime = 0;
    ni->ni_rxkeyoff = 0;

#if WLAN_SUPPORT_SPLITMAC
    if (vdev && splitmac_is_enabled(vdev)) {
        splitmac_api_set_state(peer, SPLITMAC_NODE_INIT);
    }
#endif
#undef N
}

static void
node_free(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
#if defined(CONFIG_SAWF_DEF_QUEUES) && defined(WLAN_SUPPORT_SCS)
    uint8_t i = 0;
#endif

#define       N(a)    (sizeof(a)/sizeof(a[0]))

    ic->ic_node_cleanup(ni);

    if (ni->ni_challenge != NULL) {
        OS_FREE(ni->ni_challenge);
        ni->ni_challenge = NULL;
    }

    if (ni->ni_wpa_ie != NULL) {
        OS_FREE(ni->ni_wpa_ie);
        ni->ni_wpa_ie = NULL;
    }

    if (ni->ni_wps_ie != NULL) {
        OS_FREE(ni->ni_wps_ie);
        ni->ni_wps_ie = NULL;
    }

    if (ni->ni_supp_chan_ie != NULL) {
        OS_FREE(ni->ni_supp_chan_ie);
        ni->ni_supp_chan_ie = NULL;
    }

    if (ni->ni_ath_ie != NULL) {
        OS_FREE(ni->ni_ath_ie);
        ni->ni_ath_ie = NULL;
    }

    if (ni->ni_mbo_ie != NULL) {
        OS_FREE(ni->ni_mbo_ie);
        ni->ni_mbo_ie = NULL;
    }

    if (ni->ni_supp_op_class_ie != NULL) {
        OS_FREE(ni->ni_supp_op_class_ie);
        ni->ni_supp_op_class_ie = NULL;
    }

#if UMAC_SUPPORT_WNM

        if (ni->ni_wnm != NULL) {
            ieee80211_wnm_ndetach(ni);
            OS_FREE(ni->ni_wnm);
            ni->ni_wnm = NULL;
        }
#endif

#if WLAN_SUPPORT_DAP
    if (ni->ni_dscp_action != NULL) {
        OS_FREE(ni->ni_dscp_action);
        ni->ni_dscp_action = NULL;
    }
#endif

    if (ni->ni_wme_ie != NULL) {
        OS_FREE(ni->ni_wme_ie);
        ni->ni_wme_ie = NULL;
    }

    if (ni->ni_mscs != NULL) {
        OS_FREE(ni->ni_mscs);
        ni->ni_mscs = NULL;
    }

#ifdef WLAN_SUPPORT_SCS
    qdf_spin_lock_bh(&ni->ni_scs_lock);
    if (ni->ni_scs_sessions) {
        uint8_t session_idx = 1;
        uint8_t sessions = ni->ni_scs_sessions;
        uint8_t idx = sessions - 1;

        /* Delete SCS streams in revere order to avoid shifting overhead */
        while (idx >= 0 && session_idx <= sessions) {
            if (ni->ni_scs[idx]->scs_state == SCS_STATE_ACTIVE) {
                /* Rule and svc class config is done only for active SCS sessions */
                ieee80211_send_scs_rule_config_event(ni, idx,
                                                     IEEE80211_SCS_REMOVE_RULE);
            }
            ieee80211_scs_del_stream(ni, idx);
            idx--;
            session_idx++;
        }

        ni->ni_scs_sessions = 0;
    }

#ifdef CONFIG_SAWF_DEF_QUEUES
    /* Disable service classes created via SCS events from user space */
    for (i = 0; i < MAX_SCS_REQUEST_PER_DIRECTION; i++) {
        if (ni->dl_info.dl_node[i].svc_id) {
            while (ni->dl_info.dl_node[i].ref_cnt > 0) {
                ieee80211_disable_service_class(ic,
                                                ni->dl_info.dl_node[i].svc_id);
                ni->dl_info.dl_node[i].ref_cnt--;
            }
        }
    }
#endif

    qdf_spin_unlock_bh(&ni->ni_scs_lock);

#ifdef SAWF_ADMISSION_CONTROL
    qdf_timer_sync_cancel(&ni->ni_scs_timer);
    qdf_timer_free(&ni->ni_scs_timer);
#endif

    qdf_spinlock_destroy(&ni->ni_scs_lock);
#endif

#if UMAC_SUPPORT_WNM
    OS_FREE_TIMER(&ni->ni_btm_disassoc_timer);
#endif /* UMAC_SUPPORT_WNM */

    if (ni->ni_persta) {
        OS_FREE(ni->ni_persta);
        ni->ni_persta = NULL;
    }

#if UMAC_SUPPORT_RRM
    if (ni->ni_rrm_stats) {
        OS_FREE(ni->ni_rrm_stats);
        ni->ni_rrm_stats = NULL;
    }
#endif

    if (ni->ni_qcn_ie) {
        OS_FREE(ni->ni_qcn_ie);
        ni->ni_qcn_ie = NULL;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (ni->ni_assoc_resp_optie.ie) {
        qdf_mem_free( ni->ni_assoc_resp_optie.ie);
        ni->ni_assoc_resp_optie.ie = NULL;
        ni->ni_assoc_resp_optie.length = 0;
    }
#endif

    /* Tmp node doesn't attach the pwrsave queue */
    if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)) {
        ieee80211_node_saveq_detach(ni);
    }
    ieee80211_admctl_deinit(ni);
#undef N
}

static u_int8_t
node_getsnr(const struct ieee80211_node *ni,  int8_t chain, u_int8_t flags)
{
    return ni->ni_snr;
}

#if IEEE80211_DEBUG_NODELEAK
void wlan_debug_dump_nodes_tgt(void);
#endif

void
_ieee80211_free_node(struct ieee80211_node *ni)
{
    struct ieee80211vap         *vap = ni->ni_vap;
    struct ieee80211_node       *ni_bss_node = ni->ni_bss_node;
    struct ieee80211com         *ic = ni->ni_ic;
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    struct ol_ath_soc_softc *soc = scn->soc;

    ASSERT(vap);

#if DBG_LVL_MAC_FILTERING
    if (ni->ni_dbgLVLmac_on) {
        vap->iv_print.dbgLVLmac_on_cnt--;
        if (vap->iv_print.dbgLVLmac_on_cnt == 0) {
            qdf_info("freenode:dbgLVLmac disabled for all, disable it [vap%d]",
                     vap->iv_unit);
            vap->iv_print.dbgLVLmac_on = 0;
        }
    }
#endif
    if (ni->ni_table) {
        qdf_nofl_info("%s: WARN: Freeing node while its still present in node table"
               " ni: 0x%pK, vap: 0x%pK, bss_node: 0x%pK, ic: 0x%pK, ni_table: 0x%pK,"
               " ic_sta: 0x%pK, refcnt: %d\n", __func__, ni, vap, ni_bss_node,
               vap->iv_ic, ni->ni_table, (&(vap->iv_ic)->ic_sta),
               ieee80211_node_refcnt(ni));
    }

    if (ni->ni_ext_flags & IEEE80211_NODE_NON_DOTH_STA) {
        ic->ic_non_doth_sta_cnt--;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (ni->ni_asreq_buf) {
        wbuf_free(ni->ni_asreq_buf);
        ni->ni_asreq_buf = NULL;
    }

    if (ni->ni_assoc_resp_wbuf) {
        wbuf_free(ni->ni_assoc_resp_wbuf);
        ni->ni_assoc_resp_wbuf = NULL;
    }

    if (ni->ni_associd && !wlan_peer_is_mlo(ni->peer_obj)) {
        if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) || (ic->is_mbssid_enabled)) {
            mlme_free_aid(vap->vdev_obj, ni->ni_associd);
        }
        else if (vap->iv_aid_bitmap != NULL) {
            IEEE80211_AID_CLR(vap, ni->ni_associd);
        }
    }
#else
    if (ni->ni_associd && vap->iv_aid_bitmap != NULL)
        IEEE80211_AID_CLR(vap, ni->ni_associd);
#endif

    if ((ni->ni_flags & IEEE80211_NODE_TEMP) == 0) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "%s", "station free \n");
    }

    qdf_spin_lock_bh(&soc->soc_lock);
    if (soc->peer_ext_stats_cnt && (ni->ni_flags & IEEE80211_NODE_EXT_STATS)) {
        soc->peer_ext_stats_cnt--;
    }
    qdf_spin_unlock_bh(&soc->soc_lock);

#if IEEE80211_DEBUG_NODELEAK
    do {
        rwlock_state_t lock_state;
        OS_RWLOCK_WRITE_LOCK(&ni->ni_ic->ic_nodelock,&lock_state);
        TAILQ_REMOVE(&ni->ni_ic->ic_nodes, ni, ni_alloc_list);
        OS_RWLOCK_WRITE_UNLOCK(&ni->ni_ic->ic_nodelock,&lock_state);
    } while(0);
#endif


    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                   "%s: vap=0x%x, peercount=%d, ni=0x%x, ni_bss_node=0x%x \n",__func__,
                   vap, wlan_vdev_get_peer_count(vap->vdev_obj), ni, ni->ni_bss_node);

    IEEE80211_NODE_STATE_LOCK_DESTROY(ni);

#ifdef ATH_SUPPORT_TxBF
    if ( ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf){
        OS_CANCEL_TIMER(&(ni->ni_cv_timer));
        OS_FREE_TIMER(&(ni->ni_cv_timer));
        OS_CANCEL_TIMER(&(ni->ni_report_timer));
        OS_FREE_TIMER(&(ni->ni_report_timer));
        ni->ni_txbf_timer_initialized = 0;

        /* clear TxBF mode active indicator*/
        ni->ni_explicit_compbf = 0;
        ni->ni_explicit_noncompbf = 0;
        ni->ni_implicit_bf = 0;
    }
#endif
    if((ni == vap->iv_bss) && (wlan_vdev_get_peer_count(vap->vdev_obj) > 1)) {
        qdf_nofl_info("All nodes should be freed before bss node gets freed."
                      " peer count is %d Investigate!!!!\n",
                      wlan_vdev_get_peer_count(vap->vdev_obj));
        /* Flushing HW queue to avoid bss node reference leak */
        if (ic->ic_tx_flush)
            ic->ic_tx_flush(ic);
#if IEEE80211_DEBUG_NODELEAK
        wlan_debug_dump_nodes_tgt();
#endif
    }

#if WLAN_SUPPORT_GREEN_AP
    if ((ic->ic_opmode == IEEE80211_M_HOSTAP) && (ni != ni->ni_bss_node) && (ni->is_assoc_resp_completed)) {
        wlan_green_ap_del_sta(ic->ic_pdev_obj);

        if(wlan_node_get_max_nss(ni) > 1)
            wlan_green_ap_del_multistream_sta(ic->ic_pdev_obj);
    }
#endif

#ifdef WLAN_FEATURE_11BE
    if (ni->ni_asresp_t2lmie) {
        qdf_mem_free(ni->ni_asresp_t2lmie);
        ni->ni_asresp_t2lmie = NULL;
        ni->ni_asresp_t2lmie_len = 0;
    }
#endif /* WLAN_FEATURE_11BE */

    /* Check if the mode is HOSTAP, in this case, decrement the node count */
    if (wlan_peer_get_peer_type(ni->peer_obj) != WLAN_PEER_MLO_TEMP) {
        if (ni != ni_bss_node) {
            ieee80211_free_node(ni_bss_node, WLAN_MLME_OBJMGR_ID);
        } else {
            if (vap->iv_opmode == IEEE80211_M_STA) {
                IEEE80211_DELIVER_EVENT_BSS_NODE_FREED(vap);
            }
        }
    }
    ni->ni_ic->ic_node_free(ni);
}

/*
 * Free a node. It is mostly used for decrementing
 * node reference count of an active ap or an associated station.
 * If this is last reference of the node (refcnt reaches 0),
 * free the memory.
 */
void
#if WLAN_OBJMGR_REF_ID_TRACE
ieee80211_free_node_debug(struct ieee80211_node *ni, wlan_objmgr_ref_dbgid id,
        const char *func, int line, const char *file)
{
      wlan_objmgr_free_node_debug(ni, id, func, line);
}
#else
ieee80211_free_node(struct ieee80211_node *ni, wlan_objmgr_ref_dbgid id)
{
      wlan_objmgr_free_node(ni, id);
}
#endif

/*
 * Reclaim a node. It is mostly used when a node leaves the network.
 * remove it from the node table and decrement the held reference..
 * It must be called with OS_WRITE_LOCK being held.
 */
static void
#if WLAN_OBJMGR_REF_ID_TRACE
_node_reclaim(struct ieee80211_node_table *nt, struct ieee80211_node *ni,
             const char *func, int line, const char *file)
#else
node_reclaim(struct ieee80211_node_table *nt, struct ieee80211_node *ni)
#endif
{
    if (ni->ni_table == NULL ) {
        return;
    }
    ASSERT(ieee80211_node_refcnt(ni));
    if (ieee80211_node_refcnt(ni) == 0) {
        ieee80211_note(ni->ni_vap, IEEE80211_MSG_NODE,
            "node_reclaim called with 0 refcount for %s, vap: 0x%pK \n",
            ((ni == ni->ni_vap->iv_bss) ? "BSS NODE" : "NON BSS NODE"), ni->ni_vap);
    }

#ifdef UMAC_SUPPORT_PROXY_ARP
    ieee80211_node_remove_ipv4(nt, ni);
    ieee80211_node_remove_ipv6_by_node(nt, ni);
#endif
    ni->ni_table = NULL;    /* clear reference */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID); /* decrement the ref count */
}

struct wlan_objmgr_peer *ieee80211_lookup_peer_by_mac(struct ieee80211com  *ic,
                                                    uint8_t *macaddr)
{
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;
    uint8_t pdev_id;

    if (!ic)
        return NULL;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_err("pdev is NULL");
        return NULL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if(psoc == NULL) {
        qdf_err("psoc is NULL");
        return NULL;
    }

    pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
    /* Look-up only on pdev for non HKV1 */
    if (ic->ic_get_tgt_type(ic) != TARGET_TYPE_QCA8074)
        return wlan_objmgr_get_peer(psoc, pdev_id, macaddr, WLAN_MGMT_HANDLER_ID);


    /* Check if this node is present on other VAP across SoC for HKV1.
     * Peer with same mac address cannot be created across SoC. Do a lookup
     * on SoC and delete peer if present on other radio.
     */
    return wlan_objmgr_get_peer_by_mac(psoc, macaddr, WLAN_MGMT_HANDLER_ID);
}

struct ieee80211_node*
find_logically_deleted_node_pdev_psoc(struct ieee80211com  *ic,
                                      uint8_t *macaddr,
                                      wlan_objmgr_ref_dbgid id)
{
    struct wlan_objmgr_pdev *pdev;
    struct wlan_objmgr_psoc *psoc;

    if (!ic)
        return NULL;

    pdev = ic->ic_pdev_obj;
    if(pdev == NULL) {
        qdf_err("pdev is NULL");
        return NULL;
    }

    psoc = wlan_pdev_get_psoc(pdev);
    if(psoc == NULL) {
        qdf_err("psoc is NULL");
        return NULL;
    }

    /* Look-up per pdev for non HKV1 */
    if (ic->ic_get_tgt_type(ic) != TARGET_TYPE_QCA8074) {
        return _ieee80211_find_logically_deleted_node(ic,
                                                      macaddr,
                                                      NULL,
                                                      id);
    }

    /* Check if this node is present across SoC for HKV1.
     * Peer with same mac address cannot be created across SoC.
     */
    return find_logically_deleted_node_on_soc(psoc, macaddr, NULL, id);
}

static void ieee80211_free_logically_deleted_peer_list(qdf_list_t *logical_del_peer_list)
{
    struct wlan_logically_del_peer *temp_peer = NULL;
    qdf_list_t *head;
    qdf_list_node_t *peerlist;

    if (!logical_del_peer_list) {
        QDF_TRACE(QDF_MODULE_ID_OBJ_MGR, QDF_TRACE_LEVEL_ERROR,
                        "Logically deleted peer list is NULL");
        return;
    }

    head = logical_del_peer_list;

    while (QDF_IS_STATUS_SUCCESS(qdf_list_remove_front(head, &peerlist))) {
        temp_peer = qdf_container_of(peerlist, struct wlan_logically_del_peer, list);
        wlan_objmgr_peer_release_ref(temp_peer->peer, WLAN_MLME_SB_ID);
        qdf_mem_free(temp_peer);
    }
    qdf_list_destroy(head);
    qdf_mem_free(head);
}

struct ieee80211_node *
#if WLAN_OBJMGR_REF_ID_TRACE
_ieee80211_find_logically_deleted_node_debug(struct ieee80211com *ic,
        const u_int8_t *macaddr, const u_int8_t *bssid, wlan_objmgr_ref_dbgid id,
        const char *func, int line, const char *file)
#else
_ieee80211_find_logically_deleted_node(struct ieee80211com *ic,
        const u_int8_t *macaddr, const u_int8_t *bssid, wlan_objmgr_ref_dbgid id)
#endif
{
    qdf_list_t *logical_del_peer_list = NULL;
    struct ieee80211_node *ni = NULL;
    qdf_list_node_t *peerlist = NULL;
    qdf_list_node_t *peerlist_temp = NULL;
    struct wlan_logically_del_peer *del_list = NULL;

#if WLAN_OBJMGR_REF_ID_TRACE
    logical_del_peer_list = wlan_objmgr_populate_logically_deleted_node_list_debug(ic,
            (uint8_t *)macaddr, (uint8_t *)bssid, WLAN_MLME_SB_ID, func, line);
#else
    logical_del_peer_list = wlan_objmgr_populate_logically_deleted_node_list(ic,
            (uint8_t *)macaddr, (uint8_t *)bssid, WLAN_MLME_SB_ID);
#endif

    if (!logical_del_peer_list) {
        return NULL;
    }

    if (QDF_IS_STATUS_ERROR(
                qdf_list_peek_front(logical_del_peer_list, &peerlist))) {
        return NULL;
    }

    do {
        del_list = qdf_container_of(peerlist, struct wlan_logically_del_peer, list);
        ni = (wlan_node_t)wlan_peer_get_mlme_ext_obj(del_list->peer);
        if (ni && qdf_atomic_read(&ni->ni_fw_peer_delete_rsp_pending)) {
            ieee80211_ref_node(ni, id);
            ieee80211_free_logically_deleted_peer_list(logical_del_peer_list);
            return ni;
        }
        peerlist_temp = peerlist;
    } while (QDF_IS_STATUS_SUCCESS(
                qdf_list_peek_next(logical_del_peer_list, peerlist_temp, &peerlist)));

    ieee80211_free_logically_deleted_peer_list(logical_del_peer_list);
    return NULL;
}

struct ieee80211_node *
#if WLAN_OBJMGR_REF_ID_TRACE
_ieee80211_find_node_debug(struct ieee80211com *ic, const u_int8_t *macaddr,
                     wlan_objmgr_ref_dbgid id, const char *func, int line, const char *file)
{
    return wlan_objmgr_find_node_debug(ic,(uint8_t *)macaddr, id, func, line);
}
#else
_ieee80211_find_node(struct ieee80211com *ic, const u_int8_t *macaddr,
        wlan_objmgr_ref_dbgid id)
{
    return wlan_objmgr_find_node(ic,(uint8_t *)macaddr, id);
}
#endif


struct ieee80211_node *
#if WLAN_OBJMGR_REF_ID_TRACE
ieee80211_find_node_debug(struct ieee80211com *ic, const u_int8_t *macaddr,
                          wlan_objmgr_ref_dbgid id, const char *func, int line, const char *file)
#else
ieee80211_find_node(struct ieee80211com *ic, const u_int8_t *macaddr, wlan_objmgr_ref_dbgid id)
#endif
{
    struct ieee80211_node *ni;

#if WLAN_OBJMGR_REF_ID_TRACE
    ni = _ieee80211_find_node_debug(ic, macaddr, id, func, line, file);
#else
    ni = _ieee80211_find_node(ic, macaddr, id);
#endif

    return ni;
}

#ifdef WLAN_FEATURE_11BE_MLO
struct ieee80211_node *
ieee80211_find_node_by_mld_mac(struct ieee80211vap *vap, const u_int8_t *mld_mac,
                               wlan_objmgr_ref_dbgid id)
{
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_vdev *vdev = NULL;
    struct ieee80211_node *ni;
    struct qdf_mac_addr mac;
    QDF_STATUS status;
    uint8_t i;

    if (!vap->vdev_obj->mlo_dev_ctx)
        return NULL;

    if (!mld_mac)
        return NULL;

    qdf_mem_copy(mac.bytes, mld_mac, QDF_MAC_ADDR_SIZE);
    mlo_peer_ctx = wlan_mlo_get_mlpeer(vap->vdev_obj->mlo_dev_ctx, &mac);

    /* This is not MLO peer */
    if (!mlo_peer_ctx)
        return NULL;

    for (i = 0; i < mlo_peer_ctx->link_peer_cnt; i++) {
        peer_entry = &mlo_peer_ctx->peer_list[i];

        if (!peer_entry->link_peer)
            continue;
        status = wlan_objmgr_peer_try_get_ref(peer_entry->link_peer, id);

        if (QDF_IS_STATUS_ERROR(status))
            continue;

        vdev = wlan_peer_get_vdev(peer_entry->link_peer);
        if (vdev != vap->vdev_obj) {
            wlan_objmgr_peer_release_ref(peer_entry->link_peer, id);
            continue;
        }

        ni = wlan_peer_get_mlme_ext_obj(peer_entry->link_peer);
        if (!ni) {
            wlan_objmgr_peer_release_ref(peer_entry->link_peer, id);
            continue;
        }

        return ni;
    }

    return NULL;
}

struct ieee80211_node *
ieee80211_find_node_by_mld_mac_and_pdev(struct ieee80211com *ic,
                    const u_int8_t *mld_mac, wlan_objmgr_ref_dbgid id)
{
    struct wlan_mlo_dev_context *mld_ctx = NULL;
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct wlan_objmgr_pdev *pdev = NULL;
    struct ieee80211_node *ni;
    uint8_t i;
    QDF_STATUS status;

    if (!ic)
        return NULL;

    if (!mld_mac)
        return NULL;

    mlo_peer_ctx = wlan_mlo_get_mlpeer_by_peer_mladdr((struct qdf_mac_addr *)mld_mac, &mld_ctx);
    if (!mlo_peer_ctx || !mld_ctx) {
        mlo_debug("ML peer not found for MLD mac " QDF_MAC_ADDR_FMT,
                  QDF_MAC_ADDR_REF(mld_mac));
        return NULL;
    }

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
        peer_entry = &mlo_peer_ctx->peer_list[i];

        if (!peer_entry->link_peer)
            continue;
        status = wlan_objmgr_peer_try_get_ref(peer_entry->link_peer, id);

        if (QDF_IS_STATUS_ERROR(status))
            continue;

        pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer_entry->link_peer));
        if (pdev != ic->ic_pdev_obj) {
            wlan_objmgr_peer_release_ref(peer_entry->link_peer, id);
            continue;
        }

        ni = wlan_peer_get_mlme_ext_obj(peer_entry->link_peer);
        if (!ni) {
            wlan_objmgr_peer_release_ref(peer_entry->link_peer, id);
            continue;
        }
        return ni;
    }

    return NULL;
}
#endif

/*
 * Return a reference to the appropriate node for sending
 * a data frame.  This handles node discovery in adhoc networks.
 */
struct ieee80211_node *
#if WLAN_OBJMGR_REF_ID_TRACE
ieee80211_find_txnode_debug(struct ieee80211vap *vap, const u_int8_t *macaddr,
                            wlan_objmgr_ref_dbgid id, const char *func, int line, const char *file)
#else
ieee80211_find_txnode(struct ieee80211vap *vap, const u_int8_t *macaddr, wlan_objmgr_ref_dbgid id)
#endif
{
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;
    struct ieee80211_node *ni = NULL;
    rwlock_state_t lock_state;

    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);

    if (vap->iv_bss) {
        if (vap->iv_opmode == IEEE80211_M_STA ||
            vap->iv_opmode == IEEE80211_M_WDS) {
#if WLAN_OBJMGR_REF_ID_TRACE
            ni = ieee80211_try_ref_node_debug(vap->iv_bss, id, func, line, file);
#else
            ni = ieee80211_try_ref_node(vap->iv_bss, id);
#endif
        } else if (IEEE80211_IS_MULTICAST(macaddr)) {
            if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                if (vap->iv_sta_assoc > 0) {
#if WLAN_OBJMGR_REF_ID_TRACE
                    ni = ieee80211_try_ref_node_debug(vap->iv_bss, id, func, line, file);
#else
                    ni = ieee80211_try_ref_node(vap->iv_bss, id);
#endif
                } else {
                    /* No station associated to AP */
#ifdef QCA_SUPPORT_CP_STATS
                    vdev_cp_stats_tx_nonode_inc(vap->vdev_obj, 1);
#endif
                    ni = NULL;
                }
            } else {
                ni = ieee80211_try_ref_node(vap->iv_bss, id);
            }
        } else {
            ni = _ieee80211_find_node(vap->iv_ic, macaddr, id);
            if (ni == NULL) {
                if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
                     wlan_rptr_vdev_is_wds(vap->vdev_obj)) {
                    ni = ieee80211_find_wds_node(nt, macaddr, id);
                }
            }
        }
    }
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    /*
     * Since all vaps share the same node table, we may find someone else's
     * node (sigh!).
     */
    if (ni && ni->ni_vap != vap) {
        ieee80211_unref_node(&ni, id);
        return NULL;
    }
    return ni;
}

#if WLAN_OBJMGR_REF_ID_TRACE
struct ieee80211_node *
ieee80211_find_rxnode_debug(struct ieee80211com *ic,
                            const struct ieee80211_frame_min *wh, wlan_objmgr_ref_dbgid id,
                            const char *func, int line, const char *file)
#else
struct ieee80211_node *
ieee80211_find_rxnode(struct ieee80211com *ic,
                      const struct ieee80211_frame_min *wh, wlan_objmgr_ref_dbgid id)
#endif
{
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);
#if WLAN_OBJMGR_REF_ID_TRACE
    ni = ieee80211_find_rxnode_nolock_debug(ic, wh, id, func, line, file);
#else
    ni = ieee80211_find_rxnode_nolock(ic, wh, id);
#endif
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    return ni;
}

#if ATH_SUPPORT_WRAP
static struct ieee80211_node *
#if WLAN_OBJMGR_REF_ID_TRACE
_wrap_find_rxnode_debug(struct ieee80211com *ic,
                        const uint8_t ra[QDF_MAC_ADDR_SIZE],
                        const uint8_t ta[QDF_MAC_ADDR_SIZE], wlan_objmgr_ref_dbgid id,
                        const char *func, int line, const char *file)
#else
_wrap_find_rxnode(struct ieee80211com *ic,
                  const uint8_t ra[QDF_MAC_ADDR_SIZE],
                  const uint8_t ta[QDF_MAC_ADDR_SIZE],
                  wlan_objmgr_ref_dbgid id)
#endif
{
    struct wlan_objmgr_vdev *vdev = NULL;
    struct wlan_objmgr_peer *peer;
    struct wlan_objmgr_psoc *psoc;
    struct ieee80211_node *ni = NULL;
    uint8_t pdev_id;
    wlan_if_t vap = NULL;

    if (ic && ic->ic_pdev_obj) {
        pdev_id = wlan_objmgr_pdev_get_pdev_id(ic->ic_pdev_obj);
    }
    else {
        return NULL;
    }

    psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
    if (!psoc)
        return NULL;

    if (wlan_rptr_pdev_is_qwrap(ic->ic_pdev_obj)) {
        vdev = wlan_objmgr_get_vdev_by_macaddr_from_psoc(psoc,
                pdev_id, (uint8_t *)ra,
                id);

        if (vdev == NULL) {
            return NULL;
        }

        if (wlan_vdev_mlme_get_opmode(vdev) == QDF_STA_MODE) {

            peer = wlan_vdev_get_bsspeer(vdev);
            if (peer == NULL) {
                wlan_objmgr_vdev_release_ref(vdev, id);
                return NULL;
            }

            vap = wlan_vdev_get_vap(vdev);

            /* Donot return ni when bss peer address matches with the vdev
             * address. This means that the peer retrieved is a self peer and
             * that should not be returned.
             */
            if(!vap || IEEE80211_ADDR_EQ(peer->macaddr,vap->iv_myaddr)) {
                wlan_objmgr_vdev_release_ref(vdev, id);
                return NULL;
            }

            if (QDF_STATUS_SUCCESS == wlan_objmgr_peer_try_get_ref(peer, id)) {
                ni = wlan_peer_get_mlme_ext_obj(peer);

                if (ni == NULL) {
                    wlan_objmgr_peer_release_ref(peer, id);
                    wlan_objmgr_vdev_release_ref(vdev, id);
                    return NULL;
                }
            }
        } else {
            ni = _ieee80211_find_node(ic, (uint8_t *)ta, id);
        }

        wlan_objmgr_vdev_release_ref(vdev, id);
    } else {
        ni = _ieee80211_find_node(ic, (uint8_t *)ta, id);
    }

    return ni;

}

struct ieee80211_node *
#if WLAN_OBJMGR_REF_ID_TRACE
ieee80211_find_wrap_node_debug(struct ieee80211vap *vap, const u_int8_t *macaddr,
                               wlan_objmgr_ref_dbgid id, const char *func, int line, const char *file)
#else
ieee80211_find_wrap_node(struct ieee80211vap *vap, const u_int8_t *macaddr, wlan_objmgr_ref_dbgid id)
#endif
{
    struct ieee80211_node *ni = NULL;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node_table *nt = &ic->ic_sta;
#ifndef WLAN_OBJMGR_REF_ID_TRACE
    struct wlan_objmgr_peer *peer;
#endif

    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);
#if WLAN_OBJMGR_REF_ID_TRACE
    ni = _wrap_find_rxnode_debug(ic, vap->iv_myaddr, macaddr, id, func, line, file);
#else
    if(wlan_rptr_pdev_is_qwrap(ic->ic_pdev_obj) &&
       (vap->iv_opmode == IEEE80211_M_STA)) {

        peer = wlan_vdev_get_bsspeer(vap->vdev_obj);

        if (peer == NULL)
            goto ieee80211_find_wrap_node_fail;

        /* Donot return ni when bss peer address matches with the vdev
         * address. This means that the peer retrieved is a self peer and
         * that should not be returned.
         */
        if(IEEE80211_ADDR_EQ(peer->macaddr,vap->iv_myaddr))
            goto ieee80211_find_wrap_node_fail;

        if (QDF_STATUS_SUCCESS == wlan_objmgr_peer_try_get_ref(peer, id)) {
            ni = wlan_peer_get_mlme_ext_obj(peer);
            if (ni == NULL) {
                qdf_print("ni is null <%s %d>",__func__,__LINE__);
                wlan_objmgr_peer_release_ref(peer, id);
            }
        } else {
            qdf_info("Unable to take ref for peer %pM", macaddr);
        }
    }
    else {
      ni = _ieee80211_find_node(ic, macaddr, id);
    }

ieee80211_find_wrap_node_fail:
#endif
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    return ni;

}
#endif

#if !WLAN_OBJMGR_REF_ID_TRACE
struct ieee80211_node *
ieee80211_find_rxnode_nolock(struct ieee80211com *ic,
                      const struct ieee80211_frame_min *wh, wlan_objmgr_ref_dbgid id)
#else
struct ieee80211_node *
ieee80211_find_rxnode_nolock_debug(struct ieee80211com *ic,
                             const struct ieee80211_frame_min *wh, wlan_objmgr_ref_dbgid id,
                             const char *func, int line, const char *file)
#endif
{
#define	IS_CTL(wh)  \
    ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_CTL)
#define	IS_PSPOLL(wh)   \
    ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PS_POLL)
#define	IS_BAR(wh) \
    ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_BAR)

    if (!wh) {
        qdf_debug("wbuf header is null");
        return NULL;
    }

    if (IS_CTL(wh) && !IS_PSPOLL(wh) && !IS_BAR(wh))
        return _ieee80211_find_node(ic, wh->i_addr1, id);

#if ATH_SUPPORT_WRAP
#if WLAN_OBJMGR_REF_ID_TRACE
    return _wrap_find_rxnode_debug(ic, wh->i_addr1, wh->i_addr2, id, func, line, file);
#else
    return _wrap_find_rxnode(ic, wh->i_addr1, wh->i_addr2, id);
#endif
#else
    return _ieee80211_find_node(ic, wh->i_addr2, id);
#endif
#undef IS_BAR
#undef IS_PSPOLL
#undef IS_CTL
}

#if WLAN_OBJMGR_REF_ID_TRACE
struct ieee80211_node *
ieee80211_ref_node_debug(struct ieee80211_node *ni, wlan_objmgr_ref_dbgid id,
                          const char *func, int line, const char *file)
{
    wlan_objmgr_ref_node_debug(ni, id, func, line);
    return ni;
}
qdf_export_symbol(ieee80211_ref_node_debug);

struct ieee80211_node *
ieee80211_try_ref_node_debug(struct ieee80211_node *ni, wlan_objmgr_ref_dbgid id,
                          const char *func, int line, const char *file)
{
    if (wlan_objmgr_try_ref_node_debug(ni, id, func, line) != QDF_STATUS_SUCCESS)
        return NULL;

    return ni;
}
qdf_export_symbol(ieee80211_try_ref_node_debug);

void
ieee80211_unref_node_debug(struct ieee80211_node **ni, wlan_objmgr_ref_dbgid id,
                          const char *func, int line, const char *file)
{
    wlan_objmgr_unref_node_debug(*ni, id, func, line);
    *ni = NULL;			/* guard against use */
}
qdf_export_symbol(ieee80211_unref_node_debug);
#endif

static void ieee80211_bring_up_ap_vaps(struct wlan_objmgr_vdev *vdev, void *arg)
{
    struct ieee80211com *ic;
    struct ieee80211vap *vap;

    vap = wlan_vdev_mlme_get_ext_hdl(vdev);
    if (!vap)
        return;

    ic = vap->iv_ic;
    ieee80211_bringup_ap_vaps(ic);
}

#ifdef WLAN_FEATURE_11BE_MLO
static void ieee80211_bring_up_standalone_ap_vaps(struct wlan_objmgr_vdev *vdev,
                                                  void *arg)
{
    struct wlan_objmgr_pdev *pdev = NULL;
    struct wlan_objmgr_vdev *sta_vdev = NULL;

    pdev = wlan_vdev_get_pdev(vdev);
    sta_vdev = wlan_rptr_pdev_get_stavdev(pdev);
    if (!sta_vdev)
        wlan_mbss_standalone_sched_start_stop(vdev,
                                          MBSS_SCHED_STANDALONE_AP_VDEVS_START);
}

static inline
void ieee80211_bringup_ml_ap_vaps(struct wlan_objmgr_vdev *vdev)
{
    mlo_iterate_connected_vdev_list(vdev,
                                    ieee80211_bring_up_ap_vaps,
                                    NULL);
    mlo_iterate_ml_standalone_vdev_list(vdev,
                                        ieee80211_bring_up_standalone_ap_vaps,
                                        NULL);
}
#else
static inline
void ieee80211_bringup_ml_ap_vaps(struct wlan_objmgr_vdev *vdev)
{ }
#endif

void
ieee80211_node_authorize(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    u_int8_t *macaddr;
#if defined(IPA_OFFLOAD) && defined(QCA_SUPPORT_WDS_EXTENDED)
    ol_txrx_soc_handle soc_txrx_handle;
    struct wlan_objmgr_psoc *psoc;
    struct ieee80211vap *primary_vap = NULL;
    struct wlan_objmgr_peer *primary_peer = NULL;
    struct ieee80211_node *primary_ni = NULL;
#endif
#ifdef WLAN_FEATURE_11BE
    int i = 0;
    int tid = 0;
    struct wlan_mlo_peer_context *ml_peer;
    struct wlan_t2lm_onging_negotiation_info *t2lm_info = NULL;
    bool assoc_with_t2lm = false;
#endif
#ifdef WLAN_FEATURE_11BE_MLO
    uint8_t idx = 0;
    struct wlan_mlo_link_peer_entry *peer_entry;
#endif

    if (!(ni->ni_flags & IEEE80211_NODE_AUTH)) {
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_authorize_success_inc(vap->vdev_obj, 1);
#endif
#ifdef WLAN_FEATURE_11BE
        if (ni->ni_rm_app_assoc_status != RM_ASSOC_SENT &&
            vap->iv_opmode == IEEE80211_M_HOSTAP) {

            ni->ni_rm_app_assoc_status = RM_ASSOC_SENT;

            if (wlan_peer_is_mlo(ni->peer_obj) &&
                wlan_peer_mlme_is_assoc_peer(ni->peer_obj)) {

                ml_peer = ni->peer_obj->mlo_peer_ctx;
                t2lm_info = &ml_peer->t2lm_policy.ongoing_tid_to_link_mapping;

                if (ieee80211_is_t2lm_supported(t2lm_info, ni->ni_t2lm_enable_val)) {

                    /* Assoc with t2lm info should be sent to RM APP if there was a
                     * t2lm ie in the Assoc request frame. Else, send Assoc with no
                     * t2lm info.
                     */
                    for (i = 0; i < WLAN_T2LM_MAX_DIRECTION; i++) {
                        if (t2lm_info->t2lm_info[i].direction ==
                                WLAN_T2LM_INVALID_DIRECTION)
                            continue;

                        if (!t2lm_info->t2lm_info[i].default_link_mapping)
                            assoc_with_t2lm = true;

                        t2lm_debug("dir:%d default_mapping:%d",
                                   t2lm_info->t2lm_info[i].direction,
                                   t2lm_info->t2lm_info[i].default_link_mapping);
                        for (tid = 0; tid < T2LM_MAX_NUM_TIDS; tid++) {
                            t2lm_debug("tid:%d ieee_link_map_tid:%d hw_link_map_tid:%d",
                                       tid,
                                       t2lm_info->t2lm_info[i].ieee_link_map_tid[tid],
                                       t2lm_info->t2lm_info[i].hw_link_map_tid[tid]);
                        }
                    }
                }
            }

            if (assoc_with_t2lm)
                    wlan_mlme_send_assoc_event(ni->peer_obj, true, t2lm_info,
                            QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_WITH_T2LM_INFO);
            else
                    wlan_mlme_send_assoc_event(ni->peer_obj, true, NULL,
                            QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_ASSOC_NO_T2LM_INFO);

            /* Reset the onging mapping */
            if (t2lm_info) {
                for (i = 0; i < WLAN_T2LM_MAX_DIRECTION; i++) {
                    qdf_mem_zero(&t2lm_info->t2lm_info[i], sizeof(struct wlan_t2lm_info));
                    t2lm_info->t2lm_info[i].direction = WLAN_T2LM_INVALID_DIRECTION;
                }
            }
        }

#endif /* WLAN_FEATURE_11BE */
    }

    ieee80211node_set_flag(ni, IEEE80211_NODE_AUTH);
    ni->ni_inact_reload =
            vap->vdev_mlme->mgmt.inactivity_params.keepalive_max_unresponsive_time_secs;
    ni->ni_last_authorize_rst_time = 0;

    switch(vap->iv_opmode) {
        case IEEE80211_M_STA:
            macaddr = ni->ni_bssid;
            break;
        default:
            macaddr = ni->ni_macaddr;
            break;
    }

    /* Deliver node authorize event */
    IEEE80211_DELIVER_EVENT_MLME_NODE_AUTHORIZED_INDICATION(vap, macaddr);

    if (ni->ni_inact > ni->ni_inact_reload)
        ni->ni_inact = ni->ni_inact_reload;

    if (ic->ic_node_authorize) {
        int32_t key_mgmt;
        key_mgmt = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_KEY_MGMT);
        if ( key_mgmt == -1 ) {
            qdf_err("crypto_err while getting key_mgmt params\n");
            return;
        }

        if (key_mgmt & (1 << WLAN_CRYPTO_KEY_MGMT_OWE))
            ic->ic_node_authorize(ni,3);
        else
        ic->ic_node_authorize(ni,TRUE);
    }

#if QCA_AIRTIME_FAIRNESS
    if ((vap->iv_opmode == IEEE80211_M_HOSTAP) ||
        (vap->iv_opmode == IEEE80211_M_STA))
        wlan_atf_peer_join_leave(ni->peer_obj, 1);
#endif
#if DBDC_REPEATER_SUPPORT
    if ((vap->iv_opmode == IEEE80211_M_STA) &&
        ieee80211_ic_rpt_max_phy_is_set(ic) &&
        !wlan_rptr_pdev_is_eir(ic->ic_pdev_obj)) {
        qdf_debug("[rpt_max_phy]: Node authorize received, bring ap vaps up");
        if (wlan_vdev_mlme_is_mlo_vdev(vap->vdev_obj)) {
            if (ucfg_mlo_is_mld_connected(vap->vdev_obj)) {
                ieee80211_bringup_ml_ap_vaps(vap->vdev_obj);
            }
        } else {
           ieee80211_bringup_ap_vaps(ic);
        }
    }
#endif
    wlan_node_set_peer_state(ni, WLAN_CONNECTED_STATE);

#ifdef WLAN_DBS_POWER_REDUCTION
    if (ic->ic_update_active_count)
        ic->ic_update_active_count(ic);
#endif

#if defined(IPA_OFFLOAD) && defined(QCA_SUPPORT_WDS_EXTENDED)
    if (ni->ni_ext_flags & IEEE80211_PEER_MESH_NODE) {
        if (wlan_peer_is_mlo(ni->peer_obj)) {
            primary_peer = wlan_mlo_peer_get_primary_peer(
                                                    ni->peer_obj->mlo_peer_ctx);
            if (!primary_peer) {
                qdf_err("Primary peer is NULL");
                return;
            }
            primary_ni = wlan_peer_get_mlme_ext_obj(primary_peer);
            if (!primary_ni) {
                qdf_err("primary ni is NULL");
                return;
            }
            primary_vap = primary_ni->ni_vap;
            psoc = wlan_vdev_get_psoc(primary_vap->vdev_obj);
        }
        else
            psoc = wlan_vdev_get_psoc(vap->vdev_obj);

        if (!psoc) {
            qdf_err("psoc is null");
            return;
        }
        soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
        if (!soc_txrx_handle) {
            qdf_err("soc_txrx_handle is null");
            return;
        }
        if (primary_ni) {
            cdp_wds_ext_set_bit(soc_txrx_handle,primary_ni->ni_macaddr);
            wlan_cfg80211_wds_ext_peer_learn(primary_vap,
                                             primary_ni->ni_macaddr);
        }
        else {
            cdp_wds_ext_set_bit(soc_txrx_handle, macaddr);
            wlan_cfg80211_wds_ext_peer_learn(vap, macaddr);
       }
    }
#endif
    /* start session timeout */
    ni->ni_session = vap->iv_session;
#ifdef QCA_SUPPORT_PRIMARY_LINK_MIGRATE
    if (!wlan_peer_is_mlo(ni->peer_obj) || !ni->peer_obj->mlo_peer_ctx)
        return;

    if (wlan_vdev_read_skip_pumac_cnt(vap->vdev_obj) == 0)
        return;

    for (idx = 0; idx < MAX_MLO_LINK_PEERS; idx++) {
         peer_entry = &ni->peer_obj->mlo_peer_ctx->peer_list[idx];
         if (!peer_entry || !peer_entry->link_peer)
             continue;

         if (peer_entry->is_primary && peer_entry->link_peer == ni->peer_obj) {
             wlan_mlo_set_ptqm_migration(vap->vdev_obj, ni->peer_obj->mlo_peer_ctx,
                                         false, WLAN_LINK_ID_INVALID, true);
             break;
         }
    }
#endif
}

void
ieee80211_node_unauthorize(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = NULL;

    ieee80211node_clear_flag(ni, IEEE80211_NODE_AUTH);
    ni->ni_inact_reload = ni->ni_vap->iv_inact_auth;
    ni->ni_last_authorize_rst_time = OS_GET_TIMESTAMP();
    if (ni->ni_inact > ni->ni_inact_reload)
        ni->ni_inact = ni->ni_inact_reload;

    ic = ni->ni_ic;

    if (ic == NULL) {
        qdf_nofl_info ("%s:WARN ni->ni_ic is NULL for ni(%pK) ni_macddr %s\n",
                __func__, ni, ether_sprintf(ni->ni_macaddr));
    }
    else if(ic->ic_node_authorize) {
        ic->ic_node_authorize(ni,FALSE);
    }

    wlan_node_set_peer_state(ni, WLAN_DISCONNECT_STATE);
    /* disable session timeout */
    ni->ni_session = IEEE80211_SESSION_TIME;
}

static
void ieee80211_noassoc_sta_timeout_iter_cb(void *arg, struct ieee80211_node *ni)
{
    systime_t now;
    struct ieee80211vap * tmp_vap = ni->ni_vap;

    /* Do not delete the PASN peer from Host side. FW sends an PASN peer delete
     * event to host when PASN peers are exceeds the limit.
     */
    if (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_RTT_PASN)
        return;

    /* Skip the node if delayed cleanup flag or leave ongoing flag is set.
     */
    if (ieee80211node_has_flag(ni, IEEE80211_NODE_DELAYED_CLEANUP) ||
        ieee80211node_has_flag(ni, IEEE80211_NODE_LEAVE_ONGOING)) {
        return;
    }

    /*
     * Special case ourself; we may be idle for extended periods
     * of time and regardless reclaiming our state is wrong.
     */
    if (ni == ni->ni_vap->iv_bss) {
        /* don't permit it to go negative */
        if (ni->ni_inact > 0)
            ni->ni_inact--;
        return;
    }


    if (ni->ni_inact > 0)
        ni->ni_inact--;

#if UMAC_SUPPORT_NAWDS
    /* Never deauth the timeout NAWDS station.
     * But keep checking if it's still inactive.
     */
    if (ni->ni_flags & IEEE80211_NODE_NAWDS && ni->ni_inact <= 0) {
        ni->ni_inact = 1;
        return;
    }
#endif

    /*
     * Make sure to timeout STAs who have sent 802.11
     * authentication but not have associated.
     */
    if (ni->ni_inact <= 0) {
        /*  Send a deauthenticate frame and drop the station */
        if (ni->ni_vap->iv_opmode == IEEE80211_M_IBSS) {
            ieee80211_sta_leave(ni);
        } else if (ni->ni_vap->iv_opmode != IEEE80211_M_STA) {
            now = OS_GET_TIMESTAMP();
           if (ni->ni_last_auth_rx_time != 0
               && (CONVERT_SYSTEM_TIME_TO_MS(now - ni->ni_last_auth_rx_time) >
               IEEE80211_STA_NOASSOC_TIME)) {
               IEEE80211_RL_NOTE(tmp_vap, IEEE80211_MSG_INACT, ni,
                   "station timed out due to inactivity (refcnt %u) ni_macaddr:%s \n",
                   ieee80211_node_refcnt(ni),ether_sprintf(ni->ni_macaddr));

               IEEE80211_DPRINTF(tmp_vap, IEEE80211_MSG_AUTH,
                    "%s: sending DEAUTH to %s, timeout stations reason %d\n",
                    __func__, ether_sprintf(ni->ni_macaddr), IEEE80211_REASON_AUTH_EXPIRE);
               wlan_mlme_deauth_request(tmp_vap,ni->ni_macaddr,IEEE80211_REASON_AUTH_EXPIRE);
           }
#ifdef WLAN_FEATURE_11BE_MLO
           else if (ni->peer_obj->mlo_peer_ctx && !ieee80211_node_is_authorized(ni) &&
                      (ni->ni_last_authorize_rst_time != 0 &&
                        (CONVERT_SYSTEM_TIME_TO_MS(now - ni->ni_last_authorize_rst_time) >
                        IEEE80211_STA_NOAUTHORIZE_TIME))) {
               IEEE80211_RL_NOTE(tmp_vap, IEEE80211_MSG_INACT, ni,
                   "ML station(unauthorized) timed out due to inactivity (refcnt %u) ni_macaddr:%s \n",
                   ieee80211_node_refcnt(ni),ether_sprintf(ni->ni_macaddr));

               IEEE80211_DPRINTF(tmp_vap, IEEE80211_MSG_AUTH,
                    "%s: sending DEAUTH to %s, timeout stations reason %d\n",
                    __func__, ether_sprintf(ni->ni_macaddr), IEEE80211_REASON_AUTH_EXPIRE);
               wlan_mlme_deauth_request(tmp_vap,ni->ni_macaddr,IEEE80211_REASON_AUTH_EXPIRE);
           }
#endif
        }
    }
}

/*
 * Periodically check and cleanup nodes allocated for
 * non-associated clients.
 * Free the node if AP not received any ASSOC frames from client
 * after AUTH within the configured mlme timeout STA_NOASSOC_TIME
 */
void ieee80211_noassoc_sta_timeout(struct ieee80211com *ic)
{
    wlan_mlme_iterate_node_list(ic, ieee80211_noassoc_sta_timeout_iter_cb,
                                NULL, IEEE80211_NODE_ITER_F_UNASSOC_STA | IEEE80211_NODE_ITER_F_ASSOC_STA);
}

static
void ieee80211_session_timeout_iter_cb(void *arg, struct ieee80211_node *ni)
{
    /*
     * Special case ourself; we cannot timeout our session.
     */
    if (ni == ni->ni_vap->iv_bss) {
        /* NB: don't permit it to go negative */
        if (ni->ni_session > 0)
            ni->ni_session--;
        return;
    }

    ni->ni_session--;
#if UMAC_SUPPORT_NAWDS
    /* Never timeout the session for NAWDS station. */
    if (ni->ni_flags & IEEE80211_NODE_NAWDS && ni->ni_session <= 0) {
        ni->ni_session = 1;
        return;
    }
#endif

    if (ni->ni_session <= 0) {
        /*
         * For now, just send an event up the stack and let the higer layers handle it.
         */
        IEEE80211_DELIVER_EVENT_SESSION_TIMEOUT(ni->ni_vap, ni->ni_macaddr);
    }
}

/*
 * Timeout stations whose session has expired.
 * For now, just send an event up the stack and let the higer layers handle it.
 */
void
ieee80211_session_timeout(struct ieee80211com *ic)
{

    wlan_mlme_iterate_node_list(ic, ieee80211_session_timeout_iter_cb, NULL,
        (IEEE80211_NODE_ITER_F_ASSOC_STA | IEEE80211_NODE_ITER_F_UNASSOC_STA));
}


#if QCA_SUPPORT_PEER_ISOLATION
void
ieee80211_node_isolation(wlan_if_t vap, int8_t cmd, u_int8_t *macaddr)
{
    struct ieee80211_node *ni;
    struct ieee80211com *ic = vap->iv_ic;

    ni = ieee80211_find_node(ic, macaddr, WLAN_MLME_SB_ID);
    if (!ni)
        return;

    /* The node not associated to current VAP, ignore it */
    if (vap != ni->ni_vap) {
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
        return;
    }

    /* Self node, ignore */
    if (ni == vap->iv_bss) {
        ieee80211_free_node(ni, WLAN_MLME_SB_ID);
        return;
    }

    switch (cmd) {
    case IEEE80211_PEER_ISOLATION_ADD:
        if (IEEE80211_ADDR_EQ((char *)(macaddr), (char *)(ni->ni_macaddr))) {
            QDF_TRACE(QDF_MODULE_ID_NODE, QDF_TRACE_LEVEL_INFO,
                      "Peer isolation enable: %pM", macaddr);
            ic->ic_node_peer_isolation(ni, true);
        }
        break;
    case IEEE80211_PEER_ISOLATION_DEL:
        if (IEEE80211_ADDR_EQ((char *)(macaddr), (char *)(ni->ni_macaddr))) {
            QDF_TRACE(QDF_MODULE_ID_NODE, QDF_TRACE_LEVEL_INFO,
                      "Peer isolation disable: %pM", macaddr);
            ic->ic_node_peer_isolation(ni, false);
        }
        break;
    default:
        break;
    }

    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
}
#endif

void
ieee80211_node_set_chan(struct ieee80211_node *ni)
{
    struct ieee80211_ath_channel *chan = ni->ni_vap->iv_bsschan;

    KASSERT(chan != IEEE80211_CHAN_ANYC, ("bss channel not setup\n"));

    ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(chan),
                                  chan->ic_freq, chan->ic_vhtop_freq_seg1,
                                  chan->ic_vhtop_freq_seg2
#ifdef WLAN_FEATURE_11BE
                                  , chan->op_puncture_bitmap
#endif
                                 );
    ieee80211_init_node_rates(ni, chan);
}

void
ieee80211_node_set_chan_param(struct ieee80211_node *ni,
                              enum ieee80211_phymode phymode,
                              qdf_freq_t chan_freq,
                              qdf_freq_t cfreq1,
                              qdf_freq_t cfreq2
#ifdef WLAN_FEATURE_11BE
                              , uint16_t puncture_bitmap
#endif
                             )
{
    ni->ni_ch_param.ni_bss_phymode = phymode;
    ni->ni_ch_param.ni_freq   =  chan_freq;
    ni->ni_ch_param.ni_cfreq2 =  cfreq2;
    ni->ni_ch_param.ni_cfreq1 =  cfreq1;
#ifdef WLAN_FEATURE_11BE
    ni->ni_ch_param.puncture_bitmap = puncture_bitmap;
#endif
}

/**
* @brief    update new channel, channel width and phy mode after
*           changing channel and width dynamically.
*
* @param arg    opaque pointer to peer list
* @param ni     node which needs to be updated
*
*/
void ieee80211_node_update_chan_and_phymode(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_ath_channel *chan = vap->iv_bsschan;
    struct ieee80211_ath_channel non_11be_chan;
    struct ieee80211com *ic = ni->ni_vap->iv_ic;
    enum ieee80211_cwm_width old_ni_chwidth, max_chwidth = 0; /* node chwidth cannot be greater than IC chwidth */
    uint16_t old_puncture_bitmap = 0;
    struct node_chan_width_switch_params *pi = (struct node_chan_width_switch_params *)arg;
    bool is_node_eht;

#ifdef WLAN_FEATURE_11BE
     is_node_eht = ni->ni_ext_flags & IEEE80211_NODE_EHT;
#else
     is_node_eht = false;
#endif

    KASSERT(chan != IEEE80211_CHAN_ANYC,
        ("update_chan_and_phymode: bss channel not setup\n"));

    /* Do not add this peer to the peer list if PEER_ASSOC command is not
     * sent to FW.
     */
    if (!ieee80211node_has_extflag(ni, IEEE80211_NODE_ASSOC_RESP))
        return;

#ifdef WLAN_FEATURE_11BE
    old_puncture_bitmap = ni->ni_ch_param.puncture_bitmap;
#endif
    ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(chan),
                                  chan->ic_freq,
                                  chan->ic_vhtop_freq_seg1,
                                  chan->ic_vhtop_freq_seg2
#ifdef WLAN_FEATURE_11BE
                                  , chan->op_puncture_bitmap
#endif
                                 );

    if (is_node_eht) {
        max_chwidth = ic->ic_cwm_get_width(ic);
    } else { 
        non_11be_chan = ieee80211_find_legacy_non11be_channel(ic, chan);
        max_chwidth = get_chwidth_phymode(ieee80211_chan2mode(&non_11be_chan));
    }

    old_ni_chwidth = ni->ni_chwidth;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE, "ocw %d opp %x", old_ni_chwidth, old_puncture_bitmap);

    if (ni->ni_omn_chwidth < max_chwidth) {
        switch (ni->ni_omn_chwidth) {
            case IEEE80211_CWM_WIDTH20:
            case IEEE80211_CWM_WIDTH40:
                /* For 802.11ac VHT20 and VHT40, there is an explicit opmode notify IE
                 * to let the AP know of its chwidth. To avoid comparing VHT
                 * caps, max_chwidth is limited to 40MHz */
                max_chwidth = IEEE80211_CWM_WIDTH40;
                break;
            case IEEE80211_CWM_WIDTH80:
                max_chwidth = IEEE80211_CWM_WIDTH80;
                break;
            default:
                /* Max. width is not changed */
                break;
        }
    }

    if (!pi || !pi->chan_width_peer_list) {
        qdf_err("Allocation error for chwidth peer list");
        return;
    }

    /* Update channel width for upgraded peers */
    ieee80211_update_ni_chwidth(max_chwidth, ni, vap);

    /* Update phy mode */
    ieee80211_update_phymode(ic, ni);

    /* Calculate txrx mcsnss requirement before sending bw to fw */
    ieee80211_calc_nss_based_on_txrx_mcsnss_requirement(ic, ni);

    /* Update puncture bitmap */
    ieee80211_update_ni_puncture_bitmap(ni, ic);

    /* If the peer caps don't support an upgrade, there is no need to
     * send to target */
#ifdef WLAN_FEATURE_11BE
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE, "cw %d pp %x", ni->ni_chwidth, ni->ni_ch_param.puncture_bitmap);
#endif

    if ((ni->ni_chwidth == old_ni_chwidth)
#ifdef WLAN_FEATURE_11BE
            && (ni->ni_ch_param.puncture_bitmap == old_puncture_bitmap)
#endif
            ) {
        return;
    }

    /* Populating peer list
     * NOTE: max_peers includes BSS peer */
    if (pi->num_peers < pi->max_peers) {
        (pi->num_peers)++;
        qdf_mem_copy(pi->chan_width_peer_list[pi->num_peers-1].mac_addr,
                ni->ni_macaddr,
                sizeof(pi->chan_width_peer_list[pi->num_peers-1].mac_addr));
        pi->chan_width_peer_list[pi->num_peers-1].chan_width =
            ieee80211_get_phy_ch_width(ni->ni_chwidth);
#ifdef WLAN_FEATURE_11BE
        pi->chan_width_peer_list[pi->num_peers-1].puncture_bitmap = ni->ni_ch_param.puncture_bitmap;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE, "chw %d, puncture_bitmap %x", ni->ni_chwidth, ni->ni_ch_param.puncture_bitmap);
#endif
    }
}

static
void ieee80211_pdev_iter_node(struct wlan_objmgr_pdev *pdev, void *object,
                              void *arg)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
    struct ieee80211_node *ni;
    struct ieee80211_node_iter_arg *itr_arg =
                                         (struct ieee80211_node_iter_arg *)arg;
    struct ieee80211vap *vap;
    uint8_t *macaddr;

    macaddr = wlan_peer_get_macaddr(peer);

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)  {
         qdf_err("ni unavailable for peer %pK with macaddr: "QDF_MAC_ADDR_FMT,
                  peer, QDF_MAC_ADDR_REF(macaddr));
         return;
    }

    vap = ni->ni_vap;
    /* Ignore BSS node for AP/Monitor mode */
    if (((ni == ni->ni_bss_node) &&
        ((vap->iv_opmode == IEEE80211_M_HOSTAP) ||
         (vap->iv_opmode == IEEE80211_M_MONITOR))) ||
        (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_TEMP)) {
        return;
    }

    /*
     * Ignore the self node for Station mode
     * This is required to skip the self node in case of when station vap
     * is not connected to AP
     */
    if ((vap->iv_opmode == IEEE80211_M_STA) &&
        IEEE80211_ADDR_EQ(vap->iv_myaddr, ni->ni_macaddr)) {
        return;
    }

    /* Ignore station temp nodes for AP mode */
    if (((wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_STA_TEMP) &&
        (vap->iv_opmode == IEEE80211_M_HOSTAP)) ||
        (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_TEMP))
        return;

    /*
     * Ignore unassociated stations for AP mode if bit corresponding
     * to unassociated stations is not set
     */
    if (!(itr_arg->flag & IEEE80211_NODE_ITER_F_UNASSOC_STA)) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && (ni->ni_associd == 0)) {
            return;
        }
    }

    /*
     * Ignore associated stations for AP mode if bit corresponding to
     * associated stations is not set
     */
    if (!(itr_arg->flag & IEEE80211_NODE_ITER_F_ASSOC_STA)) {
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && (ni->ni_associd != 0)) {
            return;
        }
    }

    if (itr_arg->count >= WLAN_UMAC_PSOC_MAX_PEERS) {
        qdf_err("Max peers %u reached for iteration", WLAN_UMAC_PSOC_MAX_PEERS);
        return;
    }

    /* Increment the ref count so that the node is not freed */
    if ((ni = ieee80211_try_ref_node(ni, WLAN_MLME_SB_ID)) != NULL) {
        itr_arg->nodes[itr_arg->count] = ni;
        ++itr_arg->count;
    } else {
        qdf_err("Failed to take reference over node with macaddr: "
                 QDF_MAC_ADDR_FMT, QDF_MAC_ADDR_REF(macaddr));
    }

}

void wlan_mlme_iterate_node_list(wlan_dev_t ic,
                                 ieee80211_sta_iter_func iter_func,
                                 void *arg, uint32_t flag)
{
    struct ieee80211_node_iter_arg *iter_arg;
    uint32_t idx;

    /* No need to proceed if iter_func is NULL, simply return */
    if (!iter_func) {
        qdf_err("No callback function specified for node iteration");
        return;
    }

    iter_arg = qdf_mem_malloc(sizeof(struct ieee80211_node_iter_arg));
    if (!iter_arg) {
        qdf_err("Memory allocation failed for node iter arg");
        return;
    }

    iter_arg->count = 0;
    iter_arg->ic = ic;
    iter_arg->flag = flag;

    /* Call object manager iteration API for peers at pdev level */
    wlan_objmgr_pdev_iterate_obj_list(ic->ic_pdev_obj, WLAN_PEER_OP,
                                      ieee80211_pdev_iter_node,
                                      (void *)iter_arg, 0, WLAN_MLME_SB_ID);

    for (idx = 0; idx < iter_arg->count; ++idx) {
        /* Call callback function iter_func for each node */
        iter_func(arg, iter_arg->nodes[idx]);
        /* Release reference taken over node in ieee80211_pdev_iter_node */
        ieee80211_free_node(iter_arg->nodes[idx], WLAN_MLME_SB_ID);
    }

    qdf_mem_free(iter_arg);
}

void
ieee80211_node_reset(struct ieee80211_node *node)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211vap *vap = NULL;
    int i;
    u_int8_t ac;

    if (!node) {
        qdf_print("%s: NULL Node\n", __func__);
        return;
    }

    if (!node->ni_ic) {
        qdf_print("%s: NULL node ic\n", __func__);
        return;
    }

    if (!node->ni_vap) {
        qdf_print("%s: NULL node vap\n", __func__);
        return;
    }
    ic = node->ni_ic;
    vap = node->ni_vap;
    node->ni_last_rxauth_seq = 0xfff;
    node->ni_last_auth_rx_time = 0;
    node->ni_last_assoc_rx_time = 0;
    node->ni_last_authorize_rst_time = 0;

    /* lp_iot_mode then set default beacon int val IEEE80211_LP_IOT_BCN_INTVAL_DEFAULT */
    if (vap->iv_create_flags & IEEE80211_LP_IOT_VAP)
        node->ni_intval = IEEE80211_LP_IOT_BCN_INTVAL_DEFAULT;
    else
        node->ni_intval = ic->ic_intval; /* default beacon interval */
    node->ni_txpower = ic->ic_txpowlimit; /* max power */
    node->ni_vhtintop_subtype = VHT_INTEROP_OUI_SUBTYPE; /*Setting the interop IE*/
    node->ni_node_esc = false;
    /* load inactivity values */
    node->ni_inact_reload = vap->iv_inact_init;
    node->ni_inact = node->ni_inact_reload;

    /* disable session timeout here. Set it during authorize */
    node->ni_session = IEEE80211_SESSION_TIME;

    if (vap->iv_opmode == IEEE80211_M_IBSS) {
        OS_MEMSET(node->ni_persta, 0 , sizeof(struct ni_persta_key));
    } else {
        /* Explicitly set ni_persta as NULL */
        node->ni_persta = NULL;
    }

    node->ni_ath_defkeyindex = IEEE80211_INVAL_DEFKEY;

    node->ni_wme_miss_threshold = 0;

    /* 11n or 11ac */
    node->ni_chwidth = ic->ic_cwm_get_width(ic);

    /* Initialize seq no of last & 2nd last received frames to 0xffff
    This is to avoid a case where valid frame (Retry bit is set & seq no as 0)
    gets dropped (assuming it as a duplicate frame) */
    for (i = 0; i < (IEEE80211_TID_SIZE+1); i++)
    {
       node->ni_rxseqs[i] = node->ni_last_rxseqs[i] = 0xffff;
    }

    /* set default rate and channel */
    ieee80211_node_set_chan(node);

    //WME_UAPSD_NODE_TRIGSEQINIT(ni);
    for (ac = 0; ac < WME_NUM_AC; ac++) {
        node->ni_uapsd_dyn_trigena[ac] = -1;
        node->ni_uapsd_dyn_delivena[ac] = -1;
    }

    qdf_atomic_set(&(node->ni_peer_del_req_enable), 1);
    node->previous_ps_time = qdf_get_system_timestamp();
}

void
ieee80211_copy_bss(struct ieee80211_node *nbss, const struct ieee80211_node *obss)
{
    /* propagate useful state */
    nbss->ni_ath_flags = obss->ni_ath_flags;
    nbss->ni_txpower = obss->ni_txpower;
    nbss->ni_vlan = obss->ni_vlan;
    nbss->ni_snr = obss->ni_snr;
}

#ifdef MU_CAP_WAR_ENABLED
/*
 * Function to get the total number of MU-MIMO
 * capable clients (including dedicated clients
 */
u_int16_t
get_mu_total_clients(MU_CAP_WAR *war)
{
    int cnt;
    u_int16_t total = 0;
    for (cnt=0;cnt<MU_CAP_CLIENT_TYPE_MAX;cnt++)
    {
        total += war->mu_cap_client_num[cnt];
    }
    return total;
}

/*
 * Function which determines whether the conditions are ripe
 * for the sole dedicated MU-MIMO 1X1 client to be kicked out
 * so that it can join back as SU-MIMO 2X2
 */
int
ieee80211_mu_cap_dedicated_mu_kickout(MU_CAP_WAR *war)
{
    if ((war->mu_cap_client_num[MU_CAP_DEDICATED_SU_CLIENT] == 0) &&
            (war->mu_cap_client_num[MU_CAP_CLIENT_NORMAL] == 0) &&
            (war->mu_cap_client_num[MU_CAP_DEDICATED_MU_CLIENT] == 1))
    {
        return 1;
    }
    return 0;
}

/*
 * Function which sets the probe response behaviour variable
 * based on the counts and overrides
 */
static void
update_probe_response_behaviour(MU_CAP_WAR *war)
{
    if ((!war->mu_cap_war_override) && (get_mu_total_clients(war) == 0)) {
        war->modify_probe_resp_for_dedicated = 1;
    } else {
        war->modify_probe_resp_for_dedicated = 0;
    }
}

/*
 * Find out whether this client is joining after
 * receiving our WAR-"hacked" probe response
 */
static int
is_node_result_of_modified_probe_response(u_int8_t *macaddr,
                                         MU_CAP_WAR *war)
{
    struct DEDICATED_CLIENT_MAC *dedicated_mac;
    struct DEDICATED_CLIENT_MAC *temp;
    int hash = IEEE80211_NODE_HASH(macaddr);
    LIST_FOREACH_SAFE(dedicated_mac, &war->dedicated_client_list[hash], list, temp) {
        if (IEEE80211_ADDR_EQ(dedicated_mac->macaddr, macaddr)) {
            /*
             * No need to keep this entry
             * beyond the time where it needs to be checked
             */
            LIST_REMOVE(dedicated_mac,list);
            war->dedicated_client_number--;
            OS_FREE(dedicated_mac);
            return 1;
        }
    }

    /*
     * If not present in database,means not result of tweaked probe response
     */
    return 0;
}

/*
 * Function to handle all the MU-CAP counts during
 * client join
 */
static u_int8_t
ieee80211_mu_cap_client_join(struct ieee80211_node *ni,
                            struct ieee80211vap *vap,
                            MU_CAP_WAR *war)
{
    u_int8_t new_timer_state = MU_TIMER_STOP;
    int new_index = get_mu_total_clients(war);
    int total_mu_capable_clients;
    int is_tweaked_probe_response =
       is_node_result_of_modified_probe_response(ni->ni_macaddr, war);

    /*Client joining*/
    /*
     * The check for new_index == MAX_PEER_NUM
     * is for avoiding Klocwork issues. This will
     * never happen
     */
    if (!ni->ni_mu_vht_cap || (new_index >= MAX_PEER_NUM))
    {
        return new_timer_state;
    }

    OS_MEMCPY(war->mu_cap_client_addr[new_index],
            (char *)(ni->ni_macaddr),
            QDF_MAC_ADDR_SIZE);

    /*
     * Classification of client into one of the below 3
     * -> Normal MU-Capable client
     * -> Dedicated MU-Capable (if responding to normal Probe-Resp)
     * -> Dedicated SU-Capable (if responding to "hacked" Probe-Resp)
     */
    if(!ni->ni_mu_dedicated) {
        war->mu_cap_client_flag[new_index] = MU_CAP_CLIENT_NORMAL;
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                "MU-Capable non-dedicated client joined");
    } else if (is_tweaked_probe_response) {
        war->mu_cap_client_flag[new_index] = MU_CAP_DEDICATED_SU_CLIENT;
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                "Dedicated SU-Capabale client joined\n");
    } else {
        war->mu_cap_client_flag[new_index] = MU_CAP_DEDICATED_MU_CLIENT;
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                "Dedicated MU-Capabale client joined");
    }

    /*
     * Increment the respective counters
     */
    war->mu_cap_client_num[war->mu_cap_client_flag[new_index]]++;
    if (!war->mu_cap_war)
    {
        /*
         * If WAR is disabled, take no
         * further action
         */
        return new_timer_state;
    }

    /*
     * Decide on the Kick-out action
     */
    total_mu_capable_clients =
        war->mu_cap_client_num[MU_CAP_CLIENT_NORMAL] +
        war->mu_cap_client_num[MU_CAP_DEDICATED_MU_CLIENT];

    if (((total_mu_capable_clients >= 1) &&
                (war->mu_cap_client_num[MU_CAP_DEDICATED_SU_CLIENT] >= 1)) ||
            (war->mu_cap_client_num[MU_CAP_DEDICATED_SU_CLIENT] >= 2)) {
        new_timer_state = MU_TIMER_PENDING;
        war->mu_timer_cmd = MU_CAP_TIMER_CMD_KICKOUT_SU_CLIENTS;
    } else if (ieee80211_mu_cap_dedicated_mu_kickout(war)) {
        new_timer_state = MU_TIMER_PENDING;
        war->mu_timer_cmd = MU_CAP_TIMER_CMD_KICKOUT_DEDICATED;
    }

    /*
     * Decide on override
     */
    if ((total_mu_capable_clients == 0) &&
            (war->mu_cap_client_num[MU_CAP_DEDICATED_SU_CLIENT] >= 2)) {
        /*
         * This is the tricky scenario
         * 2 dedicated clients joining together
         * Both join as 2x2 SU, then get kicked out
         * join back again as 2x2, and this becomes a cycle
         * override will ensure that this kick-out happens
         * only once.
         * The override will ensure that the next time,
         * they send probe request, the probe response will have
         * BF=1, making both of them join as 1x1.
         * Then at the time of the first dedicated client joining,
         * this override will be removed
         */
        war->mu_cap_war_override = 1;
    } else if (total_mu_capable_clients >= 1) {
        /*
         * There is a safe-guard for the mu-capable client count
         * So override can be removed, since the count will make sure
         * that the probe response has beamformer enabled.
         * There is no need of an override at this point
         */
        war->mu_cap_war_override = 0;
    }

    update_probe_response_behaviour(war);


    return new_timer_state;
}

/*
 * Function to handle all the MU-CAP counts during
 * client leave
 */
static u_int8_t
ieee80211_mu_cap_client_leave(struct ieee80211_node *ni,
                              struct ieee80211vap *vap,
                              MU_CAP_WAR *war)
{
    u_int8_t new_timer_state = MU_TIMER_STOP;
    int i;
    int total_mu_clients = get_mu_total_clients(war);
    int last_index = total_mu_clients - 1;

    if (total_mu_clients > MAX_PEER_NUM) {
        /*
         * This condition will never happen
         * Leaving the check here to make
         * Klocwork check pass
         */
        return new_timer_state;
    }

    /*Client leaving*/
    for (i = 0; i < total_mu_clients; i++) {
        if (IEEE80211_ADDR_EQ((char *)(war->mu_cap_client_addr[i]),
                                (char *)(ni->ni_macaddr))) {
            break;
        }
    }
    if (i == total_mu_clients) {
        return new_timer_state;
    }

    /* Decrement the respecitve counter*/
    war->mu_cap_client_num[war->mu_cap_client_flag[i]]--;
    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                   "MU-Capable client leaving, type: %d",
                   war->mu_cap_client_flag[i]);

    /*
     * Replace the entry for the leaving node with the last_index entry
     * Then, fill the last entry with zeroes
     */
    OS_MEMCPY((char *)(war->mu_cap_client_addr[i]),
            (char *)(war->mu_cap_client_addr[last_index]),
            QDF_MAC_ADDR_SIZE);
    qdf_mem_set(&(war->mu_cap_client_addr[last_index][0]),
                    QDF_MAC_ADDR_SIZE, 0);
    war->mu_cap_client_flag[i] = war->mu_cap_client_flag[last_index];
    war->mu_cap_client_flag[last_index] = MU_CAP_CLIENT_NORMAL;

    if (!war->mu_cap_war) {
        /* WAR feature is disabled, no further action */
        return new_timer_state;
    }

    if (ieee80211_mu_cap_dedicated_mu_kickout(war)) {
        new_timer_state = MU_TIMER_PENDING;
        war->mu_timer_cmd = MU_CAP_TIMER_CMD_KICKOUT_DEDICATED;
    }
    update_probe_response_behaviour(war);
    return new_timer_state;
}

/*
 * Function to handle MU-Capable clients'
 * join and leaves
 */
 void
ieee80211_mu_cap_client_join_leave(struct ieee80211_node *ni,
                                    const u_int8_t type)
{
    struct    ieee80211vap *vap = ni->ni_vap;
    u_int8_t  new_timer_state = MU_TIMER_STOP;
    MU_CAP_WAR *war = &vap->iv_mu_cap_war;
    qdf_spin_lock_bh(&war->iv_mu_cap_lock);

    if(type)
    {
        new_timer_state = ieee80211_mu_cap_client_join(ni, vap, war);
    } else {
        new_timer_state = ieee80211_mu_cap_client_leave(ni, vap, war);
    }

    /* Check if to active timer task*/
    if (war->mu_cap_war &&
            (new_timer_state == MU_TIMER_PENDING) &&
            (war->iv_mu_timer_state != MU_TIMER_PENDING))
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                "Starting Dedicated client timer command is %d \n",
                       war->mu_timer_cmd);
        war->iv_mu_timer_state = MU_TIMER_PENDING;
        OS_SET_TIMER(&war->iv_mu_cap_timer,war->mu_cap_timer_period*1000);
    }
    qdf_spin_unlock_bh(&war->iv_mu_cap_lock);
}
#endif

/*
 * Leave the specified IBSS/BSS network.  The node is assumed to
 * be passed in with a held reference.
 */
#if WLAN_OBJMGR_REF_ID_TRACE
bool
ieee80211_sta_leave_debug(struct ieee80211_node *ni, const char *func, int line, const char *file)
#else
bool
ieee80211_sta_leave(struct ieee80211_node *ni)
#endif
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    rwlock_state_t lock_state;
    bool node_reclaimed=false;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_ASSOC, ni,
                   "%s: 0x%x \n", __func__,ni);
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    if (ni->ni_table != NULL) { /* if it is in the table */
        KASSERT((ni->ni_table == nt),
            ("%s: unexpected node table: &ic_sta: 0x%pK, ni_table: 0x%pK,"
             " ni_vap: 0x%pK, ni_ic: 0x%pK, refcnt: %d\n",
             __func__, &ic->ic_sta, ni->ni_table, ni->ni_vap, ni->ni_ic,
             ieee80211_node_refcnt(ni)));

        /* remove wds entries using that node */
        ieee80211_remove_wds_addr(vap, nt, ni->ni_macaddr,IEEE80211_NODE_F_WDS_BEHIND | IEEE80211_NODE_F_WDS_REMOTE);
        ieee80211_del_wds_node(nt, ni);
        /* Refer the node for cleanup below */
        if (ieee80211_try_ref_node(ni, WLAN_MLME_OBJMGR_ID)) {
            /* reclaim the node to remove it from node table */
            node_reclaim(nt, ni);
            node_reclaimed=true;
        }
    }
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    /* cleanup the node */
    if (node_reclaimed) {
        ic->ic_preserve_node_for_fw_delete_resp(ni);
#ifdef WLAN_FEATURE_11BE
        /* Sends Dissassoc, if its not link removal case */
        if(!(wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_STOP_LINK_DEL) ||
            (wlan_vdev_mlme_op_flags_get(vap->vdev_obj, WLAN_VDEV_OP_MLO_LINK_TBTT_COMPLETE)))) {
                /* free the node in RM */
                wlan_mlme_send_disassoc_event(ni->peer_obj, QCA_WLAN_VENDOR_ATTR_GENERIC_CATEGORY_DISASSOC);
        }
#endif
        /* free the node */
        wlan_objmgr_delete_node(ni);
        ic->ic_node_cleanup(ni);
    }

    return node_reclaimed;
}
#ifdef WLAN_OBJMGR_REF_ID_TRACE
qdf_export_symbol(ieee80211_sta_leave_debug);
#else
qdf_export_symbol(ieee80211_sta_leave);
#endif

/*
 * Join the specified IBSS/BSS network.  The node is assumed to
 * be passed in with a reference already held for use in assigning
 * to iv_bss.
 */
int
ieee80211_sta_join_bss(struct ieee80211_node *selbs)
{
    struct ieee80211vap *vap = selbs->ni_vap;
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;
    struct ieee80211_node *obss;
    rwlock_state_t lock_state;
    struct ieee80211com *ic;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    /*
     * Committed to selbs. Leave old bss node if necessary
     */
    /*
     * iv_bss is used in:
     * 1. tx path in STA/WDS mode.
     * 2. rx input_all
     * 3. vap iteration
     * Use node table lock to synchronize the acess.
     */
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    obss = vap->iv_bss;
    ic = vap->iv_ic;
    vap->iv_bss = selbs;
    selbs->ni_bss_node = selbs;
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    if (obss != NULL) {
#if IEEE80211_DEBUG_NODELEAK
        obss->ni_flags |= IEEE80211_NODE_EXT_STATS;
#endif
        ieee80211_node_removeall_wds(&ic->ic_sta,obss);
        if (vap->iv_opmode == IEEE80211_M_STA) {
            wlan_objmgr_delete_node(obss);
        }
    }
    return 0;
}

int
ieee80211_setup_node_rsn(
    struct ieee80211_node *ni,
    ieee80211_scan_entry_t scan_entry
    )
{
    struct ieee80211vap *vap = ni->ni_vap;

    /* parse WPA/RSN IE and setup RSN info */
    if (ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY) {
        u_int8_t *rsn_ie, *wpa_ie;
        u_int8_t *wapi_ie = NULL;

        int status = IEEE80211_STATUS_SUCCESS;
        struct wlan_crypto_params tmp_crypto_params;
        struct wlan_crypto_params *peer_crypto_params;
	peer_crypto_params = wlan_crypto_peer_get_crypto_params(ni->peer_obj);
        if (!peer_crypto_params)
            return -1;
        qdf_mem_copy(&tmp_crypto_params, peer_crypto_params,sizeof(struct wlan_crypto_params));
        rsn_ie  = util_scan_entry_rsn(scan_entry);
        wpa_ie  = util_scan_entry_wpa(scan_entry);
#if ATH_SUPPORT_WAPI
        wapi_ie = util_scan_entry_wapi(scan_entry);
#endif

        if (rsn_ie != NULL)
            status = wlan_crypto_rsnie_check((struct wlan_crypto_params *)&tmp_crypto_params, rsn_ie);

        /* if a RSN IE was not there, or it's not valid, check the WPA IE */
        if ((rsn_ie == NULL) || (status != IEEE80211_STATUS_SUCCESS)) {
            if (wpa_ie != NULL)
                status = wlan_crypto_wpaie_check((struct wlan_crypto_params *)&tmp_crypto_params, wpa_ie);
        }

#if ATH_SUPPORT_WAPI
        if (wapi_ie != NULL)
            status = wlan_crypto_wapiie_check((struct wlan_crypto_params *)&tmp_crypto_params, wapi_ie);
#endif
        if (status != IEEE80211_STATUS_SUCCESS) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE,
                              "%s: invalid security settings for node %s\n",
                              __func__, ether_sprintf(ni->ni_macaddr));
            return -EINVAL;
        }

        qdf_mem_copy(peer_crypto_params, &tmp_crypto_params,sizeof(struct wlan_crypto_params));
        /*
         * if both RSN, WPA and WAPI IEs are absent, then we are certain that cipher is WEP.
         * However, we can't decide whether it's open or shared-key yet.
         */
        if ((rsn_ie == NULL) && (wpa_ie == NULL) && (wapi_ie == NULL)) {
                wlan_crypto_set_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_MCAST_CIPHER,
                         (1 << WLAN_CRYPTO_CIPHER_WEP));
                wlan_crypto_set_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER,
                         (1 << WLAN_CRYPTO_CIPHER_WEP));
        }
    }

    return 0;
}

int8_t derive_sec_chan_orientation(struct ieee80211com *ic, enum ieee80211_phymode phymode , qdf_freq_t pri_chan_freq , qdf_freq_t center_chan_freq)
{
    int8_t pri_center_ch_diff, sec_level;
    uint16_t sec_chan_20, pri_chan_40_center;
    uint8_t pri_chan, center_chan;

    if (ic == NULL) {
        qdf_err("ic null");
        return -1;
    }

    if (wlan_reg_is_same_band_freqs(pri_chan_freq, center_chan_freq) == false ) {
        qdf_err("different band frequencies pri_chan_freq %d, center_chan_freq %d", pri_chan_freq, center_chan_freq);
        return -1;
    }

    pri_chan  = wlan_reg_freq_to_chan(ic->ic_pdev_obj, pri_chan_freq);
    center_chan = wlan_reg_freq_to_chan(ic->ic_pdev_obj,center_chan_freq);

    pri_center_ch_diff = pri_chan - center_chan;
    if(pri_center_ch_diff > 0)
        sec_level = -1;
    else
        sec_level = 1;

    switch(phymode) {
        case  IEEE80211_MODE_11AXA_HE80_80:
        case  IEEE80211_MODE_11AC_VHT80_80:
        case  IEEE80211_MODE_11AXA_HE80:
        case  IEEE80211_MODE_11AC_VHT80:
#ifdef WLAN_FEATURE_11BE
        case  IEEE80211_MODE_11BEA_EHT80:
#endif /* WLAN_FEATURE_11BE */
            if(sec_level*pri_center_ch_diff < -2 )
                sec_chan_20 = center_chan - (sec_level* 2);
            else
                sec_chan_20 = center_chan - (sec_level* 6);
            if(sec_chan_20 > pri_chan )
                return 1;
            else
                return -1;
        case IEEE80211_MODE_11AXA_HE160:
        case IEEE80211_MODE_11AC_VHT160:
#ifdef WLAN_FEATURE_11BE
        case  IEEE80211_MODE_11BEA_EHT160:
#endif /* WLAN_FEATURE_11BE */
            if(sec_level*pri_center_ch_diff < -6 )
                pri_chan_40_center = center_chan - (2*sec_level*6);
            else
                pri_chan_40_center = center_chan - (2*sec_level*2);
            if(pri_chan_40_center > pri_chan)
                return 1;
            else
                return -1;
#ifdef WLAN_FEATURE_11BE
        case  IEEE80211_MODE_11BEA_EHT320:
            switch (pri_center_ch_diff) {
                case SUB20_CHAN_320_1:
                case SUB20_CHAN_320_3:
                case SUB20_CHAN_320_5:
                case SUB20_CHAN_320_7:
                case SUB20_CHAN_320_9:
                case SUB20_CHAN_320_11:
                case SUB20_CHAN_320_13:
                case SUB20_CHAN_320_15:
                    return 1;
                case SUB20_CHAN_320_2:
                case SUB20_CHAN_320_4:
                case SUB20_CHAN_320_6:
                case SUB20_CHAN_320_8:
                case SUB20_CHAN_320_10:
                case SUB20_CHAN_320_12:
                case SUB20_CHAN_320_14:
                case SUB20_CHAN_320_16:
                    return -1;
                default:
                    qdf_err("Invalid pri_chan_freq %d, center_chan_freq %d, phymode %d",pri_chan_freq, center_chan_freq, phymode);
                    return -1;

            }
#endif /* WLAN_FEATURE_11BE */
        default :
            return 0;
    }
}

#define IEEE80211_MODE_SET(bm,m)  ((bm) |= (1ULL << (m)))
#define IEEE80211_MODE_IS_SET(bm,m)  (((bm) & (1ULL << (m))) != 0 )
/*
 * check if the phymode forced by user is compatible (sub phy mode) with the phy mode of the AP.
 * if  it is compatible then return the phy mode else  return AUTO phy mode.
 *  bss_chan : bss chan of the AP .
 *  des_mode: mode forced by user for the STA.
 *  bss_mode: operating mode of the AP
*/

enum ieee80211_phymode ieee80211_get_phy_mode(struct ieee80211com *ic,
                                        struct ieee80211_ath_channel *bss_chan,
                                        enum ieee80211_phymode des_mode, enum ieee80211_phymode bss_mode)
{
    uint64_t mode_bitmap = 0;
    enum ieee80211_cwm_width bss_chwidth, des_chwidth;
    /*
     *  NOTE:-
     *  1)If a device is capable of VHT80_80 it must also support VHT160.
     *  2)However, if a device is capable  VHT160 it need not necessarily
     *  support VHT80_80.
     *  3)bss_mode is actually the mode of the channel in which AP
     *  has started operating in.
     *
     *  Given the bss_mode and des_mode the matrix entries give/output a
     *  compatible mode to be used for connection.
     *
     *
     *                                       bss_modes
     *                            (the mode of AP's current
     *                                 channel of operation)
     *                          =========================================
     *                          ||  160  |  80_80 |  80  |  40  |  20  ||
     *                  ========||=======|========|======|======|======||
     *                  ||160   ||  160  |  80    |  80  |  40  |  20  ||
     *                  ||------||-------|--------|------|------|------||
     *                  ||80_80 ||  160  |  80_80 |  80  |  40  |  20  ||
     *                  ||------||-------|--------|------|------|------||
     * des_modes        ||80    ||  80   |  80    |  80  |  40  |  20  ||
     * (the             ||------||-------|--------|------|------|------||
     * capability       ||40    ||  40   |  40    |  40  |  40  |  20  ||
     * of the STA)      ||------||-------|--------|------|------|------||
     *                  ||20    ||  20   |  20    |  20  |  20  |  20  ||
     *                  =================================================
     *
     *  In general, the compatible mode is intersection(lesser) of bss_mode and
     *  des_mode.
     *  There are two exceptions:
     *  Exception1:-  bss_mode = VHT160,   des_mode= VHT80_80
     *                  compatible_mode=VHT160
     *  Exception2:-  bss_mode = VHT80_80, des_mode= VHT160
     *                  compatible_mode=VHT80
     *
     *  In Exception1, since the des_mode is VHT80_80, the STA is also capable
     *  of VHT160 as per standard therefore compatible mode is VHT160.
     *
     *  In Exception2, since the bss_mode is VHT80_80 the AP should
     *  be able to support VHT160 as per standard but since the AP has already started in
     *  a channel with mode VHT80_80, STA must follow the channel with mode VHT80_80.
     *  And since STA's  desired mode is VHT160 if STA comes up in a channel
     *  with mode VHT160 then the AP's and STA's channels will not be compatible.
     *  Therefore, the common mode VHT80 is the compatible mode in this case.
     */

    /*
     * for the given APs phymode construct a bitmap of all compatible sub phy modes.
     */
    switch(bss_mode) {
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT320:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT320);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11BEA_EHT160 :
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
	case IEEE80211_MODE_11BEA_EHT80:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
	case IEEE80211_MODE_11BEA_EHT40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11BEA_EHT40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11BEA_EHT20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEA_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11BEG_EHT40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11BEG_EHT40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11BEG_EHT20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11BEG_EHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
#endif /* WLAN_FEATURE_11BE */
        case IEEE80211_MODE_11AXA_HE80_80:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80_80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80_80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AC_VHT80_80:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80_80);
            /* NOTE:- VHT160 is not set. See Exception2.*/
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq , bss_chan->ic_vhtop_freq_seg1) > 0) {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AXA_HE160 :
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE160 );
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AC_VHT160:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT160);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg2) > 0) {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
                IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
	case IEEE80211_MODE_11AXA_HE80:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE80);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AC_VHT80:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
            if (derive_sec_chan_orientation(ic, bss_mode , bss_chan->ic_freq, bss_chan->ic_vhtop_freq_seg1) > 0) {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            } else {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            }
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
	case IEEE80211_MODE_11AXA_HE40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AC_VHT40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AXA_HE40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AC_VHT40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AXA_HE20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXA_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AC_VHT20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11NA_HT40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11NA_HT40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11NA_HT20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11AXG_HE40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11NG_HT40PLUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40PLUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11AXG_HE40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11NG_HT40MINUS:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40MINUS);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11AXG_HE20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AXG_HE20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11NG_HT20:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11A:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
            break;
        case IEEE80211_MODE_11G:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
        case IEEE80211_MODE_11B:
            IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
            break;
    default:
        break;

    }

    /*
     * If des_mode bandwidth is greater than the bss_mode bandwidth,
     * then update the des_mode to use the bss_mode bandwidth as per the table
     * above
     */
    bss_chwidth = get_chwidth_phymode(bss_mode);
    des_chwidth = get_chwidth_phymode(des_mode);
    if ((des_chwidth != IEEE80211_CWM_WIDTHINVALID) &&
        (bss_chwidth != IEEE80211_CWM_WIDTHINVALID) &&
        (des_chwidth > bss_chwidth)) {
        des_mode = ieee80211_get_composite_phymode(get_mode_from_phymode(des_mode),
                                        bss_chwidth,
                                        ieee80211_is_phymode_40plus(bss_mode) ?
                                         IEEE80211_SEC_CHAN_OFFSET_SCA :
                                         IEEE80211_SEC_CHAN_OFFSET_SCB);
    }

    /*
     * check if the user selected mode for STA is part of the bitmap of compatible phy modes.
     */
    if (IEEE80211_MODE_IS_SET(mode_bitmap, des_mode)) {
      /*
       * if user requested HT40 then return HT40PLUS  if HT40PLUS is comaptible with AP
       * else return HT40MINUS.
       */
        switch (des_mode) {
            case IEEE80211_MODE_11NA_HT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11NA_HT40PLUS)) {
                    des_mode = IEEE80211_MODE_11NA_HT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11NA_HT40MINUS;
                }
                break;
            case IEEE80211_MODE_11AC_VHT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11AC_VHT40PLUS)) {
                    des_mode = IEEE80211_MODE_11AC_VHT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11AC_VHT40MINUS;
                }
                break;
            case IEEE80211_MODE_11AXA_HE40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11AXA_HE40PLUS)) {
                    des_mode = IEEE80211_MODE_11AXA_HE40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11AXA_HE40MINUS;
                }
                break;
            case IEEE80211_MODE_11NG_HT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11NG_HT40PLUS)) {
                    des_mode = IEEE80211_MODE_11NG_HT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11NG_HT40MINUS;
                }
               break;
            case IEEE80211_MODE_11AXG_HE40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11AXG_HE40PLUS)) {
                    des_mode = IEEE80211_MODE_11AXG_HE40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11AXG_HE40MINUS;
                }
                break;
#ifdef WLAN_FEATURE_11BE
            case IEEE80211_MODE_11BEA_EHT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11BEA_EHT40PLUS)) {
                    des_mode = IEEE80211_MODE_11BEA_EHT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11BEA_EHT40MINUS;
                }
                break;
            case IEEE80211_MODE_11BEG_EHT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11BEG_EHT40PLUS)) {
                    des_mode = IEEE80211_MODE_11BEG_EHT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11BEG_EHT40MINUS;
                }
                break;
#endif /* WLAN_FEATURE_11BE */
            default:
               break;
        }
    } else {
        /* Handle Exceptions:
         * Exception1 : BSS MODE = VHT160 and Desired mode = VHT80_80
         * Exception1 is handled automatically since bitmap is not set for des_mode
         * and the function returns AUTO. If AUTO is returned then STA connects
         * in Root AP's mode.
         *
         * Exception2 : BSS MODE = VHT80_80 and Desired mode = VHT160
         */
        if (ieee80211_is_phymode_11ac_vht80_80(bss_mode) &&
            ieee80211_is_phymode_11ac_vht160(des_mode)) {
            /* TODO xxxxx:- we also need to check elsewhere, if des_mode is really
             * supported by the hardware.
             */
            des_mode = IEEE80211_MODE_11AC_VHT80;
        } else {
            des_mode = IEEE80211_MODE_AUTO;
        }
    }
    return des_mode;
}

struct ieee80211_ath_channel
ieee80211_find_connect_channel(struct ieee80211vap *vap,
                               struct ieee80211_ath_channel *ni_chan,
                               enum ieee80211_phymode *intersected_phymode,
                               bool *is_input_phymode_radar,
                               uint8_t ieee80211_msg_debug_id)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct chan_params chan_p = {0};
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO, des_phymode, scan_phymode;

    chan_p.freq = ni_chan->ic_freq;
    chan_p.cfreq2 = ni_chan->ic_vhtop_freq_seg2;
    set_chan_params_puncture_bitmap(&chan_p, ni_chan->op_puncture_bitmap);

    scan_phymode = ieee80211_chan2mode(ni_chan);
    des_phymode = ieee80211_get_non_auto_des_mode(&chan_p, vap);
    if (des_phymode == IEEE80211_MODE_NONE) {
        ieee80211_note(vap, ieee80211_msg_debug_id, "%s Cannot find a valid mode\n", __func__);
        return empty_channel;
    }

    phymode = ieee80211_get_phy_mode(ic, ni_chan, des_phymode, scan_phymode);
    if (phymode == IEEE80211_MODE_AUTO) {
        ieee80211_note(vap, ieee80211_msg_debug_id, "%s Intersected Phymode is Auto."
                " Using Scan Entry phymode %d. Vap des mode %d", __func__, scan_phymode, vap->iv_des_mode);
        phymode = scan_phymode;
    }

    /* Find new PP by intersecting the Scan entry phymode and derived phymode */
    set_chan_params_puncture_bitmap(&chan_p,
                                    ieee80211_intersect_puncture_bitmap(ni_chan,
                                                                        phymode,
                                                                        scan_phymode)
                                    );

    *intersected_phymode = phymode;

    return ieee80211_find_dot11_channel_without_radar(ic,
                                                      &chan_p,
                                                      phymode,
                                                      is_input_phymode_radar);
}

/**
 * ieee80211_fill_chan_params() - Fill chan_p object from ni_chan.
 * @ni_chan: Pointer to struct ieee80211_ath_channel
 * @chan_p: Pointer to struct chan_params
 *
 * Return: None
 */
static void
ieee80211_fill_chan_params(struct ieee80211_ath_channel *ni_chan,
                           struct chan_params *chan_p)
{
    chan_p->freq = ni_chan->ic_freq;
    chan_p->cfreq2 = ni_chan->ic_vhtop_freq_seg2;
    set_chan_params_puncture_bitmap(chan_p, ni_chan->op_puncture_bitmap);
}

/*
 * Setup a node based on the scan entry
 */
int
ieee80211_setup_node(
    struct ieee80211_node *ni,
    ieee80211_scan_entry_t scan_entry
    )
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    u_int8_t *rates, *xrates;
    struct ieee80211_country_ie* countryie;
    u_int8_t *htcap = NULL;
    u_int8_t *htinfo = NULL;
    u_int8_t *vhtcap = NULL;
    u_int8_t *vhtop = NULL;
    u_int8_t *hecap = NULL;
    u_int8_t *hecap_6g = NULL;
    u_int8_t *heop = NULL;
    u_int8_t **tpe = NULL;
    u_int8_t *wme = NULL;
    u_int8_t *athextcap = NULL;
    u_int8_t *ssid;
#ifdef WLAN_FEATURE_11BE
    u_int8_t *ehtcap = NULL;
    u_int8_t *ehtop = NULL;
#endif /* WLAN_FEATURE_11BE */
    int i;
    int ht_rates_allowed;
    int error = 0;
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;
    struct ieee80211_ath_channel ni_chan;
    struct chan_params chan_p = {0};
    uint16_t in_punc_bitmap;
    bool is_input_phymode_radar = false;

    ASSERT((vap->iv_opmode == IEEE80211_M_STA) ||
           (vap->iv_opmode == IEEE80211_M_IBSS));

#if QCN_IE
    if (scan_entry->ie_list.qcn != NULL)
        ieee80211_parse_qcnie(scan_entry->ie_list.qcn, NULL, ni, NULL, 0xFF);
#endif

    /*
     * If NIC does not support the channels in this node, NULL is returned.
     */
    ni_chan = wlan_util_scan_entry_channel(ic, scan_entry, false);

#ifdef WLAN_FEATURE_11BE
    if (!ieee80211_is_phymode_11be(vap->iv_des_mode) &&
        IEEE80211_IS_CHAN_11BE(&ni_chan)) {
        ni_chan = ieee80211_find_non_punctured_channel(ic, &ni_chan);
    }
#endif /* WLAN_FEATURE_11BE */


    ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(&ni_chan),
                                  ni_chan.ic_freq,
                                  ni_chan.ic_vhtop_freq_seg1,
                                  ni_chan.ic_vhtop_freq_seg2
#ifdef WLAN_FEATURE_11BE
                                  , ni_chan.op_puncture_bitmap
#endif
                                 );

    /* Assert this in debug driver, but fail gracefully in release driver. */
    if (!IEEE80211_IS_CHAN_VALID(&ni_chan))
        return -EIO;

    ieee80211_fill_chan_params(&ni_chan, &chan_p);

    IEEE80211_NOTE(vap, IEEE80211_MSG_SCANENTRY, ni,
            "%s: vap=0x%p, ni->ni_ic_flags=%llx phymode=%x \n",
            __func__, vap, ni_chan.ic_flags, ieee80211_chan2mode(&ni_chan));

    ni_chan = ieee80211_find_connect_channel(vap,
                                             &ni_chan,
                                             &phymode,
                                             &is_input_phymode_radar,
                                             IEEE80211_MSG_NODE);
    if (is_input_phymode_radar) {
        ieee80211_note(vap, IEEE80211_MSG_NODE, "%s Freq [%d] Mode [%d] has"
                " radar hit subchannels", __func__, chan_p.freq, phymode);
        if (IEEE80211_IS_CHAN_VALID(&ni_chan)) {
            phymode = ieee80211_chan2mode(&ni_chan);
        } else {
            ieee80211_note(vap, IEEE80211_MSG_NODE, "%s Cannot find a valid non-radar channel", __func__);
            return -EIO;
        }
    }

    ieee80211_note(vap, IEEE80211_MSG_NODE, "%s forcing sta to"
                   " associate in %d mode\n", __func__, phymode);

    if (!IEEE80211_IS_CHAN_VALID(&ni_chan) && WLAN_REG_IS_6GHZ_CHAN_FREQ(chan_p.freq)) {
        chan_p.input_6g_pwr_mode = REG_BEST_PWR_MODE;
        ni_chan = ieee80211_find_dot11_channel(ic, &chan_p, phymode);

        if (IEEE80211_IS_CHAN_VALID(&ni_chan)) {
            in_punc_bitmap = ieee80211_get_chan_param_punc_bitmap(&chan_p);
            ieee80211_apply_best_6g_ap_pwr_mode(ic, chan_p.freq,
                                                chan_p.cfreq2,
                                                true, phymode,
                                                in_punc_bitmap);
        }
    }

    ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(&ni_chan),
                                  ni_chan.ic_freq,
                                  ni_chan.ic_vhtop_freq_seg1,
                                  ni_chan.ic_vhtop_freq_seg2
#ifdef WLAN_FEATURE_11BE
                                  , ni_chan.op_puncture_bitmap
#endif
                                 );

    if (!IEEE80211_IS_CHAN_VALID(&ni_chan)) {
        ieee80211_note(vap, IEEE80211_MSG_NODE,
                       "%s, can not find a channel with the desired mode \n", __func__);
        return -EIO;
    }

#ifdef WLAN_FEATURE_11BE_MLO
    /* If the vdev is configured as MLD STA and the phymode is not 11BE,
     * then discard the connect req by returning failure from setup node as
     * MLO is expected to be supported only with 11BE mode
     */
    if (mlo_is_mld_sta(vap->vdev_obj) && phymode < IEEE80211_MODE_11BEA_EHT20) {
        qdf_err("Invalid phymode:%d for MLO STA vdev: %d on psoc:%d",
                phymode, wlan_vdev_get_id(vap->vdev_obj),
                wlan_vdev_get_psoc_id(vap->vdev_obj));
        return -EIO;
    }
#endif

    IEEE80211_NOTE(vap, IEEE80211_MSG_SCANENTRY, ni,
        "%s vap=0x%p, ni_chan.ic_ieee=%d freq=%d phymode=%x vap_des_mode =%d \n",__func__,
        vap, ni_chan.ic_ieee, ni_chan.ic_freq, phymode, vap->iv_des_mode);

    IEEE80211_ADDR_COPY(ni->ni_bssid, util_scan_entry_bssid(scan_entry));
    ssid = util_scan_entry_ssid(scan_entry)->ssid;
    ni->ni_esslen = util_scan_entry_ssid(scan_entry)->length;
    if (ni->ni_esslen != 0 && (ni->ni_esslen < (IEEE80211_NWID_LEN+1))) {
        OS_MEMCPY(ni->ni_essid, ssid, ni->ni_esslen);

        wlan_vdev_mlme_set_ssid(vap->vdev_obj, (const uint8_t*)ssid,
                ni->ni_esslen);
    }

    ni->ni_capinfo = util_scan_entry_capinfo(scan_entry).value;
    ni->ni_erp = util_scan_entry_erpinfo(scan_entry);

    countryie = (struct ieee80211_country_ie*)util_scan_entry_country(scan_entry);
    if(countryie) {
        ni->ni_cc[0] = countryie->cc[0];
        ni->ni_cc[1] = countryie->cc[1];
        ni->ni_cc[2] = countryie->cc[2];
    } else {
        ni->ni_cc[0] = 0;
        ni->ni_cc[1] = 0;
        ni->ni_cc[2] = 0;
    }

    ni->ni_intval = util_scan_entry_beacon_interval(scan_entry);
    ni->ni_lintval = ic->ic_lintval;
    LIMIT_BEACON_PERIOD(ni->ni_intval);

    /*
     * Verify that ATIM window is smaller than beacon interval.
     * This kind of misconfiguration can put hardware into unpredictable state
     */
    ASSERT(ni->ni_intval > vap->iv_atim_window);

    /* Clear node flags */
    //ni->ni_ext_caps = ni->ni_flags = ni->ni_ath_flags = ni->ni_htcap = 0;
    ni->ni_ext_caps = 0;
    ni->ni_flags = 0;
    ni->ni_htcap = 0;
    ni->ni_ath_flags = 0;
    ni->ni_vhtcap = 0;
    qdf_mem_zero(&ni->ni_he, sizeof(struct ieee80211_he_handle));

    /* update WMM capability */
    if (((wme = util_scan_entry_wmeinfo(scan_entry))  != NULL) ||
        ((wme = util_scan_entry_wmeparam(scan_entry)) != NULL)) {
        u_int8_t    qosinfo;

        ni->ni_ext_caps |= IEEE80211_NODE_C_QOS;
        if (ieee80211_parse_wmeinfo(vap, wme, &qosinfo) >= 0) {
            if (qosinfo & WME_CAPINFO_UAPSD_EN) {
                ni->ni_ext_caps |= IEEE80211_NODE_C_UAPSD;
            }
        }
    }

    if ((athextcap = (u_int8_t *) util_scan_entry_athextcaps(scan_entry)) != NULL) {
        ieee80211_process_athextcap_ie(ni, athextcap);
    }

    /* parse WPA/RSN IE and setup RSN info */
    error = ieee80211_setup_node_rsn(ni, scan_entry);

    /*
     * With WEP and TKIP encryption algorithms:
     * Diable aggregation if IEEE80211_NODE_WEPTKIPAGGR is not set.
     * Disable 11n if IEEE80211_FEXT_WEP_TKIP_HTRATE is not set.
     */
    ht_rates_allowed = 1;
    if((IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) &&
        is_weptkip_htallowed(vap, NULL))
    || ((ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY) &&
         is_weptkip_htallowed(NULL, ni))){
        ieee80211node_set_flag(ni, IEEE80211_NODE_WEPTKIP);
        if (ieee80211_ic_wep_tkip_htrate_is_set(ic)) {
            if (!ieee80211_has_weptkipaggr(ni))
                ieee80211node_set_flag(ni, IEEE80211_NODE_NOAMPDU);
        } else {
            ht_rates_allowed = 0;
        }
    }

    if ((vap->iv_opmode == IEEE80211_M_IBSS) &&
        !ieee80211_ic_ht20Adhoc_is_set(ic) &&
        !ieee80211_ic_ht40Adhoc_is_set(ic)) {
        ht_rates_allowed = 0;
    }

    if (ht_rates_allowed) {
        u_int8_t *bwnss_map = NULL;

        htcap  = util_scan_entry_htcap(scan_entry);
        htinfo = util_scan_entry_htinfo(scan_entry);
        if (htcap && (IEEE80211_IS_CHAN_11N(&ni_chan) ||
                      IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                      IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
            ieee80211_parse_htcap(ni, htcap, NULL);
        }
        if (htinfo && (IEEE80211_IS_CHAN_11N(&ni_chan) ||
                       IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                       IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
            ieee80211_parse_htinfo(ni, htinfo);
        }

        if ((vap->iv_opmode == IEEE80211_M_IBSS) && !ieee80211_ic_ht40Adhoc_is_set(ic)) {
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
        }

        if ((vap->iv_opmode == IEEE80211_M_IBSS) && !ieee80211_ic_htAdhocAggr_is_set(ic)) {
            ieee80211node_set_flag(ni, IEEE80211_NODE_NOAMPDU);
        }

        vhtcap  = util_scan_entry_vhtcap(scan_entry);
        vhtop  = util_scan_entry_vhtop(scan_entry);
        bwnss_map = util_scan_entry_bwnss_map(scan_entry);
        if (bwnss_map) {
            ni->ni_prop_ie_used = 1;
            ni->ni_bw160_nss = IEEE80211_GET_BW_NSS_FWCONF_160(*(u_int32_t *)bwnss_map);
        } else {
            ni->ni_bw160_nss = 0;
        }

        if (vhtcap && (IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                       IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
            ieee80211_parse_vhtcap(ni, vhtcap, NULL);
        }

        if (htinfo && vhtop && (IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                                IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
            ieee80211_parse_vhtop(ni, vhtop, htinfo);
        }

        hecap = util_scan_entry_hecap(scan_entry);
        heop = util_scan_entry_heop(scan_entry);
        hecap_6g = util_scan_entry_he_6g_cap(scan_entry);

        if (hecap && IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan) && ieee80211vap_heallowed(ni->ni_vap)) {
            ieee80211_parse_hecap(ni, hecap, IEEE80211_FC0_SUBTYPE_DEBUG);
        }

        if (heop && IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan) && ieee80211vap_heallowed(ni->ni_vap)) {
            ieee80211_parse_heop(ni, heop, IEEE80211_FC0_SUBTYPE_DEBUG, NULL);
        }

        tpe = util_scan_entry_tpe(scan_entry);
        ieee80211_process_tpe_ie(ni, tpe, scan_entry);

#ifdef WLAN_FEATURE_11BE
        ehtcap = util_scan_entry_ehtcap(scan_entry);
        ehtop = util_scan_entry_ehtop(scan_entry);

        if (ehtcap && IEEE80211_IS_CHAN_EHT(&ni_chan) ) {
            ieee80211_parse_ehtcap(ni, ehtcap, IEEE80211_FC0_SUBTYPE_DEBUG);
        }

        if (ehtop && IEEE80211_IS_CHAN_EHT(&ni_chan) ) {
            ieee80211_parse_ehtop(ni, ehtop, IEEE80211_FC0_SUBTYPE_DEBUG, NULL);
        }
#endif /* WLAN_FEATURE_11BE */

        if(hecap_6g && IEEE80211_IS_CHAN_HE(&ni_chan) ) {
            ieee80211_parse_he_6g_bandcap(ni, hecap_6g, IEEE80211_FC0_SUBTYPE_DEBUG);
        }

    }

    /* NB: must be after ni_chan is setup */
    rates = util_scan_entry_rates(scan_entry);
    xrates = util_scan_entry_xrates(scan_entry);
    if (rates) {
        ieee80211_setup_rates(ni, rates, xrates, IEEE80211_F_DOXSECT);
    }
    if (htcap && (IEEE80211_IS_CHAN_11N(&ni_chan) ||
                  IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                  IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
        ieee80211_setup_ht_rates(ni, htcap, IEEE80211_F_DOXSECT);
    }
    if (htinfo && (IEEE80211_IS_CHAN_11N(&ni_chan) ||
                   IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                  IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
        ieee80211_setup_basic_ht_rates(ni, htinfo);
    }
    if (vhtcap && (IEEE80211_IS_CHAN_VHT(&ni_chan) ||
                  IEEE80211_IS_CHAN_11AX_SUPPORTED(&ni_chan))) {
        ieee80211_setup_vht_rates(ni, vhtcap, IEEE80211_F_DOXSECT);
    }

    /* 11AX TODO (Phase II)  TBD for HE rates
     * Need to populate the HE rates once NSS & MCS
     * are defined in 1.0 spec
     */

    /*
     * ieee80211_parse_vhtop would hav set the channel width based on APs operating mode/channel.
     * if vap is forced to operate in a different lower mode than what AP is opearing,
     *  then set the channel width based on  the forced channel/phy mode .
     */
    if (!ieee80211_is_phymode_auto(phymode)) {
        switch(phymode) {
        case IEEE80211_MODE_11A          :
        case IEEE80211_MODE_11B          :
        case IEEE80211_MODE_11G          :
        case IEEE80211_MODE_11NA_HT20    :
        case IEEE80211_MODE_11NG_HT20    :
        case IEEE80211_MODE_11AC_VHT20   :
        case IEEE80211_MODE_11AXA_HE20   :
        case IEEE80211_MODE_11AXG_HE20   :
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT20   :
        case IEEE80211_MODE_11BEG_EHT20   :
#endif /* WLAN_FEATURE_11BE */
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
            break;
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40PLUS :
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11AC_VHT40PLUS:
        case IEEE80211_MODE_11AC_VHT40MINUS:
        case IEEE80211_MODE_11AXA_HE40PLUS:
        case IEEE80211_MODE_11AXA_HE40MINUS:
        case IEEE80211_MODE_11AXG_HE40PLUS:
        case IEEE80211_MODE_11AXG_HE40MINUS:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT40PLUS:
        case IEEE80211_MODE_11BEA_EHT40MINUS:
        case IEEE80211_MODE_11BEG_EHT40PLUS:
        case IEEE80211_MODE_11BEG_EHT40MINUS:
#endif /* WLAN_FEATURE_11BE */
            ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
            break;
        case IEEE80211_MODE_11AC_VHT80:
        case IEEE80211_MODE_11AXA_HE80:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT80:
#endif /* WLAN_FEATURE_11BE */
            ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
            break;
        case IEEE80211_MODE_11AC_VHT160:
        case IEEE80211_MODE_11AC_VHT80_80:
        case IEEE80211_MODE_11AXA_HE160:
        case IEEE80211_MODE_11AXA_HE80_80:
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT160:
#endif /* WLAN_FEATURE_11BE */
            ni->ni_chwidth = IEEE80211_CWM_WIDTH160;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_MODE_11BEA_EHT320:
            ni->ni_chwidth = IEEE80211_CWM_WIDTH320;
            break;
#endif /* WLAN_FEATURE_11BE */
        default :
            break;

        }
    }

    /* Find min basic supported rate */
    ni->ni_minbasicrate = 0;
    for (i=0; i < ni->ni_rates.rs_nrates; i++) {
        if ((ni->ni_minbasicrate == 0) ||
            ((ni->ni_minbasicrate & IEEE80211_RATE_VAL) > (ni->ni_rates.rs_rates[i] & IEEE80211_RATE_VAL))) {
            ni->ni_minbasicrate = ni->ni_rates.rs_rates[i];
        }
    }
    wlan_peer_set_minbasicrate(ni->peer_obj, ni->ni_minbasicrate);

    /* Error at parsing WPA/RSN IE */
    if (error != 0)
        return error;

    return 0;
}

#if WLAN_OBJMGR_REF_ID_TRACE
struct ieee80211_node *
ieee80211_try_ref_bss_node_debug(struct ieee80211vap *vap, wlan_objmgr_ref_dbgid id,
                         const char *func, int line, const char *file)
#else  /* !WLAN_OBJMGR_REF_ID_TRACE */
struct ieee80211_node *
ieee80211_try_ref_bss_node(struct ieee80211vap *vap, wlan_objmgr_ref_dbgid id)
#endif  /* WLAN_OBJMGR_REF_ID_TRACE */
{
    struct ieee80211_node *ni = NULL;

    if (vap->iv_bss) {
        /**
         * Ref node is an atomic operation. No need of extra lock.
         * To avoid deadlock, previous OS_BEACON_READ_LOCK() of
         * nt->nt_nodelock is removed.
         */
#if WLAN_OBJMGR_REF_ID_TRACE
        ni = ieee80211_try_ref_node_debug(vap->iv_bss, id, func, line, file);
#else
        ni = ieee80211_try_ref_node(vap->iv_bss, id);
#endif
    }
    return ni;
}

#if WLAN_OBJMGR_REF_ID_TRACE
struct ieee80211_node *
ieee80211_ref_bss_node_debug(struct ieee80211vap *vap, wlan_objmgr_ref_dbgid id,
                         const char *func, int line, const char *file)
#else  /* !WLAN_OBJMGR_REF_ID_TRACE */
struct ieee80211_node *
ieee80211_ref_bss_node(struct ieee80211vap *vap, wlan_objmgr_ref_dbgid id)
#endif  /* WLAN_OBJMGR_REF_ID_TRACE */
{
    struct ieee80211_node *ni = NULL;

    if (vap->iv_bss) {
        /**
         * Ref node is an atomic operation. No need of extra lock.
         * To avoid deadlock, previous OS_BEACON_READ_LOCK() of
         * nt->nt_nodelock is removed.
         */
#if WLAN_OBJMGR_REF_ID_TRACE
        ni = ieee80211_ref_node_debug(vap->iv_bss, id, func, line, file);
#else
        ni = ieee80211_ref_node(vap->iv_bss, id);
#endif
    }
    return ni;
}

void
ieee80211_flush_vap_mgmt_queue(struct ieee80211vap *vap, bool force)
{
    struct ieee80211com *ic = vap->iv_ic;

    /* force flush if recovery in progress */
    if (ic->recovery_in_progress)
        force = true;

    if(vap->iv_bss) {
        if(ic->ic_if_mgmt_drain)
            ic->ic_if_mgmt_drain(vap->iv_bss, force, IEEE80211_VDEV_MGMT_DRAIN);
    }
}

void ieee80211_flush_peer_mgmt_queue(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;

    if(ic->ic_if_mgmt_drain)
        ic->ic_if_mgmt_drain(ni, false, IEEE80211_PEER_MGMT_DRAIN);
}
/*
 * Reset bss state on transition to the INIT state.
 * Clear any stations from the table (they have been
 * deauth'd) and reset the bss node (clears key, rate,
 * etc. state).
 */
int
ieee80211_reset_bss(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni, *obss;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    ieee80211_flush_vap_mgmt_queue(vap, false);


    /* Allocate peer object */
    if (wlan_psoc_nif_feat_cap_get(wlan_pdev_get_psoc(ic->ic_pdev_obj),
                                   WLAN_SOC_F_PEER_CREATE_RESP)) {
        vap->iv_bss_rsp_evt_status = false;
        vap->iv_bss_rsp_status = 0;
    } else {
        wlan_vap_set_bss_status(vap, PEER_CREATED);
    }

    ni = wlan_objmgr_alloc_ap_node(vap, vap->iv_myaddr);
    if (ni == NULL) {
        qdf_nofl_info("Failed to create bss node\n");
        return -ENOMEM;
    }

    /*
     * iv_bss is used in:
     * 1. tx path in STA/WDS mode.
     * 2. rx input_all
     * 3. vap iteration
     * Use node table lock to synchronize the acess.
     */
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    obss = vap->iv_bss;
    vap->iv_bss = ni; /* alloc node gives the needed extra reference */

    /* vap->iv_bss is assigned to vap->iv_ni in ieee80211_process_csa().
     * Hence, set vap->iv_ni to NULL to avoid accessing this pointer after
     * freeing the previous vap->iv_bss pointer.
     */
    vap->iv_ni = NULL;

    /*
     * XXX: remove the default node from node table, because
     * it's not associated to any one. This will fix reference count
     * leak when freeing the default node.
     */
    ieee80211_node_table_reset_nolock_iter_cb(vap, ni);

    /* Below check and reset is applicable for station vap */
    if (obss)
        ieee80211_node_table_reset_nolock_iter_cb(vap, obss);

    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    if (obss != NULL) {
        /* Do we really need obss info?? */
        ieee80211_copy_bss(ni, obss);
#if IEEE80211_DEBUG_NODELEAK
        obss->ni_flags |= IEEE80211_NODE_EXT_STATS;
#endif
        ni->ni_intval = obss->ni_intval;

        /*
         * Prevent node from getting physically deleted
         * till we get delete confirmation event
         */
        ic->ic_preserve_node_for_fw_delete_resp(obss);

        ieee80211_ref_node(obss, WLAN_MLME_OBJMGR_ID);

        wlan_objmgr_delete_node(obss);

        /* Cleanup the old BSS node */
        ic->ic_node_cleanup(obss);
        ieee80211_free_node(obss, WLAN_MLME_OBJMGR_ID);
    }
    return 0;
}

/*
 * Node table support.
 */
static void
ieee80211_node_table_init(struct ieee80211com *ic,
                          struct ieee80211_node_table *nt,
                          const char *name
			  )
{
    int hash;

    nt->nt_ic = ic;
    OS_RWLOCK_INIT(&nt->nt_nodelock);
    OS_RWLOCK_INIT(&nt->nt_wds_nodelock);
#ifdef UMAC_SUPPORT_PROXY_ARP
    TAILQ_INIT(&nt->nt_ipv6_node);
    for (hash = 0; hash < IEEE80211_IPV4_HASHSIZE; hash++) {
        LIST_INIT(&nt->nt_ipv4_hash[hash]);
    }
    OS_RWLOCK_INIT(&nt->nt_ipv4_hash_lock);

    for (hash = 0; hash < IEEE80211_IPV6_HASHSIZE; hash++) {
        LIST_INIT(&nt->nt_ipv6_hash[hash]);
    }
    OS_RWLOCK_INIT(&nt->nt_ipv6_hash_lock);
#endif
    nt->nt_name = name;
     /* Attach soc to node table */
    wlan_objmgr_nt_soc_attach(nt);
    for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++)
        LIST_INIT(&nt->nt_wds_hash[hash]);
    ieee80211_wds_attach(nt);
}

static void
ieee80211_node_table_reset(struct ieee80211_node_table *nt, struct ieee80211vap *match)
{
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    ieee80211_node_table_reset_nolock(nt, match);
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
}

void ieee80211_node_table_reset_nolock_iter_cb(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = NULL;
    struct ieee80211vap *match = (struct ieee80211vap *)arg;
    struct ieee80211_node_table *nt = &ni->ni_ic->ic_sta;

    if ((match != NULL) && (ni->ni_vap != match))
        return;

    if (ni->ni_associd != 0) {
        vap = ni->ni_vap;

#ifdef WLAN_FEATURE_11BE_MLO
        if (!wlan_peer_is_mlo(ni->peer_obj)) {
            if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) || (vap->iv_ic->is_mbssid_enabled)) {
                mlme_free_aid(vap->vdev_obj, ni->ni_associd);
            }
            else if (vap->iv_aid_bitmap != NULL) {
                IEEE80211_AID_CLR(vap, ni->ni_associd);
            }
        }
#else
        if (vap->iv_aid_bitmap != NULL)
            IEEE80211_AID_CLR(vap, ni->ni_associd);
#endif
    }
    /* Remove WDS entries on node table reset.*/
    if (vap != NULL) {
        ieee80211_remove_wds_addr(vap, nt, ni->ni_macaddr,IEEE80211_NODE_F_WDS_BEHIND | IEEE80211_NODE_F_WDS_REMOTE);
    }
    ieee80211_del_wds_node(nt, ni);
    node_reclaim(nt, ni);
}

static void
ieee80211_node_table_reset_nolock(struct ieee80211_node_table *nt, struct ieee80211vap *match)
{
    wlan_mlme_iterate_node_list(nt->nt_ic,
         ieee80211_node_table_reset_nolock_iter_cb, match,
         (IEEE80211_NODE_ITER_F_ASSOC_STA | IEEE80211_NODE_ITER_F_UNASSOC_STA));
}

void
ieee80211_node_attach(struct ieee80211com *ic)
{

    ieee80211_node_table_init(ic, &ic->ic_sta, "station");
#if IEEE80211_DEBUG_NODELEAK
    TAILQ_INIT(&ic->ic_nodes);
    OS_RWLOCK_INIT(&(ic)->ic_nodelock);
#endif
    ic->ic_node_alloc = node_alloc;
    ic->ic_node_free = node_free;
    ic->ic_node_cleanup = node_cleanup;
    ic->ic_node_getsnr = node_getsnr;
    ic->ic_node_authorize = NULL;
}

void
ieee80211_node_detach(struct ieee80211com *ic)
{
    struct ieee80211_node_table *nt = &ic->ic_sta;
    ieee80211_node_table_reset(nt, NULL);
    OS_RWLOCK_DESTROY(&nt->nt_nodelock);
    ieee80211_wds_detach(nt);
#ifdef UMAC_SUPPORT_PROXY_ARP
    OS_RWLOCK_DESTROY(&nt->nt_ipv4_hash_lock);
    OS_RWLOCK_DESTROY(&nt->nt_ipv6_hash_lock);
#endif

}

void
ieee80211_node_vattach(struct ieee80211vap *vap, struct vdev_mlme_obj *vdev_mlme)
{
    struct vdev_mlme_inactivity_params *inactivity_params;

    if (!vdev_mlme) {
        mlme_err(" VDEV MLME component object is NULL");
        return;
    }

    inactivity_params = &vdev_mlme->mgmt.inactivity_params;

    vap->iv_inact_init = IEEE80211_INACT_INIT;
    vap->iv_inact_auth = IEEE80211_INACT_AUTH;
    if (!wlan_get_HWcapabilities(vap->iv_ic, IEEE80211_CAP_PERF_PWR_OFLD)) {
        /* Inactivity Timer units is in terms of 15secs only for DA targets.
         */
        inactivity_params->keepalive_max_unresponsive_time_secs =
                    IEEE80211_INACT_RUN;
    } else {
        /* Update the Inactivity Timer in units of secs for offload targets.
         */
        inactivity_params->keepalive_max_unresponsive_time_secs =
                    DEFAULT_WLAN_VDEV_AP_KEEPALIVE_MAX_UNRESPONSIVE_TIME_SECS;
    }
    vap->iv_inact_probe = IEEE80211_INACT_PROBE;
    vap->iv_session = IEEE80211_SESSION_TIME;
}

void
ieee80211_node_latevdetach(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_vdev_aid_mgr *aid_mgr = NULL;
#endif

    if (!ieee80211_vap_deleted_is_set(vap)) {
        qdf_nofl_info("vap is not deleted by user,"
                      " vap: 0x%pK, vap->iv_bss: 0x%pK\n",
                      vap, vap->iv_bss);
    }

    /*
     * free the aid bitmap.
     */
#ifdef WLAN_FEATURE_11BE_MLO
    if (!(vap->vdev_obj->mlo_dev_ctx) &&  (ic->is_mbssid_enabled)) {
        aid_mgr = wlan_vdev_mlme_get_aid_mgr(vap->vdev_obj);
        if (aid_mgr) {
            wlan_vdev_aid_mgr_free(aid_mgr);
            wlan_vdev_mlme_set_aid_mgr(vap->vdev_obj, NULL);
        }
    } else
#endif
    {
        if (vap->iv_aid_bitmap) {
            if (!wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                    WLAN_PDEV_F_MBSS_IE_ENABLE)) {
                OS_FREE(vap->iv_aid_bitmap);
                vap->iv_aid_bitmap = NULL;
            }

            vap->iv_max_aid = 0;
            vap->iv_mbss_max_aid = 0;
        }
    }
}

void
ieee80211_node_vdetach(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;

    ieee80211_node_table_reset(&ic->ic_sta, vap);
    if (vap->iv_bss != NULL) {
        wlan_objmgr_delete_node(vap->iv_bss);
        vap->iv_bss = NULL;
    }
}

int
ieee80211_node_latevattach(struct ieee80211vap *vap)
{
    int error = 0;
    struct ieee80211com *ic = vap->iv_ic;
    struct mbssid_context *mbss_ctx;
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_vdev_aid_mgr *aid_mgr = NULL;
#endif

    if (ic->is_mbssid_enabled) {
        if (!ic->ic_mbss_ctx) {
            mbss_err("mbss_ctx is null");
            return -EINVAL;
        }
        mbss_ctx = ic->ic_mbss_ctx;
    }
    /*
     * Allocate these only if needed.  Beware that we
     * know adhoc mode doesn't support ATIM yet...
     */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP || \
        vap->iv_opmode == IEEE80211_M_BTAMP  || \
        vap->iv_opmode == IEEE80211_M_IBSS) {
        unsigned long bm_size;

        KASSERT(vap->iv_max_aid != 0, ("0 max aid"));

        bm_size = howmany(vap->iv_max_aid,
                          sizeof(unsigned long) * BITS_PER_BYTE);

        if (!ic->is_mbssid_enabled) {
            vap->iv_aid_bitmap = qdf_mem_malloc(bm_size *
                                              sizeof(unsigned long));
            if (vap->iv_aid_bitmap == NULL) {
                /* XXX no way to recover */
                qdf_warn("no memory for AID bitmap!");
                vap->iv_max_aid = 0;
                vap->iv_mbss_max_aid = 0;
                return -ENOMEM;
            }
        } else {
            /* Non-tx VAPs share AID map of tx VAP */
            struct ieee80211vap *tx_vap = mbss_ctx->ic_mbss[vap->iv_mbss.group_id].transmit_vap;

            KASSERT(vap->iv_mbss_max_aid != 0, ("0 mbss max aid"));
#ifdef WLAN_FEATURE_11BE_MLO
            if (!tx_vap) {
                /* Initialize AID mgr for all VAPs */
                aid_mgr = wlan_vdev_mlme_get_aid_mgr(vap->vdev_obj);
                if (!aid_mgr) {
                    aid_mgr = wlan_vdev_aid_mgr_init(vap->iv_max_aid);
                    if (!aid_mgr) {
                        mbss_err("AID bitmap allocation failed for VDEV%d",
                                 wlan_vdev_get_id(vap->vdev_obj));
                        return -ENOMEM;
                    }
                    wlan_vdev_mlme_set_aid_mgr(vap->vdev_obj, aid_mgr);
                }
            }
#endif

            if ((IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) && (tx_vap)) {
#ifndef WLAN_FEATURE_11BE_MLO
                if (!tx_vap->iv_aid_bitmap)
                    return -EINVAL;

                vap->iv_aid_bitmap   = tx_vap->iv_aid_bitmap;
#else
                mbss_info("mlo enabled for vdev %d, is_mlo_ap %d, %d",
                          wlan_vdev_get_id(vap->vdev_obj),
                          wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj),
                          vap->vdev_obj->mlo_dev_ctx?1:0);
                if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) ||
                    vap->vdev_obj->mlo_dev_ctx) {
                     wlan_mlo_vdev_init_mbss_aid_mgr(vap->vdev_obj->mlo_dev_ctx,
                                                     vap->vdev_obj,
                                                     tx_vap->vdev_obj);
                }
                else {
                     wlan_mlme_vdev_init_mbss_aid_mgr(vap->vdev_obj,
                                                      tx_vap->vdev_obj);
                }
#endif
                vap->iv_max_aid = tx_vap->iv_max_aid;

                /*
                 * iv_mbss_max_aid should follow iv_max_aid unconditionally
                 * during vattach.
                 */
                vap->iv_mbss_max_aid = vap->iv_max_aid;
            }
        }
    }

    error = ieee80211_reset_bss(vap);
    return error;
}

/*
 * Add the specified station to the station table.
 * calls alloc_node and hence return the node with 2 references.
 * one for adding it to the table and the
 * the other for the caller to use.
 */
struct ieee80211_node *
ieee80211_dup_bss(struct ieee80211vap *vap, const u_int8_t *macaddr)
{
    struct ieee80211_node *ni;

    /* Allocate station peer object */
    ni = wlan_objmgr_alloc_sta_node(vap, (uint8_t *)macaddr);
    if (ni != NULL) {
        /*
         * Inherit from iv_bss.
         */
        ni->ni_authmode = vap->iv_bss->ni_authmode;
        ni->ni_txpower = vap->iv_bss->ni_txpower;
        IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_bss->ni_bssid);
        ni->ni_bss_node = ieee80211_try_ref_bss_node(vap, WLAN_MLME_OBJMGR_ID);
        if (!ni->ni_bss_node) {
            IEEE80211_NODE_LEAVE(ni);
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
            return NULL;
        }
        ni->ni_assocuptime = OS_GET_TICKS();
    }
    return ni;
}

/*
 * Add the MLO bridge peer to the station table.
 * calls alloc_node and hence return the node with 2 references.
 * one for adding it to the table and the
 * the other for the caller to use.
 */
struct ieee80211_node *
ieee80211_mlo_bridge_node(struct ieee80211vap *vap, const u_int8_t *macaddr)
{
    struct ieee80211_node *ni;

    /* Allocate MLO bridge peer object */
    ni = wlan_objmgr_alloc_mlo_bridge_node(vap, (uint8_t *)macaddr);
    if (ni != NULL) {
        /*
         * Inherit from iv_bss.
         */
        ni->ni_authmode = vap->iv_bss->ni_authmode;
        ni->ni_txpower = vap->iv_bss->ni_txpower;
        IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_bss->ni_bssid);
        ni->ni_bss_node = ieee80211_try_ref_bss_node(vap, WLAN_MLME_OBJMGR_ID);
        if (!ni->ni_bss_node) {
            IEEE80211_NODE_LEAVE(ni);
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
            return NULL;
        }
        ni->ni_assocuptime = OS_GET_TICKS();
        qdf_info("MLO Bridge peer allocated %s", ether_sprintf(ni->ni_macaddr));
    }
    return ni;
}

#if IEEE80211_DEBUG_NODELEAK
void
ieee80211_dump_alloc_nodes(struct ieee80211com *ic)
{
    struct ieee80211_node *ni;
    u_int8_t  ssid[IEEE80211_NWID_LEN+4];
    rwlock_state_t lock_state;
    ieee80211_node_saveq_info qinfo;

    ieee80211com_note(ic, IEEE80211_MSG_NODE, "dumping all allocated nodes ... \n");
    OS_RWLOCK_READ_LOCK(&ic->ic_nodelock,&lock_state);
    TAILQ_FOREACH(ni, &ic->ic_nodes, ni_alloc_list) {
        ieee80211com_note(ic, IEEE80211_MSG_NODE, "node 0x%x mac %s  tmpnode: %d"
               " nodetable : %d flags 0x%x refcount: %d ", ni,
               ether_sprintf(ni->ni_macaddr),
               (ni->ni_flags & IEEE80211_NODE_TEMP) ? 1 : 0,
               (ni->ni_table) ? 1 : 0,ni->ni_flags, ieee80211_node_refcnt(ni));
        if (ni->ni_esslen) {
            OS_MEMCPY(ssid, ni->ni_essid, ni->ni_esslen);
            ssid[ni->ni_esslen] = 0;
        }
        ieee80211_node_saveq_get_info(ni, &qinfo);
        ieee80211com_note(ic, IEEE80211_MSG_NODE,
               "bssid %s cap 0x%x dqlen  %d mgtqlen %d  %s %s \n",
               ether_sprintf(ni->ni_bssid), ni->ni_capinfo,
                          qinfo.data_count, qinfo.mgt_count,
               ni->ni_esslen ? "ssid ":"",
               ni->ni_esslen ? (char *)ssid : "" );
        if (ic->ic_print_nodeq_info)
            ic->ic_print_nodeq_info(ni);
    }
    OS_RWLOCK_READ_UNLOCK(&ic->ic_nodelock,&lock_state);
}

void
wlan_dump_alloc_nodes(wlan_dev_t devhandle)
{
    struct ieee80211com *ic = (struct ieee80211com *) devhandle;
    ieee80211_dump_alloc_nodes(ic);
}
#endif

/* External UMAC APIs */
u_int16_t wlan_node_getpwrcapinfo(wlan_node_t node)
{
    return ((node->ni_min_txpower & 0xFF)|(node->ni_max_txpower<<8 & 0xFF00));
}

u_int16_t wlan_node_getcapinfo(wlan_node_t node)
{
    return node->ni_capinfo;
}

u_int32_t wlan_node_get_extended_capabilities(wlan_node_t node)
{
    return node->ext_caps.ni_ext_capabilities;
}

u_int32_t wlan_node_get_extended_capabilities2(wlan_node_t node)
{
    return node->ext_caps.ni_ext_capabilities2;
}

u_int32_t wlan_node_get_extended_capabilities3(wlan_node_t node)
{
    return node->ext_caps.ni_ext_capabilities3;
}

u_int32_t wlan_node_get_extended_capabilities4(wlan_node_t node)
{
    return node->ext_caps.ni_ext_capabilities4;
}

int  wlan_node_getwpaie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_wpa_ie != NULL) {
        int ielen = ni->ni_wpa_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_wpa_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;

}

int  wlan_node_getwpsie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_wps_ie != NULL) {
        int ielen = ni->ni_wps_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_wps_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;

}

int  wlan_node_getathie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_ath_ie != NULL) {
        int ielen = ni->ni_ath_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_ath_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;

}

int  wlan_node_getwmeie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_wme_ie != NULL) {
        int ielen = ni->ni_wme_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_wme_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;

}

int wlan_node_get_suppchanie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;

    if (ni->ni_supp_chan_ie != NULL) {
        int ielen = ni->ni_supp_chan_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_supp_chan_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;
}

int wlan_node_get_opclassie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;

    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;

    if (ni->ni_supp_op_class_ie != NULL) {
        int ielen = ni->ni_supp_op_class_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_supp_op_class_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;
}

static void ieee80211_node_iter(struct wlan_objmgr_vdev *vdev,
                                void *object, void *arg)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
    struct ieee80211_node *ni;
    struct ieee80211_iter_arg *itr_arg = (struct ieee80211_iter_arg *)arg;
    struct ieee80211vap *vap;
    struct ieee80211com *ic;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)  {
         qdf_warn("ni unavailable for peer %pK", peer);
         return;
    }
    vap = ni->ni_vap;
    ic = ni->ni_ic;

    /*
     * ignore if the node does not belong to the requesting vap.
     */
    if (vap != itr_arg->vap)  {
         return;
    }

    /*
     * ignore BSS node for AP/IBSS mode
     */
    if (((ni == ni->ni_bss_node) &&
        ((vap->iv_opmode == IEEE80211_M_HOSTAP) ||
         (vap->iv_opmode == IEEE80211_M_IBSS) ||
         (vap->iv_opmode == IEEE80211_M_MONITOR) ||
         (vap->iv_opmode == IEEE80211_M_BTAMP))) ||
         (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_TEMP)) {
        return;
    }

    /*
     * Ignore the self node for Station mode
     * This is required to skip the self node in case of when station vap
     * is not connected to AP
     */
    if ((vap->iv_opmode == IEEE80211_M_STA) &&
        IEEE80211_ADDR_EQ(vap->iv_myaddr, ni->ni_macaddr)) {
        return;
    }

    if (!(itr_arg->flag & IEEE80211_NODE_ITER_F_UNASSOC_STA)) {
        /*
         * ignore un associated stations for AP mode
         */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && (ni->ni_associd == 0)) {
            return;
        }

    }

    if (!(itr_arg->flag & IEEE80211_NODE_ITER_F_ASSOC_STA)) {
        /*
         * ignore associated stations for AP mode
         */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && (ni->ni_associd != 0)) {
            return;
        }

       if ((itr_arg->flag & IEEE80211_NODE_ITER_F_PASN_PEERS) &&
           (wlan_peer_get_peer_type(ni->peer_obj) != WLAN_PEER_RTT_PASN)) {
           return;
       }
    }

    if ((ic != NULL) && (itr_arg->count < ic->ic_num_clients)) {
        /* increment the ref count so that the node is not freed */
        if ((ni = ieee80211_try_ref_node(ni, WLAN_MLME_SB_ID)) != NULL) {
            itr_arg->nodes[itr_arg->count] = ni;
            ++itr_arg->count;
        }
    }

}


static int32_t
ieee80211_iterate_node_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg, u_int32_t flag)
{
  struct ieee80211com *ic = vap->iv_ic;
  struct ieee80211_iter_arg *itr_arg = NULL;
  int i, count;

  itr_arg = (struct ieee80211_iter_arg *)qdf_mem_malloc(sizeof(struct ieee80211_iter_arg));
  if (itr_arg == NULL) {
          return -1;
  }

  itr_arg->count=0;
  itr_arg->vap=vap;
  itr_arg->flag=flag;

  /*
   * we can not call the call back function iter_func from the ieee80211_sta_iter.
   * because the ieee80211_iter is called with nt lock held and will result in
   * dead lock if the implementation of iter_func calls bcak into umac to query more
   * info about the node (which is more likely).
   * instaed the ieee80211_sta_iter collects all the nodes in to the nodes array
   * part of the itr_arg and also increments the ref count on these nodes so that
   * they wont get freed.
   */

  wlan_objmgr_iterate_peerobj_list(vap->vdev_obj, ieee80211_node_iter,
                                   (void *)itr_arg, WLAN_MLME_SB_ID);
  for (i = 0;i < itr_arg->count; ++i)
  {
      if (i == ic->ic_num_clients) break;
      if (iter_func) {
          /*
           * node has been refed in ieee80211_sta_iter
           * so safe to acces the contentes of the node.
           */
          (* iter_func) (arg, itr_arg->nodes[i]);
      }
      /* decrement the ref count which is incremented above in ieee80211_sta_iter */
      ieee80211_free_node(itr_arg->nodes[i], WLAN_MLME_SB_ID);
  }
  count = itr_arg->count;
  qdf_mem_free(itr_arg);
  return (count);
}

int32_t wlan_iterate_all_sta_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg)
{
    return ieee80211_iterate_node_list(vap, iter_func, arg,
                                       IEEE80211_NODE_ITER_F_ASSOC_STA |
                                       IEEE80211_NODE_ITER_F_UNASSOC_STA);
}

int32_t wlan_iterate_station_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg)
{
    return ieee80211_iterate_node_list(vap, iter_func, arg,
                                       IEEE80211_NODE_ITER_F_ASSOC_STA);
}

int32_t wlan_iterate_unassoc_sta_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg)
{
    return ieee80211_iterate_node_list(vap, iter_func, arg,
                                       IEEE80211_NODE_ITER_F_UNASSOC_STA);
}

int32_t wlan_iterate_pasn_peer_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg)
{
    /*
     * PASN peers are unassoc STAs. Hence, pass UNASSOC_STA
     * along with the PASN_PEERS flag to get the PASN peers.
     */
    return ieee80211_iterate_node_list(vap, iter_func, arg,
                                       IEEE80211_NODE_ITER_F_UNASSOC_STA | IEEE80211_NODE_ITER_F_PASN_PEERS);
}

int wlan_node_txrate_info(wlan_node_t node, ieee80211_rate_info *rinfo)
{
    u_int8_t rc;
    rinfo->rate = node->ni_ic->ic_node_getrate(node, IEEE80211_RATE_TX);
    rinfo->avg_rate = node->ni_ic->ic_node_getrate(node, IEEE80211_AVG_RATE_TX);
    rinfo->lastrate = node->ni_ic->ic_node_getrate(node, IEEE80211_LASTRATE_TX);
    rc = (u_int8_t) node->ni_ic->ic_node_getrate(node, IEEE80211_RATECODE_TX);
    rinfo->mcs = rc;
    rinfo->type = (rinfo->mcs & 0x80)? IEEE80211_RATE_TYPE_MCS : IEEE80211_RATE_TYPE_LEGACY;
    rinfo->maxrate_per_client = node->ni_ic->ic_get_maxphyrate(node->ni_ic, node);
    rinfo->flags = (u_int8_t) node->ni_ic->ic_node_getrate(node, IEEE80211_RATEFLAGS_TX);
    return 0;
}

int wlan_node_rxrate_info(wlan_node_t node, ieee80211_rate_info *rinfo)
{
    u_int8_t rc;
    rinfo->rate = node->ni_ic->ic_node_getrate(node, IEEE80211_RATE_RX);
    rinfo->avg_rate = node->ni_ic->ic_node_getrate(node, IEEE80211_AVG_RATE_RX);
    rinfo->lastrate = node->ni_ic->ic_node_getrate(node, IEEE80211_LASTRATE_RX);
    rc = (u_int8_t) node->ni_ic->ic_node_getrate(node, IEEE80211_RATECODE_RX);
    rinfo->mcs = rc;
    rinfo->type = (rinfo->mcs & 0x80)? IEEE80211_RATE_TYPE_MCS : IEEE80211_RATE_TYPE_LEGACY;
    rinfo->flags = (u_int8_t) node->ni_ic->ic_node_getrate(node, IEEE80211_RATEFLAGS_RX);
    return 0;
}

int wlan_node_getsnr(wlan_node_t node,wlan_snr_info *snr_info,  wlan_snr_type snr_type )
{

    int chain_ix;
    int8_t avg_snr = 0;
    u_int8_t flags=0;
    struct ieee80211_node *ni = node;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = vap->iv_ic;

    if (snr_type == WLAN_SNR_TX)
        flags = IEEE80211_SNR_TX;
    else if (snr_type == WLAN_SNR_RX)
        flags = IEEE80211_SNR_RX;
    else if (snr_type == WLAN_SNR_BEACON)
        flags = IEEE80211_SNR_BEACON;
    else if (snr_type == WLAN_SNR_RX_DATA)
        flags = IEEE80211_SNR_RXDATA;

    if (snr_type == WLAN_SNR_TX) {
        snr_info->valid_mask = ic->ic_tx_chainmask;
    } else {
        snr_info->valid_mask = ic->ic_rx_chainmask;
    }

    avg_snr = ic->ic_node_getsnr(ni,-1,flags);
    snr_info->avg_snr = (avg_snr == -1) ? 0 : avg_snr;
    for(chain_ix=0;chain_ix<MAX_CHAINS; ++chain_ix) {
        snr_info->snr_ctrl[chain_ix] = ic->ic_node_getsnr(ni, chain_ix ,flags);
    }
    flags |= IEEE80211_SNR_EXTCHAN;
    for(chain_ix=0;chain_ix<MAX_CHAINS; ++chain_ix) {
        snr_info->snr_ext[chain_ix] = ic->ic_node_getsnr(ni, chain_ix ,flags);
    }
    return 0;

}

u_int8_t *wlan_node_getmacaddr(wlan_node_t node)
{
    return node->ni_macaddr;
}

u_int8_t *wlan_node_getbssid(wlan_node_t node)
{
    return node->ni_bssid;
}

u_int32_t wlan_node_set_assoc_decision(wlan_if_t vap, u_int8_t *macaddr, u_int16_t assoc_status, u_int16_t p2p_assoc_status)
{

    struct ieee80211_node *ni;
    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;
    ni->ni_assocstatus = assoc_status;
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;
}

u_int32_t wlan_node_get_assoc_decision(wlan_if_t vap, u_int8_t *macaddr)
{

    struct ieee80211_node *ni;
    u_int32_t assocstatus;

    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return EINVAL;
    assocstatus = ni->ni_assocstatus;
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return (assocstatus);
}

QDF_STATUS
wlan_node_get_chan_params(wlan_node_t node, struct ni_chan_param *ni_param)
{
    if (!node)
        return QDF_STATUS_E_INVAL;

    if (!node->ni_ch_param.ni_freq)
        return QDF_STATUS_E_INVAL;

    ni_param->ni_freq = node->ni_ch_param.ni_freq;
    ni_param->ni_cfreq1 = node->ni_ch_param.ni_cfreq1;
    ni_param->ni_cfreq2 = node->ni_ch_param.ni_cfreq2;
    ni_param->ni_bss_phymode = node->ni_ch_param.ni_bss_phymode;
#ifdef WLAN_FEATURE_11BE
    ni_param->puncture_bitmap = node->ni_ch_param.puncture_bitmap;
#endif

    return QDF_STATUS_SUCCESS;
}

u_int32_t wlan_node_get_state_flag(wlan_node_t node)
{
    return node->ni_flags;
}

u_int8_t wlan_node_get_authmode(wlan_node_t node)
{
    return node->ni_authmode;
}

u_int8_t wlan_node_get_ath_flags(wlan_node_t node)
{
    return node->ni_ath_flags;
}

u_int8_t wlan_node_get_erp(wlan_node_t node)
{
    return node->ni_erp;
}

systick_t wlan_node_get_assocuptime(wlan_node_t node)
{
    return node->ni_assocuptime;
}

u_int16_t wlan_node_get_associd(wlan_node_t node)
{
    return ieee80211_node_get_associd((struct ieee80211_node *)node);
}

u_int16_t wlan_node_get_txpower(wlan_node_t node)
{
    return ieee80211_node_get_txpower((struct ieee80211_node *)node);
}

u_int16_t wlan_node_get_vlan(wlan_node_t node)
{
    return node->ni_vlan;
}

u_int8_t wlan_node_get_operating_bands(wlan_node_t node)
{
	return node->ni_operating_bands;
}

int
wlan_node_get_ucast_ciphers(wlan_node_t node, ieee80211_cipher_type types[], u_int len)
{
    ieee80211_cipher_type cipher;
    int ucast_cipher = wlan_crypto_get_peer_param(node->peer_obj, WLAN_CRYPTO_PARAM_UCAST_CIPHER);
    u_int count = 0;
    for (cipher = IEEE80211_CIPHER_WEP;  cipher < IEEE80211_CIPHER_MAX ; cipher++) {

        if (len <= count)
                return count;

        if ((ucast_cipher >> cipher) & 0x01) {
                types[count] = cipher;
                count++;
        }
    }
    return count ;
}

void  wlan_node_get_txseqs(wlan_node_t node, u_int16_t *txseqs, u_int len)
{
    struct ieee80211_node *ni = node;

    if (len > sizeof(ni->ni_txseqs)) {
        len = sizeof(ni->ni_txseqs);
    }
    OS_MEMCPY(txseqs, ni->ni_txseqs, len);
}

void  wlan_node_get_rxseqs(wlan_node_t node, u_int16_t *rxseqs, u_int len)
{
    struct ieee80211_node *ni = node;

    if (len > sizeof(ni->ni_rxseqs)) {
        len = sizeof(ni->ni_rxseqs);
    }
    OS_MEMCPY(rxseqs, ni->ni_rxseqs, len);
}

u_int8_t wlan_node_get_uapsd(wlan_node_t node)
{
    return node->ni_uapsd;
}

u_int16_t wlan_node_get_inact(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    struct vdev_mlme_inactivity_params *inactivity_params;
    u_int16_t inact_time;

    inactivity_params = &ni->ni_vap->vdev_mlme->mgmt.inactivity_params;

    /* NB: leave all cases in case we relax ni_associd == 0 check */
    if (ieee80211_node_is_authorized(ni)) {
        inact_time = inactivity_params->keepalive_max_unresponsive_time_secs;
    } else if (ni->ni_associd != 0) {
        inact_time = ni->ni_vap->iv_inact_auth;
    } else {
        inact_time = ni->ni_vap->iv_inact_init;
    }
    inact_time = (inact_time - ni->ni_inact) * IEEE80211_INACT_WAIT;

    return inact_time;
}

u_int16_t wlan_node_get_htcap(wlan_node_t node)
{
    return node->ni_htcap;
}

u_int32_t wlan_node_get_vhtcap(wlan_node_t node)
{
    return node->ni_vhtcap;
}

u_int32_t wlan_node_get_chwidth(wlan_node_t node)
{
    return node->ni_chwidth;
}

bool wlan_node_has_flag(struct ieee80211_node *ni, u_int16_t flag)
{
    return (ieee80211node_has_flag(ni, flag));
}

bool wlan_node_has_extflag(struct ieee80211_node *ni, u_int16_t flag)
{
    return (ieee80211node_has_extflag(ni, flag));
}

u_int16_t wlan_node_get_phymodes(struct ieee80211_node *ni)
{
    return ieee80211node_get_phymodes(ni);
}

u_int16_t wlan_node_get_mode(wlan_node_t node)
{

	return wlan_node_get_phymodes(node);
}

/* To check if WEP/TKIP Aggregation can be enabled for this node. */
int
ieee80211_has_weptkipaggr(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;

    /* Both the peer node and our hardware must support aggregation during wep/tkip */
    if ((ieee80211node_has_flag(ni, IEEE80211_NODE_WEPTKIPAGGR)) &&
        ieee80211com_has_athextcap(ic, IEEE80211_ATHEC_WEPTKIPAGGR)) {
        return 1;
    }
    return 0;
}


void wlan_node_set_txpwr(wlan_if_t vap, u_int16_t txpowlevel, u_int8_t *addr)
{
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(vap->iv_ic, addr, WLAN_MLME_SB_ID);
    ASSERT(ni);
    if (!ni)
        return;
    ieee80211node_set_txpower(ni, txpowlevel);
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
}

int wlan_node_alloc_aid_bitmap(wlan_if_t vap, u_int16_t old_len)
{
    u_int8_t    *bitmap = NULL;
    u_int16_t   len = howmany(vap->iv_max_aid,
			      sizeof(unsigned long) * BITS_PER_BYTE);
#ifdef WLAN_FEATURE_11BE_MLO
    struct wlan_vdev_aid_mgr *aid_mgr;
    uint16_t start_aid;
#endif

#ifdef WLAN_FEATURE_11BE_MLO
    if (!((wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj)) ||
          vap->iv_ic->is_mbssid_enabled)) {
#else
      /* for NON-MLO builds, enable below code by default */
    {
#endif
        //qdf_nofl_info("[%s] entry\n",__func__);
        bitmap = qdf_mem_malloc(len * sizeof(unsigned long));
        if(!bitmap) {
            vap->iv_max_aid = old_len;
            return -1;
        }
        if (vap->iv_aid_bitmap) {
            OS_MEMCPY(bitmap, vap->iv_aid_bitmap, len > old_len ? old_len : len);
            OS_FREE(vap->iv_aid_bitmap);
        }
        vap->iv_aid_bitmap = (unsigned long *)bitmap;
    }
#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_vdev_mlme_is_mlo_ap(vap->vdev_obj) || vap->iv_ic->is_mbssid_enabled) {
        aid_mgr = wlan_vdev_mlme_get_aid_mgr(vap->vdev_obj);
        if (aid_mgr) {
            wlan_vdev_mlme_aid_mgr_max_aid_set(vap->vdev_obj, vap->iv_max_aid);
            start_aid = ieee80211_get_vap_start_aid(vap->iv_ic, vap);
            wlan_vdev_mlme_set_start_aid(vap->vdev_obj, start_aid);
        }
        qdf_debug("aid:%d, iv_max_aid:%d, vap:%s",start_aid, vap->iv_max_aid, vap->iv_netdev_name);
    }
#endif
    //qdf_nofl_info("[%s] exist\n",__func__);

    return 0;
}

int wlan_send_rssi(struct ieee80211vap *vap, u_int8_t *macaddr)
{
    struct ieee80211com *ic = vap->iv_ic;
    if (ic->ic_ath_send_rssi)
        ic->ic_ath_send_rssi(ic, macaddr, vap);
    return 0;
}

int wlan_node_set_fixed_rate(wlan_node_t node, u_int32_t rate)
{
    struct ieee80211_node *ni = node;
    struct ieee80211com *ic = ni->ni_vap->iv_ic;
    ni->ni_fixed_rate = rate;
    if (ic->ic_set_sta_fixed_rate) {
        ic->ic_set_sta_fixed_rate(ni);
    }
    return 0;
}

u_int8_t wlan_node_get_fixed_rate(wlan_node_t node)
{
    return node->ni_fixed_rate;
}

#define NSS_RX_SHIFT 4
u_int8_t wlan_node_get_configured_nss(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    return ((ni->ni_streams << NSS_RX_SHIFT) | ni->ni_streams);
}

u_int8_t wlan_node_get_nss(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    return ((ni->ni_rxstreams << NSS_RX_SHIFT) | ni->ni_txstreams);
}

u_int8_t wlan_node_get_nss_capability(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    if (ni->ni_vap->iv_nawds.mode == IEEE80211_NAWDS_DISABLED)
        return ((ni->ni_maxrxstreams << NSS_RX_SHIFT) | ni->ni_maxtxstreams);
    return wlan_node_get_configured_nss(node);
}
#undef NSS_RX_SHIFT

u_int8_t wlan_node_get_max_nss(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    return MAX(ni->ni_txstreams, ni->ni_rxstreams);
}
qdf_export_symbol(wlan_node_get_max_nss);

u_int8_t wlan_node_get_256qam_support(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    u_int8_t is_256qam = 0;
    is_256qam = (ni->ni_flags & IEEE80211_NODE_VHT) ? 1 : 0;

    return (is_256qam);
}

u_int32_t wlan_node_get_last_txpower(wlan_node_t node)
{
    return node->ni_ic->ic_node_get_last_txpower(node);
}

/*
 * call back for node peer delete
 */
int wlan_node_peer_delete_response_handler(struct ieee80211vap *vap, struct ieee80211_node *ni)
{

    if (!vap) {
        qdf_print("%s null vap, node: %s  ", __func__, ether_sprintf(ni->ni_macaddr));
        return -EINVAL;
    }
    IEEE80211_NOTE(vap, IEEE80211_MSG_PEER_DELETE, ni,
            "%s for node: ni: 0x%pK", __func__, ni);

    /*
     * call required clean up handlers for peer delete completion
     */
    if (ieee80211_vap_deleted_is_clear(vap) && vap->iv_bss) {
        switch (vap->iv_opmode) {
            case IEEE80211_M_HOSTAP:
                if ((qdf_mem_cmp(&ni->ni_macaddr, &vap->iv_bss->ni_macaddr,
                                QDF_MAC_ADDR_SIZE) != 0)) {
                    mlme_auth_peer_delete_handler(vap, ni);
                }
                break;
            default:
                break;
        }
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_PEER_DELETE,
                "%s: vap->iv_bss: 0x%pK vap_deleted_clr: %d\n",
                __func__, vap->iv_bss, ieee80211_vap_deleted_is_clear(vap));
    }
    return 0;
}

bool
is_node_self_peer(struct ieee80211vap *vap, const uint8_t *macaddr)
{
    bool is_self_peer = FALSE;

    switch (vap->iv_opmode) {
    case IEEE80211_M_STA:
        if (IEEE80211_ADDR_EQ(macaddr, vap->iv_myaddr)) {
            is_self_peer = TRUE;
        }
        break;
    default:
        break;
    }

    return is_self_peer;
}

#if MESH_MODE_SUPPORT
void ieee80211_check_timeout_mesh_peer_iter_cb(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = (struct ieee80211vap *)arg;
    u_int8_t  macaddr[QDF_MAC_ADDR_SIZE];

    if (ni->ni_ext_flags & IEEE80211_LOCAL_MESH_PEER) {
        ni->ni_meshpeer_timeout_cnt++;
        if (ni->ni_meshpeer_timeout_cnt > IEEE80211_MESH_PEER_TIMEOUT_CNT) {
            qdf_mem_copy(macaddr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_MESH,
                      "%s: [0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x] mesh peer timed out.\n",
                       __func__, macaddr[0],macaddr[1],macaddr[2],
                       macaddr[3],macaddr[4],macaddr[5]);
            ni->ni_meshpeer_timeout_cnt  = 0;
            /*send event to user app, let user decide what to do*/
            IEEE80211_DELIVER_EVENT_SESSION_TIMEOUT(vap, macaddr);
        }
    }

}

/*
   if no beacon received beyond some time from mesh peer, timeout the mesh peer,
   send event to user app, let user decide what to do.
 */
void ieee80211_check_timeout_mesh_peer(void *arg, wlan_if_t vaphandle)
{
    if (vaphandle->iv_mesh_vap_mode)
        wlan_mlme_iterate_node_list(vaphandle->iv_ic,
                  ieee80211_check_timeout_mesh_peer_iter_cb, (void *)vaphandle,
                  IEEE80211_NODE_ITER_F_ASSOC_STA);

}
#endif  /*MESH_MODE_SUPPORT*/


#ifdef AST_HKV1_WORKAROUND
/*
 * call back for node peer delete
 */
int wlan_wds_delete_response_handler(struct wlan_objmgr_psoc *psoc_obj,
				     struct recv_auth_params_defer *auth_params,
				     enum wds_auth_defer_action action)
{
    if ((!psoc_obj || (action == IEEE80211_AUTH_ABORT)) && auth_params) {
        if(auth_params->wbuf)
            wbuf_free(auth_params->wbuf);
        qdf_mem_free(auth_params);
	return -1;
    }
    return mlme_auth_wds_delete_resp_handler(psoc_obj, auth_params);
}
#endif

void
ieee80211_update_ack_rssi(struct ieee80211_node *ni, int rssi)
{
#define MAX_ACK_RSSI_AGE 0xff
    unsigned long age;

    if (rssi & 0x80)
        return;

    age = jiffies - ni->ni_last_ack_jiffies;
    age = jiffies_to_msecs(age);

    if (age > MAX_ACK_RSSI_AGE)
        age = MAX_ACK_RSSI_AGE;

    ni->ni_last_ack_rssi = (ni->ni_last_ack_rssi << 8) | rssi;
    ni->ni_last_ack_age = (ni->ni_last_ack_age << 8) | age;
    ni->ni_last_ack_jiffies = jiffies;
    ni->ni_last_ack_cnt = ni->ni_last_ack_cnt + 1;

    return;
}
qdf_export_symbol(ieee80211_update_ack_rssi);

static
void ieee80211_validate_aid_iter_cb(void *arg, struct ieee80211_node *ni)
{
   struct ieee80211_find_arg *find_arg = (struct ieee80211_find_arg *) arg;
   uint16_t aid = 0;

   /**
    * If node with the specified AID value has been found,
    *  no need to check for remaining nodes.
    */
   if (find_arg->result)
       return;

   aid = IEEE80211_AID(ni->ni_associd);

   if (aid == find_arg->value)
       find_arg->result = TRUE;

}

bool ieee80211_validate_aid(struct ieee80211com *ic, uint32_t value)
{
    struct ieee80211_find_arg find_arg = { value, FALSE };

    wlan_mlme_iterate_node_list(ic, ieee80211_validate_aid_iter_cb,
                                (void *)&find_arg,
                                IEEE80211_NODE_ITER_F_ASSOC_STA);

    return find_arg.result;
}
qdf_export_symbol(ieee80211_validate_aid);

int ieee80211_node_set_kickout(struct ieee80211vap *vap, u_int8_t *mac, bool allow)
{
    struct ieee80211_node *ni;

    ni =  ieee80211_vap_find_node(vap, mac, WLAN_MLME_SB_ID);
    if (ni == NULL)
        return -EINVAL;

    ni->ni_kickout_disallow = !allow;
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);

    return 0;
}

#ifdef WLAN_SUPPORT_SCS
static inline
uint8_t ieee80211_find_dl_svc_node_free_index(struct ieee80211_node *ni)
{
    uint8_t i;

    for (i = 0; i < MAX_SCS_REQUEST_PER_DIRECTION; i++) {
        if (!ni->dl_info.dl_node[i].svc_id)
            return i;
    }

    return MAX_SCS_REQUEST_PER_DIRECTION;
}

uint8_t ieee80211_find_dl_svc_node_index_nolock(struct ieee80211_node *ni,
                                                uint8_t svc_id)
{
    uint8_t i;

    for (i = 0; i < MAX_SCS_REQUEST_PER_DIRECTION; i++) {
        if (ni->dl_info.dl_node[i].svc_id &&
            (ni->dl_info.dl_node[i].svc_id == svc_id))
            return i;
    }

    return MAX_SCS_REQUEST_PER_DIRECTION;
}

qdf_export_symbol(ieee80211_find_dl_svc_node_index_nolock);

uint8_t ieee80211_find_dl_svc_node_index_lock(struct ieee80211_node *ni,
                                              uint8_t svc_id)
{
    uint8_t i;

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    i = ieee80211_find_dl_svc_node_index_nolock(ni, svc_id);
    qdf_spin_unlock_bh(&ni->ni_scs_lock);

    return i;
}

qdf_export_symbol(ieee80211_find_dl_svc_node_index_lock);

QDF_STATUS ieee80211_add_dl_svc_info(struct ieee80211_node *ni, uint8_t svc_id)
{
    uint8_t idx;
    QDF_STATUS status = QDF_STATUS_E_FAILURE;

    qdf_spin_lock_bh(&ni->ni_scs_lock);

    /* Check if svc_id already exists */
    idx = ieee80211_find_dl_svc_node_index_nolock(ni, svc_id);

    /* If svc_id does not exists, then check for free entry */
    if (idx == MAX_SCS_REQUEST_PER_DIRECTION)
        idx = ieee80211_find_dl_svc_node_free_index(ni);

    /* If existing entry/free entry is found */
    if (idx != MAX_SCS_REQUEST_PER_DIRECTION) {
        /* Increment ref_count by 1 */
        ni->dl_info.dl_node[idx].ref_cnt++;

        ni->dl_info.dl_node[idx].svc_id = svc_id;

        status = QDF_STATUS_SUCCESS;

    }
    qdf_spin_unlock_bh(&ni->ni_scs_lock);
    return status;
}

qdf_export_symbol(ieee80211_add_dl_svc_info);

QDF_STATUS ieee80211_delete_dl_svc_info(struct ieee80211_node *ni,
                                        uint8_t svc_id)
{
    uint8_t idx;
    QDF_STATUS status = QDF_STATUS_E_INVAL;

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    idx = ieee80211_find_dl_svc_node_index_nolock(ni, svc_id);

    if (idx != MAX_SCS_REQUEST_PER_DIRECTION) {
        /* Decrement ref_count by 1 if svc_id is found */
        if (ni->dl_info.dl_node[idx].ref_cnt > 0)
            ni->dl_info.dl_node[idx].ref_cnt--;

        /* Reset svc_id when ref_count reaches 0 */
        if (!ni->dl_info.dl_node[idx].ref_cnt)
            ni->dl_info.dl_node[idx].svc_id = 0;

        status = QDF_STATUS_SUCCESS;
    }

    qdf_spin_unlock_bh(&ni->ni_scs_lock);
    return status;
}

qdf_export_symbol(ieee80211_delete_dl_svc_info);
#endif /* WLAN_SUPPORT_SCS */

#if (defined(WLAN_SUPPORT_SCS) && defined(WLAN_FEATURE_11BE)) || defined(CONFIG_SAWF)

#define IEEE80211_SCS_SVC_ID_SOC_ID_SHIFT  24
#define IEEE80211_SCS_SVC_ID_PEER_ID_SHIFT 8
#define IEEE80211_SCS_SVC_ID_INDEX_SHIFT   0

#define IEEE80211_SCS_SVC_ID_INDEX_MASK    0xFF

static uint8_t ieee80211_find_ul_svc_node_free_index(struct ieee80211_node *ni)
{
    uint8_t i;

    for (i = 0; i < MAX_SCS_REQUEST_PER_DIRECTION; i++) {
        if (!ni->ul_info.ul_node[i].qos_attr.service_class_id)
            return i;
    }

    return MAX_SCS_REQUEST_PER_DIRECTION;
}

uint8_t ieee80211_find_ul_svc_node_index_nolock(struct ieee80211_node *ni,
                                                uint32_t svc_id, uint8_t rx_tid)
{
    uint8_t i;

    for (i = 0; i < MAX_SCS_REQUEST_PER_DIRECTION; i++) {
        if ((ni->ul_info.ul_node[i].qos_attr.service_class_id) &&
            (ni->ul_info.ul_node[i].qos_attr.service_class_id == svc_id) &&
            (rx_tid != SAWF_INVALID_RX_TID &&
             ni->ul_info.ul_node[i].qos_attr.actual_rx_tid == rx_tid)) {
            return i;
        }
        if ((ni->ul_info.ul_node[i].qos_attr.service_class_id) &&
            (ni->ul_info.ul_node[i].qos_attr.service_class_id == svc_id) &&
             rx_tid == SAWF_INVALID_RX_TID) {
            return i;
        }
    }

    return MAX_SCS_REQUEST_PER_DIRECTION;
}

qdf_export_symbol(ieee80211_find_ul_svc_node_index_nolock);

uint8_t ieee80211_find_ul_svc_node_index_lock(struct ieee80211_node *ni,
                                              uint32_t svc_id, uint8_t rx_tid)
{
    uint8_t i;

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    i = ieee80211_find_ul_svc_node_index_nolock(ni, svc_id, rx_tid);
    qdf_spin_unlock_bh(&ni->ni_scs_lock);

    return i;
}

qdf_export_symbol(ieee80211_find_ul_svc_node_index_lock);

static
void ieee80211_compute_resultant_ul_qos_attr(struct ieee80211_node *ni,
                                             struct ieee80211_qos_attr *qos_attr)
{
    uint8_t idx;
    bool update = false;
    uint8_t ac = qos_attr->access_category;

    /* Initial values for arbitrable SLA attributes */
    qos_attr->resultant_service_interval = UINT_MAX;
    qos_attr->min_data_rate = 0;
    qos_attr->delay_bound = UINT_MAX;

    for (idx = 0; idx < MAX_SCS_REQUEST_PER_DIRECTION; idx++) {
        if (ni->ul_info.ul_node[idx].qos_attr.service_class_id &&
            (ni->ul_info.ul_node[idx].qos_attr.access_category == ac)) {
               qos_attr->resultant_service_interval =
                        QDF_MIN(qos_attr->resultant_service_interval,
                                ni->ul_info.ul_node[idx].qos_attr.resultant_service_interval);
               qos_attr->min_data_rate =
                        QDF_MAX(qos_attr->min_data_rate,
                                ni->ul_info.ul_node[idx].qos_attr.min_data_rate);
               qos_attr->delay_bound =
                        QDF_MIN(qos_attr->delay_bound,
                                ni->ul_info.ul_node[idx].qos_attr.delay_bound);
               update = true;
        }
    }

    /* If there is no update, then set to 0 values */
    if (!update) {
        qos_attr->resultant_service_interval = 0;
        qos_attr->min_data_rate = 0;
        qos_attr->delay_bound = 0;
    }
}

uint32_t ieee80211_add_ul_svc_info(struct ieee80211_node *ni,
                                   struct ieee80211_qos_attr *qos_attr)
{
    uint8_t idx, soc_id;
    uint16_t peer_id;
    uint32_t svc_class_id = 0;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = vap->iv_ic;
    ol_txrx_soc_handle soc_txrx_handle;
    struct ieee80211_qos_attr resultant_qos_attr = {0};
    struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);

    soc_id = wlan_psoc_get_id(psoc);
    soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
    peer_id = cdp_get_peer_id(soc_txrx_handle, wlan_vdev_get_id(vap->vdev_obj),
                              ni->ni_macaddr);

    qdf_spin_lock_bh(&ni->ni_scs_lock);

    idx = ieee80211_find_ul_svc_node_free_index(ni);

    /* Return invalid svc_class_id if free entry is not found */
    if (idx == MAX_SCS_REQUEST_PER_DIRECTION) {
        qdf_spin_unlock_bh(&ni->ni_scs_lock);
        return INVALID_SCS_SVC_ID;
    }

    /* Set qos_attr at index idx */
    ni->ul_info.ul_node[idx].qos_attr = *qos_attr;

    /**
     * <---- 32 bit SCS Service Class ID ---->
     *  |  SOC_ID  |  PEER_ID  |  INDEX   |
     *  |  8 bits  |  16 bits  |  8 bits  |
     */
    if (!ni->ul_info.ul_node[idx].qos_attr.service_class_id) {
        /* assign service class id only if it is not assigned before */
        svc_class_id |= (soc_id << IEEE80211_SCS_SVC_ID_SOC_ID_SHIFT);
        svc_class_id |= (peer_id << IEEE80211_SCS_SVC_ID_PEER_ID_SHIFT);
        svc_class_id |= (idx << IEEE80211_SCS_SVC_ID_INDEX_SHIFT);

        ni->ul_info.ul_node[idx].qos_attr.service_class_id = svc_class_id;
    }

    resultant_qos_attr.access_category = qos_attr->access_category;
    resultant_qos_attr.user_priority = qos_attr->user_priority;
    resultant_qos_attr.resultant_burst_size = qos_attr->resultant_burst_size;

    ieee80211_compute_resultant_ul_qos_attr(ni, &resultant_qos_attr);

    ieee80211_send_ul_scs_qos_params_wmi_cmd(ni, &resultant_qos_attr,
                                             ADD_REQUEST);
    qdf_spin_unlock_bh(&ni->ni_scs_lock);

    return svc_class_id;
}

qdf_export_symbol(ieee80211_add_ul_svc_info);

uint32_t ieee80211_update_ul_svc_info(struct ieee80211_node *ni,
                                      struct ieee80211_qos_attr *qos_attr)
{
    uint32_t svc_id = qos_attr->service_class_id;

    if (ieee80211_delete_ul_svc_info(ni, svc_id,
                                     SAWF_INVALID_RX_TID) != QDF_STATUS_SUCCESS)
        return INVALID_SCS_SVC_ID;

    return ieee80211_add_ul_svc_info(ni, qos_attr);
}

qdf_export_symbol(ieee80211_update_ul_svc_info);

QDF_STATUS ieee80211_delete_ul_svc_info(struct ieee80211_node *ni,
                                        uint32_t svc_class_id,
                                        uint8_t rx_tid)
{
    uint8_t idx;
    struct ieee80211_qos_attr resultant_qos_attr = {0};

    qdf_spin_lock_bh(&ni->ni_scs_lock);
    idx = ieee80211_find_ul_svc_node_index_nolock(ni, svc_class_id, rx_tid);

    if (idx == MAX_SCS_REQUEST_PER_DIRECTION) {
        qdf_spin_unlock_bh(&ni->ni_scs_lock);
        return QDF_STATUS_E_INVAL;
    }

    resultant_qos_attr.access_category = ni->ul_info.ul_node[idx].qos_attr.access_category;;
    resultant_qos_attr.user_priority = ni->ul_info.ul_node[idx].qos_attr.user_priority;
    resultant_qos_attr.resultant_burst_size = ni->ul_info.ul_node[idx].qos_attr.resultant_burst_size;

    qdf_mem_zero(&ni->ul_info.ul_node[idx], sizeof(struct ul_svc_node));

    ieee80211_compute_resultant_ul_qos_attr(ni, &resultant_qos_attr);

    ieee80211_send_ul_scs_qos_params_wmi_cmd(ni, &resultant_qos_attr,
                                             SUBTRACT_REQUEST);

    qdf_spin_unlock_bh(&ni->ni_scs_lock);

    return QDF_STATUS_SUCCESS;
}

qdf_export_symbol(ieee80211_delete_ul_svc_info);
#endif /* (WLAN_FEATURE_11BE && WLAN_SUPPORT_SCS) || CONFIG_SAWF */

struct ieee80211_mcast_iter_arg {
    uint32_t peer_count;
};

static void
ieee80211_node_mcast_non_dms_iter(struct wlan_objmgr_vdev *vdev,
                                  void *object, void *arg)
{
    struct wlan_objmgr_peer *peer = (struct wlan_objmgr_peer *)object;
    struct ieee80211_mcast_iter_arg *itr_arg;
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;

    itr_arg = (struct ieee80211_mcast_iter_arg *)arg;

    ni = wlan_peer_get_mlme_ext_obj(peer);
    if (!ni)
        return;

    vap = ni->ni_vap;

    if (IEEE80211_ADDR_EQ(vap->iv_myaddr, ni->ni_macaddr))
        return;

    /* increment count of non DMS supported peer */
    if (
#ifdef WLAN_FEATURE_11BE
        !ieee80211_is_phymode_11be(ni->ni_phymode) &&
#endif
        !ieee80211_is_phymode_11ax(ni->ni_phymode)) {
        itr_arg->peer_count++;
    }
}

uint32_t ieee80211_node_mcast_non_dms_peer_count(struct ieee80211vap *vap)
{
    struct ieee80211_mcast_iter_arg itr_arg = {0};

    wlan_objmgr_iterate_peerobj_list(vap->vdev_obj,
                                     ieee80211_node_mcast_non_dms_iter,
                                     &itr_arg, WLAN_MLME_NB_ID);
    return itr_arg.peer_count;
}
