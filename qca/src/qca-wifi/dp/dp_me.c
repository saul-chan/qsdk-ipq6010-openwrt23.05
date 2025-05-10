/*
 *
 * Copyright (c) 2017-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2009 Atheros Communications Inc.
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

/*
 * This file contains the main implementation of the multicast
 * enhancement functionality.
 *
 * The main purpose of this module is to convert (by translating or
 * tunneling) the multicast stream into duplicated unicast streams for
 * performance enhancement of home wireless applications. For more
 * details, please refer to the design documentation.
 *
 */
#include <qdf_nbuf.h>
#include <qdf_module.h>
#include <qdf_mem.h>
#include <cdp_txrx_cmn_struct.h>
#include <cdp_txrx_ctrl.h>
#include <htt_common.h>
#if ATH_SUPPORT_IQUE
#include "osdep.h"
#include "osif_private.h"
#include "wbuf.h"
#include "dp_me.h"
#include "dp_txrx.h"
#include "cdp_txrx_me.h"
#include "cdp_txrx_host_stats.h"

#ifdef CONFIG_SAWF
#include <wlan_sawf.h>
#include "cdp_txrx_sawf.h"
#endif /*CONFIG_SAWF*/

#define IGMP_QUERY      0x11
#define IGMPv1_REPORT   0x12
#define IGMPv2_REPORT   0x16
#define IGMP_LEAVE      0x17
#define IGMPv3_REPORT   0x22
#define MLD_QUERY       0x82
#define MLD_REPORT      0x83
#define MLD_LEAVE       0x84
#define MLDv2_REPORT    0x8f
#define MAX_32BIT_VAL   0xFFFFFFFF

#define DP_SAWF_META_DATA_INVALID 0xffffffff

#ifdef CONFIG_SAWF
#define DP_SAWF_DEFAULT_Q_INVALID 0xFF
#define DP_ME_DOWNLINK 0x1
#define SAWF_FLOW_START 1
#define SAWF_FLOW_STOP  2
#define IPV4_VERSION 4
#define DP_ME_WDS_EXT 1

#define HASH_EXISTS_IN_NODE_HT(name, member, dmac, exists, key) \
	do { \
		dp_me_sawf_node_t *iter_ref; \
		exists = false; \
		qdf_ht_for_each_in_bucket(name, iter_ref, member, key) { \
			if (!qdf_mem_cmp(&iter_ref->mac, dmac, \
					 QDF_MAC_ADDR_SIZE)) { \
				exists = true; \
				break; \
			} \
		} \
	} while(0)

#define HASH_ADD_NODE_IN_NODE_HT(hash_map, mac, ref, is_wds_ext, key) \
	do { \
		dp_me_sawf_node_t *new_node = \
				dp_me_allocate_hash_node(mac, is_wds_ext); \
		if (new_node) \
			qdf_ht_add(hash_map, &new_node->node_hash_ptr, key); \
		ref = new_node; \
	} while(0)

#define HASH_ADD_ALL_ENTRIES_IN_NODE_HT(entry, hash_map, is_wds_ext) \
	do { \
		uint8_t mac[QDF_MAC_ADDR_SIZE]; \
		int node_iter = 0;\
		int node_cnt = entry->node_cnt; \
		bool exists = false; \
		bool is_vlan_dev = false; \
		dp_me_sawf_node_t *ref = NULL; \
		uint8_t key; \
		for (node_iter = 0;node_iter < node_cnt; node_iter++) { \
			dp_me_get_interface_mac(mac, &entry->nodes[node_iter], \
						&is_vlan_dev); \
			if (is_vlan_dev) \
				continue;\
			key = dp_me_get_mac_hash(mac); \
			HASH_EXISTS_IN_NODE_HT(hash_map, node_hash_ptr, \
					       mac, exists, key); \
			if (!exists) { \
				HASH_ADD_NODE_IN_NODE_HT(hash_map, mac, ref, \
							 is_wds_ext, key); \
			} \
		} \
	} while(0)
#endif /* CONFIG_SAWF*/

static int dp_me_igmp_mld_inspect(struct cdp_soc_t *soc, uint8_t pdev_id,
				  qdf_nbuf_t wbuf, u_int8_t *type,
				  void *group, u_int8_t *hmmc_found, u_int8_t *deny_list);

static void dp_me_get_snoop_table(dp_vdev_me_t *vdev_me, uint32_t flag,
			   dp_me_snoop_lock_t **mcast_lock,
			   struct dp_me_snoop_table **snoop_table)
{
#ifdef WLAN_FEATURE_11BE_MLO
	if (flag & DP_ME_MCTBL_MLD_VDEV) {
		*snoop_table = &(vdev_me->mld_me_snoop_table);
		*mcast_lock  = &(vdev_me->mld_me_snoop_table.lock);
	} else
#endif
	{
		(void)flag;
		*snoop_table = &(vdev_me->me_snoop_table);
		*mcast_lock  = &(vdev_me->me_snoop_table.lock);
	}
}

void dp_pdev_me_setup(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	dp_txrx_pdev_handle_t *dp_hdl;
	dp_pdev_me_t *pdev_me_hdl;

	dp_hdl = cdp_pdev_get_dp_txrx_handle(soc, pdev_id);

	if (!dp_hdl)
		return;

	pdev_me_hdl = &dp_hdl->pdev_me_hdl;

	pdev_me_hdl->pdev_deny_list_cnt= 3;
	pdev_me_hdl->pdev_denylist[0].ip = be32toh(0xeffffffa); /* 239.255.255.250 */
	pdev_me_hdl->pdev_denylist[0].mask = 0xffffffff;
	pdev_me_hdl->pdev_denylist[1].ip = be32toh(0xe00000fb); /* 224.0.0.251 */
	pdev_me_hdl->pdev_denylist[1].mask = 0xffffffff;
	pdev_me_hdl->pdev_denylist[2].ip = be32toh(0xe00000fc); /* 224.0.0.252 */
	pdev_me_hdl->pdev_denylist[2].mask = 0xffffffff;

	pdev_me_hdl->pdev_hmmc_cnt = 0;
	pdev_me_hdl->pdev_deny_list_v6_cnt = 0;
	pdev_me_hdl->pdev_hmmc_v6_cnt = 0;
	pdev_me_hdl->glob_offset = 0;
	pdev_me_hdl->glob_cur_vdev = NULL;

}
qdf_export_symbol(dp_pdev_me_setup);

/*
 * dp_me_find_next_hop_mac:
 * Find the receiver's address (RA) for a multicast group member given
 * it's destination address (DA) from the MCS snoop table.
 *
 * Parameters:
 * @soc: soc txrx handle
 * @vdev_me: vdev ME handle
 * @pdev_id: pdev id
 *
 * Returns:
 * Nothing
 */
void dp_me_find_next_hop_mac(struct cdp_soc_t *soc, dp_vdev_me_t *vdev_me,
			     uint8_t pdev_id, uint32_t flag)
{
	struct cdp_ast_entry_info ast_entry_info;
	uint32_t node_idx, nested_node_idx, node_cnt, i;
	struct dp_me_mcast_table *table;
	struct dp_me_ra_entry (*me_ra)[DP_ME_MCAST_NODE_MAX];

	if(!vdev_me) {
		return;
	}

#ifdef WLAN_FEATURE_11BE_MLO
	if (flag & DP_ME_MCTBL_MLD_VDEV) {
		table = &(vdev_me->mld_me_snoop_table.mcast_table);
		me_ra = &(vdev_me->mld_me_snoop_table.me_ra[0]);
	} else
#endif
	{
		(void)flag;
		table = &(vdev_me->me_snoop_table.mcast_table);
		me_ra = &(vdev_me->me_snoop_table.me_ra[0]);
	}


	/* Resolving the RA, if there is any failure, the destination
	 * MAC address will be used. This is done for each node of each
	 * group address
	 */

	for(i = 0; i < DP_ME_MCAST_MAX_HASH; i++) {
		struct dp_me_mcast_hash_entry *hash_entry = NULL;
		TAILQ_FOREACH(hash_entry, &table->hash_entry[i], me_mcast_entry_list_elem) {
			if(!hash_entry) {
				continue;
			}
			node_cnt = hash_entry->entry.node_cnt;
			for (node_idx = 0; node_idx < node_cnt; node_idx++) {
				me_ra[hash_entry->group_id][node_idx].dup = 0;
				if (cdp_peer_get_ast_info_by_pdev(soc, hash_entry->entry.nodes[node_idx].mac,
					pdev_id, &ast_entry_info)) {
					WLAN_ADDR_COPY(me_ra[hash_entry->group_id][node_idx].mac,
                                   ast_entry_info.peer_mac_addr);
	            } else {
					/* If AST entry is not found, then the destination address is
                	                 * copied to the receiver's address table
					 */
                    WLAN_ADDR_COPY(me_ra[hash_entry->group_id][node_idx].mac,
						hash_entry->entry.nodes[node_idx].mac);
				}

				/* If there are members with the same next-hop, subsequent members
				 * are ignored
				 */
				for (nested_node_idx = 0; nested_node_idx < node_idx; nested_node_idx++) {
					if((WLAN_ADDR_EQ(me_ra[hash_entry->group_id][node_idx].mac,
						me_ra[hash_entry->group_id][nested_node_idx].mac) == 0)) {
						me_ra[hash_entry->group_id][node_idx].dup = 1;
						break;
					}
				}

			}
		}
	}
}

/*
 * dp_me_mcast_get_ipv4_hash_idx
 * Get hash index based on ip address for ipv4
 */
uint32_t dp_me_mcast_get_ipv4_hash_idx(u_int8_t *ip)
{
	uint8_t hash_idx;

	if(!ip)
		return DP_ME_INVALID_HASH;
	hash_idx = ip[0]^ip[1]^ip[2]^ip[3];
	return hash_idx;
}

/*
 * dp_me_mcast_get_ipv6_hash_idx
 * Get hash index based on ip address for ipv6
 */
uint32_t dp_me_mcast_get_ipv6_hash_idx(u_int8_t *ip)
{
	uint8_t hash_idx;
	uint32_t ipv6_hash;

	if(!ip)
		return DP_ME_INVALID_HASH;

	ipv6_hash = ip[0]^ip[1]^ip[2]^ip[3];
	ipv6_hash = (uint16_t)((ipv6_hash >> 16)^ipv6_hash);
	hash_idx = (uint8_t)((ipv6_hash >> 8)^ipv6_hash);

	return hash_idx;
}

/*
 * dp_me_flush_table
 * Flush the MCAST table
 */
void dp_me_flush_table(struct dp_me_mcast_table *table)
{
	struct dp_me_mcast_hash_entry *hash_entry = NULL;
	struct dp_me_mcast_hash_entry *tmp_hash_entry = NULL;
	uint32_t i;
	if(!table)
		return;
	if(table->entry_cnt == 0)
		return;

	for(i = 0; i < DP_ME_MCAST_MAX_HASH; i++) {
                TAILQ_FOREACH_SAFE(hash_entry, &table->hash_entry[i], me_mcast_entry_list_elem, tmp_hash_entry) {
			if (!hash_entry) {
				continue;
			}

			TAILQ_REMOVE (&table->hash_entry[i], hash_entry, me_mcast_entry_list_elem);
			qdf_mem_free (hash_entry);
			table->entry_cnt--;
		}
	}

	return;
}

/*
 * dp_me_mcast_add_entry
 * Add entry in the MCAST table
 */
void dp_me_mcast_add_entry(struct dp_me_mcast_table *table,
				struct dp_me_mcast_entry *grp_entry)
{
	struct dp_me_mcast_hash_entry *hash_entry = NULL;
	uint32_t hash_idx, protocol;

	if(!table || !grp_entry)
		return;
	if(table->entry_cnt == DP_ME_MCAST_GROUP_MAX) {
		dp_me_info("%pK: Table Full", table);
		return;
	}

	hash_entry = (struct dp_me_mcast_hash_entry *)qdf_mem_malloc(sizeof(struct dp_me_mcast_hash_entry));
	if (!hash_entry) {
		dp_me_info("Hash entry alloc failed");
		return;
	}
	protocol = grp_entry->group.protocol;
	if(protocol == htons(ETHERTYPE_IP))
		hash_idx = dp_me_mcast_get_ipv4_hash_idx((u_int8_t *)&grp_entry->group.u.ip4);
	else
		hash_idx = dp_me_mcast_get_ipv6_hash_idx(grp_entry->group.u.ip6);

	if(hash_idx == DP_ME_INVALID_HASH) {
		qdf_mem_free (hash_entry);
		return;
	}

	qdf_mem_copy(&hash_entry->entry, grp_entry, sizeof(struct dp_me_mcast_entry));
	hash_entry->group_id = table->entry_cnt + 1;
	TAILQ_INSERT_TAIL(&table->hash_entry[hash_idx], hash_entry,  me_mcast_entry_list_elem);
	table->entry_cnt = table->entry_cnt + 1;
}

static struct dp_me_mcast_entry *dp_me_mcast_find_entry(
		struct dp_me_mcast_group *group,
		int *group_ix,
		struct dp_me_mcast_table *table)
{
	int cnt;
	bool found = false;
	struct dp_me_mcast_hash_entry *hash_entry = NULL;
	if (!group || !table)
		return NULL;

	for (cnt = 0; cnt < DP_ME_MCAST_MAX_HASH; cnt++) {
		TAILQ_FOREACH(hash_entry, &table->hash_entry[cnt], me_mcast_entry_list_elem) {
			if(!hash_entry)
				continue;

			if (!qdf_mem_cmp(&hash_entry->entry.group, group, sizeof(*group))) {
				found = true;
				goto return_entry;
			}
		}
	}

return_entry:
	if(found) {
		*group_ix = hash_entry->group_id;
		return &hash_entry->entry;
	}

	return NULL;
}

#ifdef CONFIG_SAWF
static uint8_t dp_me_get_mac_hash(uint8_t *mac)
{
	uint8_t hash = 0;
	uint8_t i;
	for(i = 0; i < QDF_MAC_ADDR_SIZE; i++) {
		hash ^= mac[i];
	}
	return hash;
}

static
dp_me_sawf_node_t *find_node_from_mac_key(uint8_t *mac, uint8_t key,
					  dp_me_sawf_group_state_t *grp_iter)
{
	dp_me_sawf_node_t *iter_ref;

	qdf_ht_for_each_in_bucket(grp_iter->sawf_me_node_state,
			          iter_ref, node_hash_ptr, key) {
		if (!qdf_mem_cmp(&(iter_ref->mac), mac, QDF_MAC_ADDR_SIZE)) {
			return iter_ref;
		}
	}

	return NULL;
}

static
dp_me_sawf_node_t *find_node_from_mac(uint8_t *mac,
				      dp_me_sawf_group_state_t *grp_iter)
{
	uint8_t key = dp_me_get_mac_hash(mac);

	return find_node_from_mac_key(mac, key, grp_iter);

}

static inline bool
dp_me_find_node_in_snoop_tbl(struct dp_me_mcast_entry *entry, uint8_t *mac)
{
	int iter;

	for (iter = 0; iter < entry->node_cnt; iter++) {
		if (!qdf_mem_cmp(entry->nodes[iter].mac, mac,
				 QDF_MAC_ADDR_SIZE)) {
			return true;
		}
	}
	return false;
}

dp_me_sawf_node_t *dp_me_allocate_hash_node(uint8_t *mac, uint8_t is_wds_ext)
{
	int idx;
	dp_me_sawf_node_t *new_node =
		(dp_me_sawf_node_t *)qdf_mem_malloc(sizeof(dp_me_sawf_node_t));

	if (!new_node) {
		dp_me_info("%s memory allocation failure",__func__);
		return NULL;
	}
	qdf_mem_set(new_node, sizeof(dp_me_sawf_node_t), 0);
	new_node->is_wds_ext = is_wds_ext;
	for(idx = 0; idx < DP_SAWF_ME_SCV_ID_MAX; idx++)
		new_node->state[idx].sawf_mark.metadata =
						DP_SAWF_META_DATA_INVALID;
	qdf_mem_copy(&(new_node->mac), mac, QDF_MAC_ADDR_SIZE);
	return new_node;
}

static dp_me_sawf_group_state_t *dp_me_update_grp_list(
				struct dp_me_snoop_table *me_snoop_table,
				struct dp_me_mcast_entry *entry,
				uint8_t is_wds_ext)
{
	uint32_t key;
	dp_me_sawf_group_state_t *iter_ref;
	bool exists = false;
	struct dp_me_mcast_group  *group = &(entry->group);
	dp_me_sawf_group_state_t *new_grp = NULL;

	if(group->protocol == htons(ETHERTYPE_IP))
		key = dp_me_mcast_get_ipv4_hash_idx((u_int8_t *)&group->u.ip4);
	else
		key = dp_me_mcast_get_ipv6_hash_idx(group->u.ip6);

	if(key == DP_ME_INVALID_HASH) {
		return NULL;
	}
	qdf_ht_for_each_in_bucket(me_snoop_table->sawf_me_group_state,
			          iter_ref, group_hash_ptr, key) {
		if (!qdf_mem_cmp(&(iter_ref->group), group,
				 sizeof(struct dp_me_mcast_group))) {
			exists = true;
			break;
		}
	}
	if (!exists) {
		new_grp = (dp_me_sawf_group_state_t *)
			  qdf_mem_malloc(sizeof(dp_me_sawf_group_state_t));
		if (!new_grp) {
			dp_me_info("Hash ME group entry memory alloc failed");
			return new_grp;
		}
		qdf_mem_set(new_grp,
			    sizeof(dp_me_sawf_group_state_t), 0);
		qdf_mem_copy(&new_grp->group, group,
			     sizeof(struct dp_me_mcast_group));
		hash_init(new_grp->sawf_me_node_state);
		HASH_ADD_ALL_ENTRIES_IN_NODE_HT(entry,
						new_grp->sawf_me_node_state,
						is_wds_ext);
		new_grp->flow_count = 0;
		qdf_ht_add(me_snoop_table->sawf_me_group_state,
			   &new_grp->group_hash_ptr, key);
	}
	return new_grp;
}

static void
dp_me_update_client_ref_list(struct dp_me_snoop_table *me_snoop_table,
			     struct dp_me_mcast_entry *entry,
			     dp_vdev_me_t *vdev_me)
{
	if (!vdev_me->sawf_me_en)
		return;
	/* Add any new group in the snoop table to the list*/
	dp_me_update_grp_list(me_snoop_table, entry, !DP_ME_WDS_EXT);
}

#ifdef WLAN_FEATURE_11BE_MLO
static inline void
dp_me_init_mld_sawf_hashtables(dp_vdev_me_t *vdev_me)
{
	qdf_ht_init(vdev_me->mld_me_snoop_table.sawf_me_group_state);

}
#else
static inline void
dp_me_init_mld_sawf_hashtables(dp_vdev_me_t *vdev_me)
{
}
#endif


inline void
dp_me_init_sawf_hashtables(dp_vdev_me_t *vdev_me)
{
	qdf_ht_init(vdev_me->me_snoop_table.sawf_me_group_state);
	dp_me_init_mld_sawf_hashtables(vdev_me);
}

inline uint32_t dp_me_get_group_key(struct dp_me_mcast_group *group)
{
	uint32_t key = DP_ME_INVALID_HASH;

	if(group->protocol == htons(ETHERTYPE_IP))
		key = dp_me_mcast_get_ipv4_hash_idx((u_int8_t *)&group->u.ip4);
	else
		key = dp_me_mcast_get_ipv6_hash_idx(group->u.ip6);

	return key;
}

inline void dp_me_set_sawf_metadata(qdf_nbuf_t nbuf, uint32_t sawf_mark)
{
	qdf_nbuf_set_mark(nbuf, sawf_mark);
}

#ifdef QCA_SUPPORT_WDS_EXTENDED
void dp_me_get_interface_mac(u_int8_t *mac, struct dp_me_mcast_node *node,
			     bool *is_vlan)
{
	osif_peer_dev *osdev = NULL;
	struct net_device *nt_dev =
		(struct net_device *)qdf_net_if_get_dev_by_name(node->if_name);

	*is_vlan = false;
	qdf_mem_copy(mac, node->mac, QDF_MAC_ADDR_SIZE);
	if (nt_dev == NULL)
		return;

	if (qdf_net_if_is_vlan_dev((struct qdf_net_if *)nt_dev)) {
		qdf_net_if_release_dev((struct qdf_net_if *)nt_dev);
		*is_vlan = true;
		return;
	}
	osdev = ath_netdev_priv(nt_dev);
	if (osdev->dev_type == OSIF_NETDEV_TYPE_WDS_EXT) {
		qdf_mem_copy(mac, osdev->peer_mac_addr, QDF_MAC_ADDR_SIZE);
	}

	qdf_net_if_release_dev((struct qdf_net_if *)nt_dev);
}
#else /* QCA_SUPPORT_WDS_EXTENDED */
void dp_me_get_interface_mac(u_int8_t *mac, struct dp_me_mcast_node *node,
			     bool *is_vlan)
{
	struct net_device *nt_dev =
		(struct net_device *)qdf_net_if_get_dev_by_name(node->if_name);

	*is_vlan = false;
	qdf_mem_copy(mac, node->mac, QDF_MAC_ADDR_SIZE);

	if (!nt_dev)
		return;
	if (qdf_net_if_is_vlan_dev((struct qdf_net_if *)nt_dev))
		*is_vlan = true;
	qdf_net_if_release_dev((struct qdf_net_if *)nt_dev);
	return;
}

#endif /* QCA_SUPPORT_WDS_EXTENDED */

#else /* CONFIG_SAWF*/
static void
dp_me_update_client_ref_list(struct dp_me_snoop_table *me_snoop_table,
			     struct dp_me_mcast_entry *entry,
			     dp_vdev_me_t *vdev_me)
{
}

static void
dp_me_refresh_sawf_node_params(ol_txrx_soc_handle soc,
			       struct dp_me_snoop_table *me_snoop_table,
			       dp_vdev_me_t *vdev_me)
{
}


inline void dp_me_init_sawf_hashtables(dp_vdev_me_t *vdev_me)
{
}

inline void dp_me_set_sawf_metadata(qdf_nbuf_t nbuf, uint32_t sawf_mark)
{
}
#endif /* CONFIG_SAWF*/
qdf_export_symbol(dp_me_set_sawf_metadata);
/*
 * dp_me_update_mcast_table
 * Update the MCAST table
 */
void
dp_me_update_mcast_table(ol_txrx_soc_handle soc, uint8_t vdev_id,
			 uint8_t pdev_id, void *data, uint32_t msglen,
			 uint16_t hdrlen, uint32_t flag)
{
	dp_vdev_me_t *vdev_me;
	dp_pdev_me_t *pdev_me_hdl;
	struct dp_me_mcast_entry entry;
	uint32_t i, offset = 0;
	uint32_t entry_size = sizeof(struct dp_me_mcast_entry);
	uint32_t entry_cnt;
	struct dp_me_mcast_table *mcast_table;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return;

	vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

	if (!vdev_me)
		return;

#ifdef WLAN_FEATURE_11BE_MLO
	if (flag & DP_ME_MCTBL_MLD_VDEV)
		mcast_table = &(vdev_me->mld_me_snoop_table.mcast_table);
	else
#endif
	{
		(void)flag;
		mcast_table = &(vdev_me->me_snoop_table.mcast_table);
	}

	if (msglen <= hdrlen) {
		dp_me_info("%p: msglen is less than hdrlen, msglen: %d, hdrlen: %d", vdev_me, msglen, hdrlen);
		dp_me_flush_table(mcast_table);
	} else {
		uint32_t mcast_table_size = entry_size *  DP_ME_MCAST_GROUP_MAX;
		uint32_t postlen = pdev_me_hdl->glob_offset + msglen;
		bool range_check = (postlen > mcast_table_size + sizeof(uint32_t) ? 1 : 0);

		if ((flag & DP_ME_MCTBL_SEG_FIRST) || range_check) {
			qdf_mem_zero(pdev_me_hdl->glob_table, pdev_me_hdl->glob_offset);
			pdev_me_hdl->glob_offset = 0;
			if (range_check) {
				return;
			}
			pdev_me_hdl->glob_cur_vdev = vdev_me;
		}

		if(pdev_me_hdl->glob_cur_vdev != vdev_me) {
			dp_me_err("%p: Failed to update segment because of different vdev is being updated.", vdev_me);
			return;
		}

		qdf_mem_copy(pdev_me_hdl->glob_table+pdev_me_hdl->glob_offset, data, msglen);
		pdev_me_hdl->glob_offset = postlen;
		if (!(flag & DP_ME_MCTBL_SEG_LAST)) {
			return;
		}

		dp_me_flush_table(mcast_table);

		if (msglen - hdrlen < mcast_table_size)
			mcast_table_size = msglen - hdrlen;

		qdf_mem_copy(&entry_cnt, pdev_me_hdl->glob_table, sizeof(entry_cnt));
		offset = sizeof(entry_cnt);
		for (i = 0; i < entry_cnt; i++) {
			qdf_mem_copy(&entry, pdev_me_hdl->glob_table + offset, entry_size);
			dp_me_mcast_add_entry(mcast_table, &entry);
			offset = offset + entry_size;
		}

		/* Populating the RA table for each group*/
		dp_me_find_next_hop_mac(soc, vdev_me, pdev_id, flag);
		qdf_mem_set(pdev_me_hdl->glob_table, sizeof(struct dp_me_mcast_entry) * DP_ME_MCAST_GROUP_MAX, 0);
		pdev_me_hdl->glob_offset = 0;
	}
}

int dp_dms_amsdu(ol_txrx_soc_handle soc, uint8_t vdev_id, uint8_t *peer_addr,
                 struct cdp_tx_exception_metadata *tx_exc_param)
{
	cdp_config_param_type buf = {0};

	tx_exc_param->peer_id = cdp_get_peer_id(soc, vdev_id, peer_addr);
	if (tx_exc_param->peer_id == HTT_INVALID_PEER)
		return 1;

	cdp_txrx_get_vdev_param(soc, vdev_id, CDP_TX_ENCAP_TYPE, &buf);
	tx_exc_param->tx_encap_type = buf.cdp_vdev_param_tx_encap;

	memset((void *)&buf, 0x0, sizeof(cdp_config_param_type));
	cdp_txrx_get_vdev_param(soc, vdev_id, CDP_ENABLE_CIPHER, &buf);
	tx_exc_param->sec_type = buf.cdp_vdev_param_cipher_en;

	return 0;
}
qdf_export_symbol(dp_dms_amsdu);

#ifdef QCA_OL_DMS_WAR

static int dp_get_tid_from_iptos(qdf_nbuf_t msdu)
{
#define TID_MASK 0x7
	struct ether_header *eh;
	uint16_t ether_type;
	uint8_t tos = 0, dscp_tid_override = 0, tid = CDP_INVALID_TID;
	uint8_t eth_hdr_size = sizeof(qdf_ether_header_t);

	eh = (struct ether_header *) qdf_nbuf_data(msdu);
	ether_type = eh->ether_type;

	if (ether_type == htons(ETH_P_8021Q)) {
		struct vlan_ethhdr *veth =
				(struct vlan_ethhdr *)qdf_nbuf_data(msdu);
		ether_type = veth->h_vlan_encapsulated_proto;
		eth_hdr_size = sizeof(struct vlan_ethhdr);
	}

	/*
	 * Multicast enhancement can only be done for IPv4, IPv6 frames.
	 * Not adding cases for ARP, EAPOL because of this reason.
	 * Vlan check is not required either.
	 */
	if (ether_type == __constant_htons(ETHERTYPE_IP))
	{
		struct iphdr *ip = (struct iphdr *)(qdf_nbuf_data(msdu) +
				    eth_hdr_size);
		/*
		 * Save ip tos/tid/dscp
		 */
		tos = ip->tos;
		dscp_tid_override = 1;
	} else if (ether_type == htons(ETHERTYPE_IPV6)) {
		/*
		 * use flowlabel
		 */
		unsigned long ver_pri_flowlabel;
		unsigned long pri;
		ver_pri_flowlabel = *(unsigned long *)(qdf_nbuf_data(msdu) +
							eth_hdr_size);
		pri = (ntohl(ver_pri_flowlabel) & IPV6_PRIORITY_MASK) >> IPV6_PRIORITY_SHIFT;
		tos = pri;
		dscp_tid_override = 1;
	}

	if (dscp_tid_override) {
		tos = (tos & (~0x3)) >> IP_PRI_SHIFT;
		tid = (tos & TID_MASK);
	}

	return tid;
}

static int
transcap_8023_to_nwifi(struct cdp_soc_t *soc, qdf_nbuf_t msdu,
		       uint8_t *peer_addr, uint8_t *macaddr)
{
	struct ieee80211_frame *wh;
	struct ether_header eth_hdr, *eh;
	uint16_t typeorlen, length;
	struct llc *llcHdr;
	struct ethervlan_header *veth, veth_hdr;

	qdf_assert(msdu != NULL);

	if (qdf_nbuf_headroom(msdu) < sizeof(*wh) + sizeof(*llcHdr))
	{
		dp_me_info("%pK: DMS encap: Don't have enough headroom", soc);
		return 1;
	}

	eh = (struct ether_header *) qdf_nbuf_data(msdu);
	/*
	 * Save addresses to be inserted later
	 */
	WLAN_ADDR_COPY(eth_hdr.ether_dhost, eh->ether_dhost);
	WLAN_ADDR_COPY(eth_hdr.ether_shost, eh->ether_shost);
	typeorlen = eh->ether_type;

	if (typeorlen == htons(ETH_P_8021Q)) {
		veth = (struct ethervlan_header *) qdf_nbuf_data(msdu);
		WLAN_ADDR_COPY(veth_hdr.ether_dhost, veth->ether_dhost);
		WLAN_ADDR_COPY(veth_hdr.ether_shost, veth->ether_shost);
		typeorlen = veth->vlan_encapsulated_proto;
	}

	length = qdf_nbuf_len(msdu);

	/*
	 * Make room for nwifi header for DMS
	 */
	if (qdf_nbuf_push_head(msdu, sizeof(*wh) + sizeof(*llcHdr)) == NULL) {
		dp_me_info("%pK: Encap: Failed to push nwifi and LLC header for DMS"
			   , soc);
		return 1;
	}

	/* Set ftype to DMS - Will be used to perform DP operations later */
	qdf_nbuf_set_tx_fctx_type(msdu, (void *) 0, CB_FTYPE_DMS);
	wh = (struct ieee80211_frame *) qdf_nbuf_data(msdu);
	WLAN_ADDR_COPY(wh->i_addr1, peer_addr);
	WLAN_ADDR_COPY(wh->i_addr2, macaddr);
	WLAN_ADDR_COPY(wh->i_addr3, eth_hdr.ether_shost);
	wh->i_fc[0] = IEEE80211_FC0_SUBTYPE_QOS;
	wh->i_fc[1] = IEEE80211_FC1_DIR_FROMDS;
	wh->i_fc[0] |= (IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA);

	eh = (struct ether_header *)((uint8_t *)wh + sizeof(*wh));
	WLAN_ADDR_COPY(eh->ether_dhost, eth_hdr.ether_dhost);
	WLAN_ADDR_COPY(eh->ether_shost, eth_hdr.ether_shost);
	eh->ether_type = htons(length - sizeof(eth_hdr) + sizeof(*llcHdr));
	llcHdr = (struct llc *)((uint8_t *)eh + sizeof(eth_hdr));

	if (eh->ether_type == htons(ETH_P_8021Q)) {
		veth = (struct ethervlan_header *) ((uint8_t *)wh + sizeof(*wh));
		WLAN_ADDR_COPY(veth->ether_dhost, veth_hdr.ether_dhost);
		WLAN_ADDR_COPY(veth->ether_shost, veth_hdr.ether_shost);
		veth->vlan_encapsulated_proto =
						htons(length - sizeof(veth_hdr) + sizeof(*llcHdr));
		llcHdr = (struct llc *)((uint8_t *)veth + sizeof(veth_hdr));
	}

	llcHdr->llc_dsap                     = LLC_SNAP_LSAP;
	llcHdr->llc_ssap                     = LLC_SNAP_LSAP;
	llcHdr->llc_un.type_snap.control     = LLC_UI;
	llcHdr->llc_un.type_snap.org_code[0] = RFC1042_SNAP_ORGCODE_0;
	llcHdr->llc_un.type_snap.org_code[1] = RFC1042_SNAP_ORGCODE_1;
	llcHdr->llc_un.type_snap.org_code[2] = RFC1042_SNAP_ORGCODE_2;
	llcHdr->llc_un.type_snap.ether_type  = typeorlen;

	return 0;
}

int
dp_dms_amsdu_war(struct cdp_soc_t *soc, struct sk_buff **skb,
		 uint8_t *peer_addr,
		 struct cdp_tx_exception_metadata *tx_exc_param,
		 uint8_t *macaddr, uint8_t desired_tid)
{
	uint8_t tid;
	tid = dp_get_tid_from_iptos(*skb);
	if (transcap_8023_to_nwifi(soc, *skb, peer_addr, macaddr)) {
		return 1;
	}

	tx_exc_param->tid = desired_tid == HTT_INVALID_TID ? tid : desired_tid;
	tx_exc_param->peer_id = CDP_INVALID_PEER;
	tx_exc_param->tx_encap_type = htt_cmn_pkt_type_native_wifi;
	tx_exc_param->sec_type = cdp_sec_type_none;

	return 0;
}
qdf_export_symbol(dp_dms_amsdu_war);

/*
 * dp_me_convert_amsdu_ucast:
 * Encapsulates an 802.11 header on the 802.3 frame and sends the resulting
 * buffer to the firmware to enable the AMSDU aggregation.
 *
 * Parameters:
 * @soc: soc txrx handle
 * @vdev_id: Handle to vap pointer
 * @wbuf: Frame buffer containing the 802.3 frame
 * @mcast_grp_mac: List of MAC address of the members of the multicast group
 * @mcast_grp_mac_cnt: Total number of members in the multicast group
 * @macaddr: vdev MAC adderss
 * @tid: desired tid
 * @is_igmp: falg to indicate if the packet is igmp packet
 *
 * Return:
 * Total Number of successful unicast conversions
 */

uint16_t dp_me_convert_amsdu_ucast(struct cdp_soc_t *soc,
				   uint8_t vdev_id,
				   qdf_nbuf_t wbuf,
				   uint8_t mcast_grp_mac[][QDF_MAC_ADDR_SIZE],
				   uint8_t mcast_grp_mac_cnt,
				   uint8_t *macaddr, uint8_t tid, bool is_igmp)
{
	uint8_t total_mcast_grp_mac = mcast_grp_mac_cnt;
	struct cdp_tx_exception_metadata tx_exc_param = {0};
	struct cdp_tx_ingress_stats stats = {0};
	uint8_t xmit_type = qdf_nbuf_get_vdev_xmit_type(wbuf);
	qdf_nbuf_t wbuf_copy = NULL;

	while(mcast_grp_mac_cnt > 0 && mcast_grp_mac_cnt--) {
		if (mcast_grp_mac_cnt > 0) {
			wbuf_copy = qdf_nbuf_copy(wbuf);
			if (wbuf_copy == NULL) {
				qdf_nbuf_free(wbuf);
				stats.mcast_en.fail_seg_alloc++;
				return (total_mcast_grp_mac - (mcast_grp_mac_cnt+1));
			}
		} else {
			wbuf_copy = wbuf;
		}
		wbuf_copy->next = NULL;

		if (dp_dms_amsdu_war(soc, &wbuf_copy,
				     mcast_grp_mac[mcast_grp_mac_cnt],
				     &tx_exc_param, macaddr, tid)) {
			dp_me_err("%pK: Unable to convert to native wifi packet",
				  soc);
			qdf_nbuf_free(wbuf_copy);
			stats.mcast_en.dropped_map_error++;
			return (total_mcast_grp_mac - (mcast_grp_mac_cnt+1));
		}

		/* Successful conversion to ucast */
		if (is_igmp)
			stats.igmp_mcast_en.igmp_ucast_converted++;
		else
			stats.mcast_en.ucast++;

		wbuf_copy = cdp_tx_send_exc(soc, vdev_id, wbuf_copy, &tx_exc_param);

		if (wbuf_copy != NULL) {
			qdf_nbuf_free(wbuf_copy);
			stats.mcast_en.dropped_send_fail++;
		}
	}

	/* Updating stats */
	cdp_update_vdev_host_stats(soc, vdev_id, &stats, DP_VDEV_STATS_TX_ME,
				   xmit_type);
	return (total_mcast_grp_mac - mcast_grp_mac_cnt);
}
#endif

static inline bool igmp_query_check (uint8_t igmp_pkt)
{

	switch (igmp_pkt){
		case IGMP_QUERY :
		case MLD_QUERY :
		case IGMPv1_REPORT:
		case IGMPv2_REPORT:
		case IGMP_LEAVE :
		case IGMPv3_REPORT :
		case MLD_REPORT :
		case MLD_LEAVE:
		case MLDv2_REPORT :
			return true ;
		default:
			return false;
	}

}

uint16_t
dp_me_convert_ucast(struct cdp_soc_t *soc, uint8_t vdev_id,
		    qdf_nbuf_t wbuf, u_int8_t newmac[][QDF_MAC_ADDR_SIZE],
		    uint8_t new_mac_cnt, uint8_t me_mode, u_int8_t *macaddr,
		    uint8_t tid, bool is_igmp)
{
	bool is_dms_pkt = false;

	if (me_mode == MC_AMSDU_ENABLE) {
		if (!dp_get_me_dms_support_needed_in_sw(soc,vdev_id)) {
			is_dms_pkt = true;
			return cdp_tx_me_convert_ucast(soc, vdev_id, wbuf,
						       newmac, new_mac_cnt,
						       tid, is_igmp,
						       is_dms_pkt);
		}
#ifdef QCA_OL_DMS_WAR
		else
			return dp_me_convert_amsdu_ucast(soc, vdev_id, wbuf,
							 newmac, new_mac_cnt,
							 macaddr, tid, is_igmp);
#endif
	}
		return cdp_tx_me_convert_ucast(soc, vdev_id, wbuf,
					       newmac, new_mac_cnt,
					       tid, is_igmp,
					       is_dms_pkt);
}

int dp_me_mcast_filter(struct dp_me_mcast_node *node, const void *ip_header,
		u_int16_t protocol)
{
	int i;

	if (!node->filter_mode ||
	    (node->filter_mode == DP_ME_MCAST_EXCLUDE && !node->nsrcs))
		return 0;

	if (node->filter_mode == DP_ME_MCAST_INCLUDE && !node->nsrcs)
		return 1;

	if (protocol == htobe16(ETHERTYPE_IP)) {
		u_int32_t ip4 = ((struct ip_header *)ip_header)->saddr;
		const u_int32_t *srcs = (u_int32_t *)node->srcs;
		for (i = 0; i < node->nsrcs; i++) {
			if (srcs[i] == ip4)
				break;
		}
	} else if (protocol == htobe16(ETHERTYPE_IPV6)) {
		qdf_net_ipv6_addr_t *ip6 = &((qdf_net_ipv6hdr_t *)ip_header)->ipv6_saddr;
		qdf_net_ipv6_addr_t *srcs = (qdf_net_ipv6_addr_t *)node->srcs;
		for (i = 0; i < node->nsrcs; i++) {
			if (!qdf_mem_cmp(&srcs[i], ip6, sizeof(*srcs)))
				break;
		}
	} else {
		return 0;
	}

	return ((node->filter_mode == DP_ME_MCAST_INCLUDE && i == node->nsrcs) ||
		(node->filter_mode == DP_ME_MCAST_EXCLUDE && i != node->nsrcs));
}


static int dp_me_mcast_hmmc_convert(ol_txrx_soc_handle soc,
				    uint8_t vdev_id,
				    dp_vdev_me_t *vdev_me,
				    qdf_nbuf_t wbuf,
				    uint8_t tid, bool is_igmp)
{
	u_int8_t newmac[MAX_SNOOP_ENTRIES][QDF_MAC_ADDR_SIZE];
	int new_mac_cnt = 0;                        /* count of entries in newmac */

	/* Convert the packet to unicast to all STAs of vdev */
	new_mac_cnt = cdp_vdev_get_peer_mac_list(soc,vdev_id,newmac,
						 MAX_SNOOP_ENTRIES, false);
	return dp_me_convert_ucast(soc, vdev_id, wbuf, newmac, new_mac_cnt,
				   vdev_me->me_mcast_mode, vdev_me->macaddr, tid,
				   is_igmp);
}

/*
 * dp_igmp_me_mcast_convert: Convert igmp mcast packets to unicast
 *
 * Parameters:
 * @soc: soc txrx handle
 * @vdev_id: id of handle to vap pointer
 * @pdev_id: id of handle to pdev pointer
 * @wbuf: Frame buffer containing the 802.3 frame
 * @tid: desired tid
 *
 * Return:
 * n  : Total Number of successful unicast conversions
 * < 0 : Packets not converted
 */

int dp_igmp_me_mcast_convert(struct cdp_soc_t *soc, uint8_t vdev_id,
			     uint8_t pdev_id, qdf_nbuf_t wbuf)
{
	dp_vdev_igmp_me_t *vdev_igmp_me;
	dp_vdev_me_t *vdev_me;
	int new_mac_cnt;
	uint8_t igmp_pkt = 0;
	u_int8_t newmac[MAX_SNOOP_ENTRIES][QDF_MAC_ADDR_SIZE];

	vdev_me = dp_get_vdev_me_handle(soc, vdev_id);
	if (!vdev_me)
		return -1;

	vdev_igmp_me = dp_get_vdev_igmp_me_handle(soc, vdev_id);

	if (!vdev_igmp_me || !vdev_igmp_me->igmp_me_enabled) {
		return -1;
	}

	if (dp_me_igmp_mld_inspect(soc, pdev_id, wbuf, &igmp_pkt, NULL, NULL, NULL)) {
		if (!igmp_query_check(igmp_pkt)){
			/* Not an IGMP/MLD packet */
			return -1;
		}
	}

	new_mac_cnt = cdp_vdev_get_peer_mac_list(soc, vdev_id, newmac,
						 MAX_SNOOP_ENTRIES, true);

	if (!new_mac_cnt) {
		return -1;
	}

	return dp_me_convert_ucast(soc, vdev_id, wbuf, newmac, new_mac_cnt,
				   vdev_me->me_mcast_mode,
				   vdev_igmp_me->macaddr,
				   DP_VO_TID, true);
}
qdf_export_symbol(dp_igmp_me_mcast_convert);

int dp_me_mcast_convert(struct cdp_soc_t *soc,
			uint8_t vdev_id, uint8_t pdev_id,
			qdf_nbuf_t wbuf, uint32_t flag)
{
	int n;
	u_int8_t hmmc_found = 0;
	u_int8_t deny_list = 0;
	rwlock_state_t lock_state;
	struct ether_header *eh = NULL;
	u_int8_t zero_mac[QDF_MAC_ADDR_SIZE];
	struct dp_me_mcast_group group;
	struct dp_me_mcast_entry *entry = NULL;
	struct dp_me_mcast_table *mcast_table;
	dp_me_snoop_lock_t       *mcast_lock;
	u_int8_t newmac[MAX_SNOOP_ENTRIES][QDF_MAC_ADDR_SIZE];
	int new_mac_cnt = 0;
	uint8_t igmp_pkt = 0;
	uint8_t *is_igmp = NULL;
	int group_ix = 0; /* Multicast group index */
	dp_vdev_me_t *vdev_me;
	uint32_t grp_cnt;
	uint16_t ether_type;
	uint8_t eth_hdr_size = sizeof(qdf_ether_header_t);
	struct dp_me_ra_entry (*me_ra)[DP_ME_MCAST_NODE_MAX];
	uint8_t xmit_type = qdf_nbuf_get_vdev_xmit_type(wbuf);

	vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

	if (!vdev_me)
		return -1;

#ifdef WLAN_FEATURE_11BE_MLO
	if (flag & DP_ME_MCTBL_MLD_VDEV) {
		mcast_table = &(vdev_me->mld_me_snoop_table.mcast_table);
		mcast_lock  = &(vdev_me->mld_me_snoop_table.lock);
		me_ra       = &(vdev_me->mld_me_snoop_table.me_ra[0]);
	} else
#endif
	{
		(void)flag;
		mcast_table = &(vdev_me->me_snoop_table.mcast_table);
		mcast_lock  = &(vdev_me->me_snoop_table.lock);
		me_ra       = &(vdev_me->me_snoop_table.me_ra[0]);
	}

	grp_cnt = mcast_table->entry_cnt;

	if (!vdev_me->me_mcast_mode)
		return -1;

	if (vdev_me->me_igmp_allow)
		is_igmp = &igmp_pkt;

	if (dp_me_igmp_mld_inspect(soc, pdev_id, wbuf, is_igmp, &group,
				   &hmmc_found, &deny_list)) {
		if (igmp_query_check(igmp_pkt)) {
			struct cdp_tx_ingress_stats stats = {0};
			stats.igmp_mcast_en.igmp_rcvd++;
			cdp_update_vdev_host_stats(soc, vdev_id, &stats,
						   DP_VDEV_STATS_TX_ME,
						   xmit_type);
			if (!vdev_me->me_igmp_allow)
				return -1;
		} else
			return -1;
	}

	if (deny_list)
		return -1;

	if (hmmc_found) {
		return dp_me_mcast_hmmc_convert(soc, vdev_id, vdev_me, wbuf,
						igmp_pkt ? DP_VO_TID : HTT_INVALID_TID,
						igmp_query_check(igmp_pkt));
	}

	OS_RWLOCK_READ_LOCK(mcast_lock, &lock_state);
	if (!mcast_table->entry_cnt ||
	    !(entry = dp_me_mcast_find_entry(&group, &group_ix, mcast_table)) ||
	    !entry->node_cnt ) {
		OS_RWLOCK_READ_UNLOCK(mcast_lock, &lock_state);
		if (vdev_me->me_igmp_allow && igmp_query_check(igmp_pkt)) {
			return -1;
		}

		/*If there are no one in group. drop the frame*/
		wbuf_complete(wbuf);
		return 1;
	}

	eh = (struct ether_header *) wbuf_header(wbuf);
	ether_type = eh->ether_type;

	if (ether_type == htons(ETH_P_8021Q)) {
		struct vlan_ethhdr *veth =
				(struct vlan_ethhdr *)wbuf_header(wbuf);
		ether_type = veth->h_vlan_encapsulated_proto;
		eth_hdr_size = sizeof(struct vlan_ethhdr);
	}

	switch (ether_type) {
	case htobe16(ETHERTYPE_IP):
	{
		struct ip_header *iph = (struct ip_header *)(wbuf_header(wbuf) +
								 eth_hdr_size);

		if (!vdev_me->me_igmp_allow && (iph->protocol == IPPROTO_IGMP))
			return -1;

		OS_MEMSET(&group, 0, sizeof group);
			  group.u.ip4 = ntohl(iph->daddr);
			  group.protocol = ETHERTYPE_IP;
	}
		break;
	case htobe16(ETHERTYPE_IPV6):
	{
		qdf_net_ipv6hdr_t *ip6h = (qdf_net_ipv6hdr_t *)(wbuf_header(wbuf) +
								   eth_hdr_size);
		u_int8_t *nexthdr = (u_int8_t *)(ip6h + 1);

		if (!vdev_me->me_igmp_allow &&
		    (ip6h->ipv6_nexthdr == IPPROTO_ICMPV6 ||
		    (ip6h->ipv6_nexthdr == IPPROTO_HOPOPTS &&
		     *nexthdr == IPPROTO_ICMPV6)))
			return -1;

		OS_MEMSET(&group, 0, sizeof group);
		OS_MEMCPY(group.u.ip6,
			  ip6h->ipv6_daddr.s6_addr,
			  sizeof(qdf_net_ipv6_addr_t));
		group.protocol = ETHERTYPE_IPV6;
	}
		break;
	default:
		return -1;
	}


	OS_MEMSET(zero_mac, 0, QDF_MAC_ADDR_SIZE);
	for (n = 0; n < entry->node_cnt; n++) {
		struct ip_header *iph = (struct ip_header *)(wbuf_header(wbuf) +
								eth_hdr_size);
		if ((WLAN_ADDR_EQ(eh->ether_shost, entry->nodes[n].mac) == 0) ||
		    (WLAN_ADDR_EQ(zero_mac, entry->nodes[n].mac) == 0 ) ||
		    dp_me_mcast_filter(&entry->nodes[n], iph, ether_type)) {
			continue;
		}

		if(new_mac_cnt < MAX_SNOOP_ENTRIES) {
			if (vdev_me->me_mcast_mode == MC_HYFI_ENABLE)
				WLAN_ADDR_COPY(newmac[new_mac_cnt], entry->nodes[n].mac);
			else
			{
				if (!me_ra[group_ix][n].dup) {
					WLAN_ADDR_COPY(newmac[new_mac_cnt], me_ra[group_ix][n].mac);
				} else {
					/* Peer is dropped since frame is being sent to a
					 * common next-hop node */
					continue;
				}
			}
			new_mac_cnt++;
		} else {
			qdf_nofl_info("WARNING: too many nodes in %s:%d",
					__func__, __LINE__);
			break;
		}
	}
	OS_RWLOCK_READ_UNLOCK(mcast_lock, &lock_state);

	if(new_mac_cnt) {
		return dp_me_convert_ucast(soc, vdev_id, wbuf, newmac, new_mac_cnt,
					   vdev_me->me_mcast_mode, vdev_me->macaddr,
					   igmp_pkt ? DP_VO_TID : HTT_INVALID_TID,
					   igmp_query_check(igmp_pkt));
	} else if(new_mac_cnt == 0){
		if ( dp_me_igmp_mld_inspect(soc, pdev_id, wbuf,
					&igmp_pkt, NULL, NULL, NULL))
			if(igmp_query_check(igmp_pkt)){
				return -1;
			}
		/*If there are no one in group. drop the frame*/
		wbuf_complete(wbuf);
		return 1;
	}
	return -1;
}
qdf_export_symbol(dp_me_mcast_convert);

static inline int _dp_me_deny_list_find(struct cdp_soc_t *soc, uint8_t pdev_id,
				   u_int32_t dip)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	for (i = 0; i < pdev_me_hdl->pdev_deny_list_cnt; i++) {
		if ((pdev_me_hdl->pdev_denylist[i].ip &
		     pdev_me_hdl->pdev_denylist[i].mask) ==
		    (dip & pdev_me_hdl->pdev_denylist[i].mask))
			return 1;
	}
	return 0;
}

static inline int _dp_me_list_v6_find(struct cdp_soc_t *soc, uint8_t pdev_id,
					   uint8_t *dip, enum list_typ lst_typ)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;
	ipv6_entry_t *src_list = NULL;
	u_int32_t src_count = 0;
	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	if(lst_typ == HMMC) {
		src_list = pdev_me_hdl->pdev_hmmcs_v6;
		src_count = pdev_me_hdl->pdev_hmmc_v6_cnt;
	} else if (lst_typ == DENY_LIST){
		src_list = pdev_me_hdl->pdev_denylist_v6;
		src_count = pdev_me_hdl->pdev_deny_list_v6_cnt;
	}
	if (!src_list) {
		return 0;
	}
	for (i = 0; i < src_count; i++) {
		u_int32_t dip_cpy[IPV6_UNIT32_SIZE], src_ip_cpy[IPV6_UNIT32_SIZE];
		int idx = 0;

		for (;idx < IPV6_UNIT32_SIZE; idx++) {
			src_ip_cpy[idx] = src_list[i].ip[idx] & src_list[i].mask[idx];
			dip_cpy[idx] = ((u_int32_t *)dip)[idx] & src_list[i].mask[idx];
		}
		if (!qdf_mem_cmp(src_ip_cpy, dip_cpy,
				 sizeof(uint32_t) * IPV6_UNIT32_SIZE))
			return 1;
	}
	return 0;
}

static inline int _dp_me_hmmc_find(struct cdp_soc_t *soc, uint8_t pdev_id,
				   u_int32_t dip)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	for (i = 0; i < pdev_me_hdl->pdev_hmmc_cnt; i++) {
		if ((pdev_me_hdl->pdev_hmmcs[i].ip & pdev_me_hdl->pdev_hmmcs[i].mask) ==
		    (dip & pdev_me_hdl->pdev_hmmcs[i].mask))
			return 1;
	}
	return 0;
}

static int dp_me_igmp_mld_inspect(struct cdp_soc_t *soc, uint8_t pdev_id,
				  qdf_nbuf_t wbuf, u_int8_t *type,
				  void *group, u_int8_t *hmmc_found, u_int8_t *deny_list)
{
	u_int16_t protocol;
	u_int32_t ip4 = 0;
	u_int8_t *ip6 = NULL;
	struct ether_header *eh = (struct ether_header *) wbuf_header(wbuf);
	int is_multicast = IEEE80211_IS_MULTICAST(eh->ether_dhost) &&
		!IEEE80211_IS_BROADCAST(eh->ether_dhost);
	int ret = 0;
	uint16_t ether_type;
	uint8_t eth_hdr_size = sizeof(qdf_ether_header_t);

	ether_type = eh->ether_type;

	if (ether_type == htons(ETH_P_8021Q)) {
		struct vlan_ethhdr *veth =
				(struct vlan_ethhdr *)wbuf_header(wbuf);
		ether_type = veth->h_vlan_encapsulated_proto;
		eth_hdr_size = sizeof(struct vlan_ethhdr);
	}

	switch (ether_type) {
	case htobe16(ETHERTYPE_IP):
	{
		struct ip_header *iph = (struct ip_header *)(wbuf_header(wbuf) + eth_hdr_size);
		int ip_headerlen;
		const struct igmp_header *igmp;

		ip4 = iph->saddr;
		protocol = ETHERTYPE_IP;

		if (iph->protocol == IPPROTO_IGMP) {
			if (!type) return 1;

			ip_headerlen = iph->version_ihl & 0x0F;

			igmp = (struct igmp_header *)(wbuf_header(wbuf) +
						eth_hdr_size + (4 * ip_headerlen));
			*type = igmp->type;
			ret = 1;
			break;
		}
		if (is_multicast && group) {
			struct dp_me_mcast_group *grp = (struct dp_me_mcast_group *)group;
			OS_MEMSET(grp, 0, sizeof(*grp));
			grp->u.ip4 = iph->daddr;
			grp->protocol = htobe16(ETHERTYPE_IP);
			if (deny_list) {
				*deny_list = _dp_me_deny_list_find(soc, pdev_id, iph->daddr);
				if (*deny_list)
					break;
			}
			if (hmmc_found)
				*hmmc_found = _dp_me_hmmc_find(soc, pdev_id, iph->daddr);
		}
	}
		break;
	case htobe16(ETHERTYPE_IPV6):
	{
		qdf_net_ipv6hdr_t *ip6h = (qdf_net_ipv6hdr_t *)(wbuf_header(wbuf) + eth_hdr_size);
		u_int8_t *nexthdr = (u_int8_t *)(ip6h + 1);
		qdf_net_icmpv6hdr_t *mld;

		ip6 = ip6h->ipv6_saddr.s6_addr;
		protocol = ETHERTYPE_IPV6;

		if (ip6h->ipv6_nexthdr == IPPROTO_ICMPV6 ||
		    (ip6h->ipv6_nexthdr == IPPROTO_HOPOPTS &&
		    *nexthdr == IPPROTO_ICMPV6)) {
			if (!type) return 1;

			if (ip6h->ipv6_nexthdr == IPPROTO_ICMPV6)
				mld = (qdf_net_icmpv6hdr_t *)nexthdr;
			else
				mld = (qdf_net_icmpv6hdr_t *)(nexthdr + 8);
			*type = mld->icmp6_type;
			ret = 1;
			break;
		}
		if (is_multicast && group) {
			struct dp_me_mcast_group *grp = (struct dp_me_mcast_group *)group;
			OS_MEMSET(grp, 0, sizeof(*grp));
			OS_MEMCPY(grp->u.ip6,
				  ip6h->ipv6_daddr.s6_addr,
				  sizeof(qdf_net_ipv6_addr_t));
			grp->protocol = htobe16(ETHERTYPE_IPV6);
			if (deny_list) {
				*deny_list = _dp_me_list_v6_find(soc, pdev_id,
								 ip6h->ipv6_daddr.s6_addr,
								 DENY_LIST);
				if (*deny_list)
					break;
			}

			if (hmmc_found)
				*hmmc_found = _dp_me_list_v6_find(soc, pdev_id,
								  ip6h->ipv6_daddr.s6_addr,
								  HMMC);

		}
	}
		break;
	default:
		/*
		 * This case returns 1 although the packet is not IGMP.
		 * This is being used by L2 layer multicast packets
		 * such as IEEE-1905.1-control packets
		 */
		return 1;
	}
	return ret;
}

static inline void
dp_me_mcast_table_init(struct dp_me_mcast_table *mcast_table) {
	int i;
	for(i = 0; i < DP_ME_MCAST_MAX_HASH; i++) {
		TAILQ_INIT(&mcast_table->hash_entry[i]);
	}
	mcast_table->entry_cnt = 0;
}

QDF_STATUS dp_me_attach(ol_txrx_soc_handle soc, uint8_t vdev_id,
			uint8_t *macaddr)
{
	struct dp_vdev_me *vdev_me;
	struct dp_me_mcast_table *mcast_table = NULL;

	vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

	if (!vdev_me)
		return QDF_STATUS_E_FAILURE;

	mcast_table = &(vdev_me->me_snoop_table.mcast_table);

	vdev_me->me_mcast_mode = 0;
	vdev_me->me_dms_support = 0;
	qdf_mem_copy(vdev_me->macaddr, macaddr, QDF_MAC_ADDR_SIZE);
        vdev_me->me_igmp_allow = 0;
	dp_me_mcast_table_init(mcast_table);
	OS_RWLOCK_INIT(&(vdev_me->me_snoop_table.lock));

#ifdef WLAN_FEATURE_11BE_MLO
	mcast_table = &(vdev_me->mld_me_snoop_table.mcast_table);
	dp_me_mcast_table_init(mcast_table);
	OS_RWLOCK_INIT(&(vdev_me->mld_me_snoop_table.lock));
#endif

	return QDF_STATUS_SUCCESS;
}

int dp_add_hmmc(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t ip, u_int32_t mask)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return -1;

	if (!ip || !mask || DP_ADD_HMMC_CHECK_IP(ip))
		return -EINVAL;

	for (i = 0; i < pdev_me_hdl->pdev_hmmc_cnt; i++) {
		if (pdev_me_hdl->pdev_hmmcs[i].ip == ip)
			break;
	}
	if (i != pdev_me_hdl->pdev_hmmc_cnt) {
		pdev_me_hdl->pdev_hmmcs[i].ip = ip;
		pdev_me_hdl->pdev_hmmcs[i].mask = mask;
		return 0;
	}
	if (pdev_me_hdl->pdev_hmmc_cnt < DP_ME_HMMC_CNT_MAX) {
		pdev_me_hdl->pdev_hmmcs[pdev_me_hdl->pdev_hmmc_cnt].ip = ip;
		pdev_me_hdl->pdev_hmmcs[pdev_me_hdl->pdev_hmmc_cnt].mask = mask;

		pdev_me_hdl->pdev_hmmc_cnt++;
		return 0;
	}
	return -1;
}

int dp_del_hmmc(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t ip, u_int32_t mask)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i, hmmc_size;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return -EINVAL;

	hmmc_size = sizeof(pdev_me_hdl->pdev_hmmcs) / DP_ME_HMMC_CNT_MAX;

	if (!ip || !mask)
		return -EINVAL;

	if (!pdev_me_hdl->pdev_hmmc_cnt)
		return 0;

	for (i = 0; i < pdev_me_hdl->pdev_hmmc_cnt; i++) {
		if (pdev_me_hdl->pdev_hmmcs[i].ip == ip &&
		    pdev_me_hdl->pdev_hmmcs[i].mask == mask)
			break;
	}

	if (i == pdev_me_hdl->pdev_hmmc_cnt)
		return -EINVAL;

	OS_MEMCPY(&pdev_me_hdl->pdev_hmmcs[i], &pdev_me_hdl->pdev_hmmcs[i+1],
		  (pdev_me_hdl->pdev_hmmc_cnt - i - 1) * hmmc_size );
	pdev_me_hdl->pdev_hmmc_cnt--;

	return 0;
}

int dp_hmmc_dump(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	dp_me_info("\nMULTICAST RANGE:");
	for (i = 0; i < pdev_me_hdl->pdev_hmmc_cnt; i++)
		qdf_info("\t%d of %d: %pI4/%pI4",
			 i+1,
			 pdev_me_hdl->pdev_hmmc_cnt,
			 &pdev_me_hdl->pdev_hmmcs[i].ip,
			 &pdev_me_hdl->pdev_hmmcs[i].mask);
	return 0;
}

void _populate_ipv6_mask(uint32_t *mask, uint32_t prefix) {
	int i = 0;
	qdf_mem_set(mask, sizeof(uint32_t) * IPV6_UNIT32_SIZE, 0);
	for (;i < IPV6_UNIT32_SIZE && (prefix > 0); i++) {
		if (prefix < (sizeof(uint32_t) * 8)) {
			mask[i] = htobe32(((1 << prefix) -1) <<
					  ((sizeof(uint32_t) * 8) - prefix));
			prefix = 0;
		} else {
			mask[i] = MAX_32BIT_VAL;
			prefix -= sizeof(uint32_t) * 8;
		}
	}
}

int dp_add_hmmc_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t *ip, u_int32_t prefix)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);
	if (!pdev_me_hdl)
		return -1;
	if (!ip || !prefix || DP_ADD_HMMC_CHECK_IP_V6(ip)
		  || prefix > MAX_IPV6_PREFIX)
		return -EINVAL;
	for (i = 0; i < pdev_me_hdl->pdev_hmmc_v6_cnt; i++) {
		if (!qdf_mem_cmp(pdev_me_hdl->pdev_hmmcs_v6[i].ip,
				 ip, sizeof(uint32_t)*IPV6_UNIT32_SIZE))
			break;
	}
	if (i != pdev_me_hdl->pdev_hmmc_v6_cnt) {
		_populate_ipv6_mask(pdev_me_hdl->pdev_hmmcs_v6[i].mask, prefix);
				    pdev_me_hdl->pdev_hmmcs_v6[i].prefix = prefix;
		return 0;
	}
	if (pdev_me_hdl->pdev_hmmc_v6_cnt < DP_ME_HMMC_CNT_MAX) {
		qdf_mem_copy(pdev_me_hdl->pdev_hmmcs_v6[pdev_me_hdl->pdev_hmmc_v6_cnt].ip,
			     ip, sizeof(uint32_t) * IPV6_UNIT32_SIZE);
		_populate_ipv6_mask(
			pdev_me_hdl->pdev_hmmcs_v6[pdev_me_hdl->pdev_hmmc_v6_cnt].mask,
			prefix);
		pdev_me_hdl->pdev_hmmcs_v6[pdev_me_hdl->pdev_hmmc_v6_cnt].prefix =
			prefix;
		pdev_me_hdl->pdev_hmmc_v6_cnt++;
		return 0;
	}
	return -1;
}
int dp_del_hmmc_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t *ip, u_int32_t prefix)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i, hmmc_v6_table_element_size;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);
	if (!pdev_me_hdl)
		return -EINVAL;
	hmmc_v6_table_element_size = sizeof(pdev_me_hdl->pdev_hmmcs_v6) /
				     DP_ME_HMMC_CNT_MAX;
	if (!ip || !prefix)
		return -EINVAL;
	if (!pdev_me_hdl->pdev_hmmc_v6_cnt)
		return 0;
	for (i = 0; i < pdev_me_hdl->pdev_hmmc_v6_cnt; i++) {
		if (!qdf_mem_cmp(pdev_me_hdl->pdev_hmmcs_v6[i].ip, ip,
				 sizeof(uint32_t)*4) &&
		    pdev_me_hdl->pdev_hmmcs_v6[i].prefix == prefix)
			break;
	}
	if (i == pdev_me_hdl->pdev_hmmc_v6_cnt)
		return -EINVAL;

	OS_MEMCPY(&pdev_me_hdl->pdev_hmmcs_v6[i], &pdev_me_hdl->pdev_hmmcs_v6[i+1],
		  (pdev_me_hdl->pdev_hmmc_v6_cnt - i - 1) * hmmc_v6_table_element_size);
	pdev_me_hdl->pdev_hmmc_v6_cnt--;

	return 0;
}
int dp_hmmc_dump_v6(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i;
	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	dp_me_info("\nMULTICAST RANGE: %d",pdev_me_hdl->pdev_deny_list_v6_cnt);
	for (i = 0; i < pdev_me_hdl->pdev_hmmc_v6_cnt; i++)
		qdf_info("\t%d of %d: %pI6/%pI6",
			 i+1,
			 pdev_me_hdl->pdev_hmmc_v6_cnt,
			 pdev_me_hdl->pdev_hmmcs_v6[i].ip,
			 pdev_me_hdl->pdev_hmmcs_v6[i].mask);
	return 0;
}

int dp_add_deny_list(ol_txrx_soc_handle soc, uint8_t pdev_id,
		     u_int32_t ip, u_int32_t mask)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return -1;

	if (!ip || !mask || DP_ADD_HMMC_CHECK_IP(ip))
		return -EINVAL;

	for (i = 0; i < pdev_me_hdl->pdev_deny_list_cnt; i++) {
		if (pdev_me_hdl->pdev_denylist[i].ip == ip)
			break;
	}

	/* IP found in list, updating entry as mask value may be different */
	if (i != pdev_me_hdl->pdev_deny_list_cnt) {
		pdev_me_hdl->pdev_denylist[i].ip = ip;
		pdev_me_hdl->pdev_denylist[i].mask = mask;
		return 0;
	}

	/* IP not found in existing list, so add entry in the list */
	if (pdev_me_hdl->pdev_deny_list_cnt < DP_ME_HMMC_CNT_MAX) {
		pdev_me_hdl->pdev_denylist[pdev_me_hdl->pdev_deny_list_cnt].ip = ip;
		pdev_me_hdl->pdev_denylist[pdev_me_hdl->pdev_deny_list_cnt].mask = mask;

		pdev_me_hdl->pdev_deny_list_cnt++;
		return 0;
	}
	return -1;
}

int dp_del_deny_list(ol_txrx_soc_handle soc, uint8_t pdev_id,
		     u_int32_t ip, u_int32_t mask)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i, hmmc_size;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return -EINVAL;

	hmmc_size = sizeof(pdev_me_hdl->pdev_denylist) / DP_ME_HMMC_CNT_MAX;

	if (!ip || !mask)
		return -EINVAL;

	if (!pdev_me_hdl->pdev_deny_list_cnt)
		return 0;

	for (i = 0; i < pdev_me_hdl->pdev_deny_list_cnt; i++) {
		if (pdev_me_hdl->pdev_denylist[i].ip == ip &&
		    pdev_me_hdl->pdev_denylist[i].mask == mask)
			break;
	}

	if (i == pdev_me_hdl->pdev_deny_list_cnt)
		return -EINVAL;

	OS_MEMCPY(&pdev_me_hdl->pdev_denylist[i], &pdev_me_hdl->pdev_denylist[i+1],
		  (pdev_me_hdl->pdev_deny_list_cnt - i - 1) * hmmc_size );
	pdev_me_hdl->pdev_deny_list_cnt--;

	return 0;
}

int dp_deny_list_dump(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	dp_me_info("\nMULTICAST RANGE:");
	for (i = 0; i < pdev_me_hdl->pdev_deny_list_cnt; i++)
		qdf_info("\t%d of %d: %pI4/%pI4",
			 i+1,
			 pdev_me_hdl->pdev_deny_list_cnt,
			 &pdev_me_hdl->pdev_denylist[i].ip,
			 &pdev_me_hdl->pdev_denylist[i].mask);
	return 0;
}

int dp_add_deny_list_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
			u_int32_t *ip, u_int32_t prefix)
{
	int i;
	dp_pdev_me_t *pdev_me_hdl;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return -1;

	if (!ip || !prefix || DP_ADD_HMMC_CHECK_IP_V6(ip) ||
           prefix > MAX_IPV6_PREFIX)
		return -EINVAL;

	for (i = 0; i < pdev_me_hdl->pdev_deny_list_v6_cnt; i++) {
		if (!qdf_mem_cmp(pdev_me_hdl->pdev_denylist_v6[i].ip,
				 ip, sizeof(uint32_t)*4))
			break;
	}

	/* IP found in list, updating entry as mask value may be different */
	if (i != pdev_me_hdl->pdev_deny_list_v6_cnt) {
		_populate_ipv6_mask(pdev_me_hdl->pdev_denylist_v6[i].mask,
				    prefix);
		pdev_me_hdl->pdev_denylist_v6[i].prefix = prefix;
		return 0;
	}

	/* IP not found in existing list, so add entry in the list */
	if (pdev_me_hdl->pdev_deny_list_v6_cnt < DP_ME_HMMC_CNT_MAX) {
		qdf_mem_copy(
		   pdev_me_hdl->pdev_denylist_v6[pdev_me_hdl->pdev_deny_list_v6_cnt].ip,
		   ip, sizeof(uint32_t) * IPV6_UNIT32_SIZE);
		_populate_ipv6_mask(
			pdev_me_hdl->pdev_denylist_v6[pdev_me_hdl->pdev_deny_list_v6_cnt].mask,
			prefix);
		pdev_me_hdl->pdev_denylist_v6[pdev_me_hdl->pdev_deny_list_v6_cnt].prefix =
			prefix;
		pdev_me_hdl->pdev_deny_list_v6_cnt++;
		return 0;
	}
	return -1;
}

int dp_del_deny_list_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
			u_int32_t *ip, u_int32_t prefix)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i, hmmc_size;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return -EINVAL;

	hmmc_size = sizeof(pdev_me_hdl->pdev_denylist_v6) / DP_ME_HMMC_CNT_MAX;

	if (!ip || !prefix)
		return -EINVAL;

	if (!pdev_me_hdl->pdev_deny_list_v6_cnt)
		return 0;

	for (i = 0; i < pdev_me_hdl->pdev_deny_list_v6_cnt; i++) {
		if (!qdf_mem_cmp(pdev_me_hdl->pdev_denylist_v6[i].ip,
				 ip, sizeof(uint32_t)*4) &&
		    pdev_me_hdl->pdev_denylist_v6[i].prefix == prefix)
			break;
	}

	if (i == pdev_me_hdl->pdev_deny_list_v6_cnt)
		return -EINVAL;

	OS_MEMCPY(&pdev_me_hdl->pdev_denylist_v6[i],
		  &pdev_me_hdl->pdev_denylist_v6[i+1],
		  (pdev_me_hdl->pdev_deny_list_v6_cnt - i - 1) * hmmc_size );
	pdev_me_hdl->pdev_deny_list_v6_cnt--;

	return 0;
}

int dp_deny_list_dump_v6(ol_txrx_soc_handle soc, uint8_t pdev_id)
{
	dp_pdev_me_t *pdev_me_hdl;
	int i;

	pdev_me_hdl = dp_get_pdev_me_handle(soc, pdev_id);

	if (!pdev_me_hdl)
		return 0;

	dp_me_info("\nMULTICAST RANGE:");
	for (i = 0; i < pdev_me_hdl->pdev_deny_list_v6_cnt; i++)
		qdf_info("\t%d of %d: %pI6/%pI6",
			 i+1,
			 pdev_me_hdl->pdev_deny_list_v6_cnt,
			 pdev_me_hdl->pdev_denylist_v6[i].ip,
			 pdev_me_hdl->pdev_denylist_v6[i].mask);
	return 0;
}

static inline
void dp_dump_snoop_table(struct dp_me_snoop_table *snoop_table)
{
	uint32_t grp_cnt, node_cnt, cnt;
	uint32_t node_idx;
	struct dp_me_mcast_table *table = NULL;
	struct dp_me_mcast_hash_entry *hash_entry = NULL;
	struct dp_me_ra_entry (*me_ra)[DP_ME_MCAST_NODE_MAX];

	table = &snoop_table->mcast_table;
	me_ra = &snoop_table->me_ra[0];

	grp_cnt = table->entry_cnt;

	if (!grp_cnt) {
		return;
	}

	dp_me_info("-------------------------------------------------------------------");
	dp_me_info("|  Group Address  | Destination Address | Receiver Address  | Dup |");
	dp_me_info("-------------------------------------------------------------------");

	for (cnt = 0; cnt < DP_ME_MCAST_MAX_HASH; cnt++) {
		TAILQ_FOREACH(hash_entry, &table->hash_entry[cnt], me_mcast_entry_list_elem) {
			if(!hash_entry)
				continue;

			node_cnt = hash_entry->entry.node_cnt;
			for (node_idx = 0; node_idx < node_cnt; node_idx++) {
				dp_me_info("| %pi4 |  %pM  | %pM |  %u  |",
					   &(hash_entry->entry.group.u.ip4),
				   &(hash_entry->entry.nodes[node_idx].mac),
				   &(me_ra[hash_entry->group_id][node_idx].mac),
				   me_ra[hash_entry->group_id][node_idx].dup);
			}
		}
	}
	dp_me_info("-------------------------------------------------------------------");
}

/**
 * Function to dump the RA table used for the Multicast Enhancement mode 6
 * feature.
 * @vdev - Handle to vdev structure
 */
void dp_show_me_ra_table(ol_txrx_soc_handle soc, uint8_t vdev_id, uint32_t flag)
{
	dp_vdev_me_t *vdev_me = dp_get_vdev_me_handle(soc, vdev_id);

	if (!vdev_me)
		return;

	dp_me_info("-------------------Dumping Legacy RA table-------------------------");
	dp_dump_snoop_table(&vdev_me->me_snoop_table);

#ifdef WLAN_FEATURE_11BE_MLO
	if (flag & DP_ME_MCTBL_MLD_VDEV) {
		dp_me_info("-------------------Dumping Mld RA table---------------------");
		dp_dump_snoop_table(&vdev_me->mld_me_snoop_table);
	}
#endif
}

#endif /* ATH_SUPPORT_IQUE */

