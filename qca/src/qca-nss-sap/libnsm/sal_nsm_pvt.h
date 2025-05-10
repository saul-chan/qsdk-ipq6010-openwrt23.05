/*
 * sal_nsm_pvt.h
 *
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __SAL_NSM_PVT_H
#define __SAL_NSM_PVT_H

#include <stdint.h>
#include "exports/sal_nsm.h"

/*
 * sal_nsm_stats
 *	NSM statistics. Packets and bytes can represent either counts or rates.
 *
 * The latency histogram is divided into SAL_NSM_LATENCY_HIST_BUCKETS buckets. Each bucket i counts the number of packets
 * whose latency is less than or equal to 1 microsecond * 2^i and greater than 1 microseconds * 2^(i - 1). The highest
 * bucket (SAL_NSM_LATENCY_HIST_BUCKETS - 1) has no upper limit.
 */
struct sal_nsm_stats {
	uint64_t packets;					/* Packet count or packet rate. */
	uint64_t bytes;						/* Byte count or byte rate. */
	uint64_t latency_hist[SAL_NSM_LATENCY_HIST_BUCKETS];	/* Latency histogram data. */
	uint64_t latency_mean;					/* Mean latency in nanoseconds. */
};

/*
 * sal_nsm_socket
 *	Structure containing necessary information to cmdmunicate with NSM.
 */
struct sal_nsm_socket {
	struct nl_sock *sk;
	struct sal_nsm_stats stats;
	int family;
	bool success;
};

#endif
