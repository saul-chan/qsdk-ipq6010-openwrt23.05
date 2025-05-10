/*
 * Copyright (c) 2013-2023 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2013-2016 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
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

#include <osdep.h>


#include <ieee80211_var.h>
#include <ieee80211_node.h>
#include <wbuf.h>
#include <ieee80211_rateset.h>
#include <mlme/ieee80211_mlme_priv.h>
#include <linux/crypto.h>

#include "ol_if_athvar.h"
#include <wlan_mlme_dp_dispatcher.h>
#include "wlan_crypto_global_def.h"
#include "wlan_crypto_global_api.h"
#ifdef QCA_SUPPORT_CP_STATS
#include <wlan_cp_stats_ic_utils_api.h>
#endif
#include <wlan_vdev_mlme.h>
#ifdef WLAN_FEATURE_11BE_MLO
#include <ieee80211_mlo.h>
#include <wlan_mlo_mgr_ap.h>
#include <wlan_mlo_mgr_peer.h>
#include <wlan_mlo_mgr_main.h>
#endif

/* FIXME : Commented calls to lock_bh */

/* Add wds address to the node table */
static inline int
_ieee80211_add_wds_addr(struct ieee80211vap *vaphandle,
		       struct ieee80211_node *ni, const u_int8_t *macaddr,
		       u_int32_t flags)
{
    int hash;
    struct ieee80211_wds_addr *wds;
    struct ieee80211_node_table *nt = &vaphandle->iv_ic->ic_sta;
    struct ieee80211com *ic = nt->nt_ic;

    rwlock_state_t lock_state;

    wds = (struct ieee80211_wds_addr *) OS_MALLOC(ni->ni_ic->ic_osdev,
            sizeof(struct ieee80211_wds_addr), GFP_KERNEL );

    if (wds == NULL) {
	    /* XXX msg */
	    return 1;
	}

    hash = IEEE80211_NODE_HASH(macaddr);
    IEEE80211_ADDR_COPY(wds->wds_macaddr, macaddr);
    if (!ieee80211_try_ref_node(ni, WLAN_WDS_ID)) {             /* Reference node */
        OS_FREE(wds);
        return -1;
    }

    wds->flags = flags;
    wds->wds_ni = ni;

    IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_WDS,  "%s: vap %pK Adding mac(%s)"
		    " to host wds table wds_flags:0x%x flags:0x%x\n", __func__,
		    vaphandle, ether_sprintf(macaddr), wds->flags, flags);

    OS_RWLOCK_WRITE_LOCK(&nt->nt_wds_nodelock, &lock_state);
    LIST_INSERT_HEAD(&nt->nt_wds_hash[hash], wds, wds_hash);
    OS_RWLOCK_WRITE_UNLOCK(&nt->nt_wds_nodelock, &lock_state);

    return ic->ic_node_add_wds_entry((void *)(vaphandle->iv_ifp), macaddr, ni->ni_macaddr, flags);
}

int
ieee80211_add_wds_addr(struct ieee80211vap *vaphandle,
                      struct ieee80211_node_table *nt,
		      struct ieee80211_node *ni, const u_int8_t *macaddr,
		      u_int32_t flags)
{
    return _ieee80211_add_wds_addr(vaphandle, ni, macaddr, flags);
}

/* remove wds address from the wds hash table */
void
ieee80211_remove_wds_addr(wlan_if_t vaphandle,
                          struct ieee80211_node_table *nt,
			  const u_int8_t *macaddr, u_int32_t flags)
{
    int hash;
    struct ieee80211_wds_addr *wds;
    struct ieee80211com *ic = nt->nt_ic;
    u_int8_t *destmac = (u_int8_t *) macaddr;
    bool is_node_delete = 0;
    rwlock_state_t lock_state;

    OS_RWLOCK_WRITE_LOCK(&nt->nt_wds_nodelock, &lock_state);

    hash = IEEE80211_NODE_HASH(macaddr);
    LIST_FOREACH(wds, &nt->nt_wds_hash[hash], wds_hash) {
        if (IEEE80211_ADDR_EQ(wds->wds_macaddr, macaddr)) {
            if (
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
                    ((wds->flags & IEEE80211_NODE_F_WDS_HM) == (flags & IEEE80211_NODE_F_WDS_HM)) &&
#endif
                    (wds->flags & flags)
                    ) {
                ieee80211_free_node(wds->wds_ni, WLAN_WDS_ID);  /* Decrement ref count */
                LIST_REMOVE(wds, wds_hash);
                IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_WDS,  "%s: vap %pK deleting mac(%s)"
                    " from host wds table wds_flags:0x%x flags:0x%x\n", __func__,
                    vaphandle, ether_sprintf(macaddr), wds->flags, flags);
                OS_FREE(wds);
                is_node_delete = 1;
            } else {
                IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_WDS, "%s: vap %pK Not delete Mac(%s)"
                    " from host wds table wds_flags:0x%x flags:0x%x\n", __func__,
                    vaphandle, ether_sprintf(macaddr), wds->flags,flags);
            }
            break;
        }
    }
    OS_RWLOCK_WRITE_UNLOCK(&nt->nt_wds_nodelock, &lock_state);
    if (is_node_delete || (flags & IEEE80211_NODE_F_WDS_HM))
    {
        IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_WDS, "%s: vap %pK Call FW to delete mac(%s)"
            " from FW table flags:0x%x\n", __func__, vaphandle, ether_sprintf(macaddr), flags);
        ic->ic_node_del_wds_entry((void *)(vaphandle->iv_ifp), destmac);
    }
}

#ifdef IPA_OFFLOAD
/* remove wds address from the wds hash table */
void
ieee80211_remove_ipa_wds_addr(wlan_if_t vaphandle, const u_int8_t *macaddr,
                              const u_int8_t *peer_macaddr)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    u_int8_t *destmac = (u_int8_t *)macaddr;
    u_int8_t *peer_mac = (u_int8_t *)peer_macaddr;
    int status;

    IEEE80211_DPRINTF_IC_CATEGORY(ic, IEEE80211_MSG_WDS, "%s: vap %pK Call FW to delete mac(%s)"
                    " from FW table \n", __func__, vaphandle, ether_sprintf(macaddr));

    /* Delete The AST entry only if end-node has not roamed,
     * as the for roamed client the AST entry will be simply
     * updated by IPA, and by checking for roaming here we
     * are avoiding race condition between delete and create
     * event for AST entry.
     */

    status = ic->ic_node_get_roaming_status((void *)(vaphandle->iv_ifp), destmac, peer_mac);

    if(!status) {
        ic->ic_node_del_wds_entry((void *)(vaphandle->iv_ifp), destmac);
        /* Send WDS Client Disconnect event to IPA */
        if (ic->ic_ipa_wds_peer_disconnect)
                ic->ic_ipa_wds_peer_disconnect(vap, destmac);
        else
                qdf_err("Failed to send client disconnect event to IPA !!!");

        qdf_debug("Deleted the AST entry for MAC: "QDF_MAC_ADDR_FMT,
                        QDF_MAC_ADDR_REF(destmac));
    }
    else if(status == -1) {
        qdf_err("Invalid response !!!");
        return;
    }

}
#endif

/* Remove node references from wds table */
void
ieee80211_del_wds_node(struct ieee80211_node_table *nt,
                      struct ieee80211_node *ni)
{
    int hash;
    struct ieee80211_wds_addr *wds;
    struct ieee80211_wds_addr *wds_next = NULL;

    for (hash=0; hash<IEEE80211_NODE_HASHSIZE; hash++) {
        for ((wds) = LIST_FIRST((&nt->nt_wds_hash[hash])); (wds);) {
            if (wds->wds_ni == ni) {
                /* Instead of freeing the node, make sure that the wds entry is
                 * flagged as stage wds_addr, and force the node pointer to be
                 * NULL. When actual staging timer expires, free the wds entry.
                 */
                wds_next = LIST_NEXT(wds, wds_hash);
                wds->wds_ni = NULL;
                /* cache the address of the node */
                OS_MEMCPY(wds->wds_ni_macaddr, ni->ni_macaddr, QDF_MAC_ADDR_SIZE);
                ieee80211_free_node(ni, WLAN_WDS_ID);
                wds = wds_next;
            } else {
                wds = LIST_NEXT(wds, wds_hash);
            }
        }
       }
}

void
ieee80211_wds_attach(struct ieee80211_node_table *nt)
{

    OS_RWLOCK_INIT(&nt->nt_wds_nodelock);
}

void
ieee80211_wds_detach(struct ieee80211_node_table *nt)
{
    OS_RWLOCK_DESTROY(&nt->nt_wds_nodelock);
}


static struct ieee80211_node *
_ieee80211_find_wds_node(
        struct ieee80211_node_table *nt,
        const u_int8_t *macaddr,
        wlan_objmgr_ref_dbgid id,
        struct ieee80211_wds_addr **wds_stag)
{
    struct ieee80211_node *ni;
    struct ieee80211_wds_addr *wds;
    int hash;

    hash = IEEE80211_NODE_HASH(macaddr);
    LIST_FOREACH(wds, &nt->nt_wds_hash[hash], wds_hash) {
        if (IEEE80211_ADDR_EQ(wds->wds_macaddr, macaddr)) {

            ni = wds->wds_ni;
            if (ni) {
                if (!ieee80211_try_ref_node(ni, id))
                    return NULL;
            }
#if ATH_SUPPORT_HYFI_ENHANCEMENTS
            *wds_stag = wds;
#endif
            return ni;
        }
    }
    return NULL;
}

/* Remove all the wds entries associated with the AP when the AP to
 * which STA is associated goes down
 */
int ieee80211_node_removeall_wds (struct ieee80211_node_table *nt,struct ieee80211_node *ni)
{
    unsigned int hash;
    struct ieee80211_wds_addr *wds;
    struct ieee80211_wds_addr *wds_next;
    rwlock_state_t lock_state;
    OS_RWLOCK_WRITE_LOCK(&nt->nt_wds_nodelock, &lock_state);
    for (hash=0 ;hash < IEEE80211_NODE_HASHSIZE;hash++) {
        for ((wds) = LIST_FIRST((&nt->nt_wds_hash[hash])); (wds);) {
            if (wds->wds_ni == ni) {
                wds_next = LIST_NEXT(wds,wds_hash);
                ieee80211_free_node(wds->wds_ni, WLAN_WDS_ID);
                LIST_REMOVE(wds, wds_hash);
                OS_FREE(wds);
                wds = wds_next;
            } else {
                wds = LIST_NEXT(wds, wds_hash);
            }
        }
	}
    OS_RWLOCK_WRITE_UNLOCK(&nt->nt_wds_nodelock, &lock_state);
    return 0;
}

struct ieee80211_node *
__ieee80211_find_wds_node(struct ieee80211_node_table *nt,
        const u_int8_t *macaddr, wlan_objmgr_ref_dbgid id,
        struct ieee80211_wds_addr **wds)
{
    struct ieee80211_node *ni;

    rwlock_state_t lock_state;
    OS_RWLOCK_WRITE_LOCK(&nt->nt_wds_nodelock, &lock_state);
    ni = _ieee80211_find_wds_node(nt, macaddr, id, wds);
    OS_RWLOCK_WRITE_UNLOCK(&nt->nt_wds_nodelock, &lock_state);
    return ni;
}

struct ieee80211_node *
ieee80211_find_wds_node(struct ieee80211_node_table *nt,
        const u_int8_t *macaddr, wlan_objmgr_ref_dbgid id)
{
    struct ieee80211_wds_addr *wds = NULL;

    return __ieee80211_find_wds_node(nt, macaddr, id, &wds);
}

void
wds_clear_wds_table(struct ieee80211_node * ni, struct ieee80211_node_table *nt, wbuf_t wbuf )
{
    struct ieee80211_frame *wh;
    rwlock_state_t lock_state;
    wh = (struct ieee80211_frame *) wbuf_header(wbuf);

    if (ni != ni->ni_vap->iv_bss) {
        struct ieee80211_node *ni_wds=NULL;
        ni_wds = ieee80211_find_wds_node(nt,wh->i_addr2,WLAN_WDS_ID);

        if (ni_wds) {
            OS_BEACON_DECLARE_AND_RESET_VAR(flags);
            OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
            (void) ieee80211_remove_wds_addr(ni->ni_vap, nt,wh->i_addr2,IEEE80211_NODE_F_WDS_BEHIND | IEEE80211_NODE_F_WDS_REMOTE);
            OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
            ieee80211_free_node(ni_wds,WLAN_WDS_ID);
        }
    }
}

void
ieee80211_nawds_learn(struct ieee80211vap *vap, u_int8_t *mac);

void
ieee80211_nawds_learn_deferwork(void *arg)

{
    struct ieee80211com *ic = (struct ieee80211com *)arg;
    nawds_dentry_t * nawds_entry = NULL;
    do {

        qdf_spin_lock_bh(&ic->ic_nawdsdefer_lock);
        nawds_entry = TAILQ_FIRST(&ic->ic_nawdslearnlist);
        if(nawds_entry)
            TAILQ_REMOVE(&ic->ic_nawdslearnlist,nawds_entry,nawds_dlist);
        qdf_spin_unlock_bh(&ic->ic_nawdsdefer_lock);
        if(!nawds_entry)
            break;
        ieee80211_nawds_learn(nawds_entry->vap, &nawds_entry->mac[0]);
        qdf_mem_free(nawds_entry);

    }while(1);

}
qdf_export_symbol(ieee80211_nawds_learn_deferwork);

void
ieee80211_nawds_learn_defer(struct ieee80211vap *vap, u_int8_t *mac)
{

    struct ieee80211com *ic = vap->iv_ic;
    nawds_dentry_t * nawds_entry ;

    if (ic->nawds_learn_defer) {
        nawds_entry = ( nawds_dentry_t * )  qdf_mem_malloc(sizeof(nawds_dentry_t));
        if (nawds_entry !=NULL) {
            nawds_entry->vap = vap;

            qdf_mem_copy(&nawds_entry->mac[0],mac,QDF_MAC_ADDR_SIZE);

            qdf_spin_lock_bh(&ic->ic_nawdsdefer_lock);
            TAILQ_INSERT_TAIL(&ic->ic_nawdslearnlist, nawds_entry, nawds_dlist);
            qdf_spin_unlock_bh(&ic->ic_nawdsdefer_lock);
            qdf_queue_work(0, ic->ic_nawds_wq, &ic->ic_nawds_work);
        }
    }
}

#if UMAC_SUPPORT_NAWDS

#ifndef UMAC_MAX_NAWDS_REPEATER
#error NAWDS feature is enabled but UMAC_MAX_NAWDS_REPEATER is not defined
#endif

/* codes for Non-Associated WDS - NAWDS */
/* common functions for UMAC and MLME support */
static int
is_nawds_valid_mac(char *addr)
{
    char nullmac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    if (IEEE80211_IS_MULTICAST(addr) ||
        IEEE80211_ADDR_EQ(addr, nullmac))
        return 0;

    return 1;
}

static bool
is_nawds_mac_bss_mac(struct ieee80211com *ic, char *mac)
{
    struct ieee80211_node *ni = NULL;

    ni = ieee80211_find_node(ic, mac, WLAN_WDS_ID);
    if (ni) {
        if (ni->ni_vap->iv_bss == ni) {
            ieee80211_free_node(ni, WLAN_WDS_ID);
            return true;
        }
        ieee80211_free_node(ni, WLAN_WDS_ID);
    }

    return false;
}

static int
is_nawds_valid_caps(struct ieee80211vap *vap, struct ieee80211com *ic, u_int64_t caps)
{
    u_int32_t ht_flags, vht_flags, he_flags, he_axa_flags, he_axg_flags;
    bool is_160_or_80p80_supported = false;
    struct ieee80211_ath_channel *chan = vap->iv_bsschan;
#ifdef WLAN_FEATURE_11BE
    u_int64_t eht_flags;
    bool is_320_supported = false;
#endif

    if (ic->ic_modecaps &
                ((1 << IEEE80211_MODE_11AC_VHT160) |
                 (1 << IEEE80211_MODE_11AC_VHT80_80) |
                 (1 << IEEE80211_MODE_11AXA_HE160) |
                 (1ULL << IEEE80211_MODE_11AXA_HE80_80)
#ifdef WLAN_FEATURE_11BE
                 | (1ULL << IEEE80211_MODE_11BEA_EHT160)
#endif /*WLAN_FEATURE_11BE */
                )) {
        is_160_or_80p80_supported = true;
    }

#ifdef WLAN_FEATURE_11BE
    if (ic->ic_modecaps & (1ULL << IEEE80211_MODE_11BEA_EHT320)) {
        is_320_supported = true;
    }
#endif /* WLAN_FEATURE_11BE */

    if(caps & NAWDS_INVALID_CAP_MODE)
        return 0;

    if(!is_160_or_80p80_supported && (caps & (NAWDS_REPEATER_CAP_11ACVHT80_80 |
                                              NAWDS_REPEATER_CAP_11ACVHT160 |
                                              NAWDS_REPEATER_CAP_11AXAHE80_80 |
                                              NAWDS_REPEATER_CAP_11AXAHE160
#ifdef WLAN_FEATURE_11BE
                                              | NAWDS_REPEATER_CAP_11BEAEHT160
#endif /*WLAN_FEATURE_11BE */
                                             )))
        return 0;

    ht_flags  = (NAWDS_REPEATER_CAP_HT20 | NAWDS_REPEATER_CAP_HT2040);
    vht_flags = (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_11ACVHT80);
    he_axa_flags = (NAWDS_REPEATER_CAP_11AXAHE20 | NAWDS_REPEATER_CAP_11AXAHE40 |
			NAWDS_REPEATER_CAP_11AXAHE80 | NAWDS_REPEATER_CAP_11AXAHE160);
    he_axg_flags = (NAWDS_REPEATER_CAP_11AXGHE20 | NAWDS_REPEATER_CAP_11AXGHE40);
    he_flags = he_axa_flags | he_axg_flags;
#ifdef WLAN_FEATURE_11BE
    eht_flags = (NAWDS_REPEATER_CAP_11BEAEHT20  |
                 NAWDS_REPEATER_CAP_11BEGEHT20  |
                 NAWDS_REPEATER_CAP_11BEAEHT40  |
                 NAWDS_REPEATER_CAP_11BEGEHT40  |
                 NAWDS_REPEATER_CAP_11BEAEHT80  |
                 NAWDS_REPEATER_CAP_11BEAEHT160 |
                 NAWDS_REPEATER_CAP_11BEAEHT320);

    if (is_320_supported) {
        he_flags  |= NAWDS_REPEATER_CAP_11AXAHE160;
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT320;
    }
#endif /* WLAN_FEATURE_11BE */

    if (IEEE80211_IS_CHAN_6GHZ(chan)) {
        if ((!(caps & he_axa_flags))
#ifdef WLAN_FEATURE_11BE
            && (!(caps & eht_flags))) {
#else
           ) {
#endif
            qdf_err("Invalid caps, For 6GHz NAWDS 11AXA mode only allowed");
            return 0;
        }
    }
    if(is_160_or_80p80_supported) {
        vht_flags |= (NAWDS_REPEATER_CAP_11ACVHT80_80 | NAWDS_REPEATER_CAP_11ACVHT160);
        he_flags |= (NAWDS_REPEATER_CAP_11AXAHE80_80 |  NAWDS_REPEATER_CAP_11AXAHE160);
#ifdef WLAN_FEATURE_11BE
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT160;
#endif /* WLAN_FEATURE_11BE */
    }
    if (caps & (NAWDS_MULTI_STREAMS)) {
        if (!((caps & he_flags) || (caps & vht_flags) || (caps & ht_flags)
#ifdef WLAN_FEATURE_11BE
              || (caps & eht_flags)
#endif /* WLAN_FEATURE_11BE */
             )) {
            return 0;
        }
    }

    if (caps & NAWDS_5TO8_STREAMS) {
        if (!((caps & he_axa_flags) || (caps & vht_flags)
#ifdef WLAN_FEATURE_11BE
              || (caps & eht_flags)
#endif
             ))
            return 0;
    }

    // check if radio nss caps match
    if ((caps & NAWDS_REPEATER_CAP_8S) && (ic->ic_spatialstreams < 8)) {
        return 0;
    }
    else if ((caps & NAWDS_REPEATER_CAP_7S) && (ic->ic_spatialstreams < 7)) {
        return 0;
    }
    else if ((caps & NAWDS_REPEATER_CAP_6S) && (ic->ic_spatialstreams < 6)) {
        return 0;
    }
    else if ((caps & NAWDS_REPEATER_CAP_5S) && (ic->ic_spatialstreams < 5)) {
        return 0;
    }
    else if ((caps & NAWDS_REPEATER_CAP_4S) && (ic->ic_spatialstreams < 4)) {
        return 0;
    }
    else if ((caps & NAWDS_REPEATER_CAP_TS) && (ic->ic_spatialstreams < 3)) {
        return 0;
    }
    else if ((caps & NAWDS_REPEATER_CAP_DS) && (ic->ic_spatialstreams < 2)) {
        return 0;
    }
    return 1;
}

/* UMAC Support Functions */
void
ieee80211_nawds_attach(struct ieee80211vap *vap)
{
    OS_MEMZERO(&vap->iv_nawds, sizeof(struct ieee80211_nawds));
    NAWDS_LOCK_INIT(&vap->iv_nawds.lock);
}

int ieee80211_nawds_send_wbuf(struct ieee80211vap *vap, wbuf_t wbuf)
{
    int i, count = 0;
    struct ieee80211_node *rep_ni, *src_ni, *wbuf_ni = NULL;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    struct ieee80211com *ic = vap->iv_ic;
    struct ether_header *eh;
    wbuf_t wbuf1 = NULL;

    /* Do not send nawds packet when mode is off or node is inactive */
    wbuf_ni = wlan_wbuf_get_peer_node(wbuf);
    if ((wbuf_ni != NULL) &&
        (wbuf_ni->ni_flags & IEEE80211_NODE_NAWDS)) {
        if ((nawds->mode == IEEE80211_NAWDS_DISABLED) ||
            (wbuf_ni->ni_inact <= 1)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                "%s: nawds mode off or node inactive\n", __func__);
            ieee80211_free_node(wbuf_ni, WLAN_MLME_SB_ID);
            wbuf_complete(wbuf);
            return -1;
        }
    }

    if (nawds->mode == IEEE80211_NAWDS_DISABLED)
        return 0;

    eh = (struct ether_header *)wbuf_header(wbuf);

    if (!((vap->iv_flags_ext & IEEE80211_FEXT_WDS) &&
         (IEEE80211_IS_MULTICAST(eh->ether_dhost))))
        return 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "nawds: %s: ",
            ether_sprintf(eh->ether_dhost));

    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
        if (!is_nawds_valid_mac(nawds->repeater[i].mac))
            continue;
        if ((rep_ni = ieee80211_find_node(ic, nawds->repeater[i].mac, WLAN_WDS_ID)) == NULL) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "missing node: %s\n",
                    ether_sprintf(nawds->repeater[i].mac));
            continue;
        }
        if (!(rep_ni->ni_flags & IEEE80211_NODE_NAWDS)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "node without flags: %s\n",
                    ether_sprintf(nawds->repeater[i].mac));
            ieee80211_free_node(rep_ni, WLAN_WDS_ID);
            continue;
        }
        if (rep_ni->ni_inact <= 1) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "node inactive: %s\n",
                    ether_sprintf(nawds->repeater[i].mac));
            ieee80211_free_node(rep_ni, WLAN_WDS_ID);
            continue;
        }
        /* To avoid a bcast storm we need to check if the src is reachable
         * over this repeater; if it is, skip this process for this
         * repeater alone and contine to send on other repeaters
         */
        src_ni = ieee80211_find_txnode(vap, eh->ether_shost, WLAN_WDS_ID);
        if (rep_ni == src_ni) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "drop: %s\n",
                    ether_sprintf(nawds->repeater[i].mac));
            ieee80211_free_node(rep_ni, WLAN_WDS_ID);
            ieee80211_free_node(src_ni, WLAN_WDS_ID);
            continue;
        }
        if (src_ni)
            ieee80211_free_node(src_ni, WLAN_WDS_ID);

        /* copy buf and send it out */
        wbuf1 = wbuf_copy(wbuf);
        if(wbuf1 == NULL) {
           /* wbuf_copy() failed */
           ieee80211_free_node(rep_ni, WLAN_WDS_ID);
           return 0;
        }
        wlan_wbuf_set_peer_node(wbuf1, rep_ni);
        wbuf_clear_flags(wbuf1);
        vap->iv_evtable->wlan_dev_xmit_queue(vap->iv_ifp, wbuf1);

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "send: %s ref: %d\n",
                ether_sprintf(nawds->repeater[i].mac),
                ieee80211_node_refcnt(rep_ni));
        count++;
    }

    return count;
}

int
ieee80211_nawds_disable_beacon(struct ieee80211vap *vap)
{
    if ((vap->iv_nawds.mode == IEEE80211_NAWDS_STATIC_BRIDGE) ||
        (vap->iv_nawds.mode == IEEE80211_NAWDS_LEARNING_BRIDGE))
        return 1;
    return 0;
}

int
ieee80211_nawds_enable_learning(struct ieee80211vap *vap)
{
    if ((vap->iv_nawds.mode == IEEE80211_NAWDS_LEARNING_REPEATER) ||
        (vap->iv_nawds.mode == IEEE80211_NAWDS_LEARNING_BRIDGE))
        return 1;
    return 0;
}

void
ieee80211_nawds_learn(struct ieee80211vap *vap, u_int8_t *mac)
{
#ifdef WLAN_FEATURE_11BE
    wlan_nawds_config_mac(vap,
                          mac,
                          vap->iv_nawds.defcaps,
                          vap->iv_nawds.def_puncture_bitmap);
#else
    wlan_nawds_config_mac(vap, mac, vap->iv_nawds.defcaps, 0);
#endif
    if (wlan_nawds_check_if_auth_fail(vap, mac)) {
        OS_SLEEP_INTERRUPTIBLE(NAWDS_AUTH_RETRY_DELAY);
        wlan_nawds_node_authorize(vap, mac);
        if (wlan_nawds_check_if_auth_fail(vap, mac))
            qdf_err("nawds dynamic node auth retry failed! investigate");
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "NAWDS repeater learned %s: %d\n",
            ether_sprintf(mac),vap->iv_nawds.defcaps);
    wlan_nawds_config_key(vap, mac, vap->iv_nawds.psk);
}


/* IEEE80211 MLME support functions */
static void
ieee80211_nawds_node_leave(wlan_if_t vaphandle, u_int8_t *addr)
{
    int i;
    struct ieee80211_node* ni;
	struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    struct ieee80211com *ic = vap->iv_ic;

    for(i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
        if (IEEE80211_ADDR_EQ(nawds->repeater[i].mac, addr))
            break;
    }

    if (i == UMAC_MAX_NAWDS_REPEATER)
        return;

    /* reclaim the node */
    ni = ieee80211_find_node(ic, addr, WLAN_WDS_ID);
    if (ni) {
#ifdef QCA_SUPPORT_WDS_EXTENDED
        struct wlan_objmgr_psoc *psoc = NULL;

        psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
        if (psoc && wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_WDS_EXTENDED)) {
            wlan_mlme_deauth_request(ni->ni_vap, ni->ni_macaddr,
                                     IEEE80211_REASON_UNSPECIFIED);
        }
#endif
        IEEE80211_NODE_LEAVE(ni);
        ieee80211_free_node(ni, WLAN_WDS_ID);
    }

    /* clear NAWDS node table for the mac */
    nawds->repeater[i].caps = 0;
    OS_MEMZERO(nawds->repeater[i].mac, QDF_MAC_ADDR_SIZE);
}

static void
ieee80211_nawds_node_leave_all(wlan_if_t vaphandle)
{
    int i;
	struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;

    for(i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
        if (is_nawds_valid_mac(nawds->repeater[i].mac)) {
            ieee80211_nawds_node_leave(vap, nawds->repeater[i].mac);
        }
    }
}

/* This function is called to configure a repeater node as a HT node.
 * To avoid too much configuration, defaults are assumed as follows
 * MAX A-MPDU factor (valid range 0-3) default 2
 * MAX mpdudensity (valid range 0-7) default 7
 * SHORTGI not supprted as HT40 is not supported
 */
#define ATH_WDS_SINGLE_STREAM_REP_MAXAMPDUFACTOR 2
#define ATH_WDS_SINGLE_STREAM_REP_MPDUDENSITY    7
#define ATH_WDS_DOUBLE_STREAM_REP_MAXAMPDUFACTOR 3
#define ATH_WDS_DOUBLE_STREAM_REP_MPDUDENSITY    0
#define ATH_WDS_TRIPLE_STREAM_REP_MAXAMPDUFACTOR 3
#define ATH_WDS_TRIPLE_STREAM_REP_MPDUDENSITY    6
#define ATH_WDS_4_STREAM_REP_MAXAMPDUFACTOR 3
#define ATH_WDS_4_STREAM_REP_MPDUDENSITY    6
static int
ieee80211_nawds_config_ht(struct ieee80211_node *ni, u_int64_t caps)
{
    struct ieee80211_node * vapnode;
    struct ieee80211com   *ic = ni->ni_ic;
    struct ieee80211vap   *vap = ni->ni_vap;
    u_int32_t  mpdudensity = 0;
    u_int16_t  maxampdufactor = 0;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;

    /* Check the HT20/40 ratesets */
    if (caps & NAWDS_REPEATER_CAP_HT2040)
        ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
    else
        ni->ni_chwidth = IEEE80211_CWM_WIDTH20;

    /* Check if DS rates could be used */
    ni->ni_htcap &= (~IEEE80211_HTCAP_C_SM_MASK);
    ni->ni_htcap &= ~(IEEE80211_HTCAP_C_SHORTGI40);

    if (caps & (NAWDS_REPEATER_CAP_DS |
                NAWDS_REPEATER_CAP_TS |
                NAWDS_REPEATER_CAP_4S |
                NAWDS_REPEATER_CAP_5S |
                NAWDS_REPEATER_CAP_6S |
                NAWDS_REPEATER_CAP_7S |
                NAWDS_REPEATER_CAP_8S)){
        ni->ni_htcap |= IEEE80211_HTCAP_C_TXSTBC;
        ni->ni_htcap |= (IEEE80211_HTCAP_C_RXSTBC & ( 1 << IEEE80211_HTCAP_C_RXSTBC_S));
        ni->ni_htcap |= IEEE80211_HTCAP_C_ADVCODING;
        ni->ni_htcap |= IEEE80211_HTCAP_C_SMPOWERSAVE_DISABLED;
        ni->ni_htcap |= IEEE80211_HTCAP_C_SHORTGI20;

        ni->ni_htcap &= (((vdev_mlme->proto.generic.ldpc & IEEE80211_HTCAP_C_LDPC_RX) &&
              (ieee80211com_get_ldpccap(ic) & IEEE80211_HTCAP_C_LDPC_RX)) ?
              ni->ni_htcap : ~IEEE80211_HTCAP_C_ADVCODING);

        if (caps & NAWDS_REPEATER_CAP_HT2040) {
            ni->ni_htcap |= IEEE80211_HTCAP_C_SHORTGI40;
            ni->ni_htcap |= IEEE80211_HTCAP_C_CHWIDTH40;
        }

        if (IEEE80211_IS_CHAN_11NA(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AC(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_11AXA(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
            || IEEE80211_IS_CHAN_11BEA(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
           )
            ni->ni_htcap &= ~IEEE80211_HTCAP_C_DSSSCCK40;
        else
            ni->ni_htcap |= IEEE80211_HTCAP_C_DSSSCCK40;

        ni->ni_updaterates = IEEE80211_NODE_SM_EN;

        if (caps & NAWDS_5TO8_STREAMS) {
            /*
             * ni_streams will later be increased to the required value in the
             * VHT specific configuration routine.
             */
            ni->ni_streams = 4;

            mpdudensity = ATH_WDS_4_STREAM_REP_MPDUDENSITY;
            maxampdufactor = ATH_WDS_4_STREAM_REP_MAXAMPDUFACTOR;
        } else if (caps & NAWDS_REPEATER_CAP_4S) {
            ni->ni_streams = 4;
            mpdudensity = ATH_WDS_4_STREAM_REP_MPDUDENSITY;
            maxampdufactor = ATH_WDS_4_STREAM_REP_MAXAMPDUFACTOR;
        } else if (caps & NAWDS_REPEATER_CAP_TS) {
            ni->ni_streams = 3;
            mpdudensity = ATH_WDS_TRIPLE_STREAM_REP_MPDUDENSITY;
            maxampdufactor = ATH_WDS_TRIPLE_STREAM_REP_MAXAMPDUFACTOR;
        } else {
            ni->ni_streams = 2;
            mpdudensity = ATH_WDS_DOUBLE_STREAM_REP_MPDUDENSITY;
            maxampdufactor = ATH_WDS_DOUBLE_STREAM_REP_MAXAMPDUFACTOR;
        }

    } else {
        ni->ni_htcap |= IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC;
        ni->ni_updaterates = IEEE80211_NODE_SM_PWRSAV_STAT;
        ni->ni_streams = 1;
        mpdudensity = ATH_WDS_SINGLE_STREAM_REP_MPDUDENSITY;
        maxampdufactor =  ATH_WDS_SINGLE_STREAM_REP_MAXAMPDUFACTOR;
    }

    if (ni->ni_streams > vdev_mlme->proto.generic.nss) {
        ni->ni_streams = vdev_mlme->proto.generic.nss;
    }

#ifdef ATH_SUPPORT_TxBF
    if (caps &  NAWDS_REPEATER_CAP_TXBF){
        ni->ni_txbf.value = ic->ic_txbf.value; /* force node's txbf setting as local setting*/
        ieee80211_match_txbfcapability(ic, ni);
    }
#endif

    ni->ni_htcap &= (((vap->iv_tx_stbc) && (ni->ni_streams > 1)) ? \
            ni->ni_htcap : ~IEEE80211_HTCAP_C_TXSTBC);
    ni->ni_htcap &= (((vap->iv_rx_stbc) && (ni->ni_streams > 0)) ? \
            ni->ni_htcap : ~IEEE80211_HTCAP_C_RXSTBC);

    /* mark the node as HT-capable */
    ieee80211node_set_flag(ni, IEEE80211_NODE_HT);

    /*
     * The Maximum Rx A-MPDU defined by this field is equal to
     *   (2^^(13 + Maximum Rx A-MPDU Factor)) - 1
     * octets.  Maximum Rx A-MPDU Factor is an integer in the
     * range 0 to 3.
     */
    ni->ni_maxampdu = ((1u << (IEEE80211_HTCAP_MAXRXAMPDU_FACTOR + maxampdufactor)) - 1);
    ni->ni_mpdudensity = ieee80211_parse_mpdudensity(mpdudensity);

    /* copy the VAP HT Rates */
    vapnode = ni->ni_vap->iv_bss;
    memcpy(&(ni->ni_htrates), &(vapnode->ni_htrates), sizeof(struct ieee80211_rateset));

    if (ic->ic_set_ampduparams) {
        ic->ic_set_ampduparams(ni);
    }

    return 0;
}

static int
ieee80211_nawds_config_base(struct ieee80211_node *ni, u_int64_t caps, uint8_t ht_disable)
{

    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic ;
    struct ieee80211_node * vapni;

    /* sanity check */
    if ((vap == NULL) || ((ic = vap->iv_ic) == NULL)) {
        return -EINVAL;
    }

    vapni = vap->iv_bss;

    /* configure the capabilties */
    ni->ni_capinfo = vapni->ni_capinfo;
    ieee80211node_set_flag(ni, vapni->ni_flags);

    /* for bkward compat I assume bg unless configured otherwise */
    ieee80211node_clear_flag(ni, IEEE80211_NODE_HT);
    ieee80211node_clear_flag(ni, IEEE80211_NODE_WDS);
    ieee80211node_set_flag(ni, IEEE80211_NODE_QOS);
    ieee80211node_set_flag(ni, IEEE80211_NODE_ERP);

    /* 11AX TODO (Phase III) - Check for 11ax changes required here */

    if ((caps &  NAWDS_REPEATER_CAP_DS) ||
            (caps & NAWDS_REPEATER_CAP_TS) ||
            (caps & NAWDS_REPEATER_CAP_4S) ||
            (caps & NAWDS_REPEATER_CAP_5S) ||
            (caps & NAWDS_REPEATER_CAP_6S) ||
            (caps & NAWDS_REPEATER_CAP_7S) ||
            (caps & NAWDS_REPEATER_CAP_8S) ||
            ht_disable) {
        if (ht_disable || IEEE80211_IS_CHAN_11NG(vap->iv_bsschan)) {
            ni->ni_capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;
            ni->ni_capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;
            IEEE80211_DISABLE_PROTECTION(ic);
            ic->ic_protmode = IEEE80211_PROT_NONE;
            ieee80211_set_protmode(ic);
        }
    }

    /* copy the rates and xrates */
    memcpy(&(ni->ni_rates), &(vapni->ni_rates), sizeof(struct ieee80211_rateset));

    if ((ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE) == 0) {
        /* set preamble to no short preamble */
        ieee80211com_clear_flags(ic, IEEE80211_F_SHPREAMBLE);
        ieee80211com_set_flags(ic, IEEE80211_F_USEBARKER);
    }
    else {
        /* set preamble to short preamble */
        ieee80211com_set_flags(ic, IEEE80211_F_SHPREAMBLE);
        ieee80211com_clear_flags(ic, IEEE80211_F_USEBARKER);
    }

    ieee80211_set_shortslottime(ic,
            IEEE80211_IS_CHAN_A(ic->ic_curchan) ||
            IEEE80211_IS_CHAN_11NA(ic->ic_curchan) ||
            (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));

    wlan_pdev_beacon_update(ic);

    return 0;
}

#if (MESH_MODE_SUPPORT||ATH_SUPPORT_NAC)
static int
ieee80211_localpeer_config_base(struct ieee80211_node *ni, uint64_t caps, uint8_t ht_disable)
{

    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic ;
    struct ieee80211_node * vapni;

    /* sanity check */
    if ((vap == NULL) || ((ic = vap->iv_ic) == NULL)) {
        return -EINVAL;
    }

    vapni = vap->iv_bss;

    /* configure the capabilties */
    ni->ni_capinfo = vapni->ni_capinfo;
    ieee80211node_set_flag(ni, vapni->ni_flags);

    /* for bkward compat I assume bg unless configured otherwise */
    ieee80211node_clear_flag(ni, IEEE80211_NODE_HT);
    ieee80211node_clear_flag(ni, IEEE80211_NODE_WDS);
    ieee80211node_set_flag(ni, IEEE80211_NODE_QOS);
    ieee80211node_set_flag(ni, IEEE80211_NODE_ERP);

    if (caps & MESH_CAPS_SHORT_SLOT) {
        ni->ni_capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;
    }

    if (caps & MESH_CAPS_SHORT_PREAMBLE) {
        ni->ni_capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;
    }

    /* copy the rates and xrates */
    memcpy(&(ni->ni_rates), &(vapni->ni_rates), sizeof(struct ieee80211_rateset));

    wlan_pdev_beacon_update(ic);

    return 0;
}

#endif
/*
 * VHT Configuration :
 * 1. vhtcap
 * 2. rx_max_rate;
 *    rx_vhtrates;
 *    tx_max_rate;
 *    ni_tx_vhtrates;
 *    IEEE80211_NODE_VHT
 */
static int
ieee80211_nawds_config_vht(struct ieee80211_node *ni, u_int64_t caps, bool target_update)
{
    struct ieee80211com   *ic = ni->ni_ic;
    struct ieee80211vap   *vap = ni->ni_vap;
    u_int32_t vhtcap_info, ampdu_len = 0;
    u_int8_t  chwidth = 0;
    bool is_160_or_80p80_supported = false;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;

    if (ic->ic_modecaps &
                ((1 << IEEE80211_MODE_11AC_VHT160) |
                 (1 << IEEE80211_MODE_11AC_VHT80_80))) {
        is_160_or_80p80_supported = true;
    }

    /* Fill in the VHT capabilities info */
    vhtcap_info    = ic->ic_vhtcap;
    vhtcap_info   &= ((vap->iv_sgi) ? ic->ic_vhtcap : ~IEEE80211_VHTCAP_SHORTGI_80);
    if(is_160_or_80p80_supported && ((caps & NAWDS_REPEATER_CAP_11ACVHT80_80)
                                     || (caps & NAWDS_REPEATER_CAP_11ACVHT160)))
    vhtcap_info   &= ((vap->iv_sgi) ? ic->ic_vhtcap : ~IEEE80211_VHTCAP_SHORTGI_160);/* Short GI for 160 and 80+80 MHz */
    vhtcap_info   &= ((vdev_mlme->proto.generic.ldpc & IEEE80211_HTCAP_C_LDPC_RX) ?  ic->ic_vhtcap  : ~IEEE80211_VHTCAP_RX_LDPC);
    vhtcap_info   &= ((vap->iv_tx_stbc) ?  ic->ic_vhtcap : ~IEEE80211_VHTCAP_TX_STBC);
    vhtcap_info   &= ((vap->iv_rx_stbc) ?  ic->ic_vhtcap : ~IEEE80211_VHTCAP_RX_STBC);

    /* Disable beam-forming capability in NAWDS mode */
    vhtcap_info &= ~(IEEE80211_VHTCAP_SU_BFORMER | IEEE80211_VHTCAP_SU_BFORMEE |
            IEEE80211_VHTCAP_MU_BFORMER | IEEE80211_VHTCAP_MU_BFORMEE);

    ni->ni_vhtcap  = htole32(vhtcap_info);

    /* Set Chwidth depending on defcaps */
    if(is_160_or_80p80_supported && ((caps & NAWDS_REPEATER_CAP_11ACVHT80_80)
           || (caps & NAWDS_REPEATER_CAP_11ACVHT160)))
        chwidth = IEEE80211_CWM_WIDTH160;
    else if(caps & NAWDS_REPEATER_CAP_11ACVHT80)
        chwidth = IEEE80211_CWM_WIDTH80;
    else if(caps & NAWDS_REPEATER_CAP_11ACVHT40)
        chwidth = IEEE80211_CWM_WIDTH40;
    else if(caps & NAWDS_REPEATER_CAP_11ACVHT20)
        chwidth = IEEE80211_CWM_WIDTH20;

    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        switch(chwidth) {
            case IEEE80211_CWM_WIDTH20:
                ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
            break;

            case IEEE80211_CWM_WIDTH40:
                    ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
            break;

            case IEEE80211_CWM_WIDTH80:
                    ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
            break;

            case IEEE80211_CWM_WIDTH160:
                    ni->ni_chwidth = IEEE80211_CWM_WIDTH160;
            break;

            default:
                /* Do nothing */
            break;
        }
    }

    /* Fill in the VHT MCS info */
    ieee80211_set_vht_rates(ic,vap);

    /*
     * The Maximum Rx A-MPDU defined by this field is equal to
     *   (2^^(13 + Maximum Rx A-MPDU Factor)) - 1
     * octets.  Maximum Rx A-MPDU Factor is an integer in the
     * range 0 to 7.
     */
    ampdu_len = (le32toh(ni->ni_vhtcap) & IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP) >> IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S;
    ni->ni_maxampdu = (1u << (IEEE80211_VHTCAP_MAX_AMPDU_LEN_FACTOR + ampdu_len)) -1;
    ni->ni_tx_vhtrates = vap->iv_vhtcap_max_mcs.tx_mcs_set.mcs_map;
    ni->ni_tx_max_rate = vap->iv_vhtcap_max_mcs.tx_mcs_set.data_rate;
    ni->ni_rx_vhtrates = vap->iv_vhtcap_max_mcs.rx_mcs_set.mcs_map;
    ni->ni_rx_max_rate = vap->iv_vhtcap_max_mcs.rx_mcs_set.data_rate;

    ieee80211node_set_flag(ni, IEEE80211_NODE_VHT);

    /* Streams decision based on TS/DS or higher flags */

    if (caps & NAWDS_REPEATER_CAP_8S) {
        ni->ni_streams = 8;
    }
    else if (caps & NAWDS_REPEATER_CAP_7S) {
        ni->ni_streams = 7;
    }
    else if (caps & NAWDS_REPEATER_CAP_6S) {
        ni->ni_streams = 6;
    }
    else if (caps & NAWDS_REPEATER_CAP_5S) {
        ni->ni_streams = 5;
    }
    else if (caps & NAWDS_REPEATER_CAP_4S) {
        ni->ni_streams = 4;
    }
    else if (caps & NAWDS_REPEATER_CAP_TS) {
        ni->ni_streams = 3;
    }
    else if (caps & NAWDS_REPEATER_CAP_DS) {
        ni->ni_streams = 2;
    } else {
        ni->ni_streams = 1;
    }
    if (ni->ni_streams > vdev_mlme->proto.generic.nss) {
        ni->ni_streams = vdev_mlme->proto.generic.nss;
    }
    /*
     * Update NSS and CHWIDTH params on target
     */
    if (target_update) {
        if (ic->ic_nss_change != NULL)
            ic->ic_nss_change(ni);
    }

    return 0;
}

static void ieee80211_copy_hecap_rates(struct ieee80211com *ic,
                                       struct ieee80211vap  *vap,
                                       struct ieee80211_node *ni)
{
    struct ieee80211_he_handle *ic_he  = &ic->ic_he;
    struct ieee80211_he_handle *ni_he  = &ni->ni_he;

    if(!(ieee80211_is_phymode_allowed(ni->ni_phymode))) {
        ieee80211_note(vap, IEEE80211_MSG_WDS,
                   "%s WARNING!!! Unsupported ni_phymode=%x\n",
                    __func__, ni->ni_phymode);
        return;
    }

    switch(ni->ni_phymode) {
        case IEEE80211_MODE_11AXA_HE80_80:
            ni_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] =
                ic_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80];
            ni_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80] =
                ic_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80_80];
            /* fall through */

        case IEEE80211_MODE_11AXA_HE160:
            ni_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160] =
              ic_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160];
            ni_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160] =
              ic_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_160];
            /* fall through */

        default:
            ni_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80] =
              ic_he->hecap_rxmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80];
            ni_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80] =
              ic_he->hecap_txmcsnssmap[HECAP_TXRX_MCS_NSS_IDX_80];
            break;
    }
}

#ifdef WLAN_FEATURE_11BE
static void
ieee80211_nawds_config_eht(struct ieee80211_node *ni, uint64_t caps, uint32_t puncture_bitmap)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_eht_handle *ni_eht, *ic_eht;
    bool is_160_or_80p80_supported = false;
    bool is_320_supported = false;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;

    if (ic->ic_modecaps &
                 (1ULL << IEEE80211_MODE_11BEA_EHT160)) {
        is_160_or_80p80_supported = true;
    }

    if (ic->ic_modecaps &
                 (1ULL << IEEE80211_MODE_11BEA_EHT320)) {
        is_320_supported = true;
    }

     ni_eht = &ni->ni_eht;
     ic_eht = &ic->ic_eht;

     qdf_mem_copy(ni_eht, ic_eht, sizeof(*ni_eht));

    /* Set Chwidth depending on defcaps */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        if (is_320_supported && (caps & NAWDS_REPEATER_CAP_11BEAEHT320)) {
            ni->ni_chwidth = IEEE80211_CWM_WIDTH320;
        } else if(is_160_or_80p80_supported && (caps &
                        NAWDS_REPEATER_CAP_11BEAEHT160))
            ni->ni_chwidth = IEEE80211_CWM_WIDTH160;
        else if(caps & NAWDS_REPEATER_CAP_11BEAEHT80)
            ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
        else if(caps & (NAWDS_REPEATER_CAP_11BEAEHT40 |
                        NAWDS_REPEATER_CAP_11BEGEHT40))
            ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
        else if(caps & (NAWDS_REPEATER_CAP_11BEAEHT20 |
                        NAWDS_REPEATER_CAP_11BEAEHT20))
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
    }

    ieee80211node_set_extflag(ni, IEEE80211_NODE_EHT);

    /* Set mpdu density to zero for HE capable APs */
    ni->ni_mpdudensity = ieee80211_parse_mpdudensity(IEEE80211_HTCAP_MPDUDENSITY_NA);

    /* Streams decision based on TS/DS or higher flags */
    if (caps & NAWDS_REPEATER_CAP_8S) {
        ni->ni_streams = 8;
    }
    else if (caps & NAWDS_REPEATER_CAP_7S) {
        ni->ni_streams = 7;
    }
    else if (caps & NAWDS_REPEATER_CAP_6S) {
        ni->ni_streams = 6;
    }
    else if (caps & NAWDS_REPEATER_CAP_5S) {
        ni->ni_streams = 5;
    }
    else if (caps & NAWDS_REPEATER_CAP_4S) {
        ni->ni_streams = 4;
    }
    else if (caps & NAWDS_REPEATER_CAP_TS) {
        ni->ni_streams = 3;
    }
    else if (caps & NAWDS_REPEATER_CAP_DS) {
        ni->ni_streams = 2;
    } else {
        ni->ni_streams = 1;
    }
    if (ni->ni_streams > vdev_mlme->proto.generic.nss) {
        ni->ni_streams = vdev_mlme->proto.generic.nss;
    }

    /* Initialize the node channel params */
    ieee80211_node_set_chan_param(ni,
                                  ieee80211_chan2mode(ic->ic_curchan),
                                  ic->ic_curchan->ic_freq,
                                  ic->ic_curchan->ic_vhtop_freq_seg1,
                                  ic->ic_curchan->ic_vhtop_freq_seg2,
                                  (uint16_t)puncture_bitmap);
    return;

}
#endif /* WLAN_FEATURE_11BE */

static void
ieee80211_nawds_config_he(struct ieee80211_node *ni, u_int64_t caps)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_he_handle *ni_he, *ic_he;
    u_int32_t *ic_hecap_phyinfo, val, *ni_hecap_phyinfo;
    u_int8_t rx_streams, tx_streams;
    bool is_160_or_80p80_supported = false;
    struct vdev_mlme_obj *vdev_mlme = vap->vdev_mlme;

    if (ic->ic_modecaps &
                ((1 << IEEE80211_MODE_11AXA_HE160) |
                 (1ULL << IEEE80211_MODE_11AXA_HE80_80))) {
        is_160_or_80p80_supported = true;
    }

     ni_he = &ni->ni_he;
     ic_he = &ic->ic_he;

     qdf_mem_copy(ni_he, ic_he, sizeof(*ni_he));

     ni_hecap_phyinfo = &ni_he->hecap_phyinfo[HECAP_PHYBYTE_IDX0];
     ic_hecap_phyinfo = &ic_he->hecap_phyinfo[HECAP_PHYBYTE_IDX0];

     if (!ic_hecap_phyinfo) {
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "%s:ic_hecap_phyinfo is"
                           " NULL \n", __func__);
         return;
     }

     val = HECAP_PHY_CBW_GET_FROM_IC(ic_hecap_phyinfo);
     hecap_override_channelwidth(vap, &val, ni);

     val = HECAP_PHY_LDPC_GET_FROM_IC(ic_hecap_phyinfo);
     if (!(val && vdev_mlme->proto.generic.ldpc)) {
       val = 0;
     }
     HECAP_PHY_LDPC_SET_TO_IC(ni_hecap_phyinfo, val);

     IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                       "%s:ldpc:%d \n", __func__, val);

     rx_streams = ieee80211_get_rxstreams(ic, vap);
     tx_streams = ieee80211_get_txstreams(ic, vap);

     val = HECAP_PHY_TXSTBC_GET_FROM_IC(ic_hecap_phyinfo);
     if (!(val && vap->iv_tx_stbc && (tx_streams > 1))) {
       val = 0;
     }
     HECAP_PHY_TXSTBC_SET_TO_IC(ni_hecap_phyinfo, val);

     IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                       "%s:tx_stbc:%d \n", __func__, val);

     val = HECAP_PHY_RXSTBC_GET_FROM_IC(ic_hecap_phyinfo);
     if (!(val && vap->iv_rx_stbc && (rx_streams > 1))) {
       val = 0;
     }
     HECAP_PHY_RXSTBC_SET_TO_IC(ni_hecap_phyinfo, val);

     IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                       "%s:rx_stbc:%d \n", __func__, val);

     /* Disable beam-forming capability in NAWDS mode */
     HECAP_PHY_SUBFMR_SET_TO_IC(ni_hecap_phyinfo, 0);
     HECAP_PHY_SUBFME_SET_TO_IC(ni_hecap_phyinfo, 0);
     HECAP_PHY_MUBFMR_SET_TO_IC(ni_hecap_phyinfo, 0);

    /* Set Chwidth depending on defcaps */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        if(is_160_or_80p80_supported && (caps &
                       (NAWDS_REPEATER_CAP_11AXAHE80_80 |
                        NAWDS_REPEATER_CAP_11AXAHE160)))
            ni->ni_chwidth = IEEE80211_CWM_WIDTH160;
        else if(caps & NAWDS_REPEATER_CAP_11AXAHE80)
            ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
        else if(caps & (NAWDS_REPEATER_CAP_11AXAHE40 |
                        NAWDS_REPEATER_CAP_11AXGHE40))
            ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
        else if(caps & (NAWDS_REPEATER_CAP_11AXAHE20 |
                        NAWDS_REPEATER_CAP_11AXGHE20))
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
    }

    ieee80211node_set_extflag(ni, IEEE80211_NODE_HE);

    /* Set mpdu density to zero for HE capable APs */
    ni->ni_mpdudensity = ieee80211_parse_mpdudensity(IEEE80211_HTCAP_MPDUDENSITY_NA);

    /* Streams decision based on TS/DS or higher flags */
    if (caps & NAWDS_REPEATER_CAP_8S) {
        ni->ni_streams = 8;
    }
    else if (caps & NAWDS_REPEATER_CAP_7S) {
        ni->ni_streams = 7;
    }
    else if (caps & NAWDS_REPEATER_CAP_6S) {
        ni->ni_streams = 6;
    }
    else if (caps & NAWDS_REPEATER_CAP_5S) {
        ni->ni_streams = 5;
    }
    else if (caps & NAWDS_REPEATER_CAP_4S) {
        ni->ni_streams = 4;
    }
    else if (caps & NAWDS_REPEATER_CAP_TS) {
        ni->ni_streams = 3;
    }
    else if (caps & NAWDS_REPEATER_CAP_DS) {
        ni->ni_streams = 2;
    } else {
        ni->ni_streams = 1;
    }
    if (ni->ni_streams > vdev_mlme->proto.generic.nss) {
        ni->ni_streams = vdev_mlme->proto.generic.nss;
    }

    return;
}

#if (MESH_MODE_SUPPORT||ATH_SUPPORT_NAC)
int wlan_mesh_config_ni_params(struct ieee80211vap *vap,
                               struct ieee80211_node *ni,
                               u_int64_t caps,
                               u_int32_t puncture_bitmap,
                               bool is_mesh_mld)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int64_t ht_flags, vht_flags, he_flags;
    uint8_t ht_disable=0;
    bool is_160_or_80p80_supported = false;
#ifdef WLAN_FEATURE_11BE
    u_int64_t eht_flags;
    bool is_320_supported = false;
#endif /* WLAN_FEATURE_11BE */
    enum ieee80211_cwm_width max_chwidth = 0;

    if (ic->ic_modecaps &
                ((1 << IEEE80211_MODE_11AC_VHT160) |
                 (1 << IEEE80211_MODE_11AC_VHT80_80) |
                 (1 << IEEE80211_MODE_11AXA_HE160) |
                 (1ULL << IEEE80211_MODE_11AXA_HE80_80)
#ifdef WLAN_FEATURE_11BE
                 | (1ULL << IEEE80211_MODE_11BEA_EHT160)
#endif /* WLAN_FEATURE_11BE */
                 ))
    {
        is_160_or_80p80_supported = true;
    }

#ifdef WLAN_FEATURE_11BE
    if (ic->ic_modecaps & (1ULL << IEEE80211_MODE_11BEA_EHT320)) {
        is_320_supported = true;
    }
#endif /* WLAN_FEATURE_11BE */

    /* another check for NG setup  was earlier 0 */
    /* we do not want very low SNR as we may start at very poor rates, so setting SNR to a normal value*/
    ni->ni_snr = ATH_DEFAULT_NORMAL_SNR;

    /* base configuration */
    ieee80211_localpeer_config_base(ni, caps, ht_disable);

    /* HT capability */
    ht_flags  = (NAWDS_REPEATER_CAP_HT20 |
                 NAWDS_REPEATER_CAP_HT2040);
    vht_flags = (NAWDS_REPEATER_CAP_11ACVHT20 |
                 NAWDS_REPEATER_CAP_11ACVHT40 |
                 NAWDS_REPEATER_CAP_11ACVHT80);
    he_flags  = (NAWDS_REPEATER_CAP_11AXAHE20 |
                 NAWDS_REPEATER_CAP_11AXGHE20 |
                 NAWDS_REPEATER_CAP_11AXAHE40 |
                 NAWDS_REPEATER_CAP_11AXGHE40 |
                 NAWDS_REPEATER_CAP_11AXAHE80);

#ifdef WLAN_FEATURE_11BE
    eht_flags = (NAWDS_REPEATER_CAP_11BEAEHT20  |
                 NAWDS_REPEATER_CAP_11BEGEHT20  |
                 NAWDS_REPEATER_CAP_11BEAEHT40  |
                 NAWDS_REPEATER_CAP_11BEGEHT40  |
                 NAWDS_REPEATER_CAP_11BEAEHT80);
#endif /* WLAN_FEATURE_11BE */

    if(is_160_or_80p80_supported) {
        vht_flags |= (NAWDS_REPEATER_CAP_11ACVHT80_80 | NAWDS_REPEATER_CAP_11ACVHT160);
        he_flags  |= (NAWDS_REPEATER_CAP_11AXAHE80_80 |  NAWDS_REPEATER_CAP_11AXAHE160);
#ifdef WLAN_FEATURE_11BE
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT160;
#endif /* WLAN_FEATURE_11BE */
    }

#ifdef WLAN_FEATURE_11BE
    if (is_320_supported) {
        he_flags  |= NAWDS_REPEATER_CAP_11AXAHE160;
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT320;
    }
#endif /* WLAN_FEATURE_11BE */

    if (caps & (ht_flags | vht_flags | he_flags
#ifdef WLAN_FEATURE_11BE
                | eht_flags
#endif /* WLAN_FEATURE_11BE */
                )) {

        /* Add VHT and HT caps only for 2.4GHz or 5GHz */
        if (IEEE80211_IS_CHAN_2GHZ(vap->iv_bsschan) ||
            IEEE80211_IS_CHAN_5GHZ(vap->iv_bsschan)) {

            /* Add HT capability for all possible caps */
            ieee80211_nawds_config_ht(ni, caps);

            /*
             * Add VHT capability.
             * NOTE: Include 2.4GHz 256QAM support.
             */
            if ((caps & vht_flags) &&
                (ieee80211vap_vhtallowed(vap)) &&
                (IEEE80211_IS_CHAN_VHT(vap->iv_bsschan) ||
                 IEEE80211_IS_CHAN_11NG(vap->iv_bsschan) ||
                 IEEE80211_IS_CHAN_HE(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
                 || IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
                )) {
                ieee80211_nawds_config_vht(ni, caps, FALSE);
            }
        }

        if ((caps & he_flags) &&
            (ieee80211vap_heallowed(vap)) &&
            (IEEE80211_IS_CHAN_HE(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
             || IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
            )) {
            ieee80211_nawds_config_he(ni, caps);
        }

#ifdef WLAN_FEATURE_11BE
        if ((caps & eht_flags) &&
            (ieee80211vap_ehtallowed(vap)) &&
            IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)) {
            ieee80211_nawds_config_vht(ni, caps, FALSE);
            ieee80211_nawds_config_he(ni, caps);
            ieee80211_nawds_config_eht(ni, caps, puncture_bitmap);
        }
#endif /* WLAN_FEATURE_11BE */

    } else {
        /* non-ht/vht i.e legacy case */
        ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
        qdf_mem_set(&ni->ni_htrates, sizeof(struct ieee80211_rateset), 0);
    }

    if ((IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) &&
         ((caps & ht_flags)  ||
          (caps & vht_flags) ||
          (!(caps & he_flags)
#ifdef WLAN_FEATURE_11BE
           && !(caps & eht_flags)
#endif /* WLAN_FEATURE_11BE */
          ))) {
            caps |= NAWDS_REPEATER_CAP_11AXAHE20;
            ieee80211_note(vap, IEEE80211_MSG_WDS,
                           "No capabilities given for 6GHz! Setting the caps to 11AX 20MHz BW\n");
    }

    /* Update the PHY mode of the node */
    if (IEEE80211_IS_CHAN_5GHZ_6GHZ(ic->ic_curchan)) {
        ni->ni_phymode = IEEE80211_MODE_11A;
    } else if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11G)) {
        ni->ni_phymode = IEEE80211_MODE_11G;
    } else {
        ni->ni_phymode = IEEE80211_MODE_11B;
    }

    /* Check for max channel width supported by vap */
    max_chwidth = vap->iv_ic->ic_cwm_get_width(vap->iv_ic);
    if (ni->ni_chwidth > max_chwidth)
        ni->ni_chwidth = max_chwidth;

    ieee80211_update_phymode(ic, ni);

    /* Rates are copied here for HE because it requires
     * ni's phymode which is set above
     */
    if ((caps & he_flags) &&
        (ieee80211vap_heallowed(vap)) &&
        IEEE80211_IS_CHAN_HE(vap->iv_bsschan)) {
        ieee80211_copy_hecap_rates(ic, vap, ni);
    }

    if (is_mesh_mld)
        ieee80211_node_join(ni, 0);
    else
        ieee80211_node_join(ni, 1);
#ifdef QCA_SUPPORT_CP_STATS
    vdev_cp_stats_authorize_attempt_inc(vap->vdev_obj, 1);
#endif

    wlan_node_set_peer_state(ni, WLAN_WAITKEY_STATE);
    /* Mark this node entry as local mesh peer */
    ieee80211node_set_extflag(ni, IEEE80211_LOCAL_MESH_PEER);
    /* set mesh intersect drop counter to avoid race condition */
    ieee80211_update_mesh_intersect_drop_count(ni);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_NAC | IEEE80211_MSG_MESH,"%s: Addr bytes[0][5]: \n",
                           __func__);
    return 0;
}

static struct
ieee80211_node *wlan_localpeer_config(wlan_if_t vaphandle, char *macaddr, uint64_t caps,
                                      uint32_t puncture_bitmap, int *prev_added)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int ret;

    /* if previously configured node found, start the node over */
    ni = ieee80211_find_node(ic, macaddr, WLAN_WDS_ID);
    if (!ni)
        ni = find_logically_deleted_node_on_soc(
                wlan_vdev_get_psoc(vap->vdev_obj), macaddr, NULL, WLAN_WDS_ID);

    if (ni) {
        *prev_added = 1;
        ieee80211_free_node(ni, WLAN_WDS_ID);
        return NULL;
    }

#ifdef AST_HKV1_WORKAROUND
    if (mlme_find_and_delete_wds_before_auth(vap, macaddr, NULL) == 0) {
        qdf_err("WDS entry exists");
        *prev_added = 1;
        return NULL;
    }
#endif

    ni = ieee80211_dup_bss(vap, macaddr);
    if (ni == NULL)
        return NULL;

    if (wlan_mesh_config_ni_params(vap, ni, caps, puncture_bitmap, false) != 0)
        return NULL;

    if (ic->ic_newassoc != NULL) {
        ret = ic->ic_newassoc(ni, 1);
        if (ret) {
            qdf_err("Node %s Param validation failed, disconnecting STA",
                     ether_sprintf(ni->ni_macaddr));
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            if (IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                ni->ni_macaddr, ni->ni_associd,
                                IEEE80211_STATUS_UNSPECIFIED,
                                &ni->peer_obj->mldaddr[0]);
#else
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                ni->ni_macaddr, ni->ni_associd,
                                IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
            }
            ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
            return NULL;
        }
    }

    if (!(ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY)) {
        ieee80211_node_authorize(ni);
    } else {
        qdf_nofl_info (" privacy \n");
    }

    /* free the extra refcount got from dup_bss() */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_NAC | IEEE80211_MSG_MESH,"%s: Addr bytes[0][5]:%2x:%2x \n",
                           __func__,macaddr[0],macaddr[5]);
    return ni;
}

int wlan_authorise_local_peer(wlan_if_t vaphandle, char *macaddr)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    /* if previously configured node found, start the node over */
    ni = ieee80211_find_node(ic, macaddr, WLAN_WDS_ID);
    if (ni) {
        if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap))
            ni->ni_capinfo |= IEEE80211_CAPINFO_PRIVACY;

        if (ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY) {
            ieee80211_node_authorize(ni);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_NAC | IEEE80211_MSG_MESH,"%s: Authorize Addr bytes[0][5]:%2x:%2x \n",
                            __func__,macaddr[0],macaddr[5]);
        }

        ieee80211_free_node(ni, WLAN_WDS_ID);
        return 0;
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_NAC | IEEE80211_MSG_NAC,"%s: Failed Addr bytes[0][5]:%2x:%2x \n",
                            __func__,macaddr[0],macaddr[5]);
        return EINVAL;
    }
    return 0;
}

int
wlan_del_localpeer(wlan_if_t vaphandle, char *macaddr)
{
    struct ieee80211_node* ni;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    /* reclaim the node */
    ni = ieee80211_find_node(ic, macaddr, WLAN_WDS_ID);

    if (ni) {
        if (ni->ni_ext_flags&IEEE80211_LOCAL_MESH_PEER) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_NAC | IEEE80211_MSG_MESH,"%s: delete mesh peer mac bytes[0][5]:%2x:%2x \n",
                               __func__,macaddr[0],macaddr[5]);
            IEEE80211_NODE_LEAVE(ni);
        }
        ieee80211_free_node(ni, WLAN_WDS_ID);
        return 0;
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_NAC,"%s: Failed Addr bytes[0][5]:%2x:%2x \n",
                            __func__,macaddr[0],macaddr[5]);
        return -1;
    }
    return 0;
}
#endif

void ieee80211_nawds_update_chan_and_phymode(struct ieee80211_node *ni,
                                             u_int32_t puncture_bitmap)
{
    struct ieee80211vap *vap = ni->ni_vap;
    enum ieee80211_cwm_width max_chwidth = 0;

    ieee80211_node_set_chan_param(ni, ieee80211_chan2mode(vap->iv_bsschan),
                                  vap->iv_bsschan->ic_freq,
                                  vap->iv_bsschan->ic_vhtop_freq_seg1,
                                  vap->iv_bsschan->ic_vhtop_freq_seg2
#ifdef WLAN_FEATURE_11BE
                                  , (uint16_t)puncture_bitmap
#endif
                                 );
    ni->ni_omn_chwidth = IEEE80211_CWM_WIDTHINVALID;
    max_chwidth = vap->iv_ic->ic_cwm_get_width(vap->iv_ic);

    if (ni->ni_chwidth > max_chwidth)
        ni->ni_chwidth = max_chwidth;
    /* Update phy mode */
    ieee80211_update_phymode(vap->iv_ic, ni);
}

void
wlan_nawds_node_authorize(struct ieee80211vap *vap, uint8_t *peer_mac_addr)
{
    struct ieee80211com *ic = NULL;
    struct ieee80211_node *ni = NULL;
    struct ieee80211_node_table  *nt;
    struct ieee80211_node *ni_wds = NULL;

    ic = vap->iv_ic;
    nt = &ic->ic_sta;

    ni = ieee80211_find_node(ic, peer_mac_addr, WLAN_WDS_ID);
    if (!ni) {
        qdf_err("node not found for mapped peer");
        return;
    }

    ni->ni_ext_flags &= ~IEEE80211_NODE_NAWDS_AUTH_SET_FAIL;

    ieee80211_node_authorize(ni);
    ic->ic_nss_change(ni);
    ic->ic_chwidth_change(ni);
    ni_wds = ieee80211_find_wds_node(nt, peer_mac_addr, WLAN_WDS_ID);
    if (ni_wds == NULL) {
        ieee80211_add_wds_addr(vap, nt, ni, peer_mac_addr,
                               IEEE80211_NODE_F_WDS_REMOTE);
    } else {
        ieee80211_free_node(ni_wds, WLAN_WDS_ID);
    }

    ieee80211_free_node(ni, WLAN_WDS_ID);
}

bool wlan_nawds_check_if_auth_fail(wlan_if_t vaphandle, char *macaddr)
{
    struct ieee80211_node *ni = NULL;

    ni = ieee80211_find_node(vaphandle->iv_ic, macaddr, WLAN_WDS_ID);
    if (!ni)
        return true;

    if (ni->ni_ext_flags & IEEE80211_NODE_NAWDS_AUTH_SET_FAIL) {
        ieee80211_free_node(ni, WLAN_WDS_ID);
        return true;
    }

    ieee80211_free_node(ni, WLAN_WDS_ID);
    return false;
}

static void wlan_nawds_config_repeater(wlan_if_t vaphandle,
                                           char *macaddr,
                                           u_int64_t caps,
                                           u_int32_t puncture_bitmap)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node       *ni_wds = NULL;
    struct ieee80211_node_table  *nt = &ic->ic_sta;
    u_int32_t ht_flags, vht_flags, he_flags;
    uint8_t ht_disable=0;
    bool is_160_or_80p80_supported = false;
    bool is_6g_operation = false;
    bool is_wds_ni = false;
    int ret = 0;
#ifdef WLAN_FEATURE_11BE
    uint64_t eht_flags;
    bool is_320_supported = false;
#endif /* WLAN_FEATURE_11BE */

    if (ic->ic_modecaps &
                ((1 << IEEE80211_MODE_11AC_VHT160) |
                 (1 << IEEE80211_MODE_11AC_VHT80_80) |
                 (1 << IEEE80211_MODE_11AXA_HE160) |
                 (1ULL << IEEE80211_MODE_11AXA_HE80_80)
#ifdef WLAN_FEATURE_11BE
                 | (1ULL << IEEE80211_MODE_11BEA_EHT160)
#endif
                ))
    {
        is_160_or_80p80_supported = true;
    }

#ifdef WLAN_FEATURE_11BE
    if (ic->ic_modecaps & (1ULL << IEEE80211_MODE_11BEA_EHT320)) {
        is_320_supported = true;
    }
#endif /* WLAN_FEATURE_11BE */

    /* if previously configured node found, start the node over */
    ni = ieee80211_find_node(ic, macaddr, WLAN_WDS_ID);
    if (ni) {
        is_wds_ni = true;
        goto reconfigure;
    } else {
        ni = find_logically_deleted_node_pdev_psoc(ic, macaddr, WLAN_WDS_ID);

        if (ni) {
            /* If FW delete resp pending then return else continue with new node creation */
            if (qdf_atomic_read(&ni->ni_fw_peer_delete_rsp_pending)) {
                qdf_debug("delete response pending for peer %s",macaddr);
                ieee80211_free_node(ni, WLAN_WDS_ID);
                return;
            }
            ieee80211_free_node(ni, WLAN_WDS_ID);
        }
    }

    ni = ieee80211_dup_bss(vap, macaddr);
    if (ni == NULL)
        return;

    /* free the extra refcount got from dup_bss() */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);
    is_6g_operation = IEEE80211_IS_CHAN_6GHZ(vap->iv_bsschan);
reconfigure:
    /* another check for NG setup  was earlier 0 */
    /* we do not want very low SNR as we may start at very poor rates, so setting SNR to a normal value*/
    ni->ni_snr = ATH_DEFAULT_NORMAL_SNR;

    /*
     * Set the default nss to 1,
     * the config ht/vht funcitons will take care of the value configured in caps.
     */
    ni->ni_streams = 1;

    if((IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) || (ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY)) &&
            wlan_crypto_vdev_has_ucastcipher(vap->vdev_obj, (1 << WLAN_CRYPTO_CIPHER_WEP))) {
       ht_disable = 1;
    }
    /* base configuration */
    ieee80211_nawds_config_base(ni, caps, ht_disable);

    /* Enable HT40 if 40 MHz or higher width is requested */
    if (caps & (NAWDS_REPEATER_CAP_HT2040 |
                NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_11ACVHT80 |
                NAWDS_REPEATER_CAP_11ACVHT80_80 | NAWDS_REPEATER_CAP_11ACVHT160 |
                NAWDS_REPEATER_CAP_11AXAHE40 | NAWDS_REPEATER_CAP_11AXGHE40 |
                NAWDS_REPEATER_CAP_11AXAHE80 | NAWDS_REPEATER_CAP_11AXAHE80_80 |
                NAWDS_REPEATER_CAP_11AXAHE160
#ifdef WLAN_FEATURE_11BE
                | NAWDS_REPEATER_CAP_11BEAEHT40 | NAWDS_REPEATER_CAP_11BEGEHT40
                | NAWDS_REPEATER_CAP_11BEAEHT80 | NAWDS_REPEATER_CAP_11BEAEHT160
                | NAWDS_REPEATER_CAP_11BEAEHT320
#endif
               )) {
        caps |= NAWDS_REPEATER_CAP_HT2040;
    }

    /* HT capability */
    ht_flags  = (NAWDS_REPEATER_CAP_HT20 | NAWDS_REPEATER_CAP_HT2040);
    vht_flags = (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_11ACVHT80);
    he_flags = (NAWDS_REPEATER_CAP_11AXAHE20 | NAWDS_REPEATER_CAP_11AXGHE20 | NAWDS_REPEATER_CAP_11AXAHE40 |
                NAWDS_REPEATER_CAP_11AXGHE40 | NAWDS_REPEATER_CAP_11AXAHE80);
#ifdef WLAN_FEATURE_11BE
    eht_flags = (NAWDS_REPEATER_CAP_11BEAEHT20  |
                 NAWDS_REPEATER_CAP_11BEGEHT20  |
                 NAWDS_REPEATER_CAP_11BEAEHT40  |
                 NAWDS_REPEATER_CAP_11BEGEHT40  |
                 NAWDS_REPEATER_CAP_11BEAEHT80);
#endif /* WLAN_FEATURE_11BE */

    if(is_160_or_80p80_supported) {
        vht_flags |= (NAWDS_REPEATER_CAP_11ACVHT80_80 | NAWDS_REPEATER_CAP_11ACVHT160);
        he_flags |= (NAWDS_REPEATER_CAP_11AXAHE80_80 |  NAWDS_REPEATER_CAP_11AXAHE160);
#ifdef WLAN_FEATURE_11BE
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT160;
#endif /* WLAN_FEATURE_11BE */
    }

#ifdef WLAN_FEATURE_11BE
    if (is_320_supported) {
        he_flags  |= NAWDS_REPEATER_CAP_11AXAHE160;
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT320;
    }
#endif /* WLAN_FEATURE_11BE */

    if (caps & (ht_flags | vht_flags | he_flags
#ifdef WLAN_FEATURE_11BE
                | eht_flags
#endif /* WLAN_FEATURE_11BE */
               ) || ht_disable) {
        if((IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) || (ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY)) &&
            wlan_crypto_vdev_has_ucastcipher(vap->vdev_obj, (1 << WLAN_CRYPTO_CIPHER_WEP))) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS, "%s: WEP Mode, so not configuring HT Cap\n", __FUNCTION__);
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
            ni->ni_streams = 1;
            OS_MEMSET(&ni->ni_htrates, 0 , sizeof(struct ieee80211_rateset));

            } else {
                if (!is_6g_operation) {
                    /* HT Capability */
                    if (ieee80211vap_htallowed(ni->ni_vap))
                        ieee80211_nawds_config_ht(ni, caps);
                    /* VHT Capability */
                    if ((caps & (vht_flags | he_flags
#ifdef WLAN_FEATURE_11BE
                                 | eht_flags
#endif /* WLAN_FEATURE_11BE */
                         )) &&
                        /* Include 2.4Ghz also if 256QAM is enabled */
                        (IEEE80211_IS_CHAN_11AC(vap->iv_bsschan) ||
                         IEEE80211_IS_CHAN_11NG(vap->iv_bsschan) ||
                         IEEE80211_IS_CHAN_HE(vap->iv_bsschan)
#ifdef WLAN_FEATURE_11BE
                         || IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)
#endif /* WLAN_FEATURE_11BE */
                        ) && ieee80211vap_vhtallowed(vap)) {
                        ieee80211_nawds_config_vht(ni, caps, FALSE);
                    }
                }
                /* HE capability */
                if ((caps & he_flags) && ieee80211vap_heallowed(vap))
                    ieee80211_nawds_config_he(ni, caps);

#ifdef WLAN_FEATURE_11BE
                if ((caps & eht_flags) &&
                    (ieee80211vap_ehtallowed(vap)) &&
                    IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)) {
                    ieee80211_nawds_config_vht(ni, caps, FALSE);
                    ieee80211_nawds_config_he(ni, caps);
                    ieee80211_nawds_config_eht(ni, caps, puncture_bitmap);
                }
#endif /* WLAN_FEATURE_11BE */
            }
    } else {
        /* non-ht/vht i.e legacy case */
        ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
        OS_MEMSET(&ni->ni_htrates, 0 , sizeof(struct ieee80211_rateset));
    }

    /* Update the PHY mode of the node */
    if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
        ni->ni_phymode = IEEE80211_MODE_11AXA_HE20;
    } else if (IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan)) {
        ni->ni_phymode = IEEE80211_MODE_11A;
    } else if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11G)) {
        ni->ni_phymode = IEEE80211_MODE_11G;
    } else {
        ni->ni_phymode = IEEE80211_MODE_11B;
    }

    ieee80211_nawds_update_chan_and_phymode(ni, puncture_bitmap);

    /* Rates are copied here for HE because it requires
     * ni's phymode which is set above
     */
    if (ieee80211vap_heallowed(vap)) {
        ieee80211_copy_hecap_rates(ic, vap, ni);
    }

    /* the node was created as a repeater to avoid kickout */
    ieee80211node_set_flag(ni, IEEE80211_NODE_WDS);
    ieee80211node_set_flag(ni, IEEE80211_NODE_NAWDS);
    wlan_peer_set_nawds(ni->peer_obj);

    ieee80211_validate_he_txrx_mcsnssmap(ic, ni);
#ifdef WLAN_FEATURE_11BE_MLO
    ieee80211_validate_eht_txrx_mcsnssmap(ic, ni);
#endif

    ieee80211_node_join(ni, 1);
    if (ic->ic_newassoc != NULL) {
        ret = ic->ic_newassoc(ni, 1);
        if (ret) {
            qdf_err("Node %s Param validation failed, disconnecting STA",
                     ether_sprintf(ni->ni_macaddr));
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            if (IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                ni->ni_macaddr, ni->ni_associd,
                                IEEE80211_STATUS_UNSPECIFIED,
                                &ni->peer_obj->mldaddr[0]);
#else
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                ni->ni_macaddr, ni->ni_associd,
                                IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
            }

            if (is_wds_ni)
                ieee80211_free_node(ni, WLAN_WDS_ID);

            return;
        }
    }

    ieee80211_node_authorize(ni);

    ic->ic_nss_change(ni);
    ic->ic_chwidth_change(ni);

    ni_wds = ieee80211_find_wds_node(nt, macaddr, WLAN_WDS_ID);
    if (ni_wds == NULL) {
        ieee80211_add_wds_addr(vaphandle, nt, ni, macaddr,
            IEEE80211_NODE_F_WDS_REMOTE);
    } else {
        ieee80211_free_node(ni_wds, WLAN_WDS_ID);
    }

    if (is_wds_ni)
        ieee80211_free_node(ni, WLAN_WDS_ID);
    return;
}
int wlan_nawds_config_mac(wlan_if_t vaphandle,
                          char *macaddr,
                          u_int64_t caps,
                          u_int32_t puncture_bitmap)
{
    int i, slot_free = -1, max_xretries = 0, slot_max_retries = 0, ret = 0;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    struct ieee80211com *ic = vap->iv_ic;
    nawds_rwlock_state_t(lock_state);
    struct ieee80211_ath_channel *chan = vap->iv_bsschan;
    bool is_6g = false;

#ifdef WLAN_FEATURE_11BE
    if (puncture_bitmap == PUNCTURE_INVALID) {
        puncture_bitmap = PUNCTURE_NONE;
    }
#endif

    ret = OS_MEMCMP(vaphandle->iv_myaddr, macaddr, QDF_MAC_ADDR_SIZE);
    if(!ret)
        return -EINVAL;

    /*do not add the Mac address in table if nawds is disable*/
    if(IEEE80211_NAWDS_DISABLED == nawds->mode){
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                "%s: nawds mode off\n", __func__);
       return -EINVAL;
    }

#ifdef WLAN_FEATURE_11BE
    if (nawds->def_puncture_bitmap == PUNCTURE_INVALID) {
        nawds->def_puncture_bitmap = PUNCTURE_NONE;
    }
#endif

    is_6g = IEEE80211_IS_CHAN_6GHZ(chan);
    /* If HE is enabled, enable HT and VHT
       If VHT is enabled, enable HT */
    if(caps & NAWDS_REPEATER_CAP_11ACVHT20){
        nawds->defcaps |= NAWDS_REPEATER_CAP_HT20;
        caps           |= NAWDS_REPEATER_CAP_HT20;
    } else if(caps & NAWDS_REPEATER_CAP_11ACVHT40){
        nawds->defcaps |= NAWDS_REPEATER_CAP_HT2040;
        caps           |= NAWDS_REPEATER_CAP_HT2040;
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE20 | NAWDS_REPEATER_CAP_11AXGHE20)) && !is_6g) {
        nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_HT20);
        caps           |= (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_HT20);
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE40 |  NAWDS_REPEATER_CAP_11AXGHE40)) && !is_6g) {
        nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_HT2040);
        caps           |= (NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_HT2040);
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE80 | NAWDS_REPEATER_CAP_11ACVHT80)) && !is_6g) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT80;
        caps           |= NAWDS_REPEATER_CAP_11ACVHT80;
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE80_80 | NAWDS_REPEATER_CAP_11ACVHT80_80)) &&!is_6g) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT80_80;
        caps           |= NAWDS_REPEATER_CAP_11ACVHT80_80;
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE160 | NAWDS_REPEATER_CAP_11ACVHT160)) && !is_6g) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT160;
        caps           |= NAWDS_REPEATER_CAP_11ACVHT160;
#ifdef WLAN_FEATURE_11BE
    } else if(caps & NAWDS_REPEATER_CAP_11BEAEHT20) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE20;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE20;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_HT20);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_HT20);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEGEHT20) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXGHE20;
        caps           |= NAWDS_REPEATER_CAP_11AXGHE20;
        if (!is_6g) {
             nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_HT20);
             caps           |= (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_HT20);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEAEHT40) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE40;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE40;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_HT2040);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_HT2040);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEGEHT40) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXGHE40;
        caps            |= NAWDS_REPEATER_CAP_11AXGHE40;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_HT2040);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT40 | NAWDS_REPEATER_CAP_HT2040);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEAEHT80) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE80;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE80;
        if (!is_6g) {
            nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT80;
            caps           |= NAWDS_REPEATER_CAP_11ACVHT80;
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEAEHT160) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE160;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE160;
        if (!is_6g) {
            nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT160;
            caps           |= NAWDS_REPEATER_CAP_11ACVHT160;
        }
#endif
    }

    /* sanity check */
    if (!is_nawds_valid_mac(macaddr) ||
        !is_nawds_valid_caps(vap, ic, caps)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                "%s: Invalid capabilities or MAC addr\n", __func__);
        return -EINVAL;
    }

    if (is_nawds_mac_bss_mac(ic, macaddr)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                "%s: Error: Attempt to add BSS MAC as repeater\n", __func__);
        return -EINVAL;
    }

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    /* try to find repeater with the mac and update the caps */
    for(i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
        if (IEEE80211_ADDR_EQ(nawds->repeater[i].mac, macaddr)) {
            nawds->repeater[i].caps = caps;
            wlan_nawds_config_repeater(vaphandle, macaddr, caps, puncture_bitmap);
            OS_RWLOCK_WRITE_UNLOCK(&nawds->lock, &lock_state);
            return 0;
        }
        if (!is_nawds_valid_mac(nawds->repeater[i].mac)) {
            if (slot_free == -1)
                slot_free = i;
        } else if (nawds->override) {
            /* the entry has a valid mac */
            struct ieee80211_node *ni = NULL;
            ni = ieee80211_find_node(ic, nawds->repeater[i].mac, WLAN_WDS_ID);
            if (!ni)
                continue;
            ieee80211_free_node(ni, WLAN_WDS_ID);
            if (ni->ni_consecutive_xretries >= max_xretries) {
                max_xretries = ni->ni_consecutive_xretries;
                slot_max_retries = i;
            }
        }
    }

    /* can't find the repeater for the given mac address */
    if (slot_free == -1) {
        if (!nawds->override) {
            NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
            return -ENOSPC;
        } else {
            ieee80211_nawds_node_leave(vaphandle,
                    nawds->repeater[slot_max_retries].mac);
            slot_free = slot_max_retries;
        }
    }

    /* clear the NAWDS repeater with the largest tx errors */

    /* configure the NAWDS repeater node */
    IEEE80211_ADDR_COPY(nawds->repeater[slot_free].mac, macaddr);
    nawds->repeater[slot_free].caps = caps;
    wlan_nawds_config_repeater(vaphandle, macaddr, caps, puncture_bitmap);

    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);

    return 0;
}

int wlan_nawds_config_key(wlan_if_t vaphandle, char *macaddr, char *psk)
{
    int ret = -1;
    int out_size;
    void *out;
    struct wlan_crypto_req_key req_key;
    int i = 0;
    char data[32];
    const char *alg = "sha256";
    const uint8_t *src;
    size_t src_len = strlen(psk);

    if (!is_nawds_valid_mac(macaddr)) {
        return -EINVAL;
    }

    qdf_mem_zero(&req_key, sizeof(struct wlan_crypto_req_key));
    qdf_mem_copy(req_key.macaddr, macaddr, QDF_MAC_ADDR_SIZE);

    for(i=0; i<6; i++)
        macaddr[i] ^= vaphandle->iv_myaddr[i];

    qdf_mem_set(data, sizeof(data), '\0');
    qdf_mem_copy(data, psk, strlen(psk));
    for (i = 0; i < strlen(psk); i=i+2) {
        data[i] ^= macaddr[i%QDF_MAC_ADDR_SIZE];
        data[i+1] ^= macaddr[(i+1)%QDF_MAC_ADDR_SIZE];
    }

    out_size = SHA256_DIGEST_SIZE;

    out = qdf_mem_malloc(out_size);
    if (!out)
        return -ENOMEM;
    qdf_mem_zero(out, out_size);

    src = data;
    ret = qdf_get_keyed_hash(alg, NULL, 0, &src, &src_len, 1, out);

    if (ret)
        goto free_out;

    if (((char *)out)[0] == 0)
        ((char *)out)[0] = 1;

    req_key.keylen = strlen(psk);
    if (strlen(psk) == 16)
        req_key.type   = WLAN_CRYPTO_CIPHER_AES_CCM;
    else if (strlen(psk) == 32)
        req_key.type   = WLAN_CRYPTO_CIPHER_AES_CCM_256;
    req_key.keyix  = 0;

    qdf_mem_copy(req_key.keydata, out, strlen(psk));

    ret = wlan_crypto_setkey(vaphandle->vdev_obj, &req_key);

    /* Zero-out local key variables */
    qdf_mem_zero(&req_key, sizeof(struct wlan_crypto_req_key));

free_out:
    kfree(out);

    return ret;
}

#if (MESH_MODE_SUPPORT||ATH_SUPPORT_NAC)
u_int64_t wlan_mesh_update_caps(u_int64_t caps)
{
    uint8_t bw;
    uint8_t nss;
    uint8_t mode;
    uint32_t short_preamble = 0;
    uint32_t short_slot = 0;

    short_slot = caps & MESH_CAPS_SHORT_SLOT;
    short_preamble = caps & MESH_CAPS_SHORT_PREAMBLE;

    if (caps & MESH_CAPS_VER1) {
        bw   = (caps >> MESH_CAPS_BW_OFFSET) & MESH_CAPS_NIBBLE_MASK;
        nss  = (caps >> MESH_CAPS_NSS_OFFSET) & MESH_CAPS_NIBBLE_MASK;
        mode = (caps >> MESH_CAPS_MODE_OFFSET) & MESH_CAPS_NIBBLE_MASK;

        if ((nss >= MESH_CAPS_MAX_NSS) || (mode >= MESH_PREAMBLE_MAX)
                                            || (bw >= MESH_PREAMBLE_MAX)) {
            qdf_err("%s: invalid nss %d bw %d mode %d !!!", __func__, nss, bw, mode);
            return QDF_STATUS_E_INVAL;
        }

        caps = nss ? (1 << (nss - 1)) : 0;

        if (mode == MESH_PREAMBLE_HT) {
            if (bw == MESH_BW_40) {
                caps |= NAWDS_REPEATER_CAP_HT2040;
            } else {
                caps |= NAWDS_REPEATER_CAP_HT20;
            }
        } else if (mode == MESH_PREAMBLE_VHT) {
            switch (bw) {
                case MESH_BW_160:   caps |= NAWDS_REPEATER_CAP_11ACVHT160; break;
                case MESH_BW_80_80: caps |= NAWDS_REPEATER_CAP_11ACVHT80_80; break;
                case MESH_BW_80:    caps |= NAWDS_REPEATER_CAP_11ACVHT80; break;
                case MESH_BW_40:    caps |= NAWDS_REPEATER_CAP_11ACVHT40; break;
                default:            caps |= NAWDS_REPEATER_CAP_11ACVHT20; break;
            }
        } else if (mode == MESH_PREAMBLE_HE) {
            switch (bw) {
                case MESH_BW_160:   caps |= NAWDS_REPEATER_CAP_11AXAHE160; break;
                case MESH_BW_80_80: caps |= NAWDS_REPEATER_CAP_11AXAHE80_80; break;
                case MESH_BW_80:    caps |= NAWDS_REPEATER_CAP_11AXAHE80; break;
                case MESH_BW_40:    caps |= NAWDS_REPEATER_CAP_11AXAHE40; break;
                default:            caps |= NAWDS_REPEATER_CAP_11AXAHE20; break;
            }
        }
#ifdef WLAN_FEATURE_11BE
        else if (mode == MESH_PREAMBLE_EHT) {
            switch(bw) {
                case MESH_BW_320:   caps |= NAWDS_REPEATER_CAP_11BEAEHT320; break;
                case MESH_BW_160:   caps |= NAWDS_REPEATER_CAP_11BEAEHT160; break;
                case MESH_BW_80:    caps |= NAWDS_REPEATER_CAP_11BEAEHT80; break;
                case MESH_BW_40:    caps |= NAWDS_REPEATER_CAP_11BEAEHT40; break;
                default:            caps |= NAWDS_REPEATER_CAP_11BEAEHT20; break;
            }
        }
#endif /* WLAN_FEATURE_11BE */

        caps |= short_slot | short_preamble;
        qdf_err("%s: derived ver1 caps %llx", __func__, caps);
    }

    return caps;
}

#endif

#if MESH_MODE_SUPPORT
#ifdef WLAN_FEATURE_11BE_MLO
void wlan_mlo_mesh_config(wlan_if_t vaphandle,
                          struct ieee80211_node *ni,
                          struct wlan_mlo_peer_context *ml_peer)
{
    struct ieee80211vap *vap = vaphandle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    int i;
    u_int64_t caps;
    u_int32_t puncture_bitmap;
    uint8_t *macaddr;
    struct ieee80211_ath_channel cur_chan;

    wlan_peer_obj_lock(peer);
    macaddr = wlan_peer_get_macaddr(peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
         if (IEEE80211_ADDR_EQ(macaddr, ml_peer->mesh_config[i].mac)) {
             caps = ml_peer->mesh_config[i].caps;
             puncture_bitmap = ml_peer->mesh_config[i].puncture_bitmap;
             break;
         }
    }

    wlan_peer_obj_unlock(peer);

    if (i == MAX_MLO_LINK_PEERS) {
        mlo_err("Could not configure mesh partner peer as link_addr mismatch");
        return;
    }

    cur_chan = *(vaphandle->iv_bsschan);
    puncture_bitmap |= cur_chan.op_puncture_bitmap;
    wlan_mesh_config_ni_params(vap, ni, caps, puncture_bitmap, true);
}

static
void mlmesh_populate_partner_info_from_repeater
                (struct mlo_partner_info *partner_info,
                struct ieee80211_wlanconfig_mesh *mlmesh, int index)
{
    int idx = partner_info->num_partner_links;

    if (partner_info->num_partner_links >=
                    (QDF_ARRAY_SIZE(partner_info->partner_link_info) - 1))
        return;

    partner_info->partner_link_info[idx].link_id = mlmesh->peers[index].link_id;
    qdf_mem_copy(&partner_info->partner_link_info[idx].link_addr.bytes[0],
                 mlmesh->peers[index].peer_link_mac,
                 sizeof(partner_info->partner_link_info[idx].link_addr.bytes));
    partner_info->partner_link_info[idx].mesh_config.caps = mlmesh->peers[index].caps;
    partner_info->partner_link_info[idx].mesh_config.puncture_bitmap =
            mlmesh->peers[index].puncture_bitmap;
    qdf_mem_copy(partner_info->partner_link_info[idx].mesh_config.mac,
                 mlmesh->peers[index].peer_link_mac,
                 sizeof(partner_info->partner_link_info[idx].mesh_config.mac));
    partner_info->num_partner_links++;
}

void wlan_mlmesh_assoc_peer(struct ieee80211vap *vap,
                            struct ieee80211_node *ni,
                            u_int8_t *peer_link_mac)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    int ret = 0;

    ni->ni_associd = peer->mlo_peer_ctx->assoc_id;

    if (ic->ic_newassoc != NULL) {
        ret = ic->ic_newassoc(ni, 1);
        if (ret) {
            qdf_err("Node %s Param validation failed, disconnecting STA",
                     ether_sprintf(ni->ni_macaddr));
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            if (IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                ni->ni_macaddr, ni->ni_associd,
                                IEEE80211_STATUS_UNSPECIFIED,
                                &ni->peer_obj->mldaddr[0]);
#else
                IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                ni->ni_macaddr, ni->ni_associd,
                                IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
            }
        }
    }
}

int wlan_localmlpeer_config(wlan_if_t vaphandle,
                            struct ieee80211_wlanconfig_mesh *mlmesh)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    struct mlo_partner_info partner_info = {0};
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct ieee80211vap *co_mld_vap;
    uint16_t num_links = 0;
    int i, j, rptinglinkid = 0;
    struct ieee80211_ath_channel cur_chan;

    /* if previously configured node found, start the node over */
    ni = ieee80211_find_node(ic, mlmesh->peers[0].peer_link_mac, WLAN_WDS_ID);
    if (!ni) {
        ni = find_logically_deleted_node_on_soc(
                wlan_vdev_get_psoc(vap->vdev_obj), mlmesh->peers[0].peer_link_mac, NULL, WLAN_WDS_ID);
    }

    if (ni) {
        ieee80211_free_node(ni, WLAN_WDS_ID);
        return -EINVAL;
    }

#ifdef AST_HKV1_WORKAROUND
    if (mlme_find_and_delete_wds_before_auth(vap, mlmesh->peers[0].peer_link_mac, NULL) == 0) {
        qdf_err("WDS entry exists");
        return -EINVAL;
    }
#endif

    ni = ieee80211_dup_bss(vap, mlmesh->peers[0].peer_link_mac);
    if (ni == NULL)
        return -EINVAL;

    cur_chan = *(vap->iv_bsschan);
    wlan_mesh_config_ni_params(vap, ni, mlmesh->peers[0].caps,
                               (mlmesh->peers[0].puncture_bitmap |
                               cur_chan.op_puncture_bitmap), true);

    /* free the extra refcount got from dup_bss() */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

    wlan_peer_set_mlo(ni->peer_obj);
    wlan_peer_mlme_set_assoc_peer(ni->peer_obj, true);
    OS_MEMCPY(ni->peer_obj->mldaddr, mlmesh->peer_mld_mac,
           sizeof(ni->peer_obj->mldaddr));
    partner_info.num_partner_links = 0;
    mlo_ap_get_vdev_list(vaphandle->vdev_obj, &num_links, vdev_list);
    for (i = 0; i < num_links; i++) {
         if (!vdev_list[i])
             continue;

         co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
         if (!co_mld_vap || (co_mld_vap == vap)) {
             mlo_release_vdev_ref(vdev_list[i]);
             continue;
         }

         for (j = 0; j < mlmesh->num_links; j++) {
             if (OS_MEMCMP(co_mld_vap->iv_myaddr, mlmesh->peers[j].vap_mac, QDF_MAC_ADDR_SIZE))
                 continue;

             if (partner_info.num_partner_links >=
                 (QDF_ARRAY_SIZE(partner_info.partner_link_info) - 1)) {
                 break;
             }

             mlmesh_populate_partner_info_from_repeater(&partner_info,
                                                        mlmesh, j);
             break;
         }
         mlo_release_vdev_ref(vdev_list[i]);
    }

    ni->ml_info = partner_info;
    ni->ml_info.num_partner_links++;
    rptinglinkid = ni->ml_info.num_partner_links - 1;

    ni->ml_info.partner_link_info[rptinglinkid].mesh_config.caps =
           mlmesh->peers[0].caps;
    ni->ml_info.partner_link_info[rptinglinkid].mesh_config.puncture_bitmap =
           mlmesh->peers[0].puncture_bitmap;
    OS_MEMCPY(ni->ml_info.partner_link_info[rptinglinkid].mesh_config.mac,
           mlmesh->peers[0].peer_link_mac,
           sizeof(ni->ml_info.partner_link_info[rptinglinkid].nawds_config.mac));

    ni->ml_info.partner_link_info[rptinglinkid].link_id = mlmesh->peers[0].link_id;
    OS_MEMCPY(ni->ml_info.partner_link_info[rptinglinkid].link_addr.bytes,
           ni->ni_macaddr,
           sizeof(ni->ml_info.partner_link_info[rptinglinkid].link_addr.bytes));

    ni->ni_ml_rptingrole = IEEE80211_MLO_RPTINGROLE_REPORTINGSTA;

    if (wlan_mlo_peer_create(vap->vdev_obj,
                             ni->peer_obj,
                             &ni->ml_info,
                             NULL, -1) != QDF_STATUS_SUCCESS) {
        wlan_peer_clear_mlo(ni->peer_obj);
        mlo_err("vdev%d: MLO peer create failed for MAC addr "QDF_MAC_ADDR_FMT,
                wlan_vdev_get_id(vap->vdev_obj),
                QDF_MAC_ADDR_REF(ni->ni_macaddr));
    } else {
        ni->ni_associd = ni->peer_obj->mlo_peer_ctx->assoc_id;
    }

    return 0;
}

#define MLO_MESH_MAX_LINKS 3
int wlan_add_localmlpeer(wlan_if_t vaphandle,
                         struct ieee80211_wlanconfig_mesh *mlmesh)
{
    int i, j;
    u_int64_t ret = 0;
    uint16_t num_links = 0;
    struct ieee80211vap *init_vap = vaphandle;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};

    if (!ieee80211_mlo_is_mld_ap(init_vap->vdev_obj)) {
        mlo_err("command initiated on non-mlo vap");
        return -EINVAL;
    }

    OS_MEMCPY(mlmesh->peers[0].vap_mac, vaphandle->iv_myaddr, QDF_MAC_ADDR_SIZE);
    mlo_ap_get_vdev_list(vaphandle->vdev_obj, &num_links, vdev_list);
    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return -EINVAL;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                num_links, QDF_ARRAY_SIZE(vdev_list));
        goto err;
    }
    if (num_links < mlmesh->num_links) {
        mlo_err("Incompatible number of links between AP and neighbour MESH.");
        goto err;
    }

    if (mlmesh->num_links > MLO_MESH_MAX_LINKS) {
        mlo_err("Max number of links supported are 3, returning");
        goto err;
    }

    for (j = 0; j < mlmesh->num_links; j++) {
        int found = 0;
        for (i = 0; i < num_links; i++) {
            struct ieee80211vap *co_mld_vap;

            if (!vdev_list[i])
                continue;

            co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
            if (!co_mld_vap) {
                continue;
            }
            if (OS_MEMCMP(co_mld_vap->iv_myaddr,
                          mlmesh->peers[j].vap_mac,
                          QDF_MAC_ADDR_SIZE)) {
                continue;
            } else {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            mlo_err("One of the vap mac addr do not match to the vaps under the MLD.");
            goto err;
        }
    }

    for (j = 0; j < mlmesh->num_links; j++) {
        ret = wlan_mesh_update_caps(mlmesh->peers[j].caps);
        if (ret == -1)
            goto err;
        mlmesh->peers[j].caps = ret;
    }

    if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
        mlo_err("Error while trying to release VDEV references - investigate.");
    return wlan_localmlpeer_config(vaphandle, mlmesh);

err:
    if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
        mlo_err("Error while trying to release VDEV references - investigate.");
    return -EINVAL;
}

int wlan_mesh_allowdata(wlan_if_t vaphandle, char *peer_mld_mac)
{
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct wlan_mlo_link_peer_entry *peer_entry = NULL;
    struct ieee80211vap *vap = vaphandle;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    struct ieee80211_node *ni;
    int i = 0;

    if (!vdev) {
        mlo_err("VDEV passed is NULL");
        return -1;
    }

    mlo_peer_ctx = wlan_mlo_get_mlpeer_by_mld_mac(vdev->mlo_dev_ctx,
                                                  (struct qdf_mac_addr *)peer_mld_mac);
    if (!mlo_peer_ctx) {
        mlo_err("MLO peer context passed is NULL");
        return -1;
    }

    for (i=0; i < mlo_peer_ctx->link_peer_cnt; i++) {
        peer_entry = &mlo_peer_ctx->peer_list[i];
        if (!peer_entry) {
            mlo_err("MLO peer entry not found");
            break;
        }

        vdev = wlan_peer_get_vdev(peer_entry->link_peer);
        if (!vdev) {
            mlo_err("Vdev not found");
            break;
        }

        ni = wlan_peer_get_mlme_ext_obj(peer_entry->link_peer);
        if (!ni) {
            mlo_err("ni is NULL for linked peer");
            break;
        }

        vap = wlan_vdev_get_vap(vdev);
        if (!vap) {
            mlo_err("VAP is NULL");
            continue;
        }
        if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap))
            ni->ni_capinfo |= IEEE80211_CAPINFO_PRIVACY;
        ieee80211_node_authorize(ni);
    }

    return 0;
}

int wlan_del_localmlpeer(wlan_if_t vaphandle, char *peer_mld_mac)
{
    struct ieee80211vap *vap = vaphandle;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct wlan_mlo_peer_context *mlo_peer_ctx = NULL;
    struct wlan_mlo_link_peer_entry *mlo_peer = NULL;
    uint16_t num_links = 0;
    bool match_found = false;
    struct qdf_mac_addr mac;
    int i;
    struct ieee80211_node *ni;

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj) || !vap->vdev_obj->mlo_dev_ctx) {
        mlo_err("command initiated on non-mlo vap or no ml peer found");
        return -EINVAL;
    }

    mlo_ap_get_vdev_list(vap->vdev_obj, &num_links, vdev_list);
    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return -EINVAL;
    }

    qdf_mem_copy(mac.bytes, peer_mld_mac, QDF_MAC_ADDR_SIZE);
    mlo_peer_ctx = wlan_mlo_get_mlpeer(vap->vdev_obj->mlo_dev_ctx, &mac);
    if(!mlo_peer_ctx) {
        mlo_err("No peer ctx for given mac");
        goto err;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                num_links, QDF_ARRAY_SIZE(vdev_list));
        goto err;
    }

    for (i = 0; i < mlo_peer_ctx->link_peer_cnt; i++) {
        mlo_peer = &mlo_peer_ctx->peer_list[i];
        if (!mlo_peer) {
            mlo_err("No ML Peer found");
            goto err;
        }

        ni = ieee80211_find_node(vap->iv_ic, mlo_peer->link_addr.bytes, WLAN_WDS_ID);
        if (ni) {
            match_found = true;
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
            IEEE80211_NODE_LEAVE(ni);
            ieee80211_free_node(ni, WLAN_WDS_ID);
            break;
        }
    }

    if (match_found) {
        if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                    QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
            mlo_err("Error while trying to release VDEV references - investigate.");
            return 0;
    }

    mlo_err("Ni match not found");
err:
    if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
        mlo_err("Error while trying to release VDEV references - investigate.");
    return -EINVAL;
}
#endif
#endif

#if (MESH_MODE_SUPPORT||ATH_SUPPORT_NAC)
int wlan_add_localpeer(wlan_if_t vaphandle, char *macaddr, u_int64_t caps,
                       u_int32_t puncture_bitmap)
{
    int prev_added = 0;
#ifdef WLAN_FEATURE_11BE
    struct ieee80211_ath_channel cur_chan;
    struct chan_params chan_p = {0};
#endif /* WLAN_FEATURE_11BE */
    u_int64_t ret = 0;

    ret = wlan_mesh_update_caps(caps);
    if (ret == -1)
        return EINVAL;
    caps = ret;
#ifdef WLAN_FEATURE_11BE
    cur_chan                  = *(vaphandle->iv_bsschan);
    chan_p.freq               = cur_chan.ic_freq;
    puncture_bitmap          |= cur_chan.op_puncture_bitmap;
    chan_p.op_puncture_bitmap = puncture_bitmap;
#endif /* WLAN_FEATURE_11BE */

    if (wlan_localpeer_config(vaphandle, macaddr, caps ,puncture_bitmap,
                              &prev_added) == NULL) {
        if (prev_added) {
            return EINVAL;
        } else {
            return ENOMEM;
        }
    }

    return 0;
}
#endif

int wlan_nawds_delete_mac(wlan_if_t vaphandle, char *macaddr)
{
    int i;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    nawds_rwlock_state_t(lock_state);

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    /* try to find repeater with the mac and update the caps */
    for(i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
        if (IEEE80211_ADDR_EQ(nawds->repeater[i].mac, macaddr)) {
            ieee80211_nawds_node_leave(vaphandle, macaddr);
            NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
            return 0;
        }
    }

    /* can't find the repeater for the given mac address */
    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
    return -ENXIO;
}

int wlan_nawds_get_mac(wlan_if_t vaphandle,
                       int num,
                       char *macaddr,
                       u_int64_t *caps)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    nawds_rwlock_state_t(lock_state);

    /* sanity check */
    if (num < 0 || num >= UMAC_MAX_NAWDS_REPEATER) {
        return -EINVAL;
    }

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    OS_MEMCPY(macaddr, nawds->repeater[num].mac, QDF_MAC_ADDR_SIZE);
    *caps = nawds->repeater[num].caps;

    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);

    return 0;
}

/* Send beacon control command to enable or disable beacon TX in beacon offload enabled mode */
static void wlan_nawds_beacon_control(wlan_if_t vaphandle,
                                      enum ieee80211_nawds_mode prev_mode,
                                      enum ieee80211_nawds_mode mode)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int ret = 0;

    switch(mode) {
    case IEEE80211_NAWDS_DISABLED:
    case IEEE80211_NAWDS_STATIC_REPEATER:
    case IEEE80211_NAWDS_LEARNING_REPEATER:
        /* Enable beaconing */
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            switch (prev_mode) {
                /* if it's one of the similar modes, don't send beacon template */
            case IEEE80211_NAWDS_DISABLED:
            case IEEE80211_NAWDS_STATIC_REPEATER:
            case IEEE80211_NAWDS_LEARNING_REPEATER:
                return;
            default:
                break;
            }

            ieee80211_mbssid_beacon_control(vap, MBSS_BCN_ENABLE);
        } else {
            if (ic->ic_beacon_offload_control) {
                ret = ic->ic_beacon_offload_control(vap, IEEE80211_BCN_OFFLD_TX_ENABLE);
            }
        }

        break;
    case IEEE80211_NAWDS_STATIC_BRIDGE:
    case IEEE80211_NAWDS_LEARNING_BRIDGE:
        if (IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap)) {
            switch(prev_mode) {
                /* if it's one of the similar modes, don't send beacon template */
            case IEEE80211_NAWDS_STATIC_BRIDGE:
            case IEEE80211_NAWDS_LEARNING_BRIDGE:
                return;
            default:
                break;
            }

            ieee80211_mbssid_beacon_control(vap, MBSS_BCN_DISABLE);
        } else {
            if (ic->ic_beacon_offload_control) {
                ret = ic->ic_beacon_offload_control(vap, IEEE80211_BCN_OFFLD_TX_DISABLE);
            }
        }

        break;
    }

    if (ret) {
        qdf_print("Failed to send beacon offload control message");
    }
}

int wlan_nawds_set_param(wlan_if_t vaphandle, enum ieee80211_nawds_param param, void *val)
{
    int ret = 0;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    struct ieee80211com *ic = vap->iv_ic;
    nawds_rwlock_state_t(lock_state);
    int mode, override;
    u_int64_t defcaps;
    bool is_mbssid_enabled = wlan_pdev_nif_feat_cap_get(ic->ic_pdev_obj,
                                                    WLAN_PDEV_F_MBSS_IE_ENABLE);

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    switch(param) {
        case IEEE80211_NAWDS_PARAM_MODE:
            mode = *((u_int8_t *) val);
            /* sanity check */
            if (mode < IEEE80211_NAWDS_DISABLED ||
                mode > IEEE80211_NAWDS_LEARNING_BRIDGE) {
                ret = -EINVAL;
                goto out;
            }
            if (mode == nawds->mode)
                goto out;

            if (is_mbssid_enabled &&
                !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(vap) &&
                (ieee80211_mbssid_get_num_beaconing_ap_vaps_up(
                                    ic, vap->iv_mbss.group_id) > 1)) {
                qdf_err("Operation is not supported for Transmitting VAP");
                goto out;
            }
            /* clear all nawds repeaters */
            ieee80211_nawds_node_leave_all(vap);
            wlan_nawds_beacon_control(vap,  nawds->mode, mode);
            /* change mode */
            nawds->mode = mode;

            break;
        case IEEE80211_NAWDS_PARAM_DEFCAPS:
            defcaps = *((u_int64_t *) val);
            if (!is_nawds_valid_caps(vap, ic, defcaps)) {
                ret = -EINVAL;
                goto out;
            }
            nawds->defcaps = defcaps;
            break;
        case IEEE80211_NAWDS_PARAM_OVERRIDE:
            override = *((u_int8_t *) val);
            nawds->override = override;
            break;
#ifdef WLAN_FEATURE_11BE
        case IEEE80211_NAWDS_PARAM_DEF_PUNCTURE_BITMAP:
            nawds->def_puncture_bitmap = *((u_int32_t *) val);
            break;
#endif
        default:
            ret = -EINVAL;
            goto out;
    }

out:
    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);

    return ret;
}

int wlan_nawds_get_param(wlan_if_t vaphandle, enum ieee80211_nawds_param param, void *val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    nawds_rwlock_state_t(lock_state);

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    switch(param) {
        case IEEE80211_NAWDS_PARAM_MODE:
            *((u_int8_t *) val) = nawds->mode;
            break;
        case IEEE80211_NAWDS_PARAM_DEFCAPS:
            *((u_int64_t *) val) = nawds->defcaps;
            break;
        case IEEE80211_NAWDS_PARAM_OVERRIDE:
            *((u_int8_t *) val) = nawds->override;
            break;
        case IEEE80211_NAWDS_PARAM_NUM:
            *((u_int8_t *) val) = UMAC_MAX_NAWDS_REPEATER;
            break;
        default:
            NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
            return -EINVAL;
    }

    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);

    return 0;
}
#endif /* UMAC_SUPPORT_NAWDS */

#ifdef UMAC_SUPPORT_MLNAWDS
void wlan_mlo_mlnawds_clear_nawds_entry(struct ieee80211vap *vap,
                                        uint8_t *peer_link_mac,
                                        uint8_t *peer_mld_mac)
{
    int i;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;

    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
         if ((IEEE80211_ADDR_EQ(nawds->repeater[i].peer_link_mac,
                                peer_link_mac)) &&
             (IEEE80211_ADDR_EQ(nawds->repeater[i].peer_mld_mac,
                                peer_mld_mac))) {
             break;
         }
    }

    if (i == UMAC_MAX_NAWDS_REPEATER)
        return;

    /* clear NAWDS node table for the mac */
    nawds->repeater[i].caps = 0;
    nawds->repeater[i].puncture_bitmap = 0;
    nawds->repeater[i].link_id = 0;
    OS_MEMZERO(nawds->repeater[i].peer_mld_mac, QDF_MAC_ADDR_SIZE);
    OS_MEMZERO(nawds->repeater[i].peer_link_mac, QDF_MAC_ADDR_SIZE);
    OS_MEMZERO(nawds->repeater[i].mac, QDF_MAC_ADDR_SIZE);
}

static void
ieee80211_mlnawds_node_leave(struct ieee80211vap *vap,
                             struct ieee80211_nawds_repeater *repeater)
{
    int i;
    struct ieee80211_node* ni;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    struct ieee80211com *ic = vap->iv_ic;

    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
         if ((IEEE80211_ADDR_EQ(nawds->repeater[i].peer_link_mac,
                                repeater->peer_link_mac)) &&
             (IEEE80211_ADDR_EQ(nawds->repeater[i].peer_mld_mac,
                                repeater->peer_mld_mac)))
             break;
    }

    if (i == UMAC_MAX_NAWDS_REPEATER)
        return;

    /* reclaim the node */
    ni = ieee80211_find_node(ic, repeater->peer_link_mac, WLAN_WDS_ID);
    if (ni) {
#ifdef QCA_SUPPORT_WDS_EXTENDED
        struct wlan_objmgr_psoc *psoc = NULL;

        psoc = wlan_pdev_get_psoc(ic->ic_pdev_obj);
        if (psoc && wlan_psoc_nif_feat_cap_get(psoc, WLAN_SOC_F_WDS_EXTENDED)) {
	    wlan_mlme_deauth_request(ni->ni_vap, ni->ni_macaddr,
                                     IEEE80211_REASON_UNSPECIFIED);
        }
#endif
        if (wlan_peer_is_mlo(ni->peer_obj)) {
            wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
        }

        IEEE80211_NODE_LEAVE(ni);
        ieee80211_free_node(ni, WLAN_WDS_ID);
    }

    /* clear NAWDS node table for the mac */
    nawds->repeater[i].caps = 0;
    nawds->repeater[i].puncture_bitmap = 0;
    nawds->repeater[i].link_id = 0;
    OS_MEMZERO(nawds->repeater[i].peer_mld_mac, QDF_MAC_ADDR_SIZE);
    OS_MEMZERO(nawds->repeater[i].peer_link_mac, QDF_MAC_ADDR_SIZE);
    OS_MEMZERO(nawds->repeater[i].mac, QDF_MAC_ADDR_SIZE);
}

static void
ieee80211_mlnawds_node_leave_all(struct ieee80211vap *vap)
{
    int i;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;

    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
         if (is_nawds_valid_mac(nawds->repeater[i].peer_mld_mac) &&
             is_nawds_valid_mac(nawds->repeater[i].peer_link_mac)) {
             ieee80211_mlnawds_node_leave(vap,
                                          &nawds->repeater[i]);
         }
    }
}

void wlan_mlnawds_assoc_peer(struct ieee80211vap *vap,
                             struct ieee80211_node *ni,
                             u_int8_t *peer_link_mac)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni_wds = NULL;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    int ret;

    if (ni->ni_chwidth != get_chwidth_phymode(ni->ni_phymode))
        ic->ic_chwidth_change(ni);

    ni->ni_associd = peer->mlo_peer_ctx->assoc_id;

    ieee80211_validate_he_txrx_mcsnssmap(ic, ni);
    ieee80211_validate_eht_txrx_mcsnssmap(ic, ni);
    if (ic->ic_newassoc != NULL) {
        ret = ic->ic_newassoc(ni, 1);
        if (ret) {
#ifdef WLAN_FEATURE_11BE_MLO
            if (wlan_peer_is_mlo(ni->peer_obj)) {
                wlan_mlo_partner_peer_disconnect_notify(ni->peer_obj);
            }
#endif
            if (IEEE80211_NODE_LEAVE(ni)) {
#ifdef WLAN_FEATURE_11BE_MLO
                  IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                      ni->ni_macaddr, ni->ni_associd,
                                      IEEE80211_STATUS_UNSPECIFIED,
                                      &ni->peer_obj->mldaddr[0]);
#else
                  IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                      ni->ni_macaddr, ni->ni_associd,
                                      IEEE80211_STATUS_UNSPECIFIED, NULL);
#endif /*WLAN_FEATURE_11BE_MLO*/
                  return;
            }
        }
    }

    ieee80211_node_authorize(ni);
    ic->ic_nss_change(ni);
    ic->ic_chwidth_change(ni);

    ni_wds = ieee80211_find_wds_node(nt, peer_link_mac, WLAN_WDS_ID);
    if (ni_wds == NULL) {
        ieee80211_add_wds_addr(vap, nt, ni,
                               peer_link_mac,
                               IEEE80211_NODE_F_WDS_REMOTE);
    } else {
        ieee80211_free_node(ni_wds, WLAN_WDS_ID);
    }
}

void wlan_mlnawds_config_ni_params(struct ieee80211vap *vap,
                                   struct ieee80211_node *ni,
                                   u_int64_t caps,
                                   u_int32_t puncture_bitmap)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int32_t ht_flags, vht_flags, he_flags;
    bool is_160_or_80p80_supported = false;
    bool is_6g_operation = false;
    uint64_t eht_flags;
    bool is_320_supported = false;

    if (ic->ic_modecaps &
                ((1 << IEEE80211_MODE_11AC_VHT160) |
                 (1 << IEEE80211_MODE_11AC_VHT80_80) |
                 (1 << IEEE80211_MODE_11AXA_HE160) |
                 (1ULL << IEEE80211_MODE_11AXA_HE80_80) |
                 (1ULL << IEEE80211_MODE_11BEA_EHT160))) {
        is_160_or_80p80_supported = true;
    }

    if (ic->ic_modecaps & (1ULL << IEEE80211_MODE_11BEA_EHT320)) {
        is_320_supported = true;
    }

    is_6g_operation = IEEE80211_IS_CHAN_6GHZ(vap->iv_bsschan);

    /* we do not want very low SNR as we may start at very poor rates,
     * so setting SNR to a normal value
     */
    ni->ni_snr = ATH_DEFAULT_NORMAL_SNR;

    /*
     * Set the default nss to 1,
     * the config ht/vht funcitons will take care of the value
     * configured in caps.
     */
    ni->ni_streams = 1;

    /* base configuration */
    ieee80211_nawds_config_base(ni, caps, 0);

    /* Enable HT40 if 40 MHz or higher width is requested */
    if (caps & (NAWDS_REPEATER_CAP_HT2040 | NAWDS_REPEATER_CAP_11ACVHT40
                | NAWDS_REPEATER_CAP_11ACVHT80 | NAWDS_REPEATER_CAP_11ACVHT80_80
                | NAWDS_REPEATER_CAP_11ACVHT160 |NAWDS_REPEATER_CAP_11AXAHE40
                | NAWDS_REPEATER_CAP_11AXGHE40 | NAWDS_REPEATER_CAP_11AXAHE80
                | NAWDS_REPEATER_CAP_11AXAHE80_80
                | NAWDS_REPEATER_CAP_11AXAHE160 | NAWDS_REPEATER_CAP_11BEAEHT40
                | NAWDS_REPEATER_CAP_11BEGEHT40 | NAWDS_REPEATER_CAP_11BEAEHT80
                | NAWDS_REPEATER_CAP_11BEAEHT160
                | NAWDS_REPEATER_CAP_11BEAEHT320)) {
        caps |= NAWDS_REPEATER_CAP_HT2040;
    }

    /* HT capability */
    ht_flags  = (NAWDS_REPEATER_CAP_HT20 | NAWDS_REPEATER_CAP_HT2040);
    vht_flags = (NAWDS_REPEATER_CAP_11ACVHT20 | NAWDS_REPEATER_CAP_11ACVHT40 |
                 NAWDS_REPEATER_CAP_11ACVHT80);
    he_flags  = (NAWDS_REPEATER_CAP_11AXAHE20 | NAWDS_REPEATER_CAP_11AXGHE20 |
                 NAWDS_REPEATER_CAP_11AXAHE40 | NAWDS_REPEATER_CAP_11AXGHE40 |
                 NAWDS_REPEATER_CAP_11AXAHE80);
    eht_flags = (NAWDS_REPEATER_CAP_11BEAEHT20 | NAWDS_REPEATER_CAP_11BEGEHT20 |
                 NAWDS_REPEATER_CAP_11BEAEHT40 | NAWDS_REPEATER_CAP_11BEGEHT40 |
                 NAWDS_REPEATER_CAP_11BEAEHT80);

    if (is_160_or_80p80_supported) {
        vht_flags |= (NAWDS_REPEATER_CAP_11ACVHT80_80 |
                      NAWDS_REPEATER_CAP_11ACVHT160);
         he_flags |= (NAWDS_REPEATER_CAP_11AXAHE80_80 |
                      NAWDS_REPEATER_CAP_11AXAHE160);
         eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT160;
    }

    if (is_320_supported) {
        he_flags  |= NAWDS_REPEATER_CAP_11AXAHE160;
        eht_flags |= NAWDS_REPEATER_CAP_11BEAEHT320;
    }

    if (caps & (ht_flags | vht_flags | he_flags | eht_flags)) {
        if (!is_6g_operation) {
            /* HT Capability */
            if (ieee80211vap_htallowed(ni->ni_vap))
                ieee80211_nawds_config_ht(ni, caps);
            /* VHT Capability */
            if ((caps & (vht_flags | he_flags | eht_flags)) &&
                /* Include 2.4Ghz also if 256QAM is enabled */
                (IEEE80211_IS_CHAN_11AC(vap->iv_bsschan) ||
                 IEEE80211_IS_CHAN_11NG(vap->iv_bsschan) ||
                 IEEE80211_IS_CHAN_HE(vap->iv_bsschan) ||
                 IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)) &&
                ieee80211vap_vhtallowed(vap)) {
                ieee80211_nawds_config_vht(ni, caps, FALSE);
            }
        }

        /* HE capability */
        if ((caps & he_flags) && ieee80211vap_heallowed(vap))
            ieee80211_nawds_config_he(ni, caps);
        if ((caps & eht_flags) &&
            (ieee80211vap_ehtallowed(vap)) &&
            IEEE80211_IS_CHAN_EHT(vap->iv_bsschan)) {
            ieee80211_nawds_config_vht(ni, caps, FALSE);
            ieee80211_nawds_config_he(ni, caps);
            ieee80211_nawds_config_eht(ni, caps, puncture_bitmap);
        }
    } else {
        /* non-ht/vht i.e legacy case */
        ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
        OS_MEMSET(&ni->ni_htrates, 0, sizeof(struct ieee80211_rateset));
    }
    /* Update the PHY mode of the node */
    if (IEEE80211_IS_CHAN_6GHZ(ic->ic_curchan)) {
        ni->ni_phymode = IEEE80211_MODE_11AXA_HE20;
    } else if (IEEE80211_IS_CHAN_5GHZ(ic->ic_curchan)) {
        ni->ni_phymode = IEEE80211_MODE_11A;
    } else if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11G)) {
        ni->ni_phymode = IEEE80211_MODE_11G;
    } else {
        ni->ni_phymode = IEEE80211_MODE_11B;
    }

    ieee80211_nawds_update_chan_and_phymode(ni, puncture_bitmap);

    /* Rates are copied here for HE because it requires
     * ni's phymode which is set above
     */
    if (ieee80211vap_heallowed(vap)) {
        ieee80211_copy_hecap_rates(ic, vap, ni);
    }

    /* the node was created as a repeater to avoid kickout */
    ieee80211node_set_flag(ni, IEEE80211_NODE_WDS);
    ieee80211node_set_flag(ni, IEEE80211_NODE_NAWDS);
    wlan_peer_set_nawds(ni->peer_obj);
    ieee80211_node_join(ni, 0);
}

void wlan_mlo_nawds_config(wlan_if_t vaphandle,
                           struct ieee80211_node *ni,
                           struct wlan_mlo_peer_context *ml_peer)
{
    struct ieee80211vap *vap = vaphandle;
    struct wlan_objmgr_peer *peer = ni->peer_obj;
    int i;
    u_int64_t caps;
    u_int32_t puncture_bitmap;
    uint8_t *macaddr;
    struct ieee80211_ath_channel cur_chan;

    ni->ni_associd = ml_peer->assoc_id;
    wlan_peer_obj_lock(peer);
    macaddr = wlan_peer_get_macaddr(peer);

    for (i = 0; i < MAX_MLO_LINK_PEERS; i++) {
         if (IEEE80211_ADDR_EQ(macaddr, ml_peer->nawds_config[i].mac)) {
             caps = ml_peer->nawds_config[i].caps;
             puncture_bitmap = ml_peer->nawds_config[i].puncture_bitmap;
             break;
         }
    }

    wlan_peer_obj_unlock(peer);

    if (i == MAX_MLO_LINK_PEERS) {
        mlo_err("Could not configure nawds partner peer as link_addr mismatch");
        return;
    }

    cur_chan = *(vaphandle->iv_bsschan);
    puncture_bitmap |= cur_chan.op_puncture_bitmap;
    wlan_mlnawds_config_ni_params(vap, ni, caps, puncture_bitmap);
}

static
void mlnawds_populate_partner_info_from_repeater
                (struct mlo_partner_info *partner_info,
                 struct ieee80211_nawds_repeater *repeater)
{
    int idx = partner_info->num_partner_links;

    if (partner_info->num_partner_links >=
                    (QDF_ARRAY_SIZE(partner_info->partner_link_info) - 1))
        return;

    partner_info->partner_link_info[idx].link_id = repeater->link_id;
    qdf_mem_copy(&partner_info->partner_link_info[idx].link_addr.bytes[0],
                 repeater->peer_link_mac,
                 sizeof(partner_info->partner_link_info[idx].link_addr.bytes));
    partner_info->partner_link_info[idx].nawds_config.caps = repeater->caps;
    partner_info->partner_link_info[idx].nawds_config.puncture_bitmap =
            repeater->puncture_bitmap;
    qdf_mem_copy(partner_info->partner_link_info[idx].nawds_config.mac,
                 repeater->peer_link_mac,
                 sizeof(partner_info->partner_link_info[idx].nawds_config.mac));
    partner_info->num_partner_links++;
}

static
struct ieee80211_node *wlan_mlnawds_config_repeater(struct ieee80211vap *vap,
                                struct ieee80211_nawds_repeater *repeater)
{
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct mlo_partner_info partner_info;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct ieee80211vap *co_mld_vap;
    uint16_t num_links = 0;
    int i, j, rptinglinkid = 0;
    struct ieee80211_ath_channel cur_chan;

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        mlo_err("Command not initiated on MLO supported vap");
        return NULL;
    }

    mlo_ap_get_active_vdev_list(vap->vdev_obj, &num_links, vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return NULL;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                num_links, QDF_ARRAY_SIZE(vdev_list));
        if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                    QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
            mlo_err("Error while trying to release VDEV references - investigate.");
        return NULL;
    }

    /* if previously configured node found, start the node over */
    ni = ieee80211_find_node(ic, repeater->peer_link_mac, WLAN_WDS_ID);
    if (ni) {
        ieee80211_free_node(ni, WLAN_WDS_ID);
        goto reconfigure;
    } else {
        ni = find_logically_deleted_node_pdev_psoc(ic,
                                                   repeater->peer_link_mac,
                                                   WLAN_WDS_ID);
        if (ni) {
            /* If FW delete resp pending then return else continue
             * with new node creation
             */
            if (qdf_atomic_read(&ni->ni_fw_peer_delete_rsp_pending)) {
                qdf_debug("delete response pending for peer %s",
                          repeater->peer_link_mac);
                ieee80211_free_node(ni, WLAN_WDS_ID);
                return NULL;
            }
            ieee80211_free_node(ni, WLAN_WDS_ID);
        }
    }

    ni = ieee80211_dup_bss(vap, repeater->peer_link_mac);
    if (ni == NULL)
        return NULL;

    /* free the extra refcount got from dup_bss() */
    ieee80211_free_node(ni, WLAN_MLME_OBJMGR_ID);

reconfigure:
    cur_chan = *(vap->iv_bsschan);

    wlan_mlnawds_config_ni_params(vap, ni,
                                  repeater->caps,
                                  (repeater->puncture_bitmap |
                                   cur_chan.op_puncture_bitmap));
    wlan_peer_set_mlo(ni->peer_obj);
    wlan_peer_mlme_set_assoc_peer(ni->peer_obj, true);
    memcpy(ni->peer_obj->mldaddr, repeater->peer_mld_mac,
           sizeof(ni->peer_obj->mldaddr));
    partner_info.num_partner_links = 0;
    for (i = 0; i < num_links; i++) {
         struct ieee80211_nawds *nawds;

         if (!vdev_list[i])
             continue;

         co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
         if (!co_mld_vap || (co_mld_vap == vap)) {
             mlo_release_vdev_ref(vdev_list[i]);
             continue;
         }

         nawds = &co_mld_vap->iv_nawds;
         for (j = 0; j < UMAC_MAX_NAWDS_REPEATER; j++) {
             if (!IEEE80211_ADDR_EQ(nawds->repeater[j].peer_mld_mac,
                                    repeater->peer_mld_mac))
                 continue;

             if (partner_info.num_partner_links >=
                 (WLAN_MAX_ML_BSS_LINKS - 1)) {
                 break;
             }

             mlnawds_populate_partner_info_from_repeater(&partner_info,
                                                         &nawds->repeater[j]);
             break;
         }
         mlo_release_vdev_ref(vdev_list[i]);
    }

    ni->ml_info = partner_info;
    ni->ml_info.num_partner_links++;
    rptinglinkid = ni->ml_info.num_partner_links - 1;

    ni->ml_info.partner_link_info[rptinglinkid].nawds_config.caps =
           repeater->caps;
    ni->ml_info.partner_link_info[rptinglinkid].nawds_config.puncture_bitmap =
           repeater->puncture_bitmap;
    memcpy(ni->ml_info.partner_link_info[rptinglinkid].nawds_config.mac,
           repeater->peer_link_mac,
           sizeof(ni->ml_info.partner_link_info[rptinglinkid].nawds_config.mac));

    ni->ml_info.partner_link_info[rptinglinkid].link_id = repeater->link_id;
    memcpy(ni->ml_info.partner_link_info[rptinglinkid].link_addr.bytes,
           ni->ni_macaddr,
           sizeof(ni->ml_info.partner_link_info[rptinglinkid].link_addr.bytes));

    ni->ni_ml_rptingrole = IEEE80211_MLO_RPTINGROLE_REPORTINGSTA;

    if (wlan_mlo_peer_create(vap->vdev_obj,
                             ni->peer_obj,
                             &ni->ml_info,
                             NULL, -1) != QDF_STATUS_SUCCESS) {
        wlan_peer_clear_mlo(ni->peer_obj);
        mlo_err("vdev%d: MLO peer create failed for MAC addr "QDF_MAC_ADDR_FMT,
                wlan_vdev_get_id(vap->vdev_obj),
                QDF_MAC_ADDR_REF(ni->ni_macaddr));
    } else {
        ni->ni_associd = ni->peer_obj->mlo_peer_ctx->assoc_id;
    }

    return ni;
}

void wlan_mlnawds_start(wlan_if_t vaphandle, char *peer_mld_macaddr)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    int i;

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        mlo_err("command initiated on non-mlo vap");
        return;
    }

    /* check if the user provided peer_mld_macaddr is added in current VAP's
     * NAWDS repeater list. If not found, alert user
     */
    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
         if (IEEE80211_ADDR_EQ(nawds->repeater[i].peer_mld_mac,
             peer_mld_macaddr)) {
             break;
         }
    }

    if (i >= UMAC_MAX_NAWDS_REPEATER) {
        mlo_err("NAWDS peer mld mac not found in this vaps's repeater list");
        return;
    }

    wlan_mlnawds_config_repeater(vap, &nawds->repeater[i]);
}

int wlan_mlnawds_config_mac(wlan_if_t vaphandle,
                            struct ieee80211_wlanconfig_nawds *mlnawds)
{
    int i, slot_free = -1, max_xretries = 0, slot_max_retries = 0, ret = 0;
    struct ieee80211vap *vap = NULL;
    struct ieee80211vap *init_vap = vaphandle;
    struct ieee80211_nawds *nawds;
    struct ieee80211com *ic;
    nawds_rwlock_state_t(lock_state);
    struct ieee80211_ath_channel *chan;
    bool is_6g = false;
    u_int64_t caps = mlnawds->caps;

    if (mlnawds->puncture_bitmap == PUNCTURE_INVALID) {
        mlnawds->puncture_bitmap = PUNCTURE_NONE;
    }

    if (!ieee80211_mlo_is_mld_ap(init_vap->vdev_obj)) {
        mlo_err("command initiated on non-mlo vap");
        return -EINVAL;
    }

    ret = OS_MEMCMP(vaphandle->iv_myaddr, mlnawds->mac, QDF_MAC_ADDR_SIZE);
    if (ret) {
        /* the link mac provided by user is not matching with the vap address
         * check if the mac is matching with co_mld vap or not. If not
         * matched, return error
         */
        struct ieee80211vap *co_mld_vap;
        struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
        uint16_t num_links = 0;

        mlo_ap_get_active_vdev_list(vaphandle->vdev_obj, &num_links, vdev_list);
        if (!num_links) {
            mlo_err("Number of VDEVs under AP-MLD is reported as 0");
            return -EINVAL;
        }

        if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
            mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                    num_links, QDF_ARRAY_SIZE(vdev_list));
            if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                        QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
                mlo_err("Error while trying to release VDEV references - investigate.");
            return -EINVAL;
        }

        for (i = 0; i < num_links; i++) {
             if (!vdev_list[i])
                 continue;

             co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
             if (!co_mld_vap) {
                 mlo_release_vdev_ref(vdev_list[i]);
                 continue;
             }

             if (OS_MEMCMP(co_mld_vap->iv_myaddr,
                           mlnawds->mac,
                           QDF_MAC_ADDR_SIZE)) {
                 mlo_release_vdev_ref(vdev_list[i]);
                 continue;
             } else {
                 vap = co_mld_vap;
                 mlo_release_vdev_ref(vdev_list[i]);
                 break;
             }
        }

        if (!vap) {
            mlo_err("Command initiated on wrong vap");
            return -EINVAL;
        }
    } else {
        vap = init_vap;
    }

    ret = OS_MEMCMP(vap->iv_myaddr,
                    mlnawds->peer_link_mac,
                    QDF_MAC_ADDR_SIZE);
    if (!ret) {
        mlo_err("NAWDS peer link mac is same as self address");
        return -EINVAL;
    }

    chan = vap->iv_bsschan;
    ic = vap->iv_ic;
    nawds = &vap->iv_nawds;
    /* do not add the Mac address in table if nawds is disable */
    if (IEEE80211_NAWDS_DISABLED == nawds->mode) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                 "%s: nawds mode off\n", __func__);
        return -EINVAL;
    }

    if (mlnawds->def_puncture_bitmap == PUNCTURE_INVALID) {
        mlnawds->def_puncture_bitmap = PUNCTURE_NONE;
    }

    nawds->def_puncture_bitmap = mlnawds->def_puncture_bitmap;

    is_6g = IEEE80211_IS_CHAN_6GHZ(chan);
    /* If HE is enabled, enable HT and VHT
      If VHT is enabled, enable HT */
    if (caps & NAWDS_REPEATER_CAP_11ACVHT20) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_HT20;
        caps           |= NAWDS_REPEATER_CAP_HT20;
    } else if(caps & NAWDS_REPEATER_CAP_11ACVHT40) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_HT2040;
        caps           |= NAWDS_REPEATER_CAP_HT2040;
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE20 |
                       NAWDS_REPEATER_CAP_11AXGHE20)) && !is_6g) {
        nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT20 |
                           NAWDS_REPEATER_CAP_HT20);
        caps           |= (NAWDS_REPEATER_CAP_11ACVHT20 |
                           NAWDS_REPEATER_CAP_HT20);
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE40 |
                       NAWDS_REPEATER_CAP_11AXGHE40)) && !is_6g) {
        nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT40 |
                           NAWDS_REPEATER_CAP_HT2040);
        caps           |= (NAWDS_REPEATER_CAP_11ACVHT40 |
                           NAWDS_REPEATER_CAP_HT2040);
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE80 |
                       NAWDS_REPEATER_CAP_11ACVHT80)) && !is_6g) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT80;
        caps           |= NAWDS_REPEATER_CAP_11ACVHT80;
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE80_80 |
                       NAWDS_REPEATER_CAP_11ACVHT80_80)) &&!is_6g) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT80_80;
        caps           |= NAWDS_REPEATER_CAP_11ACVHT80_80;
    } else if((caps & (NAWDS_REPEATER_CAP_11AXAHE160 |
                       NAWDS_REPEATER_CAP_11ACVHT160)) && !is_6g) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT160;
        caps           |= NAWDS_REPEATER_CAP_11ACVHT160;
    } else if(caps & NAWDS_REPEATER_CAP_11BEAEHT20) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE20;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE20;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT20 |
                               NAWDS_REPEATER_CAP_HT20);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT20 |
                               NAWDS_REPEATER_CAP_HT20);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEGEHT20) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXGHE20;
        caps           |= NAWDS_REPEATER_CAP_11AXGHE20;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT20 |
                               NAWDS_REPEATER_CAP_HT20);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT20 |
                               NAWDS_REPEATER_CAP_HT20);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEAEHT40) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE40;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT40 |
                               NAWDS_REPEATER_CAP_HT2040);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT40 |
                               NAWDS_REPEATER_CAP_HT2040);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEGEHT40) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXGHE40;
        caps           |= NAWDS_REPEATER_CAP_11AXGHE40;
        if (!is_6g) {
            nawds->defcaps |= (NAWDS_REPEATER_CAP_11ACVHT40 |
                               NAWDS_REPEATER_CAP_HT2040);
            caps           |= (NAWDS_REPEATER_CAP_11ACVHT40 |
                               NAWDS_REPEATER_CAP_HT2040);
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEAEHT80) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE80;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE80;
        if (!is_6g) {
            nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT80;
            caps           |= NAWDS_REPEATER_CAP_11ACVHT80;
        }
    } else if (caps & NAWDS_REPEATER_CAP_11BEAEHT160) {
        nawds->defcaps |= NAWDS_REPEATER_CAP_11AXAHE160;
        caps           |= NAWDS_REPEATER_CAP_11AXAHE160;
        if (!is_6g) {
            nawds->defcaps |= NAWDS_REPEATER_CAP_11ACVHT160;
            caps           |= NAWDS_REPEATER_CAP_11ACVHT160;
        }
    }

    /* sanity check */
    if (!is_nawds_valid_mac(mlnawds->peer_link_mac) ||
        !is_nawds_valid_mac(mlnawds->peer_mld_mac) ||
        !is_nawds_valid_caps(vap, ic, caps)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                "%s: Invalid capabilities or MAC addr\n", __func__);
        return -EINVAL;
    }

    if (is_nawds_mac_bss_mac(ic, mlnawds->peer_link_mac)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WDS,
                "%s: Error: Attempt to add BSS MAC as repeater\n", __func__);
        return -EINVAL;
    }

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    /* try to find repeater with the mac and update the caps */
    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
         if (IEEE80211_ADDR_EQ(nawds->repeater[i].peer_link_mac,
                               mlnawds->peer_link_mac) &&
             IEEE80211_ADDR_EQ(nawds->repeater[i].peer_mld_mac,
                               mlnawds->peer_mld_mac)) {
             nawds->repeater[i].caps = caps;
             nawds->repeater[i].puncture_bitmap = mlnawds->puncture_bitmap;
             nawds->repeater[i].link_id = mlnawds->link_id;
             OS_RWLOCK_WRITE_UNLOCK(&nawds->lock, &lock_state);
             return 0;
         }

         if (!is_nawds_valid_mac(nawds->repeater[i].peer_link_mac) ||
             !is_nawds_valid_mac(nawds->repeater[i].peer_mld_mac)) {
             if (slot_free == -1)
                 slot_free = i;
         } else if (nawds->override) {
             /* the entry has a valid mac */
             struct ieee80211_node *ni = NULL;

             ni = ieee80211_find_node(ic,
                                      nawds->repeater[i].peer_link_mac,
                                      WLAN_WDS_ID);
             if (!ni)
                 continue;
             ieee80211_free_node(ni, WLAN_WDS_ID);
             if (ni->ni_consecutive_xretries >= max_xretries) {
                 max_xretries = ni->ni_consecutive_xretries;
                 slot_max_retries = i;
             }
         }
    }

    /* can't find the repeater for the given mac address */
    if (slot_free == -1) {
        if (!nawds->override) {
            NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
            return -ENOSPC;
        } else {
            ieee80211_mlnawds_node_leave(vap,
                                         &nawds->repeater[slot_max_retries]);
            slot_free = slot_max_retries;
        }
    }

    /* clear the NAWDS repeater with the largest tx errors */

    /* configure the NAWDS repeater node */
    IEEE80211_ADDR_COPY(nawds->repeater[slot_free].peer_link_mac,
                        mlnawds->peer_link_mac);
    IEEE80211_ADDR_COPY(nawds->repeater[slot_free].peer_mld_mac,
                        mlnawds->peer_mld_mac);
    nawds->repeater[slot_free].caps = caps;
    nawds->repeater[slot_free].puncture_bitmap = mlnawds->puncture_bitmap;
    nawds->repeater[slot_free].link_id = mlnawds->link_id;

    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);

    return 0;
}

int wlan_mlnawds_config_key(wlan_if_t vaphandle,
                            char *peer_mld_mac,
                            char *psk)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211vap *co_mld_vap;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct wlan_crypto_req_key req_key;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    const uint8_t *src;
    void *out;
    const char *alg = "sha256";
    char data[32];
    char peer_link_mac[IEEE80211_ADDR_LEN];
    size_t src_len = strlen(psk);
    uint16_t num_links = 0;
    int ret = -EINVAL;
    int out_size, i, j, k;

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        mlo_err("Command not initiated on MLO supported vap");
        return -EINVAL;
    }

    if (!is_nawds_valid_mac(peer_mld_mac)) {
        mlo_err("Invalid mac address");
        return -EINVAL;
    }

    /* chek if the peer mld is added in this vaps nawds list */
    for (i = 0; i < UMAC_MAX_NAWDS_REPEATER; i++) {
         if (IEEE80211_ADDR_EQ(nawds->repeater[i].peer_mld_mac, peer_mld_mac))
             break;
    }

    if (i == UMAC_MAX_NAWDS_REPEATER) {
        mlo_err("peer mld mac provided is not found in this vaps nawds list");
        return -EINVAL;
    }

    mlo_ap_get_active_vdev_list(vaphandle->vdev_obj, &num_links, vdev_list);
    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return -EINVAL;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                num_links, QDF_ARRAY_SIZE(vdev_list));
        if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                    QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
            mlo_err("Error while trying to release VDEV references - investigate.");
        return -EINVAL;
    }

    out_size = SHA256_DIGEST_SIZE;
    out = qdf_mem_malloc(out_size);

    if (!out) {
        if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                    QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
            mlo_err("Error while trying to release VDEV references - investigate.");

        return -ENOMEM;
    }

    qdf_mem_zero(out, out_size);

    for (i = 0; i < num_links; i++) {
         if (!vdev_list[i])
             continue;

         co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
         if (!co_mld_vap) {
             mlo_release_vdev_ref(vdev_list[i]);
             continue;
         }
         memcpy(co_mld_vap->iv_nawds.psk, psk, src_len);
         nawds = &co_mld_vap->iv_nawds;
         for (j = 0; j < UMAC_MAX_NAWDS_REPEATER; j++) {
              if (IEEE80211_ADDR_EQ(nawds->repeater[j].peer_mld_mac,
                                    peer_mld_mac)) {
                  qdf_mem_copy(peer_link_mac,
                               nawds->repeater[j].peer_link_mac,
                               IEEE80211_ADDR_LEN);

                  qdf_mem_zero(&req_key, sizeof(struct wlan_crypto_req_key));

                  qdf_mem_copy(req_key.macaddr,
                               peer_link_mac,
                               QDF_MAC_ADDR_SIZE);

                  for (k = 0; k < 6; k++)
                       peer_link_mac[k] ^= co_mld_vap->iv_myaddr[k];

                  qdf_mem_set(data, sizeof(data), '\0');
                  qdf_mem_copy(data, psk, strlen(psk));
                  for (k = 0; k < strlen(psk); k=k+2) {
                       data[k] ^= peer_link_mac[k%QDF_MAC_ADDR_SIZE];
                       data[k+1] ^= peer_link_mac[(k+1)%QDF_MAC_ADDR_SIZE];
                  }
                  qdf_mem_zero(out, out_size);
                  src = data;
                  ret = qdf_get_keyed_hash(alg, NULL,
                                           0, &src,
                                           &src_len, 1,
                                           out);
                  if (ret) {
                      if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                                  QDF_ARRAY_SIZE(vdev_list), num_links, i)) != QDF_STATUS_SUCCESS)
                          mlo_err("Error while trying to release VDEV references - investigate.");
                      goto free_out;
                  }

                  if (((char *)out)[0] == 0)
                      ((char *)out)[0] = 1;

                  req_key.keylen = strlen(psk);
                  if (strlen(psk) == 16)
                      req_key.type   = WLAN_CRYPTO_CIPHER_AES_CCM;
                  else if (strlen(psk) == 32)
                      req_key.type   = WLAN_CRYPTO_CIPHER_AES_CCM_256;
                  req_key.keyix  = 0;
                  qdf_mem_copy(req_key.keydata, out, strlen(psk));
                  ret = wlan_crypto_setkey(co_mld_vap->vdev_obj, &req_key);
                  qdf_mem_zero(&req_key, sizeof(struct wlan_crypto_req_key));
              }
         }
         mlo_release_vdev_ref(vdev_list[i]);
    }
free_out:
    qdf_mem_free(out);
    return ret;
}

int wlan_mlnawds_delete_mac(wlan_if_t vaphandle, char *peer_mld_mac)
{
    struct ieee80211vap *vap = vaphandle;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct ieee80211vap *co_mld_vap;
    struct ieee80211_nawds *mlnawds;
    uint16_t num_links = 0;
    bool match_found = false;
    int i, j;
    nawds_rwlock_state_t(lock_state);

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        mlo_err("command initiated on non-mlo vap");
        return -EINVAL;
    }

    mlo_ap_get_active_vdev_list(vap->vdev_obj, &num_links, vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return -EINVAL;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                num_links, QDF_ARRAY_SIZE(vdev_list));
        if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                    QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
            mlo_err("Error while trying to release VDEV references - investigate.");
        return -EINVAL;
    }

    for (i = 0; i < num_links; i++) {
         co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
         if (!co_mld_vap)
             continue;

         mlnawds = &co_mld_vap->iv_nawds;
         NAWDS_WRITE_LOCK(&mlnawds->lock, &lock_state);
         for (j = 0; j < UMAC_MAX_NAWDS_REPEATER; j++) {
              if (IEEE80211_ADDR_EQ(mlnawds->repeater[j].peer_mld_mac,
                                    peer_mld_mac)) {
                  match_found = true;
                  ieee80211_mlnawds_node_leave(co_mld_vap,
                                               &mlnawds->repeater[j]);
                  break;
              }
         }
         NAWDS_WRITE_UNLOCK(&mlnawds->lock, &lock_state);
         if (match_found)
             break;
    }

    if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
        mlo_err("Error while trying to release VDEV references");

    if (!match_found) {
        mlo_err("No entry found in nawds list for the given mld mac");
        return -EINVAL;
    }

    return 0;
}

int wlan_mlnawds_set_param(wlan_if_t vaphandle,
                           enum ieee80211_nawds_param param,
                           struct ieee80211_wlanconfig_nawds *mlnawds)
{
    int ret = 0;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    nawds_rwlock_state_t(lock_state);
    int mode;
    struct wlan_objmgr_vdev *vdev_list[WLAN_UMAC_MLO_MAX_VDEVS] = {NULL};
    struct ieee80211vap *co_mld_vap;
    struct wlan_objmgr_vdev *vdev = vap->vdev_obj;
    uint16_t num_links = 0;
    int i;
    struct qdf_mac_addr *mld_addr;
    bool mode_changed = false;
    struct ieee80211com *ic = vap->iv_ic;
    bool is_mbssid_enabled;

    if (!ieee80211_mlo_is_mld_ap(vdev)) {
        mlo_err("Command initiated in non-MLO vap");
        return -EINVAL;
    }

    /* check if the command initiated vap belongs to the provided mld mac */
    wlan_vdev_obj_lock(vap->vdev_obj);
    mld_addr = (struct qdf_mac_addr *)wlan_vdev_mlme_get_mldaddr(vap->vdev_obj);
    if (qdf_mem_cmp(mld_addr, mlnawds->mld_mac, QDF_MAC_ADDR_SIZE)) {
        mlo_err("User provided MLD mac does not match the interface choosen");
        wlan_vdev_obj_unlock(vap->vdev_obj);
        return -EINVAL;
    }

    wlan_vdev_obj_unlock(vap->vdev_obj);
    mlo_ap_get_active_vdev_list(vaphandle->vdev_obj, &num_links, vdev_list);

    if (!num_links) {
        mlo_err("Number of VDEVs under AP-MLD is reported as 0");
        return -EINVAL;
    }

    if (num_links > QDF_ARRAY_SIZE(vdev_list)) {
        mlo_err("Count of VDEVs %u under AP-MLD exceeds expected limit %zu",
                num_links, QDF_ARRAY_SIZE(vdev_list));
        if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                    QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
            mlo_err("Error while trying to release VDEV references - investigate.");
        return -EINVAL;
    }

    switch(param) {
       case IEEE80211_MLNAWDS_PARAM_MODE:
           mode = mlnawds->mode;
           /* sanity check */
           if (mode < IEEE80211_NAWDS_DISABLED ||
               mode > IEEE80211_NAWDS_STATIC_BRIDGE) {
               ret = -EINVAL;
               goto out;
           }

           /* loop through all vaps of mlnawds->mld_mac & check if any co_mld_vap
            * is tx vap with additional non-tx vaps in its MBSSID group.
            * If thats the case, do not allow NAWDS to be configured on any of
            * co_mld_vaps as NAWDS can go beacon disabled mode.
            */
           for (i = 0; i < num_links; i++) {
                co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
                if (!co_mld_vap)
                    continue;

                ic = co_mld_vap->iv_ic;
                is_mbssid_enabled = wlan_pdev_nif_feat_cap_get
                        (ic->ic_pdev_obj, WLAN_PDEV_F_MBSS_IE_ENABLE);
                if (is_mbssid_enabled &&
                    !IEEE80211_VAP_IS_MBSS_NON_TRANSMIT_ENABLED(co_mld_vap) &&
                    (ieee80211_mbssid_get_num_beaconing_ap_vaps_up(ic,
                                co_mld_vap->iv_mbss.group_id)) > 1) {
                    qdf_err("Operation not permitted on tx-vap with multi vaps in mbssid group");
                    ret = -EINVAL;
                    goto out;
                }
           }

           /* loop through all vaps of mlnawds->mld_mac &
            * set the mode. Ensure to leave all the nodes
            * associated with vap
            */
           for (i = 0; i < num_links; i++) {
                co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
                if (!co_mld_vap)
                    continue;

                nawds = &co_mld_vap->iv_nawds;
                NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);
                if (mode == nawds->mode) {
                    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
                    continue;
                }

                mode_changed = true;
                nawds->mode = mode;
                ieee80211_mlnawds_node_leave_all(co_mld_vap);
                NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
           }

           if (!mode_changed)
               goto out;

           for (i = 0; i < num_links; i++) {
                co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
                if (!co_mld_vap)
                    continue;

                nawds = &co_mld_vap->iv_nawds;
                wlan_nawds_beacon_control(co_mld_vap, nawds->mode, mode);
           }

           break;
       case IEEE80211_NAWDS_PARAM_OVERRIDE:
           for (i = 0; i < num_links; i++) {
                co_mld_vap = wlan_vdev_get_mlme_ext_obj(vdev_list[i]);
                if (!co_mld_vap)
                    continue;

                nawds = &co_mld_vap->iv_nawds;
                NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);
                nawds->override = mlnawds->override;
                NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
           }
           break;
       default:
           ret = -EINVAL;
           goto out;
    }

out:
    if ((ieee80211_mlo_release_vdev_list_refs(vdev_list,
                QDF_ARRAY_SIZE(vdev_list), num_links, 0)) != QDF_STATUS_SUCCESS)
        qdf_err("Error while trying to release VDEV references - investigate");

    return ret;
}

int wlan_mlnawds_get_param(wlan_if_t vaphandle,
                           enum ieee80211_nawds_param param,
                           void *val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    nawds_rwlock_state_t(lock_state);

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        mlo_err("command initiated on non-mlo vap");
        return -EINVAL;
    }

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    switch(param) {
        case IEEE80211_MLNAWDS_PARAM_MODE:
            *((u_int8_t *) val) = nawds->mode;
            break;
        case IEEE80211_MLNAWDS_PARAM_OVERRIDE:
            *((u_int8_t *) val) = nawds->override;
            break;
        case IEEE80211_MLNAWDS_PARAM_NUM:
            *((u_int8_t *) val) = UMAC_MAX_NAWDS_REPEATER;
            break;
        default:
            NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
            return -EINVAL;
    }

    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);

    return 0;
}

int wlan_mlnawds_get_mac(wlan_if_t vaphandle,
                         int num,
                         struct ieee80211_wlanconfig_nawds *mlnawds)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_nawds *nawds = &vap->iv_nawds;
    nawds_rwlock_state_t(lock_state);

    if (!ieee80211_mlo_is_mld_ap(vap->vdev_obj)) {
        mlo_err("command initiated on non-mlo vap");
        return -EINVAL;
    }


    /* sanity check */
    if (num < 0 || num >= UMAC_MAX_NAWDS_REPEATER) {
        return -EINVAL;
    }

    NAWDS_WRITE_LOCK(&nawds->lock, &lock_state);

    OS_MEMCPY(mlnawds->peer_mld_mac,
              nawds->repeater[num].peer_mld_mac,
              QDF_MAC_ADDR_SIZE);
    OS_MEMCPY(mlnawds->peer_link_mac,
              nawds->repeater[num].peer_link_mac,
              QDF_MAC_ADDR_SIZE);
    mlnawds->caps = nawds->repeater[num].caps;
    mlnawds->puncture_bitmap = nawds->repeater[num].puncture_bitmap;
    mlnawds->link_id = nawds->repeater[num].link_id;

    NAWDS_WRITE_UNLOCK(&nawds->lock, &lock_state);
    return 0;
}
#endif /* UMAC_SUPPORT_MLNAWDS */
#ifdef IPA_OFFLOAD
int wlan_ipa_wds_remove_addr(wlan_if_t vaphandle, u_int8_t *macaddr, u_int8_t *peer_macaddr)
{
    struct ieee80211vap *vap = vaphandle;

    if (!vap || !macaddr || !peer_macaddr)
        return -EINVAL;

    if (qdf_is_macaddr_zero((struct qdf_mac_addr *)macaddr))
        return -EINVAL;

    /* Peer mac address will be passed as 00:00:00:00:00:00
     * in case, when the peer is directly associated with
     * the root AP, and in this case we are also not allowed,
     * to delete the AST entry, it will be done kickout.
     */
    if (qdf_is_macaddr_zero((struct qdf_mac_addr *)peer_macaddr))
        return -EFAULT;

    ieee80211_remove_ipa_wds_addr(vaphandle, macaddr, peer_macaddr);

    return 0;
}
#endif

#if ATH_SUPPORT_HYFI_ENHANCEMENTS
static int
is_hmwds_valid_mac(u_int8_t *addr)
{
    u_int8_t zero_mac[QDF_MAC_ADDR_SIZE] = {0x00,0x00,0x00,0x00,0x00,0x00};

    if (IEEE80211_ADDR_EQ(zero_mac, addr) ||
        IEEE80211_IS_IPV4_MULTICAST(addr) ||
        IEEE80211_IS_IPV6_MULTICAST(addr) ||
        IEEE80211_IS_BROADCAST(addr))
        return 0;

    return 1;
}

static inline int _wlan_hmwds_add_addr(wlan_if_t vaphandle, struct ieee80211_node_table *nt, u_int8_t *wds_ni_macaddr, u_int8_t *wds_macaddr)
{
    int retval = 0;
    struct ieee80211_node *wds_ni, *is_direct_ni;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_wds_addr *wds = NULL;

    if (!is_hmwds_valid_mac(wds_macaddr)) {
        qdf_err("Invalid hmwds mac addr %s\n",ether_sprintf(wds_macaddr));
        return -EINVAL;
    }

	is_direct_ni = ieee80211_find_node(vap->iv_ic, wds_macaddr, WLAN_WDS_ID);
	if(is_direct_ni)
	{
		qdf_nofl_info("Should not add a directly associated node as a wds  node \n");
		ieee80211_free_node(is_direct_ni, WLAN_WDS_ID);
		return retval;
	}

    if ((wds_ni = __ieee80211_find_wds_node(nt, wds_macaddr, WLAN_WDS_ID, &wds)) &&
            IEEE80211_ADDR_EQ(wds_ni->ni_macaddr, wds_ni_macaddr)) {
        wds->flags |= IEEE80211_NODE_F_WDS_HM;
        nt->nt_ic->ic_node_add_wds_entry((void *)(vaphandle->iv_ifp), wds_macaddr, wds_ni_macaddr, IEEE80211_NODE_F_WDS_HM);
        ieee80211_free_node(wds_ni, WLAN_WDS_ID);
    } else {
        if (wds) {
            wds->flags &= ~IEEE80211_NODE_F_WDS_HM;
            if (wds_ni) {
                /* Remove the WDS addres if it was found behind some other node
                 * to avoid duplicate entries.
                 */
                ieee80211_remove_wds_addr(vaphandle, nt, wds->wds_macaddr, IEEE80211_NODE_F_WDS_REMOTE);
                wds = NULL;
                /* Free the reference from earlier find above
                 */
                ieee80211_free_node(wds_ni, WLAN_WDS_ID);
            }
        }

        if (!(wds_ni = ieee80211_find_node(vap->iv_ic, wds_ni_macaddr, WLAN_WDS_ID)))
            return -EINVAL;

        retval = _ieee80211_add_wds_addr(vaphandle, wds_ni, wds_macaddr,
                IEEE80211_NODE_F_WDS_REMOTE | IEEE80211_NODE_F_WDS_HM);
        ieee80211_free_node(wds_ni, WLAN_WDS_ID);
    }

    return retval;
}

int wlan_hmwds_add_addr(wlan_if_t vaphandle, u_int8_t *wds_ni_macaddr, u_int8_t *wds_macaddr, u_int16_t wds_macaddr_cnt)
{
    int i, retval;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node_table *nt = NULL;
    struct ieee80211_node *ni;

    if (!vap || !wds_ni_macaddr || !wds_macaddr || !wds_macaddr_cnt)
        return -EINVAL;

    nt = &vap->iv_ic->ic_sta;

    ni = ieee80211_find_node(vap->iv_ic, wds_ni_macaddr, WLAN_WDS_ID);
    if (ni) {
        retval = nt->nt_ic->ic_node_use_4addr(ni);
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_WDS, "%s: use_4addr peer %02x:%02x:%02x"
                          ":%02x:%02x:%02x, returns %d\n", __func__, ni->ni_macaddr[0], ni->ni_macaddr[1],
                          ni->ni_macaddr[2], ni->ni_macaddr[3], ni->ni_macaddr[4], ni->ni_macaddr[5], retval);
        ieee80211_free_node(ni, WLAN_WDS_ID);
    }

    retval = 0;
    for (i = 0; i < wds_macaddr_cnt; i++) {
        ieee80211_remove_wds_addr(vaphandle, nt, wds_macaddr + i * QDF_MAC_ADDR_SIZE,
                                  IEEE80211_NODE_F_WDS_REMOTE | IEEE80211_NODE_F_WDS_BEHIND | IEEE80211_NODE_F_WDS_HM);
        retval |= _wlan_hmwds_add_addr(vap, nt, wds_ni_macaddr, wds_macaddr + i * QDF_MAC_ADDR_SIZE);
    }

    return retval;
}

int wlan_hmwds_remove_addr(wlan_if_t vaphandle, u_int8_t *macaddr)
{
    struct ieee80211vap *vap = vaphandle;
    rwlock_state_t lock_state;
    struct ieee80211_node_table *nt = NULL;

    if (!vap || !macaddr)
        return -EINVAL;
    nt = &vap->iv_ic->ic_sta;

    ieee80211_remove_wds_addr(vaphandle, nt, macaddr, IEEE80211_NODE_F_WDS_REMOTE | IEEE80211_NODE_F_WDS_BEHIND | IEEE80211_NODE_F_WDS_HM);

    return 0;
}

int wlan_hmwds_reset_addr(wlan_if_t vaphandle, u_int8_t *macaddr)
{
    unsigned int hash;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node *wds_ni;
    struct ieee80211_wds_addr *wds = NULL;
    rwlock_state_t lock_state;
    struct ieee80211_node_table *nt = NULL;
    int retval = 0;
    struct ieee80211_wds_addr *wds_next = NULL;

    if (!vap || !macaddr)
        return -EINVAL;

    nt = &vap->iv_ic->ic_sta;

    if ((wds_ni = ieee80211_find_node(vap->iv_ic, macaddr, WLAN_WDS_ID))) {
        OS_RWLOCK_WRITE_LOCK(&nt->nt_wds_nodelock, &lock_state);
        for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++) {
            for (wds = LIST_FIRST(&nt->nt_wds_hash[hash]); wds;) {
                wds_next = LIST_NEXT(wds, wds_hash);
                if (wds->flags & IEEE80211_NODE_F_WDS_HM &&
                        wds->wds_ni == wds_ni) {
                    ieee80211_free_node(wds->wds_ni, WLAN_WDS_ID);  /* Decrement ref count */
                    LIST_REMOVE(wds, wds_hash);
                    IEEE80211_DPRINTF_IC_CATEGORY(vap->iv_ic, IEEE80211_MSG_WDS,  "%s: deleting mac(%s)"
                            " from host wds table wds_flags:0x%x \n", __func__,
                            ether_sprintf(wds->wds_macaddr), wds->flags);
                    OS_FREE(wds);
                }
                wds = wds_next;
            }
	    }
        wds = NULL;
        OS_RWLOCK_WRITE_UNLOCK(&nt->nt_wds_nodelock, &lock_state);
        retval = vap->iv_ic->ic_node_update_wds_entry((void *)(vap->iv_ifp), NULL, wds_ni->ni_macaddr, 0);
    } else {
        wds_ni = _ieee80211_find_wds_node(nt, macaddr, WLAN_WDS_ID, &wds);
    }

    if (wds_ni) {
        ieee80211_free_node(wds_ni, WLAN_WDS_ID);
    }

    if (wds && wds->flags & IEEE80211_NODE_F_WDS_HM) {
        ieee80211_free_node(wds->wds_ni, WLAN_WDS_ID);  /* Decrement ref count */
        LIST_REMOVE(wds, wds_hash);
        IEEE80211_DPRINTF_IC_CATEGORY(vap->iv_ic, IEEE80211_MSG_WDS,  "%s: deleting mac(%s)"
                " from host wds table wds_flags:0x%x \n", __func__,
                ether_sprintf(wds->wds_macaddr), wds->flags);
        OS_FREE(wds);
        retval = vap->iv_ic->ic_node_update_wds_entry((void *)(vap->iv_ifp), macaddr, NULL, 0);
    }

    return retval;
}

int wlan_hmwds_reset_table(wlan_if_t vaphandle)
{
    unsigned int hash;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node_table *nt = NULL;
    struct ieee80211_wds_addr *wds;
    rwlock_state_t lock_state;
    struct ieee80211_wds_addr *wds_next;

    if (!vap)
        return -EINVAL;

    nt = &vap->iv_ic->ic_sta;

    OS_RWLOCK_WRITE_LOCK(&nt->nt_wds_nodelock, &lock_state);
    for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++) {
        for (wds = LIST_FIRST(&nt->nt_wds_hash[hash]); wds;) {
            wds_next = LIST_NEXT(wds, wds_hash);
            if (wds->flags & IEEE80211_NODE_F_WDS_HM) {
                ieee80211_free_node(wds->wds_ni, WLAN_WDS_ID);  /* Decrement ref count */
                LIST_REMOVE(wds, wds_hash);
                IEEE80211_DPRINTF_IC_CATEGORY(vap->iv_ic, IEEE80211_MSG_WDS,  "%s: deleting mac(%s)"
                        " from host wds table wds_flags:0x%x \n", __func__,
                        ether_sprintf(wds->wds_macaddr), wds->flags);
                OS_FREE(wds);
            }
            wds = wds_next;
        }
	}
    OS_RWLOCK_WRITE_UNLOCK(&nt->nt_wds_nodelock, &lock_state);

    vap->iv_ic->ic_node_update_wds_entry((void *)(vap->iv_ifp), NULL, NULL, 0);

    return 0;
}

int wlan_wds_dump_wds_addr(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = NULL;

    if (!vap)
        return -EINVAL;

    ic = vap->iv_ic;
    if(ic->ic_node_dump_wds_table)
    {
        ic->ic_node_dump_wds_table(ic);
    }
    return 0;
}

int wlan_hmwds_read_addr(wlan_if_t vaphandle, u_int8_t *wds_ni_macaddr, u_int8_t *buf, u_int16_t *buflen)
{
    unsigned int hash;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node_table *nt = NULL;
    struct ieee80211_wds_addr *wds;
    struct ieee80211_node *wds_ni;
    rwlock_state_t lock_state;
    u_int32_t buf_max_len, entry_count = 0;

    if (!vap || !wds_ni_macaddr || !buf || !buflen || !(*buflen))
        return -EINVAL;

    nt = &vap->iv_ic->ic_sta;

    if (!(wds_ni = ieee80211_find_node(vap->iv_ic, wds_ni_macaddr, WLAN_WDS_ID)))
        return -EINVAL;

    buf_max_len = *buflen;
    *buflen = 0;

    OS_RWLOCK_READ_LOCK(&nt->nt_wds_nodelock, &lock_state);
    for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++) {
        for (wds = LIST_FIRST(&nt->nt_wds_hash[hash]);
                wds && (*buflen + QDF_MAC_ADDR_SIZE < buf_max_len);) {
            if (wds->flags & IEEE80211_NODE_F_WDS_HM &&
                        wds->wds_ni == wds_ni) {
                OS_MEMCPY(buf + *buflen, wds->wds_macaddr, QDF_MAC_ADDR_SIZE);
                *buflen += QDF_MAC_ADDR_SIZE;
                entry_count++;
            }
            wds = LIST_NEXT(wds, wds_hash);
        }
        if (*buflen + QDF_MAC_ADDR_SIZE >= buf_max_len)
            break;
	}
    OS_RWLOCK_READ_UNLOCK(&nt->nt_wds_nodelock, &lock_state);

    ieee80211_free_node(wds_ni, WLAN_WDS_ID);
    *buflen = entry_count;

    return 0;
}

int wlan_wds_read_table(wlan_if_t vaphandle, struct ieee80211_wlanconfig_wds_table *wds_table)
{
    unsigned int hash;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node_table *nt;
    struct ieee80211_wds_addr *wds;
    rwlock_state_t lock_state;
    u_int32_t buf_max_len, buflen = 0;

    if (!vap || !wds_table || !wds_table->wds_entry_cnt)
        return -EINVAL;

    nt = &vap->iv_ic->ic_sta;
    buf_max_len = wds_table->wds_entry_cnt;
    wds_table->wds_entry_cnt = 0;

    OS_RWLOCK_READ_LOCK(&nt->nt_wds_nodelock, &lock_state);
    for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++) {
        for (wds = LIST_FIRST(&nt->nt_wds_hash[hash]);
                wds && (buflen + sizeof(struct ieee80211_wlanconfig_wds) < buf_max_len);) {
            struct ieee80211_wlanconfig_wds *wlanconfig_wds =
                &wds_table->wds_entries[wds_table->wds_entry_cnt];
            OS_MEMCPY(&wlanconfig_wds->destmac, wds->wds_macaddr, QDF_MAC_ADDR_SIZE);
            if (wds->wds_ni) {
                OS_MEMCPY(&wlanconfig_wds->peermac, wds->wds_ni->ni_macaddr,
                          QDF_MAC_ADDR_SIZE);
            } else {
                OS_MEMCPY(&wlanconfig_wds->peermac, wds->wds_ni_macaddr,
                          QDF_MAC_ADDR_SIZE);
            }
            wlanconfig_wds->flags = wds->flags;

            buflen += sizeof(struct ieee80211_wlanconfig_wds);
            wds_table->wds_entry_cnt++;

            wds = LIST_NEXT(wds, wds_hash);
        }
        if (buflen + sizeof(struct ieee80211_wlanconfig_wds) >= buf_max_len)
            break;
	}
    OS_RWLOCK_READ_UNLOCK(&nt->nt_wds_nodelock, &lock_state);

    return 0;
}

int wlan_hmwds_set_bridge_mac_addr(wlan_if_t vaphandle, u_int8_t *bridgemacaddr)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = NULL;

    if (!vap || !bridgemacaddr)
        return -EINVAL;

    ic = vap->iv_ic;
    if(ic->ic_node_set_bridge_mac_addr)
    {
        ic->ic_node_set_bridge_mac_addr(ic, bridgemacaddr);
    }

    return 0;
}

#endif /* ATH_SUPPORT_HYFI_ENHANCEMENTS */

