/*
 * nsm_app.c
 *
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <sal_nsm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdint.h>
#include <arpa/inet.h>

#define SID_MAX 255
#define QUEUE_ID_MAX 128
#define DROP_QUEUE_IDX_MAX 6
#define DROP_MAX 128
#define FLOW_MAX 8
#define SERVICE_MAX 128
#define NSM_APP_PREC 100
#define NSM_APP_US_PER_SEC 1000000

static int64_t nsm_app_rate_calc(uint64_t dval, int64_t dtime)
{
	uint64_t scaled_dval = dval * NSM_APP_PREC * NSM_APP_US_PER_SEC;

	if (dtime == 0) {
		return -1;
	}

	/*
	 * The best way to calculate the rate accurately is to scale
	 * the numerator up, but if this results in an overflow, we
	 * can instead scale the denominator down.
	 */
	if (scaled_dval < dval) {
		uint64_t ret;
		uint64_t scaled_dtime = dtime;
		div(scaled_dtime, (NSM_APP_PREC * NSM_APP_US_PER_SEC));
		if (scaled_dtime == 0) {
			return -1;
		}
		ret = dval;
		div(ret, scaled_dtime);
		return ret;
	}

	div(scaled_dval, dtime);
	return scaled_dval;
}

/*
 * nsm_appparam_get_int()
 * 	extract the integer from the incoming data
 */
bool nsm_app_param_get_int(const char *arg, uint16_t data_sz, void *data)
{
	long int_val;
	char *end;

	if (!arg || !data) {
		return false;
	}

	/*
	 * Reset errno to test if there any errors in the conversion
	 * process. If there are errors in during the conversion. An
	 * error will returned without any value produced in output
	 */
	errno = 0;
	int_val = strtol(arg, &end, 10);
	if (errno) {
		if (int_val == LONG_MIN) { /* Underflow */
			return false;
		}
		if (int_val == LONG_MAX) { /* Overflow */
			return false;
		}
		if (!int_val || (arg == end)) { /* Nothing is produced */
			return false;
		}
	}

	memcpy(data, &int_val, data_sz);
	return true;
}

/*
 * nlcfg_param_get_ipaddr()
 * 	extract the IPv4 or IPv6 address in network order from the incoming data
 */
bool nsm_app_param_get_ipaddr(const char *arg, uint16_t data_sz, void *data)
{
	if (!arg || !data) {
		return false;
	}

	switch(data_sz) {
	case sizeof(struct in_addr): /* IPv4 */
		if (inet_pton(AF_INET, arg, data) == 0) {
			return false;
		}

		break;

	case sizeof(struct in6_addr): /* IPv6 */
		if (inet_pton(AF_INET6, arg, data) == 0) {
			return false;
		}

		break;

	default:
		printf("IP address storage incorrect:%d\n", data_sz);
		return false;
	}

	return true;
}

/*
 * main()
 *	Queries a small number of arbitrary service class statistics.
 */
int main(int argc, char *argv[])
{
	uint32_t flow_ip[4];
	uint32_t return_ip[4];
	uint16_t flow_port;
	uint16_t return_port;
	uint8_t protocol;
	uint64_t byte_rate, packet_rate;
	uint64_t bytes, packets, bytes_2nd, packets_2nd;
	uint64_t dbytes, dpackets;
	int64_t dtime, rate;

	/*
	 *Allocating NSM socket.
	 */
	struct sal_nsm_socket *ns = sal_nsm_socket_alloc();
	if (!ns) {
		return -1;
	}

	switch (*(argv[1])) {
	case 'a':
			printf("\nSFE Stats Dump:\n\n");
			for (int i = 2; i < argc; i++) {
				long sid = strtol(argv[i], NULL, 10);
				if (errno) {
					perror("Argument could not be read as integer.");
					sal_nsm_socket_free(ns);
					return errno;
				}

				if (sid < 0 || sid > SID_MAX) {
					fprintf(stderr, "Service ID %li out of range [0, %i]\n", sid, SID_MAX);
					sal_nsm_socket_free(ns);
					return -1;
				}

				if (!sal_nsm_stats_get(ns, sid, &packets, &bytes)) {
					sal_nsm_socket_free(ns);
					return -1;
				}
				printf("SID:%li\nPackets: %" PRIu64 "\nBytes: %" PRIu64 "\n", sid, packets, bytes);
			}
			break;
	case 'b':
			printf("PPE_SERVICE_STATS:\n ");
			if (!sal_nsm_ppe_service_stats_get(ns, atoi(argv[2]), &packets, &bytes)) {
				printf("PPE_SERVICE Stats Failed Service_ID:%d\n", atoi(argv[2]));
			} else {
				printf("SERVICE_ID:%d Packets: %" PRIu64 " Bytes: %" PRIu64 "\n", atoi(argv[2]), packets, bytes);
			}
			break;
	case 'c':
			printf("\nPPE_SERVICE_THROUGHPUT:\n");

			/*
			 * Grab First sample
			 */
			if (!sal_nsm_ppe_service_stats_get(ns, atoi(argv[2]), &packets, &bytes)) {
				printf("PPE_SERVICE Throughput Sample 1 Failed Service_ID:%d\n", atoi(argv[2]));
				break;
			}

			sleep(1);

			/*
			 * Grab Second sample
			 */
			if (!sal_nsm_ppe_service_stats_get(ns, atoi(argv[2]), &packets_2nd, &bytes_2nd)) {
				printf("PPE_SERVICE Throughput Sample 2 Failed Service_ID:%d\n", atoi(argv[2]));
				break;
			}

			dbytes = bytes_2nd - bytes;
			dpackets = packets_2nd - packets;
			dtime = NSM_APP_US_PER_SEC;

			rate = nsm_app_rate_calc(dbytes, dtime);
			if (rate < 0) {
				return -1;
			}
			byte_rate = rate;

			rate = nsm_app_rate_calc(dpackets, dtime);
			if (rate < 0) {
				return -1;
			}
			packet_rate = rate;

			printf("nPPE_SERVICE_THROUGHPUT Packet Rate: %" PRIu64 " Bit Rate: %" PRIu64 " Per Second\n", packet_rate, byte_rate * 8);

			break;
	case 'd':
			printf("\nPPE_DROP_STATS:\n");
			if (!sal_nsm_ppe_drop_get(ns, atoi(argv[2]), &packets, &bytes)) {
				printf("PPE_DROP_STATS Failed\n");
			} else {
				printf("DROP Packets Service ID%d: %" PRIu64 " Bytes: %" PRIu64 "\n", atoi(argv[2]), packets, bytes);
			}
			break;
	case 'e':
			printf("\nPPE_QUEUE_STATS:\n");
			for (int i = 0; i < DROP_QUEUE_IDX_MAX; i++) {
				if (!sal_nsm_ppe_queue_drop_get(ns, atoi(argv[2]), i, &packets, &bytes)) {
					printf("PPE_QUEUE_STATS Failed\n");
				} else {
					printf("QUEUE_ID:%d QUEUE_IDX:%d Packets: %" PRIu64 " Bytes: %" PRIu64 "\n", i, atoi(argv[2]), packets, bytes);
				}
			}
			break;
	case 'f':
			if (!nsm_app_param_get_ipaddr(argv[2], 4, (void *)&flow_ip[0])) {
				printf("Bad flow_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[3], 2, (void *)&flow_port)) {
				printf("Bad flow_port\n");
				break;
			}
			if (!nsm_app_param_get_ipaddr(argv[4], 4, (void *)&return_ip[0])) {
				printf("Bad return_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[5], 2, (void *)&return_port)) {
				printf("Bad return_port\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[6], 1, (void *)&protocol)) {
				printf("Bad protocol\n");
				break;
			}

			flow_ip[0] = ntohl(flow_ip[0]);
			return_ip[0] = ntohl(return_ip[0]);

			printf("\nPPE_V4_FLOW_STATS:\n ");
			printf("\n5-Tuple Address %pI4h:%d %pI4h:%d %d\n", &flow_ip[0], flow_port, &return_ip[0], return_port, protocol);

			if (!sal_nsm_ppe_v4_flow_stats_get(ns, flow_ip[0], flow_port, return_ip[0], return_port, protocol, &packets, &bytes)) {
				printf("PPE_V4_FLOW_STATS Failed\n");
			} else {
				printf("PPE_V4_FLOW_STATS Packets: %" PRIu64 " Bytes: %" PRIu64 "\n", packets, bytes);
			}
			break;
	case 'g':
			if (!nsm_app_param_get_ipaddr(argv[2], 4, (void *)&flow_ip[0])) {
				printf("Bad flow_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[3], 2, (void *)&flow_port)) {
				printf("Bad flow_port\n");
				break;
			}
			if (!nsm_app_param_get_ipaddr(argv[4], 4, (void *)&return_ip[0])) {
				printf("Bad return_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[5], 2, (void *)&return_port)) {
				printf("Bad return_port\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[6], 1, (void *)&protocol)) {
				printf("Bad protocol\n");
				break;
			}

			flow_ip[0] = ntohl(flow_ip[0]);
			return_ip[0] = ntohl(return_ip[0]);

			printf("\nPPE_FLOW_V4_THROUGHPUT_STATS:\n");
			printf("\n5-Tuple Address %pI4h:%d %pI4h:%d %d\n", &flow_ip[0], flow_port, &return_ip[0], return_port, protocol);

			if (!sal_nsm_ppe_v4_flow_stats_get(ns, flow_ip[0], flow_port, return_ip[0], return_port, protocol, &packets, &bytes)) {
				printf("nPPE_FLOW_V4_THROUGHPUT_STATS Sample 1 Failed\n");
				break;
			}

			sleep(1);

			/*
			 * Grab Second sample
			 */
			if (!sal_nsm_ppe_v4_flow_stats_get(ns, flow_ip[0], flow_port, return_ip[0], return_port, protocol, &packets_2nd, &bytes_2nd)) {
				printf("PPE_FLOW_V4_THROUGHPUT_STATS Sample 2 Failed\n");
				break;
			}

			dbytes = bytes_2nd - bytes;
			dpackets = packets_2nd - packets;
			dtime = NSM_APP_US_PER_SEC;

			rate = nsm_app_rate_calc(dbytes, dtime);
			if (rate < 0) {
				return -1;
			}
			byte_rate = rate;

			rate = nsm_app_rate_calc(dpackets, dtime);
			if (rate < 0) {
				return -1;
			}
			packet_rate = rate;

			printf("PPE_FLOW_V4_THROUGHPUT_STATS Packet Rate: %" PRIu64 " Bit Rate: %" PRIu64 " Per Second\n", packet_rate, byte_rate * 8);

			break;
	case 'h':
			if (!nsm_app_param_get_ipaddr(argv[2], 16, (void *)&flow_ip[0])) {
				printf("Bad flow_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[3], 2, (void *)&flow_port)) {
				printf("Bad flow_port\n");
				break;
			}
			if (!nsm_app_param_get_ipaddr(argv[4], 16, (void *)&return_ip[0])) {
				printf("Bad return_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[5], 2, (void *)&return_port)) {
				printf("Bad return_port\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[6], 1, (void *)&protocol)) {
				printf("Bad protocol\n");
				break;
			}

			flow_ip[0] = ntohl(flow_ip[0]);
			flow_ip[0] = ntohl(flow_ip[1]);
			flow_ip[0] = ntohl(flow_ip[2]);
			flow_ip[0] = ntohl(flow_ip[3]);
			return_ip[0] = ntohl(return_ip[0]);
			return_ip[0] = ntohl(return_ip[1]);
			return_ip[0] = ntohl(return_ip[2]);
			return_ip[0] = ntohl(return_ip[3]);

			printf("\nPPE_V6_FLOW_STATS:\n ");
			printf("\n5-Tuple Address %pI6h:%d %pI6h:%d %d\n", &flow_ip[0], flow_port, &return_ip[0], return_port, protocol);
			if (!sal_nsm_ppe_v6_flow_stats_get(ns, flow_ip, flow_port, return_ip, return_port, protocol, &packets, &bytes)) {
				printf("PPE_V6_FLOW_STATS Failed\n");
			} else {
				printf("PPE_V6_FLOW_STATS Packets: %" PRIu64 " Bytes: %" PRIu64 "\n", packets, bytes);
			}
			break;
	case 'i':
			if (!nsm_app_param_get_ipaddr(argv[2], 16, (void *)&flow_ip[0])) {
				printf("Bad flow_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[3], 2, (void *)&flow_port)) {
				printf("Bad flow_port\n");
				break;
			}
			if (!nsm_app_param_get_ipaddr(argv[4], 16, (void *)&return_ip[0])) {
				printf("Bad return_ip\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[5], 2, (void *)&return_port)) {
				printf("Bad return_port\n");
				break;
			}
			if (!nsm_app_param_get_int(argv[6], 1, (void *)&protocol)) {
				printf("Bad protocol\n");
				break;
			}

			flow_ip[0] = ntohl(flow_ip[0]);
			flow_ip[0] = ntohl(flow_ip[1]);
			flow_ip[0] = ntohl(flow_ip[2]);
			flow_ip[0] = ntohl(flow_ip[3]);
			return_ip[0] = ntohl(return_ip[0]);
			return_ip[0] = ntohl(return_ip[1]);
			return_ip[0] = ntohl(return_ip[2]);
			return_ip[0] = ntohl(return_ip[3]);

			printf("\nPPE_FLOW_V6_THROUGHPUT_STATS\n");
			printf("\n5-Tuple Address %pI4h:%d %pI4h:%d %d\n", &flow_ip[0], flow_port, &return_ip[0], return_port, protocol);
			if (!sal_nsm_ppe_v6_flow_stats_get(ns, flow_ip, flow_port, return_ip, return_port, protocol, &packets, &bytes)) {
				printf("PPE_FLOW_V6_THROUGHPUT_STATS Sample 1 Failed\n");
			}

			sleep(1);

			/*
			 * Grab Second sample
			 */
			if (!sal_nsm_ppe_v6_flow_stats_get(ns, flow_ip, flow_port, return_ip, return_port, protocol, &packets_2nd, &bytes_2nd)) {
				printf("PPE_FLOW_V6_THROUGHPUT_STATS Sample 2 Failed\n");
				break;
			}

			dbytes = bytes_2nd - bytes;
			dpackets = packets_2nd - packets;
			dtime = NSM_APP_US_PER_SEC;

			rate = nsm_app_rate_calc(dbytes, dtime);
			if (rate < 0) {
				return -1;
			}
			byte_rate = rate;

			rate = nsm_app_rate_calc(dpackets, dtime);
			if (rate < 0) {
				return -1;
			}
			packet_rate = rate;

			printf("PPE_FLOW_V6_THROUGHPUT_STATS Packet Rate: %" PRIu64 " Bit Rate: %" PRIu64 " Per Second\n", packet_rate, byte_rate * 8);

			break;
	case '?':
			printf("\nOption a is for SFE Dump ./nsm_app a arg\n");
			printf("\nOption b is for PPE Service Stat Dump ./nsm_app b SERVICE_ID\n");
			printf("\nOption c is for PPE Service Throughput Dump ./nsm_app c SERVICE_ID\n");
			printf("\nOption d is for PPE Drop Stat Dump ./nsm_app d SERVICE_ID\n");
			printf("\nOption e is for PPE Service Drop Queue Stat Dump ./nsm_app e QUEUE_ID\n");
			printf("\nOption f is for PPE V4 Flow Stat Dump ./nsm_app f FLOW_IP FLOW_PORT RETURN_IP RETURN_PORT PROTOCOL\n");
			printf("\nOption g is for PPE V4 Flow Throughtput Dump ./nsm_app g FLOW_IP FLOW_PORT RETURN_IP RETURN_PORT PROTOCOL\n");
			printf("\nOption h is for PPE V6 Flow Stat Dump ./nsm_app h FLOW_IP FLOW_PORT RETURN_IP RETURN_PORT PROTOCOL\n");
			printf("\nOption i is for PPE V6 Flow Throughtput Dump ./nsm_app i FLOW_IP FLOW_PORT RETURN_IP RETURN_PORT PROTOCOL\n");
			break;
	default:
			printf("\nUse ? with command\n");
			break;
	}

	/*
	 * Freeing NSM socket.
	 */
	sal_nsm_socket_free(ns);
	return 0;
}
