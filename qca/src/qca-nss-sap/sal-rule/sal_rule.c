/*
 * Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/in6.h>
#include <arpa/inet.h>
#include "sal_rule.h"

#define GENL_FAMILY_NAME "spm"
#define ETH_ALEN 6
#define IPV6_LEN 4
#define SPM_CMD_RULE_DELETE 0


/*
* sal_rule_query()
* API used to send the query request to spm kernel module
* only Attribute ID and ADD_DELETE_RULE type are required
*/
int sal_rule_query(struct sal_rule_socket *rs, struct sp_rule *query_rule)
{
	struct nl_msg* msg;
	int err = 0;

	msg = nlmsg_alloc();
	if (!msg) {
		printf("failed to allocate netlink message\n");
		return -1;
	}

	if(!genlmsg_put(msg, 0, 0, rs->family_id, 0, 0, SPM_CMD_RULE_QUERY, 0)) {
		printf("adding cmd failed\n");
		goto netlink_free_error;
	}

	err = nla_put_u32(msg, SP_GNL_ATTR_ID, rs->rule.attr_id);
	if (err) {
		printf("SP_GNL_ATTR_ID failed to put nl string!\n");
		goto netlink_free_error;
	}

	err = nla_put_u8(msg, SP_GNL_ATTR_ADD_DELETE_RULE, rs->rule.add_del_rule);
	if (err) {
		printf("SP_GNL_ATTR_ADD_DELETE_RULE failed to put\n");
		goto netlink_free_error;
	}

	err = nl_send_auto(rs->sk, msg);
	if (err < 0) {
		printf("failed to send nl message!\n");
		goto netlink_free_error;
	}

	err = nl_recvmsgs_default(rs->sk);
	if (err) {
		printf("nl_recvmsgs_default returned with error %i", err);
	}

	query_rule->flags = rs->rule.flags;
	query_rule->flags_ext = rs->rule.flags_ext;
	query_rule->attr_id = rs->rule.attr_id;
	query_rule->precedence = rs->rule.precedence;
	query_rule->rule_output = rs->rule.rule_output;
	query_rule->user_priority = rs->rule.user_priority;
	query_rule->src_port = rs->rule.src_port;
	query_rule->dst_port = rs->rule.dst_port;
	query_rule->proto_num = rs->rule.proto_num;
	query_rule->vlan_id = rs->rule.vlan_id;
	query_rule->dscp = rs->rule.dscp;
	query_rule->dscp_remark = rs->rule.dscp_remark;
	query_rule->vlan_pcp = rs->rule.vlan_pcp;
	query_rule->vlan_pcp_remark = rs->rule.vlan_pcp_remark;
	query_rule->service_class_id = rs->rule.service_class_id;
	query_rule->ip_version_type = rs->rule.ip_version_type;
	query_rule->classifier_type = rs->rule.classifier_type;
	query_rule->filter_value = rs->rule.filter_value;
	query_rule->filter_mask = rs->rule.filter_mask;
	memcpy(&query_rule->src_mac, &rs->rule.src_mac, ETH_ALEN);
	memcpy(&query_rule->dst_mac, &rs->rule.dst_mac, ETH_ALEN);
	memcpy(&query_rule->src_ipv4_addr, &rs->rule.src_ipv4_addr, sizeof(uint32_t));
	memcpy(&query_rule->dst_ipv4_addr, &rs->rule.dst_ipv4_addr, sizeof(uint32_t));
	memcpy(&query_rule->src_ipv6_addr, &rs->rule.src_ipv6_addr, sizeof(struct in6_addr));
	memcpy(&query_rule->dst_ipv6_addr, &rs->rule.dst_ipv6_addr, sizeof(struct in6_addr));
	memcpy(&query_rule->src_ipv4_addr_mask, &rs->rule.src_ipv4_addr_mask, sizeof(uint32_t));
	memcpy(&query_rule->dst_ipv4_addr_mask, &rs->rule.dst_ipv4_addr_mask, sizeof(uint32_t));
	memcpy(&query_rule->src_ipv6_addr_mask, &rs->rule.src_ipv6_addr_mask, sizeof(struct in6_addr));
	memcpy(&query_rule->dst_ipv6_addr_mask, &rs->rule.dst_ipv6_addr_mask, sizeof(struct in6_addr));
	query_rule->ifindex = rs->rule.ifindex;
	query_rule->src_port_range_start = rs->rule.src_port_range_start;
	query_rule->src_port_range_end = rs->rule.src_port_range_end;
	query_rule->dst_port_range_start = rs->rule.dst_port_range_start;
	query_rule->dst_port_range_end = rs->rule.dst_port_range_end;
	query_rule->ae_type = rs->rule.ae_type;
	memcpy(&query_rule->src_iface, &rs->rule.src_iface, IFNAMSIZ);
	memcpy(&query_rule->dst_iface, &rs->rule.dst_iface, IFNAMSIZ);
	goto netlink_free;

netlink_free_error:
	nlmsg_free(msg);
	return -1;

netlink_free:
	nlmsg_free(msg);
	return 0;
}

/*
 * sal_rule_callback()
 * Netlink callback function for query message
 */
static int sal_rule_cb(struct nl_msg *msg, void* arg)
{
	struct nlattr *attr[SP_GNL_ATTR_MAX+1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct sal_rule_socket *rs = (struct sal_rule_socket *)arg;

	nla_parse(attr, SP_GNL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
					genlmsg_attrlen(gnlh, 0), NULL);

	if (attr[SP_GNL_ATTR_CLASSIFIER_TYPE]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE;
		rs->rule.classifier_type = nla_get_u8(attr[SP_GNL_ATTR_CLASSIFIER_TYPE]);
	}
	if (attr[SP_GNL_ATTR_ID]) {
		rs->rule.attr_id = nla_get_u32(attr[SP_GNL_ATTR_ID]);
	}
	if (attr[SP_GNL_ATTR_ADD_DELETE_RULE]) {
		rs->rule.add_del_rule = nla_get_u8(attr[SP_GNL_ATTR_ADD_DELETE_RULE]);
	}
	if (attr[SP_GNL_ATTR_RULE_PRECEDENCE]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_PRECEDENCE;
		rs->rule.precedence = nla_get_u8(attr[SP_GNL_ATTR_RULE_PRECEDENCE]);
	}
	if (attr[SP_GNL_ATTR_RULE_OUTPUT]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_RULE_OUTPUT;
		rs->rule.rule_output = nla_get_u8(attr[SP_GNL_ATTR_RULE_OUTPUT]);
	}
	if (attr[SP_GNL_ATTR_USER_PRIORITY]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_USER_PRIORITY;
		rs->rule.user_priority = nla_get_u8(attr[SP_GNL_ATTR_USER_PRIORITY]);
	}
	if (attr[SP_GNL_ATTR_SERVICE_CLASS_ID]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID;
		rs->rule.service_class_id = nla_get_u8(attr[SP_GNL_ATTR_SERVICE_CLASS_ID]);
	}
	if (attr[SP_GNL_ATTR_SRC_PORT]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT;
		rs->rule.src_port = nla_get_u16(attr[SP_GNL_ATTR_SRC_PORT]);
	}
	if (attr[SP_GNL_ATTR_DST_PORT]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT;
		rs->rule.dst_port = nla_get_u16(attr[SP_GNL_ATTR_DST_PORT]);
	}
	if (attr[SP_GNL_ATTR_SRC_MAC]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_MAC;
		memcpy(&rs->rule.src_mac, nla_data(attr[SP_GNL_ATTR_SRC_MAC]), ETH_ALEN);
	}
	if (attr[SP_GNL_ATTR_DST_MAC]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_MAC;
		memcpy(&rs->rule.dst_mac, nla_data(attr[SP_GNL_ATTR_DST_MAC]), ETH_ALEN);
	}
	if (attr[SP_GNL_ATTR_IP_VERSION_TYPE]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_IP_VERSION_TYPE;
		rs->rule.ip_version_type = nla_get_u8(attr[SP_GNL_ATTR_IP_VERSION_TYPE]);
	}
	if (attr[SP_GNL_ATTR_SRC_IPV4_ADDR]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV4;
		memcpy(&rs->rule.src_ipv4_addr, nla_data(attr[SP_GNL_ATTR_SRC_IPV4_ADDR]), sizeof(uint32_t));
	}
	if (attr[SP_GNL_ATTR_DST_IPV4_ADDR]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV4;
		memcpy(&rs->rule.dst_ipv4_addr, nla_data(attr[SP_GNL_ATTR_DST_IPV4_ADDR]), sizeof(uint32_t));
	}
	if (attr[SP_GNL_ATTR_SRC_IPV4_ADDR_MASK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV4_MASK;
		memcpy(&rs->rule.src_ipv4_addr_mask, nla_data(attr[SP_GNL_ATTR_SRC_IPV4_ADDR_MASK]), sizeof(uint32_t));
	}
	if (attr[SP_GNL_ATTR_DST_IPV4_ADDR_MASK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV4_MASK;
		memcpy(&rs->rule.dst_ipv4_addr_mask, nla_data(attr[SP_GNL_ATTR_DST_IPV4_ADDR_MASK]), sizeof(uint32_t));
	}
	if (attr[SP_GNL_ATTR_SRC_IPV6_ADDR]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV6;
		memcpy(&rs->rule.src_ipv6_addr, nla_data(attr[SP_GNL_ATTR_SRC_IPV6_ADDR]), sizeof(struct in6_addr));
	}
	if (attr[SP_GNL_ATTR_DST_IPV6_ADDR]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV6;
		memcpy(&rs->rule.dst_ipv6_addr, nla_data(attr[SP_GNL_ATTR_DST_IPV6_ADDR]), sizeof(struct in6_addr));
	}
	if (attr[SP_GNL_ATTR_SRC_IPV6_ADDR_MASK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV6_MASK;
		memcpy(&rs->rule.src_ipv6_addr_mask, nla_data(attr[SP_GNL_ATTR_SRC_IPV6_ADDR_MASK]), sizeof(struct in6_addr));
	}
	if (attr[SP_GNL_ATTR_DST_IPV6_ADDR_MASK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV6_MASK;
		memcpy(&rs->rule.dst_ipv6_addr_mask, nla_data(attr[SP_GNL_ATTR_DST_IPV6_ADDR_MASK]), sizeof(struct in6_addr));
	}
	if (attr[SP_GNL_ATTR_PROTOCOL_NUMBER]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_PROTO_NUM;
		rs->rule.proto_num = nla_get_u8(attr[SP_GNL_ATTR_PROTOCOL_NUMBER]);
	}
	if (attr[SP_GNL_ATTR_VLAN_ID]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_VLAN_ID;
		rs->rule.vlan_id = nla_get_u16(attr[SP_GNL_ATTR_VLAN_ID]);
	}
	if (attr[SP_GNL_ATTR_DSCP]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DSCP;
		rs->rule.dscp = nla_get_u8(attr[SP_GNL_ATTR_DSCP]);
	}
	if (attr[SP_GNL_ATTR_DSCP_REMARK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DSCP_REMARK;
		rs->rule.dscp_remark = nla_get_u8(attr[SP_GNL_ATTR_DSCP_REMARK]);
	}
	if (attr[SP_GNL_ATTR_VLAN_PCP]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_VLAN_PCP;
		rs->rule.vlan_pcp = nla_get_u8(attr[SP_GNL_ATTR_VLAN_PCP]);
	}
	if (attr[SP_GNL_ATTR_VLAN_PCP_REMARK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK;
		rs->rule.vlan_pcp_remark = nla_get_u8(attr[SP_GNL_ATTR_VLAN_PCP_REMARK]);
	}
	if (attr[SP_GNL_ATTR_FILTER_VALUE]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SPI;
		rs->rule.filter_value = nla_get_u32(attr[SP_GNL_ATTR_FILTER_VALUE]);
	}
	if (attr[SP_GNL_ATTR_FILTER_MASK]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SPI;
		rs->rule.filter_mask = nla_get_u32(attr[SP_GNL_ATTR_FILTER_MASK]);
	}
	if (attr[SP_GNL_ATTR_IFINDEX]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_IFINDEX;
		rs->rule.ifindex = nla_get_u8(attr[SP_GNL_ATTR_IFINDEX]);
	}
	if (attr[SP_GNL_ATTR_SRC_PORT_RANGE_START]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_START;
		rs->rule.src_port_range_start = nla_get_u16(attr[SP_GNL_ATTR_SRC_PORT_RANGE_START]);
	}
	if (attr[SP_GNL_ATTR_SRC_PORT_RANGE_END]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_END;
		rs->rule.src_port_range_end = nla_get_u16(attr[SP_GNL_ATTR_SRC_PORT_RANGE_END]);
	}
	if (attr[SP_GNL_ATTR_DST_PORT_RANGE_START]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT_RANGE_START;
		rs->rule.dst_port_range_start = nla_get_u16(attr[SP_GNL_ATTR_DST_PORT_RANGE_START]);
	}
	if (attr[SP_GNL_ATTR_DST_PORT_RANGE_END]) {
		rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT_RANGE_END;
		rs->rule.dst_port_range_end = nla_get_u16(attr[SP_GNL_ATTR_DST_PORT_RANGE_END]);
	}
	if (attr[SP_GNL_ATTR_AE_TYPE]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MATCH_AE_TYPE;
		rs->rule.ae_type = nla_get_u8(attr[SP_GNL_ATTR_AE_TYPE]);
	}
	if (attr[SP_GNL_ATTR_SRC_IFACE]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MATCH_SRC_IFACE;
		memcpy(&rs->rule.src_iface, nla_data(attr[SP_GNL_ATTR_SRC_IFACE]), IFNAMSIZ);
	}
	if (attr[SP_GNL_ATTR_DST_IFACE]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MATCH_DST_IFACE;
		memcpy(&rs->rule.dst_iface, nla_data(attr[SP_GNL_ATTR_DST_IFACE]), IFNAMSIZ);
	}
	if (attr[SP_GNL_ATTR_SVC_INTERVAL_DL]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_SVC_INTERVAL_DL;
		rs->rule.service_interval_dl = nla_get_u8(attr[SP_GNL_ATTR_SVC_INTERVAL_DL]);
	}
	if (attr[SP_GNL_ATTR_SVC_INTERVAL_UL]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_SVC_INTERVAL_UL;
		rs->rule.service_interval_ul = nla_get_u8(attr[SP_GNL_ATTR_SVC_INTERVAL_UL]);
	}
	if (attr[SP_GNL_ATTR_BURST_SIZE_DL]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_BURST_SIZE_DL;
		rs->rule.burst_size_dl = nla_get_u32(attr[SP_GNL_ATTR_BURST_SIZE_DL]);
	}
	if (attr[SP_GNL_ATTR_BURST_SIZE_UL]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_BURST_SIZE_UL;
		rs->rule.burst_size_ul = nla_get_u32(attr[SP_GNL_ATTR_BURST_SIZE_UL]);
	}
	if (attr[SP_GNL_ATTR_TID_BITMAP]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MSCS_TID_BITMAP;
		rs->rule.mscs_tid_bitmap = nla_get_u8(attr[SP_GNL_ATTR_TID_BITMAP]);
	}
	if (attr[SP_GNL_ATTR_SENSE_MESH_FLAG_IN]) {
		rs->rule.flags_ext |= SP_RULE_FLAG_EXT_SENSE_MESH_FLAGS_IN;
		rs->rule.mesh_sense_flags_in = nla_get_u32(attr[SP_GNL_ATTR_SENSE_MESH_FLAG_IN]);
	}
	return 0;
}

/*
 * sal_rule_create_nl_sock()
 * Creates a genric netlink socket with SPM family
 */
static struct sal_rule_socket *sal_rule_create_nl_sock()
{
	struct sal_rule_socket *rs = malloc(sizeof(struct sal_rule_socket));
	if (!rs) {
		printf("Failed to allocate memory for sal rule socket.\n");
		return NULL;
	}

	rs->sk = nl_socket_alloc();
	if(!rs->sk) {
		printf("Unable to allocate nl socket!\n");
		nl_socket_free(rs->sk);
		free(rs);
		return NULL;
	}

	/* connect to genl */
	if (genl_connect(rs->sk)) {
		printf("Unable to connect to genl!\n");
		nl_socket_free(rs->sk);
		free(rs);
		return NULL;
	}

	/* resolve the generic nl family id*/
	rs->family_id = genl_ctrl_resolve(rs->sk, GENL_FAMILY_NAME);
	if(rs->family_id < 0){
		printf("Unable to resolve family name!\n");
		nl_socket_free(rs->sk);
		free(rs);
		return NULL;
	}

	/* disable seq checks on multicast sockets */
	nl_socket_modify_cb(rs->sk, NL_CB_MSG_IN, NL_CB_CUSTOM, sal_rule_cb, rs);
	nl_socket_disable_auto_ack(rs->sk);
	nl_socket_disable_seq_check(rs->sk);

	return rs;
}

/*
* API used to send the rule to spm kernel module
* All the rule parameters present in the struct sp_rule
* are added to the netlink message
*/
int sal_rule_config(struct sal_rule_socket *rs)
{
	struct nl_msg* msg;
	int err = 0;
	int rule_id = rs->rule.attr_id;

	msg = nlmsg_alloc();
	if (!msg) {
		printf("failed to allocate netlink message\n");
		return -1;
	}

	if(!genlmsg_put(msg, 0, 0, rs->family_id, 0, 0, SPM_CMD_RULE_ACTION, 0)) {
		printf("adding cmd failed\n");
		goto netlink_free_error;
	}
	err = nla_put_u32(msg, SP_GNL_ATTR_ID, rs->rule.attr_id);
	if (err) {
		printf("SP_GNL_ATTR_ID failed to put nl string!\n");
		goto netlink_free_error;
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE) {
		err = nla_put_u8(msg, SP_GNL_ATTR_CLASSIFIER_TYPE, rs->rule.classifier_type);
		if (err) {
			printf("SP_GNL_ATTR_CLASSIFIER_TYPE failed to put\n");
			goto netlink_free_error;
		}
	}
	err = nla_put_u8(msg, SP_GNL_ATTR_ADD_DELETE_RULE, rs->rule.add_del_rule);
	if (err) {
		printf("SP_GNL_ATTR_ADD_DELETE_RULE failed to put\n");
		goto netlink_free_error;
	}
	if (rs->rule.add_del_rule == SPM_CMD_RULE_DELETE) {
		err = nl_send_auto(rs->sk, msg);
		if (err < 0) {
			printf("failed to send nl message!\n");
			goto netlink_free_error;
		}
		goto config_success;
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_PRECEDENCE){
	err = nla_put_u8(msg, SP_GNL_ATTR_RULE_PRECEDENCE, rs->rule.precedence);
		if (err) {
			printf("SP_GNL_ATTR_RULE_PRECEDENCE failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_RULE_OUTPUT) {
		err = nla_put_u8(msg, SP_GNL_ATTR_RULE_OUTPUT, rs->rule.rule_output);
		if (err) {
			printf("SP_GNL_ATTR_RULE_OUTPUT failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_PROTO_NUM) {
		err = nla_put_u8(msg, SP_GNL_ATTR_PROTOCOL_NUMBER, rs->rule.proto_num);
		if (err) {
			printf("SP_GNL_ATTR_PROTOCOL_NUMBER failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_VLAN_ID) {
		err = nla_put_u16(msg, SP_GNL_ATTR_VLAN_ID, rs->rule.vlan_id);
		if (err) {
			printf("SP_GNL_ATTR_VLAN_ID failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DSCP) {
		err = nla_put_u8(msg, SP_GNL_ATTR_DSCP, rs->rule.dscp);
		if (err) {
			printf("SP_GNL_ATTR_DSCP failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DSCP_REMARK) {
		err = nla_put_u8(msg, SP_GNL_ATTR_DSCP_REMARK, rs->rule.dscp_remark);
		if (err) {
			printf("SP_GNL_ATTR_DSCP_REMARK failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_VLAN_PCP) {
		err = nla_put_u8(msg, SP_GNL_ATTR_VLAN_PCP, rs->rule.vlan_pcp);
		if (err) {
			printf("SP_GNL_ATTR_VLAN_PCP failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK) {
		err = nla_put_u8(msg, SP_GNL_ATTR_VLAN_PCP_REMARK, rs->rule.vlan_pcp_remark);
		if (err) {
			printf("SP_GNL_ATTR_VLAN_PCP_REMARK failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_USER_PRIORITY) {
		err = nla_put_u8(msg, SP_GNL_ATTR_USER_PRIORITY, rs->rule.user_priority);
		if (err) {
			printf("SP_GNL_ATTR_USER_PRIORITY failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_MAC) {
		err = nla_put(msg, SP_GNL_ATTR_SRC_MAC, ETH_ALEN, rs->rule.src_mac);
		if (err) {
			printf("SP_GNL_ATTR_SRC_MAC failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DST_MAC) {
		err = nla_put(msg, SP_GNL_ATTR_DST_MAC, ETH_ALEN, rs->rule.dst_mac);
		if (err) {
			printf("SP_GNL_ATTR_DST_MAC failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_IP_VERSION_TYPE) {
		err = nla_put_u8(msg, SP_GNL_ATTR_IP_VERSION_TYPE, rs->rule.ip_version_type);
		if (err) {
			printf("SP_GNL_ATTR_IP_VERSION_TYPE failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV4) {
		err = nla_put_u32(msg, SP_GNL_ATTR_SRC_IPV4_ADDR, rs->rule.src_ipv4_addr);
		if (err) {
			printf("SP_GNL_ATTR_SRC_IPV4_ADDR failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DST_IPV4) {
		err = nla_put_u32(msg, SP_GNL_ATTR_DST_IPV4_ADDR, rs->rule.dst_ipv4_addr);
		if (err) {
			printf("SP_GNL_ATTR_DST_IPV4_ADDR failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV4_MASK) {
		err = nla_put_u32(msg, SP_GNL_ATTR_SRC_IPV4_ADDR_MASK, rs->rule.src_ipv4_addr_mask);
		if (err) {
			printf("SP_GNL_ATTR_SRC_IPV4_ADDR_MASK failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DST_IPV4_MASK) {
		err = nla_put_u32(msg, SP_GNL_ATTR_DST_IPV4_ADDR_MASK, rs->rule.dst_ipv4_addr_mask);
		if (err) {
			printf("SP_GNL_ATTR_DST_IPV4_ADDR_MASK failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV6) {
		err = nla_put(msg, SP_GNL_ATTR_SRC_IPV6_ADDR, sizeof(struct in6_addr), &rs->rule.src_ipv6_addr);
		if (err) {
			printf("SP_GNL_ATTR_SRC_IPV6_ADDR failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DST_IPV6){
		err = nla_put(msg, SP_GNL_ATTR_DST_IPV6_ADDR, sizeof(struct in6_addr), &rs->rule.dst_ipv6_addr);
		if (err) {
			printf("SP_GNL_ATTR_DST_IPV6_ADDR failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV6_MASK) {
		err = nla_put(msg, SP_GNL_ATTR_SRC_IPV6_ADDR_MASK, sizeof(struct in6_addr), &rs->rule.src_ipv6_addr_mask);
		if (err) {
			printf("SP_GNL_ATTR_SRC_IPV6_ADDR_MASK failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DST_IPV6_MASK){
		err = nla_put(msg, SP_GNL_ATTR_DST_IPV6_ADDR_MASK, sizeof(struct in6_addr), &rs->rule.dst_ipv6_addr_mask);
		if (err) {
			printf("SP_GNL_ATTR_DST_IPV6_ADDR_MASK failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID) {
		err = nla_put_u8(msg, SP_GNL_ATTR_SERVICE_CLASS_ID, rs->rule.service_class_id);
		if (err) {
			printf("SP_GNL_ATTR_SERVICE_CLASS_ID failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_PORT) {
		err = nla_put_u16(msg, SP_GNL_ATTR_SRC_PORT, rs->rule.src_port);
		if (err) {
			printf("SP_GNL_ATTR_SRC_PORT failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_DST_PORT) {
		err = nla_put_u16(msg, SP_GNL_ATTR_DST_PORT, rs->rule.dst_port);
		if (err) {
			printf("SP_GNL_ATTR_DST_PORT failed to put\n");
			goto netlink_free_error;
		}
	}

	if(rs->rule.flags & SP_RULE_FLAG_MATCH_SPI) {
		err = nla_put_u32(msg, SP_GNL_ATTR_FILTER_VALUE, rs->rule.filter_value);
		if (err) {
			printf("SP_GNL_ATTR_FILTER_VALUE failed to put\n");
			goto netlink_free_error;
		}

		err = nla_put_u32(msg, SP_GNL_ATTR_FILTER_MASK, rs->rule.filter_mask);
		if (err) {
			printf("SP_GNL_ATTR_FILTER_MASK failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags & SP_RULE_FLAG_MATCH_IFINDEX) {
		err = nla_put_u8(msg, SP_GNL_ATTR_IFINDEX, rs->rule.ifindex);
		if (err) {
			printf("SP_GNL_ATTR_IFINDEX failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_START) {
		err = nla_put_u16(msg, SP_GNL_ATTR_SRC_PORT_RANGE_START, rs->rule.src_port_range_start);
		if (err) {
			printf("SP_GNL_ATTR_SRC_PORT_RANGE_START failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags & SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_END) {
		err = nla_put_u16(msg, SP_GNL_ATTR_SRC_PORT_RANGE_END, rs->rule.src_port_range_end);
		if (err) {
			printf("SP_GNL_ATTR_SRC_PORT_RANGE_END failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags & SP_RULE_FLAG_MATCH_DST_PORT_RANGE_START) {
		err = nla_put_u16(msg, SP_GNL_ATTR_DST_PORT_RANGE_START, rs->rule.dst_port_range_start);
		if (err) {
			printf("SP_GNL_ATTR_DST_PORT_RANGE_START failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags & SP_RULE_FLAG_MATCH_DST_PORT_RANGE_END) {
		err = nla_put_u16(msg, SP_GNL_ATTR_DST_PORT_RANGE_END, rs->rule.dst_port_range_end);
		if (err) {
			printf("SP_GNL_ATTR_DST_PORT_RANGE_END failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_MATCH_AE_TYPE) {
		err = nla_put_u8(msg, SP_GNL_ATTR_AE_TYPE, rs->rule.ae_type);
		if (err) {
			printf("SP_GNL_ATTR_AE_TYPE failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags_ext & SP_RULE_FLAG_EXT_MATCH_SRC_IFACE){
		err = nla_put(msg, SP_GNL_ATTR_SRC_IFACE, IFNAMSIZ, &rs->rule.src_iface);
		if (err) {
			printf("SP_GNL_ATTR_SRC_IFACE failed to put\n");
			goto netlink_free_error;
		}
	}
	if(rs->rule.flags_ext & SP_RULE_FLAG_EXT_MATCH_DST_IFACE){
		err = nla_put(msg, SP_GNL_ATTR_DST_IFACE, IFNAMSIZ, &rs->rule.dst_iface);
		if (err) {
			printf("SP_GNL_ATTR_DST_IFACE failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_SVC_INTERVAL_DL) {
		err = nla_put_u8(msg, SP_GNL_ATTR_SVC_INTERVAL_DL, rs->rule.service_interval_dl);
		if (err) {
			printf("SP_GNL_ATTR_SVC_INTERVAL_DL failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_SVC_INTERVAL_UL) {
		err = nla_put_u8(msg, SP_GNL_ATTR_SVC_INTERVAL_UL, rs->rule.service_interval_ul);
		if (err) {
			printf("SP_GNL_ATTR_SVC_INTERVAL_UL failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_BURST_SIZE_DL) {
		err = nla_put_u32(msg, SP_GNL_ATTR_BURST_SIZE_DL, rs->rule.burst_size_dl);
		if (err) {
			printf("SP_GNL_ATTR_BURST_SIZE_DL failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_BURST_SIZE_UL) {
		err = nla_put_u32(msg, SP_GNL_ATTR_BURST_SIZE_UL, rs->rule.burst_size_ul);
		if (err) {
			printf("SP_GNL_ATTR_BURST_SIZE_UL failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_MSCS_TID_BITMAP) {
		err = nla_put_u8(msg, SP_GNL_ATTR_TID_BITMAP, rs->rule.mscs_tid_bitmap);
		if (err) {
			printf("SP_GNL_ATTR_TID_BITMAP failed to put\n");
			goto netlink_free_error;
		}
	}

	if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_SENSE_MESH_FLAGS_IN) {
		err = nla_put_u32(msg, SP_GNL_ATTR_SENSE_MESH_FLAG_IN, rs->rule.mesh_sense_flags_in);
		if (err) {
			printf("SP_GNL_ATTR_SENSE_MESH_FLAG_IN failed to put\n");
			goto netlink_free_error;
		}
	}
	err = nl_send_auto(rs->sk, msg);
	printf("Configure SP rule via NL message.\n");

	if (err < 0) {
		printf("failed to send nl message!\n");
		goto netlink_free_error;
	}

config_success:

	/*
	 * Catch the response of rule create message sent to SPM and notify if the rule is
	 * successfully created or not. In case of 11be SCS, return the rule id of the same.
	 */
	err = nl_recvmsgs_default(rs->sk);
	if (err) {
		printf("nl_recvmsgs_default returned with error %i\n", err);
		goto netlink_free_error;
	}

	/*
	 * Checking if the rule operartion is success or
	 * fail in spm database.
	 */
	switch (rs->rule.add_del_rule) {
	case SP_MAPDB_UPDATE_RESULT_SUCCESS_DELETE:
	case SP_MAPDB_UPDATE_RESULT_SUCCESS_ADD:
	case SP_MAPDB_UPDATE_RESULT_SUCCESS_MODIFY:
		printf("Rule operation successful\n");
		goto netlink_free;
	default:
		printf("Rule operation failed with err_id = %d\n", rs->rule.add_del_rule);
		goto operation_failed_error;
	}

operation_failed_error:
	nlmsg_free(msg);
	return rule_id;
netlink_free_error:
	nlmsg_free(msg);
        return -1;
netlink_free:
	nlmsg_free(msg);
	return 0;
}

/*
* sal_rule_flush()
* API used to flush all rules from spm database
*/
int sal_rule_flush(struct sal_rule_socket *rs, uint8_t classifier_type)
{
	struct nl_msg* msg;
	int err = 0;

	msg = nlmsg_alloc();
	if (!msg) {
		printf("failed to allocate netlink message\n");
		return -1;
	}

	if (!genlmsg_put(msg, 0, 0, rs->family_id, 0, 0, SPM_CMD_RULE_FLUSH, 0)) {
		printf("Adding SPM_CMD_RULE_FLUSH failed\n");
		goto netlink_free_error;
	}

	err = nla_put_u8(msg, SP_GNL_ATTR_CLASSIFIER_TYPE, classifier_type);
	if (err) {
		printf("SP_GNL_ATTR_CLASSIFIER_TYPE failed to put classifier type\n");
		goto netlink_free_error;
	}

	err = nl_send_auto(rs->sk, msg);
	if (err < 0) {
		printf("failed to send netlink message!\n");
		goto netlink_free_error;
	}

	nlmsg_free(msg);
	return 0;

netlink_free_error:
	nlmsg_free(msg);
	return -1;
}

struct sal_rule_socket * sal_rule_init()
{
	return(sal_rule_create_nl_sock());
}

int sal_rule_deinit(struct sal_rule_socket *rs)
{
	nl_socket_free(rs->sk);
	free(rs);
	return 0;
}
