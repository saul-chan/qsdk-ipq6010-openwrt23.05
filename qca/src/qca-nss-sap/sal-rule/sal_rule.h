/*
 * Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __SAL_RULE_H
#define __SAL_RULE_H

#include <stdint.h>
#include <stdio.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/route/link.h>

#define ETH_ALEN 6
#define IPV6_LEN 4

#define SP_RULE_FLAG_MATCH_ALWAYS_TRUE          0x01            /* Rule match always true. (skip field matching) mask */
#define SP_RULE_FLAG_MATCH_PRECEDENCE           0x04            /* Match precedence mask*/
#define SP_RULE_FLAG_MATCH_RULE_OUTPUT          0x08            /* Match rule output mask*/
#define SP_RULE_FLAG_MATCH_USER_PRIORITY        0x10            /* Match user priority mask*/
#define SP_RULE_FLAG_MATCH_SRC_MAC              0x20            /* Match source mac address mask*/
#define SP_RULE_FLAG_MATCH_DST_MAC              0x40            /* Match Destination mac address mask*/
#define SP_RULE_FLAG_MATCH_SRC_IPV4             0x80            /* Match source ipv4 address match mask */
#define SP_RULE_FLAG_MATCH_DST_IPV4             0x100           /* Match destination ipv4 address  mask */
#define SP_RULE_FLAG_MATCH_SRC_IPV6             0x200           /* Match source ipv6 address match mask */
#define SP_RULE_FLAG_MATCH_DST_IPV6             0x400           /* Match Destination ipv6 address match mask */
#define SP_RULE_FLAG_MATCH_SRC_PORT             0x800           /* Match source port mask */
#define SP_RULE_FLAG_MATCH_DST_PORT             0x1000          /* Match destination port mask */
#define SP_RULE_FLAG_MATCH_PROTO_NUM            0x2000          /* Match protocol number mask */
#define SP_RULE_FLAG_MATCH_VLAN_ID              0x4000          /* Match VLAN ID mask */
#define SP_RULE_FLAG_MATCH_DSCP                 0x8000          /* Match DSCP mask */
#define SP_RULE_FLAG_MATCH_VLAN_PCP             0x10000         /* Match VLAN pcp mask */
#define SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID     0x20000         /* Match Service class ID */
#define SP_RULE_FLAG_MATCH_SRC_IPV4_MASK	0x40000		/* Match Source ipv4 address mask */
#define SP_RULE_FLAG_MATCH_DST_IPV4_MASK	0x80000		/* Match Destination ipv4 address mask */
#define SP_RULE_FLAG_MATCH_SRC_IPV6_MASK	0x100000	/* Match Source ipv6 address mask */
#define SP_RULE_FLAG_MATCH_DST_IPV6_MASK	0x200000	/* Match Destination ipv6 address mask	*/
#define SP_RULE_FLAG_MATCH_DSCP_REMARK          0x400000        /* Match DSCP remark mask */
#define SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK      0x800000        /* Match VLAN pcp remark mask */
#define SP_RULE_FLAG_MATCH_IP_VERSION_TYPE	0x1000000	/* Match IP Version type */
#define SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE	0x2000000	/* Match Classifier type */
#define SP_RULE_FLAG_MATCH_SPI			0x4000000	/* Match SPI for TCLASS10 */
#define SP_RULE_FLAG_MATCH_IFINDEX		0x8000000	/* Match Interface Index */
#define SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_START	0x10000000	/* Match Source port range start */
#define SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_END	0x20000000	/* Match Source port range end */
#define SP_RULE_FLAG_MATCH_DST_PORT_RANGE_START	0x40000000	/* Match Destination port range start */
#define SP_RULE_FLAG_MATCH_DST_PORT_RANGE_END	0x80000000	/* Match Destination port range end */

#define SP_RULE_FLAG_EXT_MATCH_AE_TYPE		0x01		/* Match ae type extended flags */
#define SP_RULE_FLAG_EXT_MATCH_SRC_IFACE	0x02		/* Match src interface extended flags */
#define SP_RULE_FLAG_EXT_MATCH_DST_IFACE	0x04		/* Match dst interface extended flags */

/*
 * Attributes added for EMESH classifier.
 */
#define SP_RULE_FLAG_EXT_BURST_SIZE_DL		0x08		/* Match Burst Size in DL Direction */
#define SP_RULE_FLAG_EXT_BURST_SIZE_UL		0x10		/* Match Burst Size in UL Direction */
#define SP_RULE_FLAG_EXT_MSCS_TID_BITMAP	0x20		/* Match MSCS TID Bitmap */
#define SP_RULE_FLAG_EXT_SENSE_MESH_FLAGS_IN	0x40		/* Match MESH Classifier Type flags */
#define SP_RULE_FLAG_EXT_SVC_INTERVAL_DL	0x80		/* Match Service interval in DL Direction */
#define SP_RULE_FLAG_EXT_SVC_INTERVAL_UL	0x100		/* Match Service interval in UL Direction */

enum {
	SPM_CMD_RULE_UNSPEC,		/* Must NOT use element 0 */
	SPM_CMD_RULE_ACTION,
	SPM_CMD_RULE_QUERY,
	SPM_CMD_RULE_FLUSH,
	SPM_CMD_MAX,
};

enum sp_gnl_attr {
	SP_GNL_ATTR_UNSPEC,
	SP_GNL_ATTR_ID,
	SP_GNL_ATTR_ADD_DELETE_RULE,
	SP_GNL_ATTR_RULE_PRECEDENCE,
	SP_GNL_ATTR_RULE_OUTPUT,
	SP_GNL_ATTR_USER_PRIORITY,
	SP_GNL_ATTR_SRC_MAC,
	SP_GNL_ATTR_DST_MAC,
	SP_GNL_ATTR_SRC_IPV4_ADDR,
	SP_GNL_ATTR_SRC_IPV4_ADDR_MASK,
	SP_GNL_ATTR_DST_IPV4_ADDR,
	SP_GNL_ATTR_DST_IPV4_ADDR_MASK,
	SP_GNL_ATTR_SRC_IPV6_ADDR,
	SP_GNL_ATTR_SRC_IPV6_ADDR_MASK,
	SP_GNL_ATTR_DST_IPV6_ADDR,
	SP_GNL_ATTR_DST_IPV6_ADDR_MASK,
	SP_GNL_ATTR_SRC_PORT,
	SP_GNL_ATTR_DST_PORT,
	SP_GNL_ATTR_PROTOCOL_NUMBER,
	SP_GNL_ATTR_VLAN_ID,
	SP_GNL_ATTR_DSCP,
	SP_GNL_ATTR_DSCP_REMARK,
	SP_GNL_ATTR_VLAN_PCP,
	SP_GNL_ATTR_VLAN_PCP_REMARK,
	SP_GNL_ATTR_SERVICE_CLASS_ID,
	SP_GNL_ATTR_IP_VERSION_TYPE,
	SP_GNL_ATTR_CLASSIFIER_TYPE,
	SP_GNL_ATTR_FILTER_VALUE,
	SP_GNL_ATTR_FILTER_MASK,
	SP_GNL_ATTR_TID_BITMAP,
	SP_GNL_ATTR_PRIORITY_LIMIT,
	SP_GNL_ATTR_IFINDEX,
	SP_GNL_ATTR_SRC_PORT_RANGE_START,
	SP_GNL_ATTR_SRC_PORT_RANGE_END,
	SP_GNL_ATTR_DST_PORT_RANGE_START,
	SP_GNL_ATTR_DST_PORT_RANGE_END,
	SP_GNL_ATTR_AE_TYPE,
	SP_GNL_ATTR_SRC_IFACE,
	SP_GNL_ATTR_DST_IFACE,
	SP_GNL_ATTR_SVC_INTERVAL_DL,
	SP_GNL_ATTR_SVC_INTERVAL_UL,
	SP_GNL_ATTR_BURST_SIZE_DL,
	SP_GNL_ATTR_BURST_SIZE_UL,
	SP_GNL_ATTR_SENSE_MESH_FLAG_IN,
	SP_GNL_ATTR_MAX,
};

/*
 * sp_mapdb_update_results
 *      Result values of rule update.
 */
enum sp_mapdb_update_results {
	SP_MAPDB_UPDATE_RESULT_ERR_TBLFULL,		/* The rule table is full. */
	SP_MAPDB_UPDATE_RESULT_ERR_TBLEMPTY,		/* The rule table is empty. */
	SP_MAPDB_UPDATE_RESULT_ERR_ALLOCNODE,		/* Failed at allocating memory for a rule node. (struct sp_mapdb_rule_node) */
	SP_MAPDB_UPDATE_RESULT_ERR_ALLOCHASH,		/* Failed at allocating memory for a hashentry. */
	SP_MAPDB_UPDATE_RESULT_ERR_RULENOEXIST,		/* There is no such rule with the given rule id. */
	SP_MAPDB_UPDATE_RESULT_ERR_UNKNOWNBIT,		/* Unknown add/remove filter bit. */
	SP_MAPDB_UPDATE_RESULT_ERR_SINGLE_WRITER,	/* Single writer protection violation. */
	SP_MAPDB_UPDATE_RESULT_ERR_INVALIDENTRY,	/* Invalid entry of rule field. */
	SP_MAPDB_UPDATE_RESULT_ERR_NEWRULE_NULLPTR,	/* New rule is a null pointer. */
	SP_MAPDB_UPDATE_RESULT_ERR,			/* Delimiter */
	SP_MAPDB_UPDATE_RESULT_SUCCESS_ADD,		/* Successful rule add */
	SP_MAPDB_UPDATE_RESULT_SUCCESS_DELETE,		/* Successful rule deletion */
	SP_MAPDB_UPDATE_RESULT_SUCCESS_MODIFY,		/* Successful rule modification */
	SP_MAPDB_UPDATE_RESULT_LAST,
};

/*
 * spm_ae_type
 *	enum values of ae type selected by user
 */
enum spm_ae_type {
	SPM_AE_TYPE_DEFAULT,				/* selected ae type default. */
	SPM_AE_TYPE_PPE,				/* selected ae type PPE. */
	SPM_AE_TYPE_SFE,				/* selected ae type SFE. */
	SPM_AE_TYPE_PPE_DS,				/* selected ae type PPE-DS. */
	SPM_AE_TYPE_PPE_VP,				/* selected ae type PPE-VP. */
	SPM_AE_TYPE_NONE,				/* selected NONE ae type. */
};

/*
 * spm_classifier_type
 *	enum values of ae type selected by user
 */
enum spm_classifier_type {
 	SPM_RULE_TYPE_SAWF_INVALID,	/* For invalid type */
 	SPM_RULE_TYPE_MESH,		/* For non-sawf rule */
        SPM_RULE_TYPE_SAWF,		/* For SAWF rule */
        SPM_RULE_TYPE_SCS,		/* For SCS rule */
        SPM_RULE_TYPE_MSCS,		/* For MSCS rule */
        SPM_RULE_TYPE_SAWF_SCS,		/* For SAWF-SCS rule type */
        SPM_RULE_TYPE_SAWF_IFLI,	/* For IFLI rule type */
};

typedef struct sp_rule
{
	/*
         *flag to specify which rule parameters are present
         */
        uint32_t flags;
	/*
	 *flags extended to specify which rule parameters are present
	 */
        uint32_t flags_ext;
	/*
	 * Sense flags set by the MESH Classifier Type.
	 */
	uint32_t mesh_sense_flags_in;
	uint32_t attr_id;
	/*
         *Rule operation to be performed
	 */
	uint8_t add_del_rule;
	uint8_t precedence;
	/*
         * The value of rule_output determines how to set the pcp value
         * to be marked in the matched packet.
         */
	uint8_t rule_output;
	uint8_t user_priority;
	/*
         * Source mac address
         */
        u_int8_t src_mac[ETH_ALEN];
        /*
         * Destination mac address
         */
        u_int8_t dst_mac[ETH_ALEN];
        /*
         * Source ipv4 address
         */
        uint32_t src_ipv4_addr;
        /*
         * Destination ipv4 address
         */
        uint32_t dst_ipv4_addr;
        /*
         * Destination ipv6 address
         */
        struct in6_addr src_ipv6_addr;
        /*
         * Destination ipv6 address
         */
        struct in6_addr dst_ipv6_addr;
	/*
         * Source port number
         */
	uint16_t src_port;
	/*
        * Destination port number
        */
	uint16_t dst_port;
	/*
         * Protocol number
         */
	uint8_t proto_num;
	/*
         * VLAN id
         */
	uint16_t vlan_id;
	/*
         * DSCP value
         */
	uint8_t dscp;
	/*
	 * DSCP remark
	 */
	uint8_t dscp_remark;
	/*
         * Vlan Priority
         */
	uint8_t vlan_pcp;
	/*
         * Vlan Priority remark
         */
	uint8_t vlan_pcp_remark;
	/*
         * Service class id
         */
	uint8_t service_class_id;
	/*
	 * Source ipv4 address mask
	 */
	uint32_t src_ipv4_addr_mask;
	/*
	 * Destination ipv4 address mask
	 */
	uint32_t dst_ipv4_addr_mask;
	/*
	 * Filter value
	 */
	uint32_t filter_value;
	/*
	 * Filter mask
	 */
	uint32_t filter_mask;
	/*
	 * Source ipv6 address mask
	 */
	struct in6_addr src_ipv6_addr_mask;
	/*
	 * Destination ipv6 address mask
	 */
	struct in6_addr dst_ipv6_addr_mask;
	/*
	 * IP Version type
	 */
	uint8_t ip_version_type;
	/*
	 * Classifier type
	 */
	uint8_t classifier_type;
	/*
	 * Interface Index
	 */
	uint8_t ifindex;
	/*
	 * Source port range start
	 */
	uint16_t src_port_range_start;
	/*
	 * Source port range end
	 */
	uint16_t src_port_range_end;
	/*
	 * Destination port range start
	 */
	uint16_t dst_port_range_start;
	/*
	 * Destination port range end
	 */
	uint16_t dst_port_range_end;
	/*
	 * acceleration engine type
	 */
	enum spm_ae_type ae_type;
	/*
	 * Source Interface
	 */
	char src_iface[IFNAMSIZ];
	/*
	 * Destination Interface
	 */
	char dst_iface[IFNAMSIZ];
	/*
	 * Service interval specified in msec
	 * This is min latency expectation and is used
	 * by Wi-Fi FW for peer tid queue scheduling
	 */
	uint8_t service_interval_dl;
	uint8_t service_interval_ul;
	/*
	 * Burst size specified in bytes
	 * This is used by Wi-Fi FW for peer tid queue
	 * scheduling
	 */
	uint32_t burst_size_dl;
	uint32_t burst_size_ul;
	/*
	 * MSCS tid Bitmap
	 */
	uint8_t mscs_tid_bitmap;
}sp_rule;

/*
 * sal_rule_socket
 * Structure containing necessary information to communicate with sal rule.
 */
struct sal_rule_socket {
	struct nl_sock *sk;
	struct sp_rule rule;
	int family_id;
};
/*
 * sal_rule_config()
 * API used to send the rule to spm kernel module
 * All the rule parameters present in the struct sp_rule
 * are added to the netlink message
 */
int sal_rule_config(struct sal_rule_socket *rs);

/*
* sal_rule_query()
* API used to send the query request to spm kernel module
* only Attribute ID and ADD_DELETE_RULE type are required
* The callback gets the parameters for rule that matches the rule id
*/
int sal_rule_query(struct sal_rule_socket *rs, struct sp_rule *query_rule);

/*
* sal_rule_flush()
* API used to flush rules from spm database based on classifier_type
* only classifier type is required
*/
int sal_rule_flush(struct sal_rule_socket *rs, uint8_t classifier_type);

/*
 * sal_rule_init()
 * To initialize the generic netlink socket with spm family
 */
struct sal_rule_socket *sal_rule_init();

/*
 * sal_rule_deinit()
 * To delete the generic netlink socket
 */
int sal_rule_deinit(struct sal_rule_socket *rs);
#endif
