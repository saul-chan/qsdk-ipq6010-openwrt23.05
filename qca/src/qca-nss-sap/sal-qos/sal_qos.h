/*
 *  Copyright (c) 2022 Qualcomm Technologies, Inc.
 *  All Rights Reserved.
 *  Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __SAL_QOS_H
#define __SAL_QOS_H

#include <stdint.h>
#include <stdio.h>
#include <linux/if.h>
#include <stdbool.h>

#define MAX_PARAM_LENGTH 25
#define BANDWIDTH_PARAM_COUNT 3
#define PRIORITY_QUEUE_PARAM_COUNT 4
#define FLOW_ISOLATION_PARAM_COUNT 8
#define NAT_PARAM_COUNT 2
#define WASH_PARAM_COUNT 3
#define OVERHEAD_PARAM_COUNT 3

#define TC_PATH "/usr/sbin/tc"
#define QDISC "qdisc"
#define DEL "del"
#define ROOT "root"
#define DEV "dev"
#define ADD "add"
#define CAKE "cake"
#define MBITS "Mbit"
#define FWMARK "fwmark"

enum qdisc_type {
	SAL_QOS_CAKE = 1,
};

enum qdisc_state {
	STATE_INITIALIZED = 1,
};
/*
 * enum values to be used for input parameters
 */
enum sal_qos_cake_type {
	SAL_QOS_CAKE_BANDWIDTH = 1,
        SAL_QOS_CAKE_UNLIMITED,
        SAL_QOS_CAKE_AUTORATE_INGRESS,
	SAL_QOS_CAKE_BESTEFFORT = 1,
        SAL_QOS_CAKE_DIFSERV8,
        SAL_QOS_CAKE_DIFSERV4,
        SAL_QOS_CAKE_DIFSERV3,
	SAL_QOS_CAKE_FLOWBLIND = 1,
        SAL_QOS_CAKE_SRCHOST,
        SAL_QOS_CAKE_DSTHOST,
        SAL_QOS_CAKE_HOSTS,
        SAL_QOS_CAKE_FLOWS,
        SAL_QOS_CAKE_DUAL_SRCHOST,
        SAL_QOS_CAKE_DUAL_DSTHOST,
        SAL_QOS_CAKE_TRIPLE_ISOLATE,
	SAL_QOS_CAKE_NAT = 1,
        SAL_QOS_CAKE_NONAT,
	SAL_QOS_CAKE_WASH = 1,
        SAL_QOS_CAKE_NOWASH,
	SAL_QOS_CAKE_OVERHEAD = 1,
	SAL_QOS_CAKE_CONSERVATIVE,
	SAL_QOS_CAKE_RAW,
};

typedef struct  sal_qos_cake{
	/*
	 * Type of bandwidth argument
	 */
	int bandwidth_type;
	/*
         * Bandwidth Rate to be set
         */
	int bandwidth;
	/*
	 * Priority queue for the traffic
	 */
	int prioirtyqueue;
	/*
	 * Flow isolation parameter
	 */
	int flow_isolation;
	/*
         * NAT: Instructs Cake to perform a NAT lookup
	 */
	int nat;
	/*
         * wash option to clear all extra diffserv
	 */
	int wash;
	/*
	 *fwmark: Turns on fwmark-based overriding
	 */
	int fwmark;
	/*
	 *Overhead compensation
	 */
	int overhead_type;
	/*
         *Overhead compensation value
         */
	int overhead;
} sal_qos_cake;

typedef struct sal_qos_qdisc {
	/*
	 * Interface on which the cake parameters are applied
	 */
	char iface[IFNAMSIZ];
	int state;
	int qdisc_type;
	/*
         * Cake qos params to applied on the interface
	 */
	union {
		struct sal_qos_cake * cake_param;
        } qdisc_param;
} sal_qos_qdisc;

/*
 * sal_qos_create()
 * Initialize the qdisc object for specified interface
 */
sal_qos_qdisc * sal_qos_create(char * interface_name, int qdisc_type);


/*
 * sal_qos_destroy()
 * deletes the qdisc for the interface
 */
int sal_qos_destroy(sal_qos_qdisc * qdisc);

/*
 * sal_qos_set_parameter()
 * Validate and copy the qdisc parameters
 */
bool sal_qos_set_parameter(sal_qos_qdisc * qdisc, void * params);

/*
 * sal_qos_apply_qdisc()
 * Apply the cake qdisc qos parameters to a interface
 */
int sal_qos_apply_qdisc(sal_qos_qdisc * qdisc);

/*
 * sal_qos_delete_qdisc()
 * deletes the qos for the interface
 */
int sal_qos_delete_qdisc(sal_qos_qdisc * qdisc);
#endif
