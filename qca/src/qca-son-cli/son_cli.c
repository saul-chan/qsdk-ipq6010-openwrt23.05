/*
 * @File: son_cli.c
 *
 * @Abstract: Son CLI Application Main File
 *
 * @Notes:
 *
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include <qcatools_lib.h>         /* library for common headerfiles */
#include <sys/time.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <time.h>
#include <glob.h>                 /* Including glob.h for glob() function used in find_pid() */
#include <sys/queue.h>
#include <pthread.h>
#include <bufrd.h>
#include <dbg.h>
#ifndef BUILD_X86
#include <netlink/attr.h>
#endif
//#include <ath_ald_external.h>
#include <ieee80211_external.h>
#include <wlan_son_band_steering_api.h>
#include <linux/wireless.h>
#include <unistd.h>
#include <evloop.h>
#include "son_cli.h"
#include <wlan_son_ioctl.h>

#ifdef SON_MEMORY_DEBUG
#include "meminfo.h"
extern int enable_debug;
#endif

extern int de_event_socket;
struct evloopTimeout timer_soncli;
struct socket_context sock_ctx;

#define ISVALID_LINKID(link_id) (link_id != MLO_INVALID_LINK_ID)

#ifdef SON_MEMORY_DEBUG
user_input_data_t in_data;
#endif

void (*usage_cmd)(void);
int init_socket_context (struct socket_context *sock_ctx,
        int cmd_sock_id, int event_sock_id)
{
    int err = 0;
#if UMAC_SUPPORT_CFG80211
    if (sock_ctx->cfg80211) {
        sock_ctx->cfg80211_ctxt.pvt_cmd_sock_id = cmd_sock_id;
        sock_ctx->cfg80211_ctxt.pvt_event_sock_id = event_sock_id;

        err = wifi_init_nl80211(&(sock_ctx->cfg80211_ctxt));
        if (err) {
            errx(1, "unable to create NL socket");
            return -EIO;
        }
    }
#endif
    return 0;
}


int send_generic_command_cfg80211(struct socket_context *sock_ctx, const char *ifname, int maincmd, int cmd, char *data, int data_len, int link_id)
{
    int res;
    struct cfg80211_data buffer;
    buffer.data = (void *)data;
    buffer.length = data_len;
    buffer.callback = NULL;
    buffer.parse_data = 0;

    res = wifi_cfg80211_send_generic_command(&(sock_ctx->cfg80211_ctxt), maincmd, cmd, ifname, link_id /*MLO_INVALID_LINK_ID*/, (char *)&buffer, data_len);
    if (res < 0) {
        fprintf( stdout, " %s : send NL command failed \n",__func__);
        return res;
    }

    return 0;
}

int send_command_get_cfg80211( struct socket_context *sock_ctx, const char *ifname, int op, int *data)
{
    int ret;
    struct cfg80211_data buffer;
    buffer.data = data;
    buffer.length = sizeof(int);
    buffer.parse_data = 0;
    buffer.callback = NULL;
    buffer.parse_data = 0;
    if((ret=wifi_cfg80211_send_getparam_command(&(sock_ctx->cfg80211_ctxt), QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS, op, ifname, (char *)&buffer, sizeof(int))) < 0)
    {
        return -EIO;
    }
    return 0;
}


int send_command (struct socket_context *sock_ctx, const char *ifname, uint8_t link, void *buf,
        size_t buflen, void (*callback) (struct cfg80211_data *arg), int cmd, int ioctl_cmd)
{
#if UMAC_SUPPORT_CFG80211
    int msg;
    struct cfg80211_data buffer;
#endif

    if (sock_ctx->cfg80211) {
#if UMAC_SUPPORT_CFG80211
        buffer.data = buf;
        buffer.length = buflen;
        buffer.callback = callback;
        buffer.parse_data = 0;
        msg = wifi_cfg80211_send_generic_command(&(sock_ctx->cfg80211_ctxt),
                cmd,
                ioctl_cmd, ifname, MLO_INVALID_LINK_ID, (char *)&buffer, buflen);
        if (msg < 0) {
            printf("Could not send NL command\n");
            return -EIO;
        }
        return buffer.length;
#endif
    }

    return 0;
}

void destroy_socket_context (struct socket_context *sock_ctx)
{
#if UMAC_SUPPORT_CFG80211
    if (sock_ctx->cfg80211) {
        wifi_destroy_nl80211(&(sock_ctx->cfg80211_ctxt));
    }
#endif
    return;
}

typedef unsigned char uint8_t;

uint8_t delim_unix = ':';
uint8_t delim_win = '-';
uint8_t delim_ipv4_unix = '.';

#define IS_VALID(s)         (((s >= '0') && (s <= '9')) || ((s >= 'A') && (s <= 'F')) || ((s >= 'a') && (s <= 'f')))
#define TO_UPPER(s)         (((s >= 'a') && (s <= 'z')) ? (s - 32) : s)
#define IS_NUM(c)           ((c >= '0') && (c <= '9'))
#define CHAR_TO_HEX_INT(c)  (IS_NUM(c) ? (c - '0') : (TO_UPPER(c) - 55))
#define CHAR_TO_DEC(c)      (c - '0')

/*
 * soncli_mac_aton: converts mac address string into integer array format
 * @str: pointer to mac string which is to be converted
 * @out: pointer to address for output to be stored
 * @len: length of the string to be converted
 * returns 0 for fail else len if success
 */
int soncli_mac_aton(const char *str, unsigned char *out, int len)
{
    const uint8_t *tmp = NULL;
    int plen = 0;
    uint8_t delim;
    int num = 0;
    int flag_num_valid = 0;
    int ccnt = 0;

    while((*str == ' ') || (*str == '\t') || (*str == '\n')) {
        ++str;
    }

    tmp = (uint8_t *) str;

    while ((*tmp != delim_unix) && (*tmp != delim_win)) {
        ++tmp;
    }

    delim = *tmp;

    tmp = (uint8_t *)str;

    while (*tmp != '\0') {

        if (IS_VALID(*tmp) && (++ccnt < 3)) {
            num = (num * 16) + CHAR_TO_HEX_INT(*tmp);
            flag_num_valid = 1;
        } else if ((*tmp == delim) && (flag_num_valid)) {
            *out = num;
            out++;
            num = 0;
            plen++;
            ccnt = 0;
            flag_num_valid = 0;

            if (plen > len) {
                return 0;
            }
        } else {
            return 0;
        }
        tmp++;
    }

    if (*tmp == '\0') {
        *out = num;
        plen++;
    }

    if (plen == len) {
        return len;
    } else {
        return 0;
    }

}

static void usage(void)
{
    fprintf(stderr, "usage: son_cli  version\n");
    fprintf(stderr, "usage: son_cli  athx unittest\n");
    fprintf(stderr, "usage: son_cli athx cmd args\n");
    fprintf(stderr, "cmd: [bsteer_getparams bsteer_setparams] \n");
    fprintf(stderr, "cmd: [bsteer_getdbgparams bsteer_setdbgparams] \n");
    fprintf(stderr, "cmd: [bsteer_enable] [bsteer_enable_events] \n");
    fprintf(stderr, "cmd: [bsteer_getoverload bsteer_setoverload] \n");
    fprintf(stderr, "cmd: [bsteer_getrssi] \n");
    fprintf(stderr, "cmd: [bsteer_setproberespwh bsteer_getproberespwh] \n");
    fprintf(stderr, "cmd: [bsteer_setauthallow] \n");
    fprintf(stderr, "cmd: [bsteer_setsteering] \n");
    fprintf(stderr, "cmd: [set_innetwork_2g get_innetwork_2g] \n");
    fprintf(stderr, "cmd: [mapget_hwmode_opclass] \n");
    fprintf(stderr, "cmd: [mapget_oper_channels] \n");
    fprintf(stderr, "cmd: [mapget_opclass_info] \n");
}

#ifdef SON_MEMORY_DEBUG
static void usage_memdbg(void)
{
	fprintf(stderr, "usage: son_cli  memdbg \n");
	fprintf(stderr, "-m arg  :   print memory usage informaton of selected SON application\n");
	fprintf(stderr, "            (arg: wsplcd|wsplcd-lan, wsplcd-guest, hyd|hyd-lan, hyd-guest, lbd)\n");
	fprintf(stderr, "-p n    :   repeat every n seconds, if repeat option '-r' is specified, then default report interval is 10 seconds \n");
	fprintf(stderr, "-t n    :   repeat n times for configured report interval using '-p' option \n");
	fprintf(stderr, "-d      :   print memory info debug information\n\n");
	fprintf(stderr, "-h      :   print this usage and exit\n");
	fprintf(stderr, "-v      :   print version information and exit\n");
	fprintf(stderr, "\nExample : \n");
	fprintf(stderr, "    'son_cli memdbg -m wsplcd-lan' - print WSPLCD memory usage information one time and exit\n");
	fprintf(stderr, "    'son_cli memdbg -m wsplcd-lan -t 20' - print WSPLCD memory usage information repeatedly for 20 times, for every 10 seconds, \n");
	fprintf(stderr, "    'son_cli memdbg -m wsplcd-lan -p 60 -t 10' - print WSPLCD memory usage information for every 60 seconds, repeatedly for 10 times\n\n");

}
#endif

static void usage_bsteer_getparams(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>]  bsteer_getparams \n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_cb_block(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] sta_stats_timer <100/200/300/400/500>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_setparams(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] bsteer_setparams <inact_normal> <inact_overload> <util_sample_period>\n"
            "           <util_average_num_samples> <inactive_rssi_xing_high_threshold> <inactive_rssi_xing_low_threshold>\n"
            "           <low_rssi_crossing_threshold> <inact_check_period> <tx_rate_low_crossing_threshold>\n"
            "           <tx_rate_high_crossing_threshold> <rssi_rate_low_crossing_threshold> <rssi_rate_high_crossing_threshold>\n"
            "           <ap_steer_rssi_low_threshold> <interference_detection_enable> <client_classification_enable>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_getdbgparams(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] bsteer_getdbgparams \n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_setdbgparams(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] bsteer_setdbgparams "
            "<raw_chan_util_log_enable> "
                "<raw_rssi_log_enable> <raw_tx_rate_log_enable>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_enable(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_enable <enable_flag>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_enable_events()
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_enable_events <enable_flag>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_setoverload(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_setoverload <overload_flag>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_getoverload(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_getoverload\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_getrssi(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_getrssi <dstmac> <num_samples>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_setproberespwh(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_setproberespwh <dstmac> <enable_flag>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_getproberespwh(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_getproberespwh <dstmac>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_setauthallow(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_setauthallow <dstmac> <enable_flag>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_getdatarateinfo(void)
{
    fprintf(stderr, "Usage: son_cli athX/mldX [link_id <link_id>] bsteer_getdatarateinfo <dstmac>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_bsteer_setsteering(void)
{
    fprintf(stderr, "Usage, son_cli athX/mldX [link_id <link_id>] bsteer_setsteering <dstmac> <steering_flag>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}
static void usage_set_innetwork_2g(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] set_innetwork_2g [mac] channel\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

static void usage_get_innetwork_2g(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] get_innetwork_2g [0/1] [num_entries/channel]\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

/*
 * bsteer_set_cmd: bsteer_setdbgparams: set debug parameters for band steering
 *                 bsteer_setparams: set parameters for band steering
 *                 bsteer_setsteering: set band steering with destination mac address and steering flag
 *                 bsteer_setoverload: set overload using overload flag
 *                 bsteer_setproberespwh: set probe responses withheld
 *                 currently handles: bsteer_setdbgparams, bsteer_setparams, bsteer_setsteering, bsteer_setoverload, bsteer_setproberespwh
 * @ifname: interface name
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * @req_cmd: enum value for function specific command
 * return -1 on error; otherwise return 0 on success
 */
int bsteer_set_cmd (const char *ifname, int argc, char *argv[], int req_cmd, int link_id)
{
    mesh_dbg_req_t mesh_req = {0};
    int arg_num, cmd;
    u_int8_t index = 0;
    switch (req_cmd) {
        case MESH_BSTEERING_SET_PARAMS:
            cmd = MESH_BSTEERING_SET_PARAMS;
            arg_num = 18;
            usage_cmd = usage_bsteer_setparams;
            break;

        case MESH_BSTEERING_SET_DBG_PARAMS:
            cmd = MESH_BSTEERING_SET_DBG_PARAMS;
            arg_num = 6;
            usage_cmd = usage_bsteer_setdbgparams;
            break;

        case MESH_BSTEERING_SET_STEERING:
            cmd = MESH_BSTEERING_SET_STEERING;
            arg_num = 5;
            usage_cmd = usage_bsteer_setsteering;
            break;

        case MESH_BSTEERING_SET_OVERLOAD:
            cmd = MESH_BSTEERING_SET_OVERLOAD;
            arg_num = 4;
            usage_cmd = usage_bsteer_setoverload;
            break;

        case MESH_BSTEERING_SET_PROBE_RESP_WH:
            cmd = MESH_BSTEERING_SET_PROBE_RESP_WH;
            arg_num = 5;
            usage_cmd = usage_bsteer_setproberespwh;
            break;

        default:
            return -1;
    }

    if (ISVALID_LINKID(link_id)) {
        if (argc != arg_num + 2) {
            usage_cmd();
            return -1;
        }
    } else {
        if (argc != arg_num) {
            usage_cmd();
            return -1;
        }
    }

    mesh_req.mesh_cmd = cmd;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    switch (req_cmd) {
        case MESH_BSTEERING_SET_PARAMS:
            mesh_req.mesh_data.bsteering_param.inactivity_timeout_overload = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
            mesh_req.mesh_data.bsteering_param.utilization_sample_period = ISVALID_LINKID(link_id) ? atoi(argv[7]):atoi(argv[5]);
            mesh_req.mesh_data.bsteering_param.utilization_average_num_samples = ISVALID_LINKID(link_id) ? atoi(argv[8]):atoi(argv[6]);
            mesh_req.mesh_data.bsteering_param.low_rssi_crossing_threshold = ISVALID_LINKID(link_id) ? atoi(argv[11]):atoi(argv[9]);
            mesh_req.mesh_data.bsteering_param.inactivity_check_period = ISVALID_LINKID(link_id) ? atoi(argv[12]):atoi(argv[10]);
            mesh_req.mesh_data.bsteering_param.low_tx_rate_crossing_threshold = ISVALID_LINKID(link_id) ? atoi(argv[13]):atoi(argv[11]);
            mesh_req.mesh_data.bsteering_param.low_rate_rssi_crossing_threshold = ISVALID_LINKID(link_id) ? atoi(argv[15]):atoi(argv[13]);
            mesh_req.mesh_data.bsteering_param.interference_detection_enable = ISVALID_LINKID(link_id) ? atoi(argv[18]):atoi(argv[16]);
            mesh_req.mesh_data.bsteering_param.client_classification_enable = ISVALID_LINKID(link_id) ? atoi(argv[19]):atoi(argv[17]);
            for (index = 0; index < BSTEERING_MAX_CLIENT_CLASS_GROUP; index++) {
                mesh_req.mesh_data.bsteering_param.inactivity_timeout_normal[index] = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
                mesh_req.mesh_data.bsteering_param.inactive_rssi_xing_high_threshold[index] = ISVALID_LINKID(link_id) ? atoi(argv[9]):atoi(argv[7]);
                mesh_req.mesh_data.bsteering_param.inactive_rssi_xing_low_threshold[index] = ISVALID_LINKID(link_id) ? atoi(argv[10]):atoi(argv[8]);
                mesh_req.mesh_data.bsteering_param.high_tx_rate_crossing_threshold[index] = ISVALID_LINKID(link_id) ? atoi(argv[14]):atoi(argv[12]);
                mesh_req.mesh_data.bsteering_param.high_rate_rssi_crossing_threshold[index] = ISVALID_LINKID(link_id) ? atoi(argv[16]):atoi(argv[14]);
                mesh_req.mesh_data.bsteering_param.ap_steer_rssi_xing_low_threshold[index] = ISVALID_LINKID(link_id) ? atoi(argv[17]):atoi(argv[15]);
            }
            break;

        case MESH_BSTEERING_SET_DBG_PARAMS:
            mesh_req.mesh_data.bsteering_dbg_param.raw_chan_util_log_enable = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
            mesh_req.mesh_data.bsteering_dbg_param.raw_rssi_log_enable = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
            mesh_req.mesh_data.bsteering_dbg_param.raw_tx_rate_log_enable = ISVALID_LINKID(link_id) ? atoi(argv[7]):atoi(argv[5]);
            break;

        case MESH_BSTEERING_SET_STEERING:
            if (ISVALID_LINKID(link_id)) {
                if (!soncli_mac_aton(argv[5], &mesh_req.dstmac[0], MAC_ADDR_LEN)) {
                    errx(1, "Invalid mac address");
                    return -1;
                }
            } else {
                if (!soncli_mac_aton(argv[3], &mesh_req.dstmac[0], MAC_ADDR_LEN)) {
                    errx(1, "Invalid mac address");
                    return -1;
                }
            }
            mesh_req.mesh_data.value = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
            break;

        case MESH_BSTEERING_SET_OVERLOAD:
            mesh_req.mesh_data.value = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
            break;

        case MESH_BSTEERING_SET_PROBE_RESP_WH:
            if (ISVALID_LINKID(link_id)) {
                if (!soncli_mac_aton(argv[5], &mesh_req.dstmac[0], 6)) {
                    errx(1, "Invalid mac address");
                    return -1;
                }
            } else {
                if (!soncli_mac_aton(argv[3], &mesh_req.dstmac[0], 6)) {
                    errx(1, "Invalid mac address");
                    return -1;
                }
            }
            mesh_req.mesh_data.value = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
            break;

        default:
            return -1;
    }
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
    return 0;
}

/*
 * bsteer_get_cmd: bsteer_getparams: get parameters for band steering
 *                 bsteer_getdbgparams: get debug parameters for band steering
 *                 bsteer_getdatarateinfo: get data rate info of provided dest mac addr for band steering
 *                 bsteer_getoverload: get overload information
 *                 bsteer_getproberespwh: get and print probe responses withheld
 *                 currently handles: bsteer_getparams, bsteer_getdbgparams, bsteer_getdatarateinfo, bsteer_getoverload, bsteer_getproberespwh
 * @ifname: interface name
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * @req_cmd: enum value for function specific command
 * return -1 on error; otherwise return 0 on success
 */
int bsteer_get_cmd(const char *ifname, int argc, char *argv[], int req_cmd, int link_id)
{
    mesh_dbg_req_t mesh_req;
    int arg_num, cmd;
    u_int8_t index = 0;

    switch (req_cmd) {
        case MESH_BSTEERING_GET_PARAMS:
            cmd = MESH_BSTEERING_GET_PARAMS;
            arg_num = 3;
            usage_cmd = usage_bsteer_getparams;
            break;

        case MESH_BSTEERING_GET_DBG_PARAMS:
            cmd = MESH_BSTEERING_GET_DBG_PARAMS;
            arg_num = 3;
            usage_cmd =  usage_bsteer_getdbgparams;
            break;

        case MESH_BSTEERING_GET_DATARATE_INFO:
            cmd = MESH_BSTEERING_GET_DATARATE_INFO;
            arg_num = 4;
            usage_cmd = usage_bsteer_getdatarateinfo;
            break;

        case MESH_BSTEERING_GET_OVERLOAD:
            cmd = MESH_BSTEERING_GET_OVERLOAD;
            arg_num = 3;
            usage_cmd = usage_bsteer_getoverload;
            break;

        case MESH_BSTEERING_GET_PROBE_RESP_WH:
            cmd = MESH_BSTEERING_GET_PROBE_RESP_WH;
            arg_num = 4;
            usage_cmd = usage_bsteer_getproberespwh;
            break;

        default:
            return -1;
    }

    if (ISVALID_LINKID(link_id)) {
        if (argc != arg_num + 2) {
            usage_cmd();
            return -1;
        }
    } else {
        if (argc != arg_num) {
            usage_cmd();
            return -1;
        }
    }

    mesh_req.mesh_cmd = cmd;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_REQUIRED;
    if (req_cmd == MESH_BSTEERING_GET_DATARATE_INFO) {
        if (ISVALID_LINKID(link_id)) {
            if (!soncli_mac_aton(argv[5], &mesh_req.dstmac[0], 6)) {
                errx(1, "Invalid mac address");
                return -1;
            }
        } else {
            if (!soncli_mac_aton(argv[3], &mesh_req.dstmac[0], 6)) {
                errx(1, "Invalid mac address");
                return -1;
            }
        }
    }
//    send_command(&sock_ctx, ifname, &mesh_req, sizeof(mesh_req), NULL, QCA_NL80211_VENDOR_SUBCMD_DBGREQ, IEEE80211_IOCTL_DBGREQ);
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

    switch (req_cmd) {
        case MESH_BSTEERING_GET_PARAMS:
            printf("Band steering parameters: \n");
            printf("-------------------------- ------------\n");
            printf("Overload inactivity timeout: %u s\n",
                    mesh_req.mesh_data.bsteering_param.inactivity_timeout_overload);
            printf("Utilization sampling period: %u s\n",
                    mesh_req.mesh_data.bsteering_param.utilization_sample_period);
            printf("Utilization num samples to average: %u\n",
                    mesh_req.mesh_data.bsteering_param.utilization_average_num_samples);
            printf("Low RSSI crossing threshold: %u dB\n",
                    mesh_req.mesh_data.bsteering_param.low_rssi_crossing_threshold);
            printf("Inactivity check interval: %u s\n",
                    mesh_req.mesh_data.bsteering_param.inactivity_check_period);
            printf("Active steering low threshold: Tx rate %u Kbps, RSSI %u dB\n",
                    mesh_req.mesh_data.bsteering_param.low_tx_rate_crossing_threshold,
                    mesh_req.mesh_data.bsteering_param.low_rate_rssi_crossing_threshold);
            printf("Interference detection enable: %u\n",
                    mesh_req.mesh_data.bsteering_param.interference_detection_enable);
            printf("Client classification enable: %u\n",
                    mesh_req.mesh_data.bsteering_param.client_classification_enable);
            for (index = 0; index < BSTEERING_MAX_CLIENT_CLASS_GROUP; index++) {
                printf("Normal inactivity timeout [Class %d]: %u s\n",
                        index, mesh_req.mesh_data.bsteering_param.inactivity_timeout_normal[index]);
                printf("Inactive RSSI crossing high threshold [Class %d]: %u dB\n",
                        index, mesh_req.mesh_data.bsteering_param.inactive_rssi_xing_high_threshold[index]);
                printf("Inactive RSSI crossing low threshold [Class %d]: %u dB\n",
                        index, mesh_req.mesh_data.bsteering_param.inactive_rssi_xing_low_threshold[index]);
                printf("Active steering high threshold [Class %d]: Tx rate %u Kbps, RSSI %u dB\n",
                        index, mesh_req.mesh_data.bsteering_param.high_tx_rate_crossing_threshold[index],
                        mesh_req.mesh_data.bsteering_param.high_rate_rssi_crossing_threshold[index]);
                printf("AP steering low RSSI threshold [Class %d]: %u dB\n",
                        index, mesh_req.mesh_data.bsteering_param.ap_steer_rssi_xing_low_threshold[index]);
            }
            break;

        case MESH_BSTEERING_GET_DBG_PARAMS:
            printf("Band steering debug parameters: \n");
            printf("-------------------------- ------------\n");
            printf("Enable raw channel utilization logging: %s\n",
                    mesh_req.mesh_data.bsteering_dbg_param.raw_chan_util_log_enable ?
                    "yes" : "no");
            printf("Enable raw RSSI measurement logging: %s\n",
                    mesh_req.mesh_data.bsteering_dbg_param.raw_rssi_log_enable ?
                    "yes" : "no");
            break;

        case MESH_BSTEERING_GET_DATARATE_INFO:
            printf("Data rate info for %s on %s: Max Bandwidth: %u, Num streams: %u, "
                    "PHY mode: %u, Max MCS: %u, Max TX power: %u\n",
                    ISVALID_LINKID(link_id) ? argv[5]:argv[3], ifname, mesh_req.mesh_data.bsteering_datarate_info.max_chwidth,
                    mesh_req.mesh_data.bsteering_datarate_info.num_streams,
                    mesh_req.mesh_data.bsteering_datarate_info.phymode,
                    mesh_req.mesh_data.bsteering_datarate_info.max_MCS,
                    mesh_req.mesh_data.bsteering_datarate_info.max_txpower);
            break;

        case MESH_BSTEERING_GET_OVERLOAD:
            printf("%s is %soverloaded\n", ifname,
                    mesh_req.mesh_data.value ? "" : "not ");
            break;

        case MESH_BSTEERING_GET_PROBE_RESP_WH:
            printf("Probe responses withheld for %s on %s: %s\n", ISVALID_LINKID(link_id) ? argv[5]:argv[3], ifname,
                    mesh_req.mesh_data.value ? "yes" : "no");
            break;

        default:
            return -1;
    }
    return 0;
}

int bsteer_sta_stats_timer(const char *ifname, int argc, char *argv[], int req_cmd, int link_id)
{
    mesh_dbg_req_t mesh_req;
    int err, cmd;

    switch (req_cmd) {
        case MESH_BSTEERING_EVENT_REPORTING_DELAY_INTERVAL:
            cmd = MESH_BSTEERING_EVENT_REPORTING_DELAY_INTERVAL;
            usage_cmd = usage_bsteer_cb_block;
            break;

        default:
            return -1;
    }

    if (ISVALID_LINKID(link_id)) {
        if (argc != 6) {
            usage_cmd();
            return -1;
        }
    } else {
        if (argc != 4) {
            usage_cmd();
            return -1;
        }
    }

    mesh_req.mesh_data.bsteering_event_delay_interval = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
    mesh_req.mesh_cmd = cmd;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    err = send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
    if (req_cmd == MESH_BSTEERING_EVENT_REPORTING_DELAY_INTERVAL) {
        if (err < 0) {
            fprintf(stderr, "band steering callback config failed\n");
        }
    }
    return 0;
}


/*
 * bsteer_enable: enable band steering and band steering events using enable flag
 *                currently supports: bsteer_enable_events, bsteer_enable
 * @ifname: interface name
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * @req_cmd: enum value for function specific command
 * return -1 on error; otherwise return 0 on success
 */
int bsteer_enable(const char *ifname, int argc, char *argv[], int req_cmd, int link_id)
{
    mesh_dbg_req_t mesh_req;
    int err, cmd;

    switch (req_cmd) {
        case MESH_BSTEERING_ENABLE:
            cmd = MESH_BSTEERING_ENABLE;
            usage_cmd = usage_bsteer_enable;
            break;

        case MESH_BSTEERING_ENABLE_EVENTS:
            cmd = MESH_BSTEERING_ENABLE_EVENTS;
            usage_cmd = usage_bsteer_enable_events;
            break;

        default:
            return -1;
    }

    if (ISVALID_LINKID(link_id)) {
        if (argc != 6) {
            usage_cmd();
            return -1;
        }
    } else {
        if (argc != 4) {
            usage_cmd();
            return -1;
        }
    }

    mesh_req.mesh_data.value = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
    mesh_req.mesh_cmd = cmd;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    err = send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
    if (req_cmd == MESH_BSTEERING_ENABLE) {
        if (err < 0) {
            fprintf(stderr, "band steering enable failed\n");
        }
    }
    return 0;
}

/*
 * bsteer_getrssi: get RSSI for given dest mac addr and number of samples
 * @ifname: interface name
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 */
void bsteer_getrssi(const char *ifname, int argc, char *argv[], int link_id)
{
    mesh_dbg_req_t mesh_req;

    if (ISVALID_LINKID(link_id)) {
        if (argc != 7) {
            usage_bsteer_getrssi();
            return;
        }
    } else {
        if (argc != 5) {
            usage_bsteer_getrssi();
            return;
        }
    }

    mesh_req.mesh_cmd = MESH_BSTEERING_GET_RSSI;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    if (ISVALID_LINKID(link_id)) {
        if (!soncli_mac_aton(argv[5], &mesh_req.dstmac[0], 6)) {
            errx(1, "Invalid mac address");
            return;
        }
    } else {
        if (!soncli_mac_aton(argv[3], &mesh_req.dstmac[0], 6)) {
            errx(1, "Invalid mac address");
            return;
        }
    }

    mesh_req.mesh_data.value = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
}

/*
 * bsteer_setauthallow: set authentication allowance for band steering
 * @ifname: interface name
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * return -1 on error; otherwise return 0 on success
 */
int bsteer_setauthallow(const char *ifname, int argc, char *argv[], int link_id)
{
    mesh_dbg_req_t mesh_req = {0};

    if (ISVALID_LINKID(link_id)) {
        if (argc != 7) {
            usage_bsteer_setauthallow();
            return -1;
        }
    } else {
        if (argc != 5) {
            usage_bsteer_setauthallow();
            return -1;
        }
    }

    mesh_req.mesh_cmd = MESH_BSTEERING_SET_AUTH_ALLOW;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    if (ISVALID_LINKID(link_id)) {
        if (!soncli_mac_aton(argv[5], &mesh_req.dstmac[0], 6)) {
            errx(1, "Invalid mac address");
            return -1;
        }
    } else {
        if (!soncli_mac_aton(argv[3], &mesh_req.dstmac[0], 6)) {
            errx(1, "Invalid mac address");
            return -1;
        }
    }

    mesh_req.mesh_data.value = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

    return 0;
}

/* bsteer_set_sta_stats_update_interval_da: set interval for DA sta stats
 * @ifname: interface name
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * return -1 on error; otherwise return 0 on success
 */
int bsteer_set_sta_stats_update_interval_da(const char *ifname, int argc, char *argv[], int link_id)
{
    mesh_dbg_req_t mesh_req = {0};

    if (ISVALID_LINKID(link_id)) {
        if (argc != 6) {
            return -1;
        }
    } else {
        if (argc != 4) {
            return -1;
        }
    }

    memset(&mesh_req, 0, sizeof(struct mesh_dbg_req_t));
    mesh_req.mesh_cmd = MESH_BSTEERING_SET_DA_STAT_INTV;
    mesh_req.mesh_data.bsteering_sta_stats_update_interval_da = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
    mesh_req.needs_reply = DBGREQ_REPLY_IS_NOT_REQUIRED;

    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
    return 0;
}

static void set_innetwork_2g_mac(const char *ifname, int argc, char *argv[], int link_id) {
    mesh_dbg_req_t mesh_req = {0};

    if (ISVALID_LINKID(link_id)) {
        if ((argc < 7) || (argc > 7)) {
            usage_set_innetwork_2g();
            return;
        }
    } else {
        if ((argc < 5) || (argc > 5)) {
            usage_set_innetwork_2g();
            return;
        }
    }

    memset(&mesh_req, 0, sizeof(struct mesh_dbg_req_t));
    mesh_req.mesh_cmd = IEEE80211_DBGREQ_BSTEERING_SETINNETWORK_2G;
    mesh_req.mesh_data.param[0] = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);

    if (ISVALID_LINKID(link_id)) {
        if (!soncli_mac_aton(argv[5], &mesh_req.dstmac[0], 6)) {
            errx(1, "Invalid mac address");
            return;
        }
    } else {
        if (!soncli_mac_aton(argv[3], &mesh_req.dstmac[0], 6)) {
            errx(1, "Invalid mac address");
            return;
        }
    }
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
    return;
}

static void get_innetwork_2g_mac(const char *ifname, int argc, char *argv[], int link_id) {
    int len, i, param, num_entries = 0, ch;
    mesh_dbg_req_t mesh_req = {0};

    if (ISVALID_LINKID(link_id)) {
        if ((argc < 7) || (argc > 7)) {
            usage_get_innetwork_2g();
            return;
        }
    } else {
        if ((argc < 5) || (argc > 5)) {
            usage_get_innetwork_2g();
            return;
        }
    }

    memset(&mesh_req, 0, sizeof(struct mesh_dbg_req_t));
    mesh_req.mesh_cmd = IEEE80211_DBGREQ_BSTEERING_GETINNETWORK_2G;
    mesh_req.mesh_data.innetwork_2g_req.index = 0;
    mesh_req.mesh_data.innetwork_2g_req.data_addr = &num_entries;
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);
    fprintf(stdout, " Total Entries : %d\n", num_entries);

    param = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
    if (!param) {

        len = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
        if (len > num_entries || !len) {
            fprintf(stderr, " Enter valid entry/entry less than : %d\n", num_entries);
            return;
        }

        struct ieee80211_bsteering_in_network_2G_table *table_2g = NULL;
        table_2g = (void *)malloc(len * sizeof(struct ieee80211_bsteering_in_network_2G_table));
        if (NULL == table_2g) {
            errx(1, "insufficient memory \n");
            return;
        }

        (void)memset(table_2g, 0, len * sizeof(struct ieee80211_bsteering_in_network_2G_table));
        mesh_req.mesh_data.innetwork_2g_req.index = len;
        mesh_req.mesh_data.innetwork_2g_req.ch = 0;
        mesh_req.mesh_data.innetwork_2g_req.data_addr = table_2g;
        send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

        fprintf(stdout, " Number of Entries : %d\n", table_2g->total_index);
        if (table_2g->total_index != 0) {
            fprintf(stdout, "-------------------------\n");
            fprintf(stdout, " STA MAC         STA CH\n");
            fprintf(stdout, "-------------------------\n");
            for (i = 0; i < table_2g->total_index; i++) {
                fprintf(stdout, "%2x:%2x:%2x:%2x:%2x:%2x  %d\n", table_2g[i].mac_addr[0],
                        table_2g[i].mac_addr[1], table_2g[i].mac_addr[2], table_2g[i].mac_addr[3],
                        table_2g[i].mac_addr[4], table_2g[i].mac_addr[5], table_2g[i].ch);
            }
        } else
            fprintf(stdout, "no data\n");
        free(table_2g);
    } else {
        ch = ISVALID_LINKID(link_id) ? atoi(argv[6]):atoi(argv[4]);
        if (ch > 11 || ch < 1) {
            fprintf(stderr, " Enter valid 2g channel \n");
            return;
        }

        u_int8_t zeroMAC[6] = {0, 0, 0, 0, 0, 0};
        u_int8_t mac_addr[128][QDF_MAC_ADDR_SIZE] = {0};
        mesh_req.mesh_data.innetwork_2g_req.index = num_entries;
        mesh_req.mesh_data.innetwork_2g_req.ch = ch;
        mesh_req.mesh_data.innetwork_2g_req.data_addr = mac_addr;
        send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

        if (num_entries != 0) {
            fprintf(stdout, "-------------------------\n");
            fprintf(stdout, " STA MAC         STA CH\n");
            fprintf(stdout, "-------------------------\n");
            for (i = 0; i < num_entries; i++) {
                if (!memcmp(zeroMAC, mac_addr[i], QDF_MAC_ADDR_SIZE)) {
                    continue;
                }
                fprintf(stdout, "%2x:%2x:%2x:%2x:%2x:%2x  %d\n", mac_addr[i][0], mac_addr[i][1],
                        mac_addr[i][2], mac_addr[i][3], mac_addr[i][4], mac_addr[i][5], ch);
            }
        } else
            fprintf(stdout, "no data\n");
    }
    return;
}

#if QCA_SUPPORT_SON
static void usage_gethwmode_opclass(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] mapget_hwmode_opclass\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

/**
 * get_hwmode_opclass: Get opclass information for the current operating
 * hwmode.
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * return -1 on error; otherwise return 0 on success
 */
static int get_hwmode_opclass(const char *ifname, int argc, char *argv[], int link_id)
{
#define MAX_PARAM  3
    mesh_dbg_req_t mesh_req = {0};
    mapapcap_t *apcap = NULL;
    uint8_t i, j;

    if (ISVALID_LINKID(link_id)) {
        if ((argc != MAX_PARAM + 2)) {
            usage_gethwmode_opclass();
            return -1;
        }
    } else {
        if ((argc != MAX_PARAM)) {
            usage_gethwmode_opclass();
            return -1;
        }
    }

    memset(&mesh_req, 0, sizeof(struct mesh_dbg_req_t));
    mesh_req.mesh_cmd = MESH_MAP_OPCLASS_FOR_CUR_HWMODE;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_REQUIRED;
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

    apcap = &mesh_req.mesh_data.mapapcap;

    for (i = 0; i <  apcap->hwcap.num_supported_op_classes; i++) {
        fprintf(stdout, "\n opclass: %d, tx_pwr: %d, num_non_oper_chan: %d\n", apcap->hwcap.opclasses[i].opclass, apcap->hwcap.opclasses[i].max_tx_pwr_dbm, apcap->hwcap.opclasses[i].num_non_oper_chan);
        for (j = 0; j < apcap->hwcap.opclasses[i].num_non_oper_chan; j++)
            fprintf(stdout, "\n non_oper_chan_num[%d] = %d\n", j, apcap->hwcap.opclasses[i].non_oper_chan_num[j]);
    }

#undef MAX_PARAM
    return 0;
}

static void usage_oper_channels(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] mapget_oper_channels <opclass>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

/**
 * mapget_opclass_channels: Get channel information for the current opclass.
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * return -1 on error; otherwise return 0 on success
 */
static int get_oper_channels(const char *ifname, int argc, char *argv[], int link_id)
{
#define MAX_PARAM  4
    mesh_dbg_req_t mesh_req = {0};
    map_op_chan_t *opchan = NULL;
    uint8_t i;

    if (ISVALID_LINKID(link_id)) {
        if ((argc != MAX_PARAM + 2)) {
            usage_oper_channels();
            return -1;
        }
    } else {
        if ((argc != MAX_PARAM)) {
            usage_oper_channels();
            return -1;
        }
    }

    memset(&mesh_req, 0, sizeof(struct mesh_dbg_req_t));
    mesh_req.mesh_cmd = MESH_MAP_GET_OP_CHANNELS;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_REQUIRED;
    mesh_req.mesh_data.map_op_chan.opclass = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

    opchan = &mesh_req.mesh_data.map_op_chan;

    fprintf(stdout, "\n opclass: %d, ch_width: %d, num_oper_chan: %d\n", opchan->opclass, opchan->ch_width, opchan->num_oper_chan);
    for (i = 0; i <  opchan->num_oper_chan; i++) {
        fprintf(stdout, "\n channel: %d\n", opchan->oper_chan_num[i]);
    }

#undef MAX_PARAM
    return 0;
}

static void usage_get_opclass_info(void)
{
    fprintf(stderr, "usage: son_cli athX/mldX [link_id <link_id>] mapget_opclass_info <opclass>\n");
    fprintf(stderr, "Note: link_id is used only when mldX is used \n");
}

/**
 * get_opclass_info: Get operable class info
 * @argc: total number of command line arguments
 * @argv: values of command line arguments
 * return -1 on error; otherwise return 0 on success
 */
static int get_opclass_info(const char *ifname, int argc, char *argv[], int link_id)
{
#define MAX_PARAM  4
    mesh_dbg_req_t mesh_req = {0};
    map_op_class_t *opclass = NULL;
    uint8_t i;

    if (ISVALID_LINKID(link_id)) {
        if ((argc != MAX_PARAM + 2)) {
            usage_get_opclass_info();
            return -1;
        }
    } else {
        if ((argc != MAX_PARAM)) {
            usage_get_opclass_info();
            return -1;
        }
    }

    memset(&mesh_req, 0, sizeof(struct mesh_dbg_req_t));
    mesh_req.mesh_cmd = MESH_MAP_GET_OP_CLASS_INFO;
    mesh_req.needs_reply = DBGREQ_REPLY_IS_REQUIRED;
    mesh_req.mesh_data.map_op_class.opclass = ISVALID_LINKID(link_id) ? atoi(argv[5]):atoi(argv[3]);
    send_generic_command_cfg80211(&sock_ctx, ifname, QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION, QCA_NL80211_VENDOR_SUBCMD_MESH_SET_GET_CONFIG, (char *)&mesh_req, sizeof(mesh_req), link_id);

    opclass = &mesh_req.mesh_data.map_op_class;

    fprintf(stdout, "\n chwidth %d, secChanLoc %d, num_chan %d \n", opclass->ch_width, opclass->sc_loc, opclass->num_chan );
    for (i = 0; i <  opclass->num_chan; i++) {
        fprintf(stdout, "\n Channel[%d]: %d \n", i, opclass->channels[i]);
    }

#undef MAX_PARAM
    return 0;
}

#endif


#ifdef SON_MEMORY_DEBUG
void son_mem_input(int argc, char **argv, struct user_input_data *input)
{
    int c;
    int port_num;
    char procname[APP_CONFIG_LEN];
    memset(procname, 0, APP_CONFIG_LEN);

    for (;;) {

	    argc--;
	    argv++;
	    c = getopt(argc, argv, "i:m:p:r:dvh" );

	    if (c < 0) {
		    usage_memdbg();
		    break;
	    }

        switch (c) {

            case 'm':
                strlcpy(procname, optarg, APP_CONFIG_LEN);
                debug_print("%s: config[%s]", __func__, procname);
                if (strcmp(procname, "hyd-lan") == 0 || strcmp(procname, "hyd") == 0)
                    port_num = SON_CLI_HYD_LAN_PORT;
                else if (strcmp(procname, "hyd-Guest") == 0 || strcmp(procname, "hyd-guest") == 0)
                    port_num = SON_CLI_HYD_GUEST_PORT;
                else if (strcmp(procname, "wsplcd-lan") == 0 || strcmp(procname, "wsplcd") == 0)
                    port_num = SON_CLI_WSPLCD_LAN_PORT;
                else if (strcmp(procname, "wsplcd-Guest") == 0 || strcmp(procname, "wsplcd-guest") == 0)
                    port_num = SON_CLI_WSPLCD_GUEST_PORT;
                else if (strcmp(procname, "lbd") == 0)
                    port_num = SON_CLI_LBD_PORT;
                else if (strcmp(procname, "ezmesh-lan") == 0)
                    port_num = SON_CLI_EZMESH_PORT;
                else
                    port_num = INVALID_PORT_NUM;
                input->memdbg_cli_port = port_num;
                break;
            case 'p':
                input->memdbg_report_interval = atoi(optarg);
                break;
            case 't':
                input->memdbg_repeat_count = atoi(optarg);
                break;
            case 'd':
                enable_debug = 1;
                break;
            case 'h':
		usage_memdbg();
                exit(0);
                break;
	    default:
		printf("Invalid memdbg option \n");
		break;
	}
    }
    // set default report interval to 10 seconds
    if (input->memdbg_report_interval == 0 && input->memdbg_repeat_count > 1)
        input->memdbg_report_interval = 10;
    debug_print("Configuration Settings:\nPort Number:[%d]\nReport Interval:[%d]\nRepeat Count:%d\n", input->memdbg_cli_port, input->memdbg_report_interval, input->memdbg_repeat_count);
}
#endif

#ifdef SON_MEMORY_DEBUG
/* Initialize memory debug parameters */
void initialize_input_parameter(user_input_data_t *input)
{
    input->memdbg_cli_port = 0;
    input->memdbg_report_interval = 0;
    input->memdbg_repeat_count = 1;
}
#endif

extern void  SON_TestCases(char *ifname);

int main(int argc, char *argv[])
{
    char *ifname, *cmd;
    char *res;
    int link_id = MLO_INVALID_LINK_ID;
    char *optLinkId;

    sock_ctx.cfg80211 = 1;
    init_socket_context(&sock_ctx, DEFAULT_NL80211_CMD_SOCK_ID, DEFAULT_NL80211_EVENT_SOCK_ID);
#ifdef SON_MEMORY_DEBUG
    // Initialize memory debug parameters
    initialize_input_parameter(&in_data);
#endif

    if (argc < 2)
    {
	    printf("%s:%d",__func__,__LINE__);
#ifdef SON_MEMORY_DEBUG
	    usage_memdbg();
#endif
	    usage();
	    exit(-1);
    }

#ifdef SON_MEMORY_DEBUG
    if (strcmp( argv[1],"memdbg") == 0) {
	    son_mem_input(argc, argv, &in_data);
	    if (in_data.memdbg_cli_port) {
		    // Retrieve memory usage information
		    retrieve_mem_info(&in_data);
	    }
	    exit(0);
    }
#endif

    if (streq(argv[1], "version")) {
	    printf("son_cli: version %s\n", SON_CLI_VERSION);
    } else if (streq(argv[1], "-h")) {
#ifdef SON_MEMORY_DEBUG
	    usage_memdbg();
#endif
	    usage();
	    exit(0);
    } else {
	    ifname = argv[1];
	    if (!strncmp(argv[2], "link_id", strlen("link_id"))) {
	        optLinkId = argv[3];
	        cmd = argv[4];
	        link_id =  atoi(optLinkId);
	    } else {
	        cmd = argv[2];
	        res = strstr(ifname, "ath");
	        if (!res) {
		    printf("Provide proper interface name ath/(mldx and link_id) \n");
		    usage();
		    goto soncli_end;
	        }
	    }
	    if (streq(cmd, "unittest")) {
		    if (strlen(ifname) > 0) {
			    SON_TestCases(ifname);
		    }
	    }
	    else if (streq(cmd, "bsteer_getparams")) {
		    bsteer_get_cmd(ifname, argc, argv, MESH_BSTEERING_GET_PARAMS, link_id);
	    } else if (streq(cmd, "bsteer_setparams")) {
		    bsteer_set_cmd(ifname, argc, argv, MESH_BSTEERING_SET_PARAMS, link_id);
	    } else if (streq(cmd, "bsteer_getdbgparams")) {
		    bsteer_get_cmd(ifname, argc, argv, MESH_BSTEERING_GET_DBG_PARAMS, link_id);
	    } else if (streq(cmd, "bsteer_setdbgparams")) {
		    bsteer_set_cmd(ifname, argc, argv, MESH_BSTEERING_SET_DBG_PARAMS, link_id);
	    } else if (streq(cmd, "bsteer_enable")) {
		    bsteer_enable(ifname, argc, argv, MESH_BSTEERING_ENABLE, link_id);
        } else if (streq(cmd, "sta_stats_timer")) {
            bsteer_sta_stats_timer(ifname, argc, argv, MESH_BSTEERING_EVENT_REPORTING_DELAY_INTERVAL, link_id);
        } else if (streq(cmd, "bsteer_enable_events")) {
		    bsteer_enable(ifname, argc, argv, MESH_BSTEERING_ENABLE_EVENTS, link_id);
	    } else if (streq(cmd, "bsteer_setoverload")) {
		    bsteer_set_cmd(ifname, argc, argv, MESH_BSTEERING_SET_OVERLOAD, link_id);
	    } else if (streq(cmd, "bsteer_getoverload")) {
		    bsteer_get_cmd(ifname, argc, argv, MESH_BSTEERING_GET_OVERLOAD, link_id);
	    } else if (streq(cmd, "bsteer_getrssi")) {
		    bsteer_getrssi(ifname, argc, argv, link_id);
	    }  else if (streq(cmd, "bsteer_setproberespwh")) {
		    bsteer_set_cmd(ifname, argc, argv, MESH_BSTEERING_SET_PROBE_RESP_WH, link_id);
	    } else if (streq(cmd, "bsteer_getproberespwh")) {
		    bsteer_get_cmd(ifname, argc, argv, MESH_BSTEERING_GET_PROBE_RESP_WH, link_id);
	    }  else if (streq(cmd, "bsteer_setauthallow")) {
		    bsteer_setauthallow(ifname, argc, argv, link_id);
	    } else if (streq(cmd, "bsteer_getdatarateinfo")) {
		    bsteer_get_cmd(ifname, argc, argv, MESH_BSTEERING_GET_DATARATE_INFO, link_id);
	    } else if (streq(cmd, "bsteer_setsteering")) {
		    bsteer_set_cmd(ifname, argc, argv, MESH_BSTEERING_SET_STEERING, link_id);
	    } else if (streq(cmd, "set_innetwork_2g")) {
		    set_innetwork_2g_mac(ifname, argc, argv, link_id);
	    } else if (streq(cmd, "get_innetwork_2g")) {
		    get_innetwork_2g_mac(ifname, argc, argv, link_id);
	    } else if (streq(cmd, "mapget_hwmode_opclass")) {
		     get_hwmode_opclass(ifname, argc, argv, link_id);
	    } else if (streq(cmd, "mapget_oper_channels")) {
		     get_oper_channels(ifname, argc, argv, link_id);
	    } else if (streq(cmd, "mapget_opclass_info")) {
		     get_opclass_info(ifname, argc, argv, link_id);
	    }
	    else {
#ifdef SON_MEMORY_DEBUG
		    usage_memdbg();
#endif
		    usage();
	    }
    }
soncli_end:
    destroy_socket_context(&sock_ctx);
    return 0;
}
#if 0
static void help()
{
    printf("Wrong Query\n");
    printf(" Format :: son_cli athX <option> \n");
    printf("\n___________________________________________________________________________\n The following are the options \n ___________________________________________________________________________");
    printf("\nOption             \t\tEVENT ");
    printf("\ninst_rssi            \t\tATH_EVENT_BSTEERING_PROBE_REQ");
    printf("\nchan_util          \t\tATH_EVENT_BSTEERING_CHAN_UTIL");
    printf("\npeer_info          \t\tATH_EVENT_BSTEERING_NODE_ASSOCIATED");
    printf("\ntx_auth_rejec      \t\tATH_EVENT_BSTEERING_TX_AUTH_FAIL");
    printf("\nactivity_change_alert  \t\tATH_EVENT_BSTEERING_CLIENT_ACTIVITY_CHANGE");
    printf("\nrssi_threshold_crossed \t\tATH_EVENT_BSTEERING_CLIENT_RSSI_CROSSING");
    printf("\nrssi_measurement       \t\tATH_EVENT_BSTEERING_CLIENT_RSSI_MEASUREMENT");
    printf("\nbeacon_frame_report    \t\tATH_EVENT_BSTEERING_RRM_REPORT");
    printf("\nwnm_report             \t\tATH_EVENT_BSTEERING_WNM_EVENT");
    printf("\ntx_rate_threshold      \t\tATH_EVENT_BSTEERING_CLIENT_TX_RATE_CROSSING");
    printf("\ntx_rate_meas_rpt       \t\tATH_EVENT_BSTEERING_DBG_TX_RATE");
    printf("\ntx_power_change        \t\tATH_EVENT_BSTEERING_TX_POWER_CHANGE");
    printf("\nsmps_node_update       \t\tATH_EVENT_BSTEERING_SMPS_UPDATE");
    printf("\nopmode_update          \t\tATH_EVENT_BSTEERING_OPMODE_UPDATE");
    printf("\nrssi_thresh_map        \t\tATH_EVENT_BSTEERING_MAP_CLIENT_RSSI_CROSSING");
    printf("\nsta_stats              \t\tATH_EVENT_BSTEERING_STA_STATS");
    printf("\nhyd_assoc              \t\tIEEE80211_ALD_ASSOCIATE");
    printf("\nhyd_buffull            \t\tIEEE80211_ALD_BUFFULL_WRN");
    printf("\nbsteering_params       \t\tIEEE80211_DBGREQ_BSTEERING_GET_PARAMS");
    printf("\ndatarate_info          \t\tIEEE80211_DBGREQ_BSTEERING_GET_DATARATE_INFO");
    printf("\ncac_state              \t\tIEEE80211_PARAM_GET_CAC");
    printf("\nacs_state              \t\tIEEE80211_PARAM_GET_ACS");
    printf("\nsecond_center_freq     \t\tIEEE80211_PARAM_SECOND_CENTER_FREQ");
    printf("\nbandwidth              \t\tIEEE80211_PARAM_BANDWIDTH");
    printf("\nchextoffset            \t\tIEEE80211_PARAM_CHEXTOFFSET");
    printf("\nchwidth                \t\tIEEE80211_PARAM_CHWIDTH");
    printf("\nmixedbh_ulrate         \t\tIEEE80211_PARAM_WHC_MIXEDBH_ULRATE");
    printf("\nsmart_monitor          \t\tIEEE80211_PARAM_RX_FILTER_SMART_MONITOR");
    printf("\nssid                   \t\tSIOCGIWESSID");
    printf("\nwap                    \t\tSIOCGIWAP");
    printf("\nfreq                   \t\tSIOCGIWFREQ");
    printf("\nchan_info              \t\tIEEE80211_IOCTL_GETCHANINFO");
    printf("\nchan160_info           \t\tIEEE80211_WLANCONFIG_GETCHANINFO_160");
    printf("\ncfg_sta_stats          \t\tIEEE80211_IOCTL_STA_STATS");
    printf("\nsta_stats_info         \t\tIEEE80211_IOCTL_STA_INFO");
    printf("\nnetif_carrier_status   \t\tIEEE80211_IOCTL_");
    printf("\nFor cfg_sta_stats and sta_stats_info please enter MAC of the station as the 4th argument\n");
}
#endif

