/*
 * sal_nsm.h
 *
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __SAL_NSM_H
#define __SAL_NSM_H

#include <stdint.h>
#include <stdbool.h>

#define SAL_NSM_LATENCY_HIST_BUCKETS 8

struct sal_nsm_socket;

/*
 * The hist argument to sal_nsm_latency_get should be a pointer to an array of
 * SAL_NSM_LATENCY_HIST_BUCKETS 64-bit values, to which the API will write back the
 * histogram values.
 *
 * Each bucket i counts the number of packets whose latency is less than or equal to
 * 1 microsecond * 2^i and greater than 1 microseconds * 2^(i - 1). The highest
 * bucket (SAL_NSM_LATENCY_HIST_BUCKETS - 1) has no upper limit.
 *
 * The mean is the arithmetic mean latency of all packets for the given service class
 * ID in nanoseconds.
 */

/**
 * sal_nsm_latency_get
 *	Used to get the per service latency from SFE
 *
 * @datatypes
 * sal_nsm_socket
 * char
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] ns Pointer to char.
 * @param[in] sid Value of the service_id.
 * @param[in] packets Pointer to histogram counter.
 * @param[in] bytes Pointer to mean counter.
 *
 * @return
 * bool
 */
bool sal_nsm_latency_get(struct sal_nsm_socket *ns, char *dev, uint8_t sid, uint64_t *hist, uint64_t *mean);

/**
 * sal_nsm_tput_get
 *	Used to get the per service throughput from SFE
 *
 * @datatypes
 * sal_nsm_socket
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] sid Value of the service_id.
 * @param[in] packets Pointer to packet rate context.
 * @param[in] bytes Pointer to byte rate context.
 *
 * @return
 * bool
 */
bool sal_nsm_tput_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_stats_get
 *	Used to get the per service stats from SFE
 *
 * @datatypes
 * sal_nsm_socket
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] sid Value of the service_id.
 * @param[in] packets Pointer to packets processed context.
 * @param[in] bytes Pointer to bytes processed context.
 *
 * @return
 * bool
 */
bool sal_nsm_stats_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_ppe_service_stats_get
 *	Used to get the per service stats from PPE
 *
 * @datatypes
 * sal_nsm_socket
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] sid Value of the service_id.
 * @param[in] packets Pointer to packets processed context.
 * @param[in] bytes Pointer to bytes processed context.
 *
 * @return
 * bool
 */
bool sal_nsm_ppe_service_stats_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_ppe_v4_flow_stats_get
 *	Used to get the per flow stats from PPE
 *
 * @datatypes
 * sal_nsm_socket
 * uint32_t
 * uint32_t
 * uint32_t
 * uint32_t
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] flow_ip Value of Flow ip address.
 * @param[in] flow_ident Value of Flow ip identity port.
 * @param[in] return_ip Value of the Return ip address.
 * @param[in] return_ident Value of the Return ip identity port.
 * @param[in] protocol Value of the protocol number.
 * @param[in] packets Pointer to packets processed context.
 * @param[in] bytes Pointer to bytes processed context.
 *
 * @return
 * bool
 */
bool sal_nsm_ppe_v4_flow_stats_get(struct sal_nsm_socket *ns, uint32_t flow_ip, uint16_t flow_port, uint32_t return_ip, uint16_t return_port, uint8_t protocol, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_ppe_v6_flow_stats_get
 *	Used to get the per flow stats from PPE
 *
 * @datatypes
 * sal_nsm_socket
 * uint32_t[4]
 * uint32_t
 * uint32_t[4]
 * uint32_t
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] flow_ip[4] Value of Flow ip address.
 * @param[in] flow_ident Value of Flow ip identity port.
 * @param[in] return_ip[4] Value of the Return ip address.
 * @param[in] return_ident Value of the Return ip identity port.
 * @param[in] protocol Value of the protocol number	.
 * @param[in] packets Pointer to packets processed context.
 * @param[in] bytes Pointer to bytes processed context.
 *
 * @return
 * bool
 */
bool sal_nsm_ppe_v6_flow_stats_get(struct sal_nsm_socket *ns, uint32_t flow_ip[4], uint16_t flow_port, uint32_t return_ip[4], uint16_t return_port, uint8_t protocol, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_ppe_queue_drop_get
 *	Used to get the queue drop stats from PPE
 *
 * @datatypes
 * sal_nsm_socket
 * uint8_t
 * uint32_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] qid Value of the queue id.
 * @param[in] dqi Pointer to the drop_queue_item.
 * @param[in] packets Pointer to packets processed context.
 * @param[in] bytes Pointer to bytes processed context.
 *
 * @return
 * bool
 */
bool sal_nsm_ppe_queue_drop_get(struct sal_nsm_socket *ns, uint8_t qid, uint32_t dqi, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_ppe_drop_get
 *	Used tp get the drop stats from PPE
 *
 * @datatypes
 * sal_nsm_socket
 * uint8_t
 * uint64_t
 * uint64_t
 *
 * @param[in] ns Pointer to netlinke socket.
 * @param[in] sid Value of the service_id.
 * @param[in] packets Pointer to packets context.
 * @param[in] bytes Pointer to bytes context.
 *
 * @return
 * bool
 */
bool sal_nsm_ppe_drop_get(struct sal_nsm_socket *ns, uint8_t sid, uint64_t *packets, uint64_t *bytes);

/**
 * sal_nsm_socket_free
 *	Used to free netlink socket
 *
 * @datatypes
 * sal_nsm_socket
 *
 * @param[in] ns Pointer to netlinke socket.
 *
 * @return
 * void
 */
void sal_nsm_socket_free(struct sal_nsm_socket *ns);

/**
 * sal_nsm_socket_alloc
 *	Used to allocate netlink socket
 *
 * @param[in] void.
 *
 * @return
 * struct
 */
struct sal_nsm_socket *sal_nsm_socket_alloc(void);

#endif
