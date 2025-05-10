/*
 *
 * Copyright (c) 2011-2016,2017-2018 Qualcomm Innovation Center, Inc.
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

#include "ieee80211_node_priv.h"
#if WLAN_SUPPORT_GREEN_AP
#include <wlan_green_ap_api.h>
#endif

/* should this be configurable ?*/
#define IEE80211_STA_MAX_NODE_SAVEQ_LEN 300

#include <wlan_mlo_mgr_sta.h>
#include "wlan_reg_channel_api.h"
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
extern void
ieee80211_inherit_bss_self_peer(struct ieee80211_node *ni,
                                struct wlan_mlo_peer_context *ml_peer);
#endif
/*
 * Join an infrastructure network
 */
int
ieee80211_sta_join(struct ieee80211vap *vap, ieee80211_scan_entry_t scan_entry,
                   bool *thread_started, struct qdf_mac_addr *mld_mac)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni = NULL;
    const u_int8_t *macaddr = NULL;
    struct ieee80211vap *tempvap = NULL;
#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    struct wlan_objmgr_peer *peer = NULL;
#endif
    int error = 0;
    struct chan_params chan_p = {0};


    if (mlo_is_sta_bridge_vdev(vap->vdev_obj)) {
        /* Acquire lock as required by wlan_vdev_mlme_get_mldaddr() */
        wlan_vdev_obj_lock(vap->vdev_obj);
        macaddr = wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
        wlan_vdev_obj_unlock(vap->vdev_obj);
    }
    else
        macaddr = util_scan_entry_macaddr(scan_entry);

    ASSERT(vap->iv_opmode == IEEE80211_M_STA);
    /*If AP mac to which our sta vap is trying to connect has
    same mac as one of our ap vaps ,dont set that as sta bssid */
    TAILQ_FOREACH(tempvap, &ic->ic_vaps, iv_next) {
        if (tempvap->iv_opmode == IEEE80211_M_HOSTAP && IEEE80211_ADDR_EQ(tempvap->iv_myaddr,macaddr)) {
             qdf_print("[%s] Mac collision for [%s]",__func__,ether_sprintf(tempvap->iv_myaddr));
             return -EINVAL;
        }
    }

    ni = ieee80211_vap_find_node(vap, macaddr, WLAN_MLME_OBJMGR_ID);
    if (ni) {
        /*
         * reusing old node has a potential for several bugs . The old node may have some state info from previous association.
         * get rid of the old bss node and create a new bss node.
         */
        ieee80211_sta_leave(ni);
        ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
    }
    /*
     * Create a BSS node.
     */
    if (wlan_psoc_nif_feat_cap_get(wlan_pdev_get_psoc(ic->ic_pdev_obj),
                                   WLAN_SOC_F_PEER_CREATE_RESP)) {
        vap->iv_bss_rsp_evt_status = false;
        vap->iv_bss_rsp_status = 0;
    } else {
        wlan_vap_set_bss_status(vap, PEER_CREATED);
    }
    /* Allocate a Bridge node for WDS STA vap if bridge vdev exists */
    if (mlo_is_sta_bridge_vdev(vap->vdev_obj))
        ni = wlan_objmgr_alloc_mlo_bridge_node(vap, (uint8_t *)macaddr);
    else
        ni = wlan_objmgr_alloc_ap_node(vap, (uint8_t *)macaddr);

    if (!ni) {
        if (wlan_vdev_get_peer_count(vap->vdev_obj) >=
                wlan_vdev_get_max_peer_count(vap->vdev_obj)) {
            qdf_err("Peer create failed for %s", ether_sprintf(macaddr));
            ieee80211_vap_peer_create_failed_set(vap);
        }
        qdf_err("Unable to allocate AP peer, peer cnt:%d, max_peer:%d vapid:%d",
                wlan_vdev_get_peer_count(vap->vdev_obj),
                wlan_vdev_get_max_peer_count(vap->vdev_obj),
                vap->iv_unit);
        return -ENOMEM;
    }

#if defined(WLAN_FEATURE_11BE_MLO) && defined(WLAN_MLO_MULTI_CHIP)
    /* Populate the minimum info for bridge peer */
    if (mlo_is_sta_bridge_vdev(vap->vdev_obj)) {
        ni->ni_authmode = wlan_crypto_get_param(vap->vdev_obj, WLAN_CRYPTO_PARAM_AUTH_MODE);
        wlan_crypto_set_peer_param(ni->peer_obj, WLAN_CRYPTO_PARAM_AUTH_MODE, ni->ni_authmode);
        ni->ni_txpower = vap->iv_bss->ni_txpower;
        peer = ni->peer_obj;
        peer->mlo_bridge_peer = true;
        ieee80211_inherit_bss_self_peer(ni, ni->peer_obj->mlo_peer_ctx);
        IEEE80211_ADDR_COPY(ni->ni_bssid, macaddr);
        qdf_info("MLO STA Bridge peer allocated %s", ether_sprintf(macaddr));
        /* If bridge vap does not have 11be phymode */
        if (mlo_is_mld_sta(vap->vdev_obj) && ni->ni_phymode < IEEE80211_MODE_11BEA_EHT20) {
            if (wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_2G)) {
                vap->iv_des_mode = IEEE80211_MODE_11BEG_EHT20;
                vap->iv_des_hw_mode = vap->iv_des_mode;
                ni->ni_phymode = vap->iv_des_mode;
            } else if (wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_5G) ||
                       wlan_reg_is_band_present(ic->ic_pdev_obj, REG_BAND_6G)) {
                vap->iv_des_mode = IEEE80211_MODE_11BEA_EHT20;
                vap->iv_des_hw_mode = vap->iv_des_mode;
                ni->ni_phymode = vap->iv_des_mode;
            } else {
                qdf_err("Invalid Band derived from channel for bridge VAP [%u, %u]",
                wlan_vdev_get_id(vap->vdev_obj),
                wlan_vdev_get_psoc_id(vap->vdev_obj));

                ieee80211_sta_leave(ni);
                /* Release ref, which was taken to attach to ni-table */
                ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
                return error;
            }
        }
    }
#endif
    /* set the maximum number frmaes to be queued when the vap is in fake sleep */
    ieee80211_node_saveq_set_param(ni,IEEE80211_NODE_SAVEQ_DATA_Q_LEN,IEE80211_STA_MAX_NODE_SAVEQ_LEN);
    /* To become a bss node, a node need an extra reference count, which alloc node already gives */

    wlan_node_set_peer_state(ni, WLAN_JOIN_STATE);
    /* setup the bss node for association */
    if (scan_entry) {
        error = ieee80211_setup_node(ni, scan_entry);
        if (error != 0) {
            ieee80211_sta_leave(ni);
            /* Release ref, which was taken to attach to ni-table */
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
            return error;
        }
    }

#ifdef WLAN_FEATURE_11BE_MLO
    if (mld_mac)
        qdf_mem_copy(ni->peer_obj->mldaddr, mld_mac, QDF_MAC_ADDR_SIZE);
#endif
    /* copy the beacon timestamp */
    if (scan_entry)
        OS_MEMCPY(ni->ni_tstamp.data,
                  util_scan_entry_tsf(scan_entry),
                  sizeof(ni->ni_tstamp));

    /*
     * Join the BSS represented by this new node.
     * This function will free up the old BSS node
     * and use this one as the new BSS node.
     */
    ieee80211_sta_join_bss(ni);

    /* Save our home channel */
    chan_p.freq = ni->ni_ch_param.ni_freq;
    chan_p.cfreq2 = ni->ni_ch_param.ni_cfreq2;
    set_chan_params_puncture_bitmap(&chan_p, ni->ni_ch_param.puncture_bitmap);

    *vap->iv_bsschan = ieee80211_find_dot11_channel(ic, &chan_p,
						   ni->ni_ch_param.ni_bss_phymode);

    vap->iv_cur_mode = ni->ni_ch_param.ni_bss_phymode;

    /*
     * Cancel all current ACS scans on other AP VAPs
     * and ensure that we report the STA VAP's iv_bsschan
     * back to hostapd for all the AP VAPs.
     */
    wlan_iterate_vap_list(ic, osif_cancel_acs_and_start_ap, vap);

    /* Update the DotH falg */
    ieee80211_update_spectrumrequirement(vap, thread_started);

    /*
     *  The OS will control our security keys.
     *  If clear, keys will be cleared.
     *  If static WEP, keys will be plumbed before JoinInfra.
     *  If WPA/WPA2, ciphers will be setup, but no keys will be plumbed until
     *    after they are negotiated.
     *  XXX We should ASSERT that all of the foregoing is true.
     */
    return 0;
}

struct ieee80211_node *
ieee80211_mlo_tmp_node(struct ieee80211vap *vap, const u_int8_t *macaddr)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;

    if (!(IEEE80211_ADDR_IS_VALID(macaddr))) {
        qdf_info("Invalid Mac Address");
        return NULL;
    }

    /*
     * if vap is being deleted, do not allow new allocations.
     */
    if (ieee80211_vap_deleted_is_set(vap)) {
        return NULL;
    }
    /* Allocate TEMP peer object */
    ni = wlan_objmgr_alloc_mlo_tmp_node(vap, (uint8_t *)macaddr);
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

    ni->ni_bss_node = ni;
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

    ni->ni_ath_defkeyindex = IEEE80211_INVAL_DEFKEY;

    /* 11n  or 11ac */
    ni->ni_chwidth = ic->ic_cwm_get_width(ic);

    return ni;
}
