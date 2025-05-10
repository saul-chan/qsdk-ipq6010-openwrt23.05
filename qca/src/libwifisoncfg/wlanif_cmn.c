/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <net/if.h>
#include <net/ethernet.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/in6.h>
#include <arpa/inet.h>

#include "wlanif_cmn.h"

#ifdef SON_MEMORY_DEBUG

#include "qca-son-mem-debug.h"
#define QCA_MOD_INPUT QCA_MOD_LIBWIFISONCFG
#include "son-mem-debug.h"

#endif /* SON_MEMORY_DEBUG */


#define GENL_FAMILY_NAME "spm"
#define SPM_CMD_RULE_DELETE 0

#include <dbg.h>


/* For debugging support */
struct wlanifDbg_t {
    struct dbgModule *dbgModule;
} wlanifDbgS;


#define wlanifCmnDebug(level, ...) dbgf(wlanifDbgS.dbgModule,(level),__VA_ARGS__)

/*
 * API used to send the rule to spm kernel module
 * All the rule parameters present in the struct sp_rule
 * are added to the netlink message
 */
int sal_rule_config(struct sal_rule_socket * rs) {
    struct nl_msg * msg;
    int err = 0;

    msg = nlmsg_alloc();
    if (!msg) {
        wlanifCmnDebug(DBGERR,"failed to allocate netlink message\n");
        return -1;
    }

    if (!genlmsg_put(msg, 0, 0, rs -> family_id, 0, 0, SPM_CMD_RULE_ACTION, 0)) {
        wlanifCmnDebug(DBGERR,"adding cmd failed\n");
        goto netlink_free_error;
    }
    err = nla_put_u32(msg, SP_GNL_ATTR_ID, rs -> rule.attr_id);
    if (err) {
        wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_ID failed to put nl string!\n");
        goto netlink_free_error;
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE) {
        err = nla_put_u8(msg, SP_GNL_ATTR_CLASSIFIER_TYPE, rs -> rule.classifier_type);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_CLASSIFIER_TYPE failed to put\n");
            goto netlink_free_error;
        }
    }
    err = nla_put_u8(msg, SP_GNL_ATTR_ADD_DELETE_RULE, rs -> rule.add_del_rule);
    if (err) {
        wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_ADD_DELETE_RULE failed to put\n");
        goto netlink_free_error;
    }
    if (rs -> rule.add_del_rule == SPM_CMD_RULE_DELETE) {
        wlanifCmnDebug(DBGINFO, "%s:%d>  Sending sal rule config netlink message for delete rule...\n", __func__, __LINE__) ;
        err = nl_send_auto(rs -> sk, msg);
        if (err < 0) {
            wlanifCmnDebug(DBGERR,"failed to send nl message!\n");
        }
        goto netlink_free;
        return 0;
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_PRECEDENCE) {
        err = nla_put_u8(msg, SP_GNL_ATTR_RULE_PRECEDENCE, rs -> rule.precedence);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_RULE_PRECEDENCE failed to put\n");
            goto netlink_free_error;
        }
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_RULE_OUTPUT) {
        err = nla_put_u8(msg, SP_GNL_ATTR_RULE_OUTPUT, rs -> rule.rule_output);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_RULE_OUTPUT failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_PROTO_NUM) {
        err = nla_put_u8(msg, SP_GNL_ATTR_PROTOCOL_NUMBER, rs -> rule.proto_num);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_PROTOCOL_NUMBER failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_VLAN_ID) {
        err = nla_put_u16(msg, SP_GNL_ATTR_VLAN_ID, rs -> rule.vlan_id);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_VLAN_ID failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DSCP) {
        err = nla_put_u8(msg, SP_GNL_ATTR_DSCP, rs -> rule.dscp);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DSCP failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DSCP_REMARK) {
        err = nla_put_u8(msg, SP_GNL_ATTR_DSCP_REMARK, rs -> rule.dscp_remark);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DSCP_REMARK failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_VLAN_PCP) {
        err = nla_put_u8(msg, SP_GNL_ATTR_VLAN_PCP, rs -> rule.vlan_pcp);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_VLAN_PCP failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK) {
        err = nla_put_u8(msg, SP_GNL_ATTR_VLAN_PCP_REMARK, rs -> rule.vlan_pcp_remark);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_VLAN_PCP_REMARK failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_USER_PRIORITY) {
        err = nla_put_u8(msg, SP_GNL_ATTR_USER_PRIORITY, rs -> rule.user_priority);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_USER_PRIORITY failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SRC_MAC) {
        err = nla_put(msg, SP_GNL_ATTR_SRC_MAC, ETH_ALEN, rs -> rule.src_mac);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SRC_MAC failed to put\n");
            goto netlink_free_error;
        }
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DST_MAC) {
        err = nla_put(msg, SP_GNL_ATTR_DST_MAC, ETH_ALEN, rs -> rule.dst_mac);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DST_MAC failed to put\n");
            goto netlink_free_error;
        }
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_IP_VERSION_TYPE) {
        err = nla_put_u8(msg, SP_GNL_ATTR_IP_VERSION_TYPE, rs -> rule.ip_version_type);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_IP_VERSION_TYPE failed to put\n");
            goto netlink_free_error;
        }
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV4) {
        err = nla_put_u32(msg, SP_GNL_ATTR_SRC_IPV4_ADDR, rs -> rule.src_ipv4_addr);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SRC_IPV4_ADDR failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DST_IPV4) {
        err = nla_put_u32(msg, SP_GNL_ATTR_DST_IPV4_ADDR, rs -> rule.dst_ipv4_addr);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DST_IPV4_ADDR failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV4_MASK) {
        err = nla_put_u32(msg, SP_GNL_ATTR_SRC_IPV4_ADDR_MASK, rs -> rule.src_ipv4_addr_mask);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SRC_IPV4_ADDR_MASK failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DST_IPV4_MASK) {
        err = nla_put_u32(msg, SP_GNL_ATTR_DST_IPV4_ADDR_MASK, rs -> rule.dst_ipv4_addr_mask);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DST_IPV4_ADDR_MASK failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV6) {
        err = nla_put(msg, SP_GNL_ATTR_SRC_IPV6_ADDR, sizeof(struct in6_addr), & rs -> rule.src_ipv6_addr);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SRC_IPV6_ADDR failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DST_IPV6) {
        err = nla_put(msg, SP_GNL_ATTR_DST_IPV6_ADDR, sizeof(struct in6_addr), & rs -> rule.dst_ipv6_addr);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DST_IPV6_ADDR failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SRC_IPV6_MASK) {
        err = nla_put(msg, SP_GNL_ATTR_SRC_IPV6_ADDR_MASK, sizeof(struct in6_addr), & rs -> rule.src_ipv6_addr_mask);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SRC_IPV6_ADDR_MASK failed to put\n");
            goto netlink_free_error;
        }
    }
    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DST_IPV6_MASK) {
        err = nla_put(msg, SP_GNL_ATTR_DST_IPV6_ADDR_MASK, sizeof(struct in6_addr), & rs -> rule.dst_ipv6_addr_mask);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DST_IPV6_ADDR_MASK failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID) {
        err = nla_put_u8(msg, SP_GNL_ATTR_SERVICE_CLASS_ID, rs -> rule.service_class_id);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SERVICE_CLASS_ID failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SRC_PORT) {
        err = nla_put_u16(msg, SP_GNL_ATTR_SRC_PORT, rs -> rule.src_port);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SRC_PORT failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_DST_PORT) {
        err = nla_put_u16(msg, SP_GNL_ATTR_DST_PORT, rs -> rule.dst_port);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_DST_PORT failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags & SP_RULE_FLAG_MATCH_SPI) {
        err = nla_put_u32(msg, SP_GNL_ATTR_FILTER_VALUE, rs -> rule.filter_value);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_FILTER_VALUE failed to put\n");
            goto netlink_free_error;
        }

        err = nla_put_u32(msg, SP_GNL_ATTR_FILTER_MASK, rs -> rule.filter_mask);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_FILTER_MASK failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags_ext & SP_RULE_FLAG_EXT_SVC_INTERVAL_DL) {
        err = nla_put_u8(msg, SP_GNL_ATTR_SVC_INTERVAL_DL, rs -> rule.serviceIntervalDl);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SVC_INTERVAL_DL failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags_ext & SP_RULE_FLAG_EXT_SVC_INTERVAL_UL) {
        err = nla_put_u8(msg, SP_GNL_ATTR_SVC_INTERVAL_UL, rs -> rule.serviceIntervalUl);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SVC_INTERVAL_UL failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags_ext & SP_RULE_FLAG_EXT_BURST_SIZE_DL) {
        err = nla_put_u32(msg, SP_GNL_ATTR_BURST_SIZE_DL, rs -> rule.burstSizeDl);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_BURST_SIZE_DL failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags_ext & SP_RULE_FLAG_EXT_BURST_SIZE_UL) {
        err = nla_put_u32(msg, SP_GNL_ATTR_BURST_SIZE_UL, rs -> rule.burstSizeUl);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_BURST_SIZE_UL failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs -> rule.flags_ext & SP_RULE_FLAG_EXT_MSCS_TID_BITMAP) {
        err = nla_put_u8(msg, SP_GNL_ATTR_TID_BITMAP, rs -> rule.mscs_tid_bitmap);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_TID_BITMAP failed to put\n");
            goto netlink_free_error;
        }
    }

    if (rs->rule.flags_ext & SP_RULE_FLAG_EXT_MESH_FLAGS_IN) {
        err = nla_put_u32(msg, SP_GNL_ATTR_SENSE_MESH_FLAG_IN, rs -> rule.mesh_flags_in);
        if (err) {
            wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_SENSE_MESH_FLAG_IN failed to put\n");
            goto netlink_free_error;
        }
    }

    wlanifCmnDebug(DBGINFO, "%s:%d>  Sending sal rule config netlink message for add rule...\n", __func__, __LINE__) ;

    err = nl_send_auto(rs -> sk, msg);
    if (err < 0) {
        wlanifCmnDebug(DBGERR,"failed to send nl message!\n");
        goto netlink_free_error;
    }
    goto netlink_free;

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
        wlanifCmnDebug(DBGERR,"failed to allocate netlink message\n");
        return -1;
    }

    if (!genlmsg_put(msg, 0, 0, rs->family_id, 0, 0, SPM_CMD_RULE_FLUSH, 0)) {
        wlanifCmnDebug(DBGERR,"Adding SPM_CMD_RULE_FLUSH failed\n");
        goto netlink_free_error;
    }

    err = nla_put_u8(msg, SP_GNL_ATTR_CLASSIFIER_TYPE, classifier_type);
    if (err) {
        wlanifCmnDebug(DBGERR,"SP_GNL_ATTR_CLASSIFIER_TYPE failed to put classifier type\n");
        goto netlink_free_error;
    }

    wlanifCmnDebug(DBGINFO, "%s:%d>  Sending sal rule flush netlink message...\n", __func__, __LINE__) ;
    err = nl_send_auto(rs->sk, msg);
    if (err < 0) {
        wlanifCmnDebug(DBGERR,"failed to send netlink message!\n");
        goto netlink_free_error;
    }

    nlmsg_free(msg);
    return 0;

netlink_free_error:
    nlmsg_free(msg);
    return -1;
}

/*
 * sal_rule_callback()
 * Netlink callback function for query message
 */
static int sal_rule_cb(struct nl_msg * msg, void * arg) {
    struct nlattr * attr[SP_GNL_ATTR_MAX + 1];
    struct genlmsghdr * gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct sal_rule_socket * rs = (struct sal_rule_socket * ) arg;

    nla_parse(attr, SP_GNL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
        genlmsg_attrlen(gnlh, 0), NULL);

    if (attr[SP_GNL_ATTR_CLASSIFIER_TYPE]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE;
        rs -> rule.classifier_type = nla_get_u8(attr[SP_GNL_ATTR_CLASSIFIER_TYPE]);
    }
    if (attr[SP_GNL_ATTR_ID]) {
        rs -> rule.attr_id = nla_get_u32(attr[SP_GNL_ATTR_ID]);
    }
    if (attr[SP_GNL_ATTR_RULE_PRECEDENCE]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_PRECEDENCE;
        rs -> rule.precedence = nla_get_u8(attr[SP_GNL_ATTR_RULE_PRECEDENCE]);
    }
    if (attr[SP_GNL_ATTR_RULE_OUTPUT]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_RULE_OUTPUT;
        rs -> rule.rule_output = nla_get_u8(attr[SP_GNL_ATTR_RULE_OUTPUT]);
    }
    if (attr[SP_GNL_ATTR_USER_PRIORITY]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_USER_PRIORITY;
        rs -> rule.user_priority = nla_get_u8(attr[SP_GNL_ATTR_USER_PRIORITY]);
    }
    if (attr[SP_GNL_ATTR_SERVICE_CLASS_ID]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID;
        rs -> rule.service_class_id = nla_get_u8(attr[SP_GNL_ATTR_SERVICE_CLASS_ID]);
    }
    if (attr[SP_GNL_ATTR_SRC_PORT]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT;
        rs -> rule.src_port = nla_get_u16(attr[SP_GNL_ATTR_SRC_PORT]);
    }
    if (attr[SP_GNL_ATTR_DST_PORT]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT;
        rs -> rule.dst_port = nla_get_u16(attr[SP_GNL_ATTR_DST_PORT]);
    }
    if (attr[SP_GNL_ATTR_SRC_MAC]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SRC_MAC;
        memcpy( & rs -> rule.src_mac, nla_data(attr[SP_GNL_ATTR_SRC_MAC]), ETH_ALEN);
    }
    if (attr[SP_GNL_ATTR_DST_MAC]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DST_MAC;
        memcpy( & rs -> rule.dst_mac, nla_data(attr[SP_GNL_ATTR_DST_MAC]), ETH_ALEN);
    }
    if (attr[SP_GNL_ATTR_IP_VERSION_TYPE]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_IP_VERSION_TYPE;
        rs -> rule.ip_version_type = nla_get_u8(attr[SP_GNL_ATTR_IP_VERSION_TYPE]);
    }
    if (attr[SP_GNL_ATTR_SRC_IPV4_ADDR]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV4;
        memcpy( & rs -> rule.src_ipv4_addr, nla_data(attr[SP_GNL_ATTR_SRC_IPV4_ADDR]), sizeof(uint32_t));
    }
    if (attr[SP_GNL_ATTR_DST_IPV4_ADDR]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV4;
        memcpy( & rs -> rule.dst_ipv4_addr, nla_data(attr[SP_GNL_ATTR_DST_IPV4_ADDR]), sizeof(uint32_t));
    }
    if (attr[SP_GNL_ATTR_SRC_IPV4_ADDR_MASK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV4_MASK;
        memcpy( & rs -> rule.src_ipv4_addr_mask, nla_data(attr[SP_GNL_ATTR_SRC_IPV4_ADDR_MASK]), sizeof(uint32_t));
    }
    if (attr[SP_GNL_ATTR_DST_IPV4_ADDR_MASK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV4_MASK;
        memcpy( & rs -> rule.dst_ipv4_addr_mask, nla_data(attr[SP_GNL_ATTR_DST_IPV4_ADDR_MASK]), sizeof(uint32_t));
    }
    if (attr[SP_GNL_ATTR_SRC_IPV6_ADDR]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV6;
        memcpy( & rs -> rule.src_ipv6_addr, nla_data(attr[SP_GNL_ATTR_SRC_IPV6_ADDR]), sizeof(struct in6_addr));
    }
    if (attr[SP_GNL_ATTR_DST_IPV6_ADDR]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV6;
        memcpy( & rs -> rule.dst_ipv6_addr, nla_data(attr[SP_GNL_ATTR_DST_IPV6_ADDR]), sizeof(struct in6_addr));
    }
    if (attr[SP_GNL_ATTR_SRC_IPV6_ADDR_MASK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV6_MASK;
        memcpy( & rs -> rule.src_ipv6_addr_mask, nla_data(attr[SP_GNL_ATTR_SRC_IPV6_ADDR_MASK]), sizeof(struct in6_addr));
    }
    if (attr[SP_GNL_ATTR_DST_IPV6_ADDR_MASK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV6_MASK;
        memcpy( & rs -> rule.dst_ipv6_addr_mask, nla_data(attr[SP_GNL_ATTR_DST_IPV6_ADDR_MASK]), sizeof(struct in6_addr));
    }
    if (attr[SP_GNL_ATTR_PROTOCOL_NUMBER]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_PROTO_NUM;
        rs -> rule.proto_num = nla_get_u8(attr[SP_GNL_ATTR_PROTOCOL_NUMBER]);
    }
    if (attr[SP_GNL_ATTR_VLAN_ID]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_VLAN_ID;
        rs -> rule.vlan_id = nla_get_u16(attr[SP_GNL_ATTR_VLAN_ID]);
    }
    if (attr[SP_GNL_ATTR_DSCP]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DSCP;
        rs -> rule.dscp = nla_get_u8(attr[SP_GNL_ATTR_DSCP]);
    }
    if (attr[SP_GNL_ATTR_DSCP_REMARK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_DSCP_REMARK;
        rs -> rule.dscp_remark = nla_get_u8(attr[SP_GNL_ATTR_DSCP_REMARK]);
    }
    if (attr[SP_GNL_ATTR_VLAN_PCP]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_VLAN_PCP;
        rs -> rule.vlan_pcp = nla_get_u8(attr[SP_GNL_ATTR_VLAN_PCP]);
    }
    if (attr[SP_GNL_ATTR_VLAN_PCP_REMARK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK;
        rs -> rule.vlan_pcp_remark = nla_get_u8(attr[SP_GNL_ATTR_VLAN_PCP_REMARK]);
    }
    if (attr[SP_GNL_ATTR_FILTER_VALUE]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SPI;
        rs -> rule.filter_value = nla_get_u32(attr[SP_GNL_ATTR_FILTER_VALUE]);
    }
    if (attr[SP_GNL_ATTR_FILTER_MASK]) {
        rs -> rule.flags |= SP_RULE_FLAG_MATCH_SPI;
        rs -> rule.filter_mask = nla_get_u32(attr[SP_GNL_ATTR_FILTER_MASK]);
    }

    return 0;
}

/*
 * sal_rule_create_nl_sock()
 * Creates a genric netlink socket with SPM family
 */
static struct sal_rule_socket * sal_rule_create_nl_sock() {
    struct sal_rule_socket * rs = malloc(sizeof(struct sal_rule_socket));
    if (!rs) {
        wlanifCmnDebug(DBGERR,"Failed to allocate memory for sal rule socket.\n");
        return NULL;
    }

    rs -> sk = nl_socket_alloc();
    if (!rs -> sk) {
        wlanifCmnDebug(DBGERR,"Unable to allocate nl socket!\n");
        nl_socket_free(rs -> sk);
        free(rs);
        return NULL;
    }

    /* connect to genl */
    if (genl_connect(rs -> sk)) {
        wlanifCmnDebug(DBGERR,"Unable to connect to genl!\n");
        nl_socket_free(rs -> sk);
        free(rs);
        return NULL;
    }

    /* resolve the generic nl family id*/
    rs -> family_id = genl_ctrl_resolve(rs -> sk, GENL_FAMILY_NAME);
    if (rs -> family_id < 0) {
        wlanifCmnDebug(DBGERR,"Unable to resolve family name!\n");
        nl_socket_free(rs -> sk);
        free(rs);
        return NULL;
    }

    /* disable seq checks on multicast sockets */
    nl_socket_modify_cb(rs -> sk, NL_CB_MSG_IN, NL_CB_CUSTOM, sal_rule_cb, rs);
    nl_socket_disable_auto_ack(rs -> sk);
    nl_socket_disable_seq_check(rs -> sk);

    return rs;
}

struct sal_rule_socket * sal_rule_init(void) {
    return (sal_rule_create_nl_sock());
}

int sal_rule_deinit(struct sal_rule_socket * rs) {
    nl_socket_free(rs -> sk);
    free(rs);
    return 0;
}

/* Config init function to allocate and configure cfg80211*/
struct wlanif_config * wlanif_config_init(int pvt_cmd_sock_id,
                                          int pvt_event_sock_id)
{
    /* Initialize debug module used for logging */
    wlanifDbgS.dbgModule = dbgModuleFind("wlanif_cmn");
    wlanifDbgS.dbgModule->Level=DBGINFO;
    struct wlanif_config * wlanif = NULL;

    wlanif = (struct wlanif_config *) malloc(sizeof(struct wlanif_config));
    if(!wlanif)
    {
        fprintf(stderr, "Error: %s malloc failed\n",__func__);
        return NULL;
    }
#ifndef LIBCFG80211_SUPPORT
    fprintf(stderr, "Error: %s Library not compiled with CFG80211 \n",__func__);
#endif

    memset(wlanif, 0, sizeof(struct wlanif_config));

    wlanif->ctx =NULL;

    wlanif->pvt_cmd_sock_id = pvt_cmd_sock_id;
    wlanif->pvt_event_sock_id = pvt_event_sock_id;

    if ( wlanif_cfg80211_init(wlanif))
    {
        fprintf(stderr, "WLAN init ops failed\n");
        goto err;
    }
    return wlanif;
err:
    free(wlanif);
    return NULL;
}

/*Config deinit function*/
void wlanif_config_deinit(struct wlanif_config * wlanif)
{
    if(!wlanif) {
        return;
    }

    wlanif_cfg80211_deinit(wlanif);
    wlanif->ctx =NULL;
    free(wlanif);
}
