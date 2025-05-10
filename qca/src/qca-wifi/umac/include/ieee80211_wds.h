/*
 * Copyright (c) 2011,2017 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
 * All Rights Reserved.
 */

#ifndef IEEE80211_WDS_H
#define IEEE80211_WDS_H

#include <if_upperproto.h>
#include <ieee80211_var.h>
#include <ieee80211_node.h>

#if (MESH_MODE_SUPPORT||ATH_SUPPORT_NAC)

#define MESH_CAPS_VER1 0x8000
#define MESH_CAPS_BW_OFFSET 0
#define MESH_CAPS_NSS_OFFSET 4
#define MESH_CAPS_MODE_OFFSET 8
#define MESH_CAPS_NIBBLE_MASK 0xF
#define MESH_CAPS_MAX_NSS 8
#define MESH_CAPS_SHORT_SLOT 0x1000
#define MESH_CAPS_SHORT_PREAMBLE 0x2000
typedef enum {
    MESH_PREAMBLE_OFDM,
    MESH_PREAMBLE_CCK,
    MESH_PREAMBLE_HT ,
    MESH_PREAMBLE_VHT,
    MESH_PREAMBLE_HE,
#ifdef WLAN_FEATURE_11BE
    MESH_PREAMBLE_EHT,
#endif /* WLAN_FEATURE_11BE */
    MESH_PREAMBLE_MAX,
} localpeer_preamble_type_t;

typedef enum {
    MESH_BW_20,
    MESH_BW_40,
    MESH_BW_80,
    MESH_BW_80_80,
    MESH_BW_160,
#ifdef WLAN_FEATURE_11BE
    MESH_BW_320,
#endif /* WLAN_FEATURE_11BE */
    MESH_BW_MAX,
} localpeer_bw_type_t;

#endif

void
ieee80211_wds_attach(struct ieee80211_node_table *nt);
void
ieee80211_wds_detach(struct ieee80211_node_table *nt);

/* Add wds address to the node table */
int
ieee80211_add_wds_addr(wlan_if_t vaphandle,
                       struct ieee80211_node_table *nt,
		       struct ieee80211_node *ni, const u_int8_t *macaddr,
		       u_int32_t flags);

/* remove wds address from the wds hash table */
void
ieee80211_remove_wds_addr(wlan_if_t vaphandle,
                          struct ieee80211_node_table *nt,
			  const u_int8_t *macaddr,u_int32_t flags);
#ifdef IPA_OFFLOAD
/* remove wds address from the ast table */
void
ieee80211_remove_ipa_wds_addr(wlan_if_t vaphandle, const u_int8_t *macaddr,
                              const u_int8_t *peer_macaddr);
#endif
/* Remove node references from wds table */
void
ieee80211_del_wds_node(struct ieee80211_node_table *nt,
                      struct ieee80211_node *ni);

/* Remove all the wds entries associated with the AP when the AP to
 * which STA is associated goes down
 */
int ieee80211_node_removeall_wds (struct ieee80211_node_table *nt,struct ieee80211_node *ni);

struct ieee80211_node *
ieee80211_find_wds_node(struct ieee80211_node_table *nt, const u_int8_t *macaddr, wlan_objmgr_ref_dbgid id);
void wds_clear_wds_table(struct ieee80211_node * ni, struct ieee80211_node_table *nt, wbuf_t wbuf );

#if defined(UMAC_SUPPORT_MLNAWDS)
void wlan_mlo_nawds_config(wlan_if_t vaphandle,
                           struct ieee80211_node *ni,
                           struct wlan_mlo_peer_context *ml_peer);
void wlan_mlnawds_assoc_peer(struct ieee80211vap *vap,
                             struct ieee80211_node *ni,
                             u_int8_t *peer_link_mac);
void wlan_mlo_mlnawds_clear_nawds_entry(struct ieee80211vap *vap,
                                        uint8_t *peer_link_mac,
                                        uint8_t *peer_mld_mac);
#elif defined(WLAN_FEATURE_11BE_MLO)
static INLINE
void wlan_mlo_nawds_config(wlan_if_t vaphandle,
                           struct ieee80211_node *ni,
                           struct wlan_mlo_peer_context *ml_peer)
{
}

static INLINE
void wlan_mlnawds_assoc_peer(struct ieee80211vap *vap,
                             struct ieee80211_node *ni,
                             u_int8_t *peer_link_mac)
{
}

static INLINE
void wlan_mlo_mlnawds_clear_nawds_entry(struct ieee80211vap *vap,
                                        uint8_t *peer_link_mac,
                                        uint8_t *peer_mld_mac)
{
}
#endif

#if MESH_MODE_SUPPORT && defined(WLAN_FEATURE_11BE_MLO)
void wlan_mlmesh_assoc_peer(struct ieee80211vap *vap,
                            struct ieee80211_node *ni,
                            u_int8_t *peer_link_mac);

void wlan_mlo_mesh_config(wlan_if_t vaphandle,
                          struct ieee80211_node *ni,
                          struct wlan_mlo_peer_context *ml_peer);
#elif defined(WLAN_FEATURE_11BE_MLO)
static INLINE
void wlan_mlmesh_assoc_peer(struct ieee80211vap *vap,
                            struct ieee80211_node *ni,
                            u_int8_t *peer_link_mac)
{
}

static INLINE
void wlan_mlo_mesh_config(wlan_if_t vaphandle,
                          struct ieee80211_node *ni,
                          struct wlan_mlo_peer_context *ml_peer)
{
}
#endif

#if UMAC_SUPPORT_NAWDS
void ieee80211_nawds_attach(struct ieee80211vap *vap);
int ieee80211_nawds_send_wbuf(struct ieee80211vap *vap, wbuf_t wbuf);
int ieee80211_nawds_disable_beacon(struct ieee80211vap *vap);
int ieee80211_nawds_enable_learning(struct ieee80211vap *vap);
void ieee80211_nawds_learn(struct ieee80211vap *vap, u_int8_t *mac);
void ieee80211_nawds_learn_defer(struct ieee80211vap *vap, u_int8_t *mac);
void ieee80211_nawds_learn_deferwork(void *arg);
#define IEEE80211_NAWDS_LEARN(_vap, _mac) ieee80211_nawds_learn_defer(_vap, _mac)

#else
static INLINE void ieee80211_nawds_attach(struct ieee80211vap *vap) 
{
    /* do nothing */
};
static INLINE int ieee80211_nawds_send_wbuf(struct ieee80211vap *vap, wbuf_t wbuf)
{
    /* do nothing */
    return 0;
}

static INLINE int ieee80211_nawds_disable_beacon(struct ieee80211vap *vap)
{
    /* do nothing */
    return 0;
}

static INLINE int ieee80211_nawds_enable_learning(struct ieee80211vap *vap)
{
    /* do nothing */
    return 0;
}

static INLINE void ieee80211_nawds_learn(struct ieee80211vap *vap, u_int8_t *mac)
{
    /* do nothing */
}

#endif

#endif //IEEE80211_WDS_H
