/*
 * Copyright (c) 2022-2024 Qualcomm Technologies, Inc.
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
#include <libsalrule/sal_rule.h>
#include <json-c/json.h>
#include <linux/in6.h>
#include <arpa/inet.h>

#define JSON_CONF "/etc/sal_rule/spm_config_rules.json"

#define RULES "rules"
#define RULE "rule"
#define ID "in_id"
#define ADD_DELETE "in_add_delete"
#define PRECEDENCE "in_precedence"
#define RULE_OUTPUT "out_rule_output"
#define USER_PRIORITY "in_user_priority"
#define SRC_MAC "in_src_mac"
#define DST_MAC "in_dst_mac"
#define SRC_IPV4_ADDR "in_src_ipv4_addr"
#define DST_IPV4_ADDR "in_dst_ipv4_addr"
#define SRC_IPV6_ADDR "in_src_ipv6_addr"
#define DST_IPV6_ADDR "in_dst_ipv6_addr"
#define SRC_PORT_ADDR "in_src_port"
#define DST_PORT_ADDR "in_dst_port"
#define PROTO_NUM "in_proto_num"
#define VLAN_ID "in_vlan_id"
#define DSCP "in_dscp"
#define DSCP_REMARK "out_dscp_remark"
#define VLAN_PCP "in_vlan_pcp"
#define VLAN_PCP_REMARK "out_vlan_pcp_remark"
#define SERVICE_CLASS_ID "out_service_class_id"
#define SRC_IPV4_ADDR_MASK "in_src_ipv4_addr_mask"
#define DST_IPV4_ADDR_MASK "in_dst_ipv4_addr_mask"
#define SRC_IPV6_ADDR_MASK "in_src_ipv6_addr_mask"
#define DST_IPV6_ADDR_MASK "in_dst_ipv6_addr_mask"
#define IFINDEX "if_index"
#define SRC_PORT_RANGE_START "in_src_port_range_start"
#define SRC_PORT_RANGE_END "in_src_port_range_end"
#define DST_PORT_RANGE_START "in_dst_port_range_start"
#define DST_PORT_RANGE_END "in_dst_port_range_end"
#define AE_TYPE "out_ae_type"
#define SRC_IFACE "in_src_iface"
#define DST_IFACE "in_dst_iface"
#define CLASSIFIER_TYPE "rule_classifier_type"

struct json_object * get_json_object(struct json_object * response, const char *field_name)
{
	struct json_object *cur_obj;
	if(!json_object_object_get_ex(response, field_name, &cur_obj))
	{
		return NULL;
	}
	return cur_obj;
}

/* Convert a string format MAC address to u_int8_t array */
static void sal_rule_mac_addr_str2uchar(char* str, u_int8_t *mac)
{
	char *ptr, *tmp = NULL;
	int i = 0;
	static const char delims[]=":-.";

	ptr = strtok_r(str, delims, &tmp);

	while(ptr) {
		unsigned long val = strtoul(ptr, NULL, 16);

		if(val > 0xff || i >= ETH_ALEN) {
			printf("Invalid MAC address\n");
			exit(1);
		}

		mac[i] = (u_int8_t)val;
		i++;
		ptr = strtok_r(NULL, delims, &tmp);
	}

	if(i != ETH_ALEN) {
		printf("Invalid MAC address\n");
		exit(1);
	}
}

/* Convert a string format ipv4 address to u_int8_t array */
static void sal_rule_ipv4_addr_str2uchar(char *str, u_int32_t *ipv4)
{
	if (!str) {
		printf("Invalid IPV4 address\n");
		exit(1);
	}

	if (inet_pton(AF_INET, str, ipv4) == 0) {
		printf("Invalid IPV4 address %s\n", str);
		exit(1);
	}
}

/* Convert a string format ipv6 address to u_int8_t array */
static void sal_rule_ipv6_addr_str2uchar(char *str, u_int32_t *ipv6)
{
	if (!str) {
		printf("Invalid IPV6 address\n");
		exit(1);
	}

	if (inet_pton(AF_INET6, str, ipv6) == 0) {
		printf("Invalid IPV6 address\n");
		exit(1);
	}
}

/* Convert a string format ae type to enum */
static enum spm_ae_type sal_rule_char_ae_type_to_enum(char *str)
{
	if ((!str) || !(strcasecmp(str, "default"))) {
		return SPM_AE_TYPE_DEFAULT;
	}

	if (!(strcasecmp(str, "ppe"))) {
		return SPM_AE_TYPE_PPE;
	}

	if (!(strcasecmp(str, "sfe"))) {
		return SPM_AE_TYPE_SFE;
	}

	if (!(strcasecmp(str, "ppe-ds"))) {
		return SPM_AE_TYPE_PPE_DS;
	}

	if (!(strcasecmp(str, "ppe-vp"))) {
		return SPM_AE_TYPE_PPE_VP;
	}

	if (!(strcasecmp(str, "none"))) {
		return SPM_AE_TYPE_NONE;
	}

	return SPM_AE_TYPE_DEFAULT;
}

/* Convert a string format classifier type to enum */
static enum spm_classifier_type sal_rule_char_classifier_type_to_enum(char *str)
{
	if ((!str) || !(strcasecmp(str, "default"))) {
		return SPM_RULE_TYPE_SAWF;
	}

	if (!(strcasecmp(str, "ifli"))) {
		return SPM_RULE_TYPE_SAWF_IFLI;
	}

	if (!(strcasecmp(str, "scs"))) {
		return SPM_RULE_TYPE_SAWF_SCS;
	}

	if (!(strcasecmp(str, "mscs"))) {
		return SPM_RULE_TYPE_MSCS;
	}

	return SPM_RULE_TYPE_SAWF;
}

/* Convert a enum to string ae type */
static const char* sal_rule_enum_to_char_ae_type(enum spm_ae_type ae_type)
{
	switch (ae_type) {
	case SPM_AE_TYPE_NONE:
		return "none";

	case SPM_AE_TYPE_PPE:
		return "ppe";

	case SPM_AE_TYPE_SFE:
		return "sfe";

	case SPM_AE_TYPE_PPE_DS:
		return "ppe-ds";

	case SPM_AE_TYPE_PPE_VP:
		return "ppe-vp";

	default:
		return "default";
	}
}

void print_rule_query(sp_rule rule_data)
{
	int i;
	struct in_addr ip_addr;
	char addr_buf[92];
	printf("=======Query Result=======\n");
	printf("Rule id: 0x%x\n", rule_data.attr_id);
	if(rule_data.flags & SP_RULE_FLAG_MATCH_PRECEDENCE) {
		printf("Rule precedence: 0x%x\n", rule_data.precedence);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_RULE_OUTPUT) {
		printf("Rule output: 0x%x\n", rule_data.rule_output);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_USER_PRIORITY) {
		printf("User priority: 0x%x\n", rule_data.user_priority);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DSCP_REMARK) {
		printf("Dscp remark: 0x%x\n", rule_data.dscp_remark);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK) {
		printf("Vlan PCP remark: 0x%x\n", rule_data.vlan_pcp_remark);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID) {
		printf("Service_class_id: 0x%x\n", rule_data.service_class_id);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_PORT) {
		printf("Src port: 0x%x\n", rule_data.src_port);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_PORT) {
		printf("Dst port: 0x%x\n", rule_data.dst_port);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_IP_VERSION_TYPE) {
		printf("IP_version_type: 0x%x\n", rule_data.ip_version_type);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE) {
		printf("Classifier_type: 0x%x\n", rule_data.classifier_type);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_MAC) {
		printf("Src mac address: ");
		printf("%02x", rule_data.src_mac[0]);
		for (i = 1; i < 6; ++i) {
			printf(":%02x", rule_data.src_mac[i]);
		}
		printf("\n");
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_MAC) {
		printf("Dst mac address: ");
		printf("%02x", rule_data.src_mac[0]);
		for (i = 1; i < 6; ++i) {
			printf(":%02x", rule_data.dst_mac[i]);
		}
		printf("\n");
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_IPV4) {
		ip_addr.s_addr = rule_data.src_ipv4_addr;
		printf("Src ipv4 address: %s\n", inet_ntoa(ip_addr));
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_IPV4) {
		ip_addr.s_addr = rule_data.dst_ipv4_addr;
		printf("Dst ipv4 address: %s\n", inet_ntoa(ip_addr));
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_IPV4_MASK) {
		ip_addr.s_addr = rule_data.src_ipv4_addr_mask;
		printf("Src ipv4 address mask: %s\n", inet_ntoa(ip_addr));
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_IPV4_MASK) {
		ip_addr.s_addr = rule_data.dst_ipv4_addr_mask;
		printf("Dst ipv4 address mask: %s\n", inet_ntoa(ip_addr));
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_IPV6) {
		inet_ntop(AF_INET6, &rule_data.src_ipv6_addr, addr_buf, INET6_ADDRSTRLEN);
		printf("Src ipv6 address: %s\n", addr_buf);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_IPV6) {
		inet_ntop(AF_INET6, &rule_data.dst_ipv6_addr, addr_buf, INET6_ADDRSTRLEN);
		printf("Dst ipv6 address: %s\n", addr_buf);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_IPV6_MASK) {
		inet_ntop(AF_INET6, &rule_data.src_ipv6_addr_mask, addr_buf, INET6_ADDRSTRLEN);
		printf("Src ipv6 address mask: %s\n", addr_buf);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_IPV6_MASK) {
		inet_ntop(AF_INET6, &rule_data.dst_ipv6_addr_mask, addr_buf, INET6_ADDRSTRLEN);
		printf("Dst ipv6 address mask: %s\n", addr_buf);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_PROTO_NUM) {
		printf("Protocol_number: 0x%x\n", rule_data.proto_num);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_VLAN_ID) {
		printf("Vlan_id: 0x%x\n", rule_data.vlan_id);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DSCP) {
		printf("Dscp: 0x%x\n", rule_data.dscp);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_VLAN_PCP) {
		printf("Vlan_pcp: 0x%x\n", rule_data.vlan_pcp);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SPI) {
		printf("Filter_value: %x\n", rule_data.filter_value);
		printf("Filter_mask: %x\n", rule_data.filter_mask);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_IFINDEX) {
		printf("Interface Index Value : %x\n", rule_data.ifindex);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_START) {
		printf("Source port range start value : %x\n", rule_data.src_port_range_start);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_END) {
		printf("Source port range end value : %x\n", rule_data.src_port_range_end);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_PORT_RANGE_START) {
		printf("Destination port range start value : %x\n", rule_data.dst_port_range_start);
	}
	if(rule_data.flags & SP_RULE_FLAG_MATCH_DST_PORT_RANGE_END) {
		printf("Destination port range end value : %x\n", rule_data.dst_port_range_end);
	}
	if(rule_data.flags_ext & SP_RULE_FLAG_EXT_MATCH_AE_TYPE) {
		printf("ae type Value : %s\n", sal_rule_enum_to_char_ae_type(rule_data.ae_type));
	}
	if(rule_data.flags_ext & SP_RULE_FLAG_EXT_MATCH_SRC_IFACE) {
		printf("Source Interface Name : %s\n", rule_data.src_iface);
	}
	if(rule_data.flags_ext & SP_RULE_FLAG_EXT_MATCH_DST_IFACE) {
		printf("Destination Interface Name : %s\n", rule_data.dst_iface);
	}
}

int main(int argc, char** argv)
{
	FILE *fp;
	struct stat filestat;
	char* json_data;
	char* src_mac;
	char* dst_mac;
	char* src_ipv4_addr;
	char* dst_ipv4_addr;
	char* src_ipv6_addr;
	char* dst_ipv6_addr;
	char* src_ipv4_addr_mask;
	char* dst_ipv4_addr_mask;
	char* src_ipv6_addr_mask;
	char* dst_ipv6_addr_mask;
	uint32_t u32_src_ipv6_addr[IPV6_LEN];
	uint32_t u32_dst_ipv6_addr[IPV6_LEN];
	uint32_t u32_src_ipv6_addr_mask[IPV6_LEN];
	uint32_t u32_dst_ipv6_addr_mask[IPV6_LEN];
	int file_length = 0;
	int err = 0, rule_count = 0, i = 0;
	char* ae_type;
	char* src_iface;
	char* dst_iface;
	char* classifier_type;

	/*JSON objects*/
	struct json_object *jobj;
	struct json_object *rule_list;
	struct json_object *current_rule;
	struct json_object *rule_obj;
	struct json_object *id_obj;
	struct json_object *add_del_obj;
	struct json_object *precedence_obj;
	struct json_object *rule_output_obj;
	struct json_object *user_priority_obj;
	struct json_object *src_mac_obj;
	struct json_object *dst_mac_obj;
	struct json_object *src_ipv4_obj;
	struct json_object *dst_ipv4_obj;
	struct json_object *src_ipv6_obj;
	struct json_object *dst_ipv6_obj;
	struct json_object *src_port_addr_obj;
	struct json_object *dst_port_addr_obj;
	struct json_object *proto_num_obj;
	struct json_object *vlan_id_obj;
	struct json_object *dscp_obj;
	struct json_object *dscp_remark_obj;
	struct json_object *vlan_pcp_obj;
	struct json_object *vlan_pcp_remark_obj;
	struct json_object *service_class_id_obj;
	struct json_object *src_ipv4_mask_obj;
	struct json_object *dst_ipv4_mask_obj;
	struct json_object *src_ipv6_mask_obj;
	struct json_object *dst_ipv6_mask_obj;
	struct json_object *ifindex_obj;
	struct json_object *src_port_range_start_obj;
	struct json_object *src_port_range_end_obj;
	struct json_object *dst_port_range_start_obj;
	struct json_object *dst_port_range_end_obj;
	struct json_object *ae_type_obj;
	struct json_object *src_iface_obj;
	struct json_object *dst_iface_obj;
	struct json_object *classifier_type_obj;

	struct sal_rule_socket *rs = sal_rule_init();
	struct sp_rule query_rule;
	if (rs == NULL) {
		printf("netlink sock creation Error!\n");
		return -1;
	}

	fp = fopen(JSON_CONF, "rb");
	if (!fp) {
                printf("Failed to open file\n");
                return -1;
        }

	if ( stat(JSON_CONF, &filestat) != 0) {
		printf("File not found\n");
		return 1;
	}
	file_length = filestat.st_size;
	if(file_length != 0) {
		json_data = malloc(file_length);
	}
	if(json_data == NULL)
	{
		printf("Cannot allocate memory!!");
		fclose(fp);
		return 0;
	}
	if(fread(json_data, 1, file_length, fp) != file_length)
	{
		printf("Error reading File!!\n");
		free(json_data);
		fclose(fp);
		return 1;
	}
	fclose(fp);

	jobj = json_tokener_parse(json_data);
	rule_list = get_json_object(jobj, RULES);
	if(rule_list == NULL) {
		printf("Error: %s info not present!\n", RULES);
		free(json_data);
		json_object_put(jobj);
		return -1;
	}
	rule_count = json_object_array_length(rule_list);
	printf("Rule obj count %d\n", rule_count);

	for(i = 0; i < rule_count; i++)
	{
		printf("=================Config Rule=================\n");
		memset(&rs->rule, 0, sizeof(sp_rule));
		memset(&query_rule, 0, sizeof(sp_rule));
		if(err != 0)
		{
			printf("netlink sock creation Error!\n");
			free(json_data);
			exit(1);
		}

		current_rule = json_object_array_get_idx(rule_list, i);
		rule_obj = get_json_object(current_rule, RULE);
		id_obj = get_json_object(rule_obj, ID);
		if(id_obj == NULL) {
			printf("Error: %s info not present!\n", ID);
			return 0;
		}
		rs->rule.attr_id = json_object_get_int(id_obj);
		printf("Attr_id = %d\n", rs->rule.attr_id);

		add_del_obj = get_json_object(rule_obj, ADD_DELETE);
		if(add_del_obj == NULL) {
			printf(" %s info not present!\n", ADD_DELETE);
		}
		rs->rule.add_del_rule = json_object_get_int(add_del_obj);
		printf("Add_del_rule = %d\n", rs->rule.add_del_rule);

		precedence_obj = get_json_object(rule_obj, PRECEDENCE);
		if(precedence_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_PRECEDENCE;
			rs->rule.precedence = json_object_get_int(precedence_obj);
			printf("Precedence = %d\n", rs->rule.precedence);
		}
		rule_output_obj = get_json_object(rule_obj, RULE_OUTPUT);
		if(rule_output_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_RULE_OUTPUT;
			rs->rule.rule_output = json_object_get_int(rule_output_obj);
			printf("Rule_output = %d\n", rs->rule.rule_output);
		}
		user_priority_obj = get_json_object(rule_obj, USER_PRIORITY);
		if(user_priority_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_USER_PRIORITY;
			rs->rule.user_priority = json_object_get_int(user_priority_obj);
			printf("User_priority = %d\n", rs->rule.user_priority);
		}
		src_mac_obj = get_json_object(rule_obj, SRC_MAC);
		if(src_mac_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_MAC;
			src_mac = (char *)json_object_get_string(src_mac_obj);
			printf("src_mac = %s\n",src_mac);
			sal_rule_mac_addr_str2uchar(src_mac, rs->rule.src_mac);
		}
		dst_mac_obj = get_json_object(rule_obj, DST_MAC);
		if(dst_mac_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_MAC;
			dst_mac = (char *)json_object_get_string(dst_mac_obj);
			printf("dst_mac = %s\n",dst_mac);
			sal_rule_mac_addr_str2uchar(dst_mac, rs->rule.dst_mac);
		}

		src_ipv4_obj = get_json_object(rule_obj, SRC_IPV4_ADDR);
		if(src_ipv4_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV4;
			src_ipv4_addr = (char *)json_object_get_string(src_ipv4_obj);
			printf("src_ipv4_addr = %s\n",src_ipv4_addr);
			sal_rule_ipv4_addr_str2uchar(src_ipv4_addr, &rs->rule.src_ipv4_addr);
		}

		src_ipv4_mask_obj = get_json_object(rule_obj, SRC_IPV4_ADDR_MASK);
		if(src_ipv4_mask_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV4_MASK;
			src_ipv4_addr_mask = (char *)json_object_get_string(src_ipv4_mask_obj);
			printf("src_ipv4_addr_mask = %s\n",src_ipv4_addr_mask);
			sal_rule_ipv4_addr_str2uchar(src_ipv4_addr_mask, &rs->rule.src_ipv4_addr_mask);
		}

		dst_ipv4_obj = get_json_object(rule_obj, DST_IPV4_ADDR);
		if(dst_ipv4_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV4;
			dst_ipv4_addr = (char *)json_object_get_string(dst_ipv4_obj);
			printf("dst_ipv4_addr = %s\n", dst_ipv4_addr);
			sal_rule_ipv4_addr_str2uchar(dst_ipv4_addr, &rs->rule.dst_ipv4_addr);
		}
		dst_ipv4_mask_obj = get_json_object(rule_obj, DST_IPV4_ADDR_MASK);
		if(dst_ipv4_mask_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV4_MASK;
			dst_ipv4_addr_mask = (char *)json_object_get_string(dst_ipv4_mask_obj);
			printf("dst_ipv4_addr_mask = %s\n",dst_ipv4_addr_mask);
			sal_rule_ipv4_addr_str2uchar(dst_ipv4_addr_mask, &rs->rule.dst_ipv4_addr_mask);
		}
		src_ipv6_obj = get_json_object(rule_obj, SRC_IPV6_ADDR);
		if(src_ipv6_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV6;
			src_ipv6_addr = (char *)json_object_get_string(src_ipv6_obj);
			printf("src_ipv6_addr = %s\n",src_ipv6_addr);
			sal_rule_ipv6_addr_str2uchar(src_ipv6_addr, u32_src_ipv6_addr);
			memcpy(&rs->rule.src_ipv6_addr, u32_src_ipv6_addr, sizeof(struct in6_addr));
		}
		src_ipv6_mask_obj = get_json_object(rule_obj, SRC_IPV6_ADDR_MASK);
		if(src_ipv6_mask_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_IPV6_MASK;
			src_ipv6_addr_mask = (char *)json_object_get_string(src_ipv6_mask_obj);
			printf("src_ipv6_addr_mask = %s\n",src_ipv6_addr_mask);
			sal_rule_ipv6_addr_str2uchar(src_ipv6_addr_mask, u32_src_ipv6_addr_mask);
			memcpy(&rs->rule.src_ipv6_addr_mask, u32_src_ipv6_addr_mask, sizeof(struct in6_addr));
		}
		dst_ipv6_obj = get_json_object(rule_obj, DST_IPV6_ADDR);
		if(dst_ipv6_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV6;
			dst_ipv6_addr = (char *)json_object_get_string(dst_ipv6_obj);
			printf("dst_ipv6_addr = %s\n", dst_ipv6_addr);
			sal_rule_ipv6_addr_str2uchar(dst_ipv6_addr, u32_dst_ipv6_addr);
			memcpy(&rs->rule.dst_ipv6_addr, u32_dst_ipv6_addr, sizeof(struct in6_addr));
		}
		dst_ipv6_mask_obj = get_json_object(rule_obj, DST_IPV6_ADDR_MASK);
		if(dst_ipv6_mask_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_IPV6_MASK;
			dst_ipv6_addr_mask = (char *)json_object_get_string(dst_ipv6_mask_obj);
			printf("dst_ipv6_addr_mask = %s\n",dst_ipv6_addr_mask);
			sal_rule_ipv6_addr_str2uchar(dst_ipv6_addr_mask, u32_dst_ipv6_addr_mask);
			memcpy(&rs->rule.dst_ipv6_addr_mask, u32_dst_ipv6_addr_mask, sizeof(struct in6_addr));
		}
		src_port_addr_obj = get_json_object(rule_obj, SRC_PORT_ADDR);
		if(src_port_addr_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT;
			rs->rule.src_port = json_object_get_int(src_port_addr_obj);
			printf("Src_port = %d\n", rs->rule.src_port);
		}
		dst_port_addr_obj = get_json_object(rule_obj, DST_PORT_ADDR);
		if(dst_port_addr_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT;
			rs->rule.dst_port = json_object_get_int(dst_port_addr_obj);
			printf("Dst_port = %d\n", rs->rule.dst_port);
		}
		ifindex_obj = get_json_object(rule_obj, IFINDEX);
		if(ifindex_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_IFINDEX;
			rs->rule.ifindex = json_object_get_int(ifindex_obj);
			printf("Interface Index = %d\n", rs->rule.ifindex);
		}
		proto_num_obj = get_json_object(rule_obj, PROTO_NUM);
		if(proto_num_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_PROTO_NUM;
			rs->rule.proto_num = json_object_get_int(proto_num_obj);
			printf("Proto_num = %d\n", rs->rule.proto_num);
		}
		vlan_id_obj = get_json_object(rule_obj, VLAN_ID);
		if(vlan_id_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_VLAN_ID;
			rs->rule.vlan_id = json_object_get_int(vlan_id_obj);
			printf("Vlan_id = %d\n", rs->rule.vlan_id);
		}
		dscp_obj = get_json_object(rule_obj, DSCP);
		if(dscp_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DSCP;
			rs->rule.dscp = json_object_get_int(dscp_obj);
			printf("Dscp = %d\n", rs->rule.dscp);
		}
		dscp_remark_obj = get_json_object(rule_obj, DSCP_REMARK);
		if(dscp_remark_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DSCP_REMARK;
			rs->rule.dscp_remark = json_object_get_int(dscp_remark_obj);
			printf("Dscp Remark= %d\n", rs->rule.dscp_remark);
		}
		vlan_pcp_obj = get_json_object(rule_obj, VLAN_PCP);
		if(vlan_pcp_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_VLAN_PCP;
			rs->rule.vlan_pcp = json_object_get_int(vlan_pcp_obj);
			printf("Vlan_pcp = %d\n", rs->rule.vlan_pcp);
		}
		vlan_pcp_remark_obj = get_json_object(rule_obj, VLAN_PCP_REMARK);
		if(vlan_pcp_remark_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_VLAN_PCP_REMARK;
			rs->rule.vlan_pcp_remark = json_object_get_int(vlan_pcp_remark_obj);
			printf("Vlan_pcp_remark = %d\n", rs->rule.vlan_pcp_remark);
		}
		service_class_id_obj = get_json_object(rule_obj, SERVICE_CLASS_ID);
		if(service_class_id_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SERVICE_CLASS_ID;
			rs->rule.service_class_id = json_object_get_int(service_class_id_obj);
			printf("Service_class_id = %d\n", rs->rule.service_class_id);
		}
		src_port_range_start_obj = get_json_object(rule_obj, SRC_PORT_RANGE_START);
		if(src_port_range_start_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_START;
			rs->rule.src_port_range_start = json_object_get_int(src_port_range_start_obj);
			printf("Source port range start = %d\n", rs->rule.src_port_range_start);
		}
		src_port_range_end_obj = get_json_object(rule_obj, SRC_PORT_RANGE_END);
		if(src_port_range_end_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_SRC_PORT_RANGE_END;
			rs->rule.src_port_range_end = json_object_get_int(src_port_range_end_obj);
			printf("Source port range end = %d\n", rs->rule.src_port_range_end);
		}
		dst_port_range_start_obj = get_json_object(rule_obj, DST_PORT_RANGE_START);
		if(dst_port_range_start_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT_RANGE_START;
			rs->rule.dst_port_range_start = json_object_get_int(dst_port_range_start_obj);
			printf("Destination port range start = %d\n", rs->rule.dst_port_range_start);
		}
		dst_port_range_end_obj = get_json_object(rule_obj, DST_PORT_RANGE_END);
		if(dst_port_range_end_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_DST_PORT_RANGE_END;
			rs->rule.dst_port_range_end = json_object_get_int(dst_port_range_end_obj);
			printf("Destination port range end = %d\n", rs->rule.dst_port_range_end);
		}
		ae_type_obj = get_json_object(rule_obj, AE_TYPE);
		if(ae_type_obj != NULL) {
			rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MATCH_AE_TYPE;
			ae_type = (char *)json_object_get_string(ae_type_obj);
			rs->rule.ae_type = sal_rule_char_ae_type_to_enum(ae_type) ;
			printf("ae_type = %s ae_type_num = %d\n", sal_rule_enum_to_char_ae_type(rs->rule.ae_type),
													rs->rule.ae_type);
		}
		src_iface_obj = get_json_object(rule_obj, SRC_IFACE);
		if(src_iface_obj != NULL) {
			rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MATCH_SRC_IFACE;
			src_iface = (char *)json_object_get_string(src_iface_obj);
			memcpy(&rs->rule.src_iface, src_iface, IFNAMSIZ);
			printf("src_iface = %s \n", rs->rule.src_iface);
		}
		dst_iface_obj = get_json_object(rule_obj, DST_IFACE);
		if(dst_iface_obj != NULL) {
			rs->rule.flags_ext |= SP_RULE_FLAG_EXT_MATCH_DST_IFACE;
			dst_iface = (char *)json_object_get_string(dst_iface_obj);
			memcpy(&rs->rule.dst_iface, dst_iface, IFNAMSIZ);
			printf("dst_iface = %s \n", rs->rule.dst_iface);
		}
		classifier_type_obj = get_json_object(rule_obj, CLASSIFIER_TYPE);
		if(classifier_type_obj != NULL) {
			rs->rule.flags |= SP_RULE_FLAG_MATCH_CLASSIFIER_TYPE;
			classifier_type = (char *)json_object_get_string(classifier_type_obj);
			rs->rule.classifier_type = sal_rule_char_classifier_type_to_enum(classifier_type);
			printf("classifier_type = %d\n", rs->rule.classifier_type);
		}

		if (rs->rule.add_del_rule == SPM_CMD_RULE_QUERY) {
			if(sal_rule_query(rs, &query_rule) != 0)
			{
				printf("SAWF rule query failed\n");
				sal_rule_deinit(rs);
				return -1;
			}
			print_rule_query(query_rule);
		} else {
			if(sal_rule_config(rs) == -1) {
				printf("SAWF config rule update failed\n");
				sal_rule_deinit(rs);
				return -1;
			}
		}
	}
	sal_rule_deinit(rs);
	return 0;
}
