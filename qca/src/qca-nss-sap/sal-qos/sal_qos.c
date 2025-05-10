/*
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include "sal_qos.h"
#include <libgen.h>
#include <dirent.h>
#include <linux/if.h>
#include <stdbool.h>

/*
 *Cake paramenter arrays
 */
char bandwidth_params[BANDWIDTH_PARAM_COUNT + 1][MAX_PARAM_LENGTH] = {
									"unspec",
									"bandwidth",
									"unlimited",
									"autorate-ingress"
								};

char priority_queue_params[PRIORITY_QUEUE_PARAM_COUNT + 1][MAX_PARAM_LENGTH] = {
										"unspec",
										"besteffort",
										"diffserv8",
										"diffserv4",
										"diffserv3"
										};

char flow_isolation_params[FLOW_ISOLATION_PARAM_COUNT + 1][MAX_PARAM_LENGTH] = {
										"unspec",
										"flowblind",
										"srchost",
										"dsthost",
										"hosts",
										"flows",
										"dual-srchost",
										"dual-dsthost",
										"triple-isolate"
										};

char nat_params[NAT_PARAM_COUNT + 1][MAX_PARAM_LENGTH] = {
								"unspec",
								"nat",
								"nonat"
							};
char wash_params[WASH_PARAM_COUNT + 1][MAX_PARAM_LENGTH] = {
								"unspec",
								"wash",
								"nowash"
							};

char overhead_params[OVERHEAD_PARAM_COUNT + 1][MAX_PARAM_LENGTH] = {
									"unspec",
									"overhead",
									"conservative",
									"raw"
								};

/*
 * sal_qos_create()
 *	Initialize the qdisc object for specified interface
 */
sal_qos_qdisc * sal_qos_create(char * interface_name, int qdisc_type)
{
	sal_qos_qdisc *qs = (sal_qos_qdisc *)malloc(sizeof(sal_qos_qdisc));
	if(qs == NULL) {
		printf("Error: Failed to allocate memory");
		return NULL;
	}
	strlcpy(qs->iface, interface_name, IFNAMSIZ);
	qs->state = STATE_INITIALIZED;
	qs->qdisc_param.cake_param = NULL;
	qs->qdisc_type = qdisc_type;
	return qs;
}

/*
 * sal_qos_destroy()
 *	Free the qdisc object
 */
int sal_qos_destroy(sal_qos_qdisc * qdisc)
{
	if(qdisc->qdisc_param.cake_param != NULL)
		free(qdisc->qdisc_param.cake_param);
	free(qdisc);
	return 0;
}

/*
 * sal_qos_delete_qdisc()
 *	deletes the qos for the interface
 */
int sal_qos_delete_qdisc(sal_qos_qdisc * qdisc)
{
	pid_t pid;

	if(qdisc == NULL)
	{
		printf("Qdisc object uninitialized \n");
		return -1;
	}

	if((pid = fork()) == -1 )
	{
		printf("Could not fork process for deleting QDISC");
		return -1;
	}else if(pid == 0){
		execl(TC_PATH, TC_PATH, QDISC, DEL, ROOT, DEV, qdisc->iface, (char *)NULL);
	}
	return 0;
}

/*
 * sal_qos_set_parameter()
 *	Validate and copy the qdisc parameters
 */
bool sal_qos_set_parameter(sal_qos_qdisc * qdisc, void * params)
{
	sal_qos_cake *cake_param;

	if(qdisc->qdisc_type != SAL_QOS_CAKE) {
		printf("Error: unsupported QDISC format \n");
		return false;
	}

	cake_param = (sal_qos_cake *) params;

	if((cake_param->bandwidth_type < SAL_QOS_CAKE_BANDWIDTH || cake_param->bandwidth_type > SAL_QOS_CAKE_AUTORATE_INGRESS) ||
		(cake_param->prioirtyqueue < SAL_QOS_CAKE_BESTEFFORT || cake_param->prioirtyqueue > SAL_QOS_CAKE_DIFSERV3) ||
			(cake_param->flow_isolation < SAL_QOS_CAKE_FLOWBLIND || cake_param->flow_isolation > SAL_QOS_CAKE_TRIPLE_ISOLATE) ||
				(cake_param->nat <  SAL_QOS_CAKE_NAT|| cake_param->nat > SAL_QOS_CAKE_NONAT) ||
					(cake_param->wash < SAL_QOS_CAKE_WASH || cake_param->wash > SAL_QOS_CAKE_NOWASH) ||
						(cake_param->overhead_type < SAL_QOS_CAKE_OVERHEAD || cake_param->overhead_type > SAL_QOS_CAKE_RAW)) {
		printf("Invalid values found in input parameters \n");
		return false;
	}
	qdisc->qdisc_param.cake_param = malloc(sizeof(sal_qos_cake));
	memcpy(qdisc->qdisc_param.cake_param, cake_param, sizeof(sal_qos_cake));

	return true;
}

/*
 * qos_cake_apply_qdisc()
 *	Apply the cake qdisc qos parameters to a interface
 */
int sal_qos_apply_qdisc(sal_qos_qdisc * qdisc)
{
	pid_t pid;
	int err = 0;
	char cake_bandwidth_type[MAX_PARAM_LENGTH];
	char cake_bandwidth[MAX_PARAM_LENGTH];
	char cake_priority_queue[MAX_PARAM_LENGTH];
	char cake_flow_isolation[MAX_PARAM_LENGTH];
	char cake_nat[MAX_PARAM_LENGTH];
	char cake_wash[MAX_PARAM_LENGTH];
	char cake_overhead_type[MAX_PARAM_LENGTH];
	char cake_overhead[MAX_PARAM_LENGTH];
	char cake_fwmark[MAX_PARAM_LENGTH];

	if(qdisc == NULL || qdisc->qdisc_param.cake_param == NULL)
	{
		printf("Qdisc object uninitialized \n");
		return -1;
	}

	if(qdisc->qdisc_type != SAL_QOS_CAKE) {
		printf("Error: unsupported QDISC format \n");
		return false;
	}

	if(qdisc->qdisc_param.cake_param->bandwidth_type == SAL_QOS_CAKE_BANDWIDTH) {
		err = snprintf(cake_bandwidth, MAX_PARAM_LENGTH, "%d", qdisc->qdisc_param.cake_param->bandwidth);
		if(err < 0)
		{
			printf("Error copying bandwidth rate \n");
			return -1;
		}
		strlcat(cake_bandwidth, MBITS, MAX_PARAM_LENGTH);
	}
	strlcpy(cake_bandwidth_type, bandwidth_params[qdisc->qdisc_param.cake_param->bandwidth_type], MAX_PARAM_LENGTH);
	strlcpy(cake_priority_queue, priority_queue_params[qdisc->qdisc_param.cake_param->prioirtyqueue], MAX_PARAM_LENGTH);
	strlcpy(cake_flow_isolation, flow_isolation_params[qdisc->qdisc_param.cake_param->flow_isolation], MAX_PARAM_LENGTH);
	strlcpy(cake_nat, nat_params[qdisc->qdisc_param.cake_param->nat], MAX_PARAM_LENGTH);
	strlcpy(cake_wash, wash_params[qdisc->qdisc_param.cake_param->wash], MAX_PARAM_LENGTH);
	if(qdisc->qdisc_param.cake_param->overhead_type == SAL_QOS_CAKE_OVERHEAD) {
		err = snprintf(cake_overhead, MAX_PARAM_LENGTH, "%d", qdisc->qdisc_param.cake_param->overhead);
		if(err < 0)
		{
			printf("Error copying overhead value \n");
			return -1;
		}
	}
	strlcpy(cake_overhead_type, overhead_params[qdisc->qdisc_param.cake_param->overhead_type], MAX_PARAM_LENGTH);
	if(qdisc->qdisc_param.cake_param->fwmark) {
		err = snprintf(cake_fwmark, MAX_PARAM_LENGTH, "%d", qdisc->qdisc_param.cake_param->fwmark);
		if(err < 0)
		{
			printf("Error copying fwmark value \n");
			return -1;
		}
	}

	if((pid = fork()) == -1 )
	{
		printf("Could not fork process for applying QDISC parameters \n");
		return -1;
	} else if(pid == 0){
		if(qdisc->qdisc_param.cake_param->bandwidth_type == SAL_QOS_CAKE_BANDWIDTH){
			if(qdisc->qdisc_param.cake_param->overhead_type == SAL_QOS_CAKE_OVERHEAD) {
				if(qdisc->qdisc_param.cake_param->fwmark) {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_bandwidth, cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type,
								cake_overhead, FWMARK, cake_fwmark, (char *)NULL);
				} else {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_bandwidth, cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type, cake_overhead, (char *)NULL);
				}
			} else {
				if(qdisc->qdisc_param.cake_param->fwmark) {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_bandwidth, cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type, FWMARK, cake_fwmark, (char *)NULL);
				} else {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_bandwidth, cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type, (char *)NULL);
				}
			}
		} else {
			if(qdisc->qdisc_param.cake_param->overhead_type == SAL_QOS_CAKE_OVERHEAD) {
				if(qdisc->qdisc_param.cake_param->fwmark) {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type,
								cake_overhead, FWMARK, cake_fwmark, (char *)NULL);
				} else {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type, cake_overhead, (char *)NULL);
				}
			} else {
				if(qdisc->qdisc_param.cake_param->fwmark) {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type, FWMARK, cake_fwmark, (char *)NULL);
				} else {
					execl(TC_PATH, TC_PATH, QDISC, ADD, ROOT, DEV, qdisc->iface, CAKE, cake_bandwidth_type,
							cake_flow_isolation, cake_nat, cake_wash, cake_overhead_type, (char *)NULL);
				}
			}
		}
		return -1;
	}
	return 0;
}
