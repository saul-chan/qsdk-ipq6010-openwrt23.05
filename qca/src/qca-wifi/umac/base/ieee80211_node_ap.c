/*
 *
 * Copyright (c) 2011-2018, 2020-2021,2023-2024 Qualcomm Innovation Center, Inc.
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

#ifdef IPA_OFFLOAD
#include "wlan_ipa_ucfg_api.h"
#include "wlan_psoc_mlme.h"
#include "wlan_mlo_mgr_peer.h"
#endif
#include "ieee80211_node_priv.h"
#include "ieee80211_defines.h"
#include "ieee80211_api.h"
#if QCA_AIRTIME_FAIRNESS
#include <wlan_atf_utils_api.h>
#endif
#if WLAN_SUPPORT_SPLITMAC
#include <wlan_splitmac.h>
#endif
#if WLAN_SUPPORT_GREEN_AP
#include <wlan_green_ap_api.h>
#endif
#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif
#include <wlan_vdev_mlme.h>
#include <wlan_repeater_api.h>
#if WLAN_FEATURE_11BE_MLO
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#endif
#ifdef QCA_SUPPORT_WDS_EXTENDED
#include <cdp_txrx_cmn.h>
#endif

#if ATH_TxBF_DYNAMIC_LOF_ON_N_CHAIN_MASK
void iee80211_txbf_loforce_check(struct ieee80211_node *ni, bool nodejoin)
{
#ifdef ATH_SUPPORT_TxBF
    struct ieee80211com *ic = ni->ni_ic;
    if ( ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf){
        if(!nodejoin)
            ic->ic_ht_txbf_sta_assoc--;

        if(ic->ic_ht_txbf_sta_assoc == 0){
            if(ic->ic_txbf_loforceon_update)
                ic->ic_txbf_loforceon_update(ic,nodejoin);
        }
        if(nodejoin)
            ic->ic_ht_txbf_sta_assoc++;
    }
#endif
}
#endif
/*
 * Create a HOSTAP node  on current channel based on ssid.
 */
int
ieee80211_create_infra_bss(struct ieee80211vap *vap,
                      const u_int8_t *essid,
                      const u_int16_t esslen)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;
    bool new_node = false;

    if (vap->iv_bss == NULL) {
        ni = wlan_objmgr_alloc_ap_node(vap, vap->iv_myaddr);
        new_node = true;
        if (ni == NULL)
            return -ENOMEM;
    } else {
        ni = vap->iv_bss;
        ni->ni_ic = ic;
        ni->ni_vap = vap;
        ieee80211_node_reset(ni);
    }

    IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_myaddr);

    ni->ni_esslen = esslen;
    OS_MEMCPY(ni->ni_essid, essid, ni->ni_esslen);
    wlan_vdev_mlme_set_ssid(vap->vdev_obj, (const uint8_t*)essid, esslen);
    /* update mbss cache entry with ssid */
    ieee80211_mbssid_update_mbssie_cache_entry(vap, MBSS_CACHE_ENTRY_SSID);

    /* Skip re-assigning ni_intval for LP IOT vap */
    if (!(vap->iv_create_flags & IEEE80211_LP_IOT_VAP))   {
        ni->ni_intval = ic->ic_intval;
    }
    if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap)) {
        struct wlan_crypto_params *vdev_crypto_params, *peer_crypto_params = NULL;

	vdev_crypto_params = wlan_crypto_vdev_get_crypto_params(vap->vdev_obj);
        if (!vdev_crypto_params)
            goto fail;

	peer_crypto_params = wlan_crypto_peer_get_crypto_params(ni->peer_obj);
        if (!peer_crypto_params)
            goto fail;
        qdf_mem_copy(peer_crypto_params, vdev_crypto_params,sizeof(struct wlan_crypto_params));
        ni->ni_capinfo |= IEEE80211_CAPINFO_PRIVACY;
    }
    /* Set the node htcap to be same as ic htcap */
    ni->ni_htcap = ic->ic_htcap;

    if ((vap->iv_opmode != IEEE80211_M_MONITOR) && (!wlan_vap_get_bss_rsp_evt_status(vap) ||
        !wlan_vap_is_bss_created(vap))) {
        qdf_err("BSS peer creation is not complete: %d:%d",
                vap->iv_bss_rsp_evt_status, vap->iv_bss_rsp_status);
        goto fail;
    }

    return ieee80211_sta_join_bss(ni);

fail:
    /* free local node created and remain not joined to bss */
    if (new_node)
        wlan_objmgr_delete_node(ni);

    return -1;
}

/*
 * Handle a station leaving an 11g network.
 */
void
ieee80211_node_leave_11g(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    bool update_beacon = false;

    KASSERT((IEEE80211_IS_CHAN_ANYG(vap->iv_bsschan)
             || IEEE80211_IS_CHAN_11NG(vap->iv_bsschan)
             || IEEE80211_IS_CHAN_11AXG(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
             || IEEE80211_IS_CHAN_11BEG(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
            ),
            ("not in 11g, bss %u:0x%llx, curmode %u", vap->iv_bsschan->ic_freq,
             vap->iv_bsschan->ic_flags, ic->ic_curmode));

    KASSERT((ni->ni_vap->iv_opmode == IEEE80211_M_HOSTAP ||
             ni->ni_vap->iv_opmode == IEEE80211_M_BTAMP ||
             ni->ni_vap->iv_opmode == IEEE80211_M_IBSS), (" node leave in invalid opmode "));

    /*
     * If a long slot station do the slot time bookkeeping.
     */
    if ((ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME) == 0) {
        if (ic->ic_longslotsta) {
           ic->ic_longslotsta--;
           IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "long slot time station leaves, count now %d\n",
                       ic->ic_longslotsta);
        } else {
           IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                "bogus long slot station count %d", ic->ic_longslotsta);
        }

        if (ic->ic_longslotsta == 0) {
            /*
             * Re-enable use of short slot time if supported
             * and not operating in IBSS mode (per spec).
             */
            if (ic->ic_caps & IEEE80211_C_SHSLOT) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                                  "%s: re-enable use of short slot time\n",
                                  __func__);
                ieee80211_set_shortslottime(ic, 1);
                wlan_pdev_beacon_update(ic);
            }
        }
    }

    /*
     * If a non-ERP station do the protection-related bookkeeping.
     */
    if (((ni->ni_flags & IEEE80211_NODE_ERP) == 0) && (ic->ic_nonerpsta > 0)) {
        KASSERT(ic->ic_nonerpsta > 0, ("bogus non-ERP station count %d", ic->ic_nonerpsta));
        ic->ic_nonerpsta--;
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "non-ERP station leaves, count now %d", ic->ic_nonerpsta);
        if (ic->ic_nonerpsta == 0) {
            struct ieee80211vap *tmpvap;
            ieee80211_update_erp_info(vap);
            /* XXX verify mode? */
            if (ic->ic_caps & IEEE80211_C_SHPREAMBLE) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,
                                  "%s: re-enable use of short preamble\n",
                                  __func__);
                ieee80211com_set_flags(ic, IEEE80211_F_SHPREAMBLE);
                ieee80211com_clear_flags(ic, IEEE80211_F_USEBARKER);
            }

            TAILQ_FOREACH(tmpvap, &(ic)->ic_vaps, iv_next)
                ieee80211_vap_erpupdate_set(tmpvap);

            update_beacon = true;
        }
    }

    /* Trigger beacon template update */
    if (update_beacon == true)
        wlan_vdev_beacon_update(vap);
}

#if MESH_MODE_SUPPORT
static int
ieee80211_clear_scanentry_mesh_flags(void *arg, wlan_scan_entry_t scan_entry);
static int
ieee80211_clear_scanentry_mesh_flags(void *arg, wlan_scan_entry_t scan_entry)
{
#if NOT_YET_MESH_PEER_SUPPORT
    int flags = 0;
    flags = wlan_scan_entry_get_flags(scan_entry);
    flags &= ~IEEE80211_SE_FLAG_IS_MESH;
    flags &= ~IEEE80211_SE_FLAG_INTERSECT_DONE;
    wlan_scan_entry_set_flags(scan_entry, flags);
#endif
    return EOK;
}
#endif

#if WLAN_OBJMGR_REF_ID_TRACE
bool ieee80211_node_leave_debug(struct ieee80211_node *ni, const char *func, int line, const char *file)
#else
bool _ieee80211_node_leave(struct ieee80211_node *ni, const char *func, int line)
#endif
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    bool   retval = false;
    bool dup_leave = false;
    bool update_pdev_beacon = false;
#if defined(IPA_OFFLOAD) || (defined(QCA_SUPPORT_WDS_EXTENDED) && defined(WLAN_FEATURE_11BE_MLO))
    struct wlan_objmgr_psoc *psoc;
#endif
#ifdef IPA_OFFLOAD
    struct wlan_objmgr_vdev *vdev;
    enum wlan_ipa_wlan_event wlan_event;
    struct wlan_mlo_peer_context *ml_peer_ctx;
    struct wlan_mlo_link_peer_entry *link_peer;
    uint8_t i;
#endif

    IEEE80211_NOTE(vap, IEEE80211_MSG_AUTH | IEEE80211_MSG_ASSOC | IEEE80211_MSG_DEBUG,
            ni, "%s: station with aid %d leaves (refcnt %u) \n", __func__,
            IEEE80211_NODE_AID(ni), ieee80211_node_refcnt(ni));

#ifdef WLAN_FEATURE_11BE_MLO
    if (wlan_peer_is_mlo(ni->peer_obj) && wlan_mlo_partner_peer_delete_is_allowed(ni->peer_obj)) {
        struct wlan_mlo_peer_context *ml_peer;

        ml_peer = ni->peer_obj->mlo_peer_ctx;
        if (ml_peer) {
            if (ml_peer->mlpeer_state != ML_PEER_DISCONN_INITIATED) {
                mlme_err("Link peer " QDF_MAC_ADDR_FMT " with MLD mac " QDF_MAC_ADDR_FMT " delete without partner peer delete, func:%s:%u",
                         QDF_MAC_ADDR_REF(ni->ni_macaddr), QDF_MAC_ADDR_REF(ml_peer->peer_mld_addr.bytes), func, line);
            }
        }
    }
#endif

    IEEE80211_NODE_STATE_LOCK_BH(ni);
    /* If node leave is already started, return. */
    if (ieee80211node_has_flag(ni, IEEE80211_NODE_LEAVE_ONGOING))
        dup_leave = true;
    else
        ieee80211node_set_flag(ni, IEEE80211_NODE_LEAVE_ONGOING);
    IEEE80211_NODE_STATE_UNLOCK_BH(ni);

  if (dup_leave) {
      IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC | IEEE80211_MSG_DEBUG, ni,
              "dup_leave for %s\n", ether_sprintf(ni->ni_macaddr));
      return 1;
  }

#ifdef WLAN_FEATURE_11BE_MLO
    /* Just delete the node in case of tmp node */
    if (ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)
        && wlan_peer_is_assoc_rejected(ni->peer_obj)) {
        wlan_mlo_link_peer_delete(ni->peer_obj);
        wlan_objmgr_delete_node(ni);
    }
#endif

    if(ni->ni_node_esc == true) {
        ni->ni_node_esc = false;
        vap->iv_ic->ic_auth_tx_xretry--;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH, "%s: ni = 0x%pK ni->ni_macaddr = %s ic_auth_tx_xretry = %d\n",
                __func__, ni, ether_sprintf(ni->ni_macaddr), vap->iv_ic->ic_auth_tx_xretry);
    }
#ifdef ATH_SWRETRY
    if (ic->ic_reset_pause_tid)
        ic->ic_reset_pause_tid(ni->ni_ic, ni);
#endif

    KASSERT(vap->iv_opmode == IEEE80211_M_HOSTAP
            || vap->iv_opmode == IEEE80211_M_WDS ||
            vap->iv_opmode == IEEE80211_M_BTAMP  ||
            vap->iv_opmode == IEEE80211_M_IBSS   ||
            vap->iv_opmode == IEEE80211_M_STA,
            ("unexpected operating mode %u", vap->iv_opmode));

    /*
     * If node wasn't previously associated all
     * we need to do is reclaim the reference.
     */
    /* XXX ibss mode bypasses 11g and notification */

    IEEE80211_NODE_STATE_LOCK_BH(ni);

    /*
     * Prevent _ieee80211_node_leave() from reentry which would mess up the
     * value of iv_sta_assoc. Before AP received the tx ack for "disassoc
     * request", it may have received the "auth (not SUCCESS status)" to do
     * node leave. With the flag, follow-up cleanup wouldn't call
     * _ieee80211_node_leave() again when execuating the tx_complete handler.
     */
    ieee80211node_set_flag(ni, IEEE80211_NODE_LEAVE_ONGOING);

#ifdef WLAN_FEATURE_11BE_MLO
    /*
     * Bridge peer didn't go through ieee80211_node_join, so just do
     * basic node reclaim.
     */
    if (wlan_peer_get_peer_type(ni->peer_obj) == WLAN_PEER_MLO_BRIDGE) {
#ifdef QCA_SUPPORT_WDS_EXTENDED
        psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
        if (psoc && wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_WDS_EXTENDED)) {
            osif_peer_dev *osif_peer = NULL;

            cdp_wds_ext_get_peer_osif_handle(wlan_psoc_get_dp_handle(psoc),
                                             wlan_vdev_get_id(vap->vdev_obj),
                                             ni->ni_macaddr,
                                             (ol_osif_peer_handle *)&osif_peer);
            if (osif_peer) {
                IEEE80211_DELIVER_EVENT_MLME_DEAUTH_COMPLETE(vap, ni->ni_macaddr,
                                                             IEEE80211_STATUS_SUCCESS,
                                                             ni->ni_associd,
                                                             IEEE80211_REASON_UNSPECIFIED,
                                                             &ni->peer_obj->mldaddr[0]);
            }
        }
#endif
        IEEE80211_VAP_LOCK(vap);
        vap->iv_bridge_cnt--;
        IEEE80211_VAP_UNLOCK(vap);

        IEEE80211_COMM_LOCK(ic);
        ic->ic_bridge_cnt--;
        IEEE80211_COMM_UNLOCK(ic);

        qdf_nofl_info("%s radio bridge peer count=%d vap bridge peer count=%d",
                      ic->ic_netdev->name, ic->ic_bridge_cnt, vap->iv_bridge_cnt);

        retval = ieee80211_sta_leave(ni);
        if (ni->ni_noise_stats)
            OS_FREE (ni->ni_noise_stats);
        IEEE80211_NODE_STATE_UNLOCK_BH(ni);

        return retval;
    }
#endif

#if MESH_MODE_SUPPORT
    if (ni->ni_ext_flags&IEEE80211_LOCAL_MESH_PEER) {
        util_wlan_scan_db_iterate_macaddr(vap, ni->ni_macaddr,
                (scan_iterator_func)ieee80211_clear_scanentry_mesh_flags, NULL);
    }
#endif

    if (ni->ni_vap_cnt_updated) {
        if (vap->iv_sta_assoc > 0) {
            IEEE80211_VAP_LOCK(vap);
            vap->iv_sta_assoc--;
            if (ni->ni_ext_flags & IEEE80211_NODE_HE && vap->iv_ax_sta_assoc > 0) {
                    vap->iv_ax_sta_assoc--;
                    if (vap->iv_ax_sta_assoc == vap->iv_sta_assoc &&
                        ic->ic_muedca_mode_state == HEMUEDCA_HOST_DYNAMIC_MODE)
                        vap->iv_muedcastate.mu_edca_dynamic_state &=
                                ~MUEDCA_DYNAMIC_ALGO_UPDATE_STATE_MASK;
            }
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                              "%s, STA left,  decremented iv_sta_assoc(%hu)",
                              __func__,vap->iv_sta_assoc);

            if (ni->ni_vhtcap & IEEE80211_VHTCAP_MU_BFORMEE) {
                vap->iv_mu_bformee_sta_assoc--;
                /* update extended bss load element in beacon */
                ieee80211_vap_ext_bssload_update_set(vap);
            }
            if (HECAP_PHY_SUBFME_GET_FROM_IC(ni->ni_he.hecap_phyinfo))
                vap->iv_he_su_bformee_sta_assoc--;

            IEEE80211_VAP_UNLOCK(vap);
        }
        else {
            qdf_rl_nofl_info("WARNING:ic=%s,vap=%d,vap->iv_sta_assoc getting decremented below zero"
                "vap->iv_sta_assoc %d,ic->ic_sta_assoc %d,(%02x:%02x:%02x:%02x:%02x:%02x)\n",
                ic->ic_netdev->name, vap->iv_unit, vap->iv_sta_assoc, ic->ic_sta_assoc,
                ni->ni_macaddr[0], ni->ni_macaddr[1],
                ni->ni_macaddr[2],ni->ni_macaddr[3],
                ni->ni_macaddr[4],ni->ni_macaddr[5]);
        }

        IEEE80211_COMM_LOCK(ic);
        ic->ic_sta_assoc--;
        /* Update bss load element in beacon */
        ieee80211_vap_qbssload_update_set(vap);

#ifdef IPA_OFFLOAD
        if (ucfg_ipa_is_enabled() && (vap->iv_opmode == IEEE80211_M_HOSTAP)) {
            /* Check if peer is ml peer or not */
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                ml_peer_ctx = ni->peer_obj->mlo_peer_ctx;
                if (!ml_peer_ctx)
                    return false;

                for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
                    link_peer = &ml_peer_ctx->peer_list[i];
                    if (!link_peer->link_peer)
                        continue;

                    /* Send event to IPA component for Disabling WDI
                    * pipes for secondary link
                    */
                    if (!link_peer->is_primary) {
                        wlan_event = WLAN_IPA_MLO_CLIENT_DISCONNECT;
                        vdev = link_peer->link_peer->peer_objmgr.vdev;
                        psoc = wlan_vdev_get_psoc(vdev);
                        if (!psoc) {
                            qdf_err("Psoc is NULL");
                            return false;
                        }
                        ucfg_ipa_event_wq(psoc, &link_peer->link_addr,
                                          vdev, wlan_event);
                    } else {
                        /* Send Client disconnect event only with mld peer mac address and for primary link */
                        vdev = link_peer->link_peer->peer_objmgr.vdev;
                        wlan_event = WLAN_IPA_CLIENT_DISCONNECT;
                        psoc = wlan_vdev_get_psoc(vdev);
                        if (!psoc) {
                                qdf_err("Psoc is NULL");
                                return false;
                        }
                        ucfg_ipa_event_wq(psoc,
                                          &ml_peer_ctx->peer_mld_addr,
                                          vdev, wlan_event);
                    }
                }
            } else {
                /* Send event to IPA for legacy clients */
                wlan_event = WLAN_IPA_CLIENT_DISCONNECT;
                vdev = vap->vdev_obj;
                psoc = wlan_vdev_get_psoc(vdev);
                if (!psoc) {
                        qdf_err("Psoc is NULL");
                        return false;
                }
                ucfg_ipa_event_wq(psoc, ni->ni_macaddr, vdev, wlan_event);
            }
        }
#endif
        if (IEEE80211_NODE_USE_HT(ni)) {
            ic->ic_ht_sta_assoc--;
            if (ni->ni_htcap & IEEE80211_HTCAP_C_GREENFIELD) {
                ASSERT(ic->ic_ht_gf_sta_assoc > 0);
                ic->ic_ht_gf_sta_assoc--;
            }
#if ATH_TxBF_DYNAMIC_LOF_ON_N_CHAIN_MASK
            iee80211_txbf_loforce_check(ni,0);
#endif
            if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40) || (ni->ni_chwidth == IEEE80211_CWM_WIDTH80) || (ni->ni_chwidth == IEEE80211_CWM_WIDTH160))
	      ic->ic_ht40_sta_assoc--;
	  }

        /* 11AX TODO (Phase II) - Below likely to be required for 11ax as well.
         * Hence, we have added a check for AXG. However, reconfirm based on
         * latest draft.
         */
        if (IEEE80211_IS_CHAN_ANYG(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11NG(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AXG(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
            || IEEE80211_IS_CHAN_11BEG(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
            )
            ieee80211_node_leave_11g(ni);

        /* For beacon offload targets, the beacon need to updated in FW */
        if (IEEE80211_IS_CHAN_11N(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AC(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AX(vap->iv_bsschan)
#if WLAN_FEATURE_11BE
            || IEEE80211_IS_CHAN_11BE(vap->iv_bsschan)
#endif
	    ) {
            if (IEEE80211_IS_HTPROT_ENABLED(ic) &&
                (ic->ic_sta_assoc == ic->ic_ht_sta_assoc)) {
                    update_pdev_beacon = true;
                    IEEE80211_DISABLE_HTPROT(ic);
            }
        }

        ieee80211_admctl_node_leave(vap, ni);

#if DYNAMIC_BEACON_SUPPORT
        /* if no sta associated then cancel the timer and suspend the beacon */
        if (vap->iv_sta_assoc == 0 && vap->iv_dbeacon == 1 && vap->iv_dbeacon_runtime == 1) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "Node leave, Suspend beacon \n");
            qdf_spin_lock_bh(&vap->iv_dbeacon_lock);
            OS_CANCEL_TIMER(&vap->iv_dbeacon_suspend_beacon);
            if (!ieee80211_mlme_beacon_suspend_state(vap)) {
                ieee80211_mlme_set_beacon_suspend_state(vap, true);
            }
            qdf_spin_unlock_bh(&vap->iv_dbeacon_lock);
        }
#endif

        /*
         * Cleanup station state.  In particular clear various state that
         * might otherwise be reused if the node is reused before the
         * reference count goes to zero (and memory is reclaimed).
         *
         * If ni is not in node table, it has been reclaimed in another thread.
         */
#if QCA_AIRTIME_FAIRNESS
        wlan_atf_peer_join_leave(ni->peer_obj, 0);
#endif // QCA_AIRTIME_FAIRNESS
        if ((vap->watermark_threshold_flag == false) && (ic->ic_sta_assoc < vap->watermark_threshold)) {
                vap->watermark_threshold_flag = true;
            }
#ifdef MU_CAP_WAR_ENABLED
        ieee80211_mu_cap_client_join_leave(ni,0);
#endif
        retval = ieee80211_sta_leave(ni);

#ifdef WLAN_DBS_POWER_REDUCTION
        if (ic->ic_update_active_count)
            ic->ic_update_active_count(ic);
#endif

        OS_FREE (ni->ni_noise_stats);
        IEEE80211_COMM_UNLOCK(ic);
        IEEE80211_NODE_STATE_UNLOCK_BH(ni);
    } else {
        ieee80211_admctl_node_leave(vap, ni);
#ifdef MU_CAP_WAR_ENABLED
        ieee80211_mu_cap_client_join_leave(ni,0);
#endif
        retval = ieee80211_sta_leave(ni);
        if (ni->ni_noise_stats)
            OS_FREE (ni->ni_noise_stats);
        IEEE80211_NODE_STATE_UNLOCK_BH(ni);
    }

    /* 11AX TODO: Recheck future 802.11ax drafts (>D1.0) on coex rules */
    if (((ni->ni_flags & IEEE80211_NODE_HT || IEEE80211_NODE_USE_HE(ni)
#ifdef WLAN_FEATURE_11BE
          || IEEE80211_NODE_USE_EHT(ni)
#endif /* WLAN_FEATURE_11BE */
         ) &&
         (ni->ni_flags & IEEE80211_NODE_40MHZ_INTOLERANT)) &&
          ni->ni_set_40mhz_intol_bw) {
         ieee80211_change_cw(ic);
    }
    if (ieee80211_vap_ext_bssload_is_set(vap) &&
        ieee80211_vap_ext_bssload_update_is_set(vap))
        wlan_vdev_beacon_update(vap);

    if (update_pdev_beacon) {
        wlan_pdev_beacon_update(ic);
    }

    return retval;
}

#if WLAN_OBJMGR_REF_ID_TRACE
qdf_export_symbol(ieee80211_node_leave_debug);
#else
qdf_export_symbol(_ieee80211_node_leave);
#endif

/*
 * Handle a station joining an 11g network.
 */
static void
ieee80211_node_join_11g(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    bool update_beacon = false;

    KASSERT((IEEE80211_IS_CHAN_ANYG(vap->iv_bsschan) ||
             IEEE80211_IS_CHAN_11NG(vap->iv_bsschan) ||
             IEEE80211_IS_CHAN_11AXG(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
             || IEEE80211_IS_CHAN_11BEG(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
             ),
            ("not in 11g, bss %u:0x%llx, curmode %u", vap->iv_bsschan->ic_freq,
             vap->iv_bsschan->ic_flags, ic->ic_curmode));

    KASSERT((ni->ni_vap->iv_opmode == IEEE80211_M_HOSTAP ||
             ni->ni_vap->iv_opmode == IEEE80211_M_BTAMP  ||
             ni->ni_vap->iv_opmode == IEEE80211_M_IBSS), (" node join in invalid opmode "));
    /*
     * Station isn't capable of short slot time.  Bump
     * the count of long slot time stations and disable
     * use of short slot time.  Note that the actual switch
     * over to long slot time use may not occur until the
     * next beacon transmission (per sec. 7.3.1.4 of 11g).
     */
    if (ieee80211_is_phymode_11b(ni->ni_phymode) ||
        ((ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME) == 0)) {
        ic->ic_longslotsta++;
        IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_ASSOC, ni,
                       "station needs long slot time, count %d",
                       ic->ic_longslotsta);
        /* XXX vap's w/ conflicting needs won't work */
        if (!IEEE80211_IS_CHAN_108G(vap->iv_bsschan)) {
            /*
             * Don't force slot time when switched to turbo
             * mode as non-ERP stations won't be present; this
             * need only be done when on the normal G channel.
             */
            ieee80211_set_shortslottime(ic, 0);
            wlan_pdev_beacon_update(ic);
        }
    }
    /*
     * If the new station is not an ERP station
     * then bump the counter and enable protection
     * if configured.
     */
    if (!ieee80211_iserp(ni)) {
        ic->ic_nonerpsta++;
        IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_ASSOC, ni,
                       "station is !ERP, %d non-ERP stations associated\n",
                       ic->ic_nonerpsta);
            /*
             * If protection is configured, enable it.
             */
            if (ic->ic_protmode != IEEE80211_PROT_NONE) {
                IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ASSOC,
                                  "%s: enable use of protection\n", __func__);
                ieee80211com_set_flags(ic, IEEE80211_F_USEPROT);
                ieee80211_set_protmode(ic);
            }
            /*
             * If station does not support short preamble
             * then we must enable use of Barker preamble.
             */
            if ((ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE) == 0) {
                IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_ASSOC, ni,
                               "%s", "station needs long preamble");
                ieee80211com_set_flags(ic, IEEE80211_F_USEBARKER);
                ieee80211com_clear_flags(ic, IEEE80211_F_SHPREAMBLE);
            }

            /* Update ERP element if this is first non ERP station */
            if (ic->ic_nonerpsta == 1) {
	          struct ieee80211vap *tmpvap;
	          TAILQ_FOREACH(tmpvap, &(ic)->ic_vaps, iv_next)
		    ieee80211_vap_erpupdate_set(tmpvap);
	    }
            update_beacon = true;
    } else
        ieee80211node_set_flag(ni, IEEE80211_NODE_ERP);

    /* Trigger beacon template update */
    if (update_beacon == true)
        wlan_vdev_beacon_update(vap);

}

/*
 * function to handle station joining infrastructure network.
 * used for AP mode vap only.
 */
int ieee80211_node_join(struct ieee80211_node *ni, uint8_t alloc_aid)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct wlan_objmgr_vdev *vdev;
    uint16_t vap_assoc_limit;

    vdev = vap->vdev_obj;
    IEEE80211_NODE_STATE_LOCK(ni);

   vap_assoc_limit =  ieee80211_get_vap_assoc_limit(ic, vap);

#if WLAN_SUPPORT_SPLITMAC
    /*
     * In splitmac mode, the AID is assigned before the call to
     * ieee80211_node_join.
     */
    if ((splitmac_is_enabled(vdev) && (ni->ni_associd != 0)) ||
        (!splitmac_is_enabled(vdev) && (ni->ni_associd == 0))) {
#else
    if (ni->ni_associd == 0) {
#endif
        u_int16_t aid;

        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "%s: Number of associated STAs: %hu",
                       __func__, vap->iv_sta_assoc);

        if((ic->ic_sta_assoc + ic->ic_bridge_cnt) >= ic->ic_num_clients) {
          IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                         "%s: Per radio Associated STA limit reached!",
                         __func__);

#ifdef QCA_SUPPORT_CP_STATS
          vdev_cp_stats_sta_xceed_rlim_inc(vap->vdev_obj, 1);
#endif
          IEEE80211_NODE_STATE_UNLOCK(ni);
          return -1; /* Per radio soft client limit reached */
        }

        if ((vap->iv_sta_assoc + vap->iv_bridge_cnt) >= vap_assoc_limit) {
          IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                         "%s: Associated STA limit reached!",
                         __func__);

#ifdef QCA_SUPPORT_CP_STATS
          vdev_cp_stats_sta_xceed_vlim_inc(vap->vdev_obj, 1);
#endif
          IEEE80211_NODE_STATE_UNLOCK(ni);
          return -1; /* soft client limit reached */
        }

#ifdef WLAN_FEATURE_11BE_MLO
        if (!(wlan_vdev_mlme_is_mlo_ap(vdev) || (ic->is_mbssid_enabled)) &&
             (vap->iv_aid_bitmap == NULL)) {
#else
        if (vap->iv_aid_bitmap == NULL) {
#endif
            IEEE80211_NODE_STATE_UNLOCK(ni);
            return -1; /* vap is being deleted */
        }

#if WLAN_SUPPORT_SPLITMAC
        /* AID will come from the mac above us */
        if (splitmac_is_enabled(vdev)) {
          goto skip_aid;
        }
#endif

#if ATH_SUPPORT_MBO
        if (ieee80211_vap_mbo_check(vap) && ieee80211_vap_mbo_assoc_status(vap)) {
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni, "MBO association disallowed");
            IEEE80211_NODE_STATE_UNLOCK(ni);
            ieee80211_try_mark_node_for_delayed_cleanup(ni);
            return -1;
        }
#endif
        if (ieee80211_vap_oce_check(vap) && ieee80211_vap_oce_assoc_reject(vap, ni)) {
            IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                           "OCE association rejected due to low RSSI %d",
                           ni->ni_snr);
            IEEE80211_NODE_STATE_UNLOCK(ni);
            ieee80211_try_mark_node_for_delayed_cleanup(ni);
            return -1;
        }

#ifndef WLAN_FEATURE_11BE_MLO
       /* For Non-MLO enabled code, AID should be allocated from
          following block
        */
       alloc_aid = 1;
#endif
        if (alloc_aid) {
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_vdev_mlme_is_mlo_ap(vdev) || (ic->is_mbssid_enabled)) {
                aid = mlme_get_aid(vap->vdev_obj);
                if (aid == (uint16_t)-1) {
                    IEEE80211_NODE_STATE_UNLOCK(ni);
                    IEEE80211_NODE_LEAVE(ni);
                    return -1;
                }
                else {
                    ni->ni_associd = aid;
                }
            }
            else
#endif
            {
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
                    IEEE80211_NODE_STATE_UNLOCK(ni);
                    IEEE80211_NODE_LEAVE(ni);
                    return -1;
                }
                ni->ni_associd = aid | 0xc000;
                IEEE80211_AID_SET(vap, ni->ni_associd);
            }
        }
#if WLAN_SUPPORT_SPLITMAC
skip_aid:
#endif

        IEEE80211_VAP_LOCK(vap);
        vap->iv_sta_assoc++;
        ni->ni_vap_cnt_updated = 1;
        if (ni->ni_ext_flags & IEEE80211_NODE_HE) {
            vap->iv_ax_sta_assoc++;
            if (ic->ic_muedca_mode_state == HEMUEDCA_HOST_DYNAMIC_MODE &&
                vap->iv_ax_sta_assoc == vap->iv_sta_assoc ) {
                vap->iv_muedcastate.mu_edca_dynamic_state &=
                ~MUEDCA_DYNAMIC_ALGO_UPDATE_STATE_MASK;
            }
        }
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "%s: STA joined, incremented iv_sta_assoc(%hu)",
                       __func__, vap->iv_sta_assoc);

        if (ni->ni_vhtcap & IEEE80211_VHTCAP_MU_BFORMEE) {
            vap->iv_mu_bformee_sta_assoc++;
            /* update extended bss load element in beacon */
            ieee80211_vap_ext_bssload_update_set(vap);
        }
        if (HECAP_PHY_SUBFME_GET_FROM_IC(ni->ni_he.hecap_phyinfo))
            vap->iv_he_su_bformee_sta_assoc++;

        IEEE80211_VAP_UNLOCK(vap);

        /* Update bss load element in beacon */
        ieee80211_vap_qbssload_update_set(vap);

        if (vap->vdev_mlme->mgmt.generic.ampdu != 0) {
            ieee80211node_clear_flag(ni, IEEE80211_NODE_NOAMPDU);
        }
        else {
            ieee80211node_set_flag(ni, IEEE80211_NODE_NOAMPDU);
        }

        IEEE80211_COMM_LOCK(ic);
#ifdef MU_CAP_WAR_ENABLED
        ieee80211_mu_cap_client_join_leave(ni,1);
#endif

        ni->ni_noise_stats = (struct noise_stats *)OS_MALLOC(ic->ic_osdev,ic->traf_bins*sizeof(struct noise_stats),GFP_KERNEL);
        if (ni->ni_noise_stats == NULL) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ASSOC,"%s: ni_noise_stats is null\n",__func__);
        }
        ic->ic_sta_assoc++;
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "%s: STA joined, incremented iv_sta_assoc(%hu), "
                       "ic_sta_assoc=%d, peer count=%hu",
                       __func__, vap->iv_sta_assoc,
                       ic->ic_sta_assoc,
                       wlan_vdev_get_peer_count(vap->vdev_obj));

        if (ic->ic_sta_assoc >= vap->watermark_threshold)
        {
            if (vap->watermark_threshold_flag == true) {
                vap->watermark_threshold_reached++;
                vap->watermark_threshold_flag = false ;

            }
            if (ic->ic_sta_assoc >= vap->assoc_high_watermark) {
                vap->assoc_high_watermark = ic->ic_sta_assoc;
                vap->assoc_high_watermark_time = OS_GET_TIMESTAMP();
            }
        }
        if (IEEE80211_NODE_USE_HT(ni)) {
            ic->ic_ht_sta_assoc++;
            if (ni->ni_htcap & IEEE80211_HTCAP_C_GREENFIELD)
                ic->ic_ht_gf_sta_assoc++;
#if ATH_TxBF_DYNAMIC_LOF_ON_N_CHAIN_MASK
             iee80211_txbf_loforce_check(ni,1);
#endif
             if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40 ) || (ni->ni_chwidth == IEEE80211_CWM_WIDTH80) || (ni->ni_chwidth == IEEE80211_CWM_WIDTH160)) {
                  ic->ic_ht40_sta_assoc++;
      	    }
	  }


        /* 11AX TODO (Phase II) - Below likely to be required for 11ax as well.
         * Hence, we have added a check for AXG. However, reconfirm based on
         * latest draft.
         */
        if (IEEE80211_IS_CHAN_ANYG(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11NG(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AXG(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
            || IEEE80211_IS_CHAN_11BEG(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
            )
            ieee80211_node_join_11g(ni);

        /* For beacon offload targets, the beacon need to updated in FW */
        if (IEEE80211_IS_CHAN_11N(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AC(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AX(vap->iv_bsschan)
#if WLAN_FEATURE_11BE
            || IEEE80211_IS_CHAN_11BE(vap->iv_bsschan)
#endif
	    ) {
            if (!IEEE80211_IS_HTPROT_ENABLED(ic) &&
                (ic->ic_sta_assoc > ic->ic_ht_sta_assoc)) {
                    wlan_pdev_beacon_update(ic);
                    IEEE80211_ENABLE_HTPROT(ic);
            }
        }

        IEEE80211_COMM_UNLOCK(ic);
    }
    ni->ni_inact_reload = ni->ni_vap->iv_inact_auth;
    ni->ni_inact = ni->ni_inact_reload;

    if (ieee80211_vap_ext_bssload_is_set(vap) &&
        ieee80211_vap_ext_bssload_update_is_set(vap))
        wlan_vdev_beacon_update(vap);

    IEEE80211_NODE_STATE_UNLOCK(ni);
    return 0;
}

/*
 * Craft a temporary node suitable for sending a management frame
 * to the specified station.  We craft only as much state as we
 * need to do the work since the node will be immediately reclaimed
 * once the send completes, and the temporary node will NOT be put
 * into node table.
 */
struct ieee80211_node *
ieee80211_tmp_node(struct ieee80211vap *vap, const u_int8_t *macaddr)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;

    if(!(IEEE80211_ADDR_IS_VALID(macaddr))){
        qdf_nofl_info("INVALID MAC ADDRESS \n");
        return NULL;
    }

    /*
     * if vap is being deleted, do not allow new allocations.
     */
    if (ieee80211_vap_deleted_is_set(vap)) {
        return NULL;
    }
    /* Allocate TEMP peer object */
    ni = wlan_objmgr_alloc_tmp_node(vap, (uint8_t *)macaddr);
    if (ni == NULL) {
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_rx_node_alloc_inc(vap->vdev_obj, 1);
#endif
        return NULL;
    }

#if IEEE80211_DEBUG_NODELEAK
    do {
        rwlock_state_t lock_state;
        OS_RWLOCK_WRITE_LOCK(&ic->ic_nodelock,&lock_state);
        TAILQ_INSERT_TAIL(&ic->ic_nodes, ni, ni_alloc_list);
        OS_RWLOCK_WRITE_UNLOCK(&ic->ic_nodelock,&lock_state);
    } while(0);
#endif
    ieee80211node_set_flag(ni, IEEE80211_NODE_TEMP); /* useful for debugging */

    ni->ni_bss_node = ieee80211_try_ref_bss_node(vap, WLAN_MLME_OBJMGR_ID);
    if (!ni->ni_bss_node) {
        wlan_objmgr_delete_node(ni);
#ifdef QCA_SUPPORT_CP_STATS
        vdev_cp_stats_rx_node_alloc_dec(vap->vdev_obj, 1);
#endif
        return NULL;
    }
    ni->ni_vap = vap;
    ni->ni_ic = ic;
    ni->ni_table = NULL;
    ni->ni_persta = NULL;

    /* copy some default variables from parent */
    IEEE80211_ADDR_COPY(ni->ni_macaddr, macaddr);
    ni->ni_intval = ic->ic_intval; /* default beacon interval */

    /* set default rate and channel */
    ieee80211_node_set_chan(ni);

    ni->ni_txpower = ic->ic_txpowlimit;	/* max power */

#if 0
    /* in case of temp node we don't need ni_persta keys
     * to be allocated even for IBSS mode.
     */
    {
        int i;
        ieee80211_crypto_resetkey(vap, &ni->ni_persta->nips_hwkey, IEEE80211_KEYIX_NONE);
        for (i = 0; i < IEEE80211_WEP_NKID; i++) {
            ieee80211_crypto_resetkey(vap, &ni->ni_persta->nips_swkey[i], IEEE80211_KEYIX_NONE);
        }
    }
#endif
    ni->ni_ath_defkeyindex = IEEE80211_INVAL_DEFKEY;

    /* 11n  or 11ac */
    ni->ni_chwidth = ic->ic_cwm_get_width(ic);

    IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_bss->ni_bssid);


    return ni;
}

void wlan_ap_delete_assoc_list_entries(void *ctx)
{
    wlan_if_t vap = (wlan_if_t)ctx;
    struct wlan_sta_assoc_pending_list *assoc_list = NULL;
    struct wlan_sta_assoc_pending_list *tmp_assoc_list = NULL;
    systime_t current_time;

    current_time = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
    IEEE80211_VAP_LOCK(vap);
    if (!TAILQ_EMPTY(&vap->wlan_sta_assoc_pending_list)) {
        TAILQ_FOREACH_SAFE(assoc_list, &vap->wlan_sta_assoc_pending_list, assoc_pending_list,
                           tmp_assoc_list) {
            if (((current_time - assoc_list->time) > 3000) || vap->force_remove) {
                TAILQ_REMOVE(&vap->wlan_sta_assoc_pending_list, assoc_list, assoc_pending_list);
                qdf_mem_free(assoc_list);
            } else {
                qdf_timer_start(&vap->rem_peer_mac, 3000);
                IEEE80211_VAP_UNLOCK(vap);
                return;
            }
        }
    }
    if (TAILQ_EMPTY(&vap->wlan_sta_assoc_pending_list) && vap->is_timer_started) {
        vap->is_timer_started = 0;
        qdf_timer_stop(&vap->rem_peer_mac);
    }
    IEEE80211_VAP_UNLOCK(vap);
    return;
}

void wlan_ap_vdev_del_assoc_entry(wlan_if_t vap,
                                struct qdf_mac_addr *macaddr) {
    struct wlan_sta_assoc_pending_list *assoc_list = NULL;
    struct wlan_sta_assoc_pending_list *tmp_assoc_list = NULL;

    IEEE80211_VAP_LOCK(vap);
    if (!TAILQ_EMPTY(&vap->wlan_sta_assoc_pending_list)) {
        TAILQ_FOREACH_SAFE(assoc_list, &vap->wlan_sta_assoc_pending_list, assoc_pending_list,
                           tmp_assoc_list) {
            if (qdf_mem_cmp(assoc_list->peer_addr, (u8 *)macaddr, QDF_MAC_ADDR_SIZE) == 0) {
                TAILQ_REMOVE(&vap->wlan_sta_assoc_pending_list, assoc_list, assoc_pending_list);
                qdf_mem_free(assoc_list);
            }
        }
    }
    IEEE80211_VAP_UNLOCK(vap);
    return;
}

void wlan_ap_vdev_add_assoc_entry(wlan_if_t vap,
                                  struct qdf_mac_addr *macaddr) {
    struct wlan_sta_assoc_pending_list *assoc_list = NULL;

    if (!TAILQ_EMPTY(&vap->wlan_sta_assoc_pending_list)) {
        TAILQ_FOREACH(assoc_list, &vap->wlan_sta_assoc_pending_list, assoc_pending_list) {
            if (qdf_mem_cmp(assoc_list->peer_addr, (u8 *)macaddr, QDF_MAC_ADDR_SIZE) == 0)
                return;
        }
    }
    assoc_list = NULL;
    assoc_list = qdf_mem_malloc(sizeof(*assoc_list));
    if (!assoc_list)
        return;
    qdf_mem_copy(assoc_list->peer_addr,(u8 *)macaddr, QDF_MAC_ADDR_SIZE);
    assoc_list->time = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());

    IEEE80211_VAP_LOCK(vap);
    TAILQ_INSERT_TAIL(&vap->wlan_sta_assoc_pending_list, assoc_list, assoc_pending_list);
    if (!vap->is_timer_started) {
        vap->is_timer_started = 1;
        qdf_timer_start(&vap->rem_peer_mac, 3000);
    }
    IEEE80211_VAP_UNLOCK(vap);
    return;
}

struct ieee80211_node *
wlan_add_sta_node(wlan_if_t vap, const u_int8_t *macaddr, uint8_t hold_ref,
                  uint8_t *sta_created, bool is_ml, uint8_t auth_algo)
{
    struct ieee80211_node *ni;
#if WLAN_FEATURE_11BE_MLO
    struct wlan_mlo_peer_context *ml_peer = NULL;
    struct wlan_mlo_dev_context *mld_dev = NULL;
#endif

    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_OBJMGR_ID);
#if WLAN_FEATURE_11BE_MLO
    if (vap->vdev_obj->mlo_dev_ctx) {
        ml_peer = wlan_mlo_get_mlpeer_by_linkmac(vap->vdev_obj->mlo_dev_ctx, (struct qdf_mac_addr *)macaddr);

        if (!ml_peer)
            ml_peer = wlan_mlo_get_mlpeer_by_peer_mladdr((struct qdf_mac_addr *)macaddr, &mld_dev);
    }

    if (is_ml) {
        if (auth_algo != IEEE80211_AUTH_ALG_FT) {
            if (ml_peer) {
                wlan_mlo_peer_delete(ml_peer);
            } else if (ni) {
                if (wlan_peer_is_mlo(ni->peer_obj)) {
                    wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
                }
                IEEE80211_NODE_LEAVE(ni);
            }

        }

        wlan_mlo_ap_vdev_add_assoc_entry(vap->vdev_obj,
                                         (struct qdf_mac_addr *)macaddr);
        if (ni) {
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
            ni = NULL;
        }

        *sta_created = 0;
        return NULL;
    }

     if (ni == NULL && !ml_peer) {
#else
    if (ni == NULL) {
#endif
        ni = _ieee80211_find_logically_deleted_node(vap->iv_ic, macaddr, NULL, WLAN_MLME_SB_ID);
        if (ni) {
#if WLAN_FEATURE_11BE_MLO
            mlo_debug("%s: Found logically deleted node with STA-MAC: "
                      QDF_MAC_ADDR_FMT "on VAP: "
                      QDF_MAC_ADDR_FMT "while trying to add on VAP "
                      QDF_MAC_ADDR_FMT, __func__,
                      QDF_MAC_ADDR_REF(macaddr),
                      QDF_MAC_ADDR_REF(ni->ni_vap->iv_myaddr),
                      QDF_MAC_ADDR_REF(vap->iv_myaddr));
#endif
            ieee80211_free_node(ni, WLAN_MLME_SB_ID);
            return NULL;
        }
        ni = ieee80211_dup_bss(vap, macaddr);
        if(ni != NULL) {
           wlan_node_set_peer_state(ni, WLAN_AUTH_STATE);
        }
    } else if (auth_algo != IEEE80211_AUTH_ALG_FT) {
#if WLAN_FEATURE_11BE_MLO
        if (ml_peer) {
            wlan_mlo_peer_delete(ml_peer);
        } else {
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
            IEEE80211_NODE_LEAVE(ni);
        }
#else

        IEEE80211_NODE_LEAVE(ni);
#endif
        wlan_ap_vdev_add_assoc_entry(vap, (struct qdf_mac_addr *)macaddr);
        if (ni) {
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
            ni = NULL;
        }
        *sta_created = 0;
        return NULL;
    }

    if (ni == NULL) {
        *sta_created = 0;
        return NULL;
    }
    /* claim node immediately */
    if (auth_algo == IEEE80211_AUTH_ALG_FT){
        ni->is_ft_reauth = 1;
        ni->ni_authalg = IEEE80211_AUTH_ALG_FT;
    }
    if (!hold_ref) {
        ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
        ni = NULL;
    }
    *sta_created = 1;
    return ni;
}

