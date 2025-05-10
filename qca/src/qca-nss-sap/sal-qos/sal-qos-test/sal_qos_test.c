/*
 * Copyright (c) 2022,2024 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <libsalqos/sal_qos.h>
#include <json-c/json.h>
#include <stdbool.h>

#define JSON_CONF "/etc/sal_qos/qos_conf.json"

#define QOS "qos"
#define IFACE "iface"
#define BANDWIDTH_TYPE "bandwidth_type"
#define BANDWIDTH "bandwidth"
#define PRIORITYQUEUE "prioirty_queue"
#define FLOW_ISOLATION "flow_isolation"
#define NAT "nat"
#define WASH "wash"
#define FMARK "fmark"
#define OVERHEAD_TYPE "overhead_type"
#define OVERHEAD "overhead"

struct json_object * get_json_object(struct json_object * response, const char *field_name)
{
	struct json_object *cur_obj;
	if (!json_object_object_get_ex(response, field_name, &cur_obj)) {
		return NULL;
	}
	return cur_obj;
}

int main(int argc, char** argv)
{
	FILE *fp;
	struct stat filestat;
	char* json_data;
	char* interface;
	int file_length = 0;

	struct json_object *jobj;
	struct json_object *qos_obj;
	struct json_object *iface_obj;
	struct json_object *bandwidth_obj;
	struct json_object *prioirtyqueue_obj;
	struct json_object *flow_isolation_obj;
	struct json_object *nat_obj;
	struct json_object *wash_obj;
	struct json_object *bandwidth_type_obj;
	struct json_object *fwmark_obj;
	struct json_object *overhead_type_obj;
	struct json_object *overhead_obj;

	struct  sal_qos_cake cake_params;
	struct sal_qos_qdisc *qdisc_obj;

	fp = fopen(JSON_CONF, "rb");
	if (!fp) {
		printf("Failed to open file\n");
		return -1;
	}

	if (stat(JSON_CONF, &filestat) != 0) {
		printf("File not found\n");
		return -1;
	}

	file_length = filestat.st_size;
	if (file_length == 0) {
		printf("File length is 0\n");
		fclose(fp);
		return -1;
	}

	json_data = malloc(file_length + 1);
	if (json_data == NULL) {
		printf("Cannot allocate memory!!\n");
		fclose(fp);
		return 0;
	}

	if (fread(json_data, 1, file_length, fp) != file_length) {
		printf("Error reading file!!\n");
		fclose(fp);
		goto fail;
	}
	fclose(fp);

	json_data[file_length] = '\0';

	jobj = json_tokener_parse(json_data);

	qos_obj = get_json_object(jobj, QOS);
	if (qos_obj == NULL) {
		printf("%s Info not present!\n", QOS);
		goto fail;
	}

	iface_obj = get_json_object(qos_obj, IFACE);
	if (iface_obj == NULL) {
		printf("%s Info not present!\n", IFACE);
		goto fail;
	}
	interface = (char *)json_object_get_string(iface_obj);
	printf("Iface = %s \n", interface);

	qdisc_obj = sal_qos_create(interface, SAL_QOS_CAKE);
	if (qdisc_obj == NULL) {
		printf("Error: unable to create qdisc object \n");
		goto fail;
	}

	bandwidth_obj = get_json_object(qos_obj, BANDWIDTH);
	if (bandwidth_obj == NULL) {
		printf("%s Info not present!\n", BANDWIDTH);
	} else {
		cake_params.bandwidth = json_object_get_int(bandwidth_obj);
		printf("Bandwidth = %d\n", cake_params.bandwidth);
	}

	bandwidth_type_obj = get_json_object(qos_obj, BANDWIDTH_TYPE);
	if (bandwidth_type_obj == NULL) {
		printf("%s Info not present!\n", BANDWIDTH_TYPE);
		cake_params.bandwidth_type = SAL_QOS_CAKE_UNLIMITED;
	} else {
		cake_params.bandwidth_type = json_object_get_int(bandwidth_type_obj);
		printf("Bandwidth_type = %d\n", cake_params.bandwidth_type);
	}

	prioirtyqueue_obj = get_json_object(qos_obj, PRIORITYQUEUE);
	if (prioirtyqueue_obj == NULL) {
		printf("%s Info not present! Use default value\n", PRIORITYQUEUE);
		cake_params.prioirtyqueue = SAL_QOS_CAKE_DIFSERV3;
	} else {
		cake_params.prioirtyqueue = json_object_get_int(prioirtyqueue_obj);
		printf("Prioirtyqueue = %d\n", cake_params.prioirtyqueue);
	}

	flow_isolation_obj = get_json_object(qos_obj, FLOW_ISOLATION);
	if (flow_isolation_obj == NULL) {
		printf("%s Info not present!\n", FLOW_ISOLATION);
		cake_params.flow_isolation = SAL_QOS_CAKE_TRIPLE_ISOLATE;
	} else {
		cake_params.flow_isolation = json_object_get_int(flow_isolation_obj);
		printf("Flow_isolation = %d\n", cake_params.flow_isolation);
	}

	nat_obj = get_json_object(qos_obj, NAT);
	if (nat_obj == NULL) {
		printf("%s Info not present!\n", NAT);
		cake_params.nat = SAL_QOS_CAKE_NONAT;
	} else {
		cake_params.nat = json_object_get_int(nat_obj);
		printf("Nat = %d\n", cake_params.nat);
	}

	wash_obj = get_json_object(qos_obj, WASH);
	if (wash_obj == NULL) {
		printf("%s Info not present!\n", WASH);
		cake_params.wash = SAL_QOS_CAKE_NOWASH;
	} else {
		cake_params.wash = json_object_get_int(wash_obj);
		printf("Wash = %d\n", cake_params.wash);
	}

	fwmark_obj = get_json_object(qos_obj, FWMARK);
	if (fwmark_obj == NULL) {
		printf("%s Info not present!\n", FWMARK);
		cake_params.fwmark = 0;
	} else {
		cake_params.fwmark = json_object_get_int(fwmark_obj);
		printf("Fwmark = %d\n", cake_params.fwmark);
	}

	overhead_type_obj = get_json_object(qos_obj, OVERHEAD_TYPE);
	if (overhead_type_obj == NULL) {
		printf("%s Info not present!\n", OVERHEAD_TYPE);
		cake_params.overhead_type = SAL_QOS_CAKE_RAW;
	} else {
		cake_params.overhead_type = json_object_get_int(overhead_type_obj);
		printf("Overhead_type = %d\n", cake_params.overhead_type);
	}

	overhead_obj = get_json_object(qos_obj, OVERHEAD);
	if (overhead_obj == NULL) {
		printf("%s Info not present!\n", OVERHEAD);
		cake_params.overhead = 0;
	} else {
		cake_params.overhead = json_object_get_int(overhead_obj);
		printf("Overhead_type = %d\n", cake_params.overhead);
	}

	sal_qos_delete_qdisc(qdisc_obj);
	if (!sal_qos_set_parameter(qdisc_obj, &cake_params)) {
		sal_qos_destroy(qdisc_obj);
		goto fail;
	}
	sal_qos_apply_qdisc(qdisc_obj);
	sal_qos_destroy(qdisc_obj);

	free(json_data);
	return 0;

fail:
	free(json_data);
	return -1;
}
