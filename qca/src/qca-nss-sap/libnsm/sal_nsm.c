/*
 * sal_nsm.c
 *
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdio.h>
#include <netlink/genl/genl.h>
#include <linux/genetlink.h>
#include <netlink/genl/ctrl.h>
#include <nsm_nl_fam.h>
#include "exports/sal_nsm.h"
#include "sal_nsm_pvt.h"

/*
 * sal_nsm_cb()
 *	NSM netlink message handling callback. Writes provided packet and byte counts to NSM socket.
 */
static int sal_nsm_cb(struct nl_msg *rcv_msg, void *arg)
{
	struct nlattr *attrs[NSM_NL_ATTR_MAX + 1];
	struct nlmsghdr *hdr = nlmsg_hdr(rcv_msg);
	struct sal_nsm_socket *ns = (struct sal_nsm_socket *)arg;

	if (hdr->nlmsg_type == NLMSG_ERROR) {
		fprintf(stderr, "Reply contains error.\n");
		ns->success = false;
		return NL_STOP;
	}

	struct genlmsghdr *ghdr = (struct genlmsghdr *)nlmsg_data(hdr);

	nla_parse(attrs, NSM_NL_ATTR_MAX, genlmsg_attrdata(ghdr, 0), genlmsg_attrlen(ghdr, 0), NULL);

	if (attrs[NSM_NL_ATTR_RX_PACKETS]) {
		ns->stats.packets = nla_get_u64(attrs[NSM_NL_ATTR_RX_PACKETS]);
	}

	if (attrs[NSM_NL_ATTR_RX_BYTES]) {
		ns->stats.bytes = nla_get_u64(attrs[NSM_NL_ATTR_RX_BYTES]);
	}

	if (attrs[NSM_NL_ATTR_LATENCY_MEAN]) {
		ns->stats.latency_mean = nla_get_u64(attrs[NSM_NL_ATTR_LATENCY_MEAN]);
	}

	for (uint32_t i = 0; i < SAL_NSM_LATENCY_HIST_BUCKETS; i++) {
		if (!attrs[NSM_NL_ATTR_LATENCY_HIST0 + i]) {
			continue;
		}

		ns->stats.latency_hist[i] = nla_get_u64(attrs[NSM_NL_ATTR_LATENCY_HIST0 + i]);
	}

	ns->success = true;
	return NL_STOP;
}

/*
 * sal_nsm_cmd_send()
 *	Sends the requested command with the given service id to NSM.
 */
static bool sal_nsm_cmd_send(struct sal_nsm_socket *ns, uint8_t sid, char *dev, uint8_t cmd)
{
	struct nl_msg *msg = nlmsg_alloc();

	if (!msg) {
		return false;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ns->family, 0, NLM_F_REQUEST, cmd, 0);

	NLA_PUT_U8(msg, NSM_NL_ATTR_SERVICE_ID, sid);

	if (dev) {
		NLA_PUT_STRING(msg, NSM_NL_ATTR_NET_DEVICE, dev);
	}

	int ret = nl_send_auto(ns->sk, msg);
	if (ret < 0) {
		goto nla_put_failure;
	}

	return true;

	/*
	 * The NLA_PUT_U8 macro requires a failure label with this name.
	*/
nla_put_failure:
	nlmsg_free(msg);
	return false;
}

/*
 * sal_nsm_cmd_send_queue()
 *	Sends the requested command with the given queue id to NSM.
 */
static bool sal_nsm_cmd_send_queue(struct sal_nsm_socket *ns, uint8_t qid, uint32_t did, char *dev, uint8_t cmd)
{
	struct nl_msg *msg = nlmsg_alloc();

	if (!msg) {
		return false;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ns->family, 0, NLM_F_REQUEST, cmd, 0);

	NLA_PUT_U8(msg, NSM_NL_ATTR_SERVICE_ID, qid);
	NLA_PUT_U32(msg, NSM_NL_ATTR_DROP_QUEUE_ID, did);

	if (dev) {
		NLA_PUT_STRING(msg, NSM_NL_ATTR_NET_DEVICE, dev);
	}

	int ret = nl_send_auto(ns->sk, msg);
	if (ret < 0) {
		goto nla_put_failure;
	}

	return true;

	/*
	 * The NLA_PUT_U8 macro requires a failure label with this name.
	*/
nla_put_failure:
	nlmsg_free(msg);
	return false;
}

/*
 * sal_nsm_cmd_send_v4()
 *	Sends the requested command with the given v4 header
 */
static bool sal_nsm_cmd_send_v4(struct sal_nsm_socket *ns, uint32_t flow_ip, uint16_t flow_port, uint32_t return_ip, uint16_t return_port, uint8_t protocol, char *dev, uint8_t cmd)
{
	struct nl_msg *msg = nlmsg_alloc();

	if (!msg) {
		return false;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ns->family, 0, NLM_F_REQUEST, cmd, 0);
	NLA_PUT_U8(msg, NSM_NL_ATTR_SERVICE_ID, 0);
	NLA_PUT_U32(msg, NSM_NL_ATTR_FLOW_IP_0, flow_ip);
	NLA_PUT_U16(msg, NSM_NL_ATTR_FLOW_PORT, flow_port);
	NLA_PUT_U32(msg, NSM_NL_ATTR_RETURN_IP_0, return_ip);
	NLA_PUT_U16(msg, NSM_NL_ATTR_RETURN_PORT, return_port);
	NLA_PUT_U8(msg, NSM_NL_ATTR_PROTOCOL, protocol);

	if (dev) {
		NLA_PUT_STRING(msg, NSM_NL_ATTR_NET_DEVICE, dev);
	}

	int ret = nl_send_auto(ns->sk, msg);
	if (ret < 0) {
		goto nla_put_failure;
	}

	return true;

	/*
	 * The NLA_PUT_U8 macro requires a failure label with this name.
	*/
nla_put_failure:
	nlmsg_free(msg);
	return false;
}

/*
 * sal_nsm_cmd_send_v6()
 *	Sends the requested command with the given v6 header
 */
static bool sal_nsm_cmd_send_v6(struct sal_nsm_socket *ns, uint32_t flow_ip[4], uint16_t flow_port, uint32_t return_ip[4], uint16_t return_port, uint8_t protocol, char *dev, uint8_t cmd)
{
	struct nl_msg *msg = nlmsg_alloc();

	if (!msg) {
		return false;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ns->family, 0, NLM_F_REQUEST, cmd, 0);

	NLA_PUT_U8(msg, NSM_NL_ATTR_SERVICE_ID, 0);
	NLA_PUT_U32(msg, NSM_NL_ATTR_FLOW_IP_0, flow_ip[0]);
	NLA_PUT_U32(msg, NSM_NL_ATTR_FLOW_IP_1, flow_ip[1]);
	NLA_PUT_U32(msg, NSM_NL_ATTR_FLOW_IP_2, flow_ip[2]);
	NLA_PUT_U32(msg, NSM_NL_ATTR_FLOW_IP_3, flow_ip[3]);
	NLA_PUT_U16(msg, NSM_NL_ATTR_FLOW_PORT, flow_port);
	NLA_PUT_U32(msg, NSM_NL_ATTR_RETURN_IP_0, return_ip[0]);
	NLA_PUT_U32(msg, NSM_NL_ATTR_RETURN_IP_1, return_ip[1]);
	NLA_PUT_U32(msg, NSM_NL_ATTR_RETURN_IP_2, return_ip[2]);
	NLA_PUT_U32(msg, NSM_NL_ATTR_RETURN_IP_3, return_ip[3]);
	NLA_PUT_U16(msg, NSM_NL_ATTR_RETURN_PORT, return_port);
	NLA_PUT_U8(msg, NSM_NL_ATTR_PROTOCOL, protocol);

	if (dev) {
		NLA_PUT_STRING(msg, NSM_NL_ATTR_NET_DEVICE, dev);
	}

	int ret = nl_send_auto(ns->sk, msg);
	if (ret < 0) {
		goto nla_put_failure;
	}

	return true;

	/*
	 * The NLA_PUT_U8 macro requires a failure label with this name.
	*/
nla_put_failure:
	nlmsg_free(msg);
	return false;
}

/*
 * sal_nsm_latency_get()
 *	Gets netdevice latency for the device with the given name and given service class ID.
 */
bool sal_nsm_latency_get(struct sal_nsm_socket *ns, char *dev, uint8_t sid, uint64_t *hist, uint64_t *mean)
{
	if (!sal_nsm_cmd_send(ns, sid, dev, NSM_NL_CMD_GET_LATENCY)) {
		fprintf(stderr, "Failed to send netlink latency cmd.\n");
		return false;
	}

	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i\n", err);
		return false;
	}

	for (uint32_t i = 0; i < SAL_NSM_LATENCY_HIST_BUCKETS; i++) {
		hist[i] = ns->stats.latency_hist[i];
	}

	*mean = ns->stats.latency_mean;
	return true;
}

/*
 * sal_nsm_stats_get()
 *	Gets SFE per-service class statistics for the given sid from NSM.
 *
 * Statistics will be written to the given pointers for packet and byte counts.
 * Returns true on success and false on failure.
 */
bool sal_nsm_stats_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send(ns, sid, NULL, NSM_NL_CMD_GET_STATS)) {
		fprintf(stderr, "Failed to send netlink stats cmd.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_stats_get()
 *	Gets SFE per-service class throughput for the given sid from NSM.
 *
 * Throughput will be written to the given pointers for packet and byte counts.
 * Throughput is in bytes/packets per 100 seconds. Returns true on success and
 * false on failure.
 */
bool sal_nsm_tput_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send(ns, sid, NULL, NSM_NL_CMD_GET_THROUGHPUT)) {
		fprintf(stderr, "Failed to send netlink throughput cmd.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_ppe_service_stats_get()
 *	Gets PPE service class statistics for the given sid from NSM.
 *
 * Statistics will be written to the given pointers for packet and byte counts.
 * Returns true on success and false on failure.
 */
bool sal_nsm_ppe_service_stats_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send(ns, sid, NULL, NSM_NL_CMD_GET_PPE_SERVICE)) {
		fprintf(stderr, "Failed to send netlink stats PPE_SERVICE.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_ppe_v4_flow_stats_get()
 *	Gets PPE flow statistics for the given flow_id from NSM.
 *
 * Statistics will be written to the given pointers for packet and byte counts.
 * Returns true on success and false on failure.
 */
bool sal_nsm_ppe_v4_flow_stats_get(struct sal_nsm_socket *ns, uint32_t flow_ip, uint16_t flow_port, uint32_t return_ip, uint16_t return_port, uint8_t protocol, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send_v4(ns, flow_ip, flow_port, return_ip, return_port, protocol, NULL, NSM_NL_CMD_GET_V4_PPE_FLOW)) {
		fprintf(stderr, "Failed to send netlink stats PPE_FLOW.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_ppe_v6_flow_stats_get()
 *	Gets PPE flow statistics for the given flow_id from NSM.
 *
 * Statistics will be written to the given pointers for packet and byte counts.
 * Returns true on success and false on failure.
 */
bool sal_nsm_ppe_v6_flow_stats_get(struct sal_nsm_socket *ns, uint32_t flow_ip[4], uint16_t flow_port, uint32_t return_ip[4], uint16_t return_port, uint8_t protocol, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send_v6(ns, flow_ip, flow_port, return_ip, return_port, protocol, NULL, NSM_NL_CMD_GET_V6_PPE_FLOW)) {
		fprintf(stderr, "Failed to send netlink stats PPE_FLOW.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_ppe_queue_drop_get()
 *	Gets PPE queue statistics.
 *
 * Statistics will be written to the given pointers for packet and byte counts.
 * Returns true on success and false on failure.
 */
bool sal_nsm_ppe_queue_drop_get(struct sal_nsm_socket *ns, uint8_t qid, uint32_t dqi, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send_queue(ns, qid, dqi, NULL, NSM_NL_CMD_GET_PPE_QUEUE)) {
		fprintf(stderr, "Failed to send netlink stats PPE_QUEUE.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_ppe_drop_get()
 *	Gets PPE drop statistics per service.
 *
 * Statistics will be written to the given pointers for packet and byte counts.
 * Will return the per service drop count.
 * Returns true on success and false on failure.
 */
bool sal_nsm_ppe_drop_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes)
{
	if (!sal_nsm_cmd_send(ns, sid, NULL, NSM_NL_CMD_GET_PPE_DROP)) {
		fprintf(stderr, "Failed to send netlink stats PPE_DROP.");
		return false;
	}

	/*
	 * Waits for the reply to be received. The callback puts the reply in nsm_return_stats.
	 */
	int err = nl_recvmsgs_default(ns->sk);
	if (err) {
		fprintf(stderr, "nl_recvmsgs_default returned with error %i", err);
		return false;
	}

	*packets = ns->stats.packets;
	*bytes = ns->stats.bytes;
	return ns->success;
}

/*
 * sal_nsm_socket_free()
 *	Frees an NSM socket.
 */
void sal_nsm_socket_free(struct sal_nsm_socket *ns)
{
	nl_socket_free(ns->sk);
	free(ns);
}

/*
 * sal_nsm_socket_alloc()
 *	Allocates an NSM socket.
 */
struct sal_nsm_socket *sal_nsm_socket_alloc(void)
{
	int error = 0;

	struct sal_nsm_socket *ns = malloc(sizeof(struct sal_nsm_socket));
	if (!ns) {
		fprintf(stderr, "Failed to allocate memory for NSM socket.\n");
		return NULL;
	}

	ns->sk = nl_socket_alloc();
	if (!ns->sk) {
		fprintf(stderr, "Failed to allocate socket for NSM socket.\n");
		free(ns);
		return NULL;
	}

	error = genl_connect(ns->sk);
	if (error) {
		fprintf(stderr, "Failed to connect to generic netlink socket: %i.\n", error);
		nl_socket_free(ns->sk);
		free(ns);
		return NULL;
	}

	ns->family = genl_ctrl_resolve(ns->sk, NSM_NL_NAME);
	if (ns->family < 0) {
		fprintf(stderr, "Failed to resolve NSM netlink family %s: %i.\n", NSM_NL_NAME, ns->family);
		nl_socket_free(ns->sk);
		free(ns);
		return NULL;
	}

	nl_socket_modify_cb(ns->sk, NL_CB_MSG_IN, NL_CB_CUSTOM, sal_nsm_cb, ns);
	nl_socket_disable_auto_ack(ns->sk);
	nl_socket_disable_seq_check(ns->sk);
	return ns;
}
