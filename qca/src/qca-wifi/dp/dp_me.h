/*
 * Copyright (c) 2013, 2018-2021, 2023-2024 Qualcomm Innovation Center, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

/*
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
/*
 * 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#if ATH_SUPPORT_IQUE

#ifndef DP_ME_PRIV_H
#define DP_ME_PRIV_H

#include <qdf_nbuf.h>
#include <ieee80211.h>
#include <cdp_txrx_cmn_struct.h>

#ifdef CONFIG_SAWF
#include <qdf_hashtable.h>
#endif /*CONFIG_SAWF*/

#ifndef MAX_SNOOP_ENTRIES
#define MAX_SNOOP_ENTRIES    64    /* max number*/
#endif

#define MC_ME_DISABLE          (0)
#define MC_HYFI_ENABLE         (5)
#define MC_AMSDU_ENABLE        (6)

#define DP_REPORT_FROM_STA 1
#define DP_QUERY_FROM_STA  2
#define DP_ME_MCAST_SRCS_MAX 4
#define DP_ME_MCAST_NODE_MAX 8
#define DP_ME_MCAST_GROUP_MAX 256
#define DP_ME_MCAST_INCLUDE 1
#define DP_ME_MCAST_EXCLUDE 2
#define DP_ME_HMMC_CNT_MAX 8
#define DP_ME_MCAST_MAX_HASH 256
#define DP_ME_INVALID_HASH 25535
#define MAX_IPV6_PREFIX 128
#define IPV6_UNIT32_SIZE 4

#ifdef CONFIG_SAWF
#define DP_SAWF_ME_SCV_ID_MAX 16
#define DP_ME_MCAST_HASH_MAX_BIT 8
#endif /*CONFIG_SAWF*/

#define DP_ADD_HMMC_CHECK_IP(ip) \
		(((ip) & htobe32(0xf0000000)) != htobe32(0xe0000000))
#define DP_ADD_HMMC_CHECK_IP_V6(ip) \
			(((ip[0]) & htobe32(0xff000000)) != htobe32(0xff000000))

#define DP_ME_MCTBL_SEG_FIRST 0x1
#define DP_ME_MCTBL_SEG_LAST 0x2
#ifdef WLAN_FEATURE_11BE_MLO
#define DP_ME_MCTBL_MLD_VDEV 0x4
#endif

#define dp_me_alert(params...) QDF_TRACE_FATAL(QDF_MODULE_ID_ME, params)
#define dp_me_err(params...) QDF_TRACE_FATAL(QDF_MODULE_ID_ME, params)
#define dp_me_warn(params...) QDF_TRACE_WARN(QDF_MODULE_ID_ME, params)
#define dp_me_info(params...) QDF_TRACE_INFO(QDF_MODULE_ID_ME, params)
#define dp_me_debug(params...) QDF_TRACE_DEBUG(QDF_MODULE_ID_ME, params)

#define DP_VO_TID 6
/*
 * Data structures for mcast enhancement
 */

typedef rwlock_t dp_me_snoop_lock_t;

enum list_typ {
	HMMC,
	DENY_LIST
};

#ifdef CONFIG_SAWF
typedef enum sawf_admission_control_state {
	SAWF_AC_PRIORITIZED,
	SAWF_AC_DEPRIORITIZED,
} ac_state_t;
#endif /* CONFIG_SAWF */

struct dp_me_mcast_group {
	u_int32_t                       protocol;
	union {
		u_int32_t                   ip4;
		u_int8_t                    ip6[QDF_IPV6_ADDR_SIZE];
	} u;
};
struct dp_me_mcast_node {
	u_int8_t mac[QDF_MAC_ADDR_SIZE];
	u_int8_t filter_mode;
	u_int8_t nsrcs;
	u_int8_t srcs[DP_ME_MCAST_SRCS_MAX * QDF_IPV6_ADDR_SIZE];
	char if_name[IFNAMSIZ];
};
struct dp_me_mcast_entry {
	struct dp_me_mcast_group  group;
	u_int32_t                 node_cnt;
	struct dp_me_mcast_node   nodes[DP_ME_MCAST_NODE_MAX];
};
struct dp_me_mcast_hash_entry {
	uint32_t group_id;
	struct dp_me_mcast_entry entry;
	TAILQ_ENTRY(dp_me_mcast_hash_entry) me_mcast_entry_list_elem;
};

struct dp_me_mcast_table {
	uint32_t                 entry_cnt;
	TAILQ_HEAD(,dp_me_mcast_hash_entry) hash_entry[DP_ME_MCAST_MAX_HASH];
};

struct dp_me_ra_entry {
	uint8_t mac[QDF_MAC_ADDR_SIZE];
	/* Duplicate bit is set if the peer's next hop is the same as
	   an existing entry */
	bool   dup;
};

struct dp_me_snoop_table {
	dp_me_snoop_lock_t         lock;
	struct dp_me_mcast_table   mcast_table;
	struct dp_me_ra_entry      me_ra[DP_ME_MCAST_GROUP_MAX][DP_ME_MCAST_NODE_MAX];
#ifdef CONFIG_SAWF
	qdf_ht_declare(sawf_me_group_state, DP_ME_MCAST_NODE_MAX);
#endif /* CONFIG_SAWF */
};

typedef struct dp_vdev_me {
	uint8_t                    me_mcast_mode;
	uint8_t                    me_igmp_allow;
	struct dp_me_snoop_table   me_snoop_table;
#ifdef WLAN_FEATURE_11BE_MLO
	struct dp_me_snoop_table   mld_me_snoop_table;
#endif
	uint8_t                    macaddr[QDF_MAC_ADDR_SIZE];
	uint8_t                    me_dms_support:1,
#ifdef CONFIG_SAWF
				   sawf_me_en:1,
#endif /* CONFIG_SAWF */
				   reserved:6;
}dp_vdev_me_t;

typedef struct ipv6_entry {
	u_int32_t ip[IPV6_UNIT32_SIZE];
	u_int32_t mask[IPV6_UNIT32_SIZE];
	u_int32_t prefix;
} ipv6_entry_t;

typedef struct ipv4_entry {
	u_int32_t ip;
	u_int32_t mask;
} ipv4_entry_t;


typedef struct dp_pdev_me {
	u_int32_t pdev_hmmc_cnt;
	ipv4_entry_t pdev_hmmcs[DP_ME_HMMC_CNT_MAX];
	u_int32_t pdev_deny_list_cnt;
	ipv4_entry_t pdev_denylist[DP_ME_HMMC_CNT_MAX];
	u_int32_t pdev_hmmc_v6_cnt;
	ipv6_entry_t pdev_hmmcs_v6[DP_ME_HMMC_CNT_MAX];
	u_int32_t pdev_deny_list_v6_cnt;
	ipv6_entry_t pdev_denylist_v6[DP_ME_HMMC_CNT_MAX];
	char glob_table[sizeof(struct dp_me_mcast_entry) * DP_ME_MCAST_GROUP_MAX];
	int glob_offset;
	dp_vdev_me_t *glob_cur_vdev;

}dp_pdev_me_t;

typedef struct dp_vdev_igmp_me {
	uint8_t                    igmp_me_enabled;
	uint8_t                    macaddr[QDF_MAC_ADDR_SIZE];
}dp_vdev_igmp_me_t;

typedef struct dp_me_mac_fmt {
	uint8_t                    *mac;
#ifdef CONFIG_SAWF
	uint32_t                   metadata;
#endif /* CONFIG_SAWF */
}dp_me_mac_fmt_t;

#ifdef CONFIG_SAWF
typedef struct sawf_metadata {
	uint32_t metadata;
	bool     is_valid;
}sawf_metadata_t;

typedef struct dp_me_sawf_node_state {
	bool       queried;
	ac_state_t ac_state; /* Admission control State*/
	sawf_metadata_t   sawf_mark; /* Associated mark metadata*/
} dp_me_sawf_node_state_t;

typedef struct dp_me_sawf_node {
	uint8_t mac[QDF_MAC_ADDR_SIZE];
	uint8_t is_wds_ext;
	dp_me_sawf_node_state_t state[DP_SAWF_ME_SCV_ID_MAX];
	struct qdf_ht_entry node_hash_ptr;
} dp_me_sawf_node_t;

typedef struct dp_me_sawf_group_state {
	struct dp_me_mcast_group  group;
	uint8_t flow_count;    /* No. of source IPs associated with group*/
	qdf_ht_declare(sawf_me_node_state, DP_ME_MCAST_NODE_MAX);
	struct qdf_ht_entry group_hash_ptr;
} dp_me_sawf_group_state_t;
#endif /* CONFIG_SAWF */

int dp_add_hmmc(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t ip, u_int32_t mask);

int dp_del_hmmc(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t ip, u_int32_t mask);

int dp_hmmc_dump(ol_txrx_soc_handle soc, uint8_t pdev_id);

int dp_add_hmmc_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t *ip, u_int32_t mask);

int dp_del_hmmc_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
		u_int32_t *ip, u_int32_t mask);

int dp_hmmc_dump_v6(ol_txrx_soc_handle soc, uint8_t pdev_id);

int dp_add_deny_list(ol_txrx_soc_handle soc, uint8_t pdev_id,
		     u_int32_t ip, u_int32_t mask);

int dp_del_deny_list(ol_txrx_soc_handle soc, uint8_t pdev_id,
		     u_int32_t ip, u_int32_t mask);

int dp_deny_list_dump(ol_txrx_soc_handle soc, uint8_t pdev_id);

int dp_add_deny_list_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
			u_int32_t *ip, u_int32_t mask);

int dp_del_deny_list_v6(ol_txrx_soc_handle soc, uint8_t pdev_id,
			u_int32_t *ip, u_int32_t mask);

int dp_deny_list_dump_v6(ol_txrx_soc_handle soc, uint8_t pdev_id);

/**
 * Show RA table used for Multicast Enhancement Mode 6
 *
 * @soc: soc txrx handle
 * @vdev_id : vdev_id
 *
 * Return: void
 */
void dp_show_me_ra_table(ol_txrx_soc_handle soc, uint8_t vdev_id,
						uint32_t flag);

/**
 * dp_pdev_me_setup() - Setup pdev me handle
 *
 * @soc: soc txrx handle
 * @pdev_id : pdev_id
 *
 * Return: void
 */
void dp_pdev_me_setup(ol_txrx_soc_handle soc, uint8_t pdev_id);

QDF_STATUS dp_me_attach(ol_txrx_soc_handle soc, uint8_t vdev_id,
			uint8_t *macaddr);

void
dp_me_update_mcast_table(ol_txrx_soc_handle soc, uint8_t vdev_id,
			 uint8_t pdev_id, void *data, uint32_t msglen,
			 uint16_t hdrlen, uint32_t flag);

int dp_me_mcast_convert(struct cdp_soc_t *soc,
			uint8_t vdev_id,
			uint8_t pdev_id,
			qdf_nbuf_t wbuf,
			uint32_t flag);

int dp_igmp_me_mcast_convert(struct cdp_soc_t *soc,
			     uint8_t vdev_id,
			     uint8_t pdev_id,
			     qdf_nbuf_t wbuf);

void dp_me_set_sawf_metadata(qdf_nbuf_t nbuf, uint32_t sawf_markmark);
#ifdef CONFIG_SAWF
dp_me_sawf_node_t *dp_me_allocate_hash_node(uint8_t *mac, uint8_t is_wds_ext);

void dp_me_get_interface_mac(u_int8_t *mac, struct dp_me_mcast_node *node,
			     bool *is_vlan);
#endif /* CONFIG_SAWF */
#endif
#endif /* ATH_SUPPORT_IQUE */
